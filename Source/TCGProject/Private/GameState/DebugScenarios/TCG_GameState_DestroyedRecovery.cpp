#include "GameState/TCG_GameState.h"

bool ATCG_GameState::MoveGraveyardCardsToHandAndTopDeck(const FGuid& ToHandCardInstanceId, const FGuid& ToDeckCardInstanceId)
{
	if (!ToHandCardInstanceId.IsValid() || !ToDeckCardInstanceId.IsValid() || ToHandCardInstanceId == ToDeckCardInstanceId) return false;

	FTCGCardInstance* ToHandCard = FindCardInstance(ToHandCardInstanceId);
	FTCGCardInstance* ToDeckCard = FindCardInstance(ToDeckCardInstanceId);
	if (!ToHandCard || !ToDeckCard) return false;
	if (ToHandCard->Location != ETCGCardLocation::Graveyard || ToDeckCard->Location != ETCGCardLocation::Graveyard) return false;

	const bool bMovedToHand = MoveCardToLocation(ToHandCardInstanceId, ETCGCardLocation::Hand);
	const bool bMovedToDeck = MoveCardToTopOfDeck(ToDeckCardInstanceId);
	return bMovedToHand && bMovedToDeck;
}

bool ATCG_GameState::RemoveMaterialFromUnit(const FGuid& TargetCardInstanceId, const FGuid& MaterialCardInstanceId)
{
	FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
	FTCGCardInstance* MaterialCard = FindCardInstance(MaterialCardInstanceId);
	if (!TargetCard || !MaterialCard) return false;
	if (TargetCard->Location != ETCGCardLocation::Board || MaterialCard->Location != ETCGCardLocation::Board) return false;
	if (!TargetCard->StackId.IsValid() || TargetCard->StackId != MaterialCard->StackId) return false;
	if (MaterialCard->StackIndex >= TargetCard->StackIndex) return false;

	const FGuid StackId = TargetCard->StackId;
	const int32 RemovedStackIndex = MaterialCard->StackIndex;
	if (!MoveCardToLocation(MaterialCardInstanceId, ETCGCardLocation::Graveyard)) return false;

	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == StackId && Card.StackIndex > RemovedStackIndex)
		{
			Card.StackIndex--;
		}
	}

	return true;
}

bool ATCG_GameState::BeginPendingGraveyardCardsToHandAndTopDeckChoice(int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingGraveyardCardsToHandAndTopDeckChoice.bIsPending) return false;

	const int32 OwnerPlayerIndex = TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent ? 1 - PlayerIndex : PlayerIndex;

	PendingGraveyardCardsToHandAndTopDeckChoice.Reset();
	PendingGraveyardCardsToHandAndTopDeckChoice.bIsPending = true;
	PendingGraveyardCardsToHandAndTopDeckChoice.PlayerIndex = PlayerIndex;
	PendingGraveyardCardsToHandAndTopDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingGraveyardCardsToHandAndTopDeckChoice.ChainIndex = ChainEntry.ChainIndex;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != OwnerPlayerIndex) continue;
		if (Card.Location != ETCGCardLocation::Graveyard) continue;
		if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
		if (TargetFilter.bExcludeSourceCard && Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
		PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	}

	if (PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds.Num() < 2)
	{
		ClearPendingGraveyardCardsToHandAndTopDeckChoice();
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending graveyard hand/topdeck choice started Player=%d Cards=%d"), PlayerIndex, PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds.Num());
	return true;
}

bool ATCG_GameState::SubmitPendingGraveyardCardsToHandAndTopDeckChoice(int32 PlayerIndex, const FGuid& ChosenToHandCardInstanceId, const FGuid& ChosenToDeckCardInstanceId)
{
	if (!PendingGraveyardCardsToHandAndTopDeckChoice.bIsPending || PendingGraveyardCardsToHandAndTopDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenToHandCardInstanceId.IsValid() || !ChosenToDeckCardInstanceId.IsValid() || ChosenToHandCardInstanceId == ChosenToDeckCardInstanceId) return false;
	if (!PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds.Contains(ChosenToHandCardInstanceId)) return false;
	if (!PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds.Contains(ChosenToDeckCardInstanceId)) return false;

	const bool bMoved = MoveGraveyardCardsToHandAndTopDeck(ChosenToHandCardInstanceId, ChosenToDeckCardInstanceId);
	if (!bMoved) return false;

	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending graveyard hand/topdeck choice submitted Player=%d"), PlayerIndex);
	ClearPendingGraveyardCardsToHandAndTopDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingGraveyardCardsToHandAndTopDeckChoice() const { return PendingGraveyardCardsToHandAndTopDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingGraveyardCardsToHandAndTopDeckOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingGraveyardCardsToHandAndTopDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingGraveyardCardsToHandAndTopDeckChoice() { PendingGraveyardCardsToHandAndTopDeckChoice.Reset(); }

bool ATCG_GameState::BeginPendingRemoveMaterialChoice(int32 PlayerIndex, const FGuid& TargetCardInstanceId, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingRemoveMaterialChoice.bIsPending) return false;

	const FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
	if (!TargetCard || TargetCard->Location != ETCGCardLocation::Board || !TargetCard->StackId.IsValid()) return false;

	PendingRemoveMaterialChoice.Reset();
	PendingRemoveMaterialChoice.bIsPending = true;
	PendingRemoveMaterialChoice.PlayerIndex = PlayerIndex;
	PendingRemoveMaterialChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingRemoveMaterialChoice.TargetCardInstanceId = TargetCardInstanceId;
	PendingRemoveMaterialChoice.ChainIndex = ChainEntry.ChainIndex;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.StackId != TargetCard->StackId) continue;
		if (Card.StackIndex >= TargetCard->StackIndex) continue;
		PendingRemoveMaterialChoice.EligibleMaterialCardInstanceIds.Add(Card.CardInstanceId);
	}

	if (PendingRemoveMaterialChoice.EligibleMaterialCardInstanceIds.Num() <= 0)
	{
		ClearPendingRemoveMaterialChoice();
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending remove material choice started Player=%d Materials=%d"), PlayerIndex, PendingRemoveMaterialChoice.EligibleMaterialCardInstanceIds.Num());
	return true;
}

bool ATCG_GameState::SubmitPendingRemoveMaterialChoice(int32 PlayerIndex, const FGuid& ChosenMaterialCardInstanceId)
{
	if (!PendingRemoveMaterialChoice.bIsPending || PendingRemoveMaterialChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenMaterialCardInstanceId.IsValid() || !PendingRemoveMaterialChoice.EligibleMaterialCardInstanceIds.Contains(ChosenMaterialCardInstanceId)) return false;

	const bool bRemoved = RemoveMaterialFromUnit(PendingRemoveMaterialChoice.TargetCardInstanceId, ChosenMaterialCardInstanceId);
	if (!bRemoved) return false;

	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending remove material choice submitted Player=%d"), PlayerIndex);
	ClearPendingRemoveMaterialChoice();
	return true;
}

bool ATCG_GameState::HasPendingRemoveMaterialChoice() const { return PendingRemoveMaterialChoice.bIsPending; }
void ATCG_GameState::GetPendingRemoveMaterialChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingRemoveMaterialChoice.EligibleMaterialCardInstanceIds; }
void ATCG_GameState::ClearPendingRemoveMaterialChoice() { PendingRemoveMaterialChoice.Reset(); }

void ATCG_GameState::RunDebugDestroyedRecoveryScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: DestroyedRecoverySplit scenario start"));

	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Battle);
	SetMatchResult(ETCGMatchResult::None);

	const FGuid DestroyedSourceId = AddCardInstance("Debug_Water_DestroyedRecovery_Source", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
	const FGuid WaterToHandId = AddCardInstance("Debug_Water_DestroyedRecovery_Hand", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
	const FGuid WaterToDeckId = AddCardInstance("Debug_Water_DestroyedRecovery_Deck", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
	const FGuid EarthIgnoredId = AddCardInstance("Debug_Earth_DestroyedRecovery_Ignored", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;

	const FGuid DestroyerStackId = FGuid::NewGuid();
	const FGuid DestroyerMaterialAId = AddCardInstance("Debug_Water_Destroyer_Material_A", ETCGCardElement::Water, 1, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* DestroyerMaterialA = FindCardInstance(DestroyerMaterialAId))
	{
		DestroyerMaterialA->ZoneId = GetFieldZoneId(1, 0);
		DestroyerMaterialA->StackId = DestroyerStackId;
		DestroyerMaterialA->StackIndex = 0;
	}

	const FGuid DestroyerMaterialBId = AddCardInstance("Debug_Fire_Destroyer_Material_B", ETCGCardElement::Fire, 1, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* DestroyerMaterialB = FindCardInstance(DestroyerMaterialBId))
	{
		DestroyerMaterialB->ZoneId = GetFieldZoneId(1, 0);
		DestroyerMaterialB->StackId = DestroyerStackId;
		DestroyerMaterialB->StackIndex = 1;
	}

	const FGuid DestroyerTopId = AddCardInstance("Debug_Dark_Destroyer_Top", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* DestroyerTop = FindCardInstance(DestroyerTopId))
	{
		DestroyerTop->ZoneId = GetFieldZoneId(1, 0);
		DestroyerTop->StackId = DestroyerStackId;
		DestroyerTop->StackIndex = 2;
	}

	FTCGEffectTargetFilter GraveyardFilter;
	GraveyardFilter.OwnerMode = ETCGEffectTargetMode::Controller;
	GraveyardFilter.RequiredLocation = ETCGCardLocation::Graveyard;
	GraveyardFilter.bRequireElement = true;
	GraveyardFilter.RequiredElement = ETCGCardElement::Water;
	GraveyardFilter.bExcludeSourceCard = true;

	FTCGCardEffectRef SplitEffect;
	SplitEffect.Trigger = ETCGEffectTrigger::OnDestroyed;
	SplitEffect.bOptional = true;

	FTCGEffectStep MoveToHandStep;
	MoveToHandStep.StepType = ETCGEffectStepType::MoveGraveyardCardToHand;
	MoveToHandStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	MoveToHandStep.TargetFilter = GraveyardFilter;
	SplitEffect.Steps.Add(MoveToHandStep);

	FTCGEffectStep MoveToTopDeckStep;
	MoveToTopDeckStep.StepType = ETCGEffectStepType::MoveGraveyardCardToTopDeck;
	MoveToTopDeckStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	MoveToTopDeckStep.TargetFilter = GraveyardFilter;
	MoveToTopDeckStep.bRequiresPreviousStepSuccess = true;
	SplitEffect.Steps.Add(MoveToTopDeckStep);

	FTCGEffectStep RemoveMaterialStep;
	RemoveMaterialStep.StepType = ETCGEffectStepType::RemoveMaterialFromTargetUnit;
	RemoveMaterialStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
	RemoveMaterialStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
	SplitEffect.Steps.Add(RemoveMaterialStep);

	TArray<FTCGEffectChainEntry> Chain;
	const bool bChainAdded = AddCardEffectRefToChain(Chain, DestroyedSourceId, DestroyerTopId, SplitEffect);
	const bool bChainResolved = ResolveEffectChain(Chain);

	const FTCGCardInstance* SourceAfter = FindCardInstance(DestroyedSourceId);
	const FTCGCardInstance* HandAfter = FindCardInstance(WaterToHandId);
	const FTCGCardInstance* DeckAfter = FindCardInstance(WaterToDeckId);
	const FTCGCardInstance* EarthAfter = FindCardInstance(EarthIgnoredId);
	const FTCGCardInstance* DestroyerAfter = FindCardInstance(DestroyerTopId);

	const bool bSourceStillGraveyard = SourceAfter && SourceAfter->Location == ETCGCardLocation::Graveyard;
	const bool bChosenToHand = HandAfter && HandAfter->Location == ETCGCardLocation::Hand;
	const bool bChosenToDeck = DeckAfter && DeckAfter->Location == ETCGCardLocation::Deck;
	const bool bEarthStillGraveyard = EarthAfter && EarthAfter->Location == ETCGCardLocation::Graveyard;
	const bool bDestroyerStillBoard = DestroyerAfter && DestroyerAfter->Location == ETCGCardLocation::Board;
	const int32 RemainingMaterials = GetCardsUnderneathCount(DestroyerTopId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: DestroyedRecoverySplit summary ChainAdded=%s ChainResolved=%s SourceStillGraveyard=%s ChosenToHand=%s ChosenToDeck=%s EarthStillGraveyard=%s DestroyerStillBoard=%s RemainingMaterials=%d"),
		bChainAdded ? TEXT("true") : TEXT("false"),
		bChainResolved ? TEXT("true") : TEXT("false"),
		bSourceStillGraveyard ? TEXT("true") : TEXT("false"),
		bChosenToHand ? TEXT("true") : TEXT("false"),
		bChosenToDeck ? TEXT("true") : TEXT("false"),
		bEarthStillGraveyard ? TEXT("true") : TEXT("false"),
		bDestroyerStillBoard ? TEXT("true") : TEXT("false"),
		RemainingMaterials);

	EndMatch(ETCGMatchResult::Draw);
	RunDebugMaterialRebirthScenario();
}
