#include "Misc/AutomationTest.h"
#include "../WasamiGameMode.h"
#include "../WasamiSaveGame.h"
#include "../WasamiShard.h"
#include "../WasamiTriggerBox.h"
#include "../WasamiZone1Flow.h"
#include "../WasamiZone2Flow.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/BlockingVolume.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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

	AWasamiGameMode* Mode = SpawnMode(World, 4);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 1);
	if (!TestNotNull(TEXT("Zone 1's flow"), Cast<AWasamiZone1Flow>(Flow)))
	{
		return false;
	}
	TestEqual(TEXT("4: the lift arrives"), Flow->GetSection(), FName(TEXT("04_Start")));
	TestTrue(TEXT("no objective yet"), Objective(Mode).IsEmpty());
	// A trigger box walked through before the zone listens is spent (its DoOnce), as in the original; these are walked
	// through only once they are bound.
	Advance(Wrapper, AWasamiZone1Flow::ArrivalShakeSeconds + 0.1f);
	Walk(World, TEXT("04_Intercom"));
	TestEqual(TEXT("the intercom, bound once the shake is over"), Flow->GetSection(), FName(TEXT("04_Intercom")));

	// The lift's door broken open (the door's own finish calls it later); the maze's trigger saves 5.
	TestFalse(TEXT("no such event"), Flow->CallEvent(TEXT("OnNothing")));
	TestTrue(TEXT("04_DoorBreak"), Flow->CallEvent(TEXT("On04DoorBreak")));
	Walk(World, TEXT("BP_04_Trigger_Maze"));
	TestEqual(TEXT("05_Persistent"), Flow->GetSection(), FName(TEXT("05_Persistent")));
	TestEqual(TEXT("checkpoint 5 saved"), SavedCheckpoint(), 5);
	TestEqual(TEXT("the shards wanted"), Objective(Mode), FString(TEXT("COLLECT ALL SHARDS")));

	// A shard left: its check 1 s on finds it. Gone: two checks 0.03 s apart count once, 0.05 s after the first.
	Advance(Wrapper, AWasamiZone1Flow::ShardCheckDelay + 0.1f);
	TestEqual(TEXT("a shard left"), Flow->GetSection(), FName(TEXT("05_Persistent")));
	Shard->Destroy();
	Mode->CheckShards();
	Advance(Wrapper, 0.03f);
	Mode->CheckShards();
	Advance(Wrapper, 0.03f);
	TestEqual(TEXT("all collected"), Flow->GetSection(), FName(TEXT("05 All Shards Collected")));
	TestEqual(TEXT("to the parking lot"), Objective(Mode), FString(TEXT("REACH THE PARKING LOT")));
	TestFalse(TEXT("the arrow off the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8002f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("at the parking lot's trigger"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("06_CutsceneStart")));

	// The parking lot's scene is left out: straight to 06.
	Walk(World, TEXT("06_CutsceneStart"));
	TestEqual(TEXT("06"), Flow->GetSection(), FName(TEXT("06_Start")));
	TestEqual(TEXT("to the tunnel"), Objective(Mode), FString(TEXT("REACH THE TUNNEL")));
	TestTrue(TEXT("the arrow at the tunnel"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("06_TunnelEnter")));
	Walk(World, TEXT("06_TunnelEnter"));
	TestEqual(TEXT("onto the ambulance"), Objective(Mode), FString(TEXT("GET ON TOP OF THE AMBULANCE")));
	TestTrue(TEXT("the arrow at its roof"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("TriggerBox_06_AmbulanceTop")));

	// The doors hold for 25 s.
	Walk(World, TEXT("06_DoorsLock"));
	TestTrue(TEXT("the doors block"), Collides(Doors));
	Advance(Wrapper, AWasamiZone1Flow::DoorsBreakSeconds - 0.5f);
	TestTrue(TEXT("still at 24.5 s"), Collides(Doors));
	Advance(Wrapper, 0.6f);
	TestFalse(TEXT("broken in by 25 s"), Collides(Doors));

	// The ambulance's roof saves 7 (Zone 2 opens 10.5 s on, which a test world does not go on to).
	Walk(World, TEXT("TriggerBox_06_AmbulanceTop"));
	TestEqual(TEXT("06_ReachAmbulance"), Flow->GetSection(), FName(TEXT("06_ReachAmbulance")));
	TestEqual(TEXT("checkpoint 7 saved"), SavedCheckpoint(), 7);
	TestEqual(TEXT("good luck"), Objective(Mode), FString(TEXT("GOOD LUCK")));
	TestTrue(TEXT("the ambulance's sides block"), Collides(AmbulanceSide));

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

	AWasamiGameMode* Mode = SpawnMode(World, 7);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	if (!TestNotNull(TEXT("Zone 2's flow"), Cast<AWasamiZone2Flow>(Flow)))
	{
		return false;
	}
	TestEqual(TEXT("7: out of the cell's scene"), Flow->GetSection(), FName(TEXT("Cell Cutscene Finished")));
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

	// The cell's spikes kill 0.5 s after they reach the player.
	Walk(World, TEXT("Trigger_Cell_Spikes"));
	TestTrue(TEXT("not at once"), Mode->IsDeathOpen());
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
	AWasamiGameMode* Mode = SpawnMode(World, 10);
	const AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	TestEqual(TEXT("zone 2 at 10"), Flow ? Flow->GetSection() : NAME_None, FName(TEXT("Postmaze Transition")));
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("the ring piece the tick after"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
