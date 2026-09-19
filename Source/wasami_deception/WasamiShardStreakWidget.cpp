#include "WasamiShardStreakWidget.h"

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
#include "WasamiGameInstance.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// Anim (pak_reference_2's UMG_ShardStreak; the same in pak_reference), its keys as exported with UE's auto tangents.
	// StreakImage: RenderTransform's scale (both axes alike) and ColorAndOpacity's alpha.
	const FAnimKey CardScaleKeys[] = {{0., 2.f, 0., 0.}, {9000., 1.f, -5.999999848427251e-05, -5.999999848427251e-05},
		{15000., 1.1f, 0., 0.}, {30000., 1.f, -2.564103169788723e-06, -2.564103169788723e-06},
		{54000., 1.f, 3.3333342344121775e-06, 3.3333342344121775e-06}, {90000., 1.2f, 0., 0.}};
	const FAnimKey CardAlphaKeys[] = {{0., 0.f, 0., 0.}, {9000., 1.f, 1.851851811807137e-05, 1.851851811807137e-05},
		{54000., 1.f, -1.2345678442216013e-05, -1.2345678442216013e-05}, {90000., 0.f, 0., 0.}};
	// Image_161: its scale held at 1 (two linear keys), then out to 2 from 0.9 s with the author's leave tangent; its alpha.
	const FAnimKey VignetteScaleKeys[] = {{0., 1.f, 0., 0., RCIM_Linear}, {20999., 1.f, 0., 0., RCIM_Linear},
		{54000., 1.f, 0., 1.4492754417005926e-05}, {90000., 2.f, 0., 0.}};
	const FAnimKey VignetteAlphaKeys[] = {{0., 0.f, 0., 0.}, {9000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05},
		{15000., 0.5f, -1.6666666851961054e-05, -1.6666666851961054e-05},
		{54000., 0.25f, -6.666666649834951e-06, -6.666666649834951e-06}, {90000., 0.f, 0., 0.}};
	// extralife: RenderOpacity and RenderTransform's scale, in a section from 12000 ticks (0.2 s); before it the panel
	// keeps its own RenderOpacity 0 and scale 1.1.
	constexpr float LifeSectionStart = 12000.f / 60000.f;
	constexpr float LifeRestOpacity = 0.f;
	constexpr float LifeRestScale = 1.1f;
	const FAnimKey LifeOpacityKeys[] = {{12000., 0.f, 0., 0.}, {21000., 1.f, 4.166666622040793e-05, 4.166666622040793e-05},
		{54000., 1.f, -1.7543859939905815e-05, -1.7543859939905815e-05}, {90000., 0.f, 0., 0.}};
	const FAnimKey LifeScaleKeys[] = {{12000., 1.25f, 0., 0.}, {21000., 0.95f, -1.0416666555101983e-05, -1.0416666555101983e-05},
		{54000., 1.f, 2.941177172033349e-06, 2.941177172033349e-06}, {90000., 1.1f, 0., 0.}};

	// Image_161's brush tint, and its ColorAndOpacity's alpha at rest (Anim sets it from 0).
	const FLinearColor StreakVignetteTint(0.22481299936771393f, 0.f, 0.38020798563957214f, 1.f);
	constexpr float VignetteRestAlpha = 0.25f;
	constexpr float VignetteRestScale = 2.f;
	// TextBlock_94's outline colour (its outline size is UMG's default 0, so none is drawn).
	const FLinearColor TextOutline(0.6866850256919861f, 0.f, 0.9386860132217407f, 1.f);

	const TCHAR* const StreakTexturePaths[] = {
		TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_20"), TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_50"),
		TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_100"), TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_150"),
		TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_200"), TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_250"),
		TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_350"), TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_500"),
		TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_700"), TEXT("/Game/DD/UI/Menu/Streaks/shard_streak_1000")};

	UCanvasPanelSlot* StreakPlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateBrush StreakBrush(UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	void SetScale(UWidget* Widget, float Scale)
	{
		FWidgetTransform Transform = Widget->GetRenderTransform();
		Transform.Scale = FVector2D(Scale, Scale);
		Widget->SetRenderTransform(Transform);
	}
}

UWasamiShardStreakWidget::UWasamiShardStreakWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	for (const TCHAR* Path : StreakTexturePaths)
	{
		StreakTextures.Add(TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(Path)));
	}
	VignetteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Streaks/T_Vignette")));
	LifeTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Main/life_icon_02")));
	TextFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

UWasamiShardStreakWidget* UWasamiShardStreakWidget::Show(const UObject* WorldContextObject, uint8 InStreak)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiShardStreakWidget* Widget = CreateWidget<UWasamiShardStreakWidget>(Controller, StaticClass());
	if (Widget)
	{
		Widget->Streak = InStreak;
		Widget->AddToPlayerScreen(ZOrder);
	}
	return Widget;
}

bool UWasamiShardStreakWidget::GivesExtraLife(uint8 InStreak)
{
	return InStreak == 5 || InStreak == 8;
}

TSharedRef<SWidget> UWasamiShardStreakWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		// Nothing here takes the mouse: the widget lies over the game and, for its 2 s, over any screen below Z 2.
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		// StreakImage: the card, 612 × 227 at its size in the middle, clear until Anim.
		StreakImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("StreakImage"));
		StreakImage->SetBrush(StreakBrush(StreakTextures.Num() > 0 ? StreakTextures[0].LoadSynchronous() : nullptr,
			FVector2D(612.f, 227.f)));
		StreakImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
		StreakPlace(Root, StreakImage, FAnchors(0.5f, 0.5f), FMargin(0.f, 0.f, 0.f, 40.f), FVector2D(0.5f, 0.5f), true);

		// Image_161: T_Vignette tinted purple over the whole screen, over the card.
		Vignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_161"));
		Vignette->SetBrush(StreakBrush(VignetteTexture.LoadSynchronous(), FVector2D(1920.f, 1080.f), StreakVignetteTint));
		Vignette->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, VignetteRestAlpha));
		SetScale(Vignette, VignetteRestScale);
		StreakPlace(Root, Vignette, FAnchors(0.f, 0.f, 1.f, 1.f),
			FMargin(0.9609375f, 0.54052734375f, 0.9609375f, 0.54052734375f), FVector2D(0.5f, 0.5f), true);

		// extralife: 280 × 129 hanging 115 px below the middle, the skull on its left and EXTRA LIFE ! beside it.
		ExtraLife = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("extralife"));
		ExtraLife->SetRenderOpacity(LifeRestOpacity);
		SetScale(ExtraLife, LifeRestScale);
		StreakPlace(Root, ExtraLife, FAnchors(0.5f, 0.5f), FMargin(0.f, 115.45947265625f, 280.18017578125f, 129.09909057617188f),
			FVector2D(0.5f, 0.f), false);

		UImage* Skull = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_88"));
		Skull->SetBrush(StreakBrush(LifeTexture.LoadSynchronous(), FVector2D(90.f, 90.f)));
		StreakPlace(ExtraLife, Skull, FAnchors(0.f, 0.5f), FMargin(-8.f, -44.54954528808594f, 0.f, 0.f), FVector2D::ZeroVector, true);

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_94"));
		Text->SetText(FText::FromString(TEXT("EXTRA LIFE !")));
		FSlateFontInfo Font;
		Font.FontObject = TextFont.LoadSynchronous();
		Font.TypefaceFontName = TEXT("Default");
		Font.Size = 24.f;
		Font.OutlineSettings.OutlineColor = TextOutline;
		Font.OutlineSettings.bSeparateFillAlpha = true;
		Font.OutlineSettings.bApplyOutlineToDropShadows = true;
		Text->SetFont(Font);
		StreakPlace(ExtraLife, Text, FAnchors(0.f, 0.f, 1.f, 0.5f), FMargin(80.f, 44.f, 100.18017578125f, -9.450454711914062f),
			FVector2D::ZeroVector, false);
	}
	return Super::RebuildWidget();
}

void UWasamiShardStreakWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin(Streak);
	// Increment Lives (BP_DD_Functions: the game instance's Lives + 1, clamped to 0..6) where EXTRA LIFE ! shows.
	if (bExtraLife)
	{
		if (UWasamiGameInstance* Instance = GetGameInstance<UWasamiGameInstance>())
		{
			Instance->IncrementLives();
		}
	}
}

void UWasamiShardStreakWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiShardStreakWidget::Begin(uint8 InStreak)
{
	// Construct: SetBrushFromTexture(the card by Streak, bMatchSize), PlayAnimation(Anim), extralife Visible for 5 and 8
	// (then Increment Lives) or Hidden, and Delay 2 → RemoveFromParent.
	Streak = InStreak;
	StreakTexture = InStreak >= 1 && InStreak <= StreakTextures.Num() ? StreakTextures[InStreak - 1].LoadSynchronous() : nullptr;
	if (StreakImage)
	{
		StreakImage->SetBrushFromTexture(StreakTexture, true);
	}
	bExtraLife = GivesExtraLife(InStreak);
	if (ExtraLife)
	{
		ExtraLife->SetVisibility(bExtraLife ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	Elapsed = 0.f;
	bFinished = false;
	ApplyAnimation();
}

void UWasamiShardStreakWidget::Advance(float DeltaSeconds)
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

void UWasamiShardStreakWidget::ApplyAnimation()
{
	if (StreakImage)
	{
		SetScale(StreakImage, EvaluateCardScale(Elapsed));
		StreakImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, EvaluateCardAlpha(Elapsed)));
	}
	if (Vignette)
	{
		SetScale(Vignette, EvaluateVignetteScale(Elapsed));
		Vignette->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, EvaluateVignetteAlpha(Elapsed)));
	}
	if (ExtraLife)
	{
		ExtraLife->SetRenderOpacity(EvaluateLifeOpacity(Elapsed));
		SetScale(ExtraLife, EvaluateLifeScale(Elapsed));
	}
}

float UWasamiShardStreakWidget::EvaluateCardScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CardScaleKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiShardStreakWidget::EvaluateCardAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(CardAlphaKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiShardStreakWidget::EvaluateVignetteScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(VignetteScaleKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiShardStreakWidget::EvaluateVignetteAlpha(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(VignetteAlphaKeys);
	return Eval(Curve, Seconds, AnimLength);
}

float UWasamiShardStreakWidget::EvaluateLifeOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(LifeOpacityKeys);
	return Seconds < LifeSectionStart ? LifeRestOpacity : Eval(Curve, Seconds, AnimLength);
}

float UWasamiShardStreakWidget::EvaluateLifeScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(LifeScaleKeys);
	return Seconds < LifeSectionStart ? LifeRestScale : Eval(Curve, Seconds, AnimLength);
}
