#include "WasamiZone1Flow.h"

#include "Camera/CameraShakeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiChapterPortalWidget.h"
#include "WasamiDoubleDoors.h"
#include "WasamiEnemy.h"
#include "WasamiEnemy06Chase.h"
#include "WasamiGameMode.h"
#include "WasamiGarageLift.h"
#include "WasamiLoadingWidget.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiZoneBarrier.h"

namespace
{
	// BP_ArrowPointer's Change Color in the zone's sections.
	const FLinearColor ParkingLotArrow(1.f, 0.8002f, 0.f, 1.f);
	const FLinearColor TunnelArrow(1.f, 0.8317f, 0.f, 1.f);
}

AWasamiZone1Flow::AWasamiZone1Flow()
{
	ElevatorShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShake")));
	ElevatorShakeStopClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop")));
	DoorsBustedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_Door_BustedOpen_02")));
	DoorsBustedAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
	DoorsBustedShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/07_FunPlace/Boss/BP_07_CameraShake_Jump")));
	TakeOffShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/06_Hospital/06_CameraShake_Zone1_AmbulanceTakeOff")));
	PortalSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/00_Ballroom/21-Ballroom_portal_V2")));
}

void AWasamiZone1Flow::StartAt(int32 Checkpoint)
{
	// The level's event bound to TriggerVolume_1, from its start.
	if (AActor* Volume = Source(TEXT("TriggerVolume_1")))
	{
		Volume->OnActorBeginOverlap.AddDynamic(this, &AWasamiZone1Flow::OnNurseLiftTrigger);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no TriggerVolume_1 for the garage lift's NurseNear"), *GetClass()->GetName());
	}
	// Spawn (@13483): Load Progress By Level(7, 5). 0 opens the entrance (the game mode writes 4 instead, and the
	// entrance's part is played here); 7 and on are Zone 2's, for which Zone 1 does nothing.
	if (Mode && Mode->IsNewStart())
	{
		InitialStart();
	}
	switch (Checkpoint)
	{
	case 4: Start04(); break;
	case 5: Persistent05(); break;
	case 6: Start06(); break;
	default: break;
	}
}

void AWasamiZone1Flow::InitialStart()
{
	// The entrance's Spawn at 0 → 00_Initial Start (@19176), from its Delay 0: its ambience, 00_Start and the trigger
	// 00_CutsceneStart are the entrance's own, and Zone 1's lift arrives meanwhile.
	After(0.f, [this]()
	{
		UWasamiChapterPortalWidget::Show(this);
		AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (!Player)
		{
			return;
		}
		Player->bCanMove = false;
		After(InitialHoldSeconds, [Player = TWeakObjectPtr<AWasamiPlayerCharacter>(Player)]()
		{
			if (Player.IsValid())
			{
				Player->bCanMove = true;
			}
			// Bierce's Bierce_TormentTherapy_Event_01 1 s on (item 20).
		});
	});
}

void AWasamiZone1Flow::Start04()
{
	Enter(TEXT("04_Start"));
	if (AWasamiDoubleDoors* Doors = DoubleDoors(TEXT("BP_06_DoubleDoors11")))
	{
		Doors->Lock();
	}
	PlaySequence(TEXT("06_Hospital_Zone01_ElevatorArrive"));
	PlayCameraShake(ElevatorShakeClass);
	After(ArrivalShakeSeconds, [this]()
	{
		PlayCameraShake(ElevatorShakeStopClass);
		BindTrigger(TEXT("04_Intercom"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On04Intercom));
		EnableDoorBreak(TEXT("BP_06_Hospital_DoorBreak_2"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On04DoorBreak));
	});
}

void AWasamiZone1Flow::On04Intercom()
{
	Enter(TEXT("04_Intercom"));
	// The nurse's announcement (Nurse_Hospital_Zone01_Event_37_Intercom at 0.6) and Bierce 13 s on: the voices (item 20).
}

void AWasamiZone1Flow::On04DoorBreak()
{
	Enter(TEXT("04_DoorBreak"));
	// bLocked written directly, not Unlock (which would open them only for the last character in front).
	if (AWasamiDoubleDoors* Doors = DoubleDoors(TEXT("BP_06_DoubleDoors11")))
	{
		Doors->bLocked = false;
		Doors->OpenFront();
	}
	// Bierce 1 s on (item 20).
	BindTrigger(TEXT("BP_04_Trigger_Maze"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On05Transition));
}

void AWasamiZone1Flow::On05Transition()
{
	Enter(TEXT("05_Transition"));
	SaveCheckpoint(5);
	Persistent05();
}

void AWasamiZone1Flow::Persistent05()
{
	Enter(TEXT("05_Persistent"));
	SpawnNurses();
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveCollectAllShards", "COLLECT ALL SHARDS"));
	BindAllShardsCollected(GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On05AllShardsCollected));
	// BP_06_MusicPlayer_2's bFadeOut false (item 19).
	After(ShardCheckDelay, [this]()
	{
		if (Mode)
		{
			Mode->CheckShards();
		}
	});
}

void AWasamiZone1Flow::On05AllShardsCollected()
{
	Enter(TEXT("05 All Shards Collected"));
	RemoveAllEnemies(GetWorld());
	SetArrowShards(false);
	SetArrowColor(ParkingLotArrow);
	SetArrowTarget(Source(TEXT("06_CutsceneStart")));
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveReachParkingLot", "REACH THE PARKING LOT"));
	// BP_06_MusicPlayer_2's bFadeOut true (item 19).
	BindTrigger(TEXT("06_CutsceneStart"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On05ParkingLotCutscene));
	if (AWasamiZoneBarrier* Barrier = ZoneBarrier(TEXT("BP_ZoneBarrier_2")))
	{
		Barrier->DestroyBarrier();
	}
}

void AWasamiZone1Flow::On05ParkingLotCutscene()
{
	Enter(TEXT("05_ParkingLotCutscene"));
	// The scene 06_Hospital_Zone1_06Event (item 25) is left out, so its end, 06_Transition, follows at once: 06 Transition
	// and the music's bFadeOut false (item 19).
	Transition06();
}

void AWasamiZone1Flow::Transition06()
{
	Enter(TEXT("06 Transition"));
	DestroyAllShards(GetWorld());
	// The view back on the player (SetViewTargetWithBlend), which it has not left with the scene left out.
	PlayFadeOut(TransitionFadeRate);
	TeleportPlayerTo(TEXT("06_Start"));
	Start06();
}

void AWasamiZone1Flow::Start06()
{
	Enter(TEXT("06_Start"));
	SpawnNurses06();
	BindTrigger(TEXT("06_DoorsLock"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On06DoorsLock));
	BindTrigger(TEXT("TriggerBox_06_AmbulanceTop"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On06ReachAmbulance));
	SetArrowShards(false);
	SetArrowColor(TunnelArrow);
	SetArrowTarget(Source(TEXT("06_TunnelEnter")));
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveReachTunnel", "REACH THE TUNNEL"));
	BindTrigger(TEXT("06_TunnelEnter"), GET_FUNCTION_NAME_CHECKED(AWasamiZone1Flow, On06TunnelEnter));
}

void AWasamiZone1Flow::On06TunnelEnter()
{
	Enter(TEXT("06_TunnelEnter"));
	SetArrowShards(false);
	SetArrowColor(TunnelArrow);
	SetArrowTarget(Source(TEXT("TriggerBox_06_AmbulanceTop")));
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveAmbulanceTop", "GET ON TOP OF THE AMBULANCE"));
}

void AWasamiZone1Flow::On06DoorsLock()
{
	Enter(TEXT("06_DoorsLock"));
	if (AWasamiDoubleDoors* Doors = DoubleDoors(TEXT("BP_06_DoubleDoors33_36")))
	{
		Doors->Lock();
		Doors->ForceClose();
	}
	SetVolumeCollision(TEXT("BlockingVolume_1"), ECollisionEnabled::QueryAndPhysics);
	SetNursesAttackDoor(true);
	After(DoorsBreakSeconds, [this]() { BreakDoorsIn(); });
}

void AWasamiZone1Flow::BreakDoorsIn()
{
	AWasamiDoubleDoors* Doors = DoubleDoors(TEXT("BP_06_DoubleDoors33_36"));
	if (Doors)
	{
		const FVector At = Doors->GetActorLocation();
		PlaySoundAt(DoorsBustedSound, At, DoorsBustedAttenuation);
		PlayWorldCameraShake(DoorsBustedShakeClass, At, 0.f, DoorsBustedShakeRadius, 1.f, true);
	}
	ActivateEmitter(TEXT("Fracture_concrete_5"));
	SetVolumeCollision(TEXT("BlockingVolume_1"), ECollisionEnabled::NoCollision);
	SetNursesAttackDoor(false);
	After(DoorsGoneDelay, [Doors = TWeakObjectPtr<AWasamiDoubleDoors>(Doors)]()
	{
		if (Doors.IsValid())
		{
			Doors->Destroy();
		}
	});
}

void AWasamiZone1Flow::SpawnNurses()
{
	for (const TCHAR* Point : {TEXT("NurseSpawn_3"), TEXT("NurseSpawn_1"), TEXT("NurseSpawn_2")})
	{
		SpawnEnemy(AWasamiEnemy::StaticClass(), Point);
	}
	// Setup Nurse Bierce Quips: Bierce's lines as a nurse first chases (its CloseBy): the item 20's voices.
}

void AWasamiZone1Flow::SpawnNurses06()
{
	AActor* Door = Source(TEXT("DoorLocation"));
	for (const TCHAR* Point : {TEXT("06_NurseSpawn"), TEXT("06_NurseSpawn2")})
	{
		AWasamiEnemy* Enemy = SpawnEnemy(AWasamiEnemy06Chase::StaticClass(), Point, [Door](AWasamiEnemy& Spawned)
		{
			CastChecked<AWasamiEnemy06Chase>(&Spawned)->DoorLocation = Door;
		});
		if (AWasamiEnemy06Chase* Nurse = Cast<AWasamiEnemy06Chase>(Enemy))
		{
			Nurses06.Add(Nurse);
		}
	}
}

void AWasamiZone1Flow::SetNursesAttackDoor(bool bAttack)
{
	for (const TWeakObjectPtr<AWasamiEnemy06Chase>& Nurse : Nurses06)
	{
		if (Nurse.IsValid())
		{
			Nurse->bAttackDoor = bAttack;
		}
	}
}

void AWasamiZone1Flow::OnNurseLiftTrigger(AActor* OverlappedActor, AActor* OtherActor)
{
	// Cast to BP_06_ReaperNurse: any nurse. NurseNear is never cleared (Check Lift Nurses, which would, is never called).
	if (!Cast<AWasamiEnemy>(OtherActor))
	{
		return;
	}
	if (AWasamiGarageLiftZone1Special* Lift = Cast<AWasamiGarageLiftZone1Special>(Source(TEXT("hospital_garage_lift_anim_Anim_2"))))
	{
		Lift->bNurseNear = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no garage lift hospital_garage_lift_anim_Anim_2 for NurseNear"), *GetClass()->GetName());
	}
}

void AWasamiZone1Flow::On06ReachAmbulance()
{
	Enter(TEXT("06_ReachAmbulance"));
	SaveCheckpoint(7);
	RemoveAllEnemies(GetWorld());
	SetArrowShards(false);
	SetArrowColor(TunnelArrow);
	SetArrowTarget(Source(TEXT("Plane48_2")));
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveGoodLuck", "GOOD LUCK"));
	// BP_06_MusicPlayer_2's bFadeOut true (item 19).
	for (const TCHAR* Blocker : {TEXT("BlockingVolume_Ambulance_4"), TEXT("BlockingVolume_Ambulance_2"),
		TEXT("BlockingVolume_Ambulance_1"), TEXT("BlockingVolume_Ambulance_3")})
	{
		SetVolumeCollision(Blocker, ECollisionEnabled::QueryAndPhysics);
	}
	After(TakeOffDelay, [this]()
	{
		PlaySequence(TEXT("06_Hospital_Zone1_AmbulanceTakeOff"));
		PlayCameraShake(TakeOffShakeClass, TakeOffShakeScale);
		After(LoadingDelay, [this]()
		{
			// The loading screen's Construct forgets the shards collected.
			UWasamiLoadingWidget::Show(this, UWasamiLoadingWidget::AsylumLevel);
			if (USoundBase* Sound = PortalSound.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(this, Sound);
			}
			RemoveAllEnemies(GetWorld());
			After(OpenZone2Delay, [this]()
			{
				UGameplayStatics::OpenLevel(this, AWasamiGameMode::Zone2LevelName, true);
			});
		});
	});
}
