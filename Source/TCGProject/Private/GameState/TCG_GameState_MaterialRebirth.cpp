#include "GameState/TCG_GameState.h"

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
}
