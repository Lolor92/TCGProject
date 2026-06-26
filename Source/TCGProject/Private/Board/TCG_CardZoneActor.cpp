#include "Board/TCG_CardZoneActor.h"

#include "Board/TCG_BoardLayoutActor.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameState/TCG_GameState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/TCG_PlayerController.h"

namespace
{
FString ZoneTypeToString(const ETCGCardZoneType ZoneType)
{
	switch (ZoneType)
	{
	case ETCGCardZoneType::Deck:
		return TEXT("Deck");
	case ETCGCardZoneType::Hand:
		return TEXT("Hand");
	case ETCGCardZoneType::Graveyard:
		return TEXT("Graveyard");
	case ETCGCardZoneType::Banish:
		return TEXT("Banish");
	case ETCGCardZoneType::Field:
		return TEXT("Zone");
	case ETCGCardZoneType::Custom:
		return TEXT("Custom");
	default:
		return TEXT("Zone");
	}
}
}

ATCG_CardZoneActor::ATCG_CardZoneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DebugBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("DebugBounds"));
	DebugBounds->SetupAttachment(SceneRoot);
	DebugBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DebugBounds->SetCollisionObjectType(ECC_WorldDynamic);
	DebugBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	DebugBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	DebugBounds->SetGenerateOverlapEvents(false);
	DebugBounds->SetHiddenInGame(!bShowDebugShapeInGame);
	DebugBounds->bDrawOnlyIfSelected = false;

	ZoneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneLabel"));
	ZoneLabel->SetupAttachment(SceneRoot);
	ZoneLabel->SetHorizontalAlignment(EHTA_Center);
	ZoneLabel->SetVerticalAlignment(EVRTA_TextCenter);
	ZoneLabel->SetWorldSize(24.0f);
	ZoneLabel->SetHiddenInGame(!bShowDebugShapeInGame || !bShowLabel);
	ZoneLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATCG_CardZoneActor::BeginPlay()
{
	Super::BeginPlay();

	OnClicked.AddUniqueDynamic(this, &ATCG_CardZoneActor::HandleZoneClicked);
	RefreshPlacementHighlight();
}

void ATCG_CardZoneActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PlacementHighlightRefreshAccumulator += DeltaSeconds;
	if (PlacementHighlightRefreshAccumulator < PlacementHighlightRefreshInterval)
	{
		return;
	}

	PlacementHighlightRefreshAccumulator = 0.0f;
	RefreshPlacementHighlight();
}

void ATCG_CardZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlacementAnchorAttachment();
	UpdateVisuals();
}

void ATCG_CardZoneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCG_CardZoneActor, ZoneId);
	DOREPLIFETIME(ATCG_CardZoneActor, CustomLabel);
}

FName ATCG_CardZoneActor::GetGameplayZoneName() const
{
	if (ZoneId.ZoneType == ETCGCardZoneType::Field)
	{
		return ATCG_GameState::GetFieldZoneId(ZoneId.PlayerIndex, ZoneId.ZoneIndex);
	}

	return ZoneId.ZoneName;
}

FText ATCG_CardZoneActor::GetResolvedLabel() const
{
	if (!CustomLabel.IsEmpty())
	{
		return CustomLabel;
	}

	if (!ZoneId.ZoneName.IsNone())
	{
		return FText::FromName(ZoneId.ZoneName);
	}

	const FString ZoneText = ZoneTypeToString(ZoneId.ZoneType);
	if (ZoneId.ZoneType == ETCGCardZoneType::Field || ZoneId.ZoneType == ETCGCardZoneType::Custom)
	{
		return FText::FromString(FString::Printf(TEXT("%s %d"), *ZoneText, ZoneId.ZoneIndex + 1));
	}

	return FText::FromString(ZoneText);
}

void ATCG_CardZoneActor::ConfigureZone(
	const FTCGCardZoneId& NewZoneId,
	const FText& NewCustomLabel,
	const FVector& NewDebugBoxExtent,
	const FRotator& NewDebugVisualRotation,
	const FColor& NewDebugColor,
	const bool bNewShowDebugShapeInGame)
{
	ZoneId = NewZoneId;
	CustomLabel = NewCustomLabel;
	DebugBoxExtent = NewDebugBoxExtent;
	DebugVisualRotation = NewDebugVisualRotation;
	DebugColor = NewDebugColor;
	bShowDebugShapeInGame = bNewShowDebugShapeInGame;

	UpdateVisuals();
	RefreshPlacementHighlight();
}

void ATCG_CardZoneActor::SetPlacementAnchor(ATCG_BoardLayoutActor* NewPlacementAnchor)
{
	PlacementAnchor = NewPlacementAnchor;
	ApplyPlacementAnchorAttachment();
}

void ATCG_CardZoneActor::CapturePlacementFromCurrentTransform()
{
	ApplyPlacementAnchorAttachment();

	const FTransform AnchorTransform = PlacementAnchor
		? PlacementAnchor->GetActorTransform()
		: FTransform::Identity;

	SavedAnchorRelativeTransform = GetActorTransform().GetRelativeTransform(AnchorTransform);
}

void ATCG_CardZoneActor::RestorePlacementToSavedTransform()
{
	ApplyPlacementAnchorAttachment();

	if (PlacementAnchor)
	{
		SetActorRelativeTransform(SavedAnchorRelativeTransform);
		return;
	}

	const FTransform AnchorTransform = PlacementAnchor
		? PlacementAnchor->GetActorTransform()
		: FTransform::Identity;

	SetActorTransform(SavedAnchorRelativeTransform * AnchorTransform);
}

void ATCG_CardZoneActor::ApplyPlacementAnchorAttachment()
{
	if (PlacementAnchor)
	{
		if (GetAttachParentActor() != PlacementAnchor)
		{
			AttachToActor(PlacementAnchor, FAttachmentTransformRules::KeepWorldTransform);
		}

		return;
	}

	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void ATCG_CardZoneActor::UpdateVisuals()
{
	DebugBounds->SetBoxExtent(DebugBoxExtent);
	DebugBounds->SetRelativeRotation(DebugVisualRotation);
	DebugBounds->ShapeColor = bPlacementHighlightActive ? PlacementHighlightColor : DebugColor;
	DebugBounds->SetHiddenInGame(!bShowDebugShapeInGame && !bPlacementHighlightActive);

	ZoneLabel->SetText(GetResolvedLabel());
	ZoneLabel->SetTextRenderColor(bPlacementHighlightActive ? PlacementHighlightColor : DebugColor);
	ZoneLabel->SetRelativeLocation(FVector(0.0, 0.0, DebugBoxExtent.Z + LabelHeightOffset));
	ZoneLabel->SetRelativeRotation(DebugVisualRotation + FRotator(90.0, 0.0, 0.0));
	ZoneLabel->SetRelativeScale3D(FVector(LabelScale));
	ZoneLabel->SetVisibility(bShowLabel);
	ZoneLabel->SetHiddenInGame((!bShowDebugShapeInGame && !bPlacementHighlightActive) || !bShowLabel);
}

void ATCG_CardZoneActor::RefreshPlacementHighlight()
{
	if (ZoneId.ZoneType != ETCGCardZoneType::Field || !GetWorld())
	{
		SetPlacementHighlightActive(false);
		return;
	}

	ATCG_PlayerController* TCGPlayerController = Cast<ATCG_PlayerController>(GetWorld()->GetFirstPlayerController());
	if (!TCGPlayerController)
	{
		SetPlacementHighlightActive(false);
		return;
	}

	SetPlacementHighlightActive(TCGPlayerController->CanSelectedHandCardPlayToZone(GetGameplayZoneName()));
}

void ATCG_CardZoneActor::SetPlacementHighlightActive(const bool bActive)
{
	if (bPlacementHighlightActive == bActive)
	{
		return;
	}

	bPlacementHighlightActive = bActive;
	UpdateVisuals();
}

void ATCG_CardZoneActor::HandleZoneClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	if (ZoneId.ZoneType != ETCGCardZoneType::Field || !GetWorld())
	{
		return;
	}

	ATCG_PlayerController* TCGPlayerController = Cast<ATCG_PlayerController>(GetWorld()->GetFirstPlayerController());
	if (!TCGPlayerController)
	{
		return;
	}

	TCGPlayerController->HandleCardZoneActorClicked(GetGameplayZoneName());
}