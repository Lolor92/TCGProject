#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/TCGUIDataTypes.h"
#include "TCG_PlayerController.generated.h"

class ATCG_GameState;
class UTCGMatchHUDWidgetBase;
struct FTCGCardInstance;

enum class ETCGCardElement : uint8;

UCLASS()
class TCGPROJECT_API ATCG_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void CreateMatchHUD();

	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void SetMatchHUDData(const FTCGMatchHUDWidgetData& InHUDData);

	UFUNCTION(BlueprintCallable, Category="TCG|UI")
	void RefreshMatchHUDFromGameState();

	UFUNCTION(BlueprintPure, Category="TCG|UI|Placement")
	bool HasSelectedHandCard() const { return SelectedHandCardInstanceId.IsValid(); }

	UFUNCTION(BlueprintPure, Category="TCG|UI|Placement")
	FGuid GetSelectedHandCardInstanceId() const { return SelectedHandCardInstanceId; }

	UFUNCTION(BlueprintPure, Category="TCG|UI|Placement")
	bool CanSelectedHandCardPlayToZone(FName ZoneId) const;

	UFUNCTION(BlueprintPure, Category="TCG|UI|Placement")
	TArray<FName> GetValidPlacementZoneIdsForSelectedHandCard() const;

	UFUNCTION(BlueprintCallable, Category="TCG|UI|Placement")
	void TryPlaySelectedHandCardToZone(FName ZoneId);

	
	UFUNCTION(BlueprintCallable, Category="TCG|UI|Placement")
	void HandleCardZoneActorClicked(FName ZoneId);
UFUNCTION(Client, Reliable)
	void ClientSetAssignedPlayerIndex(int32 NewPlayerIndex);

	UFUNCTION(BlueprintPure, Category="TCG|UI")
	UTCGMatchHUDWidgetBase* GetMatchHUDWidget() const { return MatchHUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_PlayerState() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI")
	TSubclassOf<UTCGMatchHUDWidgetBase> MatchHUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="TCG|UI")
	TObjectPtr<UTCGMatchHUDWidgetBase> MatchHUDWidget = nullptr;

	// Fallback used only before the replicated player index is available.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI")
	int32 LocalPlayerIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="TCG|UI")
	int32 AssignedLocalPlayerIndex = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|Camera")
	FVector Player0CameraLocation = FVector(0.0f, -1200.0f, 900.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|Camera")
	FRotator Player0CameraRotation = FRotator(-55.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|Camera")
	FVector Player1CameraLocation = FVector(0.0f, 1200.0f, 900.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|Camera")
	FRotator Player1CameraRotation = FRotator(-55.0f, 180.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Debug")
	bool bUseDebugHUDData = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Debug")
	bool bSeedDebugMatchWhenEmpty = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement")
	bool bDrawPlacementDebugHighlights = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement", meta=(ClampMin="1.0", UIMin="1.0"))
	float PlacementHighlightLineThickness = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement")
	bool bDrawDragPreview = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement")
	FVector DragPreviewExtent = FVector(70.0f, 100.0f, 4.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement")
	float DragPreviewHeightOffset = 28.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement")
	float DragPreviewLineThickness = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TCG|UI|Placement", meta=(ClampMin="10.0", UIMin="10.0"))
	float DragDropZoneSearchRadius = 180.0f;

	UPROPERTY(BlueprintReadOnly, Category="TCG|UI|Placement")
	FGuid SelectedHandCardInstanceId;

	
	UPROPERTY(BlueprintReadOnly, Category="TCG|UI|Placement")
	bool bIsDraggingHandCard = false;

	bool bSuppressNextZoneActorClick = false;

	bool bWasLeftMouseDownDuringHandDrag = false;
void PushDebugHUDData();
	void SeedDebugMatchForHUDIfNeeded();
	int32 ResolveLocalPlayerIndex() const;
	void ApplyFixedBoardCamera();
	void RefreshAllLocalMatchHUDs();
	void RefreshPlacementHighlights();
	void ClearPlacementHighlights();
	bool TryResolveZoneIdFromActor(const AActor* Actor, FName& OutZoneId) const;
	bool DoesActorMatchZoneId(const AActor* Actor, FName ZoneId) const;
	void HandleBoardZoneClick();
	
	void EndHandCardDrag();
	void DrawHandCardDragPreview();
	bool GetCursorBoardPreviewLocation(FVector& OutPreviewLocation, FName& OutHoveredZoneId) const;
	bool ResolveDragDropZoneFromCursor(FName& OutZoneId, FString& OutDebugHitName) const;
	FString BuildPlacementSummaryLog(const ATCG_GameState& TCGGameState) const;
FTCGCardWidgetData FindLocalHandCardDataByHandIndex(int32 HandIndex) const;
	FTCGMatchHUDWidgetData BuildHUDDataFromGameState(const ATCG_GameState& TCGGameState, int32 ForPlayerIndex) const;
	FTCGCardWidgetData BuildCardWidgetDataFromCard(const ATCG_GameState& TCGGameState, const FTCGCardInstance& Card, int32 HandIndex) const;
	FName GetCardElementName(ETCGCardElement Element) const;

	UFUNCTION()
	void HandleHUDHandCardSelected(int32 HandIndex, UObject* SourceObject);

	

	UFUNCTION()
	void HandleHUDHandCardPressed(int32 HandIndex, UObject* SourceObject);

	UFUNCTION()
	void HandleHUDHandCardReleased(int32 HandIndex, UObject* SourceObject);
UFUNCTION(Server, Reliable)
	void ServerTryPlaySelectedHandCardToZone(FGuid CardInstanceId, FName ZoneId);

	UFUNCTION(Client, Reliable)
	void ClientRefreshMatchHUD();
};