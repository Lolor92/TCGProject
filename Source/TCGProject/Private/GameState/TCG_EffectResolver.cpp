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
static bool IsHandAttachToPlayedUnitEffect(const FTCGCardEffectRef& EffectRef)
{
for (const FTCGEffectStep& Step : EffectRef.Steps)
{
if (Step.StepType == ETCGEffectStepType::AttachSourceToUnitMaterial
&& Step.TargetMode == ETCGEffectTargetMode::TriggerTarget)
{
return true;
}
}

return false;
}

static int32 CountMaterialsUnderUnitForActivationCheck(
const ATCG_GameState* GameState,
const FGuid& TopCardInstanceId)
{
if (!GameState || !TopCardInstanceId.IsValid())
{
return 0;
}

const FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardInstanceId);
if (!TopCard || TopCard->Location != ETCGCardLocation::Board || !TopCard->StackId.IsValid())
{
return 0;
}

int32 Count = 0;
for (const FTCGCardInstance& Card : GameState->MatchCards)
{
if (Card.Location == ETCGCardLocation::Board
&& Card.StackId == TopCard->StackId
&& Card.CardInstanceId != TopCard->CardInstanceId
&& Card.StackIndex < TopCard->StackIndex)
{
Count++;
}
}

return Count;
}

static bool DoesCardNameMatchFilterForActivationCheck(
const FTCGCardInstance& Card,
const FTCGEffectTargetFilter& Filter)
{
const FString CardDefinitionIdString = Card.CardDefinitionId.ToString();

if (!Filter.NameContains.IsEmpty()
&& !CardDefinitionIdString.Contains(Filter.NameContains, ESearchCase::IgnoreCase))
{
return false;
}

if (Filter.NameContainsAny.Num() <= 0)
{
return true;
}

for (const FString& NameOption : Filter.NameContainsAny)
{
if (!NameOption.IsEmpty()
&& CardDefinitionIdString.Contains(NameOption, ESearchCase::IgnoreCase))
{
return true;
}
}

return false;
}

static bool DoesTopBoardUnitMatchActivationFilter(
const ATCG_GameState* GameState,
const FTCGCardInstance& Card,
const FTCGEffectTargetFilter& Filter,
int32 ControllerPlayerIndex,
const FGuid& SourceCardInstanceId)
{
if (!GameState)
{
return false;
}

const int32 RequiredOwner =
Filter.OwnerMode == ETCGEffectTargetMode::Opponent
? 1 - ControllerPlayerIndex
: ControllerPlayerIndex;

if (Card.OwnerPlayerIndex != RequiredOwner) return false;
if (Card.Location != ETCGCardLocation::Board) return false;
if (!Card.StackId.IsValid()) return false;
if (Filter.RequiredLocation != ETCGCardLocation::None
&& Filter.RequiredLocation != ETCGCardLocation::Board) return false;
if (Filter.bRequireElement && Card.Element != Filter.RequiredElement) return false;
if (Filter.bExcludeSourceCard && Card.CardInstanceId == SourceCardInstanceId) return false;
if (!DoesCardNameMatchFilterForActivationCheck(Card, Filter)) return false;

if (Filter.bRequireTopCard)
{
const FTCGCardInstance* TopCard = GameState->FindTopCardInStack(Card.StackId);
if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId)
{
return false;
}
}

return true;
}

static bool EffectHasMatchingMaterialDestroyStep(const FTCGCardEffectRef& EffectRef)
{
for (const FTCGEffectStep& Step : EffectRef.Steps)
{
if (Step.StepType == ETCGEffectStepType::DestroyUnitsWithMatchingMaterialCount)
{
return true;
}
}

return false;
}

static bool HasValidMatchingMaterialDestroyPairForActivation(
const ATCG_GameState* GameState,
const FTCGCardEffectRef& EffectRef,
int32 ControllerPlayerIndex,
const FGuid& SourceCardInstanceId)
{
if (!GameState)
{
return false;
}

for (const FTCGEffectStep& Step : EffectRef.Steps)
{
if (Step.StepType != ETCGEffectStepType::DestroyUnitsWithMatchingMaterialCount)
{
continue;
}

FTCGEffectTargetFilter OwnFilter = Step.TargetFilter;
OwnFilter.OwnerMode = ETCGEffectTargetMode::Controller;
OwnFilter.RequiredLocation = ETCGCardLocation::Board;
OwnFilter.bRequireTopCard = true;

FTCGEffectTargetFilter OpponentFilter = Step.SecondaryTargetFilter;
OpponentFilter.OwnerMode = ETCGEffectTargetMode::Opponent;
OpponentFilter.RequiredLocation = ETCGCardLocation::Board;
OpponentFilter.bRequireTopCard = true;

for (const FTCGCardInstance& OwnCard : GameState->MatchCards)
{
if (!DoesTopBoardUnitMatchActivationFilter(
GameState,
OwnCard,
OwnFilter,
ControllerPlayerIndex,
SourceCardInstanceId))
{
continue;
}

const int32 OwnMaterialCount = CountMaterialsUnderUnitForActivationCheck(
GameState,
OwnCard.CardInstanceId);

for (const FTCGCardInstance& OpponentCard : GameState->MatchCards)
{
if (!DoesTopBoardUnitMatchActivationFilter(
GameState,
OpponentCard,
OpponentFilter,
ControllerPlayerIndex,
SourceCardInstanceId))
{
continue;
}

const int32 OpponentMaterialCount = CountMaterialsUnderUnitForActivationCheck(
GameState,
OpponentCard.CardInstanceId);

if (OpponentMaterialCount == OwnMaterialCount)
{
return true;
}
}
}

return false;
}

return true;
}

static bool DoesSourceCardMatchSourceFilterForEffect(
	const ATCG_GameState* GameState,
	const FTCGCardInstance& SourceCard,
	const FTCGCardEffectRef& EffectRef)
{
	if (!EffectRef.bUseSourceFilter)
	{
		return true;
	}

	const FTCGEffectTargetFilter& Filter = EffectRef.SourceFilter;
	const int32 SourceControllerPlayerIndex = SourceCard.OwnerPlayerIndex;
	const int32 RequiredOwner =
		Filter.OwnerMode == ETCGEffectTargetMode::Opponent
			? 1 - SourceControllerPlayerIndex
			: SourceControllerPlayerIndex;

	if (SourceCard.OwnerPlayerIndex != RequiredOwner)
	{
		return false;
	}

	if (Filter.RequiredLocation != ETCGCardLocation::None
		&& SourceCard.Location != Filter.RequiredLocation)
	{
		return false;
	}

	if (Filter.bRequireElement && SourceCard.Element != Filter.RequiredElement)
	{
		return false;
	}

	if (!Filter.NameContains.IsEmpty()
		&& !SourceCard.CardDefinitionId.ToString().Contains(Filter.NameContains, ESearchCase::IgnoreCase))
	{
		return false;
	}

	if (Filter.bRequireTopCard && SourceCard.Location == ETCGCardLocation::Board)
	{
		const FTCGCardInstance* TopCard = GameState ? GameState->FindTopCardInStack(SourceCard.StackId) : nullptr;
		if (!TopCard || TopCard->CardInstanceId != SourceCard.CardInstanceId)
		{
			return false;
		}
	}

	return true;
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
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnSentFromMaterialToGraveyard
		|| ChainEntry.Trigger == ETCGEffectTrigger::OnMaterialOfDestroyedUnit)
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

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyedByOpponentCardEffect)
	{
		ChainEntry.bRequiresSourceOnBoard = false;
		ChainEntry.bRequiresTargetOnBoard = true;
	}

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnOpponentDrawsByCardEffect)
	{
		ChainEntry.bRequiresSourceOnBoard = false;
		ChainEntry.bRequiresTargetOnBoard = false;
	}

	if (ChainEntry.Trigger == ETCGEffectTrigger::OnYourUnitPlayed
&& IsHandAttachToPlayedUnitEffect(ChainEntry.EffectRef))
{
ChainEntry.bRequiresSourceOnBoard = false;
ChainEntry.bRequiresTargetOnBoard = true;
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
	if (!DoesSourceCardMatchSourceFilterForEffect(GameState, *SourceCard, ChainEntry.EffectRef)) return false;
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
	if (!DoesSourceCardMatchSourceFilterForEffect(GameState, *SourceCard, ResolvedEffectRef)) return false;

	if (EffectHasMatchingMaterialDestroyStep(ResolvedEffectRef)
		&& !HasValidMatchingMaterialDestroyPairForActivation(
			GameState,
			ResolvedEffectRef,
			SourceCard->OwnerPlayerIndex,
			SourceCardInstanceId))
	{
		if (bLogEffectResolverChainHelpers)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("TCG Effect: Chain skip Source=%s Reason=NoValidMatchingMaterialDestroyPair"),
				*SourceCard->CardDefinitionId.ToString());
		}

		return false;
	}

	if (ResolvedEffectRef.Trigger == ETCGEffectTrigger::OnPlay
		&& ResolvedEffectRef.TriggerFilter.bRequireTopCard
		&& SourceCard->Location == ETCGCardLocation::Board)
	{
		const FTCGCardInstance* CurrentTopCard = GameState->FindTopCardInStack(SourceCard->StackId);
		if (!CurrentTopCard || CurrentTopCard->CardInstanceId != SourceCard->CardInstanceId)
		{
			if (bLogEffectResolverChainHelpers)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("TCG Effect: Chain skip Source=%s Trigger=OnPlay Reason=OnPlaySourceNotTopCard"),
					*SourceCard->CardDefinitionId.ToString());
			}

			return false;
		}
	}

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

	if (NewEntry.Trigger == ETCGEffectTrigger::OnYourUnitDestroyedByOpponentCardEffect)
	{
		NewEntry.bRequiresSourceOnBoard = false;
		NewEntry.bRequiresTargetOnBoard = true;
	}

	if (NewEntry.Trigger == ETCGEffectTrigger::OnOpponentDrawsByCardEffect)
	{
		NewEntry.bRequiresSourceOnBoard = false;
		NewEntry.bRequiresTargetOnBoard = false;
	}
	if (NewEntry.Trigger == ETCGEffectTrigger::OnYourUnitPlayed
&& IsHandAttachToPlayedUnitEffect(NewEntry.EffectRef))
{
NewEntry.bRequiresSourceOnBoard = false;
NewEntry.bRequiresTargetOnBoard = true;
}
Chain.Add(NewEntry);

	if (bLogEffectResolverChainHelpers)
	{
		UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Chain add %d Source=%s EffectId=%s Steps=%d Optional=%s ConvertedLegacy=%s"), NewEntry.ChainIndex, *NewEntry.SourceCardDefinitionId.ToString(), NewEntry.EffectId.IsNone() ? TEXT("None") : *NewEntry.EffectId.ToString(), NewEntry.EffectRef.Steps.Num(), NewEntry.EffectRef.bOptional ? TEXT("true") : TEXT("false"), bConvertedLegacyEffect ? TEXT("true") : TEXT("false"));
	}
	return true;
}

