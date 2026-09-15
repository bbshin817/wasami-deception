# 実装記録の運用ルール

`.claude/implementation-records/` は「現行ソースが何をどう実装しているか」の記録。設計案や TODO ではなく、**いま動いているコードとアセットの説明**だけを書く。WebGL 版の同名のルールを UE5 版の対象範囲に合わせたもの。

## 構成

| パス | 内容 |
| --- | --- |
| `implementation-records/_index.md` | 記録一覧と、ソースファイル → 記録の対応表 |
| `implementation-records/00-*.md` | 全体像（プロジェクトの構成、設定、起動の流れ） |
| `implementation-records/NN-*.md` | 仕組みごとの記録（本体） |
| `implementation-records/_template.md` | 新規記録のひな形 |
| `implementation-records/_hashes.json` | 各記録が対象とするソースの blob ハッシュ（スクリプトが管理。手で編集しない） |
| `.claude/scripts/check_records.py` | 同期チェック / ハッシュ更新 |
| `.claude/scripts/post_edit_hook.py`、`stop_hook.py`、`session_start_hook.py` | Claude Code の hooks（`.claude/settings.json` で有効） |

## 記録の書き方

- 言語は日本語。テンプレートの frontmatter（`title`、`sources`、`updated`）と見出し構成を守る（`00-overview.md` だけは横断的な構成で例外）。
- `sources:` には記録が説明するファイルをリポジトリ相対パスで**すべて**列挙する。チェックスクリプトはこのリストを使う。
- 定数・既定値・アセットのパス（`/Game/...`）・設定キーは値まで書く。行番号は書かない（関数名・プロパティ名で参照する）。
- 原作データのどこから取った値かを書く（`pak_reference` のアセット名やバイトコードのオフセット、`cc2_reference` の BP 名）。
- 「なぜそうしているか」（エンジンの制約、回避したバグ、UE の版の違い）は「既知の制約・注意点」に残す。
- 1 記録 80〜300 行を目安にし、超えるなら分割して `_index.md` を更新する。

## ソースを変更したときの手順（必須）

1. `Source/`、`Content/Python/`、`Tools/`、`Config/`、`wasami_deception.uproject`、`.mcp.json` のいずれかを変更したら、対応する記録（`_index.md` の対応表で引く）の本文を現行実装に合わせて直す。
2. 挙動が変わったなら「変更履歴」に 1 行追加する（`- YYYY-MM-DD: 何を変えたか`）。
3. 新しいソースファイルを作ったら、既存記録の `sources:` に追加するか、新規記録を作って `_index.md` に載せる。
4. 最後に次を実行し、`OK` になることを確認する。

```powershell
python .claude/scripts/check_records.py            # 差分の確認
python .claude/scripts/check_records.py --update   # 記録を直し終えたらハッシュを更新
```

`--update` は記録本文を書き換えず、ハッシュと `updated:` だけを更新する。**本文を直さずにハッシュだけ更新することは禁止**（同期の意味がなくなる）。

## アセットの扱い

スクリプトで作り直せるアセット（`/Game/CC2`・`/Game/DD`・`/Game/Pipeline`・`/Game/Stage`）は git の外にあるので、ハッシュでは追えない。**それを作るスクリプトの記録に、何がどこにできるか（アセットのパス、数、設定）を書く**。手で作るアセット（UI のウィジェットなど）を足したときは、そのアセットを説明する記録を作る。

## hooks による自動化（`.claude/settings.json`）

- **PostToolUse(Edit/Write/MultiEdit)**: 記録対象のソースを編集した直後、どの記録を直すべきかを知らせる。
- **Stop**: 応答を終える前に `check_records.py` を実行し、未同期なら停止をブロックして記録の更新を促す（2 回目は警告のみで通す）。
- **SessionStart**: 未完了の進捗記録（`.claude/progress/`）と main 以外のローカルブランチ、直前のコミットを知らせる（中断からの再開用）。

hooks を無効にしていても、`check_records.py` を手元で実行すれば同じ検査ができる。

## チェックが報告する状態

| 状態 | 意味 | 対処 |
| --- | --- | --- |
| 要更新（stale） | ソースの内容が記録時から変わった | 記録本文を直して `--update` |
| 未登録（unhashed） | 記録の `sources` にあるがハッシュ未保存 | 記録内容を確認して `--update` |
| 欠落（missing） | 記録が参照するファイルが存在しない | `sources` から外すか記録を書き直す |
| 未記録（uncovered） | 対象範囲のファイルがどの記録にもない | 既存記録に追加するか新規記録を作る |
