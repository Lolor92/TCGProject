#include "GameState/TCG_GameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
	enum class ETCGDebugScenario : uint8
	{
		NormalRoundFlow,
		WraparoundBattle,
		RoundLimitTiebreak,
		OverlayPlacement
	};

	constexpr ETCGDebugScenario DebugScenario = ETCGDebugScenario::NormalRoundFlow;
	constexpr bool bEnableDebugOverlayRemovalFizzleTest = false;
	constexpr bool bLogDebugSetup = false;
	constexpr bool bLogCardSetupDetails = false;
	constexpr bool bLogRoundFlow = true;
	constexpr bool bLogPlacementFlow = true;
	constexpr bool bLogBattleFlow = false;
	constexpr bool bLogEffectChains = true;
	constexpr bool bLogVerboseCardTriggers = false;

	const FName DebugCard_FireDeckA = "Debug_Fire_Deck_A";
	const FName DebugCard_FireDeckB = "Debug_Fire_Deck_B";
	const FName DebugEffect_Draw1 = "Debug_Draw1";
	const FName DebugEffect_GainAttackForCardsUnderneath = "Debug_GainAttackForCardsUnderneath";
	const FName DebugEffect_RemoveBottomOverlay = "Debug_RemoveBottomOverlay";

	static const TCHAR* GetTCGEffectTriggerDebugName(ETCGEffectTrigger Trigger)
	{
		switch (Trigger)
		{
		case ETCGEffectTrigger::OnPlay: return TEXT("OnPlay");
		case ETCGEffectTrigger::OnDestroyed: return TEXT("OnDestroyed");
		case ETCGEffectTrigger::OnSentToGraveyard: return TEXT("OnSentToGraveyard");
		case ETCGEffectTrigger::OnBanished: return TEXT("OnBanished");
		case ETCGEffectTrigger::OnBattleStart: return TEXT("OnBattleStart");
		case ETCGEffectTrigger::OnBattleEnd: return TEXT("OnBattleEnd");
		case ETCGEffectTrigger::OnStackAdded: return TEXT("OnStackAdded");
		case ETCGEffectTrigger::OnBecomingTopCard: return TEXT("OnBecomingTopCard");
		default: return TEXT("None");
		}
	}

	static const TCHAR* GetTCGMatchResultDebugName(ETCGMatchResult Result)
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

ATCG_GameState::ATCG_GameState()
{
	bReplicates = true;
}

void ATCG_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATCG_GameState, TurnNumber);
	DOREPLIFETIME(ATCG_GameState, RoundNumber);
	DOREPLIFETIME(ATCG_GameState, PlacementStepIndex);
	DOREPLIFETIME(ATCG_GameState, Player0PlacementFieldZonesUsedThisRound);
	DOREPLIFETIME(ATCG_GameState, Player1PlacementFieldZonesUsedThisRound);
	DOREPLIFETIME(ATCG_GameState, CurrentTurnPlayerIndex);
	DOREPLIFETIME(ATCG_GameState, CurrentPhase);
	DOREPLIFETIME(ATCG_GameState, MatchResult);
	DOREPLIFETIME(ATCG_GameState, MatchCards);
	DOREPLIFETIME(ATCG_GameState, PendingDiscardChoice);
	DOREPLIFETIME(ATCG_GameState, PendingGraveyardToDeckChoice);
}

FName ATCG_GameState::GetFieldZoneId(int32 PlayerIndex, int32 FieldIndex)
{
	return FName(*FString::Printf(TEXT("Player%d_Field_%d"), PlayerIndex, FieldIndex));
}

void ATCG_GameState::StartMatch()
{
	TurnNumber = 1;
	RoundNumber = 1;
	PlacementStepIndex = 0;
	Player0PlacementFieldZonesUsedThisRound.Reset();
	Player1PlacementFieldZonesUsedThisRound.Reset();
	CurrentTurnPlayerIndex = 0;
	CurrentPhase = ETCGMatchPhase::RoundStart;
	MatchResult = ETCGMatchResult::None;
	ClearPendingDiscardChoice();
	ClearPendingGraveyardToDeckChoice();
}

void ATCG_GameState::SetPhase(ETCGMatchPhase NewPhase)
{
	CurrentPhase = NewPhase;
}

void ATCG_GameState::SetMatchResult(ETCGMatchResult NewMatchResult)
{
	MatchResult = NewMatchResult;
}

void ATCG_GameState::EndMatch(ETCGMatchResult FinalResult)
{
	SetMatchResult(FinalResult);
	SetPhase(ETCGMatchPhase::GameOver);
}

bool ATCG_GameState::IsMatchOver() const
{
	return MatchResult != ETCGMatchResult::None;
}

void ATCG_GameState::SetCurrentTurnPlayer(int32 NewPlayerIndex)
{
	CurrentTurnPlayerIndex = NewPlayerIndex;
}

int32 ATCG_GameState::GetPlacementStepPlayer() const
{
	return PlacementStepIndex % 2;
}

int32 ATCG_GameState::GetMaxPlacementStepCount() const
{
	return PlacementStepsPerPlayer * 2;
}

bool ATCG_GameState::IsPlacementPhaseComplete() const
{
	return PlacementStepIndex >= GetMaxPlacementStepCount();
}

bool ATCG_GameState::CanPlayerActInPlacementStep(int32 PlayerIndex) const
{
	return !HasPendingDiscardChoice()
		&& IsValidPlayerIndex(PlayerIndex)
		&& CurrentPhase == ETCGMatchPhase::Placement
		&& !IsPlacementPhaseComplete()
		&& GetPlacementStepPlayer() == PlayerIndex;
}

int32 ATCG_GameState::GetCompletedPlacementStepsForPlayer(int32 PlayerIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return 0;

	int32 CompletedSteps = 0;
	for (int32 StepIndex = 0; StepIndex < PlacementStepIndex; ++StepIndex)
	{
		if (StepIndex % 2 == PlayerIndex) CompletedSteps++;
	}
	return CompletedSteps;
}

int32 ATCG_GameState::GetNextRequiredFieldZoneIndex(int32 PlayerIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return INDEX_NONE;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		FGuid ExistingStackId;
		if (!FindStackIdInZone(GetFieldZoneId(PlayerIndex, FieldIndex), ExistingStackId)) return FieldIndex;
	}
	return INDEX_NONE;
}

bool ATCG_GameState::CanPlayerPlaceInFieldZone(int32 PlayerIndex, int32 FieldIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex) || FieldIndex < 0 || FieldIndex >= FieldZoneCount) return false;

	const TArray<int32>& UsedZones = PlayerIndex == 0 ? Player0PlacementFieldZonesUsedThisRound : Player1PlacementFieldZonesUsedThisRound;
	return !UsedZones.Contains(FieldIndex);
}

bool ATCG_GameState::AdvancePlacementStep()
{
	if (HasPendingDiscardChoice())
	{
		if (bLogPlacementFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement advance paused PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
		}
		return false;
	}

	if (CurrentPhase != ETCGMatchPhase::Placement || IsPlacementPhaseComplete()) return false;

	PlacementStepIndex++;
	if (IsPlacementPhaseComplete())
	{
		CurrentTurnPlayerIndex = INDEX_NONE;
		SetPhase(ETCGMatchPhase::Battle);
		return true;
	}

	SetCurrentTurnPlayer(GetPlacementStepPlayer());
	return true;
}

FTCGCardInstance& ATCG_GameState::AddCardInstance(FName CardDefinitionId, ETCGCardElement Element, int32 BaseAttack,
	int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation)
{
	FTCGCardInstance NewCard;
	NewCard.CardDefinitionId = CardDefinitionId;
	NewCard.Element = Element;
	NewCard.BaseAttack = BaseAttack;
	NewCard.OwnerPlayerIndex = OwnerPlayerIndex;
	NewCard.Location = StartingLocation;
	NewCard.LocationIndex = GetNextLocationIndex(OwnerPlayerIndex, StartingLocation);
	NewCard.ZoneId = NAME_None;
	NewCard.StackId.Invalidate();
	NewCard.StackIndex = INDEX_NONE;
	return MatchCards.Add_GetRef(NewCard);
}

FTCGCardInstance* ATCG_GameState::AddCardInstanceFromDefinition(const UTCG_CardDefinition* CardDefinition, int32 OwnerPlayerIndex,
	ETCGCardLocation StartingLocation)
{
	if (!CardDefinition) return nullptr;
	return &AddCardInstance(CardDefinition->CardDefinitionId, CardDefinition->Element, CardDefinition->BaseAttack, OwnerPlayerIndex, StartingLocation);
}

FTCGCardInstance& ATCG_GameState::AddDebugCardInstance(FName CardDefinitionId, ETCGCardElement FallbackElement,
	int32 FallbackBaseAttack, int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation)
{
	if (const UTCG_CardDefinition* CardDefinition = FindDebugCardDefinitionById(CardDefinitionId))
	{
		if (FTCGCardInstance* CardInstance = AddCardInstanceFromDefinition(CardDefinition, OwnerPlayerIndex, StartingLocation))
		{
			if (bLogCardSetupDetails)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Added card %s P%d ATK %d Effects %d"), *CardInstance->CardDefinitionId.ToString(), OwnerPlayerIndex, CardInstance->BaseAttack, CardDefinition->Effects.Num());
			}
			return *CardInstance;
		}
	}

	FTCGCardInstance& CardInstance = AddCardInstance(CardDefinitionId, FallbackElement, FallbackBaseAttack, OwnerPlayerIndex, StartingLocation);
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Added fallback card %s P%d ATK %d"), *CardInstance.CardDefinitionId.ToString(), OwnerPlayerIndex, CardInstance.BaseAttack);
	return CardInstance;
}

FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId)
{
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId) return &Card;
	}
	return nullptr;
}

const FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId) const
{
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId) return &Card;
	}
	return nullptr;
}

FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId)
{
	if (!StackId.IsValid()) return nullptr;

	FTCGCardInstance* TopCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.StackId != StackId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}
	return TopCard;
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId) const
{
	if (!StackId.IsValid()) return nullptr;

	const FTCGCardInstance* TopCard = nullptr;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.StackId != StackId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}
	return TopCard;
}

bool ATCG_GameState::CanPlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId) const
{
	const FTCGCardInstance* CardToPlace = FindCardInstance(CardInstanceId);
	const FTCGCardInstance* TopCard = FindTopCardInStack(TargetStackId);
	return CardToPlace && TopCard && CardToPlace->Element == TopCard->Element;
}

bool ATCG_GameState::PlaceCardAsNewStack(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = ETCGCardLocation::Board;
	Card->ZoneId = ZoneId;
	Card->StackId = FGuid::NewGuid();
	Card->StackIndex = 0;
	return true;
}

bool ATCG_GameState::PlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId)
{
	if (!CanPlaceCardOnStack(CardInstanceId, TargetStackId)) return false;

	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	FTCGCardInstance* TopCard = FindTopCardInStack(TargetStackId);
	if (!Card || !TopCard) return false;

	Card->Location = ETCGCardLocation::Board;
	Card->ZoneId = TopCard->ZoneId;
	Card->StackId = TargetStackId;
	Card->StackIndex = TopCard->StackIndex + 1;
	return true;
}

bool ATCG_GameState::PlayCardToZone(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Card->Location != ETCGCardLocation::Hand) return false;

	bool bPlayed = false;
	FGuid ExistingStackId;
	if (!FindStackIdInZone(ZoneId, ExistingStackId))
	{
		bPlayed = PlaceCardAsNewStack(CardInstanceId, ZoneId);
	}
	else
	{
		bPlayed = PlaceCardOnStack(CardInstanceId, ExistingStackId);
		if (bPlayed) ExecuteCardTrigger(CardInstanceId, ETCGEffectTrigger::OnStackAdded);
	}

	if (!bPlayed) return false;

	ExecuteCardTrigger(CardInstanceId, ETCGEffectTrigger::OnBecomingTopCard);

	TArray<FTCGEffectChainEntry> Chain;
	BuildStackOnPlayEffectChain(CardInstanceId, Chain);
	ResolveEffectChain(Chain);
	return true;
}

bool ATCG_GameState::IsValidPlayerIndex(int32 PlayerIndex) const
{
	return PlayerIndex == 0 || PlayerIndex == 1;
}

bool ATCG_GameState::IsCurrentTurnPlayer(int32 PlayerIndex) const
{
	return IsValidPlayerIndex(PlayerIndex) && CurrentTurnPlayerIndex == PlayerIndex;
}

bool ATCG_GameState::CanPlayerActInCurrentPhase(int32 PlayerIndex) const
{
	if (IsMatchOver() || HasPendingDiscardChoice()) return false;
	return CanPlayerActInPlacementStep(PlayerIndex);
}

bool ATCG_GameState::IsFieldZoneForPlayer(FName ZoneId, int32 PlayerIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex) || ZoneId.IsNone()) return false;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		if (ZoneId == GetFieldZoneId(PlayerIndex, FieldIndex)) return true;
	}
	return false;
}

bool ATCG_GameState::CanPlayerPlayCardToZone(int32 PlayerIndex, const FGuid& CardInstanceId, FName ZoneId) const
{
	if (!CanPlayerActInCurrentPhase(PlayerIndex)) return false;
	if (!IsFieldZoneForPlayer(ZoneId, PlayerIndex)) return false;

	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Hand) return false;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		if (ZoneId != GetFieldZoneId(PlayerIndex, FieldIndex)) continue;
		if (!CanPlayerPlaceInFieldZone(PlayerIndex, FieldIndex)) return false;

		FGuid ExistingStackId;
		if (!FindStackIdInZone(ZoneId, ExistingStackId)) return true;

		const bool bPlayerHasAnyEmptyFieldZone = GetNextRequiredFieldZoneIndex(PlayerIndex) != INDEX_NONE;
		return !bPlayerHasAnyEmptyFieldZone && CanPlaceCardOnStack(CardInstanceId, ExistingStackId);
	}
	return false;
}

bool ATCG_GameState::PlayerPlayCardToZone(int32 PlayerIndex, const FGuid& CardInstanceId, FName ZoneId)
{
	if (!CanPlayerPlayCardToZone(PlayerIndex, CardInstanceId, ZoneId))
	{
		const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: PlayerPlayCardToZone rejected Player=%d Card=%s Zone=%s Phase=%d PlacementStep=%d CurrentPlayer=%d PendingChoice=%s"),
			PlayerIndex,
			Card ? *Card->CardDefinitionId.ToString() : TEXT("None"),
			*ZoneId.ToString(),
			static_cast<int32>(CurrentPhase),
			PlacementStepIndex,
			CurrentTurnPlayerIndex,
			HasPendingDiscardChoice() ? TEXT("true") : TEXT("false"));
		return false;
	}

	const bool bPlayed = PlayCardToZone(CardInstanceId, ZoneId);
	if (bPlayed)
	{
		for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
		{
			if (ZoneId != GetFieldZoneId(PlayerIndex, FieldIndex)) continue;

			TArray<int32>& UsedZones = PlayerIndex == 0 ? Player0PlacementFieldZonesUsedThisRound : Player1PlacementFieldZonesUsedThisRound;
			UsedZones.AddUnique(FieldIndex);
			break;
		}

		if (HasPendingDiscardChoice())
		{
			if (bLogPlacementFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement paused for discard choice Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
			}
		}
		else
		{
			AdvancePlacementStep();
		}
	}
	return bPlayed;
}

bool ATCG_GameState::ExecuteCardTrigger(const FGuid& CardInstanceId, ETCGEffectTrigger Trigger)
{
	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Trigger == ETCGEffectTrigger::None) return false;

	if (bLogVerboseCardTriggers)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Trigger %s on %s"), GetTCGEffectTriggerDebugName(Trigger), *Card->CardDefinitionId.ToString());
	}
	return true;
}

bool ATCG_GameState::DoesCardEffectMatchTrigger(const FTCGCardEffectRef& EffectRef, ETCGEffectTrigger Trigger) const
{
	return Trigger != ETCGEffectTrigger::None
		&& EffectRef.Trigger == Trigger
		&& (!EffectRef.EffectId.IsNone() || EffectRef.Steps.Num() > 0);
}

const UTCG_CardDefinition* ATCG_GameState::FindDebugCardDefinitionById(FName CardDefinitionId) const
{
	if (CardDefinitionId.IsNone()) return nullptr;

	for (const TObjectPtr<UTCG_CardDefinition>& CardDefinition : DebugCardDefinitions)
	{
		if (CardDefinition && CardDefinition->CardDefinitionId == CardDefinitionId) return CardDefinition.Get();
	}
	return nullptr;
}

bool ATCG_GameState::HasDebugCardDefinition(FName CardDefinitionId) const
{
	return FindDebugCardDefinitionById(CardDefinitionId) != nullptr;
}

bool ATCG_GameState::ValidateDebugCardDefinitions() const
{
	bool bValid = true;
	TSet<FName> SeenIds;

	for (const TObjectPtr<UTCG_CardDefinition>& CardDefinition : DebugCardDefinitions)
	{
		if (!CardDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: DebugCardDefinitions contains null asset"));
			bValid = false;
			continue;
		}

		if (CardDefinition->CardDefinitionId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Debug card definition asset has None CardDefinitionId"));
			bValid = false;
			continue;
		}

		if (SeenIds.Contains(CardDefinition->CardDefinitionId))
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Duplicate debug card definition asset id %s"), *CardDefinition->CardDefinitionId.ToString());
			bValid = false;
			continue;
		}

		SeenIds.Add(CardDefinition->CardDefinitionId);
	}

	const FName RequiredDebugCards[] =
	{
		"Debug_Earth_Deck_A",
		"Debug_Earth_Deck_B",
		DebugCard_FireDeckA,
		DebugCard_FireDeckB,
		"Debug_Dark_Deck_A",
		"Debug_Dark_Deck_B",
		"Debug_Light_Deck_A"
	};

	for (const FName& RequiredDebugCard : RequiredDebugCards)
	{
		if (!HasDebugCardDefinition(RequiredDebugCard))
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Missing debug card definition asset %s"), *RequiredDebugCard.ToString());
			bValid = false;
		}
	}

	if (bLogDebugSetup)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Debug card definition validation: %s assigned: %d"), bValid ? TEXT("valid") : TEXT("invalid"), DebugCardDefinitions.Num());
	}
	return bValid;
}

int32 ATCG_GameState::GetPrintedEffectRefsForCard(const FTCGCardInstance& Card, TArray<FTCGCardEffectRef>& OutEffectRefs) const
{
	OutEffectRefs.Reset();

	if (const UTCG_CardDefinition* CardDefinition = FindDebugCardDefinitionById(Card.CardDefinitionId))
	{
		OutEffectRefs = CardDefinition->Effects;
		return OutEffectRefs.Num();
	}
	return 0;
}

int32 ATCG_GameState::GetPrintedEffectsForCardTrigger(const FTCGCardInstance& Card, ETCGEffectTrigger Trigger, TArray<FName>& OutEffectIds) const
{
	OutEffectIds.Reset();

	TArray<FTCGCardEffectRef> EffectRefs;
	GetPrintedEffectRefsForCard(Card, EffectRefs);
	for (const FTCGCardEffectRef& EffectRef : EffectRefs)
	{
		if (DoesCardEffectMatchTrigger(EffectRef, Trigger) && !EffectRef.EffectId.IsNone()) OutEffectIds.Add(EffectRef.EffectId);
	}
	return OutEffectIds.Num();
}

bool ATCG_GameState::AddCardTriggerToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId,
	const FGuid& TargetCardInstanceId, ETCGEffectTrigger Trigger, FName EffectId)
{
	FTCGCardEffectRef LegacyEffectRef;
	LegacyEffectRef.Trigger = Trigger;
	LegacyEffectRef.EffectId = EffectId;
	return AddCardEffectRefToChain(Chain, SourceCardInstanceId, TargetCardInstanceId, LegacyEffectRef);
}

void ATCG_GameState::ApplyDebugEffectChainEntryRequirements(FTCGEffectChainEntry& ChainEntry) const
{
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	const FTCGCardInstance* TargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);
	if (!SourceCard || !TargetCard) return;

	ChainEntry.bRequiresSourceOnBoard = true;
	ChainEntry.bRequiresTargetOnBoard = true;
	ChainEntry.bRequiresSourceInTargetStack = false;
	ChainEntry.bRequiresSourceUnderTarget = false;

	if (ChainEntry.EffectId == DebugEffect_Draw1 && SourceCard->CardInstanceId != TargetCard->CardInstanceId)
	{
		ChainEntry.bRequiresSourceInTargetStack = true;
		ChainEntry.bRequiresSourceUnderTarget = true;
	}
}

bool ATCG_GameState::CanResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry) const
{
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	const FTCGCardInstance* TargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);

	if (!SourceCard || !TargetCard) return false;
	if (ChainEntry.bRequiresSourceOnBoard && SourceCard->Location != ETCGCardLocation::Board) return false;
	if (ChainEntry.bRequiresTargetOnBoard && TargetCard->Location != ETCGCardLocation::Board) return false;
	if (ChainEntry.bRequiresSourceInTargetStack && (!SourceCard->StackId.IsValid() || SourceCard->StackId != TargetCard->StackId)) return false;
	if (ChainEntry.bRequiresSourceUnderTarget && (SourceCard->CardInstanceId == TargetCard->CardInstanceId || SourceCard->StackIndex >= TargetCard->StackIndex)) return false;
	return true;
}

int32 ATCG_GameState::BuildStackOnPlayEffectChain(const FGuid& TopCardInstanceId, TArray<FTCGEffectChainEntry>& OutChain)
{
	OutChain.Reset();
	const FTCGCardInstance* TopCard = FindCardInstance(TopCardInstanceId);
	if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid()) return 0;

	TArray<FTCGCardInstance> StackCards;
	GetCardsInStack(TopCard->StackId, StackCards);
	for (const FTCGCardInstance& StackCard : StackCards)
	{
		if (StackCard.StackIndex > TopCard->StackIndex) continue;
		ExecuteCardTrigger(StackCard.CardInstanceId, ETCGEffectTrigger::OnPlay);

		TArray<FTCGCardEffectRef> EffectRefs;
		GetPrintedEffectRefsForCard(StackCard, EffectRefs);
		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnPlay))
			{
				AddCardEffectRefToChain(OutChain, StackCard.CardInstanceId, TopCardInstanceId, EffectRef);
			}
		}
	}

	if (bEnableDebugOverlayRemovalFizzleTest && TopCard->CardDefinitionId == DebugCard_FireDeckA && OutChain.Num() >= 2)
	{
		FTCGCardEffectRef DebugEffectRef;
		DebugEffectRef.Trigger = ETCGEffectTrigger::OnBecomingTopCard;
		DebugEffectRef.EffectId = DebugEffect_RemoveBottomOverlay;
		AddCardEffectRefToChain(OutChain, TopCard->CardInstanceId, TopCardInstanceId, DebugEffectRef);
	}
	return OutChain.Num();
}

bool ATCG_GameState::ResolveEffectChain(const TArray<FTCGEffectChainEntry>& Chain)
{
	if (Chain.Num() <= 0) return false;

	if (bLogEffectChains) UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain resolving count=%d"), Chain.Num());
	bool bResolvedAny = false;
	for (int32 ChainIndex = Chain.Num() - 1; ChainIndex >= 0; --ChainIndex)
	{
		const FTCGEffectChainEntry& Entry = Chain[ChainIndex];
		if (bLogEffectChains)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain resolve %d Source=%s EffectId=%s Steps=%d"),
				Entry.ChainIndex,
				*Entry.SourceCardDefinitionId.ToString(),
				Entry.EffectId.IsNone() ? TEXT("None") : *Entry.EffectId.ToString(),
				Entry.EffectRef.Steps.Num());
		}

		if (!CanResolveEffectChainEntry(Entry))
		{
			if (bLogEffectChains)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain fizzle %d Source=%s EffectId=%s"),
					Entry.ChainIndex,
					*Entry.SourceCardDefinitionId.ToString(),
					Entry.EffectId.IsNone() ? TEXT("None") : *Entry.EffectId.ToString());
			}
			continue;
		}

		bResolvedAny |= ResolveEffectChainEntry(Entry);
	}
	return bResolvedAny;
}

bool ATCG_GameState::ResolveDebugEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Deprecated debug resolver ignored Chain=%d Source=%s"), ChainEntry.ChainIndex, *ChainEntry.SourceCardDefinitionId.ToString());
	return false;
}

bool ATCG_GameState::MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = NewLocation;
	Card->LocationIndex = GetNextLocationIndex(Card->OwnerPlayerIndex, NewLocation);
	if (NewLocation != ETCGCardLocation::Board)
	{
		Card->ZoneId = NAME_None;
		Card->StackId.Invalidate();
		Card->StackIndex = INDEX_NONE;
	}
	return true;
}

bool ATCG_GameState::MoveStackToLocation(const FGuid& StackId, ETCGCardLocation NewLocation)
{
	if (!StackId.IsValid()) return false;

	TArray<FGuid> CardIdsInStack;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.StackId == StackId && Card.Location == ETCGCardLocation::Board) CardIdsInStack.Add(Card.CardInstanceId);
	}
	if (CardIdsInStack.Num() <= 0) return false;

	bool bMovedAllCards = true;
	for (const FGuid& CardId : CardIdsInStack)
	{
		bMovedAllCards &= MoveCardToLocation(CardId, NewLocation);
	}
	return bMovedAllCards;
}

bool ATCG_GameState::DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const
{
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Board) return true;
	}
	return false;
}

int32 ATCG_GameState::GetCardsUnderneathCount(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* TargetCard = FindCardInstance(CardInstanceId);
	if (!TargetCard || !TargetCard->StackId.IsValid() || TargetCard->StackIndex <= 0) return 0;

	int32 Count = 0;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == TargetCard->StackId && Card.StackIndex < TargetCard->StackIndex) Count++;
	}
	return Count;
}

int32 ATCG_GameState::GetFinalAttack(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return 0;

	int32 FinalAttack = Card->BaseAttack + Card->AttackModifier + GetCardsUnderneathCount(CardInstanceId);

	if (!Card->StackId.IsValid())
	{
		return FinalAttack;
	}

	TArray<FTCGCardInstance> StackCards;
	GetCardsInStack(Card->StackId, StackCards);

	for (const FTCGCardInstance& StackCard : StackCards)
	{
		if (StackCard.StackIndex > Card->StackIndex)
		{
			continue;
		}

		TArray<FTCGCardEffectRef> EffectRefs;
		GetPrintedEffectRefsForCard(StackCard, EffectRefs);

		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (EffectRef.Trigger != ETCGEffectTrigger::None)
			{
				continue;
			}

			for (const FTCGEffectStep& Step : EffectRef.Steps)
			{
				if (Step.StepType != ETCGEffectStepType::ModifyAttack)
				{
					continue;
				}

				if (Step.ValueMode == ETCGEffectValueMode::Fixed)
				{
					FinalAttack += Step.Value;
				}
				else if (Step.ValueMode == ETCGEffectValueMode::CardsUnderneathSource)
				{
					FinalAttack += GetCardsUnderneathCount(StackCard.CardInstanceId);
				}
				else if (Step.ValueMode == ETCGEffectValueMode::CardsUnderneathTarget)
				{
					FinalAttack += GetCardsUnderneathCount(CardInstanceId);
				}
				else if (Step.ValueMode == ETCGEffectValueMode::ElementCardsInControllerGraveyard)
				{
					FinalAttack += CountCardsInLocationByElement(
						Card->OwnerPlayerIndex,
						ETCGCardLocation::Graveyard,
						Step.TargetFilter.RequiredElement);
				}
			}
		}
	}

	return FinalAttack;
}

bool ATCG_GameState::FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const
{
	OutStackId.Invalidate();
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId || !Card.StackId.IsValid()) continue;
		OutStackId = Card.StackId;
		return true;
	}
	return false;
}

void ATCG_GameState::GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == StackId) OutCards.Add(Card);
	}
	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

void ATCG_GameState::GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.ZoneId == ZoneId) OutCards.Add(Card);
	}
	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInZone(FName ZoneId) const
{
	const FTCGCardInstance* TopCard = nullptr;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}
	return TopCard;
}

bool ATCG_GameState::ResolveBattleBetweenZones(FName Player0ZoneId, FName Player1ZoneId)
{
	const FTCGCardInstance* Player0Card = FindTopCardInZone(Player0ZoneId);
	const FTCGCardInstance* Player1Card = FindTopCardInZone(Player1ZoneId);
	if (!Player0Card || !Player1Card) return false;

	const FGuid Player0WinnerId = Player0Card->CardInstanceId;
	const FGuid Player1WinnerId = Player1Card->CardInstanceId;
	const FGuid Player0LoserStackId = Player0Card->StackId;
	const FGuid Player1LoserStackId = Player1Card->StackId;

	const int32 Player0Attack = GetFinalAttack(Player0Card->CardInstanceId);
	const int32 Player1Attack = GetFinalAttack(Player1Card->CardInstanceId);

	if (Player0Attack > Player1Attack)
	{
		MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);

		if (const FTCGCardInstance* WinnerCard = FindCardInstance(Player0WinnerId))
		{
			TArray<FTCGEffectChainEntry> Chain;
			TArray<FTCGCardEffectRef> EffectRefs;
			GetPrintedEffectRefsForCard(*WinnerCard, EffectRefs);

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnBattleDestroy))
				{
					AddCardEffectRefToChain(Chain, Player0WinnerId, Player0WinnerId, EffectRef);
				}
			}

			ResolveEffectChain(Chain);
		}
	}
	else if (Player1Attack > Player0Attack)
	{
		MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);

		if (const FTCGCardInstance* WinnerCard = FindCardInstance(Player1WinnerId))
		{
			TArray<FTCGEffectChainEntry> Chain;
			TArray<FTCGCardEffectRef> EffectRefs;
			GetPrintedEffectRefsForCard(*WinnerCard, EffectRefs);

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnBattleDestroy))
				{
					AddCardEffectRefToChain(Chain, Player1WinnerId, Player1WinnerId, EffectRef);
				}
			}

			ResolveEffectChain(Chain);
		}
	}
	else
	{
		MoveStackToLocation(Player0LoserStackId, ETCGCardLocation::Graveyard);
		MoveStackToLocation(Player1LoserStackId, ETCGCardLocation::Graveyard);
	}

	return true;
}

bool ATCG_GameState::ResolveBattlePhase()
{
	if (HasPendingDiscardChoice())
	{
		if (bLogBattleFlow || bLogRoundFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase paused PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
		}
		return false;
	}

	bool bResolvedAnyBattle = false;
	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const int32 AttackerPlayerIndex = FieldIndex % 2 == 0 ? 0 : 1;
		const int32 DefenderPlayerIndex = 1 - AttackerPlayerIndex;
		const FName AttackerZoneId = GetFieldZoneId(AttackerPlayerIndex, FieldIndex);
		const FTCGCardInstance* AttackerCard = FindTopCardInZone(AttackerZoneId);
		if (!AttackerCard)
		{
			if (bLogBattleFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoAttacker"), FieldIndex, AttackerPlayerIndex);
			continue;
		}

		FName DefenderZoneId = NAME_None;
		int32 DefenderFieldIndex = INDEX_NONE;
		const FTCGCardInstance* DefenderCard = nullptr;
		for (int32 SearchOffset = 0; SearchOffset < FieldZoneCount; ++SearchOffset)
		{
			const int32 CandidateFieldIndex = (FieldIndex + SearchOffset) % FieldZoneCount;
			const FName CandidateZoneId = GetFieldZoneId(DefenderPlayerIndex, CandidateFieldIndex);
			const FTCGCardInstance* CandidateCard = FindTopCardInZone(CandidateZoneId);
			if (CandidateCard)
			{
				DefenderZoneId = CandidateZoneId;
				DefenderFieldIndex = CandidateFieldIndex;
				DefenderCard = CandidateCard;
				break;
			}
		}

		if (!DefenderCard || DefenderZoneId.IsNone())
		{
			if (bLogBattleFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoDefender"), FieldIndex, AttackerPlayerIndex);
			continue;
		}

		const FGuid AttackerStackId = AttackerCard->StackId;
		const FGuid DefenderStackId = DefenderCard->StackId;
		const int32 AttackerAttack = GetFinalAttack(AttackerCard->CardInstanceId);
		const int32 DefenderAttack = GetFinalAttack(DefenderCard->CardInstanceId);

		if (AttackerAttack > DefenderAttack)
		{
			MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
		}
		else if (DefenderAttack > AttackerAttack)
		{
			MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
		}
		else
		{
			MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
		}
		bResolvedAnyBattle = true;
	}
	return bResolvedAnyBattle;
}

ETCGMatchResult ATCG_GameState::CheckLoseConditionAfterBattle() const
{
	const bool bPlayer0HasCardOnBoard = DoesPlayerHaveAnyCardOnBoard(0);
	const bool bPlayer1HasCardOnBoard = DoesPlayerHaveAnyCardOnBoard(1);
	if (!bPlayer0HasCardOnBoard && !bPlayer1HasCardOnBoard) return ETCGMatchResult::Draw;
	if (!bPlayer0HasCardOnBoard) return ETCGMatchResult::Player1Wins;
	if (!bPlayer1HasCardOnBoard) return ETCGMatchResult::Player0Wins;
	return ETCGMatchResult::None;
}

void ATCG_GameState::GetCardsInLocation(int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location) OutCards.Add(Card);
	}
}

void ATCG_GameState::GetCardsInHand(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const
{
	GetCardsInLocation(PlayerIndex, ETCGCardLocation::Hand, OutCards);
	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex < B.LocationIndex;
	});
}

void ATCG_GameState::GetCardsInDeck(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const
{
	GetCardsInLocation(PlayerIndex, ETCGCardLocation::Deck, OutCards);
	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex > B.LocationIndex;
	});
}

int32 ATCG_GameState::GetNextLocationIndex(int32 PlayerIndex, ETCGCardLocation Location) const
{
	int32 HighestIndex = INDEX_NONE;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location) HighestIndex = FMath::Max(HighestIndex, Card.LocationIndex);
	}
	return HighestIndex + 1;
}

bool ATCG_GameState::DrawCard(int32 PlayerIndex)
{
	FTCGCardInstance* TopDeckCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Deck) continue;
		if (!TopDeckCard || Card.LocationIndex > TopDeckCard->LocationIndex) TopDeckCard = &Card;
	}
	if (!TopDeckCard) return false;

	TopDeckCard->Location = ETCGCardLocation::Hand;
	TopDeckCard->LocationIndex = GetNextLocationIndex(PlayerIndex, ETCGCardLocation::Hand);
	TopDeckCard->ZoneId = NAME_None;
	TopDeckCard->StackId.Invalidate();
	TopDeckCard->StackIndex = INDEX_NONE;
	return true;
}

int32 ATCG_GameState::DrawCards(int32 PlayerIndex, int32 Count)
{
	int32 DrawnCount = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (!DrawCard(PlayerIndex)) break;
		DrawnCount++;
	}
	return DrawnCount;
}

bool ATCG_GameState::PlayFirstCardFromHandToZone(int32 PlayerIndex, FName ZoneId)
{
	TArray<FTCGCardInstance> HandCards;
	GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() <= 0) return false;
	return PlayerPlayCardToZone(PlayerIndex, HandCards[0].CardInstanceId, ZoneId);
}

void ATCG_GameState::CreateDebugTestCards()
{
	SetupDebugMatch();
}

void ATCG_GameState::SetupDebugMatch()
{
	MatchCards.Empty();
	ValidateDebugCardDefinitions();

	for (int32 CopyIndex = 0; CopyIndex < 2; ++CopyIndex)
	{
		AddDebugCardInstance("Debug_Earth_Deck_A", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
		AddDebugCardInstance("Debug_Earth_Deck_B", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
		AddDebugCardInstance(DebugCard_FireDeckA, ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Deck);
		AddDebugCardInstance(DebugCard_FireDeckB, ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Deck);
		AddDebugCardInstance("Debug_Dark_Deck_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Deck);
		AddDebugCardInstance("Debug_Dark_Deck_B", ETCGCardElement::Dark, 2, 1, ETCGCardLocation::Deck);
		AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
		AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
	}

	TArray<FTCGCardInstance> Player0Deck;
	TArray<FTCGCardInstance> Player1Deck;
	GetCardsInDeck(0, Player0Deck);
	GetCardsInDeck(1, Player1Deck);
	if (bLogDebugSetup) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Setup match decks P0=%d P1=%d"), Player0Deck.Num(), Player1Deck.Num());
}

void ATCG_GameState::RunDebugTurnFlow()
{
	auto AddDebugBoardUnit = [this](FName CardDefinitionId, ETCGCardElement Element, int32 Attack, int32 PlayerIndex, int32 FieldIndex)
	{
		FTCGCardInstance& Card = AddCardInstance(CardDefinitionId, Element, Attack, PlayerIndex, ETCGCardLocation::Board);
		Card.ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);
		Card.StackId = FGuid::NewGuid();
		Card.StackIndex = 0;
		return Card.CardInstanceId;
	};

	auto CountBoardUnits = [this](int32 PlayerIndex)
	{
		TSet<FGuid> StackIds;
		for (const FTCGCardInstance& Card : MatchCards)
		{
			if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Board && Card.StackId.IsValid()) StackIds.Add(Card.StackId);
		}
		return StackIds.Num();
	};

	auto GetRoundLimitResult = [&CountBoardUnits]()
	{
		const int32 Player0UnitCount = CountBoardUnits(0);
		const int32 Player1UnitCount = CountBoardUnits(1);
		if (Player0UnitCount > Player1UnitCount) return ETCGMatchResult::Player0Wins;
		if (Player1UnitCount > Player0UnitCount) return ETCGMatchResult::Player1Wins;
		return ETCGMatchResult::Draw;
	};

	if (DebugScenario == ETCGDebugScenario::WraparoundBattle)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Wraparound battle scenario start"));
		MatchCards.Empty();
		SetMatchResult(ETCGMatchResult::None);
		SetPhase(ETCGMatchPhase::Battle);
		AddDebugBoardUnit("Wrap_P0_Field0", ETCGCardElement::Fire, 1, 0, 0);
		AddDebugBoardUnit("Wrap_P0_Field2", ETCGCardElement::Earth, 6, 0, 2);
		AddDebugBoardUnit("Wrap_P1_Field1", ETCGCardElement::Dark, 5, 1, 1);
		AddDebugBoardUnit("Wrap_P1_Field3", ETCGCardElement::Light, 2, 1, 3);
		const bool bResolved = ResolveBattlePhase();
		const ETCGMatchResult ScenarioResult = CheckLoseConditionAfterBattle();
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Wraparound battle scenario summary Resolved=%s P0Board=%s P1Board=%s Result=%s"),
			bResolved ? TEXT("true") : TEXT("false"),
			DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
			DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
			GetTCGMatchResultDebugName(ScenarioResult));
		EndMatch(ScenarioResult);
		return;
	}

	if (DebugScenario == ETCGDebugScenario::RoundLimitTiebreak)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit tiebreak scenario start Max=%d"), MaxRoundNumber);
		MatchCards.Empty();
		SetMatchResult(ETCGMatchResult::None);
		SetPhase(ETCGMatchPhase::Battle);
		RoundNumber = FMath::Max(1, MaxRoundNumber);
		TurnNumber = RoundNumber;
		AddDebugBoardUnit("Limit_P0_Field0", ETCGCardElement::Fire, 6, 0, 0);
		AddDebugBoardUnit("Limit_P0_Field1", ETCGCardElement::Earth, 6, 0, 1);
		AddDebugBoardUnit("Limit_P0_Field2", ETCGCardElement::Light, 6, 0, 2);
		AddDebugBoardUnit("Limit_P1_Field0", ETCGCardElement::Dark, 1, 1, 0);
		AddDebugBoardUnit("Limit_P1_Field1", ETCGCardElement::Water, 10, 1, 1);
		AddDebugBoardUnit("Limit_P1_Field2", ETCGCardElement::Wind, 1, 1, 2);
		const bool bResolved = ResolveBattlePhase();
		ETCGMatchResult ScenarioResult = CheckLoseConditionAfterBattle();
		if (ScenarioResult == ETCGMatchResult::None && RoundNumber >= FMath::Max(1, MaxRoundNumber))
		{
			ScenarioResult = GetRoundLimitResult();
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit reached R%d Max=%d P0Units=%d P1Units=%d Result=%s"), RoundNumber, MaxRoundNumber, CountBoardUnits(0), CountBoardUnits(1), GetTCGMatchResultDebugName(ScenarioResult));
		}
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit tiebreak scenario summary Resolved=%s P0Board=%s P1Board=%s Result=%s"),
			bResolved ? TEXT("true") : TEXT("false"),
			DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
			DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
			GetTCGMatchResultDebugName(ScenarioResult));
		EndMatch(ScenarioResult);
		return;
	}

	if (DebugScenario == ETCGDebugScenario::OverlayPlacement)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement scenario start"));
		MatchCards.Empty();
		SetMatchResult(ETCGMatchResult::None);
		SetPhase(ETCGMatchPhase::Placement);
		RoundNumber = 2;
		TurnNumber = RoundNumber;
		PlacementStepIndex = 0;
		Player0PlacementFieldZonesUsedThisRound.Reset();
		Player1PlacementFieldZonesUsedThisRound.Reset();
		SetCurrentTurnPlayer(0);

		AddDebugBoardUnit("Overlay_P0_Field0", ETCGCardElement::Fire, 1, 0, 0);
		AddDebugBoardUnit("Overlay_P0_Field1", ETCGCardElement::Earth, 1, 0, 1);
		AddDebugBoardUnit("Overlay_P0_Field2", ETCGCardElement::Fire, 1, 0, 2);
		AddDebugBoardUnit("Overlay_P0_Field3", ETCGCardElement::Water, 1, 0, 3);

		FTCGCardInstance& FirstOverlayCard = AddCardInstance("Overlay_P0_First_Fire", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
		const FName OverlayZoneId = GetFieldZoneId(0, 2);
		const bool bFirstOverlaySuccess = PlayerPlayCardToZone(0, FirstOverlayCard.CardInstanceId, OverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement first attempt Field=2 Type=Overlay Success=%s Expected=true UsedZones=%d"), bFirstOverlaySuccess ? TEXT("true") : TEXT("false"), Player0PlacementFieldZonesUsedThisRound.Num());

		PlacementStepIndex = 2;
		SetCurrentTurnPlayer(0);

		FTCGCardInstance& SecondOverlayCard = AddCardInstance("Overlay_P0_Second_Fire", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
		const bool bSecondOverlaySuccess = PlayerPlayCardToZone(0, SecondOverlayCard.CardInstanceId, OverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement same-zone retry Field=2 Type=Overlay Success=%s Expected=false UsedZones=%d"), bSecondOverlaySuccess ? TEXT("true") : TEXT("false"), Player0PlacementFieldZonesUsedThisRound.Num());

		PlacementStepIndex = 2;
		SetCurrentTurnPlayer(0);

		FTCGCardInstance& ThirdOverlayCard = AddCardInstance("Overlay_P0_Third_Earth", ETCGCardElement::Earth, 2, 0, ETCGCardLocation::Hand);
		const FName DifferentOverlayZoneId = GetFieldZoneId(0, 1);
		const bool bDifferentZoneOverlaySuccess = PlayerPlayCardToZone(0, ThirdOverlayCard.CardInstanceId, DifferentOverlayZoneId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Overlay placement different-zone attempt Field=1 Type=Overlay Success=%s Expected=true UsedZones=%d"), bDifferentZoneOverlaySuccess ? TEXT("true") : TEXT("false"), Player0PlacementFieldZonesUsedThisRound.Num());

		EndMatch(ETCGMatchResult::Draw);
		return;
	}

	const int32 Player0InitialDraw = DrawCards(0, InitialHandSize);
	const int32 Player1InitialDraw = DrawCards(1, InitialHandSize);
	if (bLogRoundFlow)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Starting hand draw P0=%d P1=%d Target=%d"), Player0InitialDraw, Player1InitialDraw, InitialHandSize);
	}

	while (!IsMatchOver())
	{
		if (HasPendingDiscardChoice())
		{
			if (bLogRoundFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round flow paused PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
			}
			return;
		}

		if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round %d start"), RoundNumber);

		SetPhase(ETCGMatchPhase::Placement);
		PlacementStepIndex = 0;
		Player0PlacementFieldZonesUsedThisRound.Reset();
		Player1PlacementFieldZonesUsedThisRound.Reset();
		SetCurrentTurnPlayer(GetPlacementStepPlayer());

		while (CurrentPhase == ETCGMatchPhase::Placement && !IsPlacementPhaseComplete())
		{
			if (HasPendingDiscardChoice())
			{
				if (bLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement loop paused PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
				}
				return;
			}

			const int32 PlayerIndex = GetPlacementStepPlayer();
			const int32 DrawnCount = DrawCards(PlayerIndex, CardsDrawnPerPlacementStep);

			bool bPlayed = false;
			int32 PlayedFieldIndex = INDEX_NONE;
			FString PlacementType = TEXT("None");

			TArray<FTCGCardInstance> HandCards;
			GetCardsInHand(PlayerIndex, HandCards);
			for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount && !bPlayed; ++FieldIndex)
			{
				const FName ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);
				for (const FTCGCardInstance& HandCard : HandCards)
				{
					if (!CanPlayerPlayCardToZone(PlayerIndex, HandCard.CardInstanceId, ZoneId)) continue;

					FGuid ExistingStackId;
					PlacementType = FindStackIdInZone(ZoneId, ExistingStackId) ? TEXT("Overlay") : TEXT("NewStack");
					bPlayed = PlayerPlayCardToZone(PlayerIndex, HandCard.CardInstanceId, ZoneId);
					PlayedFieldIndex = FieldIndex;
					break;
				}
			}

			if (!bPlayed)
			{
				if (bLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d Draw=%d skipped no legal placement"), RoundNumber, PlacementStepIndex + 1, PlayerIndex, DrawnCount);
				}
				AdvancePlacementStep();
				continue;
			}

			if (bLogPlacementFlow)
			{
				const TArray<int32>& UsedZones = PlayerIndex == 0 ? Player0PlacementFieldZonesUsedThisRound : Player1PlacementFieldZonesUsedThisRound;
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d Draw=%d Field=%d Type=%s UsedZones=%d Success=true"),
					RoundNumber,
					PlacementStepIndex,
					PlayerIndex,
					DrawnCount,
					PlayedFieldIndex,
					*PlacementType,
					UsedZones.Num());
			}

			if (HasPendingDiscardChoice())
			{
				if (bLogPlacementFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement loop paused after play PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
				}
				return;
			}
		}

		if (CurrentPhase == ETCGMatchPhase::Battle)
		{
			if (HasPendingDiscardChoice())
			{
				if (bLogRoundFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle start paused PendingDiscard Player=%d Count=%d"), PendingDiscardChoice.PlayerIndex, PendingDiscardChoice.RequiredCount);
				}
				return;
			}

			if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase started R%d"), RoundNumber);
			const bool bResolved = ResolveBattlePhase();
			ETCGMatchResult ResultAfterBattle = CheckLoseConditionAfterBattle();

			if (ResultAfterBattle == ETCGMatchResult::None && RoundNumber >= FMath::Max(1, MaxRoundNumber))
			{
				ResultAfterBattle = GetRoundLimitResult();
				if (bLogRoundFlow)
				{
					UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round limit reached R%d Max=%d P0Units=%d P1Units=%d Result=%s"), RoundNumber, MaxRoundNumber, CountBoardUnits(0), CountBoardUnits(1), GetTCGMatchResultDebugName(ResultAfterBattle));
				}
			}

			if (bLogRoundFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle summary R%d Resolved=%s P0Board=%s P1Board=%s Result=%s"),
					RoundNumber,
					bResolved ? TEXT("true") : TEXT("false"),
					DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
					DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
					GetTCGMatchResultDebugName(ResultAfterBattle));
			}

			if (ResultAfterBattle != ETCGMatchResult::None)
			{
				EndMatch(ResultAfterBattle);
				if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Match ended after battle"));
				return;
			}
		}

		RoundNumber++;
		TurnNumber = RoundNumber;
	}
}
