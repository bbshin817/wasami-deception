#include "Misc/AutomationTest.h"
#include "../WasamiMusicPlayer.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationCommon.h"
#include "WasamiTestEnemy.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** The ticks' length. */
	constexpr float MusicStep = 0.01f;

	void AdvanceMusicWorld(FTestWorldWrapper& Wrapper, float Seconds)
	{
		for (float Left = Seconds; Left > 1e-4f; Left -= MusicStep)
		{
			Wrapper.TickTestWorld(FMath::Min(Left, MusicStep));
		}
	}

	FString MusicFadeName(EWasamiMusicFade Fade)
	{
		switch (Fade)
		{
		case EWasamiMusicFade::In: return TEXT("in");
		case EWasamiMusicFade::Out: return TEXT("out");
		case EWasamiMusicFade::OutIfPlaying: return TEXT("out if playing");
		default: return TEXT("nothing");
		}
	}

	/** Names what a turn faded, so a wrong one reads as what it did. */
	FString MusicFadesName(const FWasamiMusicFades& Fades)
	{
		return FString::Printf(TEXT("regular %s, panic %s, override %s"), *MusicFadeName(Fades.Regular),
			*MusicFadeName(Fades.Panic), *MusicFadeName(Fades.Override));
	}

	bool SameMusicFades(const FWasamiMusicFades& A, const FWasamiMusicFades& B)
	{
		return A.Regular == B.Regular && A.Panic == B.Panic && A.Override == B.Override;
	}

	const FWasamiMusicFades RegularIn{EWasamiMusicFade::In, EWasamiMusicFade::Out, EWasamiMusicFade::None};
	const FWasamiMusicFades PanicIn{EWasamiMusicFade::Out, EWasamiMusicFade::In, EWasamiMusicFade::None};
	const FWasamiMusicFades AllOut{EWasamiMusicFade::OutIfPlaying, EWasamiMusicFade::OutIfPlaying,
		EWasamiMusicFade::OutIfPlaying};
	const FWasamiMusicFades Silent;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMusicStateTest, "Wasami.Music.State",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMusicStateTest::RunTest(const FString& Parameters)
{
	auto Check = [this](const TCHAR* What, const FWasamiMusicFades& Got, const FWasamiMusicFades& Want)
	{
		TestTrue(FString(What) + TEXT(": ") + MusicFadesName(Got), SameMusicFades(Got, Want));
	};

	// A zone with nothing chasing: the regular track comes in on the first turn, and the turns after change nothing.
	FWasamiMusicState State;
	Check(TEXT("the first turn brings the regular music in"), State.Update(false, false, false), RegularIn);
	Check(TEXT("nothing changes on the next turn"), State.Update(false, false, false), Silent);
	Check(TEXT("nor on the one after"), State.Update(false, false, false), Silent);

	// A chase and its end, each crossfading once.
	Check(TEXT("a chase brings the panic music in"), State.Update(false, false, true), PanicIn);
	Check(TEXT("the chase holds it"), State.Update(false, false, true), Silent);
	Check(TEXT("its end brings the regular music back"), State.Update(false, false, false), RegularIn);
	Check(TEXT("and holds"), State.Update(false, false, false), Silent);

	// bFadeOut: everything out, once, and nothing while it stays on — not even a chase beginning.
	Check(TEXT("bFadeOut takes all three out"), State.Update(true, false, false), AllOut);
	Check(TEXT("and stays quiet"), State.Update(true, false, false), Silent);
	Check(TEXT("a chase while faded out is not heard"), State.Update(true, false, true), Silent);
	// Dropping it does not bring the music back by itself: the pair's DoOnce is still closed on the state it left
	// (the original's; Zone 2's flow fades its regular music in itself after the cell scene).
	Check(TEXT("dropping bFadeOut in the same state brings nothing back"), State.Update(false, false, false), Silent);
	Check(TEXT("but a chase does"), State.Update(false, false, true), PanicIn);
	Check(TEXT("and its end"), State.Update(false, false, false), RegularIn);

	// bOverrideMusic: the override track alone, and the chase is not looked at while it is on.
	const FWasamiMusicFades OverrideIn{EWasamiMusicFade::Out, EWasamiMusicFade::Out, EWasamiMusicFade::In};
	Check(TEXT("bOverrideMusic brings the override track in"), State.Update(false, true, false), OverrideIn);
	Check(TEXT("and holds"), State.Update(false, true, true), Silent);
	const FWasamiMusicFades OverrideOut{EWasamiMusicFade::None, EWasamiMusicFade::None, EWasamiMusicFade::Out};
	Check(TEXT("dropping it takes the override track out"), State.Update(false, false, false), OverrideOut);
	Check(TEXT("and the chase brings the panic music in again"), State.Update(false, false, true), PanicIn);

	// A player that starts faded out (Zone 1's, placed with bFadeOut true) is silent until the flow drops it.
	FWasamiMusicState Zone1;
	Check(TEXT("the level's start with bFadeOut on fades the silence out"), Zone1.Update(true, false, false), AllOut);
	Check(TEXT("the flow dropping it starts the regular music"), Zone1.Update(false, false, false), RegularIn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiMusicActorTest, "Wasami.Music.Actor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiMusicActorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiMusicPlayer* Player = World->SpawnActor<AWasamiMusicPlayer>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("the music player"), Player))
	{
		return false;
	}

	// The three components: on the root, none of them starting itself, the hospital's tracks on two of them.
	const UAudioComponent* Regular = Player->GetRegularMusic();
	const UAudioComponent* Panic = Player->GetPanicMusic();
	const UAudioComponent* Override = Player->GetOverrideMusic();
	TestTrue(TEXT("all three on the root"), Regular->GetAttachParent() == Player->GetRootComponent()
		&& Panic->GetAttachParent() == Player->GetRootComponent()
		&& Override->GetAttachParent() == Player->GetRootComponent());
	TestTrue(TEXT("none starts itself"), !Regular->bAutoActivate && !Panic->bAutoActivate && !Override->bAutoActivate);
	TestTrue(TEXT("Zone 1's regular track"), Regular->Sound
		&& Regular->Sound->GetName() == TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING"));
	TestTrue(TEXT("the panic track"), Panic->Sound
		&& Panic->Sound->GetName() == TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING"));
	TestNull(TEXT("the override track is empty, as the original's"), ToRawPtr(Override->Sound));
	TestTrue(TEXT("nothing is playing yet"), !Regular->IsPlaying() && !Panic->IsPlaying());

	// The looping timer: the first Update comes half a second in, and the one after changes nothing.
	AdvanceMusicWorld(Wrapper, 0.55f);
	TestTrue(TEXT("the first Update brings the regular music in: ") + MusicFadesName(Player->GetLastFades()),
		SameMusicFades(Player->GetLastFades(), RegularIn));
	AdvanceMusicWorld(Wrapper, 0.55f);
	TestTrue(TEXT("the next changes nothing"), Player->GetLastFades().IsSilent());

	// Intense Music ?: an enemy of the interface that is chasing.
	AWasamiTestEnemy* Enemy = AWasamiTestEnemy::SpawnTestEnemy(World, FVector(500., 0., 0.));
	if (!TestNotNull(TEXT("the test enemy"), Enemy))
	{
		return false;
	}
	TestFalse(TEXT("an enemy that is not chasing is not intense"), Player->IsIntenseMusic());
	Enemy->bChasing = true;
	TestTrue(TEXT("one that is, is"), Player->IsIntenseMusic());
	Player->Update();
	TestTrue(TEXT("the chase brings the panic music in: ") + MusicFadesName(Player->GetLastFades()),
		SameMusicFades(Player->GetLastFades(), PanicIn));
	Enemy->bChasing = false;
	Player->Update();
	TestTrue(TEXT("its end brings the regular music back"), SameMusicFades(Player->GetLastFades(), RegularIn));

	// bFadeOut, as the zones' flow sets it: out once, and nothing until something changes.
	Player->bFadeOut = true;
	Player->Update();
	TestTrue(TEXT("bFadeOut takes all three out"), SameMusicFades(Player->GetLastFades(), AllOut));
	Player->bFadeOut = false;
	Player->Update();
	TestTrue(TEXT("dropping it in the same state brings nothing back"), Player->GetLastFades().IsSilent());

	// Zone 2's player: the same but for its regular track.
	const AWasamiMusicPlayerZone2* Zone2 = World->SpawnActor<AWasamiMusicPlayerZone2>(FVector(1000., 0., 0.),
		FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Zone 2's music player"), Zone2))
	{
		TestTrue(TEXT("Zone 2's regular track"), Zone2->GetRegularMusic()->Sound
			&& Zone2->GetRegularMusic()->Sound->GetName()
				== TEXT("DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING"));
		TestTrue(TEXT("and the same panic track"), Zone2->GetPanicMusic()->Sound == Player->GetPanicMusic()->Sound);
	}
	Wrapper.ForwardErrorMessages(this);
	return true;
}

#endif
