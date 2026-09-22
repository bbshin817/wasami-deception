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
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// loop (pak_reference_2's UMG_ChapterPortal; the old version's keys are the same), 10 s. CanvasPanel_0 fades in from
	// 0.8 s to 2 s and out from 8.5 s to 10 s.
	const FAnimKey PortalCanvasOpacityKeys[] = {
		{0., 0.f, 0., 0.},
		{48000., 0.f, 0., 0.},
		{120000., 1.f, 1.9607844023994403e-06, 1.9607844023994403e-06},
		{510000., 1.f, -2.0833333564951317e-06, -2.0833333564951317e-06},
		{600000., 0.f, 0., 0.},
	};
	// BackgroundBlur_0: up to 10 by 2 s (the 2 s key is linear: flat to 8.5 s), down to 0 by 9.75 s.
	const FAnimKey PortalBlurKeys[] = {
		{0., 0.f, 0., 0.},
		{120000., 10.f, 8.333333244081587e-05, 0., RCIM_Linear},
		{510000., 10.f, 0., -2.1505375116248615e-05},
		{585000., 0.f, 0., 0.},
	};
	// The banners' left offsets (CanvasPanelSlot_11 and _13), straight across the 10 s (the first key is linear).
	const FAnimKey PortalBannerLeftKeys[] = {
		{0., 501.624755859375f, -0.003738461062312126, -0.00271038431674242, RCIM_Linear},
		{600000., -1124.6058349609375f, -0.00271038431674242, 0.},
	};
	const FAnimKey PortalBanner2LeftKeys[] = {
		{0., 1103.771484375f, 0., -0.0036994460970163345, RCIM_Linear},
		{600000., -1115.896240234375f, -0.0036994460970163345, 0.},
	};
	// The ring (Image_40) turns to 800° and the runes (Image_41) back to −359°, straight across the 10 s.
	const FAnimKey PortalRingAngleKeys[] = {
		{0., 0.f, 0., 0.0013333333190530539, RCIM_Linear},
		{600000., 800.f, 0.0013333333190530539, 0.},
	};
	const FAnimKey PortalRunesAngleKeys[] = {
		{0., 0.f, 0., -0.0005983333103358746, RCIM_Linear},
		{600000., -359.f, -0.0005983333103358746, 0.},
	};
	// TitleCard lands from 2 s: its opacity 0 → 1 and its scale (x and y alike) 1.5 → 1 by 2.2 s, then bounces to 1.15
	// and back by 2.35 s.
	const FAnimKey PortalTitleOpacityKeys[] = {
		{0., 0.f, 0., 0.},
		{120000., 0.f, 7.5757575359602924e-06, 7.5757575359602924e-06},
		{132000., 1.f, 0., 0.},
	};
	const FAnimKey PortalTitleScaleKeys[] = {
		{120000., 1.5f, 0., 0.},
		{132000., 1.f, -2.333333577553276e-05, -2.333333577553276e-05},
		{135000., 1.149999976158142f, 0., 0.},
		{141000., 1.f, 0., 0.},
	};
	// Its colour: red, held (the 2.2 s key is constant) until it flashes white at 2.25 s, red again by 2.35 s, dark at
	// 2.4 s and red from 2.55 s. Green and blue share their keys; the alpha is keyed 1 throughout.
	const FAnimKey PortalTitleRedKeys[] = {
		{120000., 1.f, 0., 0.},
		{132000., 1.f, 0., 0., RCIM_Constant},
		{135000., 1.f, 0., 0.},
		{141000., 1.f, -8.159717253874987e-05, -8.159717253874987e-05},
		{144000., 0.265625f, 0., 0.},
		{152999., 1.f, 3.059897062485106e-05, 3.059897062485106e-05},
		{167999., 1.f, 0., 0.},
	};
	const FAnimKey PortalTitleGreenBlueKeys[] = {
		{120000., 0.f, 0., 0.},
		{132000., 0.f, 5.5555567087139934e-05, 0., RCIM_Constant},
		{135000., 1.f, 0., 0.},
		{141000., 0.f, -0.00011111103958683088, -0.00011111103958683088},
		{144000., 0.f, 0., 0.},
		{152999., 0.f, 0., 0.},
		{167999., 0.f, 0., 0.},
	};
	// CanvasPanel_0's jolt as the title lands (its translation, 2.2 s to 2.35 s).
	const FAnimKey PortalShakeXKeys[] = {
		{132000., 0.f, 0., 0.},
		{135000., -10.f, 0.00116666778922081, 0.00116666778922081},
		{137999., 7.f, 0.0016666642623022199, 0.0016666642623022199},
		{141000., 0.f, 0., 0.},
	};
	const FAnimKey PortalShakeYKeys[] = {
		{132000., 0.f, 0., 0.},
		{135000., 3.f, -0.000666667299810797, -0.000666667299810797},
		{137999., -4.f, -0.0004999993252567947, -0.0004999993252567947},
		{141000., 0.f, 0., 0.},
	};

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
const FVector2D UWasamiChapterPortalWidget::HeadOffset(0.f, -11.3f);
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

void UWasamiChapterPortalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct (after putting Level's head and title in, which the tree has here): loop from 0.8 s once, forward, at
	// speed 1, and Delay 11 → RemoveFromParent. Playing sets the first frame at once, so the card starts unseen.
	Elapsed = 0.f;
	bFinished = false;
	ApplyLoop();
}

void UWasamiChapterPortalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiChapterPortalWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Elapsed = FMath::Min(Elapsed + DeltaSeconds, RemoveDelay);
	ApplyLoop();
	if (Elapsed >= RemoveDelay)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiChapterPortalWidget::ApplyLoop()
{
	if (!Root)
	{
		return;
	}
	// Once loop has ended (9.2 s after Construct) its last frame stays, the card faded out, until the delay takes it off.
	const FWasamiChapterPortalPose Pose = EvaluateLoop(GetLoopTime());
	Root->SetRenderOpacity(Pose.CanvasOpacity);
	Root->SetRenderTranslation(Pose.CanvasShake);
	Blur->SetBlurStrength(Pose.BlurStrength);
	auto SlideBanner = [](UImage* Image, float Left)
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot))
		{
			FMargin Offsets = Slot->GetOffsets();
			Offsets.Left = Left;
			Slot->SetOffsets(Offsets);
		}
	};
	SlideBanner(Banner, Pose.BannerLeft);
	SlideBanner(Banner2, Pose.Banner2Left);
	Ring->SetRenderTransformAngle(Pose.RingAngle);
	Runes->SetRenderTransformAngle(Pose.RunesAngle);
	TitleCard->SetRenderOpacity(Pose.TitleOpacity);
	TitleCard->SetRenderScale(FVector2D(Pose.TitleScale, Pose.TitleScale));
	TitleCard->SetColorAndOpacity(Pose.TitleColor);
}

FWasamiChapterPortalPose UWasamiChapterPortalWidget::EvaluateLoop(float Seconds)
{
	// A curve holds its first key's value before it and its last after it, which is what each section does outside its
	// range: before it the widget's own value (the same), after it the last (KeepState).
	static const FRichCurve CanvasOpacity = MakeCurve(PortalCanvasOpacityKeys);
	static const FRichCurve BlurStrength = MakeCurve(PortalBlurKeys);
	static const FRichCurve BannerLeft = MakeCurve(PortalBannerLeftKeys);
	static const FRichCurve Banner2Left = MakeCurve(PortalBanner2LeftKeys);
	static const FRichCurve RingAngle = MakeCurve(PortalRingAngleKeys);
	static const FRichCurve RunesAngle = MakeCurve(PortalRunesAngleKeys);
	static const FRichCurve TitleOpacity = MakeCurve(PortalTitleOpacityKeys);
	static const FRichCurve TitleScale = MakeCurve(PortalTitleScaleKeys);
	static const FRichCurve TitleRed = MakeCurve(PortalTitleRedKeys);
	static const FRichCurve TitleGreenBlue = MakeCurve(PortalTitleGreenBlueKeys);
	static const FRichCurve ShakeX = MakeCurve(PortalShakeXKeys);
	static const FRichCurve ShakeY = MakeCurve(PortalShakeYKeys);

	FWasamiChapterPortalPose Pose;
	Pose.CanvasOpacity = Eval(CanvasOpacity, Seconds, LoopLength);
	Pose.CanvasShake = FVector2D(Eval(ShakeX, Seconds, LoopLength), Eval(ShakeY, Seconds, LoopLength));
	Pose.BlurStrength = Eval(BlurStrength, Seconds, LoopLength);
	Pose.BannerLeft = Eval(BannerLeft, Seconds, LoopLength);
	Pose.Banner2Left = Eval(Banner2Left, Seconds, LoopLength);
	Pose.RingAngle = Eval(RingAngle, Seconds, LoopLength);
	Pose.RunesAngle = Eval(RunesAngle, Seconds, LoopLength);
	Pose.TitleOpacity = Eval(TitleOpacity, Seconds, LoopLength);
	Pose.TitleScale = Eval(TitleScale, Seconds, LoopLength);
	const float GreenBlue = Eval(TitleGreenBlue, Seconds, LoopLength);
	Pose.TitleColor = FLinearColor(Eval(TitleRed, Seconds, LoopLength), GreenBlue, GreenBlue, 1.f);
	return Pose;
}

void UWasamiChapterPortalWidget::BuildScreen(UCanvasPanel* InRoot)
{
	// Each slot is the original's (UMG_ChapterPortal's WidgetTree), in its order; what the export leaves out is the slot's
	// default. The blur and the wash reach a little past the screen's edges. Where loop keys a value (the banners' left
	// offsets, the angles, the opacities, the blur), the tree's is left as the designer saved it; Construct replaces it.
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
	// Ours is the pause menu's head, low in its picture; HeadOffset lifts it into the middle of the ring. Moving the
	// picture instead would move the pause menu's head with it.
	Logo->SetRenderTranslation(HeadOffset);
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
