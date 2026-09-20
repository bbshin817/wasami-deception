#include "Misc/AutomationTest.h"
#include "../WasamiBierceTalk.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Tests/AutomationCommon.h"
#include "WasamiTestBierceTalk.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The ticks' length. */
	constexpr float TalkTick = 0.01f;

	void AdvanceTalkWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= TalkTick)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, TalkTick));
		}
	}

	/** Names what an entry did, so a wrong one reads as what it was. */
	FString TalkStepName(EWasamiTalkStep Step)
	{
		switch (Step)
		{
		case EWasamiTalkStep::Play: return TEXT("played");
		case EWasamiTalkStep::Wait: return TEXT("started a wait");
		case EWasamiTalkStep::AlreadyWaiting: return TEXT("left the wait running");
		default: return TEXT("did nothing");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiTalkStepTest, "Wasami.Dialogue.TalkStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiTalkStepTest::RunTest(const FString& Parameters)
{
	auto Check = [this](const TCHAR* What, EWasamiTalkStep Got, EWasamiTalkStep Want)
	{
		TestTrue(FString(What) + TEXT(": it ") + TalkStepName(Got), Got == Want);
	};

	// Halt ends the event before anything else is looked at.
	Check(TEXT("halted, nothing is said"), WasamiTalkStep(true, false, false), EWasamiTalkStep::Nothing);
	Check(TEXT("halted while a line is going, still nothing"), WasamiTalkStep(true, true, false),
		EWasamiTalkStep::Nothing);
	// A free component plays at once; a busy one is waited on, half a second at a time.
	Check(TEXT("a free component plays"), WasamiTalkStep(false, false, false), EWasamiTalkStep::Play);
	Check(TEXT("a busy one is waited on"), WasamiTalkStep(false, true, false), EWasamiTalkStep::Wait);
	Check(TEXT("a wait already running is left alone"), WasamiTalkStep(false, true, true),
		EWasamiTalkStep::AlreadyWaiting);
	// The wait's own turn: it comes back inside the loop, past the Halt check, and plays once the line has ended.
	Check(TEXT("the wait's turn plays once the line has ended"), WasamiTalkStep(false, false, true),
		EWasamiTalkStep::Play);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiBierceTalkActorTest, "Wasami.Dialogue.Talk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiBierceTalkActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiTestBierceTalk* Talker = World->SpawnActor<AWasamiTestBierceTalk>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the talker"), Talker))
	{
		return false;
	}
	UAudioComponent* Audio = Talker->GetAudioComponent();
	if (!TestNotNull(TEXT("its audio component"), Audio))
	{
		return false;
	}

	// The component the original's AmbientSound parent makes: the actor's root, starting itself off, and playing
	// through DialogueAttenuation once play has begun.
	TestTrue(TEXT("the component is the root"), Audio == Talker->GetRootComponent());
	TestFalse(TEXT("it does not start itself"), Audio->bAutoActivate);
	TestTrue(TEXT("it plays through DialogueAttenuation"), Audio->AttenuationSettings
		&& Audio->AttenuationSettings->GetName() == TEXT("DialogueAttenuation"));
	TestNull(TEXT("no line yet"), ToRawPtr(Audio->Sound));

	// Talk on a free component plays there and then, and Attenuate? reaches bAllowSpatialization (the hospital's calls
	// are all False, so its lines are heard the same everywhere).
	USoundWave* First = NewObject<USoundWave>();
	Talker->Talk(First, false);
	TestTrue(TEXT("the first line plays: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::Play);
	TestTrue(TEXT("it is on the component"), Audio->Sound == First);
	TestFalse(TEXT("Attenuate? False is not spatialized"), Audio->bAllowSpatialization != 0);
	TestFalse(TEXT("nothing is waiting"), Talker->IsWaiting());

	USoundWave* Second = NewObject<USoundWave>();
	Talker->Talk(Second, true);
	TestTrue(TEXT("Attenuate? True is"), Audio->bAllowSpatialization != 0);
	TestTrue(TEXT("and the line is on the component"), Audio->Sound == Second);

	// A line still going: the next Talk waits instead of cutting it short.
	Talker->bSpeaking = true;
	USoundWave* Third = NewObject<USoundWave>();
	Talker->Talk(Third, false);
	TestTrue(TEXT("a line still going is waited on: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::Wait);
	TestTrue(TEXT("the wait is running"), Talker->IsWaiting());
	TestTrue(TEXT("the line going is not cut short"), Audio->Sound == Second);

	// Another Talk while the wait runs leaves it alone — and the wait will play this one, not the one it started for.
	USoundWave* Fourth = NewObject<USoundWave>();
	Talker->Talk(Fourth, false);
	TestTrue(TEXT("a wait already running is left alone: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::AlreadyWaiting);
	TestTrue(TEXT("what it will play is the last one asked for"), Talker->GetPendingSound() == Fourth);

	// Stop Talking is the component's Stop alone: it leaves the wait running, as the original does.
	Talker->StopTalking();
	TestTrue(TEXT("Stop Talking leaves the wait running"), Talker->IsWaiting());

	// Half a second later the line is still going, so it waits again; and the turn after its end plays.
	AdvanceTalkWorld(Wrapper, 0.55f);
	TestTrue(TEXT("the wait's turn waits again while the line goes on: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::Wait);
	TestTrue(TEXT("the line going is still not cut short"), Audio->Sound == Second);
	Talker->bSpeaking = false;
	AdvanceTalkWorld(Wrapper, 0.55f);
	TestTrue(TEXT("the turn after its end plays: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::Play);
	TestTrue(TEXT("what it plays is the last one asked for"), Audio->Sound == Fourth);
	TestFalse(TEXT("and nothing is waiting any more"), Talker->IsWaiting());

	// Halt: Talk says nothing, but Attenuate? and What To Say still reach the actor (the original writes both first).
	Talker->bHalt = true;
	USoundWave* Halted = NewObject<USoundWave>();
	Talker->Talk(Halted, true);
	TestTrue(TEXT("halted, nothing is said: it ") + TalkStepName(Talker->GetLastStep()),
		Talker->GetLastStep() == EWasamiTalkStep::Nothing);
	TestTrue(TEXT("the line on the component is the one before"), Audio->Sound == Fourth);
	TestTrue(TEXT("Attenuate? reached the component all the same"), Audio->bAllowSpatialization != 0);
	TestTrue(TEXT("and so did What To Say"), Talker->GetPendingSound() == Halted);

	// BP_DD_Functions' Bierce Talk: the first talker in the world.
	TestTrue(TEXT("Find is the talker in the world"), AWasamiBierceTalk::Find(World) == Talker);
	Wrapper.ForwardErrorMessages(this);
	return true;
}

#endif
