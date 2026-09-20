---
title: 曲と環境音と台詞（ゾーンの曲の切り替え・台詞の取り込み）
sources:
  - Source/wasami_deception/WasamiMusicPlayer.h
  - Source/wasami_deception/WasamiMusicPlayer.cpp
  - Source/wasami_deception/WasamiBierceTalk.h
  - Source/wasami_deception/WasamiBierceTalk.cpp
  - Source/wasami_deception/WasamiVoice.h
  - Source/wasami_deception/WasamiVoice.cpp
  - Source/wasami_deception/Tests/WasamiMusicTests.cpp
  - Source/wasami_deception/Tests/WasamiBierceTalkTests.cpp
  - Source/wasami_deception/Tests/WasamiTestBierceTalk.h
  - Source/wasami_deception/Tests/WasamiVoiceTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_audio.py
  - Content/Python/wasami_tools/pipeline/dd_dialogue.py
  - Content/Python/wasami_tools/pipeline/dd_voices.py
  - Tools/dd/prepare_voices.py
updated: 2026-09-20
---

# 曲と環境音と台詞（ゾーンの曲の切り替え・台詞の取り込み）

## 役割
レベル自身が鳴らす音（曲・環境音・残響）と、本家の病院で鳴る効果音の洗い出し。曲は `AWasamiMusicPlayer`（本家 `pak_reference_2` の `Blueprints/06_Hospital/BP_06_MusicPlayer`。処理は親の `Blueprints/08_BearHouse/BP_08_MusicPlayer`）が、ゾーンの通常の曲・追跡の曲・上書きの曲の 3 つを持ち、0.5 s ごとに `bFadeOut`・`bOverrideMusic`・敵が追跡中かを見て 1 s でクロスフェードする。レベルにゾーンごとに 1 体置き、`bFadeOut` を触り Zone 2 の独房の場面で曲を下げ・戻すのはゾーンの流れ（11 記録）。作業一覧の項目 19 のステップ 1・2 で作った。

レベルが自分で鳴らす環境音（`AmbientSound`）と残響のボリューム（`AudioVolume`）も、本家の置き場所と値のまま置く（ステップ 3）。ほかの効果音は、それぞれを作る記録と取り込みの側にある（仕掛け・パワー・敵・UI）。捕獲の音（ホテルの叫びと館のウォッチャーの笑い・斧）は 07 記録の「捕獲の演出」にあり、取り込みは `dd_enemy.import_capture_sounds`（ステップ 4）。本家の病院で鳴る音を洗い出して残りを埋めたのがステップ 5 で、結論は下の「残りの効果音」。

本作のワサミの声（WebGL 版が鳴らしていたユーザーのワサミの台詞）は、`Tools/dd/prepare_voices.py` が wav にし `dd_voices.py` が `/Game/Wasami/Voices` に取り込む（項目 20 のステップ 5。下の「ワサミの声の取り込み」）。鳴らすのは `WasamiVoice`（ステップ 6。下の「ワサミの声を鳴らす口」）で、呼ぶのは場面ごとの記録の側。

## 公開インターフェース
- `EWasamiMusicFade`（`None` / `In` / `Out` / `OutIfPlaying`）: 1 回の `Update` が 1 つの部品に求めるフェード。`OutIfPlaying` は本家の `If Playing Fade Out`（鳴っているときだけ `FadeOut`）。
- `FWasamiMusicFades`: 3 つの部品（`Regular`・`Panic`・`Override`）の 1 回分と `IsSilent()`。
- `FWasamiMusicDoOnce`: 本家の DoOnce ノード 1 つ。`Enter()`（通したら真。以後は Reset まで偽）・`Reset()`・`IsClosed()`。`FWasamiMusicDoOnce{true}` は Start Closed。
- `FWasamiMusicState`: `Update(bFadeOut, bOverrideMusic, bIntense)` → `FWasamiMusicFades`。ワールド無しで試せる純粋な構造体で、5 つの DoOnce を持つ。
- `AWasamiMusicPlayer`（`AActor`）: `bFadeOut`・`bOverrideMusic`（どちらも `EditAnywhere, BlueprintReadWrite`）、`Update()`（`BlueprintCallable`）、`IsIntenseMusic()`（`BlueprintPure`。本家の `Intense Music ?`）、`FadeRegularMusicIn(Duration, Volume)`（`BlueprintCallable`。`Regular Music` の `FadeIn(Duration, Volume, 0, Linear)` だけを流す。Zone 2 の独房の場面が使う）、`GetLastFades()`（直前の `Update` が流したフェード。テストと道具のため）、`GetLastRegularFadeInDuration()`・`GetLastRegularFadeInVolume()`（直前の `FadeRegularMusicIn`。まだ無ければ長さが負。同じくテストのため）、部品の取り出し `GetRegularMusic()`・`GetPanicMusic()`・`GetOverrideMusic()`。定数 `UpdateInterval` 0.5・`FadeDuration` 1.0・`FadeInVolume` 1.0。ソフト参照 `RegularMusicSound`・`PanicMusicSound`・`OverrideMusicSound`。
- `AWasamiMusicPlayerZone2`: 本家の `BP_06_MusicPlayer_Zone2`（`Regular Music` の曲だけを差し替えた子）。
- `IWasamiEnemyInterface::Chasing()`（04 記録）: 本家の `DD_EnemyInterface` の `Chasing`。既定は偽、`AWasamiEnemy` は `IsChasing()`（07 記録）を返す。`IsIntenseMusic()` がこれを見る。
- ツール `WasamiDDTools.import_dd_audio()`（`dd_audio.import_all`）: 曲 3 本（`MUSIC`）・環境音 2 本（`AMBIENCE`）・残響 2 つ（`REVERBS`）の取り込み。戻り値は `music`・`ambience`・`reverbs`。
- `EWasamiTalkStep`（`Nothing` / `Play` / `Wait` / `AlreadyWaiting`）と `WasamiTalkStep(bHalt, bPlaying, bWaiting)`: 本家のウーバーグラフに 1 回入ったときの行き先。ワールド無しで試せる純粋な関数。
- `AWasamiBierceTalk`（`AActor`）: `bHalt`（`EditAnywhere, BlueprintReadWrite`）、`Talk(WhatToSay, bAttenuate)`・`StopTalking()`（どちらも `BlueprintCallable`）、`Find(WorldContext)`（静的。本家の `BP_DD_Functions` の `Bierce Talk` = `GetAllActorsOfClass` の 0 番目）、`GetAudioComponent()`、`GetPendingSound()`・`IsWaiting()`・`GetLastStep()`（テストのため）。定数 `WaitInterval` 0.5。ソフト参照 `Attenuation`。`IsSpeaking()` は `protected virtual`（音声装置の無い自動テストで鳴っている状態を作るため。`AWasamiTestBierceTalk`）。
- ツール `WasamiDDTools.import_dd_dialogue()`（`dd_dialogue.import_all`）: 病院の台詞の取り込み。戻り値は `lines` 9・`quips` 5・`cues` 1・`intercom` 1。`dd_dialogue` の公開は `LINES`・`GAMEPLAY_CUE`・`GAMEPLAY_WAVES`・`INTERCOM`（取り込む波）、`strings()`（本家の文字列表の中身）、`subtitle_key(name)`（波の名前 → 文言の鍵）、`line(rel, entries)`（字幕付きで 1 本取り込む）。
- ツール `WasamiDDTools.import_wasami_voices()`（`dd_voices.import_all`）: ワサミの声の取り込み。戻り値は `subtitled` 5・`silent` 6。`dd_voices` の公開は `SUBTITLED`・`SILENT`・`CLIPS`（取り込む id）、`SOURCE`・`VOICES_ROOT`・`SOUND_CLASS`、`asset_name(id)`（id → 波の名前）、`manifest()`（id → 原本の中身）、`voice(clip, sound_class, subtitled)`（1 本取り込む）。

- `EWasamiVoice`（`Greeting` / `Well` / `Fast` / `Best` / `Found` / `Calling` / `Others` / `Think` / `Remember` / `Fine` / `Over`）と名前空間 `WasamiVoice`: ワサミの声 11 本と鳴らし方。`Num()`・`Path(Id)`（`/Game/Wasami/Voices/Wasami_Greeting.Wasami_Greeting` …）・`Seconds(Id)`（原本の長さ）・`SubtitleSeconds(Id)`（字幕の出ている長さ）はワールド無しで試せる純粋な関数、`Load(Id)`（波を今読む）・`Say(WorldContext, Id, Volume = 1)`（2D で鳴らし、字幕があれば `SubtitleSeconds` の間出す。部品を返す）・`ShowSubtitle(WorldContext, Id)`（字幕だけ）はワールドが要る。

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

## 残りの効果音（ステップ 5）
本家の病院の両ゾーンで鳴る音を 3 つの筋から拾い、本作の `Content/DD` にある波と突き合わせた（台詞は項目 20 なので除く）。**穴は敵の移動音 1 つだけ**で、それを埋めて項目 19 を閉じた。洗い出しの筋と、そこから出た音の行き先:

1. **レベルの `AudioComponent`**（Zone 1 = 43・Zone 2 = 146）。ほとんどは置かれた BP の SCS の部品なので、音は BP 側にある。持ち主で数えると Zone 1 は `BP_06_Defib` 23（`DD_TT_Defibrillator_Zap`。08 記録）・曲 3・`BP_Collectable` 2（`Bierce_Secret_Files_Pickup`。18 記録）・`BP_FakeUseActor_…_Elevator` 5（`DD_TT_Elevator_Doors_Open`。18 記録）・`BP_SpeedBarrier` 4 と `BP_ZoneBarrier` 1（`Barrier_Loop`。08・11 記録）・`BierceTalk` 1（台詞）・環境音 2・ガレージのリフト 2（`DD_TT_GarageLift_Up`・`_Down`。08 記録）。Zone 2 は defib 13・ガレージのリフト 2・曲 3・`BP_Collectable` 1・`BP_ZoneBarrier` 1・`BierceTalk` 1・館内放送 1・のこぎりの罠 73（`SFX_Matron_SawLoop`。08 記録）・リフト 15 × 2（`DD_TT_GarageLift_Down`・`DD_TT_Lift_Loop`）・ナース 6 × 3（`Audio` = `20-Elevator_Slams`、`Skate Audio` = `DD_Rollerskating_Fast_V1_LOOP`、`Talk Audio` = 台詞）。**ナースの `Skate Audio` 以外はすべて実装済み**だった。
2. **レベル BP**（`_bytecode/…/06_Hospital_Zone_01.txt`・`_02.txt`）。台詞のほかは `21-Ballroom_portal_V2`（12 記録）・`DD_TT_Door_BustedOpen_02`（08 記録）・`DD_Needle_Trap_R1_V3`・`Ring_Piece_Pickup_v1` で、どれも実装済み。
3. **置かれた BP とその親、プレイヤー・敵・ゲームモード**の `imports`（音のパスだけを拾い、親クラスをたどる）。本作に無いのは台詞の Cue 7 本（`Nurse_Hospital_Zone01_Detected`・`Laugh`・`Patrol`・`Pursuit`・`Stunned`・`Decloak`・`PillToss`）と Bierce・Malak の全回収の台詞（どれも項目 20）、それに `DD_Syringe_Stab_Impale_R1_V2` と `RL_bodyfall_…_Impact_10` の 2 本。後の 2 本はナースの `Jumpscare Handle`（その場で捕まえる演出）のもので、本作の捕獲は別室（07 記録の「捕獲の演出」）なので鳴らす相手がいない。

- 埋めた 1 つ（敵の移動音）は 07 記録の「移動音」にある（`AWasamiEnemy::SkateAudio`・`UpdateSkateSound`、取り込みは `dd_enemy.import_move_sound`）。
- 残るのは台詞だけ（項目 20）。館内放送（Zone 2）と放送の箱 `04_Intercom`（Zone 1）も鳴らす中身は台詞なので、置いてあるだけで鳴らす側は項目 20。

## 台詞の取り込み（項目 20 のステップ 1）
本家の病院の台詞は SoundWave そのもの（`pak_reference_2` の `Audio/Dialogue/Bierce/Ch06/TT/`。音量 2.0・`DD_SoundClass_Dialogue`）で、鳴らすのは本家の `BierceTalk_Blueprint` とレベル BP。本作が作る Zone 1・Zone 2 が鳴らす分だけを `dd_dialogue.py` で取り込む（入口 `06_Hospital` の `Event_01〜08` とボス戦の `Event_11〜16`・`18B`・`23`・`24` は作らないので取り込まない）。鳴らす側は次のステップ以降（話し役 `AWasamiBierceTalk` と両ゾーンの流れ。11 記録）。

- `LINES` 9 本（ゾーンが直に喋らせる）: Zone 1 = `Event_10`（扉の破壊 `04_DoorBreak`）・`Event_09`（館内放送の 13 s 後 `04_Intercom`）、Zone 2 = `Event_17`（独房の場面の後）・`Gameplay_07`（リフト）・`Gameplay_08`（迷路の始まり）・`Event_20`（迷路のシャード全回収）・`Event_21`（欠片の回収の 1 s 後）・`Event_22`（ガレージ）・`Event_19`（Matron）。
- `GAMEPLAY_WAVES` 5 本と `GAMEPLAY_CUE`: 近づいたナースが出す一言。Cue `Bierce_TormentTherapy_Gameplay` の `SoundNodeRandom` が重み 1 で `Gameplay_05 / 02 / 04 / 01 / 03` から 1 本選び、Cue 自身が `DialogueAttenuation`（`bAttenuate` 偽・`OmniRadius` 350・`FalloffDistance` 5000）を通す。本家に `Gameplay_06` は無い。
- `INTERCOM` 1 本: Zone 1 の流れが `PlaySound2D`（音量 0.6）で鳴らす `Nurse_Hospital_Zone01_Event_37_Intercom`（12.024 s・2 ch・`SoundClassObject` 無し）。Zone 2 の館内放送は別の波で、`AmbientSound` として置いてある（上の「環境音と残響」）。
- **字幕**: UE の仕組みそのまま（`USoundWave.Subtitles` に `{Text, Time}`。オプションの SUBTITLES が `UGameplayStatics::SetSubtitlesEnabled` を切り替える。15 記録）。本家の病院の台詞の波は `Subtitles` が空なので（字幕を持つのは本作が作らない入口のナースの分だけ）、本家の文字列表 `Blueprints/Main/Strings/Strings` の文言を**名前の対応**で 0 s に 1 つ入れる（`SUBTITLE_PREFIXES`・`SUBTITLE_KEYS`）: `Bierce_TormentTherapy_Event_NN` → `06_Cutscene_Zone_01_Bierce_NN`、`..._Gameplay_NN` → `06_Gameplay_Zone_01_Bierce_NN`、館内放送 → `06_Cutscene_Zone_01_Nurse_01`。第 4 章の下水の Bierce（字幕を持つ）が同じ対応で鍵を持ち、9 本すべて鳴らす場面と文言が合う（リフトの `Gameplay_07` =「handicap accessible nightmare」、Matron の `Event_19` =「find a way to get past her」など）。文字列表のアセットは作らず、`dd_level` の秘密の書き置きと同じく素の `unreal.Text` で入れる。

## 話し役 `AWasamiBierceTalk`（項目 20 のステップ 2）
本家の `BierceTalk_Blueprint`（`Blueprints/00_Ballroom`）を写したもの。本家は `AmbientSound` の子で、部品は `AudioComponent0` 1 つだけ（それがアクタの根。`bAutoActivate` 偽・`AttenuationSettings` は `DialogueAttenuation`）。本作は `AActor` にその部品を同じ値で作る（`bStopWhenOwnerDestroyed`・`bShouldRemainActiveIfDropped` 真・`Movable` も親の `AAmbientSound` のまま。減衰は `BeginPlay` でソフト参照から入れる）。本家の BP の既定の音（舞踏場の台詞）は使わないので入れない。

- `Talk(What To Say, Attenuate?)`: 本家はまず両方をウーバーグラフのフレームに書き（＝ `Halt` で止まる呼びでも `PendingSound` と `bAllowSpatialization` は変わる）、部品の `bAllowSpatialization` に `Attenuate?` を入れてから `Halt` を見る。`Halt` が真なら何もしない。偽なら「部品が鳴っていれば `Delay 0.5` で待ち、空いたら `SetSound` → `Play(0)`」。**前の台詞は絶対に切らない**。病院の呼びは Zone 2 の迷路の始まりの `Gameplay_08` だけが `Attenuate? = True`（話し役の居る所から聞こえる）で、ほかはすべて `False`（画面の外でも同じ音量の 2D）。
- `StopTalking()`: 部品の `Stop()` だけ。走っている待ちは止めない（本家のまま）。
- 待ちの写し方: Blueprint の `Delay` は**走っている間の再入を無視する**ので、待ちの最中の `Talk` は待ちを増やさず、`PendingSound` だけが置き換わる（＝待ちが明けたときに鳴るのは最後に頼まれた台詞）。これを `WasamiTalkStep` の `AlreadyWaiting` として写し、待ちは 0.5 s の単発タイマー `WaitTimer` の繰り返しで作る。タイマーの戻りは `Halt` の手前ではなくループの中（本家の `Delay` の戻り先 @15）に入るので、待っている間に `Halt` が上がっても鳴る。
- 置き場所（`dd_level._flow` の `BIERCE_TALK_CLASS`。01 記録）: 両ゾーンの `BierceTalk_Blueprint_2` — Zone 1 (10560, −21175, 0)、Zone 2 (−10940, −865, 800)。置かれた値の上書きは無い。フォルダは `Hospital/Audio`、タグは `src:BierceTalk_Blueprint_2`。
- 喋らせる側は両ゾーンの流れ（ステップ 3・4。11 記録）。本家のレベル BP の呼び口は 2 つあり（`BP_DD_Functions` の `Bierce Talk` と、レベルが持つアクタの参照）、どちらも同じ 1 体に届く。

## ワサミの声の取り込み（項目 20 のステップ 5）
本作のワサミの声は WebGL 版が鳴らしていた 15 本（原本は `<WEBGL>/public/voices/` の mp3 と `manifest.json`。-23 LUFS・-6 dBTP に揃えた書き出しで、id・場面の区分・字幕・長さを持つ。`<WEBGL>/voices/` の wav 55 本は切り出す前の素材で mp3 と対応が取れないので使わない）。**UE は mp3 を取り込めない**ので、`python Tools/dd/prepare_voices.py` が ffmpeg で 16 bit PCM の wav に**復号するだけ**（元の 1 ch・44.1 kHz のまま＝ WebGL 版で揃えた音量をそのまま）で `SourceArt/Wasami/Voices/` に書き、`manifest.json` を隣に写す。手作りの素材なので wav は Git LFS（`.gitattributes` の `SourceArt/**/*.wav`）、取り込んだ `/Game/Wasami/Voices` の波は作り直せるので git の外（`.gitignore` の `/Content/Wasami/`）。

- 取り込むのは **WebGL 版が実際に鳴らす 11 本だけ**（`dd_voices.CLIPS`）。本家の台詞で本作が作らないレベルの分を取り込まないのと同じで、`manifest.json` に在っても鳴らさない `follow`・`safe`・`wait`・`you` の 4 本は作らない（wav は原本の一式として `SourceArt` に置いてある）。
- **字幕を出す 5 本**（`SUBTITLED`）: `greeting`（開始）・`well`（1 個目の回収）・`fast`（ブースト成功）・`best`（隠し扉が開いた後）と、敵の発見の `found`（WebGL 版は `onSpotted` で字幕を出す）。**字幕を出さない 6 本**（`SILENT`）: 巡回の `calling`・`others`・`think`・`remember` と、死亡画面の `fine`・`over`。字幕は `manifest.json` の文言をそのまま `Subtitles` の 0 s に 1 つ入れ（台詞と同じ UE の仕組み。`dd_assets.sound_file`）、出さない 6 本は空のまま入れる。
- 波は `/Game/Wasami/Voices/Wasami_<Id>`（`Wasami_Greeting` …）。値は UE の既定のまま（音量 1・ピッチ 1・ループせず）で、`SoundClassObject` は本家の `DD_SoundClass_Dialogue` — WebGL 版のバス `voice`（音量 1.0）に当たるクラスで、本家の台詞と同じミックスに乗る。
- 長さ（`manifest.json` と取り込んだ波で一致）: `greeting` 3.878・`well` 0.705・`fast` 0.637・`best` 0.517・`found` 0.622・`calling` 1.027・`others` 0.690・`think` 0.862・`remember` 0.937・`fine` 1.784・`over` 0.727 s。
- 鳴らす場面（WebGL 版の 06・15 記録。位置と音量もそこに書かれている: 敵は頭の位置で `found` 1.0・巡回 0.9）は次のステップで各所に付ける。

## ワサミの声を鳴らす口（項目 20 のステップ 6）
`WasamiVoice`（`Source/wasami_deception/WasamiVoice.h`・`.cpp`）が声 11 本の表（パッケージと原本の長さ）と鳴らし方を持つ。本家に当たるものは無いので、鳴らす場面・音量・字幕の長さは WebGL 版の実装記録 06・15 に倣う。

- `Say(WorldContext, Id, Volume = 1)`: `CreateSound2D`（`bAutoDestroy`）で鳴らす。バスは波の `DD_SoundClass_Dialogue`（WebGL 版の `voice` バスに当たる）なので、音量は既定の 1 のまま。
- **字幕は波のものを止めて出し直す**: `UAudioComponent::bSuppressSubtitles` を真にしてから `Play` し、`FSubtitleManager::GetSubtitleManager()->QueueSubtitles(波, 波の優先度, 波の折り返し・1 行, SubtitleSeconds(Id), 波の Subtitles, 0, ワールドの音声時刻)` を自分で呼ぶ。`USoundWave::HandleStart` と同じ呼びで、長さだけ `SubtitleSeconds` に替えたもの。字幕の鍵に波そのもののアドレスを使うので、同じ声を続けて鳴らすと前の行を置き換える。字幕の可否（オプションの SUBTITLES）は出す側では見ない（エンジンが描くときに `GEngine->bSubtitlesEnabled` を見る）。
- `SubtitleSeconds(Id)` = `max(2.2, 長さ + 1.2)`、`Found` だけ 2.4 固定（WebGL 版の `hud.subtitle` と敵の `onSpotted`）。
- 鳴らす場所（どれも本家に無い、本作だけのもの）:
  - `greeting` … Zone 1 の `InitialStart` の 10 s の据え置きが明けて操作が戻るところ（11 記録）。新しい始まりだけが通る道なので、WebGL 版の `greet()`（チェックポイント 1 で 1 個も取っていないとき）と同じになる。
  - `well` … `AWasamiShard::Collect` で、ゲームインスタンスの `ShardsToBeRemoved` が 1 個目になり、タブレットの残りが 1 以上のとき（06 記録）。
  - `fast` … `UWasamiPowerComponent::UseSpeedBoost` の頭（04 記録）。
  - `best` … `AWasamiSecretWall` の最初の使用から 0.5 s 後（18 記録）。本家は秘密の壁で何も喋らず、Bierce が喋るのは書類のほう。
  - `fine`・`over` … 死亡画面の `LifeLost` の段（ライフが残るとき、`Life_Lost` と一緒）と `GameOver` の段（`66_-_Game_Over` と一緒）。どちらも字幕なしで、音量 1（09 記録）。ゲームオーバーの 1.25 s 後の笑い声は鳴らさない。
  - `found` と巡回の 4 本 … 敵の `Talk`（`AWasamiEnemy` の `TalkAudio`。07 記録）。`Say` の 2D ではなく敵の口から `AgathaAttenuation` 越しに鳴る。発見はゲームモードの `TakeFoundVoice()` が全体で 12 s に 1 回に絞り、巡回の 4 本は 14〜26 s ごと。
- 死亡画面だけは `Say` ではなく画面自身の `PlaySound`（= `PlaySound2D`）で鳴らす。ゲームを止めた下で鳴る UI の音で、字幕も要らないため。

## 作るアセット
- 曲 `/Game/DD/Audio/06_Hospital/Music/`（`dd_audio.import_music` → `WasamiDDTools.import_dd_audio`）。名前は本家のまま:
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING`（84.396 s・音量 0.35・ループ）
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING`（97.215 s・0.35・ループ）
  - `DD_-_Dark_Deception_-_Chapter_4_Hospital_-_Panic_Track_v1_2_-_LOOPING`（81.127 s・0.3・ループ）
  - どれも `SoundClassObject` は `/Game/DD/Audio/SoundMix/DD_SoundClass_Music`（`dd_assets.sound` が export から入れる。01 記録）。
- 環境音 `/Game/DD/Audio/06_Hospital/`（`dd_audio.import_ambience`）: `DD_City_Ambience_Creepy_Loop`（30.272 s・ループ・`SoundClassObject` は `DD_SoundClass_SFX`）、`Nurse_Hospital_Zone01_Event_48_Intercom`（8.474 s・ループせず・`SoundClassObject` 無し = プロジェクトの既定のクラス）。
- 台詞 `/Game/DD/Audio/Dialogue/Bierce/Ch06/TT/`（`dd_dialogue.import_all` → `WasamiDDTools.import_dd_dialogue`）: `Bierce_TormentTherapy_Event_09`（2.926 s）・`_10`（5.039）・`_17`（3.251）・`_19`（3.529）・`_20`（2.235）・`_21`（3.367）・`_22`（3.901）、`Bierce_TormentTherapy_Gameplay_01`（1.811）・`_02`（2.868）・`_03`（4.679）・`_04`（5.387）・`_05`（3.274）・`_07`（4.249）・`_08`（5.689）。どれも音量 2.0・ピッチ 1・1 ch・`DD_SoundClass_Dialogue`・字幕 1 つ。Cue `Bierce_TormentTherapy_Gameplay`（`SoundClassObject` 無し・`AttenuationSettings` は `/Game/DD/Audio/Misc/DialogueAttenuation`）。館内放送 `/Game/DD/Audio/06_Hospital/Nurse_Hospital_Zone01_Event_37_Intercom`（12.024 s・音量 1・2 ch・クラス無し・字幕 1 つ）。
- ワサミの声 `/Game/Wasami/Voices/`（`dd_voices.import_all` → `WasamiDDTools.import_wasami_voices`。原本は `SourceArt/Wasami/Voices/*.wav`）: `Wasami_Greeting`・`Wasami_Well`・`Wasami_Fast`・`Wasami_Best`・`Wasami_Found`（字幕 1 つ）と `Wasami_Calling`・`Wasami_Others`・`Wasami_Think`・`Wasami_Remember`・`Wasami_Fine`・`Wasami_Over`（字幕なし）。どれも音量 1・1 ch・44.1 kHz・`DD_SoundClass_Dialogue`。
- 残響 `/Game/DD/_Engine/EngineSounds/ReverbSettings/`（`dd_audio.import_reverbs` → `dd_assets.reverb_effect`）: `BunkerHall`（Gain 0.45・DecayTime 2.0…）、`ParkingLot`（Gain 0.8・DecayTime 1.65…）。エンジンの同名のアセットは使わず、本家の書き出しの値で作り直す（01 記録の `_Engine` の決まり）。

## 原作データの根拠
- 処理: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/08_BearHouse/BP_08_MusicPlayer.txt` の `ReceiveBeginPlay` @879（タイマー 0.5 s・ループ）、ウーバーグラフ @933 からの `Update`（DoOnce の Start Closed は `PopExecutionFlowIfNot` の定数: `@824` False = 開いて始まる、`@605` True = 閉じて始まる〈`OverrideOffOnce`〉）、`Intense Music ?`（@372 の `Chasing`、真で `Result = True`）、`If Playing Fade Out`（`IsPlaying()` のときだけ `FadeOut(1, 0, Linear)`）。`bFadeOut` の分岐が `Override Music` も落とすのは @733 の `PushExecutionFlow 210`。
- 部品と曲: `pak_reference_2/_assets/.../BP_08_MusicPlayer.json`（3 つの `AudioComponent` の `bAutoActivate` 偽）、`BP_06_MusicPlayer.json`（`Regular Music` = Zone 1 通常・`Panic Music` = Panic）、`BP_06_MusicPlayer_Zone2.json`（`Regular Music` = Zone 2 通常）。`Override Music` はどちらも曲が空。
- 置き場所: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`・`_02.full.json`（`BP_06_MusicPlayer_2` の `bFadeOut` 真も）。
- インターフェース: `pak_reference_2/_bytecode/.../Characters/Shared/DD_EnemyInterface.txt` の `Chasing`。
- 台詞: `pak_reference_2/_assets/DDeception/Content/Audio/Dialogue/Bierce/Ch06/TT/*.json`（音量 2.0・クラス・長さ）と `Bierce_TormentTherapy_Gameplay.json`（`SoundNodeRandom` の重みと 5 本の順、`DialogueAttenuation`）、`Audio/06_Hospital/Nurse_Hospital_Zone01_Event_37_Intercom.json`。鳴らす場面は `_bytecode/.../06_Hospital_Zone_01.txt`・`_02.txt` と `Blueprints/00_Ballroom/BierceTalk_Blueprint.txt`。文言は `_assets/.../Blueprints/Main/Strings/Strings.json` の `string_table`、字幕の入れ方の手本は `Audio/Dialogue/Bierce/Ch04/Bierce_Sewer_01.json`（`Subtitles` が `Strings` の `04_Sewer_BierceDialogue_01` を指す）。
- 話し役: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/00_Ballroom/BierceTalk_Blueprint.txt`（`Talk` → ウーバーグラフ @213 の `bAllowSpatialization` → @254 の `Halt` → @15 の `IsPlaying` と `Delay 0.5`〈戻り先 @15〉→ @130 の `SetSound` → @171 の `Play(0.0)`、`Stop Talking` → @269 の `Stop`）と `_assets/…/BierceTalk_Blueprint.json`（親が `AmbientSound`、`AudioComponent0` の `bAutoActivate` 偽・`AttenuationSettings` = `DialogueAttenuation`）。置き場所は `_levels/06_Hospital_Zone_01.full.json`・`_02.full.json` の `BierceTalk_Blueprint_2`。`Halt` を上げる呼びは病院のどこにも無い。
- 環境音と残響: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json` の `DD_City_Ambience_Creepy_Loop2.AudioComponent0`・`_3.AudioComponent0`・`AudioVolume2`・`AudioVolume_1`、`_02.full.json` の `Nurse_Hospital_Zone01_Event_48_Intercom_2.AudioComponent0`。残響の値は `pak_reference_2/_assets/Engine/Content/EngineSounds/ReverbSettings/BunkerHall.json`・`ParkingLot.json`。
- ワサミの声: 原本は WebGL 版の `public/voices/`（`manifest.json` の `clips[]` の id・`subtitle`・`duration`）。鳴らす場面と音量・字幕は WebGL 版の実装記録 06（`Game.say` の 4 本と死亡画面の 2 本、バス `voice`）と 15（敵の `found` と巡回の 4 本を頭の位置で）。本家に当たるものは無い（本家の Bierce に当たる案内役がワサミ）。

## 依存関係
- 使う側: ゾーンの流れ（`AWasamiZone1Flow`・`AWasamiZone2Flow`。11 記録）。`src:BP_06_MusicPlayer_2` / `src:BP_06_MusicPlayer_Zone2_2` のタグで引き（`AWasamiZoneFlow::MusicPlayer`）、Zone 1 は 5 か所で `bFadeOut` を上げ下げし（`SetMusicFadeOut`）、Zone 2 は独房の場面で `FadeRegularMusicIn` を 2 回呼び、脱出で `bFadeOut` を上げる。レベルの組み立て（`dd_level._flow`。01 記録）。
- 見る側: `IWasamiEnemyInterface`（04 記録）の `Chasing` → `AWasamiEnemy::IsChasing()`（07 記録）。
- 話し役を使う側: 両ゾーンの流れ（11 記録。`src:BierceTalk_Blueprint_2` のタグか `AWasamiBierceTalk::Find`）。減衰 `/Game/DD/Audio/Misc/DialogueAttenuation`（`dd_gimmicks` が取り込む。08 記録）。
- 取り込み: `dd_assets.sound`・`dd_assets.sound_file`（01 記録）。ワサミの声の原本は `Tools/dd/prepare_voices.py` （ffmpeg が要る）が WebGL 版から作る。
- エンジン: `UAudioComponent`（`FadeIn` / `FadeOut` / `IsPlaying`、`EAudioFaderCurve::Linear`）、`FTimerManager`、`TActorIterator`。

## 既知の制約・注意点
- `Override Music` は曲が空のまま（本家の病院も空）。`bOverrideMusic` の道だけ残してある。
- 曲は 3D の減衰を持たない（本家も部品に減衰の上書きが無く、SoundWave 自身も素）ので、どこにいても同じ大きさで鳴る。置き場所は本家に合わせてあるだけ。
- テストの `Wasami.Music.Actor` は `/Game/DD/Audio/06_Hospital/Music` の曲が取り込まれていることを前提にする（`import_dd_audio` を先に走らせる）。
- **`bFadeOut` は一時停止の下では効かない**: `Update` は 0.5 s のタイマーなので、止めたゲームでは回らない。Zone 2 の脱出（11 記録の `OnEndTrigger`）が同じフレームで一時停止するため、そこで上げた `bFadeOut` は音にならず、スコア画面の下で曲は鳴り続ける。
- 館内放送（Zone 2 の `Nurse_Hospital_Zone01_Event_48_Intercom_2`）は `bAutoActivate` 偽のまま置いてあるだけで、鳴らす側がまだ無い（本家はレベル BP が鳴らす。台詞なので項目 20）。
- 話し役の待ちの戻り（`Resume`）には「待ちは無い」を値で渡す（`Step(false, false)`）。`FTimerManager::IsTimerActive` は**自分のコールバックの最中も真**なので、そこで `IsWaiting()` を見ると待ちの回が自分を「もう待っている」と誤り、台詞が二度と鳴らない（症状索引の「タイマーのコールバックの中で `IsTimerActive` が真を返す」）。
- 話し役は音声装置の無い自動テストでは鳴っている状態を作れない（`UAudioComponent::Play` は装置が無いと何もしない）ので、`IsSpeaking()` を `virtual` にして `AWasamiTestBierceTalk` が差し替える。待ちの分岐そのものは純粋な `WasamiTalkStep` でも試す。
- ワサミの声の字幕は**波のものを止めて `WasamiVoice` が出し直す**（下の「ワサミの声を鳴らす口」）。波のままだと UE の `FSubtitleManager` が音の終わりで消すので、0.5〜0.7 s の声は読む間が無い。**`Say` を通さずに波をそのまま鳴らすと、短いままの字幕が出る**（自前の部品で鳴らすなら `bSuppressSubtitles` を真にして `ShowSubtitle` を呼ぶ。敵の `Talk` がその形）。
- **字幕の長さは実時間で数える**: `UWorld::AudioTimeSeconds` は `slomo` の時間の伸縮を受けない（`LevelTick.cpp`「Audio always plays at real-time regardless of time dilation」）。`slomo 0.1` にしても字幕は 2.4 s の実時間で消えるので、PIE で撮るときは収録して後からコマを見る。
- `IsIntenseMusic()` が見るのは敵インターフェースを持つアクタだけなので、Matron（17 記録）は曲を追跡に変えない。本家も同じ（`BP_06_Matron_MiniBoss.json` に `DD_EnemyInterface` は無い）。

## 変更履歴
- 2026-09-20: 初版（作業一覧の項目 19 のステップ 1。`AWasamiMusicPlayer` と曲 3 本の取り込み・配置）。
- 2026-09-20: `FadeRegularMusicIn` を足し、ゾーンの流れからの切り替えを繋いだ（ステップ 2）。
- 2026-09-20: 環境音 2 つ・館内放送 1 つ・残響のボリューム 2 つを本家の値のまま置いた（ステップ 3）。
- 2026-09-20: 残りの効果音を洗い出し、埋めた 1 つ（敵の移動音）と残り（台詞だけ）を書いた（ステップ 5）。
- 2026-09-20: 病院の台詞の波 15 本と一言の Cue を字幕付きで取り込むようにした（`dd_dialogue.py`・`WasamiDDTools.import_dd_dialogue`。作業一覧の項目 20 のステップ 1）。
- 2026-09-20: 話し役 `AWasamiBierceTalk` を作り、両ゾーンに 1 体ずつ置くようにした（項目 20 のステップ 2）。
- 2026-09-20: ワサミの声 11 本を `/Game/Wasami/Voices` に取り込むようにした（`Tools/dd/prepare_voices.py`・`dd_voices.py`・`WasamiDDTools.import_wasami_voices`。項目 20 のステップ 5）。
- 2026-09-20: 鳴らす口 `WasamiVoice` を作り、2D の 6 本（`greeting`・`well`・`fast`・`best`・`fine`・`over`）を場面に付けた（項目 20 のステップ 6）。
- 2026-09-20: 残りの 5 本（`found` と巡回の 4 本）を敵に付けた（項目 20 のステップ 7。中身は 07 記録の「声」）。
