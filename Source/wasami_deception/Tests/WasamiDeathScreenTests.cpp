#include "Misc/AutomationTest.h"

#include "../WasamiDeathScreenWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Runs the screen at 60 frames a second until Seconds since Begin. */
	void RunUntil(UWasamiDeathScreenWidget* Screen, float Seconds)
	{
		while (Screen->GetElapsed() + 1.f / 120.f < Seconds)
		{
			Screen->Advance(1.f / 60.f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDeathScreenCurvesTest, "Wasami.DeathScreen.Curves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDeathScreenCurvesTest::RunTest(const FString& Parameters)
{
	using W = UWasamiDeathScreenWidget;
	// Fade In / Fade Out: the author's tangent keeps the black long, then lets it go fast.
	TestEqual(TEXT("Fade In starts black"), W::EvaluateFadeIn(0.f), 1.f, 1e-4f);
	TestTrue(TEXT("Fade In is still mostly black halfway"), W::EvaluateFadeIn(0.5f) > 0.7f);
	TestEqual(TEXT("Fade In ends clear"), W::EvaluateFadeIn(1.f), 0.f, 1e-4f);
	TestTrue(TEXT("Fade Out dips below 0 first"), W::EvaluateFadeOut(0.2f) < 0.f);
	TestEqual(TEXT("Fade Out ends black"), W::EvaluateFadeOut(1.f), 1.f, 1e-4f);

	// Shake.
	TestEqual(TEXT("the vignette flashes at 0.1 s"), W::EvaluateShakeVignetteAlpha(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("and is 0.3 at 0.25 s"), W::EvaluateShakeVignetteAlpha(0.25f), 0.3f, 1e-4f);
	TestEqual(TEXT("and gone at 1 s"), W::EvaluateShakeVignetteAlpha(1.f), 0.f, 1e-4f);
	TestEqual(TEXT("the vignette starts at 1.25"), W::EvaluateShakeVignetteScale(0.f), 1.25f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.1 s"), W::EvaluateShakeVignetteScale(0.1f), 1.1f, 1e-4f);
	TestTrue(TEXT("the screen jolts to (10, -10) at 0.15 s"), W::EvaluateShakeTranslation(0.15f).Equals(FVector2D(10.f, -10.f), 1e-3f));
	TestTrue(TEXT("and (-5, 5) at 0.2 s"), W::EvaluateShakeTranslation(0.2f).Equals(FVector2D(-5.f, 5.f), 1e-3f));
	TestTrue(TEXT("and back by 0.3 s"), W::EvaluateShakeTranslation(0.3f).Equals(FVector2D::ZeroVector, 1e-3f));
	TestEqual(TEXT("the lives are red at 0.1 s"), W::EvaluateShakeLifeTint(0.1f), 0.f, 1e-4f);
	TestEqual(TEXT("and white again at 0.3 s"), W::EvaluateShakeLifeTint(0.3f), 1.f, 1e-4f);

	// The screen moves on 0.5 s after the line or the 6 s, whichever is first, from the reveal at 0.5 s.
	TestEqual(TEXT("a 3 s line"), W::ProceedTime(3.f), 4.f, 1e-4f);
	TestEqual(TEXT("a line longer than 6 s"), W::ProceedTime(10.f), 7.f, 1e-4f);

	TestEqual(TEXT("Traps' lines"), W::VoiceLengths(W::TrapsLevel).Num(), 7);
	TestEqual(TEXT("Asylum's lines"), W::VoiceLengths(W::AsylumLevel).Num(), 4);
	TestEqual(TEXT("Zone 1's tips"), W::TipsFor(TEXT("L_Hospital_Zone1")).Num(), 3);
	TestEqual(TEXT("Zone 2's tips"), W::TipsFor(TEXT("L_Hospital_Zone2")).Num(), 4);
	const TArray<FText> Other = W::TipsFor(TEXT("Elsewhere"));
	TestTrue(TEXT("elsewhere, one"), Other.Num() == 1 && Other[0].ToString() == TEXT("Try not to die next time."));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDeathScreenLivesLeftTest, "Wasami.DeathScreen.LivesLeft",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDeathScreenLivesLeftTest::RunTest(const FString& Parameters)
{
	UWasamiDeathScreenWidget* Screen = NewObject<UWasamiDeathScreenWidget>();
	Screen->Begin(2, 3.f, true);
	RunUntil(Screen, 0.45f);
	TestEqual(TEXT("black until the reveal"), Screen->GetCoverAlpha(), 1.f, 1e-4f);
	TestEqual(TEXT("the whole row until then"), Screen->GetShownLives(), 6);

	RunUntil(Screen, 1.3f);
	TestTrue(TEXT("Fade In is over (1.5 times as fast)"), Screen->GetCoverAlpha() < 0.01f);
	TestEqual(TEXT("the row keeps the two lives and the one being lost"), Screen->GetShownLives(), 3);

	RunUntil(Screen, 1.65f);
	TestEqual(TEXT("Shake's Update Life takes the lost one"), Screen->GetShownLives(), 2);

	// A 3 s line: Fade Out from 4 s, the respawn 2 s later.
	RunUntil(Screen, 3.95f);
	TestTrue(TEXT("still clear before Fade Out"), Screen->GetCoverAlpha() < 0.01f);
	RunUntil(Screen, 5.05f);
	TestTrue(TEXT("black after Fade Out"), Screen->GetCoverAlpha() > 0.99f);
	RunUntil(Screen, 5.95f);
	TestFalse(TEXT("no respawn before 6 s"), Screen->HasRespawned());
	RunUntil(Screen, 6.05f);
	TestTrue(TEXT("the respawn at 6 s"), Screen->HasRespawned());
	TestFalse(TEXT("no game over"), Screen->IsGameOver());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiDeathScreenGameOverTest, "Wasami.DeathScreen.GameOver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiDeathScreenGameOverTest::RunTest(const FString& Parameters)
{
	UWasamiDeathScreenWidget* Screen = NewObject<UWasamiDeathScreenWidget>();
	Screen->Begin(0, 3.f, false);
	RunUntil(Screen, 0.7f);
	TestFalse(TEXT("the game over waits 0.25 s after the reveal"), Screen->IsGameOver());
	TestEqual(TEXT("the row hides at the reveal"), Screen->GetShownLives(), 0);
	RunUntil(Screen, 0.8f);
	TestTrue(TEXT("game over at 0.75 s"), Screen->IsGameOver());
	RunUntil(Screen, 2.7f);
	TestFalse(TEXT("no buttons before 2.75 s"), Screen->AreButtonsShown());
	RunUntil(Screen, 2.8f);
	TestTrue(TEXT("the buttons at 2.75 s"), Screen->AreButtonsShown());
	RunUntil(Screen, 12.f);
	TestFalse(TEXT("never a respawn"), Screen->HasRespawned());
	TestTrue(TEXT("the cover stays clear"), Screen->GetCoverAlpha() < 0.01f);

	// Five lives or more: nothing leaves the row and nothing shakes; the screen still moves on.
	UWasamiDeathScreenWidget* Many = NewObject<UWasamiDeathScreenWidget>();
	Many->Begin(5, 0.f, true);
	RunUntil(Many, 3.f);
	TestEqual(TEXT("the whole row with 5 lives"), Many->GetShownLives(), 6);
	RunUntil(Many, 3.05f);
	TestTrue(TEXT("a line of 0 s: the respawn at 3 s"), Many->HasRespawned());
	return true;
}

#endif
