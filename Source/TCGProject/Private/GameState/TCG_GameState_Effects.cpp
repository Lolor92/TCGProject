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
}
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
		case ETCGEffectStepType::BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw: return TEXT("BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw");
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
