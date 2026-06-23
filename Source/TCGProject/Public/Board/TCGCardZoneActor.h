#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Board/TCGCardPlacementTypes.h"
#include "TCGCardZoneActor.generated.h"

class ATCGBoardLayoutActor;
class UBoxComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class TCGPROJECT_API ATCGCardZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ATCGCardZoneActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Returns the replicated identity cards use to know which deck, hand, pile, or field slot they are in. */
	UFUNCTION(BlueprintPure, Category = "TCG|Zone")
	const FTCGCardZoneId& GetZoneId() const { return ZoneId; }

	/** Returns the editor/debug label, preferring CustomLabel, then ZoneName, then a generated label. */
	UFUNCTION(BlueprintPure, Category = "TCG|Zone")
	FText GetResolvedLabel() const;

	/** Applies the editable identity and debug footprint used by board presets and Blueprint setup tools. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Zone")
	void ConfigureZone(
		const FTCGCardZoneId& NewZoneId,
		const FText& NewCustomLabel,
		const FVector& NewDebugBoxExtent,
		const FRotator& NewDebugVisualRotation,
		const FColor& NewDebugColor,
		bool bNewShowDebugShapeInGame);

	/** Sets the board actor used as this zone's placement anchor. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Placement")
	void SetPlacementAnchor(ATCGBoardLayoutActor* NewPlacementAnchor);

	/** Saves this zone's current actor transform relative to its placement anchor, or as world space if no anchor is set. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void CapturePlacementFromCurrentTransform();

	/** Moves this zone back to the saved transform, using the placement anchor as the parent reference. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void RestorePlacementToSavedTransform();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> DebugBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> ZoneLabel;

	/** Editable zone identity. Keep this stable once gameplay starts so cards can replicate zone changes cheaply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "TCG|Zone")
	FTCGCardZoneId ZoneId;

	/** Optional visible name for this zone, such as "Player 1 Deck" or "Zone 3". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "TCG|Zone")
	FText CustomLabel;

	/** Board actor this zone is positioned relative to. Move the board actor to move the whole layout reference. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "TCG|Placement")
	TObjectPtr<ATCGBoardLayoutActor> PlacementAnchor;

	/** Saved transform relative to PlacementAnchor. If no anchor is set, this is treated as a saved world transform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Placement")
	FTransform SavedAnchorRelativeTransform = FTransform::Identity;

	/** Half-size of the debug card footprint in Unreal units. Change this to match the intended card/slot size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape", meta = (ClampMin = "1.0"))
	FVector DebugBoxExtent = FVector(100.0, 70.0, 2.0);

	/** Local rotation for the debug footprint. Use this when the zone actor position is right but the card shape needs to turn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	FRotator DebugVisualRotation = FRotator::ZeroRotator;

	/** Editor/debug color for the footprint and label. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	FColor DebugColor = FColor(0, 160, 255);

	/** Enables the simple debug footprint during play. It remains visible in the editor either way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Shape")
	bool bShowDebugShapeInGame = true;

	/** Toggles the text label above the debug footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Label")
	bool bShowLabel = true;

	/** Scales the debug text without changing the zone shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Label", meta = (ClampMin = "0.01"))
	float LabelScale = 0.45f;

	/** Raises the debug label above the footprint so it does not overlap the card shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Debug Label")
	float LabelHeightOffset = 4.0f;

private:
	/** Attaches or detaches this actor so anchor transform changes update the zone visually in the editor and in play. */
	void ApplyPlacementAnchorAttachment();

	/** Keeps editor-only/debug components in sync with editable properties. */
	void UpdateVisuals();
};
