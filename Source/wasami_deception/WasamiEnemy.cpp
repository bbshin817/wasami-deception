#include "WasamiEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
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

	// CharacterMesh0: the nurse's place and turn (its mesh faces +Y, as SK_WasamiEnemy does) and its animation.
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocationAndRotation(FVector(MeshX, MeshY, MeshZ), FRotator(0., MeshYaw, 0.));
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

	// BP_06_ReaperNurse: Generate Random Point (the AI's), then Make Choice every half second.
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
		// A DoOnce, opened again when the stun ends. Cloak(False) (the nurse's invisibility, not made) and its stunned
		// line (Talk, the item 20's voices) are left out.
		if (!bStunRunning)
		{
			bStunRunning = true;
			GetCharacterMovement()->StopMovementImmediately();
			GetWorldTimerManager().SetTimer(StunTimer, this, &AWasamiEnemy::EndStun, StunSeconds, false);
		}
		return;
	}
	// The nurse's pill throw is not made; its chase (Chase Player / Not Seeing Player) is the item 7's AI.
}

void AWasamiEnemy::EndStun()
{
	State = EWasamiEnemyState::Patrol;
	bStunRunning = false;
}
