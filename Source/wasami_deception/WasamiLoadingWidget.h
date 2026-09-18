#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiLoadingWidget.generated.h"

class UCanvasPanel;
class UImage;
class UTexture2D;

/**
 * The loading screen, after Dark Deception's UI/Main/UMG_Loading (pak_reference_2): a dark red screen with the level's
 * emblem in the middle, which fades in over 0.5 s and, 2.5 s on, out again; 1 s after that the widget takes itself off.
 * Its Construct empties the shards collected (the next level starts with all of them). Zone 1 adds it (Z 5) as it opens
 * Zone 2, which comes 2.5 s on, as the fade out starts.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiLoadingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The original's Enum_Levels value Zone 1 gives Level (06_ReachAmbulance). */
	static constexpr uint8 AsylumLevel = 7;
	/** AddToViewport's Z. */
	static constexpr int32 ZOrder = 5;
	/** FadeIn's length; Construct's Delay before it plays in reverse, and the Delay after, before RemoveFromParent. */
	static constexpr float FadeInLength = static_cast<float>(30001. / 60000.);
	static constexpr float FadeOutDelay = 2.5f;
	static constexpr float RemoveDelay = 1.f;

	/**
	 * Create(UMG_Loading) for the first player, Level set by name, AddToViewport(5): what 06_ReachAmbulance does.
	 * Returns the widget (null without a player controller).
	 */
	UFUNCTION(BlueprintCallable, Category = "Loading", meta = (WorldContext = "WorldContextObject"))
	static UWasamiLoadingWidget* Show(const UObject* WorldContextObject, uint8 InLevel);

	/** Which level's emblem Construct shows. */
	UPROPERTY(BlueprintReadWrite, Category = "Loading")
	uint8 Level = 0;

	/** Moves FadeIn and the delays on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Seconds since Construct. */
	float GetElapsed() const { return Elapsed; }

	/** Whether the delays ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

	/** CanvasPanel_0's RenderOpacity Seconds after Construct: FadeIn forwards, and backwards from 2.5 s. */
	static float EvaluateOpacity(float Seconds);

	/** FadeIn's RenderOpacity at Seconds. */
	static float EvaluateFadeIn(float Seconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * The emblems Construct picks by Level (the original's UI/Main/Loaders: loader_monkey, _agatha, _watcher, none, none,
	 * _ducky, _gremclown, _reapernurse, _lucky, _triggerteddy, none, none). Left empty until the user says whether
	 * they may be used; a level without one shows the red screen alone.
	 */
	UPROPERTY(EditAnywhere, Category = "Loading|Assets")
	TArray<TSoftObjectPtr<UTexture2D>> LevelEmblems;

private:
	void ApplyAnimation();

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> Root;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Logo;

	float Elapsed = 0.f;
	bool bFinished = false;
};
