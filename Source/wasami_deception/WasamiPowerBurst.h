#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "GameFramework/Actor.h"
#include "WasamiPowerBurst.generated.h"

class UPostProcessComponent;

/** One key of a Blueprint timeline's float track, as the export stores it. */
struct FWasamiCurveKey
{
	float Time;
	float Value;
	ERichCurveInterpMode Interp;
	float ArriveTangent;
	float LeaveTangent;
};

/**
 * The frame Dark Deception's one-shot powers share (pak_reference_2's BP_PrimalPower, BP_TelekinesisPower and
 * BP_VanishPower): two unbound post process volumes and a 2 s timeline whose float2 track fades them out — PostProcess
 * (the screen in one colour) over Lerp(1, 0, float2), PostProcess1 (the white flash) over MapRangeClamped(float2, 0,
 * 0.3, 1, 0) — after which the actor destroys itself. The powers set the volumes' settings and float2's keys in their
 * constructors and add their own work to StartPower and UpdateTimeline.
 */
UCLASS(Abstract)
class WASAMI_DECEPTION_API AWasamiPowerBurst : public AActor
{
	GENERATED_BODY()

public:
	AWasamiPowerBurst();

	virtual void Tick(float DeltaSeconds) override;

	/** A curve with the keys as given; every key keeps its tangents (none are recomputed as keys are added). */
	static FRichCurve MakeCurve(TConstArrayView<FWasamiCurveKey> Keys);

	/** PostProcess's weight at a float2 value: Lerp(1, 0, float2). */
	static float TintWeight(float Fade);

	/** PostProcess1's weight at a float2 value: MapRangeClamped(float2, 0, 0.3, 1, 0). */
	static float FlashWeight(float Fade);

	/** The timeline's length (s). */
	static constexpr float TimelineLength = 2.f;

	UPostProcessComponent* GetTint() const { return PostProcess; }
	UPostProcessComponent* GetFlash() const { return PostProcess1; }

	/** The timeline's position (s). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Power")
	float TimelinePosition = 0.f;

protected:
	/** Runs StartPower, then plays the timeline from the start (an update at 0). */
	virtual void BeginPlay() override;

	/** The power's own work on BeginPlay, before the timeline plays. */
	virtual void StartPower() {}

	/** The timeline's update at a position: the volumes' weights from float2. */
	virtual void UpdateTimeline(float Position);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power")
	TObjectPtr<USceneComponent> SceneRoot;

	/** The screen washed into one colour (saturation 0 and a colour gain); weight 0 until the timeline runs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power")
	TObjectPtr<UPostProcessComponent> PostProcess;

	/** The white flash (midtones × 100) and its fringe; weight 0 until the timeline runs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power")
	TObjectPtr<UPostProcessComponent> PostProcess1;

	/** The timeline's float2 track. */
	FRichCurve FadeCurve;

private:
	bool bPlaying = false;
};
