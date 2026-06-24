#pragma once

#include "CoreMinimal.h"
#include "Cards/TCG_CardTypes.h"
#include "GameFramework/GameState.h"
#include "Cards/TCG_CardDefinition.h"
#include "TCG_GameState.generated.h"

UENUM(BlueprintType)
enum class ETCGMatchPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
	RoundStart UMETA(DisplayName = "Round Start"),
	Placement UMETA(DisplayName = "Placement"),
	Battle UMETA(DisplayName = "Battle"),
	RoundEnd UMETA(DisplayName = "Round End"),
	GameOver UMETA(DisplayName = "Game Over")
};

UENUM(BlueprintType)
enum class ETCGMatchResult : uint8
{
	None UMETA(DisplayName = "None"),
	Player0Wins UMETA(DisplayName = "Player 0 Wins"),
	Player1Wins UMETA(DisplayName = "Player 1 Wins"),
	Draw UMETA(DisplayName = "Draw")
};

USTRUCT(BlueprintType)
struct FTCGEffectChainEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FGuid TargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FName SourceCardDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;

	// Legacy/debug shortcut. Real effects should use EffectRef.Steps.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FName EffectId = NAME_None;

	// Full effect data captured when this chain entry is built.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FTCGCardEffectRef EffectRef;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	int32 ControllerPlayerIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	bool bRequiresSourceOnBoard = true;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	bool bRequiresTargetOnBoard = true;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	bool bRequiresSourceInTargetStack = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	bool bRequiresSourceUnderTarget = false;
};

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Debug")
	TArray<TObjectPtr<UTCG_CardDefinition>> DebugCardDefinitions;
	
	static constexpr int32 FieldZoneCount = 4;

	static FName GetFieldZoneId(int32 PlayerIndex, int32 FieldIndex);

public:
	void StartMatch();
	void SetPhase(ETCGMatchPhase NewPhase);
	void SetMatchResult(ETCGMatchResult NewMatchResult);
	void EndMatch(ETCGMatchResult FinalResult);
	bool IsMatchOver() const;
	void SetCurrentTurnPlayer(int32 NewPlayerIndex);

	int32 GetPlacementStepPlayer() const;
	int32 GetMaxPlacementStepCount() const;
	bool IsPlacementPhaseComplete() const;
	bool CanPlayerActInPlacementStep(int32 PlayerIndex) const;
	int32 GetCompletedPlacementStepsForPlayer(int32 PlayerIndex) const;
	int32 GetNextRequiredFieldZoneIndex(int32 PlayerIndex) const;
	bool CanPlayerPlaceInFieldZone(int32 PlayerIndex, int32 FieldIndex) const;
	bool AdvancePlacementStep();

public:
	FTCGCardInstance& AddCardInstance(FName CardDefinitionId, ETCGCardElement Element, int32 BaseAttack,
		int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation);
	
	FTCGCardInstance* AddCardInstanceFromDefinition(const UTCG_CardDefinition* CardDefinition, int32 OwnerPlayerIndex,
		ETCGCardLocation StartingLocation);
	
	FTCGCardInstance& AddDebugCardInstance(FName CardDefinitionId, ETCGCardElement FallbackElement,
		int32 FallbackBaseAttack, int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation);

	FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId);
	const FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId) const;
	
	FTCGCardInstance* FindTopCardInStack(const FGuid& StackId);
	const FTCGCardInstance* FindTopCardInStack(const FGuid& StackId) const;

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
	
	bool ExecuteCardTrigger(const FGuid& CardInstanceId, ETCGEffectTrigger Trigger);
	bool DoesCardEffectMatchTrigger(const FTCGCardEffectRef& EffectRef, ETCGEffectTrigger Trigger) const;
	const UTCG_CardDefinition* FindDebugCardDefinitionById(FName CardDefinitionId) const;
	bool HasDebugCardDefinition(FName CardDefinitionId) const;
	bool ValidateDebugCardDefinitions() const;
	int32 GetPrintedEffectRefsForCard(const FTCGCardInstance& Card, TArray<FTCGCardEffectRef>& OutEffectRefs) const;
	int32 GetPrintedEffectsForCardTrigger(const FTCGCardInstance& Card, ETCGEffectTrigger Trigger, TArray<FName>& OutEffectIds) const;
	bool AddCardTriggerToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId,
		const FGuid& TargetCardInstanceId, ETCGEffectTrigger Trigger, FName EffectId);
	bool AddCardEffectRefToChain(TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId,
		const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef);
	void ApplyDebugEffectChainEntryRequirements(FTCGEffectChainEntry& ChainEntry) const;
	bool CanResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry) const;
	int32 BuildStackOnPlayEffectChain(const FGuid& TopCardInstanceId, TArray<FTCGEffectChainEntry>& OutChain);
	bool ResolveEffectChain(const TArray<FTCGEffectChainEntry>& Chain);
	bool ResolveEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);
	bool ResolveModularEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);
	bool ResolveEffectStep(const FTCGEffectChainEntry& ChainEntry, const FTCGEffectStep& Step, bool bPreviousStepSucceeded);
	bool ResolveDebugEffectChainEntry(const FTCGEffectChainEntry& ChainEntry);

	bool MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation);
	bool MoveStackToLocation(const FGuid& StackId, ETCGCardLocation NewLocation);
	bool DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const;
	int32 GetCardsUnderneathCount(const FGuid& CardInstanceId) const;
	int32 GetFinalAttack(const FGuid& CardInstanceId) const;
	bool FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const;
	void GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const;
	void GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const;
	const FTCGCardInstance* FindTopCardInZone(FName ZoneId) const;
	
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
	void CreateDebugTestCards();
	void SetupDebugMatch();
	void RunDebugTurnFlow();
};
