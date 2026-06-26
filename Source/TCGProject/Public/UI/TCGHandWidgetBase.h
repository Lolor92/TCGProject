#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGHandWidgetBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHandCardSelectedSignature, int32, HandIndex, UObject*, SourceObject);

UCLASS(Abstract, Blueprintable)
class TCGPROJECT_API UTCGHandWidgetBase : public UUserWidget
{
GENERATED_BODY()

public:
UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void SetHandData(const FTCGHandWidgetData& InHandData);

UFUNCTION(BlueprintPure, Category="TCG|UI|Hand")
const FTCGHandWidgetData& GetHandData() const { return HandData; }

UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void SelectHandCard(int32 HandIndex);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void ClearHandSelection();

UFUNCTION(BlueprintPure, Category="TCG|UI|Hand")
bool GetCardDataByHandIndex(int32 HandIndex, FTCGCardWidgetData& OutCardData) const;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Hand")
FTCGHandCardSelectedSignature OnHandCardSelected;

protected:
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Hand")
FTCGHandWidgetData HandData;

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Hand")
void BP_OnHandDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Hand")
void BP_OnHandSelectionChanged(int32 SelectedHandIndex);
};
