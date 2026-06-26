#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGMatchHUDWidgetBase.generated.h"

class UTCGCardDetailWidgetBase;
class UTCGHandWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHUDHandCardSelectedSignature, int32, HandIndex, UObject*, SourceObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHUDHandCardPressedSignature, int32, HandIndex, UObject*, SourceObject);

UCLASS(Abstract, Blueprintable)
class TCGPROJECT_API UTCGMatchHUDWidgetBase : public UUserWidget
{
GENERATED_BODY()

public:
UFUNCTION(BlueprintCallable, Category="TCG|UI|Match HUD")
void SetHUDData(const FTCGMatchHUDWidgetData& InHUDData);

UFUNCTION(BlueprintPure, Category="TCG|UI|Match HUD")
const FTCGMatchHUDWidgetData& GetHUDData() const { return HUDData; }

UFUNCTION(BlueprintCallable, Category="TCG|UI|Match HUD")
void SelectHandCard(int32 HandIndex);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Match HUD")
void ClearSelection();

UFUNCTION(BlueprintCallable, Category="TCG|UI|Match HUD")
void BindHandWidget(UTCGHandWidgetBase* InHandWidget);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Match HUD")
void BindCardDetailWidget(UTCGCardDetailWidgetBase* InCardDetailWidget);

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Match HUD")
FTCGHUDHandCardSelectedSignature OnHUDHandCardSelected;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Match HUD")
FTCGHUDHandCardPressedSignature OnHUDHandCardPressed;

protected:
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Match HUD")
FTCGMatchHUDWidgetData HUDData;

UPROPERTY(BlueprintReadOnly, Category="TCG|UI|Match HUD")
TObjectPtr<UTCGHandWidgetBase> HandWidget = nullptr;

UPROPERTY(BlueprintReadOnly, Category="TCG|UI|Match HUD")
TObjectPtr<UTCGCardDetailWidgetBase> CardDetailWidget = nullptr;

UFUNCTION()
void HandleHandCardSelected(int32 HandIndex, UObject* SourceObject);

UFUNCTION()
void HandleHandCardPressed(int32 HandIndex, UObject* SourceObject);

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Match HUD")
void BP_OnHUDDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Match HUD")
void BP_OnSelectionChanged(int32 SelectedHandIndex, UObject* SourceObject);
};
