#include "UI/TCGCardWidgetBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UTCGCardWidgetBase::NativeConstruct()
{
Super::NativeConstruct();

if (Root_Button)
{
Root_Button->SetIsEnabled(false);
}

RefreshCardText();
}

void UTCGCardWidgetBase::NativeDestruct()
{
if (Root_Button)
{
Root_Button->SetIsEnabled(true);
}

Super::NativeDestruct();
}

FReply UTCGCardWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
{
NotifyCardPressed();

return FReply::Handled()
.PreventThrottling();
}

return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UTCGCardWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
{
NotifyCardReleased();

return FReply::Handled()
.PreventThrottling();
}

return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UTCGCardWidgetBase::SetCardData(const FTCGCardWidgetData& InCardData)
{
CardData = InCardData;
RefreshCardText();
BP_OnCardDataChanged();
}

void UTCGCardWidgetBase::SetSelected(const bool bInSelected)
{
if (bSelected == bInSelected)
{
return;
}

bSelected = bInSelected;
BP_OnSelectedChanged(bSelected);
}

void UTCGCardWidgetBase::NotifyCardClicked()
{
OnCardClicked.Broadcast(CardData.HandIndex, CardData.SourceObject);
}

void UTCGCardWidgetBase::NotifyCardPressed()
{
OnCardPressed.Broadcast(CardData.HandIndex, CardData.SourceObject);
}

void UTCGCardWidgetBase::NotifyCardReleased()
{
OnCardReleased.Broadcast(CardData.HandIndex, CardData.SourceObject);
}

void UTCGCardWidgetBase::HandleRootButtonClicked()
{
NotifyCardClicked();
}

void UTCGCardWidgetBase::HandleRootButtonPressed()
{
NotifyCardPressed();
}

void UTCGCardWidgetBase::HandleRootButtonReleased()
{
NotifyCardReleased();
}

void UTCGCardWidgetBase::RefreshCardText()
{
if (CardName_Text)
{
CardName_Text->SetText(CardData.CardName);
}

if (Attack_Text)
{
Attack_Text->SetText(FText::Format(NSLOCTEXT("TCGCardWidget", "AttackFormat", "ATK: {0}"), FText::AsNumber(CardData.Attack)));
}

if (Element_Text)
{
Element_Text->SetText(FText::FromName(CardData.Element));
}
}
