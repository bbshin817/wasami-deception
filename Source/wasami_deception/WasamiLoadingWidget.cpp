#include "WasamiLoadingWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// FadeIn (pak_reference_2's UMG_Loading): CanvasPanel_0's RenderOpacity, 0 → 1 over 0.5 s, both keys flat.
	const FAnimKey LoadingFadeInKeys[] = {{0., 0.f, 0., 0.}, {30000., 1.f, 0., 0.}};

	// Image_23's tint: the screen's dark red (no texture).
	const FLinearColor LoadingBackground(0.49479201436042786f, 0.f, 0.f, 1.f);
	constexpr float LoadingEmblemSize = 512.f;
}

UWasamiLoadingWidget::UWasamiLoadingWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LevelEmblems.SetNum(AsylumLevel + 1);
	LevelEmblems[AsylumLevel] = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/Wasami/UI/loader_wasami")));
}

UWasamiLoadingWidget* UWasamiLoadingWidget::Show(const UObject* WorldContextObject, uint8 InLevel)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiLoadingWidget* Widget = CreateWidget<UWasamiLoadingWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->Level = InLevel;
		Widget->AddToViewport(ZOrder);
	}
	return Widget;
}

TSharedRef<SWidget> UWasamiLoadingWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		// Image_23: the whole screen and a little over it (anchors 0..1, offsets −16, −12, −10.5, −10), dark red.
		UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_23"));
		FSlateBrush Fill;
		Fill.TintColor = FSlateColor(LoadingBackground);
		Background->SetBrush(Fill);
		UCanvasPanelSlot* FillSlot = Root->AddChildToCanvas(Background);
		FAnchorData FillLayout;
		FillLayout.Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		FillLayout.Offsets = FMargin(-16.f, -12.f, -10.5f, -10.f);
		FillLayout.Alignment = FVector2D::ZeroVector;
		FillSlot->SetLayout(FillLayout);

		// Logo: 512 × 512 at its size (bAutoSize), in the middle of the screen.
		Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Logo"));
		FSlateBrush Emblem;
		Emblem.ImageSize = FVector2D(LoadingEmblemSize, LoadingEmblemSize);
		Logo->SetBrush(Emblem);
		UCanvasPanelSlot* LogoSlot = Root->AddChildToCanvas(Logo);
		FAnchorData LogoLayout;
		LogoLayout.Anchors = FAnchors(0.5f, 0.5f);
		LogoLayout.Offsets = FMargin(0.f, 0.f, 0.f, 40.f);
		LogoLayout.Alignment = FVector2D(0.5f, 0.5f);
		LogoSlot->SetLayout(LogoLayout);
		LogoSlot->SetAutoSize(true);
	}
	return Super::RebuildWidget();
}

void UWasamiLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Construct: the game instance's Shards To Be Removed emptied, the emblem by Level, PlayAnimation(FadeIn), Delay 2.5
	// → PlayAnimation(FadeIn, reverse), Delay 1 → RemoveFromParent.
	if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
	{
		Instance->ForgetCollectedShards();
	}
	if (Logo)
	{
		UTexture2D* Texture = GetLevelEmblem(Level).LoadSynchronous();
		Logo->SetBrushFromTexture(Texture, false);
		// Without one the original draws its brush blank (white); here the red screen shows alone.
		Logo->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	Elapsed = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiLoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiLoadingWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	Elapsed += DeltaSeconds;
	ApplyAnimation();
	if (Elapsed >= FadeOutDelay + RemoveDelay)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

void UWasamiLoadingWidget::ApplyAnimation()
{
	if (Root)
	{
		Root->SetRenderOpacity(EvaluateOpacity(Elapsed));
	}
}

float UWasamiLoadingWidget::EvaluateOpacity(float Seconds)
{
	// Played in reverse from 0, FadeIn starts at its end.
	return Seconds < FadeOutDelay ? EvaluateFadeIn(Seconds) : EvaluateFadeIn(FadeInLength - (Seconds - FadeOutDelay));
}

float UWasamiLoadingWidget::EvaluateFadeIn(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(LoadingFadeInKeys);
	return Eval(Curve, Seconds, FadeInLength);
}
