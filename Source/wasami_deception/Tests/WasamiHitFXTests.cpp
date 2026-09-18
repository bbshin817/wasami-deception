#include "Misc/AutomationTest.h"
#include "../WasamiHitFX.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiHitFXTest, "Wasami.HitFX.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiHitFXTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiHitFX* Fx = World->SpawnActor<AWasamiHitFX>(AWasamiHitFX::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("the hit effect"), Fx))
	{
		return false;
	}
	const TWeakObjectPtr<AWasamiHitFX> FxRef(Fx);

	// BP_HitFX's post process: the whole view, red and fringed.
	const UPostProcessComponent* PostProcess = Fx->GetPostProcess();
	const FPostProcessSettings& Settings = PostProcess->Settings;
	TestTrue(TEXT("unbound"), PostProcess->bUnbound);
	TestTrue(TEXT("its colour gain"), Settings.bOverride_ColorGain
		&& Settings.ColorGain.Equals(FVector4(1., 0.21828, 0.180477, 1.), 1e-6));
	TestTrue(TEXT("its fringe"), Settings.bOverride_SceneFringeIntensity && Settings.SceneFringeIntensity == 10.f);
	TestTrue(TEXT("the fringe's start overridden"), Settings.bOverride_ChromaticAberrationStartOffset);
	TestEqual(TEXT("shake scale"), Fx->ShakeScale, 2.f);
	TestEqual(TEXT("play rate"), Fx->PlayRate, 1.f);

	// Played from the start as it begins: full at once, fading out by 0.35 s, gone at the timeline's end (5 s).
	TestEqual(TEXT("full at once"), PostProcess->BlendWeight, 1.f);
	Wrapper.TickTestWorld(0.f);
	Wrapper.TickTestWorld(0.1f);
	const float Early = PostProcess->BlendWeight;
	TestTrue(TEXT("fading at 0.1 s"), Early > 0.f && Early < 1.f);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		Wrapper.TickTestWorld(0.1f);
	}
	TestEqual(TEXT("clear by 0.4 s"), PostProcess->BlendWeight, 0.f);
	TestTrue(TEXT("still there"), FxRef.IsValid() && !FxRef->IsActorBeingDestroyed());
	for (float Left = AWasamiHitFX::TimelineLength - 0.4f + 0.1f; Left > 0.f; Left -= 0.1f)
	{
		Wrapper.TickTestWorld(0.1f);
	}
	TestTrue(TEXT("gone at 5 s"), !FxRef.IsValid() || FxRef->IsActorBeingDestroyed());
	return true;
}

#endif
