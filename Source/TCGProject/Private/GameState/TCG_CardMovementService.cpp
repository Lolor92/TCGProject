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

bool UTCG_CardMovementService::MoveBottomOverlayToGraveyard(ATCG_GameState* GameState, const FGuid& TargetCardInstanceId)
{
	if (!GameState) return false;

	FTCGCardInstance* TargetCard = GameState->FindCardInstance(TargetCardInstanceId);
	if (!TargetCard || !TargetCard->StackId.IsValid())
	{
		return false;
	}

	FTCGCardInstance* BottomOverlayCard = nullptr;
	for (FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.StackId != TargetCard->StackId) continue;
		if (Card.CardInstanceId == TargetCard->CardInstanceId) continue;
		if (Card.StackIndex >= TargetCard->StackIndex) continue;
		if (!BottomOverlayCard || Card.StackIndex < BottomOverlayCard->StackIndex)
		{
			BottomOverlayCard = &Card;
		}
	}

	return BottomOverlayCard && MoveCardToLocation(GameState, BottomOverlayCard->CardInstanceId, ETCGCardLocation::Graveyard);
}

bool UTCG_CardMovementService::PlayGraveyardCardToEmptyZone(ATCG_GameState* GameState, const FGuid& CardInstanceId, FName ZoneId)
{
	if (!GameState) return false;

	FTCGCardInstance* Card = GameState->FindCardInstance(CardInstanceId);
	if (!Card || Card->Location != ETCGCardLocation::Graveyard) return false;
	if (ZoneId.IsNone()) return false;

	FGuid ExistingStackId;
	if (GameState->FindStackIdInZone(ZoneId, ExistingStackId)) return false;

	return GameState->PlaceCardAsNewStack(CardInstanceId, ZoneId);
}

bool UTCG_CardMovementService::ReturnUnitStackToBottomDeckMaterialsToGraveyard(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId)
{
	if (!GameState) return false;

	const FTCGCardInstance* TargetTopCard = GameState->FindCardInstance(TargetTopCardInstanceId);
	if (!TargetTopCard || TargetTopCard->Location != ETCGCardLocation::Board || !TargetTopCard->StackId.IsValid()) return false;

	const FGuid StackId = TargetTopCard->StackId;
	const FGuid TopCardId = TargetTopCard->CardInstanceId;

	TArray<FGuid> MaterialIds;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.StackId != StackId) continue;
		if (Card.CardInstanceId == TopCardId) continue;
		if (Card.StackIndex >= TargetTopCard->StackIndex) continue;
		MaterialIds.Add(Card.CardInstanceId);
	}

	bool bMovedMaterials = true;
	for (const FGuid& MaterialId : MaterialIds)
	{
		bMovedMaterials &= MoveCardToLocation(GameState, MaterialId, ETCGCardLocation::Graveyard);
	}

	const bool bMovedTop = GameState->MoveCardToBottomOfDeck(TopCardId);
	return bMovedMaterials && bMovedTop;
}

bool UTCG_CardMovementService::ReturnUnitStackToHandMaterialsToGraveyard(ATCG_GameState* GameState, const FGuid& TargetTopCardInstanceId, int32& OutMaterialCount)
{
	OutMaterialCount = 0;
	if (!GameState) return false;

	const FTCGCardInstance* TargetTopCard = GameState->FindCardInstance(TargetTopCardInstanceId);
	if (!TargetTopCard || TargetTopCard->Location != ETCGCardLocation::Board || !TargetTopCard->StackId.IsValid()) return false;

	const FGuid StackId = TargetTopCard->StackId;
	const FGuid TopCardId = TargetTopCard->CardInstanceId;

	TArray<FGuid> MaterialIds;
	for (const FTCGCardInstance& Card : GameState->MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.StackId != StackId) continue;
		if (Card.CardInstanceId == TopCardId) continue;
		if (Card.StackIndex >= TargetTopCard->StackIndex) continue;
		MaterialIds.Add(Card.CardInstanceId);
	}

	OutMaterialCount = MaterialIds.Num();

	bool bMovedMaterials = true;
	for (const FGuid& MaterialId : MaterialIds)
	{
		bMovedMaterials &= MoveCardToLocation(GameState, MaterialId, ETCGCardLocation::Graveyard);
	}

	const bool bMovedTop = GameState->MoveCardToLocation(TopCardId, ETCGCardLocation::Hand);
	return bMovedMaterials && bMovedTop;
}

bool UTCG_CardMovementService::ResolveAttackMillTwoWaterBounceBattlingUnit(ATCG_GameState* GameState, const FGuid& SourceCardInstanceId, const FGuid& BattleTargetCardInstanceId)
{
	if (!GameState) return false;

	FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
	const FTCGCardInstance* BattleTarget = GameState->FindCardInstance(BattleTargetCardInstanceId);
	if (!SourceCard || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid()) return false;
	if (!BattleTarget || BattleTarget->Location != ETCGCardLocation::Board || !BattleTarget->StackId.IsValid()) return false;

	if (!MoveBottomOverlayToGraveyard(GameState, SourceCardInstanceId))
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Attack mill bounce failed Source=%s Reason=NoMaterial"), *SourceCard->CardDefinitionId.ToString());
		return false;
	}

	TArray<FTCGCardInstance> DeckCards;
	GameState->GetCardsInDeck(SourceCard->OwnerPlayerIndex, DeckCards);
	if (DeckCards.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Attack mill bounce failed Player=%d Reason=DeckTooSmall Cards=%d"), SourceCard->OwnerPlayerIndex, DeckCards.Num());
		return false;
	}

	const FGuid FirstMilledId = DeckCards[0].CardInstanceId;
	const FGuid SecondMilledId = DeckCards[1].CardInstanceId;
	const bool bFirstWater = DeckCards[0].Element == ETCGCardElement::Water;
	const bool bSecondWater = DeckCards[1].Element == ETCGCardElement::Water;

	const bool bFirstSent = MoveCardToLocation(GameState, FirstMilledId, ETCGCardLocation::Graveyard);
	const bool bSecondSent = MoveCardToLocation(GameState, SecondMilledId, ETCGCardLocation::Graveyard);
	const bool bBothSent = bFirstSent && bSecondSent;
	const bool bBothWater = bFirstWater && bSecondWater;
	const bool bReturnedBattleTarget = bBothSent && bBothWater && ReturnUnitStackToBottomDeckMaterialsToGraveyard(GameState, BattleTargetCardInstanceId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: Attack mill bounce Source=%s Target=%s Sent=%s BothWater=%s ReturnedTarget=%s"),
		*SourceCard->CardDefinitionId.ToString(),
		*BattleTarget->CardDefinitionId.ToString(),
		bBothSent ? TEXT("true") : TEXT("false"),
		bBothWater ? TEXT("true") : TEXT("false"),
		bReturnedBattleTarget ? TEXT("true") : TEXT("false"));

	return bBothSent;
}