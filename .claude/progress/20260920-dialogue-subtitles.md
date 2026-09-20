---
title: 台詞と字幕（作業一覧の項目 20）
status: 進行中
branch: main
base: ba43daf
started: 2026-09-20 12:17
updated: 2026-09-20 13:15
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 台詞と字幕（作業一覧の項目 20）

## 依頼

`.claude/roadmap.md` の項目 20（大目標 2「ゲームとして一通り」の残り 2 項目のうちの 1 つ）。

- 目標: Bierce の台詞（`BierceTalk_Blueprint`）を本家どおり鳴らして字幕を出し、WebGL 版のワサミの声（16 本 + 字幕の `manifest.json`。巡回・発見・タイトルなど）を敵とタイトルに付ける。
- 完了の条件: 各レベル BP の `Talk` の呼び出しがそろい、字幕は本家の `_localization.json` / `_strings.json` の文言。ワサミの声は WebGL 版 06・15 記録の鳴らし方（バス・音量・字幕の秒数・場面）どおり。原本は `<WEBGL>/voices/`（wav 55）と `<WEBGL>/public/voices/`（mp3 16 + manifest）。
- 依存: 7（敵の AI。完了）、17（タイトル。完了）。規模 2。
- 決め方は大目標 2 の「見た目の詰めをしない」（本家の実機は起動しない。原作のアセットをそのまま使う）。

## 調べたこと（2026-09-20 の計画の反復）

### 本家の病院の台詞（残りは Zone 2 のぶん）

話し役は `AWasamiBierceTalk`（10 記録）、喋らせる口は流れの `BierceTalk(音, bAttenuate = false)`（11 記録）。波は `/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/`。**病院の呼びはすべて `Attenuate? = False`**。Zone 1 のぶんはステップ 3 で入れた（11 記録）。

| きっかけ（Zone 2 のレベル BP のイベント） | 鳴らすもの |
| --- | --- |
| `Cell Cutscene Finished`（本家は 1 s 後の `Miniboss_BierceTalk` の結びと同じ所。11 記録の `OnCellCutsceneFinished`） | `Event_17` |
| `Bierce Lift Quip`（`Setup Bierce Lift Quip` が全部の `AWasamiLift` の `OnPlayerOverlap` に仕掛ける。12 記録） | `Gameplay_07` |
| `Maze Trigger Start`（`OnMazeTriggerStart` の 1 s 後） | `Gameplay_08` |
| `Maze All Shards`（`OnMazeAllShards` の 2 s 後） | `Event_20` |
| 欠片の回収（`OnRingPieceCollect` の `GarageBindDelay` 1 s の所） | `Event_21` |
| `Postmaze_Trigger_Garage`（`OnPostmazeTriggerGarage` の 1 s 後） | `Event_22` |
| `Miniboss_BierceTalk`（`OnMinibossBierceTalk`） | `Event_19` |
| `Miniboss_BehindMatron`（`OnMinibossBehindMatron`） | Zone 2 に置いた `AmbientSound`（`Nurse_Hospital_Zone01_Event_48_Intercom`。`bAutoActivate` 偽。項目 19 で置いてある）を鳴らす |

**上の秒数と結び先は本家の該当イベントを `Tools/dd/bp_flow.py` で読み直してから写す**（`pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_02.txt`）。11 記録のイベントの説明にある「項目 20」の印がそのまま口。

### 字幕

UE の `USoundWave.Subtitles` に本家の文字列表 `Strings` の文言を名前の対応で入れる（ステップ 1 で実装済み。詳しくは 10 記録の「台詞の取り込み」）。`_datatables.json` を読むときは端末が CP932 なので `sys.stdout.buffer.write(....encode('utf-8'))` で出す。

### WebGL 版のワサミの声

- 原本: `C:\Users\User\Downloads\wasami-deseption\public\voices\`（mp3 15 本 + `manifest.json`）。`voices\` の wav 55 本は切り出す前の素材で mp3 との対応が取れないので使わない。
- **UE は mp3 を取り込めない**ので ffmpeg（`ffmpeg -version` で在ることを確認済み）で wav にしてから取り込む。
- `manifest.json` の 15 本（id | category | 長さ s | 字幕）:
  `greeting|intro|3.878|こんにちワサミ！` / `calling|patrol|1.027|おーい。` / `others|patrol|0.690|ほかのみんなは？` / `follow|patrol|1.034|私と一緒に行きましょう。` / `remember|patrol|0.937|覚えてます。` / `think|patrol|0.862|俺のことも想え。` / `found|chase|0.622|ここか！` / `you|chase|5.007|オマエ・ジャ。` / `wait|chase|1.447|ちょっと待ってね。` / `fast|boost|0.637|はっや！` / `over|caught|0.727|あっ、終わりです。` / `fine|respawn|1.784|まぁまぁ、そういう時だってあるか。` / `well|pickup|0.705|非常にいい！` / `safe|ring|0.682|無事でしたか！` / `best|win|0.517|さいこう！`
  （`normalization` は `integratedLUFS -23`・`truePeakDB -6`）
- WebGL 版 06 記録の鳴らし方: バスは `voice`（= `DD_SoundClass_Dialogue`。音量 1.0）。`Game.say(id)` は `play(id, {bus:'voice'})` + 字幕（`hud.subtitle(subtitle, 'ワサミ', max(2.2, 長さ + 1.2))`）。使う場面は `greeting`（開始）・`well`（1 個目の回収）・`fast`（ブースト成功）・`best`（隠し扉が開いた 0.5 s 後）。死亡画面は字幕なしで `fine`（ライフが残る）・`over`（ゲームオーバー）。敵（15 記録）は頭の位置に定位して `found`（音量 1.0）と巡回の `calling`/`others`/`think`/`remember`（0.9）。`follow`・`safe`・`wait`・`you` は使っていない。

## 計画

- [x] 1. **台詞の取り込み**（完了）: `dd_dialogue.py`（Bierce の波 15 本と一言の Cue、字幕は本家の文字列表 `Strings` の文言を名前の対応で）、`dd_assets.sound(subtitles=)`、ツール `WasamiDDTools.import_dd_dialogue`。取り込んだ中身と根拠は 10 記録の「台詞の取り込み」。
- [x] 2. **話し役 `AWasamiBierceTalk`**（完了）: 本家の `BierceTalk_Blueprint` を写し（`Talk`・`StopTalking`・`bHalt`・0.5 s の待ち・`Find`）、`dd_level._flow` が両ゾーンに 1 つずつ置くようにした。中身と根拠は 10 記録の「話し役 `AWasamiBierceTalk`」。
- [x] 3. **Zone 1 の配線**（完了）: 流れに `BierceTalk` の口を足し、`04_DoorBreak`（1 s 後の `Event_10`）・`04_Intercom`（放送 0.6 → 13 s → `Event_09`）・`Setup Nurse Bierce Quips`（発見の 5 回に 1 回の一言）を埋めた。中身と根拠は 11 記録。
- [ ] 4. **Zone 2 の配線**（`Event_17`・`Gameplay_07`・`Gameplay_08`・`Event_20`・`Event_21`・`Event_22`・`Event_19`、Matron の裏のインターコムの `AmbientSound` を鳴らす）
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.cpp/.h`
- [ ] 5. **ワサミの声の取り込み**（mp3 15 本 → wav（ffmpeg）→ `SourceArt/Wasami/Voices/`（Git LFS）→ `/Game/Wasami/Voices/*`。`manifest.json` の字幕を `Subtitles` に、`DD_SoundClass_Dialogue` を当てる）
  - 変更予定: `Tools/`（変換）、`Content/Python/wasami_tools/pipeline/dd_voices.py`（新）、`.gitattributes`
- [ ] 6. **ワサミの声を場面に付ける**（敵の `found`・巡回の 4 本を頭の位置に、流れの `greeting`・`well`・`fast`・`best`、死亡画面の `fine`・`over`）
  - 変更予定: `Source/wasami_deception/WasamiVoice.h/.cpp`（新。`Say`）、`WasamiEnemy.cpp`、`WasamiZoneFlow.cpp`、`WasamiDeathScreenWidget.cpp`、`WasamiPowerComponent.cpp`、`WasamiSecretRoomZone.cpp`
- [ ] 7. **確かめと締め**（PIE で Zone 1・Zone 2 の台詞と字幕、ワサミの声を通しで確かめ、実装記録・note を直して項目 20 を閉じる）

## 次にやること

ステップ 4（Zone 2 の配線）。上の表のとおりに `AWasamiZone2Flow` から喋らせる。

- 本家の `06_Hospital_Zone_02.txt` の各イベントを `python Tools/dd/bp_flow.py <ファイル> <イベント名>` で読み、`Delay` の秒数と順序を写す（Zone 1 では `04_DoorBreak` に記録に無かった `Delay 1` があった）。
- `Setup Bierce Lift Quip` は Zone 1 の `SetupNurseBierceQuips` と同じ形（全部の `AWasamiLift` の `OnPlayerOverlap` に結ぶ。12 記録）。重み付きの抽選があるかを本家で確かめる。
- `Miniboss_BehindMatron` だけは話し役でなく、置いてある `AAmbientSound`（`src:` のタグ。`Source(...)` → `AAmbientSound::GetAudioComponent()->Play()`）。
- テスト `Wasami.ZoneFlow.Zone2` に `SpawnTalker(World)` を足して、各区間で喋る波の名前を確かめる（**話し役はテストの `AWasamiTestBierceTalk` を使う**。下の「再開時の注意」）。

## 決定事項

- 2026-09-20: **字幕は UE の仕組み（`USoundWave.Subtitles` + `SetSubtitlesEnabled`）に載せ、作り直さない** — 本家も同じ仕組みで、オプションの SUBTITLES はすでに `SetSubtitlesEnabled` を呼んでいる（`.claude/guides/original-fidelity.md` の「UE に同じ仕組みがあれば値を写すだけ」）。
- 2026-09-20: **本家の病院の Bierce の波は `Subtitles` が空なので、`Strings` のテーブルから名前の対応で入れる** — 項目 20 の完了の条件が「字幕は `_strings.json` の文言」で、第 4 章の下水の Bierce が同じ名前の対応で入れている。下の「要確認」にも書いた。
- 2026-09-20: **本家の入口 `06_Hospital` とボス戦の台詞は作らない** — 本作はそのレベルを作らない（CLAUDE.md）。
- 2026-09-20: **WebGL 版の `voices/` の wav 55 本は使わず、`public/voices/` の mp3 15 本だけを使う** — mp3 が WebGL 版で実際に鳴っていたもので、`manifest.json` に字幕と長さがある。wav は切り出す前の素材で対応が取れない。

- 2026-09-20: **`dd_assets.sound` に `AttenuationSettings` を書く口は足さない** — 取り込む 15 本のどれも書き出しに持たず（減衰を持つのは一言の Cue だけで、そちらは `dd_assets.sound_cue` が既に `DialogueAttenuation` を作って入れる）、使い道の無い口になるため。ワサミの声（ステップ 5）で波に減衰が要るなら、そのときに足す。

## 要確認（ユーザー）

- 2026-09-20: 本家の病院の Bierce の台詞の波に字幕が入っていない（`Strings` に文言 127 件はあるが、波の `Subtitles` を使っているのは本作が作らない入口のナースの 40 件だけ。第 4 章の下水の Bierce には入っている） — 仮に **名前の対応（`Bierce_TormentTherapy_Event_NN` ↔ `06_Cutscene_Zone_01_Bierce_NN`）で字幕を入れる**。理由: 項目 20 の完了の条件が「字幕は `_strings.json` の文言」で、オプションに SUBTITLES がある以上、出ないほうが不自然。本家の実機で確かめるのは大目標 2 の決め方（見た目の詰めをしない）で控えている。場所: ステップ 1 の `dd_dialogue.py`。

## 再開時の注意

- 台詞の波と Cue は `/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/` と `/Game/DD/Audio/06_Hospital/` に取り込み済み（保存済み）。入れ直すときは `WasamiDDTools.import_dd_dialogue`。
- 話し役は両ゾーンのレベルに置いて保存済み（道も焼き直した）。置き直すときは `place_flow("Zone1")`・`place_flow("Zone2")` → **レベルごとに `build_navigation` を単独の呼びで 2 回**（1 回目は開くだけ、十数秒おいて 2 回目で焼いて保存）。
- 長時間処理: `python Tools/editor_cycle.py`（C++ のビルド。1 分ほど）と `place_flow` → `build_navigation`（3 分ほど）。
- **テストで話し役を置くときは `AWasamiTestBierceTalk`（`Tests/WasamiTestBierceTalk.h`）を使う** — テストのワールドのティックは音声装置を回さないので、素の `AWasamiBierceTalk` だと `Play` した部品がいつまでも「鳴っている」ままになり、2 本目以降の台詞が永久に待たされる（ステップ 3 で 1 度踏んだ）。`Tests/WasamiZoneFlowTests.cpp` の `SpawnTalker`・`Spoken` がその形。
- 自動テストは背面のエディタだと進まないので、走らせる前に `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`、終わったら `True` に戻す（症状索引）。
- 原本の場所: 本家の台詞 `pak_reference_2/DDeception/Content/Audio/Dialogue/Bierce/Ch06/TT/*.ogg`、ワサミの声 `C:\Users\User\Downloads\wasami-deseption\public\voices\*.mp3`。

## 検証

- check_records: OK（20 件）
- C++ ビルド: ok（`Tools/editor_cycle.py`）
- 自動テスト: `Automation RunTests Wasami` で 154 件すべて成功（`Wasami.ZoneFlow.Zone1` の台詞 3 つ・`Wasami.Dialogue.Talk`・`Wasami.Dialogue.TalkStep` を含む）
- PIE（Zone 1。`Wasami.Flow <イベント名>` で流れのイベントを呼び、`Tools/desktop.py shot` で画面を撮って確かめた）:
  - `On04DoorBreak` → 1 s 後に話し役の部品が `Bierce_TormentTherapy_Event_10` を鳴らし、**字幕が画面下に出た**（"…visible to the eye, but they can't hide the sounds they make."）。`bAllowSpatialization` 偽。
  - `On04Intercom` → 館内放送が鳴って字幕が出（"…an unruly patient on the loose… Lethal mercy is authorized!"）、13 s 後に `Event_09`（字幕「They could be anywhere! Stay alert!」）に変わった。
  - `OnBierceNurseQuip` を 20 回 → 一言の Cue `Bierce_TormentTherapy_Gameplay` が鳴った。
  - `UGameplayStatics::AreSubtitlesEnabled()` は真（設定の既定）。PIE は止めた。
