#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiLevelResults.h"
#include "WasamiLevelClearWidget.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

/**
 * The level clear screen, after Dark Deception's UI/Menu/UMG_LevelClear (pak_reference, as the WebGL version follows
 * it), which the hospital's Escape adds to the viewport (Z 6) with the results filled in. Its Construct plays
 * ClearAnimation (4.39 s): the screen fades to black and red, You Escaped! turns and lands with a white flash and
 * UI_YouEscaped at 0.75 s, the red leaves from 2.75 s as RESULTS comes in, and at 3.25 s the animation's event
 * ShowResults begins the rows.
 *
 * The tree is built here as in the original, slot for slot, but for what this game leaves out: the XP box (XPBox, and
 * Image_161 that only its Level Up Animation plays) and DIARY UNLOCKED! (FinalRankText). Its binding functions (the
 * values, the ranks' letters and colours, TOTAL SHARDS, FINAL RANK) are set once from Results, which does not change
 * under it. The widget's own tick plays the animation, as UMG_Saving's, so it goes on while the game is paused.
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
	 * Construct's run from its start: the bindings from Results, easymode off the screen unless EASY, ClearAnimation from
	 * 0 (NativeConstruct calls it; the tests call it alone).
	 */
	void Begin();

	/** Moves ClearAnimation on by DeltaSeconds, playing its sound and firing its event as it passes them. */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether ClearAnimation passed its sound (UI_YouEscaped) and its event (ShowResults). */
	bool HasPlayedEscapedSound() const { return bEscapedSoundPlayed; }
	bool HasShownResults() const { return bResultsShown; }

	/** Whether easymode is still in the tree (Construct takes it off unless the difficulty is EASY). */
	bool IsEasyModeShown() const;

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

	/** you_escaped (Image_216). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> EscapedTexture;

	/** results_window (Image_1 and Image_2, the rules above and under the rows). */
	UPROPERTY(EditAnywhere, Category = "Level Clear|Assets")
	TSoftObjectPtr<UTexture2D> RuleTexture;

	/** The level's title (LevelName, tinted red): Construct picks it by the game mode's Level, the hospital's is 7. */
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** ShowResults: ClearAnimation's event at 3.25 s, which puts the rows up one by one. */
	void ShowResults();

private:
	void BuildScreen(UCanvasPanel* Root);
	void ApplyResults();
	void ApplyAnimation();

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

	float Elapsed = 0.f;
	bool bEscapedSoundPlayed = false;
	bool bResultsShown = false;
};
