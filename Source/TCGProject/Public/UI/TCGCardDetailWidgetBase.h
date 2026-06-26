#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGCardDetailWidgetBase.generated.h"

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
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card Detail")
FTCGCardWidgetData CardData;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card Detail")
bool bHasCardData = false;

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card Detail")
void BP_OnCardDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card Detail")
void BP_OnCardDataCleared();
};
