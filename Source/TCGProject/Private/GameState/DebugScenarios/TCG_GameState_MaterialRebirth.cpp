#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
	void RunDebugAttackMillGraveyardSplitScenario(ATCG_GameState* GameState)
	{
		if (!GameState) return;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackMillGraveyardSplit scenario start"));

		GameState->MatchCards.Empty();
		GameState->StartMatch();
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->SetMatchResult(ETCGMatchResult::None);

		const FGuid AttackerId = GameState->AddCardInstance("Debug_Water_AttackMill_Attacker", ETCGCardElement::Water, 3, 0, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* Attacker = GameState->FindCardInstance(AttackerId))
		{
			Attacker->ZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
			Attacker->StackId = FGuid::NewGuid();
			Attacker->StackIndex = 0;
		}

		const FGuid WaterToHandId = GameState->AddCardInstance("Debug_Water_AttackMill_ToHand", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
		const FGuid WaterToDeckId = GameState->AddCardInstance("Debug_Water_AttackMill_ToDeck", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
		const FGuid EarthIgnoredId = GameState->AddCardInstance("Debug_Earth_AttackMill_Ignored", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
		const FGuid MilledSourceId = GameState->AddCardInstance("Debug_Water_AttackMill_MilledSource", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck).CardInstanceId;

		FTCGCardEffectRef AttackMillEffect;
		AttackMillEffect.Trigger = ETCGEffectTrigger::OnAttack;
		FTCGEffectStep MillStep;
		MillStep.StepType = ETCGEffectStepType::SendTopDeckCardToGraveyard;
		AttackMillEffect.Steps.Add(MillStep);

		TArray<FTCGEffectChainEntry> AttackChain;
		const bool bAttackChainAdded = GameState->AddCardEffectRefToChain(AttackChain, AttackerId, AttackerId, AttackMillEffect);
		const bool bAttackChainResolved = GameState->ResolveEffectChain(AttackChain);

		const FTCGCardInstance* MilledAfterAttack = GameState->FindCardInstance(MilledSourceId);
		const bool bMilledSourceSentToGraveyard = MilledAfterAttack && MilledAfterAttack->Location == ETCGCardLocation::Graveyard;

		FTCGEffectTargetFilter WaterGraveyardFilter;
		WaterGraveyardFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		WaterGraveyardFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		WaterGraveyardFilter.bRequireElement = true;
		WaterGraveyardFilter.RequiredElement = ETCGCardElement::Water;
		WaterGraveyardFilter.bExcludeSourceCard = false;

		FTCGCardEffectRef GraveyardSplitEffect;
		GraveyardSplitEffect.Trigger = ETCGEffectTrigger::OnSentFromDeckToGraveyard;
		GraveyardSplitEffect.bOptional = true;

		FTCGEffectStep SplitStep;
		SplitStep.StepType = ETCGEffectStepType::MoveGraveyardCardsToHandAndTopDeck;
		SplitStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
		SplitStep.TargetFilter = WaterGraveyardFilter;
		GraveyardSplitEffect.Steps.Add(SplitStep);

		TArray<FTCGEffectChainEntry> SplitChain;
		const bool bSplitChainAdded = GameState->AddCardEffectRefToChain(SplitChain, MilledSourceId, MilledSourceId, GraveyardSplitEffect);
		const bool bSplitChainResolved = GameState->ResolveEffectChain(SplitChain);

		const FTCGCardInstance* WaterToHandAfter = GameState->FindCardInstance(WaterToHandId);
		const FTCGCardInstance* WaterToDeckAfter = GameState->FindCardInstance(WaterToDeckId);
		const FTCGCardInstance* EarthAfter = GameState->FindCardInstance(EarthIgnoredId);
		const FTCGCardInstance* MilledSourceAfter = GameState->FindCardInstance(MilledSourceId);

		const bool bChosenToHand = WaterToHandAfter && WaterToHandAfter->Location == ETCGCardLocation::Hand;
		const bool bChosenToTopDeck = WaterToDeckAfter && WaterToDeckAfter->Location == ETCGCardLocation::Deck;
		const bool bEarthStillGraveyard = EarthAfter && EarthAfter->Location == ETCGCardLocation::Graveyard;
		const bool bMilledSourceStillGraveyard = MilledSourceAfter && MilledSourceAfter->Location == ETCGCardLocation::Graveyard;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: AttackMillGraveyardSplit summary AttackChainAdded=%s AttackChainResolved=%s MilledSourceSentToGraveyard=%s SplitChainAdded=%s SplitChainResolved=%s ChosenToHand=%s ChosenToTopDeck=%s EarthStillGraveyard=%s MilledSourceStillGraveyard=%s"),
			bAttackChainAdded ? TEXT("true") : TEXT("false"),
			bAttackChainResolved ? TEXT("true") : TEXT("false"),
			bMilledSourceSentToGraveyard ? TEXT("true") : TEXT("false"),
			bSplitChainAdded ? TEXT("true") : TEXT("false"),
			bSplitChainResolved ? TEXT("true") : TEXT("false"),
			bChosenToHand ? TEXT("true") : TEXT("false"),
			bChosenToTopDeck ? TEXT("true") : TEXT("false"),
			bEarthStillGraveyard ? TEXT("true") : TEXT("false"),
			bMilledSourceStillGraveyard ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
	}

	void RunDebugAttackMillBounceScenario(ATCG_GameState* GameState)
	{
		if (!GameState) return;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackMillBounce scenario start"));

		GameState->MatchCards.Empty();
		GameState->StartMatch();
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->SetMatchResult(ETCGMatchResult::None);

		const FGuid AttackerStackId = FGuid::NewGuid();
		const FName AttackerZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
		const FGuid AttackerMaterialId = GameState->AddCardInstance("Debug_Water_AttackMillBounce_Material", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* AttackerMaterial = GameState->FindCardInstance(AttackerMaterialId))
		{
			AttackerMaterial->ZoneId = AttackerZoneId;
			AttackerMaterial->StackId = AttackerStackId;
			AttackerMaterial->StackIndex = 0;
		}

		const FGuid AttackerTopId = GameState->AddCardInstance("Debug_Fire_AttackMillBounce_Attacker", ETCGCardElement::Fire, 1, 0, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* AttackerTop = GameState->FindCardInstance(AttackerTopId))
		{
			AttackerTop->ZoneId = AttackerZoneId;
			AttackerTop->StackId = AttackerStackId;
			AttackerTop->StackIndex = 1;
		}

		const FGuid DefenderStackId = FGuid::NewGuid();
		const FName DefenderZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
		const FGuid DefenderMaterialId = GameState->AddCardInstance("Debug_Earth_AttackMillBounce_DefenderMaterial", ETCGCardElement::Earth, 1, 1, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* DefenderMaterial = GameState->FindCardInstance(DefenderMaterialId))
		{
			DefenderMaterial->ZoneId = DefenderZoneId;
			DefenderMaterial->StackId = DefenderStackId;
			DefenderMaterial->StackIndex = 0;
		}

		const FGuid DefenderTopId = GameState->AddCardInstance("Debug_Dark_AttackMillBounce_Defender", ETCGCardElement::Dark, 99, 1, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* DefenderTop = GameState->FindCardInstance(DefenderTopId))
		{
			DefenderTop->ZoneId = DefenderZoneId;
			DefenderTop->StackId = DefenderStackId;
			DefenderTop->StackIndex = 1;
		}

		const FGuid FirstMilledId = GameState->AddCardInstance("Debug_Water_AttackMillBounce_MillA", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck).CardInstanceId;
		const FGuid SecondMilledId = GameState->AddCardInstance("Debug_Water_AttackMillBounce_MillB", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck).CardInstanceId;

		FTCGCardEffectRef BounceEffect;
		BounceEffect.Trigger = ETCGEffectTrigger::OnAttack;

		FTCGEffectStep BounceStep;
		BounceStep.StepType = ETCGEffectStepType::AttackMillTwoWaterBounceBattlingUnit;
		BounceEffect.Steps.Add(BounceStep);

		TArray<FTCGEffectChainEntry> Chain;
		const bool bChainAdded = GameState->AddCardEffectRefToChain(Chain, AttackerTopId, DefenderTopId, BounceEffect);
		const bool bChainResolved = GameState->ResolveEffectChain(Chain);

		const FTCGCardInstance* AttackerMaterialAfter = GameState->FindCardInstance(AttackerMaterialId);
		const FTCGCardInstance* FirstMilledAfter = GameState->FindCardInstance(FirstMilledId);
		const FTCGCardInstance* SecondMilledAfter = GameState->FindCardInstance(SecondMilledId);
		const FTCGCardInstance* DefenderMaterialAfter = GameState->FindCardInstance(DefenderMaterialId);
		const FTCGCardInstance* DefenderTopAfter = GameState->FindCardInstance(DefenderTopId);

		const bool bAttackerMaterialToGraveyard = AttackerMaterialAfter && AttackerMaterialAfter->Location == ETCGCardLocation::Graveyard;
		const bool bFirstMilledToGraveyard = FirstMilledAfter && FirstMilledAfter->Location == ETCGCardLocation::Graveyard;
		const bool bSecondMilledToGraveyard = SecondMilledAfter && SecondMilledAfter->Location == ETCGCardLocation::Graveyard;
		const bool bDefenderMaterialToGraveyard = DefenderMaterialAfter && DefenderMaterialAfter->Location == ETCGCardLocation::Graveyard;
		const bool bDefenderTopToDeck = DefenderTopAfter && DefenderTopAfter->Location == ETCGCardLocation::Deck;
		const bool bDefenderLeftBoardBeforeDamage = DefenderTopAfter && DefenderTopAfter->Location != ETCGCardLocation::Board;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: AttackMillBounce summary ChainAdded=%s ChainResolved=%s AttackerMaterialToGraveyard=%s FirstMilledToGraveyard=%s SecondMilledToGraveyard=%s DefenderMaterialToGraveyard=%s DefenderTopToDeck=%s DefenderLeftBoardBeforeDamage=%s"),
			bChainAdded ? TEXT("true") : TEXT("false"),
			bChainResolved ? TEXT("true") : TEXT("false"),
			bAttackerMaterialToGraveyard ? TEXT("true") : TEXT("false"),
			bFirstMilledToGraveyard ? TEXT("true") : TEXT("false"),
			bSecondMilledToGraveyard ? TEXT("true") : TEXT("false"),
			bDefenderMaterialToGraveyard ? TEXT("true") : TEXT("false"),
			bDefenderTopToDeck ? TEXT("true") : TEXT("false"),
			bDefenderLeftBoardBeforeDamage ? TEXT("true") : TEXT("false"));

		GameState->EndMatch(ETCGMatchResult::Draw);
	}

	int32 CountDebugMaterialsUnderUnit(ATCG_GameState* GameState, const FGuid& TopCardId)
	{
		if (!GameState) return 0;

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
	}

	void RunDebugHandDetachResponseScenario(ATCG_GameState* GameState)
	{
		if (!GameState) return;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: HandDetachResponse scenario start"));

		GameState->MatchCards.Empty();
		GameState->StartMatch();
		GameState->SetPhase(ETCGMatchPhase::Battle);
		GameState->SetMatchResult(ETCGMatchResult::None);

		const FGuid VictimId = GameState->AddCardInstance("Debug_HandDetach_Victim", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* Victim = GameState->FindCardInstance(VictimId))
		{
			Victim->ZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
			Victim->StackId = FGuid::NewGuid();
			Victim->StackIndex = 0;
		}

		const FGuid DestroyerStackId = FGuid::NewGuid();
		const FName DestroyerZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
		for (int32 MaterialIndex = 0; MaterialIndex < 3; ++MaterialIndex)
		{
			const FGuid MaterialId = GameState->AddCardInstance(FName(*FString::Printf(TEXT("Debug_HandDetach_DestroyerMaterial_%d"), MaterialIndex)), ETCGCardElement::Water, 1, 1, ETCGCardLocation::Board).CardInstanceId;
			if (FTCGCardInstance* MaterialCard = GameState->FindCardInstance(MaterialId))
			{
				MaterialCard->ZoneId = DestroyerZoneId;
				MaterialCard->StackId = DestroyerStackId;
				MaterialCard->StackIndex = MaterialIndex;
			}
		}

		const FGuid DestroyerId = GameState->AddCardInstance("Debug_HandDetach_Destroyer", ETCGCardElement::Dark, 10, 1, ETCGCardLocation::Board).CardInstanceId;
		if (FTCGCardInstance* Destroyer = GameState->FindCardInstance(DestroyerId))
		{
			Destroyer->ZoneId = DestroyerZoneId;
			Destroyer->StackId = DestroyerStackId;
			Destroyer->StackIndex = 3;
		}

		UTCG_CardDefinition* ResponseCardDefinition = NewObject<UTCG_CardDefinition>(GameState);
		ResponseCardDefinition->CardDefinitionId = "Debug_HandDetach_Response";
		ResponseCardDefinition->Element = ETCGCardElement::Wind;
		ResponseCardDefinition->BaseAttack = 1;

		FTCGCardEffectRef ResponseEffect;
		ResponseEffect.Trigger = ETCGEffectTrigger::OnYourUnitDestroyedByBattle;
		ResponseEffect.bOptional = true;

		FTCGEffectStep ResponseStep;
		ResponseStep.StepType = ETCGEffectStepType::DiscardSourceDetachUpToTwoMaterialsFromTarget;
		ResponseEffect.Steps.Add(ResponseStep);
		ResponseCardDefinition->Effects.Add(ResponseEffect);
		GameState->DebugCardDefinitions.Add(ResponseCardDefinition);

		FTCGCardInstance* ResponseCard = GameState->AddCardInstanceFromDefinition(ResponseCardDefinition, 0, ETCGCardLocation::Hand);
		const FGuid ResponseCardId = ResponseCard ? ResponseCard->CardInstanceId : FGuid();
		const int32 MaterialsBefore = CountDebugMaterialsUnderUnit(GameState, DestroyerId);

		const bool bBattleResolved = GameState->ResolveBattlePhase();

		const FTCGCardInstance* VictimAfter = GameState->FindCardInstance(VictimId);
		const FTCGCardInstance* ResponseAfter = GameState->FindCardInstance(ResponseCardId);
		const int32 MaterialsAfter = CountDebugMaterialsUnderUnit(GameState, DestroyerId);

		const bool bVictimGraveyard = VictimAfter && VictimAfter->Location == ETCGCardLocation::Graveyard;
		const bool bResponseDiscarded = ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Graveyard;
		const int32 DetachedCount = MaterialsBefore - MaterialsAfter;

		UE_LOG(LogTemp, Warning,
			TEXT("TCG Debug: HandDetachResponse summary Battle=%s VictimGraveyard=%s ResponseDiscarded=%s Materials=%d/%d Detached=%d Expected=2"),
			bBattleResolved ? TEXT("true") : TEXT("false"),
			bVictimGraveyard ? TEXT("true") : TEXT("false"),
			bResponseDiscarded ? TEXT("true") : TEXT("false"),
			MaterialsBefore,
			MaterialsAfter,
			DetachedCount);

		GameState->EndMatch(ETCGMatchResult::Draw);
	}
}

void ATCG_GameState::RunDebugMaterialRebirthScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: MaterialRebirth scenario start"));

	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Battle);
	SetMatchResult(ETCGMatchResult::None);

	const FGuid GraveyardToDeckId = AddCardInstance("Debug_Wind_MaterialRebirth_ToDeck", ETCGCardElement::Wind, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;

	const FGuid DestroyedStackId = FGuid::NewGuid();
	const FName DestroyedZoneId = GetFieldZoneId(0, 0);

	const FGuid MaterialSourceId = AddCardInstance("Debug_Water_MaterialRebirth_Source", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* MaterialSource = FindCardInstance(MaterialSourceId))
	{
		MaterialSource->ZoneId = DestroyedZoneId;
		MaterialSource->StackId = DestroyedStackId;
		MaterialSource->StackIndex = 0;
	}

	const FGuid DestroyedTopId = AddCardInstance("Debug_Fire_MaterialRebirth_DestroyedTop", ETCGCardElement::Fire, 5, 0, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* DestroyedTop = FindCardInstance(DestroyedTopId))
	{
		DestroyedTop->ZoneId = DestroyedZoneId;
		DestroyedTop->StackId = DestroyedStackId;
		DestroyedTop->StackIndex = 1;
	}

	const FTCGCardInstance* SourceBeforeDestroy = FindCardInstance(MaterialSourceId);
	const bool bSourceWasMaterial = SourceBeforeDestroy
		&& SourceBeforeDestroy->Location == ETCGCardLocation::Board
		&& SourceBeforeDestroy->StackId == DestroyedStackId
		&& SourceBeforeDestroy->StackIndex < 1;

	const bool bStackMovedToGraveyard = MoveStackToLocation(DestroyedStackId, ETCGCardLocation::Graveyard);

	FTCGEffectTargetFilter GraveyardUnitFilter;
	GraveyardUnitFilter.OwnerMode = ETCGEffectTargetMode::Controller;
	GraveyardUnitFilter.RequiredLocation = ETCGCardLocation::Graveyard;
	GraveyardUnitFilter.bExcludeSourceCard = true;

	FTCGCardEffectRef MaterialRebirthEffect;
	MaterialRebirthEffect.Trigger = ETCGEffectTrigger::OnSentFromMaterialToGraveyard;
	MaterialRebirthEffect.bOptional = true;

	FTCGEffectStep MoveToTopDeckStep;
	MoveToTopDeckStep.StepType = ETCGEffectStepType::MoveGraveyardCardToTopDeck;
	MoveToTopDeckStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	MoveToTopDeckStep.TargetFilter = GraveyardUnitFilter;
	MaterialRebirthEffect.Steps.Add(MoveToTopDeckStep);

	FTCGEffectStep PlaySelfStep;
	PlaySelfStep.StepType = ETCGEffectStepType::PlaySourceToEmptyZone;
	PlaySelfStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	PlaySelfStep.bRequiresPreviousStepSuccess = true;
	MaterialRebirthEffect.Steps.Add(PlaySelfStep);

	TArray<FTCGEffectChainEntry> Chain;
	const bool bChainAdded = AddCardEffectRefToChain(Chain, MaterialSourceId, DestroyedTopId, MaterialRebirthEffect);
	const bool bChainResolved = ResolveEffectChain(Chain);

	const FTCGCardInstance* SourceAfter = FindCardInstance(MaterialSourceId);
	const FTCGCardInstance* DeckAfter = FindCardInstance(GraveyardToDeckId);
	const FTCGCardInstance* DestroyedTopAfter = FindCardInstance(DestroyedTopId);

	const bool bChosenToTopDeck = DeckAfter && DeckAfter->Location == ETCGCardLocation::Deck;
	const bool bSourcePlayedToBoard = SourceAfter && SourceAfter->Location == ETCGCardLocation::Board && SourceAfter->ZoneId == DestroyedZoneId && SourceAfter->StackIndex == 0;
	const bool bDestroyedTopStillGraveyard = DestroyedTopAfter && DestroyedTopAfter->Location == ETCGCardLocation::Graveyard;
	const bool bSourceLeftGraveyard = SourceAfter && SourceAfter->Location != ETCGCardLocation::Graveyard;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: MaterialRebirth summary SourceWasMaterial=%s StackMovedToGraveyard=%s ChainAdded=%s ChainResolved=%s ChosenToTopDeck=%s SourcePlayedToBoard=%s SourceLeftGraveyard=%s DestroyedTopStillGraveyard=%s"),
		bSourceWasMaterial ? TEXT("true") : TEXT("false"),
		bStackMovedToGraveyard ? TEXT("true") : TEXT("false"),
		bChainAdded ? TEXT("true") : TEXT("false"),
		bChainResolved ? TEXT("true") : TEXT("false"),
		bChosenToTopDeck ? TEXT("true") : TEXT("false"),
		bSourcePlayedToBoard ? TEXT("true") : TEXT("false"),
		bSourceLeftGraveyard ? TEXT("true") : TEXT("false"),
		bDestroyedTopStillGraveyard ? TEXT("true") : TEXT("false"));

	EndMatch(ETCGMatchResult::Draw);

	RunDebugAttackMillGraveyardSplitScenario(this);
	RunDebugAttackMillBounceScenario(this);
	RunDebugHandDetachResponseScenario(this);
}