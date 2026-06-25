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
		HandDetachResponse
	};

	constexpr ETCGDebugRunnerScenario DebugRunnerScenario = ETCGDebugRunnerScenario::HandDetachResponse;
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
