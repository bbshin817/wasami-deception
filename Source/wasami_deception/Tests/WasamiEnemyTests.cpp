#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectIterator.h"
#include "../WasamiEnemy.h"
#include "../WasamiEnemy06Chase.h"
#include "../WasamiEnemyAnimInstance.h"
#include "../WasamiEnemySentry.h"
#include "../WasamiEnemyZone2.h"
#include "../WasamiGameMode.h"
#include "../WasamiLift.h"
#include "../WasamiPowerTypes.h"
#include "../WasamiPrimalPower.h"
#include "../WasamiTelepathyPower.h"
#include "../WasamiTelepathyTracker.h"
#include "../WasamiVanishPower.h"
#include "../WasamiViewcone.h"
#include "../WasamiVoice.h"
#include "WasamiTestListener.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The size the enemy draws its mesh at, which the clips' strides grow with. */
	constexpr float Grown = static_cast<float>(AWasamiEnemy::MeshScale);

	/** The imported clips' lengths (30 fps frames, implementation record 07). */
	TArray<float> ImportedLengths()
	{
		const int32 Frames[WasamiEnemyClip::Num] = {58, 57, 31, 20, 18, 46, 75, 117, 117, 64, 83, 106, 37, 16, 63, 72, 55, 53};
		TArray<float> Lengths;
		for (int32 Count : Frames)
		{
			Lengths.Add(Count / 30.f);
		}
		return Lengths;
	}

	float SampleWeightSum(const FWasamiEnemyAnimState& State)
	{
		TArray<FWasamiEnemyAnimSample> Samples;
		State.GetSamples(Samples);
		float Sum = 0.f;
		for (const FWasamiEnemyAnimSample& Sample : Samples)
		{
			Sum += Sample.Weight;
		}
		return Sum;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimBlendsTest, "Wasami.Enemy.Anim.Blends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimBlendsTest::RunTest(const FString& Parameters)
{
	FWasamiBoolBlend Blend;
	Blend.Update(true, 0.25f, 0.1f);
	TestEqual(TEXT("the first update snaps"), Blend.Weight, 1.f);
	Blend.Update(false, 0.25f, 0.125f);
	TestEqual(TEXT("half way out after half the blend time"), Blend.Weight, 0.5f);
	Blend.Update(true, 0.25f, 0.0625f);
	TestEqual(TEXT("turned back, it goes up at the same rate"), Blend.Weight, 0.75f);
	Blend.Update(true, 0.25f, 0.0625f);
	TestEqual(TEXT("and gets there in the time the weight had to go"), Blend.Weight, 1.f);
	Blend.Reset();
	Blend.Update(false, 0.25f, 0.01f);
	TestEqual(TEXT("after a reset it snaps again"), Blend.Weight, 0.f);

	FWasamiStateBlend States;
	States.Advance(0.1f);
	TestEqual(TEXT("the machine starts in A"), States.WeightB, 0.f);
	States.Enter(true, 0.5f, EAlphaBlendOption::Sinusoidal);
	States.Advance(0.25f);
	TestEqual(TEXT("a sinusoidal crossfade is half way at half its time"), States.WeightB, 0.5f, 1e-5f);
	States.Advance(0.125f);
	TestEqual(TEXT("and follows its curve"), States.WeightB, (FMath::Sin(0.75f * UE_PI - UE_HALF_PI) + 1.f) / 2.f, 1e-5f);
	States.Enter(false, 0.25f, EAlphaBlendOption::ExpOut);
	States.Advance(0.125f);
	const float From = 1.f - (FMath::Sin(0.75f * UE_PI - UE_HALF_PI) + 1.f) / 2.f;
	TestEqual(TEXT("turned back, A goes up from the weight it had"), 1.f - States.WeightB, FMath::Lerp(From, 1.f, 1.f - FMath::Pow(2.f, -5.f)), 1e-5f);
	States.Advance(0.125f);
	TestEqual(TEXT("and has it all at the end of its time"), States.WeightB, 0.f);
	States.Enter(false, 0.25f, EAlphaBlendOption::ExpOut);
	TestEqual(TEXT("entering the current state does nothing"), States.Elapsed, 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimLocomotionTest, "Wasami.Enemy.Anim.Locomotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimLocomotionTest::RunTest(const FString& Parameters)
{
	FWasamiEnemyAnimState State;
	State.Init(ImportedLengths());
	TArray<FWasamiEnemyAnimSample> Samples;
	State.GetSamples(Samples);
	TestEqual(TEXT("nothing to blend before the first update"), Samples.Num(), 0);

	FWasamiEnemyAnimInputs Inputs;
	Inputs.StunDuration = 17.f;
	State.Update(Inputs, 0.1f);
	TestEqual(TEXT("standing, it idles"), State.GetClipWeight(WasamiEnemyClip::Idle), 1.f);
	TestEqual(TEXT("the idle moves on"), State.GetClipTime(WasamiEnemyClip::Idle), 0.1f);

	// The patrol's 200 cm/s: Idle → Moving over 0.5 s (Sinusoidal), and a walk.
	Inputs.Speed = 200.f;
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("half way into moving"), State.GetClipWeight(WasamiEnemyClip::Walk), 0.5f, 1e-5f);
	TestEqual(TEXT("half the idle left"), State.GetClipWeight(WasamiEnemyClip::Idle), 0.5f, 1e-5f);
	const float WalkRate = 200.f / (133.f * Grown);
	TestEqual(TEXT("the walk's rate follows the speed and the grown stride"), State.GetClipRate(WasamiEnemyClip::Walk), WalkRate);
	TestEqual(TEXT("the walk started from 0"), State.GetClipTime(WasamiEnemyClip::Walk), 0.25f * WalkRate, 1e-5f);
	TestEqual(TEXT("the idle moves on while it blends out"), State.GetClipTime(WasamiEnemyClip::Idle), 0.35f);
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("walking"), State.GetClipWeight(WasamiEnemyClip::Walk), 1.f);
	TestEqual(TEXT("the idle stops once it has no weight"), State.GetClipTime(WasamiEnemyClip::Idle), 0.35f);

	// The chase's 430 cm/s: above 400 the run blends in over 0.25 s.
	Inputs.Speed = 430.f;
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("half walk"), State.GetClipWeight(WasamiEnemyClip::Walk), 0.5f);
	TestEqual(TEXT("half run"), State.GetClipWeight(WasamiEnemyClip::Run), 0.5f);
	const float ChaseRate = 430.f / (450.f * Grown);
	TestEqual(TEXT("the run's rate follows the speed"), State.GetClipRate(WasamiEnemyClip::Run), ChaseRate);
	TestEqual(TEXT("the run starts where it was left (0)"), State.GetClipTime(WasamiEnemyClip::Run), 0.125f * ChaseRate, 1e-5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("running"), State.GetClipWeight(WasamiEnemyClip::Run), 1.f);
	Inputs.Speed = 2000.f;
	State.Update(Inputs, 0.1f);
	TestEqual(TEXT("the run's rate is held at 1.8"), State.GetClipRate(WasamiEnemyClip::Run), 1.8f);
	TestEqual(TEXT("a loop wraps"), State.GetClipTime(WasamiEnemyClip::Run), FMath::Fmod(0.125f * ChaseRate * 2.f + 0.18f, 20.f / 30.f), 1e-5f);

	// Stopping: Moving → Idle over 0.25 s (ExpOut), and the idle starts over.
	Inputs.Speed = 0.f;
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("mostly idle after half the stop"), State.GetClipWeight(WasamiEnemyClip::Idle), 1.f - FMath::Pow(2.f, -5.f), 1e-5f);
	TestEqual(TEXT("the idle entered from nothing starts from 0"), State.GetClipTime(WasamiEnemyClip::Idle), 0.125f);
	TestEqual(TEXT("the weights add up to 1"), SampleWeightSum(State), 1.f, 1e-5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("idle"), State.GetClipWeight(WasamiEnemyClip::Idle), 1.f);

	// Exactly 5 cm/s changes nothing either way.
	Inputs.Speed = 5.f;
	State.Update(Inputs, 1.f);
	TestEqual(TEXT("5 cm/s does not start moving"), State.GetClipWeight(WasamiEnemyClip::Idle), 1.f);
	Inputs.Speed = 10.f;
	State.Update(Inputs, 1.f);
	Inputs.Speed = 5.f;
	State.Update(Inputs, 1.f);
	TestEqual(TEXT("nor stop it"), State.GetClipWeight(WasamiEnemyClip::Walk), 1.f);
	TestEqual(TEXT("a slow walk's rate is held at 0.5"), State.GetClipRate(WasamiEnemyClip::Walk), 0.5f);

	// The sentry's idle, and the Nightmare run.
	FWasamiEnemyAnimState Sentry;
	Sentry.Init(ImportedLengths());
	FWasamiEnemyAnimInputs SentryInputs;
	SentryInputs.bAggressiveIdle = true;
	Sentry.Update(SentryInputs, 0.1f);
	TestEqual(TEXT("the sentry idles alert from the start"), Sentry.GetClipWeight(WasamiEnemyClip::IdleAlert), 1.f);
	SentryInputs.bAggressiveIdle = false;
	Sentry.Update(SentryInputs, 0.5f);
	TestEqual(TEXT("the alert idle blends over 1 s"), Sentry.GetClipWeight(WasamiEnemyClip::IdleAlert), 0.5f);
	SentryInputs.Speed = 800.f;
	SentryInputs.bNightmare = true;
	Sentry.Update(SentryInputs, 0.5f);
	TestEqual(TEXT("the Nightmare run from the start of moving"), Sentry.GetClipWeight(WasamiEnemyClip::RunNightmare), 1.f);
	TestEqual(TEXT("the plain run is not used"), Sentry.GetClipWeight(WasamiEnemyClip::Run), 0.f);
	TestEqual(TEXT("the Nightmare run's rate"), Sentry.GetClipRate(WasamiEnemyClip::RunNightmare), 800.f / (500.f * Grown));
	SentryInputs.bNightmare = false;
	Sentry.Update(SentryInputs, 0.125f);
	TestEqual(TEXT("it switches like the run"), Sentry.GetClipWeight(WasamiEnemyClip::Run), 0.5f);
	TestEqual(TEXT("the weights add up to 1"), SampleWeightSum(Sentry), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimStunTest, "Wasami.Enemy.Anim.Stun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimStunTest::RunTest(const FString& Parameters)
{
	const TArray<float> Lengths = ImportedLengths();
	const float Fall = Lengths[WasamiEnemyClip::StunKnockDown];
	const float GetUp = Lengths[WasamiEnemyClip::StunGetUpKnockDown];
	TestEqual(TEXT("a fall's get-up"), WasamiEnemyAnim::GetUpAfter(WasamiEnemyClip::StunFlyUp), static_cast<int32>(WasamiEnemyClip::StunGetUpFlyUp));

	// The 17 s stun: the fall once, held on its back, and the get-up so that it ends with the stun.
	FWasamiStunPlayback Playback;
	Playback.Start(17.f, Fall, GetUp);
	TestEqual(TEXT("the get-up starts so that it ends with the stun"), Playback.GetUpStart, 17.f - GetUp, 1e-4f);
	TestFalse(TEXT("it starts falling"), Playback.IsGettingUp());
	TestEqual(TEXT("from the fall's start"), Playback.GetClipTime(), 0.f);
	TestFalse(TEXT("falling is not getting up"), Playback.Advance(1.f));
	TestEqual(TEXT("the fall plays"), Playback.GetClipTime(), 1.f);
	Playback.Advance(Fall);
	TestEqual(TEXT("and holds its end"), Playback.GetClipTime(), Fall);
	Playback.Advance(Playback.GetUpStart - 0.01f - Playback.Elapsed);
	TestFalse(TEXT("still lying just before"), Playback.IsGettingUp());
	TestTrue(TEXT("crossing into the get-up is told"), Playback.Advance(0.02f));
	TestEqual(TEXT("from the get-up's start"), Playback.GetClipTime(), 0.01f, 1e-3f);
	TestFalse(TEXT("once"), Playback.Advance(0.01f));
	Playback.Advance(17.f);
	TestEqual(TEXT("the get-up holds its end"), Playback.GetClipTime(), GetUp);

	FWasamiStunPlayback Short;
	Short.Start(3.f, Fall, GetUp);
	TestEqual(TEXT("a stun too short for both gets up as the fall ends"), Short.GetUpStart, Fall);
	Short.Advance(3.f);
	TestEqual(TEXT("and has not got up when it ends"), Short.GetClipTime(), 3.f - Fall, 1e-5f);

	// The fall is drawn at random from the two (the user's choice, 2026-09-18); a missing one gives way to the other.
	FWasamiEnemyAnimInputs Stunned;
	Stunned.bStunned = true;
	Stunned.StunDuration = 17.f;
	FWasamiEnemyAnimInputs Standing;
	auto CountFalls = [this, &Stunned, &Standing](TArrayView<const float> InLengths, int32 Seed, int32 (&OutCounts)[2])
	{
		FWasamiEnemyAnimState Draws;
		Draws.Init(InLengths, Seed);
		for (int32 Stun = 0; Stun < 40; ++Stun)
		{
			Draws.Update(Stunned, 0.1f);
			const int32 Index = Draws.StunFall - WasamiEnemyClip::StunFlyUp;
			if (TestTrue(TEXT("each stun falls"), Index == 0 || Index == 1))
			{
				++OutCounts[Index];
			}
			Draws.Update(Standing, 0.1f);
		}
	};
	int32 Both[2] = {};
	CountFalls(Lengths, 1234, Both);
	TestTrue(TEXT("both falls come"), Both[0] >= 10 && Both[1] >= 10);
	TArray<float> WithoutFlyUp = Lengths;
	WithoutFlyUp[WasamiEnemyClip::StunFlyUp] = 0.f;
	int32 KnockDownOnly[2] = {};
	CountFalls(WithoutFlyUp, 1234, KnockDownOnly);
	TestEqual(TEXT("without Stun_FlyUp, always Stun_KnockDown"), KnockDownOnly[1], 40);

	// In the state: running, then Primal Fear.
	FWasamiEnemyAnimState State;
	State.Init(Lengths, 1);
	FWasamiEnemyAnimInputs Inputs;
	Inputs.Speed = 800.f;
	Inputs.StunDuration = 17.f;
	State.Update(Inputs, 0.25f);
	State.Update(Inputs, 0.25f);
	TestTrue(TEXT("a chase variation plays"), State.PlayOnce(WasamiEnemyClip::ChaseSlide, 1.f, 0.25f, 0.25f));
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("over the run"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 1.f);

	Inputs.bStunned = true;
	Inputs.Speed = 0.f;
	State.Update(Inputs, 0.125f);
	const int32 FallClip = State.StunFall;
	const int32 GetUpClip = WasamiEnemyAnim::GetUpAfter(FallClip);
	if (!TestTrue(TEXT("a fall"), FallClip == WasamiEnemyClip::StunFlyUp || FallClip == WasamiEnemyClip::StunKnockDown))
	{
		return false;
	}
	TestFalse(TEXT("the stun stops what plays once"), State.IsPlayingOnce());
	TestEqual(TEXT("the stun blends in over 0.25 s"), State.Stun.Weight, 0.5f);
	TestEqual(TEXT("the fall starts from 0"), State.GetClipTime(FallClip), 0.125f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the variation blends out over 0.25 s"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.5f);
	TestEqual(TEXT("under it, the fall"), State.GetClipWeight(FallClip), 0.5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("stunned"), State.GetClipWeight(FallClip), 1.f);
	TestEqual(TEXT("the locomotion, under the stun, went to idle"), State.Moving.WeightB, 0.f);

	int32 Crossings = 0;
	for (int32 Step = 0; Step < 50; ++Step)
	{
		State.Update(Inputs, 0.25f);
		Crossings += State.StunGetUpStarted != INDEX_NONE ? 1 : 0;
	}
	TestEqual(TEXT("12.875 s in, still lying"), State.GetClipWeight(FallClip), 1.f);
	TestEqual(TEXT("at the fall's end"), State.GetClipTime(FallClip), Lengths[FallClip]);
	TestEqual(TEXT("without getting up"), Crossings, 0);
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("13.125 s in, it gets up"), State.GetClipWeight(GetUpClip), 1.f);
	TestEqual(TEXT("from the get-up's start"), State.GetClipTime(GetUpClip), 13.125f - (17.f - Lengths[GetUpClip]), 1e-4f);
	TestEqual(TEXT("the update says so, for the owner's move"), State.StunGetUpStarted, FallClip);
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("the next does not"), State.StunGetUpStarted, static_cast<int32>(INDEX_NONE));
	for (int32 Step = 0; Step < 15; ++Step)
	{
		State.Update(Inputs, 0.25f);
	}
	TestEqual(TEXT("17.125 s in, the get-up holds its end"), State.GetClipTime(GetUpClip), Lengths[GetUpClip]);

	Inputs.bStunned = false;
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the stun blends out over 0.25 s"), State.GetClipWeight(GetUpClip), 0.5f);
	TestEqual(TEXT("to the idle"), State.GetClipWeight(WasamiEnemyClip::Idle), 0.5f);
	TestEqual(TEXT("the weights add up to 1"), SampleWeightSum(State), 1.f, 1e-5f);

	// A second stun while the first blends out starts over.
	Inputs.bStunned = true;
	State.Update(Inputs, 0.0625f);
	TestFalse(TEXT("a new stun falls again"), State.StunPlayback.IsGettingUp());
	TestEqual(TEXT("from its start"), State.StunPlayback.Elapsed, 0.0625f);
	TestEqual(TEXT("its weight goes back up from where it was"), State.Stun.Weight, 0.75f);

	// Cut short while lying, it holds its pose as it blends out: it does not get up then.
	FWasamiEnemyAnimState Cut;
	Cut.Init(Lengths, 2);
	Stunned.StunDuration = 3.f;
	Cut.Update(Stunned, 0.1f);
	const float CutFall = Lengths[Cut.StunFall];
	TestEqual(TEXT("a 3 s stun gets up as the fall ends"), Cut.StunPlayback.GetUpStart, CutFall);
	Cut.Update(Stunned, CutFall - 0.2f);
	Cut.Update(Standing, 0.2f);
	TestEqual(TEXT("cut short, it blends out"), Cut.Stun.Weight, 0.2f, 1e-5f);
	TestFalse(TEXT("without getting up"), Cut.StunPlayback.IsGettingUp());
	TestEqual(TEXT("the fall holds where it was"), Cut.GetClipTime(Cut.StunFall), CutFall - 0.1f, 1e-5f);
	TestEqual(TEXT("still"), Cut.GetClipRate(Cut.StunFall), 0.f);
	TestEqual(TEXT("under the idle"), Cut.GetClipWeight(WasamiEnemyClip::Idle), 0.8f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimOnceTest, "Wasami.Enemy.Anim.Once",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimOnceTest::RunTest(const FString& Parameters)
{
	TArray<float> Lengths = ImportedLengths();
	Lengths[WasamiEnemyClip::Capture1] = 0.f;
	Lengths[WasamiEnemyClip::ChaseCharge] = 1.f;
	Lengths[WasamiEnemyClip::ChaseSlide] = 1.f;

	FWasamiEnemyAnimState State;
	State.Init(Lengths);
	FWasamiEnemyAnimInputs Inputs;
	Inputs.Speed = 700.f;
	const float RunRate = 700.f / (450.f * Grown);
	const float RunLength = Lengths[WasamiEnemyClip::Run];
	State.Update(Inputs, 0.5f);
	TestFalse(TEXT("a missing clip does not play"), State.PlayOnce(WasamiEnemyClip::Capture1, 1.f, 0.25f, 0.25f));
	TestFalse(TEXT("nor one at a rate of 0"), State.PlayOnce(WasamiEnemyClip::ChaseSlide, 0.f, 0.25f, 0.25f));
	TestFalse(TEXT("nothing plays once"), State.IsPlayingOnce());

	TestTrue(TEXT("a clip plays once"), State.PlayOnce(WasamiEnemyClip::ChaseSlide, 1.f, 0.25f, 0.25f));
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("blending in"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.5f);
	TestEqual(TEXT("over the run"), State.GetClipWeight(WasamiEnemyClip::Run), 0.5f);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		State.Update(Inputs, 0.125f);
	}
	const float RunBefore = State.GetClipTime(WasamiEnemyClip::Run);
	State.Update(Inputs, 0.125f);
	TestTrue(TEXT("0.625 s in, it plays"), State.IsPlayingOnce());
	TestEqual(TEXT("with all the weight"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 1.f);
	TestEqual(TEXT("the run moves on under it"), State.GetClipTime(WasamiEnemyClip::Run), FMath::Fmod(RunBefore + 0.125f * RunRate, RunLength), 1e-4f);
	State.Update(Inputs, 0.125f);
	TestFalse(TEXT("its blend out starts 0.25 s before its end"), State.IsPlayingOnce());
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("blending out"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.5f);
	TestEqual(TEXT("back to the run"), State.GetClipWeight(WasamiEnemyClip::Run), 0.5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("gone at its end"), State.Once.Num(), 0);
	TestEqual(TEXT("running"), State.GetClipWeight(WasamiEnemyClip::Run), 1.f);

	// Twice as fast, it ends in half the time.
	State.PlayOnce(WasamiEnemyClip::ChaseSlide, 2.f, 0.25f, 0.25f);
	State.Update(Inputs, 0.125f);
	State.Update(Inputs, 0.125f);
	TestFalse(TEXT("at 2×, the blend out starts 0.25 s before the end"), State.IsPlayingOnce());
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the time goes twice as fast"), State.Once[0].Time, 0.75f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("gone after 0.5 s"), State.Once.Num(), 0);

	// Stopped part way, and one played over another.
	State.PlayOnce(WasamiEnemyClip::ChaseSlide, 1.f, 0.25f, 0.25f);
	State.Update(Inputs, 0.25f);
	State.StopOnce(0.5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("a stop blends out over its own time"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.75f);
	State.PlayOnce(WasamiEnemyClip::ChaseCharge, 1.f, 0.25f, 0.25f);
	TestEqual(TEXT("a new one leaves one stopping"), State.Once.Num(), 2);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the old one's blend out shortens to the new one's blend in"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.25f);
	TestEqual(TEXT("the new one blends in"), State.GetClipWeight(WasamiEnemyClip::ChaseCharge), 0.5f);
	TestEqual(TEXT("the run has what they leave"), State.GetClipWeight(WasamiEnemyClip::Run), 0.25f);
	TestEqual(TEXT("the weights add up to 1"), SampleWeightSum(State), 1.f, 1e-5f);
	State.PlayOnce(WasamiEnemyClip::ChaseSlide, 1.f, 0.f, 0.25f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the ones before one without a blend in go at once"), State.Once.Num(), 1);
	TestEqual(TEXT("and it has its weight at once"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimClipsTest, "Wasami.Enemy.Anim.Clips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimClipsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("a clip by its name"), WasamiEnemyAnim::FindClip(TEXT("Chase_Slide")), static_cast<int32>(WasamiEnemyClip::ChaseSlide));
	TestEqual(TEXT("no such clip"), WasamiEnemyAnim::FindClip(TEXT("Slide")), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("the asset's path"), WasamiEnemyAnim::ClipPath(WasamiEnemyClip::RunNightmare).ToString(),
		FString(TEXT("/Game/Wasami/Enemy/A_WasamiEnemy_Run_Nightmare.A_WasamiEnemy_Run_Nightmare")));

	// The importer (WasamiDDTools.import_wasami_enemy) made every clip, on the mesh's skeleton, as long as the tests say.
	const TArray<float> Lengths = ImportedLengths();
	const USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, TEXT("/Game/Wasami/Enemy/SK_WasamiEnemy_Skeleton.SK_WasamiEnemy_Skeleton"));
	TestNotNull(TEXT("the skeleton"), Skeleton);
	for (int32 Clip = 0; Clip < WasamiEnemyClip::Num; ++Clip)
	{
		const UAnimSequence* Sequence = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(Clip).TryLoad());
		if (!TestNotNull(FString::Printf(TEXT("the clip %s"), WasamiEnemyAnim::ClipNames[Clip]), Sequence))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s's length"), WasamiEnemyAnim::ClipNames[Clip]), Sequence->GetPlayLength(), Lengths[Clip], 1e-3f);
		TestTrue(FString::Printf(TEXT("%s is on the skeleton"), WasamiEnemyAnim::ClipNames[Clip]), Sequence->GetSkeleton() == Skeleton);
	}

	// Each get-up starts from its fall's last pose turned and moved along the floor, as UE measured the imported clips
	// (the pelvis, 2026-09-18).
	struct FMoveCase
	{
		int32 Fall;
		double Yaw;
		FVector Along;
	};
	const FMoveCase Cases[] = {
		{WasamiEnemyClip::StunFlyUp, -174.241, FVector(1.44, 12.80, 0.)},
		{WasamiEnemyClip::StunKnockDown, 168.796, FVector(10.08, -51.85, 0.)},
	};
	for (const FMoveCase& Case : Cases)
	{
		const UAnimSequence* Fall = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(Case.Fall).TryLoad());
		const UAnimSequence* GetUp = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(WasamiEnemyAnim::GetUpAfter(Case.Fall)).TryLoad());
		if (!Fall || !GetUp)
		{
			continue;
		}
		const FTransform Move = WasamiEnemyAnim::MeasureGetUpMove(*Fall, *GetUp);
		TestEqual(FString::Printf(TEXT("%s's get-up turns"), WasamiEnemyAnim::ClipNames[Case.Fall]), Move.Rotator().Yaw, Case.Yaw, 0.1);
		TestTrue(FString::Printf(TEXT("%s's get-up moves"), WasamiEnemyAnim::ClipNames[Case.Fall]), Move.GetTranslation().Equals(Case.Along, 0.1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyAnimChaseVariationsTest, "Wasami.Enemy.Anim.ChaseVariations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyAnimChaseVariationsTest::RunTest(const FString& Parameters)
{
	// 追跡中のランダムの動き (the item 26): the six clips Chase_PickUp to Chase_Slide.
	TestEqual(TEXT("six variations"), WasamiEnemyAnim::NumChaseVariations, 6);
	TestEqual(TEXT("from Chase_PickUp"), WasamiEnemyAnim::FirstChaseVariation, static_cast<int32>(WasamiEnemyClip::ChasePickUp));
	for (int32 Variation = 0; Variation < WasamiEnemyAnim::NumChaseVariations; ++Variation)
	{
		const TCHAR* const Name = WasamiEnemyAnim::ClipNames[WasamiEnemyAnim::FirstChaseVariation + Variation];
		TestTrue(FString::Printf(TEXT("%s is a chase clip"), Name), FString(Name).StartsWith(TEXT("Chase_")));
	}

	// The play rate holds the clip's feet to the chase's speed: its own speed (its forward move over its length, in
	// enemy-wasami-motions.md) grown with the mesh, within the run's limits.
	const float Chase = AWasamiEnemy::MaxSpeed;
	const TArray<float> Lengths = ImportedLengths();
	for (int32 Variation = 0; Variation < WasamiEnemyAnim::NumChaseVariations; ++Variation)
	{
		const int32 Clip = WasamiEnemyAnim::FirstChaseVariation + Variation;
		const TCHAR* const Name = WasamiEnemyAnim::ClipNames[Clip];
		const float Wanted = Chase / (WasamiEnemyAnim::ChaseVariationSpeeds[Variation] * Grown);
		const float Rate = WasamiEnemyAnim::ChaseVariationRate(Clip, Chase);
		TestEqual(FString::Printf(TEXT("the rate of %s"), Name), Rate,
			FMath::Clamp(Wanted, WasamiEnemyAnim::RunRateMin, WasamiEnemyAnim::RunRateMax), 1e-4f);
		// How far the body moves while it plays, which is the clearance a chance asks for ahead of it: 2.9 to 6.3 m.
		const float Moves = Chase * Lengths[Clip] / Rate;
		TestTrue(FString::Printf(TEXT("%s carries the body %.0f cm"), Name, Moves), Moves > 250.f && Moves < 700.f);
	}
	// Chase_Slide keeps up at 1.66, Chase_Charge is faster than the chase and plays at 0.78, and Chase_VaultLand, far
	// too slow for it, plays at the limit while the body slides through the rest.
	TestEqual(TEXT("the rate of Chase_Slide"), WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::ChaseSlide, Chase), 1.665f, 1e-2f);
	TestEqual(TEXT("the rate of Chase_Charge"), WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::ChaseCharge, Chase), 0.777f, 1e-2f);
	TestEqual(TEXT("Chase_VaultLand plays at the limit"),
		WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::ChaseVaultLand, Chase), WasamiEnemyAnim::RunRateMax);
	TestTrue(TEXT("which is under what it would want"),
		Chase / (WasamiEnemyAnim::ChaseVariationSpeeds[WasamiEnemyClip::ChaseVaultLand - WasamiEnemyAnim::FirstChaseVariation] * Grown)
			> WasamiEnemyAnim::RunRateMax);
	// A body barely moving holds at the low limit, and a clip that is no variation, or no speed, plays as it is.
	TestEqual(TEXT("a crawl holds at the low limit"), WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::ChaseCharge, 10.f),
		WasamiEnemyAnim::RunRateMin);
	TestEqual(TEXT("the run is no variation"), WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::Run, Chase), 1.f);
	TestEqual(TEXT("standing still"), WasamiEnemyAnim::ChaseVariationRate(WasamiEnemyClip::ChaseSlide, 0.f), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyChaseChancesTest, "Wasami.Enemy.Chase.Chances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyChaseChancesTest::RunTest(const FString& Parameters)
{
	// The chances of 追跡中のランダムの動き, moved on by the decisions (every 0.5 s).
	constexpr float Decision = AWasamiEnemy::DecisionInterval;
	constexpr int32 First = WasamiEnemyAnim::FirstChaseVariation;
	constexpr int32 Count = WasamiEnemyAnim::NumChaseVariations;
	TestEqual(TEXT("the gaps start at 6 s"), FWasamiChaseVariations::MinGap, 6.f);
	TestEqual(TEXT("and end at 10 (about eight on average)"), FWasamiChaseVariations::MaxGap, 10.f);

	FWasamiChaseVariations Chances;
	Chances.Init(20260920);
	TestEqual(TEXT("no chance without a chase"), Chances.Advance(false, Decision), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("and no wait"), Chances.GetWait(), FWasamiChaseVariations::NotChasing);

	// The decision a chase begins with draws its first gap; the chance comes on the first decision past it.
	TestEqual(TEXT("the first decision of a chase plays nothing"), Chances.Advance(true, Decision), static_cast<int32>(INDEX_NONE));
	const float FirstGap = Chances.GetWait();
	TestTrue(FString::Printf(TEXT("a gap of %.2f s"), FirstGap),
		FirstGap >= FWasamiChaseVariations::MinGap && FirstGap <= FWasamiChaseVariations::MaxGap);
	int32 Clip = INDEX_NONE;
	float Waited = 0.f;
	while (Clip == INDEX_NONE && Waited < FWasamiChaseVariations::MaxGap + Decision)
	{
		Clip = Chances.Advance(true, Decision);
		Waited += Decision;
	}
	TestTrue(TEXT("one of the six comes"), Clip >= First && Clip < First + Count);
	TestTrue(FString::Printf(TEXT("on the first decision past the gap (%.2f s)"), Waited),
		Waited >= FirstGap && Waited < FirstGap + Decision);

	// Nothing played (the way ahead was blocked): the chance stands open and the next decision draws again.
	TestEqual(TEXT("the chance stands open"), Chances.GetWait(), 0.f);
	const int32 Open = Chances.Advance(true, Decision);
	TestTrue(TEXT("which draws one of the six again"), Open >= First && Open < First + Count);
	TestEqual(TEXT("nothing has played yet"), Chances.GetLast(), static_cast<int32>(INDEX_NONE));

	// Played: the next chance is a gap away and the draws go around this clip. A chance let go (Skip) is a gap away
	// too, and leaves the clip to go around as it was.
	Chances.Played(Open);
	TestEqual(TEXT("the clip played"), Chances.GetLast(), Open);
	TestTrue(TEXT("the next chance is a gap away"), Chances.GetWait() >= FWasamiChaseVariations::MinGap
		&& Chances.GetWait() <= FWasamiChaseVariations::MaxGap);
	Chances.Advance(true, FWasamiChaseVariations::MaxGap);
	Chances.Skip();
	TestEqual(TEXT("a chance let go changes nothing but the wait"), Chances.GetLast(), Open);
	TestTrue(TEXT("which is a gap away too"), Chances.GetWait() >= FWasamiChaseVariations::MinGap
		&& Chances.GetWait() <= FWasamiChaseVariations::MaxGap);

	// Over many chances: each of the six comes up, never twice in a row, and the gaps average about 8 s.
	int32 Draws[Count] = {};
	int32 Repeats = 0;
	int32 OutOfRange = 0;
	float GapSum = 0.f;
	constexpr int32 Many = 2000;
	for (int32 Chance = 0; Chance < Many; ++Chance)
	{
		const int32 Before = Chances.GetLast();
		// A decision long past the chance, so that every turn of the loop draws one.
		const int32 Drawn = Chances.Advance(true, FWasamiChaseVariations::MaxGap);
		if (Drawn < First || Drawn >= First + Count)
		{
			++OutOfRange;
			continue;
		}
		Repeats += Drawn == Before ? 1 : 0;
		++Draws[Drawn - First];
		Chances.Played(Drawn);
		const float Gap = Chances.GetWait();
		OutOfRange += Gap < FWasamiChaseVariations::MinGap || Gap > FWasamiChaseVariations::MaxGap ? 1 : 0;
		GapSum += Gap;
	}
	TestEqual(TEXT("every draw is one of the six, every gap 6 to 10 s"), OutOfRange, 0);
	TestEqual(TEXT("never the clip before"), Repeats, 0);
	for (int32 Variation = 0; Variation < Count; ++Variation)
	{
		TestTrue(FString::Printf(TEXT("%s comes up (%d of %d)"), WasamiEnemyAnim::ClipNames[First + Variation],
			Draws[Variation], Many), Draws[Variation] > Many / Count / 2);
	}
	TestEqual(TEXT("about 8 s apart"), GapSum / Many, 8.f, 0.15f);

	// The chase is over: the chances stop with it, and the next chase waits a whole gap and may open with any of the six.
	Chances.Advance(false, Decision);
	TestEqual(TEXT("no wait once the chase is over"), Chances.GetWait(), FWasamiChaseVariations::NotChasing);
	TestEqual(TEXT("nor a clip to go around"), Chances.GetLast(), static_cast<int32>(INDEX_NONE));
	Chances.Advance(true, Decision);
	TestTrue(TEXT("the next chase waits a whole gap"), Chances.GetWait() >= FWasamiChaseVariations::MinGap);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyVoiceCallsTest, "Wasami.Enemy.Voice.Calls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyVoiceCallsTest::RunTest(const FString& Parameters)
{
	// 巡回中と硬直中の呼びかけ: a call every 14 to 26 s (the WebGL version's vocal), each one of the four rounds voices.
	using Idle = FWasamiEnemyIdleVoices;
	TestEqual(TEXT("the gaps start at 14 s"), Idle::MinGap, 14.f);
	TestEqual(TEXT("and end at 26 (about twenty on average)"), Idle::MaxGap, 26.f);
	TestEqual(TEXT("the four rounds voices"), Idle::Num, 4);
	TestEqual(TEXT("from Calling"), Idle::First, static_cast<int32>(EWasamiVoice::Calling));

	Idle Calls;
	Calls.Init(20260920);
	int32 Draws[Idle::Num] = {};
	int32 OutOfRange = 0;
	float GapSum = 0.f;
	constexpr int32 Many = 2000;
	for (int32 Call = 0; Call < Many; ++Call)
	{
		const float Gap = Calls.DrawGap();
		OutOfRange += Gap < Idle::MinGap || Gap > Idle::MaxGap ? 1 : 0;
		GapSum += Gap;
		const int32 Voice = static_cast<int32>(Calls.DrawVoice());
		if (Voice < Idle::First || Voice >= Idle::First + Idle::Num)
		{
			++OutOfRange;
			continue;
		}
		++Draws[Voice - Idle::First];
	}
	TestEqual(TEXT("every gap is 14 to 26 s and every voice one of the four"), OutOfRange, 0);
	TestEqual(TEXT("about 20 s apart"), GapSum / Many, 20.f, 0.4f);
	for (int32 Voice = 0; Voice < Idle::Num; ++Voice)
	{
		TestTrue(FString::Printf(TEXT("the voice %d comes up (%d of %d)"), Idle::First + Voice, Draws[Voice], Many),
			Draws[Voice] > Many / Idle::Num / 2);
	}
	// The same seed says the same things.
	Idle Same;
	Same.Init(20260920);
	Calls.Init(20260920);
	TestEqual(TEXT("the same seed draws the same gap"), Same.DrawGap(), Calls.DrawGap());
	TestEqual(TEXT("and the same voice"), static_cast<int32>(Same.DrawVoice()), static_cast<int32>(Calls.DrawVoice()));

	// 発見の声: the game mode lets one Found through every 12 s, whichever enemy asks.
	TestEqual(TEXT("a Found every 12 s"), AWasamiGameMode::FoundVoiceGap, 12.f);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiGameMode* Mode = World->GetAuthGameMode<AWasamiGameMode>();
	if (!TestNotNull(TEXT("the project's game mode"), Mode))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	// The world's settings clamp a tick to 0.4 s however long a step is asked for, so the time is let by in small ones.
	auto LetBy = [&Wrapper](float Seconds)
	{
		constexpr float Step = 0.25f;
		for (float Left = Seconds; Left > 0.f; Left -= Step)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, Step));
		}
	};
	TestTrue(TEXT("the first Found of a run goes"), Mode->TakeFoundVoice());
	TestFalse(TEXT("a second at once does not"), Mode->TakeFoundVoice());
	LetBy(AWasamiGameMode::FoundVoiceGap - 1.f);
	TestFalse(TEXT("nor one a second short of the gap"), Mode->TakeFoundVoice());
	LetBy(1.5f);
	TestTrue(TEXT("one past the gap goes"), Mode->TakeFoundVoice());
	TestFalse(TEXT("and closes it again"), Mode->TakeFoundVoice());
	Wrapper.ForwardErrorMessages(this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorDefaultsTest, "Wasami.Enemy.Actor.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorDefaultsTest::RunTest(const FString& Parameters)
{
	// BP_06_ReaperNurse's CDO and components (pak_reference_2).
	const AWasamiEnemy* Enemy = GetDefault<AWasamiEnemy>();
	TestTrue(TEXT("the Enemy tag"), Enemy->ActorHasTag(TEXT("Enemy")));
	TestTrue(TEXT("the enemy interface"), Enemy->GetClass()->ImplementsInterface(UWasamiEnemyInterface::StaticClass()));
	TestTrue(TEXT("an AI possesses it, placed or spawned"), Enemy->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned);
	TestFalse(TEXT("the controller does not turn it"), Enemy->bUseControllerRotationYaw);
	TestFalse(TEXT("CanSpawn is off"), Enemy->bCanSpawn);
	TestFalse(TEXT("not a sentry"), Enemy->bAggressiveIdle);
	TestEqual(TEXT("Normal Speed"), Enemy->NormalSpeed, 200.f);
	TestEqual(TEXT("Skate Speed"), Enemy->SkateSpeed, 430.f);

	const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
	TestEqual(TEXT("the capsule's radius"), Capsule->GetUnscaledCapsuleRadius(), 34.f);
	TestEqual(TEXT("the capsule's half height"), Capsule->GetUnscaledCapsuleHalfHeight(), 118.05822f, 1e-4f);
	TestTrue(TEXT("a pawn's collision"), Capsule->GetCollisionObjectType() == ECC_Pawn);

	// Sphere: on the capsule's centre, overlapping pawns only.
	const USphereComponent* Sphere = Enemy->GetSphere();
	if (TestNotNull(TEXT("the Sphere"), Sphere))
	{
		TestTrue(TEXT("on the capsule"), Sphere->GetAttachParent() == Capsule);
		TestTrue(TEXT("at its centre"), Sphere->GetRelativeLocation().IsZero());
		TestEqual(TEXT("the Sphere's radius"), Sphere->GetUnscaledSphereRadius(), 54.928059f, 1e-4f);
		TestTrue(TEXT("a custom profile"), Sphere->GetCollisionProfileName() == UCollisionProfile::CustomCollisionProfileName);
		TestTrue(TEXT("overlapping pawns"), Sphere->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
		bool bRestIgnored = true;
		for (const ECollisionChannel Channel : {ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody, ECC_Vehicle, ECC_Destructible})
		{
			bRestIgnored &= Sphere->GetCollisionResponseToChannel(Channel) == ECR_Ignore;
		}
		TestTrue(TEXT("ignoring the rest"), bRestIgnored);
		TestTrue(TEXT("with overlap events"), Sphere->GetGenerateOverlapEvents());
	}
	for (const UClass* Nurse : {AWasamiEnemy06Chase::StaticClass(), AWasamiEnemySentry::StaticClass(), AWasamiEnemyZone2::StaticClass()})
	{
		const AWasamiEnemy* Default = CastChecked<AWasamiEnemy>(Nurse->GetDefaultObject());
		TestTrue(FString::Printf(TEXT("%s has the Sphere too"), *Nurse->GetName()),
			Default->GetSphere() && Default->GetSphere()->GetUnscaledSphereRadius() == Sphere->GetUnscaledSphereRadius());
	}

	const UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	TestEqual(TEXT("the top speed"), Movement->MaxWalkSpeed, 430.f);
	TestTrue(TEXT("the turn rate"), Movement->RotationRate.Equals(FRotator(0., 300., 0.)));
	TestTrue(TEXT("it turns to the controller's wish"), Movement->bUseControllerDesiredRotation);
	TestTrue(TEXT("and to its movement"), Movement->bOrientRotationToMovement);

	const USkeletalMeshComponent* Body = Enemy->GetMesh();
	TestTrue(TEXT("the mesh's place"), Body->GetRelativeLocation().Equals(FVector(-6.216e-5, -2.155e-4, -117.84394), 1e-4));
	TestEqual(TEXT("the mesh's turn"), Body->GetRelativeRotation().Yaw, -90.00012, 1e-4);
	TestEqual(TEXT("Wasami's head top at the nurse's"), AWasamiEnemy::MeshScale, 229.05135 / 168.52719);
	TestTrue(TEXT("grown the same on X, Y and Z"), Body->GetRelativeScale3D().Equals(FVector(AWasamiEnemy::MeshScale), 1e-6));
	TestTrue(TEXT("the mesh plays the enemy's animation"), Body->AnimClass.Get() == UWasamiEnemyAnimInstance::StaticClass());
	TestEqual(TEXT("the decisions' interval"), AWasamiEnemy::DecisionInterval, 0.5f);
	TestEqual(TEXT("the stun's wait"), AWasamiEnemy::StunSeconds, 17.f);
	TestEqual(TEXT("Can See Player's angle"), AWasamiEnemy::ViewAngle, 100.f);
	TestEqual(TEXT("the delay that forgets the player"), AWasamiEnemy::ForgetSeconds, 3.f);
	TestEqual(TEXT("the random point's radius"), AWasamiEnemy::RandomPointRadius, 3000.f);
	TestEqual(TEXT("the chase's acceptance"), AWasamiEnemy::ChaseAcceptance, 5.f);
	TestEqual(TEXT("the Point Of Interest's"), AWasamiEnemy::PointOfInterestAcceptance, 5.f);
	TestEqual(TEXT("the random point's"), AWasamiEnemy::RandomPointAcceptance, 50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemySoundTest, "Wasami.Enemy.Actor.Sound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemySoundTest::RunTest(const FString& Parameters)
{
	// Skate Audio (BP_06_ReaperNurse's): on the capsule with no transform of its own, playing from the start at no
	// volume and the low pitch.
	const AWasamiEnemy* Enemy = GetDefault<AWasamiEnemy>();
	const UAudioComponent* Skate = Enemy->GetSkateAudio();
	if (!TestNotNull(TEXT("the Skate Audio"), Skate))
	{
		return false;
	}
	TestTrue(TEXT("on the capsule"), Skate->GetAttachParent() == Enemy->GetCapsuleComponent());
	TestTrue(TEXT("with no transform of its own"), Skate->GetRelativeTransform().Equals(FTransform::Identity));
	TestTrue(TEXT("it plays from the start"), Skate->bAutoActivate);
	TestEqual(TEXT("silent at first"), Skate->VolumeMultiplier, 0.f);
	TestEqual(TEXT("the low pitch"), Skate->PitchMultiplier, 1.2f);
	// Talk Audio (the nurse's): on the capsule as well, holding no sound of its own and waiting for Talk to give it one,
	// and saying no subtitle itself (Talk puts the line up for as long as it can be read).
	const UAudioComponent* Talk = Enemy->GetTalkAudio();
	if (!TestNotNull(TEXT("the Talk Audio"), Talk))
	{
		return false;
	}
	TestTrue(TEXT("on the capsule"), Talk->GetAttachParent() == Enemy->GetCapsuleComponent());
	TestTrue(TEXT("with no transform of its own"), Talk->GetRelativeTransform().Equals(FTransform::Identity));
	TestFalse(TEXT("it waits to be spoken through"), Talk->bAutoActivate);
	TestNull(TEXT("with nothing to say"), Talk->Sound.Get());
	TestTrue(TEXT("and no subtitle of its own"), Talk->bSuppressSubtitles);
	for (const UClass* Nurse : {AWasamiEnemy06Chase::StaticClass(), AWasamiEnemySentry::StaticClass(), AWasamiEnemyZone2::StaticClass()})
	{
		const AWasamiEnemy* Default = CastChecked<AWasamiEnemy>(Nurse->GetDefaultObject());
		TestNotNull(FString::Printf(TEXT("%s has the Skate Audio too"), *Nurse->GetName()), Default->GetSkateAudio());
		TestNotNull(FString::Printf(TEXT("and %s's Talk Audio"), *Nurse->GetName()), Default->GetTalkAudio());
	}

	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	AWasamiEnemy* Spawned = AWasamiEnemy::SpawnEnemy(Wrapper.GetTestWorld(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("an enemy"), Spawned))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UAudioComponent* Audio = Spawned->GetSkateAudio();
	if (TestNotNull(TEXT("the spawned Skate Audio"), Audio) && TestNotNull(TEXT("the move loop"), Audio->Sound.Get()))
	{
		TestEqual(TEXT("DD_Rollerskating_Fast_V1_LOOP"), Audio->Sound->GetName(), TEXT("DD_Rollerskating_Fast_V1_LOOP"));
		TestTrue(TEXT("through MonkeyAttenuation"),
			Audio->AttenuationSettings && Audio->AttenuationSettings->GetName() == TEXT("MonkeyAttenuation"));
		TestTrue(TEXT("and running from the start"), Audio->IsPlaying());
	}

	// Talk: the voice is set on the Talk Audio and played through AgathaAttenuation. A line that is still sounding
	// keeps an unforced one from starting; a forced one goes over it.
	UAudioComponent* Speech = Spawned->GetTalkAudio();
	if (TestNotNull(TEXT("the spawned Talk Audio"), Speech))
	{
		TestTrue(TEXT("through AgathaAttenuation"),
			Speech->AttenuationSettings && Speech->AttenuationSettings->GetName() == TEXT("AgathaAttenuation"));
		TestNotNull(TEXT("the found line is said"), Spawned->Talk(EWasamiVoice::Found, true, AWasamiEnemy::FoundVolume));
		if (TestNotNull(TEXT("which is on it"), Speech->Sound.Get()))
		{
			TestEqual(TEXT("Wasami_Found"), Speech->Sound->GetName(), TEXT("Wasami_Found"));
		}
		TestEqual(TEXT("at its full volume"), Speech->VolumeMultiplier, AWasamiEnemy::FoundVolume);
		TestTrue(TEXT("and sounding"), Speech->IsPlaying());
		TestNull(TEXT("a rounds voice does not cut it off"),
			Spawned->Talk(EWasamiVoice::Calling, false, AWasamiEnemy::IdleVolume));
		TestEqual(TEXT("which still sounds"), Speech->Sound->GetName(), TEXT("Wasami_Found"));
		TestNotNull(TEXT("a forced one goes over it"),
			Spawned->Talk(EWasamiVoice::Calling, true, AWasamiEnemy::IdleVolume));
		TestEqual(TEXT("Wasami_Calling"), Speech->Sound->GetName(), TEXT("Wasami_Calling"));
		TestEqual(TEXT("a little under full"), Speech->VolumeMultiplier, AWasamiEnemy::IdleVolume);
		Speech->Stop();
	}

	// Update Skate Sound: a whole second of interpolation lands on the mapped value (InterpSpeed * DeltaTime >= 1).
	UCharacterMovementComponent* Movement = Spawned->GetCharacterMovement();
	struct FSpeedCase { float Speed; float Volume; float Pitch; };
	const FSpeedCase Cases[] = {
		{0.f, 0.f, 1.2f},        // standing: silent, the low pitch
		{200.f, 0.5f, 1.2f},     // half of the volume's range, still under the pitch's
		{400.f, 1.f, 1.2f},      // full volume where the pitch starts to rise
		{800.f, 1.f, 1.5f},      // the chase: full volume and the high pitch
		{2000.f, 1.f, 1.5f},     // clamped over it
	};
	for (const FSpeedCase& Case : Cases)
	{
		Movement->Velocity = FVector(Case.Speed, 0., 0.);
		Spawned->UpdateSkateSound(1.f);
		TestEqual(FString::Printf(TEXT("the volume at %.0f cm/s"), Case.Speed), Audio->VolumeMultiplier, Case.Volume, 1e-4f);
		TestEqual(FString::Printf(TEXT("the pitch at %.0f cm/s"), Case.Speed), Audio->PitchMultiplier, Case.Pitch, 1e-4f);
	}

	// It follows, it does not jump: a small step of a tick moves part of the way only.
	Movement->Velocity = FVector::ZeroVector;
	Spawned->UpdateSkateSound(0.0625f);
	TestTrue(TEXT("the volume eases back down"), Audio->VolumeMultiplier > 0.f && Audio->VolumeMultiplier < 1.f);

	Wrapper.ForwardErrorMessages(this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorChoiceTest, "Wasami.Enemy.Actor.Choice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorChoiceTest::RunTest(const FString& Parameters)
{
	// A game world without navigation, ticked by hand as in Actor.Stun: every move fails 0.1 s after it is asked for.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	// The player: the first player controller's character, not possessed so that it stays where it is put.
	const FVector InFront(1000., 0., 500.);
	ACharacter* Player = World->SpawnActor<ACharacter>(InFront, FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	if (!TestTrue(TEXT("the player is the player"), UGameplayStatics::GetPlayerCharacter(World, 0) == Player))
	{
		return false;
	}
	AWasamiEnemy* Enemy = AWasamiEnemy::SpawnEnemy(World, FVector(0., 0., 500.), 0.f);
	if (!TestNotNull(TEXT("a spawned enemy"), Enemy))
	{
		return false;
	}
	Enemy->GetCharacterMovement()->GravityScale = 0.f;
	// Held strongly: nothing else refers to it, and a garbage collection while the test runs would unbind it.
	const TStrongObjectPtr<UWasamiTestListener> Listener(NewObject<UWasamiTestListener>());
	FScriptDelegate Heard;
	Heard.BindUFunction(Listener.Get(), GET_FUNCTION_NAME_CHECKED(UWasamiTestListener, Hear));
	Enemy->OnCloseBy.Add(Heard);
	auto HeardCount = [&Listener]() { return Listener->Count; };

	// Generate Random Point at BeginPlay leaves the point as it was without navigation data.
	TestTrue(TEXT("no random point without navigation"), Enemy->RandomPoint.IsZero());

	// Can See Player: within 100 degrees of its front at any distance, when a Camera trace hits the player first.
	TestTrue(TEXT("it sees the player in front"), Enemy->CanSeePlayer());
	auto PutAt = [Player](double Degrees, double Distance)
	{
		const double Radians = FMath::DegreesToRadians(Degrees);
		Player->SetActorLocation(FVector(Distance * FMath::Cos(Radians), Distance * FMath::Sin(Radians), 500.));
	};
	PutAt(99., 1000.);
	TestTrue(TEXT("99 degrees aside"), Enemy->CanSeePlayer());
	PutAt(-101., 1000.);
	TestFalse(TEXT("not 101"), Enemy->CanSeePlayer());
	PutAt(180., 1000.);
	TestFalse(TEXT("not behind"), Enemy->CanSeePlayer());
	PutAt(0., 50000.);
	TestTrue(TEXT("however far"), Enemy->CanSeePlayer());
	Player->SetActorLocation(InFront);
	ACharacter* Between = World->SpawnActor<ACharacter>(FVector(500., 0., 500.), FRotator::ZeroRotator);
	TestFalse(TEXT("not past another character"), Enemy->CanSeePlayer());
	Between->Destroy();
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	TestFalse(TEXT("not while the player vanishes (its capsule ignores Camera)"), Enemy->CanSeePlayer());
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	TestTrue(TEXT("and again after"), Enemy->CanSeePlayer());

	// The decisions come on the ticks at 0.625, 1.125, … s. The first has not seen the player: Not Seeing Player walks
	// to the random point, then Can See Player sets Seen Player Recently.
	TickTo(0.625f);
	TestTrue(TEXT("it saw the player"), Enemy->IsChasing());
	TestEqual(TEXT("walking to the random point"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 200.f);
	TestEqual(TEXT("not chased yet"), HeardCount(), 0);
	TestTrue(TEXT("no Point Of Interest"), Enemy->PointOfInterest.IsZero());

	// The next chases: runs to the player, keeps where it is, CloseBy once.
	TickTo(1.125f);
	TestEqual(TEXT("the chase runs"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 430.f);
	TestTrue(TEXT("the Point Of Interest is the player"), Enemy->PointOfInterest.Equals(InFront));
	TestEqual(TEXT("CloseBy"), HeardCount(), 1);
	TickTo(1.625f);
	TestEqual(TEXT("once"), HeardCount(), 1);

	// Out of sight it keeps chasing: each chase starts the 3 s delay over, and it knows where the player is.
	const FVector Behind(-1000., 0., 500.);
	Player->SetActorLocation(Behind);
	TestFalse(TEXT("the player is out of sight"), Enemy->CanSeePlayer());
	TickTo(6.125f);
	TestTrue(TEXT("still chasing"), Enemy->IsChasing());
	TestTrue(TEXT("to where the player is"), Enemy->PointOfInterest.Equals(Behind));
	TestEqual(TEXT("running"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 430.f);

	// Player Vanish: the next decision walks to the Point Of Interest, whose failed move clears it 0.1 s later.
	IWasamiEnemyInterface::Execute_PlayerVanish(Enemy);
	TestFalse(TEXT("Vanish: not chasing"), Enemy->IsChasing());
	TickTo(6.625f);
	TestEqual(TEXT("walking"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 200.f);
	TestTrue(TEXT("to the Point Of Interest"), Enemy->PointOfInterest.Equals(Behind));
	TickTo(6.75f);
	TestTrue(TEXT("which the failed move clears"), Enemy->PointOfInterest.IsZero());

	// The delay from the last chase (at 6.125 s, due 9.125 s) opens CloseBy again.
	Player->SetActorLocation(InFront);
	Enemy->SetActorRotation(FRotator(0., 180., 0.));
	TickTo(9.125f);
	TestFalse(TEXT("turned away, it does not see the player"), Enemy->IsChasing());
	Enemy->SetActorRotation(FRotator::ZeroRotator);
	TickTo(9.625f);
	TestTrue(TEXT("it sees the player again"), Enemy->IsChasing());
	TickTo(10.125f);
	TestEqual(TEXT("CloseBy again"), HeardCount(), 2);

	// A stun stops the decisions, so the delay after the last chase (10.125 s) forgets the player.
	Player->SetActorLocation(Behind);
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, false);
	TickTo(10.625f);
	TestTrue(TEXT("stunned"), Enemy->IsStunRunning());
	TestTrue(TEXT("chasing yet"), Enemy->IsChasing());
	TickTo(13.125f);
	TestTrue(TEXT("until the delay is past"), Enemy->IsChasing());
	TickTo(13.1875f);
	TestFalse(TEXT("then it has forgotten the player"), Enemy->IsChasing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorStunTest, "Wasami.Enemy.Actor.Stun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorStunTest::RunTest(const FString& Parameters)
{
	// A game world that plays and is ticked by hand, in steps a float adds up exactly, so that the timers' ticks are known.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	TestNull(TEXT("an enemy without CanSpawn is gone as it begins play"), World->SpawnActor<AWasamiEnemy>());
	AWasamiEnemy* Enemy = AWasamiEnemy::SpawnEnemy(World, FVector(0., 0., 500.), 90.f);
	if (!TestNotNull(TEXT("a spawned enemy"), Enemy))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	TestEqual(TEXT("turned"), Enemy->GetActorRotation().Yaw, 90., 1e-4);
	TestNotNull(TEXT("with its AI"), Enemy->GetController());
	const USkeletalMesh* Mesh = Enemy->GetMesh()->GetSkeletalMeshAsset();
	TestTrue(TEXT("with SK_WasamiEnemy"), Mesh && Mesh->GetName() == TEXT("SK_WasamiEnemy"));
	const UWasamiEnemyAnimInstance* Anim = Enemy->GetEnemyAnim();
	if (!TestNotNull(TEXT("and its animation"), Anim))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	const FWasamiEnemyAnimState& AnimState = Anim->GetAnimState();
	const float GetUp = AnimState.GetLength(WasamiEnemyClip::StunGetUpFlyUp);
	TestEqual(TEXT("the get-ups are loaded"), GetUp, 117 / 30.f, 1e-3f);
	TestEqual(TEXT("both as long"), AnimState.GetLength(WasamiEnemyClip::StunGetUpKnockDown), GetUp, 1e-3f);
	TestTrue(TEXT("with the moves to where they start"),
		Anim->GetGetUpMove(WasamiEnemyClip::StunFlyUp).IsSet() && Anim->GetGetUpMove(WasamiEnemyClip::StunKnockDown).IsSet());
	TestTrue(TEXT("Get State reads Patrol"), IWasamiEnemyInterface::Execute_GetState(Enemy) == EWasamiEnemyState::Patrol);
	TestFalse(TEXT("it shows in the telepathy"), IWasamiEnemyInterface::Execute_NoTelepathy(Enemy));
	Enemy->SetWalkState(true);
	TestEqual(TEXT("the walk's speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 200.f);
	Enemy->SetWalkState(false);
	TestEqual(TEXT("the skate's speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 430.f);

	// It floats and keeps the speed it is given. A timer set outside a tick counts from the end of the next one and
	// fires on the first tick past its time: the decision set at the spawn is due at 0.5625 s and comes on the ticks at
	// 0.625, 1.125, … s, and a stun that the decision at 0.625 s starts is due at 17.625 s.
	UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	Movement->GravityScale = 0.f;
	TickTo(0.25f);
	Movement->Velocity = FVector(300., 0., 0.);
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, false);
	TestTrue(TEXT("Set State stuns at once"), Enemy->IsStunned());
	TestTrue(TEXT("its State"), Enemy->GetCurrentState() == EWasamiEnemyState::Stun);
	TestTrue(TEXT("Get State still reads Patrol"), IWasamiEnemyInterface::Execute_GetState(Enemy) == EWasamiEnemyState::Patrol);
	TestFalse(TEXT("the stun waits for the decision"), Enemy->IsStunRunning());
	TestEqual(TEXT("to the decision, then 17 s"), Enemy->GetStunTimeLeft(), 0.3125f + 17.f, 1e-4f);

	TickTo(0.3125f);
	TestTrue(TEXT("the animation is stunned"), Anim->bStunned);
	TestEqual(TEXT("for what is left of the stun"), AnimState.StunPlayback.GetUpStart, 17.3125f - GetUp, 1e-4f);
	TestTrue(TEXT("before the decision it still moves"), Movement->Velocity.X > 250.);
	TickTo(0.5625f);
	TestFalse(TEXT("not yet decided"), Enemy->IsStunRunning());
	TickTo(0.625f);
	TestTrue(TEXT("the decision starts the stun"), Enemy->IsStunRunning());
	TestTrue(TEXT("and stops the movement"), Movement->Velocity.IsNearlyZero());
	TestEqual(TEXT("for 17 s"), Enemy->GetStunTimeLeft(), 17.f, 1e-4f);
	TickTo(1.f);
	TestTrue(TEXT("it stays still"), Movement->Velocity.IsNearlyZero());

	// While it waits, another stun changes nothing, and a State set to Patrol and back does not start it over.
	TickTo(5.f);
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, true);
	TestEqual(TEXT("a second stun does not add time"), Enemy->GetStunTimeLeft(), 12.625f, 1e-4f);
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Patrol, false);
	TestFalse(TEXT("Patrol is not stunned"), Enemy->IsStunned());
	TestEqual(TEXT("nothing is left for the animation"), Enemy->GetStunTimeLeft(), 0.f);
	TestTrue(TEXT("but the wait goes on"), Enemy->IsStunRunning());
	TickTo(6.f);
	TestFalse(TEXT("the animation came out of the stun"), Anim->bStunned);
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, false);
	TestEqual(TEXT("stunned again, it ends with the first wait"), Enemy->GetStunTimeLeft(), 11.625f, 1e-4f);

	// Stunned again at 6.0625 s for 11.625 s, it gets up 11.625 − 3.9 s in (on the tick at 13.75 s), and the enemy moves
	// so that the get-up starts where the fall lies: its pelvis stays where it was.
	TickTo(13.6875f);
	TestFalse(TEXT("lying"), AnimState.StunPlayback.IsGettingUp());
	const int32 FallClip = AnimState.StunFall;
	const FTransform MeshBefore = Enemy->GetMesh()->GetComponentTransform();
	const FTransform ActorBefore = Enemy->GetActorTransform();
	TickTo(13.75f);
	TestTrue(TEXT("getting up"), AnimState.StunPlayback.IsGettingUp());
	const UAnimSequence* FallSequence = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(FallClip).TryLoad());
	const UAnimSequence* GetUpSequence = Cast<UAnimSequence>(WasamiEnemyAnim::ClipPath(WasamiEnemyAnim::GetUpAfter(FallClip)).TryLoad());
	if (TestNotNull(TEXT("the fall"), FallSequence) && TestNotNull(TEXT("its get-up"), GetUpSequence))
	{
		const FTransform Lying = WasamiEnemyAnim::GetRootTransform(*FallSequence, FallSequence->GetPlayLength()) * MeshBefore;
		const FTransform Starting = WasamiEnemyAnim::GetRootTransform(*GetUpSequence, 0.) * Enemy->GetMesh()->GetComponentTransform();
		TestTrue(TEXT("the pelvis stays where it lay"), Starting.GetLocation().Equals(Lying.GetLocation(), 0.5));
		TestTrue(TEXT("turned as it lay"), Starting.GetRotation().AngularDistance(Lying.GetRotation()) < FMath::DegreesToRadians(0.5));
	}
	const FTransform ActorAfter = Enemy->GetActorTransform();
	TestEqual(TEXT("the enemy turned with the get-up"), FRotator::NormalizeAxis(ActorAfter.Rotator().Yaw - ActorBefore.Rotator().Yaw),
		Anim->GetGetUpMove(FallClip).GetValue().Rotator().Yaw, 0.01);
	TestEqual(TEXT("upright"), ActorAfter.Rotator().Pitch, 0., 1e-3);
	TestEqual(TEXT("at its height"), ActorAfter.GetLocation().Z, ActorBefore.GetLocation().Z, 1e-3);
	TestEqual(TEXT("the controller turned with it"), FRotator::NormalizeAxis(Enemy->GetController()->GetControlRotation().Yaw - ActorAfter.Rotator().Yaw), 0., 0.01);
	TickTo(13.8125f);
	TestTrue(TEXT("it moves once"), Enemy->GetActorTransform().Equals(ActorAfter, 1e-3));

	TickTo(17.5625f);
	TestTrue(TEXT("still stunned"), Enemy->IsStunned());
	TestTrue(TEXT("getting up"), AnimState.StunPlayback.IsGettingUp());
	TestEqual(TEXT("one step before the get-up's end"), AnimState.StunPlayback.GetClipTime(), GetUp - Step, 1e-3f);
	TickTo(17.625f);
	TestTrue(TEXT("stunned while the wait is due"), Enemy->IsStunRunning());
	TestEqual(TEXT("the get-up ends with the wait"), AnimState.StunPlayback.GetClipTime(), GetUp, 1e-3f);
	TickTo(17.6875f);
	TestTrue(TEXT("past it, Patrol"), Enemy->GetCurrentState() == EWasamiEnemyState::Patrol);
	TestFalse(TEXT("the stun has ended"), Enemy->IsStunRunning());
	TickTo(17.75f);
	TestFalse(TEXT("the animation follows"), Anim->bStunned);

	// A stun after the end starts again at the next decision (due at 18.0625 s).
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, true);
	TestEqual(TEXT("a new stun waits for the decision"), Enemy->GetStunTimeLeft(), 0.3125f + 17.f, 1e-4f);
	TickTo(18.0625f);
	TestFalse(TEXT("not before it is past"), Enemy->IsStunRunning());
	TickTo(18.125f);
	TestTrue(TEXT("and runs again"), Enemy->IsStunRunning());
	TestEqual(TEXT("for 17 s"), Enemy->GetStunTimeLeft(), 17.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorPowersTest, "Wasami.Enemy.Actor.Powers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorPowersTest::RunTest(const FString& Parameters)
{
	// The powers find it as they find the stand-in: Primal Fear by its pawn body, Vanish by its tag, the telepathy by
	// its interface.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiEnemy* Enemy = AWasamiEnemy::SpawnEnemy(World, FVector(500., 0., 0.));
	if (!TestNotNull(TEXT("a spawned enemy"), Enemy))
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}

	TestEqual(TEXT("Primal Fear stuns it"), AWasamiPrimalPower::StunEnemies(World, FVector::ZeroVector, 1500.f), 1);
	TestTrue(TEXT("stunned"), Enemy->IsStunned());

	Enemy->bSeenPlayerRecently = true;
	TestEqual(TEXT("Vanish tells it"), AWasamiVanishPower::NotifyEnemies(World), 1);
	TestFalse(TEXT("it has not seen the player"), Enemy->bSeenPlayerRecently);

	const FTransform AtOrigin = FTransform::Identity;
	AWasamiTelepathyPower* Telepathy = World->SpawnActorDeferred<AWasamiTelepathyPower>(AWasamiTelepathyPower::StaticClass(), AtOrigin);
	Telepathy->Time = FWasamiPowerTuning::ForLevel(5).TelepathyDuration;
	Telepathy->FinishSpawning(AtOrigin);
	int32 OnEnemy = 0;
	for (TActorIterator<AWasamiTelepathyTracker> It(World); It; ++It)
	{
		OnEnemy += It->Actor.Get() == Enemy ? 1 : 0;
	}
	TestEqual(TEXT("the telepathy marks it"), OnEnemy, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorChase06Test, "Wasami.Enemy.Actor.Chase06",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorChase06Test::RunTest(const FString& Parameters)
{
	// A game world without navigation, ticked by hand in steps a float adds up exactly (as Actor.Stun). The actors tick
	// before the timers in a frame, so what a timer opens is taken up by the next tick.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	// The player behind it, where it cannot see them.
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(-1000., 0., 500.), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	const FTransform At(FRotator::ZeroRotator, FVector(0., 0., 500.));
	AWasamiEnemy06Chase* Nurse = World->SpawnActorDeferred<AWasamiEnemy06Chase>(AWasamiEnemy06Chase::StaticClass(), At,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!TestNotNull(TEXT("a spawned nurse"), Nurse))
	{
		return false;
	}
	Nurse->bCanSpawn = true;
	Nurse->FinishSpawning(At);
	Nurse->GetCharacterMovement()->GravityScale = 0.f;
	const UWasamiEnemyAnimInstance* Anim = Nurse->GetEnemyAnim();
	if (!TestNotNull(TEXT("its animation"), Anim))
	{
		return false;
	}
	// Its own Audio, not the move loop every enemy has (Skate Audio).
	TArray<UAudioComponent*> Audios;
	Nurse->GetComponents(Audios);
	UAudioComponent** Found = Audios.FindByPredicate([](const UAudioComponent* C) { return C->GetName() == TEXT("Audio"); });
	const UAudioComponent* Audio = Found ? *Found : nullptr;
	TestTrue(TEXT("its slam"), Audio && Audio->Sound && Audio->Sound->GetName() == TEXT("20-Elevator_Slams"));
	TestTrue(TEXT("Chasing always answers true"), Nurse->IsChasing());
	TestFalse(TEXT("it does not see the player"), Nurse->CanSeePlayer());

	// Its tick chases the player, seen or not.
	TickTo(Step);
	TestTrue(TEXT("chasing from its first tick"), Nurse->bSeenPlayerRecently);
	TestTrue(TEXT("to where the player is"), Nurse->PointOfInterest.Equals(Player->GetActorLocation(), 1e-3));
	TestEqual(TEXT("at the chase's speed"), Nurse->GetCharacterMovement()->MaxWalkSpeed, AWasamiEnemy::MaxSpeed);
	Player->SetActorLocation(FVector(-1000., 500., 500.));
	TickTo(2 * Step);
	TestTrue(TEXT("every tick"), Nurse->PointOfInterest.Equals(Player->GetActorLocation(), 1e-3));

	// bAttackDoor: a stab from the next tick, Wasami's stand-in clip over the rest once it has blended in; the next
	// after the montage's end (0.7167 s) and 0 to 0.5 s.
	TestEqual(TEXT("no stab without bAttackDoor"), Nurse->GetDoorAttacks(), 0);
	Nurse->bAttackDoor = true;
	TickTo(3 * Step);
	TestEqual(TEXT("the tick stabs"), Nurse->GetDoorAttacks(), 1);
	TickTo(3 * Step + 0.25f);
	float ClipTime = 0.f;
	float ClipWeight = 0.f;
	TestEqual(TEXT("with Chase_Charge"), Anim->GetMainClip(ClipTime, ClipWeight), AWasamiEnemy06Chase::DoorAttackClip);
	const float Completed = 3 * Step + FMath::CeilToFloat(AWasamiEnemy06Chase::DoorAttackSeconds / Step) * Step;
	TickTo(Completed + Step);
	TestEqual(TEXT("one stab until the montage's end and the wait"), Nurse->GetDoorAttacks(), 1);
	TickTo(Completed + AWasamiEnemy06Chase::DoorAttackMaxWait + 2 * Step);
	TestEqual(TEXT("then the next"), Nurse->GetDoorAttacks(), 2);
	// Each stab takes 0.75 s to its end on these ticks, a wait of up to 0.5 s and a tick to start the next.
	const float From = Now;
	const int32 Before = Nurse->GetDoorAttacks();
	TickTo(From + 20.f);
	const int32 In20 = Nurse->GetDoorAttacks() - Before;
	TestTrue(FString::Printf(TEXT("%d stabs in 20 s"), In20), In20 >= 14 && In20 <= 26);
	TestTrue(TEXT("still chasing"), Nurse->PointOfInterest.Equals(Player->GetActorLocation(), 1e-3));
	Nurse->bAttackDoor = false;
	TickTo(Now + 1.5f);
	const int32 Stopped = Nurse->GetDoorAttacks();
	TickTo(Now + 3.f);
	TestEqual(TEXT("no stab once bAttackDoor is cleared"), Nurse->GetDoorAttacks(), Stopped);

	// Hit FX: the dust 230 cm in front of it at half size.
	Nurse->HitFX();
	const UParticleSystemComponent* Dust = nullptr;
	for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
	{
		if (It->GetWorld() == World && It->Template && It->Template->GetName() == TEXT("P_06_NurseDoorHit"))
		{
			Dust = *It;
		}
	}
	if (TestNotNull(TEXT("the dust"), Dust))
	{
		const FVector Front = Nurse->GetActorLocation() + Nurse->GetActorForwardVector() * AWasamiEnemy06Chase::HitFXForward;
		TestTrue(TEXT("in front of it"), Dust->GetComponentLocation().Equals(Front, 0.01));
		TestTrue(TEXT("at half size"), Dust->GetComponentScale().Equals(FVector(AWasamiEnemy06Chase::HitFXScale), 1e-4));
	}

	// A stun starts on its next tick and holds it for 17 s: no chase until then.
	IWasamiEnemyInterface::Execute_SetState(Nurse, EWasamiEnemyState::Stun, false);
	TestEqual(TEXT("17 s from its next tick"), Nurse->GetStunTimeLeft(), AWasamiEnemy::StunSeconds);
	TickTo(Now + Step);
	TestTrue(TEXT("its tick starts the stun"), Nurse->IsStunRunning());
	const float Stunned = Now;
	const FVector Seen = Nurse->PointOfInterest;
	Player->SetActorLocation(FVector(-1000., -500., 500.));
	TickTo(Stunned + 1.f);
	TestTrue(TEXT("no chase while stunned"), Nurse->PointOfInterest.Equals(Seen, 1e-3));
	TickTo(Stunned + AWasamiEnemy::StunSeconds);
	TestTrue(TEXT("stunned for 17 s"), Nurse->IsStunned());
	TickTo(Stunned + AWasamiEnemy::StunSeconds + Step);
	TestTrue(TEXT("then Patrol"), Nurse->GetCurrentState() == EWasamiEnemyState::Patrol);
	TickTo(Now + Step);
	TestTrue(TEXT("and chasing again"), Nurse->PointOfInterest.Equals(Player->GetActorLocation(), 1e-3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorChaseVariationTest, "Wasami.Enemy.Actor.ChaseVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorChaseVariationTest::RunTest(const FString& Parameters)
{
	// A game world without navigation data (as Actor.Choice): the engine's NavMesh ray is blocked by default, so no
	// chance ever finds its way ahead clear and each stands open. What plays is seen in PIE (the item 26's step 2).
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	const FVector InFront(1000., 0., 500.);
	ACharacter* Player = World->SpawnActor<ACharacter>(InFront, FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	AWasamiEnemy* Enemy = AWasamiEnemy::SpawnEnemy(World, FVector(0., 0., 500.), 0.f);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller)
		|| !TestNotNull(TEXT("a spawned enemy"), Enemy))
	{
		return false;
	}
	Controller->SetPawn(Player);
	Enemy->GetCharacterMovement()->GravityScale = 0.f;
	const UWasamiEnemyAnimInstance* Anim = Enemy->GetEnemyAnim();
	if (!TestNotNull(TEXT("its animation"), Anim))
	{
		return false;
	}
	TestTrue(TEXT("an enemy may play a variation"), Enemy->CanPlayChaseVariation());
	TestFalse(TEXT("no way ahead is clear without navigation"), Enemy->IsWayAheadClear(100.f));
	TestEqual(TEXT("no chances before the chase"), Enemy->GetChaseVariations().GetWait(), FWasamiChaseVariations::NotChasing);

	// The decisions chase from 1.125 s (Actor.Choice), so the first chance has come by 10 s after it. Nothing plays,
	// as nothing clears the way, and the chance stands open.
	TickTo(1.125f + FWasamiChaseVariations::MaxGap + AWasamiEnemy::DecisionInterval);
	TestTrue(TEXT("chasing"), Enemy->IsChasing());
	TestEqual(TEXT("a chance stands open"), Enemy->GetChaseVariations().GetWait(), 0.f);
	TestEqual(TEXT("with nothing played"), Enemy->GetChaseVariations().GetLast(), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("and nothing playing over the run"), Anim->IsPlayingOnce());

	// A stun drops the chances; the chase they start over with waits a whole gap.
	IWasamiEnemyInterface::Execute_SetState(Enemy, EWasamiEnemyState::Stun, false);
	TickTo(Now + AWasamiEnemy::DecisionInterval + Step);
	TestTrue(TEXT("stunned"), Enemy->IsStunRunning());
	TestEqual(TEXT("no chances while stunned"), Enemy->GetChaseVariations().GetWait(), FWasamiChaseVariations::NotChasing);

	// The parking lot's nurses take no chance while they stab at the doors.
	const FTransform At(FRotator::ZeroRotator, FVector(0., 500., 500.));
	AWasamiEnemy06Chase* Nurse = World->SpawnActorDeferred<AWasamiEnemy06Chase>(AWasamiEnemy06Chase::StaticClass(), At,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!TestNotNull(TEXT("a spawned nurse"), Nurse))
	{
		return false;
	}
	Nurse->bCanSpawn = true;
	Nurse->FinishSpawning(At);
	Nurse->GetCharacterMovement()->GravityScale = 0.f;
	TestTrue(TEXT("a nurse of the parking lot may"), Nurse->CanPlayChaseVariation());
	Nurse->bAttackDoor = true;
	TestFalse(TEXT("but not while it stabs at the doors"), Nurse->CanPlayChaseVariation());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorZone2Test, "Wasami.Enemy.Actor.Zone2",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorZone2Test::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	const FTransform At(FRotator::ZeroRotator, FVector(13000., -1300., 100.));
	AWasamiEnemyZone2* Nurse = World->SpawnActorDeferred<AWasamiEnemyZone2>(AWasamiEnemyZone2::StaticClass(), At,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("a spawned nurse"), Nurse))
	{
		return false;
	}
	Nurse->bCanSpawn = true;
	Nurse->FinishSpawning(At);
	Nurse->GetCharacterMovement()->GravityScale = 0.f;

	// Without a player, the player reads as on the lower floor.
	TestFalse(TEXT("no player: not up"), Nurse->IsPlayerUp());
	TestTrue(TEXT("so on its floor"), Nurse->IsSameLevelAsPlayer());

	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(13000., 0., 100.), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);
	Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	auto PlayerAt = [Player](double Z) { Player->SetActorLocation(FVector(13000., 0., Z), false, nullptr, ETeleportType::TeleportPhysics); };
	auto NurseAt = [Nurse](double Z) { Nurse->SetActorLocation(FVector(13000., -1300., Z), false, nullptr, ETeleportType::TeleportPhysics); };

	// is Up? over 640, is Player Up? over 610.
	NurseAt(640.);
	TestFalse(TEXT("640: not up"), Nurse->IsUp());
	NurseAt(640.1);
	TestTrue(TEXT("over 640: up"), Nurse->IsUp());
	PlayerAt(610.);
	TestFalse(TEXT("the player at 610: not up"), Nurse->IsPlayerUp());
	PlayerAt(610.1);
	TestTrue(TEXT("over 610: up"), Nurse->IsPlayerUp());
	TestTrue(TEXT("both up: the same floor"), Nurse->IsSameLevelAsPlayer());

	// The same floor: the player and the random point, as the nurse's.
	TestTrue(TEXT("the same floor: the player"), Nurse->GetPlayerTarget() == Player);
	TestTrue(TEXT("and the random point"), Nurse->GetRandomPointDestination().Equals(Nurse->RandomPoint));

	// Zone 2's lifts, where the level has two of them: the lift nearest the world's origin, however far from the nurse;
	// the corner lift (not a BP_06_LiftBase) is not one of them.
	AWasamiLift* Near = World->SpawnActor<AWasamiLift>(FVector(6303.2421875, -438.52606201171875, -30.01825714111328), FRotator::ZeroRotator);
	AWasamiLift* ByTheNurse = World->SpawnActor<AWasamiLift>(FVector(13499.5546875, -1325.2242431640625, -29.621246337890625), FRotator::ZeroRotator);
	World->SpawnActor<AWasamiCornerLift>(FVector(100., 0., -30.), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("a lift"), Near) || !TestNotNull(TEXT("another"), ByTheNurse))
	{
		return false;
	}
	NurseAt(100.);
	TestFalse(TEXT("the nurse down, the player up: not the same floor"), Nurse->IsSameLevelAsPlayer());
	FVector MoveLocation;
	TestTrue(TEXT("the closest lift is the one nearest the origin"), Nurse->GetClosestLift(MoveLocation) == Near);
	TestTrue(TEXT("Player Target: that lift"), Nurse->GetPlayerTarget() == Near);
	const FVector NearMove = Near->GetMoveLocation()->GetComponentLocation();
	TestTrue(TEXT("its Move Location"), MoveLocation.Equals(NearMove));
	TestTrue(TEXT("149.5 cm over the floor"), NearMove.Equals(Near->GetActorLocation() + FVector(0., 0., 149.53536987304688), 0.01));
	TestTrue(TEXT("Random Point Destination: its Move Location"), Nurse->GetRandomPointDestination().Equals(NearMove));
	NurseAt(700.);
	PlayerAt(100.);
	TestTrue(TEXT("the nurse up, the player down: the same lift"), Nurse->GetPlayerTarget() == Near);

	// Of lifts as far from the origin, the last.
	AWasamiLift* Twin = World->SpawnActor<AWasamiLift>(-Near->GetActorLocation(), FRotator::ZeroRotator);
	TestTrue(TEXT("the last of equals"), Nurse->GetClosestLift(MoveLocation) == Twin);

	// None within the first best (1e9, squared): no lift and no location.
	Near->Destroy();
	ByTheNurse->Destroy();
	Twin->Destroy();
	World->SpawnActor<AWasamiLift>(FVector(31623., 0., 0.), FRotator::ZeroRotator);
	TestNull(TEXT("a lift past 31622.8 cm is never taken"), Nurse->GetClosestLift(MoveLocation));
	TestTrue(TEXT("and there is no location"), MoveLocation.IsZero());
	TestNull(TEXT("nothing to chase to"), Nurse->GetPlayerTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiEnemyActorSentryTest, "Wasami.Enemy.Actor.Sentry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiEnemyActorSentryTest::RunTest(const FString& Parameters)
{
	// A game world ticked by hand as in Actor.Stun. Update Sight's rate is drawn from 0.3 to 0.5 s, and a timer comes up
	// to two ticks after its time, so the checks leave room on either side.
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	constexpr float Step = 0.0625f;
	float Now = 0.f;
	auto TickTo = [&Wrapper, &Now](float Time)
	{
		while (Now < Time - Step / 2.f)
		{
			Wrapper.TickTestWorld(Step);
			Now += Step;
		}
	};

	// The player (not possessed, so that it stays where it is put), behind the sentry.
	const FVector Behind(-1000., 0., 500.);
	ACharacter* Player = World->SpawnActor<ACharacter>(Behind, FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the player"), Player) || !TestNotNull(TEXT("a controller"), Controller))
	{
		return false;
	}
	Controller->SetPawn(Player);

	const FTransform At(FRotator::ZeroRotator, FVector(0., 0., 500.));
	AWasamiEnemySentry* Sentry = World->SpawnActorDeferred<AWasamiEnemySentry>(AWasamiEnemySentry::StaticClass(), At,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("a sentry"), Sentry))
	{
		return false;
	}
	Sentry->bCanSpawn = true;
	Sentry->Offset = 2.f;
	Sentry->FinishSpawning(At);
	Sentry->GetCharacterMovement()->GravityScale = 0.f;
	Sentry->GetJumpDownSpot()->SetRelativeLocation(FVector(658., 0., 0.));
	Sentry->bVarIdle = true;
	TestTrue(TEXT("its idle is the alert one"), Sentry->bAggressiveIdle);
	TestFalse(TEXT("not chasing"), Sentry->IsChasing());

	// Its cone: a BP_06_Miniboss_viewcone_Nurse on the capsule, 72 cm up and 20 degrees down.
	AWasamiViewcone* Cone = Sentry->GetViewcone();
	if (!TestNotNull(TEXT("its view cone"), Cone))
	{
		return false;
	}
	TestTrue(TEXT("the nurse's cone"), Cone->GetClass() == AWasamiViewconeNurse::StaticClass());
	TestTrue(TEXT("a child of the sentry"), Cone->GetParentActor() == Sentry);
	TestTrue(TEXT("72 cm over the capsule's centre"), Cone->GetActorLocation().Equals(FVector(0., 0., 572.), 1e-3));
	TestEqual(TEXT("20 degrees down"), Cone->GetActorRotation().Pitch, -20., 1e-3);
	TestEqual(TEXT("1500 cm long"), Cone->Length, 1500.f);
	TestEqual(TEXT("20 degrees wide"), Cone->Angle, 20.f);
	TestTrue(TEXT("turned on as it finishes initializing"), Cone->bAutoOn);
	TestFalse(TEXT("shown as play begins"), Cone->IsHidden());
	TestFalse(TEXT("not on the map, as in the original"), Cone->ActorHasTag(TEXT("dd_minimap")));
	TestTrue(TEXT("its marks in the captures only"), Cone->GetPlane()->bVisibleInSceneCaptureOnly && Cone->GetDot()->bVisibleInSceneCaptureOnly);
	TestTrue(TEXT("its fan along the cone"), Cone->GetPlane()->GetRelativeLocation().Equals(AWasamiViewconeNurse::NursePlaneLocation)
		&& Cone->GetPlane()->GetRelativeScale3D().Equals(AWasamiViewconeNurse::NursePlaneScale));
	TickTo(0.125f);
	TestTrue(TEXT("lifted 1000 cm the next tick"),
		Cone->GetPlane()->GetRelativeLocation().Equals(AWasamiViewconeNurse::NursePlaneLocation + AWasamiViewcone::PlaneLift));
	TestTrue(TEXT("with the dot"), Cone->GetDot()->GetRelativeLocation().Equals(AWasamiViewcone::DotLocation));

	// Its BeginPlay is empty: it does not decide (Not Seeing Player would walk at 200).
	TickTo(1.f);
	TestFalse(TEXT("not looking before Activate"), Cone->IsInitialized());
	TestEqual(TEXT("not deciding"), Sentry->GetCharacterMovement()->MaxWalkSpeed, 430.f);

	// Activate at 1 s: the first Update Sight (0.3 to 0.5 s on) waits the 2 s Offset, then turns the cone on, which sees
	// a second later and fades in over 0.5 s.
	Sentry->Activate();
	TestTrue(TEXT("Activate starts it looking"), Cone->IsInitialized());
	TestEqual(TEXT("with the sentry's Offset"), Cone->Offset, 2.f);
	TickTo(3.3f);
	TestEqual(TEXT("waiting the Offset"), Cone->Offset, 2.f);
	TestTrue(TEXT("not looking yet"), Sentry->bVarIdle);
	TickTo(3.8f);
	TestEqual(TEXT("then Offset 0"), Cone->Offset, 0.f);
	TestFalse(TEXT("Start Looking"), Sentry->bVarIdle);
	TestFalse(TEXT("not on yet"), Cone->IsOn());
	TestEqual(TEXT("not shown yet"), Cone->GetFade(), 0.f);
	TickTo(4.3f);
	TestFalse(TEXT("a second on"), Cone->IsOn());
	TickTo(4.8f);
	TestTrue(TEXT("on"), Cone->IsOn());
	TickTo(5.3f);
	TestEqual(TEXT("faded in"), Cone->GetFade(), 1.f);

	// Every 10 s from the end of the Offset it turns: off, then on (seeing a second later).
	TickTo(13.3f);
	TestTrue(TEXT("on for 10 s"), Cone->IsOn());
	TickTo(13.8f);
	TestFalse(TEXT("then off"), Cone->IsOn());
	TestTrue(TEXT("Stop Looking"), Sentry->bVarIdle);
	TickTo(14.3f);
	TestEqual(TEXT("faded out"), Cone->GetFade(), 0.f);
	TickTo(23.8f);
	TestFalse(TEXT("Start Looking 10 s on"), Sentry->bVarIdle);
	TestFalse(TEXT("blind for a second"), Cone->IsOn());
	TickTo(24.85f);
	TestTrue(TEXT("on again"), Cone->IsOn());

	// The cone's sight, from 72 cm up and 20 degrees down.
	const FVector Eye = Cone->GetActorLocation();
	auto PutAt = [Player, Eye](double Pitch, double Distance)
	{
		Player->SetActorLocation(Eye + FRotator(Pitch, 0., 0.).Vector() * Distance);
	};
	PutAt(-20., 1000.);
	TestTrue(TEXT("inside on its axis"), Cone->PlayerInsideCone());
	TestTrue(TEXT("in full view, past the sentry's own capsule"), Cone->PlayerInFullView());
	PutAt(-1., 1000.);
	TestTrue(TEXT("19 degrees off"), Cone->PlayerInsideCone());
	PutAt(1., 1000.);
	TestFalse(TEXT("not 21"), Cone->PlayerInsideCone());
	TestFalse(TEXT("so not in full view"), Cone->PlayerInFullView());
	PutAt(-20., 1499.);
	TestTrue(TEXT("1499 cm"), Cone->PlayerInFullView());
	PutAt(-20., 1501.);
	TestFalse(TEXT("not 1501"), Cone->PlayerInsideCone());
	PutAt(-20., 1000.);
	ACharacter* Between = World->SpawnActor<ACharacter>(Eye + FRotator(-20., 0., 0.).Vector() * 500., FRotator::ZeroRotator);
	TestFalse(TEXT("not past another character"), Cone->PlayerInFullView());
	TestTrue(TEXT("though inside"), Cone->PlayerInsideCone());
	Between->Destroy();
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	TestFalse(TEXT("not while the player vanishes"), Cone->PlayerInFullView());
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

	// Seen at the next Update Sight (within 0.5 s): the cone is gone, it chases for good and leaps toward Jump Down Spot,
	// then decides as a nurse, from half a second on.
	const TWeakObjectPtr<AWasamiViewcone> WeakCone(Cone);
	const float Placed = Now;
	while (Sentry->GetViewcone() && Now < Placed + 1.f)
	{
		TickTo(Now + Step);
	}
	const float Spotted = Now;
	TestTrue(TEXT("spotted within the sight's rate"), Spotted <= Placed + AWasamiViewcone::SightRateMax + Step);
	TestTrue(TEXT("its cone destroyed"), !WeakCone.IsValid() || WeakCone->IsActorBeingDestroyed());
	TestNull(TEXT("and gone from it"), Sentry->GetViewcone());
	TestTrue(TEXT("chasing"), Sentry->IsChasing());
	TickTo(Spotted + 2.f * Step);
	TestTrue(TEXT("leaping 400 cm/s toward the spot and 500 up"), Sentry->GetCharacterMovement()->Velocity.Equals(FVector(400., 0., 500.), 1.));
	TestEqual(TEXT("not deciding yet"), Sentry->GetCharacterMovement()->MaxWalkSpeed, 430.f);
	TickTo(Spotted + AWasamiEnemy::DecisionInterval + 2.f * Step);
	TestEqual(TEXT("its first decision walks"), Sentry->GetCharacterMovement()->MaxWalkSpeed, 200.f);
	TickTo(Spotted + 2.f * AWasamiEnemy::DecisionInterval + 2.f * Step);
	TestEqual(TEXT("the next chases"), Sentry->GetCharacterMovement()->MaxWalkSpeed, 430.f);
	IWasamiEnemyInterface::Execute_PlayerVanish(Sentry);
	TestTrue(TEXT("Chasing is its own, whatever the nurse forgets"), Sentry->IsChasing());
	return true;
}

#endif
