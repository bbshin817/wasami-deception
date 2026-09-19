#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CheckBox.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "../WasamiGameMode.h"
#include "../WasamiOptionsWidget.h"
#include "../WasamiSettingsSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** A screen whose tree is built and whose Construct ran with InSettings in InLevelName. */
	UWasamiOptionsWidget* MakeOptionsScreen(UWasamiSettingsSaveGame* InSettings, const TCHAR* InLevelName)
	{
		UWasamiOptionsWidget* Screen = NewObject<UWasamiOptionsWidget>();
		Screen->Settings = InSettings;
		Screen->LevelName = InLevelName;
		Screen->Initialize();
		Screen->TakeWidget();
		return Screen;
	}

	FString OptionsTextOf(const UWidgetTree* Tree, const TCHAR* Name)
	{
		const UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(Name));
		return Text ? Text->GetText().ToString() : FString(TEXT("<none>"));
	}

	float OptionsSliderOf(const UWidgetTree* Tree, const TCHAR* Name)
	{
		const USlider* Slider = Cast<USlider>(Tree->FindWidget(Name));
		return Slider ? Slider->GetValue() : -1.f;
	}

	bool OptionsIsChecked(const UWidgetTree* Tree, const TCHAR* Name)
	{
		const UCheckBox* Check = Cast<UCheckBox>(Tree->FindWidget(Name));
		return Check && Check->IsChecked();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiOptionsScreenTest, "Wasami.Options.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiOptionsScreenTest::RunTest(const FString& Parameters)
{
	UWasamiOptionsWidget* Screen = MakeOptionsScreen(NewObject<UWasamiSettingsSaveGame>(), AWasamiGameMode::TitleLevelName);
	UWidgetTree* Tree = Screen->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}

	// The root drawn at 1.015, the wash under the frame's panel.
	const UCanvasPanel* Root = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!TestNotNull(TEXT("CanvasPanel_0 at the root"), Root))
	{
		return false;
	}
	TestEqual(TEXT("the root at 1.015"), Root->GetRenderTransform().Scale.X, 1.015, 1e-4);
	TestTrue(TEXT("Blur+Red, then CanvasPanel_2"), Root->GetChildrenCount() == 2 && Root->GetChildAt(0)->GetFName() == TEXT("Blur+Red")
		&& Root->GetChildAt(1)->GetFName() == TEXT("CanvasPanel_2"));
	for (const TCHAR* Name : {TEXT("BackgroundBlur_0"), TEXT("Image_1"), TEXT("Image_31"), TEXT("CanvasPanel_1"), TEXT("GridPanel_0"),
		TEXT("HorizontalBox_11"), TEXT("ApplyButton"), TEXT("CancelButton"), TEXT("HorizontalBox_1"), TEXT("HorizontalBox_4")})
	{
		TestNotNull(FString::Printf(TEXT("%s is there"), Name), Tree->FindWidget(Name));
	}

	// The grid's four boxes in the original's rows and columns.
	const UGridPanel* Grid = Cast<UGridPanel>(Tree->FindWidget(TEXT("GridPanel_0")));
	TestTrue(TEXT("four boxes on the title"), Grid && Grid->GetChildrenCount() == 4 && Screen->HasDifficulty());
	struct FGridCell
	{
		const TCHAR* Name;
		int32 Row, RowSpan, Column, ColumnSpan;
	};
	for (const FGridCell& Cell : {FGridCell{TEXT("GraphicsBox"), 1, 3, 0, 1}, FGridCell{TEXT("AudioBox"), 3, 1, 0, 1},
		FGridCell{TEXT("ControlsBox"), 2, 2, 1, 2}, FGridCell{TEXT("DifficultyBox"), 1, 3, 0, 1}})
	{
		const UWidget* Box = Tree->FindWidget(Cell.Name);
		const UGridSlot* Slot = Box ? Cast<UGridSlot>(Box->Slot) : nullptr;
		TestTrue(FString::Printf(TEXT("%s at row %d, column %d"), Cell.Name, Cell.Row, Cell.Column), Slot && Slot->GetRow() == Cell.Row
			&& Slot->GetRowSpan() == Cell.RowSpan && Slot->GetColumn() == Cell.Column && Slot->GetColumnSpan() == Cell.ColumnSpan);
	}
	const UWidget* Controls = Tree->FindWidget(TEXT("ControlsBox"));
	TestTrue(TEXT("CONTROLS moved down 254"), Controls && FMath::IsNearlyEqual(Controls->GetRenderTransform().Translation.Y, 254.));

	// The pictures.
	const TCHAR* const Pictures[][2] = {{TEXT("Image_31"), TEXT("options_window_frame")}};
	for (const auto& Each : Pictures)
	{
		const UImage* Image = Cast<UImage>(Tree->FindWidget(Each[0]));
		const UObject* Resource = Image ? Image->GetBrush().GetResourceObject() : nullptr;
		TestTrue(FString::Printf(TEXT("%s shows %s"), Each[0], Each[1]), Resource && Resource->GetName() == Each[1]);
	}
	const UBorder* ValueBox = Cast<UBorder>(Tree->FindWidget(TEXT("Border_0")));
	TestTrue(TEXT("the value boxes are selection_bar drawn as boxes"), ValueBox && ValueBox->Background.GetResourceObject()
		&& ValueBox->Background.GetResourceObject()->GetName() == TEXT("selection_bar") && ValueBox->Background.DrawAs == ESlateBrushDrawType::Box);
	const USlider* Music = Cast<USlider>(Tree->FindWidget(TEXT("MusicSlider")));
	const UObject* Thumb = Music ? Music->GetWidgetStyle().NormalThumbImage.GetResourceObject() : nullptr;
	TestTrue(TEXT("the sliders' thumb is slider_bar_tab"), Thumb && Thumb->GetName() == TEXT("slider_bar_tab"));
	TestTrue(TEXT("on a red bar 4 thick"), Music && Music->GetSliderBarColor().Equals(FLinearColor(1.f, 0.f, 0.f, 1.f))
		&& Music->GetWidgetStyle().BarThickness == 4.f);
	const UCheckBox* Subtitles = Cast<UCheckBox>(Tree->FindWidget(TEXT("SubtitlesCheck")));
	if (TestNotNull(TEXT("SUBTITLES' check box"), Subtitles))
	{
		const FCheckBoxStyle& Style = Subtitles->GetWidgetStyle();
		TestTrue(TEXT("checkbox_icon unchecked"), Style.UncheckedImage.GetResourceObject() && Style.UncheckedImage.GetResourceObject()->GetName() == TEXT("checkbox_icon"));
		TestTrue(TEXT("checkbox_icon_checked checked"), Style.CheckedImage.GetResourceObject()
			&& Style.CheckedImage.GetResourceObject()->GetName() == TEXT("checkbox_icon_checked"));
		TestEqual(TEXT("grey when hovered"), Style.CheckedHoveredImage.TintColor.GetSpecifiedColor().R, UWasamiOptionsWidget::CheckHoverGrey, 1e-5f);
		TestEqual(TEXT("white otherwise"), Style.CheckedImage.TintColor.GetSpecifiedColor().R, 1.f, 1e-5f);
	}
	// QUALITY's arrows restyled by Construct (images, no padding); DIFFICULTY's as the tree has them.
	const UButton* QualityLower = Cast<UButton>(Tree->FindWidget(TEXT("QualityLower")));
	TestTrue(TEXT("QUALITY's arrows drawn as images without padding"), QualityLower && QualityLower->GetStyle().Normal.DrawAs == ESlateBrushDrawType::Image
		&& QualityLower->GetStyle().NormalPadding == FMargin(0.f));
	TestTrue(TEXT("the left one turned round"), QualityLower && FMath::IsNearlyEqual(QualityLower->GetRenderTransformAngle(), 180.f));
	const UButton* DifficultyHigher = Cast<UButton>(Tree->FindWidget(TEXT("DifficultyHigher")));
	const UObject* Hovered = DifficultyHigher ? DifficultyHigher->GetStyle().Hovered.GetResourceObject() : nullptr;
	TestTrue(TEXT("DIFFICULTY's arrows as UE 4's button"), DifficultyHigher && DifficultyHigher->GetStyle().Normal.DrawAs == ESlateBrushDrawType::Box
		&& DifficultyHigher->GetStyle().NormalPadding == FMargin(2.f) && Hovered && Hovered->GetName() == TEXT("selection_bar_arrow_hover"));
	// The unseen RESOLUTION rows.
	const UWidget* Resolution = Tree->FindWidget(TEXT("HorizontalBox_1"));
	const UWidget* HiddenResolution = Tree->FindWidget(TEXT("HorizontalBox_4"));
	TestTrue(TEXT("RESOLUTION at opacity 0"), Resolution && Resolution->GetRenderOpacity() == 0.f);
	TestTrue(TEXT("the other hidden"), HiddenResolution && HiddenResolution->GetVisibility() == ESlateVisibility::Hidden);

	// Setup Values with the defaults, and the boxes' texts.
	TestEqual(TEXT("RESOLUTION SCALE at 1"), OptionsSliderOf(Tree, TEXT("ResolutionScaleSlider")), 1.f, 1e-5f);
	TestEqual(TEXT("MOUSE SENSITIVITY at 0.5"), OptionsSliderOf(Tree, TEXT("MouseSensitivitySlider")), 0.5f, 1e-5f);
	TestTrue(TEXT("SUBTITLES, HEAD BOBBING and MOUSE SMOOTHING checked"), OptionsIsChecked(Tree, TEXT("SubtitlesCheck"))
		&& OptionsIsChecked(Tree, TEXT("HeadBobbingCheck")) && OptionsIsChecked(Tree, TEXT("MouseSmoothingCheck")));
	TestTrue(TEXT("INVERTED Y AXIS and TOGGLE SPRINT not"), !OptionsIsChecked(Tree, TEXT("InvertedYCheck")) && !OptionsIsChecked(Tree, TEXT("ToggleSprintCheck")));
	TestEqual(TEXT("QUALITY reads HIGH"), OptionsTextOf(Tree, TEXT("TextBlock_4")), FString(TEXT("HIGH")));
	TestEqual(TEXT("MUSIC reads 1"), OptionsTextOf(Tree, TEXT("TextBlock_14")), FString(TEXT("1")));
	TestEqual(TEXT("MOUSE SENSITIVITY reads 0.5"), OptionsTextOf(Tree, TEXT("TextBlock_15")), FString(TEXT("0.5")));
	TestEqual(TEXT("DIFFICULTY reads NORMAL"), OptionsTextOf(Tree, TEXT("TextBlock_28")), FString(TEXT("NORMAL")));

	// The boxes read the sliders every frame, as the bindings do.
	if (USlider* Dialogue = Cast<USlider>(Tree->FindWidget(TEXT("DialogueSlider"))))
	{
		Dialogue->SetValue(2.f / 9.f);
	}
	Screen->Advance(1.f / 60.f);
	TestEqual(TEXT("DIALOGUE follows its slider"), OptionsTextOf(Tree, TEXT("TextBlock_20")), FString(TEXT("0.2")));

	// Other settings.
	UWasamiSettingsSaveGame* Changed = NewObject<UWasamiSettingsSaveGame>();
	Changed->Quality = 0;
	Changed->Music = 1.f / 3.f;
	Changed->bInvertedYAxis = true;
	Changed->bSubtitles = false;
	Changed->Difficulty = EWasamiDifficulty::Easy;
	UWasamiOptionsWidget* Other = MakeOptionsScreen(Changed, AWasamiGameMode::TitleLevelName);
	TestEqual(TEXT("QUALITY 0 reads LOW"), OptionsTextOf(Other->WidgetTree, TEXT("TextBlock_4")), FString(TEXT("LOW")));
	TestEqual(TEXT("and is the screen's"), Other->GetQualitySetting(), 0);
	TestEqual(TEXT("MUSIC 1/3 reads 0.3"), OptionsTextOf(Other->WidgetTree, TEXT("TextBlock_14")), FString(TEXT("0.3")));
	TestTrue(TEXT("INVERTED Y AXIS checked"), OptionsIsChecked(Other->WidgetTree, TEXT("InvertedYCheck")));
	TestFalse(TEXT("SUBTITLES not"), OptionsIsChecked(Other->WidgetTree, TEXT("SubtitlesCheck")));
	TestEqual(TEXT("EASY"), OptionsTextOf(Other->WidgetTree, TEXT("TextBlock_28")), FString(TEXT("EASY")));
	TestTrue(TEXT("the screen keeps the settings it read"), Other->Settings == Changed);

	// DIFFICULTY only on the title.
	UWasamiOptionsWidget* InZone = MakeOptionsScreen(NewObject<UWasamiSettingsSaveGame>(), AWasamiGameMode::Zone1LevelName);
	const UGridPanel* ZoneGrid = Cast<UGridPanel>(InZone->WidgetTree->FindWidget(TEXT("GridPanel_0")));
	TestFalse(TEXT("no DIFFICULTY in a zone"), InZone->HasDifficulty());
	TestTrue(TEXT("three boxes left"), ZoneGrid && ZoneGrid->GetChildrenCount() == 3);
	TestTrue(TEXT("the title keeps it"), UWasamiOptionsWidget::ShowsDifficulty(AWasamiGameMode::TitleLevelName));
	TestFalse(TEXT("Zone 2 does not"), UWasamiOptionsWidget::ShowsDifficulty(AWasamiGameMode::Zone2LevelName));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiOptionsFadeInTest, "Wasami.Options.FadeIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiOptionsFadeInTest::RunTest(const FString& Parameters)
{
	using W = UWasamiOptionsWidget;
	// The curves: in over 0.25 s, the scale past 1 on the way to 0.5 s.
	TestEqual(TEXT("nothing at first"), W::EvaluateFadeInOpacity(0.f), 0.f, 1e-5f);
	TestEqual(TEXT("in at 0.25 s"), W::EvaluateFadeInOpacity(0.25f), 1.f, 1e-5f);
	TestEqual(TEXT("and stays"), W::EvaluateFadeInOpacity(0.5f), 1.f, 1e-5f);
	TestEqual(TEXT("no size at first"), W::EvaluateFadeInScale(0.f), 0.f, 1e-5f);
	TestEqual(TEXT("full size at 0.25 s"), W::EvaluateFadeInScale(0.25f), 1.f, 1e-5f);
	TestTrue(TEXT("a little past it after"), W::EvaluateFadeInScale(0.35f) > 1.02f && W::EvaluateFadeInScale(0.35f) < 1.2f);
	TestEqual(TEXT("back to 1 at 0.5 s"), W::EvaluateFadeInScale(0.5f), 1.f, 1e-5f);
	TestEqual(TEXT("FadeIn is [0, 30001) ticks"), W::FadeInLength, 0.500017f, 1e-5f);
	TestEqual(TEXT("added at Z 10"), W::ViewportZOrder, 10);

	// Construct plays it from the start on the wash and the frame's panel.
	UWasamiOptionsWidget* Screen = MakeOptionsScreen(NewObject<UWasamiSettingsSaveGame>(), AWasamiGameMode::TitleLevelName);
	TestEqual(TEXT("the frame starts unseen"), Screen->GetBoxOpacity(), 0.f, 1e-5f);
	TestEqual(TEXT("at no size"), Screen->GetBoxScale(), 0.f, 1e-5f);
	TestEqual(TEXT("and the wash with it"), Screen->GetWashOpacity(), 0.f, 1e-5f);
	for (int32 Frame = 0; Frame < 15; ++Frame)
	{
		Screen->Advance(1.f / 60.f);
	}
	TestEqual(TEXT("in at 0.25 s"), Screen->GetBoxOpacity(), 1.f, 1e-3f);
	TestEqual(TEXT("the wash too"), Screen->GetWashOpacity(), 1.f, 1e-3f);
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Screen->Advance(1.f / 60.f);
	}
	TestEqual(TEXT("ends at full size"), Screen->GetBoxScale(), 1.f, 1e-4f);
	const UWidget* Box = Screen->WidgetTree->FindWidget(TEXT("CanvasPanel_2"));
	TestTrue(TEXT("on CanvasPanel_2"), Box && FMath::IsNearlyEqual(Box->GetRenderTransform().Scale.X, 1., 1e-4) && FMath::IsNearlyEqual(Box->GetRenderOpacity(), 1.f));
	return true;
}

#endif
