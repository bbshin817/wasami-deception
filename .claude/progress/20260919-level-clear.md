---
title: 脱出後のスコア表示画面（作業一覧の項目 14）
status: 進行中
branch: feature/level-clear
base: a3e64ad
started: 2026-09-19 09:10
updated: 2026-09-19 13:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 脱出後のスコア表示画面（作業一覧の項目 14）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 14（大目標 2 の最初の項目）。最終目標の「WebGL版と同じように倣う点: Escaped後のスコア表示画面」。ポータルで脱出したら、赤い You Escaped! からリザルト（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・LIVES LOST・SHARD STREAK のランクと加算シャード、TOTAL SHARDS、FINAL RANK）を、WebGL 版が原作 `UMG_LevelClear` から写した時間・アニメ・音で出し、NEXT でタイトルへ戻る（セーブは消す）。ランクの規則は本家のレベル BP が `UMG_LevelClear` に入れる値。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードを読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」。
- [x] 1. リザルトの規則（病院の値）とセーブの欄 … 2026-09-19 完了。`FWasamiLevelResults`（`WasamiLevelResults.*`）・セーブの `BonusShards`・`Secrets`・テスト `Wasami.LevelClear.Results`。実装記録は新しい **13-level-clear**（画面・連続回収もここへ書き足す）。
- [x] 2. シャードの連続回収 … 2026-09-19 完了。ゲームモードの `CheckStreak`（`Check Shards` から毎回）・`Wasami.Streak N`、節目の画面 `UWasamiShardStreakWidget`、`dd_ui.import_shard_streak`、テスト `Wasami.LevelClear.ShardStreak`。中身は 13 記録。WebGL 版と値の違いは無かった（ライフ +1 は `Check Streak` でなく画面の Construct）。
- [x] 3. スコア画面の素材と、ウィジェットの木と ClearAnimation … 2026-09-19 完了。`UWasamiLevelClearWidget`（旧版の木と `ClearAnimation`・0.75 s の音・3.25 s の `ShowResults` の口）・デバッグ `Wasami.LevelClear`・`dd_ui.import_level_clear`・テスト `Wasami.LevelClear.Screen`。中身は 13 記録。XP の箱と DIARY UNLOCKED!（`FinalRankText`）は作らなかった。
- [x] 4. リザルトの出方と NEXT … 2026-09-19 完了。`ShowResults` の Delay の連鎖・行と TOTAL・FINAL RANK のアニメと判の音・数え上げ・NEXT（Fade Out → 4 s で `OnFinished`、回収の記憶を空に）、テスト `Wasami.LevelClear.ShowResults`。中身は 13 記録。
- [ ] 5. 脱出からスコア画面へ、NEXT からタイトル（の代わり）へ
  - `AWasamiZone2Flow::OnEndTrigger`（11 記録。今は暗転して止まる）の後に、病院の `Escape`（下）: `SetGamePaused(true)`、セーブの `Time += ゲームモードの Time`・`LevelCheckpoint = 0`・`ResetTimeCounter`・書く、ステップ 1 の規則で値を作ってスコア画面を Z 6 で出す。暗転からスコア画面までの間は本家のホテルの `EndTrigger` どおり（すぐ）か、黒のフェードが黒になるのを待つかを、今の `OnEndTrigger` の順で決める。
  - `Finished` → 病院の `Finished Level`（下）: `SetGamePaused(false)` → 1 s → セーブの病院の欄を空にして書く → ゲームインスタンスを戻す（ライフ 3・回収の記憶を空に。死亡画面の QUIT TO TITLE と同じ）→ タイトル（項目 17）ができるまでは Zone 1 を開く（09 記録の QUIT TO TITLE の代わりの道と同じ。項目 17 で `TitleScreen` に替える）。本家の `SaveSlot` の `Level Ranks`・`Progress`（レベル選択用）と実績は写さない。
  - デバッグのコンソールコマンド `Wasami.Escape`（その場で脱出の流れを始める。確かめ用）。テスト `Wasami.ZoneFlow.Escape` を直す。
  - PIE: `python Tools/playthrough.py run --from z2_escape --setup`（01 記録）の後にスコア画面 → NEXT → Zone 1 まで流し、`--record` で撮る。Discord に連番のグリッド（You Escaped! → リザルト → FINAL RANK）。台本の区間 `z2_escape` の終わりをスコア画面と NEXT まで延ばす。
  - 変更予定: `WasamiZone2Flow.*`、`WasamiGameMode.*`、`Tests/WasamiZoneFlowTests.cpp`、`Tools/playthrough.py`、実装記録 11・06・13・01
- [ ] 6. 閉じる
  - 作業一覧の項目 14 を完了にし（完了の条件の読み替えを書く）、実装記録（13・06・11・索引）と handover の「現状と次の一歩」を直す。note の原稿に「脱出するとスコア画面」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 5（脱出からスコア画面へ、NEXT から Zone 1 へ）を始める。読むもの: 11 記録の `AWasamiZone2Flow::OnEndTrigger`（今は暗転して止まる）、06 記録のセーブ（`FWasamiLevelProgress`・`ResetTimeCounter` の有無）と死亡画面の QUIT TO TITLE の代わりの道（09 記録。ライフ 3・セーブの病院の欄を空にして Zone 1 を開く）、上の「本家の流れ」の `Escape` と `Finished Level`。スコア画面は `UWasamiLevelClearWidget::Show(World, FWasamiLevelResults::ForHospital(...))` で出し、`OnFinished` に `Finished Level` を結ぶ（13 記録）。

## 本家の流れ（読んだもの）

- **病院のリザルトの値**は `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt` の `Escape`（@55455）。6 行の値は 13 記録に写した（`FWasamiLevelResults`）。
  - `Escape` の順: `SetGamePaused(True)` → `levelStruct[5].Time += ゲームモードの Time`・`LevelCheckpoint = 0` → `Reset Time Counter` → `structSlot` に保存 → `SaveSlot` を読む → 時間のランク → `Create(UMG_LevelClear)` → 値を入れる → `Final Rank` が 4（S）なら実績 → `Finished` を `Finished Level` に結ぶ → `AddToViewport(6)` → 実績 `06_Trap` の確かめ。
  - `Finished Level`（@75934）: `SaveSlot` の `Progress` を 7 以上に・`Level Ranks[5]` を最高に → 保存 → DoOnce → `SetGamePaused(False)` → 1 s → `levelStruct[5]` を空の要素（`levelStruct[10]`）で上書きして `structSlot` に保存 → `Hard Check Point` = 0 → `Reset Game Instance(False)` → `Replay Mode?` なら `TitleScreen`、でなければ `06_Cinematic` を開く。
- **ウィジェット `UI/Menu/UMG_LevelClear`**: 旧版（`pak_reference`）と最新版（`pak_reference_2`）の両方にあり、関数の並びは同じ（`Construct`・`ShowResults`・`*Counter` 7 つ・`Get_*` 26・`Level Up`・`Update`・NEXT のホバー 2 と押す 1）。`ShowResults` の Delay の連鎖は最新版も WebGL 版の `TIMING` と同じ（行 0.25 s ずつ、SHARD STREAK の後 0.5 s で TOTAL、1 s で FINAL RANK、1 s で UI 入力）。NEXT は `Replay Mode?` なら DoOnce → `Fade Out`・ゲームだけの入力・カーソルを消す → `Virtual Cursor(False)` → 4 s → `Finished` → ゲームモードの `Reset Game Instance(False)`。`Replay Mode?` でなければ 1 回目は DoOnce → `StopAnimation(ClearAnimation)` → FlipFlop の A で `XP Box Animation` と `TotalSoulShardsXP` の数え上げ（0.002 × (`SaveSlot` の `Total Shards` + 合計) の歩み）、数え終わると 1.5 s 後にその DoOnce を開け、2 回目の NEXT が FlipFlop の B で `Replay Mode?` と同じフェードの道へ（最新版 @20708 → @18304 → @17684、@13381 → @13452）。
- 画像: `pak_reference_2/DDeception/Content/UI/Menu/you_escaped.png`・`results_window.png`、`UI/Menu/TitleCards/chapter_ui_title_tormenttherapy.png`。音: `Audio/UI/UI_YouEscaped.ogg`・`Level_Clear_Grade_Stamp_v1.ogg`・`_v2.ogg`・`UI_XP_Bar_Fill_V2A_0617.ogg`。連続回収: `UI/Menu/Streaks/`（`UMG_ShardStreak`・`Enum_ShardStreaks`・`T_Vignette`・`shard_streak_*`・`BP_CameraShake_Streak`）。

## 決定事項

- 2026-09-19: **リザルトの値は病院のレベル BP `06_Hospital` の `Escape` から取る**（WebGL 版が写したホテルの値ではない）— 作業一覧の完了の条件「ランクの規則は本家のレベル BP が `UMG_LevelClear` に入れる値」。本作のステージは病院で、病院のレベル BP が自分の値（時間の境目 2700 / 3600 / 4200 s、シャード 679 など）を入れている。作業一覧の根拠（`pak_reference` の `01_Hotel.txt`）は病院を知る前の書き方と読み、閉じるときに直す。
- 2026-09-19: **ウィジェットの木・アニメ・音・時間は WebGL 版と同じ（旧版の `UMG_LevelClear`）にし、最新版と違う所があれば旧版を採る** — 最終目標の「WebGL版と同じように倣う点: Escaped後のスコア表示画面」（ユーザーの原文）。ステップ 1 で比べた差（書き出しの木・スロット・アニメのキー・バインド関数）: XP の箱（最新版は `ProgressBar_0` を `Overlay_0` で包み、アニメ `FadeOutXPBar` がある。作らない）、You Escaped! の画像の名前（`Image_216` → `escaped`。中身は同じ）、`ClearAnimation` の後半のキー（最新版は `escaped` の不透明度を 3.0 → 4.35 s で 0 へ、`easymode` を 4.35 s に 1 のキー。`escaped` は親の `ClearLevel` が 2.75 → 3.0 s で消えた後なので見えない）、`easymode` の位置（旧版 (−213.86, −74.58)・最新版 (−96.27, 51.74) で自動の大きさ。EASY MODE は出さない）。見える差は無いので要確認にしない。バインド関数・数え上げ（`TimeCounter` ほか）・NEXT は名前の付け方だけが違う。
- 2026-09-19: **NEXT は WebGL 版と同じく、押すと Fade Out → 4 s → `Finished` の道だけにし、XP の箱（`XP Box Animation`・`Level Up`・プレイヤーのレベル）は作らない** — WebGL 版と同じ。本作のパワーは最初から Lv5 固定で、シャードでレベルを上げる仕組みが無い（最終目標について決めたこと）。
- 2026-09-19: **シャードの連続回収（`Check Streak` と `UMG_ShardStreak`）をこの項目に入れる** — SHARD STREAK の行の値の元で、今は数えておらず（いつも 0 で C になる）、作業一覧のどの項目にも無い。200・500 でライフが増える規則でもある。作業一覧の項目 14 の規模を 1 → 2 にした。
- 2026-09-19: **NEXT の行き先は、タイトル（項目 17）ができるまで Zone 1 の最初**（死亡画面の QUIT TO TITLE と同じ代わりの道。09 記録）。項目 17 で `TitleScreen` に替える。本家の `SaveSlot` の `Level Ranks`・`Progress`（レベル選択用）と実績は写さない（本作にレベル選択と実績が無い）。
- 2026-09-19: BONUS SHARDS・SECRETS は、赤いシャード（項目 10）と秘密（項目 12）ができるまで 0 のまま（0/2・0/4、ランク C）。EASY MODE の文字は難易度（項目 18）ができるまで出さない。
- 2026-09-19: **DIARY UNLOCKED!（`FinalRankText`。FINAL RANK が S のときだけ見える）は作らない** — 本作に日記が無く、WebGL 版も出さない。`Final Rank Animation` の `FinalRankText` のトラックも写さない。

## 要確認（ユーザー）

- 2026-09-19: TOTAL SHARDS は本家のコードどおり **679 を 2 回数える**（病院のレベル BP が `Shards_Shards` にも 679 を入れ、画面の合計は `Shards_Shards` と `Shards_Var` の文字の両方を足す。新しいセーブで 1483、全部 S で 1558）。本家の書き誤りらしいが、ゲームの規則の値なのでコードに従った。1 回にしたいときは `FWasamiLevelResults::GetTotalShards` を直す。場所: 13 記録。
- 2026-09-19: スコア画面の上のレベルの題字 — 仮に本家の病院の題字 `chapter_ui_title_tormenttherapy`（「Torment Therapy」の飾り文字。本家どおり赤く染める）を使う計画にした。理由: 本作のステージは本家の病院そのもので、題字はロゴでもキャラクターでもない UI の文字（WebGL 版の `you-escaped` と同じ扱い）。WebGL 版はステージが違ったので、ユーザーの題字「Stinky Gachimi」を使っていた。場所: 13 記録（`LevelNameTexture`）。ステップ 3 でそのとおり作った。

## 再開時の注意

- エディタは起きていて（ステップ 4 のビルドで開き直した）、Zone 1（`L_Hospital_Zone1`）を開いている。PIE なし（2026-09-19 13:20）。エディタは前面。テストを回すとテストの結果の窓がビューポートの左に重なるので、撮る前に最小化する（窓の最小化のボタン）。
- テストは MCP の `AutomationTestToolset.AutomationTestToolset` の `DiscoverTests` → `RunTestsByFilter`（引数 `filterExpression`: `StartsWith:Wasami.LevelClear` など）で回す。エディタを開き直すと MCP のセッションが切れるので、`ToolSearch` で `+unreal-mcp call_tool` を引き直す。
- 取り込んだアセットは `/Game/DD/...`（原作から作り直せるので git の外）。取り込みは `dd_ui.py` に足し、`WasamiDDTools` から呼ぶ（01 記録）。
- PIE の収録: **エディタを前面にしてから**（端末が前面だとエディタが約 3 fps に落ち、Slate の 1/8 s の打ち切りで画面が半分の速さになる。症状索引）。前面にするのは `python Tools/desktop.py click 2957 95 --allow UnrealEditor.exe --allow WindowsTerminal.exe`（タイトルバーの空き）。`python Tools/pie.py cmd "t.MaxFPS 60"` → `desktop.py record --grab gdi --region 1822 206 2862 858 --fps 60` → `pie.py cmd "Wasami.LevelClear"` → `video_probe.py sheet`。終わったら `t.MaxFPS 0` と `pie.py stop`。`Wasami.LevelClear` の画面は 7 s 後に NEXT（ビューポート 1040 × 652 のとき画面の (2810, 824)）で押せ、4 s 後に外れる。
- C++ のファイルを足したら、コミットの前に無名名前空間の名前の重複を洗う（症状索引の「ファイルを足したらビルドが落ちた」）。ステップ 3 で `tmp/anon_dupes.py` を書いた（git の外）。

## 検証

- check_records: OK（ステップ 4、2026-09-19）
- C++ ビルド: 成功（ステップ 4）。テスト `Wasami.LevelClear.*` 4 件が通った
- PIE（ステップ 4）: `Wasami.LevelClear` で行・TOTAL SHARDS・FINAL RANK が連鎖どおりの時刻に出て、NEXT で暗転して 4 s で外れた（13 記録の「確かめたこと」）
