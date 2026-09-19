#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiInteractable.h"
#include "WasamiRingPiece.generated.h"

class UParticleSystemComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Dark Deception's BP_08_RingPiece_NoPickup (pak_reference_2's Blueprints/08_BearHouse, a BP_08_RingPiece that cannot
 * be picked up): the ring piece shown over an altar, turned and scaled up, in a glow of particles and a violet light. It
 * is only to be seen: its mesh collides with nothing and ignores the look's trace, and InteractWithObject does nothing
 * (the altar, AWasamiRingStatue, is what is clicked). The zone's flow destroys it once the piece is taken. Zone 2 of
 * the hospital has one (BP_08_RingPiece_NoPickup_5) over ring_statue_2.
 *
 * The mesh, its materials and the particle system are not set by the class (assets under /Game/DD are never loaded
 * from a constructor, WasamiAssets.h): the level build places the piece with ring_piece06, M_ring_metal and
 * M_ring_metal2, and P_08_RingPiece.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiRingPiece : public AActor, public IWasamiInteractable
{
	GENERATED_BODY()

public:
	AWasamiRingPiece();

	UStaticMeshComponent* GetStaticMesh() const { return StaticMesh; }
	UParticleSystemComponent* GetParticleSystem() const { return ParticleSystem; }
	UPointLightComponent* GetPointLight() const { return PointLight; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ring Piece")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	/** StaticMesh: ring_piece06, turned (-40, 0, 30) and 20 times its size, casting no shadow. */
	UPROPERTY(VisibleAnywhere, Category = "Ring Piece")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	/** ParticleSystem: P_08_RingPiece's glow. */
	UPROPERTY(VisibleAnywhere, Category = "Ring Piece")
	TObjectPtr<UParticleSystemComponent> ParticleSystem;

	/** PointLight: violet, 500 cm. */
	UPROPERTY(VisibleAnywhere, Category = "Ring Piece")
	TObjectPtr<UPointLightComponent> PointLight;
};
