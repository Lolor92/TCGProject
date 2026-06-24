#include "GameState/TCG_GameState.h"

int32 ATCG_GameState::SendTopDeckCardsToGraveyard(int32 PlayerIndex, int32 Count)
{
	return 0;
}

bool ATCG_GameState::GrantTemporaryAttackToUnits(int32 PlayerIndex, const FTCGEffectTargetFilter& TargetFilter, int32 Amount)
{
	return false;
}

void ATCG_GameState::ClearExpiredTemporaryAttackBoosts()
{
}

bool ATCG_GameState::HandleCardSentToGraveyard(const FGuid& SentCardInstanceId)
{
	return false;
}

void ATCG_GameState::RunDebugWaterGraveyardBoostScenario()
{
	UE_LOG(LogTemp, Warning, TEXT("TCG Debug: WaterRoundBonus scenario start"));
	EndMatch(ETCGMatchResult::Draw);
}
