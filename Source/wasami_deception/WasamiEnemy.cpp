#include "WasamiEnemy.h"

#include "Blueprint/AIAsyncTaskBlueprintProxy.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemyAnimInstance.h"

namespace
{
	const FName EnemyTag(TEXT("Enemy"));
}

AWasamiEnemy::AWasamiEnemy()
{
	Tags.Add(EnemyTag);
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

	MeshAsset = TSoftObjectPtr<USkeletalMesh>(WasamiAssets::Path(TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy")));
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
}

void AWasamiEnemy::BeginPlay()
{
	Super::BeginPlay();

	// BP_DD_Character_Base: an enemy the level did not spawn with CanSpawn is removed.
	if (!bCanSpawn)
	{
		Destroy();
		return;
	}
	// The base then has the capsule ignore every speed barrier (Ignore All Speed Barriers): the item 8's barriers.

	// BP_06_ReaperNurse: Generate Random Point, then Make Choice every half second.
	GenerateRandomPoint();
	GetWorldTimerManager().SetTimer(DecisionTimer, this, &AWasamiEnemy::MakeChoice, DecisionInterval, true);
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
	const float ToDecision = GetWorldTimerManager().GetTimerRemaining(DecisionTimer);
	return FMath::Max(ToDecision, 0.f) + StunSeconds;
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
		// A DoOnce, opened again when the stun ends. Stopping the movement also aborts the AI's path (the nav movement's
		// StopActiveMovement), and no decision asks for another until the stun ends. Cloak(False) (the nurse's
		// invisibility, not made) and its stunned line (Talk, the item 20's voices) are left out.
		if (!bStunRunning)
		{
			bStunRunning = true;
			GetCharacterMovement()->StopMovementImmediately();
			GetWorldTimerManager().SetTimer(StunTimer, this, &AWasamiEnemy::EndStun, StunSeconds, false);
		}
		return;
	}
	// The pill throw (bThrowing) is not made. A sequence: first what it saw before, then what it sees now; its last
	// step, the invisibility past 1500 cm from the player (Cloak), is not made either.
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
	// A DoOnce that Reset Detection opens: the Detected line (Talk, the item 20's voices) and CloseBy. The pill throw 5 s
	// after the first chase is not made.
	if (!bDetectionClosed)
	{
		bDetectionClosed = true;
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
