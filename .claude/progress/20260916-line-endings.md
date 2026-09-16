---
title: 行末を LF に固定して実装記録のハッシュのずれを防ぐ
status: 進行中
branch: main
base: d4c2006
started: 2026-09-16 10:00
updated: 2026-09-16 10:00
---

# 行末を LF に固定して実装記録のハッシュのずれを防ぐ

## 依頼

「再発防止策は1を撮って」（2026-09-16）。提示した 3 案のうち **1 番＝`.gitattributes` に `* text=auto eol=lf` を足して行末を固定する**（「既存ファイルすべてが一度書き換わります」と説明したうえでの選択）。

背景: `feature/hospital-stage` を main にマージしたときの再チェックアウトで、`core.autocrlf=true` によりファイルが CRLF に変換され、`check_records.py` のハッシュが 6 件ずれた（ソースの内容は変わっていない）。

## 計画

- [x] 1. この記録を作る
- [ ] 2. `.gitattributes` の先頭に `* text=auto eol=lf` を足す ← 作業中
  - 変更予定: `.gitattributes`
  - 後の行が勝つので、LFS の 2 行より**上**に置く（LFS の行は `-text` 付きなので影響を受けない）
- [ ] 3. **先にコミットしてから**作業ツリーを取り込み直して LF にする（`git rm --cached -r . && git reset --hard`）
- [ ] 4. LF になったことを確かめ、`python .claude/scripts/check_records.py --update` でハッシュを取り直す
- [ ] 5. この記録を消してコミットし、main を push する

## 次にやること

ステップ 2。`.gitattributes` に行を足したら、**ステップ 3 の `git reset --hard` より前に必ずコミットする**（コミットしないと reset で変更が消える）。

## 決定事項

- 2026-09-16: 案 1（`.gitattributes` で固定）を採る — ユーザーの選択。案 2（`check_records.py` 側で行末を正規化）と案 3（何もしない）は採らない。

## 再開時の注意

- 追跡ファイルは 86 件すべてテキスト（md 46・py 19・ini 5・json 4・h 3・cs 3・cpp 3 ほか）。**`.uasset` / `.umap` の追跡は 0 件、LFS 管理のファイルも 0 件**なので、バイナリが壊れる心配はない。
- リポジトリ側（index / HEAD）はもともと LF を保存している（だから `git status` は clean のまま）。この変更で**コミットの中身は変わらず、直るのは作業ツリーだけ**。
- ステップ 3 で `git reset --hard` を使う。実行前に `git status` が clean であることを確かめること（開始時は clean）。
- エディタは起動中だが、`/Game` のアセットは `.gitignore` の対象なので影響しない。
- ステップ 3 の後はハッシュがまたずれる（CRLF → LF）ので、**必ずステップ 4 を実行する**。

## 検証

- check_records: 未実行
- C++ ビルド: 不要（C++ は変えない）
- エディタでの確認: 不要
