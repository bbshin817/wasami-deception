---
title: Shipping で敵ワサミが追わず直立する（マップの道が空で保存された）
status: ユーザー待ち
branch: main
base: 433168f
started: 2026-09-27 01:30
updated: 2026-09-27 02:00
---

# Shipping で敵ワサミが追わず直立する

## 依頼

2026-09-27 の有人セッション: 「敵ワサミが全く追いかけず直立不動になるバグが shipping にあります」

## 計画

- [x] 1. 原因 … 2026-09-27 完了。項目 63（救急車のヘッドライト）の直しのスクリプトが 2026-09-26 23:55 に Zone 1・Zone 2 を開いたのと同じ呼び出し（エディタのログのフレーム [392]）で保存し、空の道が保存された（症状索引「PIE で敵が動かない…（レベルに道が保存されていない）」）。00:39 の Release 用 Shipping はこのマップをクックした。23:00 の Development・Shipping は影響なし。
- [x] 2. 両マップの道を焼き直して保存 … 2026-09-27 完了（Zone 1 2/2・Zone 2 29/29。5.40→6.17 MB、4.94→5.68 MB）
- [x] 3. 再発防止 … 2026-09-27 完了。`Wasami.Status` に `nav=`（道のあるボリューム/全部）、`game_flow.py` の迷路の 2 節目で確かめる。配布ガイド・症状索引・06/01 記録を直し、C++ はビルド済み
- [x] 4. Development で `game_flow.py run` が通り（nav Zone 1 2/2・Zone 2 29/29）、Release 用 Shipping（0 エラー）と zip を作り直した … 2026-09-27 01:55
- [ ] 5. ユーザーが zip の Shipping で敵が追うのを確かめる ← 次

## 次にやること

ユーザーが `Saved/Archive/Release/WasamiDeception-Windows-x64.zip`（2026-09-27 01:55）の Shipping で敵が追いかけるのを確かめるのを待つ。問題が無ければこの記録を閉じる。

## 決定事項

（なし）

## 要確認（ユーザー）

（なし）

## 再開時の注意

（なし）

## 検証

- check_records: OK
- C++ ビルド: エディタ・Development・Shipping とも成功
- パッケージ: `game_flow.py run` 19 節目すべて OK・落ちなし
