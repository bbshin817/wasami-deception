#include "WasamiSwitchboxWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Construct and Set Progress: the ring's parameter.
	const FName PercentageParameter(TEXT("Percentage"));

	// UMG_07_Boss_Switchbox (pak_reference_2): the tree's sizes, tints and starting transforms.
	constexpr float KeyFontSize = 64.f;
	constexpr int32 KeyOutlineSize = 4;
	constexpr float OuterRingSize = 270.f;
	constexpr float InnerRingSize = 235.f;
	constexpr float CircleSize = 256.f;
	constexpr float SparkImageSize = 512.f;
	const FVector2D SparkStartScale(1.25, 1.);
	const FVector2D Spark2StartScale(1.5, 1.5);

	// The animations' keys (ticks at 60000 a second, value, arrive and leave tangents per tick). Interact and Interact_1
	// differ only in the key's rotation (°); Completed plays at twice its rate.
	const FAnimKey InteractKeyAngleKeys[] = {{0., -3.0f, 0., 0.}, {6000., 1.0f, 0.00033333332976326346, 0.00033333332976326346},
		{15000., 0.0f, 0., 0.}};
	const FAnimKey Interact1KeyAngleKeys[] = {{0., 3.0f, 0., 0.}, {6000., -1.0f, 0.00033333332976326346, 0.00033333332976326346},
		{15000., 0.0f, 0., 0.}};
	const FAnimKey InteractKeyScaleKeys[] = {{0., 1.100000023841858f, 0., 0.}, {6000., 1.0f, 0., 0.}};
	const FAnimKey InteractRingsScaleKeys[] = {{0., 1.0499999523162842f, 0., 0.},
		{6000., 0.9800000190734863f, -9.99999883788405e-06, -9.99999883788405e-06}, {15000., 1.0f, 0., 0.}};
	const FAnimKey CompletedSparkOpacityKeys[] = {{0., 1.0f, 0., 0.}, {30000., 0.0f, 0., 0.}};
	const FAnimKey CompletedSparkScaleXKeys[] = {{0., 1.0f, 0., 0.}, {12000., 1.25f, 0., 0.}};
	// Image_33's Scale[1] is keyed 1 → 1.
	constexpr float CompletedSparkScaleY = 1.f;
	const FAnimKey CompletedSpark2ScaleKeys[] = {{0., 2.0f, 0., 0.}, {30001., 1.5f, 0., 0.}};
	const FAnimKey CompletedRingsOpacityKeys[] = {{0., 1.0f, 0., 0.}, {15000., 0.0f, 0., 0.}};
	const FAnimKey CompletedKeyOpacityKeys[] = {{0., 1.0f, 0., 0.}, {12000., 0.0f, 0., 0.}};
	// Image_55's opacity has Image_33's keys.

	/** A slot of the original's: anchored at the canvas's middle and aligned by the child's middle. */
	UCanvasPanelSlot* PlaceCentred(UCanvasPanel* Panel, UWidget* Child, float Size, bool bAutoSize)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = FAnchors(0.5f, 0.5f);
		Layout.Alignment = FVector2D(0.5, 0.5);
		// A size-less slot keeps UMG's default offsets (0, 0, 100, 30), which an autosized child does not use.
		Layout.Offsets = Size > 0.f ? FMargin(0.f, 0.f, Size, Size) : FMargin(0.f, 0.f, 100.f, 30.f);
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
		return Slot;
	}

	FSlateBrush MaterialBrush(UMaterialInterface* Material, const FLinearColor& Tint, float ImageSize = 32.f)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Material);
		Brush.ImageSize = FVector2D(ImageSize, ImageSize);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}
}

UWasamiSwitchboxWidget::UWasamiSwitchboxWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RingMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/04_Sewer/M_04_UI_Radial_Red")));
	SparkMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/07_FunPlace/M_07_Spark")));
	Spark2Material = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/07_FunPlace/M_07_Spark2")));
	KeyFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

float UWasamiSwitchboxWidget::EvaluateInteractKeyAngle(bool bFirst, float Seconds)
{
	static const FRichCurve First = MakeCurve(InteractKeyAngleKeys);
	static const FRichCurve Second = MakeCurve(Interact1KeyAngleKeys);
	return Eval(bFirst ? First : Second, Seconds, InteractLength);
}

float UWasamiSwitchboxWidget::EvaluateInteractKeyScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(InteractKeyScaleKeys);
	return Eval(Curve, Seconds, InteractLength);
}

float UWasamiSwitchboxWidget::EvaluateInteractRingsScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(InteractRingsScaleKeys);
	return Eval(Curve, Seconds, InteractLength);
}

float UWasamiSwitchboxWidget::EvaluateCompletedSparkOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CompletedSparkOpacityKeys);
	return Eval(Curve, Seconds, CompletedLength);
}

float UWasamiSwitchboxWidget::EvaluateCompletedSparkScaleX(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CompletedSparkScaleXKeys);
	return Eval(Curve, Seconds, CompletedLength);
}

float UWasamiSwitchboxWidget::EvaluateCompletedSpark2Scale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CompletedSpark2ScaleKeys);
	return Eval(Curve, Seconds, CompletedLength);
}

float UWasamiSwitchboxWidget::EvaluateCompletedRingsOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CompletedRingsOpacityKeys);
	return Eval(Curve, Seconds, CompletedLength);
}

float UWasamiSwitchboxWidget::EvaluateCompletedKeyOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CompletedKeyOpacityKeys);
	return Eval(Curve, Seconds, CompletedLength);
}

bool UWasamiSwitchboxWidget::Initialize()
{
	const bool bResult = Super::Initialize();
	// The tree exists from the widget's creation: the door break sets Progress Speed and binds Finished before Construct.
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return bResult;
	}
	Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
	WidgetTree->RootWidget = Root;
	UMaterialInterface* Ring = RingMaterial.LoadSynchronous();

	// The slots in the original's order, which is the order they are drawn in: the key under the rings, the sparks over
	// them.
	Key = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_62"));
	Key->SetText(FText::FromString(TEXT("F")));
	FSlateFontInfo Font;
	Font.FontObject = KeyFont.LoadSynchronous();
	Font.TypefaceFontName = TEXT("Default");
	Font.Size = KeyFontSize;
	Font.OutlineSettings.OutlineSize = KeyOutlineSize;
	Key->SetFont(Font);
	PlaceCentred(Root, Key, 0.f, true);

	// CanvasPanel_107 sizes itself to its largest ring (270). Its two black rings are the red ring's material tinted
	// black, which keeps its own Percentage.
	Rings = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_107"));
	PlaceCentred(Root, Rings, 0.f, true);
	UImage* OuterRing = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_122"));
	OuterRing->SetBrush(MaterialBrush(Ring, FLinearColor::Black));
	PlaceCentred(Rings, OuterRing, OuterRingSize, false);
	UImage* InnerRing = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_123"));
	InnerRing->SetBrush(MaterialBrush(Ring, FLinearColor::Black));
	PlaceCentred(Rings, InnerRing, InnerRingSize, false);
	Circle = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("circle"));
	Circle->SetBrush(MaterialBrush(Ring, FLinearColor::White));
	PlaceCentred(Rings, Circle, CircleSize, false);

	// Image_33 and Image_55: 512 × 512 at their brush size (autosized), red, unseen until Completed.
	Spark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_33"));
	Spark->SetBrush(MaterialBrush(SparkMaterial.LoadSynchronous(), FLinearColor::Red, SparkImageSize));
	Spark->SetRenderScale(SparkStartScale);
	Spark->SetRenderOpacity(0.f);
	PlaceCentred(Root, Spark, 0.f, true);
	Spark2 = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_55"));
	Spark2->SetBrush(MaterialBrush(Spark2Material.LoadSynchronous(), FLinearColor::Red, SparkImageSize));
	Spark2->SetRenderScale(Spark2StartScale);
	Spark2->SetRenderOpacity(0.f);
	PlaceCentred(Root, Spark2, 0.f, true);
	// The fourth animation, Fade (the whole widget 0 → 1 in 0.5 s), is never played.
	return bResult;
}

void UWasamiSwitchboxWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Construct, in a DoOnce: the ring's dynamic material, emptied (and a debug PrintString, left out).
	if (bConstructed)
	{
		return;
	}
	bConstructed = true;
	Material = Circle ? Circle->GetDynamicMaterial() : nullptr;
	if (Material)
	{
		Material->SetScalarParameterValue(PercentageParameter, 0.f);
	}
}

void UWasamiSwitchboxWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiSwitchboxWidget::InteractEvent()
{
	// RandomBool selects Interact (true) or Interact_1 (false).
	Play(FMath::RandBool() ? EAnimation::Interact : EAnimation::Interact1, 1.f);
	Progress = FMath::Clamp(Progress + ProgressSpeed, 0.f, MaxProgress);
	SetProgress(Progress / MaxProgress);
	if (Progress > FinishAbove && !bFinished)
	{
		// A DoOnce: Finished, then Completed.
		bFinished = true;
		Finished.Broadcast();
		Play(EAnimation::Completed, CompletedRate);
	}
}

void UWasamiSwitchboxWidget::SetProgress(float Value)
{
	if (Material)
	{
		Material->SetScalarParameterValue(PercentageParameter, Value);
	}
}

void UWasamiSwitchboxWidget::Play(EAnimation Animation, float Rate)
{
	FPlayer* Player = Players.FindByPredicate([Animation](const FPlayer& Each) { return Each.Animation == Animation; });
	if (!Player)
	{
		Player = &Players.Add_GetRef({Animation, 0.f, Rate});
	}
	Player->Time = 0.f;
	Player->Rate = Rate;
	Apply(*Player);
}

void UWasamiSwitchboxWidget::Advance(float DeltaSeconds)
{
	// The players tick in the order they started; one that reaches its end puts its last frame on and stops.
	for (int32 Index = 0; Index < Players.Num();)
	{
		FPlayer& Player = Players[Index];
		const float Length = Player.Animation == EAnimation::Completed ? CompletedLength : InteractLength;
		Player.Time = FMath::Min(Player.Time + DeltaSeconds * Player.Rate, Length);
		Apply(Player);
		if (Player.Time >= Length)
		{
			Players.RemoveAt(Index);
			continue;
		}
		++Index;
	}
}

void UWasamiSwitchboxWidget::Apply(const FPlayer& Player)
{
	if (!Key || !Rings || !Spark || !Spark2)
	{
		return;
	}
	if (Player.Animation == EAnimation::Completed)
	{
		Spark->SetRenderOpacity(EvaluateCompletedSparkOpacity(Player.Time));
		Spark->SetRenderScale(FVector2D(EvaluateCompletedSparkScaleX(Player.Time), CompletedSparkScaleY));
		Spark2->SetRenderOpacity(EvaluateCompletedSparkOpacity(Player.Time));
		Spark2->SetRenderScale(FVector2D(EvaluateCompletedSpark2Scale(Player.Time)));
		Rings->SetRenderOpacity(EvaluateCompletedRingsOpacity(Player.Time));
		Key->SetRenderOpacity(EvaluateCompletedKeyOpacity(Player.Time));
		return;
	}
	Key->SetRenderTransformAngle(EvaluateInteractKeyAngle(Player.Animation == EAnimation::Interact, Player.Time));
	Key->SetRenderScale(FVector2D(EvaluateInteractKeyScale(Player.Time)));
	Rings->SetRenderScale(FVector2D(EvaluateInteractRingsScale(Player.Time)));
}
