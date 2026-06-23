#include "Board/TCGCardActor.h"

#include "Board/TCGBoardLayoutActor.h"
#include "Board/TCGCardZoneActor.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"

ATCGCardActor::ATCGCardActor()
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

	CardLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CardLabel"));
	CardLabel->SetupAttachment(SceneRoot);
	CardLabel->SetHorizontalAlignment(EHTA_Center);
	CardLabel->SetVerticalAlignment(EVRTA_TextCenter);
	CardLabel->SetWorldSize(18.0f);
	CardLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardLabel->SetHiddenInGame(!bShowDebugShapeInGame);
}

void ATCGCardActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPlacementAnchorAttachment();

	if (AssignedZone)
	{
		CurrentZoneId = AssignedZone->GetZoneId();
	}

	UpdateVisuals();
}

void ATCGCardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCGCardActor, CurrentZoneId);
	DOREPLIFETIME(ATCGCardActor, CardId);
}

void ATCGCardActor::SetCurrentZone(const FTCGCardZoneId& NewZoneId)
{
	CurrentZoneId = NewZoneId;
	UpdateVisuals();
}

void ATCGCardActor::SnapToAssignedZone(const bool bMatchRotation)
{
	if (!AssignedZone)
	{
		return;
	}

	SetActorLocation(AssignedZone->GetActorLocation());
	if (bMatchRotation)
	{
		SetActorRotation(AssignedZone->GetActorRotation());
	}

	CurrentZoneId = AssignedZone->GetZoneId();
}

void ATCGCardActor::SetPlacementAnchor(ATCGBoardLayoutActor* NewPlacementAnchor)
{
	PlacementAnchor = NewPlacementAnchor;
	ApplyPlacementAnchorAttachment();
}

void ATCGCardActor::CapturePlacementFromCurrentTransform()
{
	ApplyPlacementAnchorAttachment();

	const FTransform AnchorTransform = PlacementAnchor
		? PlacementAnchor->GetActorTransform()
		: FTransform::Identity;

	SavedAnchorRelativeTransform = GetActorTransform().GetRelativeTransform(AnchorTransform);
}

void ATCGCardActor::RestorePlacementToSavedTransform()
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

void ATCGCardActor::ApplyPlacementAnchorAttachment()
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

void ATCGCardActor::UpdateVisuals()
{
	DebugBounds->SetBoxExtent(DebugBoxExtent);
	DebugBounds->SetRelativeRotation(DebugVisualRotation);
	DebugBounds->ShapeColor = DebugColor;
	DebugBounds->SetHiddenInGame(!bShowDebugShapeInGame);

	const FText ResolvedLabel = CardId.IsNone()
		? FText::FromString(TEXT("Card"))
		: FText::FromName(CardId);

	CardLabel->SetText(ResolvedLabel);
	CardLabel->SetTextRenderColor(DebugColor);
	CardLabel->SetRelativeLocation(FVector(0.0, 0.0, DebugBoxExtent.Z + 3.0f));
	CardLabel->SetRelativeRotation(DebugVisualRotation + FRotator(90.0, 0.0, 0.0));
	CardLabel->SetVisibility(true);
	CardLabel->SetHiddenInGame(!bShowDebugShapeInGame);
}
