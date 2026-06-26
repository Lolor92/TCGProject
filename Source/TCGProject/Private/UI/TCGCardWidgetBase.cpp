#include "UI/TCGCardWidgetBase.h"

void UTCGCardWidgetBase::SetCardData(const FTCGCardWidgetData& InCardData)
{
CardData = InCardData;
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
