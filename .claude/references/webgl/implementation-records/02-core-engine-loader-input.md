---
title: コア基盤（エンジン選択 / アセットストリーミング / 入力）
sources:
  - src/core/engine.ts
  - src/core/loader.ts
  - src/core/input.ts
updated: 2026-09-12
---

# コア基盤（エンジン選択 / アセットストリーミング / 入力）

## 役割
描画 API（既定 WebGL2。`render.preferWebGPU=true` なら WebGPU → 失敗時 WebGL2）の選択と解像度スケーリング、マニフェスト駆動のアセット先読み（バイト単位の進捗と Blob URL 供給）、
キーボード / マウス / Pointer Lock を 1 フレーム単位のエッジ・ホールド状態に正規化する入力層。いずれも `Game.create` から生成される。

## 公開インターフェース
- `src/core/engine.ts`
  - `type GraphicsApi = 'WebGPU' | 'WebGL2'`
  - `createEngine(canvas: HTMLCanvasElement): Promise<{ engine: AbstractEngine; api: GraphicsApi }>`
  - `setRenderScale(engine: AbstractEngine, scale = 1)` — 描画解像度（下記 4.）
- `src/core/loader.ts`
  - `interface ManifestEntry { url: string; bytes: number; kind: 'level'|'texture'|'lightmap'|'env'|'model'|'audio'|'data' }`
  - `interface AssetManifest { generated: string; totalBytes: number; files: ManifestEntry[] }`
  - `type ProgressFn = (loaded: number, total: number, current: string) => void`
  - `class AssetStore(base: string)` : `resolve(rel)`, `has(rel)`, `buffer(rel)`, `url(rel)`, `blobUrlFor(absUrl)`, `loadManifest(rel)`, `preload(entries, onProgress, concurrency = 6)`, `dispose()`
- `src/core/input.ts`
  - `class Input(canvas)` : `held: Set<string>`, `pressed: Set<string>`, `locked: boolean`, `onLockChange(locked)`, `requestLock(): Promise<boolean>`, `exitLock()`, `beginFrame(): {dx, dy}`, `isDown(...codes)`, `wasPressed(...codes)`, `simulate(code, down)`, `addMouseDelta(dx, dy)`

## 内部構造と処理の流れ

### エンジン選択（engine.ts `createEngine`）
1. `CONFIG.render.preferWebGPU && await WebGPUEngine.IsSupportedAsync` のとき（既定は `preferWebGPU=false` なので、通常はこの手順を飛ばして 3. の WebGL2 になる） `new WebGPUEngine(canvas, opts)` を試す。opts は `antialias: false`, `stencil: true`, `adaptToDeviceRatio: false`, `powerPreference: 'high-performance'`, `setMaximumLimits: true`, `enableAllFeatures: true`。`await engine.initAsync()` → `configure()` → `{ api: 'WebGPU' }`。
2. 生成 / 初期化で例外が出たら `console.warn('[engine] WebGPU init failed, falling back to WebGL2', err)` して WebGL2 へ。
3. WebGL2: `new Engine(canvas, false /*antialias*/, { stencil: true, powerPreference: 'high-performance', antialias: false, preserveDrawingBuffer: false }, false /*adaptToDeviceRatio*/)`。`engine.webGLVersion < 2` なら `Error('WebGL2 が利用できません。WebGPU か WebGL2 に対応したブラウザで開いてください。')` を throw（WebGL1 は非対応。MRT と float ターゲットが必要なため）。
4. `configure(engine)`: `dpr = min(window.devicePixelRatio || 1, CONFIG.render.maxDevicePixelRatio)`、`engine.setHardwareScalingLevel(CONFIG.render.hardwareScaling / dpr / scale)`（export の `setRenderScale(engine, scale = 1)`。`configure` は `scale` 1。`scale` は設定の RESOLUTION SCALE〈`game/settings.ts` の `renderScale`、0.1..1〉で、game.ts の `applySettings` が起動時と SAVE & EXIT で呼ぶ）。既定（1 / 1）では Retina でも CSS ピクセル = 描画ピクセル。`hardwareScaling=1.25` で 80% 描画のアップスケール、`maxDevicePixelRatio=2` で Retina ネイティブ。
- `render.msaaSamples` は engine.ts では使わず `src/render/postfx.ts` が参照する。

### アセットストリーミング（loader.ts `AssetStore`）
- 内部状態: `buffers: Map<絶対URL, ArrayBuffer>`、`blobs: Map<絶対URL, blobURL>`。キーはすべて `resolve()` 後の絶対 URL。
- `resolve(rel)` = `new URL(rel, base).href`。`Game.create` は `base = new URL('./', location.href).href` で生成（`base: './'` ビルドと整合）。
- `loadManifest(rel)`: `fetch(resolve(rel), { cache: 'no-cache' })`。`!res.ok` で `Error('manifest <rel>: HTTP <status>')`。`res.json()` をそのまま返す。
- **マニフェスト**: `public/assets/manifest.json`（`scripts/build-level.mjs` が生成）。`{ generated: ISO日時, totalBytes, files: [{url, bytes, kind}] }`。記録時点で 132 ファイル / 245,552,336 bytes（level 3（`level-0.bin`〜`level-2.bin`。Cloudflare Pages の 25 MiB 上限のため 16 MiB 以下に分割）、texture 21、lightmap 2、env 1、model 2（`assets/models/wasami_enemy.glb`、`wasami_mochi.glb`）、audio 103。`kind: 'data'` は型にあるが未使用）。`url` は public ルート相対（例 `assets/level/level-0.bin`、`assets/textures/parquet/basecolor.ktx2`）。
- `Game.create` は `files` から lightmap を 1 枚に絞る: `lights.mode === 'realtime'` なら `CONFIG.lightmap.indirect`、`'baked'` なら `CONFIG.lightmap.full` のみ残す。
- `preload(entries, onProgress, concurrency = 6)`:
  1. `total = Σ entries.bytes`（マニフェスト値。`Content-Length` は見ない）。
  2. `queue` を `bytes` 降順（大きい順）に並べ、`concurrency` 個のワーカー（`Promise.all`）が `queue.shift()` で取り合う。
  3. 各ファイルは `fetch(abs)` → `!res.ok || !res.body` で `Error('<url>: HTTP <status>')`。`res.body.getReader()` でチャンク読み込みし、チャンクごとに `loaded += byteLength` して `onProgress(min(loaded, total), total, e.url)`。
  4. 読み終わりに `loaded += e.bytes - got` でマニフェストとの差分を補正（dev サーバー圧縮などで実受信量が違ってもバーが 100% で終わる）。
  5. チャンクを 1 つの `Uint8Array` に結合して `buffers.set(abs, out.buffer)`、再度 `onProgress`。
- `blobUrlFor(absUrl)`: `#` と `?` 以降を落としてキー化。`buffers` にあれば `URL.createObjectURL(new Blob([buf]))` を遅延生成してキャッシュ、無ければ `undefined`。`src/world/level.ts` の glTF ローダー `preprocessUrlAsync: (url) => store.blobUrlFor(url) ?? url` から呼ばれる。
- `url(rel)`: Blob URL があればそれ、無ければネットワーク URL。level.ts が KTX2 ライトマップ（`forcedExtension: '.ktx2'`）と `HDRCubeTexture` に使う。
- `buffer(rel)`: 未先読みなら `Error('asset not preloaded: <rel>')`。`kind === 'audio'` のデコード（`audio.decode(名前, store.buffer(url))`）に使う。
- `dispose()`: 全 Blob URL を `revokeObjectURL` し両 Map をクリア。
- 進捗の表示側マッピングは `Game.create` が `0.05 + 0.83 * (loaded/total)`、ラベル `LOADING x.x / y.y MB — <ファイル名>`（`1048576` で MB 換算、`toFixed(1)`）。

### 入力（input.ts `Input`）
- 登録するリスナー: `window` `keydown` / `keyup` / `blur`(→ `clear`) / `mouseup` / `mousemove`、`canvas` `mousedown` / `contextmenu`(preventDefault) / `auxclick`(preventDefault、中クリックのオートスクロール抑止) / `wheel`(`passive: false` で preventDefault)、`document` `pointerlockchange`。
- **ホイール**: `onWheel` が `deltaY` を px にそろえて `wheelAcc` に足す（`deltaMode` 1 = 行は × `WHEEL_LINE` 33、2 = ページは × `WHEEL_PAGE` 800。Chrome のマウスは 1 目盛りおよそ 100 px）。`beginFrame()` で `wheel = wheelAcc`（そのフレームの量。負 = 奥へ回した）にして 0 に戻す。`clear()` でも 0 に戻す。使うのはテレポーテーションの距離調整（05 記録）だけ。
- **キー識別は `KeyboardEvent.code`（物理キー）**。`keyDown`: `['Space','Tab','KeyZ','KeyQ','KeyE','F3','F7']` は `preventDefault`。`!e.repeat` のときだけ `pressedQueue` に追加、常に `held` に追加。`keyUp`: `held` から削除。
- マウスボタンは `` `Mouse${e.button}` `` → `Mouse0` 左、`Mouse1` 中、`Mouse2` 右。`mouseDown`（canvas 上のみ）は button 1 を `preventDefault`、`pressedQueue` と `held` に追加。`mouseUp`（window）で `held` から削除（canvas 外で離しても解除される）。
- `mouseMove`: `locked` のときだけ `dx += movementX`, `dy += movementY` を累積。**感度・デッドゾーンは input.ts では一切適用しない**（`CONFIG.camera.mouseSensitivity = 0.0021` は `src/player/controller.ts` が設定の感度の倍率と Y 反転を掛けて `yaw += dx * 感度`、`pitch = clamp(pitch ± dy * 感度, -1.45, 1.45)` として適用。05 記録）。
- `beginFrame()`（毎フレーム先頭に `Game.frame` から 1 回）: `pressed = new Set(pressedQueue)` → queue クリア → `{dx, dy}` を返して累積値を 0 に戻す。したがって `pressed` はそのフレームだけ有効なエッジ、`held` は継続状態。
- `isDown(...codes)` = いずれかが `held`、`wasPressed(...codes)` = いずれかが `pressed`。
- `requestLock()`: すでに `pointerLockElement === canvas` なら true。まず `requestPointerLock({ unadjustedMovement: true })`（OS のマウス加速を無効化）を試し、失敗したら引数なしで再試行、それも失敗なら false。最終的に `pointerLockElement === canvas` を返す。`exitLock()` は `document.exitPointerLock()`。
- `lockChange`: `locked = pointerLockElement === canvas`。ロック解除時は `clear()`（`held` と dx/dy をリセット）後に `onLockChange(locked)`。`Game.wireEvents` は「解除 && playing && !paused && state.running」で `pause()` する。
- `simulate(code, down)` / `addMouseDelta(dx, dy)` / `simulateWheel(dy)` はベンチマーク・検証用フック。`window.__wasami.key(code, down)` / `window.__wasami.look(dx, dy)` / `window.__wasami.wheel(dy)` として公開され `scripts/bench.mjs` などが使う（`simulate` は未ホールド時のみエッジを立てる。`key('Mouse0', true)` で左クリックも作れる）。
- 利用側のキー割り当て（記録時点）:
  | コード | 用途（利用者） |
  |---|---|
  | `KeyW/KeyS/KeyA/KeyD`, `ArrowUp/Down/Left/Right` | 移動（controller.ts `isDown`） |
  | `ShiftLeft`, `ShiftRight` | ダッシュ（前進中のみ。設定の TOGGLE SPRINT では押すたびに切り替え〈`wasPressed`〉、それ以外は押している間〈`isDown`〉） |
  | `Mouse1` | 180° ターン（`wasPressed`） |
  | `KeyQ` | テレポーテーションの位置調整に入る / 調整中はキャンセル（teleport.ts） |
  | `Mouse0` | テレポーテーションの確定（位置調整中のみ） |
  | ホイール（`wheel`） | テレポーテーションの距離（位置調整中のみ） |
  | `KeyE` | スピードブースト（controller.ts） |
  | `Space`, `Tab` | タブレット出し入れ（game.ts） |
  | `KeyZ` | ミニマップ拡大切替 |
  | `F3` | FPS オーバーレイ切替 |
  | `F7` | チェイスフラグ切替 |
  | Esc | ブラウザの Pointer Lock 解除 → `lockChange` → ポーズ（Esc 自体は判定しない） |

## 依存関係
- engine.ts → `@babylonjs/core` の `Engine`, `WebGPUEngine`, `AbstractEngine`、`../config`。
- loader.ts → 外部依存なし（`fetch` / `ReadableStream` / `URL.createObjectURL`）。
- input.ts → 外部依存なし（DOM の Pointer Lock API）。
- 利用側: `src/game/game.ts`（`createEngine`、`new AssetStore`、`new Input(canvas)`）、`src/world/level.ts`（`AssetStore` の `blobUrlFor` / `has` / `url`）、`src/player/controller.ts`（`isDown` / `wasPressed`）、`src/main.ts`（`input.locked` / `requestLock`）。
- Babylon 側の主要 API: `WebGPUEngine.IsSupportedAsync`, `WebGPUEngine.initAsync`, `Engine.webGLVersion`, `AbstractEngine.setHardwareScalingLevel`。KTX2 デコーダは `Game.create` が `KhronosTextureContainer2.URLConfig` を `store.resolve('vendor/ktx2/…')` に差し替えてローカル配信（`scripts/vendor-babylon.mjs`）。

## 設定・調整値
- `render.preferWebGPU`（既定 false = WebGL2。`?render.preferWebGPU=true` で WebGPU を試す。bench の `webgpu-*` シナリオ、smoke / flow-check の `--webgpu`、shots の `webgpu` が使用）、`render.hardwareScaling`（bench の `webgpu-scale-1.25` シナリオ）、`render.maxDevicePixelRatio`。
- `lights.mode` と `lightmap.full` / `lightmap.indirect` が先読み対象のライトマップを決める。
- `environment.hdr` は `store.has()` で存在確認してから `HDRCubeTexture` を作る。
- `preload` の `concurrency` 既定 6 はコード定数で CONFIG には無い。
- `camera.mouseSensitivity` は controller 側の適用値（input.ts は生の `movementX/Y` を渡す）。

## 既知の制約・注意点
- 先読みバッファ（約 190 MB）は `buffers` に保持され続け、Blob 化するとさらに複製が増える。`dispose()` は定義のみで、ゲーム中に呼ぶ箇所は確認できなかった。
- `preload` は 1 ファイルでも失敗すると `Promise.all` が reject し、リトライしない。他ワーカーはそのまま走り続ける。
- 補正 `loaded += e.bytes - got` は実受信がマニフェストより多いと負になり、進捗バーがわずかに戻ることがある。
- `total` はマニフェスト値なので、マニフェスト更新漏れがあると 100% と実態がずれる。
- `blobUrlFor` は `#` / `?` を落とすので、glTF がクエリ付き URL を要求しても同一バッファに解決される。
- **既定を WebGL2 にしている理由**：通常の（ウィンドウ表示の）Chrome 152、M4 Pro の 120 Hz 画面で WebGPU を使うと、キャンバス全体（3D タブレットを含む）が真っ黒になるフレームが数フレームおきに挟まり、重い場面では数秒続いた（2026-09-11、ユーザーの動画で確認）。WebGL2 では起きない。
  - 原因（修正済み、記録 13）：velvet テクスチャの KTX2 が 4096×3941 だった。WebGPU は寸法が 4 の倍数でない ASTC を作れず、そのテクスチャを描くフレーム（`tabletProbe` や `gBuffer` のパス）の `RenderEncoder` がまるごと無効になって Submit が捨てられていた。
  - 既定を WebGPU に戻すかどうかは、通常の Chrome での確認待ち。
- `enableAllFeatures: true` なので、Babylon はアダプタの機能をすべて要求する。`--enable-unsafe-webgpu` 付きの Chrome では実験的な機能（例：`texture-compression-unaligned`）も有効になり、通常の Chrome と挙動が変わる。上の黒フレームがヘッドレスで再現しなかったのはこのため（当時は smoke / bench / flow-check / shots がこのフラグ付きで起動していた。いまは外している。記録 14）。
- WebGPU の失敗検出は例外ベース。`IsSupportedAsync` が true でも初期化で落ちる環境は warn の後 WebGL2 に落ちる。
- `pressedQueue` は `blur` / ロック解除の `clear()` ではクリアされない。直前のキー押下は次の `beginFrame` で 1 回だけ届く。
- `mousedown` は canvas 上のみ監視。HUD 等の DOM 要素の上でクリックしても `Mouse0` は立たない。
- `Mouse2`（右）は現状どのモジュールも参照していない。
- Chrome は Esc 直後 ~1 秒 `requestPointerLock` を拒否する。再取得は `main.ts` の canvas click に依存。
- `unadjustedMovement` 非対応ブラウザでは通常の `requestPointerLock()` にフォールバックするため OS 加速がかかる。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: `ManifestEntry.kind` に `'model'`（シャードの GLB）を追加
- 2026-09-11: シャードが触れて自動回収になったため、`KeyE` / `Mouse0` の割り当て（シャード回収）と `KeyE` の `preventDefault` を削除
- 2026-09-11: 描画 API の既定を WebGL2 に変更（`render.preferWebGPU=false`。WebGPU の黒フレームのため）
- 2026-09-11: マウスホイール（`wheel`、`simulateWheel`）を追加し、`KeyE` を `preventDefault` に加えた。キーの割り当てを原作どおり Q = テレポーテーション（左の枠）、E = スピードブースト（右の枠）、左クリック = テレポートの確定に変更
- 2026-09-11: WebGPU の黒フレームの原因（velvet の KTX2 が 4096×3941 で、WebGPU では ASTC を作れない）と、ヘッドレスで再現しなかった理由（`--enable-unsafe-webgpu` と `enableAllFeatures`）を追記。修正は記録 13。既定は WebGL2 のまま
- 2026-09-12: 解像度の設定を `setRenderScale(engine, scale)` として export した（設定の RESOLUTION SCALE。`configure` は scale 1 で呼ぶ）