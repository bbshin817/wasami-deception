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
  - `[/Script/WindowsTargetPlatform.WindowsTargetSettings]`: DX12 / SM6、音声 48 kHz。
  - `[/Script/PythonScriptPlugin.PythonScriptPluginSettings]`: `bRemoteExecution=True`（`Tools/ue_remote.py` が使う。ローカルのマルチキャストのみ）、`bDeveloperMode=True`（`Intermediate/PythonStub/unreal.py` が出る）。
- **`DefaultEditorPerProjectUserSettings.ini`**: MCP サーバーの設定（`ServerUrlPath=/mcp`、`ServerPortNumber=8000`、`bAutoStartServer=True`、`bEnableToolSearch=True`）。
- **`DefaultInput.ini`**: テンプレートのまま。Enhanced Input（`DefaultPlayerInputClass=EnhancedPlayerInput`、`DefaultInputComponentClass=EnhancedInputComponent`）、`bEnableLegacyInputScales=True`（本家と同じ 2.5 / −2.5 の視点の倍率が掛かる。02 記録）、`bEnableMouseSmoothing=True`、`FOVScale=0.011110`。
- **`DefaultGame.ini`**、**`DefaultEditor.ini`**: テンプレートのまま（CommonUI の設定とプロジェクト ID）。

## 作業の流れ

1. 参照データの前処理（`Tools/` のスクリプト）→ `Intermediate/Pipeline/`。
2. エディタで取り込みと組み立て（MCP のツールセット）→ `/Game/DD`、`/Game/Pipeline`、`/Game/Stage/Maps/`。
3. C++ を変えたらビルドしてエディタを開き直す（`python Tools/editor_cycle.py`）。
4. 実装記録を直して `python .claude/scripts/check_records.py --update`、コミット（`.claude/guides/git-workflow.md`）。

## 既知の制約・注意点

- スクリプトで作り直せるアセット（`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git に入れていない。クローンした直後は上の 1〜2 を実行しないとレベルが開けない。
- Substrate を有効のままにしている（テンプレートの既定）。本家（UE 4.24）は Substrate を使っていないので、見た目を突き詰める段で切ることを検討する。
- `r.RayTracing=False` はこの PC に合わせた設定。RT コアのある GPU で動かすときは戻す。

## 変更履歴
- 2026-09-16: 初版（現行の構成・設定を記録）
