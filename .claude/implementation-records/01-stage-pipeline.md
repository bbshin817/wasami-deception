---
title: 取り込みの仕組み（前処理・ツールセット・ステージの組み立て）
sources:
  - Tools/cc2/prepare_stage.py
  - Tools/ue_remote.py
  - Tools/editor_cycle.py
  - Content/Python/init_unreal.py
  - Content/Python/wasami_tools/__init__.py
  - Content/Python/wasami_tools/toolsets/__init__.py
  - Content/Python/wasami_tools/toolsets/stage.py
  - Content/Python/wasami_tools/toolsets/dd.py
  - Content/Python/wasami_tools/toolsets/dev.py
  - Content/Python/wasami_tools/pipeline/__init__.py
  - Content/Python/wasami_tools/pipeline/paths.py
  - Content/Python/wasami_tools/pipeline/ue_props.py
  - Content/Python/wasami_tools/pipeline/cc2_assets.py
  - Content/Python/wasami_tools/pipeline/cc2_level.py
  - Content/Python/wasami_tools/pipeline/dd_assets.py
updated: 2026-09-16
---

# 取り込みの仕組み

## 役割
原作データ（`cc2_reference`・`pak_reference`）と WebGL 版の派生データから、UE のアセットとステージのレベルを作る。エディタの外で動く前処理（`Tools/`）と、エディタの中で動くツールセット（`Content/Python/wasami_tools`、MCP から呼ぶ）に分かれる。

## 公開インターフェース

| ツール（MCP） | 内容 |
| --- | --- |
| `WasamiStageTools.import_cc2_assets(max_items=40)` | メッシュ → テクスチャ → マテリアルの順に、まだ無いものを `max_items` 件だけ作る。戻り値は `imported` / `remaining` と種類ごとの `*_done` / `*_total` |
| `WasamiStageTools.refresh_cc2_asset_settings()` | マスターマテリアルの版が古ければ作り直し、テクスチャの設定を用途どおりに直し、全マテリアルインスタンスを再コンパイルする |
| `WasamiStageTools.build_cc2_level(map_path="/Game/Stage/Maps/L_Zone1")` | レベルを作り（または開き）、前の組み立てが置いたアクタ（タグ `cc2`）を消してから置き直し、保存する |
| `WasamiDDTools.import_dd_camera_shakes(asset_paths)` | 本家のカメラシェイクを `LegacyCameraShake` の Blueprint として `/Game/DD/<元のパス>` に作る |
| `WasamiDevTools.execute_console_command(command)` | エディタのワールドでコンソールコマンドを実行する |

| スクリプト（エディタの外） | 内容 |
| --- | --- |
| `python Tools/cc2/prepare_stage.py` | `Intermediate/Pipeline/cc2/stage_ue.json` と区画ごとのマテリアルに分けた glb を作る |
| `python Tools/ue_remote.py <file.py>` / `-c "<code>"` | 起動中のエディタで Python を実行する（PythonScriptPlugin のリモート実行） |
| `python Tools/editor_cycle.py [--quit-only] [--no-quit] [--no-build]` | 保存してエディタを閉じ、C++ をビルドし、開き直して MCP が応答するまで待つ |

## 内部構造と処理の流れ

### 前処理（`Tools/cc2/prepare_stage.py`）
- 入力: WebGL 版の `assets-src/cc2/layout.json`（配置 1,242・灯 301・マテリアル 157・反射キャプチャ・ボリューム・霧・空）と `assets-src/level/stage.json`（ゲームの目印）、`cc2_reference` の glb / png、WebGL 版が作った差し替えテクスチャ（`assets-src/cc2/tex/`）。パスは環境変数 `CC2_REF`（既定 `<repo>/cc2_reference`）と `WASAMI_WEBGL`（既定 `C:\Users\User\Downloads\wasami-deseption`）。
- **座標**: WebGL 版の行列は Blender の frame（`Tz · K · M_ue · K⁻¹`、`K = diag(0.01, −0.01, 0.01)`、`floorZ = −335`）。`M_ue = K⁻¹ · Tz⁻¹ · M · K` で UE（cm・左手・z 上）に戻し、`decompose` が位置・クォータニオン・スケールにする（鏡映は −X スケール、ずれは `fitError`。最大 0.0001 cm/m）。
- **glb**: 各メッシュを読み、プリミティブごとに `S0`, `S1`, … のマテリアルを付け直し、`images` / `textures` / `samplers` を外し、UV は `TEXCOORD_0` / `TEXCOORD_1` だけ残して `Intermediate/Pipeline/cc2/meshes/` に書く（書き出しの glb は区画が同じマテリアルを共有していて、そのままだとスロットが 1 つに潰れる）。
- **アセットのパス**: CC2 の `/Game` の木を `/Game/CC2/` にそのまま写す（`game_path`。使えない文字は `_` + ハッシュに）。生成テクスチャは `/Game/CC2/Generated/`。
- 出力 `stage_ue.json`: `meshes`（glb・アセット・区画数・当たりの種類）、`textures`（用途 albedo / normal / orm / emissive / lut / cubemap）、`materials`、`placements`、`lights`、`captures`、`volumes`、`fog`、`sky`、`gameplay`（開始・チェックポイント・シャード 301・敵の出現・特殊シャード・トリガー・配電盤）。

### 取り込み（`pipeline/cc2_assets.py`）
- `ensure_mesh_pipeline()`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を複製した `/Game/Pipeline/Interchange/PL_CC2_StaticMesh`。種類ごとのサブフォルダなし、マテリアルとテクスチャを取り込まない、当たりの自動生成なし。
- `ensure_master_material()`: `/Game/Pipeline/Materials/M_CC2_Standard`。テクスチャか定数で基本色（× 色味）・法線・ORM・発光（× 色 × 強さ）を切り替える静的スイッチ（`UseBaseColorTex` / `UseNormalTex` / `UseORMTex` / `UseEmissiveTex` / `UseAlphaOpacity`）。ブレンドと両面はインスタンスの `base_property_overrides`。`MASTER_VERSION` を上げるとその場で作り直す（インスタンスの親は保たれる）。ORM の既定は線形の白 `/Game/Pipeline/Textures/T_Default_Masks`（`_white_png` が 4×4 の PNG を書いて取り込む）。
- `import_mesh()`: Interchange で取り込み、Nanite を有効（半透明・加算を使うメッシュだけ無効、`translucent_meshes`）、フォールバックの誤差 0、当たりのある物は `CTF_USE_COMPLEX_AS_SIMPLE`。
- `import_texture()` / `apply_texture_settings()`: 用途ごとに圧縮・sRGB・LOD グループを決める（albedo / emissive は `TC_DEFAULT`・sRGB・World、normal は `TC_NORMALMAP`、orm は `TC_MASKS`、lut は `TC_VECTOR_DISPLACEMENTMAP`・ミップなし・ColorLookupTable）。取り込みの推測（UI 用や法線と誤認）を上書きする。
- `make_material()`: マテリアルインスタンスを作り、テクSチャ・定数・ブレンド（OPAQUE / MASK / BLEND / ADD）・両面・不透明マスクのしきい値（既定 0.3333）を入れる。デカールは半透明・両面・粗さ 0.8。
- `import_batch()` / `refresh_settings()`: 上をまとめて回す。`refresh_settings` はマスターの版・テクスチャの設定・全インスタンスの再コンパイル（`update_material_instance`）を行う。

### 組み立て（`pipeline/cc2_level.py`）
- レベルを開く（無ければ作る）→ タグ `cc2` のアクタを消す → 配置・灯・反射キャプチャ・ポストプロセスボリューム・霧・スカイライト・プレイヤースタートを置く → 保存。
- 配置: メッシュごとに `StaticMeshActor`、区画ごとのマテリアル、当たりの無いものは `NoCollision`、`static` 以外の役割は Movable、隠れている物は `SetActorHiddenInGame`、ラベルは `<アクター>.<コンポーネント>`、フォルダは `CC2/Meshes/<役割>`、タグに `role:` と `src:`。
- 灯: 点・スポット・矩形を CC2 の値で（強さと単位、色は sRGB のバイト、減衰半径、影、体積散乱、光源の大きさ、矩形の大きさとバーンドア、スポットの内外角）。
- ポストプロセスボリューム: `overrides` に挙がったプロパティだけを入れ、値が書き出しに無いもの（＝既定値のまま上書き）は override だけ立てる。**モーションブラーは入れない**（本家のプレイヤーの既定 0.5 を使うため。`SKIP_VOLUME_SETTINGS`）。
- 霧・スカイライト: 書き出しのプロパティをそのまま（`ue_props.apply`）。スカイライトは CC2 の HDRI を取り込んだキューブマップ。
- プレイヤースタート: `gameplay.start` の位置 + 100 cm、`unreal.Rotator` は名前付きで渡す（位置引数は roll, pitch, yaw）。

### 共通（`pipeline/paths.py`、`pipeline/ue_props.py`）
- `paths`: プロジェクトの場所、`stage_ue.json`、パイプラインのアセットのパス、`DD_PAK`（環境変数 `PAK_REF`、既定 `<project>/pak_reference`）。
- `ue_props`: UE のプロパティ名 → Python 名（`CameraISO` → `camera_iso`、`bOverride_X` → `override_x`）、書き出しの値 → Python の値（Vector / Vector4 / Color / LinearColor / 列挙）、構造体は中身だけを再帰的に入れる。

### 本家のアセット（`pipeline/dd_assets.py`）
- `pak_reference/_assets/DDeception/Content/<パス>.json` の `Default__*` のプロパティを、`LegacyCameraShake` を親にした Blueprint の CDO に入れる（UE4 の `UCameraShake` がそのまま `LegacyCameraShake` なので、振幅・周波数・ブレンドの意味が一致する）。

## 作るアセット

| パス | 中身 |
| --- | --- |
| `/Game/CC2/…` | CC2 の /Game の木そのまま。メッシュ 68・テクスチャ 286・マテリアルインスタンス 157 |
| `/Game/Pipeline/Interchange/PL_CC2_StaticMesh`、`/Game/Pipeline/Materials/M_CC2_Standard`、`/Game/Pipeline/Textures/T_Default_Masks` | 取り込みの道具 |
| `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake`・`_RunShake` | 本家の頭の揺れ |
| `/Game/Stage/Maps/L_Zone1` | ステージ。配置 1,242・灯 301・反射キャプチャ 2・ポストプロセスボリューム 2・霧・スカイライト・プレイヤースタート |

## 原作データの根拠
- 配置・灯・マテリアル・ボリューム・霧・空・ゲームの目印: `cc2_reference` の `Chaotic_Customer_Zone_1.umap.json` を WebGL 版の `scripts/cc2/layout.py` が解いた `layout.json` / `stage.json`（`.claude/references/chaotic-customer-2/README.md`）。
- 人物の描かれたポスターと落書き 10 枚は WebGL 版が差し替えたワサミの絵（`assets-src/cc2/tex/replace/`）。
- カメラシェイク: `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter_*Shake.json`。

## 依存関係
- `Tools/cc2/prepare_stage.py` は numpy（システムの Python 3.10）。エディタ側の Python は標準ライブラリと `unreal` だけ。
- ツールセットは `toolset_registry`（ToolsetRegistry プラグイン）に登録し、MCP の `call_tool` から呼ばれる。ツールは呼ばれるたびに `wasami_tools.pipeline` を読み込み直す。

## 既知の制約・注意点
- 新しいツールセットのクラスを足したときは、`reload_module` では登録されない（`.claude/guides/unreal-workflow.md` の手順で明示的に登録するか、エディタを開き直す）。
- 当たりはすべて描画のメッシュそのもの（complex as simple）。CC2 の単純な当たり（凸包・箱）は移していない。
- 動く部品（扉・柵・障壁・街灯・車・列車）はまだ静的なメッシュとして置いているだけ。
- デカール（`Chaotic_Customer_Zone_1_Decals.usda` の 112）は未取り込み。
- `refresh_cc2_asset_settings` は LUT のテクスチャを毎回「変わった」と数える（設定の読み戻しが違うため）。害はない。

## 変更履歴
- 2026-09-16: 初版（取り込みと組み立ての現行実装を記録）
