#include "Misc/AutomationTest.h"
#include "../WasamiDoorBreak.h"
#include "../WasamiDoubleDoors.h"
#include "../WasamiGameMode.h"
#include "../WasamiHitFX.h"
#include "../WasamiSaveGame.h"
#include "../WasamiShard.h"
#include "../WasamiTriggerBox.h"
#include "../WasamiZone1Flow.h"
#include "../WasamiZone2Flow.h"
#include "../WasamiZoneBarrier.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/BlockingVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "EngineUtils.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString FlowTestSlotName(TEXT("WasamiTest_ZoneFlow"));

	void SpawnTriggers(UWorld* World, std::initializer_list<const TCHAR*> Names)
	{
		for (const TCHAR* Name : Names)
		{
			AWasamiTriggerBox* Box = World->SpawnActor<AWasamiTriggerBox>(FVector(0., 0., -100000.), FRotator::ZeroRotator);
			Box->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		}
	}

	ABlockingVolume* SpawnBlocker(UWorld* World, const TCHAR* Name, ECollisionEnabled::Type Enabled)
	{
		ABlockingVolume* Volume = World->SpawnActor<ABlockingVolume>(FVector(0., 0., -100000.), FRotator::ZeroRotator);
		Volume->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		Volume->GetBrushComponent()->SetCollisionEnabled(Enabled);
		return Volume;
	}

	/** A level sequence actor placed from the original's of that name, with an empty sequence of Seconds. */
	ALevelSequenceActor* SpawnSequence(UWorld* World, const TCHAR* Name, double Seconds)
	{
		ULevelSequence* Sequence = NewObject<ULevelSequence>(GetTransientPackage());
		Sequence->Initialize();
		UMovieScene* Scene = Sequence->GetMovieScene();
		Scene->SetPlaybackRange(FFrameNumber(0), Scene->GetTickResolution().AsFrameNumber(Seconds).Value);
		ALevelSequenceActor* Actor = World->SpawnActor<ALevelSequenceActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		Actor->SetSequence(Sequence);
		Actor->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		return Actor;
	}

	/** A static actor placed from the original's of that name (Static, as the level has the ambulance and the switch). */
	AStaticMeshActor* SpawnStatic(UWorld* World, const TCHAR* Name, const FVector& Location, const FRotator& Rotation)
	{
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Location, Rotation);
		Actor->GetRootComponent()->SetMobility(EComponentMobility::Static);
		Actor->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		return Actor;
	}

	/** The player walking into the trigger box of that name. */
	void Walk(UWorld* World, const TCHAR* Name)
	{
		if (AWasamiTriggerBox* Box = Cast<AWasamiTriggerBox>(AWasamiZoneFlow::FindSource(World, Name)))
		{
			Box->NotifyPlayerOverlap(true);
		}
	}

	/** A game mode (writing to the test slot) that opens at Checkpoint. */
	AWasamiGameMode* SpawnMode(UWorld* World, int32 Checkpoint)
	{
		UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
		Save->Hospital.LevelCheckpoint = Checkpoint;
		UGameplayStatics::SaveGameToSlot(Save, FlowTestSlotName, UWasamiSaveGame::UserIndex);
		AWasamiGameMode* Mode = World->SpawnActorDeferred<AWasamiGameMode>(AWasamiGameMode::StaticClass(), FTransform::Identity);
		Mode->SaveSlotName = FlowTestSlotName;
		Mode->FinishSpawning(FTransform::Identity);
		return Mode;
	}

	int32 SavedCheckpoint()
	{
		const UWasamiSaveGame* Save = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex));
		return Save ? Save->Hospital.LevelCheckpoint : -1;
	}

	/** The sequence player a fade (Basic DD Fade Out) made for a sequence of that name, or null. */
	ULevelSequencePlayer* MadePlayer(UWorld* World, const TCHAR* SequenceName)
	{
		for (TActorIterator<ALevelSequenceActor> It(World); It; ++It)
		{
			const ULevelSequence* Sequence = It->GetSequence();
			if (Sequence && Sequence->GetName() == SequenceName)
			{
				return It->GetSequencePlayer();
			}
		}
		return nullptr;
	}

	FString Objective(const AWasamiGameMode* Mode)
	{
		return Mode->CurrentObjective.ToString();
	}

	bool Collides(const ABlockingVolume* Volume)
	{
		return Volume->GetBrushComponent()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics;
	}

	/**
	 * Seconds of play, in ticks of 0.1 s at most (the world cuts a longer tick to its MaxUndilatedFrameTime, 0.4 s). A
	 * timer set between two ticks waits, pending, for the next tick to start it (FTimerManager), as a Delay does in play;
	 * a tick of nothing starts those first.
	 */
	void Advance(FTestWorldWrapper& Wrapper, float Seconds)
	{
		Wrapper.TickTestWorld(0.f);
		for (float Left = Seconds; Left > 0.f; Left -= 0.1f)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, 0.1f));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowTriggerBoxTest, "Wasami.ZoneFlow.TriggerBox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowTriggerBoxTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiTriggerBox* Box = World->SpawnActor<AWasamiTriggerBox>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the trigger box"), Box))
	{
		return false;
	}
	const UBoxComponent* Shape = Box->GetBox();
	TestTrue(TEXT("UE's default box"), Shape->GetUnscaledBoxExtent().Equals(FVector(32.), 1e-3));
	TestTrue(TEXT("unseen in play"), Shape->bHiddenInGame);
	TestTrue(TEXT("a WorldStatic object"), Shape->GetCollisionObjectType() == ECC_WorldStatic);
	TestTrue(TEXT("queries only"), Shape->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("overlaps pawns"), Shape->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
	TestTrue(TEXT("ignores the world"), Shape->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore);
	TestTrue(TEXT("ignores moving things"), Shape->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Ignore);
	TestTrue(TEXT("ignores the view"), Shape->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);
	TestTrue(TEXT("ignores the camera"), Shape->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore);

	Box->NotifyPlayerOverlap(false);
	TestFalse(TEXT("walking out does not fire it"), Box->HasFired());
	Box->NotifyPlayerOverlap(true);
	TestTrue(TEXT("walking in does"), Box->HasFired());

	AWasamiTriggerBox* OnLeaving = World->SpawnActor<AWasamiTriggerBox>(FVector::ZeroVector, FRotator::ZeroRotator);
	OnLeaving->bEndOverlap = true;
	OnLeaving->NotifyPlayerOverlap(true);
	TestFalse(TEXT("with End Overlap, walking in does not"), OnLeaving->HasFired());
	OnLeaving->NotifyPlayerOverlap(false);
	TestTrue(TEXT("and walking out does"), OnLeaving->HasFired());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowZone1Test, "Wasami.ZoneFlow.Zone1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowZone1Test::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	SpawnTriggers(World, {TEXT("04_Intercom"), TEXT("BP_04_Trigger_Maze"), TEXT("06_CutsceneStart"), TEXT("06_DoorsLock"),
		TEXT("06_TunnelEnter"), TEXT("TriggerBox_06_AmbulanceTop")});
	ABlockingVolume* Doors = SpawnBlocker(World, TEXT("BlockingVolume_1"), ECollisionEnabled::NoCollision);
	ABlockingVolume* AmbulanceSide = SpawnBlocker(World, TEXT("BlockingVolume_Ambulance_2"), ECollisionEnabled::NoCollision);
	AWasamiShard* Shard = World->SpawnActor<AWasamiShard>(FVector(0., 0., -90000.), FRotator::ZeroRotator);
	const ALevelSequenceActor* Arrival = SpawnSequence(World, TEXT("06_Hospital_Zone01_ElevatorArrive"), 14.1);
	const ALevelSequenceActor* TakeOff = SpawnSequence(World, TEXT("06_Hospital_Zone1_AmbulanceTakeOff"), 13.9);
	AWasamiDoorBreak* DoorBreak = World->SpawnActorDeferred<AWasamiDoorBreak>(AWasamiDoorBreak::StaticClass(), FTransform::Identity);
	DoorBreak->ProgressSpeed = 1.5f;
	DoorBreak->FinishSpawning(FTransform::Identity);
	DoorBreak->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_Hospital_DoorBreak_2")));
	AWasamiDoubleDoors* LiftDoors = World->SpawnActor<AWasamiDoubleDoors>(FVector(0., 0., -80000.), FRotator::ZeroRotator);
	LiftDoors->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_DoubleDoors11")));
	AWasamiDoubleDoors* TunnelDoors = World->SpawnActor<AWasamiDoubleDoors>(FVector(0., 0., -70000.), FRotator::ZeroRotator);
	TunnelDoors->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_DoubleDoors33_36")));
	AWasamiZoneBarrier* Barrier = World->SpawnActor<AWasamiZoneBarrier>(FVector(0., 0., -60000.), FRotator::ZeroRotator);
	Barrier->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_ZoneBarrier_2")));
	const TWeakObjectPtr<AWasamiZoneBarrier> BarrierRef(Barrier);
	// The tunnel's burst of concrete, asleep as the level has it (an empty system: only its being woken is tested).
	// Its component is registered as it spawns, so it cannot be kept from waking there (SetAutoActivate is refused and
	// the template wakes it): it is put to sleep after.
	AEmitter* Burst = World->SpawnActor<AEmitter>(FVector(0., 0., -50000.), FRotator::ZeroRotator);
	Burst->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("Fracture_concrete_5")));
	Burst->GetParticleSystemComponent()->SetTemplate(NewObject<UParticleSystem>(GetTransientPackage()));
	Burst->GetParticleSystemComponent()->DeactivateImmediate();

	AWasamiGameMode* Mode = SpawnMode(World, 4);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 1);
	if (!TestNotNull(TEXT("Zone 1's flow"), Cast<AWasamiZone1Flow>(Flow)))
	{
		return false;
	}
	TestEqual(TEXT("4: the lift arrives"), Flow->GetSection(), FName(TEXT("04_Start")));
	TestTrue(TEXT("its sequence plays"), Arrival->GetSequencePlayer() && Arrival->GetSequencePlayer()->IsPlaying());
	TestTrue(TEXT("the lift's doors locked"), LiftDoors->bLocked);
	LiftDoors->OpenFront();
	TestFalse(TEXT("and they stay shut"), LiftDoors->AnySideOpen());
	TestTrue(TEXT("no objective yet"), Objective(Mode).IsEmpty());
	// A trigger box walked through before the zone listens is spent (its DoOnce), as in the original; these are walked
	// through only once they are bound.
	TestFalse(TEXT("the lift door's lock asleep"), DoorBreak->GetBox()->GetGenerateOverlapEvents());
	Advance(Wrapper, AWasamiZone1Flow::ArrivalShakeSeconds + 0.1f);
	Walk(World, TEXT("04_Intercom"));
	TestEqual(TEXT("the intercom, bound once the shake is over"), Flow->GetSection(), FName(TEXT("04_Intercom")));
	TestFalse(TEXT("no such event"), Flow->CallEvent(TEXT("OnNothing")));

	// The lift door's lock wakes with the intercom; picked (67 presses at 1.5), it breaks the door open. The maze's
	// trigger saves 5.
	TestTrue(TEXT("the lock awake"), DoorBreak->GetBox()->GetGenerateOverlapEvents());
	DoorBreak->NotifyPlayerOverlap(true);
	for (int32 Press = 0; Press < 67; ++Press)
	{
		DoorBreak->Interact();
	}
	TestEqual(TEXT("picked: 04_DoorBreak"), Flow->GetSection(), FName(TEXT("04_DoorBreak")));
	TestFalse(TEXT("the lift's doors unlocked"), LiftDoors->bLocked);
	TestTrue(TEXT("and swinging open from the front"), LiftDoors->IsOpenFront() && LiftDoors->IsOpening());
	Walk(World, TEXT("BP_04_Trigger_Maze"));
	TestEqual(TEXT("05_Persistent"), Flow->GetSection(), FName(TEXT("05_Persistent")));
	TestEqual(TEXT("checkpoint 5 saved"), SavedCheckpoint(), 5);
	TestEqual(TEXT("the shards wanted"), Objective(Mode), FString(TEXT("COLLECT ALL SHARDS")));

	// A shard left: its check 1 s on finds it. Gone: two checks 0.03 s apart count once, 0.05 s after the first.
	Advance(Wrapper, AWasamiZone1Flow::ShardCheckDelay + 0.1f);
	TestEqual(TEXT("a shard left"), Flow->GetSection(), FName(TEXT("05_Persistent")));
	TestTrue(TEXT("the barrier stands"), BarrierRef.IsValid() && !BarrierRef->IsActorBeingDestroyed());
	Shard->Destroy();
	Mode->CheckShards();
	Advance(Wrapper, 0.03f);
	Mode->CheckShards();
	Advance(Wrapper, 0.03f);
	TestEqual(TEXT("all collected"), Flow->GetSection(), FName(TEXT("05 All Shards Collected")));
	TestEqual(TEXT("to the parking lot"), Objective(Mode), FString(TEXT("REACH THE PARKING LOT")));
	TestTrue(TEXT("the barrier broken"), !BarrierRef.IsValid() || BarrierRef->IsActorBeingDestroyed());
	TestFalse(TEXT("the arrow off the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8002f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("at the parking lot's trigger"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("06_CutsceneStart")));

	// The parking lot's scene is left out: straight to 06, under the fade (Ballroom_Event_Fade at twice its rate).
	TestNull(TEXT("no fade yet"), MadePlayer(World, TEXT("Ballroom_Event_Fade")));
	Walk(World, TEXT("06_CutsceneStart"));
	TestEqual(TEXT("06"), Flow->GetSection(), FName(TEXT("06_Start")));
	const ULevelSequencePlayer* Fade = MadePlayer(World, TEXT("Ballroom_Event_Fade"));
	TestTrue(TEXT("the fade plays"), Fade && Fade->IsPlaying());
	TestEqual(TEXT("at twice its rate"), Fade ? Fade->GetPlayRate() : 0.f, AWasamiZone1Flow::TransitionFadeRate);
	TestEqual(TEXT("to the tunnel"), Objective(Mode), FString(TEXT("REACH THE TUNNEL")));
	TestTrue(TEXT("the arrow at the tunnel"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("06_TunnelEnter")));
	Walk(World, TEXT("06_TunnelEnter"));
	TestEqual(TEXT("onto the ambulance"), Objective(Mode), FString(TEXT("GET ON TOP OF THE AMBULANCE")));
	TestTrue(TEXT("the arrow at its roof"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("TriggerBox_06_AmbulanceTop")));

	// The doors swing shut, locked, and hold for 25 s; then they are broken in, with the burst, and gone 0.1 s on.
	const TWeakObjectPtr<AWasamiDoubleDoors> TunnelDoorsRef(TunnelDoors);
	TunnelDoors->OpenFront();
	TestTrue(TEXT("the tunnel's doors open"), TunnelDoors->IsOpenFront());
	Walk(World, TEXT("06_DoorsLock"));
	TestTrue(TEXT("locked"), TunnelDoors->bLocked);
	TestTrue(TEXT("and swinging shut"), !TunnelDoors->AnySideOpen() && TunnelDoors->IsClosing());
	TestTrue(TEXT("the doors block"), Collides(Doors));
	Advance(Wrapper, AWasamiZone1Flow::DoorsBreakSeconds - 0.5f);
	TestTrue(TEXT("still at 24.5 s"), Collides(Doors));
	TestFalse(TEXT("the burst asleep"), Burst->GetParticleSystemComponent()->IsActive());
	Advance(Wrapper, 0.6f);
	TestFalse(TEXT("broken in by 25 s"), Collides(Doors));
	TestTrue(TEXT("the burst woken"), Burst->GetParticleSystemComponent()->IsActive());
	Advance(Wrapper, 0.2f);
	TestTrue(TEXT("the doors gone"), !TunnelDoorsRef.IsValid() || TunnelDoorsRef->IsActorBeingDestroyed());

	// The ambulance's roof saves 7 and the ambulance leaves 1 s on (Zone 2 opens 10.5 s on, which a test world does not
	// go on to).
	Walk(World, TEXT("TriggerBox_06_AmbulanceTop"));
	TestEqual(TEXT("06_ReachAmbulance"), Flow->GetSection(), FName(TEXT("06_ReachAmbulance")));
	TestEqual(TEXT("checkpoint 7 saved"), SavedCheckpoint(), 7);
	TestEqual(TEXT("good luck"), Objective(Mode), FString(TEXT("GOOD LUCK")));
	TestTrue(TEXT("the ambulance's sides block"), Collides(AmbulanceSide));
	TestFalse(TEXT("the ambulance still"), TakeOff->GetSequencePlayer() && TakeOff->GetSequencePlayer()->IsPlaying());
	Advance(Wrapper, AWasamiZone1Flow::TakeOffDelay + 0.1f);
	TestTrue(TEXT("and leaving 1 s on"), TakeOff->GetSequencePlayer() && TakeOff->GetSequencePlayer()->IsPlaying());

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowZone2Test, "Wasami.ZoneFlow.Zone2",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowZone2Test::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	SpawnTriggers(World, {TEXT("Trigger_Cell_Spikes"), TEXT("BP_MiniBoss_Trigger"), TEXT("Miniboss_BierceTalk"),
		TEXT("Trigger_MazeStart"), TEXT("Trigger_Miniboss_BehindMatron")});
	AActor* Orb = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	Orb->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("ring_statue_orb_5")));
	const ALevelSequenceActor* Spikes = SpawnSequence(World, TEXT("06_Hospital_Zone2_Spikes"), 70.);
	const ALevelSequenceActor* DoorPicked = SpawnSequence(World, TEXT("06_Hospital_Zone2_Cell_DoorPicked"), 4.2667);
	AWasamiDoorBreak* DoorBreak = World->SpawnActorDeferred<AWasamiDoorBreak>(AWasamiDoorBreak::StaticClass(), FTransform::Identity);
	DoorBreak->ProgressSpeed = 3.f;
	DoorBreak->FinishSpawning(FTransform::Identity);
	DoorBreak->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_Hospital_DoorBreak_2")));
	// What the left-out scenes move, where the level has it.
	const FRotator AmbulanceFacing(0., -90.000684, 0.);
	const AStaticMeshActor* Ambulance = SpawnStatic(World, TEXT("hospital_ambulance_new_arrive"), FVector(-22442.7148, -5025.0068, 800.), AmbulanceFacing);
	const TWeakObjectPtr<ABlockingVolume> Blocker(SpawnBlocker(World, TEXT("Ambulance_Arrive_Blockers4"), ECollisionEnabled::QueryAndPhysics));
	AStaticMeshActor* Ceiling = SpawnStatic(World, TEXT("hospital_zone_02_holdingCell_01_false_ceiling_11"), FVector(-0.0371, 0., 0.), FRotator::ZeroRotator);
	Ceiling->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	const FVector SwitchAt(-14199.4062, 712.5936, 199.5126);
	const AStaticMeshActor* Switch = SpawnStatic(World, TEXT("hospital_zone_02_holdingCell_01_wall_switch_14"), SwitchAt, FRotator(0., 0., -43.689768));

	AWasamiGameMode* Mode = SpawnMode(World, 7);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	if (!TestNotNull(TEXT("Zone 2's flow"), Cast<AWasamiZone2Flow>(Flow)))
	{
		return false;
	}
	TestEqual(TEXT("7: out of the cell's scene"), Flow->GetSection(), FName(TEXT("Cell Cutscene Finished")));
	TestTrue(TEXT("the ambulance arrived"), Ambulance->GetActorLocation().Equals(AWasamiZone2Flow::AmbulanceArrived, 0.01)
		&& Ambulance->GetActorRotation().Equals(AmbulanceFacing, 1e-3));
	TestTrue(TEXT("its front blocker gone"), !Blocker.IsValid() || Blocker->IsActorBeingDestroyed());
	TestTrue(TEXT("the false ceiling open"), Ceiling->GetActorLocation().Equals(AWasamiZone2Flow::FalseCeilingOpen, 0.01));
	TestTrue(TEXT("the switch thrown"), Switch->GetActorLocation().Equals(SwitchAt, 0.01)
		&& Switch->GetActorRotation().Equals(AWasamiZone2Flow::WallSwitchThrown, 1e-3));
	TestTrue(TEXT("the spikes coming down"), Spikes->GetSequencePlayer() && Spikes->GetSequencePlayer()->IsPlaying());

	// The cell's door lock (34 presses at 3.0) opens the door with its sequence.
	TestTrue(TEXT("the cell door's lock awake"), DoorBreak->GetBox()->GetGenerateOverlapEvents());
	DoorBreak->NotifyPlayerOverlap(true);
	for (int32 Press = 0; Press < 34; ++Press)
	{
		DoorBreak->Interact();
	}
	TestEqual(TEXT("picked: Cell_DoorBreak"), Flow->GetSection(), FName(TEXT("Cell_DoorBreak")));
	TestTrue(TEXT("the door swings open"), DoorPicked->GetSequencePlayer() && DoorPicked->GetSequencePlayer()->IsPlaying());
	Advance(Wrapper, 1.1f);
	Walk(World, TEXT("Miniboss_BierceTalk"));
	TestEqual(TEXT("Bierce's trigger, bound 1 s on"), Flow->GetSection(), FName(TEXT("Miniboss_BierceTalk")));

	Walk(World, TEXT("BP_MiniBoss_Trigger"));
	TestEqual(TEXT("the Matron's corridor"), Flow->GetSection(), FName(TEXT("Miniboss Transition ")));
	TestEqual(TEXT("checkpoint 8 saved"), SavedCheckpoint(), 8);
	TestEqual(TEXT("past the nurses, with the original's space"), Objective(Mode), FString(TEXT("Get past the nurses ")));
	TestTrue(TEXT("the arrow clear"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(0.f, 0.f, 0.f, 0.f)));
	TestNull(TEXT("and without a target"), Flow->GetArrowTarget());

	Walk(World, TEXT("Trigger_MazeStart"));
	TestEqual(TEXT("checkpoint 9 saved"), SavedCheckpoint(), 9);
	TestEqual(TEXT("the maze's shards"), Objective(Mode), FString(TEXT("COLLECT ALL SHARDS")));
	TestTrue(TEXT("the arrow on the shards"), Flow->IsArrowOnShards());

	Mode->CheckShards();
	Advance(Wrapper, 0.1f);
	TestEqual(TEXT("checkpoint 10 saved"), SavedCheckpoint(), 10);
	TestFalse(TEXT("the statue's orb gone"), IsValid(Orb));
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("to the ring piece"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8941f, 0.f, 1.f), 1e-4f));

	// The cell's spikes kill 0.5 s after they reach the player, who is hit at once.
	Walk(World, TEXT("Trigger_Cell_Spikes"));
	TestTrue(TEXT("not at once"), Mode->IsDeathOpen());
	TestTrue(TEXT("the hit's flash"), TActorIterator<AWasamiHitFX>(World) ? true : false);
	Advance(Wrapper, AWasamiZone2Flow::SpikesDeathDelay + 0.1f);
	TestFalse(TEXT("dead 0.5 s on"), Mode->IsDeathOpen());

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowStartTest, "Wasami.ZoneFlow.Start",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowStartTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	SpawnTriggers(World, {TEXT("06_DoorsLock"), TEXT("06_TunnelEnter"), TEXT("TriggerBox_06_AmbulanceTop"),
		TEXT("Trigger_MazeStart"), TEXT("Trigger_Miniboss_BehindMatron")});
	TestNull(TEXT("no flow outside the zones"), AWasamiZoneFlow::SpawnFor(SpawnMode(World, 4), 0));
	const FVector InTunnel(-22442.7148, -5025.0068, 800.);
	const AStaticMeshActor* Ambulance = SpawnStatic(World, TEXT("hospital_ambulance_new_arrive"), InTunnel, FRotator::ZeroRotator);

	// Each checkpoint's section, as a zone reopened there starts it.
	struct FCase
	{
		int32 Zone;
		int32 Checkpoint;
		const TCHAR* Section;
		const TCHAR* Objective;
	};
	const FCase Cases[] = {
		{1, 5, TEXT("05_Persistent"), TEXT("COLLECT ALL SHARDS")},
		{1, 6, TEXT("06_Start"), TEXT("REACH THE TUNNEL")},
		{2, 8, TEXT("Miniboss Transition "), TEXT("Get past the nurses ")},
		{2, 9, TEXT("Maze Transition "), TEXT("COLLECT ALL SHARDS")},
	};
	for (const FCase& Case : Cases)
	{
		AWasamiGameMode* Mode = SpawnMode(World, Case.Checkpoint);
		const AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, Case.Zone);
		const FString What = FString::Printf(TEXT("zone %d at %d"), Case.Zone, Case.Checkpoint);
		TestEqual(*What, Flow ? Flow->GetSection() : NAME_None, FName(Case.Section));
		TestEqual(*(What + TEXT(": the objective")), Objective(Mode), FString(Case.Objective));
	}
	TestTrue(TEXT("past 7, the ambulance never arrived (the original plays its scene only at 7)"),
		Ambulance->GetActorLocation().Equals(InTunnel, 0.01));
	AWasamiGameMode* Mode = SpawnMode(World, 10);
	const AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	TestEqual(TEXT("zone 2 at 10"), Flow ? Flow->GetSection() : NAME_None, FName(TEXT("Postmaze Transition")));
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("the ring piece the tick after"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
