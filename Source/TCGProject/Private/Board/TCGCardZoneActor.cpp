#include "Board/TCGCardZoneActor.h"

#include "Board/TCGBoardLayoutActor.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"

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

ATCGCardZoneActor::ATCGCardZoneActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DebugBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("DebugBounds"));
	DebugBounds->SetupAttachment(SceneRoot);
	DebugBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

void ATCGCardZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlacementAnchorAttachment();
	UpdateVisuals();
}

void ATCGCardZoneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCGCardZoneActor, ZoneId);
	DOREPLIFETIME(ATCGCardZoneActor, CustomLabel);
}

FText ATCGCardZoneActor::GetResolvedLabel() const
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

void ATCGCardZoneActor::ConfigureZone(
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
}

void ATCGCardZoneActor::SetPlacementAnchor(ATCGBoardLayoutActor* NewPlacementAnchor)
{
	PlacementAnchor = NewPlacementAnchor;
	ApplyPlacementAnchorAttachment();
}

void ATCGCardZoneActor::CapturePlacementFromCurrentTransform()
{
	ApplyPlacementAnchorAttachment();

	const FTransform AnchorTransform = PlacementAnchor
		? PlacementAnchor->GetActorTransform()
		: FTransform::Identity;

	SavedAnchorRelativeTransform = GetActorTransform().GetRelativeTransform(AnchorTransform);
}

void ATCGCardZoneActor::RestorePlacementToSavedTransform()
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

void ATCGCardZoneActor::ApplyPlacementAnchorAttachment()
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

void ATCGCardZoneActor::UpdateVisuals()
{
	DebugBounds->SetBoxExtent(DebugBoxExtent);
	DebugBounds->SetRelativeRotation(DebugVisualRotation);
	DebugBounds->ShapeColor = DebugColor;
	DebugBounds->SetHiddenInGame(!bShowDebugShapeInGame);

	ZoneLabel->SetText(GetResolvedLabel());
	ZoneLabel->SetTextRenderColor(DebugColor);
	ZoneLabel->SetRelativeLocation(FVector(0.0, 0.0, DebugBoxExtent.Z + LabelHeightOffset));
	ZoneLabel->SetRelativeRotation(DebugVisualRotation + FRotator(90.0, 0.0, 0.0));
	ZoneLabel->SetRelativeScale3D(FVector(LabelScale));
	ZoneLabel->SetVisibility(bShowLabel);
	ZoneLabel->SetHiddenInGame(!bShowDebugShapeInGame || !bShowLabel);
}
