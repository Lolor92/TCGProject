#include "PlayerController/TCG_PlayerController.h"

#include "UI/TCGMatchHUDWidgetBase.h"

void ATCG_PlayerController::BeginPlay()
{
Super::BeginPlay();

CreateMatchHUD();

if (bUseDebugHUDData)
{
PushDebugHUDData();
}
}

void ATCG_PlayerController::CreateMatchHUD()
{
if (!IsLocalController())
{
return;
}

if (MatchHUDWidget || !MatchHUDWidgetClass)
{
return;
}

MatchHUDWidget = CreateWidget<UTCGMatchHUDWidgetBase>(this, MatchHUDWidgetClass);
if (MatchHUDWidget)
{
MatchHUDWidget->AddToViewport();
}
}

void ATCG_PlayerController::SetMatchHUDData(const FTCGMatchHUDWidgetData& InHUDData)
{
if (MatchHUDWidget)
{
MatchHUDWidget->SetHUDData(InHUDData);
}
}

void ATCG_PlayerController::PushDebugHUDData()
{
if (!IsLocalController() || !MatchHUDWidget)
{
return;
}

FTCGMatchHUDWidgetData Data;

FTCGCardWidgetData FireCard;
FireCard.CardName = FText::FromString(TEXT("Fire Cub"));
FireCard.Attack = 2;
FireCard.Element = FName(TEXT("Fire"));
FireCard.HandIndex = 0;
FireCard.bIsPlayable = true;

FTCGCardWidgetData WindCard;
WindCard.CardName = FText::FromString(TEXT("Wind Sprite"));
WindCard.Attack = 1;
WindCard.Element = FName(TEXT("Wind"));
WindCard.HandIndex = 1;
WindCard.bIsPlayable = true;

FTCGCardWidgetData EarthCard;
EarthCard.CardName = FText::FromString(TEXT("Earth Golem"));
EarthCard.Attack = 4;
EarthCard.Element = FName(TEXT("Earth"));
EarthCard.HandIndex = 2;
EarthCard.bIsPlayable = true;

Data.LocalHand.Cards = { FireCard, WindCard, EarthCard };
Data.LocalHand.SelectedHandIndex = INDEX_NONE;
Data.OpponentHandCount = 4;
Data.PhaseText = FText::FromString(TEXT("Debug UI"));
Data.TurnPlayerText = FText::FromString(TEXT("Local Player"));

SetMatchHUDData(Data);
}
