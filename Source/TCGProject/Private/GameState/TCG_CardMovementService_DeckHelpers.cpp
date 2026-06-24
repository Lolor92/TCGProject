#include "GameState/TCG_CardMovementService.h"
#include "GameState/TCG_GameState.h"

int32 UTCG_CardMovementService::DiscardCardsFromHand(ATCG_GameState* GameState, int32 PlayerIndex, int32 Count)
{
	if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex) || Count <= 0) return 0;

	TArray<FTCGCardInstance> HandCards;
	GameState->GetCardsInHand(PlayerIndex, HandCards);

	int32 DiscardedCount = 0;
	for (int32 Index = 0; Index < HandCards.Num() && DiscardedCount < Count; ++Index)
	{
		if (GameState->MoveCardToLocation(HandCards[Index].CardInstanceId, ETCGCardLocation::Graveyard))
		{
			DiscardedCount++;
		}
	}

	return DiscardedCount;
}

bool UTCG_CardMovementService::MoveCardToTopOfDeck(ATCG_GameState* GameState, const FGuid& CardInstanceId)
{
	if (!GameState) return false;

	FTCGCardInstance* Card = GameState->FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = ETCGCardLocation::Deck;
	Card->LocationIndex = GameState->GetNextLocationIndex(Card->OwnerPlayerIndex, ETCGCardLocation::Deck);
	Card->ZoneId = NAME_None;
	Card->StackId.Invalidate();
	Card->StackIndex = INDEX_NONE;
	return true;
}

bool UTCG_CardMovementService::MoveFirstHandCardToTopDeck(ATCG_GameState* GameState, int32 PlayerIndex)
{
	if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex)) return false;

	TArray<FTCGCardInstance> HandCards;
	GameState->GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() <= 0) return false;

	return MoveCardToTopOfDeck(GameState, HandCards[0].CardInstanceId);
}

bool UTCG_CardMovementService::AttachGraveyardCardToSourceMaterial(ATCG_GameState* GameState, int32 PlayerIndex, const FGuid& SourceCardInstanceId, const FTCGEffectTargetFilter& TargetFilter)
{
	if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex)) return false;

	FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid()) return false;

	const int32 OwnerPlayerIndex = TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent ? 1 - PlayerIndex : PlayerIndex;

	FTCGCardInstance* ChosenGraveyardCard = nullptr;
	for (FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
		if (Card.Location != ETCGCardLocation::Graveyard) continue;
		if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
		if (TargetFilter.bExcludeSourceCard && Card.CardInstanceId == SourceCardInstanceId) continue;

		if (!ChosenGraveyardCard || Card.LocationIndex < ChosenGraveyardCard->LocationIndex)
		{
			ChosenGraveyardCard = &Card;
		}
	}

	if (!ChosenGraveyardCard) return false;

	ChosenGraveyardCard->Location = ETCGCardLocation::Board;
	ChosenGraveyardCard->ZoneId = SourceCard->ZoneId;
	ChosenGraveyardCard->StackId = SourceCard->StackId;
	ChosenGraveyardCard->StackIndex = SourceCard->StackIndex;

	SourceCard->StackIndex++;

	return true;
}
