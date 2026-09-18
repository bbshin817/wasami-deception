#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiRingPieceWidget.generated.h"

class UBackgroundBlur;
class UBorder;
class UButton;
class UFont;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiRingPieceCloseSignature);

/**
 * The ring piece's screen, after Dark Deception's UI/01/UMG_01_RingPieceCollect (pak_reference_2): the game paused under
 * a blur and a black veil, the ring piece spinning in large in the middle, and CLOSE below it. Construct plays
 * NewAnimation_1 (the blur, the veil and the piece coming in), pauses the game, shows the cursor and gives the input to
 * the screen; 0.2 s on NewAnimation_2 brings CLOSE up and the time counter stops. CLOSE (a DoOnce) plays both back,
 * gives the input back to the game, unpauses it, broadcasts Close and takes the screen off; Destruct lets the time
 * counter run again.
 *
 * The tree is built here as in the original, slot for slot, and the widget's own tick plays the animations and counts
 * the delays, so they go on while the game is paused under it. The original's Virtual Cursor (for a gamepad) is not
 * made, as on the death screen.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiRingPieceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiRingPieceWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * What the zone's Collected Ring Piece does with the screen: Create(Self, UMG_01_RingPieceCollect, None) and
	 * AddToViewport(0) (left off the screen where there is none, as in a test's world). Returns the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ring Piece", meta = (WorldContext = "WorldContextObject"))
	static UWasamiRingPieceWidget* Show(const UObject* WorldContextObject);

	/** Close: CLOSE was clicked and the screen has gone (the zone's Ring Piece Collect). */
	UPROPERTY(BlueprintAssignable, Category = "Ring Piece")
	FWasamiRingPieceCloseSignature OnClose;

	/** ringpiece_texture: the piece's picture, which Construct puts in ringpiece (T_RingPiece_1 by default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring Piece")
	TSoftObjectPtr<UTexture2D> RingPieceTexture;

	/** Both animations' lengths (their playback ranges, [0, 30001) ticks). */
	static constexpr float AnimationLength = 30001.f / 60000.f;
	/** Construct's Delay before NewAnimation_2 (CLOSE) and the time counter's stop. */
	static constexpr float CloseButtonDelay = 0.2f;
	/** CLOSE's Delays: after NewAnimation_2 back, before NewAnimation_1 back; then before the screen goes. */
	static constexpr float ReverseDelay = 0.2f;
	static constexpr float LeaveDelay = 0.5f;

	/** Construct's run from its start (the tests call it alone; NativeConstruct calls it with the player's side). */
	void Begin();

	/** Moves the delays and the animations on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** CLOSE's click (the button's OnClicked): a DoOnce. */
	void PressClose();

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether CLOSE was pressed (the DoOnce closed). */
	bool IsClosing() const { return bClosing; }

	/** Whether the screen is done: Close broadcast and the screen taken off. */
	bool HasClosed() const { return bClosed; }

	UImage* GetRingPiece() const { return RingPiece; }
	UButton* GetCloseButton() const { return CloseButton; }
	UBorder* GetVeil() const { return Veil; }
	UBackgroundBlur* GetBlur() const { return Blur; }

	// NewAnimation_1's tracks at Seconds into it: ringpiece's scale, angle and opacity, BackgroundBlur_0's strength and
	// Button_20's BackgroundColor alpha.
	static float EvaluatePieceScale(float Seconds);
	static float EvaluatePieceAngle(float Seconds);
	static float EvaluatePieceOpacity(float Seconds);
	static float EvaluateBlurStrength(float Seconds);
	static float EvaluateVeilAlpha(float Seconds);
	/** NewAnimation_2's track: Button_107's scale. */
	static float EvaluateCloseScale(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** helvetica-neue-bold_Font: CLOSE. */
	UPROPERTY(EditAnywhere, Category = "Ring Piece|Assets")
	TSoftObjectPtr<UFont> CloseFont;

private:
	/** What a Delay resumes. */
	enum class EStep : uint8
	{
		/** Construct's: NewAnimation_2 and Pause Time Counter. */
		ShowClose,
		/** CLOSE's first: NewAnimation_1 back. */
		ReverseIn,
		/** CLOSE's second: the input back, unpaused, Close, off the screen. */
		Leave,
	};

	struct FPendingStep
	{
		float At;
		EStep Step;
	};

	void Schedule(EStep Step, float Delay);
	void RunStep(EStep Step);
	void ApplyAnimations();

	UFUNCTION()
	void OnCloseClicked();

	// CLOSE's hover: white, and back to the tree's grey.
	UFUNCTION()
	void OnCloseHovered();

	UFUNCTION()
	void OnCloseUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> Blur;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Veil;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RingPiece;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	TArray<FPendingStep> Pending;
	float Elapsed = 0.f;
	/** When the step being run was due (its Delays count from there). */
	float StepTime = 0.f;
	/** Seconds into each animation; negative when it has not played. Played back, they count down. */
	float InTime = -1.f;
	float CloseTime = -1.f;
	bool bInReverse = false;
	bool bCloseReverse = false;
	bool bClosing = false;
	bool bClosed = false;
};
