---
title: 脱出後のスコア画面（リザルトの規則・連続回収・画面）
sources:
  - Source/wasami_deception/WasamiLevelResults.h
  - Source/wasami_deception/WasamiLevelResults.cpp
  - Source/wasami_deception/WasamiShardStreakWidget.h
  - Source/wasami_deception/WasamiShardStreakWidget.cpp
  - Source/wasami_deception/WasamiLevelClearWidget.h
  - Source/wasami_deception/WasamiLevelClearWidget.cpp
  - Source/wasami_deception/Tests/WasamiLevelClearTests.cpp
updated: 2026-09-19
---

# 脱出後のスコア画面（リザルトの規則・連続回収・画面）

## 役割
Zone 2 のガレージのポータルで脱出したあとのスコア画面（本家の `UI/Menu/UMG_LevelClear`）の値。本家の病院のレベル BP `06_Hospital` の `Escape` が画面に入れる 6 行（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・LIVES LOST・SHARD STREAK）の文字・ランク・加算シャードと、画面のバインド関数が出す TOTAL SHARDS と FINAL RANK を、エンジンに依らない静的関数で作る。SHARD STREAK の元になる**シャードの連続回収**（本家 `BP_DD_GameMode` の `Check Streak` と、節目の画面 `UI/Menu/Streaks/UMG_ShardStreak`）もここに書く（ゲームモードの口は 02・06 記録の `AWasamiGameMode`）。スコア画面そのもの（`UWasamiLevelClearWidget`。本家の `UI/Menu/UMG_LevelClear` の木と `ClearAnimation`、`ShowResults` の行の出方と数え上げ、NEXT）と、脱出からの出し方（病院のレベル BP の `Escape`）と NEXT の後（同じく `Finished Level`。セーブを空にしてタイトルへ）もここに書く（口はゲームモード `AWasamiGameMode`。02 記録）。

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
- `UWasamiLevelClearWidget`（`UUserWidget`）… スコア画面。`Show(WorldContext, Results)`（本家の `Escape` の `Create` → 値を入れる → `AddToViewport(6)`。プレイヤーのコントローラが無いと出さない）・`Results`（`FWasamiLevelResults`）・`Begin()`（Construct の頭から。`NativeConstruct` が呼ぶ）・`Advance(DeltaSeconds)`・`GetElapsed()`・`HasPlayedEscapedSound()`・`HasShownResults()`・`IsEasyModeShown()`・バインドの文字の読み出し `GetValueText(Row)`・`GetRankText(Row)`・`GetRankColor(Row)`・`GetTotalText()`・`GetFinalRankText()`・`GetFinalRankColor()`・`ClearAnimation` の曲線 `Evaluate*`（10 本）・定数 `ClearLength`（263388 / 60000 s）・`EscapedSoundTime` 0.75・`ShowResultsTime` 3.25・`ViewportZOrder` 6・素材の `EscapedTexture`・`RuleTexture`・`LevelNameTexture`・`VignetteTexture`・`TextFont`・`EscapedSound`。
  - 行の出方と NEXT: `OnFinished`（`BlueprintAssignable`。NEXT から 4 s。本家の `Finished`。レベル BP の `Finished Level` が結ぶ所）・`PressNext()`（NEXT の `OnClicked`。DoOnce）・`bRemoveWhenFinished`（`Finished` で画面を外す。デバッグのコマンド用）・`GetResultsStep()`（`ShowResults` の連鎖の何段目まで進んだか 0..9）・`IsCounting(Counter)`（行 0..5 と `TotalCounter` 6 の数え上げが続いているか）・`IsNextPressed()`・`IsFinished()`・`GetShardsText(Row)`（行の「+N」）・曲線 `EvaluateRow*`（値・ランク・加算の不透明度と拡大の 6 本）・`EvaluateFinalOpacity`・`EvaluateFinalScale`・`EvaluateFinalJolt`・`EvaluateFadeOpacity`・定数 `ResultsDelays`（8 つ）・`ResultsSteps` 9・`RowLength` 1・`RowCounterTime` 0.5・`RowStampTime(Row)`・`RowCountSpan(Row)`・`TotalLength` 0.5・`TotalCountSpan` 0.5・`FinalLength` 0.75・`FinalStampTime` 0.12・`FadeLength`（60001 / 60000 s）・`FinishDelay` 4・素材の `RowStampSound`（`Level_Clear_Grade_Stamp_v2`）・`FinalStampSound`（`_v1`）・`FillSound`（`UI_XP_Bar_Fill_V2A_0617`）。
  - デバッグの `Wasami.LevelClear`（セーブの病院の欄にゲームモードの時間を足した値で、止めず・書かずに画面だけを出す。NEXT の `Finished` で画面を外す）。
- 脱出と NEXT の後（ゲームモード `AWasamiGameMode`）: `Escape()`（病院の `Escape`。戻り値は出した画面、プレイヤーが無ければ null。Zone 2 の `OnEndTrigger` が呼ぶ。11 記録）・`FinishedLevel()`（病院の `Finished Level`。画面の `OnFinished` に結ぶ。DoOnce）・`HasFinishedLevel()`・`GetLevelToOpen()`（`Finished Level` が開いたレベル。前は空）・定数 `FinishedLevelDelay` 1。デバッグの `Wasami.Escape`（その場で `Escape`。Zone 1 でも動く）。

## 内部構造と処理の流れ
- **TIME**: 文字は `TimeText`（`FTimespan::FromSeconds` の分の部分〈1 桁以上〉+ ` : ` + 秒の部分〈2 桁〉。本家の `FromSeconds` → `BreakTimespan`。分は時間の部分を含まないので、1 時間を超えると折り返す。秒は切り捨て）。ランクは `TimeRank`: 2700 s 以下 4（S）、3600 s 以下 3（A）、4200 s 以下 2（B）、それ以外 1（C）。加算（ランク 0..4）[0, 20, 20, 50, 70]。
- **SOUL SHARDS**: 文字 `679`（本家の定数の文字）、ランク 4、`Shards` 679（本家の `Shards_Shards`）。行の数え上げ（本家の `SoulShardsCounter`）は何もしないので、画面の「+N」は空のまま。
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

### スコア画面（`UWasamiLevelClearWidget`、本家 `UMG_LevelClear`）
- 旧版（`pak_reference`）の木と `ClearAnimation` を写す（WebGL 版と同じ。最新版との差は見えない所だけ〈進捗記録の決定事項〉）。木はスロットのまま C++ で組み、描く順も本家の順。作らないもの: XP の箱（`XPBox` と、その `Level Up Animation` だけが使う紫の `Image_161`）、`FinalRankText`（DIARY UNLOCKED!。FINAL RANK が S のときだけ見える。本作に日記が無い。WebGL 版も出さない）。
- 木（ルート `CanvasPanel_0` は `Visible`。ほかは `HitTestInvisible`）:
  - `Image_4`: 黒（エンジンの `Black` の代わりに色のブラシ）を全画面より外へ（余白 −38.04 / −32.03 / −63.96 / −40.01）。
  - `ResultsBox`（`VerticalBox`、1820 × 980 を真ん中に、`RenderOpacity` 0）: `LevelName`（病院の題字 `chapter_ui_title_tormenttherapy` を 648.72 × 129.6 で赤 (1, 0, 0) に染める。本家の Construct はゲームモードの `Level` で選び〈最新版の 7 が病院〉、`SetBrushFromTexture(…, False)` で大きさを保つ。余白 20・真ん中）→ `TextBlock_0` RESULTS（36・灰 0.140625）→ `Image_1`（`results_window` 914 × 18 を箱の幅に伸ばす・余白 10）→ `VerticalBox_142` の 6 行 → `Image_2`（同じ線）→ `HorizontalBox_0`（TOTAL SHARDS:〈30・紫〉と `TotalShardAmount`〈50・最小幅 140.66・中央揃え・折り返し・`RenderOpacity` 0〉）→ `HorizontalBox_1`（FINAL RANK〈50・灰 0.4167〉と `FinalRank`〈75・右揃え・拡大 1.15・`RenderOpacity` 0〉）。
  - 行（`TimeBox` ほか 6 つ、縦の真ん中）: 見出し（35・灰 0.14126・最小幅 400・左の余白 250）、値（30・最小幅 196.15・右揃え・左の余白 500。TIME だけ拡大 1.05）、ランク（40・左の余白 30）、加算（25・紫・最小幅 243・左の余白 65・右寄せ。サイズは `Automatic` で `Value` 0.5）。値・ランク・加算は `RenderOpacity` 0（行のアニメが出す）。加算の文字は本家の設計どおり `+25`・空・`+25`・`+30`・`+21`・`+21`（数え上げが書き換える）。
  - `easymode`（EASY MODE、UMG の既定のフォント、灰 0.4167、下から 10 % の所）: Construct が難易度 EASY でなければ外す（本家の `RemoveFromParent`）。
  - `NextButton`（右下から (−163.4, −89.1)、自動の大きさ。`BackgroundColor` のアルファ 0 で枠が見えず、`ColorAndOpacity` の灰 0.1146 が文字を染める）と `TextBlock_314` NEXT（35、右下揃え、右の余白 5）。ホバーと押すは下の「リザルトの出方と NEXT」。
  - `ClearLevel`（`CanvasPanel`、全画面、`RenderOpacity` 0）: `Image_5`（赤 (1, 0, 0) を画面より外へ）と `Image_216`（`you_escaped` 1141 × 276 を真ん中に）。
  - `FadeOut`（黒、`RenderOpacity` 0。NEXT の後の暗転）、`Image_6`（白い `T_Vignette` 1920 × 1080、アルファ 0）、`Image_7`（白、アルファ 0）。
- バインド関数（値・ランクの文字と色・TOTAL SHARDS・FINAL RANK と色）は `Begin` で `Results` から一度だけ入れる（`Results` は画面の間に変わらない）。数の文字は `IntText`（区切りあり。新しいセーブの合計は `1,483`）。
- `ClearAnimation`（4.39 s。書き出しのキーと UE の自動の接線）: ウィジェット全体の不透明度 0 → 1（0.25 s）。`ClearLevel` 0 → 1（0.25 s）→ 1（2.75 s）→ 0（3.0 s）、その平行移動（0.75 s からの区間）(0, 0) → (6, 3)（0.8 s）→ (−2, −7)（0.85 s）→ 0（0.95 s）。`Image_216` の不透明度 0（0.5 s）→ 1（0.75 s）、回転と拡大（0.5 s からの区間）45° / 2 → 0° / 1（0.75 s）→ −10° / 1.1（0.8 s）→ 0° / 1（0.95 s）。`Image_6` のアルファ 0（0.7 s）→ 1（0.75 s）→ 0（1.0 s）、`Image_7`（0.7 s からの区間）0 → 1（0.75 s）→ 0（0.85 s）。赤・緑・青のキーは無く、画像の白のまま（本家の色のトラックは値の無いチャンネルを今の値で埋める）。`ResultsBox` 0（3.0 s）→ 1（3.25 s）、`easymode` 0（3.0 s）→ 1（3.25 s）。後から始まる区間は、その前はウィジェット自身の値（`LevelClearEvalFrom`）。区間の後は最後のキーの値のまま（本家の `KeepState`）。
- 音のトラック: `UI_YouEscaped`（3.64 s）を 0.75 s から。区間は 4.39 s まで（音の終わりと同じ）。`Advance` が 0.75 s を過ぎたコマで `PlaySound2D`（UI の音なので止まっている間も鳴る）を、過ぎた分だけ先から鳴らす。
- イベントのトラック: 3.25 s に `ShowResults`（下）。
- ウィジェットのティック（`NativeTick` → `Advance`）で進める。ゲームが止まっていても進む（本家の UMG のアニメも、ウィジェットの Delay も、ウィジェットのティックの時間で進む: `UUserWidget` のティックが自分の潜在アクションを処理する）。1 コマの中の順は、再生中のアニメを進める → Delay を数える（`TickDelays`）→ `ClearAnimation` の音とイベント → 行のアニメの音とイベント（`TickResultsEvents`）→ 値を当てる（`ApplyAnimation`）。そのコマに始まったアニメと Delay は次のコマから数える。

### リザルトの出方（`ShowResults`）と NEXT
- **Delay の連鎖**（本家 `ShowResults`）: `Time Animation` → 0.25 s → `Soul Shards Animation` → 0.25 → `Bonus Shards Animation` → 0.25 → `Secrets Animation` → 0.25 → `Lives Lost Animation` → 0.25 → `Shard Streak Animation` → 0.5 → `Total Shards Animation` → 1 → `Final Rank Animation` → 1 → `SetInputMode_UIOnlyEx(自分, DoNotLock)` とマウスカーソル（NEXT がここから押せる。それまではゲームだけの入力なので、クリックは画面に届かない）。`ClearAnimation` の始まりから行は 3.25・3.5・…・4.5 s、TOTAL 5.0 s、FINAL RANK 6.0 s、入力 7.0 s。
- **Delay は UE の `FDelayAction` と同じく**、残り時間から毎コマの時間を引き、0 以下で終わる（1 コマに 1 回まで）。float の引き算なので、1/60 s ちょうどのコマでは 0.25 s に 16 コマ、1 s に 61 コマかかる（UE も同じ）。
- **行のアニメ**（6 つとも同じ。1 s）: 値の不透明度 0 → 1（0.25 s）と拡大 1.5 → 1（0.25 s）→ 1.05（0.3 s）→ 1（0.5 s）、ランクは同じ形を 0.25 s 遅れて（不透明度は 0.25 → 0.5 s。0.25 s のキーの入りの接線で、その前に少し 0 の下へ膨らむ〈UE も同じ〉。拡大の区間は 0.25 s から、その前は 1）、加算の「+N」はさらに 0.25 s 遅れて（0.5 → 0.75 s）。区間の後は最後のキーの 1 なので、TIME の値の木の拡大 1.05 はアニメの後 1 になる（本家どおり）。音のトラックは `Level_Clear_Grade_Stamp_v2` を TIME 24000 tick（0.4 s）、SOUL SHARDS・BONUS SHARDS 23999 tick、ほかの 3 つ 27000 tick（0.45 s）から。イベントのトラックは 0.5 s に行の数え上げ。
- **数え上げ**（本家 `TimeCounter` ほか。`StartCounter`・`StepCounter`）: `CreateSound2D(UI_XP_Bar_Fill_V2A_0617, 1, 1, 0, None, False, True)` → `Play`（波形がループ）→ すぐに 1 回目: 数を +1 し、n より小さければ「+数」（`Conv_IntToString`。区切りなし）と `Delay(span / n)`、n に着いたら「+n」と音を止める。n は行の `Shards`、span は TIME・BONUS SHARDS・LIVES LOST 0.25 s、SECRETS 0.05 s、SHARD STREAK 0.5 s。n が 0 なら 1 回目で「+0」。SOUL SHARDS の数え上げは何もしない（「+N」は空のまま）。Delay は 1 コマに 1 回までなので、span / n が 1 コマより短いと 1 コマに 1 ずつ数える（60 fps で TIME の 70 は約 1.15 s）。
- **TOTAL SHARDS**: `Total Shards Animation`（0.5 s）が `TotalShardAmount` に行の値と同じ不透明度と拡大を当て、0 s のイベントで `TotalShardsCounter`（合計を span 0.5 s で数える）。`TotalShardAmount` の文字はバインド（`Get_TotalShardAmount_Text_0`）なので、数え上げの `SetText` は見えず（UMG はバインドを優先する）、見た目は最初から合計のまま。数え上げは音だけに出て、1 コマに 1 ずつなので 60 fps で合計 1483 に約 24.7 s、音が鳴り続ける（本家のまま。WebGL 版も同じ）。
- **FINAL RANK**（`Final Rank Animation`、0.75 s）: `FinalRank` の不透明度 0 → 1（0.25 s）、拡大 1.5 → 1（0.1 s）→ 1.15（0.15 s）→ 1（0.5 s。木の 1.15 はアニメの後 1 になる）、ルート `CanvasPanel_0` の平行移動（0.15 s からの区間）(0, 0) → (4, −7)（0.2 s）→ (−2, 3)（0.25 s）→ 0（0.35 s）、`Level_Clear_Grade_Stamp_v1` を 7200 tick（0.12 s）から。`FinalRankText`（DIARY UNLOCKED!）のトラックは写さない。
- **NEXT**: ホバーで `SetColorAndOpacity` 白、外れると `Unhovered Color`（0.114583 の灰。木の値と同じ）。押すと（本家の `Replay Mode?` の道。XP の箱の数え上げの道は作らない）DoOnce → `Fade Out`（`FadeOut` の不透明度 0 → 1、1 s）・`SetInputMode_GameOnly`・カーソルを消す → 4 s → `Finished` を知らせ、ゲームモードの `Reset Game Instance(False)` の代わりにゲームインスタンスの `ForgetCollectedShards`（回収の記憶を空に。06 記録）。クリックはティックの間に来るので、フェードと 4 s は押した時から数える。
### 脱出からスコア画面へ（病院の `Escape`）と NEXT の後（`Finished Level`）
- **`Escape`**（本家の病院のポータルの `Trigger_Escape` の `Trigger` がすぐ呼ぶ。本作は Zone 2 のガレージのポータルの `OnEndTrigger` の最後。11 記録）: `SetGamePaused(true)` → チェックポイントの保存と同じ `SaveCheckpoint(0)`（本家の `Escape` も SAVING PROGRESS を Z 0 で先に出し、`Time += ゲームモードの Time`・`LevelCheckpoint = 0`・`Reset Time Counter`・保存の順。06 記録）→ `ForHospital(セーブの病院の欄, EASY 偽)` でスコア画面を Z 6 → `OnFinished` を `FinishedLevel` に結ぶ。本家が `SaveSlot` を読んで確かめる実績は写さない。
- 画面・SAVING PROGRESS・黒のフェードはどれも自分のティックで進むので、一時停止の間も動く。Zone 2 では `OnEndTrigger` の黒のフェード（0.25 s で黒）の上に、画面の全体の不透明度が同じ 0.25 s で出る。
- **`FinishedLevel`**（本家 `Finished Level`）: DoOnce → `SetGamePaused(false)` → 1 s → セーブの病院の欄を空の欄にして書く（本家の `levelStruct[5] = levelStruct[10]`）→ ゲームインスタンスの `ForgetCollectedShards` と `ResetLives`（本家の `Reset Game Instance(False)`: 回収の記憶を空に、ライフ 3）→ タイトル `L_Title`（`TitleLevelName`。14 記録）を開く。本家は `Replay Mode?` で `TitleScreen`、でなければ次の章の `06_Cinematic` を開くが、本作は病院で終わり次の章が無いのでタイトルへ（2026-09-19、作業一覧の項目 17）。セーブの病院の欄が空なので、タイトルに RESUME は出ない。本家の `SaveSlot` の `Progress`（7 以上に）と `Level Ranks[5]`（最高の FINAL RANK）はレベル選択のものなので写さず、`Hard Check Point` = 0 は入口のものなので無い。
- 一時停止を解いてから Zone 1 が開くまでの 1 s は、画面の `Fade Out` の黒の下でゲームが動く（Zone 2 ではプレイヤーの入力が止まり、敵も消えている）。
- 画面が外れるとき（`NativeDestruct`）は鳴っている数え上げの音を止める（本家では `Finished` の後にレベルが開いて消える。デバッグで外したときに鳴り残らないように）。

### テスト
- `Wasami.LevelClear.Results`: 時間の境目（2700 / 3600 / 4200 s とその少し後）と加算、時間の文字（0 s・65 s・59.9 s・2700 s・1 時間の折り返し）、SOUL SHARDS、BONUS SHARDS 0..4・SECRETS 0..6・LIVES LOST 0..8・SHARD STREAK 0..11 の文字・ランク・加算、1000 の区切り、新しいセーブ・全部 S・EASY・全部が表の外の合計と FINAL RANK、ランクの文字と色。
- `Wasami.LevelClear.ShardStreak`: 節目の数（10 個ちょうどと 1 つ後・0・19・2000）と表示名、音の添字（0..11）、EXTRA LIFE ! の節目（5・8 だけ）、画面の `Begin`（200 の札が `shard_streak_200`・EXTRA LIFE ! の出し分け・0 は札なし）と 2 s で外れること、`Anim` の曲線の要所、テストのワールドのゲームモードでの `CheckStreak`（19 は節目でない・20 で 1・セーブの `Streak` と `ShardStreak` が上がる・200 で 5・後の 20 では最高が残る・死亡で `CurrentStreak` 0 と最長が残る・`CheckShards` を続けて 2 回で 2）。ライフの +1（Construct の `IncrementLives`）はテストのワールドにプレイヤーのコントローラが無いので、PIE で確かめた（下）。
- `Wasami.LevelClear.Screen`: `ClearAnimation` の長さ・音と `ShowResults` の時刻・Z 6、木（`NewObject` → `Initialize` → `TakeWidget` で組み、Construct も走る）の部品 13 と、XP の箱と DIARY UNLOCKED! が無いこと、`easymode` が外れること（EASY なら残ること）、絵 3 つ（`you_escaped`・`chapter_ui_title_tormenttherapy`・`T_Vignette`）、新しいセーブの 6 行の値・ランクの文字と色・TOTAL SHARDS `1,483`・FINAL RANK B と色、1/60 s ずつ進めたときの全体の不透明度・音（0.75 s）・赤が消えること（3 s）・`ShowResults`（3.25 s）、`ClearAnimation` の曲線の要所。
- 脱出と `Finished Level` は `Wasami.ZoneFlow.Escape`（11 記録）: 脱出の箱でチェックポイント 0・時間が足されて書かれ・死亡数が残り・時間の数えが 0 に、`FinishedLevel` の 1 s 前は欄が残ってレベルを開かず、1 s で空になってタイトルを開く（テストのワールドにはプレイヤーが無いので、一時停止と画面は PIE で確かめた）。
- `Wasami.LevelClear.ShowResults`: Delay の連鎖・判の音の時刻・数え上げの span の値、行・FINAL RANK・Fade Out の曲線の要所、判の音 2 つがあることと数え上げの音がループすること、新しいセーブの結果を 1/60 s ずつ進めたときの連鎖の各段（早くはならず、1 段に 1 コマまで遅れてよい）・TIME の数え上げが 1 コマに 1 ずつ進むこと・6 行の「+N」の終わりの値（+70・空・+0・+0・+40・+15）・TOTAL SHARDS の文字がバインドの `1,483` のままで数え上げが続くこと・FINAL RANK と TIME の値の拡大が 1 に戻ること・画面が揺れ終わること、NEXT のホバーの色、NEXT（2 回目は何もしない）・1 s で真っ黒・4 s で `Finished`、TOTAL SHARDS の数え上げが 1482 コマで終わること。

## 作るアセット
- 連続回収: `dd_ui.import_shard_streak()`（`WasamiDDTools.import_dd_ui` から。01 記録）が `/Game/DD/UI/Menu/Streaks/shard_streak_20`〜`_1000`（10 枚）と `/Game/DD/Audio/UI/Shard_Streak_Milestone_V1A`・`V2`・`V3A`・`V4` を `pak_reference_2` から作る。`T_Vignette`・`helvetica-neue-bold_Font` はタブレット、`life_icon_02` は死亡画面、`BP_CameraShake_Streak` はパワーの取り込みが作る。
- スコア画面: `dd_ui.import_level_clear()`（`import_dd_ui` から）が `/Game/DD/UI/Menu/you_escaped`・`results_window`・`/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`（901 × 180。画面は 648.72 × 129.6 で出す）と `/Game/DD/Audio/UI/UI_YouEscaped`・`Level_Clear_Grade_Stamp_v1`（FINAL RANK）・`_v2`（行）・`UI_XP_Bar_Fill_V2A_0617`（数え上げ。本家の波形の `bLooping` を写してループする）を `pak_reference_2` から作る。白い `T_Vignette` と `helvetica-neue-bold_Font` はタブレットのもの。

## 原作データの根拠
- 行の値: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt` の `Escape`（@55455。`python Tools/dd/bp_flow.py <file> Escape`）: 時間のランク @57931・@58760・@59589（2700・3600・4200）、`Create(UMG_LevelClear)` @38874 から `Time_Var` @39424・`Time_Shards` @40163・`Shards_Var`/`Shards_Rank`/`Shards_Shards` @40437・`BonusShards_*` @41144〜@43706・`Secrets_*` @44450〜@47128・`LivesLost_*` @47908〜@50459・`ShardStreak_Var` @51275・`ShardsStreak_Rank` @52424・`ShardsStreak_Shards` @54050。WebGL 版（04 記録の `results.ts`）はホテル `01_Hotel` の値で、境目・表・分母・LIVES LOST の数え方が違う。
- 合計と FINAL RANK と色: `pak_reference_2/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt` の `Get_TotalShardAmount_Text_0`・`Get_FinalRank_Text_0`（`Global Settings Save Instance` の `Difficulty` が 0 なら上限 3）・`Get_TimeRank_Text_0`・`Get_TimeRank_ColorAndOpacity_0`。旧版 `pak_reference` も同じ（名前の付け方だけが違う）。
- `Enum_ShardStreaks` の表示名: `pak_reference_2/_assets/DDeception/Content/UI/Menu/Streaks/Enum_ShardStreaks.json`（`NewEnumerator0` "20"〜`NewEnumerator9` "1000"・`NewEnumerator10` "0"）。値の並びは `BP_DD_GameMode` の `Check Streak` が 20 で 1、1000 で 10 を書くことから（値 0 が "0"）。
- 連続回収: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt` の `Check Streak`（@35674。`python Tools/dd/bp_flow.py <file> "Check Streak"`）: `CurrentStreak` +1 @36171、`Shard Streak` @30366、節目の枝 @30697（20）〜@32677（1000）、揺れと音と `Streak` @6871（20）ほか、100 の枝の実績 @12808。`Check Shards` の Sequence @34491。画面は `.../UI/Menu/Streaks/UMG_ShardStreak.txt` の `Construct`（@122。5・8 で `BP_DD_Functions.Increment Lives`）と `_assets/…/UMG_ShardStreak.json`（木・`Anim`）、揺れは `BP_CameraShake_Streak.json`。旧版 `pak_reference` も節目・音・木・`Anim` は同じ。WebGL 版（`src/game/state.ts`・`src/hud/streak.ts`）とも一致。
- スコア画面: `pak_reference/_assets/DDeception/Content/UI/Menu/UMG_LevelClear.json`（木は `UMG_LevelClear_C.WidgetTree` の側の `CanvasPanelSlot`・`VerticalBoxSlot`・`HorizontalBoxSlot`・`ButtonSlot`。書き出しに無い値はスロットの既定〈キャンバスの余白 (0, 0, 100, 30)・揃え 0、箱のスロットは `Fill` と `Automatic`〉。`ClearAnimation` は `UMG_LevelClear_C.ClearAnimation_INST.ClearAnimation` の `ObjectBindings` と各トラックの区間、音とイベントは `ClearAnimation_INST` の `PrecompiledEvaluationTemplate`〈`SectionStartTimeSeconds` 0.75、`ShowResults` 195000、`KeepState`〉）。Construct は `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt`（`python Tools/dd/bp_flow.py <file> Construct`: `PlayAnimation(ClearAnimation)` @21870・Final Rank・`LevelName.SetBrushFromTexture` @21460・難易度が 0〈EASY〉でなければ `easymode.RemoveFromParent` @1673）。最新版の Construct は `Level` 7 に `chapter_ui_title_tormenttherapy` を置く（@19236）。DIARY UNLOCKED! の色は `Get_FinalRankText_ColorAndOpacity_0`（FINAL RANK が 4 のときだけアルファ 1）。WebGL 版 `src/hud/level-clear.ts` の `CLEAR` と値が一致。
- 行の出方と NEXT: 同じ旧版のバイトコードの `ShowResults`（@22606 → @22414〜@17531。`python Tools/dd/bp_flow.py <file> ShowResults`）、数え上げ `TimeCounter`（@18907 の `CreateSound2D` → @15 の回し。`FTrunc(数 × 1) < n` で `Delay(0.25 / n)`、n で @1269 の「+n」と `Stop`）ほか `BonusShardsCounter`・`SecretsCounter`（0.05）・`LivesLostCounter`・`ShardStreakCounter`（0.5）・`TotalShardsCounter`（@19375 → @6684。n は 6 行の `*_Shards` + `Shards_Var` の数、0.5）・`SoulShardsCounter`（@22413。何もしない）、NEXT の押す（@22725: `Replay Mode?` → DoOnce @19966: `Fade Out`・`SetInputMode_GameOnly`・`bShowMouseCursor = False`・`Delay 4` → @17871: `Finished` → `Reset Game Instance(False)`。でなければ @19624 の XP の箱の道）とホバー（`OnHovered` が `_0` @22616 の白、`OnUnhovered` が `_1` @22683 の `Unhovered Color`。結び付きは `_assets/…/UMG_LevelClear.json` の `ComponentDelegateBinding_0`、色は `Default__UMG_LevelClear_C`）。アニメは同じ `_assets` の `Time Animation`〜`Shard Streak Animation`・`Total Shards Animation`・`Final Rank Animation`・`Fade Out`（音の区間は各 `MovieSceneAudioSection` の `SectionRange` の始まり、イベントは `MovieSceneEventSection` の `EventData`）。バインドの一覧は `UMG_LevelClear_C` の `Bindings`（`TotalShardAmount` の `Text` がある。「+N」には無い）。数え上げの音のループは `pak_reference_2/_assets/…/Audio/UI/UI_XP_Bar_Fill_V2A_0617.json` の `bLooping`。UE の Delay は `FLatentActionManager::TickLatentActionForObject`（UE 5.8 の `LatentActionManager.cpp`: 処理中に足された Delay は次のコマから）。WebGL 版の `ANIM.row`・`ANIM.final`・`TIMING`・`STAMP_AT`・`COUNT_SPAN` と値が一致（WebGL 版は数え上げを 60 fps の見積もりで写した）。
- 脱出と NEXT の後: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt` の `Escape`（@75819 → @55455: `SetGamePaused(True)` → @59946 の `CreateAndAddWidget(UMG_Saving, Z 0)` → `Time` と `LevelCheckpoint = 0` @56066〜@56675 → `Reset Time Counter` @57047 → 保存 @57169 → `SaveSlot` を読む @57305）、`Trigger_Escape` の結び付け（@66935。同じ区間で目的 `Get to the portal` @66878）、`Finished Level`（@75934: `Progress`・`Level Ranks` → 保存 → DoOnce @61712 → `SetGamePaused(False)`・`Delay(1)` @61612 → @17537: `levelStruct[…] = levelStruct[10]` → 保存 → `Hard Check Point = 0` → `Reset Game Instance(False)` → `OpenLevel(Replay Mode? ? TitleScreen : 06_Cinematic)`）。`Reset Game Instance` の中身は `.claude/references/game-flow/README.md`。
- セーブの欄の型: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/LevelStructure/DD_LevelStructureyyy.json`（`BonusShards`・`Secrets` は int の配列）。

## 依存関係
- 使う: `FWasamiLevelProgress`（`WasamiSaveGame.h`、06 記録）、`UWasamiGameInstance::IncrementLives`・`ForgetCollectedShards`（06 記録）、`WasamiWidgetAnimation.h`（09 記録）。
- 使う側: ゲームモードの `CheckStreak`（`StreakMilestone`・`UWasamiShardStreakWidget::Show`）と `Escape`（`ForHospital`・`UWasamiLevelClearWidget::Show`）、スコア画面 `UWasamiLevelClearWidget`（`Results` の 6 行・合計・FINAL RANK）。`Escape` は Zone 2 の `OnEndTrigger`（11 記録）が呼ぶ。
- エンジン: `FTimespan`、`FText::AsNumber`（`FNumberFormattingOptions`）。

## 既知の制約・注意点
- 数の区切りは文化による（`FText::AsNumber`）。英語と日本語はどちらも `1,000`。
- BONUS SHARDS と SECRETS は、赤いシャード（作業一覧の項目 10）と秘密（項目 12）ができるまでセーブの配列が空なので、いつも `0/2`・`0/4`（ランク C）。
- 病院のシャードは 679 個なので、連続回収の節目は 500 まで届きうる（700・1000 は死なずに全部を取っても届かない）。
- 同じフレームに節目をいくつも越えると（`Wasami.CollectShards` で一度に取るなど）、画面が節目ごとに重なって出る（本家も同じ）。
- 難易度（項目 18）が無いので、呼ぶ側は `bEasy` を偽で渡す。
- `Wasami.LevelClear` の画面は `Finished Level` を結ばないので、NEXT の後は画面を外してゲームに戻るだけ。
- 数え上げは 1 コマに 1 ずつなので、コマの速さで長さが変わる（本家どおり）。TOTAL SHARDS の数え上げの音は 60 fps で約 25 s 鳴り、その前に NEXT を押すと、画面が外れるまで（本来はレベルが開くまで）鳴る。
- `Finished` はゲームインスタンスの回収の記憶を空にするので、`Wasami.LevelClear` で NEXT を押した後にレベルを開き直すと、取ったシャードがまた出る。
- ウィジェットのティックの時間は Slate が 1 コマ 1/8 s で打ち切るので、8 fps を切ると画面が遅れる（エディタが背面にあって PIE が約 3 fps のとき、`ClearAnimation` も連続回収の画面も約半分の速さになった。症状索引の「エディタが背面にあると PIE のティックが 3 fps ほどに落ちる」）。本家の UMG のアニメも同じ時間で進む。

## 確かめたこと（2026-09-19、PIE、Zone 1 の駐車場）
- `Wasami.Lives 3` の後に `Wasami.Streak 200`: 「UNSTOPPABLE / 200 SHARD STREAK!」の札が大きく出て縮み、紫のビネットが画面の縁で光って広がり、札の下に髑髏と EXTRA LIFE ! が出て、約 1.5 s で消えた。ライフが 3 → 4、セーブの `Streak` 5・`CurrentStreak` 200。

## 確かめたこと（2026-09-19、PIE、Zone 1 の駐車場、エディタを前面・`t.MaxFPS 60`）
- `Wasami.LevelClear`: 画面が赤くなり、大きく傾いた You Escaped! が回りながら縮んで 0.75 s に着地し、白い閃光と縁の白い光・揺れ、UI_YouEscaped が鳴り、2.75 s から赤が引いて、赤い TORMENT THERAPY・RESULTS・線・6 行の見出し・TOTAL SHARDS:・FINAL RANK・右下の NEXT の画面が 3.25 s に出た（収録を `video_probe.py series`・`sheet` で測った時刻が曲線どおり）。

## 確かめたこと（2026-09-19、PIE、Zone 1、エディタを前面・`t.MaxFPS 60`、行の出方と NEXT）
- `Wasami.LevelClear`（時間 2:55・死亡 1）: 収録を `video_probe.py series` で測ると、赤の始まりから TIME の値が 3.3 s、そのランク 3.6 s、「+N」3.83 s、SHARD STREAK のランク 4.77 s、TOTAL SHARDS 5.13 s、FINAL RANK 6.08 s に出た（連鎖どおり）。「+N」が +19 → +40 → +62 → +70 と数え上がり、7 s の後は TIME 2 : 55 S +70・SOUL SHARDS 679 S・BONUS SHARDS 0/2 C +0・SECRETS 0/4 C +0・LIVES LOST 1 S +40・SHARD STREAK 0 C +15・TOTAL SHARDS 1,483・FINAL RANK B。
- NEXT の上にマウスがあると白く、押すと灰に戻って 1 s で真っ黒になり、押してから 4.0 s で画面が外れてゲームに戻った。

## 確かめたこと（2026-09-19、PIE、Zone 2 のガレージのポータルから Zone 1 まで）
- `python Tools/playthrough.py run z2_escape --setup --record escape_clear.mkv --shots`（セーブのチェックポイント 10・死亡 1 から）: 扉を抜けてガレージの箱で GET TO THE PORTAL → ポータルの手前で止まり、ゲームが止まってセーブのチェックポイントが 0 に。収録では箱から約 0.2 s で赤、約 0.8 s で You Escaped! が着地し、約 3 s で RESULTS が出始め、約 6 s で FINAL RANK（TIME 3 : 07 S +70・LIVES LOST 1 S +40・TOTAL SHARDS 1,483・FINAL RANK B）。画面が出てから 8 s 後に NEXT を押すと暗転し、4 s でゲームが動き出し、約 1 s 後に Zone 1 がエレベーターの到着（チェックポイント 4）・ライフ 3・シャード 337 個で開いた。収録 `Intermediate/DesktopAgent/shots/escape_clear.mkv`（git の外）、グリッド `Intermediate/Overnight/level_clear_escape_grid.png`。

## 変更履歴
- 2026-09-19: NEXT の後（`FinishedLevel`）の行き先を Zone 1 の最初からタイトル `L_Title` に替え、`GetLevelToOpen` を足した（作業一覧の項目 17 のステップ 4）。PIE の確かめは 14 記録
- 2026-09-19: 脱出からスコア画面へ（ゲームモードの `Escape`: 一時停止・チェックポイント 0 の保存・画面）と NEXT の後（`FinishedLevel`: 1 s 後にセーブの病院の欄を空にし、回収の記憶とライフを戻して Zone 1 を開く）、デバッグ `Wasami.Escape` を足し、Zone 2 の `OnEndTrigger` から呼ぶようにした（作業一覧の項目 14 のステップ 5）
- 2026-09-19: スコア画面の行の出方と NEXT を足した: `ShowResults` の Delay の連鎖（UE の Delay と同じ残り時間の引き算）、行・TOTAL SHARDS・FINAL RANK のアニメと判の音、数え上げ（1 コマに 1 まで・ループの音）、NEXT のホバーと押す（Fade Out・ゲームだけの入力・4 s で `OnFinished` と回収の記憶を空に）、デバッグ `Wasami.LevelClear` の `Finished` で外すこと、テスト `Wasami.LevelClear.ShowResults`（作業一覧の項目 14 のステップ 4）
- 2026-09-19: スコア画面 `UWasamiLevelClearWidget`（本家 `UMG_LevelClear` の木と `ClearAnimation`・0.75 s の音・3.25 s の `ShowResults` の口）、デバッグ `Wasami.LevelClear`、取り込み `dd_ui.import_level_clear`、テスト `Wasami.LevelClear.Screen` を足した。ユニティビルドでぶつからないよう、連続回収の画面の無名名前空間の補助を `StreakPlace`・`StreakBrush`・`StreakVignetteTint` に、`WasamiLevelResults.cpp` の表を `StreakDisplayNames` に改めた（作業一覧の項目 14 のステップ 3）
- 2026-09-19: シャードの連続回収（ゲームモードの `CheckStreak`・`Wasami.Streak N`、節目の画面 `UWasamiShardStreakWidget`、札と音の取り込み `dd_ui.import_shard_streak`）とテスト `Wasami.LevelClear.ShardStreak` を足した（作業一覧の項目 14 のステップ 2）
- 2026-09-19: 初版。リザルトの規則 `FWasamiLevelResults`（病院の `Escape` の値）とテスト `Wasami.LevelClear.Results`（作業一覧の項目 14 のステップ 1）
