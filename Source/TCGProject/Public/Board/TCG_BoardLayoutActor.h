#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Board/TCG_CardPlacementTypes.h"
#include "TCG_BoardLayoutActor.generated.h"

class ATCG_CardZoneActor;

UCLASS(Blueprintable)
class TCGPROJECT_API ATCG_BoardLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	ATCG_BoardLayoutActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Rebuilds the zone list by finding every card zone actor in the level with this board id. */
	UFUNCTION(BlueprintCallable, Category = "TCG|Board")
	void RefreshZoneRegistry();

	/** Creates or updates the two-player board preset based on the rough reference layout. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Preset")
	void CreateOrUpdateTwoPlayerReferencePreset();

	/** Assigns this board as the placement anchor for every registered zone. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void AssignThisAsAnchorForZones();

	/** Saves every registered zone's current transform relative to this board actor. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void CaptureAllZonePlacements();

	/** Restores every registered zone to its saved transform relative to this board actor. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TCG|Placement")
	void RestoreAllZonePlacements();

	/** Returns the cached zones that belong to this board layout. */
	UFUNCTION(BlueprintPure, Category = "TCG|Board")
	TArray<ATCG_CardZoneActor*> GetRegisteredZones() const;

	/** Finds the first registered zone whose board/player/type/index/name matches the requested id. */
	UFUNCTION(BlueprintPure, Category = "TCG|Board")
	ATCG_CardZoneActor* FindZone(const FTCGCardZoneId& ZoneId) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Board id shared by all zone actors that belong to this layout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "TCG|Board")
	FName BoardId = TEXT("MainBoard");

	/** Keeps map setup simple by registering matching zones automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Board")
	bool bAutoRegisterZonesOnBeginPlay = true;

	/** Number of numbered field zones generated for each player by the reference preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1"))
	int32 PresetFieldZoneCount = 4;

	/** Half-size used for every generated zone footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	FVector PresetZoneExtent = FVector(70.0, 100.0, 2.0);

	/** Half-size used for each generated hand area. Hands are dynamic, so the preset creates one area per player instead of fixed card slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	FVector PresetHandAreaExtent = FVector(450.0, 100.0, 2.0);

	/** Horizontal spacing between generated field slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	float PresetSlotSpacing = 250.0f;

	/** Distance from board center to each player's field row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	float PresetFieldRowOffset = 150.0f;

	/** Distance from board center to each player's hand row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	float PresetHandRowOffset = 525.0f;

	/** Distance from board center to the side pile columns for deck, graveyard, and banish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	float PresetSidePileColumnOffset = 725.0f;

	/** Vertical spacing between side piles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset", meta = (ClampMin = "1.0"))
	float PresetSidePileSpacing = 225.0f;

	/** Color assigned to generated Player 1 zones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset")
	FColor PresetPlayerOneColor = FColor(0, 160, 255);

	/** Color assigned to generated Player 2 zones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset")
	FColor PresetPlayerTwoColor = FColor(255, 120, 40);

	/** Enables generated zone debug shapes during play as well as in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Preset")
	bool bPresetShowDebugShapesInGame = true;

	/** Cached list of level-placed zones. This is editor-visible for debugging but rebuilt at runtime. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "TCG|Board")
	TArray<TObjectPtr<ATCG_CardZoneActor>> RegisteredZones;

private:
	void RemoveLegacyPresetHandPreviewSlots();
	void RemoveDuplicateZonesForId(const FTCGCardZoneId& ZoneId, ATCG_CardZoneActor* ZoneToKeep);
	ATCG_CardZoneActor* FindExactRegisteredZone(const FTCGCardZoneId& ZoneId) const;
	ATCG_CardZoneActor* CreateOrUpdatePresetZone(
		const FTCGCardZoneId& ZoneId,
		const FText& Label,
		const FTransform& AnchorRelativeTransform,
		const FVector& DebugExtent,
		const FColor& DebugColor);
};
