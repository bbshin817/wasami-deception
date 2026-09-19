---
title: 設定・オプション画面・ポーズ画面
sources:
  - Source/wasami_deception/WasamiSettingsSaveGame.h
  - Source/wasami_deception/WasamiSettingsSaveGame.cpp
  - Source/wasami_deception/Tests/WasamiSettingsTests.cpp
  - Source/wasami_deception/WasamiOptionsWidget.h
  - Source/wasami_deception/WasamiOptionsWidget.cpp
  - Source/wasami_deception/Tests/WasamiOptionsTests.cpp
updated: 2026-09-19
---

# 設定・オプション画面・ポーズ画面

## 役割
プレイヤーの設定（本家の旧版 v1.6.1 の `BP_DD_Settings_SaveGame`、スロット `Settings`）と、それを読んで当てる本家の `BP_DD_GameMode` の `Check Settings Save` → `Set Settings`（音量は本家の `DD_SoundMix` のクラスの上書き）、プレイヤーが読む値（感度・Y 反転・頭の揺れ・ダッシュの切り替え・マウスのスムージング）、難易度の効き先（スコア画面の EASY）。作業一覧の項目 18。いまはステップ 3（オプション画面の木・Construct・Setup Values・FadeIn）まで。オプション画面の操作（スライダーの吸着・矢印・チェック・ホバー・SAVE & EXIT・CANCEL）とタイトルの OPTIONS はステップ 4、ポーズ画面（旧版 `UMG_Pause`）はステップ 5・6 でここに足す。

オプション画面は旧版を写す（最新版 v1.9.6〈`pak_reference_2`〉に `UMG_Options` は無く、第三者の AutoSettings プラグインの `SettingsUI` と CVar に替わった。作業一覧の完了の条件の WebGL 版 04・10 記録も旧版を写したもの。2026-09-19）。設定の項目は両版で同じ（最新版の `VSync`・`Motion Blur` は旧版のメニューに無いので持たない）。

## 公開インターフェース
- `UWasamiSettingsSaveGame`（`USaveGame`）
  - 項目（`SaveGame`。既定値は本家のクラスの既定）: `Quality` 2（0..3 = LOW / MEDIUM / HIGH / VERY HIGH）、`ResolutionScale` 1、`Brightness` 1、`Music` / `SFX` / `Dialogue` 1、`bSubtitles` 真、`MouseSensitivity` 0.5、`bHeadBobbing` 真、`bMouseSmoothing` 真、`bInvertedYAxis` 偽・`bToggleSprint` 偽（本家のクラスの既定に無い = 偽）、`Difficulty`（`EWasamiDifficulty`: `Easy` / `Normal` / `Hard` = 本家の `ENUM_DifficultySettings` の `NewEnumerator0..2`。既定 `Normal`）。本家の `Crosshair`（真）はメニューに無く、読むものも無いので持たない。
  - `SlotName`（`Settings`）・`UserIndex` 0。
  - `Check(Slot)`: 本家の `Check Settings Save`（@33036）。スロットを読み、無ければ既定の新しいセーブを作って書く。戻り値はそのセーブ。
  - `Apply(WorldContextObject)`: 本家の `Set Settings`（@33342）。下の「当てるもの」。音量はその世界の音の装置に当てる。
  - `LoadSoundMix()`・`LoadMusicClass()`・`LoadSFXClass()`・`LoadDialogueClass()`: `/Game/DD/Audio/SoundMix/` の `DD_SoundMix` と 3 つの SoundClass（柔らかい参照を読む）。
  - 規則の静的関数（オプション画面とテストが使う）: `Snap(v)`（`GridSnap_Float(v, 1/9)` を 0..1 に。`SliderGrid` = 1/9）、`SliderText(v)`（`Round(v × 10) / 10` を小数 0〜3 桁の数の文字に。10 段は 0, 0.1 … 0.4, 0.6 … 0.9, 1 と読める）、`StepValue(v, d, max)`（`Clamp(v + d, 0, max)`。`QualityMax` 3・`DifficultyMax` 1）、`QualityText`（LOW / MEDIUM / HIGH / VERY HIGH）、`DifficultyText`（EASY / NORMAL / HARD）、`GammaFor(b)`（`MapRangeClamped(b, 0, 1, 1.8, 2.2)`）、`PostProcessingQualityFor(q)`（0..2 → 2、3 → 3）、`PlayerSensitivityFor(s)`（s / 0.5）、`RotationLagSpeedFor(bSmoothing)`（12.5 / 50）。
- `UWasamiGameInstance`（06 記録）の `CheckSettingsSave()`・`GetSettings()`・`SaveSettings()`・`IsEasy()`（設定の `Difficulty` が EASY か。本家の `Global Settings Save Instance.Difficulty == 0`）・`SettingsSlotName`（下の「持ち主と流れ」）。
- `AWasamiPlayerCharacter`（02 記録）の `ApplySettings(Settings)`・`SetUpMouseSmoothing(Settings)`。
- デバッグ: `Wasami.Settings`（今の値と、音の装置が Music・SFX・Dialogue のクラスに今当てている音量〈`FAudioDevice::GetSoundClassCurrentProperties`。上書きは 1 s かけて届く〉を `LogWasamiSettings` に並べる）、`Wasami.Settings <名前> <値>`（`UWasamiSettingsSaveGame` の UPROPERTY の名前〈`Quality`・`MouseSensitivity`・`bInvertedYAxis`・`Difficulty` ほか〉に値を入れ、SAVE & EXIT と同じく当てて書いてプレイヤーに渡す）、`Wasami.ResetSettings`（既定値に戻して同じく保存）。どちらも `WasamiGameInstance.cpp`。
- `UWasamiOptionsWidget`（`UUserWidget`。C++ で木を組む。下の「オプション画面」）
  - `Show(WorldContextObject)`（BlueprintCallable）: 最初のプレイヤーに作って Z 10（`ViewportZOrder`。本家のタイトルとポーズの `CreateAndAddWidget(UMG_Options, 10)`）で足す。PIE で画面だけを出すときはリモート実行で `unreal.WasamiOptionsWidget.show(<ゲームのワールド>)`。
  - `Settings`（`Setup Values` が読み、ステップ 4 の SAVE & EXIT が書き込む設定。空ならゲームインスタンスの `GetSettings()`、それも無ければ既定の新しいもの）、`LevelName`（DIFFICULTY を決めるレベル。空なら今のレベル）。テストはどちらも入れてから足す。
  - `ShowsDifficulty(LevelName)`（タイトル `L_Title` だけ真）、`Begin(Settings, bShowDifficulty)`（Construct の中身。`NativeConstruct` が呼ぶ）、`SetupValues(Settings)`、`Advance(DeltaSeconds)`（`NativeTick` から。FadeIn と値の文字）。
  - テスト用の読み出し: `GetQualitySetting`・`GetDifficultySetting`・`HasDifficulty`・`GetFadeInTime`・`GetBoxOpacity`・`GetBoxScale`（`CanvasPanel_2`）・`GetWashOpacity`（`Blur+Red`）、曲線 `EvaluateFadeInOpacity`・`EvaluateFadeInScale`、定数 `RootScale` 1.015・`FadeInLength`・`CheckHoverGrey` 0.515625。
- テスト `Wasami.Options.Screen`（木・素材・様式・既定値と別の値の表示・値の文字がスライダーに付いていくこと・DIFFICULTY の出し分け）・`Wasami.Options.FadeIn`（`Tests/WasamiOptionsTests.cpp`）。
- テスト `Wasami.Settings.Defaults`・`Wasami.Settings.Rules`・`Wasami.Settings.SoundMix`（SoundMix の 3 つのクラスと子へ当てないこと、クラスの木と Properties、音 6 つのクラス）・`Wasami.Settings.Slot`（スロット `WasamiTest_Settings`。ゲームインスタンスの読み・当て・SAVE & EXIT の書き込み・`IsEasy`。表示ガンマは終わりに元へ戻す）・`Wasami.Settings.Player`（`Tests/WasamiSettingsTests.cpp`）。

## 内部構造と処理の流れ

### 持ち主と流れ
- 本家は設定をゲームモードの `Global Settings Save Instance` に持ち、ゲームモードの BeginPlay（@27445）が `Check Settings Save` → `Set Settings` を通し、続けて `SetBaseSoundMix(DD_SoundMix)`（@27511）する（タイトルのレベル `TitleScreen` のゲームモードも `BP_DD_GameMode`）。本作はレベルを開き直しても残るゲームインスタンスに持ち、**両方のゲームモード**（`AWasamiGameMode`・`AWasamiTitleGameMode`）の `BeginPlay` の頭で `UWasamiGameInstance::CheckSettingsSave()`（スロットを読み直す〈無ければ作って書く〉→ `Apply(this)` → 世界があれば `SetBaseSoundMix(DD_SoundMix)`）を呼ぶ。
- `GetSettings()` は読んだものを返し、まだ読んでいなければ `CheckSettingsSave()` を通す（ゲームモードとプレイヤーの BeginPlay の順は決まっていないので、先に来た方が読む）。
- プレイヤー: 本家は感度・Y 反転・頭の揺れ・ダッシュの切り替えを使うたびにゲームモードの設定から読む。本作は `AWasamiPlayerCharacter::BeginPlay` で `ApplySettings(GetSettings())` し、SAVE & EXIT（`SaveSettings`）でも渡す。ゲームインスタンスが `UWasamiGameInstance` でない（テストのワールド）ならプレイヤーの既定値のまま（既定の設定と同じ値）。
  - `ApplySettings`: `bToggleSprint`、`MouseSensitivity` = `PlayerSensitivityFor(MouseSensitivity)`、`bInvertY` = `bInvertedYAxis`、`bHeadBob` = `bHeadBobbing`、`ApplySpeed()`。
  - `SetUpMouseSmoothing`: スプリングアームの `CameraRotationLagSpeed` = スムージングありで 12.5、なしで 50。**本家の旧版でこれを呼ぶのはオプションの `Save Values`（プレイヤーがいるときだけ）だけ**なので、レベルはスプリングアームの既定 20 で始まる（WebGL 版と同じ）。
- `SaveSettings()`（本家の SAVE & EXIT: 適用 @18949 → `Save Values` @19688）: `GetSettings()` を `Apply` し、スロットに書き、最初のプレイヤーのキャラクターがあれば `ApplySettings` と `SetUpMouseSmoothing`。オプション画面（ステップ 4）は読んだ設定を書き換えてからこれを呼ぶ。

### 当てるもの（`Apply`）
- パッケージでだけ: `UGameUserSettings` に `SetOverallScalabilityLevel(Quality)` → `SetResolutionScaleValueEx(ResolutionScale × 100)` → `SetViewDistanceQuality(3)` → `SetPostProcessingQuality(PostProcessingQualityFor(Quality))` → `ApplySettings(false)`（本家の順）。**エディタ（`GIsEditor`）では飛ばす**: UE のスケーラビリティ（`sg.*`）はエディタ自身のものでもあり、`ApplySettings` はエディタの `Saved/Config/WindowsEditor/GameUserSettings.ini` に書くので、PIE のたびに開発の設定（エディタは「最高」。観察の撮り方の前提、`.claude/guides/observation.md`）が上書きされる（`.claude/guides/performance.md` の大原則 2）。
- `UGameplayStatics::SetSubtitlesEnabled(bSubtitles)`。
- 表示ガンマ: 本家のコンソールコマンド `gamma X` と同じく `GEngine->DisplayGamma` = `GammaFor(Brightness)`（UE 5.8 の `UEngine::HandleGammaCommand` は値を 0.5..5 に収めて `DisplayGamma` に入れるだけ）。エディタのビューポートにも効くので、エディタでは `UWasamiGameInstance::Init` が PIE の始まりの値を覚え、`Shutdown`（PIE の終わり）で戻す。
- 音量: `SetSoundMixClassOverride(DD_SoundMix, Music / SFX / Dialogue のクラス, 値, 1, 1, False)`（本家の引数どおり: ピッチ 1・1 s で移る・子のクラスへは当てない）。世界の無いゲームインスタンス（テスト）では飛ばす。PIE は PIE ごとの音の装置なので、エディタの音には残らない。

### 音量の仕組み（SoundMix と SoundClass）
- `DD_SoundMix`（ベースのミックス）: `SoundClassEffects` は SFX・Dialogue・Music の 3 つ（音量 1、`bApplyToChildren` 偽）。フェード 0.2 s、`Duration` −1。
- クラスの木（旧版）: `DD_SoundClass_SFX`（`bApplyAmbientVolumes`）の子に `DD_SoundClass_SFX_UI`（`bIsUISound`、残響なし）と `DD_SoundClass_SFX_Movies`（音量 0.5）。`DD_SoundClass_Music`（残響なし）と `DD_SoundClass_Dialogue` は親なし。最新版は Music の親がエンジンの `Master`（最新版の総音量のため。旧版のオプションに無いので写さない）。
- **SFX のスライダーは SFX_UI・SFX_Movies に効かない**（本家どおり）: UE はクラスの木で音量を親から子へ掛けた後（`ParseSoundClasses`）にミックスの上書きを当てるので、`bApplyToChildren` 偽の上書きは SFX のクラスの音だけに効く（UE 5.8 の `FAudioDevice::UpdateSoundClassProperties`・`ApplyClassAdjusters`）。
- 音のクラスは本家の書き出しの `SoundClassObject` のまま（01 記録の `dd_assets.sound_classes`）。**本家でクラスの無い音は無いまま**（病院の扉・リフト・エレベーター・鍵開け・針の罠など 21 件。本家でもどのスライダーも効かない）。`Pause_Sound_v1`（タイトルとポーズの曲）は旧版で SFX、最新版で Music で、最新版から取り込んだので Music。

### 難易度の効き先
- スコア画面（13 記録）: `AWasamiGameMode::Escape` とデバッグ `Wasami.LevelClear` が `FWasamiLevelResults::ForHospital(…, IsEasy())` を渡す（EASY MODE の文字と、FINAL RANK が A で止まる）。
- 死亡画面の Easy の分岐は**作らない**（2026-09-19。進捗記録の要確認）: 旧版の `UMG_DeathScreen` は難易度を読まない。最新版はライフ 0 で EASY なら `Life Animation` だけで止まり、ボタンが出ない（ポーズからしか抜けられない）。オプション画面を写す旧版に合わせた。
- 病院の敵は難易度を読まない（両版で `Difficulty` を読むのはホテル・学校・屋敷・下水・サーカスの敵と、死亡画面・スコア画面・ポーズ・オプション・ゲームインスタンス）。ポーズの EASY MODE はステップ 5。

### オプション画面（`UWasamiOptionsWidget`）
本家の旧版 `UMG_Options` の木を、スロットの値のまま（書き出しに無い値はスロット・部品の既定）`RebuildWidget` で組む（死亡画面・タイトルと同じ作り。09・14 記録）。書き出しの `UMG_Options_C.WidgetTree` の側の部品と、その `Slot` の値を読んだ。
- `CanvasPanel_0`（ルート。レンダー変換の拡大 1.015）に 2 つ:
  1. `Blur+Red`（不透明度 0 から FadeIn）: `BackgroundBlur_0`（強さ 8）と `Image_1`（赤 (0.182, 0, 0, 0.371)）を、どちらも画面いっぱいより少しはみ出して。キャンバスの既定の見え方（自分は当たらない）で、子の画像とぼかしがクリックを受け止めるので、下のタイトルは押せない。
  2. `CanvasPanel_2`（FadeIn が拡大し、不透明にする）: 枠 `Image_31`（`options_window_frame`。左右 179.396・上下 −0.959 の内側に引き伸ばす。OPTIONS の文字は絵に描かれている）、`CanvasPanel_1`（中央から (−673.56, −383.54)、1338.09 × 812.28）の中央の `GridPanel_0`（1234.53 × 673.85、中央から (−16.27, −28.77) に整列 0.5）、下の `HorizontalBox_11`（下端の中央から上へ 6.75、整列 (0.5, 1.25)。SAVE & EXIT は右に 40、CANCEL は左に 40）。
- `GridPanel_0`: 列 0 と行 0 の Fill 0.5（ほかは自動）。UE の `UGridPanel` がそのまま Slate の配置をするので、WebGL 版の `slateGrid` の計算は要らない。スロットはどれも上下左右の中央寄せ: `GraphicsBox`（列 0・行 1〜3。平行移動 (−50, 0)）、`AudioBox`（列 0・行 3、上の余白 150、Nudge (0, 34.41)。平行移動 (−50, 0)）、`ControlsBox`（列 1〜2・行 2〜3、上の余白 150、Nudge (34.65, −182.73)。平行移動 (0, 254)）、`DifficultyBox`（列 0・行 1〜3。平行移動 (650, −92)）。
- 箱（VerticalBox）: 見出し（`helvetica-neue-bold_Font` 36、白、最小の幅 550、中央揃え。スロットは中央）と行（HorizontalBox。見出しの直後は上に 25、ほかは 15。AUDIO の最初の行も 15）。
  - ラベル: TextBlock の既定の書体（エンジンの Roboto Bold 24）、灰 0.5、最小の幅 310（DIFFICULTY の行は 240.99）。QUALITY・RESOLUTION・DIFFICULTY のラベルのスロットは縦だけ中央、ほかは上下左右の中央。
  - 値の箱: `UBorder` の背景 `selection_bar` を Box で描く（`ImageSize` 242 × 242・余白 0.1 = 各辺 24.2 を引き伸ばさない）、余白 10、中央寄せ。中の文字は `helvetica-neue-bold_Font` 18、灰 0.5（隠れた RESOLUTION の行の `TextBlock_12` だけ白）、最小の幅はスライダーの行 61.28・矢印の行 215、中央揃え。スライダーの行では左に 26、矢印の行では余白なし（どちらも Fill）。
  - スライダー（右に 30。MOUSE SENSITIVITY だけ −10。Fill・縦中央）: 本家の木の様式 = UE 4 の既定の様式の上書き。バーは白の色ブラシ（余白 (0, 0, 1, 1) は書き出しのまま）を `SliderBarColor` の赤 (1, 0, 0) で、太さ 4。つまみは `slider_bar_tab` を 18 × 30（使えないときは 22 × 38）で、UE 4 の既定のつまみと同じく Box（各辺 8/32）で描く。UE 4 の様式にホバーの絵は無かったので、UE 5 のホバーの絵にはふだんの絵を入れる。
  - チェック（左に 18、中央）: Construct が付け直す様式で組む: `checkbox_icon`（外れ）・`checkbox_icon_checked`（入り）を 55 × 64 の Image で、ホバーの絵だけ灰 0.515625、押したときとふだんは白。余白 (2, 0, 0, 0)、前景は継ぐ（UE 5 の `FCheckBoxStyle` の既定と同じ。背景の絵は無し）。
  - 矢印（左は右に 5・180° 回す、右は左に 5、中央）: 21 × 32 の中身の無いボタン。**QUALITY の矢印は Construct が付け直す様式**（`selection_bar_arrow_normal` / `_hover` / `_normal` を Image で、余白 0）、**DIFFICULTY と隠れた RESOLUTION の矢印は木の様式**（UE 4 の既定のボタンの絵を矢印に替えたもの = Box で各辺 8/32、余白 2・押すと (2, 3, 2, 1)）。
  - 行: GRAPHICS = QUALITY（矢印・HIGH）・RESOLUTION SCALE・BRIGHTNESS・RESOLUTION（不透明度 0 の行。場所は取る）。AUDIO = MUSIC・SFX・DIALOGUE・SUBTITLES・RESOLUTION（Hidden の行。場所は取る）。CONTROLS = MOUSE SENSITIVITY・HEAD BOBBING・INVERTED Y AXIS・TOGGLE SPRINT・MOUSE SMOOTHING。DIFFICULTY = DIFFICULTY（矢印・NORMAL）。
- SAVE & EXIT（`ApplyButton`）・CANCEL（`CancelButton`）: UE 4 の既定のボタン（余白 2・押すと (2, 3, 2, 1)）で背景の色の α 0、中身の色 0.1146（`Unhovered Color`）。文字は `helvetica-neue-bold_Font` 30。ボタンのスロットは UE の既定 (4, 2)・中央。
- UE 5 と UE 4 の既定の違い: ボタン・スライダー・チェックの様式は UE 5 の既定に頼らず、上の値を書く（UE 5 は Starship の様式で、余白・つまみ・ホバーの絵が違う）。

Construct（`NativeConstruct` → `Begin`。本家 @21404）: `SetInputMode_UIOnlyEx(自分, DoNotLock)` とカーソル → FadeIn を初めから → `Cast To GameMode`（本作はゲームインスタンスの設定）→ `Setup Values` → チェックと QUALITY の矢印の様式（木に組み込み済み）→ レベルの名前が本家の `TitleScreen`・`00_Ballroom`・`TestMap` でなければ `DifficultyBox.RemoveFromParent()`（**ポーズから開くとゲームの中では DIFFICULTY が無い**。本作はタイトル `L_Title` だけ残す）。
- `Setup Values`（@17146）: `QualitySetting` ← `Quality`、スライダー 6 本（RESOLUTION SCALE・BRIGHTNESS・MUSIC・SFX・DIALOGUE・MOUSE SENSITIVITY）に値、チェック 5 つ（SUBTITLES・HEAD BOBBING・INVERTED Y AXIS・TOGGLE SPRINT・MOUSE SMOOTHING）に入り切り、`Difficulty Setting` ← `Difficulty`。
- 値の箱の文字は本家では関数の結び付け（描くたびに読む）: スライダーの行は `RoundFloatDecimals(Slider.GetValue(), 1)` を `Conv_FloatToText`（`UWasamiSettingsSaveGame::SliderText`）、QUALITY は `Quality Text`（`QualitySetting`）、DIFFICULTY は `GetText_0`（`Difficulty Setting`）。本作はティックごとに読み直して、変わったときだけ書く（`RefreshTexts`）。
- FadeIn（書き出しのキー。0.5 s、区間は終わっても最後の値のまま）: `Blur+Red` と `CanvasPanel_2` の不透明度 0 → 1（0.25 s）、`CanvasPanel_2` の拡大 0 → 1（0.25 s、UE の自動の接線で少し行き過ぎる）→ 1（0.5 s）。アニメの結び付けの名前は `CanvasPanel_0` だが、`AnimationBindings` の `WidgetName` は `CanvasPanel_2`（ルートの 1.015 はそのまま）。キーは `UMG_PopUp` の `Popup` と同じ値。ウィジェットのティックで進めるので、止まったゲームの上でも動く。

## 作るアセット
なし（セーブは実行時に `Saved/SaveGames/Settings.sav`）。SoundMix と SoundClass は取り込み（01 記録）が作る。オプション画面の素材は `dd_ui.import_options()`（`WasamiDDTools.import_dd_ui` の `import_all` も呼ぶ。01 記録）:

| 種類 | アセット | 数 |
| --- | --- | --- |
| テクスチャ（本家） | `/Game/DD/UI/Menu/Settings/options_window_frame`（1732 × 1200）・`selection_bar`（242 × 55）・`selection_bar_arrow_normal`・`selection_bar_arrow_hover`（21 × 32）・`slider_bar_tab`（22 × 38）・`checkbox_icon`・`checkbox_icon_checked`（55 × 64）。`pak_reference_2` から（旧版と同じ画像）、sRGB・UI の LOD グループ | 7 |

見出し・値・ボタンの書体 `/Game/DD/UI/Fonts/helvetica-neue-bold_Font` はタブレットの取り込み（03 記録）が作る。

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Save/BP_DD_Settings_SaveGame.json` の `Default__BP_DD_Settings_SaveGame_C`: 項目と既定値（`Difficulty` は `ENUM_DifficultySettings::NewEnumerator1`）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt`: `Check Settings Save` @33036、`Set Settings` @33342（ReceiveBeginPlay から @27445）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`: `Set Up Mouse Smoothing` @30382（`Mouse Smoothing` 真で 12.5、偽で 50）、マウス Y @23490（`Inverted Y Axis` 真で ×1、偽で ×−1、× `Mouse Sensitivity`）。`Set Up Mouse Smoothing` を呼ぶのは `UMG_Options` だけ（`pak_reference` の `_bytecode` を grep）。最新版は CVar `Character.MouseSmoothing` のコールバックで 0 → 12.5、1 → 50（向きが逆）だが、旧版を写す。
- `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt`: `Quality Text`（VERY HIGH ほか）、スライダーの吸着と文字・矢印の Clamp（WebGL 版 04 記録の `settings.ts` が読んだもの）。
- WebGL 版 04 記録の `settings.ts`（`snap`・`sliderText`・`stepValue`・`gamma`・`lookScale`・`rotationLagSpeed`・`SPRING_ARM_LAG_SPEED`）。
- 音量: `pak_reference/_assets/DDeception/Content/Audio/SoundMix/DD_SoundMix.json`・`DD_SoundClass_*.json`（`pak_reference_2` は Music の親が `/Engine/EngineSounds/Master` のほか同じ）、`BP_DD_GameMode.txt` の `SetBaseSoundMix` @27511 と `Set Settings` の `SetSoundMixClassOverride` @34211・@34304・@34397。
- オプション画面: `pak_reference/_assets/DDeception/Content/UI/Menu/UMG_Options.json`（木は `UMG_Options_C.WidgetTree` の側。FadeIn は `FadeIn_INST` の `MovieScene` の区間と `AnimationBindings`・`PrecompiledEvaluationTemplate` の `KeepState`、値の箱の結び付けは `UMG_Options_C` の `Bindings`、`Unhovered Color` は `Default__UMG_Options_C`）。Construct・`Setup Values` は `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt`（`python Tools/dd/bp_flow.py <file> Construct` / `"Setup Values"`）、結び付けの関数は同じファイルの `=== Resolution Scale` ほか。UE 4 の既定の様式（ボタン・スライダーの Box の絵と余白）は UE 4.21 の `FCoreStyle`、UE 5 の違いは UE 5.8 の `SlateTypes.cpp`（`FCheckBoxStyle`・`FSliderStyle` の既定）・`SSlider::OnPaint`・`SCheckBox::BuildCheckBox`。
- 難易度: `pak_reference/_bytecode` と `pak_reference_2/_bytecode` で `BP_DD_Settings_SaveGame_C.Difficulty` を grep。最新版の死亡画面の分岐は `pak_reference_2/_bytecode/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.txt` @2639（`python Tools/dd/bp_flow.py <file> 2354`）。

## 依存関係
- オプション画面: `UWasamiGameInstance::GetSettings`、`UWasamiSettingsSaveGame` の規則（`SliderText`・`QualityText`・`DifficultyText`）、`AWasamiGameMode::TitleLevelName`、`WasamiWidgetAnimation.h`（09 記録）、`WasamiAssets.h`、エンジンの `UGridPanel`・`USlider`・`UCheckBox`・`UBackgroundBlur`。使う側はステップ 4 のタイトルの OPTIONS とステップ 6 のポーズの OPTIONS（まだ無い）。
- 使う側: `UWasamiGameInstance`（持ち主）、`AWasamiGameMode`・`AWasamiTitleGameMode`（BeginPlay）、`AWasamiPlayerCharacter`（BeginPlay・`SaveSettings`）。
- エンジン: `UGameplayStatics`（`LoadGameFromSlot`・`SaveGameToSlot`・`CreateSaveGameObject`・`SetSubtitlesEnabled`・`SetBaseSoundMix`・`SetSoundMixClassOverride`）、`UGameUserSettings`、`UKismetMathLibrary`（`GridSnap_Float`・`MapRangeClamped`）、`UEngine::DisplayGamma`、`FAudioDevice::GetSoundClassCurrentProperties`（デバッグ）。
- アセット: `/Game/DD/Audio/SoundMix/DD_SoundMix`・`DD_SoundClass_Music`・`_SFX`・`_SFX_UI`・`_SFX_Movies`・`_Dialogue`（01 記録の `import_dd_sound_classes`）。

## 既知の制約・注意点
- **感度の換算は仮**（進捗記録の要確認）: 本家はマウスの軸に設定の値そのもの（既定 0.5）を掛けるが、本作の視点の速さ（1 カウント 0.175°。02 記録）は最新版の実機を感度 1 で測って合わせたので、そのまま掛けると既定で半分の速さになる。`PlayerSensitivityFor` = 設定 / 0.5 で、既定の 0.5 を今の速さ 1.0 にした（WebGL 版の `lookScale` と同じ）。
- エディタではスケーラビリティ・解像度・ポストプロセスの品質を当てない（上）。QUALITY と RESOLUTION SCALE の効きはパッケージでしか確かめられない。
- オプション画面はまだ操作できない（ステップ 4）: スライダー・チェック・矢印は UE の既定の動き（吸着・SAVE & EXIT・CANCEL・ホバーの色は無い）で、画面を閉じる道も無い。
- `SetInputMode_UIOnlyEx` にこの画面を渡すと、画面が焦点を持てないので `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget` が出る。本家も同じ（タイトル・死亡画面と同じく、そのままにしている。14 記録）。
- MOUSE SENSITIVITY のスライダーは枠の右を越えて描かれる: CONTROLS の箱の幅は見出しの最小の幅 550 で決まり、行はそれに合わせて広がり、スライダーのスロットの右の余白が −10 なので（本家の木のまま）。
- `Wasami.Settings` で値を変えると `Saved/SaveGames/Settings.sav` に残る（次の PIE もその値で始まる）。確かめた後は `Wasami.ResetSettings`。

## 確かめたこと
- 2026-09-19（ステップ 3、PIE、`L_Title`、エディタを右半分・ビューポート約 1050 × 690）: リモート実行の `unreal.WasamiOptionsWidget.show(<ゲームのワールド>)` で、タイトルの上に赤くぼけた幕と OPTIONS の枠が出て、左に GRAPHICS（QUALITY の矢印と HIGH、RESOLUTION SCALE・BRIGHTNESS の 1 と右端のつまみ）と AUDIO（MUSIC・SFX・DIALOGUE の 1、SUBTITLES の入り）、右に DIFFICULTY（NORMAL）と CONTROLS（MOUSE SENSITIVITY の 0.5 と中ほどのつまみ、HEAD BOBBING・MOUSE SMOOTHING の入り、INVERTED Y AXIS・TOGGLE SPRINT の外れ）、下に灰色の SAVE & EXIT・CANCEL が並んだ（WebGL 版 10 記録の styles.css「OPTIONS」と同じ配置）。

## 変更履歴
- 2026-09-19: 初版（作業一覧の項目 18 のステップ 1: 設定のセーブと適用）。
- 2026-09-19: 音量（`DD_SoundMix` のクラスの上書きとベースのミックス）と難易度の効き先（スコア画面の EASY、`IsEasy`）を足した（ステップ 2）。
- 2026-09-19: オプション画面 `UWasamiOptionsWidget`（本家の旧版 `UMG_Options` の木・Construct・`Setup Values`・値の文字の結び付け・FadeIn）と素材の取り込み（`dd_ui.import_options`）、テスト `Wasami.Options.Screen`・`Wasami.Options.FadeIn` を足した（ステップ 3）。
