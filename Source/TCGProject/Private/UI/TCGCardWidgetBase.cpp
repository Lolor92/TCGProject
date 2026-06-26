#include "UI/TCGCardWidgetBase.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UTCGCardWidgetBase::NativeConstruct()
{
Super::NativeConstruct();

if (Root_Button)
{
Root_Button->OnClicked.AddUniqueDynamic(this, &UTCGCardWidgetBase::HandleRootButtonClicked);
Root_Button->OnPressed.AddUniqueDynamic(this, &UTCGCardWidgetBase::HandleRootButtonPressed);
}

RefreshCardText();
}

void UTCGCardWidgetBase::NativeDestruct()
{
if (Root_Button)
{
Root_Button->OnClicked.RemoveDynamic(this, &UTCGCardWidgetBase::HandleRootButtonClicked);
Root_Button->OnPressed.RemoveDynamic(this, &UTCGCardWidgetBase::HandleRootButtonPressed);
}

Super::NativeDestruct();
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

void UTCGCardWidgetBase::HandleRootButtonClicked()
{
NotifyCardClicked();
}

void UTCGCardWidgetBase::HandleRootButtonPressed()
{
NotifyCardPressed();
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
