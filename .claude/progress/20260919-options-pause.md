---
title: オプション画面とポーズ画面（作業一覧の項目 18）
status: 進行中
branch: feature/options-pause
base: d7ae8be
started: 2026-09-19 12:03
updated: 2026-09-19 16:05
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# オプション画面とポーズ画面（作業一覧の項目 18）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 18（大目標 2）。最終目標の「WebGL版と同じように倣う点: オプション画面・ポーズ画面」。`UMG_Options`（画質・解像度・明るさ・音量 3 種・字幕・マウス感度・頭の揺れ・Y 反転・ダッシュの切り替え・マウスのスムージング・難易度。設定のセーブ `BP_DD_Settings_SaveGame`）と `UMG_Pause`（Esc）を WebGL 版の記録どおりに作る。完了の条件: WebGL 版 04 記録（`settings.ts`）の既定値と適用先、10 記録（`options.ts`・`pause.ts`）の配置・アニメ・音どおり。マウス感度は `AWasamiPlayerCharacter::MouseSensitivity`（`Look` の値に掛ける。本家の `Character.MouseSensitivity` と同じ）に入れ、`DefaultInput.ini` の `AxisConfig` の 0.07 と視点の修飾子には触らない（項目 2 の結果。02 記録）。大目標 1・2 の決め方（見た目を本家と見比べて詰めない）で進める。

## 計画

- [x] 0. 本家のコードと WebGL 版の記録を読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」。
- [x] 1. 設定のセーブと適用 … 2026-09-19 完了。`UWasamiSettingsSaveGame`（項目・既定値・規則）、ゲームインスタンスが持つ（`CheckSettingsSave`・`GetSettings`・`SaveSettings`）、両ゲームモードの BeginPlay で読んで当てる（スケーラビリティはパッケージだけ）、プレイヤーの `ApplySettings`・`SetUpMouseSmoothing`、`Wasami.Settings`・`Wasami.ResetSettings`、テスト `Wasami.Settings.*` 4 件。中身は実装記録 15。
- [x] 2. 音量の SoundClass と SoundMix、難易度の効き先 … 2026-09-19 完了。`DD_SoundMix` と SoundClass 5 つを取り込み、取り込み済みの音 62 件に本家のクラスを付けた（`import_dd_sound_classes`。01 記録）。`Apply(WorldContext)` の `SetSoundMixClassOverride` と `CheckSettingsSave` の `SetBaseSoundMix`、`UWasamiGameInstance::IsEasy()` をスコア画面へ（15・13 記録）。死亡画面の Easy の分岐はステップ 6 で最新版どおりに作る（ユーザーの回答）。
- [x] 3. オプション画面の木と素材 … 2026-09-19 完了。`UWasamiOptionsWidget`（旧版 `UMG_Options` の木をスロットのまま・Construct・`Setup Values`・値の箱の結び付け・FadeIn。タイトルの外では DIFFICULTY を外す）、`dd_ui.import_options`（7 枚）、テスト `Wasami.Options.Screen`・`FadeIn`。中身は実装記録 15。
- [x] 4. オプション画面の操作とタイトルの OPTIONS … 2026-09-19 完了。スライダーの 1/9 の吸着（`Setup Values` の間は吸着させない）・矢印・SAVE & EXIT（`WriteValues` → 持ち主 `SettingsOwner` の `SaveSettings`）・CANCEL（FadeIn の逆再生、0.3 s で外れる）・ホバー、`UI_Select_V2` の取り込み、タイトルの OPTIONS、テスト `Wasami.Options.Controls`・`SaveAndCancel`。中身は実装記録 15。閉じた後は入力の様式を戻さない（本家どおり）ので、**ポーズの上で閉じたときの焦点・Esc はステップ 5 で見る**。
- [x] 5. ポーズ画面の木・アニメ・音と開き方 … 2026-09-19 完了。`UWasamiPauseWidget`（本家の `UMG_Pause` の木をスロットのまま・Construct・FadeIn・RESUME・Destruct・EASY MODE とホバーの色。頭は本作のワサミ `T_PauseHead`・`T_PausePeek` を赤で塗る）、`dd_ui.import_pause`、プレイヤーの Esc（`IA_Escape` → `EscapePressed`: 止まっていなければ Z 5 で開く）とデバッグ `Wasami.Pause`、テスト `Wasami.Pause.*` 3 件。ポップアップ 2 つ（`Givingupbox`・`RestartBox`）と 2 枚目の幕 `CanvasPanel_3` は木にあり、拡大 0・不透明度 0 のまま。中身は実装記録 15。
- [ ] 6. ポーズのボタンの道と通しの確かめ
  - RESTART → RESTART?（`Popup_0`・選択音とウィンドウの音）→ YES: 本家どおりセーブの病院の欄を空の欄にして書き、今のレベルを開く（旧版 @5158: 選択音 → `SetGamePaused(False)` → ゲームの入力 → `Hard Check Point` 0 → `OpenLevel`。最新版は回収の記憶〈`Shards To Be Removed`〉も空にし、`UMG_BlackFade_2`〈Speed 5、Z 10〉の後に開く。下の決定事項）。死亡画面の `RestartEvent`（09 記録。Zone 2 でチェックポイント 0 なら Zone 1 の到着）と同じ道を使う。NO（再生速度 0.7 の選択音と `Popup_0` の逆再生）。
  - OPTIONS → `UWasamiOptionsWidget` を Z 10 で（ポーズの上。ゲームの中では本家どおり DIFFICULTY の箱が外れる。15 記録）。QUIT → GIVING UP?（`Popup`）→ QUIT TO TITLE（セーブはそのまま、タイトル `L_Title`）/ QUIT TO DESKTOP（`QuitGame`。WebGL 版はブラウザなので置かなかった）/ CANCEL。
  - ポップアップのアニメ `Popup`（`Givingupbox`）・`Popup_0`（`RestartBox`）は同じキー: 窓の拡大 0 → 1（0.25 s、自動の接線 3.33e-5/tick で少し行き過ぎる）→ 1（0.5 s）と不透明度 0 → 1（0.25 s）、`Blur+Red` の不透明度 1 → 0・`CanvasPanel_3` の不透明度 0 → 1（どちらも 0.25 s）。再生範囲は [0, 30001)。オプション画面の FadeIn と同じ値（`UMG_PopUp` の `Popup`）。`redblock` の見え方の切り替え（開くと Visible でメニューのクリックを遮る）は各ボタンの流れで読む。
  - **死亡画面の EASY の分岐（最新版に倣う。2026-09-19 のユーザーの回答）**: 最新版 `pak_reference_2/_bytecode/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.txt` の @2354〜: `Local Lives`（Construct の `Decrement Lives` の後。`Clamp(Lives − 1, 0, 6)` なので EASY でも 0 まで減る）が 0 なら `Fade In` → `Get Lives` が 0 で `Global Settings Save Instance.Difficulty == 0`（`ENUM_DifficultySettings` の 0 = EASY）なら `Life Animation`（@46420。残りのライフの絵を 1 s ごとに外して `Shake`）だけ → 6 s 後の DoOnce（@2743 → @2778）が `Get Lives > 0` で止まる。ゲームオーバーの音・`Death` のアニメ・`SetInputMode_UIOnlyEx`・ボタン（@3104 → @354 → @972）は出ず、`Proceed`（声の終わり @2934）は `Local Lives` が 0 でないときだけ結ぶので開き直しもしない。抜けるのはポーズの RESTART / QUIT だけ。NORMAL は今のまま。本作の `UWasamiDeathScreenWidget`（09 記録）のライフ 0 の分岐に `UWasamiGameInstance::IsEasy()` を足す。**本家のコードどおりだとここでもポーズは開かない**（ステップ 5 で読んだ: 死亡画面の間はレベルの `DeathEvent` の `SetGamePaused(true)` で止まっていて、Esc の結び付けは止まっている間は動かない）ので、`AWasamiPlayerCharacter::EscapePressed` の「止まっていれば開かない」に、EASY でライフ 0 の死亡画面が出ているときだけの例外を足す（死亡画面に待っていることを問う関数を足す。入力の様式はゲームのままなので Esc は届く。`IA_Escape` は止まっている間も起きる）。ポーズは Z 5 で死亡画面（Z 5）より後に足すので上に出る。テスト `Wasami.DeathScreen.Easy*`（EASY でライフ 0 ならボタンが出ず開き直さない、NORMAL は今のまま）。PIE で EASY・ライフ 1 から死に、ポーズの RESTART で抜ける。
  - デバッグ `Wasami.Pause`、台本 `Tools/playthrough.py` に区間 `pause`（Zone 1 でポーズ → OPTIONS で感度を変えて SAVE & EXIT → RESUME で視点の速さが変わる → ポーズ → QUIT → QUIT TO TITLE → RESUME で続き）。PIE で通し、`--record` の連番のグリッドを Discord に（ポーズの FadeIn・OPTIONS・GIVING UP?）。
  - 変更予定: `WasamiPauseWidget.*`、`WasamiDeathScreenWidget.*`（EASY の分岐と、道を共有するなら）、`WasamiGameMode.*`、`Tests/WasamiPauseTests.cpp`、`Tests/WasamiDeathScreenTests.cpp`、`Tools/playthrough.py`、実装記録 09・15・06・01
- [ ] 7. 閉じる
  - 作業一覧の項目 18 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順にポーズとオプションを足す）を直す。note の原稿に「オプションとポーズ」の節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 6 を始める（ブランチ `feature/options-pause`）。旧版 `pak_reference/_bytecode/DDeception/Content/UI/Menu/Pause/UMG_Pause.txt` の RESTART @3996・YES @5158・NO @6080・OPTIONS @6085・QUIT @4516・QUIT TO TITLE @6207・QUIT TO DESKTOP @4843・CANCEL @4709 を `python Tools/dd/bp_flow.py <file> <番地>` で読み（最新版 `pak_reference_2` の同じファイルの YES @5408・`Finish Restart` @6656 と比べる）、`UWasamiPauseWidget`（実装記録 15 の「ポーズ画面」）にポップアップのアニメとボタンの道を足す。死亡画面の EASY の分岐は `UWasamiDeathScreenWidget`（09 記録）と `AWasamiPlayerCharacter::EscapePressed`（02 記録）。大きければ 6a（ポップアップとボタンの道）・6b（死亡画面の EASY と通しの確かめ）に分ける。

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
- 2026-09-19（ユーザーの回答）: **マウス感度は設定 ÷ 0.5 を掛ける**（ステップ 1 の仮を確定。既定 0.5 で今の速さ、1 で 2 倍）。
- 2026-09-19（ユーザーの回答）: **音のクラスは本家の書き出しのまま**（本家でクラスの無い病院の音 21 件と `SFX_UI` に SFX のスライダーが効かないのも本家どおりでよい）。
- 2026-09-19（ユーザーの回答）: **死亡画面の EASY は最新版に倣う**（ライフ 0 で EASY ならゲームオーバーにせず、ボタンも出さない。抜けるのはポーズから）。ポーズが無いうちに作ると本当の行き止まりになるので、ステップ 6 でポーズの道と一緒に作る。旧版を写すのはオプション画面の木だけで、両版にあるほかの画面の規則は最新版（`.claude/guides/autonomy.md` の「決め方」）。

- 2026-09-19: **オプション画面の Esc は本家どおり何もしない**（ステップ 4）— 本家の `UMG_Options` は Esc を結ばず、UI の入力の様式なのでキャラクターの Esc も届かない。WebGL 版の Esc = CANCEL はブラウザの Esc がポインタロックを外すための足し。下の要確認。

- 2026-09-19（ステップ 5）: **ポーズは旧版の Z 5 で開く**（最新版は 1）— 最新版の 1 では Z 5 の死亡画面の下になり、ステップ 6 の EASY の死亡画面の上に出せない。**Esc はキャラクターが受ける**（旧版。本作はプレイヤーコントローラーを持たず、ゲームパッドのキーはほかの操作と同じく割り当てない）。止まりの確かめは `EscapePressed` がする（`Wasami.Pause` も同じ道）。
- 2026-09-19（ステップ 5）: **頭は WebGL 版の白い絵を本家の頭の赤で塗る**（`HeadTint` = rgb(192, 0, 0)。のぞく頭は白い絵に黒い縁なので、塗ると赤い絵に黒い縁）。

## 要確認（ユーザー）

- 2026-09-19（ステップ 4）: オプション画面で Esc を押しても何も起きない（本家どおり。閉じるのは CANCEL / SAVE & EXIT だけ）。WebGL 版は Esc を CANCEL と同じにしていた。Esc でも閉じたいか。
- 2026-09-19（ステップ 5）: 本家の最新版は、EASY でライフ 0 の死亡画面のあいだゲームが止まっていて Esc も効かない（コードどおりだと抜け道の無い行き止まり）。回答（抜けるのはポーズから）どおりにするため、その死亡画面の上でだけ、止まっていても Esc でポーズが開くようにする（ステップ 6。ポーズは旧版の Z 5 で死亡画面の上に出す）。本家どおり行き止まりのままにするか、この形でよいか。

## 再開時の注意

- 本家のウィジェットの木を読むのは `python tmp/umg_tree.py <pak_reference の …/UMG_X.json>`（ステップ 3 で作った使い捨て。git の外なので、無ければ作り直す: `WidgetTree` の側の部品を `Slots` → `Content` でたどり、`Slot`・`Parent` を除いた props を並べる）。アニメのキーは同じ `.json` の `exports` の `MovieScene`・`MovieSceneFloatSection`・`MovieScene2DTransformSection`（`outer` が `UMG_X_C.` で始まるもの）と `WidgetAnimation` の `AnimationBindings`。
- PIE でポーズ画面を出すのは `python Tools/pie.py cmd "Wasami.Pause"`（Esc はエディタが遊びを止める。症状索引）。2026-09-19 のエディタの配置（右半分）では RESUME が画面の (2345, 551)、ビューポートの切り出しは `--region 1826 180 2870 860`。PIE の前に `Saved/SaveGames/*.sav` の控えを取り、止めたら戻す（`Wasami.Kill` で死亡数が書かれる）。
- 開発用の設定のセーブ `Saved/SaveGames/Settings.sav` は既定値（ステップ 1 の確かめの後に書き直した）。`Wasami.Settings` で値を変えたら `Wasami.ResetSettings` で戻す。
- テストをリモート実行で走らせるときは、エディタを前面にする（`python Tools/desktop.py click <タイトルバーの空き> --allow WindowsTerminal.exe --allow UnrealEditor.exe`。2026-09-19 はエディタが右半分にあり、タイトルバーの空きは (2800, 82)。起動時に浮いて出るメッセージログの窓は閉じてよい）。PIE でオプション画面だけを出すのは `L_Title` を開いて `python Tools/pie.py start` の後、リモート実行の `unreal.WasamiOptionsWidget.show(<ゲームのワールド>)`（止めたら Zone 1 を開き直す）。タイトルの OPTIONS は PIE の画面の (1964, 576)（同じ窓の配置のとき）。ボタンのホバーだけを見るのは `python Tools/desktop.py click X Y --count 0 --allow UnrealEditor.exe`（動かすだけで押さない。`look` の相対の動きはカーソルを思った所へ運ばない）。

## 検証

- ステップ 1〜4: check_records OK・ビルド OK・テスト（`Wasami.Settings`・`Options`・`Title` ほか）が通り、PIE で設定・音量・オプション画面を確かめた（中身は実装記録 15 の「確かめたこと」）。
- ステップ 5: check_records OK。C++ ビルド OK（ラムダの `Padding`・`Slot` が `UUserWidget` の同名を隠して C4458 になったので改名。警告なし）。テスト `Wasami.Pause`（新しい 3 件）・`Options`・`Settings`・`Title` の 16 件と、`GameFlow`・`Power`・`Interact`・`DeathScreen`・`Capture`・`Settings.Player` の 32 件が通った。PIE（Zone 1）で `Wasami.Pause` がメニューを出してゲームと時間を止め、曲が鳴り、RESUME のホバーで白、押すと 0.5 s で消えて動きが戻り、開いている間の 2 回目は何も足さず、死亡画面の上では開かない。開発用のセーブは控えから戻した。
