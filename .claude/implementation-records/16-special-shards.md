---
title: 特殊シャード（スタンオーブと赤いシャード）
sources:
  - Content/Python/wasami_tools/pipeline/dd_specials.py
updated: 2026-09-19
---

# 特殊シャード（スタンオーブと赤いシャード）

## 役割
本家の特殊シャード 2 種（最新版 `pak_reference_2` の `Blueprints/Main/BP_PowerOrb` = スタンオーブ、`BP_BonusShard` = 赤いシャード）。1 体ずつ置かれ、150 s ごとに 5 s 明滅して出現点を移る。オーブを取ると全敵が気絶し、赤いシャードを取ると 60 s 敵がタブレットの地図に出る（作業一覧の項目 10）。**作っている途中**: いまあるのは素材の取り込みだけ（本体・演出・配置は進捗記録 `20260919-special-shards` のステップ 2〜5）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_dd_specials()`（素材。`import_dd_shards` と `import_dd_gimmicks` の後。地図の印のマスターと粒子の材質を共有する）。

## 内部構造と処理の流れ
（本体のクラスはこれから）

## 作るアセット
`WasamiDDTools.import_dd_specials`（`pipeline/dd_specials.py` の `import_all`）が作る。すべて `pak_reference_2` から。

| パス | 中身 |
| --- | --- |
| `/Game/DD/Meshes/Shared/power_orb` | オーブの本体（半径 約 44 cm の球。スロット 1 に `m_crystal_Inst3`〈本家の既定〉）。`dd_assets.static_mesh`（Nanite なし） |
| `/Game/DD/Meshes/Ring_Assets/soul_shard` | 赤いシャードの本体（2.2 × 1.8 × 8.6 cm。本家は 20 倍で置く）。スロットの本家の既定 `m_crystal_Inst1` は作らない（赤いシャードが `m_crystal_Inst` を当てる） |
| `/Game/Pipeline/Materials/M_DD_Crystal` | 本家の `m_crystal` の推定（下の「結晶の材質」） |
| `/Game/DD/Materials/Fords_Materials/m_crystal`・`m_crystal_Inst3`・`m_crystal_Inst` | 原作のパスの推定のインスタンスと、その子のオーブ用（橙: `color1` (0.526, 0.094, 0.047)・`emissive_col` (0.896, 0.226, 0)・`Fresnel Setting` (5, 0.592, 0)・`emissive_entensity` 29.9・`env_cubemap` `DefaultTextureCube`）と赤いシャード用（赤: `color1` (0.531, 0.009, 0)・`emissive_col` (0.156, 0, 0.013)・`emissive_entensity` 29.9）。`color2` はどちらも黒 |
| `/Game/DD/Materials/Shared/M_PowerOrb` | オーブの地図の印。`dd_shards` の `M_DD_MapMark` のインスタンス、`Color` (1, 0.2903, 0) |
| `/Game/Pipeline/Materials/M_DD_MapMarkMasked` | 形で切り抜く地図の印の推定のマスター: `Color` をベースカラーと自己発光に、`Mask`（`T_EnemyTriangle`）の R をマスクに（Masked、しきい 0.3333） |
| `/Game/DD/Materials/Shared/M_Bonus_Shard`・`M_Enemy` | 赤いシャードと、赤いシャードが地図に出す敵の印。`M_DD_MapMarkMasked` のインスタンス、`Color` (1, 0, 0) |
| `/Game/DD/Textures/Shared/T_EnemyTriangle` | 三角の形（1024²、G8、リニア） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Appear`・`_Disappear`・`P_ky_flash_BonusOrb_Appear`・`_Disappear` | 出現点を移るときの消える・現れる閃光。`dd_particles` が書き出しから組む |
| `.../Particles/P_ky_impact`・`P_ky_impact1` | オーブ・赤いシャードを取ったときの閃光（`P_ky_impact2`・`3` と同じ材質 2 つ） |
| `.../Textures/T_ky_flash01_4x4`・`T_ky_lensFlare01`・`T_ky_maskRGB3` | 閃光のテクスチャ（原作の設定のまま。`T_ky_maskRGB3` は AdvancedMagicFX13 のもの。除細動器の AdvancedMagicFX09 のものとは別） |
| `.../Materials/MI_ky_flare01b_primitiveG`・`R` | 消える閃光の星。`dd_shards` の推定の `M_ky_flare01_primitive` のインスタンス（`alphaDensity` 1.3 / 1.8・`baseTex` `T_ky_flash01_4x4`・`selectCh` G / R） |
| `/Game/Pipeline/Materials/M_DD_KyPrimitiveColor`・`M_DD_KyLensFlare02`、`.../Materials/M_ky_primitiveColor`・`MI_ky_primitiveColor`・`M_ky_lensFlare02` | 消える閃光の残りの材質の推定（下の「閃光の材質」）。`MI_ky_primitiveColor` は `useHilight` 真と両面の上書き |
| `/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_`・`Bonus_Shard_Pickup_v1`・`Stun_Wave_Attack_New_04` | オーブの取得・赤いシャードの取得・取得の演出の波の音 |

使うが、ここでは作らないもの: `Soul_Shard_Pickup_v2_Cue`（`import_dd_shards`）、`BP_CameraShake_Streak`・`01_Hotel_Lobby_ElevatorShakeStop`・`M_05_Primal`・`T_VignetteNew`（`import_dd_powers`）、`helvetica-neue-bold_Font`（`import_dd_tablet`）。`PPP_Collect_Shard` はどちらの BP の流れも起こさないので作らない。

### 結晶の材質（`m_crystal`。グラフは cook で消えている）
書き出しに残るのは出力の一部（金属・スペキュラ・自己発光。法線は未接続）、パラメータ、Noise 2 つの FeatureLevelSwitch、`BoundingBoxBased_0-1_UVW`、Custom を通して読むキューブ。コンパイル済みのシェーダー（`Tools/dd/cooked_shaders.py "Fords_Materials/m_crystal."` の SM5 のベースパス）を読んで、式をそのまま組んだ（不透明・ライトあり）:
- 自己発光 = max(0, Noise(反射ベクトル × 0.75 + 時間 × `emissive_speed`。3D テクスチャのグラディエント・乱流・4 段・−0.5〜0.5) × `emissive_col` × `emissive_entensity` + Fresnel(指数 5、基底 0.04) × `Fresnel Setting` + `Additive Emissive`)
- t = Noise(ワールド位置 × `tile_ratio` − 時間 × `emissive_speed`。テクスチャのシンプレックス・乱流・4 段・0〜1) + バウンディングボックスの Z（0〜1）− 0.5
- ベースカラー = saturate(lerp(`color2`, `color1`, t) + `env_cubemap` を refract(−カメラ, 法線, 0.66) の向きで × 0.5)（屈折は Custom `return refract(-V, N, 0.66);`）
- 金属 = t × 0.5、スペキュラ = t、粗さ = `roughness`（0.01）
- 推定で外したもの: シェーダーは反射と屈折の向きを `distortion_normal`（UV × 0.1 で読む）で曲げるが、推定は頂点の法線で読む（`CRYSTAL_LEFT_OUT`）。本家の親の `env_cubemap` は既定が空なので、推定の親は `DefaultTextureCube` を既定にした（赤いシャードはこれを継ぐ）。

### 地図の印の色
3 つとも書き出しは `Constant3Vector` を値なしで持つだけだが、コンパイル済みのシェーダーに定数が残っていた: `M_PowerOrb` (1, 0.2903, 0)、`M_Bonus_Shard`・`M_Enemy` (1, 0, 0)（`T_EnemyTriangle` の 1 チャンネルを 0.3333 で切る）。ついでに `M_Shard` の定数は (0.482481, 0, 1) と分かった（`dd_shards` は画面の実測で (0.70, 0.0071, 1.0) に合わせている。06 記録）。新しい 3 つはシェーダーの定数のままにし、タブレットでの見え方は見比べていない（作業一覧の項目 28 の後回し）。

### 閃光の材質（推定。グラフは cook で消えている）
- `M_ky_primitiveColor`: 残る式は `hilightPower`・`hilightColor`・`MF_ky_addHilight` と、自己発光（A = Add、B = 粒子の色）と不透明度（A = Multiply、B = 粒子の α）の静的スイッチ `useHilight`。シェーダーでは、スイッチ偽は不透明度 = 粒子の α を深さで 100 かけて消す。真（`MI_ky_primitiveColor`）は `T_ky_maskRGB3` の G と B を UV × 0.2 に時間 × (0.1, −1)・(−0.2, −2) で流して読み、n1 × n2 × (n1 + n2) × 2500 × `hilightPower` × `hilightColor` の光り。推定は真の側で粒子の色にこれを足し（Add）、不透明度は両側とも深さで消す粒子の α（Multiply の相手は分からない）。
- `M_ky_lensFlare02`: 自己発光 = 粒子の色。不透明度 = saturate(lerp(`remap1`, `remap2`, s(時間, 2)) × G^`alphaDensity`)。G は `T_ky_lensFlare01` を lerp(`rotRemap1`, `rotRemap2`, s(時間, 0.5)) × 0.25 だけ中心で回した UV で読む。s(時間, p) = (sin(2π sin(2π 時間 / p)) + 1) / 2（周期 p の Sine を `Sine_Remapped` に通したもの）。粒子の α は入らない。

## 原作データの根拠
- 本体・部品・流れ: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_PowerOrb.json`・`BP_BonusShard.json`、`_bytecode/.../BP_PowerOrb.txt`・`BP_BonusShard.txt`（`python Tools/dd/bp_flow.py`）。取得の演出は `Blueprints/Main/Powers/BP_StunCollectEffect`・`BP_BonusShardCollectEffect`。
- メッシュ: `_meshes.json` の `/Game/Meshes/Shared/power_orb`・`/Game/Meshes/Ring_Assets/soul_shard`。
- 材質: `_assets/.../Materials/Fords_Materials/m_crystal*.json`、`Materials/Shared/M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`・`M_Shard.json`、`ThirdParty/AdvancedMagicFX13/Materials/*.json` と、それぞれのコンパイル済みシェーダー（`Tools/dd/cooked_shaders.py`）。
- WebGL 版: `.claude/references/webgl/implementation-records/08`（特殊シャード）・`10`（`UMG_VignetteSides`）・`11`（地図の印）。

## 依存関係
- `dd_assets`（音・テクスチャ・メッシュ・材質・インスタンス・推定の材質）、`dd_particles`（Cascade の粒子）、`dd_stage._Graph`（01 記録）
- `dd_shards` の `M_DD_MapMark` と閃光の材質（06 記録）、`dd_gimmicks` の `MI_ky_flare14R`（08 記録）
- エンジン: `MaterialExpressionNoise`・`MaterialExpressionCustom`・`MaterialExpressionFresnel`・`MaterialExpressionRotator`

## 既知の制約・注意点
- 結晶と閃光の材質・地図の印の色は推定で、本家と見比べていない（大目標 1・2 の決め方。作業一覧の項目 28 の後回しの一覧）。
- Zone 2 の `m_crystal_Inst2`（ステージの小物）はステージの組み立てが汎用の `M_DD_Substance` のインスタンスで作っていて、ここの `m_crystal` とは別。

## 変更履歴
- 2026-09-19: 初版（素材の取り込み `dd_specials.py` と `WasamiDDTools.import_dd_specials`。作業一覧の項目 10 のステップ 1）
