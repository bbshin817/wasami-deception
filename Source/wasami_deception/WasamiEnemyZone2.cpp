#include "WasamiEnemyZone2.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiLift.h"

bool AWasamiEnemyZone2::IsUp() const
{
	return GetActorLocation().Z > UpperFloorZ;
}

bool AWasamiEnemyZone2::IsPlayerUp() const
{
	// Without a player the original reads a zero location.
	const ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	return (Player ? Player->GetActorLocation().Z : 0.) > PlayerUpperFloorZ;
}

AWasamiLift* AWasamiEnemyZone2::GetClosestLift(FVector& OutMoveLocation) const
{
	// GetAllActorsOfClass(BP_06_LiftBase) and GetFurthestOrClosestActor(From (0, 0, 0), UseClosest): a lift at a
	// squared distance not over the best so far becomes the best.
	AWasamiLift* Best = nullptr;
	double BestDistance = ClosestLiftStart;
	for (TActorIterator<AWasamiLift> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::DistSquared(It->GetActorLocation(), FVector::ZeroVector);
		if (!(Distance > BestDistance))
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	const USceneComponent* MoveLocation = Best ? Best->GetMoveLocation() : nullptr;
	OutMoveLocation = MoveLocation ? MoveLocation->GetComponentLocation() : FVector::ZeroVector;
	return Best;
}

AActor* AWasamiEnemyZone2::GetPlayerTarget() const
{
	if (IsSameLevelAsPlayer())
	{
		return Super::GetPlayerTarget();
	}
	FVector MoveLocation;
	return GetClosestLift(MoveLocation);
}

FVector AWasamiEnemyZone2::GetRandomPointDestination() const
{
	if (IsSameLevelAsPlayer())
	{
		return Super::GetRandomPointDestination();
	}
	FVector MoveLocation;
	GetClosestLift(MoveLocation);
	return MoveLocation;
}
