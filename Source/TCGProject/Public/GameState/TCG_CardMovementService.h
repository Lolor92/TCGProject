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
};
