---
title: 項目 53〜57 大目標 5（パッケージ版の再指摘 5 件）
status: 進行中
branch: main
base: 8621beb
started: 2026-09-26 01:15
updated: 2026-09-26 03:40
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
- [ ] 5. 項目 57 の残り ← 作業中。**(4) 済み**（`Tools/game_flow.py run` が 18 節目すべて `OK`・クラッシュなしで通り、19 枚とも黄色い画素 0 個）。**(2) だけ残り**: あふれ自体を減らすかを `Tools/game_perf.py` の fps と VRAM で決める。
- [ ] 5b. 項目 58（note の GIF 12・14 を撮り直す。トンネルの奥の見た目が変わったため）。**作り直したパッケージで項目 59 の (3)（トンネルの y ≈ −16100 の境目が消えたこと）も見て、59 を完了にする**（59 の (1)(2) は 2026-09-26 の有人セッションで済み。Nanite から外したトンネルの天井が影を落とす）

## 次にやること

ステップ 5 の残り（項目 57 の完了の条件 (2)）。`Tools/game_perf.py` で本編の fps と VRAM を測り、`r.Shadow.Virtual.NonNaniteVSMMaxPageAreaCoverage`（既定 0.10）を上げたときと比べて、6 GB に収まる範囲で上げるか、上げずに閉じるかを決める（**上げずに閉じる判断もあり得る**: 画面の字はもう出ず、fps は 00 記録の計測の範囲）。決めたら項目 57 を完了にし、ステップ 5b（項目 58: note の GIF 12・14 の撮り直し）へ。この記録を閉じたら、無人運転が作業一覧の項目 54 から自分の記録を作って始める。

## 決定事項

- 2026-09-26: **項目 53 の「2 台」と「置いていかれる」は、レベルの側はもう直っている** — エディタで `L_Hospital_Zone1` のアクタを読むと、走り出す `hospital_ambulance_new_teleport` とそれに付く 6 つの `BlockingVolume_Ambulance_*` はすべて `Movable`。土台の移動（`UpdateBasedMovement`）は土台が `Movable` のときだけ働くので、これで運ばれるはず。2026-09-23 のパッケージが古かっただけの可能性が高い。作り直したパッケージで再現しなければ項目 53 は閉じる。再現したら屋根に乗っている間プレイヤーを救急車に付ける。
- 2026-09-26: **配布は `Development` と `Shipping` を両方作る**（ユーザーの回答）— `Development` は Claude の確かめ用（`Wasami.*` が要る）、`Shipping` は人に渡す用。`.claude/guides/distribution.md` に書いた。
- 2026-09-26: **項目 54 は無人運転に渡す**（ユーザーの回答）。**この記録の計画には入れない**: この記録を閉じた後、無人運転が作業一覧の項目 54 から自分の進捗記録を作って始める（調べた中身は作業一覧の項目 54 に書いてある: Matron の `Detected` が手を上げたまま止まる／シネカメラの LookAt が本家の回転キーに勝っている）。
- 2026-09-26: **パッケージ版にはキーボードの入力が届かない**（`Tools/desktop.py` で `space` も `hold w` も効かず、プレイヤーが 1 cm も動かない。窓は前面で `"ok": true` は返る）。原因は不明で、症状索引に書いた。確かめはコンソールコマンドで組み、キーでしかできないことは `Wasami.*` を足す（`Wasami.Tablet` を足した）。
- 2026-09-26: **画面なしのテストは粒子と音の 6 本が落ちる** — `-NullRHI -NoSound` の制約で、直しのせいではない（症状索引に書いた）。粒子と音を見るテストはエディタを前面にして回す。

- 2026-09-26: **パッケージは直すたびに作り直してから見る** — 2026-09-23 のパッケージ（04:00）は Zone 1 の `.umap` の保存（05:31）より前で、`dd_sequence` の Movable の直しが入っていない見込み。項目 41 の「2 台は再現しない」という結論はこのパッケージで出したものなので当てにしない。
- 2026-09-26: **確かめを台本（`-ExecCmds`）だけで閉じない** — 項目 38・41・42 はどれも PIE か `-ExecCmds` の台本で閉じたのに、人が遊ぶと出た。大目標 5 は `Tools/desktop.py` で実際にキーとマウスを送って通す。

## 要確認（ユーザー）

- 2026-09-26: **配布するパッケージの構成**（項目 57 の完了の条件 (3)） — 仮に `Development` のままで、画面メッセージだけを `[SystemSettings]` で止める。理由: `Shipping` にするとエンジンの画面メッセージが根こそぎ消えて確実だが、`Wasami.*` のコンソールコマンドも消えるので、今の確かめの道具（`Tools/game_flow.py`・`game_perf.py`・各 probe）が全部使えなくなる。場所: `Config/DefaultEngine.ini` の `[SystemSettings]`、`.claude/guides/distribution.md`。

## 再開時の注意

- エディタは閉じている（2026-09-26 01:40 時点）。C++ を直したら `python Tools/editor_cycle.py`（閉じる → ビルド → 開き直す）。
- パッケージの作り直しは `.claude/guides/distribution.md` の `RunUAT.bat BuildCookRun`。**エディタを閉じてから**、`run_in_background` で走らせて必ず結果を読む（10 分ほど）。
- 2026-09-26 の調べで撮った絵: `Intermediate/DesktopAgent/shots/shot-012307.png`（黄色い文字）・`shot-012316.png`（トンネルの奥が明るい）。どちらも直す前の姿。
- パッケージ版を起動する台本は `Intermediate/Overnight/probe_ambulance.py`（救急車。`--fps N` を足した）と `Tools/game_flow.py run`（通し）。二重起動の錠は `Intermediate/Perf/.game_perf.lock`。
- 項目 56 の確かめの台本はこのセッションの scratchpad に置いた（`pkg_carry.py`）。**セッションが変わると消える**ので、続きが要るなら作り直す: `-ExecCmds` に `Wasami.ResetSave` → `open L_Hospital_Zone1` → `Wasami.Tablet` → `Wasami.Kill` → 復帰の後に `Wasami.Status` を `Wasami.Delay` で並べ、ログの `tablet=` を読むだけ。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
