---
title: 病院の落書き・壁画・ポスター・動画のワサミ版をゲームへ入れる
status: ユーザー待ち
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
- [ ] 2b. 29 枚を作り直す ← 作業中（ユーザーの指示: 「生成AIらしい画風になっています。オリジナルの顔に忠実に、表記は英語で、作風は元の本家アセットになぞらえて」）。依頼書 `Tools/wasami_art/briefs/hospital_graffiti.json`・`hospital_posters.json` を書き直し、本家の絵を `extra_refs` に渡して生成 → 採用 → 取り込み直し（ステップ 2 の `swap.py` と同じ: `dd_stage.import_texture` と `make_material`）
- [ ] 3. 救急車の案内の画面（`Ambulance_Tutorial_MediaPlayer_Video_Mat`。Zone 1 に 6・Zone 2 に 12）に `ambulance_tutorial2.mp4` を流す。本家の Zone 1・2 で画面に映るのはこの 1 本だけ（BrainScan・Lungs・BrokenHeart はレベル BP が開くが、映す材質を置いた物が無い）
- [ ] 4. 記録（実装記録 01・original-fidelity の表・wasami-art・作業一覧）、パッケージの中身の確かめ、コミット

## 次にやること

ステップ 2b: 依頼書 2 つは書き直し済み（本家の絵は `Intermediate/WasamiArt/_orig/<名前>.png`。無ければ stage_ue.json の元の png を落書きは (58,52,48)・ポスターは白に重ねて書き出す）。**Codex の OAuth が期限切れ（401 token_expired）でユーザーの `codex login` 待ち**。ログイン後: `python Tools/wasami_art/wasami_art.py gen Tools/wasami_art/briefs/hospital_posters.json Tools/wasami_art/briefs/hospital_graffiti.json --jobs 6` → `review` で見る → `adopt` → 取り込み直し。その後ステップ 3。動画 5 本も同じ指示で作り直すかは 2b の後に決める。

## 決定事項

- 2026-09-26: 動画は救急車の案内 1 本だけ入れる — 本家の両ゾーンで映るのがそれだけのため（`_levels/06_Hospital_Zone_0*.full.json` に他の `_Video_Mat` の参照が無い）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

（なし）

## 検証

- check_records: 未実行
- エディタでの確認: 未実行
