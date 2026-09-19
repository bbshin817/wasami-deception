#pragma once

#include "CoreMinimal.h"
#include "WasamiSphereBurst.h"
#include "WasamiStunCollectEffect.generated.h"

/**
 * What picking up a power orb shows, after Dark Deception's BP_StunCollectEffect (pak_reference_2): BP_PrimalPower's
 * frame with values of its own and no stun (the orb stuns every enemy itself). It moves onto the player, sounds
 * Stun_Wave_Attack_New_04 at pitch 2, shakes the camera, and over its 2 s timeline an orange sphere swells to about
 * 40 m and fades while the screen flashes white and orange.
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiStunCollectEffect : public AWasamiSphereBurst
{
	GENERATED_BODY()

public:
	AWasamiStunCollectEffect();

	/** The timeline's float, desaturation and opacity tracks (its float2 is Primal Fear's, key for key). */
	static const FRichCurve& GrowthCurve();
	static const FRichCurve& DesaturationCurve();
	static const FRichCurve& OpacityCurve();

	/** The class's Range (cm). */
	static constexpr float StunRange = 4000.f;
};
