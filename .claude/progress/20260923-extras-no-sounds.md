---
title: 項目 50 EXTRAS から曲を外す（書類の解放も追随）
status: 進行中
branch: main
base: a43279b
started: 2026-09-23 05:16
updated: 2026-09-23 05:40
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

- [x] 1. C++: SOUND ARCHIVE を本家の木に戻して空にした（`SoundTracks`・`FWasamiExtrasTrack`・`ExtrasMusicTrack` を消し、`SoundCount` 4 → 10。テスト 5 件成功。19 記録）
- [>] 2. 前処理: 書類の解放から SOUND を外して Zone 1 を組み直す ← 次
  - `dd_level.COLLECTABLE_SOUNDS` を無くし、`_collectables` が本家の `Type` = SOUND の項目を落とすようにする（本家の値どおり Art Gallery だけが残る）。
  - Zone 1（要れば Zone 2 も）を組み直して、書類の `Collectables` が Art 19・20 だけになったことを確かめる。
  - テスト `Wasami.Secrets.Collectable.Unlock` の Sound を使う行を見直す（セーブの `Unlock` の分岐の確かめはそのまま残す。置いた書類の値の確かめだけ直す）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_level.py`、`Source/wasami_deception/Tests/WasamiSecretsTests.cpp`、`/Game/Wasami/Levels/L_Hospital_Zone1`（要れば `_Zone2`）
- [ ] 3. PIE で確かめて記録を締める
  - PIE で Zone 1 の書類（秘密のエレベーターの奥、ID 1）を取り、セーブの `ExtrasSFX` が空のままで `ExtrasArt` に 19・20 が入ることを見る。続けてタイトルへ移り、EXTRAS の ART GALLERY は 19・20 が解放・SOUND ARCHIVE は全部鍵で名前が 1 つも出ないことを撮る。
  - 実装記録 19（曲の一覧・`SoundCount`・要確認）・18（書類の解放）・01（`COLLECTABLE_SOUNDS`）を直し、`python .claude/scripts/check_records.py --update`。
  - 作業一覧の項目 50 を「完了（2026-09-23）」にし、進捗記録を消す。

## 次にやること

ステップ 2。`Content/Python/wasami_tools/pipeline/dd_level.py` の `COLLECTABLE_SOUNDS`（今は `(0, 1, 2, 3)`）を無くし、`_collectables` が本家の `Type` = SOUND の項目を落とすようにしてから、Zone 1 を組み直して書類の `Collectables` が Art 19・20 だけになったことを確かめる。

## 決定事項

- 2026-09-23: **本家の曲の欄は 10**（`UMG_Extras` の `WrapBox_2` の子＝`UMG_Extras_Sound_Button` が 10 件）。`SoundCount` をこれに戻した。
- 2026-09-23: **セーブの `ExtrasSFX` と `UWasamiSaveGame::Unlock` の SOUND の分岐は残す**（作業一覧の (2)。本家のセーブの形。いつも空になるだけ）。だからセーブの仕組みを見るテスト（`Wasami.Secrets.Save.*` の Sound の行）も残す。ステップ 1 では、テストの共通のセーブ `ExtrasSave()` から曲の解放を外し、曲の解放が要るテスト（`Wasami.Extras.Item`・`.SoundButton`）はその場で自分で `Unlock` するようにした。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ステップ 2 のレベルの組み直しは `Content/Python/wasami_tools` のツールセットを MCP か `Tools/ue_remote.py` から呼ぶ（`.claude/guides/unreal-workflow.md`）。前後で保存する。
- テストはエディタの中で `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests <filter>')`。**背面のエディタは 3 fps で `FWaitForInteractiveFrameRate` が進まない**ので、先に `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`（終わったら `True` に戻す）。結果は `Saved/Logs/wasami_deception.log` の `Test Completed`。
- PIE は終わったら必ず止める。

## 検証

- check_records: OK（20 件。ステップ 1）
- C++ ビルド: OK（`python Tools/editor_cycle.py`。ステップ 1）
- テスト: `Wasami.Extras` 5 件すべて成功（ステップ 1）
- エディタでの確認（組み立て・PIE）: 未実行（ステップ 2・3）
