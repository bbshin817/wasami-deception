---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 12:31
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
- [ ] 2. クック前の確認と `BuildCookRun` を通す ← 次
  - 変更予定: `.claude/guides/distribution.md`（手順を書き足す）、出力は git の外（`Saved/Archive/Win64`）
- [ ] 3. パッケージの中身の確認（原作のロゴとキャラクターのモデルが入っていない・pak の大きさ）と、exe が起動してタイトルが出ること
- [ ] 4. 本編の fps を 7 か所で測る（実装記録 00 の表に本編の列を足す）
- [ ] 5. パッケージ版で通しプレイ（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）
- [ ] 6. 結果をまとめて項目 36 を閉じる（60 前後に届かなければ画質の選択肢の項目を立てる。大目標 3 の達成）

## 次にやること

ステップ 2: エディタを保存して閉じ（`python Tools/editor_cycle.py --quit-only`）、`RunUAT BuildCookRun` を走らせる（下の「再開時の注意」のコマンド）。通ったら `.claude/guides/distribution.md` の手順を実際に通った形に書き足し、エディタを開き直して（`python Tools/editor_cycle.py --no-quit --no-build`）コミットする。

## 決定事項

- 2026-09-21: 出力先は `Saved/Archive/Win64`（`Saved/` は git の対象外。`distribution.md` の「出力先はリポジトリの外（`Saved/` か別のフォルダ）」）。C ドライブの空きは 166 GB あるので足りる。
- 2026-09-21: **クックとパッケージ版の実行の間はエディタを閉じる**。この PC の VRAM は 6 GB で、エディタだけで 2.9〜4.1 GB の GPU メモリを使う（実装記録 00 の性能の表）。本家を同時に動かさないのと同じ理由で、本編の fps はエディタを閉じて測る（`.claude/guides/verification.md`）。
- 2026-09-21: パッケージ版は**対話デスクトップで起動する**（`python Tools/console_session.py <exe>`）。Claude は Windows のセッション 0 にいるので直に起動すると DXGI で落ちる（記憶・`editor_cycle.py` の説明）。操作と撮影は `python Tools/desktop.py`。
- 2026-09-21: 本編の fps は、PIE と同じ測り方（エンジンの CSV プロファイラ）を使えるか試してから決める。`Development` のパッケージはコンソール（`~`）と `stat unit` / `csvprofile start|stop` が使える。CSV は `<パッケージ>/wasami_deception/Saved/Profiling/CSV` に出る。読めなければ `stat unit` の画面を撮って読む（項目 36 の完了の条件は `stat unit` / `stat fps`）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **パッケージのコマンド**（`.claude/guides/distribution.md`。1 回で 30 分以上かかる見込み。`run_in_background` で走らせ、`Monitor` でログの終わりを待つ。**応答を終える前に必ず結果を読む**）:
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun \
    -project="C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" \
    -noP4 -platform=Win64 -clientconfig=Development -cook -build -stage -pak -archive \
    -archivedirectory="C:\Users\User\Desktop\wasami_deception\Saved\Archive"
  ```
  - 完了の確かめ方: 標準出力の最後が `BUILD SUCCESSFUL`（失敗は `BUILD FAILED` と `AutomationTool exiting with ExitCode=`）。成果物は `Saved/Archive/Windows/wasami_deception.exe` と `Saved/Archive/Windows/wasami_deception/Content/Paks/*.pak`。UAT のログは `%LOCALAPPDATA%\UnrealBuildTool\Log.txt` と `Saved/Logs/`（クックは `Saved/Cooked/`）。
  - 途中で止めた・失敗したときは、クックの中間出力 `Saved/Cooked/Windows` が残る（作り直せるので消してよい。git の外）。
- **エディタ**: 反復の始めに開いている（レベル `L_Hospital_Zone2`。2026-09-21 12:30 時点）。閉じるのは `python Tools/editor_cycle.py --quit-only`（保存してから閉じる）、開き直すのは `python Tools/editor_cycle.py --no-quit --no-build`。C++ は変えないのでビルドは要らない。
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
- **クック前の確認は項目 21 のステップで一度済んでいる**（実装記録 00・作業一覧の項目 21）: 既定のマップ `/Game/Stage/Maps/L_Title` とゲームモード、`/Game` の外を指す参照（`Intermediate/Overnight/pkg_check.py` で 3 つのマップから辿った結果が `pkg_check.json`。`/Engine`・`/ACLPlugin` だけ）、原作のロゴとキャラクターのモデルの 3 つとも問題なし。`Config/DefaultGame.ini` は `[/Script/UnrealEd.ProjectPackagingSettings]` を置かない（`MapsToCook` 無し = `/Game` を全部クックする）。項目 35 で使っていないアセット 18 個は消した。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- check_records: 未実行（この反復ではソースを変えていない）
- C++ ビルド: 不要（C++ は変えない）
- エディタでの確認（取り込み・組み立て・PIE）: 不要（この反復は計画だけ）
