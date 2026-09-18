#include "Misc/AutomationTest.h"
#include "../WasamiTextPromptWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTextPromptTest, "Wasami.TextPrompt.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTextPromptTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	const FText Line = FText::FromString(TEXT("Collect all soul shards in this zone to break the barrier."));
	UWasamiTextPromptWidget* Prompt = UWasamiTextPromptWidget::Show(Wrapper.GetTestWorld(), Line);
	if (!TestNotNull(TEXT("the prompt"), Prompt))
	{
		return false;
	}
	TestTrue(TEXT("the text set"), Prompt->Text.EqualTo(Line));
	TestFalse(TEXT("not on a screen the test's world does not have"), Prompt->IsInViewport());
	Prompt->TakeWidget();
	UTextBlock* Block = Prompt->GetTextBlock();
	if (!TestNotNull(TEXT("TextBlock_95"), Block))
	{
		return false;
	}

	// UMG_TextPrompt's tree, and Construct's font size and text.
	TestTrue(TEXT("a canvas at the root"), Cast<UCanvasPanel>(Prompt->GetRootWidget()) != nullptr);
	TestTrue(TEXT("the line"), Block->GetText().EqualTo(Line));
	const FSlateFontInfo& Font = Block->GetFont();
	TestTrue(TEXT("helvetica-normal"), Font.FontObject && Font.FontObject->GetName() == TEXT("helvetica-normal_Font")
		&& Font.TypefaceFontName == TEXT("Default"));
	TestEqual(TEXT("at Font Size, 24"), Font.Size, 24.f);
	TestTrue(TEXT("its drop shadow"), Block->GetShadowOffset().Equals(FVector2D(3.6068990230560303, 4.328499794006348), 1e-4)
		&& Block->GetShadowColorAndOpacity().Equals(FLinearColor(0.f, 0.f, 0.f, 0.682f), 1e-3f));
	const UCanvasPanelSlot* PromptSlot = Cast<UCanvasPanelSlot>(Block->Slot);
	if (TestNotNull(TEXT("in the canvas"), PromptSlot))
	{
		const FAnchorData Layout = PromptSlot->GetLayout();
		TestTrue(TEXT("anchored at the bottom middle"), Layout.Anchors.Minimum.Equals(FVector2D(0.5f, 1.f))
			&& Layout.Anchors.Maximum.Equals(FVector2D(0.5f, 1.f)));
		TestTrue(TEXT("centred across, hanging from its top"), Layout.Alignment.Equals(FVector2D(0.5f, 0.f)));
		TestEqual(TEXT("217 px up"), Layout.Offsets.Top, -217.0810546875f);
		TestTrue(TEXT("at its own size"), PromptSlot->GetAutoSize());
	}

	// NewAnimation_1: in over 0.25 s from 98 px lower, held, and out over the last 0.5 s drifting 110 px down.
	using P = UWasamiTextPromptWidget;
	TestEqual(TEXT("clear at first"), P::EvaluateOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("98 px lower at first"), P::EvaluateTranslationY(0.f), 98.056f, 1e-3f);
	const float Rising = P::EvaluateOpacity(0.125f);
	TestTrue(TEXT("fading in"), Rising > 0.f && Rising < 1.f);
	TestEqual(TEXT("in place at 0.25 s"), P::EvaluateTranslationY(0.25f), 0.f, 1e-3f);
	TestEqual(TEXT("opaque at 0.25 s"), P::EvaluateOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("held at 2 s"), P::EvaluateOpacity(2.f), 1.f, 1e-4f);
	TestEqual(TEXT("still in place at 2 s"), P::EvaluateTranslationY(2.f), 0.f, 1e-3f);
	TestEqual(TEXT("opaque at 4.5 s"), P::EvaluateOpacity(4.5f), 1.f, 1e-4f);
	const float Falling = P::EvaluateOpacity(4.75f);
	TestTrue(TEXT("fading out"), Falling > 0.f && Falling < 1.f);
	TestEqual(TEXT("gone at 5 s"), P::EvaluateOpacity(5.f), 0.f, 1e-4f);
	TestEqual(TEXT("110 px lower at 5 s"), P::EvaluateTranslationY(5.f), 110.17f, 1e-3f);
	TestEqual(TEXT("the text block starts clear"), Block->GetColorAndOpacity().GetSpecifiedColor().A, 0.f, 1e-4f);

	// Played by its own tick; off the screen as it ends, a little after 5 s.
	float Played = 0.f;
	while (!Prompt->IsFinished() && Played < 6.f)
	{
		Prompt->Advance(1.f / 60.f);
		Played += 1.f / 60.f;
		if (FMath::IsNearlyEqual(Played, 1.f, 0.5f / 60.f))
		{
			TestEqual(TEXT("the text block opaque at 1 s"), Block->GetColorAndOpacity().GetSpecifiedColor().A, 1.f, 1e-4f);
			TestEqual(TEXT("and in place"), Block->GetRenderTransform().Translation.Y, 0., 1e-3);
		}
	}
	TestTrue(TEXT("finished"), Prompt->IsFinished());
	TestEqual(TEXT("as the animation ends"), Played, P::AnimationLength, 1.f / 60.f);
	return true;
}

#endif
