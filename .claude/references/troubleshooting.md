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

### Python のツールセット（`WasamiDDTools` など）が `unreal.` の下に無い

- 症状: リモート実行で `unreal.WasamiDDTools` が `AttributeError`。
- 原因: Python で書いたツールセットは `unreal` モジュールには出ない。
- 対処: `from wasami_tools.toolsets.dd import WasamiDDTools` で読む。
- 出典: 01 記録。

### MCP のツールを並べて呼ぶとエディタが止まる

- 症状: 1 つの応答で複数の `call_tool` を並べると返ってこない。
- 原因: ツールはゲームスレッドで動く。
- 対処: 1 つずつ順に呼び、結果を確かめてから次を呼ぶ（失敗しても例外にならない道具がある）。
- 出典: `.claude/guides/unreal-workflow.md`。

### エディタが背面にあると PIE のティックが 3 fps ほどに落ちる（時間に依存する確認があてにならない）

- 症状: リモート実行から `LaunchCharacter` などで動かしても速さが出ない。Automation テストも進まない（テストの道具は前面で 10 fps を超えるまで、背面なら最大 600 秒待つ）。`UWidgetComponent` の画面（タブレットの地図）も描き直されない（`bTickWhenOffscreen` が偽）。
- 原因: `Use Less CPU when in Background`。
- 対処: `python Tools/desktop.py click 2957 95 --allow UnrealEditor.exe`（タイトルバーの空き。エディタの窓が今の位置のとき。撮った画面でクリックの位置がエディタの上であることを先に見る）でエディタを前面にする。PIE を始めてもエディタは前面に来ない。
- 確かめ方: `stat fps`、またはリモート実行で `unreal.GameplayStatics.get_time_seconds` の進み。
- 出典: `.claude/guides/verification.md` の「動きの確認」（2026-09-16）、03 記録。「試して駄目だった案」も参照。

### Automation テストを始めると PIE が止まる

- 症状: PIE の確認中にテストを走らせると PIE が終わる。
- 原因: エンジンの仕様（テストの開始で PIE を止める）。
- 対処: テストと PIE の確認は続けて行い、同時に走らせない。テストの後に出る Automation のログの小窓は PIE の前に閉じる。
- 出典: `.claude/guides/verification.md`。

### `capture_pose`（SceneCapture2D）の絵が PIE より暗い

- 症状: 同じ視点なのに `WasamiDevTools.capture_pose` は (13, 13, 0)、PIE は (41, 38, 25) と出る。
- 原因: シーンキャプチャは本編と同じには間接光を回さない。
- 対処: 設定どうしの A/B には使えるが、実機との数値の突き合わせは PIE で撮る（`HighResShot` か `Tools/desktop.py shot`）。PIE 中のリモート実行は `get_game_world()` を使う。
- 出典: コミット 97676a8（2026-09-16）、01 記録。

## Python（UE 5.8 の API の罠）

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
- 対処: 定数や補助の名前はファイルごとに固有にし、UE のメンバー名と同じローカル変数を避ける。
- 出典: 04 記録の「既知の制約」と「確かめたこと」（ステップ 7・8）。

### Automation テストで、一時的なワールドのアクタがイベントを捨てる

- 症状: テストのワールドに置いた的が、パワーの作用（インターフェースの呼び出し）を受けない。
- 原因: アクタが初期化前（`InitializeActorsForPlay` を通っていない）。
- 対処: ワールドを作ったら `InitializeActorsForPlay` を呼ぶ（`Wasami.Powers.PrimalStun`）。
- 出典: 進捗記録 `20260916-tablet-powers.md` の検証（ステップ 6）。

### ヘッダーや UCLASS / UPROPERTY の変更が Live Coding で効かない

- 対処: `python Tools/editor_cycle.py`（保存 → 閉じる → UBT → 開き直す）。尋ねずに走らせる。Live Coding で直したファイルは次のフルビルドで取り込まれる。
- 出典: `.claude/guides/unreal-workflow.md`。

### 歩くとタブレットがシェイクと逆に揺れる（カメラの子にした部品）

- 症状: 歩行のカメラシェイクの間、カメラの子の板が画面上で逆方向に揺れる。
- 原因: UE のカメラシェイクはカメラマネージャの視点（POV）にだけ掛かり、`UCameraComponent` は動かない。
- 対処: アクタのティックを `TG_PostUpdateWork` に移し、毎フレーム POV に視点空間の位置を掛けて板のワールド変換を置き直す（`PlaceTablet`）。
- 確かめ方: シェイク中に POV が揺れる間、板の視点空間の位置が不動。
- 出典: コミット bdb11ef（2026-09-16）、02 記録。

## 取り込み・レベル・描画

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

## 画面の操作・本家の実機

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
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11b1）。

### 本家の一瞬の演出が `gdigrab` で粗くしか撮れない（1 秒に約 10 枚）

- 症状: 本家（全画面）を `record --grab gdi` で撮ると、`--fps 60` でも 1 秒に約 10 枚。範囲を 1720 × 720 や 860 × 360 に絞っても 3 秒で 16 枚だった（エディタのビューポートなら 60 fps で撮れた）。
- 対処: MOD の `Console Command` に `slomo 0.25` と打ってから撮る（シッピングでも効く。テレキネシスの演出が 1.3 秒 → 4.7 秒に延びた）。レベルを読み直すと 1 に戻る。タブレットの出し入れは slomo でも約 0.7 秒のままだった（理由は未確認）。`Console Command` の欄は 1 回クリックしてから `type` で打ち、`enter`。
- 注意: 入力の直前に 0.3 s ほど途切れることがある（`orig-primal-a` は 1.683 → 2.067 s）。最初に写った変化を始まりとすると、演出が本作より短く見える。終わりの時刻から逆算して合わせる（`.claude/guides/observation.md` の「6. 測る」。11b1 の「Primal の閃光が 1.5 倍長い」はこれで、11b2 で見直した）。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a・11b2）、`observations/README.md`。

### 本家の MOD の無敵が効かず、Zone 1 で Reaper Nurse に捕まる

- 症状: MOD の Maps の ZONE 1（シャードの並ぶ待合の廊下）に飛ぶと、Reaper Nurse 3 体が約 7 秒で来て捕まる。MOD の Settings で `God Mode` を見ると OFF のことがあり、テンキーの 3 を送っても効いたか分からないまま捕まった。MOD のメニューを開いている間もゲームは進む。捕まり続けると `You Are Dead` → `Restart?` で入口（`06_Hospital`）からやり直しになる。
- 対処: 無敵は当てにしない。着いたらすぐ `M` → Active Enemy の `Find All`（(1327, 860)）→ `Remove All`（(1947, 860)）で敵を消す（敵が要る観察は、消す前の数秒で済ませる。順番は `.claude/guides/observation.md`）。敵のいない開始地点（ZONE 1 STARTING POINT）で済む観察はそこで行う。テンキーはエージェントの `num0`〜`num9`。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a）。

### 本家で `desktop.py click` を送ると視点が大きく回る / MOD のメニューの押し間違い

- 症状: ゲーム中の `click X Y` はカーソルを絶対座標へ動かすので、その分だけ視点が回る（真下を向いた）。MOD のメニューの左の列は、ホイールで送った位置のまま残るので、前に測った座標で別の項目（W-Editor）を押した。
- 対処: ゲーム中のクリックは画面の中央 (1720, 720) で行う。MOD のメニューは押す前に列を上端へ戻し（`scroll` 8 回）、撮って項目の位置を確かめる（上端の座標の表は `.claude/guides/observation.md`。W-Editor は (990, 487)、ボタンは y 457〜517 で、11a の「Logs のつもり」の (987, 510) はここに当たった）。**W-Editor の画面が開いたら何も動かさずに `Close` で閉じる**（開いただけで、カーソルの下の扉 `BP_06_DoubleDoors13` の変換が元の値のまま `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に書かれた。値は原作と同じなので見え方は変わらない）。
- 出典: 進捗記録 `20260916-tablet-powers.md`（2026-09-17 ステップ 11a）。

### PIE で動いている最中の絵が撮れない

- 症状: `desktop.py hold w` の間はエージェントが撮影できない。PIE の `shot showui` はエディタ全体を撮り、ビューポートが黒い。
- 対処: エディタの Python で `register_slate_post_tick_callback` に `player.add_movement_input(forward, 1.0, False)` を入れて前進させ、その間に `desktop.py shot`。UI を含む絵はエージェントの `shot`。
- 出典: `.claude/guides/verification.md`（2026-09-16）。

### エージェントが入力を断る / 反応しない

- 症状: 「前面の窓が許可の一覧に無い」で断られる。Win・Alt+Tab・Alt+F4 も断る。30 分何も来ないと自分で終了する。
- 対処: エディタに送るときは `--allow UnrealEditor.exe`（VS Code が前面で、エディタを前面にするクリックだけなら `--allow Code.exe` も）。終了していたら `python Tools/desktop.py start`。何を送ったかは `Intermediate/DesktopAgent/agent.log`。
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

- **背面のエディタの 3 fps を設定で直す**: コンソールの `set EditorPerformanceSettings bThrottleCPUWhenNotForeground False` は効かず、設定は Python から見えない → エディタを前面にする（2026-09-16、`.claude/guides/verification.md`）。
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

## 未解決

- **`ddagrab` が最初のフレームで止まる原因**（2026-09-17）。回避は `gdigrab`。
- **Vanish の煙の位置と明るさ**（2026-09-17）— 本家の煙は 2 m 以上先に明るく出るが、原作の値からは 92 cm 先になる（上の「取り込み・レベル・描画」の節）。本家の実機で、煙の出る向き（見下ろす・横を向く）を撮れば、位置の手がかりになる。
- **テレキネシスの球の粒子の灯が、本家より明るく照らす**（2026-09-17、ステップ 11b4）— 同じ画質（「高」）で、線形の明るさの寄与が本家の約 1.4〜2 倍。灯の値は原作どおりで、GI は無く反射は SSR。UE 4.24 と 5.8 の単純な灯（`FSimpleLightEntry`。非逆二乗の減衰）の扱いの違いを疑っているが、UE 4.24 のソースが手元に無く確かめていない（04 記録）。
- **スカイライトのキューブマップ（`HDRI_Epic_Courtyard_Daylight`・`TC_HDR01`）が回収できない** — 書き出しは 512×512 の平面 PNG 1 面で、UE に取り込むと Texture2D になる。シーンのキャプチャに任せている。寄与はほぼ無い（強度を 0〜50 に振っても原作の 0.5 では平均が 0.05 も動かない）ので急がない（01 記録）。
- **Zone 2 のポストプロセスボリュームの `ColorGradingLUT`**（書き出しがアセットのパスの文字列で、取り込んだテクスチャに解決する仕組みが無い）**と `WeightedBlendables`**（`M_SharpenFilter_Inst`。マスターの式が cook で消えている）— `ColorGradingIntensity` が 0 なので見た目の寄与は無い（01 記録）。
- **焼いた後も残る 1〜2 割の明るさの差**（床の手前 72 対 57、エレベーターの壁、天井の中央）— 焼き込みの品質ではない（Preview → High で ±2 以内）。床の中ほどは正面の両開き扉（BP 由来、未実装）が無いための映り込み（00 記録の「灯の焼き込み」）。
- **駆動役の使用量の読み方と、使用量の上限に達したときの `claude -p` の返事の文言** — 要確認（ユーザー）（進捗記録 `20260917-autonomy.md`）。
