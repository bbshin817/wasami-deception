---
title: ガラスが透けない（ガラスのマスターを原作のシェーダーから組む）（作業一覧の項目 46）
status: 進行中
branch: main
base: 9a7045b
started: 2026-09-22 18:49
updated: 2026-09-22 19:40
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

- [x] 1. 原作のガラスのマスター 2 つのシェーダーを読み、式を書き出した（下の「決定事項」の 1〜3）。
- [ ] 2. Sewerage の `M_Glass` のコンパイル済みのシェーダーを読み、式を書き出す ← 次
  - `python Tools/dd/cooked_shaders.py "Sewerage/Materials/BaseMaterial/M_Glass."`（パスが当たらなければ `_materials.json` で確かめる）
  - パラメータは `Dirt Mask`・`Normal`・`HDR`（テクスチャ）、`Dirt str` 1.7・`Fresnel Power` 1.5・`HDR str` 1.0 / 0.2・`Max Opacity` 0.9（スカラー）、`diffuse color`・`diffuse2 color`・`Dirt color`（ベクトル）
  - **安く組めるなら**ステップ 3 でマスターをもう 1 つ作る。式が重い（HDR キューブの疑似反射など）なら、この系列は項目 28 の後回しの一覧に 1 行書いて範囲から外し、決定事項に理由を書く
- [ ] 3. `dd_stage.py` にガラスのマスターを組み、前処理の振り分けを足す
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_stage.py`（`_build_glass`・`MASTER_OF`・`TEX_PARAM`・`SCALARS`・`VECTORS`・`ensure_masters`・`make_material` の静的切り替え・`MASTER_VERSION` を上げる）、`Content/Python/wasami_tools/pipeline/paths.py`（`MASTER_GLASS`）、`Tools/dd/prepare_stage.py`（`MASTERS` に `MM_Main_Substance_Glass` → `glass`、ステップ 2 次第で `M_Glass` → `glassdirt`）
- [ ] 4. 前処理をやり直し、エディタでマスターとインスタンスを作り直して、ガラスが新しい親になったことを確かめる
  - `python Tools/dd/prepare_stage.py` → エディタで `ensure_masters` の道（`refresh`）を走らせる。`MM_Main_Substance_Glass_Doors`・`_Police_Window` の親が `/Game/Pipeline/Materials/M_DD_Glass` になり、シェーダーのコンパイルが通ることを見る
- [ ] 5. PIE で撮って確かめ、焼き込みと fps を見る
  - 撮る場所: Zone 1 の両開き扉（`BP_06_DoubleDoors` が 28 個以上。`hospital_entrance_walkway_doubledoor1/2` のガラス）と救急車の窓（`_Police_Window`）。ステップ 2 で `M_Glass` を入れたならポスター枠も

## 次にやること

ステップ 2。`python Tools/dd/cooked_shaders.py` で Sewerage の `M_Glass` のシェーダーを読み、式（不透明度の作り方・`Max Opacity`・`Fresnel Power`・`Dirt`・`HDR`）を書き出して「決定事項」に足す。
安く組めるかどうかを決めてから、ステップ 3 の変更予定を確定させる。

## 決定事項

- 2026-09-22: **原作のガラス 2 つの式**（`Tools/dd/cooked_shaders.py` の逆アセンブルから。`Intermediate/Pipeline/dd/shaders/MM_Main_Substance_Glass{,_ColorMask}/`）。共通の部分:
  - `Metallic` 0.1・`Specular` 1.0・`Roughness` 0.0・`Normal` は既定（法線マップは無い）。
    根拠: 基本パスの PS が拡散色 = BaseColor × 0.9、鏡面色 = 0.072 + 0.1 × BaseColor（= `lerp(0.08 × Specular, BaseColor, Metallic)`）、粗さは `View.RoughnessOverrideParameter.x` だけ（材質の項が 0 で消えている）。
  - `F = Fresnel(ExponentIn 1.5, BaseReflectFractionIn 0)` = `pow(1 − saturate(dot(N, V)), 1.5)`。
  - **不透明度 = `Lerp(0.008, 0.9, F)`**（PS は `mad opacity, F, 0.892, 0.008`。0.008 + 0.892 = 0.9 なので Fresnel ノード単体ではなく Lerp）。正面で 0.008、斜めで 0.9。**これが「透ける」の正体**。
  - **屈折 = `Lerp(1.05, 0.95, F)`、方式は Index of Refraction**（UE 5.8 の `DistortionCommon.ush` は `ViewNormal.xy × (IOR − 1)` → `× ViewSizeAndInvSize.xy × BufferSizeAndInvSize.zw` →小さすぎたら clip → `× DistortionParams.zw × (0.00023, −0.00023) × FovFix`。4.21 の逆アセンブルと係数 0.00023 まで一致するので、5.8 でそのまま同じ絵になる）。**Pixel Normal Offset ではない**（5.8 のそれは頂点法線と画素法線の差を使うので、法線マップの無いこのガラスでは屈折が 0 になる）。
  - `RefractionDepthBias` は材質の**プロパティ**（UE が同名の ScalarParameter にコンパイルしてインスタンスから上書きできるようにしている）。インスタンスは全部 0.0 = 既定なので、`SCALARS` に足さず今までどおり読み飛ばしてよい。
  - 半透明のライティングは **Surface TranslucencyVolume**（`TLM_SURFACE`）。PS が半透明ボリューム（texture3d）の SH を読みつつ、反射キャプチャと `EnvBRDFApprox` も使っているため。
- 2026-09-22: **2 つの違いは BaseColor だけ**。`MM_Main_Substance_Glass` は定数 `(0.739583, 0.947552, 1.0)`（テクスチャパラメータを 1 つも持たない）。`_ColorMask` は `saturate(Lerp(Albedo.rgb × MaskedColor.rgb, Albedo.rgb, 1 − Albedo.a))` で、`MaskedColor` の既定は `(0.156516, 0.786076, 0.854167)`、`Albedo` はテクスチャパラメータ。
- 2026-09-22: **マスターは 1 つ `M_DD_Glass` にまとめる**。静的切り替え `UseMaskColor`（既定 偽 = 無地のガラス、`Constant3Vector (0.739583, 0.947552, 1.0)`）で `_ColorMask` の道と選ぶ。`_build_substance` が `UseMaskColor` でやっているのと同じ作りで、`make_material` の切り替えを立てる条件（今は `master_path == MASTER_SUBSTANCE` のときだけ）に新しいマスターを足す。`MaskedColor` の既定は原作の `_ColorMask` のもの（`0.156516, 0.786076, 0.854167`）にする。
- 2026-09-22: **Zone 1 の扉のガラスは `MM_Main_Substance_Glass_Doors` で合っている**。`stage_ue.json` の配置では Zone 2 の 2 枚しか出てこないが、Zone 1 の扉は `BP_06_DoubleDoors_C` のアクタ（28 個以上）で置かれ、その形は `dd_level.DOUBLE_DOOR_MESHES` = `hospital_entrance_walkway_doubledoor1/2`＝このガラスを持つメッシュ。`_Police_Window` は救急車（Zone 1 に 5・Zone 2 に 17）。
- 2026-09-22: **Sewerage の `M_Glass` 系を範囲に入れる**（前の「範囲外」の決定を取り消す）。取り消す理由は配置を数えた結果で、**Zone 1 のポスター枠 222 個・人工呼吸器 2 個（`MI_Glass02`）とタイル 3 枚 × 2 スロット（`MM_Main_Substance_Glass_DoorsNontransparent`）がこの系列**だから。今はどちらも `master: other` → `M_DD_Substance` で、アルベドのテクスチャが無いので白の不透明な板になっている（`BLEND_Translucent` でも α = 1）。指摘の「Zone1 の扉のガラス**など**」に当たる見込みが高い。ただし別のサードパーティのマスターなので、式が重ければステップ 2 の判断で外す。
- 2026-09-22: **`M_06_Hospital_ExteriorGlass_01`・`_02` は直さない** — 本家でも `MM_Main_Substance`（`blend_mode` の指定なし＝不透明）の子で、中身は壁のテクスチャ。本家も透けないので本作と同じ。完了の条件 (3) の「外のガラス」は、この事実の確認をもって済ませる。
- 2026-09-22: **Nanite の扱いは今のままでよい** — `dd_stage.translucent_meshes` は材質の `blend` が半透明かどうかで見ていて（`master` の種類では見ていない）、ガラスは書き出しの時点で半透明なので、マスターを替えても Nanite の対象は変わらない。ステップ 5 では fps と焼き込みの結果だけ見る。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 4 で前処理（`python Tools/dd/prepare_stage.py`、数分）とエディタでのマスターの作り直し（シェーダーのコンパイル）が入る。
- `MASTER_VERSION` を上げるとすべてのマスターが作り直され、インスタンスのシェーダーマップも張り直しになる（`dd_stage.refresh`）。ステップ 3 で上げたら、ステップ 4 を終えるまでステージの見た目は当てにしない。
- 逆アセンブルの出力は `Intermediate/Pipeline/dd/shaders/<マスター名>/` に残っている（git の対象外）。基本パスの PS で一番小さいもの（`28_ps_5_0.txt`）が読みやすく、屈折は `30_ps_5_0.txt`（歪みパス）にある。

## 検証

- check_records: 未実行
- C++ ビルド: 不要（Python の前処理だけ）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
