#include "WasamiTestEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"

namespace
{
	// The hospital nurse's capsule half height, and ACharacter's radius.
	constexpr float CapsuleRadius = 34.f;
	constexpr float CapsuleHalfHeight = 118.058f;
	const FName TestEnemyTag(TEXT("Enemy"));
}

AWasamiTestEnemy::AWasamiTestEnemy()
{
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Capsule->SetHiddenInGame(false);
	RootComponent = Capsule;
	Tags.Add(TestEnemyTag);
}

AWasamiTestEnemy* AWasamiTestEnemy::SpawnTestEnemy(const UObject* WorldContextObject, FVector Location)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AWasamiTestEnemy>(Location, FRotator::ZeroRotator, Params);
}

void AWasamiTestEnemy::SetState_Implementation(EWasamiEnemyState NewState, bool bByOrb)
{
	++SetStateCount;
	State = NewState;
	bLastByOrb = bByOrb;
}
