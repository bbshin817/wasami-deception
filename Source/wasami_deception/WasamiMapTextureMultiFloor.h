#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "WasamiMapTextureMultiFloor.generated.h"

class AWasamiMapArea;
class UMaterialInstanceDynamic;
class UTexture;

/**
 * Dark Deception's BP_MapTexture_MultiFloor (pak_reference_2's Blueprints/Main/MinimapMultifloor, a BP_MapTexture): the
 * zone's map plane under the level, which only the player's minimap capture draws, for a level with a map for each
 * floor. Every 0.9 s (Check Map) it puts on the plane the map of the floor box (AWasamiMapArea) the player is in, from
 * Map, and shows on the map only the marks of the shards in that box. Out of every box it leaves both as they are. The
 * level build places Zone 2's with the zone's map material and its Map.
 *
 * Not copied: the original makes the plane's material anew of MM_Map_Parent, whose own texture is another chapter's map
 * until the first Check Map; here it is made of the plane's own material, the zone's first-floor map.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiMapTextureMultiFloor : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AWasamiMapTextureMultiFloor();

	/** Check Map: the player's floor box, its map and its shards' marks (above). */
	UFUNCTION()
	void CheckMap();

	AWasamiMapArea* GetCurrentMapArea() const { return CurrentMapArea; }
	UMaterialInstanceDynamic* GetDynamicMat() const { return DynamicMat; }

	/** Map: each floor box's map texture. */
	UPROPERTY(EditAnywhere, Category = "Map Texture")
	TMap<TObjectPtr<AWasamiMapArea>, TObjectPtr<UTexture>> Map;

	/** Check Map's looping timer (s). */
	static constexpr float CheckMapRate = 0.9f;

	/** The map material's texture parameter. */
	static const FName TextureParameter;

protected:
	virtual void BeginPlay() override;

private:
	/** Current Map Area: the box Check Map last found the player in. */
	UPROPERTY(Transient)
	TObjectPtr<AWasamiMapArea> CurrentMapArea;

	/** Dynamic Mat: the plane's material, made at BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

	FTimerHandle CheckMapTimer;
};
