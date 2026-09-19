#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiCutsceneWidget.generated.h"

class UHorizontalBox;
class UImage;
class ULevelSequencePlayer;
class UMaterialParameterCollection;
class UOverlay;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiCutsceneSkippedSignature);

/**
 * The screen over a cutscene, after Dark Deception's UI/Main/UMG_CutsceneWidget (pak_reference_2): the cinematic bars
 * and, in the bottom right corner, PRESS P TO SKIP. Its Construct plays Start (Mat_ParameterCol's Cutscene Bars 0 -> 1
 * over a second, or straight to 1 when the scene is not a smooth transition) and listens to the sequence player it is
 * given; when the sequence finishes, Cutscene Over plays End (the bars back out) and the widget takes itself off the
 * screen.
 *
 * The player controller's keys come to it as the original's do (DD_PlayerController): any key press fades the prompt in
 * and out again (Fade, 4 s), and Skip Cutscene (P or the gamepad's right special key) plays BlackTransition, which
 * blacks the screen out over 0.15 s and there jumps the sequence to its end (Skip Cutscene), so that the cutscene ends
 * the way it would have. Show is BP_DD_Functions' Initialize Cutscene Widget: the zones' PlayCutscene calls it.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCutsceneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiCutsceneWidget(const FObjectInitializer& ObjectInitializer);

	/** AddToViewport's Z order in Create Cutscene Widget. */
	static constexpr int32 ViewportZOrder = 0;

	/** The animations' lengths, in seconds (their MovieScenes' playback ranges over 60000 ticks a second). */
	static constexpr float StartLength = 75001.f / 60000.f;
	static constexpr float EndLength = 60001.f / 60000.f;
	static constexpr float FadeLength = 240002.f / 60000.f;
	static constexpr float BlackTransitionLength = 60001.f / 60000.f;

	/** Where BlackTransition's event track jumps the sequence to its end (Skip Cutscene). */
	static constexpr float SkipAt = 9000.f / 60000.f;

	/** Skip Cutscene's JumpToSeconds: far past any sequence's end. */
	static constexpr float SkipToSeconds = 1.e7f;

	/** Mat_ParameterCol's scalar the bars read (MM_CutsceneBars). */
	static const FName BarsParameter;

	/**
	 * Initialize Cutscene Widget: creates the screen for the first player over Sequence and adds it to the viewport.
	 * Smooth transition? slides the bars in; without it they are in from the first frame. Returns it (null without a
	 * player controller).
	 */
	static UWasamiCutsceneWidget* Show(const UObject* WorldContextObject, ULevelSequencePlayer* Sequence,
		bool bInSmoothTransition);

	/** Sequence: the cutscene's player, whose OnFinished ends the screen and which Skip Cutscene jumps to the end. */
	UPROPERTY(BlueprintReadWrite, Transient, Category = "Cutscene")
	TObjectPtr<ULevelSequencePlayer> Sequence;

	/** bSmoothTransition: Start from its beginning (the bars slide in) instead of from a second in. */
	UPROPERTY(BlueprintReadWrite, Category = "Cutscene")
	bool bSmoothTransition = true;

	/** Skipped: broadcast as BlackTransition starts (nothing in the original listens). */
	UPROPERTY(BlueprintAssignable, Category = "Cutscene")
	FWasamiCutsceneSkippedSignature OnSkipped;

	/** Any Key Press: plays Fade once the screen is initialized, unless it is playing already. */
	void AnyKeyPress();

	/** Start Hold (the Skip Cutscene key): plays BlackTransition and broadcasts Skipped, unless End is playing. */
	void StartHold();

	/** Cutscene Over (the sequence's OnFinished): plays End, whose end takes the screen off. */
	UFUNCTION()
	void CutsceneOver();

	/** Moves the animations on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Initialized: set when Start ends; Any Key Press does nothing before it. */
	bool IsInitialized() const { return bInitialized; }

	/** Mat_ParameterCol's Cutscene Bars now (Start and End drive it). */
	float GetBars() const { return Bars; }

	/** HorizontalBox_98's RenderOpacity now (Fade drives it). */
	float GetPromptOpacity() const { return PromptOpacity; }

	/** Black's and Overlay_0's RenderOpacity now (BlackTransition drives them). */
	float GetBlackOpacity() const { return BlackOpacity; }
	float GetOverlayOpacity() const { return OverlayOpacity; }

	/** The animations at Seconds into them (at a play rate of 1). */
	static float EvaluateStartBars(float Seconds);
	static float EvaluateEndBars(float Seconds);
	static float EvaluateFade(float Seconds);
	static float EvaluateBlackTransition(float Seconds);
	static float EvaluateBlackTransitionOverlay(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Mat_ParameterCol, whose Cutscene Bars the post process the player controller carries reads. */
	UPROPERTY(EditDefaultsOnly, Category = "Cutscene|Assets")
	TSoftObjectPtr<UMaterialParameterCollection> ParameterCollection;

private:
	/** An animation the widget plays itself: how far into it, and whether it is playing at all. */
	struct FPlaying
	{
		bool bPlaying = false;
		float Time = 0.f;

		void Start(float At) { bPlaying = true; Time = At; }
		/** Moves on to Length at most; true on the tick it reaches the end. */
		bool Advance(float DeltaSeconds, float Length);
	};

	/** Skip Cutscene: the sequence to its end (BlackTransition's event track). */
	void SkipCutscene();

	void ApplyAnimations();
	void SetBars(float Value);

	UPROPERTY(Transient)
	TObjectPtr<UImage> Black;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> PromptOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> Prompt;

	FPlaying StartAnimation;
	FPlaying EndAnimation;
	FPlaying FadeAnimation;
	FPlaying BlackTransitionAnimation;

	bool bInitialized = false;
	bool bSkipped = false;
	float Bars = 0.f;
	float PromptOpacity = 0.f;
	float BlackOpacity = 0.f;
	float OverlayOpacity = 1.f;
};
