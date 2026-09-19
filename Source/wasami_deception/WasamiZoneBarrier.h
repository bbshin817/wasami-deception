#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiInteractable.h"
#include "WasamiZoneBarrier.generated.h"

class UAudioComponent;
class UParticleSystem;
class UPointLightComponent;
class USoundAttenuation;
class USoundBase;
class UStaticMeshComponent;
class UWasamiTextPromptWidget;

/**
 * Dark Deception's BP_ZoneBarrier (pak_reference_2's Blueprints/Main), the glowing wall that closes a zone's way on until
 * its shards are collected: two planes 3 cm apart with pulsing materials (the brighter Layer 1 in front), a violet
 * light and a humming loop. The zone's flow breaks it with Destroy (DestroyBarrier here): a burst of P_ky_impact3 and
 * the shatter sound, and the actor is gone. The hospital places one in each zone (Zone 1's in the way to the parking
 * lot, Zone 2's in the way to the garage).
 *
 * Looked at and clicked (InteractWithObject; its root and planes are tagged interact, so the hand shows over it), it
 * turns the player away: the denied sound and a text prompt, at most once every 5 s.
 *
 * The planes' materials are not set by the class (assets under /Game/DD are never loaded from a constructor,
 * WasamiAssets.h): the level build places the barriers with MM_ZoneBarrier_Inst1 and _Inst2 on them.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiZoneBarrier : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiZoneBarrier();

	/** Layer 1 | Min / Max Brightness: the front plane's (StaticMesh1's) Emissive Pulse Min and Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	float Layer1MinBrightness = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	float Layer1MaxBrightness = 9.5f;

	/** Layer 2 | Min / Max Brightness: the back plane's (StaticMesh's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	float Layer2MinBrightness = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	float Layer2MaxBrightness = 1.2f;

	/** bSound: the humming loop plays (otherwise its component is taken away when play begins). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	bool bSound = true;

	/** Interaction Text: what the prompt says as the barrier turns the player away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Barrier")
	FText InteractionText;

	/**
	 * Destroy (the original's custom event; AActor has its own Destroy): P_ky_impact3 at the back plane (twice its
	 * size), the shatter at the actor, and the actor destroyed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Zone Barrier")
	void DestroyBarrier();

	/** InteractWithObject's Delay: how long after turning the player away the barrier can do it again. */
	static constexpr float DeniedInterval = 5.f;

	/** The prompt the barrier last put up (null before the first, or once it has gone). */
	UWasamiTextPromptWidget* GetLastPrompt() const { return LastPrompt.Get(); }

	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	UStaticMeshComponent* GetStaticMesh1() const { return StaticMesh1; }
	UPointLightComponent* GetPointLight() const { return PointLight; }
	UAudioComponent* GetAudio() const { return Audio; }

protected:
	virtual void BeginPlay() override;

	/**
	 * InteractWithObject (a DoOnce): DD_RingBarrierDenied_louder at the actor (0.75, pitch 1.1, through
	 * DialogueAttenuation) and a text prompt of Interaction Text on the player's screen; the DoOnce opens again
	 * DeniedInterval later. (The original first checks NoInteract, which the hospital's barriers leave false.)
	 */
	virtual void InteractWithObject_Implementation(AActor* Interactee) override;

	/** DefaultSceneRoot: the level's actors scale it to the barrier's size. */
	UPROPERTY(VisibleAnywhere, Category = "Zone Barrier")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** StaticMesh1: the front plane (the engine's Plane, standing up), MM_ZoneBarrier_Inst1. */
	UPROPERTY(VisibleAnywhere, Category = "Zone Barrier")
	TObjectPtr<UStaticMeshComponent> StaticMesh1;

	/** StaticMesh: the back plane, 3 cm behind and a little larger, MM_ZoneBarrier_Inst2. */
	UPROPERTY(VisibleAnywhere, Category = "Zone Barrier")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(VisibleAnywhere, Category = "Zone Barrier")
	TObjectPtr<UPointLightComponent> PointLight;

	/** Audio: Barrier_Loop at half volume and 0.8 pitch, with its own attenuation (occluded, 2000 cm natural falloff). */
	UPROPERTY(VisibleAnywhere, Category = "Zone Barrier")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<USoundBase> LoopSound;

	/** Barrier_Shatter at the actor through 01_Lobby_Attenuation, as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<USoundBase> ShatterSound;

	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<USoundAttenuation> ShatterAttenuation;

	/** P_ky_impact3 (AdvancedMagicFX13), as it breaks. */
	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<UParticleSystem> BreakParticle;

	/** DD_RingBarrierDenied_louder through DialogueAttenuation, as the barrier turns the player away. */
	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<USoundBase> DeniedSound;

	UPROPERTY(EditAnywhere, Category = "Zone Barrier|Assets")
	TSoftObjectPtr<USoundAttenuation> DeniedAttenuation;

private:
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedShatterSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedShatterAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystem> LoadedBreakParticle;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LoadedDeniedSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> LoadedDeniedAttenuation;

	/** The DoOnce: closed from turning the player away until DeniedTimer opens it again. */
	bool bDeniedClosed = false;
	FTimerHandle DeniedTimer;
	TWeakObjectPtr<UWasamiTextPromptWidget> LastPrompt;
};
