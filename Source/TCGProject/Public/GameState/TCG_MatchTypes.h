#pragma once

#include "CoreMinimal.h"
#include "TCG_MatchTypes.generated.h"

UENUM(BlueprintType)
enum class ETCGMatchPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
	RoundStart UMETA(DisplayName = "Round Start"),
	Placement UMETA(DisplayName = "Placement"),
	Battle UMETA(DisplayName = "Battle"),
	RoundEnd UMETA(DisplayName = "Round End"),
	GameOver UMETA(DisplayName = "Game Over")
};

UENUM(BlueprintType)
enum class ETCGMatchResult : uint8
{
	None UMETA(DisplayName = "None"),
	Player0Wins UMETA(DisplayName = "Player 0 Wins"),
	Player1Wins UMETA(DisplayName = "Player 1 Wins"),
	Draw UMETA(DisplayName = "Draw")
};

USTRUCT(BlueprintType)
struct FTCGTemporaryAttackBoost
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FGuid TargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	int32 RoundApplied = 0;
};
