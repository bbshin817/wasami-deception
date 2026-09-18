#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WasamiGameInstance.generated.h"

/**
 * What outlives reopening a level and is never written to disk, after Dark Deception's BP_DD_GameInstance
 * (pak_reference_2): the lives, and the shards collected since the last fresh start (Shards To Be Removed), which the
 * game mode takes out of the level each time it opens again. Lives go through BP_DD_Functions' Get Lives, Decrement
 * Lives, Increment Lives and Reset Lives.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Lives when the game starts and after Reset Lives: the original's 3 for a player level of 0 to 4 (this game has
	 * no player levels). */
	static constexpr int32 StartingLives = 3;

	/** Decrement Lives and Increment Lives clamp to 0..6. */
	static constexpr int32 MaxLives = 6;

	UFUNCTION(BlueprintPure, Category = "Game")
	int32 GetLives() const { return Lives; }

	UFUNCTION(BlueprintCallable, Category = "Game")
	void DecrementLives();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void IncrementLives();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResetLives();

	/** Collect's AddUnique: remembers a shard by where it began play, truncated to whole centimetres. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void RememberCollectedShard(const FVector& StartLocation);

	/** Empties Shards To Be Removed (RESTART, QUIT TO TITLE, the loading screen to the next level). */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ForgetCollectedShards();

	const TArray<FVector>& GetShardsToBeRemoved() const { return ShardsToBeRemoved; }

	/** FTruncVector: each component truncated toward zero, the key a shard is remembered by. */
	static FVector ShardKey(const FVector& Location);

private:
	UPROPERTY(VisibleAnywhere, Category = "Game")
	int32 Lives = StartingLives;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	TArray<FVector> ShardsToBeRemoved;
};
