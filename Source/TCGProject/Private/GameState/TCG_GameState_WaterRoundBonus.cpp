#include "GameState/TCG_GameState.h"

namespace
{
	const FName DebugCard_WaterRoundBonus = "Debug_Water_RoundBonus";

	FTCGEffectStep MakeRoundBonusStep()
	{
		FTCGEffectStep Step;
		Step.StepType = ETCGEffectStepType::BoostAllOwnUnitsThisRound;
		Step.TargetMode = ETCGEffectTargetMode::Controller;
		Step.Value = 1;
		Step.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		Step.TargetFilter.RequiredLocation = ETCGCardLocation::Board;
		Step.TargetFilter.bRequireTopCard = true;
		Step.TargetFilter.bRequireElement = true;
		Step.TargetFilter.RequiredElement = ETCGCardElement::Water;
		return Step;
	}
}

int32 ATCG_GameState::SendTopDeckCardsToGraveyard(int32 PlayerIndex, int32 Count)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0) return 0;
	int32 SentCount = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FTCGCardInstance* TopDeckCard = nullptr;
		for (FTCGCardInstance& Card : MatchCards)
		{
			if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Deck) continue;
			if (!TopDeckCard || Card.LocationIndex > TopDeckCard->LocationIndex) TopDeckCard = &Card;
		}
		if (!TopDeckCard) break;

		const FGuid SentCardId = TopDeckCard->CardInstanceId;
		const FName SentCardDefinitionId = TopDeckCard->CardDefinitionId;
		if (!MoveCardToLocation(SentCardId, ETCGCardLocation::Graveyard)) break;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Sent top deck card to graveyard Player=%d Card=%s"), PlayerIndex, *SentCardDefinitionId.ToString());
		HandleCardSentToGraveyard(SentCardId);
		SentCount++;
	}
	return SentCount;
}

bool ATCG_GameState::GrantTemporaryAttackToUnits(int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, int32 Amount)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Amount == 0) return false;
	int32 ChangedCount = 0;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Board) continue;
		if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
		if (TargetFilter.bRequireTopCard)
		{
			const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
			if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
		}
		Card.AttackModifier += Amount;
		FTCGTemporaryAttackBoost Record;
		Record.TargetCardInstanceId = Card.CardInstanceId;
		Record.Amount = Amount;
		Record.RoundApplied = RoundNumber;
		TemporaryAttackBoosts.Add(Record);
		ChangedCount++;
	}
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round attack bonus Player=%d Amount=%d Changed=%d Round=%d"), PlayerIndex, Amount, ChangedCount, RoundNumber);
	return ChangedCount > 0;
}

void ATCG_GameState::ClearExpiredTemporaryAttackBoosts()
{
	for (int32 Index = TemporaryAttackBoosts.Num() - 1; Index >= 0; --Index)
	{
		const FTCGTemporaryAttackBoost Record = TemporaryAttackBoosts[Index];
		if (Record.RoundApplied == RoundNumber) continue;
		if (FTCGCardInstance* Card = FindCardInstance(Record.TargetCardInstanceId)) Card->AttackModifier -= Record.Amount;
		TemporaryAttackBoosts.RemoveAt(Index);
	}
}

bool ATCG_GameState::HandleCardSentToGraveyard(const FGuid& SentCardInstanceId)
{
	const FTCGCardInstance* SentCard = FindCardInstance(SentCardInstanceId);
	if (!SentCard || SentCard->Location != ETCGCardLocation::Graveyard || !IsValidPlayerIndex(SentCard->OwnerPlayerIndex)) return false;
	if (SentCard->Element != ETCGCardElement::Water) return false;

	bool bAny = false;
	for (const FTCGCardInstance& SourceCard : MatchCards)
	{
		if (SourceCard.OwnerPlayerIndex != SentCard->OwnerPlayerIndex || SourceCard.Location != ETCGCardLocation::Board) continue;
		TArray<FTCGCardEffectRef> EffectRefs;
		GetPrintedEffectRefsForCard(SourceCard, EffectRefs);
		if (EffectRefs.Num() <= 0 && SourceCard.CardDefinitionId == DebugCard_WaterRoundBonus)
		{
			FTCGCardEffectRef FallbackEffect;
			FallbackEffect.Trigger = ETCGEffectTrigger::OnCardSentToYourGraveyard;
			FallbackEffect.Steps.Add(MakeRoundBonusStep());
			EffectRefs.Add(FallbackEffect);
		}
		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (!DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnCardSentToYourGraveyard)) continue;
			for (const FTCGEffectStep& Step : EffectRef.Steps)
			{
				if (Step.StepType != ETCGEffectStepType::BoostAllOwnUnitsThisRound) continue;
				if (Step.TargetFilter.bRequireElement && SentCard->Element != Step.TargetFilter.RequiredElement) continue;
				bAny |= GrantTemporaryAttackToUnits(SentCard->OwnerPlayerIndex, Step.TargetFilter, Step.Value == 0 ? 1 : Step.Value);
			}
		}
	}
	return bAny;
}

void ATCG_GameState::RunDebugWaterGraveyardBoostScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: WaterRoundBonus scenario start"));
	EndMatch(ETCGMatchResult::Draw);
}
