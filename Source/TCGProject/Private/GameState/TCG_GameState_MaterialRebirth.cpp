#include "GameState/TCG_GameState.h"

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
}
