#include "Misc/AutomationTest.h"

#include "../WasamiVignetteSidesWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVignetteSidesAnimTest, "Wasami.VignetteSides.Anim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVignetteSidesAnimTest::RunTest(const FString& Parameters)
{
	using W = UWasamiVignetteSidesWidget;

	// UMG_VignetteSides' Anim and Construct's Delay, and what BP_PowerOrb and BP_BonusShard put in.
	TestEqual(TEXT("Anim runs 90001 ticks"), W::AnimLength, 1.5000167f, 1e-5f);
	TestEqual(TEXT("off after 2 s"), W::RemoveDelay, 2.f);
	TestEqual(TEXT("added at Z 5"), W::ZOrder, 5);
	TestEqual(TEXT("the orb's orange"), W::StunnedColor, FLinearColor(1.f, 0.465393f, 0.f, 1.f));
	TestEqual(TEXT("the red shard's red"), W::RevealedColor, FLinearColor(1.f, 0.f, 0.016666f, 1.f));
	TestEqual(TEXT("ENEMIES STUNNED"), W::StunnedText().ToString(), FString(TEXT("ENEMIES STUNNED")));
	TestEqual(TEXT("ENEMIES REVEALED"), W::RevealedText().ToString(), FString(TEXT("ENEMIES REVEALED")));

	// The words slam in from 4 times their size and hold from 0.1 s to 0.7 s.
	TestEqual(TEXT("the words start at 4"), W::EvaluateTextScale(0.f), 4.f, 1e-4f);
	TestEqual(TEXT("their size at 0.1 s"), W::EvaluateTextScale(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.15 s"), W::EvaluateTextScale(0.15f), 1.1f, 1e-4f);
	TestEqual(TEXT("their size from 0.25 s"), W::EvaluateTextScale(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("and on"), W::EvaluateTextScale(1.2f), 1.f, 1e-4f);
	TestEqual(TEXT("unseen at 0"), W::EvaluateTextOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("seen at 0.1 s"), W::EvaluateTextOpacity(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("still at 0.7 s"), W::EvaluateTextOpacity(0.7f), 1.f, 1e-4f);
	TestEqual(TEXT("gone at the end"), W::EvaluateTextOpacity(1.5f), 0.f, 1e-4f);

	// The vignette: 1.4 down in a straight line to 1.25 at 0.35 s, held to 0.9 s, out to 2; it flashes at 0.15 s.
	TestEqual(TEXT("the vignette starts at 1.4"), W::EvaluateVignetteScale(0.f), 1.4f, 1e-4f);
	TestEqual(TEXT("halfway down at 0.175 s"), W::EvaluateVignetteScale(0.175f), 1.325f, 1e-4f);
	TestEqual(TEXT("1.25 at 0.35 s"), W::EvaluateVignetteScale(0.35f), 1.25f, 1e-4f);
	TestEqual(TEXT("1.25 at 0.9 s"), W::EvaluateVignetteScale(0.9f), 1.25f, 1e-4f);
	TestEqual(TEXT("2 at the end"), W::EvaluateVignetteScale(1.5f), 2.f, 1e-4f);
	TestEqual(TEXT("clear at 0"), W::EvaluateVignetteAlpha(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("full at 0.15 s"), W::EvaluateVignetteAlpha(0.15f), 1.f, 1e-4f);
	TestEqual(TEXT("half at 0.25 s"), W::EvaluateVignetteAlpha(0.25f), 0.5f, 1e-4f);
	TestEqual(TEXT("a quarter at 0.9 s"), W::EvaluateVignetteAlpha(0.9f), 0.25f, 1e-4f);
	TestEqual(TEXT("clear at the end"), W::EvaluateVignetteAlpha(1.5f), 0.f, 1e-4f);

	// The whole screen twists: 0.5° at 0.1 s, −2° at 0.15 s, straight at 0.25 s, and back to its own tilt after 0.47 s.
	TestEqual(TEXT("straight at 0"), W::EvaluateCanvasAngle(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("0.5° at 0.1 s"), W::EvaluateCanvasAngle(0.1f), 0.5f, 1e-4f);
	TestEqual(TEXT("−2° at 0.15 s"), W::EvaluateCanvasAngle(0.15f), -2.f, 1e-4f);
	TestEqual(TEXT("straight at 0.25 s"), W::EvaluateCanvasAngle(0.25f), 0.f, 1e-4f);
	TestEqual(TEXT("its own tilt after the section"), W::EvaluateCanvasAngle(0.5f), -0.15806866f, 1e-5f);
	TestEqual(TEXT("1.05 at 0.133 s"), W::EvaluateCanvasScale(8000.f / 60000.f), 1.05f, 1e-4f);
	TestEqual(TEXT("its size at 0.467 s"), W::EvaluateCanvasScale(28000.f / 60000.f), 1.f, 1e-4f);
	TestEqual(TEXT("and after"), W::EvaluateCanvasScale(1.f), 1.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVignetteSidesScreenTest, "Wasami.VignetteSides.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVignetteSidesScreenTest::RunTest(const FString& Parameters)
{
	using W = UWasamiVignetteSidesWidget;

	// The orb's: the tree, built as the widget is taken (its Construct runs then).
	W* Stunned = NewObject<W>();
	Stunned->Color = W::StunnedColor;
	Stunned->bText = true;
	Stunned->TextToDisplay = W::StunnedText();
	Stunned->Initialize();
	Stunned->TakeWidget();
	UWidgetTree* Tree = Stunned->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}
	for (const TCHAR* Name : {TEXT("CanvasPanel_0"), TEXT("Image_161"), TEXT("TextBlock_47")})
	{
		TestNotNull(FString::Printf(TEXT("%s is in the tree"), Name), Tree->FindWidget(Name));
	}
	UImage* Vignette = Stunned->GetVignette();
	UTextBlock* Text = Stunned->GetTextBlock();
	if (!TestNotNull(TEXT("Image_161"), Vignette) || !TestNotNull(TEXT("TextBlock_47"), Text))
	{
		return false;
	}
	if (const UObject* Texture = Vignette->GetBrush().GetResourceObject())
	{
		TestEqual(TEXT("the vignette is T_VignetteNew"), Texture->GetName(), FString(TEXT("T_VignetteNew")));
	}
	else
	{
		AddError(TEXT("T_VignetteNew is missing: run WasamiDDTools.import_dd_powers"));
	}
	TestEqual(TEXT("the words"), Text->GetText().ToString(), FString(TEXT("ENEMIES STUNNED")));
	TestTrue(TEXT("shown"), Text->GetVisibility() != ESlateVisibility::Hidden);
	TestEqual(TEXT("the whole widget tinted"), Stunned->GetColorAndOpacity(), W::StunnedColor);
	TestEqual(TEXT("the words unseen at the start"), Text->GetRenderOpacity(), 0.f, 1e-4f);
	TestEqual(TEXT("the vignette's orange kept, clear at the start"), Vignette->GetColorAndOpacity(),
		FLinearColor(1.f, 0.23077f, 0.f, 0.f));
	for (int32 Frame = 0; Frame < 9; ++Frame)
	{
		Stunned->Advance(1.f / 60.f);
	}
	TestEqual(TEXT("the vignette full at 0.15 s"), Vignette->GetColorAndOpacity().A, 1.f, 1e-3f);
	// The words' opacity bulges a little over 1 between its keys (the exported auto tangents), as in the original.
	TestEqual(TEXT("the words seen"), Text->GetRenderOpacity(), W::EvaluateTextOpacity(Stunned->GetElapsed()), 1e-4f);
	TestTrue(TEXT("fully"), Text->GetRenderOpacity() >= 1.f);
	for (int32 Frame = 9; Frame < 119; ++Frame)
	{
		Stunned->Advance(1.f / 60.f);
	}
	TestFalse(TEXT("up until 2 s"), Stunned->IsFinished());
	Stunned->Advance(2.f / 60.f);
	TestTrue(TEXT("off at 2 s"), Stunned->IsFinished());

	// Text? false hides the words.
	W* Bare = NewObject<W>();
	Bare->Color = W::RevealedColor;
	Bare->Initialize();
	Bare->TakeWidget();
	if (TestNotNull(TEXT("TextBlock_47 without Text?"), Bare->GetTextBlock()))
	{
		TestEqual(TEXT("hidden without Text?"), Bare->GetTextBlock()->GetVisibility(), ESlateVisibility::Hidden);
	}
	TestEqual(TEXT("tinted red"), Bare->GetColorAndOpacity(), W::RevealedColor);

	// No player, no screen (as in a test's world).
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	TestNull(TEXT("nothing without a player"), W::Show(Wrapper.GetTestWorld(), W::StunnedColor, true, W::StunnedText()));
	return true;
}

#endif
