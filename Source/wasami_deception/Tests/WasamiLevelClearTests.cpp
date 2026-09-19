#include "Misc/AutomationTest.h"

#include "../WasamiGameMode.h"
#include "../WasamiLevelResults.h"
#include "../WasamiSaveGame.h"
#include "../WasamiShardStreakWidget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FString StreakSlotName(TEXT("WasamiTest_Streak"));

	FWasamiLevelProgress Progress(float Time, int32 Bonus, int32 Secrets, int32 Deaths, uint8 Streak)
	{
		FWasamiLevelProgress Result;
		Result.Time = Time;
		Result.BonusShards.SetNum(Bonus);
		Result.Secrets.SetNum(Secrets);
		Result.Deaths = Deaths;
		Result.Streak = Streak;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiLevelClearResultsTest, "Wasami.LevelClear.Results",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiLevelClearResultsTest::RunTest(const FString& Parameters)
{
	using R = FWasamiLevelResults;

	// TIME: LessEqual against 2700, 3600 and 4200 s.
	TestEqual(TEXT("0 s is S"), R::TimeRank(0.f), uint8(4));
	TestEqual(TEXT("2700 s is still S"), R::TimeRank(2700.f), uint8(4));
	TestEqual(TEXT("just past 2700 s is A"), R::TimeRank(2700.5f), uint8(3));
	TestEqual(TEXT("3600 s is A"), R::TimeRank(3600.f), uint8(3));
	TestEqual(TEXT("just past 3600 s is B"), R::TimeRank(3600.5f), uint8(2));
	TestEqual(TEXT("4200 s is B"), R::TimeRank(4200.f), uint8(2));
	TestEqual(TEXT("past 4200 s is C"), R::TimeRank(4200.5f), uint8(1));
	TestEqual(TEXT("time bonus by rank"), R::ForHospital(Progress(4300.f, 0, 0, 0, 0), false).Time.Shards, 20);
	TestEqual(TEXT("B is also 20"), R::ForHospital(Progress(4000.f, 0, 0, 0, 0), false).Time.Shards, 20);
	TestEqual(TEXT("A is 50"), R::ForHospital(Progress(3000.f, 0, 0, 0, 0), false).Time.Shards, 50);
	TestEqual(TEXT("S is 70"), R::ForHospital(Progress(100.f, 0, 0, 0, 0), false).Time.Shards, 70);

	// Time_Var: minutes, ' : ', two-digit seconds; the timespan's minutes part wraps at an hour.
	TestEqual(TEXT("0 s"), R::TimeText(0.f).ToString(), FString(TEXT("0 : 00")));
	TestEqual(TEXT("65 s"), R::TimeText(65.f).ToString(), FString(TEXT("1 : 05")));
	TestEqual(TEXT("seconds are cut, not rounded"), R::TimeText(59.9f).ToString(), FString(TEXT("0 : 59")));
	TestEqual(TEXT("45 min"), R::TimeText(2700.f).ToString(), FString(TEXT("45 : 00")));
	TestEqual(TEXT("an hour wraps"), R::TimeText(3725.f).ToString(), FString(TEXT("2 : 05")));

	// SOUL SHARDS: always 679 and S.
	const R Start = R::ForHospital(FWasamiLevelProgress(), false);
	TestEqual(TEXT("soul shards text"), Start.SoulShards.Value.ToString(), FString(TEXT("679")));
	TestEqual(TEXT("soul shards rank"), Start.SoulShards.Rank, uint8(4));
	TestEqual(TEXT("soul shards' Shards_Shards"), Start.SoulShards.Shards, 679);

	// BONUS SHARDS: of 2.
	const uint8 BonusRanks[] = {1, 3, 4, 4, 0};
	const int32 BonusShards[] = {0, 15, 25, 25, 0};
	for (int32 Count = 0; Count < 5; ++Count)
	{
		const R Row = R::ForHospital(Progress(0.f, Count, 0, 0, 0), false);
		TestEqual(FString::Printf(TEXT("bonus %d text"), Count), Row.BonusShards.Value.ToString(), FString::Printf(TEXT("%d/2"), Count));
		TestEqual(FString::Printf(TEXT("bonus %d rank"), Count), Row.BonusShards.Rank, BonusRanks[Count]);
		TestEqual(FString::Printf(TEXT("bonus %d shards"), Count), Row.BonusShards.Shards, BonusShards[Count]);
	}

	// SECRETS: of 4.
	const uint8 SecretRanks[] = {1, 1, 2, 3, 4, 4, 0};
	const int32 SecretShards[] = {0, 0, 15, 25, 35, 35, 0};
	for (int32 Count = 0; Count < 7; ++Count)
	{
		const R Row = R::ForHospital(Progress(0.f, 0, Count, 0, 0), false);
		TestEqual(FString::Printf(TEXT("secrets %d text"), Count), Row.Secrets.Value.ToString(), FString::Printf(TEXT("%d/4"), Count));
		TestEqual(FString::Printf(TEXT("secrets %d rank"), Count), Row.Secrets.Rank, SecretRanks[Count]);
		TestEqual(FString::Printf(TEXT("secrets %d shards"), Count), Row.Secrets.Shards, SecretShards[Count]);
	}

	// LIVES LOST: the deaths.
	const uint8 LivesRanks[] = {4, 4, 4, 3, 2, 1, 1, 1, 0};
	const int32 LivesShards[] = {40, 40, 40, 30, 20, 10, 10, 10, 0};
	for (int32 Deaths = 0; Deaths < 9; ++Deaths)
	{
		const R Row = R::ForHospital(Progress(0.f, 0, 0, Deaths, 0), false);
		TestEqual(FString::Printf(TEXT("%d deaths text"), Deaths), Row.LivesLost.Value.ToString(), FString::FromInt(Deaths));
		TestEqual(FString::Printf(TEXT("%d deaths rank"), Deaths), Row.LivesLost.Rank, LivesRanks[Deaths]);
		TestEqual(FString::Printf(TEXT("%d deaths shards"), Deaths), Row.LivesLost.Shards, LivesShards[Deaths]);
	}

	// SHARD STREAK: the Enum_ShardStreaks value's display name.
	const int32 Milestones[] = {0, 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000, 0};
	const uint8 StreakRanks[] = {1, 1, 2, 2, 3, 4, 4, 4, 4, 4, 4, 0};
	const int32 StreakShards[] = {15, 15, 20, 20, 25, 30, 30, 30, 30, 30, 30, 0};
	for (uint8 Streak = 0; Streak < 12; ++Streak)
	{
		const R Row = R::ForHospital(Progress(0.f, 0, 0, 0, Streak), false);
		TestEqual(FString::Printf(TEXT("streak %d milestone"), Streak), R::StreakMilestone(Streak), Milestones[Streak]);
		TestEqual(FString::Printf(TEXT("streak %d rank"), Streak), Row.ShardStreak.Rank, StreakRanks[Streak]);
		TestEqual(FString::Printf(TEXT("streak %d shards"), Streak), Row.ShardStreak.Shards, StreakShards[Streak]);
	}
	TestEqual(TEXT("streak text"), R::ForHospital(Progress(0.f, 0, 0, 0, 5), false).ShardStreak.Value.ToString(), FString(TEXT("200")));
	TestEqual(TEXT("1000 is grouped, as Conv_IntToText does"),
		R::ForHospital(Progress(0.f, 0, 0, 0, 10), false).ShardStreak.Value.ToString(), FString(TEXT("1,000")));

	// TOTAL SHARDS and FINAL RANK. A new save: S 70, 679 + 679, C 0, C 0, S 40, C 15; ranks 4 4 1 1 4 1 = 15.
	TestEqual(TEXT("the total counts 679 twice (Shards_Shards and Shards_Var)"), Start.GetTotalShards(), 70 + 679 + 679 + 40 + 15);
	TestEqual(TEXT("15 / 6 rounds down to B"), Start.GetFinalRank(), uint8(2));
	const R Best = R::ForHospital(Progress(2000.f, 2, 4, 0, 5), false);
	TestEqual(TEXT("everything S"), Best.GetFinalRank(), uint8(4));
	TestEqual(TEXT("the best total"), Best.GetTotalShards(), 70 + 679 + 679 + 25 + 35 + 40 + 30);
	TestEqual(TEXT("EASY stops at A"), R::ForHospital(Progress(2000.f, 2, 4, 0, 5), true).GetFinalRank(), uint8(3));
	const R Worst = R::ForHospital(Progress(5000.f, 9, 9, 9, 11), false);
	TestEqual(TEXT("out-of-range counts take the Selects' default"), Worst.GetFinalRank(), uint8(0));
	TestEqual(TEXT("and add nothing"), Worst.GetTotalShards(), 20 + 679 + 679);

	// Ranks' text and colour.
	TestTrue(TEXT("rank 0 shows nothing"), R::RankText(0).IsEmpty());
	TestEqual(TEXT("C"), R::RankText(1).ToString(), FString(TEXT("C")));
	TestEqual(TEXT("S"), R::RankText(4).ToString(), FString(TEXT("S")));
	TestTrue(TEXT("S is gold"), R::RankColor(4).Equals(FLinearColor(0.9387f, 0.6156f, 0.169f, 1.f)));
	TestTrue(TEXT("A is dark red"), R::RankColor(3).Equals(FLinearColor(0.533f, 0.f, 0.f, 1.f)));
	TestEqual(TEXT("0 is clear"), R::RankColor(0).A, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiLevelClearShardStreakTest, "Wasami.LevelClear.ShardStreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiLevelClearShardStreakTest::RunTest(const FString& Parameters)
{
	using M = AWasamiGameMode;
	using W = UWasamiShardStreakWidget;

	// Check Streak's milestones: exactly 20, 50, 100, 150, 200, 250, 350, 500, 700 and 1000 in a row.
	const int32 Milestones[] = {20, 50, 100, 150, 200, 250, 350, 500, 700, 1000};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Milestones); ++Index)
	{
		TestEqual(FString::Printf(TEXT("%d in a row"), Milestones[Index]), M::StreakMilestoneFor(Milestones[Index]), uint8(Index + 1));
		TestEqual(FString::Printf(TEXT("one past %d"), Milestones[Index]), M::StreakMilestoneFor(Milestones[Index] + 1), uint8(0));
		TestEqual(FString::Printf(TEXT("the milestone's display name for %d"), Milestones[Index]),
			FWasamiLevelResults::StreakMilestone(uint8(Index + 1)), Milestones[Index]);
	}
	TestEqual(TEXT("none at 0"), M::StreakMilestoneFor(0), uint8(0));
	TestEqual(TEXT("none at 19"), M::StreakMilestoneFor(19), uint8(0));
	TestEqual(TEXT("none past 1000"), M::StreakMilestoneFor(2000), uint8(0));
	const int32 Sounds[] = {INDEX_NONE, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, INDEX_NONE};
	for (int32 Milestone = 0; Milestone < UE_ARRAY_COUNT(Sounds); ++Milestone)
	{
		TestEqual(FString::Printf(TEXT("the sound for milestone %d"), Milestone), M::StreakSoundIndex(uint8(Milestone)), Sounds[Milestone]);
	}
	for (uint8 Milestone = 0; Milestone <= 10; ++Milestone)
	{
		TestEqual(FString::Printf(TEXT("an extra life at milestone %d"), Milestone), W::GivesExtraLife(Milestone), Milestone == 5 || Milestone == 8);
	}

	// UMG_ShardStreak's Construct and Anim.
	W* Widget = NewObject<W>();
	Widget->Begin(5);
	TestTrue(TEXT("EXTRA LIFE ! at 200"), Widget->IsExtraLifeShown());
	if (const UTexture2D* Card = Widget->GetStreakTexture())
	{
		TestEqual(TEXT("the 200's card"), Card->GetName(), FString(TEXT("shard_streak_200")));
	}
	else
	{
		AddError(TEXT("shard_streak_200 is missing: run WasamiDDTools.import_dd_ui"));
	}
	Widget->Begin(1);
	TestFalse(TEXT("none at 20"), Widget->IsExtraLifeShown());
	Widget->Begin(0);
	TestNull(TEXT("no card for 0 (the Select's default)"), Widget->GetStreakTexture());
	Widget->Begin(10);
	for (int32 Frame = 0; Frame < 119; ++Frame)
	{
		Widget->Advance(1.f / 60.f);
	}
	TestFalse(TEXT("up until 2 s"), Widget->IsFinished());
	Widget->Advance(2.f / 60.f);
	TestTrue(TEXT("off at 2 s"), Widget->IsFinished());
	TestEqual(TEXT("the card starts at twice its size"), W::EvaluateCardScale(0.f), 2.f, 1e-4f);
	TestEqual(TEXT("and clear"), W::EvaluateCardAlpha(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("its size at 0.15 s"), W::EvaluateCardScale(0.15f), 1.f, 1e-4f);
	TestEqual(TEXT("opaque at 0.15 s"), W::EvaluateCardAlpha(0.15f), 1.f, 1e-4f);
	TestEqual(TEXT("1.1 at 0.25 s"), W::EvaluateCardScale(0.25f), 1.1f, 1e-4f);
	TestEqual(TEXT("1.2 at the end"), W::EvaluateCardScale(1.5f), 1.2f, 1e-4f);
	TestEqual(TEXT("clear at the end"), W::EvaluateCardAlpha(1.5f), 0.f, 1e-4f);
	TestEqual(TEXT("the vignette at 1 until 0.9 s"), W::EvaluateVignetteScale(0.5f), 1.f, 1e-4f);
	TestEqual(TEXT("the vignette at 2 at the end"), W::EvaluateVignetteScale(1.5f), 2.f, 1e-4f);
	TestEqual(TEXT("the vignette flashes at 0.15 s"), W::EvaluateVignetteAlpha(0.15f), 1.f, 1e-4f);
	TestEqual(TEXT("half at 0.25 s"), W::EvaluateVignetteAlpha(0.25f), 0.5f, 1e-4f);
	TestEqual(TEXT("a quarter at 0.9 s"), W::EvaluateVignetteAlpha(0.9f), 0.25f, 1e-4f);
	TestEqual(TEXT("EXTRA LIFE ! unseen before 0.2 s"), W::EvaluateLifeOpacity(0.1f), 0.f, 1e-4f);
	TestEqual(TEXT("at its own 1.1 before 0.2 s"), W::EvaluateLifeScale(0.1f), 1.1f, 1e-4f);
	TestEqual(TEXT("1.25 at 0.2 s"), W::EvaluateLifeScale(0.2f), 1.25f, 1e-4f);
	TestEqual(TEXT("seen at 0.35 s"), W::EvaluateLifeOpacity(0.35f), 1.f, 1e-4f);
	TestEqual(TEXT("0.95 at 0.35 s"), W::EvaluateLifeScale(0.35f), 0.95f, 1e-4f);
	TestEqual(TEXT("gone at the end"), W::EvaluateLifeOpacity(1.5f), 0.f, 1e-4f);

	// The game mode's Check Streak on the save, and Check Shards running it.
	UGameplayStatics::DeleteGameInSlot(StreakSlotName, UWasamiSaveGame::UserIndex);
	FTestWorldWrapper Wrapper;
	if (!Wrapper.CreateTestWorld(EWorldType::Game) || !Wrapper.BeginPlayInTestWorld())
	{
		Wrapper.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	M* Mode = World->SpawnActorDeferred<M>(M::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("the game mode"), Mode))
	{
		return false;
	}
	Mode->SaveSlotName = StreakSlotName;
	Mode->FinishSpawning(FTransform::Identity);
	if (!TestNotNull(TEXT("a save"), Mode->GetSave()))
	{
		return false;
	}
	FWasamiLevelProgress& Hospital = Mode->GetSave()->Hospital;
	Hospital.CurrentStreak = 18;
	TestEqual(TEXT("19 is no milestone"), Mode->CheckStreak(), uint8(0));
	TestEqual(TEXT("the streak counted"), Hospital.CurrentStreak, 19);
	TestEqual(TEXT("the game mode's streak follows"), Mode->GetShardStreak(), 19);
	TestEqual(TEXT("no milestone yet"), Hospital.Streak, uint8(0));
	TestEqual(TEXT("20 is the first"), Mode->CheckStreak(), uint8(1));
	TestEqual(TEXT("the save's best milestone"), Hospital.Streak, uint8(1));
	Hospital.CurrentStreak = 199;
	TestEqual(TEXT("200"), Mode->CheckStreak(), uint8(5));
	TestEqual(TEXT("raised to 200"), Hospital.Streak, uint8(5));
	TestEqual(TEXT("the longest streak"), Mode->GetShardStreak(), 200);
	Hospital.CurrentStreak = 19;
	TestEqual(TEXT("20 again"), Mode->CheckStreak(), uint8(1));
	TestEqual(TEXT("the best milestone kept"), Hospital.Streak, uint8(5));
	TestEqual(TEXT("the longest streak kept"), Mode->GetShardStreak(), 200);
	Mode->DeathEvent(nullptr);
	TestEqual(TEXT("a death ends the streak"), Hospital.CurrentStreak, 0);
	TestEqual(TEXT("and keeps the longest"), Mode->GetShardStreak(), 200);
	Mode->CheckShards();
	Mode->CheckShards();
	TestEqual(TEXT("Check Shards counts each shard, waiting or not"), Hospital.CurrentStreak, 2);

	UGameplayStatics::DeleteGameInSlot(StreakSlotName, UWasamiSaveGame::UserIndex);
	return true;
}

#endif
