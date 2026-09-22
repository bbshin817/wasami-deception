---
title: note の進捗記事のタイトル画面の GIF を撮り直す（作業一覧の項目 43・44 の後始末）
status: 進行中
branch: main
base: 5b03250
started: 2026-09-22 18:00
updated: 2026-09-22 18:00
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# note の進捗記事のタイトル画面の GIF を撮り直す（作業一覧の項目 43・44 の後始末）

## 依頼

`CLAUDE.md` と `.claude/guides/note-progress.md` の「実装が 1 つ終わるたびに原稿 `docs/note/progress.md` を直し、同じ記事を書き換える」「**見た目や操作が変わったものは GIF を撮り直す**」から。

作業一覧の**項目 43（タイトルの曲）と項目 44（筆の跡と顔の左の縁）でタイトル画面の見え方が変わった**ので、記事の `### タイトル画面から始まる` の GIF `observations/ours/note/gif/25-title.gif`（2026-09-19 に撮ったもの）が今の画面と違う。差は目で分かる大きさ（同じ枠 860 × 360 で測って、顔の領域 x 560-860・y 20-340 の平均輝度が 10.70 → 17.62、右端の列 x 850-860 が 0.00 → 8.38〈黒い隙間が消えた〉、筆の跡の箱が 2.88 → 3.85）。

## 計画

- [ ] 1. GIF を撮り直して原稿と note を直す ← 作業中
  - 撮るもの: 今の `25-title.gif` と同じ構成（**タイトル画面 → NEW GAME → 赤い明滅と暗転 → Zone 1 のエレベーターの到着**。暗転の間は ffmpeg で縮める）。本文（`docs/note/progress.md` の 17〜21 行）は機能が変わっていないので直さない見込み。曲は GIF に入らない。
  - 撮り方は `.claude/guides/note-progress.md` の「GIF」に従う（PIE →`DisableAllScreenMessages`・`t.MaxFPS 60` → ビューポートの領域を `observations/tools/check_viewport.py` で確かめて `Tools/desktop.py record --grab gdi --region ...` → 終わったら `pie.py stop`）。**NEW GAME で RESTART? が出ないよう、PIE を始めたら先に `Wasami.ResetSave` を実行する**（14 記録の「ボタンの道」: 進みが無ければ問わない）。
  - GIF にするコマンドと幅 640・`-loop 0` は同じガイドの「GIF にする」。
  - note への反映は同じガイドの「コマンド」（`tmp/note-cli`、記事の id はガイドに、セッションの値は `Tools/note.local.json`。**ファイルはある**ので反映まで行う）。
  - 変更予定: `observations/ours/note/gif/25-title.gif`（git の外）、`docs/note/progress.md`（直すところがあれば）

## 次にやること

ステップ 1。エディタで `L_Title` を開いて PIE を始め（今のエディタは `L_Title` を開いている）、`Wasami.ResetSave` の後に NEW GAME を押すところから Zone 1 の到着までを撮って、GIF を差し替え、note へ反映する。

## 決定事項

- 2026-09-22: **撮り直す GIF は 1 本だけ**（`25-title.gif`）。項目 43・44 が変えたのはタイトル画面だけで、ほかの 10 本の GIF が映すものは変わっていない。
- 2026-09-22: **項目 44 は撮り直しを待たずに「完了」にした**（作業一覧・14 記録・handover は 5b03250 で閉じてある）。note の反映は運用の後始末なので、この記録で別に進める。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理は無い。PIE は終わったら必ず止める（`python Tools/pie.py stop`）。
- エディタは `L_Title` を開いていて、PIE は動いておらず、未保存のマップも無い（2026-09-22 18:00 に確かめた）。
- 別窓の PIE の設定（`NewWindowWidth` 等）はユーザーの値 1280・720・偽に戻してある。GIF はエディタのビューポートで撮るので触らない。

## 検証

- check_records: OK（20 件。この記録の作業ではソースを変えない見込み）
- エディタでの確認（取り込み・PIE）: 未実行
