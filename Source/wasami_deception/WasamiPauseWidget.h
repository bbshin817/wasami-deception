#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSettingsSaveGame.h"
#include "WasamiPauseWidget.generated.h"

class UAudioComponent;
class UButton;
class UCanvasPanel;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;

/**
 * The pause menu, after Dark Deception's UI/Menu/Pause/UMG_Pause (the same tree, animations and sounds in both versions):
 * a blurred red wash, a black brush stroke down the middle, the level's monster's head above EASY MODE (shown on EASY
 * only) and RESUME / RESTART / OPTIONS / QUIT, and hidden over them a second wash and the two windows, GIVING UP? (QUIT
 * TO TITLE / QUIT TO DESKTOP / CANCEL) and RESTART? (YES / NO), with the head peeking over them. The tree is built here
 * slot for slot; the heads are this game's Wasami, tinted the originals' red. Construct pauses the game, takes the input
 * with the cursor, plays UI_Pause and the pause music and fades the whole screen in (FadeIn); RESUME fades it back out,
 * gives the input back to the game and, half a second on, unpauses and takes the screen off. Destruct fades the music
 * out. A button is white while hovered. RESTART and QUIT pop their window up over a second wash (Popup_0, Popup) and
 * NO / CANCEL put it back; YES starts the level over from its start, QUIT TO TITLE opens the title, QUIT TO DESKTOP
 * quits, and OPTIONS opens the options screen over the menu. The widget's own tick plays the animations and the Delay,
 * so it works over the paused game.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiPauseWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * The Z order Esc adds the menu at: the old version's character does CreateAndAddWidget(UMG_Pause, 5), the latest
	 * version's player controller 1.
	 */
	static constexpr int32 ViewportZOrder = 5;

	/**
	 * Esc's CreateAndAddWidget(UMG_Pause) for the first player, unless a pause menu is up already (the original's Esc
	 * cannot come through then: the menu takes the input). Returns the menu (null without a player controller).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pause", meta = (WorldContext = "WorldContextObject"))
	static UWasamiPauseWidget* Show(const UObject* WorldContextObject);

	/** The pause menu on the screen, or null. */
	static UWasamiPauseWidget* Find(const UObject* WorldContextObject);

	/**
	 * The settings EASY MODE's colour is bound to (the original reads the game mode's Global Settings Save Instance).
	 * Left empty, Construct takes the game instance's, or new defaults without one; the tests set their own.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSettingsSaveGame> Settings;

	/**
	 * Construct's run (@861): UI_Pause, FadeIn from its start, the game paused, the input to the menu with the cursor,
	 * the music faded in and the time counter paused, with InSettings (kept as Settings). NativeConstruct calls it (the
	 * tests call it alone, without a world).
	 */
	void Begin(UWasamiSettingsSaveGame* InSettings);

	/** Moves FadeIn and the Delay on by DeltaSeconds and writes the bound colours (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/**
	 * RESUME (@3654): UI_Select_V3, FadeIn backwards from its end, the input to the game without the cursor, and 0.5 s
	 * later the game unpaused and the menu off the screen.
	 */
	void PressResume();

	/** Whether RESUME has been pressed, and whether the menu has taken itself off the screen. */
	bool IsResuming() const { return bResuming; }
	bool IsFinished() const { return bFinished; }

	/** RESUME's Delay before SetGamePaused(False) and RemoveFromParent. */
	static constexpr float ResumeDelay = 0.5f;

	/** The two pop-ups: GIVING UP? (the animation Popup, on Givingupbox) and RESTART? (Popup_0, on RestartBox). */
	enum class EPopup : uint8
	{
		GivingUp,
		Restart,
	};

	/**
	 * RESTART (@3996): Popup_0 from its start, UI_Select_V3 and UI_Window_PopUp_V3, and redblock Visible (the menu under
	 * it takes no clicks).
	 */
	void PressRestart();

	/** QUIT (@4516): the same with Popup (GIVING UP?). */
	void PressQuit();

	/** RESTART?'s NO (@6080) and GIVING UP?'s CANCEL (@4709): the pop-up backwards from CloseFrom, UI_Select_V3 at 0.7, redblock HitTestInvisible. */
	void PressNo();
	void PressCancel();

	/**
	 * RESTART?'s YES, as the latest version has it (@5408): the level's entry of the save emptied and written, the
	 * shards collected forgotten, UI_Select_V3, the input to the game without the cursor, and a black fade (UMG_BlackFade_2,
	 * Speed 5, Z 10) whose end (Finish Restart, @6656) unpauses and opens the level again. The lives go back to 3 (Reset
	 * Lives; the original leaves them as they are. The user's answer of 2026-09-20).
	 */
	void PressYes();

	/** OPTIONS (@6085): UI_Select_V3 and the options screen over the menu (Z 10). */
	void PressOptions();

	/** GIVING UP?'s QUIT TO TITLE (@6207): UI_Select_V3 and the title (the save as it is). */
	void PressQuitToTitle();

	/** GIVING UP?'s QUIT TO DESKTOP (@4843): UI_Select_V3 and QuitGame. */
	void PressQuitToDesktop();

	/** Finish Restart (@6656): the fade's Animation Finished; SetGamePaused(False) and OpenLevel(the level). */
	UFUNCTION()
	void FinishRestart();

	/** Seconds into a pop-up's animation, and whether redblock takes the clicks (a pop-up is open). */
	float GetPopupTime(EPopup Popup) const;
	bool IsMenuBlocked() const;

	/** Whether YES has begun the restart, and the level opened (a name; empty for the level again) once something has. */
	bool IsRestarting() const { return bRestarting; }
	bool HasLeft() const { return bLeft; }
	const FString& GetLevelToOpen() const { return LevelToOpen; }

	/** A pop-up's length (its playback range, [0, 30001) ticks), and where NO / CANCEL play it back from. */
	static constexpr float PopupLength = 30001.f / 60000.f;
	static constexpr float CloseFrom = 0.25f;

	/** NO's and CANCEL's UI_Select_V3 pitch. */
	static constexpr float ClosePitch = 0.7f;

	/** YES's UMG_BlackFade_2: its Speed and Z order. */
	static constexpr float RestartFadeSpeed = 5.f;
	static constexpr int32 RestartFadeZOrder = 10;

	/**
	 * The pop-ups' tracks at Seconds (Popup and Popup_0 have the same keys): the window's scale 0 → 1 (0.25 s, UE's auto
	 * tangent, a little past 1, then 1 to 0.5 s) and its and CanvasPanel_3's RenderOpacity 0 → 1 (0.25 s); Blur+Red's
	 * goes 1 → 0 over the same keys, one less the window's.
	 */
	static float EvaluatePopupScale(float Seconds);
	static float EvaluatePopupOpacity(float Seconds);

	/** Seconds into FadeIn, and the opacity it gives the whole menu (CanvasPanel_0). */
	float GetFadeInTime() const { return FadeInTime; }
	float GetOpacity() const { return Opacity; }

	/** FadeIn's length (its playback range, [0, 30001) ticks). */
	static constexpr float FadeInLength = 30001.f / 60000.f;

	/** FadeIn's track at Seconds: CanvasPanel_0's RenderOpacity 0 → 1 over 0.5 s, flat at both ends. */
	static float EvaluateFadeIn(float Seconds);

	/** GetColorAndOpacity_0: EASY MODE's colour at Difficulty (the dark red on EASY, clear on NORMAL, magenta on HARD). */
	static FLinearColor EasyModeColor(EWasamiDifficulty Difficulty);

	/** The pause music's CreateSound2D FadeIn and Destruct's FadeOut, in seconds. */
	static constexpr float MusicFadeInSeconds = 1.f;
	static constexpr float MusicFadeOutSeconds = 0.5f;

	/** The menu's and the windows' buttons' content colour when not hovered (the class's Unhovered Color). */
	static constexpr float UnhoveredGrey = 0.114583001f;

	/** The heads' tint: the original heads' red, rgb(192, 0, 0). */
	static FLinearColor HeadTint();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** pause_screen_bg: the black brush stroke behind the menu (Image_152). */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<UTexture2D> StrokeTexture;

	/** This game's head above the menu (Icon), white, in place of the level's pause_*_head. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<UTexture2D> HeadTexture;

	/** This game's head peeking over the windows (window_quit_head, quitwindowhead), in place of quit_window_head_*. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<UTexture2D> PeekTexture;

	/** quit_window_frame and restart_window_frame: GIVING UP?'s and RESTART?'s windows. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<UTexture2D> QuitFrameTexture;

	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<UTexture2D> RestartFrameTexture;

	/** UI_Pause: Construct's sound. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<USoundBase> OpenSound;

	/** Pause_Sound_v1: the music while the menu is up. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<USoundBase> MusicSound;

	/** UI_Select_V3: every button's sound. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

	/** UI_Window_PopUp_V3: RESTART's and QUIT's second sound. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<USoundBase> PopUpSound;

private:
	/** A pop-up's animation: seconds into it, which way it plays, and whether it is playing. */
	struct FPopupPlay
	{
		float Time = 0.f;
		bool bReverse = false;
		bool bPlaying = false;
	};

	void BuildScreen(UCanvasPanel* Root);
	void ApplyAnimation();
	void PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const;

	/** PlayAnimation on a pop-up: from StartAt, forwards to its end or backwards to its start, over again each time. */
	void PlayPopup(EPopup Popup, float StartAt, bool bReverse);

	/** Opens and closes a pop-up as RESTART / QUIT and NO / CANCEL do (the animation, the sounds and redblock). */
	void OpenPopup(EPopup Popup);
	void ClosePopup(EPopup Popup);

	/** SetInputMode_GameOnly for the owner, and OpenLevel (the level again when LevelName is null). */
	void OpenLevel(const TCHAR* LevelName);

	/** The bound and hovered colours: EASY MODE's (GetColorAndOpacity_0) and each button's (white while hovered). */
	void RefreshColours();

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnOptionsClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnYesClicked();

	UFUNCTION()
	void OnNoClicked();

	UFUNCTION()
	void OnQuitToTitleClicked();

	UFUNCTION()
	void OnQuitToDesktopClicked();

	UFUNCTION()
	void OnCancelClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootPanel;

	/** Blur+Red (the wash under the menu), CanvasPanel_3 (the pop-ups' wash over it) and its redblock. */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> WashPanel;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> VeilPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> RedBlock;

	/** Givingupbox and RestartBox (WindowOf picks one by EPopup). */
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> GivingUpBox;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RestartBox;

	UCanvasPanel* WindowOf(EPopup Popup) const { return Popup == EPopup::GivingUp ? GivingUpBox : RestartBox; }

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EasyModeText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;

	/** Every button, whose content goes white while it is hovered and back to UnhoveredGrey after. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> HoverButtons;

	/** Construct's CreateSound2D, kept as the original's CreateSound2D_ReturnValue for Destruct. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Music;

	/** Seconds into FadeIn, which runs backwards once resuming, and the opacity it gives. */
	float FadeInTime = 0.f;
	float Opacity = 0.f;
	bool bResuming = false;
	float ResumeElapsed = 0.f;
	bool bFinished = false;

	/** The pop-ups' animations, by EPopup, and the one played last (the washes show its values; none yet: INDEX_NONE). */
	FPopupPlay Popups[2];
	int32 LastPopup = INDEX_NONE;

	bool bRestarting = false;
	bool bLeft = false;
	FString LevelToOpen;
};
