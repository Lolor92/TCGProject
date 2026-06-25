#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static FGuid AddDebugBoardUnitWithMaterials(
ATCG_GameState* GameState,
FName TopCardDefinitionId,
ETCGCardElement Element,
int32 Attack,
int32 PlayerIndex,
int32 FieldIndex,
int32 MaterialCount)
{
if (!GameState)
{
return FGuid();
}

FTCGCardInstance& TopCard = GameState->AddCardInstance(
TopCardDefinitionId,
Element,
Attack,
PlayerIndex,
ETCGCardLocation::Board);

TopCard.ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
TopCard.StackId = FGuid::NewGuid();
TopCard.StackIndex = MaterialCount;

for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
{
FTCGCardInstance& MaterialCard = GameState->AddCardInstance(
FName(*FString::Printf(TEXT("%s_Material_%d"), *TopCardDefinitionId.ToString(), MaterialIndex)),
Element,
1,
PlayerIndex,
ETCGCardLocation::Board);

MaterialCard.ZoneId = TopCard.ZoneId;
MaterialCard.StackId = TopCard.StackId;
MaterialCard.StackIndex = MaterialIndex;
}

return TopCard.CardInstanceId;
}

static int32 CountMaterialsUnderUnit_AttackDetachSteal(ATCG_GameState* GameState, const FGuid& TopCardInstanceId)
{
if (!GameState)
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
if (Card.Location != ETCGCardLocation::Board) continue;
if (Card.StackId != TopCard->StackId) continue;
if (Card.CardInstanceId == TopCard->CardInstanceId) continue;
if (Card.StackIndex >= TopCard->StackIndex) continue;

Count++;
}

return Count;
}

static UTCG_CardDefinition* CreateAttackStealDefinition(ATCG_GameState* GameState)
{
UTCG_CardDefinition* CardDefinition = NewObject<UTCG_CardDefinition>(GameState);
CardDefinition->CardDefinitionId = "Debug_AttackDetachSteal_Attacker";
CardDefinition->DisplayName = FText::FromString(TEXT("Debug Attack Detach Steal"));
CardDefinition->Element = ETCGCardElement::Dark;
CardDefinition->BaseAttack = 5;
CardDefinition->Description = FText::FromString(TEXT("When this card attacks a Unit with material, detach 2 material from this Unit and if you do, take one material from the attacked Unit and attach it to this one."));

FTCGCardEffectRef EffectRef;
EffectRef.Trigger = ETCGEffectTrigger::OnAttack;

FTCGEffectStep Step;
Step.StepType = ETCGEffectStepType::AttackDetachTwoStealOneMaterial;
EffectRef.Steps.Add(Step);

CardDefinition->Effects.Add(EffectRef);
return CardDefinition;
}

static UTCG_CardDefinition* CreateMaterialLossSaveDefinition(ATCG_GameState* GameState)
{
UTCG_CardDefinition* CardDefinition = NewObject<UTCG_CardDefinition>(GameState);
CardDefinition->CardDefinitionId = "Debug_AttackDetachSteal_Save";
CardDefinition->DisplayName = FText::FromString(TEXT("Debug Material Loss Save"));
CardDefinition->Element = ETCGCardElement::Wind;
CardDefinition->BaseAttack = 1;
CardDefinition->Description = FText::FromString(TEXT("If one of your Units would lose material(s) by a card effect, you may discard this card instead."));

FTCGCardEffectRef EffectRef;
EffectRef.Trigger = ETCGEffectTrigger::OnYourUnitWouldLoseMaterialByCardEffect;
EffectRef.bOptional = true;

FTCGEffectStep Step;
Step.StepType = ETCGEffectStepType::DiscardSourcePreventMaterialLossByCardEffect;
EffectRef.Steps.Add(Step);

CardDefinition->Effects.Add(EffectRef);
return CardDefinition;
}
}

void ATCG_GameState::RunDebugAttackDetachStealMaterialScenario()
{
UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackDetachStealMaterial scenario start"));

MatchCards.Empty();
SetMatchResult(ETCGMatchResult::None);
SetPhase(ETCGMatchPhase::Battle);
RoundNumber = 1;
TurnNumber = 1;

UTCG_CardDefinition* AttackStealDefinition = CreateAttackStealDefinition(this);
UTCG_CardDefinition* SaveDefinition = CreateMaterialLossSaveDefinition(this);

DebugCardDefinitions.Add(AttackStealDefinition);
DebugCardDefinitions.Add(SaveDefinition);

const FGuid AttackerId = AddDebugBoardUnitWithMaterials(
this,
"AttackDetachSteal_P0_Attacker",
ETCGCardElement::Dark,
5,
0,
0,
3);

const FGuid DefenderId = AddDebugBoardUnitWithMaterials(
this,
"AttackDetachSteal_P1_Defender",
ETCGCardElement::Water,
3,
1,
0,
2);

FTCGCardInstance* SaveCard = AddCardInstanceFromDefinition(
SaveDefinition,
1,
ETCGCardLocation::Hand);

const int32 AttackerMaterialsBefore = CountMaterialsUnderUnit_AttackDetachSteal(this, AttackerId);
const int32 DefenderMaterialsBefore = CountMaterialsUnderUnit_AttackDetachSteal(this, DefenderId);

TArray<FTCGEffectChainEntry> Chain;
bool bChainAdded = false;

for (const FTCGCardEffectRef& EffectRef : AttackStealDefinition->Effects)
{
if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnAttack))
{
bChainAdded = AddCardEffectRefToChain(
Chain,
AttackerId,
DefenderId,
EffectRef);

break;
}
}

const bool bChainResolved = ResolveEffectChain(Chain);

const FTCGCardInstance* SaveCardAfter = SaveCard ? FindCardInstance(SaveCard->CardInstanceId) : nullptr;
const int32 AttackerMaterialsAfter = CountMaterialsUnderUnit_AttackDetachSteal(this, AttackerId);
const int32 DefenderMaterialsAfter = CountMaterialsUnderUnit_AttackDetachSteal(this, DefenderId);

const bool bSaveDiscarded =
SaveCardAfter
&& SaveCardAfter->Location == ETCGCardLocation::Graveyard;

const bool bAttackerPaidTwoMaterials =
AttackerMaterialsBefore == 3
&& AttackerMaterialsAfter == 1;

const bool bDefenderMaterialLossPrevented =
DefenderMaterialsBefore == 2
&& DefenderMaterialsAfter == 2
&& bSaveDiscarded;

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: AttackDetachStealMaterial summary ChainAdded=%s ChainResolved=%s SaveDiscarded=%s AttackerMaterials=%d/%d DefenderMaterials=%d/%d AttackerPaidTwo=%s DefenderMaterialLossPrevented=%s"),
bChainAdded ? TEXT("true") : TEXT("false"),
bChainResolved ? TEXT("true") : TEXT("false"),
bSaveDiscarded ? TEXT("true") : TEXT("false"),
AttackerMaterialsBefore,
AttackerMaterialsAfter,
DefenderMaterialsBefore,
DefenderMaterialsAfter,
bAttackerPaidTwoMaterials ? TEXT("true") : TEXT("false"),
bDefenderMaterialLossPrevented ? TEXT("true") : TEXT("false"));

EndMatch(ETCGMatchResult::Draw);
}
