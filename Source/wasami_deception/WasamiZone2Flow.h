#pragma once

#include "CoreMinimal.h"
#include "WasamiZoneFlow.h"
#include "WasamiZone2Flow.generated.h"

class UMaterialParameterCollection;

/**
 * Zone 2's level Blueprint (pak_reference_2's 06_Hospital_Zone_02): the ambulance's arrival and the capture (checkpoint
 * 7), out of the cell before its spikes come down, past the Matron's nurses (8), through the maze for all its shards
 * (9), and on to the ring piece (10), whose screen breaks the barrier to the garage. The events keep the original's
 * names in their comments and in GetSection.
 *
 * Where the original rides the ambulance from the garage to the boss fight, this game leaves by a portal in the garage,
 * as the hotel's exit is left (01_Hotel): the garage's trigger opens it, and a trigger by it (the hotel's EndTrigger) is
 * the escape.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZone2Flow : public AWasamiZoneFlow
{
	GENERATED_BODY()

public:
	AWasamiZone2Flow();

	/** Arrive Event: the ambulance's arrival plays 0.3 s after the player is moved to PlayerStart_1. */
	static constexpr float ArriveSequenceDelay = 0.3f;

	/** Cell Cutscene Start: the cell's scene plays 1 s after the capture ends. */
	static constexpr float CellSequenceDelay = 1.f;

	/** Cell Cutscene Finished: the view blends back from the scenes' cine camera to the player over 2 s. */
	static constexpr float CellViewBlendTime = 2.f;

	/**
	 * The cell's scene ducks BP_06_MusicPlayer_Zone2_2's Regular Music to 0.3 over 0.5 s as it starts (Bierce speaks
	 * through it) and brings it back to 1 over 2.5 s as it ends: the original's two FadeIn calls, which the flow makes
	 * itself since bFadeOut is not what the scene uses.
	 */
	static constexpr float CellMusicFadeIn = 0.5f;
	static constexpr float CellMusicVolume = 0.3f;
	static constexpr float CellEndMusicFadeIn = 2.5f;
	static constexpr float CellEndMusicVolume = 1.f;

	/** Spikes_Death: the player dies 0.5 s after the spikes reach them. */
	static constexpr float SpikesDeathDelay = 0.5f;

	/** Ring Piece Collect 's Delay before the garage's trigger is bound (and Bierce talks). */
	static constexpr float GarageBindDelay = 1.f;

	/** ReceiveBeginPlay: Mat_ParameterCol's Portal Extra Brightness in this zone (the garage's portal glows by it). */
	static constexpr float PortalExtraBrightness = 40.f;

	/** This game's garage portal and the trigger by it, which the level build places (dd_level.PORTALS). */
	static const FName GaragePortal;
	static const FName EscapeTrigger;

	/** BP_06_MusicPlayer_Zone2_2, this zone's music (placed with bFadeOut false, so the zone opens with its track). */
	static const FName MusicPlayerSource;

	/** The Matron over the miniboss corridor (the level's BP_06_Matron_MiniBoss), whom Activate MiniBoss Enemies wakes. */
	static const FName Matron;

	/** Postmaze Transition's secret file: a BP_Collectable of this ID at the target point collec's transform. */
	static const FName PostmazeFilePoint;
	static constexpr int32 PostmazeFileID = 3;

	/**
	 * The escape's fade: UMG_BlackFade_2's FadeIn at this rate, black in 0.25 s (as the portal's own flash, UMG_BlackFade
	 * at 2, peaks and moves the player), at BP_00_Teleport's Z order.
	 */
	static constexpr float EscapeFadeSpeed = 20.f;
	static constexpr int32 EscapeFadeZOrder = 5;

protected:
	/** ReceiveBeginPlay: Portal Extra Brightness set, then the zone's Setup (the base's BeginPlay). */
	virtual void BeginPlay() override;

	virtual void StartAt(int32 Checkpoint) override;

	/** Mat_ParameterCol, which the portal's materials read. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<UMaterialParameterCollection> ParameterCollection;

	/** The cell's door picked: BP_04_BossFight_CameraShake_Initial. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftClassPtr<UCameraShakeBase> DoorPickedShakeClass;

	/** The spikes reaching the player: DD_Needle_Trap_R1_V3 (PlaySound2D), with BP_HitFX. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundBase> SpikesSound;

	/** The ring piece taken: Ring_Piece_Pickup_v1 (PlaySound2D) as its screen comes up. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundBase> RingPiecePickupSound;

	/** The escape: 21-Ballroom_portal_V2 (PlaySound2D), as the ambulance's ride ends in the original. */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Zone")
	TSoftObjectPtr<USoundBase> EscapeSound;

private:
	/**
	 * Arrive Event: the player at PlayerStart_1 and, 0.3 s on, the ambulance's arrival played, which takes neither the
	 * view nor the input; walking into Trigger_Arrive_CaptureScene brings the capture.
	 */
	void ArriveEvent();
	/** Miniboss Transition: the Matron's corridor. */
	void MinibossTransition();
	/** Activate MiniBoss Enemies: the sentry nurses start looking. */
	void ActivateMinibossEnemies();
	/** Maze Transition: the maze's shards wanted. */
	void MazeTransition();
	/** Spawn Nurses: BP_06_ReaperNurse_Zone2 at NurseSpawn_4, _1 and _2. */
	void SpawnNurses();
	/** Postmaze Transition: the ring piece as the goal. */
	void PostmazeTransition();
	/** BP_06_MusicPlayer_Zone2_2's Regular Music FadeIn, as the cell's scene ducks the music and gives it back. */
	void FadeCellMusicIn(float Duration, float Volume);

	/** Escape_AmbulanceArrive: the arrival over, the blocker at the ambulance's front goes. */
	UFUNCTION()
	void OnEscapeAmbulanceArrive();

	/** Arrive_CaptureCutscene: the player caught, which takes their input and the view. */
	UFUNCTION()
	void OnArriveCaptureCutscene();

	/** Cell Cutscene Start: 1 s on, the cell's scene, with the player moved into the cell as it opens. */
	UFUNCTION()
	void OnCellCutsceneStart();

	UFUNCTION()
	void OnCellCutsceneFinished();

	UFUNCTION()
	void OnCellDoorBreak();

	UFUNCTION()
	void OnSpikesDeath();

	UFUNCTION()
	void OnMinibossBierceTalk();

	UFUNCTION()
	void OnMinibossTriggerTransition();

	UFUNCTION()
	void OnMinibossBehindMatron();

	UFUNCTION()
	void OnMazeTriggerStart();

	UFUNCTION()
	void OnMazeAllShards();

	/** Collected Ring Piece: ring_statue_2's Interact All Shards puts up the ring piece's screen. */
	UFUNCTION()
	void OnCollectedRingPiece();

	/** Ring Piece Collect : the screen's Close breaks the barrier and sends the player to the garage. */
	UFUNCTION()
	void OnRingPieceCollect();

	/** Postmaze_Trigger_Garage: in the garage, its portal opens (where the original sends the player to the ambulance). */
	UFUNCTION()
	void OnPostmazeTriggerGarage();

	/**
	 * The hotel's EndTrigger, by the portal: the escape (the player stopped, the screen black, the enemies gone), then
	 * the game mode's Escape (the game paused, the save, the score screen).
	 */
	UFUNCTION()
	void OnEndTrigger();
};
