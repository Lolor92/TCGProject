#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Cards/TCG_CardTypes.h"
#include "TCG_CardDefinition.generated.h"

/**
 * Static card data.
 *
 * This is the card template.
 * Runtime match data belongs in FTCGCardInstance, not here.
 */
UCLASS(BlueprintType)
class TCGPROJECT_API UTCG_CardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Stable ID used by decks, save data, and match instances.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Card")
	FName CardDefinitionId = NAME_None;

	// Name shown to the player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Card")
	FText DisplayName;

	// Card element: Fire, Wind, Earth, Water, Light, or Dark.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Card")
	ETCGCardElement Element = ETCGCardElement::Fire;

	// Base attack printed on the card.
	// Final attack later becomes:
	// BaseAttack + number of cards underneath this card.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Card")
	int32 BaseAttack = 0;

	// Text shown on the card.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Card")
	FText Description;

	// Effect references.
	// We only store the trigger and effect ID for now.
	// Full effect logic comes later.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TCG|Effects")
	TArray<FTCGCardEffectRef> Effects;
};
