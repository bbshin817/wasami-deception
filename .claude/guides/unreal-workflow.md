# エディタの操作と C++ のビルド

## 環境

- エンジン: UE 5.8.2（`C:\Program Files\Epic Games\UE_5.8`）。プロジェクト: `wasami_deception.uproject`（C++ モジュール `wasami_deception`）。
- PC: i7-9700K、32 GB、GeForce GTX 1660 SUPER（4 GB、RT コアなし）。
- MCP: `.mcp.json` の `unreal-mcp`（http://127.0.0.1:8000/mcp）。エディタが起動していないと使えない。
- この PC では Docker Desktop（`com.docker.backend.exe`）が `0.0.0.0:8000` を掴んでいる。エディタの MCP サーバーはより狭い `127.0.0.1:8000` で受けるので動くが、**エディタを閉じている間に Claude Code が接続を試すと Docker に当たって `ENDPOINT_NOT_FOUND` になり、そのセッションでは `unreal-mcp` が切れたままになる**。エディタを開き直した後は、ユーザーに `/mcp` で再接続してもらう（それまではリモート実行で作業できる）。
- Python の `unreal.Rotator(a, b, c)` の位置引数は (roll, pitch, yaw) の順。必ず `unreal.Rotator(roll=…, pitch=…, yaw=…)` と名前で渡す。

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
| `WasamiStageTools` | 病院ステージ（`06_Hospital_Zone_01`・`_02`）の取り込み（メッシュ・テクスチャ・マテリアル）とレベルの組み立て |
| `WasamiDDTools` | 本家の資産（今はカメラシェイク）を原作データから `/Game/DD/…` に作る |
| `WasamiDevTools` | コンソールコマンド |

- 各ツールは呼ばれるたびに `wasami_tools.pipeline` の中身を読み込み直すので、パイプラインの Python を直したらそのまま呼べる。
- ツールセットのクラス（引数や新しいツール）を変えたときは、読み込み直して登録し直す（ToolsetRegistry の `reload_module`。MCP のツールから自分自身を読み込み直すと危ないので、リモート実行で行う）:
  `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`
- `reload_module` が登録し直すのは、前から登録されていたクラスだけ。**新しいツールセットを足したとき**は、読み込み直した後に `from toolset_registry._registry_interface import get_toolset_registry; get_toolset_registry().register_toolset_class(<クラス>)` で登録する（次の起動からは `init_unreal.py` が登録する）。
- エディタの起動より後に `Content/Python` を作ったときは、`sys.path` に足してから `import wasami_tools; wasami_tools.register()` する。
- 原作の cook されたデータは、既定値と同じプロパティを持たない。ポストプロセスの override が立っていて値が無いのは「既定値で上書き」の意味。灯の `IntensityUnits` のように、**UE4 と UE5 で既定値が違うプロパティは、書き出しに無くても明示的に入れる**（UE 4.24 の `IntensityUnits` の既定は `Unitless`、UE5 は `Candelas`）。

## Python のリモート実行

Project Settings > Plugins > Python の Remote Execution を有効にしてある（ローカルのマルチキャストだけ）。`python Tools/ue_remote.py <file.py>` でエディタの中で Python を動かせる。調査やツールセットの登録し直しに使い、ゲームの素材を作る処理はツールセット（MCP）に入れる。コードは別々の globals / locals で実行されるので、処理は関数に入れて呼ぶ。

## ステージの取り込み

1. `python Tools/dd/prepare_stage.py` … `pak_reference_2` から `Intermediate/Pipeline/dd/stage_ue.json` を作る。区画が同じマテリアルを共有するメッシュ（13 個）は、スロットが潰れないように glTF を `Intermediate/Pipeline/dd/meshes/` へ分け直す（`.bin` は複製しない）。
2. `WasamiStageTools.import_dd_stage_assets(max_items)` … メッシュ → テクスチャ → マテリアルの順に `/Game/DD/…` へ。量が多いので `remaining` が 0 になるまで繰り返す（全部で 489 アセット・約 6 分）。
3. `WasamiStageTools.build_dd_stage_level("Zone1")` / `("Zone2")` … `/Game/Stage/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2` を組み立てる（タグ `dd` のアクタを消してから置き直すので、何度呼んでもよい）。

パイプラインを直したときは `refresh_dd_stage_assets()` でマスターマテリアルとテクスチャの設定を更新できる。MCP が使えないときは `Tools/ue_remote.py` から `wasami_tools.pipeline.dd_stage` / `dd_level` を直接呼べる。

## C++ のビルド

- `.cpp` だけの変更: Live Coding（`LiveCodingToolset.CompileLiveCoding`。プラグインを有効にしてから使える）。
- ヘッダーや UCLASS / UPROPERTY の変更、新しいクラス: エディタを閉じて UBT でビルドし、開き直す。
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" wasami_deceptionEditor Win64 Development "-Project=C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" -WaitMutex
  Start-Process "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" '"C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject"'
  ```
- **C++ はパイプラインが作るアセット（`/Game/DD`・`/Game/Pipeline`）を `ConstructorHelpers` で読まない**。ソフト参照の UPROPERTY にして、使うときに読む（`Source/wasami_deception/WasamiAssets.h`）。コンストラクタで読んだものはエディタの起動時にルートに入り、ツールセットがマテリアルを作り直すとエディタが `Assertion failed: !IsRooted()` で落ちる（2026-09-16 に一度落ちた）。
- エディタを閉じる前にすべて保存する。閉じる・開き直すことに確認は要らない（2026-09-17 のユーザーの指示。`.claude/guides/verification.md`）が、閉じたら必ず開き直す。
- **`Tools/editor_cycle.py` は Claude から実行できる**（閉じる → ビルド → 開き直す → 応答を待つ）。Claude Code は Windows のセッション 0 で動いていて、そこから直接起動したエディタはディスプレイが見えず即落ちる（`DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`）ので、スクリプトは一度きりのスケジュールタスクでログオン中のセッションに起動する（01 記録）。開き直した後は MCP の再接続（`/mcp`）だけユーザーに頼む。
