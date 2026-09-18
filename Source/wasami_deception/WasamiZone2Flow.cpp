#include "WasamiZone2Flow.h"

#include "Camera/CameraShakeBase.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiGameMode.h"
#include "WasamiHitFX.h"

namespace
{
	// BP_ArrowPointer's Change Color in the zone's sections.
	const FLinearColor MinibossArrow(0.f, 0.f, 0.f, 0.f);
	const FLinearColor RingPieceArrow(1.f, 0.8941f, 0.f, 1.f);

	/** Where a scene leaves an actor it moved (Sequencer makes the static ones movable to move them). */
	void Leave(AActor* Actor, const FVector& Location, const FRotator& Rotation)
	{
		if (USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr)
		{
			Root->SetMobility(EComponentMobility::Movable);
			Actor->SetActorLocationAndRotation(Location, Rotation);
		}
	}
}

// 06_Hospital_Zone2_AmbulanceArrive1's last keys (the ambulance keeps its rotation) and 06_Hospital_Zone2_Cell's.
const FVector AWasamiZone2Flow::AmbulanceArrived(-14157.71484375, -5025.021484375, 800.);
const FVector AWasamiZone2Flow::FalseCeilingOpen(-0.037109375, 864.614990234375, 0.);
const FRotator AWasamiZone2Flow::WallSwitchThrown(0., 0., 40.809776306152344);

AWasamiZone2Flow::AWasamiZone2Flow()
{
	DoorPickedShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/04_Sewer/Bossfight/BP_04_BossFight_CameraShake_Initial")));
	SpikesSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_Needle_Trap_R1_V3")));
}

void AWasamiZone2Flow::StartAt(int32 Checkpoint)
{
	// Spawn (@22328): Load Progress By Level(7, 8). 7 is the ambulance's arrival in the original (Arrive Event → the
	// capture → the cell); here the cell's scene is over already. 8 to 10 start at their player starts, which the game
	// mode has put the player at. 0 opens the entrance (the game mode opens Zone 1 instead).
	switch (Checkpoint)
	{
	case 7: SkippedScenesEnd(); OnCellCutsceneFinished(); break;
	case 8: Enter(TEXT("Miniboss Start ")); MinibossTransition(); break;
	case 9: Enter(TEXT("Maze Start")); MazeTransition(); break;
	case 10: Enter(TEXT("Postmaze Start")); PostmazeTransition(); break;
	default: break;
	}
}

void AWasamiZone2Flow::SkippedScenesEnd()
{
	// The arrival (Arrive Event plays 06_Hospital_Zone2_AmbulanceArrive1 in a packaged game) leaves the ambulance in the
	// yard, and its end (Escape_AmbulanceArrive) destroys the blocker at its front. The capture leaves nothing. The
	// cell's scene (Cell Cutscene Start, which moved the player to PlayerStart_Cell: the game mode starts them there)
	// leaves the false ceiling slid open and the wall switch thrown; what else it moves goes back as it ends.
	if (AActor* Ambulance = Source(TEXT("hospital_ambulance_new_arrive")))
	{
		Leave(Ambulance, AmbulanceArrived, Ambulance->GetActorRotation());
	}
	if (AActor* Blocker = Source(TEXT("Ambulance_Arrive_Blockers4")))
	{
		Blocker->Destroy();
	}
	if (AActor* Ceiling = Source(TEXT("hospital_zone_02_holdingCell_01_false_ceiling_11")))
	{
		Leave(Ceiling, FalseCeilingOpen, Ceiling->GetActorRotation());
	}
	if (AActor* Switch = Source(TEXT("hospital_zone_02_holdingCell_01_wall_switch_14")))
	{
		Leave(Switch, Switch->GetActorLocation(), WallSwitchThrown);
	}
}

void AWasamiZone2Flow::OnCellCutsceneFinished()
{
	Enter(TEXT("Cell Cutscene Finished"));
	// Enable Player Input (a level just opened has it). BP_06_MusicPlayer_Zone2_2's Regular Music fades in (item 19).
	// The view blends back to the player over 2 s (the scenes being left out, it is the player's already).
	PlaySequence(TEXT("06_Hospital_Zone2_Spikes"));
	EnableDoorBreak(TEXT("BP_06_Hospital_DoorBreak_2"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnCellDoorBreak));
	BindTrigger(TEXT("Trigger_Cell_Spikes"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnSpikesDeath));
	BindTrigger(TEXT("BP_MiniBoss_Trigger"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMinibossTriggerTransition));
	After(1.f, [this]()
	{
		// Bierce_TormentTherapy_Event_17 (item 20).
		BindTrigger(TEXT("Miniboss_BierceTalk"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMinibossBierceTalk));
	});
}

void AWasamiZone2Flow::OnCellDoorBreak()
{
	Enter(TEXT("Cell_DoorBreak"));
	PlaySequence(TEXT("06_Hospital_Zone2_Cell_DoorPicked"));
	PlayCameraShake(DoorPickedShakeClass);
}

void AWasamiZone2Flow::OnSpikesDeath()
{
	Enter(TEXT("Spikes_Death"));
	GetWorld()->SpawnActor<AWasamiHitFX>(AWasamiHitFX::StaticClass(), FTransform::Identity);
	if (USoundBase* Sound = SpikesSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
	After(SpikesDeathDelay, [this]()
	{
		if (Mode)
		{
			Mode->DeathEvent(UGameplayStatics::GetPlayerCharacter(this, 0));
		}
	});
}

void AWasamiZone2Flow::OnMinibossBierceTalk()
{
	Enter(TEXT("Miniboss_BierceTalk"));
	// Bierce_TormentTherapy_Event_19 (item 20).
}

void AWasamiZone2Flow::OnMinibossTriggerTransition()
{
	Enter(TEXT("Miniboss_Trigger_Transition"));
	MinibossTransition();
	SaveCheckpoint(8);
}

void AWasamiZone2Flow::MinibossTransition()
{
	Enter(TEXT("Miniboss Transition "));
	// The achievement's 06_NurseAlert set to 0 (achievements are not in this game). Activate MiniBoss Enemies (item 11).
	// Bierce_TormentTherapy_Event_19 (item 20).
	SetArrowShards(false);
	SetArrowColor(MinibossArrow);
	SetArrowTarget(nullptr);
	// The original's text ends in a space.
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveGetPastNurses", "Get past the nurses "));
	BindTrigger(TEXT("Trigger_MazeStart"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMazeTriggerStart));
	BindTrigger(TEXT("Trigger_Miniboss_BehindMatron"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMinibossBehindMatron));
}

void AWasamiZone2Flow::OnMinibossBehindMatron()
{
	Enter(TEXT("Miniboss_BehindMatron"));
	// Nurse_Hospital_Zone01_Event_48_Intercom_2's AudioComponent plays (item 20).
}

void AWasamiZone2Flow::OnMazeTriggerStart()
{
	Enter(TEXT("Maze Trigger Start"));
	MazeTransition();
	SaveCheckpoint(9);
	// 1 s on: Bierce_TormentTherapy_Gameplay_08 and Setup Bierce Lift Quip, which binds every AWasamiLift's
	// OnPlayerOverlap (BP_06_LiftBase's Player Overlap) to Bierce Lift Quip (item 20).
}

void AWasamiZone2Flow::MazeTransition()
{
	Enter(TEXT("Maze Transition "));
	// Whether the 06_NurseAlert achievement was earned (achievements are not in this game).
	RemoveAllEnemies(GetWorld());
	SetArrowShards(true);
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveCollectAllShards", "COLLECT ALL SHARDS"));
	BindAllShardsCollected(GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMazeAllShards));
	// Spawn Nurses (item 7).
}

void AWasamiZone2Flow::OnMazeAllShards()
{
	Enter(TEXT("Maze All Shards"));
	SaveCheckpoint(10);
	PostmazeTransition();
	// 2 s on: Bierce_TormentTherapy_Event_20 (item 20).
}

void AWasamiZone2Flow::PostmazeTransition()
{
	Enter(TEXT("Postmaze Transition"));
	RemoveAllEnemies(GetWorld());
	DestroyAllShards(GetWorld());
	// BP_Collectable ID 3 spawned at the target point collec (item 12). ring_statue_2's Interact All Shards bound to
	// Collected Ring Piece (item 13).
	const FName Orb = SourceTag(TEXT("ring_statue_orb_5"));
	TArray<AActor*> Orbs;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(Orb))
		{
			Orbs.Add(*It);
		}
	}
	for (AActor* Piece : Orbs)
	{
		Piece->Destroy();
	}
	After(0.f, [this]()
	{
		SetArrowShards(false);
		SetArrowColor(RingPieceArrow);
		SetArrowTarget(Source(TEXT("BP_08_RingPiece_NoPickup_5")));
		SetObjective(NSLOCTEXT("Wasami", "ObjectiveCollectRingPiece", "COLLECT THE RING PIECE"));
	});
}
