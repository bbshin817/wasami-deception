---
title: 全体像(起動順・フレーム更新順・モジュール依存)
sources:
  - src/main.ts
  - src/game/game.ts
updated: 2026-09-15
---

# 全体像

## 役割
WASAMI DECEPTION は Babylon.js 9.26(既定は WebGL2。`render.preferWebGPU=true` なら WebGPU を試し、失敗時は WebGL2)+ Havok + Vite 8 / TypeScript で作る一人称ホラー探索の垂直スライス。
舞台はファンゲーム『Chaotic Customer 2』の Zone_1（UE5.4 の書き出し `cc2_reference` の配置とメッシュ・テクスチャで組み立て、ライトマップを Blender で焼き直した地上の迷路と地下鉄の駅。12 記録。2026-09-13〜14 は原作『Dark Deception』の Hotel だった）。列車が着いて扉の開いた地下鉄のホームから始まり（チェックポイント 1）、東端の階段で地上へ上がると、ホールのトリガーで敵が出る（チェックポイント 2）。迷路（約 130 × 277 m）のシャード（ワサミ餅）301 個に触れて回収するとシャードの障壁 2 枚が壊れ、奥の配電盤を見て（画面の中央に手のマーク）左クリックすると暗転して脱出フェーズ（チェックポイント 4）になり、終点の箱に入るとクリア（ファンゲームのレベル BP の流れ。04 記録）。途中にはロックピックの障壁とダッシュの障壁があり、どちらも見て（手のマーク）左クリック 1 回か敵が触れると壊れる（駅の柵と同じく原作の Hotel の板張りのバリケードに倣う。04・07 記録）。制限時間は無い（経過時間はクリアタイムに使う）。敵はワサミの姿の 10 体（ファンゲームのマネキンの出現点。キャラクターのモデルは使わない）で、見つかると最短経路で追ってくる。迷路には特殊シャードが 2 つ、60 秒ごとに出現点を移りながら現れる: 取ると敵が 17 秒止まるオーブ（ENEMIES STUNNED）と、敵が 60 秒タブレットの地図に出る赤いシャード（ENEMIES REVEALED。08 記録）。ライフは 3 つ（ファンゲームは 5）。捕まると 3.5 秒の詰め寄りの後に死亡画面（Dark Deception の `UMG_DeathScreen`）が出て、ライフが残っていればチェックポイントで再開し、0 ならゲームオーバー（RESTART / LAST CHECKPOINT / QUIT TO TITLE）。敵の追跡中に「追跡フラグ」（BGM が Panic Mode の曲に替わる。画面は変えない）が立つ。罠・灯の点滅・扉・脱出の追手はこれから入れる（進捗記録 `.claude/progress/20260914-cc2-zone1.md`）。
この記録は各モジュール記録(01〜14)を横断する骨格だけを書く。詳細は各記録を参照。

## モジュール構成と依存
```
src/main.ts ──(dynamic import)──> src/game/game.ts(Game クラス: 全体統括)
                                     ├─ core/engine   createEngine(既定 WebGL2)
                                     ├─ core/loader   AssetStore(manifest.json に基づく進捗付きストリーミング、Blob URL)
                                     ├─ core/input    Input(キー・マウス・Pointer Lock)
                                     ├─ world/level   loadLevel(ステージの glTF + KTX2 + ライトマップのページ + colliders.json の衝突と階段の坂 + stage.json)
                                     ├─ world/debug-field buildDebugField(`debug.field` のときステージの代わりに組み立てる検証用の広場)
                                     ├─ world/lights  LampSystem(灯 301、灯ごとの色と範囲、影付き PointLight 3 灯のプール。'hybrid' で全部のライトマップ + 3 灯の鏡面と影、灯の状態が焼き込みから変わった分の拡散。world/stage-lamps StageLamps が街灯の点滅・脱出の色・レトロな灯・障壁と罠の灯を決める)
                                     ├─ world/shards  Shards(回収対象 301 個)
                                     ├─ world/specials SpecialShards(特殊シャード: 赤いシャードとオーブ。規則は world/special-rules、取得の演出の値は world/collect-fx)
                                     ├─ world/secrets Secrets / SecretDoors(原作の秘密のフォルダー 2 個と、その手前の隠し扉 2 枚。規則は world/secret-rules)
                                     ├─ world/gate    Gate(エレベーターの扉が左右に開く / デバッグフィールドの門がせり上がる)
                                     ├─ world/breakables Breakables(壊せる物 = 駅の柵の区画と板張りの通り道 2 か所〈出口の板・低い戸口の格子の壁〉。原作の Hotel の板張りのバリケード〈world/woodboards の BP_01_Woodboards の値〉のように 1 クリックか敵の接触で壊れ、柵の板は物理で飛び、通り道の打ち付けた板はその場で崩れ、煙が上がり、2 s 後から 2 s で消える。ロックピックとダッシュの障壁が壊れるときの煙 `smoke`、敵の重なり `touches`、薄れる物のプリウォーム `warmFade` も game が使う)
                                     ├─ world/portal  Portal(ロビーのポータルの渦とルーンの輪)
                                     ├─ enemy/navgrid NavGrid(衝突箱からの歩ける格子、A*、視線判定)
                                     ├─ enemy/enemies Enemies(ワサミのクローン 10 体。ホールのトリガーの 1 s 後に出る。特別な敵のプール 20 は罠の扉と脱出の群れ用。enemy/brain の EnemyBrain で徘徊・追跡・グリッチ・捕獲)
                                     ├─ player/controller PlayerController(Havok キャラクターコントローラ、FOV、ブースト)
                                     ├─ player/teleport   Teleport(テレポーテーション: Q で照準・ホイールで距離・左クリックで確定、照準の輪、0.12 s 後のカプセルのスイープ。原作のカメラアニメ = player/teleport-fx)
                                     ├─ render/postfx PostFx(TAA → テレポートの露出とティント → UE 4.21 のトーンマップ（原作のボリュームの色補正を焼いた LUT。render/ue-grade）→ DefaultRenderingPipeline（ガンマの符号化）→ モーションブラー → スピードブースト)
                                     ├─ hud/tablet    Tablet(3D タブレット + DynamicTexture 画面) ── hud/minimap Minimap
                                     ├─ hud/hud       Hud(HTML の字幕・SAVING)
                                     ├─ hud/vignette-sides VignetteSidesFx(特殊シャードの ENEMIES STUNNED / ENEMIES REVEALED)
                                     ├─ hud/collectables CollectableFx(秘密を取ったときの NEW EXTRAS UNLOCKED!)
                                     ├─ audio/audio   AudioManager(WebAudio、バス、ループ、バリアント)
                                     ├─ game/state    GameState(規則・イベント)、game/save SaveStore(localStorage)、game/results results(脱出の画面のリザルトの規則)
                                     └─ debug/stats   FrameStats(F3 の FPS 表示)
src/config.ts は全モジュールから参照される調整値の集約。core/overrides が URL クエリで上書きする。
```
`config.ts` 以外に自前モジュール間の循環依存はない。`hud/tablet` だけが `game/state` の型と `hud/minimap` を import する。`hud/minimap` と `game/game` は `hud/arrow-pointer`(Babylon に依存しない地図の矢印の規則)を import する。

## 起動シーケンス(main.ts → Game.create)
1. `main.ts`: `styles.css` を読み、`applyOverrides(CONFIG, URLSearchParams)` で URL クエリ上書き。DOM(`#scene`, `#loading`, `#title`, `#pause`, `#end`, `#progress-fill`, `#progress-label`, `#btn-start`, `#btn-reset`, `#options`)を取得し、`#title-version` に `package.json` の version を入れる。
2. `boot()`: 初期表示はローディング画面(`#loading`、進捗バー + ラベルのみ)。進捗 0 で「ENGINE を読み込んでいます…」を出し、`import('./game/game')` で Babylon 本体を遅延ロード。
3. `Game.create(canvas, hooks)`:
   - 0.01 `createEngine` → WebGPU か WebGL2。`Scene` 生成(clearColor は `render.clearColor`、ambient 黒、`skipPointerMovePicking`)。
   - `KhronosTextureContainer2.URLConfig` を `public/vendor/ktx2/` のローカルファイルに向ける(CDN 不使用)。Havok wasm のロードを並行開始。
   - `assets/manifest.json` を読み、ライトマップは `lights.mode` に応じて `lightmap.indirect` か `lightmap.full` のページの組だけ選んで `AssetStore.preload`(進捗 0.05〜0.88、MB 表示)。
   - 0.89 Havok プラグイン有効化(重力 `player.gravity`)。
   - 0.90 `level-meta.json` を取得し `loadLevel`(`debug.field` のときは読み込むアセットをモデル・音・HDR だけにし、代わりに `buildDebugField` で検証用の広場を組み立てる。07 記録)。`GameState` を生成(セーブ `game.saveKey` はここでは反映せず、RESUME の `start()` で反映する。`debug.freshSave` で無視)。`debug.chase` で追跡フラグ ON。
   - `Input`, `PlayerController`(activeCamera 設定), `Shards`, `LampSystem`(シャードを影キャスターに追加), 特殊シャード、障壁のコライダー（`partCollider`）、`Gate`(デバッグフィールドの門), `PostFx`, `Minimap`, `Tablet` の順に生成。Tablet は後続のプリウォームでマテリアルと ReflectionProbe をコンパイルさせるため、この時点で作る。
   - 0.93 `AudioManager` 生成、`voices/manifest.json` から字幕表、manifest の `kind: 'audio'` を全てデコード。続けて `NavGrid.fromBoxes(level.navBoxes)`、テレポートの照準の床の段ごとの格子 `FloorGrids.fromBoxes`（地上の段はその格子）と敵のモデル（`CONFIG.enemy.model`）から `Enemies`（敵は出現位置で巡回を始める）。ステージなら駅の柵の区画と板張りの通り道の `Breakables`（壊せる物。プリウォームで描くのでここで作る）。
   - 0.96 `scene.whenReadyAsync()` 後にプリウォーム: テレポートの輪・敵・特殊シャード・シャードの閃光・壊せる物（透けた板と煙のスプライト。`showcase`）を開始地点の前に出し、ロックピックとダッシュの障壁の板も透けた変種で描かせ（`warmFade`）、カメラを 0/90/180/270° に向け、奇数回は `camera.fovFast`（115°）で `scene.render()` を 4 回。WebGPU のパイプライン初回生成ヒッチ対策。
   - `minimap.capture(staticMeshes, [playerStart, ...shards], playerStart.y)`(床から 2.1 m で切る)で地図画像を作り、`tablet.setVisible(false)`。進捗 1「READY」。
   - `new Game(...)`: `FrameStats` 生成、`wireEvents()`、タブレット初期表示、`engine.runRenderLoop(frame)`、resize ハンドラ。`window.__wasami` にデバッグ API。
4. `main.ts`: セーブ有無で `RESUME` + `NEW GAME` / `NEW GAME` の切替(セーブありの `NEW GAME` は RESTART? の確認 → YES で暗転してからその場で `start(true)`)、`TitleFx`(原作のタイトルのアニメと音。`hud/title.ts`)を作ってタイトルメニュー(OPTIONS は原作の設定画面 `hud/options.ts`。SAVE & EXIT で `game.saveSettings`)を結線(RESUME / NEW GAME は原作の暗転のアニメが終わる 3.75 秒後に開始)。`PauseFx`(原作のポーズ画面 `UMG_Pause` のアニメと音。`hud/pause.ts`)を作ってポーズ画面(RESUME / RESTART? / OPTIONS / QUIT → GIVING UP?。OPTIONS はタイトルと同じもの)と終了画面のボタンを結線。`DeathFx`(原作の死亡画面 `UMG_DeathScreen` のアニメと音。`hud/death.ts`)を作って死亡画面(ゲームオーバーの RESTART? / LAST CHECKPOINT / QUIT TO TITLE)を結線。`LevelClearFx`(原作の脱出の画面 `UMG_LevelClear` のアニメと音。`hud/level-clear.ts`)を作って NEXT を結線。キャンバスか 16:9 の舞台(`#stage`)の外の黒帯のクリックでポインタ再ロック。`#loading` を隠してタイトル画面(`#title`)を表示して `fx.reveal()`(黒から明け、BGM がフェードイン)。`debug.autostart` ならタイトルを出さずに自動開始。

## フレーム更新順(Game.update)
1. 初回のみ `scene.onAfterPhysicsObservable` に物理後処理を登録: `player.step(pdt)` → `player.syncCamera(pdt)` → `teleport.applyToCamera`(発動中のカメラアニメの FOV とシェイク) → 捕獲中は原作の JumpscareShake(`enemy/jumpscare.ts`) → 特殊シャードの取得の揺れ(位置) → ソウルシャードの取得の揺れ(原作の `BP_CameraShake_ShardCollect` のロールと FOV。回収ごとに足し合わせる。`world/shard-fx`) → 門の揺れ(`shake`)適用。`pdt` は `scene.deltaTime` を 1/15 秒で上限（`debug.fixedDt > 0` ならその値。`frame` の `dt` も同じ）。
2. `input.beginFrame()` で今フレームの視点差分を取得。F3 で stats 表示切替（GPU タイマーも表示中だけ動かす。03 記録）。
3. プレイ中(`playing && !paused`)のみ:
   - F7 で追跡フラグ切替、`player.update(dt, look)`（05 記録）。
   - Space/Tab でタブレット出し入れ(プレイヤーが動けるときだけ。効果音 `tablet_down`/`tablet_up`)、Z でミニマップズーム(タブレットを上げている時だけ、即座に。`ui_select`)。
   - `shards.touching(player.feet)` でプレイヤーのカプセルに触れているシャードを判定し、それぞれ `state.collect`（触れると自動回収。回収キーは無い。判定は原作の重なりで中心から約 1 m。取ったシャードは原作どおりその場で消えて閃光・揺れ・取得音。08 記録）。
   - `specials.touching(player.feet)` の特殊シャードは `takeSpecial` → `state.collectSpecial`（04・08 記録）。配電盤・扉・立っている壊せる物とロックピック・ダッシュの障壁（2 m）は、カメラの前の視線に入ると画面の中央に手のマーク（`UMG_Interact`）が出て、左クリックで使う（`lookTarget` → `use`: 配電盤は脱出の開始か拒否、扉は開閉、壊せる物と障壁は 1 回で壊れる。原作の `BP_01_Woodboards` の `InteractWithObject`。07 記録）。
   - `enemies.update(dt, feet, sprinting, gateOpen)`。捕まえた敵がいれば `state.catchPlayer()` と捕獲の演出（`updateCatch`: 敵が詰め寄り、視点が顔へ引き寄せられ、JumpscareShake が掛かり、`enemy.catch.time` の 3.5 秒後にゲームを止めて `hooks.onDeath(lives)`）。敵の追跡（とその後 3 秒）・フレンジー・F7 で追跡フラグを決める。
   - `state.tick(dt)`（経過時間。制限時間は無い）。
   - ステージの流れ（`updateStage`）: ホールのトリガーで `reachHall()`（保存して 1 s 後に敵）、敵の追跡の間は街灯 48 が一斉に点滅（`stageLamps.update`）、曲はホールから Carol ⇄（追跡中）Dethsmass、全回収で消え、脱出で Merry_horrors（`stageMusic.update`。06 記録）、シャードの障壁はチェックポイント 1 では隠れていてプレイヤーが Box に初めて触れると立つ（`updateShardBarriers`。チェックポイント 2 では最初から立つ）、床の噴出と走る車は触れると死（`updateTraps`・`killPlayer`）、扉 6 種は左クリックで開閉し、罠の扉は板に乗ると 1/4 で壊れて敵が出る（`updateDoors`）、脱出（チェックポイント 4）ではトラックがスプラインを走り群れが追い、ゾーンの箱ごとに次のトラックや群れが来る（`updateEscape`）、動いている敵が壊せる物の箱に入るとそれが壊れ（`breakables.touch`）、飛んだ板が動く間と消えるときだけ影を描き直す（`breakables.update`）、動いている敵がロックピックかダッシュの障壁の箱に入るとそれも壊れる（壊れた障壁は煙を出し、2 s 後から 2 s で薄れて消える）、配電盤の後は 1 s で止まり 2 s で暗転して脱出の開始（チェックポイント 4）へ、脱出中にゴールの箱で `tryEscape()`。デバッグフィールドは門の前で配電盤を使った扱いにし、出口で脱出。
   - `teleport.update(dt, active)`(Q の照準・ホイールの距離・左クリックの確定。確定の 0.12 s 後にカプセルをスイープで動かし、経路上のシャードを回収。確定後の移動とカメラアニメはポーズ中も進む)。
   - `playing && !paused`（捕獲中も）なら `updateSpecials(dt)`: 特殊シャードの時計（出現・見え隠れ・移動）と見た目、赤いシャードの地図の表示（60 s）、取得の演出のポストプロセスの重み。取得の演出のカメラの揺れ（位置）は物理後フックで足す。
   ステージ OP の最中（`intro` があり `playing && !paused`。捕獲中も）は `updateIntro(dt)`: 時計を進め、13 s でプレイヤーを戻してタブレットを上げ、13.5 s で最初の台詞（`greet`）、14 s で OP を終え、`StageIntro.show(introAt(t))` で黒とタイトルカードを描く（10 記録）。
4. 常時: `lamps.update(dt, cameraPos)` → `shards.update(dt)` → デバッグフィールドの門の `update(dt)`(動いた間は `lamps.refreshShadows()`) → `post.zone`（足元が地下鉄のボリュームの上端 −9.5 m より下なら `'lobby'`、上なら `'maze'`、デバッグフィールドは `'hotel'`。ステージのボリュームの名前にするのは 09 の作業）→ `post.update(teleport.fx, boost)`（`boost` はブーストのティント・ウィジェット・ブラー・揺れ）。
5. タブレット: 目標・ブーストゲージ、マーカー(未回収シャード `#d21ee6`、全回収で配電盤・脱出中はゴールの印 `#f0cc7a`。見えている特殊シャード（赤い三角・橙の四角）と、赤いシャードの後の敵（赤い三角、向き付き）も)と、地図の矢印(`arrowAim()` → `arrow.aim` / `arrow.update`: 最寄りのシャード、全回収で配電盤、脱出中はゴール。11 記録)を `tablet.update` へ。
6. `audio.setListener(cameraPos, forward, up)`。stats 表示中は API / FOV / dash / post のデバッグ情報 / 影付きランプ index / chase を描画。

## 状態遷移とイベント(概要)
- `GameState` がホールの `reachHall` で `checkpoint { 2 }` → `collect` → 全回収で `allShardsCollected`（シャードの障壁が壊れる）→ 配電盤の `usePanel` で `checkpoint { 4 }`（全回収前は `panelDenied`）→ ゴールの `tryEscape` で `escaped`（脱出の前は何もしない）。敵に捕まると `caught { lives }`（ライフ −1）。
- 回収・全回収・チェックポイント・捕獲のたびに `SaveStore` へ保存し、チェックポイント（ホールと脱出の開始）でだけ HUD に `SAVING PROGRESS`。
- 脱出で `hooks.onEnd(results)`（ゲームを止めてすぐ。リザルトは `game/results.ts`）→ `main.ts` の `LevelClearFx`（原作の `UMG_LevelClear`: 赤い You Escaped! → リザルト → NEXT で Fade Out → 再読込でタイトル。`hud/level-clear.ts`）。捕獲の 3.5 秒後に `hooks.onDeath(lives)` → `main.ts` の `DeathFx`: ライフが残っていれば画面が済んでから `game.respawn()`（チェックポイント、シャードとライフはそのまま）、0 なら RESTART（`game.newRun()`: ステージを最初から）/ LAST CHECKPOINT（`game.restart(true)`: セーブからライフ満タン。原作の Used Hard Respawn?）/ QUIT TO TITLE（再読込）。
詳細は 04-game-state-save.md。

## 主要な設計判断(理由は各記録の「既知の制約・注意点」)
- **TAA** は `DefaultRenderingPipeline` に無いため `TAARenderingPipeline` を先頭に重ねる。WebGPU では PrePass の速度テクスチャが取れず、モーションブラーは `GeometryBufferRenderer` から速度を取り、TAA は静止時のみ蓄積。
- **ライトマップ**は Babylon PBR が加算しかしないため、影なし強度 0 の「キャリア」ライト + `LIGHTMAP_SHADOWSONLY` で拡散項に 1 回だけ乗せる。
- **影付きライト**は近い 3 灯だけ(出力 ÷ 距離² 順)をプールから割り当て、その灯の色と原作の減衰半径で 0.35 秒でフェード。シャドウマップは移動時と扉の稼働中だけ再描画し、描くのはライトの範囲に届くメッシュだけ(静的メッシュは 24 m のタイル 29 個に分けてある)。
- **衝突**は pak に原作の衝突メッシュが無いので、原作の配置の三角形を 10 cm の格子に描いて矩形に併合した箱（`colliders.json`）。**敵の経路**は同じ箱から格子を作って A* で探す。敵の頭脳は Babylon 非依存で `node --test` で検証する。
- **原作の素材の範囲**: ステージとオブジェクトのメッシュ・テクスチャ・配置は原作のものを使い、原作のキャラクターが描かれた絵（額の絵・看板・床の紋章・ポータルのロゴ）はワサミの絵に差し替える（`.claude/guides/original-fidelity.md`、ユーザーの指定）。
- **初回バンドル**は数十 KB(ローディング画面 + タイトル画面)。Babylon とアセット(約 137MB)はローディング画面の裏で読み込み、完了後にタイトル画面へ切り替える。

## 開発コマンド
| コマンド | 内容 |
| --- | --- |
| `npm run dev` | Vite 開発サーバー http://127.0.0.1:5190/ |
| `npm run build` / `npm run preview` | 型チェック + ビルド / http://127.0.0.1:4173/ |
| `npm test` | `tests/state.test.ts`(node:test、`--experimental-transform-types`) |
| `npm run typecheck` | `tsc --noEmit` |
| `npm run assets:*` | 素材パイプライン(13-asset-pipeline.md、12-level-generation-blender.md) |
| `npm run bench` | Playwright ベンチ(14-bench-and-smoke-tooling.md) |
| `npm run records:check` / `records:update` | 実装記録の同期チェック / ハッシュ更新 |

## 変更履歴
- 2026-09-11: 初版(現行実装を記録)
- 2026-09-11: 初回ロードをローディング画面 → タイトル画面の 2 段構成に変更
- 2026-09-11: タイトル画面を原作のメインメニュー風に作り直し(RESUME / NEW GAME / OPTIONS、`#api-label` を廃止して OPTIONS へ)
- 2026-09-11: シャードを触れて自動回収に変更(E / 左クリック回収と HUD プロンプト、タブレットの E スロットを撤廃)。見た目はワサミ餅のモデル
- 2026-09-11: 描画 API の既定を WebGL2 に変更(通常の Chrome の WebGPU でキャンバス全体が黒くなるフレームが挟まるため。WebGPU は `?render.preferWebGPU=true`)
- 2026-09-11: タブレットの本体・画面 UI・地図の色を原作の参考画像に合わせて作り直した(10・11 記録)
- 2026-09-11: タブレットが視点移動で動かなくなり、ダッシュで奥へ・壁際で手前へ奥行きだけが動くようにした。game.ts は視点差分を渡さない(10 記録)
- 2026-09-11: E のテレポーテーションを追加(player/teleport、05 記録)
- 2026-09-11: 操作を原作どおりにした(Q = テレポーテーションの位置調整/キャンセル、ホイール = 距離、左クリック = 確定、E = スピードブースト)
- 2026-09-11: セーブの反映を読み込み時から `start()`(RESUME)へ移した。NEW GAME はリロードせずに最初から始まる(04・08 記録)
- 2026-09-11: セーブありの NEW GAME に原作の RESTART? 確認ダイアログと、YES の後の赤い閃光・暗転を追加(01・10 記録)
- 2026-09-11: レベルを館の迷路(部屋 8・廊下 14・シャード 100・壁灯 49)に作り替え、制限時間を 6 分にした(04・12 記録)。`torch_loop` は近い 6 灯だけで鳴らす
- 2026-09-11: 館を約 121 × 116 m(部屋 17・廊下 31・壁灯 120)に広げ、シャードの間隔を 3.5 m 以上にし、制限時間を 12 分にした(12・01 記録)。敵(ワサミ 3 体、Murder Monkeys 型)と捕獲による終了 `caught` を追加(15・04 記録)
- 2026-09-12: テレポートの演出を原作の検証映像に合わせ、タブレットの矩形を `post.maskSource` で描画時に読むようにした。`debug.fixedDt` を追加(05・09・10・04 記録)
- 2026-09-12: スピードブーストの演出(発動の 1 フレームの閃光、走行中の赤い縁と集中線、FOV 101°、タブレットの赤と押し下げ)を追加(05・09・10・04 記録)
- 2026-09-12: `?debug.field=true` のデバッグフィールド(館の代わりにコードで組み立てる検証用の広場)を追加(07・04・01 記録)
- 2026-09-12: F3 の表示に GPU(ブラウザが使っているアダプタ)と DPR を加え、GPU タイマーを表示中だけ動かすようにした(03・04 記録)
- 2026-09-12: テレポーテーションを pak_reference の原作データに合わせて作り直し、露出とティントのパスを TAA の直後(トーンマップ前)に移した(05・09・10・04・01 記録)
- 2026-09-12: タイトル画面を原作のデータ(`UMG_TitleScreen` / `UMG_PopUp`)に倣って作り直した(配置・原作の UI 素材・アニメ・音。`hud/title.ts` を追加し、メニューの結線を `Game.create` の後へ移した)
- 2026-09-12: タイトルの OPTIONS を原作の設定画面(`UMG_Options`)に倣って作り直し、プレイヤーの設定(`game/settings.ts`)を加えた(10・01・04・13 記録)
- 2026-09-12: ポーズ画面を原作(`UMG_Pause`)に倣って作り直した(`hud/pause.ts` の `PauseFx`、RESTART? の YES は `game.restart()`。10・01・04・13 記録)
- 2026-09-13: ユーザーの指示で制限時間を撤廃した(`GameState` は経過時間 `elapsed` だけを数え、時間切れ `timeUp`・残り 30 秒の警告・`TIME'S UP` の終了画面をなくした。04・01・06・14 記録)
- 2026-09-13: ユーザーの指示で、舞台を手続き生成の館から原作の Hotel（迷路＋ロビー、シャード 289）に置き換えた。原作の配置とメッシュ・テクスチャで組み立ててライトマップを焼き直し（12・13 記録）、全回収 → 祭壇の欠片 → エレベーター → ロビーのサル → ポータルの流れを原作のレベル BP に倣って作った（04・07・08・15 記録）
- 2026-09-13: 死亡時とゲームオーバー時を原作に倣った: ライフ 3、捕獲の 3.5 秒の詰め寄りと JumpscareShake、原作の死亡画面 `UMG_DeathScreen`（`hud/death.ts`）、リスポーン・館の作り直し（`newRun`）・LAST CHECKPOINT。赤い閃光と終了画面の `caught` をなくした（04・01・10・15・08・13・06・14 記録）
- 2026-09-13: 原作の特殊シャード（敵を 15 s 止めるオーブ `BP_PowerOrb`、敵を 60 s 地図に出す赤いシャード `BP_BonusShard`）を、出現・見え隠れ・移動、取得の演出（`UMG_VignetteSides`・色調・閃光・揺れ・球・音）ごと原作のデータどおりに足した（world/specials・special-rules・collect-fx、hud/vignette-sides。04・08・09・10・11・12・13・15・01・06・07 記録）
- 2026-09-13: 原作の地図の矢印 `BP_ArrowPointer`（迷路の残りが 100 未満で最寄りのシャード、全回収で像、欠片の後はエレベーター、ロビーではポータルを指す弧）を足した（hud/arrow-pointer。04・10・11 記録）
- 2026-09-13: 見た目を本家に近づけるため、トーンマップを UE 4.21 のもの（原作のポストプロセスボリュームの色補正とフィルミックのカーブを焼いた LUT、cos⁴ のビネット）にし、迷路とロビーで原作のボリュームを切り替えるようにした。UE 4.21 のガウスブルーム（原作の強さ 8）と原作の指数高さフォグを足し、ゾーンごとの露出を本家の収録で合わせた。色収差・グレイン・灯のグレアのスプライトを外し、結晶の発光を抑えた（render/ue-grade。09・01・04・07・13 記録）
- 2026-09-13: 原作の画面空間の反射（UE 4.21 の SSR、Quality 50 の 16 歩の鏡面の光線を照らされた材質が辿り、TAA の履歴を映す）と、深度のパスが静的な形を格子ごとに束ねて描く深度の代理を足した。デバッグ API に `perf()` を足した（render/ssr・render/depth-proxies。09・07・01・04・14 記録）
- 2026-09-14: ユーザーの指示で、脱出の You Escaped! とリザルトを原作の `UMG_LevelClear` とレベル BP のデータどおりにした（game/results・hud/level-clear。ポータルでゲームを止めてすぐ出し、6 行のランクと加算シャード、TOTAL SHARDS、FINAL RANK、NEXT でタイトルへ。GameState に死亡数・ストリークの最高・LAST CHECKPOINT の使用。04・10・01・06・13・14 記録）
- 2026-09-14: ユーザーの指示で、原作の Hotel の秘密（`BP_Collectable` 2 個。隠し扉 `BP_Openabledoor` 2 枚の奥）と、LAST CHECKPOINT の S ランクの警告を原作のデータどおりに入れた（world/secrets・world/secret-rules・hud/collectables。隠し扉を動く部品にしてレベルを `--no-bake` で書き出し直し、SECRETS が数えられて FINAL RANK S が取れるようになった。04・08・10・12・13・01・06・14 記録）
- 2026-09-14: 原作の Hotel が鳴らさない音をやめた: 全回収の `barrier_success`・`portal_unlocked`・祭壇の `portal_loop` と 1.8 s 後の `found`、追跡の `breath`、ランダムなスイートナー、`amb_bass`、`torch_loop`（`updateTorches` と `torchLoops`）（06・13 記録）
- 2026-09-14: 原作に無い追跡の赤いティントとダッシュ中のラジアルブラーを外した（原作データの BP_DD_PlayerCharacter・BP_Monkey・01_Hotel のレベル BP に追跡やダッシュの画面の効果は無く、画面の縁のラジアルブラーと赤はスピードブーストのもの）（09 記録）
- 2026-09-14: ユーザーの指示で、ステージを Hotel からファンゲーム『Chaotic Customer 2』の Zone_1 に差し替えている（進行中）: 書き出しの配置で組み立てて焼き直し（ライトマップのページ、UE5 の灯 301、SkyLight、反射キャプチャ）、UE の当たりからの衝突と階段の坂、KTX2 のテクスチャ（人物の絵はワサミに）、駅のホーム → ホール → シャード 301 → 障壁 → 配電盤 → 脱出の流れ、灯の 'hybrid' モード（04・07・12・13・15・01 記録）
- 2026-09-14: ユーザーの指示で、祭壇と隠し扉を触れて使うのをやめ、原作の視線（カメラの前 2 m）と手のマーク（`UMG_Interact`）と左クリックで使うようにした（04・08・10 記録）。フレンジーの敵の強化を撤廃した（15 記録）
- 2026-09-14: ユーザーの指示で、ソウルシャードの取得を原作の `BP_Shard` に寄せた（その場で消えて閃光・揺れ・取得音 0.65。当たりは原作の重なりで約 1 m）。タブレットの数字を原作の helvetica-neue-bold に寄せた（04・06・08・10 記録）
- 2026-09-15: ファンゲームの駅の柵（`Fence_battle`: パンチで落ちる板と床の案内の文字）と、しゃがみ・スライド（C）・パンチ（左ボタン）を足した（world/fences・player/crouch・player/punch。04・05・06・07・10・01・13 記録）
- 2026-09-15: パンチを敵にも効かせた（ファンゲームのマネキンの `Death_by_punch`: 倒れて溶けて消え、抽選で出現地点に新しい個体。enemy/knockdown。04・06・13・15・01 記録）
- 2026-09-15: シャードの障壁をファンゲームの `barrier_C` の BeginPlay どおりにした（チェックポイント 2 でだけ立ち、1 では Box に初めて触れると立つ。`updateShardBarriers` を `updateLockpicks` の後に）。colliders.json の経路 `route` を flow-check がホームから歩き、階段の段と低い壁をデータで直した（04・06・07・12・14 記録）
- 2026-09-15: 本作の規範（あらゆる要素は本家に基づく、本家に無いギミックは複雑にしない）とユーザーの選択で、ファンゲームのパンチ（player/punch、Aim の HUD、振りのシェイク）と敵が倒れて出現地点に戻るリスポーン（enemy/knockdown、予備の個体）と床の案内の文字をやめ、駅の柵を原作の Hotel の板張りのバリケード `BP_01_Woodboards` のように 1 クリック（か敵の接触）で壊れて消える壊せる物にした（world/breakables・world/woodboards。04・05・06・07・10・13・14・15・01 記録）
- 2026-09-15: 本作の規範で、ステージのロックピックの障壁（ファンゲームの F の連打と `Lockpicking` のウィジェット）とダッシュの障壁（ブーストで走り込む）を、駅の柵と同じく原作の板張りのバリケードのように見て（手のマーク）左クリック 1 回か敵の接触で壊れ、煙を出して 2 s 後から 2 s で薄れて消えるようにした（game/lockpick と `#lockpick` を削除。04・07・10・01・12・14・15 記録）
- 2026-09-15: 本作の規範（プレイヤーの能力は本家に基づく）とユーザーの選択で、ファンゲームのしゃがみ・スライド（player/crouch、C）をやめ、それでくぐっていた駅の出口の板と低い戸口の格子の壁を、駅の柵と同じく 1 クリック（か敵の接触）で壊れる壊せる物（板張りの通り道。打ち付けた板はその場で崩れて薄れる）にした。flow-check の経路は立って歩けるマスだけを通る。ステージの素材は焼き直す（04・05・06・07・12・13・14 記録）
