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
	DOREPLIFETIME(ATCG_PlayerState, Life);
	DOREPLIFETIME(ATCG_PlayerState, MaxLife);
}

void ATCG_PlayerState::SetPlayerIndex(int32 NewPlayerIndex)
{
	PlayerIndex = NewPlayerIndex;
}

void ATCG_PlayerState::SetLife(int32 NewLife)
{
	// Keep life inside a valid range.
	Life = FMath::Clamp(NewLife, 0, MaxLife);
}
