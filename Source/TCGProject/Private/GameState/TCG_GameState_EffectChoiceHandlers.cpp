#include "GameState/TCG_GameState.h"

bool ATCG_GameState::BeginPendingDiscardChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingDiscardChoice.bIsPending) return false;
	TArray<FTCGCardInstance> HandCards;
	GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() < Count) return false;
	PendingDiscardChoice.Reset();
	PendingDiscardChoice.bIsPending = true;
	PendingDiscardChoice.PlayerIndex = PlayerIndex;
	PendingDiscardChoice.RequiredCount = Count;
	PendingDiscardChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingDiscardChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : HandCards) PendingDiscardChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingDiscardChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingDiscardChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingDiscardChoice.bIsPending || PendingDiscardChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingDiscardChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingDiscardChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Hand) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToLocation(ChosenId, ETCGCardLocation::Graveyard);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending discard choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingDiscardChoice();
	return true;
}

bool ATCG_GameState::HasPendingDiscardChoice() const { return PendingDiscardChoice.bIsPending; }
void ATCG_GameState::GetPendingDiscardChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingDiscardChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingDiscardChoice() { PendingDiscardChoice.Reset(); }

bool ATCG_GameState::BeginPendingGraveyardToDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingGraveyardToDeckChoice.bIsPending) return false;
	TArray<FTCGCardInstance> GraveyardCards;
	GetCardsInLocation(PlayerIndex, ETCGCardLocation::Graveyard, GraveyardCards);
	if (GraveyardCards.Num() < Count) return false;
	GraveyardCards.Sort([](const FTCGCardInstance& A, const FTCGCardInstance& B){ return A.LocationIndex < B.LocationIndex; });
	PendingGraveyardToDeckChoice.Reset();
	PendingGraveyardToDeckChoice.bIsPending = true;
	PendingGraveyardToDeckChoice.PlayerIndex = PlayerIndex;
	PendingGraveyardToDeckChoice.RequiredCount = Count;
	PendingGraveyardToDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingGraveyardToDeckChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : GraveyardCards) PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingGraveyardToDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingGraveyardToDeckChoice.bIsPending || PendingGraveyardToDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingGraveyardToDeckChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingGraveyardToDeckChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Graveyard) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToBottomOfDeck(ChosenId);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending graveyard-to-deck choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingGraveyardToDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingGraveyardToDeckChoice() const { return PendingGraveyardToDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingGraveyardToDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingGraveyardToDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingGraveyardToDeckChoice() { PendingGraveyardToDeckChoice.Reset(); }

bool ATCG_GameState::BeginPendingHandToTopDeckChoice(int32 PlayerIndex, int32 Count, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || Count <= 0 || PendingHandToTopDeckChoice.bIsPending) return false;
	TArray<FTCGCardInstance> HandCards;
	GetCardsInHand(PlayerIndex, HandCards);
	if (HandCards.Num() < Count) return false;
	PendingHandToTopDeckChoice.Reset();
	PendingHandToTopDeckChoice.bIsPending = true;
	PendingHandToTopDeckChoice.PlayerIndex = PlayerIndex;
	PendingHandToTopDeckChoice.RequiredCount = Count;
	PendingHandToTopDeckChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingHandToTopDeckChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : HandCards) PendingHandToTopDeckChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
	return PendingHandToTopDeckChoice.EligibleCardInstanceIds.Num() >= Count;
}

bool ATCG_GameState::SubmitPendingHandToTopDeckChoice(int32 PlayerIndex, const TArray<FGuid>& ChosenCardInstanceIds)
{
	if (!PendingHandToTopDeckChoice.bIsPending || PendingHandToTopDeckChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenCardInstanceIds.Num() != PendingHandToTopDeckChoice.RequiredCount) return false;
	TSet<FGuid> UniqueChosenIds;
	for (const FGuid& ChosenId : ChosenCardInstanceIds)
	{
		if (!ChosenId.IsValid() || UniqueChosenIds.Contains(ChosenId)) return false;
		if (!PendingHandToTopDeckChoice.EligibleCardInstanceIds.Contains(ChosenId)) return false;
		const FTCGCardInstance* Card = FindCardInstance(ChosenId);
		if (!Card || Card->OwnerPlayerIndex != PlayerIndex || Card->Location != ETCGCardLocation::Hand) return false;
		UniqueChosenIds.Add(ChosenId);
	}
	for (const FGuid& ChosenId : ChosenCardInstanceIds) MoveCardToTopOfDeck(ChosenId);
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending hand-to-top-deck choice submitted Player=%d Count=%d"), PlayerIndex, ChosenCardInstanceIds.Num());
	ClearPendingHandToTopDeckChoice();
	return true;
}

bool ATCG_GameState::HasPendingHandToTopDeckChoice() const { return PendingHandToTopDeckChoice.bIsPending; }
void ATCG_GameState::GetPendingHandToTopDeckChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingHandToTopDeckChoice.EligibleCardInstanceIds; }
void ATCG_GameState::ClearPendingHandToTopDeckChoice() { PendingHandToTopDeckChoice.Reset(); }

bool ATCG_GameState::BeginPendingPlayToEmptyZoneChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingPlayToEmptyZoneChoice.bIsPending) return false;
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	if (!SourceCard) return false;
	PendingPlayToEmptyZoneChoice.Reset();
	PendingPlayToEmptyZoneChoice.bIsPending = true;
	PendingPlayToEmptyZoneChoice.PlayerIndex = PlayerIndex;
	PendingPlayToEmptyZoneChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingPlayToEmptyZoneChoice.ChainIndex = ChainEntry.ChainIndex;
	for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
	{
		const FName ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);
		FGuid ExistingStackId;
		if (!FindStackIdInZone(ZoneId, ExistingStackId)) PendingPlayToEmptyZoneChoice.EligibleZoneIds.Add(ZoneId);
	}
	return PendingPlayToEmptyZoneChoice.EligibleZoneIds.Num() > 0;
}

bool ATCG_GameState::SubmitPendingPlayToEmptyZoneChoice(int32 PlayerIndex, FName ChosenZoneId)
{
	if (!PendingPlayToEmptyZoneChoice.bIsPending || PendingPlayToEmptyZoneChoice.PlayerIndex != PlayerIndex) return false;
	if (ChosenZoneId.IsNone() || !PendingPlayToEmptyZoneChoice.EligibleZoneIds.Contains(ChosenZoneId)) return false;
	const FGuid SourceCardId = PendingPlayToEmptyZoneChoice.SourceCardInstanceId;
	const bool bPlayed = PlaySourceCardToEmptyZone(SourceCardId, ChosenZoneId);
	if (!bPlayed) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending play-to-empty-zone choice submitted Player=%d Zone=%s"), PlayerIndex, *ChosenZoneId.ToString());
	ClearPendingPlayToEmptyZoneChoice();
	return true;
}

bool ATCG_GameState::HasPendingPlayToEmptyZoneChoice() const { return PendingPlayToEmptyZoneChoice.bIsPending; }
void ATCG_GameState::GetPendingPlayToEmptyZoneChoiceOptions(TArray<FName>& OutZoneIds) const { OutZoneIds = PendingPlayToEmptyZoneChoice.EligibleZoneIds; }
void ATCG_GameState::ClearPendingPlayToEmptyZoneChoice() { PendingPlayToEmptyZoneChoice.Reset(); }

bool ATCG_GameState::BeginPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry)
{
	if (!IsValidPlayerIndex(PlayerIndex) || PendingAttachSourceToWaterUnitChoice.bIsPending) return false;
	const FTCGCardInstance* SourceCard = FindCardInstance(ChainEntry.SourceCardInstanceId);
	if (!SourceCard) return false;
	PendingAttachSourceToWaterUnitChoice.Reset();
	PendingAttachSourceToWaterUnitChoice.bIsPending = true;
	PendingAttachSourceToWaterUnitChoice.PlayerIndex = PlayerIndex;
	PendingAttachSourceToWaterUnitChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingAttachSourceToWaterUnitChoice.ChainIndex = ChainEntry.ChainIndex;
	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != PlayerIndex) continue;
		if (Card.Location != ETCGCardLocation::Board) continue;
		if (Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
		if (Card.Element != ETCGCardElement::Water) continue;
		const FTCGCardInstance* TopCard = FindTopCardInStack(Card.StackId);
		if (!TopCard || TopCard->CardInstanceId != Card.CardInstanceId) continue;
		PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Add(Card.CardInstanceId);
	}
	return PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Num() > 0;
}

bool ATCG_GameState::SubmitPendingAttachSourceToWaterUnitChoice(int32 PlayerIndex, const FGuid& ChosenTargetCardInstanceId)
{
	if (!PendingAttachSourceToWaterUnitChoice.bIsPending || PendingAttachSourceToWaterUnitChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenTargetCardInstanceId.IsValid() || !PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds.Contains(ChosenTargetCardInstanceId)) return false;
	const bool bAttached = AttachSourceCardUnderWaterUnit(PendingAttachSourceToWaterUnitChoice.SourceCardInstanceId, ChosenTargetCardInstanceId);
	if (!bAttached) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending attach source to unit choice submitted Player=%d"), PlayerIndex);
	ClearPendingAttachSourceToWaterUnitChoice();
	return true;
}

bool ATCG_GameState::HasPendingAttachSourceToWaterUnitChoice() const { return PendingAttachSourceToWaterUnitChoice.bIsPending; }
void ATCG_GameState::GetPendingAttachSourceToWaterUnitChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingAttachSourceToWaterUnitChoice.EligibleTargetCardInstanceIds; }
void ATCG_GameState::ClearPendingAttachSourceToWaterUnitChoice() { PendingAttachSourceToWaterUnitChoice.Reset(); }

bool ATCG_GameState::BeginPendingPlayGraveyardCardToEmptyZoneChoice(
int32 PlayerIndex,
const FTCGEffectTargetFilter& TargetFilter,
const FTCGEffectChainEntry& ChainEntry)
{
ClearPendingPlayGraveyardCardToEmptyZoneChoice();

if (!IsValidPlayerIndex(PlayerIndex))
{
return false;
}

PendingPlayGraveyardCardToEmptyZoneChoice.bIsPending = true;
PendingPlayGraveyardCardToEmptyZoneChoice.PlayerIndex = PlayerIndex;
PendingPlayGraveyardCardToEmptyZoneChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
PendingPlayGraveyardCardToEmptyZoneChoice.ChainIndex = ChainEntry.ChainIndex;

const int32 RequiredOwnerPlayerIndex =
TargetFilter.OwnerMode == ETCGEffectTargetMode::Opponent
? 1 - PlayerIndex
: PlayerIndex;

auto DoesCardNameMatchFilter = [&TargetFilter](const FTCGCardInstance& Card)
{
const FString CardDefinitionIdString = Card.CardDefinitionId.ToString();

if (!TargetFilter.NameContains.IsEmpty()
&& !CardDefinitionIdString.Contains(TargetFilter.NameContains, ESearchCase::IgnoreCase))
{
return false;
}

if (TargetFilter.NameContainsAny.Num() <= 0)
{
return true;
}

for (const FString& NameOption : TargetFilter.NameContainsAny)
{
if (!NameOption.IsEmpty()
&& CardDefinitionIdString.Contains(NameOption, ESearchCase::IgnoreCase))
{
return true;
}
}

return false;
};

for (const FTCGCardInstance& Card : MatchCards)
{
if (Card.OwnerPlayerIndex != RequiredOwnerPlayerIndex) continue;
if (Card.Location != ETCGCardLocation::Graveyard) continue;
if (TargetFilter.bExcludeSourceCard && Card.CardInstanceId == ChainEntry.SourceCardInstanceId) continue;
if (TargetFilter.bRequireElement && Card.Element != TargetFilter.RequiredElement) continue;
if (!DoesCardNameMatchFilter(Card)) continue;

PendingPlayGraveyardCardToEmptyZoneChoice.EligibleCardInstanceIds.Add(Card.CardInstanceId);
}

for (int32 FieldIndex = 0; FieldIndex < FieldZoneCount; ++FieldIndex)
{
const FName ZoneId = GetFieldZoneId(PlayerIndex, FieldIndex);

FGuid ExistingStackId;
if (!FindStackIdInZone(ZoneId, ExistingStackId))
{
PendingPlayGraveyardCardToEmptyZoneChoice.EligibleZoneIds.Add(ZoneId);
}
}

const bool bStarted =
PendingPlayGraveyardCardToEmptyZoneChoice.EligibleCardInstanceIds.Num() > 0
&& PendingPlayGraveyardCardToEmptyZoneChoice.EligibleZoneIds.Num() > 0;

if (!bStarted)
{
ClearPendingPlayGraveyardCardToEmptyZoneChoice();
return false;
}

UE_LOG(LogTemp, Warning,
TEXT("TCG Effect: Pending play graveyard card to empty zone choice started Player=%d Cards=%d Zones=%d"),
PlayerIndex,
PendingPlayGraveyardCardToEmptyZoneChoice.EligibleCardInstanceIds.Num(),
PendingPlayGraveyardCardToEmptyZoneChoice.EligibleZoneIds.Num());

return true;
}

bool ATCG_GameState::SubmitPendingPlayGraveyardCardToEmptyZoneChoice(int32 PlayerIndex, const FGuid& ChosenCardInstanceId, FName ChosenZoneId)
{
	if (!PendingPlayGraveyardCardToEmptyZoneChoice.bIsPending || PendingPlayGraveyardCardToEmptyZoneChoice.PlayerIndex != PlayerIndex) return false;
	if (!ChosenCardInstanceId.IsValid() || !PendingPlayGraveyardCardToEmptyZoneChoice.EligibleCardInstanceIds.Contains(ChosenCardInstanceId)) return false;
	if (ChosenZoneId.IsNone() || !PendingPlayGraveyardCardToEmptyZoneChoice.EligibleZoneIds.Contains(ChosenZoneId)) return false;
	const bool bPlayed = PlayGraveyardCardToEmptyZone(ChosenCardInstanceId, ChosenZoneId);
	if (!bPlayed) return false;
	UE_LOG(LogTemp, Warning, TEXT("TCG Effect: Pending play graveyard card to empty zone choice submitted Player=%d Zone=%s"), PlayerIndex, *ChosenZoneId.ToString());
	ClearPendingPlayGraveyardCardToEmptyZoneChoice();
	return true;
}

bool ATCG_GameState::HasPendingPlayGraveyardCardToEmptyZoneChoice() const { return PendingPlayGraveyardCardToEmptyZoneChoice.bIsPending; }
void ATCG_GameState::GetPendingPlayGraveyardCardToEmptyZoneCardOptions(TArray<FGuid>& OutCardInstanceIds) const { OutCardInstanceIds = PendingPlayGraveyardCardToEmptyZoneChoice.EligibleCardInstanceIds; }
void ATCG_GameState::GetPendingPlayGraveyardCardToEmptyZoneZoneOptions(TArray<FName>& OutZoneIds) const { OutZoneIds = PendingPlayGraveyardCardToEmptyZoneChoice.EligibleZoneIds; }
void ATCG_GameState::ClearPendingPlayGraveyardCardToEmptyZoneChoice() { PendingPlayGraveyardCardToEmptyZoneChoice.Reset(); }
