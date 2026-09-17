#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerBurst.h"
#include "WasamiTelekinesisPower.generated.h"

class UCameraShakeBase;
class UParticleSystem;
class USoundBase;

/**
 * The telekinesis, after Dark Deception's BP_TelekinesisPower (pak_reference_2). The power spawns it 50 m under the
 * player with the upgrade level's Range; it moves onto the player, sounds Stun_Wave_Attack_New_04, shakes the camera,
 * pulls once every shard (every actor with the telekinesis interface) within Range, through walls, and over its 2 s
 * timeline the screen washes blue and flashes white; 0.2 s in, the force field's particles
 * (P_WasamiForceField, at twice their size) burst where the player was.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiTelekinesisPower : public AWasamiPowerBurst
{
	GENERATED_BODY()

public:
	AWasamiTelekinesisPower();

	/**
	 * The pull: every actor with a pawn, world-dynamic or world-static body within Radius of Center that implements
	 * the telekinesis interface gets Activate, once. Returns how many did.
	 */
	UFUNCTION(BlueprintCallable, Category = "Telekinesis", meta = (WorldContext = "WorldContextObject"))
	static int32 PullShards(const UObject* WorldContextObject, FVector Center, float Radius);

	/** Loads what the power uses into Out, so that a spawn waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The timeline's float2 track (Primal Fear's keys: it reaches 1 at 0.5 s). */
	static const FRichCurve& TelekinesisFadeCurve();

	/** The wait from the start to the force field's particles (s), and their scale. */
	static constexpr float ForceFieldDelay = 0.2f;
	static constexpr float ForceFieldScale = 2.f;

	/** Range (cm): the power sets it by the upgrade level before the spawn finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis", meta = (ExposeOnSpawn = "true"))
	float Range = 1500.f;

	/** Stun_Wave_Attack_New_04. */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Assets")
	TSoftObjectPtr<USoundBase> WaveSound;

	/** 01_Hotel_Lobby_ElevatorShakeStop, played at a scale of 25. */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Assets")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

	/** P_WasamiForceField: the blue force field (a sphere that closes in, an aura, a ground ring, star dust).
	 * P_ky_forceField_Telekinesis with its sphere's light weakened (dd_powers.FORCE_FIELD_LIGHT_SCALE). */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Assets")
	TSoftObjectPtr<UParticleSystem> ForceFieldParticles;

protected:
	virtual void StartPower() override;

private:
	/** ForceFieldDelay after the start: the particles where the actor is (where the player was), at ForceFieldScale. */
	void SpawnForceField();

	FTimerHandle ForceFieldTimer;
};
