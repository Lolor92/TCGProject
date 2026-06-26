#include "Board/TCG_BoardZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PlayerController/TCG_PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATCG_BoardZoneActor::ATCG_BoardZoneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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

void ATCG_BoardZoneActor::BeginPlay()
{
	Super::BeginPlay();

	OnClicked.AddUniqueDynamic(this, &ATCG_BoardZoneActor::HandleClicked);
	SetPlacementHighlightActive(false);
}

void ATCG_BoardZoneActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HighlightRefreshAccumulator += DeltaSeconds;
	if (HighlightRefreshAccumulator < HighlightRefreshInterval)
	{
		return;
	}

	HighlightRefreshAccumulator = 0.0f;
	RefreshPlacementHighlight();
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
	RefreshPlacementHighlight();
}

void ATCG_BoardZoneActor::RefreshPlacementHighlight()
{
	if (ZoneId.IsNone() || !GetWorld())
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

	SetPlacementHighlightActive(TCGPlayerController->CanSelectedHandCardPlayToZone(ZoneId));
}

void ATCG_BoardZoneActor::SetPlacementHighlightActive(const bool bActive)
{
	if (bPlacementHighlightActive == bActive)
	{
		return;
	}

	bPlacementHighlightActive = bActive;

	if (HighlightMesh)
	{
		HighlightMesh->SetVisibility(bPlacementHighlightActive, true);
		HighlightMesh->SetHiddenInGame(!bPlacementHighlightActive, true);
	}
}

void ATCG_BoardZoneActor::HandleClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	if (ZoneId.IsNone() || !GetWorld())
	{
		return;
	}

	ATCG_PlayerController* TCGPlayerController = Cast<ATCG_PlayerController>(GetWorld()->GetFirstPlayerController());
	if (!TCGPlayerController)
	{
		return;
	}

	TCGPlayerController->TryPlaySelectedHandCardToZone(ZoneId);
}