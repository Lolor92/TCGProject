#include "UI/TCGHandWidgetBase.h"

void UTCGHandWidgetBase::SetHandData(const FTCGHandWidgetData& InHandData)
{
HandData = InHandData;
BP_OnHandDataChanged();
BP_OnHandSelectionChanged(HandData.SelectedHandIndex);
}

void UTCGHandWidgetBase::SelectHandCard(const int32 HandIndex)
{
FTCGCardWidgetData SelectedCardData;
if (!GetCardDataByHandIndex(HandIndex, SelectedCardData))
{
return;
}

HandData.SelectedHandIndex = HandIndex;
OnHandCardSelected.Broadcast(HandIndex, SelectedCardData.SourceObject);
BP_OnHandSelectionChanged(HandData.SelectedHandIndex);
}

void UTCGHandWidgetBase::ClearHandSelection()
{
if (HandData.SelectedHandIndex == INDEX_NONE)
{
return;
}

HandData.SelectedHandIndex = INDEX_NONE;
BP_OnHandSelectionChanged(HandData.SelectedHandIndex);
}

bool UTCGHandWidgetBase::GetCardDataByHandIndex(const int32 HandIndex, FTCGCardWidgetData& OutCardData) const
{
for (const FTCGCardWidgetData& CardData : HandData.Cards)
{
if (CardData.HandIndex == HandIndex)
{
OutCardData = CardData;
return true;
}
}

return false;
}
