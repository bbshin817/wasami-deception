#include "WasamiTabletWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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
	static ConstructorHelpers::FObjectFinder<UTexture2D> Background(TEXT("/Game/DD/UI/Tablet/tablet_screen_bg"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> PlayerMark(TEXT("/Game/DD/UI/Tablet/tablet_map_player"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Vignette(TEXT("/Game/DD/UI/Menu/Streaks/T_Vignette"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Map(TEXT("/Game/Pipeline/Materials/M_DD_MapScreen"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Teleport(TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_Inst_Teleport"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Boost(TEXT("/Game/DD/Materials/MasterMaterials/MM_Powers_SpeedBoost"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font"));
	BackgroundTexture = Background.Object;
	PlayerMarkTexture = PlayerMark.Object;
	VignetteTexture = Vignette.Object;
	MapMaterial = Map.Object;
	LeftPowerMaterial = Teleport.Object;
	RightPowerMaterial = Boost.Object;
	ScreenFont = Font.Object;
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
	auto MakeFont = [this](float Size, int32 Outline)
	{
		FSlateFontInfo Info;
		Info.FontObject = ScreenFont;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		Info.OutlineSettings.OutlineSize = Outline;
		return Info;
	};

	// Image_25: the background fills the screen.
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetBrushFromTexture(BackgroundTexture, false);
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
	MapImage->SetBrushFromMaterial(MapMaterial);
	PlaceFill(MapPanel, MapImage);

	UImage* PlayerMark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PlayerMark"));
	PlayerMark->SetBrushFromTexture(PlayerMarkTexture, false);
	PlaceBox(MapPanel, PlayerMark, MapWidth * 0.5f + MarkOffsetX, MapHeight * 0.5f + MarkOffsetY, MarkSize, MarkSize);

	FlashImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Flash"));
	FlashImage->SetBrushFromTexture(VignetteTexture, false);
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

	// Skill1 / Skill2: the two power sockets, each an MM_Powers instance we can drive with `Percent`.
	LeftPower = UMaterialInstanceDynamic::Create(LeftPowerMaterial, this);
	UImage* LeftSocket = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LeftPower"));
	LeftSocket->SetBrushFromMaterial(LeftPower);
	PlaceBox(Root, LeftSocket, LeftSocketLeft, SocketTop, LeftSocketRight - LeftSocketLeft, LeftSocketBottom - SocketTop);

	RightPower = UMaterialInstanceDynamic::Create(RightPowerMaterial, this);
	UImage* RightSocket = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RightPower"));
	RightSocket->SetBrushFromMaterial(RightPower);
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

void UWasamiTabletWidget::SetPowerCharge(bool bLeftSocket, float Percent)
{
	const int32 Index = bLeftSocket ? 0 : 1;
	if (FMath::IsNearlyEqual(LastCharge[Index], Percent, 0.001f))
	{
		return;
	}
	LastCharge[Index] = Percent;
	if (UMaterialInstanceDynamic* Socket = bLeftSocket ? LeftPower : RightPower)
	{
		Socket->SetScalarParameterValue(TEXT("Percent"), Percent);
	}
}
