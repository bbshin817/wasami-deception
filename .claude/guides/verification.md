# 検証の運用ルール（エディタと PIE）

動作確認のために Claude が動かすもの（Unreal Editor、PIE、スクリーンショット、Automation テスト）の扱いを決める。WebGL 版の「検証用ブラウザの運用ルール」を UE5 版に置き換えたもの。要点は同じ: **ユーザーの PC の他の作業を邪魔しない**。

## ユーザーの PC を邪魔しない

- **作業の許可は求めずに進める**（2026-09-17 のユーザーの指示「ルールを更新。許可を求めず常に続行」）。エディタを閉じる・開き直す（`Tools/editor_cycle.py`）、PIE を始める・止める、エディタの窓とビューポートへの入力（`--allow UnrealEditor.exe`）、本家のゲームの起動と操作は、確認を取らずに行う。それまでは「エディタの開き直しと本家の起動は先に確認、PIE は確認不要（同日の指示）」だった。明示的に禁止されたときはそれに従う。
- **エディタはユーザーのアプリでもある**ことは変わらない。閉じる前に未保存の変更を確かめて保存し、閉じたら必ず開き直して元の状態（開いていたレベル）に戻す。PIE は止め忘れない。
- **変更を捨てる操作・配布・本家のセーブの編集は、これまでどおり先にユーザーに確認する**（取り返しがつかないか外に出るもの。`.claude/guides/progress-tracking.md`・`distribution.md`、下の「本家のゲームを動かすとき」）。
- **画面の操作は許可した窓にだけ送る**（2026-09-16 にユーザーの指示で変更。それまでは「OS 全体の入力を操作しない」だった）。入力は `Tools/desktop.py` から送り、前面の窓が許可した対象のときだけ届く（下の「画面を操作する」）。
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
- **メニューとゲーム内の操作は Claude が `Tools/desktop.py` で行う**（2026-09-16 から。下の「画面を操作する」）。ユーザーに頼むときは、**REPLAY で選ぶステージ・何をするか・どの画面を見てほしいか**を短く伝える。
- **エディタ（や PIE）と本家を同時に動かさない**。VRAM は 6 GB しかないので、先にエディタを閉じる（`python Tools/editor_cycle.py --quit-only`）か PIE を止めてから起動し、終わったら本家を閉じてエディタを開き直す（`.claude/guides/performance.md`）。
- 画面は `python Tools/desktop.py shot` で撮る（シッピングビルドなのでゲーム内のコンソールコマンド `HighResShot` は使えない）。比べるときは本作と**同じ場所・同じ向き**で撮って並べる。
- ゲームは（旧版のランチャでは `-log` 付きで）起動するので、ログは `C:\Users\User\AppData\Local\DDeception\Saved\Logs\` に出る（Claude が後から読める）。
- 遊ぶとセーブ（`Saved\SaveGames\SaveSlot.sav`）は書き換わる。2 つのビルドはこのセーブを共有する。スピードブーストとテレポーテーションは解放済みなのでそのまま遊べる。**Claude はセーブを編集しない**（必要になったらユーザーに確認する）。
- **Steam の登録（`steamapps\appmanifest_332950.acf`）を触るときは Steam を完全に終了してから**。Steam が動いていると書き戻される。登録は最新版（`installdir = Dark Deception`）のままにしておく（旧版を指した状態で Steam が更新すると旧版のフォルダが上書きされる）。

## 画面を操作する（2026-09-16 から）

2026-09-16 のユーザーの指示（「Windows Computer Use を使い、画面操作もあなたへ依頼したい」）で、**Claude が画面を操作してよい**ことになった。それまでの「OS 全体の入力を操作しない」は取り下げ、代わりに次の枠を守る。

- 入力と画面の取得は `Tools/desktop.py` から行う。Claude は Windows のセッション 0 にいて、セッション 1（ユーザーのデスクトップ）へは入力も画面取得も届かないので、**セッション 1 に操作エージェント**（`Tools/desktop_agent.py`、`pythonw.exe`）を常駐させ、`Intermediate/DesktopAgent/` の JSON でやり取りする。

| コマンド | 内容 |
| --- | --- |
| `python Tools/desktop.py start` / `status` / `stop` | エージェントの起動（`Tools/console_session.py` 経由）・状態・停止 |
| `ping` | セッション・画面の大きさ・前面の窓 |
| `shot --scale 0.3 --name x.png` | 画面を撮って PNG に保存（`--region LEFT TOP RIGHT BOTTOM` で範囲） |
| `click X Y` / `look --dx N --dy N` / `scroll` | 絶対座標のクリック・相対のマウス移動（視点）・ホイール |
| `key esc enter` / `combo ctrl s` / `hold shift w --ms 1500` / `type "text"` | キーを順に・同時に・押しっぱなし・文字入力 |
| `record --seconds 5 --name x.mkv` / `record_status` | 画面を 60 fps の動画に撮る（バックグラウンド。すぐ返るので、続けて入力を送れる）・終わったか |

- **入力は許可した窓にだけ届く**。前面の窓の実行ファイルが許可の一覧に無ければエージェントが断る。既定は本家のゲーム（`DDeception-Win64-Shipping.exe`・`DDeception.exe`）だけ。エディタに送るときは `--allow UnrealEditor.exe` を付ける（**PIE の確認のためなら確認は要らない**。それ以外のエディタへの入力は、その都度ユーザーの確認を取ってから）。
- **OS 全体に効くものは送らない**（Win キー、Alt+Tab、Alt+F4 はエージェントが断る）。
- **エディタを前面にするには、エディタの窓（ほかの小窓と重ならない所）を 1 回クリックする**。PIE を始めてもエディタは前面に来ない（2026-09-16）。VS Code が前面のときは、そのクリックだけ `--allow Code.exe --allow UnrealEditor.exe` で送ってよい（クリックの位置がエディタの窓の上であることを撮った画面で確かめてから）。
- **Automation テストを始めると、エンジンが PIE を止める**。テストと PIE の確認は続けて行い、同時には走らせない。テストはエディタが前面で 10 fps を超えるまで待つ（背面では 3 fps のまま最大 600 秒待つ）。
- **PIE にキーを送るときは、先にビューポートを 1 回クリックして焦点を渡す**（PIE を始めただけではキーが届かなかった。2026-09-16）。PIE でないときにビューポートをクリックすると、エディタでアクタを選んでしまう（選択だけならレベルは汚れない）。PIE を始める前に前面にある小窓（Automation のログなど）は閉じておく。撮影は 1 回に数秒かかるので、時間に依存する値は `unreal.GameplayStatics.get_time_seconds` と一緒にリモート実行で読む。
- **ユーザーが操作している間は送らない**。ユーザーがマウスやキーボードを使う必要が出たら、先に `stop` する（入力がぶつかる）。エージェントは 30 分何も来なければ自分で終了する。
- **一瞬の演出（カメラアニメ、閃光など）は `record` で撮る**（2026-09-16）。`shot` は 1 枚に数秒かかる。`record` を始めて 1 秒ほど待ち（ffmpeg の起動）、入力を送り、終わってから `ffprobe -show_entries frame=pts_time` で各フレームの時刻を、`ffmpeg -fps_mode passthrough` でフレームを取り出す（付けないと一定の速さに複製され、時刻と組にならない）。画面が変わらない間はフレームが間引かれる。入力の瞬間は絵の変化（明るさの急変など）から逆算する。動画は `Intermediate/DesktopAgent/shots/`（本家のものは `observations/` へ写す）。
- **`record` が終わらないとき**（2026-09-17）: `record_status` が秒数を過ぎても `running` のまま、動画ができず `.mkv.log` も空なら、ffmpeg の `ddagrab` が最初のフレームを待って止まっている（その日は `Opened dxgi output 0` の後に進まなかった。原因は未特定）。止まった ffmpeg は自分が起動したものなので `taskkill` で止め、`gdigrab`（CPU での取り込み）で撮る: `ffmpeg -f gdigrab -framerate 60 -draw_mouse 0 -offset_x <左> -offset_y <上> -video_size <幅>x<高さ> -i desktop -t 5 -c:v libx264 -preset ultrafast -qp 18 <出力>`。Claude のシェルがセッション 1 にいるとき（`console_session.py` が「started directly」と出す）はそのままバックグラウンドで走らせられる。範囲をビューポートに絞れば 60 fps で撮れる。取り出すフレームの番号（`-frame_pts 1`）は 1/60 秒単位になる。
- この PC の画面は **3440x1440**。座標は物理ピクセル（エージェントは DPI 対応済み）。撮った PNG は `--scale 0.2`〜`0.35` に縮めて読む（原寸は 5〜6 MB になるので会話に読み込まない。比較用に原寸を残すときは `--scale 1.0` で保存だけする）。
- 何を送ったかは `Intermediate/DesktopAgent/agent.log` に残る（前面の窓の名前つき）。

## 本家のゲームに MOD を入れるとき

Simple Mod Menu（`dd-sml` + 本体 v3.1.3。ユーザーが用意したもの。`M` キーで開閉）を使うときの決め事。**2026-09-16 に最新版へ導入済み**（`common\Dark Deception\DDeception\Content\Paks\` に `..._SimpleModMenu.pak` 150 MB と `..._SMM-Loader.pak` 176 KB）。pak の設置は Claude の許可判定で止まる（第三者のコードの組み込み）ので、入れ直すときはユーザーに実行してもらう。

- 使ってよい: **Maps**（ステージ内のチェックポイントへ直接飛ぶ。病院の Zone 1・Zone 2 に入る唯一の実用的な手）、**Active Enemies**（敵の位置の確認と除去）、**Miscellaneous**（`Num1` 飛行・`Num2` ノークリップ・`Num3` 無敵・シャードの回収）、コマンド `clvl`（現在のレベル名）・`help`・`list`。観察の場所まで移動するのに使う。
- `Maps` で飛ぶと「S ランクが取れない・ストーリーの進行が付かない・進行が失われる恐れ」の警告が出る（`YES` で進む）。セーブの控えを取ってから使う。
- **World Editor は使わない**。レベルの配置を動かす・消す・足すと**自動で保存され**（`%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation`）、原作の見え方が根拠にならなくなる。
- MOD は pak を `DDeception\Content\Paks\` に置くだけ、外すときは pak を消すだけ。ローダー（`..._SMM-Loader.pak`）は `BP_DD_GameMode` を差し替え、メニュー本体（`..._SimpleModMenu.pak`）の `/Game/SimpleModMenu/Blueprints/BP_SimpleModMenu` を生成する。**ローダーだけを入れない**（参照先が無い）。
- **MOD を入れた状態で観察したことは、記録に「MOD 入り」と書く**。挙動や値の根拠は必ず原作データ（`pak_reference_2/`）のコード。MOD は観察の足場でしかない。

## 見た目の確認

- 静止画は MCP の `EditorToolset.EditorAppToolset.CaptureViewport`（カメラの位置と向きを渡せる）か、PIE 中のコンソールコマンド `HighResShot 1280x720`（`Saved/Screenshots/WindowsEditor/` に出る）。
- 比べる相手は原作の収録と、WebGL 版の画面（`.claude/references/webgl/`、`docs/screenshots/`）。**同じ場所・同じ向き**で撮って並べる。ステージの位置は原作データの配置（`pak_reference_2/_levels/06_Hospital_Zone_0*.scene.json` の `world.location`、PlayerStart やトリガー）から取る。
- 撮った画像は会話に貼る前に縮小する（`CaptureViewport` の戻り値は base64 で大きいので、ファイルに保存してから縮小して読む）。

## 動きの確認

- **エディタが背面にあるとティックが 3 fps ほどに落ちる**（`Use Less CPU when in Background`）。リモート実行から `LaunchCharacter` などで動かしても速さが出ず、時間に依存する確認（FOV の追従、頭の揺れ、クールダウン）はあてにならない。
  - 入力を伴う確認は、ユーザーに PIE で触ってもらうか、入力を流す Automation テスト（`AutomationTestToolset`）を書く。
  - どうしてもリモートで確かめるときは、エディタを前面にしてもらうか、`Use Less CPU when in Background` を切ってから行い、確認後に戻す。（2026-09-16: この設定は Python から見えず、コンソールの `set EditorPerformanceSettings bThrottleCPUWhenNotForeground False` も効かなかった。前面にするのは上の「画面を操作する」のクリックで行う）
- **PIE で動いている最中の絵を撮る**（2026-09-16）: `desktop.py hold w` の間はエージェントが撮影できない。エディタが前面のまま、エディタの Python で `unreal.register_slate_post_tick_callback` に `player.add_movement_input(前方, 1.0, False)` を入れて毎フレーム前進させ、その間に `desktop.py shot` で撮り、終わったら `unregister_slate_post_tick_callback` で外す。PIE の `shot showui` はエディタの窓全体を撮ってビューポートが黒くなるので使えない（UI を含む絵はエージェントで撮る）。
- ゲームの音はユーザーのスピーカーから鳴る。音を確かめる必要がないときは PIE の音量を上げない。

## テスト

- 純粋な規則（ゲームの状態、ギミックの時間、セーブ）は C++ の Automation テスト（`Source/.../Tests/`）にして、`AutomationTestToolset` の `DiscoverTests` → `RunTests` で回す。エディタを開いていないときは `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests <filter>;quit" -Unattended -NullRHI`。
- 実装記録の同期は `python .claude/scripts/check_records.py`（`.claude/guides/implementation-records.md`）。
