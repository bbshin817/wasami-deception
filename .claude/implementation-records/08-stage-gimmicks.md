---
title: ステージの仕掛け（両開き扉・ゾーンの障壁・打たれた閃き）
sources:
  - Source/wasami_deception/WasamiDoubleDoors.h
  - Source/wasami_deception/WasamiDoubleDoors.cpp
  - Source/wasami_deception/Tests/WasamiDoubleDoorsTests.cpp
  - Source/wasami_deception/WasamiZoneBarrier.h
  - Source/wasami_deception/WasamiZoneBarrier.cpp
  - Source/wasami_deception/Tests/WasamiZoneBarrierTests.cpp
  - Source/wasami_deception/WasamiHitFX.h
  - Source/wasami_deception/WasamiHitFX.cpp
  - Source/wasami_deception/Tests/WasamiHitFXTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_gimmicks.py
updated: 2026-09-19
---

# ステージの仕掛け（両開き扉・ゾーンの障壁・打たれた閃き）

## 役割
病院のステージで動く仕掛け。いまは本家の両開き扉と、打たれたときの画面の閃き `BP_HitFX`（`Blueprints/Shared`）を写した `AWasamiHitFX`（画面全体が赤く滲んで 0.35 s で晴れ、カメラが揺れる。独房の棘が届いたときに流れが出す。作業一覧の項目 6 のステップ 7a。罠とナースも使う）と、ゾーンの障壁 `BP_ZoneBarrier`（`Blueprints/Main`）を写した `AWasamiZoneBarrier`（全回収まで道をふさぐ光る壁。流れが `Destroy` で壊す。作業一覧の項目 6 のステップ 4a）。両開き扉は本家の `BP_06_DoubleDoors`（`pak_reference_2` の `Blueprints/06_Hospital`）を写した `AWasamiDoubleDoors`: 幅 400 cm の出入口の両端に蝶番のある扉 2 枚で、キャラクター（プレイヤーかナース）が前の箱に入ると奥へ、後ろの箱に入ると手前へ、音とともに 1 s で 90° 開き、両側を覆う `Leave` の箱からキャラクターが出て、プレイヤーが中に残っていなければ閉じる。閉ざされている（`bLocked`）と、入ってもガタつく音（2 s に 1 回まで）だけ。ゾーンの流れ（11 記録）が名指しする扉を `Lock`・`Unlock`・`Open Front`・`Force Close` で閉ざし・開ける。作業一覧の項目 6（ゾーンの進行）のステップ 3c で、流れが名指しする Zone 1 の 2 枚のために作った（残りの 60 枚を置くのは項目 8、Zone 2 の 1 枚は項目 13）。

## 公開インターフェース
- `AWasamiDoubleDoors`（`AActor`）
  - `OnOpen`（動的マルチキャスト。本家の `Open`: どちらかの側が開いた）、`OpenAmount`（本家の `Open Amount`、90°）、`bLocked`（流れが直に書く）。
  - `Lock()`・`Unlock()`・`OpenFront()`・`ForceClose()`・`UpdateAnimationSpeed(Speed)`（両方のタイムラインの速さ。病院では呼ぶ所が無い）。
  - `NotifyFrontEnter(Other)`・`NotifyBackEnter(Other)`・`NotifyLeave(Other)` … 前の箱に入った・後ろの箱に入った・`Leave` から出た（本家の 3 つの部品のイベント）。箱の重なりが呼ぶ。テストは直接呼ぶ。
  - 読むだけ: `AnySideOpen()`・`IsOpenFront()`・`IsOpenBack()`・`IsOpening()`・`IsClosing()`（タイムラインが走っているか）・`IsLockedSoundBlocked()`・部品 `GetStaticMesh()`・`GetStaticMesh1()`・`GetFrontEnter()`・`GetBackEnter()`・`GetLeave()`。
  - 静的: `EvaluateSwing(Seconds)`（タイムラインの Float の曲線）、定数 `SwingLength` 1・`CloseSoundTime` 0.5922・`LockedSoundDelay` 2。
- `AWasamiZoneBarrier`（`AActor`）
  - `Layer1MinBrightness` 5・`Layer1MaxBrightness` 9.5（前の板 `StaticMesh1` の `Emissive Pulse Min / Max`）、`Layer2MinBrightness` 0.8・`Layer2MaxBrightness` 1.2（後ろの板 `StaticMesh`）、`bSound` 真（唸りの音）。
  - `DestroyBarrier()` … 本家の `Destroy`（`AActor::Destroy` と名前がぶつかるので改名）。
  - 読むだけ: `GetStaticMesh()`・`GetStaticMesh1()`・`GetPointLight()`・`GetAudio()`。
- `AWasamiHitFX`（`AActor`）
  - `ShakeScale` 2・`PlayRate` 1（本家の `Shake Scale`・`Play Rate`）、`ShakeClass`（`BP_04_BossFight_CameraShake_Initial`）、定数 `TimelineLength` 5。
  - `HitEffect()` … 本家の `Hit Effect`（揺れとタイムラインを頭から。`BeginPlay` がする）。`GetPostProcess()`。

## 内部構造と処理の流れ
- 部品（本家の SCS）: ルート `DefaultSceneRoot`、`StaticMesh`（x −200）・`StaticMesh1`（x +200）は `UStaticMeshComponent` の既定（BlockAllDynamic・Movable）でナビゲーションに入らない。`FrontEnter`（(0, −150, 50)・拡縮 (6.2587, 2.8200, 1)）・`BackEnter`（(0, 150, 50)・(6.2587, 2.9723, 1)）・`Leave`（(0, 0, 50)・(6.2587, 12.0652, 1)）は `UBoxComponent` の既定（32 cm・OverlapAllDynamic・ゲームで隠れる）を拡縮した箱（約 400 × 180、400 × 190、400 × 772 cm）。箱の `AreaClass`（NavArea_Obstacle）は `bCanEverAffectNavigation` 偽なので効かない。クラスのタグ `interact`（プレイヤーの「使えるもの」を見る処理が読む。項目 5 の視線の手）。
- **メッシュはクラスが入れない**（`/Game/DD` の素材はコンストラクタで読まない。`WasamiAssets.h`）。レベルの組み立て（`dd_level._flow`、01 記録）が、本家の SCS の `hospital_entrance_walkway_doubledoor2`（`StaticMesh`）・`doubledoor1`（`StaticMesh1`）と、そのメッシュの材質（`M_06_Hospital_Door_01`・`MM_Main_Substance_Glass_Doors`）を置いた扉に入れる。メッシュはステージの素材（complex-as-simple の当たり）。
- 前の箱に入る（本家 @475）: 相手を覚え（`Unlock` が使う）、`ACharacter` なら @550: 閉ざされていれば `LockedSound`、どちらかが開いていれば何もしない、そうでなければ `bOpenFront` 真・`Animation` 真 → `OpenDoor` → `OnOpen`。`OpenFront` は @550 から。
- 後ろの箱に入る（@1143）: `ACharacter` でなければ何もしない。閉ざされていれば `LockedSound`、開いていれば何もしない、そうでなければ `bOpenBack` 真・`Animation` 偽 → `OpenDoor` → `OnOpen`。
- `Leave` から出る（@889）: 相手を覚え（`Lock` が使う）、`ACharacter` で、`Player Overlapping?`（`Leave` に重なるプレイヤー。閉ざされていれば常に偽）が偽なら @1054: どちらかが開いていれば、開いていた側を偽にし（前なら `Animation` 真、後ろなら偽）`CloseDoor`。`ForceClose` は @1054 から。UE は重なりの一覧から外してから終わりのイベントを流すので、出ていくプレイヤーは数えない。
- `Lock`（@1333）: `bLocked` 真 → `Leave` から最後に出た相手で @889（閉ざしたので `Player Overlapping?` は偽: 相手がキャラクターなら開いた扉が閉じる。まだ誰も出ていなければ何もしない）。`Unlock`（@1359）: `bLocked` 偽 → 前の箱に最後に入った相手で @475（キャラクターなら前から開く）。**本家はこの 2 つで前のイベントの引数（ユーバーグラフの持続フレーム）をそのまま使う**ので、弱い参照で覚えて同じにした。
- `OpenDoor`（@665）: `PlaySoundAtLocation(SFX_06_DoubleDoor_Open, アクタの位置, 回転 0, 0.5, 1.2, 0, MonkeyAttenuation)` → `Timeline_0` を頭から。`CloseDoor`（@766）: 両方の扉の当たりを QueryAndPhysics（既定のまま。病院で当たりを切る所は無い）→ `Timeline_1` を頭から。
- `LockedSound`（@253）: DoOnce。`PlaySoundAtLocation(Locked_Door, アクタの位置, 回転 0, 0.5, 1.5, 0, 01_Lobby_Attenuation)` → 2 s の Delay で DoOnce を開け直す（タイマー）。
- タイムライン（`Timeline_0` 開く・`Timeline_1` 閉じる。どちらも 1 s、同じ Float の曲線 0 → 0.951（0.552 s）→ 1.040（0.685 s）→ 1（1 s）。書き出しのキーと UE 4.24 が出した接線のまま、`RCTM_Break` で入れて UE 5 に接線を計算し直させない）。UE の `FTimeline` を写した自前の小さな状態（位置と再生中か）をアクタのティックで進める（走っている間だけティックする）: `PlayFromStart` は位置 0 で更新を 1 回（イベントは出さない）してから再生。ティックで位置を `Δt × 速さ` 進め、1 を超えたら 1 で止め、閉じる側は `Sound` のキー（0.5922 s）が [前の位置, 新しい位置)（最後のティックは終わりを少し越す）に入ったら `PlaySoundAtLocation(SFX_06_DoubleDoor_Close, …, 0.5, 1.0, 0, MonkeyAttenuation)`、そして `Update Func`。両方が走るときは開く側 → 閉じる側の順（閉じる側が勝つ）。
- `Update Func`（開く / 閉じる、曲線の値 α）: 振れ幅 = `Animation` ? `OpenAmount` : −`OpenAmount`。開くときは `Lerp(0, 振れ幅, α)`、閉じるときは `Lerp(振れ幅, 0, α)` を `Apply Update Values`: `StaticMesh` の相対回転をヨー v、`StaticMesh1` を −v（前から開くと 2 枚とも +Y 側 = 後ろへ振れる）。

### ゾーンの障壁（`AWasamiZoneBarrier`）
- 部品（本家の SCS）: ルート `DefaultSceneRoot`（拡縮 3.2。置いた障壁はアクタの拡縮で大きさを決める: Zone 1 (3.2, 3.904, 1.504) = 幅 390・高さ 150 cm、Zone 2 (3.2, 3.820, 3.2)）、`StaticMesh1`（エンジンの `/Engine/BasicShapes/Plane`、回転 (P 0, Y 90, R −90) で立てて x を向く）、`StaticMesh`（同じ Plane、x −3.04・回転 (P −90, Y −359.98, R 359.98)・拡縮 1.071）。板は当たりを Custom（WorldStatic、応答は既定の Block のまま、QueryAndPhysics）にし、ナビゲーションに入らない。タグ `interact`（アクタ・ルート・板 2 枚）。`PointLight`（Movable・Unitless 2500・(255, 0, 188)・`SourceRadius` 214.338・`SoftSourceRadius` 1000・`AttenuationRadius` 400・`VolumetricScatteringIntensity` 0）。`Audio`（`Barrier_Loop`・音量 0.5・ピッチ 0.8、減衰の上書き: 遮蔽あり・複雑な当たりで遮蔽・低域 1500 Hz・NaturalSound・`FalloffDistance` 2000。ほかは `FSoundAttenuationSettings` の既定）。本家の `Box`（NavArea_Obstacle の重なりの箱）は何も遮らずナビゲーションに入らないので写さない。
- **板の材質はクラスが入れない**。レベルの組み立て（`dd_level._flow`、01 記録）が `MM_ZoneBarrier_Inst1`（`StaticMesh1`）・`_Inst2`（`StaticMesh`）を置いた障壁に入れる。Plane はエンジンの素材なのでコンストラクタで読む。
- `BeginPlay`: 音と粒子を読み（ソフト参照）、本家の構築スクリプト（各板の `SetScalarParameterValueOnMaterials` で `Emissive Pulse Max / Min` を層の明るさに）をここでする（エディタで動的な材質をレベルに保存しないため）。`bSound` なら `Barrier_Loop` を入れて鳴らし、偽なら `Audio` を消す（本家の `ReceiveBeginPlay` @772。本家は部品が自動で鳴り出す）。
- `DestroyBarrier`（本家 @802）: `SpawnEmitterAtLocation(P_ky_impact3, StaticMesh の位置, 回転 0, 2, 自動で消える, プールなし, 起きた状態)` → `PlaySoundAtLocation(Barrier_Shatter, アクタの位置, 回転 0, 1, 1, 0, 01_Lobby_Attenuation)` → アクタを消す。
- 写さないもの: 見て左クリック（本家の `InteractWithObject` @787: `NoInteract` でなければ 5 s に 1 回、`DD_RingBarrierDenied_louder`〈0.75・1.1・`DialogueAttenuation`〉と `UMG_TextPrompt`「Collect all soul shards in this zone to break the barrier.」）は、視線の手のマークと一緒に項目 13 で作る。`Set Visibilty`（音量・見え方・板の当たりを切り替え、消すときは粒子と音）・`Spawn Particle`・`Off By Default`・`NoInteract` は、病院の 2 つの障壁が既定のままでレベル BP も呼ばないので写さない。

### 打たれた閃き（`AWasamiHitFX`）
- 部品（本家の SCS）: `DefaultSceneRoot`、`PostProcess`（`UPostProcessComponent` の既定 = 境界なし〈どこに出しても画面全体〉、`BlendWeight` 0、上書き `ColorGain` (1, 0.21828, 0.180477, 1)・`SceneFringeIntensity` 10・`ChromaticAberrationStartOffset`〈値は既定のまま上書きだけ真。本家どおり〉）、`Timeline_0`（`UTimelineComponent`）とその曲線 `CurveFloat_0`（0 s で 1〈Cubic・User の接線 −5.21370792388916 / −5.213721752166748〉→ 0.35 s で 0〈Linear〉。書き出しの値のまま入れて接線を計算し直させない）。
- `BeginPlay`（本家 @184）: タイムラインの更新（`BlendWeight` = 値）と終わり（アクタを消す）を結び、長さを 5 s（**Blueprint のタイムラインの既定。C++ の `FTimeline` の既定は最後のキーまでなので `TL_TimelineLength` を入れる**）にして `HitEffect`: プレイヤーのコントローラーの `ClientStartCameraShake(ShakeClass, ShakeScale, CameraLocal, 回転 0)` → `SetPlayRate(PlayRate)` → `PlayFromStart`（位置 0 の更新で `BlendWeight` 1 から）。0.35 s で晴れ、5 s で消える。
- 本家で出すのは Zone 2 の `Spikes_Death`（11 記録）、ナース（`BP_06_ReaperNurse`・`BP_ReaperNurse_IntroAI`）、罠（`BP_06_Defib`・`BP_06_sawTrap_medium`）ほか。いま出すのは流れの `OnSpikesDeath` だけ。

## 作るアセット
`pipeline/dd_gimmicks.py`（`WasamiDDTools.import_dd_gimmicks`。01 記録）: `/Game/DD/Audio/06_Hospital/SFX_06_DoubleDoor_Open`（0.622 s）・`SFX_06_DoubleDoor_Close`（1.007 s）、`/Game/DD/Audio/01_Hotel/Locked_Door`（SoundCue。Random に `Locked_Door_v1`・`_v2`、減衰 `MonkeyAttenuation`、1.343 s）とその波形 2、減衰 `/Game/DD/Audio/Misc/MonkeyAttenuation`・`/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation`。扉のメッシュと材質はステージの素材。

障壁（`import_zone_barrier`。`import_dd_shards` の後）: `/Game/DD/Audio/02_School/Barrier_Loop`・`Barrier_Shatter`、テクスチャ `/Game/DD/Textures/02_ElementarySchool/school_decal_speedBarrier_01_A`（Mirror 貼り）・`/Game/DD/ThirdParty/AdvancedMagicFX13/Textures/T_ky_flare14_4x4`、材質 `/Game/DD/Materials/Shared/MM_SpeedBarrier`（cook で式が消えたので、焼き込みの半透明のベースパスのシェーダー〈`Tools/dd/cooked_shaders.py "Materials/Shared/MM_SpeedBarrier." --show 26`〉を読んだ式で組んだ: UV を中心から `Min Scale`〜`Max Scale`〈Time × `Size Pulse Speed` の正弦〉で拡縮し、背後の景色が `FadeDistance Secondary` 離れているところほど拡縮を効かせ〈DepthFade で Lerp〉、`Texture` の RGB × `Color Multiplier` を色として、その `Emissive Pulse Min`〜`Max` 倍〈Time × `Emissive Pulse Speed` の正弦〉を発光、2 倍をベースカラーに、A を `FadeDistance` の DepthFade × `Opacity Multiplier` を不透明度に。半透明・ライティングあり・被写界深度の前〈本家の `bEnableSeparateTranslucency` 偽〉。インスタンスが持つ `Color + Emissive Multiplier` はシェーダーに無い〈何も読まない〉ので作らない）とそのインスタンス `MM_ZoneBarrier_Inst1`・`_Inst2`（両面の上書き）、粒子 `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_impact3`（`dd_particles`。材質の `MI_ky_primitive2_trs` と推定の `M_ky_flare01_primitive` はシャードの閃光のもの〈06 記録〉、`MI_ky_flare14R` はその新しいインスタンス〈加算の上書き〉）。

扉が破られるとき（`import_doors_busted`。流れの `BreakDoorsIn` が鳴らし・起こす。11 記録）: 音 `/Game/DD/Audio/06_Hospital/DD_TT_Door_BustedOpen_02`、粒子 `/Game/DD/ThirdParty/BallisticsVFX/Particles/Destruction/Fractures/V2/Fracture_concrete_3`（エミッタ 4: GPU の `Fragments`〈石の破片 100、衝突あり〉・`DustTrail`、CPU の `Smoke2`・`Dust`。レベルのエミッタ `Fracture_concrete_5` の粒子。組み立てが置く。01 記録）とテクスチャ 6（`Fragments/Textures/Stones2x2`・`Gravel2x2_normal`、`SmokeDust/Textures/whisp_One_512_8x8`・`_Normal`・`whisp_redux_2048_12x12`・`whisp_redux_2048_normal`）、材質（cook で式が消えたので推定。`dd_assets.estimated_materials`、推定のマスターは `/Game/Pipeline/Materials/M_DD_WhispDirectional`・`M_DD_WhispAmb`・`M_DD_Debris`）: `SmokeDust/whispOne_Master_directional`（焼き込みの影のパスで読める不透明度 = `Base` の A × `Opacity` × 粒子の A を `Fade Distance` で深さに溶かし × `Master Opacity`。色は `Base` の RGB × `Base Overlay` × 粒子の色、法線は `Normal` を `FlattenNormal` で平らに寄せる。カメラの数 cm 以内だけを薄める `Radius` とマクロ UV のノイズは作らない）とそのインスタンス `Whisps_trans`・`Whisps_trans2`、`whispOne_Master_amb`（同じ形で色は `ColourOverlay`。インスタンスが切る `CamFade` とその `Radius`・`Hardness` は作らない）とインスタンス `whispOne_Master_amb_Inst`、`Fragments/DebrisMaster`（`Base Map` の RGB を `Desat` で灰色に寄せ × 粒子の色、不透明度は `Base Map` の A × 粒子の A、法線 `Normal Map`）。どれもライティングありの半透明（ボリューム・方向あり）で、サブ UV の段階は粒子の SubUV のまま（`Linear_Blend` は段階の間を混ぜる）。

Zone 2 のリフト（`import_lifts`）: 音 3 と減衰 2。中身は 12 記録。ガレージリフト（`import_garage_lift`）: 骨入りのメッシュとアニメ（`dd_skeletal`。01・12 記録）。

Zone 2 の独房（`import_cell`。`import_doors_busted` の後。流れの `OnSpikesDeath` が鳴らす。11 記録）: 音 `/Game/DD/Audio/06_Hospital/DD_Needle_Trap_R1_V3`（1.117 s）。打たれた閃きの揺れ `BP_04_BossFight_CameraShake_Initial` はシーケンスの取り込み（`dd_sequence.CAMERA_SHAKES`、01 記録）が作る。シーケンスが起こすレベルのエミッタの粒子 3 つ（`CELL_PARTICLES`。エミッタはシーケンスの取り込みが本家の位置に置き、テンプレートを入れる。01 記録）: `/Game/DD/Blueprints/Characters/Nurse/P_06_NurseSparks`（`06_Hospital_Zone2_Spikes` の頭の 4.27 s に廊下の床を −14170 → −12410 へ動きながら 0.6 s ごとに赤い火花。エミッタ `P_06_NurseSparks_24`、拡縮 2、自動で起きる）、`/Game/DD/ThirdParty/BallisticsVFX/Particles/Destruction/Fractures/V2/Fracture_dark_slow`（同じシーケンスの 58.8 s。独房の床の 3 m 下の `Dirt_impact_2_large_57`、拡縮 5。黒い塵 10 粒、CPU）、`…/Impacts/LegacyFX/Small-Medium-Large/Concrete/Concrete_impact_large`（`06_Hospital_Zone2_Cell_DoorPicked` の頭。独房の扉の `MetalDull_impact_Dyn_27`。火花・煙・破片のエミッタ 5、うち GPU 2）。テクスチャ 4（`Textures/FX_Textures/dust`〈パッケージの中の名前は `dust_0`〉、`Flares/Textures/Flare_white`、`SmokeDust/Textures/Squib_one_1024_8x8`・`Squib_one_normal`）。材質 4 は cook で式が消えたので、焼き込みの半透明のベースパスのシェーダーを読んだ式で推定（`CELL_MATERIALS` を `dd_assets.estimated_materials` に。推定のマスターは `/Game/Pipeline/Materials/M_DD_NurseSparks`・`M_DD_BvfxSpark`・`M_DD_BvfxRadialGradient`・`M_DD_Squib`、原作のパスにそのインスタンス）: `Blueprints/Characters/Nurse/M_06_NurseSparks`（加算・ライティングあり・被写界深度の前。ベースカラーと発光 = 粒子の RGB、不透明度 = `dust` の A の 50 乗 × 粒子の A を深さ 50 で溶かし × `CameraDepthFade`〈長さ 400・始まり 24〉）、`Flares/M_Spark`（加算・Unlit・半透明の影なし。発光 = `Flare_white` の RGB × 粒子の RGB、不透明度 = 両者の A の積）、`Flares/M_Radial_Gradient`（半透明・Unlit・レスポンシブ AA・ビーム用。発光 = 粒子の RGB、不透明度 = max(1 − 2 × UV の中心からの距離, 0) の 4 乗 × 粒子の A。本家の `RadialGradient` の呼び出しの結果を式で書いた）、`SmokeDust/Squib_one`（半透明・ライティングあり〈ボリューム・方向あり、方向の強さ 0.4 ほか影の値は書き出しから〉・球状の法線。ベースカラー = `Base` の RGB × 粒子の RGB〈`Base` は粒子の SubUV の段階の UV〉、法線 = `Squib_one_normal` の段階を混ぜて `FlattenNormal` で平らに寄せる、不透明度 = `Base` の A × 粒子の A × `Opacity` を `Fade Distance` で深さに溶かし × カメラの近くで薄め〈25 cm で 0、250 cm で 1。`CameraDepthFade` の長さ 225・始まり 25。何も切らない静的スイッチ `Cam close fade?` は作らない〉× `MasterOpacity`）。

駐車場のナースが扉を突くとき（`import_nurse_door_hit`。`import_doors_busted` の後。`AWasamiEnemy06Chase::HitFX` が鳴らし・出す。07 記録）: SoundCue `/Game/DD/Audio/01_Hotel/20-Elevator_Slams`（`20-Elevator_Slams_V3`・`_V1`・`_V2` のランダムをモジュレータに通す。ピッチ 1.8・`01_Lobby_Attenuation`。3 本の音も作る）、粒子 `/Game/DD/Particles/06_Hospital/P_06_NurseDoorHit`（スプライトのエミッタ 1、テクスチャ `whisp_One_512_8x8`）とその材質 `/Game/DD/ThirdParty/BallisticsVFX/Particles/FXMaterials/SmokeDust/Whisps_additive`（`Whisps_trans` のインスタンスで、本家の `BlendMode` の上書き〈加算〉と `Fade Distance` 120 を写す。`Radius` は推定の材質に無いので落とす〈`SMOKE_LEFT_OUT`〉）。暗いトンネルでは塵はほとんど見えない（2026-09-19 の PIE。本家の見え方は大目標 3）。

## 原作データの根拠
- `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_DoubleDoors.txt`（上の番地）と `_assets/…/BP_06_DoubleDoors.json`（SCS の部品の位置・拡縮・メッシュ、`Open Amount` 90、タグ `interact`、`Timeline_0_Template`・`Timeline_1_Template`〈長さ 1、`CurveFloat_0_1`・`CurveFloat_0_1_3` のキー、`Sound` のイベントのキー `CurveFloat_0`〉、部品のイベントの結び付け `ComponentDelegateBinding_0`）。
- 音: `_assets/…/Audio/06_Hospital/SFX_06_DoubleDoor_Open.json`・`_Close.json`、`Audio/01_Hotel/Locked_Door.json`（Random・重み 1 × 2・`AttenuationSettings` `MonkeyAttenuation`）、`Audio/Misc/MonkeyAttenuation.json`。
- 障壁: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_ZoneBarrier.txt`（上の番地）と `_assets/…/BP_ZoneBarrier.json`（SCS の部品・クラスの既定 `Layer 1/2 | Min/Max Brightness`・`Interaction Text`・`bSound`・タグ）、`_assets/…/Materials/Shared/MM_SpeedBarrier.json`（残った式の値）・`MM_ZoneBarrier_Inst1.json`・`_Inst2.json`、`ThirdParty/AdvancedMagicFX13/Materials/MI_ky_flare14R.json`・`Particles/P_ky_impact3.json`。レベル BP が呼ぶのは `Destroy` だけ（Zone 1 は `05 All Shards Collected` @16409、Zone 2 は @21485。11 記録）。
- 扉が破られるとき: `_assets/…/Audio/06_Hospital/DD_TT_Door_BustedOpen_02.json`、`ThirdParty/BallisticsVFX/Particles/Destruction/Fractures/V2/Fracture_concrete_3.json`、`…/FXMaterials/SmokeDust/whispOne_Master_directional.json`・`whispOne_Master_amb.json`・`Whisps_trans.json`・`Whisps_trans2.json`・`whispOne_Master_amb_Inst.json`、`…/Fragments/DebrisMaster.json`（残った式の設定と引数）、`python Tools/dd/cooked_shaders.py "SmokeDust/whispOne_Master_directional." --show 3`（影のパスの不透明度）。
- 打たれた閃き: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Shared/BP_HitFX.txt`（上の番地）と `_assets/…/BP_HitFX.json`（クラスの既定 `Shake Scale` 2・`Play Rate` 1、`PostProcess_GEN_VARIABLE` の設定、`Timeline_0_Template`〈長さの指定なし = 既定の 5 s〉と `CurveFloat_0`）。独房の音は `_assets/…/Audio/06_Hospital/DD_Needle_Trap_R1_V3.json`。独房の粒子は `pak_reference_2/_sequences/06_Hospital_Zone2_Spikes.json`・`_Cell_DoorPicked.json`（粒子のトラックの鍵と火花の動き）、`_levels/06_Hospital_Zone_02.full.json` の 3 つのエミッタの `ParticleSystemComponent0`（テンプレート・`bAutoActivate`・位置と拡縮）、`_assets/…/P_06_NurseSparks.json`・`Fracture_dark_slow.json`・`Concrete_impact_large.json`、材質の残った式と設定 `M_06_NurseSparks.json`・`Flares/M_Spark.json`・`M_Radial_Gradient.json`・`SmokeDust/Squib_one.json`、式は `python Tools/dd/cooked_shaders.py "Nurse/M_06_NurseSparks." --show 28`・`"Flares/M_Spark." --show 5`・`"Flares/M_Radial_Gradient." --show 5`・`"SmokeDust/Squib_one." --show 28`（半透明のベースパス。`--show 3` の影のパスも同じ不透明度）。
- 置き場所と値: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`（前処理の `stage_ue.json` の `actors`。両開き扉は Zone 1 に 62 枚、Zone 2 に 1 枚〈`bLocked` 真〉。障壁は両ゾーンに `BP_ZoneBarrier_2` が 1 つずつ、アクタの値は既定のまま）。子の `BP_06_DoubleDoors_Child`（1 枚扉 `hospital_entrance_walkway_singledoor`・箱の位置違い）は病院に置かれていない。

## 依存関係
- 自前: `AWasamiPlayerCharacter`（`Player Overlapping?` の相手のクラス。02 記録）、`WasamiAssets.h`。
- 使う側: ゾーンの流れ（`AWasamiZoneFlow::DoubleDoors`・`ZoneBarrier`、`AWasamiZone1Flow` の 04・05・06。11 記録）、レベルの組み立て（`dd_level._flow`。01 記録）。
- エンジン: `UPostProcessComponent`・`UTimelineComponent`・`UCurveFloat`・`UBoxComponent`・`UStaticMeshComponent`・`FRichCurve`・`UGameplayStatics::PlaySoundAtLocation`・`SpawnEmitterAtLocation`・`FTimerManager`・`UPointLightComponent`・`UAudioComponent`、エンジンの素材 `/Engine/BasicShapes/Plane`（100 × 100 の板、厚さ 0 の箱の当たり）。

## 既知の制約・注意点
- 置いてあるのは流れが名指しする Zone 1 の 2 枚だけ（`BP_06_DoubleDoors11`・`BP_06_DoubleDoors33_36`）。ほかの出入口は扉が無く、通り抜けられる（項目 8）。
- 扉は当たりを持ったまま掃引せずに回る（本家どおり）。開くときにプレイヤーが扉の振れる範囲（蝶番から 200 cm）にいると、扉がカプセルに食い込むことがある。
- `Unlock`・`Lock` は前のイベントの相手で本家の処理を繰り返す（上）。流れは `On04DoorBreak` で `Unlock` を使わず `bLocked` を直に書く（本家どおり）。
- 両方のタイムラインが同時に走ったときの勝ち方（閉じる側が後）は、本家では部品のティックの順で決まり、コードからは確定できない。
- 扉の破片 `Fracture_concrete_3` の材質は推定（上）。GPU のエミッタ 2 つは cook の焼き込みの表から分布を作り直して組む（01 記録）が、cook の表はその 2 つでは本家が使った値と合わない（`DustTrail` の色は表が 1 → 0.36、cook の GPU のデータは一定の 0.078。大きさも表の上限 1 に対して約 6 倍）ので、煙と破片の見え方は本家とずれる（作業一覧の後回しの一覧）。
- 独房の粒子の材質 4 つは推定（上）。`Fracture_dark_slow` の黒い塵は PIE で 10 粒が描かれている（`stat particles`）が、暗い色（0.053）と薄い不透明度（`whisp_redux_2048_12x12` の A の平均 0.1 × `Master Opacity` 0.5）で、暗い廊下ではほとんど見えない。本家でもエミッタは独房の床の 3 m 下にある（作業一覧の後回しの一覧）。
- 障壁の粒子 `P_ky_impact3` の `MI_ky_flare14R` は推定の `M_ky_flare01_primitive` のインスタンス（06 記録の閃光と同じ推定）。Zone 2 の障壁はまだ壊す所が無い（項目 13 の、欠片の画面が閉じたときの `Ring Piece Collect `）。

## テスト（`Tests/WasamiDoubleDoorsTests.cpp`）
`Wasami.DoubleDoors.Actor`: 曲線のキー（0・0.951・1.040・1）、部品の値（`interact`・90°・扉の位置と当たり・箱の大きさと位置と Pawn の重なり）、キャラクターでないと開かない、前から開いて 0.5 s で曲線どおり・1 s で 90°（2 枚は逆向き）、開いている間は後ろから開かない、`Leave` を出ると閉じ始めて 1 s で 0、後ろから開くと −90° / +90°、`Force Close`、閉ざすとガタつくだけ・2 s で次が鳴らせる・`Unlock` で前にいた者のために開く、`Lock` で開いた扉が閉じる、閉ざしたまま `Open Front` は開かない・`bLocked` 偽なら開く、`Update Animation Speed(2)` で 0.5 s で閉じる。`Wasami.ZoneBarrier.Actor`（`Tests/WasamiZoneBarrierTests.cpp`）: 部品の値（`interact`・ルート 3.2・板 2 枚がエンジンの Plane で x を向き、後ろの板が 3.04 × 3.2 cm 後ろ・Block の当たり・ナビゲーションに入らない・灯・音の音量とピッチと減衰・`Barrier_Loop`）、`DestroyBarrier` でアクタが消え、`P_ky_impact3` が後ろの板の位置に 2 倍で出る。流れから壊すのは `Wasami.ZoneFlow.Zone1`（11 記録）。テストのワールドの 0 秒のティックは `MinUndilatedFrameTime`（0.5 ms）進むので、途中の角度は 0.5005 s の値と比べる。流れから閉ざし・開けるのは `Wasami.ZoneFlow.Zone1`（11 記録）。`Wasami.HitFX.Actor`（`Tests/WasamiHitFXTests.cpp`）: 境界なし・色・滲み・上書き・`Shake Scale` 2・`Play Rate` 1、出てすぐ `BlendWeight` 1、0.1 s で 0 と 1 の間、0.4 s で 0・まだ残る、5 s で消える。流れから出すのは `Wasami.ZoneFlow.Zone2`（11 記録）。

## 確かめたこと（2026-09-18、PIE）
Zone 1 の 04: エレベーターの前の `BP_06_DoubleDoors11` は赤い 2 枚扉で閉じている → 鍵が外れると手前へ 1 s で開き、少し行き過ぎて戻る → `Leave` の外へ出ると閉じる（11 記録の「確かめたこと」）。

障壁（項目 6 のステップ 4a）: Zone 1 の 05 で `(0, −18450)` から −Y を向くと、エレベーターホールの先の出入口を紫の網目の障壁がふさいでいる → `Wasami.CollectShards` で全回収すると、紫の閃光と放射する線（`P_ky_impact3`）が出て障壁が消え、奥の廊下が見える（収録 `Intermediate/DesktopAgent/shots/barrier_break.mkv`、git の外）。デバッグの全回収は 337 個を一度に消すので、閃光の直後に約 0.6 s 止まる。

独房の粒子（項目 6 のステップ 7b）: Zone 2 の 7 を開いてすぐ `(−12500, 1100)` から −X を向くと、廊下の床を赤い火花が 0.6 s ごとに近づいてくる（`P_06_NurseSparks`）。独房の扉の前で鍵を破ると、扉が開くと同時に白い火花が弾け、黒い煙と破片が落ちる（`Concrete_impact_large`。収録 `Intermediate/DesktopAgent/shots/cell_sparks3.mkv`・`cell_door2.mkv`、git の外）。

打たれた閃き（項目 6 のステップ 7a）: Zone 2 の 7 で独房に残ると、開いてから約 19 s で棘の箱が頭に届き、画面全体が赤く滲んで色がずれ、揺れでぶれる → 約 0.3 s で晴れる → 0.5 s 後に死亡画面（収録 `Intermediate/DesktopAgent/shots/spikes_death.mkv`、git の外。グリッド `Intermediate/Overnight/cell_grid.png` の下の段）。

## 変更履歴
- 2026-09-18: 初版。本家の `BP_06_DoubleDoors` を `AWasamiDoubleDoors` に写し、音の取り込み `dd_gimmicks.py` を足した（作業一覧の項目 6 のステップ 3c）
- 2026-09-18: 本家の `BP_ZoneBarrier` を `AWasamiZoneBarrier` に写し、`dd_gimmicks.import_zone_barrier`（音・テクスチャ・シェーダーから組んだ `MM_SpeedBarrier` とインスタンス・`P_ky_impact3`）を足した（作業一覧の項目 6 のステップ 4a）
- 2026-09-18: `dd_gimmicks.import_doors_busted`（トンネルの扉が破られるときの音と、破片の粒子 `Fracture_concrete_3`・テクスチャ・推定の材質）を足した（作業一覧の項目 6 のステップ 4b）
- 2026-09-18: 本家の `BP_HitFX` を `AWasamiHitFX` に写し、`dd_gimmicks.import_cell`（独房の棘の音）を足した（作業一覧の項目 6 のステップ 7a）
- 2026-09-18: `dd_gimmicks.import_cell` に独房のシーケンスが起こす粒子 3 つ（`P_06_NurseSparks`・`Fracture_dark_slow`・`Concrete_impact_large`）とテクスチャ 4・推定の材質 4 を足した（作業一覧の項目 6 のステップ 7b）
- 2026-09-18: `dd_gimmicks.import_lifts`（Zone 2 のリフトの音 3 と減衰 2。12 記録）を足した（作業一覧の項目 6 のステップ 8a）
- 2026-09-18: `dd_gimmicks.import_garage_lift`（ガレージリフトの骨入りのメッシュとアニメ。`dd_skeletal`。12 記録）を足した（作業一覧の項目 6 のステップ 8b1）
- 2026-09-19: `dd_gimmicks.import_nurse_door_hit`（06 のナースの `HitFX` の音・塵・加算の材質）を足し、`import_all` に入れた（作業一覧の項目 7 のステップ 3）
