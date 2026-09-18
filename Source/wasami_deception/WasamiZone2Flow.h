#pragma once

#include "CoreMinimal.h"
#include "WasamiZoneFlow.h"
#include "WasamiZone2Flow.generated.h"

class UMaterialParameterCollection;

/**
 * Zone 2's level Blueprint (pak_reference_2's 06_Hospital_Zone_02): out of the cell (checkpoint 7) before its spikes come
 * down, past the Matron's nurses (8), through the maze for all its shards (9), and on to the ring piece (10), whose
 * screen breaks the barrier to the garage. The
 * ambulance's arrival, the capture and the cell's scenes (item 25) are left out: checkpoint 7 starts where the cell's
 * scene ends (Cell Cutscene Finished), in the state those scenes leave. The events keep the original's names in their
 * comments and in GetSection.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZone2Flow : public AWasamiZoneFlow
{
	GENERATED_BODY()

public:
	AWasamiZone2Flow();

	/** Spikes_Death: the player dies 0.5 s after the spikes reach them. */
	static constexpr float SpikesDeathDelay = 0.5f;

	/** Ring Piece Collect 's Delay before the garage's trigger is bound (and Bierce talks). */
	static constexpr float GarageBindDelay = 1.f;

	/** ReceiveBeginPlay: Mat_ParameterCol's Portal Extra Brightness in this zone (the garage's portal glows by it). */
	static constexpr float PortalExtraBrightness = 40.f;

	/** Where the left-out scenes leave what they move (their sections that keep their state). */
	static const FVector AmbulanceArrived;
	static const FVector FalseCeilingOpen;
	static const FRotator WallSwitchThrown;

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

private:
	/** The state the left-out arrival, capture and cell's scenes leave (checkpoint 7 only, as in the original). */
	void SkippedScenesEnd();
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

	UFUNCTION()
	void OnPostmazeTriggerGarage();
};
