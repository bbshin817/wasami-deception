---
title: 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す
status: 進行中
branch: main
base: 8b1cc1d
started: 2026-09-23 03:33
updated: 2026-09-23 04:05
---

# 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す

## 依頼

`.claude/roadmap.md` の項目 47（大目標 4）。2026-09-23 のユーザーの回答「はい（項目を立てて直す）」（項目 46 の要確認）。

前処理が作るマスターが `bUsedWithNanite`・`bUsedWithStaticLighting` を偽のまま持っている。**パッケージ版では既定の材質（灰色の市松）に落ちる**。

完了の条件:

1. どのマスターにどのフラグが要るかを洗い出す。→ **ステップ 1 で済んだ**
2. 前処理がマスターを作る・作り直すところでフラグを立てて保存し、マスターを作り直す。→ **ステップ 2（コード）・ステップ 3（作り直し）で済んだ**
3. PIE で Zone 1・Zone 2 の見た目が変わらないことを確かめる。→ **ステップ 3 で済んだ**
4. パッケージ版を作り直し、ログに `missing usage flag` が 1 件も出ないことと、祭壇の球が本家の見た目で出ることを確かめる。

## 計画

- [x] 1. 洗い出し（完了の条件 (1)）— 3 レベル・14 ブループリント・全メッシュのスロットを歩いて数え、`dd_stage.MASTER_USAGE` と 01 記録へ入れた。
- [x] 2. 前処理にフラグを入れる（完了の条件 (2)）— `dd_stage.MASTER_USAGE` と `dd_specials`・`dd_tablet`・`dd_shards` の 3 つ。`MASTER_VERSION` 1 → 2。
- [x] 3. マスター 10 個を作り直して PIE で確かめた（完了の条件 (3)）— `refresh_settings()`（マスター 7・インスタンス 158）+ `make_crystal()`・`make_minimap_materials()`・`import_mochi()`。10 個とも意図どおりのフラグがディスクに載り、Zone 1・Zone 2 の見た目は変わらなかった。
- [ ] 4. パッケージ版を作り直してログと祭壇の球を確かめる（完了の条件 (4)）← 次にやること
  - 変更予定: `Saved/Archive/Windows/`（git の外）

## 次にやること

ステップ 4。パッケージ版を作り直し、Zone 1・Zone 2 の**両方をしばらく歩いてから**ログを見る（手順と確かめ方は `.claude/guides/distribution.md`）。

- ログに `missing usage flag`（`Material … missing usage flag …` の警告）が 1 件も無いこと。
- ガレージの祭壇の球（`m_crystal_Inst2`）が灰色の市松ではなく紫で出ること。PIE での見え方は下の検証のとおり。
- 終わったら記録を消し、01 記録と handover を直して項目 47 を完了にする。

## 決定事項

（実装に入ったものは 01 記録の `ensure_masters` の「用途フラグ」の項へ移した）

- 2026-09-23: **既にあるフラグは消さない**。洗い出しは Niagara・Cascade を歩いていないので、粒子系のマスターに立っているフラグが「使われていない」ように見えるのは調べ漏れであって、余分ではない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **不思議な点（ステップ 4 で確かめる）**: 2026-09-22 のパッケージ版のログで落ちていたのは `m_crystal_Inst2`（`M_DD_Crystal` の子）の 2 件だけで、警告は **Zone 2 が読み込まれた瞬間**（`Audio Device registered with world 'L_Hospital_Zone2'` の直前）に出ている。Zone 1 を 64 秒遊んだ間は 1 件も出ていないが、ログはその直後（72 秒）に `quit` で終わっているので、**Zone 2 のほかの材質は確かめる間もなく終わった可能性が高い**。
- マスターの uasset は `/Content/Pipeline/` なので **git の対象外**（コミットは記録だけ）。フラグがディスクに載ったかは `bUsedWithNanite` などの名前がファイルのバイト列にあるかで分かる（既定値の偽なら名前表に載らない）。`Intermediate/Scratch/check_flags.py` がその確かめ（使い捨ての Python は `Intermediate/Scratch/`、git の対象外）。
- 長時間処理: パッケージの作り直し（ステップ 4）。マスターの作り直し（`Intermediate/Scratch/rebuild_masters.py`、約 10 分）はもう要らない。

## 検証

- check_records: OK（20 件。ステップ 2）
- C++ ビルド: 不要（この項目は Python の前処理だけ）
- エディタでの確認（ステップ 3）: 10 個ともフラグがディスクに載った（`check_flags.py`）。PIE の静止画（`Saved/Screenshots/WindowsEditor/HighresScreenshot00151〜00155.png`）で Zone 1 の開始地点・待合の廊下、Zone 2 のガレージの祭壇・迷路の入口・独房を見て、灰色の市松も真っ黒も無く、祭壇の球は紫のまま。`make_crystal` が `m_crystal_Inst2` の base property overrides を既定に戻すが、見え方は変わらなかった。
- メッシュの取り込みで L_Hospital_Zone1・Zone2 が dirty になったので保存した（中身の変化は無い）。エディタは Zone 1 を開いた状態に戻してある。
