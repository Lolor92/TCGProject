#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TCGUIDataTypes.h"
#include "TCGCardWidgetBase.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGCardWidgetClickedSignature, int32, HandIndex, UObject*, SourceObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTCGCardWidgetPressedSignature, int32, HandIndex, UObject*, SourceObject);

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

UFUNCTION(BlueprintCallable, Category="TCG|UI|Card")
void NotifyCardPressed();

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Card")
FTCGCardWidgetClickedSignature OnCardClicked;

UPROPERTY(BlueprintAssignable, Category="TCG|UI|Card")
FTCGCardWidgetPressedSignature OnCardPressed;

protected:
virtual void NativeConstruct() override;
virtual void NativeDestruct() override;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card")
FTCGCardWidgetData CardData;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|UI|Card")
bool bSelected = false;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card")
TObjectPtr<UButton> Root_Button = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card")
TObjectPtr<UTextBlock> CardName_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card")
TObjectPtr<UTextBlock> Attack_Text = nullptr;

UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="TCG|UI|Card")
TObjectPtr<UTextBlock> Element_Text = nullptr;

UFUNCTION()
void HandleRootButtonClicked();

UFUNCTION()
void HandleRootButtonPressed();

void RefreshCardText();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card")
void BP_OnCardDataChanged();

UFUNCTION(BlueprintImplementableEvent, Category="TCG|UI|Card")
void BP_OnSelectedChanged(bool bNewSelected);
};
