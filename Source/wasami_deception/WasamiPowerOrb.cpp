#include "WasamiPowerOrb.h"

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
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemyInterface.h"
#include "WasamiSpecialSpawnPoint.h"
#include "WasamiStunCollectEffect.h"
#include "WasamiVignetteSidesWidget.h"

namespace
{
	// BP_PowerOrb's components (pak_reference_2): the crystal, and under it the capsule and the light; the map's mark.
	const FVector OrbCrystalLocation(0., 2.288818359375e-05, 125.25003051757812);
	constexpr double OrbCrystalScale = 0.5407835245132446;
	const FVector OrbCapsuleLocation(0., -2.28881845032447e-06, 0.2914627194404602);
	const FVector OrbLightLocation(-0.21608540415763855, 1.602172778802924e-05, -0.45185428857803345);
	constexpr double OrbChildScale = 0.1;
	constexpr float OrbCapsuleSize = 840.6161499023438f;
	const FVector OrbMarkLocation(0., 2.300000051036477e-05, 2000.);
	const FVector OrbMarkScale(1.5, 1.5, 10.);

	// PointLight_GEN_VARIABLE. The export's FColor is B, G, R, A (0, 146, 255, 255). UE 4.24's local lights count in no
	// units, UE 5's in candelas, so the units are set.
	constexpr float OrbLightIntensity = 1000.f;
	constexpr float OrbLightRadius = 200.f;
	const FColor OrbLightColour(255, 146, 0, 255);

	// SpawnEmitterAtLocation(…, the crystal's location, no rotation, scale, auto destroy, no pooling, active).
	constexpr float MoveFlashScale = 0.5f;
	constexpr float CollectImpactScale = 1.f;
	// PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75); PlaySoundAtLocation(8-Dark_power_ball_countdown_, the origin, 1,
	// 1) (the wave has no attenuation); PlayCameraShake(BP_CameraShake_Streak, 1, CameraLocal).
	constexpr float PickupVolume = 0.8f;
	constexpr float PickupPitch = 0.75f;
	constexpr float CountdownVolume = 1.f;
	constexpr float CountdownPitch = 1.f;
	constexpr float CollectShakeScale = 1.f;

	// What the pickup stuns (the enemies carry it, WasamiEnemy.cpp).
	const FName EnemyTag(TEXT("Enemy"));

	FAutoConsoleCommandWithWorldAndArgs PowerOrbCommand(TEXT("Wasami.PowerOrb"),
		TEXT("Wasami.PowerOrb [N]: the level's power orbs start their 5 s flicker now (Spawn Power Orb); with N, they move to their spawn point N at once instead (the flashes, the next 150 s)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AWasamiPowerOrb> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->MoveToSpawnPoint(FCString::Atoi(*Args[0]));
				}
				else
				{
					It->SpawnPowerOrb();
				}
			}
		}));
}

AWasamiPowerOrb::AWasamiPowerOrb()
{
	// The original ticks to turn the crystal by Float 3, which is 0.
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	DefaultSceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = DefaultSceneRoot;

	SoulShard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("soul_shard"));
	SoulShard->SetupAttachment(DefaultSceneRoot);
	SoulShard->SetMobility(EComponentMobility::Movable);
	SoulShard->SetRelativeLocation(OrbCrystalLocation);
	SoulShard->SetRelativeScale3D(FVector(OrbCrystalScale));
	SoulShard->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// UShapeComponent's default profile, OverlapAllDynamic: it overlaps everything and blocks nothing. The original's
	// NavArea_Obstacle does not count, as a body that blocks no pawn is not in the navigation.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(SoulShard);
	Capsule->SetRelativeLocation(OrbCapsuleLocation);
	Capsule->SetRelativeScale3D(FVector(OrbChildScale));
	Capsule->InitCapsuleSize(OrbCapsuleSize, OrbCapsuleSize);
	Capsule->OnComponentBeginOverlap.AddDynamic(this, &AWasamiPowerOrb::OnCapsuleBeginOverlap);

	// The original's PPP_Collect_Shard under the crystal is never started, and is left out.

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(SoulShard);
	PointLight->SetRelativeLocation(OrbLightLocation);
	PointLight->SetRelativeScale3D(FVector(OrbChildScale));
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = OrbLightIntensity;
	PointLight->AttenuationRadius = OrbLightRadius;
	PointLight->LightColor = OrbLightColour;
	PointLight->CastShadows = false;

	// The default collision of a static mesh, as in the original (the shard's mark has none).
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
	StaticMesh->SetMobility(EComponentMobility::Movable);
	StaticMesh->SetRelativeLocation(OrbMarkLocation);
	StaticMesh->SetRelativeScale3D(OrbMarkScale);
	StaticMesh->SetCastShadow(false);

	CrystalMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/Shared/power_orb")));
	CrystalMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Fords_Materials/m_crystal_Inst3")));
	MapMarkMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	MapMarkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Shared/M_PowerOrb")));
	DisappearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Disappear")));
	AppearFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Appear")));
	CollectImpact = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact")));
	PickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue")));
	CountdownSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_")));
	CollectShake = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
	CollectEffectClass = AWasamiStunCollectEffect::StaticClass();
}

void AWasamiPowerOrb::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SoulShard->SetStaticMesh(CrystalMesh.LoadSynchronous());
	SoulShard->SetMaterial(0, CrystalMaterial.LoadSynchronous());
	StaticMesh->SetStaticMesh(MapMarkMesh.LoadSynchronous());
	StaticMesh->SetMaterial(0, MapMarkMaterial.LoadSynchronous());
}

void AWasamiPowerOrb::BeginPlay()
{
	Super::BeginPlay();
	// What the pickup and the moves use, so that none of them waits on a load.
	for (const TSoftObjectPtr<UParticleSystem>& Flash : {DisappearFlash, AppearFlash, CollectImpact})
	{
		LoadedAssets.Add(Flash.LoadSynchronous());
	}
	LoadedAssets.Add(PickupSound.LoadSynchronous());
	LoadedAssets.Add(CountdownSound.LoadSynchronous());
	LoadedAssets.Add(CollectShake.LoadSynchronous());
	if (CollectEffectClass)
	{
		CollectEffectClass->GetDefaultObject<AWasamiStunCollectEffect>()->LoadDefaultAssets(LoadedAssets);
	}
	UWasamiVignetteSidesWidget::LoadAssets(LoadedAssets);

	if (SpawnPoints.IsEmpty())
	{
		for (TActorIterator<AWasamiPowerOrbSpawnPoint> It(GetWorld()); It; ++It)
		{
			SpawnPoints.Add(*It);
		}
	}
	StartSpawnTimer();
}

void AWasamiPowerOrb::StartSpawnTimer()
{
	// K2_SetTimerDelegate(Spawn Power Orb, Shard Spawn Time, not looping): the one timer, set again after each move.
	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AWasamiPowerOrb::SpawnPowerOrb, ShardSpawnTime, false);
}

float AWasamiPowerOrb::GetTimeToSpawn() const
{
	return GetWorldTimerManager().GetTimerRemaining(SpawnTimer);
}

void AWasamiPowerOrb::SpawnPowerOrb()
{
	if (bFlickering || bCollected)
	{
		return;
	}
	bFlickering = true;
	Flicker();
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.SetTimer(FlickerTimer, this, &AWasamiPowerOrb::Flicker, FlickerPeriod, true);
	Timers.SetTimer(FlickerDelay, this, &AWasamiPowerOrb::FinishFlicker, FlickerLength, false);
}

void AWasamiPowerOrb::Flicker()
{
	bFlickerHidden = !bFlickerHidden;
	DefaultSceneRoot->SetVisibility(!bFlickerHidden, true);
}

void AWasamiPowerOrb::FinishFlicker()
{
	Flicker();
	GetWorldTimerManager().ClearTimer(FlickerTimer);
	bFlickering = false;
	DefaultSceneRoot->SetVisibility(true, true);
	// RandomIntegerInRange(0, n − 1): the point it is at may come again (the original's "not chosen yet" is cut off).
	MoveToSpawnPoint(SpawnPoints.IsEmpty() ? INDEX_NONE : FMath::RandRange(0, SpawnPoints.Num() - 1));
}

void AWasamiPowerOrb::MoveToSpawnPoint(int32 Index)
{
	if (bCollected)
	{
		return;
	}
	SpawnFlash(DisappearFlash.LoadSynchronous(), MoveFlashScale);
	if (SpawnPoints.IsValidIndex(Index) && SpawnPoints[Index])
	{
		SetActorLocation(SpawnPoints[Index]->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	StartSpawnTimer();
	SpawnFlash(AppearFlash.LoadSynchronous(), MoveFlashScale);
}

void AWasamiPowerOrb::SpawnFlash(UParticleSystem* Template, float Scale) const
{
	if (Template)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, Template, SoulShard->GetComponentLocation(), FRotator::ZeroRotator,
			FVector(Scale), true, EPSCPoolMethod::None, true);
	}
}

void AWasamiPowerOrb::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Collect();
	}
}

void AWasamiPowerOrb::Collect()
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;
	FTimerManager& Timers = GetWorldTimerManager();
	Timers.ClearTimer(SpawnTimer);
	Timers.ClearTimer(FlickerTimer);
	SpawnFlash(CollectImpact.LoadSynchronous(), CollectImpactScale);

	// The original destroys the orb here and goes on; it goes last here. Its Used Stun Orbs? on the game instance is
	// for an achievement only and is not copied.
	UGameplayStatics::PlaySound2D(this, PickupSound.LoadSynchronous(), PickupVolume, PickupPitch);
	UWasamiVignetteSidesWidget::Show(this, UWasamiVignetteSidesWidget::StunnedColor, true, UWasamiVignetteSidesWidget::StunnedText());
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Camera->StartCameraShake(CollectShake.LoadSynchronous(), CollectShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
	UGameplayStatics::PlaySoundAtLocation(this, CountdownSound.LoadSynchronous(), FVector::ZeroVector, CountdownVolume, CountdownPitch);
	// BP_StunCollectEffect at the origin, unrotated; it moves itself onto the player.
	if (CollectEffectClass)
	{
		GetWorld()->SpawnActor<AWasamiStunCollectEffect>(CollectEffectClass, FTransform::Identity);
	}
	StunAllEnemies(this);
	Destroy();
}

int32 AWasamiPowerOrb::StunAllEnemies(const UObject* WorldContextObject)
{
	// GetAllActorsWithTag(Enemy), then a cast to DD_EnemyInterface: only the implementers are stunned.
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, EnemyTag, Tagged);
	int32 Stunned = 0;
	for (AActor* Each : Tagged)
	{
		if (Each && Each->Implements<UWasamiEnemyInterface>())
		{
			IWasamiEnemyInterface::Execute_SetState(Each, EWasamiEnemyState::Stun, true);
			++Stunned;
		}
	}
	return Stunned;
}
