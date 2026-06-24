#include "GameState/TCG_GameState.h"
#include "GameState/TCG_BattleResolver.h"
#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_CardMovementService.h"
#include "GameState/TCG_CardQueryService.h"
#include "GameState/TCG_EffectResolver.h"
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
		case ETCGEffectTrigger::OnSentFromDeckToGraveyard: return TEXT("OnSentFromDeckToGraveyard");
		case ETCGEffectTrigger::OnSentFromHandToGraveyard: return TEXT("OnSentFromHandToGraveyard");
		case ETCGEffectTrigger::OnSentFromBoardToGraveyard: return TEXT("OnSentFromBoardToGraveyard");
		case ETCGEffectTrigger::OnSentFromMaterialToGraveyard: return TEXT("OnSentFromMaterialToGraveyard");
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
	DOREPLIFETIME(ATCG_GameState, PendingRevealDeckChoice);
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
	ClearPendingRevealDeckChoice();
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
	return UTCG_CardQueryService::FindCardInstance(this, CardInstanceId);
}

const FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId) const
{
	return UTCG_CardQueryService::FindCardInstance(this, CardInstanceId);
}

FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId)
{
	return UTCG_CardQueryService::FindTopCardInStack(this, StackId);
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInStack(const FGuid& StackId) const
{
	return UTCG_CardQueryService::FindTopCardInStack(this, StackId);
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
	return UTCG_EffectResolver::DoesCardEffectMatchTrigger(EffectRef, Trigger);
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
	return UTCG_EffectResolver::AddCardTriggerToChain(this, Chain, SourceCardInstanceId, TargetCardInstanceId, Trigger, EffectId);
}

void ATCG_GameState::ApplyDebugEffectChainEntryRequirements(FTCGEffectChainEntry& ChainEntry) const
{
	UTCG_EffectResolver::ApplyDebugEffectChainEntryRequirements(const_cast<ATCG_GameState*>(this), ChainEntry);
}

bool ATCG_GameState::CanResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry) const
{
	return UTCG_EffectResolver::CanResolveEffectChainEntry(this, ChainEntry);
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
	return UTCG_CardMovementService::MoveCardToLocation(this, CardInstanceId, NewLocation);
}

bool ATCG_GameState::MoveStackToLocation(const FGuid& StackId, ETCGCardLocation NewLocation)
{
	return UTCG_CardMovementService::MoveStackToLocation(this, StackId, NewLocation);
}

bool ATCG_GameState::DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const
{
	return UTCG_CardQueryService::DoesPlayerHaveAnyCardOnBoard(this, PlayerIndex);
}

int32 ATCG_GameState::GetCardsUnderneathCount(const FGuid& CardInstanceId) const
{
	return UTCG_CardQueryService::GetCardsUnderneathCount(this, CardInstanceId);
}

int32 ATCG_GameState::GetFinalAttack(const FGuid& CardInstanceId) const
{
	return UTCG_CardQueryService::GetFinalAttack(this, CardInstanceId);
}

bool ATCG_GameState::FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const
{
	return UTCG_CardQueryService::FindStackIdInZone(this, ZoneId, OutStackId);
}

void ATCG_GameState::GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const
{
	UTCG_CardQueryService::GetCardsInStack(this, StackId, OutCards);
}

void ATCG_GameState::GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const
{
	UTCG_CardQueryService::GetCardsInZone(this, ZoneId, OutCards);
}

const FTCGCardInstance* ATCG_GameState::FindTopCardInZone(FName ZoneId) const
{
	return UTCG_CardQueryService::FindTopCardInZone(this, ZoneId);
}

bool ATCG_GameState::ResolveBattleBetweenZones(FName Player0ZoneId, FName Player1ZoneId)
{
	return UTCG_BattleResolver::ResolveBattleBetweenZones(this, Player0ZoneId, Player1ZoneId);
}

bool ATCG_GameState::ResolveBattlePhase()
{
	return UTCG_BattleResolver::ResolveBattlePhase(this);
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
	UTCG_CardQueryService::GetCardsInLocation(this, PlayerIndex, Location, OutCards);
}

void ATCG_GameState::GetCardsInHand(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const
{
	UTCG_CardQueryService::GetCardsInHand(this, PlayerIndex, OutCards);
}

void ATCG_GameState::GetCardsInDeck(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const
{
	UTCG_CardQueryService::GetCardsInDeck(this, PlayerIndex, OutCards);
}

int32 ATCG_GameState::GetNextLocationIndex(int32 PlayerIndex, ETCGCardLocation Location) const
{
	return UTCG_CardQueryService::GetNextLocationIndex(this, PlayerIndex, Location);
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
	UTCG_DebugScenarioRunner::CreateDebugTestCards(this);
}

void ATCG_GameState::SetupDebugMatch()
{
	UTCG_DebugScenarioRunner::SetupDebugMatch(this);
}

void ATCG_GameState::RunDebugTurnFlow()
{
	UTCG_DebugScenarioRunner::RunDebugTurnFlow(this);
}
