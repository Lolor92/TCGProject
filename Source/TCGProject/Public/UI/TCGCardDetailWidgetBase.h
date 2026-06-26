#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGCardDetailWidgetBase.generated.h"

class UPanelWidget;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class TCGPROJECT_API UTCGCardDetailWidgetBase : public UUserWidget
{
GENERATED_BODY()

public:
UFUNCTION(BlueprintCallable, Category="TCG|UI|Card Detail")
void SetCardData(const FTCGCardWidgetData& InCardData);

UFUNCTION(BlueprintCallable, Category="TCG|UI|Card Detail")
void ClearCardData();

UFUNCTION(BlueprintPure, Category="TCG|UI|Card Detail")
bool HasCardData() const { return bHasCardData; }

UFUNCTION(BlueprintPure, Category="TCG|UI|Card Detail")
const FTCGCardWidgetData& GetCardData() const { return CardData; }

protected:
virtual void NativeConstruct() override;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card Detail")
FTCGCardWidgetData CardData;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card Detail")
bool bHasCardData = false;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UTextBlock> CardName_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UTextBlock> Attack_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UTextBlock> Element_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UTextBlock> Zone_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UTextBlock> MaterialCount_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card Detail")
TObjectPtr<UPanelWidget> EffectText_VerticalBox = nullptr;

void RefreshCardDataText();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card Detail")
void BP_OnCardDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card Detail")
void BP_OnCardDataCleared();
};
