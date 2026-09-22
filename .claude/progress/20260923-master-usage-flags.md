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

1. どのマスターにどのフラグが要るかを洗い出す。→ **ステップ 1 で済んだ（下の表）**
2. 前処理がマスターを作る・作り直すところでフラグを立てて保存し、マスターを作り直す。
3. PIE で Zone 1・Zone 2 の見た目が変わらないことを確かめる。
4. パッケージ版を作り直し、ログに `missing usage flag` が 1 件も出ないことと、祭壇の球が本家の見た目で出ることを確かめる。

## 立てるフラグの表（ステップ 1 の結果）

`Intermediate/Scratch/usage_scan.py`・`usage_scan2.py`（使い捨て）で、3 つのレベル（Zone1・Zone2・Title、計 7088 個のプリミティブ）・14 個のブループリントの SCS・全スタティック/スケルタルメッシュのスロットを歩いて、マスターごとの実際の使われ方を数えた。

**ステージの 7 つ**（`dd_stage.ensure_masters`）:

| マスター | 現状 | 実際の使われ方（件数） | 立てる |
| --- | --- | --- | --- |
| `M_DD_Substance` | Skeletal | Nanite+Static 1314 / 非Nanite+Static 721 / Skeletal 187 / Nanite+Movable 128 | **Nanite・StaticLighting**（Skeletal は既存） |
| `M_DD_Unlit` | なし | Nanite+Static 116 / 非Nanite+Static 15 | **Nanite・StaticLighting** |
| `M_DD_Metal` | なし | Nanite+Movable 1（`ring_statue_2`） | **Nanite・StaticLighting**（下の決定） |
| `M_DD_SubstanceFresnel` | なし | Nanite+Movable 7（`BP_Collectable2`） | **Nanite・StaticLighting**（同上） |
| `M_DD_Glass` | なし | 非Nanite+Movable 130 / 非Nanite+Static 20 | **Nanite・StaticLighting**（同上） |
| `M_DD_GlassSewerage` | なし | 非Nanite+Static 301 | **Nanite・StaticLighting**（同上） |
| `M_DD_Decal`（Deferred Decal 領域） | なし | 非Nanite+Static 227（メッシュデカール） | **StaticLighting だけ** |

**ステージの外の 3 つ**（`dd_assets.material()` 経由）:

| マスター | 作る場所 | 実際の使われ方 | 立てる |
| --- | --- | --- | --- |
| `M_DD_Crystal` | `dd_specials.py`（`CRYSTAL_MASTER`, 202 行あたり） | Zone2 `ring_statue_orb_5` が Nanite+Static、Zone1 の `BP_BonusShard/soul_shard` が非Nanite+Movable 4 | **Nanite・StaticLighting** |
| `M_DD_MapPlane` | `dd_tablet.py`（`MAP_PLANE_MASTER`) | Zone1 `BP_MapTexture_2` が Static、Zone2 `BP_MapTexture_MultiFloor_2` が Movable | **StaticLighting** |
| `M_DD_WasamiMochi` | `dd_shards.py`（`MOCHI_MASTER`) | Nanite+Movable 679（`BP_Shard*/Mochi`） | **Nanite** |

**そのほかのマスターは追加不要**。粒子系（`M_DD_Ky*`・`M_DD_PPP*`・`M_DD_Whisp*` など）は既に各モジュールが `used_with_particle_sprites` 等を立てている。

## 計画

- [x] 1. 洗い出し（完了の条件 (1)）— 上の表。
- [ ] 2. 前処理にフラグを入れる（完了の条件 (2)）← 次にやること
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_stage.py`（`ensure_masters`・`MASTER_VERSION` を "2" へ）、`dd_specials.py`・`dd_tablet.py`・`dd_shards.py`、`/Game/Pipeline/Materials/*` のマスター 10 個
- [ ] 3. マスターを作り直して PIE で確かめる（完了の条件 (3)）
  - 変更予定: マスターの uasset（作り直し）
- [ ] 4. パッケージ版を作り直してログと祭壇の球を確かめる（完了の条件 (4)）
  - 変更予定: `Saved/Archive/Windows/`（git の外）

## 次にやること

ステップ 2。上の表のとおりフラグを立てる。

- `dd_stage.py`: `ensure_masters` がマスターごとに用途フラグを立てるようにする（表を定数の辞書で持つのが分かりやすい）。`MASTER_VERSION` を `"2"` に上げて作り直させる。268 行の `used_with_skeletal_mesh`（`_build_substance` の中）は表に寄せて重複を無くす。
- `dd_specials.py`・`dd_tablet.py`・`dd_shards.py`: それぞれのマスターを作った直後に `set_editor_property` でフラグを立てる（`dd_gimmicks.py` の 306 行あたりと同じ書き方）。**フラグを変えたら `MEL.recompile_material(mat)` と `EAL.save_asset(..., only_if_is_dirty=False)` が要る**。
- コミットはステップ 2（コードだけ）とステップ 3（uasset の作り直し）で分ける。

## 決定事項

- 2026-09-23: **本家の `pak_reference_2/_materials.json` に用途フラグは入っていない**（`bUsedWith*` が 1 件も無い。cook で落ちる）。なので本作の実際の使われ方から決めた。
- 2026-09-23: 用途フラグ 1 つにつきシェーダーの組み合わせが増えるので、**全部のマスターに全部立てず、使うものだけ立てる**（VRAM 6 GB。`.claude/guides/performance.md`）。
- 2026-09-23: ただし**ステージの 6 つ（Decal 以外）には Nanite と StaticLighting を揃って立てる**。どのステージメッシュが Nanite か・Static かは取り込みのたびに変わりうるもので、いまの内訳（`M_DD_Metal` は Nanite+Movable が 1 件だけ、`M_DD_Glass` は Nanite 0 件…）はその時点の写しでしかない。6 つ × 2 フラグなら増える組み合わせは小さい。`M_DD_Decal` は Deferred Decal 領域なので Nanite は意味を持たず、StaticLighting だけにする。
- 2026-09-23: **既にあるフラグは消さない**。洗い出しは Niagara・Cascade を歩いていないので、粒子系のマスターに立っているフラグが「使われていない」ように見えるのは調べ漏れであって、余分ではない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **不思議な点（ステップ 4 で確かめる）**: 2026-09-22 のパッケージ版のログで落ちていたのは `m_crystal_Inst2`（`M_DD_Crystal` の子）の 2 件だけで、警告は **Zone 2 が読み込まれた瞬間**（`Audio Device registered with world 'L_Hospital_Zone2'` の直前）に出ている。Zone 1 を 64 秒遊んだ間は、`M_DD_Substance`・`M_DD_Unlit` の子が Nanite+Static のメッシュ 1400 個以上に付いているのに 1 件も出ていない。材質そのものに違いは無い（`m_crystal_Inst2` は `base_property_overrides` を 1 つも上書きしていない素の不透明・DefaultLit で、`m_crystal` → `M_DD_Crystal` の 2 段）。ログはその直後（72 秒）に `quit` で終わっているので、**Zone 2 のほかの材質は確かめる間もなく終わった可能性が高い**。ステップ 4 では Zone 1・Zone 2 の両方をしばらく歩いてからログを見る。
- `.uasset` に用途フラグが保存されているかは、`bUsedWithNanite` などの名前がファイルのバイト列にあるかで分かる（既定値の偽なら名前表に載らない）。Git Bash に `strings` は無いので Python でバイト列を探す。この方法で、いまディスク上に立っているのは `M_DD_Substance`・`M_DD_WasamiGltf` の `bUsedWithSkeletalMesh` と粒子系だけだと確かめた。
- 調べ物の使い捨ての Python は `Intermediate/Scratch/`（git の対象外）。レベルは `unreal.load_asset` してから `unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)` で歩ける（レベルを開き直さなくてよい）。ブループリントの部品は `unreal.SubobjectDataSubsystem.k2_gather_subobject_data_for_blueprint(bp)`。
- 長時間処理: パッケージの作り直し（ステップ 4）。手順と確かめ方は `.claude/guides/distribution.md`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目は Python の前処理だけ）
- エディタでの確認: ステップ 1 は読み取りのみ（アセットは変更していない）
