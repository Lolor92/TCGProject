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
