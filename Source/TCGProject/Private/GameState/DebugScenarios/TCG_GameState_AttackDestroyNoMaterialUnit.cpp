#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static FGuid AddAttackNoMaterialDebugBoardUnit(
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

void RunDebugAttackDestroyNoMaterialUnitScenario(ATCG_GameState* GameState)
{
if (!GameState) return;

UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackDestroyNoMaterialUnit scenario start"));

GameState->MatchCards.Empty();
GameState->SetMatchResult(ETCGMatchResult::None);
GameState->SetPhase(ETCGMatchPhase::Battle);
GameState->RoundNumber = 1;
GameState->TurnNumber = 1;

UTCG_CardDefinition* AttackerDefinition = NewObject<UTCG_CardDefinition>(GameState);
AttackerDefinition->CardDefinitionId = "Debug_Attack_Destroy_NoMaterial_Attacker";
AttackerDefinition->DisplayName = FText::FromString(TEXT("Debug Attack Destroy No-Material Attacker"));
AttackerDefinition->Element = ETCGCardElement::Fire;
AttackerDefinition->BaseAttack = 1;
AttackerDefinition->Description = FText::FromString(TEXT("When this card attacks a unit with no material, destroy that unit before damage."));

FTCGCardEffectRef AttackEffect;
AttackEffect.Trigger = ETCGEffectTrigger::OnAttack;

FTCGEffectStep CheckNoMaterialStep;
CheckNoMaterialStep.StepType = ETCGEffectStepType::CheckMaterialCount;
CheckNoMaterialStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
CheckNoMaterialStep.Value = 0;
AttackEffect.Steps.Add(CheckNoMaterialStep);

FTCGEffectStep DestroyStep;
DestroyStep.StepType = ETCGEffectStepType::DestroyUnit;
DestroyStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
DestroyStep.bRequiresPreviousStepSuccess = true;
AttackEffect.Steps.Add(DestroyStep);

AttackerDefinition->Effects.Add(AttackEffect);
GameState->DebugCardDefinitions.Add(AttackerDefinition);

FTCGCardInstance* AttackerCard = GameState->AddCardInstanceFromDefinition(
AttackerDefinition,
0,
ETCGCardLocation::Board);

if (!AttackerCard) return;

AttackerCard->ZoneId = ATCG_GameState::GetFieldZoneId(0, 0);
AttackerCard->StackId = FGuid::NewGuid();
AttackerCard->StackIndex = 0;

const FGuid AttackerId = AttackerCard->CardInstanceId;

const FGuid DefenderId = AddAttackNoMaterialDebugBoardUnit(
GameState,
"Debug_Attack_Destroy_NoMaterial_Defender",
ETCGCardElement::Water,
10,
1,
0);

const bool bResolvedBattle = GameState->ResolveBattlePhase();

const FTCGCardInstance* AttackerAfter = GameState->FindCardInstance(AttackerId);
const FTCGCardInstance* DefenderAfter = GameState->FindCardInstance(DefenderId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: AttackDestroyNoMaterialUnit summary BattleResolved=%s AttackerStillBoard=%s DefenderGraveyard=%s ExpectedDefenderGraveyard=true ExpectedAttackerStillBoard=true"),
bResolvedBattle ? TEXT("true") : TEXT("false"),
(AttackerAfter && AttackerAfter->Location == ETCGCardLocation::Board) ? TEXT("true") : TEXT("false"),
(DefenderAfter && DefenderAfter->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"));

GameState->EndMatch(ETCGMatchResult::Draw);
}
