#include "GameState/TCG_GameState.h"
#include "GameState/TCG_EffectResolver.h"

namespace
{
	static bool DiscardSourcePreventMaterialLossByCardEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry);

static bool TryHandMaterialLossReplacementForCardEffect(
ATCG_GameState* GameState,
const FGuid& TargetTopCardInstanceId);

static bool AttackDetachTwoStealOneMaterial(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry);

static int32 ResolveOpponentDrawsByCardEffectResponses(
ATCG_GameState* GameState,
int32 DrawingPlayerIndex,
const FGuid& DrawEffectSourceCardId);

constexpr bool bLogEffectResolution = false;
	constexpr bool bAutoSubmitDebugDiscardChoice = true;
	constexpr bool bAutoSubmitDebugGraveyardToDeckChoice = true;
	constexpr bool bAutoSubmitDebugHandToTopDeckChoice = true;
	constexpr bool bAutoSubmitDebugPlayToEmptyZoneChoice = true;
	constexpr bool bAutoSubmitDebugAttachSourceToUnitChoice = true;
	constexpr bool bAutoSubmitDebugPlayGraveyardCardToEmptyZoneChoice = true;
	constexpr bool bAutoSubmitDebugDestroyedRecoveryChoice = true;
	constexpr bool bAutoSubmitDebugSwapOpponentUnitZonesChoice = true;

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
            case ETCGEffectStepType::PlayGraveyardCardOnUnit: return TEXT("PlayGraveyardCardOnUnit");
            case ETCGEffectStepType::CannotBeDestroyedByCardEffects: return TEXT("CannotBeDestroyedByCardEffects");
		case ETCGEffectStepType::MoveGraveyardCardsToHandAndTopDeck: return TEXT("MoveGraveyardCardsToHandAndTopDeck");
		case ETCGEffectStepType::RemoveMaterialFromTargetUnit: return TEXT("RemoveMaterialFromTargetUnit");
		case ETCGEffectStepType::AttackMillTwoWaterBounceBattlingUnit: return TEXT("AttackMillTwoWaterBounceBattlingUnit");
		case ETCGEffectStepType::DiscardSourceDetachUpToTwoMaterialsFromTarget: return TEXT("DiscardSourceDetachUpToTwoMaterialsFromTarget");
		case ETCGEffectStepType::DestroyTargetUnitByCardEffect: return TEXT("DestroyTargetUnitByCardEffect");
		case ETCGEffectStepType::DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials: return TEXT("DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials");
		case ETCGEffectStepType::BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw: return TEXT("BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw");
		case ETCGEffectStepType::SwapTwoOpponentUnitsZones: return TEXT("SwapTwoOpponentUnitsZones");
		case ETCGEffectStepType::SwapUnitZones: return TEXT("SwapUnitZones");
		case ETCGEffectStepType::CheckMaterialCount: return TEXT("CheckMaterialCount");
		case ETCGEffectStepType::DiscardRandomCards: return TEXT("DiscardRandomCards");
		case ETCGEffectStepType::PlayCardToEmptyZone: return TEXT("PlayCardToEmptyZone");
		case ETCGEffectStepType::PlayHandCardToEmptyZone: return TEXT("PlayHandCardToEmptyZone");
case ETCGEffectStepType::MoveDeckCardToHand: return TEXT("MoveDeckCardToHand");
		case ETCGEffectStepType::PlayHandCardOnUnit: return TEXT("PlayHandCardOnUnit");
		case ETCGEffectStepType::DiscardSourcePreventMaterialLossByCardEffect: return TEXT("DiscardSourcePreventMaterialLossByCardEffect");
		case ETCGEffectStepType::DetachMaterials: return TEXT("DetachMaterials");
		case ETCGEffectStepType::StealMaterials: return TEXT("StealMaterials");
		case ETCGEffectStepType::DiscardSource: return TEXT("DiscardSource");
		case ETCGEffectStepType::DestroyUnit: return TEXT("DestroyUnit");
		case ETCGEffectStepType::ReturnUnitToHand: return TEXT("ReturnUnitToHand");
		case ETCGEffectStepType::BanishSource: return TEXT("BanishSource");
		case ETCGEffectStepType::DrawCardsForBothPlayers: return TEXT("DrawCardsForBothPlayers");
		case ETCGEffectStepType::ReturnUnitToBottomDeck: return TEXT("ReturnUnitToBottomDeck");
case ETCGEffectStepType::AttackDetachTwoStealOneMaterial: return TEXT("AttackDetachTwoStealOneMaterial");
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
		
if (TryHandMaterialLossReplacementForCardEffect(GameState, TargetCardId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Discard source detach prevented Target=%s"),
*TargetDefinitionId.ToString());

return true;
}

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

	
	static bool ReturnUnitToHandGeneric(
		ATCG_GameState* GameState,
		const FTCGEffectChainEntry& ChainEntry,
		const FTCGEffectStep& Step)
	{
		if (!GameState)
		{
			return false;
		}

		const FGuid TargetCardInstanceId =
			Step.TargetMode == ETCGEffectTargetMode::SourceCard
				? ChainEntry.SourceCardInstanceId
				: ChainEntry.TargetCardInstanceId;

		const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);
		if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: ReturnUnitToHand failed Target=%s Reason=TargetInvalid"),
				TargetCard ? *TargetCard->CardDefinitionId.ToString() : TEXT("None"));
			return false;
		}

		const FName TargetDefinitionId = TargetCard->CardDefinitionId;

		int32 MaterialCount = 0;
		const bool bReturnedUnit = ReturnUnitToHandMaterialsToGraveyard(GameState, TargetCardInstanceId, MaterialCount);

		const int32 DrawThreshold = Step.Value;
		const int32 DrawnCount =
			bReturnedUnit && DrawThreshold > 0 && MaterialCount >= DrawThreshold
				? GameState->DrawCards(ChainEntry.ControllerPlayerIndex, 1)
				: 0;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: ReturnUnitToHand Target=%s Returned=%s Materials=%d DrawThreshold=%d Drawn=%d"),
			*TargetDefinitionId.ToString(),
			bReturnedUnit ? TEXT("true") : TEXT("false"),
			MaterialCount,
			DrawThreshold,
			DrawnCount);

		return bReturnedUnit;
	}

static bool ReturnUnitToBottomDeckGeneric(
	ATCG_GameState* GameState,
	const FTCGEffectChainEntry& ChainEntry,
	const FTCGEffectStep& Step)
{
	if (!GameState)
	{
		return false;
	}

	const FGuid TargetCardInstanceId =
		Step.TargetMode == ETCGEffectTargetMode::SourceCard
			? ChainEntry.SourceCardInstanceId
			: ChainEntry.TargetCardInstanceId;

	const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);
	if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: ReturnUnitToBottomDeck failed Target=%s Reason=TargetInvalid"),
			TargetCard ? *TargetCard->CardDefinitionId.ToString() : TEXT("None"));
		return false;
	}

	const FName TargetDefinitionId = TargetCard->CardDefinitionId;
	const bool bReturned = GameState->ReturnUnitStackToBottomDeckMaterialsToGraveyard(TargetCardInstanceId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: ReturnUnitToBottomDeck Target=%s Returned=%s"),
		*TargetDefinitionId.ToString(),
		bReturned ? TEXT("true") : TEXT("false"));

	return bReturned;
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

	static bool BanishSourceGeneric(
		ATCG_GameState* GameState,
		const FTCGEffectChainEntry& ChainEntry)
	{
		if (!GameState)
		{
			return false;
		}

		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
		if (!SourceCard)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: BanishSource failed Source=%s Reason=SourceMissing"),
				ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());
			return false;
		}

		const FName SourceDefinitionId = SourceCard->CardDefinitionId;

		if (SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: BanishSource failed Source=%s Reason=WrongController"),
				*SourceDefinitionId.ToString());
			return false;
		}

		const bool bSourceInValidLocation =
			SourceCard->Location == ETCGCardLocation::Hand
			|| SourceCard->Location == ETCGCardLocation::Graveyard
			|| SourceCard->Location == ETCGCardLocation::Board;

		if (!bSourceInValidLocation)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: BanishSource failed Source=%s Reason=InvalidLocation"),
				*SourceDefinitionId.ToString());
			return false;
		}

		const bool bBanished = GameState->MoveCardToLocation(ChainEntry.SourceCardInstanceId, ETCGCardLocation::Banish);

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: BanishSource Source=%s Banished=%s"),
			*SourceDefinitionId.ToString(),
			bBanished ? TEXT("true") : TEXT("false"));

		return bBanished;
	}

	static bool DrawCardsForBothPlayersGeneric(
		ATCG_GameState* GameState,
		const FTCGEffectStep& Step,
		const FTCGEffectChainEntry* ChainEntry = nullptr)
	{
		if (!GameState)
		{
			return false;
		}

		const int32 DrawCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);
		const int32 Player0Drawn = GameState->DrawCards(0, DrawCount);
		const int32 Player1Drawn = GameState->DrawCards(1, DrawCount);

		if (ChainEntry)
		{
			if (Player0Drawn > 0)
			{
				ResolveOpponentDrawsByCardEffectResponses(GameState, 0, ChainEntry->SourceCardInstanceId);
			}

			if (Player1Drawn > 0)
			{
				ResolveOpponentDrawsByCardEffectResponses(GameState, 1, ChainEntry->SourceCardInstanceId);
			}
		}

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: DrawCardsForBothPlayers Requested=%d Drawn=%d/%d"),
			DrawCount,
			Player0Drawn,
			Player1Drawn);

		return Player0Drawn == DrawCount && Player1Drawn == DrawCount;
	}

	static bool BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw(
		ATCG_GameState* GameState,
		const FTCGEffectChainEntry& ChainEntry)
	{
		if (!GameState || !GameState->IsValidPlayerIndex(ChainEntry.ControllerPlayerIndex))
		{
			return false;
		}

		const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
		if (!SourceCard
			|| SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex
			|| SourceCard->Location != ETCGCardLocation::Graveyard)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Graveyard recycle both draw failed Source=%s Reason=SourceNotInGraveyard"),
				ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());

			return false;
		}

		TArray<FGuid> GraveyardCardIds;
		for (const FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex) continue;
			if (Card.Location != ETCGCardLocation::Graveyard) continue;
			if (Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;

			GraveyardCardIds.Add(Card.CardInstanceId);
		}

		if (GraveyardCardIds.Num() < 2)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Graveyard recycle both draw failed Source=%s Reason=NotEnoughGraveyardCards Count=%d"),
				*SourceCard->CardDefinitionId.ToString(),
				GraveyardCardIds.Num());

			return false;
		}

		const FName SourceDefinitionId = SourceCard->CardDefinitionId;

		if (!GameState->MoveCardToLocation(ChainEntry.SourceCardInstanceId, ETCGCardLocation::Banish))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Graveyard recycle both draw failed Source=%s Reason=BanishFailed"),
				*SourceDefinitionId.ToString());

			return false;
		}

		int32 ReturnedCount = 0;
		for (int32 Index = 0; Index < GraveyardCardIds.Num() && ReturnedCount < 2; ++Index)
		{
			if (GameState->MoveCardToBottomOfDeck(GraveyardCardIds[Index]))
			{
				ReturnedCount++;
			}
		}

		const int32 Player0Drawn = GameState->DrawCards(0, 1);
		const int32 Player1Drawn = GameState->DrawCards(1, 1);

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Graveyard recycle both draw Source=%s Banished=true Returned=%d Drawn=%d/%d"),
			*SourceDefinitionId.ToString(),
			ReturnedCount,
			Player0Drawn,
			Player1Drawn);

		return ReturnedCount == 2 && Player0Drawn == 1 && Player1Drawn == 1;
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
	}static bool DiscardSourcePreventMaterialLossByCardEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry)
{
if (!GameState)
{
return false;
}

const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
const FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);

if (!SourceCard
|| SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex
|| SourceCard->Location != ETCGCardLocation::Hand)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Material loss save failed Source=%s Reason=SourceNotInHand"),
ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());

return false;
}

if (!TargetCard
|| TargetCard->Location != ETCGCardLocation::Board
|| TargetCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Material loss save failed Source=%s Reason=TargetInvalid"),
*SourceCard->CardDefinitionId.ToString());

return false;
}

const FName SourceDefinitionId = SourceCard->CardDefinitionId;
const FName TargetDefinitionId = TargetCard->CardDefinitionId;

const bool bDiscardedSource = GameState->MoveCardToLocation(
ChainEntry.SourceCardInstanceId,
ETCGCardLocation::Graveyard);

if (!bDiscardedSource)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Material loss save failed Source=%s Reason=DiscardFailed"),
*SourceDefinitionId.ToString());

return false;
}

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Material loss save Source=%s Target=%s Discarded=true Prevented=true"),
*SourceDefinitionId.ToString(),
*TargetDefinitionId.ToString());

return true;
}

static bool TryHandMaterialLossReplacementForCardEffect(
ATCG_GameState* GameState,
const FGuid& TargetTopCardInstanceId)
{
if (!GameState)
{
return false;
}

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
if (!GameState->DoesCardEffectMatchTrigger(
EffectRef,
ETCGEffectTrigger::OnYourUnitWouldLoseMaterialByCardEffect))
{
continue;
}

if (!EffectRefHasStep(
EffectRef,
ETCGEffectStepType::DiscardSourcePreventMaterialLossByCardEffect))
{
continue;
}

FTCGEffectChainEntry ReplacementEntry;
ReplacementEntry.ChainIndex = 1;
ReplacementEntry.SourceCardInstanceId = HandCard.CardInstanceId;
ReplacementEntry.TargetCardInstanceId = TargetTopCardInstanceId;
ReplacementEntry.SourceCardDefinitionId = HandCard.CardDefinitionId;
ReplacementEntry.Trigger = ETCGEffectTrigger::OnYourUnitWouldLoseMaterialByCardEffect;
ReplacementEntry.EffectRef = EffectRef;
ReplacementEntry.ControllerPlayerIndex = HandCard.OwnerPlayerIndex;
ReplacementEntry.bRequiresSourceOnBoard = false;
ReplacementEntry.bRequiresTargetOnBoard = true;

return DiscardSourcePreventMaterialLossByCardEffect(GameState, ReplacementEntry);
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

				const bool bHasLegacySaveStep =
					EffectRefHasStep(EffectRef, ETCGEffectStepType::DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials);

				const bool bHasGenericSaveSteps =
					EffectRefHasStep(EffectRef, ETCGEffectStepType::DiscardSource)
					&& EffectRefHasStep(EffectRef, ETCGEffectStepType::ReturnUnitToHand);

				if (!bHasLegacySaveStep && !bHasGenericSaveSteps)
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

				if (bHasLegacySaveStep)
				{
					return DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials(GameState, ReplacementEntry);
				}

				const FTCGCardInstance* ReplacementSourceCard = GameState->FindCardInstance(ReplacementEntry.SourceCardInstanceId);
				if (!ReplacementSourceCard
					|| ReplacementSourceCard->OwnerPlayerIndex != ReplacementEntry.ControllerPlayerIndex
					|| ReplacementSourceCard->Location != ETCGCardLocation::Hand)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("TCG Effect: DiscardSource failed Source=%s Reason=SourceNotInHand"),
						ReplacementEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ReplacementEntry.SourceCardDefinitionId.ToString());
					return false;
				}

				const FName ReplacementSourceDefinitionId = ReplacementSourceCard->CardDefinitionId;
				const bool bDiscardedSource = GameState->MoveCardToLocation(ReplacementEntry.SourceCardInstanceId, ETCGCardLocation::Graveyard);

				UE_LOG(LogTemp, Warning,
					TEXT("TCG Effect: DiscardSource Source=%s Discarded=%s"),
					*ReplacementSourceDefinitionId.ToString(),
					bDiscardedSource ? TEXT("true") : TEXT("false"));

				if (!bDiscardedSource)
				{
					return false;
				}

				FTCGEffectStep ReturnStep;
				ReturnStep.StepType = ETCGEffectStepType::ReturnUnitToHand;
				ReturnStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
				ReturnStep.Value = 2;

				return ReturnUnitToHandGeneric(GameState, ReplacementEntry, ReturnStep);
			}
		}

		return false;
	}

	
static FTCGCardInstance* FindBottomMaterialUnderUnitForEffect(
ATCG_GameState* GameState,
const FGuid& TopCardInstanceId)
{
if (!GameState)
{
return nullptr;
}

FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardInstanceId);
if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid())
{
return nullptr;
}

FTCGCardInstance* BottomMaterial = nullptr;
for (FTCGCardInstance& Card : GameState->MatchCards)
{
if (Card.Location != ETCGCardLocation::Board) continue;
if (Card.StackId != TopCard->StackId) continue;
if (Card.CardInstanceId == TopCard->CardInstanceId) continue;
if (Card.StackIndex >= TopCard->StackIndex) continue;

if (!BottomMaterial || Card.StackIndex < BottomMaterial->StackIndex)
{
BottomMaterial = &Card;
}
}

return BottomMaterial;
}

static int32 CountMaterialsUnderUnitForEffect(
ATCG_GameState* GameState,
const FGuid& TopCardInstanceId)
{
if (!GameState)
{
return 0;
}

const FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardInstanceId);
if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid())
{
return 0;
}

int32 Count = 0;
for (const FTCGCardInstance& Card : GameState->MatchCards)
{
if (Card.Location != ETCGCardLocation::Board) continue;
if (Card.StackId != TopCard->StackId) continue;
if (Card.CardInstanceId == TopCard->CardInstanceId) continue;
if (Card.StackIndex >= TopCard->StackIndex) continue;

Count++;
}

return Count;
}

static bool DiscardSourceGeneric(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry)
{
if (!GameState)
{
return false;
}

const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
if (!SourceCard)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DiscardSource failed Source=%s Reason=SourceMissing"),
ChainEntry.SourceCardDefinitionId.IsNone() ? TEXT("None") : *ChainEntry.SourceCardDefinitionId.ToString());

return false;
}

const FName SourceDefinitionId = SourceCard->CardDefinitionId;

if (SourceCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DiscardSource failed Source=%s Reason=WrongController"),
*SourceDefinitionId.ToString());

return false;
}

if (SourceCard->Location != ETCGCardLocation::Hand)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DiscardSource failed Source=%s Reason=SourceNotInHand"),
*SourceDefinitionId.ToString());

return false;
}

const bool bDiscarded = GameState->MoveCardToLocation(ChainEntry.SourceCardInstanceId, ETCGCardLocation::Graveyard);

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DiscardSource Source=%s Discarded=%s"),
*SourceDefinitionId.ToString(),
bDiscarded ? TEXT("true") : TEXT("false"));

return bDiscarded;
}

static FGuid ResolveUnitTargetForMaterialStep(
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (Step.TargetMode == ETCGEffectTargetMode::SourceCard || Step.TargetMode == ETCGEffectTargetMode::None)
{
return ChainEntry.SourceCardInstanceId;
}

return ChainEntry.TargetCardInstanceId;
}

static bool DetachMaterialsGeneric(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

const FGuid TargetCardInstanceId = ResolveUnitTargetForMaterialStep(ChainEntry, Step);
const int32 DetachCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);

const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);
if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board || !TargetCard->StackId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DetachMaterials failed TargetMode=%s Reason=TargetInvalid"),
GetTCGEffectTargetModeDebugName(Step.TargetMode));

return false;
}

const FName TargetDefinitionId = TargetCard->CardDefinitionId;

const int32 AvailableMaterialCount = CountMaterialsUnderUnitForEffect(GameState, TargetCardInstanceId);
if (AvailableMaterialCount <= 0)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DetachMaterials failed Target=%s Requested=%d Reason=NoMaterials"),
*TargetDefinitionId.ToString(),
DetachCount);

return false;
}

if (!Step.bAllowPartialSuccess && AvailableMaterialCount < DetachCount)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DetachMaterials failed Target=%s Requested=%d Available=%d Reason=NotEnoughMaterials"),
*TargetDefinitionId.ToString(),
DetachCount,
AvailableMaterialCount);

return false;
}

const int32 ActualDetachCount = Step.bAllowPartialSuccess
? FMath::Min(DetachCount, AvailableMaterialCount)
: DetachCount;

if (TryHandMaterialLossReplacementForCardEffect(GameState, TargetCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DetachMaterials prevented Target=%s Requested=%d Actual=%d"),
*TargetDefinitionId.ToString(),
DetachCount,
ActualDetachCount);

return false;
}

int32 DetachedCount = 0;
for (int32 Index = 0; Index < ActualDetachCount; ++Index)
{
if (GameState->MoveBottomOverlayToGraveyard(TargetCardInstanceId))
{
DetachedCount++;
}
}

const bool bStepSuccess = Step.bAllowPartialSuccess
? DetachedCount > 0
: DetachedCount == DetachCount;

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DetachMaterials Target=%s Requested=%d Actual=%d Detached=%d PartialAllowed=%s Success=%s"),
*TargetDefinitionId.ToString(),
DetachCount,
ActualDetachCount,
DetachedCount,
Step.bAllowPartialSuccess ? TEXT("true") : TEXT("false"),
bStepSuccess ? TEXT("true") : TEXT("false"));

return bStepSuccess;
}

static bool StealMaterialsGeneric(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

const FGuid FromUnitCardInstanceId = ResolveUnitTargetForMaterialStep(ChainEntry, Step);
const FGuid ToUnitCardInstanceId = ChainEntry.SourceCardInstanceId;
const int32 StealCount = FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value);

FTCGCardInstance* FromUnitCard = GameState->FindCardInstance(FromUnitCardInstanceId);
FTCGCardInstance* ToUnitCard = GameState->FindCardInstance(ToUnitCardInstanceId);

if (!FromUnitCard || FromUnitCard->Location != ETCGCardLocation::Board || !FromUnitCard->StackId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: StealMaterials failed FromMode=%s Reason=FromInvalid"),
GetTCGEffectTargetModeDebugName(Step.TargetMode));

return false;
}

if (!ToUnitCard || ToUnitCard->Location != ETCGCardLocation::Board || !ToUnitCard->StackId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: StealMaterials failed Reason=ToInvalid"));

return false;
}

const FName FromDefinitionId = FromUnitCard->CardDefinitionId;
const FName ToDefinitionId = ToUnitCard->CardDefinitionId;

if (!FindBottomMaterialUnderUnitForEffect(GameState, FromUnitCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: StealMaterials From=%s To=%s Requested=%d FromHasNoMaterial=true"),
*FromDefinitionId.ToString(),
*ToDefinitionId.ToString(),
StealCount);

return true;
}

if (TryHandMaterialLossReplacementForCardEffect(GameState, FromUnitCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: StealMaterials From=%s To=%s Requested=%d MaterialLossPrevented=true"),
*FromDefinitionId.ToString(),
*ToDefinitionId.ToString(),
StealCount);

return true;
}

int32 StolenCount = 0;
for (int32 Index = 0; Index < StealCount; ++Index)
{
FTCGCardInstance* MaterialToSteal = FindBottomMaterialUnderUnitForEffect(GameState, FromUnitCardInstanceId);
ToUnitCard = GameState->FindCardInstance(ToUnitCardInstanceId);

if (!MaterialToSteal || !ToUnitCard || ToUnitCard->Location != ETCGCardLocation::Board || !ToUnitCard->StackId.IsValid())
{
break;
}

MaterialToSteal->OwnerPlayerIndex = ToUnitCard->OwnerPlayerIndex;
MaterialToSteal->Location = ETCGCardLocation::Board;
MaterialToSteal->ZoneId = ToUnitCard->ZoneId;
MaterialToSteal->StackId = ToUnitCard->StackId;
MaterialToSteal->StackIndex = ToUnitCard->StackIndex;

ToUnitCard->StackIndex++;
StolenCount++;
}

const bool bStoleAny = StolenCount > 0;

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: StealMaterials From=%s To=%s Requested=%d Stolen=%d Success=%s"),
*FromDefinitionId.ToString(),
*ToDefinitionId.ToString(),
StealCount,
StolenCount,
bStoleAny ? TEXT("true") : TEXT("false"));

return bStoleAny;
}

static bool AttackDetachTwoStealOneMaterial(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry)
{
if (!GameState)
{
return false;
}

FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);

if (!SourceCard || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid())
{
return false;
}

if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board || !TargetCard->StackId.IsValid())
{
return false;
}

const FName SourceDefinitionId = SourceCard->CardDefinitionId;
const FName TargetDefinitionId = TargetCard->CardDefinitionId;

if (CountMaterialsUnderUnitForEffect(GameState, ChainEntry.SourceCardInstanceId) < 2)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal failed Source=%s Reason=SourceNeedsTwoMaterials"),
*SourceDefinitionId.ToString());

return false;
}

if (TryHandMaterialLossReplacementForCardEffect(GameState, ChainEntry.SourceCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal source material loss prevented Source=%s"),
*SourceDefinitionId.ToString());

return false;
}

int32 SourceDetachedCount = 0;
for (int32 Index = 0; Index < 2; ++Index)
{
if (GameState->MoveBottomOverlayToGraveyard(ChainEntry.SourceCardInstanceId))
{
SourceDetachedCount++;
}
}

if (SourceDetachedCount < 2)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal failed Source=%s Reason=DetachFailed Detached=%d"),
*SourceDefinitionId.ToString(),
SourceDetachedCount);

return false;
}

if (!FindBottomMaterialUnderUnitForEffect(GameState, ChainEntry.TargetCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal Source=%s Target=%s Detached=2 TargetHasNoMaterial=true"),
*SourceDefinitionId.ToString(),
*TargetDefinitionId.ToString());

return true;
}

if (TryHandMaterialLossReplacementForCardEffect(GameState, ChainEntry.TargetCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal Source=%s Target=%s Detached=2 TargetMaterialLossPrevented=true"),
*SourceDefinitionId.ToString(),
*TargetDefinitionId.ToString());

return true;
}

FTCGCardInstance* MaterialToSteal = FindBottomMaterialUnderUnitForEffect(GameState, ChainEntry.TargetCardInstanceId);
SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);

if (!MaterialToSteal || !SourceCard || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid())
{
return false;
}

const FName StolenMaterialDefinitionId = MaterialToSteal->CardDefinitionId;

MaterialToSteal->OwnerPlayerIndex = SourceCard->OwnerPlayerIndex;
MaterialToSteal->Location = ETCGCardLocation::Board;
MaterialToSteal->ZoneId = SourceCard->ZoneId;
MaterialToSteal->StackId = SourceCard->StackId;
MaterialToSteal->StackIndex = SourceCard->StackIndex;

SourceCard->StackIndex++;

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Attack detach steal Source=%s Target=%s Detached=2 Stolen=%s"),
*SourceDefinitionId.ToString(),
*TargetDefinitionId.ToString(),
*StolenMaterialDefinitionId.ToString());

return true;
}

static int32 BuildYourUnitDestroyedByCardEffectHandResponseChainForEffect(
ATCG_GameState* GameState,
int32 DestroyedUnitOwnerPlayerIndex,
const FGuid& DestroyedTopCardInstanceId,
TArray<FTCGEffectChainEntry>& OutChain)
{
if (!GameState || !GameState->IsValidPlayerIndex(DestroyedUnitOwnerPlayerIndex))
{
return 0;
}

TArray<FTCGCardInstance> HandCards;
GameState->GetCardsInHand(DestroyedUnitOwnerPlayerIndex, HandCards);

for (const FTCGCardInstance& HandCard : HandCards)
{
TArray<FTCGCardEffectRef> EffectRefs;
GameState->GetPrintedEffectRefsForCard(HandCard, EffectRefs);

for (const FTCGCardEffectRef& EffectRef : EffectRefs)
{
if (!GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnYourUnitDestroyed))
{
continue;
}

GameState->AddCardEffectRefToChain(
OutChain,
HandCard.CardInstanceId,
DestroyedTopCardInstanceId,
EffectRef);
}
}

return OutChain.Num();
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

if (GameState->DoesUnitHaveInheritedCannotBeDestroyedByCardEffects(TargetCardInstanceId))
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DestroyTargetUnitByCardEffect prevented Target=%s Reason=InheritedCannotBeDestroyedByCardEffects"),
*TargetCard->CardDefinitionId.ToString());

return true;
}

if (TryHandSaveReplacementForCardEffect(GameState, TargetCardInstanceId))
{
return true;
}

const int32 DestroyedUnitOwnerPlayerIndex = TargetCard->OwnerPlayerIndex;
const FGuid DestroyedStackId = TargetCard->StackId;

TArray<FTCGEffectChainEntry> DestroyedResponseChain;
GameState->BuildYourUnitDestroyedGraveyardResponseChain(
DestroyedUnitOwnerPlayerIndex,
TargetCardInstanceId,
DestroyedResponseChain);

BuildYourUnitDestroyedByCardEffectHandResponseChainForEffect(
GameState,
DestroyedUnitOwnerPlayerIndex,
TargetCardInstanceId,
DestroyedResponseChain);

TArray<FTCGEffectChainEntry> OpponentCardEffectDestroyedResponseChain;
GameState->BuildYourUnitDestroyedByOpponentCardEffectGraveyardResponseChain(
DestroyedUnitOwnerPlayerIndex,
TargetCardInstanceId,
ChainEntry.SourceCardInstanceId,
OpponentCardEffectDestroyedResponseChain);

DestroyedResponseChain.Append(OpponentCardEffectDestroyedResponseChain);

const bool bDestroyed = GameState->MoveStackToLocation(DestroyedStackId, ETCGCardLocation::Graveyard);
if (bDestroyed && DestroyedResponseChain.Num() > 0)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Card effect destruction graveyard responses Player=%d Count=%d"),
DestroyedUnitOwnerPlayerIndex,
DestroyedResponseChain.Num());

GameState->ResolveEffectChain(DestroyedResponseChain);
}

return bDestroyed;
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

	FTCGEffectChainEntry WorkingChainEntry = ChainEntry;

	bool bResolvedAnyStep = false;
	bool bPreviousMeaningfulStepSucceeded = true;
	if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Resolve modular chain %d Source=%s Steps=%d"), WorkingChainEntry.ChainIndex, *WorkingChainEntry.SourceCardDefinitionId.ToString(), WorkingChainEntry.EffectRef.Steps.Num());
	for (const FTCGEffectStep& Step : WorkingChainEntry.EffectRef.Steps)
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
		const bool bStepSucceeded = ResolveEffectStep(WorkingChainEntry, Step, bPreviousMeaningfulStepSucceeded);
		bPreviousMeaningfulStepSucceeded = bStepSucceeded;
		bResolvedAnyStep |= bStepSucceeded;
	}
	return bResolvedAnyStep;
}


static bool ResolveSwapUnitZonesStep(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
bool bAutoSubmitDebugSwapChoice)
{
if (!GameState)
{
return false;
}

const bool bChoiceStarted = GameState->BeginPendingSwapOpponentUnitZonesChoice(
ChainEntry.ControllerPlayerIndex,
ChainEntry);

bool bAutoSubmittedChoice = false;

if (bChoiceStarted && bAutoSubmitDebugSwapChoice)
{
TArray<FGuid> ChoiceOptions;
GameState->GetPendingSwapOpponentUnitZonesChoiceOptions(ChoiceOptions);

if (ChoiceOptions.Num() >= 2)
{
bAutoSubmittedChoice = GameState->SubmitPendingSwapOpponentUnitZonesChoice(
ChainEntry.ControllerPlayerIndex,
ChoiceOptions[0],
ChoiceOptions[1]);
}
}

return bAutoSubmittedChoice;
}

static int32 DiscardRandomCardsFromPlayerHandByEffect(
ATCG_GameState* GameState,
int32 PlayerIndex,
int32 DiscardCount)
{
if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex) || DiscardCount <= 0)
{
return 0;
}

TArray<FTCGCardInstance> HandCards;
GameState->GetCardsInHand(PlayerIndex, HandCards);

int32 DiscardedCount = 0;
while (HandCards.Num() > 0 && DiscardedCount < DiscardCount)
{
const int32 RandomIndex = FMath::RandRange(0, HandCards.Num() - 1);
const FGuid CardId = HandCards[RandomIndex].CardInstanceId;
const FName CardDefinitionId = HandCards[RandomIndex].CardDefinitionId;

if (GameState->MoveCardToLocation(CardId, ETCGCardLocation::Graveyard))
{
DiscardedCount++;

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: DiscardRandomCards Player=%d Card=%s"),
PlayerIndex,
*CardDefinitionId.ToString());
}

HandCards.RemoveAtSwap(RandomIndex);
}

return DiscardedCount;
}

namespace
{
static int32 ResolveOpponentDrawsByCardEffectResponses(
ATCG_GameState* GameState,
int32 DrawingPlayerIndex,
const FGuid& DrawEffectSourceCardId)
{
if (!GameState || !GameState->IsValidPlayerIndex(DrawingPlayerIndex))
{
return 0;
}

const int32 RespondingPlayerIndex = 1 - DrawingPlayerIndex;

TArray<FTCGCardInstance> HandCards;
GameState->GetCardsInHand(RespondingPlayerIndex, HandCards);

TArray<FTCGEffectChainEntry> ResponseChain;
for (const FTCGCardInstance& HandCard : HandCards)
{
TArray<FTCGCardEffectRef> EffectRefs;
GameState->GetPrintedEffectRefsForCard(HandCard, EffectRefs);

for (const FTCGCardEffectRef& EffectRef : EffectRefs)
{
if (GameState->DoesCardEffectMatchTrigger(
EffectRef,
ETCGEffectTrigger::OnOpponentDrawsByCardEffect))
{
GameState->AddCardEffectRefToChain(
ResponseChain,
HandCard.CardInstanceId,
DrawEffectSourceCardId.IsValid() ? DrawEffectSourceCardId : HandCard.CardInstanceId,
EffectRef);
}
}
}

if (ResponseChain.Num() <= 0)
{
return 0;
}

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Opponent draw by card effect responses DrawingPlayer=%d RespondingPlayer=%d Count=%d"),
DrawingPlayerIndex,
RespondingPlayerIndex,
ResponseChain.Num());

GameState->ResolveEffectChain(ResponseChain);
return ResponseChain.Num();
}
}


static FGuid ResolveCardTargetForGenericPlayStep(
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (Step.TargetMode == ETCGEffectTargetMode::SourceCard || Step.TargetMode == ETCGEffectTargetMode::None)
{
return ChainEntry.SourceCardInstanceId;
}

return ChainEntry.TargetCardInstanceId;
}

static bool PlayTargetCardToFirstEmptyZoneForEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

const FGuid TargetCardInstanceId = ResolveCardTargetForGenericPlayStep(ChainEntry, Step);
const FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);

if (!TargetCard)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayCardToEmptyZone failed TargetMode=%s Reason=TargetMissing"),
GetTCGEffectTargetModeDebugName(Step.TargetMode));

return false;
}

const FName TargetDefinitionId = TargetCard->CardDefinitionId;

if (TargetCard->OwnerPlayerIndex != ChainEntry.ControllerPlayerIndex)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayCardToEmptyZone failed Target=%s Reason=WrongController"),
*TargetDefinitionId.ToString());

return false;
}

if (TargetCard->Location == ETCGCardLocation::Board)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayCardToEmptyZone failed Target=%s Reason=AlreadyOnBoard"),
*TargetDefinitionId.ToString());

return false;
}

FName ChosenZoneId = NAME_None;
for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
{
const FName CandidateZoneId = ATCG_GameState::GetFieldZoneId(ChainEntry.ControllerPlayerIndex, FieldIndex);

FGuid ExistingStackId;
if (!GameState->FindStackIdInZone(CandidateZoneId, ExistingStackId))
{
ChosenZoneId = CandidateZoneId;
break;
}
}

if (ChosenZoneId.IsNone())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayCardToEmptyZone failed Target=%s Reason=NoEmptyZone"),
*TargetDefinitionId.ToString());

return false;
}

const bool bPlayed = GameState->PlaceCardAsNewStack(TargetCardInstanceId, ChosenZoneId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayCardToEmptyZone Target=%s Zone=%s Played=%s"),
*TargetDefinitionId.ToString(),
*ChosenZoneId.ToString(),
bPlayed ? TEXT("true") : TEXT("false"));

return bPlayed;
}


static bool DoesCardNameMatchFilterForEffect(
const FTCGCardInstance& Card,
const FTCGEffectTargetFilter& Filter)
{
if (Filter.NameContains.IsEmpty())
{
return true;
}

return Card.CardDefinitionId.ToString().Contains(Filter.NameContains, ESearchCase::IgnoreCase);
}

static bool DoesCardMatchGenericEffectFilter(
const ATCG_GameState* GameState,
const FTCGCardInstance& Card,
const FTCGEffectTargetFilter& Filter,
int32 ControllerPlayerIndex,
const FGuid& SourceCardInstanceId)
{
if (!GameState)
{
return false;
}

const int32 RequiredOwner =
Filter.OwnerMode == ETCGEffectTargetMode::Opponent
? 1 - ControllerPlayerIndex
: ControllerPlayerIndex;

if (Card.OwnerPlayerIndex != RequiredOwner) return false;
if (Card.Location != Filter.RequiredLocation) return false;
if (Filter.bRequireElement && Card.Element != Filter.RequiredElement) return false;
if (Filter.bExcludeSourceCard && Card.CardInstanceId == SourceCardInstanceId) return false;
if (!DoesCardNameMatchFilterForEffect(Card, Filter)) return false;

if (Filter.bRequireTopCard && Card.Location == ETCGCardLocation::Board)
{
const FTCGCardInstance* TopCard = GameState->FindTopCardInStack(Card.StackId);
if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId)
{
return false;
}
}

return true;
}

static bool AttachSourceToFirstFilteredUnitMaterialForEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
if (!SourceCard)
{
return false;
}

FTCGCardInstance* TargetTopCard = nullptr;

if (Step.TargetMode == ETCGEffectTargetMode::TriggerTarget)
{
TargetTopCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);
}
else
{
for (FTCGCardInstance& Card : GameState->MatchCards)
{
if (!DoesCardMatchGenericEffectFilter(
GameState,
Card,
Step.TargetFilter,
ChainEntry.ControllerPlayerIndex,
ChainEntry.SourceCardInstanceId))
{
continue;
}

if (Card.CardInstanceId == ChainEntry.SourceCardInstanceId)
{
continue;
}

TargetTopCard = &Card;
break;
}
}

if (!TargetTopCard || TargetTopCard->Location != ETCGCardLocation::Board || !TargetTopCard->StackId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: AttachSourceToUnitMaterial failed Source=%s Reason=NoFilteredTarget"),
SourceCard ? *SourceCard->CardDefinitionId.ToString() : TEXT("None"));

return false;
}

if (ChainEntry.Trigger == ETCGEffectTrigger::OnYourUnitPlayed
&& Step.TargetMode == ETCGEffectTargetMode::TriggerTarget
&& SourceCard->Location != ETCGCardLocation::Hand)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: AttachSourceToUnitMaterial failed Source=%s Reason=SourceNotInHand"),
*SourceCard->CardDefinitionId.ToString());

return false;
}

const FName SourceDefinitionId = SourceCard->CardDefinitionId;
const FName TargetDefinitionId = TargetTopCard->CardDefinitionId;

SourceCard->OwnerPlayerIndex = TargetTopCard->OwnerPlayerIndex;
SourceCard->Location = ETCGCardLocation::Board;
SourceCard->ZoneId = TargetTopCard->ZoneId;
SourceCard->StackId = TargetTopCard->StackId;
SourceCard->StackIndex = TargetTopCard->StackIndex;

TargetTopCard->StackIndex++;

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: AttachSourceToUnitMaterial Source=%s Target=%s Attached=true"),
*SourceDefinitionId.ToString(),
*TargetDefinitionId.ToString());

return true;
}

static bool PlayFirstFilteredHandCardToEmptyZoneForEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

FGuid CardToPlayId;
FName CardToPlayDefinitionId = NAME_None;

for (const FTCGCardInstance& Card : GameState->MatchCards)
{
if (!DoesCardMatchGenericEffectFilter(
GameState,
Card,
Step.TargetFilter,
ChainEntry.ControllerPlayerIndex,
ChainEntry.SourceCardInstanceId))
{
continue;
}

CardToPlayId = Card.CardInstanceId;
CardToPlayDefinitionId = Card.CardDefinitionId;
break;
}

if (!CardToPlayId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayHandCardToEmptyZone failed Player=%d Reason=NoFilteredHandCard"),
ChainEntry.ControllerPlayerIndex);

return false;
}

FName ChosenZoneId = NAME_None;
for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
{
const FName CandidateZoneId = ATCG_GameState::GetFieldZoneId(ChainEntry.ControllerPlayerIndex, FieldIndex);

FGuid ExistingStackId;
if (!GameState->FindStackIdInZone(CandidateZoneId, ExistingStackId))
{
ChosenZoneId = CandidateZoneId;
break;
}
}

if (ChosenZoneId.IsNone())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayHandCardToEmptyZone failed Card=%s Reason=NoEmptyZone"),
*CardToPlayDefinitionId.ToString());

return false;
}

const bool bPlayed = GameState->PlayCardToZone(CardToPlayId, ChosenZoneId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayHandCardToEmptyZone Card=%s Zone=%s Played=%s"),
*CardToPlayDefinitionId.ToString(),
*ChosenZoneId.ToString(),
bPlayed ? TEXT("true") : TEXT("false"));

return bPlayed;
}


static bool PlayFirstFilteredHandCardOnFirstFilteredUnitForEffect(
	ATCG_GameState* GameState,
	const FTCGEffectChainEntry& ChainEntry,
	const FTCGEffectStep& Step)
{
	if (!GameState)
	{
		return false;
	}

	FGuid CardToPlayId;
	FName CardToPlayDefinitionId = NAME_None;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (!DoesCardMatchGenericEffectFilter(
			GameState,
			Card,
			Step.TargetFilter,
			ChainEntry.ControllerPlayerIndex,
			ChainEntry.SourceCardInstanceId))
		{
			continue;
		}

		if (Card.Location != ETCGCardLocation::Hand)
		{
			continue;
		}

		CardToPlayId = Card.CardInstanceId;
		CardToPlayDefinitionId = Card.CardDefinitionId;
		break;
	}

	if (!CardToPlayId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: PlayHandCardOnUnit failed Player=%d Reason=NoFilteredHandCard"),
			ChainEntry.ControllerPlayerIndex);

		return false;
	}

	FGuid TargetUnitId;
	FName TargetUnitDefinitionId = NAME_None;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.CardInstanceId == CardToPlayId)
		{
			continue;
		}

		if (!DoesCardMatchGenericEffectFilter(
			GameState,
			Card,
			Step.SecondaryTargetFilter,
			ChainEntry.ControllerPlayerIndex,
			ChainEntry.SourceCardInstanceId))
		{
			continue;
		}

		if (Card.Location != ETCGCardLocation::Board || !Card.StackId.IsValid())
		{
			continue;
		}

		TargetUnitId = Card.CardInstanceId;
		TargetUnitDefinitionId = Card.CardDefinitionId;
		break;
	}

	if (!TargetUnitId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: PlayHandCardOnUnit failed Card=%s Reason=NoFilteredUnitTarget"),
			*CardToPlayDefinitionId.ToString());

		return false;
	}

	const bool bPlayed = GameState->PlayCardOnUnitByEffect(CardToPlayId, TargetUnitId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: PlayHandCardOnUnit Card=%s Target=%s Played=%s"),
		*CardToPlayDefinitionId.ToString(),
		*TargetUnitDefinitionId.ToString(),
		bPlayed ? TEXT("true") : TEXT("false"));

	return bPlayed;
}

static bool PlayFirstFilteredGraveyardCardOnUnitForEffect(
ATCG_GameState* GameState,
const FTCGEffectChainEntry& ChainEntry,
const FTCGEffectStep& Step)
{
if (!GameState)
{
return false;
}

FGuid CardToPlayId;
FName CardToPlayDefinitionId = NAME_None;

for (const FTCGCardInstance& Card : GameState->MatchCards)
{
if (!DoesCardMatchGenericEffectFilter(
GameState,
Card,
Step.TargetFilter,
ChainEntry.ControllerPlayerIndex,
ChainEntry.SourceCardInstanceId))
{
continue;
}

if (Card.Location != ETCGCardLocation::Graveyard)
{
continue;
}

CardToPlayId = Card.CardInstanceId;
CardToPlayDefinitionId = Card.CardDefinitionId;
break;
}

if (!CardToPlayId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayGraveyardCardOnUnit failed Player=%d Reason=NoFilteredGraveyardCard"),
ChainEntry.ControllerPlayerIndex);

return false;
}

FGuid TargetUnitId;

if (Step.TargetMode == ETCGEffectTargetMode::SourceCard || Step.TargetMode == ETCGEffectTargetMode::None)
{
TargetUnitId = ChainEntry.SourceCardInstanceId;
}
else if (Step.TargetMode == ETCGEffectTargetMode::TriggerTarget)
{
TargetUnitId = ChainEntry.TargetCardInstanceId;
}
else if (Step.TargetMode == ETCGEffectTargetMode::SelectedTarget)
{
TargetUnitId = ChainEntry.SecondaryTargetCardInstanceId;
}
else
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayGraveyardCardOnUnit failed Card=%s Reason=UnsupportedTargetMode"),
*CardToPlayDefinitionId.ToString());

return false;
}

const FTCGCardInstance* TargetUnit = GameState->FindCardInstance(TargetUnitId);
if (!TargetUnit || TargetUnit->Location != ETCGCardLocation::Board || !TargetUnit->StackId.IsValid())
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayGraveyardCardOnUnit failed Card=%s Reason=InvalidTargetUnit"),
*CardToPlayDefinitionId.ToString());

return false;
}

const FName TargetDefinitionId = TargetUnit->CardDefinitionId;
const bool bPlayed = GameState->PlayCardOnUnitByEffect(CardToPlayId, TargetUnitId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: PlayGraveyardCardOnUnit Card=%s Target=%s Played=%s"),
*CardToPlayDefinitionId.ToString(),
*TargetDefinitionId.ToString(),
bPlayed ? TEXT("true") : TEXT("false"));

return bPlayed;
}

static bool MoveFirstFilteredDeckCardToHandForEffect(
    ATCG_GameState* GameState,
    const FTCGEffectChainEntry& ChainEntry,
    const FTCGEffectStep& Step)
{
    if (!GameState)
    {
        return false;
    }

    const int32 ControllerPlayerIndex = ChainEntry.ControllerPlayerIndex;
    const int32 WantedOwnerPlayerIndex =
        Step.TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent
            ? 1 - ControllerPlayerIndex
            : ControllerPlayerIndex;

    FTCGCardInstance* ChosenCard = nullptr;

    for (FTCGCardInstance& CandidateCard : GameState->MatchCards)
    {
        if (CandidateCard.OwnerPlayerIndex != WantedOwnerPlayerIndex)
        {
            continue;
        }

        if (CandidateCard.Location != ETCGCardLocation::Deck)
        {
            continue;
        }

        if (Step.TargetFilter.RequiredLocation != ETCGCardLocation::None
            && CandidateCard.Location != Step.TargetFilter.RequiredLocation)
        {
            continue;
        }

        if (Step.TargetFilter.bRequireElement
            && CandidateCard.Element != Step.TargetFilter.RequiredElement)
        {
            continue;
        }

        if (Step.TargetFilter.bExcludeSourceCard
            && CandidateCard.CardInstanceId == ChainEntry.SourceCardInstanceId)
        {
            continue;
        }

        if (!Step.TargetFilter.NameContains.IsEmpty())
        {
            const FString CardDefinitionIdString = CandidateCard.CardDefinitionId.ToString();
            FString DisplayNameString;

            if (const UTCG_CardDefinition* CardDefinition = GameState->FindDebugCardDefinitionById(CandidateCard.CardDefinitionId))
            {
                DisplayNameString = CardDefinition->DisplayName.ToString();
            }

            const bool bMatchesDefinitionId =
                CardDefinitionIdString.Contains(Step.TargetFilter.NameContains, ESearchCase::IgnoreCase);

            const bool bMatchesDisplayName =
                DisplayNameString.Contains(Step.TargetFilter.NameContains, ESearchCase::IgnoreCase);

            if (!bMatchesDefinitionId && !bMatchesDisplayName)
            {
                continue;
            }
        }

        if (!ChosenCard || CandidateCard.LocationIndex > ChosenCard->LocationIndex)
        {
            ChosenCard = &CandidateCard;
        }
    }

    if (!ChosenCard)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TCG Effect: MoveDeckCardToHand NoMatchingCard Player=%d NameContains=%s"),
            WantedOwnerPlayerIndex,
            *Step.TargetFilter.NameContains);

        return false;
    }

    const FName MovedCardDefinitionId = ChosenCard->CardDefinitionId;

    ChosenCard->Location = ETCGCardLocation::Hand;
    ChosenCard->LocationIndex = GameState->GetNextLocationIndex(ChosenCard->OwnerPlayerIndex, ETCGCardLocation::Hand);
    ChosenCard->ZoneId = NAME_None;
    ChosenCard->StackId.Invalidate();
    ChosenCard->StackIndex = INDEX_NONE;

    UE_LOG(LogTemp, Warning,
        TEXT("TCG Effect: MoveDeckCardToHand Card=%s Moved=true"),
        *MovedCardDefinitionId.ToString());

    return true;
}

bool ATCG_GameState::ResolveEffectStep(FTCGEffectChainEntry& ChainEntry, const FTCGEffectStep& Step, bool bPreviousStepSucceeded)
{
	bool bStepSucceeded = false;
	switch (Step.StepType)
	{
	case ETCGEffectStepType::PlayHandCardOnUnit:
	{
		bStepSucceeded = PlayFirstFilteredHandCardOnFirstFilteredUnitForEffect(this, ChainEntry, Step);
		break;
	}
    case ETCGEffectStepType::MoveDeckCardToHand:
    {
        bStepSucceeded = MoveFirstFilteredDeckCardToHandForEffect(this, ChainEntry, Step);
        break;
    }

	case ETCGEffectStepType::DrawCards:
	{
		const int32 DrawCount = FMath::Max(0, Step.Value);
		const int32 DrawnCount = DrawCards(ChainEntry.ControllerPlayerIndex, DrawCount);
		bStepSucceeded = DrawnCount == DrawCount && DrawCount > 0;

		if (DrawnCount > 0)
		{
			ResolveOpponentDrawsByCardEffectResponses(
				this,
				ChainEntry.ControllerPlayerIndex,
				ChainEntry.SourceCardInstanceId);
		}

		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DrawCards Player=%d Requested=%d Drawn=%d Success=%s"), ChainEntry.ControllerPlayerIndex, DrawCount, DrawnCount, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
case ETCGEffectStepType::DrawCardsForBothPlayers:
{
bStepSucceeded = DrawCardsForBothPlayersGeneric(this, Step, &ChainEntry);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step DrawCardsForBothPlayers Requested=%d Success=%s"),
FMath::Max(1, Step.Value <= 0 ? 1 : Step.Value),
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
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
	case ETCGEffectStepType::DiscardRandomCards:
	{
		const int32 DiscardCount = FMath::Max(0, Step.Value);
		const int32 TargetPlayerIndex =
			Step.TargetMode == ETCGEffectTargetMode::Opponent
			? 1 - ChainEntry.ControllerPlayerIndex
			: ChainEntry.ControllerPlayerIndex;

		const int32 DiscardedCount = DiscardRandomCardsFromPlayerHandByEffect(
			this,
			TargetPlayerIndex,
			DiscardCount);

		bStepSucceeded = DiscardedCount == DiscardCount && DiscardCount > 0;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Step DiscardRandomCards Controller=%d TargetPlayer=%d Requested=%d Discarded=%d Success=%s"),
			ChainEntry.ControllerPlayerIndex,
			TargetPlayerIndex,
			DiscardCount,
			DiscardedCount,
			bStepSucceeded ? TEXT("true") : TEXT("false"));
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
		ChainEntry.SecondaryTargetCardInstanceId.Invalidate();

		const int32 OwnerPlayerIndex = Step.TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent ? 1 - ChainEntry.ControllerPlayerIndex : ChainEntry.ControllerPlayerIndex;
		for (const FTCGCardInstance& Card : MatchCards)
		{
			if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
			if (Card.Location != Step.TargetFilter.RequiredLocation) continue;
			if (Step.TargetFilter.bRequireElement && Card.Element != Step.TargetFilter.RequiredElement) continue;
			if (Step.TargetFilter.bExcludeSourceCard && Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
			if (!DoesCardNameMatchFilterForEffect(Card, Step.TargetFilter)) continue;
			if (Step.TargetFilter.bRequireTopCard && Card.Location == ETCGCardLocation::Board)
			{
				const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
				if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
			}
			ChainEntry.SecondaryTargetCardInstanceId = Card.CardInstanceId;
			bStepSucceeded = true;
			break;
		}
		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step SelectTarget OwnerMode=%s Location=%d Success=%s"), GetTCGEffectTargetModeDebugName(Step.TargetFilter.OwnerMode), static_cast<int32>(Step.TargetFilter.RequiredLocation), bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::MoveBottomOverlayToGraveyard:
{
const FGuid TargetCardInstanceId =
Step.TargetMode == ETCGEffectTargetMode::SourceCard
? ChainEntry.SourceCardInstanceId
: ChainEntry.TargetCardInstanceId;

if (TryHandMaterialLossReplacementForCardEffect(this, TargetCardInstanceId))
{
bStepSucceeded = true;
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step MoveBottomOverlayToGraveyard TargetMode=%s Prevented=true Success=true"),
GetTCGEffectTargetModeDebugName(Step.TargetMode));
}
break;
}

bStepSucceeded = MoveBottomOverlayToGraveyard(TargetCardInstanceId);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step MoveBottomOverlayToGraveyard TargetMode=%s Success=%s"),
GetTCGEffectTargetModeDebugName(Step.TargetMode),
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
break;
}	case ETCGEffectStepType::PlayHandCardToEmptyZone:
	{
		bStepSucceeded = PlayFirstFilteredHandCardToEmptyZoneForEffect(this, ChainEntry, Step);
		break;
	}
	case ETCGEffectStepType::PlayCardToEmptyZone:
	{
		bStepSucceeded = PlayTargetCardToFirstEmptyZoneForEffect(this, ChainEntry, Step);
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
	case ETCGEffectStepType::PlayGraveyardCardOnUnit:
	{
		bStepSucceeded = PlayFirstFilteredGraveyardCardOnUnitForEffect(this, ChainEntry, Step);
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
case ETCGEffectStepType::BanishSource:
{
bStepSucceeded = BanishSourceGeneric(this, ChainEntry);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step BanishSource Player=%d Success=%s"),
ChainEntry.ControllerPlayerIndex,
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
break;
}
	case ETCGEffectStepType::DiscardSource:
	{
		bStepSucceeded = DiscardSourceGeneric(this, ChainEntry);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step DiscardSource Player=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::DetachMaterials:
	{
		bStepSucceeded = DetachMaterialsGeneric(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step DetachMaterials Player=%d TargetMode=%s Value=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				GetTCGEffectTargetModeDebugName(Step.TargetMode),
				Step.Value,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::StealMaterials:
	{
		bStepSucceeded = StealMaterialsGeneric(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step StealMaterials Player=%d FromMode=%s Value=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				GetTCGEffectTargetModeDebugName(Step.TargetMode),
				Step.Value,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::AttackDetachTwoStealOneMaterial:
	{
		bStepSucceeded = AttackDetachTwoStealOneMaterial(this, ChainEntry);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step AttackDetachTwoStealOneMaterial Player=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::ReturnUnitToBottomDeck:
	{
		bStepSucceeded = ReturnUnitToBottomDeckGeneric(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step ReturnUnitToBottomDeck Player=%d TargetMode=%s Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				GetTCGEffectTargetModeDebugName(Step.TargetMode),
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::AttackMillTwoWaterBounceBattlingUnit:
	{
		// Legacy alias. Keep old data assets working, but route the pieces through generic bricks.
		FTCGEffectStep LegacyDetachStep;
		LegacyDetachStep.StepType = ETCGEffectStepType::DetachMaterials;
		LegacyDetachStep.TargetMode = ETCGEffectTargetMode::SourceCard;
		LegacyDetachStep.Value = 1;

		const bool bDetachedMaterial = DetachMaterialsGeneric(this, ChainEntry, LegacyDetachStep);
		bool bMilledTwo = false;
		bool bBothWater = false;
		bool bReturnedBattleTarget = false;

		if (bDetachedMaterial)
		{
			const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
			const FTCGCardInstance* BattleTarget = FindCardInstance(ChainEntry.TargetCardInstanceId);

			if (SourceCard
				&& SourceCard->Location == ETCGCardLocation::Board
				&& BattleTarget
				&& BattleTarget->Location == ETCGCardLocation::Board)
			{
				TArray<FTCGCardInstance> DeckCards;
				GetCardsInDeck(SourceCard->OwnerPlayerIndex, DeckCards);

				if (DeckCards.Num() >= 2)
				{
					bBothWater =
						DeckCards[0].Element == ETCGCardElement::Water
						&& DeckCards[1].Element == ETCGCardElement::Water;

					bMilledTwo = SendTopDeckCardsToGraveyard(SourceCard->OwnerPlayerIndex, 2) == 2;

					if (bMilledTwo && bBothWater)
					{
						FTCGEffectStep LegacyReturnStep;
						LegacyReturnStep.StepType = ETCGEffectStepType::ReturnUnitToBottomDeck;
						LegacyReturnStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;

						bReturnedBattleTarget = ReturnUnitToBottomDeckGeneric(this, ChainEntry, LegacyReturnStep);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning,
						TEXT("TCG Effect: Attack mill bounce failed Player=%d Reason=DeckTooSmall Cards=%d"),
						SourceCard->OwnerPlayerIndex,
						DeckCards.Num());
				}
			}
		}

		bStepSucceeded = bMilledTwo;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Attack mill bounce legacy alias Detached=%s MilledTwo=%s BothWater=%s ReturnedTarget=%s"),
			bDetachedMaterial ? TEXT("true") : TEXT("false"),
			bMilledTwo ? TEXT("true") : TEXT("false"),
			bBothWater ? TEXT("true") : TEXT("false"),
			bReturnedBattleTarget ? TEXT("true") : TEXT("false"));

		if (bLogEffectResolution) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step AttackMillTwoWaterBounceBattlingUnit LegacyAlias Player=%d Success=%s"), ChainEntry.ControllerPlayerIndex, bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::DiscardSourceDetachUpToTwoMaterialsFromTarget:
	{
		// Legacy alias. Keep old data assets working, but resolve through generic steps.
		const bool bDiscardedSource = DiscardSourceGeneric(this, ChainEntry);

		if (bDiscardedSource)
		{
			FTCGEffectStep LegacyDetachStep;
			LegacyDetachStep.StepType = ETCGEffectStepType::DetachMaterials;
			LegacyDetachStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
			LegacyDetachStep.Value = 2;
			LegacyDetachStep.bAllowPartialSuccess = true;

			bStepSucceeded = DetachMaterialsGeneric(this, ChainEntry, LegacyDetachStep);
		}
		else
		{
			bStepSucceeded = false;
		}

		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step DiscardSourceDetachUpToTwoMaterialsFromTarget LegacyAlias Player=%d Discarded=%s Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				bDiscardedSource ? TEXT("true") : TEXT("false"),
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::CheckMaterialCount:
	{
		const FGuid TargetCardInstanceId =
			Step.TargetMode == ETCGEffectTargetMode::TriggerTarget
			? ChainEntry.TargetCardInstanceId
			: ChainEntry.SourceCardInstanceId;

		const FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
		const int32 ExpectedMaterialCount = FMath::Max(0, Step.Value);
		const int32 ActualMaterialCount = TargetCard ? GetCardsUnderneathCount(TargetCardInstanceId) : INDEX_NONE;

		bStepSucceeded = TargetCard
			&& TargetCard->Location == ETCGCardLocation::Board
			&& ActualMaterialCount == ExpectedMaterialCount;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: CheckMaterialCount Target=%s TargetMode=%s Expected=%d Actual=%d Success=%s"),
			TargetCard ? *TargetCard->CardDefinitionId.ToString() : TEXT("None"),
			GetTCGEffectTargetModeDebugName(Step.TargetMode),
			ExpectedMaterialCount,
			ActualMaterialCount,
			bStepSucceeded ? TEXT("true") : TEXT("false"));
		break;
	}
	case ETCGEffectStepType::DestroyUnit:
	{
		bStepSucceeded = DestroyTargetUnitByCardEffect(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step DestroyUnit Player=%d TargetMode=%s Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				GetTCGEffectTargetModeDebugName(Step.TargetMode),
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::DestroyTargetUnitByCardEffect:
	{
		// Legacy alias. Keep old data assets working, but prefer generic DestroyUnit.
		bStepSucceeded = DestroyTargetUnitByCardEffect(this, ChainEntry, Step);
		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Step DestroyTargetUnitByCardEffect LegacyAlias Player=%d Success=%s"),
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
	case ETCGEffectStepType::DiscardSourcePreventMaterialLossByCardEffect:
{
bStepSucceeded = DiscardSourcePreventMaterialLossByCardEffect(this, ChainEntry);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step DiscardSourcePreventMaterialLossByCardEffect Player=%d Success=%s"),
ChainEntry.ControllerPlayerIndex,
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
break;
}
case ETCGEffectStepType::BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw:
	{
		// Legacy alias. Keep old data assets working, but resolve through generic steps.
		const bool bBanishedSource = BanishSourceGeneric(this, ChainEntry);
		bool bReturnedCards = false;
		bool bBothDrew = false;

		if (bBanishedSource)
		{
			const int32 MoveCount = 2;
			const bool bChoiceStarted = BeginPendingGraveyardToDeckChoice(ChainEntry.ControllerPlayerIndex, MoveCount, ChainEntry);
			bool bAutoSubmittedChoice = false;

			if (bChoiceStarted && bAutoSubmitDebugGraveyardToDeckChoice)
			{
				TArray<FGuid> ChoiceOptions;
				GetPendingGraveyardToDeckChoiceOptions(ChoiceOptions);

				TArray<FGuid> DebugChosenCards;
				for (int32 Index = 0; Index < ChoiceOptions.Num() && DebugChosenCards.Num() < MoveCount; ++Index)
				{
					DebugChosenCards.Add(ChoiceOptions[Index]);
				}

				bAutoSubmittedChoice = SubmitPendingGraveyardToDeckChoice(ChainEntry.ControllerPlayerIndex, DebugChosenCards);
			}

			bReturnedCards = bAutoSubmittedChoice;

			if (bReturnedCards)
			{
				FTCGEffectStep DrawBothStep;
				DrawBothStep.StepType = ETCGEffectStepType::DrawCardsForBothPlayers;
				DrawBothStep.Value = 1;

				bBothDrew = DrawCardsForBothPlayersGeneric(this, DrawBothStep, &ChainEntry);
			}
		}

		bStepSucceeded = bBanishedSource && bReturnedCards && bBothDrew;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Graveyard recycle both draw legacy alias Banished=%s Returned=%s BothDrew=%s Success=%s"),
			bBanishedSource ? TEXT("true") : TEXT("false"),
			bReturnedCards ? TEXT("true") : TEXT("false"),
			bBothDrew ? TEXT("true") : TEXT("false"),
			bStepSucceeded ? TEXT("true") : TEXT("false"));

		if (bLogEffectResolution)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Step BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw LegacyAlias Player=%d Success=%s"),
				ChainEntry.ControllerPlayerIndex,
				bStepSucceeded ? TEXT("true") : TEXT("false"));
		}
		break;
	}
	case ETCGEffectStepType::SwapTwoOpponentUnitsZones:
{
// Legacy alias. Keep old data assets working, but prefer generic SwapUnitZones.
bStepSucceeded = ResolveSwapUnitZonesStep(this, ChainEntry, bAutoSubmitDebugSwapOpponentUnitZonesChoice);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step SwapTwoOpponentUnitsZones LegacyAlias Player=%d Success=%s"),
ChainEntry.ControllerPlayerIndex,
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
break;
}
case ETCGEffectStepType::SwapUnitZones:
{
bStepSucceeded = ResolveSwapUnitZonesStep(this, ChainEntry, bAutoSubmitDebugSwapOpponentUnitZonesChoice);
if (bLogEffectResolution)
{
UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Step SwapUnitZones Player=%d Success=%s"),
ChainEntry.ControllerPlayerIndex,
bStepSucceeded ? TEXT("true") : TEXT("false"));
}
break;
}
case ETCGEffectStepType::AttachSourceToWaterUnitMaterial:
	case ETCGEffectStepType::AttachSourceToUnitMaterial:
	{
		bStepSucceeded = AttachSourceToFirstFilteredUnitMaterialForEffect(this, ChainEntry, Step);
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
