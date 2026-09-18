#include "WasamiBlackFadeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Image_29's RenderOpacity (pak_reference_2's UMG_BlackFade_2), over 300000 ticks; the black end has the author's
	// tangent.
	const FAnimKey BlackFadeInKeys[] = {{0., 0.f, 0., 0.}, {300000., 1.f, 2.365180080232676e-06, 2.3651853098272113e-06}};
	const FAnimKey BlackFadeOutKeys[] = {{0., 1.f, 2.365180080232676e-06, 2.3651853098272113e-06}, {300000., 0.f, 0., 0.}};
}

UWasamiBlackFadeWidget* UWasamiBlackFadeWidget::Show(const UObject* WorldContextObject, bool bInFadeIn, float InSpeed, int32 ZOrder)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiBlackFadeWidget* Fade = CreateWidget<UWasamiBlackFadeWidget>(Controller, StaticClass());
	if (Fade)
	{
		Fade->bFadeIn = bInFadeIn;
		Fade->Speed = InSpeed;
		Fade->AddToViewport(ZOrder);
	}
	return Fade;
}

TSharedRef<SWidget> UWasamiBlackFadeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		// Image_29: black over the whole screen, never in the way of the mouse, transparent until the animation plays.
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_29"));
		Image->SetColorAndOpacity(FLinearColor::Black);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image->SetRenderOpacity(0.f);
		UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Image);
		FAnchorData Layout;
		Layout.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		Layout.Offsets = FMargin(0.f);
		ImageSlot->SetLayout(Layout);
	}
	return Super::RebuildWidget();
}

void UWasamiBlackFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Construct: PlayAnimation(Fade in? ? FadeIn : FadeOut, 0, 1, Forward, Speed); its first frame shows at once.
	Time = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiBlackFadeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiBlackFadeWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Time = FMath::Min(Time + DeltaSeconds * Speed, AnimationLength);
	ApplyAnimation();
	if (Time >= AnimationLength)
	{
		// The animation's finished event: Animation Finished, then off the screen unless held.
		bFinished = true;
		OnFadeFinished.Broadcast();
		if (!bHold)
		{
			RemoveFromParent();
		}
	}
}

void UWasamiBlackFadeWidget::ApplyAnimation()
{
	Opacity = bFadeIn ? EvaluateFadeIn(Time) : EvaluateFadeOut(Time);
	if (Image)
	{
		Image->SetRenderOpacity(Opacity);
	}
}

float UWasamiBlackFadeWidget::EvaluateFadeIn(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(BlackFadeInKeys);
	return Eval(Curve, Seconds, AnimationLength);
}

float UWasamiBlackFadeWidget::EvaluateFadeOut(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(BlackFadeOutKeys);
	return Eval(Curve, Seconds, AnimationLength);
}
