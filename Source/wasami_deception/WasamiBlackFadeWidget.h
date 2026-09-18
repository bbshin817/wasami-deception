#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiBlackFadeWidget.generated.h"

class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiBlackFadeFinishedSignature);

/**
 * A black screen that fades in or out, after Dark Deception's Blueprints/UMG/UMG_BlackFade_2 (pak_reference_2): its
 * Construct plays FadeIn (black from nothing) or FadeOut (nothing from black), 5 s long, at Speed; when it ends,
 * Animation Finished, and unless held the widget takes itself off the screen. The game mode adds one fading out at 10
 * (0.5 s) whenever a level opens.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiBlackFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fade in?: FadeIn, else FadeOut. */
	UPROPERTY(BlueprintReadWrite, Category = "Fade")
	bool bFadeIn = true;

	/** Speed: the animation's play rate. */
	UPROPERTY(BlueprintReadWrite, Category = "Fade")
	float Speed = 2.f;

	/** bHold?: stays on the screen when the animation ends. */
	UPROPERTY(BlueprintReadWrite, Category = "Fade")
	bool bHold = false;

	/** Animation Finished. */
	UPROPERTY(BlueprintAssignable, Category = "Fade")
	FWasamiBlackFadeFinishedSignature OnFadeFinished;

	/**
	 * Creates the fade for the first player, sets Fade in? and Speed, and adds it to the viewport at ZOrder. Returns it
	 * (null without a player controller).
	 */
	static UWasamiBlackFadeWidget* Show(const UObject* WorldContextObject, bool bInFadeIn, float InSpeed, int32 ZOrder);

	/** FadeIn's and FadeOut's length at a play rate of 1. */
	static constexpr float AnimationLength = 5.f;

	/** Moves the animation on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Image_29's RenderOpacity now. */
	float GetOpacity() const { return Opacity; }

	/** Whether the animation has ended. */
	bool IsFinished() const { return bFinished; }

	/** The animations' RenderOpacity at Seconds into them (at a play rate of 1). */
	static float EvaluateFadeIn(float Seconds);
	static float EvaluateFadeOut(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;

	/** Seconds into the animation (already scaled by Speed). */
	float Time = 0.f;
	float Opacity = 0.f;
	bool bFinished = false;
};
