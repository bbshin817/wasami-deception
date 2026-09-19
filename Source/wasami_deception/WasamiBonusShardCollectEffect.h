#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerBurst.h"
#include "WasamiBonusShardCollectEffect.generated.h"

class UCameraShakeBase;
class USoundBase;

/**
 * What picking up a bonus shard shows, after Dark Deception's BP_BonusShardCollectEffect (pak_reference_2): the
 * burst's frame and nothing else. It stays where it is spawned (its volumes are unbound), sounds
 * Stun_Wave_Attack_New_04 at 0.5 and pitch 1.5, shakes the camera, and over its 2 s timeline the screen flashes white
 * and red. Its timeline's float, desaturation and opacity tracks drive nothing and are left out.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiBonusShardCollectEffect : public AWasamiPowerBurst
{
	GENERATED_BODY()

public:
	AWasamiBonusShardCollectEffect();

	/** Loads what the class's defaults use into Out, so that a spawn waits on nothing. */
	void LoadDefaultAssets(TArray<TObjectPtr<UObject>>& Out) const;

	/** Stun_Wave_Attack_New_04. */
	UPROPERTY(EditAnywhere, Category = "Bonus Shard|Assets")
	TSoftObjectPtr<USoundBase> WaveSound;

	/** 01_Hotel_Lobby_ElevatorShakeStop, played at a scale of 25. */
	UPROPERTY(EditAnywhere, Category = "Bonus Shard|Assets")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

	/** PlaySoundAtLocation's volume and pitch for the wave. */
	static constexpr float WaveVolume = 0.5f;
	static constexpr float WavePitch = 1.5f;

protected:
	/** The wave and the shake. */
	virtual void StartPower() override;
};
