---
title: タイトル画面（作業一覧の項目 17）
status: 進行中
branch: feature/title
base: 380c9b5
started: 2026-09-19 10:53
updated: 2026-09-19 12:30
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# タイトル画面（作業一覧の項目 17）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 17（大目標 2）。最終目標の「WebGL版と同じように倣う点: タイトル画面」。WebGL 版が原作 `UMG_TitleScreen`（旧版 v1.6.1）から写したタイトル画面（配置・アニメ・音・NEW GAME の確認・開始の演出）を作り、NEW GAME で Zone 1（エレベーターの到着）へ、RESUME でセーブの地点へ。死亡画面の QUIT TO TITLE とスコア画面の NEXT の後の行き先を、今の代わりの道（Zone 1 の最初）からタイトルに替える。ロゴとワサミの顔は本作の素材（WebGL 版の `public/title/logo.webp`・`wasami-face.webp`）。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードと WebGL 版の記録を読み、計画を立てる … 2026-09-19 完了。読んだものは実装記録 14 の「原作データの根拠」と「ボタンの道」「タイトルのレベル」に移した。
- [x] 1. タイトルの素材 … 2026-09-19 完了。前処理 `Tools/dd/prepare_title.py`（顔に WebGL 版のフィルタとマスク、ロゴのグローを別の絵に）と `dd_ui.import_title()` を足して取り込んだ（中身は実装記録 14）。筆の跡の材質は推定でなく焼き込みのシェーダーから組んだ（本家の名前 `MM_TitleScreen_Mask_Grey` のまま）ので、項目 28 の後回しの一覧には書かない。
- [x] 2. タイトルの画面の木とアニメと音 … 2026-09-19 完了。`UWasamiTitleScreenWidget`（木・ホバー・Construct・`PlayFadeOut`/`PlayFadeOut0`/`FadeOutMusic`・`HasProgress`・`Show`）とテスト `Wasami.Title.*`（3 件とも通過）、版の文字の `ProjectVersion`。中身は実装記録 14。
- [x] 3. ボタンの道とタイトルのレベル … 2026-09-19 完了。NEW GAME（進みがあれば問う → セーブを消す → 10 s で Zone 1）・RESUME（5 s でチェックポイントのゾーン）・OPTIONS（選択音だけ）・QUIT（問うて閉じる）、`AWasamiGameMode::TitleLevelName`・`LevelForCheckpoint`、`UWasamiSaveGame::Erase`、`AWasamiTitleGameMode`、`L_Title`（`build_title_level`）、`GameDefaultMap` をタイトルに、テスト `Wasami.Title.WaysOut`。中身は実装記録 14。
- [x] 4. 出口をタイトルへつなぎ、通しで確かめる … 2026-09-19 完了。死亡画面の QUIT TO TITLE（セーブはそのまま）とスコア画面の NEXT の後（`FinishedLevel`）の行き先をタイトルにし、`GetLevelToOpen`・デバッグ `Wasami.Title`・台本 `playthrough.py` の区間 `title` と `click_widget` を足した。PIE で NEW GAME（問いあり・なし）・QUIT TO TITLE → RESUME → Zone 2・脱出 → タイトルを通した（中身と結果は実装記録 14・09・13・01）。
- [ ] 5. 閉じる
  - 作業一覧の項目 17 を完了にし（完了の条件の読み替えを書く）、実装記録（索引の 14 の「作っている途中」を外す）と handover の「現状と次の一歩」（遊んで確かめる手順: パッケージでなく PIE で `L_Title` から）を直す。note の原稿に「タイトル画面」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`。GIF の元はステップ 4 の収録 `Intermediate/DesktopAgent/shots/step4_title.mkv`〈git の外。無ければ `playthrough.py run title --setup --record …` で撮り直す〉）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 5 を始める（ブランチ `feature/title`）。作業一覧 `.claude/roadmap.md` の項目 17 を完了にし、索引・handover・note の原稿と記事を直して、記録を消して main へマージし push、ブランチを消す。

## 決定事項

- ステップ 0〜3 の決定（旧版を写す・メニューの 4 つ・RESUME の問いを作らない・暗転の後の 10 s / 5 s・OPTIONS は選択音だけ・タイトルは別のレベルとゲームモード・NEW GAME は進みで問う）は実装記録 14 に、ステップ 4 の決定（QUIT TO TITLE はセーブを消さず RESUME で続ける・NEXT の後は次の章が無いのでタイトル）は 09・13 に理由つきで移した。

## 要確認（ユーザー）

- 2026-09-19: タイトルの右上の版の文字 — 仮にプロジェクト設定の `ProjectVersion` を `0.1.0` にして `v0.1.0` と出す計画。理由: 本家は `v1.6.1`、WebGL 版は `package.json` の版（0.2.0）を出していた。本作に版の決まりが無い。場所: ステップ 2 の `TextBlock_0` と `Config/DefaultGame.ini`（`TODO(仮)`）。

## 再開時の注意

- 取り込んだアセットは `/Game/DD/...`・`/Game/Wasami/UI/Title/...`（git の外。`python Tools/dd/prepare_title.py` → `import_dd_ui`〈か `dd_ui.import_title()`〉で作り直せる）。本作の素材の原本は `SourceArt/Wasami/UI/title_logo.png`・`title_face.png`（Git LFS）。
- `L_Title` は git の外（`Content/Stage/Maps/L_Title.umap`）。無ければ `WasamiStageTools.build_title_level`（MCP が繋がっていなければ `python Tools/ue_remote.py -c` で `dd_level.build_title()`）。
- 本物のセーブ（`Saved/SaveGames/structSlot.sav`）はステップ 4 の PIE の前の控え `Intermediate/SaveBackup/structSlot_step4.sav` から戻した（進みあり・チェックポイント 4。RESUME が出る）。

## 検証

- ステップ 4: `editor_cycle.py` でビルドが通った。テスト `Wasami.DeathScreen.*`・`Title.*`・`GameFlow.*`・`ZoneFlow.*`・`LevelClear.*` の 26 件が通った。`check_records --update` を通した。PIE の結果は実装記録 14 の「確かめたこと」。
