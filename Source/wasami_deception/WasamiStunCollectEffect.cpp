#include "WasamiStunCollectEffect.h"

#include "Components/PostProcessComponent.h"
#include "WasamiPrimalPower.h"

namespace
{
	// BP_StunCollectEffect (pak_reference_2). PostProcess: saturation 0 under this gain, an orange screen; PostProcess1's
	// fringe (its unused gain is Primal Fear's red, left out like Primal Fear's).
	const FVector4 StunTintGain(1.6100000143051147, 0.9415510296821594, 0., 1.);
	constexpr float StunFlashFringe = 50.f;
	// BeginPlay: SetVectorParameterValue('Color') on the sphere's instance, then PlaySoundAtLocation(…, 1, 2).
	const FLinearColor StunSphereColor(0.258f, 0.0737f, 0.f, 1.f);
	constexpr float StunWavePitch = 2.f;

	// The timeline's tracks (CurveFloat_0, _2 and _3), key for key; CurveFloat_1 (float2) is BP_PrimalPower's.
	const FWasamiCurveKey StunGrowthKeys[] = {
		{0.f, 0.f, RCIM_Cubic, 0.4844522476196289f, 0.4844527244567871f},
		{1.9768927097320557f, 1.0141212940216064f, RCIM_Cubic, 0.3629918098449707f, 0.36299169063568115f},
		{2.497471570968628f, 1.0786545276641846f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey StunDesaturationKeys[] = {
		{0.006957054138183594f, 0.015488147735595703f, RCIM_Cubic, 0.f, 0.f},
		{0.4220871925354004f, 0.07975006103515625f, RCIM_Cubic, 0.12493659555912018f, 0.12493659555912018f},
		{1.9171667098999023f, 0.25414323806762695f, RCIM_Linear, 0.f, 0.f},
	};
	const FWasamiCurveKey StunOpacityKeys[] = {
		{0.f, 1.f, RCIM_Cubic, -0.21745750308036804f, -0.21745805442333221f},
		{0.7486822605133057f, 0.6337049007415771f, RCIM_Cubic, -0.5983153581619263f, -0.5983161926269531f},
		{1.924424648284912f, -0.0023174285888671875f, RCIM_Linear, 0.f, 0.f},
	};
}

const FRichCurve& AWasamiStunCollectEffect::GrowthCurve()
{
	static const FRichCurve Curve = MakeCurve(StunGrowthKeys);
	return Curve;
}

const FRichCurve& AWasamiStunCollectEffect::DesaturationCurve()
{
	static const FRichCurve Curve = MakeCurve(StunDesaturationKeys);
	return Curve;
}

const FRichCurve& AWasamiStunCollectEffect::OpacityCurve()
{
	static const FRichCurve Curve = MakeCurve(StunOpacityKeys);
	return Curve;
}

AWasamiStunCollectEffect::AWasamiStunCollectEffect()
{
	PostProcess->Settings.ColorGain = StunTintGain;
	PostProcess1->Settings.SceneFringeIntensity = StunFlashFringe;
	FadeCurve = AWasamiPrimalPower::PrimalFadeCurve();
	GrowthTrack = GrowthCurve();
	DesaturationTrack = DesaturationCurve();
	OpacityTrack = OpacityCurve();
	Range = StunRange;
	WavePitch = StunWavePitch;
	SphereColor = StunSphereColor;
}
