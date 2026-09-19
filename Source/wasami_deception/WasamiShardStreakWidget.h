#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiShardStreakWidget.generated.h"

class UCanvasPanel;
class UFont;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * A milestone of the shard streak, after Dark Deception's UI/Menu/Streaks/UMG_ShardStreak (pak_reference_2), which the
 * game mode's Check Streak adds to the player's screen (Z 2) as the shards in a row reach 20, 50, 100 … 1000: the
 * milestone's card (shard_streak_<n>) pops in in the middle, a purple vignette flashes at the screen's edges, and at
 * 200 and 500 EXTRA LIFE ! shows under the card with a life added. Construct plays Anim (1.5 s) and its Delay 2 takes
 * the widget off.
 *
 * The tree is built here as in the original, slot for slot, and the widget's own tick plays the animation and counts
 * the Delay, as UMG_Saving's.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiShardStreakWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiShardStreakWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * Check Streak's Create(Self, UMG_ShardStreak_C, None) with Streak set and AddToPlayerScreen(2), for the first
	 * player (left off the screen where there is none, as in a test's world). Returns the widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shards", meta = (WorldContext = "WorldContextObject"))
	static UWasamiShardStreakWidget* Show(const UObject* WorldContextObject, uint8 InStreak);

	/** Streak: the milestone as an Enum_ShardStreaks value, 1..10 for 20..1000 (0 shows no card). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shards")
	uint8 Streak = 1;

	/** Anim's length (its playback range, [0, 90001) ticks), and Construct's Delay before RemoveFromParent. */
	static constexpr float AnimLength = 90001.f / 60000.f;
	static constexpr float RemoveDelay = 2.f;

	/** The Z order Check Streak adds the widget at. */
	static constexpr int32 ZOrder = 2;

	/** Construct's SetVisibility of extralife and Increment Lives: Streak 5 (200) and 8 (500). */
	static bool GivesExtraLife(uint8 InStreak);

	/**
	 * Construct's run from its start for InStreak: the card, extralife shown or hidden, Anim from 0 (the tests call it
	 * alone; NativeConstruct calls it and adds the life).
	 */
	void Begin(uint8 InStreak);

	/** Moves Anim and the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the Delay ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

	/** Whether extralife is shown. */
	bool IsExtraLifeShown() const { return bExtraLife; }

	/** The card Begin put in StreakImage (null for Streak 0 or out of range, the Select's default). */
	UTexture2D* GetStreakTexture() const { return StreakTexture; }

	/** Anim's tracks at Seconds: StreakImage's scale and alpha, Image_161's scale and alpha, extralife's opacity and scale. */
	static float EvaluateCardScale(float Seconds);
	static float EvaluateCardAlpha(float Seconds);
	static float EvaluateVignetteScale(float Seconds);
	static float EvaluateVignetteAlpha(float Seconds);
	static float EvaluateLifeOpacity(float Seconds);
	static float EvaluateLifeScale(float Seconds);

	/** shard_streak_20 … shard_streak_1000, by Streak 1..10. */
	UPROPERTY(EditAnywhere, Category = "Shards|Assets")
	TArray<TSoftObjectPtr<UTexture2D>> StreakTextures;

	/** T_Vignette (Image_161, tinted purple). */
	UPROPERTY(EditAnywhere, Category = "Shards|Assets")
	TSoftObjectPtr<UTexture2D> VignetteTexture;

	/** life_icon_02 (Image_88). */
	UPROPERTY(EditAnywhere, Category = "Shards|Assets")
	TSoftObjectPtr<UTexture2D> LifeTexture;

	/** helvetica-neue-bold_Font (TextBlock_94). */
	UPROPERTY(EditAnywhere, Category = "Shards|Assets")
	TSoftObjectPtr<UFont> TextFont;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UImage> StreakImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Vignette;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> ExtraLife;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> StreakTexture;

	float Elapsed = 0.f;
	bool bFinished = false;
	bool bExtraLife = false;
};
