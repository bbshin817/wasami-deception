---
title: 画面操作（対話デスクトップの入力とスクリーンショット）を Claude からできるようにする
status: 進行中
branch: main
base: dca8e00
started: 2026-09-16 13:20
updated: 2026-09-16 13:20
---

# 画面操作（対話デスクトップの入力とスクリーンショット）を Claude からできるようにする

## 依頼

ユーザーの指示（2026-09-16）: 「Windows Computer Use を使い、画面操作もあなたへ依頼したいです。」

- これまでの規則（`.claude/guides/verification.md` の「OS 全体の入力を操作しない」）を、ユーザーの指示で変える。
- 直近の用途は、起動している本家の最新版で REPLAY から Torment Therapy（病院）を出し、画面を撮って本作と比べること。

## 前提（仕組みの制約）

- Claude は Windows の**セッション 0**にいる。セッション 1（ユーザーのデスクトップ）へは**入力も画面取得も届かない**（セッションの分離。エディタやゲームを直接起動できないのと同じ理由）。
- したがって、**セッション 1 で常駐する操作エージェント**を置き、セッション 0 の Claude はファイル経由で指示を出す。エージェントの起動は `Tools/console_session.py`（一度きりのスケジュールタスク）で行い、`pythonw.exe` を使ってコンソール窓を出さない。
- 使える道具は確認済み: Python 3.10.11（`C:\Users\User\AppData\Local\Programs\Python\Python310`）、PIL・numpy・pywin32、`pythonw.exe`。

## 計画

- [ ] 1. `Tools/desktop_agent.py`（セッション 1 常駐。spool の JSON を読んで入力・撮影を実行し、結果と PNG を書く） ← 作業中
- [ ] 2. `Tools/desktop.py`（セッション 0 の Claude が使うクライアント。`start` / `ping` / `shot` / `click` / `key` / `hold` / `look` / `stop`）
- [ ] 3. 動作確認（`ping` → `shot` で今の画面 → 前面ウィンドウの判定 → ゲームのメニュー操作）
- [ ] 4. 本家の最新版で REPLAY → Torment Therapy を出し、病院 Zone 1 とタブレットの画面を撮る
- [ ] 5. ルールの更新（`verification.md` の入力の規則を書き換え、`original-fidelity.md` の観察の手順に画面操作を入れる、`CLAUDE.md` の索引）＋ 実装記録 01 に sources を足して `check_records`
- [ ] 6. コミットと push

## 次にやること

`Tools/desktop_agent.py` を書く。spool は `Intermediate/DesktopAgent/`（git の対象外）。

## 決定事項

- 2026-09-16: 入力は**前面のウィンドウが許可した対象のときだけ**送る（既定は本家のゲーム `DDeception-Win64-Shipping.exe` / `DDeception.exe`）。エディタや PIE を操作するときは、その都度ユーザーの確認を取ってから許可の一覧に足す。ユーザーの他のアプリに入力が飛ばないようにするため。
- 2026-09-16: OS 全体に効く組み合わせ（Win キー、Alt+Tab、Alt+F4、Ctrl+Alt+Del）は送らない。エージェントは何もしないまま一定時間が過ぎたら自分で終了する（出しっぱなしにしない）。
- 2026-09-16: 入力は SendInput のスキャンコード（ゲームは RawInput でスキャンコードを読むため）、視点移動は相対のマウス移動で送る。

## 再開時の注意

- エージェントが動いているかは `Intermediate/DesktopAgent/agent.pid` と `tasklist`（`pythonw.exe`）で分かる。止めるのは `python Tools/desktop.py stop`。
- いま起動しているもの: 本家の最新版（`DDeception-Win64-Shipping.exe`、セッション 1、REPLAY 前のタイトル画面のはず）、Unreal Editor（`L_Hospital_Zone1`、未保存 0 件）、Steam。
- 画面操作の最中はユーザーがマウス・キーボードを触らない前提。触る必要が出たら先に `stop` する。

## 検証

- check_records: 未実行
- エージェントの動作確認: 未実行
- ゲームでの確認: 未実行
