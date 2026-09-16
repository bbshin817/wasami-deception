# 検証の運用ルール（エディタと PIE）

動作確認のために Claude が動かすもの（Unreal Editor、PIE、スクリーンショット、Automation テスト）の扱いを決める。WebGL 版の「検証用ブラウザの運用ルール」を UE5 版に置き換えたもの。要点は同じ: **ユーザーの PC の他の作業を邪魔しない**。

## ユーザーの PC を邪魔しない

- **エディタはユーザーのアプリでもある**。閉じる・開き直す・PIE を始める / 終えるといった操作は、最初にユーザーの確認を取る（一度確認を取れば、その作業の中では繰り返し行ってよい）。
- **OS 全体の入力を操作しない**。マウスカーソルを動かす・キーを送るツールは使わない。操作は MCP のツール、`Tools/ue_remote.py` のリモート実行、Automation テストの中だけで行う。
- **PIE を出しっぱなしにしない**。確認が終わったら `StopPIE`（または `editor_request_end_play`）で必ず止める。止め忘れるとエディタのアセット操作が失敗する（`The Editor is currently in a play mode.`）。
- **エディタを 2 つ起動しない**。`Tools/editor_cycle.py` は起動中のエディタを閉じてから開き直す。
- 長い処理（取り込み、レベルの組み立て、シェーダーのコンパイル、C++ のビルド）はバックグラウンドで走らせ、終わりを待つ間に別の作業をする。進捗記録の「再開時の注意」に、走らせているものを書く。

## 本家のゲームを動かすとき

この PC には遊べる本家が 2 つ入っている（旧版 = `pak_reference` と同一、最新版 = `pak_reference_2` と同一）。何をどちらで観察するか、どのステージをどう出すかは `.claude/guides/original-fidelity.md` の「本家のゲームを手元で動かす」。ここには動かし方の作法だけを書く。

- **本家のゲームはユーザーの画面と音を占有する**（`GameUserSettings.ini` は 2211x1247 の `FullscreenMode=1`）。エディタと同じように、**起動する前にユーザーの確認を取る**。観察が終わったら閉じてもらい、出しっぱなしにしない。
- **どちらのビルドを動かすかを先に決める**。2 つを同時に起動しない（ランチャは既に動いているときは起動を拒む）。
  - 旧版: `C:\Users\User\AppData\Local\DDeception\Launch-Classic-Ch3.cmd`
  - 最新版: `C:\Users\User\AppData\Local\DDeception\Launch-Latest.cmd`（Steam の Library の Play も同じもの）
- **Claude は Windows のセッション 0 にいるので、直接起動してもゲームは立ち上がらない**（表示出力が無く、エディタと同じ `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` で落ちる）。コンソールのセッションで起動する。
  - `python Tools/console_session.py <ランチャの .cmd>`（一度きりのスケジュールタスクでログオン中のユーザーのセッションに起動し、終わったらタスクを消す。`--wait <画像名>` で起動を待てる）。
  - ユーザーにランチャを実行してもらってもよい。
- **メニューとゲーム内の操作はユーザーの手でやってもらう**（OS 全体の入力を操作しない規則はそのまま）。頼むときは、**REPLAY で選ぶステージ・何をするか・どの画面を見てほしいか**を先に短く伝える。
- **エディタ（や PIE）と本家を同時に動かさない**。VRAM は 6 GB しかないので、先にエディタを閉じるか PIE を止めてから起動をお願いする（`.claude/guides/performance.md`）。
- 画面はユーザーに撮ってもらう。シッピングビルドなのでコンソールコマンド（`HighResShot`）は使えない前提で考える。比べるときは本作と**同じ場所・同じ向き**で撮って並べる。
- ゲームは（旧版のランチャでは `-log` 付きで）起動するので、ログは `C:\Users\User\AppData\Local\DDeception\Saved\Logs\` に出る（Claude が後から読める）。
- 遊ぶとセーブ（`Saved\SaveGames\SaveSlot.sav`）は書き換わる。2 つのビルドはこのセーブを共有する。スピードブーストとテレポーテーションは解放済みなのでそのまま遊べる。**Claude はセーブを編集しない**（必要になったらユーザーに確認する）。
- **Steam の登録（`steamapps\appmanifest_332950.acf`）を触るときは Steam を完全に終了してから**。Steam が動いていると書き戻される。登録は最新版（`installdir = Dark Deception`）のままにしておく（旧版を指した状態で Steam が更新すると旧版のフォルダが上書きされる）。

## 見た目の確認

- 静止画は MCP の `EditorToolset.EditorAppToolset.CaptureViewport`（カメラの位置と向きを渡せる）か、PIE 中のコンソールコマンド `HighResShot 1280x720`（`Saved/Screenshots/WindowsEditor/` に出る）。
- 比べる相手は原作の収録と、WebGL 版の画面（`.claude/references/webgl/`、`docs/screenshots/`）。**同じ場所・同じ向き**で撮って並べる。ステージの位置は原作データの配置（`pak_reference_2/_levels/06_Hospital_Zone_0*.scene.json` の `world.location`、PlayerStart やトリガー）から取る。
- 撮った画像は会話に貼る前に縮小する（`CaptureViewport` の戻り値は base64 で大きいので、ファイルに保存してから縮小して読む）。

## 動きの確認

- **エディタが背面にあるとティックが 3 fps ほどに落ちる**（`Use Less CPU when in Background`）。リモート実行から `LaunchCharacter` などで動かしても速さが出ず、時間に依存する確認（FOV の追従、頭の揺れ、クールダウン）はあてにならない。
  - 入力を伴う確認は、ユーザーに PIE で触ってもらうか、入力を流す Automation テスト（`AutomationTestToolset`）を書く。
  - どうしてもリモートで確かめるときは、エディタを前面にしてもらうか、`Use Less CPU when in Background` を切ってから行い、確認後に戻す。
- ゲームの音はユーザーのスピーカーから鳴る。音を確かめる必要がないときは PIE の音量を上げない。

## テスト

- 純粋な規則（ゲームの状態、ギミックの時間、セーブ）は C++ の Automation テスト（`Source/.../Tests/`）にして、`AutomationTestToolset` の `DiscoverTests` → `RunTests` で回す。エディタを開いていないときは `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests <filter>;quit" -Unattended -NullRHI`。
- 実装記録の同期は `python .claude/scripts/check_records.py`（`.claude/guides/implementation-records.md`）。
