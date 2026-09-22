---
title: 項目 52 積み残しの確かめ（パッケージ版のマウスとキー、note の GIF の撮り直し）
status: 進行中
branch: main
base: d573d72
started: 2026-09-23 06:40
updated: 2026-09-23 07:35
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
- [x] 3. GIF `12-doors-busted.gif` と `06-primal-fear.gif` を撮り直した … 2026-09-23 完了。ビューポート全体（640 × 401）で撮り直し、撮り方は `.claude/guides/note-progress.md` の「2026-09-23 の撮り方」と (06) の行に書いた。
- [ ] 4. GIF `20-capture.gif` を撮り直す（捕獲の別室。手が画面いっぱいに来る）
  - 変更予定: `observations/ours/note/gif/20-capture.gif`（git の外）
- [ ] 5. note の記事（id 180735989）を更新し、項目 52 を閉じて大目標 4 を「達成」にする
  - 変更予定: `docs/note/progress.md`、`.claude/roadmap.md`、`.claude/references/handover.md`、記録の削除

## 次にやること

ステップ 4。`20-capture.gif`（捕獲の別室。手が画面いっぱいに来る）を撮り直す。エディタは `L_Hospital_Zone1` を開いたまま、PIE は止めてある。窓は既に GIF の矩形。前の GIF は 4 種のうち 2 種をつないだもので、`Wasami.Lives N` でライフを増やし `Wasami.Capture N`（N 0〜2 が Capture_1〜3、3 が顔）で出せる。今の GIF の中身は `ffmpeg -i observations/ours/note/gif/20-capture.gif -vf "fps=2,scale=288:-1,tile=5x4" -frames:v 1 <png>` で確かめてから、同じ 2 種を同じ長さで撮る。

## 決定事項

- 2026-09-23: **完了の条件 (1)・(2) は済み**（結果は `.claude/implementation-records/00-overview.md` の「パッケージした本編の通しプレイ」と `.claude/references/troubleshooting.md`）。(2) は 6 通りすべて通った（NEW GAME・CLOSE・NEXT × マウスとキー。キーは Tab で焦点 → Enter）。
- 2026-09-23: パッケージ版は**作り直さない** — 項目 39・41・43・47 が使った `Saved/Archive/Windows/wasami_deception.exe` をそのまま使う（以後のコミットでクックの中身は変わらない）。ステップ 3〜5 は PIE だけなので使わない。
- 2026-09-23（ステップ 3）: **GIF の演出を邪魔する物は PIE のワールドで消す**。06 は廊下の餅を隠すだけでなく、項目 30 で足した除細動器 `WasamiDefib`（(−5, −796)）を `destroy_actor()` で消した（赤い放電が 3〜4 秒ごとに画面いっぱいに横切り、2026-09-18 の GIF には無かった）。12 は流れの `On06Transition` を呼ばず `On06DoorsLock` だけを呼んだ（前者はナース 2 体を出し、5 秒で捕まって死ぬ）。どちらも見せたい 1 つの出来事だけを残す staging で、ゲームの作りは変えていない。
- 2026-09-23（ステップ 3）: 敵は `note_gif.stand()` で置いても**自分からプレイヤーへ走って捕まえる**ので、置いてから撮り始めるまでを短くし、`use_when_near` で先に気絶させる。撮り終えた 23 秒の後は捕まってレベルが読み直され、隠した餅も置いた敵も消える（次の収録は PIE を始め直して組み直す）。

## 要確認（ユーザー）

1. **画面のボタンのキー操作の見た目と上下キー**（2026-09-23、ステップ 2 で分かった） — パッケージ版で Tab を押すと UMG の既定の焦点の枠（青い矩形）が出るだけで、押せる項目の色や大きさは変わらない（マウスで重ねたときの見た目とも違う）。タイトルの上下キーは 1 回で 2 つ飛ぶので（RESUME → EXTRAS → QUIT）、NEW GAME には Tab でしか行けない。本家はコントローラーの選択の見た目を持つが、本作は原作の UMG の式を写しただけでキーの選択を作っていない。**このままでよいか**（そのままなら何もしない。直すなら焦点の見た目と上下キーの 1 つずつの移動を足す）。

## 再開時の注意

- **エディタ**: `L_Hospital_Zone1` を開いたまま、PIE は止めてある。窓は GIF の矩形 `(1819, 68, 3279, 896)`（ビューポート 1039 × 652）。`observations/tools/check_viewport.py` は**観察用の別の矩形**（高さ 1269）を見るので、GIF を撮るときは使わない。
- **対話デスクトップの代理人**: 生きている（`python Tools/desktop.py ping`）。**キーやクリックを送る前に `python observations/tools/raise_editor.py` でエディタを前面に出す**（`raise_editor.resize()` は前面に出さないので、`PermissionError: the foreground window is WindowsTerminal.exe` で弾かれる）。収録だけなら前面でなくてよい（端末の窓は撮る矩形に重ならない）。
- **タブレットの枠**: `WasamiPowerComponent::CyclePower` はタブレットが出ていないと何もしないので、Python から呼んでも変わらない。Space → `1` を 3 回 → Space のキーで左の枠を Primal Fear にする（順は Speed Boost → Teleport → Telepathy → Primal Fear → Telekinesis → Vanish）。
- **収録の台本**: `tmp/primal_shot.py`（PIE を始め直し、枠を回し、置き、`tmp/primal_stage.py` で舞台を作り、23 秒撮って 0.8 秒後に `tmp/primal_go.py`）。12 の台本はガイドの「2026-09-23 の撮り方」に書いた。
- **時間のかかる処理**: PIE の収録（1 本 10〜25 秒）、エディタの開き直し。
- note の CLI は `tmp/note-cli/`、セッションの値は `Tools/note.local.json`（存在する）。記事は id 180735989・key n38f5d6ef4565、更新は `--no-notify`。

## 検証

- check_records: ステップ 2 の終わりに `--update` を通した
- C++ ビルド: 不要（この項目はコードを変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 3 の 2 本を PIE で撮り、コマの一覧（`fps` を落としたグリッド）で中身を確かめた。ステップ 4 の PIE はこれから
