---
title: 項目 40: 捕獲の音を本家の音源からワサミの音源に替える
status: 進行中
branch: main
base: d0a01e8
started: 2026-09-22 12:51
updated: 2026-09-22 13:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 項目 40: 捕獲の音を本家の音源からワサミの音源に替える

## 依頼

作業一覧 `.claude/roadmap.md` の「大目標 4: レビュー指摘の修正」の項目 40。レビュアーの指摘「敵ワサミ襲撃時について、本家の音源が再生されている。ここはワサミの任意の音源へ差し替える」。完了の条件は作業一覧の項目 40（当てる声は 2026-09-22 のユーザーの回答）。

## 計画

- [x] 1. 調べて計画を立てる … 2026-09-22 完了。
- [x] 2. `you` を取り込み、`EWasamiVoice::You` を足す … 2026-09-22 完了（12 本目の声、字幕なし、5.007 s）。
- [x] 3. 捕獲の音を差し替える … 2026-09-22 完了。ホテル型 3 本 = `You`（t = 0）、顔 = `Over`（0.2 s）。1 本の捕獲が鳴らすのは 1 つだけになったので添字の作りをやめ、`VoiceTime`・`GetVoice`・`GetVoiceDelay`・`PlayCaptureVoice`・`VoiceTimer`・`HotelVoiceSound`・`FaceVoiceSound` にした。`dd_enemy.CAPTURE_SOUNDS`・`import_capture_sounds` を外した（`sounds` 4 → 1）。テストは `Wasami.Capture.Sound` → `Wasami.Capture.Voice`。実装記録 07・01・10 を更新。
- [ ] 4. PIE で 4 本とも確かめて閉じる
  - `python Tools/pie.py start` → コンソールコマンド `Wasami.Capture <n>`（0〜3。`WasamiGameMode.cpp:74`）を 4 回。声が鳴ること・死亡画面と重なり方が許せることを耳と収録で見る。
  - 作業一覧の項目 40 を「完了（2026-09-22）」にし、handover の「現状と次の一歩」を直し、この記録を消して最後のコミットに含める。

## 次にやること

ステップ 4。PIE で捕獲を 4 本とも走らせ、ホテル型 3 本が `You`、顔が `Over` を鳴らすことと、死亡画面との重なり方が許せることを確かめる。許せなければ「決定事項」の 2 つを見直して要確認に書く。終わったら作業一覧・handover を直してこの記録を消す。

## 決定事項

- 2026-09-22: **ホテル型 3 本の `You` は `DeathDelay`（3.5 s）をはみ出すが止めない**（`PlaySound2D` のまま） — 波を測ると台詞そのものは約 0.8 s で終わり、残りは残響の尾（ピーク比 −40 dB を 2.26 s、−60 dB を 3.36 s で下回り、3.5 s で約 −81 dBFS）。**ステップ 4 で死亡画面の後に尾が残って聞こえても、それ自体は直さない**。
- 2026-09-22: **顔は `Over` 1 本だけ、`SoundTime = WatcherAnimDelay`（0.2 s）** — 本家の笑いと同じ掴む瞬間。0.2 + 0.727 = 0.927 s で終わり、死亡の 1.15 s の前に収まる。本家の斧の位置（1.05 s）は空けた。
  - **ステップ 4 で見ること**: `Over` は**ライフ 0 のときの死亡画面でも鳴る**ので、顔の捕獲で死に切ると 1 s ほどの間に 2 回続く。許せなければ要確認に書く（顔だけ別の声にする / 死亡画面の側を鳴らさない の 2 案）。

## 要確認（ユーザー）

- 2026-09-22: **捕獲の `you` に字幕を出すか** — 仮に**出さない**（`dd_voices.SILENT`）。理由: WebGL 版は `you` を鳴らさず前例が無い。捕獲は画面いっぱいの演出で、死亡画面の `Over` も字幕を出していない。場所: `Content/Python/wasami_tools/pipeline/dd_voices.py` の `SILENT`。
- 2026-09-22: **手元に残る本家の捕獲の 3 本（`/Game/DD/Audio/01_Hotel/Evil_Monkey_Scream`・`/Game/DD/Audio/03_Manor/LIVING_STATUE_Laughter_05`・`Axe_Hit_03`）のアセットを消すか** — 仮に**消さない**（取り込みの一覧から外すだけ）。理由: アセットの削除は無人運転では行わない決まり。`/Content/DD/` は git の外なので、消しても前処理で作り直せる。場所: `Content/DD/Audio/01_Hotel/`・`Content/DD/Audio/03_Manor/`。

## 再開時の注意

- C++ を変えたら `python Tools/editor_cycle.py`（閉じてビルドし開き直す。今回は 75 s + 17 s と 7 s + 16 s）。
- **テストの回し方**（MCP が切れているとき）: リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` → `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests Wasami.Capture')` → `Saved/Logs/wasami_deception.log` の `Test Completed` を読む → 真に戻す。エディタを前面に出さずに通った（済み）。

## 検証

- C++ ビルド: 成功（2026-09-22。警告 0 — `WasamiCapture.cpp` に残っていた C4305 の 5 つも `static_cast<float>` で消した）
- テスト: `Wasami.Capture` の 5 つ緑（`Camera`・`Catch`・`NoRepeat`・`Room`・`Voice`。2026-09-22）
- 前処理: エディタで `dd_enemy`・`dd` を reload して `CAPTURE_SOUNDS`・`import_capture_sounds` が無いことを確かめた（2026-09-22）
- check_records: OK（20 件。2026-09-22）
- PIE: 未実行（ステップ 4）
