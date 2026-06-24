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
	static bool ReturnUnitStackToHandMaterialsToGraveyard(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId, int32& OutMaterialCount);
	static bool ResolveCardEffectUnitRemovalWithReplacement(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId);
	static bool ResolveAttackMillTwoWaterBounceBattlingUnit(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId, const FGuid& BattleTargetCardInstanceId);
	static int32 DiscardCardsFromHand(ATCG_GameState* GameState, int32 PlayerIndex, int32 Count);
	static bool MoveCardToTopOfDeck(ATCG_GameState* GameState, const FGuid& CardInstanceId);
	static bool MoveFirstHandCardToTopDeck(ATCG_GameState* GameState, int32 PlayerIndex);
	static bool AttachGraveyardCardToSourceMaterial(ATCG_GameState* GameState, int32 PlayerIndex, const FGuid& SourceCardInstanceId, const FTCGEffectTargetFilter& TargetFilter);
};