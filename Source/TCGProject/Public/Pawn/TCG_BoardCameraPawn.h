#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TCG_BoardCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;

UCLASS()
class TCGPROJECT_API ATCG_BoardCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ATCG_BoardCameraPawn();

	UFUNCTION(BlueprintCallable, Category="TCG|Camera")
	void SetBoardCameraTransform(const FVector& NewLocation, const FRotator& NewRotation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TCG|Camera")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TCG|Camera")
	TObjectPtr<UCameraComponent> CameraComponent = nullptr;
};