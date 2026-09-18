#include "Misc/AutomationTest.h"
#include "../WasamiRingPieceWidget.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "WasamiTestListener.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Runs the screen at 60 frames a second until Seconds since Begin. */
	void RunRingScreenUntil(UWasamiRingPieceWidget* Screen, float Seconds)
	{
		while (!Screen->HasClosed() && Screen->GetElapsed() + 1.f / 120.f < Seconds)
		{
			Screen->Advance(1.f / 60.f);
		}
	}

	float RenderScale(const UWidget* Widget)
	{
		return static_cast<float>(Widget->GetRenderTransform().Scale.X);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiRingPieceCurvesTest, "Wasami.RingPiece.Curves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiRingPieceCurvesTest::RunTest(const FString& Parameters)
{
	using W = UWasamiRingPieceWidget;
	// NewAnimation_1: the piece from nothing to 4 by 0.25 s, 4.1 at 0.35 s, 4 at the end; turned in from 90 degrees
	// past -5; faded in by 0.25 s, as the blur and the black veil are.
	TestEqual(TEXT("the piece starts unseen"), W::EvaluatePieceScale(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("4 at 0.25 s"), W::EvaluatePieceScale(0.25f), 4.f, 1e-4f);
	TestEqual(TEXT("4.1 at 0.35 s"), W::EvaluatePieceScale(20999.f / 60000.f), 4.1f, 1e-4f);
	TestEqual(TEXT("4 at the end"), W::EvaluatePieceScale(W::AnimationLength), 4.f, 1e-4f);
	TestEqual(TEXT("turned 90 degrees at first"), W::EvaluatePieceAngle(0.f), 90.f, 1e-3f);
	TestEqual(TEXT("upright at 0.25 s"), W::EvaluatePieceAngle(0.25f), 0.f, 1e-3f);
	TestEqual(TEXT("-5 at 0.35 s"), W::EvaluatePieceAngle(20999.f / 60000.f), -5.f, 1e-3f);
	TestEqual(TEXT("upright at the end"), W::EvaluatePieceAngle(W::AnimationLength), 0.f, 1e-3f);
	TestEqual(TEXT("clear at first"), W::EvaluatePieceOpacity(0.f), 0.f, 1e-4f);
	const float Fading = W::EvaluatePieceOpacity(0.125f);
	TestTrue(TEXT("fading in"), Fading > 0.f && Fading < 1.f);
	TestEqual(TEXT("opaque at 0.25 s"), W::EvaluatePieceOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("no blur at first"), W::EvaluateBlurStrength(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("blur 5 at 0.25 s"), W::EvaluateBlurStrength(0.25f), 5.f, 1e-4f);
	TestEqual(TEXT("and after"), W::EvaluateBlurStrength(0.4f), 5.f, 1e-4f);
	TestEqual(TEXT("no veil at first"), W::EvaluateVeilAlpha(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("veil 0.8 at 0.25 s"), W::EvaluateVeilAlpha(0.25f), 0.8f, 1e-4f);
	// NewAnimation_2: CLOSE up to 1 by 0.1 s, 1.1 at 0.2 s, 1 from 0.3 s.
	TestEqual(TEXT("CLOSE starts unseen"), W::EvaluateCloseScale(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("1 at 0.1 s"), W::EvaluateCloseScale(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.2 s"), W::EvaluateCloseScale(0.2f), 1.1f, 1e-4f);
	TestEqual(TEXT("1 at 0.3 s"), W::EvaluateCloseScale(0.3f), 1.f, 1e-4f);
	TestEqual(TEXT("and at the end"), W::EvaluateCloseScale(W::AnimationLength), 1.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiRingPieceWidgetTest, "Wasami.RingPiece.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiRingPieceWidgetTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWasamiRingPieceWidget* Screen = UWasamiRingPieceWidget::Show(Wrapper.GetTestWorld());
	if (!TestNotNull(TEXT("the screen"), Screen))
	{
		return false;
	}
	TestFalse(TEXT("not on a screen the test's world does not have"), Screen->IsInViewport());
	// The tree (and Construct, which starts the screen).
	Screen->TakeWidget();
	const UCanvasPanel* Root = Cast<UCanvasPanel>(Screen->GetRootWidget());
	if (!TestNotNull(TEXT("a canvas at the root"), Root) || !TestEqual(TEXT("four slots"), Root->GetChildrenCount(), 4))
	{
		return false;
	}
	TestTrue(TEXT("the blur first"), Root->GetChildAt(0) == Screen->GetBlur());
	TestTrue(TEXT("then the black veil"), Root->GetChildAt(1) == Screen->GetVeil());
	TestTrue(TEXT("then the piece"), Root->GetChildAt(2) == Screen->GetRingPiece());
	TestTrue(TEXT("then CLOSE"), Root->GetChildAt(3) == Screen->GetCloseButton());
	const UImage* Piece = Screen->GetRingPiece();
	const UObject* PieceTexture = Piece->GetBrush().GetResourceObject();
	TestTrue(TEXT("T_RingPiece_1"), PieceTexture && PieceTexture->GetName() == TEXT("T_RingPiece_1"));
	TestTrue(TEXT("200 px"), Piece->GetBrush().ImageSize.Equals(FVector2D(200.f, 200.f)));
	if (const UCanvasPanelSlot* PieceSlot = Cast<UCanvasPanelSlot>(Piece->Slot))
	{
		const FAnchorData Layout = PieceSlot->GetLayout();
		TestTrue(TEXT("in the middle"), Layout.Anchors.Minimum.Equals(FVector2D(0.5f, 0.5f)) && Layout.Alignment.Equals(FVector2D(0.5f, 0.5f)));
		TestTrue(TEXT("at its own size"), PieceSlot->GetAutoSize());
	}
	const UButton* Close = Screen->GetCloseButton();
	if (const UCanvasPanelSlot* CloseSlot = Cast<UCanvasPanelSlot>(Close->Slot))
	{
		const FAnchorData Layout = CloseSlot->GetLayout();
		TestTrue(TEXT("CLOSE at the bottom middle"), Layout.Anchors.Minimum.Equals(FVector2D(0.5f, 1.f)) && Layout.Alignment.Equals(FVector2D(0.5f, 0.f)));
		TestEqual(TEXT("196 px up"), Layout.Offsets.Top, -195.9524383544922f);
	}
	const UTextBlock* Label = Cast<UTextBlock>(Close->GetChildAt(0));
	if (TestNotNull(TEXT("CLOSE's text"), Label))
	{
		TestEqual(TEXT("CLOSE"), Label->GetText().ToString(), FString(TEXT("CLOSE")));
		const FSlateFontInfo& Font = Label->GetFont();
		TestTrue(TEXT("helvetica-neue-bold 36"), Font.FontObject && Font.FontObject->GetName() == TEXT("helvetica-neue-bold_Font") && Font.Size == 36.f);
	}
	TestTrue(TEXT("CLOSE greyed"), Close->GetColorAndOpacity().Equals(FLinearColor(0.114f, 0.114f, 0.114f, 1.f), 1e-3f));

	// Construct: NewAnimation_1 from the start, CLOSE unseen until NewAnimation_2 0.2 s on.
	TestEqual(TEXT("begun"), Screen->GetElapsed(), 0.f);
	TestEqual(TEXT("the piece unseen at first"), RenderScale(Piece), 0.f, 1e-4f);
	TestEqual(TEXT("no veil at first"), Screen->GetVeil()->GetBrushColor().A, 0.f, 1e-4f);
	RunRingScreenUntil(Screen, 0.19f);
	TestEqual(TEXT("CLOSE unseen before 0.2 s"), RenderScale(Close), 0.f, 1e-4f);
	RunRingScreenUntil(Screen, 0.25f);
	TestEqual(TEXT("the piece at 4 by 0.25 s"), RenderScale(Piece), 4.f, 0.02f);
	TestEqual(TEXT("the veil at 0.8"), Screen->GetVeil()->GetBrushColor().A, 0.8f, 1e-3f);
	TestEqual(TEXT("the blur at 5"), Screen->GetBlur()->GetBlurStrength(), 5.f, 1e-3f);
	RunRingScreenUntil(Screen, 1.f);
	TestEqual(TEXT("CLOSE up"), RenderScale(Close), 1.f, 1e-4f);
	TestEqual(TEXT("the piece settled at 4"), RenderScale(Piece), 4.f, 1e-4f);
	TestEqual(TEXT("and upright"), Piece->GetRenderTransform().Angle, 0.f, 1e-3f);
	TestFalse(TEXT("open"), Screen->IsClosing());

	// CLOSE (a DoOnce): NewAnimation_2 back, 0.2 s on NewAnimation_1 back, 0.5 s on Close and off the screen.
	const TStrongObjectPtr<UWasamiTestListener> Listener(NewObject<UWasamiTestListener>());
	Screen->OnClose.AddDynamic(Listener.Get(), &UWasamiTestListener::Hear);
	Screen->PressClose();
	Screen->PressClose();
	TestTrue(TEXT("closing"), Screen->IsClosing());
	RunRingScreenUntil(Screen, 1.19f);
	TestEqual(TEXT("the piece still there before 0.2 s"), RenderScale(Piece), 4.f, 1e-4f);
	RunRingScreenUntil(Screen, 1.55f);
	TestEqual(TEXT("CLOSE gone back"), RenderScale(Close), 0.f, 1e-4f);
	const float Shrinking = RenderScale(Piece);
	TestTrue(TEXT("the piece going back"), Shrinking < 4.f);
	TestEqual(TEXT("no Close yet"), Listener->Count, 0);
	RunRingScreenUntil(Screen, 1.75f);
	TestTrue(TEXT("closed 0.7 s after the click"), Screen->HasClosed());
	TestEqual(TEXT("Close once"), Listener->Count, 1);
	TestEqual(TEXT("the piece gone"), RenderScale(Piece), 0.f, 1e-4f);
	TestEqual(TEXT("the veil gone"), Screen->GetVeil()->GetBrushColor().A, 0.f, 1e-4f);
	Screen->PressClose();
	Screen->Advance(1.f);
	TestEqual(TEXT("still once"), Listener->Count, 1);
	return true;
}

#endif
