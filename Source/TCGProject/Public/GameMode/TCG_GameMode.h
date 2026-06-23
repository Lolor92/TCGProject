#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TCG_GameMode.generated.h"

class APlayerController;

/**
 * GameMode exists only on the server.
 * It decides rules: player joining, starting match, assigning player indexes.
 */
UCLASS()
class TCGPROJECT_API ATCG_GameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATCG_GameMode();

	// Called on the server when a player joins.
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	// Simple counter used to assign player indexes.
	int32 NextPlayerIndex = 0;
};
