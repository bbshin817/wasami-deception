#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiChapterPortalWidget.generated.h"

class UBackgroundBlur;
class UCanvasPanel;
class UImage;
class UTexture2D;

/**
 * The stage's title card, after Dark Deception's UI/Menu/UMG_ChapterPortal (the same tree in both versions), which the
 * hospital's entrance puts up as the level begins: over the whole screen a blur and a faint red wash, two long banners
 * across the middle, on the left the portal (its outer ring, the runes inside it and the level's monster's head) and
 * right of it the level's title in red. The tree is built here slot for slot. Where the original's Construct picks the
 * head and the title by Level (the hospital's are pause_reapernurse_head and chapter_ui_title_tormenttherapy), this
 * game's are fixed: the pause menu's Wasami, tinted its red, and the level clear screen's "Stinky Gachimi".
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

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
};
