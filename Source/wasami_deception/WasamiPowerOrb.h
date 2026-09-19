#pragma once

#include "CoreMinimal.h"
#include "WasamiSpecialShard.h"
#include "WasamiPowerOrb.generated.h"

class AWasamiStunCollectEffect;
class USoundBase;

/**
 * The power orb, after Dark Deception's Blueprints/Main/BP_PowerOrb (pak_reference_2): the special shard's cycle over
 * the level's orb spawn points, with power_orb for its crystal and an orange light. The player's touch takes it once: a
 * flash, the pickup sound, ENEMIES STUNNED over the screen, a shake, the countdown's music, BP_StunCollectEffect around
 * the player, every actor tagged Enemy sent Set State(Stun, by orb), and the orb's end.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPowerOrb : public AWasamiSpecialShard
{
	GENERATED_BODY()

public:
	AWasamiPowerOrb();

	/** Every actor tagged Enemy that implements the enemy interface gets Set State(Stun, true). Returns how many did. */
	UFUNCTION(BlueprintCallable, Category = "Power Orb", meta = (WorldContext = "WorldContextObject"))
	static int32 StunAllEnemies(const UObject* WorldContextObject);

	/** 8-Dark_power_ball_countdown_: the 17 s the enemies lie stunned. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSoftObjectPtr<USoundBase> CountdownSound;

	/** BP_StunCollectEffect. */
	UPROPERTY(EditAnywhere, Category = "Power Orb|Assets")
	TSubclassOf<AWasamiStunCollectEffect> CollectEffectClass;

protected:
	virtual void LoadPickupAssets(TArray<TObjectPtr<UObject>>& Out) const override;
	virtual void CollectShard() override;
};
