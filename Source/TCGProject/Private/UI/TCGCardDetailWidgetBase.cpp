#include "UI/TCGCardDetailWidgetBase.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void UTCGCardDetailWidgetBase::NativeConstruct()
{
Super::NativeConstruct();
RefreshCardDataText();
}

void UTCGCardDetailWidgetBase::SetCardData(const FTCGCardWidgetData& InCardData)
{
CardData = InCardData;
bHasCardData = true;
RefreshCardDataText();
BP_OnCardDataChanged();
}

void UTCGCardDetailWidgetBase::ClearCardData()
{
CardData = FTCGCardWidgetData();
bHasCardData = false;
RefreshCardDataText();
BP_OnCardDataCleared();
}

void UTCGCardDetailWidgetBase::RefreshCardDataText()
{
if (CardName_Text)
{
CardName_Text->SetText(bHasCardData ? CardData.CardName : NSLOCTEXT("TCGCardDetail", "DefaultCardName", "Card Name"));
}

if (Attack_Text)
{
Attack_Text->SetText(FText::Format(NSLOCTEXT("TCGCardDetail", "AttackFormat", "ATK: {0}"), FText::AsNumber(CardData.Attack)));
}

if (Element_Text)
{
Element_Text->SetText(FText::Format(NSLOCTEXT("TCGCardDetail", "ElementFormat", "Element: {0}"), FText::FromName(CardData.Element)));
}

if (Zone_Text)
{
const FText ZoneText = CardData.ZoneLabel.IsEmpty()
? NSLOCTEXT("TCGCardDetail", "NoZone", "Zone: None")
: FText::Format(NSLOCTEXT("TCGCardDetail", "ZoneFormat", "Zone: {0}"), CardData.ZoneLabel);

Zone_Text->SetText(ZoneText);
}

if (MaterialCount_Text)
{
MaterialCount_Text->SetText(FText::Format(NSLOCTEXT("TCGCardDetail", "MaterialsFormat", "Materials: {0}"), FText::AsNumber(CardData.MaterialCount)));
}

if (EffectText_VerticalBox && !bHasCardData)
{
EffectText_VerticalBox->ClearChildren();
}
}
