#include "GameState/TCG_GameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr bool bEnableDebugOverlayRemovalFizzleTest = false;

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
	CurrentTurnPlayerIndex = 0;
	CurrentPhase = ETCGMatchPhase::Draw;
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

FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId)
{
	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId)
		{
			return &Card;
		}
	}

	return nullptr;
}

const FTCGCardInstance* ATCG_GameState::FindCardInstance(const FGuid& CardInstanceId) const
{
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId)
		{
			return &Card;
		}
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

	if (!CardToPlace || !TopCard) return false;

	// Core rule for now: same element can stack on same element.
	// Exceptions will be added later through effects/rules.
	return CardToPlace->Element == TopCard->Element;
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
		if (bPlayed)
		{
			ExecuteCardTrigger(CardInstanceId, ETCGEffectTrigger::OnStackAdded);
		}
	}

	if (!bPlayed) return false;

	ExecuteCardTrigger(CardInstanceId, ETCGEffectTrigger::OnBecomingTopCard);

	TArray<FTCGEffectChainEntry> Chain;
	BuildStackOnPlayEffectChain(CardInstanceId, Chain);
	ResolveEffectChain(Chain);

	return true;
}

bool ATCG_GameState::ExecuteCardTrigger(const FGuid& CardInstanceId, ETCGEffectTrigger Trigger)
{
	const FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Trigger == ETCGEffectTrigger::None) return false;

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Trigger %s on %s"), GetTCGEffectTriggerDebugName(Trigger), *Card->CardDefinitionId.ToString());

	return true;
}

int32 ATCG_GameState::GetDebugEffectsForCardTrigger(const FTCGCardInstance& Card, ETCGEffectTrigger Trigger, TArray<FName>& OutEffectIds) const
{
	OutEffectIds.Reset();

	if (Trigger != ETCGEffectTrigger::OnPlay) return 0;

	if (Card.CardDefinitionId == DebugCard_FireDeckB)
	{
		OutEffectIds.Add(DebugEffect_Draw1);
		return OutEffectIds.Num();
	}

	if (Card.CardDefinitionId == DebugCard_FireDeckA)
	{
		OutEffectIds.Add(DebugEffect_GainAttackForCardsUnderneath);
		return OutEffectIds.Num();
	}

	return 0;
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

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain add %d Source=%s Trigger=%s Effect=%s"),
		NewEntry.ChainIndex, *NewEntry.SourceCardDefinitionId.ToString(), GetTCGEffectTriggerDebugName(NewEntry.Trigger), *NewEntry.EffectId.ToString());

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

	if (!SourceCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=MissingSource"),
			ChainEntry.ChainIndex,
			*ChainEntry.SourceCardDefinitionId.ToString(),
			*ChainEntry.EffectId.ToString());

		return false;
	}

	if (!TargetCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=MissingTarget"),
			ChainEntry.ChainIndex,
			*ChainEntry.SourceCardDefinitionId.ToString(),
			*ChainEntry.EffectId.ToString());

		return false;
	}

	if (ChainEntry.bRequiresSourceOnBoard && SourceCard->Location != ETCGCardLocation::Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=SourceNotOnBoard"),
			ChainEntry.ChainIndex,
			*ChainEntry.SourceCardDefinitionId.ToString(),
			*ChainEntry.EffectId.ToString());

		return false;
	}

	if (ChainEntry.bRequiresTargetOnBoard && TargetCard->Location != ETCGCardLocation::Board)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=TargetNotOnBoard"),
			ChainEntry.ChainIndex,
			*ChainEntry.SourceCardDefinitionId.ToString(),
			*ChainEntry.EffectId.ToString());

		return false;
	}

	if (ChainEntry.bRequiresSourceInTargetStack)
	{
		if (!SourceCard->StackId.IsValid() || SourceCard->StackId != TargetCard->StackId)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=SourceNotInTargetStack"),
				ChainEntry.ChainIndex,
				*ChainEntry.SourceCardDefinitionId.ToString(),
				*ChainEntry.EffectId.ToString());

			return false;
		}
	}

	if (ChainEntry.bRequiresSourceUnderTarget)
	{
		if (SourceCard->CardInstanceId == TargetCard->CardInstanceId || SourceCard->StackIndex >= TargetCard->StackIndex)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain fizzle %d Source=%s Effect=%s Reason=SourceNotUnderTarget"),
				ChainEntry.ChainIndex,
				*ChainEntry.SourceCardDefinitionId.ToString(),
				*ChainEntry.EffectId.ToString());

			return false;
		}
	}

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
		GetDebugEffectsForCardTrigger(StackCard, ETCGEffectTrigger::OnPlay, EffectIds);

		for (const FName& EffectId : EffectIds)
		{
			AddCardTriggerToChain(OutChain, StackCard.CardInstanceId, TopCardInstanceId, ETCGEffectTrigger::OnPlay, EffectId);
		}
	}

	if (bEnableDebugOverlayRemovalFizzleTest && TopCard->CardDefinitionId == DebugCard_FireDeckA && OutChain.Num() >= 2)
	{
		AddCardTriggerToChain(OutChain, TopCard->CardInstanceId, TopCardInstanceId,
			ETCGEffectTrigger::OnBecomingTopCard, DebugEffect_RemoveBottomOverlay);
	}

	return OutChain.Num();
}

bool ATCG_GameState::ResolveEffectChain(const TArray<FTCGEffectChainEntry>& Chain)
{
	if (Chain.Num() <= 0) return false;

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain resolving count: %d"), Chain.Num());

	bool bResolvedAny = false;

	for (int32 ChainIndex = Chain.Num() - 1; ChainIndex >= 0; --ChainIndex)
	{
		const FTCGEffectChainEntry& Entry = Chain[ChainIndex];

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain resolve %d Source=%s Effect=%s"),
			Entry.ChainIndex,
			*Entry.SourceCardDefinitionId.ToString(),
			*Entry.EffectId.ToString());

		if (!CanResolveEffectChainEntry(Entry))
		{
			continue;
		}

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

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect Draw 1 from %s success: %s"),
			*SourceCard->CardDefinitionId.ToString(),
			bDrewCard ? TEXT("true") : TEXT("false"));

		return bDrewCard;
	}

	if (ChainEntry.EffectId == DebugEffect_GainAttackForCardsUnderneath)
	{
		FTCGCardInstance* MutableTargetCard = FindCardInstance(ChainEntry.TargetCardInstanceId);
		if (!MutableTargetCard) return false;

		const int32 CardsUnderneath = GetCardsUnderneathCount(ChainEntry.TargetCardInstanceId);
		MutableTargetCard->AttackModifier += CardsUnderneath;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect %s gained ATK from cards underneath: %d modifier now: %d"),
			*MutableTargetCard->CardDefinitionId.ToString(),
			CardsUnderneath,
			MutableTargetCard->AttackModifier);

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

			if (!BottomOverlayCard || Card.StackIndex < BottomOverlayCard->StackIndex)
			{
				BottomOverlayCard = &Card;
			}
		}

		if (!BottomOverlayCard)
		{
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect RemoveBottomOverlay found no overlay under %s"),
				*MutableTargetCard->CardDefinitionId.ToString());

			return false;
		}

		const FName RemovedCardDefinitionId = BottomOverlayCard->CardDefinitionId;
		const bool bMoved = MoveCardToLocation(BottomOverlayCard->CardInstanceId, ETCGCardLocation::Graveyard);

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Chain effect RemoveBottomOverlay moved %s to Graveyard success: %s"),
			*RemovedCardDefinitionId.ToString(),
			bMoved ? TEXT("true") : TEXT("false"));

		return bMoved;
	}

	return false;
}

bool ATCG_GameState::MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card) return false;

	Card->Location = NewLocation;

	// If the card leaves the board, clear its board/stack info for now.
	// Later, graveyard/banish effects can happen here or through a rules system.
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
	if (!StackId.IsValid())
	{
		return false;
	}

	TArray<FGuid> CardIdsInStack;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.StackId == StackId && Card.Location == ETCGCardLocation::Board)
		{
			CardIdsInStack.Add(Card.CardInstanceId);
		}
	}

	if (CardIdsInStack.Num() <= 0)
	{
		return false;
	}

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
		if (Card.OwnerPlayerIndex == PlayerIndex &&
			Card.Location == ETCGCardLocation::Board)
		{
			return true;
		}
	}

	return false;
}

int32 ATCG_GameState::GetCardsUnderneathCount(const FGuid& CardInstanceId) const
{
	const FTCGCardInstance* TargetCard = FindCardInstance(CardInstanceId);
	if (!TargetCard) return 0;

	// Cards not in a stack have no cards underneath.
	if (!TargetCard->StackId.IsValid() || TargetCard->StackIndex <= 0) return 0;

	int32 Count = 0;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		const bool bSameStack = Card.StackId == TargetCard->StackId;
		const bool bUnderneath = Card.StackIndex < TargetCard->StackIndex;

		if (bSameStack && bUnderneath)
		{
			Count++;
		}
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
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId) continue;
		if (!Card.StackId.IsValid()) continue;

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
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == StackId)
		{
			OutCards.Add(Card);
		}
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
		if (Card.Location == ETCGCardLocation::Board && Card.ZoneId == ZoneId)
		{
			OutCards.Add(Card);
		}
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

	if (!Player0Card || !Player1Card)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle failed P0Card=%s P1Card=%s"),
			Player0Card ? *Player0Card->CardDefinitionId.ToString() : TEXT("None"),
			Player1Card ? *Player1Card->CardDefinitionId.ToString() : TEXT("None"));

		return false;
	}

	const FGuid Player0CardId = Player0Card->CardInstanceId;
	const FGuid Player1CardId = Player1Card->CardInstanceId;
	const FGuid Player0StackId = Player0Card->StackId;
	const FGuid Player1StackId = Player1Card->StackId;

	const int32 Player0Attack = GetFinalAttack(Player0CardId);
	const int32 Player1Attack = GetFinalAttack(Player1CardId);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle %s ATK %d vs %s ATK %d"),
		*Player0Card->CardDefinitionId.ToString(),
		Player0Attack,
		*Player1Card->CardDefinitionId.ToString(),
		Player1Attack);

	if (Player0Attack > Player1Attack)
	{
		MoveStackToLocation(Player1StackId, ETCGCardLocation::Graveyard);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result P1 stack sent to Graveyard"));
	}
	else if (Player1Attack > Player0Attack)
	{
		MoveStackToLocation(Player0StackId, ETCGCardLocation::Graveyard);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result P0 stack sent to Graveyard"));
	}
	else
	{
		MoveStackToLocation(Player0StackId, ETCGCardLocation::Graveyard);
		MoveStackToLocation(Player1StackId, ETCGCardLocation::Graveyard);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle result tie, both stacks sent to Graveyard"));
	}

	return true;
}

bool ATCG_GameState::ResolveBattlePhase()
{
	bool bResolvedAnyBattle = false;

	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const FName Player0ZoneId = GetFieldZoneId(0, FieldIndex);
		const FName Player1ZoneId = GetFieldZoneId(1, FieldIndex);

		const FTCGCardInstance* Player0Card = FindTopCardInZone(Player0ZoneId);
		const FTCGCardInstance* Player1Card = FindTopCardInZone(Player1ZoneId);

		if (!Player0Card && !Player1Card)
		{
			continue;
		}

		if (!Player0Card || !Player1Card)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Debug: Battle skipped %s vs %s P0Card=%s P1Card=%s"),
				*Player0ZoneId.ToString(),
				*Player1ZoneId.ToString(),
				Player0Card ? *Player0Card->CardDefinitionId.ToString() : TEXT("None"),
				Player1Card ? *Player1Card->CardDefinitionId.ToString() : TEXT("None"));

			continue;
		}

		if (ResolveBattleBetweenZones(Player0ZoneId, Player1ZoneId))
		{
			bResolvedAnyBattle = true;
		}
	}

	return bResolvedAnyBattle;
}

ETCGMatchResult ATCG_GameState::CheckLoseConditionAfterBattle() const
{
	const bool bPlayer0HasCardOnBoard = DoesPlayerHaveAnyCardOnBoard(0);
	const bool bPlayer1HasCardOnBoard = DoesPlayerHaveAnyCardOnBoard(1);

	if (!bPlayer0HasCardOnBoard && !bPlayer1HasCardOnBoard)
	{
		return ETCGMatchResult::Draw;
	}

	if (!bPlayer0HasCardOnBoard)
	{
		return ETCGMatchResult::Player1Wins;
	}

	if (!bPlayer1HasCardOnBoard)
	{
		return ETCGMatchResult::Player0Wins;
	}

	return ETCGMatchResult::None;
}

void ATCG_GameState::GetCardsInLocation(int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards) const
{
	OutCards.Reset();

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location)
		{
			OutCards.Add(Card);
		}
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
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location)
		{
			HighestIndex = FMath::Max(HighestIndex, Card.LocationIndex);
		}
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

	if (HandCards.Num() <= 0)
	{
		return false;
	}

	HandCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex < B.LocationIndex;
	});

	return PlayCardToZone(HandCards[0].CardInstanceId, ZoneId);
}

void ATCG_GameState::SetupDebugMatch()
{
	MatchCards.Empty();

	AddCardInstance("Debug_Earth_Deck_A", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Earth_Deck_B", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Fire_Deck_A", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Fire_Deck_B", ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Deck);

	AddCardInstance("Debug_Dark_Deck_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Dark_Deck_B", ETCGCardElement::Dark, 2, 1, ETCGCardLocation::Deck);
	AddCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);

	TArray<FTCGCardInstance> Player0Deck;
	TArray<FTCGCardInstance> Player1Deck;
	GetCardsInDeck(0, Player0Deck);
	GetCardsInDeck(1, Player1Deck);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Setup match P0 deck: %d"), Player0Deck.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Setup match P1 deck: %d"), Player1Deck.Num());
}

void ATCG_GameState::RunDebugTurnFlow()
{
	SetPhase(ETCGMatchPhase::Draw);

	const int32 Player0Drawn = DrawCards(0, 2);
	const int32 Player1Drawn = DrawCards(1, 2);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Draw phase P0 drawn: %d"), Player0Drawn);
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Draw phase P1 drawn: %d"), Player1Drawn);
	
	TArray<FTCGCardInstance> Player0HandAfterDraw;
	TArray<FTCGCardInstance> Player1HandAfterDraw;
	TArray<FTCGCardInstance> Player0DeckAfterDraw;
	TArray<FTCGCardInstance> Player1DeckAfterDraw;

	GetCardsInHand(0, Player0HandAfterDraw);
	GetCardsInHand(1, Player1HandAfterDraw);
	GetCardsInDeck(0, Player0DeckAfterDraw);
	GetCardsInDeck(1, Player1DeckAfterDraw);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Draw phase P0 hand: %d deck: %d"), Player0HandAfterDraw.Num(), Player0DeckAfterDraw.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Draw phase P1 hand: %d deck: %d"), Player1HandAfterDraw.Num(), Player1DeckAfterDraw.Num());

	SetPhase(ETCGMatchPhase::Main);

	const FName Player0Field0 = GetFieldZoneId(0, 0);
	const FName Player1Field0 = GetFieldZoneId(1, 0);

	const bool bPlayer0PlayedFirstCard = PlayFirstCardFromHandToZone(0, Player0Field0);
	const bool bPlayer0PlayedSecondCard = PlayFirstCardFromHandToZone(0, Player0Field0);
	const bool bPlayer1PlayedFirstCard = PlayFirstCardFromHandToZone(1, Player1Field0);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P0 play first hand card to %s: %s"),
		*Player0Field0.ToString(),
		bPlayer0PlayedFirstCard ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P0 play second hand card to %s: %s"),
		*Player0Field0.ToString(),
		bPlayer0PlayedSecondCard ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P1 play first hand card to %s: %s"),
		*Player1Field0.ToString(),
		bPlayer1PlayedFirstCard ? TEXT("true") : TEXT("false"));

	const FTCGCardInstance* Player0TopCard = FindTopCardInZone(Player0Field0);
	const FTCGCardInstance* Player1TopCard = FindTopCardInZone(Player1Field0);

	TArray<FTCGCardInstance> Player0HandAfterPlay;
	TArray<FTCGCardInstance> Player1HandAfterPlay;
	GetCardsInHand(0, Player0HandAfterPlay);
	GetCardsInHand(1, Player1HandAfterPlay);
	
	TArray<FTCGCardInstance> Player0DeckAfterPlay;
	TArray<FTCGCardInstance> Player1DeckAfterPlay;
	GetCardsInDeck(0, Player0DeckAfterPlay);
	GetCardsInDeck(1, Player1DeckAfterPlay);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P0 hand after play: %d deck: %d board has card: %s"),
		Player0HandAfterPlay.Num(), Player0DeckAfterPlay.Num(), DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P1 hand after play: %d deck: %d board has card: %s"),
		Player1HandAfterPlay.Num(), Player1DeckAfterPlay.Num(), DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P0 top card: %s stack underneath: %d final ATK: %d"),
		Player0TopCard ? *Player0TopCard->CardDefinitionId.ToString() : TEXT("None"),
		Player0TopCard ? GetCardsUnderneathCount(Player0TopCard->CardInstanceId) : 0,
		Player0TopCard ? GetFinalAttack(Player0TopCard->CardInstanceId) : 0);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Main phase P1 top card: %s stack underneath: %d final ATK: %d"),
		Player1TopCard ? *Player1TopCard->CardDefinitionId.ToString() : TEXT("None"),
		Player1TopCard ? GetCardsUnderneathCount(Player1TopCard->CardInstanceId) : 0,
		Player1TopCard ? GetFinalAttack(Player1TopCard->CardInstanceId) : 0);

	SetPhase(ETCGMatchPhase::Battle);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase started"));

	const bool bBattleResolved = ResolveBattlePhase();

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle after resolve P0 board has card: %s"),
		DoesPlayerHaveAnyCardOnBoard(0) ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle after resolve P1 board has card: %s"),
		DoesPlayerHaveAnyCardOnBoard(1) ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle phase resolved: %s"),
		bBattleResolved ? TEXT("true") : TEXT("false"));

	const ETCGMatchResult BattleMatchResult = CheckLoseConditionAfterBattle();

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle lose check result: %s"),
		BattleMatchResult == ETCGMatchResult::Player0Wins ? TEXT("Player 0 wins") :
		BattleMatchResult == ETCGMatchResult::Player1Wins ? TEXT("Player 1 wins") :
		BattleMatchResult == ETCGMatchResult::Draw ? TEXT("Draw") :
		TEXT("None"));

	if (BattleMatchResult != ETCGMatchResult::None)
	{
		EndMatch(BattleMatchResult);
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Match ended after battle"));
		return;
	}

	SetPhase(ETCGMatchPhase::End);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: End phase reached"));
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
	GetCardsInDeck(0, Player0DeckAfterDraw);

	TArray<FTCGCardInstance> Player0HandAfterDraw;
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
	
	FGuid StackInZone;
	const bool bFoundStack = FindStackIdInZone("Player0_Field_0", StackInZone);

	TArray<FTCGCardInstance> CardsInZone;
	GetCardsInZone("Player0_Field_0", CardsInZone);

	const FTCGCardInstance* TopCard = FindTopCardInZone("Player0_Field_0");

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Found stack in Player0_Field_0: %s"), bFoundStack ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Cards in Player0_Field_0: %d"), CardsInZone.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Top card in Player0_Field_0: %s"), TopCard ? *TopCard->CardDefinitionId.ToString() : TEXT("None"));
	
	TArray<FTCGCardInstance> Player0Hand;
	GetCardsInHand(0, Player0Hand);

	TArray<FTCGCardInstance> Player1Hand;
	GetCardsInHand(1, Player1Hand);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 0 hand cards: %d"), Player0Hand.Num());
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Player 1 hand cards: %d"), Player1Hand.Num());
}
