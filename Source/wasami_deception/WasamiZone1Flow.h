#pragma once

#include "CoreMinimal.h"
#include "WasamiZoneFlow.h"
#include "WasamiZone1Flow.generated.h"

class AWasamiEnemy06Chase;

/**
 * Zone 1's level Blueprint (pak_reference_2's 06_Hospital_Zone_01): the lift arrives (checkpoint 4; on a new start
 * with the entrance's title card, UMG_ChapterPortal, over the first 10 s, the player held), the lift's door is
 * broken open, the maze past it saves 5 and wants all the shards; with the last one the barrier breaks and the parking
 * lot is next, then the tunnel, the doors the nurses break in 25 s, and the ambulance's roof, which saves 7 and opens
 * Zone 2. The events keep the original's names in their comments and in GetSection.
 *
 * The nurses: three that patrol the maze (Spawn Nurses at 05_Persistent), and the two of the parking lot that chase the
 * player from 06_Start (Spawn Nurses_06: AWasamiEnemy06Chase), stab at the tunnel's doors while they hold
 * (06_DoorsLock's bAttackDoor) and follow them in. A nurse in TriggerVolume_1 keeps the car park's garage lift down
 * for good (its NurseNear).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZone1Flow : public AWasamiZoneFlow
{
	GENERATED_BODY()

public:
	AWasamiZone1Flow();

	/** 00_Initial Start's Delay before the player's CanMove? is back, the title card up meanwhile (@19451). */
	static constexpr float InitialHoldSeconds = 10.f;
	/** How long the lift shakes before the intercom and the door's lock can be used (Spawn at 4, @14643). */
	static constexpr float ArrivalShakeSeconds = 7.f;
	/** 05_Persistent's wait before it checks the shards (so a zone reopened with none left moves on). */
	static constexpr float ShardCheckDelay = 1.f;
	/** 04_Intercom: the announcement's volume (PlaySound2D) and how long after it Bierce speaks. */
	static constexpr float IntercomVolume = 0.6f;
	static constexpr float IntercomLineDelay = 13.f;
	/** 04_DoorBreak: how long after the lift's doors open Bierce speaks. */
	static constexpr float DoorBreakLineDelay = 1.f;
	/** Bierce Nurse Quip's Random Bool With Weight: how often a nurse coming close is remarked on. */
	static constexpr float NurseQuipChance = 0.2f;
	/** 06 Transition's Basic DD Fade Out: Ballroom_Event_Fade at twice its rate (black for 1 s, clear 1.5 s later). */
	static constexpr float TransitionFadeRate = 2.f;
	/** 06_DoorsLock: how long the nurses take to break the doors in. */
	static constexpr float DoorsBreakSeconds = 25.f;
	/** As they break in: the shake's reach about the doors (full at them), and the doors gone 0.1 s on. */
	static constexpr float DoorsBustedShakeRadius = 3000.f;
	static constexpr float DoorsGoneDelay = 0.1f;
	/** 06_ReachAmbulance: the ambulance leaves 1 s on, the loading screen comes 7 s after, Zone 2 opens 2.5 s after that. */
	static constexpr float TakeOffDelay = 1.f;
	static constexpr float LoadingDelay = 7.f;
	static constexpr float OpenZone2Delay = 2.5f;
	/** The ambulance's shake as it leaves (ClientPlayCameraShake's scale). */
	static constexpr float TakeOffShakeScale = 4.f;

	/** BP_06_MusicPlayer_2, whose bFadeOut the sections raise and drop (the level places it true, so the zone opens silent). */
	static const FName MusicPlayerSource;

protected:
	virtual void StartAt(int32 Checkpoint) override;

	/** The lift's arrival: 01_Hotel_Lobby_ElevatorShake, and 01_Hotel_Lobby_ElevatorShakeStop as it ends. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftClassPtr<UCameraShakeBase> ElevatorShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftClassPtr<UCameraShakeBase> ElevatorShakeStopClass;

	/** The tunnel's doors broken in: DD_TT_Door_BustedOpen_02 through 01_Lobby_Attenuation, and BP_07_CameraShake_Jump. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundBase> DoorsBustedSound;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundAttenuation> DoorsBustedAttenuation;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftClassPtr<UCameraShakeBase> DoorsBustedShakeClass;

	/** The ambulance leaving: 06_CameraShake_Zone1_AmbulanceTakeOff. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftClassPtr<UCameraShakeBase> TakeOffShakeClass;

	/** With the loading screen: 21-Ballroom_portal_V2 (PlaySound2D). */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundBase> PortalSound;

	/** 04_Intercom: the nurse's announcement over the intercom, and Bierce's Event_09 13 s after it. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Dialogue")
	TSoftObjectPtr<USoundBase> IntercomSound;

	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Dialogue")
	TSoftObjectPtr<USoundBase> IntercomLine;

	/** 04_DoorBreak: Bierce's Event_10, a second after the lift's doors swing open. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Dialogue")
	TSoftObjectPtr<USoundBase> DoorBreakLine;

	/** Bierce Nurse Quip: Bierce_TormentTherapy_Gameplay, the cue that picks one of the five remarks at random. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Dialogue")
	TSoftObjectPtr<USoundBase> NurseQuipCue;

private:
	/** The entrance's 00_Initial Start, for a new start: the stage's title card, the player held meanwhile. */
	void InitialStart();
	/** Spawn at 4: the lift's arrival. */
	void Start04();
	/** 05_Persistent: the maze's shards wanted. */
	void Persistent05();
	/** 06 Transition: the leftover shards gone, the view back on the player, the fade and the player at 06_Start. */
	void Transition06();
	/** 06_DoorsLock 25 s on: the nurses break the tunnel's doors in. */
	void BreakDoorsIn();
	/** Spawn at 6 and the end of 06 Transition: the parking lot's nurses and triggers, the tunnel as the goal. */
	void Start06();
	/** Spawn Nurses: BP_06_ReaperNurse at NurseSpawn_3, _1 and _2, and then Setup Nurse Bierce Quips. */
	void SpawnNurses();
	/**
	 * Setup Nurse Bierce Quips: Bierce Nurse Quip bound to the CloseBy of every nurse in the world. Spawn Nurses ends
	 * with it, so those are the maze's three; the parking lot's come later (Spawn Nurses_06) and are not bound.
	 */
	void SetupNurseBierceQuips();
	/** Spawn Nurses_06: BP_06_ReaperNurse_06_Chase at 06_NurseSpawn and 06_NurseSpawn2, kept as 06 Nurses. */
	void SpawnNurses06();
	/** bAttackDoor on each of 06 Nurses. */
	void SetNursesAttackDoor(bool bAttack);

	/** BP_06_MusicPlayer_2's bFadeOut: the music fades out while it is on, and comes back as it is dropped. */
	void SetMusicFadeOut(bool bFadeOut);

	/** TriggerVolume_1's ActorBeginOverlap (bound from the level's start): a nurse sets the garage lift's NurseNear. */
	UFUNCTION()
	void OnNurseLiftTrigger(AActor* OverlappedActor, AActor* OtherActor);

	/** Bierce Nurse Quip: one time in five, a remark as a nurse first comes after the player (its CloseBy). */
	UFUNCTION()
	void OnBierceNurseQuip();

	UFUNCTION()
	void On04Intercom();

	UFUNCTION()
	void On04DoorBreak();

	UFUNCTION()
	void On05Transition();

	UFUNCTION()
	void On05AllShardsCollected();

	UFUNCTION()
	void On05ParkingLotCutscene();

	/** 06_Transition: 06_Hospital_Zone1_06Event's OnFinished (06 Transition, then the music's fade out off). */
	UFUNCTION()
	void On06Transition();

	UFUNCTION()
	void On06TunnelEnter();

	UFUNCTION()
	void On06DoorsLock();

	UFUNCTION()
	void On06ReachAmbulance();

	/** Puts the player inside the roof's fence, standing on the roof, unless they already are. */
	void PlaceOnAmbulanceRoof();

	/** 06 Nurses: those Spawn Nurses_06 spawned (gone once destroyed). */
	TArray<TWeakObjectPtr<AWasamiEnemy06Chase>> Nurses06;
};
