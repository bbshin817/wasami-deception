#include "Misc/AutomationTest.h"
#include "../WasamiPowerTypes.h"
#include "../WasamiTabletWidget.h"
#include "../WasamiTeleportAim.h"
#include "../WasamiPrimalPower.h"
#include "../WasamiShard.h"
#include "../WasamiTelekinesisPower.h"
#include "../WasamiVanishPower.h"
#include "../WasamiVanishWidget.h"
#include "../WasamiTelepathyPower.h"
#include "../WasamiTelepathyTracker.h"
#include "WasamiTestEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/Light/ParticleModuleLight.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The one light module of a force field system (the 'sphere' emitter's, shared by its LOD levels): its
	 * brightness, or -1 where the system, the module or a second one of them says the asset is not the one known. */
	float SphereLightBrightness(UParticleSystem* System)
	{
		if (!System)
		{
			return -1.f;
		}
		UParticleModuleLight* Found = nullptr;
		for (const UParticleEmitter* Emitter : System->Emitters)
		{
			for (int32 Level = 0; Emitter && Level < Emitter->LODLevels.Num(); ++Level)
			{
				const UParticleLODLevel* LOD = Emitter->LODLevels[Level];
				for (int32 Index = 0; LOD && Index < LOD->Modules.Num(); ++Index)
				{
					UParticleModuleLight* Light = Cast<UParticleModuleLight>(LOD->Modules[Index]);
					if (Light && Light != Found)
					{
						if (Found)
						{
							return -1.f;  // more than one light module: not the system this checks
						}
						Found = Light;
					}
				}
			}
		}
		return Found ? Found->BrightnessOverLife.GetValue(0.f) : -1.f;
	}
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTeleportGatesTest, "Wasami.Powers.TeleportGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTeleportGatesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("a cube"), Cube))
	{
		return false;
	}
	// The engine's 100 cm cube, scaled, with a collision profile.
	auto Block = [World, Cube](const FVector& Centre, const FVector& Size, FName Profile)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionProfileName(Profile);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();
		Actor->SetActorLocation(Centre);
		Actor->SetActorScale3D(Size / 100.);
	};
	// The engine's profile of the doors' trigger boxes: world-dynamic, overlapping everything.
	const FName OverlapAllDynamic(TEXT("OverlapAllDynamic"));
	// The player's stand-in: a capsule of the Pawn profile, away from the lanes.
	AActor* Player = World->SpawnActor<AActor>();
	UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(Player);
	Capsule->InitCapsuleSize(42.f, 88.f);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	Player->SetRootComponent(Capsule);
	Capsule->RegisterComponent();
	Player->SetActorLocation(FVector(0., -5000., 90.));

	// A door (world-dynamic, blocking pawns) 10 cm thick with its face at x 495, behind an overlap-only box (the doors'
	// FrontEnter): the move stops with the capsule touching the door.
	Block(FVector(300., 0., 150.), FVector(50., 400., 300.), OverlapAllDynamic);
	Block(FVector(500., 0., 150.), FVector(10., 400., 300.), UCollisionProfile::BlockAllDynamic_ProfileName);
	const FVector Stopped = AWasamiTeleportAim::StopAtGates(Capsule, FVector(0., 0., 90.), FVector(1000., 0., 90.));
	TestEqual(TEXT("the move stops with the capsule (radius 42) touching the door"), Stopped.X, 453., 1.);
	TestEqual(TEXT("on the line of the move"), Stopped.Y, 0., 1e-3);
	TestEqual(TEXT("at its height"), Stopped.Z, 90., 1e-3);

	// Overlap-only boxes and world-static walls are no gates (the move's own sweep stops at the walls).
	Block(FVector(300., 2000., 150.), FVector(50., 400., 300.), OverlapAllDynamic);
	Block(FVector(600., 2000., 150.), FVector(10., 400., 300.), UCollisionProfile::BlockAll_ProfileName);
	const FVector Through(1000., 2000., 90.);
	TestEqual(TEXT("no gate on the way"), AWasamiTeleportAim::StopAtGates(Capsule, FVector(0., 2000., 90.), Through), Through);

	// An ambulance (world-dynamic, 600 x 400 x 335 cm): a move onto its roof goes through its body; a move past it
	// stops at its side.
	Block(FVector(1000., 4000., 167.5), FVector(600., 400., 335.), UCollisionProfile::BlockAllDynamic_ProfileName);
	const FVector Roof(1000., 4000., 460.);
	TestEqual(TEXT("onto the roof"), AWasamiTeleportAim::StopAtGates(Capsule, FVector(0., 4000., 90.), Roof), Roof);
	const FVector Past = AWasamiTeleportAim::StopAtGates(Capsule, FVector(0., 4000., 90.), FVector(1600., 4000., 90.));
	TestEqual(TEXT("past it the move stops at its side"), Past.X, 658., 1.);

	// A door the capsule stands in at the start does not hold it.
	Block(FVector(0., 6000., 150.), FVector(10., 400., 300.), UCollisionProfile::BlockAllDynamic_ProfileName);
	const FVector Out(1000., 6000., 90.);
	TestEqual(TEXT("out of a door it stands in"), AWasamiTeleportAim::StopAtGates(Capsule, FVector(0., 6000., 90.), Out), Out);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTelekinesisTimelineTest, "Wasami.Powers.TelekinesisTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTelekinesisTimelineTest::RunTest(const FString& Parameters)
{
	// BP_TelekinesisPower's float2 is Primal Fear's, key for key: the same values at the times of the research's table.
	struct FRow
	{
		float Time, Fade;
	};
	const FRow Rows[] = {
		{0.f, -0.000698f},
		{0.1f, 0.029344f},
		{0.2f, 0.132451f},
		{0.3f, 0.320608f},
		{0.5f, 1.f},
		{1.f, 1.f},
		{2.f, 1.f},
	};
	for (const FRow& Row : Rows)
	{
		TestEqual(FString::Printf(TEXT("float2 at %.1f s"), Row.Time), AWasamiTelekinesisPower::TelekinesisFadeCurve().Eval(Row.Time), Row.Fade, 1e-5f);
	}
	TestEqual(TEXT("the tint's weight at 0.2 s"), AWasamiPowerBurst::TintWeight(0.132451f), 0.867549f, 1e-6f);
	TestEqual(TEXT("the flash's weight at 0.2 s"), AWasamiPowerBurst::FlashWeight(0.132451f), 0.558497f, 1e-5f);
	TestEqual(TEXT("the flash is gone at 0.3 s"), AWasamiPowerBurst::FlashWeight(0.320608f), 0.f);
	TestEqual(TEXT("the tint is gone at 0.5 s"), AWasamiPowerBurst::TintWeight(1.f), 0.f);

	// The class's volumes, as the export sets them: the tint is blue, the flash has Primal's fringe.
	const AWasamiTelekinesisPower* Defaults = GetDefault<AWasamiTelekinesisPower>();
	const UPostProcessComponent* Tint = Defaults->GetTint();
	const UPostProcessComponent* Flash = Defaults->GetFlash();
	TestTrue(TEXT("both volumes are unbound"), Tint->bUnbound && Flash->bUnbound);
	TestTrue(TEXT("both start at weight 0"), Tint->BlendWeight == 0.f && Flash->BlendWeight == 0.f);
	TestTrue(TEXT("the tint washes out the colour"), Tint->Settings.bOverride_ColorSaturation && Tint->Settings.ColorSaturation == FVector4(0., 0., 0., 1.));
	TestTrue(TEXT("the tint's blue gain"), Tint->Settings.bOverride_ColorGain && Tint->Settings.ColorGain.Equals(FVector4(0., 0.421727, 1.61, 1.), 1e-6));
	TestFalse(TEXT("the flash has no gain of its own"), Flash->Settings.bOverride_ColorGain || Flash->Settings.bOverride_ColorSaturation);
	TestTrue(TEXT("the flash's midtones"), Flash->Settings.bOverride_ColorGainMidtones && Flash->Settings.ColorGainMidtones == FVector4(100., 100., 100., 1.));
	TestTrue(TEXT("the flash's fringe"), Flash->Settings.bOverride_SceneFringeIntensity && Flash->Settings.SceneFringeIntensity == 50.f);
	TestTrue(TEXT("the flash's gamma override at its default"), Flash->Settings.bOverride_ColorGamma && Flash->Settings.ColorGamma == FVector4(1., 1., 1., 1.));
	TestEqual(TEXT("the class's Range"), Defaults->Range, 1500.f);
	TestEqual(TEXT("the wave"), Defaults->WaveSound.ToSoftObjectPath().ToString(), FString(TEXT("/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04.Stun_Wave_Attack_New_04")));
	TestEqual(TEXT("the shake"), Defaults->ShakeClass.ToSoftObjectPath().ToString(), FString(TEXT("/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.01_Hotel_Lobby_ElevatorShakeStop_C")));
	TestEqual(TEXT("the force field"), Defaults->ForceFieldParticles.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/Wasami/Powers/P_WasamiForceField.P_WasamiForceField")));
	// What the power loads ahead (after import_dd_powers): the wave, the shake and the force field, none missing.
	TArray<TObjectPtr<UObject>> Loaded;
	AWasamiTelekinesisPower::LoadAssets(Loaded);
	TestEqual(TEXT("three assets are loaded ahead"), Loaded.Num(), 3);
	TestFalse(TEXT("none is missing"), Loaded.Contains(nullptr));
	TestTrue(TEXT("the force field is loaded"), Loaded.Num() == 3 && Cast<UParticleSystem>(Loaded[2]) != nullptr);
	// This game's copy weakens the sphere's light; the rebuilt original keeps the value the cook saved (dd_powers).
	TestEqual(TEXT("the copy's light"), SphereLightBrightness(Cast<UParticleSystem>(Loaded.Num() == 3 ? Loaded[2] : nullptr)), 1.75f);
	TestEqual(TEXT("the original's light"), SphereLightBrightness(LoadObject<UParticleSystem>(nullptr,
		TEXT("/Game/DD/ThirdParty/AdvancedMagicFX09/Particles/P_ky_forceField_Telekinesis.P_ky_forceField_Telekinesis"))), 5.f);
	TestEqual(TEXT("the force field's wait"), AWasamiTelekinesisPower::ForceFieldDelay, 0.2f);
	TestEqual(TEXT("the force field's scale"), AWasamiTelekinesisPower::ForceFieldScale, 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTelekinesisPullTest, "Wasami.Powers.TelekinesisPull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTelekinesisPullTest::RunTest(const FString& Parameters)
{
	// A world that plays, as the shard's own test uses: the shards' capsules are in it and Activate starts their pulls.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	const float Range = FWasamiPowerTuning::ForLevel(5).TelekinesisRange;
	TestEqual(TEXT("the top level's range"), Range, 3000.f);

	// Inside the range: near, near its edge (the capsule sits about 1 m up the shard), and 25 m above (no line of sight
	// is asked for).
	AWasamiShard* Near = World->SpawnActor<AWasamiShard>(FVector(1000., 0., 0.), FRotator::ZeroRotator);
	AWasamiShard* Edge = World->SpawnActor<AWasamiShard>(FVector(0., Range - 100., 0.), FRotator::ZeroRotator);
	AWasamiShard* Above = World->SpawnActor<AWasamiShard>(FVector(0., 0., 2500.), FRotator::ZeroRotator);
	// Out of it; in it with its capsule off (the original's bDisabled); and an enemy in it, which the pull is not for.
	AWasamiShard* Far = World->SpawnActor<AWasamiShard>(FVector(-(Range + 200.), 0., 0.), FRotator::ZeroRotator);
	AWasamiShard* Disabled = World->SpawnActor<AWasamiShard>(FVector(0., -1000., 0.), FRotator::ZeroRotator);
	Disabled->FindComponentByClass<UCapsuleComponent>()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AWasamiTestEnemy* Enemy = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(500., 500., 0.));

	const int32 Pulled = AWasamiTelekinesisPower::PullShards(World, FVector::ZeroVector, Range);
	TestEqual(TEXT("three shards are pulled"), Pulled, 3);
	TestTrue(TEXT("the near one is pulled"), Near->IsPulling());
	TestTrue(TEXT("the one near the edge is pulled"), Edge->IsPulling());
	TestTrue(TEXT("the one above is pulled"), Above->IsPulling());
	TestFalse(TEXT("the far one is left alone"), Far->IsPulling());
	TestFalse(TEXT("the one with its capsule off is left alone"), Disabled->IsPulling());
	TestEqual(TEXT("the enemy is left alone"), Enemy->SetStateCount, 0);
	TestEqual(TEXT("the enemy is not told of a vanish"), Enemy->PlayerVanishCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVanishTimelineTest, "Wasami.Powers.VanishTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVanishTimelineTest::RunTest(const FString& Parameters)
{
	// BP_VanishPower's float2 (Bezier over the exported tangents) and the volumes' weights: the flash is gone at 0.12 s,
	// the purple at 0.3 s.
	struct FRow
	{
		float Time, Fade, Tint, Flash;
	};
	const FRow Rows[] = {
		{0.f, 0.002593f, 0.997407f, 0.991355f},
		{0.03f, 0.040658f, 0.959342f, 0.864475f},
		{0.06f, 0.116705f, 0.883295f, 0.610983f},
		{0.09f, 0.221632f, 0.778368f, 0.261227f},
		{0.12f, 0.346334f, 0.653666f, 0.f},
		{0.21f, 0.748052f, 0.251948f, 0.f},
		{0.27f, 0.947832f, 0.052168f, 0.f},
		{0.5f, 1.f, 0.f, 0.f},
		{2.f, 1.f, 0.f, 0.f},
	};
	for (const FRow& Row : Rows)
	{
		const FString At = FString::Printf(TEXT(" at %.2f s"), Row.Time);
		const float Fade = AWasamiVanishPower::VanishFadeCurve().Eval(Row.Time);
		TestEqual(TEXT("float2") + At, Fade, Row.Fade, 1e-5f);
		TestEqual(TEXT("the tint's weight") + At, AWasamiPowerBurst::TintWeight(Fade), Row.Tint, 1e-5f);
		TestEqual(TEXT("the flash's weight") + At, AWasamiPowerBurst::FlashWeight(Fade), Row.Flash, 1e-4f);
	}

	// The class's volumes and puff, as the export sets them.
	const AWasamiVanishPower* Defaults = GetDefault<AWasamiVanishPower>();
	const UPostProcessComponent* Tint = Defaults->GetTint();
	const UPostProcessComponent* Flash = Defaults->GetFlash();
	TestTrue(TEXT("both volumes are unbound at weight 0"), Tint->bUnbound && Flash->bUnbound && Tint->BlendWeight == 0.f && Flash->BlendWeight == 0.f);
	TestTrue(TEXT("the tint washes out the colour"), Tint->Settings.bOverride_ColorSaturation && Tint->Settings.ColorSaturation == FVector4(0., 0., 0., 1.));
	TestTrue(TEXT("the tint's purple gain"), Tint->Settings.bOverride_ColorGain && Tint->Settings.ColorGain.Equals(FVector4(0.697667, 0., 1.61, 1.), 1e-6));
	TestTrue(TEXT("the flash's midtones"), Flash->Settings.bOverride_ColorGainMidtones && Flash->Settings.ColorGainMidtones == FVector4(100., 100., 100., 1.));
	TestTrue(TEXT("the flash's fringe is overridden to 0"), Flash->Settings.bOverride_SceneFringeIntensity && Flash->Settings.SceneFringeIntensity == 0.f);
	TestFalse(TEXT("no film grain override"), Flash->Settings.bOverride_FilmGrainIntensity);
	const UParticleSystemComponent* Puff = Defaults->GetParticleSystem();
	TestTrue(TEXT("the puff's place"), Puff->GetRelativeLocation().Equals(FVector(92.422882, -0.000427, -152.146667), 1e-4));
	TestTrue(TEXT("the puff activates itself"), Puff->bAutoActivate);
	TestFalse(TEXT("the puff's tick starts off"), Puff->PrimaryComponentTick.bStartWithTickEnabled);
	TestFalse(TEXT("the puff's asset is set"), Defaults->PuffParticles.IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVanishNotifyTest, "Wasami.Powers.VanishNotify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVanishNotifyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("WasamiVanishNotifyTest"));
	World->InitializeActorsForPlay(FURL());

	// Tagged enemies, near and far (no range is asked for), and one with the interface but without the tag.
	AWasamiTestEnemy* Near = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(500., 0., 0.));
	AWasamiTestEnemy* Far = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(100000., 0., 0.));
	AWasamiTestEnemy* Untagged = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(0., 500., 0.));
	Untagged->Tags.Reset();
	// The Enemy tag without the interface (the original's Zone 2 matron): not told.
	AActor* TagOnly = World->SpawnActor<AActor>();
	TagOnly->Tags.Add(TEXT("Enemy"));

	TestEqual(TEXT("two enemies are told"), AWasamiVanishPower::NotifyEnemies(World), 2);
	TestEqual(TEXT("the near one once"), Near->PlayerVanishCount, 1);
	TestEqual(TEXT("the far one once"), Far->PlayerVanishCount, 1);
	TestEqual(TEXT("an untagged one is not told"), Untagged->PlayerVanishCount, 0);
	TestEqual(TEXT("nothing else is sent"), Near->SetStateCount, 0);

	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiVanishWidgetTest, "Wasami.Powers.VanishWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiVanishWidgetTest::RunTest(const FString& Parameters)
{
	// The animation Vanish at 1 / 15 speed: in over 1.5 s, held (a little over 1 between the keys, as the tangents
	// give), out over the last 1.5 s of the 15.
	constexpr float Speed = FWasamiPowerTuning::VanishDuration;
	struct FRow
	{
		float Seconds, Opacity;
	};
	const FRow Rows[] = {
		{0.f, 0.f},
		{0.375f, 0.155957f},
		{0.75f, 0.499219f},
		{1.125f, 0.842871f},
		{1.5f, 1.f},
		{7.5f, 1.012311f},
		{13.5f, 1.f},
		{14.25f, 0.499242f},
		{14.625f, 0.155966f},
		{15.f, 0.f},
		{20.f, 0.f},
	};
	for (const FRow& Row : Rows)
	{
		TestEqual(FString::Printf(TEXT("the opacity %.3f s into a Vanish"), Row.Seconds),
			UWasamiVanishWidget::EvaluateOpacity(Row.Seconds / Speed), Row.Opacity, 1e-5f);
	}
	TestEqual(TEXT("the class's Speed"), GetDefault<UWasamiVanishWidget>()->Speed, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTelepathyTrackerTest, "Wasami.Powers.TelepathyTracker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTelepathyTrackerTest::RunTest(const FString& Parameters)
{
	// The fade in and out, the original's Appear and Disappear opacity (Bezier over flat tangents).
	struct FRow
	{
		float Seconds, Opacity;
	};
	const FRow AppearRows[] = {
		{0.f, 0.f},
		{0.0625f, 0.042966f},
		{0.125f, 0.156241f},
		{0.25f, 0.499975f},
		{0.375f, 0.843722f},
		{0.5f, 1.f},
		{0.75f, 1.f},
	};
	for (const FRow& Row : AppearRows)
	{
		TestEqual(FString::Printf(TEXT("the fade %.4f s into Appear"), Row.Seconds),
			AWasamiTelepathyTracker::EvaluateAppearOpacity(Row.Seconds), Row.Opacity, 1e-5f);
	}
	const FRow DisappearRows[] = {
		{0.f, 1.f},
		{0.075f, 0.84375f},
		{0.15f, 0.5f},
		{0.225f, 0.15625f},
		{0.3f, 0.f},
	};
	for (const FRow& Row : DisappearRows)
	{
		TestEqual(FString::Printf(TEXT("the fade %.3f s into Disappear"), Row.Seconds),
			AWasamiTelepathyTracker::EvaluateDisappearOpacity(Row.Seconds), Row.Opacity, 1e-5f);
	}

	// The fade goes into the stencil, 0 - 255.
	TestEqual(TEXT("no fade, stencil 0"), AWasamiTelepathyTracker::StencilForFade(0.f), 0);
	TestEqual(TEXT("half, 128"), AWasamiTelepathyTracker::StencilForFade(0.5f), 128);
	TestEqual(TEXT("whole, 255"), AWasamiTelepathyTracker::StencilForFade(1.f), 255);
	TestEqual(TEXT("clamped"), AWasamiTelepathyTracker::StencilForFade(1.5f), 255);

	// The tracker's unbound post-process component, which gets the material at BeginPlay.
	const AWasamiTelepathyTracker* Default = GetDefault<AWasamiTelepathyTracker>();
	TestTrue(TEXT("the smoke covers the whole view"), Default->GetPostProcess()->bUnbound);
	TestFalse(TEXT("the silhouette material is set"), Default->SilhouetteMaterial.IsNull());
	TestTrue(TEXT("the tracker ticks"), Default->PrimaryActorTick.bCanEverTick);
	TestEqual(TEXT("the telepathy's class Time"), GetDefault<AWasamiTelepathyPower>()->Time, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTelepathyTargetsTest, "Wasami.Powers.TelepathyTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTelepathyTargetsTest::RunTest(const FString& Parameters)
{
	// A game world with its own game instance and world context that has begun play and is ticked by hand, so that the
	// telepathy and its trackers run their BeginPlay, ticks, timers and destruction.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	auto TickFor = [&Wrapper](float Seconds)
	{
		constexpr float Step = 0.05f;
		for (float Done = 0.f; Done < Seconds - 1e-4f; Done += Step)
		{
			Wrapper.TickTestWorld(Step);
		}
	};
	auto Trackers = [World]()
	{
		TArray<AWasamiTelepathyTracker*> Out;
		for (TActorIterator<AWasamiTelepathyTracker> It(World); It; ++It)
		{
			Out.Add(*It);
		}
		return Out;
	};
	auto TrackerOn = [&Trackers](const AActor* Enemy) -> AWasamiTelepathyTracker*
	{
		for (AWasamiTelepathyTracker* Each : Trackers())
		{
			if (Each->Actor.Get() == Enemy)
			{
				return Each;
			}
		}
		return nullptr;
	};

	// Enemies near and very far (no range is asked for), one that answers No Telepathy, and the Enemy tag without the
	// interface (the original's Zone 2 matron).
	AWasamiTestEnemy* Near = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(500., 0., 0.));
	AWasamiTestEnemy* Far = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(200000., 0., 0.));
	// The near one gets a body (a skinned mesh without an asset) for the marker to draw.
	USkeletalMeshComponent* NearBody = NewObject<USkeletalMeshComponent>(Near);
	NearBody->SetupAttachment(Near->GetRootComponent());
	NearBody->RegisterComponent();
	AWasamiTestEnemy* Hidden = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(0., 500., 0.));
	Hidden->bNoTelepathy = true;
	AActor* TagOnly = World->SpawnActor<AActor>();
	TagOnly->Tags.Add(TEXT("Enemy"));

	// BeginPlay looks at once.
	const FTransform AtOrigin = FTransform::Identity;
	AWasamiTelepathyPower* Telepathy = World->SpawnActorDeferred<AWasamiTelepathyPower>(AWasamiTelepathyPower::StaticClass(), AtOrigin);
	Telepathy->Time = FWasamiPowerTuning::ForLevel(5).TelepathyDuration;
	Telepathy->FinishSpawning(AtOrigin);
	TArray<AWasamiTelepathyTracker*> Found = Trackers();
	TestEqual(TEXT("a tracker on each of the two enemies"), Found.Num(), 2);
	TestEqual(TEXT("both are remembered"), Telepathy->GetActorsWithTracker().Num(), 2);
	for (const AWasamiTelepathyTracker* Each : Found)
	{
		TestTrue(TEXT("on the near or the far enemy"), Each->Actor == Near || Each->Actor == Far);
		TestTrue(TEXT("spawned at its enemy"), Each->GetActorLocation().Equals(Each->Actor->GetActorLocation()));
		TestTrue(TEXT("following from its BeginPlay"), Each->IsFollowing());
		TestEqual(TEXT("fading in from nothing"), Each->GetFade(), 0.f);
	}
	TestNull(TEXT("none on the enemy that answers No Telepathy"), TrackerOn(Hidden));
	TestNull(TEXT("none on the tag without the interface"), TrackerOn(TagOnly));
	TestEqual(TEXT("a direct second look finds nothing new"), Telepathy->UpdateTargets(), 0);

	// A tracker follows its enemy and marks its body, fading in.
	AWasamiTelepathyTracker* OnNear = TrackerOn(Near);
	if (!TestNotNull(TEXT("the near enemy's tracker"), OnNear))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	Near->SetActorLocation(FVector(600., 700., 50.));
	TickFor(0.1f);
	TestTrue(TEXT("the tracker moved onto its enemy"), OnNear->GetActorLocation().Equals(FVector(600., 700., 50.)));
	TestEqual(TEXT("the near enemy's body is marked"), OnNear->GetMarkedMeshes().Num(), 1);
	TestTrue(TEXT("it renders into custom depth"), NearBody->bRenderCustomDepth);
	TestTrue(TEXT("part way in at 0.1 s"), NearBody->CustomDepthStencilValue > 0 && NearBody->CustomDepthStencilValue < 255);

	// An enemy that appears later is found by the next look, 0.8 s after the start.
	AWasamiTestEnemy* Late = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(-800., 0., 0.));
	TickFor(0.6f);
	TestNull(TEXT("not before 0.8 s"), TrackerOn(Late));
	TickFor(0.2f);
	TestNotNull(TEXT("found by the look at 0.8 s"), TrackerOn(Late));
	TestEqual(TEXT("the near body faded in by 0.5 s, the whole stencil"), NearBody->CustomDepthStencilValue, 255);

	// A tracker whose enemy is gone stops on its next tick and goes 0.5 s later.
	AWasamiTelepathyTracker* OnLate = TrackerOn(Late);
	Late->Destroy();
	TickFor(0.05f);
	TestTrue(TEXT("a tracker without its enemy stops"), OnLate && !OnLate->IsFollowing());
	TickFor(0.6f);
	TestFalse(TEXT("and is gone 0.5 s later"), IsValid(OnLate));
	TestEqual(TEXT("the other two remain"), Trackers().Num(), 2);

	// At 9 s: the telepathy goes, the trackers stop and play Disappear, and go 0.5 s later.
	TickFor(9.f - 1.55f - 0.1f);
	TestTrue(TEXT("the telepathy is still there before 9 s"), IsValid(Telepathy));
	TickFor(0.15f);
	TestFalse(TEXT("the telepathy is gone at 9 s"), IsValid(Telepathy));
	Found = Trackers();
	TestEqual(TEXT("two trackers are removing"), Found.Num(), 2);
	for (const AWasamiTelepathyTracker* Each : Found)
	{
		TestFalse(TEXT("no longer following"), Each->IsFollowing());
		TestTrue(TEXT("its marker fades out"), Each->IsFadingOut());
	}
	TickFor(0.6f);
	TestEqual(TEXT("the trackers are gone after 0.5 s"), Trackers().Num(), 0);
	TestFalse(TEXT("the body no longer renders into custom depth"), NearBody->bRenderCustomDepth);
	TestEqual(TEXT("its stencil back to 0"), NearBody->CustomDepthStencilValue, 0);

	// Another telepathy puts trackers on the same enemies again (its own list starts empty).
	AWasamiTelepathyPower* Again = World->SpawnActorDeferred<AWasamiTelepathyPower>(AWasamiTelepathyPower::StaticClass(), AtOrigin);
	Again->Time = 1.f;
	Again->FinishSpawning(AtOrigin);
	TestEqual(TEXT("two trackers again"), Trackers().Num(), 2);
	TickFor(1.7f);
	TestEqual(TEXT("all gone after the second one ends"), Trackers().Num(), 0);
	TestFalse(TEXT("the second telepathy is gone"), IsValid(Again));

	Wrapper.ForwardErrorMessages(this);
	return true;
}

#endif
