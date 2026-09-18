#include "WasamiInteractWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "WasamiAssets.h"

UWasamiInteractWidget::UWasamiInteractWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IconTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/interact_icon_03")));
}

TSharedRef<SWidget> UWasamiInteractWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		// Image_18: the icon at 90 × 90, white at half opacity, drawn at half scale; never in the way of the mouse.
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_18"));
		FSlateBrush Brush;
		Brush.SetResourceObject(IconTexture.LoadSynchronous());
		Brush.ImageSize = FVector2D(90.f, 90.f);
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.5f));
		Image->SetRenderScale(FVector2D(0.5f, 0.5f));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);

		// Anchored at the centre, at its own size (bAutoSize), centred on the anchor; of the offsets only Bottom (40)
		// was changed from UMG's default, and with bAutoSize neither Right nor Bottom sizes it.
		UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Image);
		FAnchorData Layout;
		Layout.Anchors = FAnchors(0.5f, 0.5f);
		Layout.Offsets = FMargin(0.f, 0.f, 100.f, 40.f);
		Layout.Alignment = FVector2D(0.5f, 0.5f);
		ImageSlot->SetLayout(Layout);
		ImageSlot->SetAutoSize(true);
	}
	return Super::RebuildWidget();
}
