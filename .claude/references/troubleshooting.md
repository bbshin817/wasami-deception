# 症状索引（会ったことのある問題と、その対処）

エラー文やおかしな挙動から引く索引。**症状で引けること**が目的なので、エラー文はそのまま（grep できる形で）書く。詳しい理由は実装記録（`.claude/implementation-records/`）の「既知の制約・注意点」に残し、ここからはリンクで結ぶ。決まりは `.claude/guides/autonomy.md` の「症状索引」。

- **引く**: エラー文やおかしな挙動に会ったら、まずこのファイルを grep する。
- **書く**: 解決にエディタの開き直し 1 回以上、または 30 分以上かかったら、次に進む前にその場で書く。直さなくてよい既知の見え方（「これは正しい」）と、試して駄目だった案（「これはやらない」）も書く。
- 1 件の形: 見出しに症状、本文に「原因 / 対処 / 確かめ方 / 出典（記録・コミット・日付）」。分類は下の見出しで分ける。未解決のものは「未解決」と書く。

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

## エディタ・MCP・リモート実行

（ステップ 3 で拾う）

## Python（UE 5.8 の API の罠）

（ステップ 3 で拾う）

## C++・ビルド・テスト

（ステップ 3 で拾う）

## 取り込み・レベル・描画

（ステップ 3 で拾う）

## 画面の操作・本家の実機

（ステップ 3 で拾う）

## 直さなくてよい既知の見え方

（ステップ 3 で拾う）

## 試して駄目だった案

（ステップ 3 で拾う）

## 未解決

（ステップ 3 で拾う）
