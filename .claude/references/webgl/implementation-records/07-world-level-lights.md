---
title: ワールド：レベル読み込み・デバッグフィールド・ランプ照明
sources:
  - src/world/level.ts
  - src/world/lights.ts
  - src/world/stage-lamps.ts
  - tests/stage-lamps.test.ts
  - src/world/trap-fx.ts
  - src/world/door-parts.ts
  - src/world/debug-field.ts
  - src/world/carpet.ts
  - src/world/breakables.ts
  - src/world/woodboards.ts
  - tests/woodboards.test.ts
updated: 2026-09-15
---

# ワールド：レベル読み込み・デバッグフィールド・ランプ照明

## 役割
`level.ts` はステージ（ファンゲーム『Chaotic Customer 2』の Zone_1。地上の迷路と地下鉄の駅と階段。12 記録）の glTF（`assets/level/level.gltf`）を AssetStore 経由で読み込み、KTX2 の PBR マテリアルの調整・TEXCOORD_1 のベイク済みライトマップのページ・反射キャプチャと SkyLight の全天球・Havok の静的コライダー（`colliders.json` の箱と階段の坂のメッシュ）を組み立て、ゲーム側が使う `Level` 構造体（ステージの部品 `stage` を含む）を返す。2026-09-13〜14 は原作の Hotel だった（git の履歴にある）。
`debug-field.ts` は `?debug.field=true` のときにステージの代わりに使う検証用の広場（デバッグフィールド）を実行時にコードで組み立て、同じ `Level` を返す（`stage` は null）。
`carpet.ts` は足音の床の判定（足元が絨毯か）のために、読み込み時に絨毯の平らな三角形を地面の格子に振り分けておく（ステージに絨毯は無い）。
`lights.ts` はランプ（ステージでは 301 灯。街灯・点光源・矩形・スポット）ごとの発光面を管理し（灯のまわりのにじみはポストプロセスの UE のブルーム。09 記録）、寄与の大きい 3 灯だけに影付き `PointLight` をプールから割り当てる（そのランプの色と範囲で）。影を描くメッシュはライトの範囲に届くものだけに絞る。
`woodboards.ts` は原作の Hotel の板張りのバリケード（pak_reference の `BP_01_Woodboards`）の値と時間軸（Babylon 非依存）、`breakables.ts` はそれに倣って壊れるステージの壊せる物（駅の柵 `Fence_battle` / `Fence_battle_2` の区画と、駅の板張りの通り道 2 か所: ホームの出口に打ち付けた板と、低い戸口に掛かる格子の壁）。本作の規範（本家に無いギミックは複雑にせず、壊せる物は本家の板張りのバリケードのように 1 クリックで崩れて自然に消える）により、ファンゲームのパンチで板が 1 枚ずつ落ちる柵をやめ、区画ごとに 1 クリック（か敵が触れる）で壊れるようにした。板張りの通り道はファンゲームがしゃがみ・スライドでくぐらせる所で、本作にしゃがみは無い（05 記録）ので、ユーザーの選択でまるごと 1 クリックで壊れるようにした。game.ts はステージのロックピックとダッシュの障壁にも同じ時間軸（`WOODBOARDS_GONE`・`woodboardsOpacity`）と、breakables.ts の煙（`Breakables.smoke`）・敵の重なりの判定（`touches`）・プリウォーム（`warmFade`）を使う（04 記録）。

## 公開インターフェース
### level.ts
- `interface LevelMeta` — `level-meta.json` の型（game.ts が `fetch` して渡す。取得失敗時は `{}`）: `lightmapGamma?`、`lightmapPages?: { page, lightmapScale, lightmap?, indirectLightmap? }[]`（ページごとの PNG の倍率）、`lightmapOf?: Record<名前, ページ>`（タイルと焼いた動く部品）、`lamps?: { name, source?, actor?, owner?, kind?: 'point' | 'spot' | 'rect', position, direction?, color?, candela?, power, radius?, castShadows?, glow? }[]`（Blender の m、sRGB の色、cd と W、減衰半径 m、`glow` はその灯の発光面の動く部品の名前）、`captures?`（反射キャプチャ: `name`, `source`, `kind: 'box' | 'sphere'`, `position`, `extent?` / `radius?`, `file`, `average?`）、`captureOf?: Record<名前, 番号>`（−1 は SkyLight の全天球）、`sky?: { file?, average? } | null`（SkyLight の全天球 `captures/Sky.hdr`）、`floors?: { maze, metro }`。
- `interface ColliderData { floor; wall; ramps?: { positions, indices }; overhead?: { positions, indices }; waypoints?; enemySpawns?; route?: [x, y, z][] }`（`colliders.json`。箱は Blender の m で `[minX, minY, minZ, maxX, maxY, maxZ]`、坂は Blender の m の三角形、`overhead` は立ったプレイヤーが下に入れない地下の床の上に掛かる物の三角形〈そこの天井〉、`route` は開始地点からホールのトリガーまでの経路の点〈z は床の高さ〉。12 記録の `scripts/cc2/colliders.mjs`）。
- `interface StageJson`（`stage.json`。12 記録の layout.py）: `floors`、`start`・`checkpoints`（`{ zone: 'maze' | 'metro', position, look }`）、`shards`（`{ id, source, position }`）、`enemySpawns`、`mannequinRespawns`、`specials`（`reveal` / `stun`: `{ source, interval, duration?, points }`）、`triggers: Record<TriggerName, { min, max }>`、`panel`、`barriers`（`{ source, kind: BarrierKind, corners, centre, zone, root, widget? }`。`root` と `widget` は型に残るが `loadLevel` は読まない）、`fences?`（`{ source, class, root, sections: { comp, box, centre, flip, gate }[] }`。12 記録。stage.json に残る床の案内の文字 `tutorial` は型に無く読まない）、`boards?`（`{ name, actors }[]`。板張りの通り道〈layout.py の `BOARD_GROUPS`〉ごとのアクター。その部品は役割 `boards`）、`doors`、`hiddenWalls?`。`mannequinRespawns` は型にあるが、敵のリスポーンをやめたので `loadLevel` は読まない。
- `type TriggerName = 'chase' | 'endChase' | 'stairsTop' | 'stairsSide' | 'goal'`、`type BarrierKind = 'shard' | 'pick' | 'dash'`（ファンゲームの `barrier_C` / `Interactivate_barrier` / `barrierspeedbust`）。
- `interface Box { min: Vector3; max: Vector3 }`、`insideBox(b, p, pad = 0)`（x / z は `pad` 付き、y は ±1 m の余裕）、`boxCentre(b)`。
- `interface Lamp { index, position, glow: AbstractMesh[], power, color: Color3（線形）, range（m）, source?, owner?, lit? }` — `power` は 2.4 cd の灯（Hotel の天井灯。`lights.intensity` がこれに合わせてある）に対する比、`owner` は layout.py の持ち主（`'lamp'`・`'trapDoor'` など、無ければ null）、`lit` は発光面が照らされる面に発光部分があるもの（街灯）。
- `interface Collider { body, shape }`、`interface SlidingDoors { node, meshes, collider }`。
- `interface BarrierDef { source, kind, meshes, box: Box, centre, zone: Box }`（障壁の板と、その少し厚くした箱。`zone` はファンゲームの `Box` の AABB〈stage.json の `zone`。シャードの障壁はチェックポイント 1 でプレイヤーが初めて触れると立つ。04 記録〉。ロックピックとダッシュの障壁は game.ts が箱 `box` の視線と敵の重なりで壊す）、`interface StagePart { role, actor, meshes, nodes }`（動く部品。layout.py の役割ごと。`nodes` は部品ごとのオブジェクト〈部品の原点〉のノード）、`interface JetDef { source, position, trigger: Box, jet: Box }`（床の噴出 `Pair_trap`: 立つと噴く `Box1` と殺す `Box`）、`interface CarDef { source, kill: Box, horn: Box, unit: Vector3, parked, part }`（車 `car_trap`: 走り出しの殺す箱とクラクションの箱、Box の相対 y の 1 単位の世界の移動〈m〉、脱出で止まる相対 y、役割 `car` の部品）、`interface DoorDef { source, kind: DoorKind, origin, axes, part, sideA?, sideB?, plate?, spawn? }`（扉: アクターの局所空間の点 p〈UE の cm〉が `origin + Σ axes[i] × p[i]` にある枠、動く部品、開き戸の両側の箱、罠の扉の手前の板と出てくる位置）。
- 駅の柵（ファンゲームの `Fence_battle`）: `interface FenceSection { comp, box: Box, centre, flip: string[][], gate: string[][] }`（区画〈`Cube` / `Cube1`〉の当たりの箱と中心、ファンゲームのパンチが落とす板の名前〈`flip`・`gate` の各項目は部品名の候補の列〉。本作は区画をまとめて壊すので、`flip` と `gate` は区画の板の一覧としてだけ使う: breakables.ts）、`interface FenceDef { source, sections, planks: Map<部品名, TransformNode>, part: StagePart | null }`（板は役割 `fence` の動く部品のノードを部品名〈Fence2・4〜10〉で引く）。
- 駅の板張りの通り道: `interface BoardsDef { name, parts: StagePart[] }`（stage.json の `boards` のグループ〈`doorway`・`wayOut`〉と、役割 `boards` の部品のうちアクターがそのグループのもの。breakables.ts がグループをまるごと 1 つの壊せる物にする）。
- `doorPoint(d, p)`（扉の局所の点をワールドへ）、`doorBox(d, { c, h })`（局所の箱〈中心と半分、cm〉を 8 隅からワールドの AABB に。枠は直角にしか回らない）。
- `interface StageParts { checkpoints: Record<1 | 2 | 4, { position, yaw, zone }>, triggers: Record<TriggerName, Box>, panel, barriers: BarrierDef[], fences: FenceDef[], boards: BoardsDef[], parts: Record<役割, StagePart[]>, specials: SpecialDef[], stunTime, floors, jets: JetDef[], cars: CarDef[], doors: DoorDef[], escape: EscapeDef | null }`（`EscapeDef { first: { path, time }, paths: Record<名前, Path>, booms: Record<名前, Box>, trucks: { box, priority, path, time, flip }[], more: { box, spawns }[], walls: Box[], crowd: Vector3[], truck: StagePart | null }`: 脱出のトラックの道〈ファンゲームのスプラインを長さに沿って標本化〉と終わりのブームの点、最初のトラック、トラックを出す箱と群れを足す箱、不可視の壁、群れの最初の出現点、トラックの部品〈最初の道の始まりに置いてある〉） — `stunTime` はオーブの硬直（`stun_orb` の 17 s）。
- `interface Level` — `staticMeshes`, `playerStart`, `playerYaw`, `lamps: Lamp[]`, `shards: { id, position }[]`, `gate`（デバッグフィールドの門。ステージは null）, `gateMeshes`, `gateCollider`, `gateStyle: 'lift' | 'slide'`, `exit`（ステージはゴールの箱の中心）, `dynamicMeshes`（動く部品すべて）, `flameMaterialName`, `navBoxes: NavBox[]`（床は `floor: true`。記録 15）, `enemySpawns`, `waypoints`（`colliders.json` の床に載せた出現点と巡回点）, `route: { at: Vector3 }[]`（`colliders.json` の経路。`scripts/flow-check.mjs` がデバッグ API の `stage().route` で歩く。デバッグフィールドは空）, `stage: StageParts | null`。
- `async loadLevel(scene, store: AssetStore, meta: LevelMeta): Promise<Level>`。
- `SHARD_HEIGHT` 1.1（シャードの床からの高さ m。ファンゲームのシャードの根は 0.7 m 上）。
- `partCollider(scene, name, box): Collider` — 動く部品の ANIMATED の箱の体（game.ts が障壁に、breakables.ts が壊せる物の区画と板張りの通り道に使う）。
- `applyEnvironment(scene, store)` — HDR 環境の適用（下記）。`buildDebugField` と、キャプチャが無いときの `loadLevel` が呼ぶ。
- 非公開: `applyCaptures(...)`（反射キャプチャと SkyLight。下記の 9）、`class CaptureMixing extends MaterialPluginBase`（UE のライトマップ混合）、`class HybridLamps extends MaterialPluginBase`（'hybrid' の灯と焼き込みとの差分）。
- `interface Lamp` の `down` — 下向きの矩形の灯（街灯。meta の `kind` が `'rect'` で `direction` の z が −0.9 未満）。

### debug-field.ts
- `buildDebugField(scene, store: AssetStore): Level` — デバッグフィールドを組み立てて返す（同期。`store` は HDR 環境にだけ使う）。`gateStyle: 'lift'`、`route: []`、`stage: null`、灯は `CONFIG.lights.color` と `range`。`src/game/game.ts` が `CONFIG.debug.field` のときに `loadLevel` の代わりに呼ぶ。

### carpet.ts
- `class CarpetMap(meshes: AbstractMesh[])` — `src/game/game.ts` の `Game` のコンストラクタが `level.staticMeshes` から作る。
  - `under(feet: Vector3): boolean` — 足元に絨毯があるか。`Game.footstep` が足音の種類（`fs_carpet_` / `fs_hard_`）を選ぶのに使う。
- 作り方: 各メッシュのサブメッシュのうちマテリアル名が `/Carpet|Rug|Velvet/i` のものについて、三角形をワールド座標に直し、面がほぼ水平（`|法線.y| >= 0.7`。巻き方向は問わない。glTF のルートがシーンを鏡映しにするため）なものだけを残す。各三角形の XZ の範囲が重なる 1 m 四方のマス（`CELL`）すべてに登録する。
- `under(feet)`: 足元のマスの三角形だけを調べる。XZ への投影に足元が入るか（重心座標）を見て、入っていればその点の高さ `h` を求め、`feet.y − 0.7 <= h <= feet.y + 0.3` なら絨毯。
- ステージとデバッグフィールドは絨毯のマテリアルがないので、どこでも `false`（硬い床の足音）。

### lights.ts
- `class LampSystem`
  - `constructor(scene, lamps: readonly Lamp[], casters: AbstractMesh[])`
  - `addCaster(mesh)`、`refreshShadows()`、`setQuality(quality)`、`shadowsOn`、`update(dt, viewer)`、`activeLampIndices`、`dispose()`。
  - `setState(index, scale, tint: { r, g, b } | null = null)` — そのランプ（`index` 番）の状態: 光と発光面を `scale` 倍（0 で消灯）、`tint`（線形）があればランプの色の代わりにその色。焼き込みはランプの 1 倍・元の色。同じ状態なら何もしない。
- 非公開: `class PoolLight extends PointLight`（スロットのライト。'hybrid' の差分の係数 `delta`・`down` をシェーダーへ送る。下記）。

### stage-lamps.ts
- `interface LampStates { setState(index, scale, tint?) }`（`LampSystem` が満たす）、`interface StageLamp { index, source?, owner? }`、`interface Rgb { r, g, b }`。
- 定数（ファンゲームの `Content_game/Blueprints/Lamp_zone_1` と `Light_retro_breaking`）: `STREET_CD` 3.2（街灯の RectLight の cd。曲線の単位）、`FLICKER`（Timeline_0 の `CurveFloat_0_1`: 0 s 3.2・0.1 s 1.0・0.2 s 3.2・0.3 s 1.0・0.4 s 3.2 の三次、到着と出発の接線つき。`FLICKER_LENGTH` 0.4 s のループ）、`RECOVER`（Timeline_1 の `CurveFloat_0_1_2_3`: 3.2 → 0.4 s で 0 → 2 s で 3.2。`RECOVER_LENGTH` 2）、`ESCAPE_DARK` 1.5（`Escape_from_zone2` の Delay）、`CYCLE_LENGTH` 3、`RETRO_PATTERN`（消 0.2・点 0.45・消 0.1・点 0.1・消 0.1・点 0.2・消 0.2・点 0.5 s）。
- `cycleColor(t, out?)` — 脱出の色（Timeline_2 の `CurveLinearColor_0`: 0 / 1 / 1.993 / 2.993 s の線形、3 s のループ。最後のキーからループの終わりまでは保つ）。`retroLit(t)` — BeginPlay から `t` s でレトロな灯が点いているか（消から始まる）。
- `class StageLamps(pool: LampStates, lamps: readonly StageLamp[])` — `owner` が `'lamp'` の灯を街灯（48）、`'retroLamp'` をレトロな灯（7）に、`source` のアクター名ごとにまとめる。
  - `reset(checkpoint)` — 灯のアクターの BeginPlay: 点滅と回復を止め、回復を未再生に、時計を 0 に。チェックポイント 4 は街灯を消して脱出の色へ（`Escape_from_zone1` → `Escape_from_zone2`）、それ以外は 1。
  - `setActor(actor, on)` — そのアクターの灯（障壁の 2 灯・罠の扉の点光源）を 1 か 0 に。
  - `update(dt, chasing, allShards)` — レトロな灯を `retroLit` で点け消し（変わった時だけ）。脱出中は `ESCAPE_DARK` 後に全街灯を `cycleColor` の色・1 倍に。それ以外は `chasing` の立ち上がり（全回収でなければ）で点滅を最初から（`Activate_lamP-break`）、立ち下がりで点滅を止めて回復を再生（`DeActivate_lamP-break`。回復を一度最後まで再生した後は、UE のタイムラインが終端から `Play` しても最後の値だけなので、すぐ 1）。全回収なら点滅も回復も止めて 1（`Deacticate_after_complete`）。点滅と回復の値 / `STREET_CD` を全街灯の `scale` に。
  - `mode`（`'flicker' | 'recover' | 'escape' | 'steady'`。デバッグ API の `stage().streetLamps`）、`streetCount`。

### trap-fx.ts
- `createJetSmoke(scene, def: JetDef, texture): ParticleSystem` — 床の噴出の煙（ファンゲームの `Pair_trap` の Niagara: CPU のエミッタ "Fountain"、ワールド空間、カメラを向く `Smoke_2` のスプライト、`Scale Alpha` で消える）。Niagara の率・速さ・寿命・大きさは VM の定数に焼き込まれていて読めないので、殺す箱の長さの水平の噴流にした: 根（`def.position`）から ±5 cm の箱で出し、向きは根から `jet` の箱の中心への水平の向き（左右と上下に ±0.12 のばらつき）、長さは中心までの 2 倍（Box が根から 1〜285 cm）、寿命 `LIFE` 0.45 s（0.8〜1.1 倍）で長さを進む速さ、率 160/s、大きさ 0.25〜0.45 m を寿命で 0.3 → 1.2 倍、色 (0.95, 0.95, 0.95, 0.5) / (0.85, 0.86, 0.88, 0.35) → 透明、重力 +0.6 m/s²（湯気は少し昇る）、`BLENDMODE_STANDARD`。作った時は止めておき、game.ts の `updateTraps` が噴く間だけ `start`（04 記録）。テクスチャは `flash-sprites.ts` の `createGlowTexture`（08 記録）。

### door-parts.ts
- `class DoorParts(scene, door: DoorDef, moving: string | null)` — 扉の動く部品（`moving` の部品名で終わる `door.part.nodes`: 葉 `Modul_wall_Door`・蓋 `Vent_door`・シャッター `Big_door_zone_1`・腕 `Blocker_Cube_001`）を蝶番のノード `hinge` の下に `setParent`（ワールドの変換を保つ）。枠 W（局所の cm → Babylon のワールド、行ベクトルの行列: 行 0..2 が `axes`、行 3 が `origin`）とその逆を持つ。
  - `setPose(pose: PartPose)` — 局所の姿勢 P = T(−pivot) · R · T(pivot + move)（R の行は UE の回転子 `ueRotation` で回した局所の軸）を D = W⁻¹ · P · W にして、`hinge` の位置・四元数・拡縮に分解する（静止で単位。鏡映の枠〈ブロッカーの根の拡縮 (−1, 1, 1)〉でも剛体の変換になる）。
  - `collider(name, box: LocalBox, moving)` — 局所の箱の `doorBox` の中心にノードを置き（動くものは `hinge` の下に）、その大きさの `PhysicsShapeBox` を持つ `DoorCollider` を返す。
- `boxCollider(scene, name, box)` — ワールドの箱の動かない `DoorCollider`（脱出の不可視の壁）。
- `class DoorCollider` — `set(on)`: 体を作る（ANIMATED、保った形。`hinge` の下のものは `disablePreStep = false` で毎ステップノードに従う）か `dispose`（障壁の `solid` と同じ）。`on`。

### woodboards.ts（原作の板張りのバリケード。Babylon 非依存、`tests/woodboards.test.ts`）
- `WOODBOARDS { hold 2, fade 2, push 0.6, lift 0.3, mass 6 }` — `hold` は壊れてから薄れ始めるまでの `Delay(2.0)`（@512）、`fade` は `Timeline_0` の長さ（Opacity 1 → 0）の秒。`push`（区画の中心から外への水平の m/s）・`lift`（上への m/s）・`mass`（板 1 枚の kg）は推定（下の「既知の制約・注意点」）。
- `WOODBOARDS_GONE` = `hold + fade` = 4（壊れてから原作が `K2_DestroyActor` するまでの秒）。
- `BOARDS_BREAK: StageCue` = `{ ids: ['boards_break'], volume: 0.6, attenuation: { inner: 4, falloff: 36, natural: true } }`（`Wooden_Boards_Breaking_v5` をアクターの位置で: 呼び出しの 1.0 × 波形の Volume 0.6、減衰 `MonkeyAttenuation` は NaturalSound で形を上書きしていないので UE の既定の 400 cm の球と 3600 cm の減衰。06 記録の `cue` が鳴らす）。
- `woodboardsOpacity(t)` — 壊れて `t` s の板の不透明度: `hold` までは 1、その後 `u = min(1, (t − 2) / 2)` で `1 − u²(3 − 2u)`（`Timeline_0` の `CurveFloat_0`: 0 s のキーが接線 0 の三次で 2 s のキーへ）、4 s 以降は 0。
- `SMOKE { scale 1.5, count 3, life [2, 3], size [0.7, 1.1], radius 0.1, rise [0.1, 0.25], spin 0.02, grid 8, light 0.15, saturate 255 / 179 }`（StarterContent の `P_Explosion1` をバリケードが 1.5 倍で出したもの: スプライトのエミッタ "Smoke" 1 つ、バースト 3〈EmitterDuration 0.5、1 回〉、Lifetime 2〜3 s、StartSize 70〜110 cm、LocationPrimitiveSphere 10 cm、StartVelocity 上へ 10〜25 cm/s、StartRotation 0〜1 回転と RotationRate ±0.02 回転/s、SubUV 8 × 8。`size`・`radius`・`rise` は倍率の前。`light` は推定。`saturate` は UE が基本色〈テクスチャ × ColorOverLife〉を 1 で切ることの代わりで、`T_Smoke_SubUV` の平均の灰 0.70〈179 / 255〉に対する色の上限 1 / 0.70）。
- `smokeAt(r)` → `{ size, color, alpha, cell }` — 寿命の割合 `r`（0..1 に切る）の煙のスプライト: 大きさの倍率（SizeMultiplyLife）、灰の明るさ（ColorOverLife を `saturate` で頭打ちにして × `light`）、α（AlphaOverLife）、SubUV のコマ `min(63, floor(r × 64))`。分布は UE の FRawDistribution の焼いた表（値を一定の間隔で並べ、間は線形、最後の値で保つ）: SizeMultiplyLife は 1/35 ごとの 8 標本（0.1, 0.7542, 1.2391, 1.5797, 1.8011, 1.9283, 1.9862, 2.0）、ColorOverLife は 0.1 ごとの [10, 0.1]（寿命の 1/10 で 10 から 0.1 へ）、AlphaOverLife は 1/31 ごとの 32 標本（0 から 4/31 で最大 0.9677、そこから 0 へ）。

### breakables.ts
- `PIECE_MASK` = 1 << 4（壊れて飛ぶ板の形の membership。プレイヤーのカプセルが外す。05 記録。原作が壊れたバリケードの当たりを Visibility にする @200 の代わり）。
- `class Breakables(scene, fences: readonly FenceDef[], boards: readonly BoardsDef[] = [])` — `onBreak(at)`（壊れたとき、グループの中心で。game.ts が `BOARDS_BREAK` を鳴らす）、`targets()`（立っているものの `{ index, box }`。game.ts の視線の候補）、`nameOf(i)`（柵は `<柵のアクター>/<区画>`、例 `Fence_battle_6/Cube1`。板張りの通り道はグループ名 `doorway` / `wayOut`）、`break(i)`、`touch(x, z, r, y0, y1)`（敵の体が触れたら壊す）、`smoke(at)`（`P_Explosion1` の煙だけを `at` に。game.ts の `breakBarrier` がロックピックとダッシュの障壁の中心で呼ぶ）、`update(dt): boolean`（飛んだ板が動いているか、打ち付けた板が崩れている途中か、消えたか: 影を描き直すか）、`reset(): boolean`（壊れていたものがあったか）、`showcase(at: Vector3 | null)`（ロードのプリウォーム）、`state()`（デバッグ API の `stage().breakables`: `{ name, box: { min, max }, broken, gone }[]`。`gone` は壊れて消えた後）。
- `touches(b: Box, x, z, r, y0, y1): boolean` — 直立の体（x・z に半径 r、高さ y0〜y1）が箱 `b` と重なるか（x・z は半径ぶん広げた比較、y は区間の重なり）。`Breakables.touch` と、game.ts のロックピックとダッシュの障壁の敵の重なりが使う。
- `warmFade(meshes, on)` — ロードのプリウォームで、薄れて消えるメッシュの半透明の変種を先に作らせる: `on` なら visibility 0.999・`alwaysSelectAsActiveMesh = true`、false なら visibility 1・false に戻す。`Breakables.showcase` と、game.ts のロックピックとダッシュの障壁の板が使う。

## 内部構造と処理の流れ
### loadLevel
1. **読み込み**: `LoadAssetContainerAsync(store.resolve('assets/level/level.gltf'), …, { pluginOptions: { gltf: { preprocessUrlAsync } } })`（参照する `level-<n>.bin` と KTX2 を `store.blobUrlFor(url) ?? url` の Blob URL に差し替える）と、`assets/level/colliders.json`・`assets/level/stage.json` の fetch（失敗時は空・null）を並行。`container.addAllToScene()` の後、`transformNodes + meshes` から名前で引く `byName` を作る。KTX2 デコーダの URL 設定は game.ts の `Game.create` で行う。
2. **座標**: glTF のルート（`__root__`）の世界行列 `rootMatrix` を使い、Blender の (x, y, z) を glTF の (x, z, −y) にして `rootMatrix` で Babylon の世界座標にする（`fromBlender`）。Babylon の y は Blender の z（迷路の床が 0、駅は −21.03 m）。箱は 2 隅を変換して min / max（`toBox`）。向き `yawOf(from, to)` は `atan2(dx, dz)`。
3. **コライダー**: `colliders.json` の床と壁の箱を `PhysicsShapeBox` にし、箱の中心の水平の 12 m の格子（`COLLIDER_CELL`）ごとの `PhysicsShapeContainer` の子にして、その格子のコンテナを 1 つの `PhysicsShapeContainer` に入れ子にし、`TransformNode('levelCollision')` の STATIC ボディ 1 つにまとめる（Havok のコンテナは子を足すたびに子の全体をやり直すので、ステージの 18,719 個を 1 つのコンテナに入れると組み立てに 15 s かかっていた。格子ごとなら 0.1 s で、形は同じ）。同じ箱を `navBoxes` に積む（床は `floor: true`）。階段の坂（`ramps`）と、立ったプレイヤーが下に入れない地下の床の上に掛かる物（`overhead`。そこの天井。12 記録）があれば、それぞれワールド座標の隠れたメッシュ `stairsRamp` / `overhead` を作り、`PhysicsShapeMesh` として同じコンテナに入れる（ナビには入れない）。床の箱が無ければ警告。
4. **開始とシャード**: `stage.json` の `start`（checkpoint 1 のホーム。床から 0.05 m 上）と `look` への向き。シャードは各位置から 0.7 m 下げて `SHARD_HEIGHT` 上げた所（301 個）。
5. **動く部品**: `<役割>__<アクター>__<部品>` の名前のノード（親がすでにそうなら子は数えない）の頂点のあるメッシュを、役割ごと・アクターごとに `parts` にまとめ、`dynamicMeshes` に入れる。
6. **ランプ**: `meta.lamps` の順に `index` 1..301。位置は `fromBlender`、発光面は `glow` の名前の部品の子孫、`power = (candela ?? power / 4π) / 2.4`、`color` は sRGB → 線形、`range` は減衰半径（無ければ `CONFIG.lights.range`）、`lit: true`。個数が `CONFIG.lights.count`（301）と違えば警告。
7. **ライトマップのページ**: `staticMeshes` は頂点があり動く部品でも坂でもないメッシュ（`Level_Static_NN`。ゾーン・ページ・キャプチャ・24 m の格子ごとのタイル、12 記録）。ライトマップを貼るのは `staticMeshes` と、自分か親の名前が `lightmapOf` にある動く部品（`lit`）。`lights.mode` が `'realtime'` なら `lightmap.indirect`、それ以外（`'baked'` / `'hybrid'`）は `lightmap.full` の `{page}` をページの番号にした URL を `Texture(… , { noMipmap: false, invertY: false, forcedExtension: '.ktx2' })` で読み、`coordinatesIndex = 1`、`gammaSpace = true`、`level = lightmapScale × CONFIG.lightmap.intensity`、CLAMP。manifest に無いページは警告して飛ばす。`'hybrid'` では同じページの `lightmap.indirect` も同じ作り方で読み、`mixOf`（全部のページ → 間接光のページ）を `applyCaptures` に渡す（反射の混合が読む）。
8. **マテリアル調整**（`lit` の `PBRMaterial`）: 無照明でなくページがあれば、元のマテリアルとページの組ごとに写し（`<名前>#<ページ>`）を作ってそのページを貼る（`useLightmapAsShadowmap = false`）。albedo / bump / metallic / ambient / emissive のテクスチャに `anisotropicFilteringLevel = CONFIG.materials.anisotropy`（同じテクスチャは 1 回）、`bumpTexture.level = normalStrength`、`maxSimultaneousLights = shadowCasters + 1`、`receiveShadows = true`。`staticMeshes` だけ `freezeWorldMatrix()` と `doNotSyncBoundingInfo`。
9. **ライトマップ・キャリア**（Babylon の PBR は通常のライトマップを最終色に加算しアルベドを掛けないため）: ページが 1 つでもあれば `HemisphericLight('lightmapCarrier')` を強さ 0・色すべて黒・`LIGHTMAP_SHADOWSONLY`・`includedOnlyMeshes = lit` で 1 灯。
10. **反射**（`applyCaptures`。`environment.captures.enabled` で、manifest にキャプチャか空の全天球があるとき。無ければ `applyEnvironment` の HDRI）:
    - キャプチャごとに `HDRCubeTexture(store.url('assets/level/' + file), size = environment.captures.size (128), noMipmap false, generateHarmonics true, gammaSpace false, prefilterOnLoad true)`、読み込み後に球面調和を 0 倍（拡散はライトマップが持つので、キャプチャは鏡面だけ）。中心と大きさ（Blender の半分の大きさ (x, y, z) を Babylon の (2x, 2z, 2y) に。球は半径の立方体）を `boundingBoxPosition` / `boundingBoxSize` として持たせる（箱に投影する）。ステージのキャプチャは地下の駅と階段の箱 2 つ。
    - SkyLight の全天球（`meta.sky.file`、`captures/Sky.hdr`。12 記録の build_cc2.py が空だけを描いたもの）も同じくキューブにする（投影しない。空は遠い）。
    - `lit` の各メッシュは自分か親の名前で `captureOf` を引き、そのキャプチャ（−1 やキャプチャが無ければ空）のマテリアルの写し（`<名前>@<キャプチャ名>`）に替える: `reflectionTexture` = そのキューブ、`environmentIntensity = environment.captures.intensity`。どちらも無ければ `reflectionTexture = null`・`environmentIntensity = 0`。UE ではキャプチャの届かない所は SkyLight の全天球を映す（地上の迷路全体）。
    - ライトマップがあり `average` の分かる写しには `CaptureMixing`（UE の `r.ReflectionEnvironmentLightmapMixing`）を付ける: `CUSTOM_FRAGMENT_BEFORE_FINALCOLORCOMPOSITION` で `finalRadianceScaled *= mix(1, min(Luminance(irradiance) / (average × intensity), 10000), smoothstep(0, 1, saturate(roughness × 5 − 0.5)))`。`irradiance` は UE と同じく間接光だけのライトマップ: `'hybrid'` ではそのページの間接光のテクスチャ（`mixOf`。サンプラ `captureIndirectSampler`（宣言は `CUSTOM_FRAGMENT_DEFINITIONS`。09 記録の UBO の項）を `vLightmapUV` で読み、2.2 乗 × その `level`。define `UE_CAPTURE_MIX_INDIRECT`）、それ以外は `lightmapColor`（`'realtime'` は間接光のページそのもの）。SkyLight は Movable なので UE も正規化せず、同じ混合が掛かる。GLSL と WGSL。
    - それ以外（シャード・敵・障壁）は `scene.environmentTexture`: 0.25 s ごとにカメラを含む最初のキャプチャ、無ければ空、それも無ければ中心の最も近いキャプチャ。`scene.environmentIntensity = environment.captures.intensity`。
11. `lit` の最終の材質のうちライトマップのあるものに `addSceneAo`（09 記録の SSAO）と `addSsr`（09 記録の画面空間の反射）を付け、`lights.mode === 'hybrid'` なら `HybridLamps` も付ける（下記）。3 つとも `CaptureMixing` と同じく `doNotSerialize`。材質が決まった後、`render.depthProxies.enabled` なら `new DepthProxies(scene, staticMeshes, render.depthProxies.cell)`（09 記録）。
12. **ステージの部品**（`stage.json` があるとき）: 障壁は四隅の AABB を水平に 0.15 m 広げた箱と中心、板は役割 `barrierShard` / `barrierPick` / `barrierDash` の同じアクターの部品。チェックポイント 1 / 2 / 4 は床から 0.05 m 上の位置と `look` への向き、トリガーは箱、配電盤は位置、特殊シャードは赤いシャード（`reveal`）とオーブ（`stun`）の `{ kind, spawnTime: interval, points }`、`stunTime` はオーブの `duration`（無ければ 15）。駅の柵は区画ごとの箱・中心・`flip`・`gate` と、役割 `fence` の同じアクターの部品のノードを部品名（ノード名の `fence__<アクター>__` の後ろから末尾の `_数字` を除いたもの）で引く `planks`。板張りの通り道は `boards` のグループごとに、役割 `boards` の部品（`parts.boards`）のうちアクターがそのグループの `actors` にあるもの（`BoardsDef`）。床の噴出は根と 2 つの箱、車は 2 つの箱と `unit`（Blender の方向を `fromBlender(unit) − fromBlender(0, 0, 0)` で Babylon に）と `parked` と役割 `car` の同じアクターの部品。扉は種類ごと（stage.json の `doors`）に `origin` と `axes`（同じく方向として Babylon に）、`DOOR_ROLE`（罠 `trapDoor`・開き戸 `swingDoor`・シャッター `shutter`・通気口 `vent`・ブロッカー `blocker`、施錠は静的）の同じアクターの部品、ほかの箱と点。脱出は道の点を `fromBlender`、箱を `box`、トラックの部品は役割 `truck` のアクター `Truck_boss`。
13. `exit` はステージならゴールの箱の中心、`enemySpawns` / `waypoints` は `colliders.json` の床に載せたもの、`route` は `colliders.json` の `route` の点 `[x, y, z]` を `fromBlender` にしたもの（無ければ空）。

### 'hybrid' の灯（`HybridLamps` と `PoolLight`）
ライトマップのある面では、全部の灯の拡散光が（静的な影ごと）ライトマップに入っているので、影付きの 3 灯は鏡面の光と、拡散のうち焼き込みからの変化だけを足す（街灯の点滅・消灯、罠の扉と障壁の灯が消えること、脱出の色。stage-lamps.ts）。
- プラグインは `getCustomCode` の正規表現（`!` で始まる鍵。Babylon の materialPluginManager はインクルードを展開した後・プリプロセッサの前のコードに当て、`$1` などの置換も効く）で、フラグメントの各灯の `diffuseBase += info.diffuse * shadow;` を `#ifdef POINTLIGHT<n>` の `diffuseBase += info.diffuse * shadow * vec3(kR, kG, kB) * mix(1, max(preInfo.L.y, 0), down)` に書き換える。一致は灯のブロックの最初の灯のユニフォームから始めてその番号 n を取る。ユニフォームの綴りは、ユニフォームバッファなら `light<n>.vLightData`（WGSL）、無ければ `vLightData<n>`（この WebGL2。ヘッドレスで確かめた）で、鍵を綴りごとに 2 つ持つ（1 つのシェーダーにはどちらかしか無い）。点光源でない灯（キャリアの半球光）の行は `#ifdef` で消える。
- 係数は `PoolLight.transferToEffect` が Babylon の書き込みの後に、点光源が使わないユニフォームに書く: `vLightData.w` = kR（`down` なら 1000 引く）、`vLightFalloff.zw` = kG, kB。`LampSystem.shade` がスロットごとに毎フレーム決める: ライトの色 = 状態の色（`tint ?? color` × `scale`。各成分をランプの色の 1/100 以上に）、`delta` = (状態の色 − ランプの色) / ライトの色（そのままなら 0、消灯でほぼ −1）、`down` = ランプの `down`。拡散の項は灯の色 × 強さに比例するので、足される量は (状態の色 − ランプの色) × 灯の拡散になる。下向きの矩形（街灯）は面から灯への向きの y（真下からの余弦。UE の矩形の灯のランバートの放射）で重みを付け、灯より上の面から引かないようにする。
- キャリアの項は LIGHTMAPEXCLUDED の分岐の別の文なので残る。ライトマップの無いもの（シャード・敵）は灯の光を全部（状態の色で）受ける。

### level-meta.json が提供するもの
`scripts/blender/build_cc2.py` が出力（12 記録）。level.ts が使うのは `lightmapPages`、`lightmapOf`、`lamps[]`、`captures[]`、`captureOf`、`sky`。

### buildDebugField（debug-field.ts）
`CONFIG.debug.field` のときステージの代わりに使う検証用の広場。Blender もライトマップも使わず、実行時に箱と面だけで組み立てる（game.ts はステージの glTF・テクスチャ・ライトマップを読まず、モデル・音・HDR だけを読む）。座標は Babylon のワールド座標（y 上、床の上面 y = 0）。
- **配置**（壁の座標はすべて 0.5 m の倍数、壁の高さ `WALL_HEIGHT` 4 m）: 48 m 四方の広場と門の北の出口の小部屋、独立した壁と膝の高さの壁、幅 2 m の廊下と北西・北東の部屋、1 m 角の柱 9 本、開始位置 `PLAYER_START` (0, 0.05, −20)（出口 `EXIT` (0, 1.2, 28) の方向）、シャード 12 個 `Shard_001..012`（高さ `SHARD_Y` 1.1 m）、壁灯 15 灯（`LampGlow_N` の直径 0.12 m の球、`power` 1、色 `lights.color`、範囲 `lights.range`）、敵の出現位置 3 か所と巡回点 11 か所。
- **メッシュ・マテリアル**: `Geometry` が m 単位の UV の四角形を積んで 1 メッシュにする。`FieldFloor`（壁の下には床を張らない。地図に輪郭が出る）、`FieldWalls`、`ExitGlow`。格子の `DynamicTexture`（1 m の市松と線、5 m の太線）を albedo と emissive に使う PBR（`FIELD_GLOW` 2.2 の自己発光）。
- **門**: `TransformNode('Gate')` (0, 0, 24.25) と子の `Gate_Bars`（金の格子）。`gateStyle: 'lift'` なので `Gate` が上へ `gateLift` 3.8 m 上げる。
- **コライダー**: 床板と壁の STATIC ボディ 1 つ、門の開口の ANIMATED ボディ。`navBoxes` は床板・壁・門の箱。
- 全回収で game.ts が配電盤を使った扱いにし（脱出）、門が開き、出口で脱出する（04 記録）。

### LampSystem
- **コンストラクタ**
  - 各ランプは発光面の元マテリアルを `clone` して独立させる。`lit` の PBR（ステージの街灯など、照らされる面に発光部分があるもの）は元の `emissiveColor`（glTF の発光）を `glowColor` として保ち、照明はそのまま。そうでない PBR は元の `emissiveColor`（黒ならランプの色）を `glowColor` とし、`emissiveIntensity = lights.emissiveIntensity`（9）、`albedoColor = 黒`、`disableLighting`。発光面は `freezeWorldMatrix()` と `doNotSyncBoundingInfo`。
  - スロット `min(shadowCasters=3, 灯の数)` 個の `PoolLight`（`falloffType = Light.FALLOFF_GLTF`: 1/d² に窓 `saturate(1 − (d/range)⁴)²` を掛ける Babylon の減衰で、UE の逆 2 乗の減衰と同じ。焼き込みの灯も同じ窓。12 記録）: `tune(light, lamp)` で `range` = lamp の範囲、`shadowMaxZ` = 範囲、色は `shade`（`'realtime'` は `diffuse` と `specular` = 状態の色、`'baked'` は `diffuse` 黒、`'hybrid'` は上記）。`radius 0.06`、`shadowMinZ 0.05`。`ShadowGenerator(shadowMapSize=512)`（Poisson、bias 0.004、normalBias 0.02、darkness 0.1）、影マップは `REFRESHRATE_RENDER_ONCE`、`renderList = castersNear(位置, 範囲)`。
  - `castersNear(p, range)` / `reaches(mesh, p, range)`: メッシュの AABB と点の距離がそのライトの範囲以内のもの。
- **update(dt, viewer)**
  1. `reassignInterval`（0.2 s）ごとに `reassign(viewer)`（出力 / 距離² の順に 3 灯。割り当て中は `hysteresis` 0.3 で優遇。消灯したランプも数える: 焼き込みから差し引くため）。
  2. 発光: `flicker.amount` が 0 より大きいときは全ランプ、0 なら状態が変わったランプ（`dirty`）だけ、`emissiveColor = glowColor × flicker × scale`（`tint` があればその色を `glowColor` の最大成分の強さで）。
  3. クロスフェード（`fadeTime` 0.35 s）。付け替えでライトを移すときに `tune`（範囲と色）と `renderList` を差し替えて影マップを描き直す。スロットごとに毎フレーム `shade`（状態の色と差分）、`intensity = lights.intensity × weight × flicker × power`。

### lights.mode の違い
- `'hybrid'`（既定。ステージの 301 灯）: 直接 + 間接のライトマップ + 近い 3 灯の鏡面と影（ライトマップのある面の拡散は、灯の状態が焼き込みから変わった分だけ: `HybridLamps`）。ライトマップの無いものは 3 灯の拡散も受ける。
- `'realtime'`: 間接光だけのライトマップ + 近い 3 灯の直接の拡散・鏡面・影（Hotel の既定。灯が多いと 3 灯より遠い灯の直接光が消える）。
- `'baked'`: 直接 + 間接のライトマップ。ライトの `diffuse` は黒（Babylon の PBR は鏡面も diffuse の色で描くので、静的な面に灯は何も足さない）。

### 壊せる物（breakables.ts・woodboards.ts）
原作の Hotel の板張りのバリケード `BP_01_Woodboards`（pak_reference の `_bytecode/DDeception/Content/Blueprints/01_Hotel/BP_01_Woodboards.txt`）: プレイヤーの 200 cm の視線のトレースが Destructible の `interact` タグに当たると手のマークが出る。左クリックの `InteractWithObject`（@937 から）はタグを消し（`ComponentTags`、@48）、`ApplyRadiusDamage`（ダメージ 2、アクターの位置、半径 10 cm、衝撃 2000。APEX のかけらが物理で飛ぶ、@139）、当たりの種類を Visibility にし（`SetCollisionObjectType`。かけらはもうプレイヤーを止めない、@200）、DoOnce で `Wooden_Boards_Breaking_v5`（@312）と `P_Explosion1` をスケール 1.5 で（@432）出す。`Delay(2.0)`（@512）の後、`Timeline_0`（@942）が 2 s で材質の Opacity を 1 → 0 にして `K2_DestroyActor`。敵（タグ `Enemy` のポーン）が `Box` に重なると同じく壊れる（@1012）。本作の壊せる物はすべてこの流れと値に倣う（ファンゲームのステージの駅の柵と板張りの通り道も。柵はファンゲームのパンチで 1 枚ずつ落ちる板と 4 回で開く区画をやめ、通り道はファンゲームがしゃがみ・スライドでくぐらせる所を壊して通るようにした）。
- 生成: 柵ごと・区画ごとに 1 グループ（名前 `<柵のアクター>/<区画>`。柵 5 基 × 区画 2 = 10）。区画の `flip` と `gate` にある部品名を重複なしで集め、その柵の `planks` から引いた板をかけらにする（1 枚の板は最初に取ったグループだけ: Cube は Fence9・8・7・2、Cube1 は Fence5・6・4・10）。かけらごとに `pieceOf` が元の親と局所の位置・回転（四元数かオイラー）・拡縮と、頂点のある最初のメッシュの局所の境界箱と世界行列からワールドの箱（中心・向き・大きさ。glTF のルートの鏡映で軸が左手系になれば z を反転して回転にする）を控える。立っている板は体を持たず、区画の当たり `partCollider(scene, 'fence_<source>_<i>', box)`（ANIMATED の箱）が行く手を塞ぎ、視線もその箱を見る。続けて板張りの通り道ごとに 1 グループ（名前はグループ名: `doorway` = ホームの低い戸口に掛かる格子の壁 `Fencing_Cube_77`、`wayOut` = 出口に打ち付けた板 `Fence9_83`・`Fence10`〜`12`。部品が 1 つも無ければ作らない）: 部品のノード（柵で取ったものは除く）をかけらにし、その全メッシュのワールドの境界箱（`boundsOf`: 各ノードと子のメッシュの `boundingBox.minimumWorld` / `maximumWorld`）を当たり `partCollider(scene, 'boards_<name>', box)` と視線の箱にし、中心は箱の中心。柵の区画は `fly: true`（かけらを物理で飛ばす）、通り道は `fly: false`（その場で崩す）。グループはステージで 12（柵 10 と通り道 2）。煙は全グループで共有の `SpriteManager('breakSmoke', './fx/smoke-subuv.webp', max(3, グループ数 × 3), 128)`（8 × 8 の 128 px のコマ。13 記録の prepare-boost-fx.mjs）、ピックしない、`disableDepthWrite`。
- `break(i)`（立っているものだけ。game.ts の左クリックと `touch`）: `broken`、時計 `t = 0`、区画の当たりの体を `dispose`、かけらごとに `fly`（`fly` のグループ）か `sag`（板張りの通り道）、グループの中心で `puff` と `onBreak(centre)`。
- `fly`（`ApplyRadiusDamage` の代わり）: かけらの箱の中心と向きに置いた pivot `<板>_flying` に DYNAMIC の体（かけらの大きさの `PhysicsShapeBox`、membership `PIECE_MASK`、`filterCollideMask = ~PIECE_MASK` でかけらどうしは当たらない〈区画の板は X 字に交差していて、同時に動かすと重なりで弾け飛ぶ〉。レベルの当たりには当たって止まる）を作り、板のノードをその子にする。質量 `mass` 6 kg、区画の中心からかけらの中心への水平の向き（重なればランダム）に `push` 0.6 m/s と上へ `lift` 0.3 m/s、角速度はランダム（x・z ±1.5、y ±1 rad/s）。
- `sag`（板張りの通り道。板は壁と床に打ち付けてあり、体を持たせると壁や床から弾け出るので物理にしない）: かけらの箱の中心と向きに置いた pivot `<板>_sagging`（体なし。`flying.body` は null）の子に板のノードを付け、水平のランダムな軸 `sag` を選ぶ。`update` が `u = min(1, t / SAG.time)` の ease-out `e = 1 − (1 − u)²` で pivot を中心から `SAG.drop × e` 下げ、その軸まわりに `SAG.tilt × e` 傾ける（`SAG { time 0.35 s, drop 0.2 m, tilt 5° }`。推定）。薄れと消え方は柵と同じ（2 s 後から 2 s、4 s で消える）。
- `puff`（`P_Explosion1` のバースト）: スプライト 3 つを区画の中心から半径 `radius × scale` 0.15 m の玉の中に、向き 0〜2π、寿命 2〜3 s、大きさ (0.7〜1.1) × 1.5 m、上昇 (0.1〜0.25) × 1.5 m/s、回転 ±0.02 × 2π rad/s で出す。毎フレーム上へ進めて回し（`updatePuffs`）、`smokeAt(age / life)` で幅と高さ（大きさ × SizeMultiplyLife）、コマ `cellIndex`、色（灰 = ColorOverLife × `light`、α = AlphaOverLife）を置き、寿命で捨てる。
- `update(dt)`: 壊れて消えていないグループの `t` を進め、`WOODBOARDS_GONE` 4 s で消す（`remove`: かけらの体を `dispose`、ノードを無効に。`K2_DestroyActor`）。それまでは崩れるかけらの pivot を置き直し（上の `sag`）、かけらの全メッシュの `visibility` を `woodboardsOpacity(t)` に（変わるときだけ）。飛んでいるかけらの線速度か角速度が 0.02 を超えているか、崩れている途中（`u < 1`）か、消えたフレームなら true。最後に煙を進める。
- 透け方: glTF の読み込みは不透明な材質に `MATERIAL_OPAQUE` を付け、Babylon の `Material.needAlphaBlendingForMesh` は透明度のモードがあると `mesh.visibility` を見ないので、`visibility` だけでは透けない（最初はそのため、柵の板も通り道も透けずに 4 s で消えていた）。そこで生成時に、かけらのメッシュの材質ごとに半透明の写しを 1 つずつ作って `looks` に持つ（level.ts の `fadeCopy(material)`: `clone` して `MATERIAL_ALPHABLEND` にし、`clone` が落とす自作のプラグイン〈`UeCaptureMixing`・`UeSceneAo`・`UeSsr`・`HybridLamps`〉のうち元の材質にあるものを付け直す。`CaptureMixing` の `average`・`indirect` はそのために公開）。グループが薄れ始める（`t > hold` 2 s の最初のフレーム、`faded`）ときに写しへ替え、`restore`（`reset`）で元の材質に戻す。`showcase` は写しを付けて `visibility` 0.999 で描かせ、写しのシェーダーもロード中にコンパイルする。元から半透明の材質（光る障壁など）は写しを作らない。
- `touch(x, z, r, y0, y1)`: 直立の体（x・z に半径 r、高さ y0〜y1）が立っているグループの箱と重なれば（`touches`）`break`（原作の `Box` の BeginOverlap。game.ts が `active` な敵ごとに `enemy.radius` 0.4 m と `enemy.height` 2.1 m で呼ぶ。04 記録）。
- `reset()`（レベルを開き直す・チェックポイント）: 壊れたグループの当たりを保った形の ANIMATED の体に作り直し、かけらは体と pivot（と崩れる軸 `sag`）を捨てて元の親と局所の変換に戻し、有効・visibility 1 に。煙を捨てる。1 つでも壊れていれば true（game.ts が影を描き直す）。
- `showcase(at)`: ロードのプリウォーム（04 記録。`playerStart + (0, 1.5, 3)`）。煙を捨て、全部のかけらのメッシュを `warmFade(meshes, !!at)` にし（`at` があれば visibility 0.999・`alwaysSelectAsActiveMesh`: 薄れる間の半透明の変種のシェーダーを先に作らせる。null なら visibility 1・`alwaysSelectAsActiveMesh` false に戻す）、`at` があればそこに寿命無限の煙のスプライト（寿命の 0.1 の姿、α 0.01）を 1 つ出す。
- `tests/woodboards.test.ts`（3 件）: `hold` 2・`fade` 2・`WOODBOARDS_GONE` 4、不透明度が 0 s と 2 s で 1・3 s で 0.5・2.5 s で三次の値・4 s 以降 0・増えない。`BOARDS_BREAK` の値。`SMOKE` の数・倍率・寿命、`smokeAt` の始め（大きさ 0.1・α 0・色 255/179 × `light`（10 を基本色の上限で切った値）・コマ 0）、0.2 で大きさ 2、0.6 で 2 のまま、0.1 で色 0.1 × `light`、4/31 で α 0.9677、1 で α 0・コマ 63、0.5 でコマ 32。

## 依存関係
- level.ts の import: `../config`、`../core/loader`（`AssetStore` 型）、`../render/scene-ao`（`addSceneAo`）、`../render/ssr`（`addSsr`）、`../render/depth-proxies`（`DepthProxies`）、`../enemy/navgrid`（`NavBox` 型）、`./gate`（`GateStyle` 型）、`./specials`（`SpecialDef` 型）。
- debug-field.ts の import: `../config`、`../core/loader`、`../enemy/navgrid`（型）、`./level`（`applyEnvironment`、`Level` 型）。
- lights.ts の import: `../config`、`../game/settings`（型）、`./level`（`Lamp` 型）。
- stage-lamps.ts の import: `../core/ue-curve.ts`（`evaluateCurve`、05 記録）だけ（Babylon を使わない。`tests/stage-lamps.test.ts` が node で読む）。使う側は game.ts（04 記録）。
- trap-fx.ts の import: `@babylonjs/core`（`Color4`, `ParticleSystem`, `Vector3`）、`./level`（`boxCentre`、`JetDef` 型）。使う側は game.ts。
- door-parts.ts の import: `@babylonjs/core`（`Matrix`, `PhysicsBody`, `PhysicsMotionType`, `PhysicsShapeBox`, `Quaternion`, `TransformNode`, `Vector3`）、`../game/doors`（`ueRotation`、`LocalBox`・`PartPose` 型。04 記録）、`./level`（`doorBox`、`DoorDef` 型）。使う側は game.ts。level.ts は `../game/doors` の `DoorKind` 型を読む。
- carpet.ts の import: `@babylonjs/core` の `Vector3`, `VertexBuffer`, `AbstractMesh`（型）だけ。
- breakables.ts の import: `@babylonjs/core`（`Color4`, `Matrix`, `PhysicsBody`, `PhysicsMotionType`, `PhysicsShapeBox`, `Quaternion`, `Sprite`, `SpriteManager`, `TransformNode`, `Vector3`、型 `AbstractMesh` / `Node` / `Scene`）、`./level`（`partCollider`・`BoardsDef` / `Box` / `Collider` / `FenceDef` 型）、`./woodboards`。使う側は game.ts（`Breakables`・`touches`・`warmFade`。04 記録）と player/controller.ts（`PIECE_MASK`。05 記録）。
- woodboards.ts の import: `../audio/stage-sounds`（`StageCue` 型）だけ（Babylon を使わない。`tests/woodboards.test.ts` が node で読む）。使う側は breakables.ts と game.ts（`BOARDS_BREAK`、ロックピックとダッシュの障壁の薄れの `WOODBOARDS_GONE`・`woodboardsOpacity`）。
- 使う側: `src/game/game.ts`（`loadLevel` / `buildDebugField`、`partCollider`、`LampSystem`、`Gate`、`Shards`、`Tablet`、`Minimap.capture`、`CarpetMap`、`insideBox` / `boxCentre`）。

## 設定・調整値
- `debug.field`（既定 false）。
- `lights.count` 301, `shadowCasters` 3, `shadowMapSize` 512, `mode` `'hybrid'|'realtime'|'baked'`, `color`（レベルに色が無いときの色）, `intensity` 6.25（出力 1 = 2.4 cd のランプの強さ。Hotel の 10 はライトマップ × 1.6 に合わせたもので、同じ比でライトマップ × 1 に）, `range` 11（レベルに範囲が無いとき）, `reassignInterval` 0.2, `hysteresis` 0.3, `fadeTime` 0.35, `flicker.amount` 0 / `flicker.speed` 9, `emissiveIntensity` 9。
- `lightmap.full` `assets/level/lightmap_{page}.ktx2` / `lightmap.indirect` `assets/level/lightmap_indirect_{page}.ktx2` / `lightmap.intensity` 1（ベイクが UE の単位なので、ボリュームの手動露出がそのまま合う。09 記録。Hotel は 1.6）。
- `environment.hdr`, `intensity` 0.2, `diffuse` 0.05, `cubeSize` 256（デバッグフィールドとキャプチャが無いとき）。
- `environment.captures`: `enabled` true、`size` 128（キューブの一面。UE の `r.ReflectionCaptureResolution`）、`intensity` 1（ライトマップの `intensity` と同じにする。build_cc2.py の `CAPTURE_SCALE` も同じ値）。
- `materials.normalStrength` 1.0、`anisotropy` 8。

## 既知の制約・注意点
- ライトマップは 8bit KTX2（UASTC、`gammaSpace=true`）にガンマと倍率を掛けて格納しており、暗部でバンディングが出うる。
- 障壁・車・発光面の一部はライトマップに入らない（12 記録の `bakedRoles` 以外）。
- 影付きライトを持たない灯は発光・ブルーム・ベイクの光だけで、割り当てが替わる瞬間に鏡面の光が 0.35 s かけて出入りする。
- `maxSimultaneousLights = 4` で固定しているため、`lights.shadowCasters` を増やしてもそれ以上のライトは静的メッシュに当たらない（`+1` はキャリア分）。
- `HybridLamps` はシェーダーの文 `diffuseBase += info.diffuse * shadow;` の形、灯のユニフォームの綴り、`preInfo.L`、点光源が `vLightData.w` と `vLightFalloff.zw` を使わないこと（`PointLight.transferToEffect` が 0 を書く）に依存する（Babylon の lightFragment と pointLight。版を上げたら確かめる）。
- 灯の状態が床の光に出るのは近い 3 灯だけ（それより遠い灯は発光面だけ変わる）。差し引くのは点光源の拡散なので、焼き込みの形（矩形の広がり、BarnDoor、光源の大きさ）とは少し違う。障壁の矩形の灯は背中合わせの 2 枚なので、全方向として引く。
- `preprocessUrlAsync` は AssetStore に無いファイルはネットワーク URL にフォールバックする。ライトマップ / HDR は `store.has()` が false だと警告のみでスキップ。
- KTX2 デコーダのローカル配信設定は game.ts 側にあり、level.ts 単体では KTX2 テクスチャは解決できない。
- シャドウマップの `renderList` はライトが移るたびに `casters` から選び直すので、後から追加する動的メッシュは必ず `addCaster` を通す（game.ts はシャードと特殊シャードのモデルに対して行う。動く部品は生成時に含まれている。敵は影を落とさない）。
- 反射キャプチャは UE では画素ごとに大きさの順で混ぜるが、ここではタイル（と動く物）ごとに 1 つ。
- デバッグフィールドは照明の確認には向かない（ライトマップがなく、明るさの大半は格子の自己発光）。デバッグフィールドのシャード ID `Shard_001..012` はステージと重なるので、game.ts がセーブキーを `.field` 付きに分けている（04 記録）。
- 原作のバリケードは APEX の Destructible で、細かいかけらに割れて飛ぶ。本作は駅の柵の板（ファンゲームのメッシュ）を 1 枚ずつそのまま飛ばす。飛ぶ速さ `push` 0.6 m/s・`lift` 0.3 m/s、質量 `mass` 6 kg、角速度は推定（`ApplyRadiusDamage` の衝撃 2000 はアクターの位置から 10 cm の内にしか届かず、割れた板のほとんどは落ちるだけ。APEX のかけらの質量は cook で落ちている。最初の 2 m/s・1 m/s では X 字の板が重なりで弾け飛んで 1 s で視界から消えたので、かけらどうしを当てずに弱めた）。煙の `light` 0.15 も推定（`P_Explosion1` の材質 `M_smoke_subUV` は照らされる半透明〈TLM_VolumetricDirectional〉で、ColorOverLife に立つ所の光が掛かるが、本作のスプライトは照らされないので決まった光を掛ける）。ほかの時間・音・煙の値は原作のデータ。
- 薄れは各メッシュの `visibility`（Babylon の半透明の描き方）で、原作の材質の Opacity のパラメータではない。壊れたかけらが飛んでいる間と消えるフレームは、game.ts が影を描き直す。
- 立っている区画を止めるのは区画の箱 1 つだけで、板の間の隙間は通れない（ファンゲームの区画の `Cube` と同じ）。板張りの通り道も部品のメッシュの境界箱 1 つが塞ぐ（`Fencing_Cube_77` は幅約 3.7 m・高さ約 13.8 m の 1 つのメッシュで、胸の高さの開口ごと箱で塞ぎ、壁ごと壊れる。ユーザーの選択）。
- 板張りの通り道の崩れ方（`SAG`: 0.35 s で 0.2 m 下がり 5° 傾く）は推定で、原作の APEX のかけらが割れて落ちる代わり。
- ステージのロックピックとダッシュの障壁（game.ts。04 記録）は、壊れ方の流れ（視線の手のマークと 1 クリックか敵の接触、`P_Explosion1` の煙、2 s 後から 2 s で薄れて 4 s で消える）を原作のバリケードに倣うが、光る板でかけらにする板を持たないので、崩れ落ちずにその場で薄れる（原作のバリケードとの違い）。音は `Wooden_Boards_Breaking_v5` ではなくファンゲームの障壁の `Barrier_Shatter_Cue` のまま。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: 館の迷路・館の拡張に合わせた（灯 49 → 120、影付きライトのヒステリシス、`castersNear`、`navBoxes` / `enemySpawns` / `waypoints`）、ちらつきを `emissiveColor` の掛け直しに、衝突箱のメッシュを破棄
- 2026-09-12: デバッグフィールド（`debug-field.ts`）と `applyEnvironment`、足音の床の判定 `carpet.ts`、発光面の凍結とグレアの `cullHalos`、`LampSystem.setQuality`
- 2026-09-13: 原作の Hotel に置き換えた（ユーザーの指示）: 衝突箱を glTF の `COL_*` から `colliders.json` に、目印を `PlayerStart_Look`・`LobbyStart`・`Altar`・`Trigger_*`・`LobbySpawn_*` などに、灯を meta の W の中央値比・原作の色（線形）・減衰半径に。`Level` に `gateStyle` と `hotel` を足し、エレベーターの扉の葉を両面のマテリアルに、テクスチャの繰り返しをやめた。`LampSystem` はランプごとの色・範囲・グレアの色と `setColor`、発光は元マテリアルの色、ちらつき 0 なら更新しない。絨毯の判定に `Carpet` を加えた
- 2026-09-13: `HotelExtras` と `HotelParts` に特殊シャードの `specials`（出現の間隔と出現点）を足した
- 2026-09-13: 灯のグレアのスプライト（`lampHalo`、`createHaloTexture`、`haloMaterial`、`cullHalos`、`lights.halo`）を外した（原作に無く、UE のブルームにした。09 記録）
- 2026-09-13: ホテルの反射を弱い HDRI から原作の反射キャプチャ 12 個にした（`applyCaptures`: 箱に投影するキューブ、タイルごとのマテリアルの写し、カメラのまわりのキャプチャ、UE のライトマップ混合 `CaptureMixing`）。`LevelMeta` に `captures` と `captureOf` を足した。HDRI はデバッグフィールドとキャプチャが無いときだけ
- 2026-09-13: 実行時の影付きの灯を UE と同じ減衰（`FALLOFF_GLTF`、減衰半径で打ち切る窓）にし、ライトマップも同じ窓で焼き直した（12 記録）
- 2026-09-13: 照らされた材質に画面空間の反射のプラグイン（`addSsr`）を付け、材質が決まった後に深度の代理（`DepthProxies`）を作るようにした（09 記録）。`CaptureMixing` を `doNotSerialize` にした（キャプチャを切ると灯の発光面の `clone` でロードが止まっていた）
- 2026-09-14: 原作の Hotel の秘密と隠し扉（08・12 記録）のため、`ColliderData` に `secretDoors`、`HotelExtras` に `secrets` / `secretDoors`、`HotelParts` に `secrets` / `secretDoors` を足した。`SecretDoor_NN` のメッシュを動く物と `lit` に入れ、隠し扉ごとに蝶番の `TransformNode` と開く角度とコライダー（ナビにも入る）を作る
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 に差し替えた（ユーザーの指示）: `hotel.json` と `HotelParts` / `HotelExtras` をやめ、`stage.json` の `StageParts`（チェックポイント 1 / 2 / 4、トリガー、配電盤、障壁 `BarrierDef`、役割ごとの動く部品、特殊シャード、マネキンの復活点）に。`colliders.json` の階段の坂（`PhysicsShapeMesh`）、巡回点と出現点、ライトマップのページ（`lightmapPages` / `lightmapOf`、ページとマテリアルの組ごとの写し）、meta の `lamps` からの灯（cd を 2.4 cd に対する比に、`owner`、`lit`）、SkyLight の全天球（`sky`。キャプチャの届かない面が映す）、`partCollider`、`SHARD_HEIGHT`。灯が 301 あるので `lights.mode` の 'hybrid'（全部のライトマップと 3 灯の鏡面・影。`LampSpecularOnly`）を足した。`LampSystem` は `lit` の発光面の照明を残す。デバッグフィールドの `hotel` を `stage` に
- 2026-09-14: 'hybrid' の反射の混合を UE と同じく間接光だけのライトマップで行うようにした（間接光のページも読み、`CaptureMixing` に `indirect`。全部のライトマップで混ぜると灯のそばの反射が何倍にもなっていた）。ライトマップとキャプチャの `intensity` を 1.6 → 1、灯の `intensity` を 10 → 6.25 にした（ベイクが UE の単位で、ボリュームの手動露出がそのまま合う。09 記録）
- 2026-09-14: ステージの灯のギミック（ファンゲームの `Lamp_zone_1` と `Light_retro_breaking`）を足した: `stage-lamps.ts`（街灯 48 の追跡中の点滅と回復、脱出の消灯と色の巡回、レトロな灯 7 の点滅、障壁と罠の扉の灯）と `LampSystem.setState`（使われていない `setColor` を置き換え）。'hybrid' の `LampSpecularOnly` を `HybridLamps` にし、近い 3 灯が拡散のうち焼き込みからの変化だけを足す（`PoolLight` の係数。下向きの矩形は余弦で重み）。`Lamp` に `down`
- 2026-09-14: 障壁に反応する箱 `zone`・根 `root`・ロックピックのウィジェットの点 `widget`、動く部品に `nodes`、ステージに床の噴出 `jets`（`JetDef`）と車 `cars`（`CarDef`）を足した。床の噴出の煙 `trap-fx.ts` を足した。扉 `DoorDef`（アクターの枠）と `doorPoint` / `doorBox`、扉の動く部品と当たり `door-parts.ts` を足した。脱出 `EscapeDef`（道・ブームの点・箱・壁・群れ・トラックの部品）と `boxCollider` を足した
- 2026-09-14: `CaptureMixing` の GLSL のサンプラー `captureIndirectSampler` の宣言を `getUniforms().fragment` から `CUSTOM_FRAGMENT_DEFINITIONS` へ移した（UBO のある Windows などで宣言されなかった。09 記録）
- 2026-09-14: 駅の柵を読むようにした（`FenceDef` / `FenceSection` / `FenceText`、stage.json の `fences`、役割 `fence` の板を部品名で）。colliders.json の `overhead`（しゃがみでだけ通れる床の上に掛かる物の三角形）を階段の坂と同じくメッシュの当たりに
- 2026-09-14: 静的な当たりの箱を 12 m の格子ごとのコンテナに分け、剛体のコンテナに入れ子にした（`COLLIDER_CELL`）。1 つのコンテナに 18,719 個を足すと Havok の `addChild` が要素数の 2 乗で、「ステージを組み立てています…」の 17.7 s のうち 14.9 s を占めていた。組み立ては 2.5 s に（Node で Havok を直接呼んだ比較で、光線 2000 本の当たりが今までと一致）
- 2026-09-15: 駅の柵 `fences.ts` を足した: `Fences`（区画の当たり〈`FENCE_MASK`〉、パンチの `hitTest` / `hit`〈FlipFlop と MultiGate〉、物理で落ちる板〈`FALLEN_PLANK_MASK`、collide `~FENCE_MASK`〉、4 枚で区画が開き、立っている板が自分の体を持つ、`Wood_Break2` の `onBreak`、床の案内の文字〈beer_money、`TEXT_FADE`〉、影の描き直しを返す `update` / `reset`、デバッグ API の `state` / `textsFading`）と `rayBox`
- 2026-09-15: `ColliderData.route` と `Level.route`（開始地点からホールのトリガーまでの経路の点としゃがみの印。flow-check が歩く）を足した。デバッグフィールドは `route: []`
- 2026-09-15: 本作の規範（本家に無いギミックは複雑にせず、壊せる物は本家の Hotel の板張りのバリケードのように 1 クリックで崩れて自然に消える）とユーザーの選択で、駅の柵を作り直した: `fences.ts`（パンチの `hitTest` / `hit`、FlipFlop と MultiGate、4 枚で開く区画、立っている板の体、`FENCE_MASK`・`FALLEN_PLANK_MASK`、床の案内の文字と `TEXT_FADE`・`FENCE_TEXTS`、`rayBox`）を消し、原作の `BP_01_Woodboards` の値と時間軸の `woodboards.ts`（`WOODBOARDS`・`WOODBOARDS_GONE`・`BOARDS_BREAK`・`woodboardsOpacity`・`SMOKE`・`smokeAt`）と `tests/woodboards.test.ts`、区画ごとに 1 クリックか敵の接触で壊れて板が飛び、`P_Explosion1` の煙が上がり、2 s 後から 2 s で薄れて消える `breakables.ts`（`Breakables`・`PIECE_MASK`）を足した。level.ts の `FenceTextKey`・`FenceText`・`FenceDef.tutorial`・`StageJson` の `tutorial`・`StageParts.respawns`（マネキンの復活点）を削除した
- 2026-09-15: ステージのロックピックとダッシュの障壁も板張りのバリケードのように壊すため（04 記録）、breakables.ts に `Breakables.smoke(at)`（`P_Explosion1` の煙だけ）と、書き出した `touches`（直立の体と箱の重なり。`touch` も使う）・`warmFade`（プリウォームの半透明の変種。`showcase` も使う）を足し、煙の `SpriteManager` のフィールド名を `sprites` にした。level.ts の `BarrierDef` から `root` と `widget`（ロックピックの F の届く距離の起点とウィジェットの位置）を削除した（`StageJson` の型と stage.json には残る）
- 2026-09-15: 本作の規範（本家のプレイヤーにしゃがみ・スライドは無い）とユーザーの選択で、ファンゲームがしゃがみ・スライドでくぐらせる駅の板張りの通り道（出口の板 `wayOut`、低い戸口の格子の壁 `doorway`）を壊せる物にした: level.ts に `BoardsDef`・`StageJson.boards`・`StageParts.boards`（役割 `boards` の部品をグループのアクターで）を足し、`ColliderData.route` を `[x, y, z][]`、`Level.route` を `{ at }[]` にした（しゃがみの印を削除。`overhead` の説明を立ったプレイヤーが下に入れない所の天井に）。breakables.ts の `Breakables` が `boards` も受け取り、グループごとにメッシュの境界箱（`boundsOf`）の当たりを作り、壊れるとかけらを飛ばさずにその場で崩す（`sag`・`SAG`・`Group.fly`・`Piece.sag`）
- 2026-09-15: 壊せる物のかけらが透けずに 4 s で消えていたのを直した（不透明な glTF の材質は `visibility` で透けない）: level.ts に `fadeCopy`（半透明の写しに自作のプラグインを付け直す）を足し、breakables.ts が薄れ始めに写しへ替え、元に戻すときに戻す
