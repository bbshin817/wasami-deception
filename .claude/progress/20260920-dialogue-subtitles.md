---
title: 台詞と字幕（作業一覧の項目 20）
status: 進行中
branch: main
base: ba43daf
started: 2026-09-20 12:17
updated: 2026-09-20 13:20
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

### 本家の話し役（`BierceTalk_Blueprint`）

`pak_reference_2/_bytecode/DDeception/Content/Blueprints/00_Ballroom/BierceTalk_Blueprint.txt`。アクタ 1 つに `AudioComponent` 1 つだけ。

- `Talk(What To Say: SoundBase, Attenuate?: bool)`: `AudioComponent.bAllowSpatialization = Attenuate?` → `Halt` が真なら何もしない → 偽なら「`IsPlaying()` の間は 0.5 s の `Delay` で待つ」を繰り返し、空いたら `SetSound(What To Say)` → `Play(0.0)`。**前の台詞を切らず、終わるまで待ってから次を鳴らす**。
- `Stop Talking()`: `AudioComponent.Stop()`。
- レベル BP が呼ぶ口は 2 つ。(a) `BP_DD_Functions` の `Bierce Talk(What To Say, Attenuate?, WorldContext)` = `GetAllActorsOfClass(BierceTalk_Blueprint_C)` の 0 番目に `Talk` を投げる静的関数、(b) レベルが持つ `BierceTalk_Blueprint_2` の参照に直接 `Talk`。**病院の呼び出しは全部 `Attenuate? = False`**（画面の外でも同じ音量の 2D）。

### 本家の病院の台詞（本作が作る Zone 1・Zone 2 のぶんだけ）

波は `pak_reference_2/DDeception/Content/Audio/Dialogue/Bierce/Ch06/TT/*.ogg`（`_assets` に json、`_audio.json` に長さ。音量 2.0・`DD_SoundClass_Dialogue`）。

| ゾーン | きっかけ（レベル BP のイベント） | 鳴らすもの |
| --- | --- | --- |
| 1 | `04_DoorBreak` | `Bierce_TormentTherapy_Event_10` |
| 1 | `04_Intercom` | `PlaySound2D(Nurse_Hospital_Zone01_Event_37_Intercom, 音量 0.6, ピッチ 1)` → `Delay 13 s` → `Event_09` |
| 1 | `Setup Nurse Bierce Quips` → `Bierce Nurse Quip` | `Bierce_TormentTherapy_Gameplay`（SoundCue。`SoundNodeRandom` が重み 1 で `Gameplay_05 / 02 / 04 / 01 / 03` の 5 本から 1 本。`DialogueAttenuation`）|
| 2 | `Cell Cutscene Finished` | `Event_17` |
| 2 | `Bierce Lift Quip`（`Setup Bierce Lift Quip` が仕掛ける） | `Gameplay_07` |
| 2 | `Maze Trigger Start` | `Gameplay_08` |
| 2 | `Maze All Shards` | `Event_20` |
| 2 | 欠片の回収（`Collected Ring Piece` の流れ。HEAD TOWARDS THE GARAGE の後、`Delay 1 s`） | `Event_21` |
| 2 | `Postmaze_Trigger_Garage` | `Event_22` |
| 2 | `Miniboss_BierceTalk` | `Event_19` |
| 2 | `Miniboss_BehindMatron` | Zone 2 に置いた `AmbientSound`（`Nurse_Hospital_Zone01_Event_48_Intercom`。`bAutoActivate` 偽。項目 19 で置いてある）を鳴らす |

- Zone 1 の `Bierce Nurse Quip` は `Setup Nurse Bierce Quips` が全部の `BP_06_ReaperNurse` の `CloseBy` デリゲートに繋ぐもの（敵が近づいたら 1 言）。本作の敵は `AWasamiEnemy`（本家のナースを写したもの）なので、`CloseBy` 相当があるかをステップ 3 で確かめる（07 記録の `NurseNear` が近い）。
- 本作が作らない入口 `06_Hospital` の `Event_01〜08`、ボス戦の `Event_11〜16`・`18B`・`23`・`24` は対象外。

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
- [ ] 2. **話し役 `AWasamiBierceTalk`**（本家の `BierceTalk_Blueprint` を写す。`Talk`・`StopTalking`・`bHalt`・0.5 s 待ち）と、レベルの流れから呼ぶ口。両ゾーンに 1 つずつ置く
  - 変更予定: `Source/wasami_deception/WasamiBierceTalk.h/.cpp`（新）、`Tests/`、`Content/Python/wasami_tools/pipeline/dd_level.py`（置く）
- [ ] 3. **Zone 1 の配線**（`04_DoorBreak` → `Event_10`、`04_Intercom` → ナースのインターコム 0.6 → 13 s → `Event_09`、敵が近づいたときの `Bierce Nurse Quip`）と、字幕が画面に出ることの確かめ
  - 変更予定: `Source/wasami_deception/WasamiZone1Flow.cpp/.h`、`WasamiEnemy.cpp`（`CloseBy` 相当）
- [ ] 4. **Zone 2 の配線**（`Event_17`・`Gameplay_07`・`Gameplay_08`・`Event_20`・`Event_21`・`Event_22`・`Event_19`、Matron の裏のインターコムの `AmbientSound` を鳴らす）
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.cpp/.h`
- [ ] 5. **ワサミの声の取り込み**（mp3 15 本 → wav（ffmpeg）→ `SourceArt/Wasami/Voices/`（Git LFS）→ `/Game/Wasami/Voices/*`。`manifest.json` の字幕を `Subtitles` に、`DD_SoundClass_Dialogue` を当てる）
  - 変更予定: `Tools/`（変換）、`Content/Python/wasami_tools/pipeline/dd_voices.py`（新）、`.gitattributes`
- [ ] 6. **ワサミの声を場面に付ける**（敵の `found`・巡回の 4 本を頭の位置に、流れの `greeting`・`well`・`fast`・`best`、死亡画面の `fine`・`over`）
  - 変更予定: `Source/wasami_deception/WasamiVoice.h/.cpp`（新。`Say`）、`WasamiEnemy.cpp`、`WasamiZoneFlow.cpp`、`WasamiDeathScreenWidget.cpp`、`WasamiPowerComponent.cpp`、`WasamiSecretRoomZone.cpp`
- [ ] 7. **確かめと締め**（PIE で Zone 1・Zone 2 の台詞と字幕、ワサミの声を通しで確かめ、実装記録・note を直して項目 20 を閉じる）

## 次にやること

ステップ 2。本家の `BierceTalk_Blueprint`（上の「本家の話し役」）を `AWasamiBierceTalk` に写す（`Talk(SoundBase, bAttenuate)`・`StopTalking()`・`bHalt`、鳴り終わるまで 0.5 s ごとに待って次を鳴らす）。`UAudioComponent` 1 つだけのアクタで、両ゾーンに 1 体ずつ置く（本家の置き場所は `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`・`_02.full.json` の `BierceTalk_Blueprint_2`。`dd_level._flow` に足してタグ `src:BierceTalk_Blueprint_2` で引けるように）。テストも。

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
- 長時間処理はまだ無い。ステップ 2 は C++ なので `python Tools/editor_cycle.py` で閉じてビルドして開き直す。
- 原本の場所: 本家の台詞 `pak_reference_2/DDeception/Content/Audio/Dialogue/Bierce/Ch06/TT/*.ogg`、ワサミの声 `C:\Users\User\Downloads\wasami-deseption\public\voices\*.mp3`。

## 検証

- check_records: OK（20 件。`--update` で 01・10 記録のハッシュを更新）
- C++ ビルド: この項目ではまだ C++ を触っていない（ステップ 2 から）
- エディタでの確認: `import_dd_dialogue` で `lines` 9・`quips` 5・`cues` 1・`intercom` 1。15 本すべて音量・ピッチ・チャンネル数・長さ・`SoundClassObject`・字幕 1 つが本家の書き出しと一致し、Cue は `SoundNodeRandom` の重み 1 × 5・波の順（05/02/04/01/03）・`DialogueAttenuation` が一致。`Content/DD/...` に保存済み。
- PIE: 鳴らす側がまだ無いので未実施（字幕が画面に出ることの確かめはステップ 3）。
