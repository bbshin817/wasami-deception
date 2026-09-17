---
title: ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光。作業一覧の項目 3）
status: 進行中
branch: feature/shard-glow
base: c8aaf7a
started: 2026-09-17 22:06
updated: 2026-09-17 23:35
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光）

## 依頼

最終目標（2026-09-17、ユーザーの原文）の「ワサミシャードは、紫色のやや弱めな閃光を放つようにする。」。ユーザーの回答: 「両方。置かれている間は本家の紫の灯に加えて餅が紫に明滅し、回収時の閃光（`P_ky_flash3`）は紫でやや弱く」。

作業一覧 `.claude/roadmap.md` の項目 3。完了の条件: PIE の収録で、餅の発光が周期的に強弱し（周期・強さは仮でよい。要確認に書く）、回収の閃光の色相が紫で、画面の最大輝度が今の `P_ky_flash3` より低い。本家の紫の灯（175、半径 200、(194, 0, 255)）は変えない。

根拠: 実装記録 06（`AWasamiShard`、`M_DD_WasamiMochi`、`P_ky_flash3` の推定の材質とエミッタ 7 つの値）。本家に無い本作独自の見た目で、WebGL 版にも明滅・紫の閃光は無い（WebGL 版 08 記録: 閃光は `P_ky_flash3` の写し、浮き沈みは本作の表現）。色と強さは仮の値で進めてよい（`.claude/guides/autonomy.md` の線引き）。

## 計画

- [x] 1. 測る道具と基準の収録 — `video_probe.py series --stat peak` を足し、4.4 m 先の餅と `P_ky_flash3` を PIE で撮った（値は `observations/README.md` の「シャードの光の基準」。閃光の `core` は最大 247・p99 244、ほぼ白）
- [x] 2. 餅の紫の明滅 — `M_DD_WasamiMochi` の自己発光 = ベースカラー × (Glow + 紫 × 強さ × 波)、位相は `BeginPlay` でカスタム プリミティブ データ 0 に入れる乱数（C++・テスト）。PIE で周期 2.0 s・白飛びなし・個体ごとの位相を確かめた（`observations/README.md` の「餅の紫の明滅」、実装記録 06）
- [x] 3. 回収の閃光の本作の版 — `dd_particles.particle_system(rel, version, target, adjust)`、`dd_shards.make_flash()` が `P_ky_flash3` の 7 つの色の表を紫 (0.539, 0, 1) × 各色の最大 × `FLASH_STRENGTH` 0.6 にした `/Game/Wasami/Shard/P_WasamiShardFlash` を作り（灯の明るさ 2.5 は変えない。理由は実装記録 06「本作の回収の閃光」）、`CollectFlash` の既定のパスとテストを直した。ビルドと `Wasami.Shard` 2 件が成功
- [x] 4. 閃光の見え方の検証と値の詰め — 色の係数を「最大 ^ `FLASH_GAMMA` 0.5 × `FLASH_STRENGTH` 0.8」にした（線形 0.6 は輪と円が消え、^0.5 × 1.34 は紫のもやが旧版より強かった）。0.25 倍速で `core` の最大 170・p99 164（旧版 247・244）、色相 299°、輪の広がりは旧版とほぼ同じ（`observations/README.md` の「紫の回収の閃光」、理由は実装記録 06）。比べる画像を Discord へ送った
- [ ] 5. 仕上げ ← 次
  - 作業一覧の項目 3 を「完了（日付）」に、`handover.md` の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（会話でセッションの値を渡されていないので「note へは未反映」）、症状索引（書くことがあれば）。要確認を作業一覧の「未回答の要確認」へ移す。記録を消すコミットをして main へマージし、ブランチを消して push する（下の「再開時の注意」のマージの形）。

## 次にやること

ステップ 5（仕上げ）: 作業一覧の項目 3 を完了にし、要確認 2 件を作業一覧の「未回答の要確認」へ移す。`handover.md` の「現状と次の一歩」と note の原稿 `docs/note/progress.md` を直す（note へは未反映）。記録を消すコミットをして main へマージし（下の「再開時の注意」）、ブランチを消して push する。

## 決定事項

- 2026-09-17: 閃光は原作のパスの `P_ky_flash3` を変えず、本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash` を作ってシャードからだけ使う（理由は実装記録 06）。
- 2026-09-17: 本家の紫の灯（`LightIntensity` 175・半径 200・(194, 0, 255)）は変えない（作業一覧の完了の条件）。明滅と閃光の紫は、仮にこの灯の色相に合わせた。

## 要確認（ユーザー）

- 餅の紫の明滅の強さと周期（仮）: `PulseStrength` 1.0（4.4 m 先で餅の平均の赤と青が 117 → 202、白飛びなし）・`PulsePeriod` 2.0 s。色は本家の灯と同じ紫。明滅はテクスチャに掛けるので顔の模様は残る。変えるなら `dd_shards.py` の `MOCHI_PULSE_*`。
- 回収の閃光の色と強さ（仮）: 原作の `P_ky_flash3` の色を、灯と同じ紫 × (各色の最大 ^ 0.5) × 0.8 にした。0.25 倍速・4.4 m 先で、中心の星は輝度 247（白）→ 170（紫、色相 299°）、紫の円と衝撃波の輪は原作と同じ広がりでやや淡い。虹の円は紫の濃淡になる。変えるなら `dd_shards.py` の `FLASH_GAMMA`・`FLASH_STRENGTH`・`FLASH_COLOR`（`make_flash()` を回すだけでよい）。

## 再開時の注意

- エディタ: 2026-09-17 23:35 の時点で `L_Hospital_Zone1` が開いていて、PIE なし、保存していないアセットなし。`P_WasamiShardFlash` はステップ 4 の値（^0.5 × 0.8）で組み直して保存済み。
- main へのマージ（ステップ 5）: Claude の一時フォルダの worktree `scratchpad/main-wt` が main を開いたまま残っている（`git worktree list`。作業一覧の「未回答の要確認」）。残っていれば `git checkout main` が失敗するので、症状索引の「`git checkout main` が … already checked out」の手順（`commit-tree` → `update-ref` → `checkout --ignore-other-worktrees main`）でマージする。消えていれば普通の `git checkout main` と `git merge --no-ff`。

## 検証

- check_records: ステップ 4 で通した（06 のハッシュを更新）
- C++ ビルド: ステップ 3 で通した。`Wasami.Shard` の 2 件が成功。ステップ 4 は Python と粒子のアセットだけ
- エディタでの確認（PIE）: ステップ 1 で基準、ステップ 2 で明滅、ステップ 4 で紫の閃光を撮った（`observations/README.md`）
