---
title: ガラスが透けない（ガラスのマスターを原作のシェーダーから組む）（作業一覧の項目 46）
status: 進行中
branch: main
base: 9a7045b
started: 2026-09-22 18:49
updated: 2026-09-22 20:45
---

# ガラスが透けない（作業一覧の項目 46）

## 依頼

ゲームレビュアーの指摘（Medium）「本家であれば透過しているはずの、Zone1 の扉のガラスなどが燻んでいる」。
本作はガラスの系列も一律に `M_DD_Substance` の子にしていて、この材質の不透明度は「アルベドの α × `Opacity Override`（既定 1）」なので、
α が 1 のテクスチャ（扉のガラスは `T_White`）では透けず、くすんだ板に見える。

完了の条件（`.claude/roadmap.md` の項目 46）:
(1) `MM_Main_Substance_Glass` と `_ColorMask` のコンパイル済みのシェーダーを読み、式どおりのマスターを作る（項目 31 の `M_DD_Metal`・`M_DD_SubstanceFresnel` と同じやり方）。
(2) 前処理 `MASTERS` の振り分けにガラスを足し、ガラスのインスタンスを新しいマスターの子にする。
(3) Zone 1 の扉のガラスと外のガラスを本家の絵と見比べ、透け方と映り込みが同じに見えることを確かめる。半透明は Nanite を切る対象なので、焼き込みと fps も見る。

## 計画

- [x] 1〜2. 原作のガラスのマスター 3 つ（`MM_Main_Substance_Glass`・`_ColorMask`・Sewerage の `M_Glass`）のシェーダーを読み、式を書き出した（式は 01 記録の `ensure_masters` に写した）。
- [x] 3. `paths.py`・`dd_stage.py`・`prepare_stage.py` を直した（コミット参照）。マスター 2 つ `M_DD_Glass`（`UseMaskColor` で `_ColorMask` と選ぶ）・`M_DD_GlassSewerage`、前処理の振り分け `glass`・`sewerglass`、`bOverride_BlendMode` の読み直し。両方の組み立てを一時マテリアルで空打ちして、つなぎ先の名前がすべて通ることを確かめた。
- [ ] 4. 前処理をやり直し、エディタでマスターとインスタンスを作り直す ← 次
  - `python Tools/dd/prepare_stage.py` → エディタで `dd_stage.refresh_settings()`（`materials_remade` が 4 以上になるはず）
  - `MM_Main_Substance_Glass_Doors`・`_Police_Window` の親が `M_DD_Glass`、`MI_Glass02`・`MM_Main_Substance_Glass_DoorsNontransparent` の親が `M_DD_GlassSewerage`（後者は Blend が Opaque）になり、シェーダーのコンパイルが通ることを見る
  - 01 記録の `resolve_material` の材質の数（`substance` 69・…・`other` 13）を、前処理が出す新しい数に直す（`glass` 1・`glassmask` 1・`sewerglass` 2 が増え、`other` が 3 減るはず）
- [ ] 5. PIE で撮って確かめ、焼き込みと fps を見る
  - 撮る場所: Zone 1 の両開き扉（`BP_06_DoubleDoors` が 28 個以上。`hospital_entrance_walkway_doubledoor1/2` のガラス）・救急車の窓（`_Police_Window`）・ポスター枠（`MI_Glass02`、Zone 1 に 222）・タイル（`_DoorsNontransparent`、不透明に戻ったか）・**天井灯 `M_07_CeilingLamp_02`**（ブレンドの読みの直しで Masked → Opaque になる。本家どおりだが見た目が変わる所なので見る）

## 次にやること

ステップ 4。`python Tools/dd/prepare_stage.py` を走らせてから、エディタで `dd_stage.refresh_settings()`。

## 決定事項

- 2026-09-22: **`MM_Main_Substance_Glass_DoorsNontransparent` は本家では不透明**。`base_property_overrides` が `bOverride_BlendMode: true` なのに `BlendMode` を持たない＝列挙の既定値 `BLEND_Opaque`（書き出しは既定値の項目を省く）。前処理がこれを取りこぼして半透明のまま扱っていた。**この直しはこの項目に必須**: 直さないとタイルが Fresnel の不透明度（最低 0.35）で透ける。同じ形の材質はステージにもう 1 つ（`M_07_CeilingLamp_02`。Masked → Opaque になる）。
- 2026-09-22: **`M_06_Hospital_ExteriorGlass_01`・`_02` は直さない** — 本家でも `MM_Main_Substance`（不透明）の子で、中身は壁のテクスチャ。本家も透けないので本作と同じ。完了の条件 (3) の「外のガラス」は、この事実の確認をもって済ませる。
- 2026-09-22: **Nanite の扱いは今のままでよい** — `translucent_meshes` は材質の `blend` で見ていて、ガラスは書き出しの時点で半透明。マスターを替えても対象は変わらない。ステップ 5 では fps と焼き込みの結果だけ見る。
- 2026-09-22: **`MASTER_VERSION` は上げない**。新しいマスターは `_material()` が作って建てるだけ。上げると既存のマスター 5 つが全部再コンパイルになるだけで得がない。親の張り替えは `refresh_settings` の「親が `master_of(m)` と違うインスタンスは作り直す」の道が既にやる。
- 2026-09-22: **Zone 1 の扉のガラスは `MM_Main_Substance_Glass_Doors`**。`stage_ue.json` の配置では Zone 2 の 2 枚しか出てこないが、Zone 1 の扉は `BP_06_DoubleDoors_C` のアクタ（28 個以上）で置かれ、その形 `hospital_entrance_walkway_doubledoor1/2` がこのガラスを持つ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ステップ 4 の前処理（`python Tools/dd/prepare_stage.py`、数分）とエディタでのマスターの作成（シェーダーのコンパイル）は時間がかかる。前処理の出力は `Intermediate/Pipeline/dd/stage_ue.json`（完了の確かめ方: `materials` の `MM_Main_Substance_Glass_Doors` の `master` が `glassmask`、`MI_Glass02` が `sewerglass`、`_DoorsNontransparent` の `blend` が `BLEND_Opaque`）。
- 逆アセンブルの出力は `Intermediate/Pipeline/dd/shaders/<マスター名>/` に残っている（git の対象外）。
- `MaterialExpressionClamp`・`Saturate` の最初の入力には名前が無い（`"Input"` では `connect_material_expressions` が偽を返す）。空文字でつなぐ（`_Graph.clamp`）。

## 検証

- check_records: OK（20 件。01-stage-pipeline.md を更新してハッシュを合わせた）
- C++ ビルド: 不要（Python の前処理だけ）
- エディタでの確認: `_build_glass`・`_build_glass_sewer` を一時マテリアル（`unreal.new_object`、ディスクには書かない）で空打ちし、式 17 / 15 ノードが例外なく組めることを確かめた。マスターのアセットとインスタンスの作り直しはステップ 4。
