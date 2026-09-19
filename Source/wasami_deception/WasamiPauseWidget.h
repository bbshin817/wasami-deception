#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSettingsSaveGame.h"
#include "WasamiPauseWidget.generated.h"

class UAudioComponent;
class UButton;
class UCanvasPanel;
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
 * out. A button is white while hovered. The widget's own tick plays FadeIn and the Delay, so it works over the paused
 * game.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiPauseWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * The Z order Esc adds the menu at: the old version's character does CreateAndAddWidget(UMG_Pause, 5), the latest
	 * version's player controller 1. 5 draws it over the death screen (added at 5 before it), which EASY leaves up with
	 * no way out but this menu.
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

	/** UI_Select_V3: RESUME's sound. */
	UPROPERTY(EditAnywhere, Category = "Pause|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

private:
	void BuildScreen(UCanvasPanel* Root);
	void ApplyAnimation();
	void PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const;

	/** The bound and hovered colours: EASY MODE's (GetColorAndOpacity_0) and each button's (white while hovered). */
	void RefreshColours();

	UFUNCTION()
	void OnResumeClicked();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootPanel;

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
};
