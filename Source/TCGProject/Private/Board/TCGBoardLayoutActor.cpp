#include "Board/TCGBoardLayoutActor.h"

#include "Board/TCGCardZoneActor.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace
{
float GetCenteredSlotOffset(const int32 SlotIndex, const int32 SlotCount, const float SlotSpacing)
{
	return (static_cast<float>(SlotIndex) - (static_cast<float>(SlotCount - 1) * 0.5f)) * SlotSpacing;
}

FTCGCardZoneId MakePresetZoneId(
	const FName BoardId,
	const int32 PlayerIndex,
	const ETCGCardZoneType ZoneType,
	const int32 ZoneIndex,
	const TCHAR* ZoneName)
{
	FTCGCardZoneId ZoneId;
	ZoneId.BoardId = BoardId;
	ZoneId.PlayerIndex = PlayerIndex;
	ZoneId.ZoneType = ZoneType;
	ZoneId.ZoneIndex = ZoneIndex;
	ZoneId.ZoneName = ZoneName;

	return ZoneId;
}
}

ATCGBoardLayoutActor::ATCGBoardLayoutActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ATCGBoardLayoutActor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoRegisterZonesOnBeginPlay)
	{
		RefreshZoneRegistry();
	}
}

void ATCGBoardLayoutActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATCGBoardLayoutActor, BoardId);
}

void ATCGBoardLayoutActor::RefreshZoneRegistry()
{
	RegisteredZones.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATCGCardZoneActor> It(World); It; ++It)
	{
		ATCGCardZoneActor* Zone = *It;
		if (Zone && Zone->GetZoneId().BoardId == BoardId)
		{
			RegisteredZones.Add(Zone);
		}
	}
}

void ATCGBoardLayoutActor::CreateOrUpdateTwoPlayerReferencePreset()
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateOrUpdateTwoPlayerReferencePreset must be run on a board actor placed in a level."));
		return;
	}

	Modify();
	RefreshZoneRegistry();
	RemoveLegacyPresetHandPreviewSlots();

	const FRotator ZoneRotation = FRotator::ZeroRotator;
	const int32 PlayerOneIndex = 0;
	const int32 PlayerTwoIndex = 1;

	for (int32 SlotIndex = 0; SlotIndex < PresetFieldZoneCount; ++SlotIndex)
	{
		const float SlotX = GetCenteredSlotOffset(SlotIndex, PresetFieldZoneCount, PresetSlotSpacing);
		const int32 ZoneIndex = PresetFieldZoneCount - SlotIndex - 1;
		const int32 DisplayZoneNumber = ZoneIndex + 1;

		CreateOrUpdatePresetZone(
			MakePresetZoneId(BoardId, PlayerTwoIndex, ETCGCardZoneType::Field, ZoneIndex, *FString::Printf(TEXT("P2_Field_%d"), DisplayZoneNumber)),
			FText::FromString(FString::Printf(TEXT("P2 Zone %d"), DisplayZoneNumber)),
			FTransform(ZoneRotation, FVector(SlotX, PresetFieldRowOffset, 0.0f)),
			PresetZoneExtent,
			PresetPlayerTwoColor);

		CreateOrUpdatePresetZone(
			MakePresetZoneId(BoardId, PlayerOneIndex, ETCGCardZoneType::Field, ZoneIndex, *FString::Printf(TEXT("P1_Field_%d"), DisplayZoneNumber)),
			FText::FromString(FString::Printf(TEXT("P1 Zone %d"), DisplayZoneNumber)),
			FTransform(ZoneRotation, FVector(SlotX, -PresetFieldRowOffset, 0.0f)),
			PresetZoneExtent,
			PresetPlayerOneColor);
	}

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerTwoIndex, ETCGCardZoneType::Hand, 0, TEXT("P2_Hand")),
		FText::FromString(TEXT("P2 Hand")),
		FTransform(ZoneRotation, FVector(0.0f, PresetHandRowOffset, 0.0f)),
		PresetHandAreaExtent,
		PresetPlayerTwoColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerOneIndex, ETCGCardZoneType::Hand, 0, TEXT("P1_Hand")),
		FText::FromString(TEXT("P1 Hand")),
		FTransform(ZoneRotation, FVector(0.0f, -PresetHandRowOffset, 0.0f)),
		PresetHandAreaExtent,
		PresetPlayerOneColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerTwoIndex, ETCGCardZoneType::Deck, 0, TEXT("P2_Deck")),
		FText::FromString(TEXT("P2 Deck")),
		FTransform(ZoneRotation, FVector(PresetSidePileColumnOffset, PresetHandRowOffset, 0.0f)),
		PresetZoneExtent,
		PresetPlayerTwoColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerTwoIndex, ETCGCardZoneType::Graveyard, 0, TEXT("P2_Graveyard")),
		FText::FromString(TEXT("P2 Graveyard")),
		FTransform(ZoneRotation, FVector(PresetSidePileColumnOffset, PresetHandRowOffset - PresetSidePileSpacing, 0.0f)),
		PresetZoneExtent,
		PresetPlayerTwoColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerOneIndex, ETCGCardZoneType::Banish, 0, TEXT("P1_Banish")),
		FText::FromString(TEXT("P1 Banish")),
		FTransform(ZoneRotation, FVector(PresetSidePileColumnOffset, -PresetFieldRowOffset, 0.0f)),
		PresetZoneExtent,
		PresetPlayerOneColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerTwoIndex, ETCGCardZoneType::Banish, 0, TEXT("P2_Banish")),
		FText::FromString(TEXT("P2 Banish")),
		FTransform(ZoneRotation, FVector(-PresetSidePileColumnOffset, PresetFieldRowOffset, 0.0f)),
		PresetZoneExtent,
		PresetPlayerTwoColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerOneIndex, ETCGCardZoneType::Graveyard, 0, TEXT("P1_Graveyard")),
		FText::FromString(TEXT("P1 Graveyard")),
		FTransform(ZoneRotation, FVector(-PresetSidePileColumnOffset, -PresetHandRowOffset + PresetSidePileSpacing, 0.0f)),
		PresetZoneExtent,
		PresetPlayerOneColor);

	CreateOrUpdatePresetZone(
		MakePresetZoneId(BoardId, PlayerOneIndex, ETCGCardZoneType::Deck, 0, TEXT("P1_Deck")),
		FText::FromString(TEXT("P1 Deck")),
		FTransform(ZoneRotation, FVector(-PresetSidePileColumnOffset, -PresetHandRowOffset, 0.0f)),
		PresetZoneExtent,
		PresetPlayerOneColor);

	CaptureAllZonePlacements();
}

void ATCGBoardLayoutActor::AssignThisAsAnchorForZones()
{
	RefreshZoneRegistry();

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (Zone)
		{
			Zone->SetPlacementAnchor(this);
		}
	}
}

void ATCGBoardLayoutActor::CaptureAllZonePlacements()
{
	AssignThisAsAnchorForZones();

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (Zone)
		{
			Zone->CapturePlacementFromCurrentTransform();
		}
	}
}

void ATCGBoardLayoutActor::RestoreAllZonePlacements()
{
	AssignThisAsAnchorForZones();

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (Zone)
		{
			Zone->RestorePlacementToSavedTransform();
		}
	}
}

TArray<ATCGCardZoneActor*> ATCGBoardLayoutActor::GetRegisteredZones() const
{
	TArray<ATCGCardZoneActor*> Zones;
	Zones.Reserve(RegisteredZones.Num());

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (Zone)
		{
			Zones.Add(Zone);
		}
	}

	return Zones;
}

ATCGCardZoneActor* ATCGBoardLayoutActor::FindZone(const FTCGCardZoneId& ZoneId) const
{
	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (Zone && Zone->GetZoneId().Matches(ZoneId))
		{
			return Zone;
		}
	}

	return nullptr;
}

void ATCGBoardLayoutActor::RemoveLegacyPresetHandPreviewSlots()
{
	TArray<TObjectPtr<ATCGCardZoneActor>> ZonesToKeep;
	ZonesToKeep.Reserve(RegisteredZones.Num());

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (!Zone)
		{
			continue;
		}

		const FTCGCardZoneId& ZoneId = Zone->GetZoneId();
		const FString ZoneName = ZoneId.ZoneName.ToString();
		const bool bIsLegacyGeneratedHandSlot =
			ZoneId.BoardId == BoardId
			&& ZoneId.ZoneType == ETCGCardZoneType::Hand
			&& ZoneName.StartsWith(TEXT("P"))
			&& ZoneName.Contains(TEXT("_Hand_"));

		if (bIsLegacyGeneratedHandSlot)
		{
			Zone->Modify();
			Zone->Destroy();
			continue;
		}

		ZonesToKeep.Add(Zone);
	}

	RegisteredZones = ZonesToKeep;
}

void ATCGBoardLayoutActor::RemoveDuplicateZonesForId(const FTCGCardZoneId& ZoneId, ATCGCardZoneActor* ZoneToKeep)
{
	TArray<TObjectPtr<ATCGCardZoneActor>> ZonesToKeep;
	ZonesToKeep.Reserve(RegisteredZones.Num());

	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (!Zone)
		{
			continue;
		}

		const FTCGCardZoneId& ExistingZoneId = Zone->GetZoneId();
		const bool bIsSamePresetZone =
			ExistingZoneId.BoardId == ZoneId.BoardId
			&& ExistingZoneId.PlayerIndex == ZoneId.PlayerIndex
			&& ExistingZoneId.ZoneType == ZoneId.ZoneType
			&& ExistingZoneId.ZoneIndex == ZoneId.ZoneIndex
			&& ExistingZoneId.ZoneName == ZoneId.ZoneName;

		if (bIsSamePresetZone && Zone != ZoneToKeep)
		{
			Zone->Modify();
			Zone->Destroy();
			continue;
		}

		ZonesToKeep.Add(Zone);
	}

	RegisteredZones = ZonesToKeep;
}

ATCGCardZoneActor* ATCGBoardLayoutActor::FindExactRegisteredZone(const FTCGCardZoneId& ZoneId) const
{
	for (ATCGCardZoneActor* Zone : RegisteredZones)
	{
		if (!Zone)
		{
			continue;
		}

		const FTCGCardZoneId& ExistingZoneId = Zone->GetZoneId();
		if (ExistingZoneId.BoardId == ZoneId.BoardId
			&& ExistingZoneId.PlayerIndex == ZoneId.PlayerIndex
			&& ExistingZoneId.ZoneType == ZoneId.ZoneType
			&& ExistingZoneId.ZoneIndex == ZoneId.ZoneIndex
			&& ExistingZoneId.ZoneName == ZoneId.ZoneName)
		{
			return Zone;
		}
	}

	return nullptr;
}

ATCGCardZoneActor* ATCGBoardLayoutActor::CreateOrUpdatePresetZone(
	const FTCGCardZoneId& ZoneId,
	const FText& Label,
	const FTransform& AnchorRelativeTransform,
	const FVector& DebugExtent,
	const FColor& DebugColor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ATCGCardZoneActor* Zone = FindExactRegisteredZone(ZoneId);
	if (!Zone)
	{
		const FString ActorName = FString::Printf(TEXT("%s_%s"), *BoardId.ToString(), *ZoneId.ZoneName.ToString());

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = MakeUniqueObjectName(World, ATCGCardZoneActor::StaticClass(), FName(*ActorName));
		// Presets are often re-run while iterating in the editor. Requested avoids an editor fatal if an older actor already owns the name.
		SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
		SpawnParameters.Owner = this;
		SpawnParameters.OverrideLevel = GetLevel();
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		Zone = World->SpawnActor<ATCGCardZoneActor>(
			ATCGCardZoneActor::StaticClass(),
			AnchorRelativeTransform * GetActorTransform(),
			SpawnParameters);

		if (Zone)
		{
			Zone->SetFlags(RF_Transactional);
			RegisteredZones.AddUnique(Zone);
		}
	}

	if (!Zone)
	{
		return nullptr;
	}

	Zone->Modify();
	Zone->ConfigureZone(ZoneId, Label, DebugExtent, FRotator::ZeroRotator, DebugColor, bPresetShowDebugShapesInGame);
	Zone->SetPlacementAnchor(this);
	Zone->SetActorRelativeTransform(AnchorRelativeTransform);
	Zone->CapturePlacementFromCurrentTransform();
	RemoveDuplicateZonesForId(ZoneId, Zone);

	return Zone;
}
