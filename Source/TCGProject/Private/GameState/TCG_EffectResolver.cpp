#include "GameState/TCG_EffectResolver.h"

namespace
{
	constexpr bool bLogEffectResolverChainHelpers = false;

	const FName ResolverLegacyDebugEffect_Draw1 = "Debug_Draw1";
	const FName ResolverLegacyDebugEffect_GainAttackForCardsUnderneath = "Debug_GainAttackForCardsUnderneath";
	const FName ResolverLegacyDebugEffect_RemoveBottomOverlay = "Debug_RemoveBottomOverlay";

	static bool ConvertEffectResolverLegacyDebugEffectToSteps(FTCGCardEffectRef& EffectRef)
	{
		if (EffectRef.Steps.Num() > 0 || EffectRef.EffectId.IsNone()) return false;
		if (EffectRef.EffectId == ResolverLegacyDebugEffect_Draw1)
		{
			FTCGEffectStep DrawStep;
			DrawStep.StepType = ETCGEffectStepType::DrawCards;
			DrawStep.TargetMode = ETCGEffectTargetMode::Controller;
			DrawStep.Value = 1;
			EffectRef.Steps.Add(DrawStep);
			return true;
		}
		if (EffectRef.EffectId == ResolverLegacyDebugEffect_GainAttackForCardsUnderneath)
		{
			FTCGEffectStep GainAttackStep;
			GainAttackStep.StepType = ETCGEffectStepType::ModifyAttack;
			GainAttackStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
			GainAttackStep.ValueMode = ETCGEffectValueMode::CardsUnderneathTarget;
			EffectRef.Steps.Add(GainAttackStep);
			return true;
		}
		if (EffectRef.EffectId == ResolverLegacyDebugEffect_RemoveBottomOverlay)
		{
			FTCGEffectStep MoveOverlayStep;
			MoveOverlayStep.StepType = ETCGEffectStepType::MoveBottomOverlayToGraveyard;
			MoveOverlayStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
			EffectRef.Steps.Add(MoveOverlayStep);
			return true;
		}
		return false;
	}
}

bool UTCG_EffectResolver::DoesCardEffectMatchTrigger(const FTCGCardEffectRef& EffectRef, ETCGEffectTrigger Trigger)
{
	return Trigger != ETCGEffectTrigger::None
		&& EffectRef.Trigger == Trigger
		&& (!EffectRef.EffectId.IsNone() || EffectRef.Steps.Num() > 0);
}

bool UTCG_EffectResolver::AddCardTriggerToChain(ATCG_GameState* GameState, TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, ETCGEffectTrigger Trigger, FName EffectId)
{
	FTCGCardEffectRef LegacyEffectRef;
	LegacyEffectRef.Trigger = Trigger;
	LegacyEffectRef.EffectId = EffectId;
	return AddCardEffectRefToChain(GameState, Chain, SourceCardInstanceId, TargetCardInstanceId, LegacyEffectRef);
}

void UTCG_EffectResolver::ApplyDebugEffectChainEntryRequirements(ATCG_GameState* GameState, FTCGEffectChainEntry& ChainEntry)
{
	if (!GameState) return;

	const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
	const FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);
	if (!SourceCard || !TargetCard) return;

	ChainEntry.bRequiresSourceOnBoard = true;
	ChainEntry.bRequiresTargetOnBoard = true;
	ChainEntry.bRequiresSourceInTargetStack = false;
	ChainEntry.bRequiresSourceUnderTarget = false;

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnSentToGraveyard
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnSentFromDeckToGraveyard
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnSentFromHandToGraveyard
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnSentFromBoardToGraveyard
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnSentFromMaterialToGraveyard)
	{
		ChainEntry.bRequiresSourceOnBoard = false;
		ChainEntry.bRequiresTargetOnBoard = false;
	}

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyedByBattle)
	{
		ChainEntry.bRequiresSourceOnBoard = false;
		ChainEntry.bRequiresTargetOnBoard = true;
	}

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyed)
	{
		ChainEntry.bRequiresSourceOnBoard = false;
		ChainEntry.bRequiresTargetOnBoard = false;
	}

	if (ChainEntry.EffectId == ResolverLegacyDebugEffect_Draw1 && SourceCard->CardInstanceId != TargetCard->CardInstanceId)
	{
		ChainEntry.bRequiresSourceInTargetStack = true;
		ChainEntry.bRequiresSourceUnderTarget = true;
	}
}

bool UTCG_EffectResolver::CanResolveEffectChainEntry(const ATCG_GameState* GameState, const FTCGEffectChainEntry& ChainEntry)
{
	if (!GameState) return false;

	const FTCGCardInstance* SourceCard = GameState->FindCardInstance(ChainEntry.SourceCardInstanceId);
	const FTCGCardInstance* TargetCard = GameState->FindCardInstance(ChainEntry.TargetCardInstanceId);

	if (!SourceCard || !TargetCard) return false;
	if (ChainEntry.bRequiresSourceOnBoard && SourceCard->Location != ETCGCardLocation::Board) return false;
	if (ChainEntry.bRequiresTargetOnBoard && TargetCard->Location != ETCGCardLocation::Board) return false;
	if (ChainEntry.bRequiresSourceInTargetStack && (!SourceCard->StackId.IsValid() || SourceCard->StackId != TargetCard->StackId)) return false;
	if (ChainEntry.bRequiresSourceUnderTarget && (SourceCard->CardInstanceId == TargetCard->CardInstanceId || SourceCard->StackIndex >= TargetCard->StackIndex)) return false;
	return true;
}

bool UTCG_EffectResolver::AddCardEffectRefToChain(ATCG_GameState* GameState, TArray<FTCGEffectChainEntry>& Chain, const FGuid& SourceCardInstanceId, const FGuid& TargetCardInstanceId, const FTCGCardEffectRef& EffectRef)
{
	if (!GameState) return false;

	const FTCGCardInstance* SourceCard = GameState->FindCardInstance(SourceCardInstanceId);
	if (!SourceCard || EffectRef.Trigger == ETCGEffectTrigger::None) return false;

	FTCGCardEffectRef ResolvedEffectRef = EffectRef;
	const bool bConvertedLegacyEffect = ConvertEffectResolverLegacyDebugEffectToSteps(ResolvedEffectRef);
	if (ResolvedEffectRef.Steps.Num() <= 0) return false;

	FTCGEffectChainEntry NewEntry;
	NewEntry.ChainIndex = Chain.Num() + 1;
	NewEntry.SourceCardInstanceId = SourceCardInstanceId;
	NewEntry.TargetCardInstanceId = TargetCardInstanceId;
	NewEntry.SourceCardDefinitionId = SourceCard->CardDefinitionId;
	NewEntry.Trigger = ResolvedEffectRef.Trigger;
	NewEntry.EffectId = ResolvedEffectRef.EffectId;
	NewEntry.EffectRef = ResolvedEffectRef;
	NewEntry.ControllerPlayerIndex = SourceCard->OwnerPlayerIndex;
	ApplyDebugEffectChainEntryRequirements(GameState, NewEntry);
	if (NewEntry.Trigger == ETCGEffectTrigger::OnDestroyed)
	{
		NewEntry.bRequiresSourceOnBoard = false;
		NewEntry.bRequiresTargetOnBoard = true;
	}
	if (NewEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyedByBattle)
	{
		NewEntry.bRequiresSourceOnBoard = false;
		NewEntry.bRequiresTargetOnBoard = true;
	}
	if (NewEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyed)
	{
		NewEntry.bRequiresSourceOnBoard = false;
		NewEntry.bRequiresTargetOnBoard = false;
	}
	Chain.Add(NewEntry);

	if (bLogEffectResolverChainHelpers)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain add %d Source=%s EffectId=%s Steps=%d Optional=%s ConvertedLegacy=%s"), NewEntry.ChainIndex, *NewEntry.SourceCardDefinitionId.ToString(), NewEntry.EffectId.IsNone() ? TEXT("None") : *NewEntry.EffectId.ToString(), NewEntry.EffectRef.Steps.Num(), NewEntry.EffectRef.bOptional ? TEXT("true") : TEXT("false"), bConvertedLegacyEffect ? TEXT("true") : TEXT("false"));
	}
	return true;
}
