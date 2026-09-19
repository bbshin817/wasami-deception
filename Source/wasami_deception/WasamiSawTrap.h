#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiSawTrap.generated.h"

class UAnimSequence;
class UAudioComponent;
class UBoxComponent;
class UPointLightComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USoundBase;

/**
 * Dark Deception's saw trap in Zone 2 of the hospital: BP_06_TrapBase with BP_06_sawTrap_medium on it (pak_reference_2's
 * Blueprints/06_Hospital/Traps). A skinned mesh plays its animation over and over (AnimationSingleNode): a blade that
 * spins as it slides along its track. The construction script puts collision attach on the mesh's sawSocket (the
 * blade), so Box, a thin box about the blade, goes with it, and the whine (Audio) with Box. The player coming into Box
 * (ReceiveActorBeginOverlap, once): the whine stops, the player stops (Disable Player Movement), the hit's flash
 * (Shake Scale 1), 0.1 s on a black screen (UMG_BlackScreen at Z 0) and 0.3 s on DeathEvent(the player). Nothing else
 * refers to them (the levels' Blueprints do not). The medium one is this class; short01, short02 and long01 are its
 * subclasses, each with its own mesh, animation, size and Box.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSawTrap : public AActor
{
	GENERATED_BODY()

public:
	AWasamiSawTrap();

	/** The socket collision attach goes on (on the blade: blade or Saw by the mesh; socket names match in any case). */
	static const FName SawSocket;

	/** @658: the hit's flash's Shake Scale. */
	static constexpr float HitShakeScale = 1.f;
	/** @871: Delay 0.1 before the black screen. */
	static constexpr float BlackScreenDelay = 0.1f;
	/** @309: Delay 0.3 after it before DeathEvent. */
	static constexpr float DeathDelay = 0.3f;
	/** The construction script's RandomFloatInRange for the whine's pitch. */
	static constexpr float MinPitch = 1.f;
	static constexpr float MaxPitch = 1.2f;

	/** ReceiveActorBeginOverlap's DoOnce and what follows (the player caught). */
	UFUNCTION(BlueprintCallable, Category = "Saw Trap")
	void CatchPlayer();

	/** Whether the DoOnce has let the catch through. */
	bool HasCaughtPlayer() const { return bCaught; }

	USceneComponent* GetRoot() const { return Root; }
	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	USceneComponent* GetCollisionAttach() const { return CollisionAttach; }
	UBoxComponent* GetBox() const { return Box; }
	UAudioComponent* GetAudio() const { return Audio; }

protected:
	virtual void PreRegisterAllComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** ReceiveActorBeginOverlap (@1120): the player (only Box overlaps anything) goes on to CatchPlayer. */
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	/** root: the SCS's root (BP_06_TrapBase's DefaultSceneRoot is not made). */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<USceneComponent> Root;

	/** Mesh: the trap, scaled by the class, animated by AnimAsset; no collision (the skinned mesh's default). */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** collision attach: made on root, put on Mesh's sawSocket by the construction script. */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<USceneComponent> CollisionAttach;

	/** Box: about the blade, overlapping pawns only. */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<UBoxComponent> Box;

	/** Audio: SFX_Matron_SawLoop at 0.5, with its own attenuation (natural, occluded), on Box. */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<UAudioComponent> Audio;

	UPROPERTY(EditAnywhere, Category = "Saw Trap|Assets")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

	/** Mesh's AnimToPlay. */
	UPROPERTY(EditAnywhere, Category = "Saw Trap|Assets")
	TSoftObjectPtr<UAnimSequence> AnimAsset;

	UPROPERTY(EditAnywhere, Category = "Saw Trap|Assets")
	TSoftObjectPtr<USoundBase> LoopSound;

private:
	/** Mesh's SkeletalMesh and AnimToPlay (not loaded from the constructor, WasamiAssets.h). */
	void ApplyMeshAssets();

	/** The DoOnce: closed once the player is caught. */
	bool bCaught = false;

	FTimerHandle BlackScreenTimer;
	FTimerHandle DeathTimer;
};

/** BP_06_sawTrap_short01: the short trap (its mesh at 1.8) with a cold light over it. */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSawTrapShort01 : public AWasamiSawTrap
{
	GENERATED_BODY()

public:
	AWasamiSawTrapShort01();

	UPointLightComponent* GetPointLight() const { return PointLight; }

protected:
	/** PointLight: on root, 114 cm up, no shadows (five of Zone 2's lights are turned down to 1 by the level). */
	UPROPERTY(VisibleAnywhere, Category = "Saw Trap")
	TObjectPtr<UPointLightComponent> PointLight;
};

/** BP_06_sawTrap_short02: the short trap without the light (a plainer mesh). */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSawTrapShort02 : public AWasamiSawTrap
{
	GENERATED_BODY()

public:
	AWasamiSawTrapShort02();
};

/** BP_06_sawTrap_long01: the long trap (its mesh at 1, its Box about a large blade). */
UCLASS()
class WASAMI_DECEPTION_API AWasamiSawTrapLong01 : public AWasamiSawTrap
{
	GENERATED_BODY()

public:
	AWasamiSawTrapLong01();
};
