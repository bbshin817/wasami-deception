#include "WasamiBlackScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UWasamiBlackScreenWidget* UWasamiBlackScreenWidget::Show(const UObject* WorldContextObject, int32 ZOrder)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiBlackScreenWidget* Widget = CreateWidget<UWasamiBlackScreenWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->AddToViewport(ZOrder);
	}
	return Widget;
}

TSharedRef<SWidget> UWasamiBlackScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		// Image_29: black, anchored to the whole screen with no offsets.
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_29"));
		Image->SetColorAndOpacity(FLinearColor::Black);
		UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Image);
		FAnchorData Layout;
		Layout.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		Layout.Offsets = FMargin(0.f);
		ImageSlot->SetLayout(Layout);
	}
	return Super::RebuildWidget();
}

void UWasamiBlackScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Construct: Delay(Duration) → RemoveFromParent.
	Elapsed = 0.f;
	bFinished = false;
}

void UWasamiBlackScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiBlackScreenWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// The widget's own tick (a widget's Delay counts in it), so it goes on while the death screen pauses the game.
	Elapsed += DeltaSeconds;
	if (Elapsed >= Duration)
	{
		bFinished = true;
		RemoveFromParent();
	}
}
