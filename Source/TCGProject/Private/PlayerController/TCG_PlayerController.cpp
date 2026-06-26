#include "PlayerController/TCG_PlayerController.h"

#include "Cards/TCG_CardDefinition.h"
#include "Cards/TCG_CardTypes.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_GameState.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/TCG_BoardCameraPawn.h"
#include "PlayerState/TCG_PlayerState.h"
#include "UI/TCGMatchHUDWidgetBase.h"

void ATCG_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	CreateMatchHUD();
	ApplyFixedBoardCamera();

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

void ATCG_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATCG_PlayerController::HandleBoardZoneClick);
	}
}

void ATCG_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CreateMatchHUD();
	ApplyFixedBoardCamera();

	if (!bUseDebugHUDData)
	{
		RefreshMatchHUDFromGameState();
	}
}

void ATCG_PlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	CreateMatchHUD();
	ApplyFixedBoardCamera();

	if (!bUseDebugHUDData)
	{
		RefreshMatchHUDFromGameState();
	}
}

void ATCG_PlayerController::ClientSetAssignedPlayerIndex_Implementation(const int32 NewPlayerIndex)
{
	AssignedLocalPlayerIndex = NewPlayerIndex;
	LocalPlayerIndex = NewPlayerIndex;

	CreateMatchHUD();
	ApplyFixedBoardCamera();

	if (!bUseDebugHUDData)
	{
		RefreshMatchHUDFromGameState();
	}
}

void ATCG_PlayerController::CreateMatchHUD()
{
	if (!IsLocalPlayerController())
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
		MatchHUDWidget->OnHUDHandCardSelected.AddUniqueDynamic(this, &ATCG_PlayerController::HandleHUDHandCardSelected);
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

int32 ATCG_PlayerController::ResolveLocalPlayerIndex() const
{
	if (AssignedLocalPlayerIndex != INDEX_NONE)
	{
		return AssignedLocalPlayerIndex;
	}

	const ATCG_PlayerState* TCGPlayerState = GetPlayerState<ATCG_PlayerState>();
	if (TCGPlayerState && TCGPlayerState->PlayerIndex != INDEX_NONE)
	{
		return TCGPlayerState->PlayerIndex;
	}

	return LocalPlayerIndex;
}

void ATCG_PlayerController::ApplyFixedBoardCamera()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	ATCG_BoardCameraPawn* BoardCameraPawn = Cast<ATCG_BoardCameraPawn>(GetPawn());
	if (!BoardCameraPawn)
	{
		return;
	}

	const bool bIsPlayerOne = ResolveLocalPlayerIndex() == 1;
	BoardCameraPawn->SetBoardCameraTransform(
		bIsPlayerOne ? Player1CameraLocation : Player0CameraLocation,
		bIsPlayerOne ? Player1CameraRotation : Player0CameraRotation);
	SetViewTarget(BoardCameraPawn);
}

FTCGCardWidgetData ATCG_PlayerController::FindLocalHandCardDataByHandIndex(const int32 HandIndex) const
{
	if (!MatchHUDWidget)
	{
		return FTCGCardWidgetData();
	}

	const FTCGMatchHUDWidgetData& CurrentHUDData = MatchHUDWidget->GetHUDData();
	for (const FTCGCardWidgetData& CardData : CurrentHUDData.LocalHand.Cards)
	{
		if (CardData.HandIndex == HandIndex)
		{
			return CardData;
		}
	}

	return FTCGCardWidgetData();
}

void ATCG_PlayerController::HandleHUDHandCardSelected(const int32 HandIndex, UObject* SourceObject)
{
	const FTCGCardWidgetData SelectedCardData = FindLocalHandCardDataByHandIndex(HandIndex);
	SelectedHandCardInstanceId = SelectedCardData.CardInstanceId;
	RefreshPlacementHighlights();
}

bool ATCG_PlayerController::CanSelectedHandCardPlayToZone(const FName ZoneId) const
{
	if (!SelectedHandCardInstanceId.IsValid() || !GetWorld())
	{
		return false;
	}

	const ATCG_GameState* TCGGameState = GetWorld()->GetGameState<ATCG_GameState>();
	if (!TCGGameState)
	{
		return false;
	}

	return TCGGameState->CanPlayerPlayCardToZone(ResolveLocalPlayerIndex(), SelectedHandCardInstanceId, ZoneId);
}

TArray<FName> ATCG_PlayerController::GetValidPlacementZoneIdsForSelectedHandCard() const
{
	TArray<FName> ValidZoneIds;
	if (!SelectedHandCardInstanceId.IsValid() || !GetWorld())
	{
		return ValidZoneIds;
	}

	const ATCG_GameState* TCGGameState = GetWorld()->GetGameState<ATCG_GameState>();
	if (!TCGGameState)
	{
		return ValidZoneIds;
	}

	const int32 PlayerIndex = ResolveLocalPlayerIndex();
	for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
		if (TCGGameState->CanPlayerPlayCardToZone(PlayerIndex, SelectedHandCardInstanceId, ZoneId))
		{
			ValidZoneIds.Add(ZoneId);
		}
	}

	return ValidZoneIds;
}

void ATCG_PlayerController::TryPlaySelectedHandCardToZone(const FName ZoneId)
{
	if (!SelectedHandCardInstanceId.IsValid())
	{
		return;
	}

	if (!CanSelectedHandCardPlayToZone(ZoneId))
	{
		return;
	}

	ServerTryPlaySelectedHandCardToZone(SelectedHandCardInstanceId, ZoneId);
}

void ATCG_PlayerController::HandleBoardZoneClick()
{
	if (!SelectedHandCardInstanceId.IsValid() || !IsLocalPlayerController())
	{
		return;
	}

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	FName ZoneId = NAME_None;
	if (!TryResolveZoneIdFromActor(HitActor, ZoneId))
	{
		return;
	}

	TryPlaySelectedHandCardToZone(ZoneId);
}

bool ATCG_PlayerController::TryResolveZoneIdFromActor(const AActor* Actor, FName& OutZoneId) const
{
	if (!Actor)
	{
		return false;
	}

	for (int32 PlayerIndex = 0; PlayerIndex < 2; ++PlayerIndex)
	{
		for (int32 FieldIndex = 0; FieldIndex < ATCG_GameState::FieldZoneCount; ++FieldIndex)
		{
			const FName ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
			if (DoesActorMatchZoneId(Actor, ZoneId))
			{
				OutZoneId = ZoneId;
				return true;
			}
		}
	}

	return false;
}

bool ATCG_PlayerController::DoesActorMatchZoneId(const AActor* Actor, const FName ZoneId) const
{
	if (!Actor || ZoneId.IsNone())
	{
		return false;
	}

	if (Actor->ActorHasTag(ZoneId))
	{
		return true;
	}

	const FString ZoneIdString = ZoneId.ToString();
	if (Actor->GetName().Contains(ZoneIdString))
	{
		return true;
	}

	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (const UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->ComponentHasTag(ZoneId) || Component->GetName().Contains(ZoneIdString))
		{
			return true;
		}
	}

	return false;
}

void ATCG_PlayerController::RefreshPlacementHighlights()
{
	ClearPlacementHighlights();

	if (!bDrawPlacementDebugHighlights || !GetWorld() || !SelectedHandCardInstanceId.IsValid())
	{
		return;
	}

	const TArray<FName> ValidZoneIds = GetValidPlacementZoneIdsForSelectedHandCard();
	if (ValidZoneIds.Num() == 0)
	{
		return;
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), AllActors);

	for (const FName ZoneId : ValidZoneIds)
	{
		for (AActor* Actor : AllActors)
		{
			if (!DoesActorMatchZoneId(Actor, ZoneId))
			{
				continue;
			}

			FVector Origin = Actor->GetActorLocation();
			FVector Extent(80.0f, 80.0f, 10.0f);
			Actor->GetActorBounds(true, Origin, Extent);
			Extent.X = FMath::Max(Extent.X, 60.0f);
			Extent.Y = FMath::Max(Extent.Y, 60.0f);
			Extent.Z = FMath::Max(Extent.Z, 8.0f);

			DrawDebugBox(
				GetWorld(),
				Origin,
				Extent,
				FColor::Yellow,
				true,
				-1.0f,
				0,
				PlacementHighlightLineThickness);
		}
	}
}

void ATCG_PlayerController::ClearPlacementHighlights()
{
	if (GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
}

void ATCG_PlayerController::ServerTryPlaySelectedHandCardToZone_Implementation(const FGuid CardInstanceId, const FName ZoneId)
{
	ATCG_GameState* TCGGameState = GetWorld() ? GetWorld()->GetGameState<ATCG_GameState>() : nullptr;
	if (!TCGGameState)
	{
		return;
	}

	const int32 PlayerIndex = ResolveLocalPlayerIndex();
	if (!TCGGameState->PlayerPlayCardToZone(PlayerIndex, CardInstanceId, ZoneId))
	{
		ClientRefreshMatchHUD();
		return;
	}

	SelectedHandCardInstanceId.Invalidate();
	ClearPlacementHighlights();
	RefreshAllLocalMatchHUDs();
}

void ATCG_PlayerController::ClientRefreshMatchHUD_Implementation()
{
	SelectedHandCardInstanceId.Invalidate();
	ClearPlacementHighlights();
	RefreshMatchHUDFromGameState();
}

void ATCG_PlayerController::RefreshAllLocalMatchHUDs()
{
	if (!GetWorld())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATCG_PlayerController* TCGPlayerController = Cast<ATCG_PlayerController>(It->Get());
		if (!TCGPlayerController)
		{
			continue;
		}

		TCGPlayerController->ClientRefreshMatchHUD();
	}
}

void ATCG_PlayerController::RefreshMatchHUDFromGameState()
{
	if (!IsLocalPlayerController() || !MatchHUDWidget)
	{
		return;
	}

	const ATCG_GameState* TCGGameState = GetWorld() ? GetWorld()->GetGameState<ATCG_GameState>() : nullptr;
	if (!TCGGameState)
	{
		return;
	}

	SetMatchHUDData(BuildHUDDataFromGameState(*TCGGameState, ResolveLocalPlayerIndex()));
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
	CardData.CardInstanceId = Card.CardInstanceId;

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
	if (!IsLocalPlayerController() || !MatchHUDWidget)
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