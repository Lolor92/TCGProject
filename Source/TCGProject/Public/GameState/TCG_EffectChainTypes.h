#pragma once

#include "CoreMinimal.h"
#include "Cards/TCG_CardTypes.h"
#include "TCG_EffectChainTypes.generated.h"

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
	FGuid SecondaryTargetCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	TArray<FGuid> SelectedTargetCardInstanceIds;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FName SourceCardDefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	ETCGEffectTrigger Trigger = ETCGEffectTrigger::None;

	UPROPERTY(BlueprintReadOnly, Category = "TCG|Effects")
	FName EffectId = NAME_None;

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
