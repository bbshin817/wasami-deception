---
title: ガラスが透けない（ガラスのマスターを原作のシェーダーから組む）（作業一覧の項目 46）
status: 進行中
branch: main
base: 9a7045b
started: 2026-09-22 18:49
updated: 2026-09-22 20:10
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

- [x] 1. 原作のガラスのマスター 2 つ（`MM_Main_Substance_Glass`・`_ColorMask`）のシェーダーを読み、式を書き出した（「決定事項」の 1〜3）。
- [x] 2. Sewerage の `M_Glass` のシェーダーを読み、式を書き出した。安く組めるので範囲に入れる（「決定事項」の 6〜8）。
- [ ] 3. `dd_stage.py` にガラスのマスター 2 つを組み、前処理の振り分けと Blend の上書きの読みを直す ← 次
  - `Content/Python/wasami_tools/pipeline/paths.py`: `MASTER_GLASS = "/Game/Pipeline/Materials/M_DD_Glass"`、`MASTER_GLASS_SEWER = "/Game/Pipeline/Materials/M_DD_GlassSewerage"`
  - `Content/Python/wasami_tools/pipeline/dd_stage.py`: `_build_glass`・`_build_glass_sewer` を足し、`MASTER_OF`（`glass`・`glassmask` → `MASTER_GLASS`、`sewerglass` → `MASTER_GLASS_SEWER`）・`TEX_PARAM`（前者 `{"albedo": "Albedo"}`、後者 `{"normal": "Normal"}`）・`SCALARS`（前者 `()`、後者 `("Fresnel Power", "Max Opacity", "Min Opacity", "Metallic", "spec", "Roughness")`）・`VECTORS`（前者 `("MaskedColor",)`、後者 `("diffuse color", "diffuse2 color")`）・`ensure_masters` の一覧に足す。`make_material` の静的切り替え `UseMaskColor` を立てる条件に `master_path == paths.MASTER_GLASS`（`m["master"] == "glassmask"` のとき真）を足す
  - `Tools/dd/prepare_stage.py`: `MASTERS` に `/Game/Materials/MasterMaterials/MM_Main_Substance_Glass` → `glass`、`/Game/ThirdParty/Sewerage/Materials/BaseMaterial/M_Glass` → `sewerglass`。既にある `MM_Main_Substance_Glass_ColorMask` → `glassmask` はそのまま。**`bOverride_BlendMode` が真で `BlendMode` が書かれていないときは `BLEND_Opaque`**（決定事項の 8）
- [ ] 4. 前処理をやり直し、エディタでマスターとインスタンスを作り直す
  - `python Tools/dd/prepare_stage.py` → エディタで `dd_stage.refresh_settings()`。`MM_Main_Substance_Glass_Doors`・`_Police_Window` の親が `M_DD_Glass`、`MI_Glass02`・`MM_Main_Substance_Glass_DoorsNontransparent` の親が `M_DD_GlassSewerage`（後者は Blend が Opaque）になり、シェーダーのコンパイルが通ることを見る
- [ ] 5. PIE で撮って確かめ、焼き込みと fps を見る
  - 撮る場所: Zone 1 の両開き扉（`BP_06_DoubleDoors` が 28 個以上。`hospital_entrance_walkway_doubledoor1/2` のガラス）・救急車の窓（`_Police_Window`）・ポスター枠（`MI_Glass02`、Zone 1 に 222）・タイル（`_DoorsNontransparent`、不透明に戻ったか）

## 次にやること

ステップ 3。上の変更予定どおりに `paths.py`・`dd_stage.py`・`prepare_stage.py` を直す。
式は「決定事項」の 1〜3（`M_DD_Glass`）と 6〜7（`M_DD_GlassSewerage`）のとおり。手本は `_build_substance_fresnel`（Fresnel ノードの組み方）と `_build_substance`（`UseMaskColor` の切り替え）。

## 決定事項

- 2026-09-22: **`MM_Main_Substance_Glass`・`_ColorMask` の式**（`Tools/dd/cooked_shaders.py` の逆アセンブルから。`Intermediate/Pipeline/dd/shaders/MM_Main_Substance_Glass{,_ColorMask}/`）。共通の部分:
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
- 2026-09-22: **`M_06_Hospital_ExteriorGlass_01`・`_02` は直さない** — 本家でも `MM_Main_Substance`（`blend_mode` の指定なし＝不透明）の子で、中身は壁のテクスチャ。本家も透けないので本作と同じ。完了の条件 (3) の「外のガラス」は、この事実の確認をもって済ませる。
- 2026-09-22: **Nanite の扱いは今のままでよい** — `dd_stage.translucent_meshes` は材質の `blend` が半透明かどうかで見ていて（`master` の種類では見ていない）、ガラスは書き出しの時点で半透明なので、マスターを替えても Nanite の対象は変わらない。ステップ 5 では fps と焼き込みの結果だけ見る。
- 2026-09-22: **Sewerage の `M_Glass` の式**（`Intermediate/Pipeline/dd/shaders/M_Glass/`。基本パスの PS は `56_ps_5_0.txt`、材質の定数バッファはこのシェーダーでは `cb4`。パラメータと番地の対応は `M_Glass_table.txt` の先頭）:
  - `F = Fresnel(ExponentIn = Fresnel Power, BaseReflectFractionIn = 0.04)`（PS: `pow(1 − max(N·V, 0), cb4[6].y)` → `× 0.96 + 0.04`）。`Fresnel Power` はスカラーパラメータ（既定 1.5）。
  - **`BaseColor = Lerp(diffuse color, diffuse2 color, F)`**（`mad_sat r5.xyz, F, cb4[4] − cb4[5], cb4[5]`）。
  - **`Opacity = Lerp(Min Opacity, Max Opacity, F)`**（`mad_sat r6.w, F, cb4[7].y − cb4[7].z, cb4[7].z` → `o0.w`）。既定は 0.35 → 0.9。`MI_Glass03` の `Max Opacity` 8.0 は engine 側の saturate で頭打ちになる前提の値。
  - `Metallic` = パラメータ `Metallic`（既定 0.1）、`Specular` = パラメータ `spec`（既定 30 → engine が saturate して 1）、`Roughness` = パラメータ `Roughness`（既定 0.1）。いずれも `M_DD_Metal` のときと同じ形（鏡面色 = `lerp(0.08 × Specular, BaseColor, Metallic)`、拡散色 = `BaseColor × (1 − Metallic)`）で確かめた。
  - `Normal` = テクスチャパラメータ `Normal`（UV0。既定 `T_base_flat_n` = 平ら。本作のステージのインスタンスは上書きしていないので実質フラット）。
  - **`Dirt Mask`・`Dirt str`・`Dirt color`・`HDR`・`HDR str` はコンパイル済みのシェーダーに無い**（シェーダーマップの uniform の一覧が `diffuse color`・`diffuse2 color`・`Fresnel Power`・`Metallic`・`spec`・`Roughness`・`Max Opacity`・`Min Opacity`・`RefractionDepthBias`・`Normal` だけ）。本家でも効いていないので作らず、読み飛ばす。
  - **屈折は無い**（この材質のシェーダーマップに歪みパスのシェーダーが 1 つも無く、`0.00023` を含むシェーダーも無い）。`RefractionDepthBias` も今までどおり読み飛ばす。
  - 半透明のライティングは `MM_Main_Substance_Glass` と同じ **Surface TranslucencyVolume**（基本パスの PS が texture3d と反射キャプチャの両方を読む）。
- 2026-09-22: **安く組めるので `M_DD_GlassSewerage` として組む**（ステップ 2 の判断。範囲外にはしない）。Fresnel 1 つと Lerp 2 つ、テクスチャは法線 1 枚だけで、`M_DD_SubstanceFresnel` より軽い。対象は Zone 1 のポスター枠 222・人工呼吸器 2（`MI_Glass02`）とタイル 3 枚 × 2 スロット（`MM_Main_Substance_Glass_DoorsNontransparent`）。
- 2026-09-22: **`MM_Main_Substance_Glass_DoorsNontransparent` は本家では不透明**。`base_property_overrides` が `bOverride_BlendMode: true` なのに `BlendMode` を持たない＝列挙の既定値 `BLEND_Opaque`（書き出しは既定値の項目を省く。`_materials.json` に同じ形が 20 件あり、うちステージに乗るのはこの材質と `/Game/Materials/07_FunPlace/M_07_CeilingLamp_02` の 2 件）。`prepare_stage.py` の `if over.get("bOverride_BlendMode") and over.get("BlendMode")` がこれを取りこぼして半透明のまま扱っていた。**この直しはこの項目に必須**: 直さないと、タイルが新しいガラスのマスターの Fresnel の不透明度（最低 0.35）で透けてしまう。
- 2026-09-22: **`MASTER_VERSION` は上げない**。新しいマスターは存在しないので `_material()` が作って建てるだけで、版は関わらない。上げると既存のマスター 5 つが全部作り直し＋再コンパイルになるだけで得がない。親の張り替えは `refresh_settings` の「親が `master_of(m)` と違うインスタンスは作り直す」の道が既にやってくれる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 4 で前処理（`python Tools/dd/prepare_stage.py`、数分）とエディタでのマスターの作成（シェーダーのコンパイル）が入る。
- 逆アセンブルの出力は `Intermediate/Pipeline/dd/shaders/<マスター名>/` に残っている（git の対象外）。`M_Glass` は基本パスの PS が `56_ps_5_0.txt`、パラメータと定数バッファの対応は `M_Glass_table.txt` の先頭。

## 検証

- check_records: 未実行
- C++ ビルド: 不要（Python の前処理だけ）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
