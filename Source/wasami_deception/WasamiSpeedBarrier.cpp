#include "WasamiSpeedBarrier.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "WasamiAssets.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiPowerComponent.h"

namespace
{
	// BP_SpeedBarrier (pak_reference_2): the SCS templates, every one on DefaultSceneRoot. Every placed barrier scales its
	// root to its own size (and three of Zone 1's four move or scale their planes).
	const FVector SpeedBarrierRootScale(3.200000047683716, 3.200000286102295, 3.200000286102295);
	const FRotator SpeedBarrierFrontRotation(0.000150264153489843, 90.00021362304688, -89.99990844726562);
	const FVector SpeedBarrierBackLocation(-3.035816192626953, 4.5360986405285075e-05, -4.938111942465184e-06);
	const FRotator SpeedBarrierBackRotation(-90., -359.9805603027344, 359.9808654785156);
	const FVector SpeedBarrierBackScale(1.0710335969924927, 1.0710335969924927, 1.);
	const FVector SpeedBarrierBoxLocation(-0.46306324005126953, -3.814696901827119e-05, 0.);
	const FVector SpeedBarrierBoxScale(1., 1.6754796504974365, 1.7969528436660767);
	const FVector SpeedBarrierBox1Location(-2.2414774894714355, 4.768371218233369e-05, 0.);
	const FVector SpeedBarrierBox1Scale(0.17014530301094055, 1.7641761302947998, 1.7641761302947998);

	// PointLight_GEN_VARIABLE (UE 4.24's point light defaults otherwise; its intensity is unitless). Its colour is the
	// export's [B, G, R, A] turned round (troubleshooting.md): red.
	constexpr float SpeedBarrierLightSourceRadius = 124.18572998046875f;
	constexpr float SpeedBarrierLightSoftSourceRadius = 42.588626861572266f;
	constexpr float SpeedBarrierLightAttenuationRadius = 400.f;
	constexpr float SpeedBarrierLightIntensity = 2500.f;
	const FColor SpeedBarrierLightColor(255, 0, 0, 255);

	// Audio_GEN_VARIABLE: Barrier_Loop and its attenuation overrides (the rest at FSoundAttenuationSettings' defaults).
	constexpr float SpeedBarrierLoopVolume = 0.3f;
	constexpr float SpeedBarrierFalloffDistance = 2000.f;

	// @25: SpawnEmitterAtLocation(PPP_PortalAppear, StaticMesh's location, no rotation, 0.3, auto destroy, no pooling,
	// active), the same for P_ky_impact2 at 4, PlayCameraShake(BP_01_DoorExplode_CameraShake, 3, CameraLocal, no
	// rotation) on player 0's camera, and PlaySound2D(Barrier_Shatter, 0.8, 1, 0).
	constexpr double SpeedBarrierAppearScale = 0.30000001192092896;
	constexpr double SpeedBarrierBreakScale = 4.;
	constexpr float SpeedBarrierShakeScale = 3.f;
	constexpr float SpeedBarrierShatterVolume = 0.8f;

	/** A plane: the engine's Plane, no collision (on the world static channel, as the original's Custom body), no navigation. */
	UStaticMeshComponent* MakeSpeedBarrierPlane(AActor* Owner, USceneComponent* Root, UStaticMesh* Mesh, const TCHAR* Name)
	{
		UStaticMeshComponent* Plane = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Plane->SetupAttachment(Root);
		Plane->SetStaticMesh(Mesh);
		Plane->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		Plane->SetCollisionObjectType(ECC_WorldStatic);
		Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Plane->SetCanEverAffectNavigation(false);
		return Plane;
	}
}

AWasamiSpeedBarrier::AWasamiSpeedBarrier()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetRelativeScale3D(SpeedBarrierRootScale);
	RootComponent = DefaultSceneRoot;

	// The engine's own content, which the pipeline never rebuilds, so it may load here.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	StaticMesh1 = MakeSpeedBarrierPlane(this, DefaultSceneRoot, PlaneMesh.Object, TEXT("StaticMesh1"));
	StaticMesh1->SetRelativeRotation(SpeedBarrierFrontRotation);
	StaticMesh = MakeSpeedBarrierPlane(this, DefaultSceneRoot, PlaneMesh.Object, TEXT("StaticMesh"));
	StaticMesh->SetRelativeLocation(SpeedBarrierBackLocation);
	StaticMesh->SetRelativeRotation(SpeedBarrierBackRotation);
	StaticMesh->SetRelativeScale3D(SpeedBarrierBackScale);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(DefaultSceneRoot);
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = SpeedBarrierLightIntensity;
	PointLight->LightColor = SpeedBarrierLightColor;
	PointLight->SourceRadius = SpeedBarrierLightSourceRadius;
	PointLight->SoftSourceRadius = SpeedBarrierLightSoftSourceRadius;
	PointLight->AttenuationRadius = SpeedBarrierLightAttenuationRadius;

	// Box: the shape's defaults otherwise (32 cm, OverlapAllDynamic). Its AreaClass (NavArea_Default) does nothing, as
	// the original's is out of the navigation.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
	Box->SetRelativeLocation(SpeedBarrierBoxLocation);
	Box->SetRelativeScale3D(SpeedBarrierBoxScale);
	Box->SetCanEverAffectNavigation(false);
	Box->OnComponentBeginOverlap.AddDynamic(this, &AWasamiSpeedBarrier::OnBoxBeginOverlap);

	// The loop starts when play begins (its sound is loaded then), as the original's does when the level starts.
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(DefaultSceneRoot);
	Audio->bAutoActivate = false;
	Audio->SetVolumeMultiplier(SpeedBarrierLoopVolume);
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	Audio->AttenuationOverrides.FalloffDistance = SpeedBarrierFalloffDistance;

	// Box1: Custom on world static, ignoring the world, the traces, physics bodies, vehicles and destructibles; the other
	// channels at their defaults (the original's list holds only what differs from them): it blocks pawns. Still the
	// shape's QueryOnly (the original's Custom body does not change it), which is what stops a moving capsule. Its
	// AreaClass (NavArea_Obstacle) does nothing, as the original's is out of the navigation.
	Box1 = CreateDefaultSubobject<UBoxComponent>(TEXT("Box1"));
	Box1->SetupAttachment(DefaultSceneRoot);
	Box1->SetRelativeLocation(SpeedBarrierBox1Location);
	Box1->SetRelativeScale3D(SpeedBarrierBox1Scale);
	Box1->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Box1->SetCollisionObjectType(ECC_WorldStatic);
	Box1->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box1->SetCollisionResponseToChannels(FCollisionResponseContainer::GetDefaultResponseContainer());
	for (const ECollisionChannel Ignored : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody,
			ECC_Vehicle, ECC_Destructible})
	{
		Box1->SetCollisionResponseToChannel(Ignored, ECR_Ignore);
	}
	Box1->SetCanEverAffectNavigation(false);

	LoopSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/02_School/Barrier_Loop")));
	ShatterSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/02_School/Barrier_Shatter")));
	AppearParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_PortalAppear")));
	BreakParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact2")));
	BreakShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/BP_01_DoorExplode_CameraShake")));
}

void AWasamiSpeedBarrier::BeginPlay()
{
	Super::BeginPlay();
	LoadedShatterSound = ShatterSound.LoadSynchronous();
	LoadedAppearParticle = AppearParticle.LoadSynchronous();
	LoadedBreakParticle = BreakParticle.LoadSynchronous();
	LoadedBreakShake = BreakShakeClass.LoadSynchronous();
	Audio->SetSound(LoopSound.LoadSynchronous());
	Audio->Play();
}

bool AWasamiSpeedBarrier::IsPlayer(const AActor* Actor) const
{
	return Actor && Actor == UGameplayStatics::GetPlayerCharacter(this, 0);
}

void AWasamiSpeedBarrier::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	if (IsPlayer(Other) && bCanBeDestroyed)
	{
		BreakIfBoosting();
	}
}

void AWasamiSpeedBarrier::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsPlayer(OtherActor))
	{
		BreakIfBoosting();
	}
}

void AWasamiSpeedBarrier::BreakIfBoosting()
{
	// Array_Contains(Player's Active Powers, 0): the original keeps the player from BeginPlay (player 0 cast to its
	// player character); the same one is looked up here.
	const AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	const UWasamiPowerComponent* Powers = Player ? Player->GetPowers() : nullptr;
	if (!Powers || !Powers->IsUsingPower(EWasamiPower::SpeedBoost) || bShattered)
	{
		return;
	}
	bShattered = true;
	const FVector BreakLocation = StaticMesh->GetComponentLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, LoadedAppearParticle, BreakLocation, FRotator::ZeroRotator,
		FVector(SpeedBarrierAppearScale), true, EPSCPoolMethod::None, true);
	UGameplayStatics::SpawnEmitterAtLocation(this, LoadedBreakParticle, BreakLocation, FRotator::ZeroRotator,
		FVector(SpeedBarrierBreakScale), true, EPSCPoolMethod::None, true);
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0); Camera && LoadedBreakShake)
	{
		Camera->StartCameraShake(LoadedBreakShake, SpeedBarrierShakeScale, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
	}
	UGameplayStatics::PlaySound2D(this, LoadedShatterSound, SpeedBarrierShatterVolume, 1.f, 0.f);
	OnBarrierDestroyed.Broadcast();
	K2_DestroyActor();
}
