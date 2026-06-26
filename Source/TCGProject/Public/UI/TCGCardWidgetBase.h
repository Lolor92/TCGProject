#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGCardWidgetBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGCardWidgetClickedSignature, int32, HandIndex, UObject*, SourceObject);

UCLASS(Abstract, Blueprintable)
class TCGPROJECT_API UTCGCardWidgetBase : public UUserWidget
{
GENERATED_BODY()

public:
UFUNCTION(BlueprintCallable, Category="TCG|UI|Card")
void SetCardData(const FTCGCardWidgetData& InCardData);

UFUNCTION(BlueprintPure, Category="TCG|UI|Card")
const FTCGCardWidgetData& GetCardData() const { return CardData; }

UFUNCTION(BlueprintCallable, Category="TCG|UI|Card")
void SetSelected(bool bInSelected);

UFUNCTION(BlueprintPure, Category="TCG|UI|Card")
bool IsSelected() const { return bSelected; }

UFUNCTION(BlueprintCallable, Category="TCG|UI|Card")
void NotifyCardClicked();

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Card")
FTCGCardWidgetClickedSignature OnCardClicked;

protected:
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card")
FTCGCardWidgetData CardData;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card")
bool bSelected = false;

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card")
void BP_OnCardDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card")
void BP_OnSelectedChanged(bool bNewSelected);
};
