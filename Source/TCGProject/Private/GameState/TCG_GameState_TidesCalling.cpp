#include "GameState/TCG_GameState.h"
#include "TimerManager.h"

namespace
{
	const FName DebugCard_TidesCalling = "Debug_Water_TidesCalling";

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

	bool IsTidesCallingCard(const FTCGCardInstance* Card)
	{
		return Card && Card->CardDefinitionId == DebugCard_TidesCalling;
	}

	void StartTidesCallingOnPlayChain(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId)
	{
		if (!GameState) return;

		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
		if (!IsTidesCallingCard(SourceCard) || SourceCard->Location != ETCGCardLocation::Board)
		{
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Starting queued OnPlay chain Source=%s"), *SourceCard->CardDefinitionId.ToString());

		TArray<FTCGEffectChainEntry> Chain;
		GameState->AddCardEffectRefToChain(Chain, SourceCardInstanceId, SourceCardInstanceId, MakeTidesCallingOnPlayEffect());
		GameState->ResolveEffectChain(Chain);
	}

	void LogTidesCallingFinalSummary(ATCG_GameState* GameState, bool bSentTopDeck)
	{
		if (!GameState) return;

		const bool bTidesOnBoard = GameState->DoesPlayerHaveAnyCardOnBoard(0);
		TArray<FTCGCardInstance> Player0Deck;
		TArray<FTCGCardInstance> Player0Graveyard;
		GameState->GetCardsInDeck(0, Player0Deck);
		GameState->GetCardsInLocation(0, ETCGCardLocation::Graveyard, Player0Graveyard);

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Tide's Calling scenario final summary SentTopDeck=%s TidesOnBoard=%s Deck=%d Graveyard=%d"),
			bSentTopDeck ? TEXT("true") : TEXT("false"),
			bTidesOnBoard ? TEXT("true") : TEXT("false"),
			Player0Deck.Num(),
			Player0Graveyard.Num());
	}

	void ScheduleTidesCallingFinalSummary(ATCG_GameState* GameState, bool bSentTopDeck)
	{
		if (!GameState) return;

		// Temporary debug-only delay: Tide's OnPlay chain is currently started on the next tick.
		// Long term, final debug summaries should be printed only after the real queued-trigger drain finishes.
		GameState->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(GameState, [GameState, bSentTopDeck]()
		{
			GameState->GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(GameState, [GameState, bSentTopDeck]()
			{
				LogTidesCallingFinalSummary(GameState, bSentTopDeck);
				GameState->EndMatch(ETCGMatchResult::Draw);
			}));
		}));
	}
}

bool ATCG_GameState::MoveCardToBottomOfDeck(const FGuid& CardInstanceId)
{
	FTCGCardInstance* CardToMove = FindCardInstance(CardInstanceId);
	if (!CardToMove || !IsValidPlayerIndex(CardToMove->OwnerPlayerIndex))
	{
		return false;
	}

	const int32 PlayerIndex = CardToMove->OwnerPlayerIndex;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Deck)
		{
			Card.LocationIndex += 1;
		}
	}

	CardToMove->Location = ETCGCardLocation::Deck;
	CardToMove->LocationIndex = 0;
	CardToMove->ZoneId = NAME_None;
	CardToMove->StackId.Invalidate();
	CardToMove->StackIndex = INDEX_NONE;
	return true;
}

bool ATCG_GameState::PlaySourceCardToFirstEmptyZone(const FGuid& SourceCardInstanceId)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || !IsValidPlayerIndex(SourceCard->OwnerPlayerIndex))
	{
		return false;
	}

	const int32 PlayerIndex = SourceCard->OwnerPlayerIndex;
	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);
		FGuid ExistingStackId;
		if (FindStackIdInZone(ZoneId, ExistingStackId)) continue;

		const bool bPlayed = PlaceCardAsNewStack(SourceCardInstanceId, ZoneId);
		if (bPlayed)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Tide's Calling played from graveyard Player=%d Field=%d"), PlayerIndex, FieldIndex);

			if (IsTidesCallingCard(SourceCard))
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Queued OnPlay chain after current chain Source=%s"), *SourceCard->CardDefinitionId.ToString());
				GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, SourceCardInstanceId]()
				{
					StartTidesCallingOnPlayChain(this, SourceCardInstanceId);
				}));
			}
		}
		return bPlayed;
	}

	return false;
}

bool ATCG_GameState::SendTopDeckCardToGraveyard(int32 PlayerIndex)
{
	if (!IsValidPlayerIndex(PlayerIndex))
	{
		return false;
	}

	FTCGCardInstance* TopDeckCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Deck) continue;
		if (!TopDeckCard || Card.LocationIndex > TopDeckCard->LocationIndex)
		{
			TopDeckCard = &Card;
		}
	}

	if (!TopDeckCard)
	{
		return false;
	}

	const FGuid SentCardId = TopDeckCard->CardInstanceId;
	const FName SentCardDefinitionId = TopDeckCard->CardDefinitionId;
	const bool bMoved = MoveCardToLocation(SentCardId, ETCGCardLocation::Graveyard);
	if (!bMoved)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Sent top deck card to graveyard Player=%d Card=%s"), PlayerIndex, *SentCardDefinitionId.ToString());

	const FTCGCardInstance* SentCard = FindCardInstance(SentCardId);
	if (IsTidesCallingCard(SentCard))
	{
		TArray<FTCGEffectChainEntry> Chain;
		if (AddCardEffectRefToChain(Chain, SentCardId, SentCardId, MakeTidesCallingFromDeckToGraveyardEffect()))
		{
			for (FTCGEffectChainEntry& Entry : Chain)
			{
				Entry.bRequiresSourceOnBoard = false;
				Entry.bRequiresTargetOnBoard = false;
			}
			ResolveEffectChain(Chain);
		}
	}

	return true;
}

bool ATCG_GameState::MoveFirstGraveyardCardToBottomDeck(int32 PlayerIndex)
{
	if (!IsValidPlayerIndex(PlayerIndex))
	{
		return false;
	}

	FTCGCardInstance* GraveyardCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Graveyard) continue;
		if (!GraveyardCard || Card.LocationIndex < GraveyardCard->LocationIndex)
		{
			GraveyardCard = &Card;
		}
	}

	if (!GraveyardCard)
	{
		return false;
	}

	const FName CardDefinitionId = GraveyardCard->CardDefinitionId;
	const bool bMoved = MoveCardToBottomOfDeck(GraveyardCard->CardInstanceId);
	if (bMoved)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Moved graveyard card to bottom deck Player=%d Card=%s"), PlayerIndex, *CardDefinitionId.ToString());
	}
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
	ScheduleTidesCallingFinalSummary(this, bSentTopDeck);
}