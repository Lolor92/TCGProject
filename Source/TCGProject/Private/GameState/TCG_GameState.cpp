#include "GameState/TCG_GameState.h"
#include "Net/UnrealNetwork.h"

ATCG_GameState::ATCG_GameState()
{
	bReplicates = true;
}

void ATCG_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCG_GameState, TurnNumber);
	DOREPLIFETIME(ATCG_GameState, CurrentTurnPlayerIndex);
	DOREPLIFETIME(ATCG_GameState, CurrentPhase);
	DOREPLIFETIME(ATCG_GameState, MatchCards);
}

void ATCG_GameState::StartMatch()
{
	TurnNumber = 1;
	CurrentTurnPlayerIndex = 0;
	CurrentPhase = ETCGMatchPhase::Draw;
}

void ATCG_GameState::SetPhase(ETCGMatchPhase NewPhase)
{
	CurrentPhase = NewPhase;
}

void ATCG_GameState::SetCurrentTurnPlayer(int32 NewPlayerIndex)
{
	CurrentTurnPlayerIndex = NewPlayerIndex;
}

FTCGCardInstance& ATCG_GameState::AddCardInstance(FName CardDefinitionId, int32 OwnerPlayerIndex, 
	ETCGCardLocation StartingLocation)
{
	FTCGCardInstance NewCard;
	NewCard.CardDefinitionId = CardDefinitionId;
	NewCard.OwnerPlayerIndex = OwnerPlayerIndex;
	NewCard.Location = StartingLocation;

	// Not stacked yet.
	NewCard.StackId.Invalidate();
	NewCard.StackIndex = INDEX_NONE;
	NewCard.ZoneId = NAME_None;

	return MatchCards.Add_GetRef(NewCard);
}

FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId)
{
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId)
		{
			return &Card;
		}
	}

	return nullptr;
}

const FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId) const
{
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId)
		{
			return &Card;
		}
	}

	return nullptr;
}

bool ATCG_GameState::MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = NewLocation;

	// If the card leaves the board, clear its board/stack info for now.
	// Later, graveyard/banish effects can happen here or through a rules system.
	if (NewLocation != ETCGCardLocation::Board)
	{
		Card->ZoneId = NAME_None;
		Card->StackId.Invalidate();
		Card->StackIndex = INDEX_NONE;
	}

	return true;
}

bool ATCG_GameState::DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const
{
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex &&
			Card.Location == ETCGCardLocation::Board)
		{
			return true;
		}
	}

	return false;
}

int32 ATCG_GameState::GetCardsUnderneathCount(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* TargetCard = FindCardInstance(CardInstanceId);
	if (!TargetCard) return 0;

	// Cards not in a stack have no cards underneath.
	if (!TargetCard->StackId.IsValid() || TargetCard->StackIndex <= 0) return 0;

	int32 Count = 0;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		const bool bSameStack = Card.StackId == TargetCard->StackId;
		const bool bUnderneath = Card.StackIndex < TargetCard->StackIndex;

		if (bSameStack && bUnderneath)
		{
			Count++;
		}
	}

	return Count;
}
