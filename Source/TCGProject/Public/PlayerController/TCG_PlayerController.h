#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UI/TCGUIDataTypes.h"
#include "TCG_PlayerController.generated.h"

class ATCG_GameState;
class UTCGMatchHUDWidgetBase;
struct FTCGCardInstance;

enum class ETCGCardElement : uint8;

UCLASS()
class TCGPROJECT_API ATCG_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void CreateMatchHUD();

	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void SetMatchHUDData(const FTCGMatchHUDWidgetData& InHUDData);

	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void RefreshMatchHUDFromGameState();

	UFUNCTION(BlueprintPure, Category="TCG|UI")
	UTCGMatchHUDWidgetBase* GetMatchHUDWidget() const { return MatchHUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI")
	TSubclassOf<UTCGMatchHUDWidgetBase> MatchHUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="TCG|UI")
	TObjectPtr<UTCGMatchHUDWidgetBase> MatchHUDWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI")
	int32 LocalPlayerIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI", meta=(ClampMin="0.05", UIMin="0.05"))
	float HUDRefreshInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Debug")
	bool bUseDebugHUDData = true;

	void PushDebugHUDData();
	void StartMatchHUDRefreshTimer();
	void StopMatchHUDRefreshTimer();
	FTCGMatchHUDWidgetData BuildHUDDataFromGameState(const ATCG_GameState& TCGGameState, int32 ForPlayerIndex) const;
	FTCGCardWidgetData BuildCardWidgetDataFromCard(const ATCG_GameState& TCGGameState, const FTCGCardInstance& Card, int32 HandIndex) const;
	FName GetCardElementName(ETCGCardElement Element) const;

	FTimerHandle MatchHUDRefreshTimerHandle;
};