#include "WasamiTabletWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Curves/RichCurve.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WasamiAssets.h"

namespace
{
	// The original's UMG_Tablet, flattened to the screen's own px (its canvases nest with anchors at the middle of a
	// 714 × 864 widget; the numbers here are those rectangles worked out once, and the comment says where each comes
	// from). All of it is pak_reference's UI/Tablet/UMG_Tablet and UI/Tablet/UMG_TabletPowers.

	// Overlay_102 (13.3467, 142.3646 .. 6.6733, 13.3467 from the edges) inset by the overlay slot's padding of 34.
	constexpr float MapLeft = 47.3467f;
	constexpr float MapTop = 176.3646f;
	constexpr float MapRight = 673.3267f;
	constexpr float MapBottom = 816.6533f;

	// CanvasPanel_4 in CanvasPanel_1 (164.16989, 32 .. 164.97385, 705.0459 from the edges).
	constexpr float CountLeft = 164.16989f;
	constexpr float CountTop = 32.f;
	constexpr float CountRight = 549.02615f;
	constexpr float CountBottom = 158.9541f;

	// UMG_TabletPowers sits at (−356.9609375, −428.54052734375) from the middle and its CanvasPanel_2 at (−355, −420)
	// from its own, so its inside starts (2.0390625, 15.45947265625) from the screen's corner; CanvasPanel_3 and
	// CanvasPanel_4 are (23.432003, 32 .. 562.253967, 705.0459) and (560, 32 .. 25, 705) from that canvas' edges.
	constexpr float PowersX = 2.0390625f;
	constexpr float PowersY = 15.45947265625f;
	constexpr float LeftSocketLeft = PowersX + 23.432003f;
	constexpr float LeftSocketRight = PowersX + 714.f - 562.253967f;
	constexpr float RightSocketLeft = PowersX + 560.f;
	constexpr float RightSocketRight = PowersX + 714.f - 25.f;
	constexpr float SocketTop = PowersY + 32.f;
	constexpr float LeftSocketBottom = PowersY + 864.f - 705.0459f;
	constexpr float RightSocketBottom = PowersY + 864.f - 705.f;

	// Image_212 (60 × 60 at (−29.133867, −27.701376) from the middle of the map) and Button_0 (anchored to the middle
	// of the map's bottom, (−324.2652588, −35.3987160), 644.7755 × 49.1100).
	constexpr float MarkSize = 60.f;
	constexpr float MarkOffsetX = -29.133867f;
	constexpr float MarkOffsetY = -27.701376f;
	constexpr float BandOffsetX = -324.2652588f;
	constexpr float BandOffsetY = -35.3987160f;
	constexpr float BandWidth = 644.7755f;
	constexpr float BandHeight = 49.1100f;

	// TextBlock_107 "Z" at (−304, 312) from the middle, RenderOpacity 0.1.
	constexpr float ZoomKeyX = -304.f;
	constexpr float ZoomKeyY = 312.f;
	constexpr float ZoomKeyOpacity = 0.1f;

	// ShardCount's font is Size 100 with a 4 px outline; the objective and the "Z" keep the widget's default 24.
	constexpr float ShardFontSize = 100.f;
	constexpr int32 ShardOutlineSize = 4;
	constexpr float BandFontSize = 24.f;
	// ShardCount's Margin.Top and its RenderTransform's translation.
	constexpr float ShardMarginTop = -22.f;
	constexpr float ShardTranslationY = -12.f;

	// Button_0's Normal brush is the white square tinted this dark grey; Image_41 is T_Vignette tinted purple and
	// starts invisible (the shard flash plays with it).
	const FLinearColor BandColour(0.043735f, 0.043735f, 0.043735f, 1.f);
	const FLinearColor FlashColour(0.485150f, 0.f, 1.f, 1.f);

	// UMG_TabletPowers (pak_reference_2): the MM_Powers instance of each power, in EWasamiPower order.
	const TCHAR* const PowerMaterialPaths[WasamiPowerCount] = {
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_SpeedBoost"),
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_Inst_Teleport"),
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_Inst_Telepathy"),
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_PrimalFear"),
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_Inst_Telekinesis"),
		TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_Vanish"),
	};

	// Use Left / Use Right: a Scale track on the socket's canvas, cubic keys at ticks 0 / 3000 / 9000 / 30000 (60000 a
	// second) with the tangents the export gives per tick (1.1111e-5 and -9.2593e-6), here per second.
	constexpr float BounceLength = 0.5f;

	FRichCurve MakeBounceCurve()
	{
		FRichCurve Curve;
		const float Keys[][2] = {{0.f, 1.f}, {0.05f, 1.25f}, {0.15f, 1.1f}, {BounceLength, 1.f}};
		const float Tangents[] = {0.f, 0.66666683f, -0.55555554f, 0.f};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(Keys[Index][0], Keys[Index][1]));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = Tangents[Index];
			Key.LeaveTangent = Tangents[Index];
		}
		return Curve;
	}

	// Count Shake (UMG_Tablet, the same in both versions): cubic keys at ticks 0 / 3000 / 6000 / 12000 (60000 a second)
	// with their tangents per tick, here per second. The count's 2D transform section keys its translation and scale and
	// restores its state at the end; Image_41's colour section keys only the alpha.
	constexpr double CountShakeTicksPerSecond = 60000.;
	struct FCountShakeKey
	{
		double Ticks;
		float Value;
		double TangentPerTick;
	};
	const FCountShakeKey CountShakeXKeys[] = {{0., 0.f, 0.}, {3000., -14.f, 0.}, {6000., 0.f, 0.0015555555000901222}, {12000., 0.f, 0.}};
	const FCountShakeKey CountShakeYKeys[] = {{0., 0.f, 0.}, {3000., 9.f, -0.0020000000949949026}, {6000., -12.f, -0.0010000000474974513}, {12000., 0.f, 0.}};
	const FCountShakeKey CountShakeScaleKeys[] = {{0., 1.f, 0.}, {3000., 1.100000023841858f, 0.}, {6000., 1.f, 0.}};
	const FCountShakeKey CountShakeFlashKeys[] = {{0., 0.25f, 0.}, {12000., 0.f, 0.}};

	FRichCurve MakeCountShakeCurve(TConstArrayView<FCountShakeKey> Keys)
	{
		FRichCurve Curve;
		for (const FCountShakeKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(static_cast<float>(Each.Ticks / CountShakeTicksPerSecond), Each.Value));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = static_cast<float>(Each.TangentPerTick * CountShakeTicksPerSecond);
			Key.LeaveTangent = Key.ArriveTangent;
		}
		return Curve;
	}

	UCanvasPanelSlot* PlaceBox(UCanvasPanel* Panel, UWidget* Child, float X, float Y, float Width, float Height)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetPosition(FVector2D(X, Y));
		Slot->SetSize(FVector2D(Width, Height));
		return Slot;
	}

	UCanvasPanelSlot* PlaceFill(UCanvasPanel* Panel, UWidget* Child)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
		return Slot;
	}
}

UWasamiTabletWidget::UWasamiTabletWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BackgroundTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Tablet/tablet_screen_bg")));
	PlayerMarkTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Tablet/tablet_map_player")));
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_Vignette")));
	MapMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/Pipeline/Materials/M_DD_MapScreen")));
	for (const TCHAR* Path : PowerMaterialPaths)
	{
		PowerMaterials.Add(TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(Path)));
	}
	ScreenFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

TSharedRef<SWidget> UWasamiTabletWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Screen"));
		WidgetTree->RootWidget = Root;
		BuildScreen(Root);
	}
	return Super::RebuildWidget();
}

void UWasamiTabletWidget::BuildScreen(UCanvasPanel* Root)
{
	// The brushes, fonts and material instances below hold on to what is loaded here.
	UFont* Font = ScreenFont.LoadSynchronous();
	auto MakeFont = [Font](float Size, int32 Outline)
	{
		FSlateFontInfo Info;
		Info.FontObject = Font;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		Info.OutlineSettings.OutlineSize = Outline;
		return Info;
	};

	// Image_25: the background fills the screen.
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetBrushFromTexture(BackgroundTexture.LoadSynchronous(), false);
	PlaceBox(Root, Background, 0.f, 0.f, ScreenWidth, ScreenHeight);

	// ShardCount: centred in its box, lifted by the margin and the render transform of the original.
	ShardCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShardCount"));
	ShardCountText->SetFont(MakeFont(ShardFontSize, ShardOutlineSize));
	ShardCountText->SetJustification(ETextJustify::Center);
	ShardCountText->SetShadowOffset(FVector2D::ZeroVector);
	ShardCountText->SetText(FText::AsNumber(0));
	FWidgetTransform Lift;
	Lift.Translation = FVector2D(0.f, ShardTranslationY);
	ShardCountText->SetRenderTransform(Lift);
	ShardCountText->SetMargin(FMargin(0.f, ShardMarginTop, 0.f, 0.f));
	PlaceBox(Root, ShardCountText, CountLeft, CountTop, CountRight - CountLeft, CountBottom - CountTop);

	// CanvasPanel_762: the map and everything over it.
	UCanvasPanel* MapPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapPanel"));
	const float MapWidth = MapRight - MapLeft;
	const float MapHeight = MapBottom - MapTop;
	PlaceBox(Root, MapPanel, MapLeft, MapTop, MapWidth, MapHeight);

	UImage* MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Map"));
	MapImage->SetBrushFromMaterial(MapMaterial.LoadSynchronous());
	PlaceFill(MapPanel, MapImage);

	UImage* PlayerMark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerMark"));
	PlayerMark->SetBrushFromTexture(PlayerMarkTexture.LoadSynchronous(), false);
	PlaceBox(MapPanel, PlayerMark, MapWidth * 0.5f + MarkOffsetX, MapHeight * 0.5f + MarkOffsetY, MarkSize, MarkSize);

	FlashImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Flash"));
	FlashImage->SetBrushFromTexture(VignetteTexture.LoadSynchronous(), false);
	FlashImage->SetBrushTintColor(FSlateColor(FlashColour));
	FlashImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
	PlaceFill(MapPanel, FlashImage);

	// Button_0: the objective band, wider than the map's panel, hanging off its bottom.
	UBorder* Band = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObjectiveBand"));
	Band->SetBrushColor(BandColour);
	Band->SetHorizontalAlignment(HAlign_Center);
	Band->SetVerticalAlignment(VAlign_Center);
	ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Objective"));
	ObjectiveText->SetFont(MakeFont(BandFontSize, 0));
	ObjectiveText->SetJustification(ETextJustify::Center);
	Band->AddChild(ObjectiveText);
	PlaceBox(MapPanel, Band, MapWidth * 0.5f + BandOffsetX, MapHeight + BandOffsetY, BandWidth, BandHeight);

	// Construct: an MM_Powers instance per power, whose `Percent` the gauges drive.
	PowerIcons.Reset(WasamiPowerCount);
	for (const TSoftObjectPtr<UMaterialInterface>& Material : PowerMaterials)
	{
		UMaterialInterface* Loaded = Material.LoadSynchronous();
		PowerIcons.Add(Loaded ? UMaterialInstanceDynamic::Create(Loaded, this) : nullptr);
	}

	// Skill1 / Skill2: the two sockets, filling CanvasPanel_3 / CanvasPanel_4 (the bounce scales them about their middle,
	// as it does those canvases). Update Powers gives them their icons.
	LeftSocket = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LeftPower"));
	PlaceBox(Root, LeftSocket, LeftSocketLeft, SocketTop, LeftSocketRight - LeftSocketLeft, LeftSocketBottom - SocketTop);
	RightSocket = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RightPower"));
	PlaceBox(Root, RightSocket, RightSocketLeft, SocketTop, RightSocketRight - RightSocketLeft, RightSocketBottom - SocketTop);

	// TextBlock_107: the "Z" of the map's resize, almost invisible.
	UTextBlock* ZoomKey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoomKey"));
	ZoomKey->SetFont(MakeFont(BandFontSize, 0));
	ZoomKey->SetText(FText::FromString(TEXT("Z")));
	ZoomKey->SetRenderOpacity(ZoomKeyOpacity);
	UCanvasPanelSlot* ZoomSlot = PlaceBox(Root, ZoomKey, ScreenWidth * 0.5f + ZoomKeyX, ScreenHeight * 0.5f + ZoomKeyY, 0.f, 0.f);
	ZoomSlot->SetAutoSize(true);
}

void UWasamiTabletWidget::SetShardCount(int32 Count)
{
	if (ShardCountText && Count != LastShardCount)
	{
		LastShardCount = Count;
		ShardCountText->SetText(FText::AsNumber(Count));
	}
}

void UWasamiTabletWidget::SetObjective(const FText& Objective)
{
	if (ObjectiveText)
	{
		ObjectiveText->SetText(FText::FromString(Objective.ToString().ToUpper()));
	}
}

void UWasamiTabletWidget::SetPowersVisible(bool bVisible)
{
	if (bPowersVisible == bVisible)
	{
		return;
	}
	bPowersVisible = bVisible;
	for (UImage* Each : {LeftSocket.Get(), RightSocket.Get()})
	{
		if (Each)
		{
			Each->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
	}
}

void UWasamiTabletWidget::ShowSocketPowers(EWasamiPower Left, EWasamiPower Right)
{
	for (bool bLeft : {true, false})
	{
		const int32 Power = static_cast<int32>(bLeft ? Left : Right);
		int32& Shown = ShownPower[bLeft ? 0 : 1];
		UImage* Image = Socket(bLeft);
		if (!Image || Shown == Power)
		{
			continue;
		}
		Shown = Power;
		// SetBrushFromMaterial(None) would draw a white box; a socket with no power shows nothing instead.
		UMaterialInstanceDynamic* Icon = PowerIcons.IsValidIndex(Power) ? PowerIcons[Power].Get() : nullptr;
		Image->SetBrushFromMaterial(Icon);
		Image->SetOpacity(Icon ? 1.f : 0.f);
	}
}

void UWasamiTabletWidget::SetPowerPercent(EWasamiPower Power, float Percent)
{
	const int32 Index = static_cast<int32>(Power);
	if (!PowerIcons.IsValidIndex(Index) || !PowerIcons[Index] || FMath::IsNearlyEqual(LastPercent[Index], Percent, 0.001f))
	{
		return;
	}
	LastPercent[Index] = Percent;
	PowerIcons[Index]->SetScalarParameterValue(TEXT("Percent"), Percent);
}

void UWasamiTabletWidget::BounceSocket(bool bLeft)
{
	BounceTime[bLeft ? 0 : 1] = 0.f;
}

void UWasamiTabletWidget::PlayCountShake()
{
	if (!ShardCountText)
	{
		return;
	}
	// The state a restoring section returns to is taken when the animation first plays, not when it starts again.
	if (CountShakeTime < 0.f)
	{
		CountRestTransform = ShardCountText->GetRenderTransform();
	}
	// PlayAnimation puts the first frame on at once.
	CountShakeTime = 0.f;
	ApplyCountShake();
}

void UWasamiTabletWidget::ApplyCountShake()
{
	FWidgetTransform Transform = ShardCountText->GetRenderTransform();
	Transform.Translation = EvaluateCountShakeTranslation(CountShakeTime);
	const float Scale = EvaluateCountShakeScale(CountShakeTime);
	Transform.Scale = FVector2D(Scale, Scale);
	ShardCountText->SetRenderTransform(Transform);
	if (FlashImage)
	{
		FLinearColor Colour = FlashImage->GetColorAndOpacity();
		Colour.A = EvaluateCountShakeFlash(CountShakeTime);
		FlashImage->SetColorAndOpacity(Colour);
	}
}

void UWasamiTabletWidget::TickAnimations(float DeltaSeconds)
{
	if (CountShakeTime >= 0.f && ShardCountText)
	{
		// The last frame played is the end's tick (12000), inside the transform section; then it restores.
		CountShakeTime = FMath::Min(CountShakeTime + DeltaSeconds * CountShakeSpeed, CountShakeLength);
		ApplyCountShake();
		if (CountShakeTime >= CountShakeLength)
		{
			ShardCountText->SetRenderTransform(CountRestTransform);
			CountShakeTime = -1.f;
		}
	}

	for (bool bLeft : {true, false})
	{
		float& Time = BounceTime[bLeft ? 0 : 1];
		UImage* Image = Socket(bLeft);
		if (Time < 0.f || !Image)
		{
			continue;
		}
		Time += DeltaSeconds;
		const float Scale = EvaluateSocketBounce(Time);
		Image->SetRenderScale(FVector2D(Scale, Scale));
		if (Time >= BounceLength)
		{
			Time = -1.f;
		}
	}
}

FVector2D UWasamiTabletWidget::EvaluateCountShakeTranslation(float Seconds)
{
	static const FRichCurve X = MakeCountShakeCurve(CountShakeXKeys);
	static const FRichCurve Y = MakeCountShakeCurve(CountShakeYKeys);
	const float Clamped = FMath::Clamp(Seconds, 0.f, CountShakeLength);
	return FVector2D(X.Eval(Clamped), Y.Eval(Clamped));
}

float UWasamiTabletWidget::EvaluateCountShakeScale(float Seconds)
{
	static const FRichCurve Curve = MakeCountShakeCurve(CountShakeScaleKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, CountShakeLength));
}

float UWasamiTabletWidget::EvaluateCountShakeFlash(float Seconds)
{
	static const FRichCurve Curve = MakeCountShakeCurve(CountShakeFlashKeys);
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, CountShakeLength));
}

float UWasamiTabletWidget::EvaluateSocketBounce(float Seconds)
{
	static const FRichCurve Curve = MakeBounceCurve();
	return Curve.Eval(FMath::Clamp(Seconds, 0.f, BounceLength));
}
