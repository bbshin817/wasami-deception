#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiArrowPointer.generated.h"

class AWasamiPlayerCharacter;
class UCameraComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Dark Deception's BP_ArrowPointer (pak_reference_2's Blueprints/Main), the arrow on the tablet's map. The player carries
 * it as a child actor 2000 cm over its mesh, where the minimap's capture draws it over the level's map plane: the
 * engine's Plane with M_Arrow_Inst, turned smoothly about the vertical to point from the player's camera at the target,
 * larger as the target comes near and larger still while the map is zoomed out. With Shards? the target is the shard
 * nearest the player among those in the zone shard checker's box the player stands in, once fewer than 100 are left
 * there; otherwise it is what the zone gives. Change Color tints it.
 *
 * What the original's level Blueprints set on the arrow (Shards?, Change Color and Target) the zone's flow holds
 * (AWasamiZoneFlow::IsArrowOnShards and the rest); the arrow takes it up each time it looks for its target (Find
 * Object). The plane's material is loaded when play begins (WasamiAssets.h).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiArrowPointer : public AActor
{
	GENERATED_BODY()

public:
	AWasamiArrowPointer();

	/** Change Color: the plane's materials' Color (its RGB; dynamic instances made as needed). */
	UFUNCTION(BlueprintCallable, Category = "Arrow")
	void ChangeColor(const FLinearColor& Color);

	/**
	 * Find Object (every 0.1 s, while the player is this game's): with Shards?, hidden out of every zone shard checker's
	 * box and while 100 or more shards are left in it; shown at the nearest shard (FindClosestShard) while some are, and
	 * shown at the target it had when none is. Without, shown while the target is there. The zone's values first.
	 */
	void FindObject();

	/** Set Rotation (every 0.005 s): the yaw from the player's camera to the target, while there is one. */
	void SetRotation();

	/**
	 * Smooth Rotation (every 0.005 s): turns toward that yaw (RInterpTo, 20) and, while there is a target, eases the
	 * size (FInterpTo, 5) to 7 near it and 4 from 3000 cm on, 10 more while the map is zoomed out: (X, X, 1).
	 */
	void SmoothRotation();

	/**
	 * FindClosestShard: of the shards in the box of the first zone shard checker the player overlaps, the one nearest
	 * the player (within 99999 cm), or null.
	 */
	AActor* FindClosestShard() const;

	/** Shards?: point at the nearest shard of the zone (true on the class, as the original's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrow")
	bool bShards = true;

	AActor* GetTarget() const { return Target.Get(); }
	void SetTarget(AActor* NewTarget) { Target = NewTarget; }

	/** Target Rotation: where Set Rotation last pointed. */
	FRotator GetTargetRotation() const { return TargetRotation; }

	UStaticMeshComponent* GetPlane() const { return Plane; }

	/** Where the player carries it (the ChildActorComponent on its mesh). */
	static const FVector PlayerRelativeLocation;
	static const FVector PlayerRelativeScale;

	/** The timers of ReceiveBeginPlay (s). */
	static constexpr float RotationRate = 0.005f;
	static constexpr float FindObjectRate = 0.1f;

	/** Find Object points at shards while fewer than this are left in the zone's box. */
	static constexpr int32 ShardsPointedBelow = 100;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Plane: the engine's Plane, turned 90° and 1.5 times its size, casting no shadow. */
	UPROPERTY(VisibleAnywhere, Category = "Arrow")
	TObjectPtr<UStaticMeshComponent> Plane;

	/** M_Arrow_Inst (made by WasamiDDTools.import_dd_tablet). */
	UPROPERTY(EditAnywhere, Category = "Arrow|Assets")
	TSoftObjectPtr<UMaterialInterface> ArrowMaterial;

private:
	/** What the zone's flow gives the arrow: Shards?, its target without, and a colour not yet given. */
	void TakeUpZone();

	/** The first zone shard checker the player overlaps and the shards in its box. */
	AActor* ZoneShards(TArray<AActor*>& Shards) const;

	TWeakObjectPtr<AWasamiPlayerCharacter> Player;
	TWeakObjectPtr<UCameraComponent> PlayerCamera;
	TWeakObjectPtr<AActor> Target;
	FRotator TargetRotation = FRotator::ZeroRotator;
	TOptional<FLinearColor> Color;

	// Looping, as the original's K2_SetTimer (several calls in a frame longer than the rate, each with the frame's
	// delta, as UE's timer manager does).
	FTimerHandle SetRotationTimer;
	FTimerHandle SmoothRotationTimer;
	FTimerHandle FindObjectTimer;
};
