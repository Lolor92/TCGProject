#include "GameState/TCG_GameState.h"
#include "GameState/TCG_CardMovementService.h"

bool ATCG_GameState::MoveBottomOverlayToGraveyard(const FGuid& TargetCardInstanceId)
{
	return UTCG_CardMovementService::MoveBottomOverlayToGraveyard(this, TargetCardInstanceId);
}

bool ATCG_GameState::PlayGraveyardCardToEmptyZone(const FGuid& CardInstanceId, FName ZoneId)
{
	return UTCG_CardMovementService::PlayGraveyardCardToEmptyZone(this, CardInstanceId, ZoneId);
}

bool ATCG_GameState::ReturnUnitStackToBottomDeckMaterialsToGraveyard(const FGuid& TargetTopCardInstanceId)
{
	return UTCG_CardMovementService::ReturnUnitStackToBottomDeckMaterialsToGraveyard(this, TargetTopCardInstanceId);
}

bool ATCG_GameState::ResolveAttackMillTwoWaterBounceBattlingUnit(const FGuid& SourceCardInstanceId, const FGuid& BattleTargetCardInstanceId)
{
	return UTCG_CardMovementService::ResolveAttackMillTwoWaterBounceBattlingUnit(this, SourceCardInstanceId, BattleTargetCardInstanceId);
}
