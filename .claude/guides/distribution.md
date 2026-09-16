# 配布とパッケージの運用ルール

WebGL 版の「デプロイ（Cloudflare Pages）の運用ルール」を UE5 版に置き換えたもの。WebGL 版は main への push がそのまま配信だったが、**UE5 版の push は配信ではない**（GitHub にソースが上がるだけ）。

## 素材の扱い（いちばん大事）

- パッケージには原作『Dark Deception』から取った素材（メッシュ、テクスチャ、音、フォント）が入る。本作は非公式のファン作品で、**一般公開を前提にしない**。
- 配布（誰かに渡す、公開する、ストアに出す）の話が出たら、**先にユーザーに確認する**。Claude の判断で配布物をどこかへ上げない。
- リポジトリに入れてよいものの線引きは `.claude/guides/git-workflow.md`。参照データ（`pak_reference/`・`pak_reference_2/`・`cc2_reference/`）とスクリプトで作り直せる素材（`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git に入れない。
- 原作のロゴとキャラクターのモデルは使わない（`.claude/guides/original-fidelity.md`）。パッケージに入っていないことを、配布の前に確かめる。

## パッケージ

- 対象はこの PC と同等の Windows（DirectX 12、SM6）。`Development` か `Shipping` の Win64。
- 手順（まだ整備していない。最初に作るときにここへ書き足す）:
  ```powershell
  & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive -archivedirectory="<出力先>"
  ```
- 出力先はリポジトリの外（`Saved/` か別のフォルダ）にする。`Build/` と `Saved/` は git の対象外。
- クック前に確かめること: 参照している素材がすべて `/Game` にあるか（`Intermediate/Pipeline/` の中間データはパッケージに入らない）、既定のマップとゲームモード（`Config/DefaultEngine.ini`）、起動して 1 面が遊べるか。

## 性能の目安

- この PC（i7-9700K、32 GB、GeForce GTX 1660 SUPER 4 GB）で 1080p・60 fps 前後を目安にする。
- 計測は PIE かパッケージで `stat unit` / `stat fps`、必要なら `ProfileGPU`。結果は実装記録に残す（WebGL 版の README の FPS の表にあたる）。
- 重いときにまず見るところ: 灯の影（Zone 1 だけで 1,121 灯。仮想シャドウマップ）、Lumen の品質、Nanite の対象、テクスチャの解像度。スケーラビリティ（`r.ScreenPercentage`、シャドウの質）で落とす。
