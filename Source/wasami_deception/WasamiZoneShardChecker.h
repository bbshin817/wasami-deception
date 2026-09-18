#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiZoneShardChecker.generated.h"

class UBoxComponent;

/**
 * Dark Deception's BP_ZoneShardChecker (pak_reference_2's Blueprints/Shared): a box over a zone of a level, the tablet's
 * arrow's area (AWasamiArrowPointer points at the nearest shard in the box the player is in). Each shard collected, it
 * looks again a second later (the game mode's Collect Shard, then a Delay) and, with no shard left in the box, is gone.
 * The level build places one over each hospital zone, scaled to it.
 *
 * Not copied: the development PrintText as it goes, its Collected dispatcher (which nothing binds), and Remove / Collect
 * All In Zone, which only other chapters' levels call.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZoneShardChecker : public AActor
{
	GENERATED_BODY()

public:
	AWasamiZoneShardChecker();

	/** Check Shards: in a second, gone if its box has no shard left. A check already waiting is not put back (Delay). */
	UFUNCTION()
	void CheckShards();

	/** The shards (AWasamiShard) overlapping the box. */
	void GetShards(TArray<AActor*>& Shards) const;

	UBoxComponent* GetBox() const { return Box; }

	/** Check Shards' Delay (s). */
	static constexpr float CheckShardsDelay = 1.f;

protected:
	virtual void BeginPlay() override;

	/** DefaultSceneRoot: the level's actors scale it to the zone. */
	UPROPERTY(VisibleAnywhere, Category = "Zone Shard Checker")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Box: UBoxComponent's defaults (32 cm, OverlapAllDynamic), as the original's. */
	UPROPERTY(VisibleAnywhere, Category = "Zone Shard Checker")
	TObjectPtr<UBoxComponent> Box;

private:
	/** After the Delay: gone with no shard left. */
	void CheckShardsLeft();

	FTimerHandle CheckShardsTimer;
};
