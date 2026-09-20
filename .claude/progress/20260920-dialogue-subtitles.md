---
title: 台詞と字幕（作業一覧の項目 20）
status: 進行中
branch: main
base: ba43daf
started: 2026-09-20 12:17
updated: 2026-09-20 14:35
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

## 計画

- [x] 1. **台詞の取り込み**（完了）: `dd_dialogue.py`（Bierce の波 15 本と一言の Cue、字幕は本家の文字列表 `Strings` の文言を名前の対応で）、`dd_assets.sound(subtitles=)`、ツール `WasamiDDTools.import_dd_dialogue`。取り込んだ中身と根拠は 10 記録の「台詞の取り込み」。
- [x] 2. **話し役 `AWasamiBierceTalk`**（完了）: 本家の `BierceTalk_Blueprint` を写し（`Talk`・`StopTalking`・`bHalt`・0.5 s の待ち・`Find`）、`dd_level._flow` が両ゾーンに 1 つずつ置くようにした。中身と根拠は 10 記録の「話し役 `AWasamiBierceTalk`」。
- [x] 3. **Zone 1 の配線**（完了）: 流れに `BierceTalk` の口を足し、`04_DoorBreak`（1 s 後の `Event_10`）・`04_Intercom`（放送 0.6 → 13 s → `Event_09`）・`Setup Nurse Bierce Quips`（発見の 5 回に 1 回の一言）を埋めた。中身と根拠は 11 記録。
- [x] 4. **Zone 2 の配線**（完了）: 流れの 8 か所に本家の秒数どおりの口を足した（`Event_17`・`Event_19` の 2 か所・`Gameplay_08`〈唯一の減衰あり〉・リフトの `Gameplay_07`〈1 回だけ〉・`Event_20`・`Event_21`・`Event_22`）と、Matron の裏の放送（置いてある `AAmbientSound` を鳴らす）。中身と根拠は 11 記録。
- [x] 5. **ワサミの声の取り込み**（完了）: `Tools/dd/prepare_voices.py`（mp3 → wav）、`SourceArt/Wasami/Voices/`（wav 15 + `manifest.json`。Git LFS）、`dd_voices.py`・`dd_assets.sound_file`、ツール `WasamiDDTools.import_wasami_voices`。WebGL 版が鳴らす 11 本を `/Game/Wasami/Voices/Wasami_<Id>` に（字幕 5 本・字幕なし 6 本・`DD_SoundClass_Dialogue`）。中身と根拠は 10 記録の「ワサミの声の取り込み」。
- [x] 6. **ワサミの声の口と 2D の 6 本**（完了）: `WasamiVoice`（表・`Say`・`ShowSubtitle`）を作り、`greeting`（Zone 1 の `InitialStart`）・`well`（1 個目のシャード）・`fast`（ブースト）・`best`（秘密の壁の 0.5 s 後）・`fine`・`over`（死亡画面）を付けた。中身と根拠は 10 記録の「ワサミの声を鳴らす口」。
- [x] 7. **敵の声**（完了）: `AWasamiEnemy` に本家の `Talk Audio` と `Talk` を足し、発見（`Found`。ゲームモードの `TakeFoundVoice` で全体 12 s に 1 回）と巡回・気絶の呼びかけ（4 本を 14〜26 s ごと）を鳴らすようにした。中身と根拠は 07 記録の「声」。
- [ ] 8. **確かめと締め**（PIE で Zone 1・Zone 2 の台詞と字幕、ワサミの声を通しで確かめ、実装記録・note を直して項目 20 を閉じる）

## 次にやること

ステップ 8（確かめと締め）。**これで項目 20 のすべての声と台詞がつながったので、通しで確かめて記録を閉じる**。

- PIE で Zone 1 を頭から通し、Bierce の台詞（扉・館内放送・発見の一言）と字幕、`greeting`・`well`・`fast`・`best`、敵の発見と呼びかけが出ることを見る。`Wasami.Flow` の各イベントで区間を飛ばしてよい。
- Zone 2 も同じく（独房・Matron・迷路・リフト・欠片・ガレージの 8 か所と、Matron の裏の館内放送）。
- **字幕を撮るときは収録して後からコマを見る**（`desktop.py record` → `ffmpeg` でコマ。字幕は `slomo` の影響を受けない実時間で消える。10 記録の注意点）。PIE のビューポートの下端はログの窓に隠れるので、撮る範囲は画面の右寄りを避けて中央を入れる。
- 直す: `docs/note/progress.md` と note の記事（ガイド `note-progress.md`）、`.claude/references/handover.md` の「現状と次の一歩」、`.claude/roadmap.md` の項目 20 を完了に。
- 終わったら進捗記録を消して最後のコミットに含める。**大目標 2 の残りは項目 28 だけ**になるので、作業一覧の未着手を確かめる。

## 決定事項

- 2026-09-20（ステップ 7）: **呼びかけの間隔は本家の 3〜10 s ではなく WebGL 版の 14〜26 s** — 本家のナースは巡回・追跡・透明の 3 本を状態で使い分けるが、本作の呼びかけは 4 本しかなく、3〜10 s では同じ声がすぐ繰り返す。ほかは本家どおり（`Talk` の `bForce`、気絶中も喋る、見張りは飛び降りてから）。
- 2026-09-20: **字幕は UE の仕組み（`USoundWave.Subtitles` + `SetSubtitlesEnabled`）に載せ、作り直さない** — 本家も同じ仕組みで、オプションの SUBTITLES はすでに `SetSubtitlesEnabled` を呼んでいる（`.claude/guides/original-fidelity.md` の「UE に同じ仕組みがあれば値を写すだけ」）。

## 要確認（ユーザー）

- 2026-09-20: 本家の病院の Bierce の台詞の波に字幕が入っていない（`Strings` に文言 127 件はあるが、波の `Subtitles` を使っているのは本作が作らない入口のナースの 40 件だけ。第 4 章の下水の Bierce には入っている） — 仮に **名前の対応（`Bierce_TormentTherapy_Event_NN` ↔ `06_Cutscene_Zone_01_Bierce_NN`）で字幕を入れる**。理由: 項目 20 の完了の条件が「字幕は `_strings.json` の文言」で、オプションに SUBTITLES がある以上、出ないほうが不自然。本家の実機で確かめるのは大目標 2 の決め方（見た目の詰めをしない）で控えている。場所: ステップ 1 の `dd_dialogue.py`。

## 再開時の注意

- 敵の声の減衰 `/Game/DD/Audio/Misc/AgathaAttenuation` は取り込み・保存済み。入れ直すときは `dd_enemy.import_enemy_audio()`（`WasamiDDTools.import_wasami_enemy` の一部）。
- 台詞の波と Cue（`/Game/DD/Audio/…`）と話し役（両ゾーンに配置）は取り込み・保存済み。入れ直すときは `WasamiDDTools.import_dd_dialogue`。
- ワサミの声は `/Game/Wasami/Voices/Wasami_<Id>` に取り込み・保存済み（11 本）。入れ直すときは `WasamiDDTools.import_wasami_voices`（原本を作り直すなら先に `python Tools/dd/prepare_voices.py`。ffmpeg が要る）。`/Content/Wasami/` は git の外なので、エディタを作り直した人は取り込みから。
- 長時間処理: `python Tools/editor_cycle.py`（C++ のビルド。1 分ほど）。
- **テストで話し役を置くときは `AWasamiTestBierceTalk`（`Tests/WasamiTestBierceTalk.h`）を使う** — テストのワールドのティックは音声装置を回さないので、素の `AWasamiBierceTalk` だと `Play` した部品がいつまでも「鳴っている」ままになり、2 本目以降の台詞が永久に待たされる。`Tests/WasamiZoneFlowTests.cpp` の `SpawnTalker`・`Spoken` がその形（続けて確かめるときは `SetSound(nullptr)` で前の台詞を消す）。
- 自動テストは背面のエディタだと進まないので、走らせる前に `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`、終わったら `True` に戻す（症状索引）。

## 検証

- check_records: OK（20 件）
- ステップ 7: C++ のビルド ok、**自動テスト 157 件すべて成功**（新しい `Wasami.Enemy.Voice.Calls` を含む）。PIE（Zone 1）で敵を出して見た: 見つけると `Wasami_Found` を音量 1・`AgathaAttenuation` 越しに鳴らし、**字幕「ここか！」が約 2.5 s 出る**（収録のコマで確かめた）。12 s 経たない 2 体目は鳴らさない。追跡中は呼びかけを言わず、巡回では `Calling`・`Think`・`Remember` などを 0.9 で言う。PIE は止めた。
- 残り: ステップ 8 の通し（Zone 1・Zone 2 の台詞と字幕、ワサミの声、敵の声）。
