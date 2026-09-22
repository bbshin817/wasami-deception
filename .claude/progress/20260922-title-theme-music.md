---
title: タイトル画面の曲を本家（最新版）のテーマ曲にする（作業一覧の項目 43）
status: 進行中
branch: main
base: 9499bc6
started: 2026-09-22 16:30
updated: 2026-09-22 17:10
---

# タイトル画面の曲を本家（最新版）のテーマ曲にする

## 依頼

2026-09-22 の有人セッションで、ゲームレビュアーの指摘（Medium）:

> タイトル画面のBGMが本家と異なる

本作は本家の**旧版**の `UMG_TitleScreen` を写した（WebGL 版がそうしていたため）ので、曲がポーズ画面と同じ `Pause_Sound_v1`（ピッチ 0.5）になっている。同じ日のユーザーの回答「**曲も絵も最新版に寄せる**」で、最終目標の「タイトル画面は WebGL 版に倣う」より指摘を優先すると決まった（作業一覧の項目 43・44）。

完了の条件（作業一覧の項目 43）:

1. `DD_-_Dark_Deception_-_Theme_v1_3.ogg` を `/Game/DD/Audio` に取り込む（`dd_ui` の取り込みの一覧に足す）。
2. タイトルの曲をそれに替え、音量 0.6 → `FadeIn(2.0, 0.5)` を最新版のとおりにする。EXTRAS の曲の一覧（`Pause_Sound_v1` を「Pause Theme」として出している）とポーズ画面はそのまま。
3. パッケージ版でタイトルを開き、本家と同じ曲が同じ入り方で鳴ることを確かめる。

## 計画

- [x] 1. 計画（この記録を作る）… 2026-09-22 完了。下の「決定事項」に本家の最新版の値。
- [x] 2. 曲を取り込む … 2026-09-22 完了。`dd_ui.TITLE_SOUNDS` に足して `import_title()`（音 4 つ）→ `sound_classes()`（`changed: 0`）。`/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3` は音量 1・ピッチ 1・ループ・2 ch・213.99 s・`DD_SoundClass_Music` で、最新版の export と一致。記録 01・09 を直して `check_records --update` を通した。
- [ ] 3. タイトルのコードを最新版の値にする ← 次
  - 変更予定: `Source/wasami_deception/WasamiTitleScreenWidget.cpp`・`.h`（`MusicSound`・`MusicVolume`・`MusicPitch`、頭のコメント）、`Source/wasami_deception/Tests/WasamiSettingsTests.cpp`（サウンドクラスの一覧に足す）、実装記録 14（必要なら 10）
  - `python Tools/editor_cycle.py` でビルドし直し、PIE でタイトルを開いて曲が鳴ることと、2 秒で 0.5 まで上がる入り方を確かめる（`Tools/video_probe.py` で録って測る）。ポーズ画面と EXTRAS の「Pause Theme」が変わっていないことも見る。
- [ ] 4. パッケージ版で確かめる（完了の条件 3）
  - `RunUAT.bat BuildCookRun` の許可が出ていれば、項目 39・41 の止まっている確かめとまとめて 1 回のパッケージで見る。許可がまだなら、この項目は PIE までで閉じて「要確認」に残す。

## 次にやること

ステップ 3。`Source/wasami_deception/WasamiTitleScreenWidget.cpp` の `MusicSound`（130 行目）を `/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3` に、`MusicVolume` を 1.0 → 0.6、`MusicPitch` を 0.5 → 1.0 にし（`.h` の説明も）、`Tests/WasamiSettingsTests.cpp`（152 行目あたりのサウンドクラスの一覧）に新しい曲を足す。`python Tools/editor_cycle.py` でビルドし直し、PIE でタイトルの曲と 2 秒のフェードインを `Tools/video_probe.py` で測って確かめ、ポーズ画面と EXTRAS の「Pause Theme」が変わっていないことも見る。実装記録 14（必要なら 15・19）を直す。

## 決定事項

- 2026-09-22: **本家の最新版の値**（`pak_reference_2/_bytecode/.../UMG_TitleScreen.txt` の @6598・@3429・@2472 を `Tools/dd/bp_flow.py` で読んだ）: `CreateSound2D(Self, DD_-_Dark_Deception_-_Theme_v1_3, 0.6, 1, 0, None, False, True)` → `SetSound(同じ曲)` → `FadeIn(2, 0.5, 0, 0)`。本作は今 `Pause_Sound_v1`・音量 1.0・**ピッチ 0.5**・`FadeIn(2, 0.5)` なので、**替えるのは曲・音量（1.0 → 0.6）・ピッチ（0.5 → 1.0）の 3 つ**で、フェードインはすでに最新版と同じ。旧版（`pak_reference`）は `Pause_Sound_v1` を鳴らしており、本作はそちらを写していた。
- 2026-09-22: **`Pause_Sound_v1` は取り込みからも参照からも外さない** — ポーズ画面（`WasamiPauseWidget`）と EXTRAS の 4 曲目「Pause Theme」（`WasamiExtrasWidget`）が今も使う。タイトルの `MusicSound` だけを替える。
- 2026-09-22: 取り込みは `dd_ui.TITLE_SOUNDS` に足すだけでよかった（ステップ 2 で実施）。曲は `/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3`。

## 要確認（ユーザー）

- 2026-09-22: **完了の条件 (3)（パッケージ版での確かめ）には `RunUAT.bat BuildCookRun` の許可が要る**（作業一覧の「未回答の要確認」の `20260922-cooked-engine-assets` と同じ件。項目 39・41 も同じところで止まっている）。許可が出れば 3 項目まとめて 1 回のパッケージで確かめられる。

## 再開時の注意

- ステップ 3 で `python Tools/editor_cycle.py`（C++ のビルド、10 分前後）を走らせる。先にこの記録へ走らせる手順を書いてから始める。
- エディタは起きている（2026-09-22 17:00 に `L_Hospital_Zone2`・PIE なし・dirty なし）。曲の取り込みは済んでいるので、ステップ 3 は C++ だけ。

## 検証

- check_records: 2026-09-22 OK（20 件）
- C++ ビルド: 未実行（ステップ 3）
- エディタでの確認: 2026-09-22 取り込み OK（上のステップ 2）。PIE は未実行（ステップ 3）
