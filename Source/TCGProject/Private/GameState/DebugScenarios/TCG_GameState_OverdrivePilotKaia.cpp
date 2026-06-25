#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
UTCG_CardDefinition* MakeDebugDefinition(
	UObject* Outer,
	FName CardDefinitionId,
	const TCHAR* DisplayName,
	ETCGCardElement Element,
	int32 BaseAttack)
{
	UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(Outer);
	Definition->CardDefinitionId = CardDefinitionId;
	Definition->DisplayName = FText::FromString(DisplayName);
	Definition->Element = Element;
	Definition->BaseAttack = BaseAttack;
	return Definition;
}

static int32 CountMaterialsUnderUnit(const ATCG_GameState* GameState, const FGuid& TopCardId)
{
	if (!GameState)
	{
		return 0;
	}

	const FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardId);
	if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid())
	{
		return 0;
	}

	int32 Count = 0;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board
			&& Card.StackId == TopCard->StackId
			&& Card.CardInstanceId != TopCard->CardInstanceId
			&& Card.StackIndex < TopCard->StackIndex)
		{
			Count++;
		}
	}

	return Count;
}
}

void RunDebugOverdrivePilotKaiaScenario(ATCG_GameState* GameState)
{
	if (!GameState)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: OverdrivePilotKaia scenario start"));

	GameState->MatchCards.Empty();
	GameState->SetMatchResult(ETCGMatchResult::None);
	GameState->SetPhase(ETCGMatchPhase::Placement);
	GameState->RoundNumber = 1;
	GameState->TurnNumber = 1;
	GameState->PlacementStepIndex = 0;
	GameState->Player0PlacementFieldZonesUsedThisRound.Reset();
	GameState->Player1PlacementFieldZonesUsedThisRound.Reset();
	GameState->SetCurrentTurnPlayer(0);
	GameState->ClearPendingDiscardChoice();

	GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
	{
		return Definition
			&& (Definition->CardDefinitionId == "Debug_OverdrivePilotKaia_Stormflare"
				|| Definition->CardDefinitionId == "Debug_OverdrivePilotKaia_Stormflare_Hand"
				|| Definition->CardDefinitionId == "Debug_Kaia_Machine_Deck_Search_Target");
	});

	UTCG_CardDefinition* PlayedKaiaDefinition = MakeDebugDefinition(
		GameState,
		"Debug_OverdrivePilotKaia_Stormflare",
		TEXT("Overdrive Pilot - Kaia Stormflare"),
		ETCGCardElement::Wind,
		1);

	FTCGCardEffectRef KaiaOnPlay;
	KaiaOnPlay.Trigger = ETCGEffectTrigger::OnPlay;

	FTCGEffectStep SearchMachineStep;
	SearchMachineStep.StepType = ETCGEffectStepType::MoveDeckCardToHand;
	SearchMachineStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
	SearchMachineStep.TargetFilter.RequiredLocation = ETCGCardLocation::Deck;
	SearchMachineStep.TargetFilter.NameContains = "Machine";
	KaiaOnPlay.Steps.Add(SearchMachineStep);

	FTCGEffectStep DiscardStep;
	DiscardStep.StepType = ETCGEffectStepType::DiscardCards;
	DiscardStep.Value = 1;
	DiscardStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	DiscardStep.bRequiresPreviousStepSuccess = true;
	KaiaOnPlay.Steps.Add(DiscardStep);

	PlayedKaiaDefinition->Effects.Add(KaiaOnPlay);

	UTCG_CardDefinition* HandKaiaDefinition = MakeDebugDefinition(
		GameState,
		"Debug_OverdrivePilotKaia_Stormflare_Hand",
		TEXT("Overdrive Pilot - Kaia Stormflare"),
		ETCGCardElement::Wind,
		1);

	FTCGCardEffectRef KaiaMachinePlayed;
	KaiaMachinePlayed.Trigger = ETCGEffectTrigger::OnYourUnitPlayed;
	KaiaMachinePlayed.bOptional = true;
	KaiaMachinePlayed.TriggerFilter.OwnerMode = ETCGEffectTargetMode::Controller;
	KaiaMachinePlayed.TriggerFilter.RequiredLocation = ETCGCardLocation::Board;
	KaiaMachinePlayed.TriggerFilter.bRequireTopCard = true;
	KaiaMachinePlayed.TriggerFilter.NameContains = "Machine";

	FTCGEffectStep AttachStep;
	AttachStep.StepType = ETCGEffectStepType::AttachSourceToUnitMaterial;
	AttachStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
	KaiaMachinePlayed.Steps.Add(AttachStep);

	HandKaiaDefinition->Effects.Add(KaiaMachinePlayed);

	UTCG_CardDefinition* Machine = MakeDebugDefinition(
		GameState,
		"Debug_Kaia_Machine_Deck_Search_Target",
		TEXT("Debug Machine Deck Search Target"),
		ETCGCardElement::Earth,
		2);

	GameState->DebugCardDefinitions.Add(PlayedKaiaDefinition);
	GameState->DebugCardDefinitions.Add(HandKaiaDefinition);
	GameState->DebugCardDefinitions.Add(Machine);

	FTCGCardInstance* PlayedKaia = GameState->AddCardInstanceFromDefinition(PlayedKaiaDefinition, 0, ETCGCardLocation::Hand);
	if (!PlayedKaia)
	{
		return;
	}
	const FGuid PlayedKaiaId = PlayedKaia->CardInstanceId;

	FTCGCardInstance& DiscardJunk = GameState->AddCardInstance("Debug_Kaia_Discard_Junk", ETCGCardElement::Fire, 1, 0, ETCGCardLocation::Hand);
	const FGuid DiscardJunkId = DiscardJunk.CardInstanceId;

	FTCGCardInstance* MachineCard = GameState->AddCardInstanceFromDefinition(Machine, 0, ETCGCardLocation::Deck);
	if (!MachineCard)
	{
		return;
	}
	const FGuid MachineId = MachineCard->CardInstanceId;

	FTCGCardInstance* HandKaia = GameState->AddCardInstanceFromDefinition(HandKaiaDefinition, 0, ETCGCardLocation::Hand);
	if (!HandKaia)
	{
		return;
	}
	const FGuid HandKaiaId = HandKaia->CardInstanceId;
	const bool bPlayedKaia = GameState->PlayCardToZone(PlayedKaiaId, ATCG_GameState::GetFieldZoneId(0, 0));
	const FTCGCardInstance* MachineAfterSearch = GameState->FindCardInstance(MachineId);
	const FTCGCardInstance* JunkAfterDiscard = GameState->FindCardInstance(DiscardJunkId);
	const bool bMachineAddedToHand = MachineAfterSearch && MachineAfterSearch->Location == ETCGCardLocation::Hand;
	const bool bJunkDiscarded = JunkAfterDiscard && JunkAfterDiscard->Location == ETCGCardLocation::Graveyard;

	const bool bPlayedMachine = GameState->PlayCardToZone(MachineId, ATCG_GameState::GetFieldZoneId(0, 1));

	const FTCGCardInstance* MachineAfterPlay = GameState->FindCardInstance(MachineId);
	const FTCGCardInstance* HandKaiaAfterMachine = GameState->FindCardInstance(HandKaiaId);

	const bool bHandKaiaAttached =
		MachineAfterPlay
		&& HandKaiaAfterMachine
		&& MachineAfterPlay->Location == ETCGCardLocation::Board
		&& HandKaiaAfterMachine->Location == ETCGCardLocation::Board
		&& MachineAfterPlay->StackId.IsValid()
		&& HandKaiaAfterMachine->StackId == MachineAfterPlay->StackId
		&& HandKaiaAfterMachine->StackIndex < MachineAfterPlay->StackIndex;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: OverdrivePilotKaia summary PlayedKaia=%s MachineAddedToHand=%s JunkDiscarded=%s PlayedMachine=%s HandKaiaAttached=%s MachineMaterials=%d ExpectedAll=true"),
		bPlayedKaia ? TEXT("true") : TEXT("false"),
		bMachineAddedToHand ? TEXT("true") : TEXT("false"),
		bJunkDiscarded ? TEXT("true") : TEXT("false"),
		bPlayedMachine ? TEXT("true") : TEXT("false"),
		bHandKaiaAttached ? TEXT("true") : TEXT("false"),
		CountMaterialsUnderUnit(GameState, MachineId));

	GameState->EndMatch(ETCGMatchResult::Draw);
}
