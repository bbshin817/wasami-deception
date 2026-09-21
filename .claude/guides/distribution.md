# 配布とパッケージの運用ルール

WebGL 版の「デプロイ（Cloudflare Pages）の運用ルール」を UE5 版に置き換えたもの。WebGL 版は main への push がそのまま配信だったが、**UE5 版の push は配信ではない**（GitHub にソースが上がるだけ）。

## 素材の扱い（いちばん大事）

- パッケージには原作『Dark Deception』から取った素材（メッシュ、テクスチャ、音、フォント）が入る。本作は非公式のファン作品で、**一般公開を前提にしない**。
- 配布（誰かに渡す、公開する、ストアに出す）の話が出たら、**先にユーザーに確認する**。Claude の判断で配布物をどこかへ上げない。
- リポジトリに入れてよいものの線引きは `.claude/guides/git-workflow.md`。参照データ（`pak_reference/`・`pak_reference_2/`・`cc2_reference/`）とスクリプトで作り直せる素材（`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git に入れない。
- 原作のロゴとキャラクターのモデルは使わない（`.claude/guides/original-fidelity.md`）。パッケージに入っていないことを、配布の前に確かめる。

## パッケージ

- 対象はこの PC と同等の Windows（DirectX 12、SM6）。`Development` か `Shipping` の Win64。
- 手順（2026-09-21 に実際に通した。作業一覧の項目 36）。**エディタを閉じてから**走らせる（VRAM 6 GB、`.claude/guides/verification.md`）:
  ```bash
  python Tools/editor_cycle.py --quit-only
  "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun \
    -project="C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" \
    -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive \
    -archivedirectory="C:\Users\User\Desktop\wasami_deception\Saved\Archive"
  python Tools/editor_cycle.py --no-quit --no-build   # 終わったら開き直す
  ```
  - 30 分以上かかる見込みで書いたが、実際は**初回が 10 分ほど**（C++ のビルド約 3 分 + クック 5m 4s + ステージ・pak・アーカイブ）。2 回目からはクックが差分になり **1 分ほど**（クック 23s、ステージ 36s）。Claude が走らせるときは `run_in_background` にして、**応答を終える前に必ず結果を読む**。
  - 通った印: 標準出力の最後が `BUILD SUCCESSFUL` と `AutomationTool exiting with ExitCode=0 (Success)`。途中のクックの `Warning/Error Summary` が `Success - 0 error(s)` であること（**エラーが 1 件でも出るとクックの中身が正しくても UAT は落ちる**。症状索引の `Error_UnknownCookFailure`）。
  - 出来上がり（`Saved/Archive/Windows/`、全体で約 1.0 GB）:
    - `wasami_deception.exe`（起動役のランチャー 171 KB）と `wasami_deception/Binaries/Win64/wasami_deception.exe`（本体）
    - `wasami_deception/Content/Paks/` … `wasami_deception-Windows.ucas` 198 MB・`.pak` 11 MB・`.utoc` 178 KB と `global.ucas` 3.2 MB（UE 5 の既定は IoStore なので中身はほぼ `.ucas` に入る）
    - `Engine/` 75 MB（エンジン側のシェーダーと既定のアセット）
  - 途中の出力（どれも git の外、作り直せる。消してよい）: クックは `Saved/Cooked/Windows`（303 MB）、ステージは `Saved/StagedBuilds/Windows`（1.0 GB、アーカイブと同じ中身）。UAT のログは `%APPDATA%\Unreal Engine\AutomationTool\Logs\`。
- 出力先はリポジトリの外（`Saved/` か別のフォルダ）にする。`Build/` と `Saved/` は git の対象外。
- **パッケージが出来たら、まず中身に本編が入っているかを見る**（`BUILD SUCCESSFUL` は中身を保証しない）。手早いのは 2 つ:
  - `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` … コンテナに入ったパッケージの一覧（小文字）。`grep -c "^/game/"` が 1000 前後あること（1 桁なら本編が入っていない）。
  - `Intermediate/Overnight/uat_package.log` の `Packages Cooked: N ... Total Packages: M` … `/Game` の 1139 パッケージ + エンジンとプラグインの 500 ほどで **1600 前後**になること。
  - `UnrealPak.exe <…>.utoc -List` はコンテナの**ファイル名を持つ入り口だけ**（`.ubulk` とエンジンの `.uasset`）を出す。クックしたパッケージはパッケージ ID で引くので名前が出ず、中身の確認には使えない。
- クック前に確かめること: 参照している素材がすべて `/Game` にあるか（`Intermediate/Pipeline/` の中間データはパッケージに入らない）、既定のマップとゲームモード（`Config/DefaultEngine.ini`）、起動して 1 面が遊べるか。
- **`Config/DefaultGame.ini` の 2 つの節はパッケージのためにある**（消さない。理由はその ini のコメント）:
  - `[/Script/UnrealEd.ProjectPackagingSettings]` の `bCookAll=True`（無いと **`/Game` のパッケージが `L_Title` の 1 つしか入らない**パッケージが黙って出来る。2026-09-21 に実際に起きた）。`MapsToCook` と `DirectoriesToAlwaysCook` は書かない（書くと C++ から直に読む 195 個のアセットが落ちる）。
  - `[/Script/Engine.AssetManagerSettings]` の `GameFeatureData` の規則（無いとクックがエラー 2 件で落ちる）。

## 性能の目安

運用は `.claude/guides/performance.md`（この PC は i7-9700K、32 GB、GeForce GTX 1660 SUPER の VRAM 6 GB）。

- パッケージした本編で 1080p・60 fps 前後を目安にする。**開発中の逼迫を避けるための設定をパッケージに持ち込まない**（品質を落とさない）。
- 計測は PIE かパッケージで `stat unit` / `stat fps` / `stat RHI`、必要なら `ProfileGPU`。結果は実装記録に残す（WebGL 版の README の FPS の表にあたる）。
