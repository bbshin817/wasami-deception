#include "WasamiRingPiece.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Particles/ParticleSystemComponent.h"

namespace
{
	// BP_08_RingPiece (pak_reference_2): StaticMesh_GEN_VARIABLE, with BP_08_RingPiece_NoPickup's override of its body
	// (the NoCollision profile: WorldStatic, ignoring every channel, Visibility and Camera among them).
	const FRotator RingPieceMeshRotation(-39.99997329711914, 0., 30.00008201599121);
	const FVector RingPieceMeshScale(20.);
	const FName RingPieceInteractTag(TEXT("interact"));

	// PointLight_GEN_VARIABLE (UE 4.24's point light defaults otherwise; its intensity is unitless).
	constexpr float RingPieceLightAttenuationRadius = 500.f;
	constexpr float RingPieceLightMaxDrawDistance = 3000.f;
	constexpr float RingPieceLightMaxDistanceFadeRange = 2000.f;
	constexpr float RingPieceLightIntensity = 500.f;
	const FColor RingPieceLightColor(232, 78, 169, 255);
}

AWasamiRingPiece::AWasamiRingPiece()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetRelativeRotation(RingPieceMeshRotation);
	StaticMesh->SetRelativeScale3D(RingPieceMeshScale);
	StaticMesh->SetCastShadow(false);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMesh->SetCanEverAffectNavigation(false);
	StaticMesh->ComponentTags.Add(RingPieceInteractTag);

	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(DefaultSceneRoot);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(DefaultSceneRoot);
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = RingPieceLightIntensity;
	PointLight->LightColor = RingPieceLightColor;
	PointLight->AttenuationRadius = RingPieceLightAttenuationRadius;
	PointLight->MaxDrawDistance = RingPieceLightMaxDrawDistance;
	PointLight->MaxDistanceFadeRange = RingPieceLightMaxDistanceFadeRange;
	PointLight->CastShadows = false;
}
