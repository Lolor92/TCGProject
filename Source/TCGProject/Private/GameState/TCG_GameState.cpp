#include "GameState/TCG_GameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr bool bEnableDebugOverlayRemovalFizzleTest = false;
	constexpr bool bLogDebugSetup = true;
	constexpr bool bLogRoundFlow = true;
	constexpr bool bLogPlacementFlow = true;
	constexpr bool bLogBattleFlow = true;
	constexpr bool bLogEffectChains = false;
	constexpr bool bLogVerboseCardTriggers = false;
	constexpr int32 DebugMaxRoundCount = 20;

	const FName DebugCard_FireDeckA = "Debug_Fire_Deck_A";
	const FName DebugCard_FireDeckB = "Debug_Fire_Deck_B";
	const FName DebugEffect_Draw1 = "Debug_Draw1";
	const FName DebugEffect_GainAttackForCardsUnderneath = "Debug_GainAttackForCardsUnderneath";
	const FName DebugEffect_RemoveBottomOverlay = "Debug_RemoveBottomOverlay";
}

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
	DOREPLIFETIME(ATCG_GameState, CurrentTurnPlayerIndex);
	DOREPLIFETIME(ATCG_GameState, CurrentPhase);
	DOREPLIFETIME(ATCG_GameState, MatchResult);
	DOREPLIFETIME(ATCG_GameState, MatchCards);
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
	CurrentTurnPlayerIndex = 0;
	CurrentPhase = ETCGMatchPhase::RoundStart;
	MatchResult = ETCGMatchResult::None;
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
	return IsValidPlayerIndex(PlayerIndex) && CurrentPhase == ETCGMatchPhase::Placement && !IsPlacementPhaseComplete() && GetPlacementStepPlayer() == PlayerIndex;
}

int32 ATCG_GameState::GetCompletedPlacementStepsForPlayer(int32 PlayerIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return 0;

	int32 CompletedSteps = 0;
	for (int32 StepIndex = 0; StepIndex < PlacementStepIndex; ++StepIndex)
	{
		if (StepIndex % 2 == PlayerIndex)
		{
			CompletedSteps++;
		}
	}
	return CompletedSteps;
}

int32 ATCG_GameState::GetNextRequiredFieldZoneIndex(int32 PlayerIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return INDEX_NONE;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		FGuid ExistingStackId;
		if (!FindStackIdInZone(GetFieldZoneId(PlayerIndex, FieldIndex), ExistingStackId))
		{
			return FieldIndex;
		}
	}
	return INDEX_NONE;
}

bool ATCG_GameState::CanPlayerPlaceInFieldZone(int32 PlayerIndex, int32 FieldIndex) const
{
	if (!IsValidPlayerIndex(PlayerIndex)) return false;
	if (FieldIndex < 0 || FieldIndex >= FieldZoneCount) return false;
	return FieldIndex == GetNextRequiredFieldZoneIndex(PlayerIndex);
}

bool ATCG_GameState::AdvancePlacementStep()
{
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

FTCGCardInstance* ATCG_GameState::AddCardInstanceFromDefinition(const UTCG_CardDefinition* CardDefinition, int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation)
{
	if (!CardDefinition) return nullptr;
	return &AddCardInstance(CardDefinition->CardDefinitionId, CardDefinition->Element, CardDefinition->BaseAttack, OwnerPlayerIndex, StartingLocation);
}

FTCGCardInstance& ATCG_GameState::AddDebugCardInstance(FName CardDefinitionId, ETCGCardElement FallbackElement, int32 FallbackBaseAttack, int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation)
{
	if (const UTCG_CardDefinition* CardDefinition = FindDebugCardDefinitionById(CardDefinitionId))
	{
		if (FTCGCardInstance* CardInstance = AddCardInstanceFromDefinition(CardDefinition, OwnerPlayerIndex, StartingLocation))
		{
			if (bLogDebugSetup)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Added card %s P%d ATK %d Effects %d"),
					*CardInstance->CardDefinitionId.ToString(), OwnerPlayerIndex, CardInstance->BaseAttack, CardDefinition->Effects.Num());
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
		if (Card.StackId != StackId || Card.Location != ETCGCardLocation::Board) continue;
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
		if (Card.StackId != StackId || Card.Location != ETCGCardLocation::Board) continue;
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
	if (IsMatchOver()) return false;
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
		return !FindStackIdInZone(ZoneId, ExistingStackId);
	}
	return false;
}

bool ATCG_GameState::PlayerPlayCardToZone(int32 PlayerIndex, const FGuid& CardInstanceId, FName ZoneId)
{
	if (!CanPlayerPlayCardToZone(PlayerIndex, CardInstanceId, ZoneId))
	{
		const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: PlayerPlayCardToZone rejected Player=%d Card=%s Zone=%s Phase=%d PlacementStep=%d CurrentPlayer=%d"),
			PlayerIndex,
			Card ? *Card->CardDefinitionId.ToString() : TEXT("None"),
			*ZoneId.ToString(),
			static_cast<int32>(CurrentPhase),
			PlacementStepIndex,
			CurrentTurnPlayerIndex);
		return false;
	}

	const bool bPlayed = PlayCardToZone(CardInstanceId, ZoneId);
	if (bPlayed) AdvancePlacementStep();
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
	return Trigger != ETCGEffectTrigger::None && EffectRef.Trigger == Trigger && !EffectRef.EffectId.IsNone();
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
		if (DoesCardEffectMatchTrigger(EffectRef, Trigger)) OutEffectIds.Add(EffectRef.EffectId);
	}
	return OutEffectIds.Num();
}

bool ATCG_GameState::AddCardTriggerToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, ETCGEffectTrigger Trigger, FName EffectId)
{
	const FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || Trigger == ETCGEffectTrigger::None || EffectId.IsNone()) return false;

	FTCGEffectChainEntry NewEntry;
	NewEntry.ChainIndex = Chain.Num() + 1;
	NewEntry.SourceCardInstanceId = SourceCardInstanceId;
	NewEntry.TargetCardInstanceId = TargetCardInstanceId;
	NewEntry.SourceCardDefinitionId = SourceCard->CardDefinitionId;
	NewEntry.Trigger = Trigger;
	NewEntry.EffectId = EffectId;
	NewEntry.ControllerPlayerIndex = SourceCard->OwnerPlayerIndex;

	ApplyDebugEffectChainEntryRequirements(NewEntry);
	Chain.Add(NewEntry);

	if (bLogEffectChains)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain add %d Source=%s Trigger=%s Effect=%s"),
			NewEntry.ChainIndex, *NewEntry.SourceCardDefinitionId.ToString(), GetTCGEffectTriggerDebugName(NewEntry.Trigger), *NewEntry.EffectId.ToString());
	}
	return true;
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

		TArray<FName> EffectIds;
		GetPrintedEffectsForCardTrigger(StackCard, ETCGEffectTrigger::OnPlay, EffectIds);

		for (const FName& EffectId : EffectIds)
		{
			AddCardTriggerToChain(OutChain, StackCard.CardInstanceId, TopCardInstanceId, ETCGEffectTrigger::OnPlay, EffectId);
		}
	}

	if (bEnableDebugOverlayRemovalFizzleTest && TopCard->CardDefinitionId == DebugCard_FireDeckA && OutChain.Num() >= 2)
	{
		AddCardTriggerToChain(OutChain, TopCard->CardInstanceId, TopCardInstanceId, ETCGEffectTrigger::OnBecomingTopCard, DebugEffect_RemoveBottomOverlay);
	}
	return OutChain.Num();
}

bool ATCG_GameState::ResolveEffectChain(const TArray<FTCGEffectChainEntry>& Chain)
{
	if (Chain.Num() <= 0) return false;

	if (bLogEffectChains) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain resolving count: %d"), Chain.Num());

	bool bResolvedAny = false;
	for (int32 ChainIndex = Chain.Num() - 1; ChainIndex >= 0; --ChainIndex)
	{
		const FTCGEffectChainEntry& Entry = Chain[ChainIndex];
		if (bLogEffectChains)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain resolve %d Source=%s Effect=%s"), Entry.ChainIndex, *Entry.SourceCardDefinitionId.ToString(), *Entry.EffectId.ToString());
		}

		if (!CanResolveEffectChainEntry(Entry)) continue;
		bResolvedAny |= ResolveDebugEffectChainEntry(Entry);
	}
	return bResolvedAny;
}

bool ATCG_GameState::ResolveDebugEffectChainEntry(const FTCGEffectChainEntry& ChainEntry)
{
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	const FTCGCardInstance* TargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);
	if (!SourceCard || !TargetCard) return false;

	if (ChainEntry.EffectId == DebugEffect_Draw1)
	{
		const bool bDrewCard = DrawCard(ChainEntry.ControllerPlayerIndex);
		if (bLogEffectChains)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect Draw 1 from %s success: %s"), *SourceCard->CardDefinitionId.ToString(), bDrewCard ? TEXT("true") : TEXT("false"));
		}
		return bDrewCard;
	}

	if (ChainEntry.EffectId == DebugEffect_GainAttackForCardsUnderneath)
	{
		FTCGCardInstance* MutableTargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);
		if (!MutableTargetCard) return false;

		const int32 CardsUnderneath = GetCardsUnderneathCount(ChainEntry.TargetCardInstanceId);
		MutableTargetCard->AttackModifier += CardsUnderneath;
		if (bLogEffectChains)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect %s gained ATK from cards underneath: %d modifier now: %d"), *MutableTargetCard->CardDefinitionId.ToString(), CardsUnderneath, MutableTargetCard->AttackModifier);
		}
		return true;
	}

	if (ChainEntry.EffectId == DebugEffect_RemoveBottomOverlay)
	{
		FTCGCardInstance* MutableTargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);
		if (!MutableTargetCard || !MutableTargetCard->StackId.IsValid()) return false;

		FTCGCardInstance* BottomOverlayCard = nullptr;
		for (FTCGCardInstance& Card : MatchCards)
		{
			if (Card.Location != ETCGCardLocation::Board) continue;
			if (Card.StackId != MutableTargetCard->StackId) continue;
			if (Card.CardInstanceId == MutableTargetCard->CardInstanceId) continue;
			if (Card.StackIndex >= MutableTargetCard->StackIndex) continue;
			if (!BottomOverlayCard || Card.StackIndex < BottomOverlayCard->StackIndex) BottomOverlayCard = &Card;
		}

		if (!BottomOverlayCard) return false;
		const bool bMoved = MoveCardToLocation(BottomOverlayCard->CardInstanceId, ETCGCardLocation::Graveyard);
		if (bLogEffectChains)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect RemoveBottomOverlay success: %s"), bMoved ? TEXT("true") : TEXT("false"));
		}
		return bMoved;
	}

	return false;
}

bool ATCG_GameState::MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = NewLocation;
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
		if (Card.StackId == TargetCard->StackId && Card.StackIndex < TargetCard->StackIndex) Count++;
	}
	return Count;
}

int32 ATCG_GameState::GetFinalAttack(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return 0;
	return Card->BaseAttack + Card->AttackModifier + GetCardsUnderneathCount(CardInstanceId);
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

	const FGuid Player0StackId = Player0Card->StackId;
	const FGuid Player1StackId = Player1Card->StackId;
	const int32 Player0Attack = GetFinalAttack(Player0Card->CardInstanceId);
	const int32 Player1Attack = GetFinalAttack(Player1Card->CardInstanceId);

	if (Player0Attack > Player1Attack)
	{
		MoveStackToLocation(Player1StackId, ETCGCardLocation::Graveyard);
	}
	else if (Player1Attack > Player0Attack)
	{
		MoveStackToLocation(Player0StackId, ETCGCardLocation::Graveyard);
	}
	else
	{
		MoveStackToLocation(Player0StackId, ETCGCardLocation::Graveyard);
		MoveStackToLocation(Player1StackId, ETCGCardLocation::Graveyard);
	}

	return true;
}

bool ATCG_GameState::ResolveBattlePhase()
{
	bool bResolvedAnyBattle = false;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const int32 AttackerPlayerIndex = FieldIndex % 2 == 0 ? 0 : 1;
		const int32 DefenderPlayerIndex = 1 - AttackerPlayerIndex;
		const FName AttackerZoneId = GetFieldZoneId(AttackerPlayerIndex, FieldIndex);
		const FTCGCardInstance* AttackerCard = FindTopCardInZone(AttackerZoneId);

		if (!AttackerCard)
		{
			if (bLogBattleFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoAttacker"), FieldIndex, AttackerPlayerIndex);
			}
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
			if (bLogBattleFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration skipped Field=%d Attacker=P%d Reason=NoDefender"), FieldIndex, AttackerPlayerIndex);
			}
			continue;
		}

		const FGuid AttackerStackId = AttackerCard->StackId;
		const FGuid DefenderStackId = DefenderCard->StackId;
		const int32 AttackerAttack = GetFinalAttack(AttackerCard->CardInstanceId);
		const int32 DefenderAttack = GetFinalAttack(DefenderCard->CardInstanceId);
		const FName AttackerDefinitionId = AttackerCard->CardDefinitionId;
		const FName DefenderDefinitionId = DefenderCard->CardDefinitionId;

		if (bLogBattleFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle declaration Field=%d Attacker=P%d Target=P%d Field=%d"),
				FieldIndex, AttackerPlayerIndex, DefenderPlayerIndex, DefenderFieldIndex);
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle attack P%d %s ATK %d -> P%d %s ATK %d"),
				AttackerPlayerIndex, *AttackerDefinitionId.ToString(), AttackerAttack,
				DefenderPlayerIndex, *DefenderDefinitionId.ToString(), DefenderAttack);
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle trigger point OnAttack P%d %s"), AttackerPlayerIndex, *AttackerDefinitionId.ToString());
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle trigger point OnAttacked P%d %s"), DefenderPlayerIndex, *DefenderDefinitionId.ToString());
		}

		if (AttackerAttack > DefenderAttack)
		{
			MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
			if (bLogBattleFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result Defender P%d stack sent to Graveyard"), DefenderPlayerIndex);
		}
		else if (DefenderAttack > AttackerAttack)
		{
			MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			if (bLogBattleFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result Attacker P%d stack sent to Graveyard"), AttackerPlayerIndex);
		}
		else
		{
			MoveStackToLocation(AttackerStackId, ETCGCardLocation::Graveyard);
			MoveStackToLocation(DefenderStackId, ETCGCardLocation::Graveyard);
			if (bLogBattleFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result tie, attacker and defender stacks sent to Graveyard"));
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

void ATCG_GameState::SetupDebugMatch()
{
	MatchCards.Empty();
	ValidateDebugCardDefinitions();

	AddDebugCardInstance("Debug_Earth_Deck_A", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Earth_Deck_B", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Fire_Deck_A", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Fire_Deck_B", ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Dark_Deck_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Dark_Deck_B", ETCGCardElement::Dark, 2, 1, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
	AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);

	TArray<FTCGCardInstance> Player0Deck;
	TArray<FTCGCardInstance> Player1Deck;
	GetCardsInDeck(0, Player0Deck);
	GetCardsInDeck(1, Player1Deck);
	if (bLogDebugSetup) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Setup match decks P0=%d P1=%d"), Player0Deck.Num(), Player1Deck.Num());
}

void ATCG_GameState::RunDebugTurnFlow()
{
	int32 RoundsResolved = 0;

	while (!IsMatchOver() && RoundsResolved < DebugMaxRoundCount)
	{
		if (RoundsResolved > 0)
		{
			RoundNumber++;
			TurnNumber = RoundNumber;
		}

		SetPhase(ETCGMatchPhase::RoundStart);
		PlacementStepIndex = 0;
		SetCurrentTurnPlayer(0);

		const int32 Player0OpeningDraw = DrawCards(0, 1);
		if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round %d start P0 debug opening draw=%d"), RoundNumber, Player0OpeningDraw);

		SetPhase(ETCGMatchPhase::Placement);

		while (CurrentPhase == ETCGMatchPhase::Placement && !IsPlacementPhaseComplete())
		{
			const int32 ActivePlayer = GetPlacementStepPlayer();
			SetCurrentTurnPlayer(ActivePlayer);

			if (PlacementStepIndex > 0) DrawCard(ActivePlayer);

			const int32 FieldIndex = GetNextRequiredFieldZoneIndex(ActivePlayer);
			const int32 LogStepNumber = PlacementStepIndex + 1;

			if (FieldIndex == INDEX_NONE)
			{
				if (bLogPlacementFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d skipped no empty field"), RoundNumber, LogStepNumber, ActivePlayer);
				AdvancePlacementStep();
				continue;
			}

			const FName ZoneId = GetFieldZoneId(ActivePlayer, FieldIndex);
			const bool bPlayedCard = PlayFirstCardFromHandToZone(ActivePlayer, ZoneId);
			if (bLogPlacementFlow)
			{
				UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Placement R%d Step=%d P%d Field=%d Success=%s"),
					RoundNumber, LogStepNumber, ActivePlayer, FieldIndex, bPlayedCard ? TEXT("true") : TEXT("false"));
			}

			if (!bPlayedCard) AdvancePlacementStep();
		}

		if (CurrentPhase != ETCGMatchPhase::Battle) SetPhase(ETCGMatchPhase::Battle);
		if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase started R%d"), RoundNumber);

		const bool bBattleResolved = ResolveBattlePhase();
		const ETCGMatchResult BattleMatchResult = CheckLoseConditionAfterBattle();
		if (bLogRoundFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle summary R%d Resolved=%s P0Board=%s P1Board=%s Result=%s"),
				RoundNumber,
				bBattleResolved ? TEXT("true") : TEXT("false"),
				DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"),
				DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"),
				BattleMatchResult == ETCGMatchResult::Player0Wins ? TEXT("Player 0 wins") :
				BattleMatchResult == ETCGMatchResult::Player1Wins ? TEXT("Player 1 wins") :
				BattleMatchResult == ETCGMatchResult::Draw ? TEXT("Draw") : TEXT("None"));
		}

		if (BattleMatchResult != ETCGMatchResult::None)
		{
			EndMatch(BattleMatchResult);
			if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Match ended after battle"));
			return;
		}

		SetPhase(ETCGMatchPhase::RoundEnd);
		RoundsResolved++;
		if (bLogRoundFlow) UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round %d end"), RoundNumber);
	}

	if (!IsMatchOver() && RoundsResolved >= DebugMaxRoundCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Round loop stopped at debug max rounds: %d"), DebugMaxRoundCount);
	}
}

void ATCG_GameState::CreateDebugTestCards()
{
	MatchCards.Empty();
	AddCardInstance("Debug_Fire_Deck_A", ETCGCardElement::Fire, 1, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Water_Deck_A", ETCGCardElement::Water, 2, 0, ETCGCardLocation::Deck);

	TArray<FTCGCardInstance> Player0DeckBeforeDraw;
	GetCardsInDeck(0, Player0DeckBeforeDraw);
	const bool bDrawCard = DrawCard(0);

	TArray<FTCGCardInstance> Player0DeckAfterDraw;
	TArray<FTCGCardInstance> Player0HandAfterDraw;
	GetCardsInDeck(0, Player0DeckAfterDraw);
	GetCardsInHand(0, Player0HandAfterDraw);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 deck before draw: %d"), Player0DeckBeforeDraw.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Draw card success: %s"), bDrawCard ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 deck after draw: %d"), Player0DeckAfterDraw.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 hand after draw: %d"), Player0HandAfterDraw.Num());

	FTCGCardInstance& Player0FireA = AddCardInstance("Debug_Fire_A", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player0FireB = AddCardInstance("Debug_Fire_B", ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player0Water = AddCardInstance("Debug_Water_A", ETCGCardElement::Water, 4, 0, ETCGCardLocation::Hand);
	FTCGCardInstance& Player1Dark = AddCardInstance("Debug_Dark_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Hand);

	const FGuid FireAId = Player0FireA.CardInstanceId;
	const FGuid FireBId = Player0FireB.CardInstanceId;
	const FGuid WaterId = Player0Water.CardInstanceId;
	const FGuid DarkId = Player1Dark.CardInstanceId;

	const bool bFireNewStack = PlayCardToZone(FireAId, "Player0_Field_0");
	const bool bFireOnFire = PlayCardToZone(FireBId, "Player0_Field_0");
	const bool bWaterOnFire = PlayCardToZone(WaterId, "Player0_Field_0");
	const bool bDarkNewStack = PlayCardToZone(DarkId, "Player1_Field_0");

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Fire new stack success: %s"), bFireNewStack ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Dark new stack success: %s"), bDarkNewStack ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Fire on Fire success: %s"), bFireOnFire ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Water on Fire success: %s"), bWaterOnFire ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 has board card: %s"), DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 1 has board card: %s"), DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: FireB final attack: %d"), GetFinalAttack(FireBId));
}
