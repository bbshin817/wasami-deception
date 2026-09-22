---
title: タイトル画面の曲を本家（最新版）のテーマ曲にする（作業一覧の項目 43）
status: 進行中
branch: main
base: 9499bc6
started: 2026-09-22 16:30
updated: 2026-09-23
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
- [x] 3. タイトルのコードを最新版の値にする … 2026-09-22 完了。`MusicSound` を `/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3`・`MusicVolume` 0.6・`MusicPitch` 1.0 にし、`.h` のコメント 2 か所と実装記録 14・15・19 を直し、テスト 2 つ（`Wasami.Title.Screen` の音の値、`Wasami.Settings` のサウンドクラスの一覧）に足した。ビルド OK（`editor_cycle.py`）、`Automation RunTests Wasami.Title+Wasami.Settings` は 9 件すべて Success、PIE の `L_Title` で鳴っている音は新しい曲 1 つだけ（下の「検証」）。
- [ ] 4. パッケージ版で確かめる（完了の条件 3）← **ユーザー待ち**
  - `RunUAT.bat BuildCookRun` の許可が出たら、項目 39・41 の止まっている確かめとまとめて 1 回のパッケージで見る（下の「要確認」）。

## 次にやること

ステップ 4（**2026-09-23 に `RunUAT.bat BuildCookRun` の許可が出たので進められる**）: パッケージ版でタイトルを開き、新しい曲が 2 秒のフェードインで鳴ることを確かめる。**パッケージは 2026-09-23 に項目 39 の反復が `d348ce5` から作り直してある**（`Saved/Archive/Windows/wasami_deception.exe`・`Saved/StagedBuilds/Windows`。クックは 0 エラー、`/game` 1147 件）。**`main` がそれより進んでいなければ作り直さずにそのまま使う**（`git log --oneline d348ce5..HEAD` でアセットやコードが変わっていないか見る）。項目 47（マスターの用途フラグ）は材質を直すので、その後にもう 1 回作り、項目 52 はそれを使う。

パッケージ版の絵の撮り方（項目 39 で通した）: 画面への入力は要らない。`Tools/game_perf.py` の `launch(map, commands, timeout)` が対話デスクトップで起動して終わりを待つので、`-ExecCmds` を `t.MaxFPS 60` + `Wasami.Delay <秒> <コマンド>` の並びにし、節目ごとに `Wasami.Status`（ログに 1 行）と `Shot showui`（絵）を置いて、最後に `quit`。絵は `Saved/Archive/Windows/wasami_deception/Saved/Screenshots/Windows/ScreenShot*.png`、ログは同じ `Saved/Logs/wasami_deception.log`。

## 決定事項

- 2026-09-22: **替えたのは曲・音量（1.0 → 0.6）・ピッチ（0.5 → 1.0）の 3 つだけ**。`FadeIn(2, 0.5)` は旧版と最新版で同じなので触らない。理由と根拠（最新版の `FadeInMusic` @10519）は実装記録 14 へ移した。
- 2026-09-22: **`Pause_Sound_v1` は取り込みからも参照からも外さない** — ポーズ画面（`WasamiPauseWidget`）と EXTRAS の 4 曲目「Pause Theme」（`WasamiExtrasWidget`）が今も使う。タイトルの `MusicSound` だけを替えた。

## 要確認（ユーザー）

- 無し（2026-09-23 に `RunUAT.bat BuildCookRun` の許可が出た。`.claude/settings.json` の `permissions.allow`）。

## 再開時の注意

- 残るのはステップ 4（パッケージ版）だけで、`RunUAT.bat BuildCookRun` の許可が出るまで動かせない。コードとアセットはすべて入っていて、エディタ側にやり残しは無い。
- **エディタを前面にできないときのテストの走らせ方**（この反復で使った。症状索引にもある）: リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` → `execute_console_command(None, 'Automation RunTests <filter>')` → 終わったら `True` に戻す。
- **PIE で今鳴っている音を確かめる**: `execute_console_command(game_world, 'au.Debug.ListAudioComponents')` → `Saved/Logs/wasami_deception.log` の `ActiveSounds` の行を読む（画面に出すなら `au.Debug.Sounds 1` → 見終わったら `0`。`stat sounds` は出なかった）。

## 検証

- check_records: 2026-09-22 OK（20 件。14・15 記録のハッシュを更新）
- C++ ビルド: 2026-09-22 OK（`editor_cycle.py`、57.8 s、`Result: Succeeded`）
- Automation: 2026-09-22 `Wasami.Title+Wasami.Settings` 9 件すべて Success（`Wasami.Title.Screen` が音量 0.6・ピッチ 1.0・`FadeIn(2, 0.5)` を、`Wasami.Settings` が新しい曲のサウンドクラス `DD_SoundClass_Music` を見る）
- PIE（`L_Title`）: 2026-09-22 OK。`au.Debug.ListAudioComponents` の `AudioDevice ... has 1 ActiveSounds` が `/Game/DD/Audio/DD_-_Dark_Deception_-_Theme_v1_3` ただ 1 つ（画面の `au.Debug.Sounds` も `Total Sounds: 1 / Sound Waves: 1` で同じ曲）。旧版の `Pause_Sound_v1` は鳴っていない。PIE は停止済み、dirty なし。
- ポーズ画面と EXTRAS: 2026-09-22 変更なし（`WasamiPauseWidget.cpp:109` と `WasamiExtrasWidget.cpp:136` は `Pause_Sound_v1` のまま。触っていない）
