#include "GameMode/TCG_GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameState/TCG_GameState.h"
#include "PlayerState/TCG_PlayerState.h"

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
			TCGGameState->StartMatch();
			TCGGameState->SetupDebugMatch();

			// Add extra debug deck copies so modular draw/discard effects can be tested
			// without immediately running into an empty deck.
			for (int32 ExtraCopyIndex = 0; ExtraCopyIndex < 2; ++ExtraCopyIndex)
			{
				TCGGameState->AddDebugCardInstance("Debug_Earth_Deck_A", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Earth_Deck_B", ETCGCardElement::Earth, 1, 0, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Fire_Deck_A", ETCGCardElement::Fire, 2, 0, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Fire_Deck_B", ETCGCardElement::Fire, 3, 0, ETCGCardLocation::Deck);

				TCGGameState->AddDebugCardInstance("Debug_Dark_Deck_A", ETCGCardElement::Dark, 5, 1, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Dark_Deck_B", ETCGCardElement::Dark, 2, 1, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
				TCGGameState->AddDebugCardInstance("Debug_Light_Deck_A", ETCGCardElement::Light, 4, 1, ETCGCardLocation::Deck);
			}

			TCGGameState->RunDebugTurnFlow();
		}
	}
}
