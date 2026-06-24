#include "GameState/TCG_GameState.h"

bool ATCG_GameState::MoveBottomOverlayToGraveyard(const FGuid& TargetCardInstanceId)
{
	FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
	if (!TargetCard || !TargetCard->StackId.IsValid())
	{
		return false;
	}

	FTCGCardInstance* BottomOverlayCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.StackId != TargetCard->StackId) continue;
		if (Card.CardInstanceId == TargetCard->CardInstanceId) continue;
		if (Card.StackIndex >= TargetCard->StackIndex) continue;
		if (!BottomOverlayCard || Card.StackIndex < BottomOverlayCard->StackIndex)
		{
			BottomOverlayCard = &Card;
		}
	}

	return BottomOverlayCard && MoveCardToLocation(BottomOverlayCard->CardInstanceId, ETCGCardLocation::Graveyard);
}

bool ATCG_GameState::PlayGraveyardCardToEmptyZone(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Card->Location != ETCGCardLocation::Graveyard) return false;
	if (ZoneId.IsNone()) return false;

	FGuid ExistingStackId;
	if (FindStackIdInZone(ZoneId, ExistingStackId)) return false;

	return PlaceCardAsNewStack(CardInstanceId, ZoneId);
}
