---
title: オプション画面とポーズ画面（作業一覧の項目 18）
status: 進行中
branch: main
base: d7ae8be
started: 2026-09-19 12:03
updated: 2026-09-19 12:03
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# オプション画面とポーズ画面（作業一覧の項目 18）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 18（大目標 2）。最終目標の「WebGL版と同じように倣う点: オプション画面・ポーズ画面」。`UMG_Options`（画質・解像度・明るさ・音量 3 種・字幕・マウス感度・頭の揺れ・Y 反転・ダッシュの切り替え・マウスのスムージング・難易度。設定のセーブ `BP_DD_Settings_SaveGame`）と `UMG_Pause`（Esc）を WebGL 版の記録どおりに作る。完了の条件: WebGL 版 04 記録（`settings.ts`）の既定値と適用先、10 記録（`options.ts`・`pause.ts`）の配置・アニメ・音どおり。マウス感度は `AWasamiPlayerCharacter::MouseSensitivity`（`Look` の値に掛ける。本家の `Character.MouseSensitivity` と同じ）に入れ、`DefaultInput.ini` の `AxisConfig` の 0.07 と視点の修飾子には触らない（項目 2 の結果。02 記録）。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードと WebGL 版の記録を読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」。
- [ ] 1. 設定のセーブと適用（音量と難易度の効き先を除く）
  - 作業ブランチ `feature/options-pause` を main から作る。
  - `UWasamiSettingsSaveGame`（本家の `BP_DD_Settings_SaveGame`。項目と既定値は下の「本家の流れ」の CDO。スロット `Settings`）と、その規則の関数（WebGL 版 04 記録の `settings.ts` の `stepValue`・`snap`〈`GridSnap_Float` 1/9〉・`sliderText`・`QUALITY_TEXT`・`DIFFICULTY_TEXT`〈矢印の Clamp は 0..1〉・`gamma` = `MapRangeClamped(b, 0, 1, 1.8, 2.2)`）。置き場はゲームインスタンスか静的な関数（ゲームモードはレベルごとに作り直されるので、本家の `Global Settings Save Instance` はゲームインスタンスに持つのがよい。ステップの中で決める）。
  - 本家の `Check Settings Save`（読めなければ新しく作って書く）→ `Set Settings`（旧版 `BP_DD_GameMode` @33342: `SetOverallScalabilityLevel(Quality)`・`SetResolutionScaleValueEx(× 100)`・`SetViewDistanceQuality(3)`・`SetSubtitlesEnabled`・`SetPostProcessingQuality`〈0..2 → 2、3 → 3〉・`ApplySettings(False)`・`gamma X`）を、ゲームモードとタイトルのゲームモードの BeginPlay で。**エディタ（PIE）ではスケーラビリティと解像度を当てない**かを `.claude/guides/performance.md` に照らして決める（`sg.*` はエディタにも効き、開発用の軽い設定を上書きする）。
  - プレイヤーへの適用: `MouseSensitivity`（下の決定事項の「感度の換算」）・`bInvertY`・`bHeadBob`・`bToggleSprint`、マウスのスムージング（本家の `BP_DD_PlayerCharacter` の `Set Up Mouse Smoothing` を読む。WebGL 版は SpringArm の `CameraRotationLagSpeed` をあり 12.5 / なし 50、BeginPlay では呼ばれず既定 20 のまま）。今は `AWasamiPlayerCharacter` の既定値（感度 1.0 ほか）で動いている。
  - デバッグ（`Wasami.Settings` で今の値を出す、`Wasami.ResetSettings`）とテスト `Wasami.Settings.*`（既定値・吸着と文字・保存と読み直し・プレイヤーへの適用）。
  - 変更予定: `Source/wasami_deception/WasamiSettingsSaveGame.*`（新）、`WasamiGameInstance.*`、`WasamiGameMode.*`、`WasamiTitleGameMode.*`、`WasamiPlayerCharacter.*`、`Tests/WasamiSettingsTests.cpp`（新）、実装記録 06・02（か新しい記録 15）・`_index.md`
- [ ] 2. 音量の SoundClass と SoundMix、難易度の効き先
  - 本家の `Audio/SoundMix/DD_SoundMix`・`DD_SoundClass_Music`・`_SFX`・`_Dialogue`（と `_SFX_UI`・`_SFX_Movies`。親子を JSON で確かめる）を取り込み、取り込み済みの音（`dd_assets.py` は今 `SoundClassObject` を書かない。18 行目のコメント）に本家の SoundClass を付ける（SoundWave と SoundCue の JSON の `SoundClassObject` から。既存の音は取り込み直さずに付け直す道具を足す）。`Set Settings` と SAVE & EXIT で `SetSoundMixClassOverride(DD_SoundMix, 各 Class, 値, 1, 1, False)`（`PushSoundMixModifier` が要るかは UE 5.8 の挙動で確かめる）。
  - 難易度（本家の旧版のメニューで選べるのは EASY / NORMAL。HARD は Clamp で届かない）: スコア画面の `easymode` と FINAL RANK の A 止め（13 記録。今は出さない）、死亡画面の Easy の分岐（`pak_reference_2/.../UMG_DeathScreen.txt` の 694 行あたり。09 記録・`.claude/references/game-flow/README.md` の 78 行は「本作は作らない」としていた）を本家のコードどおりにつなぐ。病院の敵（ナース）は難易度を読まない（`pak_reference_2` で Difficulty を読むのは `BP_Monkey`・`BP_Monkey_Chef`・`BP_03_Watcher`・`BP_Agatha1`・死亡画面・スコア画面・ポーズ・ゲームインスタンスだけ）ので、敵の速さは変えない。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_assets.py`（か `dd_audio` の新しい関数）、`toolsets/dd.py`、`/Game/DD/Audio/SoundMix/…`、`WasamiLevelClearWidget.*`・`WasamiLevelResults.*`・`WasamiGameMode.cpp`（`Escape` の EASY）、`WasamiDeathScreenWidget.*`、テスト、実装記録 01・09・13・06
- [ ] 3. オプション画面の木と素材（`UWasamiOptionsWidget`）
  - 死亡画面・タイトル（09・14 記録）と同じく、旧版 `pak_reference/_assets/.../UI/Menu/UMG_Options.json`（`_bytecode/.../UMG_Options.txt`）の木をスロットのまま C++ で組む: ルート CanvasPanel_0（レンダー変換 1.015）、Blur+Red、CanvasPanel_2（枠 `options-frame`・見出し）、CanvasPanel_1 の GridPanel_0（GRAPHICS / AUDIO / CONTROLS / DIFFICULTY の 4 つの VerticalBox。UMG の `UGridPanel` はそのまま Slate の配置になるので WebGL 版の `slateGrid` は要らない）、SAVE & EXIT / CANCEL。WebGL 版 10 記録の styles.css「OPTIONS」が同じ木を写した値の控え。
  - 素材: 本家のテクスチャ（枠・値の箱・矢印・つまみ・チェック 2 枚。名前は JSON で。WebGL 版は `options-frame.webp` ほか）とフォント（Helvetica Neue Bold。取り込み済みの `/Game/DD/UI/Fonts` を確かめる）を取り込む（`dd_ui.py` に `import_options()`）。
  - `Construct` と `Setup Values`（`UMG_Options.txt` @17146。設定を読んで各部品に入れる）、`FadeIn` のアニメ（書き出しのキー）。テスト `Wasami.Options.Screen`（木の部品と既定値の表示）。
  - 変更予定: `Source/wasami_deception/WasamiOptionsWidget.*`（新）、`Tests/WasamiOptionsTests.cpp`（新）、`Content/Python/wasami_tools/pipeline/dd_ui.py`、`/Game/DD/UI/Menu/…`、実装記録 09（か 15）・01・`_index.md`
- [ ] 4. オプション画面の操作とタイトルの OPTIONS
  - スライダー（`OnValueChanged` で 1/9 に吸着して `SetValue`、値の欄の文字）、QUALITY と DIFFICULTY の矢印（`stepValue`）、チェック、SAVE & EXIT（`UI_Select_V2` → 適用〈@18949〉→ `Save Values`〈@19688: スロットに書き、プレイヤーがいれば `Set Up Mouse Smoothing`〉→ `Cancel`）、CANCEL（@21302: `FadeIn` を 0.25 から逆再生・`UI_Select_V3` の 0.7・0.3 s 後に `RemoveFromParent`）。Esc で CANCEL と同じ（WebGL 版。本家は Esc を結んでいない → 決める）。
  - タイトルの OPTIONS（`UWasamiTitleScreenWidget` の `TODO(項目 18)`）: 本家どおり `CreateAndAddWidget(UMG_Options, Z 10)` と選択音。
  - テスト `Wasami.Options.*`（吸着・矢印の端・SAVE & EXIT で保存と適用・CANCEL で捨てる）。PIE でタイトルから開いて値を変え、SAVE & EXIT → 開き直して値が残るのを確かめる。
  - 変更予定: `WasamiOptionsWidget.*`、`WasamiTitleScreenWidget.cpp`、`Tests/WasamiOptionsTests.cpp`、実装記録 14・09（か 15）
- [ ] 5. ポーズ画面の木・アニメ・音と開き方（`UWasamiPauseWidget`）
  - 旧版 `pak_reference/_assets/.../UI/Menu/Pause/UMG_Pause.json` の木（Blur+Red → 帯 Image_152・頭 Icon・EASY MODE TextBlock_1・メニュー VerticalBox_113〈RESUME / RESTART / OPTIONS / QUIT〉→ CanvasPanel_3 → Givingupbox → RestartBox）をスロットのまま C++ で。頭は本家の病院の `pause_reapernurse_head`（キャラクター）でなく本作のワサミ（WebGL 版の `<WEBGL>/public/title/pause-head.webp`、のぞく頭は `pause-peek.webp`。前処理で PNG にして `SourceArt/Wasami/UI/` へ。本家の頭の赤 `rgb(192, 0, 0)` で塗る）。帯・枠は本家のテクスチャ（`restart_window_frame_2`・`quit_window_frame` は取り込み済み、帯は JSON で名前を確かめて取り込む）。
  - `Construct`（`SetGamePaused(True)`・UI の入力とカーソル・`UI_Pause`・曲 `Pause_Sound_v1` を FadeIn・`FadeIn` のアニメ）と `Destruct`（曲を 0.5 s で消す）、RESUME（`FadeIn` の逆再生・0.5 s 後に `SetGamePaused(False)`・ゲームの入力・`RemoveFromParent`）。EASY MODE は難易度が EASY のときだけ見える（本家は色の結び付け）。
  - 開き方: 本家の旧版はキャラクターの Esc で `CreateAndAddWidget(UMG_Pause, Z 5)`、最新版はプレイヤーコントローラーの Esc と Gamepad Special Left で Z 1。Enhanced Input にポーズの入力を足す。**死亡画面・捕獲・スコア画面・読み込み画面・欠片の画面・タイトル・ポーズ中は開かない**かを本家のコードで確かめる（本家は Esc で毎回作る。ゲームが止まっている間に Esc が届くか・`SetGamePaused` の下で入力がどうなるかを読む）。**PIE では Esc がエディタの「プレイを止める」に取られる**ので、確かめはデバッグ `Wasami.Pause` か、エディタのキー割り当てを見て決める（症状索引に書く）。
  - テスト `Wasami.Pause.*`（木・アニメのキー・音・RESUME で解ける）。
  - 変更予定: `Source/wasami_deception/WasamiPauseWidget.*`（新）、`WasamiPlayerCharacter.*`（か新しいプレイヤーコントローラー）、`Tests/WasamiPauseTests.cpp`（新）、`Tools/dd/prepare_title.py`（か新しい前処理）、`SourceArt/Wasami/UI/pause_*.png`（新）、`dd_ui.py`、`/Game/DD/UI/Menu/Pause/…`・`/Game/Wasami/UI/Pause/…`、実装記録 09（か 15）・02・01
- [ ] 6. ポーズのボタンの道と通しの確かめ
  - RESTART → RESTART?（`Popup_0`・選択音とウィンドウの音）→ YES: 本家どおりセーブの病院の欄を空の欄にして書き、今のレベルを開く（旧版 @5158: 選択音 → `SetGamePaused(False)` → ゲームの入力 → `Hard Check Point` 0 → `OpenLevel`。最新版は回収の記憶〈`Shards To Be Removed`〉も空にし、`UMG_BlackFade_2`〈Speed 5、Z 10〉の後に開く。下の決定事項）。死亡画面の `RestartEvent`（09 記録。Zone 2 でチェックポイント 0 なら Zone 1 の到着）と同じ道を使う。NO（再生速度 0.7 の選択音と `Popup_0` の逆再生）。
  - OPTIONS → `UWasamiOptionsWidget` を Z 10 で（ポーズの上）。QUIT → GIVING UP?（`Popup`）→ QUIT TO TITLE（セーブはそのまま、タイトル `L_Title`）/ QUIT TO DESKTOP（`QuitGame`。WebGL 版はブラウザなので置かなかった）/ CANCEL。
  - デバッグ `Wasami.Pause`、台本 `Tools/playthrough.py` に区間 `pause`（Zone 1 でポーズ → OPTIONS で感度を変えて SAVE & EXIT → RESUME で視点の速さが変わる → ポーズ → QUIT → QUIT TO TITLE → RESUME で続き）。PIE で通し、`--record` の連番のグリッドを Discord に（ポーズの FadeIn・OPTIONS・GIVING UP?）。
  - 変更予定: `WasamiPauseWidget.*`、`WasamiDeathScreenWidget.*`（道を共有するなら）、`WasamiGameMode.*`、`Tests/WasamiPauseTests.cpp`、`Tools/playthrough.py`、実装記録 09（か 15）・06・01
- [ ] 7. 閉じる
  - 作業一覧の項目 18 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順にポーズとオプションを足す）を直す。note の原稿に「オプションとポーズ」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 1 を始める。`git switch -c feature/options-pause`。`.claude/guides/performance.md` の「大原則」と、`UWasamiSaveGame`・`UWasamiGameInstance`（06 記録）・`AWasamiPlayerCharacter` の BeginPlay と SpringArm（02 記録）を読み、本家の旧版 `BP_DD_PlayerCharacter.txt` の `Set Up Mouse Smoothing` を `python Tools/dd/bp_flow.py` で読んでから `UWasamiSettingsSaveGame` を書く。

## 本家の流れ（読んだもの）

- **設定のセーブ** `BP_DD_Settings_SaveGame`（`_assets/DDeception/Content/Blueprints/Save/BP_DD_Settings_SaveGame.json`）の CDO: `Quality` 2・`Resolution Scale` 1・`Brightness` 1・`Music` / `SFX` / `Dialogue` 1・`Subtitles` 真・`Mouse Sensitivity` 0.5・`Head Bobbing` 真・`Crosshair` 真（メニューに無い）・`Mouse Smoothing` 真・`Difficulty` `NewEnumerator1`（NORMAL）。`Inverted Y Axis`・`Toggle Sprint` は CDO に無い = 偽。最新版は `VSync`・`Motion Blur`（真）が増える。
- **旧版の読み込みと適用**（`pak_reference/_bytecode/.../Blueprints/Main/BP_DD_GameMode.txt`）: `Check Settings Save` @33036（`LoadGameFromSlot('Settings')`、無ければ `CreateSaveGameObject` → 書く）→ `Set Settings` @33342（上のステップ 1 の一覧と、`SetSoundMixClassOverride(DD_SoundMix, DD_SoundClass_Music / SFX / Dialogue, 値, 1, 1, False)`）。
- **最新版**（`pak_reference_2`）は `UMG_Options` が無く、AutoSettings プラグインの `SettingsUI`（`UI/SettingsUI`・`UI/Pages/VideoSettingsPage`・`InputSettingsPage`）と CVar（`Game.Difficulty`・`GameAudio_Master` / `_Music`・`Gamma`・`GameAudio_Subtitles`。`BP_DD_GameInstance`）に替わった。スロットは `settingsSlot`、`Set Settings` は空。ポーズの OPTIONS も `SettingsUI` を開く。
- **旧版 `UMG_Options`**（`pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt`）: `Construct` @21404、`Setup Values` @17146、`Cast To GameMode` @18811、SAVE & EXIT（`ApplyButton`）@19624 = `UI_Select_V2`(1) → 適用 @18949（`SetOverallScalabilityLevel(QualitySetting)`・`SetResolutionScaleValueEx(× 100)`・`SetViewDistanceQuality(3)`・`SetPostProcessingQuality`・`SetSubtitlesEnabled`・`ApplySettings(False)` → @16506 の `gamma` と SoundMix 3 つ）→ `Save Values` @19688（`LoadGameFromSlot('Settings')` に全項目を書き、ゲームモードの `Global Settings Save Instance` にして `SaveGameToSlot('Settings')`、プレイヤーがいれば `Set Up Mouse Smoothing`）→ `Cancel` @21302（`PlayAnimation(FadeIn, 0.25, 1, Reverse, 1)`・`UI_Select_V3`(1, 0.7)・`Delay 0.3` → `RemoveFromParent`）。スライダーの OnValueChanged: MouseSensitivity @16380・Brightness @16254・Dialogue @642・Music @516・SFX @390・ResolutionScale @30。矢印: Quality @156 / @273、Difficulty @22041 / @21782。
- **ポーズ**（`UI/Menu/Pause/UMG_Pause.txt`。両版にある）: 旧版はキャラクターの `InpActEvt_Escape` @7758 で `CreateAndAddWidget(UMG_Pause, Z 5)`、最新版は `DD_PlayerController` の Escape @746 と Gamepad Special Left で Z 1。ボタン: RESUME・RESTART・OPTIONS・QUIT、ポップアップの YES / NO（RESTART?）・QUIT TO TITLE / QUIT TO DESKTOP / CANCEL（GIVING UP?）。RESTART の YES: 旧版 @5158 = `levelStruct[レベル] = levelStruct[10]` → `SaveGameToSlot('structSlot')` → `UI_Select_V3` → `SetGamePaused(False)` → `SetInputMode_GameOnly` → カーソルを消す → `Hard Check Point` 0 → `OpenLevel(今のレベル)`。最新版 @5408 は同じ保存の後に `Shards To Be Removed` と `Sewer Doors Opened` を空にし、`SetGamePaused(False)` を `UMG_BlackFade_2`（Speed 5、Z 10）の `Animation Finished` → `Finish Restart` @6656 に移す。頭の絵は `Get Level` でレベルごと（病院は `pause_reapernurse_head`、のぞく頭は `quit_window_head_reapernurse`）。最新版の木には `pause_screen_bg` が増える。
- **難易度を読むもの**（`pak_reference_2`）: `UMG_LevelClear`・`BP_Monkey`・`BP_Monkey_Chef`・`BP_03_Watcher`・`BP_Agatha1`・`BP_DD_GameInstance`（CVar）・`UMG_Pause`・`UMG_DeathScreen`・`UMG_DeathScreen_07_Fake`。病院のナースは読まない。
- **WebGL 版**（`.claude/references/webgl/implementation-records/04-game-state-save.md` の `settings.ts`、`10-hud-tablet.md` の `pause.ts`・`options.ts`・styles.css「OPTIONS」「ポーズ画面」、`01-boot-and-config.md` の「OPTIONS の結線」「ポーズ画面の結線」）: 旧版 v1.6.1 の木を写した。ポーズはポインタロックの喪失で開き、QUIT TO DESKTOP は置かなかった。OPTIONS の Esc は CANCEL。素材は `<WEBGL>/public/title/`（`options-*.webp`・`pause-*.webp`。`<WEBGL>` = `C:\Users\User\Downloads\wasami-deseption`）。

## 決定事項

- 2026-09-19: **オプション画面は旧版（v1.6.1）の `UMG_Options` を写す** — 最新版に `UMG_Options` は無く、AutoSettings プラグイン（第三者のコード。無人運転では組み込まない）の `SettingsUI` に替わった。作業一覧の項目 18 の完了の条件（WebGL 版 04・10 記録どおり）も旧版を写したもの。タイトル（項目 17）も旧版。設定のセーブの項目は両版で同じ（最新版の `VSync`・`Motion Blur` は旧版のメニューに無いので持たない）。
- 2026-09-19: **ポーズ画面の木・アニメ・音は旧版（WebGL 版と同じ）、ボタンの道の規則は両版を比べ、違えば最新版を仮に採って要確認に書く**（`.claude/guides/autonomy.md` の「決め方」）— 完了の条件が WebGL 版 10 記録の配置・アニメ・音。ただし頭と のぞく頭はキャラクターなので本作のワサミ（WebGL 版と同じ）。
- 2026-09-19: **感度の換算は、ステップ 1 で「`MouseSensitivity` = 設定の値 ÷ 0.5」（WebGL 版の `lookScale`）を第一の案にする**（要確認に書く）— 旧版の既定 0.5 をそのまま掛けると、項目 2 で本家の最新版の実機（感度 1、0.175°/カウント。`observations/README.md` の「視点の速さ」）に合わせた今の速さが半分になる。
- 2026-09-19: **エディタで動く確かめ（PIE）はポーズを Esc でなく `Wasami.Pause` で開く**ことを第一の案にする — PIE の Esc はエディタの「プレイを止める」。ステップ 5 でキー割り当てを見て決める。

## 要確認（ユーザー）

（なし。ステップ 1 で感度の換算を書く）

## 再開時の注意

- エディタ・PIE・バックグラウンドの処理は、この計画の反復では触っていない。

## 検証

- check_records: 未実行（ソースは変えていない）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
