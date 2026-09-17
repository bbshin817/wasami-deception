#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSpeedBoostWidget.generated.h"

class UImage;
class UMaterialInterface;
class UTexture2D;

/**
 * Dark Deception's UMG_SpeedBoost (pak_reference_2's UI/Main/Powers/UMG_SpeedBoost), on the screen while a speed boost
 * lasts: red speed lines and a red vignette over the whole view, both more opaque the faster the player goes. The
 * original sets the opacities after a 0.001 s Delay from its Tick, so they follow the speed a frame late and the first
 * frame shows both at their full opacity; that is kept.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSpeedBoostWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiSpeedBoostWidget(const FObjectInitializer& ObjectInitializer);

	/** Loads what the widget shows into Out, so that the first boost does not wait for it (the original's player holds
	 * the widget class, and with it these, from the start). */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** M_Speedlines. */
	UPROPERTY(EditAnywhere, Category = "Speed Boost")
	TSoftObjectPtr<UMaterialInterface> LinesMaterial;

	/** T_VignetteNew. */
	UPROPERTY(EditAnywhere, Category = "Speed Boost")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Lines: M_Speedlines over the screen. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Lines;

	/** Image_72: T_VignetteNew over the screen. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	/** The Delay the last tick started, which sets the opacities on the next one. */
	bool bOpacityDelayed = false;
};
