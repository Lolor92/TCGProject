#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static UTCG_CardDefinition* MakeDebugDefinition(
ATCG_GameState* GameState,
FName Id,
ETCGCardElement Element,
int32 Attack)
{
UTCG_CardDefinition* Definition = NewObject<UTCG_CardDefinition>(GameState);
Definition->CardDefinitionId = Id;
Definition->DisplayName = FText::FromName(Id);
Definition->Element = Element;
Definition->BaseAttack = Attack;
GameState->DebugCardDefinitions.Add(Definition);
return Definition;
}

static FGuid AddBoardUnit(
ATCG_GameState* GameState,
const UTCG_CardDefinition* Definition,
int32 PlayerIndex,
int32 FieldIndex)
{
FTCGCardInstance* Card = GameState->AddCardInstanceFromDefinition(
Definition,
PlayerIndex,
ETCGCardLocation::Board);

if (!Card) return FGuid();

Card->ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
Card->StackId = FGuid::NewGuid();
Card->StackIndex = 0;
return Card->CardInstanceId;
}
}

void RunDebugOverdrivePilotRyuScenario(ATCG_GameState* GameState)
{
if (!GameState) return;

UE_LOG(LogTemp, Warning, TEXT("TCG Debug: OverdrivePilotRyu scenario start"));

GameState->MatchCards.Empty();
GameState->DebugCardDefinitions.Empty();
GameState->SetMatchResult(ETCGMatchResult::None);
GameState->SetPhase(ETCGMatchPhase::Battle);
GameState->RoundNumber = 1;
GameState->TurnNumber = 1;

UTCG_CardDefinition* MachineUnit = MakeDebugDefinition(
GameState,
"Debug_Machine_Strider",
ETCGCardElement::Fire,
2);

UTCG_CardDefinition* EarthHandUnit = MakeDebugDefinition(
GameState,
"Debug_Earth_Hand_Unit",
ETCGCardElement::Earth,
1);

UTCG_CardDefinition* LaterMachineUnit = MakeDebugDefinition(
GameState,
"Debug_Machine_Later_Play",
ETCGCardElement::Earth,
3);

UTCG_CardDefinition* DeckCard = MakeDebugDefinition(
GameState,
"Debug_Ryu_Draw_Target",
ETCGCardElement::Light,
1);

UTCG_CardDefinition* Ryu = MakeDebugDefinition(
GameState,
"Overdrive Pilot - Ryu Blazeheart",
ETCGCardElement::Fire,
1);

FTCGCardEffectRef RyuOnPlay;
RyuOnPlay.Trigger = ETCGEffectTrigger::OnPlay;
RyuOnPlay.bOptional = true;

FTCGEffectStep AttachStep;
AttachStep.StepType = ETCGEffectStepType::AttachSourceToUnitMaterial;
AttachStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
AttachStep.TargetFilter.RequiredLocation = ETCGCardLocation::Board;
AttachStep.TargetFilter.bRequireTopCard = true;
AttachStep.TargetFilter.NameContains = "Machine";
RyuOnPlay.Steps.Add(AttachStep);

FTCGEffectStep PlayEarthStep;
PlayEarthStep.StepType = ETCGEffectStepType::PlayHandCardToEmptyZone;
PlayEarthStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
PlayEarthStep.TargetFilter.RequiredLocation = ETCGCardLocation::Hand;
PlayEarthStep.TargetFilter.bRequireElement = true;
PlayEarthStep.TargetFilter.RequiredElement = ETCGCardElement::Earth;
PlayEarthStep.bRequiresPreviousStepSuccess = true;
RyuOnPlay.Steps.Add(PlayEarthStep);

Ryu->Effects.Add(RyuOnPlay);

FTCGCardEffectRef RyuMachinePlayed;
RyuMachinePlayed.Trigger = ETCGEffectTrigger::OnYourUnitPlayed;
RyuMachinePlayed.TriggerFilter.OwnerMode = ETCGEffectTargetMode::Controller;
RyuMachinePlayed.TriggerFilter.RequiredLocation = ETCGCardLocation::Board;
RyuMachinePlayed.TriggerFilter.bRequireTopCard = true;
RyuMachinePlayed.TriggerFilter.NameContains = "Machine";

FTCGEffectStep DrawStep;
DrawStep.StepType = ETCGEffectStepType::DrawCards;
DrawStep.Value = 1;
RyuMachinePlayed.Steps.Add(DrawStep);

Ryu->Effects.Add(RyuMachinePlayed);

const FGuid MachineId = AddBoardUnit(GameState, MachineUnit, 0, 0);

FTCGCardInstance* RyuCard = GameState->AddCardInstanceFromDefinition(
Ryu,
0,
ETCGCardLocation::Hand);

FTCGCardInstance* EarthCard = GameState->AddCardInstanceFromDefinition(
EarthHandUnit,
0,
ETCGCardLocation::Hand);

FTCGCardInstance* LaterMachineCard = GameState->AddCardInstanceFromDefinition(
LaterMachineUnit,
0,
ETCGCardLocation::Hand);

GameState->AddCardInstanceFromDefinition(
DeckCard,
0,
ETCGCardLocation::Deck);

if (!RyuCard || !EarthCard || !LaterMachineCard) return;

const FGuid RyuId = RyuCard->CardInstanceId;
const FGuid EarthId = EarthCard->CardInstanceId;
const FGuid LaterMachineId = LaterMachineCard->CardInstanceId;

GameState->PlayCardToZone(RyuId, ATCG_GameState::GetFieldZoneId(0, 1));

const FTCGCardInstance* RyuAfter = GameState->FindCardInstance(RyuId);
const FTCGCardInstance* MachineAfter = GameState->FindCardInstance(MachineId);
const FTCGCardInstance* EarthAfter = GameState->FindCardInstance(EarthId);

const bool bRyuAttached =
RyuAfter
&& MachineAfter
&& RyuAfter->Location == ETCGCardLocation::Board
&& RyuAfter->StackId == MachineAfter->StackId
&& RyuAfter->CardInstanceId != MachineAfter->CardInstanceId;

const bool bEarthPlayed =
EarthAfter
&& EarthAfter->Location == ETCGCardLocation::Board;

TArray<FTCGCardInstance> HandBeforeMachineCards;
	GameState->GetCardsInHand(0, HandBeforeMachineCards);
	const int32 HandBeforeMachinePlay = HandBeforeMachineCards.Num();
GameState->PlayCardToZone(LaterMachineId, ATCG_GameState::GetFieldZoneId(0, 2));
TArray<FTCGCardInstance> HandAfterMachineCards;
	GameState->GetCardsInHand(0, HandAfterMachineCards);
	const int32 HandAfterMachinePlay = HandAfterMachineCards.Num();

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: OverdrivePilotRyu summary RyuAttached=%s EarthPlayed=%s HandBeforeMachine=%d HandAfterMachine=%d ExpectedDraw=true"),
bRyuAttached ? TEXT("true") : TEXT("false"),
bEarthPlayed ? TEXT("true") : TEXT("false"),
HandBeforeMachinePlay,
HandAfterMachinePlay);

GameState->EndMatch(ETCGMatchResult::Draw);
}
