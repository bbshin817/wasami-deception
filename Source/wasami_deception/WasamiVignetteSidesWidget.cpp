#include "WasamiVignetteSidesWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Anim (pak_reference_2's UMG_VignetteSides), its keys as exported with UE's auto tangents.
	// TextBlock_47: RenderTransform's scale (both axes alike), slamming in from 4, and RenderOpacity.
	const FAnimKey TextScaleKeys[] = {{0., 4.f, 0., 0.}, {6000., 1.f, -0.0003222222439944744, -0.0003222222439944744},
		{9000., 1.1f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey TextOpacityKeys[] = {{0., 0.f, 0., 0.}, {6000., 1.f, 2.380952355451882e-05, 2.380952355451882e-05},
		{42000., 1.f, -1.190476177725941e-05, -1.190476177725941e-05}, {90000., 0.f, 0., 0.}};
	// Image_161: its scale in a straight line from 1.4 to 1.25 at 0.35 s (the author's arrive tangent), held to 0.9 s,
	// then out to 2; its ColorAndOpacity's alpha (the colour's other channels have no keys and keep the image's own).
	const FAnimKey VignetteScaleKeys[] = {{0., 1.4f, 0., -7.142855793063063e-06, RCIM_Linear},
		{21000., 1.25f, -7.142855793063063e-06, 0.}, {54000., 1.25f, 0., 0.}, {90000., 2.f, 0., 0.}};
	const FAnimKey VignetteAlphaKeys[] = {{0., 0.f, 0., 0.}, {9000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05},
		{15000., 0.5f, -1.6666666851961054e-05, -1.6666666851961054e-05},
		{54000., 0.25f, -6.666666649834951e-06, -6.666666649834951e-06}, {90000., 0.f, 0., 0.}};
	// CanvasPanel_0: RenderTransform's angle (degrees) and scale, in a section that ends at 28000 ticks (0.47 s).
	const FAnimKey CanvasAngleKeys[] = {{0., 0.f, 0., 0.}, {6000., 0.5f, 6.25000029685907e-05, 6.25000029685907e-05},
		{8000., 0.5f, -0.0008333333535119891, -0.0008333333535119891},
		{9000., -2.f, -7.142857066355646e-05, -7.142857066355646e-05}, {15000., 0.f, 0., 0.}};
	const FAnimKey CanvasScaleKeys[] = {{0., 1.f, 0., 0.}, {6000., 1.f, 6.249993930396158e-06, 6.249993930396158e-06},
		{8000., 1.05f, 0., 0.}, {28000., 1.f, 0., 0.}};
	constexpr float CanvasSectionEnd = 28000.f / 60000.f;

	// CanvasPanel_0's own RenderTransform (a slight tilt) outside its section.
	constexpr float CanvasRestAngle = -0.15806865692138672f;
	constexpr float CanvasRestScale = 1.f;
	// Image_161's ColorAndOpacity and scale at rest (Anim sets the alpha from 0 and the scale from 1.4).
	const FLinearColor VignetteRestColor(1.f, 0.23077000677585602f, 0.f, 0.4176790118217468f);
	constexpr float VignetteRestScale = 2.f;
	// TextBlock_47: white at 0.8 with a 1 px black outline at 0.638, unseen at rest (RenderOpacity 0).
	const FLinearColor TextColor(1.f, 1.f, 1.f, 0.800000011920929f);
	const FLinearColor TextOutline(0.f, 0.f, 0.f, 0.6380000114440918f);
	constexpr float TextRestOpacity = 0.f;

	UCanvasPanelSlot* SidesPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
		const FVector2D& Alignment, bool bAutoSize)
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

	void SetScale(UWidget* Widget, float Scale)
	{
		FWidgetTransform Transform = Widget->GetRenderTransform();
		Transform.Scale = FVector2D(Scale, Scale);
		Widget->SetRenderTransform(Transform);
	}
}

const FLinearColor UWasamiVignetteSidesWidget::StunnedColor(1.f, 0.4653930068016052f, 0.f, 1.f);
const FLinearColor UWasamiVignetteSidesWidget::RevealedColor(1.f, 0.f, 0.016666000708937645f, 1.f);

FText UWasamiVignetteSidesWidget::StunnedText()
{
	return FText::FromString(TEXT("ENEMIES STUNNED"));
}

FText UWasamiVignetteSidesWidget::RevealedText()
{
	return FText::FromString(TEXT("ENEMIES REVEALED"));
}

UWasamiVignetteSidesWidget::UWasamiVignetteSidesWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_VignetteNew")));
	TextFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

UWasamiVignetteSidesWidget* UWasamiVignetteSidesWidget::Show(const UObject* WorldContextObject, FLinearColor InColor, bool bInText,
	FText InTextToDisplay)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiVignetteSidesWidget* Widget = CreateWidget<UWasamiVignetteSidesWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->Color = InColor;
		Widget->bText = bInText;
		Widget->TextToDisplay = InTextToDisplay;
		Widget->AddToPlayerScreen(ZOrder);
	}
	return Widget;
}

void UWasamiVignetteSidesWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	const UWasamiVignetteSidesWidget* Defaults = GetDefault<UWasamiVignetteSidesWidget>();
	Out.Add(Defaults->VignetteTexture.LoadSynchronous());
	Out.Add(Defaults->TextFont.LoadSynchronous());
}

TSharedRef<SWidget> UWasamiVignetteSidesWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		// Nothing here takes the mouse: the widget lies over the game for its 2 s.
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		FWidgetTransform CanvasTransform;
		CanvasTransform.Angle = CanvasRestAngle;
		Canvas->SetRenderTransform(CanvasTransform);
		WidgetTree->RootWidget = Canvas;

		// Image_161: T_VignetteNew over the whole screen at twice its size (its middle is clear), tinted orange.
		Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_161"));
		FSlateBrush Brush;
		Brush.SetResourceObject(VignetteTexture.LoadSynchronous());
		Brush.ImageSize = FVector2D(1024.f, 1024.f);
		Vignette->SetBrush(Brush);
		Vignette->SetColorAndOpacity(VignetteRestColor);
		SetScale(Vignette, VignetteRestScale);
		SidesPlace(Canvas, Vignette, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.9609375f, 0.54052734375f, 0.9609375f, 0.54052734375f),
			FVector2D(0.5f, 0.5f), true);

		// TextBlock_47: a 151 × 40 box 233 px above the bottom's middle, the words centred on it and spilling out both
		// ways, over the vignette.
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_47"));
		TextBlock->SetText(FText::FromString(TEXT("ENEMIES STUNNED FOR 30 SECONDS")));
		TextBlock->SetColorAndOpacity(FSlateColor(TextColor));
		FSlateFontInfo Font;
		Font.FontObject = TextFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Default");
		Font.Size = 35.f;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = TextOutline;
		Font.OutlineSettings.bApplyOutlineToDropShadows = true;
		TextBlock->SetFont(Font);
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetRenderOpacity(TextRestOpacity);
		SidesPlace(Canvas, TextBlock, FAnchors(0.5f, 1.f), FMargin(-72.9609375f, -233.0810546875f, 151.f, 40.f), FVector2D::ZeroVector,
			false);
	}
	return Super::RebuildWidget();
}

void UWasamiVignetteSidesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiVignetteSidesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiVignetteSidesWidget::Begin()
{
	// Construct: Text? → TextBlock_47's SetText(TextToDisplay), or its SetVisibility(Hidden); then SetColorAndOpacity(Color)
	// on the widget itself, PlayAnimation(Anim) and Delay 2 → RemoveFromParent.
	if (TextBlock)
	{
		if (bText)
		{
			TextBlock->SetText(TextToDisplay);
		}
		else
		{
			TextBlock->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	SetColorAndOpacity(Color);
	Elapsed = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiVignetteSidesWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	ApplyAnimation();
	if (Elapsed >= RemoveDelay)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiVignetteSidesWidget::ApplyAnimation()
{
	if (Canvas)
	{
		FWidgetTransform Transform = Canvas->GetRenderTransform();
		Transform.Angle = EvaluateCanvasAngle(Elapsed);
		Transform.Scale = FVector2D(EvaluateCanvasScale(Elapsed));
		Canvas->SetRenderTransform(Transform);
	}
	if (Vignette)
	{
		SetScale(Vignette, EvaluateVignetteScale(Elapsed));
		FLinearColor Tint = VignetteRestColor;
		Tint.A = EvaluateVignetteAlpha(Elapsed);
		Vignette->SetColorAndOpacity(Tint);
	}
	if (TextBlock)
	{
		SetScale(TextBlock, EvaluateTextScale(Elapsed));
		TextBlock->SetRenderOpacity(EvaluateTextOpacity(Elapsed));
	}
}

float UWasamiVignetteSidesWidget::EvaluateTextScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(TextScaleKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiVignetteSidesWidget::EvaluateTextOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(TextOpacityKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiVignetteSidesWidget::EvaluateVignetteScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(VignetteScaleKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiVignetteSidesWidget::EvaluateVignetteAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(VignetteAlphaKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiVignetteSidesWidget::EvaluateCanvasAngle(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CanvasAngleKeys);
	return Seconds > CanvasSectionEnd ? CanvasRestAngle : Eval(Curve, Seconds, AnimLength);
}

float UWasamiVignetteSidesWidget::EvaluateCanvasScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CanvasScaleKeys);
	return Seconds > CanvasSectionEnd ? CanvasRestScale : Eval(Curve, Seconds, AnimLength);
}
