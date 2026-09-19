#include "WasamiDefib.h"

#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiGameMode.h"
#include "WasamiHitFX.h"

namespace
{
	// BP_06_Defib (pak_reference_2): the SCS templates, every one on DefaultSceneRoot.
	const FVector DefibStand01Location(0., -200., 0.);
	const FVector DefibStand02Location(-0.0017881393432617188, 200., 0.);
	const FRotator DefibStand02Rotation(0., 180.00051879882812, 0.);
	const FVector DefibStandScale(180.);
	const FVector DefibSparksLocation(0.00022321942378766835, -160., 165.);
	const FRotator DefibSparksRotation(-90., -66.57131958007812, 156.5714111328125);
	const FVector DefibSparks1Location(0.002355561126023531, 165., 165.);
	const FRotator DefibSparks1Rotation(-90., -15.945404052734375, 285.9450378417969);
	const FVector DefibBoxLocation(0., 0., 165.);
	const FVector DefibBoxScale(1., 5.980307102203369, 4.027955532073975);
	constexpr float DefibSphereRadius = 1000.f;
	constexpr float DefibZapVolume = 0.6f;

	// Fire: PlayWorldCameraShake(Self, BP_Portal_CameraShake, the actor's location, 0, 5000, 1, true).
	constexpr float DefibShakeOuterRadius = 5000.f;
}

AWasamiDefib::AWasamiDefib()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The stands: BlockAll. The original's are Static; Movable here, as the altar, so that the level's bake stays good
	// with them placed by place_dd_flow (their shadows and bounce are not in it).
	Defibrillator01 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("hospital_defibrillator_01"));
	Defibrillator01->SetupAttachment(DefaultSceneRoot);
	Defibrillator01->SetRelativeLocation(DefibStand01Location);
	Defibrillator01->SetRelativeScale3D(DefibStandScale);
	Defibrillator01->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Defibrillator02 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("hospital_defibrillator_02"));
	Defibrillator02->SetupAttachment(DefaultSceneRoot);
	Defibrillator02->SetRelativeLocationAndRotation(DefibStand02Location, DefibStand02Rotation);
	Defibrillator02->SetRelativeScale3D(DefibStandScale);
	Defibrillator02->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// The sparks: not started, nor ticking, until Fire.
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(DefaultSceneRoot);
	ParticleSystem->SetRelativeLocationAndRotation(DefibSparksLocation, DefibSparksRotation);
	ParticleSystem->bAutoActivate = false;
	ParticleSystem->PrimaryComponentTick.bStartWithTickEnabled = false;
	ParticleSystem1 = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem1"));
	ParticleSystem1->SetupAttachment(DefaultSceneRoot);
	ParticleSystem1->SetRelativeLocationAndRotation(DefibSparks1Location, DefibSparks1Rotation);
	ParticleSystem1->bAutoActivate = false;
	ParticleSystem1->PrimaryComponentTick.bStartWithTickEnabled = false;

	// Box: UBoxComponent's 32 cm scaled, Custom on the shape's own object type (world dynamic), overlapping pawns and
	// ignoring the rest. Out of the navigation, as the original's (its AreaClass, NavArea_Obstacle, blocks nothing).
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(DefaultSceneRoot);
	Box->SetRelativeLocation(DefibBoxLocation);
	Box->SetRelativeScale3D(DefibBoxScale);
	Box->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetCanEverAffectNavigation(false);
	Box->OnComponentBeginOverlap.AddDynamic(this, &AWasamiDefib::OnBoxBeginOverlap);

	// Sphere: the shape's defaults otherwise (OverlapAllDynamic), out of the navigation as Box.
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(DefaultSceneRoot);
	Sphere->InitSphereRadius(DefibSphereRadius);
	Sphere->SetCanEverAffectNavigation(false);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AWasamiDefib::OnSphereBeginOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &AWasamiDefib::OnSphereEndOverlap);

	// Its sound and attenuation are given when play begins.
	Zap = CreateDefaultSubobject<UAudioComponent>(TEXT("DD_TT_Defibrillator_Zap"));
	Zap->SetupAttachment(DefaultSceneRoot);
	Zap->bAutoActivate = false;
	Zap->SetVolumeMultiplier(DefibZapVolume);

	ZapSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_Defibrillator_Zap")));
	ZapAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/MonkeyAttenuation")));
	SparksParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/Particles/06_Hospital/P_06_Defib")));
	HitSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/07_FunPlace/Electric_Sparks_08")));
	FireShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Main/BP_Portal_CameraShake")));
}

void AWasamiDefib::BeginPlay()
{
	Super::BeginPlay();
	Zap->SetSound(ZapSound.LoadSynchronous());
	Zap->AttenuationSettings = ZapAttenuation.LoadSynchronous();
	UParticleSystem* Sparks = SparksParticle.LoadSynchronous();
	ParticleSystem->SetTemplate(Sparks);
	ParticleSystem1->SetTemplate(Sparks);
	LoadedHitSound = HitSound.LoadSynchronous();
	LoadedFireShake = FireShakeClass.LoadSynchronous();
}

bool AWasamiDefib::IsPlayer(const AActor* Actor) const
{
	return Actor && Actor == UGameplayStatics::GetPlayerCharacter(this, 0);
}

void AWasamiDefib::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// @1339: Activated, the player, and not Looping already.
	if (!bActivated || !IsPlayer(OtherActor) || bLooping)
	{
		return;
	}
	bLooping = true;
	Charge();
}

void AWasamiDefib::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// @1464: a charge under way still fires; none follows it.
	if (IsPlayer(OtherActor))
	{
		bLooping = false;
	}
}

void AWasamiDefib::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// @1560: walking into the sparks.
	if (IsPlayer(OtherActor) && bFiring)
	{
		PlayerHit();
	}
}

void AWasamiDefib::Charge()
{
	// @1009: the DoOnce, the zap from the start, Delay 2.25 → Fire → Delay 1.25 → Reset Charge.
	if (bChargeClosed)
	{
		return;
	}
	bChargeClosed = true;
	Zap->Play(0.f);
	GetWorldTimerManager().SetTimer(ChargeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Fire();
		GetWorldTimerManager().SetTimer(RechargeTimer, FTimerDelegate::CreateUObject(this, &AWasamiDefib::ResetCharge),
			RechargeDelay, false);
	}), ChargeDelay, false);
}

void AWasamiDefib::ResetCharge()
{
	// @15 → @1659: the DoOnce opened, then Charge while Looping.
	bChargeClosed = false;
	if (bLooping)
	{
		Charge();
	}
}

float AWasamiDefib::GetTimeToFire() const
{
	const FTimerManager& Timers = GetWorldTimerManager();
	if (Timers.IsTimerActive(ChargeTimer))
	{
		return Timers.GetTimerRemaining(ChargeTimer);
	}
	if (bLooping && Timers.IsTimerActive(RechargeTimer))
	{
		return Timers.GetTimerRemaining(RechargeTimer) + ChargeDelay;
	}
	return -1.f;
}

void AWasamiDefib::Fire()
{
	// @1019.
	bFiring = true;
	ParticleSystem->Activate(true);
	ParticleSystem1->Activate(true);
	if (LoadedFireShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this, LoadedFireShake, GetActorLocation(), 0.f, DefibShakeOuterRadius, 1.f, true);
	}
	if (const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0); Player && Box->IsOverlappingActor(Player))
	{
		PlayerHit();
	}
	// Delay 1 (a Delay already counting is not put back) → Firing false.
	FTimerManager& Timers = GetWorldTimerManager();
	if (!Timers.IsTimerActive(FiringTimer))
	{
		Timers.SetTimer(FiringTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { bFiring = false; }), FiringTime, false);
	}
}

void AWasamiDefib::PlayerHit()
{
	// @678: the DoOnce, the sparks' sound, the hit's flash (Shake Scale 5), Delay 0.2 → DeathEvent(the player). (The
	// achievement 06_Trap is not copied: this game has none.)
	if (bHitClosed)
	{
		return;
	}
	bHitClosed = true;
	UGameplayStatics::PlaySound2D(this, LoadedHitSound, 1.f, 1.f);
	if (AWasamiHitFX* HitFX = GetWorld()->SpawnActorDeferred<AWasamiHitFX>(AWasamiHitFX::StaticClass(), FTransform::Identity))
	{
		HitFX->ShakeScale = HitShakeScale;
		HitFX->FinishSpawning(FTransform::Identity);
	}
	GetWorldTimerManager().SetTimer(DeathTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>())
		{
			Mode->DeathEvent(UGameplayStatics::GetPlayerCharacter(this, 0));
		}
	}), DeathDelay, false);
}
