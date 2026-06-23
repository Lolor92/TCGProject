#pragma once

#include "CoreMinimal.h"
#include "Cards/TCG_CardTypes.h"
#include "GameFramework/GameState.h"
#include "TCG_GameState.generated.h"

UENUM(BlueprintType)
enum class ETCGMatchPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
	Draw UMETA(DisplayName = "Draw"),
	Main UMETA(DisplayName = "Main"),
	Battle UMETA(DisplayName = "Battle"),
	End UMETA(DisplayName = "End"),
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

/**
 * GameState exists on the server and all clients.
 *
 * It stores shared match state:
 * - turn
 * - phase
 * - active player
 * - card instances
 */
UCLASS()
class TCGPROJECT_API ATCG_GameState : public AGameState
{
	GENERATED_BODY()

public:
	ATCG_GameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// Starts at 1 when the match begins.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 TurnNumber = 0;

	// Which player is currently taking actions.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	int32 CurrentTurnPlayerIndex = INDEX_NONE;

	// Current phase of the match.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	ETCGMatchPhase CurrentPhase = ETCGMatchPhase::WaitingForPlayers;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Match")
	ETCGMatchResult MatchResult = ETCGMatchResult::None;

	// All card copies currently known by the match.
	// This is gameplay state, not visual actors.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "TCG|Cards")
	TArray<FTCGCardInstance> MatchCards;
	
	static constexpr int32 FieldZoneCount = 3;

	static FName GetFieldZoneId(int32 PlayerIndex, int32 FieldIndex);

public:
	void StartMatch();
	void SetPhase(ETCGMatchPhase NewPhase);
	void SetMatchResult(ETCGMatchResult NewMatchResult);
	void EndMatch(ETCGMatchResult FinalResult);
	bool IsMatchOver() const;
	void SetCurrentTurnPlayer(int32 NewPlayerIndex);

public:
	// Creates a new card instance and adds it to the match.
	// For now this is useful for test cards later.
	FTCGCardInstance& AddCardInstance(FName CardDefinitionId, ETCGCardElement Element, int32 BaseAttack,
		int32 OwnerPlayerIndex, ETCGCardLocation StartingLocation);

	// Finds a card by its unique instance id.
	FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId);

	// Const version for read-only checks.
	const FTCGCardInstance* FindCardInstance(const FGuid& CardInstanceId) const;
	
	// Finds the top card in a stack.
	FTCGCardInstance* FindTopCardInStack(const FGuid& StackId);
	const FTCGCardInstance* FindTopCardInStack(const FGuid& StackId) const;

	// Basic stack rule: same element can stack on same element.
	// Exceptions come later.
	bool CanPlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId) const;

	// Places a card on the board as a brand-new stack.
	bool PlaceCardAsNewStack(const FGuid& CardInstanceId, FName ZoneId);

	// Places a card on top of an existing stack.
	bool PlaceCardOnStack(const FGuid& CardInstanceId, const FGuid& TargetStackId);
	
	// Main rule entry for playing a card from hand to a board zone.
	// If the zone is empty, this creates a new stack.
	// If the zone has a stack, this tries to stack on top.
	bool PlayCardToZone(const FGuid& CardInstanceId, FName ZoneId);

	// Moves a card to a new location.
	// Later this will trigger effects like OnSentToGraveyard or OnBanished.
	bool MoveCardToLocation(const FGuid& CardInstanceId, ETCGCardLocation NewLocation);
	
	bool MoveStackToLocation(const FGuid& StackId, ETCGCardLocation NewLocation);

	// Returns true if the player has at least one card on the board.
	// This prepares your lose condition, but does not enforce it yet.
	bool DoesPlayerHaveAnyCardOnBoard(int32 PlayerIndex) const;

	// Counts how many cards are underneath this card in its stack.
	// This prepares the +1 attack per card underneath rule.
	int32 GetCardsUnderneathCount(const FGuid& CardInstanceId) const;
	
	// Base attack + 1 for each card underneath it.
	int32 GetFinalAttack(const FGuid& CardInstanceId) const;
	
	// Finds the first stack currently placed in a board zone.
	bool FindStackIdInZone(FName ZoneId, FGuid& OutStackId) const;

	// Gets all cards in a stack, sorted from bottom to top.
	void GetCardsInStack(const FGuid& StackId, TArray<FTCGCardInstance>& OutCards) const;

	// Gets all cards currently placed in a board zone.
	void GetCardsInZone(FName ZoneId, TArray<FTCGCardInstance>& OutCards) const;

	// Finds the top card in a board zone.
	const FTCGCardInstance* FindTopCardInZone(FName ZoneId) const;
	
	bool ResolveBattleBetweenZones(FName Player0ZoneId, FName Player1ZoneId);
	
	bool ResolveBattlePhase();
	
	ETCGMatchResult CheckLoseConditionAfterBattle() const;
	
	// Gets all cards owned by a player in a specific location.
	void GetCardsInLocation(int32 PlayerIndex, ETCGCardLocation Location, TArray<FTCGCardInstance>& OutCards) const;

	// Shortcut for cards currently in a player's hand.
	void GetCardsInHand(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const;
	
	void GetCardsInDeck(int32 PlayerIndex, TArray<FTCGCardInstance>& OutCards) const;

	int32 GetNextLocationIndex(int32 PlayerIndex, ETCGCardLocation Location) const;
	
	bool DrawCard(int32 PlayerIndex);
	
	// Draws multiple cards for a player.
	int32 DrawCards(int32 PlayerIndex, int32 Count);
	
	bool PlayFirstCardFromHandToZone(int32 PlayerIndex, FName ZoneId);
	
	// Temporary debug setup so we can test match cards and stack rules.
	void CreateDebugTestCards();
	
	// Temporary setup until real deck data exists.
	void SetupDebugMatch();
	
	void RunDebugTurnFlow();
};
