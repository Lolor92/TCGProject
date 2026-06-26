#include "Visual/TCG_CardVisualActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/TCGCardWidgetBase.h"

ATCG_CardVisualActor::ATCG_CardVisualActor()
{
PrimaryActorTick.bCanEverTick = false;

SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
SetRootComponent(SceneRoot);

CardBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardBody"));
CardBody->SetupAttachment(SceneRoot);
CardBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
CardBody->SetGenerateOverlapEvents(false);
CardBody->SetRelativeScale3D(CardBodyScale);

static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
if (CubeMesh.Succeeded())
{
CardBody->SetStaticMesh(CubeMesh.Object);
}

CardFace = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardFace"));
CardFace->SetupAttachment(SceneRoot);
CardFace->SetCollisionEnabled(ECollisionEnabled::NoCollision);
CardFace->SetGenerateOverlapEvents(false);
CardFace->SetWidgetSpace(EWidgetSpace::World);
CardFace->SetDrawSize(CardFaceDrawSize);
CardFace->SetTwoSided(true);
CardFace->SetRelativeLocation(CardFaceLocalLocation);
CardFace->SetRelativeRotation(CardFaceLocalRotation);
}

void ATCG_CardVisualActor::OnConstruction(const FTransform& Transform)
{
Super::OnConstruction(Transform);

if (CardBody)
{
CardBody->SetRelativeScale3D(CardBodyScale);
}

if (CardFace)
{
CardFace->SetRelativeLocation(CardFaceLocalLocation);
CardFace->SetRelativeRotation(CardFaceLocalRotation);
CardFace->SetDrawSize(CardFaceDrawSize);
}
}

void ATCG_CardVisualActor::BeginPlay()
{
Super::BeginPlay();
ApplyCardDataToFaceWidget();
ApplyTint(PlacedTint);
}

void ATCG_CardVisualActor::SetCardData(const FTCGCardWidgetData& InCardData)
{
CardData = InCardData;
ApplyCardDataToFaceWidget();
}

void ATCG_CardVisualActor::SetPreviewVisualState(const bool bCanDropHere)
{
ApplyTint(bCanDropHere ? ValidDropTint : InvalidDropTint);
}

void ATCG_CardVisualActor::ApplyCardDataToFaceWidget()
{
if (!CardFace)
{
return;
}

if (UTCGCardWidgetBase* CardWidget = Cast<UTCGCardWidgetBase>(CardFace->GetUserWidgetObject()))
{
CardWidget->SetCardData(CardData);
}
}

void ATCG_CardVisualActor::ApplyTint(const FLinearColor& Tint)
{
if (!CardBody)
{
return;
}

UMaterialInstanceDynamic* DynamicMaterial = CardBody->CreateAndSetMaterialInstanceDynamic(0);
if (!DynamicMaterial)
{
return;
}

DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
}
