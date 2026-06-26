#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGHandWidgetBase.generated.h"

class UPanelWidget;
class UTCGCardWidgetBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHandCardSelectedSignature, int32, HandIndex, UObject*, SourceObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHandCardPressedSignature, int32, HandIndex, UObject*, SourceObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGHandCardReleasedSignature, int32, HandIndex, UObject*, SourceObject);

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
void SetCardWidgetClass(TSubclassOf<UTCGCardWidgetBase> InCardWidgetClass);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void RebuildHandCards();

UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void SelectHandCard(int32 HandIndex);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Hand")
void ClearHandSelection();

UFUNCTION(BlueprintPure, Category="TCG|UI|Hand")
bool GetCardDataByHandIndex(int32 HandIndex, FTCGCardWidgetData& OutCardData) const;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Hand")
FTCGHandCardSelectedSignature OnHandCardSelected;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Hand")
FTCGHandCardPressedSignature OnHandCardPressed;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Hand")
FTCGHandCardReleasedSignature OnHandCardReleased;

protected:
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Hand")
FTCGHandWidgetData HandData;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Hand")
TSubclassOf<UTCGCardWidgetBase> CardWidgetClass;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Hand")
bool bAutoRebuildCardWidgets = true;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Hand")
TObjectPtr<UPanelWidget> CardContainer = nullptr;

UPROPERTY(BlueprintReadOnly, Category="TCG|UI|Hand")
TArray<TObjectPtr<UTCGCardWidgetBase>> SpawnedCardWidgets;

UFUNCTION()
void HandleCardWidgetClicked(int32 HandIndex, UObject* SourceObject);

UFUNCTION()
void HandleCardWidgetPressed(int32 HandIndex, UObject* SourceObject);

UFUNCTION()
void HandleCardWidgetReleased(int32 HandIndex, UObject* SourceObject);

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Hand")
void BP_OnHandDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Hand")
void BP_OnHandSelectionChanged(int32 SelectedHandIndex);
};
