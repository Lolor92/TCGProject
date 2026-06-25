#include "GameState/TCG_BattleResolver.h"
#include "GameState/TCG_GameState.h"

namespace
{
	constexpr bool bLogBattleResolverFlow = false;

	static void ReindexEffectChain(TArray<FTCGEffectChainEntry>& Chain)
	{
		for (int32 Index = 0; Index < Chain.Num(); ++Index)
		{
			Chain[Index].ChainIndex = Index + 1;
		}
	}


static bool EffectRefHasStepForBattleResolver(const FTCGCardEffectRef& EffectRef, ETCGEffectStepType StepType)
{
for (const FTCGEffectStep& Step : EffectRef.Steps)
{
if (Step.StepType == StepType)
{
return true;
}
}

return false;
}

static bool EffectRefCanNegateOpponentAttackActivation(const FTCGCardEffectRef& EffectRef)
{
const bool bHasLegacyNegate =
EffectRefHasStepForBattleResolver(EffectRef, ETCGEffectStepType::BanishSourceNegateOpponentAttackEffectActivation);

const bool bHasGenericNegate =
EffectRefHasStepForBattleResolver(EffectRef, ETCGEffectStepType::BanishSource)
&& EffectRefHasStepForBattleResolver(EffectRef, ETCGEffectStepType::NegateActivation);

return bHasLegacyNegate || bHasGenericNegate;
}

	static int32 ResolveGraveyardNegatesForOpponentAttackEffects(ATCG_GameState* GameState, int32 AttackingPlayerIndex, TArray<FTCGEffectChainEntry>& Chain, const TArray<int32>& OpponentEffectEntryIndices)
	{
		if (!GameState || !GameState->IsValidPlayerIndex(AttackingPlayerIndex) || Chain.Num() <= 0 || OpponentEffectEntryIndices.Num() <= 0) return 0;

		TArray<FTCGCardInstance> GraveyardCards;
		GameState->GetCardsInLocation(AttackingPlayerIndex, ETCGCardLocation::Graveyard, GraveyardCards);
		if (GraveyardCards.Num() <= 0) return 0;

		TArray<int32> SortedEntryIndices = OpponentEffectEntryIndices;
		SortedEntryIndices.Sort();

		TSet<FGuid> UsedResponseCards;
		int32 NegatedCount = 0;

		for (int32 ReverseIndex = SortedEntryIndices.Num() - 1; ReverseIndex >= 0; --ReverseIndex)
		{
			const int32 ChainArrayIndex = SortedEntryIndices[ReverseIndex];
			if (!Chain.IsValidIndex(ChainArrayIndex)) continue;

			const FTCGEffectChainEntry TargetEntry = Chain[ChainArrayIndex];
			const FTCGCardInstance* TargetSourceCard = GameState->FindCardInstance(TargetEntry.SourceCardInstanceId);
			const FName TargetSourceDefinitionId = TargetSourceCard ? TargetSourceCard->CardDefinitionId : TargetEntry.SourceCardDefinitionId;

			for (const FTCGCardInstance& GraveyardCard : GraveyardCards)
			{
				if (UsedResponseCards.Contains(GraveyardCard.CardInstanceId)) continue;
				if (GraveyardCard.OwnerPlayerIndex != AttackingPlayerIndex || GraveyardCard.Location != ETCGCardLocation::Graveyard) continue;

				TArray<FTCGCardEffectRef> ResponseEffects;
				GameState->GetPrintedEffectRefsForCard(GraveyardCard, ResponseEffects);
				bool bHasNegateResponse = false;
				for (const FTCGCardEffectRef& ResponseEffect : ResponseEffects)
				{
					if (GameState->DoesCardEffectMatchTrigger(ResponseEffect, ETCGEffectTrigger::OnOpponentUnitEffectActivatedWhenYourUnitAttacks)
						&& EffectRefCanNegateOpponentAttackActivation(ResponseEffect))
					{
						bHasNegateResponse = true;
						break;
					}
				}

				if (!bHasNegateResponse) continue;

				const FName ResponseDefinitionId = GraveyardCard.CardDefinitionId;
				const bool bBanishedResponse = GameState->MoveCardToLocation(GraveyardCard.CardInstanceId, ETCGCardLocation::Banish);
				if (!bBanishedResponse) continue;

				Chain.RemoveAt(ChainArrayIndex);
				UsedResponseCards.Add(GraveyardCard.CardInstanceId);
				NegatedCount++;

				UE_LOG(LogTemp, Warning,
					TEXT("TCG Battle: Graveyard negate Player=%d Source=%s Negated=%s"),
					AttackingPlayerIndex,
					*ResponseDefinitionId.ToString(),
					TargetSourceDefinitionId.IsNone() ? TEXT("None") : *TargetSourceDefinitionId.ToString());
				break;
			}
		}

		if (NegatedCount > 0)
		{
			ReindexEffectChain(Chain);
		}

		return NegatedCount;
	}

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

	static void ResolveDestroyedUnitGraveyardResponses(
		ATCG_GameState* GameState,
		int32 DestroyedUnitOwnerPlayerIndex,
		TArray<FTCGEffectChainEntry>& DestroyedResponseChain)
	{
		if (!GameState || DestroyedResponseChain.Num() <= 0)
		{
			return;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Battle: Destroyed unit graveyard responses Player=%d Count=%d"),
			DestroyedUnitOwnerPlayerIndex,
			DestroyedResponseChain.Num());

		GameState->ResolveEffectChain(DestroyedResponseChain);
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
		TArray<FTCGEffectChainEntry> DestroyedResponseChain;
		GameState->BuildYourUnitDestroyedGraveyardResponseChain(1, Player1Card->CardInstanceId, DestroyedResponseChain);

		GameState->MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);
		ResolveDestroyedUnitGraveyardResponses(GameState, 1, DestroyedResponseChain);
		ResolveDestroyedUnitHandResponses(GameState, 1, Player0WinnerId);
		ResolveDestroyedUnitByBattleTriggers(GameState, Player0WinnerId);
	}
	else if (Player1Attack > Player0Attack)
	{
		TArray<FTCGEffectChainEntry> DestroyedResponseChain;
		GameState->BuildYourUnitDestroyedGraveyardResponseChain(0, Player0Card->CardInstanceId, DestroyedResponseChain);

		GameState->MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);
		ResolveDestroyedUnitGraveyardResponses(GameState, 0, DestroyedResponseChain);
		ResolveDestroyedUnitHandResponses(GameState, 0, Player1WinnerId);
		ResolveDestroyedUnitByBattleTriggers(GameState, Player1WinnerId);
	}
	else
	{
		TArray<FTCGEffectChainEntry> Player0DestroyedResponseChain;
		TArray<FTCGEffectChainEntry> Player1DestroyedResponseChain;
		GameState->BuildYourUnitDestroyedGraveyardResponseChain(0, Player0Card->CardInstanceId, Player0DestroyedResponseChain);
		GameState->BuildYourUnitDestroyedGraveyardResponseChain(1, Player1Card->CardInstanceId, Player1DestroyedResponseChain);

		GameState->MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);
		GameState->MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);

		ResolveDestroyedUnitGraveyardResponses(GameState, 0, Player0DestroyedResponseChain);
		ResolveDestroyedUnitGraveyardResponses(GameState, 1, Player1DestroyedResponseChain);
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

		TArray<int32> OpponentAttackEffectEntryIndices;
		TArray<FTCGCardEffectRef> DefenderEffectRefs;
		GameState->GetPrintedEffectRefsForCard(*DefenderCard, DefenderEffectRefs);
		for (const FTCGCardEffectRef& EffectRef : DefenderEffectRefs)
		{
			if (GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnOpponentAttack))
			{
				const int32 BeforeChainCount = AttackChain.Num();
				if (GameState->AddCardEffectRefToChain(AttackChain, DefenderCardId, AttackerCardId, EffectRef) && AttackChain.Num() > BeforeChainCount)
				{
					OpponentAttackEffectEntryIndices.Add(AttackChain.Num() - 1);
				}
			}
		}

		ResolveGraveyardNegatesForOpponentAttackEffects(GameState, AttackerPlayerIndex, AttackChain, OpponentAttackEffectEntryIndices);
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
			TArray<FTCGEffectChainEntry> DestroyedResponseChain;
			GameState->BuildYourUnitDestroyedGraveyardResponseChain(DefenderPlayerIndex, DefenderCardId, DestroyedResponseChain);

			GameState->MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
			ResolveDestroyedUnitGraveyardResponses(GameState, DefenderPlayerIndex, DestroyedResponseChain);
			ResolveDestroyedUnitHandResponses(GameState, DefenderPlayerIndex, AttackerCardId);
			ResolveDestroyedUnitByBattleTriggers(GameState, AttackerCardId);
		}
		else if (DefenderAttack > AttackerAttack)
		{
			TArray<FTCGEffectChainEntry> DestroyedResponseChain;
			GameState->BuildYourUnitDestroyedGraveyardResponseChain(AttackerPlayerIndex, AttackerCardId, DestroyedResponseChain);

			GameState->MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			ResolveDestroyedUnitGraveyardResponses(GameState, AttackerPlayerIndex, DestroyedResponseChain);
			ResolveDestroyedUnitHandResponses(GameState, AttackerPlayerIndex, DefenderCardId);
			ResolveDestroyedUnitByBattleTriggers(GameState, DefenderCardId);
		}
		else
		{
			TArray<FTCGEffectChainEntry> AttackerDestroyedResponseChain;
			TArray<FTCGEffectChainEntry> DefenderDestroyedResponseChain;
			GameState->BuildYourUnitDestroyedGraveyardResponseChain(AttackerPlayerIndex, AttackerCardId, AttackerDestroyedResponseChain);
			GameState->BuildYourUnitDestroyedGraveyardResponseChain(DefenderPlayerIndex, DefenderCardId, DefenderDestroyedResponseChain);

			GameState->MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			GameState->MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);

			ResolveDestroyedUnitGraveyardResponses(GameState, AttackerPlayerIndex, AttackerDestroyedResponseChain);
			ResolveDestroyedUnitGraveyardResponses(GameState, DefenderPlayerIndex, DefenderDestroyedResponseChain);
		}
		bResolvedAnyBattle = true;
	}
	return bResolvedAnyBattle;
}
