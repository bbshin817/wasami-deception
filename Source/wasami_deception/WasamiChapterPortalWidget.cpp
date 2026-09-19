#include "WasamiChapterPortalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiPauseWidget.h"

namespace
{
	UCanvasPanelSlot* PlaceInPortal(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment = FVector2D::ZeroVector, bool bAutoSize = false)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = Anchors;
		Layout.Offsets = Offsets;
		Layout.Alignment = Alignment;
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
		return Slot;
	}

	FSlateBrush PortalBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}
}

const FVector2D UWasamiChapterPortalWidget::BannerSize(4165.f, 872.f);
const FVector2D UWasamiChapterPortalWidget::TitleSize(901.f, 180.f);
const FLinearColor UWasamiChapterPortalWidget::WashColor(0.109375f, 0.f, 0.f, 0.2f);
const FLinearColor UWasamiChapterPortalWidget::TitleColor(1.f, 0.f, 0.f, 1.f);

UWasamiChapterPortalWidget::UWasamiChapterPortalWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RingTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/chapter_ui_portal_outer")));
	RunesTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/TitleCards/chapter_title_portal_inner")));
	BannerTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/TitleCards/chapter_ui_banner_bg_01")));
	Banner2Texture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/TitleCards/chapter_ui_banner_bg_02")));
	HeadTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/Pause/T_PauseHead")));
	TitleTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/T_LevelTitle")));
}

UWasamiChapterPortalWidget* UWasamiChapterPortalWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiChapterPortalWidget* Card = CreateWidget<UWasamiChapterPortalWidget>(Controller, StaticClass());
	if (Card)
	{
		Card->AddToViewport(ViewportZOrder);
	}
	return Card;
}

TSharedRef<SWidget> UWasamiChapterPortalWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Panel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Panel;
		BuildScreen(Panel);
	}
	return Super::RebuildWidget();
}

void UWasamiChapterPortalWidget::BuildScreen(UCanvasPanel* InRoot)
{
	// Each slot is the original's (UMG_ChapterPortal's WidgetTree), in its order; what the export leaves out is the slot's
	// default. The blur and the wash reach a little past the screen's edges.
	Root = InRoot;
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
	const FAnchors Middle(0.5f, 0.5f);
	const FVector2D Centred(0.5f, 0.5f);

	// BackgroundBlur_0: nothing in it, at strength 0 until the animation.
	Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
	PlaceInPortal(Root, Blur, Fill, FMargin(-40.96381378173828f, -34.0782585144043f, -21.2840576171875f, -33.6737060546875f));

	// Image_1: the wash, a faint dark red (no texture).
	UImage* Wash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_1"));
	FSlateBrush WashBrush;
	WashBrush.TintColor = FSlateColor(WashColor);
	Wash->SetBrush(WashBrush);
	PlaceInPortal(Root, Wash, Fill, FMargin(-13.51351261138916f, -15.0150146484375f, -8.4083251953125f, -13.4835205078125f));

	// Image_48 and Image_49: the banners at their size, tiled across, centred on points left and right of the middle
	// (the animation slides them).
	auto MakeBanner = [this, &Middle, &Centred](const TCHAR* Name, UTexture2D* Texture, float Left, float Top)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		FSlateBrush Brush = PortalBrush(Texture, BannerSize);
		Brush.Tiling = ESlateBrushTileType::Horizontal;
		Image->SetBrush(Brush);
		PlaceInPortal(Root, Image, Middle, FMargin(Left, Top, 1898.798583984375f, 340.30029296875f), Centred, true);
		return Image;
	};
	Banner = MakeBanner(TEXT("Image_48"), BannerTexture.LoadSynchronous(), -1124.6058349609375f, 15.859466552734375f);
	Banner2 = MakeBanner(TEXT("Image_49"), Banner2Texture.LoadSynchronous(), 1103.771484375f, 19.45947265625f);

	// Icon: the portal, 470 square (its children's size), its corner a little up and left of a point a quarter across
	// and a little above the middle; the ring, the runes and the head centred in it.
	UCanvasPanel* Icon = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Icon"));
	const FVector2D Portal(PortalSize, PortalSize);
	Ring = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_40"));
	Ring->SetBrush(PortalBrush(RingTexture.LoadSynchronous(), Portal));
	Ring->SetRenderTransformAngle(800.f);
	PlaceInPortal(Icon, Ring, Middle, FMargin(0.f), Centred, true);
	Runes = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_41"));
	Runes->SetBrush(PortalBrush(RunesTexture.LoadSynchronous(), Portal));
	Runes->SetRenderTransformAngle(-359.f);
	PlaceInPortal(Icon, Runes, Middle, FMargin(0.f, 0.f, 100.f, 30.f), Centred, true);
	UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Logo"));
	Logo->SetBrush(PortalBrush(HeadTexture.LoadSynchronous(), Portal, UWasamiPauseWidget::HeadTint()));
	PlaceInPortal(Icon, Logo, Middle, FMargin(0.f), Centred, true);
	PlaceInPortal(Root, Icon, FAnchors(0.25178566575050354f, 0.47777774930000305f),
		FMargin(-239.91238403320312f, -236.5164794921875f, 100.f, 30.f), FVector2D::ZeroVector, true);

	// TitleCard: the title at its size, red, its corner right of the portal and moved (−72, 23) by its render transform.
	TitleCard = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleCard"));
	TitleCard->SetBrush(PortalBrush(TitleTexture.LoadSynchronous(), TitleSize));
	TitleCard->SetColorAndOpacity(TitleColor);
	TitleCard->SetRenderTranslation(FVector2D(-72.f, 23.f));
	PlaceInPortal(Root, TitleCard, FAnchors(0.36875003576278687f, 0.4888889193534851f),
		FMargin(71.37957763671875f, -116.528564453125f, 100.f, 40.f), FVector2D::ZeroVector, true);
}
