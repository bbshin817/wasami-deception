---
title: ワールド：ソウルシャード・特殊シャード・扉
sources:
  - src/world/shards.ts
  - src/world/gate.ts
  - src/world/special-rules.ts
  - src/world/collect-fx.ts
  - src/world/flash-sprites.ts
  - src/world/shard-fx.ts
  - tests/shard-fx.test.ts
  - src/world/specials.ts
  - src/world/interact.ts
  - tests/interact.test.ts
  - tests/specials.test.ts
updated: 2026-09-15
---

# ワールド：ソウルシャード・特殊シャード・扉

## 役割
`shards.ts` はソウルシャード（見た目はワサミ餅のモデル `public/assets/models/wasami_mochi.glb`）を配置・アニメーションし、プレイヤーのカプセルに触れているシャードの判定を提供する（触れると自動回収。取ったシャードはその場で消え、原作の閃光 `P_ky_flash3` を出す）。`shard-fx.ts` は原作の `BP_Shard` の回収の値（音量・揺れ・閃光）、`flash-sprites.ts` は閃光の板。
`gate.ts` は開く出入り口: 左右に開く扉（`slide`: 2 枚の葉。Hotel のエレベーターの扉に使っていた）と、デバッグフィールドの門（`lift`: 上がる）。ステージではいまデバッグフィールドの門だけが使う。開くとコライダーを外し、閉じる・ふさぐと戻す。
イベント発火・効果音・保存は `src/game/game.ts` / `src/game/state.ts` 側の責務で、これらのファイルは見た目と判定だけを持つ。

## 公開インターフェース
### shards.ts
- `class Shards`
  - `constructor(scene, model: AssetContainer, defs: { id, position }[], collected: ReadonlySet<string>)` — `defs` は `Level.shards`（ステージは stage.json の `Shard_001..301`）。
  - `readonly items: Shard[]`、`get meshes()`（影キャスター）、`update(dt)`、`touching(feet)`、`sweeping(from, to)`、`collect(id)`（その場で消して閃光）、`restore(collected)`、`reset()`、`showcase(at | null)`（プリウォームで閃光の板を 1 枚描く）、`dispose()`。

### gate.ts
- `type GateStyle = 'lift' | 'slide'`
- `class Gate`
  - `constructor(node: TransformNode | null, meshes: AbstractMesh[], collider: { body, shape } | null, style: GateStyle = 'lift', time = CONFIG.game.gateOpenTime)` — `time` は開く・閉じるのにかかる秒（`slide` の扉は `game.stage.doorTime` 1.6）。
  - `readonly position: Vector3` — メッシュ群の AABB の中心（y は底面）。`readonly style`。
  - `onShake: (amount) => void` — `lift` の上昇中のカメラ揺れ。
  - `get isOpen`（`progress >= 1`）、`get isMoving`（`progress !== goal`）。
  - `open(instant = false)` — 開き始める（体を `dispose()`。形とノードは保つ）。`instant` なら即座に開いた位置。
  - `close(animate = false)` — 閉じる（`animate` なら `time` かけて戻る、それ以外は即座に）。どちらもすぐに体を作り直す（`block()`）。
  - `block()` — 動かさずに体だけを作り直す（エレベーターに乗った瞬間、開いたままの扉の所で帰り道をふさぐ。原作の BlockingVolume）。
  - `update(dt): boolean` — 動いている間 true（呼び出し側がシャドウを更新する）。

### special-rules.ts（特殊シャードの規則。Babylon 非依存）
原作の `BP_BonusShard`（赤いシャード。取ると敵が 60 s 地図に出る）と `BP_PowerOrb`（取ると敵が 15 s 止まる）の時計（pak_reference の `ExecuteUbergraph_BP_BonusShard` / `_BP_PowerOrb`）。
- `type SpecialKind = 'reveal' | 'stun'`（赤いシャード / オーブ）。
- `SPECIAL`: `flicker { interval 0.1, time 5 }`（原作の `K2_SetTimer('Flicker', 0.1, true)` と `Delay 5.0`）、`reveal { time 60, refresh [0.5, 1] }`（`Delay 60.0` と、`Add To Map` をやり直す `RandomFloatInRange(0.5, 1.0)` の Delay）、`stun { time 15, notice 1 }`（行動ツリーの `Wait 15` と、状態に気づくサービス `BTS_ActorInRange` の `Interval 1`）。
- `type ClockEvent = { type: 'flicker' } | { type: 'move'; from; to }`。
- `class SpecialClock(spawnTime, points, rng = Math.random)`: `point`（立っている出現点。−1 は地図の外 = 原作のレベルが置いた遠くの位置）、`collected`、`get present`（出現点にいて未回収 = 触れて取れる。見え隠れの間も取れる: 原作の SetVisibility はカプセルを残す）、`get visible`（present かつ見え隠れの隠れている 0.1 s でない。最初の 0.1 s は隠れる: 原作の Flicker は最初の呼び出しで隠す）、`get flickering`、`update(dt): ClockEvent[]`、`collect(): boolean`（present のときだけ）、`reset(gone = false)`（地図の外・時計を最初から。`gone` なら取った扱い）、`place(point)`（デバッグ API 用: 今そこへ動いたことにする）。
  - 流れ: 生成（原作の BeginPlay）から `spawnTime` 秒でタイマー（原作の `Spawn Special Shard` / `Spawn Power Orb`）→ `flicker` イベント、5 s 見え隠れ → 出現点を `RandomIntegerInRange(0, n − 1)` で選び（今の点も選ばれうる。原作の「選び済み」の分岐は定数 False で切れている）`move` イベント、タイマーを `spawnTime` で掛け直す（原作の `Timer` → BeginPlay の続き）。以後同じ。最初の見え隠れは地図の外なので見えず、最初に現れるのは `spawnTime + 5` 秒後（ホテルの赤いシャード 105 s、オーブ 155 s）。
- `class RevealClock(rng = Math.random)`: `left`（残り秒、0 で無効）、`get on`、`start()`、`stop()`、`update(dt): 'add' | 'end' | null`（開始直後の最初の `update` と、その後 0.5〜1 s ごとに `'add'`（原作の `Add To Map` の繰り返し。後から来た敵も載る）、60 s で `'end'`（原作の `Remove From Map` と破棄））。
- 検証は `tests/specials.test.ts`（10 件）: 値、地図の外で待って 105 s に現れること・見え隠れの交互・見え隠れ中も取れること、取れるのは出現点にいるときの 1 回だけ・`reset`、地図の表示の 0.5〜1 s の付け直しと 60 s の終わり、敵の硬直 2 件（15 記録）、`GameState.collectSpecial` とセーブの `bonus`（04 記録）、下の collect-fx.ts の 2 件。

### collect-fx.ts（特殊シャードを取ったときの演出の値。Babylon 非依存）
原作の `BP_PowerOrb` / `BP_BonusShard` の取得の処理と、それが出す `BP_StunCollectEffect` / `BP_BonusShardCollectEffect`（pak_reference の `_assets` のタイムラインと PostProcess、`_bytecode` の呼び出しの引数）。
- `COLLECT_TIMELINE`（`Timeline_0`、長さ 2 s）: `float2`（CurveFloat_1。両方のポストプロセスの重み）、オーブだけの `float`（CurveFloat_0。球の半径の Lerp）・`desaturation`（CurveFloat_2）・`opacity`（CurveFloat_3）のキー（時刻・値・Leave の接線〈毎秒〉・補間）。
- `COLLECT_POST`: `gain`（`PostProcess` の ColorGain。stun (1.61, 0.9416, 0)・reveal (1.61, 0, 0.4472)、どちらも ColorSaturation 0）、`midtones` 100 と `fringe` 50（`PostProcess1` の ColorGainMidtones と SceneFringeIntensity。ColorGamma の上書きは既定の 1 のまま）。
- `COLLECT_SPHERE`: `range` 40 m（`Range` 4000 cm）、`color` (0.258, 0.0737, 0)（`M_05_Primal` の Color）。
- `COLLECT_SHAKE`: `01_Hotel_Lobby_ElevatorShakeStop` を `ClientPlayCameraShake` の倍率 25 で。0.5 s、ブレンドアウト 0.5 s（全体で直線的に弱まる）、位置だけ: 前後 2 cm・50、左右 2 cm・35、上下 3 cm・10（UE の「周波数」は rad/s）。
- `COLLECT_SOUND`: オーブ（`Soul_Shard_Pickup_v2_Cue` = `shard_pickup` を 0.8・ピッチ 0.75 × キューのモジュレーター 0.9〜1.1、`stun_countdown` を 1.0 × アセットの Volume 0.35）、赤いシャード（`bonus_pickup` 0.8）、演出の波の音（`stun_wave`: オーブ 1.0・ピッチ 2.0、赤いシャード 0.5・ピッチ 1.5）。UE のピッチは再生速度。
- `collectLook(kind, t): CollectLook` — `grade = clamp(1 − float2)`（原作の Lerp(1, 0, float2)）、`flash = clamp(1 − float2 / 0.3)`（MapRangeClamped(float2, 0, 0.3, 1, 0)。0.3 s ほどで 0）、`gain`、オーブなら `radius = 40 × float`（0 → 1.98 s で 40.6 m）・`opacity = max(0, opacity)`（1 → 0.75 s で 0.63 → 1.92 s で 0）・`desaturation`。2 s の外は全部 0。
- `collectShake(s, phases)` — カメラ空間の位置のずれ（m）: 振幅 × 0.01 × 25 × sin(位相 + 周波数 × s) × (残り時間 / 0.5)。始まりの振幅は前後・左右 0.5 m、上下 0.75 m。`phases` は UE の乱数の初期位相。

### shard-fx.ts（ソウルシャードを取ったときの演出の値。Babylon 非依存）
原作の `BP_Shard` の回収（`Capsule` の `OnComponentBeginOverlap` → `ExecuteUbergraph` の @15〜@1295: DoOnce → タブレットの `ShardCount` を 1 減らして `Count Shake` を速さ 2 で → GameMode の `Check Shards` → `ClientPlayCameraShake` → `SpawnEmitterAtLocation` → `K2_DestroyActor` → `PlaySound2D`）。
- `SHARD_SOUND`: `Soul_Shard_Pickup_v2_Cue` = `shard_pickup` を `PlaySound2D` の 0.65（`Collect(NoSound?)` なら 0 だが、Hotel では誰も呼ばない）。キューの音量は 1、モジュレーターは 0.9〜1.1（本作は 06 記録のピッチ違いのまま）。
- `SHARD_SHAKE` / `shardShake(s)`: `BP_CameraShake_ShardCollect` を倍率 0.4 で。0.1 s、ブレンドイン 0、ブレンドアウト 0.05 s。振幅があるのはロール 1.5° と FOV 3°（どちらも UE の「周波数」15 rad/s、初期位相 0 = `EOO_OffsetZero`。ピッチ・ヨー・位置は周波数だけで振幅 0）。返すのは度数 `{ roll, fov }` = 振幅 × 0.4 × sin(15 s) × 重み（min(1, 残り時間 / 0.05)）。最大はロール約 0.6°・FOV 約 1.2°。UE は取るたびに別のインスタンスを足すので、game.ts は重なった分を足し合わせる。
- `SHARD_FLASH_SCALE` 0.2（`SpawnEmitterAtLocation` の Scale）、`SHARD_FLASH`: `P_ky_flash3` のエミッタを `FlashLayer` にしたもの（倍率 1 の幅 m。StartSize のモジュールは足し合わさる）: glowSub（`MI_ky_flare01_primitiveG`、200 + 500 cm、0.15 s、紫 (0.259, 0.047, 0.757)、不透明度は寿命の 0.2 で 0.79）、core（`primitiveR`、200 cm、0.1 s、(2.94, 2.25, 5)、不透明度は 0.3 で 1.97 だが 1 に抑える、幅は半分へ）、glow（`M_ky_polarGlow02`、350 cm、0.1 s、(6.75, 6.56, 7)、0 から 1.1 倍へ）、shockwave（`M_ky_primitive_dyn2`、600 cm、0.1 s、0.5、0 から広がる）。decoCore（不透明度 0.1 まで）・dust_line（35 本の線）・light（Light モジュール）は、特殊シャードの閃光と同じく省く。0.2 倍では紫の 1.4 m のにじみに青白い 0.4 m の芯。原作のマテリアル（フレアや輪のテクスチャ）の代わりに柔らかい円を使う。
- 検証は `tests/shard-fx.test.ts`（3 件）: 音量、揺れ（0 から始まり 0.05 s で sin(0.75) × 0.6° / 1.2°、ブレンドアウトの半ばで半分、0.1 s で 0、FOV の最大は 1.2° 以下）、閃光の大きさと長さ。
- 使う側: `shards.ts`（閃光）、`src/game/game.ts`（音と揺れ。04 記録）。依存: `./flash-sprites`（型だけ）。

### flash-sprites.ts（原作の粒子の代わりの閃光の板）
- `type RGB`、`interface FlashLayer { size, grow?, aspect?, life, color, alpha, peak }`（層: 倍率 1 での幅 m、寿命での成長の倍率〈生まれ → 終わり〉、縦 / 横、寿命 s、線形 HDR の色、`peak`〈寿命の 0..1〉まで直線で `alpha` へ上がり終わりに 0）。
- `createGlowTexture(scene, name)`: 柔らかい白い円（128 px の DynamicTexture。ランプのグレアの減衰: 中心 1、0.15 で 0.6、0.45 で 0.14、縁で 0）。`additiveMaterial(scene, name, texture, color)`: 照明なし・加算・深度を書かない・両面の `StandardMaterial`（emissive と opacity に同じテクスチャ）。
- `class FlashSprites(scene, name, count, texture)`: `count` 枚のビルボードの板（`{name}{i}`）を使い回す。`flash(layers, at, scale)`（空いた板に層を 1 枚ずつ。足りなければ残りは省く）、`update(dt)`（層ごとに幅 = `size × scale × grow`、高さ = 幅 × `aspect`、`visibility` = 不透明度。寿命で消す）、`clear()`、`showcase(at | null)`（プリウォーム用に 1 枚を出す）。
- `dispose()`: 板とマテリアルを捨てる（テクスチャは使う側のもの）。
- 使う側: `specials.ts`（16 枚）、`shards.ts`（24 枚）。依存: `@babylonjs/core`。

### specials.ts（特殊シャードのシーン）
- `interface SpecialDef { kind, spawnTime, points: Vector3[] }`（`Level.stage.specials`。ステージは赤いシャードとオーブの 4 か所ずつ、間隔 60 s。07 記録）、`interface SpecialItem { kind, clock: SpecialClock, centres, node, halo, reach }`、`FLASHES`（原作の粒子の代わりの閃光の層: `appear(glow, core)`・`disappear`・`impact`）。
- `class SpecialShards(scene, mochi: AssetContainer, orb: AssetContainer, defs)`: `items`、`get meshes`（影キャスター）、`update(dt)`、`touching(feet)`、`sweeping(from, to)`、`collect(item)`、`playSphere(at)`、`reset(redGone)`、`showcase(at | null)`。
  - 見た目（原作の結晶のマテリアルは cook で式が消えているので、色と光り方は推定。`CONFIG.game.special`）: 赤いシャード = 本作のシャードの餅（ユーザーの指定で原作の soul_shard の代わり）を別に読み込んだコンテナから、`game.shard.size × red.size`（0.55 × 2 = 1.1 m。原作は同じ soul_shard を `BP_Shard` が 10 倍・`BP_BonusShard` が 20 倍で描く）、`albedoColor = red.tint`、テクスチャの自己発光 `red.glow`。出現点の `red.height` 1.1 m 上（通常のシャードと同じ）で、通常のシャードと同じ浮き沈み（±0.07 × 2 m、1.7 rad/s）と回転（0.9 rad/s。本作の表現）。オーブ = 原作の `power_orb`（13 記録）を `orb.scale` 0.5408 倍（半径 0.24 m）、PBR（`orb.albedo`、`orb.emissive`、粗さ 0.25、両面）、`orb.height` 1.2525 m 上（原作の部品の 125.25 cm）、動かない（原作の Float 3 = 0）。どちらも加算の光の輪（柔らかい円のテクスチャ、`halo.size` 1.8 m、原作の PointLight の色（FColor は BGRA で保存: (0, 31, 255) = sRGB (255, 31, 0)、(0, 146, 255) = (255, 146, 0)）× `halo.intensity`）を子に持つ。原作の PointLight の代わり（実行時に光源を足すと全マテリアルのシェーダーが組み直されるため）。
  - `update(dt)`: 各時計の `move` で、元の点（−1 でなければ）に `disappear`、新しい点に `appear` の閃光（原作の `SpawnEmitterAtLocation` の倍率: オーブ 0.5、赤いシャード 1）。`clock.visible` でノードを出し入れし（見え隠れ）、位置を今の点の中心へ。閃光の板と球を進める。
  - `touching` / `sweeping`: `present` なアイテムの中心から半径 `reach`（赤いシャード 0.99 m・オーブ 0.45 m。原作のカプセル 49.57 × 0.1 × 20 cm・840.6 × 0.1 × 0.5408 cm。どちらも半高 = 半径の球）がプレイヤーのカプセル（`capsuleRadius` 0.32、`capsuleHeight` 1.8）に届くか（シャードと同じ計算だが、プレイヤーの半径はシャードの `pickupRadius` 0.5 ではなく本作のカプセルの 0.32 m）。
  - `collect(item)`: 時計を取った状態にし、ノードを消して、そこに `impact` の閃光（原作の P_ky_impact / P_ky_impact1、倍率 1）。
  - `playSphere(at)`: `BP_StunCollectEffect` の球（直径 2 の球、`fx/stun-sphere.webp`（13 記録。脱色した T_05_PortalMaps）を自己発光に `COLLECT_SPHERE.color × sphere.intensity`、加算・両面・深度書き込みなし）を `at` に置き、`collectLook('stun', t)` の半径と不透明度で広げる（2 s で消える）。`M_05_Primal` の Desaturation と Color の掛け方は cook で消えているので、脱色した模様 × Color で近似（推定）。
  - 閃光: `FlashSprites`（flash-sprites.ts）の 16 枚の板を使い回す。層ごとに幅（m、× 倍率 × 寿命での成長）、縦横比、寿命、色（線形 HDR）、不透明度（頂点 `peak` まで直線で上がり、寿命の終わりに 0）。光の輪も同じ柔らかい円のテクスチャと `additiveMaterial`。値は原作の `_particles.json` の各エミッタ（appear: glowSub 5 m 0.33 s・core 2 m 0.2 s・glow 3.5 m 0.2 s・shockwave 6 m 0.1 s を品の色で、disappear: flare・glowSub・core、impact: impactSub 6.5 m 0.5 s と flare 8 枚を 3 枚で。disappear / impact の色はパラメータの既定 (2.2, 3, 5)）。原作の塵の線・レンズフレアの連射・Light モジュールは省く（光源の追加を避ける）。
  - `reset(redGone)`: 全アイテムの時計を `reset`（赤いシャードは `redGone` なら取った扱い）、閃光と球を消す。
  - `showcase(at)`: プリウォームで 2 つのアイテム・閃光の板 1 枚・球を `at` の前に出す（null で隠す）。
  - 依存: `../config`、`./collect-fx`（`COLLECT_SPHERE`、`collectLook`）、`./flash-sprites`（`FlashSprites`、`createGlowTexture`、`additiveMaterial`、型）、`./special-rules`（`SpecialClock`）、`@babylonjs/core`。使う側: `src/game/game.ts`（04 記録）、`src/world/level.ts`（`SpecialDef` 型）。

## 内部構造と処理の流れ
### シャード
- 位置の出所: `loadLevel` が stage.json の `shards`（ファンゲームの `shard_C` 301 個。床から 1.1 m に直す。12 記録）を返す。`Shards` は渡された `defs` の数だけ作る（`CONFIG.game.shardCount` 301 は参照しない）。
- 各シャードは `TransformNode('{id}_node')` の子に `model.instantiateModelsToScene(…, { doNotInstantiate: true })` のクローン（ジオメトリとマテリアルは共有）を付け、glTF ルートの `scaling` に `game.shard.size`（0.55）を掛ける。マテリアルは `emissiveTexture = albedoTexture`、`emissiveColor = glow`（0.3）、`maxSimultaneousLights = shadowCasters + 1`。
- 浮遊（`pose(camera)`、`onBeforeRender`）: 画面内（`base` を中心とする半径 0.6 m の球が視錐台に掛かる）のシャードだけ `position.y = base.y + sin(t·1.7)·0.07`、`rotation.y = t·0.9`、`scaling = 1 + sin(t·3.1)·0.035`。画面外は行列を凍結する。
- 回収（`collect`）: 原作の `BP_Shard` どおり、その場でノードを無効にし（原作の `K2_DestroyActor`。2026-09-14 までは本作独自の「膨らんで上がって回る 0.45 s のアニメ」だった）、今の見た目の位置（浮き沈みを含む `node.position`。原作は `SkeletalMesh` の位置）に `SHARD_FLASH` を倍率 `SHARD_FLASH_SCALE` 0.2 で出す。閃光の板は `FlashSprites` の 24 枚（4 層 × 6 個分。テレポートの経路で一度に多く取ると、あふれた分の閃光は省く）。`update` は時計と閃光を進め、`restore` / `reset` は閃光を消す。
- **触れて回収（touching）**: カプセル（半径 `player.capsuleRadius` 0.32、高さ 1.8）の軸上で `base.y` に最も近い点と `base` の距離が `player.pickupRadius + game.shard.reach`（0.5 + 0.4957 = 0.9957 m）以下。原作の重なりの判定: `BP_Shard` の `Capsule`（半径・半高 49.57 cm、`RelativeScale3D` 0.1）は `SkeletalMesh`（高さ 97.09 cm、拡大 10）に付くので、世界では半径 49.57 cm の球が根元（床）から約 1.0 m にあり、`OnComponentBeginOverlap` でプレイヤー（`BP_DD_PlayerCharacter` の `CollisionCylinder`、半径 50 cm・半高は ACharacter の既定 88 cm）と重なると取れる。本作の物理のカプセルは細い（0.32 m）ので、判定の半径だけ原作の 0.5 m を使う（2026-09-14 まではカプセル 0.32 + 0.3 = 0.62 m で、原作より 0.38 m 近づかないと取れなかった）。**経路上の回収（sweeping）**: テレポートの移動の線分上で同じ判定（05 記録）。

### 扉（Gate）
- **位置**: `node` の絶対座標を初期値に、`meshes` の AABB があればその中心（y は底面）。
- **`slide`**（ホテル）: 葉は広がりの大きい軸（x か z）に並ぶので、各葉を AABB の中心が扉の中心のどちら側にあるかでその向きへ、その葉の幅 × 0.95 だけ開く（`open` の変位を持つ）。`apply(p)` は smoothstep `p²(3 − 2p)` で各葉を `setAbsolutePosition(base + 変位 × e)`（葉の親の変換にかかわらず世界座標で動かす）。
- **`lift`**（デバッグフィールド）: `node.position.y = baseY + eased × game.gateLift`（3.8 m）。`eased` は最初の 15 % で 5 % だけ動く重い出だしと cubic ease-out。開く間 `progress < 0.9` なら `onShake(0.012 × (1 − progress))`。
- **動き**: `goal`（1 開く / 0 閉じる）へ `dt / time` ずつ。`update` は動いた間 true を返し、game.ts が影マップを描き直す。
- **コライダー**: 開き始めた瞬間に体を捨て、`close` / `block` で同じ形とノードに体を作り直す（ANIMATED）。

### 回収・脱出のイベント（game.ts 側。04 記録）
- `shardCollected` → `shards.collect(id)`（消えて閃光）、音（0.65）、揺れ、タブレット、字幕、セーブ。全回収で `allShardsCollected`（ステージはシャードの障壁が壊れる。04 記録）。
- デバッグフィールドは全回収で門の前で配電盤を使った扱い（脱出）にして門が開き、出口で脱出。

## 依存関係
- shards.ts: `../config`（`player.capsuleRadius` / `capsuleHeight` / `pickupRadius`、`game.shard`、`lights.shadowCasters`）、`./flash-sprites`（`FlashSprites`、`createGlowTexture`）、`./shard-fx`（`SHARD_FLASH`、`SHARD_FLASH_SCALE`）。
- gate.ts: `../config`（`game.gateOpenTime`、`game.gateLift`）、`@babylonjs/core`（`PhysicsBody`、`PhysicsMotionType`、`Vector3`、型）。
- 使う側: `src/game/game.ts`（`Shards`、`SpecialShards`、デバッグフィールドの `gate`）。`Level` の各欄は `src/world/level.ts` が供給（07 記録）。

## 設定・調整値
- `player.capsuleRadius` 0.32、`player.capsuleHeight` 1.8、`player.pickupRadius` 0.5（シャードの判定だけに使う原作のプレイヤーの半径。特殊シャードと秘密は `capsuleRadius`）。
- `game.shardCount` 301（表示用。実際の個数は stage.json の数）、`game.shard = { model, size: 0.55, glow: 0.3, reach: 0.4957 }`、`game.gateOpenTime` 4.2 / `game.gateLift` 3.8（デバッグフィールドの門）、`game.stage.doorTime` 1.6（`slide` の扉）。
- アニメ係数、`LAYERS` はコード内の定数。

## 既知の制約・注意点
- `touching` は浮遊を除いた `base` で判定する。
- 扉のコライダーは開き始めた瞬間に消えるので、開き始めは見た目より早く通れる。閉じるときは閉じ始めた瞬間に戻る。
- `slide` の葉は `setAbsolutePosition` で動かすので、開閉の間は葉の world matrix を毎フレーム計算する（凍結しない）。
- ポータルは影を落とさず、ライトマップにも入らない。ロゴの向きは法線まわりの回転を持たない（`FromUnitVectors` の最小回転）。
- 301 個はすべてクローン（ジオメトリとマテリアルは共有）で、見えている数だけ描画呼び出しが増える（画面外は凍結）。`touching` は毎フレーム全シャードを線形に調べる。
- 隠し扉と祭壇は原作どおり視線の先 200 cm に入れて使う（原作の Use の代わりに左クリック。E がブーストのため）。原作のトレースは当たった部品の衝突形状に対してだが、本作は扉の当たりの箱と、祭壇の像の境界の箱に対して調べ、壁越しかは Havok のレイで見る。扉の箱はナビの格子に残すので、敵は開いた後も部屋に入らない（原作の敵が秘密の部屋に入れないという記述は参考資料の [C] で、原作データでは確かめていない）。扉の状態は保存しない（原作も BeginPlay で閉じている）。
- 秘密の Fresnel の発光（式が cook で消えている）と、Packed を sRGB から戻して読む扱いは推定。原作の Interact Widget（視線が扉に当たると出る印）は出さない。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）。見た目をワサミ餅に、触れて回収に、経路上の回収 `sweeping`、`restore`
- 2026-09-12: 画面外のシャードの行列を凍結（`pose` / `freeze` / `thaw`）
- 2026-09-13: ゲームオーバーの RESTART のため `Shards.reset()` と `Gate.close()` を足した
- 2026-09-13: 原作の Hotel に置き換えた（ユーザーの指示）: `Gate` に左右に開く `slide`（エレベーターの扉）、開閉の時間、`close(animate)`、`block()` を足した。ロビーのポータル `portal.ts` を追加した。シャードは原作の 289 個
- 2026-09-13: 原作の特殊シャードの規則 `special-rules.ts`（`SpecialClock`・`RevealClock`・`SPECIAL`）、取得の演出の値 `collect-fx.ts`（`collectLook`・`collectShake`）と `tests/specials.test.ts` を追加した
- 2026-09-13: 特殊シャードのシーン `specials.ts`（赤い餅とオーブ、光の輪、見え隠れ、原作の粒子の代わりの閃光、硬直の球）を追加した
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 にした（ユーザーの指示）: シャードは stage.json の 301 個、特殊シャードの定義は `Level.stage.specials`。`tests/specials.test.ts` と `tests/secrets.test.ts` はセーブの version 3（`checkpoint`）で古いセーブを作るようにした（検査の中身は変わらない）
- 2026-09-14: 原作の Hotel の秘密（`BP_Collectable` 2 個）と隠し扉（`BP_Openabledoor` 2 枚）を追加した: 規則 `secret-rules.ts`（Bounce・箱・灯・音・Fresnel、扉の開き・音・台詞、箱とカプセルの重なり）、シーン `secrets.ts`（`Secrets`: 原作の `secret_file`、Fresnel の発光のプラグイン、近くのメッシュに限った点光源、`SecretDoors`: 蝶番の回転とコライダー）、`tests/secrets.test.ts`
- 2026-09-14: 原作の視線の値と式 `world/interact.ts`（`INTERACT`・`ALTAR_USE`・`rayBoxDistance`・`Usable`）と `tests/interact.test.ts` を足した。`SecretDoors.touching` と `SECRET_DOOR.reach` を `shut()` に替えた（隠し扉は視線と左クリックで開ける。04 記録）
- 2026-09-14: シャードの当たり判定を原作の重なりにした（ユーザーの指摘「ややシビア」）: カプセルの軸から 0.32 + `touchRadius` 0.3 = 0.62 m を、原作の `BP_Shard` の球（49.57 cm）とプレイヤーの半径（50 cm）の和 0.9957 m（`player.pickupRadius` + `game.shard.reach`）にした
- 2026-09-14: 特殊シャードの閃光の板（`FlashLayer`、板のプール、柔らかい円のテクスチャ、加算のマテリアル）を `flash-sprites.ts`（`FlashSprites`、`createGlowTexture`、`additiveMaterial`）に切り出した（ソウルシャードの回収の閃光でも使うため。見た目は変えていない）
- 2026-09-14: ソウルシャードを取ったときの挙動を原作の `BP_Shard` に寄せた（ユーザーの指示）: 本作独自の回収アニメ（`anim`）をやめてその場で消し、`P_ky_flash3` の閃光を出す。原作の値の `shard-fx.ts`（`SHARD_SOUND`・`SHARD_SHAKE`・`shardShake`・`SHARD_FLASH`・`SHARD_FLASH_SCALE`）と `tests/shard-fx.test.ts`、`Shards.showcase`、`FlashSprites.dispose` を足した（音量と揺れは game.ts。04 記録）
- 2026-09-14: ユーザーの指示（アセットは白塗りにしてエンジン内で着色する）でポータルのロゴを白のテクスチャにし、`LAYERS` の `glow` を `tint` に替えて材質で塗るようにした（ロゴは原作の `portal_monkey` の sRGB 192 の赤、渦と輪は元の色のまま）。あわせて、発光テクスチャが発光色（1.2〜1.6）に足されて全層が白く飛んでいたのを、拡散テクスチャ × 発光色の乗算に直し、KTX2 で上下が逆さだった絵を `vScale = −1` で正立させた
- 2026-09-14: Hotel を削除した（ユーザーの指示）: ポータル `portal.ts`、秘密と隠し扉 `secret-rules.ts` / `secrets.ts` と `tests/secrets.test.ts`、interact.ts の祭壇の箱 `ALTAR_USE` と `Usable` 型を消した（`interact.test.ts` は視線の値と箱の当たりだけに）
