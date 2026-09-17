#include "WasamiPowerBurst.h"

#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"

namespace
{
	// PostProcess1's weight is 1 at float2 = 0 and 0 from float2 = 0.3 on.
	constexpr float FlashFadeEnd = 0.3f;
	// PostProcess1's midtones gain (the flash); PostProcess washes the screen to saturation 0.
	const FVector4 FlashMidtonesGain(100., 100., 100., 1.);
	const FVector4 TintSaturation(0., 0., 0., 1.);
}

AWasamiPowerBurst::AWasamiPowerBurst()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = SceneRoot;

	// Both volumes keep UE's unbound, priority 0, and start at weight 0. The settings every power shares are set here;
	// the colour gain (and PostProcess1's fringe) are the power's. PostProcess1 also holds a saturation and a colour gain
	// without their overrides, which blend into nothing and are left out.
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(SceneRoot);
	PostProcess->BlendWeight = 0.f;
	PostProcess->Settings.bOverride_ColorSaturation = true;
	PostProcess->Settings.ColorSaturation = TintSaturation;
	PostProcess->Settings.bOverride_ColorGain = true;

	// ColorGamma's override is on at its default value (1, 1, 1, 1), as in the original.
	PostProcess1 = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess1"));
	PostProcess1->SetupAttachment(SceneRoot);
	PostProcess1->BlendWeight = 0.f;
	PostProcess1->Settings.bOverride_ColorGamma = true;
	PostProcess1->Settings.bOverride_ColorGainMidtones = true;
	PostProcess1->Settings.ColorGainMidtones = FlashMidtonesGain;
	PostProcess1->Settings.bOverride_SceneFringeIntensity = true;
}

FRichCurve AWasamiPowerBurst::MakeCurve(TConstArrayView<FWasamiCurveKey> Keys)
{
	// A key added after one in auto tangent mode recomputes that one's tangents: every key is made a user key as it goes
	// in (the evaluation does not look at the mode), so the export's tangents stay.
	FRichCurve Curve;
	for (const FWasamiCurveKey& Each : Keys)
	{
		FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Each.Time, Each.Value));
		Key.InterpMode = Each.Interp;
		Key.TangentMode = RCTM_User;
		Key.ArriveTangent = Each.ArriveTangent;
		Key.LeaveTangent = Each.LeaveTangent;
	}
	return Curve;
}

float AWasamiPowerBurst::TintWeight(float Fade)
{
	return FMath::Lerp(1.f, 0.f, Fade);
}

float AWasamiPowerBurst::FlashWeight(float Fade)
{
	return FMath::GetMappedRangeValueClamped(FVector2f(0.f, FlashFadeEnd), FVector2f(1.f, 0.f), Fade);
}

void AWasamiPowerBurst::BeginPlay()
{
	Super::BeginPlay();
	StartPower();
	// PlayFromStart: the position goes to 0 with an update, then the timeline plays.
	TimelinePosition = 0.f;
	bPlaying = true;
	UpdateTimeline(TimelinePosition);
}

void AWasamiPowerBurst::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bPlaying)
	{
		return;
	}
	// UE4's FTimeline: a step past the end stops at the end with a last update, then the timeline finishes.
	float NewPosition = TimelinePosition + DeltaSeconds;
	const bool bFinished = NewPosition > TimelineLength;
	if (bFinished)
	{
		NewPosition = TimelineLength;
		bPlaying = false;
	}
	TimelinePosition = NewPosition;
	UpdateTimeline(TimelinePosition);
	if (bFinished)
	{
		Destroy();
	}
}

void AWasamiPowerBurst::UpdateTimeline(float Position)
{
	const float Fade = FadeCurve.Eval(Position);
	PostProcess->BlendWeight = TintWeight(Fade);
	PostProcess1->BlendWeight = FlashWeight(Fade);
}
