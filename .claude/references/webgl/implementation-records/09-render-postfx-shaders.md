---
title: 描画：ポストプロセスとカスタムシェーダ
sources:
  - src/render/postfx.ts
  - src/render/shaders.ts
  - src/render/ue-grade.ts
  - src/render/scene-ao.ts
  - src/render/ssr.ts
  - src/render/depth-proxies.ts
  - src/render/stage-luts.ts
  - tests/ue-grade.test.ts
updated: 2026-09-15
---

# 描画：ポストプロセスとカスタムシェーダ

## 役割
`postfx.ts` の `PostFx` クラスがカメラに掛かるポストプロセス全体（TAA → カメラアニメ（HDR の露出とティント。テレポートとスピードブースト）→ UE 4.21 のガウスブルーム（縮小 6 段と段ごとの横・縦のぼかし）→ UE 4.21 のトーンマップ（ブルームの加算・露出・ビネット・原作のポストプロセスボリュームの色補正とフィルミックのカーブを焼いた LUT）→ DefaultRenderingPipeline（ガンマの符号化だけ）→ モーションブラー → スピードブースト）を構築し、毎フレームのパラメータを更新する。原作の指数高さフォグ（`ExponentialHeightFog_1`）も、Babylon の指数フォグの密度と色としてゾーンごとにここで入れる。
`ue-grade.ts` は UE 4.21 の色補正とトーンマップ（`PostProcessCombineLUTs.usf` の LUT の焼き込みと `ACES.ush` の `FilmToneMap`）の移植と、原作の Hotel のポストプロセスボリュームの値を持ち、ゾーンごとの 32³ の LUT を作る（Babylon に依存しないので `tests/ue-grade.test.ts` が node で検査する）。
`shaders.ts` は UE のトーンマップ・スピードブースト・テレポートのピクセルシェーダを GLSL（WebGL2）と WGSL（WebGPU）の両方で `ShaderStore` に登録し、WebGPU で GLSL→WGSL トランスパイラをダウンロードせずに済ませる。

## 公開インターフェース
### postfx.ts
- `class PostFx`
  - `constructor(scene, camera, quality = QUALITY[2])` — 全パスを生成。`quality`（設定の QUALITY。`game/settings.ts` の `QualityLevel`）が偽の効果は作らない: `motionBlur`（MotionBlurPostProcess を作らない）、`bloom`（UE のブルームのパスを作らず、トーンマップはブルームを足さない）、`ssao`（SSAO の 2 パス）、`dof`（被写界深度の 4 パス）、`ssr`（画面空間の反射。TAA が無くても作らない）。`ssao`・`dof`・`ssr` がすべて偽なら深度も描かない。QUALITY を変えるときは game.ts が `dispose()` して作り直す（実行中にパイプラインの効果を切り替えると Babylon がパスを付け直してチェーンの順が崩れるため）。
  - `readonly quality` — 作ったときの段（`QUALITY` の要素。同じ段なら同じオブジェクト）。
  - `zone: GradeZone`（getter / setter、既定 `'surface'`）— カメラがいるステージのポストプロセスボリューム（`'surface'` 地上・`'metro'` 地下鉄・`'field'` デバッグフィールド）。game.ts が毎フレーム入れる（04 記録）。変わると `gradeFor(zone)` を引き直し、トーンマップのパスがそのゾーンの LUT・露出（手動露出）・ビネットの強さを使う。
  - `brightness`（既定 1）— 最後のパスが掛ける明るさの指数（settings.ts の `brightnessPower` = 2.2 / gamma）。
  - `readonly pipeline: DefaultRenderingPipeline`、`readonly taa: TAARenderingPipeline | null`、`readonly motionBlur: MotionBlurPostProcess | null`。
  - `update(teleport: Readonly<TeleportFx> = NO_FX, boost: Readonly<BoostFx> = NO_BOOST)` — 追跡では画面を変えない（原作の追跡は曲だけ）。`teleport` はテレポートのカメラアニメ（05 記録の `teleport-fx.ts` の `TeleportFx`。ここでは露出 `ev` とティント `tint` を使う）、`boost` はスピードブーストの量（`tint` はカメラアニメのパス、ほかはブーストのパスで使う）。
  - `interface BoostFx { tint, lines, vignette, frame, blur, shake }`（export）— 原作のスピードブースト（05 記録の `boost-fx.ts`）: `tint` はカメラアニメのシーンのティント（効いていないとき `[1, 1, 1]`）、`lines` / `vignette` はウィジェット `UMG_SpeedBoost` の赤い集中線とビネットの不透明度、`frame` は集中線のフリップブックのコマ（0〜9）、`blur` は Chameleon のラジアルブラーの幅 0..1、`shake` は Chameleon の画面の揺れ（uv）。
  - `cut()` — カメラの瞬間移動（テレポートの移動の瞬間）。TAA の履歴を捨て、モーションブラーを 2 フレーム止め、画面空間の反射を 1 フレーム止める（履歴が前の視点のため）。
  - `get debugInfo(): string` — `taa:on grade:surface ao:on ssr:on dof:on depth:on mb:32s x1.2 boost:on/off tp:on/off` 形式（`ao` は SSAO の遮蔽、`ssr` は画面空間の反射、`depth` は深度の描画があるか）（boost と tp はそのフレームで効いているか = `fxOn`）。
  - `dispose()`。

### shaders.ts
- `UE_TONEMAP = 'wasamiUeTonemap'`、`BLOOM_DOWN = 'wasamiBloomDown'`、`BLOOM_BLUR = 'wasamiBloomBlur'`、`SSAO = 'wasamiSsao'`、`SSAO_BLEND = 'wasamiSsaoBlend'`、`DOF_SETUP = 'wasamiDofSetup'`、`DOF_MERGE = 'wasamiDofMerge'`、`TELEPORT = 'wasamiTeleport'`、`BOOST = 'wasamiBoost'` — シェーダ名（ブルームのパスの名前は `<シェーダ名><X|Y><段>` / `<シェーダ名><段>`、ほかは PostProcess 名と同じ）。
- `registerShaders()` — `ShaderStore.ShadersStore['<名前>PixelShader']` に GLSL、`ShaderStore.ShadersStoreWGSL` の同名キーに WGSL を 11 種とも代入する（被写界深度のぼかしは `BLOOM_BLUR` を使い、名前は `wasamiBloomBlurDofX` / `Y`）。

### ue-grade.ts
- `interface UeSettings` — 使う FPostProcessSettings を UE の名前で持つ: `WhiteTemp`・`WhiteTint`、`ColorSaturation` / `Contrast` / `Gamma` / `Gain` / `Offset` とそれぞれの `Shadows` / `Midtones` / `Highlights`（UE の FVector4 = rgb と倍率 w）、`ColorCorrectionShadowsMax`・`ColorCorrectionHighlightsMin`、`BlueCorrection`、`ExpandGamut`、`FilmSlope` / `FilmToe` / `FilmShoulder` / `FilmBlackClip` / `FilmWhiteClip`、`BloomIntensity`・`BloomThreshold`、`VignetteIntensity`、被写界深度の `DepthOfFieldFocalDistance` / `FocalRegion` / `NearTransitionRegion` / `FarTransitionRegion`（cm）/ `NearBlurSize` / `FarBlurSize`（画面幅の %）、SSAO の `AmbientOcclusionIntensity` / `Radius`（cm）/ `Power` / `Bias` / `FadeDistance` / `FadeRadius`（cm）/ `MipScale`、画面空間の反射の `ScreenSpaceReflectionIntensity`（%）/ `Quality`（0..100）/ `MaxRoughness`。
- `UE_DEFAULTS` — UE 4.21 の既定値（WhiteTemp 6500・Tint 0、色はすべて 1 でオフセット 0、ShadowsMax 0.09、HighlightsMin 0.5、BlueCorrection 0.6、ExpandGamut 1、FilmSlope 0.88・Toe 0.55・Shoulder 0.26・BlackClip 0・WhiteClip 0.04、BloomIntensity 0.675・Threshold −1、VignetteIntensity 0.4、DOF の FocalDistance 1000・FocalRegion 0・Near 300・Far 500・NearBlurSize 15・FarBlurSize 15、AO の Intensity 0.5・Radius 200・Power 2・Bias 3・FadeDistance 8000・FadeRadius 5000・MipScale 1.7、SSR の Intensity 100・Quality 50・MaxRoughness 0.6）。
- `UeSettings` には色・フィルム・ブルーム・DOF・SSAO・SSR のほか、`SceneColorTint`（CombineLUTs の ColorScale）、`ColorGradingLUT`（`StageLutName` か null）と `ColorGradingIntensity`、UE5 の手動露出の `CameraISO`・`CameraShutterSpeed`（UE の 1/s: 60 は 1/60 s）・`DepthOfFieldFstop`・`AutoExposureBias`、`MotionBlurAmount` がある。`UE_DEFAULTS` は UE 4.21 の既定（UE 5.4 もこれらは同じ）に UE5 の既定を足したもの: DOF の焦点距離 0（UE5 は焦点距離が無いと DOF を描かない）、ISO 100・1/60 s・f/4・露出補正 1、`MotionBlurAmount` 0.5、`ColorGradingIntensity` 1。
- `STAGE_VOLUMES` — ステージの `surface`（`PostProcessVolume3`: 地上全体・Priority 0）と `metro`（`PostProcessVolume4`: 地下鉄・Priority −1）の、`bOverride_*` が立っている設定だけ（override があって値が保存されていなければ UE の既定値。値は「設定・調整値」、出所は cc2_reference の umap を layout.py が読んだ `volumes`）。無限範囲の `PostProcessVolume_1` は重み 0 のマテリアルだけで何もしない。
- `type GradeZone = 'surface' | 'metro' | 'field'`、`GRADE_ZONES`、`gradeFor(zone)` = `{ ...UE_DEFAULTS, ...（metro なら metro、それ以外 surface のボリューム）}`（`'field'` はデバッグフィールドで surface と同じ色補正・霧なし）。
- `manualExposure(s)` — UE5 の手動露出（物理カメラ）: EV100 = log2(N² × CameraShutterSpeed × 100 / ISO)、露出 = 2^AutoExposureBias ÷ (1.2 × 2^EV100)。surface 3.70、metro 4.13。本作の画はベイクが UE の単位（白い面の輝度。1 m 先の 1 cd で白い板が 1/π に焼ける）なので、そのまま掛ける。
- `StageLutName`、`stageLut(name)`（`stage-luts.ts` の base64 を 16³ の RGB に 1 回だけ復号）、`sampleLut(data, c)`（UE の `UnwrappedTexture3DSample`: 256 × 16 のテクスチャを UVW = c × 15/16 + 0.5/16 で bilinear に読んで青のスライスを線形補間 = 格子 c × 15 の三重線形補間）。`gradeColor` はフィルムのカーブの後、sRGB の線形（負は 0）に `SceneColorTint` を掛けて sRGB に符号化し、`ColorGradingLUT` があればその表示の色を LUT で引いた色へ `ColorGradingIntensity` だけ寄せる（UE の LDR の LUT はスクリーンショットの上で作るもの）。`gradeContext` がその LUT を持つ。
- `stage-luts.ts`（生成物。`scripts/cc2/grading-luts.mjs`、12 記録）: `STAGE_LUT_SIZE` 16、`STAGE_LUTS`（`TX_LUT_02` = 地上、`LUT_U1_Filmic_Horror_Night` = 地下鉄。赤が最も速い 16³ の RGB を base64）。
- `whiteBalance(temp, tint)` — 線形 sRGB の 3×3（行優先）。4000 K 以上は昼光の色度（6500 が D65 になるよう Planck の法則の改訂を補正）、未満はプランク軌跡の色度に、等温線に沿った `tint` のずれを足した白を、Bradford の順応で D65 に移す。
- `gradeContext(s)`（入力行列 `SRGB_TO_AP1 · whiteBalance`、3 つの範囲の補正、フィルムの定数）、`gradeColor(ctx, linear)`（露出とビネットを掛けた後の線形の色 → 表示の sRGB）、`filmCurve(s)`、`linearToSrgb(x)`、`logToLin` / `linToLog`（UE の LinearRange 14・LinearGrey 0.18・ExposureGrey 444）、`LOG_BLACK` = LogToLin(0)、`LUT_SIZE` 32、`buildLut(s, size = 32)`、`vignetteMask(u, v, aspect, intensity)`、行列の `mul` / `apply`、`SRGB_TO_AP1` / `AP1_TO_SRGB`。
- 霧: `interface HeightFog { density, heightFalloff, height, inscattering }`、`STAGE_FOG`（`density` 0.3（UE は ÷ 1000 で cm あたり）、`heightFalloff` 0.2（UE の既定。÷ 1000）、`height` −273 cm（部品の UE z）、`inscattering` (0.01, 0.01, 0.01)（FogInscatteringLuminance）。方向の内散乱は平行光源が無いので効かず、体積フォグは描かない）、`STAGE_FLOOR_UE_Z` −335（Babylon の y = 0 = 迷路の床）、`ueZ(y)` = y × 100 − 335、`fogExtinction(fog, cameraZ)`（水平な視線の e 底の減衰 /m = density × 2^(−falloff × (z − height)) × ln²2 × 100。迷路の目の高さで 0.0142、ホームで 0.0190）。
- ブルーム: `UE_BLOOM`（`sizeScale` 4、`stages` = Bloom1〜6 の `{ size, tint, down }`: 0.3・0.3465・2、1・0.138・4、2・0.1176・8、10・0.066・16、30・0.066・32、64・0.061・64、`maxRadius` 31）、`BLOOM_TINT_SCALE` = 1/6、`gaussianRadius(sizePercent, viewWidth, down)`（UE のガウスの半径。`down` 分の 1 の画像の texel で、画面幅の `sizePercent` % を直径とみた半分。1e-4〜31）、`bloomRadius(size, viewWidth, down)` = `gaussianRadius(size × 4, …)`、`bloomAmount(luminance, threshold)`（しきい値のマスク。−1 以下は 1）、`gaussianTaps(radius)`（ぼかしのシェーダと同じ位置と重みの CPU 版。テスト用）。
- 被写界深度（UE 4.21 の Gaussian DOF）: `dofFarMask(depth, s)`（cm。遠焦点面 = FocalDistance + FocalRegion までは 0、そこから FarTransitionRegion で 1 へ直線。`DepthOfFieldCommon.ush` の `ComputeDOFFarFocalMask`）、`dofNearMask(depth, s)`（FocalDistance より手前で NearTransitionRegion にかけて 1 へ。ホテルでは最大 4 % 未満なので近景の層は作らない）、`dofFarRadius(s, viewWidth)` = `gaussianRadius(FarBlurSize, viewWidth, 2)`（半分の大きさの遠景の層の texel）。
- SSAO（UE 4.21 の `PostProcessAmbientOcclusion` に倣う）: `AO_SAMPLES`（6 方向。単位円の上を渦巻きに外へ。各方向を画素の反対側と対で取る）、`aoScreenRadius(s)` = Radius / 400 × MipScale / 4（UE の画面位置 −1..1 の単位で、投影の拡大率 1/tan(半画角) あたり。RadiusInWS が偽なので画面上の大きさが一定。既定で 0.2125）、`aoAdjust(ao, depth, s)` = 1 − (1 − ao^Power) × Intensity を FadeDistance − FadeRadius から FadeDistance にかけて 1 に戻す（cm）。
- 画面空間の反射（UE 4.21 の `ScreenSpaceReflections.cpp` / `.usf`）: `ssrQuality(quality, cvar = 3)`（ComputeSSRQuality: 40 未満 1、60 未満 2、80 未満 3、それ以上 4 を r.SSR.Quality〈既定 3〉で頭打ち。0 は off）、`SSR_LEVELS`（段ごとの歩数・本数・グロッシー: 1 = 8 歩 1 本、2 = 16 歩 1 本、3 = 8 歩 4 本グロッシー、4 = 12 歩 12 本グロッシー）、`ssrRoughnessScale(s)` = −2 / MaxRoughness と `ssrRoughnessFade(roughness, s)` = saturate(roughness × scale + 2)（GetRoughnessFade: MaxRoughness の半分まで 1、MaxRoughness で 0）、`ssrIntensity(s)` = saturate(Intensity × 0.01)、`ssrVignette(x, y)`（画面位置 −1..1 の当たりの端のフェード。ComputeHitVignetteFromScreenPos: 各軸 saturate(|p| × 5 − 4) の 2 乗和を 1 から引く）。

### scene-ao.ts
- `class SceneAo(scene, camera, ssao: boolean, shaderLanguage)` — シーンの深度（`scene.enableDepthRenderer(camera, false, false, NEAREST, storeCameraSpaceZ = true)`: カメラ空間の z m、何も無い所は 0。half float）と、UE の SSAO（`ssao` のとき）。`depth`（深度の RTT。被写界深度も読む）、`ao`（フィルタした遮蔽、半分の大きさ、BILINEAR。SSAO が無ければ null）、`settings`（ゾーンの `UeSettings`。PostFx がゾーンと一緒に入れる）、`dispose()`（`disableDepthRenderer` も）。`static texture(scene)` = 描いた後の `ao`、それまでと SSAO が無いときは 1×1 の白。
- `addSceneAo(material: PBRMaterial)` — ライトマップのある材質に遮蔽のプラグイン（`SceneAoPlugin`）を付ける（world/level.ts が `lit` の材質に付ける）。

### ssr.ts
- `class ScreenSpaceReflections(scene, camera, taa: TAARenderingPipeline, depth: RenderTargetTexture)` — UE の画面空間の反射。照らされた材質のプラグインが光線を辿る（下の「画面空間の反射」）。`settings`（ゾーンの `UeSettings`。PostFx がゾーンと一緒に入れる）、`reproject`（このフレームのビュー空間 → 前のフレームのクリップ空間）、`cut()`（次のフレームは反射を止める）、`dispose()`、`static bind(scene, uniformBuffer)`（プラグインが材質ごとに呼ぶ）。
- `addSsr(material: PBRMaterial)` — 照らされた材質に反射のプラグイン（`SsrPlugin`）を付ける（world/level.ts が `lit` のライトマップのある材質に付ける。`ScreenSpaceReflections` が無いあいだは辿らない）。

### depth-proxies.ts
- `class DepthProxies(scene, meshes, cell)` — レベルの静的な形を、深度だけが要るパス（scene-ao.ts の深度、WebGPU のモーションブラーの速度の GeometryBufferRenderer）のために位置だけのメッシュへ束ねたもの（下の「深度の代理」）。`proxies`（代理のメッシュ）、`static of(scene)`（登録された代理。デバッグフィールドでは null）、`attach(target: RenderTargetTexture)`（その RTT がシーンのアクティブなメッシュの代わりに、このフレームの一覧を描く）、`static detach(target)`、`dispose()`。

## 内部構造と処理の流れ
### パイプライン構築順（コンストラクタ）
0. `registerShaders()` を呼び、`engine.isWebGPU` で `ShaderLanguage.WGSL` / `GLSL` を決める。`prePassVelocity = !engine.isWebGPU`。
1. **TAARenderingPipeline('taa', scene, [camera], hdrType)**（`CONFIG.post.taa.enabled` のとき）：`hdrType` は `engine.getCaps().textureHalfFloatRender` なら `Constants.TEXTURETYPE_HALF_FLOAT`、無ければ `TEXTURETYPE_UNSIGNED_BYTE`。TAA は連鎖の先頭なのでシーンはこのテクスチャ（と TAA の履歴 ping-pong）に描かれる。`samples = 8`、`factor = 0.12`、`clampHistory = true`。`reprojectHistory = taa.reprojectHistory && prePassVelocity`（WebGL2 のみ true）、`disableOnCameraMove = !reproject`（WebGPU では静止時のみ蓄積）。
1.2. **深度と UE の SSAO と画面空間の反射**（`quality.ssao`・`quality.dof`・`quality.ssr` のどれかが真のとき `new SceneAo(scene, camera, ssaoOn, shaderLanguage)`。下の「SSAO」）。`post.ssr.enabled` かつ `quality.ssr` かつ TAA があれば `new ScreenSpaceReflections(scene, camera, taa, sceneAo.depth)`（下の「画面空間の反射」）。取れないうちの深度は 1×1 の黒の `RawTexture`（`blank`。深度 0 = ぼかし無し）。
2. **テレポート PostProcess**（常に生成）：`new PostProcess(TELEPORT, TELEPORT, { uniforms: ['fxOn','gain','grade','flash'], size 1.0, camera, BILINEAR_SAMPLINGMODE, engine, shaderLanguage, textureType: hdrType })`。パイプラインより先に作るので、カメラの連鎖では TAA の直後（TAA が無効なら先頭で、シーンがこのテクスチャに描かれる。そのため HDR の `hdrType`）に付き、ブルームとトーンマップの前の線形 HDR の画に掛かる。`onApply` で `fxOn` を送り、効いていれば `exposure = 2^min(ev, 60)`、`gain = exposure × max(0, tint × boost.tint)`（成分ごと。テレポートとスピードブーストのティントが重なれば掛け合わせる。UE では後のカメラアニメが重みで上書きする）を送る（+100 EV の白でも uniform が有限に収まり、シェーダの頭打ちで白になる）。さらに特殊シャードの取得の演出（公開フィールド `collect: Readonly<CollectPost>` = `{ grade, gain, flash }`。既定は全部 0。game.ts が毎フレーム `collectLook` を入れる。08 記録）から `grade = (gain.rgb, grade)` と `flash = (1 + (COLLECT_POST.midtones − 1) × flash, 0.01 × COLLECT_POST.fringe × flash)`（原作の `PostProcess1` の ColorGainMidtones 100 と SceneFringeIntensity 50 を重みで混ぜたもの）を送る。
2.2. **UE の Gaussian DOF**（`post.dof.enabled` かつ `quality.dof` かつ焦点距離のあるゾーンがあるとき。ステージのボリュームには無いので作らない。ブルームの前、UE と同じ）: `wasamiDofSetup`（`size` 1: シーンを持つ）が遠景の層を半分の大きさで横のぼかし `wasamiBloomBlurDofX`（`size` 0.5）へ描き、縦のぼかし `…DofY`（0.5）が `wasamiDofMerge`（0.5）へ描き、DofMerge が `sceneSampler` = DofSetup の入力（シーン）を画素ごとの遠景のマスクでぼかした層へ寄せてブルームの `Down0`（ブルームが無ければトーンマップ）へ描く。`far` = ((FocalDistance + FocalRegion) / 100, 100 / FarTransitionRegion)（m）。ぼかしは `radius = dofFarRadius(gradeFor(zone), 描画の幅)`、`taps = min(31, ⌈radius⌉)`、`tint` 1、`addOn` 0、`addSampler` = 自分の入力。
2.3. **UE のブルーム**（`post.bloom.enabled` かつ `quality.bloom`。18 パス、どれも HDR・BILINEAR）。Babylon のパスは自分のテクスチャ（大きさは自分の `size`）を前のパスの描き込み先にし、自分は次のパスのテクスチャへ描く。そのため画像は「それを入力として持つパス」で指し（`setTextureFromPostProcess`）、各パスの `size` はそこへ描かれる画像の大きさにする:
   - 縮小 6 回 `wasamiBloomDown0..5`：`Down0`（`size` 1）はシーン（テレポートのパスの出力）を持ち、それを半分に縮めてしきい値を掛けた縮小 0（1/2）を `Down1` へ描く。`Down n`（`size` 1/2^n）は縮小 n − 1 を持ち、縮小 n を次へ描く。縮小 5（1/64）は `BlurX5` が持つ。`onApply` で `texel` = 1 ÷ 自分の `width` / `height`（入力の 1 texel）、`setup` = `Down0` だけ 1、`threshold = gradeFor(zone).BloomThreshold`、`exposure` = ゾーンの露出。
   - 小さい段から `wasamiBloomBlurX5` → `Y5` → `X4` → … → `Y0`（どれも `size` = その段の縮小の大きさ 1/`stage.down`）：X は一つ前の段の合計（最初の段では縮小 5）を持つ。`externalTextureSamplerBinding = true` にして、`textureSampler` と `addSampler` にその段の縮小（`Down n + 1` が持つもの。段 5 は自分の入力）を束縛し、横にぼかして `tint` 1・`addOn` 0 で Y へ描く。`texel = (1 ÷ 縮小の幅, 0)`。Y は X の結果を持ち、縦にぼかして `tint = stage.tint × BLOOM_TINT_SCALE × post.bloom.scale` を掛け、`addSampler` = 同じ段の X が持つ一つ前の段の合計（`addOn` は最初の段で 0、ほかは 1）を足して、次の段の X（段 0 ではトーンマップ）へ描く（次の大きさで描くので、そのまま引き伸ばしになる）。`texel = (0, 1 ÷ 自分の高さ)`。両方とも `radius = bloomRadius(stage.size, 描画の幅, stage.down)`、`taps = min(31, ⌈radius⌉)`。
2.5. **UE のトーンマップ PostProcess**（常に生成）：`new PostProcess(UE_TONEMAP, UE_TONEMAP, { uniforms: ['exposure','vignette','aspect','exactSrgb','bloomIntensity'], samplers: ['lutSampler','bloomSampler'], size 1.0, camera, BILINEAR_SAMPLINGMODE, engine, shaderLanguage, textureType: hdrType })`。ブルームのパスの後、パイプラインの前に付く。自分の入力は合計のブルーム（`Y0` が全画面で描く）。`externalTextureSamplerBinding = true` にして、`onApply` で `textureSampler` = シーン（`Down0` の入力。ブルームが無ければ自分の入力がシーン）、`bloomSampler` = 自分の入力、`bloomIntensity = gradeFor(zone).BloomIntensity`（ブルームが無ければ 0）、`lutSampler` = 今のゾーンの LUT、`exposure = manualExposure(gradeFor(zone)) × post.ue.exposureScale`、`vignette = gradeFor(zone).VignetteIntensity`、`aspect = 描画の高さ ÷ 幅`、`exactSrgb = engine.useExactSrgbConversions ? 1 : 0` を送る。LUT はゾーンごとに最初に使うときに `RawTexture3D`（32³、RGBA8、ミップマップなし、BILINEAR、U/V/R とも CLAMP）で作る（`lut(zone)`、コンストラクタで今のゾーンの分を作る）。LUT のデータ `buildLut(gradeFor(zone))`（1 ゾーン約 30 ms）はモジュールの `LUTS` にページで 1 回だけ焼いて、QUALITY で作り直した PostFx とも共有する。
3. **DefaultRenderingPipeline('default', hdr=true)**：`samples = CONFIG.render.msaaSamples`（1）、`fxaaEnabled = !taa.enabled && post.fxaa`（現行は false）。ブルーム・色収差・グレインは切る（`*Enabled = false`。原作のボリュームはグレインも SceneFringe も効かせていない）。`imageProcessing` はトーンマップとビネットを切り、`exposure 1`・`contrast 1` にして、線形 → ガンマの符号化（`toGammaSpace` と 0..1 の切り詰め）だけをさせる。
4. **MotionBlurPostProcess('motionBlur')**（`motionBlur.enabled` かつ `quality.motionBlur` かつ `MotionBlurAmount` が 0 より大きいゾーンがあるとき。QUALITY の MEDIUM 以下では作らない。量はステージのボリュームから取らず Dark Deception のプレイヤーの UE の既定 0.5 なので、どのゾーンでも効く。下の「モーションブラーの量」）：`new MotionBlurPostProcess(name, scene, 1.0, camera, undefined, engine, reusable=false, undefined, blockCompilation=false, forceGeometryBuffer = !prePassVelocity)`。`isObjectBased = true`、`motionStrength = 1.2`、`motionBlurSamples = 32`。WebGPU では `forceGeometryBuffer=true` で `GeometryBufferRenderer` から速度を取り（描画パスが 1 つ増える。その G バッファにも深度の代理を `attach` する）、WebGL2 では PrePass の速度テクスチャを使う。
5. （以前のダッシュ中のラジアルブラーと、追跡中の赤いティントのパスは、原作データ〈`BP_DD_PlayerCharacter`・`BP_Monkey`・レベル BP〉に無いので 2026-09-14 に外した。画面の縁のラジアルブラーは 7 のスピードブーストの Chameleon のもの）
7. **スピードブースト PostProcess**（常に生成）：`uniforms: ['fxOn','lines','vignette','frame','blur','samples','shake','scales','grid','color']`、`samplers: ['speedlinesSampler','vignetteSampler','maskSampler']`。原作の素材 `./fx/speedlines.webp`・`vignette.webp`・`radial-mask.webp`（13 記録。`Texture`、ミップマップなし、BILINEAR、CLAMP。`overlays` に持ち `dispose` で破棄）を `onApply` で毎フレーム束縛する（素通しのときも。WebGPU は宣言したテクスチャをすべて要る）。効いていれば `lines`・`vignette`・`frame`・`shake` はそのまま、`blur = CHAMELEON.blurReach (0.05) × blur`、`samples = CHAMELEON.samples (8)`、`scales = (WIDGET.linesScale 1.25, WIDGET.vignetteScale 1.2)`、`grid = (SPEEDLINES.columns 2, rows 5)`、`color = (1, 0, 0)`（ウィジェットの色）を送る。
7.5. **霧**：コンストラクタの最後に `scene.fogMode = Scene.FOGMODE_EXP`、`fogDensity = 0`（全マテリアルの FOG 付きのシェーダをプリウォームでコンパイルさせ、読み込み中のミニマップの俯瞰の撮影には霧を掛けない）。`update` がゾーンごとに入れる（下）。
8. 1.5〜2.5 と 7 の自前のパスは生成時の `camera` 指定で、この順（TAA → テレポート → DOF 4 パス → ブルーム 18 パス → UE のトーンマップ → パイプライン → モーションブラー → ブースト）にカメラへ付き、以後は外さない。各 `onApply` は最初に `fxOn`（そのフレームで効いているか。下の「常に付けたままにする」）を送り、0 ならほかの uniform を送らずに戻る。

### SSAO（scene-ao.ts）
UE では SSAO が静的な光（ライトマップ。ボリュームの AmbientOcclusionStaticFraction は既定の 1）と反射にだけ効き、Stationary の灯の直接光と発光には効かない。そこで画面のパスで画に掛けず、シーンを描く前に遮蔽を作って、照らされた材質がライトマップと反射から引く。
1. **深度**: `DepthRenderer` の RTT がシーンの描画の前（カメラの描画対象の処理）に描かれる（PrePass の深度はシーンと同じ描画で書かれるので、その描画の材質には間に合わない）。レベルの静的な形は深度の代理（下の「深度の代理」）で描くので、描画の呼び出しは 1 割ほどしか増えない（代理が無いと倍になる）。
2. **遮蔽**: `scene.onAfterRenderTargetsRenderObservable`（描画対象の後、シーンの前）で、アクティブなカメラがそのカメラなら、`EffectRenderer` で `wasamiSsao` を半分の大きさの `sceneAoRaw`（NEAREST、half float、深度バッファなし）へ、`wasamiSsaoBlend` をフィルタして `sceneAo`（BILINEAR）へ描く（大きさは毎回描画の大きさの半分に合わせ、違えば `resize`）。uniform は `proj` = 投影行列の (m[0], m[5])、`outSize`、`depthTexel`、`radius = aoScreenRadius(settings)`、`adjust = (Power, Intensity, Bias / 1000, 0)`、`fade`、フィルタは `aoTexel`。
3. **材質**（`SceneAoPlugin`、`MaterialPluginBase`、優先 300 = 反射キャプチャのライトマップ混合〈200〉と画面空間の反射〈250〉の後。UE のスペキュラの遮蔽は両方の反射に掛かる。define `UE_SCENE_AO`、sampler `sceneAoSampler`（宣言は GLSL・WGSL とも `CUSTOM_FRAGMENT_DEFINITIONS`。「既知の制約」の UBO の項）、ubo `sceneAoTexel` = 1 ÷ 描画の大きさ。`doNotSerialize`: 材質を `clone` しても写しには付かない。Babylon は直列化したプラグインを登録されたクラス名で作り直そうとし、登録していないので例外になる）: `CUSTOM_FRAGMENT_BEFORE_FINALCOLORCOMPOSITION` で、画面の位置（GLSL `gl_FragCoord.xy`、WGSL `fragmentInputs.position.xy`）× `sceneAoTexel` で遮蔽 `sceneAo` を読み、`LIGHTMAP` のとき `finalDiffuse *= max(diffuseBase − lightmapColor × (1 − sceneAo), 0) / max(diffuseBase, 1e-6)`（ライトマップはレベルの影の無いキャリアの灯が 1 回だけ `diffuseBase` に足し、`finalDiffuse` は `diffuseBase` × アルベドなので、ライトマップの分だけが暗くなる）、`REFLECTION` のとき `finalRadianceScaled *= sceneAo`。反射のライトマップ混合（07 記録の `CaptureMixing`）は遮蔽前の `lightmapColor` を使うので二重に掛からない。描く前（読み込み中のミニマップの撮影など）は白を読む。タブレットの反射プローブの描画もこの画面の遮蔽を読む（小さく暗いので気にしない）。

### 画面空間の反射（ssr.ts）
原作の 3 つのボリュームは SSR の Intensity・Quality・MaxRoughness を override して値は既定のまま（100・50・0.6）。UE 4.21 では Quality 50 が SSR_QUALITY 2（r.SSR.Quality の既定 3 との min）で、画素ごとに鏡面方向の光線 1 本を 16 歩で辿る（グロッシーの散らしなし）。UE は GBuffer の法線と粗さで画面のパスとして辿り、反射の合成（`ReflectionEnvironment`）で SSR + (1 − SSR のアルファ) × 反射キャプチャ（ライトマップ混合込み）を環境 BRDF とスペキュラの遮蔽に通す。本作は前方描画で GBuffer が無いので、照らされた材質のプラグインが自分の法線（法線マップ込み）と粗さで光線を辿り、同じ形で合成する。
1. **準備**（`scene.onBeforeRenderTargetsRenderObservable`。このフレームの行列が決まった後、描画対象とシーンの前。アクティブなカメラがそのカメラのときだけ）: TAA の `_pingpong` が真なら `_pong`、偽なら `_ping`（TAA は自分が描く時に `_pingpong` を反転するので、それまでは最後に描いた方 = このフレームの履歴）の内部テクスチャを借りた `BaseTexture` に入れる（前のフレームのシーンの色: 線形 HDR、ブルームとトーンマップの前。UE も TAA の履歴を読む）。`reproject` = このフレームのビュー行列の逆 × 前のフレームのビュー × 射影（前の準備で覚えたもの）。2 フレーム目から有効。`cut()` で次の準備まで無効。
2. **材質**（`SsrPlugin`、優先 250、define `UE_SSR`、samplers `ssrDepthSampler`（scene-ao.ts の深度）と `ssrColorSampler`（履歴。宣言は `CUSTOM_FRAGMENT_DEFINITIONS`）、ubo `ssrTexel`、`ssrParams` = (Intensity × 0.01、−2 / MaxRoughness、フレーム番号 mod 8、有効なら 1 + `post.ssr.debug`)、`ssrReproject`、`ssrProjection`（シーンの射影行列。WebGL2 の PBR シェーダは射影行列を宣言しない）。`doNotSerialize`。無効のあいだは黒の 1×1 を束縛する）: `CUSTOM_FRAGMENT_BEFORE_FINALCOLORCOMPOSITION`（`REFLECTION` のとき）で:
   - 粗さのフェード `min(roughness × ssrParams.y + 2, 1)` が 0 以下、または自分が深度に描かれた面でない（`|深度 − 自分のビューの z| ≥ 0.02 z + 0.05`。隠れた面の分を辿らない）なら辿らない。
   - 光線: 反射方向 `reflect(−viewDirectionW, normalW)` をビュー空間へ。UE の RayCast と同じく、終点を画素の深度だけ先に取り（カメラへ向かうときは手前 0.1 z で止める）、画面位置と 1 / 深度（光線に沿って画面上で線形）で始点と歩みを作る。歩みは画面の長さ 1 に揃えてから画面の端で終わる倍率（GetStepScreenFactorToClipAtScreenEdge）を掛け、16 で割る。許容差（CompareTolerance）は始点の 1 / 深度 ÷ 16（UE の、画素からまっすぐ奥へ行く光線が 1 歩で進む深度の 2 倍を、1 / 深度の単位にしたもの）。始点を InterleavedGradientNoise(画素, フレーム mod 8) − 0.5 歩ずらす（TAA がならす）。
   - 4 歩ずつ深度を読み（1 / 深度。何も無い 0 は無限遠）、光線 − 面の差 d が (−2 × 許容差, 0) に入った最初の歩（光線が面の奥へ許容差の 2 倍以内）で当たりとし、前の歩との差を直線で補間して当たりの位置を出す。
   - 当たりのビュー空間の位置を `ssrReproject` で前のフレームの画面位置へ移して履歴の色を読む。重み = 当たりの画面の端のフェード（このフレームと前のフレームの位置の小さい方）× 粗さのフェード × 強さ。
   - 合成: `finalRadianceScaled = SSR の色 × 重み × colorSpecularEnvironmentReflectance（× coloredEnergyConservationFactor）+ (1 − 重み) × finalRadianceScaled`（キャプチャの反射は Babylon の中ですでに環境 BRDF を通っている）。
   - `post.ssr.debug`（`CUSTOM_FRAGMENT_BEFORE_FRAGCOLOR` で `finalColor` を置き換える。色は読まない: 反射の色を出すと履歴を通って自分に返り、黒か白へ流れる）: 1 = 反射の重み（赤）・粗さのフェード（緑）・辿った画素（青）、2 = すべての面を光沢があるとみなして、当たり（赤）・当たりの端のフェード（緑）・辿った画素（青）。

### 深度の代理（depth-proxies.ts）
ホテルの描画は CPU の描画の呼び出しで律速している（ヘッドレスの 1080p で CPU の時間がフレームの 9 割）。静的なレベルの数百のプリミティブが、本描画に加えて深度のパス（WebGPU では速度の G バッファも）でも 1 つずつ呼び出しになり、呼び出しが倍（WebGPU は 3 倍）になっていた。
1. **作る**（world/level.ts が材質の決まった後に `new DepthProxies(scene, staticMeshes, render.depthProxies.cell)`）: サブメッシュが 1 つで、半透明でもアルファテストでもなく、深度を書く材質の静的なメッシュを、ワールドの境界箱の中心の水平の格子（`cell` m）と、面の間引きの状態（材質の `backFaceCulling`・`cullBackFaces`、材質の面の向きをワールド行列の行列式が負なら反転したもの。深度のレンダラと同じ）で分け、位置をワールド空間へ焼いて 1 つのメッシュにする（32bit のインデックス）。代理は描かない共有の `StandardMaterial`（間引きの状態だけ）、`layerMask` 0x10000000（どのカメラの 0x0FFFFFFF とも重ならないので本描画・影・ミニマップには出ない）、ピック不可、`freezeWorldMatrix`。
2. **描く**（`attach` した RTT の `getCustomRenderList`。フレームごとに 1 回作って使い回す）: 視錐台に入る代理 + シーンのアクティブなメッシュのうち代理が含まない物（動く部品、敵、シャード、アルファテストの物）。Babylon は明示した一覧の物を視錐台で間引かず、layerMask も見ない（`forceLayerMaskCheck` が偽）ので、代理はここで視錐台を見る。
3. 代理を切った（`render.depthProxies.enabled=false`）画と比べて、ロビーと迷路で変わる画素は 0.0 % / 0.2 %（TAA の揺れ程度）。

### update(teleport, boost)
- 霧: ゾーンが `'field'`（デバッグフィールド）なら `fogDensity = 0`、ほかは `fogDensity = fogExtinction(STAGE_FOG, ueZ(カメラの y)) / 2.2`（毎フレームのカメラの高さで。Babylon の PBR は霧の係数を `toLinearSpace` で 2.2 乗するので、UE の透過率 `exp(−減衰 × 距離)` になるよう割る）、`fogColor = STAGE_FOG.inscattering`（シーンの単位の輝度。霧はトーンマップの前の線形の色に混ぜ、UE と同じく露出が掛かる）。`fx = teleport`（game.ts は `teleport.fx` を渡す）、`boostFx = boost`。
- モーションブラー（作ったときだけ）: `cutFrames > 0` の間は `motionStrength = 0` にして 1 減らし、それ以外は `post.motionBlur.strength (1.2) × MotionBlurAmount ÷ 0.5`（CONFIG の強さは UE の既定 0.5 のときのもの）。
### cut()（テレポートの移動の瞬間）
- `taa._taaThinPostProcess._reset()`（Babylon の内部。`_firstUpdate = true` になり、次のフレームは履歴を使わず今の画だけにする）で、元の場所の履歴が移動先に残らないようにする。
- `cutFrames = 2`: 移動したフレームはカメラが数 m 跳ぶので、オブジェクトベースのモーションブラーが全画面を大きく流してしまう。その回と次の回の `update` で強さを 0 にする。
- game.ts が `teleport.onArrive` で呼ぶ（`teleport.update` は `post.update` より前なので、同じフレームから効く）。

### 常に付けたままにする（`fxOn` で素通し）
- 効いているかの判定（private getter）: ブーストは `lines + vignette + blur > 0.001` か `shake` が 0 でない（`boostOn`）、カメラアニメのパスは `|ev| > 1e-4` か、テレポートかスピードブーストのティントの各成分の 1 からの差の和が 1e-4 を超える（`tinted`）か、取得の演出の `grade` か `flash` が 1e-4 を超える（`teleportOn`）。効いていないパスは `fxOn = 0` で、シェーダの先頭で入力をそのまま返す。
- 以前は効いていないパスを `detachPostProcess` で外していた（`syncAttachment`）。しかし付け外しのたびに Babylon は PrePass を作り直し（`prePassRenderer.markAsDirty()` → 次の描画で `PrePassRenderer._update`）、その中で `imageProcessingConfiguration.applyByPostProcess` をいったん false にしてから戻すので、setter が全マテリアルの全サブメッシュを dirty にする。その結果、1 フレームでエフェクトを 260〜460 個作り直し、CPU を 2.6 倍遅くした計測（Windows の Chrome 相当）では付けたフレームが 52〜74 ms、外したフレームが 38〜50 ms、直後の反射プローブのフレームも 17〜24 ms かかった。ダッシュの開始と終了、振り向き、追跡の開始と終了のたびに起き、移動中に 40 fps を割る主因だった（2026-09-12）。
- 付けたままにしたので、効いていないときも 2 パス（テレポートとブースト）それぞれに全画面のコピー 1 回分（テクスチャの読みと書き 1 回ずつ）の GPU コストがかかる。ロード中のプリウォームで 2 パスのシェーダもコンパイルされる（以前の `setPrewarm` は不要になった）。

### シェーダ（shaders.ts）
- 明るさ（設定の BRIGHTNESS。原作はコンソールコマンド `gamma 1.8〜2.2`）: チェーンの最後のブーストのパス（`wasamiBoost`）が、素通しのときも含めて出力に `bright(c)` = `brightPow == 1 ? c : pow(max(c, 0), brightPow)` を掛ける（uniform `brightPow` = `PostFx.brightness`。onApply で `fxOn` より先に毎フレーム入れる）。この時点の画像はトーンマップ・ガンマ変換後の値なので、UE の表示ガンマを g にしたときの `値^(2.2/g)` と同じになる。既定（1）では何も変えない。
#### wasamiUeTonemap（ピクセルシェーダ）
UE 4.21 のトーンマッパー（`PostProcessTonemap.usf`）を、テレポートのパスの後の線形 HDR の画に掛ける。色補正とトーンマップは UE と同じく CPU で LUT に焼いてある（下の「LUT の焼き込み」）。
- uniforms：`textureSampler`、`lutSampler`（GLSL は `highp sampler3D`、WGSL は `texture_3d<f32>`）、`bloomSampler`、`bloomIntensity`、`exposure`、`vignette`、`aspect`、`exactSrgb`。
- 処理：
  1. `c = (max(入力, 0) + bloomSampler × bloomIntensity) × exposure`（ブルームは半分の大きさを bilinear で引き伸ばす。UE も SceneColor + Bloom × Intensity に露出を掛ける）。
  2. ビネット（`VignetteSpace` と `ComputeVignetteMask`）：`p = (vUV × 2 − 1) × (1, aspect) × √2 / √(1 + aspect²) × vignette`（四隅が中心から √2 になる空間）、`c × (1 / (|p|² + 1))²`（cos⁴ の周辺減光）。
  3. `logc = clamp(log2(c + LOG_BLACK) / 14 − (log2(0.18) / 14 − 444 / 1023), 0, 1)`（UE の `LinToLog(c + LogToLin(0))`）、LUT を `logc × 31/32 + 0.5/32` で引いて × 1.05（UE の余裕分）= 表示の sRGB。
  4. 後のパイプラインの画像処理はガンマの符号化しかしないので、それで LUT の sRGB になる線形の値を出す: 既定は `sRGB^2.2`（Babylon の `toGammaSpace` は pow 1/2.2）、`exactSrgb` なら sRGB の正確な逆変換。
- WGSL 版は `textureSampleLevel(..., 0.0)` と `select`。

#### wasamiBloomDown（ピクセルシェーダ）
UE の縮小（`PostProcessDownsample` の 4 タップ）とブルームのセットアップ（`PostProcessBloom.usf`）。
- uniforms：`textureSampler`、`texel`（入力の 1 texel）、`setup`、`threshold`、`exposure`。
- 処理：出力の画素の中心から入力の ±1 texel の 4 か所を bilinear で読んで平均（= 4×4 の箱）、0〜65536 に収める。`setup` が 1 でしきい値が −1 より大きければ `clamp((dot(c, (0.3, 0.59, 0.11)) × exposure − threshold) / 2, 0, 1)` を掛ける（UE の Luminance）。−1 以下では掛けない（UE はしきい値のパスを省いて全画素をそのまま使う）。

#### wasamiBloomBlur（ピクセルシェーダ）
UE の `WeightedSampleSum` のガウスを 1 方向に（`gaussianTaps` と同じ）。
- uniforms：`textureSampler`、`addSampler`、`texel`（ぼかす向きの 1 texel）、`radius`、`taps`、`tint`、`addOn`。
- 処理：`i = −taps, −taps + 2, …, taps`（最大 32 回）について `w0 = exp(−16.7 (i / radius)²)`、`w1 = i < taps ? exp(−16.7 ((i + 1) / radius)²) : 0` の 2 texel を、重みの中心 `i + w1 / (w0 + w1)` の 1 回の bilinear で読み、重み `w0 + w1` で平均（重み 0 は飛ばす）。rgb に `× tint`、`addOn` なら `addSampler`（前の段、bilinear で引き伸ばす）の rgb を足す。アルファもぼかしてそのまま出す（被写界深度の遠景の層は重みをアルファに持つ。ブルームではアルファは 1 のまま）。
- WGSL 版は `textureSampleLevel(..., 0.0)`（ループの中で読むため）。

#### wasamiSsao（ピクセルシェーダ。`EffectRenderer` で描く）
UE 4.21 の SSAO（`PostProcessAmbientOcclusion.usf`、AmbientOcclusionQuality 50 = 半分の大きさの 1 段、6 方向、1 歩）に倣ったもの。
- uniforms：`depthSampler`（カメラ空間の深度 m）、`proj`、`outSize`、`depthTexel`、`radius`、`adjust`、`fade`。
- 処理：深度 0（何も無い）なら 1。ビュー空間の位置 `((uv × 2 − 1) / proj × z, z)`、法線は深度から（左右と上下の隣の位置との差のうち短い方どうしの外積、カメラ向きにそろえる。UE は GBuffer の法線を読む）。4×4 の画素の位置で 16 通りの角度 `2π (x / 4 + y / 16)` だけ `AO_SAMPLES` を回し（`mat2`）、`radius × proj / 2`（uv）倍した点とその反対の点で、`d = 標本の位置 − 位置` の `saturate(dot(d, n) / |d| − bias)` を重み `saturate(1 − |d| / (2 × radius × z))`（カーネルのビュー空間の届く範囲 radius × z の 2 倍で 0）で平均し、`ao = 1 − 平均`、`1 − (1 − ao^Power) × Intensity`、深度 × 100 cm で `fade` の区間で 1 へ。出力 `(ao, z, 0, 1)`。標本の対は JS で 6 行に展開する（WGSL の配列の動的な添字を避ける）。

#### wasamiSsaoBlend（ピクセルシェーダ。`EffectRenderer` で描く）
- uniforms：`textureSampler`（遮蔽の画像、NEAREST）、`depthSampler`、`aoTexel`。
- 処理：まわり 4×4 の遮蔽の texel（`(x, y) − 1.5` texel。回した 16 通りを覆う）を、深度の差 `|t.y − z| / (0.05 z + 0.02)` で 1 から減る重みで平均し（重みの和が 1e-3 以下なら 1）、遮蔽だけを出す（材質が読む）。

#### wasamiDofSetup / wasamiDofMerge（ピクセルシェーダ）
- Setup：uniforms `textureSampler`（シーン）、`depthSampler`、`texel`、`far`。出力の texel の 2×2 のシーンの画素（±0.5 texel）それぞれを 0〜65536 に収め、遠景のマスク `saturate((深度 − far.x) × far.y)` で重みを付けて `(色 × m, m)` の平均（前乗算）。
- Merge：uniforms `textureSampler`（ぼかした遠景の層）、`sceneSampler`、`depthSampler`、`far`。`f.a > 1e-4` なら `mix(シーン, f.rgb / f.a, 画素のマスク)`。近い物はマスク 0 なので鮮明なまま、遠景の層にも入らないのでにじまない。

#### LUT の焼き込み（ue-grade.ts の `buildLut` / `gradeColor`）
UE の `CombineLUTsCommon`（sRGB の表示）をそのまま移した。LUT の各点 i（0..31）の線形の色 `LogToLin(i / 31) − LogToLin(0)` について:
1. `whiteBalance(WhiteTemp, WhiteTint)` → `SRGB_TO_AP1`（= XYZ→AP1 · D65→D60 · sRGB→XYZ）。
2. 広色域への伸ばし：AP1 の輝度 `luma`（`AP1_Y` = (0.2722, 0.6741, 0.0537)）で割った色度の 1 からの距離² `d` について `(1 − 2^(−4d)) × (1 − 2^(−4 × ExpandGamut × luma²))` だけ、`EXPAND`（= XYZ→AP1 · Wide→XYZ · AP1→sRGB）を掛けた色へ寄せる。
3. `ColorCorrectAll`：影・中間・ハイライトの 3 通りに `ColorCorrect`（彩度 `mix(luma, c, S)` → 0 以上、コントラスト `(c / 0.18)^C × 0.18`、ガンマ `c^(1/G)`、`c × Gain + Offset`。各パラメータは範囲の FVector4 × 全体の FVector4 の rgb × w、オフセットは和の rgb + w）を掛け、補正前の輝度で `影 = 1 − smoothstep(0, ShadowsMax, luma)`、`ハイライト = smoothstep(HighlightsMin, 1, luma)`、`中間 = 残り` の重みで混ぜる。
4. 青の補正 `BlueCorrection`（AP0 で定義された行列を AP1 に移したもの）を掛けて `FilmToneMap`、補正を戻す。
5. `FilmToneMap`：AP0 で RRT の glow（0.05・0.08）と赤の調整（0.82・0.03・幅 135°）、AP1 に戻して 0 以上・0.96 の事前の脱色、`log10` の上でトウ・直線・ショルダーを `ToeMatch` / `StraightMatch` / `ShoulderMatch`（0.18 が 0.18 になるよう解いたもの。`filmCurve`）でつなぎ、0.93 の事後の脱色。
6. AP1 → sRGB、0 以上、`LinearToSrgb`（UE の分岐なし版）。LUT には ÷ 1.05 を 8bit で入れる（x = 赤が最も速く、次に緑、青）。
- 検証（`tests/ue-grade.test.ts`）: 行列の往復と 6500 K が単位行列、7657 K で暖色、既定のカーブの単調性と白、ゾーンの上書きの順（迷路の WhiteTemp は unbound の 7657、FilmShoulder は既定、GammaMidtones は unbound）、ホテルのグレーが暖色、LUT の大きさと原点の黒と格子点の一致、ビネットの値、被写界深度（迷路は 12.9 m まで鮮明で 24.8 m かけて全部ぼける、ロビーは 31.5 m まで鮮明、近景は 4 % 未満、遠景の半径 1920 幅で 22.3 texel と 31 の頭打ち）、SSAO（半径 0.2125、半分遮蔽でロビー 0.75・迷路 0.045、30〜80 m のフェード）、画面空間の反射（3 ゾーンとも既定の 100・50・0.6、Quality 50 は段 2 = 16 歩 1 本、r.SSR.Quality での頭打ち、粗さのフェードが 0.3 まで 1・0.45 で 0.5・0.6 で 0、端のフェードが 0.8 まで 1・0.9 で 0.75・端で 0）。既定の設定で 18% のグレーは sRGB 0.4616（線形 0.18 → 0.18）。

#### wasamiBoost（ピクセルシェーダ）
原作のスピードブースト（05 記録の `boost-fx.ts`、pak_reference）: 効果中は Chameleon のポストプロセス（画面の揺れと、`T_RadialBlurMask` の白い所 = 画面の縁だけのラジアルブラー）が速さに合わせて掛かり、その上にウィジェット `UMG_SpeedBoost`（赤い集中線のフリップブックと、その上の赤いビネット）が重なる。ウィジェットは UE では UI としてトーンマップの後に描かれるので、このパスもパイプラインの後の LDR の画に掛ける。
- uniforms：`textureSampler`、`speedlinesSampler`、`vignetteSampler`、`maskSampler`、`fxOn`、`lines`、`vignette`、`frame`、`blur`、`samples`、`shake: vec2`、`scales: vec2`（集中線・ビネットの拡大）、`grid: vec2`（列・段）、`color: vec3`。
- 処理：
  1. `uv = vUV + shake` で 1 サンプル。`blur > 0.0001` なら、中心 (0.5, 0.5) へ `1 − blur·i/(samples − 1)` 倍に縮めた uv の `samples` 個（ループ上限 16）の平均を、`maskSampler`（vUV で読む）の値で混ぜる。
  2. 集中線：`lu = (vUV − 0.5) / scales.x + 0.5`、コマ `frame` の列 `mod(frame, grid.x)`・段 `floor(frame / grid.x)`（上から数える。テクスチャは invertY なので v は `1 − (段 + 1 − lu.y) / grid.y`）で `speedlinesSampler` を読み、`mix(c, color, clamp(値 × lines))`。
  3. ビネット：`(vUV − 0.5) / scales.y + 0.5` で `vignetteSampler` を読み、`mix(c, color, clamp(値 × vignette))`。
- ブラーの量（`blurReach`）は、原作の `M_RadialBlurHLSL` の式が cook で消えているので本作の推定。揺れの形（`shakeOffset`）も `M_CameraShake` の式が消えているので、原作の収録で測った形（画像全体が 15 Hz で円を描き、全速で 0.002 uv）にした（05 記録）。`shake` はシーンの画像ごとずらすので、タブレットも一緒に揺れる（収録でも同じ）。
- WGSL 版は `textureSampleLevel(..., 0.0)` を使い（ミップマップなしで、ブラーのループの中でも読めるように）、`mod` を `frame − grid.x·floor(frame / grid.x)` で書く。

#### wasamiTeleport（ピクセルシェーダ）
原作 `CameraAnim_Teleport` の露出（AutoExposureBias）とシーンのティント（SceneColorTint）を、TAA の直後の線形 HDR の画に掛ける（UE のトーンマッパーもシーンの色に両方を掛ける。05 記録）。FOV とシェイクはカメラ側（05 記録の `applyToCamera`）、放射状の流れは FOV の変化に反応するモーションブラー。タブレットも画の一部なので、一緒に赤くなり白く飛ぶ（原作と同じ）。スピードブーストの `CameraAnim_SpeedBoost` のティント（05 記録の `boost-fx.ts`）も同じパスで掛けるので、ブーストの間もタブレットごと赤くなる。
- uniforms：`textureSampler`、`fxOn`、`gain: vec3`（`2^EV × tint`）、`grade: vec4`（取得の演出の ColorGain と重み）、`flash: vec2`（中間調のゲインと色収差の倍率）。
- 処理：`flash.y > 0.0001` なら赤を中心から `1 − flash.y` 倍、青を `1 + flash.y` 倍の uv で読み直し（UE の SceneFringe）、`color × gain`、輝度 `dot(color, (0.2126, 0.7152, 0.0722))` へ向けて彩度を `1 − grade.a` に下げて `mix(1, grade.rgb, grade.a)` を掛け（UE の ColorSaturation 0 と ColorGain を重みで混ぜたもの）、`× flash.x`（ColorGainMidtones を画面全体に掛ける近似）、`min(…, 64)`。頭打ちの 64 は、+100 EV のフレーム（`gain` は `2^60`）でも half float のテクスチャで無限大にしないため（ACES のトーンマップで 64 は白）。
- WGSL 版は `textureSampleLevel(..., 0.0)`。

#### 登録方法
- GLSL：`varying vec2 vUV; uniform sampler2D textureSampler;` + `gl_FragColor`。
- WGSL：`varying vUV: vec2f; var textureSamplerSampler: sampler; var textureSampler: texture_2d<f32>; uniform xxx: type;` を Babylon の WGSL ポストプロセス規約で書き、`@fragment fn main(input: FragmentInputs) -> FragmentOutputs` と `fragmentOutputs.color` で出力。
- `wasamiUeTonemap` とブルームの 2 つ以外の 4 つは uniform `fxOn` を持ち、0.5 未満なら main の先頭で入力を `(rgb, 1.0)` のまま出して返す（GLSL は `return;`、WGSL は `return fragmentOutputs;`。Babylon 自身の WGSL シェーダも同じ書き方）。名前を `active` にすると、GLSL ES 3.00 と WGSL の両方で予約語なのでコンパイルできない（WebGL2 はエフェクトが準備できず `whenReadyAsync` が終わらないので読み込みが止まり、WebGPU はそのパスのパイプラインが無効になる）。シェーダの文字列はテンプレートリテラルなので、コメントにバッククォートを書かない。
- 両言語とも `ShadersStore[...]` / `ShadersStoreWGSL[...]` のキーは `<名前>PixelShader`。`PostProcess` 生成時の `shaderLanguage` オプションでどちらを使うか決まる。

## 依存関係
- import：`../config`（`CONFIG.post.*`, `CONFIG.render.msaaSamples`）、`../player/teleport-fx`（`NO_FX`、`TeleportFx` 型）、`../player/boost-fx`（`CHAMELEON`、`SPEEDLINES`、`WIDGET`）、`../world/collect-fx`（`COLLECT_POST`）、`./shaders`（シェーダ名 9 種と `registerShaders`）、`./ue-grade`（LUT・ブルーム・霧・DOF・SSAO の値と式）、`./scene-ao`（`SceneAo`）。scene-ao.ts は `./shaders`（`SSAO` / `SSAO_BLEND`）と `./ue-grade` を使い、world/level.ts が `addSceneAo` を呼ぶ。
- 使う側：`src/game/game.ts`（`new PostFx(scene, player.camera)`、毎フレーム `post.update(teleport.fx, { tint, lines, vignette, frame, blur, shake })`、テレポートの移動時に `cut()`、デバッグ表示に `post.debugInfo`）。
- Babylon.js：`TAARenderingPipeline`, `DefaultRenderingPipeline`, `ImageProcessingConfiguration`, `MotionBlurPostProcess`, `PostProcess`, `ShaderStore`, `ShaderLanguage`, `Texture.BILINEAR_SAMPLINGMODE`, `Color4`, `Vector2`, `Vector4`, `RawTexture`, `RawTexture3D`、scene-ao.ts は `EffectRenderer`, `EffectWrapper`, `RenderTargetTexture`, `MaterialPluginBase`, `scene.enableDepthRenderer()`。

## 設定・調整値
- `post.taa`：`enabled true, samples 8, factor 0.12, reprojectHistory true, clampHistory true`。`post.fxaa false`。
- `post.bloom`：`enabled true, scale 1`（UE のブルームに掛ける倍率。1 = UE のまま）。強さとしきい値はゾーンのボリュームの `BloomIntensity` / `BloomThreshold`。
- `post.ssao`：`enabled true`、`post.dof`：`enabled true`（値はゾーンのボリュームの AO と DOF。QUALITY の LOW では作らない）。
- `post.ssr`：`enabled true, debug 0`（画面空間の反射。値はゾーンのボリューム。QUALITY の LOW と TAA が無いときは作らない。`debug` は上の「画面空間の反射」）。
- `render.depthProxies`：`enabled true, cell 20`（深度のパスが静的な形を 20 m の格子ごとに束ねて描く）。
- `post.ue.exposureScale`：1（ボリュームの手動露出 `manualExposure` に掛ける倍率。下の「露出」）。
- ステージのボリューム（`STAGE_VOLUMES`。cc2_reference の `Chaotic_Customer_Zone_1.umap`）:
  - `surface`（`PostProcessVolume3`）: WhiteTemp 7485.71、ColorSaturation (1.1, 1.05, 1) × 0.9、ColorGamma (0.95, 1.05, 1.04)、ColorGain (1.1, 0.9, 1.1) × 1.15、ColorOffsetShadows w −0.007、ColorSaturationMidtones × 1.1、SceneColorTint 白（override のみ）、BloomIntensity 1.1（しきい値は既定 −1）、VignetteIntensity 0.2、手動露出 ISO 4500・1 s・f/4.5、ColorGradingLUT `TX_LUT_02` を 0.7。
  - `metro`（`PostProcessVolume4`）: WhiteTemp 7900・WhiteTint 0.1、ColorSaturation (0.9518, 0.95, 0.9202) × 1.05、ColorContrast (0.985, 1, 0.9916) × 0.92、ColorGamma (1.0419, 1.0169, 0.9852) × 0.9、ColorGain (1.0991, 1.1, 1.0897) × 1.26、ColorGainShadows (1, 0.9681, 0.9756) × 0.99、ColorOffsetShadows (−0.0128, −0.007, 0) − 0.004、SceneColorTint (1, 0.9301, 0.9611)、FilmBlackClip 0（override のみ）、BloomIntensity 1.35、VignetteIntensity 0.2、手動露出 ISO 4800・1 s・f/4.4、ColorGradingLUT `LUT_U1_Filmic_Horror_Night` を 0.12。
  - どちらも SSAO は Intensity 1・Radius 90 cm（Power などは既定）、SSR は Intensity・Quality を override して値なし（100・50）・MaxRoughness 1。被写界深度は焦点距離が無く描かない（f 値は露出にだけ使う）。レンズフレア（強さ 2.07・しきい値 9.82）は入れていない。モーションブラー（どちらも MotionBlurAmount 0・MotionBlurMax 0 を override）は使わない（「既知の制約」の「モーションブラーの量」）。
- `post.motionBlur`：`objectBased true, strength 1.2, samples 32`。
- テレポートのパスに config はない（露出とティントは 05 記録の `teleport-fx.ts` の原作データ）。
- スピードブーストのパスに config はない（値は 05 記録の `boost-fx.ts` の原作データ）。
- `render.msaaSamples 1`。
- 例：`?post.motionBlur.strength=0.6&post.taa.enabled=false&post.fxaa=true`。

## 既知の制約・注意点
- **原作のポストプロセスボリュームの読み方**：UE は `bOverride_*` が立っている設定だけを混ぜる。pak の値には override の無いもの（`PostProcessVolume_1` の AutoExposureBias −3.5・GrainIntensity 0.25・MotionBlurAmount 0・LensFlareIntensity 16、`PostProcessVolume3_3` の WhiteTemp 4457・FilmShoulder 0.98・ColorGammaMidtones 0.78 など）も残っているが、UE では効かないので使わない。override があって値が無いもの（迷路の BloomThreshold）は UE の既定値。
- **モーションブラーの量**：ステージの `PostProcessVolume3`・`PostProcessVolume4` はどちらも `bOverride_MotionBlurAmount` 付きで MotionBlurAmount 0（MotionBlurMax 0、TargetFPS 120 も）にしていて、ファンゲームのプレイヤー（`ThirdPersonCharacter` の `CameraComponent`）も同じ値を override している。これはファンゲームが自分のプレイヤーのために切ったもので、本作のプレイヤーは Dark Deception のもの（pak_reference の `BP_DD_PlayerCharacter` などにモーションブラーの設定は無く、UE の既定 0.5。README の冒頭の「原作の特徴である強いモーションブラー」）。そのため `STAGE_VOLUMES` にモーションブラーの値を入れず、どのゾーンも UE の既定 0.5（`post.motionBlur.strength` 1.2 のまま）にする。2026-09-14 にステージのボリュームにしたときはこの 0 を入れていて、モーションブラーのパスが作られなくなっていた（2026-09-15 にユーザーの指摘で戻した）。新しい Steam 版（pak_reference_2）ではモーションブラーが撤廃されている（`pak_reference_2/_raw/DDeception/Config/DefaultEngine.ini` の `r.DefaultFeature.MotionBlur=False`）が、旧版（pak_reference）に従ってモーションブラーを残す（2026-09-15、ユーザーの決定）。
- **ゾーン**：原作では迷路（z ≈ 69 m）とロビー（z ≈ 0）が高さで分かれていてボリュームの箱もそうなっているが、本作は両方の床を 0 にして横に並べたので、迷路とロビーの床の範囲が x で重なる（Blender の x で迷路 −44〜80.7、ロビー 80〜118）。そのため位置ではなく game.ts の `phase` で決める。UE のボリュームの BlendRadius の混ぜ合わせはしない（移るのはエレベーターの暗転の中）。
- **露出**：ステージのボリュームは UE5 の手動露出（AEM_Manual、物理カメラ）なので、その値から計算する（`manualExposure`。surface 3.70・metro 4.13）。ベイクの単位が UE の輝度と同じことは Blender で確かめた（白い板の 1 m 上に 4π W = 1 cd の点光源を置いた DIFFUSE の焼き込みが 0.315 ≈ 1/π）。ライトマップとキャプチャの倍率（`lightmap.intensity`・`environment.captures.intensity`）は 1 にした。露出補正の既定 1（UE5 の既定と判断）と、ファンゲームが Lumen を使ったかは確定できず、ファンゲームの画面収録とはまだ比べていない（地上の白い面は明るく出る）。以下は Hotel のときの記録: UE は自動露出（4.21 の既定のヒストグラム。原作のボリュームは LowPercent などを override しているが値は既定）で画面の明るさに合わせる。Hotel はゾーンごとの固定の `post.ue.exposure` で、ライティングも Cycles の焼き直しなので、本家の収録（2026-09-13 のユーザーの画面収録、1080p）に合わせた推定。迷路は像の部屋（収録の 83.3 s、エレベーターを出たところ）で sRGB の輝度の平均・中央値・90 % 点が本家 50 / 34 / 113、本作 0.76 で 49.6 / 33.6 / 118.9（90 % 点が 5 % 明るい）。ロビーは本家の受付前（16.5 s。タイトルの黒い帯が中央に重なる）が 15 / 6 / 36、ラウンジ（20.5 s）が 31 / 14 / 85 で、本作は同じ構図の受付前が 0.7 で 17 / 11 / 42。反射キャプチャと SSAO を入れる前（反射は弱い HDRI、SSAO なし）は迷路 0.75・ロビー 0.35、ライトマップを UE の減衰の窓で焼き直す前は迷路 0.5・ロビー 0.3 だった（2026-09-13）。HDRI の反射が光沢のある大理石を一様に明るくしていたので、原作のキャプチャに替えるとロビーが大きく暗くなり、露出を 2 倍にした。
- **霧**：UE の指数高さフォグは画面のパスで深度から掛けるが、本作は Babylon のマテリアルの指数フォグ（`length(視点からの位置)` の距離）で代える。迷路はカメラが霧の高さより 69 m 上なので、部屋の高さの中での密度の変化は 1 % 程度で、指数フォグで UE の値にほぼ一致する。ロビーは霧の高さに近く（床から 0.8 m 上）、上下を見たときの高さによる変化は再現しない。Babylon の StandardMaterial（特殊シャードの加算の輪など）は 2.2 乗しないので、PBR より霧が薄い。霧の色は、UE の自動露出の値が原作データに無いので、本家の収録のロビーの廊下の奥の霧（sRGB でおよそ (38, 33, 33)）が原作の色を露出 1 で見た明るさに近いことから、原作の色 ÷ 本作の露出とした（推定）。
- **ブルームの推定部分**：段の大きさ・色・縮小・カーネル（exp(−16.7 (x/R)²)、31 texel の頭打ち）とセットアップの式は UE 4.21 のレンダラのもの。各段の Tint に掛ける 1/段数（`BLOOM_TINT_SCALE`）と、`Size` を画面幅の百分率の直径とみる（半径 = 半分）ことは、原作データに無いレンダラ側の値を UE のソースの記憶から入れたもの。原作の迷路の強さ 8 では、合計でおよそもう一度画面を足す（Tint の和 0.795 ÷ 6 × 8 = 1.06）。ランプのまわりのもやと、画面全体が柔らかく持ち上がる見え方は収録と合う。
- LUT は UE（10bit）と違って 8bit。表示の sRGB で 1/255 刻みなので段差は目立たない。
- **SSAO の推定部分**：半径の決め方（RadiusInWS が偽のときの Radius / 400、半分の大きさの段の MipScale / 4）、ユーザー調整の式、フェード、段数（Quality 50 で 1 段）は UE 4.21 のソースの記憶から。標本の 6 点の位置と、1 標本の遮蔽の式（法線とのなす角の余弦 − bias、遠い標本の重みを減らす）は UE の方式に倣った近似で、UE の 4×4 の乱数のテクスチャと TAA の揺らしの代わりに、固定の 16 通りの回転を 4×4 のフィルタで平均する。法線は UE の GBuffer（法線マップ入り）でなく深度から求める。最初は TAA の後の画全体に掛けていたが、灯に照らされた壁と灯そのものまで暗くなり、本家の明るさの分布（平均に対する中央値・90 % 点の比）から離れたので、UE と同じくライトマップと反射だけに掛ける形にした（2026-09-13）。
- **性能**：SSAO（深度の描画 1 回と半分の大きさの 2 パス）、DOF（4 パス）、反射キャプチャでタイルが 29 → 55 に増えたことで、ヘッドレスの bench（1080p）は WebGL2 が 204 → 147 fps（1% low 84）、WebGPU が 150 → 93 fps（1% low 56）になった（2026-09-13）。SSR を足し、深度のパスを代理にした後は WebGL2 145.9 fps（1% low 63.7）、WebGPU 106.0 fps（1% low 62.1）（README の表）。同じ位置で条件だけ変えた計測（scratchpad の A/B、3 か所の平均）では、WebGL2 全部 6.46 ms・SSR なし 6.05・SSAO なし 6.18・DOF なし 6.18・代理なし 6.68（格子 10 m 6.68、40 m 6.72）・ブルームなし 6.13・影なし 5.71。WebGL2 は 1080p で GPU の塗りが律速（1/4 の画素数で 6.97 → 4.78 ms。CPU プロファイルの `uniformMatrix4fv` 1.4 ms は GPU 待ち）、WebGPU は JS が律速（1/4 でも 7.5 ms。`_evaluateActiveMeshes` 0.77・`getBindGroups` 0.48・`bindForSubMesh` 0.34 ms）。WebGPU の SSR が経路で約 1.6 ms と重いのは、履歴の色（TAA の ping-pong）が毎フレーム替わって材質ごとの bind group を作り直すためと推定（未対策）。QUALITY の LOW では SSAO・DOF・SSR を作らない。
- **画面空間の反射の推定部分**：Quality の段の決め方・歩数・RayCast（光線の長さ、画面の端までの歩み、許容差、4 歩ずつの判定と補間）・粗さのフェード・端のフェード・合成の形は UE 4.21 のソースの記憶から。UE は HZB（深度のミップ）を粗さに応じて粗い段で読むが、本作は全解像度の深度を読む（Quality 50 は光沢の散らしが無く、段は歩みごとに粗さ × 0.5 ずつ上がるだけ）。UE は画面のパスで GBuffer から辿るが、本作は材質の中で辿るので、SSR の重みがキャプチャの反射を置き換えるのは同じでも、Babylon の環境 BRDF（`colorSpecularEnvironmentReflectance`）を使う。r.SSR.Quality は原作の ini が pak に無いので UE の既定 3 とした（Quality 50 では 2 が上限なので結果は同じ）。原作の床（`MM_Main_Substance` の Packed の G）は、ロビーの入口と廊下の床が粗さ 0.49 前後（フェード 0.37）、大理石の壁 0.55（0.17）、柱 0.53、厨房の床 0.61（0 = 反射なし）。
- **DOF**：UE の Gaussian DOF の近景の層（FocalDistance より手前）は、ホテルのボリュームでは NearTransition が 38 m あって最大 4 % しか混ざらないので作らない。UE の合成の細部（遠景の層の重みの戻し方）は推定。
- **シャープ**：入れない。UE 4.21 のトーンマッパーのシャープ `r.Tonemapper.Sharpen` の既定は 0（off）で、原作データ（pak_reference）に上書きが無い（ini は pak_reference に含まれない）。
- 以前は Babylon の ACES（`exposure 0.68`、`contrast 1.28`）、暗い赤のビネット（`weight 2.4`）、グレイン（7）、色収差（22）、パイプラインのブルーム（しきい値 0.55・重み 0.62・カーネル 80・scale 0.5）を使っていた（2026-09-13 まで）。
- **WebGPU の速度バッファ**：Babylon 9.26 では WebGPU で PrePass の速度テクスチャが束縛されない（`Trying to bind a null texture: velocitySampler`）。そのため WebGPU では TAA は再投影なし（`disableOnCameraMove=true`、静止中のみ蓄積、移動中は MSAA 1x 相当のジャギー）、モーションブラーは `GeometryBufferRenderer` から速度を取る（パスが 1 つ増える）。WebGL2 は PrePass 再投影付き TAA がそのまま動く。ただしロード中の `Minimap.capture` の RT 描画で PrePass が無効のまま確定する問題があり、`capture` の最後に `scene.prePassRenderer.markAsDirty()` で作り直させている（11-minimap.md）。
- **パスのテクスチャの持ち主**：Babylon の `PostProcess.activate()` は自分のテクスチャ（`size` の大きさ）を束縛して前のパスに描かせ、`apply()` は自分のテクスチャを入力として次のパスのテクスチャへ描く。`setTextureFromPostProcessOutput(ch, p)` は p の出力（= p の次のパスのテクスチャ）、`setTextureFromPostProcess(ch, p)` は p の入力（= p のテクスチャ）。最初の実装はパスの出力を指していたため、トーンマップが半分の大きさのシーンを読んで画面全体が半分の解像度になり、WebGPU ではぼかしの読み書きがずれて明るい所が赤くなっていた（2026-09-13 に直した）。
- **ポストプロセスの着脱と PrePass**：`camera.attachPostProcess` / `detachPostProcess` は毎回 `prePassRenderer.markAsDirty()` を呼び、先頭のポストプロセス（TAA）の `markTextureDirty()` も呼ぶ。PrePass の作り直しは全マテリアルを dirty にして重いので（上の「常に付けたままにする」）、実行中にパスを付け外ししない。新しいパスを足すときも、生成時に付けて `fxOn` のような uniform で素通しさせる。
- `DefaultRenderingPipeline` 自体に TAA はないため `TAARenderingPipeline` を先に重ねている。TAA ジッターとオブジェクトベースのモーションブラーの併用で、静止直後の数フレームに細部がわずかに揺れることがある。
- `cut()` の TAA 履歴の破棄は内部の `_taaThinPostProcess._reset()` に依存する（無ければ何もしない）。
- `debugInfo` のモーションブラー表示は CONFIG の値を出すだけで、実行時の実際の値ではない。
- `dispose()` の `void this.scene` は未使用警告回避。
- テレポートの露出とティントは、`scene.imageProcessingConfiguration.exposure` ではなく独立のパスで掛ける（exposure を毎フレーム変えると全 PBR マテリアルのサブメッシュが dirty になり、パイプラインの画像処理パスは `onApplyObservable` の後に bind されるので uniform を上書きすることもできない）。このパスはパイプラインより先に生成してパイプラインの前に付けているので、パイプラインが実行中に作り直される（設定を変えると `_buildPipeline` がパスを付け直して連鎖の末尾へ回す）と並びが崩れる。今は実行中に作り直さない。
- 以前はテレポートの演出を、トーンマップ後の LDR で映像から合わせた式（放射ズーム・明るさの持ち上げ・赤み・暗転・白の色）で作っていた。原作のデータ（pak_reference）に合わせて、HDR の露出とティントだけにした（2026-09-12）。
- 以前はスピードブーストの画面を、検証映像から合わせた赤い縁の帯と手続きの集中線（96 方向、60fps の 1 フレームごとに乱数で引き直す）と、発動の 1 フレームの閃光と放射ブラーで描いていた。原作のデータ（pak_reference）に合わせて、原作のテクスチャのウィジェットと Chameleon のブラー・揺れにした（2026-09-12）。
- **TAA のテクスチャ形式**：`TAARenderingPipeline` の既定の `textureType` は 0（8bit）。連鎖の先頭にあるため、既定のままだとシーンの HDR 値がブルームとトーンマップの手前で 0〜1 に切り詰められ、リニア 8bit への量子化で暗部が黒く潰れ、炎まわりの光はチャンネルごとに頭打ちして白っぽく色が抜けていた。そのため half float を明示している。
- TAA のクランプ（Babylon の `taa.fragment`）は近傍の最小値を `vec4(1)` から始めるので、HDR で 1 を超える領域では履歴の下限が 1 になる（ゴーストがわずかに残りうるが、明るい領域に限られる）。
- **材質のプラグインのサンプラーの宣言と UBO**：Babylon の WebGL2 は macOS の Chrome でだけ uniform buffer（UBO）を切る（thinEngine の `ExceptionList` の `"Mac OS.+Chrome"`）。プラグインの `getUniforms().fragment` は UBO が無いときだけ入る（PBR の `pbrFragmentDeclaration` の `#define ADDITIONAL_FRAGMENT_DECLARATION` を置き換える。UBO のときは `ubo` の分が `ADDITIONAL_UBO_DECLARATION` に入る）ので、`uniform sampler2D` はそこに書かず、UBO の有無にかかわらず入る `CUSTOM_FRAGMENT_DEFINITIONS` で宣言する（SSAO・SSR と 07 記録の `CaptureMixing`）。2026-09-14 まではそこに書いていたため、Windows など UBO のある環境では宣言が無くてコンパイルに失敗し（`'ssrDepthSampler' : undeclared identifier`）、Babylon のフォールバックの再試行が続いて読み込みが「シェーダーをコンパイルしています…」で止まっていた。Mac で確かめるときは Windows の Chrome の UA にする（UBO が有効になる）。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: TAA の中間テクスチャを half float に変更（8bit で HDR が切り詰められ暗部が潰れていた）
- 2026-09-11: テレポートのパス `wasamiTeleport`（放射ズーム・閃光と集中線・暗い赤の暗転・赤いビネット）を追加。着脱を並び順を保つ `attach()` にまとめ、`setPrewarm()`（ロード中に全パスのシェーダをコンパイル）と `cut()`（TAA 履歴の破棄とモーションブラーの一時停止）を追加
- 2026-09-11: テレポートのパスを参考動画の演出に置き換えた（閃光・集中線・暗転・赤いビネットをやめ、放射ズーム・暗部ほど強い赤み・ホワイトアウトに。uniforms は `rush, red, white, zoom, center, time, whiteColor`、`post.teleport.vignetteColor` → `white`）
- 2026-09-12: テレポートのパスを原作の検証映像に合わせた: rush 中の明るさの持ち上げ（`flare`、`flareGain`）、赤みの式（暗部は紫・明るい面はピンク）、白の後の暗転（`dark`。白に近いハイライトとタブレットだけ残す）、タブレットの矩形 `mask`（中では流れを矩形内にとどめ、暗くしない）を加え、白の色を (0.93, 0.96, 0.95) にした。タブレットの矩形は `update` の引数 `sharp` をやめ、`maskSource` を各パスの `onApply` で読むようにした
- 2026-09-12: スピードブーストのパス `wasamiBoost`（赤い縁、毎フレーム入れ替わる赤い集中線、発動の 1 フレームの閃光と放射ブラー）を追加し、`update` に `boost`（`BoostFx`）を足した。並び順はラジアル → ティント → ブースト → テレポート
- 2026-09-12: ラジアル・ティント・ブースト・テレポートのパスを付け外しせず常に付けたままにし、効いていないときは uniform `fxOn` でシェーダの先頭から素通しさせるようにした（付け外しのたびに PrePass と全マテリアルが作り直され、Windows の Chrome 相当の CPU で 40〜74 ms のフレームになっていた）。`syncAttachment` / `attach` / `setPrewarm` を削除し、ティントの WGSL を `textureSampleLevel` にした
- 2026-09-12: テレポートのパスを pak_reference の原作データに合わせて作り直した: トーンマップ後の LDR の `rush / red / dark / flare / white / zoom / flareGain / center / time / whiteColor / mask` をやめ、TAA の直後（パイプラインの前）の HDR のパスで `min(color × 2^EV × tint, 64)` を掛ける（uniforms は `fxOn, gain`、`textureType` は HDR）。`post.teleport` を削除した
- 2026-09-12: テレポートのパスでスピードブーストのカメラアニメのティント（`BoostFx.tint`）も掛けるようにした（テレポートのティントと掛け合わせる。`tinted`）。タブレットの独自の赤をやめ、ブーストの間もシーンのティントでタブレットごと赤くなる（10 記録）
- 2026-09-12: スピードブーストのパスを原作の構成に作り直した: 検証映像から合わせた赤い縁と手続きの集中線（`vignette / lines / lineOpacity / reach / blur / band / aspect / seed / edgeColor / lineColor`、`post.boost`）をやめ、`UMG_SpeedBoost` の集中線のフリップブックと赤いビネット（原作のテクスチャ `public/fx/`）と、Chameleon のマスク付きラジアルブラーと画面の揺れにした（uniforms `lines, vignette, frame, blur, samples, shake, scales, grid, color`、samplers 3 つ。`BoostFx` は `tint, lines, vignette, frame, blur, shake`）
- 2026-09-12: 設定の QUALITY で作るパスを決めるようにした（`PostFx` の `quality`。MEDIUM 以下でモーションブラーと色収差、LOW でブルームとグレインも作らない）。設定の BRIGHTNESS をブーストのパスの最後に掛ける（`brightness` → uniform `brightPow`、`bright()`）
- 2026-09-12: ブーストの `shake` を原作の収録で測った形（15 Hz の円、全速で 0.002 uv。05 記録の `shakeOffset`）にした（シェーダは変わらない）
- 2026-09-13: 特殊シャードの取得の演出（原作の BP_StunCollectEffect / BP_BonusShardCollectEffect のポストプロセス 2 枚: 彩度 0 + ColorGain の洗いと、中間調 100 倍と色収差の白い閃光）をカメラアニメのパス（`wasamiTeleport`）に足した（`collect`、uniforms `grade` / `flash`）
- 2026-09-13: 見た目を本家に近づけるため、Babylon の ACES・コントラスト・赤いビネット・グレイン・色収差をやめ、UE 4.21 のトーンマップ（`ue-grade.ts` の LUT とパス `wasamiUeTonemap`。原作のポストプロセスボリュームの色補正・フィルミックのカーブ・cos⁴ のビネット）にした。ゾーン（`zone`）で迷路とロビーのボリュームを切り替える
- 2026-09-13: Babylon のブルームをやめ、UE 4.21 のガウスブルーム（`wasamiBloomDown` / `wasamiBloomBlur` の 18 パス、原作の強さ 8・しきい値）にし、トーンマップのパスで足すようにした。露出をゾーンごと（迷路 0.5・ロビー 0.3）にし、本家の収録で合わせた
- 2026-09-13: 原作の指数高さフォグ（`HOTEL_FOG`・`fogExtinction`）を、Babylon の指数フォグの密度と色としてゾーンごとに入れるようにした
- 2026-09-13: ブルームとトーンマップが画像を「その画像を入力に持つパス」で指すように直した（`setTextureFromPostProcess`。各パスの `size` をそこへ描かれる画像の大きさにした）。以前はトーンマップが半分の大きさのシーンを読んで画面が半分の解像度になり、WebGPU では明るい所が赤くなっていた
- 2026-09-13: ライトマップを UE の減衰の窓で焼き直した（12 記録）ので、露出を迷路 0.75・ロビー 0.35 に合わせ直した
- 2026-09-13: 原作のボリュームの SSAO（`scene-ao.ts` の `SceneAo`: シーンの前に `DepthRenderer` の深度から `wasamiSsao` / `wasamiSsaoBlend` で遮蔽を作り、材質のプラグインがライトマップと反射から引く）と Gaussian DOF（`wasamiDofSetup`、`wasamiBloomBlurDofX` / `Y`、`wasamiDofMerge`、ブルームの前。同じ深度を読む）を足した。最初は PrePass の深度と法線を読む TAA の後のパスにしたが、PrePass の設定がポストプロセスに持たれていないと毎回切られて WebGL2 で効かず、効かせても画全体を暗くしたので作り直した。`BLOOM_BLUR` がアルファもぼかすようにし、`gaussianRadius` を分けた。QUALITY に `ssao` と `dof` を足した（LOW では作らない）。シャープは原作が使っていないので入れない

- 2026-09-13: 反射キャプチャと SSAO を入れたので、露出を迷路 0.76・ロビー 0.7 に合わせ直した
- 2026-09-14: 原作に無い追跡の赤いティントとダッシュ中のラジアルブラーを外した（原作データの BP_DD_PlayerCharacter・BP_Monkey・01_Hotel のレベル BP に追跡やダッシュの画面の効果は無く、画面の縁のラジアルブラーと赤はスピードブーストのもの）: `wasamiRadialBlur`・`wasamiChaseTint` のパスとシェーダ、`maskSource`・`readMask`、`update` の `dt`・`dash`・`chase` を削除（ブーストの Chameleon のラジアルブラーはそのまま）
- 2026-09-14: ステージ（Chaotic Customer 2 の Zone_1）のポストプロセスボリュームにした（ユーザーの指示）: `HOTEL_VOLUMES` を `STAGE_VOLUMES`（surface・metro）に、ゾーンを `'surface' | 'metro' | 'field'` に。UE5 の手動露出 `manualExposure`（物理カメラの ISO・シャッター・f 値と露出補正）で露出を決め（`post.ue.exposure` をやめて `exposureScale`）、`SceneColorTint` と、ボリュームのカラーグレーディング LUT（`stage-luts.ts`、`stageLut`・`sampleLut`）を LUT の焼き込みに入れた。霧を `STAGE_FOG` とカメラの高さからの `fogExtinction(fog, ueZ(y))` に（色は露出で割らない）。DOF とモーションブラーは使うゾーンがあるときだけ作る（ステージは両方なし）。SSAO と SSR の既定の設定を `'surface'` に。`tests/ue-grade.test.ts` をステージの値に書き直した
- 2026-09-14: SSAO と SSR の材質のプラグインの GLSL のサンプラー（`sceneAoSampler`・`ssrDepthSampler`・`ssrColorSampler`）の宣言を `getUniforms().fragment` から `CUSTOM_FRAGMENT_DEFINITIONS` へ移した（UBO のある Windows などで宣言されずにシェーダーのコンパイルが失敗し、読み込みが止まっていた。Mac の Chrome は Babylon が UBO を切るので起きなかった）
- 2026-09-15: モーションブラーを戻した（ユーザーの指摘）: ステージのボリュームの MotionBlurAmount 0 はファンゲームのプレイヤーのための値なので `STAGE_VOLUMES` から外し、Dark Deception のプレイヤーの UE の既定 0.5 にした（「既知の制約」の「モーションブラーの量」）。`tests/ue-grade.test.ts` の期待値を 0.5 に
- 2026-09-15: 本家の新しい Steam 版（pak_reference_2）ではモーションブラーが撤廃されているが、ユーザーの決定で旧版（pak_reference）に従って残すことを「モーションブラーの量」に書いた（コードは変わらない）
