#include "WasamiEnemy.h"

#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiCapture.h"
#include "WasamiEnemyAnimInstance.h"
#include "WasamiGameMode.h"
#include "WasamiVoice.h"
#include "WasamiZoneFlow.h"

namespace
{
	const FName EnemyTag(TEXT("Enemy"));

	// BP_06_ReaperNurse's StaticMesh (pak_reference_2), on CharacterMesh0: the map's mark.
	const FVector NurseMarkLocation(-0.00019073486328125, 21.884078979492188, 1117.843994140625);
	constexpr double NurseMarkYaw = -0.00011611320951487869;
	const FVector NurseMarkScale(2.5238659381866455, 2.5238659381866455, 10.);
}

void FWasamiChaseVariations::Init(int32 Seed)
{
	*this = FWasamiChaseVariations();
	Random.Initialize(Seed);
}

int32 FWasamiChaseVariations::Advance(bool bChasing, float Seconds)
{
	if (!bChasing)
	{
		// The next chase starts over: a whole gap before its first chance, which may draw any of the six.
		Wait = NotChasing;
		Last = INDEX_NONE;
		return INDEX_NONE;
	}
	if (Wait < 0.f)
	{
		Wait = DrawGap();
		return INDEX_NONE;
	}
	Wait = FMath::Max(Wait - Seconds, 0.f);
	return Wait > 0.f ? INDEX_NONE : DrawClip();
}

void FWasamiChaseVariations::Played(int32 Clip)
{
	Last = Clip;
	Wait = DrawGap();
}

int32 FWasamiChaseVariations::DrawClip()
{
	// Each of the six is as likely as the rest, but never the one played before: the draw goes among the other five and
	// the clip steps over it.
	const bool bAvoidLast = Last != INDEX_NONE;
	int32 Clip = WasamiEnemyAnim::FirstChaseVariation
		+ Random.RandRange(0, WasamiEnemyAnim::NumChaseVariations - (bAvoidLast ? 2 : 1));
	if (bAvoidLast && Clip >= Last)
	{
		++Clip;
	}
	return Clip;
}

void FWasamiEnemyIdleVoices::Init(int32 Seed)
{
	*this = FWasamiEnemyIdleVoices();
	Random.Initialize(Seed);
}

float FWasamiEnemyIdleVoices::DrawGap()
{
	return Random.FRandRange(MinGap, MaxGap);
}

EWasamiVoice FWasamiEnemyIdleVoices::DrawVoice()
{
	return static_cast<EWasamiVoice>(First + Random.RandRange(0, Num - 1));
}

AWasamiEnemy::AWasamiEnemy()
{
	Tags.Add(EnemyTag);
	// The nurse's ReceiveTick, which is only its Update Skate Sound here.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	// CollisionCylinder: the nurse's half height and ACharacter's radius (the nurse does not change it).
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = MaxSpeed;
	Movement->RotationRate = FRotator(0., TurnRate, 0.);
	Movement->bUseControllerDesiredRotation = true;
	Movement->bOrientRotationToMovement = true;

	// CharacterMesh0: the nurse's place and turn (its mesh faces +Y, as SK_WasamiEnemy does), Wasami grown to the
	// nurse's height, and its animation.
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(MeshX, MeshY, MeshZ), FRotator(0., MeshYaw, 0.));
	Body->SetRelativeScale3D(FVector(MeshScale));
	Body->AnimClass = UWasamiEnemyAnimInstance::StaticClass();

	// Sphere: at the capsule's centre, Custom, overlapping Pawn and ignoring the rest (its AreaClass, NavArea_Obstacle,
	// is the shape's default).
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(GetCapsuleComponent());
	Sphere->InitSphereRadius(SphereRadius);
	Sphere->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AWasamiEnemy::OnSphereBeginOverlap);

	// StaticMesh: where the nurse's unscaled mesh puts it, 10 m over the capsule's centre. The original leaves it a static
	// mesh's BlockAllDynamic; as the map's arrow's (AWasamiArrowPointer), it has no collision here, so that it stops
	// nothing on Zone 2's upper floor.
	const FTransform MarkOnCapsule = FTransform(FRotator(0., NurseMarkYaw, 0.), NurseMarkLocation, NurseMarkScale)
		* FTransform(FRotator(0., MeshYaw, 0.), FVector(MeshX, MeshY, MeshZ));
	MapMark = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	MapMark->SetupAttachment(GetCapsuleComponent());
	MapMark->SetRelativeTransform(MarkOnCapsule);
	MapMark->SetCastShadow(false);
	MapMark->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	MapMark->SetCanEverAffectNavigation(false);

	// Skate Audio: on the capsule, no transform of its own, and playing from the start at no volume — Update Skate Sound
	// raises it with the speed.
	SkateAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("Skate Audio"));
	SkateAudio->SetupAttachment(GetCapsuleComponent());
	SkateAudio->SetVolumeMultiplier(MoveVolume);
	SkateAudio->SetPitchMultiplier(MoveMinPitch);

	// Talk Audio: on the capsule as well and with nothing to say yet, so it waits for Talk rather than starting itself.
	// The waves' own subtitles would go only while the clip sounds — under a second — so they are suppressed here and
	// WasamiVoice puts them up for the time the WebGL version gave them.
	TalkAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("Talk Audio"));
	TalkAudio->SetupAttachment(GetCapsuleComponent());
	TalkAudio->bAutoActivate = false;
	TalkAudio->bSuppressSubtitles = true;

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy")));
	MoveSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/06_Hospital/DD_Rollerskating_Fast_V1_LOOP")));
	MoveAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/MonkeyAttenuation")));
	TalkAttenuation = TSoftObjectPtr<USoundAttenuation>(WasamiAssets::Path(TEXT("/Game/DD/Audio/Misc/AgathaAttenuation")));
	MapMarkMesh = TSoftObjectPtr<UStaticMesh>(WasamiAssets::Path(TEXT("/Engine/BasicShapes/Plane")));
	MapMarkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Shared/M_Enemy")));
}

AWasamiEnemy* AWasamiEnemy::SpawnEnemy(const UObject* WorldContextObject, FVector Location, float Yaw, bool bSentry)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0., Yaw, 0.), Location);
	AWasamiEnemy* Enemy = World->SpawnActorDeferred<AWasamiEnemy>(StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Enemy)
	{
		Enemy->bCanSpawn = true;
		Enemy->bAggressiveIdle = bSentry;
		Enemy->FinishSpawning(Transform);
	}
	return Enemy;
}

void AWasamiEnemy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GetMesh()->SetSkeletalMeshAsset(MeshAsset.LoadSynchronous());
	MapMark->SetStaticMesh(MapMarkMesh.LoadSynchronous());
	MapMark->SetMaterial(0, MapMarkMaterial.LoadSynchronous());
}

void AWasamiEnemy::BeginPlay()
{
	Super::BeginPlay();
	SkateAudio->AttenuationSettings = MoveAttenuation.LoadSynchronous();
	SkateAudio->SetSound(MoveSound.LoadSynchronous());
	// The original's Skate Audio holds the loop itself, so bAutoActivate starts it; ours loads the loop here, after
	// that activation has already found no sound, and has to start it by hand.
	SkateAudio->Play();
	TalkAudio->AttenuationSettings = TalkAttenuation.LoadSynchronous();
	BeginNurse();
}

void AWasamiEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSkateSound(DeltaSeconds);
}

void AWasamiEnemy::UpdateSkateSound(float DeltaSeconds)
{
	const float Speed = GetVelocity().Size();
	SkateAudio->SetVolumeMultiplier(FMath::FInterpTo(SkateAudio->VolumeMultiplier,
		FMath::GetMappedRangeValueClamped(FVector2f(0.f, MoveVolumeSpeed), FVector2f(0.f, 1.f), Speed),
		DeltaSeconds, MoveVolumeInterp));
	SkateAudio->SetPitchMultiplier(FMath::FInterpTo(SkateAudio->PitchMultiplier,
		FMath::GetMappedRangeValueClamped(FVector2f(MoveVolumeSpeed, MovePitchSpeed), FVector2f(MoveMinPitch, MoveMaxPitch), Speed),
		DeltaSeconds, MovePitchInterp));
}

void AWasamiEnemy::BeginNurse()
{
	// BP_DD_Character_Base: an enemy the level did not spawn with CanSpawn is removed.
	if (!bCanSpawn)
	{
		Destroy();
		return;
	}
	// The base then has the capsule ignore every speed barrier (Ignore All Speed Barriers): the item 8's barriers.

	// BP_06_ReaperNurse: Generate Random Point, then Make Choice every half second.
	GenerateRandomPoint();
	ChaseVariations.Init(FMath::Rand());
	GetWorldTimerManager().SetTimer(DecisionTimer, this, &AWasamiEnemy::MakeChoice, DecisionInterval, true);
	// Then Random Dialogue, which runs on its own from here (the original starts it in the same BeginPlay, after the
	// game mode; the sentry, whose BeginPlay is empty, has no call until it drops on the player).
	IdleVoices.Init(FMath::Rand());
	ScheduleIdleVoice();
}

void AWasamiEnemy::ScheduleIdleVoice()
{
	// Random Dialogue's Delay(RandomFloatInRange), drawn anew each time round.
	GetWorldTimerManager().SetTimer(IdleVoiceTimer, this, &AWasamiEnemy::SayIdleVoice, IdleVoices.DrawGap(), false);
}

void AWasamiEnemy::SayIdleVoice()
{
	// The original picks Zone01_Pursuit through a chase and Zone01_Laugh while cloaked; this game has only the four
	// rounds voices, so a chase says nothing here (its found line is Chase Player's) and a stun speaks as the rounds do.
	if (!IsChasing() || IsStunned())
	{
		// Not forced: a line still sounding is left to finish, and this call is simply lost, as the original's is.
		Talk(IdleVoices.DrawVoice(), false, IdleVolume);
	}
	ScheduleIdleVoice();
}

float AWasamiEnemy::GetIdleVoiceWait() const
{
	return FMath::Max(GetWorldTimerManager().GetTimerRemaining(IdleVoiceTimer), 0.f);
}

UAudioComponent* AWasamiEnemy::Talk(EWasamiVoice Id, bool bForce, float Volume)
{
	// Talk (@10483): without bForce a line that is still sounding keeps this one from starting at all; with it the
	// sound is set and played over whatever was there.
	if (!bForce && TalkAudio->IsPlaying())
	{
		return nullptr;
	}
	USoundBase* Sound = WasamiVoice::Load(Id);
	if (!Sound)
	{
		return nullptr;
	}
	TalkAudio->SetVolumeMultiplier(Volume);
	TalkAudio->SetSound(Sound);
	TalkAudio->Play();
	// The component says nothing itself (bSuppressSubtitles): the line goes up for as long as it can be read.
	WasamiVoice::ShowSubtitle(this, Id);
	return TalkAudio;
}

void AWasamiEnemy::SetWalkState(bool bNormal)
{
	bNormalWalk = bNormal;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bNormalWalk ? NormalSpeed : SkateSpeed;
	}
}

float AWasamiEnemy::GetStunTimeLeft() const
{
	if (!IsStunned())
	{
		return 0.f;
	}
	if (bStunRunning)
	{
		return GetWorldTimerManager().GetTimerRemaining(StunTimer);
	}
	return GetTimeToStunStart() + StunSeconds;
}

float AWasamiEnemy::GetTimeToStunStart() const
{
	return FMath::Max(GetWorldTimerManager().GetTimerRemaining(DecisionTimer), 0.f);
}

UWasamiEnemyAnimInstance* AWasamiEnemy::GetEnemyAnim() const
{
	return Cast<UWasamiEnemyAnimInstance>(GetMesh()->GetAnimInstance());
}

void AWasamiEnemy::SetState_Implementation(EWasamiEnemyState NewState, bool bByOrb)
{
	// The nurse keeps the state only; Primal Fear and the orb stun alike.
	State = NewState;
}

void AWasamiEnemy::MakeChoice()
{
	if (State == EWasamiEnemyState::Stun)
	{
		StartStun();
	}
	// The pill throw (bThrowing) is not made. A sequence: first what it saw before, then what it sees now; its last
	// step, the invisibility past 1500 cm from the player (Cloak), is not made either.
	else
	{
		if (bSeenPlayerRecently)
		{
			ChasePlayer();
			// RetriggerableDelay(3): each chase starts it over, so the player is forgotten only when the decisions stop
			// chasing (a stun) or Player Vanish clears Seen Player Recently.
			GetWorldTimerManager().SetTimer(ForgetTimer, this, &AWasamiEnemy::ForgetPlayer, ForgetSeconds, false);
		}
		else
		{
			NotSeeingPlayer();
		}
		if (CanSeePlayer())
		{
			bSeenPlayerRecently = true;
		}
	}
	// Not the nurse's: this game's chase variations (the item 26), which every decision moves on.
	UpdateChaseVariation();
}

void AWasamiEnemy::UpdateChaseVariation()
{
	const int32 Clip = ChaseVariations.Advance(IsChasing() && State != EWasamiEnemyState::Stun, DecisionInterval);
	if (Clip == INDEX_NONE)
	{
		return;
	}
	UWasamiEnemyAnimInstance* Anim = GetEnemyAnim();
	// A chance is let go when there is nothing to play it with, when something else plays once (the parking lot's stab
	// at the doors, or a variation still running) or when this enemy may not.
	if (!Anim || Anim->IsPlayingOnce() || !CanPlayChaseVariation())
	{
		ChaseVariations.Skip();
		return;
	}
	// The chase's speed, which the variation does not change (the user's instruction of 2026-09-18), and the rate that
	// keeps the clip's feet with it. The clips are in-place forms, so the body moves this far while it plays.
	const float Speed = GetCharacterMovement()->MaxWalkSpeed;
	const float Rate = WasamiEnemyAnim::ChaseVariationRate(Clip, Speed);
	const float Seconds = Rate > 0.f ? Anim->GetAnimState().GetLength(Clip) / Rate : 0.f;
	if (Seconds <= 0.f)
	{
		// The clip has not loaded (the editor's level): nothing to play.
		ChaseVariations.Skip();
		return;
	}
	if (!IsWayAheadClear(Speed * Seconds))
	{
		// The chance stays open: the next decision draws again, and one of them plays as soon as the way clears.
		return;
	}
	Anim->PlayOnce(FName(WasamiEnemyAnim::ClipNames[Clip]), Rate);
	ChaseVariations.Played(Clip);
}

bool AWasamiEnemy::IsWayAheadClear(float Distance) const
{
	// A NavMesh ray from where it stands (the nav agent's own place) straight ahead: true only when nothing blocks it,
	// so that a variation never carries the body into a wall. The engine's ray is blocked by default.
	const FVector Start = GetNavAgentLocation();
	FVector Hit;
	return !UNavigationSystemV1::NavigationRaycast(GetWorld(), Start, Start + GetActorForwardVector() * Distance, Hit,
		nullptr, GetController());
}

void AWasamiEnemy::StartStun()
{
	// A DoOnce, opened again when the stun ends. Stopping the movement also aborts the AI's path (the nav movement's
	// StopActiveMovement), and no decision asks for another until the stun ends. Cloak(False) (the nurse's invisibility,
	// not made) is left out, and so is its stunned line, Zone01_Stunned, which this game has no voice for: a stun goes on
	// saying the rounds voices (Say Idle Voice).
	if (!bStunRunning)
	{
		bStunRunning = true;
		GetCharacterMovement()->StopMovementImmediately();
		GetWorldTimerManager().SetTimer(StunTimer, this, &AWasamiEnemy::EndStun, StunSeconds, false);
	}
}

void AWasamiEnemy::EndStun()
{
	State = EWasamiEnemyState::Patrol;
	bStunRunning = false;
}

void AWasamiEnemy::ForgetPlayer()
{
	bSeenPlayerRecently = false;
	ResetDetection();
}

AActor* AWasamiEnemy::GetPlayerTarget() const
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

bool AWasamiEnemy::CanSeePlayer() const
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return false;
	}
	const FVector From = GetActorLocation();
	const FVector To = Player->GetActorLocation();
	const FVector Toward = UKismetMathLibrary::Normal(To - From, 0.0001f);
	if (!(UKismetMathLibrary::DegAcos(FVector::DotProduct(Toward, GetActorForwardVector())) < ViewAngle))
	{
		return false;
	}
	// LineTraceSingle on the Camera channel, complex, ignoring itself: Vanish has the player's capsule ignore Camera.
	FHitResult Hit;
	UKismetSystemLibrary::LineTraceSingle(this, From, To, UEngineTypes::ConvertToTraceType(ECC_Camera), true,
		TArray<AActor*>(), EDrawDebugTrace::None, Hit, true);
	const ACharacter* Seen = Cast<ACharacter>(Hit.GetActor());
	return Seen && Seen == Player;
}

void AWasamiEnemy::ChasePlayer()
{
	bSeenPlayerRecently = true;
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	PointOfInterest = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	SetWalkState(false);
	// AI MoveTo the target; its success and failure do nothing.
	UAIBlueprintHelperLibrary::CreateMoveToProxyObject(this, this, FVector::ZeroVector, GetPlayerTarget(), ChaseAcceptance, false);
	// A DoOnce that Reset Detection opens: the Detected line and CloseBy. The pill throw 5 s after the first chase is
	// not made.
	if (!bDetectionClosed)
	{
		bDetectionClosed = true;
		// Talk(Zone01_Detected, True) as this game's Wasami, forced over a rounds voice. The original leaves every nurse
		// to say it; here the game mode holds them to one between them (どの敵からでも 12 s に 1 回まで), and where there is
		// no game mode of this game's (a test world's own) it is simply said.
		AWasamiGameMode* Mode = Cast<AWasamiGameMode>(UGameplayStatics::GetGameMode(this));
		if (!Mode || Mode->TakeFoundVoice())
		{
			Talk(EWasamiVoice::Found, true, FoundVolume);
		}
		OnCloseBy.Broadcast();
	}
}

void AWasamiEnemy::NotSeeingPlayer()
{
	if (PointOfInterest.Equals(FVector::ZeroVector, 0.0001))
	{
		if (UAIAsyncTaskBlueprintProxy* Move = UAIBlueprintHelperLibrary::CreateMoveToProxyObject(this, this,
			GetRandomPointDestination(), nullptr, RandomPointAcceptance, false))
		{
			Move->OnSuccess.AddDynamic(this, &AWasamiEnemy::OnRandomPointMoveEnded);
			Move->OnFail.AddDynamic(this, &AWasamiEnemy::OnRandomPointMoveEnded);
		}
	}
	else if (UAIAsyncTaskBlueprintProxy* Move = UAIBlueprintHelperLibrary::CreateMoveToProxyObject(this, this,
		PointOfInterest, nullptr, PointOfInterestAcceptance, false))
	{
		Move->OnSuccess.AddDynamic(this, &AWasamiEnemy::OnPointOfInterestMoveEnded);
		Move->OnFail.AddDynamic(this, &AWasamiEnemy::OnPointOfInterestMoveEnded);
	}
	SetWalkState(true);
}

void AWasamiEnemy::GenerateRandomPoint()
{
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	const FVector Origin = Player ? Player->GetActorLocation() : GetActorLocation();
	// Without navigation data the point is left as it was.
	UNavigationSystemV1::K2_GetRandomReachablePointInRadius(this, Origin, RandomPoint, RandomPointRadius);
}

void AWasamiEnemy::OnRandomPointMoveEnded(EPathFollowingResult::Type MovementResult)
{
	GenerateRandomPoint();
}

void AWasamiEnemy::OnPointOfInterestMoveEnded(EPathFollowingResult::Type MovementResult)
{
	PointOfInterest = FVector::ZeroVector;
}

void AWasamiEnemy::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// BndEvt__Sphere (@9818): the player, and State not Stun (Vanish does not matter), then a DoOnce.
	if (bCatchClosed || !OtherActor || OtherActor != UGameplayStatics::GetPlayerCharacter(this, 0)
		|| State == EWasamiEnemyState::Stun)
	{
		return;
	}
	bCatchClosed = true;
	// The nurse removes every other enemy, Force Removes the stun hits (BP_04_StunHit: the gas's, which is not made)
	// and runs its Jumpscare Handle. The capture's room has a Wasami of its own, so this one is removed with the rest;
	// the capture is started first, while this one is still there to find the world by (the game mode's death being
	// closed stops it, not the removal).
	AWasamiCapture::StartCapture(this, this);
	AWasamiZoneFlow::RemoveAllEnemies(GetWorld());
}
