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
- [x] 3b. 救急車の案内の画面に動画を流す … 2026-09-26 完了。`dd_movies`・`AWasamiZoneFlow` の `ScreenPlayer`（実装記録 01・11）。PIE の Zone 1 で確認。
- [ ] 4. ← 作業中 記録（実装記録 01・original-fidelity の表・wasami-art・作業一覧）、パッケージの中身の確かめ、コミット

## 次にやること

ステップ 4: パッケージを作り直し（`distribution.md` の手順。エディタを閉じる）、`Saved/Archive/Windows/wasami_deception/Content/Movies/ambulance_tutorial2.mp4` があることと、Zone 1 の画面に流れることを確かめる。original-fidelity の表と作業一覧に書いて記録を閉じる。

## 決定事項

- 2026-09-26: 動画は救急車の案内 1 本だけ入れる — 本家の両ゾーンで映るのがそれだけのため（`_levels/06_Hospital_Zone_0*.full.json` に他の `_Video_Mat` の参照が無い）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

（なし）

## 検証

- check_records: 未実行
- エディタでの確認: 未実行
