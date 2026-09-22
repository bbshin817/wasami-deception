---
title: 項目 50 EXTRAS から曲を外す（書類の解放も追随）
status: 進行中
branch: main
base: a43279b
started: 2026-09-23 05:16
updated: 2026-09-23 05:16
---

# 項目 50 EXTRAS から曲を外す（書類の解放も追随）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 4 の項目 50。2026-09-23 のユーザーの回答「EXTRAS に曲は不要」「区分は残して曲を置かない」（項目 29・35 の要確認 2 件）。

いま EXTRAS の SOUND ARCHIVE には本作の曲 4 本（`Cold Hearted` と仮の名前 3 つ）が並び、Zone 1 の秘密の書類が 4 本とも解放する。**曲は 1 本も置かず、SOUNDS の区分は本家の木のまま残していつも鍵**にする（MOVIES と同じ扱い）。仮の曲名の件（要確認）はこれで消える。

完了の条件（作業一覧の原文）:

- (1) `UWasamiExtrasWidget` の `SoundTracks` を空にする。区分の並び・鍵の見せ方は本家の `UMG_Extras` のままで、SOUNDS を選んでも解放が 1 つも無い状態が本家の見せ方どおりに出る。
- (2) 書類 `AWasamiCollectable` の解放から曲を外す（組み立ての `dd_level.COLLECTABLE_SOUNDS`）。Zone 1 の書類は Art Gallery 19・20 だけを解放する（本家の値どおり）。セーブの `ExtrasSFX` は本家のセーブの形なので残す（いつも空）。
- (3) PIE で書類を取り、タイトルの EXTRAS で Art だけが解放され SOUNDS に曲が 1 本も出ないことを確かめる。テスト `Wasami.Extras.*`・`Wasami.Secrets.Collectable.Unlock` を直す。

## 計画

- [ ] 1. C++: SOUND ARCHIVE を本家の木に戻して空にする ← 次
  - `UWasamiExtrasWidget` の `SoundTracks` を空（`SetNum` ごと外す）にし、`SoundCount` 4 → 本家の数に戻す（下の「決定事項」。まず `pak_reference_2/.../UMG_Extras.json` の `WrapBox_2` の子の数を数えて確かめる）。使わなくなる `ExtrasMusicTrack` ヘルパーも外す。
  - テスト `Wasami.Extras.Screen`（欄の数）・`.SoundButton`（曲の名前を渡している）を直す。`Wasami.Extras.Item` の `Extras_SFX` の行はセーブの仕組みの話なのでそのまま。
  - ビルド（`python Tools/editor_cycle.py`）→ `Wasami.Extras.*` を走らせる。
  - 変更予定: `Source/wasami_deception/WasamiExtrasWidget.h`・`.cpp`、`Source/wasami_deception/Tests/WasamiExtrasTests.cpp`
- [ ] 2. 前処理: 書類の解放から SOUND を外して Zone 1 を組み直す
  - `dd_level.COLLECTABLE_SOUNDS` を無くし、`_collectables` が本家の `Type` = SOUND の項目を落とすようにする（本家の値どおり Art Gallery だけが残る）。
  - Zone 1（要れば Zone 2 も）を組み直して、書類の `Collectables` が Art 19・20 だけになったことを確かめる。
  - テスト `Wasami.Secrets.Collectable.Unlock` の Sound を使う行を見直す（セーブの `Unlock` の分岐の確かめはそのまま残す。置いた書類の値の確かめだけ直す）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_level.py`、`Source/wasami_deception/Tests/WasamiSecretsTests.cpp`、`/Game/Wasami/Levels/L_Hospital_Zone1`（要れば `_Zone2`）
- [ ] 3. PIE で確かめて記録を締める
  - PIE で Zone 1 の書類（秘密のエレベーターの奥、ID 1）を取り、セーブの `ExtrasSFX` が空のままで `ExtrasArt` に 19・20 が入ることを見る。続けてタイトルへ移り、EXTRAS の ART GALLERY は 19・20 が解放・SOUND ARCHIVE は全部鍵で名前が 1 つも出ないことを撮る。
  - 実装記録 19（曲の一覧・`SoundCount`・要確認）・18（書類の解放）・01（`COLLECTABLE_SOUNDS`）を直し、`python .claude/scripts/check_records.py --update`。
  - 作業一覧の項目 50 を「完了（2026-09-23）」にし、進捗記録を消す。

## 次にやること

ステップ 1。まず `python -c` で `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_Extras.json` の `WrapBox_2`（曲の欄）の子の数を数えて本家の `SoundCount` を確かめ、`UWasamiExtrasWidget` の `SoundTracks` を空に・`SoundCount` をその数に戻す。

## 決定事項

- 2026-09-23: **`SoundCount` は本家の数（記録 19 では 10）に戻す**。ユーザーの「区分は残して曲を置かない」「本家の木のまま残していつも鍵」に従う。2026-09-21 の「余る欄は減らす」は本作の曲を並べる前提の指示で、曲を 1 本も置かない今は本家の木がそのまま正しい（日記 10・動画 10 と同じ扱い）。本家の木の実数はステップ 1 の頭で `UMG_Extras.json` から数えて確かめる。
- 2026-09-23: **セーブの `ExtrasSFX` と `UWasamiSaveGame::Unlock` の SOUND の分岐は残す**（作業一覧の (2)。本家のセーブの形。いつも空になるだけ）。だからセーブの仕組みを見るテスト（`Wasami.Secrets.Save.*` の Sound の行）も残す。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- C++ を変えるステップ 1 のビルドは `python Tools/editor_cycle.py`（エディタを閉じて開き直す。確認は要らない）。テストは `Wasami.Extras` を Automation で走らせる。
- ステップ 2 のレベルの組み直しは `Content/Python/wasami_tools` のツールセットを MCP か `Tools/ue_remote.py` から呼ぶ（`.claude/guides/unreal-workflow.md`）。前後で保存する。
- PIE は終わったら必ず止める。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
