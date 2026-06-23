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
 *
 * We are not implementing effects yet.
 * This enum only gives the data model the correct shape for later.
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
 * Lightweight effect reference.
 *
 * Later, EffectId can point to a data asset, gameplay effect, script object,
 * or custom effect resolver.
 */
USTRUCT(BlueprintType)
struct FTCGCardEffectRef
{
	GENERATED_BODY()

public:
	// When this effect wants to trigger.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;

	// Stable ID for the actual effect logic later.
	// Example: "DrawOneCard", "DestroyWeakestEnemy", "GainAttack".
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect")
	FName EffectId = NAME_None;
};

/**
 * One unique card copy inside the current match.
 *
 * Example:
 * You can have 3 copies of the same card definition.
 * Each copy needs its own CardInstanceId.
 */
USTRUCT(BlueprintType)
struct FTCGCardInstance
{
	GENERATED_BODY()

public:
	// Unique ID for this specific card copy during the match.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FGuid CardInstanceId;

	// ID of the static card data.
	// Example: "WaterSprite", "FireWolf".
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FName CardDefinitionId = NAME_None;

	// Which player owns this card.
	// Usually 0 or 1.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 OwnerPlayerIndex = INDEX_NONE;

	// Current gameplay location.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	ETCGCardLocation Location = ETCGCardLocation::None;
	
	// Order inside Deck/Hand/Graveyard/Banish.
	// For deck, higher index means closer to the top for now.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 LocationIndex = INDEX_NONE;

	// Board zone/slot name later.
	// Example: "Player0_Field_0".
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	FName ZoneId = NAME_None;

	// ID of the stack this card belongs to.
	// Cards in the same stack share this value.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack")
	FGuid StackId;

	// Position inside the stack.
	// 0 = bottom card.
	// Higher number = closer to the top.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack")
	int32 StackIndex = INDEX_NONE;
	
	// Runtime snapshot of the card element.
	// This lets rules check stack legality without loading the card asset every time.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	ETCGCardElement Element = ETCGCardElement::Fire;

	// Runtime snapshot of printed attack.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 BaseAttack = 0;
	
	// Temporary runtime attack modifier.
	// Later this should become a proper modifier/effect system.
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card")
	int32 AttackModifier = 0;

public:
	FTCGCardInstance()
	{
		CardInstanceId = FGuid::NewGuid();
		StackId.Invalidate();
	}
};
