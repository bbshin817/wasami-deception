#include "WasamiLevelResults.h"
#include "WasamiSaveGame.h"
#include "Misc/Timespan.h"

namespace
{
	/** The Blueprint's Selects: a value past the options takes the default, which is never set (0). */
	template <typename T, int32 N>
	T Pick(const T (&Options)[N], int32 Index)
	{
		return Index >= 0 && Index < N ? Options[Index] : T(0);
	}

	/** 06_Hospital's Selects (@40163–@54050): each row's rank by its count, and its bonus shards by its rank. */
	constexpr int32 TimeShards[] = {0, 20, 20, 50, 70};
	constexpr uint8 BonusShardsRank[] = {1, 3, 4, 4};
	constexpr int32 BonusShardsShards[] = {0, 0, 0, 15, 25};
	constexpr uint8 SecretsRank[] = {1, 1, 2, 3, 4, 4};
	constexpr int32 SecretsShards[] = {0, 0, 15, 25, 35};
	/** By Deaths (the hospital does not look at Used Hard Respawn?, unlike the hotel). */
	constexpr uint8 LivesLostRank[] = {4, 4, 4, 3, 2, 1, 1, 1};
	constexpr int32 LivesLostShards[] = {0, 10, 20, 30, 40};
	/** By the Enum_ShardStreaks value. */
	constexpr uint8 ShardStreakRank[] = {1, 1, 2, 2, 3, 4, 4, 4, 4, 4, 4};
	constexpr int32 ShardStreakShards[] = {0, 15, 20, 25, 30};
	constexpr int32 StreakDisplayNames[] = {0, 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000};

	FWasamiResultRow MakeRow(FText Value, uint8 Rank, int32 Shards)
	{
		FWasamiResultRow Row;
		Row.Value = MoveTemp(Value);
		Row.Rank = Rank;
		Row.Shards = Shards;
		return Row;
	}

	FText CountText(int32 Count, int32 Of)
	{
		// Conv_IntToString, then '/N': no grouping.
		return FText::FromString(FString::Printf(TEXT("%d/%d"), Count, Of));
	}
}

FWasamiLevelResults FWasamiLevelResults::ForHospital(const FWasamiLevelProgress& Progress, bool bInEasy)
{
	FWasamiLevelResults Results;
	Results.bEasy = bInEasy;

	const uint8 Time = TimeRank(Progress.Time);
	Results.Time = MakeRow(TimeText(Progress.Time), Time, Pick(TimeShards, Time));

	Results.SoulShards = MakeRow(IntText(HospitalShards), 4, HospitalShards);

	const int32 Bonus = Progress.BonusShards.Num();
	const uint8 BonusRank = Pick(BonusShardsRank, Bonus);
	Results.BonusShards = MakeRow(CountText(Bonus, 2), BonusRank, Pick(BonusShardsShards, BonusRank));

	const int32 Found = Progress.Secrets.Num();
	const uint8 FoundRank = Pick(SecretsRank, Found);
	Results.Secrets = MakeRow(CountText(Found, 4), FoundRank, Pick(SecretsShards, FoundRank));

	const uint8 LivesRank = Pick(LivesLostRank, Progress.Deaths);
	Results.LivesLost = MakeRow(IntText(Progress.Deaths), LivesRank, Pick(LivesLostShards, LivesRank));

	const uint8 StreakRank = Pick(ShardStreakRank, Progress.Streak);
	Results.ShardStreak = MakeRow(IntText(StreakMilestone(Progress.Streak)), StreakRank, Pick(ShardStreakShards, StreakRank));
	return Results;
}

int32 FWasamiLevelResults::GetTotalShards() const
{
	int32 Total = FCString::Atoi(*SoulShards.Value.ToString());
	for (const FWasamiResultRow* Row : GetRows())
	{
		Total += Row->Shards;
	}
	return Total;
}

uint8 FWasamiLevelResults::GetFinalRank() const
{
	int32 Sum = 0;
	for (const FWasamiResultRow* Row : GetRows())
	{
		Sum += Row->Rank;
	}
	return static_cast<uint8>(FMath::Clamp(FMath::FloorToInt32(Sum / 6.f), 0, bEasy ? 3 : 4));
}

TArray<const FWasamiResultRow*, TFixedAllocator<6>> FWasamiLevelResults::GetRows() const
{
	return {&Time, &SoulShards, &BonusShards, &Secrets, &LivesLost, &ShardStreak};
}

uint8 FWasamiLevelResults::TimeRank(float Seconds)
{
	if (Seconds <= 2700.f)
	{
		return 4;
	}
	if (Seconds <= 3600.f)
	{
		return 3;
	}
	return Seconds <= 4200.f ? 2 : 1;
}

FText FWasamiLevelResults::TimeText(float Seconds)
{
	const FTimespan Span = FTimespan::FromSeconds(Seconds);
	return FText::FromString(IntText(Span.GetMinutes()).ToString() + TEXT(" : ") + IntText(Span.GetSeconds(), 2).ToString());
}

int32 FWasamiLevelResults::StreakMilestone(uint8 Streak)
{
	return Pick(StreakDisplayNames, Streak);
}

FText FWasamiLevelResults::RankText(uint8 Rank)
{
	static const FText Texts[] = {FText::GetEmpty(), FText::FromString(TEXT("C")), FText::FromString(TEXT("B")),
		FText::FromString(TEXT("A")), FText::FromString(TEXT("S"))};
	return Rank < UE_ARRAY_COUNT(Texts) ? Texts[Rank] : FText::GetEmpty();
}

FLinearColor FWasamiLevelResults::RankColor(uint8 Rank)
{
	if (Rank == 4)
	{
		return FLinearColor(0.9387f, 0.6156f, 0.169f, 1.f);
	}
	return Rank >= 1 && Rank <= 3 ? FLinearColor(0.533f, 0.f, 0.f, 1.f) : FLinearColor::Transparent;
}

FText FWasamiLevelResults::IntText(int32 Value, int32 MinimumDigits)
{
	FNumberFormattingOptions Options;
	Options.SetUseGrouping(true);
	Options.SetMinimumIntegralDigits(MinimumDigits);
	Options.SetMaximumIntegralDigits(324);
	return FText::AsNumber(Value, &Options);
}
