# 症状索引（会ったことのある問題と、その対処）

エラー文やおかしな挙動から引く索引。**症状で引けること**が目的なので、エラー文はそのまま（grep できる形で）書く。詳しい理由は実装記録（`.claude/implementation-records/`）の「既知の制約・注意点」に残し、ここからはリンクで結ぶ。決まりは `.claude/guides/autonomy.md` の「症状索引」。

- **引く**: エラー文やおかしな挙動に会ったら、まずこのファイルを grep する。
- **書く**: 解決にエディタの開き直し 1 回以上、または 30 分以上かかったら、次に進む前にその場で書く。直さなくてよい既知の見え方（「これは正しい」）と、試して駄目だった案（「これはやらない」）も書く。
- 1 件の形: 見出しに症状、本文に「原因 / 対処 / 確かめ方 / 出典（記録・コミット・日付）」。分類は下の見出しで分ける。未解決のものは「未解決」と書く。
- 記録の呼び方: 「01 記録」は `.claude/implementation-records/01-stage-pipeline.md`（番号は `_index.md`）。コミットは `git show <hash>` で本文が読める。

## Claude Code の hooks・運用スクリプト

### SessionStart / Stop hook の知らせが出ない（hook が黙って何も返さない）

- 症状: セッション開始時に、プロジェクトの hook の知らせ（未完了の進捗記録・main 以外のブランチ・無人モード）が出ない。`python .claude/scripts/session_start_hook.py | wc -c` が 0。エラーも出ない。
- 原因: この PC のコンソールは cp932。出力に cp932 に無い文字（`—` U+2014 など。コミットメッセージや記録の題名に普通に入る）が 1 つでもあると `print` が `UnicodeEncodeError` になり、hook の外側の `try` が握りつぶして exit 0・出力なしになる。
- 対処: hook の先頭で `sys.stdout` / `sys.stderr` を `reconfigure(encoding="utf-8", errors="replace")`（Claude Code は hook の出力を UTF-8 で読む）。2026-09-17 に 3 つの hook（`session_start_hook.py`・`stop_hook.py`・`post_edit_hook.py`）に入れた。新しい hook や運用スクリプトを書くときも同じ 3 行を入れる（`Tools/editor_cycle.py` にも同じ対処がある）。
- 確かめ方: `python .claude/scripts/session_start_hook.py > out.json` して、Python で `open(out.json, encoding="utf-8")` から `additionalContext` を読む。**パイプの先の Python に読ませない**（cp932 で読むと `）` の UTF-8 の 3 バイト目 0x89 が続く `\` を巻き込んで 1 文字に化け、JSON の `\n` が `n` だけ残って行が繋がって見える）。
- 出典: 2026-09-17 の無人運転の作業（進捗記録 `20260917-autonomy.md`。コミットは git log で「無人運転の決まり」を引く）。

### auto モードの分類器が `[Credential Exploration]` で Bash を拒否する（認証情報のファイルや CLI 本体を読むとき）

- 症状: `Permission for this action was denied by the Claude Code auto mode classifier. Reason: [Credential Exploration].` `%USERPROFILE%\.claude\.credentials.json` の形を見るだけ（値は出さない）でも、`claude` の CLI 本体（npm の `@anthropic-ai/claude-code/cli.js`）を使用量 API の文字列で grep するだけでも拒否される。同じコマンドに無関係な処理（`git log` など）が混ざっていると、それごと落ちる。
- 原因: auto モードの分類器は、認証情報の置き場所と CLI の内部の探索を内容によらず拒否する（意図どおりの振る舞い）。
- 対処: 認証情報と CLI 本体に触れない設計にする。使用量は `Tools/overnight.py --usage-cmd` で外から差し込む（2026-09-17）。認証情報に触れない処理は別のコマンドに分けて出す。
- 確かめ方: 拒否の文言に `[Credential Exploration]` があれば同じもの。
- 出典: 2026-09-17 の無人運転のステップ 2（進捗記録 `20260917-autonomy.md`、実装記録 01 の変更履歴）。

### auto モードの分類器が、ガイドの「先に確認」の文の書き換えを止める

- 症状: 検証のガイドの「本家の起動の前に確認する」「PIE 以外のエディタへの入力は確認する」の行を新しい決まりに書き換える Edit が auto モードで拒否され、古い文が残る。古い文が残ると次のセッションの Claude がまた許可を求める（2026-09-17 に実際に起き、ユーザーに 2 度目の指示をさせた）。
- 原因: 分類器が「確認を求める決まりを弱める編集」を止める。
- 対処: 手動モードに切り替えて書き換える。決まりの本文はガイドの冒頭にも書いておく（詳細の行が古くても冒頭が勝つ）。
- 確かめ方: `grep -n "先に確認" .claude/guides/*.md .claude/implementation-records/*.md` に、取り消したはずの決まりが残っていないこと。
- 出典: 進捗記録 `20260916-tablet-powers.md` の決定事項（2026-09-17 ステップ 9b と 10:03）、コミット 85b9be3、自動メモリ `pie-without-asking`。

### 実装記録のハッシュが、ソースを変えていないのにずれる（`check_records.py` が変更を報告する）

- 症状: main へのマージや checkout の直後に `python .claude/scripts/check_records.py` が複数の記録を「ソースが変わった」と報告する。`git status` は clean で、`git diff` も空。
- 原因: Windows の `core.autocrlf=true` で checkout のたびに行末が CRLF になり、ハッシュが行末だけで変わっていた。
- 対処: `.gitattributes` に `* text=auto eol=lf`（2026-09-16、コミット 511ac5c。LFS の 2 行は `-text` 付きで後に置いてあるので影響しない）。作業ツリーを取り込み直して追跡ファイルを LF にし、`--update` でハッシュを取り直した（e5a428c）。
- 確かめ方: `git ls-files --eol | grep w/crlf` が空。ずれたときは `git diff` が空であることを見てから `--update`。
- もう 1 つの原因（2026-09-17）: Python の `open(p, 'w')` で書き戻すと、Windows では行末が CRLF になる（`git diff` が「CRLF will be replaced by LF」と警告する）。書き戻すときは `open(p, 'w', encoding='utf-8', newline='\n')` にする。
- もう 1 つの経路（2026-09-19）: 作業ブランチで CRLF のまま書かれた C++ のハッシュを `--update` で取ると、main への切り替えとマージで git が LF に書き直した後にずれる（Stop hook が止める。項目 9 のマージの後に 02・06・07 で起きた）。
- 根本の対処（2026-09-19）: 上の 2 つの経路で、ハッシュを LF で取り直すだけのコミットが 9-17 から 11 回続いた。`check_records.py` の `git_blob_hash` が、NUL を含まない中身の `\r\n` を `\n` に揃えてからハッシュを取るようにした（git が `eol=lf` で入れる中身と同じ。LF のファイルのハッシュは変わらない）。作業コピーの行末ではもうずれないので、LF に戻して取り直すコミットは要らない。それでもずれたら、中身が本当に変わっている。
- 出典: コミット 511ac5c・d4c2006・e5a428c（2026-09-16）。

### `Tools/editor_cycle.py` がビルドの失敗を報告する途中で `UnicodeEncodeError` で落ちる

- 症状: ビルドが失敗したのに、スクリプトが `UnicodeEncodeError: 'cp932' codec can't encode …` で止まり、失敗の内容が読めない。
- 原因: ビルドの出力にこの PC のコンソール（cp932）で出せない文字が混ざる。
- 対処: 起動時に `sys.stdout` / `sys.stderr` を `errors="replace"` にし直す（2026-09-16 に入れた）。新しい運用スクリプトも同じ（上の hook の件）。
- 確かめ方: スクリプトの先頭の `reconfigure` を見る。
- 出典: 01 記録の「既知の制約」と変更履歴（2026-09-16、タブレットの取り込みの項）。

### `Tools/overnight.py` が「セッションの中からは起動できない」で終わる（exit 4）

- 症状: Claude のセッションから `python Tools/overnight.py …` を走らせると、引数の確認の後に断られる。
- 原因: 意図どおり。環境変数 `CLAUDECODE` が立っている（Claude Code の中）と `claude -p` の入れ子になるので断る。
- 対処: 駆動役はユーザーの端末から起動する。セッションの中で確かめてよいのは `--dry-run` だけ（`.claude/settings.json` の allow もそれだけ）。
- 出典: 進捗記録 `20260917-autonomy.md` の決定事項（ステップ 2）、01 記録の表。

### 無人運転の反復が `Background tasks still running after 600s; terminating.` で終わり、状態ファイルが書かれない

- 症状: 反復の Claude が調査のサブエージェントをバックグラウンドで走らせ、「終わるのを待っています」と応答を終えると、600 秒後に上の文言で打ち切られる。コミットも状態ファイルも無く、その反復の調査は失われる。駆動役のログは「状態ファイル 書かれていない」、exit は 0。
- 原因: `claude -p`（駆動役の反復）は、応答を終えた後に残ったバックグラウンドの作業を最大 600 秒待ち、過ぎると打ち切って終わる（上限は環境変数 `CLAUDE_CODE_PRINT_BG_WAIT_CEILING_MS`。文言の案内どおり）。反復 2 の調査 3 本は 600 秒に収まらなかった。600 秒以内に終わった場合に、その知らせで応答が続くかは確かめていない（どちらでも頼らない）。
- 対処: **バックグラウンドの作業を残して応答を終えない**（`.claude/guides/autonomy.md` の「無人モード」）。サブエージェントは前面（`run_in_background: false`）で呼ぶ。並べるなら 1 つのメッセージに前面の呼び出しを並べる。`run_in_background` のコマンドは、応答を終える前に終わりを待って結果を読む。2026-09-18 から、駆動役はこの反復を Discord の報告で「⚠️ 成果なし」として出す（見出しとステータスの行。打ち切りの文言があれば原因の 1 行も）。
- 確かめ方: `Intermediate/Overnight/<日時>.log` の反復の終わりに `Background tasks still running after 600s` があるか。
- 出典: 2026-09-18 15:30 の無人運転の反復 2（作業一覧の項目 5 のステップ 2。調査 3 本が失われた）。

### `git checkout main` が `'main' is already checked out at '…/scratchpad/main-wt'` で失敗する / `git branch` の main に `+` が付く

- 症状: 作業ブランチを main へマージしようとすると、main が Claude の一時フォルダ（`%LOCALAPPDATA%\Temp\claude\<プロジェクト>\<セッション>\scratchpad\main-wt`）の worktree で開かれていて切り替えられない。その worktree の `git status` は大量の「staged の変更」を示す。
- 原因: 前のセッション（2026-09-17 の昼）が main 用の worktree を作り、後で main の参照だけを別の場所から進めた（`update-ref` のマージ）。worktree の索引とファイルは古い main（`a8ef1ad`）のままなので、進んだ HEAD との差が変更に見える。独自の作業は入っていない（`git -C <worktree> diff --cached a8ef1ad` が空、`git -C <worktree> diff` も空で確かめた）。
- 対処: 2026-09-17 23:37 にユーザーの許可を得て `git worktree remove --force <パス>` → `git worktree prune` で消した（解決済み。以後のマージは普通の `git checkout main` → `git merge --no-ff`）。消す前に、索引が古い main の木と同じ（`git -C <worktree> write-tree` = `git rev-parse a8ef1ad^{tree}`）で、未ステージ・未追跡の変更が無いこと（`git -C <worktree> status --short` が `M `・`D `・`A ` の行だけ）を確かめた。`git worktree remove` は変更ありとして断り、`--force` は変更を捨てる形の操作なので、**同じ症状がまた出たら、同じ確かめ方をしてからユーザーに消してよいか聞く**（無人では消さない）。消すまでのマージは、ファイルを動かさない形で行う: 作業ブランチで `git commit-tree HEAD^{tree} -p main -p HEAD -m …` → `git update-ref refs/heads/main <新> <旧>` → `git checkout --ignore-other-worktrees main`（木が同じなのでファイルは変わらない）→ `git branch -d <作業ブランチ>` → `git push origin main`。`--no-ff` のマージと同じ形になる。main は必ず作業ブランチの祖先であることを先に確かめる（`git merge-base --is-ancestor main HEAD`）。
- 確かめ方: `git worktree list` に `scratchpad/main-wt` が出るか。
- 出典: 2026-09-17 夜の無人運転（作業一覧の項目 2 のマージ）。

## エディタ・MCP・リモート実行

### エディタ（や本家のゲーム）が起動直後に落ちる: `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`

- 症状: `UnrealEditor.exe` を直接起動すると数秒で消える。ログに `LogD3D12RHI: Adapter has … 0 output[s]` と `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`。本家のゲーム（`DDeception-Win64-Shipping.exe`）も同じ。
- 原因: Claude Code が Windows のセッション 0（サービス側）で動いているとき、そこには GPU の出力が無く、D3D12 のスワップチェーンを作れない。
- 対処: エディタは `python Tools/editor_cycle.py`（自分のセッションとコンソールのセッションを比べ、違えば一度きりのスケジュールタスク `WasamiLaunchEditor` でログオン中のセッションに起動する。プリンシパルはユーザー名ではなく SID で指定する。ドメインなしの PC ではユーザー名が `WORKGROUP` になって登録できない）。本家や任意のプログラムは `python Tools/console_session.py <exe>`。`UnrealEditor.exe` を直接起動しない。
- 確かめ方: `console_session.py` が「started directly (session 1)」と出せば Claude 自身がセッション 1 にいる（2026-09-17 はそうだった。決めつけずに毎回見る）。起動の完了は `python Tools/ue_remote.py -c "print(1)"` が答えるか（終了コード 2 は応答なし）。`tasklist | grep -i UnrealEditor` で生死。
- 出典: コミット 6534683（2026-09-16）、01 記録の「既知の制約」、自動メモリ `editor-launch-needs-user`。

### MCP の `unreal-mcp` が `ENDPOINT_NOT_FOUND` で切れたまま / ポートに繋がるのにエディタが起きていない

- 症状: エディタを開き直した後、MCP のツールが `ENDPOINT_NOT_FOUND` などで失敗し、そのセッションでは戻らない。逆に、エディタが起きていないのに 127.0.0.1:8000 に繋がる。
- 原因: Docker Desktop（`com.docker.backend.exe`）が `0.0.0.0:8000` を掴んでいる。エディタが閉じている間に Claude Code が接続を試すと Docker に当たる。（2026-09-17 にユーザーが Docker を停止した。以後も出るなら Docker が動いていないかを先に見る）
- 対処: 開き直した後はユーザーに `/mcp` で再接続してもらう。それまでは `python Tools/ue_remote.py` で同じ作業ができる（`wasami_tools.pipeline` の関数を直接呼ぶ）。
- 確かめ方: エディタの生死はポートではなく `ue_remote.py` の応答で見る。
- 出典: `.claude/guides/unreal-workflow.md` の「環境」、01 記録。

### アセットの操作が `The Editor is currently in a play mode.` で失敗する

- 症状: 取り込み・保存・アセットの作成が上の文言で失敗する。
- 原因: PIE が動いたまま。
- 対処: `unreal.EditorLevelLibrary.editor_request_end_play()`（MCP なら `EditorAppToolset.StopPIE`）で止めてからやり直す。PIE は確認が終わったら必ず止める。
- 確かめ方: `unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()` が False。
- 出典: `.claude/guides/verification.md`、`/continue` の手順 3。

### エディタが `Assertion failed: !IsRooted()` で落ちる（取り込みがマテリアルを作り直したとき）

- 症状: `import_dd_tablet` などの取り込みで、マテリアルの式を消す `MaterialEditingLibrary.delete_all_material_expressions` の途中でエディタが落ちる。ログの末尾に `MarkAsGarbage` と `check(!IsRooted())`。テクスチャ・音・メッシュの取り込み直しは同じオブジェクトに書き戻すので落ちない。
- 原因: C++ のコンストラクタが `ConstructorHelpers` で読んだアセットは、エディタの起動時の読み込み（`GIsInitialLoad`）で `AddToRoot` され（UE 5.8 の `AsyncLoading2.cpp`）、消せない。
- 対処: パイプラインが作るアセット（`/Game/DD`・`/Game/Pipeline`）は C++ からソフト参照で持ち（`Source/wasami_deception/WasamiAssets.h`）、`BeginPlay` / `RebuildWidget` で読む。`ConstructorHelpers` は使わない。
- 確かめ方: エディタのコンソールで `obj refs name=<アセット名>` を見て `(root)` が付いていないこと（起動時に読まれる `WorldGridMaterial` には付く）。
- 出典: 2026-09-16 20:36 の落ち（進捗記録 `20260916-tablet-powers.md` の再開時の注意）、01 記録・03 記録の「既知の制約」、`.claude/guides/unreal-workflow.md`。

### 取り込みが作った Blueprint（カメラシェイク）の値が、同じセッションでは効かない（振幅 0・長さ 0）

- 症状: `dd_assets.camera_shake` で作り直したシェイクを、エディタを開き直さずに PIE で鳴らすと揺れない。ディスクから読み直した後（開き直し）は効く。
- 原因: Blueprint のクラスは、直前のコンパイルで親と違うと分かったプロパティだけをインスタンスへ写す（`UBlueprintGeneratedClass` の custom property list）。コンパイルの後に CDO を書いても一覧に入らない。
- 対処: CDO を書いた後にもう一度コンパイルする（`dd_assets.camera_shake` はそうしている）。クラスの既定値を書く取り込みを新しく作るときも同じ。
- 確かめ方: PIE で `player_camera_manager` の POV の揺れを標本で読む。
- 出典: 進捗記録 `20260916-tablet-powers.md` の決定事項（2026-09-17 ステップ 6）。

### 新しいツールセットのクラスが MCP に出ない（`reload_module` の後も）

- 症状: `Content/Python/wasami_tools` に新しいツールセットのクラスを足して読み込み直しても `list_toolsets` に出ない。
- 原因: `reload_module` が登録し直すのは前から登録されていたクラスだけ。
- 対処: `from toolset_registry._registry_interface import get_toolset_registry; get_toolset_registry().register_toolset_class(<クラス>)`（リモート実行で）。次の起動からは `init_unreal.py` が登録する。読み込み直しは MCP のツールからではなくリモート実行で行う（自分自身を読み込み直すと危ない）。
- 確かめ方: `list_toolsets`。
- 出典: `.claude/guides/unreal-workflow.md`、01 記録。

### Wasami のツールセットが全部 MCP に出ない / `from wasami_tools.pipeline import …` が `ToolCallMissingAnnotation: Type <class 'dict'>: missing specification for contained type.` で失敗する

- 症状: エディタを開き直すと `list_toolsets` に `WasamiDDTools`・`WasamiStageTools`・`WasamiDevTools` が無い。リモート実行で `wasami_tools` の下を読むと上の例外（`toolsets\stage.py` の `@toolset_registry.tool_call` から）。
- 原因: ツールの戻り値（か引数）の型注釈が素の `dict`（中身の型が無い）。クラスの定義ごと失敗し、`wasami_tools/__init__.py` の読み込みが止まる。読み込み直しの間は前のクラスが残るので、開き直すまで気付かない（2026-09-18 の `place_dd_sequences`）。
- 対処: `dict[str, int]` のように中身の型まで書く。値の型が混ざるなら JSON の文字列（`-> str`）を返す。
- 確かめ方: リモート実行で `import wasami_tools` が通る。開き直した後の `list_toolsets`。
- 出典: 01 記録の「登録」。

### `WasamiDDTools` が `list_toolsets` に出るのに道具を 1 つも持たない（**未解決**）

- 症状: `list_toolsets` に `wasami_tools.toolsets.dd.WasamiDDTools` は出るが説明が空で、`describe_toolset` が `{"version":"Unknown","description":"","tools":[]}` を返し、`call_tool` は `Tool '…import_dd_gimmicks' not found`。`WasamiDevTools`・`WasamiStageTools` は同じ呼び方で道具も説明も返る（2026-09-21）。
- 原因: 未解明。クラス自体は登録されているのに、道具の一覧と説明・版だけが落ちている。
- 対処: リモート実行で同じ関数を呼ぶ。`dd.py` の `_module` と同じく `paths`・`ue_props` と使う pipeline モジュールを `importlib.reload` してから `dd_gimmicks.import_all()`（か `import_doors_busted()` のような個々の取り込み）を呼べば、MCP と同じことができる。
- 確かめ方: `python Tools/ue_remote.py <file.py>` の戻り値。
- 出典: 進捗記録 `20260921-destruction-particles.md` のステップ 5。

### Python のツールセット（`WasamiDDTools` など）が `unreal.` の下に無い

- 症状: リモート実行で `unreal.WasamiDDTools` が `AttributeError`。
- 原因: Python で書いたツールセットは `unreal` モジュールには出ない。
- 対処: `from wasami_tools.toolsets.dd import WasamiDDTools` で読む。
- 出典: 01 記録。

### `ue_remote.py -c` が `FAILED: Could not load Python file 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/<コードの先頭>'` で失敗する

- 症状: `python Tools/ue_remote.py -c "$(cat 道具; echo; echo "main(...)")"` が、コードを実行せずに、コードの先頭から「.py」までをファイル名にした `Could not load Python file` を返す。
- 原因: リモート実行はファイルの実行のモードで送っている（`MODE_EXEC_FILE`）。PythonScriptPlugin は、文字列に「.py」があると、そこまでをファイル名、残りを引数と読む。コメントの中の「ue_remote.py」でも起きる。
- 対処: 送るコードに「.py」を書かない（使い方のコメントは「ue_remote -c で送る」のように書く）。
- 確かめ方: `grep -n "\.py" <道具>` が何も返さない。
- 出典: 2026-09-17、作業一覧の項目 3 のステップ 1（進捗記録 `20260917-shard-glow.md`。`observations/tools/shard_glow/collect.py`）。

### MCP のツールを並べて呼ぶとエディタが止まる

- 症状: 1 つの応答で複数の `call_tool` を並べると返ってこない。
- 原因: ツールはゲームスレッドで動く。
- 対処: 1 つずつ順に呼び、結果を確かめてから次を呼ぶ（失敗しても例外にならない道具がある）。
- 出典: `.claude/guides/unreal-workflow.md`。

### エディタが背面にあると PIE のティックが 3 fps ほどに落ちる（時間に依存する確認があてにならない）

- 症状: リモート実行から `LaunchCharacter` などで動かしても速さが出ない。Automation テストも進まない（テストの道具は前面で 10 fps を超えるまで、背面なら最大 600 秒待つ）。`UWidgetComponent` の画面（タブレットの地図）も描き直されない（`bTickWhenOffscreen` が偽）。
- 原因: `Use Less CPU when in Background`。
- 2026-09-20: **`desktop.py shot` に古いコマが写る**。背面のエディタはビューポートを描き直さないので、PIE のゲーム時刻が進んでいても画面は数秒前のまま（字幕のように数秒で消えるものを撮ると、出ていないように見える）。撮る前に下の対処でスロットルを切る。
- 2026-09-19: **ウィジェットのティック（`NativeTick` の `InDeltaTime`）も遅れる**。Slate は 1 コマの時間を 1/8 s で打ち切る（`FSlateApplication::TickTime`）ので、約 3 fps だとスコア画面の `ClearAnimation` も連続回収の画面（2 s で消える）も約半分の速さになった。ゲームの時間（`get_time_seconds`）は実時間どおり進むので気づきにくい。画面の時刻を収録で測る前に、エディタを前面にして 8 fps を超えているか（`unreal.SystemLibrary.get_frame_count()` の進み）を見る。
- 2026-09-22: **別窓の PIE（`PlayMode_InEditorFloating`）を撮ると、ユーザーのターミナルの窓が上に被る**。重なりはそのまま絵に写るので、明るさを測ると 2 倍以上に化ける（数字だけ見ていると気づかない）。測る前に縮めた絵を目で見る。
- 対処（前面に出す。2026-09-22）: `ctypes.windll.user32.SetForegroundWindow(hwnd)` は**ターミナルから走らせた Python でも通る**（同じ状況で `SetWindowPos` は 0 を返して失敗する）。`observations/tools/title_fit/raise_pie.py` が PIE の窓（題が `wasami_deception` で始まり `NetMode` を含む）を探して前へ出す。撮る前にこれを呼べば `desktop.py` の入力も通る。
- 対処: `python Tools/desktop.py click 2957 95 --allow UnrealEditor.exe`（タイトルバーの空き。エディタの窓が今の位置のとき。撮った画面でクリックの位置がエディタの上であることを先に見る）でエディタを前面にする。PIE を始めてもエディタは前面に来ない。
- 対処（前面に出せないとき。2026-09-19）: ユーザーのターミナルが前面だと `desktop.py` はクリックを断り（前面が許した窓でない）、MCP の `SlateInspectorToolset.Windows` の `select` も Windows に前面の切り替えを止められる。そのときはリモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` にする（クラスは Python の型として出ていないが、CDO は `find_object` で取れて書ける。メモリ上だけなので開き直すと戻る）。背面のまま PIE とテストが前面と同じ速さで進み、`desktop.py record` で約 58 fps で収録できた。終わったら `True` に戻す。前面の小窓（メッセージログ）は `SlateInspectorToolset.Windows` の `list` → `close`（番号）で閉じられる。
- 確かめ方: `stat fps`、またはリモート実行で `unreal.GameplayStatics.get_time_seconds` の進み。
- 出典: `.claude/guides/verification.md` の「動きの確認」（2026-09-16）、03 記録。「試して駄目だった案」も参照。

### `-NullRHI` の `UnrealEditor-Cmd.exe` で回すと、パーティクルを見るテストだけが落ちる

- 症状: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests <filter>;quit" -Unattended -NullRHI` で回すと、`Expected 'the dust' to be not null.`（`Wasami.Enemy.Actor.Chase06`）・`Expected 'both sparks going' to be true.`（`Wasami.Defib.Charge`）・`Expected 'P_ky_impact3 at the back plane, twice its size' to be true.`（`Wasami.ZoneBarrier.Actor`）が出る。同じテストはエディタの中では成功する。
- 原因: `-NullRHI` のワールドには FX システムが無いので、`SpawnEmitterAtLocation` などが `UParticleSystemComponent` を作らず null を返す。テストのほかの主張はすべて通る。
- 対処: パーティクルを見るテストはエディタの中で回す。エディタを前面に出せないとき（`PickerHost.exe` の「Windows セキュリティ」のような別プロセスの窓が前面を握っているとき）は、上の「エディタが背面にあると PIE のティックが 3 fps ほどに落ちる」の対処で `bThrottleCPUWhenNotForeground` を偽にしてから `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests <filter>')` を送り、`Saved/Logs/wasami_deception.log` の `Test Completed` を読む（終わったら真に戻す）。パーティクルを見ないテストは `-NullRHI` のままで速い。
- 確かめ方: 同じ `-NullRHI` の実行で、パーティクルを見ない `Wasami.Enemy.Actor.Sound`・`Defaults`・`Wasami.Defib.Actor`・`Hit` などは成功する。
- 出典: 2026-09-21 の作業一覧の項目 35 のステップ 1。

### Automation テストを始めると PIE が止まる

- 症状: PIE の確認中にテストを走らせると PIE が終わる。
- 原因: エンジンの仕様（テストの開始で PIE を止める）。
- 対処: テストと PIE の確認は続けて行い、同時に走らせない。テストの後に出る Automation のログの小窓は PIE の前に閉じる。
- 出典: `.claude/guides/verification.md`。

### PIE で Esc を押すとポーズ画面が出ずに遊びが止まる

- 症状: PIE のビューポートで Esc を押すと、ゲームのポーズ画面（`UWasamiPauseWidget`）が出ずに PIE が終わる。
- 原因: エディタのキー割り当て（`PlayWorld.StopPlaySession` の既定が Esc）が、ゲームより先にキーを取る。パッケージや `-game` では起きない。割り当てはユーザーのエディタの設定なので変えない。
- 対処: ポーズ画面は `python Tools/pie.py cmd "Wasami.Pause"` で開く（プレイヤーの `EscapePressed` と同じ道。止まっている間は開かない）。ボタンは `Tools/desktop.py click` で押す。
- 出典: 2026-09-19 の作業一覧の項目 18 のステップ 5（15 記録）。

### Zone 1 の PIE（や死亡の後の開き直し）で、始まる場所が毎回違う（`04_Start`・`05_Start`・`PlayerStart_1` など）

- 症状: `python Tools/pie.py start` の後の `player:` や、死亡画面の後に開き直した場所が、回ごとに別の PlayerStart になる。`(4295, −23330)`（タグ `PlayerStart_1`）のような、チェックポイントでない所から始まることもある。
- 原因: 本作のセーブ（`Saved/SaveGames/structSlot.sav`）のチェックポイントが、そのレベルに無い値（Zone 1 なのに Zone 2 の 7〜10）になっている。`AWasamiGameMode::ChoosePlayerStart` はタグの PlayerStart が無いと UE の既定に任せ、既定は空いている PlayerStart から無作為に選ぶ。Zone 2 を `Wasami.Checkpoint 8` などで確かめた後に、そのまま Zone 1 で PIE すると起きる。
- 対処: 確かめたいチェックポイントを PIE で `python Tools/pie.py cmd "Wasami.Checkpoint 5"` のように保存してから開き直す（Zone 1 は 4〜6）。最初からにするなら `Wasami.ResetSave`。
- 確かめ方: PIE で `unreal.GameplayStatics.get_game_mode(world).get_start_checkpoint()` を読む。
- 出典: 2026-09-19 の作業一覧の項目 9 のステップ 1（進捗記録 `20260919-capture.md`。セーブは 9 のままだった）。

### `capture_pose`（SceneCapture2D）の絵が PIE より暗い

- 症状: 同じ視点なのに `WasamiDevTools.capture_pose` は (13, 13, 0)、PIE は (41, 38, 25) と出る。
- 原因: シーンキャプチャは本編と同じには間接光を回さない。
- 対処: 設定どうしの A/B には使えるが、実機との数値の突き合わせは PIE で撮る（`HighResShot` か `Tools/desktop.py shot`）。PIE 中のリモート実行は `get_game_world()` を使う。
- 出典: コミット 97676a8（2026-09-16）、01 記録。

### `EditorAssetLibrary.delete_asset` が真を返すのにアセットのファイルが残る

- 症状: 18 個のアセットをまとめて消して 18 個とも真が返り、アセットレジストリからも消えたのに、テクスチャ 1 個（`chapter_ui_title_tormenttherapy.uasset`）だけ `Content/` にファイルが残った（更新時刻は取り込んだときのまま＝一度も消されていない）。`does_asset_exist` は真（ディスクを見る）なのに `list_assets`・`get_assets_by_package_name` は空、という食い違いが出る。
- 原因: 不明。消す対象は読み込まれておらず（`find_object` が `None`）、読み取り専用でもリダイレクタが残ったのでもなかった。同じ呼び出しで材質 17 個は消えている。
- 対処: 削除の後は**ディスクを見て確かめる**（`ls Content/…`）。残っていたらそのファイルを消す。そのままエディタを開き直すとレジストリが読み直してアセットが戻る。
- 出典: 2026-09-21 の作業一覧の項目 35 のステップ 5。

## Python（UE 5.8 の API の罠）

### ログが毎フレームの `LogPython: Error: … in tick` で埋まる（`module 'unreal' has no attribute 'unregister_slate_post_tick_handle'`）

- 症状: リモート実行の `-c` で登録した tick の関数が、自分を外すところで落ち続け、毎フレーム同じトレースバックを出す（2026-09-19 は 03:43 から 1 時間余りで 7 万行。外した後の処理も走らない）。
- 原因: 外す関数の名前の誤り。正しくは `unreal.unregister_slate_post_tick_callback(handle)`（登録は `register_slate_post_tick_callback`）。
- 対処: 残った関数をリモート実行で探して外す: `gc.get_objects()` から `__name__ == 'tick'`・`__code__.co_filename == '<string>'` の関数を拾い、閉包（`__closure__`）の中のハンドル（`_DelegateHandle`）を `unregister_slate_post_tick_callback` に渡す。
- 確かめ方: `Saved/Logs/wasami_deception.log` の行数が数秒で増えなくなる。
- 出典: 項目 7 のステップ 5b（2026-09-19。読み込み画面の紋章の確かめで残ったもの）。

### `MaterialEditingLibrary.delete_all_material_expressions` が式を半分しか消さない

- 症状: 式を消して組み直したマスターに、出力につながらない式の残骸が残ってコンパイルされる（2026-09-17 に 8 つのマスターで見つけた。絵は変わっていなかった）。
- 原因: `MaterialEditingLibrary.cpp` の `DeleteAllMaterialExpressions` が、消しながら同じ配列を回す。
- 対処: 空になるまで繰り返す（`dd_stage.clear_expressions`）。
- 確かめ方: `MaterialEditingLibrary.get_num_material_expressions` が 0。
- 出典: 01 記録。

### `ComponentMask` の R と G が既定で真（1 チャンネルのつもりが 2 チャンネル）

- 対処: `r` / `g` / `b` / `a` の 4 つとも明示的に書く（`dd_shards._channel`、`dd_tablet`）。
- 出典: 01 記録。

### `DynamicParameter` の出力に名前でつなげない（`connect_material_expressions` が False）

- 原因: 出力名は `GetOutputs()` が呼ばれるまで空で、`param_names` を入れた直後は名前が無い。
- 対処: `get_material_expression_output_names` を一度呼んでからつなぐ（`dd_shards._dynamic_parameter`）。
- 出典: 01 記録。

### `connect_material_expressions` が失敗しても例外にならない / 入力が 1 本のノードにつながらない

- 症状: つないだつもりの式が外れていて、コンパイル時に `Missing … input` になる。
- 原因: `MaterialEditingLibrary` は失敗を False で返すだけ。入力が 1 本のノード（`Frac`・`Saturate`・`Ceil`・`ComponentMask`）のピン名は `""`（`"Input"` は失敗する）。`Desaturation` の最初の入力も名前が無い（`get_material_expression_input_names` が `['None', 'Fraction']`。2026-09-17 のステップ 6）。
- 対処: `dd_stage._Graph(mat, checked=True)` で例外にする。ピン名は `""`。
- 出典: 03 記録、01 記録（`_Graph`）。

### bool を返し出力引数を持つ UFUNCTION が、Python では `None` か出力引数だけを返す

- 症状: 失敗の理由（出力引数の文字列）が取れない。
- 対処: 理由を戻り値で返す形にする（`SetPropertyText` は書けたら空文字、書けなければ理由）。
- 出典: 01 記録（2026-09-17）。

### `ImportText` が構造体のテキストの知らないメンバーを黙って読み飛ばす

- 症状: 値を書いたつもりで、読み戻すと既定のまま。ログは `UE_SUPPRESS(LogExec, Verbose, …)` でしか言わない。
- 対処: `SetPropertyText` は入れ子の構造体までメンバー名を先に照合する（`CheckStructText`。ネイティブの取り込みを持つ構造体は除く）。
- 出典: 01 記録。

### `unreal.Rotator(a, b, c)` の位置引数が (roll, pitch, yaw) の順

- 対処: 必ず `unreal.Rotator(roll=…, pitch=…, yaw=…)` と名前で渡す。
- 出典: `.claude/guides/unreal-workflow.md`。

### Python から触れないプロパティ・無いライブラリ

- `decal_blend_mode`（UE 5.8 で非推奨、読めない）: いまの UE はつないだ出力で DBuffer のチャンネルが決まるので、基本色と不透明度だけをつなぐ（01 記録）。
- `MaterialExpressionIf` の `ConstAGreaterThanB` など: 扇形のマスクは `ceil(saturate(…))` で作る（03 記録）。
- `SlateBlueprintLibrary`（画面上の大きさ）: ウィジェットのパス（`get_user_widget_object()` のパス + `.WidgetTree.<名前>`）を `find_object` して `render_transform` と `get_render_opacity()` を読む（進捗記録 `20260916-tablet-powers.md` の再開時の注意）。
- **ウィジェットの画面上の位置（`get_cached_geometry()` / `get_tick_space_geometry()` / `get_paint_space_geometry()`）**: Python の名前は `unreal.SlateLibrary` で関数はあるが、Python が受け取る `Geometry` は空の写し（反映されたフィールドが無い）で、`SlateLibrary.get_local_size` は 0、`local_to_viewport` はビューポートの左上を返す。C++ の `unreal.WasamiWidgetProbe.viewport_fraction(widget, unreal.Vector2D(0.5, 0.5))`（ビューポートの大きさに対する割合。描かれていなければ (-1, -1)。09 記録）で読む。`Tools/playthrough.py` の `click_part` がこれで画面のボタン・スライダーを押す（2026-09-19。エディタの開き直し 1 回）。
- `SkeletalMeshSocket` の `socket_name`・`bone_name`（読むだけ）: ソケットをメッシュを outer に作って `set_socket_parent(mesh, 骨)` で骨を決め、`mesh.add_socket(socket, True)`（UE 5.8 は `Socket` と名付けてメッシュの一覧に入れ、骨格に写しを足す）→ `mesh.rename_socket("Socket", 名前)`（両方の名前が変わる）。消すのは `remove_socket(名前)`（両方から消える）。`dd_skeletal.add_sockets`（01 記録。2026-09-19）。
- `Use Less CPU when in Background`（`EditorPerformanceSettings`）: Python から見えない。エディタを前面にする（上）。
- `WidgetBlueprintLibrary`（`GetAllWidgetsOfClass`）: `unreal.WidgetBlueprintLibrary` は無い（`module 'unreal' has no attribute 'WidgetBlueprintLibrary'`）。`unreal.WidgetLibrary.get_all_widgets_of_class(world, cls, False)` で呼べる（`Tools/playthrough.py` の脱出の見分け。2026-09-19）。
- **ウィジェットを作る `Create`（`CreateWidget`）**: `unreal.WidgetLibrary.create` は無い（`type object 'WidgetLibrary' has no attribute 'create'`。K2 専用）。`unreal.new_object(cls, outer=pc)` → `add_to_viewport` は画面に載る（`is_in_viewport()` が True）が、**C++ で木を組むウィジェットは何も映らない**: 本作のウィジェットは `RebuildWidget` で `WidgetTree` があるときだけ木を組み、`WidgetTree` を作る `Initialize` は `CreateWidget` の中か、`Super::RebuildWidget` の中（木を組む判定の後）でしか呼ばれないので、空の木のまま載る。対処: そのウィジェットの BlueprintCallable の `Show(WorldContext)`（`CreateWidget` → `AddToViewport`。タイトル・オプション・EXTRAS の画面にある）を `unreal.WasamiExtrasWidget.show(world)` のように呼ぶ。無ければ足してビルドする（2026-09-20。エディタの開き直し 1 回。19 記録）。

### 毎フレームのコールバック（`register_slate_post_tick_callback`）が例外で黙って外れ、記録を失う

- 対処: 始める前に記録の関数を 1 回そのまま呼んで通ることを確かめる。終わったら `unregister_slate_post_tick_callback`。scratchpad の補助スクリプトはセッションごとに消えるので、要るときは作り直す。
- 出典: 進捗記録 `20260916-tablet-powers.md`（ステップ 8 で 1 回目の使用の記録を失った）。

### `register_slate_post_tick_callback` の dt が実時間でもゲーム内時間でもない（`slomo` を掛けた収録の時刻がずれる）

- 症状: `slomo 0.25` で PIE を撮るとき、slate の post tick に来る `dt` を足して「実時間 0.8 s」で撮った絵が、場面では 0.54 s だった（ゲーム内時間は Claude の足した値の約 0.67 倍で進む。実時間の 0.25 倍でも 1 倍でもない）。撮った絵の時刻を実時間だと思って読むと、暗転の始まりを「早すぎる」と読み違える。
- 原因: slate の tick に渡るのは `FApp::GetDeltaTime()` 系の値で、`GlobalTimeDilation` の掛かり方がゲームのワールドの時間と違う（クランプも入る）。
- 対処: **時刻はワールドから読む** — 毎フレーム `unreal.GameplayStatics.get_time_seconds(world)` を読み、演出の始まりの値との差で撮る（`tmp/cap_shoot.py` がこの形。捕獲の別室の 4 本を撮った）。
- 確かめ方: 撮った各フレームで `get_time_seconds` の差をログに出し、狙った場面の時刻と合っているかを見る。
- 出典: 2026-09-21 の作業一覧の項目 32 のステップ 5（進捗記録 `20260921-capture-room-fidelity.md`）。

### 画面に載ったウィジェットの数が多く出る

- 症状: `unreal.WidgetLibrary.get_all_widgets_of_class(world, cls, False)` が、外したウィジェットも GC まで数える。
- 対処: 第 3 引数（top level only）を True。
- 出典: 進捗記録 `20260916-tablet-powers.md` の再開時の注意。

## C++・ビルド・テスト

### `UPostProcessComponent`・`ULegacyCameraShake`・Cascade のクラスを継ぐ / 呼ぶとリンクに失敗する（MinimalAPI）

- 症状: `unresolved external symbol` でリンクが落ちる。
- 原因: `MinimalAPI` のクラスは他のモジュールから `ENGINE_API` の関数と仮想関数しか使えない。
- 対処: 継がない。ポストプロセスは `UPostProcessComponent` を作って持つ（`UWasamiChameleonComponent`）、カメラシェイクは Blueprint として作る（`WasamiDDTools`）、Cascade は `UParticleEmitter::Build` を `UpdateModuleLists` 経由で呼ぶ。`UParticleModule` は `Within=ParticleSystem` で、外へ出すときは `GetTransientOuterForRename` が一時的なシステムを外側にする。
- 出典: 02 記録・04 記録・01 記録の「既知の制約」、進捗記録 `20260916-tablet-powers.md`（ステップ 3 で一度ビルドが落ちた）。

### タイマーのコールバックの中で `IsTimerActive` が真を返す（空くまで待つループが自分を「待ち中」と誤る）

- 症状: 「鳴り終わるまで 0.5 s ごとに待つ」ループを単発タイマーで組むと、タイマーが明けた回が「もう待ちが走っている」と判断して何もせず、以後は二度と鳴らない（2026-09-20、`AWasamiBierceTalk` の自動テスト `Wasami.Dialogue.Talk` が落ちた）。
- 原因: `FTimerManager::IsTimerActive` は `Status != Paused` を返すので、**自分のコールバックの最中（`Executing`）も真**。
- 対処: コールバックからループへ戻る入り口には「待ちは無い」を値で渡す（`AWasamiBierceTalk::Step(bLookAtHalt, bWaiting)`。`Resume` は `Step(false, false)`、外からの `Talk` だけ `IsWaiting()` を見る）。
- 出典: 10 記録の「話し役 `AWasamiBierceTalk`」（作業一覧の項目 20 のステップ 2）。

### ファイルを足したらビルドが落ちた: 無名名前空間の名前の衝突・C4458

- 症状: 変えていないファイルで再定義や曖昧な参照のエラー、または `warning C4458: declaration of 'Slot' hides class member`（警告がエラー扱い）。
- 原因: ユニティビルドでファイルのまとまり方が変わり、無名名前空間の同じ名前（`WaveVolume`・`WavePitch`・`FadeKeys`・`EnemyTag`・`VignetteScale`）が 1 つの翻訳単位に入る。C4458 はローカル変数が `UWidget::Slot`・`UUserWidget::bInitialized` などを隠す。
- 対処: 定数や補助の名前はファイルごとに固有にし、UE のメンバー名と同じローカル変数を避ける。ファイルを足さなくても、ヘッダーを 1 つ変えて再コンパイルの範囲が変わるだけで起きる（2026-09-18: `WasamiEnemyAnimInstance.h` の定数を変えたら `WasamiEnemy.cpp` と `Tests/WasamiTestEnemy.cpp` の `EnemyTag` がぶつかった。テスト側を `TestEnemyTag` にした）。
- 2026-09-18: 新しいファイルを 7 つ足したら、既存のファイル同士（`WasamiPopUpWidget.cpp` と死亡画面の `ButtonGrey`・`Place`、パワーの `OpacityKeys`、`WasamiTelepathyTrackerWidget.cpp` の自前の `FAnimKey` と `using WasamiWidgetAnimation::FAnimKey`、`WasamiBlackFadeWidget.cpp` と死亡画面の `FadeInKeys`）がぶつかって 2 回落ちた。**先に重複を洗い出すと 1 回で済む**: 各 `.cpp` の `namespace { … }` の中の `const`/`constexpr` の名前・関数名・`struct` 名を集め、2 つ以上のファイルにあるもの（`using` の宣言は同じ実体なので除く）を片方で固有の名前に改める。
- 2026-09-18: **エンジンのヘッダーの引数名ともぶつかる**（`error C4459: declaration of 'BoxExtent' hides global declaration`）。無名名前空間の名前はその翻訳単位では大域に見えるので、同じ塊に入った `Kismet/KismetMathLibrary.inl` の `BreakBoxSphereBounds(…, FVector& BoxExtent, …)` が `WasamiDoorBreak.cpp` の `BoxExtent` を隠すと言われた（`WasamiDoubleDoors.cpp` を足して塊が変わった）。`BoxExtent`・`ComponentScale` のような一般の名前は避け、ファイルの頭字を付ける（`DoorBreakBoxExtent`・`DoorsLeaveScale`）。
- 2026-09-19: **未コミットのファイルは塊の外でコンパイルされるので、ビルドが通ってもコミットの後に落ちることがある**（UBT の適応ユニティビルドは、git で変更中のファイルをユニティの塊から外す）。項目 7 のステップ 5a で足した `WasamiViewcone.cpp` の `MinimapTag`・`OpacityName` が `WasamiPlayerCharacter.cpp`・`WasamiPrimalPower.cpp` とぶつかっていたのに、ステップ 5a のビルドは通り、コミットした後の次のビルドで落ちた。C++ のファイルを足したら、コミットの前に上の洗い出しをする（`python Tools/check_unity_names.py`。2026-09-19 から。引数の違う同じ名前の関数は多重定義として数えない）。落ちた後に直すときは、直しを先にコミットしてからビルドすると、塊が本来のまとまり方になって残りのぶつかりも出る。
- 2026-09-19: 作業一覧の項目 10 のステップ 4 で、ステップ 2・3 でコミットした `WasamiVignetteSidesWidget.cpp`（`VignetteScaleKeys`・`SetScale` ほか。`WasamiShardStreakWidget.cpp` と）と `WasamiStunCollectEffect.cpp`（`GrowthKeys` ほか。`WasamiPrimalPower.cpp` と）が、ファイルを足した次のビルドで落ちた。同じ遅れの落ち方で、洗い出しを道具にした。
- 2026-09-20: 作業一覧の項目 29 のステップ 5 で、ステップ 1 と 4 で足した名前（`WasamiCollectable.cpp` の `MakeBounceCurve` が `WasamiTabletWidget.cpp` と、`WasamiExtrasWidget.cpp` の `ExtrasPlace` が `WasamiExtrasItemWidget.cpp` と〈既定の引数の違う多重定義は呼び出しが曖昧になる〉）が、タイトル画面を変えた次のビルドで落ちた。どちらも洗い出しの道具なら拾えたが、走らせていなかった。**`Tools/editor_cycle.py` がビルドの前に `check_unity_names.py` を走らせ、ぶつかりがあればエディタを閉じずに止まる**ようにした（2026-09-20）。
- `Tools/editor_cycle.py` はビルドに失敗するとエディタを閉じたままにする。直したら `python Tools/editor_cycle.py --no-quit` でビルドして開く。
- 出典: 04 記録の「既知の制約」と「確かめたこと」（ステップ 7・8）、進捗記録 `20260917-enemy-wasami-body.md` のステップ 4、11 記録（作業一覧の項目 6 のステップ 1・3c）。

### Automation テストで、タイマー（`Delay`・`SetTimer`）が進まない

- 症状: テストのワールドで `TickTestWorld(7.1)` のように長く進めても、7 s のタイマーが発火しない。0.05 s のタイマーは発火する。
- 原因は 2 つ: (1) ティックとティックの間（テストの本文）で置いたタイマーは保留（`FTimerManager` の `PendingTimerSet`）になり、次のティックの**終わり**で有効になるだけで、そのティックの分は進まない。(2) ワールドのティックの経過は `AWorldSettings::FixupDeltaSeconds` が `MaxUndilatedFrameTime`（0.4 s）で切る。`FTestWorldWrapper::TickTestWorld` は 1 回ごとに `GFrameCounter` を進めるので、フレームの重なりは原因ではない。
- 対処: 0 秒のティックを 1 回挟んでから、0.1 s 刻みでティックする（`Tests/WasamiZoneFlowTests.cpp` の `Advance`）。
- 出典: 11 記録の「既知の制約」（作業一覧の項目 6 のステップ 1。ビルドし直し 2 回）。

### Automation テストで、一時的なワールドのアクタがイベントを捨てる

- 症状: テストのワールドに置いた的が、パワーの作用（インターフェースの呼び出し）を受けない。
- 原因: アクタが初期化前（`InitializeActorsForPlay` を通っていない）。
- 対処: ワールドを作ったら `InitializeActorsForPlay` を呼ぶ（`Wasami.Powers.PrimalStun`）。
- 出典: 進捗記録 `20260916-tablet-powers.md` の検証（ステップ 6）。

### Automation テストで、タイマーが 1〜2 刻み遅れて発火する（手で進めるワールド）

- 症状: `FTestWorldWrapper::TickTestWorld` を 0.0625 s 刻みで回すと、BeginPlay で入れた 0.5 s のループのタイマーが 0.5 s ではなく 0.625 s の更新で発火し、発火の中で入れた 17 s のタイマーの残りが 1 刻み長い（`Expected … to be 17.250000, but it was 17.312500`）。
- 原因: UE 5.8 の `FTimerManager`（UE4 も同じ）は、(1) 期限を**過ぎた**最初の更新で発火する（`InternalTime > ExpireTime`。ちょうど同じ時刻では発火しない）、(2) 更新の外（BeginPlay・テストの本文）や発火の処理の中（`LastTickedFrame` がまだ前のフレーム）で入れたタイマーは保留になり、その更新の終わりの `InternalTime` を足して数え始める。刻みが 2 進数で割り切れると期限がちょうど更新の時刻に重なり、(1) の 1 刻みが必ず出る。
- 対処: 実装は直さない（エンジンの規則）。テストの期待の時刻を規則に合わせて書く（`Wasami.Enemy.Actor.Stun` の冒頭の注釈）。`GetTimerRemaining` は更新の間では `ExpireTime − InternalTime`（保留中は入れた秒数そのもの）。
- 確かめ方: エンジンの `Engine/Source/Runtime/Engine/Private/TimerManager.cpp` の `Tick`（`InternalTime > Top->ExpireTime`、末尾の `PendingTimerSet` の `ExpireTime += InternalTime`）。
- 出典: 07 記録の「エンジンのタイマーの刻み」、進捗記録 `20260917-enemy-wasami-body.md` のステップ 3（2026-09-18。期待の時刻を直すのにビルドを 1 回やり直した）。2026-09-20 にも `Wasami.Secrets.Collectable.Save`（書類の 0.2 s の `Delay` を 0.05 s 刻みで。0.25 s ではまだ、0.3 s で発火）で同じ 1 刻みに当たり、ビルドを 1 回やり直した（18 記録）。

### Automation テストが、ほかと続けて流すときだけ落ちる（弱い参照で持った文の枠が消える）

- 症状: `Wasami.ZoneBarrier.Interact`・`Wasami.RingStatue.Interact` が単独では通るのに、`Wasami.` を全部流すと `Expected 'nor at 4.8 s' to be true.` で落ちる（最初の文の枠と違うものが返る）。
- 原因: 障壁・祭壇は出した文の枠を `TWeakObjectPtr` で持つ。テストのワールドにはビューポートが無く枠を持つものが無いので、ティックの途中の GC が枠を回収し、弱い参照が空になる（空いたメモリに次の枠が入ることもある）。GC が走るかは前に流したテストで決まる。
- 対処: テストで最初の枠を `TStrongObjectPtr` で持つ（両方のテスト。実装は直さない: ゲームではビューポートが枠を持つ）。
- 出典: 項目 8 のステップ 5（障壁、2026-09-19）とステップ 8（祭壇、同日）。

### Automation テストで、静的メッシュの部品にワールドのトレースが当たらない（箱の部品には当たる）

- 症状: テストのワールド（`FTestWorldWrapper`、`EWorldType::Game`）に出したアクタの `UStaticMeshComponent`（`BlockAllDynamic`・`Visibility` は block・物理の状態あり・`GetBodyInstance()->IsValidBodyInstance()` 真）に、`World->LineTraceSingleByChannel(…, ECC_Visibility)` が何も返さない。ティックを進めても、アクタを回して境界に厚みを持たせても、斜めに撃っても同じ。同じワールドの `UBoxComponent`（厚み 0 の箱も）には当たる。
- 原因: 未解明（テストのワールドのシーンの問い合わせが静的メッシュの体を拾わない）。ゲームには関係しない: エディタのワールドと PIE では同じメモ（`AWasamiMysteryCollectable` の `Plane` = エンジンの `Plane`、当たりは厚み 0 の箱）に当たる（PIE は `summon WasamiMysteryCollectable` で出して確かめた）。
- 対処: テストでは部品の体へ直に撃つ `UPrimitiveComponent::LineTraceComponent` で確かめる（`Wasami.Secrets.MysteryCollectable`）。ワールドのトレースで確かめたいときは PIE で。
- 出典: 18 記録の「既知の制約・注意点」、作業一覧の項目 12 のステップ 4（2026-09-20。調べるのにビルドとエディタの開き直し 4 回）。

### ヘッダーや UCLASS / UPROPERTY の変更が Live Coding で効かない

- 対処: `python Tools/editor_cycle.py`（保存 → 閉じる → UBT → 開き直す）。尋ねずに走らせる。Live Coding で直したファイルは次のフルビルドで取り込まれる。
- 出典: `.claude/guides/unreal-workflow.md`。

### 歩くとタブレットがシェイクと逆に揺れる（カメラの子にした部品）

- 症状: 歩行のカメラシェイクの間、カメラの子の板が画面上で逆方向に揺れる。
- 原因: UE のカメラシェイクはカメラマネージャの視点（POV）にだけ掛かり、`UCameraComponent` は動かない。
- 対処: アクタのティックを `TG_PostUpdateWork` に移し、毎フレーム POV に視点空間の位置を掛けて板のワールド変換を置き直す（`PlaceTablet`）。
- 確かめ方: シェイク中に POV が揺れる間、板の視点空間の位置が不動。
- 出典: コミット bdb11ef（2026-09-16）、02 記録。

### マウスの視点移動が本家の約 1/14 と遅い（Enhanced Input の感度が 0.07 × 0.07 になる）

- 症状: `Tools/desktop.py look --dx 100` で 1.22° しか回らない（本家の式の期待値は 17.5°）。縦も同じ比。実行中に対応づけの `UInputModifierScalar` を書き換えても変わらない。
- 原因: Enhanced Input の `ApplyAxisPropertyModifiers`（UE 5.8 `EnhancedInputSubsystemInterface.cpp`）が、マウスのキー（`Mouse2D` を含む。CVar `input.GlobalAxisConfigMode` の既定 0）の対応づけに、旧入力の `AxisConfig` の感度（`DefaultInput.ini` の Mouse2D 0.07）を Scalar 修飾子として自動で先頭に足す。対応づけに自分で Scalar を足すと重なる。修飾子はプレイヤーの入力へ `DuplicateObject` で写されるので、IMC の持ち主の下の修飾子を書き換えても効かない（写しは `/Engine/Transient.InputModifierScalar_N`）。
- 対処: 感度は `AxisConfig` の側だけに置き、対応づけに Scalar を足さない（作業一覧の項目 2 のステップ 3）。
- 確かめ方: PIE で `pie.py place` の後に `look --dx 1000 --allow UnrealEditor.exe` → `pie.py state` のヨーの差が 175°（0.175°/カウント、FOV 90）。実行中に試すなら `unreal.ObjectIterator(unreal.InputModifier)` で `/Engine/Transient` の写しを探して書き換える。
- 出典: 02 記録、`observations/README.md` の「視点の速さと集中線」（2026-09-17。直した後の PIE で dx 100 → 17.5°、dx 2057 → 359.94°）。

### テストの世界で `SetViewTargetWithBlend` を 2 回呼ぶとエディタが落ちる（EXCEPTION_STACK_OVERFLOW）

- 症状: Automation テストが途中で止まり、8 秒後にエディタが `Unhandled Exception: EXCEPTION_STACK_OVERFLOW` で落ちる。呼び出し履歴は `UnrealEditor-Engine.dll` と `UnrealEditor-CoreUObject.dll` だけの 7 フレームの繰り返しで、こちらのモジュールが 1 つも出ない。場面の再生（`PlayCutscene` が `SetPlayerViewTarget(シネカメラ, 0.5)`）の後に、終わりで視点をプレイヤーへ戻す（`SetPlayerViewTarget(プレイヤー, 0)`）と起きる。
- 原因: `World->SpawnActor<APlayerController>()` で作ったコントローラーは `ULocalPlayer` を持たないので `IsLocalPlayerController()` が偽。すると `APlayerCameraManager::AssignViewTarget` が `PCOwner->ClientSetViewTarget(...)` を呼び（UE 5.8 `PlayerCameraManager.cpp`）、ネットが無いので同じプロセスで `SetViewTarget` に戻る。混ざりかけ（`PendingViewTarget.Target` が残っている）のところへ同じ視点を入れ直すと、`SetViewTarget` の else の枝がまた `ClientSetViewTarget` を呼び、`PendingViewTarget.Target = NULL` はその後なので、無限に潜る。実機・PIE・パッケージではコントローラーが local なのでこの枝に入らない。
- 対処: テストの世界に素の `APlayerController` を置いたまま、視点を混ぜる game のコードを走らせない。視点とスキップの画面（`CreateWidget` も local なコントローラーを要る。`PIE: Error: ローカルプレイヤーコントローラーのみを…` が出る）は PIE で見る。
- 確かめ方: 落ちる所は `UE_LOG(LogTemp, Display, …)` の印を疑う行の前後に置いて `Saved/Logs/wasami_deception.log` で読む（自動化のログは落ちる直前まで残る）。
- 出典: 作業一覧の項目 25 のステップ 3（2026-09-20）。

### `BuildCookRun` が `BUILD FAILED` `AutomationTool exiting with ExitCode=25 (Error_UnknownCookFailure)`（クックは `Done!` まで進んでいる）

- 症状: パッケージのコマンドが `Cook failed.` で止まる。ログを遡ると `LogCook: Display: Done!` まで進んでいて、直後の `Warning/Error Summary` が `Failure - 2 error(s), 1 warning(s)`。エラーは 2 件とも同じ原因で、`LogGameFeatures: Error: Asset manager settings do not include a rule for assets of type GameFeatureData, which is required for game feature plugins to function` と、その日本語版（`LoadErrors: Error: アセット マネージャー設定に、ゲーム機能のプラグインが…`）。
- 原因: クックはエディタのコマンドレット（`UnrealEditor-Cmd.exe -run=Cook`）で走るので、**エディタ専用のプラグインも読み込まれる**。MCP のツールセット `AllToolsets` が `GameFeaturesToolset` を要求し、それが `GameFeatures` プラグインを有効にする。`GameFeatures` は起動時（UE 5.8 `GameFeaturesSubsystem.cpp` の `OnAssetManagerCreated`）に `GameFeatureData` 型の `FPrimaryAssetRules` が既定のままだとエラーを 1 件出す。クック自体は成功しているのに、コマンドレットは**エラーが 1 件でもログに出ると失敗を返す**ので UAT が落ちる。本作は Game Feature プラグインを 1 つも使っていない。
- 対処: `Config/DefaultGame.ini` に `[/Script/Engine.AssetManagerSettings]` の `+PrimaryAssetTypesToScan=(PrimaryAssetType="GameFeatureData",…,bIsEditorOnly=True,Rules=(Priority=1,…,CookRule=Unknown))` を 1 行足す（実物はその ini のコメント付き）。`Priority` を既定の `-1` から変えるのが肝で（`FPrimaryAssetRules::IsDefault()` が偽になればよい）、`Directories` は空・`CookRule=Unknown` なので何も走査せずクックの中身は変わらない。`bIsEditorOnly=True` にすると、パッケージした本編は `AssetManager.cpp` の `ShouldScanPrimaryAssetType`（`bIsEditorOnly && !GIsEditor`）で項目ごと読み飛ばすので、本編に入っていない `GameFeatures` モジュールのクラスを読もうとして ensure が出ることもない。
- やらない案: UAT の `-IgnoreCookErrors`。本物のクックのエラーまで黙って通してしまう。
- 確かめ方: 直した後のログの `Warning/Error Summary` が `Success - 0 error(s)` になり、最後が `BUILD SUCCESSFUL`。
- 出典: 作業一覧の項目 36 のステップ 2（2026-09-21。初回のパッケージ）。

### Mac のパッケージを起動すると「書類フォルダへのアクセス」を聞かれ、どちらを押しても反応しない

- 症状: `~/Documents` の下に出来た `.app` を開くと macOS の確認ダイアログが出るが、「許可」「許可しない」のどちらを押しても何も起きず、ゲームは起動しないまま固まる。
- 原因: macOS の TCC は `~/Documents`・`~/Desktop`・`~/Downloads` の中のファイルを読むアプリに確認を出す。**`.app` 自身がそこに置かれていると、自分の中身（pak）を読むだけでも聞かれる**。そのときゲームは全画面で画面と入力を掴んでいるので、システムのダイアログにクリックが届かない（どちらのボタンも効かないように見える）。
- 対処: **`~/Documents` の外にパッケージを置く**。`Tools/mac_build.sh` は `~/Applications/WasamiDeception`（TCC の保護対象外）に出す（`--archive <置き場所>` か環境変数 `WASAMI_ARCHIVE_DIR` で変えられる）。既に出来ている `.app` は `cp -R` で移すだけでよい。クックとステージはプロジェクトの中（`Saved/Cooked`・`Saved/StagedBuilds`）に残るので、差分は効いたまま。
- 固まったときの止め方: `pkill -f wasami_deception`。
- 様子を見ながら起動する: `"<app>/Contents/MacOS/wasami_deception" -windowed -ResX=1280 -ResY=720`（窓で出て、端末にログが流れる）。
- 出典: 2026-09-22、Mac で初めて起動したとき。

### Mac のパッケージした `.app` が起動の瞬間に落ちる（`Library not loaded: @rpath/libtbb.12.dylib`）

- 症状: パッケージは `BUILD SUCCESSFUL` で中身の検査も通るのに、`.app` を開くと即クラッシュ。クラッシュレポートは `Termination Reason: Namespace DYLD, Code 1, Library missing` / `Library not loaded: @rpath/libtbb.12.dylib`。`find <app> -name '*.dylib'` が**何も返さない**（1 つも入っていない）。
- 原因: 2 つ重なっている。
  1. UE 5.8 の Mac のステージが、本体が `@rpath` で読む ThirdParty の dylib を `.app` に入れない。本作の本体は 3 つ読む: `libtbb.12.dylib`・`libtbbmalloc.2.dylib`・`libmetalirconverter.dylib`（`otool -L` で分かる）。**Windows は同じ TBB を `Binaries/Win64/tbb12.dll`・`tbbmalloc.dll` として入れている**ので、入れるのが本来の姿。
  2. 本体に焼かれた「エンジンへ戻る」rpath（`@loader_path/../……×8/Shared/Epic Games/UE_5.8/Engine/…`）は **`<Project>/Binaries/Mac/` に置かれた `.app` の深さ**を前提にしている。`Saved/Archive/Mac/` の成果物はそれより 1 階層深いので、`/Users/<user>/Shared/Epic Games/…` という存在しない場所を指す。だから「同梱もされず、近道も届かない」。
- 対処: `Tools/mac_build.sh` がパッケージの後に、`otool -L` で `@rpath` の dylib を拾い、`.app` に無いものをエンジンから探して `Contents/MacOS/`（rpath の `@loader_path/`）へ複製し、**ad-hoc で署名し直す**（Apple Silicon は中身を書き換えた bundle を署名なしでは起動しない）。dylib が dylib を呼ぶ分も拾えるように、増えなくなるまで最大 4 周する。
- 置き場所の注意: **TBB はエンジンの `Binaries/ThirdParty` には無い**。`Engine/Source/ThirdParty/Intel/TBB/Deploy/oneTBB-<版>/Mac/lib`（2026-09-22 は `oneTBB-2022.3.0`）にある。`libmetalirconverter.dylib` は `Engine/Binaries/ThirdParty/Apple/MetalShaderConverter/Mac`。
- 確かめ方: `otool -L <app>/Contents/MacOS/wasami_deception | grep @rpath` に出るものが、すべて `<app>/Contents/MacOS/` にあること。
- 出典: 2026-09-22、Mac で初めてパッケージを起動したとき。

### Mac のパッケージが `ExitCode=25 (Error_UnknownCookFailure)` で落ちる（エラーは `HttpListener unable to bind to 127.0.0.1:8000` の 1 件だけ）

- 症状: Mac で `RunUAT.sh BuildCookRun -platform=Mac` が `Cook failed.` → `AutomationTool exiting with ExitCode=25`。ログの `Warning/Error Summary` は `Failure - 1 error(s), 10 warning(s)` で、**エラーは `LogHttpListener: Error: HttpListener unable to bind to 127.0.0.1:8000` の 1 件だけ**。クック自体は `Execution of commandlet took: 3m 42s` で完走している。警告 10 件はどれも無害（96 kHz の波に Bink は無駄という助言 9 件と、MCP の EULA の注意 1 件）。
- 原因: クックはエディタのコマンドレットで走るので**エディタ専用のプラグインも読み込まれる**。`ModelContextProtocol`（MCP サーバー）が `Config/DefaultEditorPerProjectUserSettings.ini` の `bAutoStartServer=True` に従って `127.0.0.1:8000` を掴みにいき、その Mac では 8000 が塞がっていて失敗し、エラーを 1 件出す。コマンドレットは**エラーが 1 件でもログに出ると失敗を返す**ので UAT が落ちる（上の `GameFeatureData` の件とまったく同じ型）。
- 対処: `Tools/mac_build.sh` が **`ModelContextProtocol` をビルドの間だけ `.uproject` から外す**（終わったら `trap` で必ず戻す）。`TargetAllowList` が Editor なのでパッケージの中身は変わらず、Mac では MCP を使わないので外して困るものが無い。**`AllToolsets` と `LiveCodingToolset` まで外さないこと**: `AllToolsets` が連れてくる `GameFeatures` を `Config/DefaultGame.ini` の `GameFeatureData` の規則が前提にしているので、Windows と同じ顔ぶれでクックする。2026-09-22 に 3 つとも外したら、クックが **29 秒**で別のエラーに変わった（`Ensure condition failed: AssetBaseClassLoaded [AssetManagerTypes.cpp:82]` → `Failed to load class /Script/GameFeatures.GameFeatureData for Primary Asset Type GameFeatureData!`。`UAssetManager::StartInitialLoading` → `ScanPrimaryAssetTypesFromConfig` → `ShouldScanPrimaryAssetType` → `FPrimaryAssetTypeInfo::FillRuntimeData` で、規則の `AssetBaseClass` が読めない）。**規則とそれを読み込むプラグインは対で動く**ので、片方だけ外すと今度はそちらがエラーを 1 件出す。
- やらない案: Mac の 8000 を空ける（その機械の事情に依存し、また塞がれば再発する）。`Config/` の `bAutoStartServer` を False にする（Windows の開発で MCP が自動起動しなくなる。`Config/` は両方で共有している）。UAT の `-IgnoreCookErrors`（本物のクックのエラーまで黙って通す）。
- 確かめ方: 直した後のログが `Success - 0 error(s)` で終わり、`Saved/Cooked/Mac/wasami_deception/Metadata/ReferencedSet.txt` の `^/game/` が `Content/` の `.uasset`＋`.umap` と同じ数（`mac_build.sh` が自動で突き合わせる）。
- 出典: 2026-09-22、Mac で初めてパッケージしたとき。

### パッケージは `BUILD SUCCESSFUL` なのに、中身に `/Game` のアセットが 1 つ（`L_Title`）しか入っていない

- 症状: `BuildCookRun` が通り、`Warning/Error Summary` も `Success - 0 error(s)`。出来たパッケージは 1.0 GB あるのに、`Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt`（コンテナに入ったパッケージの一覧）が 493 行しかなく、`^/game/` はたった 1 件（`/game/stage/maps/l_title`）。残りはエンジンとプラグインの既定のアセット。ログの `Packages Cooked: 494, ... Total Packages: 501` も同じ数。**起動しても本編が無い。**
- 原因: `Config/DefaultGame.ini` に `[/Script/UnrealEd.ProjectPackagingSettings]` を置かず、UE 5.8 の `CookOnTheFlyServer.cpp` `CollectFilesToCook` の**おまけの経路**（`bCookAll || (bCookAllByDefault && NumFilesAddedByCommandLineOrGameCallback == 0)`）に「`/Game` を全部なめる」のを任せていた。この数え上げはコマンドラインの指定・`AlwaysCookMaps`・アセットマネージャーやプラグインの `ModifyCook` など**複数の経路で増える**ので、0 のままである保証が無い。増えた瞬間に全部なめる経路が消え、既定のマップとその依存だけの（= C++ から名指しで読むアセットが丸ごと抜けた）パッケージが、**エラーも警告も出さずに**出来上がる。
- 対処: `Config/DefaultGame.ini` に `[/Script/UnrealEd.ProjectPackagingSettings]` の `bCookAll=True` を置く（Project Settings の Packaging の Advanced「Cook everything in the project content directory」。`bCookAll` は上の条件を飛ばして必ず全部なめる）。`MapsToCook` と `DirectoriesToAlwaysCook` は書かない（書くと「全部なめる」ではなく書いた分だけになり、C++ から直に読む 195 個のアセットが落ちる）。
- 確かめ方: `grep -c "^/game/" Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 1000 前後（1 桁なら本編が入っていない）。`UnrealPak.exe <…>.utoc -List` は**使えない**（コンテナのファイル名を持つ入り口だけを出し、クックしたパッケージはパッケージ ID で引くので名前が出ない）。
- 出典: 作業一覧の項目 36 のステップ 3（2026-09-21）。


### パッケージ版でだけ、黒いはずの板が灰色のグリッドになる（エディタ専用のエンジンのアセット）

- 症状: エディタ（PIE）では真っ黒に見える捕獲の別室が、パッケージ版では地面と壁にデバッグのようなグリッド模様が出る。ログにもエラーにも何も出ない。2026-09-22 のゲームレビュアーの指摘「本来暗闇のはずが、デバッグと思しきグリッドが地面に表示されている」。
- 原因: 板の材質が `/Engine/EngineDebugMaterials/BlackUnlitMaterial`（エディタ専用のデバッグ材質）で、**クックされない**。C++ の `TSoftObjectPtr` / `FObjectFinder` で書いたエンジンのアセットの参照はアセットではなくコードの中にあるので、クッカーが辿れるとは限らず、`bCookAll=True`（`/Game` を丸ごと）でも `/Engine` の外れたものは入らない。材質が読めない板は既定の材質（灰色の市松）で描かれる。
- 対処: **ゲームのコードが使うアセットは `/Game`（原作の取り込み先か `/Game/Wasami`）に置く**。エンジンのものを使うときは、パッケージに入っているかを `.utoc` の名前で確かめる。2026-09-22 に Windows のパッケージで確かめた結果 —— 入っている: `BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`Black`・`DefaultTextureCube`・`EngineFonts/Roboto`・`RobotoDistanceField`・`Faces/RobotoLight`・`RobotoRegular`・`RobotoBold`・`DroidSansFallback`・`EngineMaterials/DefaultNormal`・`WorldGridMaterial`・`EditorShapes/Textures/T_ShapeNormal`・`EngineDebugMaterials/VertexColorViewMode_RedOnly`（**`Roboto*` でひとくくりにはできない**: `Roboto` は入るが `RobotoTiny` は入らない）。**入っていない**: `EngineDebugMaterials/BlackUnlitMaterial`（捕獲の別室の壁）・`EngineFonts/RobotoTiny`（死亡画面のヒントと SAVING の字）・`Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`（SAVING の絵）。
- 確かめ方: `grep -a -o -E "[ -~]{6,}" Saved/StagedBuilds/Windows/wasami_deception/Content/Paks/wasami_deception-Windows.utoc | grep -x "<アセット名>.uasset"`。コンテナの索引はパスの部品（フォルダ名・ファイル名）を別々の文字列で持つので、`EngineDebugMaterials` のようなフォルダ名が出ても中身が入っているとは限らない。
- 直し方: 入らないものは **`/Game/DD/_Engine/<エンジンでの相対パス>` に同じ設定で作り直し**、コードの参照をそちらへ向ける（前処理の `dd_assets.engine_font` / `texture`、音は取り込み済みの複製）。2026-09-22 に 3 つとも作り直した: 壁は `/Game/Wasami/Enemy/M_WasamiCaptureBlack`、字は `/Game/DD/_Engine/EngineFonts/RobotoTiny`（面も）、絵は `/Game/DD/_Engine/Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`。
- **足したときに再発させない見張り**: ゲームのコードが指す `/Engine/…` を洗い出し、上の「入っている」の一覧に無いものが増えていないかを見る。2026-09-22 に洗い出した結果、残っているのは **`BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`EngineFonts/Roboto`・`EngineDebugMaterials/VertexColorViewMode_RedOnly`（エディタでしか出ないビルボード）だけ**で、どれもパッケージに入っている。前処理（`Content/Python/wasami_tools`）が指す `/Engine/…` と `Config/DefaultInput.ini` の `DefaultVirtualJoysticks` も全部入っている。テスト（`Source/**/Tests/*.cpp`）はパッケージに入らないので見なくてよい。

  ```bash
  grep -rn "/Engine/" Source/ --include=*.cpp --include=*.h | grep -v "/Tests/"
  grep -rn "/Engine/" Content/Python/wasami_tools/ Config/*.ini
  ```

- 出典: 2026-09-22 の有人セッション（作業一覧の項目 39）。**直してあり、2026-09-23 にパッケージ版で確かめた**（`main` の d348ce5 から作り直した Windows のパッケージ。`.utoc` に `RobotoTiny.uasset`・`SphereRenderHeightMap.uasset`・`M_WasamiCaptureBlack.uasset` が入り、`BlackUnlitMaterial.uasset` は無い）: 捕獲 4 種とも別室の背景は**真っ黒**（四隅と下端の画素が (0,0,0)。グリッドは出ない）、死亡画面のヒントと SAVING PROGRESS の字も丸も出る。
- 確かめ方（パッケージ版で捕獲と死亡画面を見る）: 画面への入力は要らない。`wasami_deception.exe L_Hospital_Zone1 -ExecCmds="t.MaxFPS 60, Wasami.Lives 6, Wasami.Delay 4 Wasami.Capture N, Wasami.Delay 4.6 Shot showui, …, Wasami.Delay 12 quit"`（`Tools/game_perf.py` の `launch` が対話デスクトップで起動して終わりを待つ。`Shot showui` の絵は `Saved/Archive/Windows/wasami_deception/Saved/Screenshots/Windows`）。`Wasami.Capture` は 0〜2 がホテル型（3.5 s で死亡画面）、3 が顔（1.15 s）。**捕獲はゲームを止めない**ので別室は 0.3〜2 s の間に撮る。

### `Wasami.Settings` でパッケージした本編が落ちる（`Assertion failed: IsInAudioThread()`）

- 症状: パッケージ版（`Development`）のコンソールや `-ExecCmds` で `Wasami.Settings [Name Value]` を打つと、設定を印字し切った直後に `LogWindows: Error: appError called: Assertion failed: IsInAudioThread() [File:…\AudioDevice.cpp] [Line: 7232]` で落ちる。エディタ・PIE では起きない。
- 原因: このコマンドの終わりが `FAudioDevice::GetSoundClassCurrentProperties`（`check(IsInAudioThread())` を持つ）をゲームスレッドから呼んでいる。エディタは音声スレッドを別に立てないことが多く、そのとき `IsInAudioThread()` はゲームスレッドでも真になるので当たらない。パッケージ版は音声スレッドが本当に別なので当たる。
- 対処: **2026-09-21 に直した**（項目 36 のステップ 5c）。読むところを `FAudioThread::RunCommandOnAudioThread` に包み、`FAudioCommandFence` の `BeginFence()` → `Wait()` で待ってからゲームスレッドで印字する（`WasamiGameInstance.cpp`。15 記録）。**音声スレッドが無いときは `IsInAudioThread()` が `IsInGameThread()` を返すので、`RunCommandOnAudioThread` はその場で走る**（エディタの道はこれまでと同じ）。音声スレッドがあるときは命令が溜められるので、**`RunCommandOnAudioThread` だけでは値が返る前に読んでしまう。柵で待つ**のが要る（`BeginFence()` は音声スレッドが無ければ何もしないが、そのときは命令が済んでいるので待たなくてよい）。`USoundClass` の読み込み（`LoadSynchronous`）はゲームスレッドに残す。画質を変えるだけなら、レベルを読み終えた後にエンジンの `scalability N` と `sg.*` を直接打てば製品の SET SETTINGS と同じ値になる（`Tools/game_perf.py` の `quality_commands`）。
- 確かめ方: `<パッケージ>/wasami_deception/Saved/Logs/wasami_deception.log` に `IsInAudioThread` があるか。落ちると `Saved/Crashes/` も増える。
- **同じ罠はほかの `Wasami.*` にもあり得る**: エディタでしか試していないコンソールコマンドは、パッケージ版で初めて音声スレッド・描画スレッドの `check` に当たる。2026-09-21 に `Source/` を `GetSoundClassCurrentProperties`・`IsInAudioThread`・`GetAudioDeviceRaw` で見たところ、音の装置を直に読むのはこの 1 か所だけだった。
- 出典: 作業一覧の項目 36 のステップ 5（2026-09-21。本編の fps の計測で最初に踏んだ）。

## 取り込み・レベル・描画

### 組み立てが置いた BP のアクタが本家と違う向き・位置になる（のこぎりの罠の刃が床に寝た円盤に見える）

- 症状: `place_dd_flow` で置いたアクタの回転に、本家のレベルに無いロールやピッチが混じる（のこぎりの罠 74 が yaw 90・roll −90 などになり、縦の刃が床に寝て見え、刃の箱も水平）。`stage_ue.json` の `actors[].world` の回転が、`_levels/<map>.full.json` のルートの部品（`<名前>.root` など）の `RelativeRotation` と合わない。
- 原因: 前処理 `Tools/dd/prepare_stage.py` の `read_zone` が、アクタの変換を `RootComponent` でなく「書き出しで最初に `world` を持つ部品」から取っていた。BP の部品がルートより先に並ぶアクタでは、ほかの部品（罠は刃に付く `Audio`、ロール 90°）の変換を拾う。
- 対処: 2026-09-19 に `read_zone` をアクタの `RootComponent` の部品の `world` から取るように直した（無ければ最初の部品）。前処理を流し直し、置き直す（`place_dd_flow` → `build_navigation`）。
- 確かめ方: `stage_ue.json` の新旧を比べ、変わるのがそのアクタの `world` だけか見る（2026-09-19 は罠 74 と、組み立てが置かない `BP_FakeUseActor_06_HospitalZone1_Elevator_C` 5・`BP_SecretRoomZone_C`・`BP_06_Matron_MiniBoss_C`・`wall_lamp_68_Blueprint_C`）。置いたアクタの `get_actor_rotation()` がルートの `RelativeRotation` と合うか。取り込んだ骨入りのメッシュを疑う前に、アクタの回転を数値で見る（参照の姿勢とアニメの最初のコマの骨を比べても違いは出ない）。
- 出典: 作業一覧の項目 8 のステップ 9（2026-09-19。01・08 記録）。

### 原作の材質の式が書き出しに無い（`Expressions` がほとんど null）／推定の材質が本家の見え方と合わない

- 症状: `pak_reference_2/_assets/**/M_*.json` に残るのは設定・パラメータ・いくつかの式だけで、つなぎ方が分からない。収録と見比べて推定を直しても、別の読み方が同じくらい当てはまる。
- 原因: cook は材質の式を捨てるが、**コンパイル済みのシェーダーは最新版の `.uexp` に残る**（1 つずつ zlib で包んだ DXBC）。
- 対処: `python Tools/dd/cooked_shaders.py "<pak のパスの一部>."`（Steam の最新版の pak を読むだけ）。半透明のベースパスのピクセルシェーダー（`texture3d` と深度の `texture2d` を持つ `ps_5_0`）の前半が材質の式。静的スイッチを上書きするインスタンスは自分のシェーダーマップを持つ。読み方は 01 記録の「cook のシェーダーを読む」。
- 確かめ方: 読んだ式で組んだ材質を PIE で撮り、形と色が収録と合うか見る。
- 出典: 作業一覧の項目 23 のステップ 5d3（2026-09-18）。星屑の推定を収録から 2 回読み（5d1 で「四芒星」、5d2 でそれを作った）、どちらも外れていた。**粒子を寿命から見分けるときは、粒子系が出る時刻（BP の `Delay`）を足す**（5d1 は力場が閃光の 0.2 秒後に出ることを落とし、白飛びした幕の破片を星屑と取り違えた）。Zone 2 の `M_SharpenFilter_Inst` のマスター（未解決の節）も同じ方法で読める見込み。

### `cooked_shaders.py` の `cb3` の表で、畳んだ演算がいつも `+`（`(A + B)`）と出る／回転（`Rotator`）を持つ材質で表が `cb3[0] = ?` だけになる

- 原因: 2026-09-19 までの `FIELDS` が `FMaterialUniformExpressionFoldedMath` を A・B・値の型・演算の順で読んでいた（本当は A・B・演算〈uint8〉・値の型〈uint32〉。演算を値の型の上位バイト〈0 = `+`〉から読んでいた）。`TrigMath`（X・Y・演算）が無く、それを含む表は読めずに捨てていた。
- 対処: 直した（01 記録の「cook のシェーダーを読む」）。**それより前の推定で表の `+` を根拠にした所**は、表を出し直して確かめる（ポータルの `Glow Multiplier + Base Glow` は直した後も `+`）。
- 確かめ方: `python Tools/dd/cooked_shaders.py "AdvancedMagicFX09/Materials/MI_ky_polarC_two."` の表に `cos((noiseRot * 0.25))` と `(-1.0 * sin(…))` が出る。
- 出典: 作業一覧の項目 8 のステップ 2（2026-09-19）。シェーダーを読んだサブエージェントが見つけた。

### 壁・床が灰色の市松（`DefaultMaterial`）で描かれる

- 症状: ログに `Failed to compile Material Instance with Base M_DD_Substance for platform PCD3D_SM6, Default Material will be used in game.`、その前に `Sampler type is Linear Color, should be Masks for …`（2026-09-16 は 814 件）。取り込みも組み立てもビルドも止まらない。
- 原因: マスターのノードのサンプラーの型と、既定テクスチャの圧縮（`TC_Masks` 対 `LINEAR_COLOR`）が合わない。
- 対処: 既定テクスチャの側をノードに合わせる（`T_DD_DefaultPacked` は `TC_Default`・リニア。`ensure_default_packed` が既存のアセットも直す）。`refresh_dd_stage_assets` で再コンパイル。
- 確かめ方: 取り込み・組み立ての後、ログに `Failed to compile Material` が無いこと。
- 出典: コミット 91bc32c（2026-09-16）、01 記録。最初の取り込みからこの状態で、それまでの PIE の絵と焼き込みはすべて市松だった。

### 灯の色の R と B が入れ替わる（天井灯が黄色、扉枠が青）

- 原因: 書き出し（`pak_reference`・`pak_reference_2`）は `FColor` を `[B, G, R, A]` の配列で持つ（エンジンが uint32 のまま書き、書き出しの道具 `ue4.py` がファイル上の順で出す）。
- 対処: `ue_props.value` が整数 4 つの配列の色だけを並べ替える。**C++ に手で写す `FColor` も `FColor(R, G, B)` = 配列の [2]・[1]・[0] の順にする**（捕獲の灯・欠片の灯・障壁の灯が配列の順のまま写されていて、2026-09-19 に直した）。
- 確かめ方: Zone 1 の天井灯 294 個が (200, 251, 255)、扉枠の灯 234 個が (255, 57, 74)。
- 出典: コミット c41a5c4（2026-09-16）。

### 間接光が実質ゼロで床が実機より暗い（Lumen）

- 症状: PIE で `r.Lumen.DiffuseIndirect.Allow` を 0 にしても平均輝度が 18.8 → 16.2 と動くだけ。距離フィールドのアトラスが 8 MB しか使われていない。
- 原因: ステージ本体が結合された巨大メッシュ（`hospital_zone_01_tiles_tile_tunnel` は 270 m）で、RT コアが無いこの PC の Lumen はソフトウェアのレイトレース。距離フィールドは 1 メッシュ 256 ボクセル上限（`r.DistanceFields.MaxPerMeshResolution`）なので 1 ボクセルが 50 cm を超え、トレースが当たらない。
- 対処: 原作と同じくライトマップを焼く（`r.AllowStaticLighting=True`・`r.GenerateMeshDistanceFields=False`・`r.DynamicGlobalIlluminationMethod=0`・`r.ReflectionMethod=2`）。手順は 00 記録の「灯の焼き込み」（git の外の `observations/tools/bake_level.py`、High 品質で Zone 1 約 106 秒・Zone 2 約 48 秒）。
- 出典: コミット 89611ee（2026-09-16）、00 記録。

### Zone 1 が暖色のもやに覆われる

- 症状: 霧を切ると画面の平均輝度が半分になる（18.7 → 9.9）。
- 原因: 原作で焼かれていた灯（`VolumetricScatteringIntensity` 20.0 の天井灯など）が本作では Movable になり、フォグに散乱していた。原作ではエンジンが焼かれた灯を動的に描かず、フォグにも注入しない。
- 対処: 灯の Mobility を書き出しの値（無ければ土台の値）で置く（`dd_level._mobility()`）。
- 出典: コミット 8c56bd5・58b66ec（2026-09-16）、01 記録。

### 灯の Mobility や値が落ちる（書き出しに無い）

- 症状: `Mobility` が無い灯を UE の既定の Static と読み、1,015 個の直接光まで焼いて絵が暗くなった。`SoftSourceRadius` 386 個・`LightingChannels` が落ちた。BP の灯（Zone 2 の `wall_lamp_68`）が強さ 8000 と色を失った。
- 原因: 書き出しは土台（アーキタイプ）と同じ値を省く。`scene.json` の `light` は `full.json` の一部。
- 対処: 省略は土台の値（`APointLight` は Stationary、BP の構築スクリプトの灯は Movable）で補い、値は `full.json` から取り、BP の灯はクラス既定を土台にする（`Export.default_mobility`）。UE4 と UE5 で既定が違うプロパティ（`IntensityUnits`: UE 4.24 は `Unitless`、UE5 は `Candelas`）は書き出しに無くても明示的に入れる。
- 出典: コミット 58b66ec（2026-09-16）、01 記録、`.claude/guides/unreal-workflow.md`。

### 床と壁が実機の約 2 倍明るい（焼いた後）

- 原因: 本作だけがステージ本体の間接光を面ごとに焼いていた。原作の結合メッシュは UV1 が全頂点 0 で、解像度も原作の値（ほとんど 64）。表面積から解像度を決めて UE に UV を作らせていたのが誤り。
- 対処: 原作のメッシュの `LightMapResolution`・`LightMapCoordinateIndex` をそのまま写す（`setup_lightmap`。UV は作らせない）。
- 確かめ方: `ShowFlag.GlobalIllumination 0` で壁が実機と一致するなら、焼いた間接光が過剰。
- 出典: コミット 81d3e7e（2026-09-16）、00 記録の表。

### 画角が違い、何もかも小さく写る（21:9 の画面で水平 107°）

- 原因: 原作（UE 4.24）の既定は `AspectRatioAxisConstraint=MaintainXFOV`、UE 5.8 は `MaintainYFOV`。どちらの ini にも上書きが無い。16:9 では差が出ない。
- 対処: `Config/DefaultEngine.ini` の `[/Script/Engine.LocalPlayer]` に `MaintainXFOV`。
- 出典: コミット 89611ee（2026-09-16）。

### 開始地点の床が漏れた光で明るい

- 原因: 結合メッシュの `bCastShadowAsTwoSided` を取り込んでおらず、内側向きの片面の天井が Movable の平行光源を遮らない。
- 対処: 取り込む（コミット 58b66ec）。確かめ方: 平行光源を切っても平均輝度が変わらない（変わるなら漏れている）。

### PIE で敵が動かない・`find_path_to_location_synchronously` が空・`project_point_to_navigation` が `None`（レベルに道が保存されていない）

- 症状: PIE の Zone 2 で、どこでも `NavigationSystemV1.project_point_to_navigation` が `None`、道の問い合わせが 0 点。見張りは見つけても追えず、迷路のナースも動かない。PIE の中で `RebuildNavigation` しても `Build total execution time: 0.00s` で何も変わらない。エディタで開いた直後も `None` だが、数秒後には道がある。
- 原因: ゲームのワールドは `RuntimeGeneration=DynamicModifiersOnly` なので形から道を作らず、**保存した道をそのまま使う**（UE 5.8 の `IsGeometryRebuildDisabled`）。エディタはレベルを開くと道を空にして数秒のティックで焼き直す（`bForceRebuildOnLoad`）ので、開いたのと同じ Python の呼び出しで保存すると空の道が保存される。同じ呼び出しの `RebuildNavigation` は `UNavigationSystemV1::Build Navigation NOT building because navigation build is locked (flags: 0x20).`（`AsyncLoadLock`）で断られる。
- 対処: レベルを保存するツールの後に、**別の呼び出しで** `WasamiStageTools.build_navigation()`（開いているレベルを同期で焼き、すべての `NavMeshBoundsVolume` に道があれば保存）。保存するツールは道の無いボリュームがあると `… is saved with navigation in N of M bounds volumes` と警告する。マップは git の外なので、組み立て直したら毎回要る。
- 確かめ方: `build_navigation()` の戻り値の `navigable` が `volumes` と同じ（Zone 1 は 2、Zone 2 は 29）。PIE で `open L_Hospital_Zone2` の後に `project_point_to_navigation(world, (-7406, -607, 0), None, None, (200, 200, 500))` が点を返す。Zone 2 のマップは道を焼くと約 0.7 MB 大きくなった（4.62 → 5.35 MB）。
- 出典: 2026-09-19、作業一覧の項目 27 のステップ 2（01 記録の「ナビゲーション」）。

### ステージを組み立て直すと焼き込みが外れる

- 原因: `build_dd_stage_level` はメッシュを作り直す。
- 対処: 組み立ての後に焼き直す（上の Lumen の件の手順）。完了は `L_Hospital_Zone*_BuiltData` の更新と未保存 0、組み立ての戻り値の `meshes`（Zone 1 は 924・Zone 2 は 820）で見る。
- 出典: 進捗記録 `20260916-tablet-powers.md` の再開時の注意。

### テレポートの閃光の後に真っ黒なフレームが 1 枚出る

- 症状: 60 fps の収録で白 → 赤 → 黒 → 赤。
- 原因: UE 5.8 のプリ露出が 1〜2 フレーム前の目の順応の読み戻しを使い（`PostProcessEyeAdaptation.cpp` の `FViewInfo::UpdatePreExposure`）、+100 EV の閃光の後に 2^100 のプリ露出でシーンカラーが溢れる。
- 対処: `r.EyeAdaptation.PreExposureOverride=1`（ユーザーの決定。シェーダーの再コンパイル不要。原作の「プリ露出無し」と同じく 1.0 に固定）。
- 出典: 00 記録・04 記録（2026-09-16）。

### マテリアルのドメインとブレンドを変える途中で、コンパイルの失敗がログに出る

- 症状: `dd_assets.material` でデカールのドメインに不透明のブレンドが重なる瞬間、コンパイルの失敗がログに出る（変えるたびにコンパイルされる）。
- 対処: ブレンド → ドメインの順で入れる（`dd_assets.material` はこの順）。
- 出典: 01 記録。

### 取り込みや PIE の後に `L_Hospital_Zone1` が未保存になる

- 症状: `import_dd_shards`（餅のメッシュの取り込み直し）や PIE の後に、`get_dirty_map_packages()` が `L_Hospital_Zone1` を返す（2026-09-17 のステップ 9b・10b2）。
- 原因: 特定していない（参照しているアセットを作り直したためと見ている）。
- 対処: 地図は git の外で作り直せるので、灯 783・シャード 337・選択なしを数えて前と同じことを確かめてから保存する。数が違えば保存せずに調べる。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 9b・10b2）。

### 取り込み直しても、アニメ（やほかのアセット）が前のまま（Interchange の置き換えの取り込み）

- 症状: `import_asset`（`replace_existing` 真）で glb を取り込み直すと、メッシュは置き換わるのにアニメは前の長さ・中身のまま。エラーも警告も出ず、`save_directory` もアニメを保存しない（`Content/.../A_*.uasset` の日時が古いまま）。
- 原因: UE 5.8 の `InterchangeManager.cpp`（`ImportAssetParameters.ReimportAsset` が空のとき）は、行き先に**ファイル名と同じ名前のアセット**があると、取り込みをそのアセットだけの再取り込みに変える（`bReplaceExisting` なら確認なし）。前処理の glb が `SK_WasamiEnemy.glb` でメッシュと同名だったので、メッシュの再取り込みになりアニメは作り直されなかった。
- 対処: 前処理の出力を、どのアセットとも違う名前にする（`dd_enemy.prepared_file()` = `WasamiEnemy.glb`）。ファイル名が違えば普通の取り込みになり、既にあるアセットは同じオブジェクトに書き戻される（`InterchangeTaskImportObject.cpp` が既存のアセットを工場の参照にする）。
- 確かめ方: 取り込みの後に `unreal.AnimationLibrary.get_num_frames(アニメ)` と `.uasset` の日時を見る。
- 出典: 2026-09-18、作業一覧の項目 4 のステップ 4b（07 記録の「取り込み」）。

### 粒子の煙が見えない（粒子は出ているのに、PIE で何も映らない）

- 症状: Vanish の `PPP_VanishPuff` が `get_num_active_particles()` で 5 個あるのに、見下ろしても映らない（2026-09-17 のステップ 11b3）。
- 切り分け: PIE の中で `ps.set_material(0, unreal.load_asset('/Engine/EngineMaterials/DefaultMaterial'))` にすると、粒子が画面を覆う → 位置は正しく、材質の側の問題。
- 原因: 材質の `CameraDepthFade` を既定の入力のまま使っていた。関数の既定は `Fade Length` 512・`Fade Offset` 24（プレビュー値を既定に使う）で、92 cm 先の粒子は 1/8 ほどしか見えない。関数の入力の既定は `MaterialEditingLibrary.get_material_function_expressions` の `FunctionInput` の `preview_value`（`export_text()` で読む。`.x` は無い）。
- 対処: 入力をパラメータにして、止めた PIE の中で MID の値を変えて撮る（`observations/tools/vanish_knobs.py`）。
- 出典: 04 記録、`observations/README.md` の「Vanish の見直し」。

### 本家と本作で、同じ値の粒子の出る位置が違って見える（Vanish の煙）

- 症状: 本家の煙はエレベーターの扉枠（234 cm 先）に隠され、画面の中央に明るく出る。本作はコードどおり 92 cm 先・目の 97 cm 下に出て、画面全体を暗く覆う（2026-09-17 のステップ 11b3）。
- 調べたこと（どれも原因ではなかった）: 部品の相対位置とスポーンの変換（バイトコード @22349〜@22741 で確認）、カメラの位置（本家もカプセル + (0, 0, 95)）、3 つの LOD（同じ値）、UE 5.8 の Cascade のバーストの位置の補間（`ParticleEmitterInstances.cpp` の `PostSpawn`。補間は移動の前後の差 = 真下の 50 m だけで、前へはずらさない）、`bJustRegistered`（描画の状態を作るたびに立つ）、柱の材質（どれも不透明）。
- 対処: 未解決（下の「未解決」）。粒子の値は原作のまま残す。
- 出典: 04 記録、進捗記録 `20260916-tablet-powers.md` の要確認。

### 本家の収録より粒子の数が多い（倍ほど）・粒子の灯が違って見える

- 症状: 同じ値の粒子なのに、PIE の火花（テレポートの照準）が本家の収録の倍ほど出る（2026-09-17 のステップ 11b4）。
- 原因: 本家の収録が画質「高」だった（本家の `%LOCALAPPDATA%\DDeception\Saved\Config\WindowsNoEditor\GameUserSettings.ini` の `sg.EffectsQuality=2`）。「高」は `r.EmitterSpawnRateScale 0.5`・`r.DetailMode 1`・`r.ParticleLightQuality 1`（UE 4.24 と 5.8 の `BaseScalability.ini`）で、本作のエディタの「最高」は 1・3・2。出現の率が半分になるのは、`bApplyGlobalSpawnRateScale` が真のエミッタだけ（斬撃のエミッタは偽）。
- 対処: 比べるときだけ、PIE の中で 3 つの cvar を本家に合わせる（`observations/tools/aim_setup.py`。終わったら戻す）。材質や粒子の値は変えない。
- 出典: 04 記録、観察の手順書の「5.」。

### 演出の模様を本家と比べたら、材質ではなく部屋を測っていた（白飛びした窓）

- 症状: テレキネシスの幕を本家と比べると「広い模様が本家の半分」と出るのに、材質のどこを疑っても説明が付かない（2026-09-18 のステップ 5c・5c2b）。
- 原因: 演出の窓（画面の中央 1200²）に写っているのは**青く白飛びした廊下**で、測っていた模様の大半は部屋の形だった。しかも背景が本家と本作で違い（本作の奥の扉は `telekinesis_setup.py` が動かしたエレベーターの扉で、真っ白に飛ぶ）、**差の向きは窓によって入れ替わる**（中央では本作が乏しく、左の壁では本作の方が広い）。
- 対処: 窓ごとに `room` を先に見る（`observations/tools/burst_bands.py bands <窓>`。100〜700 px の帯と**閃光の前の同じ窓**との相関）。0.3 を超えるなら部屋を測っているので、その窓で材質の良し悪しは決められない。加えて、**閃光の前のフレームで同じ統計を出し**、本家と本作の背景が揃っているかを先に確かめる。
- 出典: `observations/README.md` の「幕の広い模様は背景だった」（項目 23 のステップ 5c2b）。

### Cascade のエミッタを 1 つだけ出したいのに全部出る（LOD の `bEnabled`）

- 症状: 粒子系の LOD レベルの `bEnabled` を偽にして保存し、PIE で撮ると**何も変わっていない絵が撮れる**（2026-09-18 のステップ 5d1。エミッタを 1 つずつにした収録が、2 件とも全部入りだった）。読み戻すと `bEnabled` が真に戻っている。
- 原因: 系が読み込まれるとき LOD 0 の `bEnabled` は真に戻される。エディタで値を変えても保存しても残らない。
- 対処: エミッタの `DetailModeBitmask` を 0 にする（どの `r.DetailMode` でも出なくなる）。戻すのは `dd_particles` が入れる 15（低・中・高・Epic）。道具は `observations/tools/forcefield_solo.py`。**撮る前に、切ったはずのものが本当に消えているかを 1 コマ見て確かめる。**
- 出典: `observations/README.md` の「終わりの破片は星屑で、本家のは四芒星だった」（項目 23 のステップ 5d1）。

### 演出の破片を本家と比べたいのに、帯の統計では何も見えない

- 症状: 演出の終わりに飛ぶ小さな破片を比べたいのに、`burst_bands.py` の帯や太さでは本家と本作の差が出ない（2026-09-18 のステップ 5d1）。
- 原因: 破片が出る頃には窓の大半が**元に戻った部屋**で（`room` が 0.78〜0.97）、まばらな粒の寄与が部屋の模様に埋もれる。
- 対処: 粒を 1 つずつ拾う（`observations/tools/burst_blobs.py`。閃光の前のフレームを引き、高域を取って局所の背景 +30 で閾値、連結成分の面積・慣性主軸・白飛びを測る）。**短軸/長軸 > 0.2 だけ**を数える（細長い塊は部屋の稜線）。
- 出典: `observations/README.md` の「終わりの破片は星屑で、本家のは四芒星だった」（項目 23 のステップ 5d1）。

### 推定の材質を組んだら粒子が丸ごと消えた（`RadialGradientExponential` は中心でも 0.632）

- 症状: 粒子の材質の不透明度を `pow(なにか × RadialGradientExponential, starDensity)` の形に組んだら、スプライトが 1 つも写らなくなった（2026-09-18 のステップ 5d2）。
- 原因: `RadialGradientExponential`（半径 0.5・密度 1）の**最大値は中心の 0.632**（1 − 1/e）で 1 ではない。冪の指数が 35〜68 なので 0.632^50 ≈ 1e-10 になり、掛ける `flashPower`（3〜10）では戻らない。
- 対処: このマスクは**冪の外**で使う。冪の中で形を作りたいなら中心がちょうど 1 の `DiamondGradient` を使う。ついでに、**`starDensity` 乗の内側に入れた因子が形を独り占めする**（冪が残すのはスプライトの数百分の一で、その中では他の因子は 1 と変わらない）ので、テクスチャの線で形を作りたいならサンプルの側を冪にする。
- 出典: `observations/README.md` の「星屑を四芒星にした」（項目 23 のステップ 5d2）。

### `DrawMaterialToRenderTarget` が真っ白な絵しか描かない

- 症状: `unreal.RenderingLibrary.draw_material_to_render_target` で材質の形を見ようとしたら、どの材質でも全面が白（または既定の灰）になる（2026-09-18 のステップ 5d2）。
- 原因: この関数は材質を**キャンバスの材質**として描くので、ドメインが Surface のままだと使われない。UE 5.8 の Python には `bUsedWithUI` に当たるプロパティが無い（`used_with_ui` も `b_used_with_ui` も見つからない）。
- 対処: 描く前に `mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)` してから `recompile_material`。読み取りのときは、**RTF_RGBA8 のレンダーターゲットが持つのは線形の値**（PNG の 255 = 1.0）なので sRGB として読まない。道具は `observations/tools/material_probe.py`。
- 出典: 同上。

### 材質を作り直した直後に PIE を撮ると、シェーダーの準備中の絵が撮れる

- 症状: 材質を組み直してすぐ `telekinesis_burst.sh` を回したら、ビューポートの左上に「N 個のシェーダーを準備しています」が写り、粒子の見え方も当てにならなかった（2026-09-18 のステップ 5d2）。
- 原因: 材質を保存してもシェーダーの compile は非同期で、UE 5.8 の Python にはその残りを数える API が無い（`ShaderCompilingManager` は出ていない）。
- 対処: 撮る前に `python Tools/desktop.py shot --region 1826 205 2400 320` でビューポートの左上だけ撮って、文字が出ていないことを確かめる。
- 出典: 同上。

### 粒子（Cascade）を組み直すと `Assertion failed: (Index >= 0) & (Index < ArrayNum)` でエディタが落ちる（取り込みの失敗の後、次の `does_asset_exist` か保存で）

- 症状: `dd_particles.particle_system` が途中の例外（値の書き方が無い等）で止まった後、同じシステムを作り直す・何かが資産を問い合わせると、エディタが Python → EditorScriptingUtilities → AssetRegistry → Engine の中で落ちる。
- 原因: 作りかけのシステムに、LOD レベルをまだ持たないエミッタが残る。`UParticleSystem::GetAssetRegistryTags` → `HasGPUEmitter` が `LODLevels[0]` を範囲の確かめなしに読む。
- 対処: `_Build.run` は失敗したらシステムを空に戻してから例外を上げる（2026-09-18）。すでに残っているときは、資産レジストリを通さずに `unreal.find_object(None, '<パス>.<名前>')` で取って `WasamiCascadeLibrary.reset_particle_system` で空にする。
- 出典: 作業一覧の項目 6 のステップ 4b（`Fracture_concrete_3`。エディタの開き直し 2 回）。

### GPU のエミッタを持つ粒子を組み直すと `FinishParticleSystem` の中で `EXCEPTION_ACCESS_VIOLATION reading address 0x10`

- 原因: エディタは GPU のエミッタ（`ParticleModuleTypeDataGpu`）のシミュレーションを、モジュールの分布オブジェクトから作る（`UParticleEmitter::Build` → `CompileModule`。`UParticleModuleColorOverLife` は `ColorOverLife.Distribution->IsA` を確かめずに読む）。cook は分布を焼き込みの表だけにしていて、オブジェクトが無い。GPU のエミッタの `EmitterInfo`・`ResourceData` もこの組み立てが作るもの（書き出しの値は書かない）。
- 対処: `dd_particles` が GPU のエミッタのモジュールに、表から分布オブジェクトを作る（`_table_distribution`: 1 つなら定数か一様、複数なら表の点を通る直線の曲線）。
- **ただし cook の表は GPU のエミッタでは作り直されていないことがある**（`Fracture_concrete_3` の DustTrail は、表の色が 1 → 0.36 なのに cook の `ResourceData` は一定の 0.078、大きさも表の上限 1 に対して約 6 倍）。GPU の経路は表を読まないので、cook は表だけを古い版のまま残せる。**cook の `ResourceData` が正本**（`UParticleModuleTypeDataGpu::Build` は丸ごと `#if WITH_EDITOR`。cook されたゲームは組み直さず、保存された値だけを読んで描く）。
- 対処: `dd_particles._gpu_resource` が、色・アルファ・大きさ・SubUV の分布を cook の `ResourceData` から作り直す（2026-09-21。色とアルファ → `ColorOverLife` の 2 つ、大きさ → `SizeMultiplyLife` の `LifeMultiplier`〈misc の R・G ÷ 最大の大きさ。`EmitterInfo.InvMaxSize` の逆数〉、SubUV → `SubUV` の `SubImageIndex`〈misc の B〉）。**cook が分布オブジェクトを残している所はそのまま**にする（それが本家の組み立てが読んだ値そのものなので、同じシミュレーションが出る）。量子化は 8 ビットだがゲームが読む値そのものなので、読み戻しで失うものは無い。エディタの `OptimizeLookupTable` が標本点の外の角を 1〜2 段だけ丸める。
- 出典: 作業一覧の項目 6 のステップ 4b（エディタの開き直し 1 回）。

### 粒子が出て 1 秒ほどで `Array index out of bounds: 127 into an array of size 127`（PIE・エディタが落ちる）

- 症状: 流れが `Fracture_concrete_5` を起こした約 1 s 後、ワーカースレッドの粒子の更新の中で落ちる。
- 原因: 書いた焼き込みの表の `Values` が `EntryCount` より 1 つ少ない。UE のテキストの取り込みは、配列の中で指数の形（`-5.506497109308839e-05`）で書いた数の**次の要素を失う**（単独の値の `TimeBias=-9.3e-10` は正しく読む）。表の終わりの読み出し（`FDistributionLookupTable::GetEntry` は `EntryCount - 1` で切るだけ）が範囲の外へ出る。
- 対処: `dd_particles._number` は指数の形を使わず、`decimal` で正確な小数で書く（2026-09-18）。組んだ粒子の表の長さは、エディタで `Values` の数と `EntryCount × EntryStride` を比べて確かめる（2026-09-18 に /Game/DD・/Game/Pipeline の 6 つを確かめ、短かったのは `Fracture_concrete_3` だけ）。
- 出典: 作業一覧の項目 6 のステップ 4b（エディタの開き直し 1 回）。

### 粒子の事象で出るはずのエミッタが 1 つも出ない（`EventReceiverSpawn`。除細動器の稲妻）/ 組み直したエミッタの名前に引用符が付く（`"thander"`）

- 症状: 除細動器の放電で、揺れと音は出るのに稲妻が見えない。`ParticleSystemComponent.get_num_active_particles()` で数えると `born` を配る `subE1`〜`3` だけが出て、受ける `thander` は 0。`generate_particle_event('born', …)` を送っても出ない。受け手のモジュールの一覧（`EventReceiverModules`）も、生成の `EventGenerator` も揃っている。
- 原因: `dd_particles._text` が名前（`FName`）と文字列を引用符つきで書いていた。プロパティそのもののテキストの取り込み（`ImportText_Direct(…, PPF_None)`）は、`FNameProperty`・`FStrProperty` が残りの文字をすべて値にするので、名前が `"born"`（引用符ごと）になる。構造体の中（生成の `Events` の `CustomName`）は区切りつきで読まれて `born` になるので、合わない。`EmitterName` も同じく `"thander"` になっていた（`DescribeEmitterInstances` の出力で気づいた）。
- 対処: 名前と文字列は引用符なしで書き（列挙は字句として読むので引用符つきのまま）、`dd_particles` で組むシステムをすべて組み直す。
- 確かめ方: `UWasamiCascadeLibrary::DescribeEmitterInstances(部品)`（エミッタの実体ごとの一覧の数と粒子の数）を PIE の中で毎フレーム読む。エディタが背面だと PIE が約 3 fps で、寿命 0.1 s の粒子は数えられない（上の「エディタが背面にあると…」）。
- 出典: 01 記録の「Cascade のパーティクル」、08 記録の「確かめたこと」。2026-09-19、作業一覧の項目 8 のステップ 4（C++ の道具を足すためにエディタを 1 回開き直した）。

### テレポートでエレベーターの扉・閉ざした両開き扉（F で破る扉）を抜けられる

- 症状: Zone 1 の開始のエレベーターの中から、閉じた扉越しに外の廊下へテレポートできる。F の連打で破る扉 `BP_06_DoubleDoors11` も、閉ざしたまま向こうへ抜けられる。
- 原因: 本家の `BP_Power_Teleport` は移動の間カプセルの WorldDynamic と Pawn を Ignore にしてスイープする。扉 2 枚とエレベーターの扉は `BlockAllDynamic`（WorldDynamic）。照準が狙えるテレポートの床（`hospital_zone_01_teleport`）はエレベーターの中を覆わないが、すぐ外と扉の両側の廊下を覆う。本家のデータでも同じ（エディタのワールドで `ECC_TELEPORT` のオブジェクトのトレースを格子に引いて確かめた）。
- 対処: 本作は移動の前に `AWasamiTeleportAim::StopAtGates` で、道の途中の WorldDynamic の Pawn を止める部品の手前を行き先にする（行き先の真下の WorldDynamic の物〈救急車〉は除く）。04 記録の「既知の制約」。
- 確かめ方: テスト `Wasami.Powers.TeleportGates`。PIE でエレベーターの中 (−25, 3735)・南向きから撃つと y 3548.7 で止まる（04 記録の「テレポートの扉の手前での止まり」）。
- 出典: 2026-09-20 の有人セッションのユーザーの指摘（進捗記録 `20260920-teleport-gates`）。

## 画面の操作・本家の実機

### `desktop.py` が `PermissionError: the foreground window is PickerHost.exe` で入力を断る（Windows のファイアウォールの確認が出たまま）

- 症状: `python Tools/desktop.py ping` の `foreground` が `{"title": "Windows セキュリティ", "process": "PickerHost.exe"}` のままで、`click` / `key` が届かない。画面の真ん中に「パブリック ネットワークとプライベート ネットワークのアプリへのアクセスを許可しますか?」（発行元 Epic Games, Inc. / UnrealEditor）が出ている。
- 原因: Windows のファイアウォールの確認は前面を握り続ける。`SetForegroundWindow`・`AttachThreadInput` + `SetForegroundWindow`・`SwitchToThisWindow`・ゲームの窓のクリックのどれでも前面が戻らない（2026-09-21 に 4 つとも試した）。
- 対処: **この確認はユーザーが答える**（許可でも取り消しでも、消えれば入力は通る）。OS 全体の入力は操作しない決まり（`.claude/guides/verification.md`）なので、Claude は押さない。消えるまでは、画面へのキー・クリックが要る作業（パッケージ版の通しプレイ、本家の観察）はできない。
- 回避: ゲームの中の操作だけなら、コマンドラインの `-ExecCmds="…"` で足りることがある（`UEngine::Init` が遅延コマンドに積み、`UGameEngine::Init` の起動マップの読み込みの後、最初のティックで走る。`Tools/game_perf.py` はこれで 7 か所の fps を測った）。前面でなくてもゲームは普通に描き続けるので、fps の計測には影響しない。
- 回避（続きの操作が要るとき。2026-09-21）: `-ExecCmds` は起動の 1 ティックで走り切るので、**`Wasami.Delay S Command …`**（実時間 S 秒後に走らせる。レベルの開き直しをまたぐ。06 記録）で節目を並べる。今どこかは **`Wasami.Status`**（1 行でログに出る）で読み、絵は **`Shot showui`**（ゲームの中から撮るので前面が要らない。出力は `<アーカイブ>/wasami_deception/Saved/Screenshots/Windows/ScreenShotNNNNN.png`）で撮る。**`HighResShot 1` は使わない**: 3D の場面だけを描くので、UI しか無い画面（タイトル・スコア画面）は真っ黒になる（2026-09-21 に実際に撮って確かめた）。キーとマウスそのもの（メニューのクリック、歩き）は確かめられないので、そこはユーザー待ちにする。
- 確かめ方: `python Tools/desktop.py ping` の `foreground.process`。
- 出典: 作業一覧の項目 36 のステップ 4・5（2026-09-21）。

### `desktop.py` の入力が「the agent did not answer within 30 s」で止まる／窓が最大化されている

- 症状: `telekinesis_burst.sh` が何も印字せずに終わる。`set -e` が最初の `desktop.py click` の失敗で止めている（2026-09-18 のステップ 5d2）。
- 原因: 対話デスクトップの代理人が動いていない（セッションをまたぐと落ちている）。`python Tools/desktop.py start` で上げ直す。
- 対処: そのうえで `ping` の `rect` を見る。`[-8, -8, 3448, 1400]` なら**最大化されていて収録の枠はビューポートではない**ので、`python Tools/desktop.py click 2680 12 --count 2` で元に戻す（クリックを 2 回に分けると間が空いてダブルクリックにならない）。`observations/tools/check_viewport.py` が `[1819, 68, 3279, 1269]` を確かめる。
- 出典: `observations/README.md` の「星屑を四芒星にした」（項目 23 のステップ 5d2）。

### PIE にキーを送っても届かない

- 対処: 先にビューポートを 1 回クリックして焦点を渡す（PIE を始めただけでは届かない）。PIE でないときにビューポートをクリックするとアクタを選ぶ（選択だけならレベルは汚れない）。前面の小窓（メッセージログ・Automation のログ）は先に閉じる。
- 出典: コミット 9264dac（2026-09-16）、`.claude/guides/verification.md`。

### PIE でタイトルの RESUME から開いたレベルの最初のクリックが効かない（テレポートの照準で押しても移らない）

- 症状: `Tools/playthrough.py` の `z1_ambulance` が `on the roof: GOOD LUCK (saved 7) did not happen within 5 s` で止まる。照準の輪は屋根に出ているのに、左クリックで移らない。同じ所でもう一度押すと移る。頭から通すときだけ起き、`z1_ambulance --setup` を単独で流すと通る。
- 原因: `pause` の区間がタイトルへ戻り、タイトルの RESUME（画面のボタンのクリック）で Zone 1 を開いた後、キーは届くがビューポートがマウスを取っていないので、次の最初のクリックはマウスを取るのに使われてゲームに届かない（`run` の始めの `focus` のクリックに当たるものが無い）。カーソルがタブレットの上にあることとは関係しない（同じ位置の 2 回目で移った）。
- 対処: `pause` の区間の最後に `g.focus()`（ビューポートの真ん中のクリック）を足した（2026-09-19）。ほかの台本・手の確かめでも、画面のボタンでレベルを開いた後は、ゲームのクリックの前にビューポートを 1 回押す。
- 出典: 作業一覧の項目 8 のステップ 10（`traps_through5.mkv`。2026-09-19）。項目 8 のステップ 1 の「頭から通したときに 1 度だけ来なかった」も同じ。

### `desktop.py record` が終わらない（`record_status` が `running` のまま、動画も `.mkv.log` も空）

- 原因: ffmpeg の `ddagrab` が `Opened dxgi output 0` の後、最初のフレームを待って止まる（原因は未特定。「未解決」）。
- 対処: 止まった ffmpeg は自分が起動したものなので `taskkill` で止め、`gdigrab` で撮る（`python Tools/desktop.py record --grab gdi [--region L T R B]`。エージェントがセッション 1 で撮るので、Claude のシェルがセッション 0 でもそのまま使える。手で打つコマンドは `.claude/guides/verification.md` の「画面を操作する」）。フレームの取り出しは `-fps_mode passthrough`（無いと一定の速さに複製され、時刻と組にならない）。
- 試して駄目だったこと（2026-09-17）: `output_idx=1`（`Failed to enumerate DXGI output 1`。出力は 1 つだけ）、`-loglevel debug`（`Opened dxgi output 0 with dimensions 3440x1440` の後に何も出ない）。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 6・11a）、`.claude/guides/verification.md`。

### PIE のビューポートが `gdigrab` で 1 秒に 4〜10 枚しか撮れない

- 症状: 10b3 では 1 秒に約 48 枚撮れたビューポートの収録が、`frames` で 1 秒に 4〜10 枚（最初に 1.5 秒以上の穴）。ビューポートの外（VS Code の上）を撮っても同じ。`ddagrab` は最初のフレームで止まった（上の節）。
- 原因: PIE が上限なしで約 89 fps で描き、GPU の使用率が 96 %（`nvidia-smi`）。デスクトップの合成が待たされ、GDI の取り込みが遅れる。
- 対処: 撮る前に `python Tools/pie.py cmd "t.MaxFPS 60"`（GPU 63 %、1 秒に約 48 枚に戻った）。終わったら `t.MaxFPS 0` に戻す。手順は `.claude/guides/observation.md` の「5.」。
- **最初の穴は撮り始めから約 0.7〜1.7 s 続く**（2026-09-19、欠片の画面の収録で 3 回とも）。`record` の直後に押したクリックの演出（0.5 s）は穴に丸ごと落ちた。撮りたい操作は `record` を始めてから 2.5 s 以上待って送る。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11b1）、`20260919-escape.md`（ステップ 4）。

### 本家の一瞬の演出が `gdigrab` で粗くしか撮れない（1 秒に約 10 枚）

- 症状: 本家（全画面）を `record --grab gdi` で撮ると、`--fps 60` でも 1 秒に約 10 枚。範囲を 1720 × 720 や 860 × 360 に絞っても 3 秒で 16 枚だった（エディタのビューポートなら 60 fps で撮れた）。
- 対処: MOD の `Console Command` に `slomo 0.25` と打ってから撮る（シッピングでも効く。テレキネシスの演出が 1.3 秒 → 4.7 秒に延びた）。レベルを読み直すと 1 に戻る。タブレットの出し入れは slomo でも約 0.7 秒のままだった（理由は未確認）。`Console Command` の欄は 1 回クリックしてから `type` で打ち、`enter`。
- 注意: 入力の直前に 0.3 s ほど途切れることがある（`orig-primal-a` は 1.683 → 2.067 s）。最初に写った変化を始まりとすると、演出が本作より短く見える。終わりの時刻から逆算して合わせる（`.claude/guides/observation.md` の「6. 測る」。11b1 の「Primal の閃光が 1.5 倍長い」はこれで、11b2 で見直した）。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a・11b2）、`observations/README.md`。

### 本家の MOD の無敵が効かず、Zone 1 で Reaper Nurse に捕まる

- 症状: MOD の Maps の ZONE 1（シャードの並ぶ待合の廊下）に飛ぶと、Reaper Nurse 3 体が約 7 秒で来て捕まる。MOD の Settings で `God Mode` を見ると OFF のことがあり、テンキーの 3 を送っても効いたか分からないまま捕まった。MOD のメニューを開いている間もゲームは進む。捕まり続けると `You Are Dead` → `Restart?` で入口（`06_Hospital`）からやり直しになる。
- 対処: 無敵は当てにしない。着いたらすぐ `M` → Active Enemy の `Find All`（(1327, 860)）→ `Remove All`（(1947, 860)）で敵を消す（敵が要る観察は、消す前の数秒で済ませる。順番は `.claude/guides/observation.md`）。敵のいない開始地点（ZONE 1 STARTING POINT）で済む観察はそこで行う。テンキーはエージェントの `num0`〜`num9`。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a）。

### 本家で `desktop.py look` の回転量が送った量に比例しない（同じ量でも毎回違う・入力の後も動く）

- 症状: 本家で `look --dx 2057 --steps 17`（1 周のはず）を繰り返すと、回る角度が毎回違う。同じ量の往復で元の向きに戻らない。`--dy 300` の往復の後は上を向いたまま。
- 原因: 本家は UE4 のマウスの平滑化（`UPlayerInput::SmoothMouse`。OPTIONS の MOUSE SMOOTHING が既定でオン）が効く。平滑化は「1 フレームに入力がいくつ届いたか」と、それまでの平均の入力の間隔（`MouseSamplingTotal / MouseSamples`）で値を伸び縮みさせる。16 ms ごとに大きな値を 1 回ずつ送ると、フレームごとの値の伸縮と、入力の無いフレームへの持ち越しで合計が崩れる。実際のマウス（125〜1000 Hz）のように 1 フレームに入力が複数届けば、合計はほぼ保たれる。
- 対処: `look` に `--burst N` を付け、1 刻みを N 個の細かい入力に分けて続けて送る（1 個 7〜12 カウント。`SampleCount` は uint8 なので 1 フレームに 255 個を超えないこと）。レベルに入った直後は平均の間隔が初期値なので、測る前に往復を数回送って慣らす。`look --dx 2057 --steps 17 --burst 11` で 2 回とも元の絵に戻り（ずれ 2 px）、`--dx 1029 --steps 21 --burst 7` で真後ろを向いた。
- 確かめ方: `shot` の前後の絵の位相相関（ずれ 0 px・相関 0.4 以上）。壁ばかりの向きでは相関が弱いので、待合の廊下のような絵で測る。
- 出典: 進捗記録 `20260917-look-speedlines.md`（2026-09-17 ステップ 2）、`observations/README.md` の「視点の速さと集中線」。

### 本家で `desktop.py click` を送ると視点が大きく回る / MOD のメニューの押し間違い

- 症状: ゲーム中の `click X Y` はカーソルを絶対座標へ動かすので、その分だけ視点が回る（真下を向いた）。MOD のメニューの左の列は、ホイールで送った位置のまま残るので、前に測った座標で別の項目（W-Editor）を押した。
- 対処: ゲーム中のクリックは画面の中央 (1720, 720) で行う。MOD のメニューは押す前に列を上端へ戻し（`scroll` 8 回）、撮って項目の位置を確かめる（上端の座標の表は `.claude/guides/observation.md`。W-Editor は (990, 487)、ボタンは y 457〜517 で、11a の「Logs のつもり」の (987, 510) はここに当たった）。**W-Editor の画面が開いたら何も動かさずに `Close` で閉じる**（開いただけで、カーソルの下の扉 `BP_06_DoubleDoors13` の変換が元の値のまま `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に書かれた。値は原作と同じなので見え方は変わらない）。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a）。

### PIE で動いている最中の絵が撮れない

- 症状: `desktop.py hold w` の間はエージェントが撮影できない。PIE の `shot showui` はエディタ全体を撮り、ビューポートが黒い。
- 対処: エディタの Python で `register_slate_post_tick_callback` に `player.add_movement_input(forward, 1.0, False)` を入れて前進させ、その間に `desktop.py shot`。UI を含む絵はエージェントの `shot`。
- 出典: `.claude/guides/verification.md`（2026-09-16）。

### `HighResShot` の解像度をビューポートと違う縦横比にすると、無い影が写る

- 症状: PIE で `HighResShot 640x360` を撮ると、トンネルの床に**画面いっぱいの真っ黒な帯**が写る（実機の絵にも `HighResShot 1039x1025` にも無い）。`r.VolumetricFog 0` や `showflag.DynamicShadows 0` を足すと消えるので、フォグと影の不具合に見える。
- 原因: ビューポートは正方形に近い（`Tools/pie.py state` の窓で 1039x1025）のに 16:9 で撮ると、水平の画角を保ったまま縦の画角と縦横比が変わる。ボリューメトリック フォグの履歴と影の設定はその解像度のものが無いまま描かれる。**明るさを測る台本は数字を返すので、絵を並べるまで気づかない。**
- 対処: PIE の絵は**ビューポートと同じ大きさ**で撮る（`unreal.WidgetLayoutLibrary.get_viewport_size` で読んでから `HighResShot <w>x<h> filename=<名前>`。出力は `Saved/Screenshots/WindowsEditor/`）。**コンソールで入れた `r.` の値は PIE を終えても残る**ので、比べる回ごとに戻す値（`r.VolumetricFog 1` など）を明示的に送る。
- 確かめ方: 同じ止めた 1 コマを 2 つの解像度で撮って並べる。
- 出典: 進捗記録 `20260920-deferred-look-polish.md`（2026-09-21、ステップ 15）。

### note の GIF の画面の枠が、エディタのビューポートの縦横比で変わる

- 症状: ステージ OP の GIF を撮り直したら、題字「Stinky Gachimi」が右端で切れ、紋章が画面の半分を占めた（前の `37-stage-op.gif` は 640 × 401 で、紋章は画面の 1/4 ほど）。エディタの設定もゲームのコードも変えていない。
- 原因: 画面（UMG）の大きさは DPI のスケールでビューポートの**高さ**から決まるのに、3D は水平の画角を保つ。下の枠（コンテンツ ブラウザ）を畳むとビューポートが縦に伸びて正方形に近くなり（1039 × 1025）、紋章と題字だけが 1.6 倍に描かれて横に収まらなくなる。**レイアウトを触っただけで GIF の枠が変わり、絵を並べるまで気づかない。**
- 対処: 撮る前にエディタの窓を `(1819, 68, 3279, 896)` にする（`python -c "import sys; sys.path.insert(0,'observations/tools'); import raise_editor; raise_editor.resize((1819, 68, 3279, 896))"`。前の矩形が返るので、撮り終えたら同じ関数で戻す）。ビューポートが (1826, 206)-(2865, 857) の 1039 × 652 になり、収録の枠 `--region 1826 206 2864 856`（1038 × 650）が note のほかの GIF と同じ 640 × 401 に落ちる。
- 確かめ方: 撮る前に窓を 1 枚撮り、ビューポートの角を画素で測る（焦点のある level ビューポートは上辺が青 `(0, 111, 222)` の 1 px の枠）。`observations/tools/check_viewport.py` は窓の矩形しか見ないので、中の枠の高さが変わっても通ってしまう。
- 出典: 進捗記録 `20260922-note-stage-op-gif.md`（2026-09-22、ステップ 1）。

### `HighResShot` に Cascade の粒子が 1 つも写らない

- 症状: PIE で粒子（`Fracture_concrete_3` の破片と煙、`P_06_NurseDoorHit` の塵、`P_06_Defib` の稲妻）を出しているのに、`HighResShot 1600x900` の絵には**背景だけが写り、粒子がどこにも無い**。リモート実行では `UParticleSystemComponent` が `is_active() == True` で、位置も正しい。TAA の縁の揺れぶんの差（最大 +70/255 の 1 画素）しか出ないので、**差分を測っても「ほとんど見えない粒子」と読み違える**。
- 原因: `HighResShot` はシーンを描き直すので、Cascade のスプライトがその描き直しに乗らない。UI が写らないのと同じ扱い。
- 対処: **コンソールの `shot`（引数なし）なら粒子も写る**（2026-09-21、作業一覧の項目 34 のステップ 8 で分かった）。ビューポートをそのまま `Saved/Screenshots/WindowsEditor/ScreenShotNNNNN.png` に出すので、画面から撮るのと違って別の窓が重なっていても関係なく、UI（タブレット）も写る。`python Tools/pie.py cmd "shot"` か、リモート実行の `execute_console_command(world, "shot")` で呼ぶ。それでも足りないときだけ画面から撮る（`python Tools/desktop.py shot --region <l> <t> <r> <b> --scale 1.0`。ビューポートに別の窓〈出力ログなど〉が重なっているときは、重ならない矩形を測ってから）。位置や明るさだけを比べるなら `HighResShot` でよい。
- **`shot` の落とし穴 2 つ**: (1) 連写の 1 枚目が PIE でなく**エディタのビューポート**（別の場所を写した静止画）になることがある ── 並べるときに 1 枚目を捨てるか、先に 1 枚捨て撮りする。(2) エディタが前面に無いと Slate のティックが 0.3〜2 s おきに落ちるので、`register_slate_post_tick_callback` で刻む連写は**ゲーム内時間で見ると飛び飛び**になる ── 短い演出は `slomo 0.2` 前後まで落としてから撮る。
- 確かめ方: 同じ 1 コマを `HighResShot` と `desktop.py shot` の両方で撮って並べる（粒子が片方にしか無い）。
- 出典: 作業一覧の項目 33 のステップ 8（2026-09-21）。

### PIE の連写に別の視界やアウトライナーが写る（エディタの窓が最大化されている）

- 症状: `observations/tools/telekinesis_burst.sh` などの決め打ちの枠（画面の (1826, 205)〜(2978, 859)）で撮った連写に、右端にアウトライナーの一覧が写り、視界も違う（廊下のはずが壁の大写しになる）。明るさや模様の測りはそれらしい数字を返すので、**絵を見るまで気づかない**。
- 原因: エディタの窓が最大化されると、ビューポートが画面の (0, 134)〜(2743, 920) まで広がる。決め打ちの枠はその右寄りの一部 + 右のパネルになる。水平の画角は保たれる（`AspectRatio_MaintainXFOV`）ので絵が破綻せず、ただ写る範囲だけが変わる。**タイトルバーを続けて 2 回押すとダブルクリックとして最大化される**（前面に出すのに毎回 1 回押す台本では起こりうる）。
- 対処: (2680, 12) をダブルクリックして窓を (1819, 68)〜(3279, 1269) に戻す。撮る前に `python observations/tools/check_viewport.py`（`desktop.py ping` の矩形を照合して止まる）を通す。
- 確かめ方: `python Tools/desktop.py ping` の `foreground.rect` が `[1819, 68, 3279, 1269]`。`shot --region 1826 205 2978 859` にパネルが写らない。
- 出典: `observations/README.md`「幕を本家と比べた」（2026-09-18、項目 23 のステップ 5c）。

### PIE の連写で視点が向きを変えている / 力が別のものになっている

- 症状: 同じ台本で撮ったはずの 2 回の連写で、片方は床を見下ろしていたり、Q を押しても力場ではなく別の力が出たりする。
- 原因: (1) **PIE 中にビューポートを押すと、マウスの差分だけ視点が回る**（中央を押しても、直前のカーソルの位置との差で回る）。(2) **力のソケットは前の回の値を覚えている**ので、「1 を 4 回」で回す台本は 2 回目から別の力に着く（速さ → 転移 → 読心 → 恐怖 → 念動 → 消失の 6 つを回る）。
- 対処: (1) クリックとキーを送り終えた**後に**もう一度 `python Tools/pie.py place ...` で姿勢を置き直す。ただし**最初の置き直しは消せない**: `observations/tools/pie_pose.py` はポーンから 800 cm 以内のエレベーターの扉を隠すので、隠す前にポーンが目的の場所に立っていなければならない。(2) 今のソケットを `WasamiPowerComponent.get_socket_power(True)` で読み、`(目的 − 今) % 6` 回だけ押す。
- 確かめ方: 撮る直前の `python Tools/pie.py state` の `player:` と、台本が最後に印字する `socket left`。
- 出典: `observations/README.md`「幕を本家と比べた」（2026-09-18、項目 23 のステップ 5c）。

### エージェントが入力を断る / 反応しない

- 症状: 「前面の窓が許可の一覧に無い」で断られる。Win・Alt+Tab・Alt+F4 も断る。30 分何も来ないと自分で終了する。
- 対処: エディタに送るときは `--allow UnrealEditor.exe`（VS Code が前面で、エディタを前面にするクリックだけなら `--allow Code.exe` も。無人運転で駆動役のターミナルが前面のときは `--allow WindowsTerminal.exe`。2026-09-17、撮った画面でタイトルバーの空き〈2957, 95〉がエディタの上であることを確かめて押した）。終了していたら `python Tools/desktop.py start`。何を送ったかは `Intermediate/DesktopAgent/agent.log`。
- 前面が Chrome リモートデスクトップの共有バー（`remoting_desktop.exe`。ユーザーが遠隔で画面を見ているとき）だと、エージェントがその窓のプロセス名を読めず（`foreground.process` が空）、`--allow remoting_desktop.exe` を付けても「the foreground window is unknown ()」で断る（2026-09-20）。キーの要らない確かめは、リモート実行で `UWasamiPowerComponent` の `UsePower`・`ConfirmTeleport` などを呼んで済ませる（04 記録の「テレポートの扉の手前での止まり」）。キーが要るときはユーザーにエディタを 1 回クリックしてもらう。
- 出典: `.claude/guides/verification.md`。

### 撮った PNG が大きくて会話に読めない（5〜6 MB）

- 対処: `--scale 0.2`〜`0.35` で縮めて読む。比較用の原寸は `--scale 1.0` で保存だけする。画面は 3440x1440、座標は物理ピクセル。
- 出典: `.claude/guides/verification.md`。

### 本家のゲームが起動しない / 2 つ目が起動しない / エディタと同時に動かない

- 原因: セッション 0 から直接起動すると `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE`（上）。ランチャは既に動いていると起動を拒む。エディタと同時だと VRAM 6 GB が足りない。
- 対処: `python Tools/console_session.py <ランチャの .cmd> [--wait <画像名>]`。先にエディタを閉じる（`python Tools/editor_cycle.py --quit-only`）か PIE を止め、終わったら本家を閉じてエディタを開き直す。
- 出典: `.claude/guides/verification.md`。

### 本家でステージ内の場所へ行けない（コマンドラインのマップ指定も `HighResShot` も使えない）

- 原因: シッピングビルド。
- 対処: MOD（Simple Mod Menu、`M` キー）の Maps でチェックポイントへ飛ぶ（REPLAY だと入口の長い導入を歩く）。画面は `desktop.py shot`。MOD の pak の設置は Claude の許可判定で止まる（第三者のコードの組み込み）のでユーザーに頼む。World Editor は使わない（自動保存され、原作の見え方が根拠にならなくなる）。
- 出典: コミット f0496b1（2026-09-16）、`.claude/guides/verification.md`。

### Steam の登録（`appmanifest_332950.acf`）が書き戻される / 旧版が上書きされる

- 原因: Steam が動いていると manifest を書き戻す。app 332950 は登録を 1 つしか持てず、旧版を指したまま Steam が更新すると旧版のフォルダが上書きされる。
- 対処: 触るときは Steam を完全に終了してから。登録は最新版（`installdir = Dark Deception`）のままにする。
- 出典: コミット dca8e00（2026-09-16）、`.claude/guides/verification.md`。

### 救急車の屋根からプレイヤーが落ちる（Zone 1 の救急車が走り出して 1〜2 s）

- 症状: 救急車の屋根に乗ると、走り出して 1〜2 s でプレイヤーだけが屋根の後ろへ抜けて落ち、救急車だけがトンネルへ去る（読み込み画面と Zone 2 には進む）。フレームレートが低いほど出やすく、エディタが前面でない PIE（約 3 fps）では屋根の真ん中に立っていても落ちる。パッケージ版でも出る。
- 原因: 走り出しで当たりを入れる囲いのうち**後ろの壁 `BlockingVolume_Ambulance_3`**。囲いは救急車の子で、シーケンスが掃引なしで動かすので、1 フレームの進み（走り出しの終わりは 2440 cm/s）が壁との隙間を超えると、壁がプレイヤーのカプセルの中に現れる。土台の移動（`UCharacterMovementComponent::UpdateBasedMovement`）は掃引なので、始めから食い込んでいると移動が中止になり、押し出しが 46 cm 後ろへ出して屋根から外す。
- 対処: **直した**（2026-09-22、項目 41）。`AWasamiZone1Flow::On06ReachAmbulance` は後ろの壁だけ当たりを入れない。前と左右の 3 枚はそのまま。
- 確かめ方: `python Intermediate/Overnight/pie_ambulance.py`（PIE で屋根に落として走り出させ、`Wasami.Status` と毎ティックの測りを表にする）。プレイヤーと救急車の y の差が −66.7 のまま読み込み画面まで続けば乗っている。`--no-walls` は囲いを 4 枚とも切った比較用。
- 出典: 進捗記録 `20260922-zone1-ambulance.md`（2026-09-22 ステップ 3b）、`20260918-zone-progression.md`（2026-09-19 ステップ 10a）。

### `UnrealEditor-Cmd.exe` を Bash から直に起動するとテストが 1 件も走らずに終了コード 255 で落ちる

- 症状: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Wasami;quit" -Unattended -NullRHI -log` を Claude の Bash から直に走らせると、標準出力に UnrealBuildTool の `##PlatformValidate:` の 14 行だけが出て**終了コード 255**で終わる。エンジンのログ（`Saved/Logs/`）は 1 行も増えない。エディタを閉じてから回しても同じなので、二重起動が原因と見間違えやすい。終了コード 255 はテストが落ちたときの値（`RequestExitWithStatus(1, 255)`）とも同じで、そこでも紛らわしい。
- 原因: 突き止めていない（エンジンの初期化まで届いていない）。疑わしいのは `-log`（別のログ用コンソール窓を開く）で、Claude の端末から起動した子プロセスでは窓を持てない。
- 対処: `python Tools/console_session.py "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<uproject の絶対パス>" -ExecCmds="Automation RunTests <filter>;quit" -Unattended -NullRHI -NoSplash -ABSLOG="<絶対パス>.log" --wait UnrealEditor-Cmd.exe` で対話デスクトップに起こす。`-log` の代わりに **`-ABSLOG` で自分のログファイルに書かせる**（`Saved/Logs/wasami_deception.log` はエディタのものと混ざる）。`console_session.py` は起こすだけで待たないので、`tasklist` から `UnrealEditor-Cmd.exe` が消えるまで待ってからログの `Test Completed` を読む。**エディタは先に閉じておく**（`python Tools/editor_cycle.py --quit-only`）。
- 確かめ方: ログの末尾に `**** TEST COMPLETE. EXIT CODE: N ****` があり、`grep -c "Test Completed"` が件数を返す。`Wasami` 全体で 157 件・約 70 s。
- 出典: 進捗記録 `20260923-escape-score-immediate.md`（2026-09-23 ステップ 1）。

### Automation テストを `-NullRHI` で走らせると粒子と音のテストだけが落ちる

- 症状: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Wasami;quit" -Unattended -NullRHI` で、`Wasami.ZoneFlow.Zone1` の「the burst woken」（扉の破片のエミッタ）が落ちる。`-NoSound` も付けると `Wasami.ZoneFlow.Zone2` の「the announcement plays」も落ちる。コードは正しくても落ちるので、変更の巻き添えと見間違えやすい。
- 原因: `UParticleSystemComponent::Activate` は `FApp::CanEverRender()` が偽だと何もしないで返る（UE 5.8 の `ParticleSystemComponent.cpp:3492`）。`-NullRHI` はこれを偽にする。音は `-NoSound` がそのまま切る。
- 対処: `-NoSound` は付けない。残る「the burst woken」の 1 件は `-NullRHI` の制約として読み飛ばし、ほかの結果で判断する。描画ありで確かめたいときはエディタを**前面にして**走らせるしかないが、背面のままだと `FWaitForInteractiveFrameRate` が「Current FPS=3」と出したまま 600 s 待って進まない（下の「PIE のフレームレートが約 3 fps に落ちる」）。`Tools/desktop.py` では前面にできない（前面の窓が許可したアプリでないと入力を送らない）。
- 出典: 進捗記録 `20260922-zone1-ambulance.md`（2026-09-22 ステップ 3b）、`.claude/guides/verification.md`。

### PIE のフレームレートが約 3 fps に落ちる（エディタが前面でないとき）

- 症状: `Tools/pie.py` で流した `Wasami.Delay` の台本が、0.25 s 刻みで積んだのに 2 つずつ同じ時刻のログに出る。毎ティックの測りも 0.33 s 刻みにしかならない（スレートの毎ティックの呼び出しが 1 フレームに 1 回なので、これがそのままフレームの間隔）。
- 原因: エディタに窓の前面がない（Claude が動かしている間はいつもそう）。ゲームの時間は実時間どおりに進むので、**1 フレームが 0.33 s** になる。
- 対処: 測りとしてはそのまま使える（むしろ動く床の当たりなどは最悪の場合の試験になる）。ただし**フレームレートに依る挙動を「60 fps で確かめた」と書かない**。`t.MaxFPS 60` を送っても上限が下がるだけで、実際のフレームレートは上がらない。
- 出典: 進捗記録 `20260922-zone1-ambulance.md`（2026-09-22 ステップ 3b）、`.claude/guides/verification.md`（テストが背面で進まない話）。

### 本家でプレイヤーが一切動けなくなる（`ghost` / `fly` を試した後）

- 症状: MOD のコンソールに `ghost`（または `fly`）を打った後、`w`・`a`・`s`・`d` をいくら送ってもその場から動かない。`space`（タブレット）と `m`（MOD のメニュー）は効くので、入力自体は届いている。タブレットの地図の赤い点も動かない。
- 原因: シッピングビルドなので `ghost` / `fly` は有効にならない（コンソールは受け付けるが何も起きない）。それでもプレイヤーの移動の状態が壊れ、歩けなくなる（推定）。
- 対処: **使わない。** 入ってしまったら MOD の Maps でチェックポイントを読み直す（`walk` を打っても戻らなかった）。本家の中の長い移動は、ゲーム自身のテレポートのパワーで行う（`.claude/guides/observation.md` の 4）。
- 出典: 進捗記録 `20260920-deferred-look-polish.md`（2026-09-20 ステップ 4）。

### 本家の着地点から数歩で止まる（CHECKPOINT 9 の救急車の車庫）

- 症状: MOD の Maps で飛んだ後、前へ 2〜3 歩で止まる。横へ回っても別の物に当たり、15 m 先の目標まで 20 分かけて届かない。絵だけでは部屋が似ていて進めているかも分からない。
- 原因: 着地点（`PlayerStart_MiniBoss` など）が救急車に囲まれている。
- 対処: 進み具合は**タブレットの地図の赤い点**（`space`）で見る。長い移動はテレポートのパワーで行い、1 回の収録に数十 m 歩く撮影は 1 か所だけ入れる。
- 出典: 進捗記録 `20260920-deferred-look-polish.md`（2026-09-20 ステップ 4）。

### `desktop.py look` で回した角度が、狙いより大幅に小さい（本家の中で向きが定まらない）

- 症状: `look --dx 250 --steps 6 --burst 11` のように送っても、画面がほとんど回らない・狙った出口の方を向けない。何度も回し直しているうちに、自分がどちらを向いているか分からなくなる。`--burst` を増やしても変わらない。
- 原因 1: **`--burst` は合計を増やさない**。`desktop_agent.do_look` は 1 イベントを `dx // steps // burst` にして `steps × burst` 回送るので、**合計はほぼ `dx`**。観察の手順の文書は 2026-09-21 まで「合計は `dx × burst`」と書いていて、これを信じると実際の 11 分の 1 しか回らない。
- 原因 2: **整数の割り算で切り捨てられる**。`dx // steps // burst` が 3 になるような値（250//6//11 = 3）だと、送られるのは 3 × 66 = 198 カウントで、`dx` の 8 割しか出ない。
- 対処: **`dx` は `steps × burst` の倍数にする**。**1° ≒ 5.2 カウント**（最新版・既定の感度）なので、45° = `--dx 231 --steps 3 --burst 11`、90° = `--dx 462 --steps 6 --burst 11`、180° = `--dx 924 --steps 6 --burst 11`。知らない場所では、まず 8 回の `--dx 231` で 360° の一覧を 1 枚に貼って出口を探す（`.claude/guides/observation.md` の 4）。
- 確かめ方: 同じ向きから 8 回送って元の絵に戻れば 360°。戻らなければ 1° あたりのカウントを測り直す。
- 出典: 2026-09-21、項目 28 のステップ 16（`.claude/guides/observation.md` の 4 を直した）。

### 本家の Zone 2 の到着（MOD の TUNNEL）から、歩いて捕まる場面のトリガーへ行けない（未解決）

- 症状: MOD の Maps の TUNNEL（索引 7）は `06_Hospital_Zone_02` を救急車の屋根 `PlayerStart_1` (−22545, −5035, 1262) で開き、到着の後はプレイヤーが救急車の停まったガレージに降りる。ここから `Trigger_Arrive_CaptureScene` (−11600, −1000) まで約 148 m（東へ 94 m → 北へ 34 m）歩くと捕まる場面と独房の場面が流れるが、**ガレージから出る道が絵だけでは見つからない**。ガレージの 4 面は、トンネル（救急車が来た方）・`WEST 2 ENTRANCE` の袋小路・救急車の壁画の壁・`HOSPITAL EAST ENTRANCE ←` の案内板の壁で、タブレットの地図はこの区域では真っ黒（地図のテクスチャが無く、目的の矢印も出ない）。2026-09-21 に 25 分かけて届かなかった。
- 分かっていること: 救急車には `BP_Power_Teleport_Zone` が無い（テレポートの輪が屋根に吸われない）ので、ここは**到着のガレージ**で合っている（脱出の救急車ではない）。`WEST 2 ENTRANCE` の中は曲がり角で行き止まりに見えるが、奥で折れている可能性が残る。
- 次に試すこと: テレポートの照準を最大まで押すと、輪は**歩ける床の一番遠く**に乗るので、それで道の向きを読む。または `Ambulance_Arrive_Blockers*` (−22293〜−22783, −5030) の外側の床を原作のレベルの静的メッシュから起こして、出口の向きを先に決める。
- 出典: 2026-09-21、項目 28 のステップ 16（進捗記録 `20260920-deferred-look-polish.md`）。

### PIE の音を録っても無音（`AudioMixerLibrary.start_recording_output` の wav が全部 0）

- 症状: `unreal.AudioMixerLibrary.start_recording_output(world, N)` → 音を鳴らす → `stop_recording_output(world, unreal.AudioRecordingExportType.WAV_FILE, '<名前>', '<フォルダー>')` で `Saved/BouncedWavFiles/<フォルダー>/<名前>.wav` は出来るが、全サンプルが 0。長さは録った実時間ぶんある。音の部品は `is_playing()` が真で、ログの WASAPI も `InitializeHardware succeeded` と出ている。
- 原因: エディタが前面でないとき、UE は出力の音量を 0 にする（`FApp::UnfocusedVolumeMultiplier`。既定 0。`FWindowsPlatformApplicationMisc::PumpMessages` が毎回 `FApp::SetVolumeMultiplier` に入れ直し、`FAudioDevice::Update` が主音量に掛ける）。Claude は Windows のセッション 0 にいてエディタを前面にできない（項目 33 のファイアウォールのダイアログが前面に居座るので `SetForegroundWindow` も断られる）。`au.UnfocusedVolumeMultiplier` というコンソール変数は**無い**（`Command not recognized`）。
- 対処（2026-09-22 に解決）: **コンソール変数 `au.DisableAppVolume 1`** を PIE の中で実行してから録る。`FAudioDevice::Update` の `if (!DisableAppVolumeCvar) { PrimaryVolume *= FApp::GetVolumeMultiplier(); }` を飛ばすので、前面かどうかに関わらず音が出る（開き直しは要らない。エディタを閉じると既定の 0 に戻る）。手順は `unreal.SystemLibrary.execute_console_command(w, 'au.DisableAppVolume 1')` → `unreal.AudioMixerLibrary.start_recording_output(w, 秒)` →（鳴らす）→ `stop_recording_output(w, unreal.AudioRecordingExportType.WAV_FILE, '<名前>', '<フォルダー>')` → `Saved/BouncedWavFiles/<フォルダー>/<名前>.wav`。**音はユーザーのスピーカーから鳴る**ので、音を確かめるときだけ入れる。
  - `Saved/Config/WindowsEditor/Engine.ini` に `[Audio] UnfocusedVolumeMultiplier=1.0` を書く手も試したが、**エディタが起動時にこのファイルを消す**ので効かない（2026-09-22）。
  - **時刻を測るなら `bThrottleCPUWhenNotForeground` も偽にする**: 背面のエディタは 3 fps ほどでティックするので、タイマーの発火が最大 0.3 s 遅れて記録に出る。
- 確かめ方: 録った wav の最大振幅が 0 でないこと（`au.DisableAppVolume` を入れないと 0 のまま）。波の照合は `numpy` の相互相関（本作の声の原本は `SourceArt/Wasami/Voices/*.wav`。48 kHz モノラルへの変換は `ffmpeg`）。
- 出典: 2026-09-21 の作業一覧の項目 35 のステップ 6（10・19 記録の「確かめたこと（2026-09-21）」）、解決は 2026-09-22 の項目 40 のステップ 4。

### 録った PIE の音で、曲や効果音に重なった声が判定できない（`au.DumpActiveSounds` は PIE の音を出さない）

- 症状: 相互相関の値が低く、声が鳴っているのか曲だけなのか決められない。鳴っているものを engine に直接聞こうと `au.DumpActiveSounds` を PIE の中から送っても、**鳴っている最中でも `Active Sound Count: 0`** としか出ない（エディタ側の音声装置を見ていて、PIE の装置は見ない）。`stat sounds` は `Command not recognized`。Python には発音中の `UAudioComponent` を数える口が無い（`PlaySound2D` の部品はどのアクタにも付かないので `get_all_actors_of_class` では拾えない）。
- 原因（相関が低くなるほう）: 正規化した相関は**原本と同じ長さの窓**の実効値で割るので、窓にステージ側の音が入ると下がる。`SourceArt/Wasami/Voices/you.wav` は 5.007 s のうち実音が 0.103〜1.992 s で残りは無音なので、窓が 3 s ぶん余計に拾う。Zone 1 は到着そのものが最初の 6 s に最大 0.36 の音を出している（何も引き金を引かずに録った `tmp/pie_idle_record.py` で確かめた）。
- 対処: **重なっている既知の音を波から引いてから見る**（`tmp/gameover_music_residual.py`）。曲や効果音は同じ波を頭から鳴らすので、録りとの相互相関で始まる時刻を出し、最小二乗の倍率を掛けて引ける。残りの実効値が元の数 % まで落ちれば、その区間はその音だけで、ほかには何も鳴っていない。判定は相関の絶対値でなく**山と次点の比**で行う。
- 出典: 2026-09-23 の作業一覧の項目 48 のステップ 2（10 記録の「確かめたこと（2026-09-23）」）。

### `HighResShot` の PNG に UMG（タブレット・スコア画面・EXTRAS）が写らない

- 症状: PIE で `HighResShot 1 filename=x` を実行すると `Saved/Screenshots/WindowsEditor/x.png` は出来るが、3D の場面だけで UI が無い。`HighResShot 1280x720` の形でも同じ。
- 原因: `HighResShot` は場面のレンダーターゲットを書き出すので、Slate で描く UMG は入らない。
- 対処: UI を写すなら `Tools/desktop.py shot`（窓の取り込み）。エディタを前面にできないときは写せないので、UI は `unreal.WidgetLibrary.get_all_widgets_of_class(world, cls, True)` と部品の値で確かめる。
- 出典: 2026-09-21 の作業一覧の項目 35 のステップ 6。

### `AWasamiTriggerBox` が 2 度と鳴らない（流れが結ぶ前にプレイヤーを置いた）

- 症状: `pie.py place` や `set_actor_location` でプレイヤーを引き金の箱に置いたのに `OnTrigger` が来ない。出てもう一度入れても来ない。`get_overlapping_actors` にはその箱が出る。
- 原因: `AWasamiTriggerBox::NotifyPlayerOverlap` の DoOnce（`bBeginClosed`）は**結び先があるかに関わらず**最初の重なりで閉じる（原作の `ReceiveActorBeginOverlap` の DoOnce のまま。11 記録）。流れがまだ `BindTrigger` していない段階で重なると、その箱は永久に鳴らない。
- 対処: 流れの手順（チェックポイント → 区間 → 結び）を踏んでから置く。踏むのが長い場合は、流れの `On...` が `UFUNCTION()`（`BindUFunction` で結ぶため）なので `flow.call_method('OnEndTrigger')` のようにリモート実行から直に呼ぶ（箱の配線は Automation の `Wasami.ZoneFlow` が見ている）。
- 確かめ方: 区間が進んだかは、その区間の副作用（ポータルの `locked`、目的の文字）で見る。`Section` は `UPROPERTY` ではないので Python からは読めない。
- 出典: 2026-09-21 の作業一覧の項目 35 のステップ 6（11 記録の「脱出の間合い」）。

## 直さなくてよい既知の見え方

- **エディタの起動直後の「メッセージログ」**（起動時の読み込みエラー 1 件、GameFeatureData の設定の警告）— 前からあるもの。ビューポートの左に重なるので PIE の前に × で閉じる（進捗記録 `20260916-tablet-powers.md` の再開時の注意）。`Tools/editor_cycle.py` の開き直しの後に閉じ忘れると、`playthrough.py` の画面のボタンのクリックが小窓に当たり、`pause` の区間がタイトルの RESUME の後に `L_Hospital_Zone1 to open did not happen within 40 s` で止まる（2026-09-19。`desktop.py ping` の前面が `メッセージ ログ` になる。窓の右上の × を `--allow UnrealEditor.exe` で押す）。
- **エンジンの起動時の `LogAutomationTest: Error: Condition failed` 19 件** — エンジン自身の自己テスト。毎回同じ数（04 記録の「確かめたこと」）。
- **Automation テストの後に `get_dirty_map_packages()` が `/Temp/Untitled_1`・`/Temp/Untitled_3` を返す** — `Wasami.Powers.PrimalStun`・`VanishNotify` などが作った一時的なワールドのパッケージ。ガベージコレクションでも消えないが、`/Temp` なので保存されず、その後の `editor_cycle.py` の終了も妨げない（2026-09-17 の 3 つのセッションのログで確かめた）。「未保存なし」を確かめるときは `/Game` のものだけを見る。
- **VSM の「非 Nanite マーキング ジョブ キュー オーバーフロー」2 件** — 前のセッションから出ているもの（04 記録）。
- **PIE のビューポートの外周に、縮尺の違う絵が枠のように出る** — 2026-09-17 のステップ 6 の撮影から。Vanish の前からある（04 記録）。
- **焼き込みの警告 2 種**（`LightmassImportanceVolume` が無い、ライトマップ UV の重なり 7 メッシュ）— 原作の Zone 1・Zone 2 にもボリュームは無く、UV は原作のまま。直さない（01 記録の「ライトマップ」）。
- **エディタで最初の閃光（シャードの回収）だけ描画が約 0.6 秒止まる** — 粒子の材質のシェーダーをその場でコンパイルする（`LogShaderCompilers` のジョブ 0.5〜0.6 秒）。ゲームの時間は止まらず、2 回目からは止まらない。パッケージでは起きないはず（06 記録）。
- **取り込みのログの `FlipBook output 2 is UVs`**（04 記録）。
- **`import_dd_powers` などが前回と同じ数のアセットを作って `Failed to compile` なしで終わる** — 正常（各記録の「検証」）。
- **Vanish の効果中に死亡のリセットが来て、15 秒が終わる前に使い直すとゲージの FlipFlop が 1 つずれる** — 本家の `BP_Powers` と Delay の作りどおり（04 記録）。
- **`UMG_SpeedBoost` の最初のフレームが不透明度 1** — 本家の Delay の順の写し（04 記録）。
- **シャードを回収した瞬間の −1 の閃きが 1 フレームぶん進んで見えることがある** — 本作の画面はプレイヤーのティックで進むため（03 記録・06 記録）。
- **テレポートの閃光の白が旧版の Manor (234, 245, 244) と本作の病院 (252, 252, 251) で違う** — ステージのポストプロセス（色の補正）の差と見ている（04 記録）。
- **`Tools/overnight.py` がセッションの中で exit 4** — 意図どおり（上の hooks の節）。
- **UI の材質の `Time` と UMG のアニメは `slomo` に従わない** — 本家も本作も同じ（`observations/README.md` の「Vanish の見直し」）。`slomo` で撮った収録の縁のもやの明滅は、実時間の周期で読む。

## 試して駄目だった案

- **背面のエディタの 3 fps を設定で直す**: コンソールの `set EditorPerformanceSettings bThrottleCPUWhenNotForeground False` は効かず、設定は Python の型としては見えない → エディタを前面にする（2026-09-16、`.claude/guides/verification.md`）。2026-09-19: CDO を `unreal.find_object` で取れば Python から書ける（上の「エディタが背面にあると…」の対処）。
- **`UPostProcessComponent` / `ULegacyCameraShake` を C++ で継ぐ** → MinimalAPI でリンクできない（上の C++ の節）。
- **Lumen で間接光を出す** → 距離フィールドが潰れてゼロ。焼き込みに切り替えた（コミット 89611ee）。
- **ライトマップの解像度を表面積から決め（1 テクセル 20 cm）、UV の無い結合メッシュに UE の UV を作らせる** → 実機の 2 倍明るい。原作の値を写す（コミット 81d3e7e）。
- **灯を全部 Movable にして `VolumetricScatteringIntensity` を 0 に上書き**（初期の `dd_level.py`）→ もやと間接光の欠落。Mobility を書き出しの値で置く（コミット 8c56bd5・58b66ec）。
- **`ConstructorHelpers` でパイプラインの素材を読む** → 作り直しでエディタが落ちる（上のエディタの節）。
- **`connect_material_expressions` に `"Input"` のピン名** → つながらない。`""` にする（03 記録）。
- **ffmpeg の `ddagrab` で PIE を収録** → 最初のフレームで止まる。`gdigrab`（上の画面の節）。
- **`desktop.py hold w` しながら `shot`** / **PIE の `shot showui`** → 撮れない・ビューポートが黒い（上）。
- **モニタを点ける、Bash のサンドボックスを切る**（セッション 0 の DXGI の落ちに対して）→ 直らない（自動メモリ `editor-launch-needs-user`）。
- **hook の出力をパイプで別の Python に読ませて確かめる** → cp932 で化けて行が繋がって見える。ファイルに落として UTF-8 で読む（上）。
- **認証情報のファイルや CLI 本体を読んで使用量を得る** → auto モードの分類器が拒否（上）。
- **auto モードでガイドの「先に確認」の行を書き換える** → 分類器が止める。手動モードで（上）。
- **Steam を動かしたまま `appmanifest` を書く** → 書き戻される（上）。
- **本家のコマンドラインでマップを指定** → シッピングビルドでは使えない。MOD の Maps（コミット f0496b1）。
- **タブレットの板をカメラの子に置いたままにする** → シェイクと逆に揺れる。POV から置き直す（コミット bdb11ef）。
- **本家の中を絵だけを見て歩いて目的の場所へ行く**（Zone 2 の到着のガレージ → 捕まる場面のトリガー、148 m）→ 25 分かけて出口も見つからなかった。地図が黒い区域では、先に原作のレベルから道を起こすか、テレポートの照準で歩ける床を読む（上）。
- **本家の連写から星屑の形を読んで材質を推定する**（項目 23 のステップ 5d1・5d2）→ 白飛びした四芒星は星屑ではなく、原作の星屑はテクスチャを読まないひし形だった。材質は先に `Tools/dd/cooked_shaders.py` で式を読む（上の「原作の材質の式が書き出しに無い」）。
- **PIE の収録で slate の post tick の `dt` を実時間として足す** → `slomo` を掛けると場面の時刻が 1.5 倍ずれる。`get_time_seconds` で撮る（上）。
- **`burst_flow.py` の `flow`・`width` の px を `--ds` 倍せずに比べる** → 黙って本家が本作の半分の値に出る（`--ds 2` で測った本家は 2 倍、`SCALE=2.97` で `--ds 1` になる本作は 2.97 倍して本家の px にする）。項目 23 のステップ 5c はこれで「本作の流れが本家の 3〜4 倍」と読み違えた（5c2a で直した）。

## 未解決

- **`ddagrab` が最初のフレームで止まる原因**（2026-09-17）。回避は `gdigrab`。
- **Vanish の煙の位置と明るさ**（2026-09-17）— 本家の煙は 2 m 以上先に明るく出るが、原作の値からは 92 cm 先になる（上の「取り込み・レベル・描画」の節）。本家の実機で、煙の出る向き（見下ろす・横を向く）を撮れば、位置の手がかりになる。
- **テレキネシスの球の粒子の灯が、本家より明るく照らす**（2026-09-17、ステップ 11b4）— 同じ画質（「高」）で、線形の明るさの寄与が本家の約 1.4〜2 倍。灯の値は原作どおりで、GI は無く反射は SSR。UE 4.24 と 5.8 の単純な灯（`FSimpleLightEntry`。非逆二乗の減衰）の扱いの違いを疑っているが、UE 4.24 のソースが手元に無く確かめていない（04 記録）。
- **スカイライトのキューブマップ（`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`）が回収できない** — 書き出しは 512×512 の平面 PNG 1 面で、UE に取り込むと Texture2D になる。シーンのキャプチャに任せている。寄与はほぼ無い（強度を 0〜50 に振っても原作の 0.5 では平均が 0.05 も動かない）ので急がない（01 記録）。
- **Zone 2 のポストプロセスボリュームの `ColorGradingLUT`**（書き出しがアセットのパスの文字列で、取り込んだテクスチャに解決する仕組みが無い）**と `WeightedBlendables`**（`M_SharpenFilter_Inst`。マスターの式が cook で消えている）— `ColorGradingIntensity` が 0 なので見た目の寄与は無い（01 記録）。
- **焼いた後も残る 1〜2 割の明るさの差**（床の手前 72 対 57、エレベーターの壁、天井の中央）— 焼き込みの品質ではない（Preview → High で ±2 以内）。床の中ほどは正面の両開き扉（BP 由来、未実装）が無いための映り込み（00 記録の「灯の焼き込み」）。
- **駆動役の使用量の読み方と、使用量の上限に達したときの `claude -p` の返事の文言** — 要確認（ユーザー）（進捗記録 `20260917-autonomy.md`）。
