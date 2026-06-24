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

	FTCGEffectStep ResponseStep;
	ResponseStep.StepType = ETCGEffectStepType::BanishSourceReturnTwoGraveyardCardsToBottomDeckBothDraw;
	ResponseEffect.Steps.Add(ResponseStep);

	ResponseDefinition->Effects.Add(ResponseEffect);
	DebugCardDefinitions.Add(ResponseDefinition);

	const FTCGCardInstance* ResponseCard = AddCardInstanceFromDefinition(ResponseDefinition, 0, ETCGCardLocation::Graveyard);
	const FGuid ResponseId = ResponseCard ? ResponseCard->CardInstanceId : FGuid();

	const FGuid RecycleAId = AddCardInstance("Debug_GraveyardRecycle_ReturnA", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;
	const FGuid RecycleBId = AddCardInstance("Debug_GraveyardRecycle_ReturnB", ETCGCardElement::Wind, 1, 0, ETCGCardLocation::Graveyard).CardInstanceId;

	const FGuid Player0DrawId = AddCardInstance("Debug_GraveyardRecycle_P0Draw", ETCGCardElement::Dark, 1, 0, ETCGCardLocation::Deck).CardInstanceId;
	const FGuid Player1DrawId = AddCardInstance("Debug_GraveyardRecycle_P1Draw", ETCGCardElement::Light, 1, 1, ETCGCardLocation::Deck).CardInstanceId;

	const FName VictimZoneId = GetFieldZoneId(0, 0);
	const FGuid VictimId = AddCardInstance("Debug_GraveyardRecycle_Victim", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* Victim = FindCardInstance(VictimId))
	{
		Victim->ZoneId = VictimZoneId;
		Victim->StackId = FGuid::NewGuid();
		Victim->StackIndex = 0;
	}

	const FName DestroyerZoneId = GetFieldZoneId(1, 0);
	const FGuid DestroyerId = AddCardInstance("Debug_GraveyardRecycle_Destroyer", ETCGCardElement::Fire, 3, 1, ETCGCardLocation::Board).CardInstanceId;
	if (FTCGCardInstance* Destroyer = FindCardInstance(DestroyerId))
	{
		Destroyer->ZoneId = DestroyerZoneId;
		Destroyer->StackId = FGuid::NewGuid();
		Destroyer->StackIndex = 0;
	}

	const bool bBattle = ResolveBattleBetweenZones(VictimZoneId, DestroyerZoneId);

	const FTCGCardInstance* ResponseAfter = FindCardInstance(ResponseId);
	const FTCGCardInstance* RecycleAAfter = FindCardInstance(RecycleAId);
	const FTCGCardInstance* RecycleBAfter = FindCardInstance(RecycleBId);
	const FTCGCardInstance* Player0DrawAfter = FindCardInstance(Player0DrawId);
	const FTCGCardInstance* Player1DrawAfter = FindCardInstance(Player1DrawId);
	const FTCGCardInstance* VictimAfter = FindCardInstance(VictimId);
	const FTCGCardInstance* DestroyerAfter = FindCardInstance(DestroyerId);

	const bool bResponseBanished = ResponseAfter && ResponseAfter->Location == ETCGCardLocation::Banish;
	const bool bReturnedA = RecycleAAfter && RecycleAAfter->Location == ETCGCardLocation::Deck;
	const bool bReturnedB = RecycleBAfter && RecycleBAfter->Location == ETCGCardLocation::Deck;
	const bool bPlayer0Drawn = Player0DrawAfter && Player0DrawAfter->Location == ETCGCardLocation::Hand;
	const bool bPlayer1Drawn = Player1DrawAfter && Player1DrawAfter->Location == ETCGCardLocation::Hand;
	const bool bVictimGraveyard = VictimAfter && VictimAfter->Location == ETCGCardLocation::Graveyard;
	const bool bDestroyerStillBoard = DestroyerAfter && DestroyerAfter->Location == ETCGCardLocation::Board;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: GraveyardRecycleBothDraw summary Battle=%s ResponseBanished=%s ReturnedToDeck=%d Player0Drawn=%s Player1Drawn=%s VictimGraveyard=%s DestroyerStillBoard=%s"),
		bBattle ? TEXT("true") : TEXT("false"),
		bResponseBanished ? TEXT("true") : TEXT("false"),
		(bReturnedA ? 1 : 0) + (bReturnedB ? 1 : 0),
		bPlayer0Drawn ? TEXT("true") : TEXT("false"),
		bPlayer1Drawn ? TEXT("true") : TEXT("false"),
		bVictimGraveyard ? TEXT("true") : TEXT("false"),
		bDestroyerStillBoard ? TEXT("true") : TEXT("false"));

	EndMatch(ETCGMatchResult::Draw);
}
