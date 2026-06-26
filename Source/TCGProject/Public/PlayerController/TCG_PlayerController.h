#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/TCGUIDataTypes.h"
#include "TCG_PlayerController.generated.h"

class UTCGMatchHUDWidgetBase;

UCLASS()
class TCGPROJECT_API ATCG_PlayerController : public APlayerController
{
GENERATED_BODY()

public:
UFUNCTION(BlueprintCallable, Category="TCG|UI")
void CreateMatchHUD();

UFUNCTION(BlueprintCallable, Category="TCG|UI")
void SetMatchHUDData(const FTCGMatchHUDWidgetData& InHUDData);

UFUNCTION(BlueprintPure, Category="TCG|UI")
UTCGMatchHUDWidgetBase* GetMatchHUDWidget() const { return MatchHUDWidget; }

protected:
virtual void BeginPlay() override;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI")
TSubclassOf<UTCGMatchHUDWidgetBase> MatchHUDWidgetClass;

UPROPERTY(BlueprintReadOnly, Category="TCG|UI")
TObjectPtr<UTCGMatchHUDWidgetBase> MatchHUDWidget = nullptr;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Debug")
bool bUseDebugHUDData = true;

void PushDebugHUDData();
};
