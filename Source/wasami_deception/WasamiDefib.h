#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiDefib.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraShakeBase;
class UParticleSystem;
class UParticleSystemComponent;
class USoundAttenuation;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Dark Deception's BP_06_Defib (pak_reference_2's Blueprints/06_Hospital), the hospital's trap of two defibrillator
 * stands 4 m apart: while the player is within 10 m of it, it charges (the zap's sound) and 2.25 s later fires, sparks
 * leaping between the stands and the camera shaking, and 1.25 s after that charges again. A player between the stands
 * as it fires, or walking in while the sparks last (1 s), is hit: the sparks' sound, the hit's flash, and 0.2 s later
 * the game mode's DeathEvent. The hospital places 23 in Zone 1 and 13 in Zone 2; its level Blueprints name none.
 *
 * The stands' mesh and material are not set by the class (assets under /Game/DD are never loaded from a constructor,
 * WasamiAssets.h): the level build places the defibrillators with hospital_defibrillator_01 and
 * M_06_Hospital_Defibrillator on both. The sounds, the sparks and the shake are loaded when play begins.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiDefib : public AActor
{
	GENERATED_BODY()

public:
	AWasamiDefib();

	/** Activated: the player coming into Sphere starts the charging (the CDO's true; no placed one clears it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defib")
	bool bActivated = true;

	/** Charge's Delay: from the zap's sound to Fire. */
	static constexpr float ChargeDelay = 2.25f;
	/** The Delay after Fire before Reset Charge (and the next Charge while the player stays in Sphere). */
	static constexpr float RechargeDelay = 1.25f;
	/** Fire's Delay: how long Firing lasts. */
	static constexpr float FiringTime = 1.f;
	/** Player Hit's Delay: from the hit to DeathEvent. */
	static constexpr float DeathDelay = 0.2f;
	/** The hit's flash's Shake Scale. */
	static constexpr float HitShakeScale = 5.f;

	/**
	 * Charge (a DoOnce, opened by Reset Charge): the zap's sound from the start, and ChargeDelay later Fire; RechargeDelay
	 * after that the DoOnce opens, and Charge comes again while Looping.
	 */
	UFUNCTION(BlueprintCallable, Category = "Defib")
	void Charge();

	/**
	 * Fire: Firing for FiringTime, both sparks started over, BP_Portal_CameraShake from the actor (0 to 5000 cm, falloff 1,
	 * turned toward it), and Player Hit if the player is in Box.
	 */
	UFUNCTION(BlueprintCallable, Category = "Defib")
	void Fire();

	/**
	 * Player Hit (a DoOnce, never opened again): Electric_Sparks_08 (2D), BP_HitFX with a Shake Scale of 5, and DeathDelay
	 * later the game mode's DeathEvent with the player as the cause.
	 */
	UFUNCTION(BlueprintCallable, Category = "Defib")
	void PlayerHit();

	/** Looping: the player is in Sphere (set as the player comes in, cleared as they leave). */
	bool IsLooping() const { return bLooping; }
	/** Charge's DoOnce closed: from the zap until Reset Charge. */
	bool IsCharging() const { return bChargeClosed; }
	bool IsFiring() const { return bFiring; }
	/** Player Hit has gone through. */
	bool HasHitPlayer() const { return bHitClosed; }

	UStaticMeshComponent* GetDefibrillator01() const { return Defibrillator01; }
	UStaticMeshComponent* GetDefibrillator02() const { return Defibrillator02; }
	UParticleSystemComponent* GetParticleSystem() const { return ParticleSystem; }
	UParticleSystemComponent* GetParticleSystem1() const { return ParticleSystem1; }
	UBoxComponent* GetBox() const { return Box; }
	USphereComponent* GetSphere() const { return Sphere; }
	UAudioComponent* GetZap() const { return Zap; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** hospital_defibrillator_01: the stand at y −200. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UStaticMeshComponent> Defibrillator01;

	/** hospital_defibrillator_02: the stand at y +200, turned to face the other. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UStaticMeshComponent> Defibrillator02;

	/** ParticleSystem: P_06_Defib at the first stand's paddles. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UParticleSystemComponent> ParticleSystem;

	/** ParticleSystem1: P_06_Defib at the second's. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UParticleSystemComponent> ParticleSystem1;

	/** Box: the gap between the stands (64 × 383 × 258 cm), overlapping pawns only. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UBoxComponent> Box;

	/** Sphere: 10 m around, where the player starts the charging. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<USphereComponent> Sphere;

	/** DD_TT_Defibrillator_Zap: the charge's sound at 0.6 through MonkeyAttenuation. */
	UPROPERTY(VisibleAnywhere, Category = "Defib")
	TObjectPtr<UAudioComponent> Zap;

	UPROPERTY(EditAnywhere, Category = "Defib|Assets")
	TSoftObjectPtr<USoundBase> ZapSound;

	UPROPERTY(EditAnywhere, Category = "Defib|Assets")
	TSoftObjectPtr<USoundAttenuation> ZapAttenuation;

	UPROPERTY(EditAnywhere, Category = "Defib|Assets")
	TSoftObjectPtr<UParticleSystem> SparksParticle;

	/** Electric_Sparks_08, as the player is hit. */
	UPROPERTY(EditAnywhere, Category = "Defib|Assets")
	TSoftObjectPtr<USoundBase> HitSound;

	/** BP_Portal_CameraShake, as it fires. */
	UPROPERTY(EditAnywhere, Category = "Defib|Assets")
	TSoftClassPtr<UCameraShakeBase> FireShakeClass;

private:
	/** Reset Charge: opens Charge's DoOnce, and charges again while Looping. */
	void ResetCharge();

	bool IsPlayer(const AActor* Actor) const;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedHitSound;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedFireShake;

	bool bLooping = false;
	bool bFiring = false;
	/** Charge's DoOnce. */
	bool bChargeClosed = false;
	/** Player Hit's DoOnce. */
	bool bHitClosed = false;
	FTimerHandle ChargeTimer;
	FTimerHandle RechargeTimer;
	FTimerHandle FiringTimer;
	FTimerHandle DeathTimer;
};
