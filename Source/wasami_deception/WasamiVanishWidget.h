#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiVanishWidget.generated.h"

class UImage;
class UMaterialInterface;

/**
 * Dark Deception's UMG_Vanish (pak_reference_2's UI/Main/Powers/UMG_Vanish), on the screen from a Vanish until the power
 * is ready again: a purple, wobbling vignette over the whole view (MM_WobblyVignette). Its Construct plays the 1 s
 * animation Vanish at 1 / Speed (the power sets Speed to 15), which fades the image in over the first tenth, holds it and
 * fades it out over the last tenth; the image then stays at an opacity of 0 until the widget is removed.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiVanishWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiVanishWidget(const FObjectInitializer& ObjectInitializer);

	/** Loads what the widget shows into Out, so that the first Vanish does not wait for it. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The animation Vanish's RenderOpacity track at a time (s) of its 1 s. */
	static float EvaluateOpacity(float AnimationSeconds);

	/** The animation's length (s). */
	static constexpr float AnimationLength = 1.f;

	/** How many times longer than its 1 s the animation plays (the original's default is 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vanish", meta = (ExposeOnSpawn = "true"))
	float Speed = 1.f;

	/** MM_WobblyVignette (its graph is an estimate). */
	UPROPERTY(EditAnywhere, Category = "Vanish")
	TSoftObjectPtr<UMaterialInterface> VignetteMaterial;

	/** Where the animation is (s of its 1 s), and whether it still plays. */
	float GetAnimationTime() const { return AnimationTime; }
	bool IsAnimationPlaying() const { return bAnimationPlaying; }

	UImage* GetVignette() const { return Vignette; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** The animation's update: Image_82's render opacity. */
	void ApplyAnimation();

	/** Image_82: MM_WobblyVignette over the screen, tinted purple. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	float AnimationTime = 0.f;
	float PlaybackSpeed = 1.f;
	bool bAnimationPlaying = false;
};
