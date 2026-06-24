#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Cards/TCG_CardTypes.h"
#include "TCG_CardMovementService.generated.h"

class ATCG_GameState;

UCLASS()
class TCGPROJECT_API UTCG_CardMovementService : public UObject
{
	GENERATED_BODY()

public:
	static bool MoveCardToLocation(ATCG_GameState* GameState, const FGuid& CardInstanceId, ETCGCardLocation NewLocation);
	static bool MoveStackToLocation(ATCG_GameState* GameState, const FGuid& StackId, ETCGCardLocation NewLocation);
	static bool MoveBottomOverlayToGraveyard(ATCG_GameState* GameState, const FGuid& TargetCardInstanceId);
	static bool PlayGraveyardCardToEmptyZone(ATCG_GameState* GameState, const FGuid& CardInstanceId, FName ZoneId);
	static bool ReturnUnitStackToBottomDeckMaterialsToGraveyard(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId);
	static bool ResolveAttackMillTwoWaterBounceBattlingUnit(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId, const FGuid& BattleTargetCardInstanceId);
};
