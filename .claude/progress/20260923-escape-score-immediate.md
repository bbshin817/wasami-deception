---
title: 項目 49 脱出のスコア画面を本家どおり即時に出す
status: 進行中
branch: main
base: 2311ca0
started: 2026-09-23 04:34
updated: 2026-09-23 05:35
---

# 項目 49 脱出のスコア画面を本家どおり即時に出す

## 依頼

作業一覧 `.claude/roadmap.md` の項目 49（大目標 4）。2026-09-23 のユーザーの回答「即時」。

いまはポータルに触れてからスコア画面まで 1 s（PIE の実測 1.026 s）画面が黒く、曲のフェードを聞かせている。本家 `06_Hospital` の `Trigger_Escape` は触れた所で `Escape` を呼ぶ（@66935）ので、本作もその場でスコア画面を出す。

完了の条件（作業一覧の原文の要旨）:

1. `AWasamiZone2Flow` の脱出が、引き金からスコア画面までを待たずに出す（`After(EscapeMusicFade, …)` の待ちを外す）。
2. **曲は聞こえる形で引いたまま**にする（2026-09-21 のユーザーの回答）。`FadeAllMusicOut(EscapeMusicFade)` は残し、スコア画面の下でフェードが続く形にする。スコア画面の待ちと曲のフェードを結んでいた `static_assert`（`EscapeMusicFade == AWasamiMusicPlayer::FadeDuration`）は意味が変わるので見直す。
3. PIE で脱出を録り、ポータルに触れてからスコア画面が出るまでに黒い間が無いことと、曲が切れずに引いていることを確かめる。テスト `Wasami.ZoneFlow.*` を直す。

## 計画

- [x] 1. 脱出を即時にした（C++）。`Escape(PauseDelay)` が一時停止だけを遅らせ、保存と画面は引き金のフレームに来る。ビルド通過、`Wasami` 157 件で関係するものは全部成功
- [ ] 2. PIE で脱出を録って確かめ、記録を直す ← 次
  - 変更予定: `.claude/implementation-records/10-audio.md`・`11-stage-zone2.md`・`13-*`（触れた分だけ）、`.claude/roadmap.md`（項目 49 を完了に）、この進捗記録を削除
  - 撮り方は実装記録 13 の脱出の録り（`Tools/pie.py` + `Tools/video_probe.py`）。黒い間（触れた時刻 → スコア画面が出た時刻）と曲の波を測る

## 次にやること

ステップ 2。PIE で脱出を録って確かめる。`python Tools/playthrough.py run z2_escape --setup --record escape_immediate.mkv --shots`（13 記録の脱出の録り。セーブのチェックポイント 10 から）で撮り、`Tools/video_probe.py` で 2 つ測る:

1. **黒い間が無いこと** — ポータルの箱に触れたコマからスコア画面（赤・`You Escaped!`）が出始めるコマまで。前は 1.026 s（11 記録の「脱出の間合い」）。0.25 s の黒のフェードの下でスコア画面が同時に立ち上がるはずなので、黒だけの間は 0.25 s 前後まで縮む。
2. **曲が切れずに引いていること** — 触れてから 1 s の波。`video_probe.py series` で振幅が段々に落ちて 0 になり、途中で急に 0 にならないこと。

測ったら 11 記録の「脱出の間合い」（項目 35 の測り）と 10 記録の検証を新しい数字で置き換え、13 記録の `Wasami.LevelClear` の録りの記述も要れば直す。`.claude/roadmap.md` の項目 49 を完了にし、この進捗記録を削除して最後のコミットに含める。

## 決定事項

- 2026-09-23: **遅らせるのは一時停止だけにした**（`AWasamiGameMode::Escape(float PauseDelay = 0.f)`）。止めたゲームは UI でない音を 1 つも鳴らさない（`FAudioDevice::HandlePause`）ので、曲を聞かせるには「音を UI 扱いにする」か「その間は止めない」かの二択。前者は `bIsUISound` が再生を始めるときにしか `FActiveSound` に写らず、鳴っている曲には後から効かない（効かせるには鳴らし直すしかなく、曲が頭に戻る）。そこで後者にし、**保存とスコア画面は引き金のフレームのまま**・一時停止だけを `EscapeMusicFade`（1 s）後にした。その 1 s は入力が切れていて敵も消えているので、動くものは残っていない。
- 2026-09-23: 待ちの間に NEXT が押されたときは止め直さない（`PauseAfterEscape` が `bLevelFinished` を見る。`FinishedLevel` 側でもタイマーを消す）。`FinishedLevel` は止めを解く側なので、後から止めると画面が固まる。
- 2026-09-23: `static_assert`（`EscapeMusicFade == AWasamiMusicPlayer::FadeDuration`）は残した。曲のフェードの長さ＝遅らせる長さなので縛りの意味は変わらない。文言だけ直した。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 走らせたままの処理は無い。エディタは開き直して応答する。
- **テストの回し方**（素の `UnrealEditor-Cmd.exe` は 1 件も走らずに落ちる。症状索引に書いた）: `python Tools/console_session.py "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<uproject>" -ExecCmds="Automation RunTests Wasami;quit" -Unattended -NullRHI -NoSplash -ABSLOG="<絶対パス>.log" --wait UnrealEditor-Cmd.exe` を**エディタを閉じてから**回し、`tasklist` から消えるのを待ってログの `Test Completed` を読む。`-NoSound` は付けない（症状索引）。
- 本家の根拠: `pak_reference_2/_bytecode/…/06_Hospital.txt` の `Trigger_Escape`（@66935）、`06_Hospital_Zone_02.txt` の脱出（`Postmaze_Trigger_Ambulance` @1511・音楽 @1423）。

## 検証

- check_records: OK（20 件。02・11 記録のハッシュを更新）
- C++ ビルド: 成功（`Tools/editor_cycle.py`、12 手順）
- テスト: `Automation RunTests Wasami` を 157 件。`Wasami.ZoneFlow.Escape`・`Zone2`・`Wasami.GameFlow.*` はすべて成功。落ちた 4 件（`Defib.Charge`・`Enemy.Actor.Chase06`・`ZoneBarrier.Actor`・`ZoneFlow.Zone1`）は `-NullRHI` で粒子が動かない既知のもの（症状索引の 2 件）で、今回の変更とは関係しない
- エディタでの確認（PIE の録り）: 未実行（ステップ 2）
