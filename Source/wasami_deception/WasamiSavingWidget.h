#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiSavingWidget.generated.h"

class UCanvasPanel;
class UFont;
class UTextBlock;
class UTexture2D;
class UThrobber;

/**
 * SAVING PROGRESS, after Dark Deception's UI/Main/UMG_Saving (pak_reference_2): the words and a one-piece throbber at
 * the bottom right, which its animation init fades up to half and out again over 3 s; then the widget takes itself off
 * the screen. The levels add it (Z 0) where they save a checkpoint.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiSavingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiSavingWidget(const FObjectInitializer& ObjectInitializer);

	/** CreateAndAddWidget(UMG_Saving) for the first player, at Z 0. Returns the widget (null without a player controller). */
	UFUNCTION(BlueprintCallable, Category = "Save", meta = (WorldContext = "WorldContextObject"))
	static UWasamiSavingWidget* Show(const UObject* WorldContextObject);

	/** init's length, and Construct's Delay before RemoveFromParent. */
	static constexpr float InitLength = 3.f;
	static constexpr float RemoveDelay = 3.f;

	/** Moves init and the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Construct. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the Delay ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

	/** init's RenderOpacity at Seconds: the words' track and the throbber's. */
	static float EvaluateTextOpacity(float Seconds);
	static float EvaluateThrobberOpacity(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** The engine's RobotoTiny (its Light typeface). */
	UPROPERTY(EditAnywhere, Category = "Save|Assets")
	TSoftObjectPtr<UFont> TextFont;

	/** The engine's SphereRenderHeightMap: the throbber's piece. */
	UPROPERTY(EditAnywhere, Category = "Save|Assets")
	TSoftObjectPtr<UTexture2D> ThrobberTexture;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text;

	UPROPERTY(Transient)
	TObjectPtr<UThrobber> Throbber;

	float Elapsed = 0.f;
	bool bFinished = false;
};
