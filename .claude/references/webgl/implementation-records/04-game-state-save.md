---
title: ゲーム進行・状態・セーブ
sources:
  - src/game/game.ts
  - src/game/state.ts
  - src/game/results.ts
  - src/game/save.ts
  - src/game/settings.ts
  - src/game/traps.ts
  - tests/traps.test.ts
  - src/game/doors.ts
  - tests/doors.test.ts
  - src/game/escape.ts
  - tests/escape.test.ts
  - tests/state.test.ts
  - tests/results.test.ts
  - tests/settings.test.ts
updated: 2026-09-15
---

# ゲーム進行・状態・セーブ

## 役割
`Game` クラスがエンジン・物理・アセット・ワールド・プレイヤー・ポストプロセス・HUD・音声の全サブシステムを生成し、毎フレームの更新順を決めるオーケストレータ。
`GameState` は Babylon 非依存の純粋なルール（ステージ = ファンゲーム『Chaotic Customer 2』の Zone_1 の流れ: 地下鉄のホーム〈チェックポイント 1〉→ 階段の上のホールのトリガーで敵が出る〈チェックポイント 2〉→ シャード 301 の全回収でシャードの障壁が壊れる → その奥の配電盤で脱出フェーズ〈チェックポイント 4〉→ 脱出の終点／捕獲）とイベント通知を担い、`SaveStore` は localStorage への進行保存を担う。トリガー・障壁・配電盤の後の暗転は `Game` が進める。制限時間は無い（経過時間はクリアタイムにだけ使う）。2026-09-13〜14 は原作の Hotel の流れ（祭壇・指輪の欠片・エレベーター・ロビー・ポータル）だった（git の履歴にある）。

## 公開インターフェース
### src/game/game.ts
- `interface GameHooks { onProgress(fraction, label); onPauseChange(paused); onEnd(results); onDeath(lives); onStarted() }` — `src/main.ts` が実装し、ローディング表示・ポーズ画面・脱出の画面（`onEnd` は脱出の終点でゲームを止めてすぐ、`results.ts` のリザルトを渡す）・死亡画面（`onDeath` は捕獲の `enemy.catch.time` 後、ゲームを止めて残りのライフを渡す）を制御する。
- `class Game`（コンストラクタは private）
  - `static async create(canvas, hooks): Promise<Game>` — 全初期化。戻った時点で `hooks.onProgress(1, 'READY')` 済み。
  - `start(fresh = false)` / `pause()` / `resume()` / `restart(hard = false)` / `respawn()` / `newRun()` — タイトル・ポーズ・死亡画面からの遷移。`levelResults(): Results` — 今の走りの脱出の画面のリザルト。`start` は `fresh` なら `store.clear()` と `checkpointWarning.set(false)`（NEW GAME）、そうでなければ `restoreSave()`（RESUME）をしてから始める。
  - 公開フィールド: `state: GameState`, `store: SaveStore`, `settingsStore: SettingsStore`（キー `CONFIG.game.settingsKey`）, `settings: Settings`, `hud: Hud`, `stats: FrameStats`, `engine`, `api: GraphicsApi`, `scene`, `input`, `audio`, `level`, `player`, `teleport`, `lamps`, `shards`, `gate`（デバッグフィールドの門。ステージでは動かない）, `post`, `tablet`, `enemies`, `specials`（`SpecialShards`。ステージの赤いシャードとオーブ、デバッグフィールドは null。08 記録）, `checkpointWarning`（`SavedFlag`、キー `CONFIG.game.checkpointWarningKey`。LAST CHECKPOINT の警告に YES と答えたか。main.ts の死亡画面が読み書きし、NEW GAME で戻す）, `playing`, `paused`
  - `get hasSave` — `store.load()` が非 null か。
  - `saveSettings(settings)` — `settings` を写して保持し、`settingsStore.save` で書いて `applySettings` する（タイトルの OPTIONS の SAVE & EXIT）。ゲーム中（`playing`）なら `player.controls.lagSpeed = rotationLagSpeed`（Dark Deception の UMG_Options の SAVE & EXIT はプレイヤーのキャラクターがいるときだけ `Set Up Mouse Smoothing` を呼ぶ）。
  - private `applySettings(s)`（Set Settings と SAVE & EXIT の適用）: `audio.setVolumes({ music, sfx, voice: dialogue })`、`hud.voiceSubtitles = subtitles`、`player.controls` に `look: lookScale`・`invertY`・`headBob: headBobbing`・`toggleSprint`、`enemies.speedScale = enemySpeedScale`、`setRenderScale(engine, renderScale)`、`lamps.setQuality(qualityOf)`、QUALITY が今の `post.quality` と違えば `post.dispose()` して `new PostFx(scene, camera, quality)` に替える、`post.brightness = brightnessPower`。コンストラクタでも呼ぶ。
  - `post` は `public`（QUALITY で作り直すので readonly ではない）。
  - `window.__wasami` に `debugApi()` の戻り値を公開（ベンチマーク／スクリーンショット／検証用）: `state()`, `shards()`, `specials()`, `specialPlace(kind, i)`, `exit()`, `stage()`（ステージの場所。デバッグフィールドは null: `start`、`route`（開始地点からホールのトリガーまでの経路 `{ at }[]`〈床の上の点〉。07 記録の `Level.route`、`scripts/flow-check.mjs` が歩く）、`checkpoints`（`{ 1|2|4: { position, yaw, zone } }`）、`panel`、`panelBox`（視線で使う配電盤の AABB）、`triggers`（`{ chase, endChase, stairsTop, stairsSide, goal }` の AABB `{ min, max }`）、`barriers`（`{ source, kind, box, zone, broken }`。`zone` はファンゲームの `Box` の箱〈シャードの障壁がチェックポイント 1 で初めて触れると立つ〉）、`breakables`（壊せる物〈駅の柵の区画と板張りの通り道〉ごとの `{ name, box, broken, gone }`。`name` は柵なら `<柵のアクター>/<区画>`〈例 `Fence_battle_6/Cube1`〉、通り道なら `doorway` / `wayOut`、`gone` は壊れて消えた後。07 記録の `Breakables.state()`）、`floors`、`streetLamps`（街灯の状態 `StageLamps.mode`: `'flicker' | 'recover' | 'escape' | 'steady'`）、`jets`（`{ source, trigger, jet, firing }`）、`cars`（`{ source, units, parked, kill, unit, body, paint }`。`body` は車の最初の部品のワールドの位置、`paint` は塗りの色）、`doors`（`{ source, kind, state, target, plate, origin, solid }`。`state` は `'open' | 'shut' | 'lifted' | 'down' | 'broken' | 'whole' | 'locked'`）、`escape`（脱出中だけ: `{ t, hold, walls, fired, trucks: { path, distance, shown, stopped, at }[], crowd, boxes: { trucks: { path, box }[], more: box[] }, paths: { 名前: { length, start, mid, end } } }`。`at` は運び台の位置〈組み立ての置き場所からのずれ〉））, `enemies()`（各体の `role` / `special` / `active` / `mode` / `position [x, z]` / `spawn` / `yaw` / `speed` / `distance` / `stunned`。15 記録）, `enemyPose(i, x, z, yaw)`, `navPath(ax, az, bx, bz)`, `navInfo()`, `surface(x, y, z)`, `ray(x, y, z, dx, dy, dz, length)`（その点から向きを正規化して `length` m の Havok の光線をマスクなしで引き、`{ distance, by }`〈当たった体の transformNode の名前か null〉、当たらないか物理が無ければ null）, `start(fresh = false)`, `results()`, `setChase()`, `gradeZone(z)`（色補正のゾーンを強制する `forcedZone`。null でプレイヤーの位置に従う）, `collectAll()`（残りのシャードを全部回収する。検証の近道）, `teleport()`, `pose()`（最後に `player.snapView()`）, `face(yaw, pitch = 0)`（その場で `player.yaw` / `pitch` を変えて `snapView()` するだけ。`pose` はプレイヤーを動かし、立たせる）, `key()`, `look()`, `wheel()`, `loop(on)`, `step()`, `record()`, `stopRecord()`, `renderSize()`, `gpuMs()`, `perf()`（最後のフレームの描画の呼び出し数 `draws` と、直近 1 秒の CPU のフレーム時間 `cpuMs`）, `settings()`, `applied()`（`buses`、`voiceSubtitles`、`controls`、`enemySpeedScale`（`{ walk, chase }`）、`hardwareScaling`、`quality`（0..3）、`motionBlur`、`bloom`、`shadows`、`brightness`）, `scene`, `config`, `lamps()`（`index` / `source` / `position` / `glow`）。`state()` は `collected`・`remaining`・`elapsed`・`allShards`・`checkpoint`・`chase`・`caught`・`lives`・`streak`・`deaths`・`streakBest`・`hardRespawn`・`bonus`（赤いシャードを取ったか）・`reveal`（地図の表示の残り秒）・`revealed`・`collectFx`・`intro`（ステージ OP の秒、無ければ null）・`panel`（配電盤の後の秒、−1 はなし）・`controls`（`player.enabled`）・`usable`（手のマークの相手: `'panel'`・扉のアクター名・壊せる物の名前〈`breakables.nameOf`〉・null）・`player`・`yaw`・`fov`・`dash`・`steps`・`lights`・`boost: { time, cooldown }`・`audio`・`teleport: { state, cooldown, jumping, distance, target, fx }`・`barriers`（壊れた障壁のアクター名）・`gate`（デバッグフィールドの門が開いているか）・`arrow`（`{ visible, yaw, scale, color }`）。`loop(false)` で描画ループを止め、`step()` で 1 フレームずつ進める（`debug.fixedDt` と組み合わせて演出を撮る。WebGPU では描画ループの外で描いた画が画面に出ないので WebGL2 で行う）。

### src/game/state.ts
- `type Checkpoint = 1 | 2 | 4` — ファンゲームのキャラクターの `checkpoint` のうち本作にあるもの（1 ホーム、2 ホール、4 脱出。0 の列車の中と 3 は無い）。
- `interface SaveData { version: 3; collected: string[]; checkpoint: Checkpoint; bonus?: boolean; escaped: boolean; elapsed: number; deaths?: number; streakBest?: number; secrets?: number[]; savedAt: number }` — `checkpoint` は到達したチェックポイント（ファンゲームのセーブのスロット "checkpoint"。以後の再開はそのリスポーン位置）、`bonus` は赤いシャードを取ったか（以後は現れない）、`elapsed` は遊んだ秒数（クリアタイム）、`deaths` は死亡画面の数、`streakBest` は到達したシャード連続回収の最高の節目 0..10（この 2 つは脱出の画面のリザルトが読む）、`secrets` は取った秘密の ID（ステージに秘密は無い）。
- `interface GameEvents`（イベント名 → ペイロード）
  - `shardCollected`: `{ id, collected, remaining, streak }`（`streak` はこの 1 個を含む連続回収数）
  - `shardStreak`: `{ count, life }`（連続回収が節目に達した。`shardCollected` の後。`life` ならライフを足し済み）
  - `allShardsCollected`: `{ total }`（ファンゲームの shard = 0。シャードの障壁が壊れる）
  - `checkpoint`: `{ checkpoint: Checkpoint }`（2 はホールのトリガー、4 は配電盤）
  - `chaseChanged`: `{ chase }`
  - `escaped`: `{ elapsed }`
  - `panelDenied`: `{ remaining }`（シャードが残っているのに配電盤を使った。ステージでは障壁の奥なので起きず、デバッグフィールドの門で起きる）
  - `caught`: `{ lives }`（この捕獲の後に残るライフ。0 はゲームオーバー）
  - `specialCollected`: `{ kind: 'reveal' | 'stun' }`（特殊シャードを取った。`SpecialKind` は `world/special-rules.ts` の型）
  - `secretFound`: `{ id, fresh, count }`（秘密を取った）
- `SHARD_STREAKS: readonly { count, life }[]`（Dark Deception の `BP_DD_GameMode` の Check Streak の節目 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000。`life` は 200 と 500 だけ）、`MAX_LIVES = 6`
- `class GameState`
  - `constructor(shardIds: readonly string[], chaseOnAllShards = false, maxLives = 3)` — ID 重複で `Error('duplicate shard ids')`。`lives = maxLives` で始まる。
  - フィールド: `collected: Set<string>`, `elapsed`, `checkpoint`（1 から）, `escaped`, `caught`, `lives`（連続回収で最大 `MAX_LIVES` まで増える）, `maxLives`, `streak`, `bonusShard`, `deaths`, `streakBest`, `hardRespawn`（ゲームオーバーで LAST CHECKPOINT を使ったか。セーブされない）, `secrets: Set<number>`, `chase`（setter は変化時のみ `chaseChanged` 発火）
  - getter: `remaining`, `allShards`（`remaining === 0`）, `escaping`（`checkpoint === 4`）, `frenzy`（`chaseOnAllShards && allShards && !escaping`。ステージは偽、デバッグフィールドは Dark Deception のフレンジー）, `running`（`!escaped && !caught`）, `over`（`caught && lives === 0`）, `objective`（本作の目的の文言。ファンゲームの目的の文言はデータに無い）
  - `on(event, fn): () => void`（解除関数を返す）, `collect(id): boolean`, `reachHall(): boolean`, `usePanel(): boolean`, `tryEscape(): boolean`, `catchPlayer(): boolean`, `collectSpecial(kind): boolean`, `collectSecret(id): boolean`, `respawn(): boolean`, `tick(dt)`, `toSave(now?)`, `restore(save)`

### src/game/results.ts（脱出の画面のリザルト。Dark Deception の 01_Hotel のレベル BP が `UMG_LevelClear` に入れる値と、ウィジェットのバインド関数）
Babylon・DOM 非依存。画面は 10 記録の `hud/level-clear.ts`。
- `RANK_TEXT` = `['', 'C', 'B', 'A', 'S']`（`Enum_Ranks` の値 0..4 を `Get_*Rank_Text_0` が出す文字。0 は何も出さない）、`type Rank = 0..4`。`STREAK_MILESTONES` = `[0, 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000]`（`Enum_ShardStreaks` の値の順。値 0 が既定の `NewEnumerator10` "0"）。
- `interface RunStats { time; shards; bonusShards; secrets; deaths; hardRespawn; totalLives; streak; easy }`（レベル BP が読むもの: levelStruct の Time・BonusShards・Secrets・Deaths・Streak、GameInstance の `Used Hard Respawn?`、セーブの Total Lives、設定の難易度。`shards` は原作では文字列 '289' の定数）。
- `timeRank(s)`: 600 以下 S、720 以下 A、840 以下 B、それ以外 C（@38215 / @39088 / @39961 の LessEqual）。`timeText(s)`: `FromSeconds` → `BreakTimespan` の Minutes（1 桁以上）+ `' : '` + Seconds（2 桁）。分は時間の分の部分なので 1 時間を超えると折り返す（原作どおり）。
- `ROW_SHARDS`: 各行のランク（0, C, B, A, S）ごとの加算シャード（レベル BP の `*_Shards` の Select）。`time` [0, 10, 20, 30, 40]、`bonusShards` [0, 0, 0, 0, 10]、`secrets` [0, 0, 5, 5, 10]、`livesLost` [0, 5, 5, 10, 20]、`shardStreak` [0, 5, 10, 15, 20]。
- `results(stats): { rows, total, finalRank }`: 行は TIME（`timeText`・`timeRank`）、SOUL SHARDS（`shards` をそのまま・S 固定・加算は無し〈原作は `Shards_Shards` を設定せず、数え上げのイベントも何もしない〉で `shards: null`）、BONUS SHARDS（`N/1`、ランクは 0 → C・1 → S）、SECRETS（`N/2`、0 → C・1 → B・2 → S）、LIVES LOST（`hardRespawn` ならセーブの Total Lives、でなければ `deaths`。0 → S・1 → A・2〜5 → C・6 以上は Select の既定で 0（何も出ない））、SHARD STREAK（`STREAK_MILESTONES[streak]`、ランクは値 0..10 → C, C, C, B, A, A, S, C, C, C, C。@32706 の表どおりで、350 以上は C になる）。Select の範囲外は 0。`total` = 各行の加算 + `shards`（`Get_TotalShardAmount_Text_0`）、`finalRank` = 6 行のランクの和 ÷ 6 の切り捨てを 0..（`easy` なら 3、それ以外 4）に収める（`Get_FinalRank_Text_0`）。
- `COUNTER_FPS` 60、`class Counter(n, span)`: ウィジェットの `*Counter` イベントからの数え上げ（+1 をすぐ出し、`Delay(span / n)` ごとに次の数、n で止める。n 0 は +0）。UE の Delay は tick で終わり 1 フレームに 1 歩しか進まないので、1 歩を `ceil(span / n × 60) / 60` s（1 フレーム以上）とする（60 fps とみなす推定）。`step`・`length`（+n を出すまでの秒。ループ音はこの間鳴る）・`at(t)`（イベントから t 秒の数）。

### src/game/save.ts
- `interface KeyValueStorage { getItem; setItem; removeItem }` — テストで差し替え可能。
- `class SaveStore(key, storage = safeLocalStorage())` — `load(): SaveData | null`, `save(data): boolean`, `clear()`。
- `safeLocalStorage(): KeyValueStorage | null` — `localStorage`（参照で例外が出る・存在しない〈node〉なら null）。settings.ts も使う。
- `class SavedFlag(key, storage = safeLocalStorage())` — `get(): boolean`（値 `'1'` なら true。無い・読めなければ false）、`set(on)`（true で `'1'` を書き、false で消す）。LAST CHECKPOINT の警告に使う（進行のセーブとは別のキー。RESTART と脱出で進行のセーブを消しても残る）。

### src/game/settings.ts（プレイヤーの設定。Dark Deception の `BP_DD_Settings_SaveGame` と `UMG_Options`）
Babylon 非依存（`./save.ts` を拡張子付きで import。node のテストが解決できるように）。
- `interface Settings { quality; resolutionScale; brightness; music; sfx; dialogue; subtitles; mouseSensitivity; headBobbing; invertY; toggleSprint; mouseSmoothing; difficulty }` — 原作の SaveGame の項目（`Crosshair` はメニューに無いので持たない）。数値は原作の値域: `quality` 0..3（LOW / MEDIUM / HIGH / VERY HIGH）、`difficulty` 0..2（EASY / NORMAL / HARD）、ほかは 0..1。
- `DEFAULT_SETTINGS`（凍結）: 原作の CDO の値。`quality 2`、`resolutionScale 1`、`brightness 1`、`music` / `sfx` / `dialogue 1`、`subtitles true`、`mouseSensitivity 0.5`、`headBobbing true`、`invertY false`、`toggleSprint false`（この 2 つは CDO に無い = false）、`mouseSmoothing true`、`difficulty 1`（`NewEnumerator1` = NORMAL）。
- `QUALITY_TEXT` = `['LOW', 'MEDIUM', 'HIGH', 'VERY HIGH']`（原作の `Quality Text`）、`DIFFICULTY_TEXT` = `['EASY', 'NORMAL', 'HARD']`（`GetText_0`）。`QUALITY_MAX` 3、`DIFFICULTY_MAX` 1（原作の矢印の Clamp が 0..1 なので HARD には届かない）。`stepValue(value, delta, max)` = `clamp(value + delta, 0, max)`。
- `SLIDER_GRID` = 1/9、`snap(v)` = UE の `GridSnap_Float`（`floor((v + g/2) / g) × g`、0..1 に収める）。原作のスライダーは値が変わるたびにこれを `SetValue` し直す（10 段）。
- `sliderText(v)`: 原作の値の欄。`RoundFloatDecimals(v, 1)`（MoreporkFunctions: `Round(v × 10) / 10`、Round は floor(x + 0.5)）を `Conv_FloatToText`（小数は最大 3 桁、整数なら小数なし）。10 段は `0, 0.1, 0.2, 0.3, 0.4, 0.6, 0.7, 0.8, 0.9, 1` と読める（0.5 は既定の感度を動かす前だけ）。
- `gamma(b)` = `MapRangeClamped(b, 0, 1, 1.8, 2.2)`（原作はコンソールコマンド `gamma X`）。`brightnessPower(b)` = `2.2 / gamma(b)`（UE の表示ガンマは既定 2.2。仕上がった画像の値を `値^(2.2/g)` にし直すのと同じ）。
- `MIN_RESOLUTION_SCALE` 0.1、`renderScale(s)` = `clamp(resolutionScale, 0.1, 1)`（原作は `SetResolutionScaleValueEx(× 100)`。UE がスクリーンパーセンテージを 10 以上に保つのは推定。エンジン本体は pak に無い）。
- `lookScale(s)` = `mouseSensitivity / 0.5`（原作はマウスの軸 × 感度。本作の `camera.mouseSensitivity` を既定 0.5 のときの速さとする）。
- `rotationLagSpeed(s)` = スムージングありで 12.5、なしで 50（原作の `Set Up Mouse Smoothing` が SpringArm の `CameraRotationLagSpeed` に入れる値）。`SPRING_ARM_LAG_SPEED` = 20（原作の `BP_DD_PlayerCharacter` の SpringArm の既定。原作の Set Up Mouse Smoothing は BeginPlay では呼ばれないので、レベルはこの速さで始まる。`start` と `again` で `controls.lagSpeed` に入れる）。`LAG_MAX_STEP` 1/60。`lagRotation(previous, target, dt, speed)`: 1 つの角度の回転ラグ。`dt ≤ 1/60` なら `QInterpTo`（`previous + (target − previous) × min(1, dt × speed)`）、それより長いフレームは UE の SpringArm のサブステップどおり、`previous` から `target` へフレーム内で直線に動く目標へ 1/60 s ずつ近づける。`speed ≤ 0` は即座に `target`、`dt ≤ 0` は `previous`。
- `enemySpeedScale(s)`: EASY なら `{ walk: 100 / 200, chase: 300 / 430 }`（原作の `BP_Monkey` の BeginPlay が EASY で Walk Speed を既定 200 から 100 に〈@1383〉、Run Speed を 430 から 300 に〈@15〉する）、それ以外 `{ walk: 1, chase: 1 }`。以前は「追跡の速さは変えない」と読んでいたが誤りだった（2026-09-15）。
- `interface QualityLevel { shadows; shadowMapScale; bloom; motionBlur; ssao; dof; ssr }`、`QUALITY`（4 段）、`qualityOf(s)`: 各段で本作の描画のどれを残すか（原作は UE のスケーラビリティを切り替えるが本作に同じものが無いので、ユーザーが選んだ当て方）。HIGH は CONFIG どおり（すべて true、倍率 1）。MEDIUM はモーションブラーを切る。LOW はさらにランプの影とブルーム、SSAO と被写界深度と画面空間の反射も切る（09 記録）。VERY HIGH は影マップを 2 倍にする。
- `sanitize(data)`: 欠けた・型の違う項目は既定値、数値は値域に収める（`quality` / `difficulty` は整数に丸める）。
- `class SettingsStore(key, storage = safeLocalStorage())` — `load(): Settings`（無い・読めない・JSON が壊れていれば既定値）、`save(s): boolean`（`sanitize` してから書く。storage が無い／例外で false）。進行のセーブとは別のキーに置く（原作の `Settings` スロット）。

## 内部構造と処理の流れ
### `Game.create` の初期化順（進捗値は `onProgress` の fraction）
1. 0.01: `createEngine(canvas)`（既定 WebGL2、`render.preferWebGPU=true` なら WebGPU を試す。`src/core/engine.ts`）
2. 0.04: `Scene` 生成。`clearColor = CONFIG.render.clearColor`, `ambientColor = Black`, `skipPointerMovePicking = true`
3. `AssetStore` 生成、`KhronosTextureContainer2.URLConfig` を `vendor/ktx2/*` のローカル配信に差し替え（CDN 不使用）
4. `HavokPhysics()` の WASM ロードを開始（await は後回し）
5. `assets/manifest.json` を読み、`kind !== 'lightmap'` のもの＋現在の `lights.mode` に対応するライトマップのページ（`realtime` → `CONFIG.lightmap.indirect`、`baked` → `CONFIG.lightmap.full`、`hybrid` → 両方（全部で照らし、間接光で反射を混ぜる。07 記録）。`{page}` を数字にしたパターン `pagePattern`）を `store.preload`。`CONFIG.debug.field` のときは `kind` が `model` / `audio` / `env` のものだけ。ステージは全体で約 237 MB（テクスチャ・level の bin 8 本・ライトマップ）。進捗は 0.05 + 0.83 × 読み込み比率、ラベルは `LOADING x.x / y.y MB — ファイル名`
6. 0.89: `scene.enablePhysics(gravity=(0, CONFIG.player.gravity, 0), new HavokPlugin(true, havok))`
7. 0.90: ステージは「ステージを組み立てています…」、`loadLevelMeta(store)`（`assets/level/level-meta.json` を fetch、失敗時 `{}`）→ `loadLevel(scene, store, meta)`。`debug.field` のときは「デバッグフィールドを組み立てています…」で `buildDebugField(scene, store)`（07 記録）。どちらも同じ `Level` を返す
8. `GameState(level.shards の id, debug.field || CONFIG.game.chaseOnAllShards, CONFIG.game.lives)` 生成（デバッグフィールドだけフレンジーを残す）。セーブはここでは反映しない（`start()` の `restoreSave()` で反映する）。追跡フラグは毎フレーム `update` が決める（`CONFIG.debug.chase` は `forcedChase` の初期値）
9. `Input(canvas)` → `PlayerController(scene, input, level.playerStart, level.playerYaw)` → `scene.activeCamera = player.camera` → `Teleport(scene, player, input)`（05 記録）
10. 餅モデル（`CONFIG.game.shard.model`）→ `Shards(scene, shardModel, level.shards, state.collected)` → 保存された設定の QUALITY → `LampSystem(scene, level.lamps, staticMeshes + gateMeshes + dynamicMeshes)`（シャードのメッシュを影キャスターに）→ `lamps.setQuality(quality)` → ステージに特殊シャードがあれば、餅をもう一度（赤くするマテリアルを分けるため）と `assets/models/power_orb.glb` を読み込んで `new SpecialShards(scene, …, stage.specials)`（メッシュを影キャスターに。08 記録）→ 障壁ごとに `partCollider(scene, 'barrier<i>_<アクター>', def.box)`（07 記録。ANIMATED の箱の体。`Barrier { def, node, shape, body, broken, goneIn, appeared }`。`goneIn` は壊れてからメッシュが消えるまでの秒〈−1 は立っているか消えた後〉、`appeared` はシャードの障壁がこの走りで立ったか）→ `Gate(level.gate, gateMeshes, gateCollider, level.gateStyle, slide なら game.stage.doorTime、lift なら game.gateOpenTime)`
11. `PostFx(scene, player.camera, quality)`（09 記録）→ `Minimap(scene)` → `Tablet(scene, camera, minimap, 反射プローブ対象メッシュ)`
12. 0.93: `AudioManager()` 生成、`voices/manifest.json` から `id → subtitle` の `Map`、manifest の `kind === 'audio'` を全件 `audio.decode` → `audio.preparePitchVariants('shard_pickup', …)`
    - 敵: `NavGrid.fromBoxes(level.navBoxes, { cell: enemy.navCell, radius: enemy.radius, height: enemy.navHeight })` → `FloorGrids.fromBoxes(level.navBoxes, navOpts, nav)`（テレポートの照準の床の段ごとの格子。地上の段は `nav`）→ `teleport.zone = (x, y, z) => floorGrids.walkable(x, y, z)` → 敵のモデル → `Enemies(scene, enemyModel, nav, level.enemySpawns, level.waypoints, audio, stage ? game.stage.specials : 0)`（`enemy.count` 10 体。特別な敵のプールはステージで 20: 罠の扉から出る敵と脱出の群れ。記録 15）→ ステージなら `new Breakables(scene, stage.fences, stage.boards)`（壊せる物 = 駅の柵の区画と板張りの通り道 2 か所。薄れる板の半透明と煙のシェーダーをプリウォームで作らせるのでここで作る。デバッグフィールドは null。07 記録）
13. 0.96: `scene.whenReadyAsync()` → **プリウォーム描画**: カメラを yaw 0/90/180/270° に回し、奇数回は `CONFIG.camera.fovFast` の FOV で `scene.render()` を計 4 回。テレポートの照準の輪（`teleport.preview(playerStart)`）、敵の 1 体目（`enemies.showcase(playerStart + (0, 0, 4))`）、特殊シャード（`specialShards.showcase(playerStart + (0, 0, 3))`）、シャードの閃光の板（`shards.showcase(playerStart + (0.5, 1.2, 3))`）、壊せる物の透けた板と煙のスプライト（`breakables.showcase(playerStart + (0, 1.5, 3))`）、ロックピックとダッシュの障壁の板の透けた変種（その `def.meshes` を `warmFade(meshes, true)`。壊れると薄れて消えるので。07 記録）も描き、終わったら戻す（`warmFade(meshes, false)`）。カメラ回転・FOV を復元
14. 地図: ステージは `minimap.capture` を 2 回（`'surface'`: `METRO_TOP` より上に届く静的メッシュ・床 `floors.maze`・probes はシャード、`'metro'`: `METRO_TOP` より下に届くもの・床 `floors.metro`・probes はホームの開始地点と 1 m 四方の 4 点）、デバッグフィールドは `'main'` を 1 回（11 記録）→ `tablet.setVisible(false)`
15. 1.0 `'READY'`: `new Game(...)`。コンストラクタで `stageLamps`（ステージなら `new StageLamps(lamps, level.lamps)`、デバッグフィールドは null。07 記録）、`breakables`（`create` が作ったものを受け取り、あれば `onBreak` に `cue(BOARDS_BREAK, at)`〈原作の `Wooden_Boards_Breaking_v5`。07 記録の woodboards.ts、06 記録〉）、`FrameStats` 生成、`showStats(CONFIG.debug.stats)`、`syncWorld()`、`wireEvents()`、`applySettings`、タブレットの初期表示（残数・目的）、`engine.runRenderLoop(frame)`、`resize` リスナー登録

### `start(fresh)`（タイトルからの開始）
`fresh` なら `store.clear()` と `checkpointWarning.set(false)`、そうでなければ `restoreSave()` → `resetSpecials()` → `hooks.onStarted()` → `audio.resume()` → HUD・タブレット表示 → `playing = true, paused = false` → 回転ラグを SpringArm の既定に → デバッグフィールドなら `setMusic(chase ? 'bgm_chase' : 'bgm_normal', 2.5)`（ステージの曲は `toCheckpoint` と `updateStage`）→ `catching = null`、`chaseHold = 0`、`scene.animationsEnabled = true` → `toCheckpoint()` → `state.frenzy` なら `startFrenzyLoop()` → `startIntro()` → `input.requestLock()`。
`greet()`（OP の 13.5 s、`debug.skipIntro` なら開始の時）: チェックポイント 1 で 1 個も取っていなければボイス `greeting`。それ以外は何もしない。

### ステージ OP（`startIntro()` / `updateIntro(dt)`。Dark Deception のレベル BP とタイトルカード `UMG_ChapterPortal`、10 記録の stage-intro.ts）
- `startIntro()`（`start` と `again` の中。`respawn` では呼ばない）: `debug.skipIntro` なら `intro = null`、`introView.show(null)`、`greet()` だけ。そうでなければ `intro = new IntroClock()`、`player.enabled = false`、タブレットが上がっていれば `toggle()` で下ろす、`introView.show(introAt(0))`（黒）。
- `updateIntro(dt)`（`intro` があり `playing && !paused` のとき）: `'release'`（13 s）は `player.enabled = !catching && panelT < 0` とタブレットを上げる（捕獲中でなければ。SE `tablet_up` 0.6）、`'voice'`（13.5 s）は `greet()`、`'done'`（14 s）は `intro = null`。最後に `introView.show(...)`。

`restoreSave()`（RESUME のときだけ）: `CONFIG.debug.freshSave` でなければ `store.load()`。null か `escaped` なら何もしない。`state.restore(save)` → `shards.restore(state.collected)` → `tablet.setShards(remaining)` → `syncWorld()`。

`syncWorld()`（状態どおりに世界を置く。イベントなし。ファンゲームの各アクターがレベルを開いた時に見る状態）: 敵の出番と配電盤の時計（`enemiesIn` / `panelT`）を −1 に、`hud.fade(0, 0)`。デバッグフィールドは脱出中なら門を `open(true)`、そうでなければ `close()`。ステージは灯を `stageLamps.reset(checkpoint)`（灯の BeginPlay。07 記録の `stage-lamps.ts`）にしてから、障壁ごとに `setBarrier(b, down)`: シャードの障壁は全回収かチェックポイントが 2 でなければ倒れ（ファンゲームの `barrier_C` の BeginPlay: 表示・当たり・灯があるのはチェックポイント 2 だけ。1 ではプレイヤーが Box に初めて触れると立つ: 下記の `updateShardBarriers`）、ダッシュの障壁はチェックポイント 2 以上でそのリスポーン位置から `DASH_CLEAR` 50 m（ファンゲームの 5000 cm）以内なら消え（`barrierspeedbust` の BeginPlay）、ロックピックの障壁は立つ。最後に `lamps.refreshShadows()`。コンストラクタ・`restoreSave`・`respawn`・`again`・配電盤の後の脱出の開始で呼ぶ。
- `setBarrier(b, down)`: `broken = down`、`appeared = !down`、`goneIn = −1`、メッシュの表示（`setEnabled(!down)` と、薄れの途中から戻すための `visibility = 1`）と `solid(b, !down)`、障壁の灯（`stageLamps.setActor(source, !down)`）、ロックピックとダッシュの障壁のループの音（`barrierLoops`。止めてから、立っていれば `cueLoop(STAGE_SOUND.barrierLoop, centre)` を掛け直す。06 記録）。`solid(b, on)`: 保っておいた形で ANIMATED の体を作り直す／`dispose` する（Gate と同じ）。
- `breakBarrier(b)`: 壊れていなければ `broken = true`、コライダーと灯（`setActor(source, false)`。ファンゲームの障壁は割れると灯を破棄する）はすぐ外し、ループの音を止め、障壁の中心で `Barrier_Shatter_Cue`（`STAGE_SOUND.barrierShatter`。シャードの障壁は `shardBarrierShatter`。06 記録）。シャードの障壁は `goneIn = game.stage.barrierGone`（3 s。`barrier_C` は爆発して 3 s 後に消える）。ロックピックとダッシュの障壁（`Interactivate_barrier`・`barrierspeedbust`）は原作の Hotel の板張りのバリケード `BP_01_Woodboards` に倣い（07 記録の woodboards.ts）、中心に `breakables.smoke(centre)`（`P_Explosion1` の煙。07 記録）を出して `goneIn = WOODBOARDS_GONE`（4 s）。消えるまでは毎フレームの時計（下の「毎フレーム」の 6）が進め、ロックピックとダッシュの障壁は板の `visibility` を `woodboardsOpacity(WOODBOARDS_GONE − goneIn)`（2 s までは 1、そこから 2 s で接線 0 の三次で 0 へ）にし、0 で `hideBarrier`（メッシュを消して影を描き直す）。
  - ロックピックとダッシュの障壁が壊れるのは、視線で左クリックされたとき（下の「視線で使う」）と `active` な敵が箱に重なったとき（「ステージの流れ」）。ファンゲームの F の連打（`Lockpicking` のウィジェット）とブースト中の走り込みはやめた（本作の規範: 本家に無いギミックは複雑にせず、1 クリックで壊れて自然に消える。案内なしで遊べるように手のマークで示す）。

`toCheckpoint()`（ファンゲームのリスポーン）: `catching`・`chaseHold`・`enemiesIn`・`panelT` を戻し、`hud.fade(0, 0)`。ステージは `stage.checkpoints[state.checkpoint]` の位置と向き（1 ホーム・2 ホール・4 脱出の開始）へ `player.teleport`、`enemies.reset(checkpoint === 2)`（ホームでは敵はまだ出ていない。脱出の追手は 09 のギミックで）、`stageLamps.reset(checkpoint)`（ファンゲームのリスタートで灯のアクターが開き直すのと同じ）、罠・扉・脱出・壊せる物を開き直し（`resetTraps`・`resetDoors`・`resetEscape`・`resetBreakables`）、曲を `setMusic(null, 0.1)` で切って `stageMusic.reset(checkpoint, allShards)`（原作の RestartLevel が音を切る。06 記録の `stage-music.ts`）。デバッグフィールドは開始地点へ、`enemies.reset(!escaping)`。`start`・`respawn`・`again`・配電盤の後で呼ぶ。

### 毎フレーム（`frame` → `update`）
`frame` は `dt = min(engine.getDeltaTime()/1000, 1/15)` にクランプし（`CONFIG.debug.fixedDt > 0` ならその値に固定）、`stats.push`、`time += dt`、`update(dt)`、`scene.render()` の順。`update` の内容:
1. 初回のみ `scene.onAfterPhysicsObservable` に物理後フックを登録: `player.step(pdt)` → `player.syncCamera(pdt)` → `teleport.applyToCamera(camera)` → 敵の捕獲中は `jumpscareShake(catching.t)`（記録 15）の度数を FOV と回転に（罠の死では揺らさない） → 特殊シャードの取得の揺れ（`pickupShake`。08 記録）→ ソウルシャードの取得の揺れ（`shardShakes`。08 記録の shard-fx.ts）→ 門の揺れ `shake`
2. `look = input.beginFrame()`
3. `F3` で `showStats(!表示中)`（常時有効）
4. `active = playing && !paused && !catching` のときのみ:
   - `F7` で `forcedChase` トグル
   - `player.update(dt, look)`
   - `player.enabled` なら、`KeyQ` / `KeyE` の押下で `tablet.pop('teleport')` / `tablet.pop('boost')`
   - `player.enabled` なら `Space`/`Tab` でタブレット出し入れ（SE `tablet_down`/`tablet_up` 0.6）、`KeyZ` でミニマップズーム（タブレットを上げている時だけ。SE `ui_select` 0.5・再生速度 2）
   - `shards.touching(player.feet)` の各シャードで `state.collect(id)`、`specials.touching(player.feet)` の各特殊シャードで `takeSpecial(item)`
   - プレイヤーが動けてテレポートの照準中でなければ `usable = lookTarget()`（下記「視線で使う」）、あって `Mouse0` が押されたら `use(usable)`。`active` でないフレームは `usable = null`
   - `enemies.update(dt, player.feet, player.sprinting, state.frenzy)`（記録 15）。`caught` があり `state.running` なら `catching = { actor, t: 0 }` にして `state.catchPlayer()`。`chasing` なら `chaseHold = CONFIG.enemy.chaseHold`（3 s）、でなければ減らす
   - ステージ OP でプレイヤーを止めている間でなければ `state.tick(dt)`
   - ステージなら `updateStage(dt)`（下記「ステージの流れ」）。デバッグフィールドは門: 脱出中で `level.exit` から水平 1.6 m 未満なら `tryEscape()`、脱出前で門から 2.4 m 未満ならラッチ付きで `usePanel()`（全回収なら脱出＝門が開く、残りがあれば `panelDenied`）、4 m 離れるとラッチ解除
   - `state.running` なら追跡フラグ: `state.chase = forcedChase || state.frenzy || chaseHold > 0`
5. 捕獲中（ポーズ中を除く）は `updateCatch(dt)`: 敵の捕獲なら `enemies.lunge(actor, feet, dt)` で敵を詰め寄らせ、プレイヤーの yaw / pitch を敵の顔へ `1 − exp(−14 dt)` で引き寄せる（罠の死 `killPlayer` は `actor` が null で、どちらもしない）。`CONFIG.enemy.catch.time`（3.5 s）で `showDeath()`: `paused = true`、`player.enabled = false`、`scene.animationsEnabled = false`、`audio.setPaused(true)`、`hooks.onDeath(state.lives)`。
6. ステージ OP の最中は `updateIntro(dt)`。`playing && !paused` なら `updateSpecials(dt)` と、壊れて消えるのを待つ障壁の `goneIn` を減らし（ロックピックとダッシュの障壁は板の `visibility` を `woodboardsOpacity(WOODBOARDS_GONE − goneIn)` に）、0 で `hideBarrier`（ゲームの時計）
7. `teleport.update(dt, active)`
8. 常時: `lamps.update(dt, cameraPos)` → `shards.update(dt)` → `gate.update(dt)` が true なら `lamps.refreshShadows()` → スピードブースト（05 記録の `boost-fx.ts`。`tint`、効果中のウィジェットの `widgetOpacity`、Chameleon の `chameleon`、揺れの位相 `boostShakePhase`、集中線のコマ）→ `post.zone` = `forcedZone` か、デバッグフィールドなら `'field'`、足元が `METRO_TOP`（−9.5 m。地下鉄のポストプロセスボリュームの上端）より下なら `'metro'`、それ以外 `'surface'`（09 記録のステージのボリューム）→ `post.update(teleport.fx, { tint, lines, vignette, frame, blur, shake })`
9. タブレット更新: `setObjective`、`setRefill('boost' | 'teleport', …)`、地図の矢印（`arrowAim()`: シャードの間は最寄りのシャード（`shards: true`）、全回収で配電盤を `ARROW.color.altar`、脱出中はゴールの箱の中心を `ARROW.color.portal`。デバッグフィールドは全回収で `level.exit`。`arrow.update(dt, { player, camera, yaw, zoomedOut, zone: shards.items })`）、マーカー（`markers()`: 未回収シャード `#d21ee6`、見えている特殊シャードと地図に載った敵（08・11 記録）、全回収で配電盤、脱出中はゴールに `#f0cc7a`。どれも地上なので、足元が `METRO_TOP` より下なら出さない。デバッグフィールドは脱出中に出口）、地図の階（ステージは `tablet.minimap.layer` = 足元が `METRO_TOP` より下なら `'metro'`、それ以外 `'surface'`）、`tablet.update(dt, { player, yaw, markers, arrow, time })`
10. `audio.setListener(camera.position, player.forward, camera.upVector)`
11. `#stats` 表示中はデバッグ文字列（API, fov, dash, post.debugInfo, 有効ランプ, chase, checkpoint, 敵ごとの `mode:距離m`）を更新

### 特殊シャード（`resetSpecials()` / `updateSpecials(dt)` / `putOnMap()` / `takeSpecial(item)`。ファンゲームの `red_shard` / `stun_orb`、08 記録）
- `takeSpecial(item)`: `state.collectSpecial(item.kind)` が true のときだけ `specials.collect(item)`。
- `updateSpecials(dt)`: `specials.update(dt)` → `reveal.update(dt)` が `'add'` なら `putOnMap()`（`active` な敵を全員 `revealed` に）、`'end'` なら `revealed.clear()`（60 s）→ `collectFx` があれば `t += dt` して `post.collect = collectLook(kind, t)`（`COLLECT_TIMELINE.length` 2 s で終わり）。
- `resetSpecials()`（`start` と `again`）: `specials.reset(state.bonusShard)`、`reveal.stop()`、`revealed.clear()`、`collectFx`・`pickupShake`・`shardShakes` を消し、`post.collect` を 0 に、`countdown` を止め、`vsidesView.clear()`。リスポーンでは呼ばない。

### 視線で使う（配電盤・扉・壊せる物・障壁。`lookTarget()` / `use(target)`。値は 08 記録の `world/interact.ts`）
`Target` = `{ kind: 'panel' } | { kind: 'door'; door } | { kind: 'break'; index } | { kind: 'barrier'; barrier }`。カメラの位置から前方への線が候補の箱に入る距離 `rayBoxDistance` を求め、候補ごとの届く距離以内（近すぎる下限があればそれ以上）で最も近いものを選ぶ。候補は、脱出前で配電盤の後の暗転中でなければ配電盤（`panelBox(stage.panel)`: 床から −0.3〜2.2 m・水平 ±0.6 m の `PANEL_USE`。ファンゲームの `Lamp_zone_1_on_off_2` に視線の箱が無いので推定。`INTERACT.reach` 2 m）。選んだものまで Havok のレイを引き、`reach − wallSlack`（0.25 m）より手前で当たれば壁越しとして null。候補には扉も入る（`target` のある扉のうち、中心がカメラから 4 m 以内で、上げたブロッカーを除く。届く距離 `INTERACT.reach` 2 m・近すぎる `DOOR_NEAR` 0.25 m〈ファンゲームのトレースの 25 < d < 200 cm〉）。立っている壊せる物も入る（`breakables.targets()` の区画の箱。届く距離 `INTERACT.reach` 2 m、下限なし。原作の `BP_01_Woodboards` はプレイヤーの 200 cm のトレースが `interact` タグに当たると手のマーク）。立っているロックピックとダッシュの障壁（壊れていず体のあるもの）も入る（障壁の箱 `def.box`。届く距離 `INTERACT.reach` 2 m、下限なし）。`use` は配電盤の `state.usePanel()`、扉は `useDoor`（下記）、壊せる物は `breakables.break(index)`（原作の `InteractWithObject`。1 回で壊れる。07 記録）、障壁は `breakBarrier(barrier)`（上記。これも 1 回で壊れる）。左クリック（本作の E はブースト）。デバッグフィールドには対象が無い。

### 壊せる物（`breakables` / `resetBreakables()`。07 記録の breakables.ts・woodboards.ts）
- ステージの駅の柵の区画と板張りの通り道（出口に打ち付けた板 `wayOut` と低い戸口の格子の壁 `doorway`。`Breakables`、デバッグフィールドは null）。`create` で敵の後・プリウォームの前に作り（上の 12・13）、コンストラクタが `onBreak` に `cue(BOARDS_BREAK, at)`（区画の中心で原作の `Wooden_Boards_Breaking_v5`。06 記録）をつなぐ。
- 壊れるのは、視線で左クリックされたとき（上の「視線で使う」）と敵が触れたとき: 毎フレーム `updateStage` の中（扉の後）で `active` な敵ごとに `breakables.touch(brain.x, brain.z, CONFIG.enemy.radius 0.4, root.position.y, root.position.y + CONFIG.enemy.height 2.1)`（原作の `Box` にタグ `Enemy` のポーンが重なる）。続けて `breakables.update(dt)` が真（飛んだ板が動いているか、打ち付けた板が崩れている途中か、消えた）なら `lamps.refreshShadows()`。
- `resetBreakables()`（`syncWorld` と `toCheckpoint`）: `breakables.reset()` が真（壊れていたものを立て直した）なら `lamps.refreshShadows()`。

### ステージの流れ（`updateStage(dt)` / `updatePanel(dt)`。ファンゲームのレベル BP と各 BP、値は `CONFIG.game.stage`）
- **ホールのトリガー**（`trigger2`）: チェックポイント 2 より前に足元が `stage.triggers.chase` に入ったら `state.reachHall()`（`checkpoint` イベントで SAVING PROGRESS を出して保存し、`enemiesIn = enemyDelay`（1 s。`Spawn_Enemies` の Delay 1））。`enemiesIn` が 0 を切ったら `enemies.setMazeActive(true)`。
- **街灯**: `stageLamps.update(dt, foesChasing || forcedChase, state.allShards)`（07 記録の `stage-lamps.ts`）。`foesChasing` は `enemies.update` の `chasing`（ファンゲームのプレイヤーの `Player See By Monster` が 0 より大きい）、F7 と `debug.chase` の強制の追跡もマネキンの追跡と同じに扱う。
- **シャードの障壁**（`barrier_C`。`updateShardBarriers()`、曲の後・罠の前）: チェックポイント 1 で全回収でないときだけ。まだ立っていない（`appeared` が偽の）シャードの障壁ごとに、足元の x・z が箱 `def.zone` の中で、カプセルの高さの範囲（足元〜足元 + `CONFIG.player.capsuleHeight`）が箱の y の範囲と重なれば（ファンゲームの重なり）`setBarrier(b, false)`（板・当たり・灯。プレイヤーの後ろに立つ）、障壁の中心で `cue(STAGE_SOUND.shardBarrierAppear)`（06 記録）、`lamps.refreshShadows()`。2 回目以降に触れたときのファンゲームの `not_yet_C` のウィジェットは作っていない。
- **罠**（`updateTraps(dt)`。`game/traps.ts`。`CONFIG.debug.traps` が偽〈検証のスクリプトが全シャードへ瞬間移動する時〉なら何もしない: 噴出も車も止まり、誰も殺さない）: 床の噴出（`Pair_trap`）は `Jet.update(dt, 足元が stage.jets の trigger の中)`（立つと 3 s 噴き〈`JET_FIRE`〉、4 s 休む〈`JET_RELOAD`〉。休みの間は反応しない）、`'fire'` / `'stop'` で煙 `jetSmoke`（07 記録の `world/trap-fx.ts` の `createJetSmoke`）を `start` / `stop`（`'fire'` で `PAR` の音 `STAGE_SOUND.jet` を噴出の位置で）、噴いている間に足元が `jet` の箱の中なら `killPlayer()`。車（`car_trap`）は `CarRun.update(dt)`（BeginPlay の Delay 0.1 s〈`CAR_BEGIN`〉はレベルの位置〈相対 y 20500〉のまま、それから Timeline の 0 → 22000 を 8 s、7 s ごとに最初から: 2750 cm/s で 1 回 19250）、走り出すたびに `RandomGate(CAR_PAINTS)` の次の塗りを車体のスロット 1（材質 `Car1`）の車ごとの写し `paint` の `albedoColor` に（MultiGate の出力 0〜3 = `Car1`〈灰〉・`Car3`〈青〉・`Car4`〈黒〉・`Car2`〈赤〉。4 枚のテクスチャは同じ柄なので灰を平均の色の比で染める。出尽くしたらそのまま）。車輪は回さない（ファンゲームは Sphere1・2 の Roll を 0.375 s で 1 回転）、`placeCar` で車の部品のノードを `base + step × units`（`step` は stage の `unit` を親の空間に直したもの）、殺す箱を `unit × units` ずらし（`carBox`）、エンジンの音をその中心へ動かし（`follow`）、クラクションの箱 `horn`（Box1）も同じだけずらして足元が入れば `carHorn`（`hornIn` で 3 s〈`HORN_COOLDOWN`〉に 1 回）、殺す箱に足元が入れば `killPlayer(true)`、入った敵（`stunned` でない）は `brain.stun(CAR_STUN, 0)`（10 s。ファンゲームのマネキンは死ぬ〈`Death_by_car`〉が、本作の敵は死なないので、ダッシュの気絶と同じ長さ）。
  - `killPlayer(runOver = false)`: `state.running` で捕獲中でなければ `catching = { actor: null, t: 0 }` と `state.catchPlayer()`（ファンゲームの `death()`。死亡画面は捕獲と同じく `enemy.catch.time` 後）。車とトラック（`runOver`）は轢かれた音の対（`runOverFall`・`runOverPunch`）を 2D で。
  - `resetTraps()`（`syncWorld` と `toCheckpoint`）: 噴出を休みなしに・煙を止め、車をレベルの位置から（BeginPlay の Delay から。チェックポイント 4 は `parked`: レベルの相対 y 20500〈地図の外〉に止める。Timeline が BeginPlay の 0.1 s 後に止められる）、塗りを灰に・MultiGate を新しく、`placeCar`、エンジンの音を掛け直し（BeginPlay から鳴る）、クラクションをすぐ鳴らせるように。
  - `traps.ts`（Babylon を使わない）: `Jet`（`firing`、`update` は `'fire'` / `'stop'` / null、`reset`）、`CarRun(parkedAt)`（`t` は −`CAR_BEGIN` から、`units` は負の間と `parked` のときレベルの位置、`update` は走り出す時〈Delay の後の最初と 7 s ごと〉に true、`reset(parked)`）、`RandomGate`（MultiGate のランダム・ループなし: まだ出ていない出口から 1 つ、出尽くしたら null、`reset`）、定数 `JET_FIRE` 3・`JET_RELOAD` 4・`CAR_TO` 22000・`CAR_LENGTH` 8・`CAR_RESTART` 7・`CAR_BEGIN` 0.1・`HORN_COOLDOWN` 3（クラクションは音と一緒に 10 で）・`CAR_PAINTS`（灰に掛ける色: 1 / (0.358, 1.441, 3.336) / (0.123, 0.089, 0.093) / (2.3, 0.09, 0.094)。各テクスチャの線形の平均 ÷ 灰の平均）。`tests/traps.test.ts`。
- **扉**（`updateDoors(dt)`。`game/doors.ts`、07 記録の `world/door-parts.ts`。ファンゲームの扉 6 種、参照資料の「扉の部品・支点・当たり」）: `DoorState`（`logic`、`parts`〈`DoorParts`〉、当たり `part`〈葉・蓋・シャッター・腕〉と `cube`〈罠の扉の Cube〉・`post`〈ブロッカーの柱〉・`wall`〈ブロッカーの壁〉、interact の箱 `target`、開き戸の `right`、`wasMoving`）をコンストラクタの `makeDoor` が作る: 施錠 121（当たりは静的なステージのもの、`target` は葉の箱 `LEAF_BOX`）、開き戸 6（葉の当たりは蝶番に従う）、罠の扉 25（葉と Cube、`target` なし）、通気口 13（蓋の当たりを開閉で入れ切り）、シャッター 20（羽根の当たり、`target` はボタン。`FIXED_SHUTTER` = `Big_door_zone_1_BP23` は `World_change` で開かない）、ブロッカー 25（腕・柱・壁、`target` は腕と柱）。毎フレーム: 開き戸は足元が `sideA` なら `right = false`、`sideB` なら true。罠の扉は `update(dt, 足元が plate の中, 全回収でない)` が true なら `breakTrapDoor`（Cube の当たりを切り、灯を `stageLamps.setActor(source, false)`、`spawn` に特別な敵を `enemies.spawnSpecial`〈プレイヤーへ向けて〉）。シャッターは閉じていて敵が根から 3 m〈`SHUTTER.enemyReach`〉以内なら `force()`（マネキンの `Opendoor`。本作の敵も扉を通り抜ける: ファンゲームのマネキンは閉じた扉に遮られない）。通気口とシャッターは `part.set(solid)`。動いている間と止まったフレームに `parts.setPose(pose)`、どれか動けば `lamps.refreshShadows()`。
  - `useDoor(d)`: 施錠 `use()`（4 s に 1 回 `doorLocked`）、開き戸 `use(right)`（`'open'` / `'close'` で `doorOpen` / `doorClose`）、通気口 `use()`（`vent`、2D）、シャッター `use()`（ボタンの `shutterButton` と羽根の `shutterMove`）、ブロッカー `use()` なら壁の当たりを切り `blocker`。音は interact の箱の中心で（06 記録の `STAGE_SOUND`）。マネキンがシャッターを開けるとき（`force()`）は `shutterMove`、罠の扉が破れるとき（`breakTrapDoor`）は `trapBreach`。
  - `resetDoors()`（`syncWorld` と `toCheckpoint`）: 論理を `doorLogic` で作り直し、姿勢を `REST`、当たりをすべて入れ、罠の扉の灯を点ける。
  - `doors.ts`（Babylon を使わない）: `Sequence`（前後に再生、端で止まる）、`Cooldown`（DoOnce と n s 後の再開）、`LockedDoor`（4 s）・`SwingDoor`（`SWING`: 蝶番 (−7, 70, 0)、Yaw の `Open_Left` 0 → −91 / `Open_Right` 0 → 91 の三次、0.5167 s、閉じるのは同じシーケンスの逆再生、0.7 s）・`VentDoor`（`VENT`: Pitch 0 → 120 線形 0.6667 s、開けると当たりなし、閉じて 0.6 s で当たり、0.75 s）・`ShutterDoor`（`SHUTTER`: 相対 Z の開 0 → 227.5 → 239.1 → 350 → 360〈1.671 s〉・閉 215 → 0 → 3〈0.946 s〉を線形〈補間は推定〉、開閉の切り替え、2 s、使うまではレベルの高さ 0、`force`）・`BlockerDoor`（`BLOCKER`: 蝶番 (0, −35, 106)、Pitch 0 → −15 → 4 → −75、2.754 s、一度だけ）・`TrapDoor`（`TRAP`: 板に乗るたびに `RandomGate([0, 1, 2, 3])`、1 で壊れて葉が X −60・Z 14・Pitch 90・Yaw 10〈0.521 s〉、それ以外は 1 s 待つ。最初の 4 回のどれかで必ず壊れる）、`LEAF_BOX`、`REST`、`ueRotation`（UE の FRotationMatrix）、`posePoint`。`tests/doors.test.ts`。
- **脱出**（`updateEscape(dt)`。チェックポイント 4 のあいだ `escapeClock` がある。`game/escape.ts`。ファンゲームのレベル BP・`Escape_zone_1_Blueprint`・`Truck_boss`・`Mannequin_Escape`、参照資料の「脱出フェーズ・トラック・車・噴出の詳細」）: `EscapeClock.step(dt)` の出来事ごとに、`inputOff`（0.1 s: `escapeHold = true`、プレイヤーを止める。入力を戻す 3 か所〈OP の解除・`resume`・配電盤の後〉は `escapeHold` の間は戻さない）、`truck`（0.9 s: 最初のトラック〈Spline_3、8 s〉を `spawnTruck`、ゾーンの不可視の壁 10 枚の当たりを入れ、プレイヤーを戻す）、`crowd`（1.1 s: 群れ 15 を `enemies.spawnSpecial`〈プレイヤーへ向けて。プール 20〉）。ゾーンが出た後は、足元が入った箱ごとに一度だけ: Box10〜13 はそれぞれのスプラインと時間でトラック、Box15・16 は群れ 3・2。`triggers.endChase`（TriggerBox_1）で一度だけ `enemies.clearSpecials()`。トラックごとに `TruckRun.update`、出ていれば `placeTruck`（運び台のノード `carrier` に、組み立ての置き場所〈最初のスプラインの始まり〉からの変換 `truckRef · RotationY(heading) · Translation(pos)` を分解）、止まっていなければブームの点の箱に `traceHits` で止まる（`stopped`。以後は終点まで転がるだけ。エンジンの音を止めて `truckBoom`）か、足元が `inTruck` なら `killPlayer(true)`。ビープとエンジンの音は毎フレームトラックの位置（`truckSoundAt`: 道の点の 1 m 上）へ。トレースが閉じたシャッターの羽根の箱に当たれば `breakDown`。群れの特別な敵が閉じたシャッターの羽根か下りたブロッカーの腕の箱から水平に `CROWD_BREAKS` 1.5 m 以内なら `breakDown`。ボス戦のマネキン（Box17。プレイヤーが殴って倒す）、トラックの標識のデカールと火と灯、ゾーンの灯 9 と飾りのマネキン 30、壊れた扉の破片と煙は作っていない。
  - コンストラクタ: `stage.escape` があれば壁の `boxCollider` を作って切っておき、トラックの部品のノードを `truck0` の下に `setParent`、`truckRef` = 最初のスプラインの 0 m の姿勢の逆行列。
  - `resetEscape()`（`syncWorld` と `toCheckpoint`）: トラックの音を止めてトラックを空に・運び台を隠し、壁を切り、発火した箱を忘れ、`escapeHold = false`、脱出中なら新しい `EscapeClock`。`syncWorld` はチェックポイント 4 でダッシュの障壁 `ESCAPE_BARRIER`（`barrierspeedbust6`）も倒す（レベル BP）。
  - `spawnTruck(path, time)`: 使っていない運び台（無ければ最初の運び台の子ごとの `clone`）で `TruckRun`、道の始まりでビープ（`truckBeep`、1 回）とエンジン（`truckEngine`、fadeIn 2 s、頭から）を鳴らす。`breakDown(d)`: 扉を `gone` にして部品を隠し当たりを切る（シャッターなら `shutterCrash`）（`resetDoors` が戻す。`gone` の扉は interact も `updateDoors` も飛ばす）。
  - `escape.ts`（Babylon を使わない）: `TRUCK_CURVE`（Timeline_0 のキー → f(s) = −0.77983s³ + 1.55967s² + 0.22017s）、`TRUCK_BEGIN` 0.1、`Path` と `pathAt(path, d)`（等間隔の点の線形補間と向き）、`TruckRun(path, duration)`（`wait`・`t`・`shown`・`distance` = f(t / duration) × 長さ・`pose`・`stopped`）、`truckPoint`（トラックの空間〈UE の cm: X 前・Y 右・Z 上〉→ ワールド）、`TRUCK_KILL`（Box3: 中心 (−6, 40, 160)・半分 (336, 118, 171)）と `inTruck`、`TRUCK_TRACE`（Box1 から前へ 150 cm・半径 50）と `traceHits`（球を 7 点で掃く）、`ESCAPE_AT` と `EscapeClock`。`tests/escape.test.ts`。
- **ロックピックとダッシュの障壁**（`Interactivate_barrier`・`barrierspeedbust`。脱出の後）: 壊れていない障壁ごとに、`active` な敵の体が箱に重なれば（`touches(def.box, brain.x, brain.z, CONFIG.enemy.radius, root.position.y, root.position.y + CONFIG.enemy.height)`。07 記録の breakables.ts）`breakBarrier`（原作の板張りのバリケードの `Box` に `Enemy` のポーンが重なる @1012 と同じ）。プレイヤーは視線で左クリックして壊す（上の「視線で使う」）。ファンゲームのロックピックのウィジェット `Lockpicking` と F の連打、ブースト中の走り込み（MaxWalkSpeed ≥ 900）で割れるダッシュの障壁は、本作の規範でやめた。
- **配電盤**（`Lamp_zone_1_on_off_2` の "Ele" → `Activate_escape_seq`）: `usePanel()` が通ると `checkpoint` イベント（4）で `panelT = 0`。`updatePanel` は `panel.stop`（1 s）で `player.enabled = false`、`panel.escape`（2 s）で `hud.fade(1, panel.fade)` と `stageLights`（2D）（ファンゲームのフェードとカットシーン `CUTSCENE_END_ZONE_2` の代わりの黒）、`escape + fade` で `syncWorld()`・`toCheckpoint()`（チェックポイント 4 の脱出の開始）・`persist()`・`hud.fade(0, 0.8)`・プレイヤーを戻す（OP の間でなければ）。
- **曲**（06 記録の `stage-music.ts`）: `updateStage` が毎フレーム `stageMusic.update(dt, foesChasing || forcedChase, allShards)` を呼び、`music` は `setMusic(id, fadeIn, volume, { fadeOut, offset })`、`once` は `music` バスで 1 回鳴らす。デバッグ API の `state().music`（BGM の id）と `state().stageMusic`（`'calm'`・`'chase'`・`'escape'`・null）。`panelHolding`（`panelT >= panel.stop`）の間は `resume` もプレイヤーを戻さない。
- **脱出の終点**: 脱出中で配電盤の暗転が終わっていて、足元が `stage.triggers.goal`（`Lamp_zone_1_on_off` の Box）に入ったら `state.tryEscape()`。

### ポーズ・再開・リトライ・終了
- `pause()`: `paused = true`, `player.enabled = false`, `scene.animationsEnabled = false`, `audio.setPaused(true)`, `onPauseChange(true)`。トリガーは `input.onLockChange`（ポインタロック喪失かつ `state.running`）と `window` の `blur`。
- `resume()`: `paused = false`, `player.enabled = !catching && !intro?.holding && !panelHolding`, `scene.animationsEnabled = true`, `audio.setPaused(false)`, `onPauseChange(false)`, `requestLock()`。
- `restart(hard = false)`（ポーズ画面の RESTART? の YES と、ゲームオーバーの LAST CHECKPOINT〈`hard`〉）: `hard` なら `state.hardRespawn = true`、`again()`。シャードは拾うたびに保存するので残り、ライフは満タン。
- `respawn()`（死亡画面の後、ライフが残っているとき。ファンゲームは 3.5 s 後に RestartLevel）: `state.respawn()` が false なら何もしない。`syncWorld()`、`toCheckpoint()`、タブレットが下りていれば上げ、`resume()`（シャード・経過時間・ライフはそのまま）。
- `newRun()`（ゲームオーバーの RESTART の YES）: `store.clear()`、`state.hardRespawn = false`、空のセーブ（`version 3`、チェックポイント 1）で `restore`、`shards.reset()`、`tablet.setShards`、`frenzy_loop` を 0.5 s で止め、`again()`。
- private `again()`: セーブ（無ければ現在の `toSave()`）で `restore`、`shards.restore`、`tablet.setShards`、`syncWorld()`、`resetSpecials()`、`toCheckpoint()`、回転ラグを戻し、タブレットが下りていれば上げ、デバッグフィールドなら `setMusic(...)`、フレンジーなら `startFrenzyLoop()`、`startIntro()`、`resume()`。
- `endRun()`（脱出）: `catching = null`, `paused = true`, `player.enabled = false`, `scene.animationsEnabled = false`, `audio.setPaused(true)`、すぐ `hooks.onEnd(levelResults())`。ポインタロックはここでは外さない（画面が UI 入力に切り替わる時に main.ts が外す）。
- `levelResults()`: `results({ time: elapsed, shards: shardIds.length, bonusShards: bonusShard ? 1 : 0, secrets: secrets.size, deaths, hardRespawn, totalLives: maxLives, streak: streakBest, easy: settings.difficulty === 0 })`。

### `GameState` のイベント配線（`wireEvents`）
- `shardCollected`（値は 08 記録の shard-fx.ts）→ `shards.collect(id)`, ステージは `stageShardSound()`（Chaotic Customer 2 の `Soul_Shard_Pickup_v2/v3/v4` の MultiGate。06 記録の `STAGE_SHARD`）、デバッグフィールドは SE `shard_pickup` を `SHARD_SOUND.volume` 0.65 で（`audio.playPitchVariant`）, `shardShakes.push(0)`, `tablet.setShards(remaining)`。1 個目（残りあり）ならボイス `well`。`persist(false)`（毎回だまって保存する。ファンゲームもシャードごとに名前をセーブする）
- `shardStreak` → `streakView.show(count, life)`、`streakSound(count)` の SE（0.7）、`player.playStreakShake()`
- `specialCollected`（値は 08 記録の collect-fx.ts）→ 取得音、`vsidesView.show(kind)`、`player.playStreakShake()`、演出の波の音、`collectFx = { kind, t: 0 }`、`pickupShake`。オーブは続けて `stun_countdown`（`countdown` に持つ）、`enemies.stunAll(stage.stunTime ?? SPECIAL.stun.time, SPECIAL.stun.notice)`（ステージは `stun_orb` の 17 s。15 記録）、`specials.playSphere(player.position)`。赤いシャードは `reveal.start()` と最初の `putOnMap()`、`persist(false)`
- `allShardsCollected`（ファンゲームの shard = 0）→ デバッグフィールドはそのまま `usePanel()`（脱出の扱い）。ステージはシャードの障壁を `breakBarrier`、フレンジーなら `startFrenzyLoop()`、`persist(false)`
- `checkpoint` → 2 なら `persist()`（SAVING PROGRESS）と `enemiesIn = enemyDelay` と `stageMusic.reachHall()`。4 なら `frenzy_loop` を 2 s で止め、デバッグフィールドは `persist()`・`gate.open()`・`gate_lever` → 500 ms 後 `gate_creak`・`gate_rumble`(0.8)、ステージは `panelT = 0` と配電盤の 1.5 m 上で `panelSpark` を 2 つ（上の配電盤の流れ）
- `panelDenied` → SE `barrier_denied`(0.8)、字幕「すべてのシャードを集めろ。」
- `chaseChanged` → デバッグフィールドなら `setMusic(chase ? 'bgm_chase' : 'bgm_normal', chase ? 0.4 : 2)`（ステージは `stageMusic`）
- `caught` → 敵の捕獲なら SE `enemy_scream`（罠の死では鳴らさない）、`player.enabled = false`、タブレットが出ていれば `toggle()` で下ろして `tablet_down`（0.6）、`state.chase = false`、`store.save(toSave())`
- `enemies.onSpotted` → ボイス `found` の字幕（`ワサミ: ここか！`、2.4 s）
- `escaped` → `store.clear()`、`endRun()`
- プレイヤーコールバック: `onFootstep(pitch)` → `footstep(pitch)`（`CarpetMap`（記録 07）の `under(player.feet)` が真なら `fs_carpet_` 25 種、それ以外 `fs_hard_` 10 種。`stepBag` で重複なしに選び、`audio.play(id, { volume: FOOTSTEP.volume (0.875), rate: pitch × (0.9〜1.1) })`。`steps` を数える）、`onBoost(ok)` → `power_boost`(0.7・再生速度 2)＋ボイス `fast`＋`boostFresh = true`／`power_not_ready`(0.35)、`onBoostReady` → `power_ready`(0.5)、`gate.onShake(a)` → `shake = max(shake, a)`
- テレポートのコールバック（05 記録）: `onAim(aiming)` → 照準中だけ `teleport_aim` のループ（0.65）と `teleport_enter`(1.75)、`onArrive(from, to)` → `teleport`(1.0)、経路上のシャードと特殊シャードの回収、`post.cut()`、`onCancel` → `power_ready`(0.5)、`onDenied` → `power_not_ready`(0.35)、`onReady` → `power_ready`(0.5)
- `say(id)`: `voices` Map に存在する id のみ、`voice` バスで再生し `hud.subtitle(subtitle, 'ワサミ', max(2.2, duration + 1.2))`

### `GameState` のルール
- `collect(id)`: `running` かつ既知 ID かつ未回収のときのみ true。`streak` を +1 して `shardCollected` を発火し、節目ちょうどなら `streakBest` を上げて `shardStreak`（`life` なら先に `lives = min(MAX_LIVES, lives + 1)`）。全回収で `allShardsCollected` を発火し、`frenzy` なら `chase = true`。
- `reachHall()`: `running` でチェックポイント 2 より前なら `checkpoint = 2` にして `checkpoint { 2 }` を発火して true。それ以外 false（1 回だけ）。
- `usePanel()`: `running` でないか脱出中なら false。シャードが残っていれば `panelDenied { remaining }` を発火して false。全回収なら `checkpoint = 4`、`chase = false`、`checkpoint { 4 }` を発火して true。
- `tryEscape()`: `running` でないか脱出中でなければ false。脱出中なら `escaped = true`, `chase = false`, `escaped` 発火。
- `catchPlayer()`: `running` でなければ false。`caught = true`、`lives = max(0, lives − 1)`、`deaths += 1`、`streak = 0` にして `caught { lives }` を発火。
- `respawn()`: 捕まっていてライフが残っているときだけ `caught = false` にして true。
- `tick(dt)`: `running` 中のみ `elapsed += dt`。
- `collectSpecial(kind)`: `running` のときだけ。`reveal` は `bonusShard` が false のときだけ true にして発火、`stun` は毎回発火。どちらもシャードの数と連続回収には入らない。
- `collectSecret(id)`: `running` のときだけ。ID を `secrets` に足し、`secretFound` を発火して true。
- `objective`（本作の文言）: `escaped` → `YOU ESCAPED.`、脱出中 → `ESCAPE.`、全回収 → `TURN ON THE ELECTRICAL PANEL.`、チェックポイント 1 → `GET OUT OF THE STATION.`、それ以外 `COLLECT ALL SHARDS.`
- `toSave()`: `version 3`、`checkpoint`、`bonus`、`elapsed` を 0.1 秒単位に丸める、`deaths`、`streakBest`、`secrets`。`restore()`: 既知 ID のみ復元、`checkpoint` はセーブのもの（4 で全回収でなければ 2。ファンゲームのチェックポイント 4 は shard 0）、`bonusShard = !!save.bonus`、`escaped/caught` は false、`lives = maxLives`、`streak = 0`、`deaths`、`streakBest`（0..10）、`secrets`、`elapsed = max(0, save.elapsed)`、`_chase = frenzy`（イベントは発火しない。`hardRespawn` は触らない）。

### `SaveStore`
- キーは `CONFIG.game.saveKey = 'wasami-deception.save.v4'`（ステージ。v3 は原作の Hotel、v2 は以前の館の迷路、v1 はさらに前のレベルのセーブで、読まない）。`CONFIG.debug.field` のときは `'wasami-deception.save.v4.field'`（デバッグフィールドのシャード ID はステージと重なるので分ける）。
- `load()`: `version === 3`、`collected` が string 配列のときのみ受理。`checkpoint` は 2 か 4 ならそれ、それ以外 1。`bonus`/`escaped` は `Boolean()`、`elapsed` と `savedAt` は数値でなければ 0、`deaths` / `streakBest` は有限の数なら 0 以上の整数に、でなければ 0、`secrets` は配列の中の 0 以上の整数を重複なしで（無ければ空）。JSON 不正・例外は null。
- `save()`: storage が無い／例外で false。`persist(announce = true)` は保存が true かつ `announce` のときだけ `hud.showSaving()`（`CONFIG.game.savingToastSeconds = 2.6` 秒表示）と SE `save`(0.35)。
- 保存タイミング: `shardCollected` 後（毎回、表示なし）、赤いシャードの後（表示なし）、`allShardsCollected` 後（表示なし）、ホールのトリガー（SAVING PROGRESS を出す）、配電盤の後の脱出の開始（出す）、`caught` 時（表示なし）。`escaped` 時と `start(true)`（NEW GAME）で `clear()`。ポーズ画面の RESTART? の YES は `restart()`、ゲームオーバーの LAST CHECKPOINT は `restart(true)`、ゲームオーバーの RESTART の YES は `newRun()`（`clear()`）、GIVING UP? とゲームオーバーの QUIT TO TITLE は `location.reload()`（セーブは残る）。

## 依存関係
- import: `../audio/audio`, `../config`, `../core/engine`, `../core/input`, `../core/loader`, `../debug/stats`, `../enemy/brain`（`wrapAngle`）, `../enemy/enemies`, `../enemy/jumpscare`, `../enemy/navgrid`, `../hud/arrow-pointer`, `../hud/hud`, `../hud/minimap`, `../hud/streak`, `../hud/vignette-sides`, `../hud/stage-intro`, `../hud/tablet`, `../player/controller`, `../player/boost-fx`, `../player/teleport`, `../player/walk`, `../render/postfx`, `../world/carpet`, `../world/debug-field`, `../world/breakables`（`Breakables`・`touches`・`warmFade`）, `../world/woodboards`（`BOARDS_BREAK`・`WOODBOARDS_GONE`・`woodboardsOpacity`）, `../world/gate`, `../world/level`（`loadLevel`・`partCollider`・`insideBox`・`boxCentre`・型）, `../world/lights`, `../world/shards`, `../world/collect-fx`, `../world/shard-fx`, `../world/interact`, `../world/special-rules`, `../world/specials`, `./results`, `./save`, `./settings`, `./state`
- 使う側: `src/main.ts`（`Game.create`, `start/resume/restart/respawn/newRun`, `settings` / `saveSettings`, `audio`, `hasSave`, `input.requestLock`）
- 外部: `@babylonjs/core` の `Scene`, `HavokPlugin`, `KhronosTextureContainer2`, `PhysicsBody`, `Vector3`, `Color3/Color4` など; `@babylonjs/havok`（WASM は `?url` import でローカル配信）
- `state.ts` / `save.ts` は Babylon 非依存（`node --test` で実行可能）

## 設定・調整値
- `CONFIG.game`: `lives` 3（ファンゲームは 5、本作の 3 のまま）, `chaseOnAllShards` false, `saveKey`, `savingToastSeconds` 2.6（`shardCount` 301 は本モジュールでは未参照。シャード数はレベルデータ由来）, `stage`（`enemyDelay` 1、`barrierGone` 3、`panel` {stop 1, escape 2, fade 1}、`doorTime` 1.6（デバッグフィールドの門）、`specialSpeed` 4.25（15 記録））, `gateOpenTime` / `gateLift`（デバッグフィールドの門）
- `CONFIG.debug`: `stats`, `chase`（`forcedChase` の初期値）, `freshSave`（`autostart` は `main.ts` が参照）
- `CONFIG.enemy`: `count` 10、`navCell` / `radius` / `navHeight`（格子。`radius` は壊せる物と障壁に触れる体の半径にも）, `model`, `chaseHold` 3, `height`（顔の高さと、壊せる物と障壁に触れる体の高さ）, `catch.time` 3.5（ほかは記録 15）
- `CONFIG.render.clearColor`, `CONFIG.player.gravity`, `CONFIG.player.boost`, `CONFIG.camera.fovFast`（プリウォーム）, `CONFIG.lights.mode` / `CONFIG.lightmap.full|indirect`（読み込むライトマップのページの選択）
- game.ts の定数: `PANEL_USE`（配電盤の視線の箱）、`DASH_CLEAR` 50、`METRO_TOP` −9.5
- すべて URL クエリで上書き可能（`src/core/overrides.ts`）。例: `?debug.freshSave=true&debug.chase=true`

## 既知の制約・注意点
- `save.escaped === true` のセーブは復元しない（クリア後は `clear()` されるため通常は存在しない）。
- タイトル表示中（`start()` 前）の `state()` / `shards()` はセーブ未反映の初期状態。セーブの有無は `hasSave` で見る。
- ステージのトリガー（ホール・ゴール）は足元の点と箱の包含（`insideBox`、y は ±1 m の余裕）で判定する（物理トリガーではない）。敵が壊せる物とロックピック・ダッシュの障壁に触れるのも、敵の頭脳の位置の円柱と箱の重なり（`touches`）で、物理の重なりではない。デバッグフィールドの脱出・門は XZ 距離（1.6 m / 2.4 m、解除 4 m）。
- ファンゲームの開始は列車の中（チェックポイント 0）で、カットシーン・鉄パイプ・列車の走行を経てホームに着くが、本作は列車が着いて扉の開いたホーム（チェックポイント 1）から始める（ユーザーの選んだ範囲）。
- ロックピックとダッシュの障壁は光る板でかけら（板）を持たないので、原作の板張りのバリケードのように崩れ落ちず、その場で薄れて消える（煙と時間はバリケードと同じ、音はファンゲームの `Barrier_Shatter_Cue`）。薄れは板のメッシュの `visibility` で、灯は壊れた瞬間に消える。
- 地上と地下の色補正のゾーンと地図の階は足元の高さ（`METRO_TOP` −9.5 m）で分ける（ファンゲームの 2 つのボリュームは高さでほぼ接している。地図はファンゲームでは階段の 2 つのトリガーで入れ替わる）。
- `frame` の `dt` と物理後フックの `pdt` は別クランプ（どちらも上限 1/15 秒）。
- プリウォームはロード画面中に 4 回描画するため、WebGL2 でもコンパイル時間分ロードが延びる。
- 追跡フラグは毎フレーム代入するので、`state.chase` を外から書いても次のフレームで戻る（F7 とデバッグ API の `setChase` は `forcedChase` を切り替える）。
- 捕獲の演出中はプレイヤーの入力を止めるが、ポーズはできる。死亡画面の間は `paused` のままなので、Esc やフォーカスの喪失でポーズ画面は出ない。リスポーンの後にポインタロックが外れていれば、キャンバスのクリックで取り直す。
- 脱出の画面の SECRETS はステージに秘密が無いので 0 / 2（results.ts の '/2' は Dark Deception の Hotel の定数）。SOUL SHARDS はレベルのシャード数（ステージは 301）。
- LAST CHECKPOINT の警告のフラグは原作ではグローバルなセーブの項目。本作は進行のセーブと別のキーに置き、NEW GAME で戻す。

## テスト（tests/state.test.ts）
`node --test` で実行。以下を検証する:
1. 全シャード回収でイベントが `shardCollected ×3 → allShardsCollected` の順で 1 回ずつ発火し（ステージはフレンジーなし）、重複 ID・未知 ID は false。`objective` が `GET OUT OF THE STATION.` → ホールで `COLLECT ALL SHARDS.` → 全回収で `TURN ON THE ELECTRICAL PANEL.`。`chaseOnAllShards` を渡せば `frenzy` と `chase` が立つ。
2. ホールのトリガー: `reachHall()` は 1 回だけ true で `checkpoint { 2 }`。
3. 配電盤: 全回収前の `usePanel()` は `panelDenied{remaining:3}` で false、全回収後は 1 回だけ true で `checkpoint { 4 }`、`escaping`、`frenzy`・`chase` が false、`objective` が `ESCAPE.`。
4. 脱出の終点: 脱出前（全回収でも）の `tryEscape()` は false、脱出中は true で `escaped { elapsed: 10 }`、以後 `tick` しても経過時間が止まる。
5. 制限時間が無いこと。
6. `SaveStore` の往復: 保存→別インスタンスで `restore` してもイベントは発火せず、`elapsed` は 12.3、チェックポイント 2・4 が戻る。version 2（Hotel）は null、`elapsed` の無い version 3 は 0・チェックポイント 1 で読み、知らないチェックポイントは 1、`collected` が数値配列／非 JSON は null。`clear()` 後も null。
7. 全回収でないのにチェックポイント 4 のセーブはホール（2）に戻る。
8. （関連）`applyOverrides` が URL クエリを既存値の型に変換し、未知キーは無視して適用数 5 を返す。
9. ライフ: 3 で始まり、捕獲ごとに 2, 1, 0。ライフが残っていれば `respawn()` で戻りシャードが残る。0 で `over`。`restore` で 3 に戻る。`maxLives` 6 を渡せば 6。
10. 連続回収: 20 個で節目、捕獲と `restore` で 0 に戻る。500 個続けると 8 つの節目、200 と 500 でライフが 3 → 5。`MAX_LIVES` 6 からは増えない。
11. 捕獲は 1 回だけ true で、以後は回収・配電盤・ホール・脱出が false、経過時間が止まる。`restore(toSave())` で戻る。

## テスト（tests/results.test.ts）
1. `timeRank` が 600 / 720 / 840 s で段を変え、`timeText` が `9 : 05` の形（1 時間で分が折り返す）。
2. すべて S の走り（500 s・赤いシャード 1・秘密 2・死亡 0・節目 250）の行の文字・ランク・加算、合計 289 + 100、FINAL RANK S（EASY なら A）。
3. 秘密が 0 だと FINAL RANK は A まで。遅く・取り残し・2 回死亡・節目なしなら C。
4. 行ごとの表: 赤いシャード 0 → C・+0、秘密 1 → B・+5、死亡 0/1/2/5/6 → S/A/C/C/なし、LAST CHECKPOINT の後はセーブのライフ 3 → C・+5、節目 0/1/3/4/6/7/10 → C/C/B/A/S/C/C。
5. `Counter`: 40 を 0.25 s なら 1 フレームずつ（39/60 s で +40）、10 を 0.25 s なら 2 フレームずつ、0 と 1 は長さ 0。
6. `GameState` の `deaths` と `streakBest` が `SaveStore` の保存・読み込みと `restore` を往復し、以前のセーブ（version 3 の欠けたもの）では 0。

## 変更履歴
- 2026-09-11〜09-13: 初版から、館の迷路・敵・テレポート・ブースト・設定・ライフ・ステージ OP・連続回収・特殊シャード・地図の矢印・原作の Hotel の流れ（祭壇・欠片・エレベーター・ロビー・ポータル）までの変更（詳細は git の履歴のこの記録）
- 2026-09-14: 脱出の画面（原作の `UMG_LevelClear`）のため、リザルトの規則 `results.ts` と `tests/results.test.ts` を足し、`GameState` に `deaths` / `streakBest` / `hardRespawn` を足した。脱出で `endRun()` がゲームを止めてすぐ `onEnd(levelResults())`
- 2026-09-14: 原作の Hotel の秘密と隠し扉、LAST CHECKPOINT の警告のフラグ `checkpointWarning` を入れた。本家に無い表示と音を外した。祭壇と隠し扉を原作の視線で使うようにした。ソウルシャードの取得を原作の `BP_Shard` に寄せた
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 に差し替えた（ユーザーの指示）: `GameState` を `checkpoint`（1 / 2 / 4）・`escaping`・`reachHall()`・`usePanel()`・`tryEscape()`（脱出中だけ）と `checkpoint`・`panelDenied` のイベント、本作の目的の文言、`chaseOnAllShards` の既定 false にし、`ringPiece`・`touchAltar()`・`altarDenied` をやめた。`SaveData` を version 3（`checkpoint`）に、キーを v4 に。`Game` から Hotel の祭壇・欠片・エレベーター・ロビー・ポータル・秘密・隠し扉・`phase` を外し、ステージの流れ（ホールのトリガーと 1 s 後の敵、障壁の `partCollider` と `breakBarrier`・`syncWorld` のダッシュの障壁の 50 m、配電盤の視線と暗転と脱出の開始、ゴールの箱）と `lookTarget` の配電盤・ロックピックの障壁、マーカーと矢印の行き先、`post.zone` の地上と地下、ライトマップのページの読み込み（`pagePattern`）、オーブの硬直をステージの 17 s に。デバッグ API の `hotel()`・`secrets()`・`secretDoors()` を `stage()` に、`state()` の `ringPiece`・`phase`・`doors`・`secrets` を `checkpoint`・`panel`・`barriers`・`gate` に
- 2026-09-14: ステージの灯のギミックをつないだ: `stageLamps`（07 記録の `StageLamps`）を `syncWorld` と `toCheckpoint` で `reset(checkpoint)`、`updateStage` で `update(dt, foesChasing || forcedChase, allShards)`（`foesChasing` は `enemies.update` の `chasing`）、障壁の `setBarrier` / `breakBarrier` でその障壁の灯を `setActor`。デバッグ API の `stage()` に `streetLamps`
- 2026-09-14: ロックピックの障壁を仮のクリックからファンゲームの連打にした: `game/lockpick.ts`（`Lockpick`、`pickDecay`、`interpEaseInOut`）と `updateLockpicks`（箱の中の F、4 m 以内の最初の障壁、ウィジェットを障壁の上に `toScreen`）。`lookTarget` と `use` から障壁を外し（`Target` は配電盤だけ）、デバッグ API の `stage().barriers` に `zone`・`pick`、`state().usable` を `'panel'`・null に
- 2026-09-14: 床の噴出と車を足した: `game/traps.ts`（`Jet`・`CarRun`・`RandomGate`）、`updateTraps`・`placeCar`・`resetTraps`、敵のいない死 `killPlayer`（`catching.actor` を null 可に。突進・顔への向き・揺れ・叫びは敵の捕獲だけ）、`CAR_STUN`、車の BeginPlay の 0.1 s と走り出すたびの塗り（`CAR_PAINTS`）、噴出の煙。デバッグ API の `stage()` に `jets`・`cars`
- 2026-09-14: 扉 6 種を足した: `game/doors.ts`（論理）、`makeDoor`・`updateDoors`・`useDoor`・`breakTrapDoor`・`resetDoors`、`lookTarget` の扉（`DOOR_NEAR`）、罠の扉から出る敵のための特別な敵のプール（`CONFIG.game.stage.specials` 20）。デバッグ API の `stage()` に `doors`、`state().usable` に扉のアクター名
- 2026-09-14: 脱出フェーズを足した: `game/escape.ts`（トラックの Timeline とスプラインの道、当たりとトレース、レベル BP の開始の時計）、`updateEscape`・`resetEscape`・`spawnTruck`・`placeTruck`・`breakDown`、入力の `escapeHold`、チェックポイント 4 の `barrierspeedbust6`。デバッグ API の `stage()` に `escape`
- 2026-09-14: ステージの音を Chaotic Customer 2 のものにした（06 記録）: 曲は `stageMusic`（`toCheckpoint` で切って `reset`、広間で `reachHall`、`updateStage` で `update`。デバッグフィールドだけが `bgm_normal` / `bgm_chase`）、効果音は `cue` / `cueLoop` / `follow`（障壁のループと破砕、扉、噴出、車のエンジンとクラクション、トラックのビープ・エンジン・ブーム、轢かれた音〈`killPlayer(runOver)`〉、配電盤の火花と `Stage_Lights`、シャードの `stageShardSound`）。`barrier_break` と `playAt` をやめた。`CarState` に `engine`・`hornIn`、`TruckState` に `beep`・`engine`、`carBox`。デバッグ API の `state()` に `music`・`stageMusic`
- 2026-09-14: 色補正のゾーンをステージのボリュームの `'surface'` / `'metro'` / `'field'` にし（09 記録）、'hybrid' ではライトマップの両方のページを読むようにした（07 記録）
- 2026-09-14: 地図を地上と地下鉄の 2 つの階にした（11 記録）: 2 枚を撮り、足元の高さで `tablet.minimap.layer` を選び、地下鉄ではマーカーを出さない
- 2026-09-15: 駅の柵とパンチをつないだ: `fences`（ステージの `Fences`、`onBreak` で `woodBreak`）、`player.onCrouch` で `crouch` の音、`updatePunch`（タブレットを下ろしていて照準中でない左ボタン）と `punchTrace`（柵を除く光線と `hitTest`）、`updateStage` の `fences.update`（板が動く間は影を描き直す）、`syncWorld` / `toCheckpoint` の `resetFences`、`hud.setAim`。デバッグ API の `state()` に `crouch`・`punch`、`stage()` に `fences`・`fenceTexts`
- 2026-09-15: パンチを敵にも当てた（ファンゲームのマネキンの `Death_by_punch`。15 記録）: `punchTrace` が柵の区画と敵の体の箱（`rayBox`）の近いほうに当て、敵には `mannequinHit` と倒れたら `mannequinFall`。`Enemies` に予備の個体 `enemy.spares` と `respawnPoints`（ステージの `respawns`）を渡す。デバッグ API の `state().punch` に `last`、`enemies()` に `hits`・`down`・`downTime`・`spare`・`appear`、新しく `ray()`
- 2026-09-15: シャードの障壁をファンゲームの `barrier_C` の BeginPlay どおりにした: `syncWorld` はチェックポイント 2 でだけ立て、チェックポイント 1 では `updateShardBarriers`（`updateLockpicks` の後）が Box に初めて触れたときに立てて `shardBarrierAppear` を鳴らす（`Barrier.appeared`、`setBarrier` が設定）。これまでは全回収まで立ち続け、ホームから歩くと階段の上で道を塞いでいた。デバッグ API に `face(yaw, pitch)` と `stage().route` を足した
- 2026-09-15: テレポートの照準の判定を床の段ごとの格子 `FloorGrids.fromBoxes(level.navBoxes, navOpts, nav)` にした（地上の段は敵の `nav`。駅のホームで輪が出なかった。05・15 記録）
- 2026-09-15: 本作の規範とユーザーの選択で、ファンゲームのパンチをやめ、駅の柵を原作の板張りのバリケードのように 1 クリックで壊れる壊せる物にした: `updatePunch`・`punchTrace`・`punch` / `punchHeld` / `punchHit` / `punchLast`・`hud.setAim` を削除し、`fences`（`Fences`）を `create` で作ってコンストラクタに渡す `breakables`（`Breakables`。`onBreak` で `BOARDS_BREAK`）に、`resetFences` を `resetBreakables` にした。`Target` に `{ kind: 'break'; index }`（`lookTarget` の候補と `use` の `break`）、`updateStage` で `active` な敵の `touch` と `update`、プリウォームで `breakables.showcase`。`Enemies` に予備の個体とリスポーンの地点を渡さない（15 記録）。デバッグ API の `state().punch` と `enemies()` の `hits`・`down`・`downTime`・`spare`・`appear` を削除し、`stage().fences`・`fenceTexts` を `stage().breakables` に、`state().usable` に壊せる物の名前を足した
- 2026-09-15: 敵の速さの倍率 `enemyWalkScale` を `enemySpeedScale`（`{ walk, chase }`）にし、EASY で追跡も 300 / 430 にした（原作の BP_Monkey の BeginPlay。15 記録）
- 2026-09-15: 本作の規範で、ロックピックとダッシュの障壁を原作の板張りのバリケードのように壊すようにした: 視線の手のマークと左クリック（`Target` の `{ kind: 'barrier'; barrier }`、`lookTarget` の候補と `use`）か `active` な敵の重なり（breakables.ts の `touches`）で `breakBarrier` し、ファンゲームの障壁の破砕音に `breakables.smoke` の煙を足し、`WOODBOARDS_GONE` 4 s で消える（2 s から `woodboardsOpacity` で薄れる。`setBarrier` は `visibility` を 1 に戻す）。F の連打（`game/lockpick.ts`・`tests/lockpick.test.ts`、`updateLockpicks`・`toScreen`・`PICK_REACH`・`ACTOR_HEIGHT`、`Barrier.pick`、`hud.setLockpick` / `lockpickPress` の呼び出し）とブースト中の走り込みでダッシュの障壁が割れるのをやめた。プリウォームで障壁の板の半透明の変種も作る（`warmFade`）。デバッグ API の `stage().barriers[].pick` を削除し、`state().usable` に障壁のアクター名を足した。「既知の制約」の古い記述（連打と罠などが未実装）を今の実装に合わせた
- 2026-09-15: ファンゲームのしゃがみ・スライドをやめたので（本作の規範。05 記録）、`player.onCrouch` の `cue(STAGE_SOUND.crouch)` とデバッグ API の `state().crouch` を削除し、`stage().route` を `{ at }` にした。壊せる物に駅の板張りの通り道（出口の板と低い戸口の格子の壁）を足した（`new Breakables(scene, stage.fences, stage.boards)`。07 記録）
