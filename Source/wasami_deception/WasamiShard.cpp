#include "WasamiShard.h"

#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/RichCurve.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiPlayerCharacter.h"
#include "WasamiTabletWidget.h"
#include "WasamiVoice.h"

namespace
{
	// BP_Shard's components (pak_reference_2): the SkeletalMesh, and under it the light and the capsule.
	const FVector ShardBodyLocation(0., 2.288818359375e-05, 97.08537292480469);
	constexpr double ShardBodyScale = 10.;
	const FVector ShardLightLocation(-0.21608541905879974, 1.6021729607018642e-05, -0.4518539607524872);
	const FVector ShardCapsuleLocation(0., -2.28881845032447e-06, 0.2914627194404602);
	constexpr double ShardChildScale = 0.1;
	constexpr float ShardCapsuleSize = 49.57180404663086f;
	const FVector ShardPlaneScale(1.5, 1.5, 10.);
	// The crystal's LDMaxDrawDistance.
	constexpr float ShardDrawDistance = 3000.f;

	// PointLight_GEN_VARIABLE. The export's FColor is B, G, R, A (255, 0, 194, 255). UE 4.24's local lights count in
	// no units, UE 5's in candelas, so the units are set.
	constexpr float ShardLightRadius = 200.f;
	constexpr float ShardLightDrawDistance = 1750.f;
	constexpr float ShardLightFadeRange = 1500.f;
	const FColor ShardLightColour(194, 0, 255, 255);
	constexpr float ShardLightScattering = 2.5f;

	// This game's mochi is 1 m across as imported; the WebGL version drew it 0.55 m, and the user asked for 1.5 times
	// that (2026-09-17), here under the crystal's scale.
	constexpr double MochiSize = 0.825;

	// BeginPlay: SetPlayRate(RandomFloatInRange(0.05, 0.15)) on the crystal. Its animation (soul_shard_skeletal_anim_loop)
	// lasts 1.6667 s at a RateScale of 0.5 and turns the crystal twice around Z (its two bones turn 7° a frame each,
	// the same way once the PSA's inverted root is undone). The skinned mesh only moves its pose while it has been
	// rendered within the last second (OnlyTickPoseWhenRendered).
	constexpr float MinSpinRate = 0.05f;
	constexpr float MaxSpinRate = 0.15f;
	constexpr float SpinLoopDegrees = 720.f;
	constexpr float SpinLoopLength = 1.6666666269302368f;
	constexpr float SpinRateScale = 0.5f;
	constexpr float SpinRenderedWithin = 1.f;

	// Activate: Shard Pull at SetPlayRate(RandomFloatInRange(0.8, 1.2)). Its Alpha track is linear, (0, 0) → (0.75, 1).
	constexpr float MinPullRate = 0.8f;
	constexpr float MaxPullRate = 1.2f;
	constexpr float PullAlphaEnd = 0.75f;

	// Collect: ClientPlayCameraShake(BP_CameraShake_ShardCollect, 0.4, CameraLocal),
	// SpawnEmitterAtLocation(P_ky_flash3, the crystal's location, no rotation, 0.2, auto destroy, no pooling, active; this
	// game spawns its purple version, P_WasamiShardFlash) and
	// PlaySound2D(Soul_Shard_Pickup_v2_Cue, NoSound ? 0 : 0.65, 1, 0, OnlyFew). The count is Clamp(count − 1, 0, 9999).
	constexpr float CollectShakeScale = 0.4f;
	constexpr double CollectFlashScale = 0.2;
	constexpr float PickupVolume = 0.65f;
	constexpr int32 MaxShardCount = 9999;

	FRichCurve MakePullCurve()
	{
		FRichCurve Curve;
		for (const FVector2f& Key : {FVector2f(0.f, 0.f), FVector2f(PullAlphaEnd, 1.f)})
		{
			Curve.GetKey(Curve.AddKey(Key.X, Key.Y)).InterpMode = RCIM_Linear;
		}
		return Curve;
	}
}

AWasamiShard::AWasamiShard()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(DefaultSceneRoot);
	Body->SetRelativeLocation(ShardBodyLocation);
	Body->SetRelativeScale3D(FVector(ShardBodyScale));

	Mochi = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mochi"));
	Mochi->SetupAttachment(Body);
	Mochi->SetRelativeScale3D(FVector(MochiSize / ShardBodyScale));
	Mochi->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Mochi->SetCanEverAffectNavigation(false);
	Mochi->SetCastShadow(false);
	Mochi->SetCullDistance(ShardDrawDistance);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Body);
	PointLight->SetRelativeLocation(ShardLightLocation);
	PointLight->SetRelativeScale3D(FVector(ShardChildScale));
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = LightIntensity;
	PointLight->AttenuationRadius = ShardLightRadius;
	PointLight->MaxDrawDistance = ShardLightDrawDistance;
	PointLight->MaxDistanceFadeRange = ShardLightFadeRange;
	PointLight->LightColor = ShardLightColour;
	PointLight->CastShadows = false;
	PointLight->VolumetricScatteringIntensity = ShardLightScattering;

	// A custom profile on the world static channel over OverlapAllDynamic's responses (UShapeComponent's default):
	// it overlaps everything and blocks nothing.
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(Body);
	Capsule->SetRelativeLocation(ShardCapsuleLocation);
	Capsule->SetRelativeScale3D(FVector(ShardChildScale));
	Capsule->InitCapsuleSize(ShardCapsuleSize, ShardCapsuleSize);
	Capsule->SetCollisionObjectType(ECC_WorldStatic);
	Capsule->OnComponentBeginOverlap.AddDynamic(this, &AWasamiShard::OnCapsuleBeginOverlap);

	Plane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane"));
	Plane->SetupAttachment(DefaultSceneRoot);
	Plane->SetRelativeLocation(FVector(0., 0., MinimapPlaneHeight));
	Plane->SetRelativeScale3D(ShardPlaneScale);
	Plane->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Plane->SetCanEverAffectNavigation(false);
	Plane->SetCastShadow(false);

	MochiMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Shard/SM_WasamiMochi")));
	PlaneMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	MapMarkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Shared/M_Shard")));
	PickupSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue")));
	PickupConcurrency = TSoftObjectPtr<USoundConcurrency>(WasamiAssets::Path(TEXT("/Game/DD/Audio/OnlyFew")));
	CollectShake = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Blueprints/Shared/BP_CameraShake_ShardCollect")));
	CollectFlash = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/Wasami/Shard/P_WasamiShardFlash")));
}

void AWasamiShard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The construction script: the crystal's material (the mochi carries its own), the mark's height, the light.
	Mochi->SetStaticMesh(MochiMesh.LoadSynchronous());
	Plane->SetStaticMesh(PlaneMesh.LoadSynchronous());
	Plane->SetMaterial(0, MapMarkMaterial.LoadSynchronous());
	Plane->SetRelativeLocation(FVector(0., 0., MinimapPlaneHeight));
	PointLight->SetIntensity(LightIntensity);
}

void AWasamiShard::BeginPlay()
{
	Super::BeginPlay();
	LoadedPickupSound = PickupSound.LoadSynchronous();
	LoadedPickupConcurrency = PickupConcurrency.LoadSynchronous();
	LoadedCollectShake = CollectShake.LoadSynchronous();
	LoadedCollectFlash = CollectFlash.LoadSynchronous();
	SpinRate = FMath::FRandRange(MinSpinRate, MaxSpinRate);
	// The original's crystals all start their animation at 0; this game's mochi start at a yaw of their own, so that
	// their faces do not all turn together (the user's ask, 2026-09-19).
	SpinAngle = FMath::FRandRange(0.f, 360.f);
	Mochi->SetRelativeRotation(FRotator(0., SpinAngle, 0.));
	PreviousLocation = GetActorLocation();
}

void AWasamiShard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsPulling())
	{
		TickPull(DeltaSeconds);
	}
	if (!IsValid(this))
	{
		return;	// collected at the pull's end
	}
	if (Mochi->WasRecentlyRendered(SpinRenderedWithin))
	{
		SpinAngle = FMath::Fmod(SpinAngle + SpinSpeed(SpinRate) * DeltaSeconds, 360.f);
		Mochi->SetRelativeRotation(FRotator(0., SpinAngle, 0.));
	}
}

void AWasamiShard::Activate_Implementation()
{
	// PlayFromStart: the position goes to 0 with an update, then the timeline plays.
	PullRate = FMath::FRandRange(MinPullRate, MaxPullRate);
	PullTime = 0.f;
	ApplyPull();
}

void AWasamiShard::TickPull(float DeltaSeconds)
{
	// UE's FTimeline: a step past the end stops at the end with a last update, then the timeline finishes.
	float NewTime = PullTime + DeltaSeconds * PullRate;
	const bool bFinished = NewTime > PullLength;
	if (bFinished)
	{
		NewTime = PullLength;
	}
	PullTime = NewTime;
	ApplyPull();
	if (bFinished)
	{
		PullTime = -1.f;
		// Collected at the end whether it arrived or not.
		Collect(false);
	}
}

void AWasamiShard::ApplyPull()
{
	// The player's place every update, so the shard follows a moving player; a sweep, which the capsule's overlaps do
	// not stop.
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Target = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	SetActorLocation(PullLocation(PreviousLocation, Target, EvaluatePullAlpha(PullTime)), true);
}

void AWasamiShard::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		Collect(false);
	}
}

void AWasamiShard::Collect(bool bNoSound)
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;

	const AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	UWasamiTabletWidget* Screen = Player ? Player->GetTabletScreen() : nullptr;
	if (!Screen)
	{
		return;
	}
	Screen->SetShardCount(FMath::Clamp(Screen->GetShardCount() - 1, 0, MaxShardCount));
	Screen->PlayCountShake();

	// @773: the game mode's Check Shards (Collect Shard, and whether any shard is left 0.05 s on, after this one is gone).
	if (AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>())
	{
		Mode->CheckShards();
	}
	if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
	{
		Controller->ClientStartCameraShake(LoadedCollectShake, CollectShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
	UGameplayStatics::SpawnEmitterAtLocation(this, LoadedCollectFlash, Body->GetComponentLocation(), FRotator::ZeroRotator,
		FVector(CollectFlashScale), true, EPSCPoolMethod::None, true);
	// Shards To Be Removed: a shard collected with its sound is remembered by where it began play, so that reopening
	// the level (a death, LAST CHECKPOINT) leaves it out.
	if (!bNoSound)
	{
		if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
		{
			Instance->RememberCollectedShard(PreviousLocation);
			// Not the original's: the WebGL version's well on the run's first shard, where there are more to take
			// (its record 04). Shards To Be Removed counts them, so a respawn's shards do not say it again.
			if (Instance->GetShardsToBeRemoved().Num() == 1 && Screen->GetShardCount() > 0)
			{
				WasamiVoice::Say(this, EWasamiVoice::Well);
			}
		}
	}
	// The original destroys the shard and then plays the sound in the same frame; here the sound goes first, while the
	// shard is still a valid world context.
	UGameplayStatics::PlaySound2D(this, LoadedPickupSound, bNoSound ? 0.f : PickupVolume, 1.f, 0.f, LoadedPickupConcurrency);
	Destroy();
}

float AWasamiShard::EvaluatePullAlpha(float Seconds)
{
	static const FRichCurve Curve = MakePullCurve();
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, PullLength));
}

FVector AWasamiShard::PullLocation(const FVector& From, const FVector& Player, float Alpha)
{
	return UKismetMathLibrary::VEase(From, FVector(Player.X, Player.Y, From.Z), Alpha, EEasingFunc::ExpoIn);
}

float AWasamiShard::SpinSpeed(float PlayRate)
{
	return SpinLoopDegrees / SpinLoopLength * SpinRateScale * PlayRate;
}
