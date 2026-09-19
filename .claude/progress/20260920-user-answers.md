---
title: 要確認への回答の反映（2026-09-20）
status: 進行中
branch: main
base: acb1ee3
started: 2026-09-20 10:00
updated: 2026-09-20 11:40
---

# 要確認への回答の反映（2026-09-20）

## 依頼

有人セッションで、作業一覧の「未回答の要確認」と項目 12 の記録の要確認（16 件）に回答をもらった。番号は有人セッションで示した一覧の番号。

1. EXTRAS は実装し、タイトル画面から見られる形に（中身は「枠組みだけ先に作る」）→ 作業一覧の新しい項目 29
2. オプション画面の Esc は閉じない（本家どおり）
3. EASY でライフ 0 の死亡画面は本家どおり（ポーズも開かない行き止まり）
4. はい（3 で道が無くなるので効かない）
5. ポーズの RESTART の YES でライフを 3 に戻す
6. 版表記は v1.0.0
7. TOTAL SHARDS の 679 は 1 回だけ数える
8. スコア画面の題字は WebGL 版の「Stinky Gachimi」
9〜12・14〜16: 今のままでよい（仮を外す）
13: 保留

## 計画

- [x] 1. コードの直し（3・5・6・7・8）と、答えの出た `TODO(仮)` を外す … 2026-09-20 完了。実装記録 00・01・02・07・09・13・14・15・18 の変更履歴。
- [x] 2. ビルド、題字の取り込み、テスト … 2026-09-20 完了。`Wasami.LevelClear/Pause/DeathScreen/GameFlow/Title/Secrets` の 38 件が通り、PIE の `Wasami.LevelClear` で赤の「Stinky Gachimi」と TOTAL SHARDS 784 を見た。
- [x] 3. 記録と作業一覧（「未回答の要確認」は 13 の保留だけ残し、項目 29〈EXTRAS〉を足した）、項目 12 の記録の要確認、note の原稿の文（ポーズの EASY の一文）と記事の書き換え … 2026-09-20 完了。
- [ ] 4. note の記事の GIF `observations/ours/note/gif/24-level-clear.gif`（スコア画面）を撮り直す（題字が「Torment Therapy」、合計が 1,483 の古い画面のまま）

## 次にやること

ステップ 4: `python Tools/pie.py start` → `python Tools/desktop.py start` → `python Tools/playthrough.py run z2_escape --setup --record escape_clear.mkv`（ポータルをくぐって You Escaped! からリザルトまで。前の GIF は 640 × 401・約 10.5 s）→ `pie.py stop`。`.claude/guides/note-progress.md` の「GIF にする」の ffmpeg で 24-level-clear.gif を作り直し、note の記事を書き換える（原稿の文は直してある）。済んだらこの記録を消してコミットする。

## 決定事項

- 2026-09-20: 回答は上の「依頼」のとおり（反映済み）。

## 要確認（ユーザー）

- （なし）

## 再開時の注意

- 2026-09-20 の有人セッションでは、Chrome リモートデスクトップの共有バー（`remoting_desktop.exe`。デスクトップのエージェントには実行ファイルの名前が読めず空になる）が前面にあり、`desktop.py` の入力がすべて断られたので撮れなかった。前面がエディタでないと台本もビューポートをクリックできない。
- `playthrough.py --setup` は開発用のセーブ（`Saved/SaveGames/structSlot.sav`）を書き換える。撮る前に控えを取り、撮った後に戻す。
