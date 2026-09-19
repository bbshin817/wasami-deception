#include "Misc/AutomationTest.h"

#include "../WasamiCutsceneWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCutsceneWidgetTreeTest, "Wasami.Cutscene.Widget.Tree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCutsceneWidgetTreeTest::RunTest(const FString& Parameters)
{
	using W = UWasamiCutsceneWidget;

	TestEqual(TEXT("added at Z 0"), W::ViewportZOrder, 0);
	TestEqual(TEXT("the bars' scalar"), W::BarsParameter, FName(TEXT("Cutscene Bars")));

	W* Screen = NewObject<W>();
	Screen->Initialize();
	Screen->TakeWidget();
	UWidgetTree* Tree = Screen->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}

	// CanvasPanel_0 with the black screen under the prompt's corner (the original's order).
	UCanvasPanel* Root = Cast<UCanvasPanel>(Tree->RootWidget);
	if (!TestNotNull(TEXT("CanvasPanel_0 is the root"), Root) || !TestEqual(TEXT("two under it"), Root->GetChildrenCount(), 2))
	{
		return false;
	}
	TestEqual(TEXT("child 0"), Root->GetChildAt(0)->GetFName(), FName(TEXT("Black")));
	TestEqual(TEXT("child 1"), Root->GetChildAt(1)->GetFName(), FName(TEXT("Overlay_0")));

	// Black: the whole screen, out of the mouse's way and clear until BlackTransition plays.
	const UImage* Black = Cast<UImage>(Tree->FindWidget(TEXT("Black")));
	if (TestNotNull(TEXT("Black"), Black))
	{
		TestTrue(TEXT("black"), Black->GetColorAndOpacity().Equals(FLinearColor::Black));
		TestEqual(TEXT("hit test invisible"), Black->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("clear"), Black->GetRenderOpacity(), 0.f);
		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Black->Slot);
		if (TestNotNull(TEXT("Black's canvas slot"), Slot))
		{
			const FAnchorData Layout = Slot->GetLayout();
			TestTrue(TEXT("anchored to the whole screen"), Layout.Anchors.Minimum.Equals(FVector2D::ZeroVector) && Layout.Anchors.Maximum.Equals(FVector2D(1.f, 1.f)));
			TestTrue(TEXT("no offsets"), Layout.Offsets.Left == 0.f && Layout.Offsets.Top == 0.f && Layout.Offsets.Right == 0.f && Layout.Offsets.Bottom == 0.f);
		}
	}

	// Overlay_0: its own size, 20 px in from the bottom right corner.
	const UOverlay* Corner = Cast<UOverlay>(Tree->FindWidget(TEXT("Overlay_0")));
	if (TestNotNull(TEXT("Overlay_0"), Corner))
	{
		const UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Corner->Slot);
		if (TestNotNull(TEXT("Overlay_0's canvas slot"), Slot))
		{
			const FAnchorData Layout = Slot->GetLayout();
			TestTrue(TEXT("anchored bottom right"), Layout.Anchors.Minimum.Equals(FVector2D(1.f, 1.f)) && Layout.Anchors.Maximum.Equals(FVector2D(1.f, 1.f)));
			TestEqual(TEXT("its position"), FVector2D(Layout.Offsets.Left, Layout.Offsets.Top), FVector2D(-20.f, -20.f));
			TestEqual(TEXT("its alignment"), FVector2D(Layout.Alignment), FVector2D(1.f, 1.f));
			TestTrue(TEXT("at its own size"), Slot->GetAutoSize());
		}
		TestEqual(TEXT("one in the corner"), Corner->GetChildrenCount(), 1);
	}

	// HorizontalBox_98: PRESS / P / TO SKIP, clear until a key press fades it in.
	const UHorizontalBox* Prompt = Cast<UHorizontalBox>(Tree->FindWidget(TEXT("HorizontalBox_98")));
	if (!TestNotNull(TEXT("HorizontalBox_98"), Prompt) || !TestEqual(TEXT("three lines"), Prompt->GetChildrenCount(), 3))
	{
		return false;
	}
	TestEqual(TEXT("the prompt clear"), Prompt->GetRenderOpacity(), 0.f);

	struct FLine
	{
		const TCHAR* Widget;
		const TCHAR* Text;
		bool bLight;
		ESlateSizeRule::Type Size;
	};
	const FLine Lines[] = {
		{TEXT("TextBlock_22"), TEXT("PRESS "), true, ESlateSizeRule::Fill},
		{TEXT("TextBlock_24"), TEXT("P"), false, ESlateSizeRule::Automatic},
		{TEXT("TextBlock_25"), TEXT(" TO SKIP"), true, ESlateSizeRule::Automatic},
	};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FLine& Each = Lines[Index];
		const UTextBlock* Text = Cast<UTextBlock>(Tree->FindWidget(Each.Widget));
		if (!TestNotNull(Each.Widget, Text))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s is child %d"), Each.Widget, Index), Prompt->GetChildAt(Index)->GetFName(), FName(Each.Widget));
		TestEqual(FString::Printf(TEXT("%s's text"), Each.Widget), Text->GetText().ToString(), FString(Each.Text));
		const FSlateFontInfo Font = Text->GetFont();
		if (Each.bLight)
		{
			TestEqual(FString::Printf(TEXT("%s's typeface"), Each.Widget), Font.TypefaceFontName, FName(TEXT("Light")));
			TestEqual(FString::Printf(TEXT("%s's size"), Each.Widget), Font.Size, 16.f);
		}
		else
		{
			// The key keeps the tree's default font, which is bigger than the words around it.
			TestTrue(FString::Printf(TEXT("%s is bigger"), Each.Widget), Font.Size > 16.f);
		}
		const UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Text->Slot);
		if (TestNotNull(FString::Printf(TEXT("%s's slot"), Each.Widget), Slot))
		{
			TestEqual(FString::Printf(TEXT("%s's size rule"), Each.Widget), static_cast<int32>(Slot->GetSize().SizeRule.GetValue()), static_cast<int32>(Each.Size));
			TestEqual(FString::Printf(TEXT("%s across"), Each.Widget), Slot->GetHorizontalAlignment(), HAlign_Center);
			TestEqual(FString::Printf(TEXT("%s down"), Each.Widget), Slot->GetVerticalAlignment(), VAlign_Center);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiCutsceneWidgetAnimationTest, "Wasami.Cutscene.Widget.Animation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiCutsceneWidgetAnimationTest::RunTest(const FString& Parameters)
{
	using W = UWasamiCutsceneWidget;

	TestEqual(TEXT("Start is 1.25 s"), W::StartLength, 75001.f / 60000.f);
	TestEqual(TEXT("End is 1 s"), W::EndLength, 60001.f / 60000.f);
	TestEqual(TEXT("Fade is 4 s"), W::FadeLength, 240002.f / 60000.f);
	TestEqual(TEXT("BlackTransition is 1 s"), W::BlackTransitionLength, 60001.f / 60000.f);
	TestEqual(TEXT("the sequence jumps at 0.15 s"), W::SkipAt, 9000.f / 60000.f);

	// Start: the bars down over a second and held; a scene that is not a smooth transition plays it from a second in,
	// where they are already down.
	TestEqual(TEXT("no bars at 0 s"), W::EvaluateStartBars(0.f), 0.f, 1e-4f);
	const float Sliding = W::EvaluateStartBars(0.5f);
	TestTrue(TEXT("coming in at 0.5 s"), Sliding > 0.f && Sliding < 1.f);
	TestEqual(TEXT("down at 1 s"), W::EvaluateStartBars(1.f), 1.f, 1e-4f);
	TestEqual(TEXT("down at 1.25 s"), W::EvaluateStartBars(W::StartLength), 1.f, 1e-4f);

	// End: the bars back up over a second.
	TestEqual(TEXT("down at 0 s"), W::EvaluateEndBars(0.f), 1.f, 1e-4f);
	TestEqual(TEXT("up at 1 s"), W::EvaluateEndBars(W::EndLength), 0.f, 1e-4f);

	// Fade: the prompt in over a second, held two, out over one.
	TestEqual(TEXT("prompt clear at 0 s"), W::EvaluateFade(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("prompt in at 1 s"), W::EvaluateFade(1.f), 1.f, 1e-4f);
	TestEqual(TEXT("prompt in at 3 s"), W::EvaluateFade(3.f), 1.f, 1e-4f);
	TestEqual(TEXT("prompt gone at 4 s"), W::EvaluateFade(W::FadeLength), 0.f, 1e-4f);

	// BlackTransition: black by 0.15 s (where the sequence jumps to its end), held to 0.25 s, gone by 0.5 s; the
	// prompt's corner goes with it.
	TestEqual(TEXT("clear at 0 s"), W::EvaluateBlackTransition(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("black at 0.15 s"), W::EvaluateBlackTransition(W::SkipAt), 1.f, 1e-4f);
	TestEqual(TEXT("black at 0.25 s"), W::EvaluateBlackTransition(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("clear again at 0.5 s"), W::EvaluateBlackTransition(0.5f), 0.f, 1e-4f);
	TestEqual(TEXT("clear at 1 s"), W::EvaluateBlackTransition(W::BlackTransitionLength), 0.f, 1e-4f);
	TestEqual(TEXT("the corner in at 0 s"), W::EvaluateBlackTransitionOverlay(0.f), 1.f, 1e-4f);
	TestEqual(TEXT("the corner gone at 0.15 s"), W::EvaluateBlackTransitionOverlay(W::SkipAt), 0.f, 1e-4f);

	// The screen's own clock. Nothing plays until Construct, so a key press does nothing: Any Key Press waits for
	// Initialized (Start's end).
	W* Screen = NewObject<W>();
	Screen->Initialize();
	Screen->TakeWidget();
	TestFalse(TEXT("not initialized yet"), Screen->IsInitialized());
	Screen->AnyKeyPress();
	Screen->Advance(0.5f);
	TestEqual(TEXT("the prompt stays clear"), Screen->GetPromptOpacity(), 0.f, 1e-4f);

	// The Skip Cutscene key: BlackTransition from the start, the corner gone with the black by 0.15 s and clear again
	// by 0.5 s.
	Screen->StartHold();
	TestEqual(TEXT("black clear as it starts"), Screen->GetBlackOpacity(), 0.f, 1e-4f);
	Screen->Advance(W::SkipAt);
	TestEqual(TEXT("black at 0.15 s"), Screen->GetBlackOpacity(), 1.f, 1e-4f);
	TestEqual(TEXT("the corner gone at 0.15 s"), Screen->GetOverlayOpacity(), 0.f, 1e-4f);
	Screen->Advance(0.5f - W::SkipAt);
	TestEqual(TEXT("clear again at 0.5 s"), Screen->GetBlackOpacity(), 0.f, 1e-4f);

	// Cutscene Over: End from the start, the bars up again by its end.
	Screen->CutsceneOver();
	TestEqual(TEXT("the bars down as End starts"), Screen->GetBars(), 1.f, 1e-4f);
	Screen->Advance(W::EndLength * 0.5f);
	const float Leaving = Screen->GetBars();
	TestTrue(TEXT("the bars going at half way"), Leaving > 0.f && Leaving < 1.f);
	Screen->Advance(W::EndLength);
	TestEqual(TEXT("the bars up at the end"), Screen->GetBars(), 0.f, 1e-4f);
	return true;
}

#endif
