#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiPopUpWidget.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiPopUpClickSignature);

/**
 * A YES / NO question, after Dark Deception's UI/Main/TitleScreen/UMG_PopUp (pak_reference_2): a blurred red wash over
 * the screen, and in the middle one of the pause menu's window frames with the question and the two buttons, which its
 * animation Popup fades and scales in. YES tells whoever asked (YesClick) and leaves the closing to them (Close
 * Animation); NO closes it by itself. The tree is built here as in the original, slot for slot, and the widget's own
 * tick plays Popup and the Delay, so it works over the paused game.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiPopUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiPopUpWidget(const FObjectInitializer& ObjectInitializer);

	/** The original's Frame values: which window frame Construct puts behind the question. */
	static constexpr uint8 RestartFrame = 0;
	static constexpr uint8 QuitFrame = 1;
	static constexpr uint8 BlankFrame = 2;

	/**
	 * Create(UMG_PopUp) for the first player with Text and Frame set, added at ZOrder (the death screen's RESTART adds it
	 * at 5, its LAST CHECKPOINT at 10). Returns the pop-up (null without a player controller).
	 */
	UFUNCTION(BlueprintCallable, Category = "PopUp", meta = (WorldContext = "WorldContextObject"))
	static UWasamiPopUpWidget* Show(const UObject* WorldContextObject, const FText& InText, uint8 InFrame, int32 ZOrder);

	/** The question (RichTextBlock_374's text). Set before the pop-up is added. */
	UPROPERTY(BlueprintReadWrite, Category = "PopUp")
	FText Text;

	/** Frame: RestartFrame, QuitFrame or BlankFrame. Set before the pop-up is added. */
	UPROPERTY(BlueprintReadWrite, Category = "PopUp")
	uint8 Frame = RestartFrame;

	/** YesClick: YES was clicked (the pop-up stays until Close Animation). */
	UPROPERTY(BlueprintAssignable, Category = "PopUp")
	FWasamiPopUpClickSignature OnYesClick;

	/** NoClick: NO was pressed, and the pop-up is closing. */
	UPROPERTY(BlueprintAssignable, Category = "PopUp")
	FWasamiPopUpClickSignature OnNoClick;

	/** Close Animation: Popup backwards from 0.25 s, and off the screen 0.25 s later. */
	UFUNCTION(BlueprintCallable, Category = "PopUp")
	void CloseAnimation();

	/** Press No: NO's OnPressed (the closing, the select sound at 0.7 and NoClick). */
	UFUNCTION(BlueprintCallable, Category = "PopUp")
	void PressNo();

	/** Popup's length (its playback range, [0, 30001) ticks), and where closing plays it back from. */
	static constexpr float PopupLength = 30001.f / 60000.f;
	static constexpr float CloseFrom = 0.25f;
	/** The Delay after closing begins before RemoveFromParent. */
	static constexpr float CloseDelay = 0.25f;
	/** NO's select sound plays at this pitch. */
	static constexpr float NoPitch = 0.7f;

	/** Moves Popup and the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Whether the pop-up is closing, and whether it has taken itself off the screen. */
	bool IsClosing() const { return bClosing; }
	bool IsFinished() const { return bFinished; }

	/** Givingupbox's (the frame and its contents') opacity and scale, and CanvasPanel_3's (the wash's) opacity. */
	float GetBoxOpacity() const { return BoxOpacity; }
	float GetBoxScale() const { return BoxScale; }

	/** Popup's tracks at Seconds: the two RenderOpacity tracks share their keys; the scale is Givingupbox's. */
	static float EvaluateOpacity(float Seconds);
	static float EvaluateScale(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** The frames Construct picks from by Frame: restart_window_frame_2, quit_window_frame, blank_window_frame. */
	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<UTexture2D> RestartFrameTexture;

	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<UTexture2D> QuitFrameTexture;

	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<UTexture2D> BlankFrameTexture;

	/** helvetica-normal_Font: PopupText's Default row, the question's style. */
	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<UFont> QuestionFont;

	/** UI_Window_PopUp_V3: Construct. */
	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<USoundBase> PopUpSound;

	/** UI_Select_V3: YES, and NO at 0.7. */
	UPROPERTY(EditAnywhere, Category = "PopUp|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

private:
	UFUNCTION()
	void OnYesClicked();

	UFUNCTION()
	void OnNoPressed();

	UFUNCTION()
	void OnYesHovered();

	UFUNCTION()
	void OnYesUnhovered();

	UFUNCTION()
	void OnNoHovered();

	UFUNCTION()
	void OnNoUnhovered();

	/** Popup backwards from CloseFrom, and the Delay to RemoveFromParent. */
	void StartClosing();
	void ApplyAnimation();
	void PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Wash;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Box;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Question;

	UPROPERTY(Transient)
	TObjectPtr<UButton> YesButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NoButton;

	/** Seconds into Popup; it runs forwards from Construct and backwards once closing. */
	float PopupTime = 0.f;
	bool bClosing = false;
	float CloseElapsed = 0.f;
	bool bFinished = false;
	float BoxOpacity = 0.f;
	float BoxScale = 0.f;
};
