#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSpeedBarrier.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraShakeBase;
class UParticleSystem;
class UPointLightComponent;
class USoundBase;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiSpeedBarrierDestroyedSignature);

/**
 * Dark Deception's BP_SpeedBarrier (pak_reference_2's Blueprints/Main/Traps), a glowing red wall that only the speed
 * boost breaks: two planes 3 cm apart with pulsing materials (the zone barrier's master, AWasamiZoneBarrier), a red
 * light and a humming loop. The planes block nothing; Box1, a thin box just behind the front plane, blocks pawns, and
 * Box, a deeper one about it, overlaps them. The player coming into Box, or running into Box1 (ReceiveHit, while Can Be
 * Destroyed?), with the speed boost on (in Active Powers) shatters it: the portal's burst and one of P_ky_impact2, the
 * camera shaking, the shatter sound, Destroyed, and the actor gone. Without the speed boost (walking or sprinting) Box1 only stops the
 * player. Nothing else refers to them (the levels' Blueprints do not); Zone 1 of the hospital has four.
 *
 * The planes' materials are not set by the class (assets under /Game/DD are never loaded from a constructor,
 * WasamiAssets.h): the level build places the barriers with MM_SpeedBarrier_Inst and _Inst2 on them.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSpeedBarrier : public AActor
{
	GENERATED_BODY()

public:
	AWasamiSpeedBarrier();

	/** Can Be Destroyed?: whether running into Box1 with the speed boost breaks it (coming into Box does either way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed Barrier")
	bool bCanBeDestroyed = true;

	/** Destroyed (the original's event dispatcher; AActor has its own OnDestroyed): the barrier is shattering. */
	UPROPERTY(BlueprintAssignable, Category = "Speed Barrier")
	FWasamiSpeedBarrierDestroyedSignature OnBarrierDestroyed;

	/**
	 * The original's @25, where the hit and the overlap go: if the player is using the speed boost, the DoOnce: the
	 * portal's burst at StaticMesh (0.3 times its size), the impact burst there too (4), the camera shake (3), the
	 * shatter (2D, 0.8), Destroyed, and the actor destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Speed Barrier")
	void BreakIfBoosting();

	/** Whether the DoOnce has let the break through. */
	bool HasShattered() const { return bShattered; }

	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	UStaticMeshComponent* GetStaticMesh1() const { return StaticMesh1; }
	UPointLightComponent* GetPointLight() const { return PointLight; }
	UBoxComponent* GetBox() const { return Box; }
	UBoxComponent* GetBox1() const { return Box1; }
	UAudioComponent* GetAudio() const { return Audio; }

protected:
	virtual void BeginPlay() override;

	/** ReceiveHit (@761): the player, while Can Be Destroyed?, goes on to BreakIfBoosting. Only Box1 blocks anything. */
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Box's overlap (@840): the player goes on to BreakIfBoosting, whatever Can Be Destroyed? is. */
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** DefaultSceneRoot: the level's actors scale it to the barrier's size. */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** StaticMesh1: the front plane (the engine's Plane, standing up), MM_SpeedBarrier_Inst. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UStaticMeshComponent> StaticMesh1;

	/** StaticMesh: the back plane, 3 cm behind and a little larger, MM_SpeedBarrier_Inst2. The burst comes from it. */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UPointLightComponent> PointLight;

	/** Box: about the planes, 1 m deep on each side at the class's size, overlapping (the shape's OverlapAllDynamic). */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UBoxComponent> Box;

	/** Audio: Barrier_Loop at 0.3, with its own attenuation (natural, falling off over 2000 cm). */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UAudioComponent> Audio;

	/** Box1: just behind the front plane, 35 cm thick at the class's size, blocking pawns in queries. */
	UPROPERTY(VisibleAnywhere, Category = "Speed Barrier")
	TObjectPtr<UBoxComponent> Box1;

	UPROPERTY(EditAnywhere, Category = "Speed Barrier|Assets")
	TSoftObjectPtr<USoundBase> LoopSound;

	/** Barrier_Shatter, 2D, as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Speed Barrier|Assets")
	TSoftObjectPtr<USoundBase> ShatterSound;

	/** PPP_PortalAppear (PyroParticlePack), the portal's opening burst, as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Speed Barrier|Assets")
	TSoftObjectPtr<UParticleSystem> AppearParticle;

	/** P_ky_impact2 (AdvancedMagicFX13), as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Speed Barrier|Assets")
	TSoftObjectPtr<UParticleSystem> BreakParticle;

	/** BP_01_DoorExplode_CameraShake, as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Speed Barrier|Assets")
	TSoftClassPtr<UCameraShakeBase> BreakShakeClass;

private:
	bool IsPlayer(const AActor* Actor) const;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedShatterSound;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> LoadedAppearParticle;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> LoadedBreakParticle;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedBreakShake;

	/** The DoOnce: closed once the break is let through (the actor is gone then). */
	bool bShattered = false;
};
