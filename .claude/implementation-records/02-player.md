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
本家 Dark Deception の `BP_DD_PlayerCharacter` に倣った一人称のプレイヤー。移動・視点・速さに連動する FOV・頭の揺れ・180° ターン・スピードブーストを持つ。`AWasamiGameMode` がそれを既定のポーンとして出す。

## 公開インターフェース

- `AWasamiGameMode : AGameModeBase` — コンストラクタで `DefaultPawnClass = AWasamiPlayerCharacter::StaticClass()`。`Config/DefaultEngine.ini` の `GlobalDefaultGameMode` がこれを指す。
- `AWasamiPlayerCharacter : ACharacter`
  - `IsSprintOn()` / `IsBoosting()` / `GetBoostCharge()`（BlueprintPure）。
  - 移動の値: `WalkingSpeed` 300、`SprintingSpeed` 600、`BoostSpeed` 870、`BoostDuration` 6.75、`BoostCooldown` 8.5（cm/s、秒）。
  - オプション: `bToggleSprint`、`MouseSensitivity` 1.0、`bInvertY`、`bHeadBob`（本家の OPTIONS の TOGGLE SPRINT / MOUSE SENSITIVITY / INVERTED Y AXIS / HEAD BOBBING）。
  - カメラ: `BaseFOV` 90、`FastFOV` 115、`FOVSpeedRange` (300, 900)、`FOVInterpSpeed` 0.5。
  - 頭の揺れ: `WalkShakeClass` / `RunShakeClass`（既定は `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake` と `_RunShake`）。

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

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.json`: `Walking Speed` 300、`Sprinting Speed` 600、`CollisionCylinder` の半径 50 と歩ける斜面 44°、`SpringArm_GEN_VARIABLE`（`TargetArmLength` 0、`bDoCollisionTest` false、`bUsePawnControlRotation` true、`bEnableCameraRotationLag` true、`CameraRotationLagSpeed` 20、`CameraLagSpeed` 8、相対位置 (0, 0, 95)）。
- 頭の揺れ: `BP_DD_PlayerCharacter_WalkShake`（ピッチ 振幅 0.2・周波数 12、上下 振幅 2・周波数 13・オフセット 0、ブレンドイン 1・アウト 0.5）と `_RunShake`（ピッチ 0.5 / 18、ヨー 0.2 / 10、上下 4 / 20）。`/Game/DD` に同じ値の `LegacyCameraShake` として作ってある（01 記録）。
- ブーストの 6.75 秒 / 8.5 秒と 870 cm/s、FOV の 90→115（`MapRangeClamped(Speed, 300, 900, 90, 115)` を 0.001 秒のタイマーで `FInterpTo` 0.5）、180° ターン、マウスの感度 0.07 と FOV スケーリング: WebGL 版の実装記録 05（`.claude/references/webgl/implementation-records/05-player-controller.md`）にバイトコードの根拠がある。

## 依存関係
- `EnhancedInput`（`UInputAction`、`UInputMappingContext`、修飾子 `UInputModifierSwizzleAxis` / `Negate` / `Scalar` / `Smooth` / `FOVScaling`）。
- `UCameraShakeBase`（`/Game/DD` の `LegacyCameraShake` の Blueprint を `ConstructorHelpers::FClassFinder` で読む）。
- `Config/DefaultEngine.ini` の `GlobalDefaultGameMode` と `Config/DefaultInput.ini`（00 記録）。

## 既知の制約・注意点
- `ULegacyCameraShake` は `MinimalAPI` なので C++ で派生できない。本家のシェイクは Blueprint として作っている（01 記録の `WasamiDDTools`）。
- 入力の実際の手触り（ダッシュ時の FOV の広がり、頭の揺れ、180°、ブースト）はまだ確かめていない。エディタが背面にあるとティックが 3 fps ほどに落ち、リモートからの疑似操作では確かめられない（`.claude/guides/verification.md`）。
- まだ無いもの: 視線の先の手のマーク（interact）、テレポーテーション、タブレット、足音、しゃがみは作らない（本家に無い）。

## 変更履歴
- 2026-09-16: 初版（移動・視点・FOV・頭の揺れ・180°・ブーストを記録）
