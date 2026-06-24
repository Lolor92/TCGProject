#include "GameState/TCG_GameState.h"

void ATCG_GameState::RunDebugGraveyardPlayChoiceScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: GraveyardPlayChoice scenario start"));

	MatchCards.Empty();
	StartMatch();
	SetPhase(ETCGMatchPhase::Placement);
	SetMatchResult(ETCGMatchResult::None);

	FTCGCardInstance& SourceCard = AddCardInstance(
		"Debug_Water_GraveyardPlay_Source",
		ETCGCardElement::Water,
		1,
		0,
		ETCGCardLocation::Graveyard);

	FTCGCardInstance& WaterPick = AddCardInstance(
		"Debug_Water_GraveyardPlay_Pick",
		ETCGCardElement::Water,
		3,
		0,
		ETCGCardLocation::Graveyard);

	FTCGCardInstance& EarthOther = AddCardInstance(
		"Debug_Earth_GraveyardPlay_Other",
		ETCGCardElement::Earth,
		2,
		0,
		ETCGCardLocation::Graveyard);

	FTCGCardInstance& OccupiedCard = AddCardInstance(
		"Debug_Fire_GraveyardPlay_OccupiedZone",
		ETCGCardElement::Fire,
		1,
		0,
		ETCGCardLocation::Board);
	OccupiedCard.ZoneId = GetFieldZoneId(0, 0);
	OccupiedCard.StackId = FGuid::NewGuid();
	OccupiedCard.StackIndex = 0;

	FTCGEffectTargetFilter TargetFilter;
	TargetFilter.OwnerMode = ETCGEffectTargetMode::Controller;
	TargetFilter.RequiredLocation = ETCGCardLocation::Graveyard;
	TargetFilter.bRequireElement = true;
	TargetFilter.RequiredElement = ETCGCardElement::Water;
	TargetFilter.bExcludeSourceCard = true;

	FTCGEffectChainEntry DebugChainEntry;
	DebugChainEntry.ChainIndex = 1;
	DebugChainEntry.SourceCardInstanceId = SourceCard.CardInstanceId;
	DebugChainEntry.TargetCardInstanceId = SourceCard.CardInstanceId;
	DebugChainEntry.SourceCardDefinitionId = SourceCard.CardDefinitionId;
	DebugChainEntry.Trigger = ETCGEffectTrigger::OnSentFromDeckToGraveyard;
	DebugChainEntry.ControllerPlayerIndex = 0;

	const bool bChoiceStarted = BeginPendingPlayGraveyardCardToEmptyZoneChoice(
		0,
		TargetFilter,
		DebugChainEntry);

	TArray<FGuid> CardOptions;
	TArray<FName> ZoneOptions;
	GetPendingPlayGraveyardCardToEmptyZoneCardOptions(CardOptions);
	GetPendingPlayGraveyardCardToEmptyZoneZoneOptions(ZoneOptions);

	const bool bSourceExcluded = !CardOptions.Contains(SourceCard.CardInstanceId);
	const bool bWaterEligible = CardOptions.Contains(WaterPick.CardInstanceId);
	const bool bEarthExcluded = !CardOptions.Contains(EarthOther.CardInstanceId);
	const bool bOccupiedZoneExcluded = !ZoneOptions.Contains(GetFieldZoneId(0, 0));

	bool bSubmitted = false;
	FGuid ChosenCardId;
	FName ChosenZoneId = NAME_None;

	if (CardOptions.Num() > 0 && ZoneOptions.Num() > 0)
	{
		ChosenCardId = CardOptions[0];
		ChosenZoneId = ZoneOptions[0];
		bSubmitted = SubmitPendingPlayGraveyardCardToEmptyZoneChoice(0, ChosenCardId, ChosenZoneId);
	}

	const FTCGCardInstance* SourceAfter = FindCardInstance(SourceCard.CardInstanceId);
	const FTCGCardInstance* ChosenAfter = FindCardInstance(ChosenCardId);
	const FTCGCardInstance* EarthAfter = FindCardInstance(EarthOther.CardInstanceId);

	const bool bSourceStillGraveyard = SourceAfter && SourceAfter->Location == ETCGCardLocation::Graveyard;
	const bool bChosenOnBoard = ChosenAfter && ChosenAfter->Location == ETCGCardLocation::Board && ChosenAfter->ZoneId == ChosenZoneId;
	const bool bEarthStillGraveyard = EarthAfter && EarthAfter->Location == ETCGCardLocation::Graveyard;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Debug: GraveyardPlayChoice summary ChoiceStarted=%s Submitted=%s SourceExcluded=%s WaterEligible=%s EarthExcluded=%s OccupiedZoneExcluded=%s SourceStillGraveyard=%s ChosenOnBoard=%s EarthStillGraveyard=%s CardOptions=%d ZoneOptions=%d Zone=%s"),
		bChoiceStarted ? TEXT("true") : TEXT("false"),
		bSubmitted ? TEXT("true") : TEXT("false"),
		bSourceExcluded ? TEXT("true") : TEXT("false"),
		bWaterEligible ? TEXT("true") : TEXT("false"),
		bEarthExcluded ? TEXT("true") : TEXT("false"),
		bOccupiedZoneExcluded ? TEXT("true") : TEXT("false"),
		bSourceStillGraveyard ? TEXT("true") : TEXT("false"),
		bChosenOnBoard ? TEXT("true") : TEXT("false"),
		bEarthStillGraveyard ? TEXT("true") : TEXT("false"),
		CardOptions.Num(),
		ZoneOptions.Num(),
		ChosenZoneId.IsNone() ? TEXT("None") : *ChosenZoneId.ToString());

	EndMatch(ETCGMatchResult::Draw);
}
