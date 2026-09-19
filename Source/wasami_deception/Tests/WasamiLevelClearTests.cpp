#include "Misc/AutomationTest.h"

#include "../WasamiGameMode.h"
#include "../WasamiLevelClearWidget.h"
#include "../WasamiLevelResults.h"
#include "../WasamiSaveGame.h"
#include "../WasamiShardStreakWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiLevelClearScreenTest, "Wasami.LevelClear.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiLevelClearScreenTest::RunTest(const FString& Parameters)
{
	using W = UWasamiLevelClearWidget;
	using R = FWasamiLevelResults;

	// ClearAnimation's length, its audio track and its event track.
	TestEqual(TEXT("ClearAnimation runs 263388 ticks"), W::ClearLength, 4.3898f, 1e-4f);
	TestEqual(TEXT("UI_YouEscaped from 0.75 s"), W::EscapedSoundTime, 0.75f);
	TestEqual(TEXT("ShowResults at 3.25 s"), W::ShowResultsTime, 3.25f);
	TestEqual(TEXT("added at Z 6"), W::ViewportZOrder, 6);

	// The tree, built as the widget is taken (its Construct runs then), with a new save's results.
	W* Screen = NewObject<W>();
	Screen->Results = R::ForHospital(FWasamiLevelProgress(), false);
	Screen->Initialize();
	Screen->TakeWidget();
	UWidgetTree* Tree = Screen->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}
	for (const TCHAR* Name : {TEXT("Image_4"), TEXT("ResultsBox"), TEXT("LevelName"), TEXT("TimeBox"), TEXT("ShardStreakShards"),
		TEXT("TotalShardAmount"), TEXT("FinalRank"), TEXT("NextButton"), TEXT("ClearLevel"), TEXT("Image_216"), TEXT("FadeOut"),
		TEXT("Image_6"), TEXT("Image_7")})
	{
		TestNotNull(FString::Printf(TEXT("%s is in the tree"), Name), Tree->FindWidget(Name));
	}
	TestNull(TEXT("no XP box"), Tree->FindWidget(TEXT("XPBox")));
	TestNull(TEXT("no DIARY UNLOCKED!"), Tree->FindWidget(TEXT("FinalRankText")));
	TestFalse(TEXT("easymode taken off"), Screen->IsEasyModeShown());
	const TCHAR* const Images[][2] = {{TEXT("Image_216"), TEXT("you_escaped")}, {TEXT("LevelName"), TEXT("chapter_ui_title_tormenttherapy")},
		{TEXT("Image_6"), TEXT("T_Vignette")}};
	for (const auto& Each : Images)
	{
		const UImage* Image = Cast<UImage>(Tree->FindWidget(Each[0]));
		const UObject* Texture = Image ? Image->GetBrush().GetResourceObject() : nullptr;
		if (Texture)
		{
			TestEqual(FString::Printf(TEXT("%s shows %s"), Each[0], Each[1]), Texture->GetName(), FString(Each[1]));
		}
		else
		{
			AddError(FString::Printf(TEXT("%s is missing: run WasamiDDTools.import_dd_ui"), Each[1]));
		}
	}

	// The bindings: the values, the ranks' letters and colours, TOTAL SHARDS and FINAL RANK.
	const TCHAR* const Values[] = {TEXT("0 : 00"), TEXT("679"), TEXT("0/2"), TEXT("0/4"), TEXT("0"), TEXT("0")};
	const uint8 Ranks[] = {4, 4, 1, 1, 4, 1};
	for (int32 Row = 0; Row < 6; ++Row)
	{
		TestEqual(FString::Printf(TEXT("row %d's value"), Row), Screen->GetValueText(Row).ToString(), FString(Values[Row]));
		TestEqual(FString::Printf(TEXT("row %d's rank"), Row), Screen->GetRankText(Row).ToString(), R::RankText(Ranks[Row]).ToString());
		TestTrue(FString::Printf(TEXT("row %d's rank colour"), Row), Screen->GetRankColor(Row).Equals(R::RankColor(Ranks[Row])));
	}
	TestEqual(TEXT("TOTAL SHARDS"), Screen->GetTotalText().ToString(), FString(TEXT("1,483")));
	TestEqual(TEXT("FINAL RANK"), Screen->GetFinalRankText().ToString(), FString(TEXT("B")));
	TestTrue(TEXT("its colour"), Screen->GetFinalRankColor().Equals(R::RankColor(2)));

	// ClearAnimation, frame by frame: clear at first, the sound as it passes 0.75 s, ShowResults as it passes 3.25 s.
	TestEqual(TEXT("the screen starts clear"), Screen->GetRenderOpacity(), 0.f, 1e-4f);
	float Seconds = 0.f;
	auto RunTo = [&](float Until)
	{
		while (Seconds + 1e-4f < Until)
		{
			Screen->Advance(1.f / 60.f);
			Seconds += 1.f / 60.f;
		}
	};
	RunTo(0.25f);
	TestEqual(TEXT("in at 0.25 s"), Screen->GetRenderOpacity(), 1.f, 1e-3f);
	RunTo(0.74f);
	TestFalse(TEXT("no sound before 0.75 s"), Screen->HasPlayedEscapedSound());
	RunTo(0.76f);
	TestTrue(TEXT("the sound at 0.75 s"), Screen->HasPlayedEscapedSound());
	RunTo(3.24f);
	TestFalse(TEXT("no results before 3.25 s"), Screen->HasShownResults());
	TestEqual(TEXT("the red leaves by 3 s"), Tree->FindWidget(TEXT("ClearLevel"))->GetRenderOpacity(), 0.f, 1e-4f);
	RunTo(3.26f);
	TestTrue(TEXT("ShowResults at 3.25 s"), Screen->HasShownResults());

	// ClearAnimation's tracks.
	TestEqual(TEXT("RESULTS clear at 3 s"), W::EvaluateResultsOpacity(3.f), 0.f, 1e-4f);
	TestEqual(TEXT("RESULTS in at 3.25 s"), W::EvaluateResultsOpacity(3.25f), 1.f, 1e-4f);
	TestEqual(TEXT("You Escaped! clear at 0.5 s"), W::EvaluateEscapedOpacity(0.5f), 0.f, 1e-4f);
	TestEqual(TEXT("in at 0.75 s"), W::EvaluateEscapedOpacity(0.75f), 1.f, 1e-4f);
	TestEqual(TEXT("unturned before its section"), W::EvaluateEscapedAngle(0.4f), 0.f, 1e-4f);
	TestEqual(TEXT("at its own size before its section"), W::EvaluateEscapedScale(0.4f), 1.f, 1e-4f);
	TestEqual(TEXT("45° at 0.5 s"), W::EvaluateEscapedAngle(0.5f), 45.f, 1e-3f);
	TestEqual(TEXT("twice its size at 0.5 s"), W::EvaluateEscapedScale(0.5f), 2.f, 1e-4f);
	TestEqual(TEXT("lands at 0.75 s"), W::EvaluateEscapedAngle(0.75f), 0.f, 1e-3f);
	TestEqual(TEXT("at its size at 0.75 s"), W::EvaluateEscapedScale(0.75f), 1.f, 1e-4f);
	TestEqual(TEXT("-10° at 0.8 s"), W::EvaluateEscapedAngle(0.8f), -10.f, 1e-3f);
	TestEqual(TEXT("1.1 at 0.8 s"), W::EvaluateEscapedScale(0.8f), 1.1f, 1e-4f);
	TestEqual(TEXT("still after 0.95 s"), W::EvaluateEscapedAngle(1.5f), 0.f, 1e-3f);
	TestEqual(TEXT("the red in at 0.25 s"), W::EvaluateClearOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("still at 2.75 s"), W::EvaluateClearOpacity(2.75f), 1.f, 1e-4f);
	TestEqual(TEXT("gone at 3 s"), W::EvaluateClearOpacity(3.f), 0.f, 1e-4f);
	TestTrue(TEXT("the jolt at 0.8 s"), W::EvaluateClearJolt(0.8f).Equals(FVector2D(6., 3.), 1e-3));
	TestTrue(TEXT("and back at 0.85 s"), W::EvaluateClearJolt(0.85f).Equals(FVector2D(-2., -7.), 1e-3));
	TestTrue(TEXT("none before"), W::EvaluateClearJolt(0.5f).IsZero());
	TestEqual(TEXT("the vignette clear at 0.7 s"), W::EvaluateVignetteAlpha(0.7f), 0.f, 1e-3f);
	TestEqual(TEXT("flashes at 0.75 s"), W::EvaluateVignetteAlpha(0.75f), 1.f, 1e-4f);
	TestEqual(TEXT("gone at 1 s"), W::EvaluateVignetteAlpha(1.f), 0.f, 1e-4f);
	TestEqual(TEXT("the white clear before its section"), W::EvaluateFlashAlpha(0.6f), 0.f, 1e-4f);
	TestEqual(TEXT("the white at 0.75 s"), W::EvaluateFlashAlpha(0.75f), 1.f, 1e-4f);
	TestEqual(TEXT("gone at 0.85 s"), W::EvaluateFlashAlpha(0.85f), 0.f, 1e-4f);
	TestEqual(TEXT("EASY MODE clear before 3 s"), W::EvaluateEasyOpacity(2.f), 0.f, 1e-4f);
	TestEqual(TEXT("EASY MODE in at 3.25 s"), W::EvaluateEasyOpacity(3.25f), 1.f, 1e-4f);

	// On EASY, easymode stays.
	W* Easy = NewObject<W>();
	Easy->Results = R::ForHospital(FWasamiLevelProgress(), true);
	Easy->Initialize();
	Easy->TakeWidget();
	TestTrue(TEXT("easymode stays on EASY"), Easy->IsEasyModeShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWasamiLevelClearShowResultsTest, "Wasami.LevelClear.ShowResults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWasamiLevelClearShowResultsTest::RunTest(const FString& Parameters)
{
	using W = UWasamiLevelClearWidget;
	using R = FWasamiLevelResults;

	// The timings: the Delay chain, the rows' stamps and counters, the others.
	const float Delays[] = {0.25f, 0.25f, 0.25f, 0.25f, 0.25f, 0.5f, 1.f, 1.f};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Delays)); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Delay %d"), Index), W::ResultsDelays[Index], Delays[Index]);
	}
	TestEqual(TEXT("Time's stamp at 0.4 s"), W::RowStampTime(0), 0.4f, 1e-6f);
	TestEqual(TEXT("Bonus Shards' at 23999 ticks"), W::RowStampTime(2), 23999.f / 60000.f, 1e-6f);
	TestEqual(TEXT("Secrets' at 0.45 s"), W::RowStampTime(3), 0.45f, 1e-6f);
	TestEqual(TEXT("Shard Streak's at 0.45 s"), W::RowStampTime(5), 0.45f, 1e-6f);
	const float Spans[] = {0.25f, 0.f, 0.25f, 0.05f, 0.25f, 0.5f};
	for (int32 Row = 0; Row < 6; ++Row)
	{
		TestEqual(FString::Printf(TEXT("row %d's counter's span"), Row), W::RowCountSpan(Row), Spans[Row]);
	}
	TestEqual(TEXT("TOTAL SHARDS' span"), W::TotalCountSpan, 0.5f);
	TestEqual(TEXT("FINAL RANK's stamp at 7200 ticks"), W::FinalStampTime, 0.12f);
	TestEqual(TEXT("Finished 4 s after NEXT"), W::FinishDelay, 4.f);

	// The curves.
	TestEqual(TEXT("a value clear at first"), W::EvaluateRowValueOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("in at 0.25 s"), W::EvaluateRowValueOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("1.5 times its size at first"), W::EvaluateRowValueScale(0.f), 1.5f, 1e-4f);
	TestEqual(TEXT("its size at 0.25 s"), W::EvaluateRowValueScale(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("1.05 at 0.3 s"), W::EvaluateRowValueScale(17999.f / 60000.f), 1.05f, 1e-4f);
	TestEqual(TEXT("its size after 0.5 s"), W::EvaluateRowValueScale(0.9f), 1.f, 1e-4f);
	TestEqual(TEXT("the rank clear at 0.25 s"), W::EvaluateRowRankOpacity(0.25f), 0.f, 1e-4f);
	TestEqual(TEXT("in at 0.5 s"), W::EvaluateRowRankOpacity(0.5f), 1.f, 1e-4f);
	TestEqual(TEXT("at its own size before its section"), W::EvaluateRowRankScale(0.2f), 1.f, 1e-4f);
	TestEqual(TEXT("1.5 at 0.25 s"), W::EvaluateRowRankScale(0.25f), 1.5f, 1e-4f);
	TestEqual(TEXT("1.05 at 0.55 s"), W::EvaluateRowRankScale(0.55f), 1.05f, 1e-4f);
	TestEqual(TEXT("the +N clear at 0.5 s"), W::EvaluateRowShardsOpacity(0.5f), 0.f, 1e-4f);
	TestEqual(TEXT("in at 0.75 s"), W::EvaluateRowShardsOpacity(0.75f), 1.f, 1e-4f);
	TestEqual(TEXT("at its own size before its section"), W::EvaluateRowShardsScale(0.4f), 1.f, 1e-4f);
	TestEqual(TEXT("1.5 at 0.5 s"), W::EvaluateRowShardsScale(0.5f), 1.5f, 1e-4f);
	TestEqual(TEXT("1.05 at 0.8 s"), W::EvaluateRowShardsScale(0.8f), 1.05f, 1e-4f);
	TestEqual(TEXT("its size at 1 s"), W::EvaluateRowShardsScale(1.f), 1.f, 1e-4f);
	TestEqual(TEXT("FINAL RANK in at 0.25 s"), W::EvaluateFinalOpacity(0.25f), 1.f, 1e-4f);
	TestEqual(TEXT("1.5 times its size at first"), W::EvaluateFinalScale(0.f), 1.5f, 1e-4f);
	TestEqual(TEXT("its size at 0.1 s"), W::EvaluateFinalScale(0.1f), 1.f, 1e-4f);
	TestEqual(TEXT("1.15 at 0.15 s"), W::EvaluateFinalScale(0.15f), 1.15f, 1e-4f);
	TestEqual(TEXT("1 after 0.5 s"), W::EvaluateFinalScale(0.75f), 1.f, 1e-4f);
	TestTrue(TEXT("no jolt before 0.15 s"), W::EvaluateFinalJolt(0.1f).IsZero());
	TestTrue(TEXT("the jolt at 0.2 s"), W::EvaluateFinalJolt(0.2f).Equals(FVector2D(4., -7.), 1e-3));
	TestTrue(TEXT("and back at 0.25 s"), W::EvaluateFinalJolt(0.25f).Equals(FVector2D(-2., 3.), 1e-3));
	TestTrue(TEXT("still after 0.35 s"), W::EvaluateFinalJolt(0.6f).Equals(FVector2D::ZeroVector, 1e-3));
	TestEqual(TEXT("the fade clear at first"), W::EvaluateFadeOpacity(0.f), 0.f, 1e-4f);
	TestEqual(TEXT("halfway at 0.5 s"), W::EvaluateFadeOpacity(0.5f), 0.5f, 1e-3f);
	TestEqual(TEXT("black at 1 s"), W::EvaluateFadeOpacity(1.f), 1.f, 1e-3f);

	// The sounds: the stamps and the fill, which loops.
	const TCHAR* const Sounds[] = {TEXT("/Game/DD/Audio/UI/Level_Clear_Grade_Stamp_v1.Level_Clear_Grade_Stamp_v1"),
		TEXT("/Game/DD/Audio/UI/Level_Clear_Grade_Stamp_v2.Level_Clear_Grade_Stamp_v2")};
	for (const TCHAR* Path : Sounds)
	{
		if (!LoadObject<USoundBase>(nullptr, Path))
		{
			AddError(FString::Printf(TEXT("%s is missing: run WasamiDDTools.import_dd_ui"), Path));
		}
	}
	const USoundWave* Fill = LoadObject<USoundWave>(nullptr, TEXT("/Game/DD/Audio/UI/UI_XP_Bar_Fill_V2A_0617.UI_XP_Bar_Fill_V2A_0617"));
	if (TestNotNull(TEXT("the fill sound"), Fill))
	{
		TestTrue(TEXT("it loops"), Fill->bLooping);
	}

	// A new save's results, frame by frame at 60 fps: TIME +70, SOUL SHARDS none, BONUS SHARDS and SECRETS +0,
	// LIVES LOST +40, SHARD STREAK +15.
	W* Screen = NewObject<W>();
	Screen->Results = R::ForHospital(FWasamiLevelProgress(), false);
	Screen->Initialize();
	Screen->TakeWidget();
	UWidgetTree* Tree = Screen->WidgetTree;
	if (!TestNotNull(TEXT("the tree"), Tree))
	{
		return false;
	}
	float Seconds = 0.f;
	auto RunTo = [&](float Until)
	{
		while (Seconds + 1e-4f < Until)
		{
			Screen->Advance(1.f / 60.f);
			Seconds += 1.f / 60.f;
		}
	};
	const UWidget* TimeValue = Tree->FindWidget(TEXT("Time"));
	const UWidget* TimeRank = Tree->FindWidget(TEXT("TimeRank"));
	const UWidget* TimeShards = Tree->FindWidget(TEXT("TimeShards"));
	const UWidget* LivesValue = Tree->FindWidget(TEXT("LivesLost"));
	const UWidget* Total = Tree->FindWidget(TEXT("TotalShardAmount"));
	const UWidget* Final = Tree->FindWidget(TEXT("FinalRank"));
	const UWidget* Fade = Tree->FindWidget(TEXT("FadeOut"));
	if (!TestTrue(TEXT("the rows and the rest are in the tree"), TimeValue && TimeRank && TimeShards && LivesValue && Total && Final && Fade))
	{
		return false;
	}
	RunTo(3.2f);
	TestEqual(TEXT("no row before ShowResults"), Screen->GetResultsStep(), 0);
	TestEqual(TEXT("the values clear"), TimeValue->GetRenderOpacity(), 0.f);
	RunTo(3.3f);
	TestEqual(TEXT("TIME's row at 3.25 s"), Screen->GetResultsStep(), 1);
	TestTrue(TEXT("its value coming in"), TimeValue->GetRenderOpacity() > 0.f && TimeValue->GetRenderOpacity() < 1.f);
	// The rank's curve dips a little below 0 before 0.25 s (the key there arrives with a tangent), as UE's does.
	TestTrue(TEXT("its rank still clear"), TimeRank->GetRenderOpacity() <= 0.f);
	// The rows 0.25 s apart, TOTAL SHARDS 0.5 s after SHARD STREAK, FINAL RANK 1 s on, the input 1 s after that. Never
	// early; a Delay's float countdown can take a frame more than its length (0.25 s at 1/60 s is 16 frames, as UE's).
	const float StepTimes[] = {3.5f, 3.75f, 4.f, 4.25f, 4.5f, 5.f, 6.f, 7.f};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(StepTimes)); ++Index)
	{
		RunTo(StepTimes[Index] - 0.03f);
		TestEqual(FString::Printf(TEXT("step %d not before %.2f s"), Index + 2, StepTimes[Index]), Screen->GetResultsStep(), Index + 1);
		RunTo(StepTimes[Index] + static_cast<float>(Index + 2) / 60.f + 0.01f);
		TestEqual(FString::Printf(TEXT("step %d at %.2f s"), Index + 2, StepTimes[Index]), Screen->GetResultsStep(), Index + 2);
		if (Index == 2)
		{
			// TIME's counter began at 3.75 s: +1 a frame (Delay(0.25 / 70) is shorter than a frame).
			TestTrue(TEXT("TIME is counting"), Screen->IsCounting(0));
			const FString Before = Screen->GetShardsText(0).ToString();
			RunTo(Seconds + 1.f / 60.f);
			const FString After = Screen->GetShardsText(0).ToString();
			TestTrue(TEXT("one a frame"), Before.StartsWith(TEXT("+")) && FCString::Atoi(*After.RightChop(1)) == FCString::Atoi(*Before.RightChop(1)) + 1);
		}
		if (Index == 1)
		{
			TestEqual(TEXT("TIME's value in"), TimeValue->GetRenderOpacity(), 1.f, 1e-4f);
			TestEqual(TEXT("at its size (the animation's last key over the tree's 1.05)"), TimeValue->GetRenderTransform().Scale.X, 1., 1e-4);
			TestEqual(TEXT("TIME's rank in"), TimeRank->GetRenderOpacity(), 1.f, 1e-4f);
		}
	}
	TestEqual(TEXT("TIME counted to +70"), Screen->GetShardsText(0).ToString(), FString(TEXT("+70")));
	TestFalse(TEXT("and stopped"), Screen->IsCounting(0));
	TestEqual(TEXT("SOUL SHARDS' +N stays blank"), Screen->GetShardsText(1).ToString(), FString());
	TestEqual(TEXT("BONUS SHARDS +0"), Screen->GetShardsText(2).ToString(), FString(TEXT("+0")));
	TestEqual(TEXT("SECRETS +0"), Screen->GetShardsText(3).ToString(), FString(TEXT("+0")));
	TestEqual(TEXT("LIVES LOST +40"), Screen->GetShardsText(4).ToString(), FString(TEXT("+40")));
	TestEqual(TEXT("SHARD STREAK +15"), Screen->GetShardsText(5).ToString(), FString(TEXT("+15")));
	TestEqual(TEXT("TOTAL SHARDS in"), Total->GetRenderOpacity(), 1.f, 1e-4f);
	TestEqual(TEXT("its text still the bound total"), Screen->GetTotalText().ToString(), FString(TEXT("1,483")));
	TestTrue(TEXT("its counter still going (one a frame to 1483)"), Screen->IsCounting(W::TotalCounter));
	TestEqual(TEXT("FINAL RANK in"), Final->GetRenderOpacity(), 1.f, 1e-4f);
	TestEqual(TEXT("at its size"), Final->GetRenderTransform().Scale.X, 1., 1e-4);
	TestEqual(TEXT("the screen still"), Tree->RootWidget->GetRenderTransform().Translation.Size(), 0., 1e-3);

	// NEXT's hover and unhover colours.
	UButton* Next = Cast<UButton>(Tree->FindWidget(TEXT("NextButton")));
	if (TestNotNull(TEXT("NEXT"), Next))
	{
		Next->OnHovered.Broadcast();
		TestTrue(TEXT("white while hovered"), Next->GetColorAndOpacity().Equals(FLinearColor::White));
		Next->OnUnhovered.Broadcast();
		TestTrue(TEXT("grey again"), Next->GetColorAndOpacity().Equals(FLinearColor(0.114583f, 0.114583f, 0.114583f, 1.f), 1e-4f));
	}

	// NEXT: a DoOnce, Fade Out over 1 s and Finished 4 s on.
	Screen->PressNext();
	TestTrue(TEXT("NEXT pressed"), Screen->IsNextPressed());
	const float Pressed = Seconds;
	RunTo(Pressed + 0.5f);
	Screen->PressNext();
	TestEqual(TEXT("fading"), Fade->GetRenderOpacity(), 0.5f, 0.05f);
	RunTo(Pressed + 1.1f);
	TestEqual(TEXT("black after 1 s (a second NEXT does nothing)"), Fade->GetRenderOpacity(), 1.f, 1e-3f);
	RunTo(Pressed + 3.95f);
	TestFalse(TEXT("not finished before 4 s"), Screen->IsFinished());
	RunTo(Pressed + 4.05f);
	TestTrue(TEXT("Finished at 4 s"), Screen->IsFinished());

	// TOTAL SHARDS' counter, left alone, steps one a frame to its number.
	RunTo(5.f + 1482.f / 60.f + 0.5f);
	TestFalse(TEXT("TOTAL SHARDS' counter done after 1482 frames"), Screen->IsCounting(W::TotalCounter));
	return true;
}

#endif
