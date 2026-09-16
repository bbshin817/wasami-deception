#include "Misc/AutomationTest.h"
#include "../WasamiPowerTypes.h"
#include "../WasamiTabletWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPowerGaugeTest, "Wasami.Powers.Gauge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPowerGaugeTest::RunTest(const FString& Parameters)
{
	// A power with a duration: the first Set Delay empties the icon over it, the next fills it over the cooldown.
	FWasamiPowerGauge Gauge;
	Gauge.SetDelay(2.f, false);
	TestEqual(TEXT("the icon waits for the timeline's first tick"), Gauge.Percent, 1.f);
	Gauge.Tick(1.f);
	TestEqual(TEXT("halfway through the effect"), Gauge.Percent, 0.5f);
	Gauge.Tick(5.f);
	TestEqual(TEXT("empty at the effect's end"), Gauge.Percent, 0.f);
	TestFalse(TEXT("the timeline stops at its start"), Gauge.IsPlaying());
	Gauge.SetDelay(4.f, false);
	Gauge.Tick(1.f);
	TestEqual(TEXT("a quarter into the cooldown"), Gauge.Percent, 0.25f);
	Gauge.Stop();
	TestEqual(TEXT("Stop puts the icon back to 1"), Gauge.Percent, 1.f);
	TestFalse(TEXT("Stop stops the timeline"), Gauge.IsPlaying());
	Gauge.SetDelay(1.f, false);
	Gauge.Tick(0.25f);
	TestEqual(TEXT("the FlipFlop is back on emptying"), Gauge.Percent, 0.75f);

	// The teleport's: no FlipFlop, the icon's own value decides.
	FWasamiPowerGauge Teleport;
	Teleport.SetDelay(0.05f, true);
	Teleport.Tick(0.1f);
	TestEqual(TEXT("a full teleport icon empties"), Teleport.Percent, 0.f);
	Teleport.SetDelay(5.f, true);
	Teleport.Tick(2.5f);
	TestEqual(TEXT("an empty teleport icon fills"), Teleport.Percent, 0.5f);
	Teleport.Tick(0.5f);
	Teleport.SetDelay(5.f, true);
	Teleport.Tick(2.5f);
	TestEqual(TEXT("a teleport icon below 1 fills from the start"), Teleport.Percent, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPowerTuningTest, "Wasami.Powers.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPowerTuningTest::RunTest(const FString& Parameters)
{
	const FWasamiPowerTuning& Top = FWasamiPowerTuning::ForLevel(5);
	TestEqual(TEXT("boost speed at level 5"), Top.BoostSpeed, 950.f);
	TestEqual(TEXT("boost time at level 5"), Top.BoostDuration, 9.75f);
	TestEqual(TEXT("boost cooldown at level 5 (the code's, 1 s over the table)"), Top.BoostCooldown, 7.5f);
	TestEqual(TEXT("teleport distance at level 5"), Top.TeleportDistance, 1500.f);
	TestEqual(TEXT("primal cooldown at level 5"), Top.PrimalCooldown, 23.f);
	TestEqual(TEXT("vanish cooldown at level 5"), Top.VanishCooldown, 15.f);
	TestTrue(TEXT("levels past 5 read 5"), &FWasamiPowerTuning::ForLevel(9) == &Top);
	TestEqual(TEXT("telekinesis range without upgrades"), FWasamiPowerTuning::ForLevel(0).TelekinesisRange, 1750.f);
	TestEqual(TEXT("boost cooldown at level 1"), FWasamiPowerTuning::ForLevel(1).BoostCooldown, 9.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiSocketBounceTest, "Wasami.Powers.SocketBounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiSocketBounceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("starts at 1"), UWasamiTabletWidget::EvaluateSocketBounce(0.f), 1.f, 1e-4f);
	TestEqual(TEXT("1.25 at 0.05 s"), UWasamiTabletWidget::EvaluateSocketBounce(0.05f), 1.25f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.15 s"), UWasamiTabletWidget::EvaluateSocketBounce(0.15f), 1.1f, 1e-4f);
	// Hermite between the two middle keys with their tangents (0.6667 and -0.5556 per second).
	TestEqual(TEXT("between the middle keys"), UWasamiTabletWidget::EvaluateSocketBounce(0.1f), 1.19028f, 1e-3f);
	TestEqual(TEXT("back to 1 at 0.5 s"), UWasamiTabletWidget::EvaluateSocketBounce(0.5f), 1.f, 1e-4f);
	TestEqual(TEXT("stays at 1 after the end"), UWasamiTabletWidget::EvaluateSocketBounce(2.f), 1.f, 1e-4f);
	return true;
}

#endif
