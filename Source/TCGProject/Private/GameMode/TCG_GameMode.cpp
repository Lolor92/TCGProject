#include "GameMode/TCG_GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameState/TCG_DebugScenarioRunner.h"
#include "GameState/TCG_GameState.h"
#include "PlayerState/TCG_PlayerState.h"

ATCG_GameMode::ATCG_GameMode()
{
	GameStateClass = ATCG_GameState::StaticClass();
	PlayerStateClass = ATCG_PlayerState::StaticClass();
}

void ATCG_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	ATCG_PlayerState* TCGPlayerState = NewPlayer->GetPlayerState<ATCG_PlayerState>();
	if (!TCGPlayerState) return;

	TCGPlayerState->SetPlayerIndex(NextPlayerIndex);
	NextPlayerIndex++;

	if (NextPlayerIndex >= 2)
	{
		ATCG_GameState* TCGGameState = GetGameState<ATCG_GameState>();
		if (TCGGameState)
		{
			if (TCGGameState->MatchCards.Num() > 0)
			{
				return;
			}

			UTCG_DebugScenarioRunner::RunDebugTurnFlow(TCGGameState);
		}
	}
}