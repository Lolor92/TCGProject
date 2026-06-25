#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

void ATCG_GameState::RunDebugGraveyardRecycleBothDrawScenario()
{
UE_LOG(LogTemp, Warning, TEXT("TCG Debug: GraveyardRecycleBothDraw scenario start"));

MatchCards.Empty();
StartMatch();
SetPhase(ETCGMatchPhase::Battle);
SetMatchResult(ETCGMatchResult::None);

UTCG_CardDefinition* ResponseDefinition = NewObject<UTCG_CardDefinition>(this);
ResponseDefinition->CardDefinitionId = "Debug_GraveyardRecycle_Response";
ResponseDefinition->DisplayName = FText::FromString("Debug Graveyard Recycle Response");
ResponseDefinition->Element = ETCGCardElement::Light;
ResponseDefinition->BaseAttack = 1;

FTCGCardEffectRef ResponseEffect;
ResponseEffect.Trigger = ETCGEffectTrigger::OnYourUnitDestroyed;
ResponseEffect.bOptional = true;

FTCGEffectStep BanishStep;
BanishStep.StepType = ETCGEffectStepType::BanishSource;
ResponseEffect.Steps.Add(BanishStep);

FTCGEffectStep ReturnStep;
ReturnStep.StepType = ETCGEffectStepType::MoveGraveyardCardToBottomDeck;
ReturnStep.Value = 2;
ReturnStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
ReturnStep.bRequiresPreviousStepSuccess = true;
ResponseEffect.Steps.Add(ReturnStep);

FTCGEffectStep DrawBothStep;
DrawBothStep.StepType = ETCGEffectStepType::DrawCardsForBothPlayers;
DrawBothStep.Value = 1;
DrawBothStep.bRequiresPreviousStepSuccess = true;
ResponseEffect.Steps.Add(DrawBothStep);

ResponseDefinition->Effects.Add(ResponseEffect);
DebugCardDefinitions.Add(ResponseDefinition);

const FTCGCardInstance* ResponseCard = AddCardInstanceFromDefinition(ResponseDefinition, 0, ETCGCardLocation::Graveyard);
const FGuid ResponseId = ResponseCard ? ResponseCard->CardInstanceId : FGuid();

const FGuid RecycleAId = AddCardInstance("Debug_GraveyardRecycle_ReturnA", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
const FGuid RecycleBId = AddCardInstance("Debug_GraveyardRecycle_ReturnB", ETCGCardElement::Wind, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;

const FGuid Player0DrawId = AddCardInstance("Debug_GraveyardRecycle_P0Draw", ETCGCardElement::Dark, 1, 0, ETCGCardLocation::Deck).CardInstanceId;
const FGuid Player1DrawId = AddCardInstance("Debug_GraveyardRecycle_P1Draw", ETCGCardElement::Light, 1, 1, ETCGCardLocation::Deck).CardInstanceId;

FTCGCardInstance& DestroyedUnit = AddCardInstance("Debug_GraveyardRecycle_DestroyedUnit", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Graveyard);

TArray<FTCGEffectChainEntry> Chain;
const bool bChainAdded = AddCardEffectRefToChain(Chain, ResponseId, DestroyedUnit.CardInstanceId, ResponseEffect);
const bool bChainResolved = ResolveEffectChain(Chain);

const FTCGCardInstance* ResponseAfter = FindCardInstance(ResponseId);
const FTCGCardInstance* RecycleAAfter = FindCardInstance(RecycleAId);
const FTCGCardInstance* RecycleBAfter = FindCardInstance(RecycleBId);
const FTCGCardInstance* Player0DrawAfter = FindCardInstance(Player0DrawId);
const FTCGCardInstance* Player1DrawAfter = FindCardInstance(Player1DrawId);
const FTCGCardInstance* DestroyedAfter = FindCardInstance(DestroyedUnit.CardInstanceId);

const bool bResponseBanished = ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Banish;
const bool bReturnedA = RecycleAAfter && RecycleAAfter->Location == ETCGCardLocation::Deck;
const bool bReturnedB = RecycleBAfter && RecycleBAfter->Location == ETCGCardLocation::Deck;
const bool bPlayer0Drawn = Player0DrawAfter && Player0DrawAfter->Location == ETCGCardLocation::Hand;
const bool bPlayer1Drawn = Player1DrawAfter && Player1DrawAfter->Location == ETCGCardLocation::Hand;
const bool bDestroyedStillGraveyard = DestroyedAfter && DestroyedAfter->Location == ETCGCardLocation::Graveyard;

UE_LOG(LogTemp, Warning,
TEXT("TCG Debug: GraveyardRecycleBothDraw summary ChainAdded=%s ChainResolved=%s ResponseBanished=%s ReturnedToDeck=%d Player0Drawn=%s Player1Drawn=%s DestroyedStillGraveyard=%s"),
bChainAdded ? TEXT("true") : TEXT("false"),
bChainResolved ? TEXT("true") : TEXT("false"),
bResponseBanished ? TEXT("true") : TEXT("false"),
(bReturnedA ? 1 : 0) + (bReturnedB ? 1 : 0),
bPlayer0Drawn ? TEXT("true") : TEXT("false"),
bPlayer1Drawn ? TEXT("true") : TEXT("false"),
bDestroyedStillGraveyard ? TEXT("true") : TEXT("false"));

EndMatch(ETCGMatchResult::Draw);
}
