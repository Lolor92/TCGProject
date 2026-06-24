#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TCG_BattleResolver.generated.h"

class ATCG_GameState;

UCLASS()
class TCGPROJECT_API UTCG_BattleResolver : public UObject
{
	GENERATED_BODY()

public:
	static bool ResolveBattleBetweenZones(ATCG_GameState* GameState, FName Player0ZoneId, FName Player1ZoneId);
	static bool ResolveBattlePhase(ATCG_GameState* GameState);
};
