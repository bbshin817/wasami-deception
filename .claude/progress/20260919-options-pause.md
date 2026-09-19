---
title: オプション画面とポーズ画面（作業一覧の項目 18）
status: 進行中
branch: feature/options-pause
base: d7ae8be
started: 2026-09-19 12:03
updated: 2026-09-19 13:15
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# オプション画面とポーズ画面（作業一覧の項目 18）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 18（大目標 2）。最終目標の「WebGL版と同じように倣う点: オプション画面・ポーズ画面」。`UMG_Options`（画質・解像度・明るさ・音量 3 種・字幕・マウス感度・頭の揺れ・Y 反転・ダッシュの切り替え・マウスのスムージング・難易度。設定のセーブ `BP_DD_Settings_SaveGame`）と `UMG_Pause`（Esc）を WebGL 版の記録どおりに作る。完了の条件: WebGL 版 04 記録（`settings.ts`）の既定値と適用先、10 記録（`options.ts`・`pause.ts`）の配置・アニメ・音どおり。マウス感度は `AWasamiPlayerCharacter::MouseSensitivity`（`Look` の値に掛ける。本家の `Character.MouseSensitivity` と同じ）に入れ、`DefaultInput.ini` の `AxisConfig` の 0.07 と視点の修飾子には触らない（項目 2 の結果。02 記録）。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードと WebGL 版の記録を読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」。
- [x] 1. 設定のセーブと適用 … 2026-09-19 完了。`UWasamiSettingsSaveGame`（項目・既定値・規則）、ゲームインスタンスが持つ（`CheckSettingsSave`・`GetSettings`・`SaveSettings`）、両ゲームモードの BeginPlay で読んで当てる（スケーラビリティはパッケージだけ）、プレイヤーの `ApplySettings`・`SetUpMouseSmoothing`、`Wasami.Settings`・`Wasami.ResetSettings`、テスト `Wasami.Settings.*` 4 件。中身は実装記録 15。
- [x] 2. 音量の SoundClass と SoundMix、難易度の効き先 … 2026-09-19 完了。`DD_SoundMix` と SoundClass 5 つを取り込み、取り込み済みの音 62 件に本家のクラスを付けた（`import_dd_sound_classes`。01 記録）。`Apply(WorldContext)` の `SetSoundMixClassOverride` と `CheckSettingsSave` の `SetBaseSoundMix`、`UWasamiGameInstance::IsEasy()` をスコア画面へ（15・13 記録）。死亡画面の Easy の分岐は作らない（要確認）。
- [x] 3. オプション画面の木と素材 … 2026-09-19 完了。`UWasamiOptionsWidget`（旧版 `UMG_Options` の木をスロットのまま・Construct・`Setup Values`・値の箱の結び付け・FadeIn。タイトルの外では DIFFICULTY を外す）、`dd_ui.import_options`（7 枚）、テスト `Wasami.Options.Screen`・`FadeIn`。中身は実装記録 15。
- [ ] 4. オプション画面の操作とタイトルの OPTIONS
  - スライダー（`OnValueChanged` で 1/9 に吸着して `SetValue`。値の欄の文字は画面がティックごとに読むので要らない）、QUALITY と DIFFICULTY の矢印（`stepValue`）、チェック、SAVE & EXIT（`UI_Select_V2` → 適用〈@18949〉→ `Save Values`〈@19688: スロットに書き、プレイヤーがいれば `Set Up Mouse Smoothing`〉→ `Cancel`）、CANCEL（@21302: `FadeIn` を 0.25 から逆再生・`UI_Select_V3` の 0.7・0.3 s 後に `RemoveFromParent`）。Esc で CANCEL と同じ（WebGL 版。本家は Esc を結んでいない → 決める）。
  - SAVE & EXIT と CANCEL のホバー（@15864〜@16040: 入ると中身の色が白、出ると `Unhovered Color` 0.1146。死亡画面・ポップアップと同じ）。
  - SAVE & EXIT は画面の値を `Settings`（`Begin` が持つ設定。ゲームインスタンスのものならそのまま）に書いてから `UWasamiGameInstance::SaveSettings()`。ゲームインスタンスの無いテストでは `UWasamiSettingsSaveGame` を別のスロットに書く道を用意する（`Wasami.Settings.Slot` の作りに倣う）。
  - タイトルの OPTIONS（`UWasamiTitleScreenWidget` の `TODO(項目 18)`）: 本家どおり `CreateAndAddWidget(UMG_Options, Z 10)` と選択音。
  - テスト `Wasami.Options.*`（吸着・矢印の端・SAVE & EXIT で保存と適用・CANCEL で捨てる）。PIE でタイトルから開いて値を変え、SAVE & EXIT → 開き直して値が残るのを確かめる。
  - 変更予定: `WasamiOptionsWidget.*`、`WasamiTitleScreenWidget.cpp`、`Tests/WasamiOptionsTests.cpp`、`Tests/WasamiTitleScreenTests.cpp`、実装記録 14・15
- [ ] 5. ポーズ画面の木・アニメ・音と開き方（`UWasamiPauseWidget`）
  - 旧版 `pak_reference/_assets/.../UI/Menu/Pause/UMG_Pause.json` の木（Blur+Red → 帯 Image_152・頭 Icon・EASY MODE TextBlock_1・メニュー VerticalBox_113〈RESUME / RESTART / OPTIONS / QUIT〉→ CanvasPanel_3 → Givingupbox → RestartBox）をスロットのまま C++ で。頭は本家の病院の `pause_reapernurse_head`（キャラクター）でなく本作のワサミ（WebGL 版の `<WEBGL>/public/title/pause-head.webp`、のぞく頭は `pause-peek.webp`。前処理で PNG にして `SourceArt/Wasami/UI/` へ。本家の頭の赤 `rgb(192, 0, 0)` で塗る）。帯・枠は本家のテクスチャ（`restart_window_frame_2`・`quit_window_frame` は取り込み済み、帯は JSON で名前を確かめて取り込む）。
  - `Construct`（`SetGamePaused(True)`・UI の入力とカーソル・`UI_Pause`・曲 `Pause_Sound_v1` を FadeIn・`FadeIn` のアニメ）と `Destruct`（曲を 0.5 s で消す）、RESUME（`FadeIn` の逆再生・0.5 s 後に `SetGamePaused(False)`・ゲームの入力・`RemoveFromParent`）。EASY MODE は難易度が EASY のときだけ見える（本家は色の結び付け。`UWasamiGameInstance::IsEasy()`）。
  - 開き方: 本家の旧版はキャラクターの Esc で `CreateAndAddWidget(UMG_Pause, Z 5)`、最新版はプレイヤーコントローラーの Esc と Gamepad Special Left で Z 1。Enhanced Input にポーズの入力を足す。**死亡画面・捕獲・スコア画面・読み込み画面・欠片の画面・タイトル・ポーズ中は開かない**かを本家のコードで確かめる（本家は Esc で毎回作る。ゲームが止まっている間に Esc が届くか・`SetGamePaused` の下で入力がどうなるかを読む）。**PIE では Esc がエディタの「プレイを止める」に取られる**ので、確かめはデバッグ `Wasami.Pause` か、エディタのキー割り当てを見て決める（症状索引に書く）。
  - テスト `Wasami.Pause.*`（木・アニメのキー・音・RESUME で解ける）。
  - 変更予定: `Source/wasami_deception/WasamiPauseWidget.*`（新）、`WasamiPlayerCharacter.*`（か新しいプレイヤーコントローラー）、`Tests/WasamiPauseTests.cpp`（新）、`Tools/dd/prepare_title.py`（か新しい前処理）、`SourceArt/Wasami/UI/pause_*.png`（新）、`dd_ui.py`、`/Game/DD/UI/Menu/Pause/…`・`/Game/Wasami/UI/Pause/…`、実装記録 09（か 15）・02・01
- [ ] 6. ポーズのボタンの道と通しの確かめ
  - RESTART → RESTART?（`Popup_0`・選択音とウィンドウの音）→ YES: 本家どおりセーブの病院の欄を空の欄にして書き、今のレベルを開く（旧版 @5158: 選択音 → `SetGamePaused(False)` → ゲームの入力 → `Hard Check Point` 0 → `OpenLevel`。最新版は回収の記憶〈`Shards To Be Removed`〉も空にし、`UMG_BlackFade_2`〈Speed 5、Z 10〉の後に開く。下の決定事項）。死亡画面の `RestartEvent`（09 記録。Zone 2 でチェックポイント 0 なら Zone 1 の到着）と同じ道を使う。NO（再生速度 0.7 の選択音と `Popup_0` の逆再生）。
  - OPTIONS → `UWasamiOptionsWidget` を Z 10 で（ポーズの上。ゲームの中では本家どおり DIFFICULTY の箱が外れる。15 記録）。QUIT → GIVING UP?（`Popup`）→ QUIT TO TITLE（セーブはそのまま、タイトル `L_Title`）/ QUIT TO DESKTOP（`QuitGame`。WebGL 版はブラウザなので置かなかった）/ CANCEL。
  - デバッグ `Wasami.Pause`、台本 `Tools/playthrough.py` に区間 `pause`（Zone 1 でポーズ → OPTIONS で感度を変えて SAVE & EXIT → RESUME で視点の速さが変わる → ポーズ → QUIT → QUIT TO TITLE → RESUME で続き）。PIE で通し、`--record` の連番のグリッドを Discord に（ポーズの FadeIn・OPTIONS・GIVING UP?）。
  - 変更予定: `WasamiPauseWidget.*`、`WasamiDeathScreenWidget.*`（道を共有するなら）、`WasamiGameMode.*`、`Tests/WasamiPauseTests.cpp`、`Tools/playthrough.py`、実装記録 09（か 15）・06・01
- [ ] 7. 閉じる
  - 作業一覧の項目 18 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順にポーズとオプションを足す）を直す。note の原稿に「オプションとポーズ」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 4 を始める（ブランチ `feature/options-pause`）。`python Tools/dd/bp_flow.py pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt <番地>` でスライダーの OnValueChanged（@30・@390・@516・@642・@16254・@16380）、矢印（@156・@273・@21782・@22041）、SAVE & EXIT（@19624 → 適用 @18949 → `Save Values` @19688）、CANCEL（@16082 → `Cancel` @21302）、ホバー（@15864〜@16040）を読み、`UWasamiOptionsWidget`（実装記録 15 の「オプション画面」）に足す。タイトルの `PressOptions` から開く。

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
- 2026-09-19: **オプション画面（ステップ 3・4）は、読んだ設定（`UWasamiGameInstance::GetSettings()`）を画面の値で書き換えてから `SaveSettings()` を呼ぶ**（本家の SAVE & EXIT = 適用 → `Save Values`。ステップ 1 で作った道）。CANCEL は書き換えない（画面は値を自分で持ち、SAVE & EXIT のときだけ設定へ写す）。
- 2026-09-19: **エディタで動く確かめ（PIE）はポーズを Esc でなく `Wasami.Pause` で開く**ことを第一の案にする — PIE の Esc はエディタの「プレイを止める」。ステップ 5 でキー割り当てを見て決める。

## 要確認（ユーザー）

- 死亡画面の EASY（2026-09-19、ステップ 2）: 最新版の死亡画面はライフ 0 で難易度が EASY だと、ライフの絵が消えるだけでゲームオーバーにならず、ボタンも出ない（ポーズからしか抜けられない）。旧版の死亡画面は難易度を読まない。オプション画面を写す旧版に合わせて**作らなかった**（EASY でもライフ 0 でゲームオーバー）。最新版どおりにする方がよければ直す。
- SFX の音量が効かない音（2026-09-19、ステップ 2）: 本家どおり、音のクラスは本家の書き出しのまま付けたので、本家でクラスの無い病院の音 21 件（扉・リフト・エレベーター・鍵開け・針の罠など）は SFX のスライダーで小さくならない。UI の音（SFX_UI）も本家どおり SFX のスライダーが効かない。すべての効果音に効かせる方がよければ直す。
- マウス感度の換算（2026-09-19、ステップ 1）: 本家の旧版はマウスの軸に設定の値（既定 0.5）をそのまま掛けるが、本作の視点の速さは項目 2 で最新版の実機を感度 1 で測って合わせてある。そのまま掛けると既定で今の半分の速さになるので、**設定 ÷ 0.5 を掛ける**（既定で今の速さ、最大 1 で 2 倍。WebGL 版の `lookScale` と同じ）と仮に決めた。本家どおり設定の値そのものを掛ける（既定で今の半分）方がよければ直す。

## 再開時の注意

- 本家のウィジェットの木を読むのは `python tmp/umg_tree.py <pak_reference の …/UMG_X.json>`（ステップ 3 で作った使い捨て。git の外なので、無ければ作り直す: `WidgetTree` の側の部品を `Slots` → `Content` でたどり、`Slot`・`Parent` を除いた props を並べる）。ステップ 5 の `UMG_Pause` にも使う。

- 開発用の設定のセーブ `Saved/SaveGames/Settings.sav` は既定値（ステップ 1 の確かめの後に書き直した）。`Wasami.Settings` で値を変えたら `Wasami.ResetSettings` で戻す。
- テストをリモート実行で走らせるときは、エディタを前面にする（`python Tools/desktop.py click <タイトルバーの空き> --allow WindowsTerminal.exe --allow UnrealEditor.exe`。2026-09-19 はエディタが右半分にあり、タイトルバーの空きは (2800, 82)。起動時に浮いて出るメッセージログの窓は閉じてよい）。PIE でオプション画面だけを出すのは `L_Title` を開いて `python Tools/pie.py start` の後、リモート実行の `unreal.WasamiOptionsWidget.show(<ゲームのワールド>)`（止めたら Zone 1 を開き直す）。

## 検証

- ステップ 1: check_records OK。C++ ビルド OK（`WasamiVanishWidget.cpp` の定数がユニティの塊でぶつかったので `VanishTicksPerSecond` に改め、無名名前空間の名前の重複を洗い出した〈多重定義の `Place` だけ〉）。テスト `Wasami.Settings`・`Title`・`GameFlow`・`Capture` の 18 件が通った。PIE（Zone 1）で `Settings.sav` が作られ、プレイヤーは感度 1.0・ラグ 20 で始まり、`Wasami.Settings MouseSensitivity 1` で 2.0、`bMouseSmoothing False` でラグ 50、`Brightness 0.5` で画面の平均の明るさが 56.5 → 48.9 になり、PIE を止めるとエディタの明るさが戻った（`gamma` の前後で同じ）。
- ステップ 2: check_records OK。C++ ビルド OK。テスト `Wasami.Settings`（新しい `SoundMix` を含む）・`LevelClear`・`GameFlow`・`Title`・`ZoneFlow`・`DeathScreen` の 31 件が通った。PIE（Zone 1）で `Wasami.Settings Music 0.3`・`SFX 0.5` の後、音の装置の Music と SFX のクラスの音量が約 1 s で 0.300・0.500 になり、Dialogue は 1 のまま。`Wasami.ResetSettings` で既定に戻した。
- ステップ 3: check_records OK。C++ ビルド OK（ラムダの引数 `Padding` が `UUserWidget::Padding` を隠して C4458 になったので改名）。テスト `Wasami.Options`（新しい `Screen`・`FadeIn`）・`Settings`・`Title`・`DeathScreen` の 18 件が通った。PIE（`L_Title`）で画面を出し、本家の旧版と同じ配置（左に GRAPHICS・AUDIO、右に DIFFICULTY・CONTROLS、下に SAVE & EXIT・CANCEL）と既定値の表示を撮って確かめた。
