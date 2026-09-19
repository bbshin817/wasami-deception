#include "WasamiMaximizePictureWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "WasamiAssets.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// FadeIn (UMG_MaximizePicture), its keys as exported with UE's auto tangents: CanvasPanel_0's RenderOpacity 0 → 1
	// over 0.5 s, and Button_0's RenderTransform scale (x and y alike) 0.5 → 1 over 0.25 s. The sections end at 30001
	// ticks and a widget animation keeps what they set.
	const FAnimKey PictureOpacityKeys[] = {{0., 0.f, 0., 0.}, {30000., 1.f, 0., 0.}};
	const FAnimKey PictureScaleKeys[] = {{0., 0.5f, 0., 0.}, {15000., 1.f, 0., 3.333333370392211e-05}};

	// Button_116: UE 4's default button tinted black at 0.5 (the texture's colour goes under the black).
	const FLinearColor PictureDim(0.f, 0.f, 0.f, 0.5f);
	// TextBlock_1's shadow.
	const FLinearColor PictureTextShadow(0.f, 0.f, 0.f, 0.734000027179718f);

	UCanvasPanelSlot* PicturePlace(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	FSlateFontInfo PictureFont(UFont* Font, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Font;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		return Info;
	}

	/** UE 4's default button brush (a box, 8/32 kept at each edge, 32 square) of Texture (none: plain) tinted Tint. */
	FSlateBrush PictureBrush(UTexture2D* Texture, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(32.f, 32.f);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(8.f / 32.f);
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	/** A button that only shows its Normal brush (it lets the pointer through): UE 4's paddings, not focusable. */
	void PictureBackdropButton(UButton* Button, const FSlateBrush& Normal)
	{
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(Normal).SetHovered(Normal).SetPressed(Normal).SetNormalPadding(FMargin(2.f))
			.SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		// IsFocusable, which UE 5 lets be set only before the Slate widget is made.
		if (FBoolProperty* Property = FindFProperty<FBoolProperty>(UButton::StaticClass(), TEXT("IsFocusable")))
		{
			Property->SetPropertyValue_InContainer(Button, false);
		}
		Button->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UWasamiMaximizePictureWidget::UWasamiMaximizePictureWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NormalFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	BoldFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
	WhiteTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Engine/EngineResources/WhiteSquareTexture")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
}

UWasamiMaximizePictureWidget* UWasamiMaximizePictureWidget::Show(const UObject* WorldContextObject, UTexture2D* InTexture,
	const FText& InText)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiMaximizePictureWidget* View = CreateWidget<UWasamiMaximizePictureWidget>(Controller, StaticClass());
	if (View)
	{
		View->Texture = InTexture;
		View->Text = InText;
		View->AddToViewport(ViewportZOrder);
	}
	return View;
}

TSharedRef<SWidget> UWasamiMaximizePictureWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;

		// Button_116: black at 0.5 over the whole screen, letting the pointer through.
		Dim = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_116"));
		PictureBackdropButton(Dim, PictureBrush(nullptr, PictureDim));
		PicturePlace(Root, Dim, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// BackgroundBlur_0: the screen blurred (6), and the pointer stopped here.
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
		Blur->SetBlurStrength(6.f);
		Blur->SetVisibility(ESlateVisibility::Visible);
		PicturePlace(Root, Blur, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(0.f));

		// ScaleBox_0 → Button_0 → Image_1: the picture 50 px in on a black ground (a button that only shows its Normal),
		// fitted into the screen 100 px in from each edge.
		ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ScaleBox_0"));
		ScaleBox->SetUserSpecifiedScale(0.933122992515564f);
		Ground = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_0"));
		PictureBackdropButton(Ground, PictureBrush(WhiteTexture.LoadSynchronous(), FLinearColor::Black));
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_1"));
		FSlateBrush PictureSize;
		PictureSize.ImageSize = FVector2D(1024.f, 1024.f);
		Image->SetBrush(PictureSize);
		if (UButtonSlot* PictureSlot = Cast<UButtonSlot>(Ground->AddChild(Image)))
		{
			PictureSlot->SetPadding(FMargin(50.f));
		}
		ScaleBox->AddChild(Ground);
		PicturePlace(Root, ScaleBox, FAnchors(0.f, 0.f, 1.f, 1.f), FMargin(100.f), FVector2D(0.5f, 0.5f));

		// TextBlock_1: the Text 27 px down from the top's middle (helvetica-normal 30, a black shadow at 0.73).
		TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_1"));
		TextBlock->SetFont(PictureFont(NormalFont.LoadSynchronous(), 30.f));
		TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
		TextBlock->SetShadowColorAndOpacity(PictureTextShadow);
		PicturePlace(Root, TextBlock, FAnchors(0.5f, 0.f), FMargin(0.f, 27.35800552368164f, 100.f, 30.f), FVector2D(0.5f, 0.f), true);

		// Back: BACK (helvetica-neue-bold 24) on a clear button 69 px up from the bottom's middle, dark grey until hovered.
		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Back"));
		FButtonStyle BackStyle = BackButton->GetStyle();
		BackStyle.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		BackButton->SetStyle(BackStyle);
		BackButton->SetColorAndOpacity(FLinearColor(UnhoveredGrey, UnhoveredGrey, UnhoveredGrey, 1.f));
		BackButton->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		UTextBlock* BackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
		BackText->SetText(FText::FromString(TEXT("BACK")));
		BackText->SetFont(PictureFont(BoldFont.LoadSynchronous(), 24.f));
		// ButtonSlot_0: UE's defaults (4, 2), centred.
		BackButton->AddChild(BackText);
		PicturePlace(Root, BackButton, FAnchors(0.5f, 1.f), FMargin(0.f, -69.0810546875f, 100.f, 30.f), FVector2D(0.5f, 0.f), true);

		BackButton->OnClicked.AddDynamic(this, &UWasamiMaximizePictureWidget::OnBackClicked);
		BackButton->OnHovered.AddDynamic(this, &UWasamiMaximizePictureWidget::OnBackHovered);
		BackButton->OnUnhovered.AddDynamic(this, &UWasamiMaximizePictureWidget::OnBackUnhovered);
		ApplyAnimation();
	}
	return Super::RebuildWidget();
}

void UWasamiMaximizePictureWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Begin();
}

void UWasamiMaximizePictureWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiMaximizePictureWidget::Begin()
{
	// @305: PlayAnimation(FadeIn, 0, 1, Forward, 2); Image_1.SetBrushFromTexture(Texture, True).
	FadeInTime = 0.f;
	bFadeInReverse = false;
	bClosing = bFinished = false;
	CloseElapsed = 0.f;
	if (Image && Texture)
	{
		Image->SetBrushFromTexture(Texture, true);
	}
	ApplyAnimation();
}

void UWasamiMaximizePictureWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// FadeIn moves on first and the Delay comes due after it, as a user widget ticks them.
	const float Step = DeltaSeconds * FadeInSpeed;
	FadeInTime = bFadeInReverse ? FMath::Max(FadeInTime - Step, 0.f) : FMath::Min(FadeInTime + Step, FadeInLength);
	ApplyAnimation();
	if (bClosing)
	{
		CloseElapsed += DeltaSeconds;
		if (CloseElapsed >= BackDelay)
		{
			bFinished = true;
			RemoveFromParent();
		}
	}
}

void UWasamiMaximizePictureWidget::PressBack()
{
	// @144: PlayAnimation(FadeIn, 0, 1, Reverse, 2), which a reverse play starts at the end; @15: PlaySound2D(UI_Select_V3,
	// 1, 0.7); Delay(0.25) → RemoveFromParent, which a pending Delay ignores.
	FadeInTime = FadeInLength;
	bFadeInReverse = true;
	ApplyAnimation();
	if (GetWorld())
	{
		if (USoundBase* Loaded = SelectSound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, BackPitch);
		}
	}
	if (!bClosing)
	{
		bClosing = true;
		CloseElapsed = 0.f;
	}
}

void UWasamiMaximizePictureWidget::HoverBack(bool bHovered)
{
	// @238: Back.SetColorAndOpacity(white); @196: Back.SetColorAndOpacity(Unhovered Color).
	if (BackButton)
	{
		const float Grey = bHovered ? 1.f : UnhoveredGrey;
		BackButton->SetColorAndOpacity(FLinearColor(Grey, Grey, Grey, 1.f));
	}
}

void UWasamiMaximizePictureWidget::ApplyAnimation()
{
	if (Root)
	{
		Root->SetRenderOpacity(EvaluateOpacity(FadeInTime));
	}
	if (Ground)
	{
		const float Scale = EvaluateScale(FadeInTime);
		FWidgetTransform Transform = Ground->GetRenderTransform();
		Transform.Scale = FVector2D(Scale, Scale);
		Ground->SetRenderTransform(Transform);
	}
	// TextBlock_1's binding: the Text.
	if (TextBlock)
	{
		TextBlock->SetText(Text);
	}
}

void UWasamiMaximizePictureWidget::OnBackClicked()
{
	PressBack();
}

void UWasamiMaximizePictureWidget::OnBackHovered()
{
	HoverBack(true);
}

void UWasamiMaximizePictureWidget::OnBackUnhovered()
{
	HoverBack(false);
}

float UWasamiMaximizePictureWidget::EvaluateOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PictureOpacityKeys);
	return Eval(Curve, Seconds, FadeInLength);
}

float UWasamiMaximizePictureWidget::EvaluateScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(PictureScaleKeys);
	return Eval(Curve, Seconds, FadeInLength);
}
