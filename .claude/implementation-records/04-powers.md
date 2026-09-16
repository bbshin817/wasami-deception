---
title: タブレットのパワー（枠・入力・ゲージ・強化段階・スピードブーストとその演出・テレポーテーション・カメラアニメ・FX）
sources:
  - Source/wasami_deception/WasamiPowerTypes.h
  - Source/wasami_deception/WasamiPowerTypes.cpp
  - Source/wasami_deception/WasamiPowerComponent.h
  - Source/wasami_deception/WasamiPowerComponent.cpp
  - Source/wasami_deception/WasamiEnemyInterface.h
  - Source/wasami_deception/WasamiTelekinesisInterface.h
  - Source/wasami_deception/WasamiCameraAnim.h
  - Source/wasami_deception/WasamiCameraAnim.cpp
  - Source/wasami_deception/WasamiChameleonComponent.h
  - Source/wasami_deception/WasamiChameleonComponent.cpp
  - Source/wasami_deception/WasamiSpeedBoostWidget.h
  - Source/wasami_deception/WasamiSpeedBoostWidget.cpp
  - Source/wasami_deception/WasamiTeleportAim.h
  - Source/wasami_deception/WasamiTeleportAim.cpp
  - Source/wasami_deception/Tests/WasamiPowerTests.cpp
  - Source/wasami_deception/Tests/WasamiCameraAnimTests.cpp
updated: 2026-09-16
---

# タブレットのパワー

## 役割
本家 Dark Deception のタブレットのパワー 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）の土台。本家がプレイヤー（`BP_DD_PlayerCharacter`）・ゲージのアクタ（`BP_Powers`）・タブレットの枠（`UMG_TabletPowers`）に分けて持つものを、プレイヤーに付ける `UWasamiPowerComponent` 1 つにまとめる。いま中身まであるのはスピードブースト（演出を含む）とテレポーテーションの仕組み（照準・移動・取り消し・再使用。カメラアニメと見た目はまだ）で、ほかの 4 種は枠に出て入力を受けるところまで（進捗記録 `.claude/progress/20260916-tablet-powers.md` のステップ 4b〜10 で足す）。パワーの演出に使う共通の部品として、UE 5 に無い UE4 の `CameraAnim` の再生（`UWasamiCameraAnim` / `UWasamiCameraAnimModifier`）と、プレイヤーの FX（本家の Chameleon、`UWasamiChameleonComponent`）もここに書く。原作の調査は `.claude/references/powers/`。**テレポーテーションだけ `pak_reference`（旧版）、それ以外は `pak_reference_2`（最新版）に従う**（ユーザーの指示）。

## 公開インターフェース

### `WasamiPowerTypes.h`
- `EWasamiPower : uint8` … `SpeedBoost`・`Teleport`・`Telepathy`・`PrimalFear`・`Telekinesis`・`Vanish`・`None`（本家の `Enum_RingAltar_Skills` の並び）。`WasamiPowerCount` = 6。
- `FWasamiPowerSlot`（USTRUCT）… `Power`（既定 SpeedBoost）と `bAvailable`（既定 false）。本家の `Struct_Power`。
- `FWasamiPowerTuning` … 強化段階ごとの値（下の表）。`ForLevel(Level)`（0〜5 に丸める）、定数 `TeleportCooldown` 5・`VanishDuration` 15・`MaxLevel` 5。
- `FWasamiPowerGauge` … `BP_Powers` のタイムライン 1 本と、それがアイコンに書く `Percent`。`SetDelay(Seconds, bTeleport)`・`Stop()`・`Tick(DeltaSeconds)`・`IsPlaying()`・`Percent`。

### `UWasamiPowerComponent : UActorComponent`（`AWasamiPlayerCharacter` の `Powers`）
- 入力: `UsePowerLeftPressed()` / `UsePowerRightPressed()`（Q / E）、`CyclePower(bool bLeft)`（BlueprintCallable）と `CyclePowerLeft()` / `CyclePowerRight()`（1 / 2）。プレイヤーの入力が直に結ぶ（02 記録）。`ConfirmTeleport()`（左クリック）・`AdjustTeleportDistance(AxisValue)`（ホイール）は BlueprintCallable で、照準が出ているときだけそれへ渡す（プレイヤーが呼ぶ）。
- `UsePower(bool bLeft)`・`ResetPowers()`（BlueprintCallable）。
- 読み出し（BlueprintPure）: `GetTeleportAim()`（出ている照準。無ければ null）、`GetSocketPower(bLeft)`（枠が解放済みの範囲の外なら `None`）、`GetGaugePercent(Power)`、`IsPowerAvailable(Power)`、`IsUsingPower(Power)`（本家の `Is Player Using Power ?`。`Active Powers` に入っているか）、`HasPowers()`、`GetUpgradeLevel(Power)`。C++ だけの `GetTuning(Power)`。
- `OnPowerUsed(EWasamiPower)`（BlueprintAssignable）… 本家の `UsedPower`（と、パワーごとの `UsedTelepathy` などをまとめたもの）。
- 設定（EditAnywhere）: `UnlockedPowers`（既定は 6 種すべてを並び順に）、`UpgradeLevel`（既定 5。0〜5）。
- 素材（ソフト参照。`BeginPlay` で読む。00 記録の決まり）: `RefillSound` `/Game/DD/Audio/UI/power_refilled`、`CycleSound` `/Game/DD/Audio/UI/UI_Select_V3`、`BoostSound` `/Game/DD/Audio/UI/Shard_Streak_Milestone_V5`、`BoostShakeClass` `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`（`_C`）、`BoostCameraAnim` `/Game/DD/Animation/Camera/CameraAnim_SpeedBoost`（`UWasamiCameraAnim`）、`TeleportAimSound` `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Mode_Entered`。`BoostWidgetClass`（既定 `UWasamiSpeedBoostWidget`）、`TeleportAimClass`（既定 `AWasamiTeleportAim`）。`BeginPlay` でブーストのウィジェットの素材（`UWasamiSpeedBoostWidget::LoadAssets`）と照準の素材（`AWasamiTeleportAim::LoadAssets`）も読んで持っておく（本家はプレイヤーがそれらのクラスを参照しているので、素材は最初から読まれている。最初の使用で読み込み待ちを出さないため）。

### `AWasamiTeleportAim : AActor`（`WasamiTeleportAim.h`）
本家の `BP_Power_Teleport`（旧版）。パワーが出し、移動か取り消しで消える。
- `AdjustDistance(AxisValue)`・`Confirm()`（BlueprintCallable。ホイールと左クリック）。
- `OnUsed`（BlueprintAssignable、引数なし）… 本家の `Used`。プレイヤーを動かした直後、自分を消す直前に出す。
- `MaxDistance`（既定 1000。`ExposeOnSpawn`。パワーが出すときに強化段階の値を入れる）、読み出し用の `Distance`（既定 1000）・`Alpha`（既定 0.6）・`Location`（移動先）。
- static: `DistanceFor(Alpha, MaxDistance)` = `Lerp(250, MaxDistance, Alpha)`、`StepAlpha(Alpha, AxisValue)` = `Clamp(AxisValue / 10 + Alpha, 0, 1)`、`LoadAssets(Out)`。
- 素材（ソフト参照。`BeginPlay` で読む）: `AimingLoopSound` `/Game/DD/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227`、`CommittedSound` `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Committed`、`CommittedShakeClass` `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`（`_C`）。
- コンポーネント（本家と同じ木）: `DefaultSceneRoot` → `SpringArm`（`TargetArmLength` 0 だけ変える。ほかは UE の既定＝位置ラグの速さ 10・サブステップあり）→ `Decal`（`DecalSize` (3, 100, 100)、相対回転 (P −90, Y 0, R 5.46e-5)、拡縮 (3.3264, 1, 1)。ソケット名なしでアームに付くので、アームの先〈ラグで遅れる位置〉に付いていく）、`DefaultSceneRoot` → `Audio`（音量 0.65、減衰なし）。本家の `Arrow`（エディタの表示用）と、デカールの子の `ParticleSystem`（ステップ 5）は置いていない。`GetSpringArm()`・`GetDecal()`。

### `UWasamiCameraAnim : UDataAsset`（`WasamiCameraAnim.h`）
本家の `CameraAnim`（UE4 の `UCameraAnim`。UE 5 には無い）を取り込みが写したもの（01 記録の `dd_assets.camera_anim`）。`AnimLength`（既定 3）・`BaseFOV`（既定 90）・`BasePostProcessSettings`（上書きフラグごと）・`BasePostProcessBlendWeight`（既定 0 = PP が効かない。UE4 と同じ）・`FloatTracks` / `ColorTracks`（`FWasamiCameraAnimFloatTrack` / `FWasamiCameraAnimColorTrack` = `PropertyName`〈`CameraComponent.PostProcessSettings.SceneColorTint` のような原作の名前〉と Matinee の曲線 `FInterpCurveFloat` / `FInterpCurveLinearColor`）。
- `ApplyPostProcessTracks(Time, Settings)` … `CameraComponent.PostProcessSettings.` で始まるトラックの値を、`FPostProcessSettings` の同名のメンバー（float か `FLinearColor`）へ書く。上書きフラグは触らない（UE4 でもトラックは値だけを動かし、フラグは基準の設定のまま）。評価は `FInterpCurve::Eval`（保存された接線のまま。UE4 の Matinee と同じ式）。

### `FWasamiCameraAnimPlayback`（`WasamiCameraAnim.h`）
再生中のアニメの時間の進み方（UE4 の `UCameraAnimInst` の写し。純粋な値の構造体でテストできる）。`Start(AnimLength, Rate, Scale, BlendIn, BlendOut, bLoop, Duration)`・`Advance(DeltaTime)`・`Stop(bImmediate)`、読み出しは `CurTime`・`Weight`・`bBlendingOut`・`bFinished`。

### `UWasamiCameraAnimModifier : UCameraModifier`（`WasamiCameraAnim.h`）
- `Get(PlayerCameraManager)` … カメラマネージャのこのモディファイアを返す（無ければ足す）。
- `Play(Anim, Rate, Scale, BlendInTime, BlendOutTime, bLoop, Duration)` → ハンドル（本家の `PlayCameraAnim`。`bRandomStartTime` は常に false、再生空間は CameraLocal 相当で、移動も回転もしない）、`Stop(Handle, bImmediate)`、`IsPlaying(Handle)`。

### `UWasamiChameleonComponent : UActorComponent`（`AWasamiPlayerCharacter` の `FX`）
本家のプレイヤーの子アクタ `FX`（`/Game/ThirdParty/Chameleon/Chameleon`。ポストプロセスの効果集）。
- `bCameraShake`・`CameraShakePower`（既定 0.01）・`CameraShakeFrequency`（既定 10）… 本家の `Camera Shake`・`Camera Shake Power`・`Camera Shake Frequency`（既定は Chameleon の CDO）。
- 素材: `CameraShakeMaterial` `/Game/Pipeline/Materials/M_DD_ChameleonCameraShake`（ソフト参照）。

### `UWasamiSpeedBoostWidget : UUserWidget`
本家の `UMG_SpeedBoost`。`LinesMaterial`（`/Game/DD/UI/Main/Powers/M_Speedlines`）・`VignetteTexture`（`/Game/DD/UI/Menu/Streaks/T_VignetteNew`）はソフト参照で `RebuildWidget` で読む。`LoadAssets(Out)`（static）はその 2 つを読んで `Out` に足す。

### `IWasamiEnemyInterface`（`UWasamiEnemyInterface`）
本家の `DD_EnemyInterface` のうちパワーに関わる 4 つ。どれも `BlueprintNativeEvent`（C++ の敵は `_Implementation` を上書きし、呼ぶ側は `IWasamiEnemyInterface::Execute_*`）。既定の中身は本家の `BP_DD_Character_Base` と同じ。
- `SetState(EWasamiEnemyState State, bool bByOrb)` … Primal Fear は `(Stun, false)`、パワーオーブは `(Stun, true)`。既定は何もしない。
- `GetState()` … 既定は `Patrol`。
- `PlayerVanish()` … Vanish を使った瞬間に全敵へ 1 回。既定は何もしない。
- `NoTelepathy()` … true ならテレパシーに映らない。既定は false。
- `EWasamiEnemyState : uint8` … `Patrol`・`Pursue`・`Stun`・`Teleport`（本家の `Enum_EnemyStates`。表示名の表で値 2 が欠けているが、値 2 が Stun として使われている）。

### `IWasamiTelekinesisInterface`（`UWasamiTelekinesisInterface`）
本家の `DD_TelekinesisInterface`。`Activate()`（`BlueprintNativeEvent`、既定は何もしない）。本家では `BP_Shard` が実装し、テレキネシスが届くとプレイヤーへ飛んで回収される。

## 内部構造と処理の流れ

### 枠と解放済みのパワー（`BeginPlay`）
- `Powers`（`FWasamiPowerSlot` の配列）を `UnlockedPowers` から、すべて使える状態で作る（本家の `Check` が `Power_Default` から未解放を外すのと同じ結果）。
- 左右の枠は `Powers` の添字 `LeftIndex` / `RightIndex` を持ち、**既定は左右とも 0**（本家の GameInstance の `Selected Power Left/Right` の既定。左右とも Speed Boost）。左右が同じパワーを指してもよい。
- タブレットの画面へは、プレイヤーが毎フレーム `GetSocketPower` と `GetGaugePercent` を渡す（02・03 記録）。

### Q / E（`UsePowerKey`）
`bCanInteract`（本家の `Can Interact?`）が偽なら何もしない。キーごとの DoOnce（`bLeftKeyClosed` / `bRightKeyClosed`）を通ったら `UsePower` を呼び、0.5 秒後に戻す。

### `UsePower(bLeft)`
1. プレイヤーの `bCanUseTablet` と `bHasInput` が両方真でなければ終わる（タブレットを構えている必要は無い）。
2. **枠を弾ませる**（`UWasamiTabletWidget::BounceSocket`）。使えるかの判定より前なので、使えないときも弾む。
3. 枠のパワーを取る（添字が範囲外なら構造体の既定 = 使えない）。`bAvailable` が偽なら終わる。**音も何も出さない**（最新版どおり。旧版は `power_not_ready` を鳴らしていた。ユーザーの回答）。
   - **例外（旧版から採る。ユーザーの回答）**: 解放済みが 1 つ以上あり、テレポートを使用中（`Active Powers` にある）で、押した側がテレポートを使った側（`bTeleportLeft`。本家の `CurrentSide`）なら、テレポートを取り消す（`ResetTeleport`）。枠のパワーが何かは見ない（使った側の枠は照準中に切り替えられないので、ふつうはテレポートのまま）。キーごとの 0.5 秒の連打防止を通った後なので、照準を出してから 0.5 秒は取り消せない。
4. `Use Power` 側の DoOnce（`bUseClosed`）を通ったら、`OnPowerUsed` を出し、パワーごとの処理へ（いまは `SpeedBoost` と `Teleport`。ほかは何もしない）。
5. 最後に 0.5 秒の Delay で DoOnce を戻す。**連打防止は Q / E ごとと、発動全体の 2 段**。

### 1 / 2（`CyclePower`）
**タブレットを構えているとき**（`IsTabletUp()`）で、その側の `bCanCycleLeft` / `bCanCycleRight`（テレポートを使った側だけ、照準を出してから移動か取り消しまで偽になる）が真のときだけ効く。解放済みが 1 つ以上なら `UI_Select_V3` を音量 1.5・ピッチ 2.0 で鳴らし、添字が末尾なら 0、そうでなければ `Clamp(添字 + 1, 0, 5)`。使えるかどうかにはよらない。

### Delay（`Delay`）
本家の Blueprint の `Delay` ノードは、数えている間にもう一度呼ばれても無視される。`FTimerManager` で「そのハンドルのタイマーが動いていなければ仕掛ける」として写す。原作の癖（死亡のリセットの後に古い Delay が残って早く切れるなど）もそのまま出る。

### ゲージ（`FWasamiPowerGauge`、`TickComponent`）
- 本家の `BP_Powers` のタイムライン（長さ 1 秒、0→1 の直線）。`SetDelay(D)` は再生速度を 1/D にし、FlipFlop で「終わりから逆再生（アイコンが 1→0）」と「頭から再生（0→1）」を交互に行う（FlipFlop の最初は逆再生）。**テレポートだけは FlipFlop を使わず、アイコンの値が 1 未満なら頭から、そうでなければ終わりから**。位置を動かすだけでは `Percent` は変わらず、次のティックから書く。端に着いたら止まる。
- `Stop()`（本家の `Stop <power> Timeline`）は止めて `Percent` を 1 にする。
- コンポーネントのティックで 6 本とも進める。
- 結果として、効果時間のあるパワー（Speed Boost・Telepathy・Vanish）は効果中に 1→0・再使用中に 0→1、ほか（Teleport・Primal・Telekinesis）は使った瞬間に 0.05 秒で 0・再使用中に 0→1 になる。

### 充填（`Refill`・`SetPowerAvailable`）
`power_refilled` を `PlaySound2D` の音量 0.5 で鳴らし、そのパワーの `bAvailable` を真にする。`SetPowerAvailable` は本家の `Array_Find({P, 逆の値})` → `Array_Set` と同じく、値が逆になっている最初の要素だけを書き換える。

### スピードブースト（`UseSpeedBoost` → `EndSpeedBoost` → `RefillSpeedBoost`）
- 使った瞬間: `Active Powers` に足す → `Shard_Streak_Milestone_V5` をプレイヤーの位置で鳴らす（`PlaySoundAtLocation`、音量・ピッチ 1.0、減衰なし。SoundWave 自体が音量 0.7・ピッチ 2.0 と同時発音 `NewSoundConcurrency` を持つ）→ 使えない状態にする → `BP_CameraShake_Streak` を倍率 1.0・`CameraLocal` で再生 → **歩きもダッシュも**ブーストの速さにする（`SetMoveSpeeds(速さ, 速さ)`）→ ゲージの `SetDelay(効果時間)` → `Delay(効果時間)` で終わりへ → 終わり用と充填用の DoOnce を開く。
- 終わり（DoOnce_3。開いているときだけ）: 閉じる → 速さを **300 / 600 に戻す**（本家は元の値ではなく定数を書く）→ `Active Powers` から外す → ゲージの `SetDelay(再使用)` → `Delay(再使用)` で充填へ。
- 充填（DoOnce_4。開いているときだけ）: 閉じて `Refill`。
- どちらの DoOnce も最初は閉じている（本家の Start Closed。@1211・@1241）ので、使う前にリセットしても何も起きない。
- Lv5 の値: 速さ 950 cm/s・効果 9.75 秒・再使用 7.5 秒（効果の後。合わせて 17.25 秒）。
- 本家の `UI Cooldown(True, 10)` の放送は作っていない（受ける `UMG_Tablet` の結び付け先のウィジェットが木に無く、見た目の効果が無いと見られる。調査 01 §4.4）。
- **演出**（本家の使った瞬間の Sequence @15020〜@23332 と @11203）: 使った瞬間、速さを入れた後に、ゲージ → `CameraAnim_SpeedBoost` を `Play(Rate 1, Scale 1, BlendIn 0.5, BlendOut 0.5, loop なし, Duration = 効果時間)`（ハンドルを持つ）→ 終わりの Delay → DoOnce を開く → `Sprinting Effects` のタイマー（0.001 秒の繰り返し。値を速さから書くだけなので `bMaxOncePerFrame` で 1 フレーム 1 回にしている）と FX の `bCameraShake = true` → `UMG_SpeedBoost` を作ってビューポートに載せる（Z 順 1）。
- `Sprinting Effects`（`UpdateSprintingEffects`）: 速さ（速度ベクトルの長さ。本家の `Speed` = `VSize(Velocity)`）を 0〜870 cm/s → FX の `CameraShakeFrequency` 0〜15、`CameraShakePower` 0〜0.003 に写す（`MapRangeClamped`）。本家はここで `Radial Blur Width`（0〜1）も書くが、FX の `Radial Blur` は一度も有効にならないので作っていない（下の「FX」）。
- 終わり: 速さ・`Active Powers` の後に、タイマーを止め、FX の `bCameraShake = false`、ウィジェットを外す（`RemoveFromParent`）。**カメラアニメは止めない**（効果時間と同じ長さで自然に終わる）。
- 死亡のリセット（本家の `Reset Speed Boost 1` = @37067）は、カメラアニメを `Stop(immediate)` で即座に止めてから終わりの処理へ進む。

### テレポーテーション（`UseTeleport` → 照準 → `UsedTeleport` → `RefillTeleport`、取り消しは `ResetTeleport`）
**旧版（`pak_reference`）に従う**（ユーザーの指示）。共通の仕組み（Q / E の連打防止、使えないときの無音など）は最新版のまま。
- 使った瞬間（`UseTeleport`。本家の @10017〜@11623）: `bTeleportLeft` を覚える → `Active Powers` に足す → `Teleport_Mode_Entered` を `PlaySound2D` の音量 1.75 → 使えない状態 → ゲージの `SetDelay(0.05)`（アイコンが 0.05 秒で 0 になる）→ 照準をプレイヤーの位置の **50 m 下**に回転 0・`AlwaysSpawn` で遅延スポーンし、`MaxDistance` に強化段階の値（Lv5 で 1500）を入れてから `FinishSpawning`、`OnUsed` に `UsedTeleport` を結ぶ → 使った側の `bCanCycle*` を偽 → クールダウンの Gate（`bTeleportGateOpen`）と充填の DoOnce（`bTeleportRefillOpen`。本家の DoOnce_5、最初は閉じている）を開く。
- 照準（`AWasamiTeleportAim`）:
  - `BeginPlay`: 照準ループの音をかけて再生（アクタが消えると止まる）。`Distance = DistanceFor(Alpha, MaxDistance)`（本家はホイールの軸の束縛が毎フレーム `Distance` を書くので、最初のフレームからこの値。Lv5 では本家のクラス既定の 1000 と同じ）。0.5 秒後にアームの位置ラグを有効にする。
  - 毎ティック: プレイヤー（`GetPlayerCharacter(0)`）のアクタ位置（カプセルの中心）+ アクタの前方（ヨーだけ）× `Distance` から真下へ 500 cm、`ECC_GameTraceChannel1`（`Teleport`。00 記録）の**オブジェクトのトレース**を複雑コリジョンで行い（自分は除く）、当たったらアームを `SetWorldLocation(当たった点, スイープなし, TeleportPhysics)` で動かす。当たらなければアームはそのまま。オブジェクトのトレースは応答を見ず、「オブジェクトの種類が Teleport で、クエリが有効」なものに当たる（病院のゾーン。01 記録）。
  - ラグ: 最初の 0.5 秒はアームの先が当たった点に即座に付き、以後は UE の SpringArm の `VInterpTo`（速さ 10、1/60 秒ずつのサブステップ）で遅れて付いていく。**0.5 秒より後に初めて床を捉えたときは、スポーン位置（50 m 下）から追ってくる**（本家どおり。テレポートのフラグではラグは戻らない）。
  - ホイール（`AdjustDistance`）: `Alpha = StepAlpha(Alpha, 値)`、`Distance = DistanceFor(Alpha, MaxDistance)`。照準のたびに 0.6 から。1 目盛りは Lv5 で 125 cm、範囲は 250〜1500 cm。
  - 左クリック（`Confirm`）: `Location = デカールのワールド位置 + (0, 0, 125)` を**先に**書き、DoOnce（`bConfirmed`）を通ったら、プレイヤーのカプセルの `WorldDynamic` と `Pawn` の応答を Ignore にし、0.12 秒後に移動（`Commit`）。移動の前の 2 回目のクリックは移動先だけを変える。本家はクリックの瞬間にカメラアニメ `CameraAnim_Teleport` を再生するが、それはステップ 4b で足す。
  - 移動（`Commit`）: `BP_CameraShake_Streak` を倍率 1・`CameraLocal` → `Teleport_Committed` を `PlaySound2D`（音量 1）→ `SetActorLocation(Location, スイープあり, TeleportPhysics)`（壁などで止まる。カプセルの中心を床 + 125 cm に置くので、立ち姿の 88 cm まで約 37 cm 落ちる）→ カプセルの `Pawn` と `WorldDynamic` を Block に戻す → `OnUsed` → 自分を消す。
  - `EndPlay` で 2 つのタイマー（ラグの有効化・移動）を止める（UE 5.8 のアクタは消えるときに自分のタイマーを消さない。本家の Delay はアクタと一緒に消える）。
- 移動の後（`UsedTeleport`。本家の @30854）: 使った側の `bCanCycle*` を真 → `Active Powers` から外す → ゲージの `SetDelay(5)`（アイコンが 0 → 1 を 5 秒）→ Gate が開いていれば `Delay(5)` の後に `RefillTeleport`。再使用は段階によらず 5 秒（本家の `00_Ballroom` だけの 1 秒は病院に無い）。
- 充填（`RefillTeleport`）: 充填の DoOnce が開いていれば閉じて `Refill`（`power_refilled` 0.5・使える状態）。
- 取り消し・死亡のリセット（`ResetTeleport`。本家の `Reset Teleport` @31827 と `BP_Powers` の `Stop Teleport Timeline`）: Gate を閉じる → `RefillTeleport`（使った後なら即座に充填・音）→ 照準があれば消す → `UsedTeleport`（Gate が閉じているので `Delay` は始まらない）→ ゲージを止めて 1。結果、すぐ使える・アイコンは 1・`power_refilled` が 1 回。
  - **クリックから移動までの 0.12 秒の間に取り消すと、移動は起きず、カプセルは `Pawn` と `WorldDynamic` を無視したまま残る**（本家どおり。次のテレポートの移動で戻る）。

### カメラアニメの再生（`UWasamiCameraAnimModifier::ModifyCamera`）
- 毎フレーム、再生中の各アニメを `Advance` し、終わっていなければ `BasePostProcessSettings` の写しにトラックの値を書き、重み `BasePostProcessBlendWeight × Weight` で `AddCachedPPBlend(…, VTBlendOrder_Base)` する（UE4 はカメラアニメの PP を通常のカメラの PP の下に重ねた。UE 5.8 の後継も `r.CameraAnimation.LegacyPostProcessBlending`〈既定 true〉で同じ位置に置く）。終わったものは外す。PP の値の重ね合わせ（`SceneColorTint` は重みで線形補間）はエンジンが行う。
- `Advance`（UE 5.8 の後継 `CameraAnimationCameraModifier.cpp` の `TickAnimation` と同じ形。UE4 の `CameraAnimInst.cpp` は手元に無い）: 時間を `Delta × Rate` 進め、ブレンドの経過も進める。ループしないアニメは「長さ − BlendOut × Rate」を過ぎたらブレンドアウトを始め、長さを過ぎたら終わる。ブレンドインは経過が BlendIn を超えたら終わる。ブレンドアウトの経過が BlendOut を超えたら終わる。重みは `min(ブレンドインの経過 / BlendIn, 1 − ブレンドアウトの経過 / BlendOut) × Scale`（どちらも直線）。`Duration` が正なら、`Duration − BlendOut` を数え終えたところで `Stop(false)`（ブレンドアウト）を呼ぶ。**`Duration` はブレンドアウトを含む長さ**（UE 5.8 の `FCameraAnimationParams::DurationOverride` の説明「including blends」。後継はこの値を使っていない）。
- `Stop(bImmediate)`: 即座なら（または BlendOut が 0 なら）終わり・重み 0、そうでなければブレンドアウトを始める（既に始まっていれば続ける）。
- スピードブーストでは: 0〜0.5 秒で色調が 0 → 1、9.25 秒まで 1、9.25〜9.75 秒で 1 → 0。`CameraAnim_SpeedBoost` の色調のトラックは開始前（−0.0018 秒）の 1 キーだけなので、値は常に (2.0, 0.584, 0.498)。
- FOV のトラック（`CameraComponent.FieldOfView`）は、いまは再生しない（`Play` が警告を出す）。`CameraAnim_Teleport` を入れるステップ 4 で、基準の FOV の扱い（`BaseFOV` 137.24 か t=0 のキーか）を決めて足す。Move トラックは取り込まない。

### FX（`UWasamiChameleonComponent`）
- 本家の Chameleon は、範囲なし（`Unbound`）の `PostProcessComponent`（`InternalPP`）を持ち、毎ティック `InitChameleon` で「`Native Post Process`（上書きなし）で設定を上書き → 有効な効果ごとに MID のパラメータを書いて `AddOrUpdateBlendable(MID, 1)`」を行う。本作は `BeginPlay` で持ち主に `UPostProcessComponent`（`bEnabled`・`bUnbound`）を作って付け（`UPostProcessComponent` は MinimalAPI で他のモジュールから派生できない）、揺れのマテリアルの MID を作る。毎ティック、ボリュームのブレンダブルを空にし、`bCameraShake` なら MID に `ShakePower` / `ShakeFQ` を書いて重み 1 で足す。
- 揺れの詳細設定（本家の `Camera Shake - Advanced`）は CDO の既定のまま（ブレンド `0 - Normal`・`BlendingOpacity` 1・カスタム深度とステンシルなし・距離のブレンドなし・白のマスク）なので、効果をそのまま画面に出す。
- **`Radial Blur` は作らない**。CDO の `Radial Blur`（有効フラグ）は false で、プレイヤーのテンプレートは `Custom Depth Highlighter` 系しか上書きせず、どのコードも true にしない（`Chameleon_C.Radial Blur` への書き込みはプレイヤーの `Radial Blur Width` だけ）。`InitChameleon` は無効な効果の関数を素通りするので、本家でも放射ブラーは出ていない。
- プレイヤーのテンプレートは `Custom Depth Highlighter (Clip)` を有効にしている（縁取り (1,0,0)、中 (0.0802,0,0)）。病院の `BP_06_ReaperNurse` などの敵が `SetRenderCustomDepth` を呼んで赤く縁取られる仕組みなので、敵を作るとき（M4）にこの FX へ足す。いまは作っていない。
- ボリュームは空でも常にある（上書きなし・ブレンダブルなし）。揺れのマテリアルが毎フレーム描かれるのはブースト中だけ（性能のルール）。

### `UMG_SpeedBoost`（`UWasamiSpeedBoostWidget`）
- 木: キャンバス `CanvasPanel_0` に、`Lines`（`M_Speedlines` のブラシ）、`Image_72`（`T_VignetteNew`）の順。どちらも全面に広げ（アンカー 0〜1、余白 0）、色 (1,0,0,1)、描画の拡縮 1.25 / 1.2（中心基準）。
- 不透明度: 本家の Tick は `Delay(0.001)` の後に `Lines.SetOpacity(MapRangeClamped(Speed, 0, 900, 0, 0.15))`・`Image_72.SetOpacity(…, 0, 0.5)` を行う。Delay はティックで仕掛けて次のティックの前に切れるので、**値は 1 フレーム遅れで、載った最初のフレームは両方とも不透明度 1 のまま描かれる**。本作は `NativeTick` で「前のティックが仕掛けた Delay があれば値を書く、それから Delay を仕掛けたことにする」として同じ順にしている。速さは持ち主のポーンの速度の長さ。
- UI マテリアルにはウィジェットの色と不透明度が頂点カラーとして掛かる（UE 5.8 `SlateElementPixelShader.usf` の `GetColor`: 材質の色 × 頂点カラー）ので、`M_Speedlines` の白黒の線が赤く、薄く出る。
- 本家のウィジェットのアニメ `Fade` はどこからも再生されないので作っていない。

### 死亡のリセット（`ResetPowers`。本家の `BP_Powers.Reset All Powers`）
1. スピードブースト: 終わりの処理（効果中なら即座に終わり、再使用のゲージが始まる）→ 充填の処理（使った後なら即座に充填）→ ゲージを止めて 1。
2. テレポート: `ResetTeleport`（上の「テレポーテーション」）。
3. Primal Fear: ゲージを止めて 1 → 充填。
4. Vanish: ゲージを止めて 1 → 充填（ウィジェットを消す処理は Vanish の実装で足す）。
- **Primal と Vanish は、使っていなくても充填を通るので `power_refilled` が鳴る**（本家の `Reset Primal` = @37261: Push @6444 → @27788 の Gate の Close。`Reset Vanish` = @37737 も同じ形）。Telepathy と Telekinesis はリセットしない。
- 本家のクールダウン明けの Gate（Telepathy・Primal・Telekinesis・Vanish）は「Open の直後に Enter」（@6301〜@6444 ほか）なので、閉じても次の Enter の前に必ず開く＝素通しと同じ。Gate の状態は持たない。
- 呼ぶのは死亡画面（本家は `UMG_DeathScreen` の暗転の 2 秒後、生き返りの直前）。死亡はまだ無い（M5）。

## 強化段階の値（`FWasamiPowerTuning`。本家の `BP_DD_PlayerCharacter` の分岐）

| 段階 | ブースト速さ / 時間 / 再使用 | テレポート距離 | テレパシー時間 / 再使用 | Primal 半径 / 再使用 | テレキネシス半径 / 再使用 | Vanish 再使用 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 870 / 6.75 / 9.5 | 1000 | 5 / 8.5 | 1500 / 35 | 1750 / 10.5 | 30 |
| 1 | 870 / 6.75 / 9.5 | 1000 | 5 / 8.5 | 1500 / 35 | 2000 / 10.0 | 28 |
| 2 | 890 / 7.5 / 9.0 | 1125 | 6 / 8.0 | 2000 / 32 | 2250 / 9.5 | 24 |
| 3 | 910 / 8.25 / 8.5 | 1250 | 7 / 7.5 | 2500 / 29 | 2500 / 9.0 | 20 |
| 4 | 930 / 9.0 / 8.0 | 1375 | 8 / 7.0 | 3000 / 26 | 2750 / 8.5 | 18 |
| **5（本作）** | **950 / 9.75 / 7.5** | **1500** | **9 / 6.5** | **3500 / 23** | **3000 / 8.0** | **15** |

- テレポートの再使用は段階によらず 5 秒、Vanish の効果は 15 秒。
- 本作は祭壇が無いので全パワーを Lv5 に固定（ユーザーの回答）。本家は段階をパワーごとにセーブに持つが、本作は `UpgradeLevel` 1 つ。
- **スピードブーストの再使用は最新版のコードの値**（祭壇の表と旧版より 1 秒長い。ユーザーの回答）。

## 作るアセット
取り込みは `WasamiDDTools.import_dd_powers()`（`pipeline/dd_powers.py`、01 記録）。アイコンは `import_dd_tablet()`（03 記録）。

| パス | 中身 |
| --- | --- |
| `/Game/DD/Audio/UI/power_refilled` | 充填の音（1.515 秒、48 kHz） |
| `/Game/DD/Audio/UI/Shard_Streak_Milestone_V5` | ブーストの音（1.058 秒。SoundWave の Volume 0.7・Pitch 2.0・ConcurrencySet `NewSoundConcurrency`） |
| `/Game/DD/Audio/NewSoundConcurrency` | 同時発音（MaxCount 2・VolumeScale 0.5、ほかは既定） |
| `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Mode_Entered` | テレポートの照準の開始（旧版。1.956 秒、48 kHz、エンジンの音。SoundWave の値は既定のまま） |
| `/Game/DD/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227` | 照準のループ（旧版。3.733 秒、`bLooping`） |
| `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Committed` | テレポートの移動（旧版。1.543 秒） |
| `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak` | ブーストとテレポートの移動のシェイク（`LegacyCameraShake`。振動 0.5 秒・ブレンドイン 0・アウト 0.25、回転 Pitch 0.25/30・Yaw 0.25/40・Roll 0.5/35、FOV 2.0/10） |
| `/Game/DD/Animation/Camera/CameraAnim_SpeedBoost` | `WasamiCameraAnim`。長さ 24.503 秒、`BaseFOV` 137.24、`BasePostProcessBlendWeight` 1.0、基準の PP は `bOverride_WhiteTemp`・`WhiteTint`・`SceneColorTint` が真（WhiteTemp 6500・WhiteTint 0 は中立）で `SceneColorTint` (2.0, 0.583955, 0.498, 1)、色のトラック 1 本（−0.0017948 秒に同じ色の 1 キー、`CIM_CurveAutoClamped`） |
| `/Game/DD/UI/Main/Powers/T_Speedlines` | 集中線（3841 × 5404、2 列 × 5 段のコマ。sRGB・`TC_Default`・`TEXTUREGROUP_UI`。原作の cook も非圧縮 BGRA8・ミップ 1 で、本作の実測も約 81 MB〈`blueprint_get_memory_size` 84,934,656〉） |
| `/Game/DD/UI/Menu/Streaks/T_VignetteNew` | ビネット（1024²、白地にアルファで縁。sRGB・`TC_EditorIcon`・`TEXTUREGROUP_UI`。本作は 4 MB〈ミップなし。原作の cook は 11 段のミップあり。全面に引き伸ばすだけなので見た目は同じ〉） |
| `/Game/DD/UI/Main/Powers/M_Speedlines` | 原作のグラフどおり。User Interface・Translucent。`/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook` の呼び出し（入力はすべて既定 = 2 列 × 2 段、位相 `Time`、`TexCoord 0`）の出力 2 番（`UVs`）を `T_Speedlines` の `TextureSample` の UV に、その RGB を Emissive に。Opacity は未接続（1）。UE 5.8 の FlipBook は 4.24 の書き出しと同じ 38 ノードで、出力の並びは `SortPriority` だけで決まる（`MaterialExpressions.cpp` の `GetInputsAndOutputs`）ので、出力 2 番は原作と同じ `UVs`（取り込みのログで確認）。2 × 5 のシートを 2 × 2 で読むので、1 コマは 1 列 × 2.5 段ぶんが 1 秒に 4 コマで流れる（原作のまま） |
| `/Game/Pipeline/Materials/M_DD_ChameleonCameraShake` | **推定**。Chameleon の `M_CameraShake`（Post Process）。書き出しに残るのはパラメータ `ShakePower`（既定 0.01）・`ShakeFQ`（既定 50）と `MakeFloat2` 1 つ・`MF_SetBlending`・`MF_DepthOnlyMasking` だけで、HLSL・数式・シーンテクスチャは無い。本作は `ScreenPosition.ViewportUV + Append(sin(Time × ShakeFQ), cos(Time × ShakeFQ)) × ShakePower` で `PostProcessInput0` を読み、その色を Emissive に出す（UE の Sine / Cosine は周期 1 = `ShakeFQ` 回/秒の円）。ブレンドの位置は既定（トーンマップの後）。実機との見比べは進捗記録のステップ 11 |

## 原作データの根拠
- 仕組みの全体と各値: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`（`Use Power` @29314〜、使えないときの @16335、DoOnce_11 @11731、スピードブースト @13628〜@16280、終わり @30、充填 @1206、Reset 系 @37067〜@37753）、`UI/BP_Powers.txt`（`Set Delay`・FlipFlop・`Reset All Powers`）、`UI/Tablet/UMG_TabletPowers.txt`（`Check`・`Cycle Power Left/Right`・`Update Powers`）。まとめは `.claude/references/powers/01-player-system.md`。
- `Has Input`・`Can Interact?`・`Can Use Tablet?`・`Can Cycle Left?/Right?` の既定（すべて true）: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.json` の CDO。
- キー: `pak_reference_2/_raw/DDeception/Config/DefaultInput.ini`（`Use Power Left` Q、`Use Power Right` E、`Cycle Power Left` 1、`Cycle Power Right` 2、`Use Power` R は受ける BP が無い）。
- 敵とシャードのインターフェース: `pak_reference_2/_assets/DDeception/Content/Blueprints/Characters/Shared/DD_EnemyInterface.json`・`DD_TelekinesisInterface.json`、既定の中身は `BP_DD_Character_Base.txt`（調査 04 §4）。
- 素材: `pak_reference_2/_assets/DDeception/Content/Audio/UI/*.json`・`Audio/NewSoundConcurrency.json`・`UI/Menu/Streaks/BP_CameraShake_Streak.json`（旧版と同じ値。アイコン・音の ogg も両版で同一であることを突き合わせた）。
- ブーストの演出: `BP_DD_PlayerCharacter.txt`（`PlayCameraAnim` @15636 の引数、効果時間の選択 @15466〜@15535、`UMG_SpeedBoost` の生成 @11203・`AddToViewport(1)` @11256、`Sprinting Effects` のタイマー @23268 と中身 @35054、`Camera Shake = True` @23332、終わりの @358〜@436、`Reset Speed Boost 1` の `Stop(True)` @37067、`Speed = VSize` @31419）。`_camera/CameraAnim_SpeedBoost.json`・`_assets/DDeception/Content/Animation/Camera/CameraAnim_SpeedBoost.json`。
- `UMG_SpeedBoost`: `_bytecode/DDeception/Content/UI/Main/Powers/UMG_SpeedBoost.txt`（Tick → Delay 0.001 → 不透明度、Construct → プレイヤーを取る）、`_assets/…/UI/Main/Powers/UMG_SpeedBoost.json`（木・色・拡縮・ブラシ）、`M_Speedlines.json`（式とつなぎ）、`_textures.json`（`T_Speedlines`・`T_VignetteNew` の設定）、`_assets/Engine/Content/Functions/Engine_MaterialFunctions02/Texturing/FlipBook.json`（4.24 の FlipBook。38 ノード）。
- Chameleon: `_bytecode/DDeception/Content/ThirdParty/Chameleon/Chameleon.txt`（`InitChameleon`・`Radial Blur Func`・`Camera Shake Func`・`Set Advanced Effect Features`・`ApplyChameleonSettings`）、`_assets/…/ThirdParty/Chameleon/Chameleon.json`（CDO: `Enabled`・`Unbound`、揺れの既定、`Camera Shake - Advanced`、`Native Post Process`）、`Enums/BlendModes.json`（`NewEnumerator0` = `0 - Normal`）、`Materials/M_CameraShake.json`・`M_RadialBlurHLSL.json`、`BP_DD_PlayerCharacter.json` の `FX_GEN_VARIABLE`（テンプレート）。放射ブラーの有効フラグを書くコードが無いことは `_bytecode` 全体の検索で確かめた（`Chameleon_C.Radial Blur`・`Camera Shake` を書くのはプレイヤーとカットシーン `Cutscene_01_Exterior` だけ）。
- UE 5.8 の挙動: `Engine/Plugins/Cameras/EngineCameras/Source/EngineCameras/Private/Animations/CameraAnimationCameraModifier.cpp`（ブレンドの形・PP の重ね方）、`Engine/Source/Runtime/Engine/Classes/Camera/CameraModifier.h`・`Private/Camera/CameraModifier.cpp`（`ModifyCamera` → `AddCachedPPBlend`）、`Private/PlayerCameraManager.cpp`（`ApplyCameraModifiers` が先に PP の蓄えを空にする）、`Core/Public/Math/InterpCurve.h`（`Eval`）、`Shaders/Private/SlateElementPixelShader.usf`（UI マテリアルの色 × 頂点カラー）、`Classes/Components/PostProcessComponent.h`（MinimalAPI、`AddOrUpdateBlendable` は inline）。

- テレポーテーション（旧版）: `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_Power_Teleport.txt`（BeginPlay @1060、Tick @1525、ホイール @1279、クリック @2342 → @547、移動 @15）、`_assets/…/Powers/BP_Power_Teleport.json`（コンポーネントの値、入力の束縛）、`BP_DD_PlayerCharacter.txt`（使った瞬間 @10017〜@11623、使えないときの取り消し @14489〜@14826、`UsedTeleport` @30854、充填 @4045、`Reset Teleport` @31827）、`UI/BP_Powers.txt`（`Set Delay Teleport` @1746、`Stop Teleport Timeline` @2322）、音の書き出し（`_assets/…/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227.json` の `bLooping`、`_assets/Engine/Content/VREditor/Sounds/UI/*.json`）。まとめは `.claude/references/powers/02-teleport.md`。
- UE 5.8 の挙動（テレポート）: `Engine/Source/Runtime/Engine/Private/GameFramework/SpringArmComponent.cpp`（ラグ、`GetSocketTransform` がソケット名を見ないこと、長さ 0 ではトレースしないこと）、`Private/Components/DecalComponent.cpp`（材質が無いデカールは既定のデカール材で描く）、`Private/KismetSystemLibrary.cpp`・`KismetTraceUtils.cpp`（`LineTraceSingleForObjects` の中身）。

## 依存関係
- `AWasamiPlayerCharacter`（02 記録）: `bCanInteract`・`bCanUseTablet`・`bHasInput`・`IsTabletUp()`・`SetMoveSpeeds()`・`GetTabletScreen()`・コントローラのカメラマネージャ。プレイヤーがこのコンポーネントを作り、入力を結び、毎フレーム画面へ値を渡す。
- `UWasamiTabletWidget`（03 記録）: `BounceSocket`。
- `WasamiAssets.h`（00 記録）。
- エンジン: `FTimerManager`、`UGameplayStatics::PlaySound2D` / `PlaySoundAtLocation`、`APlayerCameraManager::StartCameraShake` / `AddNewCameraModifier` / `AddCachedPPBlend`、`UCameraModifier`、`UPostProcessComponent`、`UMG`（`UUserWidget`・`UWidgetTree`・`UCanvasPanel`・`UImage`）。
- 取り込み: `WasamiDDTools.import_dd_powers()`（01 記録の `dd_powers.py` と `dd_assets.camera_anim` / `texture` / `material`）。

## テスト（`Tests/WasamiPowerTests.cpp`）
`Automation RunTests Wasami`（6 件、2026-09-16 にすべて成功）。
- `Wasami.Powers.Gauge` … FlipFlop の交互の向き、途中の値（2 秒で 1 秒後 0.5 など）、端で止まる、`Stop` で 1、テレポートの向きの決まり方。
- `Wasami.Powers.Tuning` … Lv5 の値、段階の丸め、Lv0 のテレキネシス半径、Lv1 のブーストの再使用 9.5。
- `Wasami.Powers.SocketBounce` … 弾みのキーの値と、キーの間の値（0.1 秒で 1.19028）。
- `Wasami.Powers.TeleportDistance` … Lv5 の最初の距離 1000（強化なしなら 700）、`Alpha` 0 / 1 の端、1 目盛りで +0.1（Lv5 で +125 cm）、1 フレームに 2 目盛り、0 と 1 での切り詰め。
- `Wasami.CameraAnim.Playback`（`Tests/WasamiCameraAnimTests.cpp`）… ブーストの再生（0.25 秒で 0.5、0.5 秒で 1、9.25 秒でブレンドアウトが始まり 9.5 秒で 0.5、9.75 秒で 0、その次で終わり）、即座の停止、ブレンドの無い 0.5 秒のアニメが長さで終わること、ブレンドイン中の停止が小さい方の重みで続くこと。
- `Wasami.CameraAnim.Tracks` … ブーストの 1 キーの色が保たれ上書きフラグを触らないこと、`CameraAnim_Teleport`（旧版）のキーと接線で、書き出しの 60 fps の標本（`CameraAnim_Teleport.csv`）と同じ値になること（0.1 秒の露出 1.149884・色調 (1.528122, 0.532324, 0.471878)、0.05 秒、8/60 秒の露出の山 67.69149）。FOV のトラックは PP を変えない。

## 確かめたこと（2026-09-16、PIE、`L_Hospital_Zone1` の開始地点、ユーザーの了承のうえで `Tools/desktop.py` から入力）
- 始めは左右とも Speed Boost、6 種とも使える、ゲージはすべて 1。
- E: 歩き・ダッシュ・`MaxWalkSpeed` が 950 になり、`IsUsingPower(SpeedBoost)` が真、使えない状態、約 2 秒後のゲージが 0.796（1 − 2 / 9.75 = 0.795）。使ってから約 18 秒後（効果 9.75 秒と再使用 7.5 秒を過ぎた時点）に読むと、300 / 600・使える・ゲージ 1 に戻っていた。
- Q（左 = Speed Boost）で同じように効き、タブレットの左の枠のアイコンが扇形に灰色へ変わっていくのを撮った。
- タブレットを上げて 1 で左が Teleport に、2 で右が Teleport → Telepathy → … → Vanish → Speed Boost と巡回した。右の枠を 6 種すべてに切り替えて撮り、どのアイコンも出た。
- タブレットを下ろすと 1 / 2 は効かない。Q で Teleport（中身が未実装）を使っても使える状態のまま。
- ブースト中に `ResetPowers` を呼ぶと、即座に 300 / 600・使える・ゲージ 1 に戻った。
- PIE のログにこの仕組みの警告やエラーは無かった。

### スピードブーストの演出（2026-09-16、PIE、ユーザーの了承のうえで `Tools/desktop.py` から E を送り、前進はエディタの Python の毎フレームのコールバックで `AddMovementInput` を入れた）
- 取り込み: `import_dd_powers()` が `camera_anims 1・textures 2・materials 2` を作り、`FlipBook output 2 is UVs` をログに出した。読み戻した値は上の「作るアセット」のとおり。`Failed to compile` なし。`import_dd_tablet()` も前回と同じ数（テクスチャ 25・マテリアル 2・メッシュ 1・フォント 1・音 3・ミニマップ 8）で通った。
- 立ち止まって E: 最大速さ 950、`IsUsingPower` 真、FX の `bCameraShake` 真（速さ 0 なので強さ・周波数は 0）、画面に載ったウィジェット 1。開始地点の絵の平均が (22.7, 22.7, 30.2) → (36.2, 14.8, 18.1) と赤くなった。
- 走りながら（速さ 950）: FX の強さ 0.003・周波数 15、FOV 115。赤い集中線・赤いビネット・赤い色調が出た。
- 効果時間の後: 最大速さ 300、揺れ偽、ウィジェット 0、色調が消えた（廊下の絵の平均 (73.1, 28.6, 33.5) → (67.1, 84.1, 85.3)）。再使用の後に使える・ゲージ 1。
- ブースト中に `ResetPowers`: 1.4 秒後の絵の平均が色調の無い状態と同じ (67.1, 84.1, 85.4) で、カメラアニメが即座に止まった。揺れ偽・ウィジェット 0・使える。
- PIE の開始以降、ログに警告もエラーも無かった。エンジンの起動時の `LogAutomationTest: Error: Condition failed` 19 件は前回の起動にも同じ数あり、エンジン自身の自己テストのもの。

### テレポーテーション（2026-09-16、PIE、`L_Hospital_Zone1`、ユーザーの了承のうえで `Tools/desktop.py` から Space・1・2・Q・E・左クリック・ホイールを送り、値はエディタの Python で読んだ）
- 組み立て直したレベルのゾーン（床のメッシュと救急車の屋根の箱）は、オブジェクトの種類 `Teleport`・`QueryOnly`・全チャンネル Overlap・プロファイル `Custom`・ゲームで非表示・影なしで、開き直しても保たれていた（01 記録）。
- プレイヤースタート (15, 385)・南向きで、タブレットを上げて 1 → 左の枠が Teleport。Q で照準が出て、使えない状態・使用中・ゲージ 0、照準ループが鳴り、アームとデカールが 10 m 先の床（Z 0）に付いた（`Distance` 1000・`Alpha` 0.6・`MaxDistance` 1500）。画面には 2 m 四方のデカール（UE の既定の材質）が見えた。
- 照準中の 1 では左の枠が変わらず、取り消した後は変わった。
- ホイールを奥へ 1 目盛りで `Distance` 1125（アームも −740 へ）、手前へ 2 目盛りで 875（−490）。
- 左クリック: 毎フレームの記録で、クリックのフレームにカプセルの `Pawn`・`WorldDynamic` が Ignore になり、約 0.13 秒後（0.12 秒 + 1 フレーム）にプレイヤーが (15, −490, 125)（デカール + 125 cm）へ移り、同じフレームで Block に戻って照準が消えた。次のフレームで床の高さ（90.1）へ下りた（UE の歩行の床合わせ）。ゲージは 3 秒後に 0.593（3 / 5 = 0.6）、7 秒後には使える・ゲージ 1。
- 右の枠も Teleport にして左から照準を出し、E を押しても取り消されない（無音）。Q で取り消すと即座に使える・ゲージ 1・照準が消えた。
- 照準中に `ResetPowers` を呼ぶと、照準が消えて使える・ゲージ 1。クリックの直後（移動の前）に `ResetPowers` を呼ぶと、移動は起きずカプセルが Ignore のまま残り（本家どおり）、次のテレポートの移動で Block に戻った。
- PIE の間、この仕組みの警告やエラーは無かった（VSM の「非 Nanite マーキング ジョブ キュー オーバーフロー」2 件は前のセッションのログにも出ていたもの）。音はユーザーのスピーカーで確かめていない（照準ループが再生中であることは読んだ）。

## 既知の制約・注意点
- **スピードブーストとテレポート以外のパワーは中身が無い**（枠に出る・弾む・`OnPowerUsed` が出るだけで、使える状態は変わらない）。
- テレポートの**カメラアニメ（`CameraAnim_Teleport`）はまだ再生しない**（ステップ 4b。FOV の基準を本家の実機で決めてから）。**デカールの材質とパーティクルもまだ無い**（ステップ 5）。いまのデカールは UE の既定のデカール材で描かれる。
- 本家では照準のアクタがクリックを受け、入力を消費しないので、プレイヤー自身の左クリック（調べる）も同時に走る。本作のプレイヤーにはまだ調べる処理が無い（02 記録）。
- ゲームパッドでの確定（最新版の `Gamepad_FaceButton_Bottom`）は旧版に無いので入れていない。
- 本家は `Check` を `Delay 0.2` の後に行うが、本作は `BeginPlay` ですぐ作る。
- 本家の `Power` 配列は解放フラグ（セーブ）から作るが、本作はすべて解放済み（`UnlockedPowers`）。選んだ枠をレベルをまたいで残す本家の仕組み（GameInstance）は、ステージが病院だけなので作っていない。
- 本家のパワーの放送（`UsedTeleportPower`・`UsedPrimal` など）を購読するのは本家のチュートリアルや台本のレベルだけで、病院には無い。本作は `OnPowerUsed` 1 つにまとめた。
- ゲームパッドの割り当て（LT / RT / LB / RB）はまだ入れていない（プレイヤーの入力がキーボードとマウスだけのため）。
- 音と揺れはユーザーのスピーカーと画面で確かめていない（PIE の確認は値と絵）。
- **推定のもの**: `M_DD_ChameleonCameraShake` の揺れ方（円・sin/cos）。本家の実機で見比べる（進捗記録のステップ 11）。
- `Duration` の扱い（ブレンドアウトを含む）は UE 5.8 の説明に拠る。UE4 の `CameraAnimInst.cpp` で確かめていない。違っていれば色調の消え方が 0.5 秒ずれるだけ。
- `UMG_SpeedBoost` の最初のフレームが不透明度 1 で出る（本家の Delay の順の写し）。1 フレームなので撮影では確かめていない。
- `T_Speedlines` は原作どおり非圧縮で約 81 MB あり、プレイヤーの `BeginPlay` から持ち続ける（本家もプレイヤーがクラスを参照しているので同じ）。
- カメラアニメの FOV のトラックはまだ再生しない（ステップ 4）。
- FX の `Custom Depth Highlighter (Clip)`（敵の縁取り）はまだ無い（M4）。

## 変更履歴
- 2026-09-16: テレポーテーションの仕組みを足した（旧版。`AWasamiTeleportAim`、使った瞬間・照準・ホイール・クリック・0.12 秒後のスイープ移動・再使用 5 秒・同じ側の Q / E での取り消し・死亡のリセット、音 3 つ、テスト `Wasami.Powers.TeleportDistance`）。カメラアニメと見た目はまだ
- 2026-09-16: スピードブーストの演出を足した（`CameraAnim_SpeedBoost` の赤い色調、`UMG_SpeedBoost` の集中線とビネット、FX の画面の揺れ。放射ブラーは本家で無効なので作らない）。UE4 の CameraAnim の再生（`UWasamiCameraAnim`・`FWasamiCameraAnimPlayback`・`UWasamiCameraAnimModifier`）、FX（`UWasamiChameleonComponent`）、`UWasamiSpeedBoostWidget`、テスト `Wasami.CameraAnim` 2 件を足した
- 2026-09-16: 初版（パワーの土台: 枠・Q/E/1/2・2 段の連打防止・ゲージ・強化段階の表・死亡のリセット・スピードブースト、敵とシャードのインターフェース、テスト）
