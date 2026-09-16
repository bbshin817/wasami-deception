#include "WasamiTeleportAim.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiCameraAnim.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// BP_Power_Teleport (pak_reference): the trace looks this far down from the point in front of the player, for the
	// Teleport object channel (ObjectTypeQuery7 = the first custom object channel, ECC_GameTraceChannel1).
	constexpr float TraceDepth = 500.f;
	const ECollisionChannel TeleportChannel = ECC_GameTraceChannel1;
	// Distance = Lerp(MinDistance, Max Distance, Alpha); the wheel's value is divided by this before it moves Alpha.
	constexpr float MinDistance = 250.f;
	constexpr float WheelDivisor = 10.f;
	// The spring arm's position lag comes on this long after the spawn.
	constexpr float LagDelay = 0.5f;
	// A click puts the destination this far over the decal's spot (the capsule's centre); the move comes this late.
	constexpr float DestinationRise = 125.f;
	constexpr float CommitDelay = 0.11999999731779099f;
	// The aiming loop's AudioComponent, and PlaySound2D / PlayCameraShake on the move.
	constexpr float AimingLoopVolume = 0.6499999761581421f;
	constexpr float CommittedVolume = 1.f;
	constexpr float CommittedShakeScale = 1.f;
	// PlayCameraAnim(CameraAnim_Teleport, Rate, Scale, BlendInTime, BlendOutTime, bLoop, bRandomStartTime false,
	// Duration) on the click.
	constexpr float ConfirmAnimRate = 1.f;
	constexpr float ConfirmAnimScale = 1.f;
	constexpr float ConfirmAnimBlendTime = 0.f;
	constexpr bool bConfirmAnimLoop = false;
	constexpr float ConfirmAnimDuration = 0.f;
	// The decal: DecalSize (half extents, X along the projection) and its transform on the arm.
	const FVector DecalSize(3.f, 100.f, 100.f);
	const FRotator DecalRotation(-90.f, 0.f, 5.4641506721964106e-05f);
	const FVector DecalScale(3.326378583908081f, 1.f, 1.f);
	// The particle system on the decal. With the decal's transform it ends up unrotated in the world, scaled
	// (0.665, 0.2, 0.2), 13.75 cm over the decal's centre (the trace's hit).
	const FVector ParticleLocation(-4.134940147399902f, -0.00026260362938046455f, 2.3245811462402344e-06f);
	const FRotator ParticleRotation(90.f, 0.9113311767578125f, -359.0887451171875f);
	const FVector ParticleScale(0.2f, 0.2f, 0.2f);
}

AWasamiTeleportAim::AWasamiTeleportAim()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = SceneRoot;

	// Only the arm's length is set; its lag speed (10), substepping and the rest are UE's defaults, as in the original.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 0.f;

	// On the arm without a socket: the arm's socket transform (its lagged end) is what any child follows.
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(SpringArm);
	Decal->DecalSize = DecalSize;
	Decal->SetRelativeRotation(DecalRotation);
	Decal->SetRelativeScale3D(DecalScale);

	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(Decal);
	ParticleSystem->SetRelativeLocation(ParticleLocation);
	ParticleSystem->SetRelativeRotation(ParticleRotation);
	ParticleSystem->SetRelativeScale3D(ParticleScale);

	// No attenuation: the loop is heard as it is, wherever the aim is.
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(SceneRoot);
	Audio->VolumeMultiplier = AimingLoopVolume;

	AimingLoopSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227")));
	CommittedSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Committed")));
	CommittedShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak")));
	ConfirmCameraAnim = TSoftObjectPtr<UWasamiCameraAnim>(WasamiAssets::Path(TEXT("/Game/DD/Animation/Camera/CameraAnim_Teleport")));
	DecalMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Blueprints/Main/Powers/M_Decal_Teleport")));
	AimParticles = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2")));
}

void AWasamiTeleportAim::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const AWasamiTeleportAim* Defaults = GetDefault<AWasamiTeleportAim>();
	Out.Add(Defaults->AimingLoopSound.LoadSynchronous());
	Out.Add(Defaults->CommittedSound.LoadSynchronous());
	Out.Add(Defaults->CommittedShakeClass.LoadSynchronous());
	Out.Add(Defaults->ConfirmCameraAnim.LoadSynchronous());
	Out.Add(Defaults->DecalMaterial.LoadSynchronous());
	Out.Add(Defaults->AimParticles.LoadSynchronous());
}

float AWasamiTeleportAim::DistanceFor(float InAlpha, float InMaxDistance)
{
	return FMath::Lerp(MinDistance, InMaxDistance, InAlpha);
}

float AWasamiTeleportAim::StepAlpha(float InAlpha, float AxisValue)
{
	return FMath::Clamp(AxisValue / WheelDivisor + InAlpha, 0.f, 1.f);
}

void AWasamiTeleportAim::BeginPlay()
{
	Super::BeginPlay();
	LoadedCommittedSound = CommittedSound.LoadSynchronous();
	LoadedCommittedShake = CommittedShakeClass.LoadSynchronous();
	LoadedConfirmCameraAnim = ConfirmCameraAnim.LoadSynchronous();
	Audio->SetSound(AimingLoopSound.LoadSynchronous());
	Audio->Play();
	Decal->SetDecalMaterial(DecalMaterial.LoadSynchronous());
	ParticleSystem->SetTemplate(AimParticles.LoadSynchronous());

	// The original's wheel binding writes Distance on every frame, zero or not: from the first frame it is the one for
	// Alpha 0.6 and the spawned Max Distance.
	Distance = DistanceFor(Alpha, MaxDistance);
	GetWorldTimerManager().SetTimer(LagTimer, this, &AWasamiTeleportAim::EnableLag, LagDelay, false);
}

void AWasamiTeleportAim::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LagTimer);
	GetWorldTimerManager().ClearTimer(CommitTimer);
	Super::EndPlay(EndPlayReason);
}

AWasamiPlayerCharacter* AWasamiTeleportAim::GetPlayer() const
{
	return Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

void AWasamiTeleportAim::EnableLag()
{
	SpringArm->bEnableCameraLag = true;
}

void AWasamiTeleportAim::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AWasamiPlayerCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	// From the capsule's centre, along the actor's facing (its yaw), then straight down; complex collision, self ignored.
	const FVector Start = Player->GetActorLocation() + Player->GetActorForwardVector() * Distance;
	const FVector End = Start - FVector(0., 0., TraceDepth);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WasamiTeleportAim), true);
	Params.AddIgnoredActor(this);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(TeleportChannel), Params))
	{
		// Nothing to stand on: the arm stays where it was.
		return;
	}
	SpringArm->SetWorldLocation(Hit.Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void AWasamiTeleportAim::AdjustDistance(float AxisValue)
{
	Alpha = StepAlpha(Alpha, AxisValue);
	Distance = DistanceFor(Alpha, MaxDistance);
}

void AWasamiTeleportAim::Confirm()
{
	// The destination is written before the DoOnce, so a click during the wait moves it.
	Location = Decal->GetComponentLocation() + FVector(0., 0., DestinationRise);
	if (bConfirmed)
	{
		return;
	}
	bConfirmed = true;
	// On the first player's camera; a take-back or the aim's end leaves it playing.
	if (UWasamiCameraAnimModifier* Anims = UWasamiCameraAnimModifier::Get(UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		Anims->Play(LoadedConfirmCameraAnim, ConfirmAnimRate, ConfirmAnimScale, ConfirmAnimBlendTime, ConfirmAnimBlendTime,
			bConfirmAnimLoop, ConfirmAnimDuration);
	}
	// While the player moves, its capsule passes through world-dynamic things and pawns.
	if (const AWasamiPlayerCharacter* Player = GetPlayer())
	{
		UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
		Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	GetWorldTimerManager().SetTimer(CommitTimer, this, &AWasamiTeleportAim::Commit, CommitDelay, false);
}

void AWasamiTeleportAim::Commit()
{
	AWasamiPlayerCharacter* Player = GetPlayer();
	const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
	if (PC && PC->PlayerCameraManager && LoadedCommittedShake)
	{
		PC->PlayerCameraManager->StartCameraShake(LoadedCommittedShake, CommittedShakeScale, ECameraShakePlaySpace::CameraLocal);
	}
	UGameplayStatics::PlaySound2D(this, LoadedCommittedSound, CommittedVolume);
	if (Player)
	{
		// A sweep: walls and closed doors stop the capsule on the way.
		Player->SetActorLocation(Location, true, nullptr, ETeleportType::TeleportPhysics);
		UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}
	OnUsed.Broadcast();
	Destroy();
}
