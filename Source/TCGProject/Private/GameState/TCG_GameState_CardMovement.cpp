#include "GameState/TCG_GameState.h"

bool ATCG_GameState::MoveBottomOverlayToGraveyard(const FGuid& TargetCardInstanceId)
{
	FTCGCardInstance* TargetCard = FindCardInstance(TargetCardInstanceId);
	if (!TargetCard || !TargetCard->StackId.IsValid())
	{
		return false;
	}

	FTCGCardInstance* BottomOverlayCard = nullptr;
	for (FTCGCardInstance& Card : MatchCards)
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

	return BottomOverlayCard && MoveCardToLocation(BottomOverlayCard->CardInstanceId, ETCGCardLocation::Graveyard);
}

bool ATCG_GameState::PlayGraveyardCardToEmptyZone(const FGuid& CardInstanceId, FName ZoneId)
{
	FTCGCardInstance* Card = FindCardInstance(CardInstanceId);
	if (!Card || Card->Location != ETCGCardLocation::Graveyard) return false;
	if (ZoneId.IsNone()) return false;

	FGuid ExistingStackId;
	if (FindStackIdInZone(ZoneId, ExistingStackId)) return false;

	return PlaceCardAsNewStack(CardInstanceId, ZoneId);
}

bool ATCG_GameState::ReturnUnitStackToBottomDeckMaterialsToGraveyard(const FGuid& TargetTopCardInstanceId)
{
	const FTCGCardInstance* TargetTopCard = FindCardInstance(TargetTopCardInstanceId);
	if (!TargetTopCard || TargetTopCard->Location != ETCGCardLocation::Board || !TargetTopCard->StackId.IsValid()) return false;

	const FGuid StackId = TargetTopCard->StackId;
	const FGuid TopCardId = TargetTopCard->CardInstanceId;

	TArray<FGuid> MaterialIds;
	for (const FTCGCardInstance& Card : MatchCards)
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
		bMovedMaterials &= MoveCardToLocation(MaterialId, ETCGCardLocation::Graveyard);
	}

	const bool bMovedTop = MoveCardToBottomOfDeck(TopCardId);
	return bMovedMaterials && bMovedTop;
}

bool ATCG_GameState::ResolveAttackMillTwoWaterBounceBattlingUnit(const FGuid& SourceCardInstanceId, const FGuid& BattleTargetCardInstanceId)
{
	FTCGCardInstance* SourceCard = FindCardInstance(SourceCardInstanceId);
	const FTCGCardInstance* BattleTarget = FindCardInstance(BattleTargetCardInstanceId);
	if (!SourceCard || SourceCard->Location != ETCGCardLocation::Board || !SourceCard->StackId.IsValid()) return false;
	if (!BattleTarget || BattleTarget->Location != ETCGCardLocation::Board || !BattleTarget->StackId.IsValid()) return false;

	if (!MoveBottomOverlayToGraveyard(SourceCardInstanceId))
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Attack mill bounce failed Source=%s Reason=NoMaterial"), *SourceCard->CardDefinitionId.ToString());
		return false;
	}

	TArray<FTCGCardInstance> DeckCards;
	GetCardsInDeck(SourceCard->OwnerPlayerIndex, DeckCards);
	if (DeckCards.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Attack mill bounce failed Player=%d Reason=DeckTooSmall Cards=%d"), SourceCard->OwnerPlayerIndex, DeckCards.Num());
		return false;
	}

	const FGuid FirstMilledId = DeckCards[0].CardInstanceId;
	const FGuid SecondMilledId = DeckCards[1].CardInstanceId;
	const bool bFirstWater = DeckCards[0].Element == ETCGCardElement::Water;
	const bool bSecondWater = DeckCards[1].Element == ETCGCardElement::Water;

	const bool bFirstSent = MoveCardToLocation(FirstMilledId, ETCGCardLocation::Graveyard);
	const bool bSecondSent = MoveCardToLocation(SecondMilledId, ETCGCardLocation::Graveyard);
	const bool bBothSent = bFirstSent && bSecondSent;
	const bool bBothWater = bFirstWater && bSecondWater;
	const bool bReturnedBattleTarget = bBothSent && bBothWater && ReturnUnitStackToBottomDeckMaterialsToGraveyard(BattleTargetCardInstanceId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: Attack mill bounce Source=%s Target=%s Sent=%s BothWater=%s ReturnedTarget=%s"),
		*SourceCard->CardDefinitionId.ToString(),
		*BattleTarget->CardDefinitionId.ToString(),
		bBothSent ? TEXT("true") : TEXT("false"),
		bBothWater ? TEXT("true") : TEXT("false"),
		bReturnedBattleTarget ? TEXT("true") : TEXT("false"));

	return bBothSent;
}
