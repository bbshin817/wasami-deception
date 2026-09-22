---
title: 項目 40: 捕獲の音を本家の音源からワサミの音源に替える
status: 進行中
branch: main
base: d0a01e8
started: 2026-09-22 12:51
updated: 2026-09-22 12:51
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
- 声: `dd_voices.py` の `SUBTITLED`（5 本）+ `SILENT`（6 本）= 11 本を取り込む。`you` は「WebGL 版が鳴らさない 4 本（follow・safe・wait・you）」として**外してある**。原本 `SourceArt/Wasami/Voices/you.wav` と `manifest.json` の項目（id `you`、category `chase`、字幕「オマエ・ジャ。」、5.007098 s）は**ある**。
- `EWasamiVoice`（`WasamiVoice.h`）は 11 個で、`Over` が最後。`WasamiVoice.cpp:33-35` の `Clips` 表（パスと秒数）と `static_assert(... == (uint8)EWasamiVoice::Over + 1)` が最後の値に結びついている。
- `Over` は死亡画面の**ゲームオーバー**（`EStep::GameOver`、ライフ 0 のときだけ）でも鳴る。ライフが残っているときの死亡画面は `Fine`（`EStep::LifeLost`、画面が出て 0.5 s ごろ）。
- `/Content/DD/` も `/Content/Wasami/` も `.gitignore` の対象（前処理で作り直せる）。アセットの追加・削除は git に出ない。

## 計画

- [x] 1. 調べて計画を立てる … 2026-09-22 完了（この記録）。
- [ ] 2. `you` を取り込み、`EWasamiVoice::You` を足す
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_voices.py`、`Source/wasami_deception/WasamiVoice.h`・`.cpp`、`Source/wasami_deception/Tests/`（声のテストがあれば）、`.claude/implementation-records/10-audio.md`、`/Game/Wasami/Voices/Wasami_You`
  - `dd_voices` の `SILENT` か `SUBTITLED` に `you` を足し（下の「決定事項」）、冒頭の「never played」の説明を直す。`EWasamiVoice` に `You` を足し、`Clips` 表（`/Game/Wasami/Voices/Wasami_You`, 5.007）と `static_assert` の最後の値を直す。
  - エディタで `import_wasami_voices` を走らせて `Wasami_You` ができることを確かめ、`python Tools/editor_cycle.py` でビルドしてテストを通す。
- [ ] 3. 捕獲の音を差し替える
  - 変更予定: `Source/wasami_deception/WasamiCapture.h`・`.cpp`、`Source/wasami_deception/Tests/WasamiCaptureTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_enemy.py`、`Content/Python/wasami_tools/toolsets/dd.py`、`.claude/implementation-records/07-enemies.md`・`01-stage-pipeline.md`
  - `ScreamSound`・`LaughSound`・`HitSound` を `WasamiVoice::Path(EWasamiVoice::You)` と `…::Over` の 2 つ（`ChaseVoiceSound`・`CaughtVoiceSound` のような本作の名前）に替え、`NumSounds`（顔も 1 に）・`SoundTime`・`GetSound` を直す。時刻は下の「決めること」。
  - `dd_enemy.CAPTURE_SOUNDS` を外し（`import_capture_sounds` と `import_all` の戻り値の数、`dd.py` の説明も）、実装記録 07（「音」の節・アセットの表・履歴）と 01（`import_wasami_enemy` の行）を直す。
  - `Wasami.Capture.Sound` テスト（`WasamiCaptureTests.cpp:603-612` が波の名前を見ている）を新しい波と時刻に直す。
  - `python Tools/editor_cycle.py` でビルド → テスト → `python .claude/scripts/check_records.py --update`。
- [ ] 4. PIE で 4 本とも確かめて閉じる
  - `python Tools/pie.py start` → チェックポイントで `Wasami.Capture <n>`（0〜3）を 4 回。声が鳴ること・死亡画面の `Fine`/`Over` と重ならない（重なり方が許せる）ことを耳と `GetSoundDelay` で見る。
  - 作業一覧の項目 40 を「完了（2026-09-22）」にし、handover の「現状と次の一歩」を直し、この記録を消して最後のコミットに含める。

## 次にやること

ステップ 2。`dd_voices.py` に `you` を足して（`SILENT` に置く。下の決定事項）冒頭の説明を直し、`WasamiVoice.h`/`.cpp` に `You` を足す（`Clips` は `{TEXT("/Game/Wasami/Voices/Wasami_You"), 5.007}`、`static_assert` の最後を `You` に）。そのあとエディタで `import_wasami_voices`、`python Tools/editor_cycle.py`、テスト。

## 決定事項

- 2026-09-22: **`you` には字幕を付けない**（`dd_voices.SILENT` に入れる） — 捕獲は台詞の場面ではなく、字幕は死亡画面の `Over` と同じく出していない。WebGL 版は `you` を鳴らさないので前例が無く、同じ chase の `found`（字幕あり）は「敵が見つけた合図」で画面に出す意味があるのに対し、捕獲は画面いっぱいの演出なので字幕が邪魔になる。要確認に載せる。
- 2026-09-22: **本家の 3 本のアセットは消さない** — `/Content/DD/` は前処理で作り直せて git の外なので、`CAPTURE_SOUNDS` から外せば次の取り込みからは作られない。手元に残る古いアセットの削除は「変更を捨てる操作」なので無人運転では行わず、要確認に載せる。

## 決めること（ステップ 3 で）

- **ホテル型 3 本の `SoundTime`**: 本家は t = 0。`you` は 5.007 s で、場面は `DeathDelay = 3.5 s` で終わる（`PlaySound2D` は止められないので死亡画面へ 1.5 s はみ出し、ライフが残っていれば 4.0 s ごろの `Fine` と重なる）。候補: (a) t = 0 のまま（頭から鳴らす。はみ出しは許す）、(b) `DeathDelay` に収まるよう `SpawnSound2D` に替えて場面の終わりで止める、(c) 波の実音の長さ（無音の後ろを除いた長さ）を測って、収まるなら t = 0 のまま・収まらなければ (b)。まず `you.wav` の実音の終わりを測ってから決める。
- **顔（`FaceChoice`）の `SoundTime`**: `Over` は 0.727 s で `FaceDeathDelay = 1.15 s` に収まる。本家の笑いと同じ `WatcherAnimDelay = 0.2 s`（掴む瞬間）が第一候補。`NumSounds(FaceChoice)` は 2 → 1。

## 要確認（ユーザー）

- 2026-09-22: **捕獲の `you` に字幕を出すか** — 仮に**出さない**（`dd_voices.SILENT`）。理由: WebGL 版は `you` を鳴らさず前例が無い。捕獲は画面いっぱいの演出で、死亡画面の `Over` も字幕を出していない。場所: `Content/Python/wasami_tools/pipeline/dd_voices.py` の `SILENT`。
- 2026-09-22: **手元に残る本家の捕獲の 3 本（`/Game/DD/Audio/01_Hotel/Evil_Monkey_Scream`・`/Game/DD/Audio/03_Manor/LIVING_STATUE_Laughter_05`・`Axe_Hit_03`）のアセットを消すか** — 仮に**消さない**（取り込みの一覧から外すだけ）。理由: アセットの削除は無人運転では行わない決まり。`/Content/DD/` は git の外なので、消しても前処理で作り直せる。場所: `Content/DD/Audio/01_Hotel/`・`Content/DD/Audio/03_Manor/`。

## 再開時の注意

- 長時間の処理はまだ走らせていない。C++ を変えたら `python Tools/editor_cycle.py`（エディタを閉じて Development Editor をビルドし開き直す。10〜20 分）。
- `you.wav` の実音の長さを測るのは `SourceArt/Wasami/Voices/you.wav`（44 バイトの WAV ヘッダ + PCM。python の `wave` と `audioop`、または `Tools/video_probe.py` は動画用なので使わない）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
