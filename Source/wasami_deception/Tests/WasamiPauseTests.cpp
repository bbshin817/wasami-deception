#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "../WasamiPauseWidget.h"
#include "../WasamiSettingsSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** A menu whose tree is built and whose Construct ran (without a world) with the difficulty given. */
	UWasamiPauseWidget* MakePauseMenu(EWasamiDifficulty Difficulty = EWasamiDifficulty::Normal)
	{
		UWasamiSettingsSaveGame* Settings = NewObject<UWasamiSettingsSaveGame>();
		Settings->Difficulty = Difficulty;
		UWasamiPauseWidget* Menu = NewObject<UWasamiPauseWidget>();
		Menu->Settings = Settings;
		Menu->Initialize();
		Menu->TakeWidget();
		return Menu;
	}

	void RunPause(UWasamiPauseWidget* Menu, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - 1e-4f; Time += 1.f / 60.f)
		{
			Menu->Advance(1.f / 60.f);
		}
	}

	FString PauseTextOf(const UWidgetTree* Tree, const TCHAR* Name)
	{
		const UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(Name));
		return Text ? Text->GetText().ToString() : FString(TEXT("<none>"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPauseScreenTest, "Wasami.Pause.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPauseScreenTest::RunTest(const FString& Parameters)
{
	UWasamiPauseWidget* Menu = MakePauseMenu();
	UWidgetTree* Tree = Menu->WidgetTree;
	const UCanvasPanel* Root = Tree ? Cast<UCanvasPanel>(Tree->RootWidget) : nullptr;
	if (!TestNotNull(TEXT("CanvasPanel_0 at the root"), Root))
	{
		return false;
	}

	// The root's children in the original's order: the wash, the stroke, the head, EASY MODE, the menu, the second wash
	// and the two windows.
	const TCHAR* Order[] = {TEXT("Blur+Red"), TEXT("Image_152"), TEXT("Icon"), TEXT("TextBlock_1"), TEXT("VerticalBox_113"),
		TEXT("CanvasPanel_3"), TEXT("Givingupbox"), TEXT("RestartBox")};
	if (TestEqual(TEXT("eight children"), Root->GetChildrenCount(), 8))
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			TestTrue(FString::Printf(TEXT("child %d is %s"), Index, Order[Index]), Root->GetChildAt(Index)->GetFName() == FName(Order[Index]));
		}
	}

	// The menu: RESUME, RESTART, OPTIONS and QUIT, top at the middle, none focusable, in the unhovered grey at 36.
	const UVerticalBox* List = Cast<UVerticalBox>(Tree->FindWidget(TEXT("VerticalBox_113")));
	const TCHAR* Items[] = {TEXT("RESUME"), TEXT("RESTART"), TEXT("OPTIONS"), TEXT("QUIT")};
	if (TestTrue(TEXT("four items"), List && List->GetChildrenCount() == 4))
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const UButton* Button = Cast<UButton>(List->GetChildAt(Index));
			TestTrue(FString::Printf(TEXT("%s in place"), Items[Index]), Button && Button->GetFName() == FName(Items[Index]));
			if (Button)
			{
				TestFalse(FString::Printf(TEXT("%s takes no focus"), Items[Index]), Button->GetIsFocusable());
				TestEqual(FString::Printf(TEXT("%s's grey"), Items[Index]), Button->GetColorAndOpacity().R, UWasamiPauseWidget::UnhoveredGrey, 1e-5f);
				const UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0));
				TestTrue(FString::Printf(TEXT("%s's label at 36"), Items[Index]), Label && Label->GetText().ToString() == Items[Index]
					&& FMath::IsNearlyEqual(Label->GetFont().Size, 36.f));
			}
		}
	}
	const UCanvasPanelSlot* ListSlot = List ? Cast<UCanvasPanelSlot>(List->Slot) : nullptr;
	TestTrue(TEXT("the menu's top at the middle"), ListSlot && ListSlot->GetAnchors().Minimum == FVector2D(0.5f, 0.5f)
		&& ListSlot->GetAlignment() == FVector2D(0.5f, 0.f) && ListSlot->GetAutoSize());

	// The head: 900 square at the middle of the top, a fifth above it, in the originals' red.
	const UImage* Icon = Cast<UImage>(Tree->FindWidget(TEXT("Icon")));
	const UCanvasPanelSlot* IconSlot = Icon ? Cast<UCanvasPanelSlot>(Icon->Slot) : nullptr;
	TestTrue(TEXT("the head's slot"), IconSlot && IconSlot->GetAnchors().Minimum == FVector2D(0.5f, 0.f)
		&& IconSlot->GetAlignment() == FVector2D(0.5f, 0.2f) && IconSlot->GetAutoSize());
	TestTrue(TEXT("the head 900 square, red"), Icon && Icon->GetBrush().ImageSize == FVector2D(900.f, 900.f)
		&& Icon->GetBrush().TintColor.GetSpecifiedColor().Equals(UWasamiPauseWidget::HeadTint()));
	TestEqual(TEXT("the head's red"), UWasamiPauseWidget::HeadTint().ToFColorSRGB(), FColor(192, 0, 0));

	// The stroke: 912 wide, stretched down the screen.
	const UImage* Stroke = Cast<UImage>(Tree->FindWidget(TEXT("Image_152")));
	const UCanvasPanelSlot* StrokeSlot = Stroke ? Cast<UCanvasPanelSlot>(Stroke->Slot) : nullptr;
	TestTrue(TEXT("the stroke's slot"), StrokeSlot && StrokeSlot->GetAnchors().Maximum == FVector2D(0.5f, 1.f)
		&& FMath::IsNearlyEqual(StrokeSlot->GetOffsets().Left, -20.56032943725586f) && Stroke->GetBrush().ImageSize.X == 912.f);

	// The pop-ups' parts, hidden: the second wash clear and letting clicks through, the windows at scale 0 and clear.
	const UWidget* Veil = Tree->FindWidget(TEXT("CanvasPanel_3"));
	TestTrue(TEXT("the second wash clear"), Veil && Veil->GetRenderOpacity() == 0.f);
	const UWidget* RedBlock = Tree->FindWidget(TEXT("redblock"));
	TestTrue(TEXT("redblock lets clicks through"), RedBlock && RedBlock->GetVisibility() == ESlateVisibility::HitTestInvisible);
	for (const TCHAR* Name : {TEXT("Givingupbox"), TEXT("RestartBox")})
	{
		const UWidget* Window = Tree->FindWidget(Name);
		TestTrue(FString::Printf(TEXT("%s hidden"), Name), Window && Window->GetRenderOpacity() == 0.f
			&& Window->GetRenderTransform().Scale == FVector2D::ZeroVector);
	}
	for (const TCHAR* Name : {TEXT("QuitToTitleButton"), TEXT("QuitToDesktopButton"), TEXT("CancelButton"), TEXT("YesButton"), TEXT("NoButton")})
	{
		const UButton* Button = Cast<UButton>(Tree->FindWidget(Name));
		TestTrue(FString::Printf(TEXT("%s is there and focusable"), Name), Button && Button->GetIsFocusable());
	}
	TestEqual(TEXT("QUIT TO TITLE"), PauseTextOf(Tree, TEXT("TextBlock_4")), FString(TEXT("QUIT TO TITLE")));
	TestEqual(TEXT("QUIT TO DESKTOP"), PauseTextOf(Tree, TEXT("TextBlock_5")), FString(TEXT("QUIT TO DESKTOP")));
	TestEqual(TEXT("the quit text"), PauseTextOf(Tree, TEXT("quittext")), FString(TEXT("YOU WILL BE ABLE TO RESTART \r\nFROM LAST CHECKPOINT")));
	const UImage* Peek = Cast<UImage>(Tree->FindWidget(TEXT("quitwindowhead")));
	const UCanvasPanelSlot* PeekSlot = Peek ? Cast<UCanvasPanelSlot>(Peek->Slot) : nullptr;
	TestTrue(TEXT("RESTART?'s head lower"), PeekSlot && FMath::IsNearlyEqual(PeekSlot->GetOffsets().Top, 77.3114013671875f)
		&& Peek->GetVisibility() == ESlateVisibility::HitTestInvisible);

	// EASY MODE: clear on NORMAL, the dark red on EASY (the colour binding).
	const UTextBlock* Easy = Cast<UTextBlock>(Tree->FindWidget(TEXT("TextBlock_1")));
	TestTrue(TEXT("EASY MODE clear on NORMAL"), Easy && Easy->GetColorAndOpacity().GetSpecifiedColor().A == 0.f);
	UWasamiPauseWidget* EasyMenu = MakePauseMenu(EWasamiDifficulty::Easy);
	const UTextBlock* EasyText = Cast<UTextBlock>(EasyMenu->WidgetTree->FindWidget(TEXT("TextBlock_1")));
	TestTrue(TEXT("EASY MODE red on EASY"), EasyText
		&& EasyText->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor(0.5255f, 0.f, 0.f, 1.f)));
	TestTrue(TEXT("magenta on HARD"), UWasamiPauseWidget::EasyModeColor(EWasamiDifficulty::Hard).Equals(FLinearColor(1.f, 0.f, 1.f, 1.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPauseFadeInTest, "Wasami.Pause.FadeIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPauseFadeInTest::RunTest(const FString& Parameters)
{
	// FadeIn: 0 → 1 over 0.5 s, flat at both ends (half way at 0.25 s).
	TestEqual(TEXT("clear at 0"), UWasamiPauseWidget::EvaluateFadeIn(0.f), 0.f, 1e-5f);
	TestEqual(TEXT("half at 0.25 s"), UWasamiPauseWidget::EvaluateFadeIn(0.25f), 0.5f, 1e-3f);
	TestEqual(TEXT("whole at 0.5 s"), UWasamiPauseWidget::EvaluateFadeIn(0.5f), 1.f, 1e-5f);
	TestTrue(TEXT("slow to start"), UWasamiPauseWidget::EvaluateFadeIn(0.05f) < 0.05f);

	UWasamiPauseWidget* Menu = MakePauseMenu();
	TestEqual(TEXT("clear at Construct"), Menu->GetOpacity(), 0.f, 1e-5f);
	RunPause(Menu, 0.25f);
	TestEqual(TEXT("half in at 0.25 s"), Menu->GetOpacity(), 0.5f, 0.02f);
	RunPause(Menu, 0.5f);
	TestEqual(TEXT("in at 0.5 s and after"), Menu->GetOpacity(), 1.f, 1e-5f);
	TestEqual(TEXT("the root carries it"), Menu->WidgetTree->RootWidget->GetRenderOpacity(), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPauseResumeTest, "Wasami.Pause.Resume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPauseResumeTest::RunTest(const FString& Parameters)
{
	// RESUME: FadeIn backwards from its end, and the menu off 0.5 s on.
	UWasamiPauseWidget* Menu = MakePauseMenu();
	RunPause(Menu, 1.f);
	UButton* Resume = Cast<UButton>(Menu->WidgetTree->FindWidget(TEXT("RESUME")));
	if (!TestNotNull(TEXT("RESUME"), Resume))
	{
		return false;
	}
	Resume->OnClicked.Broadcast();
	TestTrue(TEXT("resuming"), Menu->IsResuming());
	TestEqual(TEXT("from the end"), Menu->GetFadeInTime(), UWasamiPauseWidget::FadeInLength, 1e-5f);
	RunPause(Menu, 0.25f);
	TestEqual(TEXT("half out at 0.25 s"), Menu->GetOpacity(), 0.5f, 0.02f);
	TestFalse(TEXT("still up"), Menu->IsFinished());

	// Pressed again: the fade starts over, the Delay does not.
	Menu->PressResume();
	TestEqual(TEXT("the fade from the end again"), Menu->GetOpacity(), 1.f, 1e-3f);
	RunPause(Menu, 0.2f);
	TestFalse(TEXT("not off before 0.5 s"), Menu->IsFinished());
	RunPause(Menu, 1.f / 20.f + 1.f / 60.f);
	TestTrue(TEXT("off at 0.5 s from the first press"), Menu->IsFinished());
	const float Left = Menu->GetOpacity();
	RunPause(Menu, 0.5f);
	TestEqual(TEXT("nothing moves after"), Menu->GetOpacity(), Left, 1e-6f);
	return true;
}

#endif
