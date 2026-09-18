#include "WasamiGameInstance.h"

#include "Kismet/KismetMathLibrary.h"

void UWasamiGameInstance::DecrementLives()
{
	Lives = FMath::Clamp(Lives - 1, 0, MaxLives);
}

void UWasamiGameInstance::IncrementLives()
{
	Lives = FMath::Clamp(Lives + 1, 0, MaxLives);
}

void UWasamiGameInstance::ResetLives()
{
	Lives = StartingLives;
}

void UWasamiGameInstance::RememberCollectedShard(const FVector& StartLocation)
{
	ShardsToBeRemoved.AddUnique(ShardKey(StartLocation));
}

void UWasamiGameInstance::ForgetCollectedShards()
{
	ShardsToBeRemoved.Reset();
}

FVector UWasamiGameInstance::ShardKey(const FVector& Location)
{
	const FIntVector Truncated = UKismetMathLibrary::FTruncVector(Location);
	return FVector(Truncated.X, Truncated.Y, Truncated.Z);
}
