#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WasamiSettingsSaveGame.generated.h"

/** Dark Deception's ENUM_DifficultySettings: NewEnumerator0 to 2, EASY / NORMAL / HARD on the OPTIONS. */
UENUM(BlueprintType)
enum class EWasamiDifficulty : uint8
{
	Easy,
	Normal,
	Hard
};

/**
 * The player's settings, after Dark Deception's BP_DD_Settings_SaveGame (pak_reference, slot Settings) as its OPTIONS
 * (UMG_Options, v1.6.1) sets them, with the defaults of its class. Crosshair, which the menu does not show and nothing
 * here reads, is left out. Also the rules the OPTIONS and the game mode's Set Settings go by: the sliders' snap and
 * text, the arrows' steps and names, the gamma the brightness gives, and what the player takes from them.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The original's slot name. */
	static const FString SlotName;

	static constexpr int32 UserIndex = 0;

	/**
	 * The game mode's Check Settings Save: the save in Slot, or a new one (the defaults) written there when there is none
	 * to read.
	 */
	static UWasamiSettingsSaveGame* Check(const FString& Slot);

	/**
	 * The game mode's Set Settings: the scalability at Quality (with the view distance at 3 and the post-processing at
	 * PostProcessingQualityFor), the resolution scale × 100, applied without a check; the subtitles; the display gamma
	 * (GammaFor); and MUSIC, SFX and DIALOGUE as the volumes of DD_SoundMix's Music, SFX and Dialogue classes
	 * (SetSoundMixClassOverride over 1 s, not to their children: SFX leaves SFX_UI and SFX_Movies as they are, as in the
	 * original) on WorldContextObject's audio device. In the editor the first part is skipped: UE's scalability is the
	 * editor's own too, and its settings would be written into the editor's GameUserSettings.ini
	 * (.claude/guides/performance.md).
	 */
	void Apply(const UObject* WorldContextObject) const;

	/**
	 * DD_SoundMix (pak_reference's Audio/SoundMix, rebuilt by the pipeline's import_dd_sound_classes): the base sound mix
	 * BP_DD_GameMode's BeginPlay sets, whose classes Set Settings overrides.
	 */
	static class USoundMix* LoadSoundMix();

	/** The sound classes the MUSIC, SFX and DIALOGUE sliders set. */
	static class USoundClass* LoadMusicClass();
	static class USoundClass* LoadSFXClass();
	static class USoundClass* LoadDialogueClass();

	/** QUALITY: 0 to 3, LOW / MEDIUM / HIGH / VERY HIGH, the level of UE's overall scalability. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 Quality = 2;

	/** RESOLUTION SCALE: 0 to 1, UE's resolution scale / 100. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float ResolutionScale = 1.f;

	/** BRIGHTNESS: 0 to 1, a display gamma of 1.8 to 2.2. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float Brightness = 1.f;

	/** MUSIC, SFX and DIALOGUE: 0 to 1, the volumes of DD_SoundMix's classes. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float Music = 1.f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float SFX = 1.f;

	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float Dialogue = 1.f;

	/** SUBTITLES. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bSubtitles = true;

	/** MOUSE SENSITIVITY: 0 to 1; the player's multiplier on the mouse is PlayerSensitivityFor it. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 0.5f;

	/** HEAD BOBBING. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bHeadBobbing = true;

	/** MOUSE SMOOTHING: the camera's rotation lag, RotationLagSpeedFor it. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bMouseSmoothing = true;

	/** INVERTED Y AXIS (not in the class's defaults: false). */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bInvertedYAxis = false;

	/** TOGGLE SPRINT (not in the class's defaults: false). */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bToggleSprint = false;

	/** DIFFICULTY. */
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EWasamiDifficulty Difficulty = EWasamiDifficulty::Normal;

	/** The sliders' grid: each change snaps to it (GridSnap_Float), so a slider has ten stops. */
	static constexpr float SliderGrid = 1.f / 9.f;

	/** GridSnap_Float(Value, 1/9), within the slider's 0 to 1. */
	static float Snap(float Value);

	/**
	 * A slider's value box: RoundFloatDecimals(Value, 1) (MoreporkFunctions: Round(Value × 10) / 10) as Conv_FloatToText
	 * writes it (up to 3 decimals, none for a whole number). The ten stops read 0, 0.1 … 0.4, 0.6 … 0.9, 1.
	 */
	static FText SliderText(float Value);

	/** The arrows' Clamp: QUALITY 0 to 3, DIFFICULTY 0 to 1 (HARD is out of reach on the original's menu too). */
	static constexpr int32 QualityMax = 3;
	static constexpr int32 DifficultyMax = 1;

	/** An arrow: Clamp(Value + Delta, 0, Max). */
	static int32 StepValue(int32 Value, int32 Delta, int32 Max);

	/** Quality Text: LOW, MEDIUM, HIGH, VERY HIGH. */
	static FText QualityText(int32 Level);

	/** The DIFFICULTY's text: EASY, NORMAL, HARD. */
	static FText DifficultyText(EWasamiDifficulty Level);

	/** Set Settings' gamma: MapRangeClamped(Brightness, 0, 1, 1.8, 2.2). */
	static float GammaFor(float Brightness);

	/** Set Settings' post-processing quality: 2 for a Quality of 0 to 2, 3 for 3. */
	static int32 PostProcessingQualityFor(int32 Quality);

	/**
	 * The player's MouseSensitivity for the setting: the setting / 0.5. The original multiplies the mouse by the setting
	 * itself, but this game's view speed (0.175° a count) was matched to the latest version played at its default,
	 * which the older menu's default of 0.5 stands for.
	 */
	static float PlayerSensitivityFor(float Setting);

	/** Set Up Mouse Smoothing: the spring arm's CameraRotationLagSpeed, 12.5 with the smoothing and 50 without. */
	static float RotationLagSpeedFor(bool bSmoothing);
};
