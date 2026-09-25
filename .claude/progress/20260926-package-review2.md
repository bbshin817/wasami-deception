---
title: 項目 53〜57 大目標 5（パッケージ版の再指摘 5 件）
status: 進行中
branch: main
base: 8621beb
started: 2026-09-26 01:15
updated: 2026-09-26 08:30
---

# 項目 53〜57: パッケージ版の再指摘 5 件（大目標 5）

## 依頼

2026-09-26 の有人セッション。ユーザーが `Saved\Archive\Windows\wasami_deception.exe`（2026-09-23 04:00 のパッケージ）を本 PC の Windows で遊んでの指摘（原文）:

> 次の点が改善していません。
> 【問題点】
> ・Zone1->2へ移動する際、救急車に置いていかれる。進む救急車と、道の道中で静止している救急車の2台が存在する
> ・Zone2のボスワサミ->牢獄へのシーンで、謎に手を掲げているワサミ、振り返る際のモーションが壊滅的。ワサミが殴りかかるシーン、床に倒れ込むシーンについて、本家を踏襲できていない
> ・トンネルで、本家はトンネルの奥は暗闇が続くのに、本作はそうではない
> ・ライフ減少後の復帰時、死亡前にオンにしていたタブレット・Shiftダッシュがリセットされている
> ・画面上部に黄色い文字が表示される。おそらくUEのデバッグ用文字列で、配布要パッケージ版としては不適切
>
> 【検証環境】
> Saved\Archive\Windows\wasami_deception.exeを本Windows上で実行

作業一覧は `.claude/roadmap.md` の「大目標 5」（項目 53〜57）。

## 計画

- [x] 0. 原因の切り分けと記録 … 2026-09-26 完了。5 件とも原因まで特定し、作業一覧に大目標 5（項目 53〜57）を書き、項目 38・41・42 に再指摘の注記を入れた。切り分けの中身は各項目の「分かっていること」。
- [x] 1. 項目 57（黄色い文字）と項目 56（罠で死んだときの引き継ぎ） … 2026-09-26 完了（`493e93b`）。ini に `r.Shadow.Virtual.AllowScreenOverflowMessages=0`、`AWasamiGameMode::DeathEvent` が `RememberPlayerStateOnce` を呼ぶ。テストは `Wasami.GameFlow.Lives` に足して通った。
- [x] 2. 項目 55（トンネルのシャドウプレーン） … 2026-09-26 完了（`493e93b`）。8 つ目のマスター `M_DD_ShadowPlane` を cook のシェーダーの式から組み、PIE でトンネルの奥が暗く沈むのを見た。
- [x] 3. パッケージを作り直して 53・55・56・57 を確かめた … 2026-09-26 完了。黄色い文字は消え（黄色い画素 167/156 → 4 = 背景のばらつき）、トンネルの奥の灰色の板も消え、救急車は 60 fps でも 15 fps でも運ばれて 1 台しか見えず、罠の死から戻ったタブレットは `tablet=1` のまま。**項目 53 は直しを足さずに閉じた**（古いパッケージが原因だった）。パッケージ版にキーが届かないと分かったので `Wasami.Tablet` と `Wasami.Status` の `tablet=`・`sprint=`・`speed=` を足した。
- [x] 4. `Shipping` のパッケージも作った … 2026-09-26 完了。`Saved/Archive/Shipping/Windows/`（クック 0 エラー）。起動して全画面でタイトルが出て、黄色い字も無い。`.claude/guides/distribution.md` に手順を書いた。
- [x] 5. 項目 57 の残り（完了の条件 (2)）… 2026-09-26 完了。パッケージを作り直し（項目 59 の Nanite を外した後。0 エラー・1662 パッケージ）、VSM のあふれは**速さだけの注意で影は欠けない**こと・**`NonNaniteVSMMaxPageAreaCoverage` は UE 5.8 に無い**こと・測っても遅くなっていないこと（駐車場 cp6 62.0 fps、トンネル 62.1 fps、GPU メモリ 2702〜2731 MB、カードの山 3466〜3674 MB / 6144 MB）を確かめ、**何も上げずに項目 57 を閉じた**（根拠は実装記録 00 の「VSM のあふれの警告」と作業一覧の項目 57）。
- [x] 5b. 項目 59 の完了の条件 (3) … 2026-09-26 完了。パッケージ版のトンネルの 5 か所から奥を撮り、境目が無く一様に暗いことを見て**項目 59 を完了にした**（絵は `Intermediate/Tunnel/pkg-*.png`）。
- [ ] 5c. 項目 58（note の GIF `12-doors-busted.gif`・`14-ambulance-zone2.gif` をエディタの PIE で撮り直し、ほかの GIF にトンネルの奥が写っていないかも見て、`docs/note/progress.md` と note の記事を直す）

## 次にやること

ステップ 5c（項目 58: note の GIF `12-doors-busted.gif`・`14-ambulance-zone2.gif` をエディタの PIE で撮り直す）。
- 撮り方は `.claude/guides/note-progress.md` の「GIF」（窓を `(1819, 68, 3279, 896)` にしてから `Tools/desktop.py record`）。12 の台本は同じ節の「2026-09-23 の撮り方」にそのまま書いてある。
- **14（救急車で Zone 2 へ）の撮り方は記録が無い**ので組み直す: `L_Hospital_Zone1` の PIE で救急車の屋根に立たせ（`Wasami.Trigger TriggerBox_06_AmbulanceTop` か `Wasami.Flow On06ReachAmbulance`）、**1 s 後に走り出し、7 s 後に読み込み画面、その 2.5 s 後に Zone 2 が開く**（`AWasamiZone1Flow::On06ReachAmbulance`）。トンネルを走る数秒を切り出す。
- ほかの GIF にトンネルの奥が写っていないかも見る（13 のガレージ、23 の通し）。
- 撮り直したら `docs/note/progress.md`（文は変えない）と note の記事を直す（`tmp/note-cli`。セッションの値は `Tools/note.local.json` にある）。
この記録を閉じたら、無人運転が作業一覧の項目 54 から自分の記録を作って始める（大目標 5 の残りは 54 だけになる）。

## 決定事項

- 2026-09-26: **配布は `Development` と `Shipping` を両方作る**（ユーザーの回答）— `Development` は Claude の確かめ用（`Wasami.*` が要る）、`Shipping` は人に渡す用。`.claude/guides/distribution.md` に書いた。
- 2026-09-26: **項目 54 は無人運転に渡す**（ユーザーの回答）。**この記録の計画には入れない**: この記録を閉じた後、無人運転が作業一覧の項目 54 から自分の進捗記録を作って始める（調べた中身は作業一覧の項目 54 に書いてある: Matron の `Detected` が手を上げたまま止まる／シネカメラの LookAt が本家の回転キーに勝っている）。
- 2026-09-26: **パッケージ版にはキーボードの入力が届かない**（`Tools/desktop.py` で `space` も `hold w` も効かず、プレイヤーが 1 cm も動かない。窓は前面で `"ok": true` は返る）。原因は不明で、症状索引に書いた。確かめはコンソールコマンドで組み、キーでしかできないことは `Wasami.*` を足す（`Wasami.Tablet` を足した）。

- 2026-09-26: **確かめを台本（`-ExecCmds`）だけで閉じない** — 項目 38・41・42 はどれも PIE か `-ExecCmds` の台本で閉じたのに、人が遊ぶと出た。大目標 5 は `Tools/desktop.py` で実際にキーとマウスを送って通す。

## 要確認（ユーザー）

- （いまは無い。配布するパッケージの構成はユーザーが「`Development` と `Shipping` を両方作る」と答え、`.claude/guides/distribution.md` に書いた）

## 再開時の注意

- エディタは開き直してある（2026-09-26 08:3x。5b でパッケージを走らせるために一度閉じた）。閉じ直すときは `python Tools/editor_cycle.py --quit-only`、開き直しは `python Tools/editor_cycle.py`（`run_in_background` で走らせて必ず結果を読む）。
- **パッケージ版にはキーが届かない**ので、進める台本は `-ExecCmds` と `Wasami.*` で組む（`Tools/game_flow.py run`、fps は `Tools/game_perf.py`、任意の場所の絵は `Wasami.Delay <秒> BugItGo …` と `Wasami.Delay <秒> Shot`。二重起動の錠は `Intermediate/Perf/.game_perf.lock`）。
- GIF はパッケージではなく**エディタの PIE で撮る**（今までの 43 本と同じ作り方。`.claude/guides/note-progress.md`）。

## 検証

- check_records: OK（2026-09-26。実装記録 01 にパッケージ版での確かめを 1 行足した）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
