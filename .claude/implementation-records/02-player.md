---
title: プレイヤーとゲームモード
sources:
  - Source/wasami_deception/WasamiGameMode.h
  - Source/wasami_deception/WasamiGameMode.cpp
  - Source/wasami_deception/WasamiPlayerCharacter.h
  - Source/wasami_deception/WasamiPlayerCharacter.cpp
updated: 2026-09-16
---

# プレイヤーとゲームモード

## 役割
本家 Dark Deception の `BP_DD_PlayerCharacter` に倣った一人称のプレイヤー。移動・視点・速さに連動する FOV・頭の揺れ・180° ターン・スピードブーストと、手に持つタブレット（板の出し入れ、画面、ミニマップのシーンキャプチャ）を持つ。`AWasamiGameMode` がそれを既定のポーンとして出し、タブレットの帯に出す目的の文を持つ。

## 公開インターフェース

- `AWasamiGameMode : AGameModeBase` — コンストラクタで `DefaultPawnClass = AWasamiPlayerCharacter::StaticClass()`。`Config/DefaultEngine.ini` の `GlobalDefaultGameMode` がこれを指す。`CurrentObjective`（FText、既定 `Collect all shards`）はタブレットの帯に出す目的（本家の `BP_DD_GameMode` の `Current Objective`）。
- `AWasamiPlayerCharacter : ACharacter`
  - `IsSprintOn()` / `IsBoosting()` / `GetBoostCharge()` / `GetBoostSocketPercent()` / `IsTabletUp()`（BlueprintPure）、`ToggleTablet()` / `ResizeMap()`（BlueprintCallable）。
  - 移動の値: `WalkingSpeed` 300、`SprintingSpeed` 600、`BoostSpeed` 870、`BoostDuration` 6.75、`BoostCooldown` 8.5（cm/s、秒）。
  - オプション: `bToggleSprint`、`MouseSensitivity` 1.0、`bInvertY`、`bHeadBob`（本家の OPTIONS の TOGGLE SPRINT / MOUSE SENSITIVITY / INVERTED Y AXIS / HEAD BOBBING）。
  - カメラ: `BaseFOV` 90、`FastFOV` 115、`FOVSpeedRange` (300, 900)、`FOVInterpSpeed` 0.5。
  - 頭の揺れ: `WalkShakeClass` / `RunShakeClass`（既定は `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake` と `_RunShake`）。
  - タブレット: `bCanMove`（本家の `CanMove?`。false の間は移動・視点・ダッシュ・タブレットが止まる）、`bCanUseTablet`（`Can Use Tablet?`）、`ShardActorClass`（画面が数えるシャードのクラス。未設定なら 0 を出す）。
  - コンポーネント: `Tablet`（板のスタティックメッシュ）、`TabletScreen`（`UWidgetComponent`、`UWasamiTabletWidget`）、`MinimapCapture`（`USceneCaptureComponent2D`）。

## 内部構造と処理の流れ

- **体とカメラ**（コンストラクタ）: カプセルは半径 50・半高 88（本家の `CollisionCylinder` は半径 50、半高は `ACharacter` の既定 88）、歩ける斜面は 44°（`WalkableSlope_Increase`）。`USpringArmComponent` をカプセルに付け、相対位置 (0, 0, 95)、`TargetArmLength` 0、当たり判定なし、`bUsePawnControlRotation` true、回転ラグ 20（`CameraRotationLagSpeed`）、位置ラグの速さ 8。`UCameraComponent` をアームのソケットに付け、FOV 90。目の高さは床から約 183 cm。
- **入力**（`CreateInput`、`SetupPlayerInputComponent`）: Enhanced Input の `UInputAction` と `UInputMappingContext` を実行時に作る（アセットにしない）。
  - 移動: W / ↑（Swizzle で Y に）、S / ↓（Swizzle + Negate）、D / →、A / ←（Negate）。`Move` は操作の向き（ヨーだけ）の前と右へ `AddMovementInput`。
  - 視点: `EKeys::Mouse2D` に Smooth → Scalar 0.07 → FOVScaling（`FOVScale` 0.01111、`UE4_BackCompat`）の順で修飾子を付ける（本家の `DefaultInput.ini` の `MouseX/Y` の感度 0.07、UE4 のマウススムージングと FOV スケーリングと同じ）。`Look` は `AddControllerYawInput` / `AddControllerPitchInput`（`bInvertY` でなければ Y を反転）。コントローラ側の 2.5 / −2.5 は `bEnableLegacyInputScales=True` により掛かる。
  - Shift（ダッシュ）、中クリック（180° ターン）、E（スピードブースト）。
- **速さ**（`ApplySpeed`）: ブースト中は `BoostSpeed`、それ以外はダッシュの有無で `SprintingSpeed` / `WalkingSpeed` を `MaxWalkSpeed` に入れる。加減速は UE の既定のまま（本家も `MaxWalkSpeed` しか上書きしていない）。
- **ダッシュ**: Shift の押下で入り、離すと戻る。`bToggleSprint` のときは押すたびに反転（本家の TOGGLE SPRINT）。向きは問わない。
- **FOV**（`UpdateFOV`）: `BeginPlay` で 0.001 秒のループタイマーを張り、毎回 `FInterpTo(現在, MapRangeClamped(速さ, 300→900, 90→115), フレームの delta, 0.5)`。本家の `FOV Multiplier` と同じ仕組み（タイマーが 1 フレームに何度も呼ばれるので、追従の速さはフレームレートで変わる）。
- **頭の揺れ**（`UpdateHeadBob` / `StopHeadBob`、本家の `Update Bob`）: ダッシュの状態が変わったら止める。速さが 1 cm/s 以下なら止める。動いていて未再生なら、`bHeadBob` のときにダッシュかどうかでシェイクを 1 つ再生する。止めるときはブレンドアウトさせる（`StopAllInstancesOfCameraShake(..., false)`）。
- **180° ターン**（`TurnAround`）: 押した瞬間に `SetControlRotation(0, ヨー + 180, 0)`（ピッチは水平に戻る）。回って見えるのはスプリングアームの回転ラグによる。
- **スピードブースト**（`UseBoost`）: クールダウン中は何もしない。効果 6.75 秒、クールダウンは発動と同時に始まり効果時間を含む（6.75 + 8.5 = 15.25 秒）。`Tick` で残り時間を減らし、切れたら速さを戻す。
- **タブレットを画面に留める**（`PlaceTablet`、`BeginPlay` の `SetTickGroup(TG_PostUpdateWork)`）: 毎フレーム、板の**ワールド変換を「これから描く視点」から置き直す**（`PlayerCameraManager` の `GetCameraLocation` / `GetCameraRotation` に、タイムラインが決めた視点空間の位置と回転を掛ける）。UE のカメラシェイクはカメラマネージャの視点にだけ掛かり、`UCameraComponent` は動かないので、板をカメラの子のままにすると歩くたびに**画面上でシェイクと逆に揺れる**。視点に直に置けば画面上で 1 px も動かない（ユーザーの指示。WebGL 版 10 記録の 2026-09-12 と同じ判断）。カメラマネージャが視点を決めた後に動かすため、`BeginPlay` でアクタのティックを `TG_PostUpdateWork` に移している。カメラマネージャが取れないときはカメラコンポーネントの変換を使う。
- **タブレットの構成**（コンストラクタ）: `Tablet` はカメラの子で、メッシュ `/Game/DD/Meshes/Player/Tablet/tablet_new_pCube2`、当たり無し、`bSelfShadowOnly`、初期位置は伏せた状態 (35.39891, −21.994417, −39.701378)・回転 (P0, Y90, R180)。その子の `TabletScreen` は相対位置 (0, 0.8922737, 0.2078171)・回転 (P0, Y90, R0)・スケール (0.28, 0.024465779, 0.024465779)、World 空間、DrawSize 714 × 864、Masked かつ片面（本家が上書きしている `Widget3DPassThrough_Masked_OneSided` が選ばれる）。`MinimapCapture` はカプセルの子で (0, 0, 3000)・ピッチ −90、正射影 `OrthoWidth` 4000、`SCS_BaseColor`、`PRM_UseShowOnlyList`、ターゲットは `/Game/DD/UI/Minimap/T_NewMap`。`bCaptureEveryFrame` は false から始め、タブレットを上げるときに true、下ろし終わったときに false にする（本家は常に撮っているが、下ろしている間は画面が見えないので絵は変わらない。`.claude/guides/performance.md`）。
- **出し入れ**（`ToggleTablet` / `UpdateTablet` / `ApplyTabletInterp` / `PlaceTablet`）: Space。`bCanMove` と `bCanUseTablet` が両方 true のときだけ効く。上げるときは `05_Tablet_Woosh_v2_1`、下げるときは `05_Tablet_Woosh_v1_1` を音量 0.5・ピッチ 1.5 で鳴らし、それぞれのカーブを頭から再生する（途中で押し直しても頭から。本家の `PlayFromStart` と同じ）。毎フレーム `TabletRaiseCurve`（0.5 秒。0 → 0.3 s で 0.9〈接線 2.0〉→ 0.5 s で 1.0）か `TabletLowerCurve`（0.3 秒。1 → 0.2 s で 0.1〈接線 −0.4674788 / −1.3626982 / −3.1061733〉→ 0.3 s で 0）を評価して `TabletInterp` に入れ、`PlaceTablet` が視点空間の位置の Z を −39.701378 ↔ −4.321648 に線形補間、回転を (P0,Y90,R180) と (P0,Y90,R0) の最短の Slerp にする。板は隠さない（伏せると視界の下に出るだけ）。
- **地図の拡縮**（`ResizeMap`）: Z。タブレットが上がっているときだけ効き、`UI_Select_V3` を音量 0.5・ピッチ 4.0 で鳴らして `OrthoWidth` を 4000 ↔ 10000 で切り替える。
- **画面の更新**: `UpdateTablet` が毎フレーム、左のパワー枠に 1.0（テレポーテーションは未実装）、右に `GetBoostSocketPercent()`（ブースト中は 1 → 0、その後のクールダウンで 0 → 1）、帯にゲームモードの `CurrentObjective` を渡す。`UpdateTabletScreen` は 0.1 秒ごとのタイマーで、`RefreshMinimapContents`（タグ `dd_minimap` のアクタと `ShardActorClass` のアクタを `ShowOnlyActors` に入れ直し、その数を数える）を呼んでからシャード数を渡す。

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.json`: `Walking Speed` 300、`Sprinting Speed` 600、`CollisionCylinder` の半径 50 と歩ける斜面 44°、`SpringArm_GEN_VARIABLE`（`TargetArmLength` 0、`bDoCollisionTest` false、`bUsePawnControlRotation` true、`bEnableCameraRotationLag` true、`CameraRotationLagSpeed` 20、`CameraLagSpeed` 8、相対位置 (0, 0, 95)）、`Tablet_GEN_VARIABLE`・`Widget_GEN_VARIABLE`・`SceneCaptureComponent2D_GEN_VARIABLE` の各設定。
- タブレットの出し入れ: 同 BP のバイトコード `InpActEvt_Toggle Tablet`（@19575。`CanMove?` と `Can Use Tablet?` の判定、woosh の音量 0.5・ピッチ 1.5、`TabletInterp` / `Timeline_1` の `PlayFromStart`）、その更新（@24367〜@25154 の `RLerp` と `VLerp`）、タイムラインのカーブ `CurveFloat_1`（長さ 0.5）と `CurveFloat_1_2`（長さ 0.3）。地図の拡縮は @18388（`UI_Select_V3` 0.5 / 4.0、`OrthoWidth` 10000 ↔ 4000）。キー割り当ては `pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` の `Toggle Tablet` = SpaceBar、`Resize Map` = Z。
- 頭の揺れ: `BP_DD_PlayerCharacter_WalkShake`（ピッチ 振幅 0.2・周波数 12、上下 振幅 2・周波数 13・オフセット 0、ブレンドイン 1・アウト 0.5）と `_RunShake`（ピッチ 0.5 / 18、ヨー 0.2 / 10、上下 4 / 20）。`/Game/DD` に同じ値の `LegacyCameraShake` として作ってある（01 記録）。
- ブーストの 6.75 秒 / 8.5 秒と 870 cm/s、FOV の 90→115（`MapRangeClamped(Speed, 300, 900, 90, 115)` を 0.001 秒のタイマーで `FInterpTo` 0.5）、180° ターン、マウスの感度 0.07 と FOV スケーリング: WebGL 版の実装記録 05（`.claude/references/webgl/implementation-records/05-player-controller.md`）にバイトコードの根拠がある。

## 依存関係
- `EnhancedInput`（`UInputAction`、`UInputMappingContext`、修飾子 `UInputModifierSwizzleAxis` / `Negate` / `Scalar` / `Smooth` / `FOVScaling`）。
- タブレットの画面は `UWasamiTabletWidget`（03 記録）、板・画面・地図の素材は `WasamiDDTools.import_dd_tablet` が作る `/Game/DD` のアセット（03 記録）、地図の板はレベルの組み立てが置く（01 記録）。`UMG`（`UWidgetComponent`）と `Engine`（`USceneCaptureComponent2D`、`FRichCurve`）。
- `UCameraShakeBase`（`/Game/DD` の `LegacyCameraShake` の Blueprint を `ConstructorHelpers::FClassFinder` で読む）。
- `Config/DefaultEngine.ini` の `GlobalDefaultGameMode` と `Config/DefaultInput.ini`（00 記録）。

## 既知の制約・注意点
- `ULegacyCameraShake` は `MinimalAPI` なので C++ で派生できない。本家のシェイクは Blueprint として作っている（01 記録の `WasamiDDTools`）。
- 入力の実際の手触り（ダッシュ時の FOV の広がり、頭の揺れ、180°、ブースト）はまだ確かめていない。エディタが背面にあるとティックが 3 fps ほどに落ち、リモートからの疑似操作では確かめられない（`.claude/guides/verification.md`）。
- 本家はタブレットをカメラ → `Scene`(0, 0, −94.9577) → `Tablet` と繋いでいるが、タイムラインが入れる相対 Z（−39.70 → −4.32）はカメラ基準の値で、`Scene` を挟むと板はカメラの約 1 m 下に行き画面に映らない。原作の収録から測った画面上の位置（x 4.8〜31.8 %・y 30.3〜91.0 %。WebGL 版 10 記録）は、カメラ相対 (35.399, −21.994, −4.322) に置いた計算（x 4.6〜31.5 %・y 30.0〜91.5 %）と合うので、`Scene` は置かずカメラの直下に付けている。
- 本家の画面はシャードの数を 0.01 秒ごとに数え直す（`UMG_Tablet` の Construct のループ）。本作は 0.1 秒ごと（数は回収でしか変わらないので見た目は変わらない）。
- `ShardActorClass` が空の間、シャード数は 0 のまま（シャードは M3）。左のパワー枠は常に `Percent` 1（テレポーテーションは未実装）。
- まだ無いもの: 視線の先の手のマーク（interact）、テレポーテーション、足音、しゃがみは作らない（本家に無い）。

## 変更履歴
- 2026-09-16: 初版（移動・視点・FOV・頭の揺れ・180°・ブーストを記録）
- 2026-09-16: タブレット（板・画面・ミニマップのシーンキャプチャ、Space の出し入れと Z の拡縮）と、ゲームモードの `CurrentObjective`、`bCanMove` による移動・視点・ダッシュの停止を足した
- 2026-09-16: ミニマップのシーンキャプチャを、タブレットを上げている間だけ撮るようにした（性能のルール）
- 2026-09-16: 歩くとタブレットが揺れる件を直した（ユーザーの指摘）。板をカメラの子として置くのをやめ、`TG_PostUpdateWork` で毎フレーム、カメラマネージャの視点に直接置くようにした（`PlaceTablet`）
