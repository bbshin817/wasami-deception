---
title: ステージを CC2 から本家の病院（06_Hospital Zone 1・Zone 2）へ差し替える
status: 進行中
branch: feature/hospital-stage
base: f2ad354
started: 2026-09-16 09:01
updated: 2026-09-16 09:21
---

# ステージを CC2 から本家の病院（06_Hospital Zone 1・Zone 2）へ差し替える

## 依頼

「方針を変えます。ステージは CC2 でなく、pak_reference_2 の病院ステージ Zone1 を採用してください。」（2026-09-16）

確認したうえでの決定（同日、下の「決定事項」）:
- 採用する範囲は **Zone 1 + Zone 2**（`06_Hospital_Zone_01`・`06_Hospital_Zone_02`）。入口 `06_Hospital` とボス戦は採らない。
- いまの CC2 のステージ（アセットとコード）は**全部消す**。参照データ `cc2_reference/` 自体は消さない。
- ゲームの流れと値は**病院の原作どおり**にする。

## 計画

- [x] 1. 作業ブランチ `feature/hospital-stage` を作り、この記録を作る
- [x] 2. CC2 の撤去
  - 消したアセット: `/Game/CC2`（511 アセット・733 MB）、`/Game/Stage/Maps/L_Zone1`、`/Game/Pipeline/Materials/M_CC2_Standard`、`/Game/Pipeline/Interchange/PL_CC2_StaticMesh`。空のレベル `/Game/Stage/Maps/L_Hospital_Zone1` を作って開いた（`/Game/Pipeline/Textures/T_Default_Masks` は病院でも使うので残した）
  - 消したコード: `Tools/cc2/prepare_stage.py`、`pipeline/cc2_assets.py`、`pipeline/cc2_level.py`、`toolsets/stage.py`（`WasamiStageTools`）
  - 直したもの: `pipeline/paths.py`（CC2 の定数を外し `DD_PAK2` を追加）、`wasami_tools/__init__.py`、`Config/DefaultEngine.ini`（既定マップ → `L_Hospital_Zone1`）、`.gitignore`、`CLAUDE.md`、ガイド 6 件、実装記録 00・01・`_index`、`handover.md`
- [x] 3. 前処理 `Tools/dd/prepare_stage.py` → `Intermediate/Pipeline/dd/stage_ue.json`（3.7 MB、problems 0）。メッシュ 66（うちエンジンの Plane・Cube が 2）・テクスチャ 282・マテリアル 143。Zone1: 配置 923・灯 1,120・反射キャプチャ 10・アクタ 873、Zone2: 配置 819・灯 751・反射キャプチャ 1・ポストプロセス 1・アクタ 836
- [ ] 4. 取り込み `Content/Python/wasami_tools/pipeline/dd_stage.py` と `toolsets/stage.py`: メッシュ 66・テクスチャ 282・マテリアル 143 を `/Game/DD/…` に。マスターマテリアルは `M_DD_Substance`（発光・マスクは静的スイッチ）・`M_DD_Decal`（メッシュデカール）・`M_DD_Unlit` の 3 つ ← 作業中
- [ ] 5. 組み立て `pipeline/dd_level.py`: `/Game/Stage/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2`（配置・灯・反射キャプチャ・霧・スカイライト・ポストプロセス・プレイヤースタート）
- [ ] 6. PIE で歩いて当たりと見た目を確かめ、性能を測る。実装記録（01）とガイド・CLAUDE.md を病院に合わせて直し、`check_records.py --update` を通す

## 次にやること

ステップ 4。エディタ側の取り込み `Content/Python/wasami_tools/pipeline/dd_stage.py` と、それを呼ぶ `toolsets/stage.py`（`WasamiStageTools` を病院用に作り直す）を書く。`stage_ue.json` を読み、glTF（`pak_reference_2/_meshes_gltf/**.gltf`）と PNG を直接取り込み、マテリアルインスタンスを作る。量が多いので CC2 と同じく `max_items` で区切って `remaining` が 0 になるまで呼ぶ。新しいツールセットのクラスは `reload_module` では登録されないので、`Tools/ue_remote.py` から明示的に登録するかエディタを開き直す。

## 決定事項

- 2026-09-16: 採用範囲は Zone 1 + Zone 2 — ユーザーの回答。Torment Therapy の 2 ゾーンを通しで遊べるようにする。入口（`06_Hospital`）とボス戦（`06_Hospital_Bossfight`）は今回は採らない。
- 2026-09-16: CC2 は全部消す — ユーザーの回答。`/Game/CC2`・`L_Zone1`・`Tools/cc2`・`cc2_assets.py`・`cc2_level.py` を削除する。CC2 の実装は git 履歴（`f2ad354` 以前）に残るので、必要なら取り出せる。参照データ `cc2_reference/` は消さない。
- 2026-09-16: ゲームの流れと値は病院の原作どおり — ユーザーの回答。`.claude/guides/original-fidelity.md` の「ステージだけは CC2」という例外が無くなり、**すべて本家基準に一本化**される。病院は `pak_reference`（UE 4.21）に無いので、根拠は `pak_reference_2`（UE 4.24）。敵はワサミ、ライフ 3 など本作独自の決定は維持。

## 再開時の注意

### 調べた事実（2026-09-16、`pak_reference_2` から）

`06_Hospital_Zone_01`（exports 7,667）:
- レベルストリーミングは無く**自己完結**。レベル BP の最後に `OpenLevel('06_Hospital_Zone_02')`。PlayerStart 4（`04_Start`・`05_Start`・`06_Start`・`PlayerStart_1`）
- 地形は大きなタイル 5 枚（`hospital_zone_01_tiles_tile_01`/`_02`/`_03`/`_parking`/`_tunnel`、18.4 + 17.5 + 19.9 + 0.9 + 7.9 MB）＋ 小物 31 種。配置 923・灯 1,121（点 1,103・スポット 15・方向 1・矩形 1・スカイ 1）・パーティクル 390・音 43
- ゲームの部品: `BP_Shard_C` **337**、`BP_ZoneShardChecker_C` 1、`BP_ZoneBarrier_C` 1、`BP_SpeedBarrier_C` 4、`BP_06_DoubleDoors_C` 62、`BP_06_Defib_C` 23、`BP_PowerOrbSpawnPoint_C` 11、`BP_BonusShardSpawnPoint_C` 10、`BP_PowerOrb_C` 1、`BP_BonusShard_C` 1、`BP_Power_Teleport_Zone_C` 2（メッシュ `hospital_zone_01_teleport`）、`BP_06_GarageLift_Zone1_Special_C` 1（Zone 2 への出口）、`BP_06_Hospital_DoorBreak_C` 1、`BP_06_MusicPlayer_C` 1、`BP_MapTexture_C` 1、`BP_TriggerBox_Base_C` 6、`NavMeshBoundsVolume` 2、`BlockingVolume` 7、`AudioVolume` 2、反射キャプチャ 10、`ExponentialHeightFog` 1、`SkyLight` 1。**PostProcessVolume は無い**
- 敵は `BP_06_ReaperNurse`・`BP_06_ReaperNurse_06_Chase`（レベル BP が `Spawn Nurses` / `Spawn Nurses_06` で出す）

`06_Hospital_Zone_02`（exports 7,165）: 配置 819・灯 752・パーティクル 381・音 146・`PostProcessVolume` 1・`NavMeshBoundsVolume` 29。タイルが重い（`hospital_zone_02_tiles_tile_02` 114 MB、`_tile_03` 106 MB）

Zone 1 + Zone 2 の合計（重複を除く）: **メッシュ 67（glTF 423 MB）・マテリアル 146・テクスチャ 291（PNG 666 MB）**

マテリアルの親（＝写すべきマスターマテリアル）:

| 親 | 数 | 引数 |
| --- | ---: | --- |
| `/Game/Materials/MasterMaterials/MM_Main_Substance` | 59 | `Albedo`・`Normal`・`Packed`（R=AO, G=Roughness, B=Metallic）、スカラ `Normal Flatness`・`Roughness Power`・`RefractionDepthBias` |
| `/Game/Materials/01_Hotel/M_01_Hotel_Decals` | 31 | `Texture` |
| `MM_Main_Substance_Emissive` | 25 | 上 + `Emissive`、`Emissive Intensity`・`Emissive Multiplier`、`Color Multiplier` |
| `MM_Main_Substance_AlphaColorMask` | 10 | 上 + `Mask Color` |
| `MM_Lit` | 5 | `Light Color`・`Light Multiplier` |
| その他 7 系統 | 11 | ガラス・車・第三者製 |

親チェーンの深さは 140 個が 2 段（インスタンス → マスター）。マスターの引数（`_assets/.../MasterMaterials/*.json` の式から確定）:

| マスター | 既定・引数 |
| --- | --- |
| `MM_Main_Substance` | `Albedo`（sRGB）・`Normal`（`SAMPLERTYPE_Normal`）・`Packed`（`SAMPLERTYPE_LinearColor`）、`Metallic Power` 1.0・`Roughness Power` 1.0・`Normal Flatness`（既定なし＝0）。`bUsedWithSkeletalMesh` |
| `MM_Main_Substance_Emissive` | 上 + `Emissive` テクスチャ・`Emissive Intensity`・`Opacity Override`・`Emissive Color Multiplier`（1,1,1,1）。`BLEND_Masked` |
| `MM_Main_Substance_AlphaColorMask` | 上 + `Mask Color`（1,1,1,1） |
| `MM_Lit` | `MSM_Unlit`。`Light Multiplier` 1.0・`Light Color`（0,0,0,1）・コレクション引数 `All Lights` |
| `M_01_Hotel_Decals` | `MD_DeferredDecal`・`BLEND_Translucent`・`DBM_DBuffer_ColorRoughness`。`Texture`・`Roughness Power` 1.0・`Emissive Multiplier`・`Color Multiplier`・`Fade Length (S)` 500・`Fade Offset (S)` 500・静的ブール `Distance Fade?` |

**式の中身（`Roughness Power` などがどう効くか）は cook で消えている**（`MaterialExpressionMaterialFunctionCall` 1 つに集約）。作り直すマスターでは pow(値, Power)・法線の平坦化と解釈する（推定と明記して記録に書く）。

**BP の中の灯**: レベルの灯 1,873 のうち 1,251 は `light: {}` で、値は Blueprint のクラス既定にある。内訳は `BP_Shard_C` 679、`PointLight351_Tunnel_Blueprint_C` 190、`PointLight_Blueprint_C` 294、`PointLight_Blueprint_WallLight_C` 50、`PointLight_Blueprint_Ceiling_Z2_C` 34、`BP_06_sawTrap_short01_C` 22 ほか。`_assets/.../<BP>.json` の `LightComponent0` か `<名前>_GEN_VARIABLE` の `props` を使い、`RelativeLocation` 等があれば親のワールド変換に合成する（書き出しの `world` は BP 内の相対変換を含まないため）。

**デカールはメッシュデカール**: デカール材 31 種は 227 スロットすべてが `/Engine/BasicShapes/Plane` の `StaticMeshActor` に載っている（`DecalActor` も `DecalComponent` も無い）。UE の mesh decal（`MD_DeferredDecal` のマテリアルをスタティックメッシュに割り当てる）なので、そのまま再現できる。`M_01_Hotel_Decals` は `DBM_DBuffer_ColorRoughness`・`BLEND_Translucent`。

**`Normal Flatness` の扱いは未確定**: インスタンスは 1.2〜3.0 を入れるが、マスターの既定は 0 で、式は cook で消えている。0〜1 の lerp とも、XY の倍率とも読めて確定できない。**いまは適用しない**（WebGL 版も同じ判断: `.claude/references/webgl/implementation-records/13-asset-pipeline.md`）。`Roughness Power`・`Metallic Power` は既定 1.0 なので pow(値, Power) として実装する（推定）。見え方を原作の映像と比べる段で見直す。

**マスターごとの配置スロット数**: substance 1,459・other 491・emissive 345・decal 227・alphamask 185・lit 131・glassmask 2・translucent 1。`other` の 11 種（`M_06_Sky`・`M_TeleportZone`・`m_crystal`〈シャード〉・`M_06_Nurse_Items`・`M_06_ShadowPlane_Zone1_Tunnel`・ガラス 2 種・車 2 種・動画 1）は `M_DD_Substance` に持っているテクスチャだけ載せる形で代用する。

**テクスチャの設定**は `_textures.json` の `srgb` / `compression` / `lod_group` をそのまま使える（病院の 365 枚: DXT1 sRGB 139、BC5 法線 93〈`TC_Normalmap` + `TEXTUREGROUP_WorldNormalMap`〉、DXT1 リニア 84、DXT5 sRGB 45。2048² が 227 枚、4096² が 9 枚）。

### 気をつけること

- **灯の単位**: 書き出しに `IntensityUnits` が無い点光源が 1,296 個ある。UE 4.24 の `ULocalLightComponent.IntensityUnits` の既定は `Unitless`、UE5 の既定は `Candelas` なので、**書き出しに無いものは明示的に `Unitless` を入れる**（入れないと明るさが桁違いになる）。`Candelas` と明記されているのは点 528・スポット 30・矩形 4。`bUseInverseSquaredFalloff=false` が点 1・スポット 11 あり、その場合は `LightFalloffExponent` を使う
- メッシュは `_meshes_gltf/**.gltf` + `.bin`（CUE4Parse、LOD0 のみ、UV 8 組、頂点色つき、**マテリアル名はスロット名**）。`body_setup` はタイル 5 枚とも `false` なので当たりは描画メッシュ（complex as simple）
- 座標は UE の生値（cm・Z 上）。CC2 のような座標変換は要らない
- 取り込みは量が多い（PNG 666 MB）。CC2 と同じく 1 回の呼び出しで作る数を区切り、`remaining` が 0 になるまで繰り返す
- **MCP は未接続**（`mcp__unreal-mcp__*` がこのセッションに出ていない。`/mcp` で再接続しても出なかった）。エディタは起動中で、`python Tools/ue_remote.py -c "…"` は応答する。当面はリモート実行で進める
- エディタで開いているレベルは `L_Zone1`（消す対象）。消す前に別のレベルへ移る

### 使った調べもののスクリプト（一時ファイル、消えてよい）

`C:\Users\User\AppData\Local\Temp\claude\c--Users-User-Desktop-wasami-deception\c82238db-baab-462c-b82e-e8b780c914e9\scratchpad\` の `analyze_hospital.py`（規模）、`hospital_detail.py`（メッシュと配置）、`hospital_pipeline.py`（マテリアル・テクスチャ・灯）

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この作業では C++ を変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
