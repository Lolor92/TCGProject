#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TCG_PlayerState.generated.h"

/**
 * PlayerState exists on the server and all clients.
 * 
 * This should store player-owned match info.
 * Important: this game has no life points.
 * A player loses if they have no cards on their board after the battle phase.
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

	// True once this player has lost the match.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Player")
	bool bHasLost = false;

public:
	// Server should call this when the player joins.
	void SetPlayerIndex(int32 NewPlayerIndex);

	// Server will call this later when checking the lose condition.
	void SetHasLost(bool bNewHasLost);
};
