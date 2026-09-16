#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiTelepathyTrackerWidget.generated.h"

class UImage;
class UMaterialInterface;
class USizeBox;

/**
 * Dark Deception's UMG_TelepathyTracker (pak_reference_2's Blueprints/Main/Powers/Telepathy/UMG_TelepathyTracker), the
 * marker a telepathy tracker shows over an enemy: a red, smoky, additive disc (MM_Telepathy_Inst) in a 256 × 256 size
 * box. Its Construct plays the 0.5 s animation Appear (the image grows from nothing and fades in), gives the image's
 * material a random Tiling and Speed and the image a random angle; Remove plays the 0.3 s Disappear (a swell, then
 * gone); Set Size scales the size box. Both animations keep their last values when they end.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiTelepathyTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiTelepathyTrackerWidget(const FObjectInitializer& ObjectInitializer);

	virtual bool Initialize() override;

	/** Loads what the widget shows into Out, so that the first telepathy does not wait for it. */
	static void LoadAssets(TArray<TObjectPtr<UObject>>& Out);

	/** Appear's tracks (image scale and opacity) at a time (s) of its 0.5 s. */
	static float EvaluateAppearScale(float Seconds);
	static float EvaluateAppearOpacity(float Seconds);
	/** Disappear's tracks at a time (s) of its 0.3 s. */
	static float EvaluateDisappearScale(float Seconds);
	static float EvaluateDisappearOpacity(float Seconds);

	/** The animations' last evaluated times (s): the tick before each one's playback range ends. */
	static constexpr float AppearLength = 0.5f;
	static constexpr float DisappearLength = 0.3f;

	/** Remove: plays Disappear from its start. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	void Remove();

	/** Set Size: the size box's render scale, on both axes. */
	UFUNCTION(BlueprintCallable, Category = "Telepathy")
	void SetSize(float Size);

	/** MM_Telepathy_Inst (its parent's graph is an estimate). */
	UPROPERTY(EditAnywhere, Category = "Telepathy")
	TSoftObjectPtr<UMaterialInterface> TrackerMaterial;

	bool IsAppearPlaying() const { return bAppearPlaying; }
	bool IsDisappearPlaying() const { return bDisappearPlaying; }
	USizeBox* GetSizeBox() const { return SizeBox; }
	UImage* GetImage() const { return Image; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyAppear();
	void ApplyDisappear();

	/** SizeBox_54: 256 × 256, the root. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	/** Image_90: MM_Telepathy_Inst, filling the size box. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;

	float AppearTime = 0.f;
	float DisappearTime = 0.f;
	bool bAppearPlaying = false;
	bool bDisappearPlaying = false;
};
