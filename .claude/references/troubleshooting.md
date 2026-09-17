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
