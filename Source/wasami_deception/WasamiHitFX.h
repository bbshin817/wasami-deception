#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WasamiHitFX.generated.h"

class UCameraShakeBase;
class UCurveFloat;
class UPostProcessComponent;
class UTimelineComponent;

/**
 * The original's Blueprints/Shared/BP_HitFX: the flash of being hit. Spawned anywhere, it shakes the player's camera and
 * washes the whole view red with colour fringes (an unbound post process whose weight falls from 1 to 0 in 0.35 s), then
 * goes 5 s on (its timeline's length). The zone flows spawn it as the cell's spikes reach the player; the nurses and the
 * traps that kill do too (their items).
 */
UCLASS()
class WASAMI_DECEPTION_API AWasamiHitFX : public AActor
{
	GENERATED_BODY()

public:
	AWasamiHitFX();

	/** The timeline's length (the Blueprint timeline's default; its curve is 0 from 0.35 s). */
	static constexpr float TimelineLength = 5.f;

	/** Hit Effect: the shake, and the timeline played from the start at PlayRate. BeginPlay does it. */
	UFUNCTION(BlueprintCallable, Category = "Wasami|Hit")
	void HitEffect();

	UPostProcessComponent* GetPostProcess() const { return PostProcess; }

	/** ClientPlayCameraShake's scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wasami|Hit")
	float ShakeScale = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wasami|Hit")
	float PlayRate = 1.f;

	/** BP_04_BossFight_CameraShake_Initial (made by the level build with the sequences, dd_sequence). */
	UPROPERTY(EditDefaultsOnly, Category = "Wasami|Hit")
	TSoftClassPtr<UCameraShakeBase> ShakeClass;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTimelineUpdate(float Value);

	UFUNCTION()
	void OnTimelineFinished();

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Hit")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Hit")
	TObjectPtr<UPostProcessComponent> PostProcess;

	UPROPERTY(VisibleAnywhere, Category = "Wasami|Hit")
	TObjectPtr<UTimelineComponent> Timeline;

	/** The timeline's float track (CurveFloat_0). */
	UPROPERTY()
	TObjectPtr<UCurveFloat> Curve;
};
