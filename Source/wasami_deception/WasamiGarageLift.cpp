#include "WasamiGarageLift.h"

#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "AnimationRuntime.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"

DEFINE_LOG_CATEGORY_STATIC(LogWasamiGarageLift, Log, All);

namespace
{
	// BP_06_GarageLift (pak_reference_2): the SCS templates. The boxes sit on joint4, keep UBoxComponent's 32 cm extent
	// and are sized by their scale under the mesh's 30.
	constexpr double LiftMeshScale = 30.;
	const FRotator LiftBoxRotation(0., 0., -90.000244140625);
	const FVector LiftBoxScale(0.20314204692840576, 0.27342960238456726, 0.009533732198178768);
	const FVector LiftOverlapBoxLocation(0., -2.33333158493042, 0.);
	const FRotator LiftOverlapBoxRotation(0., 0., -90.00023651123047);
	const FVector LiftOverlapBoxScale(0.20314200222492218, 0.2734299898147583, 0.07126771658658981);

	// Both of the ABP's sequence players play it.
	const TCHAR* const LiftAnimPackage = TEXT("/Game/DD/Meshes/06_Hospital/hospital_garage_lift_anim_Anim");

	/**
	 * Custom collision on UBoxComponent's defaults (OverlapAllDynamic: a WorldDynamic object) that ignores the engine's
	 * channels but pawns. The project's Teleport channel is not listed, so it keeps its default (Overlap).
	 */
	void RespondToPawnsOnly(UBoxComponent* Box, ECollisionResponse PawnResponse)
	{
		for (const ECollisionChannel Channel : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody,
				 ECC_Vehicle, ECC_Destructible})
		{
			Box->SetCollisionResponseToChannel(Channel, ECR_Ignore);
		}
		Box->SetCollisionResponseToChannel(ECC_Pawn, PawnResponse);
	}
}

const FName AWasamiGarageLift::PlatformBone(TEXT("joint4"));

AWasamiGarageLift::AWasamiGarageLift()
{
	PrimaryActorTick.bCanEverTick = false;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// The engine's default collision for it (NoCollision), as the original's.
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(DefaultSceneRoot);
	SkeletalMesh->SetRelativeScale3D(FVector(LiftMeshScale));
	SkeletalMesh->AnimClass = UWasamiGarageLiftAnimInstance::StaticClass();

	// Box_GEN_VARIABLE: query and physics, blocking pawns. (Its NavArea_Obstacle and PhysMat_Metal are left out: the
	// enemies' navigation is the work list's item 7, and nothing reads the surface.)
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(SkeletalMesh, PlatformBone);
	Box->SetRelativeRotation(LiftBoxRotation);
	Box->SetRelativeScale3D(LiftBoxScale);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RespondToPawnsOnly(Box, ECR_Block);

	// Overlap Box_GEN_VARIABLE: query only (the default), overlapping pawns; out of the navigation, as it blocks nothing.
	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Overlap Box"));
	OverlapBox->SetupAttachment(SkeletalMesh, PlatformBone);
	OverlapBox->SetRelativeLocation(LiftOverlapBoxLocation);
	OverlapBox->SetRelativeRotation(LiftOverlapBoxRotation);
	OverlapBox->SetRelativeScale3D(LiftOverlapBoxScale);
	RespondToPawnsOnly(OverlapBox, ECR_Overlap);
	OverlapBox->SetCanEverAffectNavigation(false);

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(DefaultSceneRoot);
	Audio->bAutoActivate = false;

	Audio1 = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio1"));
	Audio1->SetupAttachment(DefaultSceneRoot);
	Audio1->bAutoActivate = false;

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/DD/Meshes/06_Hospital/hospital_garage_lift_anim")));
	UpSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_GarageLift_Up")));
	DownSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_GarageLift_Down")));
	SoundAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
}

void AWasamiGarageLift::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();
	// Before the boxes register on joint4 (a spawned actor registers its components before OnConstruction).
	SkeletalMesh->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
}

void AWasamiGarageLift::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SkeletalMesh->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
}

void AWasamiGarageLift::BeginPlay()
{
	Super::BeginPlay();
	USoundAttenuation* Attenuation = SoundAttenuation.LoadSynchronous();
	Audio->SetSound(UpSound.LoadSynchronous());
	Audio->AttenuationSettings = Attenuation;
	Audio1->SetSound(DownSound.LoadSynchronous());
	Audio1->AttenuationSettings = Attenuation;
}

bool AWasamiGarageLift::IsPlayerOverlapping() const
{
	return OverlapBox->IsOverlappingActor(UGameplayStatics::GetPlayerCharacter(this, 0));
}

bool AWasamiGarageLiftZone1Special::IsPlayerOverlapping() const
{
	return !bNurseNear && Super::IsPlayerOverlapping();
}

void FWasamiGarageLiftAnimInstanceProxy::PreEvaluateAnimation(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::PreEvaluateAnimation(InAnimInstance);

	const UWasamiGarageLiftAnimInstance* Instance = CastChecked<UWasamiGarageLiftAnimInstance>(InAnimInstance);
	Sequence = Instance->Sequence;
	PlayerOnTime = Instance->PlayerOnTime;
	PlayerOnWeight = Instance->StateBlend.WeightB;
}

bool FWasamiGarageLiftAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	// An animation without a skeleton is skipped, as the engine's sequence player does.
	if (!Sequence || !Sequence->GetSkeleton())
	{
		Output.ResetToRefPose();
		return true;
	}
	auto Extract = [this](float Time, bool bLoop, FPoseContext& Pose)
	{
		FAnimationPoseData PoseData(Pose);
		Sequence->GetAnimationPose(PoseData, FAnimExtractContext(static_cast<double>(Time), false, FDeltaTimeRecord(), bLoop));
	};

	// Default: the first frame (play rate 0, looping). PlayerOn: its time (not looping).
	if (PlayerOnWeight <= 0.f)
	{
		Extract(0.f, true, Output);
		return true;
	}
	if (PlayerOnWeight >= 1.f)
	{
		Extract(PlayerOnTime, false, Output);
		return true;
	}
	FPoseContext DefaultPose(Output);
	FPoseContext PlayerOnPose(Output);
	Extract(0.f, true, DefaultPose);
	Extract(PlayerOnTime, false, PlayerOnPose);
	FAnimationPoseData OutPoseData(Output);
	FAnimationRuntime::BlendTwoPosesTogether(FAnimationPoseData(DefaultPose), FAnimationPoseData(PlayerOnPose), 1.f - PlayerOnWeight, OutPoseData);
	return true;
}

float UWasamiGarageLiftAnimInstance::GetLength() const
{
	return Sequence ? static_cast<float>(Sequence->GetPlayLength()) : 0.f;
}

void UWasamiGarageLiftAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Sequence = nullptr;
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		const FSoftObjectPath Path = WasamiAssets::Path(LiftAnimPackage);
		Sequence = Cast<UAnimSequence>(Path.TryLoad());
		if (!Sequence)
		{
			UE_LOG(LogWasamiGarageLift, Warning, TEXT("No animation %s (WasamiDDTools.import_dd_gimmicks makes it)"), *Path.ToString());
		}
	}
	// The state machine starts in Default.
	StateBlend = FWasamiStateBlend();
	PlayerOnTime = 0.f;
	bPlayerOn = false;
	bUpdated = false;
}

void UWasamiGarageLiftAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// The event graph: PlayerOn? = the owner's Player Overlapping? (the owner cast to BP_06_GarageLift).
	const AWasamiGarageLift* Lift = Cast<AWasamiGarageLift>(GetOwningActor());
	bPlayerOn = Lift && Lift->IsPlayerOverlapping();

	// New State Machine: Default → PlayerOn on PlayerOn?, PlayerOn → Default on its negation.
	const bool bWasInPlayerOn = StateBlend.bInB;
	if (bUpdated && bPlayerOn != bWasInPlayerOn)
	{
		const bool bAlreadyActive = (bPlayerOn ? StateBlend.WeightB : 1.f - StateBlend.WeightB) > 0.f;
		StateBlend.Enter(bPlayerOn, CrossfadeDuration, EAlphaBlendOption::HermiteCubic);
		if (bPlayerOn && !bAlreadyActive)
		{
			PlayerOnTime = 0.f;
		}
	}
	bUpdated = true;
	StateBlend.Advance(DeltaSeconds);
	// PlayerOn's sequence player (play rate 1, not looping) runs while its state has weight; Default's stays on 0.
	if (StateBlend.WeightB > 0.f)
	{
		PlayerOnTime = FMath::Min(PlayerOnTime + DeltaSeconds, GetLength());
	}

	// PlayerOn's start and end notifies.
	if (StateBlend.bInB != bWasInPlayerOn)
	{
		if (StateBlend.bInB)
		{
			PlayerOnEvent();
		}
		else
		{
			PlayerOffEvent();
		}
	}
}

void UWasamiGarageLiftAnimInstance::PlayerOnEvent()
{
	if (const AWasamiGarageLift* Lift = Cast<AWasamiGarageLift>(GetOwningActor()))
	{
		Lift->GetAudio()->Play(0.f);
	}
}

void UWasamiGarageLiftAnimInstance::PlayerOffEvent()
{
	if (const AWasamiGarageLift* Lift = Cast<AWasamiGarageLift>(GetOwningActor()))
	{
		Lift->GetAudio()->FadeOut(UpFadeOutSeconds, 0.f);
		Lift->GetAudio1()->Play(0.f);
	}
}

FAnimInstanceProxy* UWasamiGarageLiftAnimInstance::CreateAnimInstanceProxy()
{
	return new FWasamiGarageLiftAnimInstanceProxy(this);
}
