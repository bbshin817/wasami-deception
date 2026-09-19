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
プレイヤーの設定（本家の旧版 v1.6.1 の `BP_DD_Settings_SaveGame`、スロット `Settings`）と、それを読んで当てる本家の `BP_DD_GameMode` の `Check Settings Save` → `Set Settings`、プレイヤーが読む値（感度・Y 反転・頭の揺れ・ダッシュの切り替え・マウスのスムージング）。作業一覧の項目 18。いまはステップ 1（設定のセーブと適用）まで。音量（SoundMix）と難易度の効き先はステップ 2、オプション画面（旧版 `UMG_Options`）はステップ 3・4、ポーズ画面（旧版 `UMG_Pause`）はステップ 5・6 でここに足す。

オプション画面は旧版を写す（最新版 v1.9.6〈`pak_reference_2`〉に `UMG_Options` は無く、第三者の AutoSettings プラグインの `SettingsUI` と CVar に替わった。作業一覧の完了の条件の WebGL 版 04・10 記録も旧版を写したもの。2026-09-19）。設定の項目は両版で同じ（最新版の `VSync`・`Motion Blur` は旧版のメニューに無いので持たない）。

## 公開インターフェース
- `UWasamiSettingsSaveGame`（`USaveGame`）
  - 項目（`SaveGame`。既定値は本家のクラスの既定）: `Quality` 2（0..3 = LOW / MEDIUM / HIGH / VERY HIGH）、`ResolutionScale` 1、`Brightness` 1、`Music` / `SFX` / `Dialogue` 1、`bSubtitles` 真、`MouseSensitivity` 0.5、`bHeadBobbing` 真、`bMouseSmoothing` 真、`bInvertedYAxis` 偽・`bToggleSprint` 偽（本家のクラスの既定に無い = 偽）、`Difficulty`（`EWasamiDifficulty`: `Easy` / `Normal` / `Hard` = 本家の `ENUM_DifficultySettings` の `NewEnumerator0..2`。既定 `Normal`）。本家の `Crosshair`（真）はメニューに無く、読むものも無いので持たない。
  - `SlotName`（`Settings`）・`UserIndex` 0。
  - `Check(Slot)`: 本家の `Check Settings Save`（@33036）。スロットを読み、無ければ既定の新しいセーブを作って書く。戻り値はそのセーブ。
  - `Apply()`: 本家の `Set Settings`（@33342）。下の「当てるもの」。
  - 規則の静的関数（オプション画面とテストが使う）: `Snap(v)`（`GridSnap_Float(v, 1/9)` を 0..1 に。`SliderGrid` = 1/9）、`SliderText(v)`（`Round(v × 10) / 10` を小数 0〜3 桁の数の文字に。10 段は 0, 0.1 … 0.4, 0.6 … 0.9, 1 と読める）、`StepValue(v, d, max)`（`Clamp(v + d, 0, max)`。`QualityMax` 3・`DifficultyMax` 1）、`QualityText`（LOW / MEDIUM / HIGH / VERY HIGH）、`DifficultyText`（EASY / NORMAL / HARD）、`GammaFor(b)`（`MapRangeClamped(b, 0, 1, 1.8, 2.2)`）、`PostProcessingQualityFor(q)`（0..2 → 2、3 → 3）、`PlayerSensitivityFor(s)`（s / 0.5）、`RotationLagSpeedFor(bSmoothing)`（12.5 / 50）。
- `UWasamiGameInstance`（06 記録）の `CheckSettingsSave()`・`GetSettings()`・`SaveSettings()`・`SettingsSlotName`（下の「持ち主と流れ」）。
- `AWasamiPlayerCharacter`（02 記録）の `ApplySettings(Settings)`・`SetUpMouseSmoothing(Settings)`。
- デバッグ: `Wasami.Settings`（今の値を `LogWasamiSettings` に並べる）、`Wasami.Settings <名前> <値>`（`UWasamiSettingsSaveGame` の UPROPERTY の名前〈`Quality`・`MouseSensitivity`・`bInvertedYAxis`・`Difficulty` ほか〉に値を入れ、SAVE & EXIT と同じく当てて書いてプレイヤーに渡す）、`Wasami.ResetSettings`（既定値に戻して同じく保存）。どちらも `WasamiGameInstance.cpp`。
- テスト `Wasami.Settings.Defaults`・`Wasami.Settings.Rules`・`Wasami.Settings.Slot`（スロット `WasamiTest_Settings`。ゲームインスタンスの読み・当て・SAVE & EXIT の書き込み。表示ガンマは終わりに元へ戻す）・`Wasami.Settings.Player`（`Tests/WasamiSettingsTests.cpp`）。

## 内部構造と処理の流れ

### 持ち主と流れ
- 本家は設定をゲームモードの `Global Settings Save Instance` に持ち、ゲームモードの BeginPlay（@27445）が `Check Settings Save` → `Set Settings` を通す（タイトルのレベル `TitleScreen` のゲームモードも `BP_DD_GameMode`）。本作はレベルを開き直しても残るゲームインスタンスに持ち、**両方のゲームモード**（`AWasamiGameMode`・`AWasamiTitleGameMode`）の `BeginPlay` の頭で `UWasamiGameInstance::CheckSettingsSave()`（スロットを読み直す〈無ければ作って書く〉→ `Apply`）を呼ぶ。
- `GetSettings()` は読んだものを返し、まだ読んでいなければ `CheckSettingsSave()` を通す（ゲームモードとプレイヤーの BeginPlay の順は決まっていないので、先に来た方が読む）。
- プレイヤー: 本家は感度・Y 反転・頭の揺れ・ダッシュの切り替えを使うたびにゲームモードの設定から読む。本作は `AWasamiPlayerCharacter::BeginPlay` で `ApplySettings(GetSettings())` し、SAVE & EXIT（`SaveSettings`）でも渡す。ゲームインスタンスが `UWasamiGameInstance` でない（テストのワールド）ならプレイヤーの既定値のまま（既定の設定と同じ値）。
  - `ApplySettings`: `bToggleSprint`、`MouseSensitivity` = `PlayerSensitivityFor(MouseSensitivity)`、`bInvertY` = `bInvertedYAxis`、`bHeadBob` = `bHeadBobbing`、`ApplySpeed()`。
  - `SetUpMouseSmoothing`: スプリングアームの `CameraRotationLagSpeed` = スムージングありで 12.5、なしで 50。**本家の旧版でこれを呼ぶのはオプションの `Save Values`（プレイヤーがいるときだけ）だけ**なので、レベルはスプリングアームの既定 20 で始まる（WebGL 版と同じ）。
- `SaveSettings()`（本家の SAVE & EXIT: 適用 @18949 → `Save Values` @19688）: `GetSettings()` を `Apply` し、スロットに書き、最初のプレイヤーのキャラクターがあれば `ApplySettings` と `SetUpMouseSmoothing`。オプション画面（ステップ 4）は読んだ設定を書き換えてからこれを呼ぶ。

### 当てるもの（`Apply`）
- パッケージでだけ: `UGameUserSettings` に `SetOverallScalabilityLevel(Quality)` → `SetResolutionScaleValueEx(ResolutionScale × 100)` → `SetViewDistanceQuality(3)` → `SetPostProcessingQuality(PostProcessingQualityFor(Quality))` → `ApplySettings(false)`（本家の順）。**エディタ（`GIsEditor`）では飛ばす**: UE のスケーラビリティ（`sg.*`）はエディタ自身のものでもあり、`ApplySettings` はエディタの `Saved/Config/WindowsEditor/GameUserSettings.ini` に書くので、PIE のたびに開発の設定（エディタは「最高」。観察の撮り方の前提、`.claude/guides/observation.md`）が上書きされる（`.claude/guides/performance.md` の大原則 2）。
- `UGameplayStatics::SetSubtitlesEnabled(bSubtitles)`。
- 表示ガンマ: 本家のコンソールコマンド `gamma X` と同じく `GEngine->DisplayGamma` = `GammaFor(Brightness)`（UE 5.8 の `UEngine::HandleGammaCommand` は値を 0.5..5 に収めて `DisplayGamma` に入れるだけ）。エディタのビューポートにも効くので、エディタでは `UWasamiGameInstance::Init` が PIE の始まりの値を覚え、`Shutdown`（PIE の終わり）で戻す。
- 音量（`SetSoundMixClassOverride(DD_SoundMix, …)`）はステップ 2。

## 作るアセット
なし（セーブは実行時に `Saved/SaveGames/Settings.sav`）。

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Save/BP_DD_Settings_SaveGame.json` の `Default__BP_DD_Settings_SaveGame_C`: 項目と既定値（`Difficulty` は `ENUM_DifficultySettings::NewEnumerator1`）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt`: `Check Settings Save` @33036、`Set Settings` @33342（ReceiveBeginPlay から @27445）。
- `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`: `Set Up Mouse Smoothing` @30382（`Mouse Smoothing` 真で 12.5、偽で 50）、マウス Y @23490（`Inverted Y Axis` 真で ×1、偽で ×−1、× `Mouse Sensitivity`）。`Set Up Mouse Smoothing` を呼ぶのは `UMG_Options` だけ（`pak_reference` の `_bytecode` を grep）。最新版は CVar `Character.MouseSmoothing` のコールバックで 0 → 12.5、1 → 50（向きが逆）だが、旧版を写す。
- `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_Options.txt`: `Quality Text`（VERY HIGH ほか）、スライダーの吸着と文字・矢印の Clamp（WebGL 版 04 記録の `settings.ts` が読んだもの）。
- WebGL 版 04 記録の `settings.ts`（`snap`・`sliderText`・`stepValue`・`gamma`・`lookScale`・`rotationLagSpeed`・`SPRING_ARM_LAG_SPEED`）。

## 依存関係
- 使う側: `UWasamiGameInstance`（持ち主）、`AWasamiGameMode`・`AWasamiTitleGameMode`（BeginPlay）、`AWasamiPlayerCharacter`（BeginPlay・`SaveSettings`）。
- エンジン: `UGameplayStatics`（`LoadGameFromSlot`・`SaveGameToSlot`・`CreateSaveGameObject`・`SetSubtitlesEnabled`）、`UGameUserSettings`、`UKismetMathLibrary`（`GridSnap_Float`・`MapRangeClamped`）、`UEngine::DisplayGamma`。

## 既知の制約・注意点
- **感度の換算は仮**（進捗記録の要確認）: 本家はマウスの軸に設定の値そのもの（既定 0.5）を掛けるが、本作の視点の速さ（1 カウント 0.175°。02 記録）は最新版の実機を感度 1 で測って合わせたので、そのまま掛けると既定で半分の速さになる。`PlayerSensitivityFor` = 設定 / 0.5 で、既定の 0.5 を今の速さ 1.0 にした（WebGL 版の `lookScale` と同じ）。
- エディタではスケーラビリティ・解像度・ポストプロセスの品質を当てない（上）。QUALITY と RESOLUTION SCALE の効きはパッケージでしか確かめられない。
- `Wasami.Settings` で値を変えると `Saved/SaveGames/Settings.sav` に残る（次の PIE もその値で始まる）。確かめた後は `Wasami.ResetSettings`。

## 変更履歴
- 2026-09-19: 初版（作業一覧の項目 18 のステップ 1: 設定のセーブと適用）。
