# エディタの操作と C++ のビルド

## 環境

- エンジン: UE 5.8.2（`C:\Program Files\Epic Games\UE_5.8`）。プロジェクト: `wasami_deception.uproject`（C++ モジュール `wasami_deception`）。
- PC: i7-9700K、32 GB、GeForce GTX 1660 SUPER（4 GB、RT コアなし）。
- MCP: `.mcp.json` の `unreal-mcp`（http://127.0.0.1:8000/mcp）。エディタが起動していないと使えない。

## MCP の使い方

- `list_toolsets` → `describe_toolset` で道具と引数を確かめ、`call_tool` で呼ぶ。
- **1 つずつ順に呼ぶ**（ゲームスレッドで動くので並べて呼ぶと止まる）。**結果を必ず確かめる**（失敗しても例外にならない道具がある）。
- 一括の変更（取り込み、ステージの組み立て、多数のアセットの編集）の前後で保存する（`AssetTools.save_assets`、または `WasamiStageTools` の各処理が保存する）。
- PIE の間はアセットを作らない。結果がおかしいときは `EditorAppToolset.IsPIERunning` を確かめる。
- 見た目の確認は `EditorAppToolset.CaptureViewport`（カメラの位置を渡せる）、ログは `LogsToolset`、プレイの確認は `EditorAppToolset.StartPIE` / `StopPIE`。

## プロジェクトのツールセット

`Content/Python/init_unreal.py` がエディタの起動時に `wasami_tools` のツールセットを登録し、MCP に出る。

| ツールセット | 内容 |
| --- | --- |
| `WasamiStageTools` | CC2 の Zone_1 の取り込み（メッシュ・テクスチャ・マテリアル）とステージのレベルの組み立て |
| `WasamiDDTools` | 本家の資産（今はカメラシェイク）を原作データから `/Game/DD/…` に作る |
| `WasamiDevTools` | コンソールコマンド |

- 各ツールは呼ばれるたびに `wasami_tools.pipeline` の中身を読み込み直すので、パイプラインの Python を直したらそのまま呼べる。
- ツールセットのクラス（引数や新しいツール）を変えたときは、読み込み直して登録し直す（ToolsetRegistry の `reload_module`。MCP のツールから自分自身を読み込み直すと危ないので、リモート実行で行う）:
  `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`
- `reload_module` が登録し直すのは、前から登録されていたクラスだけ。**新しいツールセットを足したとき**は、読み込み直した後に `from toolset_registry._registry_interface import get_toolset_registry; get_toolset_registry().register_toolset_class(<クラス>)` で登録する（次の起動からは `init_unreal.py` が登録する）。
- エディタの起動より後に `Content/Python` を作ったときは、`sys.path` に足してから `import wasami_tools; wasami_tools.register()` する。
- 原作の cook されたデータは、既定値と同じプロパティを持たない。ポストプロセスの override が立っていて値が無いのは「既定値で上書き」の意味（`cc2_level` はそう扱う）。

## Python のリモート実行

Project Settings > Plugins > Python の Remote Execution を有効にしてある（ローカルのマルチキャストだけ）。`python Tools/ue_remote.py <file.py>` でエディタの中で Python を動かせる。調査やツールセットの登録し直しに使い、ゲームの素材を作る処理はツールセット（MCP）に入れる。コードは別々の globals / locals で実行されるので、処理は関数に入れて呼ぶ。

## ステージの取り込み

1. `python Tools/cc2/prepare_stage.py` … `<WEBGL>` の layout.json・stage.json と cc2_reference から `Intermediate/Pipeline/cc2/`（`stage_ue.json` と区画ごとの glb）を作る。
2. MCP で `WasamiStageTools` の取り込みを呼ぶ（メッシュ・テクスチャ・マテリアルを `/Game/CC2/…` に）。量が多いので 1 回に処理する数を区切ってあり、残りが 0 になるまで呼ぶ。
3. MCP で `WasamiStageTools` のレベルの組み立てを呼ぶ（`/Game/Stage/Maps/…`）。

## C++ のビルド

- `.cpp` だけの変更: Live Coding（`LiveCodingToolset.CompileLiveCoding`。プラグインを有効にしてから使える）。
- ヘッダーや UCLASS / UPROPERTY の変更、新しいクラス: エディタを閉じて UBT でビルドし、開き直す。
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" wasami_deceptionEditor Win64 Development "-Project=C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" -WaitMutex
  Start-Process "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" '"C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject"'
  ```
- エディタを閉じる前にすべて保存する。エディタを閉じる・開き直すことは、ユーザーが作業中かもしれないので、最初に確認を取る。
