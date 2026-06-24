#include "GameState/TCG_GameState.h"

namespace
{
	const FName DebugCard_BattleDestroyReveal = "Debug_Water_BattleDestroyReveal";

	FTCGCardInstance* FindTopDeckCard(ATCG_GameState* GameState, int32 PlayerIndex)
	{
		if (!GameState || !GameState->IsValidPlayerIndex(PlayerIndex)) return nullptr;

		FTCGCardInstance* TopDeckCard = nullptr;
		for (FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.OwnerPlayerIndex != PlayerIndex || Card.Location != ETCGCardLocation::Deck) continue;
			if (!TopDeckCard || Card.LocationIndex > TopDeckCard->LocationIndex) TopDeckCard = &Card;
		}
		return TopDeckCard;
	}

	TArray<FGuid> GetTopDeckCardIds(ATCG_GameState* GameState, int32 PlayerIndex, int32 Count)
	{
		TArray<FTCGCardInstance> DeckCards;
		GameState->GetCardsInDeck(PlayerIndex, DeckCards);

		TArray<FGuid> RevealedCardIds;
		for (int32 Index = 0; Index < DeckCards.Num() && RevealedCardIds.Num() < Count; ++Index)
		{
			RevealedCardIds.Add(DeckCards[Index].CardInstanceId);
		}
		return RevealedCardIds;
	}

	int32 CountPlayerCardsInLocation(ATCG_GameState* GameState, int32 PlayerIndex, ETCGCardLocation Location)
	{
		int32 Count = 0;
		for (const FTCGCardInstance& Card : GameState->MatchCards)
		{
			if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location) Count++;
		}
		return Count;
	}
}

bool ATCG_GameState::RevealTopDeckCardsAddElementToHand(int32 PlayerIndex, int32 Count, const FTCGEffectTargetFilter& TargetFilter)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0) return false;

	const TArray<FGuid> RevealedCardIds = GetTopDeckCardIds(this, PlayerIndex, Count);
	if (RevealedCardIds.Num() <= 0) return false;

	FGuid ChosenCardId;
	for (const FGuid& RevealedCardId : RevealedCardIds)
	{
		const FTCGCardInstance* RevealedCard = FindCardInstance(RevealedCardId);
		if (!RevealedCard) continue;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Revealed top deck card Player=%d Card=%s Element=%d"),
			PlayerIndex,
			*RevealedCard->CardDefinitionId.ToString(),
			static_cast<int32>(RevealedCard->Element));

		const bool bMatchesElement = !TargetFilter.bRequireElement || RevealedCard->Element == TargetFilter.RequiredElement;
		if (!ChosenCardId.IsValid() && bMatchesElement)
		{
			ChosenCardId = RevealedCardId;
		}
	}

	if (!ChosenCardId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Reveal found no matching card, cards stay on top deck in same order Player=%d Revealed=%d"), PlayerIndex, RevealedCardIds.Num());
		return false;
	}

	return SubmitPendingRevealDeckChoice(PlayerIndex, ChosenCardId);
}

bool ATCG_GameState::BeginPendingRevealDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectTargetFilter& TargetFilter, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingRevealDeckChoice.bIsPending) return false;

	const TArray<FGuid> RevealedCardIds = GetTopDeckCardIds(this, PlayerIndex, Count);
	if (RevealedCardIds.Num() <= 0) return false;

	PendingRevealDeckChoice.Reset();
	PendingRevealDeckChoice.bIsPending = true;
	PendingRevealDeckChoice.PlayerIndex = PlayerIndex;
	PendingRevealDeckChoice.RequiredCount = 1;
	PendingRevealDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingRevealDeckChoice.ChainIndex = ChainEntry.ChainIndex;
	PendingRevealDeckChoice.RevealedCardInstanceIds = RevealedCardIds;

	for (const FGuid& RevealedCardId : RevealedCardIds)
	{
		const FTCGCardInstance* RevealedCard = FindCardInstance(RevealedCardId);
		if (!RevealedCard) continue;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Revealed top deck card Player=%d Card=%s Element=%d"),
			PlayerIndex,
			*RevealedCard->CardDefinitionId.ToString(),
			static_cast<int32>(RevealedCard->Element));

		const bool bMatchesElement = !TargetFilter.bRequireElement || RevealedCard->Element == TargetFilter.RequiredElement;
		if (bMatchesElement)
		{
			PendingRevealDeckChoice.EligibleCardInstanceIds.Add(RevealedCardId);
		}
	}

	if (PendingRevealDeckChoice.EligibleCardInstanceIds.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Reveal choice found no matching card, cards stay on top deck in same order Player=%d Revealed=%d"), PlayerIndex, RevealedCardIds.Num());
		ClearPendingRevealDeckChoice();
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending reveal deck choice started Player=%d Revealed=%d Eligible=%d"),
		PlayerIndex,
		PendingRevealDeckChoice.RevealedCardInstanceIds.Num(),
		PendingRevealDeckChoice.EligibleCardInstanceIds.Num());

	return true;
}

bool ATCG_GameState::SubmitPendingRevealDeckChoice(int32 PlayerIndex, const FGuid& ChosenCardInstanceId)
{
	if (!PendingRevealDeckChoice.bIsPending || PendingRevealDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenCardInstanceId.IsValid() || !PendingRevealDeckChoice.EligibleCardInstanceIds.Contains(ChosenCardInstanceId)) return false;

	const TArray<FGuid> RevealedCardIds = PendingRevealDeckChoice.RevealedCardInstanceIds;
	const FTCGCardInstance* ChosenCardBeforeMove = FindCardInstance(ChosenCardInstanceId);
	const FName ChosenCardDefinitionId = ChosenCardBeforeMove ? ChosenCardBeforeMove->CardDefinitionId : NAME_None;

	if (!MoveCardToLocation(ChosenCardInstanceId, ETCGCardLocation::Hand)) return false;

	int32 SentOtherCount = 0;
	for (const FGuid& RevealedCardId : RevealedCardIds)
	{
		if (RevealedCardId == ChosenCardInstanceId) continue;
		const FTCGCardInstance* OtherCardBeforeMove = FindCardInstance(RevealedCardId);
		const FName OtherCardDefinitionId = OtherCardBeforeMove ? OtherCardBeforeMove->CardDefinitionId : NAME_None;
		if (MoveCardToLocation(RevealedCardId, ETCGCardLocation::Graveyard))
		{
			SentOtherCount++;
			UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Reveal sent other card to graveyard Player=%d Card=%s"), PlayerIndex, *OtherCardDefinitionId.ToString());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Reveal added chosen card to hand Player=%d Card=%s SentOther=%d"),
		PlayerIndex,
		*ChosenCardDefinitionId.ToString(),
		SentOtherCount);

	ClearPendingRevealDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingRevealDeckChoice() const { return PendingRevealDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingRevealDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingRevealDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::GetPendingRevealDeckChoiceRevealedCards(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingRevealDeckChoice.RevealedCardInstanceIds; }
void ATCG_GameState::ClearPendingRevealDeckChoice() { PendingRevealDeckChoice.Reset(); }

namespace
{
	bool SendTopDeckCardToGraveyardAndResolveSelfMill(ATCG_GameState* GameState, int32 PlayerIndex)
	{
		FTCGCardInstance* TopDeckCard = FindTopDeckCard(GameState, PlayerIndex);
		if (!TopDeckCard) return false;

		const FGuid SentCardId = TopDeckCard->CardInstanceId;
		const FName SentCardDefinitionId = TopDeckCard->CardDefinitionId;
		const bool bWasBattleDestroyRevealCard = SentCardDefinitionId == DebugCard_BattleDestroyReveal;

		if (!GameState->MoveCardToLocation(SentCardId, ETCGCardLocation::Graveyard)) return false;

		UE_LOG(LogTemp, Warning, TEXT("TCG Debug: Battle destroy effect sent top deck card to graveyard Player=%d Card=%s"),
			PlayerIndex,
			*SentCardDefinitionId.ToString());

		if (bWasBattleDestroyRevealCard)
		{
			FTCGEffectTargetFilter WaterFilter;
			WaterFilter.bRequireElement = true;
			WaterFilter.RequiredElement = ETCGCardElement::Water;
			GameState->RevealTopDeckCardsAddElementToHand(PlayerIndex, 2, WaterFilter);
		}
		return true;
	}
}

void RunDebugBattleDestroyRevealScenario(ATCG_GameState* GameState)
{
	if (!GameState) return;

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: BattleDestroyReveal scenario start"));
	GameState->MatchCards.Empty();
	GameState->StartMatch();
	GameState->SetPhase(ETCGMatchPhase::Battle);

	FTCGCardInstance& Attacker = GameState->AddCardInstance("Debug_Water_BattleWinner", ETCGCardElement::Water, 5, 0, ETCGCardLocation::Board);
	Attacker.ZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
	Attacker.StackId = FGuid::NewGuid();
	Attacker.StackIndex = 0;

	FTCGCardInstance& Defender = GameState->AddCardInstance("Debug_Fire_BattleLoser", ETCGCardElement::Fire, 1, 1, ETCGCardLocation::Board);
	Defender.ZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
	Defender.StackId = FGuid::NewGuid();
	Defender.StackIndex = 0;

	GameState->AddCardInstance("Debug_Earth_RevealOther", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
	GameState->AddCardInstance("Debug_Water_RevealPick", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck);
	GameState->AddCardInstance(DebugCard_BattleDestroyReveal, ETCGCardElement::Water, 2, 0, ETCGCardLocation::Deck);

	const int32 HandBefore = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Hand);
	const int32 GraveBefore = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Graveyard);
	const int32 DeckBefore = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Deck);

	const bool bBattleResolved = GameState->ResolveBattleBetweenZones(Attacker.ZoneId, Defender.ZoneId);
	const bool bDefenderDestroyed = !GameState->FindTopCardInZone(Defender.ZoneId);
	const bool bMillResolved = bBattleResolved && bDefenderDestroyed && SendTopDeckCardToGraveyardAndResolveSelfMill(GameState, 0);

	const int32 HandAfter = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Hand);
	const int32 GraveAfter = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Graveyard);
	const int32 DeckAfter = CountPlayerCardsInLocation(GameState, 0, ETCGCardLocation::Deck);

	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: BattleDestroyReveal summary Battle=%s DefenderDestroyed=%s Mill=%s Deck=%d/%d Hand=%d/%d Grave=%d/%d"),
		bBattleResolved ? TEXT("true") : TEXT("false"),
		bDefenderDestroyed ? TEXT("true") : TEXT("false"),
		bMillResolved ? TEXT("true") : TEXT("false"),
		DeckBefore,
		DeckAfter,
		HandBefore,
		HandAfter,
		GraveBefore,
		GraveAfter);

	GameState->EndMatch(ETCGMatchResult::Draw);
}
