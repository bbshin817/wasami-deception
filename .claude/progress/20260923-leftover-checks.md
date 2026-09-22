---
title: 項目 52 積み残しの確かめ（パッケージ版のマウスとキー、note の GIF の撮り直し）
status: 進行中
branch: main
base: d573d72
started: 2026-09-23 06:40
updated: 2026-09-23 06:45
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
- [ ] 2. 完了の条件 (2): パッケージ版を起動し、NEW GAME・CLOSE・NEXT をマウスで押し、キーでも進めることを確かめる
  - 変更予定: `.claude/references/troubleshooting.md`（ファイアウォールの項に「2026-09-23 に解消」を足す）、`.claude/implementation-records/13-*`（要れば）
- [ ] 3. GIF `12-doors-busted.gif` と `06-primal-fear.gif` を撮り直す（どちらも Zone 1 の PIE）
  - 変更予定: `observations/ours/note/gif/12-doors-busted.gif`・`06-primal-fear.gif`（git の外）
- [ ] 4. GIF `20-capture.gif` を撮り直す（捕獲の別室。手が画面いっぱいに来る）
  - 変更予定: `observations/ours/note/gif/20-capture.gif`（git の外）
- [ ] 5. note の記事（id 180735989）を更新し、項目 52 を閉じて大目標 4 を「達成」にする
  - 変更予定: `docs/note/progress.md`、`.claude/roadmap.md`、`.claude/references/handover.md`、記録の削除

## 次にやること

ステップ 2。`python Tools/console_session.py Saved/Archive/Windows/wasami_deception.exe --wait wasami_deception-Win64-Shipping.exe` でパッケージ版を対話デスクトップに起動し、`python Tools/desktop.py ping` で前面がゲームの窓になったことを確かめてから、タイトルの NEW GAME を `desktop.py click` で押す。以降は欠片の画面の CLOSE、スコア画面の NEXT まで。キーでも進めることは、同じ 3 つの画面で Enter／Space を `desktop.py key` で送って確かめる。押す座標は `Shot showui` の絵か `desktop.py shot --region` で測る。**終わったらゲームを必ず閉じる**（`.claude/guides/verification.md`。エディタと同時に動かさない）。

## 決定事項

- 2026-09-23: **完了の条件 (1) は満たされている** — `Tools/desktop.py ping` の前面は `wasami_deception - Unreal Editor`（rect `(1819, 68, 3279, 896)`）で、`PickerHost.exe`（Windows セキュリティ）ではない。`EnumWindows` で見える最上位の窓を数えると UnrealEditor の窓は 1 つだけで、浮いた「出力ログ」の窓は無い。画面の絵（`Intermediate/DesktopAgent/shots/shot-064324.png`）でも確認。ユーザーが 2026-09-23 にファイアウォールを許可したため。これで**画面へのクリックとキーが要る作業ができる**ようになった（項目 36 のステップ 4・5 で塞がれていたもの）。
- 2026-09-23: パッケージ版は**作り直さない** — 項目 39・41・43・47 が使ったものと同じ `Saved/Archive/Windows/wasami_deception.exe`（`d348ce5` から作った。2026-09-23 04:00）をそのまま使う。以後のコミットはエディタの外の前処理と記録だけで、クックの中身は変わらないため。
- 2026-09-23: GIF の枠はガイドのまま — エディタの窓を `(1819, 68, 3279, 896)` にしてビューポートを 1039 × 652 に戻してから撮る（`.claude/guides/note-progress.md` の「枠をそろえる」）。**今のエディタの窓はもうこの矩形**なので、直す必要は無いかもしれない（撮る前に `observations/tools/check_viewport.py` で確かめる）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **エディタ**: 起きている（pid 30388、窓 `(1819, 68, 3279, 896)`）。開いているレベルは `L_Hospital_Zone2`（ビューポートのカメラは項目 51 で Zone 2 の落書きを見た位置）。PIE は動いていない。
- **対話デスクトップの代理人**: この反復で `python Tools/desktop.py start` を走らせて上げた。セッションをまたぐと落ちるので、再開したらまず `ping` で生きているか見る。
- **パッケージ版とエディタは同時に動かさない**（`.claude/guides/verification.md`）。ステップ 2 でゲームを起動する前にエディタを閉じ（保存してから）、終わったら `python Tools/editor_cycle.py` で開き直す。ステップ 3・4 の GIF は PIE なのでエディタが要る。
- **パッケージ版で Zone 2 の好きな場所へ行くには、先に Zone 1 に入ってから `Wasami.Checkpoint 10` を送る**（`Wasami.Checkpoint` はゾーンのゲームモードが要るので `L_Title` では効かない。項目 47 の注）。
- **時間のかかる処理**: パッケージ版の通し（`Tools/game_flow.py run` で約 4 分）、エディタの開き直し（`Tools/editor_cycle.py`。C++ を触らないので数分）。
- note の CLI は `tmp/note-cli/`、セッションの値は `Tools/note.local.json`（存在する）。記事は id 180735989・key n38f5d6ef4565、更新は `--no-notify`。

## 検証

- check_records: 未実行
- C++ ビルド: 不要（この項目はコードを変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
