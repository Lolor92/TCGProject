#include "GameState/TCG_CardQueryService.h"
#include "GameState/TCG_GameState.h"

FTCGCardInstance* UTCG_CardQueryService::FindCardInstance(ATCG_GameState* GameState, const FGuid& CardInstanceId)
{
	if (!GameState) return nullptr;

	for (FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId) return &Card;
	}
	return nullptr;
}

const FTCGCardInstance* UTCG_CardQueryService::FindCardInstance(const ATCG_GameState* GameState, const FGuid& CardInstanceId)
{
	if (!GameState) return nullptr;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.CardInstanceId == CardInstanceId) return &Card;
	}
	return nullptr;
}

FTCGCardInstance* UTCG_CardQueryService::FindTopCardInStack(ATCG_GameState* GameState, const FGuid& StackId)
{
	if (!GameState || !StackId.IsValid()) return nullptr;

	FTCGCardInstance* TopCard = nullptr;
	for (FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.StackId != StackId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}
	return TopCard;
}

const FTCGCardInstance* UTCG_CardQueryService::FindTopCardInStack(const ATCG_GameState* GameState, const FGuid& StackId)
{
	if (!GameState || !StackId.IsValid()) return nullptr;

	const FTCGCardInstance* TopCard = nullptr;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.StackId != StackId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}
	return TopCard;
}

bool UTCG_CardQueryService::DoesPlayerHaveAnyCardOnBoard(const ATCG_GameState* GameState, int32 PlayerIndex)
{
	if (!GameState) return false;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == ETCGCardLocation::Board) return true;
	}
	return false;
}

int32 UTCG_CardQueryService::GetCardsUnderneathCount(const ATCG_GameState* GameState, const FGuid& CardInstanceId)
{
	const FTCGCardInstance* TargetCard = FindCardInstance(GameState, CardInstanceId);
	if (!TargetCard || !TargetCard->StackId.IsValid() || TargetCard->StackIndex <= 0) return 0;

	int32 Count = 0;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == TargetCard->StackId && Card.StackIndex < TargetCard->StackIndex)
		{
			Count++;
		}
	}

	return Count;
}

int32 UTCG_CardQueryService::GetFinalAttack(const ATCG_GameState* GameState, const FGuid& CardInstanceId)
{
	const FTCGCardInstance* Card = FindCardInstance(GameState, CardInstanceId);
	if (!GameState || !Card) return 0;

	int32 FinalAttack = Card->BaseAttack + Card->AttackModifier + GetCardsUnderneathCount(GameState, CardInstanceId);

	if (!Card->StackId.IsValid())
	{
		return FinalAttack;
	}

	TArray<FTCGCardInstance> StackCards;
	GetCardsInStack(GameState, Card->StackId, StackCards);

	for (const FTCGCardInstance& StackCard : StackCards)
	{
		if (StackCard.StackIndex > Card->StackIndex)
		{
			continue;
		}

		TArray<FTCGCardEffectRef> EffectRefs;
		GameState->GetPrintedEffectRefsForCard(StackCard, EffectRefs);

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
					FinalAttack += GetCardsUnderneathCount(GameState, StackCard.CardInstanceId);
				}
				else if (Step.ValueMode == ETCGEffectValueMode::CardsUnderneathTarget)
				{
					FinalAttack += GetCardsUnderneathCount(GameState, CardInstanceId);
				}
				else if (Step.ValueMode == ETCGEffectValueMode::ElementCardsInControllerGraveyard)
				{
					FinalAttack += GameState->CountCardsInLocationByElement(
						Card->OwnerPlayerIndex,
						ETCGCardLocation::Graveyard,
						Step.TargetFilter.RequiredElement);
				}
			}
		}
	}

	return FinalAttack;
}

bool UTCG_CardQueryService::FindStackIdInZone(const ATCG_GameState* GameState, FName ZoneId, FGuid& OutStackId)
{
	OutStackId.Invalidate();
	if (!GameState) return false;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId || !Card.StackId.IsValid()) continue;

		OutStackId = Card.StackId;
		return true;
	}

	return false;
}

void UTCG_CardQueryService::GetCardsInStack(const ATCG_GameState* GameState, const FGuid& StackId, TArray<FTCGCardInstance>& OutCards)
{
	OutCards.Reset();
	if (!GameState) return;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.StackId == StackId) OutCards.Add(Card);
	}

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

void UTCG_CardQueryService::GetCardsInZone(const ATCG_GameState* GameState, FName ZoneId, TArray<FTCGCardInstance>& OutCards)
{
	OutCards.Reset();
	if (!GameState) return;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location == ETCGCardLocation::Board && Card.ZoneId == ZoneId) OutCards.Add(Card);
	}

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.StackIndex < B.StackIndex;
	});
}

const FTCGCardInstance* UTCG_CardQueryService::FindTopCardInZone(const ATCG_GameState* GameState, FName ZoneId)
{
	if (!GameState) return nullptr;

	const FTCGCardInstance* TopCard = nullptr;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board || Card.ZoneId != ZoneId) continue;
		if (!TopCard || Card.StackIndex > TopCard->StackIndex) TopCard = &Card;
	}

	return TopCard;
}

void UTCG_CardQueryService::GetCardsInLocation(const ATCG_GameState* GameState, int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards)
{
	OutCards.Reset();
	if (!GameState) return;

	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location) OutCards.Add(Card);
	}
}

void UTCG_CardQueryService::GetCardsInHand(const ATCG_GameState* GameState, int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards)
{
	GetCardsInLocation(GameState, PlayerIndex, ETCGCardLocation::Hand, OutCards);

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex < B.LocationIndex;
	});
}

void UTCG_CardQueryService::GetCardsInDeck(const ATCG_GameState* GameState, int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards)
{
	GetCardsInLocation(GameState, PlayerIndex, ETCGCardLocation::Deck, OutCards);

	OutCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B)
	{
		return A.LocationIndex > B.LocationIndex;
	});
}

int32 UTCG_CardQueryService::GetNextLocationIndex(const ATCG_GameState* GameState, int32 PlayerIndex, ETCGCardLocation Location)
{
	if (!GameState) return 0;

	int32 HighestIndex = INDEX_NONE;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.OwnerPlayerIndex == PlayerIndex && Card.Location == Location)
		{
			HighestIndex = FMath::Max(HighestIndex, Card.LocationIndex);
		}
	}

	return HighestIndex + 1;
}
