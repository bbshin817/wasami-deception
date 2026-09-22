#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WasamiGameMode.generated.h"

class AWasamiZoneFlow;
class UCameraShakeBase;
class USoundBase;
class UWasamiGameInstance;
class UWasamiLevelClearWidget;
class UWasamiSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWasamiDeathSignature, AActor*, Cause);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiAllShardsAlreadyCollectedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiCollectShardSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiAllShardsCollectedSignature);

/**
 * The game's mode, after Dark Deception's BP_DD_GameMode (pak_reference_2): spawns the player (AWasamiPlayerCharacter)
 * at the level's player start, keeps what the tablet's band shows, reads the save (or makes and writes one) when play
 * begins, counts the time played, takes the shards collected before a reopening out of the level, and passes a death on
 * to whoever listens (Death Dispatcher). The hospital's zones have no level Blueprints of their own here, so it also
 * does their part: the player starts at the saved checkpoint's player start (their Spawn), a death shows the death
 * screen and pauses the game (their DeathEvent), and a checkpoint's save shows SAVING PROGRESS; the rest of a zone's
 * level Blueprint is its AWasamiZoneFlow, which the mode spawns when play begins.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWasamiGameMode();

	virtual void Tick(float DeltaSeconds) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/**
	 * Current Objective: the tablet's band shows it in upper case. Empty until a zone's flow sets one (the original's
	 * default); Zone 1 sets COLLECT ALL SHARDS past the lift's door.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FText CurrentObjective;

	/**
	 * DeathEvent(Cause), once until ResetDeath: broadcasts OnDeath, shows the death screen (Level by the zone and whether
	 * the player caused it) and pauses the game, keeps the best shard streak and sets the save's current streak back to
	 * 0 (written by the death screen).
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

	/**
	 * Check Shards: a shard was collected (the shard calls it; Zone 1 calls it once a second after the shards are
	 * wanted). Broadcasts Collect Shard, Check Streak at once and, 0.05 s later, counts the level's shards: none left
	 * broadcasts All Shards Collected. A check already waiting is not put back (the original's Delay); Check Streak runs
	 * on every call.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void CheckShards();

	/**
	 * Check Streak: the save's current streak + 1 (the game mode's Shard Streak follows it up); where it reaches a
	 * milestone (StreakMilestoneFor), UMG_ShardStreak for it on the player's screen (Z 2; 200 and 500 add a life),
	 * BP_CameraShake_Streak at 1, the milestone's sound, and the save's Streak raised to it. Returns the milestone, or 0.
	 * Not written to the slot here (the checkpoints and the death screen write it).
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	uint8 CheckStreak();

	/** The Enum_ShardStreaks value Check Streak shows at a current streak: 1..10 at 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000; else 0. */
	static uint8 StreakMilestoneFor(int32 CurrentStreak);

	/** Check Streak's sound by milestone, as an index into StreakSounds: V1A for 1 and 2, V2 for 3 to 5, V3A for 6 and 7, V4 for 8 to 10. */
	static int32 StreakSoundIndex(uint8 Milestone);

	/** Shard_Streak_Milestone_V1A, _V2, _V3A and _V4 (PlaySound2D at 1). */
	UPROPERTY(EditDefaultsOnly, Category = "Game|Assets")
	TArray<TSoftObjectPtr<USoundBase>> StreakSounds;

	/** BP_CameraShake_Streak (PlayCameraShake at 1, camera-local). */
	UPROPERTY(EditDefaultsOnly, Category = "Game|Assets")
	TSoftClassPtr<UCameraShakeBase> StreakShakeClass;

	/** Collect Shard. */
	UPROPERTY(BlueprintAssignable, Category = "Game")
	FWasamiCollectShardSignature OnCollectShard;

	/** All Shards Collected: the zone's flow listens while its shards are wanted. */
	UPROPERTY(BlueprintAssignable, Category = "Game")
	FWasamiAllShardsCollectedSignature OnAllShardsCollected;

	/** The zone's flow spawned when play began, or null outside the zones. */
	AWasamiZoneFlow* GetZoneFlow() const { return ZoneFlow; }

	/**
	 * 発見の声 (the item 20): whether the enemy that has just seen the player may say so, taking the turn where it
	 * may. Not the original's, whose nurses each say their own Detected line: どの敵からでも FoundVoiceGap に 1 回まで
	 * (the WebGL version's rule, its record 15), so a corridor of them that all see the player at once says it once.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	bool TakeFoundVoice();

	/** How long a Found keeps every enemy quiet (s). */
	static constexpr float FoundVoiceGap = 12.f;

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
	 * A checkpoint's save: SAVING PROGRESS on the screen, the level's checkpoint, the time played added to the saved time,
	 * the counter back to 0, and the save written.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void SaveCheckpoint(int32 Checkpoint);

	/**
	 * The hospital's Escape (06_Hospital @55455, which its portal's Trigger_Escape calls): the game paused, the save as
	 * at a checkpoint with checkpoint 0 (SAVING PROGRESS, the time added, the counter back to 0, written), and the level
	 * clear screen with the save's results at Z 6, its Finished bound to FinishedLevel. Returns the screen, or null where
	 * there is no player to show it to.
	 *
	 * PauseDelay holds that pause back, and only it: the save and the screen still come in this frame, as the original's
	 * do. A paused game renders no sound that is not a UI one (FAudioDevice::HandlePause), so a caller with a sound to
	 * be heard as the screen comes up gives its length here — Zone 2's escape, whose music is to fall away rather than
	 * be cut (2026-09-21, the user's answer), passes AWasamiZone2Flow::EscapeMusicFade.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	UWasamiLevelClearWidget* Escape(float PauseDelay = 0.f);

	/**
	 * Finished Level (@75934), once: the game unpaused, and FinishedLevelDelay on the hospital's save entry emptied and
	 * written, the game instance reset (the shards collected forgotten, 3 lives) and the title opened (the original
	 * opens 06_Cinematic, the next chapter's, or TitleScreen when replaying; this game has no next chapter).
	 */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void FinishedLevel();

	/** Finished Level's Delay before the save is emptied and the level opens. */
	static constexpr float FinishedLevelDelay = 1.f;

	/** Whether Finished Level ran (its DoOnce closed). */
	bool HasFinishedLevel() const { return bLevelFinished; }

	/** The level Finished Level opened after its Delay (empty before). */
	const FString& GetLevelToOpen() const { return LevelToOpen; }

	/** The save read or made when play began (Struct Save). */
	UFUNCTION(BlueprintPure, Category = "Game")
	UWasamiSaveGame* GetSave() const { return StructSave; }

	/** SaveGameToSlot(Struct Save). */
	UFUNCTION(BlueprintCallable, Category = "Game")
	void WriteSave();

	UWasamiGameInstance* GetWasamiGameInstance() const;

	/** Total Shards: the level's shards 0.2 s after play began, before the collected ones were taken out. */
	int32 GetTotalShards() const { return TotalShards; }

	/** Shard Streak: the longest streak this level (Check Streak raises it as the streak grows, DeathEvent as one ends). */
	int32 GetShardStreak() const { return ShardStreak; }

	/**
	 * Destroys the level's shards whose place, truncated, is among Collected (no sound, no count), and returns how many
	 * shards are left.
	 */
	static int32 RemoveCollectedShards(UWorld* World, const TArray<FVector>& Collected);

	/**
	 * The checkpoint the level opened at: the save's, with Zone 1's 0 written as 4 (the entrance, which this game does
	 * not have, saves 4 before it opens Zone 1). Settled when the player is placed, before any actor begins play; items
	 * 6 and 13 set their sections up from it.
	 */
	UFUNCTION(BlueprintPure, Category = "Game")
	int32 GetStartCheckpoint() const { return StartCheckpoint; }

	/**
	 * Whether the level opened as the hospital's start: Zone 1 with no checkpoint saved, whose 0 is written as 4 (the
	 * entrance's Spawn calls its 00_Initial Start only at 0). NEW GAME, RESTART and a finished level open it so; a
	 * death's reopening and RESUME find 4 or on.
	 */
	UFUNCTION(BlueprintPure, Category = "Game")
	bool IsNewStart() const { return bNewStart; }

	/** The hospital's zone a level name is (1 for 'L_Hospital_Zone1', 2 for 'L_Hospital_Zone2'), or 0. */
	static int32 ZoneOf(const FString& LevelName);

	/**
	 * The player start (its PlayerStartTag, the original's actor name) a zone's Spawn moves the player to at a
	 * checkpoint: Zone 1's 4, 5, 6 and Zone 2's 7 to 10; None for any other.
	 */
	static FName PlayerStartTagFor(int32 Zone, int32 Checkpoint);

	/**
	 * The death screen's Level a zone's DeathEvent gives: Zone 1 Traps when the player caused the death and Asylum
	 * otherwise, Zone 2 the other way round (as in the original).
	 */
	static uint8 DeathScreenLevelFor(int32 Zone, bool bCausedByPlayer);

	/** Zone 1's level, which Zone 2 opens without a checkpoint (the original opens the entrance). */
	static const TCHAR* Zone1LevelName;

	/** Zone 2's level, which Zone 1 opens from the ambulance's roof. */
	static const TCHAR* Zone2LevelName;

	/** The title's level (the original's TitleScreen). */
	static const TCHAR* TitleLevelName;

	/**
	 * The zone a saved checkpoint goes on in, as the entrance's Spawn opens it (the title's RESUME goes through there):
	 * Zone 2 for 7 to 10, Zone 1 for the rest (4 to 6, and 0, which Zone 1 reads as its arrival).
	 */
	static const TCHAR* LevelForCheckpoint(int32 Checkpoint);

	/** The fade from black a level opens with (UMG_BlackFade_2 fading out at 10, Z 10). */
	static constexpr float OpeningFadeSpeed = 10.f;
	static constexpr int32 OpeningFadeZOrder = 10;

	/** The save's slot (the original's structSlot); the tests write elsewhere. */
	UPROPERTY(EditDefaultsOnly, Category = "Game")
	FString SaveSlotName;

	/** The level the mode tells the zones by; left empty, the world's current one (the tests set it). */
	FString LevelName;

protected:
	virtual void BeginPlay() override;

private:
	/** Check For Level Struct Save: reads the slot, or makes a save and writes it. */
	void CheckForLevelStructSave();

	/** 0.2 s after BeginPlay: Total Shards, and the collected shards taken out. */
	void RemoveShardsToBeRemoved();

	/** Check Shards' count, 0.05 s after the shard. */
	void CountShardsLeft();

	/** Reads the save and settles StartCheckpoint and bNewStart, once (the player is placed before BeginPlay). */
	void PrepareStart();

	/** The hospital's zone of LevelName, or of the world's current level when it is empty. */
	int32 CurrentZone() const;

	/** The zone's DeathEvent: the death screen for the cause, the game paused. */
	void ShowDeathScreen(AActor* Cause);

	/** Finished Level after its Delay: the save entry emptied, the game instance reset, Zone 1 opened. */
	void LeaveFinishedLevel();

	/** Escape's pause when it was held back: skipped where NEXT has already been pressed, as that unpauses. */
	void PauseAfterEscape();

	UPROPERTY(Transient)
	TObjectPtr<UWasamiSaveGame> StructSave;

	/** StreakSounds and StreakShakeClass, loaded when play begins. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> LoadedStreakSounds;

	UPROPERTY(Transient)
	TSubclassOf<UCameraShakeBase> LoadedStreakShake;

	float Time = 0.f;
	/** The time counter's gate, open from the start. */
	bool bTimeCounting = true;
	/** DeathEvent's DoOnce. */
	bool bDeathClosed = false;
	/** Finished Level's DoOnce. */
	bool bLevelFinished = false;
	FString LevelToOpen;
	int32 TotalShards = 0;
	int32 ShardStreak = 0;
	int32 StartCheckpoint = 0;
	bool bNewStart = false;
	bool bStartPrepared = false;
	/** When Take Found Voice last let one through (the world's time), a gap before play so the first one goes. */
	float LastFoundVoice = -FoundVoiceGap;
	FTimerHandle ShardRemovalTimer;
	FTimerHandle CheckShardsTimer;
	FTimerHandle FinishedLevelTimer;
	FTimerHandle EscapePauseTimer;

	UPROPERTY(Transient)
	TObjectPtr<AWasamiZoneFlow> ZoneFlow;
};
