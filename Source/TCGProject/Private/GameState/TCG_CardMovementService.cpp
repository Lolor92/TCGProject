#include "GameState/TCG_CardMovementService.h"
#include "GameState/TCG_GameState.h"

bool UTCG_CardMovementService::MoveCardToLocation(ATCG_GameState* GameState, const FGuid& CardInstanceId, ETCGCardLocation NewLocation)
{
	if (!GameState) return false;

	FTCGCardInstance* Card = GameState->FindCardInstance(CardInstanceId);
	if (!Card) return false;

	const ETCGCardLocation PreviousLocation = Card->Location;
	const FTCGCardInstance* PreviousTopCard = nullptr;
	if (PreviousLocation == ETCGCardLocation::Board && Card->StackId.IsValid())
	{
		PreviousTopCard = GameState->FindTopCardInStack(Card->StackId);
	}

	const bool bWasMaterial =
		PreviousLocation == ETCGCardLocation::Board
		&& Card->StackId.IsValid()
		&& PreviousTopCard
		&& PreviousTopCard->CardInstanceId != CardInstanceId;

	if (PreviousLocation == NewLocation)
	{
		return true;
	}

	Card->Location = NewLocation;
	Card->LocationIndex = GameState->GetNextLocationIndex(Card->OwnerPlayerIndex, NewLocation);

	if (NewLocation != ETCGCardLocation::Board)
	{
		Card->ZoneId = NAME_None;
		Card->StackId.Invalidate();
		Card->StackIndex = INDEX_NONE;
	}

	if (NewLocation == ETCGCardLocation::Graveyard && PreviousLocation != ETCGCardLocation::Graveyard)
	{
		GameState->HandleCardSentToGraveyard(CardInstanceId);

		ETCGEffectTrigger SpecificGraveyardTrigger = ETCGEffectTrigger::None;
		if (PreviousLocation == ETCGCardLocation::Deck)
		{
			SpecificGraveyardTrigger = ETCGEffectTrigger::OnSentFromDeckToGraveyard;
		}
		else if (PreviousLocation == ETCGCardLocation::Hand)
		{
			SpecificGraveyardTrigger = ETCGEffectTrigger::OnSentFromHandToGraveyard;
		}
		else if (bWasMaterial)
		{
			SpecificGraveyardTrigger = ETCGEffectTrigger::OnSentFromMaterialToGraveyard;
		}
		else if (PreviousLocation == ETCGCardLocation::Board)
		{
			SpecificGraveyardTrigger = ETCGEffectTrigger::OnSentFromBoardToGraveyard;
		}

		TArray<FTCGEffectChainEntry> GraveyardChain;

		const FTCGCardInstance* MovedCard = GameState->FindCardInstance(CardInstanceId);
		if (MovedCard)
		{
			TArray<FTCGCardEffectRef> EffectRefs;
			GameState->GetPrintedEffectRefsForCard(*MovedCard, EffectRefs);

			for (const FTCGCardEffectRef& EffectRef : EffectRefs)
			{
				if (GameState->DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnSentToGraveyard))
				{
					GameState->AddCardEffectRefToChain(GraveyardChain, CardInstanceId, CardInstanceId, EffectRef);
				}

				if (SpecificGraveyardTrigger != ETCGEffectTrigger::None
					&& GameState->DoesCardEffectMatchTrigger(EffectRef, SpecificGraveyardTrigger))
				{
					GameState->AddCardEffectRefToChain(GraveyardChain, CardInstanceId, CardInstanceId, EffectRef);
				}
			}
		}

		GameState->ResolveEffectChain(GraveyardChain);
	}

	return true;
}

bool UTCG_CardMovementService::MoveStackToLocation(ATCG_GameState* GameState, const FGuid& StackId, ETCGCardLocation NewLocation)
{
	if (!GameState || !StackId.IsValid()) return false;

	TArray<FGuid> CardIdsInStack;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.StackId == StackId && Card.Location == ETCGCardLocation::Board)
		{
			CardIdsInStack.Add(Card.CardInstanceId);
		}
	}

	if (CardIdsInStack.Num() <= 0) return false;

	bool bMovedAllCards = true;
	for (const FGuid& CardId : CardIdsInStack)
	{
		bMovedAllCards &= MoveCardToLocation(GameState, CardId, NewLocation);
	}

	return bMovedAllCards;
}
