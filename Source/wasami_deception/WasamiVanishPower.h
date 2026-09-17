#pragma once

#include "CoreMinimal.h"
#include "WasamiPowerBurst.h"
#include "WasamiVanishPower.generated.h"

class UParticleSystem;
class UParticleSystemComponent;
class USoundBase;

/**
 * Vanish's burst, after Dark Deception's BP_VanishPower (pak_reference_2). The power spawns it 50 m under the player,
 * turned as the player; it moves onto the player, sounds Stun_Wave_Attack_New_04, tells every enemy (the actors tagged
 * Enemy that implement the enemy interface) once that the player vanished, puffs PPP_VanishPuff in front of the player,
 * and over its 2 s timeline the screen flashes white and purple for 0.3 s. The rest of Vanish (the capsule the enemies'
 * sight passes through, the widget, the 15 s) is the power component's.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiVanishPower : public AWasamiPowerBurst
{
	GENERATED_BODY()

public:
	AWasamiVanishPower();

	/** Player Vanish, once, to every actor tagged Enemy that implements the enemy interface. Returns how many. */
	UFUNCTION(BlueprintCallable, Category = "Vanish", meta = (WorldContext = "WorldContextObject"))
	static int32 NotifyEnemies(const UObject* WorldContextObject);

	/** Loads what the burst uses into Out, so that a spawn waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The timeline's float2 track (Vanish's own: it reaches 1 at 0.3 s). */
	static const FRichCurve& VanishFadeCurve();

	UParticleSystemComponent* GetParticleSystem() const { return ParticleSystem; }

	/** PPP_VanishPuff: five puffs of smoke that rise and fade. */
	UPROPERTY(EditAnywhere, Category = "Vanish|Assets")
	TSoftObjectPtr<UParticleSystem> PuffParticles;

	/** Stun_Wave_Attack_New_04. */
	UPROPERTY(EditAnywhere, Category = "Vanish|Assets")
	TSoftObjectPtr<USoundBase> WaveSound;

protected:
	virtual void StartPower() override;

	/** The puff, 92 cm in front of the capsule's centre and 152 cm down. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vanish")
	TObjectPtr<UParticleSystemComponent> ParticleSystem;
};
