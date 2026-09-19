#include "WasamiSawTrap.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiBlackScreenWidget.h"
#include "WasamiGameMode.h"
#include "WasamiHitFX.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// The SCS templates (pak_reference_2): BP_06_TrapBase's Mesh (LDMaxDrawDistance 5000) and BP_06_sawTrap_medium's
	// scale on it, and its Box (UBoxComponent's 32 cm extent, scaled; under Mesh's scale through collision attach).
	constexpr float SawTrapDrawDistance = 5000.f;
	constexpr double SawTrapMediumScale = 1.5;
	const FVector SawTrapMediumBoxScale(1.2858065366744995, 0.051492467522621155, 1.344196081161499);
	// The children's InheritableComponentHandler records: Mesh's scale, Box's scale (short01's Box turns by 5e-22°, left
	// out).
	constexpr double SawTrapShortScale = 1.7999999523162842;
	constexpr double SawTrapLongScale = 1.;
	const FVector SawTrapShort01BoxScale(1.3157035112380981, 0.051492467522621155, 1.3740930557250977);
	const FVector SawTrapShort02BoxScale(1.1945092678070068, 0.051492467522621155, 1.252898931503296);
	const FVector SawTrapLong01BoxScale(5.382521629333496, 0.051492467522621155, 5.440911293029785);

	// medium's Audio_GEN_VARIABLE: SFX_Matron_SawLoop at 0.5 (its pitch 0.5, which the construction script replaces),
	// and its attenuation overrides (the rest at FSoundAttenuationSettings' defaults).
	constexpr float SawTrapLoopVolume = 0.5f;
	constexpr float SawTrapLoopPitch = 0.5f;
	constexpr float SawTrapOcclusionLowPass = 4000.f;
	constexpr float SawTrapOcclusionVolume = 0.3f;
	constexpr float SawTrapOcclusionTime = 0.3f;
	// short01's own on top of them.
	constexpr float SawTrapShort01OcclusionVolume = 0.2f;
	constexpr float SawTrapShort01OcclusionTime = 1.f;
	constexpr float SawTrapShort01ShapeRadius = 100.f;
	constexpr float SawTrapShort01FalloffDistance = 1500.f;

	// short01's PointLight_GEN_VARIABLE (UE 4.24's point light defaults otherwise; its intensity is unitless). Its colour
	// is the export's [B, G, R, A] (251, 254, 255, 255) turned round (troubleshooting.md): a cold white.
	const FVector SawTrapLightLocation(0., -5.12607937773238e-23, 113.61906433105469);
	const FVector SawTrapLightScale(3.260774612426758, 0.09268643707036972, 3.3658759593963623);
	constexpr float SawTrapLightSoftSourceRadius = 100.f;
	constexpr float SawTrapLightAttenuationRadius = 250.f;
	constexpr float SawTrapLightMaxDrawDistance = 3000.f;
	constexpr float SawTrapLightMaxDistanceFadeRange = 1500.f;
	constexpr float SawTrapLightIntensity = 1750.f;
	const FColor SawTrapLightColor(255, 254, 251, 255);

	/** '/Game/DD/Meshes/06_Hospital/hospital_sawTrap_<Kind>_anim' and its _Anim (dd_skeletal.import_saw_traps). */
	FSoftObjectPath SawTrapMeshPath(const TCHAR* Kind)
	{
		return WasamiAssets::Path(*FString::Printf(TEXT("/Game/DD/Meshes/06_Hospital/hospital_sawTrap_%s_anim"), Kind));
	}

	FSoftObjectPath SawTrapAnimPath(const TCHAR* Kind)
	{
		return WasamiAssets::Path(*FString::Printf(TEXT("/Game/DD/Meshes/06_Hospital/hospital_sawTrap_%s_anim_Anim"), Kind));
	}

	/**
	 * BP_DD_Functions' Disable Player Movement: the player's CanMove? off and its movement stopped at once, and every
	 * camera shake of player 0's camera stopped (not letting them blend out).
	 */
	void DisablePlayerMovement(const UObject* WorldContextObject)
	{
		if (AWasamiPlayerCharacter* Player = Cast<AWasamiPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0)))
		{
			Player->bCanMove = false;
			Player->GetCharacterMovement()->StopMovementImmediately();
		}
		if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0))
		{
			Camera->StopAllCameraShakes(false);
		}
	}
}

const FName AWasamiSawTrap::SawSocket(TEXT("sawSocket"));

AWasamiSawTrap::AWasamiSawTrap()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetRelativeScale3D(FVector(SawTrapMediumScale));
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Mesh->SetCullDistance(SawTrapDrawDistance);

	CollisionAttach = CreateDefaultSubobject<USceneComponent>(TEXT("collision attach"));
	CollisionAttach->SetupAttachment(Root);

	// Box: Custom on UBoxComponent's defaults (OverlapAllDynamic: a WorldDynamic object), query and physics, ignoring
	// the engine's channels but pawns, which it overlaps; the other channels at their defaults (the original's list holds
	// only what differs from them). Its NavArea_Obstacle is the shape's default, and it blocks nothing, so it is not in
	// the navigation.
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(CollisionAttach);
	Box->SetRelativeScale3D(SawTrapMediumBoxScale);
	Box->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToChannels(FCollisionResponseContainer::GetDefaultResponseContainer());
	for (const ECollisionChannel Ignored : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody,
			ECC_Vehicle, ECC_Destructible})
	{
		Box->SetCollisionResponseToChannel(Ignored, ECR_Ignore);
	}
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Box->SetNotifyRigidBodyCollision(true);

	// The whine starts when play begins (its sound is loaded then), as the original's does when the level starts.
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Box);
	Audio->bAutoActivate = false;
	Audio->SetVolumeMultiplier(SawTrapLoopVolume);
	Audio->SetPitchMultiplier(SawTrapLoopPitch);
	Audio->bOverrideAttenuation = true;
	Audio->AttenuationOverrides.bEnableOcclusion = true;
	Audio->AttenuationOverrides.bUseComplexCollisionForOcclusion = true;
	Audio->AttenuationOverrides.OcclusionLowPassFilterFrequency = SawTrapOcclusionLowPass;
	Audio->AttenuationOverrides.OcclusionVolumeAttenuation = SawTrapOcclusionVolume;
	Audio->AttenuationOverrides.OcclusionInterpolationTime = SawTrapOcclusionTime;
	Audio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(SawTrapMeshPath(TEXT("medium_01")));
	AnimAsset = TSoftObjectPtr<UAnimSequence>(SawTrapAnimPath(TEXT("medium_01")));
	LoopSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Animation/Enemies/Nurse/Matron/Sounds/SFX_Matron_SawLoop")));
}

void AWasamiSawTrap::ApplyMeshAssets()
{
	Mesh->AnimationData.AnimToPlay = AnimAsset.LoadSynchronous();
	Mesh->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
}

void AWasamiSawTrap::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();
	// Before Mesh registers and starts its animation (a spawned actor registers its components before OnConstruction).
	ApplyMeshAssets();
}

void AWasamiSawTrap::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyMeshAssets();
	// BP_06_TrapBase's construction script: K2_AttachToComponent(Mesh, 'sawSocket', SnapToTarget, KeepWorld,
	// KeepRelative, weld). Onto the blade at its first frame, turned as the actor (every placed one's then turns by a
	// roll of −90° from the socket), and it goes with the blade from then on.
	CollisionAttach->AttachToComponent(Mesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld, EAttachmentRule::KeepRelative, true), SawSocket);
	// BP_06_sawTrap_medium's: SetPitchMultiplier(RandomFloatInRange(1, 1.2)) (each placed one keeps its own).
	Audio->SetPitchMultiplier(FMath::FRandRange(MinPitch, MaxPitch));
}

void AWasamiSawTrap::BeginPlay()
{
	Super::BeginPlay();
	Audio->SetSound(LoopSound.LoadSynchronous());
	Audio->Play();
}

void AWasamiSawTrap::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		CatchPlayer();
	}
}

void AWasamiSawTrap::CatchPlayer()
{
	// @978: the DoOnce; @1013: the whine's component destroyed, then a Sequence: Disable Player Movement → the hit's
	// flash (@658) → Delay 0.1 → CreateAndAddWidget(UMG_BlackScreen, Z 0) → Delay 0.3 → DeathEvent(the player); and
	// (@15) a loop over GetAllActorsOfClass(BP_06_sawTrap_medium) with nothing in it. (The achievement 06_Trap is not
	// copied: this game has none.)
	if (bCaught)
	{
		return;
	}
	bCaught = true;
	if (Audio)
	{
		Audio->DestroyComponent();
	}
	DisablePlayerMovement(this);
	if (AWasamiHitFX* HitFX = GetWorld()->SpawnActorDeferred<AWasamiHitFX>(AWasamiHitFX::StaticClass(), FTransform::Identity))
	{
		HitFX->ShakeScale = HitShakeScale;
		HitFX->FinishSpawning(FTransform::Identity);
	}
	GetWorldTimerManager().SetTimer(BlackScreenTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UWasamiBlackScreenWidget::Show(this, 0);
		GetWorldTimerManager().SetTimer(DeathTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (AWasamiGameMode* Mode = GetWorld()->GetAuthGameMode<AWasamiGameMode>())
			{
				Mode->DeathEvent(UGameplayStatics::GetPlayerCharacter(this, 0));
			}
		}), DeathDelay, false);
	}), BlackScreenDelay, false);
}

AWasamiSawTrapShort01::AWasamiSawTrapShort01()
{
	Mesh->SetRelativeScale3D(FVector(SawTrapShortScale));
	Box->SetRelativeScale3D(SawTrapShort01BoxScale);
	Audio->AttenuationOverrides.OcclusionVolumeAttenuation = SawTrapShort01OcclusionVolume;
	Audio->AttenuationOverrides.OcclusionInterpolationTime = SawTrapShort01OcclusionTime;
	Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(SawTrapShort01ShapeRadius, 0., 0.);
	Audio->AttenuationOverrides.FalloffDistance = SawTrapShort01FalloffDistance;
	MeshAsset = TSoftObjectPtr<USkeletalMesh>(SawTrapMeshPath(TEXT("short_01")));
	AnimAsset = TSoftObjectPtr<UAnimSequence>(SawTrapAnimPath(TEXT("short_01")));

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Root);
	PointLight->SetRelativeLocation(SawTrapLightLocation);
	PointLight->SetRelativeScale3D(SawTrapLightScale);
	PointLight->SetMobility(EComponentMobility::Movable);
	PointLight->IntensityUnits = ELightUnits::Unitless;
	PointLight->Intensity = SawTrapLightIntensity;
	PointLight->LightColor = SawTrapLightColor;
	PointLight->SoftSourceRadius = SawTrapLightSoftSourceRadius;
	PointLight->AttenuationRadius = SawTrapLightAttenuationRadius;
	PointLight->MaxDrawDistance = SawTrapLightMaxDrawDistance;
	PointLight->MaxDistanceFadeRange = SawTrapLightMaxDistanceFadeRange;
	PointLight->CastShadows = false;
}

AWasamiSawTrapShort02::AWasamiSawTrapShort02()
{
	Mesh->SetRelativeScale3D(FVector(SawTrapShortScale));
	Box->SetRelativeScale3D(SawTrapShort02BoxScale);
	MeshAsset = TSoftObjectPtr<USkeletalMesh>(SawTrapMeshPath(TEXT("short_02")));
	AnimAsset = TSoftObjectPtr<UAnimSequence>(SawTrapAnimPath(TEXT("short_02")));
}

AWasamiSawTrapLong01::AWasamiSawTrapLong01()
{
	Mesh->SetRelativeScale3D(FVector(SawTrapLongScale));
	Box->SetRelativeScale3D(SawTrapLong01BoxScale);
	MeshAsset = TSoftObjectPtr<USkeletalMesh>(SawTrapMeshPath(TEXT("long_01")));
	AnimAsset = TSoftObjectPtr<UAnimSequence>(SawTrapAnimPath(TEXT("long_01")));
}
