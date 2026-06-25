#pragma once

#include "CoreMinimal.h"
#include "TCG_CardTypes.generated.h"

UENUM(BlueprintType)
enum class ETCGCardElement : uint8
{
	Fire UMETA(DisplayName = "Fire"),
	Wind UMETA(DisplayName = "Wind"),
	Earth UMETA(DisplayName = "Earth"),
	Water UMETA(DisplayName = "Water"),
	Light UMETA(DisplayName = "Light"),
	Dark UMETA(DisplayName = "Dark")
};

UENUM(BlueprintType)
enum class ETCGCardLocation : uint8
{
	None UMETA(DisplayName = "None"),
	Deck UMETA(DisplayName = "Deck"),
	Hand UMETA(DisplayName = "Hand"),
	Board UMETA(DisplayName = "Board"),
	Graveyard UMETA(DisplayName = "Graveyard"),
	Banish UMETA(DisplayName = "Banish")
};

UENUM(BlueprintType)
enum class ETCGEffectTrigger : uint8
{
	None UMETA(DisplayName = "None"),
	OnPlay UMETA(DisplayName = "01 - On Play"),
	OnDestroyed UMETA(DisplayName = "02 - On Destroyed"),
	OnSentToGraveyard UMETA(DisplayName = "03 - On Sent To Graveyard"),
	OnSentFromDeckToGraveyard UMETA(DisplayName = "04 - On Sent From Deck To Graveyard"),
	OnSentFromHandToGraveyard UMETA(DisplayName = "05 - On Sent From Hand To Graveyard"),
	OnSentFromBoardToGraveyard UMETA(DisplayName = "06 - On Sent From Board To Graveyard"),
	OnSentFromMaterialToGraveyard UMETA(DisplayName = "07 - On Sent From Material To Graveyard"),
	OnBanished UMETA(DisplayName = "08 - On Banished"),
	OnBattleStart UMETA(DisplayName = "09 - On Battle Start"),
	OnBattleEnd UMETA(DisplayName = "10 - On Battle End"),
	OnStackAdded UMETA(DisplayName = "11 - On Stack Added"),
	OnBecomingTopCard UMETA(DisplayName = "12 - On Becoming Top Card"),
	OnAttack UMETA(DisplayName = "13 - On Attack"),
	OnCardSentToYourGraveyard UMETA(DisplayName = "14 - On Card Sent To Your Graveyard"),
	OnDestroyedUnitByBattle UMETA(DisplayName = "15 - On Destroyed Unit By Battle"),
	OnDestroyedByBattle UMETA(DisplayName = "16 - On Destroyed By Battle"),
	OnYourUnitDestroyedByBattle UMETA(DisplayName = "17 - On Your Unit Destroyed By Battle"),
	OnOpponentAttack UMETA(DisplayName = "18 - On Opponent Attack"),
	OnOpponentUnitEffectActivatedWhenYourUnitAttacks UMETA(DisplayName = "19 - On Opponent Unit Effect Activated When Your Unit Attacks"),
	OnYourUnitWouldBeDestroyedByCardEffect UMETA(DisplayName = "20 - On Your Unit Would Be Destroyed By Card Effect"),
	OnYourUnitDestroyed UMETA(DisplayName = "21 - On Your Unit Destroyed"),
	OnYourUnitWouldLoseMaterialByCardEffect UMETA(DisplayName = "22 - On Your Unit Would Lose Material By Card Effect")
};

UENUM(BlueprintType)
enum class ETCGEffectStepType : uint8
{
	None UMETA(DisplayName = "None"),
	Then UMETA(DisplayName = "00 - Then / Divider"),
	DrawCards UMETA(DisplayName = "10 - Draw Cards"),
	DiscardCards UMETA(DisplayName = "11 - Discard Cards"),
	ModifyAttack UMETA(DisplayName = "12 - Modify Attack"),
	SelectTarget UMETA(DisplayName = "90 - Check Target Exists"),
	MoveBottomOverlayToGraveyard UMETA(DisplayName = "44 - Move Bottom Material To Graveyard"),
	PlaySourceToEmptyZone UMETA(DisplayName = "40 - Play Source To Empty Zone"),
	SendTopDeckCardToGraveyard UMETA(DisplayName = "20 - Send Top Deck Card To Graveyard"),
	MoveGraveyardCardToBottomDeck UMETA(DisplayName = "32 - Move Graveyard Card To Bottom Deck"),
	AttachSourceToWaterUnitMaterial UMETA(DisplayName = "Deprecated - Attach Source To Water Unit Material"),
	AttachSourceToUnitMaterial UMETA(DisplayName = "41 - Attach Source To Unit Material"),
	MoveHandCardToTopDeck UMETA(DisplayName = "23 - Move Hand Card To Top Deck"),
	AttachGraveyardCardToSourceMaterial UMETA(DisplayName = "42 - Attach Graveyard Card To Source Material"),
	SendTopDeckCardsToGraveyard UMETA(DisplayName = "21 - Send Top Deck Cards To Graveyard"),
	BoostAllOwnUnitsThisRound UMETA(DisplayName = "13 - Boost Own Units This Round"),
	RevealTopDeckCardsAddElementToHand UMETA(DisplayName = "22 - Reveal Top Deck Cards, Add Element To Hand"),
	PlayGraveyardCardToEmptyZone UMETA(DisplayName = "33 - Play Graveyard Card To Empty Zone"),
	MoveGraveyardCardsToHandAndTopDeck UMETA(DisplayName = "34 - Choose 2 Graveyard Cards To Hand / Top Deck"),
	RemoveMaterialFromTargetUnit UMETA(DisplayName = "43 - Remove Material From Target Unit"),
	MoveGraveyardCardToHand UMETA(DisplayName = "30 - Move Graveyard Card To Hand"),
	MoveGraveyardCardToTopDeck UMETA(DisplayName = "31 - Move Graveyard Card To Top Deck"),
	AttackMillTwoWaterBounceBattlingUnit UMETA(DisplayName = "45 - Attack: Remove Material, Mill 2, Bounce Battle Target If Water"),
	DiscardSourceDetachUpToTwoMaterialsFromTarget UMETA(DisplayName = "46 - Discard Source, Detach Up To 2 Materials From Target"),
	BanishSourceNegateOpponentAttackEffectActivation UMETA(DisplayName = "47 - Banish Source, Negate Opponent Attack Effect Activation"),
	DestroyTargetUnitByCardEffect UMETA(DisplayName = "48 - Destroy Target Unit By Card Effect"),
	DiscardSourceReturnTargetUnitToHandDrawIfTwoMaterials UMETA(DisplayName = "49 - Discard Source, Return Target Unit To Hand, Draw If 2+ Materials"),
	BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw UMETA(DisplayName = "50 - Banish Source, Return 2 Graveyard Cards To Bottom Deck, Both Draw"),
	SwapTwoOpponentUnitsZones UMETA(DisplayName = "51 - Choose 2 Opponent Units, Swap Zones"),
	DiscardSourcePreventMaterialLossByCardEffect UMETA(DisplayName = "52 - Discard Source, Prevent Material Loss By Card Effect"),

	// Generic material steps. Prefer these over rigid one-card effect steps.
	DetachMaterials UMETA(DisplayName = "54 - Generic: Detach Materials"),
	StealMaterials UMETA(DisplayName = "55 - Generic: Steal Materials"),
	DiscardSource UMETA(DisplayName = "56 - Generic: Discard Source"),
	DestroyUnit UMETA(DisplayName = "57 - Generic: Destroy Unit"),
	ReturnUnitToHand UMETA(DisplayName = "58 - Generic: Return Unit To Hand"),
	ReturnUnitToBottomDeck UMETA(DisplayName = "59 - Generic: Return Unit To Bottom Deck"),
	BanishSource UMETA(DisplayName = "60 - Generic: Banish Source"),
	DrawCardsForBothPlayers UMETA(DisplayName = "61 - Generic: Draw Cards For Both Players"),
	NegateActivation UMETA(DisplayName = "62 - Generic: Negate Activation"),

	// Legacy/debug-only rigid step. Keep temporarily while migrating old effects.
	AttackDetachTwoStealOneMaterial UMETA(DisplayName = "53 - Legacy Attack: Detach 2, Steal 1 Material")
};

UENUM(BlueprintType)
enum class ETCGEffectTargetMode : uint8
{
	None UMETA(DisplayName = "None"),
	Controller UMETA(DisplayName = "Controller"),
	Opponent UMETA(DisplayName = "Opponent"),
	SourceCard UMETA(DisplayName = "Source Card"),
	TriggerTarget UMETA(DisplayName = "Trigger Target"),
	SelectedTarget UMETA(DisplayName = "Selected Target")
};

UENUM(BlueprintType)
enum class ETCGEffectValueMode : uint8
{
	Fixed UMETA(DisplayName = "Fixed"),
	CardsUnderneathSource UMETA(DisplayName = "Cards Underneath Source"),
	CardsUnderneathTarget UMETA(DisplayName = "Cards Underneath Target"),
	WaterCardsInControllerGraveyard UMETA(DisplayName = "Water Cards In Controller Graveyard Legacy"),
	ElementCardsInControllerGraveyard UMETA(DisplayName = "Element Cards In Controller Graveyard")
};

UENUM(BlueprintType)
enum class ETCGEffectSelectionMode : uint8
{
	Automatic UMETA(DisplayName = "Automatic"),
	PlayerChoice UMETA(DisplayName = "Player Choice")
};

USTRUCT(BlueprintType)
struct FTCGEffectTargetFilter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target Filter")
	ETCGEffectTargetMode OwnerMode = ETCGEffectTargetMode::Controller;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target Filter")
	ETCGCardLocation RequiredLocation = ETCGCardLocation::Board;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Target Filter",
		meta = (EditCondition = "RequiredLocation == ETCGCardLocation::Board", EditConditionHides))
	bool bRequireTopCard = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target Filter")
	bool bRequireElement = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Target Filter",
		meta = (EditCondition = "bRequireElement", EditConditionHides))
	ETCGCardElement RequiredElement = ETCGCardElement::Water;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Target Filter")
	bool bExcludeSourceCard = false;
};

USTRUCT(BlueprintType)
struct FTCGEffectStep
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "1. Step")
	ETCGEffectStepType StepType = ETCGEffectStepType::None;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "2. Target",
		meta = (EditCondition = "StepType == ETCGEffectStepType::ModifyAttack || StepType == ETCGEffectStepType::MoveBottomOverlayToGraveyard || StepType == ETCGEffectStepType::RemoveMaterialFromTargetUnit || StepType == ETCGEffectStepType::AttachGraveyardCardToSourceMaterial || StepType == ETCGEffectStepType::DestroyTargetUnitByCardEffect || StepType == ETCGEffectStepType::DetachMaterials || StepType == ETCGEffectStepType::StealMaterials || StepType == ETCGEffectStepType::DestroyUnit || StepType == ETCGEffectStepType::ReturnUnitToHand || StepType == ETCGEffectStepType::ReturnUnitToBottomDeck", EditConditionHides))
	ETCGEffectTargetMode TargetMode = ETCGEffectTargetMode::None;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "3. Value",
		meta = (EditCondition = "StepType == ETCGEffectStepType::DrawCards || StepType == ETCGEffectStepType::DiscardCards || StepType == ETCGEffectStepType::ModifyAttack || StepType == ETCGEffectStepType::SendTopDeckCardsToGraveyard || StepType == ETCGEffectStepType::RevealTopDeckCardsAddElementToHand || StepType == ETCGEffectStepType::MoveGraveyardCardToBottomDeck || StepType == ETCGEffectStepType::MoveHandCardToTopDeck || StepType == ETCGEffectStepType::BoostAllOwnUnitsThisRound || StepType == ETCGEffectStepType::DetachMaterials || StepType == ETCGEffectStepType::StealMaterials || StepType == ETCGEffectStepType::ReturnUnitToHand || StepType == ETCGEffectStepType::DrawCardsForBothPlayers", EditConditionHides))
	int32 Value = 0;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "3. Value",
		meta = (EditCondition = "StepType == ETCGEffectStepType::ModifyAttack", EditConditionHides))
	ETCGEffectValueMode ValueMode = ETCGEffectValueMode::Fixed;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "4. Choice",
		meta = (EditCondition = "StepType != ETCGEffectStepType::None && StepType != ETCGEffectStepType::Then", EditConditionHides))
	ETCGEffectSelectionMode SelectionMode = ETCGEffectSelectionMode::Automatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "5. Chain")
	bool bRequiresPreviousStepSuccess = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "5. Chain",
		meta = (EditCondition = "StepType == ETCGEffectStepType::DetachMaterials", EditConditionHides))
	bool bAllowPartialSuccess = false;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "6. Filter",
		meta = (EditCondition = "StepType == ETCGEffectStepType::SelectTarget || StepType == ETCGEffectStepType::ModifyAttack || StepType == ETCGEffectStepType::RevealTopDeckCardsAddElementToHand || StepType == ETCGEffectStepType::MoveGraveyardCardToHand || StepType == ETCGEffectStepType::MoveGraveyardCardToTopDeck || StepType == ETCGEffectStepType::MoveGraveyardCardsToHandAndTopDeck || StepType == ETCGEffectStepType::PlayGraveyardCardToEmptyZone || StepType == ETCGEffectStepType::AttachGraveyardCardToSourceMaterial", EditConditionHides))
	FTCGEffectTargetFilter TargetFilter;
};

USTRUCT(BlueprintType)
struct FTCGCardEffectRef
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "1. Trigger")
	ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "2. Legacy")
	FName EffectId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "3. Steps")
	TArray<FTCGEffectStep> Steps;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "4. Timing")
	bool bOptional = false;
};

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

	FTCGCardInstance()
	{
		CardInstanceId = FGuid::NewGuid();
		StackId.Invalidate();
	}
};