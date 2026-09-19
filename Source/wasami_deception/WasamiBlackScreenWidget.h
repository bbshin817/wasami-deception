#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiBlackScreenWidget.generated.h"

class UImage;

/**
 * A black screen for a moment, after Dark Deception's Blueprints/UMG/UMG_BlackScreen (pak_reference_2): Image_29, black
 * over the whole screen; its Construct waits Duration (a Delay) and takes the widget off the screen. (Its FadeIn and
 * FadeOut are never played.) The saw traps add it (Z 0) between catching the player and the death.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiBlackScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** CreateAndAddWidget(UMG_BlackScreen) for the first player at ZOrder. Returns it (null without a player controller). */
	static UWasamiBlackScreenWidget* Show(const UObject* WorldContextObject, int32 ZOrder);

	/** Duration: Construct's Delay before RemoveFromParent. */
	UPROPERTY(BlueprintReadWrite, Category = "Black Screen")
	float Duration = 1.f;

	/** Moves the Delay on by DeltaSeconds (NativeTick calls it). */
	void Advance(float DeltaSeconds);

	/** Whether the Delay ran out and the widget took itself off. */
	bool IsFinished() const { return bFinished; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;

	float Elapsed = 0.f;
	bool bFinished = false;
};
