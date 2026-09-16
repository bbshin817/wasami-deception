#include "WasamiVanishWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Curves/RichCurve.h"
#include "Materials/MaterialInterface.h"
#include "WasamiAssets.h"

namespace
{
	// UMG_Vanish: Image_82 fills the canvas, tinted purple, scaled up about its middle.
	const FLinearColor VanishVignetteColour(0.27869701385498047f, 0.16055700182914734f, 0.5364580154418945f, 1.f);
	constexpr float VanishVignetteScale = 1.0499999523162842f;

	// The animation Vanish: one RenderOpacity track, cubic keys at ticks 0 / 6000 / 54000 / 60000 (60000 a second) with
	// the tangents saved per tick, here per second.
	constexpr float TicksPerSecond = 60000.f;
	FRichCurve MakeOpacityCurve()
	{
		struct FKey
		{
			double Ticks;
			float Value;
			double TangentPerTick;
		};
		const FKey Keys[] = {
			{0., 0.f, 0.},
			{6000., 1.f, 1.0416666782475659e-06},
			{54000., 1.f, -1.0101009593199706e-06},
			{60000., 0.f, 0.},
		};
		FRichCurve Curve;
		for (const FKey& Each : Keys)
		{
			FRichCurveKey& Key = Curve.GetKey(Curve.AddKey(static_cast<float>(Each.Ticks / TicksPerSecond), Each.Value));
			Key.InterpMode = RCIM_Cubic;
			Key.TangentMode = RCTM_User;
			Key.ArriveTangent = static_cast<float>(Each.TangentPerTick * TicksPerSecond);
			Key.LeaveTangent = Key.ArriveTangent;
		}
		return Curve;
	}
}

UWasamiVanishWidget::UWasamiVanishWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VignetteMaterial = TSoftObjectPtr<UMaterialInterface>(WasamiAssets::Path(TEXT("/Game/DD/Materials/Special/MM_WobblyVignette")));
}

void UWasamiVanishWidget::LoadAssets(TArray<TObjectPtr<UObject>>& Out)
{
	Out.Add(GetDefault<UWasamiVanishWidget>()->VignetteMaterial.LoadSynchronous());
}

float UWasamiVanishWidget::EvaluateOpacity(float AnimationSeconds)
{
	static const FRichCurve Curve = MakeOpacityCurve();
	return Curve.Eval(FMath::Clamp(AnimationSeconds, 0.f, AnimationLength));
}

TSharedRef<SWidget> UWasamiVanishWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_82"));
		Vignette->SetBrushFromMaterial(VignetteMaterial.LoadSynchronous());
		Vignette->SetColorAndOpacity(VanishVignetteColour);
		Vignette->SetRenderScale(FVector2D(VanishVignetteScale));
		UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Vignette);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
	}
	return Super::RebuildWidget();
}

void UWasamiVanishWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// PlayAnimation(Vanish, 0, 1 loop, forward, 1 / Speed): the first frame's values go on at once. A division by 0
	// gives 0 in a Blueprint, which holds the animation at its start.
	PlaybackSpeed = Speed != 0.f ? 1.f / Speed : 0.f;
	AnimationTime = 0.f;
	bAnimationPlaying = true;
	ApplyAnimation();
}

void UWasamiVanishWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bAnimationPlaying)
	{
		return;
	}
	// One loop: past the end, the end's values stay (the animation does not restore the image).
	AnimationTime += InDeltaTime * PlaybackSpeed;
	if (AnimationTime >= AnimationLength)
	{
		AnimationTime = AnimationLength;
		bAnimationPlaying = false;
	}
	ApplyAnimation();
}

void UWasamiVanishWidget::ApplyAnimation()
{
	if (Vignette)
	{
		Vignette->SetRenderOpacity(EvaluateOpacity(AnimationTime));
	}
}
