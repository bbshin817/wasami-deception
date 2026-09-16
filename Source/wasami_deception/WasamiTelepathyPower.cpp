#include "WasamiTelepathyPower.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiEnemyInterface.h"
#include "WasamiTelepathyTracker.h"

namespace
{
	// BP_Telepathy (pak_reference_2): K2_SetTimer('Update Targets', 0.8, looping).
	constexpr float UpdateInterval = 0.8f;
}

AWasamiTelepathyPower::AWasamiTelepathyPower()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = SceneRoot;
	TrackerClass = AWasamiTelepathyTracker::StaticClass();
}

void AWasamiTelepathyPower::BeginPlay()
{
	Super::BeginPlay();
	// Update Targets at once and every 0.8 s; Finish after Time (a timer of 0 s is never set, as a Blueprint's).
	UpdateTargets();
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(UpdateTimer, this, &AWasamiTelepathyPower::UpdateTargetsOnTimer, UpdateInterval, true);
	Timers.SetTimer(FinishTimer, this, &AWasamiTelepathyPower::Finish, Time, false);
}

void AWasamiTelepathyPower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);
	GetWorldTimerManager().ClearTimer(FinishTimer);
	Super::EndPlay(EndPlayReason);
}

int32 AWasamiTelepathyPower::UpdateTargets()
{
	// Every enemy of the level, however far; each gets one tracker for the whole telepathy.
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithInterface(this, UWasamiEnemyInterface::StaticClass(), Enemies);
	int32 Spawned = 0;
	for (AActor* Enemy : Enemies)
	{
		if (IWasamiEnemyInterface::Execute_NoTelepathy(Enemy) || ActorsWithTracker.Contains(Enemy))
		{
			continue;
		}
		// Deferred at the enemy, unrotated, with the enemy to follow; the class decides how to handle a collision.
		const FTransform SpawnTransform(FRotator::ZeroRotator, Enemy->GetActorLocation());
		if (AWasamiTelepathyTracker* Tracker = GetWorld()->SpawnActorDeferred<AWasamiTelepathyTracker>(TrackerClass,
			SpawnTransform))
		{
			Tracker->Actor = Enemy;
			Tracker->FinishSpawning(SpawnTransform);
			++Spawned;
		}
		ActorsWithTracker.AddUnique(Enemy);
	}
	return Spawned;
}

void AWasamiTelepathyPower::Finish()
{
	GetWorldTimerManager().ClearTimer(UpdateTimer);
	// Every tracker in the world, whichever telepathy put it there.
	TArray<AActor*> Trackers;
	UGameplayStatics::GetAllActorsOfClass(this, TrackerClass, Trackers);
	for (AActor* Each : Trackers)
	{
		CastChecked<AWasamiTelepathyTracker>(Each)->Remove();
	}
	Destroy();
}
