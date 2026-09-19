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
- もう 1 つの経路（2026-09-19）: 作業ブランチで CRLF のまま書かれた C++ のハッシュを `--update` で取ると、main への切り替えとマージで git が LF に書き直した後にずれる（Stop hook が止める。項目 9 のマージの後に 02・06・07 で起きた）。マージの後にも `check_records.py` を走らせ、保存されたハッシュが今のファイルの CRLF 版と一致する（行末だけの差）ことを見てから `--update` してコミットする。
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
- 2026-09-19: **ウィジェットのティック（`NativeTick` の `InDeltaTime`）も遅れる**。Slate は 1 コマの時間を 1/8 s で打ち切る（`FSlateApplication::TickTime`）ので、約 3 fps だとスコア画面の `ClearAnimation` も連続回収の画面（2 s で消える）も約半分の速さになった。ゲームの時間（`get_time_seconds`）は実時間どおり進むので気づきにくい。画面の時刻を収録で測る前に、エディタを前面にして 8 fps を超えているか（`unreal.SystemLibrary.get_frame_count()` の進み）を見る。
- 対処: `python Tools/desktop.py click 2957 95 --allow UnrealEditor.exe`（タイトルバーの空き。エディタの窓が今の位置のとき。撮った画面でクリックの位置がエディタの上であることを先に見る）でエディタを前面にする。PIE を始めてもエディタは前面に来ない。
- 対処（前面に出せないとき。2026-09-19）: ユーザーのターミナルが前面だと `desktop.py` はクリックを断り（前面が許した窓でない）、MCP の `SlateInspectorToolset.Windows` の `select` も Windows に前面の切り替えを止められる。そのときはリモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` にする（クラスは Python の型として出ていないが、CDO は `find_object` で取れて書ける。メモリ上だけなので開き直すと戻る）。背面のまま PIE とテストが前面と同じ速さで進み、`desktop.py record` で約 58 fps で収録できた。終わったら `True` に戻す。前面の小窓（メッセージログ）は `SlateInspectorToolset.Windows` の `list` → `close`（番号）で閉じられる。
- 確かめ方: `stat fps`、またはリモート実行で `unreal.GameplayStatics.get_time_seconds` の進み。
- 出典: `.claude/guides/verification.md` の「動きの確認」（2026-09-16）、03 記録。「試して駄目だった案」も参照。

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
- `Use Less CPU when in Background`（`EditorPerformanceSettings`）: Python から見えない。エディタを前面にする（上）。
- `WidgetBlueprintLibrary`（`GetAllWidgetsOfClass`）: `unreal.WidgetBlueprintLibrary` は無い（`module 'unreal' has no attribute 'WidgetBlueprintLibrary'`）。`unreal.WidgetLibrary.get_all_widgets_of_class(world, cls, False)` で呼べる（`Tools/playthrough.py` の脱出の見分け。2026-09-19）。

### 毎フレームのコールバック（`register_slate_post_tick_callback`）が例外で黙って外れ、記録を失う

- 対処: 始める前に記録の関数を 1 回そのまま呼んで通ることを確かめる。終わったら `unregister_slate_post_tick_callback`。scratchpad の補助スクリプトはセッションごとに消えるので、要るときは作り直す。
- 出典: 進捗記録 `20260916-tablet-powers.md`（ステップ 8 で 1 回目の使用の記録を失った）。

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

### ファイルを足したらビルドが落ちた: 無名名前空間の名前の衝突・C4458

- 症状: 変えていないファイルで再定義や曖昧な参照のエラー、または `warning C4458: declaration of 'Slot' hides class member`（警告がエラー扱い）。
- 原因: ユニティビルドでファイルのまとまり方が変わり、無名名前空間の同じ名前（`WaveVolume`・`WavePitch`・`FadeKeys`・`EnemyTag`・`VignetteScale`）が 1 つの翻訳単位に入る。C4458 はローカル変数が `UWidget::Slot`・`UUserWidget::bInitialized` などを隠す。
- 対処: 定数や補助の名前はファイルごとに固有にし、UE のメンバー名と同じローカル変数を避ける。ファイルを足さなくても、ヘッダーを 1 つ変えて再コンパイルの範囲が変わるだけで起きる（2026-09-18: `WasamiEnemyAnimInstance.h` の定数を変えたら `WasamiEnemy.cpp` と `Tests/WasamiTestEnemy.cpp` の `EnemyTag` がぶつかった。テスト側を `TestEnemyTag` にした）。
- 2026-09-18: 新しいファイルを 7 つ足したら、既存のファイル同士（`WasamiPopUpWidget.cpp` と死亡画面の `ButtonGrey`・`Place`、パワーの `OpacityKeys`、`WasamiTelepathyTrackerWidget.cpp` の自前の `FAnimKey` と `using WasamiWidgetAnimation::FAnimKey`、`WasamiBlackFadeWidget.cpp` と死亡画面の `FadeInKeys`）がぶつかって 2 回落ちた。**先に重複を洗い出すと 1 回で済む**: 各 `.cpp` の `namespace { … }` の中の `const`/`constexpr` の名前・関数名・`struct` 名を集め、2 つ以上のファイルにあるもの（`using` の宣言は同じ実体なので除く）を片方で固有の名前に改める。
- 2026-09-18: **エンジンのヘッダーの引数名ともぶつかる**（`error C4459: declaration of 'BoxExtent' hides global declaration`）。無名名前空間の名前はその翻訳単位では大域に見えるので、同じ塊に入った `Kismet/KismetMathLibrary.inl` の `BreakBoxSphereBounds(…, FVector& BoxExtent, …)` が `WasamiDoorBreak.cpp` の `BoxExtent` を隠すと言われた（`WasamiDoubleDoors.cpp` を足して塊が変わった）。`BoxExtent`・`ComponentScale` のような一般の名前は避け、ファイルの頭字を付ける（`DoorBreakBoxExtent`・`DoorsLeaveScale`）。
- 2026-09-19: **未コミットのファイルは塊の外でコンパイルされるので、ビルドが通ってもコミットの後に落ちることがある**（UBT の適応ユニティビルドは、git で変更中のファイルをユニティの塊から外す）。項目 7 のステップ 5a で足した `WasamiViewcone.cpp` の `MinimapTag`・`OpacityName` が `WasamiPlayerCharacter.cpp`・`WasamiPrimalPower.cpp` とぶつかっていたのに、ステップ 5a のビルドは通り、コミットした後の次のビルドで落ちた。C++ のファイルを足したら、コミットの前に上の洗い出しをする。落ちた後に直すときは、直しを先にコミットしてからビルドすると、塊が本来のまとまり方になって残りのぶつかりも出る。
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
- 出典: 07 記録の「エンジンのタイマーの刻み」、進捗記録 `20260917-enemy-wasami-body.md` のステップ 3（2026-09-18。期待の時刻を直すのにビルドを 1 回やり直した）。

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

## 取り込み・レベル・描画

### 原作の材質の式が書き出しに無い（`Expressions` がほとんど null）／推定の材質が本家の見え方と合わない

- 症状: `pak_reference_2/_assets/**/M_*.json` に残るのは設定・パラメータ・いくつかの式だけで、つなぎ方が分からない。収録と見比べて推定を直しても、別の読み方が同じくらい当てはまる。
- 原因: cook は材質の式を捨てるが、**コンパイル済みのシェーダーは最新版の `.uexp` に残る**（1 つずつ zlib で包んだ DXBC）。
- 対処: `python Tools/dd/cooked_shaders.py "<pak のパスの一部>."`（Steam の最新版の pak を読むだけ）。半透明のベースパスのピクセルシェーダー（`texture3d` と深度の `texture2d` を持つ `ps_5_0`）の前半が材質の式。静的スイッチを上書きするインスタンスは自分のシェーダーマップを持つ。読み方は 01 記録の「cook のシェーダーを読む」。
- 確かめ方: 読んだ式で組んだ材質を PIE で撮り、形と色が収録と合うか見る。
- 出典: 作業一覧の項目 23 のステップ 5d3（2026-09-18）。星屑の推定を収録から 2 回読み（5d1 で「四芒星」、5d2 でそれを作った）、どちらも外れていた。**粒子を寿命から見分けるときは、粒子系が出る時刻（BP の `Delay`）を足す**（5d1 は力場が閃光の 0.2 秒後に出ることを落とし、白飛びした幕の破片を星屑と取り違えた）。Zone 2 の `M_SharpenFilter_Inst` のマスター（未解決の節）も同じ方法で読める見込み。

### 壁・床が灰色の市松（`DefaultMaterial`）で描かれる

- 症状: ログに `Failed to compile Material Instance with Base M_DD_Substance for platform PCD3D_SM6, Default Material will be used in game.`、その前に `Sampler type is Linear Color, should be Masks for …`（2026-09-16 は 814 件）。取り込みも組み立てもビルドも止まらない。
- 原因: マスターのノードのサンプラーの型と、既定テクスチャの圧縮（`TC_Masks` 対 `LINEAR_COLOR`）が合わない。
- 対処: 既定テクスチャの側をノードに合わせる（`T_DD_DefaultPacked` は `TC_Default`・リニア。`ensure_default_packed` が既存のアセットも直す）。`refresh_dd_stage_assets` で再コンパイル。
- 確かめ方: 取り込み・組み立ての後、ログに `Failed to compile Material` が無いこと。
- 出典: コミット 91bc32c（2026-09-16）、01 記録。最初の取り込みからこの状態で、それまでの PIE の絵と焼き込みはすべて市松だった。

### 灯の色の R と B が入れ替わる（天井灯が黄色、扉枠が青）

- 原因: 書き出し（`pak_reference`・`pak_reference_2`）は `FColor` を `[B, G, R, A]` の配列で持つ（エンジンが uint32 のまま書き、書き出しの道具 `ue4.py` がファイル上の順で出す）。
- 対処: `ue_props.value` が整数 4 つの配列の色だけを並べ替える。
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
- 対処: `dd_particles` が GPU のエミッタのモジュールに、表から分布オブジェクトを作る（`_table_distribution`: 1 つなら定数か一様、複数なら表の点を通る直線の曲線）。**ただし cook の表は GPU のエミッタでは作り直されていないことがあり**（`Fracture_concrete_3` の DustTrail は、表の色が 1 → 0.36 なのに cook の `ResourceData` は一定の 0.078、大きさも表の上限 1 に対して約 6 倍）、見え方は本家とずれる（作業一覧の後回しの一覧）。
- 出典: 作業一覧の項目 6 のステップ 4b（エディタの開き直し 1 回）。

### 粒子が出て 1 秒ほどで `Array index out of bounds: 127 into an array of size 127`（PIE・エディタが落ちる）

- 症状: 流れが `Fracture_concrete_5` を起こした約 1 s 後、ワーカースレッドの粒子の更新の中で落ちる。
- 原因: 書いた焼き込みの表の `Values` が `EntryCount` より 1 つ少ない。UE のテキストの取り込みは、配列の中で指数の形（`-5.506497109308839e-05`）で書いた数の**次の要素を失う**（単独の値の `TimeBias=-9.3e-10` は正しく読む）。表の終わりの読み出し（`FDistributionLookupTable::GetEntry` は `EntryCount - 1` で切るだけ）が範囲の外へ出る。
- 対処: `dd_particles._number` は指数の形を使わず、`decimal` で正確な小数で書く（2026-09-18）。組んだ粒子の表の長さは、エディタで `Values` の数と `EntryCount × EntryStride` を比べて確かめる（2026-09-18 に /Game/DD・/Game/Pipeline の 6 つを確かめ、短かったのは `Fracture_concrete_3` だけ）。
- 出典: 作業一覧の項目 6 のステップ 4b（エディタの開き直し 1 回）。

## 画面の操作・本家の実機

### `desktop.py` の入力が「the agent did not answer within 30 s」で止まる／窓が最大化されている

- 症状: `telekinesis_burst.sh` が何も印字せずに終わる。`set -e` が最初の `desktop.py click` の失敗で止めている（2026-09-18 のステップ 5d2）。
- 原因: 対話デスクトップの代理人が動いていない（セッションをまたぐと落ちている）。`python Tools/desktop.py start` で上げ直す。
- 対処: そのうえで `ping` の `rect` を見る。`[-8, -8, 3448, 1400]` なら**最大化されていて収録の枠はビューポートではない**ので、`python Tools/desktop.py click 2680 12 --count 2` で元に戻す（クリックを 2 回に分けると間が空いてダブルクリックにならない）。`observations/tools/check_viewport.py` が `[1819, 68, 3279, 1269]` を確かめる。
- 出典: `observations/README.md` の「星屑を四芒星にした」（項目 23 のステップ 5d2）。

### PIE にキーを送っても届かない

- 対処: 先にビューポートを 1 回クリックして焦点を渡す（PIE を始めただけでは届かない）。PIE でないときにビューポートをクリックするとアクタを選ぶ（選択だけならレベルは汚れない）。前面の小窓（メッセージログ・Automation のログ）は先に閉じる。
- 出典: コミット 9264dac（2026-09-16）、`.claude/guides/verification.md`。

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

### 収録中に救急車の屋根からプレイヤーが落ちる（Zone 1 の救急車が走り出して約 1.5 s）

- 症状: テレポーテーションで救急車の屋根へ渡り、GDI の 60 fps で PIE を収録していると、救急車が走り出して約 1.5 s（約 700 cm/s）でプレイヤーが屋根の後ろへ抜けて落ち、救急車だけがトンネルへ去る（読み込み画面と Zone 2 には進む）。収録しない・`place` で屋根の真ん中に置く・`slomo 0.2` では落ちない。
- 原因: 屋根の後ろの壁 `BlockingVolume_Ambulance_3`（厚さ 20 cm、シーケンスが掃引なしで動かす）。テレポーテーションは後ろから狙うので屋根の後ろの端（壁から約 8 cm）に着き、収録の負荷でフレームレートが下がると 1 フレームの救急車の進みが隙間を超えて、壁がカプセルに食い込み後ろへ押し出される（推定。`t.MaxFPS 25` なら収録なしでも壁から 8 cm では落ち、真ん中では落ちない）。11 記録の既知の制約。
- 対処: 収録は `desktop.py record --grab gdi --fps 30`、テレポーテーションの狙いを E の後にホイールで 2 目盛り前へ寄せてから左クリック（`desktop.py scroll --dx 120 --allow UnrealEditor.exe` を 2 回。屋根の y −20000 に着く）。本作の直しはしていない（本家も同じ作り）。
- 確かめ方: PIE のプレイヤーの `character_movement.get_movement_base()`（UE 5.8 は非推奨の警告が出るが読める）と位置。土台が `BlockingVolume_Ambulance_5` で Z 402 のまま y が増えていれば乗っている。
- 出典: 進捗記録 `20260918-zone-progression.md`（2026-09-19 ステップ 10a）。

## 直さなくてよい既知の見え方

- **エディタの起動直後の「メッセージログ」**（起動時の読み込みエラー 1 件、GameFeatureData の設定の警告）— 前からあるもの。ビューポートの左に重なるので PIE の前に × で閉じる（進捗記録 `20260916-tablet-powers.md` の再開時の注意）。
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
- **本家の連写から星屑の形を読んで材質を推定する**（項目 23 のステップ 5d1・5d2）→ 白飛びした四芒星は星屑ではなく、原作の星屑はテクスチャを読まないひし形だった。材質は先に `Tools/dd/cooked_shaders.py` で式を読む（上の「原作の材質の式が書き出しに無い」）。
- **`burst_flow.py` の `flow`・`width` の px を `--ds` 倍せずに比べる** → 黙って本家が本作の半分の値に出る（`--ds 2` で測った本家は 2 倍、`SCALE=2.97` で `--ds 1` になる本作は 2.97 倍して本家の px にする）。項目 23 のステップ 5c はこれで「本作の流れが本家の 3〜4 倍」と読み違えた（5c2a で直した）。

## 未解決

- **`ddagrab` が最初のフレームで止まる原因**（2026-09-17）。回避は `gdigrab`。
- **Vanish の煙の位置と明るさ**（2026-09-17）— 本家の煙は 2 m 以上先に明るく出るが、原作の値からは 92 cm 先になる（上の「取り込み・レベル・描画」の節）。本家の実機で、煙の出る向き（見下ろす・横を向く）を撮れば、位置の手がかりになる。
- **テレキネシスの球の粒子の灯が、本家より明るく照らす**（2026-09-17、ステップ 11b4）— 同じ画質（「高」）で、線形の明るさの寄与が本家の約 1.4〜2 倍。灯の値は原作どおりで、GI は無く反射は SSR。UE 4.24 と 5.8 の単純な灯（`FSimpleLightEntry`。非逆二乗の減衰）の扱いの違いを疑っているが、UE 4.24 のソースが手元に無く確かめていない（04 記録）。
- **スカイライトのキューブマップ（`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`）が回収できない** — 書き出しは 512×512 の平面 PNG 1 面で、UE に取り込むと Texture2D になる。シーンのキャプチャに任せている。寄与はほぼ無い（強度を 0〜50 に振っても原作の 0.5 では平均が 0.05 も動かない）ので急がない（01 記録）。
- **Zone 2 のポストプロセスボリュームの `ColorGradingLUT`**（書き出しがアセットのパスの文字列で、取り込んだテクスチャに解決する仕組みが無い）**と `WeightedBlendables`**（`M_SharpenFilter_Inst`。マスターの式が cook で消えている）— `ColorGradingIntensity` が 0 なので見た目の寄与は無い（01 記録）。
- **焼いた後も残る 1〜2 割の明るさの差**（床の手前 72 対 57、エレベーターの壁、天井の中央）— 焼き込みの品質ではない（Preview → High で ±2 以内）。床の中ほどは正面の両開き扉（BP 由来、未実装）が無いための映り込み（00 記録の「灯の焼き込み」）。
- **駆動役の使用量の読み方と、使用量の上限に達したときの `claude -p` の返事の文言** — 要確認（ユーザー）（進捗記録 `20260917-autonomy.md`）。
