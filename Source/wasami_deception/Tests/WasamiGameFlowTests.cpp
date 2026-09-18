#include "Misc/AutomationTest.h"
#include "../WasamiBlackFadeWidget.h"
#include "../WasamiDeathScreenWidget.h"
#include "../WasamiGameInstance.h"
#include "../WasamiGameMode.h"
#include "../WasamiLoadingWidget.h"
#include "../WasamiSaveGame.h"
#include "../WasamiSavingWidget.h"
#include "../WasamiShard.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString TestSlotName(TEXT("WasamiTest_structSlot"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowLivesTest, "Wasami.GameFlow.Lives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowLivesTest::RunTest(const FString& Parameters)
{
	UWasamiGameInstance* Instance = NewObject<UWasamiGameInstance>();
	TestEqual(TEXT("3 lives to start with"), Instance->GetLives(), 3);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Instance->DecrementLives();
	}
	TestEqual(TEXT("never below 0"), Instance->GetLives(), 0);
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Instance->IncrementLives();
	}
	TestEqual(TEXT("never above 6"), Instance->GetLives(), 6);
	Instance->ResetLives();
	TestEqual(TEXT("3 after Reset Lives"), Instance->GetLives(), 3);

	// Shards To Be Removed: truncated toward zero, each place once.
	TestTrue(TEXT("truncated toward zero"), UWasamiGameInstance::ShardKey(FVector(100.7, -200.3, -5.9)).Equals(FVector(100., -200., -5.), 0.));
	Instance->RememberCollectedShard(FVector(100.7, -200.3, 5.9));
	Instance->RememberCollectedShard(FVector(100.2, -200.9, 5.1));
	Instance->RememberCollectedShard(FVector(300., 0., 0.));
	TestEqual(TEXT("the same whole centimetres once"), Instance->GetShardsToBeRemoved().Num(), 2);
	Instance->ForgetCollectedShards();
	TestEqual(TEXT("forgotten"), Instance->GetShardsToBeRemoved().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowSaveTest, "Wasami.GameFlow.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowSaveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the original's slot"), UWasamiSaveGame::SlotName, FString(TEXT("structSlot")));
	UWasamiSaveGame* Save = NewObject<UWasamiSaveGame>();
	TestEqual(TEXT("no checkpoint in a new save"), Save->Hospital.LevelCheckpoint, 0);
	Save->Hospital.LevelCheckpoint = 9;
	Save->Hospital.Deaths = 4;
	Save->Hospital.Time = 321.5f;
	Save->Hospital.CurrentStreak = 12;
	Save->Hospital.Streak = 2;
	Save->bLastCheckpointWarning = true;

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("saved"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	const UWasamiSaveGame* Loaded = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("loaded"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("the checkpoint"), Loaded->Hospital.LevelCheckpoint, 9);
	TestEqual(TEXT("the deaths"), Loaded->Hospital.Deaths, 4);
	TestEqual(TEXT("the time"), Loaded->Hospital.Time, 321.5f);
	TestEqual(TEXT("the current streak"), Loaded->Hospital.CurrentStreak, 12);
	TestEqual(TEXT("the streak's rank"), static_cast<int32>(Loaded->Hospital.Streak), 2);
	TestTrue(TEXT("the warning"), Loaded->bLastCheckpointWarning);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowRemoveShardsTest, "Wasami.GameFlow.RemoveShards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowRemoveShardsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	const FVector Places[] = {FVector(100.7, -200.3, 5.9), FVector(-1000.2, 50., 0.), FVector(4000., 4000., 10.)};
	TArray<AWasamiShard*> Shards;
	for (const FVector& Place : Places)
	{
		Shards.Add(World->SpawnActor<AWasamiShard>(Place, FRotator::ZeroRotator));
	}
	if (!TestEqual(TEXT("three shards"), Shards.Num(), 3) || !TestNotNull(TEXT("each spawned"), Shards[2]))
	{
		return false;
	}

	// The keys a collect would have remembered, the second one twice off by a fraction.
	const TArray<FVector> Collected = {UWasamiGameInstance::ShardKey(Places[0]), UWasamiGameInstance::ShardKey(FVector(-1000.9, 50.4, 0.3))};
	TestEqual(TEXT("one left"), AWasamiGameMode::RemoveCollectedShards(World, Collected), 1);
	TestFalse(TEXT("the first gone"), IsValid(Shards[0]));
	TestFalse(TEXT("the second gone"), IsValid(Shards[1]));
	TestTrue(TEXT("the third kept"), IsValid(Shards[2]));
	TestEqual(TEXT("nothing more with nothing collected"), AWasamiGameMode::RemoveCollectedShards(World, {}), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowGameModeTest, "Wasami.GameFlow.GameMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowGameModeTest::RunTest(const FString& Parameters)
{
	UGameplayStatics::DeleteGameInSlot(TestSlotName, UWasamiSaveGame::UserIndex);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AWasamiGameMode* Mode = World->SpawnActorDeferred<AWasamiGameMode>(AWasamiGameMode::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("the game mode"), Mode))
	{
		return false;
	}
	TestEqual(TEXT("the original's slot by default"), Mode->SaveSlotName, FString(TEXT("structSlot")));
	Mode->SaveSlotName = TestSlotName;
	Mode->FinishSpawning(FTransform::Identity);

	// Check For Level Struct Save: no slot, so a new save is made and written.
	if (!TestNotNull(TEXT("a save"), Mode->GetSave()))
	{
		return false;
	}
	TestTrue(TEXT("written at once"), UGameplayStatics::DoesSaveGameExist(TestSlotName, UWasamiSaveGame::UserIndex));
	TestEqual(TEXT("outside the zones, the save's checkpoint as it is"), Mode->GetStartCheckpoint(), 0);

	// The time counter: counts while its gate is open.
	constexpr float Step = 1.f / 60.f;
	for (int32 Index = 0; Index < 60; ++Index)
	{
		Wrapper.TickTestWorld(Step);
	}
	TestEqual(TEXT("a second counted"), Mode->GetTime(), 1.f, 1e-3f);
	Mode->PauseTimeCounter();
	Wrapper.TickTestWorld(Step);
	TestEqual(TEXT("not while paused"), Mode->GetTime(), 1.f, 1e-3f);
	Mode->UnpauseTimeCounter();
	Wrapper.TickTestWorld(Step);
	TestEqual(TEXT("again once unpaused"), Mode->GetTime(), 1.f + Step, 1e-3f);

	// SaveCheckpoint: the checkpoint and the time into the slot, the counter back to 0.
	Mode->GetSave()->Hospital.Time = 10.f;
	Mode->SaveCheckpoint(5);
	TestEqual(TEXT("the counter back to 0"), Mode->GetTime(), 0.f);
	const UWasamiSaveGame* Written = Cast<UWasamiSaveGame>(UGameplayStatics::LoadGameFromSlot(TestSlotName, UWasamiSaveGame::UserIndex));
	if (TestNotNull(TEXT("the slot read back"), Written))
	{
		TestEqual(TEXT("checkpoint 5 written"), Written->Hospital.LevelCheckpoint, 5);
		TestEqual(TEXT("the time added"), Written->Hospital.Time, 11.f + Step, 1e-3f);
	}

	// DeathEvent: once until Reset Death, keeping the best streak and zeroing the save's.
	Mode->GetSave()->Hospital.CurrentStreak = 5;
	TestTrue(TEXT("open at first"), Mode->IsDeathOpen());
	Mode->DeathEvent(nullptr);
	TestFalse(TEXT("closed after a death"), Mode->IsDeathOpen());
	TestEqual(TEXT("the streak kept"), Mode->GetShardStreak(), 5);
	TestEqual(TEXT("the save's streak zeroed"), Mode->GetSave()->Hospital.CurrentStreak, 0);
	Mode->GetSave()->Hospital.CurrentStreak = 9;
	Mode->DeathEvent(nullptr);
	TestEqual(TEXT("a second death does nothing"), Mode->GetShardStreak(), 5);
	Mode->ResetDeath();
	Mode->DeathEvent(nullptr);
	TestEqual(TEXT("again after Reset Death"), Mode->GetShardStreak(), 9);
	Mode->GetSave()->Hospital.CurrentStreak = 3;
	Mode->ResetDeath();
	Mode->DeathEvent(nullptr);
	TestEqual(TEXT("the best streak stays"), Mode->GetShardStreak(), 9);

	// A reopening reads the slot back.
	AWasamiGameMode* Reopened = World->SpawnActorDeferred<AWasamiGameMode>(AWasamiGameMode::StaticClass(), FTransform::Identity);
	Reopened->SaveSlotName = TestSlotName;
	Reopened->FinishSpawning(FTransform::Identity);
	TestEqual(TEXT("the checkpoint read at BeginPlay"), Reopened->GetSave() ? Reopened->GetSave()->Hospital.LevelCheckpoint : -1, 5);

	UGameplayStatics::DeleteGameInSlot(TestSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowCheckpointsTest, "Wasami.GameFlow.Checkpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowCheckpointsTest::RunTest(const FString& Parameters)
{
	using M = AWasamiGameMode;
	TestEqual(TEXT("Zone 1"), M::ZoneOf(TEXT("L_Hospital_Zone1")), 1);
	TestEqual(TEXT("Zone 2"), M::ZoneOf(TEXT("L_Hospital_Zone2")), 2);
	TestEqual(TEXT("not a zone"), M::ZoneOf(TEXT("Untitled_1")), 0);
	TestEqual(TEXT("Zone 1 opens by name"), FString(M::Zone1LevelName), FString(TEXT("L_Hospital_Zone1")));

	// The zones' Spawn: the checkpoint's player start.
	TestEqual(TEXT("4: the lift"), M::PlayerStartTagFor(1, 4), FName(TEXT("04_Start")));
	TestEqual(TEXT("5"), M::PlayerStartTagFor(1, 5), FName(TEXT("05_Start")));
	TestEqual(TEXT("6"), M::PlayerStartTagFor(1, 6), FName(TEXT("06_Start")));
	TestEqual(TEXT("7: the cell (its scene left out)"), M::PlayerStartTagFor(2, 7), FName(TEXT("PlayerStart_Cell")));
	TestEqual(TEXT("8"), M::PlayerStartTagFor(2, 8), FName(TEXT("PlayerStart_MiniBoss")));
	TestEqual(TEXT("9"), M::PlayerStartTagFor(2, 9), FName(TEXT("PlayerStart_Maze")));
	TestEqual(TEXT("10"), M::PlayerStartTagFor(2, 10), FName(TEXT("PlayerStart_PostMaze")));
	TestTrue(TEXT("Zone 2's checkpoints are not Zone 1's"), M::PlayerStartTagFor(1, 7).IsNone());
	TestTrue(TEXT("nor Zone 1's Zone 2's"), M::PlayerStartTagFor(2, 5).IsNone());
	TestTrue(TEXT("no player start for 0"), M::PlayerStartTagFor(1, 0).IsNone());
	TestTrue(TEXT("none outside the zones"), M::PlayerStartTagFor(0, 4).IsNone());

	// The zones' DeathEvent: which death lines, Zone 2 the other way round.
	TestEqual(TEXT("Zone 1, the player: Traps"), M::DeathScreenLevelFor(1, true), UWasamiDeathScreenWidget::TrapsLevel);
	TestEqual(TEXT("Zone 1, an enemy: Asylum"), M::DeathScreenLevelFor(1, false), UWasamiDeathScreenWidget::AsylumLevel);
	TestEqual(TEXT("Zone 2, the player: Asylum"), M::DeathScreenLevelFor(2, true), UWasamiDeathScreenWidget::AsylumLevel);
	TestEqual(TEXT("Zone 2, an enemy: Traps"), M::DeathScreenLevelFor(2, false), UWasamiDeathScreenWidget::TrapsLevel);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowSavingTest, "Wasami.GameFlow.Saving",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowSavingTest::RunTest(const FString& Parameters)
{
	using S = UWasamiSavingWidget;
	// init: 0 → 0.5 by 0.5 s, held to 2 s, 0 by 2.5 s.
	TestEqual(TEXT("the words start unseen"), S::EvaluateTextOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("half by 0.5 s"), S::EvaluateTextOpacity(0.5f), 0.5f, 1e-4f);
	TestEqual(TEXT("still half at 1.25 s"), S::EvaluateTextOpacity(1.25f), 0.5f, 1e-4f);
	TestEqual(TEXT("gone by 2.5 s"), S::EvaluateTextOpacity(2.5f), 0.f, 1e-4f);
	TestEqual(TEXT("the throbber half by 0.5 s"), S::EvaluateThrobberOpacity(0.5f), 0.5f, 1e-4f);
	TestTrue(TEXT("and its auto tangents lift it over half between"), S::EvaluateThrobberOpacity(0.75f) > 0.5f);
	TestEqual(TEXT("the throbber gone by 2.5 s"), S::EvaluateThrobberOpacity(2.5f), 0.f, 1e-4f);

	UWasamiSavingWidget* Saving = NewObject<UWasamiSavingWidget>();
	for (int32 Frame = 0; Frame < 179; ++Frame)
	{
		Saving->Advance(1.f / 60.f);
	}
	TestFalse(TEXT("on the screen until 3 s"), Saving->IsFinished());
	Saving->Advance(2.f / 60.f);
	TestTrue(TEXT("off at 3 s"), Saving->IsFinished());

	// The fade a level opens with: FadeOut at 10, 0.5 s.
	using F = UWasamiBlackFadeWidget;
	TestEqual(TEXT("FadeOut starts black"), F::EvaluateFadeOut(0.f), 1.f, 1e-4f);
	TestEqual(TEXT("and ends clear"), F::EvaluateFadeOut(F::AnimationLength), 0.f, 1e-4f);
	TestEqual(TEXT("FadeIn starts clear"), F::EvaluateFadeIn(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("and ends black"), F::EvaluateFadeIn(F::AnimationLength), 1.f, 1e-4f);
	UWasamiBlackFadeWidget* Fade = NewObject<UWasamiBlackFadeWidget>();
	Fade->bFadeIn = false;
	Fade->Speed = AWasamiGameMode::OpeningFadeSpeed;
	Fade->Advance(0.45f);
	TestTrue(TEXT("still fading at 0.45 s"), !Fade->IsFinished() && Fade->GetOpacity() > 0.f && Fade->GetOpacity() < 0.5f);
	Fade->Advance(0.1f);
	TestTrue(TEXT("done by 0.55 s"), Fade->IsFinished());
	TestEqual(TEXT("clear"), Fade->GetOpacity(), 0.f, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiGameFlowLoadingTest, "Wasami.GameFlow.Loading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiGameFlowLoadingTest::RunTest(const FString& Parameters)
{
	using L = UWasamiLoadingWidget;
	// FadeIn: 0 → 1 over 0.5 s, flat at both keys; played back from its end 2.5 s on.
	TestEqual(TEXT("FadeIn starts clear"), L::EvaluateFadeIn(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("half way at 0.25 s"), L::EvaluateFadeIn(0.25f), 0.5f, 1e-3f);
	TestEqual(TEXT("whole by 0.5 s"), L::EvaluateFadeIn(L::FadeInLength), 1.f, 1e-4f);
	TestEqual(TEXT("held at 2 s"), L::EvaluateOpacity(2.f), 1.f, 1e-4f);
	TestEqual(TEXT("still whole as the fade out starts"), L::EvaluateOpacity(L::FadeOutDelay), 1.f, 1e-3f);
	TestEqual(TEXT("half way out"), L::EvaluateOpacity(L::FadeOutDelay + 0.25f), 0.5f, 1e-3f);
	TestEqual(TEXT("gone by 3 s"), L::EvaluateOpacity(3.01f), 0.f, 1e-4f);

	UWasamiLoadingWidget* Loading = NewObject<UWasamiLoadingWidget>();
	for (int32 Frame = 0; Frame < 209; ++Frame)
	{
		Loading->Advance(1.f / 60.f);
	}
	TestFalse(TEXT("on the screen until 3.5 s"), Loading->IsFinished());
	Loading->Advance(2.f / 60.f);
	TestTrue(TEXT("off at 3.5 s"), Loading->IsFinished());
	return true;
}

#endif
