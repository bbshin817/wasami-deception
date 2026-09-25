---
title: 項目 53〜57 大目標 5（パッケージ版の再指摘 5 件）
status: 進行中
branch: main
base: 8621beb
started: 2026-09-26 01:15
updated: 2026-09-26 01:40
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
- [ ] 1. 項目 57（黄色い文字）と項目 56（罠で死んだときの引き継ぎ）← 作業中
  - 変更予定: `Config/DefaultEngine.ini`、`Source/wasami_deception/WasamiGameMode.cpp`・`WasamiGameInstance.{h,cpp}`・`Tests/WasamiGameFlowTests.cpp`
- [ ] 2. 項目 55（トンネルのシャドウプレーン）
  - 変更予定: `Tools/dd/prepare_stage.py`、`Content/Python/wasami_tools/pipeline/dd_stage.py`・`paths.py`、`/Game/DD/Materials`、`L_Hospital_Zone1`
- [ ] 3. 項目 53（救急車）
  - 変更予定: `Source/wasami_deception/WasamiZone1Flow.{h,cpp}`、`L_Hospital_Zone1`
- [ ] 4. 項目 54（Zone 2 の場面の演技）
  - 変更予定: `.claude/references/enemy-wasami-motions.md`、`Content/Python/wasami_tools/pipeline/dd_sequence.py`、`L_Hospital_Zone2`
- [ ] 5. パッケージを作り直して 5 件を通しで確かめる（人が遊ぶ形。`Tools/desktop.py` でキーとマウスを送る）

## 次にやること

ステップ 1。まず `Config/DefaultEngine.ini` の `[SystemSettings]` に `r.Shadow.Virtual.AllowScreenOverflowMessages=0` を理由付きで足す。次に `AWasamiGameMode::DeathEvent` の入口で、持ち越しがまだ無いときだけ `UWasamiGameInstance::RememberPlayerState` を呼ぶようにし（`HasCarriedPlayerState()` を足す）、`Wasami.GameFlow.*` にテストを足して `python Tools/editor_cycle.py` でビルドする。

## 決定事項

- 2026-09-26: **パッケージは直すたびに作り直してから見る** — 2026-09-23 のパッケージ（04:00）は Zone 1 の `.umap` の保存（05:31）より前で、`dd_sequence` の Movable の直しが入っていない見込み。項目 41 の「2 台は再現しない」という結論はこのパッケージで出したものなので当てにしない。
- 2026-09-26: **確かめを台本（`-ExecCmds`）だけで閉じない** — 項目 38・41・42 はどれも PIE か `-ExecCmds` の台本で閉じたのに、人が遊ぶと出た。大目標 5 は `Tools/desktop.py` で実際にキーとマウスを送って通す。

## 要確認（ユーザー）

- 2026-09-26: **配布するパッケージの構成**（項目 57 の完了の条件 (3)） — 仮に `Development` のままで、画面メッセージだけを `[SystemSettings]` で止める。理由: `Shipping` にするとエンジンの画面メッセージが根こそぎ消えて確実だが、`Wasami.*` のコンソールコマンドも消えるので、今の確かめの道具（`Tools/game_flow.py`・`game_perf.py`・各 probe）が全部使えなくなる。場所: `Config/DefaultEngine.ini` の `[SystemSettings]`、`.claude/guides/distribution.md`。

## 再開時の注意

- エディタは閉じている（2026-09-26 01:40 時点）。C++ を直したら `python Tools/editor_cycle.py`（閉じる → ビルド → 開き直す）。
- パッケージの作り直しは `.claude/guides/distribution.md` の `RunUAT.bat BuildCookRun`。**エディタを閉じてから**、`run_in_background` で走らせて必ず結果を読む（10 分ほど）。
- 2026-09-26 の調べで撮った絵: `Intermediate/DesktopAgent/shots/shot-012307.png`（黄色い文字）・`shot-012316.png`（トンネルの奥が明るい）。どちらも直す前の姿。
- パッケージ版を起動する台本は `Intermediate/Overnight/probe_ambulance.py`（救急車）と `Tools/game_flow.py run`（通し）。二重起動の錠は `Intermediate/Perf/.game_perf.lock`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
