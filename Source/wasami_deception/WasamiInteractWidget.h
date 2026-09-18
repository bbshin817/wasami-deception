#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "WasamiInteractWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * The hand in the middle of the screen, after Dark Deception's Blueprints/UMG/UMG_Interact (pak_reference_2): one
 * image of interact_icon_03, 90 × 90 at half opacity and half scale, just above the centre. The player adds it at
 * BeginPlay, collapsed, and shows it while it looks at something it can use.
 */
UCLASS()
class WASAMI_DECEPTION_API UWasamiInteractWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWasamiInteractWidget(const FObjectInitializer& ObjectInitializer);

	UImage* GetImage() const { return Image; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** UI/Main/interact_icon_03. */
	UPROPERTY(EditAnywhere, Category = "Interact|Assets")
	TSoftObjectPtr<UTexture2D> IconTexture;

private:
	UPROPERTY(Transient)
	TObjectPtr<UImage> Image;
};
