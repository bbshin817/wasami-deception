#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSettingsSaveGame.h"
#include "WasamiOptionsWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCheckBox;
class UFont;
class USlider;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWasamiGameInstance;

/**
 * The options screen, after Dark Deception's UI/Menu/UMG_Options as of v1.6.1 (pak_reference; the latest version put
 * the AutoSettings plugin's SettingsUI in its place, and the WebGL version copied this one): a blurred red wash, and
 * over it the options frame with GRAPHICS (QUALITY, RESOLUTION SCALE, BRIGHTNESS), AUDIO (MUSIC, SFX, DIALOGUE,
 * SUBTITLES), CONTROLS (MOUSE SENSITIVITY, HEAD BOBBING, INVERTED Y AXIS, TOGGLE SPRINT, MOUSE SMOOTHING) and
 * DIFFICULTY in a grid, and SAVE & EXIT / CANCEL under it, which FadeIn scales and fades in. The tree is built here
 * slot for slot, the two RESOLUTION rows the original keeps unseen included, with the style Construct gives the check
 * boxes and QUALITY's arrows. Construct takes the input with the cursor, plays FadeIn and reads the settings into the
 * controls (Setup Values); DIFFICULTY is taken off anywhere but the title. The value boxes read their sliders every
 * frame, as the original's bindings do. A slider snaps to its ten stops as it moves, the arrows step QUALITY and
 * DIFFICULTY, and SAVE & EXIT applies and saves what the screen shows (through the game instance) before closing as
 * CANCEL does: FadeIn backwards and off the screen. Esc does nothing, as in the original. The widget's own tick plays
 * FadeIn and the Delay, so it works over the paused game.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiOptionsWidget(const FObjectInitializer& ObjectInitializer);

	/** The Z order the title's and the pause menu's OPTIONS add the screen at (CreateAndAddWidget(UMG_Options, 10)). */
	static constexpr int32 ViewportZOrder = 10;

	/** CreateAndAddWidget(UMG_Options) for the first player at Z 10. Returns the screen (null without a player controller). */
	UFUNCTION(BlueprintCallable, Category = "Options", meta = (WorldContext = "WorldContextObject"))
	static UWasamiOptionsWidget* Show(const UObject* WorldContextObject);

	/**
	 * The settings Setup Values reads, which SAVE & EXIT writes into (the original's game mode's Global Settings Save
	 * Instance). Left empty, Construct takes the game instance's (UWasamiGameInstance::GetSettings), or new defaults
	 * without one; the tests set their own.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSettingsSaveGame> Settings;

	/**
	 * The game instance whose SaveSettings SAVE & EXIT calls (the original's Save Values writes the slot and the game
	 * mode's instance, and sets up the player's mouse smoothing). Left empty, Construct takes the world's, and its
	 * settings when Settings is empty too; the tests set one with a slot of their own. Without one, SAVE & EXIT only
	 * writes the screen's values into Settings.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiGameInstance> SettingsOwner;

	/** The level Construct decides DIFFICULTY by; left empty, the world's current one (the tests set it). */
	FString LevelName;

	/**
	 * Whether DIFFICULTY stays in a level: the original keeps it on its title (TitleScreen, and 00_Ballroom and TestMap)
	 * and takes it off elsewhere (the pause menu's OPTIONS); here, on the title's level (AWasamiGameMode::TitleLevelName).
	 */
	static bool ShowsDifficulty(const FString& InLevelName);

	/**
	 * Construct's run: the input to the screen with the cursor, FadeIn from its start, Setup Values with InSettings (kept
	 * as Settings), and DifficultyBox off unless bInShowDifficulty. NativeConstruct calls it (the tests call it alone).
	 */
	void Begin(UWasamiSettingsSaveGame* InSettings, bool bInShowDifficulty);

	/**
	 * Setup Values: QUALITY and DIFFICULTY into the screen's QualitySetting and Difficulty Setting, the six sliders to
	 * their values and the five check boxes to theirs.
	 */
	void SetupValues(const UWasamiSettingsSaveGame& InSettings);

	/** Moves FadeIn and the Delay on by DeltaSeconds and writes the value boxes (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/**
	 * SAVE & EXIT (ApplyButton, @19624): UI_Select_V2, the settings applied (@18949) and saved (Save Values, @19688) as
	 * the screen shows them (WriteValues, then the owner's SaveSettings), then Cancel.
	 */
	void PressSave();

	/** CANCEL (CancelButton, @16082; Cancel, @21302): FadeIn backwards from 0.25 s, UI_Select_V3 at 0.7, and off 0.3 s later. */
	void PressCancel();

	/** Save Values' writes: the screen's QualitySetting, sliders, check boxes and Difficulty Setting into InSettings. */
	void WriteValues(UWasamiSettingsSaveGame& InSettings) const;

	/** Whether the screen is closing, and whether it has taken itself off the screen. */
	bool IsClosing() const { return bClosing; }
	bool IsFinished() const { return bFinished; }

	/** Cancel plays FadeIn backwards from here, and RemoveFromParent comes this long after. */
	static constexpr float CloseFrom = 0.25f;
	static constexpr float CloseDelay = 0.3f;

	/** Cancel's UI_Select_V3 plays at this pitch. */
	static constexpr float CancelPitch = 0.7f;

	/** The screen's QualitySetting and Difficulty Setting (what the arrows step and the boxes read). */
	int32 GetQualitySetting() const { return QualitySetting; }
	EWasamiDifficulty GetDifficultySetting() const { return DifficultySetting; }

	/** Whether DifficultyBox is still in the grid. */
	bool HasDifficulty() const { return DifficultyBox != nullptr; }

	/** Seconds into FadeIn. */
	float GetFadeInTime() const { return FadeInTime; }

	/** CanvasPanel_2's (the frame and its contents') opacity and scale, and Blur+Red's opacity. */
	float GetBoxOpacity() const { return BoxOpacity; }
	float GetBoxScale() const { return BoxScale; }
	float GetWashOpacity() const { return WashOpacity; }

	/** CanvasPanel_0's render scale in the tree. */
	static constexpr float RootScale = 1.015f;

	/** FadeIn's length (its playback range, [0, 30001) ticks). */
	static constexpr float FadeInLength = 30001.f / 60000.f;

	/** FadeIn's tracks at Seconds: Blur+Red's and CanvasPanel_2's RenderOpacity (the same keys), and CanvasPanel_2's scale. */
	static float EvaluateFadeInOpacity(float Seconds);
	static float EvaluateFadeInScale(float Seconds);

	/** Construct's tint on the check boxes' hovered pictures (the pressed and unhovered ones stay white). */
	static constexpr float CheckHoverGrey = 0.515625f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** options_window_frame: the frame, with OPTIONS written on it. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> FrameTexture;

	/** selection_bar: the value boxes. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> ValueBoxTexture;

	/** selection_bar_arrow_normal and _hover: the arrows, the left ones turned round. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> ArrowTexture;

	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> ArrowHoverTexture;

	/** slider_bar_tab: the sliders' thumb. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> ThumbTexture;

	/** checkbox_icon and checkbox_icon_checked. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> CheckTexture;

	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UTexture2D> CheckedTexture;

	/** helvetica-neue-bold_Font: the headings, the value boxes and SAVE & EXIT / CANCEL (the labels are in the text block's default font). */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<UFont> Font;

	/** UI_Select_V2: SAVE & EXIT. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<USoundBase> SaveSound;

	/** UI_Select_V3: Cancel, at CancelPitch. */
	UPROPERTY(EditAnywhere, Category = "Options|Assets")
	TSoftObjectPtr<USoundBase> CancelSound;

private:
	void BuildScreen(UCanvasPanel* Root);
	void BindControls();
	void ApplyAnimation();
	void PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const;

	/** A slider's OnValueChanged: SetValue(GridSnap_Float(Value, 1/9)). Setup Values' own SetValue is let be. */
	void SnapSlider(USlider* Slider, float Value);

	UFUNCTION()
	void OnResolutionScaleChanged(float Value);

	UFUNCTION()
	void OnBrightnessChanged(float Value);

	UFUNCTION()
	void OnMusicChanged(float Value);

	UFUNCTION()
	void OnSFXChanged(float Value);

	UFUNCTION()
	void OnDialogueChanged(float Value);

	UFUNCTION()
	void OnMouseSensitivityChanged(float Value);

	UFUNCTION()
	void OnQualityLowerClicked();

	UFUNCTION()
	void OnQualityHigherClicked();

	UFUNCTION()
	void OnDifficultyLowerClicked();

	UFUNCTION()
	void OnDifficultyHigherClicked();

	UFUNCTION()
	void OnApplyClicked();

	UFUNCTION()
	void OnCancelClicked();

	UFUNCTION()
	void OnApplyHovered();

	UFUNCTION()
	void OnApplyUnhovered();

	UFUNCTION()
	void OnCancelHovered();

	UFUNCTION()
	void OnCancelUnhovered();

	/** The bound texts: the sliders' values (SliderText), Quality Text and the DIFFICULTY's. */
	void RefreshTexts();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Wash;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Box;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DifficultyBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(Transient)
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> BrightnessSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MusicSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SFXSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> DialogueSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> SubtitlesCheck;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> HeadBobbingCheck;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> InvertedYCheck;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> ToggleSprintCheck;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> MouseSmoothingCheck;

	// The value boxes' texts, bound as in the original: QUALITY's, the sliders' and the DIFFICULTY's.
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QualityText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResolutionScaleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BrightnessText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MusicText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SFXText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MouseSensitivityText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyText;

	/** QualitySetting (the class's default 2) and Difficulty Setting. */
	int32 QualitySetting = 2;
	EWasamiDifficulty DifficultySetting = EWasamiDifficulty::Normal;

	/** Whether Setup Values is setting the sliders (their OnValueChanged lets them be). */
	bool bSettingUp = false;

	/** Seconds into FadeIn, and the values it moves (kept once it ends). It runs backwards once closing. */
	float FadeInTime = 0.f;
	bool bClosing = false;
	float CloseElapsed = 0.f;
	bool bFinished = false;
	float BoxOpacity = 1.f;
	float BoxScale = 1.f;
	float WashOpacity = 0.f;
};
