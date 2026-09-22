---
title: 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す
status: 進行中
branch: main
base: 8b1cc1d
started: 2026-09-23 03:33
updated: 2026-09-23 04:35
---

# 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す

## 依頼

`.claude/roadmap.md` の項目 47（大目標 4）。2026-09-23 のユーザーの回答「はい（項目を立てて直す）」（項目 46 の要確認）。

前処理が作るマスターが `bUsedWithNanite`・`bUsedWithStaticLighting` を偽のまま持っている。**パッケージ版では既定の材質（灰色の市松）に落ちる**。

完了の条件:

1. どのマスターにどのフラグが要るかを洗い出す。→ **ステップ 1 で済んだ**
2. 前処理がマスターを作る・作り直すところでフラグを立てて保存し、マスターを作り直す。→ **コードはステップ 2 で済んだ（作り直しはステップ 3）**
3. PIE で Zone 1・Zone 2 の見た目が変わらないことを確かめる。
4. パッケージ版を作り直し、ログに `missing usage flag` が 1 件も出ないことと、祭壇の球が本家の見た目で出ることを確かめる。

## 計画

- [x] 1. 洗い出し（完了の条件 (1)）— 3 レベル・14 ブループリント・全メッシュのスロットを歩いて数えた。結果は `dd_stage.MASTER_USAGE` と 01 記録の `ensure_masters` の「用途フラグ」の項へ入れた。
- [x] 2. 前処理にフラグを入れる（完了の条件 (2)）— `dd_stage.MASTER_USAGE`（ステージの 7 つ）と `dd_specials._build_crystal`・`dd_tablet._build_map_plane`・`dd_shards._build_mochi`。`MASTER_VERSION` 1 → 2。`_build_substance` の `used_with_skeletal_mesh` は表へ寄せた。01 記録も直した。
- [ ] 3. マスターを作り直して PIE で確かめる（完了の条件 (3)）← 次にやること
  - 変更予定: `/Game/Pipeline/Materials/` のマスター 10 個の uasset（作り直し）
- [ ] 4. パッケージ版を作り直してログと祭壇の球を確かめる（完了の条件 (4)）
  - 変更予定: `Saved/Archive/Windows/`（git の外）

## 次にやること

ステップ 3。マスター 10 個を作り直して保存し、PIE で見た目が変わらないことを確かめる。

- ステージの 7 つ: `MASTER_VERSION` を上げたので `WasamiStageTools.refresh_dd_stage_assets()` が作り直す（インスタンスの再コンパイルも走るので時間がかかる。MCP かバックグラウンドの `Tools/ue_remote.py` で走らせ、待つ間に別のことをする）。
- ステージの外の 3 つは `dd_assets.material()` が毎回作り直すので、それぞれの取り込みを呼べばよい（`import_dd_specials`・`import_dd_tablet`・`import_dd_shards` のツール名は呼ぶ前に `describe_toolset` か `dd_tools.py` で確かめる）。重いなら `dd_specials.make_crystal()`・`dd_tablet.make_minimap_materials()`・`dd_shards.import_mochi()` を直に呼ぶ。
- 立ったかは下の「再開時の注意」のバイト列の探し方で 10 個とも確かめる（`EAL.save_asset(..., only_if_is_dirty=False)` を忘れるとディスクに残らない）。
- PIE は Zone 1・Zone 2 を少し歩いて、材質が変わっていないこと（灰色の市松や真っ黒が出ないこと）を見る。終わったら必ず止める。
- コミットは uasset 10 個 + 記録。

## 決定事項

（実装に入ったものは 01 記録の `ensure_masters` の「用途フラグ」の項へ移した）

- 2026-09-23: **既にあるフラグは消さない**。洗い出しは Niagara・Cascade を歩いていないので、粒子系のマスターに立っているフラグが「使われていない」ように見えるのは調べ漏れであって、余分ではない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **不思議な点（ステップ 4 で確かめる）**: 2026-09-22 のパッケージ版のログで落ちていたのは `m_crystal_Inst2`（`M_DD_Crystal` の子）の 2 件だけで、警告は **Zone 2 が読み込まれた瞬間**（`Audio Device registered with world 'L_Hospital_Zone2'` の直前）に出ている。Zone 1 を 64 秒遊んだ間は、`M_DD_Substance`・`M_DD_Unlit` の子が Nanite+Static のメッシュ 1400 個以上に付いているのに 1 件も出ていない。ログはその直後（72 秒）に `quit` で終わっているので、**Zone 2 のほかの材質は確かめる間もなく終わった可能性が高い**。ステップ 4 では Zone 1・Zone 2 の両方をしばらく歩いてからログを見る。
- `.uasset` に用途フラグが保存されているかは、`bUsedWithNanite` などの名前がファイルのバイト列にあるかで分かる（既定値の偽なら名前表に載らない）。Git Bash に `strings` は無いので Python でバイト列を探す。
- 調べ物の使い捨ての Python は `Intermediate/Scratch/`（git の対象外）。
- 長時間処理: マスターの作り直し（ステップ 3。インスタンスの再コンパイル）とパッケージの作り直し（ステップ 4。手順と確かめ方は `.claude/guides/distribution.md`）。

## 検証

- check_records: OK（20 件。ステップ 2）
- C++ ビルド: 不要（この項目は Python の前処理だけ）
- エディタでの確認: ステップ 3 でこれから（ステップ 2 はコードだけ。`used_with_nanite`・`used_with_static_lighting`・`used_with_skeletal_mesh` の 3 つが `unreal.Material` のプロパティ名として通ることは `M_DD_Substance` を読んで確かめた）
