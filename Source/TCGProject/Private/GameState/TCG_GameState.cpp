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

bool ATCG_GameState::PlayCardToZone(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Card->Location != ETCGCardLocation::Hand) return false;

	FGuid ExistingStackId;
	if (!FindStackIdInZone(ZoneId, ExistingStackId))
	{
		return PlaceCardAsNewStack(CardInstanceId, ZoneId);
	}

	return PlaceCardOnStack(CardInstanceId, ExistingStackId);
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

bool ATCG_GameState::FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const
{
	OutStackId.Invalidate();

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId) continue;
		if (!Card.StackId.IsValid()) continue;

		OutStackId = Card.StackId;
		return true;
	}

	return false;
}

void ATCG_GameState::GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == StackId)
		{
			OutCards.Add(Card);
		}
	}

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

void ATCG_GameState::GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.ZoneId == ZoneId)
		{
			OutCards.Add(Card);
		}
	}

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInZone(FName ZoneId) const
{
	const FTCGCardInstance* TopCard = nullptr;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}

	return TopCard;
}

void ATCG_GameState::GetCardsInLocation(int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location)
		{
			OutCards.Add(Card);
		}
	}
}

void ATCG_GameState::GetCardsInHand(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const
{
	GetCardsInLocation(PlayerIndex, ETCGCardLocation::Hand, OutCards);
}

void ATCG_GameState::CreateDebugTestCards()
{
	MatchCards.Empty();

	FTCGCardInstance& Player0FireA = AddCardInstance("Debug_Fire_A", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player0FireB = AddCardInstance("Debug_Fire_B", ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player0Water = AddCardInstance("Debug_Water_A", ETCGCardElement::Water, 4, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player1Dark = AddCardInstance("Debug_Dark_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Hand);

	const FGuid FireAId = Player0FireA.CardInstanceId;
	const FGuid FireBId = Player0FireB.CardInstanceId;
	const FGuid WaterId = Player0Water.CardInstanceId;
	const FGuid DarkId = Player1Dark.CardInstanceId;

	const bool bFireNewStack = PlayCardToZone(FireAId, "Player0_Field_0");
	const bool bFireOnFire = PlayCardToZone(FireBId, "Player0_Field_0");
	const bool bWaterOnFire = PlayCardToZone(WaterId, "Player0_Field_0");
	const bool bDarkNewStack = PlayCardToZone(DarkId, "Player1_Field_0");
	
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Fire new stack success: %s"), bFireNewStack ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Dark new stack success: %s"), bDarkNewStack ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Fire on Fire success: %s"), bFireOnFire ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Water on Fire success: %s"), bWaterOnFire ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 has board card: %s"), DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 1 has board card: %s"), DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: FireB final attack: %d"), GetFinalAttack(FireBId));
	
	FGuid StackInZone;
	const bool bFoundStack = FindStackIdInZone("Player0_Field_0", StackInZone);

	TArray<FTCGCardInstance> CardsInZone;
	GetCardsInZone("Player0_Field_0", CardsInZone);

	const FTCGCardInstance* TopCard = FindTopCardInZone("Player0_Field_0");

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Found stack in Player0_Field_0: %s"), bFoundStack ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Cards in Player0_Field_0: %d"), CardsInZone.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Top card in Player0_Field_0: %s"), TopCard ? *TopCard->CardDefinitionId.ToString() : TEXT("None"));
	
	TArray<FTCGCardInstance> Player0Hand;
	GetCardsInHand(0, Player0Hand);

	TArray<FTCGCardInstance> Player1Hand;
	GetCardsInHand(1, Player1Hand);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 hand cards: %d"), Player0Hand.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 1 hand cards: %d"), Player1Hand.Num());
}
