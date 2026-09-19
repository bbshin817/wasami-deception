#pragma once

#include "CoreMinimal.h"
#include "WasamiSpecialShard.h"
#include "WasamiBonusShard.generated.h"

class AWasamiBonusShardCollectEffect;
class AWasamiPlayerCharacter;

/**
 * The red bonus shard, after Dark Deception's Blueprints/Main/BP_BonusShard (pak_reference_2): the special shard's
 * cycle over the level's bonus shard spawn points, with soul_shard at 20 for its crystal and a red light. A frame after
 * BeginPlay (its Delay 0) it removes itself if the save has its ID among the level's BonusShards; otherwise the spawn
 * timer starts. The player's touch takes it once: its ID into the save's BonusShards (written at the next checkpoint's
 * save), a flash, the pickup sound, ENEMIES REVEALED over the screen, a shake, the crystal, its mark and its light
 * removed, BP_BonusShardCollectEffect, and the reveal: the class of every actor tagged Enemy is added to the player's
 * map (Add To Map) now and again every 1 to 2 s (so enemies that come later are shown too), and 60 s after the pickup
 * they are all removed (Remove From Map) and the shard destroys itself.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiBonusShard : public AWasamiSpecialShard
{
	GENERATED_BODY()

public:
	AWasamiBonusShard();

	/** The reveal's round: Add To Map for the class of every actor tagged Enemy, the next round in 1 to 2 s. */
	void RevealEnemies();

	/** Whether the reveal is running (from the pickup until its 60 s are over). */
	bool IsRevealing() const { return bRevealEnding; }

	/** Seconds to the reveal's end (-1 when it is not running). */
	float GetRevealTimeLeft() const;

	/** ID: which of the level's bonus shards it is, in the save's BonusShards (Zone 2's is 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Shard")
	int32 ID = 0;

	/** The reveal's length (s) and the range of each round's delay (s). */
	static constexpr float RevealLength = 60.f;
	static constexpr float RevealRoundMin = 1.f;
	static constexpr float RevealRoundMax = 2.f;

	/** BP_BonusShardCollectEffect. */
	UPROPERTY(EditAnywhere, Category = "Bonus Shard|Assets")
	TSubclassOf<AWasamiBonusShardCollectEffect> CollectEffectClass;

protected:
	virtual void LoadPickupAssets(TArray<TObjectPtr<UObject>>& Out) const override;

	/** Delay 0, then the save's check. */
	virtual void BeginCycle() override;

	virtual void CollectShard() override;

private:
	/** The Delay 0's end: removed if the save has the ID, else the spawn timer. */
	void CheckSave();
	/** A round's delay over: the next round. */
	void OnRevealRound();
	/** The Delay 60's end: Remove From Map for the class of every actor tagged Enemy, and the shard's end. */
	void EndReveal();

	/** The player the pickup cast (BP_DD_PlayerCharacter), whose map shows the enemies. */
	TWeakObjectPtr<AWasamiPlayerCharacter> RevealPlayer;

	FTimerHandle RoundTimer;
	FTimerHandle RevealTimer;
	/** The two Delays' latent actions: a call while one runs does nothing. */
	bool bRoundPending = false;
	bool bRevealEnding = false;
};
