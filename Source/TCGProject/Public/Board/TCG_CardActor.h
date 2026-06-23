#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Board/TCG_CardPlacementTypes.h"
#include "TCG_CardActor.generated.h"

class ATCG_BoardLayoutActor;
class ATCG_CardZoneActor;
class UBoxComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class TCGPROJECT_API ATCG_CardActor : public AActor
{
	GENERATED_BODY()

public:
	ATCG_CardActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Updates the card's replicated logical position without forcing a world transform change. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Card")
	void SetCurrentZone(const FTCGCardZoneId& NewZoneId);

	/** Returns the replicated zone identity this card currently belongs to. */
	UFUNCTION(BlueprintPure, Category = "TCG|Card")
	const FTCGCardZoneId& GetCurrentZone() const { return CurrentZoneId; }

	/** Moves this placeholder card to its assigned zone actor for quick map testing. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Card")
	void SnapToAssignedZone(bool bMatchRotation = true);

	/** Sets the board actor used as this placeholder card's placement anchor. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Placement")
	void SetPlacementAnchor(ATCG_BoardLayoutActor* NewPlacementAnchor);

	/** Saves this card's current actor transform relative to its placement anchor, or as world space if no anchor is set. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void CapturePlacementFromCurrentTransform();

	/** Moves this placeholder card back to the saved transform, using the placement anchor as the parent reference. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void RestorePlacementToSavedTransform();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> DebugBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> CardLabel;

	/** Replicated logical location, such as Player 0 Hand or Player 1 Graveyard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "TCG|Card")
	FTCGCardZoneId CurrentZoneId;

	/** Temporary card identity for debug labels. Later this can map to a real card data asset or database id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "TCG|Card")
	FName CardId = NAME_None;

	/** Optional design-time zone reference used by SnapToAssignedZone while arranging test cards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Card")
	TObjectPtr<ATCG_CardZoneActor> AssignedZone;

	/** Board actor this placeholder card is positioned relative to. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "TCG|Placement")
	TObjectPtr<ATCG_BoardLayoutActor> PlacementAnchor;

	/** Saved transform relative to PlacementAnchor. If no anchor is set, this is treated as a saved world transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Placement")
	FTransform SavedAnchorRelativeTransform = FTransform::Identity;

	/** Half-size of the placeholder card shape in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape", meta = (ClampMin = "1.0"))
	FVector DebugBoxExtent = FVector(63.0, 88.0, 1.5);

	/** Local rotation for the placeholder card shape, independent from the actor transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	FRotator DebugVisualRotation = FRotator::ZeroRotator;

	/** Editor/debug color for the placeholder card. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	FColor DebugColor = FColor(255, 255, 255);

	/** Enables the simple debug card shape during play. It remains useful for early multiplayer layout testing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	bool bShowDebugShapeInGame = true;

private:
	/** Attaches or detaches this actor so anchor transform changes update the placeholder visually in the editor and in play. */
	void ApplyPlacementAnchorAttachment();

	/** Keeps the placeholder card footprint and label aligned with editable properties. */
	void UpdateVisuals();
};
