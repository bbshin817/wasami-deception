# 配布とパッケージの運用ルール

WebGL 版の「デプロイ（Cloudflare Pages）の運用ルール」を UE5 版に置き換えたもの。WebGL 版は main への push がそのまま配信だったが、**UE5 版の push は配信ではない**（GitHub にソースが上がるだけ）。

## 素材の扱い（いちばん大事）

- パッケージには原作『Dark Deception』から取った素材（メッシュ、テクスチャ、音、フォント）が入る。本作は非公式のファン作品で、**一般公開を前提にしない**。
- 配布（誰かに渡す、公開する、ストアに出す）の話が出たら、**先にユーザーに確認する**。Claude の判断で配布物をどこかへ上げない。
- リポジトリに入れてよいものの線引きは `.claude/guides/git-workflow.md`。参照データ（`pak_reference/`・`pak_reference_2/`・`cc2_reference/`）とスクリプトで作り直せる素材（`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git に入れない。
- 原作のロゴとキャラクターのモデルは使わない（`.claude/guides/original-fidelity.md`）。パッケージに入っていないことを、配布の前に確かめる。

## パッケージ

- 対象はこの PC と同等の Windows（DirectX 12、SM6）。`Development` か `Shipping` の Win64。Mac で作って遊ぶ手順は下の「Mac 版のパッケージ」。
- 手順（2026-09-21 に実際に通した。作業一覧の項目 36）。**エディタを閉じてから**走らせる（VRAM 6 GB、`.claude/guides/verification.md`）:
  ```bash
  python Tools/editor_cycle.py --quit-only
  "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun \
    -project="C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" \
    -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive \
    -archivedirectory="C:\Users\User\Desktop\wasami_deception\Saved\Archive"
  python Tools/editor_cycle.py --no-quit --no-build   # 終わったら開き直す
  ```
  - 30 分以上かかる見込みで書いたが、実際は**初回が 10 分ほど**（C++ のビルド約 3 分 + クック + ステージ・pak・アーカイブ）。`/Game` の 1139 パッケージを全部クックし直すのは **5 分 22 秒**（2026-09-21、`bCookAll=True` を入れて作り直したとき。シェーダーの DDC が温まっていた分は速い）。クックの中身が変わらなければ差分になり **1 分ほど**。Claude が走らせるときは `run_in_background` にして、**応答を終える前に必ず結果を読む**。
  - 通った印: 標準出力の最後が `BUILD SUCCESSFUL` と `AutomationTool exiting with ExitCode=0 (Success)`。途中のクックの `Warning/Error Summary` が `Success - 0 error(s)` であること（**エラーが 1 件でも出るとクックの中身が正しくても UAT は落ちる**。症状索引の `Error_UnknownCookFailure`）。
  - 出来上がり（`Saved/Archive/Windows/`、全体で約 **1.7 GB**）:
    - `wasami_deception.exe`（起動役のランチャー 171 KB）と `wasami_deception/Binaries/Win64/wasami_deception.exe`（本体）
    - `wasami_deception/Content/Paks/` … `wasami_deception-Windows.ucas` **970 MB**・`.pak` 11 MB・`.utoc` 746 KB と `global.ucas` 3.2 MB（UE 5 の既定は IoStore なので中身はほぼ `.ucas` に入る）
    - `Engine/` 75 MB（エンジン側のシェーダーと既定のアセット）
  - 途中の出力（どれも git の外、作り直せる。消してよい）: クックは `Saved/Cooked/Windows`、ステージは `Saved/StagedBuilds/Windows`（アーカイブと同じ中身）。UAT のログは `%APPDATA%\Unreal Engine\AutomationTool\Logs\`。
- 出力先はリポジトリの外（`Saved/` か別のフォルダ）にする。`Build/` と `Saved/` は git の対象外。
- **パッケージが出来たら、まず中身に本編が入っているかを見る**（`BUILD SUCCESSFUL` は中身を保証しない）。手早いのは 2 つ:
  - `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` … コンテナに入ったパッケージの一覧（小文字）。`grep -c "^/game/"` が `Content/` のパッケージ数と同じだけあること（2026-09-21 は **1139 件 / 全 1650 行**。1 桁なら本編が入っていない）。
  - `Intermediate/Overnight/uat_package.log` の `Packages Cooked: N ... Total Packages: M` … `/Game` の 1139 パッケージ + エンジンとプラグインの 500 ほどで **1600 前後**になること（2026-09-21 は `Packages Cooked: 1650 ... Total Packages: 1657`）。
  - `UnrealPak.exe <…>.utoc -List` はコンテナの**ファイル名を持つ入り口だけ**（`.ubulk` とエンジンの `.uasset`）を出す。クックしたパッケージはパッケージ ID で引くので名前が出ず、中身の確認には使えない。
- **原作のロゴとキャラクターのモデルが入っていないことの確かめ方**（同じ `ReferencedSet.txt` を名前で見る。2026-09-21 に実際に通した）:
  ```bash
  R=Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt
  grep -iE "logo|darkdeception|glowstick|splash" $R
  grep "^/game/" $R | grep -viE "/audio/|/sounds?/|dialogue" | grep -iE "reaper|nurse|monkey|bierce|matron|character|/sk_|_skel"
  ```
  - ロゴに出てよいのは、エンジンの `zenlogo_64` と本作の `t_titlelogo`・`t_titlelogoglow`・`m_dd_portallogo` だけ。`dark_deception` を含む曲のファイル名は音なので使ってよい（`original-fidelity.md` の表）。
  - スケルタルメッシュに出てよいのは本作の `sk_wasamienemy`・`sk_wasamiboss` だけ。`hospital_*_anim_skeleton` は小物（ガレージのリフト・のこぎり罠）の動きで、キャラクターではない。
  - 名前に `nurse` が出るものは中身を見る。**音・火花・小物のテクスチャは使ってよい**が、**キャラクターの姿が描かれた絵（ポスター・看板・デカール）が見つかったら `original-fidelity.md` の決まりどおりユーザーに確認する**（2026-09-21 に 3 枚見つかった: `hospital_poster_nurse_01_D`・`hospital_poster_nurse_02`・`hospital_decal_nurseambulance`。ユーザー待ち）。
- **最後に通しで遊べるかを確かめる**: `python Tools/game_flow.py run`（**エディタを閉じてから**。2026-09-21 に実際に通した。道具は実装記録 01、結果は 00 記録の「パッケージした本編の通しプレイ」）。パッケージ版を対話デスクトップで起動し、コマンドラインの `-ExecCmds` だけでタイトル → Zone 1 → Zone 2 → 脱出のスコア画面まで **228 s** で進めて、18 の節目（レベル・チェックポイント・ライフ・シャード・目的・画面のウィジェット）を `Wasami.Status` のログで確かめ、`quit` で終える。
  - 通った印: 最後の行が `the playthrough went through`（終了コード 0）、`no crash report`、表の右端がすべて `OK`。落ちたところは表の行と `Intermediate/GameFlow/*.png` の絵で分かる。
  - 画面への入力は使わないので、**マウスとキーそのもの（タイトルの NEW GAME、欠片の画面の CLOSE、スコア画面の NEXT）は別に確かめる**（PIE では `Tools/playthrough.py` が実際に押している）。
  - 二重起動を防ぐ錠は `Tools/game_perf.py`（本編の fps の計測）と共有している（`Intermediate/Perf/.game_perf.lock`）。同時には走らせられない。
- **`bCookAll=True` は `/Game` を丸ごとクックする**ので、どのレベルからも使っていないアセットもパッケージに入る。原作の素材をパッケージから外したいときは `Content/` から消すしかない（作業一覧の項目 35 と同じ）。
- クック前に確かめること: 参照している素材がすべて `/Game` にあるか（`Intermediate/Pipeline/` の中間データはパッケージに入らない。**`/Engine` のアセットも、コードから `TSoftObjectPtr` や `FObjectFinder` で指しただけのものは入らないことがある**〈2026-09-22 に `BlackUnlitMaterial`・`RobotoTiny`・`SphereRenderHeightMap` が入っていないと分かり、`/Game/Wasami` と `/Game/DD/_Engine` に作り直した。症状索引「パッケージ版でだけ、黒いはずの板が灰色のグリッドになる」に、入るもの・入らないものの一覧と、コードの `/Engine/…` を洗い出す grep がある。**エンジンのアセットを新しく指したら、その grep で一覧に無いものが増えていないかを見る**〉）、既定のマップとゲームモード（`Config/DefaultEngine.ini`）、起動して 1 面が遊べるか。
- **見た目の確かめを PIE だけで済ませない**（2026-09-22 のレビューの指摘から）。パッケージ版でだけ出る違い（クックされないアセット、エディタ専用の既定値）があるので、見た目に関わる直しは `Saved/Archive/Windows/wasami_deception.exe` でも 1 度見る。
- **`Config/DefaultGame.ini` の 2 つの節はパッケージのためにある**（消さない。理由はその ini のコメント）:
  - `[/Script/UnrealEd.ProjectPackagingSettings]` の `bCookAll=True`（無いと **`/Game` のパッケージが `L_Title` の 1 つしか入らない**パッケージが黙って出来る。2026-09-21 に実際に起きた）。`MapsToCook` と `DirectoriesToAlwaysCook` は書かない（書くと C++ から直に読む 195 個のアセットが落ちる）。
  - `[/Script/Engine.AssetManagerSettings]` の `GameFeatureData` の規則（無いとクックがエラー 2 件で落ちる）。

## Mac 版のパッケージ（Mac 上でビルドして遊ぶ）

2026-09-22 のユーザーの指示による運用。**Mac では開発しない**（エディタの UI・MCP・参照データ `pak_reference*`・Git LFS はどれも要らない）。Windows が作った物を受け取り、Metal 用にクックして遊ぶだけ。

- Mac 側のパスは `/Users/sbaba/Documents/wasami_deception`（GitHub からの clone）、エンジンは `/Users/Shared/Epic Games/UE_5.8`（`UE_ENGINE_DIR` で変えられる）。**エンジンは Windows と同じ 5.8 系**（今 5.8.2）。アセットは 5.8 で保存してあるので古い版では開けない。Xcode（Metal のコンパイラを含む）も要る。
- 道具は `Tools/mac_build.sh`（同期 → 前提チェック → エディタのビルド → BuildCookRun → 中身の検査 → 起動）。**clone したファイルに実行ビットは無いので `bash` を付けて呼ぶ**:
  ```bash
  bash Tools/mac_build.sh --check          # 前提だけ確かめる（初回に）
  bash Tools/mac_build.sh --sync --run     # 毎回これ 1 本
  ```
- **クックはエディタのコマンドレットが走る**ので、遊ぶだけでもエディタのビルド（＝Xcode）は要る。これは省けない。

### 運び方（git pull + scp。Windows には何も入れない）

`Content/` は **1204 ファイル・1.2 GB がすべて `.gitignore`**（参照データから作り直せる素材。`git-workflow.md`）。clone で来るのは 424 ファイルだけで、アセットは 1 つも来ない。しかも無人運転が回るたびに書き換わる（2026-09-22 時点で直近 1 日に 238 ファイル、2 日で 541 ファイル）。**だから追跡ファイルと Content を別の経路で運ぶ**（Git LFS に入れる案は、この大きさと頻度では履歴が破裂するので採らない）:

- **追跡ファイル**（`Source/`・`Config/`・`Tools/`・`.uproject`・`.claude/`）は GitHub から `git pull --ff-only`。
- **`Content/`** は Windows から `scp -rp` で**毎回まるごと取り直す**（1.2 GB）。`Content.new` に受けてから入れ替えるので、途中で落ちても前のものが残る。**差分ではなくまるごと**なのは、消えた・改名されたアセットが Mac に残ると、クック（`bCookAll=True`）が必ずそれも焼いて 1 エラーで UAT ごと落ちるため。
- 接続は `~/.ssh/config` の **`desktop`**（Tailscale 越し。2026-09-22 時点で 100.80.88.93）。リポジトリは SSH のホーム `C:\Users\User` からの相対パス **`Desktop/wasami_deception`** で届く（`--sync desktop:Desktop/wasami_deception` が既定値）。
- **Windows には何も入れない**（2026-09-22 のユーザーの選択）。`scp` は **sftp サブシステム**（`sshd_config` の `Subsystem sftp sftp-server.exe`）を通るので、Windows の既定シェルが PowerShell のままでも影響を受けない。OpenSSH 8 以前の Mac では `scp` が既定でログインシェルを通ってしまうので、`mac_build.sh` は `ssh -V` を見て 9 未満なら `-s` を付ける。
- **コードとアセットの時点がズレないように**、`mac_build.sh` は同期のときに `ssh desktop "git -C Desktop/wasami_deception rev-parse HEAD"` で Windows の HEAD を読み、Mac の HEAD と違えば**止まる**（Windows に未 push のコミットがある＝コードだけ古い状態になる）。Windows で push してからやり直す。未コミットの変更があるときは警告だけ出して続ける。
- 同期は **Windows 側が止まっているときに取る**（無人運転の最中だと、コミットとアセットの時点がズレる）。
- **Mac 側の作業ディレクトリは 1 つを使い回す**（毎回 clone し直さない）。`Intermediate/` と `Saved/Cooked/Mac` が残っていれば C++ もクックも差分で済む。シェーダーの DDC はプロジェクトの外（`~/Library/Application Support/Epic/UnrealEngine/Common`）なので、作業ディレクトリを作り直しても生き残る。

**別の運び方（今は使わない）**: Windows に rsync を入れれば差分同期にできる（`scoop install rsync`。使うときは `bash Tools/mac_build.sh --sync-rsync <取り込み元>`）。**Windows の既定シェルを変える必要は無い**ことは 2026-09-22 に確かめた（PowerShell 越しでも引数の解釈もバイナリの素通しも壊れない。最初に壊れて見えたのは、試した Git Bash が引数 `/c` を `C:/` に変換していたためで、PowerShell のせいではなかった）。毎回 1.2 GB の転送が重くなったら切り替える。

### 触らなくてよいもの（2026-09-22 に確かめた）

- **`wasami_deception.uproject`**: `EngineAssociation` の GUID `{932002E3-…}` はどこにも登録されていない（`HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds` は空で、エンジンは Launcher の `LauncherInstalled.dat` に `UE_5.8` として入るだけ）。`Build.sh` / `RunUAT.sh` に `-project=` を渡す限り Mac でも使われないので書き換えない（`.uproject` をダブルクリックで開くときだけ関係する）。
- **`ModelContextProtocol` だけ**、`mac_build.sh` が**ビルドの間だけ `.uproject` から外し、終わったら必ず戻す**（trap）。`TargetAllowList: ["Editor"]` なのでパッケージの中身は変わらない。**外さないとクックが落ちる**: このプラグインが `127.0.0.1:8000` を掴みにいって失敗するとエラーが 1 件ログに出て、クックが完走していても UAT が `ExitCode=25` で落ちる（2026-09-22 に Mac で実際に起きた。症状索引）。**`AllToolsets` と `LiveCodingToolset` はそのまま**にする（`AllToolsets` が連れてくる `GameFeatures` を、`Config/DefaultGame.ini` の `GameFeatureData` の規則が前提にしているので、Windows と同じ顔ぶれでクックする）。3 つとも Mac の UE 5.8.2 に同梱されていた（`NoRedist` だが入っている）。
- **`Config/` に Mac 用に足すものは無い**。`[/Script/WindowsTargetPlatform.WindowsTargetSettings]` の音声の値は `BaseEngine.ini` の `[/Script/MacTargetPlatform.MacTargetSettings]` の既定と同じ（`AudioSampleRate=48000`・`AudioCallbackBufferFrameSize=1024`・`AudioNumBuffersToEnqueue=1`・`AudioNumSourceWorkers=4`）。`CacheSizeKB=65536` は Mac の節に無いが、エンジンの既定 `FAudioStreamCachingSettings::DefaultCacheSize = 64 * 1024` と同じ値。残りのキー（`CompressionOverrides`・`MaxChunkSizeOverrideKB`・各 `*SampleRate`・`CompressionQualityModifier`・`AutoStreamingThreshold`・`SoundCueCookQualityIndex`・プラグイン名）は既定値そのもので、`bResampleForDevice=False` のときサンプルレートの表は読まれない（`FPlatformCompressionUtilities`）。RHI は既に `+TargetedRHIs=SF_METAL_SM6`。アーキテクチャも既定のままでよい（`DefaultArchitecture=MacTargetArchitectureHost` なので、手元のビルドは Universal にならず host〈Apple Silicon なら arm64〉だけを作る）。

### 時間の目安と、最初の 1 回に確かめること

- **初回**は Mac で `git clone` してから `bash Tools/mac_build.sh --check` → `bash Tools/mac_build.sh --sync --run`。エンジンと Xcode の用意に加えて **Metal のシェーダーを全部コンパイルする**ので数時間を見る。
- **2 回目から**は「Content の転送（1.2 GB）+ 差分ビルド + 差分クック」で、転送が直結なら **10〜20 分**が目安（Windows の実績: クックは中身が変わらなければ約 1 分、全クックで 5 分 22 秒）。アセットが大きく変わった回は、変わった分の Metal のシェーダーのコンパイルが上乗せされる。
- **Mac でしか分からなかったこと**（2026-09-22 に実機で確かめた分）:
  1. ~~プラグイン 3 つがエンジンにあるか~~ → **入っていた**（Mac の UE 5.8.2。`NoRedist` だが同梱）。ただし別の理由で外す（上の「触らなくてよいもの」）
  2. ~~Metal のコンパイラが呼べるか~~ → **呼べた**（Xcode 27.0。`--check` が見る。無ければ `xcodebuild -downloadComponent MetalToolchain`）
  3. **Metal SM6 での見え方**（Substrate・仮想シャドウマップ・露出）が Windows と揃うか → **まだ未確認**
  4. クックの所要は **3 分 42 秒**（初回、`PeakPhysMemoryMB=6961`）。シェーダーで数時間という見込みより ずっと速かった
- 中身の検査は Windows と同じ考え方で、`mac_build.sh` が `Saved/Cooked/Mac/wasami_deception/Metadata/ReferencedSet.txt` の `^/game/` の数と `Content/` の `.uasset`＋`.umap` の数（2026-09-22 時点で 1139）を突き合わせ、合わなければ止まる。
- **ステージが入れ忘れる dylib は `mac_build.sh` が後から同梱する**（`libtbb.12.dylib`・`libtbbmalloc.2.dylib`・`libmetalirconverter.dylib`。入れないと起動の瞬間に `Library not loaded: @rpath/libtbb.12.dylib` で落ちる。症状索引）。同梱したら ad-hoc で署名し直す。
- 出来上がりは **`~/Applications/WasamiDeception/Mac/wasami_deception.app`**（`--archive <置き場所>` か `WASAMI_ARCHIVE_DIR` で変えられる）。**`~/Documents` の下には置かない**: そこに置いた `.app` は起動のたびに macOS が「書類フォルダへのアクセス」を聞き（TCC）、ゲームが全画面で画面と入力を掴んでいるとその確認に触れられず固まる（2026-09-22 に実際に起きた。症状索引）。クックとステージはプロジェクトの中（`Saved/Cooked`・`Saved/StagedBuilds`）に残るので差分は効いたまま。自分の Mac でビルドした物は ad-hoc 署名で、そのまま起動できる。ゲームのログは `~/Library/Logs/wasami_deception/wasami_deception.log`。
- ログを見ながら遊ぶなら `.app` の中の実行ファイルを直に呼ぶ（`wasami_deception.app/Contents/MacOS/wasami_deception`）。`Development` でビルドしているので `Wasami.Status` などのコンソールコマンドも使える（`~` で開く）。
- **ほかの Mac に渡すのは配布**（Gatekeeper を通すには notarize が要る）。このファイルの頭の決まりどおり、必ずユーザーに確認する。

## 性能の目安

運用は `.claude/guides/performance.md`（この PC は i7-9700K、32 GB、GeForce GTX 1660 SUPER の VRAM 6 GB）。

- パッケージした本編で 1080p・60 fps 前後を目安にする。**開発中の逼迫を避けるための設定をパッケージに持ち込まない**（品質を落とさない）。
- 計測は PIE かパッケージで `stat unit` / `stat fps` / `stat RHI`、必要なら `ProfileGPU`。結果は実装記録に残す（WebGL 版の README の FPS の表にあたる）。
