#include "Board/TCG_CardActor.h"

#include "Board/TCG_BoardLayoutActor.h"
#include "Board/TCG_CardZoneActor.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"

ATCG_CardActor::ATCG_CardActor()
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

void ATCG_CardActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyPlacementAnchorAttachment();

	if (AssignedZone)
	{
		CurrentZoneId = AssignedZone->GetZoneId();
	}

	UpdateVisuals();
}

void ATCG_CardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCG_CardActor, CurrentZoneId);
	DOREPLIFETIME(ATCG_CardActor, CardId);
}

void ATCG_CardActor::SetCurrentZone(const FTCGCardZoneId& NewZoneId)
{
	CurrentZoneId = NewZoneId;
	UpdateVisuals();
}

void ATCG_CardActor::SnapToAssignedZone(const bool bMatchRotation)
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

void ATCG_CardActor::SetPlacementAnchor(ATCG_BoardLayoutActor* NewPlacementAnchor)
{
	PlacementAnchor = NewPlacementAnchor;
	ApplyPlacementAnchorAttachment();
}

void ATCG_CardActor::CapturePlacementFromCurrentTransform()
{
	ApplyPlacementAnchorAttachment();

	const FTransform AnchorTransform = PlacementAnchor
		? PlacementAnchor->GetActorTransform()
		: FTransform::Identity;

	SavedAnchorRelativeTransform = GetActorTransform().GetRelativeTransform(AnchorTransform);
}

void ATCG_CardActor::RestorePlacementToSavedTransform()
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

void ATCG_CardActor::ApplyPlacementAnchorAttachment()
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

void ATCG_CardActor::UpdateVisuals()
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
