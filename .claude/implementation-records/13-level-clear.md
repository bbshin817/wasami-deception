---
title: 脱出後のスコア画面（リザルトの規則）
sources:
  - Source/wasami_deception/WasamiLevelResults.h
  - Source/wasami_deception/WasamiLevelResults.cpp
  - Source/wasami_deception/Tests/WasamiLevelClearTests.cpp
updated: 2026-09-19
---

# 脱出後のスコア画面（リザルトの規則）

## 役割
Zone 2 のガレージのポータルで脱出したあとのスコア画面（本家の `UI/Menu/UMG_LevelClear`）の値。本家の病院のレベル BP `06_Hospital` の `Escape` が画面に入れる 6 行（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・LIVES LOST・SHARD STREAK）の文字・ランク・加算シャードと、画面のバインド関数が出す TOTAL SHARDS と FINAL RANK を、エンジンに依らない静的関数で作る。画面そのもの・出し方・連続回収は作業一覧の項目 14 の残りのステップ（進捗記録 `20260919-level-clear`）。

## 公開インターフェース
- `FWasamiResultRow`（USTRUCT）… 1 行: `Value`（`FText`。本家の `<行>_Var`）・`Rank`（`uint8`。本家の `Enum_Ranks`: 0 は空・1 C・2 B・3 A・4 S）・`Shards`（`int32`。本家の `<行>_Shards`。合計に足し、行の数え上げが数える値）。
- `FWasamiLevelResults`（USTRUCT）… 6 行 `Time`・`SoulShards`・`BonusShards`・`Secrets`・`LivesLost`・`ShardStreak` と `bEasy`（難易度 EASY。FINAL RANK が A で止まる）。
  - `ForHospital(Progress, bEasy)`（静的）… 病院の `Escape`: セーブの病院の欄（`FWasamiLevelProgress`、06 記録。`Time` はゲームモードの時間を足した後の値）から 6 行を作る。
  - `GetTotalShards()` … 本家の `Get_TotalShardAmount_Text_0`: 6 行の `Shards` の和 + `SoulShards.Value` を数にしたもの（`Conv_TextToString` → `Conv_StringToInt`）。
  - `GetFinalRank()` … 本家の `Get_FinalRank_Text_0`: 6 行のランクの和 ÷ 6 の切り捨てを 0..4（`bEasy` なら 0..3）に収める。
  - `GetRows()` … 6 行を画面の順に。
  - 静的: `TimeRank(Seconds)`・`TimeText(Seconds)`・`StreakMilestone(Streak)`・`RankText(Rank)`・`RankColor(Rank)`・`IntText(Value, MinimumDigits)`。定数 `HospitalShards` 679。

## 内部構造と処理の流れ
- **TIME**: 文字は `TimeText`（`FTimespan::FromSeconds` の分の部分〈1 桁以上〉+ ` : ` + 秒の部分〈2 桁〉。本家の `FromSeconds` → `BreakTimespan`。分は時間の部分を含まないので、1 時間を超えると折り返す。秒は切り捨て）。ランクは `TimeRank`: 2700 s 以下 4（S）、3600 s 以下 3（A）、4200 s 以下 2（B）、それ以外 1（C）。加算（ランク 0..4）[0, 20, 20, 50, 70]。
- **SOUL SHARDS**: 文字 `679`（本家の定数の文字）、ランク 4、`Shards` 679（本家の `Shards_Shards`）。行の数え上げ（本家の `SoulShardsCounter`）は何もしないので、画面の「+N」は空のまま（画面のステップで写す）。
- **BONUS SHARDS**: 文字 `<数>/2`（セーブの `BonusShards` の長さ。区切りなし）。ランク（長さ 0..3）[1, 3, 4, 4]、加算（ランク 0..4）[0, 0, 0, 15, 25]。
- **SECRETS**: 文字 `<数>/4`（セーブの `Secrets` の長さ）。ランク（長さ 0..5）[1, 1, 2, 3, 4, 4]、加算 [0, 0, 15, 25, 35]。
- **LIVES LOST**: 文字はセーブの `Deaths`。ランク（死亡数 0..7）[4, 4, 4, 3, 2, 1, 1, 1]、加算 [0, 10, 20, 30, 40]。病院は本家のホテルと違い `Used Hard Respawn?`（LAST CHECKPOINT を使ったか）を見ない。
- **SHARD STREAK**: 文字は `StreakMilestone(セーブの Streak)`（`Enum_ShardStreaks` の値の表示名: 0 が "0"、1..10 が 20・50・100・150・200・250・350・500・700・1000）。ランク（値 0..10）[1, 1, 2, 2, 3, 4, 4, 4, 4, 4, 4]、加算 [0, 15, 20, 25, 30]。
- **表の外の値**: 本家の Select の既定（`K2Node_Select_Default_*`）は書かれていないので 0。行のランクが 0（文字は空、色は透明）・加算 0 になる（例: 8 回以上死ぬと LIVES LOST のランクが空）。`Pick` がこれを写す。
- **数の文字**: `IntText` は本家の `Conv_IntToText(値, 符号なし, 区切りあり, 最小の桁, 最大 324)` と同じ `FText::AsNumber` の書式。区切りがあるので 1000 は `1,000`（連続回収の 1000）。
- **合計**: 6 行の加算 + `SoulShards.Value` の数。`Shards_Shards` と `Shards_Var` の両方に 679 が入るので、**679 は 2 回数えられる**（本家のまま。新しいセーブで 70 + 679 + 679 + 0 + 0 + 40 + 15 = 1483）。
- **FINAL RANK**: ランクの和 ÷ 6 を `FloorToInt32`、0..4（EASY は 0..3）に収める。新しいセーブ（時間 0・死亡 0）は 4 + 4 + 1 + 1 + 4 + 1 = 15 → 2（B）。全部 S なら 4。
- **ランクの色**（`RankColor`、本家の `Get_<行>Rank_ColorAndOpacity_0`）: 4 は金 (0.9387, 0.6156, 0.169, 1)、1..3 は暗い赤 (0.533, 0, 0, 1)、0 は (0, 0, 0, 0)。
- テスト `Wasami.LevelClear.Results`: 時間の境目（2700 / 3600 / 4200 s とその少し後）と加算、時間の文字（0 s・65 s・59.9 s・2700 s・1 時間の折り返し）、SOUL SHARDS、BONUS SHARDS 0..4・SECRETS 0..6・LIVES LOST 0..8・SHARD STREAK 0..11 の文字・ランク・加算、1000 の区切り、新しいセーブ・全部 S・EASY・全部が表の外の合計と FINAL RANK、ランクの文字と色。

## 作るアセット
なし（値だけ）。スコア画面の素材は画面のステップで足す。

## 原作データの根拠
- 行の値: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt` の `Escape`（@55455。`python Tools/dd/bp_flow.py <file> Escape`）: 時間のランク @57931・@58760・@59589（2700・3600・4200）、`Create(UMG_LevelClear)` @38874 から `Time_Var` @39424・`Time_Shards` @40163・`Shards_Var`/`Shards_Rank`/`Shards_Shards` @40437・`BonusShards_*` @41144〜@43706・`Secrets_*` @44450〜@47128・`LivesLost_*` @47908〜@50459・`ShardStreak_Var` @51275・`ShardsStreak_Rank` @52424・`ShardsStreak_Shards` @54050。WebGL 版（04 記録の `results.ts`）はホテル `01_Hotel` の値で、境目・表・分母・LIVES LOST の数え方が違う。
- 合計と FINAL RANK と色: `pak_reference_2/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt` の `Get_TotalShardAmount_Text_0`・`Get_FinalRank_Text_0`（`Global Settings Save Instance` の `Difficulty` が 0 なら上限 3）・`Get_TimeRank_Text_0`・`Get_TimeRank_ColorAndOpacity_0`。旧版 `pak_reference` も同じ（名前の付け方だけが違う）。
- `Enum_ShardStreaks` の表示名: `pak_reference_2/_assets/DDeception/Content/UI/Menu/Streaks/Enum_ShardStreaks.json`（`NewEnumerator0` "20"〜`NewEnumerator9` "1000"・`NewEnumerator10` "0"）。値の並びは `BP_DD_GameMode` の `Check Streak` が 20 で 1、1000 で 10 を書くことから（値 0 が "0"）。
- セーブの欄の型: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/LevelStructure/DD_LevelStructureyyy.json`（`BonusShards`・`Secrets` は int の配列）。

## 依存関係
- 使う: `FWasamiLevelProgress`（`WasamiSaveGame.h`、06 記録）。
- 使う側: まだ無い（スコア画面と脱出の流れが使う予定）。
- エンジン: `FTimespan`、`FText::AsNumber`（`FNumberFormattingOptions`）。

## 既知の制約・注意点
- 数の区切りは文化による（`FText::AsNumber`）。英語と日本語はどちらも `1,000`。
- BONUS SHARDS と SECRETS は、赤いシャード（作業一覧の項目 10）と秘密（項目 12）ができるまでセーブの配列が空なので、いつも `0/2`・`0/4`（ランク C）。SHARD STREAK は連続回収（項目 14 のステップ 2）ができるまで 0（C）。
- 難易度（項目 18）が無いので、呼ぶ側は `bEasy` を偽で渡す。

## 変更履歴
- 2026-09-19: 初版。リザルトの規則 `FWasamiLevelResults`（病院の `Escape` の値）とテスト `Wasami.LevelClear.Results`（作業一覧の項目 14 のステップ 1）
