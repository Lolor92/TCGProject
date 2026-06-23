#include "PlayerState/TCG_PlayerState.h"
#include "Net/UnrealNetwork.h"

ATCG_PlayerState::ATCG_PlayerState()
{
	bReplicates = true;
}

void ATCG_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCG_PlayerState, PlayerIndex);
	DOREPLIFETIME(ATCG_PlayerState, bHasLost);
}

void ATCG_PlayerState::SetPlayerIndex(int32 NewPlayerIndex)
{
	PlayerIndex = NewPlayerIndex;
}

void ATCG_PlayerState::SetHasLost(bool bNewHasLost)
{
	bHasLost = bNewHasLost;
}
