#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Cards/TCG_CardTypes.h"
#include "TCG_CardQueryService.generated.h"

class ATCG_GameState;

UCLASS()
class TCGPROJECT_API UTCG_CardQueryService : public UObject
{
	GENERATED_BODY()

public:
	static FTCGCardInstance* FindCardInstance(ATCG_GameState* GameState, const FGuid& CardInstanceId);
	static const FTCGCardInstance* FindCardInstance(const ATCG_GameState* GameState, const FGuid& CardInstanceId);

	static FTCGCardInstance* FindTopCardInStack(ATCG_GameState* GameState, const FGuid& StackId);
	static const FTCGCardInstance* FindTopCardInStack(const ATCG_GameState* GameState, const FGuid& StackId);

	static bool DoesPlayerHaveAnyCardOnBoard(const ATCG_GameState* GameState, int32 PlayerIndex);
	static int32 GetCardsUnderneathCount(const ATCG_GameState* GameState, const FGuid& CardInstanceId);
	static int32 GetFinalAttack(const ATCG_GameState* GameState, const FGuid& CardInstanceId);

	static bool FindStackIdInZone(const ATCG_GameState* GameState, FName ZoneId, FGuid& OutStackId);
	static void GetCardsInStack(const ATCG_GameState* GameState, const FGuid& StackId, TArray<FTCGCardInstance>& OutCards);
	static void GetCardsInZone(const ATCG_GameState* GameState, FName ZoneId, TArray<FTCGCardInstance>& OutCards);
	static const FTCGCardInstance* FindTopCardInZone(const ATCG_GameState* GameState, FName ZoneId);

	static void GetCardsInLocation(const ATCG_GameState* GameState, int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards);
	static void GetCardsInHand(const ATCG_GameState* GameState, int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards);
	static void GetCardsInDeck(const ATCG_GameState* GameState, int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards);
	static int32 GetNextLocationIndex(const ATCG_GameState* GameState, int32 PlayerIndex, ETCGCardLocation Location);
};
