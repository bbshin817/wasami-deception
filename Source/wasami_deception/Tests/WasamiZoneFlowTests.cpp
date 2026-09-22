#include "Misc/AutomationTest.h"
#include "../WasamiCollectable.h"
#include "../WasamiDoorBreak.h"
#include "../WasamiDoubleDoors.h"
#include "../WasamiEnemy.h"
#include "../WasamiEnemy06Chase.h"
#include "../WasamiEnemySentry.h"
#include "../WasamiEnemyZone2.h"
#include "../WasamiGameMode.h"
#include "../WasamiGarageLift.h"
#include "../WasamiHitFX.h"
#include "../WasamiLift.h"
#include "../WasamiMatron.h"
#include "../WasamiMusicPlayer.h"
#include "../WasamiPlayerCharacter.h"
#include "../WasamiPortal.h"
#include "../WasamiRingPiece.h"
#include "../WasamiRingPieceWidget.h"
#include "../WasamiRingStatue.h"
#include "../WasamiSaveGame.h"
#include "../WasamiShard.h"
#include "../WasamiTriggerBox.h"
#include "../WasamiZone1Flow.h"
#include "../WasamiZone2Flow.h"
#include "../WasamiZoneBarrier.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/LightComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/BlockingVolume.h"
#include "Engine/PointLight.h"
#include "Engine/TargetPoint.h"
#include "Engine/TriggerVolume.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "EngineUtils.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"
#include "WasamiTestBierceTalk.h"
#include "UObject/UObjectIterator.h"

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

	/**
	 * Zone 1's target points its flow spawns the nurses at (far below and apart, turned to 90 degrees) and its
	 * TriggerVolume_1, which is returned.
	 */
	ATriggerVolume* SpawnZone1NursePlaces(UWorld* World)
	{
		double Y = 0.;
		for (const TCHAR* Name : {TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2"), TEXT("NurseSpawn_3"), TEXT("06_NurseSpawn"),
			TEXT("06_NurseSpawn2"), TEXT("DoorLocation")})
		{
			Y += 1000.;
			ATargetPoint* Point = World->SpawnActor<ATargetPoint>(FVector(0., Y, -40000.), FRotator(0., 90., 0.));
			Point->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		}
		ATriggerVolume* Volume = World->SpawnActor<ATriggerVolume>(FVector(0., 0., -100000.), FRotator::ZeroRotator);
		Volume->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("TriggerVolume_1")));
		return Volume;
	}

	/** Zone 2's target points its flow spawns the maze's nurses at (far below and apart, turned to 90 degrees). */
	void SpawnZone2NursePlaces(UWorld* World)
	{
		double Y = 0.;
		for (const TCHAR* Name : {TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2"), TEXT("NurseSpawn_4")})
		{
			Y -= 1000.;
			ATargetPoint* Point = World->SpawnActor<ATargetPoint>(FVector(0., Y, -40000.), FRotator(0., 90., 0.));
			Point->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		}
	}

	/** The zone's music player placed from the original's actor of that name, with bFadeOut as the level has it. */
	template <typename T>
	T* SpawnMusicPlayer(UWorld* World, FName Name, bool bFadeOut)
	{
		T* Music = World->SpawnActor<T>(FVector(0., 0., -110000.), FRotator::ZeroRotator);
		Music->bFadeOut = bFadeOut;
		Music->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		return Music;
	}

	/**
	 * The level's one talker, placed from the original's BierceTalk_Blueprint_2: what Bierce Talk finds and speaks
	 * through. It is the test's own kind, whose bSpeaking stands in for a line going: a test world's ticks do not
	 * carry the audio device, so a component a Play has once started there keeps reporting itself as playing, and a
	 * plain talker would wait out every line that follows for ever.
	 */
	AWasamiTestBierceTalk* SpawnTalker(UWorld* World)
	{
		AWasamiTestBierceTalk* Talker = World->SpawnActor<AWasamiTestBierceTalk>(FVector(0., 0., -130000.), FRotator::ZeroRotator);
		Talker->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BierceTalk_Blueprint_2")));
		return Talker;
	}

	/** What the talker was last given to say, by name, or nothing. */
	FString Spoken(const AWasamiBierceTalk* Talker)
	{
		const USoundBase* Sound = Talker->GetAudioComponent()->Sound;
		return Sound ? Sound->GetName() : FString();
	}

	/** Zone 2's altar ring_statue_2 (far below), whose Interact All Shards the ring piece's section binds. */
	AWasamiRingStatue* SpawnRingStatue(UWorld* World)
	{
		AWasamiRingStatue* Statue = World->SpawnActor<AWasamiRingStatue>(FVector(0., 0., -40000.), FRotator::ZeroRotator);
		Statue->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("ring_statue_2")));
		return Statue;
	}

	/** The enemies of class T in play (of T itself when bExact). */
	template <typename T>
	TArray<T*> Alive(UWorld* World, bool bExact)
	{
		TArray<T*> Found;
		for (TActorIterator<T> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed() && (!bExact || It->GetClass() == T::StaticClass()))
			{
				Found.Add(*It);
			}
		}
		return Found;
	}

	/** The player walking into the trigger box of that name. */
	void Walk(UWorld* World, const TCHAR* Name)
	{
		if (AWasamiTriggerBox* Box = Cast<AWasamiTriggerBox>(AWasamiZoneFlow::FindSource(World, Name)))
		{
			Box->NotifyPlayerOverlap(true);
		}
	}

	/**
	 * A game mode (writing to the test slot) that opens at Checkpoint. Given a zone's level name, the mode takes the
	 * world for it and spawns the zone's flow as it begins play.
	 */
	AWasamiGameMode* SpawnMode(UWorld* World, int32 Checkpoint, const TCHAR* LevelName = nullptr)
	{
		UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
		Save->Hospital.LevelCheckpoint = Checkpoint;
		UGameplayStatics::SaveGameToSlot(Save, FlowTestSlotName, UWasamiSaveGame::UserIndex);
		AWasamiGameMode* Mode = World->SpawnActorDeferred<AWasamiGameMode>(AWasamiGameMode::StaticClass(), FTransform::Identity);
		Mode->SaveSlotName = FlowTestSlotName;
		if (LevelName)
		{
			Mode->LevelName = LevelName;
		}
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
	ABlockingVolume* AmbulanceRear = SpawnBlocker(World, TEXT("BlockingVolume_Ambulance_3"), ECollisionEnabled::NoCollision);
	AWasamiShard* Shard = World->SpawnActor<AWasamiShard>(FVector(0., 0., -90000.), FRotator::ZeroRotator);
	const ALevelSequenceActor* Arrival = SpawnSequence(World, TEXT("06_Hospital_Zone01_ElevatorArrive"), 14.1);
	const ALevelSequenceActor* TakeOff = SpawnSequence(World, TEXT("06_Hospital_Zone1_AmbulanceTakeOff"), 13.9);
	const ALevelSequenceActor* Event06 = SpawnSequence(World, TEXT("06_Hospital_Zone1_06Event"), 10.53);
	// 06_CineCamera, the parking lot scene's camera (a plain one here: the flow only makes it the view target).
	AActor* CineCamera = World->SpawnActor<ACameraActor>(FVector(0., 0., -30000.), FRotator::ZeroRotator);
	CineCamera->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("06_CineCamera")));
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
	ATriggerVolume* NurseLiftVolume = SpawnZone1NursePlaces(World);
	AWasamiGarageLiftZone1Special* GarageLift = World->SpawnActor<AWasamiGarageLiftZone1Special>(FVector(0., 0., -120000.),
		FRotator::ZeroRotator);
	GarageLift->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("hospital_garage_lift_anim_Anim_2")));
	// BP_06_MusicPlayer_2, placed faded out as the level has it: the sections take the music away and give it back.
	AWasamiMusicPlayer* Music = SpawnMusicPlayer<AWasamiMusicPlayer>(World, AWasamiZone1Flow::MusicPlayerSource, true);
	const AWasamiTestBierceTalk* Talker = SpawnTalker(World);

	AWasamiGameMode* Mode = SpawnMode(World, 4);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 1);
	if (!TestNotNull(TEXT("Zone 1's flow"), Cast<AWasamiZone1Flow>(Flow)))
	{
		return false;
	}
	TestEqual(TEXT("4: the lift arrives"), Flow->GetSection(), FName(TEXT("04_Start")));
	TestTrue(TEXT("and does so in silence"), Music->bFadeOut);
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
	// The announcement is the flow's own PlaySound2D; Bierce waits it out (13 s), and says nothing meanwhile.
	TestEqual(TEXT("Bierce silent over the announcement"), Spoken(Talker), FString());

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
	// A second after the doors, Bierce's line on them; the announcement's own follows 13 s after the intercom.
	TestEqual(TEXT("nothing said in that second"), Spoken(Talker), FString());
	Advance(Wrapper, AWasamiZone1Flow::DoorBreakLineDelay + 0.1f);
	TestEqual(TEXT("04_DoorBreak's line"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_10")));
	Advance(Wrapper, AWasamiZone1Flow::IntercomLineDelay - AWasamiZone1Flow::DoorBreakLineDelay);
	TestEqual(TEXT("04_Intercom's line, 13 s on"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_09")));
	Walk(World, TEXT("BP_04_Trigger_Maze"));
	TestEqual(TEXT("05_Persistent"), Flow->GetSection(), FName(TEXT("05_Persistent")));
	TestFalse(TEXT("the maze's music comes in"), Music->bFadeOut);
	TestEqual(TEXT("checkpoint 5 saved"), SavedCheckpoint(), 5);
	TestEqual(TEXT("the shards wanted"), Objective(Mode), FString(TEXT("COLLECT ALL SHARDS")));
	// Spawn Nurses: a nurse at each of NurseSpawn_3, _1 and _2, turned as the point is.
	const TArray<AWasamiEnemy*> MazeNurses = Alive<AWasamiEnemy>(World, true);
	TestEqual(TEXT("three nurses in the maze"), MazeNurses.Num(), 3);
	for (const AWasamiEnemy* Nurse : MazeNurses)
	{
		const AActor* Point = nullptr;
		for (const TCHAR* Name : {TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2"), TEXT("NurseSpawn_3")})
		{
			const AActor* Candidate = AWasamiZoneFlow::FindSource(World, Name);
			if (Candidate && Candidate->GetActorLocation().Equals(Nurse->GetActorLocation(), 0.01))
			{
				Point = Candidate;
			}
		}
		TestNotNull(TEXT("each at a spawn point"), Point);
		TestTrue(TEXT("with CanSpawn"), Nurse->bCanSpawn);
		TestEqual(TEXT("turned as the point"), Nurse->GetActorRotation().Yaw, 90., 1e-3);
	}
	// Setup Nurse Bierce Quips: each nurse's CloseBy reaches Bierce Nurse Quip, which speaks one first chase in five
	// through the cue that picks the remark. Counted over a hundred, so that neither never nor always would pass.
	int32 Quips = 0;
	FString Quip;
	for (int32 Try = 0; Try < 100; ++Try)
	{
		Talker->GetAudioComponent()->SetSound(nullptr);
		MazeNurses[0]->OnCloseBy.Broadcast();
		if (const FString Said = Spoken(Talker); !Said.IsEmpty())
		{
			++Quips;
			Quip = Said;
		}
	}
	TestEqual(TEXT("a nurse coming close is remarked on, through the cue"), Quip,
		FString(TEXT("Bierce_TormentTherapy_Gameplay")));
	TestTrue(*FString::Printf(TEXT("but not every time (%d of 100)"), Quips), Quips < 100);

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
	TestTrue(TEXT("and the music goes"), Music->bFadeOut);
	TestEqual(TEXT("the maze's nurses removed"), Alive<AWasamiEnemy>(World, false).Num(), 0);
	TestEqual(TEXT("to the parking lot"), Objective(Mode), FString(TEXT("REACH THE PARKING LOT")));
	TestTrue(TEXT("the barrier broken"), !BarrierRef.IsValid() || BarrierRef->IsActorBeingDestroyed());
	TestFalse(TEXT("the arrow off the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8002f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("at the parking lot's trigger"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("06_CutsceneStart")));

	// The parking lot's scene: 06_Hospital_Zone1_06Event plays and nothing of 06 comes until it is over. The view over
	// to 06_CineCamera and the skip screen want a local player controller, which a test world cannot make (a bare one
	// sends the engine's SetViewTarget into an endless ClientSetViewTarget: .claude/references/troubleshooting.md), so
	// they are for the PIE run to see.
	TestNull(TEXT("no fade yet"), MadePlayer(World, TEXT("Ballroom_Event_Fade")));
	// The scene raises bFadeOut itself, which is seen by dropping it first (the section before raised it).
	Music->bFadeOut = false;
	Walk(World, TEXT("06_CutsceneStart"));
	TestEqual(TEXT("the parking lot's scene"), Flow->GetSection(), FName(TEXT("05_ParkingLotCutscene")));
	TestTrue(TEXT("which takes the music away"), Music->bFadeOut);
	TestTrue(TEXT("it plays"), Event06->GetSequencePlayer() && Event06->GetSequencePlayer()->IsPlaying());
	TestNull(TEXT("no fade while it runs"), MadePlayer(World, TEXT("Ballroom_Event_Fade")));
	TestEqual(TEXT("and no nurse of 06"), Alive<AWasamiEnemy06Chase>(World, true).Num(), 0);

	// 06_Transition, when it is over: 06, under the fade (Ballroom_Event_Fade at twice its rate).
	Advance(Wrapper, 10.53f + 0.2f);
	TestFalse(TEXT("the scene over"), Event06->GetSequencePlayer() && Event06->GetSequencePlayer()->IsPlaying());
	TestEqual(TEXT("06"), Flow->GetSection(), FName(TEXT("06_Start")));
	TestFalse(TEXT("the parking lot gives it back"), Music->bFadeOut);
	const ULevelSequencePlayer* Fade = MadePlayer(World, TEXT("Ballroom_Event_Fade"));
	TestTrue(TEXT("the fade plays"), Fade && Fade->IsPlaying());
	TestEqual(TEXT("at twice its rate"), Fade ? Fade->GetPlayRate() : 0.f, AWasamiZone1Flow::TransitionFadeRate);
	TestEqual(TEXT("to the tunnel"), Objective(Mode), FString(TEXT("REACH THE TUNNEL")));
	// Spawn Nurses_06: the parking lot's two, given the doors' place.
	const TArray<AWasamiEnemy06Chase*> Nurses06 = Alive<AWasamiEnemy06Chase>(World, true);
	TestEqual(TEXT("two nurses chase from 06"), Nurses06.Num(), 2);
	TestEqual(TEXT("and no other"), Alive<AWasamiEnemy>(World, false).Num(), 2);
	for (const AWasamiEnemy06Chase* Nurse : Nurses06)
	{
		TestTrue(TEXT("told where the doors are"),
			Nurse->DoorLocation && Nurse->DoorLocation == AWasamiZoneFlow::FindSource(World, TEXT("DoorLocation")));
		TestFalse(TEXT("not at the doors yet"), Nurse->bAttackDoor);
	}
	// A nurse in TriggerVolume_1 keeps the garage lift down for good; anything else does not.
	NurseLiftVolume->OnActorBeginOverlap.Broadcast(NurseLiftVolume, Burst);
	TestFalse(TEXT("no nurse near the lift"), GarageLift->bNurseNear);
	if (Nurses06.Num() > 0)
	{
		NurseLiftVolume->OnActorBeginOverlap.Broadcast(NurseLiftVolume, Nurses06[0]);
	}
	TestTrue(TEXT("a nurse near the lift"), GarageLift->bNurseNear);
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
	for (const AWasamiEnemy06Chase* Nurse : Nurses06)
	{
		TestTrue(TEXT("the nurses stab at them"), Nurse->bAttackDoor);
	}
	Advance(Wrapper, AWasamiZone1Flow::DoorsBreakSeconds - 0.5f);
	TestTrue(TEXT("still at 24.5 s"), Collides(Doors));
	for (const AWasamiEnemy06Chase* Nurse : Nurses06)
	{
		TestTrue(FString::Printf(TEXT("stab after stab (%d)"), Nurse->GetDoorAttacks()), Nurse->GetDoorAttacks() >= 10);
	}
	TestFalse(TEXT("the burst asleep"), Burst->GetParticleSystemComponent()->IsActive());
	Advance(Wrapper, 0.6f);
	TestFalse(TEXT("broken in by 25 s"), Collides(Doors));
	TestTrue(TEXT("the burst woken"), Burst->GetParticleSystemComponent()->IsActive());
	for (const AWasamiEnemy06Chase* Nurse : Nurses06)
	{
		TestFalse(TEXT("the nurses stop stabbing"), Nurse->bAttackDoor);
	}
	Advance(Wrapper, 0.2f);
	TestTrue(TEXT("the doors gone"), !TunnelDoorsRef.IsValid() || TunnelDoorsRef->IsActorBeingDestroyed());

	// The ambulance's roof saves 7 and the ambulance leaves 1 s on (Zone 2 opens 10.5 s on, which a test world does not
	// go on to).
	Walk(World, TEXT("TriggerBox_06_AmbulanceTop"));
	TestEqual(TEXT("06_ReachAmbulance"), Flow->GetSection(), FName(TEXT("06_ReachAmbulance")));
	TestTrue(TEXT("the ambulance takes the music away"), Music->bFadeOut);
	TestEqual(TEXT("checkpoint 7 saved"), SavedCheckpoint(), 7);
	TestEqual(TEXT("good luck"), Objective(Mode), FString(TEXT("GOOD LUCK")));
	TestTrue(TEXT("the ambulance's sides block"), Collides(AmbulanceSide));
	// Not the wall behind the player: the moving fence would land on the capsule and push the player off the roof.
	TestFalse(TEXT("but not the wall behind the player"), Collides(AmbulanceRear));
	TestEqual(TEXT("the nurses removed"), Alive<AWasamiEnemy>(World, false).Num(), 0);
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
	SpawnTriggers(World, {TEXT("Trigger_Arrive_CaptureScene"), TEXT("Trigger_Cell_Spikes"), TEXT("BP_MiniBoss_Trigger"),
		TEXT("Miniboss_BierceTalk"), TEXT("Trigger_MazeStart"), TEXT("Trigger_Miniboss_BehindMatron")});
	SpawnZone2NursePlaces(World);
	AActor* Orb = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity);
	Orb->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("ring_statue_orb_5")));
	SpawnRingStatue(World);
	// Where Postmaze Transition spawns the secret file (far below, turned as the level's is).
	const FTransform FileAt(FRotator(0., -90., 0.), FVector(0., 3000., -40000.));
	ATargetPoint* FilePoint = World->SpawnActor<ATargetPoint>(FileAt.GetLocation(), FileAt.Rotator());
	FilePoint->Tags.Add(AWasamiZoneFlow::SourceTag(AWasamiZone2Flow::PostmazeFilePoint));
	// The scenes' lengths, as _sequences has them: the ambulance's arrival, the capture and the cell.
	const float ArrivalSeconds = 6.7667f;
	const float CaptureSeconds = 26.2333f;
	const float CellSeconds = 74.0667f;
	const ALevelSequenceActor* Arrival = SpawnSequence(World, TEXT("06_Hospital_Zone2_AmbulanceArrive1_2"), ArrivalSeconds);
	const ALevelSequenceActor* Capture = SpawnSequence(World, TEXT("06_Hospital_Zone2_Capture"), CaptureSeconds);
	const ALevelSequenceActor* Cell = SpawnSequence(World, TEXT("06_Hospital_Zone2_Cell"), CellSeconds);
	const ALevelSequenceActor* Spikes = SpawnSequence(World, TEXT("06_Hospital_Zone2_Spikes"), 70.);
	const ALevelSequenceActor* DoorPicked = SpawnSequence(World, TEXT("06_Hospital_Zone2_Cell_DoorPicked"), 4.2667);
	AWasamiDoorBreak* DoorBreak = World->SpawnActorDeferred<AWasamiDoorBreak>(AWasamiDoorBreak::StaticClass(), FTransform::Identity);
	DoorBreak->ProgressSpeed = 3.f;
	DoorBreak->FinishSpawning(FTransform::Identity);
	DoorBreak->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_Hospital_DoorBreak_2")));
	// The blocker at the ambulance's front, which the arrival's end destroys.
	const TWeakObjectPtr<ABlockingVolume> Blocker(SpawnBlocker(World, TEXT("Ambulance_Arrive_Blockers4"), ECollisionEnabled::QueryAndPhysics));
	// A sentry the level places (its BeginPlay is empty: it stays without CanSpawn), far below.
	AWasamiEnemySentry* Sentry = World->SpawnActor<AWasamiEnemySentry>(FVector(0., -5000., -40000.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("a sentry"), Sentry) || !TestNotNull(TEXT("with its cone"), Sentry->GetViewcone()))
	{
		return false;
	}
	Sentry->Offset = 10.f;
	const TWeakObjectPtr<AWasamiEnemySentry> WeakSentry(Sentry);
	// The Matron the level places, with her two cones, far below too.
	AWasamiMatron* Matron = World->SpawnActor<AWasamiMatron>(FVector(0., -8000., -40000.), FRotator::ZeroRotator);
	AWasamiViewcone* LongCone = World->SpawnActor<AWasamiViewconeMatronLong>(FVector(0., -8000., -39000.), FRotator::ZeroRotator);
	AWasamiViewcone* ShortCone = World->SpawnActor<AWasamiViewconeMatronShort>(FVector(0., -8000., -39200.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the Matron"), Matron) || !TestNotNull(TEXT("her long cone"), LongCone) || !TestNotNull(TEXT("her short cone"), ShortCone))
	{
		return false;
	}
	Matron->Tags.Add(AWasamiZoneFlow::SourceTag(AWasamiZone2Flow::Matron));
	Matron->LongCone = LongCone;
	Matron->ShortCone = ShortCone;
	const TWeakObjectPtr<AWasamiMatron> WeakMatron(Matron);
	// BP_06_MusicPlayer_Zone2_2, which the level places without bFadeOut: this zone opens with its track on, and the
	// cell's scene ducks it and gives it back itself (Regular Music's FadeIn).
	const AWasamiMusicPlayerZone2* Music = SpawnMusicPlayer<AWasamiMusicPlayerZone2>(World,
		AWasamiZone2Flow::MusicPlayerSource, false);
	AWasamiTestBierceTalk* Talker = SpawnTalker(World);
	// Two of the maze's lifts, whose Player Overlap Setup Bierce Lift Quip binds (far below and apart).
	TArray<AWasamiLift*> Lifts;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Lifts.Add(World->SpawnActor<AWasamiLift>(FVector(0., 2000. + 1000. * Index, -40000.), FRotator::ZeroRotator));
	}
	// The announcement behind the Matron, as the level places it: an AmbientSound of the original's wave, its
	// AudioComponent left off until Miniboss_BehindMatron plays it.
	const FTransform IntercomAt(FVector(0., -8000., -39500.));
	AAmbientSound* Intercom = World->SpawnActorDeferred<AAmbientSound>(AAmbientSound::StaticClass(), IntercomAt);
	Intercom->GetAudioComponent()->bAutoActivate = false;
	Intercom->GetAudioComponent()->SetSound(LoadObject<USoundBase>(nullptr,
		TEXT("/Game/DD/Audio/06_Hospital/Nurse_Hospital_Zone01_Event_48_Intercom")));
	Intercom->FinishSpawning(IntercomAt);
	Intercom->Tags.Add(AWasamiZoneFlow::SourceTag(AWasamiZone2Flow::BehindMatronIntercom));
	if (!TestNotNull(TEXT("the intercom's wave"), Intercom->GetAudioComponent()->GetSound()))
	{
		return false;
	}

	AWasamiGameMode* Mode = SpawnMode(World, 7);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	if (!TestNotNull(TEXT("Zone 2's flow"), Cast<AWasamiZone2Flow>(Flow)))
	{
		return false;
	}
	// Arrive Event: the ambulance drives in 0.3 s on, with the player left to walk about (the arrival takes neither the
	// view nor the input), and its end takes the blocker at its front away.
	TestEqual(TEXT("7: the ambulance's arrival"), Flow->GetSection(), FName(TEXT("Arrive Event")));
	TestFalse(TEXT("which waits its 0.3 s"), Arrival->GetSequencePlayer() && Arrival->GetSequencePlayer()->IsPlaying());
	Advance(Wrapper, AWasamiZone2Flow::ArriveSequenceDelay + 0.05f);
	TestTrue(TEXT("then drives in"), Arrival->GetSequencePlayer() && Arrival->GetSequencePlayer()->IsPlaying());
	TestTrue(TEXT("its front blocker still there"), Blocker.IsValid() && !Blocker->IsActorBeingDestroyed());
	Advance(Wrapper, ArrivalSeconds);
	TestEqual(TEXT("arrived: Escape_AmbulanceArrive"), Flow->GetSection(), FName(TEXT("Escape_AmbulanceArrive")));
	TestTrue(TEXT("its front blocker gone"), !Blocker.IsValid() || Blocker->IsActorBeingDestroyed());

	// The capture, walked into: the player's input goes and the scene plays. Its view of CineCameraActor_2 and its skip
	// screen want a local player controller, which a test world cannot make (a bare one sends the engine's SetViewTarget
	// into an endless ClientSetViewTarget: .claude/references/troubleshooting.md), so they are for the PIE run to see.
	Walk(World, TEXT("Trigger_Arrive_CaptureScene"));
	TestEqual(TEXT("caught"), Flow->GetSection(), FName(TEXT("Arrive_CaptureCutscene")));
	TestTrue(TEXT("the capture plays"), Capture->GetSequencePlayer() && Capture->GetSequencePlayer()->IsPlaying());
	Advance(Wrapper, CaptureSeconds);

	// The cell's scene, 1 s after the capture: it moves the cell's own doors, switch and ceiling, and the player into it
	// (PlayerStart_Cell, which this world has no more than the other starts), with the view left on the capture's camera.
	TestEqual(TEXT("the capture over: Cell Cutscene Start"), Flow->GetSection(), FName(TEXT("Cell Cutscene Start")));
	TestFalse(TEXT("which waits its 1 s"), Cell->GetSequencePlayer() && Cell->GetSequencePlayer()->IsPlaying());
	TestTrue(TEXT("the music untouched until then"), Music->GetLastRegularFadeInDuration() < 0.f);
	Advance(Wrapper, AWasamiZone2Flow::CellSequenceDelay + 0.05f);
	TestTrue(TEXT("then the cell"), Cell->GetSequencePlayer() && Cell->GetSequencePlayer()->IsPlaying());
	TestEqual(TEXT("with the music ducked under it"), Music->GetLastRegularFadeInVolume(), AWasamiZone2Flow::CellMusicVolume);
	TestEqual(TEXT("over half a second"), Music->GetLastRegularFadeInDuration(), AWasamiZone2Flow::CellMusicFadeIn);
	Advance(Wrapper, CellSeconds);
	TestEqual(TEXT("out of the cell's scene"), Flow->GetSection(), FName(TEXT("Cell Cutscene Finished")));
	TestEqual(TEXT("the music back up"), Music->GetLastRegularFadeInVolume(), AWasamiZone2Flow::CellEndMusicVolume);
	TestEqual(TEXT("over 2.5 s"), Music->GetLastRegularFadeInDuration(), AWasamiZone2Flow::CellEndMusicFadeIn);
	TestFalse(TEXT("and never faded out"), Music->bFadeOut);
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
	// A second after the cell's scene, Bierce's line on it, and only then his trigger down the corridor.
	TestEqual(TEXT("Bierce silent in that second"), Spoken(Talker), FString());
	Advance(Wrapper, AWasamiZone2Flow::CellLineDelay + 0.1f);
	TestEqual(TEXT("the cell's line"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_17")));
	Talker->GetAudioComponent()->SetSound(nullptr);
	Walk(World, TEXT("Miniboss_BierceTalk"));
	TestEqual(TEXT("Bierce's trigger, bound 1 s on"), Flow->GetSection(), FName(TEXT("Miniboss_BierceTalk")));
	TestEqual(TEXT("its line"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_19")));
	Talker->GetAudioComponent()->SetSound(nullptr);
	TestFalse(TEXT("the sentry not looking in the cell"), Sentry->GetViewcone()->IsInitialized());
	TestFalse(TEXT("nor the Matron"), Matron->IsActivated() || LongCone->IsInitialized() || ShortCone->IsInitialized());

	Walk(World, TEXT("BP_MiniBoss_Trigger"));
	TestEqual(TEXT("the Matron's corridor"), Flow->GetSection(), FName(TEXT("Miniboss Transition ")));
	TestTrue(TEXT("Activate MiniBoss Enemies: the sentry looks"), Sentry->GetViewcone()->IsInitialized());
	TestEqual(TEXT("its cone waiting its Offset"), Sentry->GetViewcone()->Offset, 10.f);
	TestTrue(TEXT("and then the Matron: her Switch"), Matron->IsActivated());
	TestTrue(TEXT("her cones hers"), LongCone->GetOwner() == Matron && ShortCone->GetOwner() == Matron);
	TestTrue(TEXT("and looking"), LongCone->IsInitialized() && ShortCone->IsInitialized());
	TestEqual(TEXT("the section's own line, the same one"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_19")));
	TestEqual(TEXT("checkpoint 8 saved"), SavedCheckpoint(), 8);
	TestEqual(TEXT("past the nurses, with the original's space"), Objective(Mode), FString(TEXT("Get past the nurses ")));
	TestTrue(TEXT("the arrow clear"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(0.f, 0.f, 0.f, 0.f)));
	TestNull(TEXT("and without a target"), Flow->GetArrowTarget());

	// Behind her: the announcement, which is the level's own AmbientSound and not a line of Bierce's.
	TestFalse(TEXT("the intercom silent until then"), Intercom->GetAudioComponent()->IsPlaying());
	Walk(World, TEXT("Trigger_Miniboss_BehindMatron"));
	TestEqual(TEXT("behind the Matron"), Flow->GetSection(), FName(TEXT("Miniboss_BehindMatron")));
	TestTrue(TEXT("the announcement plays"), Intercom->GetAudioComponent()->IsPlaying());
	Talker->GetAudioComponent()->SetSound(nullptr);

	Walk(World, TEXT("Trigger_MazeStart"));
	TestEqual(TEXT("checkpoint 9 saved"), SavedCheckpoint(), 9);
	TestEqual(TEXT("the maze's shards"), Objective(Mode), FString(TEXT("COLLECT ALL SHARDS")));
	TestTrue(TEXT("the arrow on the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("the sentry removed"), !WeakSentry.IsValid() || WeakSentry->IsActorBeingDestroyed());
	TestTrue(TEXT("the Matron removed (tagged Enemy)"), !WeakMatron.IsValid() || WeakMatron->IsActorBeingDestroyed());
	// Spawn Nurses: a Zone 2 nurse at each of NurseSpawn_4, _1 and _2, turned as the point is.
	const TArray<AWasamiEnemy*> MazeNurses = Alive<AWasamiEnemy>(World, false);
	TestEqual(TEXT("three nurses in the maze"), MazeNurses.Num(), 3);
	for (const AWasamiEnemy* Nurse : MazeNurses)
	{
		const AActor* Point = nullptr;
		for (const TCHAR* Name : {TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2"), TEXT("NurseSpawn_4")})
		{
			const AActor* Candidate = AWasamiZoneFlow::FindSource(World, Name);
			if (Candidate && Candidate->GetActorLocation().Equals(Nurse->GetActorLocation(), 0.01))
			{
				Point = Candidate;
			}
		}
		TestNotNull(TEXT("each at a spawn point"), Point);
		TestTrue(TEXT("of the Zone 2 kind"), Nurse->GetClass() == AWasamiEnemyZone2::StaticClass());
		TestTrue(TEXT("with CanSpawn"), Nurse->bCanSpawn);
		TestEqual(TEXT("turned as the point"), Nurse->GetActorRotation().Yaw, 90., 1e-3);
	}

	// A second after the maze starts: Bierce's line on it — the hospital's one line spoken with Attenuate? on, so that
	// it is heard from where the talker stands — and Setup Bierce Lift Quip on the lifts.
	TestEqual(TEXT("Bierce silent in that second"), Spoken(Talker), FString());
	Advance(Wrapper, AWasamiZone2Flow::MazeLineDelay + 0.1f);
	TestEqual(TEXT("the maze's line"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Gameplay_08")));
	TestTrue(TEXT("heard from the talker"), Talker->GetAudioComponent()->bAllowSpatialization);
	// Bierce Lift Quip: the first lift the player steps on, a second on, and no other (its DoOnce).
	Talker->GetAudioComponent()->SetSound(nullptr);
	Lifts[0]->OnPlayerOverlap.Broadcast();
	TestEqual(TEXT("nothing said at once"), Spoken(Talker), FString());
	Advance(Wrapper, AWasamiZone2Flow::LiftQuipDelay + 0.1f);
	TestEqual(TEXT("the lifts' remark"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Gameplay_07")));
	TestFalse(TEXT("unattenuated again"), Talker->GetAudioComponent()->bAllowSpatialization);
	Talker->GetAudioComponent()->SetSound(nullptr);
	Lifts[1]->OnPlayerOverlap.Broadcast();
	Lifts[0]->OnPlayerOverlap.Broadcast();
	Advance(Wrapper, AWasamiZone2Flow::LiftQuipDelay + 0.1f);
	TestEqual(TEXT("but the once"), Spoken(Talker), FString());

	Mode->CheckShards();
	Advance(Wrapper, 0.1f);
	TestEqual(TEXT("checkpoint 10 saved"), SavedCheckpoint(), 10);
	TestEqual(TEXT("the maze's nurses removed"), Alive<AWasamiEnemy>(World, false).Num(), 0);
	TestFalse(TEXT("the statue's orb gone"), IsValid(Orb));
	// The secret file after the maze: ID 3 at collec, with its folder.
	const TArray<AWasamiCollectable*> Files = Alive<AWasamiCollectable>(World, true);
	if (TestEqual(TEXT("one secret file"), Files.Num(), 1))
	{
		TestEqual(TEXT("ID 3"), Files[0]->ID, 3);
		TestTrue(TEXT("at collec"), Files[0]->GetActorLocation().Equals(FileAt.GetLocation(), 0.01)
			&& Files[0]->GetActorRotation().Equals(FileAt.Rotator(), 1e-3));
		TestNotNull(TEXT("with secret_file"), Files[0]->GetStaticMesh()->GetStaticMesh().Get());
	}
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("to the ring piece"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8941f, 0.f, 1.f), 1e-4f));
	// Two seconds after the maze's last shard: Bierce's line on it.
	TestEqual(TEXT("Bierce silent meanwhile"), Spoken(Talker), FString());
	Advance(Wrapper, AWasamiZone2Flow::MazeAllShardsLineDelay + 0.1f);
	TestEqual(TEXT("the line once every shard is taken"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_20")));

	// The cell's spikes kill 0.5 s after they reach the player, who is hit at once.
	Walk(World, TEXT("Trigger_Cell_Spikes"));
	TestTrue(TEXT("not at once"), Mode->IsDeathOpen());
	TestTrue(TEXT("the hit's flash"), TActorIterator<AWasamiHitFX>(World) ? true : false);
	Advance(Wrapper, AWasamiZone2Flow::SpikesDeathDelay + 0.1f);
	TestFalse(TEXT("dead 0.5 s on"), Mode->IsDeathOpen());

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowRingPieceTest, "Wasami.ZoneFlow.RingPiece",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowRingPieceTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	SpawnTriggers(World, {TEXT("Postmaze_Trigger_Garage"), TEXT("Trigger_MazeStart"), TEXT("Trigger_Miniboss_BehindMatron")});
	// The room after the maze: the altar, the piece over it, its two pink lights, and the barrier and the locked doors
	// on the way to the garage (far below and apart).
	AWasamiRingStatue* Statue = SpawnRingStatue(World);
	AWasamiRingPiece* Piece = World->SpawnActor<AWasamiRingPiece>(FVector(0., 0., -39000.), FRotator::ZeroRotator);
	Piece->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_08_RingPiece_NoPickup_5")));
	const TWeakObjectPtr<AWasamiRingPiece> PieceRef(Piece);
	TArray<APointLight*> Lights;
	for (const TCHAR* Name : {TEXT("PointLight202"), TEXT("PointLight201_6")})
	{
		APointLight* Light = World->SpawnActor<APointLight>(FVector(0., 1000., -39000.), FRotator::ZeroRotator);
		Light->Tags.Add(AWasamiZoneFlow::SourceTag(Name));
		Lights.Add(Light);
	}
	AWasamiZoneBarrier* Barrier = World->SpawnActor<AWasamiZoneBarrier>(FVector(0., 3000., -40000.), FRotator::ZeroRotator);
	Barrier->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_ZoneBarrier_2")));
	const TWeakObjectPtr<AWasamiZoneBarrier> BarrierRef(Barrier);
	AWasamiDoubleDoors* Doors = World->SpawnActor<AWasamiDoubleDoors>(FVector(0., 6000., -40000.), FRotator::ZeroRotator);
	Doors->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_DoubleDoors2")));
	Doors->bLocked = true;
	const AWasamiTestBierceTalk* Talker = SpawnTalker(World);

	AWasamiGameMode* Mode = SpawnMode(World, 10);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	if (!TestNotNull(TEXT("Zone 2's flow"), Cast<AWasamiZone2Flow>(Flow)))
	{
		return false;
	}
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("to the ring piece"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));
	TestTrue(TEXT("the arrow at the piece"), Flow->GetArrowTarget() == Piece);

	// The altar clicked with no shard left: Collected Ring Piece puts up the screen, whose Close is Ring Piece Collect .
	IWasamiInteractable::Execute_InteractWithObject(Statue, nullptr);
	TestEqual(TEXT("Collected Ring Piece"), Flow->GetSection(), FName(TEXT("Collected Ring Piece")));
	UWasamiRingPieceWidget* Screen = nullptr;
	for (TObjectIterator<UWasamiRingPieceWidget> It; It; ++It)
	{
		if (It->GetWorld() == World && It->OnClose.IsBound())
		{
			Screen = *It;
		}
	}
	if (!TestNotNull(TEXT("the ring piece's screen, its Close bound"), Screen))
	{
		return false;
	}
	TestTrue(TEXT("the barrier stands meanwhile"), BarrierRef.IsValid() && !BarrierRef->IsActorBeingDestroyed());
	Screen->Begin();
	Screen->Advance(1.f);
	Screen->PressClose();
	Screen->Advance(UWasamiRingPieceWidget::ReverseDelay + 0.01f);
	Screen->Advance(UWasamiRingPieceWidget::LeaveDelay + 0.01f);
	TestTrue(TEXT("closed"), Screen->HasClosed());

	TestEqual(TEXT("Ring Piece Collect "), Flow->GetSection(), FName(TEXT("Ring Piece Collect ")));
	for (const APointLight* Light : Lights)
	{
		TestFalse(TEXT("the altar's light off"), Light->GetLightComponent()->IsVisible());
	}
	TestTrue(TEXT("the barrier broken"), !BarrierRef.IsValid() || BarrierRef->IsActorBeingDestroyed());
	TestFalse(TEXT("the arrow off the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("its colour"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.8941f, 0.f, 1.f), 1e-4f));
	TestTrue(TEXT("at the garage"), Flow->GetArrowTarget() && Flow->GetArrowTarget() == AWasamiZoneFlow::FindSource(World, TEXT("Postmaze_Trigger_Garage")));
	TestEqual(TEXT("to the garage"), Objective(Mode), FString(TEXT("HEAD TOWARDS THE GARAGE")));
	TestTrue(TEXT("the piece taken"), !PieceRef.IsValid() || PieceRef->IsActorBeingDestroyed());
	TestFalse(TEXT("the doors to the garage unlocked"), Doors->bLocked);

	// The garage's trigger is bound 1 s on (walking through it before would use it up).
	const AWasamiTriggerBox* Garage = Cast<AWasamiTriggerBox>(AWasamiZoneFlow::FindSource(World, TEXT("Postmaze_Trigger_Garage")));
	TestFalse(TEXT("the garage not bound at once"), Garage->OnTrigger.IsBound());
	Advance(Wrapper, AWasamiZone2Flow::GarageBindDelay - 0.1f);
	TestFalse(TEXT("nor before 1 s"), Garage->OnTrigger.IsBound());
	TestEqual(TEXT("nor has Bierce spoken"), Spoken(Talker), FString());
	Advance(Wrapper, 0.2f);
	TestTrue(TEXT("bound by 1 s"), Garage->OnTrigger.IsBound());
	TestEqual(TEXT("with his line on the piece"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_21")));
	Walk(World, TEXT("Postmaze_Trigger_Garage"));
	TestEqual(TEXT("the garage"), Flow->GetSection(), FName(TEXT("Postmaze_Trigger_Garage")));
	// And his line in the garage, a second on.
	Advance(Wrapper, AWasamiZone2Flow::GarageLineDelay + 0.1f);
	TestEqual(TEXT("the garage's line"), Spoken(Talker), FString(TEXT("Bierce_TormentTherapy_Event_22")));

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowEscapeTest, "Wasami.ZoneFlow.Escape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowEscapeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	SpawnTriggers(World, {TEXT("Postmaze_Trigger_Garage"), *AWasamiZone2Flow::EscapeTrigger.ToString()});
	SpawnRingStatue(World);
	// The garage's portal, locked as the level places it (far below).
	AWasamiPortal* Portal = World->SpawnActorDeferred<AWasamiPortal>(AWasamiPortal::StaticClass(), FTransform::Identity);
	Portal->bLocked = true;
	Portal->bMaskedPortalMaterial = true;
	Portal->FinishSpawning(FTransform(FVector(0., 0., -40000.)));
	Portal->Tags.Add(AWasamiZoneFlow::SourceTag(AWasamiZone2Flow::GaragePortal));
	const AWasamiMusicPlayerZone2* Music = SpawnMusicPlayer<AWasamiMusicPlayerZone2>(World,
		AWasamiZone2Flow::MusicPlayerSource, false);

	AWasamiGameMode* Mode = SpawnMode(World, 10);
	AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	if (!TestNotNull(TEXT("Zone 2's flow"), Cast<AWasamiZone2Flow>(Flow)))
	{
		return false;
	}
	Advance(Wrapper, 0.02f);
	const AWasamiTriggerBox* End = Cast<AWasamiTriggerBox>(AWasamiZoneFlow::FindSource(World, AWasamiZone2Flow::EscapeTrigger));
	TestFalse(TEXT("the portal's trigger not bound before the garage"), End->OnTrigger.IsBound());

	// The garage's trigger (Ring Piece Collect  binds it): the portal opens, the arrow and the objective go to it.
	TestTrue(TEXT("Postmaze_Trigger_Garage"), Flow->CallEvent(TEXT("OnPostmazeTriggerGarage")));
	TestEqual(TEXT("the garage"), Flow->GetSection(), FName(TEXT("Postmaze_Trigger_Garage")));
	TestFalse(TEXT("the portal open"), Portal->bLocked);
	TestFalse(TEXT("the arrow off the shards"), Flow->IsArrowOnShards());
	TestTrue(TEXT("the hotel's red"), Flow->GetArrowColor().IsSet() && Flow->GetArrowColor()->Equals(FLinearColor(1.f, 0.f, 0.016666f, 1.f), 1e-4f));
	TestTrue(TEXT("at the portal"), Flow->GetArrowTarget() == Portal);
	TestEqual(TEXT("to the portal"), Objective(Mode), FString(TEXT("GET TO THE PORTAL")));
	TestTrue(TEXT("the portal's trigger bound"), End->OnTrigger.IsBound());

	// A nurse about (none is left by then in play), a death this level, and the player walking into the portal's trigger.
	AWasamiEnemySentry* Sentry = World->SpawnActor<AWasamiEnemySentry>(FVector(0., -5000., -40000.), FRotator::ZeroRotator);
	const TWeakObjectPtr<AWasamiEnemySentry> WeakSentry(Sentry);
	Mode->GetSave()->Hospital.Deaths = 2;
	const float Played = Mode->GetTime();
	TestTrue(TEXT("time played"), Played > 0.f);
	Walk(World, *AWasamiZone2Flow::EscapeTrigger.ToString());
	TestEqual(TEXT("the escape"), Flow->GetSection(), FName(TEXT("EndTrigger")));
	TestTrue(TEXT("the enemies removed"), !WeakSentry.IsValid() || WeakSentry->IsActorBeingDestroyed());
	TestTrue(TEXT("the music taken away"), Music->bFadeOut);
	TestEqual(TEXT("and faded out to be heard"), Music->GetLastFadeOutDuration(), AWasamiZone2Flow::EscapeMusicFade);

	// The hospital's Escape, which waits for that fade: the save is untouched until it ends, and the time played does
	// not grow while it runs (the counter stops at the trigger, so the level is timed to the portal).
	TestEqual(TEXT("the checkpoint kept while the fade runs"), SavedCheckpoint(), 10);
	Advance(Wrapper, AWasamiZone2Flow::EscapeMusicFade + 0.1f);

	// Then checkpoint 0 saved with the time added and the counter back to 0 (no player here, so no pause and no screen).
	const auto SavedEntry = []()
	{
		const UWasamiSaveGame* Save = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex));
		return Save ? Save->Hospital : FWasamiLevelProgress();
	};
	TestEqual(TEXT("checkpoint 0 saved"), SavedEntry().LevelCheckpoint, 0);
	TestEqual(TEXT("the time added"), SavedEntry().Time, Played, 1e-4f);
	TestEqual(TEXT("the deaths kept"), SavedEntry().Deaths, 2);
	TestEqual(TEXT("the counter back to 0"), Mode->GetTime(), 0.f);

	// The screen's Finished → Finished Level: 1 s on the hospital's entry is emptied and written, and the title opens.
	TestFalse(TEXT("Finished Level not yet"), Mode->HasFinishedLevel());
	Mode->FinishedLevel();
	TestTrue(TEXT("Finished Level"), Mode->HasFinishedLevel());
	Advance(Wrapper, AWasamiGameMode::FinishedLevelDelay - 0.1f);
	TestEqual(TEXT("the entry kept before 1 s"), SavedEntry().Deaths, 2);
	TestTrue(TEXT("no level before 1 s"), Mode->GetLevelToOpen().IsEmpty());
	Advance(Wrapper, 0.2f);
	TestEqual(TEXT("the entry emptied at 1 s"), SavedEntry().Deaths, 0);
	TestEqual(TEXT("its time too"), SavedEntry().Time, 0.f);
	TestEqual(TEXT("then the title"), Mode->GetLevelToOpen(), FString(AWasamiGameMode::TitleLevelName));

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiZoneFlowNewStartTest, "Wasami.ZoneFlow.NewStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiZoneFlowNewStartTest::RunTest(const FString& Parameters)
{
	// Each case in a world of its own, Zone 1's flow spawned by the game mode as play begins. The test world has no
	// local player, so the title card is not made: CreateWidget reports the controller, for the new start only.
	AddExpectedError(TEXT("PlayerController_0"), EAutomationExpectedErrorFlags::Contains, 1);
	struct FCase
	{
		const TCHAR* What;
		int32 Saved;
		bool bNewStart;
	};
	const FCase Cases[] = {
		{TEXT("NEW GAME (0 saved): "), 0, true},
		{TEXT("reopened at 4 (a death, RESUME): "), 4, false},
	};
	for (const FCase& Case : Cases)
	{
		const FString What = Case.What;
		FTestWorldWrapper Wrapper;
		if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
		{
			Wrapper.ForwardErrorMessages(this);
			return false;
		}
		UWorld* World = Wrapper.GetTestWorld();
		SpawnTriggers(World, {TEXT("04_Intercom")});
		SpawnSequence(World, TEXT("06_Hospital_Zone01_ElevatorArrive"), 14.1);
		AWasamiDoubleDoors* LiftDoors = World->SpawnActor<AWasamiDoubleDoors>(FVector(0., 0., -80000.), FRotator::ZeroRotator);
		LiftDoors->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_DoubleDoors11")));
		AWasamiDoorBreak* DoorBreak = World->SpawnActor<AWasamiDoorBreak>(FVector(0., 0., -90000.), FRotator::ZeroRotator);
		DoorBreak->Tags.Add(AWasamiZoneFlow::SourceTag(TEXT("BP_06_Hospital_DoorBreak_2")));
		SpawnZone1NursePlaces(World);

		AWasamiGameMode* Mode = SpawnMode(World, Case.Saved, AWasamiGameMode::Zone1LevelName);
		const AWasamiZoneFlow* Flow = Mode->GetZoneFlow();
		TestEqual(What + TEXT("a new start"), Mode->IsNewStart(), Case.bNewStart);
		TestEqual(What + TEXT("the lift arrives"), Flow ? Flow->GetSection() : NAME_None, FName(TEXT("04_Start")));
		TestEqual(What + TEXT("4 saved"), SavedCheckpoint(), 4);
		// The player: player 0's character without being possessed (so it makes no widgets), not falling.
		AWasamiPlayerCharacter* Player = World->SpawnActor<AWasamiPlayerCharacter>(FVector(0., 0., 50000.), FRotator::ZeroRotator);
		APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!TestNotNull(*(What + TEXT("the player")), Player) || !TestNotNull(*(What + TEXT("its controller")), Controller))
		{
			return false;
		}
		Controller->SetPawn(Player);
		Player->GetCharacterMovement()->DisableMovement();
		Advance(Wrapper, 0.05f);
		TestEqual(What + TEXT("held from the next tick"), Player->bCanMove, !Case.bNewStart);
		Advance(Wrapper, AWasamiZone1Flow::InitialHoldSeconds - 0.3f);
		TestEqual(What + TEXT("still held"), Player->bCanMove, !Case.bNewStart);
		Advance(Wrapper, 0.4f);
		TestTrue(What + TEXT("free 10 s on"), Player->bCanMove);
	}

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
	SpawnZone1NursePlaces(World);
	SpawnZone2NursePlaces(World);
	SpawnRingStatue(World);
	// Zone 1's music player, which 05_Persistent gives the music back on (both zones' are in this one world, and only
	// Zone 1's sections touch theirs among the checkpoints below).
	const AWasamiMusicPlayer* Music = SpawnMusicPlayer<AWasamiMusicPlayer>(World, AWasamiZone1Flow::MusicPlayerSource, true);
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
	TestFalse(TEXT("05_Persistent gave the music back"), Music->bFadeOut);
	AWasamiGameMode* Mode = SpawnMode(World, 10);
	const AWasamiZoneFlow* Flow = AWasamiZoneFlow::SpawnFor(Mode, 2);
	TestEqual(TEXT("zone 2 at 10"), Flow ? Flow->GetSection() : NAME_None, FName(TEXT("Postmaze Transition")));
	Advance(Wrapper, 0.02f);
	TestEqual(TEXT("the ring piece the tick after"), Objective(Mode), FString(TEXT("COLLECT THE RING PIECE")));

	UGameplayStatics::DeleteGameInSlot(FlowTestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
