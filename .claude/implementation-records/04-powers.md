---
title: タブレットのパワー（枠・入力・ゲージ・強化段階・スピードブーストとその演出・テレポーテーション・Telepathy・Primal Fear とテレキネシスと Vanish と一瞬の演出の基底・カメラアニメ・FX）
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
  - Source/wasami_deception/WasamiPowerBurst.h
  - Source/wasami_deception/WasamiPowerBurst.cpp
  - Source/wasami_deception/WasamiPrimalPower.h
  - Source/wasami_deception/WasamiPrimalPower.cpp
  - Source/wasami_deception/WasamiTelekinesisPower.h
  - Source/wasami_deception/WasamiTelekinesisPower.cpp
  - Source/wasami_deception/WasamiVanishPower.h
  - Source/wasami_deception/WasamiVanishPower.cpp
  - Source/wasami_deception/WasamiVanishWidget.h
  - Source/wasami_deception/WasamiVanishWidget.cpp
  - Source/wasami_deception/WasamiTelepathyPower.h
  - Source/wasami_deception/WasamiTelepathyPower.cpp
  - Source/wasami_deception/WasamiTelepathyTracker.h
  - Source/wasami_deception/WasamiTelepathyTracker.cpp
  - Source/wasami_deception/WasamiTelepathyTrackerWidget.h
  - Source/wasami_deception/WasamiTelepathyTrackerWidget.cpp
  - Source/wasami_deception/Tests/WasamiTestEnemy.h
  - Source/wasami_deception/Tests/WasamiTestEnemy.cpp
  - Source/wasami_deception/Tests/WasamiPowerTests.cpp
  - Source/wasami_deception/Tests/WasamiCameraAnimTests.cpp
updated: 2026-09-19
---

# タブレットのパワー

## 役割
本家 Dark Deception のタブレットのパワー 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）の仕組みと演出。本家がプレイヤー（`BP_DD_PlayerCharacter`）・ゲージのアクタ（`BP_Powers`）・タブレットの枠（`UMG_TabletPowers`）に分けて持つものを、プレイヤーに付ける `UWasamiPowerComponent` 1 つにまとめる。**6 種とも中身まである**（2026-09-17）: スピードブースト（演出を含む）、テレポーテーション（照準とその見た目・移動・取り消し・再使用・カメラアニメ）、Telepathy（`AWasamiTelepathyPower` と、敵ごとの画面空間の印 `AWasamiTelepathyTracker`・`UWasamiTelepathyTrackerWidget`）、Primal Fear（`AWasamiPrimalPower`）、テレキネシス（`AWasamiTelekinesisPower` と力場の粒子 `P_WasamiForceField`）、Vanish（`AWasamiVanishPower` と `UWasamiVanishWidget`）。推定した材質は最新版の実機の収録と見比べて値を決めた（下の「作るアセット」と「既知の制約・注意点」。収録と測った値は `observations/README.md`）。Primal・Telekinesis・Vanish が共有する一瞬の演出（全画面のポストプロセス 2 つと 2 秒のタイムライン）は基底 `AWasamiPowerBurst` にまとめた。パワーのテストは仮の的 `AWasamiTestEnemy` を使う（敵ワサミ `AWasamiEnemy` に Primal Fear・Vanish・Telepathy が届くことは 07 記録のテスト `Wasami.Enemy.Actor.Powers`）。パワーの演出に使う共通の部品として、UE 5 に無い UE4 の `CameraAnim` の再生（`UWasamiCameraAnim` / `UWasamiCameraAnimModifier`）と、プレイヤーの FX（本家の Chameleon、`UWasamiChameleonComponent`）もここに書く。原作の調査は `.claude/references/powers/`。**テレポーテーションだけ `pak_reference`（旧版）、それ以外は `pak_reference_2`（最新版）に従う**（ユーザーの指示）。

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
- 素材（ソフト参照。`BeginPlay` で読む。00 記録の決まり）: `RefillSound` `/Game/DD/Audio/UI/power_refilled`、`CycleSound` `/Game/DD/Audio/UI/UI_Select_V3`、`BoostSound` `/Game/DD/Audio/UI/Shard_Streak_Milestone_V5`、`BoostShakeClass` `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`（`_C`）、`BoostCameraAnim` `/Game/DD/Animation/Camera/CameraAnim_SpeedBoost`（`UWasamiCameraAnim`）、`TeleportAimSound` `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Mode_Entered`、`TelepathySound` `/Game/DD/Audio/SharedGameplay/Telepathy`、`TelepathyEndSound`（`TeleportAimSound` と同じ音）、`TelepathyShakeClass`（`BoostShakeClass` と同じシェイク）。`BoostWidgetClass`（既定 `UWasamiSpeedBoostWidget`）、`TeleportAimClass`（既定 `AWasamiTeleportAim`）、`TelepathyPowerClass`（既定 `AWasamiTelepathyPower`）、`PrimalPowerClass`（既定 `AWasamiPrimalPower`）、`TelekinesisPowerClass`（既定 `AWasamiTelekinesisPower`）、`VanishPowerClass`（既定 `AWasamiVanishPower`）、`VanishWidgetClass`（既定 `UWasamiVanishWidget`）。`BeginPlay` でブーストのウィジェットの素材（`UWasamiSpeedBoostWidget::LoadAssets`）、照準の素材（`AWasamiTeleportAim::LoadAssets`）、Telepathy の印の素材（`UWasamiTelepathyTrackerWidget::LoadAssets`）、Primal の素材（`AWasamiPrimalPower::LoadAssets`）、テレキネシスの素材（`AWasamiTelekinesisPower::LoadAssets`）、Vanish の素材（`AWasamiVanishPower::LoadAssets`・`UWasamiVanishWidget::LoadAssets`）も読んで持っておく（本家はプレイヤーがそれらのクラスを参照しているので、素材は最初から読まれている。最初の使用で読み込み待ちを出さないため）。

### `AWasamiTeleportAim : AActor`（`WasamiTeleportAim.h`）
本家の `BP_Power_Teleport`（旧版）。パワーが出し、移動か取り消しで消える。
- `AdjustDistance(AxisValue)`・`Confirm()`（BlueprintCallable。ホイールと左クリック）。
- `OnUsed`（BlueprintAssignable、引数なし）… 本家の `Used`。プレイヤーを動かした直後、自分を消す直前に出す。
- `MaxDistance`（既定 1000。`ExposeOnSpawn`。パワーが出すときに強化段階の値を入れる）、読み出し用の `Distance`（既定 1000）・`Alpha`（既定 0.6）・`Location`（移動先）。
- static: `DistanceFor(Alpha, MaxDistance)` = `Lerp(250, MaxDistance, Alpha)`、`StepAlpha(Alpha, AxisValue)` = `Clamp(AxisValue / 10 + Alpha, 0, 1)`、`LoadAssets(Out)`。
- 素材（ソフト参照。`BeginPlay` で読む）: `AimingLoopSound` `/Game/DD/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227`、`CommittedSound` `/Game/DD/_Engine/VREditor/Sounds/UI/Teleport_Committed`、`CommittedShakeClass` `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`（`_C`）、`ConfirmCameraAnim` `/Game/DD/Animation/Camera/CameraAnim_Teleport`、`DecalMaterial` `/Game/DD/Blueprints/Main/Powers/M_Decal_Teleport`、`AimParticles` `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2`（`UParticleSystem`）。
- コンポーネント（本家と同じ木）: `DefaultSceneRoot` → `SpringArm`（`TargetArmLength` 0 だけ変える。ほかは UE の既定＝位置ラグの速さ 10・サブステップあり）→ `Decal`（`DecalSize` (3, 100, 100)、相対回転 (P −90, Y 0, R 5.46e-5)、拡縮 (3.3264, 1, 1)。ソケット名なしでアームに付くので、アームの先〈ラグで遅れる位置〉に付いていく）、`Decal` → `ParticleSystem`（`UParticleSystemComponent`。相対位置 (−4.13494, −0.000263, 2.3e-6)・回転 (P 90, Y 0.91133, R −359.08875)・拡縮 0.2。UE の既定の `bAutoActivate` のまま）、`DefaultSceneRoot` → `Audio`（音量 0.65、減衰なし）。本家の `Arrow`（エディタの表示用）は置いていない。`GetSpringArm()`・`GetDecal()`・`GetParticleSystem()`（C++ だけ）。
- 見た目: `BeginPlay` でデカールに `DecalMaterial` を入れ、パーティクルに `AimParticles` を `SetTemplate` する（登録済みで `bAutoActivate` なので、その場で動き出す。本家はコンポーネントのテンプレートとして持ち、スポーンで自動で始まる）。合成したワールド変換は調査 02 §2.1 どおり（PIE で読み戻し: 粒子の原点は当たった点の 13.754 cm 上、拡縮 (0.665276, 0.2, 0.2)、回転 ≈ 0）。

### `AWasamiTelepathyPower : AActor`（`WasamiTelepathyPower.h`）
本家の `BP_Telepathy`（最新版）。敵に印を付け、時間が来たら全部の印を外すだけ（音・揺れ・ゲージ・再使用はパワーのコンポーネント）。
- `Time`（既定 0。`ExposeOnSpawn`。パワーが強化段階の効果時間〈Lv5 で 9〉を入れる。Blueprint の `SetTimer` と同じく 0 秒のタイマーは仕掛けられないので、0 なら終わらない）、`TrackerClass`（既定 `AWasamiTelepathyTracker`）。
- `UpdateTargets()`（BlueprintCallable。新しく出した印の数を返す）・`Finish()`（BlueprintCallable）、読み出し `GetActorsWithTracker()`（本家の `Actors With Tracker`）。
- コンポーネントは `DefaultSceneRoot` だけ。

### `AWasamiTelepathyTracker : AActor`（`WasamiTelepathyTracker.h`）
本家の `BP_TelepathyTracker`（最新版）。敵 1 体に 1 つ。
- `Actor`（`ExposeOnSpawn`。追う敵）、`Remove()`（BlueprintCallable）、static `SizeForDistance(距離)`（BlueprintPure。`MapRangeUnclamped(距離, 0, 10000, 0.5, 0.1)`）。読み出し `IsFollowing()`（本家の Gate が開いているか）・`GetWidget()`・`GetWidgetReference()`（C++ だけ）。
- コンポーネント: `DefaultSceneRoot` → `Widget`（`UWidgetComponent`。空間 Screen、クラス `UWasamiTelepathyTrackerWidget`。ほかは UE の既定〈`DrawSize` 500 × 500・ピボット (0.5, 0.5)・`bDrawAtDesiredSize` 偽。UE 4.24 の既定と同じ〉）。本家の `WindowVisibility` Visible は UE 4.24 の既定値で、ワールド空間の仮想の窓にしか使われない（UE 5.8 も古い資産を読むと Visible に直す）ので写していない（UE 5.8 の `SetWindowVisibility` は、ウィジェットを作る前に呼ぶと ensure する）。アクタはティックする。

### `UWasamiTelepathyTrackerWidget : UUserWidget`（`WasamiTelepathyTrackerWidget.h`）
本家の `UMG_TelepathyTracker`（最新版）。
- 木: `SizeBox_54`（幅と高さの上書き 256）→ `Image_90`（ブラシ `MM_Telepathy_Inst`、ほかは既定。スロットも既定 = 箱いっぱい）。**`Initialize` で組む**（本家の BP ウィジェットは作った時点で木があり、トラッカーの `Set Size` が `Construct` より先に来うるため）。
- `Remove()`・`SetSize(Size)`（BlueprintCallable）、`TrackerMaterial`（ソフト参照 `/Game/DD/Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst`）。
- static `EvaluateAppearScale` / `EvaluateAppearOpacity`（0.5 秒のアニメ `Appear`）・`EvaluateDisappearScale` / `EvaluateDisappearOpacity`（0.3 秒の `Disappear`）、定数 `AppearLength` 0.5・`DisappearLength` 0.3、`LoadAssets(Out)`。読み出し `IsAppearPlaying()`・`IsDisappearPlaying()`・`GetSizeBox()`・`GetImage()`（C++ だけ）。

### `AWasamiPowerBurst : AActor`（`WasamiPowerBurst.h`、抽象）
本家の `BP_PrimalPower`・`BP_TelekinesisPower`・`BP_VanishPower` に共通の形。
- コンポーネント: `DefaultSceneRoot` → `PostProcess`・`PostProcess1`（`UPostProcessComponent`。UE の既定の `bUnbound` 真・`Priority` 0・`BlendRadius` 100 のまま、`BlendWeight` 0）。`GetTint()`・`GetFlash()`（C++ だけ）。
  - `PostProcess` の共通の値: `ColorSaturation` (0, 0, 0, 1) を上書き、`ColorGain` の上書きフラグ（値はパワーが入れる）。
  - `PostProcess1` の共通の値: `ColorGamma` の上書き（値は既定の (1, 1, 1, 1)。本家どおり）、`ColorGainMidtones` (100, 100, 100, 1)、`SceneFringeIntensity` の上書きフラグ（値はパワーが入れる）。本家の `PostProcess1` は上書きフラグの無い `ColorSaturation`・`ColorGain` も持つが、ブレンドに効かないので写していない。
- `TimelineLength` = 2、`TimelinePosition`（読み出し用）、`FadeCurve`（`float2` のトラック。派生クラスのコンストラクタが入れる）。
- static: `MakeCurve(Keys)`（`FWasamiCurveKey` = 時刻・値・補間・到着と出発の接線、の並びから `FRichCurve` を作る。どのキーも `RCTM_User` にして書き出しの接線を保つ）、`TintWeight(float2)` = `Lerp(1, 0, float2)`、`FlashWeight(float2)` = `MapRangeClamped(float2, 0, 0.3, 1, 0)`。
- 派生クラスが上書きするもの: `StartPower()`（`BeginPlay` の中、タイムラインの前）、`UpdateTimeline(Position)`（基底は重みを書く）。

### `AWasamiPrimalPower : AWasamiPowerBurst`（`WasamiPrimalPower.h`）
本家の `BP_PrimalPower`（最新版）。
- `Range`（既定 1500。`ExposeOnSpawn`。パワーが強化段階の値〈Lv5 で 3500〉を入れる）。
- static `StunEnemies(WorldContext, Center, Radius)`（BlueprintCallable）: 半径の中の Pawn の体を持つアクタのうち、`IWasamiEnemyInterface` を実装するものに `SetState(Stun, false)` を 1 回ずつ送り、その数を返す。
- static: `GrowthCurve()`（`float`）・`PrimalFadeCurve()`（`float2`）・`DesaturationCurve()`・`OpacityCurve()`、`LoadAssets(Out)`。
- コンポーネント: 基底の 3 つ + `Sphere`（`UStaticMeshComponent`、ルートの原点・拡縮 1、`NoCollision`、動かすので `Movable`）。`GetSphere()`・`GetMaterialInstance()`（C++ だけ）。
- 値: `PostProcess` の `ColorGain` (1.6100000143051147, 0.12956300377845764, 0, 1)（赤）、`PostProcess1` の `SceneFringeIntensity` 50。
- 素材（ソフト参照。`StartPower` で読む）: `SphereMesh` `/Engine/BasicShapes/Sphere`、`SphereMaterial` `/Game/DD/Materials/05_Circus/M_05_Primal`、`WaveSound` `/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04`、`ShakeClass` `/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop`（`_C`）。

### `AWasamiTelekinesisPower : AWasamiPowerBurst`（`WasamiTelekinesisPower.h`）
本家の `BP_TelekinesisPower`（最新版）。
- `Range`（既定 1500。`ExposeOnSpawn`。パワーが強化段階の値〈Lv5 で 3000〉を入れる）。
- static `PullShards(WorldContext, Center, Radius)`（BlueprintCallable）: 半径の中の Pawn・WorldDynamic・WorldStatic の体を持つアクタのうち、`IWasamiTelekinesisInterface` を実装するもの（シャード `AWasamiShard`、06 記録）に `Activate` を 1 回ずつ送り、その数を返す。
- static: `TelekinesisFadeCurve()`（`float2`。Primal と同じキー）、`LoadAssets(Out)`（音・シェイク・粒子の 3 つ）、定数 `ForceFieldDelay` 0.2・`ForceFieldScale` 2。
- コンポーネント: 基底の 3 つだけ（粒子はコンポーネントではなく `SpawnEmitterAtLocation` で出す）。
- 値: `PostProcess` の `ColorGain` (0, 0.4217270016670227, 1.6100000143051147, 1)（青）、`PostProcess1` の `SceneFringeIntensity` 50（Primal と同じ）。本家の CDO の `Range` 1500 はパワーが必ず上書きする。
- 素材（ソフト参照。`StartPower` で読む）: `WaveSound` `/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04`、`ShakeClass` `/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop`（`_C`）、`ForceFieldParticles` `/Game/Wasami/Powers/P_WasamiForceField`（01 記録の `dd_powers` が作る、球の灯だけを弱めた写し）。

### `AWasamiVanishPower : AWasamiPowerBurst`（`WasamiVanishPower.h`）
本家の `BP_VanishPower`（最新版）。Vanish の一瞬の演出と敵への通知だけを受け持つ（見えない扱い・ウィジェット・15 秒はパワーのコンポーネント）。
- static `NotifyEnemies(WorldContext)`（BlueprintCallable）: タグ `Enemy` の全アクタのうち、`IWasamiEnemyInterface` を実装するものに `PlayerVanish` を 1 回ずつ送り、その数を返す（距離も遮蔽も見ない）。
- static: `VanishFadeCurve()`（`float2`）、`LoadAssets(Out)`。
- コンポーネント: 基底の 3 つ + `ParticleSystem`（`UParticleSystemComponent`。相対位置 (92.42288, −0.000427, −152.14667)、`bStartWithTickEnabled` 偽〈書き出しどおり。粒子のコンポーネントは起動で自分のティックを入れる〉、UE の既定の `bAutoActivate`）。`GetParticleSystem()`（C++ だけ）。
- 値: `PostProcess` の `ColorGain` (0.6976670026779175, 0, 1.6100000143051147, 1)（紫）、`PostProcess1` の `SceneFringeIntensity` は上書きありで 0。本家の `PostProcess1` の `GrainIntensity` の上書き（既定の 0）は写していない（UE 5.8 は `GrainIntensity_DEPRECATED` として持つだけで、ブレンドも描画もしない。後継の `FilmGrainIntensity` とは別の効果）。本家の CDO の `Range` 1500 はどこからも使われないので持たない。
- 素材（ソフト参照。`StartPower` で読む）: `PuffParticles` `/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff`、`WaveSound` `/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04`。

### `UWasamiVanishWidget : UUserWidget`（`WasamiVanishWidget.h`）
本家の `UMG_Vanish`。
- 木: キャンバス `CanvasPanel_0` に `Image_82`（全面に広げ〈アンカー 0〜1、余白 0〉、ブラシ `MM_WobblyVignette`、色 (0.278697, 0.160557, 0.536458, 1)、描画の拡縮 1.05〈中心基準〉）。`RebuildWidget` で作る。
- `Speed`（既定 1。`ExposeOnSpawn`。パワーが 15 を入れる）、`VignetteMaterial`（ソフト参照 `/Game/DD/Materials/Special/MM_WobblyVignette`）。
- static `EvaluateOpacity(アニメの秒)`（1 秒のアニメ `Vanish` の `RenderOpacity` のトラック）、`LoadAssets(Out)`、定数 `AnimationLength` 1。読み出し `GetAnimationTime()`・`IsAnimationPlaying()`・`GetVignette()`（C++ だけ）。

### `AWasamiTestEnemy : AActor`（`Tests/WasamiTestEnemy.h`）
敵（M4）の代わりの仮の的。テストと PIE の確認だけに使う。
- ルートはカプセル（半径 34・半高 118.058〈病院のナース〉、プロファイル `Pawn`、ゲームでも見える）、タグ `Enemy`、`IWasamiEnemyInterface` を実装し、`SetStateCount`・`State`・`bLastByOrb`・`PlayerVanishCount` を数える。`bNoTelepathy` が `NoTelepathy` の答え。
- static `SpawnTestEnemy(WorldContext, Location)`（BlueprintCallable。PIE の Python からも出せる）。

### `UWasamiCameraAnim : UDataAsset`（`WasamiCameraAnim.h`）
本家の `CameraAnim`（UE4 の `UCameraAnim`。UE 5 には無い）を取り込みが写したもの（01 記録の `dd_assets.camera_anim`）。`AnimLength`（既定 3）・`BaseFOV`（既定 90。書き出しの値を持つだけで、再生には使わない）・`BasePostProcessSettings`（上書きフラグごと）・`BasePostProcessBlendWeight`（既定 0 = PP が効かない。UE4 と同じ）・`FloatTracks` / `ColorTracks`（`FWasamiCameraAnimFloatTrack` / `FWasamiCameraAnimColorTrack` = `PropertyName`〈`CameraComponent.PostProcessSettings.SceneColorTint` のような原作の名前〉と Matinee の曲線 `FInterpCurveFloat` / `FInterpCurveLinearColor`）。
- `FindFieldOfViewTrack()` … `CameraComponent.FieldOfView` のトラック（無ければ null）。
- `ApplyPostProcessTracks(Time, Settings)` … `CameraComponent.PostProcessSettings.` で始まるトラックの値を、`FPostProcessSettings` の同名のメンバー（float か `FLinearColor`）へ書く。上書きフラグは触らない（UE4 でもトラックは値だけを動かし、フラグは基準の設定のまま）。評価は `FInterpCurve::Eval`（保存された接線のまま。UE4 の Matinee と同じ式）。

### `FWasamiCameraAnimPlayback`（`WasamiCameraAnim.h`）
再生中のアニメの時間の進み方（UE4 の `UCameraAnimInst` の写し。純粋な値の構造体でテストできる）。`Start(AnimLength, Rate, Scale, BlendIn, BlendOut, bLoop, Duration)`・`Advance(DeltaTime)`・`Stop(bImmediate)`、読み出しは `CurTime`・`Weight`・`bBlendingOut`・`bFinished`。

### `UWasamiCameraAnimModifier : UCameraModifier`（`WasamiCameraAnim.h`）
- `Get(PlayerCameraManager)` … カメラマネージャのこのモディファイアを返す（無ければ足す）。
- `Play(Anim, Rate, Scale, BlendInTime, BlendOutTime, bLoop, Duration)` → ハンドル（本家の `PlayCameraAnim`。`bRandomStartTime` は常に false、再生空間は CameraLocal 相当で、移動も回転もしない）、`Stop(Handle, bImmediate)`、`IsPlaying(Handle)`。
- static `AddFieldOfView(ViewFOV, TrackFOV, InitialFOV, Weight)` = `Clamp(ViewFOV + (TrackFOV − InitialFOV) × Weight, 5, 170)`。

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
本家の `DD_TelekinesisInterface`。`Activate()`（`BlueprintNativeEvent`、既定は何もしない）。本家では `BP_Shard` が実装し、テレキネシスが届くとプレイヤーへ飛んで回収される。本作では `AWasamiShard` が実装する（06 記録）。

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
4. `Use Power` 側の DoOnce（`bUseClosed`）を通ったら、`OnPowerUsed` を出し、パワーごとの処理へ（`UseSpeedBoost`・`UseTeleport(bLeft)`・`UseTelepathy`・`UsePrimal`・`UseTelekinesis`・`UseVanish`）。
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
  - 左クリック（`Confirm`）: `Location = デカールのワールド位置 + (0, 0, 125)` を**先に**書き、DoOnce（`bConfirmed`）を通ったら、プレイヤーのカプセルの `WorldDynamic` と `Pawn` の応答を Ignore にし、0.12 秒後に移動（`Commit`）。移動の前の 2 回目のクリックは移動先だけを変える。DoOnce を通った瞬間に、`GetPlayerCameraManager(0)` の `UWasamiCameraAnimModifier` で `CameraAnim_Teleport` を `Play(…, Rate 1, Scale 1, BlendIn 0, BlendOut 0, ループなし, Duration 0)` する（本家の @641。再生空間 CameraLocal は移動のトラックが原点だけなので扱わない）。0.5 秒で自然に終わり、取り消し・照準の破棄・死亡のリセットでは止めない（本家もカメラマネージャが持ち続ける）。
  - 移動（`Commit`）: `BP_CameraShake_Streak` を倍率 1・`CameraLocal` → `Teleport_Committed` を `PlaySound2D`（音量 1）→ `SetActorLocation(Location, スイープあり, TeleportPhysics)`（壁などで止まる。カプセルの中心を床 + 125 cm に置くので、立ち姿の 88 cm まで約 37 cm 落ちる）→ カプセルの `Pawn` と `WorldDynamic` を Block に戻す → `OnUsed` → 自分を消す。
  - 見た目（`BeginPlay`。下の「作るアセット」）: 2 m 四方のデカールに、1 秒周期で明滅する縁の鋭い赤い円（半径約 70 cm）を加算で出す。デカールの子のパーティクル `P_ky_cutter2` が、回る斬撃（大きさ 199.6 cm の正方形のスプライトが寿命 1 秒で 2 倍に広がり、4 × 4 のコマを進める）と上へ昇る赤い火花（毎秒 30、寿命 2 秒）を出す。粒子はローカル空間なので、アームのラグと一緒に動く。
  - `EndPlay` で 2 つのタイマー（ラグの有効化・移動）を止める（UE 5.8 のアクタは消えるときに自分のタイマーを消さない。本家の Delay はアクタと一緒に消える）。
- 移動の後（`UsedTeleport`。本家の @30854）: 使った側の `bCanCycle*` を真 → `Active Powers` から外す → ゲージの `SetDelay(5)`（アイコンが 0 → 1 を 5 秒）→ Gate が開いていれば `Delay(5)` の後に `RefillTeleport`。再使用は段階によらず 5 秒（本家の `00_Ballroom` だけの 1 秒は病院に無い）。
- 充填（`RefillTeleport`）: 充填の DoOnce が開いていれば閉じて `Refill`（`power_refilled` 0.5・使える状態）。
- 取り消し・死亡のリセット（`ResetTeleport`。本家の `Reset Teleport` @31827 と `BP_Powers` の `Stop Teleport Timeline`）: Gate を閉じる → `RefillTeleport`（使った後なら即座に充填・音）→ 照準があれば消す → `UsedTeleport`（Gate が閉じているので `Delay` は始まらない）→ ゲージを止めて 1。結果、すぐ使える・アイコンは 1・`power_refilled` が 1 回。
  - **クリックから移動までの 0.12 秒の間に取り消すと、移動は起きず、カプセルは `Pawn` と `WorldDynamic` を無視したまま残る**（本家どおり。次のテレポートの移動で戻る）。

### Telepathy（`UseTelepathy` → `EndTelepathy` → `RefillTelepathy`、アクタは `AWasamiTelepathyPower`、印は `AWasamiTelepathyTracker`）
- 使った瞬間（本家の @16461〜@18164）: `Active Powers` に足す → 開始の音 `Telepathy` を `PlaySound2D` の音量 0.6 → 使えない状態 → `BP_CameraShake_Streak` を倍率 1・`CameraLocal` → `AWasamiTelepathyPower` を**ワールドの原点**に回転 0 で遅延スポーンし（衝突の扱いはクラスの既定）、`Time` に効果時間（Lv5 で 9 秒）を入れてから `FinishSpawning` → ゲージの `SetDelay(効果時間)`（アイコンが 1 → 0）→ `Delay(効果時間)` で `EndTelepathy`。効果時間はプレイヤーの Delay とアクタの `Finish` のタイマーが別々に数える。
- 終わり（`EndTelepathy`、@4271〜@4971）: `Active Powers` から外す → 終わりの音 `Teleport_Mode_Entered`（エンジンの VREditor の音。テレポートの照準の開始と同じもの）を `PlaySound2D` の音量 1.0・ピッチ 1.5 → 再使用（Lv5 で 6.5 秒）でゲージの `SetDelay`（アイコンが 0 → 1）→ `Delay(再使用)` で `RefillTelepathy`（`power_refilled` 0.5・使える状態。前の Gate は素通し）。
- アクタ（`AWasamiTelepathyPower`）: `BeginPlay` ですぐ `UpdateTargets`、0.8 秒ごとの繰り返しのタイマー、`Time` 秒後の `Finish` のタイマー。`EndPlay` で 2 つのタイマーを止める。
  - `UpdateTargets`: `GetAllActorsWithInterface(IWasamiEnemyInterface)` で**レベルの全敵**（距離も遮蔽も見ない）を集め、`NoTelepathy` が真のもの（本家では `BP_08_BearTrap` だけ）と、`ActorsWithTracker` に入っているものを飛ばし、残りの敵ごとに印を**敵の位置**に回転 0 で遅延スポーンして `Actor` を入れ、`FinishSpawning` して配列に足す。効果中に現れた敵も次の 0.8 秒で拾う。タグ `Enemy` だけでインターフェースの無いもの（本家の Zone 2 のマトロン）には付かない。敵がいなくても、使用・音・揺れ・ゲージ・再使用は同じ。
  - `Finish`: 繰り返しのタイマーを止め、**ワールドの全ての印**（`TrackerClass` のアクタ）に `Remove` → 自分を消す。
- 印（`AWasamiTelepathyTracker`）: `BeginPlay`（コンポーネントの `BeginPlay` がウィジェットを作った後）でウィジェットを `WidgetReference` に持ち、Gate を開く。毎ティック、Gate が開いていれば `Update`: 敵が無効なら `Remove`、有効なら敵の原点（カプセルの中心）へ `SetActorLocation`（スイープなし）し、プレイヤー（`GetPlayerCharacter(0)`。いなければ距離 0）との距離から `SizeForDistance` をウィジェットの `SetSize` に入れる（0 cm で 0.5、50 m で 0.3、100 m で 0.1、125 m で 0、それより遠いと負 = 反転。本家どおりクランプしない）。
  - `Remove`: Gate を閉じる → ウィジェットの `Remove`（Disappear）→ `Delay(0.5)` で自分を消す（数えている間の 2 回目は仕掛け直さない）。`EndPlay` でそのタイマーを止める。
  - 画面での出方: UE 5.8 の画面空間のウィジェットは、ビューポートの層（`SWorldWidgetScreenLayer`）で、コンポーネントの位置を投影した点に `DrawSize` 500 × 500 の枠をピボット (0.5, 0.5) で置く（カメラの後ろなら出さない。近いものほど上）。ルートの `SizeBox` の 256 は希望の大きさにしか効かず、中身は 500 × 500 に広がるので、印の大きさは「500 × `SetSize` × Appear の拡縮 × DPI の拡大率」（距離 0 で 250、100 m で 50）。世界の描画の上に重なるので壁越しに見える。シーンキャプチャには写らないので**タブレットの地図には出ない**（本家どおり。地図は `ShowOnlyActors` だけを描く。03 記録）。
- ウィジェット（`UWasamiTelepathyTrackerWidget`）: `NativeConstruct`（画面の層に載って Slate の部品ができたとき = 印の最初のティックのころ）で、Appear を頭から再生して**その場で最初のフレームを入れ**（拡縮 0・不透明度 0）、`Image_90` の MID を作って `Tiling`・`Speed` に 0.5〜1.5 の乱数、`Image_90` の描画の角度に 0.5〜360 の乱数を入れる（この順。本家の `RandomFloatInRange`）。`NativeTick` で Appear → Disappear の順に時刻を進めて値を書く（UE 4.24 の `UUserWidget` は再生中のプレイヤーを始めた順に進めるので、両方が動くときは Disappear が勝つ）。`Remove` は Disappear を頭から（再生中でもやり直し）、最初のフレームをその場で入れる。`SetSize` は `SizeBox_54` の描画の拡縮を (値, 値) にする。
  - アニメ（ティックは 1 秒 60000。キーは 3 次で、書き出しの 1 ティックあたりの接線を × 60000 して `FRichCurve` で評価）: **Appear**（再生範囲 [0, 30001)）は `Image_90` の拡縮 0 / 15000 / 30000 → 0 / 1 / 0.95（接線 0 / 3e-5 / 0）と不透明度 0 / 30001 → 0 / 1。**Disappear**（[0, 18001)）は拡縮 0 / 9000 / 18000 → 1 / 1.1 / 0（接線 0 / −3.33e-5 / 0）と不透明度 0 / 18000 → 1 / 0。どちらも描画の変換は拡縮の X / Y だけを書き、角度はそのまま（UE の 2D 変換のトラックは、データのあるチャンネルだけを書く）。
  - 終わり: UE 5.8 は再生範囲の終わりの 1 ティック前（Appear は 0.5 秒、Disappear は 0.3 秒）で最後に評価し、値を保つ（`UWidgetAnimation` の既定の終わり方は KeepState）。Appear は拡縮 0.95・不透明度 ≈ 1 で止まる。Disappear の拡縮の区間は [0, 18000) なので、最後の評価では拡縮を書かず（直前の値のまま）、不透明度 0 を書く。
- 死亡のリセット: **Telepathy は戻さない**（本家の `Reset All Powers` に無い。効果中の印もそのまま残る）。

### 一瞬の演出の基底（`AWasamiPowerBurst`）
- `BeginPlay`: アクタの `BeginPlay` → `StartPower()` → タイムラインを頭から（位置 0 にして **その場で 1 回更新**。UE4 / UE 5.8 の `FTimeline::PlayFromStart` が `SetPlaybackPosition(0)` の更新を出すのと同じ）。
- 毎ティック: 位置 + `DeltaSeconds` が長さ 2 を**超えたら**、位置を 2 に揃えて止め、最後の更新をしてから自分を消す（`FTimeline::TickTimeline` と同じ。ちょうど 2 のティックでは終わらない）。タイムラインの再生速度は 1。
- 更新（`UpdateTimeline`）: `float2` を評価し、`PostProcess.BlendWeight = TintWeight`、`PostProcess1.BlendWeight = FlashWeight`。`float2` は開始前（−0.0116 秒）のキーから負に振れるので、位置 0 では `PostProcess` の重みが 1.000698 になる（本家どおり）。
- `UTimelineComponent` と `UCurveFloat` は使わず、アクタのティックで `FRichCurve` を評価する。`FRichCurve::AddKey` は前の Auto のキーの接線を計算し直すので、キーはすべて `RCTM_User` にして入れる（評価は接線のモードを見ない）。

### Primal Fear（`UsePrimal` → `StartPrimalCooldown` → `RefillPrimal`、アクタは `AWasamiPrimalPower`）
- 使った瞬間（本家の @18337〜@19621）: `Active Powers` に足す → 使えない状態 → ゲージの `SetDelay(0.05)`（アイコンが 0.05 秒で 0）→ `AWasamiPrimalPower` をプレイヤーの位置の **50 m 下**に回転 0・`AlwaysSpawn` で遅延スポーンし、`Range` に強化段階の値（Lv5 で 3500）を入れてから `FinishSpawning` → `Delay(0.06)` で `StartPrimalCooldown`。
- `StartPrimalCooldown`（@5187〜@6084）: 再使用の秒数（Lv5 で 23。本家の `00_Circus_Entrance` だけの 5 秒は病院に無い）でゲージの `SetDelay`（アイコンが 0 → 1）→ `Active Powers` から外す → `Delay(再使用)` で `RefillPrimal`（`power_refilled` 0.5・使える状態。前の Gate は素通し）。
- アクタの `StartPower`（本家の `ReceiveBeginPlay` @915〜@1450）: 球にメッシュを入れ、`M_05_Primal` から MID を作る → プレイヤー（`GetPlayerCharacter(0)`）のカプセルの中心へ `SetActorLocation`（スイープなし。以後はプレイヤーに付いていかない）→ `Stun_Wave_Attack_New_04` を `PlaySoundAtLocation` の位置 (0, 0, 0)・音量 1・ピッチ 1（減衰の設定が無いので空間化されず、どこでも同じに聞こえる）→ `GetPlayerController(0)` の `ClientStartCameraShake(01_Hotel_Lobby_ElevatorShakeStop, 25, CameraLocal)` → `StunEnemies(プレイヤーの位置, Range)`。
  - `StunEnemies`: `UKismetSystemLibrary::SphereOverlapActors`（オブジェクトの種類は Pawn だけ、クラスの絞り込みなし、除外なし）。**遮蔽は見ない**（壁越し・上下の階にも効く）。判定は 0 秒の 1 回だけで、球の広がりとは連動しない。本家は「インターフェースを実装する、または `Enemy` タグ」で絞ってからインターフェースへキャストするので、実際に届くのは実装するものだけ。本作はそれを直接書く（タグだけの敵〈本家の Zone 2 のマトロン〉には効かない）。本家の `PrintText`（デバッグ表示。出荷版では出ない）と、何にもつながっていない DoOnce は写していない。
  - 敵がいなくても、音・揺れ・見た目・再使用は同じ。気絶の秒数は敵の側が決める（病院のナースは 17 秒。M4）。
- 更新（`UpdateTimeline`、本家の @1511〜@1938）: 球の拡縮 = `Lerp(0, Range, float) / 50`（半径 = `Range × float`。位置 0 で 0、最後は `Range` の 98.35%）→ 基底の重み → MID の `Desaturation`・`Opacity` にトラックの値。
- 見え方（Lv5）: 0.5 秒で球の半径が約 2150 cm、1 秒で 3225 cm、1.5 秒で 3440 cm。画面は赤い単色（彩度 0 × ゲイン (1.61, 0.13, 0)）が 0.5 秒で消え、白い閃光（中間調 × 100・色収差 50）が約 0.29 秒で消える。2 秒で自分を消す。
- 死亡のリセット: ゲージを止めて 1 → 充填（使っていなくても `power_refilled` が鳴る。下の「死亡のリセット」）。動いている 0.06 秒と再使用の Delay は止めないので、リセットの後に古い Delay が切れると、もう一度充填の音が鳴る（本家の癖どおり）。

### テレキネシス（`UseTelekinesis` → `StartTelekinesisCooldown` → `RefillTelekinesis`、アクタは `AWasamiTelekinesisPower`）
- 使った瞬間（本家の @19661〜@20984）: `Active Powers` に足す → 使えない状態 → ゲージの `SetDelay(0.05)`（アイコンが 0.05 秒で 0）→ `AWasamiTelekinesisPower` をプレイヤーの位置の **50 m 下**に回転 0・`AlwaysSpawn` で遅延スポーンし、`Range` に強化段階の値（Lv5 で 3000）を入れてから `FinishSpawning` → `Delay(0.06)` で `StartTelekinesisCooldown`。Primal と同じ形。
- `StartTelekinesisCooldown`（@8329〜@9937）: 再使用の秒数（Lv5 で 8。本家の `00_Ballroom` だけの 1 秒は病院に無い）でゲージの `SetDelay`（アイコンが 0 → 1）→ `Active Powers` から外す → `Delay(再使用)` で `RefillTelekinesis`（`power_refilled` 0.5・使える状態。前の Gate は素通し）。使ってから再び使えるまで 8.06 秒。
- アクタの `StartPower`（本家の `ReceiveBeginPlay` @661〜@1151）: プレイヤー（`GetPlayerCharacter(0)`）のカプセルの中心へ `SetActorLocation`（スイープなし。以後はプレイヤーに付いていかない）→ `Stun_Wave_Attack_New_04` を `PlaySoundAtLocation` の位置 (0, 0, 0)・音量 1・ピッチ 1（Primal と同じ）→ `GetPlayerController(0)` の `ClientStartCameraShake(01_Hotel_Lobby_ElevatorShakeStop, 25, CameraLocal)` → `PullShards(プレイヤーの位置, Range)` → 基底がタイムラインを再生 → 0.2 秒のタイマーで `SpawnForceField`。
  - `PullShards`: `UKismetSystemLibrary::SphereOverlapActors`（オブジェクトの種類は本家の `[2, 1, 0]` = Pawn・WorldDynamic・WorldStatic、クラスの絞り込みなし、除外なし）。**遮蔽は見ない**（壁越し・上下の階のシャードも寄る）。判定は 0 秒の 1 回だけで、後から範囲に入ったシャードは対象にならない。本家は `DoesImplementInterface` で絞ってからインターフェースへキャストするので、届くのはシャード（`AWasamiShard`。06 記録の `Activate`: 0.8〜1.2 倍速の 1 秒でプレイヤーへ水平に寄り、終わりに届いていなくても回収）だけ。カプセルの当たりを切ったシャード（本家の `bDisabled`）は問い合わせに入らない（テスト `TelekinesisPull`）。
  - `SpawnForceField`（本家の `Delay(0.2)` → `SpawnEmitterAtLocation` @15〜@43）: `ForceFieldParticles` が読めれば、アクタの位置（使った瞬間のプレイヤーの位置。付いていかない）に回転 0・拡縮 2・自動破棄・プールなし・自動起動で出す（粒子が読めなければ何も出さない）。粒子のコンポーネントはワールドの `WorldSettings` に付き、エミッタの長さ 2 秒の後に消える。
- 更新（`UpdateTimeline`）: 基底の重みだけ（本家の `float`・`desaturation`・`opacity` のトラックは Primal と同じキーで、どこにもつながっていない）。
- 見え方（Lv5）: 画面は青い単色（彩度 0 × ゲイン (0, 0.42, 1.61)）が 0.5 秒で消え、白い閃光（中間調 × 100・色収差 50）が約 0.29 秒で消える。2 秒で自分を消す。半径 30 m のシャードが 0.8〜1.25 秒でまとめて回収され、その数だけ `Count Shake` と回収の音（`OnlyFew`。06 記録の未解決の重なり）。
- 死亡のリセット: **何もしない**（本家の `Reset Telekinesis` はどこからも呼ばれない。死んでも再使用の Delay は続く）。

### Vanish（`UseVanish` → `EndVanish` → `RefillVanish`、アクタは `AWasamiVanishPower`、画面は `UWasamiVanishWidget`）
- 使った瞬間（本家の @20985〜@22969）: `Active Powers` に足す → 使えない状態 → **プレイヤーのカプセルの `ECC_Camera` の応答を Ignore**（本家の敵の視線は `Camera` チャンネルのトレースなので、線がプレイヤーを素通りして後ろの壁に当たる。敵〈M4〉の視線をこのチャンネルで作れば同じ仕組みになる）→ ゲージの `SetDelay(15)`（アイコンが 15 秒で 1 → 0）→ `UWasamiVanishWidget` を作り（持ち主はプレイヤーのコントローラ。本家は `OwningPlayer` なしの `Create` で、最初のローカルプレイヤーになる）、`Speed = 15`、`AddToPlayerScreen(0)` → `AWasamiVanishPower` をプレイヤーの位置の **50 m 下**に**プレイヤーの向き**で `AlwaysSpawn` の遅延スポーンをして `FinishSpawning` → `Delay(15)` で `EndVanish`。効果時間は段階によらず 15 秒（`FWasamiPowerTuning::VanishDuration`。ウィジェットの `Speed` も同じ 15）。
- 終わり（`EndVanish`、@6856〜@7725）: ゲージの `SetDelay(再使用)`（Lv5 で 15 秒、アイコンが 0 → 1）→ `Active Powers` から外す → カプセルの `ECC_Camera` を Block に戻す → `Delay(再使用)` で `RefillVanish`。**敵へは何も知らせない**。効果中に解除する処理も無い（シャードを取る・別のパワーを使う・捕まる、のどれでも続く）。
- 充填（`RefillVanish`、@7927〜@8292）: `Refill`（`power_refilled` 0.5・使える状態）→ ウィジェットを外す（`RemoveFromParent`。15 秒の時点で不透明度は 0 になっているので、見た目は変わらない）。前の Gate は素通し（ステップ 2 の決定）。本家どおり、外したウィジェットへの参照は持ったまま。
- アクタの `StartPower`（本家の `ReceiveBeginPlay` @471〜@705）: 粒子に `PPP_VanishPuff` を `SetTemplate`（登録済みで自動起動なので、その場で動き出す。本家はクラスのテンプレートとしてスポーンで起動する）→ プレイヤーのカプセルの中心へ `SetActorLocation`（スイープなし。回転はスポーンのまま）→ `Stun_Wave_Attack_New_04` を `PlaySoundAtLocation` の位置 (0, 0, 0)（Primal と同じ）→ `NotifyEnemies`。基底のタイムラインは重みだけを動かす（本家の `float`・`desaturation`・`opacity` のトラックは Primal と同じキーで、どこにもつながっていないので作らない）。
- 粒子（`PPP_VanishPuff`）は Tick で出るので、移した後の位置に出る: カプセルの中心から、プレイヤーの向きで前へ 92.4 cm、下へ 152.1 cm に置かれ、`LocationWorldOffset` (0, 0, 150) を足して床から約 86 cm の胸の前に、煙が 5 つ（0 秒のバースト）出て、上へ加速しながら 0.5〜1 秒で消える。
- 見え方: 画面は紫の単色（彩度 0 × ゲイン (0.698, 0, 1.61)）が 0.3 秒で消え、白い閃光（中間調 × 100、色収差なし）が約 0.12 秒で消える。2 秒でアクタを消す。
- ウィジェット（`NativeConstruct` / `NativeTick`）: `Construct` で本家の `PlayAnimation(Vanish, 0, 1 回, 順方向, 1 / Speed)` を写し、アニメの時刻 0 の値をその場で入れる（`Speed` が 0 なら再生速度 0 = 動かない。Blueprint の割り算の 0 除算は 0）。毎ティック、時刻 += 経過 × 再生速度、1 秒を超えたら 1 に揃えて止める。値は `Image_82` の `RenderOpacity`。
  - アニメのトラック（`MovieSceneFloatSection_0`）: ティック 0 / 6000 / 54000 / 60000（60000 = 1 秒。`TickResolution` は書き出しに無く `UMovieScene` の既定）に値 0 / 1 / 1 / 0、補間は 3 次、接線は 1 ティックあたり 0 / 1.0416667e-6 / −1.0101010e-6 / 0。秒あたりに直して（× 60000）`FRichCurve` で評価する（ムービーシーンの 3 次の式は `FRichCurve` と同じ。03 記録の枠の弾みと同じ扱い）。
  - 1/15 倍速で、0〜1.5 秒で 0 → 1、1.5〜13.5 秒は 1（キーの間で最大 1.0123）、13.5〜15 秒で 1 → 0。以後は 0 のまま、再使用が明けて外されるまで画面にある。
- 敵の側（M4 で作る）: 本家の 3 つの経路のうち、(1) カプセルの `Camera` 応答、(2) `IsUsingPower(Vanish)`（本家の `Active Powers` の 5）、(3) 使った瞬間の `PlayerVanish`（病院のナースは `Seen Player Recently = False` で追跡をやめる）が本作にもそろった。Vanish 中でも触れれば捕まる（本家の捕獲は Vanish を見ない）。
- 死亡のリセット: ゲージを止めて 1 → `RefillVanish`（使っていなくても `power_refilled` が鳴り、ウィジェットを外す）。**カプセルの応答と `Active Powers` は元の 15 秒の `EndVanish` まで残る**（本家どおり）。リセットの直後に使い直すと、動いている `Delay(15)` は仕掛け直されないので、先の Delay で終わる。

### カメラアニメの再生（`UWasamiCameraAnimModifier::ModifyCamera`）
- 毎フレーム、再生中の各アニメを `Advance` し、終わっていなければ、FOV のトラックがあれば視点の FOV に `AddFieldOfView(視点の FOV, キーの値, InitialFOV, Weight)` を入れ、`BasePostProcessSettings` の写しにトラックの値を書き、重み `BasePostProcessBlendWeight × Weight` が正なら `AddCachedPPBlend(…, VTBlendOrder_Base)` する（UE4 はカメラアニメの PP を通常のカメラの PP の下に重ねた。UE 5.8 の後継も `r.CameraAnimation.LegacyPostProcessBlending`〈既定 true〉で同じ位置に置く）。終わったものは外す。PP の値の重ね合わせ（`SceneColorTint` は重みで線形補間）はエンジンが行う。
- `Advance`（UE 5.8 の後継 `CameraAnimationCameraModifier.cpp` の `TickAnimation` と同じ形。UE4 の `CameraAnimInst.cpp` は手元に無い）: 時間を `Delta × Rate` 進め、ブレンドの経過も進める。ループしないアニメは「長さ − BlendOut × Rate」を過ぎたらブレンドアウトを始め、長さを過ぎたら終わる。ブレンドインは経過が BlendIn を超えたら終わる。ブレンドアウトの経過が BlendOut を超えたら終わる。重みは `min(ブレンドインの経過 / BlendIn, 1 − ブレンドアウトの経過 / BlendOut) × Scale`（どちらも直線）。`Duration` が正なら、`Duration − BlendOut` を数え終えたところで `Stop(false)`（ブレンドアウト）を呼ぶ。**`Duration` はブレンドアウトを含む長さ**（UE 5.8 の `FCameraAnimationParams::DurationOverride` の説明「including blends」。後継はこの値を使っていない）。
- `Stop(bImmediate)`: 即座なら（または BlendOut が 0 なら）終わり・重み 0、そうでなければブレンドアウトを始める（既に始まっていれば続ける）。
- スピードブーストでは: 0〜0.5 秒で色調が 0 → 1、9.25 秒まで 1、9.25〜9.75 秒で 1 → 0。`CameraAnim_SpeedBoost` の色調のトラックは開始前（−0.0018 秒）の 1 キーだけなので、値は常に (2.0, 0.584, 0.498)。
- **FOV のトラック**（`CameraComponent.FieldOfView`）: `Play` の時点のキーの値（開始時刻で評価。`CameraAnim_Teleport` では 90）を `InitialFOV` に持ち、毎フレーム「キーの値 − `InitialFOV`」に重みを掛けて視点の FOV に足し、5〜170° に収める（UE4 の `bRelativeToInitialFOV`〈書き出しに無い = 既定の真〉の形）。**基準は `BaseFOV`（137.24）ではない**: 旧版の実機で `CameraAnim_Teleport` を 60 fps で収録し、クリックの直後に画面が広がること、閃光の後（アニメの 0.24〜0.40 秒）の拡大率がアニメの後に対して 1.26〜1.06（画角 ≈ 103°〜93°）で、終わりで跳ばないことを確かめた（`BaseFOV` 基準なら 43〜51° に狭まり、終わりで 90° へ跳ぶ）。プレイヤーの FOV（ダッシュで動く）の上に足すので、ダッシュ中でも変化の量は同じ。Move トラックは取り込まない。PP とは別に、`BasePostProcessBlendWeight` が 0 でも FOV は効く。`Play` は PP と FOV 以外のトラックにだけ警告を出す。

### FX（`UWasamiChameleonComponent`）
- 本家の Chameleon は、範囲なし（`Unbound`）の `PostProcessComponent`（`InternalPP`）を持ち、毎ティック `InitChameleon` で「`Native Post Process`（上書きなし）で設定を上書き → 有効な効果ごとに MID のパラメータを書いて `AddOrUpdateBlendable(MID, 1)`」を行う。本作は `BeginPlay` で持ち主に `UPostProcessComponent`（`bEnabled`・`bUnbound`）を作って付け（`UPostProcessComponent` は MinimalAPI で他のモジュールから派生できない）、揺れのマテリアルの MID を作る。毎ティック、ボリュームのブレンダブルを空にし、`bCameraShake` なら MID に `ShakePower` / `ShakeFQ` を書いて重み 1 で足す。
- 揺れの詳細設定（本家の `Camera Shake - Advanced`）は CDO の既定のまま（ブレンド `0 - Normal`・`BlendingOpacity` 1・カスタム深度とステンシルなし・距離のブレンドなし・白のマスク）なので、効果をそのまま画面に出す。
- **`Radial Blur` は作らない**。CDO の `Radial Blur`（有効フラグ）は false で、プレイヤーのテンプレートは `Custom Depth Highlighter` 系しか上書きせず、どのコードも true にしない（`Chameleon_C.Radial Blur` への書き込みはプレイヤーの `Radial Blur Width` だけ）。`InitChameleon` は無効な効果の関数を素通りするので、本家でも放射ブラーは出ていない。
- プレイヤーのテンプレートは `Custom Depth Highlighter (Clip)` を有効にしている（縁取り (1,0,0)、中 (0.0802,0,0)）。病院の `BP_06_ReaperNurse` などの敵が `SetRenderCustomDepth` を呼んで赤く縁取られる仕組みだが、最新版ではナースの `Custom Depth(Duration)` を呼ぶ者がいない（`.claude/references/powers/04-primal-telepathy.md` 3.10 節）。**本作では作らない**（2026-09-17 のユーザーの回答「不要」）。
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
4. Vanish: ゲージを止めて 1 → `RefillVanish`（充填とウィジェットを外す。カプセルの応答と `Active Powers` は戻さない）。
- **Primal と Vanish は、使っていなくても充填を通るので `power_refilled` が鳴る**（本家の `Reset Primal` = @37261: Push @6444 → @27788 の Gate の Close。`Reset Vanish` = @37737 も同じ形）。Telepathy と Telekinesis はリセットしない。
- 本家のクールダウン明けの Gate（Telepathy・Primal・Telekinesis・Vanish）は「Open の直後に Enter」（@6301〜@6444 ほか）なので、閉じても次の Enter の前に必ず開く＝素通しと同じ。Gate の状態は持たない。
- 呼ぶのは死亡画面（`UWasamiDeathScreenWidget` の再開の段。本家の `UMG_DeathScreen` と同じく暗転の 2 秒後、今のレベルを開き直す直前。09 記録）。ゲームオーバーのボタンの道は呼ばない（開き直しでパワーも作り直される）。

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
| `/Game/DD/Animation/Camera/CameraAnim_Teleport` | `WasamiCameraAnim`（**旧版** `pak_reference`）。長さ 0.5 秒、`BaseFOV` 137.24（再生には使わない）、`BasePostProcessBlendWeight` 1.0、基準の PP は `bOverride_WhiteTemp`・`WhiteTint`・`SceneColorTint`・`AutoExposureBias` が真（`AutoExposureBias` 1.1223 はトラックが t=0 から上書きする）。トラックは FOV（0 → 90 / 0.13 → 150 / 0.18 → 80 / 0.25 → 100 / 0.4 → 90、接線 0）・`AutoExposureBias`（6 キー、0.1354 秒に 100）・`SceneColorTint`（5 キー、0.128〜0.211 秒に (2.0, 0.1145, 0.0)）、すべて `CIM_CurveAutoClamped`。読み戻して 02-teleport.md §5.1 の値と一致 |
| `/Game/DD/UI/Main/Powers/T_Speedlines` | 集中線（3841 × 5404、2 列 × 5 段のコマ。sRGB・`TC_Default`・`TEXTUREGROUP_UI`・`NeverStream` 真〈原作の書き出し〉。原作の cook も非圧縮 BGRA8・ミップ 1 で、本作の実測も約 81 MB〈`blueprint_get_memory_size` 84,934,656〉） |
| `/Game/DD/UI/Menu/Streaks/T_VignetteNew` | ビネット（1024²、白地にアルファで縁。sRGB・`TC_EditorIcon`・`TEXTUREGROUP_UI`。本作は 4 MB〈ミップなし。原作の cook は 11 段のミップあり。全面に引き伸ばすだけなので見た目は同じ〉） |
| `/Game/DD/UI/Main/Powers/M_Speedlines` | User Interface・Translucent。`/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook` の呼び出し（列 = 定数 2、段 = 定数 5、位相 = `Time` × 3〈`Multiply` の B〉、UV は既定の `TexCoord 0`）の出力 2 番（`UVs`）を `T_Speedlines` の `TextureSample` の UV に、その RGB を Emissive に、A を Opacity に（原作のコンパイル済みシェーダーの Slate の画素シェーダーが `saturate(A)` を不透明度にしている。`Tools/dd/cooked_shaders.py "UI/Main/Powers/M_Speedlines." --show 4`。テクスチャの画面いっぱいの横線は RGB が白で A が 0 なので、これで消える）。原作の書き出しに残るのは呼び出しと `TextureSample` の 2 つだけで、`Expressions` の残り 4 つ（null）と呼び出しの入力は cook で消えている。その 4 つを定数 2・定数 5・`Time`・`Multiply` と読み、値は最新版の実機の無劣化の連写とテクスチャの照合で決めた（1 コマが画面いっぱいに 1 コマ、コマ 0〜9 の順、1 秒に約 30 コマ。`observations/README.md` の「視点の速さと集中線」。WebGL 版の 60 コマ/s とは違う）。UE 5.8 の FlipBook は位相の小数部を中で取り（`Clamp Anim` 偽）、コマ k = floor(10 × 位相) を U = k / 2（ラップで列が回る）・V = floor(5 × 位相) / 5 で引く（左→右・上→下）。4.24 の書き出しと同じ 38 ノードで、出力の並びは `SortPriority` だけで決まる（`MaterialExpressions.cpp` の `GetInputsAndOutputs`）ので、出力 2 番は原作と同じ `UVs`（取り込みのログで確認）。以前は入力をすべて既定（2 × 2・位相 `Time`）にしていて、1 画面に 1 列 × 2.5 段を縦に縮めて映し、ミップの無い 1 px の水平線がちらついてノイズに見えた。2026-09-18 までは Opacity が未接続（1）で、A が 0 の横線が走査線のように画面を横切っていた（本家の実機の同じコマには出ない） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Textures/T_ky_slash01_4x4` | テレポートの斬撃（**旧版**。2048 × 2048 の 4 × 4 コマ、R に三日月・G にその明るい縁・B に X 字〈本家の画面には出ない〉。sRGB なし・`TC_Default`〈DXT1〉・`TEXTUREGROUP_Effects`。原作の cook も DXT1・sRGB なし・12 ミップ） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2` | テレポートの照準の粒子（**旧版**。`dd_particles` が書き出しの値をそのまま写した Cascade。LOD 距離 0 / 2500 / 5000、固定のバウンズ ±150 × ±150 × ±50。エミッタ `cutter`〈斬撃。LOD0: 毎秒 10 と 50 cm 動くごとに 1、寿命 1、大きさ 300、寿命で 1 → 2 倍、色は粒子パラメータ `color`〈既定 (13, 0, 0.216663)〉・アルファ 1 → 0、コマ番号の 16 段の表、回転 0〜1 回転・毎秒 0.5〜1 回転、Z 軸に寝かせる、位置 Z 0〜5。`bUseLegacySpawningBehavior` 真〉とエミッタ `Particle Emitter`〈火花。毎秒 30、寿命 2、大きさ 5〜15 / 25、速さ (±5, ±5, 300〜500)、色 (5, 0, 0) → (1, 0, 0)・アルファ 1 → 0、位置 ±170 × ±170〉。どちらも詳細度は Low〜Epic〈書き出しの 7 に Epic を足した 15〉。材質は下の 2 つ） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Materials/M_ky_slash01_4x4` | `M_DD_KySlash` のインスタンス。原作のパラメータ `alphaDensity` 1.5・`colorCorrect` 2.0・`depthFade` 100・`hilightColor` (3.9051919, 4.0955548, 5.0, 1.0)・`Texture` = `T_ky_slash01_4x4` |
| `/Game/DD/PyroParticlePack/Materials/PPP_Radial_Gradient_Doffed` | `M_DD_PPPRadialGradient` のインスタンス（原作にパラメータは無い） |
| `/Game/DD/Blueprints/Main/Powers/M_Decal_Teleport` | `M_DD_DecalTeleport` のインスタンス（原作にパラメータは無い） |
| `/Game/Pipeline/Materials/M_DD_KySlash` | **推定**（`M_ky_slash01_4x4`。cook に残るのは設定〈Translucent・Unlit・両面・スプライトとメッシュ粒子〉、`ParticleSubUV`〈`T_ky_slash01_4x4`・Linear Color〉、4 つのパラメータ、Emissive が `Lerp` から来ること、式が 11 個あったことだけ）。残りの 6 個を粒子の色・`Lerp`・`Power`・`Multiply` 2 つ・`DepthFade` と読んだ（本作のマスターも 11 個）。Emissive = 粒子の色を `TextureSampleParameterSubUV`（`Texture`）の G で `hilightColor` へ `Lerp` したもの。Opacity = `R^colorCorrect × alphaDensity × 粒子のアルファ` を `DepthFade`（`depthFade`）に通したもの（半透明の Opacity はエンジンが 0〜1 に切る）。2026-09-17（ステップ 11b4）に最新版の収録に合わせて組み直した: 前の推定（Emissive = 粒子の色 × R^colorCorrect、Opacity = `saturate(R × alphaDensity)`）は、粒子の原点が床の 13.75 cm 上にあるため `DepthFade` で半分ほどに薄まり、本家の太いサーモン色の帯と白い芯（斬撃の重なり）より細く暗かった。粒子の色 (13, 0, 0.22) をそのまま出すと、トーンマッパーでサーモン色になる |
| `/Game/Pipeline/Materials/M_DD_PPPRadialGradient` | **推定**（`PPP_Radial_Gradient_Doffed`。cook に残るのは設定〈Translucent・Unlit・Responsive AA・分離透過なし・スプライト / ビーム / 静的ライティング〉、Emissive = `ParticleColor` の RGB〈原作どおり〉、関数 `RadialGradient`・`CameraDepthFade`）。Opacity = `RadialGradient`（既定。硬さ 0 の `SphereMask`）× `CameraDepthFade`（既定。長さ 512・オフセット 24）× 粒子のアルファ。UE 4 の「分離透過なし」は UE 5 の `TranslucencyPass` = `MTP_BeforeDOF` |
| `/Game/Pipeline/Materials/M_DD_DecalTeleport` | **推定**（`M_Decal_Teleport`。cook に残るのは設定〈Deferred Decal・Translucent・DBM_Emissive〉、関数 `RadialGradientExponential`・`CheapContrast`・`LinearGradient`、Emissive が `Multiply` から来ることだけ）。Emissive = `Color` (1, 0, 0) × `Intensity` 1 × `saturate(CheapContrast(RadialGradientExponential〈既定: 中心 0.5・半径 0.5・密度 2.333〉, Contrast 5))` × `Lerp(PulseLow 0.2, PulseHigh 1.0, sin(2π 時刻) / 2 + 1/2)`。Emissive だけをつなぐので UE 5.8 では加算のデカール（`SourceAlpha, One`）。`LinearGradient` は使わない（下の「確かめたこと」）。**色と明るさは 2026-09-17（ステップ 11b4）に最新版の収録から決めた**: 本家の表示値（暗い側 (139, 81, 81)、明るい側 (238, 97, 90)）を、UE のフィルミックのトーンマッパー（既定の値。UE 4.24 と 5.8 で同じ式）を numpy で組んで線形の明るさに戻すと、明滅で増えるのはほぼ純粋な赤（G・B は R の 0.3 % 未満）で、床の上に足される赤は約 0.17〜0.22 ↔ 0.94〜1.05。丸めた `PulseLow` 0.2・`PulseHigh` 1.0 は仮の値（床の明るさが本家と違い、デカールの下の床が決まらないため幅がある。`dd_powers.DECAL_*`） |
| `/Game/DD/Audio/SharedGameplay/Telepathy` | Telepathy の開始の音（1.760 秒、44.1 kHz。SoundWave の値は既定のまま。両版で同じ ogg）。終わりの音は上の `Teleport_Mode_Entered`（両版で同じ ogg） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16` | Telepathy の印のノイズ A（1024²、R に煙状・G に縦の筋・B にまだら。sRGB なし・`TC_Default`〈DXT1〉・`TEXTUREGROUP_Effects`。原作の cook も同じ） |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise` | Telepathy の印のノイズ B（512²、どのチャンネルも雲状。sRGB・`TC_Default`〈DXT1〉・`TEXTUREGROUP_Effects`。原作の cook も同じ） |
| `/Game/DD/Blueprints/Main/Powers/Telepathy/MM_Telepathy` | `M_DD_Telepathy` のインスタンス。原作のパラメータ `Color` (1, 0, 0, 1)・`Tiling` 1・`Speed` 1 |
| `/Game/DD/Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst` | `MM_Telepathy` のインスタンス（原作と同じ親子）。`Speed` 1。原作の `Size` 1（親に無いパラメータ）・`RefractionDepthBias` 0（UI では使われない）と、親と同じになる `BlendMode` Additive・`ShadingModel` Unlit の上書きは写していない |
| `/Game/Pipeline/Materials/M_DD_Telepathy` | **推定**（`MM_Telepathy`。cook に残るのは設定〈UI・Additive〉、Emissive = `Color` の RGB〈原作どおり〉、パラメータ `Tiling`・`Speed`、`Panner_0` を座標にした `T_ky_noise16` のサンプル〈Linear Color〉、`Panner_1` を座標にした `T_ky_noise` のサンプル〈Color〉、`RadialGradientExponential` の呼び出し、式が 21 個あったこと。`MaterialFunctionInfos` の `ExponentialDensity` は `RadialGradientExponential` の中の依存）。座標 = TexCoord 0（繰り返し 0.6）× `Tiling`、パンの時間 = `Time` × `Speed`、Opacity = `saturate((A の R + B の R) × RadialGradientExponential〈既定〉× Gain)`（`Gain` 0.4）。UI の加算は Emissive × saturate(Opacity) × ウィジェットの色と不透明度で、画面には sRGB にしてから足される（Substrate でも同じ。`SubstrateCreateUIMaterial`）。**`Gain` 0.4 と繰り返し 0.6 は 2026-09-17（ステップ 11b5）に最新版の収録から当てはめた値**: 本家の印が足す赤を不透明度に戻すと、中央値 約 0.02・最大 約 0.07 のなめらかな雲だった（前の `Gain` 3 は中がほぼ飽和した明るい丸）。候補の式を numpy で描き、大きさに依らない統計（しきい値ごとの面積比・足す赤の分布・縁の凹凸・細かさ）を本家と比べると、この式の `Gain` 0.38〜0.42・繰り返し 0.5〜0.7 がいちばん近かった（`observations/README.md` の「Telepathy の印の見直し」）。繰り返し 1 のままでは、本家より細かい筋と穴が出る。**グラフの形・この 2 つの値・パンの速さ（(0.05, −0.1)・(−0.04, −0.15)。本家の雲の変わる速さとは合うが、向きは分からない）は推定**（`dd_powers.TELEPATHY_*`） |
| `/Game/DD/Audio/SharedGameplay/Stun_Wave_Attack_New_04` | Primal Fear の音（1.710 秒、44.1 kHz。SoundWave の値は既定のまま） |
| `/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop` | Primal Fear のシェイク（`LegacyCameraShake`。振動 0.5 秒・ブレンドイン 0・アウト 0.5、位置 X 2/50・Y 2/35・Z 3/10〈始まりの位相は乱数、正弦波〉、回転と FOV なし。倍率 25 で鳴らす） |
| `/Game/DD/Textures/05_Circus/T_05_PortalMaps` | Primal の球（2048²、R に星状の粒・G に中心の丸い光・B に雲状のノイズ。sRGB なし・`TC_Default`〈DXT1〉・`TEXTUREGROUP_World`。原作の cook も DXT1・sRGB なし・12 ミップ） |
| `/Game/DD/Materials/05_Circus/M_05_Primal` | `M_DD_Primal` のインスタンス。原作のパラメータ `Color` (1, 0, 0, 1)・`Opacity` 1・`Desaturation` 0（書き出しに既定値が無い = UE の既定の 0） |
| `/Game/Pipeline/Materials/M_DD_Primal` | **推定**（`M_05_Primal`。cook に残るのは設定〈Translucent・両面・`bUsedWithStaticLighting`。シェーディングモデルは書き出しに無いので既定の DefaultLit〉、パラメータ 3 つ、`Panner_1` を座標にした `T_05_PortalMaps` のサンプル 1 つ〈Linear Color〉、Emissive が `Add_2` から来ることだけ。cook はどのマテリアルでも Opacity の入力を残さない）。式が 43 個あったこと）。推定は最新版の収録（ステップ 11b2、`observations/README.md`）に合わせた: 球の中から見ると、B の雲で濃淡のついた暗い赤の幕に、R の粒ほどの明るい欠片（横長でブロック状。エンジンの球の UV は横 360°・縦 180°で、数テクセルの粒が拡大されるため）が散り、球がレベルと交わる所が光る。座標 = TexCoord 0 × `Tiling` 2 を `Panner` (0.1, 0.1) へ、欠片 = `saturate(R × SparkleGain 10)`、雲 = `Lerp(CloudDark 0.01, CloudBright 0.1, B)`、縁 = `1 − DepthFade(EdgeDistance 50)`（Opacity の入力は既定の 1）。Emissive = `Desaturation(Color × 雲, Desaturation) + Color × (欠片 × SparkleBrightness 3 + 縁 × EdgeBrightness 6)`、Opacity = `saturate(CloudOpacity 0.95 + 欠片 + 縁) × Opacity`。**パンの速さと、決まらない値（`dd_powers.PRIMAL_KNOBS`。マスターのパラメータで、原作のインスタンスは設定しない）は仮の値**。パンの速さは、球が見える約 0.3 秒の間の動きがシェイクの揺れに埋もれて測れなかった |
| `/Game/DD/Particles/Shared/SmokeTest/T_LoopingSmoke_8x8` | Vanish の煙（4096²、8 × 8 コマの灰色の煙をアルファに。sRGB・`TC_Default`〈DXT5〉・`TEXTUREGROUP_World`。原作の cook も DXT5・sRGB・13 ミップ） |
| `/Game/DD/Textures/FX_Textures/T_perlinnoise` | `MM_WobblyVignette` のノイズ（2048²、低周波のパーリンノイズ、平均 0.465。sRGB なし・`TC_Grayscale`〈G8〉。原作の cook も G8・12 ミップ） |
| `/Game/DD/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff` | Vanish の煙の粒子（最新版。`dd_particles` が書き出しの値をそのまま写した Cascade。LOD 距離 0 / 2500 / 5000、固定のバウンズ ±約 4800。エミッタ 1〈既定の名前 `Particle Emitter`、3 LOD とも同じモジュール 14〉: 長さ 3 秒・1 回、出現の率 0 で 0 秒に 5 個のバースト、寿命 0.5〜1、大きさ 200〜250 × `SizeScale` (1.5, 1, 1)、速度 0、色 (3.6956, 1.4102, 10) → (1, 1, 1)・アルファ 1 → 0、`LocationWorldOffset` (0, 0, 150)、ワールドの抵抗 5、位置 ±50、回転 0〜1・回転速度 0〜0.1、8 × 8 の SubUV〈`Linear_Blend`、寿命で 0 → 30〉、ワールドの加速 (0, 0, 250)。詳細度は Low〜Epic〈7 に Epic を足した 15〉。材質は下の `M_LoopingSmoke1_Sheet`） |
| `/Game/DD/Particles/Shared/SmokeTest/M_LoopingSmoke1_Sheet` | `M_DD_LoopingSmoke` のインスタンス（原作にパラメータは無い） |
| `/Game/DD/Materials/Special/MM_WobblyVignette` | `M_DD_WobblyVignette` のインスタンス（原作にパラメータは無い） |
| `/Game/Pipeline/Materials/M_DD_LoopingSmoke` | **推定**（`M_LoopingSmoke1_Sheet`。cook に残るのは設定〈Translucent・分離透過なし・スプライト。シェーディングモデルは書き出しに無いので既定の DefaultLit〉、`ParticleSubUV`〈`T_LoopingSmoke_8x8`〉、関数 `CameraDepthFade`、式が 10 個あったこと。Emissive の入力は残っていない〈cook は接続があれば残す〉）。BaseColor = コマの RGB × 粒子の色、Opacity = コマのアルファ × 粒子のアルファ × `CameraDepthFade`（`Fade Length` = `FadeLength`、`Fade Offset` = `FadeOffset`）。UE は BaseColor を 0〜1 に切るので、粒子の色の 1 を超える部分（紫の 3.7 / 1.4 / 10）は白に近くなる。分離透過なしは UE 5 の `MTP_BeforeDOF`。ライティングを受ける半透明なので、見え方は場所の明るさで変わる。書き出しの 408 個の材質のうち Emissive が残るのは 296 個で、BaseColor と Opacity が残るものは 1 つも無い（2026-09-17 に数えた）。**`FadeLength` 64・`FadeOffset` 0 は仮の値**（`dd_powers.SMOKE_FADE_*`）: エンジンの既定（512・24）では 92 cm 先の煙が 1/8 ほどしか見えず、最新版の収録（ステップ 11b3）のもやは粒子のアルファどおりの濃さなので、薄めは約 2.3 m までに 1 になる、としか決まらない |
| `/Game/Pipeline/Materials/M_DD_WobblyVignette` | **推定**（`MM_WobblyVignette`。cook に残るのは設定〈UI・Translucent〉、Emissive = `TexCoord` の `T_VignetteNew` の RGB〈原作どおり。白〉、もう 1 つの `T_VignetteNew` のサンプル、`Panner_2` / `Panner_3` を座標にした `T_perlinnoise` のサンプル 2 つ〈LinearGrayscale〉、関数 `LinearSine`、式が 22 個あったこと）。Opacity = `saturate(T_VignetteNew の A × Lerp(A × (1 − s), B × s, s) × WobbleGain)`（A・B はウィジェットの UV〈繰り返し 1〉でパンした 2 つのノイズ、s = `LinearSine(Time, WobblePeriod)`。どちらのノイズも s の 2 乗で出入りするので、s が端に来るたびに片方の塊が膨らみ、s が半ばのときは縁に薄い輪だけが残る）。値は最新版の収録（ステップ 11b3、`observations/README.md` の「Vanish の見直し」）から当てはめた: パンの速さ (0.16, 0.006)・(0.095, −0.011)、`WobblePeriod` 10.4、`WobbleGain` 0.67（`dd_powers.WOBBLE_*`）。グラフの形は推定。ウィジェットの紫が白に掛かり、本家も本作も sRGB (142, 110, 194) へ混ざる |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_sphere` | テレキネシスの粒子 `sphere` のメッシュ（最新版。半径 10 cm の球、559 頂点。Nanite なし、ライトマップ 64・UV 0、スロット `WorldGridMaterial`〈エミッタが材質を上書きするので描かれない〉） |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_windLine27midPoly` | テレキネシスの粒子 `aura` のメッシュ（最新版。外接球 15.70 cm の渦巻く帯、294 頂点。設定は上と同じ） |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Textures/T_ky_maskRGB5`・`T_ky_shockWave02_4x4`・`T_ky_circle01_4x4`・`T_ky_maskRGB3`・`T_ky_dust_longStar`・`T_ky_wall02_4x4` | テレキネシスの粒子の材質のテクスチャ（最新版。どれも `TC_Default`〈DXT1〉。sRGB は `T_ky_maskRGB3`・`T_ky_dust_longStar`・`T_ky_wall02_4x4` だけ、グループは `T_ky_circle01_4x4` だけ World で残りは Effects。1024² / 2048² / 2048² / 1024² / 512² / 2048²。原作の cook と同じ）。中身: `T_ky_maskRGB5` は R 煙の筋・G 小さな欠片・B 横の筋、`T_ky_maskRGB3` は R まばらな引っかき・G 雲・B 泡、`T_ky_circle01_4x4` は R 縁がとげとげの輪（16 コマ）、`T_ky_shockWave02_4x4` は広がる輪（16 コマ）、`T_ky_dust_longStar` は中央の行が 1 で上下へ 0.37 まで落ちる横の光（灰色）、`T_ky_wall02_4x4` は灰色の電気の筋（16 コマ） |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Particles/P_ky_forceField_Telekinesis` | テレキネシスの力場の粒子（最新版。`dd_particles` が書き出しの値をそのまま写した Cascade。エミッタ 4〈`aura`・`ground`・`sphere`・`dustSq`〉・LOD 3・詳細度 15。`aura` と `sphere` はメッシュのエミッタで、3 つの LOD が 1 つの型データ〈`LODValidity` 7、`bOverrideMaterial`〉を共有する。値は調査 03 §2.6。材質は `aura` が `MI_ky_aura7c`、`ground` が `MI_ky_shockWave02_4x4_nonD`、`sphere` が `M_ky_wall02_4x4_two`、`dustSq` が `MI_ky_starDust_sq`） |
| `/Game/Wasami/Powers/P_WasamiForceField` | 本作の力場の粒子（`dd_powers.make_force_field`。上の写しで、`sphere` の `ParticleModuleLight` の `BrightnessOverLife` だけ 5.0 → 1.75（`FORCE_FIELD_LIGHT_SCALE` 0.35）。ほかは原作のまま。下の「既知の制約」）。C++ の `ForceFieldParticles` の既定 |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Materials/M_ky_wall02_4x4_two`・`M_ky_aura7`・`M_ky_shockWave02_4x4`・`M_ky_starDust` | 下の推定のマスターのインスタンス（原作のパラメータの既定値のうち推定にあるもの。`opacity` 0.1・`baseColor` (0.0606, 0.0692, 0.145)／`baseDensity` 0.3・`baseOpacity` 0.2・`hilightPower` 10・`hilightDensity` 2・`depthFade` 100・`maskU` 1・`maskV` 1・`maskRadiusControl` (0.5, 0, 0.5, 2)／`baseDensity` 1・`depthFade` 100・`coreDensity` 8・`coreHardness` 50・`hilightDetailPower` 50・`coreHilightPower` 10・`coreColor` (2, 0.1572, 0.119)／`maskRadius` 0.5・`maskDensity` 1・`fadeValue` 100・`starPower` 0〈既定なし〉。`M_ky_starDust` の `threshold` 400 は作らなかった WPO の側のもので写さない） |
| `/Game/DD/ThirdParty/AdvancedMagicFX09/Materials/MI_ky_aura7c`・`MI_ky_shockWave02_4x4_nonD`・`MI_ky_starDust_sq` | 原作のインスタンス（原作と同じ親子。`hilightDensity` 1.5・`hilightPower` 5・`maskU` 1・`maskV` 0.2・`maskRadiusControl` (0.5, 0, 0.125, 2)／`depthFade` 0・`hilightDetailPower` 10・`coreColor` (1.2186, 1.2017, 2.0)・`baseTex` `T_ky_circle01_4x4`／`maskDensity` 1・`maskRadius` 0.5・静的スイッチ `swSQdust` 偽。`RefractionDepthBias` と、親と同じになる `BasePropertyOverrides` は写さない） |
| `/Game/Pipeline/Materials/M_DD_KyWall02` | **原作のコンパイル済みシェーダーの式どおり**（2026-09-18、作業一覧の項目 23 のステップ 5e。`Tools/dd/cooked_shaders.py` で `M_ky_wall02_4x4_two` のスプライト〈05〉とメッシュの粒子〈15〉の半透明のベースパスのピクセルシェーダーを読んだ。両方同じ式）。設定は cook のまま（Translucent・Unlit・両面・スプライトとメッシュの粒子）。tex = `baseTex` の SubUV のサンプル（2 コマの混ぜ）として、Emissive = `Lerp(baseColor, 1, tex.RGB × (1 + 粒子の色))`（= Screen。**クランプしない**ので、粒子の色が HDR〈B 20〜50〉の間は筋が 1 を大きく超える）、Opacity = `DepthFade((tex.R + opacity) × 粒子の α)`（距離は式の既定の 100。saturate はエンジンのもの）。Lerp の B と最初の Add の B はつながず式の定数 1 にして、式の数が cook の 10 と一致する。前の推定（`Lerp(baseColor, 粒子の色, R)`、`saturate(R + opacity) × α`、深度のぼかし無し）より筋が白っぽい（幕だけの収録で τ0.7〜0.9 の R が 10〜20 段階上）。**本家の終わりの白飛びした横長の破片は、式どおりの幕でも出なかった**（幕のテクスチャは滑らかな網目で、その形にならない。候補はオーラの `T_ky_maskRGB5` の G。`observations/README.md`「幕を原作のシェーダーどおりにしたが、白飛びした破片は出なかった」）。**球が縮む間の画面を横切る速さは合わせる対象から外した**（球の中心はカプセルの中心の 180 cm 上、カメラはその 85 cm 下で、半径 2600 → 132 cm の間ずっと球の中にいる。見かけの流れはこの幾何と、球の回転〈Z 軸 ±0.05 回転/s の発動ごとの乱数〉と 0.5 秒のカメラの揺れで決まる。`observations/README.md`「幕の流れは合わせる対象から外した」） |
| `/Game/Pipeline/Materials/M_DD_KyAura7` | **原作のコンパイル済みシェーダーの式どおり**（2026-09-18、作業一覧の項目 23 のステップ 5f。`Tools/dd/cooked_shaders.py` で `M_ky_aura7` のメッシュの粒子〈25〉の半透明のベースパスのピクセルシェーダーを読んだ。`MI_ky_aura7c` は自分のシェーダーマップを持たない）。設定は cook のまま（Translucent・Unlit・両面・スプライト・ビーム・メッシュの粒子）。メッシュの帯は U が幅（0・0.5・1）、V が長さ。`T_ky_maskRGB5`（Linear Color。R もや・G ぎざぎざの欠片・B 筋）の 8 つのサンプルのうち 4 つは別のパンのサンプルの B で座標を曲げる: 欠片 = G(パン (−0.1, 0.2) + 5 B(パン (−0.2, −0.3))²) × G(TexCoord を中心の周りに 0.1 rad/s で回した `Rotator` + 5 B(パン (0.2, 0.5))²)^`hilightDensity` × `hilightPower` × 150、もや = (R(TexCoord × 0.5 をパン (0.6, 0.5) + 0.6 B(パン (−0.1, −0.2))) × R(パン (−0.5, −0.3) + 0.2 B(パン (0.04, 0.1))))^`baseDensity` × 2 + `baseOpacity`。Emissive = (欠片 + もや) × 粒子の色（**クランプしない**。欠片は `MI_ky_aura7c` の 1.5・5 で最大 750、粒子の色 (0, 0.44, 2.44) で白飛びする）、Opacity = (欠片 + もや) × 粒子の α × **メッシュの頂点カラーの R**（帯の中央の線が 1、縁が 0）× マスクを `depthFade` で薄める（saturate はエンジンのもの）。マスク = `RadialGradientExponential`（UV = TexCoord × (`maskU`, `maskV`)、中心 = (`maskRadiusControl` の R, `maskOffsetY`)、半径 B、密度 A）で、帯の中央の線を残し、粒子が `maskOffsetY` を 0.1 → 0.2 と動かすと線の V = 0 の側から消える。コードでは曲げる B のうちもやの 2 つは max(B, 0)（テクスチャの B は負にならないので B のまま）。`Param2〜4` は使わない。式は 65 個（cook は 61。冪の指数と倍率を定数の式にした分）。**本家の終わりの白飛びした横長の破片はこの欠片**（前の推定は R だけを読み、欠片が無かった。`observations/README.md`「オーラを原作のシェーダーどおりにし…」） |
| `/Game/Pipeline/Materials/M_DD_KyShockWave02` | **原作のコンパイル済みシェーダーの式どおり**（2026-09-18、作業一覧の項目 23 のステップ 5g。`Tools/dd/cooked_shaders.py` で `M_ky_shockWave02_4x4` のスプライト〈5〉の半透明のベースパスのピクセルシェーダーを読んだ。メッシュの粒子〈15〉ほかも同じ式。`MI_ky_shockWave02_4x4_nonD` は自分のシェーダーマップを持たない）。設定は cook のまま（Translucent・Unlit・両面・スプライトとメッシュの粒子）。shape = `baseTex`（Linear Color。インスタンスは `T_ky_circle01_4x4`）の SubUV のサンプル → 静的マスク `selectCh`（既定 R。インスタンスは上書きしない）。`T_ky_maskRGB3`（Color。R まばらな引っかき・B 泡）の 2 つのサンプルは TexCoord × 4 を座標に、**R をパン (0.3, 1.0)、B をパン (−0.2, −0.2)** で読む。火花 = (引っかき × 泡 × `hilightDetailPower`) × ((引っかき + 泡) × `hilightDetailPower`) × `coreHilightPower` × saturate(shape^`coreDensity` × `coreHardness`)。Emissive = 粒子の色 × shape + `coreColor` × 火花（**クランプしない**。輪は粒子の色 (0, 0.28, 5) の青で、輪のいちばん明るい線の上で引っかきと泡が重なる所だけ `MI_ky_shockWave02_4x4_nonD` の値で最大 2000 × `coreColor` (1.22, 1.20, 2.0) の白飛びした火花になる）、Opacity = shape^`baseDensity` × 粒子の α を `depthFade` で薄める（インスタンスの 0 は UE が小さな値に直すので薄めない = 名前の `nonD`。原作も `Max(depthFade, 1e-5)`。saturate はエンジンのもの）。式は cook と同じ 31 個。前の推定（両方のサンプルの R、仮のパンの速さ `SHOCKWAVE_PANS`、輪を `coreColor` で塗る）は消した（`observations/README.md`「地面の輪を原作のシェーダーどおりにした」） |
| `/Game/Pipeline/Materials/M_DD_KyStarDust` | **原作のコンパイル済みシェーダーの式どおり**（2026-09-18、作業一覧の項目 23 のステップ 5d3。`Tools/dd/cooked_shaders.py` で `M_ky_starDust`〈`swSQdust` 真〉と、自分のシェーダーマップを持つ `MI_ky_starDust_sq`〈偽〉の半透明のベースパスのピクセルシェーダーを読んだ。01 記録の「cook のシェーダーを読む」）。設定は cook のまま（Translucent・Unlit・Responsive AA・スプライトとメッシュの粒子）、Emissive = 粒子の色の RGB。twinkle = (`Sine`(Time × `flashTime`) + 1) × 0.5 × `flashPower`（`Sine` は周期 1）として、**真の側** = `Blend_Screen`(サンプルの R, TexCoord を 1/4 回転した座標のサンプルの R)^`starDensity` × `flashPower` × twinkle、**偽の側**（この力場が使う方。名前の `_sq` = square）= `DiamondGradient`(`Falloff` = `starDensity`) × (twinkle + `starPower`)で**テクスチャを読まない**。どちらも × 粒子の α（`dustSq` は 2）× `RadialGradientExponential`(`maskRadius`, `maskDensity`) を `fadeValue` で薄めたものが Opacity で、クランプはエンジンの saturate だけ。`Rotator` の回転は定数（シェーダーの cos 0.000796・sin 1 = 既定の速さ 0.25 × 時刻 6.28。定数は cook で消えていたので 6.28 を置く）。`useDistanceSize`（WPO）はつながない。**力場の星屑は、粒ごとに `flashTime`（1〜8）回/秒で明滅する小さな水色のひし形**（スプライトの ln(k) / `starDensity` の幅、k = 1.26 × `flashPower` × twinkle）で、**粒子の色 (0, 0.3225, 1) より明るくならない**（Emissive がその色で Opacity は saturate）。**本家の連写の終わりに見える白飛びした四芒星は星屑ではない**（5d1・5d2 はそう読んで偽の側を四芒星にしていたが、コードで否定された。白飛びできるのは Emissive が HDR の粒子で、幕 `sphere` の粒子の色は (0, 2.94, 20)〜(0, 6.46, 44)。幕の式を読むのは次のステップ）。PIE の `dustSq` だけの収録 `pie-dust-cook025` で、芯 (0, 145〜163, 209〜218) の小さなひし形が τ0.3〜1.5 に出ることを確かめた（`observations/README.md`「星屑は原作のシェーダーどおりのひし形だった」） |
| `/Game/Pipeline/Materials/M_DD_ChameleonCameraShake` | **推定**。Chameleon の `M_CameraShake`（Post Process）。書き出しに残るのはパラメータ `ShakePower`（既定 0.01）・`ShakeFQ`（既定 50）と `MakeFloat2` 1 つ・`MF_SetBlending`・`MF_DepthOnlyMasking` だけで、HLSL・数式・シーンテクスチャは無い。本作は `ScreenPosition.ViewportUV + Append(sin(Time × ShakeFQ), cos(Time × ShakeFQ)) × ShakePower` で `PostProcessInput0` を読み、その色を Emissive に出す（UE の Sine / Cosine は周期 1 = `ShakeFQ` 回/秒の円）。ブレンドの位置は既定（トーンマップの後）。**実機と見比べていない**（作業一覧の項目 2 で本家のブーストを撮るときに合わせる） |

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
- テレポートの見た目（旧版）: `pak_reference/_assets/DDeception/Content/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2.json`（粒子。01 記録の `dd_particles`）、`…/Materials/M_ky_slash01_4x4.json`・`_assets/DDeception/Content/PyroParticlePack/Materials/PPP_Radial_Gradient_Doffed.json`・`Blueprints/Main/Powers/M_Decal_Teleport.json`（材質の設定と残った式）、`_textures.json`（`T_ky_slash01_4x4`）、`BP_Power_Teleport.json` の `ParticleSystem_GEN_VARIABLE`・`Decal_GEN_VARIABLE`。まとめは `.claude/references/powers/02-teleport.md` §2・§5.3・§5.4。推定の材質の形は旧版の実機の収録から決めた（下の「確かめたこと」）。関数の中身は UE 5.8 のもの（`RadialGradient` は原作と同じ StateId）。
- Primal Fear: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_PrimalPower.txt`（`ReceiveBeginPlay` @915〜@1478、更新 @1511〜@1938、終わり @15）、`_assets/…/Powers/BP_PrimalPower.json`（コンポーネントの値、`CurveFloat_0〜3`、`Timeline_0_Template`）、`BP_DD_PlayerCharacter.txt`（使った瞬間 @18337〜@19621、再使用 @5187〜@6084、充填 @6301〜@6444、`Reset Primal` @37261）、`_assets/…/Materials/05_Circus/M_05_Primal.json`・`M_05_WarpTest.json`（シェーディングモデルが書き出されることの比較）、`_textures.json`（`T_05_PortalMaps`）、`_assets/…/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.json`、`_assets/…/Audio/SharedGameplay/Stun_Wave_Attack_New_04.json`。まとめは `.claude/references/powers/04-primal-telepathy.md` §2。
- Vanish: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_VanishPower.txt`（`ReceiveBeginPlay` @471〜@705、敵のループ @30〜@466、更新 @738〜@940、終わり @15）、`_assets/…/Powers/BP_VanishPower.json`（コンポーネントの値、`CurveFloat_0〜3`、`Timeline_0_Template`）、`BP_DD_PlayerCharacter.txt`（使った瞬間 @20985〜@22969、終わり @6856〜@7725、Gate の初期化と Open → Enter @7784〜@7917、充填 @7927〜@8328、`Reset Vanish` @37737 → @27838 → @27826・@7927）、`_assets/…/UI/Main/Powers/UMG_Vanish.json`（木・色・拡縮・アニメのキー）と `_bytecode/…/UMG_Vanish.txt`（`Construct` → `PlayAnimation`）、`_assets/…/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff.json`、`_assets/…/Particles/Shared/SmokeTest/M_LoopingSmoke1_Sheet.json`（同じ材質を使う遊園地の粒子も色が 10 → 1）、`_assets/…/Materials/Special/MM_WobblyVignette.json`、`_assets/Engine/Content/Functions/Engine_MaterialFunctions02/Utility/LinearSine.json`、`_textures.json`（`T_LoopingSmoke_8x8`・`T_perlinnoise`）。まとめは `.claude/references/powers/03-telekinesis-vanish.md` §3。
- Telepathy: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`（使った瞬間 @16461〜@18164、終わり @4271〜@4971、充填 @3762〜@3880）、`_bytecode/…/Powers/Telepathy/BP_Telepathy.txt`（`ReceiveBeginPlay` @72 → @86 → @10、`Update Targets`、`Finish`）・`BP_TelepathyTracker.txt`（Gate の初期化 @85〈Start Closed〉、Enter @186、Open @245、Close @255、`ReceiveBeginPlay` @270、`Update`、`Remove` @416 → @15）・`UMG_TelepathyTracker.txt`（`Construct` @154〜@506、`Remove` @102、`Set Size` @10）、`_assets/…/Powers/Telepathy/BP_TelepathyTracker.json`（`Widget_GEN_VARIABLE`）・`UMG_TelepathyTracker.json`（木とアニメのキー・区間・再生範囲）・`MM_Telepathy.json`・`MM_Telepathy_Inst.json`、`_assets/…/Materials/Player/M_TelepathyRange.json`（`ExponentialDensity` が依存として載ることの比較）、`_textures.json`（`T_ky_noise16`・`T_ky_noise`）、両版の `Telepathy.ogg`・`Teleport_Mode_Entered.ogg`（md5 が一致）。まとめは `.claude/references/powers/04-primal-telepathy.md` §3。
- テレキネシスの力場: `pak_reference_2/_assets/DDeception/Content/ThirdParty/AdvancedMagicFX09/Particles/P_ky_forceField_Telekinesis.json`（粒子。01 記録の `dd_particles`）、`…/Materials/M_ky_wall02_4x4_two.json`・`M_ky_aura7.json`・`M_ky_shockWave02_4x4.json`・`M_ky_starDust.json`・`MI_ky_aura7c.json`・`MI_ky_shockWave02_4x4_nonD.json`・`MI_ky_starDust_sq.json`（材質の設定・残った式・インスタンスの値）、`_meshes.json` と `_meshes_gltf/ThirdParty/AdvancedMagicFX09/Meshes/*.gltf`（帯の UV）、`DDeception/Content/ThirdParty/AdvancedMagicFX09/Textures/*.png`（チャンネルの中身）。関数の入力（`RadialGradientExponential` の UVs・CenterPosition・Radius・Density、`DiamondGradient` の Falloff、`Blend_Screen` の Base・Blend）は UE 5.8 のもの。`DepthFade` の距離 0 の扱いは `Engine/Source/Runtime/Engine/Private/Materials/MaterialExpressions.cpp`（`UMaterialExpressionDepthFade::Compile` の `Max(FadeDistance, UE_DELTA)`）。まとめは `.claude/references/powers/03-telekinesis-vanish.md` §2.6。
- UE 5.8 の挙動（Telepathy）: `Engine/Source/Runtime/UMG/Private/Components/WidgetComponent.cpp`（既定の `DrawSize` 500・`WindowVisibility`、`BeginPlay` の `InitWidget`、画面空間はティックで層に載せる、古い資産の `WindowVisibility` を Visible に直す）、`UMG/Private/Slate/SWorldWidgetScreenLayer.cpp`（投影・枠の大きさ・ピボット・距離の Z 順）、`UMG/Private/Animation/WidgetAnimationState.cpp`（`Play` がその場で最初のフレームを評価する）、`MovieScene/Private/Evaluation/MovieScenePlaybackManager.cpp`（最後の評価は終わりの 1 ティック前）、`MovieScene/Public/MovieSceneSection.h`（区間の既定の終わり方 KeepState）と `Config/BaseEngine.ini`（`DefaultCompletionMode` は Level / Template Sequence だけ）、`UMG/Private/UserWidget.cpp`（`Initialize`・`CreateWidgetInstance`）、`SlateRHIRenderer/Private/SlateRHIRenderingPolicy.cpp`（UI の加算は `One, One`）、`Shaders/Private/SlateElementPixelShader.usf`・`Substrate/SubstrateLegacyConversion.ush`（加算の色 × 不透明度）、`Engine/Private/LevelActor.cpp`（ワールドのコンテキストが無いと `DestroyActor` が警告する）・`Engine/Public/Tests/AutomationCommon.h`（`FTestWorldWrapper`。エンジン自身の `TimerManagerTests.cpp` と同じ使い方）。
- UE 5.8 の挙動（Vanish）: `Engine/Source/Runtime/Engine/Classes/Engine/Scene.h`（`GrainIntensity_DEPRECATED` と `FilmGrainIntensity`）・`Private/Scene.cpp`（変換が無いこと）・`Private/SceneView.cpp`（ブレンドするのは `FilmGrainIntensity` だけ）、`Private/Particles/ParticleSystemComponent.cpp`（コンストラクタの `bAutoActivate`、起動と停止でティックを自分で切り替えること）、`Classes/Particles/ParticleEmitter.h` の `FParticleBurst`、`UMG/Public/Blueprint/UserWidget.h`（`AddToPlayerScreen`）。
- UE 5.8 の挙動（Primal）: `Engine/Source/Runtime/Engine/Private/Timeline.cpp`（`PlayFromStart` の更新、長さを超えたティックでの終わり）、`Private/Components/PostProcessComponent.cpp`（既定の `bUnbound`・`BlendRadius`・`Priority`）、`Private/Curves/RichCurve.cpp`（`AddKey` と `SetKeys` が Auto の接線を計算し直す）、`Private/KismetSystemLibrary.cpp`（`SphereOverlapActors`）、`PhysicsCore/Private/ChaosScene.cpp`（`AddActorsToScene_AssumesLocked` が既定で体をすぐに問い合わせの構造へ入れる。テストの一時的なワールドで重なり判定が効く理由）。
- UE 5.8 の挙動（テレポート）: `Engine/Source/Runtime/Engine/Private/GameFramework/SpringArmComponent.cpp`（ラグ、`GetSocketTransform` がソケット名を見ないこと、長さ 0 ではトレースしないこと）、`Private/Components/DecalComponent.cpp`（材質が無いデカールは既定のデカール材で描く）、`Renderer/Private/DecalRenderingCommon.cpp`（Emissive だけのデカールは `BO_Add, BF_SourceAlpha, BF_One` の加算）、`Private/Particles/ParticleSystemComponent.cpp`（`SetTemplate` は登録済みで `bAutoActivate` なら動き出す）、`Private/KismetSystemLibrary.cpp`・`KismetTraceUtils.cpp`（`LineTraceSingleForObjects` の中身）。

## 依存関係
- `AWasamiPlayerCharacter`（02 記録）: `bCanInteract`・`bCanUseTablet`・`bHasInput`・`IsTabletUp()`・`SetMoveSpeeds()`・`GetTabletScreen()`・コントローラのカメラマネージャ。プレイヤーがこのコンポーネントを作り、入力を結び、毎フレーム画面へ値を渡す。
- `UWasamiTabletWidget`（03 記録）: `BounceSocket`。
- `AWasamiShard`（06 記録）: テレキネシスの `Activate` の受け手（`IWasamiTelekinesisInterface`）。テスト `TelekinesisPull` も置く。
- `WasamiAssets.h`（00 記録）。
- エンジン: `FTimerManager`、`UGameplayStatics::PlaySound2D` / `PlaySoundAtLocation`、`APlayerCameraManager::StartCameraShake` / `AddNewCameraModifier` / `AddCachedPPBlend`、`UCameraModifier`、`UPostProcessComponent`、`UMG`（`UUserWidget`・`UWidgetTree`・`UCanvasPanel`・`UImage`）。
- 取り込み: `WasamiDDTools.import_dd_powers()`（01 記録の `dd_powers.py` と `dd_assets.camera_anim` / `texture` / `material`）。

## テスト（`Tests/WasamiPowerTests.cpp`）
`Automation RunTests Wasami`（21 件。うち `Wasami.Cascade.Build`・`Cascade.MeshEmitter` は 01 記録、`Wasami.Tablet.CountShake` は 03 記録、`Wasami.Shard.PullCurve`・`Shard.Actor` は 06 記録）。
- `Wasami.Powers.Gauge` … FlipFlop の交互の向き、途中の値（2 秒で 1 秒後 0.5 など）、端で止まる、`Stop` で 1、テレポートの向きの決まり方。
- `Wasami.Powers.Tuning` … Lv5 の値、段階の丸め、Lv0 のテレキネシス半径、Lv1 のブーストの再使用 9.5。
- `Wasami.Powers.SocketBounce` … 弾みのキーの値と、キーの間の値（0.1 秒で 1.19028）。
- `Wasami.Powers.TeleportDistance` … Lv5 の最初の距離 1000（強化なしなら 700）、`Alpha` 0 / 1 の端、1 目盛りで +0.1（Lv5 で +125 cm）、1 フレームに 2 目盛り、0 と 1 での切り詰め。
- `Wasami.Powers.PrimalTimeline` … `BP_PrimalPower` の 4 本のトラックの値（0〜2 秒の 9 点。書き出しの接線で計算した値と 1e-5 以内）、重みの式（位置 0 で 1.000698 と 1、0.2 秒で 0.8675 と 0.5585、0.3 秒で閃光 0、0.5 秒で色 0）、クラスの既定（範囲なし、重み 0、各上書きと値、球の当たりなし、`Range` 1500）。
- `Wasami.Powers.PrimalStun` … 一時的なゲームのワールドに仮の的を並べ、`StunEnemies(原点, 3500)` が近く・端（3450）・真上 30 m の 3 体にだけ `SetState(Stun, false)` を 1 回ずつ送ること。遠く（3600）・体が Pawn でない的・`Enemy` タグだけで実装の無いアクタには送らないこと。
- `Wasami.Powers.TelekinesisTimeline` … `BP_TelekinesisPower` の `float2`（Primal と同じキー。0〜2 秒の 7 点）と重み、クラスの既定（範囲なし・重み 0、青のゲイン (0, 0.421727, 1.61)、中間調 100、色収差 50、ガンマの上書き、`Range` 1500、音とシェイクと粒子のパス、`LoadAssets` が 3 つとも読めること〈取り込みの後〉、0.2 秒と拡縮 2）。
- `Wasami.Powers.TelekinesisPull` … `FTestWorldWrapper` のワールドにシャードを並べ、`PullShards(原点, 3000)` が近く（10 m）・端（29 m）・真上 25 m の 3 つだけを引き寄せ始めること。遠く（32 m）・カプセルの当たりを切ったシャード・範囲内の仮の的（Pawn の体、テレキネシスのインターフェースなし）には何もしないこと（`SetState`・`PlayerVanish` も来ない）。
- `Wasami.Powers.VanishTimeline` … `BP_VanishPower` の `float2`（0〜2 秒の 9 点。書き出しの接線で計算した値と 1e-5 以内）と重み（0 秒で色 0.997・閃光 0.991、0.12 秒で閃光 0、0.3 秒以降で色 0）、クラスの既定（範囲なし・重み 0、紫のゲイン、中間調 100、色収差の上書きが 0、フィルムグレインの上書きなし、粒子の位置・自動起動・ティックの開始なし、粒子の素材の指定）。
- `Wasami.Powers.VanishNotify` … 一時的なゲームのワールドで、タグ `Enemy` と実装のある的には距離によらず `PlayerVanish` が 1 回ずつ届き、タグの無い的と、タグだけで実装の無いアクタには届かないこと。`SetState` は送らないこと。
- `Wasami.Powers.VanishWidget` … アニメ `Vanish` の不透明度を 1/15 倍速で読んだ値（0 / 0.375 / 0.75 / 1.125 / 1.5 / 7.5 / 13.5 / 14.25 / 14.625 / 15 / 20 秒で 0 / 0.156 / 0.499 / 0.843 / 1 / 1.0123 / 1 / 0.499 / 0.156 / 0 / 0）と、クラスの `Speed` 1。
- `Wasami.Powers.TelepathyTracker` … 距離から印の大きさ（0 / 50 / 100 / 125 m で 0.5 / 0.3 / 0.1 / 0、さらに遠いと負）、Appear と Disappear の拡縮と不透明度（書き出しの接線で計算した値と 1e-5 以内。Appear は 0.25 秒で拡縮 1・0.375 秒で 1.031・0.5 秒以降 0.95、Disappear は 0.15 秒で 1.1・0.3 秒で 0）、トラッカーのウィジェットコンポーネントの既定（画面空間・クラス・500 × 500・ピボット 0.5・希望の大きさで描かない）、ティックすること、Telepathy のクラスの `Time` 0。
- `Wasami.Powers.TelepathyTargets` … UE の `FTestWorldWrapper`（ゲームインスタンスとワールドのコンテキストを持ち、プレイを始めて手でティックするワールド）で、`BeginPlay` が近く・遠く（2 km）の 2 体に印を出し（敵の位置に、追従を始めて、ウィジェット付きで）、`NoTelepathy` の的とタグだけのアクタには出さないこと。直後の `UpdateTargets` は 0。印が動いた敵へ移り、プレイヤーがいないので箱の拡縮が 0.5 になること。後から出た敵は 0.7 秒では見つからず、0.8 秒の検索で見つかること。敵が消えた印は次のティックで追従をやめ、0.5 秒後に消えること。9 秒の直前まで Telepathy が残り、9 秒で消えて、残りの印が追従をやめて Disappear を始め、0.5 秒後に消えること。2 回目の使用（1 秒）で同じ敵に再び印が付き、終わりから 0.5 秒で全部消えること。
- `Wasami.CameraAnim.Playback`（`Tests/WasamiCameraAnimTests.cpp`）… ブーストの再生（0.25 秒で 0.5、0.5 秒で 1、9.25 秒でブレンドアウトが始まり 9.5 秒で 0.5、9.75 秒で 0、その次で終わり）、即座の停止、ブレンドの無い 0.5 秒のアニメが長さで終わること、ブレンドイン中の停止が小さい方の重みで続くこと。
- `Wasami.CameraAnim.Tracks` … ブーストの 1 キーの色が保たれ上書きフラグを触らないこと、`CameraAnim_Teleport`（旧版）のキーと接線で、書き出しの 60 fps の標本（`CameraAnim_Teleport.csv`）と同じ値になること（0.1 秒の露出 1.149884・色調 (1.528122, 0.532324, 0.471878)、0.05 秒、8/60 秒の露出の山 67.69149）。FOV のトラックは PP を変えない。
- `Wasami.CameraAnim.FieldOfView` … `CameraAnim_Teleport` の FOV のキーで、モディファイアの再生が書き出しの 60 fps の標本と同じ変化を足すこと（1/60 秒で 92.706、0.05 秒に 100° の視点で 119.80、0.13 秒で 150）、`BaseFOV` を基準にしないこと、0.5 秒の後は視点を変えないこと、5〜170° の切り詰めと重みの掛け方。

## 確かめたこと（2026-09-16、PIE、`L_Hospital_Zone1` の開始地点、ユーザーの了承のうえで `Tools/desktop.py` から入力）
- 始めは左右とも Speed Boost、6 種とも使える、ゲージはすべて 1。
- E: 歩き・ダッシュ・`MaxWalkSpeed` が 950 になり、`IsUsingPower(SpeedBoost)` が真、使えない状態、約 2 秒後のゲージが 0.796（1 − 2 / 9.75 = 0.795）。使ってから約 18 秒後（効果 9.75 秒と再使用 7.5 秒を過ぎた時点）に読むと、300 / 600・使える・ゲージ 1 に戻っていた。
- Q（左 = Speed Boost）で同じように効き、タブレットの左の枠のアイコンが扇形に灰色へ変わっていくのを撮った。
- タブレットを上げて 1 で左が Teleport に、2 で右が Teleport → Telepathy → … → Vanish → Speed Boost と巡回した。右の枠を 6 種すべてに切り替えて撮り、どのアイコンも出た。
- タブレットを下ろすと 1 / 2 は効かない。Q で Teleport を使っても使える状態のまま（このときはテレポートの中身が未実装。4a で足した）。
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

### テレポートのカメラアニメ（2026-09-16）
- **旧版の実機で観察**（Deadly Decadence の入口の噴水、テレポート 2 回を `Tools/desktop.py record` で 60 fps 収録。`observations/classic/`）: クリックの直後に画面が広がり、1 フレームだけ全面が (234, 245, 244) の白になり、赤く暗いフレームを経て戻る。閃光の後（アニメの 0.24〜0.40 秒）のフレームはアニメの後に対して拡大率 1.26 → 1.30 → 1.14 → 1.06（画角 ≈ 103° → 93°）、アニメの後は 0.98 のまま跳ばない。→ FOV の基準は開始時のキー（90）。`BaseFOV` 137.24 が基準なら 43〜51° に狭まり、終わりで跳ぶはず。
- 取り込み: `import_dd_powers()` が `camera_anims 2` を作り、読み戻した値は上の表のとおり。テスト 7 件が成功した。
- PIE（`L_Hospital_Zone1`、プレイヤーを (−25, 3000) で南向きにして、ユーザーの了承のうえで Q と左クリックを送った。毎フレームの視点の FOV をエディタの Python で記録）: クリックのフレームで 91.1、0.117 秒で 149.9、0.127 秒で移動、その後 81.7 → 102.0 → 90 と、キーの曲線に移動のシェイク `BP_CameraShake_Streak` の FOV の振動（振幅 2・0.5 秒）が重なった値になった。カメラコンポーネントの FOV は 90 のまま（足し算はモディファイアだけ）。0.62 秒で 90 に戻った。
- 収録（60 fps）: 広がり → 白 (252, 252, 251) → 赤 (88, 6, 0) → 戻り、が出た。**最初の回は白の直後に真っ黒なフレームが 1 枚出た**（UE 5.8 のプリ露出が +100 EV を 1〜2 フレーム遅れて使い、シーンカラーが溢れる。00 記録）。`r.EyeAdaptation.PreExposureOverride=1` を入れると、白が出た回でも黒は出ず、白 → 赤 (61, 5, 0) → 戻り になった。閃光は 9 ms ほどしか続かないので、フレームの刻みによっては白が出ない回がある（PIE の 6 回のうち 2 回で出た。旧版の 2 回はどちらも出た）。
- PIE の間、`LogWasamiCameraAnim` の警告は無かった。PIE は止めた。

### テレポートの見た目（2026-09-17）
- **旧版の実機で観察**（Deadly Decadence の噴水の前の芝、照準を最短 250 cm にし、動かないカメラで 60 fps・3 秒、取り消した後の背景を 1.5 秒。`observations/classic/aim-top-a*.mkv`。フレームごとの最小値で動く斬撃と火花を消して背景と比べた）: (1) デカールは **1 秒周期で明滅する**（内側の表示値 R が約 50 ↔ 177。なめらかで、山が広く谷が鋭い）。(2) **縁の鋭い円**で、内側はほぼ一様、縁の幅は半径の 1 割ほど。(3) 加算で光る（芝の模様が残る）。(4) 光る部分に出る直線の縁は床の起伏（箱は上下 ±10 cm。向きを変えると縁の向きが変わり、砂利道には映らない）で、材質の `LinearGradient` の効きは見えない。(5) 斬撃は暗い部分でも背景を暗くする。(6) Manor はポストプロセスの色の補正が強く（`ColorGain` (1.43, 0, 0.56)・LUT 0.9・ブルーム 2.5）、表示値から色と明るさは戻せない。
- **PIE で比べた**（`L_Hospital_Zone1`、プレイヤーを (15, 385, 90.15)・ヨー −90・ピッチ −33.3 に置き、ユーザーの了承のうえで Space・1・Space・Q を送り、距離は `AdjustDistance(−1)` × 8 で 250 cm に。同じ手順で収録。`observations/ours/pie-aim-a*.mkv`・`teleport-aim-classic-vs-pie.png`）: 円の半径を 1 とした正規化で、斬撃の光の外端は旧版 1.97〜2.04・PIE 1.97〜2.06、明るい帯は旧版 1.36〜1.63・PIE 1.31〜1.69（同じ粒子系なので、デカールの大きさが合っている）。縁は PIE でも ρ 0.94〜1.10 で落ちる（旧版 0.9〜1.1）。明滅は 1 秒周期で、山が広く谷が鋭い形も同じ。表示値の R は背景 62 に対して 90 ↔ 211（色と明るさは仮の値）。**火花は PIE のほうが大きく多く見える**（旧版は Manor のポストプロセスで暗い部分が削られている可能性があり、材質の違いと切り分けられない）。
- 照準からの移動（左クリック）は見た目を足した後も働いた（(15, 385) → (15, −615)、照準は消えた）。PIE の間、この仕組みの警告は無かった。PIE は止めた。

### 照準とテレキネシスの星屑の見直し（2026-09-17、ステップ 11b4、PIE、`L_Hospital_Zone1`。キーは `Tools/desktop.py`、値はリモート実行。画質は本家の収録と同じ「高」の 3 つの cvar。詳細は `observations/README.md` の「照準とテレキネシスの粒の見直し」）
- 照準（Lv3、待合 (15, 385)・ヨー −90・ピッチ −26.7、512.5 cm 先）: `HighResShot 3440x1440` で床の市松の十字と輪の大きさが本家の `orig-aim-a-full.png` と重なった。直した材質で、輪は本家と同じ太いサーモン色の帯と白い芯になった。デカールの箱の中央値は暗い側 (143, 85, 86) ↔ 明るい側 (238, 102, 95)、周期 1.00 s（本家 (139, 81, 81) ↔ (238, 97, 90)、約 1.0 s）。
- テレキネシス（Lv4、開始地点、速さ 0.25）: 星屑は本家と同じ小さな水色の四角になり、大きな十字は出なくなった。
- 材質 3 つはエラーなくコンパイルされた（`M_DD_KySlash` の式は 11 個、`M_DD_KyStarDust` は 23 個）。インスタンスの `depthFade` は 100 のまま。PIE を止め、画質の cvar と `t.MaxFPS` を戻し、未保存なし。

### Primal Fear（2026-09-17、PIE、`L_Hospital_Zone1`、Space・1・Q とビューポートのクリックは `Tools/desktop.py` から送り、値はエディタの Python で毎フレーム読んだ）
- 取り込み: `import_dd_powers()` が `sounds 6・camera_shakes 2・camera_anims 2・textures 4・materials 10・particle_systems 1` を作った（`Failed to compile` なし）。読み戻し: 音 1.710 秒、シェイクの位置の振動 X 2/50・Y 2/35・Z 3/10（始まりの位相は乱数、正弦波）・長さ 0.5・ブレンドアウト 0.5・回転と FOV 0、テクスチャ sRGB なし・`TC_Default`・2048、マスターは Translucent・両面・DefaultLit・静的ライティング用・ノード 14、インスタンスの親とパラメータ（`Opacity` 1・`Desaturation` 0・`Color` (1, 0, 0, 1)）。
- プレイヤーを (−25, 3000)・南向きに置き、仮の的を 800 cm 先・3350 cm 先・3700 cm 先・真上 30 m に出して Q: 800・3350・真上の 3 体だけ `SetState(Stun, false)` を受け、3700 の的は受けなかった。2 回目の使用で 3 体とも 2 回になった。
- 毎フレームの記録（約 98 fps）: アクタはプレイヤーの位置 (−25, 3000, 90.1) にあり、`Range` 3500、材質は `M_05_Primal` の MID。スポーンしたフレームにそのままティックも来るので、最初に描かれるのは位置 0.0102（本家もコンポーネントのティックは同じ仕組み）。球の半径は位置 0.49 で 2116 cm、0.97 で 3180 cm、1.44 以降 3442 cm（= 3500 × 0.98349）。重みは 0.2485 で色 0.788・閃光 0.292、0.368 で閃光 0、0.49 で色 0.045 → 0.5 で 0。`Desaturation`・`Opacity` もトラックどおり。アクタは最初の行から 1.993 秒後（位置が 2 を超えたティック）に消えた。
- ゲージは使ったフレームから 0.05 秒で 0、0.0625 秒で使用中が外れ、その後 23 秒で 0 → 1（11.558 秒で 0.4998）、23.065 秒で使える状態に戻った。`ResetPowers` の後も使える・ゲージ 1。
- 画面（gdigrab で 60 fps 収録。`ddagrab` が止まったため。検証のガイド）: Q の次のフレームで画面がオレンジ〜黄に飛び（閃光 × 赤い単色）、赤い雲状の球が重なり、約 0.28 秒で閃光が消えて赤い単色になり、0.48 秒で元の色に戻った。その後は廊下の奥（壁に隠れない所）に広がる球の赤い雲が見えた。このときの球の材質は最初の推定で、11b2 で最新版の収録と見比べて組み直した（上の表の `M_DD_Primal`、下の「既知の制約」）。
- シェイク: 最初は画面の位置が揺れなかった。取り込みが新しく作ったブループリントのクラスの既定値を、コンパイルの**後**に書いていたため、インスタンスに値が届いていなかった（01 記録の `camera_shake`）。直した後、カメラの位置は Q の直後から最大 68 cm 揺れ、0.48 秒で収まった（倍率 25 の 50 / 50 / 75 cm がブレンドアウトで減る）。
- PIE の間、この仕組みの警告やエラーは無かった（VSM の「非 Nanite マーキング ジョブ キュー オーバーフロー」は前からのもの）。音はユーザーのスピーカーで確かめていない。PIE は止めた。

### Vanish（2026-09-17、PIE、`L_Hospital_Zone1`、Space・1・Q とビューポートのクリックは `Tools/desktop.py` から送り、値はエディタの Python で毎フレーム読んだ）
- 取り込み: `import_dd_powers()` が `sounds 6・camera_shakes 2・camera_anims 2・textures 6・materials 14・particle_systems 2` を作った（`Failed to compile` なし）。`PPP_VanishPuff` はエミッタ 1・LOD 3・モジュール 14 で、`LODValidity`（すべて 7）と並びが書き出しと一致し、バーストは `((Count=5))`、材質は `M_LoopingSmoke1_Sheet`。推定の 2 つのマスターは、煙が DefaultLit・Translucent・`MTP_BeforeDOF`・スプライト用で BaseColor と Opacity がつながり（Emissive なし）、ビネットが UI・Translucent で Emissive がビネットの RGB、Opacity が `Saturate` ← 強さ ← ビネットの A × `Lerp`（2 つのノイズ、`LinearSine` の `Value` に `Time`・`Period` に `WobblePeriod`）とつながっていることを読み戻した。
- プレイヤーを (−25, 3000)・南向きに置き、仮の的を 800 cm 先と約 2.3 km 先に出して、左の枠を Vanish にして Q: そのフレームで、カプセルの `Camera` 応答が Block → Ignore（プロファイルは `Pawn`）、`IsUsingPower(Vanish)` 真、使えない状態、両方の的の `PlayerVanishCount` が 1（距離によらない）、ウィジェット 1（`Speed` 15）、Vanish のアクタがプレイヤーの位置 (−25, 3000, 90.15)・ヨー −90 にあり、粒子（`PPP_VanishPuff`、起動中）が (−25, 2907.58, −62.0) = プレイヤーの前 92.4 cm・下 152.1 cm にあった。
- 毎フレームの記録（約 70〜100 fps）: 最初に描かれるのはタイムラインの位置 0.0125（重み 0.987 / 0.956）。閃光の重みは位置 0.107 で 0.036、次の記録（0.194）で 0。色の重みは 0.2 で 0.29、0.3 で 0。アクタは位置 1.9987 の次のフレーム（2 を超えたティック）で消えた。ウィジェットの不透明度は 0.371 秒で 0.164、0.753 秒で 0.512、1.501 秒で 1.00006、7.505 秒で 1.0123、13.876 秒で 0.834、14.251 秒で 0.488、14.996 秒で 0（曲線どおり）。ゲージは 15 秒で 1 → 0。15.005 秒でカプセルが Block に戻り使用中が外れ、ゲージが 0 → 1 を 15 秒、30.006 秒で使える状態になって同じフレームでウィジェットが外れた。
- 画面（gdigrab で 60 fps 収録）: Q の次のフレームで画面全体が白紫に飛び（閃光 × 紫の単色）、約 0.2 秒で紫の単色、約 0.23 秒で元の色に戻り、その後 1.5 秒かけて画面の縁に紫の揺らぐビネットが出た。**初回だけ**閃光の間に約 0.09 秒の引っかかりがあった（2 回目は無い。エディタが新しい材質のシェーダーを初めて使うときのコンパイルと見ている）。下を向いて使うと、0.3〜0.8 秒に画面全体へ灰紫のもや（煙）が掛かり、約 1 秒で晴れた。煙はカメラから約 1 m にあり、`CameraDepthFade`（既定の長さ 512 cm・オフセット 24 cm）で大きく透けるので、正面を向いているとほとんど見えない。本家との見比べは 11b3 で行った（下の「既知の制約」の Vanish の項）。
- 死亡のリセット（効果の 3.7 秒後に `ResetPowers`）: 使える状態・ゲージ 1・ウィジェット 0 になり、カプセルの Ignore と使用中は残った。その 3 秒後に使い直すと、的に 2 度目の `PlayerVanish`、新しいウィジェットとアクタが出た。終わりは最初の使用から 15.0 秒（使い直しの Delay は仕掛け直されない）で、そこから 15 秒後に使える状態になり、2 つ目のウィジェットが外れた。**使い直しの `SetDelay` が 1 回多くなるのでゲージの FlipFlop がずれ**、使い直しではアイコンが 0 → 1、再使用中は 1 → 0 と逆に動き、使える状態になったときアイコンは 0 だった（本家の `Stop Vanish Timeline` も FlipFlop を戻さず、終わりの Delay も仕掛け直さないので、本家どおり）。
- PIE の間、この仕組みの警告やエラーは無かった（VSM の「非 Nanite マーキング ジョブ キュー オーバーフロー」は前からのもの）。音はユーザーのスピーカーで確かめていない。PIE は止めた。PIE のビューポートの外周に、縮尺の違う絵が枠のように出るのは、Vanish の前（ステップ 6 の撮影）からあるもの。

### Telepathy（2026-09-17、PIE、`L_Hospital_Zone1`、Space・1・Q とビューポートのクリックは `Tools/desktop.py` から送り、値はエディタの Python で毎フレーム読んだ）
- ビルド: 1 回目は、ファイルが増えてユニティビルドのまとまり方が変わり、`WasamiVanishPower.cpp` と `WasamiPrimalPower.cpp`（`WaveVolume`・`WavePitch`・`FadeKeys`）、`WasamiVanishPower.cpp` とテストの的（`EnemyTag`）、`WasamiVanishWidget.cpp` と `WasamiSpeedBoostWidget.cpp`（`VignetteScale`）の無名名前空間の名前がぶつかり、新しいウィジェットのローカル変数 `bInitialized` が `UUserWidget` のメンバーを隠して（C4458）落ちた。名前を変えて通した（警告なし）。
- 取り込み: `import_dd_powers()` が `sounds 7・camera_shakes 2・camera_anims 2・textures 8・materials 17・particle_systems 2` を作った（`Failed to compile` なし）。読み戻し: `M_DD_Telepathy` は UI・Additive・ノード 17 で、Emissive ← `Color` の RGB、Opacity ← `Saturate` ← `Multiply`（`Gain`）。`MM_Telepathy` の親は `M_DD_Telepathy`（`Tiling` 1・`Speed` 1・`Color` (1, 0, 0, 1)）、`MM_Telepathy_Inst` の親は `MM_Telepathy`（`Speed` 1）。テクスチャは 1024・sRGB なし / 512・sRGB、どちらも `TC_Default`・`TEXTUREGROUP_Effects`。音は 1.760 秒。
- テスト 15 件が成功した。`TelepathyTargets` は最初、`UWorld::CreateWorld` だけのワールドで書いたが、ワールドのコンテキストが無く `DestroyActor` が警告し、タイマーも期待どおりに進まなかったので、UE の `FTestWorldWrapper` に書き直した（テストだけ Live Coding で入れ替えた）。
- プレイヤーを (−25, 3000)・南向きに置き、仮の的を 800 cm 先・30 m 先・後ろ 5 m・右前（`NoTelepathy`）に出し、左の枠を Telepathy にして Q。毎フレームの記録（約 100 fps）: 使ったフレームで Telepathy のアクタが 1 つ出て、正面・30 m 先・後ろの 3 体（と、1 回目に足した的）に印が付き、`NoTelepathy` の的には付かなかった。箱の拡縮は距離どおり（800 cm で 0.468、30 m で 0.38、5 m で 0.48、1123 cm で 0.4551）。印の Appear は曲線どおりで（0.23 秒で 0.97、0.30 秒で最大 1.055、0.5 秒以降 0.95・不透明度 1）、角度はそれぞれ乱数（131.4°・358.5°・8.9° など）。**カメラの後ろの印は画面の層で折りたたまれ、Slate がティックしないので Appear が 0 のまま進まない**（本家の UMG も Slate のティックでアニメを進めるので同じ。見える向きになったら始まる）。
- 使ってから 4.10 秒後に足した的には、4.80 秒の検索（0.8 秒ごと）で印が付いた。
- 9.00 秒を過ぎた最初のフレームで Telepathy のアクタが消え、同じフレームで使用中が外れた。印は Disappear（0.1 秒で最大 1.127、0.27 秒で 0.065・不透明度 0.017）を経て、9.50 秒を過ぎたフレームで全部消えた。15.51 秒で使える状態に戻った。ゲージは 2 秒で 0.777、4.5 秒で 0.498、8.9 秒で 0.010、再使用の 0.50 秒で 0.077、3.24 秒で 0.499、15.4 秒で 0.984（1 → 0 を 9 秒、0 → 1 を 6.5 秒）。
- 画面（gdigrab で 60 fps 収録。`observations/ours/pie-telepathy-*`）: 印は壁や的に隠れずに赤い丸として重なって出た。縁のぼけた赤い円で、中はほぼ飽和し、ノイズの模様は薄い（**このときの材質**。11b5 で最新版の収録に合わせて濃さを下げた。下の「Telepathy の印の見直し」）。正面 800 cm の印の直径は約 110〜120 px（ビューポート 884 × 596。DPI の拡大率は約 0.55 なので、500 × 0.468 × 0.95 × 0.55 ≈ 122 px と合う）。30 m 先の印は正面の印の後ろに重なって見えない（近いものが上）。出るときに膨らみ、終わりに一度膨らんでから 0.3 秒で消えた。
- PIE の間、この仕組みの警告やエラーは無かった（ログのエラーは計測のスクリプトの書き損じだけ）。音はユーザーのスピーカーで確かめていない。PIE は止めた。

### テレキネシス（2026-09-17、PIE、`L_Hospital_Zone1`、Space・1 × 4・Space・Q とビューポートのクリックは `Tools/desktop.py` から送り、値はエディタの Python で毎フレーム読んだ）
- ビルドは警告なし、テストは 20 件とも成功。取り込みは足していない（音とシェイクは Primal のもの）。
- 1 回目: プレイヤーを (0, 700)・南向きに置いて Q。アクタはプレイヤーの位置 (0, 700, 90.1) にあり `Range` 3000。半径の中の 8 個（703〜2801 cm。横の廊下のものを含む）が 0.627〜0.863 秒で回収され（シャードは `Alpha` が 1 になる 0.75 / 再生速度 秒でプレイヤーに届いて、触れて回収される）、3107 cm 先の 9 個目は残った。2 回目: (2, −10500) で 41 個が 0.595〜0.933 秒で回収された（壁越しにも寄る）。
- 毎フレームの記録（約 100 fps）: ゲージは使ったフレームから 0.052 秒で 0、0.064 秒で使用中が外れ、その後 8 秒で 0 → 1（4 秒で 0.4925）、8.069 秒で使える状態に戻った。重みは位置 0.0624 で色 0.990・閃光 0.966、0.2055 で 0.860・0.532、0.3144 で閃光 0、0.5144 で色 0。アクタは 1.99 秒後に消えた。カメラの位置は Q の直後から最大 45 cm 揺れ、0.5 秒で収まった。
- 音（`au.Debug.ListWaves`）: 使った直後に `Stun_Wave_Attack_New_04` が音量 1 で鳴っていた。回収の音は、8 個が約 0.24 秒の間に続いた後、8 つとも残って音量が新しい順に 0.47・0.24・0.13・0.06・0.03・0.01・0.01・0.00（06 記録の未解決の同時発音の差と同じ現象。本家の収録に音が無く、聞き比べていない。下の「既知の制約」）。
- 画面（gdigrab で 60 fps 収録）: Q の次のフレームで画面が明るい水色に飛び（閃光 × 青い単色）、約 0.3 秒で閃光が消えて青い単色になり、0.5 秒で元の色に戻った。その間に餅が廊下の奥から飛んでくる。**揺れの間、下げたタブレットの黒い裏面が視界を横切るフレームがある**（全面の黒が 1 枚、部分的な黒が数枚。下の「既知の制約」）。色と見え方は 11b1 で最新版の収録と見比べた（`observations/README.md`）。
- PIE の間、この仕組みの警告やエラーは無かった。音はユーザーのスピーカーで確かめていない。PIE は止めた。

### テレキネシスの力場の素材（2026-09-17、取り込みと読み戻し。PIE は次の節）
- `import_dd_powers()` が `sounds 7・camera_shakes 2・camera_anims 2・textures 14・meshes 2・materials 28・particle_systems 3` を作った（`Failed to compile` なし）。`import_dd_shards()` も前と同じ数（`flash_materials` 13）で通り、閃光の静的マスクは G / R のまま。
- 推定のマスターは、式が `M_DD_KyWall02` 8・`M_DD_KyAura7` 64（5f から 65）・`M_DD_KyShockWave02` 31・`M_DD_KyStarDust` 22 で、パラメータの名前は上の表のとおり。原作のパスのインスタンスと原作のインスタンスの値・親・テクスチャを読み戻して書き出しどおり、`MI_ky_starDust_sq` の `swSQdust` は偽（親は真のまま）。
- `P_ky_forceField_Telekinesis` はエミッタ 4・LOD 3・詳細度 15、型データは `aura`（`SM_ky_windLine27midPoly`）と `sphere`（`SM_ky_sphere`）で `LODValidity` 7、各エミッタの材質は上の表のとおり。
- 取り込みの後に `L_Hospital_Zone1` が未保存になった（`import_dd_shards` の餅のメッシュの取り込み直しの後。06 記録のシャードが参照する）。灯 783・シャード 337・選択なしを数えてから保存した。

### テレキネシスの力場（2026-09-17、PIE、`L_Hospital_Zone1`、Space・1 × 4・Space・Q とビューポートのクリックは `Tools/desktop.py` から送り、値はエディタの Python で毎フレーム読んだ。収録は gdigrab の 60 fps で `observations/ours/pie-telekinesis-forcefield-*`）
- ビルドは警告なし、テストは 21 件とも成功（`TelekinesisTimeline` を粒子のパスと `LoadAssets` の確認に変えた）。
- プレイヤーを (0, 700)・南向きに置いて Q（2 回）: 使ってから 0.208〜0.210 秒（タイムラインの位置 0.223〜0.225）のフレームで `P_ky_forceField_Telekinesis` のコンポーネントが 1 つ、プレイヤーの位置 (0, 700, 90.15)・拡縮 2・起動中で出て、2.214〜2.217 秒で消えた（アクタは 1.99 秒で先に消える）。1 回目はその間にシャード 8 個が回収され、閃光 `P_ky_flash3` が 8 つ出て約 1 秒ずつで消えた。
- 画面（プレイヤーの視点）: 閃光と青の色調の後、0.3 秒ごろから水色の 4 本の光（星屑）がプレイヤーの周りを回りながら散り、0.7〜1.2 秒に青い風の筋（オーラ）と床の紫白の稲妻のような輪（地面の輪）、暗い青の幕（球）が重なって、約 1.4 秒で星屑が消え、元の色に戻った。**球の粒子の灯（`ParticleModuleLight`）が 0.2〜0.7 秒ごろ廊下を明るい水色に照らす**（青の色調が消えた後も画面が白っぽい）。
- 外から（同じ粒子を 15 m 先に拡縮 2 で出し、`slomo 0.25`。`pie-telekinesis-forcefield-outside-slomo025.mkv`）: 出た直後から廊下全体が水色に照らされ、約 0.45 秒（実時間）で青い球の壁が出現点の周りに見え、約 0.55 秒で廊下の奥の小さな青い球まで縮み、約 0.9 秒で消えた（球が外から内へ縮む。調査 03 §2.6 の計算どおり）。
- **初回だけ**、新しい材質のシェーダーをその場でコンパイルして描画が約 0.1 秒ずつ 3 回止まった（画面の左上に「シェーダーをコンパイルしています」。2 回目は止まらない。9b の閃光と同じ）。
- PIE の間、ログに警告やエラーは無かった。このときは最初の推定の材質のまま（11b4 で星屑を直し、明るさの差は下の「既知の制約」に書いた）。PIE は止めた。

### Telepathy の印の見直し（2026-09-17、ステップ 11b5、PIE の別窓 2580 × 1080、`L_Hospital_Zone1` の開始地点。キーは `Tools/desktop.py`、値はリモート実行。詳細は `observations/README.md` の「Telepathy の印の見直し」）
- エンジンの `RadialGradientExponential`（既定の入力）を描画先に描いて読むと、`1 − exp(−(2.33 × (1 − 距離 / 0.5))²)`（半径の外は 0）だった。
- 模様を止めた印（`Tiling` 1・`Speed` 0・角度 0、箱 229.8 px）の足す赤は、numpy の予測と画素ごとに平均 1.2 段階で合った。
- 材質を作り直した後（`Gain` 0.4・繰り返し 0.6）、仮の的 5 体（9 m・右前 4.3 m・37 m・60 m の左右）の印を 12 秒撮った（`observations/ours/pie-telepathy-f.mkv`）。足す赤の中央値は 31〜36（本家 36〜39）、90 % 点は 52〜64（本家 58〜64）、しきい値 20 を超える面積の割合は 0.76〜0.78（本家 0.80〜0.81）。前の `Gain` 3 では、模様を止めた印の足す赤の平均が 93 で、中はほぼ飽和していた。
- `M_DD_Telepathy` はエラーなくコンパイルされた（ノード 17、TexCoord の繰り返し 0.6）。`MM_Telepathy_Inst` の `Gain` は 0.4。PIE を止め、`t.MaxFPS` と PIE の別窓の設定（1280 × 720・中央に置かない）を戻し、未保存なし。C++ は変えていない。

## 既知の制約・注意点
- **倍率 25 のシェイク（Primal Fear・テレキネシス）の間、下げたタブレットが視界を横切って黒いフレームが出る**（2026-09-17、テレキネシスの収録で見つけた）。シェイクは視点だけを最大 50 / 50 / 75 cm 動かし、カメラの子のタブレット（下げた状態で視点の前 35 cm・下 40 cm・右 22 cm。02 記録。本家の値で、本家も隠さない）は動かないため。仕組みも値も本家の写しなので直していない。**ステップ 11a・11b1（2026-09-17）の測定では、本家にも本作にも見えなかった**（`video_probe.py series --dark 8` の下半分の暗い画素の割合: 本家 primal-a 0.045 → 0.099・telekinesis-a 0.044 → 0.045、本作 primal-a 0.030 → 0.042・telekinesis-a / b 0.030 のまま。本作は 1 秒約 48 枚 × slomo 0.25 で撮った。`observations/README.md`）。10a で見えた黒は、その後の変更で出なくなったか、刻みの違いによる。見直しは要らないと決めた。
- **テレキネシスの力場の粒子の材質 4 つは原作のシェーダーの式どおり**（2026-09-17 のユーザーの回答で詰めた。作業一覧の項目 23。星屑 5d3・幕 5e・オーラ 5f・地面の輪 5g。白飛びした終わりの破片はオーラの欠片）。**地面の輪は本家と見比べていない**（本家の連写 3 件のどこにも写らず、プレイヤーの視点からは幕と光に隠れて見えない。式どおりにした後の収録で、形と画面の平均が前の推定と変わらないことだけを見た）。**幕が画面を横切る速さも合わせる対象から外した**（球の幾何と、発動ごとに変わる球の回転とカメラの揺れで決まり、材質では決まらない）。敵はテレキネシスの対象ではない（本家どおり）。
  - **原作の材質の式は cook のシェーダーから読める**（2026-09-18、5d3。01 記録の「cook のシェーダーを読む」）。力場の 4 つはそれで式どおりにした。Telepathy の印も同じ方法で置き換えられる見込み（進捗記録 20260918-power-look-tuning のステップ 6）。cb3 のどこがどのパラメータかは、同じ道具が uexp の式の木から印字する（5f で足した）。
  - **球の粒子の灯は本作だけ 0.35 倍に弱めてある**（原作から離れる本作の調整。2026-09-17 のユーザの指示。ステップ 11b6）。原作の `ParticleModuleLight`（`BrightnessOverLife` 5。単純な灯なので画質「高」でも出る）のままだと UE 5.8 では力場の間の画面が本家より明るく、同じ条件（Lv4・速さ 0.25・画質「高」、青の閃光から 1.6～2.2 秒）の画面全体の平均が本家 (97.0, 144.4, 190.1)、本作 (125.0, 175.2, 217.5) だった。原因は分かっていない（GI は無く反射は SSR なので Lumen の二次光ではない。UE 4.24 と 5.8 の単純な灯の扱いの違いは、UE 4.24 のソースが手元に無いので確かめていない）ので、仕組みを探らず見え方を合わせた。`/Game/DD` の再構築は原作の 5 のままで、弱めた写し `/Game/Wasami/Powers/P_WasamiForceField` を別に作ってそちらを出す（`dd_powers.FORCE_FIELD_LIGHT_SCALE`、01 記録）。
    - 0.35 の根拠: 灯はシーンの線形の明るさにそのまま乗るので、倍率 1.00・0.60・0.35 の 3 回の収録をトーンマッパーの逆で線形に戻すと、画面全体の平均は倍率の 1 次式に乗った（予測と実測の差は 1 段階以内）。0.35 で本作は (95.1, 148.7, 193.0) になり、本家との差は (−1.9, +4.3, +2.9)。二乗平均誤差の最小は 0.30～0.35 で平ら（2.5～2.9 段階。比べた窓の中のフレーム間のばらつき約 7 段階より小さい）なので、測った丸い値 0.35 を採った。残る差（R が不足し G・B が過剰）は色味の違いで、灯の `ColorScaleOverLife` は原作の (1.5, 1.2, 1.2) のままなので明るさ 1 つでは消せない。力場の推定の材質（オーラ・地面の輪・幕）を詰めるときに見直す。
    - ステップ 11b4 の見積もり「本家の灯は本作の 0.5～0.7 倍」は、灯を切った収録 (28～49, 56～67, 73～80) を土台に使っていた。この土台は暗すぎてトーンマッパーの逆が R で戻らず（暗部のトーンの足）、倍率を大きく見積もっていた。強さを変えた収録同士で解く方が確だ。
  - 本家の力場の終わりに見える、端のぎざぎざの明るい破片（横長の断片。τ0.6〜1.0 に白飛びする）は**オーラの `T_ky_maskRGB5` の G の欠片**だった（5f。星屑でも幕でもない）。式どおりのオーラで、本作も横の帯に並んだ角ばった青い断片を出し、形の比（短軸/長軸）は τ0.9〜1.0 で 0.24〜0.26（本家 0.28〜0.29）。**本作の収録では白飛びしない**（0 %。本家 2〜4 %）: 破片の幅は本家の 4〜5 px で、本作のビューポート（1152 px 幅）では 1.5 px ほどになり、TAA で芯が薄まる。ゲームを止めて 3440×1440 で撮ると芯は白く飛んでいた（1 枚だけ）。
- **Telepathy の印の材質 `M_DD_Telepathy` はグラフが推定**（ノイズのつなぎ方は推定、`Gain`・繰り返しは収録からの当てはめ、パンの速さと向きは仮の値〈2026-09-17 のユーザーの回答で、本家の観察を続けて詰める。作業一覧の項目 23〉）。2026-09-17（ステップ 11b5）に最新版の収録と見比べ、濃さと模様の細かさを合わせた（上の表）。本家の雲は、縁にこぶのある塊（丸より少しいびつ）で、本作は丸に近い。印は 2026-09-19 に PIE の Zone 2 の見張りのナース 6 体にも付くことを確かめた（07 記録の「パワーの作用を PIE で確かめたこと」）。
  - 印の画面上の大きさは、本家と同じ縦横比の別窓の PIE（2580 × 1080、UI の DPI の倍率 1.0）で、`500 × 箱の拡縮 × 0.95 × DPI` どおりだった。本家の印の大きさは、ナースの距離が分からないので直接は比べられない。雲の直径が箱の約 0.87 倍になる、という形では合う。
  - 表示の式（背景 + 255 × sRGB(不透明度)。UE 5.8 の `SlateElementPixelShader.usf`）は、模様を止めた印（`Tiling` 1・`Speed` 0・角度 0）の PIE の画面と numpy の予測が画素ごとに平均 1.2 段階で合うことで確かめた。UE 4.24 も同じ式と見ている（UE 4.24 のソースは手元に無い）。
- Telepathy の印の `Appear` が角度を触らないこと（2D 変換のトラックはデータのあるチャンネルだけを書く）と、アニメの最後の評価の時刻は UE 5.8 のソースに拠る。UE 4.24 のソースは手元に無い（本家も乱数の角度を入れているので、角度は残る前提）。
- ユニティビルドで無名名前空間の名前がぶつからないよう、定数や補助の名前はファイルごとに固有にする（ステップ 8 でファイルが増えてまとまり方が変わり、Vanish と Primal の定数がぶつかった）。
- **Vanish の煙の材質 `M_DD_LoopingSmoke` とビネットの材質 `M_DD_WobblyVignette` はグラフが推定**。見えない扱いは、2026-09-19 に PIE の敵ワサミ（Zone 2 の見張りのナース）でも確かめた（効果の間は見つけず、3 m まで近づいても追わない。07 記録の「パワーの作用を PIE で確かめたこと」）。
  - ビネットは 2026-09-17（ステップ 11b3）に最新版の収録と見比べて値を決めた。本家の塊の位置は、ノイズのテクスチャを繰り返し 1 で貼ったものと相関 0.84 で重なり、2 つのノイズの流れる速さと入れ替わりの周期もこの式で再現できた（当てはめの誤差 0.061。フレームごとに自由に合わせても 0.048）。塊の位置と明滅の山の時刻は、エディタの起動からの時刻（UI の材質の `Time`）で決まるので、毎回違う。
  - **煙の位置と明るさは本家と合っていない**（ステップ 11b3）。本家のもやはエレベーターの扉枠（234 cm 先）に隠され、画面の中央の約 40 % に明るい藤色（sRGB (153, 121, 204) へ約 0.6）で出る。本作の煙はコードどおり 92 cm 先・目の 97 cm 下に出て、画面全体を暗い紫に薄く覆う（中央で (+18, −7, +25)。本家は (+67, +43, +86)）。消えるまでの長さ（実時間で約 0.9 秒）は合う。粒子の値・スポーンの位置・部品の位置は原作どおりで、UE 5.8 の Cascade のコード（バーストの位置の補間、`bJustRegistered`、LOD）にも前へずらす仕組みは見つからなかった。粒子の値は原作のまま、材質のフェードだけを仮の値にして見えるようにした今の形で、2026-09-17 にユーザーが「これでよい」とした（煙を前へずらすことはしない）。
- **Primal Fear の球の材質 `M_DD_Primal` はグラフが推定**（パンの速さと `PRIMAL_KNOBS` は仮の値）。2026-09-17（ステップ 11b2）に最新版の収録と見比べて値を決めた。本家と同じ条件（Lv3 = 半径 2500、正面 1,036 cm を扉でふさぐ）では、画面全体の色の推移が本家と合い、球が扉を越えるときの菱形（エンジンの球の頂点が正面に来る形）も同じ形で出た。違いは、閃光の終わり際の明るさの落ち方（本家は約 0.07 秒かけて下がり、本作は 0.01 秒ほどで落ちる。ポストプロセスの値は原作どおりなので、UE4 と UE 5.8 の色の処理の違いと見ている）と、閃光の直後の黄みが本家より少し強いこと。本作の病院には廊下の奥の両開き扉がまだ無い（作業一覧の項目 8）ので、扉が入るまでは球が奥まで見え、Lv5（半径 3500）では +3.5 s まで赤い雲が残る。
- Primal の気絶は、2026-09-19 に PIE の敵ワサミ（Zone 2 の見張りのナース）でも確かめた（17 s 止まり、明けに見失う。07 記録の「パワーの作用を PIE で確かめたこと」）。
- **Vanish の効果中に死亡のリセットが来て、元の 15 秒が終わる前に使い直すと、ゲージの FlipFlop が 1 つずれ、以後アイコンの動きが逆になる**（使い直しの `SetDelay` だけが増え、終わりは 1 回のまま。本家の `BP_Powers` と Delay の作りどおり。ブーストはリセットがその場で終わりの処理を通すので、ずれない）。
- Vanish の煙は、2026-09-17（ステップ 11b3）までは `CameraDepthFade` の既定の値のため、正面を向いているとほとんど見えなかった（上の「確かめたこと」）。いまは仮の `FadeLength` 64 で見えるが、本家より暗く、手前に出る（上の Vanish の項）。本家の見張りナースのように、気絶の処理を後から動かす敵の扱いは敵の側で作る。
- **テレポートの照準の材質 3 つはグラフが推定**（原作のグラフは cook で消えている）。粒子の値そのものは原作の書き出しどおり（`P_ky_cutter2` は両版で同じ）。2026-09-17（ステップ 11b4）に最新版の病院と同じ条件（Lv3・512.5 cm 先・画質「高」）で見比べ、デカールの色と明るさ、斬撃のグラフを直した（上の表）。火花の材質は変えていない: 本家の収録は画質「高」（`r.EmitterSpawnRateScale` 0.5）で、火花のエミッタは数が半分になる（斬撃のエミッタは `bApplyGlobalSpawnRateScale` が偽で減らない）。同じ画質で撮ると、数（12〜14 個）・直径（5〜23 px）・色が本家と重なった。本作の既定は画質「最高」で、火花は本家の「最高」と同じく倍になる。
- 粒子の詳細度のビットは、UE 5.8 が古い資産を読むときと同じく Epic を足している（01 記録）。
- 本家では照準のアクタがクリックを受け、入力を消費しないので、プレイヤー自身の左クリック（調べる）も同時に走る。本作のプレイヤーにはまだ調べる処理が無い（02 記録）。
- ゲームパッドでの確定（最新版の `Gamepad_FaceButton_Bottom`）は旧版に無いので入れていない。
- 本家は `Check` を `Delay 0.2` の後に行うが、本作は `BeginPlay` ですぐ作る。
- 本家の `Power` 配列は解放フラグ（セーブ）から作るが、本作はすべて解放済み（`UnlockedPowers`）。選んだ枠をレベルをまたいで残す本家の仕組み（GameInstance）は、ステージが病院だけなので作っていない。
- 本家のパワーの放送（`UsedTeleportPower`・`UsedPrimal` など）を購読するのは本家のチュートリアルや台本のレベルだけで、病院には無い。本作は `OnPowerUsed` 1 つにまとめた。
- ゲームパッドの割り当て（LT / RT / LB / RB）はまだ入れていない（プレイヤーの入力がキーボードとマウスだけのため）。
- 音と揺れはユーザーのスピーカーと画面で確かめていない（PIE の確認は値と絵）。
- テレポートの閃光の白は、本作の病院では (252, 252, 251)、旧版の Manor では (234, 245, 244)。トーンマッパーの上限の色がステージのポストプロセス（色の補正）で違うためと見ている（病院の Zone 1 はボリュームが無い）。最新版の病院の絵とは比べていない。
- **推定のもの**: `M_DD_ChameleonCameraShake` の揺れ方（円・sin/cos）。本家の実機と見比べていない（作業一覧の項目 2 で本家のブーストを撮るときに合わせる）。
- `Duration` の扱い（ブレンドアウトを含む）は UE 5.8 の説明に拠る。UE4 の `CameraAnimInst.cpp` で確かめていない。違っていれば色調の消え方が 0.5 秒ずれるだけ。
- `UMG_SpeedBoost` の最初のフレームが不透明度 1 で出る（本家の Delay の順の写し）。1 フレームなので撮影では確かめていない。
- `T_Speedlines` は原作どおり非圧縮で約 81 MB あり、プレイヤーの `BeginPlay` から持ち続ける（本家もプレイヤーがクラスを参照しているので同じ）。
- カメラアニメの FOV の基準（開始時のキーの値）は実機の観察で決めた（UE4 の `CameraAnimInst.cpp` は手元に無い）。`CameraAnim_Teleport` は開始時のキーが 90 で UE4 のカメラの既定の FOV と同じなので、「開始時のキー」と「90」のどちらと読んでも同じ値になる。`bRelativeToInitialFOV` が偽のアニメ（本家の 2 つには無い）の扱いは作っていない。
- FX の `Custom Depth Highlighter (Clip)`（敵の縁取り）は作らない（2026-09-17 のユーザーの回答「不要」。上の「FX（`UWasamiChameleonComponent`）」）。

## 変更履歴
- 2026-09-18: `WasamiTelepathyTrackerWidget.cpp` の無名名前空間のキーの型を `FTrackerAnimKey` にした（ファイルが増えてユニティビルドの塊が変わり、`WasamiWidgetAnimation::FAnimKey` の `using` とぶつかった。作業一覧の項目 6 のステップ 1）
- 2026-09-18: `ResetPowers` の呼び元を死亡画面に書き直した（作業一覧の項目 5。09 記録）
- 2026-09-18: `M_DD_KyShockWave02`（力場の地面の輪）を原作のコンパイル済みシェーダーの式どおりに組み直した（`T_ky_maskRGB3` の R をパン (0.3, 1.0)・B をパン (−0.2, −0.2)、TexCoord × 4。Emissive = 粒子の色 × shape + `coreColor` × 火花、Opacity = shape^`baseDensity` × α。式は cook と同じ 31 個。仮の値 `SHOCKWAVE_PANS` を消した。`dd_powers`。C++ は変えていない。作業一覧の項目 23、ステップ 5g）
- 2026-09-18: `M_DD_KyAura7`（力場のオーラ）を原作のコンパイル済みシェーダーの式どおりに組み直した（`T_ky_maskRGB5` の G の欠片 × 150 と R のもや、頂点カラーの R を不透明度に掛ける。仮の値 `AURA_LAYERS` を消した。`dd_powers`。`Tools/dd/cooked_shaders.py` は cb3 の並びを印字するようにした〈01 記録〉。C++ は変えていない。作業一覧の項目 23、ステップ 5f）
- 2026-09-18: `M_DD_KyWall02`（力場の幕）を原作のコンパイル済みシェーダーの式どおりに組み直した（Emissive = `Lerp(baseColor, 1, tex.RGB × (1 + 粒子の色))`、Opacity = `DepthFade((tex.R + opacity) × α)`。式は cook と同じ 10 個。`dd_powers`、`dd_assets.depth_faded_opacity` は距離を省けるようにした〈01 記録〉。C++ は変えていない。作業一覧の項目 23、ステップ 5e）
- 2026-09-18: `M_Speedlines` の Opacity に `T_Speedlines` の A をつないだ（原作のコンパイル済みシェーダーのとおり。未接続のため、A が 0 の画面いっぱいの横線が走査線のように出ていた。ユーザーの指摘。`dd_powers`。C++ は変えていない）
- 2026-09-18: `M_DD_KyStarDust` を原作のコンパイル済みシェーダーの式どおりに組み直した（`Tools/dd/cooked_shaders.py` で読んだ。偽の側は `DiamondGradient` × (twinkle + `starPower`)、真の側は十字 × `flashPower` × twinkle、どちらも × α × `RadialGradientExponential`。5d2 の四芒星は取り消し。`dd_powers`。C++ は変えていない。作業一覧の項目 23、ステップ 5d3）
- 2026-09-18: `M_DD_KyStarDust` の `swSQdust` の真と偽を入れ替え、偽の側（力場が使う方）を四芒星にした。2 つのサンプルの R を `starDensity` 乗した線を `Blend_Screen` で重ね、`DiamondGradient`（`Falloff` = `starDensity`）で腕を切り、`RadialGradientExponential` で縁を落として `flashPower` を掛ける。真の側はその `DiamondGradient` × `flashPower` だけ。`RadialGradientExponential` は中心でも 0.632 なので冪の内側には置けない（`dd_powers`。C++ は変えていない。作業一覧の項目 23、ステップ 5d2）
- 2026-09-18: テレキネシスの力場の球の灯を本作だけ 0.35 倍に弱めた（`P_WasamiForceField` を新しく作り、`ForceFieldParticles` の既定をそちらに向けた。原作の再構築 `P_ky_forceField_Telekinesis` は 5.0 のまま。作業一覧の項目 23、ステップ 11b6、01 記録）
- 2026-09-18: テストの的（`Tests/WasamiTestEnemy.cpp`）のタグの定数を `TestEnemyTag` にした（敵ワサミ `WasamiEnemy.cpp` の `EnemyTag` とユニティビルドでぶつかった。07 記録）
- 2026-09-17: `M_Speedlines` の FlipBook を既定の 2 × 2・1 周/秒から、本家の実機に合わせた 2 × 5・3 周/秒（30 コマ/s）に直し、`T_Speedlines` の `NeverStream` を原作どおり真にした（`dd_powers`・`dd_assets.texture`。01 記録。ソースの C++ は変えていない）
- 2026-09-17: テレポートの照準とテレキネシスの星屑の推定の材質を最新版の収録に合わせた。`M_DD_DecalTeleport` の `Color`・`PulseLow`・`PulseHigh` を (1, 0, 0)・0.2・1.0 に、`M_DD_KySlash` を「粒子の色を G で `hilightColor` へ、不透明度は R^colorCorrect × alphaDensity × α を深度で薄める」に組み直し、`M_DD_KyStarDust` の `swSQdust` の真と偽を入れ替えて偽の側を `DiamondGradient`（`Falloff` = `starDensity`）× `flashPower` にした（`dd_powers`。C++ は変えていない）
- 2026-09-17: Vanish の推定の材質を最新版の収録に合わせた。`M_DD_WobblyVignette` を `Lerp(A × (1 − s), B × s, s)` に組み直して `WOBBLE_*` を当てはめた値にし、`M_DD_LoopingSmoke` の `CameraDepthFade` の入力をパラメータ `FadeLength`・`FadeOffset`（仮に 64・0）にした（`dd_powers`。ソースの C++ は変えていない）
- 2026-09-17: Primal Fear の球の推定の材質 `M_DD_Primal` を最新版の収録に合わせて組み直した（B の雲を暗い幕に、R を小さな明るい欠片に、`DepthFade` で交わる所の光を足し、決まらない値をマスターのパラメータ `PRIMAL_KNOBS` にした。`dd_powers`。ソースの C++ は変えていない）
- 2026-09-17: テレキネシスの力場の粒子の参照 `ForceFieldParticles` に `P_ky_forceField_Telekinesis` を入れ、`LoadAssets` が常に読むようにした。テスト `TelekinesisTimeline` を粒子のパスと `LoadAssets` の確認に変えた
- 2026-09-17: テレキネシスの力場の粒子 `P_ky_forceField_Telekinesis` と、その材質（推定のマスター 4 つ・原作のパスのインスタンス 4 つ・原作のインスタンス 3 つ）を取り込み対象に足した（`dd_powers`。01 記録。C++ は変えていない）
- 2026-09-17: テレキネシスの粒子のメッシュ 2 つとテクスチャ 6 枚を取り込み対象に足した（`dd_powers`。01 記録。ソースの C++ は変えていない）
- 2026-09-17: テレキネシス（`AWasamiTelekinesisPower`〈半径の中のシャードに `Activate` を 1 回ずつ、青い画面と閃光、音、シェイク〉、`UseTelekinesis`〈0.05 秒でアイコンが落ち、0.06 秒後に再使用 8 秒〉、粒子の枠〈空〉）とテスト `Wasami.Powers.TelekinesisTimeline`・`TelekinesisPull` を足した。粒子 `P_ky_forceField_Telekinesis` は次のステップ
- 2026-09-17: テレキネシスのインターフェースを実装するシャード（`AWasamiShard`、06 記録）ができたことを書き足した（ソースは変えていない）
- 2026-09-17: Telepathy（`AWasamiTelepathyPower`〈0.8 秒ごとにレベルの全敵に印、時間で全部外す〉、`AWasamiTelepathyTracker`〈画面空間のウィジェットで敵を追い、距離で大きさ〉、`UWasamiTelepathyTrackerWidget`〈赤い煙の円、Appear / Disappear〉、開始と終わりの音、シェイク、9 秒と再使用 6.5 秒）とテスト `Wasami.Powers.TelepathyTracker`・`TelepathyTargets` を足した。印の材質は推定。ユニティビルドでぶつかった Vanish の無名名前空間の名前を変えた
- 2026-09-17: Vanish（`AWasamiVanishPower`〈紫と白の一瞬の演出・煙 `PPP_VanishPuff`・音・全敵への `PlayerVanish`〉、`UWasamiVanishWidget`〈紫の揺らぐビネット、15 秒で出て消える〉、カプセルの `Camera` 応答の切り替え、15 秒と再使用 15 秒、リセット）とテスト `Wasami.Powers.VanishTimeline`・`VanishNotify`・`VanishWidget` を足した。煙とビネットの材質は推定
- 2026-09-17: 一瞬の演出の基底 `AWasamiPowerBurst`（全画面のポストプロセス 2 つと 2 秒のタイムライン）と Primal Fear（`AWasamiPrimalPower`。半径の中の敵に気絶を 1 回、赤い球、音、シェイク、再使用 23 秒）、仮の的 `AWasamiTestEnemy`、テスト `Wasami.Powers.PrimalTimeline`・`PrimalStun` を足した。球の材質は推定
- 2026-09-17: テレポートの照準の見た目を足した（デカールの材質 `M_Decal_Teleport`〈推定〉と、デカールの子のパーティクル `P_ky_cutter2`〈原作の書き出しを Cascade に写したもの〉。斬撃と火花の材質は推定）
- 2026-09-16: テレポートのクリックで `CameraAnim_Teleport`（旧版）を再生するようにし、カメラアニメの FOV のトラックの再生（開始時のキーからの変化を足す。基準は旧版の実機で決めた）とテスト `Wasami.CameraAnim.FieldOfView` を足した
- 2026-09-16: テレポーテーションの仕組みを足した（旧版。`AWasamiTeleportAim`、使った瞬間・照準・ホイール・クリック・0.12 秒後のスイープ移動・再使用 5 秒・同じ側の Q / E での取り消し・死亡のリセット、音 3 つ、テスト `Wasami.Powers.TeleportDistance`）。カメラアニメと見た目はまだ
- 2026-09-16: スピードブーストの演出を足した（`CameraAnim_SpeedBoost` の赤い色調、`UMG_SpeedBoost` の集中線とビネット、FX の画面の揺れ。放射ブラーは本家で無効なので作らない）。UE4 の CameraAnim の再生（`UWasamiCameraAnim`・`FWasamiCameraAnimPlayback`・`UWasamiCameraAnimModifier`）、FX（`UWasamiChameleonComponent`）、`UWasamiSpeedBoostWidget`、テスト `Wasami.CameraAnim` 2 件を足した
- 2026-09-16: 初版（パワーの土台: 枠・Q/E/1/2・2 段の連打防止・ゲージ・強化段階の表・死亡のリセット・スピードブースト、敵とシャードのインターフェース、テスト）
