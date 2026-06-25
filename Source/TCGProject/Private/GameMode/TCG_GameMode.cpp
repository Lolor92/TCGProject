#include "GameMode/TCG_GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameState/TCG_GameState.h"
#include "PlayerState/TCG_PlayerState.h"

void RunDebugOverdrivePilotKaiaScenario(ATCG_GameState* GameState);

ATCG_GameMode::ATCG_GameMode()
{
	// Tell Unreal which GameState and PlayerState this GameMode uses.
	GameStateClass = ATCG_GameState::StaticClass();
	PlayerStateClass = ATCG_PlayerState::StaticClass();
}

void ATCG_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	ATCG_PlayerState* TCGPlayerState = NewPlayer->GetPlayerState<ATCG_PlayerState>();
	if (!TCGPlayerState) return;

	// Assign player 0, then player 1, then 2, etc.
	// For now we only care about 0 and 1.
	TCGPlayerState->SetPlayerIndex(NextPlayerIndex);
	NextPlayerIndex++;

	// Once 2 players joined, start the match.
	if (NextPlayerIndex >= 2)
	{
		ATCG_GameState* TCGGameState = GetGameState<ATCG_GameState>();
		if (TCGGameState)
		{
			// Dedicated Overdrive Pilot Kaia debug scenario.
			RunDebugOverdrivePilotKaiaScenario(TCGGameState);
		}
	}
}
