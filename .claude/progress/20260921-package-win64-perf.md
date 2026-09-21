---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 12:55
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
- [x] 2. `BuildCookRun` を通した … 2026-09-21 完了。初回は `GameFeatureData` の規則が無くてクックがエラー 2 件で落ちたので `Config/DefaultGame.ini` に規則を足し、`BUILD SUCCESSFUL`（`Saved/Archive/Windows/`、約 1.0 GB）。手順を `distribution.md` に、失敗を症状索引に書いた。
- [ ] 3. パッケージの中身の確認（原作のロゴとキャラクターのモデルが入っていない・pak の大きさ）と、exe が起動してタイトルが出ること ← 次
  - 変更予定: なし（確認だけ。必要なら記録に表を足す）
- [ ] 4. 本編の fps を 7 か所で測る（実装記録 00 の表に本編の列を足す）
- [ ] 5. パッケージ版で通しプレイ（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）
- [ ] 6. 結果をまとめて項目 36 を閉じる（60 前後に届かなければ画質の選択肢の項目を立てる。大目標 3 の達成）

## 次にやること

ステップ 3: 出来たパッケージ `Saved/Archive/Windows/` の中身を確かめる。(a) 原作のロゴとキャラクターのモデルが入っていないこと（`Manifest_UFSFiles_Win64.txt` の名前を見る。クック前の確認は項目 21 で済んでいるが、実物でも見る）、(b) `Tools/console_session.py` で exe を起動してタイトルが出ること（`Tools/desktop.py shot` で 1 枚）。エディタは閉じてから起動する。

## 決定事項

- 2026-09-21: 出力先は `Saved/Archive/Win64`（`Saved/` は git の対象外。`distribution.md` の「出力先はリポジトリの外（`Saved/` か別のフォルダ）」）。C ドライブの空きは 166 GB あるので足りる。
- 2026-09-21: **クックとパッケージ版の実行の間はエディタを閉じる**。この PC の VRAM は 6 GB で、エディタだけで 2.9〜4.1 GB の GPU メモリを使う（実装記録 00 の性能の表）。本家を同時に動かさないのと同じ理由で、本編の fps はエディタを閉じて測る（`.claude/guides/verification.md`）。
- 2026-09-21: パッケージ版は**対話デスクトップで起動する**（`python Tools/console_session.py <exe>`）。Claude は Windows のセッション 0 にいるので直に起動すると DXGI で落ちる（記憶・`editor_cycle.py` の説明）。操作と撮影は `python Tools/desktop.py`。
- 2026-09-21: 本編の fps は、PIE と同じ測り方（エンジンの CSV プロファイラ）を使えるか試してから決める。`Development` のパッケージはコンソール（`~`）と `stat unit` / `csvprofile start|stop` が使える。CSV は `<パッケージ>/wasami_deception/Saved/Profiling/CSV` に出る。読めなければ `stat unit` の画面を撮って読む（項目 36 の完了の条件は `stat unit` / `stat fps`）。

- 2026-09-21: クックが `GameFeatureData` の規則が無いというエラー 2 件で落ちる件は、**`Config/DefaultGame.ini` に規則を 1 行足して直した**（UAT の `-IgnoreCookErrors` は本物のエラーまで黙らせるので採らない）。理由は ini のコメントと症状索引の `Error_UnknownCookFailure` の項に書いた。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」に実際に通った形で書いた（2 回目からは 1 分ほど。`run_in_background` で走らせ、**応答を終える前に必ず結果を読む**）。出来上がりは `Saved/Archive/Windows/`（本体は `wasami_deception/Binaries/Win64/wasami_deception.exe`、起動は直下の `wasami_deception.exe`）。
- **パッケージ版の起動**: エディタを閉じて（`python Tools/editor_cycle.py --quit-only`）から `python Tools/console_session.py "C:\Users\User\Desktop\wasami_deception\Saved\Archive\Windows\wasami_deception.exe"`。操作と撮影は `python Tools/desktop.py`。コンソールは `~`、`stat unit` / `csvprofile start|stop` が使える（`Development`）。
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
- **エディタ**: ステップ 2 の終わりに開き直した（`python Tools/editor_cycle.py --no-quit --no-build`）。C++ は変えないのでビルドは要らない。
- **クック前の確認は項目 21 のステップで一度済んでいる**（実装記録 00・作業一覧の項目 21）: 既定のマップ `/Game/Stage/Maps/L_Title` とゲームモード、`/Game` の外を指す参照（`Intermediate/Overnight/pkg_check.py` で 3 つのマップから辿った結果が `pkg_check.json`。`/Engine`・`/ACLPlugin` だけ）、原作のロゴとキャラクターのモデルの 3 つとも問題なし。`Config/DefaultGame.ini` は `[/Script/UnrealEd.ProjectPackagingSettings]` を置かない（`MapsToCook` 無し = `/Game` を全部クックする）。項目 35 で使っていないアセット 18 個は消した。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- check_records: OK（`--update` 済み。`DefaultGame.ini` を変えたので 00 記録の `DefaultGame.ini` の節と変更履歴を直した）
- C++ ビルド: `BuildCookRun -build` が通った（`Development` の Win64）
- パッケージ: `BUILD SUCCESSFUL` / `AutomationTool exiting with ExitCode=0`、クックは `Success - 0 error(s), 1 warning(s)`（残る 1 件は MCP プラグインの EULA の注意で無害）
