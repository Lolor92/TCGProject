#pragma once

#include "CoreMinimal.h"
#include "TCG_CardTypes.generated.h"

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

UENUM(BlueprintType)
enum class ETCGEffectTrigger : uint8
{
	None                      UMETA(DisplayName = "None"),
	OnPlay                    UMETA(DisplayName = "On Play"),
	OnDestroyed               UMETA(DisplayName = "On Destroyed"),
	OnSentToGraveyard         UMETA(DisplayName = "On Sent To Graveyard"),
	OnSentFromDeckToGraveyard     UMETA(DisplayName = "On Sent From Deck To Graveyard"),
	OnSentFromHandToGraveyard     UMETA(DisplayName = "On Sent From Hand To Graveyard"),
	OnSentFromBoardToGraveyard    UMETA(DisplayName = "On Sent From Board To Graveyard"),
	OnSentFromMaterialToGraveyard UMETA(DisplayName = "On Sent From Material To Graveyard"),
	OnBanished                UMETA(DisplayName = "On Banished"),
	OnBattleStart             UMETA(DisplayName = "On Battle Start"),
	OnBattleEnd               UMETA(DisplayName = "On Battle End"),
	OnStackAdded              UMETA(DisplayName = "On Stack Added"),
	OnBecomingTopCard         UMETA(DisplayName = "On Becoming Top Card"),
	OnAttack                  UMETA(DisplayName = "On Attack"),
	OnCardSentToYourGraveyard UMETA(DisplayName = "On Card Sent To Your Graveyard"),
	OnDestroyedUnitByBattle   UMETA(DisplayName = "On Destroyed Unit By Battle"),
	OnDestroyedByBattle       UMETA(DisplayName = "On Destroyed By Battle")
};

UENUM(BlueprintType)
enum class ETCGEffectStepType : uint8
{
	None                                UMETA(DisplayName = "None"),
	Then                                UMETA(DisplayName = "Then"),
	DrawCards                           UMETA(DisplayName = "Draw Cards"),
	DiscardCards                        UMETA(DisplayName = "Discard Cards"),
	ModifyAttack                        UMETA(DisplayName = "Modify Attack"),
	SelectTarget                        UMETA(DisplayName = "Select Target"),
	MoveBottomOverlayToGraveyard        UMETA(DisplayName = "Move Bottom Overlay To Graveyard"),
	PlaySourceToEmptyZone               UMETA(DisplayName = "Play Source To Empty Zone"),
	SendTopDeckCardToGraveyard          UMETA(DisplayName = "Send Top Deck Card To Graveyard"),
	MoveGraveyardCardToBottomDeck       UMETA(DisplayName = "Move Graveyard Card To Bottom Deck"),
	AttachSourceToWaterUnitMaterial     UMETA(DisplayName = "Attach Source To Water Unit Material Legacy"),
	AttachSourceToUnitMaterial          UMETA(DisplayName = "Attach Source To Unit Material"),
	MoveHandCardToTopDeck               UMETA(DisplayName = "Move Hand Card To Top Deck"),
	AttachGraveyardCardToSourceMaterial UMETA(DisplayName = "Attach Graveyard Card To Source Material"),
	SendTopDeckCardsToGraveyard         UMETA(DisplayName = "Send Top Deck Cards To Graveyard"),
	BoostAllOwnUnitsThisRound           UMETA(DisplayName = "Boost All Own Units This Round"),
	RevealTopDeckCardsAddElementToHand  UMETA(DisplayName = "Reveal Top Deck Cards Add Element To Hand"),
	PlayGraveyardCardToEmptyZone        UMETA(DisplayName = "Play Graveyard Card To Empty Zone")
};

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

UENUM(BlueprintType)
enum class ETCGEffectValueMode : uint8
{
	Fixed                             UMETA(DisplayName = "Fixed"),
	CardsUnderneathSource             UMETA(DisplayName = "Cards Underneath Source"),
	CardsUnderneathTarget             UMETA(DisplayName = "Cards Underneath Target"),
	WaterCardsInControllerGraveyard   UMETA(DisplayName = "Water Cards In Controller Graveyard Legacy"),
	ElementCardsInControllerGraveyard UMETA(DisplayName = "Element Cards In Controller Graveyard")
};

UENUM(BlueprintType)
enum class ETCGEffectSelectionMode : uint8
{
	Automatic    UMETA(DisplayName = "Automatic"),
	PlayerChoice UMETA(DisplayName = "Player Choice")
};

USTRUCT(BlueprintType)
struct FTCGEffectTargetFilter
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectTargetMode OwnerMode = ETCGEffectTargetMode::Controller;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGCardLocation RequiredLocation = ETCGCardLocation::Board;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") bool bRequireTopCard = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") bool bRequireElement = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect", meta = (EditCondition = "bRequireElement")) ETCGCardElement RequiredElement = ETCGCardElement::Water;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") bool bExcludeSourceCard = false;
};

USTRUCT(BlueprintType)
struct FTCGEffectStep
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectStepType StepType = ETCGEffectStepType::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectTargetMode TargetMode = ETCGEffectTargetMode::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") int32 Value = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectValueMode ValueMode = ETCGEffectValueMode::Fixed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectSelectionMode SelectionMode = ETCGEffectSelectionMode::Automatic;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") bool bRequiresPreviousStepSuccess = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") FTCGEffectTargetFilter TargetFilter;
};

USTRUCT(BlueprintType)
struct FTCGCardEffectRef
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") FName EffectId = NAME_None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") TArray<FTCGEffectStep> Steps;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effect") bool bOptional = false;
};

USTRUCT(BlueprintType)
struct FTCGCardInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") FGuid CardInstanceId;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") FName CardDefinitionId = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") int32 OwnerPlayerIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") ETCGCardLocation Location = ETCGCardLocation::None;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") int32 LocationIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") FName ZoneId = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack") FGuid StackId;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Stack") int32 StackIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") ETCGCardElement Element = ETCGCardElement::Fire;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") int32 BaseAttack = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Card") int32 AttackModifier = 0;
	FTCGCardInstance(){ CardInstanceId = FGuid::NewGuid(); StackId.Invalidate(); }
};