#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "../WasamiEnemy.h"
#include "../WasamiEnemyAnimInstance.h"
#include "../WasamiPowerTypes.h"
#include "../WasamiPrimalPower.h"
#include "../WasamiTelepathyPower.h"
#include "../WasamiTelepathyTracker.h"
#include "../WasamiVanishPower.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The size the enemy draws its mesh at, which the clips' strides grow with. */
	constexpr float Grown = static_cast<float>(AWasamiEnemy::MeshScale);

	/** The imported clips' lengths (30 fps frames, implementation record 07). */
	TArray<float> ImportedLengths()
	{
		const int32 Frames[WasamiEnemyClip::Num] = {58, 57, 31, 20, 18, 45, 227, 64, 83, 106, 37, 16, 63, 72, 55, 53, 46, 75, 93};
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

	// The patrol's 350 cm/s: Idle → Moving over 0.5 s (Sinusoidal), and a walk.
	Inputs.Speed = 350.f;
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("half way into moving"), State.GetClipWeight(WasamiEnemyClip::Walk), 0.5f, 1e-5f);
	TestEqual(TEXT("half the idle left"), State.GetClipWeight(WasamiEnemyClip::Idle), 0.5f, 1e-5f);
	const float WalkRate = 350.f / (133.f * Grown);
	TestEqual(TEXT("the walk's rate follows the speed and the grown stride"), State.GetClipRate(WasamiEnemyClip::Walk), WalkRate);
	TestEqual(TEXT("the walk started from 0"), State.GetClipTime(WasamiEnemyClip::Walk), 0.25f * WalkRate, 1e-5f);
	TestEqual(TEXT("the idle moves on while it blends out"), State.GetClipTime(WasamiEnemyClip::Idle), 0.35f);
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("walking"), State.GetClipWeight(WasamiEnemyClip::Walk), 1.f);
	TestEqual(TEXT("the idle stops once it has no weight"), State.GetClipTime(WasamiEnemyClip::Idle), 0.35f);

	// The chase's 800 cm/s: above 400 the run blends in over 0.25 s.
	Inputs.Speed = 800.f;
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("half walk"), State.GetClipWeight(WasamiEnemyClip::Walk), 0.5f);
	TestEqual(TEXT("half run"), State.GetClipWeight(WasamiEnemyClip::Run), 0.5f);
	const float ChaseRate = 800.f / (450.f * Grown);
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
	const float Loop = Lengths[WasamiEnemyClip::StunLoop];
	const float Recover = Lengths[WasamiEnemyClip::StunRecover];

	// The 17 s stun: the loop's end comes round when the recovery starts, 17 − 7.567 s in.
	FWasamiStunPlayback Playback;
	Playback.Start(17.f, Loop, Recover);
	TestEqual(TEXT("the recovery starts so that it ends with the stun"), Playback.RecoverStart, 17.f - Recover, 1e-4f);
	TestEqual(TEXT("the loop starts at the phase that ends it then"), Playback.LoopStart, 1.5f - FMath::Fmod(17.f - Recover, 1.5f), 1e-4f);
	const float Phase = FMath::Fmod(Playback.LoopStart + Playback.RecoverStart, Loop);
	TestTrue(TEXT("which is a whole number of loops before"), Phase < 1e-4f || Loop - Phase < 1e-4f);
	TestFalse(TEXT("it starts in the loop"), Playback.IsRecovering());
	TestEqual(TEXT("at that phase"), Playback.GetClipTime(), Playback.LoopStart, 1e-5f);
	Playback.Advance(Playback.RecoverStart - 0.01f);
	TestFalse(TEXT("still looping just before"), Playback.IsRecovering());
	TestEqual(TEXT("near the loop's end"), Playback.GetClipTime(), Loop - 0.01f, 1e-3f);
	Playback.Advance(0.02f);
	TestTrue(TEXT("recovering just after"), Playback.IsRecovering());
	TestEqual(TEXT("from the recovery's start"), Playback.GetClipTime(), 0.01f, 1e-3f);
	Playback.Advance(17.f);
	TestEqual(TEXT("the recovery holds its end"), Playback.GetClipTime(), Recover);

	FWasamiStunPlayback Short;
	Short.Start(5.f, Loop, Recover);
	TestTrue(TEXT("a stun shorter than the recovery recovers at once"), Short.IsRecovering());
	TestEqual(TEXT("from part way through"), Short.GetClipTime(), Recover - 5.f, 1e-5f);
	Short.Advance(5.f);
	TestEqual(TEXT("and ends with the stun"), Short.GetClipTime(), Recover, 1e-5f);

	// In the state: running, then Primal Fear.
	FWasamiEnemyAnimState State;
	State.Init(Lengths);
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
	TestFalse(TEXT("the stun stops what plays once"), State.IsPlayingOnce());
	TestEqual(TEXT("the stun blends in over 0.25 s"), State.Stun.Weight, 0.5f);
	TestEqual(TEXT("the loop moved on from its phase"), State.StunPlayback.GetClipTime(), State.StunPlayback.LoopStart + 0.125f, 1e-5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the variation blends out over 0.25 s"), State.GetClipWeight(WasamiEnemyClip::ChaseSlide), 0.5f);
	TestEqual(TEXT("under it, the stun's loop"), State.GetClipWeight(WasamiEnemyClip::StunLoop), 0.5f);
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("stunned"), State.GetClipWeight(WasamiEnemyClip::StunLoop), 1.f);
	TestEqual(TEXT("the locomotion, under the stun, went to idle"), State.Moving.WeightB, 0.f);

	for (int32 Step = 0; Step < 36; ++Step)
	{
		State.Update(Inputs, 0.25f);
	}
	TestEqual(TEXT("9.375 s in, still the loop"), State.GetClipWeight(WasamiEnemyClip::StunLoop), 1.f);
	State.Update(Inputs, 0.25f);
	TestEqual(TEXT("9.625 s in, the recovery"), State.GetClipWeight(WasamiEnemyClip::StunRecover), 1.f);
	TestEqual(TEXT("from its start"), State.GetClipTime(WasamiEnemyClip::StunRecover), 9.625f - (17.f - Recover), 1e-4f);
	for (int32 Step = 0; Step < 30; ++Step)
	{
		State.Update(Inputs, 0.25f);
	}
	TestEqual(TEXT("17.125 s in, the recovery holds its end"), State.GetClipTime(WasamiEnemyClip::StunRecover), Recover);

	Inputs.bStunned = false;
	State.Update(Inputs, 0.125f);
	TestEqual(TEXT("the stun blends out over 0.25 s"), State.GetClipWeight(WasamiEnemyClip::StunRecover), 0.5f);
	TestEqual(TEXT("to the idle"), State.GetClipWeight(WasamiEnemyClip::Idle), 0.5f);
	TestEqual(TEXT("the weights add up to 1"), SampleWeightSum(State), 1.f, 1e-5f);

	// A second stun while the first blends out starts over.
	Inputs.bStunned = true;
	State.Update(Inputs, 0.0625f);
	TestFalse(TEXT("a new stun loops again"), State.StunPlayback.IsRecovering());
	TestEqual(TEXT("its weight goes back up from where it was"), State.Stun.Weight, 0.75f);
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
	TestEqual(TEXT("Normal Speed"), Enemy->NormalSpeed, 350.f);
	TestEqual(TEXT("Skate Speed"), Enemy->SkateSpeed, 800.f);

	const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
	TestEqual(TEXT("the capsule's radius"), Capsule->GetUnscaledCapsuleRadius(), 34.f);
	TestEqual(TEXT("the capsule's half height"), Capsule->GetUnscaledCapsuleHalfHeight(), 118.05822f, 1e-4f);
	TestTrue(TEXT("a pawn's collision"), Capsule->GetCollisionObjectType() == ECC_Pawn);

	const UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement();
	TestEqual(TEXT("the top speed"), Movement->MaxWalkSpeed, 800.f);
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
	const float Recover = Anim->GetAnimState().GetLength(WasamiEnemyClip::StunRecover);
	TestEqual(TEXT("the recovery is loaded"), Recover, 227 / 30.f, 1e-3f);
	TestTrue(TEXT("Get State reads Patrol"), IWasamiEnemyInterface::Execute_GetState(Enemy) == EWasamiEnemyState::Patrol);
	TestFalse(TEXT("it shows in the telepathy"), IWasamiEnemyInterface::Execute_NoTelepathy(Enemy));
	Enemy->SetWalkState(true);
	TestEqual(TEXT("the walk's speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 350.f);
	Enemy->SetWalkState(false);
	TestEqual(TEXT("the skate's speed"), Enemy->GetCharacterMovement()->MaxWalkSpeed, 800.f);

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
	TestEqual(TEXT("for what is left of the stun"), Anim->GetAnimState().StunPlayback.RecoverStart, 17.3125f - Recover, 1e-4f);
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

	TickTo(17.5625f);
	TestTrue(TEXT("still stunned"), Enemy->IsStunned());
	TestTrue(TEXT("recovering"), Anim->GetAnimState().StunPlayback.IsRecovering());
	TestEqual(TEXT("one step before the recovery's end"), Anim->GetAnimState().StunPlayback.GetClipTime(), Recover - Step, 1e-3f);
	TickTo(17.625f);
	TestTrue(TEXT("stunned while the wait is due"), Enemy->IsStunRunning());
	TestEqual(TEXT("the recovery ends with the wait"), Anim->GetAnimState().StunPlayback.GetClipTime(), Recover, 1e-3f);
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

#endif
