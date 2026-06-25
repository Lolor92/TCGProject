#include "GameState/TCG_GameState.h"
#include "Cards/TCG_CardDefinition.h"

namespace
{
	static FGuid AddDebugBoardUnitWithMaterials(
		ATCG_GameState* GameState,
		FName TopCardDefinitionId,
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
			ETCGCardElement::Earth,
			3,
			PlayerIndex,
			ETCGCardLocation::Board);

		TopCard.ZoneId = ATCG_GameState::GetFieldZoneId(PlayerIndex, FieldIndex);
		TopCard.StackId = FGuid::NewGuid();
		TopCard.StackIndex = MaterialCount;

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			FTCGCardInstance& MaterialCard = GameState->AddCardInstance(
				FName(*FString::Printf(TEXT("%s_Material_%d"), *TopCardDefinitionId.ToString(), MaterialIndex)),
				ETCGCardElement::Water,
				1,
				PlayerIndex,
				ETCGCardLocation::Board);

			MaterialCard.ZoneId = TopCard.ZoneId;
			MaterialCard.StackId = TopCard.StackId;
			MaterialCard.StackIndex = MaterialIndex;
		}

		return TopCard.CardInstanceId;
	}

	static int32 CountMaterialsUnderUnit(ATCG_GameState* GameState, const FGuid& TopCardInstanceId)
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

	static UTCG_CardDefinition* CreateMaterialLossSaveCardDefinition(ATCG_GameState* GameState)
	{
		if (!GameState)
		{
			return nullptr;
		}

		UTCG_CardDefinition* CardDefinition = NewObject<UTCG_CardDefinition>(GameState);
		CardDefinition->CardDefinitionId = "Debug_MaterialLoss_Save";
		CardDefinition->DisplayName = FText::FromString(TEXT("Debug Material Loss Save"));
		CardDefinition->Element = ETCGCardElement::Wind;
		CardDefinition->BaseAttack = 1;
		CardDefinition->Description = FText::FromString(TEXT("If one of your Units would lose material(s) by a card effect, you may discard this card instead."));

		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnYourUnitWouldLoseMaterialByCardEffect;
		EffectRef.bOptional = true;

		FTCGEffectStep EffectStep;
		EffectStep.StepType = ETCGEffectStepType::DiscardSourcePreventMaterialLossByCardEffect;
		EffectRef.Steps.Add(EffectStep);

		CardDefinition->Effects.Add(EffectRef);
		return CardDefinition;
	}

	static UTCG_CardDefinition* CreateOpponentRemoveMaterialCardDefinition(ATCG_GameState* GameState)
	{
		if (!GameState)
		{
			return nullptr;
		}

		UTCG_CardDefinition* CardDefinition = NewObject<UTCG_CardDefinition>(GameState);
		CardDefinition->CardDefinitionId = "Debug_Remove_Material_Effect";
		CardDefinition->DisplayName = FText::FromString(TEXT("Debug Remove Material Effect"));
		CardDefinition->Element = ETCGCardElement::Dark;
		CardDefinition->BaseAttack = 2;
		CardDefinition->Description = FText::FromString(TEXT("Debug effect: remove the bottom material from target Unit."));

		FTCGCardEffectRef EffectRef;
		EffectRef.Trigger = ETCGEffectTrigger::OnPlay;

		FTCGEffectStep EffectStep;
		EffectStep.StepType = ETCGEffectStepType::MoveBottomOverlayToGraveyard;
		EffectStep.TargetMode = ETCGEffectTargetMode::TriggerTarget;
		EffectRef.Steps.Add(EffectStep);

		CardDefinition->Effects.Add(EffectRef);
		return CardDefinition;
	}
}

void ATCG_GameState::RunDebugMaterialLossSaveReplacementScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: MaterialLossSaveReplacement scenario start"));

	MatchCards.Empty();
	SetMatchResult(ETCGMatchResult::None);
	SetPhase(ETCGMatchPhase::Battle);
	RoundNumber = 1;
	TurnNumber = 1;

	UTCG_CardDefinition* SaveCardDefinition = CreateMaterialLossSaveCardDefinition(this);
	UTCG_CardDefinition* RemoveMaterialCardDefinition = CreateOpponentRemoveMaterialCardDefinition(this);

	DebugCardDefinitions.Add(SaveCardDefinition);
	DebugCardDefinitions.Add(RemoveMaterialCardDefinition);

	const FGuid ProtectedUnitId = AddDebugBoardUnitWithMaterials(
		this,
		"MaterialLossSave_P0_ProtectedUnit",
		0,
		0,
		2);

	FTCGCardInstance* SaveCard = AddCardInstanceFromDefinition(
		SaveCardDefinition,
		0,
		ETCGCardLocation::Hand);

	FTCGCardInstance* RemoveMaterialCard = AddCardInstanceFromDefinition(
		RemoveMaterialCardDefinition,
		1,
		ETCGCardLocation::Hand);

	const int32 MaterialsBefore = CountMaterialsUnderUnit(this, ProtectedUnitId);

	TArray<FTCGEffectChainEntry> Chain;
	bool bChainAdded = false;

	if (RemoveMaterialCard && RemoveMaterialCardDefinition)
	{
		for (const FTCGCardEffectRef& EffectRef : RemoveMaterialCardDefinition->Effects)
		{
			if (DoesCardEffectMatchTrigger(EffectRef, ETCGEffectTrigger::OnPlay))
			{
				bChainAdded = AddCardEffectRefToChain(
					Chain,
					RemoveMaterialCard->CardInstanceId,
					ProtectedUnitId,
					EffectRef);

				break;
			}
		}
	}

	const bool bChainResolved = ResolveEffectChain(Chain);

	const FTCGCardInstance* SaveCardAfter = SaveCard ? FindCardInstance(SaveCard->CardInstanceId) : nullptr;
	const int32 MaterialsAfter = CountMaterialsUnderUnit(this, ProtectedUnitId);

	const bool bResponseDiscarded =
		SaveCardAfter
		&& SaveCardAfter->Location == ETCGCardLocation::Graveyard;

	const bool bMaterialLossPrevented =
		MaterialsBefore == 2
		&& MaterialsAfter == 2;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: MaterialLossSaveReplacement summary ChainAdded=%s ChainResolved=%s ResponseDiscarded=%s MaterialsBefore=%d MaterialsAfter=%d MaterialLossPrevented=%s"),
		bChainAdded ? TEXT("true") : TEXT("false"),
		bChainResolved ? TEXT("true") : TEXT("false"),
		bResponseDiscarded ? TEXT("true") : TEXT("false"),
		MaterialsBefore,
		MaterialsAfter,
		bMaterialLossPrevented ? TEXT("true") : TEXT("false"));

	EndMatch(ETCGMatchResult::Draw);
}
