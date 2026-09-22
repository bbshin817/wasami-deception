---
title: 項目 40: 捕獲の音を本家の音源からワサミの音源に替える
status: 進行中
branch: main
base: d0a01e8
started: 2026-09-22 12:51
updated: 2026-09-22 13:05
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 項目 40: 捕獲の音を本家の音源からワサミの音源に替える

## 依頼

作業一覧 `.claude/roadmap.md` の「大目標 4: レビュー指摘の修正」の項目 40。レビュアーの指摘「敵ワサミ襲撃時について、本家の音源が再生されている。ここはワサミの任意の音源へ差し替える」。

完了の条件（作業一覧より）:

1. 2026-09-22 のユーザーの回答どおりに当てる。捕獲 3 本（ホテル型）の頭の叫び（`Evil_Monkey_Scream` の代わり）= `you`（オマエ・ジャ。5.007 s。WebGL 版が鳴らさないので未取り込み → `dd_voices` に足して取り込む）、顔を間近に写す 4 本目（`LIVING_STATUE_Laughter_05` と `Axe_Hit_03` の代わり）= `Over`（あっ、終わりです。0.727 s。取り込み済み）。
2. 捕獲の 4 本それぞれの音の入る時刻（`AWasamiCapture::SoundTime`）を新しい音の長さに合わせる。
3. 本家の 3 本を捕獲から外す（ほかで使っていないことを確かめてから）。

## 今の作り（調べた結果。2026-09-22）

- `AWasamiCapture`（`Source/wasami_deception/WasamiCapture.cpp`）
  - `NumChoices = 4`、`FaceChoice = 3`。ホテル型（0〜2）は `DeathDelay = 3.5 s`、顔は `FaceDeathDelay = WatcherAnimDelay(0.2) + WatcherHitDelay(0.85) + WatcherBlackDelay(0.1) = 1.15 s`。
  - `NumSounds(Choice)` = 顔 2 / ほか 1。`SoundTime` = ホテル型 0 s、顔 0.2 s（笑い）と 1.05 s（斧）。
  - `GetSound` はソフト参照 `ScreamSound`・`LaughSound`・`HitSound`（`WasamiCapture.cpp:388-390` で `/Game/DD/Audio/…` を代入）。鳴らすのは `PlayCaptureSound` の `UGameplayStatics::PlaySound2D(this, Sound, 1.f, 1.f)`（戻り値を持たないので途中で止められない）。`Start` が `SoundTimers` を張る（t = 0 のものは即座に鳴らしてタイマーを張らない）。
- 取り込み: `dd_enemy.CAPTURE_SOUNDS`（3 本、`CAPTURE_SOUND_VERSION = 1`）→ `import_capture_sounds`。説明は `Content/Python/wasami_tools/toolsets/dd.py:283-284`。
- 本家の 3 本は**捕獲だけが使っている**（`grep` 済み。ほかの参照は実装記録と作業一覧の文だけ）。
- 声（ステップ 2 で更新済み）: `dd_voices.py` の `SUBTITLED`（5 本）+ `SILENT`（7 本）= 12 本、`EWasamiVoice` は `You` が最後（`WasamiVoice::Path(EWasamiVoice::You)` = `/Game/Wasami/Voices/Wasami_You.Wasami_You`、5.007 s、字幕なし）。
- `Over` は死亡画面の**ゲームオーバー**（`EStep::GameOver`、ライフ 0 のときだけ）でも鳴る。ライフが残っているときの死亡画面は `Fine`（`EStep::LifeLost`、画面が出て 0.5 s ごろ）。
- `/Content/DD/` も `/Content/Wasami/` も `.gitignore` の対象（前処理で作り直せる）。アセットの追加・削除は git に出ない。

## 計画

- [x] 1. 調べて計画を立てる … 2026-09-22 完了（この記録）。
- [x] 2. `you` を取り込み、`EWasamiVoice::You` を足す … 2026-09-22 完了。`dd_voices.SILENT` に `you`（12 本目）、`EWasamiVoice::You`（`Over` の次、5.007 s）、テスト 2 つ緑、`/Game/Wasami/Voices/Wasami_You` 取り込み済み（1 ch・44.1 kHz・`DD_SoundClass_Dialogue`・字幕 0）。
- [ ] 3. 捕獲の音を差し替える
  - 変更予定: `Source/wasami_deception/WasamiCapture.h`・`.cpp`、`Source/wasami_deception/Tests/WasamiCaptureTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_enemy.py`、`Content/Python/wasami_tools/toolsets/dd.py`、`.claude/implementation-records/07-enemies.md`・`01-stage-pipeline.md`
  - `ScreamSound`・`LaughSound`・`HitSound` を `WasamiVoice::Path(EWasamiVoice::You)` と `…::Over` の 2 つ（`ChaseVoiceSound`・`CaughtVoiceSound` のような本作の名前）に替え、`NumSounds`（顔も 1 に）・`SoundTime`・`GetSound` を直す。時刻は「決定事項」のとおり。
  - `dd_enemy.CAPTURE_SOUNDS` を外し（`import_capture_sounds` と `import_all` の戻り値の数、`dd.py` の説明も）、実装記録 07（「音」の節・アセットの表・履歴）と 01（`import_wasami_enemy` の行）を直す。
  - `Wasami.Capture.Sound` テスト（`WasamiCaptureTests.cpp:603-612` が波の名前を見ている）を新しい波と時刻に直す。
  - `python Tools/editor_cycle.py` でビルド → テスト → `python .claude/scripts/check_records.py --update`。
- [ ] 4. PIE で 4 本とも確かめて閉じる
  - `python Tools/pie.py start` → チェックポイントで `Wasami.Capture <n>`（0〜3）を 4 回。声が鳴ること・死亡画面の `Fine`/`Over` と重ならない（重なり方が許せる）ことを耳と `GetSoundDelay` で見る。
  - 作業一覧の項目 40 を「完了（2026-09-22）」にし、handover の「現状と次の一歩」を直し、この記録を消して最後のコミットに含める。

## 次にやること

ステップ 3。`WasamiCapture` の `ScreamSound`・`LaughSound`・`HitSound` を `WasamiVoice::Path(EWasamiVoice::You)`（ホテル型 0〜2）と `…::Over`（顔 3）の 2 つに替え、`NumSounds(FaceChoice)` を 2 → 1、`SoundTime` をホテル型 0 s・顔 0.2 s（下の「決めること」で決着済み）にする。`dd_enemy.CAPTURE_SOUNDS` と `import_capture_sounds` を外し（`import_all` の戻り値と `dd.py` の説明も）、`WasamiCaptureTests.cpp:603-612` を新しい波と時刻に直し、実装記録 07・01 を直す。

## 決定事項

- 2026-09-22: **`you` には字幕を付けない**（`dd_voices.SILENT` に入れる） — 捕獲は画面いっぱいの演出で、同じ場面の `Over` も字幕を出していない。WebGL 版は `you` を鳴らさないので前例が無い。要確認に載せた。
- 2026-09-22: **本家の 3 本のアセットは消さない** — `CAPTURE_SOUNDS` から外せば次の取り込みからは作られない。削除は無人運転では行わない。要確認に載せた。
- 2026-09-22: **ホテル型 3 本の `SoundTime` は本家と同じ t = 0**（`PlaySound2D` のまま、止める仕組みは足さない） — `you.wav` を測ったら、台詞そのものは約 0.8 s で終わり、残りは残響の尾（ピーク比 -40 dB を 2.26 s、-60 dB を 3.36 s で下回り、3.5 s の時点で約 -81 dBFS）。`DeathDelay = 3.5 s` の中に聞こえる分がすべて収まるので、はみ出しは実際には聞こえない。
- 2026-09-22: **顔（`FaceChoice`）は `Over` を 1 本だけ、`SoundTime = WatcherAnimDelay`（0.2 s）** — 本家の笑いと同じ掴む瞬間。0.727 s で `FaceDeathDelay = 1.15 s` に収まる。

## 要確認（ユーザー）

- 2026-09-22: **捕獲の `you` に字幕を出すか** — 仮に**出さない**（`dd_voices.SILENT`）。理由: WebGL 版は `you` を鳴らさず前例が無い。捕獲は画面いっぱいの演出で、死亡画面の `Over` も字幕を出していない。場所: `Content/Python/wasami_tools/pipeline/dd_voices.py` の `SILENT`。
- 2026-09-22: **手元に残る本家の捕獲の 3 本（`/Game/DD/Audio/01_Hotel/Evil_Monkey_Scream`・`/Game/DD/Audio/03_Manor/LIVING_STATUE_Laughter_05`・`Axe_Hit_03`）のアセットを消すか** — 仮に**消さない**（取り込みの一覧から外すだけ）。理由: アセットの削除は無人運転では行わない決まり。`/Content/DD/` は git の外なので、消しても前処理で作り直せる。場所: `Content/DD/Audio/01_Hotel/`・`Content/DD/Audio/03_Manor/`。

## 再開時の注意

- C++ を変えたら `python Tools/editor_cycle.py`（エディタを閉じて Development Editor をビルドし開き直す。今回は 44 s + 開き直し 17 s で済んだ）。
- **テストの回し方**（MCP が切れているとき）: リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` → `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests Wasami.Capture')` → `Saved/Logs/wasami_deception.log` の `Test Completed` を読む → 真に戻す。エディタを前面に出さずに通った。
- `WasamiCapture.cpp:349-365` に既存の C4305 警告（double → float）が 5 つある。ステップ 3 でこのあたりを触るので、ついでに `f` を付けて消す。

## 検証

- check_records: ステップ 2 で更新済み
- C++ ビルド: ステップ 2 で成功（2026-09-22）
- テスト: `Wasami.Voice.Table`・`Wasami.Voice.Waves` 緑（2026-09-22）
- エディタでの確認: `import_wasami_voices` = `{'subtitled': 5, 'silent': 7}`、`Wasami_You` 5.007 s
- PIE: 未実行（ステップ 4）
