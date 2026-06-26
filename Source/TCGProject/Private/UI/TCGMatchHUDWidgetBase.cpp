#include "UI/TCGMatchHUDWidgetBase.h"

#include "UI/TCGCardDetailWidgetBase.h"
#include "UI/TCGHandWidgetBase.h"

void UTCGMatchHUDWidgetBase::SetHUDData(const FTCGMatchHUDWidgetData& InHUDData)
{
HUDData = InHUDData;

if (HandWidget)
{
HandWidget->SetHandData(HUDData.LocalHand);
}

BP_OnHUDDataChanged();
}

void UTCGMatchHUDWidgetBase::SelectHandCard(const int32 HandIndex)
{
FTCGCardWidgetData SelectedCardData;
if (!HUDData.LocalHand.Cards.IsValidIndex(HandIndex))
{
// HandIndex may be a logical replicated hand index rather than array position.
bool bFound = false;
for (const FTCGCardWidgetData& CardData : HUDData.LocalHand.Cards)
{
if (CardData.HandIndex == HandIndex)
{
SelectedCardData = CardData;
bFound = true;
break;
}
}

if (!bFound)
{
return;
}
}
else
{
SelectedCardData = HUDData.LocalHand.Cards[HandIndex];
}

HUDData.LocalHand.SelectedHandIndex = SelectedCardData.HandIndex;

if (HandWidget)
{
HandWidget->SelectHandCard(SelectedCardData.HandIndex);
}

if (CardDetailWidget)
{
CardDetailWidget->SetCardData(SelectedCardData);
}

OnHUDHandCardSelected.Broadcast(SelectedCardData.HandIndex, SelectedCardData.SourceObject);
BP_OnSelectionChanged(SelectedCardData.HandIndex, SelectedCardData.SourceObject);
}

void UTCGMatchHUDWidgetBase::ClearSelection()
{
HUDData.LocalHand.SelectedHandIndex = INDEX_NONE;

if (HandWidget)
{
HandWidget->ClearHandSelection();
}

if (CardDetailWidget)
{
CardDetailWidget->ClearCardData();
}

BP_OnSelectionChanged(INDEX_NONE, nullptr);
}

void UTCGMatchHUDWidgetBase::BindHandWidget(UTCGHandWidgetBase* InHandWidget)
{
if (HandWidget)
{
HandWidget->OnHandCardSelected.RemoveDynamic(this, &UTCGMatchHUDWidgetBase::HandleHandCardSelected);
}

HandWidget = InHandWidget;

if (HandWidget)
{
HandWidget->OnHandCardSelected.AddUniqueDynamic(this, &UTCGMatchHUDWidgetBase::HandleHandCardSelected);
HandWidget->SetHandData(HUDData.LocalHand);
}
}

void UTCGMatchHUDWidgetBase::BindCardDetailWidget(UTCGCardDetailWidgetBase* InCardDetailWidget)
{
CardDetailWidget = InCardDetailWidget;

FTCGCardWidgetData SelectedCardData;
bool bFoundSelectedCard = false;

for (const FTCGCardWidgetData& CardData : HUDData.LocalHand.Cards)
{
if (CardData.HandIndex == HUDData.LocalHand.SelectedHandIndex)
{
SelectedCardData = CardData;
bFoundSelectedCard = true;
break;
}
}

if (CardDetailWidget)
{
if (bFoundSelectedCard)
{
CardDetailWidget->SetCardData(SelectedCardData);
}
else
{
CardDetailWidget->ClearCardData();
}
}
}

void UTCGMatchHUDWidgetBase::HandleHandCardSelected(const int32 HandIndex, UObject* SourceObject)
{
FTCGCardWidgetData SelectedCardData;
if (!HUDData.LocalHand.Cards.IsValidIndex(HandIndex))
{
bool bFound = false;
for (const FTCGCardWidgetData& CardData : HUDData.LocalHand.Cards)
{
if (CardData.HandIndex == HandIndex)
{
SelectedCardData = CardData;
bFound = true;
break;
}
}

if (!bFound)
{
return;
}
}
else
{
SelectedCardData = HUDData.LocalHand.Cards[HandIndex];
}

HUDData.LocalHand.SelectedHandIndex = SelectedCardData.HandIndex;

if (CardDetailWidget)
{
CardDetailWidget->SetCardData(SelectedCardData);
}

OnHUDHandCardSelected.Broadcast(SelectedCardData.HandIndex, SelectedCardData.SourceObject);
BP_OnSelectionChanged(SelectedCardData.HandIndex, SelectedCardData.SourceObject);
}
