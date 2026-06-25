#include "GameState/TCG_GameState.h"

int32 ATCG_GameState::BuildYourUnitDestroyedGraveyardResponseChain(
	int32 DestroyedUnitOwnerPlayerIndex,
	const FGuid& DestroyedTopCardInstanceId,
	TArray<FTCGEffectChainEntry>& OutChain)
{
	if (!IsValidPlayerIndex(DestroyedUnitOwnerPlayerIndex) || !DestroyedTopCardInstanceId.IsValid())
	{
		return 0;
	}

	const int32 StartingCount = OutChain.Num();

	TArray<FTCGCardInstance> GraveyardCards;
	GetCardsInLocation(DestroyedUnitOwnerPlayerIndex, ETCGCardLocation::Graveyard, GraveyardCards);

	for (const FTCGCardInstance& GraveyardCard : GraveyardCards)
	{
		if (GraveyardCard.OwnerPlayerIndex != DestroyedUnitOwnerPlayerIndex) continue;
		if (GraveyardCard.Location != ETCGCardLocation::Graveyard) continue;

		TArray<FTCGCardEffectRef> EffectRefs;
		GetPrintedEffectRefsForCard(GraveyardCard, EffectRefs);

		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnYourUnitDestroyed))
			{
				AddCardEffectRefToChain(
					OutChain,
					GraveyardCard.CardInstanceId,
					DestroyedTopCardInstanceId,
					EffectRef);
			}
		}
	}

	return OutChain.Num() - StartingCount;
}

int32 ATCG_GameState::BuildYourUnitDestroyedByOpponentCardEffectGraveyardResponseChain(
int32 DestroyedUnitOwnerPlayerIndex,
const FGuid& DestroyedTopCardInstanceId,
const FGuid& DestroyingUnitCardInstanceId,
TArray<FTCGEffectChainEntry>& OutChain)
{
OutChain.Reset();

if (!IsValidPlayerIndex(DestroyedUnitOwnerPlayerIndex)
|| !DestroyedTopCardInstanceId.IsValid()
|| !DestroyingUnitCardInstanceId.IsValid())
{
return 0;
}

const FTCGCardInstance* DestroyingUnit = FindCardInstance(DestroyingUnitCardInstanceId);
if (!DestroyingUnit
|| DestroyingUnit->Location != ETCGCardLocation::Board
|| DestroyingUnit->OwnerPlayerIndex == DestroyedUnitOwnerPlayerIndex)
{
return 0;
}

TArray<FTCGCardInstance> GraveyardCards;
GetCardsInLocation(DestroyedUnitOwnerPlayerIndex, ETCGCardLocation::Graveyard, GraveyardCards);

for (const FTCGCardInstance& GraveyardCard : GraveyardCards)
{
TArray<FTCGCardEffectRef> EffectRefs;
GetPrintedEffectRefsForCard(GraveyardCard, EffectRefs);

for (const FTCGCardEffectRef& EffectRef : EffectRefs)
{
if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnYourUnitDestroyedByOpponentCardEffect))
{
AddCardEffectRefToChain(
OutChain,
GraveyardCard.CardInstanceId,
DestroyingUnitCardInstanceId,
EffectRef);
}
}
}

return OutChain.Num();
}
