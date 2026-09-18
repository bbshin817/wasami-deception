#include "WasamiZoneShardChecker.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WasamiGameMode.h"
#include "WasamiShard.h"

AWasamiZoneShardChecker::AWasamiZoneShardChecker()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
}

void AWasamiZoneShardChecker::BeginPlay()
{
	Super::BeginPlay();
	// ReceiveBeginPlay: Check Shards on the game mode's Collect Shard. (Its ZoneShards, the shards in the box as play
	// begins, are for Remove / Collect All In Zone only.)
	if (AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>())
	{
		Mode->OnCollectShard.AddDynamic(this, &AWasamiZoneShardChecker::CheckShards);
	}
}

void AWasamiZoneShardChecker::GetShards(TArray<AActor*>& Shards) const
{
	Box->GetOverlappingActors(Shards, AWasamiShard::StaticClass());
}

void AWasamiZoneShardChecker::CheckShards()
{
	if (!GetWorldTimerManager().IsTimerActive(CheckShardsTimer))
	{
		GetWorldTimerManager().SetTimer(CheckShardsTimer, this, &AWasamiZoneShardChecker::CheckShardsLeft, CheckShardsDelay,
			false);
	}
}

void AWasamiZoneShardChecker::CheckShardsLeft()
{
	TArray<AActor*> Shards;
	GetShards(Shards);
	if (Shards.Num() == 0)
	{
		K2_DestroyActor();
	}
}
