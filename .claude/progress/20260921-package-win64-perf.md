---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 13:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む -->

# Windows のパッケージと本編の性能の計測（作業一覧の項目 36）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 36（大目標 3 の最後の項目）。2026-09-21 の有人セッションで、項目 21 の性能の要確認に「クックして本編の fps を測る」と回答をもらった分。

- `Development` の Win64 でパッケージ（クック → ビルド → ステージ → pak → アーカイブ）し、出来た exe がタイトルから脱出まで通しで遊べる。
- 両ゾーンの代表の場所（項目 21 で PIE を測った 7 か所）の fps を測り、実装記録 00 の性能の表に**本編の列**を足す。
- **1080p で 60 前後に届かなければ**、製品に画質の選択肢（解像度スケールか品質プリセット）を用意する項目を立てる。
- パッケージに原作のロゴとキャラクターのモデルが入っていないことを確かめる。
- **手順は `.claude/guides/distribution.md` の「まだ整備していない」の節に書き足す**。
- クックとビルドはユーザーが 2026-09-21 に承認済み（無人運転で行ってよい）。**配布（誰かに渡す・公開する）は別途ユーザーに確認する**ので行わない。

## 計画

- [x] 1. 計画（この記録を作ってステップに分ける） … 2026-09-21 完了。
- [x] 2. `BuildCookRun` を通した … 2026-09-21 完了。`GameFeatureData` の規則が無くてクックがエラー 2 件で落ちたので `Config/DefaultGame.ini` に足し、`BUILD SUCCESSFUL`。手順を `distribution.md` に、失敗を症状索引に書いた。
- [x] 3. パッケージの中身の確認 … 2026-09-21 完了。**`/Game` のアセットが `L_Title` の 1 つしか入っていなかった**（全 501 パッケージ・494 クック。残りはエンジンとプラグインの既定）。`Config/DefaultGame.ini` に `[/Script/UnrealEd.ProjectPackagingSettings]` の `bCookAll=True` を足して直し、症状索引・`distribution.md`・実装記録 00 を直した。**作り直しはステップ 4。**
- [ ] 4. パッケージを作り直し、本編が入っていることと原作のロゴ・キャラクターのモデルが入っていないことを確かめ、exe が起動してタイトルが出ることまで見る ← 次
  - 変更予定: なし（必要なら記録に数を書く）
- [ ] 5. 本編の fps を 7 か所で測る（実装記録 00 の表に本編の列を足す）
- [ ] 6. パッケージ版で通しプレイ（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）
- [ ] 7. 結果をまとめて項目 36 を閉じる（60 前後に届かなければ画質の選択肢の項目を立てる。大目標 3 の達成）

## 次にやること

ステップ 4: `bCookAll=True` を入れたのでパッケージを**作り直す**（下の「再開時の注意」のコマンド。今度は `/Game` を全部クックするので初回は長い。`run_in_background` で走らせ、**応答を終える前に必ず結果を読む**）。終わったら:
- (a) 本編が入ったこと: `grep -c "^/game/" Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 1000 前後（`/Game` は 1139 パッケージ。1 桁なら失敗）。
- (b) 原作のロゴとキャラクターのモデルが入っていないこと: 同じ `ReferencedSet.txt` を名前で見る（クック前の確認は項目 21 で済んでいるが、実物でも見る）。
- (c) `python Tools/console_session.py "…\Saved\Archive\Windows\wasami_deception.exe"` で起動してタイトルが出ること（`python Tools/desktop.py shot` で 1 枚）。エディタは閉じてから起動する。

## 決定事項

- 2026-09-21: 出力先は `Saved/Archive/Windows`（`Saved/` は git の対象外）。
- 2026-09-21: **クックとパッケージ版の実行の間はエディタを閉じる**。この PC の VRAM は 6 GB で、エディタだけで 2.9〜4.1 GB の GPU メモリを使う（実装記録 00 の性能の表）。本家を同時に動かさないのと同じ理由（`.claude/guides/verification.md`）。
- 2026-09-21: パッケージ版は**対話デスクトップで起動する**（`python Tools/console_session.py <exe>`）。Claude は Windows のセッション 0 にいるので直に起動すると DXGI で落ちる。操作と撮影は `python Tools/desktop.py`。
- 2026-09-21: 本編の fps は、PIE と同じ測り方（エンジンの CSV プロファイラ）を使えるか試してから決める。`Development` のパッケージはコンソール（`~`）と `stat unit` / `csvprofile start|stop` が使える。CSV は `<パッケージ>/wasami_deception/Saved/Profiling/CSV` に出る。読めなければ `stat unit` の画面を撮って読む（項目 36 の完了の条件は `stat unit` / `stat fps`）。
- 2026-09-21: **`BUILD SUCCESSFUL` は中身を保証しない**ので、パッケージのたびに `ReferencedSet.txt` の `/game/` の数を見る（症状索引・`distribution.md`）。`UnrealPak.exe <…>.utoc -List` は中身の確認に使えない（ファイル名を持つ入り口だけを出し、クックしたパッケージはパッケージ ID で引くので名前が出ない）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」に実際に通った形で書いた。出来上がりは `Saved/Archive/Windows/`（起動は直下の `wasami_deception.exe`）。
  ```bash
  python Tools/editor_cycle.py --quit-only
  "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun \
    -project="C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" \
    -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive \
    -archivedirectory="C:\Users\User\Desktop\wasami_deception\Saved\Archive" > Intermediate/Overnight/uat_package.log 2>&1
  python Tools/editor_cycle.py --no-quit --no-build   # 終わったら開き直す
  ```
- **比べる相手**（実装記録 00 の「性能」の PIE の 7 か所。1080p 相当・Epic、fps avg / p95 / GPU ms）:
  | 場所（チェックポイント） | PIE fps avg | p95 | GPU ms |
  | --- | --- | --- | --- |
  | Z1 リフトの到着（cp 4） | 46.0 | 33.9 | 21.10 |
  | Z1 迷路の始まり（cp 5） | 57.7 | 54.8 | 16.64 |
  | Z1 駐車場（cp 6） | 52.7 | 36.9 | 18.40 |
  | Z2 独房（cp 7 + 場面の後） | 49.7 | 47.8 | 19.51 |
  | Z2 見張りの廊下（cp 8） | 47.3 | 35.0 | 20.50 |
  | Z2 迷路（cp 9） | 60.1 | 55.7 | 16.08 |
  | Z2 祭壇の車庫（cp 10） | 56.0 | 37.9 | 17.20 |
- **エディタ**: ステップ 2 の終わりに開き直したまま（ステップ 4 の前に閉じる）。C++ は変えないのでビルドは要らない。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- check_records: ステップ 3 の終わりに `--update` で通す（`DefaultGame.ini` を変えたので 00 記録を直した）。
- ステップ 3 の根拠: `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 493 件で `^/game/` は 1 件（`/game/stage/maps/l_title`）、`Intermediate/Overnight/uat_package.log` の `Packages Cooked: 494, ... Total Packages: 501`、コンテナの PackageStore も 494 パッケージ。`Content/` は 1139 パッケージ・1.1 GB。
