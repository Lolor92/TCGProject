#include "PlayerController/TCG_PlayerController.h"

#include "Board/TCG_CardZoneActor.h"

#include "Cards/TCG_CardDefinition.h"
#include "Cards/TCG_CardTypes.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Actor.h"
#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_GameState.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/TCG_BoardCameraPawn.h"
#include "PlayerState/TCG_PlayerState.h"
#include "TimerManager.h"
#include "UI/TCGMatchHUDWidgetBase.h"

namespace
{
	void TCGScreenDebug(const UObject* WorldContextObject, const FString& Message, const FColor& Color = FColor::Yellow)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, Color, Message);
		}

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	}

	FString NormalizeTCGZoneText(FString Text)
	{
		Text.ToLowerInline();

		FString Normalized;
		Normalized.Reserve(Text.Len());
		for (const TCHAR Character : Text)
		{
			if (FChar::IsAlnum(Character))
			{
				Normalized.AppendChar(Character);
			}
		}

		return Normalized;
	}

	bool ParseTCGFieldZoneId(const FName ZoneId, int32& OutPlayerIndex, int32& OutFieldIndex)
	{
		FString PlayerPart;
		FString FieldPart;
		if (!ZoneId.ToString().Split(TEXT("_Field_"), &PlayerPart, &FieldPart))
		{
			return false;
		}

		PlayerPart.RemoveFromStart(TEXT("Player"));
		return LexTryParseString(OutPlayerIndex, *PlayerPart)
			&& LexTryParseString(OutFieldIndex, *FieldPart);
	}

	TArray<FString> BuildTCGZoneAliases(const FName ZoneId)
	{
		TArray<FString> Aliases;
		Aliases.Add(NormalizeTCGZoneText(ZoneId.ToString()));

		int32 PlayerIndex = INDEX_NONE;
		int32 FieldIndex = INDEX_NONE;
		if (!ParseTCGFieldZoneId(ZoneId, PlayerIndex, FieldIndex))
		{
			return Aliases;
		}

		const TArray<FString> RawAliases = {
			FString::Printf(TEXT("Player%d_Field_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%dField%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%d Field %d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%d_FieldZone_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%dFieldZone%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%d_Zone_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("Player%dZone%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%d_Field_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%dField%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%d_Zone_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%dZone%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%d_FieldZone_%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("P%dFieldZone%d"), PlayerIndex, FieldIndex),
			FString::Printf(TEXT("FieldZone%d"), FieldIndex),
			FString::Printf(TEXT("Field_%d"), FieldIndex),
			FString::Printf(TEXT("Field%d"), FieldIndex),
			FString::Printf(TEXT("Zone_%d"), FieldIndex),
			FString::Printf(TEXT("Zone%d"), FieldIndex),
			FString::Printf(TEXT("Slot_%d"), FieldIndex),
			FString::Printf(TEXT("Slot%d"), FieldIndex)
		};

		for (const FString& RawAlias : RawAliases)
		{
			Aliases.AddUnique(NormalizeTCGZoneText(RawAlias));
		}

		return Aliases;
	}

	bool DoesTextMatchTCGZone(const FString& CandidateText, const FName ZoneId)
	{
		const FString NormalizedCandidate = NormalizeTCGZoneText(CandidateText);
		if (NormalizedCandidate.IsEmpty())
		{
			return false;
		}

		for (const FString& Alias : BuildTCGZoneAliases(ZoneId))
		{
			if (!Alias.IsEmpty() && NormalizedCandidate.Contains(Alias))
			{
				return true;
			}
		}

		return false;
	}
}

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

	// Board zones still handle normal clicks through ATCG_CardZoneActor.
	// Drag placement is finalized here when the player releases the mouse over a zone.
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ATCG_PlayerController::EndHandCardDrag);
	}
}

void ATCG_PlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsDraggingHandCard)
	{
		const bool bLeftMousePhysicallyDown = FSlateApplication::IsInitialized()
			&& FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);

		if (!bLeftMousePhysicallyDown)
		{
			EndHandCardDrag();
			return;
		}
	}

	DrawHandCardDragPreview();
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
		MatchHUDWidget->OnHUDHandCardPressed.AddUniqueDynamic(this, &ATCG_PlayerController::HandleHUDHandCardPressed);
		MatchHUDWidget->OnHUDHandCardReleased.AddUniqueDynamic(this, &ATCG_PlayerController::HandleHUDHandCardReleased);
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

	const TArray<FName> ValidZoneIds = GetValidPlacementZoneIdsForSelectedHandCard();
	TCGScreenDebug(this, FString::Printf(
		TEXT("TCG UI: Selected hand index %d valid zones %d local player %d card valid %s"),
		HandIndex,
		ValidZoneIds.Num(),
		ResolveLocalPlayerIndex(),
		SelectedHandCardInstanceId.IsValid() ? TEXT("true") : TEXT("false")),
		ValidZoneIds.Num() > 0 ? FColor::Green : FColor::Red);

	for (const FName ZoneId : ValidZoneIds)
	{
		TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Valid zone %s"), *ZoneId.ToString()), FColor::Yellow);
	}

	RefreshPlacementHighlights();
}

void ATCG_PlayerController::HandleHUDHandCardPressed(const int32 HandIndex, UObject* SourceObject)
{
	// The hand widget already selects on press before broadcasting this event.
	// Do not select again here, or the log/detail/highlight path fires multiple times.
	bIsDraggingHandCard = SelectedHandCardInstanceId.IsValid();
	bWasLeftMouseDownDuringHandDrag = false;

	if (bIsDraggingHandCard)
	{
		TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Drag started from hand index %d"), HandIndex), FColor::Cyan);
	}
}

void ATCG_PlayerController::HandleHUDHandCardReleased(const int32 HandIndex, UObject* SourceObject)
{
	// UMG Button release can fire when capture changes while dragging away from the widget.
	// The player controller ends drag from Slate's physical mouse-button state instead.
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
		TCGScreenDebug(this, TEXT("TCG UI: Play ignored, no selected hand card"), FColor::Red);
		return;
	}

	if (!CanSelectedHandCardPlayToZone(ZoneId))
	{
		TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Play ignored, invalid zone %s"), *ZoneId.ToString()), FColor::Red);
		return;
	}

	TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Request play to %s"), *ZoneId.ToString()), FColor::Green);
	ServerTryPlaySelectedHandCardToZone(SelectedHandCardInstanceId, ZoneId);
}

void ATCG_PlayerController::HandleCardZoneActorClicked(const FName ZoneId)
{
	// Placement is now drag-release only. Do not allow the old
	// click-hand-card, click-zone flow to submit a play request.
	if (bSuppressNextZoneActorClick)
	{
		bSuppressNextZoneActorClick = false;
	}
}

void ATCG_PlayerController::EndHandCardDrag()
{
	if (!bIsDraggingHandCard)
	{
		return;
	}

	bIsDraggingHandCard = false;
	bWasLeftMouseDownDuringHandDrag = false;
	FlushPersistentDebugLines(GetWorld());

	if (!SelectedHandCardInstanceId.IsValid())
	{
		TCGScreenDebug(this, TEXT("TCG UI: Drag cancelled, card returned to hand"), FColor::Yellow);
		return;
	}

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		TCGScreenDebug(this, TEXT("TCG UI: Drag cancelled, card returned to hand"), FColor::Yellow);
		return;
	}

	FName ZoneId = NAME_None;
	if (const ATCG_CardZoneActor* ZoneActor = Cast<ATCG_CardZoneActor>(HitResult.GetActor()))
	{
		ZoneId = ZoneActor->GetGameplayZoneName();
	}
	else
	{
		TryResolveZoneIdFromActor(HitResult.GetActor(), ZoneId);
	}

	if (ZoneId.IsNone())
	{
		TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Drag cancelled over %s, card returned to hand"), *GetNameSafe(HitResult.GetActor())), FColor::Yellow);
		return;
	}

	if (!CanSelectedHandCardPlayToZone(ZoneId))
	{
		TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Drag cancelled over invalid zone %s, card returned to hand"), *ZoneId.ToString()), FColor::Yellow);
		return;
	}

	TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Drag release play to %s"), *ZoneId.ToString()), FColor::Green);
	TryPlaySelectedHandCardToZone(ZoneId);
	bSuppressNextZoneActorClick = true;
}

bool ATCG_PlayerController::GetCursorBoardPreviewLocation(FVector& OutPreviewLocation, FName& OutHoveredZoneId) const
{
	OutPreviewLocation = FVector::ZeroVector;
	OutHoveredZoneId = NAME_None;

	if (!GetWorld())
	{
		return false;
	}

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		OutPreviewLocation = HitResult.ImpactPoint + FVector(0.0f, 0.0f, DragPreviewHeightOffset);

		if (const ATCG_CardZoneActor* ZoneActor = Cast<ATCG_CardZoneActor>(HitResult.GetActor()))
		{
			OutHoveredZoneId = ZoneActor->GetGameplayZoneName();
		}
		else
		{
			TryResolveZoneIdFromActor(HitResult.GetActor(), OutHoveredZoneId);
		}

		return true;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float DistanceToBoardPlane = -WorldOrigin.Z / WorldDirection.Z;
	if (DistanceToBoardPlane < 0.0f)
	{
		return false;
	}

	OutPreviewLocation = WorldOrigin + (WorldDirection * DistanceToBoardPlane) + FVector(0.0f, 0.0f, DragPreviewHeightOffset);
	return true;
}

void ATCG_PlayerController::DrawHandCardDragPreview()
{
	if (!bDrawDragPreview || !bIsDraggingHandCard || !SelectedHandCardInstanceId.IsValid() || !GetWorld())
	{
		return;
	}

	FVector PreviewLocation = FVector::ZeroVector;
	FName HoveredZoneId = NAME_None;
	if (!GetCursorBoardPreviewLocation(PreviewLocation, HoveredZoneId))
	{
		return;
	}

	const bool bCanDropHere = !HoveredZoneId.IsNone() && CanSelectedHandCardPlayToZone(HoveredZoneId);
	const FColor PreviewColor = bCanDropHere ? FColor::Green : FColor::Cyan;

	DrawDebugBox(
		GetWorld(),
		PreviewLocation,
		DragPreviewExtent,
		FQuat::Identity,
		PreviewColor,
		false,
		0.0f,
		0,
		DragPreviewLineThickness);
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
		TCGScreenDebug(this, TEXT("TCG UI: Zone click found no hit under cursor"), FColor::Red);
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	FName ZoneId = NAME_None;
	if (!TryResolveZoneIdFromActor(HitActor, ZoneId))
	{
		TCGScreenDebug(this, FString::Printf(
			TEXT("TCG UI: Hit actor %s but no ZoneId matched"),
			*GetNameSafe(HitActor)), FColor::Red);
		return;
	}

	TCGScreenDebug(this, FString::Printf(TEXT("TCG UI: Hit zone %s"), *ZoneId.ToString()), FColor::Green);
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

	if (Actor->ActorHasTag(ZoneId) || DoesTextMatchTCGZone(Actor->GetName(), ZoneId))
	{
		return true;
	}

	for (const FName ActorTag : Actor->Tags)
	{
		if (DoesTextMatchTCGZone(ActorTag.ToString(), ZoneId))
		{
			return true;
		}
	}

	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (const UActorComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->ComponentHasTag(ZoneId) || DoesTextMatchTCGZone(Component->GetName(), ZoneId))
		{
			return true;
		}

		for (const FName ComponentTag : Component->ComponentTags)
		{
			if (DoesTextMatchTCGZone(ComponentTag.ToString(), ZoneId))
			{
				return true;
			}
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
		TCGScreenDebug(this, TEXT("TCG UI: No valid zones to highlight"), FColor::Red);
		return;
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(this, AActor::StaticClass(), AllActors);

	int32 MatchedActorCount = 0;
	int32 HighlightedZoneCount = 0;
	for (const FName ZoneId : ValidZoneIds)
	{
		AActor* BestActor = nullptr;
		FVector BestOrigin = FVector::ZeroVector;
		FVector BestExtent = FVector::ZeroVector;
		float BestScore = -1.0f;
		int32 ZoneMatchCount = 0;

		for (AActor* Actor : AllActors)
		{
			if (!DoesActorMatchZoneId(Actor, ZoneId))
			{
				continue;
			}

			MatchedActorCount++;
			ZoneMatchCount++;

			FVector Origin = Actor->GetActorLocation();
			FVector Extent(80.0f, 80.0f, 10.0f);
			Actor->GetActorBounds(true, Origin, Extent);

			const float Score = Extent.X * Extent.Y;
			TCGScreenDebug(this, FString::Printf(
				TEXT("TCG UI: Match %s actor %s extent %.1f %.1f %.1f score %.1f"),
				*ZoneId.ToString(),
				*GetNameSafe(Actor),
				Extent.X,
				Extent.Y,
				Extent.Z,
				Score), FColor::Cyan);

			if (Score > BestScore)
			{
				BestActor = Actor;
				BestOrigin = Origin;
				BestExtent = Extent;
				BestScore = Score;
			}
		}

		if (BestActor)
		{
			HighlightedZoneCount++;
			BestExtent.X = FMath::Max(BestExtent.X, 60.0f);
			BestExtent.Y = FMath::Max(BestExtent.Y, 60.0f);
			BestExtent.Z = FMath::Max(BestExtent.Z, 8.0f);

			DrawDebugBox(
				GetWorld(),
				BestOrigin,
				BestExtent,
				FColor::Yellow,
				true,
				-1.0f,
				0,
				PlacementHighlightLineThickness);

			TCGScreenDebug(this, FString::Printf(
				TEXT("TCG UI: Highlight %s using %s matches %d"),
				*ZoneId.ToString(),
				*GetNameSafe(BestActor),
				ZoneMatchCount), FColor::Green);
		}
	}

	TCGScreenDebug(this, FString::Printf(
		TEXT("TCG UI: Highlight valid zones %d matched actors %d highlighted zones %d"),
		ValidZoneIds.Num(),
		MatchedActorCount,
		HighlightedZoneCount),
		HighlightedZoneCount > 0 ? FColor::Green : FColor::Red);

	if (MatchedActorCount == 0)
	{
		int32 CandidateCount = 0;
		for (const AActor* Actor : AllActors)
		{
			if (!Actor || CandidateCount >= 12)
			{
				continue;
			}

			const FString LowerName = Actor->GetName().ToLower();
			if (LowerName.Contains(TEXT("zone"))
				|| LowerName.Contains(TEXT("field"))
				|| LowerName.Contains(TEXT("slot"))
				|| LowerName.Contains(TEXT("player")))
			{
				CandidateCount++;
				TCGScreenDebug(this, FString::Printf(
					TEXT("TCG UI: Candidate zone actor %s"),
					*GetNameSafe(Actor)), FColor::Cyan);
			}
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

	const FTCGCardInstance* Card = TCGGameState->FindCardInstance(CardInstanceId);
	if (!Card)
	{
		ClientRefreshMatchHUD();
		return;
	}

	// Trust the replicated card owner, not any local-player fallback on the server.
	// This prevents listen-server / split-client context from resolving the wrong player.
	const int32 PlayerIndex = Card->OwnerPlayerIndex;
	if (!TCGGameState->PlayerPlayCardToZone(PlayerIndex, CardInstanceId, ZoneId))
	{
		ClientRefreshMatchHUD();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TCG UI: Placement summary after play: %s"), *BuildPlacementSummaryLog(*TCGGameState));

	SelectedHandCardInstanceId.Invalidate();
	ClearPlacementHighlights();
	RefreshAllLocalMatchHUDs();
}

FString ATCG_PlayerController::BuildPlacementSummaryLog(const ATCG_GameState& TCGGameState) const
{
	int32 HandCounts[2] = { 0, 0 };
	int32 DeckCounts[2] = { 0, 0 };
	int32 BoardCounts[2] = { 0, 0 };

	for (const FTCGCardInstance& Card : TCGGameState.MatchCards)
	{
		if (!TCGGameState.IsValidPlayerIndex(Card.OwnerPlayerIndex))
		{
			continue;
		}

		switch (Card.Location)
		{
		case ETCGCardLocation::Hand:
			HandCounts[Card.OwnerPlayerIndex]++;
			break;
		case ETCGCardLocation::Deck:
			DeckCounts[Card.OwnerPlayerIndex]++;
			break;
		case ETCGCardLocation::Board:
			BoardCounts[Card.OwnerPlayerIndex]++;
			break;
		default:
			break;
		}
	}

	return FString::Printf(
		TEXT("P0 Hand=%d Deck=%d Board=%d | P1 Hand=%d Deck=%d Board=%d"),
		HandCounts[0],
		DeckCounts[0],
		BoardCounts[0],
		HandCounts[1],
		DeckCounts[1],
		BoardCounts[1]);
}

void ATCG_PlayerController::ClientRefreshMatchHUD_Implementation()
{
	SelectedHandCardInstanceId.Invalidate();
	ClearPlacementHighlights();
	RefreshMatchHUDFromGameState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			RefreshMatchHUDFromGameState();
		}));

		FTimerHandle DelayedHUDRefreshHandle;
		World->GetTimerManager().SetTimer(DelayedHUDRefreshHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			RefreshMatchHUDFromGameState();
		}), 0.10f, false);
	}
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