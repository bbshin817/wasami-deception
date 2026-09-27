---
title: Telepathy の印を敵の体の煙のシルエットにする（項目 64）
status: ユーザー待ち
branch: main
base: 18177ef
started: 2026-09-27 11:00
updated: 2026-09-27 20:15
---

# Telepathy の印を敵の体の煙のシルエットにする（項目 64）

## 依頼

「テレパシーで見える敵の影について、本家もこのような実装になっていますか？ どうにも本作では、単純な球体として簡略化されている印象を受けます。私のイメージでは、敵の体そのものが煙を帯びているようなイメージでした。」
→ 本家の最新版も画面空間の煙の丸だと説明し、選択肢（1 今のまま / 2 体の形に変える / 3 併用）から **2** を選んだ（本家のどちらの版とも違う本作独自の見た目）## 計画

- [x] 1. C++: 印を体のカスタム深度＋ステンシルのフェードとポストプロセスに替え、丸のウィジェットを消した。`r.CustomDepth=3` … 2026-09-27 完了。実装記録 04 の「印」
- [x] 2. 材質 `M_DD_TelepathySilhouette`・`MI_WasamiTelepathySilhouette` を作り、PIE で撮った … 2026-09-27 完了。実装記録 04 の表
- [x] 3. パッケージ 3 つを作り直した … 2026-09-27 20:08 完了（Development `Saved/Archive/Windows`・Shipping `Saved/Archive/Shipping/Windows`・Release の zip `Saved/Archive/Release/WasamiDeception-Windows-x64.zip`。どれも 0 エラー・1235 パッケージ）。パッケージ版で使うためのコマンド `Wasami.Power Name` を足した（06 記録）。Development の Zone 2 で見張りが壁越しに赤い煙の人影になるのを撮った（`Intermediate/DesktopAgent/shots/shot-201116.png`・`shot-201136.png`。どちらもポーズ画面越し）
- [ ] 4. ユーザーの見た目の確かめ（煙の濃さ・広がり・色の指示があれば `dd_powers.TELEPATHY_SILHOUETTE_*` を直して `make_telepathy_silhouette()` を呼ぶ）。問題が無ければ note の記事を直し、記録を閉じる。

## 次にやること

ユーザーがパッケージ（どれも 2026-09-27 20:08 のもの）で Telepathy の煙を見て、値の直しを指示するのを待つ。煙の周りの輪の標本がずれた「残像」のように見えるときは、輪の数を増やすか `RadiusPx` を下げる。

## 決定事項

- 2026-09-27: 印の対象の拾い方（0.8 秒ごと・全敵・No Telepathy）、効果時間、音、揺れは変えない。変えるのは見た目だけ。

## 要確認（ユーザー）

- 2026-09-27: 煙の見た目の値 — 仮に Claude が目で決めた（濃さ 1・輪の半径 30 px・揺らぎ 9 px・上への立ち 0.7・体の塗り 0.3・赤 (1, 0.06, 0.03)）。理由: 本家に無い本作独自の見た目。場所: `dd_powers.TELEPATHY_SILHOUETTE_DEFAULTS`・`TELEPATHY_SILHOUETTE_COLOR`。

## 再開時の注意

（なし。エディタは開き直してある）

## 検証

- C++ ビルド成功。テスト `Wasami.Powers.TelepathyTracker`・`TelepathyTargets`・`Wasami.Enemy.Actor.Powers` が成功（エディタの中）
- PIE: `Intermediate/DesktopAgent/shots/shot-190130.png`（開いた扉の前の 2 体）・`shot-190224.png`（閉じた扉越し）
- check_records: OK
