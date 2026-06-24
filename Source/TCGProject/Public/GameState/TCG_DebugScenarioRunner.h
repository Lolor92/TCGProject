#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TCG_DebugScenarioRunner.generated.h"

class ATCG_GameState;

UCLASS()
class TCGPROJECT_API UTCG_DebugScenarioRunner : public UObject
{
	GENERATED_BODY()

public:
	static void CreateDebugTestCards(ATCG_GameState* GameState);
	static void SetupDebugMatch(ATCG_GameState* GameState);
	static void RunDebugTurnFlow(ATCG_GameState* GameState);
};
