#include "WasamiEnemy06Chase.h"

#include "Camera/CameraShakeBase.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "WasamiAssets.h"
#include "WasamiEnemyAnimInstance.h"

const FName AWasamiEnemy06Chase::DoorAttackClip(TEXT("Chase_Charge"));

AWasamiEnemy06Chase::AWasamiEnemy06Chase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Audio on the capsule; the ParticleSystem component under it (Glass_fracture, never activated) only marks where
	// Hit FX puts its dust (HitFXForward).
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(GetCapsuleComponent());
	Audio->bAutoActivate = false;

	DoorHitSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/01_Hotel/20-Elevator_Slams")));
	DoorHitParticle = TSoftObjectPtr<UParticleSystem>(WasamiAssets::Path(TEXT("/Game/DD/Particles/06_Hospital/P_06_NurseDoorHit")));
	DoorHitShakeClass = TSoftClassPtr<UCameraShakeBase>(WasamiAssets::ClassPath(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop")));
}

void AWasamiEnemy06Chase::BeginPlay()
{
	Super::BeginPlay();
	if (!IsActorBeingDestroyed())
	{
		Audio->SetSound(DoorHitSound.LoadSynchronous());
	}
}

void AWasamiEnemy06Chase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsStunned())
	{
		StartStun();
		return;
	}
	ChasePlayer();
	if (bAttackDoor && !bDoorAttackClosed)
	{
		StartDoorAttack();
	}
}

void AWasamiEnemy06Chase::StartDoorAttack()
{
	bDoorAttackClosed = true;
	++DoorAttacks;
	if (UWasamiEnemyAnimInstance* Anim = GetEnemyAnim())
	{
		Anim->PlayOnce(DoorAttackClip);
	}
	GetWorldTimerManager().SetTimer(DoorHitTimer, this, &AWasamiEnemy06Chase::OnDoorAttackNotify, DoorHitTime, false);
	GetWorldTimerManager().SetTimer(DoorAttackTimer, this, &AWasamiEnemy06Chase::OnDoorAttackCompleted, DoorAttackSeconds, false);
}

void AWasamiEnemy06Chase::OnDoorAttackNotify()
{
	if (UKismetMathLibrary::RandomBoolWithWeight(HitFXChance))
	{
		HitFX();
	}
}

void AWasamiEnemy06Chase::OnDoorAttackCompleted()
{
	// Delay(RandomFloatInRange(0, 0.5)), then the DoOnce opens again (a Delay of 0 waits for the next tick).
	const float Wait = FMath::Max(UKismetMathLibrary::RandomFloatInRange(0.f, DoorAttackMaxWait), UE_KINDA_SMALL_NUMBER);
	GetWorldTimerManager().SetTimer(DoorAttackTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bDoorAttackClosed = false;
	}), Wait, false);
}

void AWasamiEnemy06Chase::HitFX()
{
	const FVector At = GetCapsuleComponent()->GetComponentTransform().TransformPosition(FVector(HitFXForward, 0., 0.));
	if (UParticleSystem* Particle = DoorHitParticle.LoadSynchronous())
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, Particle, At, FRotator::ZeroRotator, FVector(HitFXScale), true,
			EPSCPoolMethod::None, true);
	}
	Audio->Play(0.f);
	if (const TSubclassOf<UCameraShakeBase> Shake = DoorHitShakeClass.LoadSynchronous())
	{
		UGameplayStatics::PlayWorldCameraShake(this, Shake, GetActorLocation(), 0.f, HitShakeRadius, 1.f, true);
	}
}
