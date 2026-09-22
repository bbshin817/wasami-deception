---
title: ガラスが透けない（ガラスのマスターを原作のシェーダーから組む）（作業一覧の項目 46）
status: 進行中
branch: main
base: 9a7045b
started: 2026-09-22 18:49
updated: 2026-09-22 18:49
---

# ガラスが透けない（作業一覧の項目 46）

## 依頼

ゲームレビュアーの指摘（Medium）「本家であれば透過しているはずの、Zone1 の扉のガラスなどが燻んでいる」。
本家の扉のガラス `MM_Main_Substance_Glass_Doors` は `MM_Main_Substance_Glass_ColorMask`（`BLEND_Translucent`）の子で、
`MaskedColor` と `RefractionDepthBias` を持ち、インスタンスで `BLEND_AlphaComposite` に上書きしている。
本作はガラスの系列も一律に `M_DD_Substance` の子にしていて、この材質の不透明度は「アルベドの α × `Opacity Override`（既定 1）」なので、
α が 1 のテクスチャ（扉のガラスは `T_White`）では透けず、くすんだ板に見える。

完了の条件（`.claude/roadmap.md` の項目 46）:
(1) `MM_Main_Substance_Glass` と `_ColorMask` のコンパイル済みのシェーダーを読み、式どおりのマスターを作る（項目 31 の `M_DD_Metal`・`M_DD_SubstanceFresnel` と同じやり方）。
(2) 前処理 `MASTERS` の振り分けにガラスを足し、ガラスのインスタンスを新しいマスターの子にする。
(3) Zone 1 の扉のガラスと外のガラスを本家の絵と見比べ、透け方と映り込みが同じに見えることを確かめる。半透明は Nanite を切る対象なので、焼き込みと fps も見る。

## 計画

- [ ] 1. 原作のガラスのマスター 2 つのコンパイル済みのシェーダーを読み、式を書き出して、作るマスターの形を決める ← 作業中
  - `python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Substance_Glass."`（`_ColorMask.` も。ただの `MM_Main_Substance_Glass` は 2 つに当たるので末尾に `.` を付ける）
  - 決めること: マスターを 1 つ（静的切り替え `UseMaskColor` で `_ColorMask` を兼ねる）にするか 2 つに分けるか / 使うパラメータ（`MaskedColor`・`RefractionDepthBias`・`Albedo`）/ 既定の BlendMode（親は `BLEND_Translucent`、インスタンスが `BLEND_AlphaComposite` に上書き）/ 屈折・フレネルの式の有無
  - 決めたことは下の「決定事項」に書く（記録だけ。コードは次のステップ）
- [ ] 2. `dd_stage.py` にガラスのマスターを組み、前処理の振り分けを足す
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_stage.py`（`_build_glass`・`MASTER_OF`・`TEX_PARAMS`・`SCALARS`・`COLORS`・`ensure_masters`・`MASTER_VERSION` を上げる）、`Content/Python/wasami_tools/pipeline/paths.py`（`MASTER_GLASS`）、`Tools/dd/prepare_stage.py`（`MASTERS` に `MM_Main_Substance_Glass` → `glass`）
- [ ] 3. 前処理をやり直し、エディタでマスターとインスタンスを作り直して、ガラスが新しい親になったことを確かめる
  - `python Tools/dd/prepare_stage.py` → エディタで `ensure_masters` の道（`refresh`）を走らせる。`MM_Main_Substance_Glass_Doors`・`_Police_Window` の親が `/Game/Pipeline/Materials/M_DD_Glass` になり、シェーダーのコンパイルが通ることを見る
- [ ] 4. Zone 1 の扉のガラスを PIE で撮って本家と見比べ、焼き込みと fps を見る
  - 撮る場所は Zone 1 の両開き扉（`AWasamiDoubleDoors` のガラス）と外のガラス。本家は `.claude/guides/observation.md` の台本に従う（絵が要るときだけ）

## 次にやること

ステップ 1。`python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Substance_Glass."` と `"MasterMaterials/MM_Main_Substance_Glass_ColorMask."` を読み、
式（アルベド → 不透明度の作り方、`MaskedColor` の使われ方、屈折・フレネル）を書き出して「決定事項」にまとめ、この記録をコミットする。

## 決定事項

- 2026-09-22: **直す対象は 2 つ**（`Intermediate/Pipeline/dd/stage_ue.json` を数えた）。`MM_Main_Substance_Glass_Doors`（`master: glassmask`・`BLEND_AlphaComposite`・`MaskedColor` 青緑・アルベドは `T_White`）と `MM_Main_Substance_Glass_Police_Window`（root が `MM_Main_Substance_Glass`・`master: other`・`BLEND_Translucent`）。前者が指摘の扉のガラス。
- 2026-09-22: **`M_06_Hospital_ExteriorGlass_01`・`_02` は直さない** — 本家でも `MM_Main_Substance`（`blend_mode` の指定なし＝不透明）の子で、中身は壁のテクスチャ（`hospital_wall_ext_0*_D/N/S`）。本家も透けないので本作と同じ。完了の条件 (3) の「外のガラス」は、この事実の確認をもって済ませる。
- 2026-09-22: **Sewerage の `M_Glass` 系（`MM_Main_Substance_Glass_DoorsNontransparent`・`MI_Glass02`。`master: other` → `M_DD_Substance`）は今回の範囲外**。項目 46 の根拠が名指しするのは `MM_Main_Substance_Glass` と `_ColorMask` の 2 つで、`M_Glass` は別のサードパーティのマスター（`Dirt Mask`・`Fresnel Power`・`HDR str`・`Max Opacity` を持つ）。名前のとおり「透けない方」の扉のガラスなので指摘には当たらない。ステップ 1 で式を読んだついでに安い作りで済むと分かれば足す（外れれば項目 28 の後回しの一覧に 1 行）。
- 2026-09-22: **Nanite の扱いは今のままでよい** — `dd_stage.translucent_meshes` は材質の `blend` が半透明かどうかで見ていて（`master` の種類では見ていない）、ガラスの 2 つは書き出しの時点で `BLEND_AlphaComposite`・`BLEND_Translucent` を持つので、マスターを替えても Nanite の対象は変わらない。ステップ 4 では fps と焼き込みの結果だけ見る。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 3 で前処理（`python Tools/dd/prepare_stage.py`、数分）とエディタでのマスターの作り直し（シェーダーのコンパイル）が入る。
- `MASTER_VERSION` を上げるとすべてのマスターが作り直され、インスタンスのシェーダーマップも張り直しになる（`dd_stage.refresh`）。ステップ 2 で上げたら、ステップ 3 を終えるまでステージの見た目は当てにしない。

## 検証

- check_records: 未実行
- C++ ビルド: 不要（Python の前処理だけ）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
