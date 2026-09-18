#include "WasamiLift.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiPlayerCharacter.h"

namespace
{
	// BP_06_LiftBase (pak_reference_2): the SCS templates. The boxes keep UBoxComponent's 32 cm extent unless given one
	// and are sized by their scale; LiftCollisionOverlap and BottomCollision hang under LiftCollision, so its scale sizes
	// them too.
	const FVector LiftCollisionScale(4.552351474761963, 4.586122035980225, 1.1901235580444336);
	const FVector LiftOverlapLocation(0., 0., 63.51140213012695);
	const FVector LiftBottomExtent(32., 32., 256.75);
	const FVector LiftBottomLocation(0., 0., -288.75);
	const FVector LiftMoveLocation(0., 0., 149.53536987304688);
	// BP_06_Lift's and BP_06_LiftBase_Corner's LiftCollision1.
	const FVector LiftTopCollisionScale(4.552350997924805, 4.586122035980225, 1.1901240348815918);

	// MovementAudio_GEN_VARIABLE's volume and pitch; FadeIn (0.5 s to 1, from the start) and FadeOut (0.5 s to 0).
	constexpr float LiftMovementVolume = 0.7f;
	constexpr float LiftMovementPitch = 1.5f;
	constexpr float LiftFadeSeconds = 0.5f;

	UBoxComponent* MakeBlockingBox(AActor* Owner, const TCHAR* Name)
	{
		UBoxComponent* Box = Owner->CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		return Box;
	}
}

AWasamiLiftBase::AWasamiLiftBase()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	LiftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiftMesh"));
	LiftMesh->SetupAttachment(DefaultSceneRoot);
	LiftMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	LiftCollision = MakeBlockingBox(this, TEXT("LiftCollision"));
	LiftCollision->SetupAttachment(LiftMesh);
	LiftCollision->SetRelativeScale3D(LiftCollisionScale);

	// UBoxComponent's defaults (OverlapAllDynamic), out of the navigation (its AreaClass, NavArea_Obstacle, does nothing
	// for a box that blocks nothing).
	LiftCollisionOverlap = CreateDefaultSubobject<UBoxComponent>(TEXT("LiftCollisionOverlap"));
	LiftCollisionOverlap->SetupAttachment(LiftCollision);
	LiftCollisionOverlap->SetRelativeLocation(LiftOverlapLocation);
	LiftCollisionOverlap->SetCanEverAffectNavigation(false);

	BottomCollision = MakeBlockingBox(this, TEXT("BottomCollision"));
	BottomCollision->SetupAttachment(LiftCollision);
	BottomCollision->SetBoxExtent(LiftBottomExtent, false);
	BottomCollision->SetRelativeLocation(LiftBottomLocation);

	MoveLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MoveLocation"));
	MoveLocation->SetupAttachment(LiftMesh);
	MoveLocation->SetRelativeLocation(LiftMoveLocation);

	LiftCollision1 = MakeBlockingBox(this, TEXT("LiftCollision1"));
	LiftCollision1->SetupAttachment(DefaultSceneRoot);
	LiftCollision1->SetRelativeScale3D(LiftTopCollisionScale);
	LiftCollision1->SetRelativeLocation(FVector(0., 0., TopLocation));

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(DefaultSceneRoot);
	Audio->bAutoActivate = false;

	MovementAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementAudio"));
	MovementAudio->SetupAttachment(DefaultSceneRoot);
	MovementAudio->bAutoActivate = false;
	MovementAudio->SetVolumeMultiplier(LiftMovementVolume);
	MovementAudio->SetPitchMultiplier(LiftMovementPitch);

	ClunkSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_GarageLift_Down")));
	ClunkAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/MonkeyAttenuation")));
	MovementSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_TT_Lift_Loop")));
	MovementAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation")));
}

void AWasamiLiftBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The construction script: LiftCollision1 at Top Location. (Preview Top, which shows the floor at the top in the
	// editor, is left out.)
	LiftCollision1->SetRelativeLocation(FVector(0., 0., TopLocation));
}

void AWasamiLiftBase::BeginPlay()
{
	Super::BeginPlay();
	Audio->SetSound(ClunkSound.LoadSynchronous());
	Audio->AttenuationSettings = ClunkAttenuation.LoadSynchronous();
	MovementAudio->SetSound(MovementSound.LoadSynchronous());
	MovementAudio->AttenuationSettings = MovementAttenuation.LoadSynchronous();
}

bool AWasamiLiftBase::IsCharacterOnTop() const
{
	TArray<AActor*> Characters;
	LiftCollisionOverlap->GetOverlappingActors(Characters, ACharacter::StaticClass());
	return Characters.Num() > 0;
}

float AWasamiLiftBase::GetHeight() const
{
	return static_cast<float>(LiftMesh->GetRelativeLocation().Z);
}

float AWasamiLiftBase::StepFrom(float Height, float DeltaSeconds) const
{
	const float Speed = TopLocation * (IsCharacterOnTop() ? SpeedWithCharacter : SpeedWithoutCharacter);
	return FMath::FInterpConstantTo(Height, GetTargetHeight(), DeltaSeconds, Speed);
}

void AWasamiLiftBase::UpdatePosition(float DeltaSeconds)
{
	const float Height = StepFrom(GetHeight(), DeltaSeconds);
	LiftMesh->SetRelativeLocation(FVector(0., 0., Height));
	// IsMoving? = !(FInterpTo_Constant(...) == target): the Blueprint's pure nodes run again for it, from where the floor
	// now is (and with the overlaps the move has just updated), so the floor counts as stopped a step before it gets there.
	bIsMoving = StepFrom(Height, DeltaSeconds) != GetTargetHeight();
}

void AWasamiLiftBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePosition(UGameplayStatics::GetWorldDeltaSeconds(this));
	// The two DoOnce reset each other, so between them they fire on the changes of IsMoving? (the stopping one starts
	// closed: a floor at rest from the start plays nothing).
	if (bIsMoving && !bMovementSoundOn)
	{
		bMovementSoundOn = true;
		MovementAudio->FadeIn(LiftFadeSeconds, 1.f, 0.f);
		Audio->Play(0.f);
	}
	else if (!bIsMoving && bMovementSoundOn)
	{
		bMovementSoundOn = false;
		MovementAudio->FadeOut(LiftFadeSeconds, 0.f);
		Audio->Play(0.f);
	}
}

void AWasamiLift::BeginPlay()
{
	Super::BeginPlay();
	// BP_06_Lift's ReceiveBeginPlay.
	LiftCollision1->DestroyComponent();
	LiftCollision1 = nullptr;
	LiftCollisionOverlap->OnComponentBeginOverlap.AddDynamic(this, &AWasamiLift::OnLiftCollisionOverlapBegin);
}

float AWasamiLift::GetTargetHeight() const
{
	return IsCharacterOnTop() ? TopLocation : 0.f;
}

void AWasamiLift::OnLiftCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	NotifyOverlap(OtherActor);
}

void AWasamiLift::NotifyOverlap(AActor* Other)
{
	// Cast to BP_DD_PlayerCharacter.
	if (Cast<AWasamiPlayerCharacter>(Other))
	{
		OnPlayerOverlap.Broadcast();
	}
}

bool AWasamiCornerLift::IsPlayerOnTopFloor() const
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	return IsValid(Player) && Player->GetActorLocation().Z > TopFloorHeight;
}

float AWasamiCornerLift::GetTargetHeight() const
{
	const bool bTop = bPlayerForceMovement ? bGoUp : IsPlayerOnTopFloor();
	return bTop ? TopLocation : 0.f;
}

bool AWasamiCornerLift::IsDoubleCheckPending() const
{
	return GetWorldTimerManager().IsTimerActive(DoubleCheckTimer);
}

void AWasamiCornerLift::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (OtherActor != nullptr && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		// K2_SetTimer('Double Check', 1 s, not looping): a new one replaces the one waiting.
		GetWorldTimerManager().SetTimer(DoubleCheckTimer, this, &AWasamiCornerLift::DoubleCheck, DoubleCheckDelay, false);
	}
}

void AWasamiCornerLift::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	if (OtherActor != nullptr && OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		GetWorldTimerManager().ClearTimer(DoubleCheckTimer);
		bPlayerForceMovement = false;
	}
}

void AWasamiCornerLift::DoubleCheck()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!LiftCollisionOverlap->IsOverlappingActor(Player))
	{
		return;
	}
	bPlayerForceMovement = true;
	bGoUp = !IsPlayerOnTopFloor();
	Audio->Play(0.f);
}

void AWasamiCornerLift::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DoubleCheckTimer);
	Super::EndPlay(EndPlayReason);
}
