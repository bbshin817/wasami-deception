---
title: Telepathy の印を敵の体の煙のシルエットにする（項目 64）
status: 進行中
branch: main
base: 18177ef
started: 2026-09-27 11:00
updated: 2026-09-27 11:00
---

# Telepathy の印を敵の体の煙のシルエットにする（項目 64）

## 依頼

「テレパシーで見える敵の影について、本家もこのような実装になっていますか？ どうにも本作では、単純な球体として簡略化されている印象を受けます。私のイメージでは、敵の体そのものが煙を帯びているようなイメージでした。」
→ 本家の最新版も画面空間の煙の丸だと説明し、選択肢（1 今のまま / 2 体の形に変える / 3 併用）から **2** を選んだ（本家のどちらの版とも違う本作独自の見た目）。

## 計画

- [ ] 1. C++: トラッカーを体のカスタム深度＋ステンシルのフェードに替え、ポストプロセスの部品を持たせる。丸のウィジェット `UWasamiTelepathyTrackerWidget` を消す。テストを直す。`r.CustomDepth=3` ← 作業中
  - 変更予定: `Source/wasami_deception/WasamiTelepathyTracker.*`・`WasamiTelepathyTrackerWidget.*`（削除）・`WasamiPowerComponent.cpp`・`Tests/WasamiPowerTests.cpp`・`Tests/WasamiEnemyTests.cpp`・`Config/DefaultEngine.ini`
- [ ] 2. 材質: `dd_powers` にポストプロセスのマスター `M_DD_TelepathySilhouette` とインスタンス `/Game/Wasami/Powers/MI_WasamiTelepathySilhouette` を作る。エディタで作り、PIE の Zone 2 で撮って確かめる。実装記録 04 を直す。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`、`/Game/Pipeline/Materials`、`/Game/Wasami/Powers`

## 次にやること

ステップ 1 の C++ を書き、`python Tools/editor_cycle.py` でビルドする。

## 決定事項

- 2026-09-27: 印の対象の拾い方（0.8 秒ごと・全敵・No Telepathy）、効果時間、音、揺れは変えない。変えるのは見た目だけ。
- 2026-09-27: フェードは敵ごとに、ステンシルの値（0〜255）に込める。長さは本家の Appear 0.5 秒・Disappear 0.3 秒のまま。ポストプロセスの材質は 1 つで、トラッカーごとの unbound の PostProcess 部品に同じ材質を載せる（同じ材質の blendable は 1 回にまとまる。PIE で 1 体と複数で明るさが同じかを見る）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは閉じている（MCP は接続できなかった）。ビルドは `python Tools/editor_cycle.py`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
