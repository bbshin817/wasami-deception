---
title: 病院の落書き・壁画・ポスター・動画のワサミ版をゲームへ入れる
status: 進行中
branch: main
base: ef5db06
started: 2026-09-26 13:00
updated: 2026-09-26 13:20
---

# 病院の落書き・壁画・ポスター・動画のワサミ版をゲームへ入れる

## 依頼

2026-09-26 の有人セッション。「ゲーム内のアセットは正しくワサミ版として作られた？」→ 生成済みの `SourceArt/Wasami/Stage`（29 枚）・`SourceArt/Wasami/Movies`（5 本）がゲームへ入っていないと報告 → 「このセッションで修正してください」。

## 計画

- [x] 1. ポスター 11 枚に依頼書の `bg` を敷き直した … 2026-09-26 完了。候補から flatten → fit_size で採用し直し、`generated.json` の `bg` も直した（どれも RGB・不透明）。
- [x] 2. `prepare_stage.py` が `SourceArt/Wasami/Stage/<本家の名前>.png` を `/Game/Wasami/Stage/T_<名前>` として差し替える … 2026-09-26 完了。29 枚を取り込み材質 29 個を指し直し、本家の 29 枚は参照元 0。PIE の Zone 1 で 6 か所を確認。
- [x] 2b. 29 枚を作り直した … 2026-09-26 完了。本家の絵を画風の参照にし、顔は写真、字は英語。採用・取り込み直し・PIE で確認（実装記録 01）。
- [x] 3a. 救急車の案内の動画を作り直した … 2026-09-26 完了。`Tools/wasami_art/ambulance_video.py` で 8 s のループ（実装記録 01「救急車の案内の動画」）。
- [ ] 3b. ← 作業中 その動画をゲームの画面（`Ambulance_Tutorial_MediaPlayer_Video_Mat`。Zone 1 に 6・Zone 2 に 12）で流す。本家の Zone 1・2 で画面に映るのはこの 1 本だけ（ほかの 4 本はレベル BP が開くが映す物が無い）
- [ ] 4. 記録（実装記録 01・original-fidelity の表・wasami-art・作業一覧）、パッケージの中身の確かめ、コミット

## 次にやること

ステップ 3b: `FileMediaSource`（`Content/Movies/ambulance_tutorial2.mp4`。パッケージに入れるには `Content/Movies` に置き、`DirectoriesToAlwaysStageAsNonUFS` か UFS の設定を確かめる）・`MediaPlayer`（Loop）・`MediaTexture` を作り、`/Game/DD/Movies/Ambulance_Tutorial_MediaPlayer_Video_Mat` を MediaTexture を Emissive に出す材質にして、ゾーンの BeginPlay で OpenSource する。

## 決定事項

- 2026-09-26: 動画は救急車の案内 1 本だけ入れる — 本家の両ゾーンで映るのがそれだけのため（`_levels/06_Hospital_Zone_0*.full.json` に他の `_Video_Mat` の参照が無い）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

（なし）

## 検証

- check_records: 未実行
- エディタでの確認: 未実行
