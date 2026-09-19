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

private:
	/** The entrance's 00_Initial Start, for a new start: the stage's title card, the player held meanwhile. */
	void InitialStart();
	/** Spawn at 4: the lift's arrival. */
	void Start04();
	/** 05_Persistent: the maze's shards wanted. */
	void Persistent05();
	/** 06 Transition: the leftover shards gone, the fade and the player at 06_Start. */
	void Transition06();
	/** 06_DoorsLock 25 s on: the nurses break the tunnel's doors in. */
	void BreakDoorsIn();
	/** Spawn at 6 and the end of 06 Transition: the parking lot's nurses and triggers, the tunnel as the goal. */
	void Start06();
	/** Spawn Nurses: BP_06_ReaperNurse at NurseSpawn_3, _1 and _2. */
	void SpawnNurses();
	/** Spawn Nurses_06: BP_06_ReaperNurse_06_Chase at 06_NurseSpawn and 06_NurseSpawn2, kept as 06 Nurses. */
	void SpawnNurses06();
	/** bAttackDoor on each of 06 Nurses. */
	void SetNursesAttackDoor(bool bAttack);

	/** TriggerVolume_1's ActorBeginOverlap (bound from the level's start): a nurse sets the garage lift's NurseNear. */
	UFUNCTION()
	void OnNurseLiftTrigger(AActor* OverlappedActor, AActor* OtherActor);

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

	UFUNCTION()
	void On06TunnelEnter();

	UFUNCTION()
	void On06DoorsLock();

	UFUNCTION()
	void On06ReachAmbulance();

	/** 06 Nurses: those Spawn Nurses_06 spawned (gone once destroyed). */
	TArray<TWeakObjectPtr<AWasamiEnemy06Chase>> Nurses06;
};
