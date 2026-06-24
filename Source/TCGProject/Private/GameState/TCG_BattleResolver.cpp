#include "GameState/TCG_BattleResolver.h"
#include "GameState/TCG_GameState.h"

namespace
{
	constexpr bool bLogBattleResolverFlow = false;

	static void ResolveDestroyedUnitHandResponses(ATCG_GameState* GameState, int32 DestroyedUnitOwnerPlayerIndex, const FGuid& DestroyerCardInstanceId)
	{
		if (!GameState || !GameState->IsValidPlayerIndex(DestroyedUnitOwnerPlayerIndex) || !DestroyerCardInstanceId.IsValid()) return;

		const FTCGCardInstance* DestroyerCard = GameState->FindCardInstance(DestroyerCardInstanceId);
		if (!DestroyerCard || DestroyerCard->Location != ETCGCardLocation::Board) return;

		TArray<FTCGCardInstance> HandCards;
		GameState->GetCardsInHand(DestroyedUnitOwnerPlayerIndex, HandCards);
		if (HandCards.Num() <= 0) return;

		TArray<FTCGEffectChainEntry> Chain;
		for (const FTCGCardInstance& HandCard : HandCards)
		{
			TArray<FTCGCardEffectRef> EffectRefs;
			GameState->GetPrintedEffectRefsForCard(HandCard, EffectRefs);
			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnYourUnitDestroyedByBattle))
				{
					GameState->AddCardEffectRefToChain(Chain, HandCard.CardInstanceId, DestroyerCardInstanceId, EffectRef);
				}
			}
		}

		if (Chain.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Battle: Destroyed unit hand responses Player=%d Destroyer=%s Count=%d"), DestroyedUnitOwnerPlayerIndex, *DestroyerCard->CardDefinitionId.ToString(), Chain.Num());
			GameState->ResolveEffectChain(Chain);
		}
	}

	static void ResolveDestroyedUnitByBattleTriggers(ATCG_GameState* GameState, const FGuid& WinnerCardInstanceId)
	{
		if (!GameState || !WinnerCardInstanceId.IsValid()) return;

		if (const FTCGCardInstance* WinnerCard = GameState->FindCardInstance(WinnerCardInstanceId))
		{
			TArray<FTCGEffectChainEntry> Chain;
			TArray<FTCGCardEffectRef> EffectRefs;
			GameState->GetPrintedEffectRefsForCard(*WinnerCard, EffectRefs);

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnDestroyedUnitByBattle))
				{
					GameState->AddCardEffectRefToChain(Chain, WinnerCardInstanceId, WinnerCardInstanceId, EffectRef);
				}
			}

			GameState->ResolveEffectChain(Chain);
		}
	}
}

bool UTCG_BattleResolver::ResolveBattleBetweenZones(ATCG_GameState* GameState, FName Player0ZoneId, FName Player1ZoneId)
{
	if (!GameState) return false;

	const FTCGCardInstance* Player0Card = GameState->FindTopCardInZone(Player0ZoneId);
	const FTCGCardInstance* Player1Card = GameState->FindTopCardInZone(Player1ZoneId);
	if (!Player0Card || !Player1Card) return false;

	const FGuid Player0WinnerId = Player0Card->CardInstanceId;
	const FGuid Player1WinnerId = Player1Card->CardInstanceId;
	const FGuid Player0LoserStackId = Player0Card->StackId;
	const FGuid Player1LoserStackId = Player1Card->StackId;

	const int32 Player0Attack = GameState->GetFinalAttack(Player0Card->CardInstanceId);
	const int32 Player1Attack = GameState->GetFinalAttack(Player1Card->CardInstanceId);

	if (Player0Attack > Player1Attack)
	{
		GameState->MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);
		ResolveDestroyedUnitHandResponses(GameState, 1, Player0WinnerId);
		ResolveDestroyedUnitByBattleTriggers(GameState, Player0WinnerId);
	}
	else if (Player1Attack > Player0Attack)
	{
		GameState->MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);
		ResolveDestroyedUnitHandResponses(GameState, 0, Player1WinnerId);
		ResolveDestroyedUnitByBattleTriggers(GameState, Player1WinnerId);
	}
	else
	{
		GameState->MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);
		GameState->MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);
	}

	return true;
}

bool UTCG_BattleResolver::ResolveBattlePhase(ATCG_GameState* GameState)
{
	if (!GameState) return false;

	if (GameState->HasPendingDiscardChoice())
	{
		if (bLogBattleResolverFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase paused PendingDiscard Player=%d Count=%d"), GameState->PendingDiscardChoice.PlayerIndex, GameState->PendingDiscardChoice.RequiredCount);
		}
		return false;
	}

	bool bResolvedAnyBattle = false;
	for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
	{
		const int32 AttackerPlayerIndex = FieldIndex % 2 == 0 ? 0 : 1;
		const int32 DefenderPlayerIndex = 1 - AttackerPlayerIndex;
		const FName AttackerZoneId = ATCG_GameState::GetFieldZoneId(AttackerPlayerIndex, FieldIndex);
		const FTCGCardInstance* AttackerCard = GameState->FindTopCardInZone(AttackerZoneId);
		if (!AttackerCard)
		{
			if (bLogBattleResolverFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoAttacker"), FieldIndex, AttackerPlayerIndex);
			continue;
		}

		FName DefenderZoneId = NAME_None;
		const FTCGCardInstance* DefenderCard = nullptr;
		for (int32 SearchOffset = 0; SearchOffset < ATCG_GameState::FieldZoneCount; ++SearchOffset)
		{
			const int32 CandidateFieldIndex = (FieldIndex + SearchOffset) % ATCG_GameState::FieldZoneCount;
			const FName CandidateZoneId = ATCG_GameState::GetFieldZoneId(DefenderPlayerIndex, CandidateFieldIndex);
			const FTCGCardInstance* CandidateCard = GameState->FindTopCardInZone(CandidateZoneId);
			if (CandidateCard)
			{
				DefenderZoneId = CandidateZoneId;
				DefenderCard = CandidateCard;
				break;
			}
		}

		if (!DefenderCard || DefenderZoneId.IsNone())
		{
			if (bLogBattleResolverFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoDefender"), FieldIndex, AttackerPlayerIndex);
			continue;
		}

		const FGuid AttackerCardId = AttackerCard->CardInstanceId;
		const FGuid DefenderCardId = DefenderCard->CardInstanceId;

		TArray<FTCGEffectChainEntry> AttackChain;
		TArray<FTCGCardEffectRef> AttackEffectRefs;
		GameState->GetPrintedEffectRefsForCard(*AttackerCard, AttackEffectRefs);

		for (const FTCGCardEffectRef& EffectRef : AttackEffectRefs)
		{
			if (GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnAttack))
			{
				GameState->AddCardEffectRefToChain(AttackChain, AttackerCardId, DefenderCardId, EffectRef);
			}
		}

		const bool bAttackChainResolved = GameState->ResolveEffectChain(AttackChain);

		const FTCGCardInstance* AttackerAfterAttackEffects = GameState->FindCardInstance(AttackerCardId);
		const FTCGCardInstance* DefenderAfterAttackEffects = GameState->FindCardInstance(DefenderCardId);

		if (!AttackerAfterAttackEffects
			|| AttackerAfterAttackEffects->Location != ETCGCardLocation::Board
			|| !DefenderAfterAttackEffects
			|| DefenderAfterAttackEffects->Location != ETCGCardLocation::Board)
		{
			if (bLogBattleResolverFlow)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("TCG Debug: Battle damage skipped Field=%d Attacker=P%d OnAttackResolved=%s Reason=UnitLeftBoard"),
					FieldIndex,
					AttackerPlayerIndex,
					bAttackChainResolved ? TEXT("true") : TEXT("false"));
			}

			bResolvedAnyBattle = true;
			continue;
		}

		const FGuid AttackerStackId = AttackerAfterAttackEffects->StackId;
		const FGuid DefenderStackId = DefenderAfterAttackEffects->StackId;
		const int32 AttackerAttack = GameState->GetFinalAttack(AttackerCardId);
		const int32 DefenderAttack = GameState->GetFinalAttack(DefenderCardId);

		if (AttackerAttack > DefenderAttack)
		{
			GameState->MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
			ResolveDestroyedUnitHandResponses(GameState, DefenderPlayerIndex, AttackerCardId);
			ResolveDestroyedUnitByBattleTriggers(GameState, AttackerCardId);
		}
		else if (DefenderAttack > AttackerAttack)
		{
			GameState->MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			ResolveDestroyedUnitHandResponses(GameState, AttackerPlayerIndex, DefenderCardId);
			ResolveDestroyedUnitByBattleTriggers(GameState, DefenderCardId);
		}
		else
		{
			GameState->MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			GameState->MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
		}
		bResolvedAnyBattle = true;
	}
	return bResolvedAnyBattle;
}