#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static FGuid AddDebugBoardUnit(
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

static int32 CountOpponentEffectResponseMaterialsUnderUnit(ATCG_GameState* GameState, const FGuid& TopCardId)
{
const FTCGCardInstance* TopCard = GameState->FindCardInstance(TopCardId);
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
}

void RunDebugOpponentEffectDestructionGraveyardResponseScenario(ATCG_GameState* GameState)
{
if (!GameState) return;

UE_LOG(LogTemp, Warning, TEXT("TCG Debug: OpponentEffectDestructionGraveyardResponse scenario start"));

GameState->MatchCards.Empty();
GameState->SetMatchResult(ETCGMatchResult::None);
GameState->SetPhase(ETCGMatchPhase::Battle);
GameState->RoundNumber = 1;
GameState->TurnNumber = 1;

const FGuid VictimId = AddDebugBoardUnit(
GameState,
"OpponentEffect_P0_Victim",
ETCGCardElement::Earth,
1,
0,
0);

const FGuid DestroyerId = AddDebugBoardUnit(
GameState,
"OpponentEffect_P1_Destroyer",
ETCGCardElement::Dark,
10,
1,
0);

FTCGCardInstance* DestroyerCard = GameState->FindCardInstance(DestroyerId);
if (DestroyerCard)
{
DestroyerCard->StackIndex = 2;

for (int32 MaterialIndex = 0; MaterialIndex < 2; ++MaterialIndex)
{
FTCGCardInstance& MaterialCard = GameState->AddCardInstance(
FName(*FString::Printf(TEXT("OpponentEffect_P1_Material_%d"), MaterialIndex)),
ETCGCardElement::Water,
1,
1,
ETCGCardLocation::Board);

MaterialCard.ZoneId = DestroyerCard->ZoneId;
MaterialCard.StackId = DestroyerCard->StackId;
MaterialCard.StackIndex = MaterialIndex;
}
}

UTCG_CardDefinition* ResponseCardDefinition = NewObject<UTCG_CardDefinition>(GameState);
ResponseCardDefinition->CardDefinitionId = "Debug_Graveyard_OpponentEffectDestroy_Response";
ResponseCardDefinition->DisplayName = FText::FromString(TEXT("Debug Graveyard Opponent Effect Destroy Response"));
ResponseCardDefinition->Element = ETCGCardElement::Water;
ResponseCardDefinition->BaseAttack = 1;
ResponseCardDefinition->Description = FText::FromString(TEXT("If your unit is destroyed by an opponent effect, banish this card from your Graveyard; detach 1 material from the destroying unit."));

FTCGCardEffectRef ResponseEffect;
ResponseEffect.Trigger = ETCGEffectTrigger::OnYourUnitDestroyedByOpponentCardEffect;
ResponseEffect.bOptional = true;

FTCGEffectStep BanishStep;
BanishStep.StepType = ETCGEffectStepType::BanishSource;
BanishStep.TargetMode = ETCGEffectTargetMode::SourceCard;
ResponseEffect.Steps.Add(BanishStep);

FTCGEffectStep DetachStep;
DetachStep.StepType = ETCGEffectStepType::DetachMaterials;
DetachStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
DetachStep.Value = 1;
DetachStep.bRequiresPreviousStepSuccess = true;
ResponseEffect.Steps.Add(DetachStep);

ResponseCardDefinition->Effects.Add(ResponseEffect);
GameState->DebugCardDefinitions.Add(ResponseCardDefinition);

FTCGCardInstance* ResponseCard = GameState->AddCardInstanceFromDefinition(
ResponseCardDefinition,
0,
ETCGCardLocation::Graveyard);

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
DestroyChainEntry.SourceCardDefinitionId = "OpponentEffect_P1_Destroyer";
DestroyChainEntry.Trigger = ETCGEffectTrigger::OnPlay;
DestroyChainEntry.EffectRef = DestroyEffect;
DestroyChainEntry.ControllerPlayerIndex = 1;
DestroyChainEntry.bRequiresSourceOnBoard = true;
DestroyChainEntry.bRequiresTargetOnBoard = true;

TArray<FTCGEffectChainEntry> DestroyChain;
DestroyChain.Add(DestroyChainEntry);

const int32 MaterialsBefore = CountOpponentEffectResponseMaterialsUnderUnit(GameState, DestroyerId);
const bool bResolved = GameState->ResolveEffectChain(DestroyChain);

const FTCGCardInstance* VictimAfter = GameState->FindCardInstance(VictimId);
const FTCGCardInstance* ResponseAfter = ResponseCard ? GameState->FindCardInstance(ResponseCard->CardInstanceId) : nullptr;
const int32 MaterialsAfter = CountOpponentEffectResponseMaterialsUnderUnit(GameState, DestroyerId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: OpponentEffectDestructionGraveyardResponse summary Resolved=%s VictimGraveyard=%s ResponseBanished=%s MaterialsBefore=%d MaterialsAfter=%d Detached=%d ExpectedDetached=1"),
bResolved ? TEXT("true") : TEXT("false"),
(VictimAfter && VictimAfter->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"),
(ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Banish) ? TEXT("true") : TEXT("false"),
MaterialsBefore,
MaterialsAfter,
MaterialsBefore - MaterialsAfter);

GameState->EndMatch(ETCGMatchResult::Draw);
}
