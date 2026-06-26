#include "Board/TCG_BoardZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATCG_BoardZoneActor::ATCG_BoardZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	SetRootComponent(ClickBounds);
	ClickBounds->SetBoxExtent(ZoneExtent);
	ClickBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	HighlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightMesh"));
	HighlightMesh->SetupAttachment(ClickBounds);
	HighlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HighlightMesh->SetVisibility(false);
	HighlightMesh->SetHiddenInGame(true);
	HighlightMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
	HighlightMesh->SetRelativeScale3D(HighlightScale);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		HighlightMesh->SetStaticMesh(PlaneMeshFinder.Object);
	}
}

void ATCG_BoardZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ClickBounds)
	{
		ClickBounds->SetBoxExtent(ZoneExtent);
	}

	if (HighlightMesh)
	{
		HighlightMesh->SetRelativeScale3D(HighlightScale);
		if (HighlightMaterial)
		{
			HighlightMesh->SetMaterial(0, HighlightMaterial);
		}
		HighlightMesh->SetVisibility(bPlacementHighlightActive);
		HighlightMesh->SetHiddenInGame(!bPlacementHighlightActive);
	}
}

void ATCG_BoardZoneActor::SetZoneId(const FName NewZoneId)
{
	ZoneId = NewZoneId;
}

void ATCG_BoardZoneActor::SetPlacementHighlightActive(const bool bActive)
{
	bPlacementHighlightActive = bActive;

	if (HighlightMesh)
	{
		HighlightMesh->SetVisibility(bPlacementHighlightActive, true);
		HighlightMesh->SetHiddenInGame(!bPlacementHighlightActive, true);
	}
}