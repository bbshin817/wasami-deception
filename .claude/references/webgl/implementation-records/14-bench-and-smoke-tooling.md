---
title: ベンチマークとスモーク / フロー確認ツール（Playwright）
sources:
  - scripts/bench.mjs
  - scripts/smoke.mjs
  - scripts/flow-check.mjs
  - scripts/enemy-check.mjs
  - scripts/shots.mjs
  - scripts/launch-browser.mjs
updated: 2026-09-15
---

# ベンチマークとスモーク / フロー確認ツール（Playwright）

## 役割
`playwright-core` から実機の Chrome を起動し（`scripts/launch-browser.mjs` の `launchChrome()`。リポジトリ直下の `.is_headless` が `true` ならヘッドレス・ミュート、それ以外はウィンドウ表示・音あり）、ゲームが `window.__wasami` に公開するデバッグ API（`src/game/game.ts` の `debugApi`）を叩いて、FPS 計測（bench）、起動とシェーダーエラーの確認（smoke）、ステージの最初から脱出まで（地下鉄のホーム・ホールのトリガー・シャード全回収・障壁・配電盤・脱出の開始・ゴール）とセーブ復元の E2E 確認（flow-check）、敵の出番・追跡・捕獲・死亡画面・リスポーン・逃げ切り・グリッチ・ゲームオーバーの確認（enemy-check）、README 用スクリーンショット撮影（shots）を自動化する。すべて 1920×1080・DPR 1 のビューポートで動く。

## 公開インターフェース
| スクリプト | 起動 | 引数 |
| --- | --- | --- |
| `bench.mjs` | `npm run build && npm run bench` | `--only <scenario>[,<scenario>…]`（カンマ区切りで複数）, `--vsync`（垂直同期を切らない） |
| `smoke.mjs` | `node scripts/smoke.mjs [baseUrl] [--webgpu | --webgl2] [--query "a=b&c=d"] [--out dir] [--timeout ms]` | baseUrl 既定 `http://127.0.0.1:5173/`（vite dev）、out 既定 `smoke-out` |
| `flow-check.mjs` | `node scripts/flow-check.mjs [baseUrl] [--webgpu | --webgl2] [--out dir]` | baseUrl 既定 `http://127.0.0.1:5190/` |
| `enemy-check.mjs` | `node scripts/enemy-check.mjs [baseUrl] [--webgpu | --webgl2] [--out dir]` | baseUrl 既定 `http://127.0.0.1:5190/`、out 既定 `smoke-out` |
| `shots.mjs` | `node scripts/shots.mjs <baseUrl> <outDir> '<json>'` | 既定 `http://127.0.0.1:5190/`, `smoke-out/shots`, `{}` |

`launch-browser.mjs`（上の 4 本が共通で使う）:
- `isHeadless: boolean` — `.is_headless`（`scripts/` の 1 つ上 = リポジトリ直下）が存在し、中身を trim したものが `'true'` のとき true。モジュール読み込み時に 1 回だけ読む。
- `async launchChrome(args = [])` — `chromium.launch({ channel: 'chrome', headless: isHeadless, args })`。`isHeadless` なら `args` の末尾に `--mute-audio` を足し、さらに返す `Browser` の `newContext` を差し替えて、作られる全コンテキストに `addInitScript(disablePointerLock)` を入れる（`browser.newPage()` も内部で `newContext()` を通るので同じく効く）。`disablePointerLock` は `Element.prototype.requestPointerLock` を `Promise.resolve()` を返すだけに、`Document.prototype.exitPointerLock` を何もしない関数にする。運用ルールは `.claude/guides/verification-browser.md`。

利用するデバッグ API（`window.__wasami`）: `api`（'WebGPU'/'WebGL2'）、`state()`（collected/remaining/elapsed/allShards/ringPiece/phase/chase/caught/lives/player/yaw/fov/dash/lights/doors/usable）、`shards()`、`stage()`（ステージの場所: 開始地点・チェックポイント・トリガー・配電盤・障壁・壊せる物（`breakables`）・開始地点からホールのトリガーまでの経路 `route`〈`{ at }[]`〉など。デバッグフィールドでは null。04 記録）、`enemies()`（role/special/active/mode/position/spawn/distance など）、`enemyPose(i, x, z, yaw)`、`navPath(ax, az, bx, bz)` / `navInfo()`、`start()`、`collectAll()`、`teleport(x,y,z,yaw)`、`pose(x,y,z,yaw,pitch)`、`face(yaw, pitch)`（その場で向きだけ変える。`pose` はプレイヤーを動かして立たせる）、`key(code, down)`（`input.simulate`）、`look(dx, dy)`（`input.addMouseDelta`）、`record()` / `stopRecord()`（`FrameStats.startRecording/stopRecording`）、`renderSize()`、`perf()`（`draws` / `cpuMs`）。

## 内部構造と処理の流れ

### bench.mjs（1080p フレームタイム計測）
1. `dist/index.html` が無ければ「先に npm run build」と出して終了。`vite preview --port 4199 --strictPort` を子プロセスで起動し、`fetch` が ok になるまで 200 ms × 最大 50 回待つ。
2. `launchChrome(args)`。args は `--ignore-gpu-blocklist`, `--window-size=1920,1160`, `--window-position=0,0`、`--vsync` が無い場合に `--disable-gpu-vsync`, `--disable-frame-rate-limit`（60fps 上限を外して余力を測る）。`isHeadless` のときは「ヘッドレス・ミュートで計測する（FPS は README の表と直接比べない）」旨を `console.warn` する。
3. `SCENARIOS`（URL クエリで `CONFIG` を上書き）。ゲームの既定は WebGL2 なので、`webgpu-*` は定数 `WEBGPU`（`render.preferWebGPU=true`）を明示する:
   | name | params |
   | --- | --- |
   | `webgpu-full` | `render.preferWebGPU=true` |
   | `webgl2-full` | `render.preferWebGPU=false`（ゲームの既定と同じ） |
   | `webgpu-no-taa` | `render.preferWebGPU=true`, `post.taa.enabled=false` |
   | `webgpu-no-motionblur` | `render.preferWebGPU=true`, `post.motionBlur.enabled=false` |
   | `webgpu-no-shadows` | `render.preferWebGPU=true`, `lights.shadowCasters=0` |
   | `webgpu-scale-1.25` | `render.preferWebGPU=true`, `render.hardwareScaling=1.25`（描画 1536×864） |
   | `webgl2-no-ssr` / `webgpu-no-ssr` | `post.ssr.enabled=false`（原作の画面空間の反射を切る） |
   | `webgl2-no-ssao-dof` / `webgpu-no-ssao-dof` | `post.ssao.enabled=false`, `post.dof.enabled=false` |
   | `webgl2-no-captures` / `webgpu-no-captures` | `environment.captures.enabled=false`（反射を弱い HDRI に戻す。タイルは 55 のまま） |
4. シナリオごとに `newPage({ viewport: 1920×1080, deviceScaleFactor: 1 })`、コンソール / pageerror を収集、`?debug.autostart=true&debug.freshSave=true&debug.stats=true&debug.skipIntro=true&debug.traps=false&enemy.catchRadius=0&<params>` で開き（敵は動いて負荷に入るが、固定経路の途中で捕まらない。床の噴出と車は止める）、`window.__wasami.state` が出るまで最大 180 s 待つ（この時間が `loadSeconds`）。出なければ「読み込みが終わらなかった」とコンソールの最後の 15 行を出し、結果に `{ scenario, error: 'load timeout', logs }` を入れて次の場面へ進む。2.5 s 待つ。
5. 固定経路（`record()` 〜 `stopRecord()`）: ホテルの開始の部屋は狭い（約 5 × 6.5 m）ので、先に最長の直線の廊下（Babylon の z = 46.75。敵の格子で x = −37.4〜35.4 が通れる）の x = −30 へ `pose`（yaw π/2 で +x を向く）して 1 s 待ってから計測を始める。`KeyW` + `ShiftLeft` でダッシュしつつ `look(sin(i/5)×18, 0)` を 100 ms × 40 回（4 s、視点を左右に振る）→ `<name>-sprint.png` 撮影 → Shift 解除で歩行 2.5 s → `Mouse1` を 50 ms 押す（180° 振り向き）→ 0.6 s → 再ダッシュ 2.5 s → 停止 1.5 s → `<name>-idle.png`。合計およそ 11〜13 s。
6. 指標は `src/debug/stats.ts` の `summarize(ms[])`: 毎フレームの `engine.getDeltaTime()` を記録し、`frames`, `fps = 1000/avg`, `avgMs`, `p50Ms`, `p95Ms`, `p99Ms`（ソート後 `floor(p×n)` 番目）, `low1Fps = 1000 / p99Ms`。加えて `renderSize`, `loadSeconds`, `perf`（経路の最後に止まった時の `perf()`: 描画の呼び出し数 `draws` と直近 1 秒の CPU のフレーム時間 `cpuMs`。行の末尾にも出す）, `errors`（error/warn を含むログ先頭 12 件）。
7. 出力 `bench-results/results.json`: `{ date, vsync, userAgent, results: [{ scenario, api, renderSize, loadSeconds, frames, fps, avgMs, p50Ms, p95Ms, p99Ms, low1Fps, errors }] }`。`userAgent` は `'Chrome (playwright-core, headed)'`、ヘッドレス時は `'Chrome (playwright-core, headless)'`。PNG は `.gitignore`、`results.json` はコミット対象。README「FPS 計測（1080p）」の表はこの JSON から転記されている（原作の SSR・SSAO・被写界深度・反射キャプチャと深度の代理を入れた後に M4 Pro のヘッドレスで計測した値。ホテルへの置き換え直後は WebGPU 既定 144.0 fps・WebGL2 191.3 fps、以前の館では 289.0 / 317.6 fps だった）。経路の計測は回ごとに数 % ぶれるので、効果ごとの寄与は同じ位置で条件だけ変えて測る方が確か（09 記録の「性能」）。

### smoke.mjs（起動スモーク）
- 目的: WebGPU / WebGL2 それぞれで起動〜歩行までに runtime / shader エラーが出ないか、外部リクエストが無いかを確認する。
- `launchChrome(['--ignore-gpu-blocklist'])`（vsync 操作なし）。`console`, `pageerror`（stack 付き）, `requestfailed` を記録し、`request` イベントで hostname が `127.0.0.1`/`localhost` 以外かつ `blob:`/`data:` 以外の URL を `external` に集める。
- クエリ `debug.freshSave=true&debug.stats=true&debug.skipIntro=true`、`--webgpu` で `render.preferWebGPU=true`、`--webgl2` で `render.preferWebGPU=false`（どちらも無ければゲームの既定 = WebGL2）、`--query` で任意追加。
- 手順と撮影: `<tag>-0-loading.png`（読み込み直後）→ `__wasami.state` か `#fatal` の表示を最大 240 s（`--timeout`）待ち、`#fatal` が見えていればその文言で例外（`<tag>-fail.png` と `<tag>-log.txt` を残す）→ `<tag>-1-title.png` → `start()` → 2.5 s → `<tag>-2-start.png` → `KeyW` 1.2 s → `ShiftLeft` 0.7 s → `<tag>-3-sprint.png` → 解除 1.2 s → `state()` → `<tag>-4-stop.png`。
- `<tag>-log.txt` に全ログ + `--- external requests` + `--- state` を書き、stdout に state JSON と「N warnings/errors, M external requests」を出す。`tag` は `--webgpu` なら `webgpu`、それ以外は `webgl2`。出力先 `smoke-out/` は `.gitignore`。

### flow-check.mjs（ゲームフロー E2E）
- `node:assert/strict` で検証する。`launchChrome()`（追加の引数なし）。`pageerror` と `console.error` を `errors` に集める。撮影は `<out>/flow-<名前>.png`。座標は `stage()` の Babylon のワールド座標で、ゴールの箱は中心の、箱の下端から 1.05 m 上へ `teleport` して入る（ホールのトリガーへはホームから歩く）。
- 手順:
  1. `?debug.stats=true&enemy.enabled=false&debug.traps=false&debug.skipIntro=true[&render.preferWebGPU=true|false]`（敵と罠なし: シャードの上へ次々に移動するので途中で捕まったり、床の噴出や車の前に降りて死んだりしないように。ステージ OP なし: 13 s の間プレイヤーが止まるため。指定なしはゲームの既定 = WebGL2）で開き、`localStorage.removeItem(KEY)`（`KEY = 'wasami-deception.save.v4'`）→ reload → `start()`。`stage()` を控える。`checkpoint` 1 でホーム（`checkpoints['1']`）の 0.6 m 以内 → `0-platform`。
  2. ホームから歩いてホールへ: `stage().route`（12 記録の colliders.json の経路。2 点以上）の点を順に歩く（`walkTo`: `KeyW` と `ShiftLeft` を押したまま〈いつも走る〉50 ms ごとに `face` で次の点へ向き、0.35 m 以内で次の点へ。0.2 m 動かないまま 4 s で `1-stuck` を撮り、止まった位置を出して失敗）。区間ごとに、通る壊れていない壊せる物（駅の柵の区画と板張りの通り道〈ホームの低い戸口の `doorway`、出口の `wayOut`〉。`stage().breakables` の箱を水平 0.3 m 広げて区間と交わるもの、近い順）は 1.2 m 手前で止まり、1 クリックで壊す（`clickBreak`: 区画の 0.5 m 先への yaw で pitch 0・0.25・−0.25・0.5・−0.5 の順に `face` して 150 ms 待ち、`state().usable` がその名前になったら〈手のマーク。どの pitch でもならなければ失敗〉`Mouse0` を 60 ms タップし、300 ms 後に `broken` を確かめる。原作の板張りのバリケードと同じく 1 回で壊れる）。歩く間は、`state().usable` が立っているロックピックかダッシュの障壁（`stage().barriers` の `kind` 'pick' / 'dash'）の `source` になったら（道すがら視線に乗って手のマークが出た）、前のクリックから 500 ms 過ぎていれば（`lastClick`）`Mouse0` をタップしてそのアクター名を `opened` に足す（止まらずに歩き続ける。原作の板張りのバリケードと同じく 1 回で壊れる）。歩き終えたら `walked the route (<点> points) in <秒> s; <壊した壊せる物の名前と障壁のアクター名>` を出し、何か 1 つ以上壊した（`opened`）・ダッシュの障壁が壊れた、を確かめる。続けて `checkpoint` 2 を最大 3 s 待ち、`#saving` が出てセーブの `checkpoint` が 2。ホームでは `state().music` が null だったこと、トリガーの 0.6 s 後の曲（`music` が `cc2_carol` か、敵がもう見ていれば `cc2_dethsmass`）を最大 3 s 待つ → `1-hall`。
  3. 配電盤: `panelBox` の中心を、±x / ±z の 1 m 先（床は `panelBox` の下端 + 0.35）から見て（`lookAt`: 目の高さ 1.62 m からの pitch）`usable` が 'panel' になる最初の側を探し、`#interact` が出る → `2-panel-hand`。同じ側の 3 m からは 'panel' にならない。1 m から `click`（`Mouse0` を 0.08 s）→ 0.6 s 後に `checkpoint` 2 のまま・`panel` −1（全回収の前は拒否）。
  4. シャード: 1 個目は ±x / ±z に 1.5 m 離れた点のうち `navPath` が 1.7 m 未満の最初の点へ向いて `teleport`（足は y − 1.05）、0.5 s 後に未回収、`KeyW` で歩いて最大 4 s で回収 → `3-first-shard`。残りの 300 個はまだのものだけ、真上の足位置へ `teleport` して `collected` を最大 4 s 待つ。`remaining` 0、`allShards`、`state().barriers` にシャードの障壁（`stage().barriers` の `kind` 'shard'）がすべて入る、`chase` false（ステージにフレンジーは無い）、セーブの `collected` が全シャード、曲が消える（`music` null。`stop_music_maneqiuns`）のを最大 3 s 待つ。
  5. 配電盤をもう一度見て `usable` 'panel' → `click` → `panel` ≥ 0 を最大 2 s、`checkpoint` 4 かつ `panel` < 0（暗転が明けた）を最大 8 s 待つ。脱出の開始（`checkpoints['4']`）の 0.6 m 以内、脱出の開始の保持（レベル BP の DisableInput、0.1〜0.9 s）が明けて `controls` true かつ `stage().escape.t` > 0.9 を最大 4 s 待つ、`#saving`・セーブの `checkpoint` 4、脱出の曲（`music` が `cc2_horrors`。開始の 1.5 s 後）を最大 3 s 待つ → 0.9 s 後に `4-escape-start`。
  6. reload → `#btn-reset`（NEW GAME）で `#restart`、`#restart-no` で閉じてセーブが残る、もう一度 → `#restart-yes` → `#title` が隠れるまで待ち、全シャードが戻り `checkpoint` 1・ホーム・セーブ `null`。手順 5 のセーブを書き戻す。
  7. reload → `#btn-start` が `RESUME`、`start()` 前は未反映。`start()` → 1.5 s 後に `remaining` 0・`checkpoint` 4・脱出の開始・全シャードが `collected`・シャードの障壁が壊れたまま。ゴールの箱（`triggers.goal`）へ `teleport` → `#end` を最大 3 s → 1 s 後に `6-escaped`、セーブ `null`。NEXT の `inert` が外れるまで最大 12 s → `7-results`。6 行の見出し・値・ランクの文字と合計がデバッグ API の `results()` と一致すること（ステージには秘密が無いので FINAL RANK S は確かめない）。NEXT で再読込を待ち、`#btn-start` が `NEW GAME`。最後に `errors` が空。
- 成功時は `flow OK: {shards, panelSide, saved, restoredButton}` を出力（`saved.collected` は個数）。

### enemy-check.mjs（敵の E2E）
- `node:assert/strict` で検証する。`launchChrome()`、`pageerror` と `console.error` を `errors` に集める。撮影は `<out>/enemy-<tag>-<名前>.png`（`tag` は `webgpu` / `webgl2`）。
- 座標は Babylon のワールド座標。直線はホールのリスポーン（`stage().checkpoints['2']`）のまわりで敵の格子から探す（`straight(len)`: 0〜20 m 離れた点から 8 方向に、`navPath` の歩く距離が直線の長さ ± 0.3 m の向き）。短い直線 13 m（捕獲・グリッチ・ゲームオーバー）と長い直線 34 m（無ければ 26 m。逃げ切り）。`foes()` は `special` でない迷路の敵（`enemyPose` の添字と同じ）。直線に使わない敵 1〜9 は出現位置へ戻しておく。
- 手順:
  1. `?debug.stats=true&debug.freshSave=true&debug.skipIntro=true&debug.traps=false[&render.preferWebGPU=…]`（罠なし: 敵の代わりに床の噴出や車が殺さないように）で開き、セーブを消して `start(true)` → 1.5 s。敵が 10 体、ホームではどれも `active` でない。ホールのトリガーへ `teleport` → `checkpoint` 2、0.4 s 後もまだ出ていない、さらに 0.9 s 後（トリガーから約 1.3 s）に全員 `active`、`ambusher` が 3 体（3 体目ごと）、`wander` か `chase`。`spawn` を控える。
  2. 捕獲: 短い直線の始点にプレイヤー、12 m 先に敵 0（こちら向き）。0.9 s 後に `chase` → `1-chase.png` → `state().caught` を最大 8 s 待ち、敵 0 が `caught`、0.35 s 後に `2-caught.png`、セーブの `collected` が捕まった時点のものと一致、`lives` 2。
  3. 死亡画面: `#death` が出るまで最大 5 s、ドクロが 3、`state().audio.paused` が true。1.9 s 後にドクロ 2 → `3-death.png`。`#death` が消えるまで最大 8 s（リスポーン）、0.7 s 後に `caught` false、`lives` 2、プレイヤーがホール（チェックポイント 2）の 0.6 m 以内、全敵が出現位置の 2.5 m 以内。
  4. 逃げ切り: 長い直線の 8 m の所にプレイヤー、始点に敵 0（同じ向き）、`KeyW` + `ShiftLeft` で直線の終わりまで（(長さ − 10) ÷ 6.5 s）。0.6 s 後に敵が `chase`、最後に捕まっておらず、距離が 9 m より開いていること → `4-outrun.png`。
  5. グリッチ: 短い直線の始点にプレイヤー、9 m 先に敵 0（こちら向き）、0.5 s 後に `chase`。`KeyQ` → `wheel(-400)`（最大 10 m）→ `Mouse0` でテレポートし、60 ms ごとに 16 回 `mode` を記録。直線に沿って 5 m より先へ跳び、記録に `stagger` があり、捕まっていないこと → `5-glitch.png`。
  6. ゲームオーバー: 2 回、短い直線の始点にプレイヤー、1 m 先に敵 0 を置いて捕まるのを待つ（`lives` が 1、0）。ライフが残っているうちは死亡画面が消えるまで待つ。0 の画面でドクロが 0、`.death__menu` の `inert` が外れるまで最大 4 s 待ち、0.6 s 後に `.death__dead` の不透明度が 0.95 超 → `6-game-over.png`。警告のフラグ（`localStorage` の `wasami-deception.lastCheckpointWarning`）が無いことを確かめ、LAST CHECKPOINT をクリックすると 0.4 s 後に `#death-warning`（S ランクの警告）が出ること → `7-checkpoint-warning.png`。`#death-warning-no` で閉じて死亡画面のまま、もう一度 LAST CHECKPOINT でまた出ること、`#death-warning-yes` で死亡画面が消え、フラグが `'1'` になること。0.3 s 後に `lives` 3・`caught` false・`checkpoint` 2。
  7. `errors` が空。成功時は `enemy check OK: {spawns, caughtAt, outrunGap, modes}`。

### shots.mjs（スクリーンショット撮影）
- `launchChrome(['--ignore-gpu-blocklist'])`。
- JSON 仕様: `{ variants: [{ name, query }], poses: [{ name, at: [x, y, z, yawDeg, pitchDeg], hideHud, wait }], webgpu, webgl2 }`。`webgpu: true` で `render.preferWebGPU=true`、`webgl2: true` で `false`、どちらも無ければゲームの既定（WebGL2）。既定は variant `default`（query 空）× pose `start`（移動なし）。
- variant ごとに `?debug.freshSave=true&debug.skipIntro=true[&render.preferWebGPU=true|false][&query]` で開き `start()` → 1.5 s。pose ごとに `at` があれば `pose(x, y, z, yaw·π/180, pitch·π/180)`（Babylon ワールド座標の足位置）、`#hud` を `hideHud` に応じて非表示、`#stats` を常に非表示にし、`wait`（既定 1800 ms）後に `<out>/<variant>-<pose>.png` を保存。
- README の `docs/screenshots/hotel-{corridor,altar,lobby,frenzy}.png` は 2026-09-13 に同じ手順（`pose` で足位置と向きを置き、HUD を隠すものは隠す）で撮った。frenzy は `collectAll()` と `setChase(true)` の後に撮る（shots.mjs には前処理の欄が無いので、そのときは使い捨てのスクリプトで撮った）。

## 依存関係
- `playwright-core` 1.63.0（devDependency、ブラウザは同梱せず `channel: 'chrome'` でインストール済み Chrome を使う。起動は `launch-browser.mjs` に集約）、`vite preview`（bench のみ）。
- ゲーム側: `src/game/game.ts` の `debugApi`、`src/debug/stats.ts` の `FrameStats`/`summarize`、`src/config.ts` の `debug.*`（`stats`, `chase`, `autostart`, `freshSave`, `skipIntro`）と URL クエリ上書き、`index.html` の要素 id（`#fatal`, `#saving`, `#subtitles`, `#btn-start`, `#end`, `#clear-next`, `.clear__row` / `.clear__total-value` / `.clear__final-rank`, `#hud`, `#stats`）、セーブキー `wasami-deception.save.v4`（flow-check / enemy-check の `KEY`。`CONFIG.game.saveKey` と手で合わせる）。
- リポジトリ直下の `.is_headless`（ユーザーのローカル設定、未追跡）。

## 設定・調整値
- URL クエリで `CONFIG` の任意キーを上書きできる（`render.preferWebGPU`, `render.hardwareScaling`, `post.taa.enabled`, `post.motionBlur.enabled`, `lights.shadowCasters`, `debug.*` など）。bench の `SCENARIOS`、smoke の `--query`、shots の `variants[].query` がその入口。
- bench のポート 4199、待機時間（load 180 s、経路内の各 sleep）はスクリプト内定数。
- `.is_headless` の中身 `true` でヘッドレス・ミュート（`launch-browser.mjs`）。

## 既知の制約・注意点
- GPU が使える Chrome が前提（WebGPU と GPU 計測のため）。Chrome のヘッドレスモードでも GPU は使われ、macOS（Apple GPU）では WebGPU で描画できることを 2026-09-11 に確認した。GPU の無い CI 環境では動かない。
- ヘッドレスでは `requestAnimationFrame` がおよそ 60 回/秒に制限されることがある（2026-09-11 に確認）。bench の FPS はウィンドウ表示での計測（README の表）と直接比べない。
- bench は `--disable-gpu-vsync --disable-frame-rate-limit` で数百 fps を出すため、絶対値より構成間の差を見る用途。1% low は p99 フレーム時間の逆数で、下位 1% の平均ではない。
- bench・shots・flow-check の既定 URL は環境ごとに異なる（4199 preview / 5173 dev / 5190）。5190 は開発時に使っていたポートと思われ、`package.json` に対応するスクリプトはない。
- flow-check の手順 2 は colliders.json の `route` を歩くので、壊す柵と板張りの通り道・割る障壁は経路の点から決まる（経路が変われば手順も変わる。経路は立って歩けるマスだけを通る。12 記録）。以前はホールへ `teleport` していて、ホームから出られないこと・階段の段で止まること・シャードの障壁が道を塞ぐことを見逃していた。ロックピックとダッシュの障壁は、歩く向き（次の点への yaw・pitch 0）の視線に手のマークが乗ったときだけクリックするので、経路がその障壁を正面から通らなければ壊せずに当たって止まる（`1-stuck`）。
- flow-check の歩いて触れる確認は 1 個目（`Shard_001`）だけで、近づく向きは `navPath` で床が通れる側を選ぶ（1.5 m 先に別のシャードが無いことは前提）。
- bench の経路と enemy-check の直線は、ステージのホールのリスポーン（チェックポイント 2）のまわりで実行時に敵の格子から探す（`navPath` の歩く距離が直線の長さと合う向き）。見つからなければ bench はリスポーンの向きのまま走る（壁に当たっても計測は続く）。
- shots の `pose` は足位置 + yaw/pitch を直接設定するため、衝突や物理を経由しない（壁内に置ける）。
- smoke は起動とシェーダーの致命的エラーしか見ない。描画結果の妥当性は PNG を目視する運用。
- `isHeadless` はモジュール読み込み時に 1 回だけ評価するので、スクリプト実行中に `.is_headless` を書き換えても反映されない。
- **ポインターロックの無効化**：ゲームは `start()` で `canvas.requestPointerLock()` を呼ぶ。macOS の Chrome はヘッドレスでも実際のカーソルを捕捉・ワープさせるため、検証中にユーザーのカーソルが急に動いた（2026-09-11）。ヘッドレス時は `launchChrome()` がこれを無効化する。無効化してもゲームの `Input.requestLock()` は `false` を返すだけで一時停止にはならず、入力は `window.__wasami` の `key` / `look` で与えるので検証に支障はない。`launchChrome()` を通さずに作ったコンテキストには効かない。
- Chrome は `--enable-unsafe-webgpu` を付けずに起動する。付けると実験的な WebGPU の機能（例：`texture-compression-unaligned`）が出て、ゲームは `enableAllFeatures: true` なのでそれも有効にし、通常の Chrome では起きる不具合が隠れる（velvet の KTX2 が 4096×3941 だった黒フレームがヘッドレスで再現しなかった原因。記録 02・13）。フラグなしでもヘッドレスの WebGPU は動く（Apple metal-3）。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: flow-check.mjs のリロード後のボタン名の検証を `CONTINUE` → `RESUME` に変更(タイトル画面の作り直しに合わせた)
- 2026-09-11: flow-check.mjs のシャード回収を「1.1 m 手前で `#prompt` を確認して `KeyE`」から「1.5 m 手前で未回収を確認し、`KeyW` で歩いて触れて自動回収」に変更
- 2026-09-11: Chrome の起動を `launch-browser.mjs` の `launchChrome()` に集約し、`.is_headless` が `true` ならヘッドレス・ミュートで起動するようにした（bench の `userAgent` も headed / headless を書き分ける）
- 2026-09-11: ヘッドレス時は `launchChrome()` が全ページでポインターロックを無効化するようにした（macOS の Chrome がヘッドレスでも実カーソルを捕捉・ワープさせていた）
- 2026-09-11: ゲームの既定が WebGL2 になったため、bench の `webgpu-*` シナリオに `render.preferWebGPU=true` を明示し、smoke / flow-check に `--webgpu`、shots の仕様に `webgpu` を追加した（指定なしはゲームの既定 = WebGL2）
- 2026-09-11: bench / smoke / flow-check / shots の Chrome 起動引数から `--enable-unsafe-webgpu` を外した（通常の Chrome と同じ WebGPU の機能で検証するため）
- 2026-09-11: flow-check.mjs のセーブ復元の検証を `start()` の後へ移した（セーブの反映が RESUME の時点になったため）。`start()` 前は未反映であることと、復元後にシャードがすべて回収済みであることも確認する
- 2026-09-11: flow-check.mjs に RESTART? の検証を追加した（NO でセーブが残り、YES で暗転の後にリロードなしで最初から始まる）
- 2026-09-11: 館の迷路（シャード 100 個）に合わせ、flow-check.mjs の歩いて触れる確認を 1 個目だけにし、残りはシャードの上へ移動して回収するようにした。セーブキーを v2（`KEY`）にした
- 2026-09-11: 敵の追加に合わせ、enemy-check.mjs を追加した。flow-check.mjs は `enemy.enabled=false`、bench.mjs は `enemy.catchRadius=0` で開く
- 2026-09-14: ステージの罠（床の噴出と車）に合わせ、flow-check.mjs・enemy-check.mjs・bench.mjs を `debug.traps=false` で開くようにした
- 2026-09-12: テレポートの照準が原作どおり 7 m から始まるようになったので、enemy-check.mjs の手順 5 でホイールを 4 目盛り回して 10 m にしてから確定するようにした
- 2026-09-13: 制限時間の撤廃に合わせ、enemy-check.mjs の残り時間の確認をやめ（捕獲はセーブの `collected` を確かめる）、`state()` の `timeLeft` を `elapsed` にした
- 2026-09-13: enemy-check.mjs を死亡画面に合わせた: 捕獲でライフが 2、3.5 s 後の死亡画面でドクロが 3 → 2、リスポーンでライフ 2・開始地点（以前の終了画面と `#btn-again` の手順を置き換え）。最後に 2 回捕まってゲームオーバー（YOU ARE DEAD とボタン）→ LAST CHECKPOINT でライフ 3 の手順を足した
- 2026-09-13: ステージ OP（開始から 13 s プレイヤーが止まる）を飛ばすため、bench / smoke / flow-check / enemy-check / shots のクエリに `debug.skipIntro=true` を足した
- 2026-09-13: 舞台を原作の Hotel に置き換えたのに合わせた: flow-check.mjs を祭壇の拒否 → 289 個の回収（1 個目は `navPath` で近づく側を選ぶ）→ 欠片（セーブ）→ 9.6 s でエレベーターが開き切る → ロビー → RESTART? → RESUME でチェックポイント 2（ロビーのエレベーター）→ ポータルで脱出の流れにし、`gateOpen` を `allShards` / `ringPiece` / `phase` / `doors` に替えた。enemy-check.mjs は回廊（z = 46.75）で演じ、迷路のサル 3 体と特別な敵のプール 13 体を確かめ、リスポーンは `hotel().start`。bench.mjs の経路を回廊へ移した。セーブキーを v3 にした。README のスクリーンショットをホテルのもの（`hotel-*.png`）に撮り直した
- 2026-09-13: bench.mjs の `--only` にカンマ区切りの複数を渡せるようにし、原作の画面空間の効果と反射キャプチャの負荷を測る場面（`*-no-ssr`、`*-no-ssao-dof`、`*-no-captures`）を足した。結果に `perf`（止まった時の描画の呼び出し数と CPU の時間）を足し、読み込みが終わらない場面はログを出して次へ進むようにした
- 2026-09-14: flow-check.mjs の脱出の確認を、原作の `UMG_LevelClear` に倣った脱出の画面に合わせた（`6-escaped` を 1 s 後の赤い You Escaped! に、`7-results` を足してリザルトの行・合計・FINAL RANK をデバッグ API の `results()` と照合し、NEXT でタイトルへ戻ることまで確かめる）
- 2026-09-14: flow-check.mjs に隠し扉を歩いて開けて奥の秘密 2 個を取り、赤いシャードも取る手順（`2b-secret-room-<n>`・`2b-secret-taken`）を足し、脱出の画面の 6 行と FINAL RANK が S であることを確かめるようにした。enemy-check.mjs のゲームオーバーで、LAST CHECKPOINT の S ランクの警告（NO で閉じてまた聞く、YES で保存して戻る。`7-checkpoint-warning`）を確かめるようにした
- 2026-09-14: flow-check.mjs の全回収の確認を「`#saving` が出る」から「`#saving` も字幕も出ない」に替え（原作の全回収は画面に何も出さない）、欠片で `#saving` が出ることを確かめるようにした。要素 id の一覧から `#prompt` を外した
- 2026-09-14: flow-check.mjs の祭壇と隠し扉を、見て（`lookAt`）手のマークと `usable` を確かめ、左クリック（`click`）で使う手順にした。3 m からは使えないこと、欠片の後は祭壇に手が出ないことも確かめる
- 2026-09-14: shots.mjs の使い方の例の上書きを `post.ue.exposureScale` にした（`post.ue.exposure` がボリュームの手動露出に替わったため。09 記録）。flow-check.mjs と enemy-check.mjs は Hotel の流れのままで、ステージの流れへの書き直しは進捗記録の検証のステップで行う
- 2026-09-14: flow-check.mjs と enemy-check.mjs をステージ（Chaotic Customer 2 の Zone_1）の流れに書き直した: flow-check はホーム → ホールのトリガー（チェックポイント 2）→ 全回収前の配電盤の拒否 → 全シャードとシャードの障壁 → 配電盤と暗転（チェックポイント 4）→ RESTART? と RESUME → ゴール → 脱出の画面（results() と一致）。enemy-check は 10 体がホールのトリガーの 1 s 後に出ること、直線を敵の格子から探し、捕獲・ホールへのリスポーン・逃げ切り・グリッチ・ゲームオーバーを確かめる。セーブキーを v4 に（試しのベイクで両方とも OK）。bench.mjs の経路を Hotel の回廊からステージのホール（チェックポイント 2 のリスポーンから、敵の格子で最も長く真っすぐ歩ける向き）にした
- 2026-09-14: flow-check.mjs の手順 5 で、脱出の開始の保持（0.1〜0.9 s）が明けるのを待ってから `controls` を確かめるようにした。ステージの曲の確認（ホームは無音、ホールで Carol か Dethsmass、全回収で無音、脱出で Merry_horrors。デバッグ API の `state().music`。06 記録）を手順 2・4・5 に足した
- 2026-09-14: README のスクリーンショットをステージのもの（`docs/screenshots/stage-platform.png`・`stage-hall.png`・`stage-lockpick.png`・`stage-escape.png`）に撮り直し、`hotel-*.png` を消した（撮影は scratchpad のスクリプトで `pose` / `teleport` / `key` / `collectAll`。ロックピックと脱出は敵の出ていないチェックポイント 1 から。このファイルのコードは変更なし）
- 2026-09-15: flow-check.mjs の手順 2 を、ホールへの `teleport` から、ホームから `stage().route` を歩く（`face` で向きを取り、柵を殴って開け、ロックピックの障壁を F で割り、しゃがみ・ブーストとスライドでダッシュの障壁を割る）手順にした。enemy-check.mjs の `foes()` から予備の個体（`spare`）を外した
- 2026-09-15: 駅の柵を原作の板張りのバリケードのように 1 クリックで壊すようにしたのに合わせた: flow-check.mjs の手順 2 は柵の区画を殴って開ける `punchOpen`（タブレットを下ろして長押しを最大 8 回）から、手のマークがその名前に乗る pitch を探して 1 回クリックする `clickBreak` にし、`stage().fences` を `stage().breakables` にした（タブレットはもう下ろさない）。enemy-check.mjs の `foes()` は `special` でない敵だけにした（予備の個体は無くなった）
- 2026-09-15: ロックピックとダッシュの障壁を 1 クリックで壊すようにしたのに合わせた: flow-check.mjs の手順 2 から、ロックピックの障壁の 1 m 手前で止まって `KeyF` を連打する手順と、ダッシュの障壁の 3.5 m 手前のブースト（`KeyE`）を外し、歩く間に `state().usable` が立っているロックピックかダッシュの障壁のアクター名になったら 1 回クリックする（`lastClick` で 500 ms に 1 回）ようにした
- 2026-09-15: ファンゲームのしゃがみ・スライドをやめ、駅の出口の板と低い戸口の格子の壁を 1 クリックで壊すようにしたのに合わせた: flow-check.mjs の手順 2 から、しゃがみの区間の `KeyC` と立ち上がりの待ち（`lastTap`・`lowEnd`）、しゃがみの区間の 0.25 m、ホールで立っていることの確認を外し、いつも走り、経路上の板張りの通り道も柵の区画と同じく 1.2 m 手前で 1 クリックで壊す（`stage().route` は `{ at }`）
