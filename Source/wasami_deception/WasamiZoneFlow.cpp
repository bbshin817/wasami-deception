#include "WasamiZoneFlow.h"

#include "Components/BrushComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Volume.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WasamiGameMode.h"
#include "WasamiShard.h"
#include "WasamiTriggerBox.h"
#include "WasamiZone1Flow.h"
#include "WasamiZone2Flow.h"

namespace
{
	// Remove All Enemies' tag (BP_DD_Functions): the nurses carry it (AWasamiEnemy).
	const FName RemovableEnemyTag(TEXT("Enemy"));

	FAutoConsoleCommandWithWorldAndArgs FlowCommand(TEXT("Wasami.Flow"),
		TEXT("Wasami.Flow Event: calls the zone flow's event of that function name (On04DoorBreak, On05Transition, OnMazeTriggerStart, ...), as the original's level Blueprint would."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const AWasamiGameMode* Mode = World ? World->GetAuthGameMode<AWasamiGameMode>() : nullptr;
			AWasamiZoneFlow* Flow = Mode ? Mode->GetZoneFlow() : nullptr;
			if (Flow && Args.Num() > 0 && !Flow->CallEvent(FName(*Args[0])))
			{
				UE_LOG(LogTemp, Warning, TEXT("Wasami.Flow: no event %s on %s"), *Args[0], *Flow->GetClass()->GetName());
			}
		}));
}

AWasamiZoneFlow::AWasamiZoneFlow()
{
	PrimaryActorTick.bCanEverTick = false;
}

AWasamiZoneFlow* AWasamiZoneFlow::SpawnFor(AWasamiGameMode* Mode, int32 Zone)
{
	UWorld* World = Mode ? Mode->GetWorld() : nullptr;
	UClass* FlowClass = Zone == 1 ? AWasamiZone1Flow::StaticClass() : Zone == 2 ? AWasamiZone2Flow::StaticClass() : nullptr;
	if (!World || !FlowClass)
	{
		return nullptr;
	}
	AWasamiZoneFlow* Flow = World->SpawnActorDeferred<AWasamiZoneFlow>(FlowClass, FTransform::Identity, Mode);
	if (Flow)
	{
		Flow->Mode = Mode;
		Flow->FinishSpawning(FTransform::Identity);
	}
	return Flow;
}

FName AWasamiZoneFlow::SourceTag(FName Name)
{
	return FName(*(TEXT("src:") + Name.ToString()));
}

AActor* AWasamiZoneFlow::FindSource(const UWorld* World, FName Name)
{
	if (!World || Name.IsNone())
	{
		return nullptr;
	}
	const FName Tag = SourceTag(Name);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Tag))
		{
			return *It;
		}
	}
	return nullptr;
}

void AWasamiZoneFlow::RemoveAllEnemies(UWorld* World)
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsWithTag(World, RemovableEnemyTag, Enemies);
	for (AActor* Enemy : Enemies)
	{
		Enemy->Destroy();
	}
}

void AWasamiZoneFlow::DestroyAllShards(UWorld* World)
{
	// The zones then set the tablet's count to what is left (0); the player's refresh counts the same.
	TArray<AWasamiShard*> Shards;
	for (TActorIterator<AWasamiShard> It(World); It; ++It)
	{
		Shards.Add(*It);
	}
	for (AWasamiShard* Shard : Shards)
	{
		Shard->Destroy();
	}
}

void AWasamiZoneFlow::BeginPlay()
{
	Super::BeginPlay();
	// The zone's Setup: Important Casts and Spawn. Its Spawn also blends the view to the player and enables their input,
	// which a level just opened already has.
	StartAt(Mode ? Mode->GetStartCheckpoint() : 0);
}

bool AWasamiZoneFlow::CallEvent(FName Function)
{
	UFunction* Event = FindFunction(Function);
	if (!Event || Event->NumParms != 0)
	{
		return false;
	}
	ProcessEvent(Event, nullptr);
	return true;
}

void AWasamiZoneFlow::Enter(const TCHAR* Name)
{
	Section = FName(Name);
}

void AWasamiZoneFlow::BindTrigger(FName Source, FName Function)
{
	AWasamiTriggerBox* Box = Cast<AWasamiTriggerBox>(FindSource(GetWorld(), Source));
	if (!Box)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no trigger box %s for %s"), *GetClass()->GetName(), *Source.ToString(), *Function.ToString());
		return;
	}
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, Function);
	Box->OnTrigger.AddUnique(Delegate);
}

void AWasamiZoneFlow::BindAllShardsCollected(FName Function)
{
	if (Mode)
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, Function);
		Mode->OnAllShardsCollected.AddUnique(Delegate);
	}
}

void AWasamiZoneFlow::SetObjective(const FText& Objective)
{
	if (Mode)
	{
		Mode->CurrentObjective = Objective;
	}
}

void AWasamiZoneFlow::SaveCheckpoint(int32 Checkpoint)
{
	if (Mode)
	{
		Mode->SaveCheckpoint(Checkpoint);
	}
}

void AWasamiZoneFlow::SetVolumeCollision(FName Source, ECollisionEnabled::Type Enabled)
{
	const AVolume* Volume = Cast<AVolume>(FindSource(GetWorld(), Source));
	if (UBrushComponent* Brush = Volume ? Volume->GetBrushComponent() : nullptr)
	{
		Brush->SetCollisionEnabled(Enabled);
	}
}

void AWasamiZoneFlow::TeleportPlayerTo(FName PlayerStartTag)
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag != PlayerStartTag)
		{
			continue;
		}
		if (Player)
		{
			Player->TeleportTo(It->GetActorLocation(), FRotator::ZeroRotator);
		}
		if (Controller)
		{
			Controller->SetControlRotation(It->GetActorRotation());
		}
		return;
	}
}

void AWasamiZoneFlow::After(float Seconds, TFunction<void()>&& Then)
{
	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, MoveTemp(Then));
	if (Seconds <= 0.f)
	{
		GetWorldTimerManager().SetTimerForNextTick(Delegate);
		return;
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, Delegate, Seconds, false);
}
