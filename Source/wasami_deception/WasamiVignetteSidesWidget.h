#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiVignetteSidesWidget.generated.h"

class UCanvasPanel;
class UFont;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * The flash a special shard puts on the screen when taken, after Dark Deception's UI/Menu/Streaks/UMG_VignetteSides
 * (pak_reference_2): the stun orb's ENEMIES STUNNED in orange and the red shard's ENEMIES REVEALED in red. A vignette
 * (T_VignetteNew) flares at the screen's edges and the words slam in above the bottom, the whole screen giving a small
 * twist; Construct plays Anim (1.5 s) and its Delay 2 takes the widget off. The pickups add it to the player's screen
 * at Z 5 with Color, Text? and TextToDisplay set.
 *
 * The tree is built here as in the original, slot for slot, and the widget's own tick plays the animation and counts
 * the Delay, as UMG_ShardStreak's (UWasamiShardStreakWidget).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiVignetteSidesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiVignetteSidesWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * The pickups' Create(Self, UMG_VignetteSides_C, None), Color, Text? and TextToDisplay set by name, and
	 * AddToPlayerScreen(5), for the first player (left off the screen where there is none, as in a test's world).
	 * Returns the widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Special Shards", meta = (WorldContext = "WorldContextObject"))
	static UWasamiVignetteSidesWidget* Show(const UObject* WorldContextObject, FLinearColor InColor, bool bInText, FText InTextToDisplay);

	/** Color: the whole widget's tint (Construct's SetColorAndOpacity on itself). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Shards")
	FLinearColor Color = FLinearColor(1.f, 0.39522600173950195f, 0.f, 1.f);

	/** Text?: whether TextBlock_47 shows TextToDisplay (it is hidden otherwise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Shards")
	bool bText = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special Shards")
	FText TextToDisplay;

	/** BP_PowerOrb's and BP_BonusShard's Color and TextToDisplay (both with Text? true). */
	static const FLinearColor StunnedColor;
	static const FLinearColor RevealedColor;
	static FText StunnedText();
	static FText RevealedText();

	/** Anim's length (its playback range, [0, 90001) ticks), and Construct's Delay before RemoveFromParent. */
	static constexpr float AnimLength = 90001.f / 60000.f;
	static constexpr float RemoveDelay = 2.f;

	/** The Z order the pickups add the widget at. */
	static constexpr int32 ZOrder = 5;

	/**
	 * Construct's run from its start with the widget's Color, Text? and TextToDisplay: the words or none, the tint,
	 * Anim from 0 (the tests call it alone; NativeConstruct calls it).
	 */
	void Begin();

	/** Moves Anim and the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the Delay ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

	/** TextBlock_47, and Image_161 (the vignette), for the tests. */
	UTextBlock* GetTextBlock() const { return TextBlock; }
	UImage* GetVignette() const { return Vignette; }

	/**
	 * Anim's tracks at Seconds: TextBlock_47's scale and RenderOpacity, Image_161's scale and alpha, and
	 * CanvasPanel_0's angle (degrees) and scale (its section ends at 28000 ticks, and the panel keeps its own after).
	 */
	static float EvaluateTextScale(float Seconds);
	static float EvaluateTextOpacity(float Seconds);
	static float EvaluateVignetteScale(float Seconds);
	static float EvaluateVignetteAlpha(float Seconds);
	static float EvaluateCanvasAngle(float Seconds);
	static float EvaluateCanvasScale(float Seconds);

	/** T_VignetteNew (Image_161). */
	UPROPERTY(EditAnywhere, Category = "Special Shards|Assets")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

	/** helvetica-neue-bold_Font (TextBlock_47). */
	UPROPERTY(EditAnywhere, Category = "Special Shards|Assets")
	TSoftObjectPtr<UFont> TextFont;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBlock;

	float Elapsed = 0.f;
	bool bFinished = false;
};
