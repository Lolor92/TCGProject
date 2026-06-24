#include "GameState/TCG_GameState.h"
#include "GameState/TCG_EffectResolver.h"

namespace
{
	constexpr bool bLogEffectResolution = false;
	constexpr bool bAutoSubmitDebugDiscardChoice = true;
	constexpr bool bAutoSubmitDebugGraveyardToDeckChoice = true;
	constexpr bool bAutoSubmitDebugHandToTopDeckChoice = true;
	constexpr bool bAutoSubmitDebugPlayToEmptyZoneChoice = true;
	constexpr bool bAutoSubmitDebugAttachSourceToUnitChoice = true;
	constexpr bool bAutoSubmitDebugPlayGraveyardCardToEmptyZoneChoice = true;
	constexpr bool bAutoSubmitDebugDestroyedRecoveryChoice = true;

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
		case ETCGEffectStepType::MoveGraveyardCardToHand: return TEXT("MoveGraveyardCardToHand");
		case ETCGEffectStepType::MoveGraveyardCardToTopDeck: return TEXT("MoveGraveyardCardToTopDeck");
		case ETCGEffectStepType::MoveGraveyardCardToBottomDeck: return TEXT("MoveGraveyardCardToBottomDeck");
		case ETCGEffectStepType::MoveHandCardToTopDeck: return TEXT("MoveHandCardToTopDeck");
		case ETCGEffectStepType::AttachSourceToWaterUnitMaterial: return TEXT("AttachSourceToWaterUnitMaterialLegacy");
		case ETCGEffectStepType::AttachSourceToUnitMaterial: return TEXT("AttachSourceToUnitMaterial");
		case ETCGEffectStepType::AttachGraveyardCardToSourceMaterial: return TEXT("AttachGraveyardCardToSourceMaterial");
		case ETCGEffectStepType::SendTopDeckCardsToGraveyard: return TEXT("SendTopDeckCardsToGraveyard");
		case ETCGEffectStepType::BoostAllOwnUnitsThisRound: return TEXT("BoostAllOwnUnitsThisRound");
		case ETCGEffectStepType::RevealTopDeckCardsAddElementToHand: return TEXT("RevealTopDeckCardsAddElementToHand");
		case ETCGEffectStepType::PlayGraveyardCardToEmptyZone: return TEXT("PlayGraveyardCardToEmptyZone");
		case ETCGEffectStepType::MoveGraveyardCardsToHandAndTopDeck: return TEXT("MoveGraveyardCardsToHandAndTopDeck");
		case ETCGEffectStepType::RemoveMaterialFromTargetUnit: return TEXT("RemoveMaterialFromTargetUnit");
		case ETCGEffectStepType::AttackMillTwoWaterBounceBattlingUnit: return TEXT("AttackMillTwoWaterBounceBattlingUnit");
		case ETCGEffectStepType::DiscardSourceDetachUpToTwoMaterialsFromTarget: return TEXT("DiscardSourceDetachUpToTwoMaterialsFromTarget");
		case ETCGEffectStepType::DestroyTargetUnitByCardEffect: return TEXT("DestroyTargetUnitByCardEffect");
		case ETCGEffectStepType::DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials: return TEXT("DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials");
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

	static void GetFilteredGraveyardCards(ATCG_GameState* GameState, int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, const FTCGEffectChainEntry& ChainEntry, TArray<FGuid>& OutCardIds)
	{
		OutCardIds.Reset();
		if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex)) return;

		const int32 OwnerPlayerIndex = TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent ? 1 - PlayerIndex : PlayerIndex;
		for (const FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
			if (Card.Location != ETCGCardLocation::Graveyard) continue;
			if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
			if (TargetFilter.bExcludeSourceCard && Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
			OutCardIds.Add(Card.CardInstanceId);
		}
	}

	static bool MoveFirstFilteredGraveyardCardToHand(ATCG_GameState* GameState, int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, const FTCGEffectChainEntry& ChainEntry)
	{
		TArray<FGuid> Options;
		GetFilteredGraveyardCards(GameState, PlayerIndex, TargetFilter, ChainEntry, Options);
		if (Options.Num() <= 0) return false;
		return GameState->MoveCardToLocation(Options[0], ETCGCardLocation::Hand);
	}

	static bool MoveFirstFilteredGraveyardCardToTopDeck(ATCG_GameState* GameState, int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, const FTCGEffectChainEntry& ChainEntry)
	{
		TArray<FGuid> Options;
		GetFilteredGraveyardCards(GameState, PlayerIndex, TargetFilter, ChainEntry, Options);
		if (Options.Num() <= 0) return false;
		return GameState->MoveCardToTopOfDeck(Options[0]);
	}

	static bool DiscardSourceDetachUpToTwoMaterialsFromTarget(ATCG_GameState* GameState, const FTCGEffectChainEntry& ChainEntry)
	{
		if (!GameState) return false;

		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
		const FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);
		if (!SourceCard || SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex || SourceCard->Location != ETCGCardLocation::Hand)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Discard source detach failed Source=%s Reason=SourceNotInHand"), ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());
			return false;
		}
		if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Discard source detach failed Source=%s Reason=TargetNotOnBoard"), *SourceCard->CardDefinitionId.ToString());
			return false;
		}

		const FGuid TargetCardId = ChainEntry.TargetCardInstanceId;
		const FName SourceDefinitionId = SourceCard->CardDefinitionId;
		const FName TargetDefinitionId = TargetCard->CardDefinitionId;
		const bool bDiscardedSource = GameState->MoveCardToLocation(ChainEntry.SourceCardInstanceId, ETCGCardLocation::Graveyard);
		if (!bDiscardedSource)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Discard source detach failed Source=%s Reason=DiscardFailed"), *SourceDefinitionId.ToString());
			return false;
		}

		int32 DetachedCount = 0;
		for (int32 MaterialIndex = 0; MaterialIndex < 2; ++MaterialIndex)
		{
			if (!GameState->MoveBottomOverlayToGraveyard(TargetCardId)) break;
			DetachedCount++;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Discard source detach Source=%s Target=%s Discarded=true Detached=%d"),
			*SourceDefinitionId.ToString(),
			*TargetDefinitionId.ToString(),
			DetachedCount);

		return true;
	}

	static bool ReturnUnitToHandMaterialsToGraveyard(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId, int32& OutMaterialCount)
	{
		OutMaterialCount = 0;
		if (!GameState) return false;

		const FTCGCardInstance* TargetTopCard = GameState->FindCardInstance(TargetTopCardInstanceId);
		if (!TargetTopCard || TargetTopCard->Location != ETCGCardLocation::Board || !TargetTopCard->StackId.IsValid()) return false;

		const FGuid StackId = TargetTopCard->StackId;
		const FGuid TopCardId = TargetTopCard->CardInstanceId;

		TArray<FGuid> MaterialIds;
		for (const FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.Location != ETCGCardLocation::Board) continue;
			if (Card.StackId != StackId) continue;
			if (Card.CardInstanceId == TopCardId) continue;
			if (Card.StackIndex >= TargetTopCard->StackIndex) continue;
			MaterialIds.Add(Card.CardInstanceId);
		}

		OutMaterialCount = MaterialIds.Num();
		bool bMovedMaterials = true;
		for (const FGuid& MaterialId : MaterialIds)
		{
			bMovedMaterials &= GameState->MoveCardToLocation(MaterialId, ETCGCardLocation::Graveyard);
		}

		const bool bMovedTop = GameState->MoveCardToLocation(TopCardId, ETCGCardLocation::Hand);
		return bMovedMaterials && bMovedTop;
	}

	static bool DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials(ATCG_GameState* GameState, const FTCGEffectChainEntry& ChainEntry)
	{
		if (!GameState) return false;

		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
		const FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);
		if (!SourceCard || SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex || SourceCard->Location != ETCGCardLocation::Hand)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Unit save failed Source=%s Reason=SourceNotInHand"), ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());
			return false;
		}
		if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board || TargetCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Unit save failed Source=%s Reason=TargetInvalid"), *SourceCard->CardDefinitionId.ToString());
			return false;
		}

		const FName SourceDefinitionId = SourceCard->CardDefinitionId;
		const FName TargetDefinitionId = TargetCard->CardDefinitionId;
		const bool bDiscardedSource = GameState->MoveCardToLocation(ChainEntry.SourceCardInstanceId, ETCGCardLocation::Graveyard);
		if (!bDiscardedSource)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Unit save failed Source=%s Reason=DiscardFailed"), *SourceDefinitionId.ToString());
			return false;
		}

		int32 MaterialCount = 0;
		const bool bReturnedUnit = ReturnUnitToHandMaterialsToGraveyard(GameState, ChainEntry.TargetCardInstanceId, MaterialCount);
		const int32 DrawnCount = bReturnedUnit && MaterialCount >= 2 ? GameState->DrawCards(ChainEntry.ControllerPlayerIndex, 1) : 0;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Unit save Source=%s Target=%s Discarded=true Returned=%s Materials=%d Drawn=%d"),
			*SourceDefinitionId.ToString(),
			*TargetDefinitionId.ToString(),
			bReturnedUnit ? TEXT("true") : TEXT("false"),
			MaterialCount,
			DrawnCount);

		return bReturnedUnit;
	}

	static bool EffectRefHasStep(const FTCGCardEffectRef& EffectRef, ETCGEffectStepType StepType)
	{
		for (const FTCGEffectStep& Step : EffectRef.Steps)
		{
			if (Step.StepType == StepType)
			{
				return true;
			}
		}

		return false;
	}

	static bool TryHandSaveReplacementForCardEffect(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId)
	{
		if (!GameState) return false;

		const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetTopCardInstanceId);
		if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board)
		{
			return false;
		}

		TArray<FTCGCardInstance> HandCards;
		GameState->GetCardsInHand(TargetCard->OwnerPlayerIndex, HandCards);

		for (const FTCGCardInstance& HandCard : HandCards)
		{
			TArray<FTCGCardEffectRef> EffectRefs;
			GameState->GetPrintedEffectRefsForCard(HandCard, EffectRefs);

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (!GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnYourUnitWouldBeDestroyedByCardEffect))
				{
					continue;
				}

				if (!EffectRefHasStep(EffectRef, ETCGEffectStepType::DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials))
				{
					continue;
				}

				FTCGEffectChainEntry ReplacementEntry;
				ReplacementEntry.ChainIndex = 1;
				ReplacementEntry.SourceCardInstanceId = HandCard.CardInstanceId;
				ReplacementEntry.TargetCardInstanceId = TargetTopCardInstanceId;
				ReplacementEntry.SourceCardDefinitionId = HandCard.CardDefinitionId;
				ReplacementEntry.Trigger = ETCGEffectTrigger::OnYourUnitWouldBeDestroyedByCardEffect;
				ReplacementEntry.EffectRef = EffectRef;
				ReplacementEntry.ControllerPlayerIndex = HandCard.OwnerPlayerIndex;
				ReplacementEntry.bRequiresSourceOnBoard = false;
				ReplacementEntry.bRequiresTargetOnBoard = true;

				return DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials(GameState, ReplacementEntry);
			}
		}

		return false;
	}

	static bool DestroyTargetUnitByCardEffect(ATCG_GameState* GameState, const FTCGEffectChainEntry& ChainEntry, const FTCGEffectStep& Step)
	{
		if (!GameState) return false;

		const FGuid TargetCardInstanceId =
			Step.TargetMode == ETCGEffectTargetMode::SourceCard
				? ChainEntry.SourceCardInstanceId
				: ChainEntry.TargetCardInstanceId;

		const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);
		if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board || !TargetCard->StackId.IsValid())
		{
			return false;
		}

		if (TryHandSaveReplacementForCardEffect(GameState, TargetCardInstanceId))
		{
			return true;
		}

		return GameState->MoveStackToLocation(TargetCard->StackId, ETCGCardLocation::Graveyard);
	}
}

bool ATCG_GameState::AddCardEffectRefToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef)
{
	return UTCG_EffectResolver::AddCardEffectRefToChain(this, Chain, SourceCardInstanceId, TargetCardInstanceId, EffectRef);
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
			if (Step.TargetFilter.bExcludeSourceCard && Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
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
	case ETCGEffectStepType::PlayGraveyardCardToEmptyZone:
	{
		const bool bChoiceStarted = BeginPendingPlayGraveyardCardToEmptyZoneChoice(ChainEntry.ControllerPlayerIndex, Step.TargetFilter, ChainEntry);
		bool bAutoSubmittedChoice = false;
		FGuid DebugChosenCardId;
		FName DebugChosenZoneId = NAME_None;
		if (bChoiceStarted && bAutoSubmitDebugPlayGraveyardCardToEmptyZoneChoice)
		{
			TArray<FGuid> CardOptions;
			TArray<FName> ZoneOptions;
			GetPendingPlayGraveyardCardToEmptyZoneCardOptions(CardOptions);
			GetPendingPlayGraveyardCardToEmptyZoneZoneOptions(ZoneOptions);
			if (CardOptions.Num() > 0 && ZoneOptions.Num() > 0)
			{
				DebugChosenCardId = CardOptions[0];
				DebugChosenZoneId = ZoneOptions[0];
				bAutoSubmittedChoice = SubmitPendingPlayGraveyardCardToEmptyZoneChoice(ChainEntry.ControllerPlayerIndex, DebugChosenCardId, DebugChosenZoneId);
			}
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step PlayGraveyardCardToEmptyZone Player=%d Pending=%s AutoSubmitted=%s Zone=%s"), ChainEntry.ControllerPlayerIndex, bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"), DebugChosenZoneId.IsNone() ? TEXT("None") : *DebugChosenZoneId.ToString());
		bStepSucceeded = bAutoSubmittedChoice;
		break;
	}
	case ETCGEffectStepType::MoveGraveyardCardToHand:
	{
		bStepSucceeded = MoveFirstFilteredGraveyardCardToHand(this, ChainEntry.ControllerPlayerIndex, Step.TargetFilter, ChainEntry);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveGraveyardCardToHand Player=%d Mode=%s Success=%s"), ChainEntry.ControllerPlayerIndex, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::MoveGraveyardCardToTopDeck:
	{
		bStepSucceeded = MoveFirstFilteredGraveyardCardToTopDeck(this, ChainEntry.ControllerPlayerIndex, Step.TargetFilter, ChainEntry);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveGraveyardCardToTopDeck Player=%d Mode=%s Success=%s"), ChainEntry.ControllerPlayerIndex, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::MoveGraveyardCardsToHandAndTopDeck:
	{
		const bool bChoiceStarted = BeginPendingGraveyardCardsToHandAndTopDeckChoice(ChainEntry.ControllerPlayerIndex, Step.TargetFilter, ChainEntry);
		bool bAutoSubmittedChoice = false;
		if (bChoiceStarted && bAutoSubmitDebugDestroyedRecoveryChoice)
		{
			TArray<FGuid> ChoiceOptions;
			GetPendingGraveyardCardsToHandAndTopDeckOptions(ChoiceOptions);
			if (ChoiceOptions.Num() >= 2)
			{
				bAutoSubmittedChoice = SubmitPendingGraveyardCardsToHandAndTopDeckChoice(ChainEntry.ControllerPlayerIndex, ChoiceOptions[0], ChoiceOptions[1]);
			}
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step MoveGraveyardCardsToHandAndTopDeck Player=%d Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
		bStepSucceeded = bAutoSubmittedChoice;
		break;
	}
	case ETCGEffectStepType::RemoveMaterialFromTargetUnit:
	{
		const FGuid TargetCardInstanceId = Step.TargetMode == ETCGEffectTargetMode::SourceCard ? ChainEntry.SourceCardInstanceId : ChainEntry.TargetCardInstanceId;
		const bool bChoiceStarted = BeginPendingRemoveMaterialChoice(ChainEntry.ControllerPlayerIndex, TargetCardInstanceId, ChainEntry);
		bool bAutoSubmittedChoice = false;
		if (bChoiceStarted && bAutoSubmitDebugDestroyedRecoveryChoice)
		{
			TArray<FGuid> ChoiceOptions;
			GetPendingRemoveMaterialChoiceOptions(ChoiceOptions);
			if (ChoiceOptions.Num() > 0)
			{
				bAutoSubmittedChoice = SubmitPendingRemoveMaterialChoice(ChainEntry.ControllerPlayerIndex, ChoiceOptions[0]);
			}
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step RemoveMaterialFromTargetUnit Player=%d Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
		bStepSucceeded = bAutoSubmittedChoice;
		break;
	}
	case ETCGEffectStepType::AttackMillTwoWaterBounceBattlingUnit:
	{
		bStepSucceeded = ResolveAttackMillTwoWaterBounceBattlingUnit(ChainEntry.SourceCardInstanceId, ChainEntry.TargetCardInstanceId);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step AttackMillTwoWaterBounceBattlingUnit Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::DiscardSourceDetachUpToTwoMaterialsFromTarget:
	{
		bStepSucceeded = DiscardSourceDetachUpToTwoMaterialsFromTarget(this, ChainEntry);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DiscardSourceDetachUpToTwoMaterialsFromTarget Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::DestroyTargetUnitByCardEffect:
	{
		bStepSucceeded = DestroyTargetUnitByCardEffect(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DestroyTargetUnitByCardEffect Player=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials:
	{
		bStepSucceeded = DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials(this, ChainEntry);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
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
			const bool bChoiceStarted = BeginPendingRevealDeckChoice(ChainEntry.ControllerPlayerIndex, RevealCount, Step.TargetFilter, ChainEntry);
			bool bAutoSubmittedChoice = false;
			if (bChoiceStarted)
			{
				TArray<FGuid> ChoiceOptions;
				GetPendingRevealDeckChoiceOptions(ChoiceOptions);
				if (ChoiceOptions.Num() > 0) bAutoSubmittedChoice = SubmitPendingRevealDeckChoice(ChainEntry.ControllerPlayerIndex, ChoiceOptions[0]);
			}
			if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step RevealTopDeckCardsAddElementToHand Player=%d Reveal=%d Mode=%s Pending=%s AutoSubmitted=%s"), ChainEntry.ControllerPlayerIndex, RevealCount, GetTCGEffectSelectionModeDebugName(Step.SelectionMode), bChoiceStarted ? TEXT("true") : TEXT("false"), bAutoSubmittedChoice ? TEXT("true") : TEXT("false"));
			bStepSucceeded = bAutoSubmittedChoice;
			break;
		}
		bStepSucceeded = RevealTopDeckCardsAddElementToHand(ChainEntry.ControllerPlayerIndex, RevealCount, Step.TargetFilter);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step RevealTopDeckCardsAddElementToHand Player=%d Reveal=%d Success=%s"), ChainEntry.ControllerPlayerIndex, RevealCount, bStepSucceeded ? TEXT("true") : TEXT("false"));
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
	case ETCGEffectStepType::BoostAllOwnUnitsThisRound:
	{
		bStepSucceeded = GrantTemporaryAttackToUnits(ChainEntry.ControllerPlayerIndex, Step.TargetFilter, Step.Value);
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step BoostAllOwnUnitsThisRound Player=%d Amount=%d Success=%s"), ChainEntry.ControllerPlayerIndex, Step.Value, bStepSucceeded ? TEXT("true") : TEXT("false"));
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
