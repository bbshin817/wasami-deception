#include "WasamiGameMode.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiGameInstance.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiSaveGame.h"
#include "WasamiShard.h"

namespace
{
	// ReceiveBeginPlay's Delay before the collected shards are taken out (@34280 → @4551).
	constexpr float ShardRemovalDelay = 0.2f;

	int32 CountShards(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AWasamiShard> It(World); It; ++It)
		{
			++Count;
		}
		return Count;
	}
}

AWasamiGameMode::AWasamiGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AWasamiPlayerCharacter::StaticClass();
	CurrentObjective = NSLOCTEXT("Wasami", "ObjectiveCollectAllShards", "Collect all shards");
	SaveSlotName = UWasamiSaveGame::SlotName;
}

void AWasamiGameMode::BeginPlay()
{
	Super::BeginPlay();
	CheckForLevelStructSave();
	// The original also sets the tablet's count to the level's shards here; the player's tablet refresh (every 0.1 s)
	// counts what is left.
	GetWorldTimerManager().SetTimer(ShardRemovalTimer, this, &AWasamiGameMode::RemoveShardsToBeRemoved, ShardRemovalDelay, false);
}

void AWasamiGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTimeCounting)
	{
		Time += DeltaSeconds;
	}
}

void AWasamiGameMode::CheckForLevelStructSave()
{
	StructSave = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UWasamiSaveGame::UserIndex));
	if (!StructSave)
	{
		StructSave = Cast<UWasamiSaveGame>(UGameplayStatics::CreateSaveGameObject(UWasamiSaveGame::StaticClass()));
		WriteSave();
	}
}

void AWasamiGameMode::WriteSave()
{
	if (StructSave)
	{
		UGameplayStatics::SaveGameToSlot(StructSave, SaveSlotName, UWasamiSaveGame::UserIndex);
	}
}

void AWasamiGameMode::DeathEvent(AActor* Cause)
{
	if (bDeathClosed)
	{
		return;
	}
	bDeathClosed = true;
	// Not here: Bierce's idle lines, whose timer the original clears first (the voices are item 20).
	OnDeath.Broadcast(Cause);
	if (StructSave)
	{
		ShardStreak = FMath::Max(ShardStreak, StructSave->Hospital.CurrentStreak);
		StructSave->Hospital.CurrentStreak = 0;
	}
}

void AWasamiGameMode::ResetDeath()
{
	bDeathClosed = false;
}

void AWasamiGameMode::PauseTimeCounter()
{
	bTimeCounting = false;
}

void AWasamiGameMode::UnpauseTimeCounter()
{
	bTimeCounting = true;
}

void AWasamiGameMode::ResetTimeCounter()
{
	Time = 0.f;
}

void AWasamiGameMode::SaveCheckpoint(int32 Checkpoint)
{
	// The levels' checkpoints (Zone 1's 05_Transition, 06_ReachAmbulance, Zone 2's four): after UMG_Saving is added
	// (not yet), the checkpoint, Time += the game mode's Time, Reset Time Counter, SaveGameToSlot.
	if (!StructSave)
	{
		return;
	}
	StructSave->Hospital.LevelCheckpoint = Checkpoint;
	StructSave->Hospital.Time += Time;
	ResetTimeCounter();
	WriteSave();
}

UWasamiGameInstance* AWasamiGameMode::GetWasamiGameInstance() const
{
	return GetGameInstance<UWasamiGameInstance>();
}

void AWasamiGameMode::RemoveShardsToBeRemoved()
{
	TotalShards = CountShards(GetWorld());
	const UWasamiGameInstance* Instance = GetWasamiGameInstance();
	if (!Instance || Instance->GetShardsToBeRemoved().IsEmpty())
	{
		return;
	}
	if (RemoveCollectedShards(GetWorld(), Instance->GetShardsToBeRemoved()) < 1)
	{
		OnAllShardsAlreadyCollected.Broadcast();
	}
}

int32 AWasamiGameMode::RemoveCollectedShards(UWorld* World, const TArray<FVector>& Collected)
{
	// Each shard's present place, truncated, against the list; destroyed shards drop out of the count that follows.
	TArray<AWasamiShard*> Found;
	for (TActorIterator<AWasamiShard> It(World); It; ++It)
	{
		if (Collected.Contains(UWasamiGameInstance::ShardKey(It->GetActorLocation())))
		{
			Found.Add(*It);
		}
	}
	for (AWasamiShard* Shard : Found)
	{
		Shard->Destroy();
	}
	return CountShards(World);
}
