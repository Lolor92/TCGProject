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

FTCGCardInstance& ATCG_GameState::AddCardInstance(FName CardDefinitionId, ETCGCardElement Element, int32 BaseAttack,
	int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation)
{
	FTCGCardInstance NewCard;
	NewCard.CardDefinitionId = CardDefinitionId;
	NewCard.Element = Element;
	NewCard.BaseAttack = BaseAttack;
	NewCard.OwnerPlayerIndex = OwnerPlayerIndex;
	NewCard.Location = StartingLocation;
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

FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId)
{
	if (!StackId.IsValid()) return nullptr;

	FTCGCardInstance* TopCard = nullptr;

	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.StackId != StackId || Card.Location != ETCGCardLocation::Board) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}

	return TopCard;
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId) const
{
	if (!StackId.IsValid()) return nullptr;

	const FTCGCardInstance* TopCard = nullptr;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.StackId != StackId || Card.Location != ETCGCardLocation::Board) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}

	return TopCard;
}

bool ATCG_GameState::CanPlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId) const
{
	const FTCGCardInstance* CardToPlace = FindCardInstance(CardInstanceId);
	const FTCGCardInstance* TopCard = FindTopCardInStack(TargetStackId);

	if (!CardToPlace || !TopCard) return false;

	// Core rule for now: same element can stack on same element.
	// Exceptions will be added later through effects/rules.
	return CardToPlace->Element == TopCard->Element;
}

bool ATCG_GameState::PlaceCardAsNewStack(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = ETCGCardLocation::Board;
	Card->ZoneId = ZoneId;
	Card->StackId = FGuid::NewGuid();
	Card->StackIndex = 0;

	return true;
}

bool ATCG_GameState::PlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId)
{
	if (!CanPlaceCardOnStack(CardInstanceId, TargetStackId)) return false;

	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	FTCGCardInstance* TopCard = FindTopCardInStack(TargetStackId);

	if (!Card || !TopCard) return false;

	Card->Location = ETCGCardLocation::Board;
	Card->ZoneId = TopCard->ZoneId;
	Card->StackId = TargetStackId;
	Card->StackIndex = TopCard->StackIndex + 1;

	return true;
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

int32 ATCG_GameState::GetFinalAttack(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return 0;

	return Card->BaseAttack + GetCardsUnderneathCount(CardInstanceId);
}
