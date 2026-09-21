---
title: 全体像（プロジェクトの構成・設定・モジュール）
sources:
  - wasami_deception.uproject
  - .mcp.json
  - Config/DefaultEngine.ini
  - Config/DefaultGame.ini
  - Config/DefaultInput.ini
  - Config/DefaultEditor.ini
  - Config/DefaultEditorPerProjectUserSettings.ini
  - Source/wasami_deception.Target.cs
  - Source/wasami_deceptionEditor.Target.cs
  - Source/wasami_deception/wasami_deception.Build.cs
  - Source/wasami_deception/wasami_deception.cpp
  - Source/wasami_deception/wasami_deception.h
  - Source/wasami_deception/WasamiAssets.h
  - Tools/mac_build.sh
updated: 2026-09-22
---

# 全体像

## 役割
Dark Deception のワサミ版ファンゲームの UE 5.8.2 版。ステージ（本家の病院「Torment Therapy」の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`）もプレイヤーもゲームの仕組みも、本家 Dark Deception の値に倣う（`.claude/guides/original-fidelity.md`）。この記録はプロジェクトの骨格（モジュール、設定、参照データ、作業の流れ）だけを書く。仕組みごとの詳細は各記録（`_index.md`）を参照。

## 構成

| 場所 | 中身 |
| --- | --- |
| `Source/wasami_deception/` | ゲームの C++ モジュール（`Runtime`、`LoadingPhase: Default`）。プレイヤーとゲームモードは 02 記録、タブレットの画面は 03 記録、パワーは 04 記録、ゲームの流れの画面（死亡画面）は 09 記録 |
| `Content/Python/` | エディタの Python。`init_unreal.py` が `wasami_tools` のツールセットを ToolsetRegistry に登録し、MCP から呼べるようにする（01 記録） |
| `Tools/` | エディタの外で動かすスクリプト（参照データの前処理、リモート実行、エディタの開き直し。01 記録） |
| `Intermediate/Pipeline/` | 前処理の出力（git の対象外、作り直せる） |
| `pak_reference/`、`pak_reference_2/`、`cc2_reference/` | 原作データ（git の対象外。読み取り専用） |
| `.claude/` | 運用ルール・参考資料・進捗記録・実装記録 |

## モジュールとビルド

- `wasami_deception.Build.cs` の公開依存: `Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`UMG`（タブレットの画面。03 記録）、`LevelSequence`・`MovieScene`（ゾーンの流れがレベルのシーケンスを再生する。11 記録）、`AIModule`・`GameplayTasks`・`NavigationSystem`（敵の AI MoveTo とランダムの点。07 記録）。非公開依存: `Slate`、`SlateCore`、`EngineSettings`（タイトルの版の文字がプロジェクト設定の `ProjectVersion` を読む。14 記録）、`AnimationCore`（Matron の LookAt の `SolveAim`。17 記録）。
- ターゲット: `wasami_deception.Target.cs`（Game）と `wasami_deceptionEditor.Target.cs`（Editor）。どちらも `BuildSettingsVersion.V7`、`IncludeOrderVersion.Unreal5_8`（テンプレートのまま）。
- `wasami_deception.cpp` / `.h` はモジュールの実装（`IMPLEMENT_PRIMARY_GAME_MODULE`）。
- **パイプラインが作るアセット（`/Game/DD`・`/Game/Pipeline`）の参照の決まり**（`WasamiAssets.h`）: C++ はそれらをソフト参照で持ち（`TSoftObjectPtr` / `TSoftClassPtr` の UPROPERTY に、`WasamiAssets::Path("/Game/…/Name")`〈→ `/Game/…/Name.Name`〉や `WasamiAssets::ClassPath`〈→ `…/BP_Name.BP_Name_C`〉で既定のパスを入れる）、使うとき（`BeginPlay`・`RebuildWidget`）に `LoadSynchronous` で読む。`ConstructorHelpers` で読むとエディタの起動時の読み込みでルートに入り、パイプラインが作り直そうとするとエディタが落ちる（01 記録の注意点）。
- Automation テストは `Source/wasami_deception/Tests/`（名前は `Wasami.*`）。モジュールのヘッダーは `../` で読む（`Tests/` はモジュールの include パスに入っていない）。
- ビルドと開き直しの手順は `.claude/guides/unreal-workflow.md`、自動化は `Tools/editor_cycle.py`（01 記録）。

## プラグイン（`wasami_deception.uproject`）

| プラグイン | 目的 |
| --- | --- |
| `ModelingToolsEditorMode` | テンプレートの既定（エディタのみ） |
| `ModelContextProtocol` | MCP サーバー（エディタを Claude から操作する） |
| `AllToolsets` | エンジン同梱のツールセット一式（アクタ・アセット・マテリアル・シーケンサなど） |
| `LiveCodingToolset` | MCP から Live Coding のコンパイルを走らせる |

道具の 3 つ（`ModelContextProtocol`・`AllToolsets`・`LiveCodingToolset`）は `ModelingToolsEditorMode` と同じく `"TargetAllowList": ["Editor"]` を付けてある（2026-09-20）。`ModelContextProtocol` は Runtime のモジュールを 2 つ持つので、付けないと製品のビルドに MCP が同梱される。ゲームのモジュールはどれにも依存していないので、製品から外して困らない。

`.mcp.json` は Claude Code の接続先（`http://127.0.0.1:8000/mcp`、HTTP）。ポートの衝突については `.claude/guides/unreal-workflow.md`。

## 設定（`Config/`）

- **`DefaultEngine.ini`**
  - `[/Script/EngineSettings.GameMapsSettings]`: `GameDefaultMap`（パッケージしたゲームが最初に開くマップ）はタイトルのレベル `/Game/Stage/Maps/L_Title`（本家もタイトルは別のレベル `TitleScreen`。ゲームモードはレベルの World Settings の `AWasamiTitleGameMode`。14 記録）、`EditorStartupMap` は `/Game/Stage/Maps/L_Hospital_Zone1`（開発で開くのはゾーン）、`GlobalDefaultGameMode` は `/Script/wasami_deception.WasamiGameMode`、`GameInstanceClass` は `/Script/wasami_deception.WasamiGameInstance`（ライフと回収済みのシャードの記憶。06 記録）。
  - `[/Script/Engine.RendererSettings]`: **静的ライティング有効**（`r.AllowStaticLighting=True`）、仮想シャドウマップ有効、**メッシュ距離フィールドは作らない**（`r.GenerateMeshDistanceFields=False`）、**動的 GI なし**（`r.DynamicGlobalIlluminationMethod=0`）、**反射は SSR**（`r.ReflectionMethod=2`）、Substrate 有効、**`r.RayTracing=False`**（この PC の GeForce GTX 1660 SUPER に RT コアが無い）。最初の 4 つは 2026-09-16 に Lumen から切り替えたもの。理由は下の「灯の焼き込み」。
  - `[/Script/Engine.LocalPlayer]`: `AspectRatioAxisConstraint=AspectRatio_MaintainXFOV`（2026-09-16）。カメラの FOV 90 は原作では**水平**（UE 4.24 のエンジン既定が `MaintainXFOV` で、原作のプロジェクト設定は上書きしていない）。UE 5 の既定は `MaintainYFOV` に変わっており、そのままだとこの PC の 21:9（3440x1440）で水平 107° になって何もかも小さく写る。16:9 ではどちらでも同じ。
  - `[/Script/Engine.RendererSettings]` の**露出**（2026-09-16）: 原作のプロジェクト設定をそのまま写した。`r.DefaultFeature.AutoExposure=False`・`.Method=0`・`.ExtendDefaultLuminanceRange=False`・`.Bias=0.0`、`r.DefaultFeature.LensFlare=False`、`r.DefaultFeature.LightUnits=1`。UE5 だけの**ローカル露出**は無効値の 1.0 にする（`r.DefaultFeature.LocalExposure.HighlightContrastScale` / `.ShadowContrastScale`。新規プロジェクトの既定 0.8 は原作に無い階調補正になる）。原作の `r.UsePreExposure=False`（プレエクスポージャ無し）は、代わりに `r.EyeAdaptation.PreExposureOverride=1`（プレエクスポージャを 1.0 に固定）で写す（2026-09-16、ユーザーの決定。下の「既知の制約・注意点」）。根拠と効果は下の「露出」。
  - `[/Script/WindowsTargetPlatform.WindowsTargetSettings]`: DX12 / SM6、音声 48 kHz。
  - `[/Script/Engine.CollisionProfile]`（2026-09-16）: 独自のオブジェクトチャンネル **`Teleport`**（`ECC_GameTraceChannel1`、既定の応答 **Overlap**）。本家の旧版 `DefaultEngine.ini` の値（最新版は既定 Ignore。テレポーテーションは旧版に従う）。テレポートの照準が病院のゾーンをこのチャンネルで探す（04 記録）。旧版のもう 1 つの `Malak`（`ECC_GameTraceChannel2`、Block）は別の章の敵のものなので写していない。チャンネルの設定はエディタの起動時に読まれる。
  - `[/Script/NavigationSystem.RecastNavMesh]` と `[/Script/NavigationSystem.NavigationSystemV1]`（2026-09-19）: 本家の最新版の `DefaultEngine.ini` のナビの設定を写した。`RuntimeGeneration=DynamicModifiersOnly`（道は読み込みのときに作り、動く修飾子だけを実行中に直す）・`bForceRebuildOnLoad=True`（エディタはレベルを読むたびに作り直す。ゲームは `DynamicModifiersOnly` なので形からは作らず、保存した道をそのまま使う。01 記録の「ナビゲーション」）・`bFixedTilePoolSize=True`・`ObservedPathsTickInterval=1`・`bAutoDestroyWhenNoNavigation=False`、セルの大きさ 10 cm（UE 5 はタイルの解像度ごとに持つので `NavMeshResolutionParams[0..2]` の 3 つとも `CellSize=10`。`CellHeight` 10・`AgentMaxStepHeight` 35 は UE 4.24 の既定で、UE 5 が古い NavMesh から埋めるのと同じ）、エージェント `Default` 1 つ（半径 40・高さ 144・`DefaultQueryExtent` (50, 50, 250)。本家のレベルの `RecastNavMesh-Default` も `AgentRadius` 40・`AgentMaxHeight` 144。UE 5 は `AgentMaxHeight` を `AgentHeight` と呼び、エージェントから入れる）、`bSkipAgentHeightCheckWhenPickingNavData=True`。探索の上限 `DefaultMaxSearchNodes` は両方とも既定の 2048 のままで、セル 10 cm では約 20 m より遠い 2 点の道が途中までになることがある（本家も同じ。01 記録の「ナビゲーション」）。設定はエディタの起動時に読まれる。
  - `[SystemSettings]`（2026-09-21）: `fx.Cascade.UseVelocityForMotionBlur=0`。原作（UE 4.21・4.24）にはこの切り替えが無く、Required モジュールの `bUseVelocityForMotionBlur` は原作のどのアセットにも立っていない（＝すべて false）。UE 5 は既定を CVar `fx.Cascade.UseVelocityForMotionBlur`（既定 true）に移し、モジュールが`bOverrideUseVelocityForMotionBlur` で上書きしないときはそちらを使う（`UParticleModuleRequired::ShouldUseVelocityForMotionBlur`）ので、GPU のエミッタの `ResourceData`・`EmitterInfo` が原作の cook と食い違っていた。本作の Cascade はすべて原作のものなので、モジュールごとに上書きせずここで原作の既定に戻す。CVar はエンジンの起動時に読まれ、`UParticleModuleTypeDataGpu::Build`（エディタでの読み込みごと）がその値を焼き込む。
  - `[/Script/PythonScriptPlugin.PythonScriptPluginSettings]`: `bRemoteExecution=True`（`Tools/ue_remote.py` が使う。ローカルのマルチキャストのみ）、`bDeveloperMode=True`（`Intermediate/PythonStub/unreal.py` が出る）。
- **`DefaultEditorPerProjectUserSettings.ini`**: MCP サーバーの設定（`ServerUrlPath=/mcp`、`ServerPortNumber=8000`、`bAutoStartServer=True`、`bEnableToolSearch=True`）。
- **`DefaultInput.ini`**: テンプレートのまま。Enhanced Input（`DefaultPlayerInputClass=EnhancedPlayerInput`、`DefaultInputComponentClass=EnhancedInputComponent`）、`bEnableLegacyInputScales=True`（本家と同じ 2.5 / −2.5 の視点の倍率が掛かる。02 記録）、`bEnableMouseSmoothing=True`、`FOVScale=0.011110`。
- **`DefaultGame.ini`**: CommonUI の設定とプロジェクト ID に、`[/Script/EngineSettings.GeneralProjectSettings]` の `ProjectVersion=1.0.0`（タイトルの右上の版の文字。2026-09-20 のユーザーの回答。14 記録）。
  - **`[/Script/UnrealEd.ProjectPackagingSettings]` は `bCookAll=True` だけを置く**（2026-09-21 に修正。それまでは節ごと置かない方針だった）。UE 5.8 のクックは `CollectFilesToCook` の終わりの `if (bCookAll || (bCookAllByDefault && NumFilesAddedByCommandLineOrGameCallback == 0))` で「`/Game` を全部入れる」経路に入るが、**この数え上げはコマンドラインの指定・`AlwaysCookMaps`・アセットマネージャーやプラグインの `ModifyCook` など複数の経路で増える**ので 0 のままである保証が無い。2026-09-21 の初回のパッケージでは実際に増えて、**`/Game` のパッケージが `L_Title` の 1 つしか入らない**パッケージがエラーも警告も無しに出来た（全 501 パッケージ・494 クック。症状索引の「パッケージは `BUILD SUCCESSFUL` なのに、中身に `/Game` のアセットが 1 つ（`L_Title`）しか入っていない」）。`bCookAll=True` は条件を飛ばして必ず全部なめるので影響を受けない。本作は **C++ が `/Game` のパスを直に名指しして読むアセットが 237 個**あり、そのうち 195 個はどのマップからも参照されていない（アセットレジストリから辿れない）ので、Project Settings の「Maps to Cook」「Directories to Always Cook」を埋めると全部入れる経路が「書いた分だけ」に変わり、195 個が黙って落ちる（だから `bCookAll` だけを立てる）。`/Game` の 1134 パッケージのうち、3 つのマップか C++ の名指しから辿れるのは 1106（残り 28。2026-09-20 に数えた）。その 28 のうち **使っていない原作の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy` と `Pipeline/Debug` の検証用の材質 `M_Probe_*` 17 個は 2026-09-21 に消した**（2026-09-21 のユーザーの回答。作業一覧の項目 35。どちらも git の管理外で、参照は 0 件。空になった `/Game/Pipeline/Debug` のフォルダーごと消した。作り直す経路は無い: `dd_ui.py` の取り込みの一覧に題字はもう無く、`Tools/dd/prepare_level_title.py` は `/Game` ではなく `pak_reference_2` の PNG を読む。`M_Probe_*` を作る Python も無い〈検証のときに MCP で直に作ったもの〉）。残る 10 個はタブレットの地図の印 4 つ・サードパーティの HDRI と火花 3 つ・取り込みのプリセット 3 つ。
  - **`[/Script/Engine.AssetManagerSettings]` に `GameFeatureData` の規則を 1 つ置く**（2026-09-21。作業一覧の項目 36 のステップ 2。理由はファイルにも書いた）。クックはエディタのコマンドレットで走るので、MCP のツールセット（`AllToolsets` → `GameFeaturesToolset`。どちらも Editor ターゲット限定）が連れてくる `GameFeatures` プラグインも読み込まれ、`GameFeatureData` 型の `FPrimaryAssetRules` が既定のままだと起動時にエラーを 1 件出す（UE 5.8 `GameFeaturesSubsystem.cpp` の `OnAssetManagerCreated`）。コマンドレットはエラーが 1 件でもログに出ると失敗を返すので、クックの中身が正しくても UAT が `ExitCode=25 (Error_UnknownCookFailure)` で落ちる。規則は `Priority=1`（既定の `-1` から変えて `IsDefault()` を偽にするだけが目的）・`Directories` 空・`CookRule=Unknown` なのでクックの中身は変わらず、`bIsEditorOnly=True` なのでパッケージした本編は `AssetManager.cpp` の `ShouldScanPrimaryAssetType` で項目ごと読み飛ばす（本編に `GameFeatures` モジュールは入らない）。UAT の `-IgnoreCookErrors` は本物のエラーまで黙らせるので採らない。
- **`DefaultEditor.ini`**: テンプレートのまま。

## 露出（2026-09-16）

**原作の露出はポストプロセスボリュームではなくプロジェクト設定で決まっている。** 根拠:

- 原作の `06_Hospital_Zone_01` に `PostProcessVolume` は **0 個**（`pak_reference_2/_levels/06_Hospital_Zone_01.scene.json` の `counts`）。Zone 2 と入口 `06_Hospital` には 1 個ずつあるが、どちらも `bOverride_AutoExposure*` を持たない（色補正・ブルーム・AO・DOF・シャープンだけ）。
- `pak_reference_2/_raw/DDeception/Config/DefaultEngine.ini`（UE 4.24）の `[/Script/Engine.RendererSettings]` に `r.DefaultFeature.AutoExposure=False`・`.Method=0`・`.ExtendDefaultLuminanceRange=False`・`r.UsePreExposure=False`・`r.DefaultFeature.MotionBlur=False`・`r.DefaultFeature.LensFlare=False`・`r.DefaultFeature.LightUnits=1`。
- UE 5.8 でも同じ cvar が効く。`r.DefaultFeature.AutoExposure=0` は `AutoExposureMinBrightness` と `MaxBrightness` を 1 にする（`Engine/Source/Runtime/Engine/Private/SceneView.cpp:2060`）。旧レンジでは `MinWhitePointLuminance = MaxWhitePointLuminance = 1` になり（`PostProcessEyeAdaptation.cpp:665`）、露出 = `2^AutoExposureBias` に固定される。
- UE 4.24 の `AutoExposureBias` の既定は **0.0**。原作に入っている第三者プラグイン 2 つ（`ThirdParty/Chameleon/Chameleon`、`ThirdParty/LightProbes/Blueprints/Light_ProbeController`）が、上書きしていない `FPostProcessSettings` をそのまま持っており、どちらも `AutoExposureBias 0.0`・`LowPercent 80`・`HighPercent 98.3`・`MinBrightness 0.03`・`MaxBrightness 2.0` と同じ値を示す。UE 5 の既定は `r.DefaultFeature.AutoExposure.Bias` が 1.0（= 2 倍明るい）なので、**0.0 を明示する**。
- 灯の単位は取り込みのままでよい。`r.DefaultFeature.LightUnits` はエディタで灯を置く工場（`ActorFactoryPointLight.cpp:23` ほか）でしか読まれず、`ULocalLightComponent` の CDO の既定は `Unitless`（`LocalLightComponent.cpp:14`）。原作の Zone 1 の灯のうち単位を書き出している 419 個はすべて Candelas で、BP の天井灯 294 個もクラス既定が Candelas（書き出しをそのまま入れる）。単位の無い灯は UE 4.24 の既定の Unitless で、`dd_level.py` の `DEFAULT_LIGHT_UNITS` がそれを明示する（2026-09-16 に「1,121 個すべて単位なし」と書いていたのは誤り）。

効果（同じ場所・同じ向き〈Zone 1 の廊下 (1801, −9601)・ヨー 90、目の高さ 187〉の 1280 × 720。画像は `observations/ours/`）:

| 撮り方 | 設定 | 画面全体の中央値 RGB | 平均輝度 | 上位 1 % |
| --- | --- | --- | --- | --- |
| `capture_pose`（`zone1-corridor-before.png`） | UE5 の既定（自動露出 ON・EV100・Bias 1.0・ローカル露出 0.8） | (175, 170, 140) | 165.7 | 245.5 |
| `capture_pose`（`zone1-corridor-after.png`） | 原作の設定 | (13, 13, 0) | 20.9 | 194.1 |
| **PIE の `HighResShot`**（`zone1-corridor-pie.png`） | 原作の設定 | **(41, 38, 25)** | 45.3 | 193.2（最大 231.0） |
| 参考: 本家の実機の廊下（別の廊下。`observations/README.md`） | — | **(44, 38, 39)** | — | 天井の灯 227〜236 |

**露出が原作どおりになったことの決め手はタブレット**（`zone1-corridor-pie-tablet.png`）。タブレットの画面は UMG の決まった色なので、灯の実装の差が混ざらず露出だけを映す:

| | 直す前 | 直した後 | 本家の実機 |
| --- | --- | --- | --- |
| 上の帯の地 | 162 | **(34, 34, 33)** | (32, 32, 31) |
| 地図の地（黒） | (4, 3, 2) | **(0, 0, 0)** | (0, 0, 0) |

ステージ側も、明るさは原作とほぼ同じところに来た（中央値 R 41 / G 38 に対し原作 44 / 38、いちばん明るい面 231 に対し原作 227〜236）。**残っている差は色温度**: 青が 25 に対し原作は 39 で、本作のほうが黄緑に寄っている。灯の色・スカイライト・反射キャプチャ（書き出しが平面 PNG でキューブマップを戻せない）のどれかで、露出とは別の課題（M1 の残り）。

**`capture_pose` の絵は絶対の明るさの比較には使えない**（上の表で PIE の (41,38,25) に対し (13,13,0) と暗い）。`SceneCapture2D` は Lumen の間接光を本編と同じようには回さないため。**同じ視点で設定 A と設定 B を比べる用途には使える**（暗くなる度合いは同じ）が、本家の実機と数値を突き合わせるときは **PIE の `HighResShot`** で撮る。

## 灯の焼き込み（2026-09-16）

**原作はライティングを焼いている。本作も同じようにする。** Lumen ではこのステージの間接光は出せない。

原作が焼いている証拠:
- 原作の `DefaultEngine.ini`（UE 4.24）は `r.AllowStaticLighting=True`・`r.GenerateMeshDistanceFields=False`・`r.PrecomputedVisibilityWarning=False`。
- `06_Hospital_Zone_01` の StaticMeshComponent 963 個が `VisibilityId` を持つ（プリコンピューテッド ビジビリティはライティングビルドで作られる）。PointLightComponent 665 個が `LightGuid` を持つ。
- Zone 1 の灯 1,120 個のうち 669 個が Stationary（書き出しに `Mobility` が無い＝土台の `APointLight` などの既定。01 記録）。Stationary の灯は間接光と影を焼き、直接光は毎フレーム描く。
- 書き出しには原作のライトマップそのものは入っていない（`_manifest.json` の `not_recovered`: "Baked lighting (MapBuildDataRegistry lightmaps are not exported)"）ので、焼き直す。

**Lumen が使えない理由**（2026-09-16 に実測）: ステージの本体は結合された巨大なメッシュで（`hospital_zone_01_tiles_tile_tunnel` は 270 m、`tiles_tile_01` は 138 m）、
この PC の GPU に RT コアが無いので Lumen はソフトウェアのレイトレースになる。メッシュの距離フィールドは 1 メッシュ 256 ボクセルが上限（`r.DistanceFields.MaxPerMeshResolution`）なので、
**1 ボクセルが 50 cm を超えて**壁も廊下も潰れ、トレースが何も当たらない（距離フィールドのアトラスは 8 MB しか使われていなかった）。
PIE で `r.Lumen.DiffuseIndirect.Allow` を 1 → 0 にしても画面の平均輝度が 18.8 → 16.2 と動くだけで、**間接光は実質ゼロだった**。

本作の焼き方（取り込みは 01 記録）:
- `Config/DefaultEngine.ini`: `r.AllowStaticLighting=True`、`r.GenerateMeshDistanceFields=False`、`r.DynamicGlobalIlluminationMethod=0`、`r.ReflectionMethod=2`（SSR + レベルの反射キャプチャ 10 個。原作と同じ構成）。
- 灯・メッシュ・スカイライトの `Mobility` は書き出しの値、無ければ土台の値（Zone 1 は灯が Stationary 669・Movable 451、スカイライトが Stationary、メッシュが Static 916・Movable 7）。**2026-09-16 の途中までは省略を Static と読み、1,015 個の灯の直接光まで焼いていた**（下の表の「灯の色も直した後」の列までがその状態）。
- ライトマップの解像度と UV の番号は**原作のメッシュの値をそのまま写す**（ほとんど 64。01 記録の `setup_lightmap`）。UV は UE に作らせない。結合されたステージのメッシュ（Zone 1 は `tiles_tile_01/02/03`）は原作でも UV1 が全頂点 0 なので、間接光と静的な影がメッシュ全体で 1 点の値になる。**2026-09-16 の途中までは**解像度を表面積から 1 テクセル 20 cm（最大 2048）で決め、結合メッシュに UE の UV を作らせていた（下の表の「Stationary・両面の影」の列までがその状態）。
- ビルドは**原作と同じ High 品質**（原作の `06_Hospital_Zone_01_BuiltData` の `LevelLightingQuality` が `Quality_High`）で、`LevelEditorSubsystem.build_light_maps(QUALITY_HIGH, True)` → レベルと未保存のパッケージを保存する（手順のスクリプトは git の外の `observations/tools/bake_level.py`）。**Zone 1 で 105.7 秒**（うち Lightmass 70 秒、`L_Hospital_Zone1_BuiltData` 35.9 MB）、**Zone 2 で 48.0 秒**（`L_Hospital_Zone2_BuiltData` 20.6 MB）。Swarm と UnrealLightmass が動く（RAM 3.4 GB ほど）。Preview 品質なら Zone 1 は 32 秒（35.0 MB。原作の解像度に揃える前は 80〜135 秒・90 MB）。
- 焼き込みの警告は 2 種類だけで、どちらも原作どおりなので直さない: インポータンスボリュームが無い（原作の Zone 1・Zone 2 にも `LightmassImportanceVolume` は無い）、ライトマップ UV の重なり（両ゾーンで 7 メッシュ。原作の UV のまま。01 記録の「ライトマップ」）。

結果（実機の開始地点 `04_Start` と同じ視点・同じ 3440x1440 の PIE で比べたもの。画像は `observations/`、領域は `observations/README.md`）:

**注意: 2026-09-16 の途中まで、病院の壁・床のマテリアルはコンパイルに失敗して既定のマテリアル（灰色の市松）で描かれており、灯の色も R と B が入れ替わっていた（どちらも 01 記録）。** 下の表の「直す前」の 2 列はその状態のもの。

| 面 | 焼いた後・直す前（`zone1-start-pie-baked.png`） | マテリアルを直した後（`…-fixedmat.png`） | 灯の色も直した後（`…-fixedcolor.png`） | Stationary・両面の影（`…-twosided-baked.png`） | 原作の解像度と UV（`…-origlm.png`、Preview） | **同じ・High 品質**（`…-origlm-high.png`、現状） | 同じ・扉の位置をふさぐ（`…-origlm-high-doorblock.png`） | 実機 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 画面全体の中央値 | (23, 22, 14) | (78, 71, 55) | (54, 68, 72) | (69, 91, 93) | (40, 56, 58) | **(40, 56, 58)** | (37, 52, 54) | (34, 45, 47) |
| 平均輝度 | 32.2 | 89.8 | 84.0 | 96.8 | 68.4 | **68.7** | 67.1 | 58.1 |
| 床（手前中央、平均輝度） | — | — | 135.7 | 114.4 | 75.5 | **76.7** | 71.8 | 57.4 |
| 床（中ほど、平均輝度） | — | — | 144.3 | 134.0 | 95.2 | **97.2** | **52.0** | 49.0 |
| 右の壁（平均輝度） | — | — | 69.7 | 68.3 | 38.1 | **37.7** | 34.6 | 31.9 |
| 天井の中央 | (59, 58, 39) | (183, 179, 145) | (151, 178, 177) | (189, 209, 209) | (182, 204, 205) | **(182, 204, 204)** | (182, 204, 204) | (160, 187, 187) |
| 天井灯の面 | (49, 63, 65) | (127, 125, 107) | (94, 122, 127) | (121, 150, 153) | (99, 129, 133) | **(98, 128, 132)** | (98, 128, 132) | (95, 125, 130) |
| エレベーターの壁（右） | (18, 17, 10) | (62, 59, 44) | (42, 56, 58) | (68, 92, 93) | (43, 63, 65) | **(43, 63, 65)** | (43, 64, 65) | (38, 49, 50) |

- 灯の色を直して**色相が実機と合い**、原作のライトマップの解像度と UV に揃えて**明るさもほぼ合った**。それまで床と壁が実機の約 2 倍だったのは、本作だけステージ本体の間接光を面ごとに焼いていたため（「Stationary・両面の影」の列で `ShowFlag.GlobalIllumination 0` にすると、壁とエレベーターの壁が実機とほぼ同じ値になった）。
- 床の中ほどの残りの差は、実機で正面をふさいでいる赤い両開き扉（BP 由来で本作にはまだ無い）が無く、光沢のある床に明るい廊下の奥が映っているため。PIE の中だけで別のエレベーターの扉を扉の位置へ動かすと 97.2 → 52.0（実機 49.0。Preview のときは 95.2 → 52.6）になった。扉には実機と同じ丸いスポットライトが当たる。
- まだ残る差は、床の手前（72 対 57）・エレベーターの壁（左 56 対 44、右 68 対 59）・天井の中央（194 対 183）で、どれも 1〜2 割明るい。**焼き込みの品質が原因ではない**（Preview から原作と同じ High にしても、どの領域も平均輝度が ±2 以内しか動かなかった）。
- 性能（2026-09-16、この状態の開始地点。`observations/tools/pie_frametime.py`、`Use Less CPU when in Background` を切って測定）: PIE のフレーム時間は平均 12.7 ms（78.5 fps）・中央値 11.9 ms・95 パーセンタイル 16.8 ms・最大 41 ms。VRAM は PIE 中 5.1 GB / 6 GB（エディタだけなら 3.7 GB）で、`.claude/guides/performance.md` の目安（5 GB 程度まで）の上限にある。

## 性能（2026-09-20、作業一覧の項目 21 のステップ 5・6）

**測り方**は `.claude/guides/performance.md` の「測り方」と道具 `Tools/perf_probe.py`（01 記録）。画面の `stat` は読まず、エンジンの CSV プロファイラ（`csvprofile start` / `stop`）が 1 フレーム 1 行で書く列を平均・p95 にまとめる。PIE のビューポート（1039x654）は 1080p の 3 分の 1 の画素しか無いので `r.ScreenPercentage 175`（1818x1144 = 208 万画素 ≒ 1080p の 207 万）で画素数を合わせ、エンジンの 62 fps の平滑を `t.MaxFPS 500` で外し、敵のいる場所は `slomo 0.05` で実質止めて 10 s 測る（等倍との fps の差は 1 % 未満）。スケーラビリティは 11 群すべて 3（Epic）＝製品と同じ品質。チェックポイントで開くだけでは届かない場所（Zone 2 の独房）は、先に PlayerStart のタグの所へ置いて場面を終わらせてから測る。結果の JSON は `Intermediate/Perf/*.json`（git の外）。

| ゾーンと場所（チェックポイント） | fps avg | p95 の fps | Frame ms | Game ms | GPU ms | DrawCalls | Prims | GPU メモリ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Z1 リフトの到着（cp 4） | 46.0 | 33.9 | 21.72 | 9.56 | 21.10 | 1019 | 336k | 4111 MB |
| Z1 迷路の始まり（cp 5） | 57.7 | 54.8 | 17.33 | 9.50 | 16.64 | 1107 | 350k | 4111 MB |
| Z1 駐車場（cp 6） | 52.7 | 36.9 | 18.96 | 9.63 | 18.40 | 503 | 816k | 4111 MB |
| Z2 独房（cp 7 + 場面の後） | 49.7 | 47.8 | 20.11 | 11.00 | 19.51 | 476 | 36k | 3631 MB |
| Z2 見張りの廊下（cp 8） | 47.3 | 35.0 | 21.14 | 11.33 | 20.50 | 584 | 884k | 3790 MB |
| Z2 迷路（cp 9） | 60.1 | 55.7 | 16.64 | 11.02 | 16.08 | 447 | 20k | 2871 MB |
| Z2 祭壇の車庫（cp 10） | 56.0 | 37.9 | 17.84 | 10.28 | 17.20 | 474 | 40k | 3592 MB |
| 参考: Z1 cp 4 を 720p 相当（116 %） | 79.4 | 76.2 | 12.60 | 8.96 | 12.05 | 1018 | 336k | — |
| 参考: Z2 cp 8 を 720p 相当（116 %） | 71.9 | 69.6 | 13.91 | 11.74 | 13.27 | 584 | 884k | 3626 MB |

- **どこも GPU 律速**（GPU ms ≒ Frame ms、GameThread は 9.5〜11.3 ms で余裕がある）。720p 相当に落とすと両ゾーンの重い場所が 72〜79 fps まで上がるので、フレーム時間のほとんどは画素にかかっている（1080p にすると +8〜9 ms）。描画数は Zone 1 のほうが多く（DrawCalls 約 1000。Zone 2 は 450〜580）、プリミティブは見通しの利く場所（Z1 駐車場 816k、Z2 見張りの廊下 884k）で増える。
- **メモリは両ゾーンとも目安内**: GPU メモリ 2871〜4111 MB（予算 4893〜5198 MB）、`nvidia-smi` のカード全体で 3980〜5348 MB / 6144 MB、エディタの常駐 RAM 3.0〜3.3 GB（目安 20 GB）、システムの空き 13.8〜14 GB。Zone 2 のほうが軽い。
- **fps は 46〜60（平均 52 ほど）で、目安の「1080p で 60 前後」に 10〜20 % 届かない**。ただし PIE の数字にはエディタ自身の描画が載るので、パッケージした本編が同じ数字になるとは限らない（実測にはクックが要り、クックは配布の話なので無人モードでは行わない）。
- **対処は入れていない**。逼迫を避ける設定はエディタにだけ効く場所に置く決まり（`.claude/guides/performance.md` の大原則 2）で、エディタ側を軽くしても製品の fps は動かない。製品の fps を上げるには `Config/DefaultEngine.ini` の描画設定か製品の既定のスケーラビリティを下げることになり、「最終的に遊べるゲームの品質を損なってはならない」に反する。**2026-09-21 のユーザーの回答は「クックして本編の fps を測る」**（作業一覧の項目 36）で、実際に測った結果が下の「パッケージした本編の fps」。**製品の初期値の HIGH なら 1080p で平均 63.2 fps**なので、画質の選択肢を足す項目は立てない。
- 参考: 2026-09-16 の開始地点だけの計測（上の「灯の焼き込み」の終わり）は、ビューポートの画素のまま測った数字なので、この表とは比べられない。

### パッケージした本編の fps（2026-09-21、作業一覧の項目 36 のステップ 5・5b）

**測り方**は `Tools/game_perf.py`（01 記録）。画面への入力は使わず、コマンドラインの `-ExecCmds` だけで測る（1 回目の起動でセーブにチェックポイントを書いて `quit`、2 回目の起動で `t.MaxFPS 500` と `CsvProfile frames=1800` を流し、最後の 10 s をまとめる）。**解像度は `Saved/Archive/Windows/wasami_deception/Saved/Config/Windows/GameUserSettings.ini` の `FullscreenMode=2`（ウィンドウ）+ `ResolutionSizeX/Y=1920/1080`**（既定の `1`＝ボーダーレスだとデスクトップの解像度になる）。**画質は製品自身の OPTIONS の QUALITY**（`UWasamiSettingsSaveGame::Quality`。0 LOW〜3 VERY HIGH、初期値 2 HIGH）で切り替える。`GameUserSettings.ini` の `sg.*` は起動時に製品の設定が上書きするので当てにしない。PIE の表（スケーラビリティ 11 群すべて 3）と同じ品質なのは **VERY HIGH** の列で、**HIGH** の列が遊ぶ人が実際に見る絵。結果の JSON は `Intermediate/Perf/pkg_*.json`・`pkgh_*.json`（git の外）。

| 場所（チェックポイント） | VH fps | VH p95 | VH GPU ms | VH GPU メモリ | HIGH fps | HIGH p95 | HIGH GPU ms | HIGH GPU メモリ | （PIE fps） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Z1 リフトの到着（cp 4） | 43.2 | 32.9 | 22.54 | 2679 MB | 57.3 | 55.6 | 16.97 | 2197 MB | 46.0 |
| Z1 迷路の始まり（cp 5） | 54.9 | 37.5 | 17.43 | 2708 MB | 65.2 | 41.9 | 14.20 | 2228 MB | 57.7 |
| Z1 駐車場（cp 6） | 51.7 | 40.0 | 18.84 | 2769 MB | 62.6 | 37.6 | 15.10 | 2288 MB | 52.7 |
| Z2 独房（cp 7 + 場面の後） | 49.1 | 33.4 | 19.76 | 2920 MB | 60.3 | 44.2 | 16.10 | 2503 MB | 49.7 |
| Z2 見張りの廊下（cp 8） | 46.7 | 31.9 | 20.65 | 2950 MB | 56.4 | 34.4 | 16.89 | 2533 MB | 47.3 |
| Z2 迷路（cp 9） | 58.2 | 46.0 | 16.67 | 2087 MB | 72.4 | 51.7 | 13.25 | 2214 MB | 60.1 |
| Z2 祭壇の車庫（cp 10） | 52.6 | 30.4 | 17.79 | 2887 MB | 67.9 | 48.9 | 14.18 | 2406 MB | 56.0 |

- **同じ場所を見ている**ことの確かめ: プリミティブ数が PIE とほぼ同じ（331k 対 336k、347k 対 350k、812k 対 816k、34k 対 36k、880k 対 884k、17k 対 20k、37k 対 40k）。ドローコールは本編のほうが毎回 100〜150 少ない（エディタだけの描画のぶん）。
- **VERY HIGH（PIE と同じ品質）の本編は PIE より 1〜6 % 遅い**（43.2〜58.2 対 46.0〜60.1）。GameThread は 9.5〜11.3 ms → 2.5〜5.4 ms に減る（エディタのぶんが消える）が、GPU が 0.15〜1.44 ms 増えるので差し引きで遅い。**PIE の数字は本編のおおよその目安として使える。**
- **製品の初期値の HIGH は 56.4〜72.4 fps（7 か所の平均 63.2）**で、VERY HIGH（平均 50.9）より **24〜33 % 速い**（GPU ms が 3.4〜5.6 ms 減る）。**遊ぶ人が実際に見る絵は「1080p で 60 前後」に届いている**（7 か所中 5 か所が 60 以上、残る 2 か所も 56〜57）。60 を割るのは Zone 1 のリフトの到着と Zone 2 の見張りの廊下で、どちらもドローコールかプリミティブが多い場所。
- **どの品質も GPU 律速のまま**（GPU ms ≒ Frame ms、GameThread は 2.4〜5.8 ms）。p95 の fps は 30〜56 と場所によってばらつき、ストリーミングとシェーダーの当たりが残る。
- **メモリは余裕がある**: GPU メモリは VERY HIGH で 2087〜2950 MB、HIGH で 2197〜2533 MB（予算 5198 MB）。PIE より 0.7〜1.4 GB 少なく、`nvidia-smi` のカード全体の山も 2850〜3712 MB / 6144 MB。常駐 RAM は 1.5〜2.0 GB。
- **画質の選択肢はすでに製品にある**（OPTIONS の QUALITY が LOW / MEDIUM / HIGH / VERY HIGH の 4 段と、RESOLUTION SCALE のスライダー。どちらも本家と同じ並び。15 記録）。1080p で 60 に届かないのは最高画質の VERY HIGH だけなので、新しい仕組みを足す必要は無い。

### パッケージした本編の通しプレイ（2026-09-21、作業一覧の項目 36 のステップ 6b）

- **パッケージ版がタイトルから脱出まで 1 回の起動（228 s）で通った**。`python Tools/game_flow.py run`（01 記録）がコマンドラインの `-ExecCmds` だけで進め、**18 の節目がすべて期待どおり**だった（タイトル → `Wasami.ResetSave` + `open` → Zone 1 の到着とステージ OP → 鍵 → 迷路で保存 5 → 全回収 → 駐車場の場面 → 救急車の屋根で保存 7 → Zone 2 → 捕まる場面と独房 → 鍵 → 保存 8・9 → 全回収と欠片 → ガレージ → ポータル → スコア画面 FINAL RANK A）。レベルの読み込みは Zone 1 が 0.60 s・Zone 2 が 1.12 s、`Saved/Archive/Windows/wasami_deception/Saved/Crashes` は 0 件。
- **マウスとキーそのものはパッケージ版では未確認**（タイトルの NEW GAME、欠片の画面の CLOSE、スコア画面の NEXT）。画面への入力が塞がれていたので（ファイアウォールの確認の窓。症状索引）コマンドで回り道した。この 3 つは PIE で `Tools/playthrough.py` が実際に押して通している（11 記録）ので、残るのは「パッケージ版でも押せるか」だけ。

## Mac でのビルド（2026-09-22）

Windows で作った物を Mac で受け取り、Metal 用にクックして遊ぶための道具 `Tools/mac_build.sh`（macOS 専用。macOS が積んでいる bash 3.2 で動くように書いてある）。**Mac では開発しない**（ユーザーの指示。エディタの UI・MCP・参照データ・Git LFS は要らない）。運用と、そう決めた理由は `.claude/guides/distribution.md` の「Mac 版のパッケージ」。Mac 側のパスは `/Users/sbaba/Documents/wasami_deception`、エンジンは `/Users/Shared/Epic Games/UE_5.8`（`UE_ENGINE_DIR` で上書きできる。`Tools/ue_remote.py`・`Tools/editor_cycle.py` と同じ作法）。

- 段取りは 6 つ。**同期**（`--sync [host[:repo]]`。既定は `desktop:Desktop/wasami_deception`）→ **前提チェック** → **エディタのビルド**（`Build.sh wasami_deceptionEditor Mac Development`。クックはエディタのコマンドレットが走るので、遊ぶだけでも要る）→ **`RunUAT.sh BuildCookRun -platform=Mac -clientconfig=<既定 Development> -cook -build -stage -pak -archive`** → **中身の検査** → **起動**（`--run`）。引数は `--sync`・`--sync-rsync`・`--no-git`・`--sync-only`・`--check`・`--no-package`・`--run`・`--config`・`--force`。
- **同期は 2 段で、Windows には何も入れない**（2026-09-22 のユーザーの選択）。追跡ファイルは GitHub から `git pull --ff-only`、git に入らない `Content/`（1204 ファイル・1.2 GB）は Windows から `scp -rp` で**毎回まるごと**取り直す（`Content.new` に受けてから入れ替えるので、途中で落ちても前のものが残る）。差分にしないのは、消えた・改名されたアセットが Mac に残るとクック（`bCookAll=True`）が必ずそれも焼いて落ちるため。`scp` は **sftp サブシステム**を通るので Windows の既定シェル（PowerShell）に左右されない。ただし OpenSSH 8 以前の `scp` はログインシェルを通るので、`ssh -V` が 9 未満なら `-s` を付ける。
- **コードとアセットの時点のズレを止める**: `ssh <host> "git -C <repo> rev-parse HEAD; echo ---; git -C <repo> status --porcelain"` で Windows の HEAD を読み、Mac の HEAD と違えば止まる（`--force` で続行）。未コミットがあれば警告だけ。PowerShell の出力は CRLF なので `\r` を落として読む。**この形の引数なら PowerShell 越しでも壊れない**（2026-09-22 に実測。裸の `.` を含む引数列もバイナリの素通しも問題なし）。
- **同期の後は `exec` で自分を起動し直す**（`git pull` で自分自身が新しくなっているかもしれないため）。
- `--sync-rsync <取り込み元>` は、Windows に rsync を入れた場合の差分同期（既定では使わない。除外は `Intermediate/`・`Binaries/`・`Saved/`・`DerivedDataCache/`・`pak_reference*`・`cc2_reference`・`tmp/`・`observations/`・`__pycache__/`・`.DS_Store`）。rsync は既定で一時ファイルに書いてから rename するので、走っている最中の自分自身を入れ替えても走り続ける（`--inplace` を足すとこれが崩れる）。
- 前提チェックが見るもの: macOS であること、`.uproject` と Content（`Content/Stage/Maps/L_Title.umap`）があること、`Build.sh`・`RunUAT.sh` があること、エンジンが **5.8 系**であること（違えば止まる。承知のうえなら `--force`）、`xcodebuild` と `xcrun -sdk macosx metal` が呼べること、そして**このエンジンに無いプラグイン**。
- **足りないプラグインはビルドの間だけ `.uproject` から外す**（`ModelContextProtocol`・`AllToolsets`・`LiveCodingToolset`。3 つとも `NoRedist: true` なのでエンジンの配り方によっては入っていない）。python3 で JSON を書き換え、`trap … EXIT INT TERM` で必ず元に戻す。3 つとも `TargetAllowList: ["Editor"]` なので、外してもパッケージの中身は変わらない。
- **中身の検査**は `Saved/Cooked/Mac/wasami_deception/Metadata/ReferencedSet.txt` の `^/game/` の数と、`Content/` の `.uasset`＋`.umap` の数の一致で見る（Windows の 2026-09-21 のパッケージでは両方 1139）。`BUILD SUCCESSFUL` は中身を保証しない（上の `bCookAll` の顛末）。
- ログは `Intermediate/MacBuild/build_editor.log` と `Intermediate/MacBuild/uat_package.log`、出来上がりは `Saved/Archive/Mac/*.app`。
- **Mac のために `Config/` と `wasami_deception.uproject` は変えていない**（音声・RHI・アーキテクチャの既定が Windows と揃っていること、`EngineAssociation` が使われないことを確かめた。根拠はガイドの「触らなくてよいもの」）。
- **Windows からは動かせない**（macOS 専用）。2026-09-22 時点で Mac の実機ではまだ通していない。

## 作業の流れ

1. 参照データの前処理（`Tools/` のスクリプト）→ `Intermediate/Pipeline/`。
2. エディタで取り込みと組み立て（MCP のツールセット）→ `/Game/DD`、`/Game/Pipeline`、`/Game/Stage/Maps/`。
3. C++ を変えたらビルドしてエディタを開き直す（`python Tools/editor_cycle.py`）。
4. 実装記録を直して `python .claude/scripts/check_records.py --update`、コミット（`.claude/guides/git-workflow.md`）。

## 既知の制約・注意点

- スクリプトで作り直せるアセット（`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git に入れていない。クローンした直後は上の 1〜2 を実行しないとレベルが開けない。
- Substrate を有効のままにしている（テンプレートの既定）。本家（UE 4.24）は Substrate を使っていないので、見た目を突き詰める段で切ることを検討する。
- `r.RayTracing=False` はこの PC に合わせた設定。RT コアのある GPU で動かすときは戻す。
- 原作のプロジェクト設定のうち、**写していない 2 つ**（`Config/DefaultEngine.ini` にも理由を書いてある）:
  - `r.UsePreExposure=False` … プリ露出はトーンマッパーで打ち消される精度の工夫で、露出を 1.0 に固定した今は何も変えない。切り替えるとシェーダーが全部コンパイルし直しになる。**ただし一つだけ目に見える違いがあった**（2026-09-16）: UE 5.8 はプリ露出に 1〜2 フレーム前の目の順応の読み戻しを使う（自動露出を切っても計測方式は Histogram のまま。`PostProcessEyeAdaptation.cpp` の `FViewInfo::UpdatePreExposure`）ので、テレポートの `CameraAnim_Teleport` の +100 EV の閃光の後、2^100 のプリ露出でシーンカラーが溢れて**真っ黒なフレームが 1 枚**出た（PIE の 60 fps の収録で、白 → 赤 → 黒 → 赤）。ユーザーの決定で `r.EyeAdaptation.PreExposureOverride=1` を入れた（シェーダーの再コンパイルは要らない。原作の「プリ露出無し」と同じく 1.0 に固定する）。入れる前後で開始地点の廊下の絵の平均は 0.02 以内で同じ、白が出た回でも黒は出なかった（04 記録）。
  - `r.DefaultFeature.MotionBlur=False` … 原作はこれでモーションブラーを切っている（ゲームに設定項目は無く、BP のバイトコードも触っていないので戻る箇所が無い）。**2026-09-16 にユーザーが「0.5 のまま（今は変えない）」と決めた**ので写さない。本作は原作よりモーションブラーの掛かった絵になる。

## 変更履歴
- 2026-09-22: 同期を **git pull + scp**（Windows には何も入れない。ユーザーの選択）にし、Windows の HEAD との突き合わせを足した。`--sync-rsync` は別の運び方として残した
- 2026-09-22: Mac で受け取ってビルドして遊ぶための `Tools/mac_build.sh` を足し、「Mac でのビルド」の節を書いた（ユーザーの指示。Mac では開発しない。`Config/` と `.uproject` は変えない）。ついでに古くなっていたターゲットの版（`BuildSettingsVersion.V7`・`IncludeOrderVersion.Unreal5_8`）を直した
- 2026-09-21: パッケージした本編がタイトルから脱出まで通しで遊べることを `Tools/game_flow.py` で確かめ、「パッケージした本編の通しプレイ」の節を足した（項目 36 のステップ 6b。これで項目 36 と大目標 3 を閉じた）
- 2026-09-21: パッケージした本編の fps を 7 か所 × 2 画質で測って「パッケージした本編の fps」の節を足した（項目 36 のステップ 5・5b）。初期値の HIGH は平均 63.2 fps で目安に届き、VERY HIGH は平均 50.9 fps
- 2026-09-21: 有人セッションで性能の要確認に回答をもらい、**クックして本編の fps を測る**ことにした（項目 36）。使っていない原作の題字と `Pipeline/Debug` の材質 17 個はクックの前に**消した**（項目 35）
- 2026-09-20: `ProjectVersion` を 1.0.0 にした（ユーザーの回答。仮の 0.1.0 から）
- 2026-09-16: 初版（現行の構成・設定を記録）
- 2026-09-19: `DefaultEngine.ini` に本家のナビの設定（`RecastNavMesh`・`NavigationSystemV1`）を足した（作業一覧の項目 7 のステップ 1）
- 2026-09-18: `GameInstanceClass` を `WasamiGameInstance` にした（作業一覧の項目 5 のステップ 3）
- 2026-09-16: パイプラインのアセットをソフト参照で持つ決まり（`WasamiAssets.h`）と、Automation テストの置き場所を足した
- 2026-09-16: 露出を原作のプロジェクト設定に合わせた（「露出」の節）。写していない 2 つの設定を「既知の制約・注意点」に足した
- 2026-09-16: 「灯の焼き込み」の結果を、マテリアルのコンパイル失敗と灯の色の取り違え（01 記録）を直した後の値に書き換えた
- 2026-09-16: 「灯の焼き込み」を、灯の Mobility・両面の影・原作のライトマップの解像度と UV に揃えた後の値に書き換え、床の残りの差が扉の不在による映り込みであることと、開始地点のフレーム時間と VRAM を足した
- 2026-09-16: 当たりのチャンネル `Teleport`（旧版の既定 Overlap）を足した
- 2026-09-16: 焼き込みを原作と同じ High 品質にし（Zone 1 は 105.7 秒、Zone 2 は 48.0 秒で初めて焼いた）、結果の表に High の列を足した。残りの差は品質によらないこと、焼き込みの警告が原作どおりであることを書いた
- 2026-09-16: 原作の `r.UsePreExposure=False` の代わりに `r.EyeAdaptation.PreExposureOverride=1` を入れた（テレポートの閃光の後の黒いフレームの対処。ユーザーの決定）
- 2026-09-18: 依存に `LevelSequence`・`MovieScene` を足した（ゾーンの流れがシーケンスを再生する。作業一覧の項目 6 のステップ 3a）
- 2026-09-19: 依存に `AIModule`・`GameplayTasks`・`NavigationSystem` を足した（敵の判断。作業一覧の項目 7 のステップ 2）
- 2026-09-19: 非公開の依存に `EngineSettings` を、`DefaultGame.ini` に `ProjectVersion`（仮に 0.1.0）を足した（タイトルの版の文字。作業一覧の項目 17 のステップ 2）
- 2026-09-19: 非公開の依存に `AnimationCore` を足した（Matron の LookAt。作業一覧の項目 11 のステップ 2）
- 2026-09-20: パッケージの下ごしらえを確かめた（作業一覧の項目 21 のステップ 7）。道具のプラグイン 3 つを Editor ターゲット限定にし、パッケージ設定を空のままにする理由を `DefaultGame.ini` に書いた。クックとビルドは配布の話なので行っていない
- 2026-09-20: 両ゾーンの性能を測って「性能」の節を足した（作業一覧の項目 21 のステップ 5・6）。1080p 相当・Epic で 46〜60 fps・どこも GPU 律速、メモリは目安内。対処は入れず、選択肢をユーザーの判断待ちにした
- 2026-09-21: Windows の `Development` のパッケージを初めて通した（作業一覧の項目 36 のステップ 2）。`DefaultGame.ini` に `AssetManagerSettings` の `GameFeatureData` の規則を足してクックの失敗を直した。手順は `.claude/guides/distribution.md`、出来上がりは `Saved/Archive/Windows`（約 1.0 GB）
