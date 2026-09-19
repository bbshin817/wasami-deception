---
title: 特殊シャード（スタンオーブと赤いシャード）
sources:
  - Content/Python/wasami_tools/pipeline/dd_specials.py
  - Source/wasami_deception/WasamiVignetteSidesWidget.h
  - Source/wasami_deception/WasamiVignetteSidesWidget.cpp
  - Source/wasami_deception/Tests/WasamiVignetteSidesTests.cpp
  - Source/wasami_deception/WasamiPowerOrb.h
  - Source/wasami_deception/WasamiPowerOrb.cpp
  - Source/wasami_deception/WasamiStunCollectEffect.h
  - Source/wasami_deception/WasamiStunCollectEffect.cpp
  - Source/wasami_deception/WasamiSpecialSpawnPoint.h
  - Source/wasami_deception/WasamiSpecialSpawnPoint.cpp
  - Source/wasami_deception/Tests/WasamiSpecialShardTests.cpp
updated: 2026-09-19
---

# 特殊シャード（スタンオーブと赤いシャード）

## 役割
本家の特殊シャード 2 種（最新版 `pak_reference_2` の `Blueprints/Main/BP_PowerOrb` = スタンオーブ、`BP_BonusShard` = 赤いシャード）。1 体ずつ置かれ、150 s ごとに 5 s 明滅して出現点を移る。オーブを取ると全敵が気絶し、赤いシャードを取ると 60 s 敵がタブレットの地図に出る（作業一覧の項目 10）。**作っている途中**: いまあるのは素材の取り込み、取得の画面 `UWasamiVignetteSidesWidget`、オーブ `AWasamiPowerOrb` とその取得の演出 `AWasamiStunCollectEffect`、出現点 2 種（赤いシャードと配置は進捗記録 `20260919-special-shards` のステップ 4・5）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_dd_specials()`（素材。`import_dd_shards` と `import_dd_gimmicks` の後。地図の印のマスターと粒子の材質を共有する）。
- `UWasamiVignetteSidesWidget`（`UUserWidget`）… 取得の画面（本家 `UI/Menu/Streaks/UMG_VignetteSides`）。`Show(WorldContext, Color, bText, TextToDisplay)`（本家の拾い物の `Create(Self, UMG_VignetteSides_C, None)` → 名前で `Color`・`Text?`・`TextToDisplay` → `AddToPlayerScreen(5)`。プレイヤーのコントローラが無いと出さない）・`Color`（既定 (1, 0.3952, 0)）・`bText`（`Text?`、既定 偽）・`TextToDisplay`・定数 `StunnedColor` (1, 0.4654, 0)・`RevealedColor` (1, 0, 0.0167)・`StunnedText()`「ENEMIES STUNNED」・`RevealedText()`「ENEMIES REVEALED」（オーブと赤いシャードが入れる値）・`AnimLength`（90001 / 60000 s）・`RemoveDelay` 2 s・`ZOrder` 5・static `LoadAssets(Out)`（絵と書体を読んで足す。オーブが `BeginPlay` で呼ぶ）・`Begin()`（Construct の頭から。テストが単独で呼ぶ）・`Advance(DeltaSeconds)`・`IsFinished()`・`GetTextBlock()`・`GetVignette()`・`Evaluate*`（`Anim` の 6 本の曲線）・素材の `VignetteTexture`（`T_VignetteNew`。`import_dd_powers` が取り込む）・`TextFont`。

- `AWasamiPowerOrb`（`AActor`）… スタンオーブ（本家 `Blueprints/Main/BP_PowerOrb`）。`ShardSpawnTime`（既定 150 s）・`SpawnPoints`（空なら `BeginPlay` でレベルの `AWasamiPowerOrbSpawnPoint` を全部。本家はエディタのボタン `Auto Assign Spawn Points` がクラスで集めて入れる）・`SpawnPowerOrb()`（明滅を始める。明滅中の呼び出しは無視）・`MoveToSpawnPoint(Index)`（明滅なしで点 `Index` へ。範囲外なら動かない。閃光と次の 150 s は同じ）・`Collect()`（1 回だけ）・static `StunAllEnemies(WorldContext)`・`IsFlickering()`・`GetTimeToSpawn()`（次の `SpawnPowerOrb` までの秒。無ければ −1）・`GetSpawnPoints()`・`GetCrystal()`・`GetMapMark()`・`GetCapsule()`・`GetLight()`・定数 `FlickerLength` 5・`FlickerPeriod` 0.1・素材（`CrystalMesh`・`CrystalMaterial`・`MapMarkMesh`・`MapMarkMaterial`・`DisappearFlash`・`AppearFlash`・`CollectImpact`・`PickupSound`・`CountdownSound`・`CollectShake`・`CollectEffectClass`）。デバッグのコンソールコマンド `Wasami.PowerOrb [N]`（レベルのオーブが今すぐ明滅を始める。N を付けると明滅なしで出現点 N へ）。
- `AWasamiStunCollectEffect`（`AWasamiSphereBurst`。04 記録）… オーブの取得の演出（本家 `Blueprints/Main/Powers/BP_StunCollectEffect`）。static `GrowthCurve()`・`DesaturationCurve()`・`OpacityCurve()`、定数 `StunRange` 4000。
- `AWasamiSpecialSpawnPoint`（抽象）と `AWasamiPowerOrbSpawnPoint`・`AWasamiBonusShardSpawnPoint` … 出現点（本家 `BP_PowerOrbSpawnPoint`・`BP_BonusShardSpawnPoint`）。`GetBillboard()`・`BillboardMaterial`・定数 `BillboardSize` 32。


## 内部構造と処理の流れ

### オーブ `AWasamiPowerOrb`
- 部品（本家の SCS のまま）: `DefaultSceneRoot`（Movable）→ `soul_shard`（`power_orb`・`m_crystal_Inst3`、相対 (0, 0, 125.25)・拡縮 0.5408、`NoCollision`）→ その子に `Capsule`（相対 (0, 0, 0.29)・拡縮 0.1・半径と半高 840.6 → 世界で 45.46 cm の球、中心 125.4 cm。`UShapeComponent` の既定の `OverlapAllDynamic`）と `PointLight`（相対 (−0.216, 0, −0.452)・拡縮 0.1、Movable、単位なし 1000・減衰 200・sRGB (255, 146, 0)〈書き出しの FColor は B, G, R, A〉・影なし）。ルートの子に `StaticMesh`（地図の印: `Plane`・`M_PowerOrb`、相対 (0, 0, 2000)・拡縮 (1.5, 1.5, 10)・影なし。当たりは本家どおりスタティックメッシュの既定〈シャードの印と違い、本家が切っていない〉）。メッシュと材質は `OnConstruction` で入れる。本家の `PPP_Collect_Shard` はどの流れも起こさないので作らない。本家の `NavArea_Obstacle` は、Pawn を止めない体はナビゲーションに入らないので効かない（写していない）。本家の Tick は `Float 3`（0）で結晶を回すだけなので Tick しない。
- `BeginPlay`: 取得と移動で使う素材（粒子 3・音 2・シェイク、演出の既定の素材、`UWasamiVignetteSidesWidget::LoadAssets`）を読んで持つ → 出現点が空ならレベルの `AWasamiPowerOrbSpawnPoint` を集める → `ShardSpawnTime` の繰り返さないタイマーで `SpawnPowerOrb`。
- `SpawnPowerOrb`（本家の同名のイベント）: `Flicker`（ルートの見え隠れを子ごと反転。反転の状態は明滅をまたいで残る。最初は隠れる）→ 0.1 s ごとの `Flicker` のタイマー → 5 s（本家の `Delay 5`）で: もう一度 `Flicker` → タイマーを消す → 見える → `MoveToSpawnPoint(RandomIntegerInRange(0, n − 1))`（今いる点も選ばれる。本家の「選び済み」の分岐は定数 False で切れている）。
- `MoveToSpawnPoint`: 結晶の位置に `P_ky_flash_PowerOrb_Disappear`（拡縮 0.5）→ 点の位置へ `SetActorLocation`（スイープなし・テレポート）→ `ShardSpawnTime` のタイマーを掛け直す → 新しい位置に `P_ky_flash_PowerOrb_Appear`（拡縮 0.5）。出現は 150 s + 5 s ごと（最初は 155 s）。
- 隠れている間もカプセルは残るので取れる。重なりは相手が `GetPlayerCharacter(0)` のときだけ `Collect`。
- `Collect`（本家の重なりの DoOnce の先）: 2 つのタイマーを消す → 結晶の位置に `P_ky_impact`（拡縮 1）→ `PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75)` → `UWasamiVignetteSidesWidget::Show(StunnedColor, 真, ENEMIES STUNNED)` → カメラマネージャーの `StartCameraShake(BP_CameraShake_Streak, 1, CameraLocal)` → `PlaySoundAtLocation(8-Dark_power_ball_countdown_, 原点, 1, 1)`（減衰の設定が無いのでどこでも同じに聞こえる。17.3 s）→ `AWasamiStunCollectEffect` を原点に出す → `StunAllEnemies` → `Destroy`。本家は `DestroyActor` を閃光の直後に呼んで続きを流す（本作は最後に呼ぶ。見た目は同じ）。ゲームインスタンスの `Used Stun Orbs?`（実績だけ）と `PrintString` は写さない。
- `StunAllEnemies`: タグ `Enemy` の全アクタのうち `IWasamiEnemyInterface` を実装するものに `SetState(Stun, true)`（本家は `GetAllActorsWithTag(Enemy)` → インターフェースへのキャスト）。距離も遮蔽も見ない。気絶の秒数とアニメは敵の側（07 記録）。
- 地図: プレイヤーの `MinimapActorClasses` の既定にオーブのクラスがあり、キャプチャがいつも写す（02 記録）。見え隠れで印も明滅する（本家どおり）。タグ `dd_minimap` を付けないのは、地図の板の組み立て（`place_dd_minimap`）がそのタグのアクタを消して置き直すため。
- テレキネシスのインターフェースを持たないので引き寄せられない。

### 取得の演出 `AWasamiStunCollectEffect`
本家の `BP_StunCollectEffect` は `BP_PrimalPower` と部品も流れも同じで（04 記録の `AWasamiSphereBurst`）、違いは値だけ: `Range` 4000、`PostProcess` の `ColorGain` (1.61, 0.9416, 0, 1)（橙。`PostProcess1` は Primal と同じ中間調 × 100・色収差 50）、球の MID の `Color` (0.258, 0.0737, 0)、波のピッチ 2、トラック `float`（0 → 1.977 s で 1.014 → 2.497 s で 1.079。2 s で半径 約 40.9 m）・`desaturation`（0.015 → 0.42 s で 0.080 → 1.92 s で 0.254）・`opacity`（1 → 0.75 s で 0.634 → 1.92 s で −0.002）。`float2` は Primal と同じキー。**敵を気絶させない**（オーブが全敵に送る）。オーブが原点に出し、`StartPower` がプレイヤーの位置へ動かす。2 s で消える。

### 出現点
本家の 2 つとも、ルートの `MaterialBillboard`（32 × 32 cm・ワールド空間・ゲーム中は隠れる）だけ。材質はオーブの点が `m_crystal_Inst3`、赤いシャードの点がエンジンの `VertexColorViewMode_RedOnly`（`OnConstruction` で入れる）。本体は点のアクタの位置を読む。

### 取得の画面 `UWasamiVignetteSidesWidget`
- 木（`RebuildWidget` が本家のスロットどおりに組む。連続回収の画面 `UWasamiShardStreakWidget` と同じ作り。13 記録）: ルート `CanvasPanel_0`（`HitTestInvisible`、平常の角度 −0.158°）→ `Image_161`（`T_VignetteNew` を 1024² のブラシで、アンカー全面・オフセット 0.96 / 0.54・整列 (0.5, 0.5)・自動の大きさ、`ColorAndOpacity` (1, 0.2308, 0, 0.4177)、平常の拡大 2）→ `TextBlock_47`（アンカー下中央、左 −72.96・上 −233.08、151 × 40、中央揃え。白 α 0.8、helvetica-neue-bold 35、縁取り 1 の黒 α 0.638〈影にも〉、平常の `RenderOpacity` 0。既定の文は本家の「ENEMIES STUNNED FOR 30 SECONDS」で、本家の拾い物はどれも上書きする）。
- `NativeConstruct` → `Begin`: `Text?` なら `TextBlock_47` に `TextToDisplay`、偽なら `Hidden`。続けて**ウィジェット全体**の `SetColorAndOpacity(Color)`（本家のバイトコードは対象なしの `SetColorAndOpacity` = `UUserWidget` のもの。文字と縁の色の両方に掛かる: オーブは橙の縁と橙の文字、赤いシャードは赤）。`Anim` を 0 から、2 s で `RemoveFromParent`。`NativeTick` → `Advance` が経過を進めて曲線を当てる（UMG のアニメは使わない。スレートの実時間で進むので `slomo` の影響を受けない）。
- `Anim`（1.5 s。キーは書き出しの値、接線は書き出しの自動の値を `WasamiWidgetAnimation` で）: 文字の拡大 4 → 0.1 s で 1 → 0.15 s で 1.1 → 0.25 s で 1、文字の不透明 0 → 0.1 s で 1 → 0.7 s で 1 → 1.5 s で 0（キーの間で自動の接線が 1 を少し越える。本家も同じ）、縁の拡大 1.4 から 0.35 s の 1.25 まで直線 → 0.9 s まで 1.25 → 1.5 s で 2、縁の α 0 → 0.15 s で 1 → 0.25 s で 0.5 → 0.9 s で 0.25 → 1.5 s で 0（色のほかのチャンネルにキーは無く、画像の (1, 0.2308, 0) のまま）、ルートの角度 0 → 0.1 s で 0.5° → 0.133 s で 0.5° → 0.15 s で −2° → 0.25 s で 0 と拡大 1 → 0.133 s で 1.05 → 0.467 s で 1。ルートの区間は 28000 ティック（0.467 s）で終わり、その後はルートの平常の値（WebGL 版と同じ読み）。アニメの後（1.5〜2 s）は最後の値のまま（UMG_ShardStreak と同じ。平常の値に戻さない）。

## 作るアセット
`WasamiDDTools.import_dd_specials`（`pipeline/dd_specials.py` の `import_all`）が作る。すべて `pak_reference_2` から。

| パス | 中身 |
| --- | --- |
| `/Game/DD/Meshes/Shared/power_orb` | オーブの本体（半径 約 44 cm の球。スロット 1 に `m_crystal_Inst3`〈本家の既定〉）。`dd_assets.static_mesh`（Nanite なし） |
| `/Game/DD/Meshes/Ring_Assets/soul_shard` | 赤いシャードの本体（2.2 × 1.8 × 8.6 cm。本家は 20 倍で置く）。スロットの本家の既定 `m_crystal_Inst1` は作らない（赤いシャードが `m_crystal_Inst` を当てる） |
| `/Game/Pipeline/Materials/M_DD_Crystal` | 本家の `m_crystal` の推定（下の「結晶の材質」） |
| `/Game/DD/Materials/Fords_Materials/m_crystal`・`m_crystal_Inst3`・`m_crystal_Inst` | 原作のパスの推定のインスタンスと、その子のオーブ用（橙: `color1` (0.526, 0.094, 0.047)・`emissive_col` (0.896, 0.226, 0)・`Fresnel Setting` (5, 0.592, 0)・`emissive_entensity` 29.9・`env_cubemap` `DefaultTextureCube`）と赤いシャード用（赤: `color1` (0.531, 0.009, 0)・`emissive_col` (0.156, 0, 0.013)・`emissive_entensity` 29.9）。`color2` はどちらも黒 |
| `/Game/DD/Materials/Shared/M_PowerOrb` | オーブの地図の印。`dd_shards` の `M_DD_MapMark` のインスタンス、`Color` (1, 0.2903, 0) |
| `/Game/Pipeline/Materials/M_DD_MapMarkMasked` | 形で切り抜く地図の印の推定のマスター: `Color` をベースカラーと自己発光に、`Mask`（`T_EnemyTriangle`）の R をマスクに（Masked、しきい 0.3333） |
| `/Game/DD/Materials/Shared/M_Bonus_Shard`・`M_Enemy` | 赤いシャードと、赤いシャードが地図に出す敵の印。`M_DD_MapMarkMasked` のインスタンス、`Color` (1, 0, 0) |
| `/Game/DD/Textures/Shared/T_EnemyTriangle` | 三角の形（1024²、G8、リニア） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Appear`・`_Disappear`・`P_ky_flash_BonusOrb_Appear`・`_Disappear` | 出現点を移るときの消える・現れる閃光。`dd_particles` が書き出しから組む |
| `.../Particles/P_ky_impact`・`P_ky_impact1` | オーブ・赤いシャードを取ったときの閃光（`P_ky_impact2`・`3` と同じ材質 2 つ） |
| `.../Textures/T_ky_flash01_4x4`・`T_ky_lensFlare01`・`T_ky_maskRGB3` | 閃光のテクスチャ（原作の設定のまま。`T_ky_maskRGB3` は AdvancedMagicFX13 のもの。除細動器の AdvancedMagicFX09 のものとは別） |
| `.../Materials/MI_ky_flare01b_primitiveG`・`R` | 消える閃光の星。`dd_shards` の推定の `M_ky_flare01_primitive` のインスタンス（`alphaDensity` 1.3 / 1.8・`baseTex` `T_ky_flash01_4x4`・`selectCh` G / R） |
| `/Game/Pipeline/Materials/M_DD_KyPrimitiveColor`・`M_DD_KyLensFlare02`、`.../Materials/M_ky_primitiveColor`・`MI_ky_primitiveColor`・`M_ky_lensFlare02` | 消える閃光の残りの材質の推定（下の「閃光の材質」）。`MI_ky_primitiveColor` は `useHilight` 真と両面の上書き |
| `/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_`・`Bonus_Shard_Pickup_v1`・`Stun_Wave_Attack_New_04` | オーブの取得・赤いシャードの取得・取得の演出の波の音 |

使うが、ここでは作らないもの: `Soul_Shard_Pickup_v2_Cue`（`import_dd_shards`）、`BP_CameraShake_Streak`・`01_Hotel_Lobby_ElevatorShakeStop`・`M_05_Primal`・`T_VignetteNew`（`import_dd_powers`）、`helvetica-neue-bold_Font`（`import_dd_tablet`）。`PPP_Collect_Shard` はどちらの BP の流れも起こさないので作らない。

### 結晶の材質（`m_crystal`。グラフは cook で消えている）
書き出しに残るのは出力の一部（金属・スペキュラ・自己発光。法線は未接続）、パラメータ、Noise 2 つの FeatureLevelSwitch、`BoundingBoxBased_0-1_UVW`、Custom を通して読むキューブ。コンパイル済みのシェーダー（`Tools/dd/cooked_shaders.py "Fords_Materials/m_crystal."` の SM5 のベースパス）を読んで、式をそのまま組んだ（不透明・ライトあり）:
- 自己発光 = max(0, Noise(反射ベクトル × 0.75 + 時間 × `emissive_speed`。3D テクスチャのグラディエント・乱流・4 段・−0.5〜0.5) × `emissive_col` × `emissive_entensity` + Fresnel(指数 5、基底 0.04) × `Fresnel Setting` + `Additive Emissive`)
- t = Noise(ワールド位置 × `tile_ratio` − 時間 × `emissive_speed`。テクスチャのシンプレックス・乱流・4 段・0〜1) + バウンディングボックスの Z（0〜1）− 0.5
- ベースカラー = saturate(lerp(`color2`, `color1`, t) + `env_cubemap` を refract(−カメラ, 法線, 0.66) の向きで × 0.5)（屈折は Custom `return refract(-V, N, 0.66);`）
- 金属 = t × 0.5、スペキュラ = t、粗さ = `roughness`（0.01）
- 推定で外したもの: シェーダーは反射と屈折の向きを `distortion_normal`（UV × 0.1 で読む）で曲げるが、推定は頂点の法線で読む（`CRYSTAL_LEFT_OUT`）。本家の親の `env_cubemap` は既定が空なので、推定の親は `DefaultTextureCube` を既定にした（赤いシャードはこれを継ぐ）。

### 地図の印の色
3 つとも書き出しは `Constant3Vector` を値なしで持つだけだが、コンパイル済みのシェーダーに定数が残っていた: `M_PowerOrb` (1, 0.2903, 0)、`M_Bonus_Shard`・`M_Enemy` (1, 0, 0)（`T_EnemyTriangle` の 1 チャンネルを 0.3333 で切る）。ついでに `M_Shard` の定数は (0.482481, 0, 1) と分かった（`dd_shards` は画面の実測で (0.70, 0.0071, 1.0) に合わせている。06 記録）。新しい 3 つはシェーダーの定数のままにし、タブレットでの見え方は見比べていない（作業一覧の項目 28 の後回し）。

### 閃光の材質（推定。グラフは cook で消えている）
- `M_ky_primitiveColor`: 残る式は `hilightPower`・`hilightColor`・`MF_ky_addHilight` と、自己発光（A = Add、B = 粒子の色）と不透明度（A = Multiply、B = 粒子の α）の静的スイッチ `useHilight`。シェーダーでは、スイッチ偽は不透明度 = 粒子の α を深さで 100 かけて消す。真（`MI_ky_primitiveColor`）は `T_ky_maskRGB3` の G と B を UV × 0.2 に時間 × (0.1, −1)・(−0.2, −2) で流して読み、n1 × n2 × (n1 + n2) × 2500 × `hilightPower` × `hilightColor` の光り。推定は真の側で粒子の色にこれを足し（Add）、不透明度は両側とも深さで消す粒子の α（Multiply の相手は分からない）。
- `M_ky_lensFlare02`: 自己発光 = 粒子の色。不透明度 = saturate(lerp(`remap1`, `remap2`, s(時間, 2)) × G^`alphaDensity`)。G は `T_ky_lensFlare01` を lerp(`rotRemap1`, `rotRemap2`, s(時間, 0.5)) × 0.25 だけ中心で回した UV で読む。s(時間, p) = (sin(2π sin(2π 時間 / p)) + 1) / 2（周期 p の Sine を `Sine_Remapped` に通したもの）。粒子の α は入らない。

## 原作データの根拠
- 本体・部品・流れ: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_PowerOrb.json`・`BP_BonusShard.json`、`_bytecode/.../BP_PowerOrb.txt`・`BP_BonusShard.txt`（`python Tools/dd/bp_flow.py`）。取得の演出は `Blueprints/Main/Powers/BP_StunCollectEffect`・`BP_BonusShardCollectEffect`。
- メッシュ: `_meshes.json` の `/Game/Meshes/Shared/power_orb`・`/Game/Meshes/Ring_Assets/soul_shard`。
- 材質: `_assets/.../Materials/Fords_Materials/m_crystal*.json`、`Materials/Shared/M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`・`M_Shard.json`、`ThirdParty/AdvancedMagicFX13/Materials/*.json` と、それぞれのコンパイル済みシェーダー（`Tools/dd/cooked_shaders.py`）。
- 取得の画面: `_assets/DDeception/Content/UI/Menu/Streaks/UMG_VignetteSides.json`（木・`Anim` のキー・区間）と `_bytecode/.../UMG_VignetteSides.txt`（Construct）。オーブと赤いシャードが入れる値は `BP_PowerOrb.txt` @1811〜・`BP_BonusShard.txt` @3341〜。
- WebGL 版: `.claude/references/webgl/implementation-records/08`（特殊シャード）・`10`（`UMG_VignetteSides`）・`11`（地図の印）。

## 依存関係
- `dd_assets`（音・テクスチャ・メッシュ・材質・インスタンス・推定の材質）、`dd_particles`（Cascade の粒子）、`dd_stage._Graph`（01 記録）
- `dd_shards` の `M_DD_MapMark` と閃光の材質（06 記録）、`dd_gimmicks` の `MI_ky_flare14R`（08 記録）
- 取得の画面: `WasamiWidgetAnimation.h`（曲線）、`WasamiAssets.h`、`import_dd_powers` の `T_VignetteNew`（04 記録）と `import_dd_tablet` の書体 `helvetica-neue-bold_Font`（03 記録）
- エンジン: `MaterialExpressionNoise`・`MaterialExpressionCustom`・`MaterialExpressionFresnel`・`MaterialExpressionRotator`
- オーブ: `AWasamiSphereBurst`（04 記録。取得の演出の基底）、`IWasamiEnemyInterface` とタグ `Enemy`（07 記録）、プレイヤーの `MinimapActorClasses`（02 記録）、`UWasamiVignetteSidesWidget`

## テスト（`Tests/WasamiSpecialShardTests.cpp`）
- `Wasami.PowerOrb.Parts`: 部品の値（結晶のメッシュ・材質・位置・拡縮・当たりなし、カプセルの 45.46 cm・中心・`OverlapAllDynamic`・Pawn の重なり・重なりの通知、灯の色・単位なし 1000・減衰 200・影なし、印の材質・20 m 上・拡縮・影なし）、`ShardSpawnTime` 150、テレキネシスのインターフェースを持たない、プレイヤーの地図がオーブのクラスを写す、出現点のビルボード（ルート・ゲーム中は隠れる・32 cm・`m_crystal_Inst3`）。
- `Wasami.PowerOrb.CollectEffect`: 演出の 4 本のトラック（書き出しのキーと接線から Python で計算した値）、橙のゲイン・閃光・`Range` 4000・球の色・ピッチ 2、Primal Fear は色を入れずピッチ 1。
- `Wasami.PowerOrb.Cycle`: 出現点 3 つ（と赤いシャードの点 1 つ。数えない）のテストのワールドで、150 s までは明滅しない → 明滅の間は置いた位置・次のタイマーなし・3 s で約 30 回反転・灯と印も一緒・カプセルは残る → 155 s の後は見えて出現点の 1 つにいて、移ってから 150 s → 2 巡しても出現点 → `MoveToSpawnPoint(1)`。0.05 s のティックは 150 s で少し遅れるので、見る時刻はタイマーから 0.5 s ずらす。
- `Wasami.PowerOrb.Collect`: ほかのキャラクターが触れても取れない、明滅で隠れている間にプレイヤーが触れると取れて消える、遠い敵も含めてインターフェースを持つ敵 2 体に `SetState(Stun, true)` が 1 回ずつ（タグだけのアクタには送らない）、演出が 1 つプレイヤーの位置に出て 2 s で消える。テストのワールドにはローカルプレイヤーが無いので、ENEMIES STUNNED は画面に出ない。

## 既知の制約・注意点
- 結晶と閃光の材質・地図の印の色は推定で、本家と見比べていない（大目標 1・2 の決め方。作業一覧の項目 28 の後回しの一覧）。
- オーブの地図の印（20 m 上の板）は本家どおり当たりを持つ（スタティックメッシュの既定）。上の階の床から 20 m 上に来ると見えない板としてぶつかりうるのも本家と同じ（配置はステップ 5 で確かめる）。
- Zone 2 の `m_crystal_Inst2`（ステージの小物）はステージの組み立てが汎用の `M_DD_Substance` のインスタンスで作っていて、ここの `m_crystal` とは別。

## 変更履歴
- 2026-09-19: オーブ `AWasamiPowerOrb`（本家 `BP_PowerOrb`）、取得の演出 `AWasamiStunCollectEffect`（`BP_StunCollectEffect`。Primal Fear と共有する球の基底 `AWasamiSphereBurst` を 04 記録に）、出現点 2 種、デバッグのコマンド `Wasami.PowerOrb`、テスト `Wasami.PowerOrb.*` を足した（ステップ 3）
- 2026-09-19: 初版（素材の取り込み `dd_specials.py` と `WasamiDDTools.import_dd_specials`。作業一覧の項目 10 のステップ 1）
- 2026-09-19: 取得の画面 `UWasamiVignetteSidesWidget`（本家 `UMG_VignetteSides`）とテスト `Wasami.VignetteSides.*` を足した（ステップ 2）
