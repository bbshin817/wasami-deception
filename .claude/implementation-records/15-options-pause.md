---
title: 設定・オプション画面・ポーズ画面
sources:
  - Source/wasami_deception/WasamiSettingsSaveGame.h
  - Source/wasami_deception/WasamiSettingsSaveGame.cpp
  - Source/wasami_deception/Tests/WasamiSettingsTests.cpp
updated: 2026-09-19
---

# 設定・オプション画面・ポーズ画面

## 役割
プレイヤーの設定（本家の旧版 v1.6.1 の `BP_DD_Settings_SaveGame`、スロット `Settings`）と、それを読んで当てる本家の `BP_DD_GameMode` の `Check Settings Save` → `Set Settings`（音量は本家の `DD_SoundMix` のクラスの上書き）、プレイヤーが読む値（感度・Y 反転・頭の揺れ・ダッシュの切り替え・マウスのスムージング）、難易度の効き先（スコア画面の EASY）。作業一覧の項目 18。いまはステップ 2（音量と難易度）まで。オプション画面（旧版 `UMG_Options`）はステップ 3・4、ポーズ画面（旧版 `UMG_Pause`）はステップ 5・6 でここに足す。

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

## 作るアセット
なし（セーブは実行時に `Saved/SaveGames/Settings.sav`）。SoundMix と SoundClass は取り込み（01 記録）が作る。

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Save/BP_DD_Settings_SaveGame.json` の `Default__BP_DD_Settings_SaveGame_C`: 項目と既定値（`Difficulty` は `ENUM_DifficultySettings::NewEnumerator1`）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt`: `Check Settings Save` @33036、`Set Settings` @33342（ReceiveBeginPlay から @27445）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`: `Set Up Mouse Smoothing` @30382（`Mouse Smoothing` 真で 12.5、偽で 50）、マウス Y @23490（`Inverted Y Axis` 真で ×1、偽で ×−1、× `Mouse Sensitivity`）。`Set Up Mouse Smoothing` を呼ぶのは `UMG_Options` だけ（`pak_reference` の `_bytecode` を grep）。最新版は CVar `Character.MouseSmoothing` のコールバックで 0 → 12.5、1 → 50（向きが逆）だが、旧版を写す。
- `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt`: `Quality Text`（VERY HIGH ほか）、スライダーの吸着と文字・矢印の Clamp（WebGL 版 04 記録の `settings.ts` が読んだもの）。
- WebGL 版 04 記録の `settings.ts`（`snap`・`sliderText`・`stepValue`・`gamma`・`lookScale`・`rotationLagSpeed`・`SPRING_ARM_LAG_SPEED`）。
- 音量: `pak_reference/_assets/DDeception/Content/Audio/SoundMix/DD_SoundMix.json`・`DD_SoundClass_*.json`（`pak_reference_2` は Music の親が `/Engine/EngineSounds/Master` のほか同じ）、`BP_DD_GameMode.txt` の `SetBaseSoundMix` @27511 と `Set Settings` の `SetSoundMixClassOverride` @34211・@34304・@34397。
- 難易度: `pak_reference/_bytecode` と `pak_reference_2/_bytecode` で `BP_DD_Settings_SaveGame_C.Difficulty` を grep。最新版の死亡画面の分岐は `pak_reference_2/_bytecode/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.txt` @2639（`python Tools/dd/bp_flow.py <file> 2354`）。

## 依存関係
- 使う側: `UWasamiGameInstance`（持ち主）、`AWasamiGameMode`・`AWasamiTitleGameMode`（BeginPlay）、`AWasamiPlayerCharacter`（BeginPlay・`SaveSettings`）。
- エンジン: `UGameplayStatics`（`LoadGameFromSlot`・`SaveGameToSlot`・`CreateSaveGameObject`・`SetSubtitlesEnabled`・`SetBaseSoundMix`・`SetSoundMixClassOverride`）、`UGameUserSettings`、`UKismetMathLibrary`（`GridSnap_Float`・`MapRangeClamped`）、`UEngine::DisplayGamma`、`FAudioDevice::GetSoundClassCurrentProperties`（デバッグ）。
- アセット: `/Game/DD/Audio/SoundMix/DD_SoundMix`・`DD_SoundClass_Music`・`_SFX`・`_SFX_UI`・`_SFX_Movies`・`_Dialogue`（01 記録の `import_dd_sound_classes`）。

## 既知の制約・注意点
- **感度の換算は仮**（進捗記録の要確認）: 本家はマウスの軸に設定の値そのもの（既定 0.5）を掛けるが、本作の視点の速さ（1 カウント 0.175°。02 記録）は最新版の実機を感度 1 で測って合わせたので、そのまま掛けると既定で半分の速さになる。`PlayerSensitivityFor` = 設定 / 0.5 で、既定の 0.5 を今の速さ 1.0 にした（WebGL 版の `lookScale` と同じ）。
- エディタではスケーラビリティ・解像度・ポストプロセスの品質を当てない（上）。QUALITY と RESOLUTION SCALE の効きはパッケージでしか確かめられない。
- `Wasami.Settings` で値を変えると `Saved/SaveGames/Settings.sav` に残る（次の PIE もその値で始まる）。確かめた後は `Wasami.ResetSettings`。

## 変更履歴
- 2026-09-19: 初版（作業一覧の項目 18 のステップ 1: 設定のセーブと適用）。
- 2026-09-19: 音量（`DD_SoundMix` のクラスの上書きとベースのミックス）と難易度の効き先（スコア画面の EASY、`IsEasy`）を足した（ステップ 2）。
