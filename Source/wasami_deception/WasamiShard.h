#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiTelekinesisInterface.h"
#include "WasamiShard.generated.h"

class UCameraShakeBase;
class UCapsuleComponent;
class UMaterialInterface;
class UParticleSystem;
class UPointLightComponent;
class UPrimitiveComponent;
class USoundBase;
class USoundConcurrency;
class UStaticMesh;
class UStaticMeshComponent;
struct FHitResult;

/**
 * A soul shard, after the latest version's Blueprints/Main/BP_Shard (pak_reference_2). Its purple light, its pickup
 * capsule and its mark for the tablet's minimap (a plane 20 m up that only the map's scene capture sees from above) are
 * the original's; the shard itself is this game's Wasami mochi, turning as the original's crystal does. Touching it
 * collects it: the tablet's count goes down by one with Count Shake, the camera shakes, a flash bursts where the
 * crystal was (the original's P_ky_flash3; this game's is P_WasamiShardFlash, purple and weaker), the pickup sound
 * plays (one at a time, OnlyFew) and the shard is gone. The telekinesis (Activate) pulls it to the player over about a
 * second and collects it at the end even when it has not arrived.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiShard : public AActor, public IWasamiTelekinesisInterface
{
	GENERATED_BODY()

public:
	AWasamiShard();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Collect(NoSound?), once: the count and Count Shake on the player's tablet, the collect shake, the flash, the pickup
	 * sound (silent with NoSound) and the shard's end. Like the original, it stops after closing itself when there is no
	 * player or tablet screen to count on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shard")
	void Collect(bool bNoSound);

	/** Whether Shard Pull is playing. */
	bool IsPulling() const { return PullTime >= 0.f; }

	/** Shard Pull's play rate, set when it starts. */
	float GetPullRate() const { return PullRate; }

	/** The mochi's play rate, the original's SetPlayRate on its crystal's animation. */
	float GetSpinRate() const { return SpinRate; }

	/** Shard Pull's Alpha track at Seconds of the timeline: 0 → 1 over its first 0.75 s. */
	static float EvaluatePullAlpha(float Seconds);

	/** Where the pull puts a shard that started at From toward Player at Alpha: ExpoIn, the start's height kept. */
	static FVector PullLocation(const FVector& From, const FVector& Player, float Alpha);

	/** How fast the mochi turns at a play rate (° a second, yaw): the crystal's animation turns it twice a loop. */
	static float SpinSpeed(float PlayRate);

	/** Shard Pull's length (s) at a play rate of 1. */
	static constexpr float PullLength = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void Activate_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** Where the original's SkeletalMesh sits (97 cm up, scaled 10): the light and the capsule hang under it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<USceneComponent> Body;

	/** This game's shard, in place of the original's crystal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<UStaticMeshComponent> Mochi;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<UPointLightComponent> PointLight;

	/** What the player touches (a ball of 49.57 cm) and what the telekinesis finds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<UCapsuleComponent> Capsule;

	/** The minimap's mark: the engine's plane with M_Shard, Minimap Plane Height over the shard. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shard")
	TObjectPtr<UStaticMeshComponent> Plane;

	/** Light Intensity, which the construction script gives the light. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shard")
	float LightIntensity = 175.f;

	/** Minimap Plane Height (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shard")
	float MinimapPlaneHeight = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<UStaticMesh> MochiMesh;

	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<UStaticMesh> PlaneMesh;

	/** M_Shard. */
	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<UMaterialInterface> MapMarkMaterial;

	/** Soul_Shard_Pickup_v2_Cue. */
	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<USoundBase> PickupSound;

	/** OnlyFew: one pickup sound at a time. */
	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<USoundConcurrency> PickupConcurrency;

	/** BP_CameraShake_ShardCollect. */
	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftClassPtr<UCameraShakeBase> CollectShake;

	/** P_WasamiShardFlash (this game's purple, weaker P_ky_flash3), spawned at the crystal's place (Body) at a fifth of
	 * its size. */
	UPROPERTY(EditAnywhere, Category = "Shard|Assets")
	TSoftObjectPtr<UParticleSystem> CollectFlash;

private:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TickPull(float DeltaSeconds);
	void ApplyPull();

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedPickupSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundConcurrency> LoadedPickupConcurrency;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedCollectShake;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> LoadedCollectFlash;

	/** Previous Location: where the shard was when play began, which the pull starts from. */
	FVector PreviousLocation = FVector::ZeroVector;
	float SpinRate = 1.f;
	float SpinAngle = 0.f;
	/** Seconds into Shard Pull; negative when it is not playing. */
	float PullTime = -1.f;
	float PullRate = 1.f;
	/** Collect's DoOnce. */
	bool bCollected = false;
};
