---
title: ミニマップ（トップダウン一発キャプチャ → 輪郭画像 → タブレット画面へ描画）
sources:
  - src/hud/minimap.ts
  - src/hud/arrow-pointer.ts
  - tests/arrow-pointer.test.ts
updated: 2026-09-14
---

# ミニマップ

## 役割
レベルの静的メッシュをロード時に一度だけ正射影の真上カメラで `RenderTargetTexture` に描き、ピクセルを読み戻して Dark Deception 風の「黒地に灰色の輪郭線」画像に変換する。ゲーム中はその画像をプレイヤー中心に回転・拡縮してタブレット画面（`Tablet.drawScreen` が渡す矩形）へ描き、シャード / 出口のマーカーとプレイヤードットを重ねる。

## 公開インターフェース
- `interface MapMarker { x: number; z: number; color: string; size?: number; shape?: 'square' | 'circle' | 'triangle'; angle?: number }` — `triangle` は原作の `T_EnemyTriangle`（`M_Enemy` / `M_Bonus_Shard` の板）の形で、頂点を `angle`（Babylon の yaw: 0 = +Z、π/2 = +X）の向きへ。ワールド座標のマーカー。`size` は一辺の m（既定 `MARKER_SIZE` 1.5）。原作と同じくワールドの大きさなので、ズームアウトで地図と一緒に縮む。
- `interface MapArrow { x: number; z: number; yaw: number; size: number; color: string }` — 地図の矢印（`arrow-pointer.ts`、§7.5）。`x, z` は板のいる所（プレイヤーの足元）、`yaw` はアクターの yaw（0 = +Z）、`size` は板の一辺の m（`ARROW.plane` × scale）。
- `class Minimap`
  - `constructor(scene: Scene)`
  - `zoomIndex: number`（public、既定 0）
  - `toggleZoom()` — `zoomIndex = (zoomIndex + 1) % CONFIG.hud.minimap.viewWidth.length`。表示の幅は次の `draw` から即座に変わる（補間しない）。game.ts で Z キーに割当（タブレットを上げている時だけ。SE `ui_select` volume 0.5・再生速度 2）。
    - 根拠: 原作（pak_reference の `BP_DD_PlayerCharacter`、ubergraph @18388〜@18632）の Z は `isTabletUp?` のときだけ、`PlaySound2D(UI_Select_V3, 0.5, 4.0)` を鳴らし、FlipFlop で `SceneCaptureComponent2D.OrthoWidth` を 10000（`mapZoomedOut?` = true）と 4000 に交互に代入するだけで、補間は無い。ピッチ 4.0 は UE4 の上限 `MAX_PITCH` 2.0 で頭打ちになるので、再生速度は 2。
  - `async capture(meshes: AbstractMesh[], probes: {x,z}[], floor: number, cutHeight = 2.1, layer = 'main', below = 1.5): Promise<void>` — 一発キャプチャと輪郭化を、レイヤー `layer`（階）として持つ。`floor` は歩く床の高さ（ワールド y）、`below` はそれより下を切る深さ（下の階が階段の吹き抜けから映らないように）。game.ts はロード末尾で、ステージなら地上（`'surface'`: 地下鉄の上端 −9.5 m より上に届くタイル、床 0、probes はシャード）と地下鉄（`'metro'`: −9.5 m より下に届くタイル、床 −21.03 m、probes はホームの開始地点と 1 m 四方の 4 点）の 2 枚、デバッグフィールドは `'main'` の 1 枚を await する。
  - `layer: string`（public、既定 `'main'`）— 描く階。無い名前なら最初に撮った階。game.ts がステージで毎フレーム、足元が −9.5 m より下なら `'metro'`、それ以外 `'surface'` を入れる（ファンゲームは階段の TriggerBox_2 / TriggerBox_3 で地上の地図 `Plane_2` と地下の地図 `Plane2` を入れ替える。本作は高さで分ける）。`layerNames` は撮った階の名前。
  - `draw(ctx, rect, panel, unit, player:{x,z}, yaw, markers, arrow, time)` — `arrow` は地図の矢印（null なら描かない）。`rect` は切り抜く矩形（見えるマップ）、`panel` は原作のキャプチャが引き伸ばされるマップのパネル（縮尺と中心を決める。10 記録の `LAYOUT.panel`）。タブレットの `DynamicTexture` の 2D コンテキストに描画。`Tablet.drawScreen` から 30Hz で呼ばれる。パワーの使用中の赤は、タブレットごと postfx のシーンのティントで付く（09・10 記録）ので、ここでは描かない。

## 内部構造と処理の流れ

### 1. キャプチャ範囲と解像度（`capture` 前半）
- 全 `meshes` の `computeWorldMatrix(true)` → `getBoundingInfo().boundingBox.minimumWorld/maximumWorld` を `Vector3.Minimize/Maximize` で統合し、XZ 範囲に `margin = 2` m を足して `minX/maxX/minZ/maxZ` に保存。
- `size = CONFIG.hud.minimap.captureSize (2048)`。長辺を 2048 とし（館の迷路 約 79 × 71 m で約 26 px/m）、短辺はアスペクト比で丸める（`texW`, `texH`）。`pxPerMeter = texW / (maxX - minX)`。

### 2. 正射影カメラ
- `FreeCamera('minimapCam')` を範囲中心の上空 `y = max.y + 10` に置き、`mode = Camera.ORTHOGRAPHIC_CAMERA`。
- `orthoLeft/Right = ∓w/2`、`orthoTop/Bottom = ±d/2`（w = X 幅、d = Z 奥行）。`rotation = (π/2, 0, 0)`（真下向き）、`minZ 0.1`、`maxZ = max.y + 40`、`inputs.clear()`。
- レイヤーマスクは設定していない（既定 `0x0FFFFFFF`）。描画対象は `rt.renderList = meshes` で限定する。

### 3. RenderTargetTexture と床マスク描画
- `RenderTargetTexture('minimapRT', {texW, texH}, scene, false)`、`activeCamera = cam`、`renderList = meshes`、`clearColor = (0,0,0,1)`。
- `StandardMaterial('minimapMask')`（`disableLighting = true`、`emissiveColor 白`、`backFaceCulling = true`）を `rt.setMaterialForRendering(meshes, mat)` で全メッシュに適用 → ジオメトリのある所は白、無い所は黒。
- クリップ平面: `Plane(0, 1, 0, -(floor + cutHeight))`（既定 `cutHeight = 2.1` m。床から測る。全メッシュの最低点 `min.y` から測るとホテルでは −1.8 m になり、床より下で切れて地図が真っ黒になっていた: ロビーの壁 `Level_Static_01_primitive7` が y = −3.9 m まで下がっている）。`onBeforeRenderObservable` で `scene.clipPlane` に設定し、`onAfterRenderObservable` で元に戻す。天井（高さ 2.1m 以上）を切り落とし、床と壁の断面だけを映す。同じく `scene.clipPlane2` に `Plane(0, −1, 0, floor − below)`（床より `below` 1.5 m 下から下を切る。ステージの地上の地図に地下鉄が映らないように）。カメラの `maxZ` は範囲の高さ + 40 m。範囲・画像・`pxPerMeter` はレイヤーごとに持ち、`toImage` と `outline` はレイヤーを受け取るモジュールの関数。
- 一発レンダ: `rt.isReadyForRendering()` が false の間 50ms ごとに再試行し、準備でき次第 `rt.render()` を 1 回だけ実行（毎フレーム更新はしない）。
- `await rt.readPixels()` → `Uint8Array` に変換し、`rt` / `mat` / `cam` を `dispose()`。続けて `scene.prePassRenderer?.markAsDirty()` で PrePass を dirty にし直す（理由は「既知の制約・注意点」。WebGPU では PrePass を使わないので null で何もしない）。`pixels` が null なら画像無しのまま終了。

### 4. 行順の判定（WebGL2 / WebGPU 差の吸収）
- 読み戻しの行順は API によって上下が逆になるため、`probes`（プレイヤー開始位置とシャード位置 = 床上と分かっている点）で判定する。
- `floorAt(x, z, flip)`: `toImage(x,z)` で画素へ変換し、`flip` なら `row = texH - 1 - iy`。その画素の R 値 `> 60` なら 1。
- `score(flip) = Σ floorAt(probe, flip)`。`score(true) > score(false)` なら `flipY = true` として `outline()` に渡す。

### 5. 輪郭抽出（`outline(px, w, h, flipY)`）
- `stride = px.length / h`（バイト/行）。`mask[y*w + x] = (R > 60) ? 1 : 0` の二値マスクを作る（`flipY` なら `sy = h-1-y` の行を参照）。
- エッジ検出: 半径 `r = max(1, round(pxPerMeter / 19))`（撮影解像度によらず床の縁の両側に約 5 cm。現行 26 px/m で 1）の正方近傍 `(2r+1)²` を走査し、マスク値が自分と異なる画素が一つでもあればエッジ。
- 出力色（α 255）: エッジ `LINE = [205,190,194]`（原作の参考画像の地図の線に合わせたわずかに赤紫寄りの暖かい灰。タブレットのマップ枠 `FRAME = '#7d7476'` と同じ色味だが、縮小して描くと細い線が黒と混ざって暗くなるので枠より明るくしてある）、床内部 `FLOOR = [8,8,8]`、外側 `VOID = [0,0,0]`。
- `document.createElement('canvas')`（w×h）に `putImageData` し、`this.image` としてキャッシュ。以降はこの canvas を `drawImage` するだけで再計算しない。

### 6. 座標変換（`toImage(x, z)`）
- `[(x - minX) * pxPerMeter, (maxZ - z) * pxPerMeter]` — 画像上端が +Z、右が +X。マーカーもプレイヤーも同じ変換を使う。

### 7. 描画（`draw`）
- ズーム: `width = CONFIG.hud.minimap.viewWidth[zoomIndex]`（`[40, 100]` m。初期は `zoomIndex` 0 の 40）をそのまま使う。原作と同じく補間しない。
- 中心: `panel` の中心（原作のキャプチャはカプセルの真上にあり、プレイヤーのアイコン `Image_212` もパネルの中心）。
- `rect` で `clip()` し `#000` で塗る。
- 画像がある場合:
  - `translate(cx, cy)` → `scale(panel.w / width, panel.h / width)`（パネルの幅と高さがどちらも `width` m）→ `CONFIG.hud.minimap.rotateWithPlayer (true)` なら `rotate(-yaw)` → `scale(1 / pxPerMeter)` → `translate(-px, -py)`（プレイヤー画素位置を中心へ）。`imageSmoothingEnabled = true` で `drawImage(image, 0, 0)`。回してから引き伸ばすのは、原作の正方形のキャプチャ（プレイヤーと一緒に回る）が縦にわずかに長いパネル（626 × 640）へ引き伸ばされるのと同じ順。
  - 根拠: 原作（pak_reference の `BP_DD_PlayerCharacter`）の `SceneCaptureComponent2D` は `CollisionCylinder` の子で、`ProjectionType` 正射影、`OrthoWidth` 既定 4000 cm（Z で 10000）、`TextureTarget` `T_NewMap`（512 × 512）。`UMG_MiniMap` の `Image_80`（`M_NewMap` = `T_NewMap` をそのまま）が `UMG_Tablet` のマップのパネル `CanvasPanel_762` いっぱいに置かれるので、パネルの幅に 40 m / 100 m が映る。
  - マーカー: プレイヤーから X か Z が `width` m より離れたものは描かない（パネルの角までは約 0.72 幅。シャード 100 個の `shadowBlur` を毎回描かないため）。残りを同じ変換空間で `toImage(m.x, m.z)` の位置に一辺 `s = (m.size ?? MARKER_SIZE) * pxPerMeter`（`MARKER_SIZE` 1.5 m）の正方形（既定）、円（`shape: 'circle'`）、または三角（`shape: 'triangle'`: 中心から `angle` の向きへ 0.47 s に頂点、反対へ 0.46 s に幅 0.88 s の底辺。T_EnemyTriangle の α の外形。画像の y は −Z）を塗る。`shadowColor = m.color`、`shadowBlur = 10 * unit`（影のぼかしは変換に掛からない画面の px）。
    - 根拠: 原作の地図のマーカーはキャプチャに写るワールドの板で、シャードは BP_Shard の `Plane`（`/Engine/BasicShapes/Plane` 100 cm × `RelativeScale3D` 1.5、`M_Shard`、`Minimap Plane Height` 2000 cm 上）、ほかは `BP_MiniMapMarker` の同じ 1.5 倍の板（`M_RingAltar`）。ズームインでパネルの幅の 3.75 %、ズームアウトで 1.5 %。
  - game.ts が渡すマーカー: 未回収シャード `color '#d21ee6'`（マゼンタ）、祭壇の印 `color '#f0cc7a'`（淡い金。ホテルは原作の `BP_MiniMapMarker_308`（像の位置、`M_RingAltar`）どおり最初からロビーに移るまで。デバッグフィールドは欠片の後の出口）。どちらも size 既定（1.5 m）。色は原作の参考画像の実測。特殊シャード（08 記録）: 見えている赤いシャードは `#ff2a1a` の三角 1.5 m（頂点 −Z。原作の `M_Bonus_Shard` の板の向きは推定）、オーブは `#ff9a1a` の四角 1.5 m、赤いシャードを取ってからの 60 s は敵が `#ff2020` の三角 2.52 m（原作の `BP_Monkey` の `StaticMesh`: Plane × 2.5239、`M_Enemy`。頂点を敵の向きへ）。どれも原作のプレイヤーが地図のキャプチャの `ShowOnlyActors` に載せる板（特殊シャードは BeginPlay から、敵は `Add To Map` から `Remove From Map` まで）で、色は原作の基底マテリアルの定数が cook で消えているので推定（`CONFIG.game.special.map`）。
- 矢印（`arrow` が null でなければ、マーカーの後に同じ変換空間で）: `toImage(arrow.x, arrow.z)` を中心に、一辺 `s = arrow.size * pxPerMeter` の板の `ARROW.arc` の弧を `arrow.color` で塗る。半径 `inner`（0.355 s）〜`outer`（0.428 s）、キャンバスの角度 `arrow.yaw − π/2`（画像の y は −Z）を中心に ±`half`（30°）、両端は放射状。`shadowColor` はその色、`shadowBlur = 10 * unit`（マーカーと同じ）。原作の板はシャードの板と同じ 20 m 上にあるので前後は決まらないが、本作はマーカーの上に描く。
- プレイヤードット（クリップ解除後、常にパネルの中心に固定）: 半径 `pr = (6 + sin(time*6)*0.8) * unit` の `#ff3030` 円、`shadowColor '#ff2a2a'`、`shadowBlur 16*unit`。中心に `pr*0.4` の `#ffd0d0` ハイライト。向きは地図側の回転で表現するため、ドット自体に矢印は無い。

### 7.5 地図の矢印の規則（`arrow-pointer.ts`）
原作（pak_reference）の `BP_ArrowPointer` を移したもの。Babylon に依存しない純粋なモジュールで、`tests/arrow-pointer.test.ts`（7 件）が node --test で確かめる。
- 原作の形: プレイヤー（`BP_DD_PlayerCharacter`）の子アクター `BP_ArrowPointer`（`RelativeLocation` (0, 0, 2000)、`RelativeScale3D` (5, 5, 1)）。プレイヤーが BeginPlay で地図のキャプチャの `ShowOnlyActors` に載せる（ubergraph @19914〜）。部品は `Plane`（`/Engine/BasicShapes/Plane`、`RelativeRotation` yaw 90、`RelativeScale3D` 1.5、`CastShadow` false、`M_Arrow_Inst`）。`M_Arrow` は Masked（`OpacityMaskClipValue` 0.3333）で `T_Arrow`（1024²）を使う。`T_Arrow` の白い部分は、板の中心から一辺の 0.355〜0.428 の半径、±30° の弧（両端は放射状）。
- `srgbHex(r, g, b)` — linear 色を CSS の sRGB の 16 進にする（キャプチャのベースカラーが UI のガンマを通って見える、という推定）。
- `ARROW`（原作の値）: `turnEvery` 0.005、`findEvery` 0.1（BeginPlay の K2_SetTimer: Set Rotation と Smooth Rotation が 0.005 s、Find Object が 0.1 s のループ）、`turnSpeed` 20（RInterpTo）、`scaleSpeed` 5（FInterpTo）、`scale` {near 7, far 4, range 30, zoomedOut 10}（MapRangeClamped(水平距離, 0, 3000 cm, 7, 4) + `mapZoomedOut?` なら 10）、`startScale` 5、`below` 100、`reach` 999.99（FindClosestShard の Closest Distance の初期値 99999 cm）、`plane` 1.5、`arc` {inner 0.355, outer 0.428, half 30°}、`color` {shard `#a200ff`（`M_Arrow_Inst` の Color (0.361, 0, 1)）, altar `#ffeb00`（Change Color (1, 0.8294, 0)、01_Hotel @42616）, elevator `#ffffff`（(1, 1, 1)、@12132）, portal `#ff0023`（(1, 0, 0.0167)、@43212）}。
- `interface ArrowShard { base: {x, z}; collected }`（world/shards.ts の items がそのまま入る）、`type ArrowAim = { shards: true } | { shards: false; target: {x, z} | null; color }`（レベル BP が設定する `Shards?`・`Target`・`Change Color`）、`interface ArrowFrame { player; camera; yaw; zoomedOut; zone: ArrowShard[] | null }`。
- `class ArrowPointer`: 公開の `visible`（板の SetVisibility。初期 false）、`yaw`（アクターのワールド yaw。0 = +Z、π/2 = +X）、`scale`（初期 5）、`color`（初期は紫）。
  - `aim(a)` — `shards: true` なら `Shards?` を立てて色を紫に戻し、レベルが置いた点を外す（Find Object が選んだシャードは残す）。`false` ならシャードを外し、`target` と `color` を置く。ゲームが毎フレーム状態から呼ぶ。
  - `update(dt, f)` — まず前のフレームのプレイヤーに対する相対 yaw をプレイヤーの今の yaw に足す（子アクターは付いているので、次の Smooth Rotation まではプレイヤーと一緒に回る）。Find Object を 0.1 s ごとに（溜まった回数だけ）呼ぶ。0.005 s の期限がこのフレームで何回来たかを数え（UE4 のタイマーは、ループのタイマーを期限の回数だけ呼ぶ。60 fps で 3〜4 回）、1 回以上なら Set Rotation（カメラ → Target の yaw、`atan2(dx, dz)`）を 1 回、Smooth Rotation をその回数だけ呼ぶ。最後に相対 yaw を記憶する。
  - Smooth Rotation — yaw を最短の向きへ `min(1, dt × 20)`（dt はフレームの dt。UE の GetWorldDeltaSeconds）ずつ。Target が有効なら、scale を目標（上の MapRangeClamped）へ `min(1, dt × 5)` ずつ。
  - Find Object — `Shards?` なら: `zone` が null（どのゾーンにも重なっていない）なら非表示。ゾーンの未回収シャードが 100 以上なら非表示。1〜99 なら最寄り（水平距離が最小。等距離なら先の方）を Target にして表示。0 なら Target を変えずに表示（原作の `Array_Length <= 0 OR !IsValid` の枝）。`Shards?` でなければ、Target が有効なら表示・無ければ非表示。回収されたシャードは原作では壊されて IsValid が false になるので、次の Find Object までは向きを変えない（最後の Target Rotation へ回り続け、scale は止まる）。
- 距離: 原作の FindClosestShard は GetDistanceTo（3D）だが、本作のシャードは同じ高さ（1.1 m）で床は平らなので、水平距離で同じシャードが選ばれる。

### 8. 単位
- `unit` は `Tablet.drawScreen` の `u = W/300`（640px 幅の画面なら約 2.13）。グロー（`shadowBlur`）とプレイヤードットは設計 px で指定し `unit` 倍される（マーカーの大きさは m）。

### 9. ゲームループとの接続（時系列）
1. ロード中（`Game.create` 相当の初期化）: `new Minimap(scene)` → `new Tablet(scene, player.camera, minimap, reflected)`。タブレットはプリウォーム描画（4 方向 + ダッシュ FOV の `scene.render()`）の前に作られ、マテリアル / プローブをここでコンパイルする。
2. プリウォーム後: ステージなら `'surface'` と `'metro'` の 2 枚（上の `capture`）、デバッグフィールドなら `'main'` の 1 枚で地図画像を確定 → `tablet.setVisible(false)` → 進捗 `READY`。毎フレーム `tablet.minimap.layer` を足元の高さで選ぶ。地下鉄の階ではマーカーを出さない（シャードも行き先もすべて地上。04 記録）。
3. `start()`: `hud.setVisible(true)`、`tablet.setVisible(true)`。
4. 毎フレーム `update(dt)`（プレイ中・非ポーズ時）:
   - `Space/Tab` → `tablet.toggle()`、`KeyZ`（`tablet.lowered` でない時だけ）→ `tablet.minimap.toggleZoom()` と SE `ui_select`（0.5、再生速度 2）。
   - `shards.touching(player.feet)` に触れているシャードを `state.collect`（触れると自動回収）。
   - `tablet.setObjective(state.objective)`、`setRefill(...)`。
   - 未回収シャード（`#d21ee6`）と祭壇の印（`#f0cc7a`）から `markers` を作る。地図の矢印は `arrow.aim(arrowAim())` と `arrow.update(dt, { player: feet, camera, yaw, zoomedOut, zone })`（04 記録）。`tablet.update(dt, { player: feet, yaw, markers, arrow, time })`（`arrow` は表示中だけ）。
   - `Tablet.update` が 1/30 秒ごとに `drawScreen` → `minimap.draw(ctx, rect, panel, u, player, yaw, markers, arrow, time)`。
5. イベント: シャード回収 `tablet.setShards(remaining)`（原作の Count Shake: マップの閃きと数字の揺れ。10 記録）、開門 `hud.showBanner('THE GATES ARE OPEN')`、セーブ `hud.showSaving()`。

## 依存関係
- import: `../config`（`CONFIG.hud.minimap`）、`./arrow-pointer`（`ARROW.arc`）。`arrow-pointer.ts` は何も import しない。
- Babylon.js: `FreeCamera`, `Camera.ORTHOGRAPHIC_CAMERA`, `RenderTargetTexture`（`setMaterialForRendering`, `readPixels`, `onBefore/AfterRenderObservable`）, `StandardMaterial`, `Plane`（`scene.clipPlane`）, `Vector3`, `Color3`, `Color4`。
- 使う側: `src/hud/tablet.ts`（`Tablet` が `minimap.draw` を呼び、`MapMarker` / `MapArrow` 型を `TabletFrame.markers` / `arrow` に使用）、`src/game/game.ts`（生成、`capture`、`toggleZoom`、マーカー生成、`ArrowPointer` の更新と `MapArrow` の生成）。

## 設定・調整値
- `hud.minimap.captureSize`: 2048（RT の長辺 px）。
- `hud.minimap.viewWidth`: `[40, 100]`（Z キーで循環する、マップのパネルの幅に映す m。原作の `OrthoWidth` 4000 / 10000 cm）。
- `hud.minimap.rotateWithPlayer`: true（false なら北固定）。
- `hud.minimap.redrawHz`: 30（呼び出し側 `Tablet.update` の再描画周期。minimap.ts 自体は参照しない）。
- `capture` の `floor` 引数（階の床の y。ステージは地上 0・地下鉄 −21.03 m）と `cutHeight` 引数（既定 2.1 m）と `below`（既定 1.5 m）と `margin = 2`、二値化閾値 `60`、エッジ半径 `w/420`、輪郭色 `LINE` / `FLOOR` / `VOID` はコード内定数。
- URL クエリで `?hud.minimap.captureSize=2048` などの上書きが可能。

## 既知の制約・注意点
- キャプチャは一度きり。門（`gateMeshes`）は `capture` の `meshes` に含まれていない（`level.staticMeshes` のみ）ため、門の開閉は地図に反映されない。動的に変化する地形は非対応。
- `readPixels` の行順判定は probes の多数決に依存する。probes が全て境界付近にあると誤判定しうる（現状はプレイヤー開始 + シャード 100 点）。
- `rt.readPixels()` が null（失敗）を返した場合、`image` は null のまま黒い地図になり、エラーは出さない。
- レイヤーマスクを使っていないので、`renderList` に入れたメッシュはメインカメラ側の設定に関係なく描かれる。逆に `renderList` 外（照明のグロー等）は映らない。
- クリップ平面はシーングローバルな `scene.clipPlane` を一時的に差し替える方式。RT 描画中に他のレンダーが割り込む構造ではないが、`prevClip` の復元に依存している。
- エッジ検出は `(2r+1)²` 近傍の全走査で O(w·h·r²)。2048×N（r = 1）で数十 ms 程度、ロード時 1 回なので許容している。
- 線の太さは原作と違う。原作の地図はキャプチャに写る壁（`SCS_BaseColor`、表示リストだけ）で、2026-09-13 の収録では壁が約 6 px / パネル約 340 px（約 0.7 m）の帯に見える。本作は床の縁の両側 5 cm の輪郭線なので、同じ縮尺では細い（ズームアウトではさらに細く淡い）。
- `capture` は Babylon の `isReadyForRendering` を 50ms ポーリングで待つ。シェーダーコンパイルが遅い環境ではその分ロードが延びる。
- **PrePass の再構成（WebGL2）**：Babylon の PrePass は dirty になると、次の描画の直前（`_beforeDraw` / `_clear`）に `scene.activeCamera` のポストプロセスを見て作り直される。キャプチャ用マテリアルの生成などで dirty になった状態でこの RT を描くと、その時点の activeCamera はポストプロセスの無い `minimapCam` なので、PrePass が無効・`imageProcessingConfiguration.applyByPostProcess = false` で確定してしまう。以前はこのまま起動し、TAA 再投影とモーションブラーに速度が渡らず（強い残像）、トーンマップも二重に掛かり、最初のダッシュ（ポストプロセスの着脱で再び dirty になる）で約 0.3 s 止まって画面の明るさが半分に落ちていた。そのため `capture` の最後に dirty にし直し、次のプレイヤーカメラの描画で作り直させている。
- **地図の矢印の推定と原作との違い**: (1) 弧の向き。`/Engine/BasicShapes/Plane` の UV は書き出しに無いので、弧が Target の側に来る（BP がアクターを Target へ向け、板の yaw 90 がテクスチャの向きを合わせる、という作りの意図）として描く。(2) 色。原作は linear の Color をキャプチャのベースカラーとして描き UI に出すので、sRGB に直した値を使う（推定）。(3) エレベーターの Target。原作は `Elevator2light` だが、本作は灯を持たないので、0.24 m 離れたエレベーターのトリガー（`TriggerVolume5_3`）の中心を使う。(4) ポータルに変わる時刻。原作は `01_Hotel_Lobby_Transition1` の開始で、本作では乗車の暗転（`ride.fade`）に当てる。(5) 最初の 0.1 s。原作は最初の Find Object まで板が既定の表示のまま（向きは Target Rotation の既定 0）。ステージの導入で見えないので、本作は非表示から始める。(6) ゾーン。原作はプレイヤーが重なる `BP_ZoneShardChecker` の中のシャードを数える。ホテルでは 1 つの箱が迷路全体を覆い、シャードはすべてその中にあるので、本作は迷路にいる間だけ全シャードを数える。デバッグフィールド（原作に無い）は常に全シャード。
- **地図の矢印の数え方**: 原作は「ゾーン内のシャードが 100 未満」で出る。ホテルは 289 個なので、190 個取るまで出ない。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: フレーム更新の説明を、シャードの触れて自動回収（`shards.touching`）に合わせて更新
- 2026-09-11: キャプチャ後に PrePass を dirty にし直すようにした（WebGL2 で PrePass が無効のまま起動していた）
- 2026-09-11: 参考画像（原作のタブレット）に合わせ、輪郭を灰 190 から暖かい灰 `LINE [205,190,194]` にし、マーカーの既定サイズを 11→14 にした。game.ts のマーカー色をシャード `#b43cff`→`#d21ee6`（マゼンタ）、出口 `#ffd23a`（size 13）→`#f0cc7a`（既定サイズ）に変更
- 2026-09-11: `tablet.update` の引数から `lookDx` が無くなったのに合わせて記述を更新
- 2026-09-11: 館の迷路に合わせ、撮影を 2048 px、ズームを `[11, 24]` にし、輪郭の半径を `pxPerMeter` から決めるようにした。表示範囲の外のマーカーを描かないようにし、マーカーをズームアウトに合わせて縮めるようにした
- 2026-09-12: `draw` に `tint`（パワーの使用中に地図の線を赤くする量。'color' 合成）を追加した
- 2026-09-12: game.ts のタブレット更新の記述を `setPower` から `setRefill` に直した（10 記録）
- 2026-09-12: `draw` の `tint` を削除した（スピードブーストの間の地図の赤は、タブレットごと postfx のシーンのティントで付く。10 記録）
- 2026-09-13: Z のズームを原作どおり即座に切り替えるようにし（補間 `exp(-8dt)` と `draw` の `dt` を削除）、タブレットを上げている時だけ受け付けて `ui_select`（UI_Select_V3、0.5、再生速度 2）を鳴らすようにした（以前は `select` 0.4、いつでも）
- 2026-09-13: 地図の縮尺を原作のキャプチャに揃えた（ユーザーの指示）: `viewRadius` [11, 24]（矩形の短辺の半分の m）を `viewWidth` [40, 100]（原作のマップのパネルの幅の m）にし、`draw` に `panel` を足して中心をパネルの中心に（以前は矩形の高さの 0.55）、マーカーを設計 px の 14（ズームアウトで縮める）から原作と同じワールドの 1.5 m にした
- 2026-09-13: ホテルで地図の輪郭が出ていなかったのを直した（ユーザーの指摘）: クリップ平面を全メッシュの最低点 + 2.1 m から、新しい引数 `floor`（開始地点の足元の y）+ 2.1 m にした（ロビーの壁が y = −3.9 m まで下がっていて、床より下で切れていた）
- 2026-09-13: 特殊シャードのため、マーカーに三角（`shape: 'triangle'`、向き `angle`）を足した
- 2026-09-13: 原作の `BP_ArrowPointer` の規則（最寄りのシャード・目標を指す地図の矢印）を `arrow-pointer.ts` に足した（ユーザーの指示）
- 2026-09-13: `draw` に `arrow`（`MapArrow`）を足し、地図の矢印の弧をマーカーの上に描くようにした
- 2026-09-13: game.ts の祭壇の印を原作どおり最初から出し、行き先（エレベーター・ポータル）の印をやめた（04 記録）
- 2026-09-14: ダッシュのラジアルブラーを外したので、タブレットの矩形をマスクに渡す記述を消した（09 記録）
- 2026-09-14: ステージ（Chaotic Customer 2 の Zone_1）の地上と地下鉄の 2 つの階のため、地図を階ごとのレイヤーにした（`capture` の `layer` と `below`、`layer` / `layerNames`、下の階を切る `clipPlane2`、範囲と画像をレイヤーごとに。`toImage` / `outline` をモジュールの関数に）。game.ts が 2 枚を撮り、足元の高さで階を選ぶ
