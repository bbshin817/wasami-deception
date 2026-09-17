---
title: ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光。作業一覧の項目 3）
status: 進行中
branch: feature/shard-glow
base: c8aaf7a
started: 2026-09-17 22:06
updated: 2026-09-17 22:40
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光）

## 依頼

最終目標（2026-09-17、ユーザーの原文）の「ワサミシャードは、紫色のやや弱めな閃光を放つようにする。」。ユーザーの回答: 「両方。置かれている間は本家の紫の灯に加えて餅が紫に明滅し、回収時の閃光（`P_ky_flash3`）は紫でやや弱く」。

作業一覧 `.claude/roadmap.md` の項目 3。完了の条件: PIE の収録で、餅の発光が周期的に強弱し（周期・強さは仮でよい。要確認に書く）、回収の閃光の色相が紫で、画面の最大輝度が今の `P_ky_flash3` より低い。本家の紫の灯（175、半径 200、(194, 0, 255)）は変えない。

根拠: 実装記録 06（`AWasamiShard`、`M_DD_WasamiMochi`、`P_ky_flash3` の推定の材質とエミッタ 7 つの値）。本家に無い本作独自の見た目で、WebGL 版にも明滅・紫の閃光は無い（WebGL 版 08 記録: 閃光は `P_ky_flash3` の写し、浮き沈みは本作の表現）。色と強さは仮の値で進めてよい（`.claude/guides/autonomy.md` の線引き）。

## 計画

- [x] 1. 測る道具と基準の収録 — `video_probe.py series --stat peak` を足し、4.4 m 先の餅と `P_ky_flash3` を PIE で撮った（値は `observations/README.md` の「シャードの光の基準」。閃光の `core` は最大 247・p99 244、ほぼ白）
- [ ] 2. 餅の紫の明滅 ← 次
  - `dd_shards._build_mochi` の自己発光に、`PulseColor` × `PulseStrength` × (0.5 + 0.5 sin(2π (Time / `PulsePeriod` + 位相))) を足す。位相はオブジェクトの位置から（`ObjectPositionWS` を frac に）。`MI_WasamiMochi` に仮の値。`dd_shards.import_mochi()` を走らせ直す（メッシュは Nanite。Nanite の材質で `Time` と `ObjectPositionWS` が効くかを確かめる）。
  - PIE で同じ場所（ステップ 1 の `pie-shard-mochi-a` と同じ置き方・範囲・箱 `mochi=555,422,612,472`）を撮り、`video_probe.py period` で周期、`series` で餅の箱の強弱を測る。強さは、明滅が分かり、白飛びしない値を画面で決める（仮）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_shards.py`、`/Game/Pipeline/Materials/M_DD_WasamiMochi`、`/Game/Wasami/Shard/MI_WasamiMochi`、実装記録 06
- [ ] 3. 回収の閃光の本作の版
  - `dd_particles.particle_system` に、出力先のパスと、書き出しの値を組む前に直す関数を渡せるようにする（組み立てと照合はそのまま使う）。
  - `dd_shards` で `/Game/Wasami/Shard/P_WasamiShardFlash` を `P_ky_flash3` の書き出しから作る: 白・青白の色（core (2.94, 2.25, 5.0)、glow (6.75, 6.56, 7) → 白、shockwave の白の始まり、dust_line の色〈書き出しで確かめる〉）を紫の色相へ、各エミッタの色と灯の明るさ（2.5）を仮の係数倍にする。`M_ky_polarGlow02` の虹のテクスチャは粒子の色を掛けるので、紫を掛ければ緑が消える（材質は原作のパスのものを共有し、変えない）。
  - `AWasamiShard::CollectFlash` の既定のパスを新しい版にし、`Tests/WasamiShardTests.cpp` の期待値を直す。C++ をビルド（`python Tools/editor_cycle.py`）し、`Wasami.Shard` のテストを通す。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_particles.py`、`dd_shards.py`、`Source/wasami_deception/WasamiShard.cpp`・`WasamiShard.h`、`Source/wasami_deception/Tests/WasamiShardTests.cpp`、`/Game/Wasami/Shard/P_WasamiShardFlash`、実装記録 06・01
- [ ] 4. 閃光の見え方の検証と値の詰め
  - ステップ 1 と同じ場所・撮り方で新しい閃光を撮り（旧版は Python で `P_ky_flash3` を同じ位置・拡縮 0.2 で出して並べて撮れる）、色相が紫で、最大輝度が旧版より低いことを測る。紫に見えない・弱すぎる・強すぎるなら係数と色を直して撮り直す（2 回まで）。本作の画面の比べる画像を `Tools/discord_notify.py image` で送る（本家の絵は含めない）。
  - 変更予定: `dd_shards.py`（値）、`/Game/Wasami/Shard/P_WasamiShardFlash`、`observations/README.md`、実装記録 06
- [ ] 5. 仕上げ
  - 作業一覧の項目 3 を「完了（日付）」に、`handover.md` の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（会話でセッションの値を渡されていないので「note へは未反映」）、症状索引（書くことがあれば）。要確認を作業一覧の「未回答の要確認」へ移す。記録を消すコミットをして main へマージし、ブランチを消して push する（下の「再開時の注意」のマージの形）。

## 次にやること

ステップ 2: `dd_shards._build_mochi` の自己発光に紫の明滅（`PulseColor` × `PulseStrength` × (0.5 + 0.5 sin(2π (Time / `PulsePeriod` + 位相)))、位相は `ObjectPositionWS` から）を足し、`dd_shards.import_mochi()` を走らせ直して、PIE で `pie-shard-mochi-b.mkv` を撮って測る。

## 決定事項

- 2026-09-17: 閃光は原作のパスの `P_ky_flash3` を変えず、本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash` を作ってシャードからだけ使う — `/Game/DD` は原作データから作り直す場所（取り込み直しで上書きされる）で、原作の値を保つため。新旧を同じ PIE で並べて測れる。
- 2026-09-17: 明滅は材質だけで行う（アクタのティックも動的マテリアルも使わない）— 両ゾーンで約 680 個のシャードの処理を増やさないため。個体ごとの位相はオブジェクトの位置から（本家の結晶も個体ごとに再生速度が乱数で、そろって光らない）。
- 2026-09-17: 本家の紫の灯（`LightIntensity` 175・半径 200・(194, 0, 255)）は変えない（作業一覧の完了の条件）。明滅と閃光の紫は、仮にこの灯の色 (194, 0, 255) の色相に合わせる。
- 2026-09-17: 明滅の周期・強さ、閃光の色と係数は仮の値（本家に無い本作独自の見た目。決め方の階段の 4）。候補は周期 2 s・閃光の係数 0.6 で、各ステップの PIE で決め、決めた値をそのステップで「要確認」に 1 行ずつ書き、コードに `TODO(仮)` を付ける。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: 2026-09-17 22:40 の時点で `L_Hospital_Zone1` が開いていて、PIE なし、保存していないマップなし。PIE でシャード 330・4・_2 を回収したが、PIE の中だけなのでレベルは変わっていない。
- 撮り方（ステップ 2・4）: `pie.py start` → `cmd "t.MaxFPS 60"` → `place 0 -157 --yaw -90`（餅 330 が 4.4 m 先）→ `desktop.py click 2600 500 --allow UnrealEditor.exe` → `record --grab gdi --region 1826 204 2978 858`。回収は `python Tools/ue_remote.py -c "$(cat observations/tools/shard_glow/collect.py; echo; echo "main('BP_Shard4')")"`（`place 0 -757` で 4、`place 0 -1360` で _2 が 4.4 m 先）。**エディタのセッションで最初の閃光は粒子が描かれない**ので、先に 1 つ捨てで回収する。閃光の箱は `core=548,412,608,472`・`flash=498,362,658,522`、測るのは `series --stat peak`。終わったら `t.MaxFPS 0`・`pie.py stop`・`desktop.py stop`。
- main へのマージ（ステップ 5）: Claude の一時フォルダの worktree `scratchpad/main-wt` が main を開いたまま残っている（`git worktree list`。作業一覧の「未回答の要確認」）。残っていれば `git checkout main` が失敗するので、症状索引の「`git checkout main` が … already checked out」の手順（`commit-tree` → `update-ref` → `checkout --ignore-other-worktrees main`）でマージする。消えていれば普通の `git checkout main` と `git merge --no-ff`。

## 検証

- check_records: ステップ 1 で通した（01 のハッシュを更新）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 1 で基準を撮った（上の撮り方）
