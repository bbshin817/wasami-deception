#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WasamiGameMode.generated.h"

class UWasamiGameInstance;
class UWasamiSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWasamiDeathSignature, AActor*, Cause);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiAllShardsAlreadyCollectedSignature);

/**
 * The game's mode, after Dark Deception's BP_DD_GameMode (pak_reference_2): spawns the player (AWasamiPlayerCharacter)
 * at the level's player start, keeps what the tablet's band shows, reads the save (or makes and writes one) when play
 * begins, counts the time played, takes the shards collected before a reopening out of the level, and passes a death on
 * to whoever listens (Death Dispatcher).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWasamiGameMode();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Current Objective: the tablet's band shows it in upper case. The hospital's Zone 1 sets it to COLLECT ALL SHARDS
	 * once the player is on their way (pak_reference_2's 06_Hospital_Zone_01, @2293).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FText CurrentObjective;

	/**
	 * DeathEvent(Cause), once until ResetDeath: broadcasts OnDeath, keeps the best shard streak and sets the save's
	 * current streak back to 0 (written by the death screen).
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void DeathEvent(AActor* Cause);

	/** Reset Death: opens DeathEvent again. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResetDeath();

	/** Whether DeathEvent would go through. */
	bool IsDeathOpen() const { return !bDeathClosed; }

	/** Death Dispatcher. */
	UPROPERTY(BlueprintAssignable, Category = "Game")
	FWasamiDeathSignature OnDeath;

	/** All Shards Already Collected: the level reopened with every shard in it collected before. */
	UPROPERTY(BlueprintAssignable, Category = "Game")
	FWasamiAllShardsAlreadyCollectedSignature OnAllShardsAlreadyCollected;

	/** Pause Time Counter and Unpause Time Counter: the gate the tick counts through. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void PauseTimeCounter();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void UnpauseTimeCounter();

	/** Reset Time Counter: Time = 0. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ResetTimeCounter();

	/** Seconds played since the level opened or the last checkpoint. */
	UFUNCTION(BlueprintPure, Category = "Game")
	float GetTime() const { return Time; }

	/**
	 * A checkpoint's save: the level's checkpoint, the time played added to the saved time, the counter back to 0, and
	 * the save written.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SaveCheckpoint(int32 Checkpoint);

	/** The save read or made when play began (Struct Save). */
	UFUNCTION(BlueprintPure, Category = "Game")
	UWasamiSaveGame* GetSave() const { return StructSave; }

	/** SaveGameToSlot(Struct Save). */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void WriteSave();

	UWasamiGameInstance* GetWasamiGameInstance() const;

	/** Total Shards: the level's shards 0.2 s after play began, before the collected ones were taken out. */
	int32 GetTotalShards() const { return TotalShards; }

	/** Shard Streak: the best streak a death has cut short. */
	int32 GetShardStreak() const { return ShardStreak; }

	/**
	 * Destroys the level's shards whose place, truncated, is among Collected (no sound, no count), and returns how many
	 * shards are left.
	 */
	static int32 RemoveCollectedShards(UWorld* World, const TArray<FVector>& Collected);

	/** The save's slot (the original's structSlot); the tests write elsewhere. */
	UPROPERTY(EditDefaultsOnly, Category = "Game")
	FString SaveSlotName;

protected:
	virtual void BeginPlay() override;

private:
	/** Check For Level Struct Save: reads the slot, or makes a save and writes it. */
	void CheckForLevelStructSave();

	/** 0.2 s after BeginPlay: Total Shards, and the collected shards taken out. */
	void RemoveShardsToBeRemoved();

	UPROPERTY(Transient)
	TObjectPtr<UWasamiSaveGame> StructSave;

	float Time = 0.f;
	/** The time counter's gate, open from the start. */
	bool bTimeCounting = true;
	/** DeathEvent's DoOnce. */
	bool bDeathClosed = false;
	int32 TotalShards = 0;
	int32 ShardStreak = 0;
	FTimerHandle ShardRemovalTimer;
};
