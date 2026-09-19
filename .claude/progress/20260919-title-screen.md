---
title: タイトル画面（作業一覧の項目 17）
status: 進行中
branch: feature/title
base: 380c9b5
started: 2026-09-19 10:53
updated: 2026-09-19 11:40
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
- [ ] 4. 出口をタイトルへつなぎ、通しで確かめる
  - 死亡画面の QUIT TO TITLE（`UWasamiDeathScreenWidget::PressQuitToTitle`）: 本家どおり DoOnce → 回収の記憶を空に → 1 s → タイトル。今の代わりの道（ライフ 3・セーブの `Hospital` を空に・Zone 1）をやめる（本家はセーブを消さないので、タイトルの RESUME がチェックポイントから続ける）。
  - スコア画面の NEXT の後（`AWasamiGameMode::LeaveFinishedLevel`）: セーブの病院の欄を空に・回収の記憶とライフを戻した後、Zone 1 でなくタイトルを開く（本家は `Replay Mode?` ならタイトル、でなければ `06_Cinematic`。本作に次の章は無い）。
  - デバッグ `Wasami.Title`（タイトルを開く）。テスト `Wasami.ZoneFlow.Escape`・死亡画面のテストの行き先を直す。
  - PIE: エディタで `L_Title` を開いて `python Tools/pie.py start` → 版と RESUME の有無 → NEW GAME（セーブあり: RESTART? → YES）→ 赤い閃光と脈動・暗転 → 10 s で Zone 1 の到着。`Wasami.Checkpoint 8` → `Wasami.Title` → RESUME → 5 s で Zone 2 の `PlayerStart_MiniBoss`。QUIT は PIE を止めるので最後に。台本 `Tools/playthrough.py` に区間 `title` を足す（`click_at` はビューポートの割合）。`--record` で撮り、Discord に連番のグリッド（タイトル → NEW GAME の閃光 → 暗転）。
  - 変更予定: `WasamiDeathScreenWidget.cpp`、`WasamiGameMode.*`、`Tests/WasamiDeathScreenTests.cpp`・`Tests/WasamiZoneFlowTests.cpp`、`Tools/playthrough.py`、実装記録 09・13・06・01
- [ ] 5. 閉じる
  - 作業一覧の項目 17 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順: パッケージでなく PIE で `L_Title` から）を直す。note の原稿に「タイトル画面」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 4 を始める（ブランチ `feature/title`）。`UWasamiDeathScreenWidget` の `EStep::QuitToTitle`（`WasamiDeathScreenWidget.cpp` の `RunStep`）と `AWasamiGameMode::LeaveFinishedLevel` の行き先を `AWasamiGameMode::TitleLevelName` にし、デバッグ `Wasami.Title` を足して、死亡画面とスコア画面のテストの行き先を直す。PIE は上の計画の手順（`L_Title` を開いて始める）。

## 決定事項

- ステップ 0〜3 の決定（旧版を写す・メニューの 4 つ・RESUME の問いを作らない・暗転の後の 10 s / 5 s・OPTIONS は選択音だけ・タイトルは別のレベルとゲームモード・NEW GAME は進みで問う）は実装記録 14 に理由つきで移した。

## 要確認（ユーザー）

- 2026-09-19: タイトルの右上の版の文字 — 仮にプロジェクト設定の `ProjectVersion` を `0.1.0` にして `v0.1.0` と出す計画。理由: 本家は `v1.6.1`、WebGL 版は `package.json` の版（0.2.0）を出していた。本作に版の決まりが無い。場所: ステップ 2 の `TextBlock_0` と `Config/DefaultGame.ini`（`TODO(仮)`）。

## 再開時の注意

- 取り込んだアセットは `/Game/DD/...`・`/Game/Wasami/UI/Title/...`（git の外。`python Tools/dd/prepare_title.py` → `import_dd_ui`〈か `dd_ui.import_title()`〉で作り直せる）。本作の素材の原本は `SourceArt/Wasami/UI/title_logo.png`・`title_face.png`（Git LFS）。
- `L_Title` は git の外（`Content/Stage/Maps/L_Title.umap`）。無ければ `WasamiStageTools.build_title_level`（MCP が繋がっていなければ `python Tools/ue_remote.py -c` で `dd_level.build_title()`）。
- 本物のセーブ（`Saved/SaveGames/structSlot.sav`）はいま進みあり（RESUME が出る）。ステップ 3 の PIE では NEW GAME・QUIT の問いを NO で閉じただけで、セーブは書き換えていない。

## 検証

- ステップ 3: `editor_cycle.py` でビルドが通った。テスト `Wasami.Title.*` 4 件と `Wasami.DeathScreen.*` 5 件・`Wasami.GameFlow.*` 7 件が通った。`check_records --update` を通した。PIE で `L_Title` を開くと `WasamiTitleGameMode`・ポーンなし・画面（RESUME あり）が出て、QUIT で「Giving Up?」の枠、NEW GAME で RESTART? の枠と文が出て、どちらも NO で閉じた。暗転と音の見え方・聞こえ方と、YES の後の道はステップ 4 の通しで
