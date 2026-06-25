#include "GameState/TCG_GameState.h"

namespace
{
	static bool IsValidSwapTopUnit(const ATCG_GameState* GameState, const FTCGCardInstance* Card)
	{
		if (!GameState || !Card) return false;
		if (Card->Location != ETCGCardLocation::Board) return false;
		if (!Card->StackId.IsValid()) return false;
		if (Card->ZoneId.IsNone()) return false;

		const FTCGCardInstance* TopCard = GameState->FindTopCardInStack(Card->StackId);
		return TopCard && TopCard->CardInstanceId == Card->CardInstanceId;
	}

	static bool IsGuidInList(const TArray<FGuid>& List, const FGuid& Value)
	{
		for (const FGuid& Entry : List)
		{
			if (Entry == Value)
			{
				return true;
			}
		}

		return false;
	}
}

bool ATCG_GameState::BeginPendingSwapOpponentUnitZonesChoice(int32 PlayerIndex, const FTCGEffectChainEntry& ChainEntry)
{
	ClearPendingSwapOpponentUnitZonesChoice();

	if (!IsValidPlayerIndex(PlayerIndex))
	{
		return false;
	}

	const int32 OpponentPlayerIndex = 1 - PlayerIndex;

	for (const FTCGCardInstance& Card : MatchCards)
	{
		if (Card.OwnerPlayerIndex != OpponentPlayerIndex) continue;
		if (!IsValidSwapTopUnit(this, &Card)) continue;

		PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds.Add(Card.CardInstanceId);
	}

	if (PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds.Num() < 2)
	{
		ClearPendingSwapOpponentUnitZonesChoice();
		return false;
	}

	PendingSwapOpponentUnitZonesChoice.bIsPending = true;
	PendingSwapOpponentUnitZonesChoice.PlayerIndex = PlayerIndex;
	PendingSwapOpponentUnitZonesChoice.SourceCardInstanceId = ChainEntry.SourceCardInstanceId;
	PendingSwapOpponentUnitZonesChoice.ChainIndex = ChainEntry.ChainIndex;

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: Pending swap opponent unit zones choice started Player=%d Units=%d"),
		PlayerIndex,
		PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds.Num());

	return true;
}

bool ATCG_GameState::SubmitPendingSwapOpponentUnitZonesChoice(
	int32 PlayerIndex,
	const FGuid& FirstTargetCardInstanceId,
	const FGuid& SecondTargetCardInstanceId)
{
	if (!PendingSwapOpponentUnitZonesChoice.bIsPending)
	{
		return false;
	}

	if (PendingSwapOpponentUnitZonesChoice.PlayerIndex != PlayerIndex)
	{
		return false;
	}

	if (!FirstTargetCardInstanceId.IsValid()
		|| !SecondTargetCardInstanceId.IsValid()
		|| FirstTargetCardInstanceId == SecondTargetCardInstanceId)
	{
		return false;
	}

	if (!IsGuidInList(PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds, FirstTargetCardInstanceId)
		|| !IsGuidInList(PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds, SecondTargetCardInstanceId))
	{
		return false;
	}

	const bool bSwapped = SwapTwoUnitStackZones(FirstTargetCardInstanceId, SecondTargetCardInstanceId);

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: Pending swap opponent unit zones choice submitted Player=%d Swapped=%s"),
		PlayerIndex,
		bSwapped ? TEXT("true") : TEXT("false"));

	ClearPendingSwapOpponentUnitZonesChoice();

	return bSwapped;
}

bool ATCG_GameState::HasPendingSwapOpponentUnitZonesChoice() const
{
	return PendingSwapOpponentUnitZonesChoice.bIsPending;
}

void ATCG_GameState::GetPendingSwapOpponentUnitZonesChoiceOptions(TArray<FGuid>& OutCardInstanceIds) const
{
	OutCardInstanceIds = PendingSwapOpponentUnitZonesChoice.EligibleTargetCardInstanceIds;
}

void ATCG_GameState::ClearPendingSwapOpponentUnitZonesChoice()
{
	PendingSwapOpponentUnitZonesChoice.Reset();
}

bool ATCG_GameState::SwapTwoUnitStackZones(const FGuid& FirstTopCardInstanceId, const FGuid& SecondTopCardInstanceId)
{
	if (FirstTopCardInstanceId == SecondTopCardInstanceId)
	{
		return false;
	}

	const FTCGCardInstance* FirstTopSnapshot = FindCardInstance(FirstTopCardInstanceId);
	const FTCGCardInstance* SecondTopSnapshot = FindCardInstance(SecondTopCardInstanceId);

	if (!IsValidSwapTopUnit(this, FirstTopSnapshot)
		|| !IsValidSwapTopUnit(this, SecondTopSnapshot))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("TCG Effect: Swap opponent unit zones fizzled Reason=TargetMissingOrNotTopBoardUnit"));

		return false;
	}

	if (FirstTopSnapshot->OwnerPlayerIndex != SecondTopSnapshot->OwnerPlayerIndex)
	{
		return false;
	}

	const FGuid FirstStackId = FirstTopSnapshot->StackId;
	const FGuid SecondStackId = SecondTopSnapshot->StackId;
	const FName FirstZoneId = FirstTopSnapshot->ZoneId;
	const FName SecondZoneId = SecondTopSnapshot->ZoneId;

	if (!FirstStackId.IsValid()
		|| !SecondStackId.IsValid()
		|| FirstStackId == SecondStackId
		|| FirstZoneId.IsNone()
		|| SecondZoneId.IsNone()
		|| FirstZoneId == SecondZoneId)
	{
		return false;
	}

	for (FTCGCardInstance& Card : MatchCards)
	{
		if (Card.Location != ETCGCardLocation::Board) continue;

		if (Card.StackId == FirstStackId)
		{
			Card.ZoneId = SecondZoneId;
		}
		else if (Card.StackId == SecondStackId)
		{
			Card.ZoneId = FirstZoneId;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("TCG Effect: Swapped unit zones First=%s Second=%s ZoneA=%s ZoneB=%s"),
		FirstTopSnapshot->CardDefinitionId.IsNone() ? TEXT("None") : *FirstTopSnapshot->CardDefinitionId.ToString(),
		SecondTopSnapshot->CardDefinitionId.IsNone() ? TEXT("None") : *SecondTopSnapshot->CardDefinitionId.ToString(),
		*FirstZoneId.ToString(),
		*SecondZoneId.ToString());

	return true;
}
