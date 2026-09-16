---
title: 取り込みの仕組み（ツールセット・リモート実行・本家のアセット）
sources:
  - Tools/dd/prepare_stage.py
  - Tools/ue_remote.py
  - Tools/editor_cycle.py
  - Content/Python/init_unreal.py
  - Content/Python/wasami_tools/__init__.py
  - Content/Python/wasami_tools/toolsets/__init__.py
  - Content/Python/wasami_tools/toolsets/dd.py
  - Content/Python/wasami_tools/toolsets/dev.py
  - Content/Python/wasami_tools/pipeline/__init__.py
  - Content/Python/wasami_tools/pipeline/paths.py
  - Content/Python/wasami_tools/pipeline/ue_props.py
  - Content/Python/wasami_tools/pipeline/dd_assets.py
updated: 2026-09-16
---

# 取り込みの仕組み

## 役割
原作データ（`pak_reference/`・`pak_reference_2/`）から UE のアセットを作る。エディタの外で動くスクリプト（`Tools/`）と、エディタの中で動くツールセット（`Content/Python/wasami_tools`、MCP から呼ぶ）に分かれる。

**ステージの取り込みと組み立ては作り直しの途中**（2026-09-16）。CC2 の Zone_1 を取り込んでいた前処理・ツールセット（`Tools/cc2/prepare_stage.py`、`WasamiStageTools`、`pipeline/cc2_assets.py`・`cc2_level.py`）は、ステージを本家の病院（`06_Hospital_Zone_01`・`06_Hospital_Zone_02`）へ差し替える方針変更で削除した。病院の**前処理はできている**（`Tools/dd/prepare_stage.py`）。エディタ側の取り込みとレベルの組み立ては、この記録に書き足していく。

## 公開インターフェース

| ツール（MCP） | 内容 |
| --- | --- |
| `WasamiDDTools.import_dd_camera_shakes(asset_paths)` | 本家のカメラシェイクを `LegacyCameraShake` の Blueprint として `/Game/DD/<元のパス>` に作る |
| `WasamiDevTools.execute_console_command(command)` | エディタのワールドでコンソールコマンドを実行する |

| スクリプト（エディタの外） | 内容 |
| --- | --- |
| `python Tools/dd/prepare_stage.py [--out <dir>]` | 病院の Zone 1・Zone 2 を `Intermediate/Pipeline/dd/stage_ue.json` にまとめる |
| `python Tools/ue_remote.py <file.py>` / `-c "<code>"` | 起動中のエディタで Python を実行する（PythonScriptPlugin のリモート実行）。終了コードは 0 成功 / 1 Python エラー / 2 エディタが応答しない |
| `python Tools/editor_cycle.py [--quit-only] [--no-quit] [--no-build]` | 保存してエディタを閉じ、C++ をビルドし、開き直して MCP が応答するまで待つ |

## 内部構造と処理の流れ

### 前処理（`Tools/dd/prepare_stage.py`）
- 入力は原作データ `pak_reference_2`（環境変数 `PAK_REF2`、既定 `<repo>/pak_reference_2`）の `_levels/06_Hospital_Zone_01{,_02}.scene.json` と `.full.json`、`_meshes.json`、`_materials.json`、`_textures.json`、`_assets/**`。出力は `Intermediate/Pipeline/dd/stage_ue.json`（約 3.7 MB）。**素材は複製しない**（glTF と PNG は `pak_reference_2` から直接取り込む。書き出しのメッシュは元のメッシュ空間のままで、glTF のマテリアル名がスロット名なので、CC2 のような区画の作り直しは要らない）。
- **アセットのパス**: 本家の `/Game` の木を `/Game/DD/` にそのまま写す（`asset_of`。使えない文字は `_` + ハッシュ）。`/Engine/...`（`BasicShapes/Plane`・`Cube`）は取り込まず、エンジンのものをそのまま使う。テクスチャは `DDeception/Content/…` → `/Game/DD/…`、`Engine/Content/…` → `/Game/DD/_Engine/…`。
- **マテリアル**（`resolve_material`）: 親チェーンを子 → 親にたどり、テクスチャ・スカラ・ベクタ・`base_property_overrides`（ブレンド・両面・不透明マスク）を解決する（子が勝つ）。根の `Material` の `texture_expressions` は最後の既定。根のパスで `master` を決める（`substance` 59・`decal` 31・`emissive` 25・`alphamask` 10・`lit` 5・`translucent` 1・`glassmask` 1・`other` 11）。テクスチャは引数名から用途に振り分ける（`TEX_KIND`: `Albedo`/`Texture` → albedo、`Normal` → normal、`Packed` → packed、`Emissive` → emissive）。振り分けられない引数（`DetailRoughnessT`・`Dirt Mask`・`HDR` など 8 種）は `unknownParams` に記録するだけで使わない。
- **配置**: `scene.json` の `static_meshes` をそのまま（`world` はアタッチ階層を合成済みの絶対変換）。スロットごとのマテリアルは配置の `override_materials` → メッシュの `material_slots` の順。`full.json` から描画と当たりに関わるプロパティだけ拾う（`KEEP_COMPONENT_PROPS` と `BodyInstance.CollisionProfileName`）。
- **灯**: `scene.json` の `lights`。**値が空のもの（BP の中の灯）は Blueprint のクラス既定から取る**（`Export.light_template`: 同名のコンポーネント → クラス内の任意の灯 → 親クラス、と最大 4 段たどる）。クラス既定に相対変換があれば親のワールド変換に合成する（`compose`。書き出しの `world` は BP 内の相対変換を含まない）。スカイライトはこの一覧に入っているので分けて `sky` にし、`full.json` の `SourceType`・`Cubemap` を混ぜる。
- **環境**: 反射キャプチャ（球 10・箱 1）、霧、スカイライト、ポストプロセスボリューム（Zone 2 に 1 つ、`bUnbound`）。キューブマップは `_textures.json` から実ファイルを引く。
- **ゲームの部品**: メッシュと灯以外のアクタ（`SKIP_ACTOR_CLASSES` を除く）を `actors` に、クラス・名前・ルートのワールド変換・単純なプロパティで出す（Zone 1 で 873、Zone 2 で 836）。

### 登録（`init_unreal.py`、`wasami_tools/__init__.py`）
- エディタの起動時に `init_unreal.py` が `wasami_tools.register()` を呼び、`Registration([dd.WasamiDDTools, dev.WasamiDevTools])` が ToolsetRegistry に登録する（MCP に出る）。
- 各ツールは呼ばれるたびに `wasami_tools.pipeline` の中身（`paths`・`ue_props` と対象のモジュール）を `importlib.reload` で読み込み直すので、パイプラインの Python を直したらエディタを開き直さずに呼べる。
- ツールセットのクラス自体（引数や新しいツール）を変えたときは、`reload_module` で登録し直す。**新しいツールセットのクラスを足したときは `reload_module` では登録されない**（`.claude/guides/unreal-workflow.md` の手順）。

### 共通（`pipeline/paths.py`、`pipeline/ue_props.py`）
- `paths`: プロジェクトの場所（`PROJECT`）、原作データの場所（`DD_PAK` = 環境変数 `PAK_REF`、既定 `<project>/pak_reference`。`DD_PAK2` = `PAK_REF2`、既定 `<project>/pak_reference_2`）、本家のアセットの置き場所 `DD_ROOT` = `/Game/DD`、パッケージパスの分解（`split`・`object_path`）。
- `ue_props`: UE のプロパティ名 → Python 名（`CameraISO` → `camera_iso`、`bOverride_X` → `override_x`）、書き出しの値 → Python の値（Vector / Vector4 / Color / LinearColor / 列挙）、構造体は中身だけを再帰的に入れる（`apply`）。読めなかったものは `failures` に積む。

### 本家のアセット（`pipeline/dd_assets.py`）
- `pak_reference/_assets/DDeception/Content/<パス>.json` の `Default__*` のプロパティを、`LegacyCameraShake` を親にした Blueprint の CDO に入れる（UE4 の `UCameraShake` がそのまま `LegacyCameraShake` なので、振幅・周波数・ブレンドの意味が一致する）。
- 1 つでも入らないプロパティがあれば例外にする（黙って違う値のアセットを作らないため）。

## 作るアセット

| パス | 中身 |
| --- | --- |
| `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake`・`_RunShake` | 本家の頭の揺れ（02 記録のプレイヤーが参照する） |
| `/Game/Pipeline/Textures/T_Default_Masks` | 線形の白 4×4（マスク用サンプラーの既定値。マスターマテリアルを作り直すときに使う） |
| `/Game/Stage/Maps/L_Hospital_Zone1` | ステージのレベル。いまは空（組み立てはこれから） |

## 原作データの根拠
- カメラシェイク: `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter_*Shake.json`。
- ステージ（これから）: `pak_reference_2/_levels/06_Hospital_Zone_01.scene.json`・`06_Hospital_Zone_02.scene.json`、`_meshes.json`、`_materials.json`、`_textures.json`、`_meshes_gltf/`。

## 依存関係
- エディタ側の Python は標準ライブラリと `unreal` だけ（Pillow・numpy は使えない）。エディタの外のスクリプトはシステムの Python。
- ツールセットは `toolset_registry`（ToolsetRegistry プラグイン）に登録し、MCP の `call_tool` から呼ばれる。
- `Tools/ue_remote.py` はエンジン同梱の `remote_execution`（`UE_ENGINE_DIR`、既定 `C:\Program Files\Epic Games\UE_5.8\Engine`）を読む。リモート実行は別々の globals / locals でコードを走らせるので、処理は関数に入れて呼ぶ。

## 既知の制約・注意点
- 新しいツールセットのクラスを足したときは、`reload_module` では登録されない（明示的に `register_toolset_class` するか、エディタを開き直す）。
- 原作の cook されたデータは、既定値と同じプロパティを持たない。ポストプロセスの override が立っていて値が無いのは「既定値で上書き」の意味。
- MCP のポートは Docker Desktop と衝突しうる（`.claude/guides/unreal-workflow.md`）。MCP が使えないときは `Tools/ue_remote.py` で作業できる。

## 変更履歴
- 2026-09-16: ステージを本家の病院へ差し替える方針変更にともない、CC2 の前処理（`Tools/cc2/prepare_stage.py`）・取り込み（`pipeline/cc2_assets.py`）・組み立て（`pipeline/cc2_level.py`）・ツールセット（`toolsets/stage.py` の `WasamiStageTools`）を削除し、`paths.py` から CC2 の定数を外して `DD_PAK2`（`pak_reference_2`）を足した
- 2026-09-16: 初版（取り込みと組み立ての現行実装を記録）
