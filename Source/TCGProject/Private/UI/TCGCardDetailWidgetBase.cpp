#include "UI/TCGCardDetailWidgetBase.h"

void UTCGCardDetailWidgetBase::SetCardData(const FTCGCardWidgetData& InCardData)
{
CardData = InCardData;
bHasCardData = true;
BP_OnCardDataChanged();
}

void UTCGCardDetailWidgetBase::ClearCardData()
{
CardData = FTCGCardWidgetData();
bHasCardData = false;
BP_OnCardDataCleared();
}
