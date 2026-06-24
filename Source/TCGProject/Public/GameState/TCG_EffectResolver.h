#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Cards/TCG_CardTypes.h"
#include "GameState/TCG_GameState.h"
#include "TCG_EffectResolver.generated.h"

class ATCG_GameState;

UCLASS()
class TCGPROJECT_API UTCG_EffectResolver : public UObject
{
	GENERATED_BODY()

public:
	static bool DoesCardEffectMatchTrigger(const FTCGCardEffectRef& EffectRef, ETCGEffectTrigger Trigger);
	static bool AddCardTriggerToChain(ATCG_GameState* GameState, TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, ETCGEffectTrigger Trigger, FName EffectId);
	static bool AddCardEffectRefToChain(ATCG_GameState* GameState, TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef);
	static void ApplyDebugEffectChainEntryRequirements(ATCG_GameState* GameState, FTCGEffectChainEntry& ChainEntry);
	static bool CanResolveEffectChainEntry(const ATCG_GameState* GameState, const FTCGEffectChainEntry& ChainEntry);
};
