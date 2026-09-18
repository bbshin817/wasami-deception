#include "WasamiZone2Flow.h"

#include "Camera/CameraShakeBase.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiDoubleDoors.h"
#include "WasamiEnemySentry.h"
#include "WasamiEnemyZone2.h"
#include "WasamiGameMode.h"
#include "WasamiHitFX.h"
#include "WasamiRingPieceWidget.h"
#include "WasamiRingStatue.h"
#include "WasamiZoneBarrier.h"

namespace
{
	// BP_ArrowPointer's Change Color in the zone's sections.
	const FLinearColor MinibossArrow(0.f, 0.f, 0.f, 0.f);
	const FLinearColor RingPieceArrow(1.f, 0.8941f, 0.f, 1.f);
	const FLinearColor GarageArrow(1.f, 0.8941f, 0.f, 1.f);

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
	RingPiecePickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/RingStatue/Ring_Piece_Pickup_v1")));
	ParameterCollection = TSoftObjectPtr<UMaterialParameterCollection>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Special/Mat_ParameterCol")));
}

void AWasamiZone2Flow::BeginPlay()
{
	// ReceiveBeginPlay (@23216): SetScalarParameterValue(Mat_ParameterCol, 'Portal Extra Brightness', 40), then Setup.
	if (UMaterialParameterCollection* Collection = ParameterCollection.LoadSynchronous())
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("Portal Extra Brightness"), PortalExtraBrightness);
	}
	Super::BeginPlay();
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
	// The achievement's 06_NurseAlert set to 0 (achievements are not in this game).
	ActivateMinibossEnemies();
	// Bierce_TormentTherapy_Event_19 (item 20).
	SetArrowShards(false);
	SetArrowColor(MinibossArrow);
	SetArrowTarget(nullptr);
	// The original's text ends in a space.
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveGetPastNurses", "Get past the nurses "));
	BindTrigger(TEXT("Trigger_MazeStart"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMazeTriggerStart));
	BindTrigger(TEXT("Trigger_Miniboss_BehindMatron"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMinibossBehindMatron));
}

void AWasamiZone2Flow::ActivateMinibossEnemies()
{
	// Every BP_06_ReaperNurse_Sentry's Activate, then the Matron's (MnM_Matron_Idle_2, item 11).
	for (TActorIterator<AWasamiEnemySentry> It(GetWorld()); It; ++It)
	{
		It->Activate();
	}
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
	SpawnNurses();
}

void AWasamiZone2Flow::SpawnNurses()
{
	for (const TCHAR* Point : {TEXT("NurseSpawn_4"), TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2")})
	{
		SpawnEnemy(AWasamiEnemyZone2::StaticClass(), Point);
	}
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
	// BP_Collectable ID 3 spawned at the target point collec (item 12).
	if (AWasamiRingStatue* Statue = Cast<AWasamiRingStatue>(Source(TEXT("ring_statue_2"))))
	{
		Statue->OnInteractAllShards.AddUniqueDynamic(this, &AWasamiZone2Flow::OnCollectedRingPiece);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no ring altar ring_statue_2"), *GetClass()->GetName());
	}
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

void AWasamiZone2Flow::OnCollectedRingPiece()
{
	Enter(TEXT("Collected Ring Piece"));
	// UMG_01_RingPieceCollect with ringpiece_texture T_RingPiece_1 (the screen's default), at Z 0, its Close bound to
	// Ring Piece Collect ; then Ring_Piece_Pickup_v1 (PlaySound2D, a UI sound, so it plays over the paused game).
	if (UWasamiRingPieceWidget* Screen = UWasamiRingPieceWidget::Show(this))
	{
		Screen->OnClose.AddDynamic(this, &AWasamiZone2Flow::OnRingPieceCollect);
	}
	UGameplayStatics::PlaySound2D(this, RingPiecePickupSound.LoadSynchronous());
}

void AWasamiZone2Flow::OnRingPieceCollect()
{
	Enter(TEXT("Ring Piece Collect "));
	// The altar's two pink lights off.
	for (const TCHAR* Name : {TEXT("PointLight202"), TEXT("PointLight201_6")})
	{
		if (const ALight* Light = Cast<ALight>(Source(Name)))
		{
			Light->GetLightComponent()->SetVisibility(false, false);
		}
	}
	if (AWasamiZoneBarrier* Barrier = ZoneBarrier(TEXT("BP_ZoneBarrier_2")))
	{
		Barrier->DestroyBarrier();
	}
	SetArrowShards(false);
	SetArrowColor(GarageArrow);
	SetArrowTarget(Source(TEXT("Postmaze_Trigger_Garage")));
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveHeadTowardsGarage", "HEAD TOWARDS THE GARAGE"));
	if (AActor* Piece = Source(TEXT("BP_08_RingPiece_NoPickup_5")))
	{
		Piece->Destroy();
	}
	// bLocked written directly: the doors to the garage open as the player comes up to them.
	if (AWasamiDoubleDoors* Doors = DoubleDoors(TEXT("BP_06_DoubleDoors2")))
	{
		Doors->bLocked = false;
	}
	After(GarageBindDelay, [this]()
	{
		// Bierce_TormentTherapy_Event_21 (item 20). The original also binds Postmaze_Trigger_Ambulance, on the
		// ambulance's roof, to the ride to the boss fight; this game leaves by the garage's portal instead.
		BindTrigger(TEXT("Postmaze_Trigger_Garage"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnPostmazeTriggerGarage));
	});
}

void AWasamiZone2Flow::OnPostmazeTriggerGarage()
{
	Enter(TEXT("Postmaze_Trigger_Garage"));
	// The original points the arrow at the ambulance (GET ON TOP OF THE AMBULANCE) and has Bierce talk 1 s on; this
	// game opens the garage's portal here instead (item 13's step 5).
}
