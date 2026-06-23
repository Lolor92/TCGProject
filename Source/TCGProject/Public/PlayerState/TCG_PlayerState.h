#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TCG_PlayerState.generated.h"

/**
 * PlayerState exists on the server and all clients.
 * We use it for player-specific match data: life, player index, resources later.
 */
UCLASS()
class TCGPROJECT_API ATCG_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATCG_PlayerState();

	// Replication setup.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 0 = player one, 1 = player two.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Player")
	int32 PlayerIndex = INDEX_NONE;

	// Current life/health of this player.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Player")
	int32 Life = 30;

	// Max life, useful for UI later.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Player")
	int32 MaxLife = 30;

public:
	// Server should call this when the player joins.
	void SetPlayerIndex(int32 NewPlayerIndex);

	// Server should call this when life changes.
	void SetLife(int32 NewLife);
};
