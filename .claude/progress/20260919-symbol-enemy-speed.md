---
title: ポータルのシンボルの色と位置、敵ワサミの速さ（有人セッションの指摘）
status: 進行中
branch: feature/options-pause（直してからコミットを main へも cherry-pick する）
base: 7a37d9a
started: 2026-09-19 13:35
updated: 2026-09-19 13:45
---

# ポータルのシンボルの色と位置、敵ワサミの速さ（有人セッションの指摘）

## 依頼

2026-09-19 の有人セッションでのユーザーの指摘（原文）:
- 「wasami_symbolはすでに画像に着色されています。本家の慣習を見ると、アセットは白の透過画像で、ゲーム側で任意の色を指定していると思われます。そのためか、現在のロゴは周りの魔法陣とシンボルで若干の赤のコントラストに差異を感じます。」
- 「wasami_symbolは魔法陣において、やや上方に調整してください。」
- 「敵ワサミのスピードが早く、ゲームの難易度が格段に上がっています。これを緩和してください」

## 計画

- [x] 1. ポータルのシンボル（2026-09-19 完了。中身は 08 記録）: 原本 `SourceArt/Wasami/UI/wasami_symbol.png` を白の透過画像にする（α はそのまま）。`Tools/dd/prepare_portal_logo.py` は白と α で描き、シンボルの箱をテクスチャの真ん中（輪の中心）に置く（今はサルの顔の箱の中心で 38 px 下）。`dd_gimmicks._build_portal_logo` に色のパラメータ `Tint`（既定 白 = 鍵の姿は今のまま）を足し、`MI_Portal_Wasami` の `Tint` を輪の平均の明るさに合わせた赤にする。取り込み直して PIE でガレージのポータルを開けて撮り、輪と顔の赤を見比べる。
  - 変更予定: `SourceArt/Wasami/UI/wasami_symbol.png`、`Tools/dd/prepare_portal_logo.py`、`Content/Python/wasami_tools/pipeline/dd_gimmicks.py`、`/Game/Wasami/Portal/T_Portal_Wasami`・`MI_Portal_Wasami`、`/Game/Pipeline/Materials/M_DD_PortalLogo`（作り直せる）、実装記録 08・01
- [ ] 2. 敵ワサミの速さ: 巡回 350 → 200、追跡 800 → 430（本家の `BP_Monkey` の `Walk Speed`・`Run Speed`。WebGL 版の迷路の敵と同じ）。`AWasamiEnemy` の `MaxSpeed`・`NormalSpeed`・`SkateSpeed`、テスト `Wasami.Enemy.*` の期待値、実装記録 07。ビルド（`Tools/editor_cycle.py`）→ テスト → PIE。
- [ ] 3. main へ cherry-pick して push（エディタの作業ツリーを動かさないよう、一時の worktree で）。この記録を消す。

## 決定事項

- 2026-09-19: 「魔法陣」はガレージのポータル（外と内の輪 `portal_outer_active`・`portal_inner_active` と、その上の `Logo` の板）と読んだ。収録 `Intermediate/Overnight/portal_open.png` で、顔が輪より明るく橙に寄り、口が下の小円に近い。読み込み画面の紋章（`prepare_loader.py`）は印を本家の赤 192 で紋章の画像に焼き込んでいて輪と同じ赤なので、色は変えない。位置も本家の印の箱の中心（円の中心の 6.5 px 下）のままにする。
- 2026-09-19: 本家のポータルのテクスチャ（輪・サルの顔）は白でなく赤 (192, 0, 0) を焼き込んだ画像。色の差は画像の色でなく材質の明るさによる: 輪 = テクスチャ × (0.05 + 0.1 + 0.1 sin) × 0.2 × (1 + 40) で平均 0.03、ロゴ = テクスチャ × 0.05 × (1 + 40)。本家どおりでもロゴは輪の平均の 5/3 倍明るい。ユーザーの指示どおり白の画像を材質で色付けする形にし、色は輪の平均に合わせる（192 の線形 0.527 × 0.03 / 0.05 = 0.316）。
- 2026-09-19: 敵の速さは本家の `BP_Monkey`（`Walk Speed` 200・`Run Speed` 430）にする。WebGL 版（ゲームの規則の正本）はこの値で「歩き 300 < 追跡 430 < ダッシュ 600。歩いていると追いつかれ、走れば引き離せる」だった。いまのナースの値（350・800）は追跡がダッシュより速く、見つかると Vanish か気絶でしか逃げられない。

## 次にやること

ステップ 2 を始める（`Source/wasami_deception/WasamiEnemy.h` の `MaxSpeed`・`NormalSpeed`・`SkateSpeed`、`Tests/WasamiEnemyTests.cpp` の 350・800 の期待値）。

## 再開時の注意

- エディタは作業ブランチ `feature/options-pause` のビルドで開いている。main へは一時の worktree で cherry-pick する（エディタの作業ツリーで `git checkout main` をしない）。
