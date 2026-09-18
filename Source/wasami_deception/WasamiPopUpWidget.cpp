#include "WasamiPopUpWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
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

	// Popup (pak_reference_2's UMG_PopUp): Givingupbox's and CanvasPanel_3's RenderOpacity 0 → 1 (0.25 s; their sections
	// end there and the value stays), and Givingupbox's scale 0 → 1 (0.25 s, UE's auto tangent, so it overshoots a
	// little) → 1 (0.5 s).
	const FAnimKey OpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey ScaleKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05}, {30000., 1.f, 0., 0.}};

	// The buttons' ColorAndOpacity (and Unhovered Color), and PopupText's Default row: helvetica-normal 28 in 0.965.
	constexpr float ButtonGrey = 0.114583001f;
	constexpr float QuestionGrey = 0.964685977f;
	constexpr float QuestionSize = 28.f;
	// YES and NO: the text block's default font (the engine's Roboto Bold) at 28.
	constexpr float AnswerSize = 28.f;

	// redblock's tint.
	const FLinearColor WashRed(0.182292f, 0.f, 0.f, 0.371429f);

	void Place(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets, const FVector2D& Alignment,
		bool bAutoSize)
	{
		UCanvasPanelSlot* Slot = Panel->AddChildToCanvas(Child);
		FAnchorData Layout;
		Layout.Anchors = Anchors;
		Layout.Offsets = Offsets;
		Layout.Alignment = Alignment;
		Slot->SetLayout(Layout);
		Slot->SetAutoSize(bAutoSize);
	}
}

UWasamiPopUpWidget::UWasamiPopUpWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RestartFrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/restart_window_frame_2")));
	QuitFrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/quit_window_frame")));
	BlankFrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Pause/blank_window_frame")));
	QuestionFont = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-normal_Font")));
	PopUpSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Window_PopUp_V3")));
	SelectSound = TSoftObjectPtr<USoundBase>(WasamiAssets::Path(TEXT("/Game/DD/Audio/UI/UI_Select_V3")));
}

UWasamiPopUpWidget* UWasamiPopUpWidget::Show(const UObject* WorldContextObject, const FText& InText, uint8 InFrame, int32 ZOrder)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiPopUpWidget* PopUp = CreateWidget<UWasamiPopUpWidget>(Controller, StaticClass());
	if (PopUp)
	{
		PopUp->Text = InText;
		PopUp->Frame = InFrame;
		PopUp->AddToViewport(ZOrder);
	}
	return PopUp;
}

TSharedRef<SWidget> UWasamiPopUpWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Each slot is the original's CanvasPanelSlot, in the original's order.
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
		const FAnchors Middle(0.5f, 0.5f);
		const FVector2D Centred(0.5f, 0.5f);

		// CanvasPanel_3: the blur and the red wash over the whole screen. Visible, so nothing under it takes a click.
		Wash = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_3"));
		Wash->SetVisibility(ESlateVisibility::Visible);
		UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_1"));
		Blur->SetBlurStrength(8.f);
		Place(Wash, Blur, Fill, FMargin(0.f, -12.012011528015137f, -12.912841796875f, -10.48046875f), FVector2D::ZeroVector, false);
		UImage* Red = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("redblock"));
		FSlateBrush RedBrush;
		RedBrush.TintColor = FSlateColor(WashRed);
		Red->SetBrush(RedBrush);
		Red->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Wash, Red, Fill, FMargin(-15.0150146484375f, -18.018016815185547f, -21.921836853027344f, -28.49853515625f),
			FVector2D::ZeroVector, false);
		Place(Root, Wash, Fill, FMargin(0.f, 0.f, -50.450401306152344f, -57.027000427246094f), FVector2D::ZeroVector, false);

		// Givingupbox: the frame and, 115 px below its middle, the question over YES and NO; Popup fades and scales it.
		Box = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Givingupbox"));
		Box->SetRenderOpacity(0.f);
		FrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_420"));
		FrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		FSlateBrush FrameBrush;
		FrameBrush.ImageSize = FVector2D(1222.f, 928.f);
		FrameImage->SetBrush(FrameBrush);
		Place(Box, FrameImage, Middle, FMargin(0.f, 0.f, 100.f, 40.f), Centred, true);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_0"));
		// RichTextBlock_374 with PopupText's Default row; the questions carry no tags, so a text block in that style
		// draws them the same.
		Question = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RichTextBlock_374"));
		FSlateFontInfo QuestionStyle;
		QuestionStyle.FontObject = QuestionFont.LoadSynchronous();
		QuestionStyle.TypefaceFontName = TEXT("Default");
		QuestionStyle.Size = QuestionSize;
		Question->SetFont(QuestionStyle);
		Question->SetColorAndOpacity(FSlateColor(FLinearColor(QuestionGrey, QuestionGrey, QuestionGrey, 1.f)));
		Question->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* QuestionSlot = Column->AddChildToVerticalBox(Question);
		QuestionSlot->SetHorizontalAlignment(HAlign_Center);
		QuestionSlot->SetVerticalAlignment(VAlign_Center);

		UHorizontalBox* Answers = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_0"));
		auto MakeButton = [this, Answers](const TCHAR* Name, const TCHAR* TextName, const TCHAR* Label, const FMargin& SlotPadding)
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			Button->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, 1.f));
			Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
			UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
			Caption->SetText(FText::FromString(Label));
			FSlateFontInfo LabelFont = Caption->GetFont();
			LabelFont.Size = AnswerSize;
			Caption->SetFont(LabelFont);
			Button->AddChild(Caption);
			UHorizontalBoxSlot* ButtonSlot = Answers->AddChildToHorizontalBox(Button);
			ButtonSlot->SetPadding(SlotPadding);
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
			return Button;
		};
		YesButton = MakeButton(TEXT("Yes"), TEXT("TextBlock_4"), TEXT("YES"), FMargin(0.f, 40.f, 50.f, 0.f));
		NoButton = MakeButton(TEXT("No"), TEXT("TextBlock_5"), TEXT("NO"), FMargin(0.f, 40.f, 0.f, 0.f));
		UVerticalBoxSlot* AnswersSlot = Column->AddChildToVerticalBox(Answers);
		AnswersSlot->SetHorizontalAlignment(HAlign_Center);
		AnswersSlot->SetVerticalAlignment(VAlign_Center);
		Place(Box, Column, Middle, FMargin(0.f, 115.20497131347656f, 100.f, 30.f), Centred, true);
		Place(Root, Box, Middle, FMargin(0.f, 0.f, 100.f, 30.f), Centred, true);

		// The original's ComponentDelegateBinding: YES on its click, NO on its press, both on hover.
		YesButton->OnClicked.AddDynamic(this, &UWasamiPopUpWidget::OnYesClicked);
		NoButton->OnPressed.AddDynamic(this, &UWasamiPopUpWidget::OnNoPressed);
		YesButton->OnHovered.AddDynamic(this, &UWasamiPopUpWidget::OnYesHovered);
		YesButton->OnUnhovered.AddDynamic(this, &UWasamiPopUpWidget::OnYesUnhovered);
		NoButton->OnHovered.AddDynamic(this, &UWasamiPopUpWidget::OnNoHovered);
		NoButton->OnUnhovered.AddDynamic(this, &UWasamiPopUpWidget::OnNoUnhovered);
	}
	return Super::RebuildWidget();
}

void UWasamiPopUpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Construct: the pop-up sound, Popup from 0, the frame for Frame (at its own size) and the question.
	PlaySound(PopUpSound, 1.f);
	PopupTime = 0.f;
	bClosing = bFinished = false;
	CloseElapsed = 0.f;
	ApplyAnimation();
	const TSoftObjectPtr<UTexture2D>& FrameTexture = Frame == QuitFrame ? QuitFrameTexture : Frame == BlankFrame ? BlankFrameTexture : RestartFrameTexture;
	if (FrameImage)
	{
		FrameImage->SetBrushFromTexture(FrameTexture.LoadSynchronous(), true);
	}
	if (Question)
	{
		Question->SetText(Text);
	}
}

void UWasamiPopUpWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiPopUpWidget::Advance(float DeltaSeconds)
{
	if (bFinished)
	{
		return;
	}
	// Popup moves on first and the Delay comes due after it, as a user widget ticks them.
	PopupTime = bClosing ? FMath::Max(PopupTime - DeltaSeconds, 0.f) : FMath::Min(PopupTime + DeltaSeconds, PopupLength);
	ApplyAnimation();
	if (bClosing)
	{
		CloseElapsed += DeltaSeconds;
		if (CloseElapsed >= CloseDelay)
		{
			bFinished = true;
			RemoveFromParent();
		}
	}
}

void UWasamiPopUpWidget::CloseAnimation()
{
	StartClosing();
}

void UWasamiPopUpWidget::PressNo()
{
	StartClosing();
	PlaySound(SelectSound, NoPitch);
	OnNoClick.Broadcast();
}

void UWasamiPopUpWidget::StartClosing()
{
	// PlayAnimation(Popup, 0.25, Reverse) starts over each time; the Delay does not (a pending Delay ignores another).
	PopupTime = CloseFrom;
	if (!bClosing)
	{
		bClosing = true;
		CloseElapsed = 0.f;
	}
	ApplyAnimation();
}

void UWasamiPopUpWidget::ApplyAnimation()
{
	BoxOpacity = EvaluateOpacity(PopupTime);
	BoxScale = EvaluateScale(PopupTime);
	if (Box)
	{
		Box->SetRenderOpacity(BoxOpacity);
		Box->SetRenderScale(FVector2D(BoxScale, BoxScale));
	}
	if (Wash)
	{
		Wash->SetRenderOpacity(BoxOpacity);
	}
}

void UWasamiPopUpWidget::OnYesClicked()
{
	PlaySound(SelectSound, 1.f);
	OnYesClick.Broadcast();
}

void UWasamiPopUpWidget::OnNoPressed()
{
	PressNo();
}

void UWasamiPopUpWidget::OnYesHovered()
{
	YesButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiPopUpWidget::OnYesUnhovered()
{
	YesButton->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, 1.f));
}

void UWasamiPopUpWidget::OnNoHovered()
{
	NoButton->SetColorAndOpacity(FLinearColor::White);
}

void UWasamiPopUpWidget::OnNoUnhovered()
{
	NoButton->SetColorAndOpacity(FLinearColor(ButtonGrey, ButtonGrey, ButtonGrey, 1.f));
}

void UWasamiPopUpWidget::PlaySound(const TSoftObjectPtr<USoundBase>& Sound, float Pitch) const
{
	// A UI sound, which goes on while the game is paused.
	if (GetWorld())
	{
		if (USoundBase* Loaded = Sound.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Loaded, 1.f, Pitch);
		}
	}
}

float UWasamiPopUpWidget::EvaluateOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(OpacityKeys);
	return Eval(Curve, Seconds, PopupLength);
}

float UWasamiPopUpWidget::EvaluateScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(ScaleKeys);
	return Eval(Curve, Seconds, PopupLength);
}
