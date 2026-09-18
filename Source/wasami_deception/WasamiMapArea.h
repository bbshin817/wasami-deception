#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiMapArea.generated.h"

class UBoxComponent;

/**
 * Dark Deception's BP_MapArea (pak_reference_2's Blueprints/Main/MinimapMultifloor): a box over one floor of a level
 * with a map for each floor (the hospital's Zone 2, two). AWasamiMapTextureMultiFloor shows the map of the one the
 * player is in, and on it only the shards in it. The level build places them, scaled to the floor.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMapArea : public AActor
{
	GENERATED_BODY()

public:
	AWasamiMapArea();

	/** Get Shards: the shards (AWasamiShard) overlapping the box. */
	void GetShards(TArray<AActor*>& Shards) const;

	UBoxComponent* GetBox() const { return Box; }

protected:
	/**
	 * Box, the root (the original's SCS puts it in place of the default scene root): UBoxComponent's 32 cm, the level's
	 * actors scale it to the floor. It overlaps world static and dynamic objects and pawns and ignores the rest.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Map Area")
	TObjectPtr<UBoxComponent> Box;
};
