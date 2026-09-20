---
title: 台詞と字幕（作業一覧の項目 20）
status: 進行中
branch: main
base: ba43daf
started: 2026-09-20 12:17
updated: 2026-09-20 14:05
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

### 敵の声（ステップ 7 で使う）

WebGL 版 15 記録の鳴らし方（波は `/Game/Wasami/Voices/Wasami_<Id>`、鳴らす口は `WasamiVoice`。どちらも 10 記録）:

- 発見の `found`（音量 1.0・字幕あり）… **どの敵からでも 12 s に 1 回まで**、その敵の頭の位置で。本家の同じ場所は `ChasePlayer` の DoOnce（`bDetectionClosed`）の中の「Detected line」。
- 巡回中と硬直中の `calling`・`others`・`think`・`remember`（0.9・字幕なし）… **14〜26 s ごと**にどれか 1 本を敵の位置で（本家の `Random Idle Sounds` は 5〜10 s だが、4 本しかないので WebGL 版の間隔を採る）。
- 本家のナースは `Talk Audio`（`UAudioComponent`、`AttenuationSettings` = `Audio/Misc/AgathaAttenuation`〈`bEnableOcclusion` 真・`LPFRadiusMin` 2000・`LPFRadiusMax` 3600・`LPFFrequencyAtMax` 50・`OcclusionLowPassFilterFrequency` 10000・`OcclusionInterpolationTime` 0.5・`NaturalSound`・`FalloffDistance` 8000。`pak_reference_2/_assets/.../Nurse/BP_06_ReaperNurse.json`）を持つので、それを写して鳴らす。減衰の取り込みは `dd_enemy.py` に足す（`dd_assets.attenuation`）。
- 字幕を `WasamiVoice::Say` と同じ長さで出すには、部品の `bSuppressSubtitles` を真にして `WasamiVoice::ShowSubtitle` を呼ぶ。

## 計画

- [x] 1. **台詞の取り込み**（完了）: `dd_dialogue.py`（Bierce の波 15 本と一言の Cue、字幕は本家の文字列表 `Strings` の文言を名前の対応で）、`dd_assets.sound(subtitles=)`、ツール `WasamiDDTools.import_dd_dialogue`。取り込んだ中身と根拠は 10 記録の「台詞の取り込み」。
- [x] 2. **話し役 `AWasamiBierceTalk`**（完了）: 本家の `BierceTalk_Blueprint` を写し（`Talk`・`StopTalking`・`bHalt`・0.5 s の待ち・`Find`）、`dd_level._flow` が両ゾーンに 1 つずつ置くようにした。中身と根拠は 10 記録の「話し役 `AWasamiBierceTalk`」。
- [x] 3. **Zone 1 の配線**（完了）: 流れに `BierceTalk` の口を足し、`04_DoorBreak`（1 s 後の `Event_10`）・`04_Intercom`（放送 0.6 → 13 s → `Event_09`）・`Setup Nurse Bierce Quips`（発見の 5 回に 1 回の一言）を埋めた。中身と根拠は 11 記録。
- [x] 4. **Zone 2 の配線**（完了）: 流れの 8 か所に本家の秒数どおりの口を足した（`Event_17`・`Event_19` の 2 か所・`Gameplay_08`〈唯一の減衰あり〉・リフトの `Gameplay_07`〈1 回だけ〉・`Event_20`・`Event_21`・`Event_22`）と、Matron の裏の放送（置いてある `AAmbientSound` を鳴らす）。中身と根拠は 11 記録。
- [x] 5. **ワサミの声の取り込み**（完了）: `Tools/dd/prepare_voices.py`（mp3 → wav）、`SourceArt/Wasami/Voices/`（wav 15 + `manifest.json`。Git LFS）、`dd_voices.py`・`dd_assets.sound_file`、ツール `WasamiDDTools.import_wasami_voices`。WebGL 版が鳴らす 11 本を `/Game/Wasami/Voices/Wasami_<Id>` に（字幕 5 本・字幕なし 6 本・`DD_SoundClass_Dialogue`）。中身と根拠は 10 記録の「ワサミの声の取り込み」。
- [x] 6. **ワサミの声の口と 2D の 6 本**（完了）: `WasamiVoice`（表・`Say`・`ShowSubtitle`）を作り、`greeting`（Zone 1 の `InitialStart`）・`well`（1 個目のシャード）・`fast`（ブースト）・`best`（秘密の壁の 0.5 s 後）・`fine`・`over`（死亡画面）を付けた。中身と根拠は 10 記録の「ワサミの声を鳴らす口」。
- [ ] 7. **敵の声**（`found` と巡回の 4 本。本家の `Talk Audio` の部品と `AgathaAttenuation` を足し、発見は全体で 12 s に 1 回、巡回・硬直は 14〜26 s ごと）
  - 変更予定: `WasamiEnemy.h/.cpp`、`Tests/WasamiEnemyTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_enemy.py`（`Audio/Misc/AgathaAttenuation` の取り込み）
- [ ] 8. **確かめと締め**（PIE で Zone 1・Zone 2 の台詞と字幕、ワサミの声を通しで確かめ、実装記録・note を直して項目 20 を閉じる）

## 次にやること

ステップ 7（敵の声）。上の「敵の声（ステップ 7 で使う）」のとおりに `AWasamiEnemy` へ付ける。

- `dd_enemy.py` に `Audio/Misc/AgathaAttenuation` の取り込みを足し（`ENEMY_ATTENUATIONS` の形）、エディタで `WasamiDDTools.import_dd_enemy…` を走らせて `/Game/DD/Audio/Misc/AgathaAttenuation` を作る。
- `AWasamiEnemy` に `TalkAudio`（本家の `Talk Audio`。`CollisionCylinder` に付け、`bAutoActivate` 偽・減衰は上の 1 つ）を足し、`bSuppressSubtitles` を真にしてから `Play`、字幕は `WasamiVoice::ShowSubtitle`。
- 発見は `ChasePlayer` の `bDetectionClosed` の DoOnce の中（本家の Detected line の場所）。**12 s の間隔は敵ごとではなく全体**なので、置き場所を決める（案: `AWasamiGameMode` に持たせ、純粋な小さな構造体〈`Allow(いまの時刻)`〉にしてテストする）。
- 巡回・硬直の 4 本は `MakeChoice`（0.5 s ごとの決定）から数えるか、自前のタイマーで 14〜26 s の乱数。硬直中も鳴らす（本家の `Random Idle Sounds` と同じ）。
- テストは `Tests/WasamiEnemyTests.cpp` に間隔と回数の規則（純粋な部分）を足す。音声装置の無いテストでは `WasamiVoice::Say` は何もしない。

## 決定事項

- 2026-09-20（ステップ 6）: **ワサミの声の字幕は波のものを止めて出し直す** — WebGL 版の `max(2.2, 長さ + 1.2)` を守るため。UE の既定は波の長さで消えるので、0.5 s の声は読めない。`bSuppressSubtitles` + `FSubtitleManager::QueueSubtitles`（`USoundWave::HandleStart` と同じ呼びで長さだけ差し替え）。
- 2026-09-20（ステップ 6）: **死亡画面は本家の Bierce の死亡台詞の「長さの表」をそのまま待ちに使い、鳴らす声だけワサミにする** — 本家の台詞の波は取り込んでおらず（本作は病院の TT の 15 本だけ）、WebGL 版も `fine`・`over` に替えていた。表（`VoiceLengths`）は本家の記録として残す。
- 2026-09-20（ステップ 6）: **`greeting` は Zone 1 の `InitialStart` の据え置きが明ける 10 s** — WebGL 版の `greet()`（1 回の遊びの始めで 1 個も取っていないとき）に当たるのが `IsNewStart` の道だけだから。ステージ OP の札は 9.2 s で薄れて 11 s で外れるので、字幕は札の後ろに隠れずに読める（PIE で確かめた）。Zone 2 の始まりでは鳴らさない（同じ遊びの続き）。
- 2026-09-20（ステップ 6）: **`best` は秘密の部屋の区域ではなく秘密の壁（`AWasamiSecretWall`）の最初の使用の 0.5 s 後** — WebGL 版の規則が「隠し扉が開いた 0.5 s 後」だから。本家はここで何も喋らない（Bierce が喋るのは書類のほう）。
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
- C++ のビルド ok（`Tools/editor_cycle.py`）。自動テスト **156 件すべて成功**（新しい `Wasami.Voice.Table`・`Wasami.Voice.Waves` を含む。`Waves` は取り込んだ 11 本の長さ・クラス・字幕の有無を実際の波で見る）。
- PIE（Zone 1。`Wasami.ResetSave` → PIE を開き直して新しい始まりにし、`Tools/desktop.py shot` で撮った）: 10 s で操作が戻るところで **`greeting` の字幕「こんにちワサミ！」が画面下に出た**（ゲーム時刻 11.4 s の絵）。**波が終わる 13.88 s より後の 14.9 s でも出ていた**ので、`SubtitleSeconds`（5.078 s）で出し直せている。PIE は止めた。
  - **背面のエディタでは `desktop.py shot` に数秒前のコマが写る**（スロットル。症状索引に足した）。撮る前に `bThrottleCPUWhenNotForeground` を偽にする。
- 取り込み（ステップ 5）: 11 本とも長さが `manifest.json` と一致・1 ch・音量 1・`DD_SoundClass_Dialogue`、字幕は 5 本が文言と一致・6 本が空（`Wasami.Voice.Waves` が毎回見る）。
- 残り: Zone 2 の台詞と、敵の声（ステップ 7）・通し（ステップ 8）。
