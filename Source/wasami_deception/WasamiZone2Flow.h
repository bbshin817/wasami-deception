#pragma once

#include "CoreMinimal.h"
#include "WasamiZoneFlow.h"
#include "WasamiZone2Flow.generated.h"

/**
 * Zone 2's level Blueprint (pak_reference_2's 06_Hospital_Zone_02): out of the cell (checkpoint 7), past the Matron's
 * nurses (8), through the maze for all its shards (9), and on to the ring piece (10). The ambulance's arrival, the
 * capture and the cell's scenes (item 25) are left out: checkpoint 7 starts where the cell's scene ends (Cell Cutscene
 * Finished). The events keep the original's names in their comments and in GetSection.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZone2Flow : public AWasamiZoneFlow
{
	GENERATED_BODY()

public:
	/** Spikes_Death: the player dies 0.5 s after the spikes reach them. */
	static constexpr float SpikesDeathDelay = 0.5f;

protected:
	virtual void StartAt(int32 Checkpoint) override;

private:
	/** Miniboss Transition: the Matron's corridor. */
	void MinibossTransition();
	/** Maze Transition: the maze's shards wanted. */
	void MazeTransition();
	/** Postmaze Transition: the ring piece as the goal. */
	void PostmazeTransition();

	UFUNCTION()
	void OnCellCutsceneFinished();

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
};
