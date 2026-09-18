#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiTextPromptWidget.generated.h"

class UFont;
class UTextBlock;

/**
 * A line of text low on the screen, after Dark Deception's UI/Main/UMG_TextPrompt (pak_reference_2): what the zone
 * barrier and the ring altar say when they turn the player away. Construct sets the text at Font Size and plays
 * NewAnimation_1 (the line rises into place as it fades in, stays, then drifts down as it fades out, 5 s in all), and
 * the widget takes itself off the screen as the animation ends. The tree is built here as in the original, and the
 * widget's own tick plays the animation.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiTextPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiTextPromptWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Create(Self, UMG_TextPrompt, None) with Text set, and AddToPlayerScreen(0) (the first player's, whom a widget made
	 * for the world belongs to; it is left off the screen where there is none, as in a test's world). Returns the prompt.
	 */
	UFUNCTION(BlueprintCallable, Category = "Text Prompt", meta = (WorldContext = "WorldContextObject"))
	static UWasamiTextPromptWidget* Show(const UObject* WorldContextObject, const FText& InText);

	/** Text: the line. Set before the prompt is added. */
	UPROPERTY(BlueprintReadWrite, Category = "Text Prompt")
	FText Text;

	/** Font Size: the size Construct sets the line's font to (the class's default; nobody sets another). */
	UPROPERTY(BlueprintReadWrite, Category = "Text Prompt")
	int32 FontSize = 24;

	/** NewAnimation_1's length (its playback range, [0, 300001) ticks), after which the prompt is taken off. */
	static constexpr float AnimationLength = 300001.f / 60000.f;

	/** Moves NewAnimation_1 on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Whether the animation has ended and the prompt taken itself off the screen. */
	bool IsFinished() const { return bFinished; }

	UTextBlock* GetTextBlock() const { return TextBlock; }

	/** NewAnimation_1's tracks on TextBlock_95 at Seconds: ColorAndOpacity's alpha, and the RenderTransform's y. */
	static float EvaluateOpacity(float Seconds);
	static float EvaluateTranslationY(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** helvetica-normal_Font: TextBlock_95's font. */
	UPROPERTY(EditAnywhere, Category = "Text Prompt|Assets")
	TSoftObjectPtr<UFont> PromptFont;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBlock;

	float AnimationTime = 0.f;
	bool bFinished = false;
};
