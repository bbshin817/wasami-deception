#pragma once

#include "CoreMinimal.h"
#include "WasamiEnemy.h"
#include "WasamiEnemyZone2.generated.h"

class AWasamiLift;

/**
 * The nurses of Zone 2's maze, after Dark Deception's BP_06_ReaperNurse_Zone2 (pak_reference_2), a BP_06_ReaperNurse:
 * Spawn Nurses spawns three as the maze begins. The maze has two floors that only the lifts join, so when it and the
 * player are not on the same one (is Up? against is Player Up?), Chase Player moves to the closest lift instead of the
 * player (Player Target) and Not Seeing Player walks to that lift's Move Location instead of the random point (Random
 * Point Destination). The rest is the nurse's.
 *
 * Get Closest Lift takes every BP_06_LiftBase (the BP_06_Lift_03 and _04 lifts, AWasamiLift; the corner lifts are not
 * BP_06_LiftBase) and MoreporkFunctions' GetFurthestOrClosestActor from the world's origin, not from the nurse, so every
 * nurse takes the same lift (the one nearest the origin), wherever it is.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiEnemyZone2 : public AWasamiEnemy
{
	GENERATED_BODY()

public:
	/** is Up?: its Z over this (cm). */
	static constexpr double UpperFloorZ = 640.;
	/** is Player Up?: the player's Z over this (cm). */
	static constexpr double PlayerUpperFloorZ = 610.;
	/** GetFurthestOrClosestActor's first best (a squared distance): a lift farther than its root is never taken. */
	static constexpr double ClosestLiftStart = 1e9;

	/** is Up?: it stands on the upper floor. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsUp() const;

	/** is Player Up?: the player stands on the upper floor (not without a player). */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsPlayerUp() const;

	/** Same Level As Player?: is Up? equals is Player Up?. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsSameLevelAsPlayer() const { return IsUp() == IsPlayerUp(); }

	/**
	 * Get Closest Lift: the lift whose actor is nearest the world's origin (the last of equals), and where its Move
	 * Location is now (zero without one).
	 */
	AWasamiLift* GetClosestLift(FVector& OutMoveLocation) const;

	/** Player Target: the player on the same floor, else the closest lift. */
	virtual AActor* GetPlayerTarget() const override;

	/** Random Point Destination: the random point on the same floor, else the closest lift's Move Location. */
	virtual FVector GetRandomPointDestination() const override;
};
