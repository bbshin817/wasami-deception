---
title: プレイヤーコントローラとテレポーテーション
sources:
  - src/player/controller.ts
  - src/player/teleport.ts
  - src/player/teleport-fx.ts
  - src/player/teleport-ring.ts
  - src/player/boost-fx.ts
  - tests/teleport-fx.test.ts
  - tests/boost-fx.test.ts
  - src/core/ue-curve.ts
  - tests/ue-curve.test.ts
  - src/player/walk.ts
  - tests/walk.test.ts
updated: 2026-09-15
---

# プレイヤーコントローラとテレポーテーション

## 役割
一人称プレイヤーの移動・視点・スプリント・スピードブースト・180° ターン・歩き・走りのカメラシェイク（原作の頭の揺れ）・足音タイミングを担う。
Havok の `PhysicsCharacterController`（カプセル）で衝突・接地を解決し、`UniversalCamera` を物理後に追従させる。移動の速さ・加減速と FOV は原作（pak_reference の `BP_DD_PlayerCharacter`、UE 4.21 の CharacterMovement の歩行）に合わせ、Babylon 非依存の計算を `walk.ts` に置く。
`teleport.ts` はパワー「Teleportation」（原作のタブレット左の枠）。原作の Blueprint（pak_reference の `BP_Power_Teleport` のバイトコード）どおりに作っている: Q で照準に入ると、カプセル中心から視線の水平の向きへ `aimDistance` m 先の点を真下へトレースし、歩ける床（原作の Teleport_Zone の代わりにナビ格子）なら照準の輪をそこへ動かす（輪は原作の SpringArm のラグで遅れて追う）。マウスホイールで距離、照準中の Q で取り消し、左クリックで確定。確定から 0.12 s 後に、輪の位置 + 1.25 m へカプセルをスイープで動かす（壁・閉じた門で止まる）。`teleport-fx.ts` は Babylon 非依存の部分: 原作のカメラアニメ `CameraAnim_Teleport`（FOV・露出・シーンのティント）とカメラシェイク `BP_CameraShake_Streak` の評価、距離の式、SpringArm のラグ。`teleport-ring.ts` は照準の輪で、原作と同じく床の発光（デカール `M_Decal_Teleport`）とパーティクル `P_ky_cutter2`（床に寝た斬撃のフリップブックと赤い火花）で作る。
ファンゲーム『Chaotic Customer 2』のプレイヤー（`ThirdPersonCharacter`）のしゃがみ・スライド（CTRL）は持たない（本作の規範: プレイヤーの能力は本家に基づき、本家のプレイヤーにしゃがみ・スライドは無い）。カプセルは常に立ちの 1.8 m。ファンゲームの駅でそれが要った 2 か所（出口に打ち付けた板と、低い戸口に掛かる格子の壁）は 1 クリックで壊れる（07 記録の breakables.ts）。

## 公開インターフェース
- `class PlayerController`
  - `constructor(scene: Scene, input: Input, start: Vector3, yaw = 0)` — `start` は足元位置。
  - `readonly camera: UniversalCamera`（名前 `'player'`）
  - 状態: `yaw`, `pitch`, `dash`（0..1 の平滑化スプリント強度）, `boostTime`, `boostCooldown`, `sprinting`, `speed`（水平速度 m/s）, `enabled`
  - コールバック: `onFootstep(pitch: number)`（原作の足音。`pitch` は MaxWalkSpeed が 6.5 m/s を超えていれば 1.5、でなければ 1）, `onBoost(ok: boolean)`, `onBoostReady()`
  - getter: `position`（カプセル中心）, `feet`（中心 − カプセルの高さ/2。高さは `CONFIG.player.capsuleHeight` 1.8 の定数 `capsuleHeight`）, `forward`（カメラ前方）, `fovDegrees`
  - `teleport(feet: Vector3, yaw: number)` — 位置・速度リセット、`pitch = 0`、`snapView()`（リトライとデバッグ API 用）
  - `snapView()` — カメラの向き（`viewYaw` / `viewPitch`）を視点（`yaw` / `pitch`）に即座に合わせる（カット。デバッグ API の `pose` も呼ぶ）
  - `readonly controls = { look: 1, invertY: false, headBob: true, toggleSprint: false, lagSpeed: 20 }` — プレイヤーの設定（game.ts の `applySettings` が settings.ts から入れる。既定は原作の既定値のとき）。`lagSpeed` だけは原作どおり、レベルの開始（`start` / `again`）で SpringArm の既定 20、ゲーム中のオプションの SAVE & EXIT で Mouse Smoothing の 12.5 / 50（`saveSettings`。04 記録）
  - `readonly bob: ShakeView`（walk.ts）— このフレームの歩き・走りのカメラシェイク（ピッチ上・ヨー右・ロール °、上下 m）。`syncCamera` がカメラに掛ける（タブレットはカメラの子なので一緒に動き、画面上では揺れない。10 記録）
  - `setFeet(feet: Vector3)` — カプセルの位置だけを動かす。向き・速度はそのまま（テレポーテーションで走り続けられる）
  - `update(dt, look: {dx, dy})` — 毎フレーム（物理前）に呼ぶ
  - `step(dt)` — `scene.onAfterPhysicsObservable` から呼ぶ物理ステップ
  - `syncCamera(dt)` — 物理後のカメラ配置
  - `dispose()`
- `class Teleport`（teleport.ts）
  - `constructor(scene, player: PlayerController, input: Input)` — 照準の輪（`TeleportRing`）を作る。
  - 状態: `aiming`（Q から移動・取り消しまで。確定から移動までの 0.12 s も含む）、`cooldown`（残り秒）、`target: Vector3 | null`（輪の床の位置。照準を始めてから床を捉えるまで null）、`fx: TeleportFx`（今のカメラアニメ）、`shake: ShakeOffsets`（今のシェイク、°）、`zone: (x, z) => boolean`（輪を置ける床の判定。game.ts がナビ格子の `walkable` を入れる。既定は常に true）
  - getter: `state: 'ready' | 'aiming' | 'cooldown'`、`charge`（0..1 の充填。1 = 使える）、`jumping`（カメラアニメかシェイクの途中か）、`aimDistance`（今の照準の距離 m）、`percent`（原作のタブレットの枠の `Percent`: 使えるとき 1、Q から `GREY_TIME` 0.05 s で 0、移動の後はクールダウンに合わせて 0 → 1、取り消しで 1）
  - コールバック: `onAim(aiming)`（照準の開始と終了。終了は移動・取り消し・ポーズ）、`onArrive(from, to)`（移動した瞬間。足位置）、`onCancel()`（照準中の Q）、`onDenied()`（クールダウン中の Q）、`onReady()`（充填完了）
  - `update(dt, active)` — 毎フレーム（描画前）。入力と照準は `active`（プレイ中かつポーズでない）のときだけ。確定後の移動とカメラアニメは常に進む。
  - `cancel(refill = false)` — 照準をやめる（跳ばない）。`refill` は照準中の Q（`onCancel` を呼ぶ）。
  - `applyToCamera(camera: TargetCamera)` — 物理後、コントローラがカメラを置いた後にカメラアニメの FOV とシェイクを足す。
  - `preview(feet | null)` — ロード中のプリウォームで輪を強制描画する（`TeleportRing.preview`。マテリアルとパーティクルのシェーダを先にコンパイルする）。
- `class TeleportRing`（teleport-ring.ts）
  - `constructor(scene)` — 床の発光の板と、斬撃と火花の 2 つの `ParticleSystem` を作る（非表示）。
  - `update(at: Vector3 | null, dt)` — 毎フレーム。`at`（床の位置）に輪を置き、null なら即座に消す。
  - `preview(at | null)` — プリウォーム用。`at` に出して斬撃を 3 本出す（`isReady()` でエフェクトを作らせる）。null で消す。
- teleport-fx.ts: `interface TeleportFx { fov, ev, tint: [r, g, b] }`（カメラアニメの FOV °、露出 EV、シーンのティント）、`interface ShakeOffsets { pitch, yaw, roll, fov }`（°）、`interface Vec3 { x, y, z }`、`NO_FX`、`NO_SHAKE`、定数 `BASE_FOV` 90、`MOVE_TIME` 0.12、`ANIM_TIME` 0.5、`SHAKE`、`TOTAL_TIME`（0.62）、`WHITE_TIME`（0.135416…）、`WHITE_EV` 100、`WHITE_FROM` 8、関数 `teleportFx(time)`、`shakeAt(s, phases)`、`widen(fov, animFov, extra = 0)`、`aimAlpha(alpha, wheelPx, step)`、`aimDistance(alpha, { minDistance, maxDistance })`、`springLag(current, target, dt, speed, maxStep = 1/60)`
- boost-fx.ts（スピードブーストの演出の Babylon 非依存の部分。値は pak_reference の `BP_DD_PlayerCharacter`・`CameraAnim_SpeedBoost`）: `BOOST_TINT` (2.0, 0.583955, 0.498)（`CameraAnim_SpeedBoost` の唯一のトラックのシーンのティント）、`BLEND_TIME` 0.5（`PlayCameraAnim` のブレンドイン・アウト。UE では線形）、`tintWeight(elapsed, duration)`（効果の外は 0、`min(1, elapsed / 0.5, (duration − elapsed) / 0.5)`）、`boostTint(elapsed, duration)`（白から `BOOST_TINT` への線形補間）。ウィジェット `UMG_SpeedBoost` と Chameleon（pak_reference の `UMG_SpeedBoost`・`Sprinting Effects`・`Chameleon`）: `ORIGINAL_SPRINT` 600（原作の走り cm/s。本作の `sprintSpeed` にあたる）、`WIDGET { fullSpeed 900, lines 0.15, vignette 0.5, linesScale 1.25, vignetteScale 1.2 }`、`SPEEDLINES { columns 2, rows 5, fps 60 }`、`CHAMELEON { fullSpeed 870, samples 8, blurReach 0.05, shakePower 0.003, shakeFrequency 15 }`、`originalSpeed(speed, sprintSpeed)`（m/s → 原作の cm/s）、`widgetOpacity(cmps, fresh)`（`MapRangeClamped(cmps, 0, 900)` × 0.15 / 0.5。`fresh` は 1 / 1: 原作の Tick は `Delay(0.001)` の後に不透明度を設定するので、出たフレームは不透明度 1）、`speedlineFrame(time)`（`floor(time × 60) mod 10`）、`chameleon(cmps)`（`MapRangeClamped(cmps, 0, 870)` をブラーの幅に、× 0.003 を揺れの強さに、× 15 を周波数（Hz）に）、`SHAKE_REACH` 2/3、`shakeOffset(phase, power)`（`phase` は周。`power × SHAKE_REACH × (sin 2πφ, −cos 2πφ)` の uv〈y は上向き〉: 画像が円を描き、下向きの y が x より 1/4 周先に進む、収録と同じ回り方）。コマ送りの速さ（FlipBook の入力が消えているので、検証映像の「毎フレーム入れ替わる」）、`blurReach`（`M_RadialBlurHLSL` の式が消えているので、検証映像の発動の放射ブラーの量）は本作の推定。揺れの形は `M_CameraShake` の式（`ShakePower`・`ShakeFQ` の使い方）が消えているので、原作の収録で測った: 2026-09-11 の収録（60fps）のブースト中 2 回で、タブレットの数字の重心（1080p、サブピクセル）の 9 フレーム移動平均からのずれが RMS x 2.2 / y 1.4〜1.6 px（通常時 0.01 px）、最も強いのが 14 Hz（14〜16 Hz。原作が約 100fps で動いていて 60fps で撮った分ぶれる）、x と y の位相差 90〜97°。つまり画像全体が約 15 Hz（`ShakeFQ` 15 を Hz と読む）で円を描き、移動平均の減衰を戻した振幅は x 0.0018・y 0.0023 uv なので、全速の `ShakePower` 0.003 に対し 0.002 uv（`SHAKE_REACH` 2/3）。60fps では 15 Hz は 4 フレームで 1 周する。`tests/boost-fx.test.ts` が確かめる
- walk.ts（原作の歩き・走りの Babylon 非依存の部分。`tests/walk.test.ts` が確かめる）: `UE_WALKING { maxAcceleration 20.48, brakingDeceleration 20.48, groundFriction 8, brakingFrictionFactor 2, brakingSubStep 1/33, brakeToStop 0.1 }`（UE 4.21 の CharacterMovement の既定値を m に直したもの。原作は MaxWalkSpeed しか変えていない）、`interface Planar { x, z }`、`calcVelocity(v, input, maxSpeed, dt)`（歩行の `CalcVelocity`。`v` を書き換える）、`interface FovCurve { fov, fovFast, fovSpeeds, fovInterpSpeed }`、`fovTarget(speed, curve)`、`FOV_TIMER` 0.001、`FOV_FRAME_RATE` 100、`fovFollow(current, target, dt, interpSpeed)`、`interface Oscillator { amplitude, frequency, zero? }`、`interface LoopShakeDef { blendIn, blendOut, pitch?, yaw?, lift? }`、`interface ShakeView { pitch, yaw, lift, roll? }`、`WALK_SHAKE`、`RUN_SHAKE`、`BOB_SPEED` 0.01、`class LoopShake(def, random = Math.random)`（`stop()`、`update(dt, out): boolean`）、`FOOTSTEP { walk 0.55, sprint [0.35, 0.4], boost 0.25, minSpeed 0.5, fastSpeed 6.5, fastPitch 1.5, volume 0.875, pitch [0.9, 1.1], marble 10, carpet 25 }`、`footstepDelay(boosted, sprinting, maxSpeed, random = Math.random)`

## 内部構造と処理の流れ
### 生成（コンストラクタ）
- `UniversalCamera` を `start + (0, CONFIG.camera.eyeHeight, 0)` に生成し `inputs.clear()`（Babylon 標準入力は使わない）。
- `minZ = CONFIG.camera.near (0.05)`, `maxZ = CONFIG.camera.far (90)`。
- `fovMode = Camera.FOVMODE_HORIZONTAL_FIXED` — UE4 流儀で設定値を**水平 FOV** として扱う。`fov` は度→ラジアン（`DEG = π/180`）。
- `updateUpVectorFromRotation = true` — `upVector` を毎フレーム `rotation`（yaw・pitch・roll）から作り直す。Babylon の `TargetCamera._getViewMatrix` は既定では `rotation.z` が前回と変わったときだけ `upVector` を更新するため、ダッシュ終了で roll が 0 に固定されるとその時点の yaw・pitch で傾いた `upVector` が残り、後から視点を回すと画面がロールしていた（下を向いてダッシュ→90° 振り向きで約 28.6°）。
- `PhysicsCharacterController(start + (0, h/2, 0), { capsuleHeight: CONFIG.player.capsuleHeight (1.8), capsuleRadius: CONFIG.player.capsuleRadius (0.32) }, scene)`。
- `cc.maxSlopeCosine = cos(50°)`（登坂上限 50°）、`cc.keepDistance = 0.04`。`cc.acceleration = 1`・`cc.maxAcceleration = 1e9`（`calculateMovement` の既定のゲイン 0.05 と加速の上限 50 m/s² を外し、walk.ts の速度をそのまま使う）。
- 質量・ステップ高（`maxStepHeight` 等）は明示設定していない（Havok の既定値）。
- `filterShape()`: カプセルの形の `filterCollideMask = ~PIECE_MASK`（壊れて飛ぶ・崩れる壊せる物の板〈07 記録の breakables.ts の `PIECE_MASK` 1 << 4〉に当たらない。原作の `BP_01_Woodboards` が壊れると当たりを Visibility にする〈@200〉のと同じ）。最後に `syncCamera(0)` で初期カメラ配置。

### `update(dt, look)` — 物理前
- `enabled` のとき:
  - `sens = mouseSensitivity × controls.look`、`yaw += look.dx * sens`, `pitch = clamp(pitch + look.dy * sens * (controls.invertY ? −1 : 1), -1.45, 1.45)`（原作はマウスの軸 × Mouse Sensitivity、Y は Inverted Y Axis でなければ −1 倍）。`controls.toggleSprint` なら Shift の押下エッジ（`wasPressed`）ごとに `sprintLatch` を反転する（原作の Toggle Sprint は押すたびにラッチを反転する。物理の段数に関係なく 1 フレーム 1 回にするため `update` で扱う）（`CONFIG.camera.mouseSensitivity = 0.0021`）
  - `Mouse1`（中クリック）の押下エッジで `turnAround()`（進行中でも押すたびに）
  - `KeyE` の押下エッジで `tryBoost()`（原作どおり右の枠のパワー。Q は左の枠のテレポーテーション）
- 180° ターン `turnAround()`: 原作の `InpActEvt_180 Turn`（`BP_DD_PlayerCharacter`、IE_Pressed だけ、ubergraph @9292〜@9530）は `CanMove?` のとき `GetControlRotation` → `BreakRotator` → `MakeRotator(Roll 0, Pitch 0, Yaw + 180)` → `SetControlRotation` するだけで、時間をかけて回す処理はない。本作も `yaw ± π`・`pitch = 0` を一度に入れ、見える回転は `syncCamera` の回転ラグ（`lagRotation`、原作の SpringArm の `CameraRotationLagSpeed`。レベルの開始時は既定の 20、ゲーム中にオプションを SAVE & EXIT した後は Mouse Smoothing の 12.5 / 50）に任せる。ピッチも同じラグで水平へ戻る。回る向きは UE の `QInterpTo`（最短経路）に倣い、押した時点のラグの残り `yaw − viewYaw` が正なら −π（左回り）、それ以外は +π（右回り）。原作ではちょうど 180° の釣り合いを浮動小数の誤差で破るので向きは一定しない（2026-09-13 の収録 71.13 s は左、71.63 s に押して戻るときは右）。本作は釣り合いなら右。収録（71.13 s、画面の帯の列の明るさの横ずれを NCC で測り f = 960 px で角度にした）の残りは、押してから 5 フレームで 26.2°、7 で 13.2°、9 で 7.5°、10 で 4.2°、12 で 1.9° と 1/60 s ごとに約 0.69 倍で、20/s（約 100 fps なら (1 − 20/100)^(100/60) ≈ 0.689）と合う。本作は 60 fps で 1 − 20/60 ≈ 0.67 倍（5 フレームで 156°、8 で 173°、10 で 177°）。
- 最初に `streak.time += dt`、`SHAKE.duration`（0.5 s）以上で `streak = null`（この後の `tryBoost` で始めたシェイクは、そのフレームに 0 s から見える）。
- `boostTime`, `boostCooldown` を `dt` ずつ減算（`enabled` でなくても進む）。`boostCooldown` が 0 に達した瞬間 `onBoostReady()`。

### `tryBoost()`（E キー）
- `boostCooldown > 0` なら `onBoost(false)`（`power_not_ready` を鳴らす側の合図）。
- それ以外は `boostTime = boost.duration (6.75)`, `boostCooldown = boost.duration + boost.cooldown (6.75 + 8.5 = 15.25)`、原作の `PlayCameraShake(BP_CameraShake_Streak, 1.0)` として `playStreakShake()`、`onBoost(true)`。
- `playStreakShake()`（public）: `streak = { time: 0, phases: 4 つのランダムな位相 }`（UE の振動子の既定はランダムな初期位相。再生中なら最初からやり直す）。ブーストと、シャードの連続回収の節目（game.ts。原作の `BP_DD_GameMode` の Check Streak も同じシェイクを 1.0 で掛ける。04 記録）が呼ぶ。
- 効果は `step()` で速度に `boost.multiplier (1.45)` を掛ける。クールダウンは発動と同時に始まる（効果時間込み）。発動の瞬間の赤い閃光は game.ts（04 記録）、効果中のシーンの赤（`boost-fx.ts` の `boostTint`。タブレットも一緒に赤くなる）と、原作のウィジェットの赤い集中線とビネット、Chameleon のラジアルブラーと画面の揺れ（`boost-fx.ts` の値）は 09 記録。
- 音（game.ts、原作の音と音量。pak_reference の `BP_DD_PlayerCharacter` の E の処理）: 発動で `power_boost`（原作の `Shard_Streak_Milestone_V5`。原作はアセット自体の Pitch 2.0・Volume 0.7 を `PlaySound2D` の 1.0 に掛けて鳴らすので、再生速度 2・音量 0.7）、使えないとき `power_not_ready`（0.35。原作の `Use Power` の共通の音量）、充填完了で `power_ready`（原作の `power_refilled`、0.5）。

### `step(dt)` — 物理後フック内
1. `dt <= 0` なら何もしない。
2. 入力ベクトル: `W/↑` → `iz+1`, `S/↓` → `iz−1`, `D/→` → `ix+1`, `A/←` → `ix−1`（`enabled` 時のみ）。長さで正規化。
3. ダッシュ: `sprint = enabled && (controls.toggleSprint ? sprintLatch : Shift(Left/Right) 押下)`。原作の Sprint は向きに関係なく MaxWalkSpeed を走りの速さにするので、後ろ向き・横向きでも走れる。`sprinting = moving && sprint`（敵の気配と `dash` に使う）。
4. 最高速度 `maxSpeed`: `boostTime > 0` なら `boost.speed`（8.7。原作はブースト中に Walking Speed と Sprinting Speed の両方を 870 にするので、歩いても 8.7）、それ以外は `sprint ? sprintSpeed (6) : walkSpeed (3)`。
   - 頭の揺れ（原作の `Update Bob`）: `sprint` が前のステップ（`sprintOn`）と変わったら `stopBob()`（両方のシェイクを止めて DoOnce を戻す。原作のダッシュの押下・解放・切り替え）。続けて `updateBob(sprint)`: 前のステップの `speed3` が `BOB_SPEED`（1 cm/s）以下なら `stopBob()`、超えていて DoOnce が開いていれば閉じ、`controls.headBob` なら `new LoopShake(sprint ? RUN_SHAKE : WALK_SHAKE)` を `bobShakes` に足す（原作の Head Bobbing は倍率 1 / 0 で、UE は倍率 0 のシェイクを作らない）。
5. `cc.checkSupport(dt, DOWN)` で接地判定。接地していれば、`yaw` だけで回した入力（原作の前進・右の軸も yaw だけの向き）で `calcVelocity(velocity, 入力, maxSpeed, dt)`（walk.ts。UE の歩行の速度計算。下の節）。非接地では水平速度をそのまま保つ（UE の落下は摩擦もブレーキもなく、AirControl 0.05 はほとんど効かない）。
6. Havok へ渡す速度（生成時に `cc.acceleration = 1`・`cc.maxAcceleration = 1e9` にしてあるので、`calculateMovement` は `velocity` を面に沿わせるだけで、独自の追従や加速の上限は掛けない）:
   - `SUPPORTED`: `cc.calculateMovement(dt, forward, averageSurfaceNormal, current, averageSurfaceVelocity, velocity, UP)`。結果から面速度を引き、上向き成分が `> 1e-3` なら `surfaceNormal.cross(out).cross(UP)` で水平面に再投影（坂・段差で打ち上げられないようにする）し、面速度を戻す。
   - 非接地: `calculateMovement(dt, forward, UP, current, Zero, velocity, UP)` の垂直成分を捨て、現在の垂直速度を維持しつつ `gravity (−19.6) × dt` を加える。
7. `cc.setVelocity(out)` → `cc.integrate(dt, support, (0, gravity, 0))`。
8. `v = cc.getVelocity()`（Havok の解決後の速度。壁に当たれば沿って滑った後の値）から `velocity = (v.x, 0, v.z)`（UE の歩行も実際に動いた分を次のフレームの速度にする）、`speed = hypot(v.x, v.z)`、`speed3 = |v|`（FOV 用。原作の `Speed` は上下を含む速さ）。
9. `dash` 目標: スプリント中は `clamp((speed − walkSpeed) / (sprintSpeed − walkSpeed), 0, 1)`、非スプリントは 0。`dash += (target − dash) × (1 − exp(−DASH_RATE (6) × dt))`。`dash < 0.005` で 0。F3 の表示とデバッグ API に出すだけ（以前のダッシュのラジアルブラーは原作に無いので外した。FOV、画面の集中線とビネットは速さで決まる。04・09 記録。タブレットは FOV で縮む。10 記録）。
10. 足音（原作の Tick の Delay。下の「原作の歩き・走り」）: `stepDelay` が null なら `footstepDelay(boostTime > 0, sprint, maxSpeed)` を入れ、毎ステップ `dt` を引く（始めたステップから引く。UE の latent action も追加したフレームに進む）。0 以下になったら null に戻し、接地かつ `speed > FOOTSTEP.minSpeed`（0.5 m/s）なら `onFootstep(maxSpeed > 6.5 ? 1.5 : 1)`。止まっていても Delay は回り続けるので、歩き出してから最初の 1 歩までは 0〜0.55 s のどこか。

### `syncCamera(dt)` — 物理後
- ブーストのシェイク: 位置と回転を置いた後、`streak` があれば `shakeAt(streak.time, streak.phases)`（teleport-fx.ts。テレポートの移動と同じ `BP_CameraShake_Streak`、0.5 s、最後の 0.25 s で弱まる）を足す: `camera.fov = (fov + s.fov)°`、`rotation.x −= s.pitch°`、`rotation.y += s.yaw°`、`rotation.z += s.roll°`。`fovDegrees`（F3 の表示）にはシェイクを含めない。シェイクの FOV でタブレットも一緒に縮む（カメラの子。原作も同じ）。
- FOV（原作の `FOV Multiplier`）: `fov = fovFollow(fov, fovTarget(speed3, CONFIG.camera), dt, fovInterpSpeed)`、`camera.fov = fov × DEG`（水平 FOV）。歩き以下 90°、ダッシュ 102.5°、ブースト 113.75°、9 m/s 以上 115°（下の「原作の歩き・走り」）。タブレットはカメラの子なので、この FOV でそのまま縮む（ダッシュ 0.80 倍、ブースト 0.65 倍。収録の 0.78〜0.8 倍・0.637 倍と合う。10 記録）。
- 回転ラグ（原作の SpringArm の `CameraRotationLagSpeed`）: `viewYaw = lagRotation(viewYaw, yaw, dt, controls.lagSpeed)`、`viewPitch` も同じ（settings.ts。1/60 s ずつのサブステップ）。カメラの向きとシェイクの上下の向きはこの `view*` を使う。移動の向きは `yaw`（原作の Control Rotation）のまま。
- 歩き・走りのカメラシェイク: `bob`（ロールも）を 0 にし、`bobShakes` の各 `LoopShake` を `update(dt, bob)` で足す（終わったものは外す）。
- 位置: カプセル中心 + `eye = CONFIG.camera.eyeHeight − カプセルの高さ/2`（1.62 − 0.9 = 0.72）+ 視点の上向き `(sin viewYaw · sin viewPitch, cos viewPitch, cos viewYaw · sin viewPitch) × bob.lift`（UE の CameraLocal の上下）。
- 回転: `camera.rotation = (viewPitch − bob.pitch°, viewYaw + bob.yaw°, bob.roll°)`（UE のピッチの上向きは Babylon の `rotation.x` の負）。`bob.roll` はいま常に 0（原作の歩き・走りのシェイクにロールは無い。以前の横揺れとダッシュのロール、ファンゲームのしゃがみの一回きりのシェイクはやめた）。

### 原作の歩き・走り（walk.ts）
原作の `BP_DD_PlayerCharacter`（pak_reference の CDO とバイトコード）は、BeginPlay で `Sprinting Speed` 600・`Walking Speed` 300 を入れ、Shift の押下で `MaxWalkSpeed = Sprinting Speed`、離して `Walking Speed`（TOGGLE SPRINT では押すたびに反転。@23878 / @23813 / @8118）。スピードブーストは両方を 870（強化 0。@12413 / @12773）にし、終わると 600 / 300 に戻す。CharacterMovement は `MaxWalkSpeed` 以外を上書きしていないので、加減速は UE 4.21 の既定値。
- `calcVelocity(v, input, maxSpeed, dt)`（`UCharacterMovementComponent::CalcVelocity` の歩行）: 入力を長さ 1 に切り詰め、加速度 `入力 × maxAcceleration`。上限 `maxInput = maxSpeed × |入力|`。入力が 0 か、速さが上限を超えている（`|v|² > maxInput² × 1.01`）ならブレーキ（上限を超えていて入力がその向きを押しているなら、上限より下げない）。そうでなければ摩擦で向きを変える: `v −= (v − 入力の向き × |v|) × min(dt × 8, 1)`。入力があれば `v += 加速度 × dt` を上限（超えていたらその時点の速さ）で切り詰める。
- ブレーキ（`ApplyVelocityBraking`）: 摩擦 `8 × 2 = 16`、減速 20.48 m/s²（最初の向きの逆）で `v += (−16 v + 減速) × h` を 1/33 s 以下の小刻み（残りの半分まで）で進め、向きが反転したら 0。最後に 0.1 m/s 以下なら 0。
- 60fps での値（テスト）: 止まった状態から歩きの 3 m/s まで 9 フレーム（0.15 s）、手を離すと歩きから 4 フレーム・走りから 6 フレームで止まり、Shift を離すと 2 フレームで 3 m/s に落ちる。斜めの入力でも速くならない。
- FOV: `fovTarget(speed, c)` = `MapRangeClamped(speed, fovSpeeds[0] 3, fovSpeeds[1] 9, fov 90, fovFast 115)`（カメラの既定の FOV 90 は原作も上書きしていない）。原作は 0.001 s のループタイマー `FOV Multiplier`（@29760）で `FInterpTo(FOV, target, フレームの DeltaSeconds, 0.5)` を呼ぶ。UE のタイマーは 1 フレームに経過した回数だけ呼ぶので、追従の速さがフレームレートで変わる（毎秒 約 500 × dt: 60fps で 1 フレーム約 16.7 回、実効 8.4 /s、100fps で 10 回、5.0 /s）。原作データでは決まらないので、2026-09-11 の原作の収録（ユーザー提供、60fps）で測った速さに合わせる: ブーストの終わり 2 回で、タブレットの画面上の大きさ（1/tan(FOV/2) に比例。10 記録）から逆算した FOV の残りが 4.97 / 4.89 /s で縮む（約 100fps のときの追従）。`fovFollow` はどのフレームレートでも 100fps のときの追従にする: 1/100 s ごとに差を `(1 − 0.5/100)^(1000/100)`（約 0.951）倍、50 % が 0.14 s、90 % が 0.46 s。
- カメラシェイク（`BP_DD_PlayerCharacter_WalkShake` / `_RunShake`、pak_reference の `_camera/_camera_shakes.json`）: 長さ 999999 s（止めるまで続く）、ブレンドイン 1.0 s・アウト 0.5 s、CameraLocal。WALK: ピッチ 0.2° / 12 rad/s、上下 2 cm / 13 rad/s。RUN: ピッチ 0.5° / 18、ヨー 0.2° / 10、上下 4 cm / 20。上下は位相 0 から（EOO_OffsetZero）、回転は UE の既定どおりランダムな位相から。周波数は UE4 の振動子どおり rad/s（`phase += dt × Frequency`、`sin(phase)`）。`LoopShake.update` の重みは `min(time / blendIn, left / blendOut)`（UE の `UCameraShake::UpdateAndApplyCameraShake`）。`stop()` は `StopShake(bImmediately = false)` で、残り時間を `min(残り, blendOut)` にする（何度呼んでも延びない）。原作の `Update Bob`（@8520）は前進・後退・右の軸入力のたびに走り、ダッシュの押下・解放（@23878 / @23813）と TOGGLE SPRINT の切り替え（@8118）は両方のシェイクを止めて DoOnce を戻してから同じ判定をする。歩きの上下の周期は約 0.48 s、走りは約 0.31 s（足音の間隔とは連動しない）。
- 足音（`FOOTSTEP`、`footstepDelay`）: 原作の Tick（@26366〜26677）は毎回 `Delay(ブースト中 0.25 : (Sprinting? || MaxWalkSpeed > 650) ? RandomFloatInRange(0.35, 0.4) : 0.55)` を呼ぶ（待っている Delay があれば何もしない）。Delay が終わると @2528: 横の速さ > 50 cm/s かつ歩行モードなら、足元への下向きトレースの床の材質で SoundCue を選び、`PlaySoundAtLocation(音量 0.7, ピッチ MaxWalkSpeed > 650 ? 1.5 : 1.0, 減衰 01_Lobby_Attenuation)`（位置は足元の 8 cm 上、進む向きへ 30 cm）。`Footsteps_Marble`（10 種）・`Footsteps_Carpet`（25 種）は VolumeMultiplier 1.25、Modulator のピッチ 0.9〜1.1。鳴らすのは game.ts（04 記録）。60fps の間隔は歩き 33 フレーム、ダッシュ 21〜24 フレーム、ブースト中 15 フレーム（1/60 s を引き続けると丸めで 0 の少し上に残るので、`1e-6` 以下で終わりとみなす。UE では float32 の DeltaSeconds が切り上がっていて、同じフレーム数になる）。

### テレポーテーション（teleport.ts）
原作の構成（pak_reference の `BP_Power_Teleport` のバイトコード）: Q で照準のアクター（デカール・パーティクル・照準ループの音・SpringArm を持つ）がスポーンし、毎 Tick に「アクター位置（カプセル中心）+ 前方 × Distance」から下へ 500 cm を Teleport チャンネル（レベルに置いた不可視の Teleport_Zone のメッシュだけが当たる）でトレースし、当たれば SpringArm（アーム長 0。スポーンの 0.5 s 後にカメララグを有効にする。ラグ速度は UE 既定の 10）を当たった位置へ動かす。デカールはその子なので遅れて追う。左クリックで「デカールの位置 + 125 cm」を控え、カメラアニメを再生し、0.12 s 後にカメラシェイク・確定音・`SetActorLocation(位置, sweep, teleport)`（その間カプセルは WorldDynamic と Pawn を無視）→ `Used`（プレイヤーが 5 s のクールダウンを始める）→ アクターを破棄。本作はこれを次のように移している。
- **操作**（`update()`、`active` かつ `player.enabled` のときだけ）: `KeyQ`（定数 `KEY`）の押下エッジで、照準中なら `cancel(true)`（原作の `Reset Teleport`: 照準を消し、すぐ使える。`onCancel`）、クールダウン中かジャンプ中なら `onDenied()`、それ以外は `startAim()`（`aiming = true`、`alpha = startAlpha (0.6)`、`aimTime = 0`、`target = null`、`spawn = 今のカプセル中心 − (0, SPAWN_DEPTH 50, 0)`（原作が照準のアクターをスポーンする位置）、`onAim(true)`）。照準中は `input.wheel` があれば `alpha = aimAlpha(alpha, wheel, wheelStep 0.1)`、`Mouse0`（定数 `CONFIRM`）の押下エッジで、まだ確定していなければ `confirm()`。`active` でない（ポーズ・終了）か `player.enabled` が false なら、確定前の照準を `cancel()`（音なし）。確定後の 0.12 s はポーズ中も進む。
- **距離**: `aimDistance = aimDistance(alpha) = minDistance + (maxDistance − minDistance) × alpha` = `Lerp(2.5, 10, alpha)`。Q のたびに 0.6（7 m）から。`aimAlpha` はホイール 100 px（Chrome のマウスの約 1 目盛り）で alpha ±0.1（0.75 m）、奥へ回す（deltaY 負）と遠く、0..1 にクランプ。原作は `Alpha = FClamp(Alpha + 軸の値 / 10)`、`Distance = Lerp(250, Max Distance, Alpha)`、`Max Distance` は強化段階 0 で 1000 cm（強化 1〜5 で 1000〜1500 cm。本作に強化はない）。
- **トレース**（`trace(dt)`、照準中は毎フレーム。確定から移動までも続く）: カプセル中心 + `(sin yaw, 0, cos yaw) × aimDistance`（上下の向きは使わない）から真下へ `traceDepth (5)` m の Havok の `raycastToRef`。当たり、かつ `zone(x, 当たった点の y, z)`（game.ts の `FloorGrids`: 当たった点の高さの床の段の格子で歩けるセル。壁・ベンチ・門の箱の下と、そこから敵の半径 0.4 m 以内は歩けない。どの段からも 0.3 m 以上離れた高さ（階段の坂、ベンチの上）には置けない）なら、その点を `traced` にする。当たらないか歩けない床なら、輪は最後の位置のまま。輪の位置 `target` は、最初に捉えたときと照準開始から `lagDelay (0.5)` s までは `traced` そのもの、以後は `springLag(target, traced, dt, lagSpeed 10)`。物理エンジンが無いときは足の高さの点を使う。
- **確定**（`confirm()`）: 輪の位置 `target`（床をまだ捉えていなければ `spawn`。原作のデカールは床を捉えるまでスポーン位置のまま）から `to = 輪 + (0, landHeight − capsuleHeight / 2, 0)`（足位置。カプセル中心を輪の 1.25 m 上に置く）と、シェイクの 4 軸のランダムな位相（0〜2π）を持つ `jump` を作る。照準（輪・照準ループ）は移動まで続く。
- **移動**（`advance()`）: `jump.time` が `MOVE_TIME`（0.12 s）に達した最初のフレームで、取り消されていなければ `sweep(今の足位置, to)` → `player.setFeet(結果)`、`aiming = false` + `onAim(false)`（原作は移動と同時に照準のアクターを破棄する）、`cooldown = 5`、`onArrive(from, 結果)`（game.ts が確定音・経路上のシャード回収・`post.cut()`）。確定から移動までに Q を押すと `cancelled` になり、移動しないがカメラアニメは最後まで続く（原作は照準のアクターと待ち中の移動だけが消え、カメラマネージャのアニメは残る）。そのあと `fx = teleportFx(time)`、`shake = shakeAt(time − MOVE_TIME, phases)`（移動していないときは 0）。`TOTAL_TIME`（0.62 s。シェイクの終わり）で `jump = null`、`fx` と `shake` を静止に戻す。
- **白の 1 フレーム**: 露出のキー（+100 EV、0.1354 s）は幅が約 0.01 s しかなく、60fps では 8 フレーム目（0.133 s、移動と同じフレーム）が +67.7 EV で真っ白になる。フレームの刻みがこのキーをまたぐと飛び越えうるので、まだ白（`WHITE_FROM` 8 EV 以上）を出していなければ、`WHITE_TIME` を過ぎた最初のフレームの露出を +100 EV にする（`jump.white`）。
- **スイープ**（`sweep(from, to)`）: 原作の `SetActorLocation(sweep)` に当たる Havok の `shapeCast`（`physics.getPhysicsPlugin() as HavokPlugin`）。カプセル（`PhysicsShapeCapsule`、半径 `capsuleRadius − SWEEP_SHRINK (0.02)`、プレイヤーと同じ高さ。初回に作る）を、今のカプセル中心 + `SWEEP_LIFT (0.05)` m から `to` のカプセル中心まで動かし、当たれば `hitFraction − SWEEP_SKIN (0.02) / 長さ` の位置で止める（最初から壁や床に触れないよう細く・少し浮かせてある）。当たるのは静的コライダーの箱（壁・床・ベンチ）と閉じた門の ANIMATED ボディ。敵は物理ボディを持たないので当たらない（原作もカプセルは Pawn を無視する）。止まった位置の足位置を返す。ヘッドレスの Chrome の検証（デバッグフィールド）で、壁の向こうの輪へは壁面の 0.32 m 手前、閉じた門の向こうへは門の 0.32 m 手前で止まり、何もなければ 7 m 進んだ。
- **カメラ**（`applyToCamera`、物理後、`syncCamera` がカメラを置いた後）: `camera.fov = widen(camera.fov, fx.fov, shake.fov)`、`rotation.x −= shake.pitch`、`rotation.y += shake.yaw`、`rotation.z += shake.roll`（° → rad。UE のピッチの上向きは Babylon の `rotation.x` の負）。`syncCamera` が毎回 FOV と回転を設定し直すので積み重ならない。カメラの位置は変えない（原作のカメラアニメの移動トラックは原点の 1 キーだけ）。プレイヤーの `fovDegrees` は変えないので、タブレットは FOV の広がりで画面中央へ縮み、振れ戻りで大きくなる（原作のカメラに付いたタブレットと同じ）。
- **照準の輪**: 毎フレーム `ring.update(aiming ? target : null, dt)`（下の「照準の輪（teleport-ring.ts）」）。照準中で `target` があるときだけ出て、移動・取り消しで即座に消える（原作は照準のアクターごと消える）。位置のラグは `trace` で済んでいる。
- **音**（game.ts、すべて原作の音と音量）: 照準に入ると `teleport_enter`（`Teleport_Mode_Entered`、1.75）と `teleport_aim` のループ（0.65。原作の照準のアクターの AudioComponent と同じで、Q で鳴り始め、移動か取り消しで止まる。fadeIn 0.02 s、止めるとき 0.03 s）、移動の瞬間に `teleport`（`Teleport_Committed`、1.0）、照準中の Q で `power_ready`（原作の `power_refilled`、0.5）、クールダウン中の Q で `power_not_ready`（0.35）、充填完了で `power_ready`（0.5）。`Teleport_Mode_Entered` と `Teleport_Committed` は UE エンジン同梱の音で、pak_reference からコピーした（13 記録）。
- **タブレット**（game.ts）: Q を押すたびに `tablet.pop('teleport')`（原作の `Use Power` は使えるかの判定の前に「Use Left」で左の枠を弾ませる）、毎フレーム `tablet.setRefill('teleport', percent)`（原作の枠の表示。10 記録）。

### 照準の輪（teleport-ring.ts）
原作の照準のアクターは、SpringArm の先にデカール `M_Decal_Teleport`（放射グラデーションの発光デカール。`DecalSize` 100 cm で 2 m 四方）を持ち、その子にパーティクル `P_ky_cutter2`（スケール 0.2）を付けている。`P_ky_cutter2` は 2 つのエミッタ: 「cutter」（`M_ky_slash01_4x4`: 4×4 のフリップブックで、R は三日月が一周余り掃いて消える斬撃。毎秒 10 + 移動 50 cm ごとに 1、寿命 1 s、大きさ 300 cm × 寿命で 1 → 2 倍、向きランダム・0.5〜1 回転/s、Z 軸固定で床に寝る、色 (13, 0, 0.22) でアルファ 1 → 0、ローカル空間）と、赤い点（`PPP_Radial_Gradient_Doffed`: 毎秒 30、寿命 2 s、上へ 300〜500 cm/s、色 (5, 0, 0) → (1, 0, 0) でアルファ 1 → 0、±170 cm の範囲から、ローカル空間）。本作はこの構成をそのまま作る。
- **根**（`createGlow()`）: `MeshBuilder.CreateGround('teleportRing')`（一辺 `ring.glow.size` 2 m）の床に水平な板。256² の `DynamicTexture` に UE の RadialGradientExponential に当たる `(1 − d/r)^density (2.33)` の放射グラデ（中央が最も明るい）を 9 段の色止めで描く。`PBRMaterial`（albedo 黒、`disableLighting`、`environmentIntensity 0`、`emissiveTexture`、`emissiveColor = glow.color [1.0, 0.15, 0.5]`、`emissiveIntensity = glow.intensity (0.6)`、`ALPHA_ADD`、深度書き込みなし）。床 + `LIFT`（0.02 m）に置く。2 つのパーティクルのエミッタでもある（どちらも `isLocal = true` なので、輪と一緒に動く）。原作のデカールのパラメータは cook で消えているので、形と色は推定。
- **斬撃**（`createSlashes()`）: `ParticleSystem('teleportSlashes', 48, scene, null, true)`（アニメーションシート有効）。テクスチャは 1024² の `DynamicTexture` に `drawSlashSheet()` が描く 4×4 のフリップブック（白地に透明。原作のテクスチャはサードパーティ製なので写さず、キャンバスで描く）。`spriteCellWidth/Height 256`、セル 0〜15、`spriteCellChangeSpeed 1`（Babylon は寿命に比例してセルを進める）。原作のフレームの進み（`SubImageIndex` の曲線 `SUB_UV`: 寿命の 1/3 で 10.6 フレーム目まで進む ease-out）に合わせ、セル k には寿命 `(k + 0.5)/16` のときの原作のフレームを描く。
  - `drawSlash(p)`（`p` = フレーム / 15）: セルの縁（半径 0.42 セル）に沿う三日月。頭の角度は `−90° + 396° × (0.15 + 0.85 × (1 − (1 − p)^2.2))`（最初に速く一周余り）、尾は `−90° + 331° × clamp((p − 0.25)/0.75)`（遅れて追う）、太さは `0.015 + 0.11 × sin(π·min(1, p/0.9))^1.5` セル、明るさは `min(1, 0.35 + p/0.12) × (1 − 0.9·smoothstep((p − 0.55)/0.45))`。本体は 48 分割の短い弧（外縁を半径にそろえ、太さは両端 0・頭寄りで最大）、その上に LCG 乱数（seed 11）の細い筆の弧 36 本（尾へ向かって薄くなる）。
  - 床に寝かせる: `isBillboardBased = false` の四角は向き（`direction`）に垂直な面に描かれるので、向きをほぼ真上 `(0.001, 1, 0)`（真上ちょうどだと Babylon の四角の基底の外積が 0 になる）、`emitPower 0.001` にする。`isLocal = true`、出る位置は輪の中心の床 + 0〜0.01 m。
  - 寿命 `slash.life` 1 s、初期の向き 0〜2π、角速度 `slash.spin` 0.5〜1 回転/s、大きさは原作の曲線 `GROW`（寿命の k/7 で 1, 1.055, 1.198, 1.394, 1.606, 1.802, 1.945, 2）× `slash.size`（1.3 m）の `addSizeGradient`、色は `slash.color [4, 1.2, 2]` でアルファ 1 → 0 の `addColorGradient`、`BLENDMODE_ADD`、`updateSpeed 1/60`。
  - 出し方: `emitRate = 0` で、`update` が毎フレーム `owed += slash.rate (10) × dt + 動いた距離 / slash.spacing (0.5)`（原作の SpawnPerUnit）の整数部を `manualEmitCount` にする（1 フレーム `MAX_BURST` 3 本まで、持ち越しは 1 まで。照準の最初の 0.5 s に輪が跳ぶと一度に何本も出るため）。出たときは 1 本目をすぐ出す。
- **火花**（`createSparks()`）: `ParticleSystem('teleportSparks', 96)`、32² の放射グラデの点。`isLocal = true`、輪の中心から半径 `sparks.radius` 0.8 m の円内（面積一様）の床 + 0.01 m から、ほぼ真上（横に ±0.01）へ `sparks.speed` 0.6〜1.0 m/s、寿命 `sparks.life` 2 s、大きさ `sparks.size` 0.02〜0.05 m、`emitRate = sparks.rate` 30、色 `sparks.color (5, 0, 0)` アルファ 1 → `sparks.end (1, 0, 0)` アルファ 0、`BLENDMODE_ADD`、`updateSpeed 1/60`。
- **表示**: `update(at)` は、出ていなければ根を有効にして両方を `start()` し、毎フレーム根を `at + LIFT` に置く。`at` が null なら根を無効にし、両方を `stop()` + `reset()`（空中の粒も消える。原作のアクターの破棄と同じ）。
- 原作の見た目との違い: 原作のパーティクルはデカールの不均等なスケール（X 3.33 × 0.2、Y・Z 0.2）を受けるので、斬撃は 300 cm × (0.665, 0.2) の楕円に、火花は ±113 × ±34 cm の長方形の範囲になる。本作は輪を丸く見せるため一様にし、斬撃の大きさを原作の映像（外径約 1.6 m の帯）に合わせて 1.3 m、火花を半径 0.8 m の円にした。斬撃の色も、原作の (13, 0, 0.22) を Babylon の ACES に通すと橙に飽和するので、映像で見える白に近いピンクになるよう (4, 1.2, 2) にした。数・寿命・回転・大きさの伸び・フレームの進み・火花の速さと色は原作の値。

### テレポートの演出（teleport-fx.ts）
- 原作 `CameraAnim_Teleport`（0.5 s、再生の倍率 1、ブレンドなし）のトラックを、書き出されたキー（`CIM_CurveAutoClamped`、解決済みの接線）から UE と同じ三次エルミート（接線 × キー間隔）で評価する（`src/core/ue-curve.ts` の `evaluateCurve`。キーは `curveKeys(times, values, tangents)` で作る `CurveKey = [時刻 s, 値, 接線（値/s。到着の接線で、5 番目が無ければ出発も同じ）, 補間?, 出発の接線?]`。4 番目の `Interp`（`'cubic'` 既定 / `'linear'` / `'constant'`）はそのキーから次のキーまでの区間の補間で、UE の `RCIM_Linear` は直線、`RCIM_Constant` は次のキーまでそのキーの値を保つ。三次の区間はそのキーの出発の接線と次のキーの到着の接線で結ぶ（ファンゲームの街灯の曲線は両者がわずかに違う。07 記録の `stage-lamps.ts`）。範囲外は端のキーの値。タイトル画面・ステージ OP のアニメ〈10 記録〉と共用。`tests/ue-curve.test.ts` がキー上の値、範囲外、接線 0 の 2 キー〈smoothstep〉、接線による行き過ぎ〈`UMG_PopUp` の拡大〉、Linear と Constant の区間を確かめる）。
  - FOV: 90 → 150 (0.13 s) → 80 (0.18) → 100 (0.25) → 90 (0.40)、接線はすべて 0。
  - AutoExposureBias（EV）: 0 → 1 (0.07、接線 15.38/s) → 2 (0.13、62.07) → 100 (0.1354、0) → 2 (0.1398、−107.3) → 0 (0.21)。
  - SceneColorTint（RGB）: 白 (0、0.0697) → (2, 0.1145, 0) (0.1282〜0.2110) → 白 (0.3795)。接線は書き出しの値（R 7.8 / 7.08 / −3.98、G −6.91 / −6.27 / 3.52、B −7.8 / −7.08 / 3.98）。
  - 移動トラックは原点の 1 キーだけなので使わない（カメラは動かない）。
  - `teleportFx(time)`: 範囲外（負・NaN・0.5 以上）は `{ fov 90, ev 0, tint [1, 1, 1] }`。60fps の値は pak_reference の `_camera/CameraAnim_Teleport.csv` と一致する（テスト）。
- シェイク `BP_CameraShake_Streak`（移動と同時に始まる。0.5 s、ブレンドイン 0、ブレンドアウト 0.25 s、倍率 1）: Pitch 0.25° / 30、Yaw 0.25° / 40、Roll 0.5° / 35、FOV 2° / 10。`shakeAt(s, phases)` = `振幅 × sin(位相 + 周波数 × s) × 重み`、重みは最後の 0.25 s で線形に 0 へ。UE4 の振動子は `offset += dt × Frequency; sin(offset)` なので Frequency は rad/s として扱う（pak_reference の README にある `sin(2π·f·t)` ではない）。位相は UE の既定（ランダム）と同じく `confirm` で決める。
- `widen(fov, animFov, extra)`: 原作のカメラの FOV は 90°（プレイヤーの既定）で、FOV のキーはそこから始まり、そこへ戻る。本作の FOV（歩き 75°、ダッシュ 90°、ブースト 101°）でも像の縮み方が同じになるよう、焦点距離を `tan(45°) / tan(animFov / 2)` 倍する（`2·atan(tan(fov/2) × tan(animFov/2) / tan(45°))`）。シェイクの FOV は度で足し、UE と同じく 5〜170° に収める。歩きの 75° はピークの 150° で約 142°、ダッシュの 90° はそのまま 150°。
- `springLag(current, target, dt, speed, maxStep)`: UE の SpringArm の位置のラグ。`dt` が `maxStep`（1/60 s）以下なら `min(1, dt × speed)` だけ詰める。長いフレームは、目標がそのフレームのあいだに直線で動いたとして 1/60 s ずつの小刻みで詰める（UE の `bUseCameraLagSubstepping`）。速度 10 では 60fps の 1 フレームで差の 1/6、7 m/s で動く点には約 0.7 m 遅れて付いていく（テスト）。
- 画面への反映: 露出とティントは postfx の `wasamiTeleport`（TAA の直後の HDR のパス。09 記録）、FOV とシェイクは `applyToCamera`。放射状の流れは専用のパスを持たず、FOV の変化で速度が出るオブジェクトベースのモーションブラーに任せる（原作の UE のモーションブラーと同じ仕組み）。タブレットもシーンの一部なので、露出とティントで一緒に赤くなり白く飛ぶ。
- 60fps での並び（ヘッドレスの Chrome で `debug.fixedDt` = 1/60 の 1 フレームずつ確認）: 1〜7 フレーム目は FOV が広がり（歩きで 76° → 140° 前後）、露出が +0.1 → +1.4 EV で明るくなり、5 フレーム目から赤橙になる。8 フレーム目（0.133 s）で移動し真っ白（+67.7 EV）、9 フレーム目は FOV 125°（原作の値）・+1.1 EV の赤、11 フレーム目で最も狭く（80°）、15 フレーム目で 100°、24 フレーム目（0.4 s）で戻る。赤は 0.21 s まで (2, 0.11, 0) のまま、0.38 s で抜ける。白の後の明るさの戻り方は、以前に原作の映像で測った値（0.47 → 0.6 → 0.71 → 0.88 → 0.98）とティントの輝度（0.49 → 0.56 → 0.68 → 0.81 → 0.93）が合う。

### 入力の消費方法
- `Input.beginFrame()` は `Game.update` が毎フレーム 1 回呼び、押下エッジ集合とマウスデルタを確定する。コントローラは `input.wasPressed(code)`（エッジ）と `input.isDown(code)`（保持）だけを参照し、キーイベントを直接扱わない。
- `update` はフレーム側で、`step`/`syncCamera` は `onAfterPhysicsObservable` 内で `Game` が呼ぶ（本クラス自身は Observable を登録しない）。
- `enabled = false`（ポーズ・終了）でも `step` は動くが入力がゼロになり減速停止する。ターン・ブーストのタイマーも進む。

## 依存関係
- controller.ts の import: `../config`（`CONFIG`）, `../core/input`（型のみ）, `../game/settings`（`lagRotation`）, `./teleport-fx`（`SHAKE`・`shakeAt`）, `./walk`, `../world/breakables`（`PIECE_MASK`。07 記録）
- 使う側: `src/game/game.ts`（生成、`update/step/syncCamera` 呼び出し、コールバック接続、`feet/forward/dash/speed/yaw/fovDegrees` の参照、`teleport`）
- 外部: `@babylonjs/core` の `PhysicsCharacterController`（`shape`）, `CharacterSupportedState`, `UniversalCamera`, `Camera.FOVMODE_HORIZONTAL_FIXED`, `Quaternion`, `Vector3`
- teleport.ts → `../config`、`../core/input`（型）、`./controller`（型）、`./teleport-fx`、`./teleport-ring`。Babylon の `PhysicsRaycastResult`、`PhysicsEngineV2.raycastToRef`、`HavokPlugin.shapeCast`、`PhysicsShapeCapsule`、`ShapeCastResult`、`Quaternion`、`TargetCamera`（型）。使う側は game.ts（生成・`zone`・コールバック・`update`・`applyToCamera`・`preview`・デバッグ API の `state().teleport`）。
- teleport-ring.ts → `../config`。Babylon の `MeshBuilder.CreateGround`、`DynamicTexture`、`PBRMaterial`、`Constants.ALPHA_ADD`、`ParticleSystem`（アニメーションシート、`isLocal`、`addSizeGradient` / `addColorGradient`、`manualEmitCount`）、`Matrix.IdentityReadOnly`、`Color3` / `Color4`。使う側は teleport.ts だけ。
- teleport-fx.ts は import なし（`tests/teleport-fx.test.ts` が `node --test` で直接読む）。値は pak_reference の `_camera/CameraAnim_Teleport.json`、`_camera/_camera_shakes.json`、`_bytecode/.../BP_Power_Teleport.txt`。

## 設定・調整値
- `CONFIG.camera`: `fov` 90, `fovFast` 115, `fovSpeeds` [3, 9], `fovInterpSpeed` 0.5, `near` 0.05, `far` 90, `eyeHeight` 1.62, `mouseSensitivity` 0.0021
- `CONFIG.player`: `walkSpeed` 3, `sprintSpeed` 6, `capsuleRadius` 0.32, `capsuleHeight` 1.8, `gravity` −19.6, `boost { speed 8.7, duration 6.75, cooldown 8.5 }`（加減速は config ではなく walk.ts の `UE_WALKING`。原作のデータ）
- `DASH_RATE` 6（controller.ts の定数。`dash` の平滑化）
- `CONFIG.player.teleport`（原作の値）: `minDistance` 2.5 / `maxDistance` 10（照準の距離の範囲 m。原作の 250 cm と強化なしの `Max Distance` 1000 cm）, `startAlpha` 0.6（Q のたびの alpha、7 m）, `wheelStep` 0.1（ホイール 100 px あたりの alpha）, `traceDepth` 5（真下へのトレースの長さ m）, `lagDelay` 0.5 / `lagSpeed` 10（輪が追い始めるまでの秒と SpringArm のラグ速度）, `landHeight` 1.25（輪からカプセル中心までの高さ m）, `cooldown` 5（移動からの秒。原作の `Set Delay(5.0)`。チュートリアルの 00_Ballroom だけ 1.0 だが本作にはない）, `ring { glow { size 2, density 2.33, color [1.0, 0.15, 0.5], intensity 0.6 }, slash { size 1.3, rate 10, spacing 0.5, life 1, spin [0.5, 1], color [4, 1.2, 2] }, sparks { rate 30, life 2, size [0.02, 0.05], speed [0.6, 1.0], radius 0.8, color [5, 0, 0], end [1, 0, 0] } }`（照準の輪。上の節）。カメラアニメとシェイクの値は config ではなく teleport-fx.ts の定数（原作のデータ）。
- `CONFIG.player.capsuleRadius` / `capsuleHeight` は本クラスのほか、`src/world/shards.ts` の `touching()`（シャードに触れたかの判定。半径は原作のプレイヤーの `pickupRadius` 0.5 を使い、カプセルの軸の高さだけを本作のもので決める）も参照する。
- いずれも URL クエリで上書き可（例: `?camera.fovFast=120&player.sprintSpeed=8`）。

## 既知の制約・注意点
- 質量・ステップ高・`maxSlopeCosine` 以外の Havok キャラクタ設定は既定値のまま。段差の乗り越え高さは未調整。
- FOV は水平固定モードのため、縦長ウィンドウでは垂直 FOV が大きく広がる。プリウォーム（`game.ts`）もこの水平 FOV で描画する。
- ジャンプは未実装。空中では水平速度を保ち、入力は効かない（原作の AirControl 0.05 は再現していない）。
- 視点の高さ（原作はカプセル中心 + 95 cm で足元から約 183 cm）、カプセルの半径（原作 50 cm）、重力（原作の DefaultEngine.ini が手元に無く未確認）は原作に合わせていない。館は Blender の手続き生成で原作のスケールではないため。
- `pitch` の上限 ±1.45 rad（約 ±83°）はハードコード。
- `dispose()` はカメラのみ破棄し、`PhysicsCharacterController` は破棄しない。
- キーは原作どおり Q = 左の枠（Teleportation）、E = 右の枠（Speed Boost）。原作資料の「PC は Q / E で発動」[B] とユーザーの指示による。
- 距離は照準に入るたびに 7 m（alpha 0.6）に戻る。原作は照準のアクターを毎回作り直すので、前回の alpha を覚えない。
- 輪を置ける床は、原作の Teleport_Zone のメッシュの代わりに床の段ごとのナビ格子（`zone`。navgrid.ts の `FloorGrids`。ステージでは床の箱の上面が 地上 0 m・駅の中の −9.92 m と −12.68 m・ホームとトンネル −21.02 m の 4 段。地上の段は敵の格子そのもの）で判定する。ファンゲームのテレポートはゾーンを持たず（`ThirdPersonCharacter` の `Teleport_springArm_Z` が下向き 900 cm を `ECC_WorldStatic` で探る）、駅の中でも照準が出る。2026-09-15 までは敵の格子（地上の段だけ）で判定していて、駅の中では輪が出ず、クリックすると動かずにクールダウンだけ始まっていた。敵の半径 0.4 m で壁から離してあるので、壁ぎわ 0.4 m 以内や家具・門の下には置けない（原作のゾーンの範囲とは一致しない）。
- トレースは原作どおり 1 本の下向きのレイだけで、途中の壁は見ない。壁を越えた先に歩ける床があれば輪はそこに出て、確定時のスイープが壁の手前で止める（原作と同じ）。
- 着地はカプセル中心が輪の 1.25 m 上なので、足が床から 0.35 m 浮いて落ちる（原作も立ち姿のカプセル中心 88 cm に対し 125 cm で、約 37 cm 落ちる）。
- 照準を始めてから床を一度も捉えないうちのクリックは、原作どおり `spawn`（Q の時点のカプセル中心の 50 m 下）+ 1.25 m へのスイープになり、すぐ下の床で止まるのでその場に留まる（足が 0.05 m ほど浮いて落ちる）。カメラアニメ・確定音・5 s のクールダウンはふつうの発動と同じ。
- 照準は水平の向きだけで、見下ろしても狙いは変わらない。トレースはカプセル中心の高さから下なので、それより高い床は狙えない（館は平らな床だけ）。
- 確定から移動までの 0.12 s に Q を押すと、移動だけが取り消され、カメラアニメ（白飛びを含む）は最後まで続く（原作と同じ）。
- 照準の輪と火花は反射プローブ・ミニマップの撮影対象に入らない（どちらも構築時のリスト）。
- 移動の瞬間はカメラが大きく跳ぶので、そのフレームと次のフレームはモーションブラーを止める（postfx の `cut()`。白の 1 フレームと重なる）。60fps より速い画面では、移動（0.12 s）が白（0.132〜0.139 s）の 1 フレーム前に来て、跳んだ画が 1 フレーム見える（原作も同じ）。
- 焦点距離の広がりのピークでは水平 FOV が約 142°（歩き）〜150°（ダッシュ）になる。水平固定モードなので縦長のウィンドウでは垂直方向がさらに広がる。
- **本家の版**：テレポーテーションは旧版（pak_reference、UE 4.21 の `dd.pak`）に従う（2026-09-15、ユーザーの決定）。新しい Steam 版（pak_reference_2）の `BP_Power_Teleport` は `CameraAnim_Teleport` をやめて `wtfUE4`（露出のトラックなし）と、0.12 s 後の赤い閃光（PostProcess の SceneColorTint (10, 0, 0) を Timeline「Red Effect」0.5 s で重ねる）にしているが、採らない（`pak_reference_2/README.md` §3.1）。
- しゃがみ・スライドは無い（本家のプレイヤーに無い）。ステージの地下には、ファンゲームのしゃがみでだけ通れる低い床が残りうる（colliders.json の `overhead` がそこの天井になる。12 記録）が、立ったプレイヤーは通れない。flow-check の経路（`route`）はそこを通らない。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: `camera.updateUpVectorFromRotation = true` を追加。ダッシュ後に視点を回すとカメラが傾いたまま戻らない不具合（`upVector` が `rotation.z` 変化時にしか更新されない）を修正
- 2026-09-11: テレポーテーションを追加（`teleport.ts` の `Teleport`、`teleport-fx.ts` の演出の時間軸、`tests/teleport-fx.test.ts`、コントローラの `setFeet`）。原作の画面収録で照準の輪・約 9 m・演出の 4 段階・経路上の回収・4.5 s のクールダウンを確認して合わせた
- 2026-09-11: 操作を原作どおりに変更: Q で位置調整に入る / 調整中の Q でキャンセル / マウスホイールで距離（`adjustDistance`、`player.teleport.wheelStep`）/ 左クリックで確定（以前は E 長押しで照準、離して移動）。スピードブーストを Q から E に移した
- 2026-09-11: 発動の演出をユーザーの参考動画に合わせて置き換えた: 焦点距離が縮み（FOV 75° → 約 137°）ながら赤くなる → 一瞬のホワイトアウト（ここで移動）→ 焦点距離が少し振れ戻りつつ戻り、赤みが消える（約 0.27 s）。暗い赤の暗転・赤いビネット・集中線・`fovKick` をやめ、`TeleportTiming { rush, hold, recover, swing }`・`TeleportFx { glitch, rush, wide, red, white, dolly }`・`fx.focal` にした
- 2026-09-11: ユーザーの指示で演出を 1.25 倍の長さにし（0.27 → 0.3375 s）、焦点距離の変化を強め（`focal` 0.3 → 0.2、`swing` 0.25 → 0.4）、焦点距離の戻りを ease-out + バウンス（`recoverWide`、`TeleportTiming.bounce` 0.4）にした
- 2026-09-11: ユーザーの指示でホワイトアウトのフェードイン・アウトをやめ、hold の間だけ真っ白になる一瞬の明滅にした（`WHITE_IN` / `WHITE_OUT` を削除）
- 2026-09-12: 発動の演出を原作の検証映像（60fps）の実測に合わせて作り直した: rush 0.14 s（画面は中ほどで明るくなり、最後の 2 フレームだけ暗く赤い。タブレットの表示は 1 フレーム早く赤）→ 白は 1 フレーム（`hold` 1/60、移動したフレームは必ず白）→ 0.075 s の暗転（タブレットの赤い表示とランプだけが見える）→ ease-out で明るさと赤が戻る。焦点距離は 1.16 倍への振れ戻りと 0.82 倍の谷の 1 往復だけにした（`bounce` → `dip`）。`TeleportTiming` に `dark` / `dip`、`TeleportFx` に `flare` / `dark` / `screen` を追加
- 2026-09-12: 照準の輪を検証映像に合わせて作り直した（外径約 1.6 m の太い帯と白に近いピンクの芯、筆の弧 110 本の渦、出るときに弧が 0.2 s で一周して描かれる扇形の不透明度マスク `reveal`、輪から浮かぶピンクの火花 `ParticleSystem`）。出るときの拡大と大きさの脈動をやめた
- 2026-09-12: スピードブーストを検証映像に合わせた: ダッシュの速さを超えた分の `boostDash` を追加し、FOV を `camera.fovBoost` 101° まで広げる（`dash` の目標のブースト分 +0.25 はやめた）
- 2026-09-12: テレポーテーションを pak_reference の原作データ（`BP_Power_Teleport` のバイトコード、`CameraAnim_Teleport`、`BP_CameraShake_Streak`）に合わせて作り直した: 距離 `Lerp(2.5, 10, alpha)`（Q のたびに 7 m、ホイールで alpha ±0.1）、真下へのトレースとナビ格子の床（`zone`）、SpringArm のラグで追う輪（`springLag`）、確定から 0.12 s 後のカプセルのスイープ（輪 + 1.25 m。壁・閉じた門で止まる）、移動と同時に照準が消える、クールダウン 5 s（移動から）、照準中の Q は `onCancel`。演出はカメラアニメの FOV・露出・ティントとシェイク（`TeleportFx { fov, ev, tint }`、`shake`）にし、ドリー・放射ズーム・グリッチ・映像から測った時間軸（`TeleportTiming`、`CONFIG.player.teleport.fx`）と、`adjustDistance` / `moveTime` / `totalTime` / `onJump` / 9 本のレイの壁判定を削除した
- 2026-09-12: 照準の輪を原作の構成（床の発光のデカール + `P_ky_cutter2` の床に寝た斬撃のフリップブックと赤い火花）で作り直し、`teleport-ring.ts`（`TeleportRing`）に分けた。映像に合わせて描いていた帯のテクスチャ・扇形の `reveal`・回転・ピンクの火花（`createMarker` / `drawReveal` / `createSparks`、`player.teleport.marker`）を削除し、`player.teleport.ring` にした
- 2026-09-12: ユーザーの指示で残りを原作どおりにした: 床を捉える前のクリックを拒否せず、原作のスポーン位置（50 m 下）へのスイープにした（`spawn`、`SPAWN_DEPTH`）。原作の枠の `Percent` を返す `percent`（`GREY_TIME` 0.05 s）を追加し、音を原作の `Teleport_Mode_Entered`（1.75）と `Teleport_Committed`（1.0）にした。`onDenied` はクールダウン中の Q だけ
- 2026-09-12: ブーストの音を原作のもの（pak_reference の `BP_DD_PlayerCharacter` の E の処理）にした: 発動は `Shard_Streak_Milestone_V5`（1.0。以前は `Stun_Wave_Attack_New_04` を 0.8）、使えないときの `power_not_ready` は 0.35（以前 0.6）。「音」の項を追加した
- 2026-09-12: スピードブーストの演出の原作データ `boost-fx.ts`（`CameraAnim_SpeedBoost` のシーンのティントとブレンド）と `tests/boost-fx.test.ts` を追加した
- 2026-09-12: `boost-fx.ts` に原作のウィジェット `UMG_SpeedBoost`（集中線とビネットの不透明度、フリップブック）と Chameleon（ラジアルブラーの幅、画面の揺れ）の値と関数を追加した
- 2026-09-12: スピードブーストの発動時に原作の `BP_CameraShake_Streak` を掛けるようにした（`streak`、`syncCamera` で `shakeAt` を足す。テレポートの移動と同じシェイク）。`boostDash` の説明から画面の赤い縁と集中線を外した
- 2026-09-12: ブーストの発動音を原作のアセットの設定どおり再生速度 2・音量 0.7 にした（04 記録）
- 2026-09-12: `teleport-fx.ts` の曲線評価 `evaluate` と `keys` を `src/core/ue-curve.ts`（`evaluateCurve` / `curveKeys` / `CurveKey`）に移し、タイトル画面と共用にした（挙動は同じ）。`tests/ue-curve.test.ts` を追加
- 2026-09-12: プレイヤーの設定（タイトルの OPTIONS）を入れる `controls` を追加した: 感度の倍率と Y 反転（原作のマウスの軸 × Mouse Sensitivity）、頭の揺れのオン・オフ（原作の Head Bobbing）、ダッシュの切り替え（`sprintLatch`。原作の Toggle Sprint）、カメラの回転ラグ（`viewYaw` / `viewPitch`、`lagRotation`。原作の SpringArm の CameraRotationLagSpeed 12.5 / 50）。`snapView()` を追加し、`teleport` で呼ぶ
- 2026-09-12: 歩き・走りを原作（pak_reference の `BP_DD_PlayerCharacter` と UE 4.21 の CharacterMovement の既定値）に合わせた: 速さ 3 / 6 m/s（以前 3.6 / 7.2）、加減速は `walk.ts` の `calcVelocity`（以前は指数の追従 14 / 18 と Havok の `calculateMovement` の既定のゲイン 0.05）、後ろ向きでもダッシュできる、ブースト中は歩きも走りも 8.7 m/s（以前は × 1.45）、FOV は速さで 90〜115°（以前はダッシュかどうかで 75 / 90 / 101°）。`walk.ts` と `tests/walk.test.ts` を追加
- 2026-09-12: 頭の揺れを原作の歩き・走りのカメラシェイク（`BP_DD_PlayerCharacter_WalkShake` / `_RunShake` と `Update Bob` の出し入れ）にした（walk.ts の `LoopShake`、`WALK_SHAKE` / `RUN_SHAKE`、コントローラの `bob`・`updateBob`・`stopBob`）。以前の速さに比例する上下・横の揺れとダッシュのロール（`camera.headBobAmplitude` / `sprintRoll`）を削除した
- 2026-09-12: 足音を原作の Tick の Delay（歩き 0.55 s、ダッシュか 650 cm/s 超で 0.35〜0.4 s、ブースト中 0.25 s、横の速さ 0.5 m/s 超の接地で鳴る）にした（walk.ts の `FOOTSTEP` / `footstepDelay`、`stepDelay`、`onFootstep(pitch)`）。以前の歩幅の周期（`bobPhase`、`camera.headBobFrequency`）と使われていなかった `audio.footstepInterval` を削除した
- 2026-09-12: タブレットはカメラシェイクを打ち消さず、カメラと一緒に動くようにした（ユーザーの指示。10 記録）
- 2026-09-12: FOV の追従を 2026-09-11 の原作の収録の速さにした（`FOV_FRAME_RATE` 60 → 100: 8.4/s → 5.0/s。ブーストの後の FOV の戻りが収録で 4.9〜5.0/s）。タブレットを FOV だけで縮めるので（10 記録）、`boostDash` を削除した
- 2026-09-12: ブースト中の画面の揺れを原作の収録に合わせた（ユーザーの指摘: 小刻みな揺れが要る）: `shakeOffset` を 15 rad/s（2.4 Hz）の `(sin φ, cos 1.3φ)` × power から、周波数を Hz とした円 × `SHAKE_REACH`（2/3）にした（全速で 15 Hz、0.002 uv）
- 2026-09-13: 180° ターンを原作の `InpActEvt_180 Turn` にした: 0.16 s のイーズインアウト（`turn`、`camera.turnAroundTime`）をやめ、押下で `yaw ± π`・`pitch = 0` を一度に入れて回転ラグで回す（`turnAround()`。向きはラグの残りの逆側、釣り合いなら右）。ターン中でも押し直せる
- 2026-09-13: `controls.lagSpeed` の既定を 12.5 から原作の SpringArm の既定 20 にした（ユーザーの指摘: 振り返りが本家より遅い。収録の実測も 20 と合う）。原作の Set Up Mouse Smoothing（12.5 / 50）はゲーム中のオプションの SAVE & EXIT からしか呼ばれない（04 記録）
- 2026-09-13: `ue-curve.ts` の `CurveKey` に 4 番目の補間 `Interp`（`'cubic'` / `'linear'` / `'constant'`）を足した（ステージ OP の `loop` の Linear と Constant のキーのため。10 記録）。省けば従来どおり三次エルミート
- 2026-09-14: `CurveKey` に 5 番目の出発の接線を足した（ファンゲームの街灯の Timeline の曲線 `CurveFloat_0_1` / `CurveFloat_0_1_2_3` は到着と出発の接線が違う。07 記録）。省けば到着と同じ
- 2026-09-13: `BP_CameraShake_Streak` を始める処理を公開の `playStreakShake()` にした（シャードの連続回収の節目でも掛けるため。04 記録）
- 2026-09-14: `dash` の平滑化の速さを `CONFIG.post.radialBlur.fadeRate` から controller.ts の定数 `DASH_RATE`（6）へ移した（ダッシュのラジアルブラーを外したため。`dash` は F3 の表示とデバッグ API だけ）
- 2026-09-15: ファンゲーム『Chaotic Customer 2』のしゃがみ・スライド（`crouch.ts`、C、カプセルの差し替え `setCapsule`、立てるかの `headClear`、`onCrouch`、速さの上限としゃがみ中のダッシュなし）とパンチの状態（`punch.ts`: `Hands_up_anim`・Aim の溜め・振りの時間）、一回きりのカメラシェイク（walk.ts の `OneShotShake`・`HIT_SHAKE`・`SIT_DOWN_SHAKE` / `_2`、`ShakeView.roll`、`punchShake`）を足した。カプセルは倒れた柵の板（`FALLEN_PLANK_MASK`）に当たらない。`tests/crouch.test.ts`・`tests/punch.test.ts`
- 2026-09-15: テレポートの照準を地下の床でも出すようにした（ユーザーの指摘。駅のホームで輪が出なかった）: `zone` を `(x, y, z)` にし、当たった点の高さの床の段の格子（game.ts の `FloorGrids`）で判定する
- 2026-09-15: 本作の規範とユーザーの選択で、ファンゲームのパンチをやめた: `punch.ts`・`tests/punch.test.ts`、コントローラの `punchShake`、walk.ts の `HIT_SHAKE` を削除した。カプセルと立てるかの光線が外す形を、倒れた柵の板の `FALLEN_PLANK_MASK`（fences.ts）から壊せる物の飛ぶ板の `PIECE_MASK`（07 記録の breakables.ts。同じ 1 << 4）にした
- 2026-09-15: 本作の規範（プレイヤーの能力は本家に基づく。本家のプレイヤーにしゃがみ・スライドは無い）とユーザーの選択で、ファンゲームのしゃがみ・スライドをやめた: `crouch.ts`・`tests/crouch.test.ts`、コントローラの `crouch`・`onCrouch`・`pressCrouch`・`standUp`・`setCapsule`・`headClear`・`shakes`・C（`KeyC`）・しゃがみの速さの上限としゃがみ中のダッシュなし・しゃがみの目の高さ（目は `CONFIG.camera.eyeHeight`、カプセルの高さは定数）、walk.ts の `SIT_DOWN_SHAKE` / `SIT_DOWN_SHAKE_2` を削除した（`OneShotShake` は walk.ts に残るが使う所は無い）。駅でそれが要った出口の板と低い戸口の格子の壁は 1 クリックで壊れる（07 記録）
- 2026-09-15: 使う所の無くなった一回きりのシェイク（`WaveOscillator`・`OneShotShakeDef`・`OneShotShake`。ファンゲームのしゃがみとパンチのシェイクに使っていた）を walk.ts から削除した
- 2026-09-15: 本家の新しい Steam 版（pak_reference_2）のテレポートは演出が違う（`wtfUE4` と赤い閃光）が、ユーザーの決定で旧版（pak_reference）に従うことを「既知の制約・注意点」に書いた（コードは変わらない）
