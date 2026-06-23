#pragma once

#include "CoreMinimal.h"
#include "TCG_CardPlacementTypes.generated.h"

UENUM(BlueprintType)
enum class ETCGCardZoneType : uint8
{
	/** A face-down draw pile. */
	Deck UMETA(DisplayName = "Deck"),

	/** A group of cards owned by a player but not on the field. */
	Hand UMETA(DisplayName = "Hand"),

	/** A discard pile for cards that have been used or destroyed. */
	Graveyard UMETA(DisplayName = "Graveyard"),

	/** A removed-from-play pile. */
	Banish UMETA(DisplayName = "Banish"),

	/** A numbered board slot where cards can be played. */
	Field UMETA(DisplayName = "Field"),

	/** A designer-defined zone for game-specific layouts. */
	Custom UMETA(DisplayName = "Custom")
};

USTRUCT(BlueprintType)
struct TCGPROJECT_API FTCGCardZoneId
{
	GENERATED_BODY()

	/** Groups zones that belong to the same board layout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Zone")
	FName BoardId = TEXT("MainBoard");

	/** Zero-based player owner. Use 0 for Player 1, 1 for Player 2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Zone", meta = (ClampMin = "0"))
	int32 PlayerIndex = 0;

	/** The broad gameplay purpose of this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Zone")
	ETCGCardZoneType ZoneType = ETCGCardZoneType::Field;

	/** Zero-based index within this zone type, such as field slot 0, 1, 2, 3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Zone", meta = (ClampMin = "0"))
	int32 ZoneIndex = 0;

	/** Optional exact name for special cases, labels, or Blueprint lookup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TCG|Zone")
	FName ZoneName = NAME_None;

	/** Loose match used by board lookup; an empty ZoneName acts like a wildcard. */
	bool Matches(const FTCGCardZoneId& Other) const
	{
		const bool bZoneNamesMatch = ZoneName.IsNone() || Other.ZoneName.IsNone() || ZoneName == Other.ZoneName;

		return BoardId == Other.BoardId
			&& PlayerIndex == Other.PlayerIndex
			&& ZoneType == Other.ZoneType
			&& ZoneIndex == Other.ZoneIndex
			&& bZoneNamesMatch;
	}
};
