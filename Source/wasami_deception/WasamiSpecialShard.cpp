#include "WasamiSpecialShard.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiVignetteSidesWidget.h"

namespace
{
	// BP_PowerOrb's and BP_BonusShard's components (pak_reference_2) where they are the same: under the crystal the
	// capsule and the light at 0.1; the map's mark.
	const FVector SpecialCapsuleLocation(0., -2.28881845032447e-06, 0.2914627194404602);
	const FVector SpecialLightLocation(-0.21608540415763855, 1.602172778802924e-05, -0.45185428857803345);
	constexpr double SpecialChildScale = 0.1;
	const FVector SpecialMarkLocation(0., 2.300000051036477e-05, 2000.);
	const FVector SpecialMarkScale(1.5, 1.5, 10.);

	// PointLight_GEN_VARIABLE. UE 4.24's local lights count in no units, UE 5's in candelas, so the units are set.
	constexpr float SpecialLightIntensity = 1000.f;
	constexpr float SpecialLightRadius = 200.f;

	// PlayCameraShake(BP_CameraShake_Streak, 1, CameraLocal).
	constexpr float SpecialShakeScale = 1.f;
}

AWasamiSpecialShard::AWasamiSpecialShard()
{
	// The original ticks to turn the crystal by Float 3, which is 0.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = DefaultSceneRoot;

	SoulShard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("soul_shard"));
	SoulShard->SetupAttachment(DefaultSceneRoot);
	SoulShard->SetMobility(EComponentMobility::Movable);
	SoulShard->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// UShapeComponent's default profile, OverlapAllDynamic: it overlaps everything and blocks nothing. The original's
	// NavArea_Obstacle does not count, as a body that blocks no pawn is not in the navigation.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(SoulShard);
	Capsule->SetRelativeLocation(SpecialCapsuleLocation);
	Capsule->SetRelativeScale3D(FVector(SpecialChildScale));
	Capsule->OnComponentBeginOverlap.AddDynamic(this, &AWasamiSpecialShard::OnCapsuleBeginOverlap);

	// The original's PPP_Collect_Shard under the crystal is never started, and is left out.

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(SoulShard);
	PointLight->SetRelativeLocation(SpecialLightLocation);
	PointLight->SetRelativeScale3D(FVector(SpecialChildScale));
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = SpecialLightIntensity;
	PointLight->AttenuationRadius = SpecialLightRadius;
	PointLight->CastShadows = false;

	// The original leaves the plane a static mesh's BlockAllDynamic, which puts an unseen board 20 m over the shard. As
	// the map's arrow's (AWasamiArrowPointer), it has no collision here, so that it stops nothing on Zone 2's upper floor.
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetMobility(EComponentMobility::Movable);
	StaticMesh->SetRelativeLocation(SpecialMarkLocation);
	StaticMesh->SetRelativeScale3D(SpecialMarkScale);
	StaticMesh->SetCastShadow(false);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StaticMesh->SetCanEverAffectNavigation(false);

	MapMarkMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	CollectShake = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
}

void AWasamiSpecialShard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SoulShard->SetStaticMesh(CrystalMesh.LoadSynchronous());
	SoulShard->SetMaterial(0, CrystalMaterial.LoadSynchronous());
	StaticMesh->SetStaticMesh(MapMarkMesh.LoadSynchronous());
	StaticMesh->SetMaterial(0, MapMarkMaterial.LoadSynchronous());
}

void AWasamiSpecialShard::BeginPlay()
{
	Super::BeginPlay();
	// What the pickup and the moves use, so that none of them waits on a load.
	for (const TSoftObjectPtr<UParticleSystem>& Flash : {DisappearFlash, AppearFlash, CollectImpact})
	{
		LoadedAssets.Add(Flash.LoadSynchronous());
	}
	LoadedAssets.Add(PickupSound.LoadSynchronous());
	LoadedAssets.Add(CollectShake.LoadSynchronous());
	UWasamiVignetteSidesWidget::LoadAssets(LoadedAssets);
	LoadPickupAssets(LoadedAssets);

	if (SpawnPoints.IsEmpty() && SpawnPointClass)
	{
		for (TActorIterator<AActor> It(GetWorld(), SpawnPointClass); It; ++It)
		{
			SpawnPoints.Add(*It);
		}
	}
	BeginCycle();
}

void AWasamiSpecialShard::StartSpawnTimer()
{
	// K2_SetTimerDelegate(Spawn …, Shard Spawn Time, not looping): the one timer, set again after each move.
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AWasamiSpecialShard::SpawnSpecialShard, ShardSpawnTime, false);
}

float AWasamiSpecialShard::GetTimeToSpawn() const
{
	return GetWorldTimerManager().GetTimerRemaining(SpawnTimer);
}

FVector AWasamiSpecialShard::GetCrystalLocation() const
{
	return IsValid(SoulShard) ? SoulShard->GetComponentLocation() : RemovedCrystalLocation;
}

void AWasamiSpecialShard::SpawnSpecialShard()
{
	if (bFlickering || bCollected)
	{
		return;
	}
	bFlickering = true;
	Flicker();
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(FlickerTimer, this, &AWasamiSpecialShard::Flicker, FlickerPeriod, true);
	Timers.SetTimer(FlickerDelay, this, &AWasamiSpecialShard::FinishFlicker, FlickerLength, false);
}

void AWasamiSpecialShard::Flicker()
{
	bFlickerHidden = !bFlickerHidden;
	DefaultSceneRoot->SetVisibility(!bFlickerHidden, true);
}

void AWasamiSpecialShard::FinishFlicker()
{
	Flicker();
	GetWorldTimerManager().ClearTimer(FlickerTimer);
	bFlickering = false;
	DefaultSceneRoot->SetVisibility(true, true);
	// RandomIntegerInRange(0, n − 1): the point it is at may come again (the original's "not chosen yet" is cut off).
	MoveToSpawnPoint(SpawnPoints.IsEmpty() ? INDEX_NONE : FMath::RandRange(0, SpawnPoints.Num() - 1));
}

void AWasamiSpecialShard::MoveToSpawnPoint(int32 Index)
{
	// Not stopped by the pickup: a bonus shard taken while it flickers stays for its reveal with the flicker running,
	// and at the Delay's end flashes twice where its crystal was (the crystal's component is gone and stays behind).
	SpawnFlash(DisappearFlash.LoadSynchronous(), MoveFlashScale);
	if (SpawnPoints.IsValidIndex(Index) && SpawnPoints[Index])
	{
		SetActorLocation(SpawnPoints[Index]->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	StartSpawnTimer();
	SpawnFlash(AppearFlash.LoadSynchronous(), MoveFlashScale);
}

void AWasamiSpecialShard::SpawnFlash(UParticleSystem* Template, float Scale) const
{
	if (Template)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, Template, GetCrystalLocation(), FRotator::ZeroRotator,
			FVector(Scale), true, EPSCPoolMethod::None, true);
	}
}

void AWasamiSpecialShard::PlayCollectShake() const
{
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Camera->StartCameraShake(CollectShake.LoadSynchronous(), SpecialShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
}

void AWasamiSpecialShard::RemoveCrystal()
{
	if (IsValid(SoulShard))
	{
		RemovedCrystalLocation = SoulShard->GetComponentLocation();
		SoulShard->DestroyComponent();
	}
}

void AWasamiSpecialShard::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Collect();
	}
}

void AWasamiSpecialShard::Collect()
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;
	GetWorldTimerManager().ClearTimer(SpawnTimer);
	CollectShard();
}
