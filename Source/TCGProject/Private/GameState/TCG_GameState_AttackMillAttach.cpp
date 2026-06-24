#include "GameState/TCG_GameState.h"

namespace
{
	const FName DebugCard_AttackMillAttach = "Debug_Water_AttackMillAttach";

	FTCGCardEffectRef MakeAttackMillAttachEffect()
	{
		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnAttack;
		EffectRef.bOptional = true;

		FTCGEffectStep MillStep;
		MillStep.StepType = ETCGEffectStepType::SendTopDeckCardToGraveyard;
		MillStep.TargetMode = ETCGEffectTargetMode::Controller;
		EffectRef.Steps.Add(MillStep);

		FTCGEffectStep ThenStep;
		ThenStep.StepType = ETCGEffectStepType::Then;
		EffectRef.Steps.Add(ThenStep);

		FTCGEffectStep AttachStep;
		AttachStep.StepType = ETCGEffectStepType::AttachGraveyardCardToSourceMaterial;
		AttachStep.TargetMode = ETCGEffectTargetMode::SourceCard;
		AttachStep.SelectionMode = ETCGEffectSelectionMode::PlayerChoice;
		AttachStep.bRequiresPreviousStepSuccess = true;
		AttachStep.TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
		AttachStep.TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
		AttachStep.TargetFilter.bRequireTopCard = false;
		AttachStep.TargetFilter.bRequireElement = true;
		AttachStep.TargetFilter.RequiredElement = ETCGCardElement::Water;
		EffectRef.Steps.Add(AttachStep);

		return EffectRef;
	}
}

void ATCG_GameState::RunDebugAttackMillAttachScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackMillAttach scenario start"));
	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Battle);
	SetCurrentTurnPlayer(0);

	FTCGCardInstance& Attacker = AddDebugCardInstance(DebugCard_AttackMillAttach, ETCGCardElement::Water, 3, 0, ETCGCardLocation::Board);
	Attacker.ZoneId = GetFieldZoneId(0, 0);
	Attacker.StackId = FGuid::NewGuid();
	Attacker.StackIndex = 0;
	const FGuid AttackerId = Attacker.CardInstanceId;
	const FGuid AttackerStackId = Attacker.StackId;

	AddCardInstance("Debug_Water_Grave_Attach_A", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Graveyard);
	AddCardInstance("Debug_Fire_Grave_NotEligible", ETCGCardElement::Fire, 1, 0, ETCGCardLocation::Graveyard);
	AddCardInstance("Debug_Water_Milled_Attach_B", ETCGCardElement::Water, 1, 0, ETCGCardLocation::Deck);

	TArray<FTCGCardInstance> StackBefore;
	TArray<FTCGCardInstance> DeckBefore;
	TArray<FTCGCardInstance> GraveyardBefore;
	GetCardsInStack(AttackerStackId, StackBefore);
	GetCardsInDeck(0, DeckBefore);
	GetCardsInLocation(0, ETCGCardLocation::Graveyard, GraveyardBefore);

	ExecuteCardTrigger(AttackerId, ETCGEffectTrigger::OnAttack);
	TArray<FTCGEffectChainEntry> Chain;
	TArray<FTCGCardEffectRef> EffectRefs;
	const FTCGCardInstance* AttackerAfterSetup = FindCardInstance(AttackerId);
	if (AttackerAfterSetup)
	{
		GetPrintedEffectRefsForCard(*AttackerAfterSetup, EffectRefs);
		for (const FTCGCardEffectRef& EffectRef : EffectRefs)
		{
			if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnAttack)) AddCardEffectRefToChain(Chain, AttackerId, AttackerId, EffectRef);
		}
	}
	if (Chain.Num() <= 0) AddCardEffectRefToChain(Chain, AttackerId, AttackerId, MakeAttackMillAttachEffect());
	const bool bResolved = ResolveEffectChain(Chain);

	TArray<FTCGCardInstance> StackAfter;
	TArray<FTCGCardInstance> DeckAfter;
	TArray<FTCGCardInstance> GraveyardAfter;
	GetCardsInStack(AttackerStackId, StackAfter);
	GetCardsInDeck(0, DeckAfter);
	GetCardsInLocation(0, ETCGCardLocation::Graveyard, GraveyardAfter);

	const int32 WaterGraveAfter = CountCardsInLocationByElement(0, ETCGCardLocation::Graveyard, ETCGCardElement::Water);
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: AttackMillAttach summary Resolved=%s StackBefore=%d StackAfter=%d DeckBefore=%d DeckAfter=%d GraveBefore=%d GraveAfter=%d WaterGYAfter=%d"),
		bResolved ? TEXT("true") : TEXT("false"),
		StackBefore.Num(),
		StackAfter.Num(),
		DeckBefore.Num(),
		DeckAfter.Num(),
		GraveyardBefore.Num(),
		GraveyardAfter.Num(),
		WaterGraveAfter);

	EndMatch(ETCGMatchResult::Draw);
}
