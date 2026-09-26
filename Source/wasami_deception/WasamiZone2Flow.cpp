#include "WasamiZone2Flow.h"

#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiBlackFadeWidget.h"
#include "WasamiCollectable.h"
#include "WasamiDoubleDoors.h"
#include "WasamiEnemySentry.h"
#include "WasamiEnemyZone2.h"
#include "WasamiGameMode.h"
#include "WasamiHitFX.h"
#include "WasamiLift.h"
#include "WasamiMatron.h"
#include "WasamiMusicPlayer.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiPortal.h"
#include "WasamiRingPieceWidget.h"
#include "WasamiRingStatue.h"
#include "WasamiZoneBarrier.h"

namespace
{
	// BP_ArrowPointer's Change Color in the zone's sections.
	const FLinearColor MinibossArrow(0.f, 0.f, 0.f, 0.f);
	const FLinearColor RingPieceArrow(1.f, 0.8941f, 0.f, 1.f);
	const FLinearColor GarageArrow(1.f, 0.8941f, 0.f, 1.f);
	// 01_Hotel's Change Color as it points the arrow at its exit portal.
	const FLinearColor PortalArrow(1.f, 0.f, 0.016666f, 1.f);

	static_assert(AWasamiZone2Flow::EscapeMusicFade == AWasamiMusicPlayer::FadeDuration,
		"the escape fades the music over the player's own fade, and holds Escape's pause back for just as long");
}

const FName AWasamiZone2Flow::GaragePortal(TEXT("Wasami_GaragePortal"));
const FName AWasamiZone2Flow::EscapeTrigger(TEXT("Wasami_EscapeTrigger"));
const FName AWasamiZone2Flow::MusicPlayerSource(TEXT("BP_06_MusicPlayer_Zone2_2"));
const FName AWasamiZone2Flow::Matron(TEXT("MnM_Matron_Idle_2"));
const FName AWasamiZone2Flow::BehindMatronIntercom(TEXT("Nurse_Hospital_Zone01_Event_48_Intercom_2"));
const FName AWasamiZone2Flow::PostmazeFilePoint(TEXT("collec"));

AWasamiZone2Flow::AWasamiZone2Flow()
{
	DoorPickedShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/04_Sewer/Bossfight/BP_04_BossFight_CameraShake_Initial")));
	SpikesSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_Needle_Trap_R1_V3")));
	RingPiecePickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/RingStatue/Ring_Piece_Pickup_v1")));
	EscapeSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/00_Ballroom/21-Ballroom_portal_V2")));
	ParameterCollection = TSoftObjectPtr<UMaterialParameterCollection>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Special/Mat_ParameterCol")));
	CellLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Event_17")));
	MinibossLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Event_19")));
	LiftQuipLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Gameplay_07")));
	MazeLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Gameplay_08")));
	MazeAllShardsLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Event_20")));
	RingPieceLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Event_21")));
	GarageLine = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/Bierce_TormentTherapy_Event_22")));
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
	// Spawn (@22328): Load Progress By Level(7, 8). 7 is the ambulance's arrival (Arrive Event → the capture → the
	// cell), which the original plays again whenever the level opens there, the spikes' death included. 8 to 10 start at
	// their player starts, which the game mode has put the player at. 0 opens the entrance (the game mode opens Zone 1
	// instead).
	switch (Checkpoint)
	{
	case 7: ArriveEvent(); break;
	case 8: Enter(TEXT("Miniboss Start ")); MinibossTransition(); break;
	case 9: Enter(TEXT("Maze Start")); MazeTransition(); break;
	case 10: Enter(TEXT("Postmaze Start")); PostmazeTransition(); break;
	default: break;
	}
}

void AWasamiZone2Flow::ArriveEvent()
{
	Enter(TEXT("Arrive Event"));
	// @24359: Is Packaged For Distribution tells the two apart. This game is played packaged, so it takes that path: the
	// player to PlayerStart_1 and, 0.3 s on, 06_Hospital_Zone2_AmbulanceArrive1 played. The arrival is no cut scene —
	// no view of its own, no skip screen and the player's input left alone — so they walk about the yard while the
	// ambulance drives in, until they walk into Trigger_Arrive_CaptureScene. (An editor build goes straight to the
	// capture instead, which this game has no use for.)
	TeleportPlayerTo(TEXT("PlayerStart_1"));
	After(ArriveSequenceDelay, [this]()
	{
		// Unlike the original, the player rides attached to the ambulance until it stops (as off Zone 1's roof): the
		// based move alone left them on the road when one long frame (the level just opened) pushed them off the roof.
		RidePlayerOn(TEXT("hospital_ambulance_new_arrive"));
		PlaySequence(TEXT("06_Hospital_Zone2_AmbulanceArrive1_2"),
			GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnEscapeAmbulanceArrive));
		BindTrigger(TEXT("Trigger_Arrive_CaptureScene"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnArriveCaptureCutscene));
	});
}

void AWasamiZone2Flow::OnEscapeAmbulanceArrive()
{
	Enter(TEXT("Escape_AmbulanceArrive"));
	StopPlayerRide();
	if (AActor* Blocker = Source(TEXT("Ambulance_Arrive_Blockers4")))
	{
		Blocker->Destroy();
	}
}

void AWasamiZone2Flow::OnArriveCaptureCutscene()
{
	Enter(TEXT("Arrive_CaptureCutscene"));
	// @25143: Disable Player Input first, then the view to CineCameraActor_2 over 0.5 s and 06_Hospital_Zone2_Capture
	// played with its skip screen. The scene's own camera anim (CameraAnim_Nurse_01 over its 20.53 to 25.27 s) needs
	// nothing here: its move carries the cine camera, and the pipeline adds it to the scene's transform track, which
	// UE 5.8 has no camera anim track for (item 28's step 14d, 01 record's dd_sequence.bake_location).
	DisablePlayerInput(this);
	PlayCutscene(TEXT("06_Hospital_Zone2_Capture"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnCellCutsceneStart),
		TEXT("CineCameraActor_2"));
}

void AWasamiZone2Flow::FadeCellMusicIn(float Duration, float Volume)
{
	if (AWasamiMusicPlayer* Music = MusicPlayer(MusicPlayerSource))
	{
		Music->FadeRegularMusicIn(Duration, Volume);
	}
}

void AWasamiZone2Flow::OnCellCutsceneStart()
{
	Enter(TEXT("Cell Cutscene Start"));
	// @3149: a 1 s wait, then 06_Hospital_Zone2_Cell played with its skip screen — whose bars do not slide in, as the
	// capture's are up already — its OnFinished bound to Cell Cutscene Finished, and the player moved into the cell
	// (PlayerStart_Cell), where the scene is about to show them. The scene takes no camera of its own: it animates the
	// capture's CineCameraActor_2, which the view is on still, and its fade carries the black the capture ends on (the
	// cell in sight at 7.43 s).
	After(CellSequenceDelay, [this]()
	{
		PlayCutscene(TEXT("06_Hospital_Zone2_Cell"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnCellCutsceneFinished),
			NAME_None, false);
		TeleportPlayerTo(TEXT("PlayerStart_Cell"));
		// And the zone's music ducked under the scene (@3688), which bFadeOut cannot do: a fade in to a lower volume.
		FadeCellMusicIn(CellMusicFadeIn, CellMusicVolume);
	});
}

void AWasamiZone2Flow::OnCellCutsceneFinished()
{
	Enter(TEXT("Cell Cutscene Finished"));
	// @23769: Enable Player Input, the music back up over 2.5 s, and the view blended back from the scenes' cine camera
	// to the player over 2 s.
	EnablePlayerInput(this);
	FadeCellMusicIn(CellEndMusicFadeIn, CellEndMusicVolume);
	SetPlayerViewTarget(UGameplayStatics::GetPlayerCharacter(this, 0), CellViewBlendTime);
	PlaySequence(TEXT("06_Hospital_Zone2_Spikes"));
	EnableDoorBreak(TEXT("BP_06_Hospital_DoorBreak_2"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnCellDoorBreak));
	BindTrigger(TEXT("Trigger_Cell_Spikes"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnSpikesDeath));
	BindTrigger(TEXT("BP_MiniBoss_Trigger"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnMinibossTriggerTransition));
	After(CellLineDelay, [this]()
	{
		// @2987: Bierce Talk(Event_17, False), and then the trigger bound.
		BierceTalk(CellLine);
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
	// @24311: Bierce Talk(Event_19, False), and nothing else. The line is the same as the section's own below: whichever
	// comes first is heard, and the talker waits the other out (it never cuts a line short).
	BierceTalk(MinibossLine);
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
	// @4587: Talk(Event_19, False) straight after them, before the arrow.
	BierceTalk(MinibossLine);
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
	// Every BP_06_ReaperNurse_Sentry's Activate, then the Matron's (the level's MnM_Matron_Idle_2).
	for (TActorIterator<AWasamiEnemySentry> It(GetWorld()); It; ++It)
	{
		It->Activate();
	}
	if (AWasamiMatron* Placed = Cast<AWasamiMatron>(Source(Matron)))
	{
		Placed->Activate();
	}
}

void AWasamiZone2Flow::OnMinibossBehindMatron()
{
	Enter(TEXT("Miniboss_BehindMatron"));
	// @22264: Nurse_Hospital_Zone01_Event_48_Intercom_2's AudioComponent Play(0) — the announcement is the level's own
	// AmbientSound, not a line of Bierce's, and it is placed without bAutoActivate, so this is what starts it.
	const AAmbientSound* Intercom = Cast<AAmbientSound>(Source(BehindMatronIntercom));
	if (UAudioComponent* Audio = Intercom ? Intercom->GetAudioComponent() : nullptr)
	{
		Audio->Play(0.f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no intercom %s"), *GetClass()->GetName(), *BehindMatronIntercom.ToString());
	}
}

void AWasamiZone2Flow::OnMazeTriggerStart()
{
	Enter(TEXT("Maze Trigger Start"));
	MazeTransition();
	SaveCheckpoint(9);
	After(MazeLineDelay, [this]()
	{
		// @2926: Talk(Gameplay_08, True) — the hospital's one attenuated line, heard from where the talker stands — and
		// then Setup Bierce Lift Quip.
		BierceTalk(MazeLine, true);
		SetupBierceLiftQuips();
	});
}

void AWasamiZone2Flow::SetupBierceLiftQuips()
{
	// Get All Actors Of Class(BP_06_LiftBase): every lift of the maze, its Player Overlap bound to Bierce Lift Quip.
	TArray<AActor*> Lifts;
	UGameplayStatics::GetAllActorsOfClass(this, AWasamiLift::StaticClass(), Lifts);
	for (AActor* Lift : Lifts)
	{
		CastChecked<AWasamiLift>(Lift)->OnPlayerOverlap.AddUniqueDynamic(this, &AWasamiZone2Flow::OnBierceLiftQuip);
	}
}

void AWasamiZone2Flow::OnBierceLiftQuip()
{
	// @14266: a DoOnce before the Delay, so only the first lift stepped on is remarked on.
	if (bLiftQuipSaid)
	{
		return;
	}
	bLiftQuipSaid = true;
	After(LiftQuipDelay, [this]() { BierceTalk(LiftQuipLine); });
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
	// @2879: the event's other pin, a Delay of 2 s that the save and Postmaze Transition start before.
	After(MazeAllShardsLineDelay, [this]() { BierceTalk(MazeAllShardsLine); });
}

void AWasamiZone2Flow::PostmazeTransition()
{
	Enter(TEXT("Postmaze Transition"));
	RemoveAllEnemies(GetWorld());
	DestroyAllShards(GetWorld());
	// BeginDeferredActorSpawnFromClass(BP_Collectable, collec's transform, AdjustIfPossibleButAlwaysSpawn) with ID 3.
	if (const AActor* Point = Source(PostmazeFilePoint))
	{
		const FTransform Transform = Point->GetActorTransform();
		if (AWasamiCollectable* File = GetWorld()->SpawnActorDeferred<AWasamiCollectable>(AWasamiCollectable::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			File->ID = PostmazeFileID;
			File->FinishSpawning(Transform);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no target point %s for the secret file"), *GetClass()->GetName(), *PostmazeFilePoint.ToString());
	}
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
		// @43: Talk(Event_21, False), and then the trigger bound. The original also binds Postmaze_Trigger_Ambulance, on
		// the ambulance's roof, to the ride to the boss fight; this game leaves by the garage's portal instead.
		BierceTalk(RingPieceLine);
		BindTrigger(TEXT("Postmaze_Trigger_Garage"), GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnPostmazeTriggerGarage));
	});
}

void AWasamiZone2Flow::OnPostmazeTriggerGarage()
{
	Enter(TEXT("Postmaze_Trigger_Garage"));
	// The original points the arrow at the ambulance (GET ON TOP OF THE AMBULANCE); this game opens the garage's portal
	// here, as the hotel opens its exit once the ring piece is taken (01_Hotel @43247 to @43565: the arrow red at the
	// portal, Lock/Unlock(False, False), without its sound and shake), and binds the trigger by it. The objective is
	// the hotel's 'Get back to the portal.', in the hospital's capitals, the player not having been to it.
	AWasamiPortal* Portal = Cast<AWasamiPortal>(Source(GaragePortal));
	if (Portal)
	{
		Portal->LockUnlock(false, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no portal %s"), *GetClass()->GetName(), *GaragePortal.ToString());
	}
	SetArrowShards(false);
	SetArrowColor(PortalArrow);
	SetArrowTarget(Portal);
	SetObjective(NSLOCTEXT("Wasami", "ObjectiveGetToPortal", "GET TO THE PORTAL"));
	BindTrigger(EscapeTrigger, GET_FUNCTION_NAME_CHECKED(AWasamiZone2Flow, OnEndTrigger));
	// @218: Talk(Event_22, False) a second on, as the original has it over its own way out.
	After(GarageLineDelay, [this]() { BierceTalk(GarageLine); });
}

void AWasamiZone2Flow::OnEndTrigger()
{
	Enter(TEXT("EndTrigger"));
	// BP_00_Teleport's way in (ReceiveActorBeginOverlap @2187): DisableInput on the player, Sprinting? false, the fade at
	// Z 5 and the portal's Sound (which neither the class nor the hotel's exit sets). Its UMG_BlackFade flashes the
	// screen over 0.5 s and moves the player at its peak, 0.25 s on; here the fade goes black in those 0.25 s and stays
	// under the score screen, which comes over it in the same 0.25 s.
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->DisableInput(Controller);
		Player->StopSprinting();
	}
	if (UWasamiBlackFadeWidget* Fade = UWasamiBlackFadeWidget::Show(this, true, EscapeFadeSpeed, EscapeFadeZOrder))
	{
		Fade->bHold = true;
	}
	// Then as the original's ride to the boss fight ends (Postmaze_Trigger_Ambulance @1511, with the loading screen):
	// 21-Ballroom_portal_V2 and Remove All Enemies.
	UGameplayStatics::PlaySound2D(this, EscapeSound.LoadSynchronous());
	RemoveAllEnemies(GetWorld());
	// The zone's music taken away as the level ends, where the original's ride to the boss fight takes it (@1423, with
	// its GOOD LUCK and the loading screen 7 s on). bFadeOut is raised as the original raises it, but on its own it is
	// never heard: Escape stops the game in this same frame, so the music player's 0.5 s Update never comes round. So
	// the components are faded here instead, and Escape below holds its pause back for as long as the fade — a paused
	// game has no sound at all (FAudioDevice::HandlePause pauses every source that is not a UI sound), so that wait is
	// what lets the fade be heard, now under the score screen rather than under a black hold.
	if (AWasamiMusicPlayer* Music = MusicPlayer(MusicPlayerSource))
	{
		Music->bFadeOut = true;
		Music->FadeAllMusicOut(EscapeMusicFade);
	}
	// And the hospital's own portal: its Trigger_Escape calls Escape at once (06_Hospital @66935), which saves and puts
	// up the score screen, as the hotel's EndTrigger does. It is called at once here too (2026-09-23, the user's answer
	// 即時), so nothing black stands between the portal and the screen. The counter is stopped first, so the time saved
	// is the time played to the portal.
	if (AWasamiGameMode* GameMode = GetMode())
	{
		GameMode->PauseTimeCounter();
		GameMode->Escape(EscapeMusicFade);
	}
}
