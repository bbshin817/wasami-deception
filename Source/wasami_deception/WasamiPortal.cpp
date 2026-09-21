#include "WasamiPortal.h"

#include "Camera/CameraShakeBase.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/RichCurve.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiAssets.h"

namespace
{
	// BP_00_Teleport (pak_reference_2): the SCS templates (rotations as the export's pitch, yaw, roll).
	const FVector CollisionLocation(0., 0., 100.);
	const FVector CollisionScale(0.5, 2., 3.5);
	const FVector CubeScale(0.25);
	const FVector VortexLocation(-1.309136152267456, -1.5258903658832423e-05, 100.);
	const FRotator VortexRotation(-90., 16.69925880432129, -106.69956970214844);
	const FVector VortexScale(70., 69.99999237060547, 69.99999237060547);
	const FVector OuterLocation(-1.7029898602416438e-09, 0.018704663962125778, -3.2016214390750974e-07);
	const FRotator OuterRotation(6.830188794992864e-06, 3.4150934880017303e-06, -1.7075466303140274e-06);
	const FVector OuterScale(0.7142857313156128, 0.7142857909202576, 0.7142857909202576);
	const FVector InnerLocation(0., -1.5305704481605176e-09, -2.304645406805972e-12);
	const FRotator InnerRotation(-6.830188794992864e-06, -1.7075442428904353e-06, -9.220753418048844e-05);
	const FVector InnerScale(1., 0.9999998807907104, 0.9999998807907104);
	const FVector LogoLocation(3.859375397041731e-08, 0.005920510273426771, -2.8650836156884907e-06);
	const FVector LogoLockLocation(0.0775734931230545, 0.08848155289888382, 0.00538179324939847);
	const FRotator LogoRotation(90., 90., -179.99954223632812);
	const FVector LogoPlaneScale(0.019999999552965164, 0.020000003278255463, 0.020000003278255463);
	const FRotator LogoAudioRotation(90., -130.7762451171875, 139.2232666015625);
	const FVector AppearLocation(-6.72321081161499, -5.599670475930907e-05, 19.929922103881836);
	const FVector AppearScale(0.10000000149011612, 0.10000002384185791, 0.10000002384185791);

	// Portal Loop_GEN_VARIABLE: Portal_Sound_v3 at half volume, its attenuation overridden (natural sound, 2000 cm).
	constexpr float LoopVolume = 0.5f;
	constexpr float LoopFalloffDistance = 2000.f;
	// Audio_GEN_VARIABLE: portal_unlocked, a sphere of 1000 cm falling off over 2000, not playing by itself.
	constexpr float UnlockedSphereRadius = 1000.f;
	constexpr float UnlockedFalloffDistance = 2000.f;

	// Lock/Unlock's open light (UserConstructionScript's is pure red) and logo scale (Logo Scale x 0.02).
	const FLinearColor LockUnlockOpenLight(1.f, 0.0152f, 0.f, 1.f);
	const FLinearColor ConstructionOpenLight(1.f, 0.f, 0.f, 1.f);
	constexpr float LogoScaleFactor = 0.02f;
	// PlayWorldCameraShake(Self, BP_Portal_CameraShake, the Logo's location, 0, 3000, 1, true).
	constexpr float OpenShakeOuterRadius = 3000.f;

	// The hotel's exit's BP_00_StrobingLight_65 (a PointLight): its LightComponent0 and where it stands from the portal
	// (the light's location less the portal's, over the portal's scale of 2.5). The class's own values otherwise:
	// no shadow; its Intensity is the timeline's times Light Intensity.
	const FVector StrobingLightLocation((-1784.26123046875 + 1704.5880126953125) / 2.5,
		(-2539.67431640625 + 2541.65576171875) / 2.5, (153.36434936523438 + 84.79782104492188) / 2.5);
	constexpr float StrobingLightSourceRadius = 4.97201681137085f;
	constexpr float StrobingLightSoftSourceRadius = 52.53159713745117f;
	constexpr float StrobingLightAttenuationRadius = 500.f;

	/**
	 * BP_00_StrobingLight's CurveFloat_0 (the Strobe timeline, 2 s, looping): 0.5 at 0 (cubic, the author's tangents
	 * 0.5757), 1 at 1 (cubic, flat), 0.5 at 2 (linear).
	 */
	FRichCurve MakeStrobeCurve()
	{
		struct FKey { float Time, Value, Arrive, Leave; ERichCurveInterpMode Interp; };
		static const FKey Keys[] = {
			{0.f, 0.5f, 0.5757500529289246f, 0.5757492184638977f, RCIM_Cubic},
			{1.f, 1.f, 0.f, 0.f, RCIM_Cubic},
			{2.f, 0.5f, 0.f, 0.f, RCIM_Linear},
		};
		FRichCurve Curve;
		for (const FKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Each.Time, Each.Value));
			Key.InterpMode = Each.Interp;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = Each.Arrive;
			Key.LeaveTangent = Each.Leave;
		}
		return Curve;
	}

	/** A plane of the disc: no shadow, lit only by lighting channel 2 (the original's), out of the navigation. */
	UStaticMeshComponent* MakePortalPlane(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Plane = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Plane->SetupAttachment(Parent);
		Plane->SetCastShadow(false);
		Plane->LightingChannels.bChannel0 = false;
		Plane->LightingChannels.bChannel2 = true;
		Plane->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Plane->SetCanEverAffectNavigation(false);
		return Plane;
	}
}

AWasamiPortal::AWasamiPortal()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// UBoxComponent's defaults otherwise (OverlapAllDynamic). Its NavArea_Obstacle is left out: a box that blocks
	// nothing is not in the way of the navigation.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetupAttachment(DefaultSceneRoot);
	Collision->SetRelativeLocation(CollisionLocation);
	Collision->SetRelativeScale3D(CollisionScale);
	Collision->SetCanEverAffectNavigation(false);

	// The engine's own content, which the pipeline never rebuilds, so it may load here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CubeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));

	Cube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));
	Cube->SetupAttachment(DefaultSceneRoot);
	Cube->SetStaticMesh(CubeMesh.Object);
	Cube->SetMaterial(0, CubeMaterial.Object);
	Cube->SetRelativeScale3D(CubeScale);
	Cube->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Cube->SetHiddenInGame(true);
	Cube->SetCanEverAffectNavigation(false);

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(Cube);

	Vortex = MakePortalPlane(this, DefaultSceneRoot, TEXT("Vortex"));
	Vortex->SetRelativeLocationAndRotation(VortexLocation, VortexRotation);
	Vortex->SetRelativeScale3D(VortexScale);
	Outer = MakePortalPlane(this, Vortex, TEXT("Outer"));
	Outer->SetRelativeLocationAndRotation(OuterLocation, OuterRotation);
	Outer->SetRelativeScale3D(OuterScale);
	Inner = MakePortalPlane(this, Outer, TEXT("Inner"));
	Inner->SetRelativeLocationAndRotation(InnerLocation, InnerRotation);
	Inner->SetRelativeScale3D(InnerScale);
	Logo = MakePortalPlane(this, Inner, TEXT("Logo"));
	Logo->SetStaticMesh(PlaneMesh.Object);
	Logo->SetRelativeLocationAndRotation(LogoLocation, LogoRotation);
	Logo->SetRelativeScale3D(LogoPlaneScale);
	LogoLock = MakePortalPlane(this, Inner, TEXT("LogoLock"));
	LogoLock->SetStaticMesh(PlaneMesh.Object);
	LogoLock->SetRelativeLocationAndRotation(LogoLockLocation, LogoRotation);
	LogoLock->SetRelativeScale3D(LogoPlaneScale);
	LogoLock->SetVisibility(false);

	// The loop starts as play begins (its sound is loaded then), as the original's plays from the level's start.
	PortalLoop = CreateDefaultSubobject<UAudioComponent>(TEXT("PortalLoop"));
	PortalLoop->SetupAttachment(Logo);
	PortalLoop->SetRelativeRotation(LogoAudioRotation);
	PortalLoop->bAutoActivate = false;
	PortalLoop->SetVolumeMultiplier(LoopVolume);
	PortalLoop->bOverrideAttenuation = true;
	PortalLoop->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	PortalLoop->AttenuationOverrides.FalloffDistance = LoopFalloffDistance;

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Logo);
	Audio->SetRelativeRotation(LogoAudioRotation);
	Audio->bAutoActivate = false;
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(UnlockedSphereRadius, 0., 0.);
	Audio->AttenuationOverrides.FalloffDistance = UnlockedFalloffDistance;

	auto MakeAppear = [this](const TCHAR* Name)
	{
		UParticleSystemComponent* Appear = CreateDefaultSubobject<UParticleSystemComponent>(Name);
		Appear->SetupAttachment(Logo);
		Appear->SetRelativeLocationAndRotation(AppearLocation, LogoAudioRotation);
		Appear->SetRelativeScale3D(AppearScale);
		Appear->bAutoActivate = false;
		return Appear;
	};
	PortalAppear = MakeAppear(TEXT("PPP_PortalAppear"));
	PortalAppearLock = MakeAppear(TEXT("PPP_PortalAppear_Lock"));

	StrobingLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StrobingLight"));
	StrobingLight->SetupAttachment(DefaultSceneRoot);
	StrobingLight->SetRelativeLocation(StrobingLightLocation);
	StrobingLight->SetMobility(EComponentMobility::Movable);
	StrobingLight->IntensityUnits = ELightUnits::Unitless;
	StrobingLight->Intensity = StrobingLightIntensity;
	StrobingLight->SourceRadius = StrobingLightSourceRadius;
	StrobingLight->SoftSourceRadius = StrobingLightSoftSourceRadius;
	StrobingLight->AttenuationRadius = StrobingLightAttenuationRadius;
	StrobingLight->CastShadows = false;

	DiscMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/00_Ballroom/circle_portal_decal")));
	VortexMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Inst")));
	VortexMaskedMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Masked")));
	VortexLockedMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Locked_Inst")));
	VortexLockedMaskedMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Locked_Inst_Masked")));
	OuterMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Outer_Inst")));
	OuterLockedMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Outer_Locked_Inst")));
	InnerMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Inner_Inst")));
	InnerLockedMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex_Inner_Locked_Inst")));
	LogoMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Wasami/Portal/MI_Portal_Wasami")));
	AppearParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_PortalAppear")));
	AppearLockParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_PortalAppear_Lock")));
	LockMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/00_Ballroom/M_00_Portal_Lock")));
	LoopSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/00_Ballroom/Portal_Sound_v3")));
	UnlockedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/00_Ballroom/portal_unlocked")));
	OpenShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/BP_Portal_CameraShake")));
}

float AWasamiPortal::EvaluateStrobe(float Seconds)
{
	static const FRichCurve Curve = MakeStrobeCurve();
	return Curve.Eval(FMath::Fmod(FMath::Max(Seconds, 0.f), StrobeLength));
}

void AWasamiPortal::LoadLook()
{
	UStaticMesh* Disc = DiscMesh.LoadSynchronous();
	for (UStaticMeshComponent* Plane : {Vortex.Get(), Outer.Get(), Inner.Get()})
	{
		Plane->SetStaticMesh(Disc);
	}
	LoadedVortexMaterial = VortexMaterial.LoadSynchronous();
	LoadedVortexMaskedMaterial = VortexMaskedMaterial.LoadSynchronous();
	LoadedVortexLockedMaterial = VortexLockedMaterial.LoadSynchronous();
	LoadedVortexLockedMaskedMaterial = VortexLockedMaskedMaterial.LoadSynchronous();
	LoadedOuterMaterial = OuterMaterial.LoadSynchronous();
	LoadedOuterLockedMaterial = OuterLockedMaterial.LoadSynchronous();
	LoadedInnerMaterial = InnerMaterial.LoadSynchronous();
	LoadedInnerLockedMaterial = InnerLockedMaterial.LoadSynchronous();
	// UserConstructionScript: the logo's material by Portal Enemy (this game's symbol, whichever).
	Logo->SetMaterial(0, LogoMaterial.LoadSynchronous());
	LogoLock->SetMaterial(0, LockMaterial.LoadSynchronous());
}

void AWasamiPortal::ApplyLock(const FLinearColor& OpenLightColor)
{
	if (bLocked)
	{
		Logo->SetRelativeScale3D(FVector::ZeroVector);
		LogoLock->SetVisibility(true, false);
		Vortex->SetMaterial(0, bMaskedPortalMaterial ? LoadedVortexLockedMaskedMaterial : LoadedVortexLockedMaterial);
		Outer->SetMaterial(0, LoadedOuterLockedMaterial);
		Inner->SetMaterial(0, LoadedInnerLockedMaterial);
		StrobingLight->SetLightColor(LockedLightColor, true);
		PortalLoop->SetVolumeMultiplier(0.f);
	}
	else
	{
		Logo->SetRelativeScale3D(FVector(LogoScale * LogoScaleFactor));
		LogoLock->SetVisibility(false, false);
		Vortex->SetMaterial(0, bMaskedPortalMaterial ? LoadedVortexMaskedMaterial : LoadedVortexMaterial);
		Outer->SetMaterial(0, LoadedOuterMaterial);
		Inner->SetMaterial(0, LoadedInnerMaterial);
		StrobingLight->SetLightColor(OpenLightColor, true);
		PortalLoop->SetVolumeMultiplier(LoopVolume);
	}
}

void AWasamiPortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LoadLook();
	ApplyLock(ConstructionOpenLight);
}

void AWasamiPortal::BeginPlay()
{
	Super::BeginPlay();
	LoadLook();
	Audio->SetSound(UnlockedSound.LoadSynchronous());
	PortalLoop->SetSound(LoopSound.LoadSynchronous());
	// The original's SCS holds these templates; here they are loaded as play begins (WasamiAssets.h).
	PortalAppear->SetTemplate(AppearParticle.LoadSynchronous());
	PortalAppearLock->SetTemplate(AppearLockParticle.LoadSynchronous());
	PortalLoop->Play();
	// BP_00_StrobingLight's ReceiveBeginPlay: Strobe plays (and loops).
	StrobeSeconds = 0.f;
	StrobingLight->SetIntensity(EvaluateStrobe(0.f) * StrobingLightIntensity);
}

void AWasamiPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Strobe's update: the light's intensity is the timeline's value times Light Intensity.
	StrobeSeconds = FMath::Fmod(StrobeSeconds + DeltaSeconds, StrobeLength);
	StrobingLight->SetIntensity(EvaluateStrobe(StrobeSeconds) * StrobingLightIntensity);
}

void AWasamiPortal::LockUnlock(bool bLock, bool bPlaySoundAndShake)
{
	bLocked = bLock;
	ApplyLock(LockUnlockOpenLight);
	if (!bPlaySoundAndShake)
	{
		return;
	}
	if (bLocked)
	{
		PortalAppearLock->Activate(true);
		return;
	}
	Audio->Play(0.f);
	if (UClass* Shake = OpenShakeClass.LoadSynchronous())
	{
		UGameplayStatics::PlayWorldCameraShake(this, Shake, Logo->GetComponentLocation(), 0.f, OpenShakeOuterRadius, 1.f, true);
	}
	PortalAppear->Activate(true);
}
