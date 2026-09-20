---
title: 台詞と字幕（作業一覧の項目 20）
status: 進行中
branch: main
base: ba43daf
started: 2026-09-20 12:17
updated: 2026-09-20 13:40
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

### 本家の病院の台詞（Zone 1・Zone 2 とも配線済み）

話し役は `AWasamiBierceTalk`（10 記録）、喋らせる口は流れの `BierceTalk(音, bAttenuate = false)`（11 記録）。波は `/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/`。本家のバイトコードから写した秒数・順序・`Attenuate?` は 11 記録に書いた（**迷路の始まりの `Gameplay_08` だけ `Attenuate? = True`**）。

### 字幕

UE の `USoundWave.Subtitles` に本家の文字列表 `Strings` の文言を名前の対応で入れる（ステップ 1 で実装済み。詳しくは 10 記録の「台詞の取り込み」）。`_datatables.json` を読むときは端末が CP932 なので `sys.stdout.buffer.write(....encode('utf-8'))` で出す。

### WebGL 版のワサミの声（鳴らす場面。ステップ 6 で使う）

波は `/Game/Wasami/Voices/Wasami_<Id>`（ステップ 5 で取り込み済み。長さと字幕は 10 記録）。WebGL 版 06・15 記録の鳴らし方:

- 流れ（字幕あり。`Game.say`）: `greeting`（開始）・`well`（1 個目の回収）・`fast`（ブースト成功）・`best`（隠し扉が開いた 0.5 s 後。脱出では鳴らさない）。バスは `voice`（= `DD_SoundClass_Dialogue`。音量 1.0）。
- 死亡画面（字幕なし）: `fine`（ライフが残る）・`over`（ゲームオーバー）。
- 敵（15 記録。頭の位置に定位）: 発見の `found`（音量 1.0。どの敵からでも 12 s に 1 回まで。字幕は `onSpotted` から出す）と、巡回の `calling`・`others`・`think`・`remember`（0.9。字幕なし）。
- **字幕の出ている長さ**は WebGL 版が `max(2.2, 長さ + 1.2)`（`found` は 2.4 s 固定）。UE の `Subtitles` は音が終わると消えるので、短い声（0.5〜0.7 s）はそのままだと読む間がない。手立ては `FSubtitleManager::QueueSubtitles`（`SoundDuration` を渡せる。`Runtime/Engine/Public/SubtitleManager.h` の ENGINE_API）か、`UAudioComponent::bSuppressSubtitles` で波の字幕を止めて自前で出す。

## 計画

- [x] 1. **台詞の取り込み**（完了）: `dd_dialogue.py`（Bierce の波 15 本と一言の Cue、字幕は本家の文字列表 `Strings` の文言を名前の対応で）、`dd_assets.sound(subtitles=)`、ツール `WasamiDDTools.import_dd_dialogue`。取り込んだ中身と根拠は 10 記録の「台詞の取り込み」。
- [x] 2. **話し役 `AWasamiBierceTalk`**（完了）: 本家の `BierceTalk_Blueprint` を写し（`Talk`・`StopTalking`・`bHalt`・0.5 s の待ち・`Find`）、`dd_level._flow` が両ゾーンに 1 つずつ置くようにした。中身と根拠は 10 記録の「話し役 `AWasamiBierceTalk`」。
- [x] 3. **Zone 1 の配線**（完了）: 流れに `BierceTalk` の口を足し、`04_DoorBreak`（1 s 後の `Event_10`）・`04_Intercom`（放送 0.6 → 13 s → `Event_09`）・`Setup Nurse Bierce Quips`（発見の 5 回に 1 回の一言）を埋めた。中身と根拠は 11 記録。
- [x] 4. **Zone 2 の配線**（完了）: 流れの 8 か所に本家の秒数どおりの口を足した（`Event_17`・`Event_19` の 2 か所・`Gameplay_08`〈唯一の減衰あり〉・リフトの `Gameplay_07`〈1 回だけ〉・`Event_20`・`Event_21`・`Event_22`）と、Matron の裏の放送（置いてある `AAmbientSound` を鳴らす）。中身と根拠は 11 記録。
- [x] 5. **ワサミの声の取り込み**（完了）: `Tools/dd/prepare_voices.py`（mp3 → wav）、`SourceArt/Wasami/Voices/`（wav 15 + `manifest.json`。Git LFS）、`dd_voices.py`・`dd_assets.sound_file`、ツール `WasamiDDTools.import_wasami_voices`。WebGL 版が鳴らす 11 本を `/Game/Wasami/Voices/Wasami_<Id>` に（字幕 5 本・字幕なし 6 本・`DD_SoundClass_Dialogue`）。中身と根拠は 10 記録の「ワサミの声の取り込み」。
- [ ] 6. **ワサミの声を場面に付ける**（敵の `found`・巡回の 4 本を頭の位置に、流れの `greeting`・`well`・`fast`・`best`、死亡画面の `fine`・`over`）
  - 変更予定: `Source/wasami_deception/WasamiVoice.h/.cpp`（新。`Say`）、`WasamiEnemy.cpp`、`WasamiZoneFlow.cpp`、`WasamiDeathScreenWidget.cpp`、`WasamiPowerComponent.cpp`、`WasamiSecretRoomZone.cpp`
- [ ] 7. **確かめと締め**（PIE で Zone 1・Zone 2 の台詞と字幕、ワサミの声を通しで確かめ、実装記録・note を直して項目 20 を閉じる）

## 次にやること

ステップ 6（ワサミの声を場面に付ける）。上の「WebGL 版のワサミの声」の表どおりに鳴らす側を作る。

- 鳴らす口は `Source/wasami_deception/WasamiVoice.h/.cpp`（新。`Say`）にまとめる予定。波はソフト参照（`WasamiAssets.h` の作法）で入れる。
- 付ける先: `WasamiEnemy.cpp`（`found`・巡回の 4 本。頭の位置に定位＝減衰が要る。本家の台詞の減衰は `/Game/DD/Audio/Misc/DialogueAttenuation`〈`bAttenuate` 偽・`OmniRadius` 350・`FalloffDistance` 5000〉なので、そのまま使えるか要検討）、`WasamiZoneFlow.cpp`（`greeting`）、`WasamiShard`/`WasamiZoneFlow`（`well` = 1 個目の回収）、`WasamiPowerComponent.cpp`（`fast`）、`WasamiSecretRoomZone.cpp`（`best`）、`WasamiDeathScreenWidget.cpp`（`fine`・`over`。09 記録の「声は長さだけ待つ」の口が既にある）。
- 字幕の長さをどう合わせるかはこのステップで決める（上の「字幕の出ている長さ」）。

## 決定事項

- 2026-09-20: **字幕は UE の仕組み（`USoundWave.Subtitles` + `SetSubtitlesEnabled`）に載せ、作り直さない** — 本家も同じ仕組みで、オプションの SUBTITLES はすでに `SetSubtitlesEnabled` を呼んでいる（`.claude/guides/original-fidelity.md` の「UE に同じ仕組みがあれば値を写すだけ」）。
- 2026-09-20: **ワサミの声は WebGL 版が実際に鳴らす 11 本だけ取り込む** — `follow`・`safe`・`wait`・`you` は WebGL 版のどこからも鳴らない（06 記録）。本家の台詞で本作が作らないレベルの分を外すのと同じ。原本の wav 15 本は一式として `SourceArt` に置いてある。
- 2026-09-20: **字幕を出さない 6 本（巡回 4・死亡画面 2）は `Subtitles` を空のまま入れる** — WebGL 版がその 6 本を字幕なしで鳴らすので、波に入れなければ UE の既定の振る舞いがそのまま正しくなる（鳴らす側で止める仕掛けが要らない）。

## 要確認（ユーザー）

- 2026-09-20: 本家の病院の Bierce の台詞の波に字幕が入っていない（`Strings` に文言 127 件はあるが、波の `Subtitles` を使っているのは本作が作らない入口のナースの 40 件だけ。第 4 章の下水の Bierce には入っている） — 仮に **名前の対応（`Bierce_TormentTherapy_Event_NN` ↔ `06_Cutscene_Zone_01_Bierce_NN`）で字幕を入れる**。理由: 項目 20 の完了の条件が「字幕は `_strings.json` の文言」で、オプションに SUBTITLES がある以上、出ないほうが不自然。本家の実機で確かめるのは大目標 2 の決め方（見た目の詰めをしない）で控えている。場所: ステップ 1 の `dd_dialogue.py`。

## 再開時の注意

- 台詞の波と Cue（`/Game/DD/Audio/…`）と話し役（両ゾーンに配置）は取り込み・保存済み。入れ直すときは `WasamiDDTools.import_dd_dialogue`。
- ワサミの声は `/Game/Wasami/Voices/Wasami_<Id>` に取り込み・保存済み（11 本）。入れ直すときは `WasamiDDTools.import_wasami_voices`（原本を作り直すなら先に `python Tools/dd/prepare_voices.py`。ffmpeg が要る）。`/Content/Wasami/` は git の外なので、エディタを作り直した人は取り込みから。
- 長時間処理: `python Tools/editor_cycle.py`（C++ のビルド。1 分ほど）。
- **テストで話し役を置くときは `AWasamiTestBierceTalk`（`Tests/WasamiTestBierceTalk.h`）を使う** — テストのワールドのティックは音声装置を回さないので、素の `AWasamiBierceTalk` だと `Play` した部品がいつまでも「鳴っている」ままになり、2 本目以降の台詞が永久に待たされる。`Tests/WasamiZoneFlowTests.cpp` の `SpawnTalker`・`Spoken` がその形（続けて確かめるときは `SetSound(nullptr)` で前の台詞を消す）。
- 自動テストは背面のエディタだと進まないので、走らせる前に `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`、終わったら `True` に戻す（症状索引）。

## 検証

- check_records: OK（20 件）
- 取り込み（ステップ 5）: `import_wasami_voices` を 2 回（新規と入れ直し）走らせ、11 本とも長さが `manifest.json` と一致・1 ch・音量 1・`DD_SoundClass_Dialogue`、字幕は 5 本が `manifest.json` の文言と一致・6 本が空。保存済み（エディタに未保存なし）。
- C++ は変えていないのでビルドとテストは走らせていない（ステップ 4 の時点で C++ ビルド ok・自動テスト 154 件成功）。
- PIE: Zone 1 の台詞と字幕はステップ 3 で確かめた。Zone 2 とワサミの声はステップ 7 の通しで確かめる。
