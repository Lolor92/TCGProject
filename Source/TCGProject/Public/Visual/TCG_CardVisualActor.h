#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/TCGUIDataTypes.h"
#include "TCG_CardVisualActor.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;
class UTCGCardWidgetBase;

UCLASS(Blueprintable)
class TCGPROJECT_API ATCG_CardVisualActor : public AActor
{
GENERATED_BODY()

public:
ATCG_CardVisualActor();

UFUNCTION(BlueprintCallable, Category="TCG|Card Visual")
void SetCardData(const FTCGCardWidgetData& InCardData);

UFUNCTION(BlueprintCallable, Category="TCG|Card Visual")
void SetPreviewVisualState(bool bCanDropHere);

UFUNCTION(BlueprintPure, Category="TCG|Card Visual")
const FTCGCardWidgetData& GetCardData() const { return CardData; }

protected:
virtual void OnConstruction(const FTransform& Transform) override;
virtual void BeginPlay() override;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
TObjectPtr<USceneComponent> SceneRoot;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
TObjectPtr<UStaticMeshComponent> CardBody;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
TObjectPtr<UWidgetComponent> CardFace;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FVector CardBodyScale = FVector(1.4f, 2.0f, 0.04f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FVector CardFaceLocalLocation = FVector(0.0f, 0.0f, 5.0f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FRotator CardFaceLocalRotation = FRotator(0.0f, 0.0f, 0.0f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FVector2D CardFaceDrawSize = FVector2D(180.0f, 260.0f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FLinearColor ValidDropTint = FLinearColor(0.15f, 0.9f, 0.25f, 1.0f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FLinearColor InvalidDropTint = FLinearColor(0.1f, 0.55f, 1.0f, 1.0f);

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TCG|Card Visual")
FLinearColor PlacedTint = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

UPROPERTY(BlueprintReadOnly, Category="TCG|Card Visual")
FTCGCardWidgetData CardData;

private:
void ApplyCardDataToFaceWidget();
void ApplyTint(const FLinearColor& Tint);
};
