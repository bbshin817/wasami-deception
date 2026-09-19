#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiLevelResults.h"
#include "WasamiLevelClearWidget.generated.h"

class UAudioComponent;
class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWasamiLevelClearFinishedSignature);

/**
 * The level clear screen, after Dark Deception's UI/Menu/UMG_LevelClear (pak_reference, as the WebGL version follows
 * it), which the hospital's Escape adds to the viewport (Z 6) with the results filled in. Its Construct plays
 * ClearAnimation (4.39 s): the screen fades to black and red, You Escaped! turns and lands with a white flash and
 * UI_YouEscaped at 0.75 s, the red leaves from 2.75 s as RESULTS comes in, and at 3.25 s the animation's event
 * ShowResults begins the rows: its Delay chain plays each row's animation 0.25 s apart (the value, the rank with its
 * stamp, the "+N" its counter counts up to the fill sound), then TOTAL SHARDS, FINAL RANK (a louder stamp and the
 * screen jolted), and gives the input to the screen with the cursor. NEXT fades it to black and 4 s on broadcasts
 * Finished (the original's way when replaying a level; the XP box's count that comes first otherwise is left out).
 *
 * The tree is built here as in the original, slot for slot, but for what this game leaves out: the XP box (XPBox, and
 * Image_161 that only its Level Up Animation plays) and DIARY UNLOCKED! (FinalRankText). Its binding functions (the
 * values, the ranks' letters and colours, TOTAL SHARDS, FINAL RANK) are set once from Results, which does not change
 * under it. The widget's own tick plays the animations and counts the Delays (a widget's latent actions go by its
 * tick), so they go on while the game is paused; an animation or a Delay started in a tick counts from the next.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiLevelClearWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiLevelClearWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Escape's Create(Self, UMG_LevelClear_C, None), its sets of the results and AddToViewport(6), for the first player
	 * (left off where there is none, as in a test's world). Returns the widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Clear", meta = (WorldContext = "WorldContextObject"))
	static UWasamiLevelClearWidget* Show(const UObject* WorldContextObject, const FWasamiLevelResults& InResults);

	/** The Z order Escape adds the widget at. */
	static constexpr int32 ViewportZOrder = 6;

	/** What the level Blueprint puts into the widget's variables before adding it. */
	UPROPERTY(BlueprintReadWrite, Category = "Level Clear")
	FWasamiLevelResults Results;

	/** ClearAnimation's length (its playback range, [0, 263388) ticks: its audio track's end). */
	static constexpr float ClearLength = 263388.f / 60000.f;
	/** Its audio track: UI_YouEscaped from 45000 ticks. */
	static constexpr float EscapedSoundTime = 0.75f;
	/** Its event track: ShowResults at 195000 ticks. */
	static constexpr float ShowResultsTime = 3.25f;

	/**
	 * ShowResults' Delays, after each of its steps in turn: the six rows' animations (0.25 s, after SHARD STREAK 0.5 s),
	 * Total Shards Animation (1 s) and Final Rank Animation (1 s), the last before the input goes to the screen.
	 */
	static constexpr float ResultsDelays[] = {0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.5f, 1.f, 1.f};
	static constexpr int32 ResultsSteps = 9;

	/** A row's animation (Time Animation … Shard Streak Animation, all alike): 1 s, its counter event at 0.5 s. */
	static constexpr float RowLength = 1.f;
	static constexpr float RowCounterTime = 0.5f;
	/** Its audio track, Level_Clear_Grade_Stamp_v2: from 24000 ticks in Time's, 23999 in Soul Shards' and Bonus Shards', 27000 in the rest. */
	static float RowStampTime(int32 Row);
	/** The row's counter's span (it waits span / n between numbers): 0.25, none (SOUL SHARDS' counts nothing), 0.25, 0.05, 0.25, 0.5 s. */
	static float RowCountSpan(int32 Row);
	/** Total Shards Animation: 0.5 s, its counter event at its start (span 0.5 s). */
	static constexpr float TotalLength = 0.5f;
	static constexpr float TotalCountSpan = 0.5f;
	/** Final Rank Animation: 0.75 s, its audio track (Level_Clear_Grade_Stamp_v1) from 7200 ticks. */
	static constexpr float FinalLength = 0.75f;
	static constexpr float FinalStampTime = 0.12f;
	/** Fade Out (60001 ticks) and NEXT's Delay to Finished. */
	static constexpr float FadeLength = 60001.f / 60000.f;
	static constexpr float FinishDelay = 4.f;
	/** The counters: the six rows' (in GetRows' order), then TOTAL SHARDS'. */
	static constexpr int32 TotalCounter = 6;

	/** Finished: NEXT's Delay's end (the level Blueprint binds its Finished Level to it). */
	UPROPERTY(BlueprintAssignable, Category = "Level Clear")
	FWasamiLevelClearFinishedSignature OnFinished;

	/** Takes the screen off at Finished (for the debug command, where nothing takes over). */
	bool bRemoveWhenFinished = false;

	/** NEXT's OnClicked (a DoOnce): Fade Out, the input back to the game without the cursor, and Finished 4 s on. */
	void PressNext();

	/**
	 * Construct's run from its start: the bindings from Results, easymode off the screen unless EASY, ClearAnimation from
	 * 0 (NativeConstruct calls it; the tests call it alone).
	 */
	void Begin();

	/**
	 * Moves the screen on by DeltaSeconds (the widget's tick): the animations playing, the Delays (ShowResults' chain,
	 * the counters', NEXT's), and the animations' sounds and events as they pass them.
	 */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether ClearAnimation passed its sound (UI_YouEscaped) and its event (ShowResults). */
	bool HasPlayedEscapedSound() const { return bEscapedSoundPlayed; }
	bool HasShownResults() const { return bResultsShown; }

	/** Whether easymode is still in the tree (Construct takes it off unless the difficulty is EASY). */
	bool IsEasyModeShown() const;

	/** How many of ShowResults' steps have run (0..ResultsSteps: the six rows, TOTAL SHARDS, FINAL RANK, the input). */
	int32 GetResultsStep() const { return ResultsStep; }
	/** Whether a counter (a row's, or TotalCounter) is counting, its fill sound on. */
	bool IsCounting(int32 Counter) const;
	/** Whether NEXT was pressed, and whether Finished went out. */
	bool IsNextPressed() const { return FadeTime >= 0.f; }
	bool IsFinished() const { return bFinished; }

	/** A row's "+N" as its counter left it. */
	FText GetShardsText(int32 Row) const;

	/** The texts the bindings show: a row's value, rank letter and colour (rows in GetRows' order), the total, FINAL RANK. */
	FText GetValueText(int32 Row) const;
	FText GetRankText(int32 Row) const;
	FLinearColor GetRankColor(int32 Row) const;
	FText GetTotalText() const;
	FText GetFinalRankText() const;
	FLinearColor GetFinalRankColor() const;

	/**
	 * ClearAnimation's tracks at Seconds: the whole widget's opacity, ResultsBox's, You Escaped!'s (Image_216) opacity,
	 * angle and scale, ClearLevel's (the red and the lettering) opacity and jolt, Image_6's (the white vignette) and
	 * Image_7's (the white over the screen) alpha, easymode's opacity.
	 */
	static float EvaluateRootOpacity(float Seconds);
	static float EvaluateResultsOpacity(float Seconds);
	static float EvaluateEscapedOpacity(float Seconds);
	static float EvaluateEscapedAngle(float Seconds);
	static float EvaluateEscapedScale(float Seconds);
	static float EvaluateClearOpacity(float Seconds);
	static FVector2D EvaluateClearJolt(float Seconds);
	static float EvaluateVignetteAlpha(float Seconds);
	static float EvaluateFlashAlpha(float Seconds);
	static float EvaluateEasyOpacity(float Seconds);

	/**
	 * A row's animation at Seconds: the value in and down from 1.5 times its size past 1.05, the rank alike 0.25 s on
	 * and the "+N" 0.25 s after that (Total Shards Animation's is the value's). Before its section a scale is the
	 * widget's own, 1.
	 */
	static float EvaluateRowValueOpacity(float Seconds);
	static float EvaluateRowValueScale(float Seconds);
	static float EvaluateRowRankOpacity(float Seconds);
	static float EvaluateRowRankScale(float Seconds);
	static float EvaluateRowShardsOpacity(float Seconds);
	static float EvaluateRowShardsScale(float Seconds);
	/** Final Rank Animation at Seconds: the letter in from 1.5 times its size past 1.15, and the whole screen's jolt. */
	static float EvaluateFinalOpacity(float Seconds);
	static float EvaluateFinalScale(float Seconds);
	static FVector2D EvaluateFinalJolt(float Seconds);
	/** Fade Out at Seconds: FadeOut's opacity. */
	static float EvaluateFadeOpacity(float Seconds);

	/** you_escaped (Image_216). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> EscapedTexture;

	/** results_window (Image_1 and Image_2, the rules above and under the rows). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> RuleTexture;

	/**
	 * The level's title (LevelName, tinted red): Construct picks it by the game mode's Level, the hospital's is 7. This
	 * game shows the WebGL version's "Stinky Gachimi" in its place (the user's answer of 2026-09-20).
	 */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> LevelNameTexture;

	/** T_Vignette (Image_6). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

	/** helvetica-neue-bold_Font (every text but easymode). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UFont> TextFont;

	/** UI_YouEscaped (ClearAnimation's audio track). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<USoundBase> EscapedSound;

	/** Level_Clear_Grade_Stamp_v2 (the rows' audio tracks) and _v1 (Final Rank Animation's). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<USoundBase> RowStampSound;

	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<USoundBase> FinalStampSound;

	/** UI_XP_Bar_Fill_V2A_0617 (looping), which a counter plays until it reaches its number. */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<USoundBase> FillSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** ShowResults: ClearAnimation's event at 3.25 s, which puts the rows up one by one. */
	void ShowResults();

private:
	/** A counter (the original's *Counter events): +1 each Delay(span / n) from 1 to n, the fill sound until n. */
	struct FCounter
	{
		int32 Number = 0;
		float Span = 0.f;
		int32 Count = 0;
		// Its Delay: the time left, which each tick after the one that started it takes from.
		float DelayRemaining = 0.f;
		bool bDelaying = false;
		TWeakObjectPtr<UAudioComponent> Sound;
	};

	void BuildScreen(UCanvasPanel* Root);
	void ApplyResults();
	void ApplyAnimation();
	void TickDelays(float DeltaSeconds);
	void TickResultsEvents();
	void RunResultsStep();
	void StartCounter(int32 Index, int32 Number, float Span);
	void StepCounter(int32 Index);
	void PlayUISound(const TSoftObjectPtr<USoundBase>& Sound, float StartTime = 0.f);
	void Finish();

	UFUNCTION()
	void OnNextClicked();

	UFUNCTION()
	void OnNextHovered();

	UFUNCTION()
	void OnNextUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ResultsBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ValueTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RankTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ShardsTexts;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TotalShardAmount;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FinalRank;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EasyMode;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NextButton;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> ClearLevel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Escaped;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FadeOut;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Flash;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootPanel;

	float Elapsed = 0.f;
	bool bEscapedSoundPlayed = false;
	bool bResultsShown = false;

	// ShowResults' chain: how many of its steps ran, and the time left in the Delay after the last.
	int32 ResultsStep = 0;
	float ResultsDelayRemaining = 0.f;
	// How far each row's, TOTAL SHARDS' and FINAL RANK's animation has played (below 0 until it starts), and which of
	// their sounds and counter events went.
	float RowTimes[6] = {-1.f, -1.f, -1.f, -1.f, -1.f, -1.f};
	bool bRowStamped[6] = {};
	bool bRowCounted[6] = {};
	float TotalTime = -1.f;
	bool bTotalCounted = false;
	float FinalTime = -1.f;
	bool bFinalStamped = false;
	FCounter Counters[7];

	// NEXT: how far Fade Out has played (below 0 until NEXT), the time left to Finished, and Finished.
	float FadeTime = -1.f;
	float FinishRemaining = 0.f;
	bool bFinished = false;
};
