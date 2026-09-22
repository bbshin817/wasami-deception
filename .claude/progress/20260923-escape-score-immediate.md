---
title: 項目 49 脱出のスコア画面を本家どおり即時に出す
status: 進行中
branch: main
base: 2311ca0
started: 2026-09-23 04:34
updated: 2026-09-23 04:34
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

- [ ] 1. 脱出を即時にし、曲のフェードが一時停止の下でも聞こえるようにする（C++） ← 次
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.cpp`・`.h`（`Escape` の `After` の待ち・`EscapeMusicFade` の doc・`static_assert`）、要るなら `Source/wasami_deception/WasamiMusicPlayer.cpp`・`.h`（一時停止の下で鳴らす扱い）、`Source/wasami_deception/Tests/WasamiZoneFlowTests.cpp`（`Wasami.ZoneFlow.Escape`）
  - `python Tools/editor_cycle.py` でビルドし、`Wasami.ZoneFlow.*` を通す
- [ ] 2. PIE で脱出を録って確かめ、記録を直す
  - 変更予定: `.claude/implementation-records/10-audio.md`・`11-stage-zone2.md`・`13-*`（触れた分だけ）、`.claude/roadmap.md`（項目 49 を完了に）、この進捗記録を削除
  - 撮り方は実装記録 13 の脱出の録り（`Tools/pie.py` + `Tools/video_probe.py`）。黒い間（触れた時刻 → スコア画面が出た時刻）と曲の波を測る

## 次にやること

ステップ 1。`WasamiZone2Flow.cpp:488` の `After(EscapeMusicFade, …)` を外して `GetMode()->Escape()` をその場で呼ぶ。**ただし `AWasamiGameMode::Escape` は最初に `SetGamePaused(true)` する**（`WasamiGameMode.cpp:491`）ので、そのままだと `FAudioDevice::HandlePause` が UI 以外の音源を止め、直前に始めた `FadeAllMusicOut(EscapeMusicFade)` のフェードが聞こえないまま曲が固まるおそれがある（完了の条件 2 に反する）。一時停止の下でも鳴らす手（音のコンポーネントを UI サウンド扱いにする、など）を実装時に決め、PIE の波で確かめる。

## 決定事項

- 2026-09-23: 本家どおり「触れた所で `Escape`」にする — 本家 `06_Hospital` の `Trigger_Escape` が待たずに `Escape` を呼ぶ（`WasamiZone2Flow.cpp:481` のコメントの @66935）。いまの 1 s の待ちは本作が曲のフェードを聞かせるために足したもので、ユーザーの回答「即時」で不要になった。
- 2026-09-23: 曲のフェードは残す — 2026-09-21 のユーザーの回答「曲は聞こえる形で引いたまま」。フェードを聞かせる置き場が「黒い間」から「スコア画面の下」に変わるだけ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。C++ を変えるので、ステップ 1 の実装後に `python Tools/editor_cycle.py`（エディタを閉じてビルドして開き直す。完了は起動後に `Tools/ue_remote.py` が答えること）。
- 本家の根拠: `pak_reference_2/_bytecode/…/06_Hospital.txt` の `Trigger_Escape`（@66935）、`06_Hospital_Zone_02.txt` の脱出（`Postmaze_Trigger_Ambulance` @1511・音楽 @1423）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
