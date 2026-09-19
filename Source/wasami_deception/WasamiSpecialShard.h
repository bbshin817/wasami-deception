#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSpecialShard.generated.h"

class UCameraShakeBase;
class UCapsuleComponent;
class UMaterialInterface;
class UParticleSystem;
class UPointLightComponent;
class UPrimitiveComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;
struct FHitResult;

/**
 * What Dark Deception's BP_PowerOrb and BP_BonusShard (pak_reference_2) share: the same components and the same cycle,
 * with values of their own. One is placed off the map; every Shard Spawn Time (150 s) it flickers for 5 s where it is,
 * then vanishes in a flash and appears at one of the level's spawn points of its kind picked at random (the one it is
 * at among them). The player touching it (also while it is flickered out: only the looks go) takes it once; what that
 * does is the subclass's. Its mark on the tablet's map is a plane 20 m up. The telekinesis does not pull it.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiSpecialShard : public AActor
{
	GENERATED_BODY()

public:
	AWasamiSpecialShard();

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Spawn Power Orb / Spawn Special Shard: the flicker (the root's visibility turned over now and every 0.1 s), and
	 * after 5 s the move to a spawn point with its flashes and the next Shard Spawn Time. A call while it flickers is
	 * ignored (the original's Delay is already running), as is one once it is taken.
	 */
	UFUNCTION(BlueprintCallable, Category = "Special Shard")
	void SpawnSpecialShard();

	/**
	 * The end of the flicker's move without the flicker: the vanishing flash, the jump to spawn point Index (none when
	 * it is not one), the next Shard Spawn Time and the appearing flash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Special Shard")
	void MoveToSpawnPoint(int32 Index);

	/** What the player's touch does, once (the original's DoOnce): the spawn timer cleared, then the subclass's pickup. */
	UFUNCTION(BlueprintCallable, Category = "Special Shard")
	void Collect();

	/** Whether the 5 s flicker is running. */
	bool IsFlickering() const { return bFlickering; }

	/** Whether it was taken. */
	bool IsCollected() const { return bCollected; }

	/** Seconds to the next Spawn Special Shard (-1 when none is set: while it flickers, or once collected). */
	float GetTimeToSpawn() const;

	/** The spawn points in use (the level's spawn points of its kind unless given). */
	const TArray<TObjectPtr<AActor>>& GetSpawnPoints() const { return SpawnPoints; }

	UStaticMeshComponent* GetCrystal() const { return SoulShard; }
	UStaticMeshComponent* GetMapMark() const { return StaticMesh; }
	UCapsuleComponent* GetCapsule() const { return Capsule; }
	UPointLightComponent* GetLight() const { return PointLight; }

	/** Where the crystal is, or was when it was removed. */
	FVector GetCrystalLocation() const;

	/** Shard Spawn Time (s): from BeginPlay to the first flicker, and from each move to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Shard")
	float ShardSpawnTime = 150.f;

	/**
	 * Spawn Points. Empty takes every actor of SpawnPointClass in the level on BeginPlay (the original's editor button,
	 * Auto Assign Spawn Points, fills it with all of them by class).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Special Shard")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	/** The flicker's length (the Delay) and its period (the looping timer), in seconds. */
	static constexpr float FlickerLength = 5.f;
	static constexpr float FlickerPeriod = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UStaticMesh> CrystalMesh;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UMaterialInterface> CrystalMaterial;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UStaticMesh> MapMarkMesh;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UMaterialInterface> MapMarkMaterial;

	/** The flashes where the crystal leaves and arrives, at MoveFlashScale. */
	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UParticleSystem> DisappearFlash;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UParticleSystem> AppearFlash;

	/** The flash where the crystal is taken, at 1. */
	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<UParticleSystem> CollectImpact;

	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftObjectPtr<USoundBase> PickupSound;

	/** BP_CameraShake_Streak. */
	UPROPERTY(EditAnywhere, Category = "Special Shard|Assets")
	TSoftClassPtr<UCameraShakeBase> CollectShake;

protected:
	virtual void BeginPlay() override;

	/** Loads what the pickup uses besides the base's assets into Out, so that it waits on no load. */
	virtual void LoadPickupAssets(TArray<TObjectPtr<UObject>>& Out) const {}

	/** What BeginPlay does once the spawn points are in: the spawn timer. */
	virtual void BeginCycle() { StartSpawnTimer(); }

	/** The pickup, after Collect has closed the DoOnce and cleared the spawn timer. */
	virtual void CollectShard() {}

	/** Timer: Shard Spawn Time to the next Spawn Special Shard. */
	void StartSpawnTimer();

	/** SpawnEmitterAtLocation(Template, the crystal's location, no rotation, Scale, auto destroy). */
	void SpawnFlash(UParticleSystem* Template, float Scale) const;

	/** The pickup's shake: PlayCameraShake(BP_CameraShake_Streak, 1, CameraLocal) on player 0's camera. */
	void PlayCollectShake() const;

	/** DestroyComponent on the crystal, keeping where it was for the flashes. */
	void RemoveCrystal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Shard")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** The crystal (the original names it soul_shard); the capsule and the light are under it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Shard")
	TObjectPtr<UStaticMeshComponent> SoulShard;

	/** What the player touches: a ball under the crystal at 0.1. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Shard")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Shard")
	TObjectPtr<UPointLightComponent> PointLight;

	/** The minimap's mark: the engine's plane, 20 m over the root. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Special Shard")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** What the spawn points are when none are given. */
	TSubclassOf<AActor> SpawnPointClass;

	/** The move's flashes' scale. */
	float MoveFlashScale = 1.f;

	FTimerHandle SpawnTimer;
	FTimerHandle FlickerTimer;

private:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Flicker: the root's visibility (with its children) turned over. */
	void Flicker();
	/** The Delay's end: the flicker stops, the root shows and it moves. */
	void FinishFlicker();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	FTimerHandle FlickerDelay;
	FVector RemovedCrystalLocation = FVector::ZeroVector;
	/** Flicker's toggle, kept from one flicker to the next as the original's is. */
	bool bFlickerHidden = false;
	bool bFlickering = false;
	/** The pickup's DoOnce. */
	bool bCollected = false;
};
