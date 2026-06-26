#include "PlayerController/TCG_PlayerController.h"

#include "Cards/TCG_CardDefinition.h"
#include "Cards/TCG_CardTypes.h"
#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_GameState.h"
#include "UI/TCGMatchHUDWidgetBase.h"

void ATCG_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateMatchHUD();

	if (bUseDebugHUDData)
	{
		PushDebugHUDData();
	}
	else
	{
		SeedDebugMatchForHUDIfNeeded();
		RefreshMatchHUDFromGameState();
	}
}

void ATCG_PlayerController::CreateMatchHUD()
{
	if (!IsLocalController())
	{
		return;
	}

	if (MatchHUDWidget || !MatchHUDWidgetClass)
	{
		return;
	}

	MatchHUDWidget = CreateWidget<UTCGMatchHUDWidgetBase>(this, MatchHUDWidgetClass);
	if (MatchHUDWidget)
	{
		MatchHUDWidget->AddToViewport();
	}
}

void ATCG_PlayerController::SetMatchHUDData(const FTCGMatchHUDWidgetData& InHUDData)
{
	if (MatchHUDWidget)
	{
		MatchHUDWidget->SetHUDData(InHUDData);
	}
}

void ATCG_PlayerController::SeedDebugMatchForHUDIfNeeded()
{
	if (!bSeedDebugMatchWhenEmpty || !HasAuthority() || !GetWorld())
	{
		return;
	}

	ATCG_GameState* TCGGameState = GetWorld()->GetGameState<ATCG_GameState>();
	if (!TCGGameState || TCGGameState->MatchCards.Num() > 0)
	{
		return;
	}

	UTCG_DebugScenarioRunner::SetupDebugMatch(TCGGameState);
	TCGGameState->StartMatch();
	TCGGameState->DrawCards(0, TCGGameState->InitialHandSize);
	TCGGameState->DrawCards(1, TCGGameState->InitialHandSize);
	TCGGameState->SetPhase(ETCGMatchPhase::Placement);
	TCGGameState->SetCurrentTurnPlayer(0);

	UE_LOG(LogTemp, Warning, TEXT("TCG UI: Seeded real debug match data for HUD preview"));
}

void ATCG_PlayerController::RefreshMatchHUDFromGameState()
{
	if (!IsLocalController() || !MatchHUDWidget)
	{
		return;
	}

	const ATCG_GameState* TCGGameState = GetWorld() ? GetWorld()->GetGameState<ATCG_GameState>() : nullptr;
	if (!TCGGameState)
	{
		return;
	}

	SetMatchHUDData(BuildHUDDataFromGameState(*TCGGameState, LocalPlayerIndex));
}

FTCGMatchHUDWidgetData ATCG_PlayerController::BuildHUDDataFromGameState(const ATCG_GameState& TCGGameState, int32 ForPlayerIndex) const
{
	FTCGMatchHUDWidgetData Data;

	const int32 LocalIndex = TCGGameState.IsValidPlayerIndex(ForPlayerIndex) ? ForPlayerIndex : 0;
	const int32 OpponentIndex = 1 - LocalIndex;

	TArray<FTCGCardInstance> LocalHandCards;
	TCGGameState.GetCardsInHand(LocalIndex, LocalHandCards);

	Data.LocalHand.Cards.Reserve(LocalHandCards.Num());
	for (int32 HandIndex = 0; HandIndex < LocalHandCards.Num(); ++HandIndex)
	{
		Data.LocalHand.Cards.Add(BuildCardWidgetDataFromCard(TCGGameState, LocalHandCards[HandIndex], HandIndex));
	}
	Data.LocalHand.SelectedHandIndex = INDEX_NONE;

	TArray<FTCGCardInstance> LocalDeckCards;
	TArray<FTCGCardInstance> LocalGraveyardCards;
	TArray<FTCGCardInstance> LocalBanishedCards;
	TArray<FTCGCardInstance> OpponentHandCards;
	TArray<FTCGCardInstance> OpponentDeckCards;
	TArray<FTCGCardInstance> OpponentGraveyardCards;
	TArray<FTCGCardInstance> OpponentBanishedCards;

	TCGGameState.GetCardsInDeck(LocalIndex, LocalDeckCards);
	TCGGameState.GetCardsInLocation(LocalIndex, ETCGCardLocation::Graveyard, LocalGraveyardCards);
	TCGGameState.GetCardsInLocation(LocalIndex, ETCGCardLocation::Banish, LocalBanishedCards);
	TCGGameState.GetCardsInHand(OpponentIndex, OpponentHandCards);
	TCGGameState.GetCardsInDeck(OpponentIndex, OpponentDeckCards);
	TCGGameState.GetCardsInLocation(OpponentIndex, ETCGCardLocation::Graveyard, OpponentGraveyardCards);
	TCGGameState.GetCardsInLocation(OpponentIndex, ETCGCardLocation::Banish, OpponentBanishedCards);

	Data.LocalDeckCount = LocalDeckCards.Num();
	Data.LocalGraveyardCount = LocalGraveyardCards.Num();
	Data.LocalBanishedCount = LocalBanishedCards.Num();
	Data.OpponentHandCount = OpponentHandCards.Num();
	Data.OpponentDeckCount = OpponentDeckCards.Num();
	Data.OpponentGraveyardCount = OpponentGraveyardCards.Num();
	Data.OpponentBanishedCount = OpponentBanishedCards.Num();

	Data.PhaseText = FText::FromString(UEnum::GetValueAsString(TCGGameState.CurrentPhase));
	Data.TurnPlayerText = TCGGameState.CurrentTurnPlayerIndex == INDEX_NONE
		? FText::FromString(TEXT("No active player"))
		: FText::Format(NSLOCTEXT("TCGMatchHUD", "TurnPlayerFormat", "Player {0}"), FText::AsNumber(TCGGameState.CurrentTurnPlayerIndex));

	return Data;
}

FTCGCardWidgetData ATCG_PlayerController::BuildCardWidgetDataFromCard(const ATCG_GameState& TCGGameState, const FTCGCardInstance& Card, int32 HandIndex) const
{
	FTCGCardWidgetData CardData;

	const UTCG_CardDefinition* CardDefinition = TCGGameState.FindDebugCardDefinitionById(Card.CardDefinitionId);
	CardData.CardName = CardDefinition && !CardDefinition->DisplayName.IsEmpty()
		? CardDefinition->DisplayName
		: FText::FromName(Card.CardDefinitionId);
	CardData.Attack = TCGGameState.GetFinalAttack(Card.CardInstanceId);
	CardData.Element = GetCardElementName(Card.Element);
	CardData.HandIndex = HandIndex;
	CardData.StackCount = 1;
	CardData.MaterialCount = TCGGameState.GetCardsUnderneathCount(Card.CardInstanceId);
	CardData.bIsTopCard = true;
	CardData.bIsPlayable = false;

	for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = ATCG_GameState::GetFieldZoneId(Card.OwnerPlayerIndex, FieldIndex);
		if (TCGGameState.CanPlayerPlayCardToZone(Card.OwnerPlayerIndex, Card.CardInstanceId, ZoneId))
		{
			CardData.bIsPlayable = true;
			break;
		}
	}

	CardData.bCanOverlay = false;
	CardData.ZoneLabel = Card.ZoneId.IsNone() ? FText::FromString(TEXT("Hand")) : FText::FromName(Card.ZoneId);

	if (CardDefinition)
	{
		for (const FTCGCardEffectRef& EffectRef : CardDefinition->Effects)
		{
			FTCGUIEffectLine EffectLine;
			EffectLine.TriggerName = FName(*UEnum::GetValueAsString(EffectRef.Trigger));
			EffectLine.EffectText = CardDefinition->Description;
			CardData.Effects.Add(EffectLine);
		}
	}

	return CardData;
}

FName ATCG_PlayerController::GetCardElementName(ETCGCardElement Element) const
{
	switch (Element)
	{
	case ETCGCardElement::Fire:
		return FName(TEXT("Fire"));
	case ETCGCardElement::Wind:
		return FName(TEXT("Wind"));
	case ETCGCardElement::Earth:
		return FName(TEXT("Earth"));
	case ETCGCardElement::Water:
		return FName(TEXT("Water"));
	case ETCGCardElement::Light:
		return FName(TEXT("Light"));
	case ETCGCardElement::Dark:
		return FName(TEXT("Dark"));
	default:
		return NAME_None;
	}
}

void ATCG_PlayerController::PushDebugHUDData()
{
	if (!IsLocalController() || !MatchHUDWidget)
	{
		return;
	}

	FTCGMatchHUDWidgetData Data;

	FTCGCardWidgetData FireCard;
	FireCard.CardName = FText::FromString(TEXT("Fire Cub"));
	FireCard.Attack = 2;
	FireCard.Element = FName(TEXT("Fire"));
	FireCard.HandIndex = 0;
	FireCard.bIsPlayable = true;

	FTCGCardWidgetData WindCard;
	WindCard.CardName = FText::FromString(TEXT("Wind Sprite"));
	WindCard.Attack = 1;
	WindCard.Element = FName(TEXT("Wind"));
	WindCard.HandIndex = 1;
	WindCard.bIsPlayable = true;

	FTCGCardWidgetData EarthCard;
	EarthCard.CardName = FText::FromString(TEXT("Earth Golem"));
	EarthCard.Attack = 4;
	EarthCard.Element = FName(TEXT("Earth"));
	EarthCard.HandIndex = 2;
	EarthCard.bIsPlayable = true;

	Data.LocalHand.Cards = { FireCard, WindCard, EarthCard };
	Data.LocalHand.SelectedHandIndex = INDEX_NONE;
	Data.OpponentHandCount = 4;
	Data.PhaseText = FText::FromString(TEXT("Debug UI"));
	Data.TurnPlayerText = FText::FromString(TEXT("Local Player"));

	SetMatchHUDData(Data);
}