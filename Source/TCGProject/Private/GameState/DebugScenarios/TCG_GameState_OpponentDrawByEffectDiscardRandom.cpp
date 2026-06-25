#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
static FGuid AddOpponentDrawEffectDebugBoardUnit(
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

void RunDebugOpponentDrawByEffectDiscardRandomScenario(ATCG_GameState* GameState)
{
if (!GameState) return;

UE_LOG(LogTemp, Warning, TEXT("TCG Debug: OpponentDrawByEffectDiscardRandom scenario start"));

GameState->MatchCards.Empty();
GameState->SetMatchResult(ETCGMatchResult::None);
GameState->SetPhase(ETCGMatchPhase::Battle);
GameState->RoundNumber = 1;
GameState->TurnNumber = 1;

UTCG_CardDefinition* DrawSourceDefinition = NewObject<UTCG_CardDefinition>(GameState);
DrawSourceDefinition->CardDefinitionId = "Debug_P1_Draw_By_Effect_Source";
DrawSourceDefinition->DisplayName = FText::FromString(TEXT("Debug P1 Draw By Effect Source"));
DrawSourceDefinition->Element = ETCGCardElement::Water;
DrawSourceDefinition->BaseAttack = 1;
DrawSourceDefinition->Description = FText::FromString(TEXT("Debug: draw 1 by card effect."));

FTCGCardEffectRef DrawEffect;
DrawEffect.Trigger = ETCGEffectTrigger::OnPlay;

FTCGEffectStep DrawStep;
DrawStep.StepType = ETCGEffectStepType::DrawCards;
DrawStep.Value = 1;
DrawEffect.Steps.Add(DrawStep);

DrawSourceDefinition->Effects.Add(DrawEffect);
GameState->DebugCardDefinitions.Add(DrawSourceDefinition);

FTCGCardInstance* DrawSourceCard = GameState->AddCardInstanceFromDefinition(
DrawSourceDefinition,
1,
ETCGCardLocation::Board);

if (!DrawSourceCard) return;

	const FGuid DrawSourceCardId = DrawSourceCard->CardInstanceId;

DrawSourceCard->ZoneId = ATCG_GameState::GetFieldZoneId(1, 0);
DrawSourceCard->StackId = FGuid::NewGuid();
DrawSourceCard->StackIndex = 0;

UTCG_CardDefinition* ResponseDefinition = NewObject<UTCG_CardDefinition>(GameState);
ResponseDefinition->CardDefinitionId = "Debug_P0_Opponent_Draw_Response";
ResponseDefinition->DisplayName = FText::FromString(TEXT("Debug P0 Opponent Draw Response"));
ResponseDefinition->Element = ETCGCardElement::Dark;
ResponseDefinition->BaseAttack = 1;
ResponseDefinition->Description = FText::FromString(TEXT("If your opponent draws by card effect, discard this card; opponent discards 2 random cards."));

FTCGCardEffectRef ResponseEffect;
ResponseEffect.Trigger = ETCGEffectTrigger::OnOpponentDrawsByCardEffect;
ResponseEffect.bOptional = true;

FTCGEffectStep DiscardSourceStep;
DiscardSourceStep.StepType = ETCGEffectStepType::DiscardSource;
DiscardSourceStep.TargetMode = ETCGEffectTargetMode::SourceCard;
ResponseEffect.Steps.Add(DiscardSourceStep);

FTCGEffectStep RandomDiscardStep;
RandomDiscardStep.StepType = ETCGEffectStepType::DiscardRandomCards;
RandomDiscardStep.TargetMode = ETCGEffectTargetMode::Opponent;
RandomDiscardStep.Value = 2;
RandomDiscardStep.bRequiresPreviousStepSuccess = true;
ResponseEffect.Steps.Add(RandomDiscardStep);

ResponseDefinition->Effects.Add(ResponseEffect);
GameState->DebugCardDefinitions.Add(ResponseDefinition);

FTCGCardInstance* ResponseCard = GameState->AddCardInstanceFromDefinition(
ResponseDefinition,
0,
ETCGCardLocation::Hand);

if (!ResponseCard) return;

	const FGuid ResponseCardId = ResponseCard->CardInstanceId;

// P1 has 3 cards in hand before the effect draw, then draws 1, then should randomly discard 2.
for (int32 Index = 0; Index < 3; ++Index)
{
GameState->AddCardInstance(
FName(*FString::Printf(TEXT("Debug_P1_Hand_%d"), Index)),
ETCGCardElement::Fire,
1,
1,
ETCGCardLocation::Hand);
}

GameState->AddCardInstance(
"Debug_P1_Deck_Draw_Target",
ETCGCardElement::Earth,
1,
1,
ETCGCardLocation::Deck);

TArray<FTCGCardInstance> Player1HandBefore;
GameState->GetCardsInHand(1, Player1HandBefore);

TArray<FTCGEffectChainEntry> Chain;
GameState->AddCardEffectRefToChain(
Chain,
DrawSourceCardId,
DrawSourceCardId,
DrawEffect);

const bool bResolved = GameState->ResolveEffectChain(Chain);

TArray<FTCGCardInstance> Player1HandAfter;
GameState->GetCardsInHand(1, Player1HandAfter);

const FTCGCardInstance* ResponseAfter = GameState->FindCardInstance(ResponseCardId);

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: OpponentDrawByEffectDiscardRandom summary Resolved=%s ResponseDiscarded=%s Player1HandBefore=%d Player1HandAfter=%d ExpectedAfter=2"),
bResolved ? TEXT("true") : TEXT("false"),
(ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Graveyard) ? TEXT("true") : TEXT("false"),
Player1HandBefore.Num(),
Player1HandAfter.Num());

GameState->EndMatch(ETCGMatchResult::Draw);
}
