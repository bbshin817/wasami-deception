#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiCollectablesWidget.generated.h"

class UCanvasPanel;
class UFont;
class UImage;
class USoundBase;
class UTextBlock;
class UTexture2D;

/**
 * The banner a secret file puts up when taken, after Dark Deception's UI/Main/UMG_Collectables (pak_reference_2): NEW
 * EXTRAS UNLOCKED! on the extras frame (extras_unlock_bg) in the middle of the screen, beside one of the four extras'
 * pictures (art, diary, sound, movie) drawn at random, the whole popping in from nothing over a faint red flash of the
 * screen. Construct plays NewAnimation_1 (2.2 s) and its Delay 2 takes the widget off. BP_Collectable adds it to the
 * player's screen at Z 0.
 * TODO(item 29): the original also unlocks the picked extra in its own save (Extras_Art / Extras_SFX); nothing is kept
 * until the extras screen is made (the user's answer of 2026-09-20; the work list's item 29).
 *
 * The tree is built here as in the original, slot for slot, and the widget's own tick plays the animation and counts
 * the Delay, as UMG_VignetteSides' (UWasamiVignetteSidesWidget).
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCollectablesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiCollectablesWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * BP_Collectable's Create(Self, UMG_Collectables_C, None) and AddToPlayerScreen(0), for the first player (left off
	 * the screen where there is none, as in a test's world). Returns the widget.
	 */
	static UWasamiCollectablesWidget* Show(const UObject* WorldContextObject);

	/** Loads what the widget draws with (and plays) into Out, so that a pickup waits on nothing. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** NewAnimation_1's length (its playback range, [0, 132001) ticks), and Construct's Delay before RemoveFromParent. */
	static constexpr float AnimLength = 132001.f / 60000.f;
	static constexpr float RemoveDelay = 2.f;

	/** The Z order BP_Collectable adds the widget at (AddToPlayerScreen). */
	static constexpr int32 ZOrder = 0;

	/** Construct's words for TextBlock_150. */
	static FText UnlockedText();

	/**
	 * Construct's run from its start: NewAnimation_1 from 0 and the widget's words and picture (the tests call it alone;
	 * NativeConstruct calls it).
	 */
	void Begin();

	/** Moves NewAnimation_1 and the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Begin. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the Delay ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

	/** Construct's RandomIntegerInRange(0, 3): 0 art, 1 diary, 2 sound, 3 movie (INDEX_NONE before Begin, and on the secret's). */
	int32 GetIconIndex() const { return IconIndex; }

	/** CanvasPanel_0, Image_297 (the red flash), Image_89 (the frame), Image_249 (the picture) and TextBlock_150, for the tests. */
	UCanvasPanel* GetCanvas() const { return Canvas; }
	UImage* GetFlash() const { return Flash; }
	UImage* GetFrame() const { return Frame; }
	UImage* GetIcon() const { return Icon; }
	UTextBlock* GetTextBlock() const { return TextBlock; }

	/**
	 * NewAnimation_1's tracks at Seconds: CanvasPanel_0's scale (both axes alike) and RenderOpacity, and Image_297's
	 * RenderOpacity (its section ends at 60001 ticks, at 0).
	 */
	static float EvaluateScale(float Seconds);
	static float EvaluateOpacity(float Seconds);
	static float EvaluateFlashOpacity(float Seconds);

	/** extras_unlock_bg (Image_89). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UTexture2D> FrameTexture;

	/** Image_249's picture in the tree (art_icon; the secret's T_MysteryRoom). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UTexture2D> IconTexture;

	/** Construct's four pictures, by RandomIntegerInRange(0, 3): art_icon, diary_icon, sound_icon, movie_icon. */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TArray<TSoftObjectPtr<UTexture2D>> RandomIcons;

	/** WhiteSquareTexture, tinted red (Image_297). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UTexture2D> FlashTexture;

	/** helvetica-neue-bold_Font (TextBlock_150). */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<UFont> TextFont;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Construct's own part after PlayAnimation: the words and the picture drawn at random. */
	virtual void ConstructContent();

	/** TextBlock_150's font size and its slot's offsets (where the two widgets' trees differ). */
	float TextSize = 20.f;
	FMargin TextOffsets = FMargin(-90.441650390625f, 0.f, 151.f, 40.f);

	int32 IconIndex = INDEX_NONE;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Flash;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Icon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBlock;

	float Elapsed = 0.f;
	bool bFinished = false;
};

/**
 * The banner the secret room puts up the first time the player walks in, after UI/Main/UMG_Collectables_Secret: the same
 * tree and NewAnimation_1 as UMG_Collectables, with the room's picture (T_MysteryRoom) and YOU FOUND A MYSTERIOUS ROOM
 * a little smaller and further left. Construct plays DD_LVL2_15_V1_Secret_Mystery_Room_120818 at 0.5 first.
 * BP_SecretRoomZone adds it with CreateAndAddWidget at Z 1.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiCollectablesSecretWidget : public UWasamiCollectablesWidget
{
	GENERATED_BODY()

public:
	UWasamiCollectablesSecretWidget(const FObjectInitializer& ObjectInitializer);

	/** BP_SecretRoomZone's CreateAndAddWidget(UMG_Collectables_Secret_C, None, 1), for the first player. */
	static UWasamiCollectablesSecretWidget* Show(const UObject* WorldContextObject);

	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	static constexpr int32 SecretZOrder = 1;

	/** Construct's words, and its PlaySound2D's volume. */
	static FText FoundText();
	static constexpr float MusicVolume = 0.5f;

	/** DD_LVL2_15_V1_Secret_Mystery_Room_120818. */
	UPROPERTY(EditAnywhere, Category = "Secrets|Assets")
	TSoftObjectPtr<USoundBase> MusicSound;

protected:
	virtual void ConstructContent() override;
};
