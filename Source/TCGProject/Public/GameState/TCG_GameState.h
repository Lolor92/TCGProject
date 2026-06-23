#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TCG_GameState.generated.h"


UENUM(BlueprintType)
enum class ETCGMatchPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
	Draw              UMETA(DisplayName = "Draw"),
	Main              UMETA(DisplayName = "Main"),
	Battle            UMETA(DisplayName = "Battle"),
	End               UMETA(DisplayName = "End")
};

/**
 * GameState exists on server and clients.
 * This stores shared match state: turn, phase, active player.
 */
UCLASS()
class TCGPROJECT_API ATCG_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ATCG_GameState();

	// Replication setup.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Starts at 1 when the match begins.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 TurnNumber = 0;

	// Which player is currently taking actions.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 CurrentTurnPlayerIndex = INDEX_NONE;

	// Current phase of the match.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	ETCGMatchPhase CurrentPhase = ETCGMatchPhase::WaitingForPlayers;

public:
	// Server should call this when both players are ready.
	void StartMatch();

	// Server should call this when moving to another phase.
	void SetPhase(ETCGMatchPhase NewPhase);

	// Server should call this when changing active player.
	void SetCurrentTurnPlayer(int32 NewPlayerIndex);
};
