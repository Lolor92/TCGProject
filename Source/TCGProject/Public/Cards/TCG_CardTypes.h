#pragma once

#include "CoreMinimal.h"
#include "TCG_CardTypes.generated.h"

/**
 * The element/type of a unit card.
 *
 * This game only has Unit cards.
 * Fire/Wind/Earth/Water/Light/Dark are not card categories like Spell/Unit.
 * They are the card's element.
 */
UENUM(BlueprintType)
enum class ETCGCardElement : uint8
{
	Fire  UMETA(DisplayName = "Fire"),
	Wind  UMETA(DisplayName = "Wind"),
	Earth UMETA(DisplayName = "Earth"),
	Water UMETA(DisplayName = "Water"),
	Light UMETA(DisplayName = "Light"),
	Dark  UMETA(DisplayName = "Dark")
};

/**
 * Where a card currently exists in the match.
 *
 * This is gameplay state, not visual placement.
 */
UENUM(BlueprintType)
enum class ETCGCardLocation : uint8
{
	None      UMETA(DisplayName = "None"),
	Deck      UMETA(DisplayName = "Deck"),
	Hand      UMETA(DisplayName = "Hand"),
	Board     UMETA(DisplayName = "Board"),
	Graveyard UMETA(DisplayName = "Graveyard"),
	Banish    UMETA(DisplayName = "Banish")
};

/**
 * Effect trigger timing.
 */
UENUM(BlueprintType)
enum class ETCGEffectTrigger : uint8
{
	None              UMETA(DisplayName = "None"),
	OnPlay            UMETA(DisplayName = "On Play"),
	OnDestroyed       UMETA(DisplayName = "On Destroyed"),
	OnSentToGraveyard UMETA(DisplayName = "On Sent To Graveyard"),
	OnBanished        UMETA(DisplayName = "On Banished"),
	OnBattleStart     UMETA(DisplayName = "On Battle Start"),
	OnBattleEnd       UMETA(DisplayName = "On Battle End"),
	OnStackAdded      UMETA(DisplayName = "On Stack Added"),
	OnBecomingTopCard UMETA(DisplayName = "On Becoming Top Card")
};

/**
 * Reusable effect step kind.
 *
 * A card effect is built from small pieces: draw, discard, modify attack,
 * selection, card movement, and flow-control steps such as Then.
 */
UENUM(BlueprintType)
enum class ETCGEffectStepType : uint8
{
	None                         UMETA(DisplayName = "None"),
	Then                         UMETA(DisplayName = "Then"),
	DrawCards                    UMETA(DisplayName = "Draw Cards"),
	DiscardCards                 UMETA(DisplayName = "Discard Cards"),
	ModifyAttack                 UMETA(DisplayName = "Modify Attack"),
	SelectTarget                 UMETA(DisplayName = "Select Target"),
	MoveBottomOverlayToGraveyard UMETA(DisplayName = "Move Bottom Overlay To Graveyard"),
	PlaySourceToEmptyZone        UMETA(DisplayName = "Play Source To Empty Zone"),
	SendTopDeckCardToGraveyard   UMETA(DisplayName = "Send Top Deck Card To Graveyard"),
	MoveGraveyardCardToBottomDeck UMETA(DisplayName = "Move Graveyard Card To Bottom Deck")
};

/**
 * What an effect step acts on.
 */
UENUM(BlueprintType)
enum class ETCGEffectTargetMode : uint8
{
	None             UMETA(DisplayName = "None"),
	Controller       UMETA(DisplayName = "Controller"),
	Opponent         UMETA(DisplayName = "Opponent"),
	SourceCard       UMETA(DisplayName = "Source Card"),
	TriggerTarget    UMETA(DisplayName = "Trigger Target"),
	SelectedTarget   UMETA(DisplayName = "Selected Target")
};

/**
 * How an effect step gets its numeric value.
 */
UENUM(BlueprintType)
enum class ETCGEffectValueMode : uint8
{
	Fixed                  UMETA(DisplayName = "Fixed"),
	CardsUnderneathSource  UMETA(DisplayName = "Cards Underneath Source"),
	CardsUnderneathTarget  UMETA(DisplayName = "Cards Underneath Target")
};

/**
 * Whether a step chooses its affected cards automatically or asks the player.
 */
UENUM(BlueprintType)
enum class ETCGEffectSelectionMode : uint8
{
	Automatic    UMETA(DisplayName = "Automatic"),
	PlayerChoice UMETA(DisplayName = "Player Choice")
};

/**
 * Basic target filter for selection steps.
 *
 * This is intentionally small for now. It gives us a data shape that can grow
 * into player choice, enemy units, overlays, graveyard cards, and so on.
 */
USTRUCT(BlueprintType)
struct FTCGEffectTargetFilter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectTargetMode OwnerMode = ETCGEffectTargetMode::Controller;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGCardLocation RequiredLocation = ETCGCardLocation::Board;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	bool bRequireTopCard = true;
};

/**
 * One reusable piece of an effect.
 */
USTRUCT(BlueprintType)
struct FTCGEffectStep
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectStepType StepType = ETCGEffectStepType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectTargetMode TargetMode = ETCGEffectTargetMode::None;

	// Generic number used by the step.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	int32 Value = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectValueMode ValueMode = ETCGEffectValueMode::Fixed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectSelectionMode SelectionMode = ETCGEffectSelectionMode::Automatic;

	// If true, this step only resolves when the previous meaningful step succeeded.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	bool bRequiresPreviousStepSuccess = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	FTCGEffectTargetFilter TargetFilter;
};

/**
 * Effect reference and optional inline modular effect.
 *
 * EffectId remains only as a temporary migration field for existing debug assets.
 * Real effects should use modular Steps.
 */
USTRUCT(BlueprintType)
struct FTCGCardEffectRef
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	FName EffectId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	TArray<FTCGEffectStep> Steps;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	bool bOptional = false;
};

/**
 * One unique card copy inside the current match.
 */
USTRUCT(BlueprintType)
struct FTCGCardInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FName CardDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 OwnerPlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	ETCGCardLocation Location = ETCGCardLocation::None;
	
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 LocationIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FName ZoneId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack")
	FGuid StackId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack")
	int32 StackIndex = INDEX_NONE;
	
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	ETCGCardElement Element = ETCGCardElement::Fire;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 BaseAttack = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 AttackModifier = 0;

public:
	FTCGCardInstance()
	{
		CardInstanceId = FGuid::NewGuid();
		StackId.Invalidate();
	}
};
