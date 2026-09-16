# テレキネシスと Vanish：原作データの調査（pak_reference_2、UE 4.24）

- 調べた日: 2026-09-16。**読んだだけで、リポジトリは何も変更していない。**
- 根拠の書き方: `ファイル @オフセット`。オフセットは `.txt` の `@` の値で、JSON の `mem` と同じ実行時のオフセット。`.txt` で `@` が付いていない行は、`.json` の `mem` から補った（例: `BP_TelekinesisPower @777`）。
- 略記:
  - `PC` = `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`
  - `TK` = `.../Blueprints/Main/Powers/BP_TelekinesisPower.txt`
  - `VA` = `.../Blueprints/Main/Powers/BP_VanishPower.txt`
  - `SH` = `.../Blueprints/Main/BP_Shard.txt`
  - `PW` = `.../UI/BP_Powers.txt`（パワーの枠のクールダウン表示。プレイヤーの `Power Controller` 子アクタ）
  - `RN` = `.../Blueprints/Characters/Nurse/BP_06_ReaperNurse.txt`
- 「確定」はバイトコードかアセットの値そのもの。「推測」はエンジンの既定値やソースから導いたもので、必ずそう書く。

---

## 0. 要点

1. **テレキネシス**
   - 使った瞬間のプレイヤーの位置を中心にした球（`SphereOverlapActors`）の中から、`DD_TelekinesisInterface` を実装したアクタ（= `BP_Shard` とその子 `BP_Shard_Special`）すべてに `Activate` を呼ぶ。
   - シャードは自分の `Shard Pull` タイムライン（1.0 s、再生速度は 0.8〜1.2 の乱数）で、水平方向だけプレイヤーへ寄る（`VEase` ExpoIn）。タイムラインが終わると、**届いていなくても** `Collect(False)` で回収される。
   - 半径は強化段階 0〜5 で 1750/2000/2250/2500/2750/3000。クールダウンは 10.5/10.0/9.5/9.0/8.5/8.0 s で、`00_Ballroom` だけ 1.0 s。
   - **青い見た目はパーティクル `P_ky_forceField_Telekinesis`（×2 倍）と、画面全体の後処理（彩度 0 と青のゲイン）でできている。デカールや範囲のメッシュは無い。**
2. **Vanish**
   - 効果時間は全段階 15 s。クールダウンは効果が終わってから 30/28/24/20/18/15 s。
   - 見えなくなる仕組みは 3 つある。
     1. プレイヤーのカプセルの **Camera チャンネルへの応答を Ignore** にする。敵の視線トレースは `LineTraceSingle` の TraceTypeQuery2 = Camera なので、プレイヤーに当たらない。
     2. `Active Powers` に 5 を入れる（見る側は `Is Player Using Power ?(5)` で判定する）。
     3. 使った瞬間にタグ `Enemy` の全アクタへ `DD_EnemyInterface.Player Vanish` を呼ぶ（ナースは `Seen Player Recently=False` で追跡をやめる）。
   - **終わるときに敵へ知らせる処理は無い。** 途中で解除する処理も無い（シャードを取る、別のパワーを使う、攻撃される、のどれでも解除されない）。
   - 死んで復活すると `Reset Vanish` が枠とウィジェットだけを回復させるが、透明の状態は 15 s が経つまで残る。
3. **病院 Zone 1・Zone 2 のレベル BP に、2 つのパワーを特別に扱う処理は無い。**
   - Zone 2 の見張りナースとマトロンのビューコーンはカメラチャンネルのトレースを使うので、Vanish の衝突の仕組みで見えなくなる。
   - `BP_06_ReaperNurse_06_Chase`（Zone 1 の特別な追跡ナース）と、見つかった後の Sentry は、Tick で常に追うので、Vanish では止まらない（推測を含む。§3.8）。

---

## 1. 共通の土台（2 つのパワーに関係するところだけ）

### 1.1 入力と枠

- 入力（`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` 50〜55 行）:
  - `Use Power Left` = Q / Gamepad_LeftTrigger
  - `Use Power Right` = E / Gamepad_RightTrigger
  - `Cycle Power Left/Right` = 1 / 2
  - （`Use Power` = R の割り当てもある）
- `InpActEvt_Use Power Left`（PC @23370）
  1. `Can Interact?` を確かめる。
  2. DoOnce（`Temp_bool_IsClosed_Variable_8`）を通す。
  3. `Use Power(True)` を呼ぶ（@23451）。
  4. `Delay(0.5)`（@23466）の後、@6833 で DoOnce を開け直す。
- `Use Power Right`（PC @23521）も同じ形で、`Use Power(False)` を呼び、`Delay(0.5)` の後 @6810 で開け直す。
- `Use Power` 本体（PC @29314）
  1. `Can Use Tablet?` と `Has Input` を確かめる。
  2. `Powers.Animation(1?)` を呼ぶ（@29334）。
  3. `Power To Use = Powers.Power[ Select(1?: False→Right Power Index, True→Left Power Index) ]`（@29398）。
  4. @11703 へ跳ぶ。
- PC @11703 以降
  1. `Power To Use.Available?` が False なら @16335 へ行くが、何もしない。
  2. True なら、次の順に進む。
     - DoOnce（`IsClosed_Variable_11`）
     - `UsedPower` をブロードキャスト（@11802）
     - `Power` で分岐: **4 → @19661（テレキネシス）、5 → @20985（Vanish）**
     - 分岐の後、@16280 の `Delay(0.5)` → @1668 で DoOnce を開け直す。**パワー全体に 0.5 s の連打防止がある。**
- パワーの番号は `Enum_RingAltar_Skills`（`_assets/.../Blueprints/Enums/Enum_RingAltar_Skills.json`）。
  - 0 Speed Boost、1 Teleport、2 Telepathy、3 Primal Fear、**4 Telekinesis**、**5 Vanish**、6 None
  - `Struct_Power` の `Power_4_...` はこの列挙（`Struct_Power.json` の imports）。
- 強化段階は `Get Power Upgrade Level`（PC 10543 行〜）で取る。
  - `GameMode.Global Save Instance` の `Telekinesis Upgrades`（case 4）/ `Vanish Upgrades`（case 5）の整数を返す。
  - 本作の段階は 0〜5（下の Select が case 0〜5）。
- 使えるかどうか（解放）は、セーブの `Telekinesis Unlock` / `Vanish Unlock` で決まる（`UI/Tablet/UMG_TabletPowers.txt` @3996 付近、@4100 付近）。

### 1.2 枠のクールダウン表示（`BP_Powers`）

- `Set Delay(Duration, Power)`（PW @1609）が、Power で `Set Delay Telekinesis`（@1976）/ `Set Delay Vanish`（@2000）に分かれる。
- `Set Delay Vanish`（PW @3268）
  1. `Timeline_3.SetPlayRate(1/Duration)`（@3347）
  2. @1228 の FlipFlop（`Temp_bool_Variable_1` を反転）
     - True なら `Timeline_3.ReverseFromEnd`（@886）
     - False なら `PlayFromStart`（@952）
- `Set Delay Telekinesis`（PW @3476）
  1. `Timeline_4.SetPlayRate(1/Duration)`（@3555）
  2. @1314 の FlipFlop（`Temp_bool_Variable_2`）
     - True なら `ReverseFromEnd`（@1062）
     - False なら `PlayFromStart`（@1128）
- タイムライン `Timeline_3_Template` / `Timeline_4_Template`（`_assets/.../UI/BP_Powers.json`）
  - 長さ 1.0。トラック `Floaty` は直線 0（0 s）→ 1（1 s）。
  - 更新のたびに `Powers.Vanish`（@808）/ `Powers.Telekinesis`（@985）の MID のスカラー `Percent` へ書く。
- MID は `UMG_TabletPowers` が作る（`UI/Tablet/UMG_TabletPowers.txt`）。
  - @2744 で `CreateDynamicMaterialInstance(MM_Powers_Vanish, 'vanish')`
  - @2806 で `CreateDynamicMaterialInstance(MM_Powers_Inst_Telekinesis, 'telekinesis')`
- 使い方の流れ
  - 1 回目の `Set Delay`（使った直後）では Percent が 1→0 に減る。
  - 2 回目（クールダウンの開始）では 0→1 に戻る。
- `Reset All Powers`（PW @2551）は `Stop Vanish Timeline`（@3393: `Timeline_3.Stop` @919、`Vanish` の Percent=1.0 @3403）と `Player.Reset Vanish`（@2667）を呼ぶ。**テレキネシスは含まれない。**

---

## 2. テレキネシス

### 2.1 発動（プレイヤー側、PC @19661〜@20984）

| 順 | オフセット | 処理 |
|---|---|---|
| 1 | @19661 | `UsedTelekinesis` をブロードキャスト |
| 2 | @19700 | `Active Powers.AddUnique(4)` |
| 3 | @19768〜@20038 | `Powers.Power` の `{4, Available?=True}` を探して `{4, False}` に置き換え、`Powers.Update Powers()` |
| 4 | @20074 | `PowerController.Set Delay(0.05, 4)`（枠の表示が 0.05 s で空になる） |
| 5 | @20117〜@20306 | `BeginDeferredActorSpawnFromClass(BP_TelekinesisPower_C, Transform(プレイヤーの位置 − (0,0,5000), 回転 (0,0,0), 拡大 1), CollisionHandling=1 (AlwaysSpawn), Owner=None)` |
| 6 | @20348〜@20538 | `SetFloatPropertyByName('Range', 段階 0→1750.0 / 1→2000.0 / 2→2250.0 / 3→2500.0 / 4→2750.0 / 5→3000.0)`（`Get Power Upgrade Level(4)` @20486） |
| 7 | @20892 | `FinishSpawningActor`（同じ変換） |
| 8 | @20930 | `Delay(0.06)` の後、**@8329**（クールダウンの開始）へ |
| 9 | — | 分岐を抜けると、§1.1 の 0.5 s の連打防止へ |

### 2.2 クールダウンと回復（PC @8329〜@9937）

- @8329: `GetCurrentLevelName(True)` が `'00_Ballroom'` と等しいかを見る（@8520）。
- @8633: `PowerController.Set Delay(値, 4)`。値は次のとおり。
  - `00_Ballroom` なら **1.0**
  - それ以外は段階 0→**10.5** / 1→**10.0** / 2→**9.5** / 3→**9.0** / 4→**8.5** / 5→**8.0**（@8359〜@8497 の定数と @8668 の二重 Select）
- @8868: `Active Powers.RemoveItem(4)`。**「テレキネシス使用中」の扱いは 0.06 s で終わる。**
- @9202: `Delay(同じ値)` の後、@9429 へ。
- @9429〜@9562 はゲート（`Temp_bool_Whether_the_gate_is_currently_open_or_close_Variable_4`。初期は閉、Open の後に Enter する並び）。
- @9572〜@9937 で回復する。
  - `PlaySound2D(/Game/Audio/UI/power_refilled, Volume 0.5, Pitch 1.0, StartTime 0.0)`（@9572）
  - `{4, True}` を書き戻し（@9631〜@9819）、`Update Powers`（@9901）
- `Reset Telekinesis`（PC 9410 行 → @37753 → @28238）は、ゲートを閉じて（@28226）@9572 の回復を直接呼ぶ。**全バイトコードを grep したが、どこからも呼ばれていない。** 死んでもテレキネシスのクールダウンは続く。
- 使ってから再び使えるまで: 0.06 + クールダウン（段階 0 なら 10.56 s）。

### 2.3 DataTable との照合

`_datatables.json` の `/Game/UI/RingAltar_UI/Enums/PowersTable_Telekinesis`:

| 行 | 表示（原文） | コスト | コードの値（段階） |
|---|---|---|---|
| （段階 0。表に行が無い） | — | — | Range 1750 / CD 10.5 |
| Upgrade2 | `RANGE: <YEL>2000</> COOLDOWN: <YEL>10.00</>` | 2 | 段階 1: 2000 / 10.0 |
| Upgrade3 | `RANGE: <YEL>2250</> COOLDOWN: <YEL>09.50</>` | 2 | 段階 2: 2250 / 9.5 |
| Upgrade4 | `RANGE: <YEL>2500</> COOLDOWN: <YEL>09.00</>` | 2 | 段階 3: 2500 / 9.0 |
| Upgrade5 | `RANGE: <YEL>2750</> COOLDOWN: <YEL>08.50</>` | 2 | 段階 4: 2750 / 8.5 |
| Upgrade6 | `RANGE: <YEL>3000</> COOLDOWN: <YEL>08.00</>` | 2 | 段階 5: 3000 / 8.0 |

表示とコードはすべて一致する（コスト列の名前は `Cost_5_2A8785B64B7BA566060E5AAEA840E938`）。

### 2.4 `BP_TelekinesisPower` の処理（TK、時系列）

- 親クラスは `/Script/Engine.Actor`。
- CDO の `Range` = **1500.0**。発動側が必ず上書きするので、実際にこの値になることは無い。
- イベントの入口: `ReceiveBeginPlay` → @661、`Timeline_0__UpdateFunc` → @1206、`Timeline_0__FinishedFunc` → @1424。

**BeginPlay の直後（t=0）**

- @661〜@737: `SetActorLocation(GetPlayerCharacter(0).GetActorLocation(), bSweep=False, bTeleport=False)`。スポーンした Z−5000 から、プレイヤーのカプセルの中心へ移す。
- @777: `PlaySoundAtLocation(/Game/Audio/SharedGameplay/Stun_Wave_Attack_New_04, Location=(0,0,0), Rotation=(0,0,0), Volume 1.0, Pitch 1.0, StartTime 0.0, Attenuation=None, Concurrency=None, Owner=None)`。
  - 位置が原点のままだが、この SoundWave は減衰の設定を持たない（`_assets/.../Stun_Wave_Attack_New_04.json` にあるのは `NumChannels 2, SampleRate 44100, Duration 1.710045, SoundClass DD_SoundClass_SFX` だけ）。
  - したがって実質的に 2D で鳴る（推測: 減衰なし・ステレオは空間化されない）。
- @851〜@877: `GetPlayerController(0).ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop_C, Scale=25.0, PlaySpace=0 (CameraLocal), UserPlaySpaceRot=(0,0,0))`。
- @942: 対象の種類の配列 `[ByteConst 2, 1, 0]`。
  - EObjectTypeQuery3/2/1 に当たり、**Pawn, WorldDynamic, WorldStatic** の意味になる。
  - 対応は UE の既定のオブジェクトチャンネルの並び。`DefaultEngine.ini` に独自のオブジェクトチャンネルは `Teleport`（GameTraceChannel1）しかなく、旧 README で ObjectTypeQuery7 = Teleport と確かめてある。
- @959〜@1046: `SphereOverlapActors(中心 = GetPlayerCharacter(0).GetActorLocation(), 半径 = Range, 種類 = 上の配列, ActorClassFilter = None, ActorsToIgnore = Temp_object_Variable, OutActors)`。
  - `ActorsToIgnore` は何もつながっていない配列（推測: 空）。
- @1113 → @633〜@600 は ForLoop。各要素について次を行う。
  - @177: `DoesImplementInterface(item, DD_TelekinesisInterface_C)`
  - @302: `ObjToInterfaceCast`
  - @359: `Activate()`（インターフェイスの呼び出し）
- ループが終わったら、@1118 で `Timeline_0.PlayFromStart()`。
- @1151: `Delay(0.2)`。

**t=0.2 s**

- @15〜@43: `SpawnEmitterAtLocation(P_ky_forceField_Telekinesis, 自分の位置（= t=0 のプレイヤーのカプセル中心）, Rotation (0,0,0), Scale (2.0,2.0,2.0), bAutoDestroy=True, PoolingMethod=0 (None), bAutoActivate=True)`。
- このパーティクルはプレイヤーに付いて行かない。

**毎フレーム（タイムラインの更新、@1206〜@1359）**

- `PostProcess.BlendWeight = Lerp(1.0, 0.0, float2)`
- `PostProcess1.BlendWeight = MapRangeClamped(float2, 0.0, 0.3, 1.0, 0.0)`

**t=2.0 s（タイムラインの終わり、@1424 → @1409）**

- `K2_DestroyActor()`

**使っていないもの**

- タイムラインのトラック `float` / `desaturation` / `opacity` はどこからも読まれない。

### 2.5 コンポーネントとタイムライン（`_assets/.../Powers/BP_TelekinesisPower.json`）

SCS の構成:

- `DefaultSceneRoot`（SceneComponent、値なし）
  - `PostProcess`（`PostProcess_GEN_VARIABLE`）
  - `PostProcess1`（`PostProcess1_GEN_VARIABLE`）

| export | 値（書き出しにあるものだけ。無いものは既定値） |
|---|---|
| `PostProcess_GEN_VARIABLE` | `bOverride_ColorSaturation`, `bOverride_ColorGain`、`ColorSaturation [0,0,0,1]`、`ColorGain [0.0, 0.4217270016670227, 1.6100000143051147, 1]`、`BlendWeight 0.0` |
| `PostProcess1_GEN_VARIABLE` | `bOverride_ColorGamma`（値は既定の [1,1,1,1]）、`bOverride_ColorGainMidtones`、`bOverride_SceneFringeIntensity`、`ColorGainMidtones [100,100,100,1]`、`SceneFringeIntensity 50.0`、`BlendWeight 0.0`。ほかに `ColorSaturation [0,0,0,1]` と `ColorGain [1.61, 0.129563, 0, 1]` の値もあるが、**override が立っていないので効かない** |

- PostProcessComponent の `bUnbound`・`Priority`・`BlendRadius` は書き出しに無いので既定値（推測: UE の既定は bUnbound=true、Priority=0）。つまり画面全体に掛かる。
- 見え方
  - `PostProcess` は「白黒にして青（シアン寄り）のゲインを掛ける」。
  - `PostProcess1` は「中間調を 100 倍する白飛び ＋ 色収差 50」。

`Timeline_0_Template`（`TimelineLength 2.0`）:

| トラック | CurveFloat | キー（Time, Value, Arrive, Leave, 補間） |
|---|---|---|
| float | CurveFloat_0 | (0.0, 0.0, 1.5797574520111084, 1.5797579288482666, Cubic/User) → (0.992901086807251, 0.9189547300338745, 0.3629918098449707, 0.36299169063568115, Cubic/User) → (1.5134799480438232, 0.9834880232810974, 0, 0, Linear/Auto) |
| **float2**（使う） | CurveFloat_1 | (**-0.011600494384765625**, 0.0, -0.0950283631682396, -0.09502881020307541, Cubic/User) → (**0.5**, 1.0, 4.5270514488220215, 4.527058124542236, Cubic/User) |
| desaturation | CurveFloat_2 | (0.006957054138183594, 0.015488147735595703, 0, 0, Cubic/Auto) → (1.016563892364502, 0.17036807537078857, 0.6986591815948486, 0.6986591815948486, Cubic/Auto) → (1.4109413623809814, 0.996394693851471, 0, 0, Linear/Auto) |
| opacity | CurveFloat_3 | (0.0, 1.0, -0.21745750308036804, -0.21745805442333221, Cubic/User) → (0.6773991584777832, 0.725354790687561, -0.5983153581619263, -0.5983161926269531, Cubic/User) → (1.592355489730835, 0.002581477165222168, 0, 0, Linear/Auto) |

float2 を UE の 3 次補間（P1=P0+Leave·Δ/3、P2=P3−Arrive·Δ/3 のベジエ）で計算した値（計算値）:

| t (s) | 0 | 0.05 | 0.10 | 0.15 | 0.20 | 0.25 | 0.30 | 0.35 | 0.40 | 0.45 | ≥0.50 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| float2 | -0.0007 | 0.0059 | 0.0293 | 0.0710 | 0.1325 | 0.2151 | 0.3206 | 0.4503 | 0.6058 | 0.7885 | 1.0 |
| PostProcess の重み | 1.0007 | 0.994 | 0.971 | 0.929 | 0.868 | 0.785 | 0.679 | 0.550 | 0.394 | 0.212 | 0 |
| PostProcess1 の重み | 1.0 | 0.980 | 0.902 | 0.763 | 0.559 | 0.283 | 0 | 0 | 0 | 0 | 0 |

### 2.6 パーティクル `P_ky_forceField_Telekinesis`（`_particles.json`）

**システム全体**

- `lod_distances [0, 2500, 5000]`
- 固定の外接箱 `[-200,-200,-100]〜[200,200,250]`（`bUseFixedRelativeBoundingBox`）
- エミッタ 4 つ。すべて `EmitterDuration 2.0`、`EmitterLoops 1`、ワールド空間（`bUseLocalSpace` の記載なし = false）。
- 以下は LOD0 の値。**コンポーネントの拡大 2.0 は、大きさと出現位置のオフセットの両方に掛かる**（UE 5.8 のソース `ParticleSystemRender.cpp` の `Particle.Size * Source.Scale`、`ParticleModules_Location.cpp` の `EmitterToSimulation.TransformVector` で確かめた。4.24 も同じと推測）。
- 粒子の大きさは 0 から始まり、Size モジュールは加算になる（`ParticleEmitterInstances.cpp` の `PreSpawn` は Memzero、`UParticleModuleSize::SpawnEx` は `Size +=`。UE 5.8 のソースで確認）。

| エミッタ | 種類・マテリアル | 出し方 | 主な値 |
|---|---|---|---|
| `aura` | メッシュ `SM_ky_windLine27midPoly`（`bOverrideMaterial`）、`MI_ky_aura7c` | バースト 1 個ずつ、t = 0 / 0.1 / 0.2 | 寿命 0.8。大きさ 13 + (1.5,1.5,1.0) = (14.5,14.5,14.0)（加算は推測、上の注）× `SizeMultiplyLife`（10 → 0.53 で 1.556 → 1.0 で 0.542、16 標本）。`MeshRotation` の開始はエミッタ時刻の曲線 (0,0,0)@0 → (0,0,1)@0.2。`MeshRotationRate (0,0,-1)`。色 (0, 0.437213, 2.439914)、α 0.15 × `AlphaScaleOverLife`（0 → 0.3 で 0.917 → 1.0 で 0.083）。出現位置 (0,0,90)。`SourceMovement (1,1,1)`。`ParameterDynamic maskOffsetY` 0.1→0.2（寿命）、Param2〜4 = 1 |
| `ground` | スプライト、`MI_ky_shockWave02_4x4_nonD`（4×4、`PSUVIM_Linear_Blend`、`bDelayFirstLoopOnly`） | バースト 1 個、t=0 | 寿命 1.0。大きさ X=360（`ScreenAlignment` の既定は PSA_Square なので Y=X。UE 5.8 の `GetParticleSize`）× `SizeMultiplyLife`（aura と同じ曲線）。色 (0, 0.279298, 5.0)、α 0 → 0.3 で 0.458 → 1.0 で 0.042。SubUV の番号 8 → 31（寿命）。回転は乱数 0〜1。`OrientationAxisLock EPAL_Z`（地面と平行）。出現位置 (0,0,5) |
| `sphere` | メッシュ `SM_ky_sphere`（半径 10 cm）、`M_ky_wall02_4x4_two`（4×4） | バースト 1 個、t=0 | 寿命 1.0。大きさ 13 × `SizeMultiplyLife`（10 → 1.556 → 0.542）。色 `ColorOverLife`（128 標本）は (0, 2.936, 20) → 0.3 で (0, 7.306, 49.76) → 0.7 以降 (0, 0.658, 5.0)。α（32 標本）は 0 → 0.1 で 0.96 → 1.0 で 0。SubUV の番号 0 → 0.5 で 29.8 → 1.0 で 40。`MeshRotationRate` は乱数 (0,0,±0.05)。出現位置 (0,0,90)。**`ParticleModuleLight`**（`bUseInverseSquaredFalloff False`、`ColorScaleOverLife (1.5,1.2,1.2)`、`BrightnessOverLife 5.0`、`RadiusScale 40`、`LightExponent 8`） |
| `dustSq` | スプライト、`MI_ky_starDust_sq` | バースト 5 個、t=0。出現率はエミッタ時刻の曲線 20@0.4 → 0@0.5（それより前は 20/s） | 寿命 0.7〜0.9。大きさ 40〜80 × `SizeMultiplyLife`（0.3 → 0.3 で 0.977 → 1.0 で 0.5）。色 (0, 0.322458, 1.0)、α 2.0。`LocationPrimitiveSphere`（半径は曲線 50@0 → 150@0.2、`SurfaceOnly`、`Velocity True`、`VelocityScale` 0〜1、`StartLocation` (0,0,-50)〜(0,0,0)）。速度 (0,0,0)〜(0,0,500)。`AccelerationDrag` 2@0.3 → 5@1.0（ワールド）。加速度 (0,0,100)。回転 0〜1、回転速度 0〜1。`Orbit`（Offset (50,0,-10)〜(100,0,10)、Rotation (0,0,-1)〜(0,0,1)、RotationRate (-0.1,-0.1,-2)〜(0.1,0.1,-1)）。`ParameterDynamic`: `flashTime` 1〜8、`flashPower` 3〜10、`starDensity` 68〜35（spawn 時だけ） |

- **見え方（計算・推測）**
  - 青い球（`sphere`）の半径は、10 cm × 13 × 寿命の倍率 × 2 で決まる。t=0 で約 2600 cm、寿命の中ほどで約 404 cm、寿命の終わりで約 141 cm。**外から内へ縮む**。
  - 地面の輪（`ground`）も幅 360×10×2 = 7200 cm から縮む。
  - 見た目の大きさは Range と連動しない（拡大は常に 2）。
  - 出現位置 +90 と +5 は ×2 され、カプセルの中心（床から 88 cm）からの高さになる。そのため `ground` の輪も床ではなく、カプセルの中心 +10 cm あたりに出る（推測）。
- SubUV の番号が 16 コマを超える（8→31、0→40）。
  - スプライトのシェーダは横を `fmod`、縦を `floor(index/4)` で計算するので、UV が 1 を超え、テクスチャの Wrap で繰り返す。
  - メッシュ粒子は `ParticleSystemRender.cpp` で `A % X`、`A / X` と計算する（どちらも UE 5.8 のソースで確認。サンプラが Wrap かは推測）。

### 2.7 何を引き寄せるか（条件と除外）

- **対象**は、`SphereOverlapActors` が返したアクタのうち `DD_TelekinesisInterface`（`Blueprints/Characters/Shared/DD_TelekinesisInterface`、関数 `Activate` だけ）を実装するもの。
  - 全 `_assets`・`_bytecode` を grep すると、このインターフェイスを参照するのは `BP_Shard` と `BP_TelekinesisPower` だけ。
  - **実装者は `BP_Shard` と、その子 `BP_Shard_Special`**（`BP_Shard_Special.json` の super = `BP_Shard_C`。Activate を上書きしていないので同じように寄る）。
- **対象にならないもの**
  - `BP_BonusShard`: super は Actor で、インターフェイスを持たない。
  - `bDisabled` のシャード: BeginPlay（SH @2336〜@2361）で `SetActorHiddenInGame(True)` と `Capsule.SetCollisionEnabled(0 = NoCollision)` になる。重なりの問い合わせに引っかからない（推測: オブジェクト種類の問い合わせはクエリ衝突が無効な部品を拾わない）。
  - **病院 Zone 1・Zone 2 の配置シャードは、どれも `bDisabled` を持たない**（`06_Hospital_Zone_01.full.json` の `BP_Shard_C` 337 個、Zone 2 は 342 個。どちらも `bDisabled` が書き出しに無い = 既定の False）。
- **引っかかる部品**
  - シャードの `Capsule`: `ObjectType ECC_WorldStatic`、`CollisionProfileName Custom`、半径と半高さ 49.57180404663086、相対拡大 0.1。
  - 親の `SkeletalMesh` の拡大が 10 なので、ワールドでは半径約 49.6 cm の球になる。
  - 応答は既定の OverlapAllDynamic のまま（推測: UE 5.8 の `UShapeComponent` のコンストラクタが OverlapAllDynamic。4.24 も同じと推測）。
- **範囲**
  - 球。中心は**使った瞬間のプレイヤーのアクタ位置（カプセルの中心）**、半径は `Range`。
  - 1 回だけ判定する。後から範囲に入ったシャードは対象にならない。
  - 高さも含む 3D の判定。

### 2.8 シャード側の処理（`BP_Shard`、SH）

**`Activate`（SH 695 行 → @2610）**

- @2610: `Shard Pull.SetPlayRate(RandomFloatInRange(0.8, 1.2))`
- @2689 → @1581: `Shard Pull.PlayFromStart()`

**`Shard Pull` の更新（@1614〜@1999）**

- `A = (Previous Location.X, .Y, .Z)`。`Previous Location` は BeginPlay で入れた初期位置（@2400〜@2428）。
- `B = (プレイヤーの X, プレイヤーの Y, Previous Location.Z)`。**高さは変えない。**
- `SetActorLocation(VEase(A, B, Alpha, EasingFunc=8, BlendExp=2.0, Steps=2), bSweep=True, bTeleport=False)`（@1892、@1959）
  - `EasingFunc 8` = **ExpoIn**（UE 5.8 の `EEasingFunc` の並びで確認）。
  - 式は `α' = (α==0) ? 0 : 2^(10(α−1))`、位置 = `Lerp(A, B, α')`（UE 5.8 の `FMath::InterpExpoIn`）。
  - BlendExp と Steps は ExpoIn では使われない。
  - プレイヤーの位置は毎フレーム取り直すので、プレイヤーを追いかける。
  - α' は α=0.5 で 0.031、α=0.9 で 0.5。**ほとんど動かず、最後に一気に寄る。**

**`Shard Pull_Template`**

- `TimelineLength 1.0`、トラック `Alpha`（CurveFloat_0）は Linear の (0.0, 0.0) → (0.75, 1.0)。0.75 以降は 1.0 のまま。
- 実時間では、再生速度 r（0.8〜1.2）に対して、位置が届くのは 0.75/r = 0.625〜0.9375 s、タイムラインが終わるのは 1/r = 0.833〜1.25 s。

**終わり（@2540）**

- `Collect(False)`。**届いたかどうかに関係なく回収する。**

**途中でカプセルがプレイヤーに重なった場合**

- `BndEvt__Capsule_..._ComponentBeginOverlap`（→ @2456）で、`OtherActor == GetPlayerCharacter(0)` なら @55 の回収へ進む。
- 回収は DoOnce（`Temp_bool_IsClosed_Variable`）で 1 回だけ。
- `bSweep=True` でも、カプセルの応答は全部 Overlap（推測）なので壁で止まらない（推測）。

**回収 `Collect(NoSound?)`（→ @2535 → @55〜）**

1. @91〜@615: `Player.Widget` の `UMG_Tablet.ShardCount` の文字を整数にして −1 し、`Clamp(0, 9999)` して書き戻す。
2. @682: `PlayAnimation(Count Shake, 0.0, 1, 0, 2.0, False)`。
3. @773: `GameMode.Check Shards()`。
4. @835: `ClientPlayCameraShake(BP_CameraShake_ShardCollect_C, 0.4, CameraLocal, (0,0,0))`。
5. @950: `SpawnEmitterAtLocation(P_ky_flash3, SkeletalMesh の位置, (0,0,0), (0.2,0.2,0.2), True, 0, True)`。
6. @1019 で NoSound により分かれる。
   - **NoSound=False の場合**
     - @1228〜@1478: `GameInstance.Shards To Be Removed.AddUnique(FTruncVector(Previous Location) を Vector にしたもの)`。
     - @1515: `PrintString`（Shipping では出ない）。
     - @1033 へ進む。
   - **NoSound=True の場合**: そのまま @1033 へ。
7. @1033: `K2_DestroyActor()`。
8. @1112: `PlaySound2D(/Game/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue, Volume = Select(NoSound: False→0.65, True→0.0), Pitch 1.0, StartTime 0.0, Concurrency = /Game/Audio/OnlyFew, Owner None)`。

**回収まわりのアセット**

- `OnlyFew`: `MaxCount 1`、`ResolutionRule StopOldest`、`VolumeScale 0.5`。**テレキネシスで一度に大量に回収しても、取得音は 1 つずつしか鳴らない。**
- `Soul_Shard_Pickup_v2_Cue`: `SoundNodeModulator`（PitchMin 0.9、PitchMax 1.1）→ `Soul_Shard_Pickup_v2`（0.43775 s、2ch、48 kHz、SFX）。
- `BP_CameraShake_ShardCollect`: `OscillationDuration 0.1`、BlendIn 0、BlendOut 0.05。
  - Rot: Pitch Freq 20、Yaw Freq 15、Roll Amp 1.5 / Freq 15 / `EOO_OffsetZero`。
  - Loc: XYZ Freq 25（Amp は 0）。
  - FOV: Amp 3.0 / Freq 15 / `EOO_OffsetZero`。

**シャードのコンポーネント（`BP_Shard.json`）**

- `SkeletalMesh`
  - メッシュ `soul_shard_skeletal`、アニメ `soul_shard_skeletal_anim_loop`（`AnimationSingleNode`）
  - 相対位置 (0, 0, 97.085)、拡大 10、`LDMaxDrawDistance 3000`、影なし
  - BeginPlay で再生速度を 0.05〜0.15 の乱数にする（@2256）
- `PointLight`: 強さ 175、色 (255,0,194)、減衰 200、`MaxDrawDistance 1750`、`MaxDistanceFadeRange 1500`、影なし、`VolumetricScatteringIntensity 2.5`
- `Capsule`: §2.7
- `PPP_Collect_Shard`（`bAutoActivate False`。バイトコードで起動する箇所は無い）
- `Plane`（ミニマップの印。`M_Shard`、Z 2000、拡大 (1.5,1.5,10)、NoCollision）
- 構築スクリプトで、`Material`（`m_crystal_Inst1`）をスロット 0 へ設定し、灯の強さを `Light Intensity` 175 にする。

**旧版との違い**

- `pak_reference_2/_diff.json` の `changed_bytecode` / `project_references` に `BP_Shard` がある。新版で `Activate`、`Enable`、`ReceiveTick`、`Shard Pull` が足された。
- **シャードの寄せ方と `Shards To Be Removed` は新版にしかない。** 本作のシャード（M3、まだ未実装）をどちらの版に合わせるかは、original-fidelity.md に従いユーザーの確認が要る。
- なお、今回の進捗記録では「テレポーテーション以外のパワーは pak_reference_2 に従う」と決めている。

### 2.9 カメラと音のまとめ

- カメラ
  - `01_Hotel_Lobby_ElevatorShakeStop`（`_assets/.../Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.json`）: `OscillationDuration 0.5`、`OscillationBlendInTime 0.0`、`OscillationBlendOutTime 0.5`。
  - `LocOscillation` は X (Amp 2.0, Freq 50.0)、Y (2.0, 35.0)、Z (3.0, 10.0)。回転と FOV は無い。
  - `InitialOffset` は書き出しに無いので既定（推測: EOO_OffsetRandom）。
  - **Scale 25.0 で再生するので、振幅は X・Y 50 cm、Z 75 cm。0.5 s で直線的に弱まる。**
  - CameraAnim や FOV の演出は無い。
- 音
  - 使った瞬間: `Stun_Wave_Attack_New_04`（1.0 / 1.0）
  - 回復: `power_refilled`（音量 0.5、2D、1.514667 s、2ch、48 kHz）
  - 回収: `Soul_Shard_Pickup_v2_Cue`（0.65）

---

## 3. Vanish

### 3.1 発動（プレイヤー側、PC @20985〜@23151）

| 順 | オフセット | 処理 |
|---|---|---|
| 1 | @21005 | `Active Powers.AddUnique(5)` |
| 2 | @21073 | `UsedVanish` をブロードキャスト |
| 3 | @21092〜@21362 | `{5, True}` を `{5, False}` にして `Update Powers` |
| 4 | **@21398** | **`CapsuleComponent.SetCollisionResponseToChannel(Channel=4 (ECC_Camera), Response=0 (ECR_Ignore))`** |
| 5 | @21628 | `PowerController.Set Delay(15.0, 5)`（段階 0〜5 はすべて 15.0。@21438〜@21553） |
| 6 | @21799 | `WidgetBlueprintLibrary.Create(Self, UMG_Vanish_C, OwningPlayer=None)` |
| 7 | @22042 | `SetFloatPropertyByName(widget, 'Speed', 15.0)`（全段階 15.0） |
| 8 | @22207, @22226 | `Vanish Widget = widget`、`AddToPlayerScreen(ZOrder 0)` |
| 9 | @22486, @22741 | `BeginDeferredActorSpawnFromClass(BP_VanishPower_C, Transform(プレイヤーの位置 − (0,0,5000), **プレイヤーの GetActorRotation**, 1), AlwaysSpawn)` → `FinishSpawningActor` |
| 10 | @22969 | **`Delay(15.0)`**（全段階 15.0）の後、**@6856**（終わり）へ |

- ECollisionChannel の 4 は Camera（0 WorldStatic、1 WorldDynamic、2 Pawn、3 Visibility、4 Camera）。ECollisionResponse の 0 は Ignore、2 は Block。
- プレイヤーのカプセルは `ACharacter` の既定のプロファイル `Pawn`（Visibility だけ Ignore、Camera は既定の Block）。
- 本作の `AWasamiPlayerCharacter` は、カプセル 50 / 88 と原作に合わせてある。

### 3.2 終わりとクールダウン（PC @6856〜@8328）

- @6856〜@7207: `PowerController.Set Delay(クールダウン, 5)`。クールダウンは段階 0→**30.0** / 1→**28.0** / 2→**24.0** / 3→**20.0** / 4→**18.0** / 5→**15.0**（@6994 の `Get Power Upgrade Level(5)` と @7073 の Select）。
- @7274: `Active Powers.RemoveItem(5)`。
- **@7334: `CapsuleComponent.SetCollisionResponseToChannel(4 Camera, 2 Block)`**（見えるように戻す）。
- @7725: `Delay(同じクールダウン)` の後、@7784 へ。
- @7784〜@7917 はゲート `_3`（Open → Enter）。
- @7927〜@8328 で回復する。
  - `PlaySound2D(power_refilled, 0.5, 1.0, 0.0)`
  - `{5, True}` を書き戻して `Update Powers`
  - **@8292: `Vanish Widget.RemoveFromParent()`**（ウィジェットはクールダウンが終わるまで画面に残る。15 s の時点で不透明度は 0）
- 使ってから再び使えるまで: 15 + クールダウン（段階 0 なら 45 s）。
- **終わるときに敵へ知らせる処理（`Player Vanish` の逆など）は無い。** `BP_VanishPower` にも終わりの処理は無い。

### 3.3 DataTable との照合

`PowersTable_Vanish`:

| 行 | 表示（原文） | コスト | コードの値 |
|---|---|---|---|
| （段階 0） | — | — | 15 / 30.0 |
| Upgrade2 | `DURATION: <YEL>15</> COOLDOWN: <YEL>28.00</>` | 2 | 段階 1: 15 / 28.0 |
| Upgrade3 | `DURATION: <YEL>15</> COOLDOWN: <YEL>24.00</>` | 2 | 段階 2: 15 / 24.0 |
| Upgrade4 | `DURATION: <YEL>15</> COOLDOWN: <YEL>20.00</>` | 2 | 段階 3: 15 / 20.0 |
| Upgrade5 | `DURATION: <YEL>15</> COOLDOWN: <YEL>18.00</>` | 2 | 段階 4: 15 / 18.0 |
| Upgrade6 | `DURATION: <YEL>15</> COOLDOWN: <YEL>15.00</>` | 2 | 段階 5: 15 / 15.0 |

すべて一致する。

### 3.4 `BP_VanishPower` の処理（VA）

- 親は Actor。CDO の `Range` = 1500.0 だが、どこからも使われない。
- 入口: `ReceiveBeginPlay` → @471、`Timeline_0__UpdateFunc` → @941 → @738、`Timeline_0__FinishedFunc` → @15。
- t=0 の処理
  - @471〜@547: `SetActorLocation(プレイヤーの位置, False, …, False)`（回転はスポーン時のプレイヤーの向きのまま）。
  - @587: `PlaySoundAtLocation(Stun_Wave_Attack_New_04, (0,0,0), (0,0,0), 1.0, 1.0, 0.0, None, None, None)`（テレキネシスと同じ音）。
  - @656〜@667: `GetAllActorsWithTag('Enemy')`。
  - @700 → @443〜@410 の ForLoop で、`ObjToInterfaceCast(DD_EnemyInterface_C)`（@112）が成功したものに **`Player Vanish()`**（@169）を呼ぶ。**この 1 回だけ。**
  - @705: `Timeline_0.PlayFromStart()`。
- 更新（@738〜@891）
  - `PostProcess.BlendWeight = Lerp(1, 0, float2)`
  - `PostProcess1.BlendWeight = MapRangeClamped(float2, 0, 0.3, 1, 0)`
- 終わり（@15）: `K2_DestroyActor()`（t=2.0 s）。

### 3.5 コンポーネント・後処理・タイムライン（`BP_VanishPower.json`）

SCS の構成:

- `DefaultSceneRoot`
  - `PostProcess`
  - `PostProcess1`
  - `ParticleSystem`

| export | 値 |
|---|---|
| `ParticleSystem_GEN_VARIABLE` | `Template /Game/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff`、**`RelativeLocation [92.42288208007812, -0.00042724609375, -152.14666748046875]`**、`PrimaryComponentTick.bStartWithTickEnabled false`（`bAutoActivate` は既定 = true） |
| `PostProcess_GEN_VARIABLE` | `bOverride_ColorSaturation`, `bOverride_ColorGain`、`ColorSaturation [0,0,0,1]`、**`ColorGain [0.6976670026779175, 0.0, 1.6100000143051147, 1]`**（紫）、`BlendWeight 0` |
| `PostProcess1_GEN_VARIABLE` | `bOverride_ColorGamma`（既定値）、`bOverride_ColorGainMidtones`、`bOverride_SceneFringeIntensity`（**値は既定の 0**。テレキネシスの 50 と違う）、`bOverride_GrainIntensity`（既定の 0）、`ColorGainMidtones [100,100,100,1]`、`BlendWeight 0`。override の無い `ColorSaturation [0,0,0,1]`、`ColorGain [1.61, 0.134167, 0, 1]` は効かない |

`Timeline_0_Template`（長さ 2.0）:

- float、desaturation、opacity の各トラックはテレキネシスと同じキー。
- **float2 だけが違い**、(-0.011600494384765625, 0.0, -0.0950283631682396, -0.09502881020307541) → (**0.30000001192092896**, 1.0, **1.0569698810577393**, 1.0569703578948975)。

計算値:

| t (s) | 0 | 0.03 | 0.06 | 0.09 | 0.12 | 0.15 | 0.18 | 0.21 | 0.24 | 0.27 | ≥0.30 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| float2 | 0.0026 | 0.0407 | 0.1167 | 0.2216 | 0.3463 | 0.4817 | 0.6186 | 0.7481 | 0.8608 | 0.9478 | 1.0 |
| PostProcess | 0.997 | 0.959 | 0.883 | 0.778 | 0.654 | 0.518 | 0.381 | 0.252 | 0.139 | 0.052 | 0 |
| PostProcess1 | 0.991 | 0.865 | 0.611 | 0.261 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

画面は 0.3 s で白飛びと紫の単色から戻る。**15 s の間ずっと続く色の変化は無く、効果中の画面の変化は §3.7 のウィジェットだけ。**

### 3.6 パーティクル `PPP_VanishPuff`

**システム全体**

- `lod_distances [0, 2500, 5000]`、固定の外接箱（±約 4800）
- マテリアル `M_LoopingSmoke1_Sheet`（Translucent、`CameraDepthFade`、テクスチャ `T_LoopingSmoke_8x8`）
- エミッタ 1 つ（スプライト）

**LOD0 の値**

- `EmitterDuration 3.0`、`EmitterLoops 1`、SubUV 8×8、`PSUVIM_Linear_Blend`
- 出現: 率 0、バーストは t=0 に 5 個
- 寿命 0.5〜1.0。大きさ 200〜250 × `SizeScale (1.5, 1.0, 1.0)`
- 速度 0
- `ColorOverLife`: (3.6956191062927246, 1.4101959466934204, 10.0) → (1,1,1)。`AlphaOverLife`: 1 → 0
- `LocationWorldOffset (0,0,150)`（ワールド空間なので拡大は掛からない）
- `AccelerationDrag 5.0`（ワールド）。`Location` は乱数 (-50,-50,-50)〜(50,50,50)
- 回転は乱数 0〜1、回転速度 0〜0.1
- SubUV の番号 0 → 30（寿命、64 標本）
- `AccelerationConstant (0,0,250)`（ワールド）

**出る位置（計算・推測）**

- アクタ（カプセルの中心）から、プレイヤーの向きで前へ 92.4 cm。
- 高さは −152.1 + 150 = −2.1 cm。**床から約 86 cm、プレイヤーの胸の前に、紫白の煙が 5 つ出て上へ加速して消える。**
- 粒子が実際に出るのは Tick の中なので、BeginPlay で移した後の位置になる（推測）。

### 3.7 画面の演出（`UMG_Vanish`）

**ウィジェット木（`_assets/.../UI/Main/Powers/UMG_Vanish.json`）**

- `CanvasPanel_0`
  - `CanvasPanelSlot_0`（アンカー (0,0)〜(1,1)、Offsets Right 0 / Bottom 0 = 画面全体）
    - `Image_82`
      - `Brush.ResourceObject /Game/Materials/Special/MM_WobblyVignette`
      - **`ColorAndOpacity [0.27869701385498047, 0.16055700182914734, 0.5364580154418945, 1.0]`**（紫）
      - `RenderTransform.Scale [1.05, 1.05]`

**CDO**

- `Speed 1.0`（発動側が 15.0 を入れる）

**バイトコード（`UI/Main/Powers/UMG_Vanish.txt`）**

- `Construct` → @10: `PlayAnimation(Vanish, StartAtTime 0.0, NumLoops 1, PlayMode 0 (Forward), PlaybackSpeed = 1.0/Speed, bRestoreState False)`
- Speed=15 なので、再生速度は 1/15。

**アニメ `Vanish`（`Vanish_INST`）**

- MovieScene の `DisplayRate 20`、再生範囲 0〜60000 ティック。
- `TickResolution` は書き出しに無いので `UMovieScene` の既定の 60000（pak_reference/README.md の 427 行）。**長さは 1.0 s で、1/15 倍速にすると 15 s。**
- トラックは `Image_82` の `RenderOpacity`（`MovieSceneFloatTrack_0`）の 1 本だけ。
- キー（ティック: 値、補間）: 0: 0.0 (Cubic/Auto, 接線 0)、6000: 1.0 (Cubic/Auto, 接線 1.0416666782475659e-06)、54000: 1.0 (Cubic/Auto, 接線 -1.0101009593199706e-06)、60000: 0.0 (Cubic/Auto, 接線 0)。
- 実時間では、**0〜1.5 s で 0→1、1.5〜13.5 s は 1、13.5〜15 s で 1→0**。接線はほぼ 0 で、両端が緩む。
- イーズの関数は既定（`EaseInFunction` / `EaseOutFunction` に値なし）。

**マテリアル `MM_WobblyVignette`（`_assets/.../Materials/Special/MM_WobblyVignette.json`）**

- `MaterialDomain MD_UI`、`BlendMode BLEND_Translucent`
- `EmissiveColor` ← `TextureSample_0`（`T_VignetteNew`、UV は `TextureCoordinate_2`）
- `TextureSample_2` / `_4`（`T_perlinnoise`、`SAMPLERTYPE_LinearGrayscale`、UV は `Panner_3` / `Panner_2`）
- `TextureSample_5`（`T_VignetteNew`）
- 関数 `/Engine/Functions/Engine_MaterialFunctions02/Utility/LinearSine`
- **cook でノードの一部（パンナーの速さ、係数、不透明度の配線）が消えている。** 「縁が揺らぐビネット」の作り方は推測で組み直すしかない。
- `T_VignetteNew` は 1024²、B8G8R8A8、sRGB、`TC_EditorIcon`。RGB は白、α は中央 0・縁 196〜229（WebGL の 13 記録）。
- `T_perlinnoise` は 2048²、G8、リニア。
- ウィジェットの色が紫なので、縁が紫に揺らぐ画面になる（推測）。

**プレイヤーの Chameleon などの後処理について**

- `Chameleon FX` は Vanish のコードから触られていない。
- 15 s の間に続く後処理は無い（§3.5）。

### 3.8 見えない扱いの仕組みと、敵の反応

**A. 衝突チャンネル（主の仕組み）**

- 敵の視線判定は `LineTraceSingle(..., TraceChannel = ByteConst 1, bTraceComplex True, ActorsToIgnore, DrawDebugType 0, ...)` の命中アクタがプレイヤーかどうかで決まる。
  - `BP_DD_Character_Base.Player Trace`（`Blueprints/Shared/BP_DD_Character_Base.txt` @524）
  - `RN` の `Can See Player`（@455）
  - `BP_06_Miniboss_viewcone.Player In Full View?`（`Blueprints/06_Hospital/Miniboss/BP_06_Miniboss_viewcone.txt` @158）
- TraceTypeQuery2 は ECC_Camera。`DefaultEngine.ini` に独自のトレースチャンネルは無い（`Teleport` は `bTraceType=False`）。
- **カプセルが Camera を Ignore していると、線はプレイヤーを素通りして後ろの壁に当たるので、「見えない」になる。**
- 視野の条件（参考）: `DegAcos(Dot(敵の前方, 敵→プレイヤー)) < 100.0`。
- プレイヤーの `CharacterMesh0` にはメッシュが無い。`Tablet` と `Widget` は NoCollision（`BP_DD_PlayerCharacter.json`）。ほかに当たる部品は無い（推測）。

**B. `Active Powers` に 5 が入っているか（`BP_DD_Functions.Is Player Using Power ?`、`Blueprints/Macros/BP_DD_Functions.txt` 3246 行）**

- `BTS_ActorInRange`（@164）: 敵の `Get State` が 2（Stun）か Vanish 中なら、`IsInRange?` を False にする。
  - 使うのはサルの BT（`DD_BehaviorTree_Monkey`、`_Monkey_Chef`）だけで、病院は使わない。
- `BP_03_Watcher`（@6276 付近）
- `BP_07_SecurityCamera`（遊園地の監視カメラ。@1419 で byte 5、`Can See? AND NOT Active Powers.Contains(5)`）
- **病院に監視カメラ（`BP_07_SecurityCamera`）は無い**（Zone 1・2 の full.json のクラス一覧）。

**C. `DD_EnemyInterface.Player Vanish`（使った瞬間に 1 回）**

- 基底の `BP_DD_Character_Base.Player Vanish` は @81 → @119 で何もしない。

実装している例:

| 敵 | 入口 | 処理 |
|---|---|---|
| `BP_06_ReaperNurse`（病院の通常のナース。Zone 1 の `Spawn Nurses` が出す。子の `_Zone2` は Zone 2 の `Spawn Nurses`） | RN 2807 行 → @10871 | `Seen Player Recently = False` だけ |
| `BP_Agatha1` | @11072 | `Player Seen Recently? = False` |
| `BP_Ducky` | @12779 | `Player Seen Recently? = False` |
| `BP_GremClown` | @8756 | `Chase Player? = False` → `Delay(0.2)` → @3885（`Boss?` で分かれ、歩く速さ 425/375/450 などを入れ直す処理。追跡をやめた状態に戻すと推測） |
| `BP_MonkeySpecial` | @687 | `PopExecutionFlow` だけ（何もしない。サルは B の BTS で止まる） |

**ReaperNurse の流れ（RN）**

- Tick（→ @10743 → @6644 …）から @7107 へ進む。
  - `State==2`（Stun）なら別の処理。
  - `Seen Player Recently` が True なら `Chase Player()`（@7524）。追跡では、`Seen=True`、`Point Of Interest` = プレイヤーの位置、`Set Walk State(False)`、`CreateMoveToProxyObject(Target=Player Target, AcceptanceRadius 5.0)` を行う。
  - False なら `Not Seeing Player()`（@7940）。
    - `Point Of Interest` が 0 でなければ、そこへ `MoveTo(AcceptanceRadius 5.0)`。
    - 着いたか失敗したら（@5551 / @10818 → @5578）`Point Of Interest = 0` にする。
    - 0 なら `Random Point Destination` へ `MoveTo(50.0)` し、`Set Walk State(True)`。
  - 並行して `Can See Player`（@7423）が True なら、`Seen=True` と `RetriggerableDelay(3.0)` の後、@3813 で `Seen=False` と `Reset Detection` を行う。
- **Vanish の瞬間の反応**
  - 追跡をやめ、最後に見た位置（`Point Of Interest` は消されない）へ歩いて行く。
  - その後は徘徊に戻る。
  - 15 s の間は `Can See Player` が成り立たないので、再び見つからない。
- **触れられた場合**
  - プレイヤーに触れたときの捕獲（`BndEvt__Sphere...` → @9818）は、`OtherActor == Player` と `State != 2` しか見ない。
  - **Vanish 中でも触れれば捕まる**（Vanish の判定は無い）。

**病院の特殊な敵**

- `BP_06_ReaperNurse_06_Chase`（Zone 1 の `Spawn Nurses_06` が出す）
  - `ReceiveTick`（→ @1034）が、Stun でなければ常に `Chase Player()` を呼ぶ（@1103 以降）。
  - `Chasing` は常に True を返す。
  - **Player Vanish で `Seen` が False になっても、次の Tick でまた追う。Vanish では止まらない**（Tick の分岐の一部は推測）。
- `BP_06_ReaperNurse_Sentry`（Zone 2 に 6 体配置。親は 06_Chase）
  - `ReceiveTick`（→ @1018）は `Chasing` が True のときだけ、親の Tick（@657、常に追う）を呼ぶ。
  - `Chasing` は `BPI_06_Viewcone.Player Spotted`（→ @1035）で立つ（推測: @720 以降）。
  - 見つかる前はビューコーン（A の仕組み）で見えない。**見つかった後は Vanish で止まらない**（推測）。
- `BP_06_Matron_MiniBoss`（Zone 2）
  - super は Actor で、タグ `Enemy` はあるが `DD_EnemyInterface` を持たない。**Player Vanish は呼ばれない**（キャストに失敗する）。
  - ビューコーン（`BP_06_Miniboss_viewcone_Matron_Long/Short`）を使うので、A の仕組みで見えなくなる。
  - `BP_06_Miniboss_viewcone` の `Player In Full View?` は、`bOn`、自分の親アクタを無視するトレース、命中が `BP_DD_PlayerCharacter`、距離 ≤ `Length`、`Player Inside Cone?` をすべて満たすときに真になる。

### 3.9 途中で解除される条件

- **バイトコードに、効果中に解除する処理は無い。**
  - Camera の応答を Block に戻す箇所は @7334 だけ。
  - `Active Powers` から 5 を消す箇所は @7274 だけ。
  - 他の `Array_RemoveItem(Active Powers, …)` は 0 / 1 / 2 / 3 番（PC 139・1182・1507・8773 行の `Temp_byte_Variable_1/6/8/3` = 0/2/3/1）。
- シャードを取る、別のパワーを使う、捕まる、のどれでも解除されない。
- 死んで復活したとき（`UMG_DeathScreen` @208 → `Reset Powers` → PC @37130 → `PowerController.Reset All Powers`）
  - `Stop Vanish Timeline` と `Reset Vanish`（PC @37737 → @27838。ゲート `_3` を閉じ（@27826）、@7927 の回復を直接呼ぶ）が走る。
  - 枠は満タンに戻り、音が鳴り、ウィジェットは消える。
  - **カプセルの Camera=Ignore と Active Powers の 5 は、元の 15 s の Delay が終わるまで残る。**
  - 15 s が経つと @6856 で `Set Delay(クールダウン)` と Delay が走り、クールダウンの後にもう一度回復する（音も鳴る）。
  - 復活直後に再び使った場合、`Delay` ノードは実行中の同じ遅延を重ねて登録しないので、先の遅延で終わる（推測: UE の `FDelayAction` の仕様）。

---

## 4. 病院 Zone 1・Zone 2 での特別扱い

- `_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`（5301 行）と `06_Hospital_Zone_02.txt`（7194 行）を grep した。
  - `Telekinesis`、`Vanish`、`Active Powers`、`Player Vanish`、`PowerController`、`Upgrade` は見つからなかった。
  - `Activate(` は瓦礫の `ParticleSystemComponent.Activate`（Zone 1 の 320 行）、実績の非同期処理（Zone 2 の 1164・4337 行）、マトロンと敵の起動（Zone 2 の `Activate MiniBoss Enemies`、7161・7170 行）だけで、テレキネシスとは関係ない。
- **特別扱いは無い。** パワー側で特別扱いされるレベルは `00_Ballroom` だけ（テレキネシスのクールダウン 1.0 s、PC @8520）。
- レベルが持つもの
  - Zone 1: `BP_Shard_C` 337、`BP_BonusShard_C` 1。敵は `Spawn Nurses`（`BP_06_ReaperNurse_C`）と `Spawn Nurses_06`（`BP_06_ReaperNurse_06_Chase_C`）で出る（Zone 1 の 4755・4923 行）。
  - Zone 2: `BP_Shard_C` 342、`BP_BonusShard_C` 1、`BP_06_ReaperNurse_Sentry_C` 6、`BP_06_Matron_MiniBoss_C` 1、ビューコーン（Nurse 6、Matron Long 1、Short 1）。`Spawn Nurses` は `BP_06_ReaperNurse_Zone2_C` と Sentry を扱う（Zone 2 の 6919 行）。
  - 病院に `BP_Shard_Special` は無い。
  - Zone 1 にはシャードを消すループ（`GetAllActorsOfClass(BP_Shard_C)` → `K2_DestroyActor`、Zone 1 の @9190〜）もある。テレキネシスとは別の処理。
- Zone 2 の `PostProcessVolume_1`（`bUnbound`）は `ColorSaturationShadows`、ブルーム、AO、`ColorGradingIntensity 0`、LUT だけを上書きする。パワーの後処理（Saturation / Gain / GainMidtones / Fringe）とは項目が重ならない。

---

## 5. 使っているアセットと実ファイル

実ファイルは `pak_reference_2/` からの相対パス。すべて存在を確かめた（`OK`）。

| 種類 | アセット | 使う場所 | 実ファイル |
|---|---|---|---|
| BP | `/Game/Blueprints/Main/Powers/BP_TelekinesisPower` | テレキネシス | `_assets/...json`、`_bytecode/...txt` |
| BP | `/Game/Blueprints/Main/Powers/BP_VanishPower` | Vanish | 同上 |
| インターフェイス | `/Game/Blueprints/Characters/Shared/DD_TelekinesisInterface`（`Activate`） | テレキネシス | 同上 |
| インターフェイス | `/Game/Blueprints/Characters/Shared/DD_EnemyInterface`（`Player Vanish` ほか） | Vanish | 同上 |
| パーティクル | `/Game/ThirdParty/AdvancedMagicFX09/Particles/P_ky_forceField_Telekinesis` | テレキネシス | `_particles.json` |
| パーティクル | `/Game/ThirdParty/PyroParticlePack/Particles/PPP_VanishPuff` | Vanish | `_particles.json` |
| パーティクル | `/Game/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3`（エミッタ 7、マテリアル `MI_ky_flare01_primitiveG/R`・`MI_ky_primitive2_trs`・`M_ky_empty`・`M_ky_polarGlow02`・`M_ky_primitive_dyn2`） | シャードの回収 | `_particles.json` |
| メッシュ | `SM_ky_sphere`（半径 10） | TK の sphere | `_meshes_gltf/ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_sphere.gltf` |
| メッシュ | `SM_ky_windLine27midPoly`（外接球 15.70） | TK の aura | `_meshes_gltf/ThirdParty/AdvancedMagicFX09/Meshes/SM_ky_windLine27midPoly.gltf` |
| メッシュ | `soul_shard_skeletal` ＋ アニメ `soul_shard_skeletal_anim_loop` | シャード | `_meshes_gltf/Meshes/Ring_Assets/soul_shard_skeletal.gltf`、`_anims_psa/Meshes/Ring_Assets/soul_shard_skeletal.psk`・`soul_shard_skeletal_anim_loop.psa` |
| マテリアル | `MI_ky_aura7c`（親 `M_ky_aura7`、TwoSided、Translucent、Unlit。`hilightDensity 1.5`、`hilightPower 5.0`、`maskU 1.0`、`maskV 0.2`、`maskRadiusControl [0.5,0,0.125,2.0]`） | TK | テクスチャ `DDeception/Content/ThirdParty/AdvancedMagicFX09/Textures/T_ky_maskRGB5.png` |
| マテリアル | `MI_ky_shockWave02_4x4_nonD`（親 `M_ky_shockWave02_4x4`。`depthFade 0.0`、`hilightDetailPower 10.0`、`coreColor [1.21858,1.201712,2.0,1]`、`baseTex T_ky_circle01_4x4`） | TK | `.../AdvancedMagicFX09/Textures/T_ky_circle01_4x4.png`、`T_ky_maskRGB3.png` |
| マテリアル | `MI_ky_starDust_sq`（親 `M_ky_starDust`。`maskDensity 1.0`、`maskRadius 0.5`） | TK | `.../AdvancedMagicFX09/Textures/T_ky_dust_longStar.png` |
| マテリアル | `M_ky_wall02_4x4_two`（Translucent、Unlit、TwoSided。スカラー `opacity` 既定 0.1、ベクトル `baseColor` 既定 [0.060628,0.069241,0.145,1]、SubUV のテクスチャ `baseTex`） | TK の sphere | `.../AdvancedMagicFX09/Textures/T_ky_wall02_4x4.png` |
| マテリアル | `M_LoopingSmoke1_Sheet`（Translucent、ParticleSubUV、CameraDepthFade） | Vanish の煙 | `DDeception/Content/Particles/Shared/SmokeTest/T_LoopingSmoke_8x8.png`（4096²、DXT5、sRGB） |
| マテリアル | `MM_WobblyVignette`（MD_UI、Translucent） | UMG_Vanish | `DDeception/Content/UI/Menu/Streaks/T_VignetteNew.png`、`DDeception/Content/Textures/FX_Textures/T_perlinnoise.png` |
| マテリアル | `MM_Powers_Inst_Telekinesis` / `MM_Powers_Vanish`（親 `MM_Powers`、Translucent、Unlit。`Percent 1.0`。`DisabledPower` = `ring_altar_power_*_icon`、`EnabledPower` = `ring_altar_power_*_icon_inactive`） | タブレットの枠 | `DDeception/Content/UI/RingAltar_UI/Textures/ring_altar_power_telekinesis_icon{,_inactive,_hover,_selected}.png`、`ring_altar_power_vanish_icon{,_inactive,_hover,_selected}.png` |
| マテリアル | `m_crystal_Inst1`（シャードの結晶。`emissive_entensity 25.0` ほか）、`M_Shard` | シャード | — |
| ウィジェット | `/Game/UI/Main/Powers/UMG_Vanish` | Vanish | `_assets/.../UI/Main/Powers/UMG_Vanish.json`、`_bytecode/.../UMG_Vanish.txt` |
| 音 | `/Game/Audio/SharedGameplay/Stun_Wave_Attack_New_04`（SoundWave、1.710045 s、2ch、44.1 kHz、SFX、減衰なし） | TK・VA の発動 | `DDeception/Content/Audio/SharedGameplay/Stun_Wave_Attack_New_04.ogg` |
| 音 | `/Game/Audio/UI/power_refilled`（1.514667 s、2ch、48 kHz、SFX） | 回復 | `DDeception/Content/Audio/UI/power_refilled.ogg` |
| 音 | `/Game/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue` → `Soul_Shard_Pickup_v2` | シャードの回収 | `DDeception/Content/Audio/SharedGameplay/Soul_Shard_Pickup_v2.ogg` |
| 同時発音 | `/Game/Audio/OnlyFew`（MaxCount 1、StopOldest、VolumeScale 0.5） | シャードの回収 | `_assets/.../Audio/OnlyFew.json` |
| カメラシェイク | `/Game/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop` | TK | `_camera/_camera_shakes.json`、`_assets/.../01_Hotel_Lobby_ElevatorShakeStop.json` |
| カメラシェイク | `/Game/Blueprints/Shared/BP_CameraShake_ShardCollect` | シャードの回収 | 同上 |
| DataTable | `PowersTable_Telekinesis` / `PowersTable_Vanish` | リングの祭壇の表示 | `_datatables.json` |

---

## 6. UE 5.8 で再現するときの注意

1. **Cascade はまだ使える。**
   - UE 5.8 に `UParticleSystem`、`UParticleSystemComponent`、`UGameplayStatics::SpawnEmitterAtLocation(…, Scale, bAutoDestroy, EPSCPoolMethod, bAutoActivate)` と、各モジュールのソース（`Engine/Source/Runtime/Engine/Private/Particles/`）が残っている。クラス自体に非推奨の印は無い（`ParticleSystem.h` の `UE_DEPRECATED` は別の関数だけ）。
   - 方針「UE に同じ仕組みがあれば値を写す」に従えば、Cascade のアセットを値どおりに組むのが最も忠実。
   - ただし、エミッタやモジュール、分布（`FRawDistribution` の表）を Python や MCP から組めるかは**未確認**。できなければ Niagara（Cascade→Niagara 変換や手組み）になる。
   - 使っている機能: メッシュ粒子、`ParticleModuleLight`（粒子の灯）、`Orbit`、`ParameterDynamic`（マテリアルの動的パラメータ）、`LocationPrimitiveSphere`、`OrientationAxisLock`、`SubUV`（Linear_Blend）、`SizeMultiplyLife`、`SourceMovement`、`AccelerationDrag` / `AccelerationConstant`（ワールド）、`LocationWorldOffset`。
2. **拡大と加算**
   - コンポーネントの拡大 2 は、大きさと出現位置のオフセットの両方に掛かる。
   - Size モジュールは 0 からの加算（UE 5.8 のソースで確認）。
3. **マテリアルのノードは失われている**
   - cook で消えた（`M_ky_*` はパラメータの既定値とテクスチャだけ、`MM_WobblyVignette` は式の一部だけ）。
   - `MI_*` のパラメータ値は取れるが、親の式は作り直しになる。見た目を合わせるには、実機の観察（verification.md の作法）が要る。
4. **後処理**
   - UE 5.8 の `FPostProcessSettings` にも `ColorSaturation`、`ColorGain`、`ColorGainMidtones`、`ColorGamma`、`SceneFringeIntensity` はある（`Engine/Classes/Engine/Scene.h` の 1514・1520・1555・1621 行）。
   - **`GrainIntensity` は無く、`FilmGrainIntensity` に置き換わった。** Vanish の PostProcess1 の `bOverride_GrainIntensity` は既定値 0 での上書きなので、実害は無い（写さなくてよい、推測）。
   - UE5 はトーンマッパーの既定が違う。`ColorGainMidtones 100` の白飛びや、彩度 0 × ゲインの色は見比べが要る。
   - 00 記録にあるとおり、本作は露出を固定にしている。
   - UPostProcessComponent は `bUnbound` の既定が true（推測）。アクタに付けて `BlendWeight` をタイムラインで動かすやり方が、そのまま使える。
5. **カメラシェイク**
   - UE 5.8 に `UMatineeCameraShake` は無い（`MatineeCameraShake.h` は 5.5 で非推奨のヘッダ）。
   - 旧式の値は `ULegacyCameraShake`（EngineCameras プラグイン、`Plugins/Cameras/EngineCameras/Source/EngineCameras/Public/Shakes/LegacyCameraShake.h`）に、`OscillationDuration`、`LocOscillation` などをそのまま写せる。
   - 呼び出しは `APlayerController::ClientStartCameraShake(Shake, Scale, ECameraShakePlaySpace::CameraLocal, Rot)`。**Scale 25 で振幅が 25 倍になる**のを忘れないこと。
6. **ExpoIn**
   - `UKismetMathLibrary::VEase(A, B, Alpha, EEasingFunc::ExpoIn, 2.0, 2)` がそのまま使える（`FMath::InterpExpoIn`）。
7. **トレースと重なりの判定**
   - `UKismetSystemLibrary::SphereOverlapActors` と `LineTraceSingle(ETraceTypeQuery::TraceTypeQuery2)` が使える。
   - 本作の敵（これから作る）の視線判定を **ECC_Camera のトレース**にし、プレイヤーのカプセルの Camera 応答を切り替えれば、原作と同じ仕組みになる。
   - 加えて「Vanish 中」のフラグ（`Active Powers` 相当）と、使った瞬間の `Player Vanish` の通知（タグ `Enemy` ＋インターフェイス）も用意すると、原作の 3 経路がそろう。
8. **音**
   - `PlaySoundAtLocation((0,0,0))` を減衰なしのステレオで鳴らすのは、実質 `PlaySound2D`（推測）。
   - 取得音の同時発音の制限（`OnlyFew`）は `USoundConcurrency` に写せる。
9. **UMG**
   - アニメの長さ 1 s を `PlaybackSpeed = 1/Speed` で引き伸ばしている。
   - C++ で作るなら、RenderOpacity を 15 s の区分曲線で動かすだけでよい。
   - ウィジェットはクールダウンが終わるまで viewport に残る（不透明度 0）。
10. **テレキネシスはシャードが前提**
    - 本作のシャードは未実装（M3。`WasamiPlayerCharacter.h` の `ShardActorClass` は枠だけ）。
    - シャード側の `Activate`（寄せと回収）が要る。§2.8 の旧版との差の確認も必要。
11. **レベル名の特別扱い**
    - `GetCurrentLevelName == '00_Ballroom'` の 1.0 s は、本作では該当しない（病院だけ）。

---

## 7. 推測・未確定の一覧

| 項目 | 状態 | 何を見たか |
|---|---|---|
| `Stun_Wave_Attack_New_04` を原点で鳴らすのが実質 2D | 推測 | SoundWave に減衰の設定が無いこと（`_assets` の JSON） |
| PostProcessComponent の `bUnbound=true`・Priority 0 | 推測（UE の既定） | 書き出しに値が無い |
| シャードのカプセルの応答が全部 Overlap で、壁を抜ける | 推測 | UE 5.8 の `UShapeComponent` のコンストラクタ（OverlapAllDynamic）。4.24 の確認はしていない |
| `ActorsToIgnore` が空 | 推測 | `Temp_object_Variable` が何もつながっていないローカル変数 |
| `bDisabled` のシャードが重なりの問い合わせに入らない | 推測 | `SetCollisionEnabled(NoCollision)` |
| パーティクルの大きさ（0 からの加算、×2）、`ground` の高さ | UE 5.8 のソースで確認。4.24 も同じと推測 | `ParticleEmitterInstances.cpp`、`ParticleModules_Size.cpp`、`ParticleModules_Location.cpp`、`ParticleSystemRender.cpp` |
| SubUV の番号がコマ数を超えたときに繰り返す | 推測（サンプラの Wrap） | UE 5.8 のシェーダ `ParticleSpriteVertexFactory.ush` と `ParticleSystemRender.cpp` |
| `ElevatorShakeStop` の `InitialOffset` がランダム | 推測（UE4 の既定） | 書き出しに値が無い |
| `MM_WobblyVignette` の見た目 | **未確定** | cook で式の大半が消えている。残っているのはテクスチャ 4 つ、`LinearSine`、パンナーの名前だけ |
| `BP_06_ReaperNurse_06_Chase` / `_Sentry` が Vanish で止まらない | 推測（Tick の分岐は部分的に読んだ） | `_06_Chase` @1034〜@1150、`_Sentry` @1018〜@1035・@637〜@720 |
| `BP_GremClown` の @3885 以降の意味 | 推測（速さの入れ直し） | 冒頭の数行だけ読んだ |
| 復活直後の 2 回目の Vanish の遅延 | 推測（`Delay` ノードの仕様） | PC @22969 と @6856 |
| Vanish の煙が BeginPlay で移した後の位置に出る | 推測（粒子は Tick で出る） | `PrimaryComponentTick.bStartWithTickEnabled false`、自動起動 |
| Cascade のアセットを MCP / Python で組めるか | **未確定** | 調べていない（エディタを触っていない） |

---

## 8. 旧版（pak_reference）との関係

- `pak_reference/_bytecode/.../Powers/` に `BP_TelekinesisPower` と `BP_VanishPower` は無い（Teleport と Primal だけ）。旧版は UI（`UMG_TabletPowers*`、`UMG_RingAltar`、`BP_DD_GameMode`）が名前を持つだけ。**2 つのパワーの根拠は pak_reference_2 しかない。**
- 新版で変わったもの（`_diff.json`）
  - `BP_Shard`: `Activate`、`Enable`、`ReceiveTick`、`Shard Pull` が足された。
  - `BP_DD_PlayerCharacter`: `Get Power Upgrade Level`、`Use Power Left/Right`、`Reset Telekinesis/Vanish` など。
  - シャードは「両方にあって違う」ものに当たる。
