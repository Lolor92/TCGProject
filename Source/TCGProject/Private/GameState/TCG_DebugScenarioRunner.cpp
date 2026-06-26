#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
	enum class ETCGDebugRunnerScenario : uint8
	{
		NormalRoundFlow,
		WraparoundBattle,
		RoundLimitTiebreak,
		OverlayPlacement,
		HandDetachResponse,
		MachineMaterialRecovery,
		GraveyardMachineProtection,
		SelectedGraveyardPlay,
		FilteredMaterialDefense,
		GraveyardPilotMachineAttach,
		MatchingMaterialDestroy,
		OverdrivePilotKaia
	};

	constexpr ETCGDebugRunnerScenario DebugRunnerScenario = ETCGDebugRunnerScenario::MatchingMaterialDestroy;
	constexpr bool bDebugRunnerLogDebugSetup = false;
	constexpr bool bDebugRunnerLogRoundFlow = true;
	constexpr bool bDebugRunnerLogPlacementFlow = true;

	const FName DebugRunnerCard_FireDeckA = "Debug_Fire_Deck_A";
	const FName DebugRunnerCard_FireDeckB = "Debug_Fire_Deck_B";

	static const TCHAR* GetTCGDebugRunnerMatchResultDebugName(ETCGMatchResult Result)
	{
		switch (Result)
		{
		case ETCGMatchResult::Player0Wins: return TEXT("Player 0 wins");
		case ETCGMatchResult::Player1Wins: return TEXT("Player 1 wins");
		case ETCGMatchResult::Draw: return TEXT("Draw");
		default: return TEXT("None");
		}
	}
}

void UTCG_DebugScenarioRunner::CreateDebugTestCards(ATCG_GameState* GameState)
{
	SetupDebugMatch(GameState);
}

void UTCG_DebugScenarioRunner::SetupDebugMatch(ATCG_GameState* GameState)
{
	if (!GameState) return;

	GameState->MatchCards.Empty();
	GameState->ValidateDebugCardDefinitions();

	for (int32 CopyIndex = 0; CopyIndex < 2; ++CopyIndex)
	{
		GameState->AddDebugCardInstance("Debug_Earth_Deck_A", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance("Debug_Earth_Deck_B", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance(DebugRunnerCard_FireDeckA, ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance(DebugRunnerCard_FireDeckB, ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance("Debug_Dark_Deck_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance("Debug_Dark_Deck_B", ETCGCardElement::Dark, 2, 1, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
		GameState->AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
	}

	TArray<FTCGCardInstance> Player0Deck;
	TArray<FTCGCardInstance> Player1Deck;
	GameState->GetCardsInDeck(0, Player0Deck);
	GameState->GetCardsInDeck(1, Player1Deck);
	if (bDebugRunnerLogDebugSetup) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Setup match decks P0=%d P1=%d"), Player0Deck.Num(), Player1Deck.Num());
}

void UTCG_DebugScenarioRunner::RunDebugTurnFlow(ATCG_GameState* GameState)
{
	if (!GameState) return;

	auto AddDebugBoardUnit = [GameState](FName CardDefinitionId, ETCGCardElement Element, int32 Attack, int32 PlayerIndex, int32 FieldIndex)
	{
		FTCGCardInstance& Card = GameState->AddCardInstance(CardDefinitionId, Element, Attack, PlayerIndex, ETCGCardLocation::Board);
		Card.ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
		Card.StackId = FGuid::NewGuid();
		Card.StackIndex = 0;
		return Card.CardInstanceId;
	};

	auto CountBoardUnits = [GameState](int32 PlayerIndex)
	{
		TSet<FGuid> StackIds;
		for (const FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Board && Card.StackId.IsValid()) StackIds.Add(Card.StackId);
		}
		return StackIds.Num();
	};

	auto CountMaterialsUnderUnit = [GameState](const FGuid& TopCardId)
	{
		const FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardId);
		if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid()) return 0;

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
	};

	auto GetRoundLimitResult = [&CountBoardUnits]()
	{
		const int32 Player0UnitCount = CountBoardUnits(0);
		const int32 Player1UnitCount = CountBoardUnits(1);
		if (Player0UnitCount > Player1UnitCount) return ETCGMatchResult::Player0Wins;
		if (Player1UnitCount > Player0UnitCount) return ETCGMatchResult::Player1Wins;
		return ETCGMatchResult::Draw;
	};





	if (DebugRunnerScenario == ETCGDebugRunnerScenario::MatchingMaterialDestroy)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Matching material destroy scenario start"));

		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->RoundNumber = 1;
		GameState->TurnNumber = 1;
		GameState->PlacementStepIndex = 0;
		GameState->SetCurrentTurnPlayer(INDEX_NONE);
		GameState->ClearPendingDiscardChoice();
		GameState->ClearPendingGraveyardToDeckChoice();

		GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
		{
			return Definition
				&& (Definition->CardDefinitionId == "Debug_MatchingMaterialDestroy_Attacker"
					|| Definition->CardDefinitionId == "Debug_MatchingMaterialDestroy_ProtectedOwnUnit");
		});

		auto MakeDefinition = [GameState](
			FName CardDefinitionId,
			const TCHAR* DisplayName,
			ETCGCardElement Element,
			int32 BaseAttack)
		{
			UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
			Definition->CardDefinitionId = CardDefinitionId;
			Definition->DisplayName = FText::FromString(DisplayName);
			Definition->Element = Element;
			Definition->BaseAttack = BaseAttack;
			return Definition;
		};

		UTCG_CardDefinition* AttackerDefinition = MakeDefinition(
			"Debug_MatchingMaterialDestroy_Attacker",
			TEXT("Debug Matching Material Destroy Attacker"),
			ETCGCardElement::Dark,
			10);

		FTCGCardEffectRef AttackEffect;
		AttackEffect.Trigger = ETCGEffectTrigger::OnAttack;
		AttackEffect.bOptional = true;
		AttackEffect.TriggerFilter.bRequireTopCard = true;

		FTCGEffectStep DestroyMatchingStep;
		DestroyMatchingStep.StepType = ETCGEffectStepType::DestroyUnitsWithMatchingMaterialCount;
		DestroyMatchingStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		DestroyMatchingStep.TargetFilter.RequiredLocation = ETCGCardLocation::Board;
		DestroyMatchingStep.TargetFilter.bRequireTopCard = true;
		DestroyMatchingStep.TargetFilter.NameContains = "ProtectedOwnUnit";
		DestroyMatchingStep.SecondaryTargetFilter.OwnerMode = ETCGEffectTargetMode::Opponent;
		DestroyMatchingStep.SecondaryTargetFilter.RequiredLocation = ETCGCardLocation::Board;
		DestroyMatchingStep.SecondaryTargetFilter.bRequireTopCard = true;
		DestroyMatchingStep.SecondaryTargetFilter.NameContains = "TargetOpponentUnit";
		AttackEffect.Steps.Add(DestroyMatchingStep);

		AttackerDefinition->Effects.Add(AttackEffect);

		UTCG_CardDefinition* ProtectedOwnDefinition = MakeDefinition(
			"Debug_MatchingMaterialDestroy_ProtectedOwnUnit",
			TEXT("Debug Protected Own Unit"),
			ETCGCardElement::Light,
			1);

		FTCGCardEffectRef CannotDestroyEffect;
		CannotDestroyEffect.Trigger = ETCGEffectTrigger::None;

		FTCGEffectStep CannotDestroyStep;
		CannotDestroyStep.StepType = ETCGEffectStepType::CannotBeDestroyedByCardEffects;
		CannotDestroyEffect.Steps.Add(CannotDestroyStep);

		ProtectedOwnDefinition->Effects.Add(CannotDestroyEffect);

		GameState->DebugCardDefinitions.Add(AttackerDefinition);
		GameState->DebugCardDefinitions.Add(ProtectedOwnDefinition);

		FTCGCardInstance* AttackerCard = GameState->AddCardInstanceFromDefinition(
			AttackerDefinition,
			0,
			ETCGCardLocation::Board);
		const FGuid AttackerId = AttackerCard ? AttackerCard->CardInstanceId : FGuid();

		FTCGCardInstance* ProtectedOwnUnit = GameState->AddCardInstanceFromDefinition(
			ProtectedOwnDefinition,
			0,
			ETCGCardLocation::Board);
		const FGuid ProtectedOwnUnitId = ProtectedOwnUnit ? ProtectedOwnUnit->CardInstanceId : FGuid();

		if (!AttackerCard || !ProtectedOwnUnit || !AttackerId.IsValid() || !ProtectedOwnUnitId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Matching material destroy setup failed"));
			GameState->EndMatch(ETCGMatchResult::Draw);
			return;
		}

		AttackerCard->ZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
		AttackerCard->StackId = FGuid::NewGuid();
		AttackerCard->StackIndex = 0;

		ProtectedOwnUnit->ZoneId = ATCG_GameState::GetFieldZoneId(0, 1);
		ProtectedOwnUnit->StackId = FGuid::NewGuid();
		ProtectedOwnUnit->StackIndex = 2;

		auto AddMaterialUnderUnit = [GameState](const FGuid& TopUnitId, FName MaterialName, int32 OwnerPlayerIndex, int32 StackIndex)
		{
			FTCGCardInstance* TopUnit = GameState->FindCardInstance(TopUnitId);
			if (!TopUnit)
			{
				return FGuid();
			}

			FTCGCardInstance& Material = GameState->AddCardInstance(
				MaterialName,
				ETCGCardElement::Earth,
				1,
				OwnerPlayerIndex,
				ETCGCardLocation::Board);

			Material.ZoneId = TopUnit->ZoneId;
			Material.StackId = TopUnit->StackId;
			Material.StackIndex = StackIndex;
			return Material.CardInstanceId;
		};

		AddMaterialUnderUnit(ProtectedOwnUnitId, "Debug_MatchingMaterialDestroy_OwnMatA", 0, 0);
		AddMaterialUnderUnit(ProtectedOwnUnitId, "Debug_MatchingMaterialDestroy_OwnMatB", 0, 1);

		const FGuid TargetOpponentUnitId = AddDebugBoardUnit(
			"Debug_MatchingMaterialDestroy_TargetOpponentUnit",
			ETCGCardElement::Water,
			1,
			1,
			0);

		FTCGCardInstance* TargetOpponentUnit = GameState->FindCardInstance(TargetOpponentUnitId);
		if (TargetOpponentUnit)
		{
			TargetOpponentUnit->StackIndex = 2;
		}

		AddMaterialUnderUnit(TargetOpponentUnitId, "Debug_MatchingMaterialDestroy_OppMatA", 1, 0);
		AddMaterialUnderUnit(TargetOpponentUnitId, "Debug_MatchingMaterialDestroy_OppMatB", 1, 1);

		const FGuid WrongOpponentUnitId = AddDebugBoardUnit(
			"Debug_MatchingMaterialDestroy_WrongOpponentUnit",
			ETCGCardElement::Water,
			1,
			1,
			1);

		FTCGCardInstance* WrongOpponentUnit = GameState->FindCardInstance(WrongOpponentUnitId);
		if (WrongOpponentUnit)
		{
			WrongOpponentUnit->StackIndex = 1;
		}

		AddMaterialUnderUnit(WrongOpponentUnitId, "Debug_MatchingMaterialDestroy_WrongOppMatA", 1, 0);

		const int32 OwnMaterialsBefore = CountMaterialsUnderUnit(ProtectedOwnUnitId);
		const int32 TargetOpponentMaterialsBefore = CountMaterialsUnderUnit(TargetOpponentUnitId);
		const int32 WrongOpponentMaterialsBefore = CountMaterialsUnderUnit(WrongOpponentUnitId);

		const bool bResolvedBattle = GameState->ResolveBattlePhase();

		const FTCGCardInstance* AttackerAfter = GameState->FindCardInstance(AttackerId);
		const FTCGCardInstance* ProtectedOwnAfter = GameState->FindCardInstance(ProtectedOwnUnitId);
		const FTCGCardInstance* TargetOpponentAfter = GameState->FindCardInstance(TargetOpponentUnitId);
		const FTCGCardInstance* WrongOpponentAfter = GameState->FindCardInstance(WrongOpponentUnitId);

		const bool bAttackerStayedBoard =
			AttackerAfter
			&& AttackerAfter->Location == ETCGCardLocation::Board;

		const bool bProtectedOwnStayedBoard =
			ProtectedOwnAfter
			&& ProtectedOwnAfter->Location == ETCGCardLocation::Board;

		const bool bTargetOpponentDestroyed =
			TargetOpponentAfter
			&& TargetOpponentAfter->Location == ETCGCardLocation::Graveyard;

		const bool bWrongOpponentStayedBoard =
			WrongOpponentAfter
			&& WrongOpponentAfter->Location == ETCGCardLocation::Board;

		const bool bExpectedAll =
			bResolvedBattle
			&& OwnMaterialsBefore == 2
			&& TargetOpponentMaterialsBefore == 2
			&& WrongOpponentMaterialsBefore == 1
			&& bAttackerStayedBoard
			&& bProtectedOwnStayedBoard
			&& bTargetOpponentDestroyed
			&& bWrongOpponentStayedBoard;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: MatchingMaterialDestroy summary BattleResolved=%s OwnMaterials=%d TargetOpponentMaterials=%d WrongOpponentMaterials=%d AttackerStayed=%s ProtectedOwnStayed=%s TargetOpponentDestroyed=%s WrongOpponentStayed=%s ExpectedAll=%s"),
			bResolvedBattle ? TEXT("true") : TEXT("false"),
			OwnMaterialsBefore,
			TargetOpponentMaterialsBefore,
			WrongOpponentMaterialsBefore,
			bAttackerStayedBoard ? TEXT("true") : TEXT("false"),
			bProtectedOwnStayedBoard ? TEXT("true") : TEXT("false"),
			bTargetOpponentDestroyed ? TEXT("true") : TEXT("false"),
			bWrongOpponentStayedBoard ? TEXT("true") : TEXT("false"),
			bExpectedAll ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::GraveyardPilotMachineAttach)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Graveyard pilot machine attach scenario start"));

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
		GameState->ClearPendingGraveyardToDeckChoice();

		GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
		{
			return Definition
				&& (Definition->CardDefinitionId == "Debug_GraveyardPilotMachineAttach_Source"
					|| Definition->CardDefinitionId == "Debug_GraveyardPilotMachineAttach_Pilot"
					|| Definition->CardDefinitionId == "Debug_GraveyardPilotMachineAttach_Junk");
		});

		auto MakeDefinition = [GameState](
			FName CardDefinitionId,
			const TCHAR* DisplayName,
			ETCGCardElement Element,
			int32 BaseAttack)
		{
			UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
			Definition->CardDefinitionId = CardDefinitionId;
			Definition->DisplayName = FText::FromString(DisplayName);
			Definition->Element = Element;
			Definition->BaseAttack = BaseAttack;
			return Definition;
		};

		UTCG_CardDefinition* SourceDefinition = MakeDefinition(
			"Debug_GraveyardPilotMachineAttach_Source",
			TEXT("Debug Graveyard Pilot Machine Attach Source"),
			ETCGCardElement::Light,
			1);

		FTCGCardEffectRef AttachEffect;
		AttachEffect.Trigger = ETCGEffectTrigger::OnPlay;
		AttachEffect.bOptional = true;
		AttachEffect.TriggerFilter.bRequireTopCard = true;

		FTCGEffectStep AttachStep;
		AttachStep.StepType = ETCGEffectStepType::AttachGraveyardCardToSourceMaterial;
		AttachStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		AttachStep.TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		AttachStep.TargetFilter.bRequireTopCard = false;
		AttachStep.TargetFilter.NameContainsAny.Add(TEXT("Pilot"));
		AttachStep.TargetFilter.NameContainsAny.Add(TEXT("Machine"));
		AttachEffect.Steps.Add(AttachStep);

		SourceDefinition->Effects.Add(AttachEffect);

		UTCG_CardDefinition* PilotDefinition = MakeDefinition(
			"Debug_GraveyardPilotMachineAttach_Pilot",
			TEXT("Debug Graveyard Pilot Unit"),
			ETCGCardElement::Wind,
			2);

		UTCG_CardDefinition* JunkDefinition = MakeDefinition(
			"Debug_GraveyardPilotMachineAttach_Junk",
			TEXT("Debug Graveyard Junk Unit"),
			ETCGCardElement::Earth,
			2);

		GameState->DebugCardDefinitions.Add(SourceDefinition);
		GameState->DebugCardDefinitions.Add(PilotDefinition);
		GameState->DebugCardDefinitions.Add(JunkDefinition);

		FTCGCardInstance* SourceCard = GameState->AddCardInstanceFromDefinition(
			SourceDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid SourceId = SourceCard ? SourceCard->CardInstanceId : FGuid();

		FTCGCardInstance* PilotCard = GameState->AddCardInstanceFromDefinition(
			PilotDefinition,
			0,
			ETCGCardLocation::Graveyard);
		const FGuid PilotId = PilotCard ? PilotCard->CardInstanceId : FGuid();

		FTCGCardInstance* JunkCard = GameState->AddCardInstanceFromDefinition(
			JunkDefinition,
			0,
			ETCGCardLocation::Graveyard);
		const FGuid JunkId = JunkCard ? JunkCard->CardInstanceId : FGuid();

		if (!SourceId.IsValid() || !PilotId.IsValid() || !JunkId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Graveyard pilot machine attach setup failed"));
			GameState->EndMatch(ETCGMatchResult::Draw);
			return;
		}

		const bool bPlayedSource = GameState->PlayCardToZone(
			SourceId,
			ATCG_GameState::GetFieldZoneId(0, 0));

		const FTCGCardInstance* SourceAfter = GameState->FindCardInstance(SourceId);
		const FTCGCardInstance* PilotAfter = GameState->FindCardInstance(PilotId);
		const FTCGCardInstance* JunkAfter = GameState->FindCardInstance(JunkId);

		const int32 MaterialsAfter = CountMaterialsUnderUnit(SourceId);

		const bool bPilotAttached =
			SourceAfter
			&& PilotAfter
			&& SourceAfter->Location == ETCGCardLocation::Board
			&& PilotAfter->Location == ETCGCardLocation::Board
			&& SourceAfter->StackId.IsValid()
			&& PilotAfter->StackId == SourceAfter->StackId
			&& PilotAfter->StackIndex < SourceAfter->StackIndex;

		const bool bJunkStayedGraveyard =
			JunkAfter
			&& JunkAfter->Location == ETCGCardLocation::Graveyard;

		const bool bExpectedAll =
			bPlayedSource
			&& bPilotAttached
			&& bJunkStayedGraveyard
			&& MaterialsAfter == 1;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: GraveyardPilotMachineAttach summary PlayedSource=%s PilotAttached=%s JunkStayedGraveyard=%s MaterialsAfter=%d ExpectedAll=%s"),
			bPlayedSource ? TEXT("true") : TEXT("false"),
			bPilotAttached ? TEXT("true") : TEXT("false"),
			bJunkStayedGraveyard ? TEXT("true") : TEXT("false"),
			MaterialsAfter,
			bExpectedAll ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::FilteredMaterialDefense)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Filtered material defense scenario start"));

		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->RoundNumber = 1;
		GameState->TurnNumber = 1;
		GameState->PlacementStepIndex = 0;
		GameState->SetCurrentTurnPlayer(INDEX_NONE);
		GameState->ClearPendingDiscardChoice();
		GameState->ClearPendingGraveyardToDeckChoice();

		GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
		{
			return Definition
				&& Definition->CardDefinitionId == "Debug_FilteredMaterialDefense_Defender";
		});

		UTCG_CardDefinition* DefenderDefinition = NewObject<UTCG_CardDefinition>(GameState);
		DefenderDefinition->CardDefinitionId = "Debug_FilteredMaterialDefense_Defender";
		DefenderDefinition->DisplayName = FText::FromString(TEXT("Debug Filtered Material Defense"));
		DefenderDefinition->Element = ETCGCardElement::Light;
		DefenderDefinition->BaseAttack = 1;

		FTCGCardEffectRef DefenseEffect;
		DefenseEffect.Trigger = ETCGEffectTrigger::OnOpponentAttack;
		DefenseEffect.bOptional = true;
		DefenseEffect.TriggerFilter.bRequireTopCard = true;

		FTCGEffectStep DetachCostStep;
		DetachCostStep.StepType = ETCGEffectStepType::DetachFilteredMaterials;
		DetachCostStep.TargetMode = ETCGEffectTargetMode::SourceCard;
		DetachCostStep.Value = 2;
		DetachCostStep.TargetFilter.NameContains = "Pilot";
		DetachCostStep.SecondaryTargetFilter.NameContains = "Machine";
		DefenseEffect.Steps.Add(DetachCostStep);

		FTCGEffectStep DestroyAttackerStep;
		DestroyAttackerStep.StepType = ETCGEffectStepType::DestroyUnit;
		DestroyAttackerStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
		DestroyAttackerStep.bRequiresPreviousStepSuccess = true;
		DefenseEffect.Steps.Add(DestroyAttackerStep);

		DefenderDefinition->Effects.Add(DefenseEffect);
		GameState->DebugCardDefinitions.Add(DefenderDefinition);

		const FGuid AttackerId = AddDebugBoardUnit(
			"Debug_FilteredMaterialDefense_Attacker",
			ETCGCardElement::Dark,
			10,
			0,
			0);

		FTCGCardInstance* DefenderCard = GameState->AddCardInstanceFromDefinition(
			DefenderDefinition,
			1,
			ETCGCardLocation::Board);
		const FGuid DefenderId = DefenderCard ? DefenderCard->CardInstanceId : FGuid();

		if (!AttackerId.IsValid() || !DefenderId.IsValid() || !DefenderCard)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Filtered material defense setup failed"));
			GameState->EndMatch(ETCGMatchResult::Draw);
			return;
		}

		DefenderCard->ZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
		DefenderCard->StackId = FGuid::NewGuid();
		DefenderCard->StackIndex = 2;

		FTCGCardInstance& PilotMaterial = GameState->AddCardInstance(
			"Debug_FilteredMaterialDefense_Pilot",
			ETCGCardElement::Wind,
			1,
			1,
			ETCGCardLocation::Board);
		PilotMaterial.ZoneId = DefenderCard->ZoneId;
		PilotMaterial.StackId = DefenderCard->StackId;
		PilotMaterial.StackIndex = 0;

		FTCGCardInstance& MachineMaterial = GameState->AddCardInstance(
			"Debug_FilteredMaterialDefense_Machine",
			ETCGCardElement::Fire,
			1,
			1,
			ETCGCardLocation::Board);
		MachineMaterial.ZoneId = DefenderCard->ZoneId;
		MachineMaterial.StackId = DefenderCard->StackId;
		MachineMaterial.StackIndex = 1;

		const int32 MaterialsBefore = CountMaterialsUnderUnit(DefenderId);
		const bool bResolvedBattle = GameState->ResolveBattlePhase();

		const FTCGCardInstance* AttackerAfter = GameState->FindCardInstance(AttackerId);
		const FTCGCardInstance* DefenderAfter = GameState->FindCardInstance(DefenderId);
		const FTCGCardInstance* PilotAfter = GameState->FindCardInstance(PilotMaterial.CardInstanceId);
		const FTCGCardInstance* MachineAfter = GameState->FindCardInstance(MachineMaterial.CardInstanceId);

		const int32 MaterialsAfter = CountMaterialsUnderUnit(DefenderId);

		const bool bAttackerDestroyed =
			AttackerAfter
			&& AttackerAfter->Location == ETCGCardLocation::Graveyard;

		const bool bDefenderStayedBoard =
			DefenderAfter
			&& DefenderAfter->Location == ETCGCardLocation::Board;

		const bool bPilotDetached =
			PilotAfter
			&& PilotAfter->Location == ETCGCardLocation::Graveyard;

		const bool bMachineDetached =
			MachineAfter
			&& MachineAfter->Location == ETCGCardLocation::Graveyard;

		const bool bExpectedAll =
			bResolvedBattle
			&& MaterialsBefore == 2
			&& MaterialsAfter == 0
			&& bPilotDetached
			&& bMachineDetached
			&& bAttackerDestroyed
			&& bDefenderStayedBoard;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: FilteredMaterialDefense summary BattleResolved=%s MaterialsBefore=%d MaterialsAfter=%d PilotDetached=%s MachineDetached=%s AttackerDestroyed=%s DefenderStayedBoard=%s ExpectedAll=%s"),
			bResolvedBattle ? TEXT("true") : TEXT("false"),
			MaterialsBefore,
			MaterialsAfter,
			bPilotDetached ? TEXT("true") : TEXT("false"),
			bMachineDetached ? TEXT("true") : TEXT("false"),
			bAttackerDestroyed ? TEXT("true") : TEXT("false"),
			bDefenderStayedBoard ? TEXT("true") : TEXT("false"),
			bExpectedAll ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::SelectedGraveyardPlay)
{
UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Selected graveyard play scenario start"));

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
GameState->ClearPendingGraveyardToDeckChoice();

GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
{
return Definition
&& (Definition->CardDefinitionId == "Debug_SelectedGraveyardPlay_Source"
|| Definition->CardDefinitionId == "Debug_SelectedGraveyardPlay_Cargo");
});

auto MakeDefinition = [GameState](
FName CardDefinitionId,
const TCHAR* DisplayName,
ETCGCardElement Element,
int32 BaseAttack)
{
UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
Definition->CardDefinitionId = CardDefinitionId;
Definition->DisplayName = FText::FromString(DisplayName);
Definition->Element = Element;
Definition->BaseAttack = BaseAttack;
return Definition;
};

UTCG_CardDefinition* SourceDefinition = MakeDefinition(
"Debug_SelectedGraveyardPlay_Source",
TEXT("Debug Selected Graveyard Play Source"),
ETCGCardElement::Light,
1);

FTCGCardEffectRef SelectAndPlayEffect;
SelectAndPlayEffect.Trigger = ETCGEffectTrigger::OnPlay;
SelectAndPlayEffect.bOptional = true;
SelectAndPlayEffect.TriggerFilter.bRequireTopCard = true;

FTCGEffectStep SelectChosenHostStep;
SelectChosenHostStep.StepType = ETCGEffectStepType::SelectTarget;
SelectChosenHostStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
SelectChosenHostStep.TargetFilter.RequiredLocation = ETCGCardLocation::Board;
SelectChosenHostStep.TargetFilter.bRequireTopCard = true;
SelectChosenHostStep.TargetFilter.NameContains = "ChosenHost";
SelectAndPlayEffect.Steps.Add(SelectChosenHostStep);

FTCGEffectStep PlayCargoOnSelectedStep;
PlayCargoOnSelectedStep.StepType = ETCGEffectStepType::PlayGraveyardCardOnUnit;
PlayCargoOnSelectedStep.TargetMode = ETCGEffectTargetMode::SelectedTarget;
PlayCargoOnSelectedStep.bRequiresPreviousStepSuccess = true;
PlayCargoOnSelectedStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
PlayCargoOnSelectedStep.TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
PlayCargoOnSelectedStep.TargetFilter.bRequireTopCard = false;
PlayCargoOnSelectedStep.TargetFilter.NameContains = "Cargo";
SelectAndPlayEffect.Steps.Add(PlayCargoOnSelectedStep);

SourceDefinition->Effects.Add(SelectAndPlayEffect);

UTCG_CardDefinition* CargoDefinition = MakeDefinition(
"Debug_SelectedGraveyardPlay_Cargo",
TEXT("Debug Selected Graveyard Cargo"),
ETCGCardElement::Earth,
2);

GameState->DebugCardDefinitions.Add(SourceDefinition);
GameState->DebugCardDefinitions.Add(CargoDefinition);

const FGuid WrongHostId = AddDebugBoardUnit(
"Debug_SelectedGraveyardPlay_WrongHost",
ETCGCardElement::Water,
1,
0,
0);

const FGuid ChosenHostId = AddDebugBoardUnit(
"Debug_SelectedGraveyardPlay_ChosenHost",
ETCGCardElement::Water,
1,
0,
1);

FTCGCardInstance* SourceCard = GameState->AddCardInstanceFromDefinition(
SourceDefinition,
0,
ETCGCardLocation::Hand);
const FGuid SourceId = SourceCard ? SourceCard->CardInstanceId : FGuid();

FTCGCardInstance* CargoCard = GameState->AddCardInstanceFromDefinition(
CargoDefinition,
0,
ETCGCardLocation::Graveyard);
const FGuid CargoId = CargoCard ? CargoCard->CardInstanceId : FGuid();

if (!WrongHostId.IsValid() || !ChosenHostId.IsValid() || !SourceId.IsValid() || !CargoId.IsValid())
{
UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Selected graveyard play setup failed"));
GameState->EndMatch(ETCGMatchResult::Draw);
return;
}

const bool bPlayedSource = GameState->PlayCardToZone(
SourceId,
ATCG_GameState::GetFieldZoneId(0, 2));

const FTCGCardInstance* WrongHostAfter = GameState->FindCardInstance(WrongHostId);
const FTCGCardInstance* ChosenHostAfter = GameState->FindCardInstance(ChosenHostId);
const FTCGCardInstance* SourceAfter = GameState->FindCardInstance(SourceId);
const FTCGCardInstance* CargoAfter = GameState->FindCardInstance(CargoId);

const bool bCargoPlayedOnChosenHost =
CargoAfter
&& ChosenHostAfter
&& CargoAfter->Location == ETCGCardLocation::Board
&& ChosenHostAfter->Location == ETCGCardLocation::Board
&& CargoAfter->StackId.IsValid()
&& CargoAfter->StackId == ChosenHostAfter->StackId
&& ChosenHostAfter->StackIndex < CargoAfter->StackIndex;

const bool bCargoNotPlayedOnWrongHost =
!CargoAfter
|| !WrongHostAfter
|| CargoAfter->StackId != WrongHostAfter->StackId;

const bool bSourceStillOwnUnit =
SourceAfter
&& SourceAfter->Location == ETCGCardLocation::Board
&& SourceAfter->ZoneId == ATCG_GameState::GetFieldZoneId(0, 2)
&& SourceAfter->StackId.IsValid();

const bool bExpectedAll =
bPlayedSource
&& bCargoPlayedOnChosenHost
&& bCargoNotPlayedOnWrongHost
&& bSourceStillOwnUnit;

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: SelectedGraveyardPlay summary PlayedSource=%s CargoOnChosen=%s CargoNotOnWrong=%s SourceStillOwnUnit=%s ExpectedAll=%s"),
bPlayedSource ? TEXT("true") : TEXT("false"),
bCargoPlayedOnChosenHost ? TEXT("true") : TEXT("false"),
bCargoNotPlayedOnWrongHost ? TEXT("true") : TEXT("false"),
bSourceStillOwnUnit ? TEXT("true") : TEXT("false"),
bExpectedAll ? TEXT("true") : TEXT("false"));

GameState->EndMatch(ETCGMatchResult::Draw);
return;
}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::GraveyardMachineProtection)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Graveyard machine protection scenario start"));

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
		GameState->ClearPendingGraveyardToDeckChoice();

		GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
		{
			return Definition
				&& (Definition->CardDefinitionId == "Debug_GraveyardMachineProtection_Card"
					|| Definition->CardDefinitionId == "Debug_GraveyardMachineProtection_Machine"
					|| Definition->CardDefinitionId == "Debug_GraveyardMachineProtection_Destroyer");
		});

		auto MakeDefinition = [GameState](
			FName CardDefinitionId,
			const TCHAR* DisplayName,
			ETCGCardElement Element,
			int32 BaseAttack)
		{
			UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
			Definition->CardDefinitionId = CardDefinitionId;
			Definition->DisplayName = FText::FromString(DisplayName);
			Definition->Element = Element;
			Definition->BaseAttack = BaseAttack;
			return Definition;
		};

		UTCG_CardDefinition* ProtectionCardDefinition = MakeDefinition(
			"Debug_GraveyardMachineProtection_Card",
			TEXT("Debug Graveyard Machine Protection Card"),
			ETCGCardElement::Light,
			1);

		FTCGCardEffectRef PlayMachineFromGraveyardEffect;
		PlayMachineFromGraveyardEffect.Trigger = ETCGEffectTrigger::OnPlay;
		PlayMachineFromGraveyardEffect.bOptional = true;
		PlayMachineFromGraveyardEffect.TriggerFilter.bRequireTopCard = true;

		FTCGEffectStep PlayMachineStep;
		PlayMachineStep.StepType = ETCGEffectStepType::PlayGraveyardCardOnUnit;
		PlayMachineStep.TargetMode = ETCGEffectTargetMode::SourceCard;
		PlayMachineStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		PlayMachineStep.TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		PlayMachineStep.TargetFilter.bRequireTopCard = false;
		PlayMachineStep.TargetFilter.NameContains = "Machine";
		PlayMachineFromGraveyardEffect.Steps.Add(PlayMachineStep);

		ProtectionCardDefinition->Effects.Add(PlayMachineFromGraveyardEffect);

		FTCGCardEffectRef CannotBeDestroyedEffect;
		CannotBeDestroyedEffect.Trigger = ETCGEffectTrigger::None;

		FTCGEffectStep CannotBeDestroyedStep;
		CannotBeDestroyedStep.StepType = ETCGEffectStepType::CannotBeDestroyedByCardEffects;
		CannotBeDestroyedEffect.Steps.Add(CannotBeDestroyedStep);

		ProtectionCardDefinition->Effects.Add(CannotBeDestroyedEffect);

		UTCG_CardDefinition* MachineDefinition = MakeDefinition(
			"Debug_GraveyardMachineProtection_Machine",
			TEXT("Debug Graveyard Machine Unit"),
			ETCGCardElement::Fire,
			3);

		UTCG_CardDefinition* DestroyerDefinition = MakeDefinition(
			"Debug_GraveyardMachineProtection_Destroyer",
			TEXT("Debug Card Effect Destroyer"),
			ETCGCardElement::Dark,
			2);

		GameState->DebugCardDefinitions.Add(ProtectionCardDefinition);
		GameState->DebugCardDefinitions.Add(MachineDefinition);
		GameState->DebugCardDefinitions.Add(DestroyerDefinition);

		FTCGCardInstance* ProtectionCard = GameState->AddCardInstanceFromDefinition(
			ProtectionCardDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid ProtectionCardId = ProtectionCard ? ProtectionCard->CardInstanceId : FGuid();

		FTCGCardInstance* MachineCard = GameState->AddCardInstanceFromDefinition(
			MachineDefinition,
			0,
			ETCGCardLocation::Graveyard);
		const FGuid MachineCardId = MachineCard ? MachineCard->CardInstanceId : FGuid();

		FTCGCardInstance* DestroyerCard = GameState->AddCardInstanceFromDefinition(
			DestroyerDefinition,
			1,
			ETCGCardLocation::Board);
		const FGuid DestroyerCardId = DestroyerCard ? DestroyerCard->CardInstanceId : FGuid();

		if (!ProtectionCardId.IsValid() || !MachineCardId.IsValid() || !DestroyerCardId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Graveyard machine protection setup failed"));
			GameState->EndMatch(ETCGMatchResult::Draw);
			return;
		}

		FTCGCardInstance* DestroyerOnBoard = GameState->FindCardInstance(DestroyerCardId);
		if (DestroyerOnBoard)
		{
			DestroyerOnBoard->ZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
			DestroyerOnBoard->StackId = FGuid::NewGuid();
			DestroyerOnBoard->StackIndex = 0;
		}

		const bool bPlayedProtectionCard = GameState->PlayCardToZone(
			ProtectionCardId,
			ATCG_GameState::GetFieldZoneId(0, 0));

		const FTCGCardInstance* ProtectionAfterPlay = GameState->FindCardInstance(ProtectionCardId);
		const FTCGCardInstance* MachineAfterPlay = GameState->FindCardInstance(MachineCardId);

		const bool bMachinePlayedFromGraveyard =
			ProtectionAfterPlay
			&& MachineAfterPlay
			&& ProtectionAfterPlay->Location == ETCGCardLocation::Board
			&& MachineAfterPlay->Location == ETCGCardLocation::Board
			&& ProtectionAfterPlay->StackId.IsValid()
			&& MachineAfterPlay->StackId == ProtectionAfterPlay->StackId
			&& ProtectionAfterPlay->StackIndex < MachineAfterPlay->StackIndex;

		FTCGCardEffectRef DestroyByCardEffect;
		DestroyByCardEffect.Trigger = ETCGEffectTrigger::OnPlay;

		FTCGEffectStep DestroyStep;
		DestroyStep.StepType = ETCGEffectStepType::DestroyTargetUnitByCardEffect;
		DestroyStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
		DestroyByCardEffect.Steps.Add(DestroyStep);

		TArray<FTCGEffectChainEntry> DestroyChain;
		GameState->AddCardEffectRefToChain(
			DestroyChain,
			DestroyerCardId,
			MachineCardId,
			DestroyByCardEffect);

		const bool bDestroyAttemptResolved = GameState->ResolveEffectChain(DestroyChain);

		const FTCGCardInstance* ProtectionAfterDestroyAttempt = GameState->FindCardInstance(ProtectionCardId);
		const FTCGCardInstance* MachineAfterDestroyAttempt = GameState->FindCardInstance(MachineCardId);

		const bool bMachineStillOnBoard =
			MachineAfterDestroyAttempt
			&& MachineAfterDestroyAttempt->Location == ETCGCardLocation::Board;

		const bool bProtectionStillMaterial =
			ProtectionAfterDestroyAttempt
			&& MachineAfterDestroyAttempt
			&& ProtectionAfterDestroyAttempt->Location == ETCGCardLocation::Board
			&& ProtectionAfterDestroyAttempt->StackId == MachineAfterDestroyAttempt->StackId
			&& ProtectionAfterDestroyAttempt->StackIndex < MachineAfterDestroyAttempt->StackIndex;

		const bool bInheritedProtection =
			MachineAfterDestroyAttempt
			&& GameState->DoesUnitHaveInheritedCannotBeDestroyedByCardEffects(MachineAfterDestroyAttempt->CardInstanceId);

		const bool bExpectedAll =
			bPlayedProtectionCard
			&& bMachinePlayedFromGraveyard
			&& bDestroyAttemptResolved
			&& bMachineStillOnBoard
			&& bProtectionStillMaterial
			&& bInheritedProtection;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: GraveyardMachineProtection summary PlayedProtection=%s MachineFromGraveyard=%s DestroyAttemptResolved=%s MachineStillBoard=%s ProtectionStillMaterial=%s InheritedProtection=%s ExpectedAll=%s"),
			bPlayedProtectionCard ? TEXT("true") : TEXT("false"),
			bMachinePlayedFromGraveyard ? TEXT("true") : TEXT("false"),
			bDestroyAttemptResolved ? TEXT("true") : TEXT("false"),
			bMachineStillOnBoard ? TEXT("true") : TEXT("false"),
			bProtectionStillMaterial ? TEXT("true") : TEXT("false"),
			bInheritedProtection ? TEXT("true") : TEXT("false"),
			bExpectedAll ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::MachineMaterialRecovery)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Machine material recovery scenario start"));

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
		GameState->ClearPendingGraveyardToDeckChoice();

		GameState->DebugCardDefinitions.RemoveAll([](const TObjectPtr<UTCG_CardDefinition>& Definition)
		{
			return Definition
				&& (Definition->CardDefinitionId == "Debug_MaterialRecovery_Card"
					|| Definition->CardDefinitionId == "Debug_MachineMaterialRecovery_Machine"
					|| Definition->CardDefinitionId == "Debug_MaterialRecovery_EarthHand"
					|| Definition->CardDefinitionId == "Debug_MaterialRecovery_EarthGraveyard"
					|| Definition->CardDefinitionId == "Debug_MaterialRecovery_Host");
		});

		auto MakeDefinition = [GameState](
			FName CardDefinitionId,
			const TCHAR* DisplayName,
			ETCGCardElement Element,
			int32 BaseAttack)
		{
			UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
			Definition->CardDefinitionId = CardDefinitionId;
			Definition->DisplayName = FText::FromString(DisplayName);
			Definition->Element = Element;
			Definition->BaseAttack = BaseAttack;
			return Definition;
		};

		UTCG_CardDefinition* EffectCardDefinition = MakeDefinition(
			"Debug_MaterialRecovery_Card",
			TEXT("Debug Machine Material Recovery Card"),
			ETCGCardElement::Wind,
			1);

		FTCGCardEffectRef AttachAndPlayEarthEffect;
		AttachAndPlayEarthEffect.Trigger = ETCGEffectTrigger::OnYourUnitPlayed;
		AttachAndPlayEarthEffect.bOptional = true;
		AttachAndPlayEarthEffect.TriggerFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		AttachAndPlayEarthEffect.TriggerFilter.RequiredLocation = ETCGCardLocation::Board;
		AttachAndPlayEarthEffect.TriggerFilter.bRequireTopCard = true;
		AttachAndPlayEarthEffect.TriggerFilter.NameContains = "Machine";
		AttachAndPlayEarthEffect.bUseSourceFilter = true;
		AttachAndPlayEarthEffect.SourceFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		AttachAndPlayEarthEffect.SourceFilter.RequiredLocation = ETCGCardLocation::Hand;
		AttachAndPlayEarthEffect.SourceFilter.bRequireTopCard = false;

		FTCGEffectStep AttachStep;
		AttachStep.StepType = ETCGEffectStepType::AttachSourceToUnitMaterial;
		AttachStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
		AttachAndPlayEarthEffect.Steps.Add(AttachStep);

		FTCGEffectStep PlayEarthOnUnitStep;
		PlayEarthOnUnitStep.StepType = ETCGEffectStepType::PlayHandCardOnUnit;
		PlayEarthOnUnitStep.bRequiresPreviousStepSuccess = true;
		PlayEarthOnUnitStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		PlayEarthOnUnitStep.TargetFilter.RequiredLocation = ETCGCardLocation::Hand;
		PlayEarthOnUnitStep.TargetFilter.bRequireTopCard = false;
		PlayEarthOnUnitStep.TargetFilter.bRequireElement = true;
		PlayEarthOnUnitStep.TargetFilter.RequiredElement = ETCGCardElement::Earth;
		PlayEarthOnUnitStep.SecondaryTargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		PlayEarthOnUnitStep.SecondaryTargetFilter.RequiredLocation = ETCGCardLocation::Board;
		PlayEarthOnUnitStep.SecondaryTargetFilter.bRequireTopCard = true;
		PlayEarthOnUnitStep.SecondaryTargetFilter.NameContains = "Host";
		AttachAndPlayEarthEffect.Steps.Add(PlayEarthOnUnitStep);

		EffectCardDefinition->Effects.Add(AttachAndPlayEarthEffect);

		FTCGCardEffectRef MaterialRecoveryEffect;
		MaterialRecoveryEffect.Trigger = ETCGEffectTrigger::OnMaterialOfDestroyedUnit;
		MaterialRecoveryEffect.TriggerFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		MaterialRecoveryEffect.TriggerFilter.RequiredLocation = ETCGCardLocation::Board;
		MaterialRecoveryEffect.TriggerFilter.bRequireTopCard = true;
		MaterialRecoveryEffect.TriggerFilter.NameContains = "Machine";
		MaterialRecoveryEffect.bUseSourceFilter = true;
		MaterialRecoveryEffect.SourceFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		MaterialRecoveryEffect.SourceFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		MaterialRecoveryEffect.SourceFilter.bRequireTopCard = false;

		FTCGEffectStep RecoverEarthStep;
		RecoverEarthStep.StepType = ETCGEffectStepType::MoveGraveyardCardToHand;
		RecoverEarthStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		RecoverEarthStep.TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		RecoverEarthStep.TargetFilter.bRequireTopCard = false;
		RecoverEarthStep.TargetFilter.bRequireElement = true;
		RecoverEarthStep.TargetFilter.RequiredElement = ETCGCardElement::Earth;
		RecoverEarthStep.TargetFilter.bExcludeSourceCard = true;
		MaterialRecoveryEffect.Steps.Add(RecoverEarthStep);

		EffectCardDefinition->Effects.Add(MaterialRecoveryEffect);

		UTCG_CardDefinition* MachineDefinition = MakeDefinition(
			"Debug_MachineMaterialRecovery_Machine",
			TEXT("Debug Machine Unit"),
			ETCGCardElement::Fire,
			3);

		UTCG_CardDefinition* EarthHandDefinition = MakeDefinition(
			"Debug_MaterialRecovery_EarthHand",
			TEXT("Debug Earth Hand Unit"),
			ETCGCardElement::Earth,
			2);

		UTCG_CardDefinition* EarthGraveyardDefinition = MakeDefinition(
			"Debug_MaterialRecovery_EarthGraveyard",
			TEXT("Debug Earth Graveyard Unit"),
			ETCGCardElement::Earth,
			2);

		UTCG_CardDefinition* HostDefinition = MakeDefinition(
			"Debug_MaterialRecovery_Host",
			TEXT("Debug Host Unit"),
			ETCGCardElement::Water,
			1);

		GameState->DebugCardDefinitions.Add(EffectCardDefinition);
		GameState->DebugCardDefinitions.Add(MachineDefinition);
		GameState->DebugCardDefinitions.Add(EarthHandDefinition);
		GameState->DebugCardDefinitions.Add(EarthGraveyardDefinition);
		GameState->DebugCardDefinitions.Add(HostDefinition);

		FTCGCardInstance* HostCard = GameState->AddCardInstanceFromDefinition(
			HostDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid HostId = HostCard ? HostCard->CardInstanceId : FGuid();

		FTCGCardInstance* EffectCard = GameState->AddCardInstanceFromDefinition(
			EffectCardDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid EffectCardId = EffectCard ? EffectCard->CardInstanceId : FGuid();

		FTCGCardInstance* MachineCard = GameState->AddCardInstanceFromDefinition(
			MachineDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid MachineId = MachineCard ? MachineCard->CardInstanceId : FGuid();

		FTCGCardInstance* EarthHandCard = GameState->AddCardInstanceFromDefinition(
			EarthHandDefinition,
			0,
			ETCGCardLocation::Hand);
		const FGuid EarthHandId = EarthHandCard ? EarthHandCard->CardInstanceId : FGuid();

		FTCGCardInstance* EarthGraveyardCard = GameState->AddCardInstanceFromDefinition(
			EarthGraveyardDefinition,
			0,
			ETCGCardLocation::Graveyard);
		const FGuid EarthGraveyardId = EarthGraveyardCard ? EarthGraveyardCard->CardInstanceId : FGuid();

		if (!HostId.IsValid() || !EffectCardId.IsValid() || !MachineId.IsValid() || !EarthHandId.IsValid() || !EarthGraveyardId.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Machine material recovery setup failed"));
			GameState->EndMatch(ETCGMatchResult::Draw);
			return;
		}

		const bool bPlayedHost = GameState->PlayCardToZone(
			HostId,
			ATCG_GameState::GetFieldZoneId(0, 0));

		// Simulate the normal placement rule already using this zone this round.
		// The later effect-based overlay should still be allowed.
		GameState->Player0PlacementFieldZonesUsedThisRound.AddUnique(0);
		const bool bZoneAlreadyUsedBeforeEffect = GameState->Player0PlacementFieldZonesUsedThisRound.Contains(0);

		const bool bPlayedMachine = GameState->PlayCardToZone(
			MachineId,
			ATCG_GameState::GetFieldZoneId(0, 1));

		const FTCGCardInstance* MachineAfterPlay = GameState->FindCardInstance(MachineId);
		const FTCGCardInstance* EffectAfterMachine = GameState->FindCardInstance(EffectCardId);
		const FTCGCardInstance* EarthHandAfterEffect = GameState->FindCardInstance(EarthHandId);
		const FTCGCardInstance* HostAfterEffect = GameState->FindCardInstance(HostId);

		const bool bEffectAttachedToMachine =
			MachineAfterPlay
			&& EffectAfterMachine
			&& MachineAfterPlay->Location == ETCGCardLocation::Board
			&& EffectAfterMachine->Location == ETCGCardLocation::Board
			&& MachineAfterPlay->StackId.IsValid()
			&& EffectAfterMachine->StackId == MachineAfterPlay->StackId
			&& EffectAfterMachine->StackIndex < MachineAfterPlay->StackIndex;

		const bool bEarthPlayedOnHost =
			EarthHandAfterEffect
			&& HostAfterEffect
			&& EarthHandAfterEffect->Location == ETCGCardLocation::Board
			&& HostAfterEffect->Location == ETCGCardLocation::Board
			&& EarthHandAfterEffect->StackId.IsValid()
			&& EarthHandAfterEffect->StackId == HostAfterEffect->StackId
			&& HostAfterEffect->StackIndex < EarthHandAfterEffect->StackIndex;

		bool bDestroyedMachineStack = false;
		if (MachineAfterPlay && MachineAfterPlay->StackId.IsValid())
		{
			bDestroyedMachineStack = GameState->MoveStackToLocation(
				MachineAfterPlay->StackId,
				ETCGCardLocation::Graveyard);
		}

		const FTCGCardInstance* MachineAfterDestroy = GameState->FindCardInstance(MachineId);
		const FTCGCardInstance* EffectAfterDestroy = GameState->FindCardInstance(EffectCardId);
		const FTCGCardInstance* EarthRecoveredAfterDestroy = GameState->FindCardInstance(EarthGraveyardId);

		const bool bMachineDestroyed =
			MachineAfterDestroy
			&& MachineAfterDestroy->Location == ETCGCardLocation::Graveyard;

		const bool bEffectCardInGraveyard =
			EffectAfterDestroy
			&& EffectAfterDestroy->Location == ETCGCardLocation::Graveyard;

		const bool bEarthRecovered =
			EarthRecoveredAfterDestroy
			&& EarthRecoveredAfterDestroy->Location == ETCGCardLocation::Hand;

		const bool bExpectedAll =
			bPlayedHost
			&& bZoneAlreadyUsedBeforeEffect
			&& bPlayedMachine
			&& bEffectAttachedToMachine
			&& bEarthPlayedOnHost
			&& bDestroyedMachineStack
			&& bMachineDestroyed
			&& bEffectCardInGraveyard
			&& bEarthRecovered;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: MachineMaterialRecovery summary PlayedHost=%s ZoneAlreadyUsed=%s PlayedMachine=%s Attached=%s EarthPlayedOnHost=%s DestroyedStack=%s MachineGraveyard=%s EffectGraveyard=%s EarthRecovered=%s ExpectedAll=%s"),
			bPlayedHost ? TEXT("true") : TEXT("false"),
			bZoneAlreadyUsedBeforeEffect ? TEXT("true") : TEXT("false"),
			bPlayedMachine ? TEXT("true") : TEXT("false"),
			bEffectAttachedToMachine ? TEXT("true") : TEXT("false"),
			bEarthPlayedOnHost ? TEXT("true") : TEXT("false"),
			bDestroyedMachineStack ? TEXT("true") : TEXT("false"),
			bMachineDestroyed ? TEXT("true") : TEXT("false"),
			bEffectCardInGraveyard ? TEXT("true") : TEXT("false"),
			bEarthRecovered ? TEXT("true") : TEXT("false"),
			bExpectedAll ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::HandDetachResponse)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Hand detach response scenario start"));
		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->RoundNumber = 1;
		GameState->TurnNumber = 1;

		const FGuid VictimId = AddDebugBoardUnit("HandDetach_P0_Victim", ETCGCardElement::Earth, 1, 0, 0);
		const FGuid DestroyerId = AddDebugBoardUnit("HandDetach_P1_Destroyer", ETCGCardElement::Dark, 10, 1, 0);

		FTCGCardInstance* DestroyerCard = GameState->FindCardInstance(DestroyerId);
		if (DestroyerCard)
		{
			DestroyerCard->StackIndex = 3;
			for (int32 MaterialIndex = 0; MaterialIndex < 3; ++MaterialIndex)
			{
				FTCGCardInstance& MaterialCard = GameState->AddCardInstance(FName(*FString::Printf(TEXT("HandDetach_P1_Material_%d"), MaterialIndex)), ETCGCardElement::Water, 1, 1, ETCGCardLocation::Board);
				MaterialCard.ZoneId = DestroyerCard->ZoneId;
				MaterialCard.StackId = DestroyerCard->StackId;
				MaterialCard.StackIndex = MaterialIndex;
			}
		}

		UTCG_CardDefinition* ResponseCardDefinition = NewObject<UTCG_CardDefinition>(GameState);
		ResponseCardDefinition->CardDefinitionId = "Debug_HandDetach_Response";
		ResponseCardDefinition->DisplayName = FText::FromString(TEXT("Debug Hand Detach Response"));
		ResponseCardDefinition->Element = ETCGCardElement::Wind;
		ResponseCardDefinition->BaseAttack = 1;
		ResponseCardDefinition->Description = FText::FromString(TEXT("If your unit is destroyed, you may discard this card; detach up to 2 materials from the unit that destroyed your unit."));

		FTCGCardEffectRef ResponseEffect;
		ResponseEffect.Trigger = ETCGEffectTrigger::OnYourUnitDestroyedByBattle;
		ResponseEffect.bOptional = true;
		FTCGEffectStep DiscardStep;
		DiscardStep.StepType = ETCGEffectStepType::DiscardSource;
		ResponseEffect.Steps.Add(DiscardStep);

		FTCGEffectStep DetachStep;
		DetachStep.StepType = ETCGEffectStepType::DetachMaterials;
		DetachStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
		DetachStep.Value = 2;
		DetachStep.bAllowPartialSuccess = true;
		DetachStep.bRequiresPreviousStepSuccess = true;
		ResponseEffect.Steps.Add(DetachStep);
		ResponseCardDefinition->Effects.Add(ResponseEffect);
		GameState->DebugCardDefinitions.Add(ResponseCardDefinition);

		FTCGCardInstance* ResponseCard = GameState->AddCardInstanceFromDefinition(ResponseCardDefinition, 0, ETCGCardLocation::Hand);
		const int32 MaterialsBefore = CountMaterialsUnderUnit(DestroyerId);

		const bool bResolved = GameState->ResolveBattlePhase();
		const FTCGCardInstance* VictimAfterBattle = GameState->FindCardInstance(VictimId);
		const FTCGCardInstance* ResponseAfterBattle = ResponseCard ? GameState->FindCardInstance(ResponseCard->CardInstanceId) : nullptr;
		const int32 MaterialsAfter = CountMaterialsUnderUnit(DestroyerId);

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: Hand detach response summary Resolved=%s VictimGraveyard=%s ResponseDiscarded=%s MaterialsBefore=%d MaterialsAfter=%d Detached=%d ExpectedDetached=2"),
			bResolved ? TEXT("true") : TEXT("false"),
			(VictimAfterBattle && VictimAfterBattle->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"),
			(ResponseAfterBattle && ResponseAfterBattle->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"),
			MaterialsBefore,
			MaterialsAfter,
			MaterialsBefore - MaterialsAfter);

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::WraparoundBattle)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Wraparound battle scenario start"));
		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Battle);
		AddDebugBoardUnit("Wrap_P0_Field0", ETCGCardElement::Fire, 1, 0, 0);
		AddDebugBoardUnit("Wrap_P0_Field2", ETCGCardElement::Earth, 6, 0, 2);
		AddDebugBoardUnit("Wrap_P1_Field1", ETCGCardElement::Dark, 5, 1, 1);
		AddDebugBoardUnit("Wrap_P1_Field3", ETCGCardElement::Light, 2, 1, 3);
		const bool bResolved = GameState->ResolveBattlePhase();
		const ETCGMatchResult ScenarioResult = GameState->CheckLoseConditionAfterBattle();
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Wraparound battle scenario summary Resolved=%s P0Board=%s P1Board=%s Result=%s"),
			bResolved ? TEXT("true") : TEXT("false"),
			GameState->DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
			GameState->DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
			GetTCGDebugRunnerMatchResultDebugName(ScenarioResult));
		GameState->EndMatch(ScenarioResult);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::RoundLimitTiebreak)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit tiebreak scenario start Max=%d"), GameState->MaxRoundNumber);
		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->RoundNumber = FMath::Max(1, GameState->MaxRoundNumber);
		GameState->TurnNumber = GameState->RoundNumber;
		AddDebugBoardUnit("Limit_P0_Field0", ETCGCardElement::Fire, 6, 0, 0);
		AddDebugBoardUnit("Limit_P0_Field1", ETCGCardElement::Earth, 6, 0, 1);
		AddDebugBoardUnit("Limit_P0_Field2", ETCGCardElement::Light, 6, 0, 2);
		AddDebugBoardUnit("Limit_P1_Field0", ETCGCardElement::Dark, 1, 1, 0);
		AddDebugBoardUnit("Limit_P1_Field1", ETCGCardElement::Water, 10, 1, 1);
		AddDebugBoardUnit("Limit_P1_Field2", ETCGCardElement::Wind, 1, 1, 2);
		const bool bResolved = GameState->ResolveBattlePhase();
		ETCGMatchResult ScenarioResult = GameState->CheckLoseConditionAfterBattle();
		if (ScenarioResult == ETCGMatchResult::None && GameState->RoundNumber >= FMath::Max(1, GameState->MaxRoundNumber))
		{
			ScenarioResult = GetRoundLimitResult();
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit reached R%d Max=%d P0Units=%d P1Units=%d Result=%s"), GameState->RoundNumber, GameState->MaxRoundNumber, CountBoardUnits(0), CountBoardUnits(1), GetTCGDebugRunnerMatchResultDebugName(ScenarioResult));
		}
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit tiebreak scenario summary Resolved=%s P0Board=%s P1Board=%s Result=%s"),
			bResolved ? TEXT("true") : TEXT("false"),
			GameState->DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
			GameState->DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
			GetTCGDebugRunnerMatchResultDebugName(ScenarioResult));
		GameState->EndMatch(ScenarioResult);
		return;
	}

	if (DebugRunnerScenario == ETCGDebugRunnerScenario::OverlayPlacement)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement scenario start"));
		GameState->MatchCards.Empty();
		GameState->SetMatchResult(ETCGMatchResult::None);
		GameState->SetPhase(ETCGMatchPhase::Placement);
		GameState->RoundNumber = 2;
		GameState->TurnNumber = GameState->RoundNumber;
		GameState->PlacementStepIndex = 0;
		GameState->Player0PlacementFieldZonesUsedThisRound.Reset();
		GameState->Player1PlacementFieldZonesUsedThisRound.Reset();
		GameState->SetCurrentTurnPlayer(0);

		AddDebugBoardUnit("Overlay_P0_Field0", ETCGCardElement::Fire, 1, 0, 0);
		AddDebugBoardUnit("Overlay_P0_Field1", ETCGCardElement::Earth, 1, 0, 1);
		AddDebugBoardUnit("Overlay_P0_Field2", ETCGCardElement::Fire, 1, 0, 2);
		AddDebugBoardUnit("Overlay_P0_Field3", ETCGCardElement::Water, 1, 0, 3);

		FTCGCardInstance& FirstOverlayCard = GameState->AddCardInstance("Overlay_P0_First_Fire", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
		const FName OverlayZoneId = ATCG_GameState::GetFieldZoneId(0, 2);
		const bool bFirstOverlaySuccess = GameState->PlayerPlayCardToZone(0, FirstOverlayCard.CardInstanceId, OverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement first attempt Field=2 Type=Overlay Success=%s Expected=true UsedZones=%d"), bFirstOverlaySuccess ? TEXT("true") : TEXT("false"), GameState->Player0PlacementFieldZonesUsedThisRound.Num());

		GameState->PlacementStepIndex = 2;
		GameState->SetCurrentTurnPlayer(0);

		FTCGCardInstance& SecondOverlayCard = GameState->AddCardInstance("Overlay_P0_Second_Fire", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
		const bool bSecondOverlaySuccess = GameState->PlayerPlayCardToZone(0, SecondOverlayCard.CardInstanceId, OverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement same-zone retry Field=2 Type=Overlay Success=%s Expected=false UsedZones=%d"), bSecondOverlaySuccess ? TEXT("true") : TEXT("false"), GameState->Player0PlacementFieldZonesUsedThisRound.Num());

		GameState->PlacementStepIndex = 2;
		GameState->SetCurrentTurnPlayer(0);

		FTCGCardInstance& ThirdOverlayCard = GameState->AddCardInstance("Overlay_P0_Third_Earth", ETCGCardElement::Earth, 2, 0, ETCGCardLocation::Hand);
		const FName DifferentOverlayZoneId = ATCG_GameState::GetFieldZoneId(0, 1);
		const bool bDifferentZoneOverlaySuccess = GameState->PlayerPlayCardToZone(0, ThirdOverlayCard.CardInstanceId, DifferentOverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement different-zone attempt Field=1 Type=Overlay Success=%s Expected=true UsedZones=%d"), bDifferentZoneOverlaySuccess ? TEXT("true") : TEXT("false"), GameState->Player0PlacementFieldZonesUsedThisRound.Num());

		GameState->EndMatch(ETCGMatchResult::Draw);
		return;
	}

	const int32 Player0InitialDraw = GameState->DrawCards(0, GameState->InitialHandSize);
	const int32 Player1InitialDraw = GameState->DrawCards(1, GameState->InitialHandSize);
	if (bDebugRunnerLogRoundFlow)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Starting hand draw P0=%d P1=%d Target=%d"), Player0InitialDraw, Player1InitialDraw, GameState->InitialHandSize);
	}

	while (!GameState->IsMatchOver())
	{
		if (GameState->HasPendingDiscardChoice())
		{
			if (bDebugRunnerLogRoundFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round flow paused PendingDiscard Player=%d Count=%d"), GameState->PendingDiscardChoice.PlayerIndex, GameState->PendingDiscardChoice.RequiredCount);
			}
			return;
		}

		if (bDebugRunnerLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round %d start"), GameState->RoundNumber);

		GameState->SetPhase(ETCGMatchPhase::Placement);
		GameState->PlacementStepIndex = 0;
		GameState->Player0PlacementFieldZonesUsedThisRound.Reset();
		GameState->Player1PlacementFieldZonesUsedThisRound.Reset();
		GameState->SetCurrentTurnPlayer(GameState->GetPlacementStepPlayer());

		while (GameState->CurrentPhase == ETCGMatchPhase::Placement && !GameState->IsPlacementPhaseComplete())
		{
			if (GameState->HasPendingDiscardChoice())
			{
				if (bDebugRunnerLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement loop paused PendingDiscard Player=%d Count=%d"), GameState->PendingDiscardChoice.PlayerIndex, GameState->PendingDiscardChoice.RequiredCount);
				}
				return;
			}

			const int32 PlayerIndex = GameState->GetPlacementStepPlayer();
			const int32 DrawnCount = GameState->DrawCards(PlayerIndex, GameState->CardsDrawnPerPlacementStep);

			bool bPlayed = false;
			int32 PlayedFieldIndex = INDEX_NONE;
			FString PlacementType = TEXT("None");

			TArray<FTCGCardInstance> HandCards;
			GameState->GetCardsInHand(PlayerIndex, HandCards);
			for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount && !bPlayed; ++FieldIndex)
			{
				const FName ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
				for (const FTCGCardInstance& HandCard : HandCards)
				{
					if (!GameState->CanPlayerPlayCardToZone(PlayerIndex, HandCard.CardInstanceId, ZoneId)) continue;

					FGuid ExistingStackId;
					PlacementType = GameState->FindStackIdInZone(ZoneId, ExistingStackId) ? TEXT("Overlay") : TEXT("NewStack");
					bPlayed = GameState->PlayerPlayCardToZone(PlayerIndex, HandCard.CardInstanceId, ZoneId);
					PlayedFieldIndex = FieldIndex;
					break;
				}
			}

			if (!bPlayed)
			{
				if (bDebugRunnerLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d Draw=%d skipped no legal placement"), GameState->RoundNumber, GameState->PlacementStepIndex + 1, PlayerIndex, DrawnCount);
				}
				GameState->AdvancePlacementStep();
				continue;
			}

			if (bDebugRunnerLogPlacementFlow)
			{
				const TArray<int32>& UsedZones = PlayerIndex == 0 ? GameState->Player0PlacementFieldZonesUsedThisRound : GameState->Player1PlacementFieldZonesUsedThisRound;
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d Draw=%d Field=%d Type=%s UsedZones=%d Success=true"),
					GameState->RoundNumber,
					GameState->PlacementStepIndex,
					PlayerIndex,
					DrawnCount,
					PlayedFieldIndex,
					*PlacementType,
					UsedZones.Num());
			}

			if (GameState->HasPendingDiscardChoice())
			{
				if (bDebugRunnerLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement loop paused after play PendingDiscard Player=%d Count=%d"), GameState->PendingDiscardChoice.PlayerIndex, GameState->PendingDiscardChoice.RequiredCount);
				}
				return;
			}
		}

		if (GameState->CurrentPhase == ETCGMatchPhase::Battle)
		{
			if (GameState->HasPendingDiscardChoice())
			{
				if (bDebugRunnerLogRoundFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle start paused PendingDiscard Player=%d Count=%d"), GameState->PendingDiscardChoice.PlayerIndex, GameState->PendingDiscardChoice.RequiredCount);
				}
				return;
			}

			if (bDebugRunnerLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase started R%d"), GameState->RoundNumber);
			const bool bResolved = GameState->ResolveBattlePhase();
			ETCGMatchResult ResultAfterBattle = GameState->CheckLoseConditionAfterBattle();

			if (ResultAfterBattle == ETCGMatchResult::None && GameState->RoundNumber >= FMath::Max(1, GameState->MaxRoundNumber))
			{
				ResultAfterBattle = GetRoundLimitResult();
				if (bDebugRunnerLogRoundFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit reached R%d Max=%d P0Units=%d P1Units=%d Result=%s"), GameState->RoundNumber, GameState->MaxRoundNumber, CountBoardUnits(0), CountBoardUnits(1), GetTCGDebugRunnerMatchResultDebugName(ResultAfterBattle));
				}
			}

			if (bDebugRunnerLogRoundFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle summary R%d Resolved=%s P0Board=%s P1Board=%s Result=%s"),
					GameState->RoundNumber,
					bResolved ? TEXT("true") : TEXT("false"),
					GameState->DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
					GameState->DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
					GetTCGDebugRunnerMatchResultDebugName(ResultAfterBattle));
			}

			if (ResultAfterBattle != ETCGMatchResult::None)
			{
				GameState->EndMatch(ResultAfterBattle);
				if (bDebugRunnerLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Match ended after battle"));
				return;
			}
		}

		GameState->RoundNumber++;
		GameState->TurnNumber = GameState->RoundNumber;
	}
}
