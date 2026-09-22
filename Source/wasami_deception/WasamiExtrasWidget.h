#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiExtrasWidget.generated.h"

class UBackgroundBlur;
class UButton;
class UCanvasPanel;
class UDataTable;
class UFont;
class UImage;
class UMaterialInterface;
class URichTextBlock;
class USlider;
class USoundBase;
class UTexture2D;
class UVerticalBox;
class UWasamiExtrasItemWidget;
class UWasamiExtrasSoundBarWidget;
class UWasamiExtrasSoundButtonWidget;
class UWasamiSaveGame;
class UWidgetSwitcher;
class UWrapBox;

/**
 * The extras screen, after Dark Deception's UI/Main/TitleScreen/UMG_Extras (pak_reference_2): black under the title's
 * panning brush strokes (MM_TitleScreen_Mask_) and a light blur, the sections on the left (ART GALLERY, BIERCE DIARIES,
 * SOUND ARCHIVE, the hidden MOVIES and CREDITS; dark grey, white while hovered or chosen), a thin red line and the
 * section chosen on the right: the credits scrolling up, the 35 pictures, the ten diaries and the sounds over their
 * player and the ten movies. BACK at the bottom left. Construct sounds UI_Window_PopUp_V2, fades the screen in over
 * 0.25 s and opens the Art Gallery; BACK fades it out, sounds UI_Select_V3 (pitch 0.7) and 0.25 s later broadcasts
 * FadeMusic and takes the screen off. Choosing a section stops the sound playing (Check If Playing).
 *
 * The tree is built here as in the original, slot for slot, with the parts of UWasamiExtrasItemWidget and
 * UWasamiExtrasSoundWidget.h; the widget's own tick plays FadeIn, Credits_Scroll and BACK's Delay. What the sections
 * show is provisional (the original's pictures, diaries, movies and credits are not used; 2026-09-20, the user's
 * "枠組みだけ先に作る"): TODO(仮) pictures 19 to 22 are this game's own, the diaries are empty and the credits
 * are this game's. The Sound Archive keeps the original's ten slots with no track put in any of them, so it is
 * always all locked, as MOVIES is (2026-09-23, the user's 「EXTRAS に曲は不要」「区分は残して曲を置かない」).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiExtrasWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiExtrasWidget(const FObjectInitializer& ObjectInitializer);

	/** The Z order the title screen adds the screen at. */
	static constexpr int32 ViewportZOrder = 2;

	/**
	 * The title screen's EXTRAS (UMG_TitleScreen @2293): Create(UMG_Extras) for the first player and AddToViewport(2).
	 * Returns the screen (null without a player controller); the title binds its FadeMusic.
	 */
	UFUNCTION(BlueprintCallable, Category = "Extras", meta = (WorldContext = "WorldContextObject"))
	static UWasamiExtrasWidget* Show(const UObject* WorldContextObject);

	/**
	 * FadeMusic: broadcast as BACK takes the screen off (the title screen binds its FadeInMusic, which brings its music
	 * back).
	 */
	FSimpleMulticastDelegate OnFadeMusic;

	/**
	 * The save the parts read their locks from, read once as the tree is built: SaveSlotName's when none is given, a new
	 * one when the slot is empty (the parts' own rule).
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWasamiSaveGame> Save;

	FString SaveSlotName;

	/** The sections: the original's Active Button values and WidgetSwitcher_276's children, in its order. */
	static constexpr int32 CreditsSection = 0;
	static constexpr int32 ArtGallerySection = 1;
	static constexpr int32 BierceDiariesSection = 2;
	static constexpr int32 SoundArchiveSection = 3;
	static constexpr int32 MoviesSection = 4;
	static constexpr int32 SectionCount = 5;

	/**
	 * How many extras each section lays out (WrapBox_0's pictures, WrapBox_1's diaries, WrapBox_2's sounds,
	 * WrapBox_3's movies), each as many as the original's tree holds. The Sound Archive keeps its ten slots although
	 * this game puts no track in them (2026-09-23, the user's 「EXTRAS に曲は不要」).
	 */
	static constexpr int32 ArtCount = 35;
	static constexpr int32 DiaryCount = 10;
	static constexpr int32 SoundCount = 10;
	static constexpr int32 VideoCount = 10;

	/** FadeIn's length (its playback range, [0, 30001) ticks) and the speed both of its plays go at. */
	static constexpr float FadeInLength = 30001.f / 60000.f;
	static constexpr float FadeInSpeed = 2.f;

	/** Credits_Scroll's length ([0, 2388001) ticks), played at 1. */
	static constexpr float CreditsScrollLength = 2388001.f / 60000.f;

	/** BACK's select sound's pitch and its Delay before FadeMusic and the screen going. */
	static constexpr float BackPitch = 0.7f;
	static constexpr float BackDelay = 0.25f;

	/** Unhovered Color: the section buttons' and BACK's colour until hovered or chosen (white then). */
	static constexpr float UnhoveredGrey = 0.11f;

	/**
	 * Construct (@649): PlaySound2D(UI_Window_PopUp_V2), PlayAnimation(FadeIn, 0, 1, Forward, 2) and the Art Gallery
	 * shown (@755) without its select sound. NativeConstruct calls it (the tests call it alone, without a world).
	 */
	void Begin();

	/** Moves FadeIn, Credits_Scroll and BACK's Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/**
	 * A section button's click. ART GALLERY (@2219), BIERCE DIARIES (@2332) and SOUND ARCHIVE (@3070) do nothing while
	 * theirs is chosen; CREDITS (@2805) and MOVIES (@3405) always go on. The section is shown: WidgetSwitcher_276 to it,
	 * Active Button, Reset All Colors, its button white, CREDITS' Credits_Scroll from the start, Check If Playing; and
	 * PlaySound2D(UI_Select_V3), before all that for ART GALLERY and after it for the rest.
	 */
	void Select(int32 Section);

	/** A section button's hover (white) and unhover (Unhovered Color), both left alone while its section is chosen. */
	void HoverSection(int32 Section, bool bHovered);

	/**
	 * BACK (@1023): PlayAnimation(FadeIn, 0, 1, Reverse, 2), which starts over from the end each time, PlaySound2D
	 * (UI_Select_V3, 1, 0.7) and Delay(0.25) → FadeMusic and RemoveFromParent (a pending Delay ignores another).
	 */
	void PressBack();

	/** BACK's hover (@914: white) and unhover (@981: Unhovered Color). */
	void HoverBack(bool bHovered);

	/** Reset All Colors (@2599): the five section buttons at Unhovered Color. */
	void ResetAllColors();

	/**
	 * Check If Playing (@3337): GetAllWidgetsOfClass(UMG_Extras_Sound_Button, not top level only), and each one Playing?
	 * deselected and its bar's sound stopped (a paused one is left alone).
	 */
	void CheckIfPlaying();

	/** Active Button: the section chosen (0 until Construct). */
	int32 GetActiveButton() const { return ActiveButton; }

	/** Seconds into FadeIn (going down while it plays backwards) and into Credits_Scroll. */
	float GetFadeInTime() const { return FadeInTime; }
	float GetCreditsTime() const { return CreditsTime; }

	/** Whether BACK has been pressed, and whether the screen has taken itself off. */
	bool IsClosing() const { return bClosing; }
	bool IsFinished() const { return bFinished; }

	/** FadeIn's CanvasPanel_0 RenderOpacity (0 → 1 over 0.5 s) and Credits_Scroll's RichTextBlock_258 translation's Y. */
	static float EvaluateOpacity(float Seconds);
	static float EvaluateCreditsY(float Seconds);

	/** The provisional credits (RichText_Credits' tags). TODO(仮): the credits are to be decided with the user. */
	static const TCHAR* const CreditsText;

	/** The widgets, for the tests. */
	UCanvasPanel* GetRoot() const { return Root; }
	UImage* GetBackdrop() const { return Backdrop; }
	UBackgroundBlur* GetBlur() const { return Blur; }
	UVerticalBox* GetSectionBox() const { return SectionBox; }
	UButton* GetSectionButton(int32 Section) const { return SectionButtons.IsValidIndex(Section) ? SectionButtons[Section] : nullptr; }
	UButton* GetBackButton() const { return BackButton; }
	USlider* GetSlider() const { return Slider; }
	UWidgetSwitcher* GetSwitcher() const { return Switcher; }
	URichTextBlock* GetCredits() const { return Credits; }
	const TArray<TObjectPtr<UWasamiExtrasItemWidget>>& GetArtItems() const { return ArtItems; }
	const TArray<TObjectPtr<UWasamiExtrasSoundButtonWidget>>& GetDiaryButtons() const { return DiaryButtons; }
	const TArray<TObjectPtr<UWasamiExtrasSoundButtonWidget>>& GetSoundButtons() const { return SoundButtons; }
	const TArray<TObjectPtr<UWasamiExtrasItemWidget>>& GetVideoItems() const { return VideoItems; }
	UWasamiExtrasSoundBarWidget* GetDiaryBar() const { return DiaryBar; }
	UWasamiExtrasSoundBarWidget* GetSoundBar() const { return SoundBar; }
	UWrapBox* GetArtBox() const { return ArtBox; }
	UWrapBox* GetDiaryBox() const { return DiaryBox; }
	UWrapBox* GetSoundBox() const { return SoundBox; }
	UWrapBox* GetVideoBox() const { return VideoBox; }

	/** MM_TitleScreen_Mask_: the panning brush strokes. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UMaterialInterface> BackdropMaterial;

	/** helvetica-normal_Font (the sections, YEL and Locked) and helvetica-neue-bold_Font (BACK). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> NormalFont;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> BoldFont;

	/** The engine's Roboto (RichText_Credits' REG, RED and RED_BOLD). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> CreditsFont;

	/** UI_Window_PopUp_V2 (Construct) and UI_Select_V3 (the sections and BACK). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

	/**
	 * The Art Gallery's pictures by ID (ArtCount; none where the original's has none or where this game has yet to put
	 * one). TODO(仮): 19 to 22 (the ones the hospital's files unlock) are this game's title face and pause pictures and
	 * the portal's Wasami until the pictures are decided with the user.
	 */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TArray<TSoftObjectPtr<UTexture2D>> ArtTextures;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** @755 and its like: the switcher, Active Button, Reset All Colors, the button white, Credits_Scroll, Check If Playing. */
	void ShowSection(int32 Section);

	void PlaySelectSound(float Pitch);
	void ApplyAnimation();

	/** RichText_Credits: its five rows (REG, YEL, Locked, RED, RED_BOLD) as a table made here. */
	UDataTable* MakeCreditsStyles();

	UFUNCTION()
	void OnCreditsClicked();

	UFUNCTION()
	void OnArtGalleryClicked();

	UFUNCTION()
	void OnBierceDiariesClicked();

	UFUNCTION()
	void OnSoundArchiveClicked();

	UFUNCTION()
	void OnMoviesClicked();

	UFUNCTION()
	void OnCreditsHovered();

	UFUNCTION()
	void OnCreditsUnhovered();

	UFUNCTION()
	void OnArtGalleryHovered();

	UFUNCTION()
	void OnArtGalleryUnhovered();

	UFUNCTION()
	void OnBierceDiariesHovered();

	UFUNCTION()
	void OnBierceDiariesUnhovered();

	UFUNCTION()
	void OnSoundArchiveHovered();

	UFUNCTION()
	void OnSoundArchiveUnhovered();

	UFUNCTION()
	void OnMoviesHovered();

	UFUNCTION()
	void OnMoviesUnhovered();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnBackHovered();

	UFUNCTION()
	void OnBackUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Root;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Backdrop;

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> Blur;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SectionBox;

	/** ArtGallery, BierceDiaries, … by section (CREDITS first, as Active Button counts them). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SectionButtons;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;

	UPROPERTY(Transient)
	TObjectPtr<USlider> Slider;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(Transient)
	TObjectPtr<URichTextBlock> Credits;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CreditsStyles;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> ArtBox;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> DiaryBox;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> SoundBox;

	UPROPERTY(Transient)
	TObjectPtr<UWrapBox> VideoBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWasamiExtrasItemWidget>> ArtItems;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWasamiExtrasSoundButtonWidget>> DiaryButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWasamiExtrasSoundButtonWidget>> SoundButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWasamiExtrasItemWidget>> VideoItems;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiExtrasSoundBarWidget> DiaryBar;

	UPROPERTY(Transient)
	TObjectPtr<UWasamiExtrasSoundBarWidget> SoundBar;

	int32 ActiveButton = 0;
	float FadeInTime = 0.f;
	bool bFadeInReverse = false;
	float CreditsTime = 0.f;
	bool bCreditsScrolling = false;
	bool bClosing = false;
	float CloseElapsed = 0.f;
	bool bFinished = false;
};
