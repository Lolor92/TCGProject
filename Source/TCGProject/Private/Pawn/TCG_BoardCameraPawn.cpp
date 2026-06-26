#include "Pawn/TCG_BoardCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ATCG_BoardCameraPawn::ATCG_BoardCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("BoardCamera"));
	CameraComponent->SetupAttachment(SceneRoot);
	CameraComponent->bUsePawnControlRotation = false;
}

void ATCG_BoardCameraPawn::SetBoardCameraTransform(const FVector& NewLocation, const FRotator& NewRotation)
{
	SetActorLocationAndRotation(NewLocation, NewRotation);
}