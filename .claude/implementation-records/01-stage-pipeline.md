---
title: 取り込みの仕組み（ツールセット・リモート実行・本家のアセット）
sources:
  - Tools/dd/prepare_stage.py
  - Tools/ue_remote.py
  - Tools/editor_cycle.py
  - Tools/console_session.py
  - Tools/desktop.py
  - Tools/desktop_agent.py
  - Content/Python/init_unreal.py
  - Content/Python/wasami_tools/__init__.py
  - Content/Python/wasami_tools/toolsets/__init__.py
  - Content/Python/wasami_tools/toolsets/dd.py
  - Content/Python/wasami_tools/toolsets/dev.py
  - Content/Python/wasami_tools/toolsets/stage.py
  - Content/Python/wasami_tools/pipeline/__init__.py
  - Content/Python/wasami_tools/pipeline/paths.py
  - Content/Python/wasami_tools/pipeline/ue_props.py
  - Content/Python/wasami_tools/pipeline/dd_assets.py
  - Content/Python/wasami_tools/pipeline/dd_stage.py
  - Content/Python/wasami_tools/pipeline/dd_level.py
  - Content/Python/wasami_tools/pipeline/dd_tablet.py
updated: 2026-09-16
---

# 取り込みの仕組み

## 役割
原作データ（`pak_reference/`・`pak_reference_2/`）から UE のアセットを作る。エディタの外で動くスクリプト（`Tools/`）と、エディタの中で動くツールセット（`Content/Python/wasami_tools`、MCP から呼ぶ）に分かれる。

**ステージの取り込みと組み立ては作り直しの途中**（2026-09-16）。CC2 の Zone_1 を取り込んでいた前処理・ツールセット（`Tools/cc2/prepare_stage.py`、`WasamiStageTools`、`pipeline/cc2_assets.py`・`cc2_level.py`）は、ステージを本家の病院（`06_Hospital_Zone_01`・`06_Hospital_Zone_02`）へ差し替える方針変更で削除した。病院の**前処理と取り込みはできている**（`Tools/dd/prepare_stage.py`、`pipeline/dd_stage.py`）。レベルの組み立ては、この記録に書き足していく。

## 公開インターフェース

| ツール（MCP） | 内容 |
| --- | --- |
| `WasamiStageTools.import_dd_stage_assets(max_items=40)` | メッシュ → テクスチャ → マテリアルの順に、まだ無いものを `max_items` 件だけ作る。戻り値は `imported` / `remaining` と種類ごとの `*_done` / `*_total` |
| `WasamiStageTools.refresh_dd_stage_assets()` | マスターマテリアルの版が古ければ作り直し、テクスチャの設定を原作どおりに直し、全マテリアルインスタンスを再コンパイルする |
| `WasamiStageTools.build_dd_stage_level(zone="Zone1", map_path="")` | そのゾーンのレベルを作り（または開き）、前の組み立てが置いたアクタ（タグ `dd`）を消してから置き直し、保存する |
| `WasamiDDTools.import_dd_camera_shakes(asset_paths)` | 本家のカメラシェイクを `LegacyCameraShake` の Blueprint として `/Game/DD/<元のパス>` に作る |
| `WasamiDDTools.import_dd_tablet()` | タブレット一式（メッシュ・マテリアル・テクスチャ・フォント・音・ミニマップ）を `/Game/DD` に作る（中身は 03 記録） |
| `WasamiDevTools.execute_console_command(command)` | エディタのワールドでコンソールコマンドを実行する |

| スクリプト（エディタの外） | 内容 |
| --- | --- |
| `python Tools/dd/prepare_stage.py [--out <dir>]` | 病院の Zone 1・Zone 2 を `Intermediate/Pipeline/dd/stage_ue.json` にまとめる |
| `python Tools/ue_remote.py <file.py>` / `-c "<code>"` | 起動中のエディタで Python を実行する（PythonScriptPlugin のリモート実行）。終了コードは 0 成功 / 1 Python エラー / 2 エディタが応答しない |
| `python Tools/editor_cycle.py [--quit-only] [--no-quit] [--no-build]` | 保存してエディタを閉じ、C++ をビルドし、**対話デスクトップで**開き直して、リモート実行が応答するまで待つ |
| `python Tools/console_session.py <exe> [args] [--wait <画像名>]` | 任意のプログラムを**対話デスクトップ（コンソールのセッション）で**起動する。Claude はセッション 0 にいて GPU の出力が見えないので、GUI のプログラムは一度きりのスケジュールタスク（ログオン中のユーザーの SID・`LogonType Interactive`）経由で起動する。起動したらタスクを消す。本家のゲームのランチャを動かすのに使う（`.claude/guides/verification.md`） |
| `python Tools/desktop.py <start\|shot\|click\|key\|hold\|look\|stop\|…>` | 対話デスクトップの画面を撮り、入力を送る。セッション 1 に常駐する `Tools/desktop_agent.py`（`pythonw.exe`、`console_session.py` が起動）と `Intermediate/DesktopAgent/` の JSON でやり取りする。入力は前面の窓が許可した対象（既定は本家のゲーム）のときだけ届き、OS 全体に効くキーは断る。使い方と枠は `.claude/guides/verification.md` の「画面を操作する」 |

## 内部構造と処理の流れ

### 前処理（`Tools/dd/prepare_stage.py`）
- 入力は原作データ `pak_reference_2`（環境変数 `PAK_REF2`、既定 `<repo>/pak_reference_2`）の `_levels/06_Hospital_Zone_01{,_02}.scene.json` と `.full.json`、`_meshes.json`、`_materials.json`、`_textures.json`、`_assets/**`。出力は `Intermediate/Pipeline/dd/stage_ue.json`（約 3.7 MB）。**素材はほとんど複製しない**（glTF と PNG は `pak_reference_2` から直接取り込む。書き出しのメッシュは元のメッシュ空間のままで、glTF のマテリアル名がスロット名）。
- **区画の分け直し（`sectioned_gltf`）**: 同じマテリアルを使う区画が 2 つ以上あるメッシュは、glTF ではそれらが同じマテリアルを指すため、UE の取り込みが 1 スロットに統合してしまい、それより後ろのスロット番号がすべてずれる（原作の StaticMesh は区画ごとにスロットを持つ）。そういうメッシュだけ、glTF の JSON を `Intermediate/Pipeline/dd/meshes/` に書き直す（`materials` を区画ごとの一意な名前 `<元のマテリアル名>__<番号>` にし、各プリミティブがそれを指すようにし、テクスチャの定義を外す）。**`.bin` は複製せず**、`buffers[].uri` を `pak_reference_2` の元ファイルへの相対 URI にする。病院では 13 個が対象（`hospital_zone_01_tiles_tile_01` 19 区画 → 統合されると 16 など）。
- **アセットのパス**: 本家の `/Game` の木を `/Game/DD/` にそのまま写す（`asset_of`。使えない文字は `_` + ハッシュ）。`/Engine/...`（`BasicShapes/Plane`・`Cube`）は取り込まず、エンジンのものをそのまま使う。テクスチャは `DDeception/Content/…` → `/Game/DD/…`、`Engine/Content/…` → `/Game/DD/_Engine/…`。
- **マテリアル**（`resolve_material`）: 親チェーンを子 → 親にたどり、テクスチャ・スカラ・ベクタ・`base_property_overrides`（ブレンド・両面・不透明マスク）を解決する（子が勝つ）。根の `Material` の `texture_expressions` は最後の既定。根のパスで `master` を決める（`substance` 59・`decal` 31・`emissive` 25・`alphamask` 10・`lit` 5・`translucent` 1・`glassmask` 1・`other` 11）。テクスチャは引数名から用途に振り分ける（`TEX_KIND`: `Albedo`/`Texture` → albedo、`Normal` → normal、`Packed` → packed、`Emissive` → emissive）。振り分けられない引数（`DetailRoughnessT`・`Dirt Mask`・`HDR` など 8 種）は `unknownParams` に記録するだけで使わない。
- **配置**: `scene.json` の `static_meshes` をそのまま（`world` はアタッチ階層を合成済みの絶対変換）。スロットごとのマテリアルは配置の `override_materials` → メッシュの `material_slots` の順。`full.json` から描画と当たりに関わるプロパティだけ拾う（`KEEP_COMPONENT_PROPS` と `BodyInstance.CollisionProfileName`）。
- **灯**: `scene.json` の `lights`。**値が空のもの（BP の中の灯）は Blueprint のクラス既定から取る**（`Export.light_template`: 同名のコンポーネント → クラス内の任意の灯 → 親クラス、と最大 4 段たどる）。クラス既定に相対変換があれば親のワールド変換に合成する（`compose`。書き出しの `world` は BP 内の相対変換を含まない）。スカイライトはこの一覧に入っているので分けて `sky` にし、`full.json` の `SourceType`・`Cubemap` を混ぜる。
- **環境**: 反射キャプチャ（球 10・箱 1）、霧、スカイライト、ポストプロセスボリューム（Zone 2 に 1 つ、`bUnbound`）。キューブマップは `_textures.json` から実ファイルを引く。
- **ゲームの部品**: メッシュと灯以外のアクタ（`SKIP_ACTOR_CLASSES` を除く）を `actors` に、クラス・名前・ルートのワールド変換・単純なプロパティで出す（Zone 1 で 873、Zone 2 で 836）。

### 取り込み（`pipeline/dd_stage.py`）
- `ensure_mesh_pipeline()`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を複製した `/Game/Pipeline/Interchange/PL_DD_StaticMesh`。種類ごとのサブフォルダなし、マテリアルとテクスチャを取り込まない、当たりの自動生成なし。
- `ensure_masters()`: 本家のマスターマテリアル（式は cook で消えている）を 3 つに作り直す。`MASTER_VERSION` を上げるとその場で作り直す（インスタンスの親は保たれる）。
  - `/Game/Pipeline/Materials/M_DD_Substance` … `MM_Main_Substance` と派生（Emissive・AlphaColorMask・Translucent・Glass）と分類外のすべて。`Albedo`（sRGB）・`Normal`・`Packed`（R 遮蔽・G 粗さ・B 金属、それぞれ `Roughness Power` / `Metallic Power` の pow を通す）・`Emissive`（× `Emissive Color Multiplier` × `Emissive Intensity`）。静的スイッチ `UseEmissive`・`UseMaskColor`（`Mask Color` を Albedo のアルファで乗せる）。不透明度は Albedo のアルファ × `Opacity Override`。`Packed` の既定は `/Game/Pipeline/Textures/T_DD_DefaultPacked`（遮蔽 1・粗さ 0.5・金属 0 の 4×4 を `_solid_png` が書いて取り込む。マスク用サンプラーは sRGB のテクスチャを受け付けないため）
  - `M_DD_Decal` … `M_01_Hotel_Decals`。`MD_DeferredDecal`・`BLEND_Translucent`、`Texture` × `Color Multiplier` を基本色に、アルファを不透明度に
  - `M_DD_Unlit` … `MM_Lit`。`MSM_Unlit` で `Light Color` × `Light Multiplier` を発光に
- `import_mesh()`: Interchange で glTF を取り込み、Nanite を有効（半透明・加算・デカールを使うメッシュだけ無効、`translucent_meshes`）、フォールバックの誤差 0、当たりは `CTF_USE_COMPLEX_AS_SIMPLE`。取り込んだスロット数が書き出しと違えば警告する。
- `import_texture()` / `apply_texture_settings()`: 圧縮・sRGB・LOD グループは**原作のテクスチャの設定をそのまま写す**（`_textures.json` の `compression` / `srgb` / `lod_group`。無指定は `TC_Default`・`TEXTUREGROUP_World`）。取り込みの推測（UI 用や法線と誤認）を上書きする。
- `make_material()`: `master` に対応するマスターのインスタンスを作り、用途ごとのテクスチャ（`TEX_PARAM`）、写せるスカラ・ベクタ（`SCALARS`・`VECTORS`）、静的スイッチ、`base_property_overrides`（ブレンド・両面・不透明マスクのしきい値・Unlit の上書き）を入れる。写せない引数（`Normal Flatness`・`RefractionDepthBias`・`Emissive Multiplier`・`Fade Length (S)` など）は数えるだけで**推測しない**。
- `import_batch()` / `refresh_settings()`: 上をまとめて回す。`import_batch` はまだ無いものだけを `max_items` 件作り、`/Game/DD` と `/Game/Pipeline` を保存する。

### 組み立て（`pipeline/dd_level.py`）
- `build(zone, map_path)`: レベルを開く（無ければ作る）→ タグ `dd` のアクタを消す → 配置・灯・反射キャプチャ・霧・スカイライト・ポストプロセスボリューム・プレイヤースタート・ミニマップの地図の板を置く → 保存。戻り値は種類ごとの数と `failed_settings`。
- 配置: `StaticMeshActor` をワールド変換（書き出しは合成済み）で置き、スロットごとにマテリアルを割り当てる。デカール材が載っていれば `NoCollision`（本家のデカールは板メッシュ）。`Mobility`・`CollisionProfileName`・`bVisible`・`bHiddenInGame` は明示的に入れ、残りは `ue_props.apply`。ラベルは `<アクター>.<コンポーネント>`、フォルダは `Hospital/Meshes/<アクターのクラス>`、タグに `src:`。
- 灯: `PointLight` / `SpotLight` / `RectLight` / `DirectionalLight`。**静的ライティングを切っている（Lumen）のですべて Movable**。`IntensityUnits` を `Intensity` より先に入れる（単位で数の意味が変わるため）。**書き出しに `IntensityUnits` が無い局所灯は `Unitless` を明示する**（UE 4.24 の既定は Unitless、UE5 は Candelas。入れないと明るさが桁違いになる）。
- 反射キャプチャ: 球と箱。明るさ、球の影響半径、箱は書き出しのスケール。
- 霧・スカイライト: 書き出しのプロパティをそのまま（`ue_props.apply`。UE5 で改名されたものは `ue_props.RENAMED` が読み替える）。スカイライトの `SLS_SpecifiedCubemap` には TextureCube が要るが、書き出しの HDRI は平面の PNG（`PF_FloatRGBA` を 8 bit に落としたもの）なので Texture2D にしかならない。その場合は指定せずシーンのキャプチャに任せ、`failed_settings` に記録する。
- ポストプロセスボリューム: `bOverride_*` が立っているものだけ入れ、値が書き出しに無いもの（＝既定値のまま上書き）は override だけ立てる。
- プレイヤースタート: `actors` の `PlayerStart`（変換はその `CollisionCapsule` のもの）。

### 登録（`init_unreal.py`、`wasami_tools/__init__.py`）
- エディタの起動時に `init_unreal.py` が `wasami_tools.register()` を呼び、`Registration([dd.WasamiDDTools, dev.WasamiDevTools])` が ToolsetRegistry に登録する（MCP に出る）。
- 各ツールは呼ばれるたびに `wasami_tools.pipeline` の中身（`paths`・`ue_props` と対象のモジュール）を `importlib.reload` で読み込み直すので、パイプラインの Python を直したらエディタを開き直さずに呼べる。
- ツールセットのクラス自体（引数や新しいツール）を変えたときは、`reload_module` で登録し直す。**新しいツールセットのクラスを足したときは `reload_module` では登録されない**（`.claude/guides/unreal-workflow.md` の手順）。

- 地図の板（`_map_plane`）: 書き出しの `BP_MapTexture_C`（Zone 1）/ `BP_MapTexture_MultiFloor_C`（Zone 2）のワールド変換に `/Engine/BasicShapes/Plane` を置き、`MAP_PLANE_MATERIAL` のゾーンごとのマテリアル（`/Game/DD/UI/Minimap/MM_Map_06_Zone01`・`MM_Map_06_Zone2`）を入れ、影・ナビ・当たりを切り、`bVisibleInSceneCaptureOnly` を立て、タグ `dd_minimap` を付ける。プレイヤーのシーンキャプチャがこのタグで拾う（03 記録）。

### タブレットの素材（`pipeline/dd_tablet.py`）
`import_all()` がタブレットのメッシュ・マテリアル・テクスチャ 17・フォント・音 3・ミニマップのレンダーターゲットとマテリアルを `/Game/DD` に作り、自前のマスター 3 つを `/Game/Pipeline/Materials` に建てる。中身と原作の根拠は 03 記録。`dd_stage` の `import_texture` / `import_mesh` / `ensure_masters` / `_Graph` を使い回す。

### 共通（`pipeline/paths.py`、`pipeline/ue_props.py`）
- `paths`: プロジェクトの場所（`PROJECT`）、原作データの場所（`DD_PAK` = 環境変数 `PAK_REF`、既定 `<project>/pak_reference`。`DD_PAK2` = `PAK_REF2`、既定 `<project>/pak_reference_2`）、本家のアセットの置き場所 `DD_ROOT` = `/Game/DD`、パッケージパスの分解（`split`・`object_path`）。
- `ue_props`: UE のプロパティ名 → Python 名（`CameraISO` → `camera_iso`、`bOverride_X` → `override_x`）、書き出しの値 → Python の値（辞書の Vector / Vector4 / Color / LinearColor、**`pak_reference_2` が色やベクトルに使う配列**〈`[183, 163, 145, 255]`〉、列挙）、構造体は中身だけを再帰的に入れる（`apply`）。読めなかったものは `failures` に積む。**UE の版で名前が変わったプロパティは `RENAMED` で読み替える**（`FogInscatteringColor` → `FogInscatteringLuminance`、`DirectionalInscatteringColor` → `DirectionalInscatteringLuminance`。どちらも同じ LinearColor の改名なので値はそのまま）。

### 本家のアセット（`pipeline/dd_assets.py`）
- `pak_reference/_assets/DDeception/Content/<パス>.json` の `Default__*` のプロパティを、`LegacyCameraShake` を親にした Blueprint の CDO に入れる（UE4 の `UCameraShake` がそのまま `LegacyCameraShake` なので、振幅・周波数・ブレンドの意味が一致する）。
- 1 つでも入らないプロパティがあれば例外にする（黙って違う値のアセットを作らないため）。

## 作るアセット

| パス | 中身 |
| --- | --- |
| `/Game/DD/Meshes/…`・`/Game/DD/Textures/…`・`/Game/DD/Materials/…` ほか | 病院のステージ。本家の `/Game` の木そのまま。メッシュ 64・テクスチャ 282・マテリアルインスタンス 143 |
| `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake`・`_RunShake` | 本家の頭の揺れ（02 記録のプレイヤーが参照する） |
| `/Game/Pipeline/Interchange/PL_DD_StaticMesh`、`/Game/Pipeline/Materials/M_DD_Substance`・`M_DD_Decal`・`M_DD_Unlit`、`/Game/Pipeline/Textures/T_DD_DefaultPacked` | 取り込みの道具 |
| `/Game/Stage/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2` | ステージのレベル（`build_dd_stage_level` が組み立てる。Zone 1 は配置 923・灯 1,120、Zone 2 は配置 819・灯 751） |

## 原作データの根拠
- カメラシェイク: `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter_*Shake.json`。
- ステージ（これから）: `pak_reference_2/_levels/06_Hospital_Zone_01.scene.json`・`06_Hospital_Zone_02.scene.json`、`_meshes.json`、`_materials.json`、`_textures.json`、`_meshes_gltf/`。

## 依存関係
- エディタ側の Python は標準ライブラリと `unreal` だけ（Pillow・numpy は使えない）。エディタの外のスクリプトはシステムの Python。
- ツールセットは `toolset_registry`（ToolsetRegistry プラグイン）に登録し、MCP の `call_tool` から呼ばれる。
- `Tools/ue_remote.py` はエンジン同梱の `remote_execution`（`UE_ENGINE_DIR`、既定 `C:\Program Files\Epic Games\UE_5.8\Engine`）を読む。リモート実行は別々の globals / locals でコードを走らせるので、処理は関数に入れて呼ぶ。

## 既知の制約・注意点
- 新しいツールセットのクラスを足したときは、`reload_module` では登録されない（明示的に `register_toolset_class` するか、エディタを開き直す）。
- **原作のマテリアルの式は cook で消えている**ので、`Normal Flatness`（インスタンスは 1.2〜3.0、マスターの既定は 0）・`Roughness Power` / `Metallic Power` 以外のスカラは適用していない。`Roughness Power` / `Metallic Power` は既定 1.0 が恒等になる pow として実装した（推定）。見え方を原作と比べる段で見直す。
- **UE の版の違い**: 本家のデカールは `DecalBlendMode = DBM_DBuffer_ColorRoughness` だが、UE 5.8 では `decal_blend_mode` が非推奨（No longer used）で Python から読めない。いまの UE はつないだ出力で DBuffer のチャンネルが決まるので、基本色と不透明度だけをつないでいる。
- 当たりはすべて描画のメッシュそのもの（complex as simple）。書き出しのメッシュは `body_setup` を持たない。
- `editor_cycle.py` は起動時に `sys.stdout` / `sys.stderr` を `errors="replace"` にし直す。ビルドの出力にこの PC のコンソール（cp932）で出せない文字が混ざると、ビルドの失敗を報告する途中で `UnicodeEncodeError` になって落ちていた。
- **エディタの起動はセッションを跨ぐ**。Claude Code は Windows のセッション 0（サービス側）で動いており、そこには GPU の出力が無い（ログの `LogD3D12RHI: Adapter has … 0 output[s]`）ので、そのまま起動したエディタは D3D12 のスワップチェーンを作れず `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` で即落ちる。`start_editor()` は `ProcessIdToSessionId` と `WTSGetActiveConsoleSessionId` で自分のセッションとコンソールのセッションを比べ、違えば一度きりのスケジュールタスク（`WasamiLaunchEditor`。プリンシパルはログオン中のユーザーを **SID で**指定し、`LogonType Interactive`・`RunLevel Limited`）でログオン中のセッションに起動する。ユーザー名の形（ドメインなしの PC では `WORKGROUP` になる）では登録できないので SID を使う。タスクはエディタが応答したら消す（`drop_task`）。
- 起動の完了は**リモート実行が答えるか**で見る（`editor_answers`）。この PC では Docker Desktop が 127.0.0.1:8000 を掴んでいるため、MCP のポートに繋がってもエディタが起きているとは限らない。
- Zone 2 のポストプロセスボリュームの **`ColorGradingLUT` と `WeightedBlendables` は入れていない**。前者は書き出しがアセットのパスの文字列（`/Game/ThirdParty/Chameleon/LUTs/LUT_Classic8`）で、取り込んだテクスチャに解決する仕組みがまだない。後者はポストプロセスのマテリアル（`M_SharpenFilter_Inst`）で、マスターの式が cook で消えている。**このボリュームは `ColorGradingIntensity` が 0 なので、LUT の見た目への寄与は無い**。
- スカイライトのキューブマップ（`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`）は**回収できていない**。原作は TextureCube だが、書き出しは 512×512 の平面 PNG（float の階調は落ちている）で、UE に取り込むと Texture2D になる。いまはシーンのキャプチャに任せている。病院は屋内なので寄与は小さいが、見え方を比べる段で見直す。
- 原作の cook されたデータは、既定値と同じプロパティを持たない。ポストプロセスの override が立っていて値が無いのは「既定値で上書き」の意味。
- MCP のポートは Docker Desktop と衝突しうる（`.claude/guides/unreal-workflow.md`）。MCP が使えないときは `Tools/ue_remote.py` で作業できる。

## 変更履歴
- 2026-09-16: 画面操作の道具（`Tools/desktop.py` と `Tools/desktop_agent.py`）を足した。Claude はセッション 0 にいてセッション 1 の画面を触れないので、セッション 1 に常駐するエージェントとファイル経由でやり取りする（ユーザーの指示で「画面操作も Claude が行う」に変更）
- 2026-09-16: `Tools/console_session.py` を足した（`editor_cycle.py` の対話デスクトップでの起動を、任意のプログラムに使える形にしたもの。手元の本家のゲームの起動に使う）
- 2026-09-16: `editor_cycle.py` がエディタを対話デスクトップ（コンソールのセッション）でスケジュールタスク経由で起動するようにし、起動の判定を MCP のポートからリモート実行の応答に変えた
- 2026-09-16: タブレットの素材の取り込み（`pipeline/dd_tablet.py`、`WasamiDDTools.import_dd_tablet`）と、レベルにミニマップの地図の板を置く `_map_plane` を足した。`editor_cycle.py` の出力の文字化けで落ちる問題を直した
- 2026-09-16: 病院の取り込み（`pipeline/dd_stage.py`、マスターマテリアル 3 種）と組み立て（`pipeline/dd_level.py`）、それを呼ぶ `WasamiStageTools` を足した。`ue_props` が配列の色・ベクトルを読めるようにした
- 2026-09-16: ステージを本家の病院へ差し替える方針変更にともない、CC2 の前処理（`Tools/cc2/prepare_stage.py`）・取り込み（`pipeline/cc2_assets.py`）・組み立て（`pipeline/cc2_level.py`）・ツールセット（`toolsets/stage.py` の `WasamiStageTools`）を削除し、`paths.py` から CC2 の定数を外して `DD_PAK2`（`pak_reference_2`）を足した
- 2026-09-16: 初版（取り込みと組み立ての現行実装を記録）
