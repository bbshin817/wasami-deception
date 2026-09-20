---
title: 曲と環境音（ゾーンの曲の切り替え）
sources:
  - Source/wasami_deception/WasamiMusicPlayer.h
  - Source/wasami_deception/WasamiMusicPlayer.cpp
  - Source/wasami_deception/Tests/WasamiMusicTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_audio.py
updated: 2026-09-20
---

# 曲と環境音（ゾーンの曲の切り替え）

## 役割
レベル自身が鳴らす音。いまは曲だけ: `AWasamiMusicPlayer`（本家 `pak_reference_2` の `Blueprints/06_Hospital/BP_06_MusicPlayer`。処理は親の `Blueprints/08_BearHouse/BP_08_MusicPlayer`）が、ゾーンの通常の曲・追跡の曲・上書きの曲の 3 つを持ち、0.5 s ごとに `bFadeOut`・`bOverrideMusic`・敵が追跡中かを見て 1 s でクロスフェードする。レベルにゾーンごとに 1 体置き、`bFadeOut` を触り Zone 2 の独房の場面で曲を下げ・戻すのはゾーンの流れ（11 記録）。作業一覧の項目 19 のステップ 1・2 で作った。

レベルが自分で鳴らす環境音（`AmbientSound`）と残響のボリューム（`AudioVolume`）も、本家の置き場所と値のまま置く（ステップ 3）。ほかの効果音はこの後のステップで足す（仕掛けやパワー、敵、UI の音は、それぞれを作る記録と取り込みの側にある）。

## 公開インターフェース
- `EWasamiMusicFade`（`None` / `In` / `Out` / `OutIfPlaying`）: 1 回の `Update` が 1 つの部品に求めるフェード。`OutIfPlaying` は本家の `If Playing Fade Out`（鳴っているときだけ `FadeOut`）。
- `FWasamiMusicFades`: 3 つの部品（`Regular`・`Panic`・`Override`）の 1 回分と `IsSilent()`。
- `FWasamiMusicDoOnce`: 本家の DoOnce ノード 1 つ。`Enter()`（通したら真。以後は Reset まで偽）・`Reset()`・`IsClosed()`。`FWasamiMusicDoOnce{true}` は Start Closed。
- `FWasamiMusicState`: `Update(bFadeOut, bOverrideMusic, bIntense)` → `FWasamiMusicFades`。ワールド無しで試せる純粋な構造体で、5 つの DoOnce を持つ。
- `AWasamiMusicPlayer`（`AActor`）: `bFadeOut`・`bOverrideMusic`（どちらも `EditAnywhere, BlueprintReadWrite`）、`Update()`（`BlueprintCallable`）、`IsIntenseMusic()`（`BlueprintPure`。本家の `Intense Music ?`）、`FadeRegularMusicIn(Duration, Volume)`（`BlueprintCallable`。`Regular Music` の `FadeIn(Duration, Volume, 0, Linear)` だけを流す。Zone 2 の独房の場面が使う）、`GetLastFades()`（直前の `Update` が流したフェード。テストと道具のため）、`GetLastRegularFadeInDuration()`・`GetLastRegularFadeInVolume()`（直前の `FadeRegularMusicIn`。まだ無ければ長さが負。同じくテストのため）、部品の取り出し `GetRegularMusic()`・`GetPanicMusic()`・`GetOverrideMusic()`。定数 `UpdateInterval` 0.5・`FadeDuration` 1.0・`FadeInVolume` 1.0。ソフト参照 `RegularMusicSound`・`PanicMusicSound`・`OverrideMusicSound`。
- `AWasamiMusicPlayerZone2`: 本家の `BP_06_MusicPlayer_Zone2`（`Regular Music` の曲だけを差し替えた子）。
- `IWasamiEnemyInterface::Chasing()`（04 記録）: 本家の `DD_EnemyInterface` の `Chasing`。既定は偽、`AWasamiEnemy` は `IsChasing()`（07 記録）を返す。`IsIntenseMusic()` がこれを見る。
- ツール `WasamiDDTools.import_dd_audio()`（`dd_audio.import_all`）: 曲 3 本（`MUSIC`）・環境音 2 本（`AMBIENCE`）・残響 2 つ（`REVERBS`）の取り込み。戻り値は `music`・`ambience`・`reverbs`。

## 内部構造と処理の流れ
- 部品（本家の SCS）: `DefaultSceneRoot` → `RegularMusic`・`PanicMusic`・`OverrideMusic`（`UAudioComponent`。本家の名前は `Regular Music` など。どれも `bAutoActivate = false` で、フェードインまで鳴らない）。曲は `BeginPlay` でソフト参照から入れる（`WasamiAssets.h`）。本家の音量・減衰の上書きは無く、音量は SoundWave 自身の 0.35（通常）・0.3（追跡）。
- `BeginPlay`: 曲を入れてから `UpdateInterval`（0.5 s）の繰り返しタイマーで `Update`（本家の `K2_SetTimer(Self, 'Update', 0.5, looping)`）。
- `Update` → `FWasamiMusicState::Update(bFadeOut, bOverrideMusic, IsIntenseMusic())` → 返ったフェードを `ApplyFade` で 3 つの部品に流す（`In` = `FadeIn(1, 1, 0, Linear)`、`Out` = `FadeOut(1, 0, Linear)`、`OutIfPlaying` = 鳴っていれば同じ `FadeOut`）。
- `FWasamiMusicState::Update`（本家の `Update` の順そのまま。**どの分岐も DoOnce で囲まれ、逆の分岐がその DoOnce を開け直すので、鳴らし直しは遷移した 1 回だけ**）:
  1. `bFadeOut` が真 → `FadeOutOnce` が通れば 3 つとも `OutIfPlaying`。**真の間はここで終わり**（上書きも追跡も見ない）。偽なら `FadeOutOnce` を開け直して次へ。
  2. `bOverrideMusic` が真 → `OverrideOnOnce` が通れば `Regular` `Out`・`Panic` `Out`・`Override` `In` と `OverrideOffOnce` を開け直す。**真の間はここで終わり**。偽なら `OverrideOffOnce`（**Start Closed**: レベルの始めは何も鳴っていないので流さない）が通れば `Override` `Out` と `OverrideOnOnce` を開け直し、通っても通らなくても次へ。
  3. `IsIntenseMusic()` が真 → `IntenseOnce` が通れば `Regular` `Out`・`Panic` `In` と `CalmOnce` を開け直す。偽 → `CalmOnce` が通れば `Panic` `Out`・`Regular` `In` と `IntenseOnce` を開け直す。
- **`bFadeOut` は 3 の組を開け直さない**（本家のまま）。フェードアウトして同じ状態のまま `bFadeOut` を偽に戻しても曲は戻らず、追跡の有無が変わったときに戻る。Zone 2 の流れが独房の場面の後に `Regular Music` を自分でフェードインするのはこのため（ステップ 2）。
- `IsIntenseMusic()`: ワールドのアクタを回り、敵インターフェースを持つ最初の `Chasing()` が真のもので真（本家の `Get All Actors With Interface(DD_EnemyInterface)` の走査。本家がここで出す `PrintString("<名前> IS CHASING")` は写さない）。
- 置き場所（`dd_level._flow` の `MUSIC_PLAYER_CLASSES`。01 記録）: Zone 1 `BP_06_MusicPlayer_2` (9110, −21600, 0) に `AWasamiMusicPlayer`、Zone 2 `BP_06_MusicPlayer_Zone2_2` (6455.227, −249.887, −8.084) に `AWasamiMusicPlayerZone2`。置かれた値の上書きは Zone 1 の `bFadeOut` 真だけ（`MUSIC_PLAYER_PROPS`）で、レベルは無音で始まる。

## 環境音と残響（ステップ 3）
本家のレベルが自分で持つ音のアクタ。C++ のクラスは作らず、UE の `AAmbientSound`・`AAudioVolume` を本家の値のまま置く（組み立ては `dd_level._flow` の `set_ambient_sound`・`set_audio_volume`。01 記録）。

- Zone 1 の街の環境音 2 つ（どちらも `DD_City_Ambience_Creepy_Loop`。`AudioComponent` が `bOverrideAttenuation` で自前の箱の減衰を持つ。`NaturalSound`）:
  - `DD_City_Ambience_Creepy_Loop2` (11245.70, −21067.76, 397.40)・拡縮 (−9.807852, 1, 1): 音量 0.3・ピッチ 0.3・ローパス 1500 Hz（`bEnableLowPassFilter` 真）、箱 (1922.76, 65049.45, 622.97)・`FalloffDistance` **1605.404**。Y に 650 m 伸びた箱で、ゾーンの始まり（エレベーター前）からガレージまでの窓の外に鳴り続ける。
  - `DD_City_Ambience_Creepy_Loop_3` (4475.70, −23447.76, 517.40): 音量 0.3（ピッチもローパスも既定）、箱 (1922.76, 1447.80, 622.97)・`FalloffDistance` 3000。駐車場の上。
- Zone 2 の館内放送 1 つ: `Nurse_Hospital_Zone01_Event_48_Intercom` (−6651, −136, 128)、`bAutoActivate` **偽**（減衰も上書きも無い素の部品）。本家のレベル BP が鳴らす台詞なので、鳴らすのは項目 20。置くのはこの項目。
- Zone 1 の残響のボリューム 2 つ（`Settings` の `Volume` はどちらも 1.0、`bApplyReverb`・`FadeTime` は既定の真・2.0）:
  - `AudioVolume2` (11255, −6050, 0)・拡縮 (8.796858, 205.202454, 7.914235): `BunkerHall`。
  - `AudioVolume_1` (4480, −23450, 0)・拡縮 (13.989363, 17.642340, 7.914235): `ParkingLot`。
- Zone 1 には放送の箱 `04_Intercom`（`Box` 部品）もあるが、鳴らす中身は台詞なので項目 20。

## 作るアセット
- 曲 `/Game/DD/Audio/06_Hospital/Music/`（`dd_audio.import_music` → `WasamiDDTools.import_dd_audio`）。名前は本家のまま:
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING`（84.396 s・音量 0.35・ループ）
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING`（97.215 s・0.35・ループ）
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING`（81.127 s・0.3・ループ）
  - どれも `SoundClassObject` は `/Game/DD/Audio/SoundMix/DD_SoundClass_Music`（`dd_assets.sound` が export から入れる。01 記録）。
- 環境音 `/Game/DD/Audio/06_Hospital/`（`dd_audio.import_ambience`）: `DD_City_Ambience_Creepy_Loop`（30.272 s・ループ・`SoundClassObject` は `DD_SoundClass_SFX`）、`Nurse_Hospital_Zone01_Event_48_Intercom`（8.474 s・ループせず・`SoundClassObject` 無し = プロジェクトの既定のクラス）。
- 残響 `/Game/DD/_Engine/EngineSounds/ReverbSettings/`（`dd_audio.import_reverbs` → `dd_assets.reverb_effect`）: `BunkerHall`（Gain 0.45・DecayTime 2.0…）、`ParkingLot`（Gain 0.8・DecayTime 1.65…）。エンジンの同名のアセットは使わず、本家の書き出しの値で作り直す（01 記録の `_Engine` の決まり）。

## 原作データの根拠
- 処理: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/08_BearHouse/BP_08_MusicPlayer.txt` の `ReceiveBeginPlay` @879（タイマー 0.5 s・ループ）、ウーバーグラフ @933 からの `Update`（DoOnce の Start Closed は `PopExecutionFlowIfNot` の定数: `@824` False = 開いて始まる、`@605` True = 閉じて始まる〈`OverrideOffOnce`〉）、`Intense Music ?`（@372 の `Chasing`、真で `Result = True`）、`If Playing Fade Out`（`IsPlaying()` のときだけ `FadeOut(1, 0, Linear)`）。`bFadeOut` の分岐が `Override Music` も落とすのは @733 の `PushExecutionFlow 210`。
- 部品と曲: `pak_reference_2/_assets/.../BP_08_MusicPlayer.json`（3 つの `AudioComponent` の `bAutoActivate` 偽）、`BP_06_MusicPlayer.json`（`Regular Music` = Zone 1 通常・`Panic Music` = Panic）、`BP_06_MusicPlayer_Zone2.json`（`Regular Music` = Zone 2 通常）。`Override Music` はどちらも曲が空。
- 置き場所: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`・`_02.full.json`（`BP_06_MusicPlayer_2` の `bFadeOut` 真も）。
- インターフェース: `pak_reference_2/_bytecode/.../Characters/Shared/DD_EnemyInterface.txt` の `Chasing`。
- 環境音と残響: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json` の `DD_City_Ambience_Creepy_Loop2.AudioComponent0`・`_3.AudioComponent0`・`AudioVolume2`・`AudioVolume_1`、`_02.full.json` の `Nurse_Hospital_Zone01_Event_48_Intercom_2.AudioComponent0`。残響の値は `pak_reference_2/_assets/Engine/Content/EngineSounds/ReverbSettings/BunkerHall.json`・`ParkingLot.json`。

## 依存関係
- 使う側: ゾーンの流れ（`AWasamiZone1Flow`・`AWasamiZone2Flow`。11 記録）。`src:BP_06_MusicPlayer_2` / `src:BP_06_MusicPlayer_Zone2_2` のタグで引き（`AWasamiZoneFlow::MusicPlayer`）、Zone 1 は 5 か所で `bFadeOut` を上げ下げし（`SetMusicFadeOut`）、Zone 2 は独房の場面で `FadeRegularMusicIn` を 2 回呼び、脱出で `bFadeOut` を上げる。レベルの組み立て（`dd_level._flow`。01 記録）。
- 見る側: `IWasamiEnemyInterface`（04 記録）の `Chasing` → `AWasamiEnemy::IsChasing()`（07 記録）。
- 取り込み: `dd_assets.sound`（01 記録）。
- エンジン: `UAudioComponent`（`FadeIn` / `FadeOut` / `IsPlaying`、`EAudioFaderCurve::Linear`）、`FTimerManager`、`TActorIterator`。

## 既知の制約・注意点
- `Override Music` は曲が空のまま（本家の病院も空）。`bOverrideMusic` の道だけ残してある。
- 曲は 3D の減衰を持たない（本家も部品に減衰の上書きが無く、SoundWave 自身も素）ので、どこにいても同じ大きさで鳴る。置き場所は本家に合わせてあるだけ。
- テストの `Wasami.Music.Actor` は `/Game/DD/Audio/06_Hospital/Music` の曲が取り込まれていることを前提にする（`import_dd_audio` を先に走らせる）。
- **`bFadeOut` は一時停止の下では効かない**: `Update` は 0.5 s のタイマーなので、止めたゲームでは回らない。Zone 2 の脱出（11 記録の `OnEndTrigger`）が同じフレームで一時停止するため、そこで上げた `bFadeOut` は音にならず、スコア画面の下で曲は鳴り続ける。
- 館内放送（Zone 2 の `Nurse_Hospital_Zone01_Event_48_Intercom_2`）は `bAutoActivate` 偽のまま置いてあるだけで、鳴らす側がまだ無い（本家はレベル BP が鳴らす。台詞なので項目 20）。
- `IsIntenseMusic()` が見るのは敵インターフェースを持つアクタだけなので、Matron（17 記録）は曲を追跡に変えない。本家も同じ（`BP_06_Matron_MiniBoss.json` に `DD_EnemyInterface` は無い）。

## 変更履歴
- 2026-09-20: 初版（作業一覧の項目 19 のステップ 1。`AWasamiMusicPlayer` と曲 3 本の取り込み・配置）。
- 2026-09-20: `FadeRegularMusicIn` を足し、ゾーンの流れからの切り替えを繋いだ（ステップ 2）。
- 2026-09-20: 環境音 2 つ・館内放送 1 つ・残響のボリューム 2 つを本家の値のまま置いた（ステップ 3）。
