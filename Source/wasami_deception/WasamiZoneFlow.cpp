#include "WasamiZoneFlow.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BrushComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Volume.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiCutsceneWidget.h"
#include "WasamiDoorBreak.h"
#include "WasamiDoubleDoors.h"
#include "WasamiEnemy.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiShard.h"
#include "WasamiTriggerBox.h"
#include "WasamiZone1Flow.h"
#include "WasamiZone2Flow.h"
#include "WasamiZoneBarrier.h"

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
	FadeSequence = TSoftObjectPtr<ULevelSequence>(WasamiAssets::Path(TEXT("/Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade")));
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

void AWasamiZoneFlow::DisablePlayerInput(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0))
	{
		Player->DisableInput(Controller);
		Player->GetCharacterMovement()->StopMovementImmediately();
		if (AWasamiPlayerCharacter* WasamiPlayer = Cast<AWasamiPlayerCharacter>(Player))
		{
			WasamiPlayer->bHasInput = false;
		}
	}
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0))
	{
		Camera->StopAllCameraShakes(false);
	}
}

void AWasamiZoneFlow::EnablePlayerInput(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0))
	{
		Player->EnableInput(Controller);
		if (AWasamiPlayerCharacter* WasamiPlayer = Cast<AWasamiPlayerCharacter>(Player))
		{
			WasamiPlayer->bHasInput = true;
		}
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

void AWasamiZoneFlow::EnableDoorBreak(FName Source, FName Function)
{
	AWasamiDoorBreak* DoorBreak = Cast<AWasamiDoorBreak>(FindSource(GetWorld(), Source));
	if (!DoorBreak)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no door break %s for %s"), *GetClass()->GetName(), *Source.ToString(), *Function.ToString());
		return;
	}
	DoorBreak->EnableSwitch();
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, Function);
	DoorBreak->FinishedEvent.AddUnique(Delegate);
}

AWasamiDoubleDoors* AWasamiZoneFlow::DoubleDoors(FName Source) const
{
	AWasamiDoubleDoors* Doors = Cast<AWasamiDoubleDoors>(FindSource(GetWorld(), Source));
	if (!Doors)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no double doors %s"), *GetClass()->GetName(), *Source.ToString());
	}
	return Doors;
}

AWasamiZoneBarrier* AWasamiZoneFlow::ZoneBarrier(FName Source) const
{
	AWasamiZoneBarrier* Barrier = Cast<AWasamiZoneBarrier>(FindSource(GetWorld(), Source));
	if (!Barrier)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no zone barrier %s"), *GetClass()->GetName(), *Source.ToString());
	}
	return Barrier;
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

void AWasamiZoneFlow::PlaySequence(FName Source, FName Finished)
{
	if (ULevelSequencePlayer* Player = SequencePlayer(Source))
	{
		Player->Play();
		BindSequenceFinished(Player, Finished);
	}
}

void AWasamiZoneFlow::BindSequenceFinished(ULevelSequencePlayer* Player, FName Finished)
{
	if (Player && !Finished.IsNone())
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, Finished);
		Player->OnFinished.AddUnique(Delegate);
	}
}

ULevelSequencePlayer* AWasamiZoneFlow::SequencePlayer(FName Source) const
{
	ALevelSequenceActor* Actor = Cast<ALevelSequenceActor>(FindSource(GetWorld(), Source));
	ULevelSequencePlayer* Player = Actor ? Actor->GetSequencePlayer() : nullptr;
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no level sequence actor %s"), *GetClass()->GetName(), *Source.ToString());
	}
	return Player;
}

void AWasamiZoneFlow::PlayCutscene(FName Source, FName Finished, FName Camera, bool bSmoothTransition)
{
	ULevelSequencePlayer* Player = SequencePlayer(Source);
	if (!Player)
	{
		return;
	}
	if (!Camera.IsNone())
	{
		SetPlayerViewTarget(FindSource(GetWorld(), Camera), CutsceneViewBlendTime);
	}
	// Initialize Cutscene Widget, then Play; the screen binds Cutscene Over to OnFinished itself, and the flow's own
	// event comes after it.
	UWasamiCutsceneWidget::Show(this, Player, bSmoothTransition);
	Player->Play();
	BindSequenceFinished(Player, Finished);
}

void AWasamiZoneFlow::SetPlayerViewTarget(AActor* Target, float BlendTime)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (Controller && Target)
	{
		Controller->SetViewTargetWithBlend(Target, BlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.f, false);
	}
}

void AWasamiZoneFlow::PlayCameraShake(const TSoftClassPtr<UCameraShakeBase>& Shake, float Scale)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (const TSubclassOf<UCameraShakeBase> ShakeClass = Shake.LoadSynchronous(); ShakeClass && Controller)
	{
		Controller->ClientStartCameraShake(ShakeClass, Scale, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
	}
}

void AWasamiZoneFlow::PlayWorldCameraShake(const TSoftClassPtr<UCameraShakeBase>& Shake, const FVector& Epicenter,
	float InnerRadius, float OuterRadius, float Falloff, bool bOrientTowardsEpicenter)
{
	if (const TSubclassOf<UCameraShakeBase> ShakeClass = Shake.LoadSynchronous())
	{
		UGameplayStatics::PlayWorldCameraShake(this, ShakeClass, Epicenter, InnerRadius, OuterRadius, Falloff, bOrientTowardsEpicenter);
	}
}

void AWasamiZoneFlow::PlayFadeOut(float PlayRate)
{
	ULevelSequence* Sequence = FadeSequence.LoadSynchronous();
	if (!Sequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no fade sequence %s"), *GetClass()->GetName(), *FadeSequence.ToString());
		return;
	}
	// The original's playback settings are the defaults (no auto play, no restoring of state).
	ALevelSequenceActor* Actor = nullptr;
	if (ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(this, Sequence, FMovieSceneSequencePlaybackSettings(), Actor))
	{
		Player->SetPlayRate(PlayRate);
		Player->Play();
	}
}

void AWasamiZoneFlow::PlaySoundAt(const TSoftObjectPtr<USoundBase>& Sound, const FVector& Location,
	const TSoftObjectPtr<USoundAttenuation>& Attenuation)
{
	if (USoundBase* Loaded = Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Loaded, Location, FRotator::ZeroRotator, 1.f, 1.f, 0.f, Attenuation.LoadSynchronous());
	}
}

void AWasamiZoneFlow::ActivateEmitter(FName Source)
{
	AEmitter* Emitter = Cast<AEmitter>(FindSource(GetWorld(), Source));
	if (UParticleSystemComponent* Particles = Emitter ? Emitter->GetParticleSystemComponent() : nullptr)
	{
		Particles->Activate(true);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("%s: no emitter %s"), *GetClass()->GetName(), *Source.ToString());
}

AWasamiEnemy* AWasamiZoneFlow::SpawnEnemy(TSubclassOf<AWasamiEnemy> Class, FName SpawnPoint,
	const TFunction<void(AWasamiEnemy&)>& Setup) const
{
	const AActor* Point = Source(SpawnPoint);
	if (!Point)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no target point %s for %s"), *GetClass()->GetName(), *SpawnPoint.ToString(),
			*GetNameSafe(Class));
		return nullptr;
	}
	const FTransform Transform = Point->GetActorTransform();
	AWasamiEnemy* Enemy = GetWorld()->SpawnActorDeferred<AWasamiEnemy>(Class, Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Enemy)
	{
		Enemy->bCanSpawn = true;
		if (Setup)
		{
			Setup(*Enemy);
		}
		Enemy->FinishSpawning(Transform);
	}
	return Enemy;
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
