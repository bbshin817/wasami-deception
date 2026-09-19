#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiMysteryNoteWidget.generated.h"

class UAudioComponent;
class UBackgroundBlur;
class UButton;
class UCanvasPanel;
class UDataTable;
class UFont;
class UImage;
class URichTextBlock;
class UScaleBox;
class USoundBase;
class UTextBlock;
class UTexture2D;

/**
 * The screen a note in the secret room opens, after Dark Deception's Blueprints/UMG/UMG_MysteryNote (pak_reference_2):
 * the note's picture (paper) swings up into the middle of the screen, the screen blurs and darkens over it, and the
 * note's words are written out on top, with the page (1/2) under them, a grey arrow to the next page and CLOSE at the
 * bottom right. Construct shows Texture, takes the input with the cursor, plays Open, pauses the game and writes the
 * first of Texts (a note of one page drops the arrow); with E Note it fades DD_LoreNote_01 in. The arrow plays
 * SwitchPage and goes round the pages; CLOSE unpauses, gives the input back, plays Open backwards and takes the screen
 * off half a second on, and Destruct fades the sound out. BP_MysteryCollectable adds it to the viewport at Z 2.
 *
 * The tree is built here as in the original, slot for slot (MysteryText's text styles as a table of the widget's own),
 * and the widget's own tick plays the animations and the Delay, so it works over the paused game.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiMysteryNoteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiMysteryNoteWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * BP_MysteryCollectable's InteractWithObject: Create(Self, UMG_MysteryNote_C, None) with Texture, Texts and E Note
	 * (its Lore Note) set by name, and AddToViewport(2), for the first player. Returns the screen (null without a player
	 * controller).
	 */
	static UWasamiMysteryNoteWidget* Show(const UObject* WorldContextObject, UTexture2D* InTexture, const TArray<FText>& InTexts,
		bool bInLoreNote);

	/** Loads what the screen draws with (and plays) into Out. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** The note's picture (paper's brush, at the picture's own size). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture;

	/** The note's pages. */
	UPROPERTY(Transient)
	TArray<FText> Texts;

	/** E Note: whether the note plays DD_LoreNote_01 (the class's default is true). */
	UPROPERTY(Transient)
	bool bENote = true;

	/** The Z order BP_MysteryCollectable adds the screen at. */
	static constexpr int32 ViewportZOrder = 2;

	/** Open's and SwitchPage's lengths (their playback ranges, [0, 45001) and [0, 15001) ticks). */
	static constexpr float OpenLength = 45001.f / 60000.f;
	static constexpr float SwitchPageLength = 15001.f / 60000.f;

	/** CLOSE's PlayAnimation(Open, 0.35, 1, Reverse, 1) start time (counted back from Open's end), and its Delay. */
	static constexpr float CloseFrom = 0.35f;
	static constexpr float CloseDelay = 0.5f;

	/** E Note's CreateSound2D FadeIn and Destruct's FadeOut, in seconds. */
	static constexpr float SoundFadeInSeconds = 0.5f;
	static constexpr float SoundFadeOutSeconds = 0.5f;

	/**
	 * Construct's run (@183): paper's picture, the input to the screen with the cursor, Open from its start, the game
	 * paused, the page written, the arrow dropped for a single page, and E Note's sound. NativeConstruct calls it (the
	 * tests call it alone, without a world).
	 */
	void Begin();

	/** Moves Open, SwitchPage and CLOSE's Delay on by DeltaSeconds and writes the page (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/**
	 * NextPage (@917): SwitchPage from its start, and the next page, or the first after the last (Update Text writes it).
	 */
	void PressNextPage();

	/**
	 * Close (@823): the game unpaused, the input to the game without the cursor, Open backwards from CloseFrom, and 0.5 s
	 * later the screen off (a pending Delay ignores another).
	 */
	void PressClose();

	/** currentText: the page shown, from 0. */
	int32 GetCurrentText() const { return CurrentText; }

	/** GetText_0, TextBlock_256's binding: "page/pages" from 1. */
	FText GetPageText() const;

	/** Whether CLOSE has been pressed, and whether the screen has taken itself off. */
	bool IsClosing() const { return bClosing; }
	bool IsFinished() const { return bFinished; }

	/** Seconds into Open (going down while it plays backwards) and into SwitchPage. */
	float GetOpenTime() const { return OpenTime; }
	float GetSwitchPageTime() const { return SwitchPageTime; }

	/** E Note's sound (null until Construct makes it in a world). */
	UAudioComponent* GetSound() const { return Sound; }

	/**
	 * Open's tracks at Seconds: ScaleBox_91's (the paper's) RenderTransform translation's y and angle (degrees), the
	 * RenderOpacity 0 → 1 over 0.35 s of ScaleBox_91, Image_0, Close, ScaleBox_0 and NextPage (the same keys),
	 * TextBlock_256's (from 0.15 s), and BackgroundBlur_0's BlurStrength 0 → 3. SwitchPage's: RichTextBlock_0's
	 * RenderOpacity 0 → 1 over 0.25 s.
	 */
	static float EvaluatePaperY(float Seconds);
	static float EvaluatePaperAngle(float Seconds);
	static float EvaluateFade(float Seconds);
	static float EvaluatePageOpacity(float Seconds);
	static float EvaluateBlur(float Seconds);
	static float EvaluateSwitchPage(float Seconds);

	/** paper, RichTextBlock_0, TextBlock_256, NextPage, Close, BackgroundBlur_0, Image_0 and the two scale boxes, for the tests. */
	UImage* GetPaper() const { return Paper; }
	URichTextBlock* GetTextBlock() const { return TextBlock; }
	UTextBlock* GetPageBlock() const { return PageBlock; }
	UButton* GetNextPageButton() const { return NextPageButton; }
	UButton* GetCloseButton() const { return CloseButton; }
	UBackgroundBlur* GetBlur() const { return Blur; }
	UImage* GetDim() const { return Dim; }
	UScaleBox* GetPaperBox() const { return PaperBox; }
	UScaleBox* GetTextBox() const { return TextBox; }

	/** helvetica-neue-bold_Font (CLOSE and the page) and helvetica-normal_Font (MysteryText's styles). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UFont> BoldFont;

	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UFont> NormalFont;

	/** selection_bar_arrow_hover (NextPage's three looks). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UTexture2D> ArrowTexture;

	/** DD_LoreNote_01. */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<USoundBase> LoreNoteSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Update Text (@1256): RichTextBlock_0.SetText(Texts[currentText]). */
	void UpdateText();
	void ApplyAnimation();

	UFUNCTION()
	void OnNextPageClicked();

	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY(Transient)
	TObjectPtr<UImage> Paper;

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> PaperBox;

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> Blur;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Dim;

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> TextBox;

	UPROPERTY(Transient)
	TObjectPtr<URichTextBlock> TextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PageBlock;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NextPageButton;

	/** MysteryText: the rich text's styles (Default and Player). */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> TextStyles;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Sound;

	int32 CurrentText = 0;
	float OpenTime = 0.f;
	float SwitchPageTime = 0.f;
	bool bOpenReverse = false;
	bool bSwitchPagePlayed = false;
	bool bClosing = false;
	float CloseElapsed = 0.f;
	bool bFinished = false;
};
