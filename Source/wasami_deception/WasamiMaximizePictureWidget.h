#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiMaximizePictureWidget.generated.h"

class UBackgroundBlur;
class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class UScaleBox;
class USoundBase;
class UTextBlock;
class UTexture2D;

/**
 * A picture of the extras screen seen large, after Dark Deception's UI/Main/TitleScreen/UMG_MaximizePicture
 * (pak_reference_2): the screen darkened by half and blurred (6), and over it the picture on a black ground 50 px wider
 * each way, fitted into the screen 100 px in from its edges, its Text at the top (helvetica-normal 30, shadowed) and BACK
 * at the bottom (dark grey, white while hovered). Construct plays FadeIn at twice its speed (the whole fades in over
 * 0.25 s while the picture grows from half its size over 0.125 s); BACK plays it backwards alike, sounds UI_Select_V3
 * (pitch 0.7) and takes the view off 0.25 s later. The extras screen's pictures open it at Z 3.
 *
 * The tree is built here as in the original, slot for slot, and the widget's own tick plays FadeIn and the Delay.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiMaximizePictureWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiMaximizePictureWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * UMG_Extras_Extra's click (@10): Create(Self, UMG_MaximizePicture_C, None) with Texture and Text set by name and
	 * AddToViewport(3), for the first player. Returns the view (null without a player controller).
	 */
	static UWasamiMaximizePictureWidget* Show(const UObject* WorldContextObject, UTexture2D* InTexture, const FText& InText);

	/** Texture: the picture (Image_1's brush, at the picture's own size). */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture;

	/** Text: TextBlock_1's words (its binding). */
	UPROPERTY(Transient)
	FText Text;

	static constexpr int32 ViewportZOrder = 3;

	/** FadeIn's length (its playback range, [0, 30001) ticks) and the speed both of its plays go at. */
	static constexpr float FadeInLength = 30001.f / 60000.f;
	static constexpr float FadeInSpeed = 2.f;

	/** BACK's select sound's pitch and its Delay before the view goes. */
	static constexpr float BackPitch = 0.7f;
	static constexpr float BackDelay = 0.25f;

	/** Unhovered Color: BACK's colour until it is hovered (white while it is). */
	static constexpr float UnhoveredGrey = 0.11f;

	/**
	 * Construct (@409 → @305): PlayAnimation(FadeIn, 0, 1, Forward, 2) and Image_1.SetBrushFromTexture(Texture, True).
	 * NativeConstruct calls it (the tests call it alone, without a world).
	 */
	void Begin();

	/** Moves FadeIn and BACK's Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/**
	 * BACK (@144): PlayAnimation(FadeIn, 0, 1, Reverse, 2), which starts over from the end each time, PlaySound2D
	 * (UI_Select_V3, 1, 0.7) and Delay(0.25) → RemoveFromParent (a pending Delay ignores another).
	 */
	void PressBack();

	/** BACK's hover (@238: white) and unhover (@196: Unhovered Color). */
	void HoverBack(bool bHovered);

	/** Seconds into FadeIn (going down while it plays backwards). */
	float GetFadeInTime() const { return FadeInTime; }

	/** Whether BACK has been pressed, and whether the view has taken itself off. */
	bool IsClosing() const { return bClosing; }
	bool IsFinished() const { return bFinished; }

	/**
	 * FadeIn's tracks at Seconds: CanvasPanel_0's (the whole view's) RenderOpacity 0 → 1 over 0.5 s, and Button_0's
	 * (the picture's black ground's) RenderTransform scale 0.5 → 1 over 0.25 s.
	 */
	static float EvaluateOpacity(float Seconds);
	static float EvaluateScale(float Seconds);

	/** The widgets, for the tests. */
	UCanvasPanel* GetRoot() const { return Root; }
	UButton* GetDim() const { return Dim; }
	UBackgroundBlur* GetBlur() const { return Blur; }
	UScaleBox* GetScaleBox() const { return ScaleBox; }
	UButton* GetGround() const { return Ground; }
	UImage* GetImage() const { return Image; }
	UTextBlock* GetTextBlock() const { return TextBlock; }
	UButton* GetBackButton() const { return BackButton; }

	/** helvetica-normal_Font (the Text) and helvetica-neue-bold_Font (BACK). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> NormalFont;

	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UFont> BoldFont;

	/** The engine's WhiteSquareTexture (the picture's black ground). */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<UTexture2D> WhiteTexture;

	/** UI_Select_V3. */
	UPROPERTY(EditAnywhere, Category = "Extras|Assets")
	TSoftObjectPtr<USoundBase> SelectSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyAnimation();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnBackHovered();

	UFUNCTION()
	void OnBackUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Root;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Dim;

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> Blur;

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> ScaleBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Ground;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;

	float FadeInTime = 0.f;
	bool bFadeInReverse = false;
	bool bClosing = false;
	float CloseElapsed = 0.f;
	bool bFinished = false;
};
