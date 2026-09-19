#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WasamiGameInstance.generated.h"

class UWasamiSettingsSaveGame;

/**
 * What outlives reopening a level and is never written to disk, after Dark Deception's BP_DD_GameInstance
 * (pak_reference_2): the lives, and the shards collected since the last fresh start (Shards To Be Removed), which the
 * game mode takes out of the level each time it opens again. Lives go through BP_DD_Functions' Get Lives, Decrement
 * Lives, Increment Lives and Reset Lives. Also the capture's bag of clips, which the hotel keeps in its level Blueprint
 * but a death here opens the level again.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/**
	 * BP_DD_GameMode's Check Settings Save and Set Settings, which both game modes run when play begins: the settings
	 * read from SettingsSlotName (or made and written there) and applied. Returns them.
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	UWasamiSettingsSaveGame* CheckSettingsSave();

	/** Global Settings Save Instance: the settings read last (read and applied first when none were yet). */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	UWasamiSettingsSaveGame* GetSettings();

	/**
	 * What the OPTIONS' SAVE & EXIT does with the settings once changed: applied (Set Settings), written to the slot, and
	 * given to the player if there is one (the values it reads, and Set Up Mouse Smoothing).
	 */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	/** The settings' slot; empty for the original's Settings (the tests use their own). */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FString SettingsSlotName;

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

	/** The capture's clip (0 to AWasamiCapture::NumChoices - 1), from the bag TakeNoRepeat draws from. */
	UFUNCTION(BlueprintCallable, Category = "Game")
	int32 TakeCaptureChoice();

	/**
	 * The macro Random Integer In Range (No Repeat) (StandardMacros, as the hotel's Death Event uses it): when not
	 * started, fills Remaining with Min..Max; then shuffles it and takes the last, and either empties it (fewer than 2
	 * were left, so the next call fills it again) or removes the one taken. Each run of Max - Min + 1 calls gives each
	 * number once; a run's last may be the next run's first. A range of one number gives Min.
	 */
	static int32 TakeNoRepeat(TArray<int32>& Remaining, bool& bStarted, int32 Min, int32 Max, const FRandomStream& Stream);

private:
	FString GetSettingsSlot() const;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	int32 Lives = StartingLives;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiSettingsSaveGame> Settings;

	/** The editor's display gamma when play began, given back when it ends (Set Settings changes the engine's). */
	float EditorDisplayGamma = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	TArray<FVector> ShardsToBeRemoved;

	/** The macro's Remaining valid choices and Started yet? for the capture. */
	TArray<int32> CaptureChoices;
	bool bCaptureChoicesStarted = false;
	FRandomStream CaptureStream{FMath::Rand()};
};
