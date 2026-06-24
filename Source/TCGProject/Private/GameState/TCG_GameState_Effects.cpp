#include "GameState/TCG_GameState.h"

namespace
{
	constexpr bool bLogEffectResolution = true;
	constexpr bool bAutoSubmitDebugDiscardChoice = true;
	constexpr bool bAutoSubmitDebugGraveyardToDeckChoice = true;
	constexpr bool bAutoSubmitDebugHandToTopDeckChoice = true;
	constexpr bool bAutoSubmitDebugPlayToEmptyZoneChoice = true;
	constexpr bool bAutoSubmitDebugAttachSourceToUnitChoice = true;

	const FName LegacyDebugEffect_Draw1 = "Debug_Draw1";
	const FName LegacyDebugEffect_GainAttackForCardsUnderneath = "Debug_GainAttackForCardsUnderneath";
	const FName LegacyDebugEffect_RemoveBottomOverlay = "Debug_RemoveBottomOverlay";

	static const TCHAR* GetTCGEffectStepDebugName(ETCGEffectStepType StepType)
	{
		switch (StepType)
		{
		case ETCGEffectStepType::Then: return TEXT("Then");
		case ETCGEffectStepType::DrawCards: return TEXT("DrawCards");
		case ETCGEffectStepType::DiscardCards: return TEXT("DiscardCards");
		case ETCGEffectStepType::ModifyAttack: return TEXT("ModifyAttack");
		case ETCGEffectStepType::SelectTarget: return TEXT("SelectTarget");
		case ETCGEffectStepType::MoveBottomOverlayToGraveyard: return TEXT("MoveBottomOverlayToGraveyard");
		case ETCGEffectStepType::PlaySourceToEmptyZone: return TEXT("PlaySourceToEmptyZone");
		case ETCGEffectStepType::SendTopDeckCardToGraveyard: return TEXT("SendTopDeckCardToGraveyard");
		case ETCGEffectStepType::MoveGraveyardCardToBottomDeck: return TEXT("MoveGraveyardCardToBottomDeck");
		case ETCGEffectStepType::MoveHandCardToTopDeck: return TEXT("MoveHandCardToTopDeck");
		case ETCGEffectStepType::AttachSourceToWaterUnitMaterial: return TEXT("AttachSourceToWaterUnitMaterialLegacy");
		case ETCGEffectStepType::AttachSourceToUnitMaterial: return TEXT("AttachSourceToUnitMaterial");
		case ETCGEffectStepType::AttachGraveyardCardToSourceMaterial: return TEXT("AttachGraveyardCardToSourceMaterial");
		case ETCGEffectStepType::SendTopDeckCardsToGraveyard: return TEXT("SendTopDeckCardsToGraveyard");
		case ETCGEffectStepType::BoostAllOwnUnitsThisRound: return TEXT("BoostAllOwnUnitsThisRound");
		case ETCGEffectStepType::RevealTopDeckCardsAddElementToHand: return TEXT("RevealTopDeckCardsAddElementToHand");
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

	static const TCHAR* GetTCGEffectValueModeDebugName(ETCGEffectValueMode ValueMode)
	{
		switch (ValueMode)
		{
		case ETCGEffectValueMode::CardsUnderneathSource: return TEXT("CardsUnderneathSource");
		case ETCGEffectValueMode::CardsUnderneathTarget: return TEXT("CardsUnderneathTarget");
		case ETCGEffectValueMode::WaterCardsInControllerGraveyard: return TEXT("WaterCardsInControllerGraveyardLegacy");
		case ETCGEffectValueMode::ElementCardsInControllerGraveyard: return TEXT("ElementCardsInControllerGraveyard");
		default: return TEXT("Fixed");
		}
	}

	static const TCHAR* GetTCGEffectSelectionModeDebugName(ETCGEffectSelectionMode SelectionMode)
	{
		switch (SelectionMode)
		{
		case ETCGEffectSelectionMode::PlayerChoice: return TEXT("PlayerChoice");
		default: return TEXT("Automatic");
		}
	}

	static bool ConvertLegacyDebugEffectToSteps(FTCGCardEffectRef& EffectRef)
	{
		if (EffectRef.Steps.Num() > 0 || EffectRef.EffectId.IsNone()) return false;
		if (EffectRef.EffectId == LegacyDebugEffect_Draw1)
		{
			FTCGEffectStep DrawStep;
			DrawStep.StepType = ETCGEffectStepType::DrawCards;
			DrawStep.TargetMode = ETCGEffectTargetMode::Controller;
			DrawStep.Value = 1;
			EffectRef.Steps.Add(DrawStep);
			return true;
		}
		if (EffectRef.EffectId == LegacyDebugEffect_GainAttackForCardsUnderneath)
		{
			FTCGEffectStep GainAttackStep;
			GainAttackStep.StepType = ETCGEffectStepType::ModifyAttack;
			GainAttackStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
			GainAttackStep.ValueMode = ETCGEffectValueMode::CardsUnderneathTarget;
			EffectRef.Steps.Add(GainAttackStep);
			return true;
		}
		if (EffectRef.EffectId == LegacyDebugEffect_RemoveBottomOverlay)
		{
			FTCGEffectStep MoveOverlayStep;
			MoveOverlayStep.StepType = ETCGEffectStepType::MoveBottomOverlayToGraveyard;
			MoveOverlayStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
			EffectRef.Steps.Add(MoveOverlayStep);
			return true;
		}
		return false;
	}
}

bool ATCG_GameState::AddCardEffectRefToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef)
{
	const FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || EffectRef.Trigger == ETCGEffectTrigger::None) return false;
	FTCGCardEffectRef ResolvedEffectRef = EffectRef;
	const bool bConvertedLegacyEffect = ConvertLegacyDebugEffectToSteps(ResolvedEffectRef);
	if (ResolvedEffectRef.Steps.Num() <= 0) return false;
	FTCGEffectChainEntry NewEntry;
	NewEntry.ChainIndex = Chain.Num() + 1;
	NewEntry.SourceCardInstanceId = SourceCardInstanceId;
	NewEntry.TargetCardInstanceId = TargetCardInstanceId;
	NewEntry.SourceCardDefinitionId = SourceCard->CardDefinitionId;
	NewEntry.Trigger = ResolvedEffectRef.Trigger;
	NewEntry.EffectId = ResolvedEffectRef.EffectId;
	NewEntry.EffectRef = ResolvedEffectRef;
	NewEntry.ControllerPlayerIndex = SourceCard->OwnerPlayerIndex;
	ApplyDebugEffectChainEntryRequirements(NewEntry);
	Chain.Add(NewEntry);
	if (bLogEffectResolution)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain add %d Source=%s EffectId=%s Steps=%d Optional=%s ConvertedLegacy=%s"), NewEntry.ChainIndex, *NewEntry.SourceCardDefinitionId.ToString(), NewEntry.EffectId.IsNone() ? TEXT("None") : *NewEntry.EffectId.ToString(), NewEntry.EffectRef.Steps.Num(), NewEntry.EffectRef.bOptional ? TEXT("true") : TEXT("false"), bConvertedLegacyEffect ? TEXT("true") : TEXT("false"));
	}
	return true;
}

bool ATCG_GameState::ResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	if (ChainEntry.EffectRef.Steps.Num() <= 0)
	{
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain entry has no modular steps Chain=%d Source=%s"), ChainEntry.ChainIndex, *ChainEntry.SourceCardDefinitionId.ToString());
		return false;
	}
	return ResolveModularEffectChainEntry(ChainEntry);
}

bool ATCG_GameState::ResolveModularEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	if (ChainEntry.EffectRef.Steps.Num() <= 0) return false;
	bool bResolvedAnyStep = false;
	bool bPreviousMeaningfulStepSucceeded = true;
	if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Resolve modular chain %d Source=%s Steps=%d"), ChainEntry.ChainIndex, *ChainEntry.SourceCardDefinitionId.ToString(), ChainEntry.EffectRef.Steps.Num());
	for (const FTCGEffectStep& Step : ChainEntry.EffectRef.Steps)
	{
		if (Step.StepType == ETCGEffectStepType::None) continue;
		if (Step.StepType == ETCGEffectStepType::Then)
		{
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step Then PreviousSuccess=%s"), bPreviousMeaningfulStepSucceeded ? TEXT("true") : TEXT("false"));
			continue;
		}
		if (Step.bRequiresPreviousStepSuccess && !bPreviousMeaningfulStepSucceeded)
		{
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step skipped Type=%s Reason=PreviousStepFailed"), GetTCGEffectStepDebugName(Step.StepType));
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
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DrawCards Player=%d Requested=%d Drawn=%d Success=%s"), ChainEntry.ControllerPlayerIndex, DrawCount, DrawnCount, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::DiscardCards:
	{
		const int32 DiscardCount = FMath::Max(0, Step.Value);
		if (Step.SelectionMode == ETCGEffectSelectionMode::PlayerChoice)
		{
			const bool bChoiceStarted = BeginPendingDiscardChoice(ChainEntry.ControllerPlayerIndex, DiscardCount, ChainEntry);
			bool bAutoSubmittedChoice = false;
			if (bChoiceStarted && bAutoSubmitDebugDiscardChoice)
			{
				TArray<FGuid> ChoiceOptions;
				GetPendingDiscardChoiceOptions(ChoiceOptions);
				TArray<FGuid> DebugChosenCards;
				for (int32 Index = 0; Index < ChoiceOptions.Num() && DebugChosenCards.Num() < DiscardCount; ++Index) DebugChosenCards.Add(ChoiceOptions[Index]);
				bAutoSubmittedChoice = SubmitPendingDiscardChoice(ChainEntry.ControllerPlayerIndex, DebugChosenCards);
			}
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DiscardCards Player=%d Requested=%d Mode=%s Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, DiscardCount, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
			bStepSucceeded = bAutoSubmittedChoice;
			break;
		}
		const int32 DiscardedCount = DiscardCardsFromHand(ChainEntry.ControllerPlayerIndex, DiscardCount);
		bStepSucceeded = DiscardedCount == DiscardCount && DiscardCount > 0;
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DiscardCards Player=%d Requested=%d Discarded=%d Mode=%s Success=%s"), ChainEntry.ControllerPlayerIndex, DiscardCount, DiscardedCount, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::ModifyAttack:
	{
		FGuid TargetCardInstanceId;
		if (Step.TargetMode == ETCGEffectTargetMode::SourceCard || Step.TargetMode == ETCGEffectTargetMode::None) TargetCardInstanceId = ChainEntry.SourceCardInstanceId;
		else if (Step.TargetMode == ETCGEffectTargetMode::TriggerTarget) TargetCardInstanceId = ChainEntry.TargetCardInstanceId;
		int32 AttackDelta = Step.Value;
		if (Step.ValueMode == ETCGEffectValueMode::CardsUnderneathSource) AttackDelta = GetCardsUnderneathCount(ChainEntry.SourceCardInstanceId);
		else if (Step.ValueMode == ETCGEffectValueMode::CardsUnderneathTarget) AttackDelta = GetCardsUnderneathCount(ChainEntry.TargetCardInstanceId);
		else if (Step.ValueMode == ETCGEffectValueMode::WaterCardsInControllerGraveyard) AttackDelta = CountCardsInLocationByElement(ChainEntry.ControllerPlayerIndex, ETCGCardLocation::Graveyard, ETCGCardElement::Water);
		else if (Step.ValueMode == ETCGEffectValueMode::ElementCardsInControllerGraveyard) AttackDelta = CountCardsInLocationByElement(ChainEntry.ControllerPlayerIndex, ETCGCardLocation::Graveyard, Step.TargetFilter.RequiredElement);
		FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
		if (TargetCard && TargetCard->Location == ETCGCardLocation::Board)
		{
			TargetCard->AttackModifier += AttackDelta;
			bStepSucceeded = true;
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step ModifyAttack TargetMode=%s ValueMode=%s Amount=%d Success=%s"), GetTCGEffectTargetModeDebugName(Step.TargetMode), GetTCGEffectValueModeDebugName(Step.ValueMode), AttackDelta, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::SelectTarget:
	{
		const int32 OwnerPlayerIndex = Step.TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent ? 1 - ChainEntry.ControllerPlayerIndex : ChainEntry.ControllerPlayerIndex;
		for (const FTCGCardInstance& Card : MatchCards)
		{
			if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
			if (Card.Location != Step.TargetFilter.RequiredLocation) continue;
			if (Step.TargetFilter.bRequireElement && Card.Element != Step.TargetFilter.RequiredElement) continue;
			if (Step.TargetFilter.bRequireTopCard && Card.Location == ETCGCardLocation::Board)
			{
				const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
				if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
			}
			bStepSucceeded = true;
			break;
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step SelectTarget OwnerMode=%s Location=%d Success=%s"), GetTCGEffectTargetModeDebugName(Step.TargetFilter.OwnerMode), static_cast<int32>(Step.TargetFilter.RequiredLocation), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::MoveBottomOverlayToGraveyard:
	{
		const FGuid TargetCardInstanceId = Step.TargetMode == ETCGEffectTargetMode::SourceCard ? ChainEntry.SourceCardInstanceId : ChainEntry.TargetCardInstanceId;
		bStepSucceeded = MoveBottomOverlayToGraveyard(TargetCardInstanceId);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveBottomOverlayToGraveyard TargetMode=%s Success=%s"), GetTCGEffectTargetModeDebugName(Step.TargetMode), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::PlaySourceToEmptyZone:
	{
		if (Step.SelectionMode == ETCGEffectSelectionMode::PlayerChoice)
		{
			const bool bChoiceStarted = BeginPendingPlayToEmptyZoneChoice(ChainEntry.ControllerPlayerIndex, ChainEntry);
			bool bAutoSubmittedChoice = false;
			FName DebugChosenZoneId = NAME_None;
			if (bChoiceStarted && bAutoSubmitDebugPlayToEmptyZoneChoice)
			{
				TArray<FName> ChoiceOptions;
				GetPendingPlayToEmptyZoneChoiceOptions(ChoiceOptions);
				if (ChoiceOptions.Num() > 0)
				{
					DebugChosenZoneId = ChoiceOptions[0];
					bAutoSubmittedChoice = SubmitPendingPlayToEmptyZoneChoice(ChainEntry.ControllerPlayerIndex, DebugChosenZoneId);
				}
			}
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step PlaySourceToEmptyZone Player=%d Mode=%s Pending=%s AutoSubmitted=%s Zone=%s"), ChainEntry.ControllerPlayerIndex, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"), DebugChosenZoneId.IsNone() ? TEXT("None") : *DebugChosenZoneId.ToString());
			bStepSucceeded = bAutoSubmittedChoice;
			break;
		}
		bStepSucceeded = PlaySourceCardToFirstEmptyZone(ChainEntry.SourceCardInstanceId);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step PlaySourceToEmptyZone Success=%s"), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::AttachSourceToWaterUnitMaterial:
	case ETCGEffectStepType::AttachSourceToUnitMaterial:
	{
		const bool bChoiceStarted = BeginPendingAttachSourceToWaterUnitChoice(ChainEntry.ControllerPlayerIndex, ChainEntry);
		bool bAutoSubmittedChoice = false;
		if (bChoiceStarted && (Step.SelectionMode == ETCGEffectSelectionMode::Automatic || bAutoSubmitDebugAttachSourceToUnitChoice))
		{
			TArray<FGuid> ChoiceOptions;
			GetPendingAttachSourceToWaterUnitChoiceOptions(ChoiceOptions);
			if (ChoiceOptions.Num() > 0) bAutoSubmittedChoice = SubmitPendingAttachSourceToWaterUnitChoice(ChainEntry.ControllerPlayerIndex, ChoiceOptions[0]);
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step %s Player=%d Mode=%s Pending=%s AutoSubmitted=%s"), GetTCGEffectStepDebugName(Step.StepType), ChainEntry.ControllerPlayerIndex, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
		bStepSucceeded = bAutoSubmittedChoice;
		break;
	}
	case ETCGEffectStepType::AttachGraveyardCardToSourceMaterial:
	{
		const FGuid SourceUnitId = Step.TargetMode == ETCGEffectTargetMode::TriggerTarget ? ChainEntry.TargetCardInstanceId : ChainEntry.SourceCardInstanceId;
		bStepSucceeded = AttachGraveyardCardToSourceMaterial(ChainEntry.ControllerPlayerIndex, SourceUnitId, Step.TargetFilter);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step AttachGraveyardCardToSourceMaterial Player=%d Mode=%s Success=%s"), ChainEntry.ControllerPlayerIndex, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::SendTopDeckCardToGraveyard:
	{
		bStepSucceeded = SendTopDeckCardToGraveyard(ChainEntry.ControllerPlayerIndex);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step SendTopDeckCardToGraveyard Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::SendTopDeckCardsToGraveyard:
	{
		const int32 SendCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);
		const int32 SentCount = SendTopDeckCardsToGraveyard(ChainEntry.ControllerPlayerIndex, SendCount);
		bStepSucceeded = SentCount == SendCount;
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step SendTopDeckCardsToGraveyard Player=%d Requested=%d Sent=%d Success=%s"), ChainEntry.ControllerPlayerIndex, SendCount, SentCount, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::RevealTopDeckCardsAddElementToHand:
		{
			const int32 RevealCount = FMath::Max(1, Step.Value <= 0 ? 2 : Step.Value);

			if (Step.SelectionMode == ETCGEffectSelectionMode::PlayerChoice)
			{
				const bool bChoiceStarted = BeginPendingRevealDeckChoice(
					ChainEntry.ControllerPlayerIndex,
					RevealCount,
					Step.TargetFilter,
					ChainEntry);

				bool bAutoSubmittedChoice = false;

				if (bChoiceStarted)
				{
					TArray<FGuid> ChoiceOptions;
					GetPendingRevealDeckChoiceOptions(ChoiceOptions);

					// Debug auto-submit for now, until UI submits the real chosen card.
					if (ChoiceOptions.Num() > 0)
					{
						bAutoSubmittedChoice = SubmitPendingRevealDeckChoice(
							ChainEntry.ControllerPlayerIndex,
							ChoiceOptions[0]);
					}
				}

				if (bLogEffectResolution)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("TCG Effect: Step RevealTopDeckCardsAddElementToHand Player=%d Reveal=%d Mode=%s Pending=%s AutoSubmitted=%s"),
						ChainEntry.ControllerPlayerIndex,
						RevealCount,
						GetTCGEffectSelectionModeDebugName(Step.SelectionMode),
						bChoiceStarted ? TEXT("true") : TEXT("false"),
						bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
				}

				bStepSucceeded = bAutoSubmittedChoice;
				break;
			}

			bStepSucceeded = RevealTopDeckCardsAddElementToHand(
				ChainEntry.ControllerPlayerIndex,
				RevealCount,
				Step.TargetFilter);

			if (bLogEffectResolution)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("TCG Effect: Step RevealTopDeckCardsAddElementToHand Player=%d Reveal=%d Success=%s"),
					ChainEntry.ControllerPlayerIndex,
					RevealCount,
					bStepSucceeded ? TEXT("true") : TEXT("false"));
			}

			break;
		}
	case ETCGEffectStepType::MoveGraveyardCardToBottomDeck:
	{
		const int32 MoveCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);
		if (Step.SelectionMode == ETCGEffectSelectionMode::PlayerChoice)
		{
			const bool bChoiceStarted = BeginPendingGraveyardToDeckChoice(ChainEntry.ControllerPlayerIndex, MoveCount, ChainEntry);
			bool bAutoSubmittedChoice = false;
			if (bChoiceStarted && bAutoSubmitDebugGraveyardToDeckChoice)
			{
				TArray<FGuid> ChoiceOptions;
				GetPendingGraveyardToDeckChoiceOptions(ChoiceOptions);
				TArray<FGuid> DebugChosenCards;
				for (int32 Index = 0; Index < ChoiceOptions.Num() && DebugChosenCards.Num() < MoveCount; ++Index) DebugChosenCards.Add(ChoiceOptions[Index]);
				bAutoSubmittedChoice = SubmitPendingGraveyardToDeckChoice(ChainEntry.ControllerPlayerIndex, DebugChosenCards);
			}
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveGraveyardCardToBottomDeck Player=%d Requested=%d Mode=%s Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, MoveCount, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
			bStepSucceeded = bAutoSubmittedChoice;
			break;
		}
		bStepSucceeded = MoveFirstGraveyardCardToBottomDeck(ChainEntry.ControllerPlayerIndex);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveGraveyardCardToBottomDeck Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::MoveHandCardToTopDeck:
	{
		const int32 MoveCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);
		if (Step.SelectionMode == ETCGEffectSelectionMode::PlayerChoice)
		{
			const bool bChoiceStarted = BeginPendingHandToTopDeckChoice(ChainEntry.ControllerPlayerIndex, MoveCount, ChainEntry);
			bool bAutoSubmittedChoice = false;
			if (bChoiceStarted && bAutoSubmitDebugHandToTopDeckChoice)
			{
				TArray<FGuid> ChoiceOptions;
				GetPendingHandToTopDeckChoiceOptions(ChoiceOptions);
				TArray<FGuid> DebugChosenCards;
				for (int32 Index = 0; Index < ChoiceOptions.Num() && DebugChosenCards.Num() < MoveCount; ++Index) DebugChosenCards.Add(ChoiceOptions[Index]);
				bAutoSubmittedChoice = SubmitPendingHandToTopDeckChoice(ChainEntry.ControllerPlayerIndex, DebugChosenCards);
			}
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveHandCardToTopDeck Player=%d Requested=%d Mode=%s Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, MoveCount, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
			bStepSucceeded = bAutoSubmittedChoice;
			break;
		}
		bStepSucceeded = MoveFirstHandCardToTopDeck(ChainEntry.ControllerPlayerIndex);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveHandCardToTopDeck Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	default:
		break;
	}
	if (bLogEffectResolution && Step.StepType != ETCGEffectStepType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step result Type=%s PreviousSuccess=%s Success=%s"), GetTCGEffectStepDebugName(Step.StepType), bPreviousStepSucceeded ? TEXT("true") : TEXT("false"), bStepSucceeded ? TEXT("true") : TEXT("false"));
	}
	return bStepSucceeded;
}

bool ATCG_GameState::BeginPendingDiscardChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingDiscardChoice.bIsPending) return false;
	TArray<FTCGCardInstance> HandCards;
	GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() < Count) return false;
	PendingDiscardChoice.Reset();
	PendingDiscardChoice.bIsPending = true;
	PendingDiscardChoice.PlayerIndex = PlayerIndex;
	PendingDiscardChoice.RequiredCount = Count;
	PendingDiscardChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingDiscardChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : HandCards) PendingDiscardChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingDiscardChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingDiscardChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingDiscardChoice.bIsPending || PendingDiscardChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingDiscardChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingDiscardChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Hand) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToLocation(ChosenId, ETCGCardLocation::Graveyard);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending discard choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingDiscardChoice();
	return true;
}

bool ATCG_GameState::HasPendingDiscardChoice() const { return PendingDiscardChoice.bIsPending; }
void ATCG_GameState::GetPendingDiscardChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingDiscardChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingDiscardChoice() { PendingDiscardChoice.Reset(); }

bool ATCG_GameState::BeginPendingGraveyardToDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingGraveyardToDeckChoice.bIsPending) return false;
	TArray<FTCGCardInstance> GraveyardCards;
	GetCardsInLocation(PlayerIndex, ETCGCardLocation::Graveyard, GraveyardCards);
	if (GraveyardCards.Num() < Count) return false;
	GraveyardCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B){ return A.LocationIndex < B.LocationIndex; });
	PendingGraveyardToDeckChoice.Reset();
	PendingGraveyardToDeckChoice.bIsPending = true;
	PendingGraveyardToDeckChoice.PlayerIndex = PlayerIndex;
	PendingGraveyardToDeckChoice.RequiredCount = Count;
	PendingGraveyardToDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingGraveyardToDeckChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : GraveyardCards) PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingGraveyardToDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingGraveyardToDeckChoice.bIsPending || PendingGraveyardToDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingGraveyardToDeckChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Graveyard) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToBottomOfDeck(ChosenId);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending graveyard-to-deck choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingGraveyardToDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingGraveyardToDeckChoice() const { return PendingGraveyardToDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingGraveyardToDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingGraveyardToDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingGraveyardToDeckChoice() { PendingGraveyardToDeckChoice.Reset(); }

bool ATCG_GameState::BeginPendingHandToTopDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingHandToTopDeckChoice.bIsPending) return false;
	TArray<FTCGCardInstance> HandCards;
	GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() < Count) return false;
	PendingHandToTopDeckChoice.Reset();
	PendingHandToTopDeckChoice.bIsPending = true;
	PendingHandToTopDeckChoice.PlayerIndex = PlayerIndex;
	PendingHandToTopDeckChoice.RequiredCount = Count;
	PendingHandToTopDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingHandToTopDeckChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : HandCards) PendingHandToTopDeckChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingHandToTopDeckChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingHandToTopDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingHandToTopDeckChoice.bIsPending || PendingHandToTopDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingHandToTopDeckChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingHandToTopDeckChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Hand) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToTopOfDeck(ChosenId);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending hand-to-top-deck choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingHandToTopDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingHandToTopDeckChoice() const { return PendingHandToTopDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingHandToTopDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingHandToTopDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingHandToTopDeckChoice() { PendingHandToTopDeckChoice.Reset(); }

bool ATCG_GameState::BeginPendingPlayToEmptyZoneChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingPlayToEmptyZoneChoice.bIsPending) return false;
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	if (!SourceCard || SourceCard->OwnerPlayerIndex != PlayerIndex) return false;
	PendingPlayToEmptyZoneChoice.Reset();
	PendingPlayToEmptyZoneChoice.bIsPending = true;
	PendingPlayToEmptyZoneChoice.PlayerIndex = PlayerIndex;
	PendingPlayToEmptyZoneChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingPlayToEmptyZoneChoice.ChainIndex = ChainEntry.ChainIndex;
	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);
		FGuid ExistingStackId;
		if (!FindStackIdInZone(ZoneId, ExistingStackId)) PendingPlayToEmptyZoneChoice.EligibleZoneIds.Add(ZoneId);
	}
	if (PendingPlayToEmptyZoneChoice.EligibleZoneIds.Num() <= 0){ ClearPendingPlayToEmptyZoneChoice(); return false; }
	return true;
}

bool ATCG_GameState::SubmitPendingPlayToEmptyZoneChoice(int32 PlayerIndex, FName ChosenZoneId)
{
	if (!PendingPlayToEmptyZoneChoice.bIsPending || PendingPlayToEmptyZoneChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenZoneId.IsNone() || !PendingPlayToEmptyZoneChoice.EligibleZoneIds.Contains(ChosenZoneId)) return false;
	const FGuid SourceCardInstanceId = PendingPlayToEmptyZoneChoice.SourceCardInstanceId;
	const bool bPlayed = PlaySourceCardToEmptyZone(SourceCardInstanceId, ChosenZoneId);
	if (!bPlayed) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending play-to-empty-zone choice submitted Player=%d Zone=%s"), PlayerIndex, *ChosenZoneId.ToString());
	ClearPendingPlayToEmptyZoneChoice();
	return true;
}

bool ATCG_GameState::HasPendingPlayToEmptyZoneChoice() const { return PendingPlayToEmptyZoneChoice.bIsPending; }
void ATCG_GameState::GetPendingPlayToEmptyZoneChoiceOptions(TArray<FName>& OutZoneIds) const { OutZoneIds = PendingPlayToEmptyZoneChoice.EligibleZoneIds; }
void ATCG_GameState::ClearPendingPlayToEmptyZoneChoice() { PendingPlayToEmptyZoneChoice.Reset(); }

bool ATCG_GameState::BeginPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingAttachSourceToWaterUnitChoice.bIsPending) return false;
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	if (!SourceCard || SourceCard->OwnerPlayerIndex != PlayerIndex || SourceCard->Location != ETCGCardLocation::Graveyard) return false;
	const FTCGEffectStep* AttachStep = nullptr;
	for (const FTCGEffectStep& Step : ChainEntry.EffectRef.Steps)
	{
		if (Step.StepType == ETCGEffectStepType::AttachSourceToUnitMaterial || Step.StepType == ETCGEffectStepType::AttachSourceToWaterUnitMaterial)
		{
			AttachStep = &Step;
			break;
		}
	}
	PendingAttachSourceToWaterUnitChoice.Reset();
	PendingAttachSourceToWaterUnitChoice.bIsPending = true;
	PendingAttachSourceToWaterUnitChoice.PlayerIndex = PlayerIndex;
	PendingAttachSourceToWaterUnitChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingAttachSourceToWaterUnitChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Board) continue;
		if (AttachStep)
		{
			if (Card.Location != AttachStep->TargetFilter.RequiredLocation) continue;
			if (AttachStep->TargetFilter.bRequireElement && Card.Element != AttachStep->TargetFilter.RequiredElement) continue;
			if (AttachStep->StepType == ETCGEffectStepType::AttachSourceToWaterUnitMaterial && Card.Element != ETCGCardElement::Water) continue;
			if (AttachStep->TargetFilter.bRequireTopCard)
			{
				const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
				if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
			}
		}
		PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Add(Card.CardInstanceId);
	}
	if (PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Num() <= 0){ ClearPendingAttachSourceToWaterUnitChoice(); return false; }
	return true;
}

bool ATCG_GameState::SubmitPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FGuid& ChosenTargetCardInstanceId)
{
	if (!PendingAttachSourceToWaterUnitChoice.bIsPending || PendingAttachSourceToWaterUnitChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenTargetCardInstanceId.IsValid() || !PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Contains(ChosenTargetCardInstanceId)) return false;
	const FGuid SourceCardInstanceId = PendingAttachSourceToWaterUnitChoice.SourceCardInstanceId;
	const bool bAttached = AttachSourceCardUnderWaterUnit(SourceCardInstanceId, ChosenTargetCardInstanceId);
	if (!bAttached) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending attach-source-to-unit choice submitted Player=%d"), PlayerIndex);
	ClearPendingAttachSourceToWaterUnitChoice();
	return true;
}

bool ATCG_GameState::HasPendingAttachSourceToWaterUnitChoice() const { return PendingAttachSourceToWaterUnitChoice.bIsPending; }
void ATCG_GameState::GetPendingAttachSourceToWaterUnitChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds; }
void ATCG_GameState::ClearPendingAttachSourceToWaterUnitChoice() { PendingAttachSourceToWaterUnitChoice.Reset(); }

bool ATCG_GameState::MoveCardToTopOfDeck(const FGuid& CardInstanceId)
{
	FTCGCardInstance* CardToMove = FindCardInstance(CardInstanceId);
	if (!CardToMove || !IsValidPlayerIndex(CardToMove->OwnerPlayerIndex)) return false;
	const int32 PlayerIndex = CardToMove->OwnerPlayerIndex;
	const FName CardDefinitionId = CardToMove->CardDefinitionId;
	CardToMove->Location = ETCGCardLocation::Deck;
	CardToMove->LocationIndex = GetNextLocationIndex(PlayerIndex, ETCGCardLocation::Deck);
	CardToMove->ZoneId = NAME_None;
	CardToMove->StackId.Invalidate();
	CardToMove->StackIndex = INDEX_NONE;
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Moved card to top deck Player=%d Card=%s"), PlayerIndex, *CardDefinitionId.ToString());
	return true;
}

bool ATCG_GameState::AttachGraveyardCardToSourceMaterial(int32 PlayerIndex, const FGuid& SourceCardInstanceId, const FTCGEffectTargetFilter& TargetFilter)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || SourceCard->OwnerPlayerIndex != PlayerIndex || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid()) return false;

	FTCGCardInstance* ChosenCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Graveyard) continue;
		if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
		if (!ChosenCard || Card.LocationIndex < ChosenCard->LocationIndex) ChosenCard = &Card;
	}
	if (!ChosenCard) return false;

	const FName AttachedCardDefinitionId = ChosenCard->CardDefinitionId;
	const FGuid TargetStackId = SourceCard->StackId;
	const FName TargetZoneId = SourceCard->ZoneId;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == TargetStackId) Card.StackIndex += 1;
	}

	ChosenCard->Location = ETCGCardLocation::Board;
	ChosenCard->LocationIndex = INDEX_NONE;
	ChosenCard->ZoneId = TargetZoneId;
	ChosenCard->StackId = TargetStackId;
	ChosenCard->StackIndex = 0;

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Attached graveyard card as material Source=%s Material=%s Zone=%s"), *SourceCard->CardDefinitionId.ToString(), *AttachedCardDefinitionId.ToString(), *TargetZoneId.ToString());
	return true;
}

bool ATCG_GameState::MoveFirstHandCardToTopDeck(int32 PlayerIndex)
{
	if (!IsValidPlayerIndex(PlayerIndex)) return false;
	FTCGCardInstance* HandCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Hand) continue;
		if (!HandCard || Card.LocationIndex < HandCard->LocationIndex) HandCard = &Card;
	}
	return HandCard ? MoveCardToTopOfDeck(HandCard->CardInstanceId) : false;
}

int32 ATCG_GameState::DiscardCardsFromHand(int32 PlayerIndex, int32 Count)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0) return 0;
	TArray<FTCGCardInstance*> HandCards;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Hand) HandCards.Add(&Card);
	}
	HandCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B){ return A.LocationIndex < B.LocationIndex; });
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
