# 検証の運用ルール（エディタと PIE）

動作確認のために Claude が動かすもの（Unreal Editor、PIE、スクリーンショット、Automation テスト）の扱いを決める。WebGL 版の「検証用ブラウザの運用ルール」を UE5 版に置き換えたもの。要点は同じ: **ユーザーの PC の他の作業を邪魔しない**。

## ユーザーの PC を邪魔しない

- **エディタはユーザーのアプリでもある**。閉じる・開き直す・PIE を始める / 終えるといった操作は、最初にユーザーの確認を取る（一度確認を取れば、その作業の中では繰り返し行ってよい）。
- **OS 全体の入力を操作しない**。マウスカーソルを動かす・キーを送るツールは使わない。操作は MCP のツール、`Tools/ue_remote.py` のリモート実行、Automation テストの中だけで行う。
- **PIE を出しっぱなしにしない**。確認が終わったら `StopPIE`（または `editor_request_end_play`）で必ず止める。止め忘れるとエディタのアセット操作が失敗する（`The Editor is currently in a play mode.`）。
- **エディタを 2 つ起動しない**。`Tools/editor_cycle.py` は起動中のエディタを閉じてから開き直す。
- 長い処理（取り込み、レベルの組み立て、シェーダーのコンパイル、C++ のビルド）はバックグラウンドで走らせ、終わりを待つ間に別の作業をする。進捗記録の「再開時の注意」に、走らせているものを書く。

## 見た目の確認

- 静止画は MCP の `EditorToolset.EditorAppToolset.CaptureViewport`（カメラの位置と向きを渡せる）か、PIE 中のコンソールコマンド `HighResShot 1280x720`（`Saved/Screenshots/WindowsEditor/` に出る）。
- 比べる相手は WebGL 版の画面（`.claude/references/webgl/`、`docs/screenshots/`）と、原作・ファンゲームの収録。**同じ場所・同じ向き**で撮って並べる。ステージの位置は `Intermediate/Pipeline/cc2/stage_ue.json` の `gameplay`（チェックポイント、トリガー）から取る。
- 撮った画像は会話に貼る前に縮小する（`CaptureViewport` の戻り値は base64 で大きいので、ファイルに保存してから縮小して読む）。

## 動きの確認

- **エディタが背面にあるとティックが 3 fps ほどに落ちる**（`Use Less CPU when in Background`）。リモート実行から `LaunchCharacter` などで動かしても速さが出ず、時間に依存する確認（FOV の追従、頭の揺れ、クールダウン）はあてにならない。
  - 入力を伴う確認は、ユーザーに PIE で触ってもらうか、入力を流す Automation テスト（`AutomationTestToolset`）を書く。
  - どうしてもリモートで確かめるときは、エディタを前面にしてもらうか、`Use Less CPU when in Background` を切ってから行い、確認後に戻す。
- ゲームの音はユーザーのスピーカーから鳴る。音を確かめる必要がないときは PIE の音量を上げない。

## テスト

- 純粋な規則（ゲームの状態、ギミックの時間、セーブ）は C++ の Automation テスト（`Source/.../Tests/`）にして、`AutomationTestToolset` の `DiscoverTests` → `RunTests` で回す。エディタを開いていないときは `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests <filter>;quit" -Unattended -NullRHI`。
- 実装記録の同期は `python .claude/scripts/check_records.py`（`.claude/guides/implementation-records.md`）。
