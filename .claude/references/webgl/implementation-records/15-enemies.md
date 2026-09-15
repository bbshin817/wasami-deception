---
title: 敵 AI（ナビゲーション格子・敵の頭脳・シーン上の敵）
sources:
  - src/enemy/navgrid.ts
  - src/enemy/brain.ts
  - src/enemy/enemies.ts
  - tests/navgrid.test.ts
  - src/enemy/jumpscare.ts
  - tests/enemy-brain.test.ts
  - tests/jumpscare.test.ts
updated: 2026-09-15
---

# 敵 AI（ナビゲーション格子・敵の頭脳・シーン上の敵）

## 役割
ステージ（Chaotic Customer 2 の Zone_1）の迷路を徘徊してプレイヤーを追う敵（ワサミの姿のクローン 10 体。ファンゲームの `Spawn_Enemies` の 10 体のマネキンの代わり。ホールのトリガーの 1 s 後に出る）と、プレイヤーへまっすぐ向かう「特別な」敵のプール（Hotel の終盤のサルに使っていた。ステージの脱出の追手に使う予定で、いまは 0 体）。迷路の敵は原作の Murder Monkeys に倣う（参考資料 `.claude/references/dark-deception/03-enemies-ai.md`）: 最短経路で追い、全力疾走には追いつけず、振り向くのが苦手で、急な方向転換では止まって「グリッチ」してから向きを変える。足音で接近がわかる。
- `navgrid.ts`: レベルの衝突箱（`COL_*` の AABB）から歩ける格子を作り、A* の最短経路（8 近傍、角のすり抜けなし、糸引きで間引いた折れ線）と格子上の視線判定を提供する。
- `brain.ts`: 1 体ぶんの状態機械（徘徊・追跡・捜索・グリッチ・捕獲）と移動。
- `enemies.ts`: シーン上の敵。モデルのクローン、アニメのブレンド、位置付きの音、捕まえたときの詰め寄り。
- `jumpscare.ts`: 捕獲のカメラシェイク（原作の館の Death Event がジャンプスケアと一緒に掛ける `JumpscareShake`）。Babylon 非依存。
- `navgrid.ts`・`brain.ts` は Babylon を import しない（`brain.ts` は navgrid を型だけ import）ので `node --test` で検証できる。

## 公開インターフェース
### navgrid.ts
- `interface NavBox { min: [x, y, z]; max: [x, y, z]; floor: boolean }` — Babylon のワールド座標（y が上、床は y = 0）。`floor` の箱は上のセルを歩けるようにし、それ以外の箱はセルをふさぐ。
- `interface NavOptions { cell; radius; height; step?; floorY? }` — 床の高さ `floorY`（既定 0。上面がそこから 0.3 m 以内の床の箱を床とし、ほかの高さもそこから測る）、セルの一辺（m）、エージェントの半径（ふさぐ箱をこれだけ太らせる）、高さ（これより上にしかない箱 = 戸口のまぐさはふさがない）、段差（既定 0.05。これより下にしかない箱はふさがない）。
- `interface Point { x; z }`
- `class FloorGrids` — 床の段ごとの格子。`static fromBoxes(boxes, opt, surface?)`（床の箱の上面を、群の最も低いものから 0.3 m 以内で段にまとめ、段の高さ = 上面の平均。段ごとに `floorY` を段の高さにした `NavGrid.fromBoxes`。`surface` を渡すと高さ 0 の段にそれを使う）、`levels`（`{ y, grid }`）、`walkable(x, y, z)`（`y` から 0.3 m 未満の段のうち最も近い段の格子。無ければ false）。テレポートの照準の判定に使う（05 記録）。敵の格子は地上の段だけ。
- `class NavGrid`
  - `static fromBoxes(boxes, opt)`、`readonly walk: Uint8Array`（1 = 歩ける。添字 `j * cols + i`、i が x、j が z）、`minX`, `minZ`, `cols`, `rows`, `cell`。
  - `indexOf(x, z)`（格子外は −1）、`centre(index)`、`walkable(x, z)`。
  - `nearestWalkable(p, maxCells = 8)` — 歩ける点はそのまま、ふさがった点は `maxCells` セル以内で最も近い歩けるセルの中心（無ければ null）。
  - `lineOfSight(a, b)` — 線分が通るセルがすべて歩けるか。
  - `findPath(from, to, maxExpand = 150000)` — `from` の後の折れ線の点列（最後は `to`、`to` がふさがっていれば最寄りの歩けるセルの中心）。届かない、または展開セル数が `maxExpand` を超えたら null。
- `pathLength(from, path)` — `from` から点列をたどった長さ。

### brain.ts
- `type EnemyMode = 'wander' | 'chase' | 'search' | 'stagger' | 'stun' | 'caught'`、`type EnemyRole = 'chaser' | 'ambusher'`、`type BrainEvent = 'spotted' | 'lost' | 'stagger' | 'caught'`
- `interface BrainConfig` — `CONFIG.enemy` のうち頭脳が使うキー（下の設定表の「頭脳」）。
- `interface PlayerInfo { x, z, vx, vz, noisy }`（`noisy` = ダッシュ中）、`interface BrainContext { waypoints, others, frenzy }`
- `wrapAngle(a)` — [−π, π) に収める。
- `class EnemyBrain(nav, cfg, role, spawn, yaw = 0, rng = Math.random)`
  - 公開フィールド: `mode`, `x`, `z`, `yaw`（Babylon の yaw。0 = +Z、π/2 = +X）, `speed`, `path`, `lastKnown`, `distance`（前回の更新でのプレイヤーとの直線距離）
  - `update(dt, player, ctx): BrainEvent[]`、`reset(spawn, yaw = 0)`（硬直も解く）
  - `stun(time, delay = 0)`（特殊シャードのオーブ。原作の `Set State(Stun)`。下の「硬直」。`caught` では何もしない）、`get stunned`（止まっている、または止まる前の `delay` の間）

### enemies.ts
- `interface EnemyActor { brain, root, spawn, spawnYaw, anims, meshes, active, special, step, call, alerted }` — `active`: 動いて感知して描かれる（プールの特別な敵、出番の前の迷路の敵は false）。`special`: 出したときからプレイヤーへまっすぐ向かう（常に感知）。
- `class Enemies(scene, container, nav, spawns, waypoints, audio, specials = 0)` — `specials` は特別な敵のプールの数（game.ts はステージで `game.stage.specials` 20、デバッグフィールドで 0）。
  - 頭脳には `CONFIG.enemy` の写し `brainConfig` を全体で共有して渡す。`speedScale`（set / get、`{ walk, chase }`）は `brainConfig.walkSpeed = CONFIG.enemy.walkSpeed × walk`、`brainConfig.chaseSpeed = CONFIG.enemy.chaseSpeed × chase` で、全員の巡回と追跡の速さにすぐ効く。設定の DIFFICULTY が EASY なら `{ walk: 0.5, chase: 300 / 430 }`（原作の `BP_Monkey` が EASY で Walk Speed を 200 → 100、Run Speed を 430 → 300 にする。`game/settings.ts` の `enemySpeedScale`。アニメの歩き・走りの配分は CONFIG のまま）。
  - `readonly actors: EnemyActor[]`、`readonly nav: NavGrid`、`onSpotted: (actor) => void`
  - `update(dt, feet, sprinting, frenzy): { caught: EnemyActor | null; chasing: boolean }`
  - `lunge(actor, feet, dt)`、`reset(active = true)`（迷路の敵を出現位置に戻して `active` にし、特別な敵をすべてプールへ）、`setMazeActive(on)`、`spawnSpecial(at: Vector3, yaw = 0): EnemyActor | null`（プールの 1 体を出す。空なら null）、`clearSpecials()`、`stunAll(time, notice)`（特殊シャードのオーブ: `active` な全員に `brain.stun(time, random × notice)`。原作の `Set State(Stun)` と、行動ツリーのサービスの間隔の中のどこで気づくか）、`showcase(at: Vector3 | null)`

### jumpscare.ts
- `JUMPSCARE_SHAKE`: 原作の `Blueprints/Main/JumpscareShake`（pak_reference の `_camera/_camera_shakes.json`）を 01_Hotel の Death Event の `ClientPlayCameraShake(JumpscareShake, 0.3)`（@52699）の倍率で。`scale` 0.3、`duration` 1、`blendIn` 0.2、`blendOut` 0.2（アセットに無いので UE の既定）、振幅（°）と UE の「周波数」（rad/s。teleport-fx.ts と同じ読み）: `pitch` [2, 40]、`yaw` [2, 50]、`roll` [4, 35]、`fov` [0.1, 3]。初期位相は `EOO_OffsetZero`（どの軸も 0）。
- `jumpscareShake(s): ShakeOffsets`（`{ pitch, yaw, roll, fov }` 度）: 捕獲から `s` 秒の `振幅 × sin(周波数 × s) × 0.3 × min(1, s / 0.2) × min(1, (1 − s) / 0.2)`。0 未満・1 以上は全部 0。game.ts の物理後フックが捕獲中にカメラへ足す（記録 04）。
- 原作の館は、捕まると敵を全部消し、入力を止め、タブレットを下ろし、別室のジャンプスケア用カメラ（`JumpscareCam`）と巨大な猿（`JumpscareMonkey`、`Monkey_Killshot_*` 2.93 s）の 3 本のシーケンスから重複なしのランダムで 1 本を再生し、3.5 s 後（@52764）に死亡画面を出す。本作は猿のモデルとアニメを使わないので、捕まえた敵の詰め寄り（`lunge`）とこのシェイクを同じ 3.5 s（`CONFIG.enemy.catch.time`）続ける。

## 内部構造と処理の流れ
### ナビゲーション格子
- **作成（fromBoxes）**: `floor` かつ上面が |y − floorY| < 0.3（`floorY` は既定 0）の箱を床とし、その x/z の外接矩形を格子の範囲にする。床の箱に中心が入るセルを 1 にし、床以外で縦の範囲が (floorY + step, floorY + height) と重なる箱を x/z に `radius` だけ太らせて、中心が入るセルを 0 にする。セルの中心が区間 [a, b] に入る添字は `ceil((a − min) / cell − 0.5)`〜`floor((b − min) / cell − 0.5)`。現行のホテル（迷路と、x を 100 m ずらしたロビー。12 記録）は `cell` 0.35 m で 465 × 296 セル、歩けるセル 15,886（廊下は幅約 2 m なので、半径 0.4 m で太らせると歩けるのは中央の約 1.2 m）。
- **視線（lineOfSight）**: セル単位の DDA（Amanatides–Woo）で線分が通るセルを順にたどり、ふさがったセルがあれば false。ちょうどセルの角を通るときは両脇のセルも歩けなければ false（2 つのふさがりの間をすり抜けない）。終点がセルの境界に乗るとき（次の境界までの t が 1 以上）はそこで終点のセルを調べて終える（角の上の終点で斜めに行き過ぎないため）。
- **経路（findPath）**: 始点と終点を `snap`（歩けるセル。ふさがっていれば 8 セル以内で最も近い歩けるセル）に合わせ、同じセルなら `[goal]`。A*（8 近傍、斜めのコスト √2、斜めは両脇の直交セルが歩けるときだけ、ヒューリスティックは octile 距離）。g・親・訪問・確定の配列は格子と同じ長さの型付き配列を使い回し、世代番号で毎回の初期化を省く。優先度付きキューは自前の 2 分ヒープ。最後に糸引き（`pull`）: 基点（`from` が歩ければそのもの、でなければ始点セルの中心）から視線が通る最も遠いセルへ跳ぶことを繰り返す。現行の館での呼び出しは平均 0.27 ms、最悪 8.1 ms（全シャードを回る経路の見積もりで 586 回、M4 Pro のヘッドレス Chrome）。

### 敵の頭脳（EnemyBrain.update）
1. `caught` なら何もしない。`distance` を更新し、グリッチのクールダウンを減らす。
2. **知覚**（0.15 s ごと、`senses`）: frenzy なら常に感知。直線距離 < `senseRadius`（14 m。ダッシュ中は × `noiseScale` 1.4 = 19.6 m）なら壁越しでも感知（気配）。それ以外は `sightRange`（32 m）以内・向きから ±`sightFov`/2（70°）以内で、プレイヤーの最寄りの歩けるセルまで視線が通れば感知。感知したときだけ `lastKnown` を更新する（判定の合間にテレポートした先を覚えない）。
3. 感知中は `unseen = 0`、wander / search なら chase にして `'spotted'`。感知していない chase / stagger の間は `unseen` を足し、chase で `loseTime`（5 s）を超えたら search にして経路を捨て `'lost'`。
4. **捕獲**: stagger 以外で、硬直（`stunned`）でもなく `distance < catchRadius`（0.95 m）なら caught にして `'caught'`。壁越しの判定はしない（壁は 0.37 m 以上の厚みがあり、敵は壁から 0.4 m、プレイヤーは 0.32 m より近づけないので、壁を挟むと中心間は 1.09 m 以上になる）。
5. **stagger 中**: 速度を 3 × `accel` で落とし、`stagger.time`（0.75 s）後に chase（`unseen > loseTime` なら search）へ戻る。移動しない。
   - **硬直**（`stun(time, delay)`。原作の `BP_PowerOrb` がタグ Enemy の全員に `Set State(2 = Stun)`、値は `world/special-rules.ts` の `SPECIAL.stun`。08 記録）: 呼ばれた時点から捕まえない（原作の `BP_Monkey` は State ≠ Stun のときだけ Death Event を呼ぶ）。`delay` 秒は今の動きを続け（原作の行動ツリー `DD_BehaviorTree_Monkey` は、サービス `BTS_ActorInRange` が 1 s ごとに状態を黒板に写して Stun なら IsInRange を切るまで今の枝を続ける）、その後 `mode = 'stun'` にして経路を捨てる。stun の間は毎更新の最初（距離の計算の直後）に速度を 3 × `accel` で落として止まり、感知も移動もしない（原作の BTT_SetSpeed_Copy が速さ 0）。`time` 秒（原作の Wait 15）で wander に戻し、`target`・`lastKnown`・感知の状態を消す（原作の BTT_SetEnemyState で Patrol）。
6. **計画（plan）**:
   - chase: 目的地は、感知中ならプレイヤー（ambusher は 7 m より遠ければ `intercept`）、感知していなければ `lastKnown`（視線と距離を切ると、最後にいた場所へ向かう）。3 m 以内で視線が通れば直接 `[goal]`、それ以外は `repath`（0.4 s）ごとか経路が空なら `findPath`（失敗したら前の経路のまま）。
   - search: `lastKnown` まで 1.2 m より遠ければ経路をたどる（空なら repath ごとに探索）。着いたか届かなければ wander へ。
   - wander: 待機中（`idleTimer`）はそのまま。経路が残っていればそのまま。着いたら 40% の確率で `idle`（1〜3 s）待つ。次の巡回点は `pickWaypoint`: 4 m より遠い巡回点から、半分の確率でプレイヤーに近い 6 点、残りは全体から無作為（群れがプレイヤーの方へ寄っていく）。
   - `intercept`（ambusher の先読み）: プレイヤーの最寄りの歩ける点から、速度 × `lookahead`（2 s）を 8 分割してたどり、歩けて視線の通る最も先の点。
7. **操舵（steer）**: 0.3 m 以内の経路点を捨て、次の点への向き `desired` と現在の `yaw` の差 `diff` を取る。chase で `speed > 2`、クールダウン 0、|diff| > `stagger.angle`（100°）なら stagger にして `'stagger'`（原作の振り向きの弱点。テレポートで背後へ抜けると起きる）。それ以外は `turnRate`（chase 4.5 / wander・search 2.5 rad/s）で向きを変え、目標速度 = chase: `chaseSpeed`（4.3）、search: `chaseSpeed` × 0.8、wander: `walkSpeed`（2.0）。これに × max(0, cos(向き直した後の差))（向き直るまで遅い）。加速は `accel`（8 m/s²）、減速はその 1.5 倍。
8. **移動**: 経路点へ向かって（向きではなく）`speed × dt` 進む（点を越えない）。他の敵と 1.1 m 以内なら半分ずつ押し離す。移動先が歩けなければ x だけ、z だけの順に試し、どれも駄目なら止まって再計画（chase 以外は経路を捨てる）。
- 速度の関係: プレイヤーの歩き 3（原作の 300 cm/s）< 追跡 4.3（原作の BP_Monkey の Run Speed 430 cm/s。近づいても速くならない）< ダッシュ 6（原作の 600 cm/s。ブースト 8.7）。歩いていると追いつかれ、走れば引き離せる。フレンジーでも速さは変わらない（以前の 1.15 倍は 2026-09-14 にユーザーの指示で撤廃。原作データの全回収はジャンプスケアのサルのマテリアルを変えるだけ）。

### シーン上の敵（Enemies）
- **生成**: モデルのコンテナの PBR マテリアルの `emissiveColor` を `emissive`（0.12）の灰色にする（glTF は自分のテクスチャで全面を自己発光させていて、そのままでは暗い館で平たく光る）。`maxSimultaneousLights = lights.shadowCasters + 1`。`enemy.enabled` なら `min(count, spawns.length)` 体を作る。各体は `instantiateModelsToScene(name => 'enemy{i}_' + name, false, { doNotInstantiate: true })`（スキンとアニメを複製、マテリアルは共有）を親 `TransformNode('enemy{i}')` に付け、glTF のルートの `scaling` に `height / 1.7`（2.1 m）を掛ける。メッシュは `isPickable = false`。アニメは idle / walk / run だけを `start(true)` で始めて重みでブレンドし、skip / spin は破棄する。重み 0 のグループも全チャンネルを毎フレーム動かしてしまうので、生成時に idle 以外は `pause()` し、`blend` で重みが正になったら `play(true)`（一時停止した位置から再開）、0 になったら `pause()` する。3 体目ごと（i % 3 === 2）が ambusher。最初の向きは巡回点の重心の方。続けて特別な敵を `specials` 体、同じ作り方で作って隠す（`setActive(false)`: `root.setEnabled(false)` とアニメの `pause()`）。特別な敵の頭脳には `specialConfig`（`CONFIG.enemy` の写しに `chaseSpeed = game.stage.specialSpeed`（4.25。原作の BP_MonkeySpecial2 の 425 cm/s））を渡し、役割は chaser。モデルの正面は glTF の +Z（つま先と顔の向きで確認）で、Babylon の読み込み後も +Z なので `modelYaw` は 0。
- **update**: プレイヤーの速度を足の位置の差分から推定する（±12 m/s で切り、指数平滑 6/s。テレポートの跳びを抑える）。`active` の体だけ、`brain.update`（`others` は他の `active` な体の頭脳、`frenzy` は引数か、特別な敵なら常に true = 常に感知して追う）→ イベント処理（`react`）→ アニメと位置（`animate`）。
- **spawnSpecial(at, yaw)**: プールの最初の隠れた特別な敵を `brain.reset(at, yaw)` して置き、`setActive(true)`（有効化と idle のブレンド）。game.ts がエレベーターの中で扉を破るサル（`DoorMonkey`）と、ロビーの 6 基の出現点から 2 体ずつ出す（04 記録）。`clearSpecials()` は全部をプールへ戻す。
- **setMazeActive(on)**: 迷路の敵を出し入れする（特別な敵は除く）。戻り値の `caught` は最初に caught になった体、`chasing` は chase か stagger の体があるか。
- **アニメ（blend）**: 速度 s から `move = clamp(s / 0.8)`、`run = clamp((s − walkSpeed) / (0.9 × chaseSpeed − walkSpeed))`、重み idle `1 − move`、walk `move × (1 − run)`、run `move × run`（`AnimationGroup.weight`。重み 0 のグループは `pause()`、正なら `isPlaying` でなければ `play(true)`）。`speedRatio` は walk が s / `clipSpeed.walk`（1.8）を 0.5〜2、run が s / `clipSpeed.run`（4.6）を 0.6〜1.8 に。stagger 中は `speedRatio 0`（姿勢が止まる）で、位置を ±4.5 cm、向きを ±0.3 rad、毎フレームランダムにずらす（グリッチ）。stun 中は idle（重み 1）を `speedRatio` 0.35（`STUN_SWAY.rate`）で流し、根を周期 1.29167 s（原作の `Monkey_Stunned` の長さ）でロール ±0.12 rad・ピッチ ±0.05 rad（半分の周波数）に揺らす（位相は出現位置の x から。本作のモデルに気絶のアニメが無いための代わり）。それ以外では根のロール・ピッチを 0 に戻す。
- **音**（`AudioManager` の位置付き再生。HRTF、refDistance 1.2 m、rolloff 1.6、max 40 m）:
  - 足音 `enemy_fs_1..12`（原作の Murder Monkeys の足音）: 速度 0.5 m/s 超で、間隔 = 歩幅（走り 1.6 m / 歩き 0.95 m）÷ 速度（0.24〜0.7 s）、音量 走り 1.1 / 歩き 0.75。
  - 発見: `'spotted'` で、どの敵からでも 12 s に 1 回まで、ボイス `found`（「ここか！」）をその敵の頭の位置で鳴らし、`onSpotted` を呼ぶ（game.ts が字幕を出す）。
  - 警告: chase 中に `alertRange`（7 m）以内に入ったら、1 回の追跡につき 1 回、全体で 8 s に 1 回まで `enemy_alert`（0.75、定位なし）。wander に戻るとリセット。
  - 巡回中と硬直中の呼びかけ（原作の Random Idle Sounds は Stun でも巡回と同じ 5〜10 s ごと）: `vocal`（14〜26 s）ごとに `calling` / `others` / `think` / `remember` のどれか（0.9）をその敵の位置で。
- **frenzy**: 見た目は変えない（以前の体の赤い発光 `frenzyEmissive` は 2026-09-14 にユーザーの指示で撤廃）。フレンジーで変わるのは、迷路の敵が常にプレイヤーを感知することだけ。
- **lunge(actor, feet, dt)**（捕獲の演出。game.ts が毎フレーム呼ぶ）: 敵とプレイヤーを結ぶ線上で、プレイヤーから `catch.reach`（0.75 m）の点へ `1 − exp(−16 dt)` で寄り、プレイヤーの方を向いて run を `speedRatio 1.8` で回す。
- **reset(active)**: 全員を出現位置・最初の向き・wander に戻し、足音と呼びかけのタイマー、警告、アニメの重み、速度の推定を初期化し、迷路の敵は `active`、特別な敵は隠す（NEW GAME・RESUME・リスポーン。チェックポイント 1 と 4 では `reset(false)`）。
- **showcase(at)**: 事前描画用に 1 体目を `at` に −Z 向きで立たせる（null で出現位置に戻す）。

## 依存関係
- navgrid.ts: なし。brain.ts: `./navgrid`（型のみ）。enemies.ts: `@babylonjs/core`（`TransformNode`, `PBRMaterial`, `Color3`, `Vector3`、型 `AnimationGroup` / `AssetContainer` / `AbstractMesh` / `Scene`）、`../audio/audio`、`../config`、`./brain`、`./navgrid`（型）。
- 使う側: `src/game/game.ts`（`NavGrid.fromBoxes(level.navBoxes, …)` と、テレポートの照準の判定の `FloorGrids.fromBoxes`（地上の段は敵の格子）、敵のモデルの読み込み、`Enemies` の生成・`update`・`lunge`・`reset`・`showcase`・`onSpotted`、`active` な敵の体（`brain.x`・`brain.z`・`root.position.y` と `enemy.radius`・`enemy.height`）で壊せる物とロックピック・ダッシュの障壁に触れさせる（07 記録の `Breakables.touch` と `touches`）、デバッグ API の `enemies()` / `enemyPose()` / `navPath()` / `navInfo()`。記録 04）。`src/world/level.ts` が `navBoxes` / `enemySpawns` / `waypoints` を返す（記録 07）。モデルは `public/assets/models/wasami_enemy.glb`（記録 13）、出現位置 `EnemySpawn_NN` と巡回点 `Waypoint_NN` は Blender が置く（記録 12）。
- jumpscare.ts: なし。使う側は `src/game/game.ts`（物理後フック）。
- テスト: `tests/navgrid.test.ts`、`tests/enemy-brain.test.ts`、`tests/jumpscare.test.ts`。E2E は `scripts/enemy-check.mjs`（記録 14）。

## 設定・調整値（`CONFIG.enemy`、URL クエリで上書き可）
| 区分 | キー = 既定値 |
| --- | --- |
| 出現 | `enabled` true、`count` 10、`model` `'assets/models/wasami_enemy.glb'`、`height` 2.1、`modelYaw` 0 |
| 格子（game.ts） | `navCell` 0.35、`radius` 0.4、`navHeight` 2.0 |
| 頭脳 | `walkSpeed` 2.0、`chaseSpeed` 4.3（原作の BP_Monkey）、`accel` 8、`turnRate` {wander 2.5, chase 4.5}、`senseRadius` 14、`noiseScale` 1.4、`sightRange` 32、`sightFov` 140、`loseTime` 5、`catchRadius` 0.95、`stagger` {angle 100, time 0.75, cooldown 2}、`repath` 0.4、`lookahead` 2、`idle` [1, 3] |
| 見た目と音 | `clipSpeed` {walk 1.8, run 4.6}、`emissive` 0.12、`vocal` [14, 26]、`alertRange` 7 |
| game.ts | `chaseHold` 3（最後の敵が追跡をやめてから追跡フラグを保つ秒数）、`catch` {reach 0.75, time 3.5（原作の館の Delay 3.5。捕獲から死亡画面まで）} |
| 特別な敵 | `game.stage.specialSpeed` 4.25（原作の BP_MonkeySpecial2 の 425 cm/s） |
- 迷路の敵の速さ（巡回 200・追跡 430 cm/s、EASY で 100・300、`BP_Monkey` の `Default__BP_Monkey_C` と BTT_SetSpeed @726 / @564）と加速度（MaxAcceleration 800 cm/s²）と特別な敵の速さは原作の値。それ以外の頭脳の値（感知・視界・見失うまで・旋回・stagger・捕獲距離・巡回の待ちと行き先）は原作の値ではない（参考資料の [B] は挙動の性質だけ）。原作の BP_Monkey は視覚・聴覚を持たず、1 s ごとに水平 60 m 以内なら追い（BTS_ActorInRange）、巡回はプレイヤーの 45 m 以内のランダムな点へ 0〜2 s 待って向かい（BTT_FindPointInRadius）、旋回は 125°/s。

## 既知の制約・注意点
- 敵は影を落とさない（影付きライトのシャドウマップは、ライトが移ったときと門が動く間しか描き直さないため）。物理ボディも持たないので、プレイヤーは体をすり抜けられるが、中心間 0.95 m で捕まる。テレポートの照準やタブレットの奥行きのレイは物理なので、敵に当たらない。壊せる物（07 記録）に触れる判定だけは、game.ts が足元の x・z から ±`radius` 0.4 m・高さ `height` 2.1 m の直立の体と区画の箱の重なりで見る（`Breakables.touch`。原作の `BP_01_Woodboards` の `Box` にタグ `Enemy` のポーンが重なると壊れる）。
- 格子は読み込み時に 1 回だけ作る。門の衝突箱もふさいだままなので、敵は出口の小部屋に入らない。
- 気配（壁越しの感知）は直線距離なので、壁の向こうでも 14 m（ダッシュ中 19.6 m）以内なら気づかれる。原作の Murder Monkeys が館内でプレイヤーの位置をある程度わかっているのに倣いつつ、広い迷路に 3 体いるので距離つきに弱めた。
- `findPath` は同期。追跡中の各体は 0.4 s ごとに探索する。巡回の経路（館の端から端まで）は着くまで 1 回だけ探す。
- 迷路の出現位置は原作の `MonkeySpawner2/3/4`（迷路の 3 隅。12 記録）。原作はトリガーの 6.5 s 後に出すが、本作は開始から出現位置で巡回する。ロビーの特別な敵の出現位置は `BP_MonkeySpawner` 6 基（エレベーターのかごの中。扉は game.ts が開く）。
- 3 体の頭脳は独立していて、役割の違いは ambusher の先読みだけ。原作 1.1.2 の「散開して挟み撃ち」は、プレイヤー寄りの巡回点の選び方と先読みで近いものにしている。
- ボイスの `found` / `calling` / `others` / `think` / `remember` / `you` は、案内役のワサミの字幕付きボイスと同じ音声を使う（敵もワサミ）。
- ポーズ中は game.ts が `scene.animationsEnabled = false` にしてアニメを止める（敵の頭脳も更新しない）。
- 1 回きりの位置付きの音は鳴らした時点の位置で固定（足音やボイスは短いので敵を追わない）。

## テスト
### tests/navgrid.test.ts（7 件）
10 × 10 m の部屋を x = 5 の壁（z 0〜7、隙間 z 7〜10 の上にまぐさ）で仕切り、低いベンチと離れた床の島を置いた格子（cell 0.25、radius 0.4、height 2）で、床と壁・ベンチ・まぐさの判定、壁で視線が切れること、隙間を回る経路（各区間に視線、長さ 12〜13.5 m、4 点以下）、一直線なら 1 点、届かない島は null・壁の中の始点と終点は最寄りへ寄せる、`nearestWalkable`、展開の上限を検証する。

### tests/enemy-brain.test.ts（8 件）
`CONFIG.enemy` と同じ値をテスト内に固定し、長さ 200 m・幅 4 m の廊下（x = 100 に 1.5 m の隙間を残す壁）の格子（cell 0.35）で、決定的な乱数を使って検証する: `wrapAngle`、静止したプレイヤーを 10 m 先で聞きつけて 3 s 未満で捕まえる、背後 18 m の静かなプレイヤーには気づかずダッシュなら気づく、ダッシュで逃げるプレイヤーには追いつけず差が 9 m を超える、追跡中に背後へテレポートされると 2.0〜2.4 s にグリッチしてからしか捕まえない、ambusher の目的地が走るプレイヤーの前方、見失ってから（5〜5.5 s）捜索を経て巡回に戻る、隙間を通って巡回点の間を往復し歩ける範囲を出ない。

敵の硬直は `tests/specials.test.ts`（08 記録）の 2 件: 追跡中に `stun(15, 2)` すると 2 s はプレイヤーに届いても捕まえず、その後 15 s は動かず速度 0、明けると wander に戻ってその場のプレイヤーを捕まえる。`delay` 0 ならすぐ stun、`reset` で解ける。

### tests/jumpscare.test.ts（1 件）
0 s と 1 s（と負の時刻）で全部 0、0.5 s で振幅 × 0.3 × sin(周波数 × 0.5)、ブレンドの中ほど（0.1 s・0.9 s）で半分の重み、ロールの最大が 1.2° 以下で 1.15° 超。

## 変更履歴
- 2026-09-11: 初版（ナビゲーション格子 `navgrid.ts` とテスト）
- 2026-09-11: 敵の頭脳 `brain.ts`（徘徊・追跡・捜索・グリッチ・捕獲、ambusher の先読み）とシーン上の敵 `enemies.ts`（モデル・アニメのブレンド・位置付きの音・詰め寄り）、`tests/enemy-brain.test.ts` を追加
- 2026-09-12: 重み 0 のアニメーショングループを `pause()` し、重みが付いたら `play(true)` で再開するようにした（9 グループが重み 0 でも全チャンネルを毎フレーム動かしていた。再生中は 9 → 3）
- 2026-09-12: 設定の DIFFICULTY の EASY で巡回の速さを半分にする `walkScale` を追加した（頭脳に渡す設定を共有の `brainConfig` にした。原作の `BP_Monkey`）
- 2026-09-13: 原作の Hotel の終盤のため、特別な敵のプール（`specials`、`EnemyActor.active` / `special`、`spawnSpecial` / `clearSpecials` / `setMazeActive`、`reset(active)`）を加えた。特別な敵は常に感知して `game.hotel.specialSpeed` で追う
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 にした（ユーザーの指示）: 特別な敵の追跡の速さを `game.stage.specialSpeed` から読む。敵は `enemy.count` 10 体を colliders.json の出現点に（game.ts がホールのトリガーの後に `setMazeActive(true)`、チェックポイント 2 の再開では `reset(true)`、ホームと脱出では `reset(false)`）
- 2026-09-14: ステージの特別な敵のプールを 20 にした（game.ts: 罠の扉が壊れるとその奥から `spawnSpecial`。脱出の群れも同じプールから）
- 2026-09-13: 捕獲を原作の館に倣った: `jumpscare.ts`（`JUMPSCARE_SHAKE`、`jumpscareShake`）と `tests/jumpscare.test.ts` を追加し、`catch.time` を 1.4 → 3.5（原作の Delay 3.5）にした。捕獲の後は死亡画面（記録 04・10）
- 2026-09-13: 原作の特殊シャードのオーブのため、頭脳に硬直 `stun(time, delay)` / `stunned` と `EnemyMode` の `'stun'` を足した（捕まえない、少し後に止まって 15 s 立つ、巡回に戻る）
- 2026-09-13: `Enemies.stunAll(time, notice)` と、硬直中の見た目（idle をゆっくり流して根を揺らす）・呼びかけを足した
- 2026-09-14: ユーザーの指示でフレンジーの敵の強化を撤廃した: 1.15 倍の速さ（`frenzySpeed`）と体の赤い発光（`frenzyEmissive`・`setFrenzy`）を削除。フレンジーは常に感知するだけ
- 2026-09-15: パンチを敵に効かせた（ファンゲームのマネキンの `Death_by_punch`）: `knockdown.ts`（`KNOCKDOWN`・`Knockdown`・`respawnLottery`）と `tests/knockdown.test.ts`、頭脳の `'down'` と `knock()`、`Enemies` の `punch`・予備の個体 `spares`・`respawnPoints`・出現待ち・後ろへ倒れる姿勢と溶ける visibility・出現地点に現れて 2 s 立つ新しい個体。設定表の `count` を 10 に直した
- 2026-09-15: `NavOptions.floorY`（床の高さ。既定 0）と、床の段ごとの格子 `FloorGrids`（テレポートの照準の判定。05 記録）を足した。敵の格子は変わらない
- 2026-09-15: 迷路の敵の速さを原作の BP_Monkey の値（巡回 2.0・追跡 4.3 m/s）にし、原作に無い近づいたときの加速（`closeBoost`・`closeRange`）を外した。EASY は巡回と追跡の両方を遅くする `speedScale`（`{ walk: 0.5, chase: 300 / 430 }`）にした（以前の「追跡は変えない」は誤り）
- 2026-09-15: 本作の規範（本家に無いギミックは複雑にしない）とユーザーの選択で、パンチで敵が倒れて出現地点に新しい個体が出る仕組みをやめた: `knockdown.ts`・`tests/knockdown.test.ts`、頭脳の `'down'` と `knock()`、`Enemies` の `punch`・`updateDown`・`updateRespawns`・`setVisibility`・`respawnPoints`・出現待ち・予備の個体（`spares` の引数、`EnemyActor` の `knock`・`appear`・`spare`）と倒れる姿勢を削除した。敵は壊せる物（07 記録）に触れると壊す（game.ts）。役割の「近づくと少し速くなる」の記述を消した（`closeBoost` は同日に外している）
- 2026-09-15: game.ts の `active` な敵がロックピックとダッシュの障壁の箱に重なっても壊れるようになった（07 記録の `touches`。04 記録）ことを「使う側」に記した（このファイルのソースは変更なし）
