#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiTitleScreenWidget.generated.h"

class UAudioComponent;
class UBorder;
class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class UMaterialInterface;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWasamiSaveGame;

/**
 * The title screen, after Dark Deception's UI/Main/TitleScreen/UMG_TitleScreen as of v1.6.1 (pak_reference; the WebGL
 * version copied the same): black, the face on the right, the smoke's black and the panning brush strokes on the left,
 * the logo with its glow, the notice, the menu (RESUME / NEW GAME / OPTIONS / QUIT, the red brush under the one
 * hovered), and over them the black cover Slideshow lifts, the black and red FadeOut plays with and the version. The
 * tree is built here slot for slot, without the hidden video and slideshow and the chapters, replays and extras this
 * game has none of. Construct (the cover, the input, the music) runs from NativeConstruct; the animations and their
 * sounds are ticked by the widget.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiTitleScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiTitleScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** The Z order the TitleScreen level adds the screen at. */
	static constexpr int32 ViewportZOrder = 1;

	/** What the TitleScreen level's BeginPlay does with the screen: creates it for the first player and adds it at Z 1. */
	UFUNCTION(BlueprintCallable, Category = "Title", meta = (WorldContext = "WorldContextObject"))
	static UWasamiTitleScreenWidget* Show(const UObject* WorldContextObject);

	/** The save Construct reads (the original's SaveSlot; this game's one save); the tests read elsewhere. */
	FString SaveSlotName;

	/**
	 * Whether RESUME stays: a game was begun and not ended. The original takes it off while the save's New Game? is set
	 * and Progress is under 2; here Zone 1 writes its arrival's checkpoint 4 as it opens, and NEW GAME, RESTART and the
	 * level clear's NEXT empty the hospital's entry.
	 */
	static bool HasProgress(const UWasamiSaveGame* Save);

	/** TextBlock_0's text: 'v' and the project's version (Project Settings' ProjectVersion). */
	static FText VersionText();

	/**
	 * Construct's run: RESUME off without progress, Slideshow, the input to the screen with the cursor, and the music
	 * (Pause_Sound_v1 at pitch 0.5, fading in over 2 s to 0.5). NativeConstruct calls it with the save's progress (the
	 * tests call it alone).
	 */
	void Begin(bool bInHasProgress);

	/** Moves the animations and their sounds on by DeltaSeconds. */
	void Advance(float DeltaSeconds);

	/**
	 * FadeOut (NEW GAME's): the black over 3.75 s, the screen's pulse and the red flash from 0.15 s, Start_New_Game from
	 * the start with its volume curve and Bierce's line at 1.65 s.
	 */
	void PlayFadeOut();

	/** FadeOut_0 (RESUME's): the black over 3.75 s, nothing else. */
	void PlayFadeOut0();

	/** Music.FadeOut(Seconds). */
	void FadeOutMusic(float Seconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the menu keeps RESUME. */
	bool HasResume() const { return ResumeButton != nullptr; }

	/** Image_128's opacity (Slideshow's cover), Image_0's (the black), Image_2's (the red) and CanvasPanel_0's scale. */
	float GetCoverOpacity() const { return CoverOpacity; }
	float GetBlackOpacity() const { return BlackOpacity; }
	float GetRedOpacity() const { return RedOpacity; }
	float GetPulseScale() const { return PulseScale; }

	/** Start_New_Game's volume now (FadeOut's curve; 0 before FadeOut), and whether Bierce's line was played. */
	float GetStartVolume() const { return StartVolume; }
	bool HasPlayedVoice() const { return bVoicePlayed; }

	// The animations' lengths (their playback ranges, at 60000 ticks a second).
	static constexpr float SlideshowLength = 2.5f;
	static constexpr float FadeOut0Length = 3.75f;
	/** FadeOut runs as long as its audio section (Start_New_Game's length); its pictures end at 3.75 s. */
	static constexpr float FadeOutLength = 565580.f / 60000.f;
	/** Bierce_Title_Modified_03's section starts at 98999 ticks. */
	static constexpr float VoiceTime = 98999.f / 60000.f;

	// Construct's music.
	static constexpr float MusicVolume = 1.f;
	static constexpr float MusicPitch = 0.5f;
	static constexpr float MusicFadeInSeconds = 2.f;
	static constexpr float MusicFadeInLevel = 0.5f;

	// The animations' curves at Seconds into them.
	static float EvaluateSlideshow(float Seconds);
	/** Image_0's opacity in FadeOut and FadeOut_0 (the same keys). */
	static float EvaluateBlack(float Seconds);
	static float EvaluatePulse(float Seconds);
	static float EvaluateRed(float Seconds);
	static float EvaluateStartVolume(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** title_screen_video_mask: the smoke's black. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UTexture2D> VideoMaskTexture;

	/** MM_TitleScreen_Mask_Grey: the panning brush strokes. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UMaterialInterface> StrokesMaterial;

	/** title_screen_selection_marker: the red brush under the button hovered or pressed. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UTexture2D> MarkerTexture;

	/** This game's logo, its glow and Wasami's face (item 17's step 1). */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UTexture2D> LogoTexture;

	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UTexture2D> LogoGlowTexture;

	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UTexture2D> FaceTexture;

	/** helvetica-normal_Font: the menu, the notice and the version. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<UFont> MenuFont;

	/** Pause_Sound_v1: the music. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<USoundBase> MusicSound;

	/** Start_New_Game and Bierce_Title_Modified_03: FadeOut's audio track. */
	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<USoundBase> StartSound;

	UPROPERTY(EditAnywhere, Category = "Title|Assets")
	TSoftObjectPtr<USoundBase> VoiceSound;

private:
	void BuildScreen(UCanvasPanel* Root);
	void ApplyAnimations();

	// The buttons' hover: the text white, and back to Unhovered Color.
	UFUNCTION()
	void OnResumeHovered();

	UFUNCTION()
	void OnResumeUnhovered();

	UFUNCTION()
	void OnNewGameHovered();

	UFUNCTION()
	void OnNewGameUnhovered();

	UFUNCTION()
	void OnOptionsHovered();

	UFUNCTION()
	void OnOptionsUnhovered();

	UFUNCTION()
	void OnQuitHovered();

	UFUNCTION()
	void OnQuitUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	/** Image_128: the black cover Slideshow lifts. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Cover;

	/** Image_0: the black FadeOut and FadeOut_0 bring in. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Black;

	/** Image_2: FadeOut's red flash. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Red;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OptionsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResumeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NewGameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OptionsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuitText;

	/** Construct's CreateSound2D, kept as the original's Music. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Music;

	/** FadeOut's Start_New_Game, whose volume the section's curve moves. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> StartAudio;

	float Elapsed = 0.f;
	/** Construct's DoOnce. */
	bool bConstructed = false;

	/** Seconds into each animation; negative when it is not playing. */
	float SlideshowTime = -1.f;
	float FadeOutTime = -1.f;
	float FadeOut0Time = -1.f;
	bool bVoicePlayed = false;

	// The tree's values, which the animations move and keep once they end.
	float CoverOpacity = 0.f;
	float BlackOpacity = 0.f;
	float RedOpacity = 0.f;
	float PulseScale = 1.f;
	float StartVolume = 0.f;
};
