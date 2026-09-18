#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"

/**
 * The original's widget animations (UMG MovieScene sections, pak_reference_2) as curves: the keys as exported, in ticks
 * at 60000 a second with the value and the arrive and leave tangents per tick. A key is cubic unless the export says
 * linear (its InterpMode, which shapes the way on to the next key); the exported tangents are the ones UE worked out
 * (auto) or the author's (user), and go in as they are.
 */
namespace WasamiWidgetAnimation
{
	constexpr double TicksPerSecond = 60000.;

	struct FAnimKey
	{
		double Ticks;
		float Value;
		double ArrivePerTick;
		double LeavePerTick;
		ERichCurveInterpMode Interp = RCIM_Cubic;
	};

	inline FRichCurve MakeCurve(TConstArrayView<FAnimKey> Keys)
	{
		FRichCurve Curve;
		for (const FAnimKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(static_cast<float>(Each.Ticks / TicksPerSecond), Each.Value));
			Key.InterpMode = Each.Interp;
			Key.TangentMode = RCTM_Break;
			Key.ArriveTangent = static_cast<float>(Each.ArrivePerTick * TicksPerSecond);
			Key.LeaveTangent = static_cast<float>(Each.LeavePerTick * TicksPerSecond);
		}
		return Curve;
	}

	/** The curve at Seconds into an animation Length long. */
	inline float Eval(const FRichCurve& Curve, float Seconds, float Length)
	{
		return Curve.Eval(FMath::Clamp(Seconds, 0.f, Length));
	}
}
