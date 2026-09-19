---
title: 脱出後のスコア画面（リザルトの規則・連続回収）
sources:
  - Source/wasami_deception/WasamiLevelResults.h
  - Source/wasami_deception/WasamiLevelResults.cpp
  - Source/wasami_deception/WasamiShardStreakWidget.h
  - Source/wasami_deception/WasamiShardStreakWidget.cpp
  - Source/wasami_deception/Tests/WasamiLevelClearTests.cpp
updated: 2026-09-19
---

# 脱出後のスコア画面（リザルトの規則・連続回収）

## 役割
Zone 2 のガレージのポータルで脱出したあとのスコア画面（本家の `UI/Menu/UMG_LevelClear`）の値。本家の病院のレベル BP `06_Hospital` の `Escape` が画面に入れる 6 行（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・LIVES LOST・SHARD STREAK）の文字・ランク・加算シャードと、画面のバインド関数が出す TOTAL SHARDS と FINAL RANK を、エンジンに依らない静的関数で作る。SHARD STREAK の元になる**シャードの連続回収**（本家 `BP_DD_GameMode` の `Check Streak` と、節目の画面 `UI/Menu/Streaks/UMG_ShardStreak`）もここに書く（ゲームモードの口は 02・06 記録の `AWasamiGameMode`）。スコア画面そのものと出し方は作業一覧の項目 14 の残りのステップ（進捗記録 `20260919-level-clear`）。

## 公開インターフェース
- `FWasamiResultRow`（USTRUCT）… 1 行: `Value`（`FText`。本家の `<行>_Var`）・`Rank`（`uint8`。本家の `Enum_Ranks`: 0 は空・1 C・2 B・3 A・4 S）・`Shards`（`int32`。本家の `<行>_Shards`。合計に足し、行の数え上げが数える値）。
- `FWasamiLevelResults`（USTRUCT）… 6 行 `Time`・`SoulShards`・`BonusShards`・`Secrets`・`LivesLost`・`ShardStreak` と `bEasy`（難易度 EASY。FINAL RANK が A で止まる）。
  - `ForHospital(Progress, bEasy)`（静的）… 病院の `Escape`: セーブの病院の欄（`FWasamiLevelProgress`、06 記録。`Time` はゲームモードの時間を足した後の値）から 6 行を作る。
  - `GetTotalShards()` … 本家の `Get_TotalShardAmount_Text_0`: 6 行の `Shards` の和 + `SoulShards.Value` を数にしたもの（`Conv_TextToString` → `Conv_StringToInt`）。
  - `GetFinalRank()` … 本家の `Get_FinalRank_Text_0`: 6 行のランクの和 ÷ 6 の切り捨てを 0..4（`bEasy` なら 0..3）に収める。
  - `GetRows()` … 6 行を画面の順に。
  - 静的: `TimeRank(Seconds)`・`TimeText(Seconds)`・`StreakMilestone(Streak)`・`RankText(Rank)`・`RankColor(Rank)`・`IntText(Value, MinimumDigits)`。定数 `HospitalShards` 679。

- 連続回収（ゲームモード `AWasamiGameMode`）: `CheckStreak()`（戻り値は出した節目、無ければ 0）・静的 `StreakMilestoneFor(CurrentStreak)`（20・50・100・150・200・250・350・500・700・1000 ちょうどで 1..10、ほかは 0）・静的 `StreakSoundIndex(Milestone)`（`StreakSounds` の添字）・`StreakSounds`（`Shard_Streak_Milestone_V1A`・`V2`・`V3A`・`V4`）・`StreakShakeClass`（`BP_CameraShake_Streak`）。`CheckShards()` が毎回呼ぶ。デバッグの `Wasami.Streak N`（セーブの `CurrentStreak` を N − 1 にして `CheckStreak`。N 番目を取ったのと同じ）。
- `UWasamiShardStreakWidget`（`UUserWidget`）… 節目の画面。`Show(WorldContext, Streak)`（本家の `Create` → `Streak` → `AddToPlayerScreen(2)`。プレイヤーのコントローラが無いと出さない）・`Streak`（1..10）・`GivesExtraLife(Streak)`（5〈200〉と 8〈500〉）・`Begin(Streak)`（Construct の頭から。テストが単独で呼ぶ）・`Advance(DeltaSeconds)`・`IsFinished()`・`IsExtraLifeShown()`・`GetStreakTexture()`・`Evaluate*`（`Anim` の 6 本の曲線）・定数 `AnimLength`（90001 / 60000 s）・`RemoveDelay` 2 s・`ZOrder` 2・素材の `StreakTextures`（10 枚）・`VignetteTexture`・`LifeTexture`・`TextFont`。

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
### 連続回収（`Check Streak`）
- `CheckStreak()`（本家 @35674）: セーブの病院の `CurrentStreak` を +1 し、ゲームモードの `ShardStreak` がそれより小さければ上げる（本家の `Shard Streak`。`DeathEvent` も死亡のときに最大を取る）。`CurrentStreak` が節目ちょうどなら、`UWasamiShardStreakWidget::Show(節目)`（本家 @30741 ほか: Create → `Streak` → `AddToPlayerScreen(2)`）→ `PlayerCameraManager->StartCameraShake(BP_CameraShake_Streak, 1, CameraLocal)`（本家の `PlayCameraShake(…, 1, 0, 0)`）→ `PlaySound2D(節目の音, 1, 1)`（音の波形の `Volume` は 0.7）→ セーブの `Streak` を、その `Enum_ShardStreaks` の表示名の数（`FWasamiLevelResults::StreakMilestone`）が節目の数より小さいときだけ節目にする（本家の `Less_IntInt(Conv_StringToInt(GetEnumeratorUserFriendlyName(...)))`。表示名は単調なので、実際は最大を取るのと同じ）。この順は本家どおり（画面の Construct が先に走ってライフを足す）。セーブはここでは書かない（チェックポイントと死亡画面が書く。本家も同じ）。
- 音: 20・50 は `V1A`、100・150・200 は `V2`、250・350 は `V3A`、500・700・1000 は `V4`（本家の枝の `PlaySound2D`）。
- 本家の 100 の枝は、ボールルーム（`Level` 0）のときに Steam の実績を読み込む（`CacheAchievements`）。病院ではないので写さない。
- `Check Shards` の中の順（本家 @34491）: `Collect Shard` を流し、Sequence の 1 本目で `Delay 0.05`（待っている間は置き直さない）、2 本目で `Check Streak`。だから待っている間の呼び出しでも毎回数える。Zone 1 の `05_Persistent` が 1 s 後に呼ぶ `Check Shards`（シャードを取っていない呼び出し。11 記録）でも 1 数える（本家も同じ）。
- 死亡: `DeathEvent` が `ShardStreak` を最大にして `CurrentStreak` を 0 にし、死亡画面の Construct も 0 にして書く（06・09 記録）。セーブの `Streak`（最高の節目）は残るので、スコア画面の SHARD STREAK は脱出までの最高の節目。

### 節目の画面（`UWasamiShardStreakWidget`、本家 `UMG_ShardStreak`）
- 木（スロットのまま。ルート `CanvasPanel_0` は全画面。この画面は `HitTestInvisible` にして、2 s の間も下の画面のクリックを止めない〈本家の既定の `Visible` から変えた所〉）:
  - `StreakImage`: 節目の札（612 × 227。`SetBrushFromTexture(…, bMatchSize)` で札の大きさ）。真ん中に揃え（アンカー 0.5・揃え 0.5・`bAutoSize`）、`ColorAndOpacity` (1, 1, 1, 0)。
  - `Image_161`: `T_Vignette`（1920 × 1080）を紫 (0.2248, 0, 0.3802) に染め、全画面（アンカー 0..1、端の余白 0.96 / 0.54 px）、`ColorAndOpacity` のアルファ 0.25・`RenderTransform` の拡大 2（どちらも `Anim` が最初のコマで上書きする）。札より後ろ（上）に描く。
  - `extralife`（`CanvasPanel`）: 280 × 129 を真ん中から 115 px 下に（アンカー 0.5・揃え (0.5, 0)）、`RenderOpacity` 0・拡大 1.1。中に `Image_88`（`life_icon_02`、90 × 90。左の縦の真ん中から (−8, −44.5)）と `TextBlock_94`（`EXTRA LIFE !`、`helvetica-neue-bold_Font` の `Default`・24。輪郭の色は紫だが太さが UMG の既定の 0 なので描かない。左 80・右 100 の余白で上半分に）。
- Construct（`NativeConstruct` → `Begin`）: 札を `Streak` で選ぶ（0 や範囲の外は Select の既定の null）、`Anim` を 0 から、`extralife` を `Streak` 5・8 なら `Visible`（そしてゲームインスタンスの `IncrementLives`。0..6 に収まる）、ほかは `Hidden`。2 s で `RemoveFromParent`。`Anim` と 2 s はウィジェットのティック（`Advance`）で進める（SAVING PROGRESS と同じ。本家の `Delay` はゲームの時間なので、止まっている間も進むのが本家との違い）。
- `Anim`（1.5 s。書き出しのキーと UE の自動の接線、`WasamiWidgetAnimation.h`）: 札の拡大 2 → 1（0.15 s）→ 1.1（0.25 s）→ 1（0.5 s）→ 1（0.9 s）→ 1.2（1.5 s）、アルファ 0 → 1（0.15 s）→ 1（0.9 s）→ 0（1.5 s）。ビネットの拡大は 1 のまま（線形の 2 つのキー、0.35 s）→ 0.9 s から作者の接線で 2 へ、アルファ 0 → 1（0.15 s）→ 0.5（0.25 s）→ 0.25（0.9 s）→ 0。`extralife` は 0.2 s からの区間で、不透明度 0 → 1（0.35 s）→ 1（0.9 s）→ 0、拡大 1.25 → 0.95（0.35 s）→ 1（0.9 s）→ 1.1。0.2 s より前は区間の外なので、パネル自身の不透明度 0・拡大 1.1 のまま。旧版（`pak_reference`）も同じ値（WebGL 版 `hud/streak.ts` と一致）。

### テスト
- `Wasami.LevelClear.Results`: 時間の境目（2700 / 3600 / 4200 s とその少し後）と加算、時間の文字（0 s・65 s・59.9 s・2700 s・1 時間の折り返し）、SOUL SHARDS、BONUS SHARDS 0..4・SECRETS 0..6・LIVES LOST 0..8・SHARD STREAK 0..11 の文字・ランク・加算、1000 の区切り、新しいセーブ・全部 S・EASY・全部が表の外の合計と FINAL RANK、ランクの文字と色。
- `Wasami.LevelClear.ShardStreak`: 節目の数（10 個ちょうどと 1 つ後・0・19・2000）と表示名、音の添字（0..11）、EXTRA LIFE ! の節目（5・8 だけ）、画面の `Begin`（200 の札が `shard_streak_200`・EXTRA LIFE ! の出し分け・0 は札なし）と 2 s で外れること、`Anim` の曲線の要所、テストのワールドのゲームモードでの `CheckStreak`（19 は節目でない・20 で 1・セーブの `Streak` と `ShardStreak` が上がる・200 で 5・後の 20 では最高が残る・死亡で `CurrentStreak` 0 と最長が残る・`CheckShards` を続けて 2 回で 2）。ライフの +1（Construct の `IncrementLives`）はテストのワールドにプレイヤーのコントローラが無いので、PIE で確かめた（下）。

## 作るアセット
- 連続回収: `dd_ui.import_shard_streak()`（`WasamiDDTools.import_dd_ui` から。01 記録）が `/Game/DD/UI/Menu/Streaks/shard_streak_20`〜`_1000`（10 枚）と `/Game/DD/Audio/UI/Shard_Streak_Milestone_V1A`・`V2`・`V3A`・`V4` を `pak_reference_2` から作る。`T_Vignette`・`helvetica-neue-bold_Font` はタブレット、`life_icon_02` は死亡画面、`BP_CameraShake_Streak` はパワーの取り込みが作る。
- スコア画面の素材は画面のステップで足す。

## 原作データの根拠
- 行の値: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt` の `Escape`（@55455。`python Tools/dd/bp_flow.py <file> Escape`）: 時間のランク @57931・@58760・@59589（2700・3600・4200）、`Create(UMG_LevelClear)` @38874 から `Time_Var` @39424・`Time_Shards` @40163・`Shards_Var`/`Shards_Rank`/`Shards_Shards` @40437・`BonusShards_*` @41144〜@43706・`Secrets_*` @44450〜@47128・`LivesLost_*` @47908〜@50459・`ShardStreak_Var` @51275・`ShardsStreak_Rank` @52424・`ShardsStreak_Shards` @54050。WebGL 版（04 記録の `results.ts`）はホテル `01_Hotel` の値で、境目・表・分母・LIVES LOST の数え方が違う。
- 合計と FINAL RANK と色: `pak_reference_2/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt` の `Get_TotalShardAmount_Text_0`・`Get_FinalRank_Text_0`（`Global Settings Save Instance` の `Difficulty` が 0 なら上限 3）・`Get_TimeRank_Text_0`・`Get_TimeRank_ColorAndOpacity_0`。旧版 `pak_reference` も同じ（名前の付け方だけが違う）。
- `Enum_ShardStreaks` の表示名: `pak_reference_2/_assets/DDeception/Content/UI/Menu/Streaks/Enum_ShardStreaks.json`（`NewEnumerator0` "20"〜`NewEnumerator9` "1000"・`NewEnumerator10` "0"）。値の並びは `BP_DD_GameMode` の `Check Streak` が 20 で 1、1000 で 10 を書くことから（値 0 が "0"）。
- 連続回収: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt` の `Check Streak`（@35674。`python Tools/dd/bp_flow.py <file> "Check Streak"`）: `CurrentStreak` +1 @36171、`Shard Streak` @30366、節目の枝 @30697（20）〜@32677（1000）、揺れと音と `Streak` @6871（20）ほか、100 の枝の実績 @12808。`Check Shards` の Sequence @34491。画面は `.../UI/Menu/Streaks/UMG_ShardStreak.txt` の `Construct`（@122。5・8 で `BP_DD_Functions.Increment Lives`）と `_assets/…/UMG_ShardStreak.json`（木・`Anim`）、揺れは `BP_CameraShake_Streak.json`。旧版 `pak_reference` も節目・音・木・`Anim` は同じ。WebGL 版（`src/game/state.ts`・`src/hud/streak.ts`）とも一致。
- セーブの欄の型: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/LevelStructure/DD_LevelStructureyyy.json`（`BonusShards`・`Secrets` は int の配列）。

## 依存関係
- 使う: `FWasamiLevelProgress`（`WasamiSaveGame.h`、06 記録）、`UWasamiGameInstance::IncrementLives`（06 記録）、`WasamiWidgetAnimation.h`（09 記録）。
- 使う側: ゲームモードの `CheckStreak`（`StreakMilestone`・`UWasamiShardStreakWidget::Show`）。リザルトはまだ無い（スコア画面と脱出の流れが使う予定）。
- エンジン: `FTimespan`、`FText::AsNumber`（`FNumberFormattingOptions`）。

## 既知の制約・注意点
- 数の区切りは文化による（`FText::AsNumber`）。英語と日本語はどちらも `1,000`。
- BONUS SHARDS と SECRETS は、赤いシャード（作業一覧の項目 10）と秘密（項目 12）ができるまでセーブの配列が空なので、いつも `0/2`・`0/4`（ランク C）。
- 病院のシャードは 679 個なので、連続回収の節目は 500 まで届きうる（700・1000 は死なずに全部を取っても届かない）。
- 同じフレームに節目をいくつも越えると（`Wasami.CollectShards` で一度に取るなど）、画面が節目ごとに重なって出る（本家も同じ）。
- 難易度（項目 18）が無いので、呼ぶ側は `bEasy` を偽で渡す。

## 確かめたこと（2026-09-19、PIE、Zone 1 の駐車場）
- `Wasami.Lives 3` の後に `Wasami.Streak 200`: 「UNSTOPPABLE / 200 SHARD STREAK!」の札が大きく出て縮み、紫のビネットが画面の縁で光って広がり、札の下に髑髏と EXTRA LIFE ! が出て、約 1.5 s で消えた。ライフが 3 → 4、セーブの `Streak` 5・`CurrentStreak` 200。

## 変更履歴
- 2026-09-19: シャードの連続回収（ゲームモードの `CheckStreak`・`Wasami.Streak N`、節目の画面 `UWasamiShardStreakWidget`、札と音の取り込み `dd_ui.import_shard_streak`）とテスト `Wasami.LevelClear.ShardStreak` を足した（作業一覧の項目 14 のステップ 2）
- 2026-09-19: 初版。リザルトの規則 `FWasamiLevelResults`（病院の `Escape` の値）とテスト `Wasami.LevelClear.Results`（作業一覧の項目 14 のステップ 1）
