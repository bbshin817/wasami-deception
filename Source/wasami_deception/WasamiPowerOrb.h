#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiPowerOrb.generated.h"

class AWasamiStunCollectEffect;
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
 * The power orb, after Dark Deception's Blueprints/Main/BP_PowerOrb (pak_reference_2). One is placed off the map; every
 * Shard Spawn Time (150 s) it flickers for 5 s where it is, then vanishes in a flash and appears at one of the level's
 * orb spawn points picked at random (the one it is at among them). The player touching it (also while it is flickered
 * out: only the looks go) takes it once: a flash, the pickup sound, ENEMIES STUNNED over the screen, a shake, the
 * countdown's music, BP_StunCollectEffect around the player, and every actor tagged Enemy is sent Set State(Stun, by
 * orb). Its mark on the tablet's map is a plane 20 m up. The telekinesis does not pull it.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPowerOrb : public AActor
{
	GENERATED_BODY()

public:
	AWasamiPowerOrb();

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Spawn Power Orb: the flicker (the root's visibility turned over now and every 0.1 s), and after 5 s the move to a
	 * spawn point with its flashes and the next Shard Spawn Time. A call while it flickers is ignored (the original's
	 * Delay is already running).
	 */
	UFUNCTION(BlueprintCallable, Category = "Power Orb")
	void SpawnPowerOrb();

	/**
	 * The end of the flicker's move without the flicker: the vanishing flash, the jump to spawn point Index (none when
	 * it is not one), the next Shard Spawn Time and the appearing flash.
	 */
	UFUNCTION(BlueprintCallable, Category = "Power Orb")
	void MoveToSpawnPoint(int32 Index);

	/** What the player's touch does, once: the pickup and the orb's end. */
	UFUNCTION(BlueprintCallable, Category = "Power Orb")
	void Collect();

	/** Every actor tagged Enemy that implements the enemy interface gets Set State(Stun, true). Returns how many did. */
	UFUNCTION(BlueprintCallable, Category = "Power Orb", meta = (WorldContext = "WorldContextObject"))
	static int32 StunAllEnemies(const UObject* WorldContextObject);

	/** Whether the 5 s flicker is running. */
	bool IsFlickering() const { return bFlickering; }

	/** Seconds to the next Spawn Power Orb (-1 when none is set: while it flickers, or once collected). */
	float GetTimeToSpawn() const;

	/** The spawn points in use (the level's orb spawn points unless given). */
	const TArray<TObjectPtr<AActor>>& GetSpawnPoints() const { return SpawnPoints; }

	UStaticMeshComponent* GetCrystal() const { return SoulShard; }
	UStaticMeshComponent* GetMapMark() const { return StaticMesh; }
	UCapsuleComponent* GetCapsule() const { return Capsule; }
	UPointLightComponent* GetLight() const { return PointLight; }

	/** Shard Spawn Time (s): from BeginPlay to the first flicker, and from each move to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power Orb")
	float ShardSpawnTime = 150.f;

	/**
	 * Spawn Points. Empty takes every AWasamiPowerOrbSpawnPoint of the level on BeginPlay (the original's editor button,
	 * Auto Assign Spawn Points, fills it with all of them by class).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Power Orb")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	/** The flicker's length (the Delay) and its period (the looping timer), in seconds. */
	static constexpr float FlickerLength = 5.f;
	static constexpr float FlickerPeriod = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UStaticMesh> CrystalMesh;

	/** m_crystal_Inst3. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UMaterialInterface> CrystalMaterial;

	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UStaticMesh> MapMarkMesh;

	/** M_PowerOrb. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UMaterialInterface> MapMarkMaterial;

	/** P_ky_flash_PowerOrb_Disappear and _Appear, at half size where the crystal leaves and arrives. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UParticleSystem> DisappearFlash;

	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UParticleSystem> AppearFlash;

	/** P_ky_impact, where the crystal is taken. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<UParticleSystem> CollectImpact;

	/** Soul_Shard_Pickup_v2_Cue, at 0.8 and pitch 0.75. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<USoundBase> PickupSound;

	/** 8-Dark_power_ball_countdown_: the 17 s the enemies lie stunned. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<USoundBase> CountdownSound;

	/** BP_CameraShake_Streak. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftClassPtr<UCameraShakeBase> CollectShake;

	/** BP_StunCollectEffect. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSubclassOf<AWasamiStunCollectEffect> CollectEffectClass;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Orb")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** The crystal (the original names it soul_shard): power_orb 125 cm up at 0.54; the capsule and light under it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Orb")
	TObjectPtr<UStaticMeshComponent> SoulShard;

	/** What the player touches: a ball of 45.5 cm (840.6 at 0.1 under the crystal's 0.54). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Orb")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Orb")
	TObjectPtr<UPointLightComponent> PointLight;

	/** The minimap's mark: the engine's plane with M_PowerOrb, 20 m over the orb. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power Orb")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

private:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Timer: Shard Spawn Time to the next Spawn Power Orb. */
	void StartSpawnTimer();
	/** Flicker: the root's visibility (with its children) turned over. */
	void Flicker();
	/** The Delay's end: the flicker stops, the orb shows and moves. */
	void FinishFlicker();
	void SpawnFlash(UParticleSystem* Template, float Scale) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;

	FTimerHandle SpawnTimer;
	FTimerHandle FlickerTimer;
	FTimerHandle FlickerDelay;
	/** Flicker's toggle, kept from one flicker to the next as the original's is. */
	bool bFlickerHidden = false;
	bool bFlickering = false;
	/** The pickup's DoOnce. */
	bool bCollected = false;
};
