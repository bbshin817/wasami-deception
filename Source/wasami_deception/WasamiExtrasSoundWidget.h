#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiExtrasSoundWidget.generated.h"

class UAudioComponent;
class UButton;
class UFont;
class UImage;
class UProgressBar;
class USoundBase;
class USoundWave;
class UTextBlock;
class UTexture2D;
class UWasamiSaveGame;

/**
 * The player under a list of sounds on the extras screen (the Bierce Diaries' and the Sound Archive's), after Dark
 * Deception's UI/Main/TitleScreen/UMG_Extras_Sound_Bar (pak_reference_2): the sound's name above (upper case,
 * helvetica-normal 30), a dark red bar filling as it plays over a darker one, the time played on the left and the
 * sound's length on the right. Set Sound stops what played and plays the new sound as a UI sound, which reports how far
 * it has played; Pause and Resume pause it. The texts and the bar are the original's bindings, read each tick.
 *
 * The tree is built here as in the original, slot for slot.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiExtrasSoundBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiExtrasSoundBarWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Set Sound (@193): Sound.Stop(), Text kept, CreateSound2D(NewSound, 1, 1, 0, None, False, True) kept as Sound (not
	 * kept across levels, gone when it ends), its OnAudioPlaybackPercent bound to CustomEvent_0, and Play(0).
	 */
	void SetSound(USoundBase* NewSound, const FText& InText);

	/** Pause (@455) and Resume (@493): Sound.SetPaused(True / False). */
	void Pause();
	void Resume();

	/** Sound.Stop(): the extras screen's Check If Playing stops a playing button's bar's sound. */
	void StopSound();

	/** CustomEvent_0 (@142): the playing sound's report, kept as Playback Percent and Sound Wave. */
	UFUNCTION()
	void OnPlaybackPercent(const USoundWave* PlayingSoundWave, const float PlaybackPercent);

	/** Sound: the sound playing (null before Set Sound, and after it ends). */
	UAudioComponent* GetSound() const { return Sound; }

	/** Playback Percent and Sound Wave, as the sound last reported them. */
	float GetPlaybackPercent() const { return PlaybackPercent; }
	const USoundWave* GetSoundWave() const { return SoundWave.Get(); }

	/** Text: the sound's name as Set Sound was given it. */
	const FText& GetText() const { return Text; }

	/** GetText_0 (TextBlock_358's binding): Sound Wave's length as minutes:seconds. */
	FText GetDurationText() const;

	/** GetText_1 (TextBlock_357's): the time played (the length × Playback Percent) alike. */
	FText GetElapsedText() const;

	/** GetText_2 (TextBlock_202's): TextToUpper(Text). */
	FText GetTitleText() const;

	/**
	 * FromSeconds → BreakTimespan → the minutes (one digit at least) and the seconds (two), joined by ':' (a timespan's
	 * minutes and seconds, each within its unit, the part of a second dropped).
	 */
	static FText FormatTime(float Seconds);

	/** ProgressBar_316 (the ground), ProgressBar_315 (the bar played) and the three texts, for the tests. */
	UProgressBar* GetGroundBar() const { return GroundBar; }
	UProgressBar* GetPlayedBar() const { return PlayedBar; }
	UTextBlock* GetTitleBlock() const { return TitleBlock; }
	UTextBlock* GetElapsedBlock() const { return ElapsedBlock; }
	UTextBlock* GetDurationBlock() const { return DurationBlock; }

	/** Writes the bindings (NativeTick calls it; so does each change). */
	void ApplyBindings();

	/** helvetica-normal_Font and the engine's WhiteSquareTexture (the bars). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> NormalFont;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> WhiteTexture;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> GroundBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> PlayedBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ElapsedBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DurationBlock;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Sound;

	/** The wave the sound last reported (its sound holds it). */
	TWeakObjectPtr<const USoundWave> SoundWave;

	FText Text;
	float PlaybackPercent = 0.f;
};

/**
 * A sound on the extras screen, after UMG_Extras_Sound_Button (pak_reference_2): a 256 dark red square with a black one
 * inside it, the play icon in the middle and DIARY n or SOUND n under it, red while hovered. A diary is unlocked while its
 * level has no rank (the save's Level Ranks[ID] is None), a sound while its ID is in Extras_SFX; a locked one shows the
 * lock without the label and lets the pointer through. Pressing plays the sound on the bar (Set Sound), pauses it or
 * resumes it, and the first press after a Deselect deselects every other sound button and turns this one white with the
 * pause icon.
 *
 * The tree is built here as in the original, slot for slot. The extras screen sets ID, Text, Sound, the bar and Diary?
 * (and may hand over the save) before it adds the button.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiExtrasSoundButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiExtrasSoundButtonWidget(const FObjectInitializer& ObjectInitializer);

	/** ID: the sound's place in its list (Level Ranks' for a diary, Extras_SFX's for a sound). */
	UPROPERTY(Transient)
	int32 ID = 0;

	/** Text: the sound's name, which the bar shows. */
	UPROPERTY(Transient)
	FText Text;

	/** Sound: what the bar plays. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> Sound;

	/** Sound Bar: the list's player. */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiExtrasSoundBarWidget> SoundBar;

	/** Diary?: a Bierce diary (unlocked by Level Ranks) rather than a sound (by Extras_SFX). The class's default is true. */
	UPROPERTY(Transient)
	bool bDiary = true;

	/** The save the lock is read from; Construct reads SaveSlotName's when none is given (a new one when it is empty). */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSaveGame> Save;

	FString SaveSlotName;

	/**
	 * Level Ranks' length in a new BP_DD_SaveGame (nine levels, all None). This game's save keeps no ranks (the level
	 * clear screen does not write the original's, 13 record), so a diary within it is unlocked and any past it is not
	 * (Array_Get past the end gives 0, the C rank).
	 */
	static constexpr int32 LevelRankCount = 9;

	/** The select sound's pitch on a press. */
	static constexpr float SelectPitch = 1.5f;

	/** The colours: at rest (dark red), hovered (red) and selected (white). */
	static const FLinearColor RestColour;
	static const FLinearColor HoverColour;
	static const FLinearColor SelectedColour;

	/**
	 * PreConstruct and Construct (@950): the save read; a locked sound's Button_0 HitTestInvisible, TextBlock_131 off its
	 * parent and Buton the lock at its own size. NativeConstruct calls it (the tests call it alone, without a world).
	 */
	void Begin();

	/** The click (@2043); see the class. */
	void Press();

	/**
	 * Deselect (@2409): for an unlocked sound, the first-press gate opened again, Paused?, Playing? and Selected? off,
	 * the play icon and the colours at rest.
	 */
	void Deselect();

	/** The hover (@1485: red) and unhover (@1764: dark red), both left alone while selected. */
	void Hover(bool bHovered);

	/** Whether the save has this sound unlocked (Begin reads the save). */
	bool IsUnlocked() const;

	/** GetText_0 (TextBlock_131's binding): DIARY or SOUND and ID + 1. */
	FText GetLabelText() const;

	bool IsPlaying() const { return bPlaying; }
	bool IsPaused() const { return bPaused; }
	bool IsSelected() const { return bSelected; }

	/** Button_0 (the square pressed), Button_1 (the black inside), Buton (the icon) and TextBlock_131, for the tests. */
	UButton* GetButton() const { return Button; }
	UButton* GetInner() const { return Inner; }
	UImage* GetIcon() const { return Icon; }
	UTextBlock* GetLabel() const { return Label; }

	/** extras_play_icon, extras_pause_icon, locked_-_Copy (the lock), WhiteSquareTexture and helvetica-normal_Font. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> PlayTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> PauseTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> LockedTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> WhiteTexture;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> NormalFont;

	/** UI_Select_V3. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void SetColours(const FLinearColor& Colour);
	void SetIcon(const TSoftObjectPtr<UTexture2D>& Texture);

	UFUNCTION()
	void OnButtonClicked();

	UFUNCTION()
	void OnButtonHovered();

	UFUNCTION()
	void OnButtonUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Inner;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Icon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	bool bPaused = false;
	bool bPlaying = false;
	bool bSelected = false;

	/** The press's DoOnce: closed by the first press, opened again by Deselect. */
	bool bSelectGateClosed = false;
};
