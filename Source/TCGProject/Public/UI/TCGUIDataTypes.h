#pragma once

#include "CoreMinimal.h"
#include "TCGUIDataTypes.generated.h"

class UObject;

USTRUCT(BlueprintType)
struct TCGPROJECT_API FTCGUIEffectLine
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FText EffectText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FName TriggerName = NAME_None;
};

USTRUCT(BlueprintType)
struct TCGPROJECT_API FTCGCardWidgetData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FGuid CardInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FText CardName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FName Element = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 HandIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 StackCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 MaterialCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	bool bIsTopCard = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	bool bIsPlayable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	bool bCanOverlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FText ZoneLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	TArray<FTCGUIEffectLine> Effects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	TObjectPtr<UObject> SourceObject = nullptr;
};

USTRUCT(BlueprintType)
struct TCGPROJECT_API FTCGHandWidgetData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	TArray<FTCGCardWidgetData> Cards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 SelectedHandIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct TCGPROJECT_API FTCGMatchHUDWidgetData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 LocalDeckCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 LocalGraveyardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 LocalBanishedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 OpponentHandCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 OpponentDeckCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 OpponentGraveyardCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	int32 OpponentBanishedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FText PhaseText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FText TurnPlayerText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TCG|UI")
	FTCGHandWidgetData LocalHand;
};