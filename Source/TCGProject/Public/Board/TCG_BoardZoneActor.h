#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TCG_BoardZoneActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS()
class TCGPROJECT_API ATCG_BoardZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ATCG_BoardZoneActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="TCG|Board Zone")
	FName GetZoneId() const { return ZoneId; }

	UFUNCTION(BlueprintCallable, Category="TCG|Board Zone")
	void SetZoneId(FName NewZoneId);

	UFUNCTION(BlueprintCallable, Category="TCG|Board Zone")
	void RefreshPlacementHighlight();

	UFUNCTION(BlueprintCallable, Category="TCG|Board Zone")
	void SetPlacementHighlightActive(bool bActive);

	UFUNCTION(BlueprintPure, Category="TCG|Board Zone")
	bool IsPlacementHighlightActive() const { return bPlacementHighlightActive; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	TObjectPtr<UBoxComponent> ClickBounds = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	TObjectPtr<UStaticMeshComponent> HighlightMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	FName ZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	FVector ZoneExtent = FVector(60.0f, 90.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	FVector HighlightScale = FVector(1.2f, 1.8f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Board Zone")
	TObjectPtr<UMaterialInterface> HighlightMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Board Zone", meta=(ClampMin="0.02", UIMin="0.02"))
	float HighlightRefreshInterval = 0.05f;

private:
	UFUNCTION()
	void HandleClicked(AActor* TouchedActor, FKey ButtonPressed);

	float HighlightRefreshAccumulator = 0.0f;
	bool bPlacementHighlightActive = false;
};