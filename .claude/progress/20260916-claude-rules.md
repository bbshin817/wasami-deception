---
title: WebGL 版の .claude のルールをすべて引き継ぎ、進捗と中断再開に対応する
status: 進行中
branch: main
base: 045a310
started: 2026-09-16 08:20
updated: 2026-09-16 08:40
---

# WebGL 版の .claude のルールをすべて引き継ぎ、進捗と中断再開に対応する

## 依頼

> 元リポジトリの.claudeルールをすべて継承し、進捗・中断再開に対応してください。

## 計画

- [x] 1. 参考資料を移す（`<WEBGL>/.claude/references/` と WebGL 版の仕様・実装記録をこのリポジトリへ）
- [x] 2. 進捗記録のひな形と、実装記録の運用ルール・ひな形・索引
- [x] 3. 同期チェックとフックのスクリプト（UE5 版の対象範囲に合わせる）
- [x] 4. 検証の運用ルールと配布の運用ルール（ブラウザ版のルールを UE5 版に置き換える）
- [x] 5. 今あるコードの実装記録（全体像・取り込みの仕組み・プレイヤー）とハッシュ登録
- [x] 6. CLAUDE.md の索引を更新し、参照パスをこのリポジトリのものに直す
- [ ] 7. コミットして push し、この記録を消す ← 作業中

## 次にやること

ステップ 7: `.claude/` 一式（参考資料・ルール・記録・スクリプト・settings.json）と CLAUDE.md をコミットして push し、この進捗記録を最後のコミットで削除する。

## 決定事項

- 2026-09-16: WebGL 版の `.claude` のルールは、ブラウザ固有のもの（検証用ブラウザ、Cloudflare Pages）も UE5 版の同じ役割のルール（`verification.md`＝エディタと PIE での検証、`distribution.md`＝パッケージと配布）に置き換えて引き継ぐ。
- 2026-09-16: 参考資料（原作の調査 `dark-deception/`、CC2 の調査 `chaotic-customer-2/`、WebGL 版の README・CLAUDE.md・実装記録 19 件）はこのリポジトリの `.claude/references/` にコピーした（計約 1.1 MB）。参照データを移したのと同じく、WebGL 版のフォルダが無くても再開できるようにするため。
- 2026-09-16: 実装記録の対象範囲は `Source/`・`Content/Python/`・`Tools/`・`Config/`・`wasami_deception.uproject`・`.mcp.json`（拡張子 cpp/h/cs/py/ini/uproject/json）。`.claude/` 配下は対象外。スクリプトで作り直せるアセットはハッシュで追えないので、それを作るスクリプトの記録に書く。
- 2026-09-16: hooks は `.claude/settings.json` で有効にした（WebGL 版は `hooks.example.json` を貼る方式だった）。PostToolUse（編集したソースの記録を知らせる）、Stop（記録が未同期なら 1 回だけ停止をブロック）、SessionStart（未完了の進捗記録・main 以外のブランチ・未コミットの変更・直前のコミットを知らせる＝中断からの再開）。

## 再開時の注意

- MCP（unreal-mcp）は切断されたまま。エディタ側は正常（`http://127.0.0.1:8000/mcp` に POST で 200 が返る）なので、ユーザーに `/mcp` の画面で `unreal-mcp` を Reconnect してもらう。それまではエディタの操作は `python Tools/ue_remote.py` で行う。
- エディタは起動したまま（L_Zone1 を開いている）。PIE は止めてある。

## 検証

- check_records: OK（3 件の記録が対象 30 ファイルをすべて覆っている。未記録なし）
- hooks: 3 つとも手で流して確認（post_edit は記録名を返す、session_start は未完了の進捗記録とブランチを返す、stop は同期していれば exit 0）
- C++ ビルド: この作業ではソースを変えていない
