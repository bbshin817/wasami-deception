#include "WasamiZoneBarrier.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiAssets.h"

namespace
{
	// BP_ZoneBarrier (pak_reference_2): the SCS templates. The root's 3.2 is the class's; every placed barrier scales
	// its root to its own size.
	const FVector ZoneBarrierRootScale(3.2, 3.2, 3.2);
	const FRotator ZoneBarrierFrontRotation(0.000150264153489843, 90.00021362304688, -89.99990844726562);
	const FVector ZoneBarrierBackLocation(-3.035816192626953, 4.5360986405285075e-05, -4.938111942465184e-06);
	const FRotator ZoneBarrierBackRotation(-90., -359.9805603027344, 359.9808654785156);
	const FVector ZoneBarrierBackScale(1.0710335969924927, 1.0710335969924927, 1.);
	const FName ZoneBarrierInteractTag(TEXT("interact"));

	// PointLight_GEN_VARIABLE (UE 4.24's point light defaults otherwise; its intensity is unitless).
	constexpr float ZoneBarrierLightSourceRadius = 214.3380126953125f;
	constexpr float ZoneBarrierLightSoftSourceRadius = 1000.f;
	constexpr float ZoneBarrierLightAttenuationRadius = 400.f;
	constexpr float ZoneBarrierLightIntensity = 2500.f;
	const FColor ZoneBarrierLightColor(255, 0, 188, 255);

	// Audio_GEN_VARIABLE: Barrier_Loop and its attenuation overrides (the rest at FSoundAttenuationSettings' defaults).
	constexpr float ZoneBarrierLoopVolume = 0.5f;
	constexpr float ZoneBarrierLoopPitch = 0.8f;
	constexpr float ZoneBarrierOcclusionLowPass = 1500.f;
	constexpr float ZoneBarrierFalloffDistance = 2000.f;

	// UserConstructionScript: SetScalarParameterValueOnMaterials on each plane.
	const FName EmissivePulseMaxName(TEXT("Emissive Pulse Max"));
	const FName EmissivePulseMinName(TEXT("Emissive Pulse Min"));

	// Destroy: SpawnEmitterAtLocation(P_ky_impact3, StaticMesh's location, no rotation, 2, auto destroy, no pooling,
	// active) and PlaySoundAtLocation(Barrier_Shatter, the actor's location, no rotation, 1, 1, 0, 01_Lobby_Attenuation).
	constexpr double ZoneBarrierBreakScale = 2.;

	/** A plane of the barrier: the engine's Plane standing up, custom collision (WorldStatic, blocking), no navigation. */
	UStaticMeshComponent* MakeZoneBarrierPlane(AActor* Owner, USceneComponent* Root, UStaticMesh* Mesh, const TCHAR* Name)
	{
		UStaticMeshComponent* Plane = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Plane->SetupAttachment(Root);
		Plane->SetStaticMesh(Mesh);
		// Setting the object type makes the profile Custom, keeping the component's blocking responses (the original's
		// Custom body lists no responses, so they are at the default: block).
		Plane->SetCollisionObjectType(ECC_WorldStatic);
		Plane->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Plane->SetCanEverAffectNavigation(false);
		Plane->ComponentTags.Add(ZoneBarrierInteractTag);
		return Plane;
	}

	void SetZoneBarrierBrightness(UStaticMeshComponent* Plane, float Min, float Max)
	{
		if (IsValid(Plane))
		{
			Plane->SetScalarParameterValueOnMaterials(EmissivePulseMaxName, Max);
			Plane->SetScalarParameterValueOnMaterials(EmissivePulseMinName, Min);
		}
	}
}

AWasamiZoneBarrier::AWasamiZoneBarrier()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(ZoneBarrierInteractTag);

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetRelativeScale3D(ZoneBarrierRootScale);
	DefaultSceneRoot->ComponentTags.Add(ZoneBarrierInteractTag);
	RootComponent = DefaultSceneRoot;

	// The engine's own content, which the pipeline never rebuilds, so it may load here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	StaticMesh1 = MakeZoneBarrierPlane(this, DefaultSceneRoot, PlaneMesh.Object, TEXT("StaticMesh1"));
	StaticMesh1->SetRelativeRotation(ZoneBarrierFrontRotation);
	StaticMesh = MakeZoneBarrierPlane(this, DefaultSceneRoot, PlaneMesh.Object, TEXT("StaticMesh"));
	StaticMesh->SetRelativeLocation(ZoneBarrierBackLocation);
	StaticMesh->SetRelativeRotation(ZoneBarrierBackRotation);
	StaticMesh->SetRelativeScale3D(ZoneBarrierBackScale);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(DefaultSceneRoot);
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = ZoneBarrierLightIntensity;
	PointLight->LightColor = ZoneBarrierLightColor;
	PointLight->SourceRadius = ZoneBarrierLightSourceRadius;
	PointLight->SoftSourceRadius = ZoneBarrierLightSoftSourceRadius;
	PointLight->AttenuationRadius = ZoneBarrierLightAttenuationRadius;
	PointLight->VolumetricScatteringIntensity = 0.f;

	// The loop starts when play begins (its sound is loaded then), as the original's does when the level starts.
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(DefaultSceneRoot);
	Audio->bAutoActivate = false;
	Audio->SetVolumeMultiplier(ZoneBarrierLoopVolume);
	Audio->SetPitchMultiplier(ZoneBarrierLoopPitch);
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bEnableOcclusion = true;
	Audio->AttenuationOverrides.bUseComplexCollisionForOcclusion = true;
	Audio->AttenuationOverrides.OcclusionLowPassFilterFrequency = ZoneBarrierOcclusionLowPass;
	Audio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	Audio->AttenuationOverrides.FalloffDistance = ZoneBarrierFalloffDistance;

	LoopSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/02_School/Barrier_Loop")));
	ShatterSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/02_School/Barrier_Shatter")));
	ShatterAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
	BreakParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact3")));
}

void AWasamiZoneBarrier::BeginPlay()
{
	Super::BeginPlay();
	LoadedShatterSound = ShatterSound.LoadSynchronous();
	LoadedShatterAttenuation = ShatterAttenuation.LoadSynchronous();
	LoadedBreakParticle = BreakParticle.LoadSynchronous();

	// UserConstructionScript: each plane's pulse between its layer's brightnesses (dynamic instances of the materials
	// the level gave them). Done here, so that no dynamic instance is saved with the level.
	SetZoneBarrierBrightness(StaticMesh1, Layer1MinBrightness, Layer1MaxBrightness);
	SetZoneBarrierBrightness(StaticMesh, Layer2MinBrightness, Layer2MaxBrightness);

	// ReceiveBeginPlay: without bSound the loop's component is destroyed. (NoInteract and Off By Default, which clear
	// the planes' tags and hide the barrier, are false on the hospital's barriers.)
	if (!bSound)
	{
		Audio->DestroyComponent();
		return;
	}
	Audio->SetSound(LoopSound.LoadSynchronous());
	Audio->Play();
}

void AWasamiZoneBarrier::DestroyBarrier()
{
	if (IsValid(StaticMesh))
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, LoadedBreakParticle, StaticMesh->GetComponentLocation(),
			FRotator::ZeroRotator, FVector(ZoneBarrierBreakScale), true, EPSCPoolMethod::None, true);
	}
	UGameplayStatics::PlaySoundAtLocation(this, LoadedShatterSound, GetActorLocation(), FRotator::ZeroRotator, 1.f, 1.f,
		0.f, LoadedShatterAttenuation);
	K2_DestroyActor();
}
