---
title: note の進捗記事のステージ OP の GIF を撮り直す（作業一覧の項目 45 の後始末）
status: 進行中
branch: main
base: 02e2a56
started: 2026-09-22 18:40
updated: 2026-09-22 18:40
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# note の進捗記事のステージ OP の GIF を撮り直す（作業一覧の項目 45 の後始末）

## 依頼

`CLAUDE.md` と `.claude/guides/note-progress.md` の「実装が 1 つ終わるたびに原稿 `docs/note/progress.md` を直し、同じ記事を書き換える」「**見た目や操作が変わったものは GIF を撮り直す**」から。

作業一覧の**項目 45 でステージ OP の紋章の見え方が変わった**（`HeadOffset`（0, −11.3）でワサミの頭が輪の中心へ上がった）ので、記事の `### ステージの始まりのタイトルカード` の GIF `observations/ours/note/gif/37-stage-op.gif`（2026-09-20 04:36 に撮ったもの、6.6 MB）が今の画面と違う。紋章は GIF（幅 640）の中で直径 290 px ほどなので、頭の 11.3 px（470 px 換算）は 7 px ほどの違いになる。項目 43・44 のタイトルの GIF（`25-title.gif`）と同じ後始末。

## 計画

- [ ] 1. GIF を撮り直して原稿と note を直す ← 作業中
  - 撮るもの: 今の `37-stage-op.gif` と同じ構成（**エレベーターの中でタイトルカードが出て消えるまで**）。本文（`docs/note/progress.md` の 22〜26 行）は出来ることが変わっていないので直さない見込み。
  - 出し方: `python Tools/pie.py start`（エディタは `L_Hospital_Zone1` を開いている）→ `Wasami.ResetSave` → 新しい始まりでステージ OP が出る（11 記録の「確かめたこと」の「ステージ OP の通し」。出してから約 9 s で薄れて消え、10 s でプレイヤーが動ける）。タイトルから通す要は無い。紋章だけを出す `Wasami.ChapterPortal` は**使わない**（記事の GIF は本物の始まりを見せる）。
  - 撮り方は `.claude/guides/note-progress.md` の「GIF」に従う（PIE → `pie.py cmd "DisableAllScreenMessages" "t.MaxFPS 60"` → ビューポートを 1 回クリック → `Tools/desktop.py record --grab gdi --region ...` → 終わったら `pie.py stop`）。ビューポートの領域は 2026-09-22 に測り直した: **エディタの窓 (1819, 68)-(3279, 1269) の中の (7, 137) から 1039 × 1025**（= 画面の (1826, 205)-(2865, 1230)）。偶数の大きさに丸めて使う。
  - **背面のエディタは 3 fps になるので、撮る前に `bThrottleCPUWhenNotForeground` を偽にし、終わったら真に戻す**（症状索引）。窓を前へ出すのは `python observations/tools/raise_editor.py`。
  - GIF にするコマンドと幅 640・`-loop 0` は同じガイドの「GIF にする」。
  - note への反映は同じガイドの「コマンド」（`tmp/note-cli`、記事の id はガイドに、セッションの値は `Tools/note.local.json`）。**ファイルが無ければ原稿だけ直して「note へは未反映」と報告する。**
  - 変更予定: `observations/ours/note/gif/37-stage-op.gif`（git の外）、`docs/note/progress.md`（直すところがあれば）

## 次にやること

ステップ 1。`L_Hospital_Zone1` で PIE を始め、`Wasami.ResetSave` の後にエレベーターの中のステージ OP が出て消えるまでを撮って GIF を差し替え、note へ反映する。

## 決定事項

- 2026-09-22: **撮り直す GIF は 1 本だけ**（`37-stage-op.gif`）。項目 45 が変えたのはステージ OP の紋章の中の頭の位置だけで、ほかの 43 本の GIF が映すものは変わっていない（読み込み画面の紋章は直していない）。
- 2026-09-22: **項目 45 は撮り直しを待たずに「完了」にした**（作業一覧・09 記録・handover は 02e2a56 で閉じてある）。note の反映は運用の後始末なので、この記録で別に進める。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理は無い。PIE は終わったら必ず止める（`python Tools/pie.py stop`）。
- エディタは `L_Hospital_Zone1` を開いていて、PIE は動いておらず、未保存のマップも無い（2026-09-22 18:35 に確かめた）。`bThrottleCPUWhenNotForeground` は真（既定）に戻してある。
- 項目 45 の確かめに使った絵は `observations/portal_symbol/pie_portal_screen.png`（今のステージ OP の全画面）・`pie_portal_grid.png`・`pie_loader_grid.png`。撮り直した GIF と見比べるのに使える。

## 検証

- check_records: OK（20 件。この記録の作業ではソースを変えない見込み）
- エディタでの確認（PIE の収録）: 未実行
