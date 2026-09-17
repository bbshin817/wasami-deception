---
title: ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光。作業一覧の項目 3）
status: 進行中
branch: feature/shard-glow
base: c8aaf7a
started: 2026-09-17 22:06
updated: 2026-09-17 22:54
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
- [ ] 4. 閃光の見え方の検証と値の詰め ← 次
  - ステップ 1 と同じ場所・撮り方で新しい閃光を撮り（旧版は Python で `P_ky_flash3` を同じ位置・拡縮 0.2 で出して並べて撮れる）、色相が紫で、最大輝度が旧版より低いことを測る。紫に見えない・弱すぎる・強すぎるなら係数と色を直して撮り直す（2 回まで）。本作の画面の比べる画像を `Tools/discord_notify.py image` で送る（本家の絵は含めない）。
  - 変更予定: `dd_shards.py`（値）、`/Game/Wasami/Shard/P_WasamiShardFlash`、`observations/README.md`、実装記録 06
- [ ] 5. 仕上げ
  - 作業一覧の項目 3 を「完了（日付）」に、`handover.md` の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（会話でセッションの値を渡されていないので「note へは未反映」）、症状索引（書くことがあれば）。要確認を作業一覧の「未回答の要確認」へ移す。記録を消すコミットをして main へマージし、ブランチを消して push する（下の「再開時の注意」のマージの形）。

## 次にやること

ステップ 4: ステップ 1 と同じ場所・撮り方で `P_WasamiShardFlash`（シャードの回収）と旧版の `P_ky_flash3`（Python で同じ位置・拡縮 0.2 で出す）を撮り、`series --stat peak` で色相が紫・最大輝度が旧版より低いことを測る。合わなければ `dd_shards.FLASH_STRENGTH`（と色）を直して `make_flash()` を回し、撮り直す（2 回まで）。

## 決定事項

- 2026-09-17: 閃光は原作のパスの `P_ky_flash3` を変えず、本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash` を作ってシャードからだけ使う — `/Game/DD` は原作データから作り直す場所（取り込み直しで上書きされる）で、原作の値を保つため。新旧を同じ PIE で並べて測れる。
- 2026-09-17: 本家の紫の灯（`LightIntensity` 175・半径 200・(194, 0, 255)）は変えない（作業一覧の完了の条件）。明滅と閃光の紫は、仮にこの灯の色 (194, 0, 255) の色相に合わせる。
- 2026-09-17: 閃光の色と係数は仮の値（本家に無い本作独自の見た目。決め方の階段の 4）。いまは `dd_shards.FLASH_STRENGTH` 0.6（`TODO(仮)` 付き）・色は `FLASH_COLOR`（餅の明滅と同じ紫）。ステップ 4 の PIE で決め、決めた値を「要確認」に 1 行で書く。値を変えたら `python Tools/ue_remote.py` で `importlib.reload(dd_particles)`・`reload(dd_shards)` → `dd_shards.make_flash()`（C++ のビルドは要らない）。

## 要確認（ユーザー）

- 餅の紫の明滅の強さと周期（仮）: `PulseStrength` 1.0（4.4 m 先で餅の平均の赤と青が 117 → 202、白飛びなし）・`PulsePeriod` 2.0 s。色は本家の灯と同じ紫。明滅はテクスチャに掛けるので顔の模様は残る。変えるなら `dd_shards.py` の `MOCHI_PULSE_*`。

## 再開時の注意

- エディタ: 2026-09-17 22:53 の時点で `L_Hospital_Zone1` が開いていて、PIE なし、保存していないアセットなし。C++ はステップ 3 の分までビルド済み。エディタを開き直した後は MCP が切れているので、テストはリモート実行で `Automation RunTests Wasami.Shard` を送り、ログの `Test Completed` を見る（エディタが背面だと 3 fps で進まない。ターミナルが前面なら、撮った画面で位置を確かめてから `desktop.py click 2957 95 --allow WindowsTerminal.exe --allow UnrealEditor.exe`）。
- 撮り方（ステップ 4）: `pie.py start` → `cmd "t.MaxFPS 60"` → `place 0 -157 --yaw -90`（餅 330 が 4.4 m 先）→ `desktop.py click 2600 500 --allow UnrealEditor.exe` → `record --grab gdi --region 1826 204 2978 858`。回収は `python Tools/ue_remote.py -c "$(cat observations/tools/shard_glow/collect.py; echo; echo "main('BP_Shard4')")"`（`place 0 -757` で 4、`place 0 -1360` で _2 が 4.4 m 先）。**エディタのセッションで最初の閃光は粒子が描かれない**ので、先に 1 つ捨てで回収する。閃光の箱は `core=548,412,608,472`・`flash=498,362,658,522`、測るのは `series --stat peak`。終わったら `t.MaxFPS 0`・`pie.py stop`・`desktop.py stop`。
- main へのマージ（ステップ 5）: Claude の一時フォルダの worktree `scratchpad/main-wt` が main を開いたまま残っている（`git worktree list`。作業一覧の「未回答の要確認」）。残っていれば `git checkout main` が失敗するので、症状索引の「`git checkout main` が … already checked out」の手順（`commit-tree` → `update-ref` → `checkout --ignore-other-worktrees main`）でマージする。消えていれば普通の `git checkout main` と `git merge --no-ff`。

## 検証

- check_records: ステップ 3 で通した（01・06 のハッシュを更新）
- C++ ビルド: ステップ 3 で通した。`Wasami.Shard` の 2 件が成功（起動時のログの `Condition failed` 19 行はエンジンの自己テストで、前のログにも同じ数ある）
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 1 で基準、ステップ 2 で明滅を撮った（上の撮り方）
