#include "GameState/TCG_GameState.h"
#include "TimerManager.h"

namespace
{
	const FName DebugCard_TidesCalling = "Debug_Water_TidesCalling";
	const FName DebugCard_DreampoolMirechant = "Dreampool_Mirechant";

	FTCGCardEffectRef MakeTidesCallingFromDeckToGraveyardEffect()
	{
		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnSentToGraveyard;
		EffectRef.bOptional = true;
		FTCGEffectStep PlayStep;
		PlayStep.StepType = ETCGEffectStepType::PlaySourceToEmptyZone;
		PlayStep.TargetMode = ETCGEffectTargetMode::SourceCard;
		EffectRef.Steps.Add(PlayStep);
		return EffectRef;
	}

	FTCGCardEffectRef MakeTidesCallingOnPlayEffect()
	{
		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnPlay;
		EffectRef.bOptional = true;
		FTCGEffectStep MillStep;
		MillStep.StepType = ETCGEffectStepType::SendTopDeckCardToGraveyard;
		MillStep.TargetMode = ETCGEffectTargetMode::Controller;
		EffectRef.Steps.Add(MillStep);
		FTCGEffectStep ThenStep;
		ThenStep.StepType = ETCGEffectStepType::Then;
		EffectRef.Steps.Add(ThenStep);
		FTCGEffectStep RecycleStep;
		RecycleStep.StepType = ETCGEffectStepType::MoveGraveyardCardToBottomDeck;
		RecycleStep.TargetMode = ETCGEffectTargetMode::Controller;
		RecycleStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
		RecycleStep.Value = 1;
		RecycleStep.bRequiresPreviousStepSuccess = true;
		EffectRef.Steps.Add(RecycleStep);
		return EffectRef;
	}

	FTCGCardEffectRef MakeMirechantFromDeckToGraveyardEffect()
	{
		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnSentToGraveyard;
		EffectRef.bOptional = true;
		FTCGEffectStep AttachStep;
		AttachStep.StepType = ETCGEffectStepType::AttachSourceToUnitMaterial;
		AttachStep.TargetMode = ETCGEffectTargetMode::Controller;
		AttachStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
		AttachStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		AttachStep.TargetFilter.RequiredLocation = ETCGCardLocation::Board;
		AttachStep.TargetFilter.bRequireTopCard = true;
		AttachStep.TargetFilter.bRequireElement = true;
		AttachStep.TargetFilter.RequiredElement = ETCGCardElement::Water;
		EffectRef.Steps.Add(AttachStep);
		return EffectRef;
	}

	bool IsTidesCallingCard(const FTCGCardInstance* Card)
	{
		return Card && Card->CardDefinitionId == DebugCard_TidesCalling;
	}

	bool IsMirechantCard(const FTCGCardInstance* Card)
	{
		return Card && Card->CardDefinitionId == DebugCard_DreampoolMirechant;
	}

	int32 GetContinuousAttackBonusForStack(ATCG_GameState* GameState, const FTCGCardInstance& TopCard)
	{
		if (!GameState || !TopCard.StackId.IsValid()) return 0;

		int32 Bonus = 0;
		TArray<FTCGCardInstance> StackCards;
		GameState->GetCardsInStack(TopCard.StackId, StackCards);
		for (const FTCGCardInstance& StackCard : StackCards)
		{
			if (StackCard.StackIndex > TopCard.StackIndex) continue;

			TArray<FTCGCardEffectRef> EffectRefs;
			GameState->GetPrintedEffectRefsForCard(StackCard, EffectRefs);
			if (EffectRefs.Num() <= 0 && IsMirechantCard(&StackCard))
			{
				FTCGCardEffectRef MirechantPassive;
				MirechantPassive.Trigger = ETCGEffectTrigger::None;
				FTCGEffectStep PassiveStep;
				PassiveStep.StepType = ETCGEffectStepType::ModifyAttack;
				PassiveStep.TargetMode = ETCGEffectTargetMode::SourceCard;
				PassiveStep.ValueMode = ETCGEffectValueMode::ElementCardsInControllerGraveyard;
				PassiveStep.TargetFilter.bRequireElement = true;
				PassiveStep.TargetFilter.RequiredElement = ETCGCardElement::Water;
				MirechantPassive.Steps.Add(PassiveStep);
				EffectRefs.Add(MirechantPassive);
			}

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (EffectRef.Trigger != ETCGEffectTrigger::None) continue;
				for (const FTCGEffectStep& Step : EffectRef.Steps)
				{
					if (Step.StepType != ETCGEffectStepType::ModifyAttack) continue;
					if (Step.ValueMode == ETCGEffectValueMode::WaterCardsInControllerGraveyard)
					{
						Bonus += GameState->CountCardsInLocationByElement(TopCard.OwnerPlayerIndex, ETCGCardLocation::Graveyard, ETCGCardElement::Water);
					}
					else if (Step.ValueMode == ETCGEffectValueMode::ElementCardsInControllerGraveyard)
					{
						Bonus += GameState->CountCardsInLocationByElement(TopCard.OwnerPlayerIndex, ETCGCardLocation::Graveyard, Step.TargetFilter.RequiredElement);
					}
				}
			}
		}
		return Bonus;
	}

	int32 GetFinalAttackPreviewWithContinuousEffects(ATCG_GameState* GameState, const FGuid& CardInstanceId)
	{
		const FTCGCardInstance* Card = GameState ? GameState->FindCardInstance(CardInstanceId) : nullptr;
		if (!Card) return 0;
		return Card->BaseAttack + Card->AttackModifier + GameState->GetCardsUnderneathCount(CardInstanceId) + GetContinuousAttackBonusForStack(GameState, *Card);
	}

	void StartQueuedOnPlayChain(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId)
	{
		if (!GameState) return;
		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
		if (!SourceCard || SourceCard->Location != ETCGCardLocation::Board) return;
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Starting queued OnPlay chain Source=%s"), *SourceCard->CardDefinitionId.ToString());
		TArray<FTCGEffectChainEntry> Chain;
		GameState->BuildStackOnPlayEffectChain(SourceCardInstanceId, Chain);
		if (Chain.Num() <= 0 && IsTidesCallingCard(SourceCard)) GameState->AddCardEffectRefToChain(Chain, SourceCardInstanceId, SourceCardInstanceId, MakeTidesCallingOnPlayEffect());
		GameState->ResolveEffectChain(Chain);
	}

	void QueueOnPlayChainAfterCurrentChain(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId)
	{
		if (!GameState || !SourceCardInstanceId.IsValid()) return;
		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Queued OnPlay chain after current chain Source=%s"), SourceCard ? *SourceCard->CardDefinitionId.ToString() : TEXT("None"));
		GameState->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(GameState, [GameState, SourceCardInstanceId]()
		{
			StartQueuedOnPlayChain(GameState, SourceCardInstanceId);
		}));
	}
}

int32 ATCG_GameState::CountCardsInLocationByElement(int32 PlayerIndex, ETCGCardLocation Location, ETCGCardElement Element) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return 0;
	int32 Count = 0;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location && Card.Element == Element) Count++;
	}
	return Count;
}

bool ATCG_GameState::MoveCardToBottomOfDeck(const FGuid& CardInstanceId)
{
	FTCGCardInstance* CardToMove = FindCardInstance(CardInstanceId);
	if (!CardToMove || !IsValidPlayerIndex(CardToMove->OwnerPlayerIndex)) return false;
	const int32 PlayerIndex = CardToMove->OwnerPlayerIndex;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Deck) Card.LocationIndex += 1;
	}
	CardToMove->Location = ETCGCardLocation::Deck;
	CardToMove->LocationIndex = 0;
	CardToMove->ZoneId = NAME_None;
	CardToMove->StackId.Invalidate();
	CardToMove->StackIndex = INDEX_NONE;
	return true;
}

bool ATCG_GameState::AttachSourceCardUnderWaterUnit(const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
	if (!SourceCard || !TargetCard) return false;
	if (SourceCard->Location != ETCGCardLocation::Graveyard) return false;
	if (TargetCard->Location != ETCGCardLocation::Board || !TargetCard->StackId.IsValid()) return false;
	if (SourceCard->OwnerPlayerIndex != TargetCard->OwnerPlayerIndex) return false;

	const FGuid TargetStackId = TargetCard->StackId;
	const FName TargetZoneId = TargetCard->ZoneId;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == TargetStackId) Card.StackIndex += 1;
	}

	SourceCard->Location = ETCGCardLocation::Board;
	SourceCard->LocationIndex = INDEX_NONE;
	SourceCard->ZoneId = TargetZoneId;
	SourceCard->StackId = TargetStackId;
	SourceCard->StackIndex = 0;

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Attached source as material Source=%s Target=%s Zone=%s"), *SourceCard->CardDefinitionId.ToString(), *TargetCard->CardDefinitionId.ToString(), *TargetZoneId.ToString());
	return true;
}

bool ATCG_GameState::PlaySourceCardToEmptyZone(const FGuid& SourceCardInstanceId, FName ZoneId)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || !IsValidPlayerIndex(SourceCard->OwnerPlayerIndex) || ZoneId.IsNone()) return false;
	if (!IsFieldZoneForPlayer(ZoneId, SourceCard->OwnerPlayerIndex)) return false;
	FGuid ExistingStackId;
	if (FindStackIdInZone(ZoneId, ExistingStackId)) return false;
	const bool bPlayed = PlaceCardAsNewStack(SourceCardInstanceId, ZoneId);
	if (bPlayed)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Source played to empty zone Player=%d Zone=%s"), SourceCard->OwnerPlayerIndex, *ZoneId.ToString());
		ExecuteCardTrigger(SourceCardInstanceId, ETCGEffectTrigger::OnBecomingTopCard);
		QueueOnPlayChainAfterCurrentChain(this, SourceCardInstanceId);
	}
	return bPlayed;
}

bool ATCG_GameState::PlaySourceCardToFirstEmptyZone(const FGuid& SourceCardInstanceId)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || !IsValidPlayerIndex(SourceCard->OwnerPlayerIndex)) return false;
	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = GetFieldZoneId(SourceCard->OwnerPlayerIndex, FieldIndex);
		FGuid ExistingStackId;
		if (FindStackIdInZone(ZoneId, ExistingStackId)) continue;
		return PlaySourceCardToEmptyZone(SourceCardInstanceId, ZoneId);
	}
	return false;
}

bool ATCG_GameState::SendTopDeckCardToGraveyard(int32 PlayerIndex)
{
	if (!IsValidPlayerIndex(PlayerIndex)) return false;
	FTCGCardInstance* TopDeckCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Deck) continue;
		if (!TopDeckCard || Card.LocationIndex > TopDeckCard->LocationIndex) TopDeckCard = &Card;
	}
	if (!TopDeckCard) return false;
	const FGuid SentCardId = TopDeckCard->CardInstanceId;
	const FName SentCardDefinitionId = TopDeckCard->CardDefinitionId;
	const bool bMoved = MoveCardToLocation(SentCardId, ETCGCardLocation::Graveyard);
	if (!bMoved) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Sent top deck card to graveyard Player=%d Card=%s"), PlayerIndex, *SentCardDefinitionId.ToString());

	const FTCGCardInstance* SentCard = FindCardInstance(SentCardId);
	if (SentCard)
	{
		TArray<FTCGEffectChainEntry> Chain;
		TArray<FTCGCardEffectRef> EffectRefs;
		GetPrintedEffectRefsForCard(*SentCard, EffectRefs);
		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnSentToGraveyard)) AddCardEffectRefToChain(Chain, SentCardId, SentCardId, EffectRef);
		}
		if (Chain.Num() <= 0 && IsTidesCallingCard(SentCard)) AddCardEffectRefToChain(Chain, SentCardId, SentCardId, MakeTidesCallingFromDeckToGraveyardEffect());
		if (Chain.Num() <= 0 && IsMirechantCard(SentCard)) AddCardEffectRefToChain(Chain, SentCardId, SentCardId, MakeMirechantFromDeckToGraveyardEffect());
		for (FTCGEffectChainEntry& Entry : Chain)
		{
			Entry.bRequiresSourceOnBoard = false;
			Entry.bRequiresTargetOnBoard = false;
		}
		ResolveEffectChain(Chain);
	}
	return true;
}

bool ATCG_GameState::MoveFirstGraveyardCardToBottomDeck(int32 PlayerIndex)
{
	if (!IsValidPlayerIndex(PlayerIndex)) return false;
	FTCGCardInstance* GraveyardCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Graveyard) continue;
		if (!GraveyardCard || Card.LocationIndex < GraveyardCard->LocationIndex) GraveyardCard = &Card;
	}
	if (!GraveyardCard) return false;
	const FName CardDefinitionId = GraveyardCard->CardDefinitionId;
	const bool bMoved = MoveCardToBottomOfDeck(GraveyardCard->CardInstanceId);
	if (bMoved) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Moved graveyard card to bottom deck Player=%d Card=%s"), PlayerIndex, *CardDefinitionId.ToString());
	return bMoved;
}

void ATCG_GameState::RunDebugTidesCallingScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Tide's Calling scenario start"));
	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Placement);
	SetCurrentTurnPlayer(0);
	AddCardInstance("Debug_Water_Filler_A", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Water_Filler_B", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck);
	AddCardInstance(DebugCard_TidesCalling, ETCGCardElement::Water, 2, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Water_GraveyardSeed", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard);
	const bool bSentTopDeck = SendTopDeckCardToGraveyard(0);
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Tide's Calling scenario final summary SentTopDeck=%s"), bSentTopDeck ? TEXT("true") : TEXT("false"));
	EndMatch(ETCGMatchResult::Draw);
}

void ATCG_GameState::RunDebugDreampoolMirechantScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Dreampool Mirechant scenario start"));
	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Placement);
	SetCurrentTurnPlayer(0);

	FTCGCardInstance& WaterUnit = AddCardInstance("Debug_Water_TargetUnit", ETCGCardElement::Water, 3, 0, ETCGCardLocation::Board);
	WaterUnit.ZoneId = GetFieldZoneId(0, 0);
	WaterUnit.StackId = FGuid::NewGuid();
	WaterUnit.StackIndex = 0;
	AddCardInstance("Debug_Water_Grave_A", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard);
	AddCardInstance("Debug_Water_Grave_B", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard);
	AddCardInstance("Debug_Fire_Grave_C", ETCGCardElement::Fire, 1, 0, ETCGCardLocation::Graveyard);
	AddCardInstance(DebugCard_DreampoolMirechant, ETCGCardElement::Water, 3, 0, ETCGCardLocation::Deck);

	const int32 BeforeAttack = GetFinalAttackPreviewWithContinuousEffects(this, WaterUnit.CardInstanceId);
	const bool bSentTopDeck = SendTopDeckCardToGraveyard(0);
	const int32 AfterAttack = GetFinalAttackPreviewWithContinuousEffects(this, WaterUnit.CardInstanceId);
	TArray<FTCGCardInstance> StackCards;
	GetCardsInStack(WaterUnit.StackId, StackCards);
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Dreampool Mirechant summary SentTopDeck=%s StackCount=%d WaterGY=%d BeforeATK=%d AfterATK=%d"),
		bSentTopDeck ? TEXT("true") : TEXT("false"),
		StackCards.Num(),
		CountCardsInLocationByElement(0, ETCGCardLocation::Graveyard, ETCGCardElement::Water),
		BeforeAttack,
		AfterAttack);
	EndMatch(ETCGMatchResult::Draw);
}