#include "GameState/TCG_GameState.h"
#include "GameState/TCG_CardMovementService.h"

int32 ATCG_GameState::DiscardCardsFromHand(int32 PlayerIndex, int32 Count)
{
	return UTCG_CardMovementService::DiscardCardsFromHand(this, PlayerIndex, Count);
}

bool ATCG_GameState::MoveCardToTopOfDeck(const FGuid& CardInstanceId)
{
	return UTCG_CardMovementService::MoveCardToTopOfDeck(this, CardInstanceId);
}

bool ATCG_GameState::MoveFirstHandCardToTopDeck(int32 PlayerIndex)
{
	return UTCG_CardMovementService::MoveFirstHandCardToTopDeck(this, PlayerIndex);
}

bool ATCG_GameState::AttachGraveyardCardToSourceMaterial(int32 PlayerIndex, const FGuid& SourceCardInstanceId, const FTCGEffectTargetFilter& TargetFilter)
{
	return UTCG_CardMovementService::AttachGraveyardCardToSourceMaterial(this, PlayerIndex, SourceCardInstanceId, TargetFilter);
}
