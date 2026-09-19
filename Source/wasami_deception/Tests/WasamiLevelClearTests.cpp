#include "Misc/AutomationTest.h"

#include "../WasamiLevelResults.h"
#include "../WasamiSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
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

#endif
