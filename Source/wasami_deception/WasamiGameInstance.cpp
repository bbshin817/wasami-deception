#include "WasamiGameInstance.h"

#include "Kismet/KismetMathLibrary.h"
#include "WasamiCapture.h"

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

int32 UWasamiGameInstance::TakeCaptureChoice()
{
	return TakeNoRepeat(CaptureChoices, bCaptureChoicesStarted, 0, AWasamiCapture::NumChoices - 1, CaptureStream);
}

int32 UWasamiGameInstance::TakeNoRepeat(TArray<int32>& Remaining, bool& bStarted, int32 Min, int32 Max, const FRandomStream& Stream)
{
	if (Max - Min <= 0)
	{
		// The macro warns that the range is not valid and gives the value it chose last; one number is all there is.
		return Min;
	}
	if (!bStarted)
	{
		for (int32 Value = Min; Value <= Max; ++Value)
		{
			Remaining.Add(Value);
		}
		bStarted = true;
	}
	// Array_Shuffle's swaps, then the last.
	const int32 LastIndex = Remaining.Num() - 1;
	for (int32 Index = 0; Index <= LastIndex; ++Index)
	{
		const int32 Other = Stream.RandRange(Index, LastIndex);
		if (Other != Index)
		{
			Remaining.Swap(Index, Other);
		}
	}
	const int32 Chosen = Remaining.Last();
	if (Remaining.Num() < 2)
	{
		Remaining.Reset();
		bStarted = false;
	}
	else
	{
		Remaining.RemoveAt(LastIndex);
	}
	return Chosen;
}
