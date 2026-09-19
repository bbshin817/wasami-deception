---
title: タイトル画面（作業一覧の項目 17）
status: 進行中
branch: feature/title
base: 380c9b5
started: 2026-09-19 10:53
updated: 2026-09-19 11:55
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# タイトル画面（作業一覧の項目 17）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 17（大目標 2）。最終目標の「WebGL版と同じように倣う点: タイトル画面」。WebGL 版が原作 `UMG_TitleScreen`（旧版 v1.6.1）から写したタイトル画面（配置・アニメ・音・NEW GAME の確認・開始の演出）を作り、NEW GAME で Zone 1（エレベーターの到着）へ、RESUME でセーブの地点へ。死亡画面の QUIT TO TITLE とスコア画面の NEXT の後の行き先を、今の代わりの道（Zone 1 の最初）からタイトルに替える。ロゴとワサミの顔は本作の素材（WebGL 版の `public/title/logo.webp`・`wasami-face.webp`）。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードと WebGL 版の記録を読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」。
- [x] 1. タイトルの素材 … 2026-09-19 完了。前処理 `Tools/dd/prepare_title.py`（顔に WebGL 版のフィルタとマスク、ロゴのグローを別の絵に）と `dd_ui.import_title()` を足して取り込んだ（中身は実装記録 14）。筆の跡の材質は推定でなく焼き込みのシェーダーから組んだ（本家の名前 `MM_TitleScreen_Mask_Grey` のまま）ので、項目 28 の後回しの一覧には書かない。
- [x] 2. タイトルの画面の木とアニメと音 … 2026-09-19 完了。`UWasamiTitleScreenWidget`（木・ホバー・Construct・`PlayFadeOut`/`PlayFadeOut0`/`FadeOutMusic`・`HasProgress`・`Show`）とテスト `Wasami.Title.*`（3 件とも通過）、版の文字の `ProjectVersion`。中身は実装記録 14。
- [ ] 3. ボタンの道とタイトルのレベル
  - クリックの結線はここで（ステップ 2 はホバーだけ）。本家は RESUME だけ `OnPressed`、ほかは `OnClicked`。進みの判定は `UWasamiTitleScreenWidget::HasProgress`、暗転は `PlayFadeOut`/`PlayFadeOut0`、曲は `FadeOutMusic`。Delay は死亡画面と同じ `Pending` の形で `Advance` に足す。
  - NEW GAME: セーブに進みが無ければ確かめずに、あれば `UWasamiPopUpWidget`（Frame 0 = RESTART、`STARTING A NEW GAME WILL RESET ALL PROGRESS.`、Z 2、選択音 1.0）の YES の後に（ポップアップの `PressNo` で閉じる）: 曲を 1 s で消す → セーブを消す（本家の `Erase Save Files`: 新しいセーブ〈`Hospital` 空・`bLastCheckpointWarning` 偽〉を書く）→ ゲームだけの入力 → `FadeOut` → **10 s** 後に Zone 1 を開く（本家は `00_TypeWriter`）。
  - RESUME: 曲を 4 s で消す → ゲームだけの入力 → `FadeOut_0` → **5 s** 後にセーブのチェックポイントのゾーンを開く（0・4〜6 → Zone 1、7〜10 → Zone 2。本家の入口 `06_Hospital` の `Spawn` @81063 の振り分け。静的関数 `AWasamiGameMode::LevelForCheckpoint`）。本家の RESUME の問い `UMG_PopUp_Resume`（チェックポイントから続けるか、NO でレベルを最初から）は作らない（下の決定事項）。
  - QUIT: `UWasamiPopUpWidget`（Frame 1 = `quit_window_frame`、Z 2、選択音 1.0）の YES で `QuitGame`。
  - OPTIONS: 選択音 1.0 だけ（`UMG_Options` を Z 10 で開くのは項目 18。`TODO(項目 18)`）。
  - タイトルのレベル `/Game/Stage/Maps/L_Title`（空のレベル。`WasamiStageTools` に作る道具を足す。`Content/Stage` は git の外なので道具で作り直せるようにする）とゲームモード `AWasamiTitleGameMode`（ポーンなし。BeginPlay で本家の TitleScreen のレベル BP: `Reset Game Instance(True)` = 回収の記憶を空に・ライフ 3、タイトルの画面を Z 1 で出す）。レベルの World Settings でこのゲームモードにする。`Config/DefaultEngine.ini` の `GameDefaultMap` を `L_Title` に（`EditorStartupMap` は Zone 1 のまま）。
  - テスト（NEW GAME の確かめの有無・セーブを消す・10 s / 5 s の後のレベル名・チェックポイントからゾーン・QUIT のポップアップ）。
  - 変更予定: `WasamiTitleScreenWidget.*`、`WasamiTitleGameMode.*`（新）、`WasamiGameMode.*`（`TitleLevelName`・`LevelForCheckpoint`・`EraseSave`）、`WasamiSaveGame.*`（要れば）、`Tests/WasamiTitleScreenTests.cpp`、`Content/Python/wasami_tools/toolsets/stage.py`（と `pipeline/` の組み立て）、`Config/DefaultEngine.ini`、実装記録 09・06・02・00・01
- [ ] 4. 出口をタイトルへつなぎ、通しで確かめる
  - 死亡画面の QUIT TO TITLE（`UWasamiDeathScreenWidget::PressQuitToTitle`）: 本家どおり DoOnce → 回収の記憶を空に → 1 s → タイトル。今の代わりの道（ライフ 3・セーブの `Hospital` を空に・Zone 1）をやめる（本家はセーブを消さないので、タイトルの RESUME がチェックポイントから続ける）。
  - スコア画面の NEXT の後（`AWasamiGameMode::LeaveFinishedLevel`）: セーブの病院の欄を空に・回収の記憶とライフを戻した後、Zone 1 でなくタイトルを開く（本家は `Replay Mode?` ならタイトル、でなければ `06_Cinematic`。本作に次の章は無い）。
  - デバッグ `Wasami.Title`（タイトルを開く）。テスト `Wasami.ZoneFlow.Escape`・死亡画面のテストの行き先を直す。
  - PIE: エディタで `L_Title` を開いて `python Tools/pie.py start` → 版と RESUME の有無 → NEW GAME（セーブあり: RESTART? → YES）→ 赤い閃光と脈動・暗転 → 10 s で Zone 1 の到着。`Wasami.Checkpoint 8` → `Wasami.Title` → RESUME → 5 s で Zone 2 の `PlayerStart_MiniBoss`。QUIT は PIE を止めるので最後に。台本 `Tools/playthrough.py` に区間 `title` を足す（`click_at` はビューポートの割合）。`--record` で撮り、Discord に連番のグリッド（タイトル → NEW GAME の閃光 → 暗転）。
  - 変更予定: `WasamiDeathScreenWidget.cpp`、`WasamiGameMode.*`、`Tests/WasamiDeathScreenTests.cpp`・`Tests/WasamiZoneFlowTests.cpp`、`Tools/playthrough.py`、実装記録 09・13・06・01
- [ ] 5. 閉じる
  - 作業一覧の項目 17 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順: パッケージでなく PIE で `L_Title` から）を直す。note の原稿に「タイトル画面」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 3 を始める（ブランチ `feature/title`）。`WasamiTitleScreenWidget.*`（実装記録 14）と `WasamiGameMode.*`（06 記録の `FinishedLevel`・セーブの読み書き）・`WasamiPopUpWidget.*`（09 記録）を読み、ボタンの道（NEW GAME・RESUME・QUIT・OPTIONS）と `AWasamiTitleGameMode`・`L_Title`（`WasamiStageTools` で作る）・`GameDefaultMap` を作る。

## 本家の流れ（読んだもの）

- **レベル `TitleScreen`**（`pak_reference/_bytecode/DDeception/Content/TitleScreen.txt` の `ReceiveBeginPlay` @4390）: ゲームインスタンスの `Used Hard Respawn?` = 偽・`Hard Check Point` = 0 → ゲームモードの `Reset Game Instance(True)` → `Create(UMG_TitleScreen)` → `AddToViewport(1)` → `SaveSlot` を読んでゲームモードの `Level` を決め、実績を確かめる（写さない）。レベル自体は 2D のウィジェットだけ（3D の背景は無い）。
- **`Reset Game Instance`**（`pak_reference_2/.../BP_DD_GameMode.txt` @38662）: `Shards To Be Removed` を空に、ライフを `Total Lives` → `Reset Lives`、`Replay`（True）なら `Replay Mode?` 偽・`Game Instance Time` 0、ボーナスのオーブ・パワーのオーブ・秘密・連続回収などを空に。本作では回収の記憶を空に・ライフ 3（13 記録の `FinishedLevel` と同じ）。
- **`Erase Save Files`**（同 @39493）: 新しい `SaveSlot`（`Last Checkpoint Warning` 偽ほか）と新しい `structSlot` を書く。本作のセーブは 1 つなので、新しい `UWasamiSaveGame` を書く。
- **`UMG_TitleScreen`（旧版 `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt`）**:
  - `Construct` @7356: DoOnce → `SaveSlot` の `Progress` → `Current Level` → `PlayAnimation(Slideshow)` → `SetInputMode_UIOnlyEx` → カーソル → `Setup Buttons` → `CreateSound2D(Pause_Sound_v1, 1, 0.5)` → `SetSound` → `FadeIn(2, 0.5, 0)` → `SaveSlot` の `New Game?` と `Progress < 2` なら `Resume` を外す → `Image_97` を `Progress` の敵の横顔に（本作は顔の絵 1 枚）。
  - `Setup Buttons` @9538: 押下とホバーのブラシを `title_screen_selection_marker`（394×74。Margin・Tint・DrawAs・Tiling は NewGame の元の様式）にした様式を 7 つのボタンへ。
  - ホバー（各ボタン 2 つ）: 入ると文字を白 (1,1,1,1)、出ると `Unhovered Color`。
  - NEW GAME クリック @8532: `SaveSlot` の `New Game?` が真なら確かめずに @1748、偽なら `UMG_PopUp`（`Text` = `STARTING A NEW GAME WILL RESET ALL PROGRESS.`、Frame は既定 0、Z 2）と `UI_Select_V3`（1）、YES = `New Game` → ポップアップの `Press No` → @1748: `Music.FadeOut(1)` → `Erase Save Files` → `New Game?` 偽で保存 → `SetInputMode_GameOnly` → `PlayAnimation(FadeOut)` → `Delay(10)` → `OpenLevel('00_TypeWriter')`。
  - RESUME（OnPressed）@9272: `levelStruct[Progress − 1].LevelCheckpoint > 0` なら `UMG_PopUp_Resume`（`Progress found for: <RED>{levelName}</>\r\n\r\nDo you wish to continue from checkpoint?`、Frame 2、Z 5。YES = チェックポイントのレベルを開く、NO = その欄を空にして `00_Ballroom`）、でなければ @115: `Music.FadeOut(4)` → `SetInputMode_GameOnly` → `PlayAnimation(FadeOut_0)` → `Delay(5)` → `OpenLevel('00_Ballroom')`。
  - QUIT クリック @8527: `UMG_PopUp`（Frame 1、Z 2）と `UI_Select_V3`（1）、YES = `QuitGame`。
  - OPTIONS クリック @8405: `CreateAndAddWidget(UMG_Options, Z 10)` と `UI_Select_V3`（1）。
  - アニメ（`_assets/.../UMG_TitleScreen.json`）: `Slideshow`（`Image_128`）、`FadeOut`（`Image_0`・`CanvasPanel_0`・`Image_2`、音のトラック: `Start_New_Game` の音量のキー 0 / 27000 / 98999 tick、`Bierce_Title_Modified_03` の区間 98999〜250942）、`FadeOut_0`（`Image_0`）。
  - 最新版（`pak_reference_2`、v1.9.6）はアニメが `FadeOut_NewGame`・`FadeOut_Resume`（RESUME にも開始の音と声）・`FadeOut_Norm` に分かれ、RESUME の YES は病院なら `06_Hospital` を開く。旧版を採る（下の決定事項）。
- 入口 `06_Hospital` の `Spawn`（`pak_reference_2/.../06_Hospital.txt` @81063。`.claude/references/game-flow/README.md` の表）: チェックポイント 4〜6 で Zone 1、7〜10 で Zone 2 を開く。タイトルの続きもここを通る。
- WebGL 版（`.claude/references/webgl/implementation-records/10-hud-tablet.md` の `title.ts` と styles.css「タイトル画面」、01 記録の「ボタン / イベント配線」）: メニューは RESUME / NEW GAME / OPTIONS（ページは閉じられないので QUIT なし）。RESUME も確かめずに暗転してから続きへ。暗転の後はブラウザの都合（ユーザー操作の有効期間）で 3.75 s で始めていた。

## 決定事項

- 2026-09-19: **画面は WebGL 版と同じく旧版（v1.6.1）の `UMG_TitleScreen` を写す**（木・アニメ・音・時間。最新版のアニメの分け方〈RESUME にも開始の音と声〉は採らない）— 最終目標の「WebGL版と同じように倣う点: タイトル画面」（ユーザーの原文）。WebGL 版は旧版を写した。
- 2026-09-19: **メニューは RESUME / NEW GAME / OPTIONS / QUIT**。CHAPTERS・REPLAY・EXTRAS と隠れた動画・スライドは作らない — WebGL 版も作っていない。本作は 1 ステージで、章の選択・リプレイ・おまけが無い。QUIT は WebGL 版に無いが（ページは閉じられない）、作業一覧の項目の題にあり、本家どおりデスクトップのゲームを閉じる道が要る。
- 2026-09-19: **RESUME は本家の問い `UMG_PopUp_Resume` を出さずに、暗転して 5 s 後にセーブのチェックポイントのゾーンを開く** — WebGL 版と同じ（問いなしで続きへ）。本家の問いの NO（そのレベルを最初から）は NEW GAME と重なり、YES の行き先は本家の旧版で 5 s 後にレベルを開くのと同じ。
- 2026-09-19: **暗転の後にレベルを開くまでの時間は本家のコードの Delay（NEW GAME 10 s、RESUME 5 s）** — ゲームの流れは本家のコードから写す。WebGL 版の 3.75 s はブラウザのユーザー操作の有効期間に収めるための都合（WebGL 版 01 記録）で、UE には無い。
- 2026-09-19: **OPTIONS はこの項目では選択音だけ**。`UMG_Options` を開くのは項目 18（依存 17）。
- 2026-09-19: **タイトルは別のレベル `L_Title` と別のゲームモード `AWasamiTitleGameMode`** — 本家もタイトルは別のレベル `TitleScreen`。今のゲームモードはゾーンの準備（チェックポイント・開始の場所・ゾーンの流れ・フェード）を BeginPlay でするので、タイトルでは使わない。パッケージの始まり（`GameDefaultMap`）をタイトルにし、エディタの開始のレベルは Zone 1 のまま（開発で開くのはゾーン）。

## 要確認（ユーザー）

- 2026-09-19: タイトルの右上の版の文字 — 仮にプロジェクト設定の `ProjectVersion` を `0.1.0` にして `v0.1.0` と出す計画。理由: 本家は `v1.6.1`、WebGL 版は `package.json` の版（0.2.0）を出していた。本作に版の決まりが無い。場所: ステップ 2 の `TextBlock_0` と `Config/DefaultGame.ini`（`TODO(仮)`）。

## 再開時の注意

- 取り込んだアセットは `/Game/DD/...`・`/Game/Wasami/UI/Title/...`（git の外。`python Tools/dd/prepare_title.py` → `import_dd_ui`〈か `dd_ui.import_title()`〉で作り直せる）。本作の素材の原本は `SourceArt/Wasami/UI/title_logo.png`・`title_face.png`（Git LFS）。

## 検証

- check_records: ステップ 2 で通した
- C++ ビルド: ステップ 2 で通した（`editor_cycle.py`、その後 `.cpp` だけ Live Coding）。テスト `Wasami.Title.*` 3 件通過
- エディタ: ステップ 2 で PIE の Zone 1 に `unreal.WasamiTitleScreenWidget.show(<ゲームのワールド>)` で出し、配置（ロゴとグロー・顔・メニュー 4 つ・注記・`v0.1.0`）を撮って確かめた。暗転と音の見え方・聞こえ方はステップ 4 の通しで
