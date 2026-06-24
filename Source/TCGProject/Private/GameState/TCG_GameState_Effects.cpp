#include "GameState/TCG_GameState.h"

namespace
{
	constexpr bool bLogEffectResolution = true;

	static const TCHAR* GetTCGEffectStepDebugName(ETCGEffectStepType StepType)
	{
		switch (StepType)
		{
		case ETCGEffectStepType::Then: return TEXT("Then");
		case ETCGEffectStepType::DrawCards: return TEXT("DrawCards");
		case ETCGEffectStepType::DiscardCards: return TEXT("DiscardCards");
		case ETCGEffectStepType::ModifyAttack: return TEXT("ModifyAttack");
		case ETCGEffectStepType::SelectTarget: return TEXT("SelectTarget");
		default: return TEXT("None");
		}
	}

	static const TCHAR* GetTCGEffectTargetModeDebugName(ETCGEffectTargetMode TargetMode)
	{
		switch (TargetMode)
		{
		case ETCGEffectTargetMode::Controller: return TEXT("Controller");
		case ETCGEffectTargetMode::Opponent: return TEXT("Opponent");
		case ETCGEffectTargetMode::SourceCard: return TEXT("SourceCard");
		case ETCGEffectTargetMode::TriggerTarget: return TEXT("TriggerTarget");
		case ETCGEffectTargetMode::SelectedTarget: return TEXT("SelectedTarget");
		default: return TEXT("None");
		}
	}
}

bool ATCG_GameState::AddCardEffectRefToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId,
	const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef)
{
	const FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || EffectRef.Trigger == ETCGEffectTrigger::None)
	{
		return false;
	}

	if (EffectRef.EffectId.IsNone() && EffectRef.Steps.Num() <= 0)
	{
		return false;
	}

	FTCGEffectChainEntry NewEntry;
	NewEntry.ChainIndex = Chain.Num() + 1;
	NewEntry.SourceCardInstanceId = SourceCardInstanceId;
	NewEntry.TargetCardInstanceId = TargetCardInstanceId;
	NewEntry.SourceCardDefinitionId = SourceCard->CardDefinitionId;
	NewEntry.Trigger = EffectRef.Trigger;
	NewEntry.EffectId = EffectRef.EffectId;
	NewEntry.EffectRef = EffectRef;
	NewEntry.ControllerPlayerIndex = SourceCard->OwnerPlayerIndex;
	ApplyDebugEffectChainEntryRequirements(NewEntry);
	Chain.Add(NewEntry);

	if (bLogEffectResolution)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain add %d Source=%s EffectId=%s Steps=%d Optional=%s"),
			NewEntry.ChainIndex,
			*NewEntry.SourceCardDefinitionId.ToString(),
			NewEntry.EffectId.IsNone() ? TEXT("None") : *NewEntry.EffectId.ToString(),
			NewEntry.EffectRef.Steps.Num(),
			NewEntry.EffectRef.bOptional ? TEXT("true") : TEXT("false"));
	}

	return true;
}

bool ATCG_GameState::ResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	if (ChainEntry.EffectRef.Steps.Num() > 0)
	{
		return ResolveModularEffectChainEntry(ChainEntry);
	}

	return ResolveDebugEffectChainEntry(ChainEntry);
}

bool ATCG_GameState::ResolveModularEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	if (ChainEntry.EffectRef.Steps.Num() <= 0)
	{
		return false;
	}

	bool bResolvedAnyStep = false;
	bool bPreviousMeaningfulStepSucceeded = true;

	if (bLogEffectResolution)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Resolve modular chain %d Source=%s Steps=%d"),
			ChainEntry.ChainIndex,
			*ChainEntry.SourceCardDefinitionId.ToString(),
			ChainEntry.EffectRef.Steps.Num());
	}

	for (const FTCGEffectStep& Step : ChainEntry.EffectRef.Steps)
	{
		if (Step.StepType == ETCGEffectStepType::None)
		{
			continue;
		}

		if (Step.StepType == ETCGEffectStepType::Then)
		{
			if (bLogEffectResolution)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step Then PreviousSuccess=%s"),
					bPreviousMeaningfulStepSucceeded ? TEXT("true") : TEXT("false"));
			}
			continue;
		}

		if (Step.bRequiresPreviousStepSuccess && !bPreviousMeaningfulStepSucceeded)
		{
			if (bLogEffectResolution)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step skipped Type=%s Reason=PreviousStepFailed"),
					GetTCGEffectStepDebugName(Step.StepType));
			}
			bPreviousMeaningfulStepSucceeded = false;
			continue;
		}

		const bool bStepSucceeded = ResolveEffectStep(ChainEntry, Step, bPreviousMeaningfulStepSucceeded);
		bPreviousMeaningfulStepSucceeded = bStepSucceeded;
		bResolvedAnyStep |= bStepSucceeded;
	}

	return bResolvedAnyStep;
}

bool ATCG_GameState::ResolveEffectStep(const FTCGEffectChainEntry& ChainEntry, const FTCGEffectStep& Step, bool bPreviousStepSucceeded)
{
	bool bStepSucceeded = false;

	switch (Step.StepType)
	{
	case ETCGEffectStepType::DrawCards:
	{
		const int32 DrawCount = FMath::Max(0, Step.Value);
		const int32 DrawnCount = DrawCards(ChainEntry.ControllerPlayerIndex, DrawCount);
		bStepSucceeded = DrawnCount == DrawCount && DrawCount > 0;
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DrawCards Player=%d Requested=%d Drawn=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				DrawCount,
				DrawnCount,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::DiscardCards:
	{
		const int32 DiscardCount = FMath::Max(0, Step.Value);
		const int32 DiscardedCount = DiscardCardsFromHand(ChainEntry.ControllerPlayerIndex, DiscardCount);
		bStepSucceeded = DiscardedCount == DiscardCount && DiscardCount > 0;
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DiscardCards Player=%d Requested=%d Discarded=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				DiscardCount,
				DiscardedCount,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::ModifyAttack:
	{
		FGuid TargetCardInstanceId;
		if (Step.TargetMode == ETCGEffectTargetMode::SourceCard || Step.TargetMode == ETCGEffectTargetMode::None)
		{
			TargetCardInstanceId = ChainEntry.SourceCardInstanceId;
		}
		else if (Step.TargetMode == ETCGEffectTargetMode::TriggerTarget)
		{
			TargetCardInstanceId = ChainEntry.TargetCardInstanceId;
		}

		FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
		if (TargetCard && TargetCard->Location == ETCGCardLocation::Board)
		{
			TargetCard->AttackModifier += Step.Value;
			bStepSucceeded = true;
		}

		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step ModifyAttack TargetMode=%s Amount=%d Success=%s"),
				GetTCGEffectTargetModeDebugName(Step.TargetMode),
				Step.Value,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::SelectTarget:
	{
		// Target selection is intentionally scaffolded here. Player-choice UI and selected-target storage
		// will be added in the next pass. For now this step checks whether a matching target exists.
		const int32 OwnerPlayerIndex = Step.TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent
			? 1 - ChainEntry.ControllerPlayerIndex
			: ChainEntry.ControllerPlayerIndex;

		for (const FTCGCardInstance& Card : MatchCards)
		{
			if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
			if (Card.Location != Step.TargetFilter.RequiredLocation) continue;
			if (Step.TargetFilter.bRequireTopCard && Card.Location == ETCGCardLocation::Board)
			{
				const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
				if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
			}

			bStepSucceeded = true;
			break;
		}

		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step SelectTarget OwnerMode=%s Location=%d Success=%s"),
				GetTCGEffectTargetModeDebugName(Step.TargetFilter.OwnerMode),
				static_cast<int32>(Step.TargetFilter.RequiredLocation),
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	default:
		break;
	}

	if (bLogEffectResolution && Step.StepType != ETCGEffectStepType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step result Type=%s PreviousSuccess=%s Success=%s"),
			GetTCGEffectStepDebugName(Step.StepType),
			bPreviousStepSucceeded ? TEXT("true") : TEXT("false"),
			bStepSucceeded ? TEXT("true") : TEXT("false"));
	}

	return bStepSucceeded;
}

int32 ATCG_GameState::DiscardCardsFromHand(int32 PlayerIndex, int32 Count)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0)
	{
		return 0;
	}

	TArray<FTCGCardInstance*> HandCards;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Hand)
		{
			HandCards.Add(&Card);
		}
	}

	HandCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex < B.LocationIndex;
	});

	const int32 CardsToDiscard = FMath::Min(Count, HandCards.Num());
	for (int32 Index = 0; Index < CardsToDiscard; ++Index)
	{
		FTCGCardInstance* Card = HandCards[Index];
		if (!Card) continue;

		Card->Location = ETCGCardLocation::Graveyard;
		Card->LocationIndex = GetNextLocationIndex(PlayerIndex, ETCGCardLocation::Graveyard);
		Card->ZoneId = NAME_None;
		Card->StackId.Invalidate();
		Card->StackIndex = INDEX_NONE;
	}

	return CardsToDiscard;
}
