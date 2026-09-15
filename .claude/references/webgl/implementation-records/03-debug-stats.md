---
title: デバッグ統計（FPS / フレームタイムのオーバーレイと計測）
sources:
  - src/debug/stats.ts
updated: 2026-09-11
---

# デバッグ統計（FPS / フレームタイムのオーバーレイと計測）

## 役割
フレーム時間のサンプルを保持して FPS / 平均 / パーセンタイルを集計し、F3 で切り替える `<pre id="stats">` オーバーレイ用の文字列を作る。
ベンチマーク（`scripts/bench.mjs`）向けに任意区間の記録 (`startRecording` / `stopRecording`) も提供する。

## 公開インターフェース
- `class FrameStats(engine: AbstractEngine, scene: Scene)`
  - `push(ms: number)` : フレーム時間サンプルを追加。
  - `startRecording()` / `stopRecording(): ReturnType<typeof summarize>` : 記録区間の開始 / 終了と集計。
  - `get drawCalls: number`、`get gpuMs: number`（未計測なら `NaN`）、`get cpuMs: number`
  - `readonly adapter: string` : ブラウザが実際に使っている GPU（下記）。
  - `set gpuTiming(on: boolean)` : GPU タイマークエリの有効・無効。
  - `text(extra: string): string` : オーバーレイ 5 行テキスト。
- `summarize(ms: number[]): { frames, fps, avgMs, p50Ms, p95Ms, p99Ms, low1Fps }`

## 内部構造と処理の流れ
### 計測ソース
- コンストラクタで `SceneInstrumentation(scene)` を作り `captureFrameTime = true`、`EngineInstrumentation(engine)` を作る。GPU タイマークエリ（`captureGPUFrameTime`）は作った時点では動かさず、`gpuTiming = true` の間だけ動かす（毎フレーム query を 1 つ発行するため）。`Game.showStats(on)` がオーバーレイの表示と連動させるので、F3 を閉じている間は計測しない。
- `adapter` = `(engine as Engine | WebGPUEngine).getInfo()` から作る。WebGL2 は `renderer`（Babylon の `getGlInfo()` が `WEBGL_debug_renderer_info` の `UNMASKED_RENDERER_WEBGL` を読む。Windows の Chrome なら `ANGLE (NVIDIA, NVIDIA GeForce … Direct3D11 vs_5_0 ps_5_0, D3D11)` のように GPU と ANGLE のバックエンドがわかる）、WebGPU は `vendor / renderer / version`（アダプタの vendor / architecture / description）。ソフトウェア描画や意図しない GPU で動いていないかを確かめるためのもの。`extractDriverInfo()` は WebGPU で空文字を返すので使わない。
- `drawCalls` = `sceneInst.drawCallsCounter.current`。
- `cpuMs` = `sceneInst.frameTimeCounter.lastSecAverage`（直近 1 秒平均、ms）。
- `gpuMs` = `engineInst.gpuFrameTimeCounter.lastSecAverage`（ns）を `* 1e-6` で ms に変換。値が 0 以下なら `NaN`（GPU タイマークエリが使えない環境）。

### サンプリング
- `samples: number[]` はリングバッファ相当。`push(ms)` で末尾追加し、長さが **240** を超えたら先頭を `shift()`（60 fps で約 4 秒分）。
- 呼び出し元は `Game.frame`（`engine.runRenderLoop` 登録）で、毎フレーム `rawMs = engine.getDeltaTime()` をそのまま `stats.push(rawMs)` する（クランプや平滑化なし）。
- `recording` が非 null のときは同じ `ms` をそこにも push（上限なし）。

### 集計 `summarize(ms)`
- 空配列なら全て 0 を返す。
- `sorted` を昇順に作り、`avg = Σ / n`。分位 `q(p) = sorted[min(n-1, floor(p * n))]`。
- 戻り値: `frames = n`、`fps = 1000 / avg`、`avgMs = avg`、`p50Ms = q(0.5)`、`p95Ms = q(0.95)`、`p99Ms = q(0.99)`、`low1Fps = 1000 / q(0.99)`。
- 240 サンプル時の p99 は `sorted[237]`（上位 3 番目に遅いフレーム）。

### 記録
- `startRecording()` で `recording = []`。`stopRecording()` は `recording ?? []` を `summarize` して返し、`recording = null` に戻す。
- `Game.debugApi` が `window.__wasami.record()` / `window.__wasami.stopRecord()` として公開し、`scripts/bench.mjs` がシナリオ実行の前後で呼んで JSON を得る。

### オーバーレイ文字列 `text(extra)`
1. `summarize(this.samples)` と `engine.getRenderWidth()/getRenderHeight()`。
2. 5 行を `\n` で結合:
   - `${fps.toFixed(1)} fps  ${avgMs.toFixed(2)} ms  p99 ${p99Ms.toFixed(1)} ms`
   - `render ${w}x${h}  dpr ${devicePixelRatio.toFixed(2)}  draws ${drawCalls}`
   - `cpu ${cpuMs.toFixed(2)} ms  gpu ${gpuMs.toFixed(2)} ms`（`NaN` なら `gpu n/a`）
   - `adapter`
   - `extra`（呼び出し元が渡す任意行）
3. `Game.frame` は `statsEl.hidden` が false のときだけ `statsEl.textContent = stats.text(extra)` を毎フレーム実行する。`extra` は `` `${api}  fov ${fovDegrees.toFixed(1)}  dash ${dash.toFixed(2)}\n${post.debugInfo}\nlights ${activeLampIndices.join(',')}  chase ${state.chase}` ``。

### 表示切替（F3）
- 対象 DOM は `index.html` の `<pre id="stats" class="stats" hidden>`。`Game` のコンストラクタで `showStats(CONFIG.debug.stats)`（`statsEl.hidden = !on` と `stats.gpuTiming = on`）。
- `Game.update` で `input.wasPressed('F3')` のたびに `showStats(statsEl.hidden)` で反転（エッジ判定。`Input.keyDown` が `F3` を `preventDefault` するのでブラウザ側の F3 検索は抑止される）。
- F3 の案内はどこにも出さない(タイトルの OPTIONS を原作の設定画面にしたため)。

## 依存関係
- import: `@babylonjs/core` の `EngineInstrumentation`, `SceneInstrumentation`, `AbstractEngine`, `Scene`、型だけの `Engine`, `WebGPUEngine`（`getInfo()` のため）。自前モジュールへの依存なし（`CONFIG` も参照しない）。
- 利用側: `src/game/game.ts`（`new FrameStats(engine, scene)`、`push`、`text`、`startRecording` / `stopRecording`）。
- スクリプト: `scripts/bench.mjs`（`?debug.stats=true` + record/stopRecord）、`scripts/smoke.mjs` / `scripts/flow-check.mjs`（`debug.stats=true` で起動）、`scripts/shots.mjs`（撮影時に `#stats` を hidden にする）。

## 設定・調整値
- `CONFIG.debug.stats`（既定 `false`）: 起動時にオーバーレイを表示するか。URL `?debug.stats=true`（または `?debug.stats`）で上書き可。
- サンプル数 240 と `1e-6`、分位点 0.5 / 0.95 / 0.99 はコード定数で CONFIG には無い。
- `render.hardwareScaling` / `render.maxDevicePixelRatio` の結果は `render WxH` 行で確認できる。

## 既知の制約・注意点
- `fps` は直近 240 サンプルの平均フレーム時間の逆数であり、瞬間値ではない。起動直後はサンプルが少なく値が荒れる。
- `getDeltaTime()` をそのまま積むため、タブ非表示から復帰した直後の巨大なデルタが p99 / 記録に混入する。
- `gpuMs` は WebGL2 で `EXT_disjoint_timer_query_webgl2` が無い環境や、WebGPU の timestamp クエリが無効な環境で `n/a` になる。GPU タイマーは F3 を開いたときに始まるので、開いてから 1 秒ほどは `gpu n/a` のことがある（`lastSecAverage` がまだ無い）。
- `recording` は上限なしで増えるので、長時間 `startRecording` のまま放置するとメモリを消費する。`stopRecording` を呼ばずに `startRecording` を再度呼ぶと前の記録は破棄される。
- `text()` は表示中のみ毎フレーム呼ばれるが、`summarize` が毎回ソート（240 要素）を行う。コストは小さいが厳密なベンチ時はオーバーレイを消す方がよい（`shots.mjs` はそのため hidden にしている）。
- `drawCalls` は `SceneInstrumentation` の現フレーム値であり、シャドウマップやポストプロセスのパスも含む。
- `low1Fps` は "1% low" の近似（p99 フレーム時間の逆数）で、厳密な下位 1% 平均ではない。
- `text()` の各値は `toFixed` で丸めた表示用文字列で、`stopRecording()` が返す JSON の生値とは桁数が異なる。
- `push()` は `Game.frame` の先頭付近で `engine.getDeltaTime()` を積む。ポーズ中に早期 return されるかどうかは game.ts 側の実装に依存し、本記録では未確認。
- サンプル上限 240 は `shift()` による先頭削除なので、厳密には O(n) のコピーが毎フレーム走る（実害は無い規模）。
- WebGPU と WebGL2 で `gpuFrameTimeCounter` の粒度・遅延が異なるため、両 API の `gpu` 行を直接比較するときは注意。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: F3 の案内をタイトルの `#api-label` から OPTIONS の VIDEO タブ(`FPS OVERLAY` = `F3`)へ移した
- 2026-09-12: オーバーレイに GPU（`adapter`）と DPR を加え、GPU タイマークエリを F3 の表示中だけ動かすようにした（`gpuTiming`。以前は非表示でも毎フレーム動いていた）。Windows の Chrome で重いという報告の切り分け用
- 2026-09-12: タイトルの OPTIONS を原作の設定画面にしたので、VIDEO タブの F3 の案内が無くなった（10 記録）
