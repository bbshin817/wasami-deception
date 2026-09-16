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
updated: 2026-09-16
---

# 全体像

## 役割
Dark Deception のワサミ版ファンゲームの UE 5.8.2 版。ステージ（本家の病院「Torment Therapy」の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`）もプレイヤーもゲームの仕組みも、本家 Dark Deception の値に倣う（`.claude/guides/original-fidelity.md`）。この記録はプロジェクトの骨格（モジュール、設定、参照データ、作業の流れ）だけを書く。仕組みごとの詳細は各記録（`_index.md`）を参照。

## 構成

| 場所 | 中身 |
| --- | --- |
| `Source/wasami_deception/` | ゲームの C++ モジュール（`Runtime`、`LoadingPhase: Default`）。プレイヤーとゲームモードは 02 記録 |
| `Content/Python/` | エディタの Python。`init_unreal.py` が `wasami_tools` のツールセットを ToolsetRegistry に登録し、MCP から呼べるようにする（01 記録） |
| `Tools/` | エディタの外で動かすスクリプト（参照データの前処理、リモート実行、エディタの開き直し。01 記録） |
| `Intermediate/Pipeline/` | 前処理の出力（git の対象外、作り直せる） |
| `pak_reference/`、`pak_reference_2/`、`cc2_reference/` | 原作データ（git の対象外。読み取り専用） |
| `.claude/` | 運用ルール・参考資料・進捗記録・実装記録 |

## モジュールとビルド

- `wasami_deception.Build.cs` の公開依存: `Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`UMG`（タブレットの画面。03 記録）。非公開依存: `Slate`、`SlateCore`。
- ターゲット: `wasami_deception.Target.cs`（Game）と `wasami_deceptionEditor.Target.cs`（Editor）。どちらも `BuildSettingsVersion.V5`、`IncludeOrderVersion.Unreal5_6`（テンプレートのまま）。
- `wasami_deception.cpp` / `.h` はモジュールの実装（`IMPLEMENT_PRIMARY_GAME_MODULE`）。
- ビルドと開き直しの手順は `.claude/guides/unreal-workflow.md`、自動化は `Tools/editor_cycle.py`（01 記録）。

## プラグイン（`wasami_deception.uproject`）

| プラグイン | 目的 |
| --- | --- |
| `ModelingToolsEditorMode` | テンプレートの既定（エディタのみ） |
| `ModelContextProtocol` | MCP サーバー（エディタを Claude から操作する） |
| `AllToolsets` | エンジン同梱のツールセット一式（アクタ・アセット・マテリアル・シーケンサなど） |
| `LiveCodingToolset` | MCP から Live Coding のコンパイルを走らせる |

`.mcp.json` は Claude Code の接続先（`http://127.0.0.1:8000/mcp`、HTTP）。ポートの衝突については `.claude/guides/unreal-workflow.md`。

## 設定（`Config/`）

- **`DefaultEngine.ini`**
  - `[/Script/EngineSettings.GameMapsSettings]`: `GameDefaultMap` と `EditorStartupMap` は `/Game/Stage/Maps/L_Hospital_Zone1`、`GlobalDefaultGameMode` は `/Script/wasami_deception.WasamiGameMode`。
  - `[/Script/Engine.RendererSettings]`: 静的ライティング無効（`r.AllowStaticLighting=False`）、仮想シャドウマップ有効、メッシュ距離フィールド生成、Lumen（`r.DynamicGlobalIlluminationMethod=1`、`r.ReflectionMethod=1`）、Substrate 有効、**`r.RayTracing=False`**（この PC の GeForce GTX 1660 SUPER に RT コアが無く、Lumen はソフトウェアのレイトレースで動かすため）。
  - `[/Script/Engine.RendererSettings]` の**露出**（2026-09-16）: 原作のプロジェクト設定をそのまま写した。`r.DefaultFeature.AutoExposure=False`・`.Method=0`・`.ExtendDefaultLuminanceRange=False`・`.Bias=0.0`、`r.DefaultFeature.LensFlare=False`、`r.DefaultFeature.LightUnits=1`。UE5 だけの**ローカル露出**は無効値の 1.0 にする（`r.DefaultFeature.LocalExposure.HighlightContrastScale` / `.ShadowContrastScale`。新規プロジェクトの既定 0.8 は原作に無い階調補正になる）。根拠と効果は下の「露出」。
  - `[/Script/WindowsTargetPlatform.WindowsTargetSettings]`: DX12 / SM6、音声 48 kHz。
  - `[/Script/PythonScriptPlugin.PythonScriptPluginSettings]`: `bRemoteExecution=True`（`Tools/ue_remote.py` が使う。ローカルのマルチキャストのみ）、`bDeveloperMode=True`（`Intermediate/PythonStub/unreal.py` が出る）。
- **`DefaultEditorPerProjectUserSettings.ini`**: MCP サーバーの設定（`ServerUrlPath=/mcp`、`ServerPortNumber=8000`、`bAutoStartServer=True`、`bEnableToolSearch=True`）。
- **`DefaultInput.ini`**: テンプレートのまま。Enhanced Input（`DefaultPlayerInputClass=EnhancedPlayerInput`、`DefaultInputComponentClass=EnhancedInputComponent`）、`bEnableLegacyInputScales=True`（本家と同じ 2.5 / −2.5 の視点の倍率が掛かる。02 記録）、`bEnableMouseSmoothing=True`、`FOVScale=0.011110`。
- **`DefaultGame.ini`**、**`DefaultEditor.ini`**: テンプレートのまま（CommonUI の設定とプロジェクト ID）。

## 露出（2026-09-16）

**原作の露出はポストプロセスボリュームではなくプロジェクト設定で決まっている。** 根拠:

- 原作の `06_Hospital_Zone_01` に `PostProcessVolume` は **0 個**（`pak_reference_2/_levels/06_Hospital_Zone_01.scene.json` の `counts`）。Zone 2 と入口 `06_Hospital` には 1 個ずつあるが、どちらも `bOverride_AutoExposure*` を持たない（色補正・ブルーム・AO・DOF・シャープンだけ）。
- `pak_reference_2/_raw/DDeception/Config/DefaultEngine.ini`（UE 4.24）の `[/Script/Engine.RendererSettings]` に `r.DefaultFeature.AutoExposure=False`・`.Method=0`・`.ExtendDefaultLuminanceRange=False`・`r.UsePreExposure=False`・`r.DefaultFeature.MotionBlur=False`・`r.DefaultFeature.LensFlare=False`・`r.DefaultFeature.LightUnits=1`。
- UE 5.8 でも同じ cvar が効く。`r.DefaultFeature.AutoExposure=0` は `AutoExposureMinBrightness` と `MaxBrightness` を 1 にする（`Engine/Source/Runtime/Engine/Private/SceneView.cpp:2060`）。旧レンジでは `MinWhitePointLuminance = MaxWhitePointLuminance = 1` になり（`PostProcessEyeAdaptation.cpp:665`）、露出 = `2^AutoExposureBias` に固定される。
- UE 4.24 の `AutoExposureBias` の既定は **0.0**。原作に入っている第三者プラグイン 2 つ（`ThirdParty/Chameleon/Chameleon`、`ThirdParty/LightProbes/Blueprints/Light_ProbeController`）が、上書きしていない `FPostProcessSettings` をそのまま持っており、どちらも `AutoExposureBias 0.0`・`LowPercent 80`・`HighPercent 98.3`・`MinBrightness 0.03`・`MaxBrightness 2.0` と同じ値を示す。UE 5 の既定は `r.DefaultFeature.AutoExposure.Bias` が 1.0（= 2 倍明るい）なので、**0.0 を明示する**。
- 灯の単位は取り込みのままでよい。`r.DefaultFeature.LightUnits` はエディタで灯を置く工場（`ActorFactoryPointLight.cpp:23` ほか）でしか読まれず、`ULocalLightComponent` の CDO の既定は `Unitless`（`LocalLightComponent.cpp:14`）。原作の Zone 1 の灯 1,121 個はすべて `IntensityUnits` を書き出していない = Unitless で、`dd_level.py` の `DEFAULT_LIGHT_UNITS` と一致する。

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
  - `r.UsePreExposure=False` … プリ露出はトーンマッパーで打ち消される精度の工夫で、露出を 1.0 に固定した今は何も変えない。切り替えるとシェーダーが全部コンパイルし直しになる。
  - `r.DefaultFeature.MotionBlur=False` … 原作はこれでモーションブラーを切っている（ゲームに設定項目は無く、BP のバイトコードも触っていないので戻る箇所が無い）。**2026-09-16 にユーザーが「0.5 のまま（今は変えない）」と決めた**ので写さない。本作は原作よりモーションブラーの掛かった絵になる。

## 変更履歴
- 2026-09-16: 初版（現行の構成・設定を記録）
- 2026-09-16: 露出を原作のプロジェクト設定に合わせた（「露出」の節）。写していない 2 つの設定を「既知の制約・注意点」に足した
