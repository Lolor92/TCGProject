#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

void ATCG_GameState::RunDebugSwapOpponentUnitZonesScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: SwapOpponentUnitZones scenario start"));

	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Battle);
	SetMatchResult(ETCGMatchResult::None);

	const FName SourceZoneId = GetFieldZoneId(0, 0);
	const FGuid SourceId = AddCardInstance("Debug_SwapZones_Source", ETCGCardElement::Wind, 1, 0, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* Source = FindCardInstance(SourceId))
	{
		Source->ZoneId = SourceZoneId;
		Source->StackId = FGuid::NewGuid();
		Source->StackIndex = 0;
	}

	const FName OpponentZoneA = GetFieldZoneId(1, 0);
	const FName OpponentZoneB = GetFieldZoneId(1, 1);

	const FGuid OpponentUnitAId = AddCardInstance("Debug_SwapZones_OpponentA", ETCGCardElement::Fire, 2, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* OpponentA = FindCardInstance(OpponentUnitAId))
	{
		OpponentA->ZoneId = OpponentZoneA;
		OpponentA->StackId = FGuid::NewGuid();
		OpponentA->StackIndex = 0;
	}

	const FGuid OpponentUnitBId = AddCardInstance("Debug_SwapZones_OpponentB", ETCGCardElement::Water, 3, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* OpponentB = FindCardInstance(OpponentUnitBId))
	{
		OpponentB->ZoneId = OpponentZoneB;
		OpponentB->StackId = FGuid::NewGuid();
		OpponentB->StackIndex = 0;
	}

	FTCGCardEffectRef SwapEffect;
	SwapEffect.Trigger = ETCGEffectTrigger::OnPlay;

	FTCGEffectStep SwapStep;
	SwapStep.StepType = ETCGEffectStepType::SwapTwoOpponentUnitsZones;
	SwapEffect.Steps.Add(SwapStep);

	TArray<FTCGEffectChainEntry> Chain;
	const bool bChainAdded = AddCardEffectRefToChain(Chain, SourceId, SourceId, SwapEffect);
	const bool bChainResolved = ResolveEffectChain(Chain);

	const FTCGCardInstance* OpponentAAfter = FindCardInstance(OpponentUnitAId);
	const FTCGCardInstance* OpponentBAfter = FindCardInstance(OpponentUnitBId);

	const bool bOpponentASwapped = OpponentAAfter && OpponentAAfter->ZoneId == OpponentZoneB;
	const bool bOpponentBSwapped = OpponentBAfter && OpponentBAfter->ZoneId == OpponentZoneA;
	const bool bBothStillBoard = OpponentAAfter && OpponentBAfter
		&& OpponentAAfter->Location == ETCGCardLocation::Board
		&& OpponentBAfter->Location == ETCGCardLocation::Board;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: SwapOpponentUnitZones summary ChainAdded=%s ChainResolved=%s OpponentASwapped=%s OpponentBSwapped=%s BothStillBoard=%s"),
		bChainAdded ? TEXT("true") : TEXT("false"),
		bChainResolved ? TEXT("true") : TEXT("false"),
		bOpponentASwapped ? TEXT("true") : TEXT("false"),
		bOpponentBSwapped ? TEXT("true") : TEXT("false"),
		bBothStillBoard ? TEXT("true") : TEXT("false"));

	EndMatch(ETCGMatchResult::Draw);
}
