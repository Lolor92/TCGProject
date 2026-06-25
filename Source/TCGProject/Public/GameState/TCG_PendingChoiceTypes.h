#pragma once

#include "CoreMinimal.h"

#include "TCG_PendingChoiceTypes.generated.h"

USTRUCT(BlueprintType)
struct FTCGPendingDiscardChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 RequiredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		RequiredCount = 0;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingGraveyardToDeckChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 RequiredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		RequiredCount = 0;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingHandToTopDeckChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 RequiredCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		RequiredCount = 0;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingPlayToEmptyZoneChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FName> EligibleZoneIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleZoneIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingAttachSourceToWaterUnitChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleTargetCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleTargetCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingRevealDeckChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 RequiredCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> RevealedCardInstanceIds;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		RequiredCount = 1;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		RevealedCardInstanceIds.Reset();
		EligibleCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingPlayGraveyardCardToEmptyZoneChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FName> EligibleZoneIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleCardInstanceIds.Reset();
		EligibleZoneIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingGraveyardCardsToHandAndTopDeckChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingRemoveMaterialChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid TargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleMaterialCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		TargetCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleMaterialCardInstanceIds.Reset();
	}
};

USTRUCT(BlueprintType)
struct FTCGPendingSwapOpponentUnitZonesChoice
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	bool bIsPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 PlayerIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	int32 ChainIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Choice")
	TArray<FGuid> EligibleTargetCardInstanceIds;

	void Reset()
	{
		bIsPending = false;
		PlayerIndex = INDEX_NONE;
		SourceCardInstanceId.Invalidate();
		ChainIndex = INDEX_NONE;
		EligibleTargetCardInstanceIds.Reset();
	}
};