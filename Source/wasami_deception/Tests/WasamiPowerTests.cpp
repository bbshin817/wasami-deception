#include "Misc/AutomationTest.h"
#include "../WasamiPowerTypes.h"
#include "../WasamiTabletWidget.h"
#include "../WasamiTeleportAim.h"
#include "../WasamiPrimalPower.h"
#include "WasamiTestEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTeleportDistanceTest, "Wasami.Powers.TeleportDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTeleportDistanceTest::RunTest(const FString& Parameters)
{
	// Lerp(250, Max Distance, Alpha): every aim starts at Alpha 0.6, which at level 5 (1500) is the class's own 1000.
	const float Max = FWasamiPowerTuning::ForLevel(5).TeleportDistance;
	TestEqual(TEXT("the first distance at level 5"), AWasamiTeleportAim::DistanceFor(0.6f, Max), 1000.f, 1e-3f);
	TestEqual(TEXT("the first distance without upgrades"), AWasamiTeleportAim::DistanceFor(0.6f, 1000.f), 700.f, 1e-3f);
	TestEqual(TEXT("Alpha 0 is 250 cm"), AWasamiTeleportAim::DistanceFor(0.f, Max), 250.f, 1e-3f);
	TestEqual(TEXT("Alpha 1 is the maximum"), AWasamiTeleportAim::DistanceFor(1.f, Max), Max, 1e-3f);
	// A wheel notch (±1) moves Alpha by a tenth, 125 cm at level 5, within 0 to 1.
	const float Up = AWasamiTeleportAim::StepAlpha(0.6f, 1.f);
	TestEqual(TEXT("a notch up"), Up, 0.7f, 1e-5f);
	TestEqual(TEXT("a notch up is 125 cm further"), AWasamiTeleportAim::DistanceFor(Up, Max), 1125.f, 1e-2f);
	TestEqual(TEXT("two notches down in a frame"), AWasamiTeleportAim::StepAlpha(0.6f, -2.f), 0.4f, 1e-5f);
	TestEqual(TEXT("clamped at 1"), AWasamiTeleportAim::StepAlpha(0.95f, 1.f), 1.f, 1e-5f);
	TestEqual(TEXT("clamped at 0"), AWasamiTeleportAim::StepAlpha(0.05f, -1.f), 0.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPrimalTimelineTest, "Wasami.Powers.PrimalTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPrimalTimelineTest::RunTest(const FString& Parameters)
{
	// BP_PrimalPower's tracks at the times of the research's table (Bezier over the exported tangents).
	struct FRow
	{
		float Time, Growth, Fade, Desaturation, Opacity;
	};
	const FRow Rows[] = {
		{0.f, 0.f, -0.000698f, 0.015488f, 1.f},
		{0.1f, 0.150556f, 0.029344f, 0.013753f, 0.975541f},
		{0.2f, 0.286645f, 0.132451f, 0.009453f, 0.945612f},
		{0.3f, 0.408825f, 0.320608f, 0.004881f, 0.910149f},
		{0.5f, 0.613689f, 1.f, 0.004152f, 0.822367f},
		{1.f, 0.921497f, 1.f, 0.159048f, 0.438256f},
		{1.2f, 0.968728f, 1.f, 0.576909f, 0.229825f},
		{1.4f, 0.982647f, 1.f, 0.994729f, 0.065877f},
		{2.f, 0.983488f, 1.f, 0.996395f, 0.002581f},
	};
	for (const FRow& Row : Rows)
	{
		const FString At = FString::Printf(TEXT(" at %.1f s"), Row.Time);
		TestEqual(TEXT("float") + At, AWasamiPrimalPower::GrowthCurve().Eval(Row.Time), Row.Growth, 1e-5f);
		TestEqual(TEXT("float2") + At, AWasamiPrimalPower::PrimalFadeCurve().Eval(Row.Time), Row.Fade, 1e-5f);
		TestEqual(TEXT("desaturation") + At, AWasamiPrimalPower::DesaturationCurve().Eval(Row.Time), Row.Desaturation, 1e-5f);
		TestEqual(TEXT("opacity") + At, AWasamiPrimalPower::OpacityCurve().Eval(Row.Time), Row.Opacity, 1e-5f);
	}

	// The volumes' weights: the tint starts a little over 1 (float2 dips below 0) and is gone at 0.5 s, the flash at 0.3.
	TestEqual(TEXT("the tint's weight at the start"), AWasamiPowerBurst::TintWeight(-0.000698f), 1.000698f, 1e-6f);
	TestEqual(TEXT("the flash's weight at the start"), AWasamiPowerBurst::FlashWeight(-0.000698f), 1.f);
	TestEqual(TEXT("the tint's weight at 0.2 s"), AWasamiPowerBurst::TintWeight(0.132451f), 0.867549f, 1e-6f);
	TestEqual(TEXT("the flash's weight at 0.2 s"), AWasamiPowerBurst::FlashWeight(0.132451f), 0.558497f, 1e-5f);
	TestEqual(TEXT("the flash is gone at 0.3 s"), AWasamiPowerBurst::FlashWeight(0.320608f), 0.f);
	TestEqual(TEXT("the tint is gone at 0.5 s"), AWasamiPowerBurst::TintWeight(1.f), 0.f);

	// The class's volumes and sphere, as the export sets them.
	const AWasamiPrimalPower* Defaults = GetDefault<AWasamiPrimalPower>();
	const UPostProcessComponent* Tint = Defaults->GetTint();
	const UPostProcessComponent* Flash = Defaults->GetFlash();
	TestTrue(TEXT("both volumes are unbound"), Tint->bUnbound && Flash->bUnbound);
	TestEqual(TEXT("the tint starts at weight 0"), Tint->BlendWeight, 0.f);
	TestTrue(TEXT("the tint washes out the colour"), Tint->Settings.bOverride_ColorSaturation && Tint->Settings.ColorSaturation == FVector4(0., 0., 0., 1.));
	TestTrue(TEXT("the tint's red gain"), Tint->Settings.bOverride_ColorGain && Tint->Settings.ColorGain.Equals(FVector4(1.61, 0.129563, 0., 1.), 1e-6));
	TestFalse(TEXT("the flash has no gain of its own"), Flash->Settings.bOverride_ColorGain || Flash->Settings.bOverride_ColorSaturation);
	TestTrue(TEXT("the flash's midtones"), Flash->Settings.bOverride_ColorGainMidtones && Flash->Settings.ColorGainMidtones == FVector4(100., 100., 100., 1.));
	TestTrue(TEXT("the flash's fringe"), Flash->Settings.bOverride_SceneFringeIntensity && Flash->Settings.SceneFringeIntensity == 50.f);
	TestTrue(TEXT("the flash's gamma override at its default"), Flash->Settings.bOverride_ColorGamma && Flash->Settings.ColorGamma == FVector4(1., 1., 1., 1.));
	TestTrue(TEXT("the sphere collides with nothing"), Defaults->GetSphere()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestEqual(TEXT("the class's Range"), Defaults->Range, 1500.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiPrimalStunTest, "Wasami.Powers.PrimalStun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiPrimalStunTest::RunTest(const FString& Parameters)
{
	// An actor drops Blueprint events (Set State) until its world has initialised its actors.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("WasamiPrimalStunTest"));
	World->InitializeActorsForPlay(FURL());
	const float Range = FWasamiPowerTuning::ForLevel(5).PrimalRange;

	// Inside the range: near, near its edge, and a floor above (no line of sight is asked for).
	AWasamiTestEnemy* Near = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(1000., 0., 0.));
	AWasamiTestEnemy* Edge = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(0., Range - 50., 0.));
	AWasamiTestEnemy* Above = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(0., 0., 3000.));
	// Out of it, and an enemy whose body is not a pawn.
	AWasamiTestEnemy* Far = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(-(Range + 100.), 0., 0.));
	AWasamiTestEnemy* NotPawn = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(500., 500., 0.));
	NotPawn->FindComponentByClass<UCapsuleComponent>()->SetCollisionObjectType(ECC_WorldDynamic);
	// A pawn body with the Enemy tag but without the interface (the original's Zone 2 matron): not stunned.
	AActor* TagOnly = World->SpawnActor<AActor>();
	UCapsuleComponent* TagOnlyBody = NewObject<UCapsuleComponent>(TagOnly);
	TagOnlyBody->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	TagOnly->SetRootComponent(TagOnlyBody);
	TagOnlyBody->RegisterComponent();
	TagOnly->SetActorLocation(FVector(-500., 0., 0.));
	TagOnly->Tags.Add(TEXT("Enemy"));

	const int32 Stunned = AWasamiPrimalPower::StunEnemies(World, FVector::ZeroVector, Range);
	TestEqual(TEXT("three enemies are stunned"), Stunned, 3);
	for (const AWasamiTestEnemy* Each : {Near, Edge, Above})
	{
		TestEqual(TEXT("one Set State"), Each->SetStateCount, 1);
		TestTrue(TEXT("the state is Stun"), Each->State == EWasamiEnemyState::Stun);
		TestFalse(TEXT("not by an orb"), Each->bLastByOrb);
	}
	TestEqual(TEXT("the far one is left alone"), Far->SetStateCount, 0);
	TestEqual(TEXT("a body that is not a pawn is left alone"), NotPawn->SetStateCount, 0);

	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return true;
}

#endif
