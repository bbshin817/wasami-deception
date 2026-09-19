#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiChapterPortalWidget.generated.h"

class UBackgroundBlur;
class UCanvasPanel;
class UImage;
class UTexture2D;

/** The chapter portal's animation, loop, at one time: what each of its ten tracks sets. */
struct FWasamiChapterPortalPose
{
	/** CanvasPanel_0's RenderOpacity: the whole card fading in and, at the end, out. */
	float CanvasOpacity = 0.f;
	/** CanvasPanel_0's render translation: the jolt as the title lands. */
	FVector2D CanvasShake = FVector2D::ZeroVector;
	/** BackgroundBlur_0's BlurStrength. */
	float BlurStrength = 0.f;
	/** The left offsets of Image_48's and Image_49's slots (CanvasPanelSlot_11 and _13): the banners sliding left. */
	float BannerLeft = 0.f;
	float Banner2Left = 0.f;
	/** Image_40's and Image_41's render angles: the ring and the runes turning opposite ways. */
	float RingAngle = 0.f;
	float RunesAngle = 0.f;
	/** TitleCard's RenderOpacity, render scale (x and y alike) and ColorAndOpacity. */
	float TitleOpacity = 0.f;
	float TitleScale = 1.f;
	FLinearColor TitleColor = FLinearColor::White;
};

/**
 * The stage's title card, after Dark Deception's UI/Menu/UMG_ChapterPortal (the same tree in both versions), which the
 * hospital's entrance puts up as the level begins: over the whole screen a blur and a faint red wash, two long banners
 * across the middle, on the left the portal (its outer ring, the runes inside it and the level's monster's head) and
 * right of it the level's title in red. The tree is built here slot for slot. Where the original's Construct picks the
 * head and the title by Level (the hospital's are pause_reapernurse_head and chapter_ui_title_tormenttherapy), this
 * game's are fixed: the pause menu's Wasami, tinted its red, and the level clear screen's "Stinky Gachimi".
 *
 * Construct then plays loop once from 0.8 s at speed 1 (the card fades in behind a growing blur, the title lands with a
 * flash and a jolt, the banners slide and the portal turns, and at the end it all fades out) and 11 s later takes the
 * card off. The widget's own tick plays both, so they run on while the game is paused as the original's do.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiChapterPortalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiChapterPortalWidget(const FObjectInitializer& ObjectInitializer);

	/** AddToViewport's Z (the hospital's 00_Initial Start). */
	static constexpr int32 ViewportZOrder = 0;

	/**
	 * Create(UMG_ChapterPortal) for the first player and AddToViewport(0), as the hospital's 00_Initial Start does.
	 * Returns the card (null without a player controller).
	 */
	UFUNCTION(BlueprintCallable, Category = "Chapter Portal", meta = (WorldContext = "WorldContextObject"))
	static UWasamiChapterPortalWidget* Show(const UObject* WorldContextObject);

	/** The brushes' sizes (the tree's ImageSize; Construct's SetBrushFromTexture leaves them). */
	static constexpr float PortalSize = 470.f;
	static const FVector2D BannerSize;
	static const FVector2D TitleSize;

	/** Image_1's tint: the wash over the blur. */
	static const FLinearColor WashColor;
	/** TitleCard's ColorAndOpacity: the title's grey drawn red. */
	static const FLinearColor TitleColor;

	/** loop's length (its playback range, [0, 600001) ticks). */
	static constexpr float LoopLength = 600001.f / 60000.f;
	/** Where Construct starts loop (PlayAnimation's StartAtTime). */
	static constexpr float LoopStart = 0.8f;
	/** Construct's Delay before RemoveFromParent. */
	static constexpr float RemoveDelay = 11.f;

	/** Moves the card on by DeltaSeconds: loop, then the delay that takes it off (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Construct. */
	float GetElapsed() const { return Elapsed; }

	/** Where loop is (from LoopStart to its end, where it stays). */
	float GetLoopTime() const { return FMath::Min(LoopStart + Elapsed, LoopLength); }

	/** Whether the delay has run out and the card taken itself off the screen. */
	bool IsFinished() const { return bFinished; }

	/** loop's tracks at Seconds into it. Every section keeps its last value once past it (KeepState). */
	static FWasamiChapterPortalPose EvaluateLoop(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Image_40: the portal's outer ring (UI/Main/chapter_ui_portal_outer). */
	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> RingTexture;

	/** Image_41: the runes inside it (UI/Menu/TitleCards/chapter_title_portal_inner). */
	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> RunesTexture;

	/** Image_48 and Image_49: the banners (chapter_ui_banner_bg_01 and _02), tiled across. */
	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> BannerTexture;

	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> Banner2Texture;

	/** Logo: the head in the portal, the pause menu's Wasami (white, tinted UWasamiPauseWidget::HeadTint()). */
	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> HeadTexture;

	/** TitleCard: the level's title (the level clear screen's T_LevelTitle, grey like the original's cards). */
	UPROPERTY(EditAnywhere, Category = "Chapter Portal|Assets")
	TSoftObjectPtr<UTexture2D> TitleTexture;

private:
	void BuildScreen(UCanvasPanel* InRoot);
	void ApplyLoop();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Root;

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> Blur;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Banner;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Banner2;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Ring;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Runes;

	UPROPERTY(Transient)
	TObjectPtr<UImage> TitleCard;

	float Elapsed = 0.f;
	bool bFinished = false;
};
