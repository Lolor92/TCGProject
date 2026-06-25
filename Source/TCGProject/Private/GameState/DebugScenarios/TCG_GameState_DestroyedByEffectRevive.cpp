#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static FGuid AddDestroyedReviveDebugBoardUnit(
ATCG_GameState* GameState,
FName CardDefinitionId,
ETCGCardElement Element,
int32 Attack,
int32 PlayerIndex,
int32 FieldIndex)
{
FTCGCardInstance& Card = GameState->AddCardInstance(
CardDefinitionId,
Element,
Attack,
PlayerIndex,
ETCGCardLocation::Board);

Card.ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
Card.StackId = FGuid::NewGuid();
Card.StackIndex = 0;

return Card.CardInstanceId;
}
}

void RunDebugDestroyedByEffectReviveScenario(ATCG_GameState* GameState)
{
if (!GameState) return;

UE_LOG(LogTemp, Warning, TEXT("TCG Debug: DestroyedByEffectRevive scenario start"));

GameState->MatchCards.Empty();
GameState->SetMatchResult(ETCGMatchResult::None);
GameState->SetPhase(ETCGMatchPhase::Battle);
GameState->RoundNumber = 1;
GameState->TurnNumber = 1;

const FGuid VictimId = AddDestroyedReviveDebugBoardUnit(
GameState,
"Debug_P0_Destroyed_Revive_Victim",
ETCGCardElement::Earth,
1,
0,
0);

const FGuid DestroyerId = AddDestroyedReviveDebugBoardUnit(
GameState,
"Debug_P1_Destroyed_Revive_Destroyer",
ETCGCardElement::Dark,
10,
1,
0);

UTCG_CardDefinition* ResponseDefinition = NewObject<UTCG_CardDefinition>(GameState);
ResponseDefinition->CardDefinitionId = "Debug_P0_Destroyed_Revive_Response";
ResponseDefinition->DisplayName = FText::FromString(TEXT("Debug P0 Destroyed Revive Response"));
ResponseDefinition->Element = ETCGCardElement::Light;
ResponseDefinition->BaseAttack = 1;
ResponseDefinition->Description = FText::FromString(TEXT("If one of your units is destroyed by a card effect, discard this card; play the destroyed unit from your Graveyard into an empty zone."));

FTCGCardEffectRef ResponseEffect;
ResponseEffect.Trigger = ETCGEffectTrigger::OnYourUnitDestroyed;
ResponseEffect.bOptional = true;

FTCGEffectStep DiscardSourceStep;
DiscardSourceStep.StepType = ETCGEffectStepType::DiscardSource;
DiscardSourceStep.TargetMode = ETCGEffectTargetMode::SourceCard;
ResponseEffect.Steps.Add(DiscardSourceStep);

FTCGEffectStep PlayDestroyedStep;
PlayDestroyedStep.StepType = ETCGEffectStepType::PlayCardToEmptyZone;
PlayDestroyedStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
PlayDestroyedStep.bRequiresPreviousStepSuccess = true;
ResponseEffect.Steps.Add(PlayDestroyedStep);

ResponseDefinition->Effects.Add(ResponseEffect);
GameState->DebugCardDefinitions.Add(ResponseDefinition);

FTCGCardInstance* ResponseCard = GameState->AddCardInstanceFromDefinition(
ResponseDefinition,
0,
ETCGCardLocation::Hand);

if (!ResponseCard) return;
const FGuid ResponseCardId = ResponseCard->CardInstanceId;

FTCGCardEffectRef DestroyEffect;
DestroyEffect.Trigger = ETCGEffectTrigger::OnPlay;

FTCGEffectStep DestroyStep;
DestroyStep.StepType = ETCGEffectStepType::DestroyUnit;
DestroyStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
DestroyEffect.Steps.Add(DestroyStep);

FTCGEffectChainEntry DestroyChainEntry;
DestroyChainEntry.ChainIndex = 1;
DestroyChainEntry.SourceCardInstanceId = DestroyerId;
DestroyChainEntry.TargetCardInstanceId = VictimId;
DestroyChainEntry.SourceCardDefinitionId = "Debug_P1_Destroyed_Revive_Destroyer";
DestroyChainEntry.Trigger = ETCGEffectTrigger::OnPlay;
DestroyChainEntry.EffectRef = DestroyEffect;
DestroyChainEntry.ControllerPlayerIndex = 1;
DestroyChainEntry.bRequiresSourceOnBoard = true;
DestroyChainEntry.bRequiresTargetOnBoard = true;

TArray<FTCGEffectChainEntry> DestroyChain;
DestroyChain.Add(DestroyChainEntry);

const bool bResolved = GameState->ResolveEffectChain(DestroyChain);

const FTCGCardInstance* VictimAfter = GameState->FindCardInstance(VictimId);
const FTCGCardInstance* ResponseAfter = GameState->FindCardInstance(ResponseCardId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: DestroyedByEffectRevive summary Resolved=%s VictimBoard=%s VictimOwner=%d ResponseDiscarded=%s ExpectedVictimBoard=true ExpectedResponseDiscarded=true"),
bResolved ? TEXT("true") : TEXT("false"),
(VictimAfter && VictimAfter->Location == ETCGCardLocation::Board) ? TEXT("true") : TEXT("false"),
VictimAfter ? VictimAfter->OwnerPlayerIndex : INDEX_NONE,
(ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"));

GameState->EndMatch(ETCGMatchResult::Draw);
}
