#pragma once

#include "CoreMinimal.h"
#include "WasamiSphereBurst.h"
#include "WasamiPrimalPower.generated.h"

/**
 * Primal Fear, after Dark Deception's BP_PrimalPower (pak_reference_2). The power spawns it 50 m under the player with
 * the upgrade level's Range; it moves onto the player, sounds Stun_Wave_Attack_New_04, shakes the camera, stuns once
 * every enemy within Range (through walls), and over its 2 s timeline a red sphere around the spot swells to Range and
 * fades while the screen flashes white and red.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiPrimalPower : public AWasamiSphereBurst
{
	GENERATED_BODY()

public:
	AWasamiPrimalPower();

	/**
	 * The stun: every actor with a pawn-type body within Radius of Center that implements the enemy interface gets
	 * Set State(Stun, bByOrb false), once. Returns how many did.
	 */
	UFUNCTION(BlueprintCallable, Category = "Primal Fear", meta = (WorldContext = "WorldContextObject"))
	static int32 StunEnemies(const UObject* WorldContextObject, FVector Center, float Radius);

	/** Loads what the power uses into Out, so that a spawn waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The timeline's tracks. */
	static const FRichCurve& GrowthCurve();
	static const FRichCurve& DesaturationCurve();
	static const FRichCurve& OpacityCurve();
	static const FRichCurve& PrimalFadeCurve();

protected:
	/** The sphere burst's start, then the stun of every enemy within Range. */
	virtual void StartPower() override;
};
