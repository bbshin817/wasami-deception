#pragma once

#include "CoreMinimal.h"
#include "WasamiLevelResults.generated.h"

struct FWasamiLevelProgress;

/**
 * One row of the level clear screen: what the level Blueprint puts into UMG_LevelClear's <row>_Var, <row>_Rank and
 * <row>_Shards before adding it.
 */
USTRUCT(BlueprintType)
struct FWasamiResultRow
{
	GENERATED_BODY()

	/** The value column (the widget's <row>_Var). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FText Value;

	/** Enum_Ranks: 0 shows nothing, then 1 C, 2 B, 3 A, 4 S. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	uint8 Rank = 0;

	/** The bonus shards the row adds to the total (and its counter counts up to). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	int32 Shards = 0;
};

/**
 * The level clear screen's values, as Dark Deception works them out (pak_reference_2): the hospital's level Blueprint
 * 06_Hospital fills UMG_LevelClear in on Escape (@38874–@54050), and the widget's binding functions add them up
 * (Get_TotalShardAmount_Text_0, Get_FinalRank_Text_0; the same in both versions). The screen is
 * UWasamiLevelClearWidget.
 */
USTRUCT(BlueprintType)
struct FWasamiLevelResults
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow Time;

	/** Shards_Var is the literal '679' and Shards_Shards 679; its counter does nothing, so its "+N" stays empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow SoulShards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow BonusShards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow Secrets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow LivesLost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	FWasamiResultRow ShardStreak;

	/** The difficulty is EASY (the settings' Difficulty 0): the final rank stops at A. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Results")
	bool bEasy = false;

	/** The hospital's soul shards, the text the level Blueprint writes into Shards_Var. */
	static constexpr int32 HospitalShards = 679;

	/** 06_Hospital's Escape: the rows for the level's save (its Time already has the time counter added). */
	static FWasamiLevelResults ForHospital(const FWasamiLevelProgress& Progress, bool bEasy);

	/** Get_TotalShardAmount_Text_0: every row's bonus shards and Shards_Var as a number (so 679 counts twice). */
	int32 GetTotalShards() const;

	/** Get_FinalRank_Text_0: the six ranks' sum over 6, rounded down, clamped to 0..4 (0..3 on EASY). */
	uint8 GetFinalRank() const;

	/** The rows in the screen's order. */
	TArray<const FWasamiResultRow*, TFixedAllocator<6>> GetRows() const;

	/** Time Rank: LessEqual against 2700, 3600 and 4200 s, else C. */
	static uint8 TimeRank(float Seconds);

	/** Time_Var: FromSeconds → BreakTimespan, minutes (at least 1 digit) ' : ' seconds (2 digits); an hour wraps. */
	static FText TimeText(float Seconds);

	/** Enum_ShardStreaks' display name by value: 0 is "0", then 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000. */
	static int32 StreakMilestone(uint8 Streak);

	/** Get_<row>Rank_Text_0: '' C B A S. */
	static FText RankText(uint8 Rank);

	/** Get_<row>Rank_ColorAndOpacity_0: S is gold, C to A dark red, 0 clear. */
	static FLinearColor RankColor(uint8 Rank);

	/** Conv_IntToText as the Blueprints call it (grouped, at least the given digits). */
	static FText IntText(int32 Value, int32 MinimumDigits = 1);
};
