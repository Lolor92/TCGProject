#pragma once

#include "CoreMinimal.h"
#include "Cards/TCG_CardDefinition.h"
#include "Cards/TCG_CardTypes.h"
#include "GameFramework/GameState.h"
#include "GameState/TCG_EffectChainTypes.h"
#include "GameState/TCG_MatchTypes.h"
#include "GameState/TCG_PendingChoiceTypes.h"

#include "TCG_GameState.generated.h"

UCLASS()
class TCGPROJECT_API ATCG_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ATCG_GameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 TurnNumber = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 RoundNumber = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 PlacementStepIndex = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	TArray<int32> Player0PlacementFieldZonesUsedThisRound;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	TArray<int32> Player1PlacementFieldZonesUsedThisRound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Rules", meta = (ClampMin = "1", UIMin = "1"))
	int32 PlacementStepsPerPlayer = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Rules", meta = (ClampMin = "0", UIMin = "0"))
	int32 InitialHandSize = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Rules", meta = (ClampMin = "0", UIMin = "0"))
	int32 CardsDrawnPerPlacementStep = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Rules", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxRoundNumber = 10;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 CurrentTurnPlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	ETCGMatchPhase CurrentPhase = ETCGMatchPhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	ETCGMatchResult MatchResult = ETCGMatchResult::None;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Cards")
	TArray<FTCGCardInstance> MatchCards;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	TArray<FTCGTemporaryAttackBoost> TemporaryAttackBoosts;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Choice")
	FTCGPendingDiscardChoice PendingDiscardChoice;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Choice")
	FTCGPendingGraveyardToDeckChoice PendingGraveyardToDeckChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingHandToTopDeckChoice PendingHandToTopDeckChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingPlayToEmptyZoneChoice PendingPlayToEmptyZoneChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingAttachSourceToWaterUnitChoice PendingAttachSourceToWaterUnitChoice;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Choice")
	FTCGPendingRevealDeckChoice PendingRevealDeckChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingPlayGraveyardCardToEmptyZoneChoice PendingPlayGraveyardCardToEmptyZoneChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingGraveyardCardsToHandAndTopDeckChoice PendingGraveyardCardsToHandAndTopDeckChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingRemoveMaterialChoice PendingRemoveMaterialChoice;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FTCGPendingSwapOpponentUnitZonesChoice PendingSwapOpponentUnitZonesChoice;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Debug")
	TArray<TObjectPtr<UTCG_CardDefinition>> DebugCardDefinitions;

	static constexpr int32 FieldZoneCount = 4;

	static FName GetFieldZoneId(int32 PlayerIndex, int32 FieldIndex);

public:
	// Match lifecycle
	void StartMatch();
	void SetPhase(ETCGMatchPhase NewPhase);
	void SetMatchResult(ETCGMatchResult NewMatchResult);
	void EndMatch(ETCGMatchResult FinalResult);
	bool IsMatchOver() const;
	void SetCurrentTurnPlayer(int32 NewPlayerIndex);

	// Placement
	int32 GetPlacementStepPlayer() const;
	int32 GetMaxPlacementStepCount() const;
	bool IsPlacementPhaseComplete() const;
	bool CanPlayerActInPlacementStep(int32 PlayerIndex) const;
	int32 GetCompletedPlacementStepsForPlayer(int32 PlayerIndex) const;
	int32 GetNextRequiredFieldZoneIndex(int32 PlayerIndex) const;
	bool CanPlayerPlaceInFieldZone(int32 PlayerIndex, int32 FieldIndex) const;
	bool AdvancePlacementStep();

	// Card instances and lookup
	FTCGCardInstance& AddCardInstance(
		FName CardDefinitionId,
		ETCGCardElement Element,
		int32 BaseAttack,
		int32 OwnerPlayerIndex,
		ETCGCardLocation StartingLocation);

	FTCGCardInstance* AddCardInstanceFromDefinition(
		const UTCG_CardDefinition* CardDefinition,
		int32 OwnerPlayerIndex,
		ETCGCardLocation StartingLocation);

	FTCGCardInstance& AddDebugCardInstance(
		FName CardDefinitionId,
		ETCGCardElement FallbackElement,
		int32 FallbackBaseAttack,
		int32 OwnerPlayerIndex,
		ETCGCardLocation StartingLocation);

	FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId);
	const FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId) const;
	FTCGCardInstance* FindTopCardInStack(const FGuid& StackId);
	const FTCGCardInstance* FindTopCardInStack(const FGuid& StackId) const;
	const FTCGCardInstance* FindTopCardInZone(FName ZoneId) const;

	// Placement validation and play
	bool CanPlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId) const;
	bool PlaceCardAsNewStack(const FGuid& CardInstanceId, FName ZoneId);
	bool PlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId);
	bool PlayCardToZone(const FGuid& CardInstanceId, FName ZoneId);
	bool IsValidPlayerIndex(int32 PlayerIndex) const;
	bool IsCurrentTurnPlayer(int32 PlayerIndex) const;
	bool CanPlayerActInCurrentPhase(int32 PlayerIndex) const;
	bool IsFieldZoneForPlayer(FName ZoneId, int32 PlayerIndex) const;
	bool CanPlayerPlayCardToZone(int32 PlayerIndex, const FGuid& CardInstanceId, FName ZoneId) const;
	bool PlayerPlayCardToZone(int32 PlayerIndex, const FGuid& CardInstanceId, FName ZoneId);

	// Card definitions and effects
	bool ExecuteCardTrigger(const FGuid& CardInstanceId, ETCGEffectTrigger Trigger);
	bool DoesCardEffectMatchTrigger(const FTCGCardEffectRef& EffectRef, ETCGEffectTrigger Trigger) const;
	const UTCG_CardDefinition* FindDebugCardDefinitionById(FName CardDefinitionId) const;
	bool HasDebugCardDefinition(FName CardDefinitionId) const;
	bool ValidateDebugCardDefinitions() const;
	int32 GetPrintedEffectRefsForCard(const FTCGCardInstance& Card, TArray<FTCGCardEffectRef>& OutEffectRefs) const;
	int32 GetPrintedEffectsForCardTrigger(
		const FTCGCardInstance& Card,
		ETCGEffectTrigger Trigger,
		TArray<FName>& OutEffectIds) const;

	// Effect chains
	bool AddCardTriggerToChain(
		TArray<FTCGEffectChainEntry>& Chain,
		const FGuid& SourceCardInstanceId,
		const FGuid& TargetCardInstanceId,
		ETCGEffectTrigger Trigger,
		FName EffectId);

	bool ResolveDebugEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);

	bool AddCardEffectRefToChain(
		TArray<FTCGEffectChainEntry>& Chain,
		const FGuid& SourceCardInstanceId,
		const FGuid& TargetCardInstanceId,
		const FTCGCardEffectRef& EffectRef);

	void ApplyDebugEffectChainEntryRequirements(FTCGEffectChainEntry& ChainEntry) const;
	bool CanResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry) const;
	int32 BuildStackOnPlayEffectChain(const FGuid& TopCardInstanceId, TArray<FTCGEffectChainEntry>& OutChain);
	int32 BuildYourUnitDestroyedGraveyardResponseChain(
		int32 DestroyedUnitOwnerPlayerIndex,
		const FGuid& DestroyedTopCardInstanceId,
		TArray<FTCGEffectChainEntry>& OutChain);
	bool ResolveEffectChain(const TArray<FTCGEffectChainEntry>& Chain);
	bool ResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);
	bool ResolveModularEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);
	bool ResolveEffectStep(
		const FTCGEffectChainEntry& ChainEntry,
		const FTCGEffectStep& Step,
		bool bPreviousStepSucceeded);

	// Pending choice: discard
	bool BeginPendingDiscardChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingDiscardChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingDiscardChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingDiscardChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingDiscardChoice();

	// Pending choice: graveyard to deck
	bool BeginPendingGraveyardToDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingGraveyardToDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingGraveyardToDeckChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingGraveyardToDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingGraveyardToDeckChoice();

	// Pending choice: hand to top deck
	bool BeginPendingHandToTopDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingHandToTopDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingHandToTopDeckChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingHandToTopDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingHandToTopDeckChoice();

	// Pending choice: play to empty zone
	bool BeginPendingPlayToEmptyZoneChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingPlayToEmptyZoneChoice(int32 PlayerIndex, FName ChosenZoneId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingPlayToEmptyZoneChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingPlayToEmptyZoneChoiceOptions(TArray<FName>& OutZoneIds) const;

	void ClearPendingPlayToEmptyZoneChoice();

	// Pending choice: attach source to unit
	bool BeginPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FGuid& ChosenTargetCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingAttachSourceToWaterUnitChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingAttachSourceToWaterUnitChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingAttachSourceToWaterUnitChoice();

	// Pending choice: reveal deck
	bool BeginPendingRevealDeckChoice(
		int32 PlayerIndex,
		int32 Count,
		const FTCGEffectTargetFilter& TargetFilter,
		const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingRevealDeckChoice(int32 PlayerIndex, const FGuid& ChosenCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingRevealDeckChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingRevealDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingRevealDeckChoiceRevealedCards(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingRevealDeckChoice();

	// Pending choice: play graveyard card to empty zone
	bool BeginPendingPlayGraveyardCardToEmptyZoneChoice(
		int32 PlayerIndex,
		const FTCGEffectTargetFilter& TargetFilter,
		const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingPlayGraveyardCardToEmptyZoneChoice(
		int32 PlayerIndex,
		const FGuid& ChosenCardInstanceId,
		FName ChosenZoneId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingPlayGraveyardCardToEmptyZoneChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingPlayGraveyardCardToEmptyZoneCardOptions(TArray<FGuid>& OutCardInstanceIds) const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingPlayGraveyardCardToEmptyZoneZoneOptions(TArray<FName>& OutZoneIds) const;

	void ClearPendingPlayGraveyardCardToEmptyZoneChoice();

	// Pending choice: graveyard cards to hand and top deck
	bool BeginPendingGraveyardCardsToHandAndTopDeckChoice(
		int32 PlayerIndex,
		const FTCGEffectTargetFilter& TargetFilter,
		const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingGraveyardCardsToHandAndTopDeckChoice(
		int32 PlayerIndex,
		const FGuid& ChosenToHandCardInstanceId,
		const FGuid& ChosenToDeckCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingGraveyardCardsToHandAndTopDeckChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingGraveyardCardsToHandAndTopDeckOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingGraveyardCardsToHandAndTopDeckChoice();

	// Pending choice: remove material
	bool BeginPendingRemoveMaterialChoice(
		int32 PlayerIndex,
		const FGuid& TargetCardInstanceId,
		const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingRemoveMaterialChoice(int32 PlayerIndex, const FGuid& ChosenMaterialCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingRemoveMaterialChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingRemoveMaterialChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingRemoveMaterialChoice();

	// Pending choice: swap opponent unit zones
	bool BeginPendingSwapOpponentUnitZonesChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry);

	UFUNCTION(BlueprintCallable, Category = "TCG|Choice")
	bool SubmitPendingSwapOpponentUnitZonesChoice(
		int32 PlayerIndex,
		const FGuid& FirstTargetCardInstanceId,
		const FGuid& SecondTargetCardInstanceId);

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	bool HasPendingSwapOpponentUnitZonesChoice() const;

	UFUNCTION(BlueprintPure, Category = "TCG|Choice")
	void GetPendingSwapOpponentUnitZonesChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const;

	void ClearPendingSwapOpponentUnitZonesChoice();

	// Card movement and effect helpers
	bool MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation);
	bool MoveCardToBottomOfDeck(const FGuid& CardInstanceId);
	bool MoveCardToTopOfDeck(const FGuid& CardInstanceId);
	bool MoveStackToLocation(const FGuid& StackId, ETCGCardLocation NewLocation);
	bool MoveBottomOverlayToGraveyard(const FGuid& TargetCardInstanceId);
	bool PlaySourceCardToFirstEmptyZone(const FGuid& SourceCardInstanceId);
	bool PlaySourceCardToEmptyZone(const FGuid& SourceCardInstanceId, FName ZoneId);
	bool PlayGraveyardCardToEmptyZone(const FGuid& CardInstanceId, FName ZoneId);
	bool MoveGraveyardCardsToHandAndTopDeck(const FGuid& ToHandCardInstanceId, const FGuid& ToDeckCardInstanceId);
	bool RemoveMaterialFromUnit(const FGuid& TargetCardInstanceId, const FGuid& MaterialCardInstanceId);
	bool ReturnUnitStackToBottomDeckMaterialsToGraveyard(const FGuid& TargetTopCardInstanceId);
	bool ResolveAttackMillTwoWaterBounceBattlingUnit(
		const FGuid& SourceCardInstanceId,
		const FGuid& BattleTargetCardInstanceId);
	bool AttachSourceCardUnderWaterUnit(const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId);
	bool AttachGraveyardCardToSourceMaterial(
		int32 PlayerIndex,
		const FGuid& SourceCardInstanceId,
		const FTCGEffectTargetFilter& TargetFilter);
	int32 SendTopDeckCardsToGraveyard(int32 PlayerIndex, int32 Count);
	bool SendTopDeckCardToGraveyard(int32 PlayerIndex);
	bool RevealTopDeckCardsAddElementToHand(
		int32 PlayerIndex,
		int32 Count,
		const FTCGEffectTargetFilter& TargetFilter);
	bool MoveFirstGraveyardCardToBottomDeck(int32 PlayerIndex);
	bool MoveFirstHandCardToTopDeck(int32 PlayerIndex);
	bool GrantTemporaryAttackToUnits(int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, int32 Amount);
	void ClearExpiredTemporaryAttackBoosts();
	bool HandleCardSentToGraveyard(const FGuid& SentCardInstanceId);
	bool DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const;
	int32 GetCardsUnderneathCount(const FGuid& CardInstanceId) const;
	int32 GetFinalAttack(const FGuid& CardInstanceId) const;
	int32 CountCardsInLocationByElement(int32 PlayerIndex, ETCGCardLocation Location, ETCGCardElement Element) const;
	bool FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const;
	bool SwapTwoUnitStackZones(const FGuid& FirstTopCardInstanceId, const FGuid& SecondTopCardInstanceId);
	void GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const;
	void GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const;

	// Battle and card containers
	bool ResolveBattleBetweenZones(FName Player0ZoneId, FName Player1ZoneId);
	bool ResolveBattlePhase();
	ETCGMatchResult CheckLoseConditionAfterBattle() const;
	void GetCardsInLocation(int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards) const;
	void GetCardsInHand(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const;
	void GetCardsInDeck(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const;
	int32 GetNextLocationIndex(int32 PlayerIndex, ETCGCardLocation Location) const;
	bool DrawCard(int32 PlayerIndex);
	int32 DrawCards(int32 PlayerIndex, int32 Count);
	int32 DiscardCardsFromHand(int32 PlayerIndex, int32 Count);
	bool PlayFirstCardFromHandToZone(int32 PlayerIndex, FName ZoneId);

	// Debug scenarios
	void CreateDebugTestCards();
	void SetupDebugMatch();
	void RunDebugTurnFlow();
	void RunDebugTidesCallingScenario();
	void RunDebugDreampoolMirechantScenario();
	void RunDebugDraw2Return1Scenario();
	void RunDebugAttackMillAttachScenario();
	void RunDebugWaterGraveyardBoostScenario();

	UFUNCTION(BlueprintCallable, Category = "TCG|Debug")
	void RunDebugGraveyardPlayChoiceScenario();

	UFUNCTION(BlueprintCallable, Category = "TCG|Debug")
	void RunDebugDestroyedRecoveryScenario();

	UFUNCTION(BlueprintCallable, Category = "TCG|Debug")
	void RunDebugMaterialRebirthScenario();

	UFUNCTION(BlueprintCallable, Category = "TCG|Debug")
	void RunDebugGraveyardRecycleBothDrawScenario();

	UFUNCTION(BlueprintCallable, Category = "TCG|Debug")
	void RunDebugSwapOpponentUnitZonesScenario();
};