---
title: 項目 52 積み残しの確かめ（パッケージ版のマウスとキー、note の GIF の撮り直し）
status: 進行中
branch: main
base: d573d72
started: 2026-09-23 06:40
updated: 2026-09-23 07:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 項目 52 積み残しの確かめ（パッケージ版のマウスとキー、note の GIF の撮り直し）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 52（大目標 4「レビュー指摘の修正」の最後の未着手）。2026-09-23 のユーザーの了承（項目 36・33 の要確認）で残った積み残し 3 つ。

- (a) パッケージ版で**タイトルの NEW GAME・欠片の画面の CLOSE・スコア画面の NEXT を実際にマウスとキーで押せるか**が未確認。通しプレイ（項目 36）は前面を離さないファイアウォールの窓のせいで `-ExecCmds` に回り道したので、クリックとキーそのものは一度も試せていない。
- (b) note の GIF `12-doors-busted.gif` は、扉が破られるときの煙が原作どおり黒くなったので撮り直す（前は窓が邪魔でビューポート全体を撮れていなかった）。
- (c) 項目 37 で敵ワサミの手のひらの向きが変わったので、敵が近くに写る `20-capture.gif`（捕獲の別室。手が画面いっぱいに来る）と `06-primal-fear.gif`（倒れて起き上がる腕立て）も撮り直す。`18-enemy-chase`・`42-chase-variations`・`19-sentry-jump` は敵が遠く手が数 px なので撮り直さない。

完了の条件（作業一覧の原文）:

1. 画面にファイアウォールの確認の窓とエディタの「出力ログ」の浮いた窓が無いことを確かめる（まだ出ていたらユーザーに知らせて止まる。**OS 全体の入力は操作しない**）。
2. パッケージ版（項目 39・41・43・47 と同じもの）を起動し、3 つのボタンをマウスで押して進み、キーでも進めることを確かめる。
3. ビューポート全体を撮って `12-doors-busted.gif`・`20-capture.gif`・`06-primal-fear.gif` を撮り直し、note の記事を更新する。

## 計画

- [x] 1. 完了の条件 (1): 画面が空いていることを確かめる … 2026-09-23 完了。下の「決定事項」に結果。
- [x] 2. 完了の条件 (2): NEW GAME・CLOSE・NEXT の 6 通り（マウスとキー）を押して確かめた … 2026-09-23 完了。結果は 00 記録と下の「決定事項」。
- [ ] 3. GIF `12-doors-busted.gif` と `06-primal-fear.gif` を撮り直す（どちらも Zone 1 の PIE）
  - 変更予定: `observations/ours/note/gif/12-doors-busted.gif`・`06-primal-fear.gif`（git の外）
- [ ] 4. GIF `20-capture.gif` を撮り直す（捕獲の別室。手が画面いっぱいに来る）
  - 変更予定: `observations/ours/note/gif/20-capture.gif`（git の外）
- [ ] 5. note の記事（id 180735989）を更新し、項目 52 を閉じて大目標 4 を「達成」にする
  - 変更予定: `docs/note/progress.md`、`.claude/roadmap.md`、`.claude/references/handover.md`、記録の削除

## 次にやること

ステップ 3。エディタは開き直してある（`L_Hospital_Zone2` が開いている見込み）。`observations/tools/check_viewport.py` で窓とビューポートの大きさを確かめてから、Zone 1 の PIE で `12-doors-busted.gif`（扉が破られるときの黒い煙）と `06-primal-fear.gif`（敵が倒れて起き上がる腕立て）を撮り直す。撮り方と枠は `.claude/guides/note-progress.md`、前に撮ったときの位置と台本は `observations/README.md`。

## 決定事項

- 2026-09-23: **完了の条件 (1)・(2) は済み**（結果は `.claude/implementation-records/00-overview.md` の「パッケージした本編の通しプレイ」と `.claude/references/troubleshooting.md`）。(1) ファイアウォールの窓も浮いた「出力ログ」の窓も無い。(2) 6 通りすべて通った（NEW GAME・CLOSE・NEXT × マウスとキー）。**キーは Tab で焦点 → Enter**。絵は `Intermediate/DesktopAgent/shots/` の `title-*`・`ring-*`・`score-*`・`after-next-*`。
- 2026-09-23: パッケージ版は**作り直さない** — 項目 39・41・43・47 が使ったものと同じ `Saved/Archive/Windows/wasami_deception.exe`（`d348ce5` から作った。2026-09-23 04:00）をそのまま使う。以後のコミットはエディタの外の前処理と記録だけで、クックの中身は変わらないため。
- 2026-09-23: GIF の枠はガイドのまま — エディタの窓を `(1819, 68, 3279, 896)` にしてビューポートを 1039 × 652 に戻してから撮る（`.claude/guides/note-progress.md` の「枠をそろえる」）。**今のエディタの窓はもうこの矩形**なので、直す必要は無いかもしれない（撮る前に `observations/tools/check_viewport.py` で確かめる）。

## 要確認（ユーザー）

1. **画面のボタンのキー操作の見た目と上下キー**（2026-09-23、ステップ 2 で分かった） — パッケージ版で Tab を押すと UMG の既定の焦点の枠（青い矩形）が出るだけで、押せる項目の色や大きさは変わらない（マウスで重ねたときの見た目とも違う）。タイトルの上下キーは 1 回で 2 つ飛ぶので（RESUME → EXTRAS → QUIT）、NEW GAME には Tab でしか行けない。本家はコントローラーの選択の見た目を持つが、本作は原作の UMG の式を写しただけでキーの選択を作っていない。**このままでよいか**（そのままなら何もしない。直すなら焦点の見た目と上下キーの 1 つずつの移動を足す）。

## 再開時の注意

- **エディタ**: ステップ 2 の終わりに `python Tools/editor_cycle.py --no-quit --no-build` で開き直した（パッケージ版と同時に動かさないため一度閉じた）。PIE は動いていない。
- **対話デスクトップの代理人**: 生きている（`python Tools/desktop.py ping` で確かめる。セッションをまたぐと落ちる）。パッケージ版へ入力を送るときは `--allow wasami_deception.exe` が要る。
- **パッケージ版**: `Saved/Archive/Windows/wasami_deception.exe`（Development。窓は `(812, 143, 2628, 1225)`）。起動は `Tools/game_perf.py` の `start_game`、終わりは `kill()`。ステップ 3・4 は PIE なので使わない。
- **GIF の枠**: エディタの窓を `(1819, 68, 3279, 896)` にしてビューポートを 1039 × 652 に戻してから撮る（`.claude/guides/note-progress.md` の「枠をそろえる」）。撮る前に `observations/tools/check_viewport.py` で確かめる。
- **時間のかかる処理**: エディタの開き直し（`Tools/editor_cycle.py`。C++ を触らないので数分）、PIE の収録。
- note の CLI は `tmp/note-cli/`、セッションの値は `Tools/note.local.json`（存在する）。記事は id 180735989・key n38f5d6ef4565、更新は `--no-notify`。

## 検証

- check_records: ステップ 2 の終わりに `--update` を通した
- C++ ビルド: 不要（この項目はコードを変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 3・4 の PIE でこれから
