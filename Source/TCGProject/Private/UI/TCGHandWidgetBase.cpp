#include "UI/TCGHandWidgetBase.h"

#include "Components/PanelWidget.h"
#include "UI/TCGCardWidgetBase.h"

namespace
{
	bool DoesHandWidgetLayoutMatch(
		const TArray<TObjectPtr<UTCGCardWidgetBase>>& SpawnedCardWidgets,
		const FTCGHandWidgetData& NewHandData)
	{
		if (SpawnedCardWidgets.Num() != NewHandData.Cards.Num())
		{
			return false;
		}

		for (int32 CardIndex = 0; CardIndex < NewHandData.Cards.Num(); ++CardIndex)
		{
			const UTCGCardWidgetBase* ExistingCardWidget = SpawnedCardWidgets[CardIndex];
			if (!ExistingCardWidget)
			{
				return false;
			}

			const FTCGCardWidgetData& ExistingCardData = ExistingCardWidget->GetCardData();
			const FTCGCardWidgetData& NewCardData = NewHandData.Cards[CardIndex];

			if (ExistingCardData.HandIndex != NewCardData.HandIndex
				|| ExistingCardData.CardInstanceId != NewCardData.CardInstanceId)
			{
				return false;
			}
		}

		return true;
	}
}

void UTCGHandWidgetBase::SetHandData(const FTCGHandWidgetData& InHandData)
{
	const bool bCanReuseCardWidgets = bAutoRebuildCardWidgets
		&& DoesHandWidgetLayoutMatch(SpawnedCardWidgets, InHandData);

	HandData = InHandData;

	if (bCanReuseCardWidgets)
	{
		for (int32 CardIndex = 0; CardIndex < HandData.Cards.Num(); ++CardIndex)
		{
			UTCGCardWidgetBase* CardWidget = SpawnedCardWidgets[CardIndex];
			if (!CardWidget)
			{
				continue;
			}

			const FTCGCardWidgetData& CardData = HandData.Cards[CardIndex];
			CardWidget->SetCardData(CardData);
			CardWidget->SetSelected(CardData.HandIndex == HandData.SelectedHandIndex);
		}
	}
	else if (bAutoRebuildCardWidgets)
	{
		RebuildHandCards();
	}

	BP_OnHandDataChanged();
	BP_OnHandSelectionChanged(HandData.SelectedHandIndex);
}

void UTCGHandWidgetBase::SetCardWidgetClass(TSubclassOf<UTCGCardWidgetBase> InCardWidgetClass)
{
	CardWidgetClass = InCardWidgetClass;

	if (bAutoRebuildCardWidgets)
	{
		RebuildHandCards();
	}
}

void UTCGHandWidgetBase::RebuildHandCards()
{
	SpawnedCardWidgets.Reset();

	if (!CardContainer)
	{
		return;
	}

	CardContainer->ClearChildren();

	if (!CardWidgetClass)
	{
		return;
	}

	for (const FTCGCardWidgetData& CardData : HandData.Cards)
	{
		UTCGCardWidgetBase* CardWidget = CreateWidget<UTCGCardWidgetBase>(GetOwningPlayer(), CardWidgetClass);
		if (!CardWidget)
		{
			continue;
		}

		CardWidget->SetCardData(CardData);
		CardWidget->SetSelected(CardData.HandIndex == HandData.SelectedHandIndex);
		CardWidget->OnCardClicked.AddUniqueDynamic(this, &UTCGHandWidgetBase::HandleCardWidgetClicked);
		CardWidget->OnCardPressed.AddUniqueDynamic(this, &UTCGHandWidgetBase::HandleCardWidgetPressed);

		CardContainer->AddChild(CardWidget);
		SpawnedCardWidgets.Add(CardWidget);
	}
}

void UTCGHandWidgetBase::SelectHandCard(const int32 HandIndex)
{
	FTCGCardWidgetData SelectedCardData;
	if (!GetCardDataByHandIndex(HandIndex, SelectedCardData))
	{
		return;
	}

	HandData.SelectedHandIndex = HandIndex;

	for (UTCGCardWidgetBase* CardWidget : SpawnedCardWidgets)
	{
		if (CardWidget)
		{
			CardWidget->SetSelected(CardWidget->GetCardData().HandIndex == HandData.SelectedHandIndex);
		}
	}

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

	for (UTCGCardWidgetBase* CardWidget : SpawnedCardWidgets)
	{
		if (CardWidget)
		{
			CardWidget->SetSelected(false);
		}
	}

	BP_OnHandSelectionChanged(HandData.SelectedHandIndex);
}

void UTCGHandWidgetBase::HandleCardWidgetClicked(const int32 HandIndex, UObject* SourceObject)
{
	SelectHandCard(HandIndex);
}

void UTCGHandWidgetBase::HandleCardWidgetPressed(const int32 HandIndex, UObject* SourceObject)
{
	SelectHandCard(HandIndex);
	OnHandCardPressed.Broadcast(HandIndex, SourceObject);
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