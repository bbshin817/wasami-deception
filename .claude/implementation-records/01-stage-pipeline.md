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
| `WasamiDevTools.capture_pose(out_path, x, y, z, yaw, pitch, fov, width, height)` | いまのレベルを 1 つの視点から PNG に描く（下の「見た目を撮る」） |

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
- **ライトマップの設定（`Export.lightmap`）**: メッシュごとに、原作の `StaticMesh` の `LightMapResolution` と `LightMapCoordinateIndex` を `_assets/DDeception/Content/<パス>.json` から取り、`lightmapResolution`・`lightmapUv` に書く（省かれていれば UE の既定の 4 と 0。`StaticMesh.cpp:4738`、番号はコンストラクタで設定しない）。`_meshes.json` の `lightmap_uv_index` も同じ値だが、省略が `None` になるのでこちらを使う。病院の 64 メッシュは 64 が 59（うち UV の番号が 2 のもの 4・3 のもの 3）・32 が 1・16 が 2・8 が 1・4 が 1（`spike_brush_StaticMesh`、どちらも省略）。**原作の結合されたステージのメッシュ**（Zone 1・Zone 2 の `tiles_tile_01/02/03`、Zone 2 の `miniboss_room`）**は `TEXCOORD_1` が全頂点 (0,0)** で、ライトマップ UV の展開を持たない（2026-09-16 まではこれを glTF から測り、UE に UV を作らせていた）。
- **アセットのパス**: 本家の `/Game` の木を `/Game/DD/` にそのまま写す（`asset_of`。使えない文字は `_` + ハッシュ）。`/Engine/...`（`BasicShapes/Plane`・`Cube`）は取り込まず、エンジンのものをそのまま使う。テクスチャは `DDeception/Content/…` → `/Game/DD/…`、`Engine/Content/…` → `/Game/DD/_Engine/…`。
- **マテリアル**（`resolve_material`）: 親チェーンを子 → 親にたどり、テクスチャ・スカラ・ベクタ・`base_property_overrides`（ブレンド・両面・不透明マスク）を解決する（子が勝つ）。根の `Material` の `texture_expressions` は最後の既定。根のパスで `master` を決める（`substance` 59・`decal` 31・`emissive` 25・`alphamask` 10・`lit` 5・`translucent` 1・`glassmask` 1・`other` 11）。テクスチャは引数名から用途に振り分ける（`TEX_KIND`: `Albedo`/`Texture` → albedo、`Normal` → normal、`Packed` → packed、`Emissive` → emissive）。振り分けられない引数（`DetailRoughnessT`・`Dirt Mask`・`HDR` など 8 種）は `unknownParams` に記録するだけで使わない。
- **配置**: `scene.json` の `static_meshes` をそのまま（`world` はアタッチ階層を合成済みの絶対変換）。スロットごとのマテリアルは配置の `override_materials` → メッシュの `material_slots` の順。`full.json` から描画と当たりに関わるプロパティだけ拾う（`KEEP_COMPONENT_PROPS` と `BodyInstance.CollisionProfileName`）。**`bCastShadowAsTwoSided` も拾う**: Zone 1 のステージ本体の結合メッシュ 5 個（`tiles_tile_01/02/03`・`parking`・`tunnel`）が `True` で、内側から見た片面の部屋なので、これが無いと天井が上からの平行光源（Movable、1.5）を遮らず、Lightmass でも隣の部屋の灯が壁・天井を抜ける（2026-09-16 まで拾っておらず、開始地点の床は平行光源の漏れで 146 → 117〈平均輝度〉も明るかった）。
- **灯**: `scene.json` の `lights` の一覧を回し、**値は `full.json` のそのコンポーネントの値から取る**（`scene.json` の `light` はその一部で、`SoftSourceRadius`・`LightingChannels`・矩形灯の `SourceWidth`/`SourceHeight`・`MaxDrawDistance` を持たない。Zone 1 では `SoftSourceRadius` 386 個・`LightingChannels` 6 個〈カットシーンの灯。チャンネル 1 だけを照らす〉が落ちていた）。**BP の中の灯は「クラス既定＋レベル側の差分」**（`Export.light_template`: 同名のコンポーネント → クラス内の任意の灯 → 親の BP、と最大 4 段たどる。2026-09-16 まではレベル側に値が 1 つでもあるとクラス既定を使っておらず、Zone 2 の `wall_lamp_68` 7 個が強さ 8000 と色を、ノコギリの罠の灯 5 個が高さ +114 cm を失っていた）。クラス既定の相対変換は、レベル側がその成分を持たないときだけ親のワールド変換に合成する（`compose`。書き出しの `world` はレベル側の相対変換だけを含む）。スカイライトもこの一覧に入っているので分けて `sky` にする。
- **Mobility は土台（アーキタイプ）の値で補う**（`Export.default_mobility`・`NATIVE_MOBILITY`）。書き出しは土台と同じ値を省くので、`Mobility` が無いのは「UE の既定」ではなく「その部品の土台の値」。UE の灯のアクタ（`APointLight`・`ASpotLight`・`ARectLight`・`ADirectionalLight`）は灯を **Stationary** にし（UE 5.8 の `Light.cpp:190,244`・`SpotLight.cpp:27`・`RectLight.cpp:12`。UE4 から同じ）、`USkyLightComponent` も Stationary（`SkyLightComponent.cpp:312`）、`AStaticMeshActor` はメッシュを Static（`StaticMeshActor.cpp:34`）、BP の構築スクリプトで足した部品は `USceneComponent` の既定の **Movable**（`SceneComponent.cpp:124`）。BP では、クラスの部品の記録（CDO の子オブジェクト、または `<名前>_GEN_VARIABLE`）→ その `template`（ネイティブのアクタの子オブジェクト / コンポーネントのクラス / 親の BP の記録）の順にたどる。親は `BlueprintGeneratedClass` の `super` から取る（import 表からの推測は `BP_Shard_C` の親を `BP_DD_GameInstance_C` と誤っていた）。**根拠**: 原作の全レベルで `PointLight` アクタの灯の `Mobility` は「省略 1,342・Movable 779・Static 193」で、Stationary と書かれたものが無い。結果は Zone 1 の灯が Stationary 669・Movable 451（シャード 337 個は `BP_Shard_C` の構築スクリプトの灯で Movable。拾うと消えるので理にかなう）、Zone 2 が Stationary 155・Movable 596、スカイライトはどちらも Stationary、メッシュは Zone 1 が Static 916・Movable 7。前処理はすべての灯・メッシュ・スカイライトに `Mobility` を書く。
- **環境**: 反射キャプチャ（球 10・箱 1）、霧、スカイライト、ポストプロセスボリューム（Zone 2 に 1 つ、`bUnbound`）。キューブマップは `_textures.json` から実ファイルを引く。
- **ゲームの部品**: メッシュと灯以外のアクタ（`SKIP_ACTOR_CLASSES` を除く）を `actors` に、クラス・名前・ルートのワールド変換・単純なプロパティで出す（Zone 1 で 873、Zone 2 で 836）。

### 取り込み（`pipeline/dd_stage.py`）
- `ensure_mesh_pipeline()`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を複製した `/Game/Pipeline/Interchange/PL_DD_StaticMesh`。種類ごとのサブフォルダなし、マテリアルとテクスチャを取り込まない、当たりの自動生成なし。
- `ensure_masters()`: 本家のマスターマテリアル（式は cook で消えている）を 3 つに作り直す。`MASTER_VERSION` を上げるとその場で作り直す（インスタンスの親は保たれる）。
  - `/Game/Pipeline/Materials/M_DD_Substance` … `MM_Main_Substance` と派生（Emissive・AlphaColorMask・Translucent・Glass）と分類外のすべて。`Albedo`（sRGB）・`Normal`・`Packed`（R 遮蔽・G 粗さ・B 金属、それぞれ `Roughness Power` / `Metallic Power` の pow を通す）・`Emissive`（× `Emissive Color Multiplier` × `Emissive Intensity`）。静的スイッチ `UseEmissive`・`UseMaskColor`（`Mask Color` を Albedo のアルファで乗せる）。不透明度は Albedo のアルファ × `Opacity Override`。`Packed` の既定は `/Game/Pipeline/Textures/T_DD_DefaultPacked`（遮蔽 1・粗さ 0.5・金属 0 の 4×4 を `_solid_png` が書いて取り込む。**`TC_Default`・リニア**＝ Packed のノードのサンプラー `LINEAR_COLOR` と同じ型。原作の Packed テクスチャも 85 個が `TC_Default`・リニアで `TC_Masks` は 0 個。`ensure_default_packed` は既存のアセットの設定も直し、直したら `ensure_masters` がマスターを再コンパイルする）
  - `M_DD_Decal` … `M_01_Hotel_Decals`。`MD_DeferredDecal`・`BLEND_Translucent`、`Texture` × `Color Multiplier` を基本色に、アルファを不透明度に
  - `M_DD_Unlit` … `MM_Lit`。`MSM_Unlit` で `Light Color` × `Light Multiplier` を発光に
- `import_mesh()`: Interchange で glTF を取り込み、Nanite を有効（半透明・加算・デカールを使うメッシュだけ無効、`translucent_meshes`）、フォールバックの誤差 0、当たりは `CTF_USE_COMPLEX_AS_SIMPLE`。取り込んだスロット数が書き出しと違えば警告する。最後に `setup_lightmap()`。
- `setup_lightmap()`: 焼き込みのためのライトマップの設定。**原作の解像度と UV の番号をそのまま写す**（`lightmapResolution`・`lightmapUv`）。**UV は UE に作らせない**（LOD0 の `generate_lightmap_u_vs` を偽にする）。結合されたステージのメッシュは UV1 が全頂点 (0,0) のままなので、原作と同じく、ライトマップと静的な影がメッシュ全体で 1 点の値になる。変えたときだけ真を返す（ビルド設定を変えるとメッシュが作り直しになる）。**根拠**: 原作の `06_Hospital_Zone_01_BuiltData` は HQ ライトマップ 10 枚・計 580 万テクセル（最大 1024）・影のテクスチャ 512×512 が 1 枚で、`LevelLightingQuality` は `Quality_High`。2026-09-16 までは「原作の解像度は書き出しに無い」と誤って 1 テクセル 20 cm（32〜2048）で決め、UV1 を持たないメッシュに UE の展開を作らせ、番号も 1 に決め打ちしていた（番号が 2・3 の 7 メッシュはテクスチャの UV で焼いていた）。
- `import_texture()` / `apply_texture_settings()`: 圧縮・sRGB・LOD グループは**原作のテクスチャの設定をそのまま写す**（`_textures.json` の `compression` / `srgb` / `lod_group`。無指定は `TC_Default`・`TEXTUREGROUP_World`）。取り込みの推測（UI 用や法線と誤認）を上書きする。ただし **UE が sRGB を切る圧縮**（`TC_Alpha`・`TC_Normalmap`・`TC_Masks`・HDR の 5 種。`Texture.cpp:772`、`NO_SRGB_COMPRESSION`）では sRGB を求めない（原作は HDR の空 2 枚〈`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`〉で sRGB を真にしており、求めると UE が戻すので、かけ直すたびに変わったと数えて保存し直していた）。
- `make_material()`: `master` に対応するマスターのインスタンスを作り、用途ごとのテクスチャ（`TEX_PARAM`）、写せるスカラ・ベクタ（`SCALARS`・`VECTORS`）、静的スイッチ、`base_property_overrides`（ブレンド・両面・不透明マスクのしきい値・Unlit の上書き）を入れる。写せない引数（`Normal Flatness`・`RefractionDepthBias`・`Emissive Multiplier`・`Fade Length (S)` など）は数えるだけで**推測しない**。
- `import_batch()` / `refresh_settings()`: 上をまとめて回す。`refresh_settings` は取り込み済みのアセットに、マスターの作り直し・テクスチャの設定・**メッシュのライトマップの設定**（`setup_lightmap`、戻り値の `lightmaps_updated`）・インスタンスの再コンパイルをかけ直す。`import_batch` はまだ無いものだけを `max_items` 件作り、`/Game/DD` と `/Game/Pipeline` を保存する。

### 組み立て（`pipeline/dd_level.py`）
- `build(zone, map_path)`: レベルを開く（無ければ作る）→ タグ `dd` のアクタを消す → 配置・灯・反射キャプチャ・霧・スカイライト・ポストプロセスボリューム・プレイヤースタート・ミニマップの地図の板を置く → 保存。戻り値は種類ごとの数と `failed_settings`。
- 配置: `StaticMeshActor` をワールド変換（書き出しは合成済み）で置き、スロットごとにマテリアルを割り当てる。デカール材が載っていれば `NoCollision`（本家のデカールは板メッシュ）。`Mobility`・`CollisionProfileName`・`bVisible`・`bHiddenInGame` は明示的に入れ、残りは `ue_props.apply`。ラベルは `<アクター>.<コンポーネント>`、フォルダは `Hospital/Meshes/<アクターのクラス>`、タグに `src:`。
- 灯: `PointLight` / `SpotLight` / `RectLight` / `DirectionalLight`。**Mobility は前処理が書いた値**（`_set_mobility()`。値が無ければ置いたアクタの既定のまま＝同じ土台の規則）。Zone 1 は Stationary 669・Movable 451、Zone 2 は Stationary 155・Movable 596。メッシュとスカイライトも同じ扱い。`IntensityUnits` を `Intensity` より先に入れる（単位で数の意味が変わるため）。**書き出しに `IntensityUnits` が無い局所灯は `Unitless` を明示する**（UE 4.24 の既定は Unitless、UE5 は Candelas。入れないと明るさが桁違いになる。Zone 1 の単位つきの灯は 419 個がすべて Candelas で、BP の天井灯 294 個もクラス既定が Candelas）。`LightGuid`・`MapBuildDataId` と `IESTexture`（光のプロファイルは書き出しに無い。病院で使うのは強さ 0 の救急車のスポットライト 6 個）は入れない。`MaxDrawDistance`・`MaxDistanceFadeRange` は入れる（動的に描く灯に効く。天井灯 294 個は 4000 cm で消える）。
- **Stationary と Movable の灯はエンジンが毎フレーム描く**。焼き込みに入るのは Stationary の灯の間接光（と影を落とす 3 個の影）だけで、直接光は描画のたび。ボリューメトリック フォグの灯の注入も描く灯が対象なので、**天井灯 294 個の `VolumetricScatteringIntensity` 20.0・シャードの灯 337 個の 2.5 は原作どおり効く**。（2026-09-16 の途中まで、Mobility の省略を UE の既定の Static と読んで 1,015 個の直接光まで焼いていた。そのときは `FLightSceneInfo::ShouldRenderLightViewIndependent`〈`LightSceneInfo.cpp:245`〉が焼いた Static の灯を描かないので、散乱の値は効いていなかった）
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
- `ue_props`: UE のプロパティ名 → Python 名（`CameraISO` → `camera_iso`、`bOverride_X` → `override_x`）、書き出しの値 → Python の値（辞書の Vector / Vector4 / Color / LinearColor、**`pak_reference_2` が色やベクトルに使う配列**〈`[183, 163, 145, 255]`〉、列挙）、構造体は中身だけを再帰的に入れる（`apply`）。**整数 4 つの配列の色（FColor）は [B, G, R, A] の順として読む**（エンジンは FColor を uint32 のまま書き〈`Color.h` の `Ar << DWColor()`〉、リトルエンディアンでバイトは B,G,R,A。書き出しの道具 `ue4.py` の `'Color': ('u8', 4)` はその順のまま出す。`pak_reference` も同じ）。浮動小数の配列（LinearColor）と、名前つきの辞書の色は並べ替えない。読めなかったものは `failures` に積む。**UE の版で名前が変わったプロパティは `RENAMED` で読み替える**（`FogInscatteringColor` → `FogInscatteringLuminance`、`DirectionalInscatteringColor` → `DirectionalInscatteringLuminance`。どちらも同じ LinearColor の改名なので値はそのまま）。

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
- **マテリアルのノードの既定テクスチャは、ノードのサンプラーの型と合わせる**。合わないとマスターのコンパイルが失敗し（`Sampler type is Linear Color, should be Masks for …`）、**そのマスターのインスタンスがすべて UE の `DefaultMaterial`（灰色の市松）で描かれる**。ログには `Failed to compile Material Instance with Base M_DD_Substance for platform PCD3D_SM6, Default Material will be used in game.` が出るだけで、組み立てもビルドも止まらない。2026-09-16 まで `T_DD_DefaultPacked` が `TC_Masks` だったため、**病院の `M_DD_Substance` 系のマテリアル（壁・床・金属など）はずっとこの状態で、それまでの PIE の絵と焼き込みはすべて市松のまま**だった。取り込みや組み立ての後は、ログに `Failed to compile Material` が無いことを確かめる。
- `editor_cycle.py` は起動時に `sys.stdout` / `sys.stderr` を `errors="replace"` にし直す。ビルドの出力にこの PC のコンソール（cp932）で出せない文字が混ざると、ビルドの失敗を報告する途中で `UnicodeEncodeError` になって落ちていた。
- **エディタの起動はセッションを跨ぐ**。Claude Code は Windows のセッション 0（サービス側）で動いており、そこには GPU の出力が無い（ログの `LogD3D12RHI: Adapter has … 0 output[s]`）ので、そのまま起動したエディタは D3D12 のスワップチェーンを作れず `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` で即落ちる。`start_editor()` は `ProcessIdToSessionId` と `WTSGetActiveConsoleSessionId` で自分のセッションとコンソールのセッションを比べ、違えば一度きりのスケジュールタスク（`WasamiLaunchEditor`。プリンシパルはログオン中のユーザーを **SID で**指定し、`LogonType Interactive`・`RunLevel Limited`）でログオン中のセッションに起動する。ユーザー名の形（ドメインなしの PC では `WORKGROUP` になる）では登録できないので SID を使う。タスクはエディタが応答したら消す（`drop_task`）。
- 起動の完了は**リモート実行が答えるか**で見る（`editor_answers`）。この PC では Docker Desktop が 127.0.0.1:8000 を掴んでいるため、MCP のポートに繋がってもエディタが起きているとは限らない。
- Zone 2 のポストプロセスボリュームの **`ColorGradingLUT` と `WeightedBlendables` は入れていない**。前者は書き出しがアセットのパスの文字列（`/Game/ThirdParty/Chameleon/LUTs/LUT_Classic8`）で、取り込んだテクスチャに解決する仕組みがまだない。後者はポストプロセスのマテリアル（`M_SharpenFilter_Inst`）で、マスターの式が cook で消えている。**このボリュームは `ColorGradingIntensity` が 0 なので、LUT の見た目への寄与は無い**。
- スカイライトのキューブマップ（`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`）は**回収できていない**。原作は TextureCube だが、書き出しは 512×512 の平面 PNG 1 面（float の階調は落ちている）で、UE に取り込むと Texture2D になる。いまはシーンのキャプチャに任せている。実測でも**寄与はほとんど無い**（Zone 1 の廊下で強度を 0 / 0.5 / 5 / 50 と振って撮ると、原作の 0.5 では平均が 0.05 も動かない。暗い屋内をキャプチャしているため）。同じアセットは Epic の StarterContent のものだが、この PC の UE 5.8 には入っていない（`Engine/Content/StarterContent` にテクスチャが 1 枚だけ、`FeaturePacks/` にも無い）。
- **焼き込みの警告は原作どおりなので直さない**（2026-09-16、High 品質で両ゾーンを焼いたとき。00 記録の「灯の焼き込み」）:
  - インポータンスボリュームが無い — 原作の Zone 1・Zone 2 にも `LightmassImportanceVolume` は無い（Zone 1 のボリュームは AudioVolume 2・BlockingVolume 7・NavMeshBoundsVolume 2・TriggerVolume 2、Zone 2 は BlockingVolume 10・NavMeshBoundsVolume 29・NavModifierVolume 30・PostProcessVolume 1）。
  - ライトマップ UV の重なり — Zone 1 は `hospital_bed_02` 1.1 %・`hospital_pill_sign` 28.6 %・`hospital_ambulance_new` 49.3 %・`hospital_zone_01_tiles_tile_tunnel` 97.1 %、Zone 2 は `hospital_electricChair_01` 33.1 %・`hospital_ambulance_new`（と `_complexcollision`）49.3 %・`hospital_zone_02_holdingCell_01_jail_door` 1.5 %・`hospital_zone_02_CCTVset` 66.7 %。**原作の UV のまま**出ている: 取り込みに使った glTF（`Intermediate/Pipeline/dd/meshes/`）の原作の `LightMapCoordinateIndex` の UV を、原作の `LightMapResolution` のテクセルに塗って重なりを数えると（UE とは数え方が違うので値は一致しない）、64 メッシュのうち重なりの多い上位 8 個のうち 7 個がこの 7 メッシュだった（残る `spike_brush_StaticMesh` はライトマップの値が無く、Zone 2 に Movable で置かれているので焼き込みに入らない）。`_complexcollision` も原作の Zone 2 で見えるメッシュとして置かれている（`hospital_ambulance_new9`）。原作の値を写す方針なので UV は作り直さない。
- **ボリューメトリック フォグの間接光**: 原作は焼き込みがあるので、フォグはボリュメトリック ライトマップ経由で `VolumetricFogStaticLightingScatteringIntensity`（Zone 1 は既定の 1.0）の分だけ焼かれた間接光を受け取る。本作も 2026-09-16 から焼いているので同じ経路がある（それより前は何も焼かず、Lumen はフォグに同じようには寄与しなかった）。
- 原作の cook されたデータは、既定値と同じプロパティを持たない。ポストプロセスの override が立っていて値が無いのは「既定値で上書き」の意味。
- MCP のポートは Docker Desktop と衝突しうる（`.claude/guides/unreal-workflow.md`）。MCP が使えないときは `Tools/ue_remote.py` で作業できる。

## 見た目を撮る（`WasamiDevTools.capture_pose`）

エディタのビューポートを通さずに、一時的な `SceneCapture2D`（`SCS_FINAL_COLOR_LDR`）でレベルを PNG に描く。

- **エディタの窓が前面でなくても撮れる**のが要点。`HighResShot` と `AutomationLibrary.take_high_res_screenshot` は、エディタが前面でないとき（`GetActiveViewport()` が無い）**何も言わずに要求を捨てて、ファイルを書かない**。Claude はセッション 0 にいて窓を前に出せないので、この 2 つは使えない。
- `SCS_FINAL_COLOR_LDR` なので、露出・ブルーム・トーンマッパーはゲームと同じものが掛かる（露出の比較に使える。00 記録の「露出」）。
- 視点の高さは、プレイヤーの目が capsule の中心 +95 cm なので、`PlayerStart` の z 92 に対して **187**。ヨー 0 は +X 方向。FOV は本家の静止時の 90。
- 落とし穴: `unreal.Rotator` の引数の順は `(roll, pitch, yaw)`。位置引数で `(pitch, yaw, roll)` のつもりで渡すと、ヨーのつもりの値がピッチになる（天井や床を向いた絵が撮れる）。
- **絶対の明るさの比較には使えない**。`SceneCapture2D` は Lumen の間接光を本編と同じようには回さず、同じ視点でも PIE の絵より暗く出る（Zone 1 の廊下で中央値 (13,13,0) 対 PIE の (41,38,25)。00 記録の「露出」）。同じ視点で**設定 A と設定 B を比べる**のには使える。**本家の実機と数値を突き合わせるときは PIE の `HighResShot 1280x720`** で撮る（PIE 中なら、エディタが前面でなくても `Saved/Screenshots/WindowsEditor/` に書かれる）。
- PIE の中で使う相手は `UnrealEditorSubsystem.get_game_world()`。`get_editor_world()` は PIE 中もエディタのワールドを返すので、この道具は PIE の絵を撮れない。

## 変更履歴
- 2026-09-16: 焼き込みの警告（インポータンスボリュームが無い・ライトマップ UV の重なり）がどちらも原作どおりであることを「既知の制約・注意点」に書いた（ソースは変えていない）
- 2026-09-16: `apply_texture_settings` が、UE が sRGB を切る圧縮（HDR など）で sRGB を求めないようにした（HDR の空 2 枚が `refresh_settings` のたびに変わったと数えられていた）
- 2026-09-16: ライトマップの解像度と UV の番号を原作のメッシュの値から取るようにし（`Export.lightmap`）、UV1 を持たないメッシュにも UE の展開を作らせないようにした（`setup_lightmap`。`refresh_settings` が既存のメッシュにもかけ直す）。それまでは解像度を表面積から決め（最大 2048）、番号を 1 に決め打ちし、結合メッシュに UV を作らせていたため、原作では 1 点の値だったステージ本体の間接光が面ごとに焼かれ、壁と床が実機より明るかった。表面積の計測（`gltf_surface`・`areaM2`・`lightmapUvUsed`）は使わなくなったので外した
- 2026-09-16: 配置の `bCastShadowAsTwoSided` を取り込むようにした（Zone 1 のステージ本体 5 個。無いと片面の天井が平行光源の影にならず、焼き込みでも光が壁を抜けていた）。Zone 1 を組み立て直した
- 2026-09-16: 灯・メッシュ・スカイライトの Mobility の省略を、UE の既定の Static ではなく**部品の土台の値**として読むようにした（`Export.default_mobility`）。原作の灯の大半は Stationary で、BP の構築スクリプトの灯（シャードなど）は Movable。それまでは Zone 1 の 1,015 個を Static として直接光まで焼いており、床と壁が実機の約 2 倍明るかった。あわせて、灯の値を `full.json` から取り（`SoftSourceRadius`・`LightingChannels`・`MaxDrawDistance` などが落ちていた）、BP の灯はレベル側に値があってもクラス既定を土台にし、`parent_class` を `super` から取るようにし、`MaxDrawDistance`/`MaxDistanceFadeRange` を入れるようにした。両ゾーンを組み立て直した
- 2026-09-16: 書き出しの FColor（整数 4 つの配列）を [B, G, R, A] として読むようにした（`ue_props.value`）。それまでは R と B を取り違えていて、Zone 1 の天井灯 294 個が本来の (200, 251, 255) の青白い光ではなく (255, 251, 200) の黄色に、扉枠の灯 234 個が本来の (255, 57, 74) の赤ではなく (74, 57, 255) の青になっていた（実機の開始地点の床の色の比と、廊下の扉枠の赤い光で確かめた）。両ゾーンのレベルを組み立て直した
- 2026-09-16: `T_DD_DefaultPacked` を `TC_Masks` から `TC_Default`・リニアに直した（Packed のノードは `LINEAR_COLOR` なので型が合わず、`M_DD_Substance` のコンパイルが失敗して、インスタンスがすべて既定のマテリアルの市松で描かれていた）。`ensure_default_packed` が既存のアセットも直し、`ensure_masters` がそのときマスターを再コンパイルするようにした
- 2026-09-16: 原作で焼かれていた（Static の）灯の `VolumetricScatteringIntensity` を 0 にするようにした。原作では効いていなかった値がそのまま効いて、Zone 1 の画面が暖色のもやに覆われていた（切り分けは `capture_pose` の A/B。霧を切ると平均輝度が 18.7 → 9.9 になり、もやが霧由来と分かった）
- 2026-09-16: `WasamiDevTools.capture_pose` を足した（エディタが前面でなくても見た目を撮れるようにするため）
- 2026-09-16: 画面操作の道具（`Tools/desktop.py` と `Tools/desktop_agent.py`）を足した。Claude はセッション 0 にいてセッション 1 の画面を触れないので、セッション 1 に常駐するエージェントとファイル経由でやり取りする（ユーザーの指示で「画面操作も Claude が行う」に変更）
- 2026-09-16: `Tools/console_session.py` を足した（`editor_cycle.py` の対話デスクトップでの起動を、任意のプログラムに使える形にしたもの。手元の本家のゲームの起動に使う）
- 2026-09-16: `editor_cycle.py` がエディタを対話デスクトップ（コンソールのセッション）でスケジュールタスク経由で起動するようにし、起動の判定を MCP のポートからリモート実行の応答に変えた
- 2026-09-16: タブレットの素材の取り込み（`pipeline/dd_tablet.py`、`WasamiDDTools.import_dd_tablet`）と、レベルにミニマップの地図の板を置く `_map_plane` を足した。`editor_cycle.py` の出力の文字化けで落ちる問題を直した
- 2026-09-16: 病院の取り込み（`pipeline/dd_stage.py`、マスターマテリアル 3 種）と組み立て（`pipeline/dd_level.py`）、それを呼ぶ `WasamiStageTools` を足した。`ue_props` が配列の色・ベクトルを読めるようにした
- 2026-09-16: ステージを本家の病院へ差し替える方針変更にともない、CC2 の前処理（`Tools/cc2/prepare_stage.py`）・取り込み（`pipeline/cc2_assets.py`）・組み立て（`pipeline/cc2_level.py`）・ツールセット（`toolsets/stage.py` の `WasamiStageTools`）を削除し、`paths.py` から CC2 の定数を外して `DD_PAK2`（`pak_reference_2`）を足した
- 2026-09-16: 初版（取り込みと組み立ての現行実装を記録）
