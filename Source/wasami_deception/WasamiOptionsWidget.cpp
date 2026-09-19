#include "WasamiOptionsWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "WasamiAssets.h"
#include "WasamiGameInstance.h"
#include "WasamiGameMode.h"
#include "WasamiWidgetAnimation.h"

namespace
{
	using WasamiWidgetAnimation::Eval;
	using WasamiWidgetAnimation::FAnimKey;
	using WasamiWidgetAnimation::MakeCurve;

	// FadeIn (pak_reference's UMG_Options; the same keys as UMG_PopUp's Popup): Blur+Red's and CanvasPanel_2's
	// RenderOpacity 0 → 1 (0.25 s; the sections keep the value), and CanvasPanel_2's scale 0 → 1 (0.25 s, UE's auto
	// tangent, so it overshoots a little) → 1 (0.5 s). The animation's binding named CanvasPanel_0 is CanvasPanel_2.
	const FAnimKey OptionsFadeInOpacityKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 0., 0.}};
	const FAnimKey OptionsFadeInScaleKeys[] = {{0., 0.f, 0., 0.}, {15000., 1.f, 3.333333370392211e-05, 3.333333370392211e-05}, {30000., 1.f, 0., 0.}};

	// The labels' and the value boxes' ColorAndOpacity, SAVE & EXIT's and CANCEL's (and Unhovered Color), and Image_1's.
	constexpr float OptionsLabelGrey = 0.5f;
	constexpr float OptionsButtonGrey = 0.114583001f;
	const FLinearColor OptionsWashRed(0.182292f, 0.f, 0.f, 0.371429f);

	// The rows' widths: the labels', DIFFICULTY's label's, the text boxes' (QUALITY's, DIFFICULTY's, RESOLUTION's) and
	// the slider boxes'.
	constexpr float OptionsLabelWidth = 310.f;
	constexpr float OptionsShortLabelWidth = 240.99429321289062f;
	constexpr float OptionsWideValueWidth = 215.f;
	constexpr float OptionsNarrowValueWidth = 61.27743911743164f;

	UCanvasPanelSlot* PlaceInOptions(UCanvasPanel* Panel, UWidget* Child, const FAnchors& Anchors, const FMargin& Offsets,
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

	/** A brush of Texture at Size, drawn as DrawAs with Margin (the fraction of Size the box keeps unstretched). */
	FSlateBrush OptionsBrush(UTexture2D* Texture, const FVector2D& Size, ESlateBrushDrawType::Type DrawAs = ESlateBrushDrawType::Image,
		const FMargin& Margin = FMargin(0.f), const FLinearColor& Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = Size;
		Brush.DrawAs = DrawAs;
		Brush.Margin = Margin;
		Brush.TintColor = FSlateColor(Tint);
		return Brush;
	}

	FSlateFontInfo OptionsFontInfo(UFont* Object, float Size)
	{
		FSlateFontInfo Info;
		Info.FontObject = Object;
		Info.TypefaceFontName = TEXT("Default");
		Info.Size = Size;
		return Info;
	}

	void AddToOptionsRow(UHorizontalBox* Row, UWidget* Child, const FMargin& Padding, EHorizontalAlignment HAlign, EVerticalAlignment VAlign,
		bool bFill = false)
	{
		UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Child);
		Slot->SetPadding(Padding);
		Slot->SetHorizontalAlignment(HAlign);
		Slot->SetVerticalAlignment(VAlign);
		if (bFill)
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	void SetOptionsText(UTextBlock* Text, const FText& Value)
	{
		if (Text && !Text->GetText().EqualTo(Value))
		{
			Text->SetText(Value);
		}
	}
}

UWasamiOptionsWidget::UWasamiOptionsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FrameTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/options_window_frame")));
	ValueBoxTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/selection_bar")));
	ArrowTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/selection_bar_arrow_normal")));
	ArrowHoverTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/selection_bar_arrow_hover")));
	ThumbTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/slider_bar_tab")));
	CheckTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/checkbox_icon")));
	CheckedTexture = TSoftObjectPtr<UTexture2D>(WasamiAssets::Path(TEXT("/Game/DD/UI/Menu/Settings/checkbox_icon_checked")));
	Font = TSoftObjectPtr<UFont>(WasamiAssets::Path(TEXT("/Game/DD/UI/Fonts/helvetica-neue-bold_Font")));
}

UWasamiOptionsWidget* UWasamiOptionsWidget::Show(const UObject* WorldContextObject)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(WorldContextObject, 0);
	if (!Controller)
	{
		return nullptr;
	}
	UWasamiOptionsWidget* Screen = CreateWidget<UWasamiOptionsWidget>(Controller, StaticClass());
	if (Screen)
	{
		Screen->AddToViewport(ViewportZOrder);
	}
	return Screen;
}

bool UWasamiOptionsWidget::ShowsDifficulty(const FString& InLevelName)
{
	return InLevelName == AWasamiGameMode::TitleLevelName;
}

TSharedRef<SWidget> UWasamiOptionsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_0"));
		WidgetTree->RootWidget = Root;
		BuildScreen(Root);
	}
	return Super::RebuildWidget();
}

void UWasamiOptionsWidget::BuildScreen(UCanvasPanel* Root)
{
	// Each slot is the original's (pak_reference's UMG_Options), in its order; what the export leaves out is the slot's
	// default. The root is drawn a little larger than the screen.
	const FAnchors Fill(0.f, 0.f, 1.f, 1.f);
	const FAnchors Middle(0.5f, 0.5f);
	const FVector2D Centred(0.5f, 0.5f);
	Root->SetRenderScale(FVector2D(RootScale, RootScale));
	UFont* HeadingFont = Font.LoadSynchronous();
	UTexture2D* Arrow = ArrowTexture.LoadSynchronous();
	UTexture2D* ArrowHover = ArrowHoverTexture.LoadSynchronous();
	UTexture2D* ValueBox = ValueBoxTexture.LoadSynchronous();
	UTexture2D* Thumb = ThumbTexture.LoadSynchronous();
	UTexture2D* Check = CheckTexture.LoadSynchronous();
	UTexture2D* Checked = CheckedTexture.LoadSynchronous();

	// Blur+Red: the blur and the red wash over the whole screen, a little past its right and bottom; FadeIn brings it in.
	Wash = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Blur+Red"));
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("BackgroundBlur_0"));
	Blur->SetBlurStrength(8.f);
	PlaceInOptions(Wash, Blur, Fill, FMargin(0.f, -12.012011528015137f, -12.912841796875f, -10.48046875f));
	UImage* Red = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_1"));
	FSlateBrush RedBrush;
	RedBrush.TintColor = FSlateColor(OptionsWashRed);
	Red->SetBrush(RedBrush);
	PlaceInOptions(Wash, Red, Fill, FMargin(-15.0150146484375f, -18.018016815185547f, -21.921836853027344f, -28.49853515625f));
	PlaceInOptions(Root, Wash, Fill, FMargin(0.f, 0.f, -21.921924591064453f, -36.005943298339844f));

	// CanvasPanel_2: the frame, the grid and the two buttons; FadeIn scales and fades it in.
	Box = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_2"));
	UImage* Frame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_31"));
	Frame->SetBrush(OptionsBrush(FrameTexture.LoadSynchronous(), FVector2D(1563.1300048828125f, 1083.f)));
	PlaceInOptions(Box, Frame, Fill, FMargin(179.39593505859375f, -0.9594730138778687f, 179.39599609375f, -0.9594730138778687f), Centred, true);

	// The pieces of a row.
	auto MakeLabel = [this](const TCHAR* Name, const TCHAR* Label, float MinWidth)
	{
		// The text block's default font (the engine's Roboto Bold 24).
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Label));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(OptionsLabelGrey, OptionsLabelGrey, OptionsLabelGrey, 1.f)));
		Text->SetMinDesiredWidth(MinWidth);
		return Text;
	};
	auto MakeHeading = [this, HeadingFont](const TCHAR* Name, const TCHAR* Label)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Label));
		Text->SetFont(OptionsFontInfo(HeadingFont, 36.f));
		Text->SetMinDesiredWidth(550.f);
		Text->SetJustification(ETextJustify::Center);
		return Text;
	};
	// A value box: the border's selection_bar drawn as a box (0.1 of 242 kept at each edge) around a centred text.
	auto MakeValueBox = [this, HeadingFont, ValueBox](const TCHAR* BorderName, const TCHAR* TextName, const TCHAR* Value, float MinWidth,
		bool bGrey, TObjectPtr<UTextBlock>* OutText)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
		Border->SetBrush(OptionsBrush(ValueBox, FVector2D(242.f, 242.f), ESlateBrushDrawType::Box, FMargin(0.1f)));
		Border->SetHorizontalAlignment(HAlign_Center);
		Border->SetVerticalAlignment(VAlign_Center);
		Border->SetPadding(FMargin(10.f));
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		Text->SetText(FText::FromString(Value));
		if (bGrey)
		{
			Text->SetColorAndOpacity(FSlateColor(FLinearColor(OptionsLabelGrey, OptionsLabelGrey, OptionsLabelGrey, 1.f)));
		}
		Text->SetFont(OptionsFontInfo(HeadingFont, 18.f));
		Text->SetMinDesiredWidth(MinWidth);
		Text->SetJustification(ETextJustify::Center);
		Border->SetContent(Text);
		if (OutText)
		{
			*OutText = Text;
		}
		return Border;
	};
	// An arrow: a button of the arrow alone (21 × 32), the left one turned round. QUALITY's are restyled by Construct:
	// drawn as images without padding. The others keep the tree's: UE 4's default button (drawn as boxes, 8/32 of the
	// size kept at each edge, with its paddings) with the arrows as its brushes.
	auto MakeArrow = [this, Arrow, ArrowHover](const TCHAR* Name, bool bLeft, bool bConstructStyle)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		const FVector2D Size(21.f, 32.f);
		if (bConstructStyle)
		{
			FSlateBrush Disabled;
			Disabled.ImageSize = FVector2D::ZeroVector;
			Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
			Style.SetNormal(OptionsBrush(Arrow, Size)).SetHovered(OptionsBrush(ArrowHover, Size)).SetPressed(OptionsBrush(Arrow, Size))
				.SetDisabled(Disabled).SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));
		}
		else
		{
			const FMargin Kept(8.f / 32.f);
			Style.SetNormal(OptionsBrush(Arrow, Size, ESlateBrushDrawType::Box, Kept)).SetHovered(OptionsBrush(ArrowHover, Size, ESlateBrushDrawType::Box, Kept))
				.SetPressed(OptionsBrush(Arrow, Size, ESlateBrushDrawType::Box, Kept)).SetNormalPadding(FMargin(2.f))
				.SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		}
		Button->SetStyle(Style);
		if (bLeft)
		{
			Button->SetRenderTransformAngle(180.f);
		}
		return Button;
	};
	// A slider: the red bar 4 thick (UE 4's white bar brush, its margin as written) and slider_bar_tab 18 × 30 as the thumb
	// (UE 4's default thumb drawn as a box, 8/32 kept at each edge), at 1 until Setup Values.
	auto MakeSlider = [this, Thumb](const TCHAR* Name, TObjectPtr<USlider>* OutSlider)
	{
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
		FSlateColorBrush Bar(FLinearColor::White);
		Bar.Margin = FMargin(0.f, 0.f, 1.f, 1.f);
		const FMargin Kept(8.f / 32.f);
		const FSlateBrush ThumbBrush = OptionsBrush(Thumb, FVector2D(18.f, 30.f), ESlateBrushDrawType::Box, Kept);
		FSliderStyle Style;
		// UE 4's slider style had no hovered pictures: the normal ones stand for them.
		Style.SetNormalBarImage(Bar).SetHoveredBarImage(Bar).SetDisabledBarImage(FSlateColorBrush(FLinearColor::Gray))
			.SetNormalThumbImage(ThumbBrush).SetHoveredThumbImage(ThumbBrush)
			.SetDisabledThumbImage(OptionsBrush(Thumb, FVector2D(22.f, 38.f), ESlateBrushDrawType::Box, Kept)).SetBarThickness(4.f);
		Slider->SetWidgetStyle(Style);
		Slider->SetSliderBarColor(FLinearColor(1.f, 0.f, 0.f, 1.f));
		Slider->SetSliderHandleColor(FLinearColor::White);
		Slider->SetValue(1.f);
		if (OutSlider)
		{
			*OutSlider = Slider;
		}
		return Slider;
	};
	// A check box in Construct's style: checkbox_icon / _checked (55 × 64), the hovered ones in grey.
	auto MakeCheck = [this, Check, Checked](const TCHAR* Name, TObjectPtr<UCheckBox>* OutCheck)
	{
		UCheckBox* CheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), Name);
		const FVector2D Size(55.f, 64.f);
		const FLinearColor Hover(CheckHoverGrey, CheckHoverGrey, CheckHoverGrey, 1.f);
		FCheckBoxStyle Style;
		Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
			.SetUncheckedImage(OptionsBrush(Check, Size)).SetUncheckedHoveredImage(OptionsBrush(Check, Size, ESlateBrushDrawType::Image, FMargin(0.f), Hover))
			.SetUncheckedPressedImage(OptionsBrush(Check, Size))
			.SetCheckedImage(OptionsBrush(Checked, Size)).SetCheckedHoveredImage(OptionsBrush(Checked, Size, ESlateBrushDrawType::Image, FMargin(0.f), Hover))
			.SetCheckedPressedImage(OptionsBrush(Checked, Size))
			.SetPadding(FMargin(2.f, 0.f, 0.f, 0.f));
		CheckBox->SetWidgetStyle(Style);
		CheckBox->SetCheckedState(ECheckBoxState::Checked);
		if (OutCheck)
		{
			*OutCheck = CheckBox;
		}
		return CheckBox;
	};
	auto MakeRow = [this](UVerticalBox* Column, const TCHAR* Name, float Top)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
		Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		return Row;
	};
	auto MakeColumn = [this, &MakeHeading](const TCHAR* Name, const TCHAR* HeadingName, const TCHAR* Heading, const FVector2D& Translation)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		Column->SetRenderTranslation(Translation);
		UVerticalBoxSlot* HeadingSlot = Column->AddChildToVerticalBox(MakeHeading(HeadingName, Heading));
		HeadingSlot->SetHorizontalAlignment(HAlign_Center);
		HeadingSlot->SetVerticalAlignment(VAlign_Center);
		return Column;
	};
	// A slider row: its label, its value box 26 to the right, and the slider filling the rest.
	auto AddSliderRow = [&](UVerticalBox* Column, const TCHAR* RowName, float Top, const TCHAR* LabelName, const TCHAR* Label,
		const TCHAR* BorderName, const TCHAR* TextName, TObjectPtr<UTextBlock>* OutText, const TCHAR* SliderName, TObjectPtr<USlider>* OutSlider,
		float SliderRight)
	{
		UHorizontalBox* Row = MakeRow(Column, RowName, Top);
		AddToOptionsRow(Row, MakeLabel(LabelName, Label, OptionsLabelWidth), FMargin(0.f), HAlign_Center, VAlign_Center);
		AddToOptionsRow(Row, MakeValueBox(BorderName, TextName, TEXT("0"), OptionsNarrowValueWidth, true, OutText), FMargin(26.f, 0.f, 0.f, 0.f), HAlign_Fill,
			VAlign_Fill);
		AddToOptionsRow(Row, MakeSlider(SliderName, OutSlider), FMargin(0.f, 0.f, SliderRight, 0.f), HAlign_Fill, VAlign_Center, true);
	};
	auto AddCheckRow = [&](UVerticalBox* Column, const TCHAR* RowName, const TCHAR* LabelName, const TCHAR* Label, const TCHAR* CheckName,
		TObjectPtr<UCheckBox>* OutCheck)
	{
		UHorizontalBox* Row = MakeRow(Column, RowName, 15.f);
		AddToOptionsRow(Row, MakeLabel(LabelName, Label, OptionsLabelWidth), FMargin(0.f), HAlign_Center, VAlign_Center);
		AddToOptionsRow(Row, MakeCheck(CheckName, OutCheck), FMargin(18.f, 0.f, 0.f, 0.f), HAlign_Center, VAlign_Center);
	};
	// An arrow row: its label, the left arrow, the value box and the right arrow.
	auto AddArrowRow = [&](UHorizontalBox* Row, const TCHAR* LabelName, const TCHAR* Label, float LabelWidth, const TCHAR* LowerName,
		const TCHAR* BorderName, const TCHAR* TextName, const TCHAR* Value, bool bGrey, TObjectPtr<UTextBlock>* OutText, const TCHAR* HigherName,
		bool bConstructStyle)
	{
		AddToOptionsRow(Row, MakeLabel(LabelName, Label, LabelWidth), FMargin(0.f), HAlign_Fill, VAlign_Center);
		AddToOptionsRow(Row, MakeArrow(LowerName, true, bConstructStyle), FMargin(0.f, 0.f, 5.f, 0.f), HAlign_Center, VAlign_Center);
		AddToOptionsRow(Row, MakeValueBox(BorderName, TextName, Value, OptionsWideValueWidth, bGrey, OutText), FMargin(0.f), HAlign_Fill, VAlign_Fill);
		AddToOptionsRow(Row, MakeArrow(HigherName, false, bConstructStyle), FMargin(5.f, 0.f, 0.f, 0.f), HAlign_Center, VAlign_Center);
	};

	// GraphicsBox: QUALITY, RESOLUTION SCALE, BRIGHTNESS, and a RESOLUTION row the original keeps unseen (opacity 0).
	UVerticalBox* Graphics = MakeColumn(TEXT("GraphicsBox"), TEXT("TextBlock_0"), TEXT("GRAPHICS"), FVector2D(-50.f, 0.f));
	AddArrowRow(MakeRow(Graphics, TEXT("HorizontalBox_0"), 25.f), TEXT("TextBlock_1"), TEXT("QUALITY"), OptionsLabelWidth, TEXT("QualityLower"),
		TEXT("Border_0"), TEXT("TextBlock_4"), TEXT("HIGH"), true, &QualityText, TEXT("QualityHigher"), true);
	AddSliderRow(Graphics, TEXT("HorizontalBox_3"), 15.f, TEXT("TextBlock_6"), TEXT("RESOLUTION SCALE"), TEXT("Border_1"), TEXT("TextBlock_7"),
		&ResolutionScaleText, TEXT("ResolutionScaleSlider"), &ResolutionScaleSlider, 30.f);
	AddSliderRow(Graphics, TEXT("HorizontalBox_6"), 15.f, TEXT("BRIGHTNESS"), TEXT("BRIGHTNESS"), TEXT("Border_6"), TEXT("TextBlock_16"),
		&BrightnessText, TEXT("BrightnessSlider"), &BrightnessSlider, 30.f);
	UHorizontalBox* Resolution = MakeRow(Graphics, TEXT("HorizontalBox_1"), 15.f);
	Resolution->SetRenderOpacity(0.f);
	AddArrowRow(Resolution, TEXT("TextBlock_2"), TEXT("RESOLUTION"), OptionsLabelWidth, TEXT("SwitchResolutionLeft"), TEXT("Border_2"),
		TEXT("TextBlock_8"), TEXT("1920 X 1080"), true, nullptr, TEXT("Button_5"), false);

	// AudioBox: MUSIC, SFX, DIALOGUE, SUBTITLES, and another RESOLUTION row, hidden (it still takes its room).
	UVerticalBox* Audio = MakeColumn(TEXT("AudioBox"), TEXT("TextBlock_3"), TEXT("AUDIO"), FVector2D(-50.f, 0.f));
	AddSliderRow(Audio, TEXT("HorizontalBox_5"), 15.f, TEXT("TextBlock_13"), TEXT("MUSIC"), TEXT("Border_5"), TEXT("TextBlock_14"), &MusicText,
		TEXT("MusicSlider"), &MusicSlider, 30.f);
	AddSliderRow(Audio, TEXT("HorizontalBox_7"), 15.f, TEXT("TextBlock_17"), TEXT("SFX"), TEXT("Border_7"), TEXT("TextBlock_18"), &SFXText,
		TEXT("SFXSlider"), &SFXSlider, 30.f);
	AddSliderRow(Audio, TEXT("HorizontalBox_8"), 15.f, TEXT("TextBlock_19"), TEXT("DIALOGUE"), TEXT("Border_8"), TEXT("TextBlock_20"), &DialogueText,
		TEXT("DialogueSlider"), &DialogueSlider, 30.f);
	AddCheckRow(Audio, TEXT("HorizontalBox_9"), TEXT("TextBlock_21"), TEXT("SUBTITLES"), TEXT("SubtitlesCheck"), &SubtitlesCheck);
	UHorizontalBox* HiddenResolution = MakeRow(Audio, TEXT("HorizontalBox_4"), 15.f);
	HiddenResolution->SetVisibility(ESlateVisibility::Hidden);
	HiddenResolution->SetRenderOpacity(0.f);
	AddArrowRow(HiddenResolution, TEXT("TextBlock_11"), TEXT("RESOLUTION"), OptionsLabelWidth, TEXT("Button_4"), TEXT("Border_4"), TEXT("TextBlock_12"),
		TEXT("1920 X 1080"), false, nullptr, TEXT("Button_6"), false);

	// ControlsBox: MOUSE SENSITIVITY (its slider 10 past the row's right), HEAD BOBBING, INVERTED Y AXIS, TOGGLE SPRINT,
	// MOUSE SMOOTHING.
	UVerticalBox* Controls = MakeColumn(TEXT("ControlsBox"), TEXT("TextBlock_9"), TEXT("CONTROLS"), FVector2D(0.f, 254.f));
	AddSliderRow(Controls, TEXT("HorizontalBox_2"), 25.f, TEXT("TextBlock_10"), TEXT("MOUSE SENSITIVITY"), TEXT("Border_3"), TEXT("TextBlock_15"),
		&MouseSensitivityText, TEXT("MouseSensitivitySlider"), &MouseSensitivitySlider, -10.f);
	AddCheckRow(Controls, TEXT("HorizontalBox_13"), TEXT("TextBlock_27"), TEXT("HEAD BOBBING"), TEXT("HeadBobbingCheck"), &HeadBobbingCheck);
	AddCheckRow(Controls, TEXT("HorizontalBox_15"), TEXT("TextBlock_30"), TEXT("INVERTED Y AXIS"), TEXT("InvertedYCheck"), &InvertedYCheck);
	AddCheckRow(Controls, TEXT("HorizontalBox_10"), TEXT("TextBlock_22"), TEXT("TOGGLE SPRINT"), TEXT("ToggleSprintCheck"), &ToggleSprintCheck);
	AddCheckRow(Controls, TEXT("HorizontalBox_12"), TEXT("TextBlock_24"), TEXT("MOUSE SMOOTHING"), TEXT("MouseSmoothingCheck"), &MouseSmoothingCheck);

	// DifficultyBox: DIFFICULTY (its label narrower).
	DifficultyBox = MakeColumn(TEXT("DifficultyBox"), TEXT("TextBlock_25"), TEXT("DIFFICULTY"), FVector2D(650.f, -92.f));
	AddArrowRow(MakeRow(DifficultyBox, TEXT("HorizontalBox_14"), 25.f), TEXT("TextBlock_26"), TEXT("DIFFICULTY"), OptionsShortLabelWidth,
		TEXT("DifficultyLower"), TEXT("Border_9"), TEXT("TextBlock_28"), TEXT("HIGH"), true, &DifficultyText, TEXT("DifficultyHigher"), false);

	// GridPanel_0: column 0 and row 0 fill half the room left (Slate's grid; the rows and columns are the original's).
	UGridPanel* Grid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("GridPanel_0"));
	Grid->SetColumnFill(0, 0.5f);
	Grid->SetRowFill(0, 0.5f);
	auto AddToGrid = [Grid](UWidget* Child, int32 Row, int32 RowSpan, int32 Column, int32 ColumnSpan, float Top, const FVector2D& Nudge)
	{
		UGridSlot* Slot = Grid->AddChildToGrid(Child, Row, Column);
		Slot->SetRowSpan(RowSpan);
		Slot->SetColumnSpan(ColumnSpan);
		Slot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetNudge(Nudge);
	};
	AddToGrid(Graphics, 1, 3, 0, 1, 0.f, FVector2D::ZeroVector);
	AddToGrid(Audio, 3, 1, 0, 1, 150.f, FVector2D(0.f, 34.41068649291992f));
	AddToGrid(Controls, 2, 2, 1, 2, 150.f, FVector2D(34.645301818847656f, -182.7278289794922f));
	AddToGrid(DifficultyBox, 1, 3, 0, 1, 0.f, FVector2D::ZeroVector);

	// CanvasPanel_1 (1338.09 × 812.28 from (-673.56, -383.54) off the middle), the grid (1234.53 × 673.85) centred in it
	// at (-16.27, -28.77).
	UCanvasPanel* GridHolder = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_1"));
	PlaceInOptions(GridHolder, Grid, Middle, FMargin(-16.266260147094727f, -28.772523880004883f, 1234.5345458984375f, 673.8483276367188f), Centred);
	PlaceInOptions(Box, GridHolder, Middle, FMargin(-673.5615234375f, -383.54351806640625f, 1338.0880126953125f, 812.2822265625f));

	// HorizontalBox_11: SAVE & EXIT and CANCEL, 40 either side of the middle of the bottom, raised by a quarter of their
	// height. Each is UE 4's default button, its background clear and its content in the grey of Unhovered Color.
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HorizontalBox_11"));
	auto MakeButton = [this, HeadingFont, Buttons](const TCHAR* Name, const TCHAR* TextName, const TCHAR* Label, const FMargin& SlotPadding)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormalPadding(FMargin(2.f)).SetPressedPadding(FMargin(2.f, 3.f, 2.f, 1.f));
		Button->SetStyle(Style);
		Button->SetColorAndOpacity(FLinearColor(OptionsButtonGrey, OptionsButtonGrey, OptionsButtonGrey, 1.f));
		Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		Text->SetText(FText::FromString(Label));
		Text->SetFont(OptionsFontInfo(HeadingFont, 30.f));
		// ButtonSlot_0: UE's defaults (4, 2), centred.
		UButtonSlot* TextSlot = Cast<UButtonSlot>(Button->AddChild(Text));
		TextSlot->SetPadding(FMargin(4.f, 2.f));
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		AddToOptionsRow(Buttons, Button, SlotPadding, HAlign_Center, VAlign_Center);
		return Button;
	};
	MakeButton(TEXT("ApplyButton"), TEXT("TextBlock_5"), TEXT("SAVE & EXIT"), FMargin(0.f, 0.f, 40.f, 0.f));
	MakeButton(TEXT("CancelButton"), TEXT("TextBlock_23"), TEXT("CANCEL"), FMargin(40.f, 0.f, 0.f, 0.f));
	PlaceInOptions(Box, Buttons, FAnchors(0.5f, 1.f), FMargin(0.f, -6.752490997314453f, 100.f, 30.f), FVector2D(0.5f, 1.25f), true);

	PlaceInOptions(Root, Box, Fill, FMargin(0.f));
	ApplyAnimation();
}

void UWasamiOptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Cast To GameMode → Global Settings Save Instance: the settings the game instance keeps (new defaults without one).
	UWasamiSettingsSaveGame* Read = Settings;
	FString Level = LevelName;
	if (UWorld* World = GetWorld())
	{
		if (!Read)
		{
			if (UWasamiGameInstance* Instance = Cast<UWasamiGameInstance>(World->GetGameInstance()))
			{
				Read = Instance->GetSettings();
			}
		}
		if (Level.IsEmpty())
		{
			Level = UGameplayStatics::GetCurrentLevelName(this, true);
		}
	}
	if (!Read)
	{
		Read = NewObject<UWasamiSettingsSaveGame>(this);
	}
	Begin(Read, ShowsDifficulty(Level));
}

void UWasamiOptionsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UWasamiOptionsWidget::Begin(UWasamiSettingsSaveGame* InSettings, bool bInShowDifficulty)
{
	// Construct (@21404): SetInputMode_UIOnlyEx(Self, DoNotLock), the cursor shown, PlayAnimation(FadeIn), Cast To
	// GameMode, Setup Values, the check boxes' and QUALITY's arrows' styles (built into the tree), and DifficultyBox
	// removed unless the level is the title.
	if (APlayerController* Controller = GetOwningPlayer())
	{
		UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, this, EMouseLockMode::DoNotLock);
		Controller->SetShowMouseCursor(true);
	}
	FadeInTime = 0.f;
	ApplyAnimation();
	Settings = InSettings;
	if (Settings)
	{
		SetupValues(*Settings);
	}
	if (!bInShowDifficulty && DifficultyBox)
	{
		DifficultyBox->RemoveFromParent();
		DifficultyBox = nullptr;
	}
	RefreshTexts();
}

void UWasamiOptionsWidget::SetupValues(const UWasamiSettingsSaveGame& InSettings)
{
	// Setup Values (@17146), in its order.
	QualitySetting = InSettings.Quality;
	auto SetSlider = [](USlider* Slider, float Value)
	{
		if (Slider)
		{
			Slider->SetValue(Value);
		}
	};
	auto SetCheck = [](UCheckBox* Check, bool bChecked)
	{
		if (Check)
		{
			Check->SetCheckedState(bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		}
	};
	SetSlider(ResolutionScaleSlider, InSettings.ResolutionScale);
	SetSlider(BrightnessSlider, InSettings.Brightness);
	SetSlider(MusicSlider, InSettings.Music);
	SetSlider(SFXSlider, InSettings.SFX);
	SetSlider(DialogueSlider, InSettings.Dialogue);
	SetCheck(SubtitlesCheck, InSettings.bSubtitles);
	SetSlider(MouseSensitivitySlider, InSettings.MouseSensitivity);
	SetCheck(HeadBobbingCheck, InSettings.bHeadBobbing);
	SetCheck(InvertedYCheck, InSettings.bInvertedYAxis);
	SetCheck(ToggleSprintCheck, InSettings.bToggleSprint);
	SetCheck(MouseSmoothingCheck, InSettings.bMouseSmoothing);
	DifficultySetting = InSettings.Difficulty;
	RefreshTexts();
}

void UWasamiOptionsWidget::Advance(float DeltaSeconds)
{
	FadeInTime = FMath::Min(FadeInTime + DeltaSeconds, FadeInLength);
	ApplyAnimation();
	RefreshTexts();
}

void UWasamiOptionsWidget::ApplyAnimation()
{
	BoxOpacity = WashOpacity = EvaluateFadeInOpacity(FadeInTime);
	BoxScale = EvaluateFadeInScale(FadeInTime);
	if (Box)
	{
		Box->SetRenderOpacity(BoxOpacity);
		Box->SetRenderScale(FVector2D(BoxScale, BoxScale));
	}
	if (Wash)
	{
		Wash->SetRenderOpacity(WashOpacity);
	}
}

void UWasamiOptionsWidget::RefreshTexts()
{
	// The bindings: Resolution Scale, Brightness, Music, SFX, Dialogue and Mouse Sensitivity read their sliders;
	// Quality Text reads QualitySetting and GetText_0 Difficulty Setting.
	auto SliderValue = [](const USlider* Slider)
	{
		return UWasamiSettingsSaveGame::SliderText(Slider ? Slider->GetValue() : 0.f);
	};
	SetOptionsText(QualityText, UWasamiSettingsSaveGame::QualityText(QualitySetting));
	SetOptionsText(ResolutionScaleText, SliderValue(ResolutionScaleSlider));
	SetOptionsText(BrightnessText, SliderValue(BrightnessSlider));
	SetOptionsText(MusicText, SliderValue(MusicSlider));
	SetOptionsText(SFXText, SliderValue(SFXSlider));
	SetOptionsText(DialogueText, SliderValue(DialogueSlider));
	SetOptionsText(MouseSensitivityText, SliderValue(MouseSensitivitySlider));
	SetOptionsText(DifficultyText, UWasamiSettingsSaveGame::DifficultyText(DifficultySetting));
}

float UWasamiOptionsWidget::EvaluateFadeInOpacity(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(OptionsFadeInOpacityKeys);
	return Eval(Curve, Seconds, FadeInLength);
}

float UWasamiOptionsWidget::EvaluateFadeInScale(float Seconds)
{
	static const FRichCurve Curve = MakeCurve(OptionsFadeInScaleKeys);
	return Eval(Curve, Seconds, FadeInLength);
}
