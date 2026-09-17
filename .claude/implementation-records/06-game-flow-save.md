---
title: ゲームの流れ（シャード）
sources:
  - Source/wasami_deception/WasamiShard.h
  - Source/wasami_deception/WasamiShard.cpp
  - Source/wasami_deception/Tests/WasamiShardTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_shards.py
  - SourceArt/Wasami/wasami_mochi.glb
updated: 2026-09-17
---

# ゲームの流れ（シャード）

## 役割
本家のソウルシャード（最新版 `pak_reference_2` の `Blueprints/Main/BP_Shard`）。ステージに置かれ、触れると回収され（タブレットの数が 1 減る）、テレキネシスで引き寄せられる。見た目は本作のワサミ餅（ユーザーの決定）。チェックポイント・セーブ・ライフ・死亡・脱出（M3 の残り）はこの記録に書き足していく。

いまは**シャードの最小限**（タブレットのパワーの作業のステップ 9。`.claude/progress/20260916-tablet-powers.md`）: ゲームモードの `Check Shards`（`Collect Shard` の通知と連続回収の判定 `Check Streak`）、ゲームインスタンスの `Shards To Be Removed`（チェックポイントからのやり直しで取ったシャードを消す）、`bDisabled` と `Enable` はまだ無い。回収の閃光 `P_ky_flash3` はステップ 9b で足した。

## 公開インターフェース
- `AWasamiShard`（`AActor`、`IWasamiTelekinesisInterface` を実装）
  - `Collect(bool bNoSound)` … 本家の `Collect(NoSound?)`。1 回だけ（DoOnce）。下の「回収」。
  - `Activate`（インターフェース）… 本家の `Activate`。下の「引き寄せ」。
  - `IsPulling()`・`GetPullRate()`・`GetSpinRate()` … 確認用。
  - 静的関数: `EvaluatePullAlpha(Seconds)`（`Shard Pull` の `Alpha`）、`PullLocation(From, Player, Alpha)`（ExpoIn で水平だけ寄せた位置）、`SpinSpeed(PlayRate)`（餅の回る速さ °/s）。定数 `PullLength` = 1。
  - 部品: `DefaultSceneRoot`、`Body`（本家の `SkeletalMesh` の位置と拡縮だけを持つ `USceneComponent`）、その子の `Mochi`（`UStaticMeshComponent`）・`PointLight`・`Capsule`、ルートの子の `Plane`（ミニマップの印）。
  - 値: `LightIntensity` 175（本家の `Light Intensity`）、`MinimapPlaneHeight` 2000（`Minimap Plane Height`）。
  - 素材（ソフト参照、`WasamiAssets.h`）: `MochiMesh` `/Game/Wasami/Shard/SM_WasamiMochi`、`PlaneMesh` `/Engine/BasicShapes/Plane`、`MapMarkMaterial` `/Game/DD/Materials/Shared/M_Shard`、`PickupSound` `/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue`、`PickupConcurrency` `/Game/DD/Audio/OnlyFew`、`CollectShake` `/Game/DD/Blueprints/Shared/BP_CameraShake_ShardCollect`、`CollectFlash` `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3`。
- ツール: `WasamiDDTools.import_dd_shards()`（素材）、`WasamiStageTools.place_dd_shards(zone)`（配置。01 記録）。

## 内部構造と処理の流れ

### 部品（本家の `BP_Shard.json` の SCS）
- `Body` = 本家の `SkeletalMesh`: 相対位置 (0, 2.2888e−5, 97.0854)、拡縮 10。
  - `Mochi`: 拡縮 0.055（10 倍の下で 0.55 m。WebGL 版の `game.shard.size`）、当たりなし・ナビに関わらない・影なし、描画距離 3000（本家の `LDMaxDrawDistance`）。メッシュは構築時に入れる（`OnConstruction`）。材質はメッシュが持つ。
  - `PointLight`: 相対位置 (−0.2161, 1.6e−5, −0.4519)・拡縮 0.1、Movable、単位なし（UE 4.24 の既定。UE 5 は cd なので明示する）、強さ 175（構築時に `LightIntensity` を入れ直す）、減衰半径 200、`MaxDrawDistance` 1750・`MaxDistanceFadeRange` 1500、色 (194, 0, 255)（書き出しの FColor は B, G, R, A の順で (255, 0, 194, 255)）、影なし、`VolumetricScatteringIntensity` 2.5。餅の中に入る（影を落とさないので周りを照らす）。
  - `Capsule`: 相対位置 (0, −2.3e−6, 0.2915)・拡縮 0.1、半径と半高さ 49.5718（ワールドで半径 49.57 cm の球、床から約 100 cm）。`WorldStatic` にして `Custom` のプロファイル（UE 5.8 の `UShapeComponent` の既定 `OverlapAllDynamic` の応答 = 全チャンネル Overlap、QueryOnly）。重なりの開始をコンストラクタで結ぶ（本家の部品のバインドと同じく、生成時から）。
  - 本家の `PPP_Collect_Shard`（`bAutoActivate` 偽で、起動する処理がどこにも無い）は作らない。
- `Plane`: ルートの子、`/Engine/BasicShapes/Plane`、材質 `M_Shard`、拡縮 (1.5, 1.5, 10)、当たりなし・影なし。構築時に相対位置を (0, 0, `MinimapPlaneHeight`) にする（本家の構築スクリプトの `MakeVector(0, 0, Minimap Plane Height)`）。上向きの片面なので下からは見えず、プレイヤーのシーンキャプチャ（真上から、`ShowOnlyActors` にシャードが入る。02・03 記録）にだけ写る。地図の板（`dd_minimap`）はゾーンの床より下にあるので、20 m 上の印は地図の上に写る。

### BeginPlay
回収の音・同時発音・シェイク・閃光を読み込んで持つ。餅の再生速度 `SpinRate` を `RandomFloatInRange(0.05, 0.15)`（本家の結晶の `SetPlayRate`）、`PreviousLocation` を今の位置にする。

### 餅の回転（ティック）
本家の結晶はスケルタルのループアニメ `soul_shard_skeletal_anim_loop`（長さ 1.6667 s・`RateScale` 0.5・`OnlyTickPoseWhenRendered`）で回る。餅はスタティックメッシュなので、アクタのティックで `Mochi` のヨーを `SpinSpeed(SpinRate)` = 720 / 1.6667 × 0.5 × 再生速度（10.8〜32.4 °/s）ずつ増やす。スキンのメッシュと同じく、**最近 1 秒以内に描かれたときだけ**回す（`WasRecentlyRendered(1)`。UE 5.8 の `USkinnedMeshComponent` の `bRecentlyRendered` と同じ幅）。本家の `BP_Shard` 自身はティックを使わない（`bStartWithTickEnabled` 偽）ので、これは餅にしたための差。
- 回る向きと量: PSA の 2 本の骨がどちらも Z 軸まわりに 1 コマ 7° 回り、メッシュの頂点はすべて子の骨に付く。PSK/PSA はルートの骨の回転を W を反転して保存する（同じ書き出しの glTF の参照姿勢と比べて確かめた）ので、戻すと 2 本は同じ向きに回り、合成は 1 ループで 2 周・ヨーが増える向き。額面どおりに読むと 2 本が打ち消し合って回らない。

### 引き寄せ（`Activate` → `Shard Pull`）
- `Activate`: 再生速度 `PullRate` を `RandomFloatInRange(0.8, 1.2)` にし、位置 0 から再生する（UE の `FTimeline::PlayFromStart` と同じく、その場で位置 0 の更新を出す）。
- 更新: `SetActorLocation(VEase(PreviousLocation, (プレイヤーの X, プレイヤーの Y, PreviousLocation.Z), Alpha, ExpoIn), スイープ)`。`Alpha` は線形の (0, 0) → (0.75, 1)。プレイヤーの位置は毎回取り直す（動くプレイヤーを追う）。ExpoIn なので Alpha 0.5 で 1/32、0.9 で 1/2 しか寄らず、最後に一気に寄る。ルートは当たりを持たないのでスイープは何にも止まらない。プレイヤーがいなければ原点へ寄る（本家は `None` への呼び出しで 0 を読む）。
- ティック: 位置 += 経過 × `PullRate`。長さ 1 を**超えた**ティックで 1 に揃えて更新し、終わる（UE の `FTimeline` と同じ。`AWasamiPowerBurst` と同じ規則）。実時間では 0.625〜0.9375 s で届き、0.833〜1.25 s で終わる。
- 終わり: `Collect(false)`。**届いたかどうかに関係なく回収する**。途中でカプセルがプレイヤーに重なれば、その時に回収される。

### 回収（`Collect`）
1. DoOnce（`bCollected`）。
2. プレイヤー（`AWasamiPlayerCharacter`）とタブレットの画面（`GetTabletScreen`）が無ければここで終わる（本家のキャストの失敗と同じ。DoOnce は閉じたまま）。
3. 画面の数を `Clamp(数 − 1, 0, 9999)` にして（本家は文字列を整数にして引く）、`PlayCountShake()`（本家の `PlayAnimation(Count Shake, 0, 1, Forward, 2.0)`。03 記録）。
4. （本家のゲームモードの `Check Shards` はまだ無い。）
5. `ClientStartCameraShake(BP_CameraShake_ShardCollect, 0.4, CameraLocal)`。
6. `SpawnEmitterAtLocation(P_ky_flash3, Body の位置, 回転 0, 拡縮 0.2, 自動破棄, プールなし, 自動起動)`（本家は `SkeletalMesh` の `K2_GetComponentLocation`。`BP_Shard` @900〜@950）。部品はワールドの `WorldSettings` に付き、シャードの破棄の影響を受けない。エミッタの長さ（`RequiredModule` の既定の 1 秒）で終わって消える。（`NoSound` が偽のときの `Shards To Be Removed` はまだ無い。）
7. `PlaySound2D(Soul_Shard_Pickup_v2_Cue, NoSound ? 0 : 0.65, 1.0, 0, OnlyFew)` → `Destroy()`。本家は破棄してから鳴らすが、同じフレームなので聞こえ方は同じ。破棄の後のワールドの取り方を当てにしないよう、音を先にした。
- 重なりの開始（`OnCapsuleBeginOverlap`）: 相手がプレイヤー（`GetPlayerCharacter(0)`）なら `Collect(false)`。本家の重なりの経路は `NoSound` を書かずに回収へ飛ぶが、そこへ来るのは DoOnce が開いているとき（＝`Collect` がまだ呼ばれていない、`NoSound` が既定の偽）だけなので同じ。
- タブレットの数は、プレイヤーが 0.1 秒ごとにシャードのアクタを数え直す（02 記録。破棄されたアクタは数えない）。回収の直後の −1 は本家どおり画面に直接書く。

## 作るアセット
`WasamiDDTools.import_dd_shards`（`pipeline/dd_shards.py`）が作る。

| パス | 中身 |
| --- | --- |
| `/Game/Wasami/Shard/SM_WasamiMochi` | 餅のメッシュ（`SourceArt/Wasami/wasami_mochi.glb`、6,000 三角形、Nanite、スロット 1 に `MI_WasamiMochi`）。1 m の大きさで原点が中心 |
| `/Game/Wasami/Shard/T_WasamiMochi_BaseColor`・`_MetallicRoughness`・`_Normal` | glb に埋め込まれた 1024² の JPEG を `Intermediate/Pipeline/wasami/shard/` に書き出して取り込む。ベースカラーは sRGB、金属と粗さはリニア、法線は `TC_Normalmap`・`TEXTUREGROUP_WorldNormalMap`・緑を反転（glTF の法線は Y が上向き、UE は下向き） |
| `/Game/Pipeline/Materials/M_DD_WasamiMochi`、`/Game/Wasami/Shard/MI_WasamiMochi` | glTF の金属・粗さの材質（係数はすべて 1）: ベースカラー、金属 = B、粗さ = G、法線、両面。自己発光 = ベースカラー × `Glow`（0.3。WebGL 版の `game.shard.glow`） |
| `/Game/Pipeline/Materials/M_DD_MapMark`、`/Game/DD/Materials/Shared/M_Shard` | 地図の印の推定のマスターと、その原作のパスのインスタンス。`Color` をベースカラー（シーンキャプチャが読む）と自己発光に出す。`M_Shard` の色は (0.70, 0.0071, 1.0)（下の「印の色」） |
| `/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2`・`Soul_Shard_Pickup_v2_Cue` | 回収の音（0.43775 s）と、その Cue（`SoundNodeModulator` のピッチ 0.9〜1.1、音量は既定の 0.95〜1.05 → `SoundNodeWavePlayer`）。`dd_assets.sound_cue`（01 記録） |
| `/Game/DD/Audio/OnlyFew` | 同時発音（`MaxCount` 1・`StopOldest`・`VolumeScale` 0.5） |
| `/Game/DD/Blueprints/Shared/BP_CameraShake_ShardCollect` | 回収の揺れ（0.1 s、ブレンドアウト 0.05 s、ロール 1.5°・FOV 3° を周波数 15 で、ほかは振幅 0） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3` | 回収の閃光（下の「回収の閃光」）。`dd_particles` が書き出しから組む（01 記録） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Textures/T_ky_flare01`・`T_ky_flareVertical02`・`T_ky_decoLinesB_sml`・`T_ky_deco_rainbow` | 閃光のテクスチャ（原作の設定のまま） |
| `/Game/Pipeline/Materials/M_DD_KyFlare01Primitive`・`M_DD_KyPrimitive`・`M_DD_KyPrimitiveDyn2`・`M_DD_KyPolarGlow02`・`M_DD_KyEmpty` | 閃光の材質の推定のマスター（下の「閃光の材質」） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Materials/M_ky_flare01_primitive`・`M_ky_primitive`・`M_ky_primitive_dyn2`・`M_ky_polarGlow02`・`M_ky_empty` | 原作のパスに置く推定のマスターのインスタンス（原作の既定値のうち、作ったパラメータ） |
| `.../Materials/MI_ky_flare01_primitiveG`・`MI_ky_flare01_primitiveR`・`MI_ky_primitive2_trs` | 原作のインスタンス（親は上の原作のパスのもの。`alphaDensity` 1.3・`baseTex` `T_ky_flare01`・静的マスク `selectCh` G / R、`alphaValue` 2・`depthFade` 0・`radius` 0.3・`radiusDensity` 1.6）。ブレンドとシェーディングの上書きは親と同じなので写さない |

配置: `WasamiStageTools.place_dd_shards` / `build_dd_stage_level` が、病院のレベルに `AWasamiShard` を本家の位置に置く（Zone 1 は 337、Zone 2 は 342。タグ `dd`・`dd_shard`、フォルダ `Hospital/Gameplay/Shards`、ラベルは本家の名前）。シャードの灯は部品なので、前処理の灯の一覧のうち `BP_Shard_C` のものは単独では置かない（01 記録）。

### 回収の閃光（`P_ky_flash3`、AdvancedMagicFX13。両版で同じ）
エミッタ 7（どれも `bUseLegacySpawningBehavior`、詳細度 High まで〈UE 5.8 で Epic を足す〉、LOD 3〈距離 0 / 2500 / 5000〉。値は書き出しの参照表のまま。大きさは拡縮 0.2 を掛けた値）:
| エミッタ | 材質 | 中身 |
| --- | --- | --- |
| shockwave | `M_ky_primitive_dyn2` | 0 s と 0.02 s に 1 つずつ、寿命 0.1 s、120 cm まで広がる輪（`SizeMultiplyLife`）、色は白 → 紫、α 0.5 → 0.1、動的パラメータ `dynOutDen` 1・`dynInR` 0.2・`dynInDen` 3 |
| glowSub | `MI_ky_flare01_primitiveG`（柔らかい丸） | 1 つ、寿命 0.15 s、140 cm（`Size` 2 つの和）、紫 (0.259, 0.047, 0.757)、α 0 → 0.79 → 0 |
| decoCore | `MI_ky_flare01_primitiveR`（星） | 1 つ、寿命 0.1 s、160 cm、紫、α 0 → 0.095 → 0、乱数の回転 |
| core | `MI_ky_flare01_primitiveR` | 1 つ、寿命 0.1 s、40 cm、明るい (2.94, 2.25, 5.0)、α 1 → 2 → |
| glow | `M_ky_polarGlow02` | 0 s と 0.02 s に 1 つずつ、寿命 0.1 s、70 × 70 cm（`PSA_Rectangle`。大きさの表は Y も 350）、色 (6.75, 6.56, 7) → 白、α 5 → 0。`RequiredModule` の `bEnabled` は偽だが、UE 5.8 は LOD の `bEnabled` だけを見るので描かれる（`ParticleEmitterInstances.cpp`。LOD へ写す処理は `ParticleLODLevel.cpp` でコメントアウト） |
| dust_line | `MI_ky_primitive2_trs` | 35 本（LOD 1 は 20）、寿命 0.08〜0.15 s、半径 60〜70 cm の球面から中心へ速さ ×(−8〜−2)、速さの向きの細長い板（`SizeScaleBySpeed` で縦 1〜3 倍） |
| light | `M_ky_empty`（見えない） | 1 つ、寿命 0.1 s、`ParticleModuleLight`（逆二乗でない指数 16、明るさ 2.5、半径は大きさ 20 cm × 8）で紫 (0.319, 0.130, 1.0) の灯 |

### 閃光の材質（推定。グラフは cook で消えている）
共通: 半透明・Unlit・スプライトとメッシュの粒子用。原作のインスタンスと粒子が使う静的スイッチの側だけを作る（`dd_shards.py` の各ビルダーの説明に、残っていた式と推定を書いた）。
- `M_DD_KyFlare01Primitive`: 自己発光 = 粒子の色（残っている）。不透明度 = 静的マスク `selectCh` で選んだ `baseTex` の 1 チャンネル × `alphaDensity` × 粒子の α を `DepthFade`（`depthFade`）。`useFresnel` の真の側（`fresPower`・`fresDensity`）は作らない。
- `M_DD_KyPrimitive`: 自己発光 = 粒子の色。不透明度 = `RadialGradientExponential(radius, radiusDensity)` × `alphaValue` × 粒子の α を `DepthFade`。`useFresnel`・`useTexColor`・`useDistanceSize` の真の側（ノイズ `T_ky_noise6` など）は作らない。
- `M_DD_KyPrimitiveDyn2`: 自己発光 = 粒子の色（残っている）。不透明度 = saturate(外の勾配〈関数の既定の半径、密度 `outDensity` + `dynOutDen`〉 − 内の勾配〈半径 `inR` + `dynInR`、密度 `inDensity` + `dynInDen`〉) × 粒子の α を `DepthFade`。**動的パラメータは足すと見た**（材質の既定値が 0 で、掛けると外の密度 0 で何も描かれないため）。この値では縁だけが 0.24 ほど残る薄い輪になる。
- `M_DD_KyPolarGlow02`: `VectorToRadialValue`（角度、中心からの距離 × 2）で、虹 `T_ky_deco_rainbow` を (角度 × `polarUV`, 距離 × (`polarUV_density` + `polarUV_den`) + `baseOffsetY` + 動的 `baseOffsetY`) に読んで ^`texPower` × `texDensity` × 粒子の色（`useBaseTexColor`）を自己発光に、線のノイズの α を (角度 + `noiseU` + 時間 × `noiseXspd`, 距離 × `noisePolarUV_density` + `noiseV` + 時間 × `noiseYspd`) に読んで × `noiseDensity` ^`noisePower`、× 輪 `RGE(maskRadiusOut, maskRadiusOutDensity)` × (1 − `RGE(maskRadiusIn, maskRadiusInDensity)`)、× 上下の縁の `saturate((1 − |2V − 1|) × topAndUnderMask)`、× 粒子の α を `DepthFade` で不透明度に。`noisePolarUV`・`noisePolarUV_val` の使い道は分からないので使わない。
- `M_DD_KyEmpty`: 不透明度 0。書き出しは式が 1 つあったことだけを残す（cook は不透明度の入力を残さない）。何もつながないと既定の不透明度 1 で黒い板になるが、エミッタ `light` は灯のためにあるので、見えない材質と見た。

### 印の色
原作の `M_Shard` の定数は cook で消えている。WebGL 版は原作の参考画像（ホテルの地図）で印を `#d21ee6` (210, 30, 230) と測った（画面に出た色）。タブレットの画面はワールドに置いたウィジェットなので、地図はステージのトーンマップを通って表示される（地図の線の色はこの経路のまま最新版の実機と一致している。03 記録）。そこで PIE でタブレットを上げ、印の表示色を測りながら `Color` を合わせた（2026-09-17）: sRGB (210, 30, 230) をそのままリニアにした (0.6445, 0.0130, 0.7913) は (205, 9, 206) と表示され、(0.70, 0.0071, 1.0) は (211, 29, 217) と表示された。青はベースカラーの上限 1 で 217 までしか上がらない（トーンマップの肩）。緑は赤と青につられて動く（色ごとに独立ではない）。

## 原作データの根拠
- 処理: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_Shard.txt`（回収 @55〜@1576、引き寄せ @1581〜@1999、BeginPlay @2121〜@2455、重なり @2456、`Activate` @2610、終わり @2540、構築スクリプト）。調査のまとめは `.claude/references/powers/03-telekinesis-vanish.md` §2.7・§2.8。
- 部品とタイムライン: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_Shard.json`（`*_GEN_VARIABLE`、`Shard Pull_Template`、`CurveFloat_0`、`Default__BP_Shard_C`）。
- 結晶のアニメ: `pak_reference_2/_assets/DDeception/Content/Meshes/Ring_Assets/soul_shard_skeletal_anim_loop.json`（`SequenceLength` 1.6667・`NumFrames` 51・`RateScale` 0.5）、`_anims_psa/Meshes/Ring_Assets/soul_shard_skeletal.psk`・`soul_shard_skeletal_anim_loop.psa`、`_meshes_gltf/Meshes/Ring_Assets/soul_shard_skeletal.gltf`（参照姿勢）。
- 音・同時発音・揺れ: `_assets/.../Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue.json`・`Soul_Shard_Pickup_v2.json`（両版で同じ。ogg の md5 も一致）、`Audio/OnlyFew.json`（最新版だけ）、`Blueprints/Shared/BP_CameraShake_ShardCollect.json`（両版で同じ）。
- 閃光: `_bytecode/.../BP_Shard.txt` @900〜@1019（`SpawnEmitterAtLocation`）、`_assets/DDeception/Content/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3.json`（両版で同じ。違いは `FixedRelativeBoundingBox` の詰め物のバイトだけ）、`.../Materials/M_ky_*.json`・`MI_ky_*.json`・`Mfunction/MF_ky_VectorToRadialValue.json`（説明文が UE の `VectorToRadialValue` と同じ）、`_textures.json`。UE 5.8 の `RadialGradientExponential` と `VectorToRadialValue` の出力は、レンダーターゲットに描いて数値で確かめた（2026-09-17。前者は中心 ≈ 1 − exp(−密度) から半径で 0、密度 0 で全面 0。後者の `Radial Coordinates` は R が角度 0〜1、G が中心からの距離 × 2）。
- 地図の印: `_assets/.../Materials/Shared/M_Shard.json`（残る式は `Constant3Vector_0` 1 つ、Emissive に接続、値なし）。色は WebGL 版の `.claude/references/webgl/implementation-records/11-minimap.md`。
- 配置: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`・`06_Hospital_Zone_02.full.json` の `BP_Shard_C`（どれも回転・拡縮なし、`bDisabled` なし）→ 前処理の `stage_ue.json` の `actors`。
- 餅: WebGL 版の `public/assets/models/wasami_mochi.glb`（`scripts/prepare-shard-model.mjs` が原本の 300 万三角形・2048² から作ったもの）と `src/world/shards.ts`・`src/config.ts` の `game.shard`（大きさ 0.55・`glow` 0.3）。浮き沈み・脈動は WebGL 版の表現で本家に無いので採らない。
- UE 5.8: `ShapeComponent.cpp`（`OverlapAllDynamic`、`AreaClass` の既定）、`SkinnedMeshComponent.cpp`（`bRecentlyRendered` の 1 秒）、`Timeline.cpp`（`PlayFromStart` の更新）、`KismetMathLibrary`（`VEase`）、`GameplayStatics.h`（`PlaySound2D` の `bIsUISound` 既定 真。UE 4.24 も真）。

## 依存関係
- 自前: `AWasamiPlayerCharacter`（`GetTabletScreen`、02 記録）、`UWasamiTabletWidget`（`GetShardCount`・`SetShardCount`・`PlayCountShake`、03 記録）、`IWasamiTelekinesisInterface`（04 記録）、`WasamiAssets.h`（00 記録）。取り込みは `dd_assets`・`dd_stage`・`paths`（01 記録）。
- 使う側: プレイヤーの `ShardActorClass`（既定がこのクラス。数と地図、02 記録）、レベルの組み立て（01 記録）、テレキネシス（`AWasamiTelekinesisPower::PullShards` が半径の中のシャードに `Activate` を呼ぶ。04 記録）。
- エンジン: `UCapsuleComponent`・`UPointLightComponent`・`UStaticMeshComponent`、`UGameplayStatics`（`PlaySound2D`・`GetPlayerCharacter`・`GetPlayerController`）、`UKismetMathLibrary::VEase`、`FRichCurve`、`APlayerController::ClientStartCameraShake`。

## 既知の制約・注意点
- **見た目は原作と違う**（ユーザーの決定。`.claude/guides/original-fidelity.md`）。大きさ・位置・回転の速さ・灯は原作の値に合わせ、材質は餅のテクスチャ（推定なし）に WebGL 版の自己発光を足したもの。回る速さは実機で確かめていない（ステップ 11）。
- 原作の結晶の `Material`（`m_crystal_Inst1`）は、餅がメッシュの材質を持つので使わない。
- `M_Shard` の色は推定（原作の値は cook で消えた。上の「印の色」）。病院の実機の地図にシャードが写る場面をまだ撮っていない。
- 回収の音の同時発音は、2 つ目で 1 つ目が止まらない（上の「確かめたこと」）。
- 回転のティックは 340 個ほどのシャードすべてで走るが、描かれていないシャードは回転を書かない。
- ゲームモードの `Check Shards`、`Shards To Be Removed`、`bDisabled`/`Enable` は未実装（上の「役割」）。
- **閃光の材質は推定**（上の「閃光の材質」）。見え方は本家と見比べていない（ステップ 11）。
- **エディタで最初の閃光だけ、描画が約 0.6 秒止まる**。初めて使う粒子の材質のシェーダー（3 件）をエディタがその場でコンパイルするため（ログの `LogShaderCompilers` のジョブ 0.5〜0.6 秒）。ゲームの時間は止まらず、2 回目からは止まらない。パッケージしたゲームではシェーダーが先に作られているので起きないはず。
- 本家の数の読み取りは文字列を整数にする（`Conv_StringToInt`）ので、1,000 以上で桁区切りが入ると 1 と読む癖がある。本作は整数を持つので起きない（病院は 342 以下）。
- テストのワールドにはプレイヤーがいないので、回収はタブレットが無いところで止まる（本家と同じ）。回収の結果は PIE で確かめる。

## 確かめたこと（2026-09-17、PIE、Zone 1 の −Y へ延びる廊下。シャード 331・330・4・_2・5 が X≈0 に並ぶ）
- 配置: Zone 1 に 337、Zone 2 に 342。単独で置いていたシャードの灯は外れ、Zone 1 の灯は 1,120 → 783。シャードの部品は Movable なので焼き込みはそのまま。PIE の開始でシャード 337・画面の数「337」。
- 見た目: 廊下の中央に餅が並んで見え、近づくとワサミの顔の餅（glb の +Y の暗い面が上。WebGL 版と同じ向き）。描かれている 3 つのヨーを 3.5 秒おいて読むと、毎秒 27.3・23.1・16.1°（範囲 10.8〜32.4 の中）。地図の印は本編では描かれない（`WasRecentlyRendered` が偽）。
- 地図: タブレットを上げると、廊下に並ぶシャードが紫の四角で出る（上の「印の色」）。
- 触れて回収（前進の入力で歩いてシャード 331 へ）: プレイヤーの中心がシャードから 97 cm（カプセルの半径の和 99.6 cm）に来たフレームで、アクタが消え、画面の数が 337 → 336。同じフレームの `Count Shake` は約 0.03 秒ぶん進んだ値（数の移動 (−8.4, 6.2)・拡縮 1.06・閃きの α 0.236。プレイヤーの画面の更新が回収より後のフレーム順のため）で、0.09 秒後に数の変換が元の (0, −12)・1.0 に戻り、α は 0 のまま。揺れは FOV が最大 +0.78°・ロールが最大 0.39° で、0.1 秒で 0 に戻った。
- 音（`ListWaves`）: `Soul_Shard_Pickup_v2` が 1 つ、音量 0.47〜0.51 で鳴る。0.65 × Cue の既定の `VolumeMultiplier` 0.75（UE 5.8 も 0.75。原作の書き出しは既定と同じ値を省くので原作も 0.75。書き出した Cue 126 個のうち 20 個だけが別の値を持つ）× Modulator の音量 0.95〜1.05 = 0.46〜0.51 と合う。
- 引き寄せ（プレイヤーは (0, −157) に立ったまま、4.4 m 先の 330 と 10.4 m 先の 4 に Python から `activate()`）: どちらも初めはほとんど動かず（0.35 秒で 8 cm と 52 cm）、最後に一気に寄り、プレイヤーに触れた所で回収された（4 が 0.64 秒、330 が 0.84 秒。再生速度の乱数で遠い方が先に着いた）。高さは 0 のまま。数は 336 → 335 → 334。
- **同時発音の差（未解決）**: 0.2 秒差で 2 つ回収すると、1 つ目の音は止まらず音量が 0.5 倍（0.24）になって鳴り続け、2 つが重なった（2 回試して同じ）。`OnlyFew` は `MaxCount` 1・`StopOldest`・`VolumeScale` 0.5 を原作どおりに写してあり、UE 5.8 の `SoundConcurrency.cpp` を読む限りは古い方が止まるはず。原因は特定していない。テレキネシスでまとめて回収したときに聞こえ方が変わりうるので、ステップ 10・11 で本家と聞き比べる。2026-09-17（ステップ 10a）: テレキネシスで 8 個が約 0.24 秒の間に回収されたとき、8 つの音がどれも止まらず、音量が新しい順に 0.47・0.24・0.13・0.06・0.03・0.01・0.01・0.00（新しい音が来るたびに古い音が 0.5 倍）で重なった。本家の `OnlyFew` の書き出しは `MaxCount` 1・`StopOldest`・`VolumeScale` 0.5 だけで、ほかの値（`VoiceStealReleaseTime` など）は既定のまま。

- 回収の閃光（2026-09-17、同じ廊下。Python から 4 m 先のシャードの `collect`、続けて歩いて 2 つに触れる。60 fps の gdigrab の収録と毎フレームの記録）: 回収のフレームに `P_ky_flash3` の部品が `WorldSettings` に 1 つ出て、位置は結晶の位置（高さ 97.1 cm）、拡縮 0.2、約 1.0 秒で消えた（3 回とも）。4 m 先からは、虹色の星と半透明の円（最初の絵）、紫の衝撃波の円・明るい中心の星・横の帯・床を照らす灯が 3〜4 フレームで消え、床の明るさが少し残った。歩いて触れたときは、閃光がカメラの約 1 m 先・60 cm 下に出るので、画面の下に紫の衝撃波の弧と細い光の筋が 5 フレームほど見えた。最初の 1 回だけ描画が約 0.6 秒止まった（上の「既知の制約」）。

## テスト（`Tests/WasamiShardTests.cpp`）
- `Wasami.Shard.PullCurve` … `Alpha` の値（0 / 0.375 / 0.75 / 1 秒）、ExpoIn の位置（Alpha 0 で元の位置、0.5 で 1/32、0.9 で 1/2、1 でプレイヤーの X・Y、高さは元のまま）、回る速さ（0.05 で 10.8、0.15 で 32.4 °/s）。
- `Wasami.Shard.Actor` … 一時的なゲームのワールドに置いて、閃光の既定のパス（`CollectFlash`）、カプセル（半径と半高さ 49.57、高さ約 100 cm、`WorldStatic`・`Custom`・QueryOnly・Pawn とワールドへ Overlap・重なりのイベントあり）、灯（位置・強さ 175・単位なし・半径 200・色・影なし・Movable）、餅（0.55 m・97.085 cm・描画距離 3000）、印（20 m 上・拡縮・当たりなし）、再生速度の範囲。`Activate` でその場の位置の更新、再生速度の範囲、0.45 の時点でわずかにしか寄らないこと、終わりに原点へ着いて止まること、プレイヤーがいないので破棄されないこと。
- `Wasami.Tablet.CountShake`（03 記録）。

## 変更履歴
- 2026-09-17: 回収の閃光 `P_ky_flash3` を足した（`Collect` の揺れの後に出す。素材の取り込みは `dd_shards.py` のテクスチャ 4・推定のマスター 5・インスタンス 8・粒子 1）
- 2026-09-17: 初版（シャードの最小限: `AWasamiShard`〈ワサミ餅・灯・カプセル・地図の印・回転〉、回収、引き寄せ、素材の取り込み `dd_shards.py`、配置、テスト）
