# テレポーテーション（BP_Power_Teleport）原作データ調査 — UE5.8 C++ 実装のための裏づけ

調査日: 2026-09-16。読み取りのみ（リポジトリ・エディタは変更していない）。
版の方針: **テレポーテーションは旧版 `pak_reference/`（UE 4.21）に従う**（ユーザーの決定）。ステージは `pak_reference_2/`（UE 4.24）の病院 Zone 1・Zone 2。

表記: 「確定」はバイトコード・アセット・設定・エンジンソースで確かめた値。「推測」は根拠が間接的なもの。`@数字` はバイトコードの実行時オフセット（`.txt` の行頭の値。Jump・EntryPoint が指すのもこの値）。

---

## 0. 要点（結論）

1. **流れ（確定）**: Q（`Use Power Left`）→ プレイヤーが `BP_Power_Teleport` を「プレイヤー位置 − (0,0,5000)、回転 0」に AlwaysSpawn で出す → アクタは毎 Tick に「プレイヤーのアクタ位置 + アクタ前方 × Distance」から真下 500 cm を **ObjectTypeQuery7（= ECC_GameTraceChannel1「Teleport」）** で複雑コリジョンのトレースをし、当たった点へ SpringArm を `SetWorldLocation(teleport=true)` で動かす（デカールとパーティクルは SpringArm の子）。スポーン 0.5 s 後に SpringArm の位置ラグ（速度 10、UE 既定）を有効にする。左クリック（キー直結、IE_Pressed、入力は消費しない）で「デカールのワールド位置 + (0,0,125)」を控え、DoOnce の中で `PlayCameraAnim(CameraAnim_Teleport, 1,1,0,0,false,false,0,CameraLocal)`、カプセルの WorldDynamic・Pawn を Ignore、`Delay(0.12)`。0.12 s 後に `PlayCameraShake(BP_CameraShake_Streak, 1, CameraLocal)`、`PlaySound2D(Teleport_Committed, 1.0, 1.0)`、`Player.SetActorLocation(Location, sweep=true, teleport=true)`、カプセルの Pawn・WorldDynamic を Block に戻し、`Used` を放送して自分を破棄。
2. **クールダウン（確定）**: `Used` → プレイヤーの `UsedTeleport` が `Set Delay(5.0)`（レベル名が `00_Ballroom` なら 1.0）で枠を 0→1 に 5 s で満たし、同時に `Delay(5.0)` の後で `power_refilled`（音量 0.5）を鳴らして使用可能に戻す。照準に入った瞬間は `Set Delay(0.05)` で枠を 1→0 に 0.05 s で落とす。
3. **照準中の Q（確定）**: 「使用中の一覧に Teleport がある」かつ「押した側 == 使った側」なら `BP_Powers.Reset Teleport` → プレイヤーの `Reset Teleport`（`power_refilled` 0.5・使用可能に戻す・アクタ破棄）→ 枠を 1.0 に。クールダウンは付かない。それ以外の使えない Q は `power_not_ready`（0.35）。
4. **新しい発見（WebGL 版の記録に無い）**: (a) 使用可能なパワーを発動すると 0.5 s は**どちらの枠のパワーも**発動できない共有の DoOnce がある（プレイヤー @9729〜・@14412）。(b) 確定の DoOnce は `Location` の代入の**後ろ**にあるので、0.12 s の間の 2 回目のクリックは移動先だけを更新する。(c) 床を 0.5 s 以内に捉えなかった場合、以後に捉えた床へデカールはスポーン位置（50 m 下）からラグで追ってくる。(d) クリックは入力を消費しないので、プレイヤーの左クリック（手持ちアイテムの Use / カメラ前方 200 cm の InteractWithObject）も同時に走る。
5. **照準の輪の見た目（確定、WebGL 版の記録の訂正）**: パーティクルコンポーネントのワールド変換は「回転 ≒ 恒等（world 軸）、スケール (0.66528, 0.2, 0.2)、デカール原点（当たった点）の **13.75 cm 上**」。斬撃のスプライトは `PSA_Square` なので**正方形**（一辺 300 × 0.665 = 199.6 cm、寿命で 1→2 倍）で、WebGL 版の「楕円 300 × (0.665, 0.2)」ではない（UE の `GetParticleSize` は PSA_Square で Size.Y = Size.X）。火花の出現範囲・速さはスケールを受ける（±113 × ±34 cm、上向き 60〜100 cm/s）。
6. **CameraAnim の FOV（2026-09-16 に旧版の実機で確定）**: アセットの `BaseFOV` は 137.24 だが、実効は `POV.FOV + (key(t) − key(0)) × 重み`（key(0) = 90）。旧版の 60 fps の収録で、クリックの直後に広がり、終わりで跳ばないことを確かめた（§5.1）。UE 4.21 のソースは手元に無く、`InitialFOV` の代入元は未確認のまま。
7. **UE5.8（確定）**: `UCameraAnim`/`UCameraAnimInst`/`PlayCameraAnim` は無い（5.8 のソースに無い）。`ULegacyCameraShake`（EngineCameras プラグイン、既定で有効。`MatineeCameraShake.h` は 5.5 で非推奨の別名）と Cascade（`UParticleSystem`、Cascade エディタ、`CascadeToNiagaraConverter`）は残っている。`FInterpCurve::Eval` の三次エルミートは UE4 と同じ式。SpringArm のラグの実装も同じで、`SetWorldLocation(teleport=true)` でラグはリセットされない。デカールの `DecalBlendMode` は UE5 で廃止（出力ピンから推定）。

---

## 1. `BP_Power_Teleport` の全処理（旧版）

根拠: `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_Power_Teleport.txt`（Ubergraph 2484 バイト）。

### 1.1 イベント入口（確定）

| イベント | Ubergraph の入口 | 備考 |
|---|---:|---|
| `ReceiveBeginPlay` | @1520（→ Jump @1060） | |
| `ReceiveTick` | @1525 | `K2Node_Event_DeltaSeconds` は未使用 |
| `InpAxisKeyEvt_MouseWheelAxis_K2Node_InputAxisKeyEvent_0` | @1279 | 軸**キー**直結（アクションマッピングではない） |
| `InpActEvt_LeftMouseButton_K2Node_InputKeyEvent_0` | @2476（→ Jump @2342） | キー直結 |
| Delay(0.12) の完了 | @15 | `SkipOffsetConst 15` |
| Delay(0.5) の完了 | @513 | `SkipOffsetConst 513` |
| `Used__DelegateSignature` | — | 引数なしのマルチキャストデリゲート |
| `UserConstructionScript` | — | 空 |

入力の束縛（`_assets/.../BP_Power_Teleport.json` の `InputKeyDelegateBinding_0` / `InputAxisKeyDelegateBinding_0`、確定）:
- `InputChord.Key = LeftMouseButton`（Shift/Ctrl/Alt/Cmd すべて false）、`InputKeyEvent = IE_Pressed`、**`bConsumeInput = false`**、`bExecuteWhenPaused = false`、`bOverrideParentBinding = true`。
- `AxisKey = MouseWheelAxis`、**`bConsumeInput = false`**、`bExecuteWhenPaused = false`、`bOverrideParentBinding = true`。
- CDO `AutoReceiveInput = EAutoReceiveInput::Player0`（PlayerController 0 の入力スタックに積まれる。優先度は既定 0、`bBlockInput` 既定 false）。
- 旧版 `DefaultInput.ini`（`C:\Users\User\Desktop\decrypt_dd\dd_extracted\DDeception\Config\DefaultInput.ini` 15 行目）: `+AxisConfig=(AxisKeyName="MouseWheelAxis",AxisProperties=(DeadZone=0.000000,Sensitivity=1.000000,Exponent=1.000000,bInvert=False))`。ホイールの値は 1 目盛り ±1（UE5.8 でも `FSceneViewport` が `InMouseEvent.GetWheelDelta()` を、Windows が `WHEEL_DELTA`(120) で割った値を渡す。確定）。奥へ回すと +1。

### 1.2 BeginPlay（@1520 → @1060、確定）

```
@1060 GetPlayerCharacter(0) → @1108 Cast<BP_DD_PlayerCharacter_C> → 失敗なら終了
@1183 Player = キャスト結果
@1202 Delay(0.5) → @513
@513  SpringArm.bEnableCameraLag = True
```

### 1.3 Tick（@1525、確定）

```
@1525 P  = GetPlayerCharacter(0)
@1573 L  = P.K2_GetActorLocation()          // カプセル中心
@1623 F  = P.GetActorForwardVector()         // アクタの前方（ピッチは使わない）
@1673/@1719 Start = L + F * Distance
@1765 End   = Start - (0, 0, 500)
@1815 ObjectTypes = [ByteConst 6]            // EObjectTypeQuery の添字 6 = ObjectTypeQuery7
@1828 LineTraceSingleForObjects(Self, Start, End, ObjectTypes,
        bTraceComplex = True, ActorsToIgnore = Temp_object_Variable（空配列）,
        DrawDebugType = 0 (None), OutHit, bIgnoreSelf = True,
        TraceColor = (1,0,0,1), TraceHitColor = (0,1,0,1), DrawTime = 5.0)
@1993 当たらなければ終了（SpringArm は前の位置のまま）
@2003 BreakHitResult → BreakVector(Location) → @2234 MakeVector（そのまま）
@2289 SpringArm.K2_SetWorldLocation(HitLocation, bSweep = False, SweepHitResult, bTeleport = True)
```

- トレース長は 500 cm（確定）。距離の原点はプレイヤーのアクタ位置（カプセル中心）。`BP_DD_PlayerCharacter` の CDO はカプセル半径 50（半高は ACharacter 既定の 88、上書きなし）。
- `Hit Distance` という変数はあるが未使用（確定）。
- 推測: プレイヤーは ACharacter 既定の `bUseControllerRotationYaw = true`（CDO で上書きなし）なので、アクタ前方 = 視点のヨー方向。

### 1.4 マウスホイール（@1279、確定）

```
@1279 v = AxisValue / 10.0
@1321 a = v + Alpha
@1367 Alpha = FClamp(a, 0.0, 1.0)             // @1414
@1441 Distance = Lerp(250.0, Max Distance, Alpha)   // @1492
```

- CDO: `Distance` 1000.0、`Alpha` 0.6000000238418579、`Max Distance` 1000.0、`Location` (0,0,0)。
- 軸の束縛は値が 0 のフレームでも毎フレーム呼ばれる（UE4 の InputAxisKey の仕様。推測ではなく UE の通常の挙動だが 4.21 ソースでは未確認）ので、`Distance` は毎フレーム再計算される。結果として Q のたびに Distance = Lerp(250, Max, 0.6)（Max 1000 なら 700 cm）。1 目盛り = Alpha ±0.1 = (Max − 250) × 0.1（Max 1000 なら 75 cm）。
- 推測: スポーン直後の最初の Tick が入力処理より先に走ると、そのフレームだけ CDO の Distance 1000 でトレースする（同じ TG_PrePhysics 内の順序は不定）。

### 1.5 左クリック（@2476 → @2342、確定）

```
@2342 PopExecutionFlowIfNot(True)            // 常に通過
@2344 D = Decal.K2_GetComponentLocation()
@2394/@2444 Location = D + (0, 0, 125)       // ★ DoOnce より前（2 回目のクリックでも更新される）
@2471 Jump @547
@547  DoOnce（Temp_bool_Has_Been_Initd_Variable / Temp_bool_IsClosed_Variable、StartClosed = false）
        閉じていれば何もしない
@582  IsClosed = True
@593  PCM = GetPlayerCameraManager(0)
@641  PCM.PlayCameraAnim(CameraAnim_Teleport, Rate 1.0, Scale 1.0, BlendInTime 0.0, BlendOutTime 0.0,
                         bLoop False, bRandomStartTime False, Duration 0.0,
                         PlaySpace ByteConst 0 = CameraLocal, UserPlaySpaceRot (0,0,0))
@786  Player.CapsuleComponent.SetCollisionResponseToChannel(ByteConst 1 = ECC_WorldDynamic, ByteConst 0 = ECR_Ignore)
@896  Player.CapsuleComponent.SetCollisionResponseToChannel(ByteConst 2 = ECC_Pawn,         ByteConst 0 = ECR_Ignore)
@958  Delay(0.11999999731779099) → @15
```

### 1.6 0.12 s 後（@15、確定）

```
@15   PCM = GetPlayerCameraManager(0)
@63   PCM.PlayCameraShake(BP_CameraShake_Streak_C, Scale 1.0, PlaySpace ByteConst 0 = CameraLocal, UserPlaySpaceRot (0,0,0))
@138  PlaySound2D(Self, /Engine/VREditor/Sounds/UI/Teleport_Committed, Volume 1.0, Pitch 1.0, StartTime 0.0,
                  ConcurrencySettings None, OwningActor None)
@197  Player.K2_SetActorLocation(Location, bSweep = True, SweepHitResult, bTeleport = True)
@307  GetPlayerCharacter(0).CapsuleComponent.SetCollisionResponseToChannel(2 = ECC_Pawn,         2 = ECR_Block)
@417  GetPlayerCharacter(0).CapsuleComponent.SetCollisionResponseToChannel(1 = ECC_WorldDynamic, 2 = ECR_Block)
@479  Used.Broadcast()
      K2_DestroyActor()
```

- `pak_reference/README.md` §4.7 は戻す処理を @351・@461 と書いているが、`.txt` の実行時オフセットでは @307・@417（内容は同じ）。
- 移動の間、カプセル（`Pawn` プロファイル、ObjectType Pawn）は WorldDynamic と Pawn を無視するので、スイープを止めるのは WorldStatic・PhysicsBody・Vehicle・Destructible と、カプセルが Block する独自チャンネル（旧版では `Malak`＝ECC_GameTraceChannel2、既定 Block）の物だけ（確定: ini と呼び出し。どの物体が該当するかはレベル次第）。Teleport チャンネルの物体（ゾーン）は Pawn に Overlap なので止めない。
- 着地: カプセル中心を床 + 125 cm に置く。立ち姿の中心は床 + 88 cm なので約 37 cm 落ちる（確定値からの計算）。`SetActorLocation` は CharacterMovement の Velocity を変えない（UE の仕様）。
- 推測: 確定から移動までの間に Q で `Reset Teleport` されるとアクタが破棄され、待ち中の Delay も消えるので移動はしない。カメラアニメはカメラマネージャ側なので最後まで続く（WebGL 版の記録と同じ解釈）。

### 1.7 コリジョンチャンネル（確定）

- 旧版の ini（`pak_reference` の元 pak の設定。`pak_reference/README.md` の `../dd_extracted` は `C:\Users\User\Desktop\decrypt_dd\dd_extracted\` にある）`DDeception/Config/DefaultEngine.ini` 238〜239 行:
  - `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,Name="Teleport",DefaultResponse=ECR_Overlap,bTraceType=False,bStaticObject=False)`
  - `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel2,Name="Malak",DefaultResponse=ECR_Block,bTraceType=False,bStaticObject=False)`
  - 独自プロファイルは無い（エンジン既定プロファイルの並べ直しだけ）。
- 最新版 `pak_reference_2/_raw/DDeception/Config/DefaultEngine.ini` 53 行: `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,Name="Teleport",DefaultResponse=ECR_Ignore,bTraceType=False,bStaticObject=False)`（Malak は無い）。→ **版で DefaultResponse が違う（旧 Overlap / 新 Ignore）**。テレポートは旧版に従うので Overlap が既定。
- `ObjectTypeQuery7` = Teleport の理由（UE5.8 `Engine/Source/Runtime/Engine/Private/Collision/CollisionProfile.cpp` 355〜381 行・440〜458 行で確認）: `ObjectTypeMapping` はエンジンの非トレースチャンネル（WorldStatic, WorldDynamic, Pawn, PhysicsBody, Vehicle, Destructible）の 6 個の後に、ini の独自オブジェクトチャンネルを並べる。Teleport が最初の独自オブジェクトチャンネルなので添字 6。**UE5 で C++ にするときは添字ではなく `FCollisionObjectQueryParams(ECC_GameTraceChannel1)` を使う**のが安全。
- オブジェクトクエリは応答（Response）を見ない（UE5.8 `PhysicsEngine/CollisionQueryFilterCallback.cpp` 32〜40 行「The regular object type filter logic just chooses None/Block」）。当たる条件は「ObjectType が Teleport で、コリジョンが Query 有効」だけ。

---

## 2. `BP_Power_Teleport` のコンポーネント構成（旧版、確定）

根拠: `pak_reference/_assets/DDeception/Content/Blueprints/Main/Powers/BP_Power_Teleport.json`（`*_GEN_VARIABLE` と `SCS_Node_*`）。親クラス `/Script/Engine.Actor`。CDO `PrimaryActorTick.bCanEverTick = true`。

```
DefaultSceneRoot (SceneComponent)                      … 値の上書きなし
├─ Arrow (ArrowComponent)                              … 上書きなし（エディタ表示用、ゲームでは非表示が既定）
├─ SpringArm (SpringArmComponent)                      … TargetArmLength 0.0 だけ上書き
│   └─ Decal (DecalComponent)                          … ソケット名なしで SpringArm に付く
│       └─ ParticleSystem (ParticleSystemComponent)
└─ Audio (AudioComponent)
```

| コンポーネント | 上書きされた値 | 既定のまま（UE の既定値。5.8 のソースで確認したもの） |
|---|---|---|
| SpringArm | `TargetArmLength` 0.0 | `bEnableCameraLag` false（BeginPlay の 0.5 s 後に true）、`CameraLagSpeed` 10、`CameraRotationLagSpeed` 10、`bEnableCameraRotationLag` false、`bUseCameraLagSubstepping` true、`CameraLagMaxTimeStep` 1/60、`CameraLagMaxDistance` 0、`bDoCollisionTest` true（ただし TargetArmLength 0 なのでトレースしない）、`ProbeSize` 12、`bUsePawnControlRotation` false、`bInheritPitch/Yaw/Roll` true、TickGroup `TG_PostPhysics` |
| Decal | `DecalMaterial` `/Game/Blueprints/Main/Powers/M_Decal_Teleport`、`DecalSize` [3.0, 100.0, 100.0]、`RelativeRotation` [-90.0, 0.0, 5.4641506721964106e-05]（Pitch, Yaw, Roll）、`RelativeScale3D` [3.326378583908081, 1.0, 1.0] | `SortOrder` 0、`FadeScreenSize` 0.01、Fade 系 0 |
| ParticleSystem | `Template` `/Game/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2`、`RelativeLocation` [-4.134940147399902, -0.00026260362938046455, 2.3245811462402344e-06]、`RelativeRotation` [90.0, 0.9113311767578125, -359.0887451171875]、`RelativeScale3D` [0.2, 0.2, 0.2] | `bAutoActivate` true |
| Audio | `Sound` `/Game/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227`、`VolumeMultiplier` 0.6499999761581421 | `bAutoActivate` true（スポーンで鳴り始め、破棄で止まる）、減衰設定なし |

### 2.1 ワールド変換の計算（確定値からの計算。UE の FTransform の合成＝回転は積、スケールは成分ごとの積）

アクタはワールド回転 (0,0,0) でスポーンされ、SpringArm は回転を継ぐだけなので、**デカールとパーティクルの向きはプレイヤーの向きに関係なく常に同じ**。

| 量 | 値 |
|---|---|
| デカールの軸 | X（投影方向）= (0,0,−1)（真下）、Y = (0,1,0)、Z = (1,0,0) |
| デカールの箱（半径。UE の `DecalSize` は半径で、`GetTransformIncludingDecalSize` がスケールに掛ける） | 上下 ±3 × 3.3264 = **±9.98 cm**、水平 ±100 × ±100 cm（**2 m 四方、world X・Y に沿う**） |
| パーティクルのワールド回転 | ほぼ恒等（Rotator ≈ (0, 5.5e-5, −7.6e-5)）。ローカル X = world X、Z = world Z |
| パーティクルのワールドスケール | **(0.66528, 0.2, 0.2)**（ローカル軸 = world 軸） |
| パーティクルの原点 | デカール原点（トレースの当たった点）の **+13.754 cm 上**（相対位置 −4.1349 × デカールの X スケール 3.3264 を、下向きの X 軸で回したもの） |

- 推測: RelativeRotation の Pitch 90 / Yaw 0.91 / Roll −359.09 はジンバルロック付近で Yaw と Roll が打ち消し合う表現で、実質「Pitch +90」。
- UE5 でも同じ親子構成で同じ値を置けば同じ合成になる（`FTransform` の成分ごとのスケール合成は UE5.8 でも同じ）。

### 2.2 位置の追従（UE5.8 `Engine/Source/Runtime/Engine/Private/GameFramework/SpringArmComponent.cpp` 93〜236 行、確定。4.21 も同じ構造と推測）

- `ArmOrigin = GetComponentLocation()`（= Tick で置いた当たり点）。ラグ有効時は `DesiredLoc = VInterpTo(PreviousDesiredLoc, ArmOrigin, dt, 10)`、`dt > 1/60` なら 1/60 s ずつ、目標を直線で動かしながら小刻みに補間。ラグ無効時も `PreviousDesiredLoc = ArmOrigin` を毎 Tick 更新する。
- `RelativeSocketLocation` を更新して子（デカール）を動かす。`GetSocketTransform` はソケット名を無視してアームの先の変換を返すので、ソケット名なしの子も遅れて追う。
- **テレポートフラグでラグはリセットされない**（UE5.8 に該当処理なし）。
- 結果（確定値からの導出）: 照準開始から 0.5 s までは当たり点に即座に付く。0.5 s より後に初めて床を捉えた場合、デカールは**スポーン位置（プレイヤーの 50 m 下）から**速度 10 で追ってくる（WebGL 版は「最初に捉えたときは即座に置く」としている）。

---

## 3. プレイヤー側（`BP_DD_PlayerCharacter`、旧版、確定）

根拠: `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`、`pak_reference/_bytecode/DDeception/Content/UI/BP_Powers.txt`、`pak_reference/_assets/DDeception/Content/UI/BP_Powers.json`。パワーの列挙 `Enum_RingAltar_Skills`: 0 Speed Boost、**1 Teleport**、2 Telepathy、3 Primal Fear、4 Telekinesis、5 Vanish、6 None。

### 3.1 入力（旧版 `DefaultInput.ini`）

`Use Power Left` = Q、`Use Power Right` = E、`Use Power` = R、`Cycle Power Left` = One、`Cycle Power Right` = Two、`Sprint` = LeftShift、`Interact` = F、`Toggle Tablet` = SpaceBar。プレイヤーのアクション束縛はすべて IE_Pressed で、`Use Power Left/Right`・`Cycle Power *`・`Toggle Tablet`・`Interact` は `bConsumeInput = true`、`Sprint` は false。左クリックはキー直結で IE_Pressed（`InpActEvt_LeftMouseButton_K2Node_InputKeyEvent_2`）と IE_Released（`_3`）、どちらも `bConsumeInput = false`。

### 3.2 Q の処理

```
InpActEvt_Use Power Left (@18336): Can Interact? が false なら終了 → Use Power(True)
Use Power (@24141):
  CanMove? が false なら終了
  Powers.Animation(左か)                      // 枠を弾ませる（使えるかの判定の前）
  Power To Use = Powers.Power[左 ? Left Power Index : Right Power Index]
  @9701 Available? が false → @14489（下の「使えない」）
  @9724 Push @14412（下の 0.5 s）
  @9729〜 DoOnce2（Temp_bool_*_Variable2）: 閉じていれば何もしない
  switch Power: 1 → @10017
@10017 CurrentSide = 左か
       Active Powers.AddUnique(1)
       UsedTeleportPower.Broadcast()
@10143 PlaySound2D(/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered, Volume 1.75, Pitch 1.0, 0.0)
       Powers.Power の {1, Available true} を {1, Available false} に置き換え → Powers.Update Powers()
@10508 PowerController.Set Delay(0.05, 1)      // 枠を 1→0 に 0.05 s
@10762 BeginDeferredActorSpawnFromClass(BP_Power_Teleport_C,
         Transform(Location = プレイヤー位置 − (0,0,5000), Rotation (0,0,0), Scale (1,1,1)),
         CollisionHandlingOverride ByteConst 1 = AlwaysSpawn, Owner None)
@11035 SetFloatPropertyByName('Max Distance', GameMode.Global Save Instance.Teleport Upgrades に応じて
         0→1000, 1→1000, 2→1125, 3→1250, 4→1375, 5→1500)
@11433 FinishSpawningActor（同じ変換で作り直したもの）
@11493 生成物.Used に UsedTeleport を AddDynamic
@11557 左なら Can Cycle Left? = False、右なら Can Cycle Right? = False
@11582〜 Sequence: DoOnce1→ゲートを閉じる（初回だけ）/ ゲートを開く / DoOnce5 をリセット
       → 最後に 'Sprinting Effects' のタイマーと Chameleon の Camera Shake を触る分岐へ（@18222〜。テレポート専用ではなく共用の後処理と推測）
@14412 Delay(0.5) → @2505: DoOnce2 をリセット
```

- **0.5 s の共有ロック（新発見）**: DoOnce2 は switch 全体（全パワー）を包むので、使用可能なパワーを発動した後 0.5 s は、どちらの枠の使用可能なパワーも発動しない（音も鳴らない）。「使えない」分岐（下）は DoOnce2 の外。
- 本作ではプレイヤーの強化が無いので `Max Distance` は 1000（強化 0・1）。

### 3.3 使えないときの Q（@14489）

```
Powers.Power の要素数 < 1 なら終了
if (Active Powers に 1 がある AND 押した側 == CurrentSide):
    @14789 PowerController.Reset Teleport()   // 取り消し
else:
    @14826 PlaySound2D(/Game/Audio/UI/power_not_ready, Volume 0.35, Pitch 1.0, 0.0)
```

### 3.4 `Used` の後（`UsedTeleport` @30854）

```
左なら Can Cycle Left? = True、右なら Can Cycle Right? = True
Active Powers.RemoveItem(1)
PowerController.Set Delay(レベル名 == "00_Ballroom" ? 1.0 : 5.0, 1)    // 枠を 0→1
@21836 → (ゲートが開いていれば) @21711 Delay(同じ 1.0 / 5.0) → @4045
@4045 DoOnce5（StartClosed = true。使用時 @18106 でリセット）
      → @4116 PlaySound2D(/Game/Audio/UI/power_refilled, Volume 0.5, Pitch 1.0, 0.0)
      → Powers.Power の {1, false} を {1, true} に戻す → Powers.Update Powers()
```

### 3.5 取り消し（`BP_Powers.Reset Teleport` @2488 → @2188）

```
BP_Powers: Player.Reset Teleport() → Stop Teleport Timeline()（@2322: Timeline_0.Stop → Teleport.Percent = 1.0）
Player.Reset Teleport (@31827) の実行順:
  1) @22224: ゲートを閉じる
  2) @4045: DoOnce5 → power_refilled（0.5）・使用可能に戻す
  3) @31252: 生成した BP_Power_Teleport を K2_DestroyActor
  4) @30854: UsedTeleport と同じ（Can Cycle を戻す、Active Powers から除く、Set Delay(5.0)）。ゲートが閉じているので Delay→再度の refill は起きない
```
→ 結果: すぐ使える・枠は 1.0・`power_refilled` が 1 回鳴る。呼び出し元はほかに `UMG_DeathScreen`（`Reset Powers` → `Reset All Powers`）と `BP_02_GymDoors`（旧版・新版とも）。

### 3.6 タブレットの枠（`BP_Powers`、確定）

- `Set Delay(Duration, Power)` → Power 1 なら `Set Delay Teleport(Duration)`（@1746）: `Timeline_0.SetPlayRate(1.0 / Duration)` → `Powers.Teleport`（MID）の `Percent` を読み、**1.0 未満なら `PlayFromStart`、そうでなければ `ReverseFromEnd`**。
- `Timeline_0`（`Timeline_0_Template`）: 長さ 1.0、トラック `Floaty`、曲線 `CurveFloat_0_1` = (0, 0) → (1, 1)、両キー `RCIM_Linear`。更新（@498）で `Powers.Teleport.SetScalarParameterValue('Percent', Floaty)`。完了（@497）は何もしない。
- したがって: Q で 1→0 を 0.05 s、移動で 0→1 を 5.0 s（線形）、取り消しで即 1.0。
- 照準中のほかの入力: 移動（Forward/Back/Left/Right の軸）、ダッシュ、タブレットの出し入れ（`CanMove?`・`Can Use Tablet?` だけを見る）はテレポートで制限されない（`Active Powers` を参照するのは上の箇所と枠の切り替えだけ）。枠の切り替えは `isTabletUp? AND Can Cycle Left?/Right?` で、使用中の側だけ止まる。
- 左クリック（IE_Pressed、@7716）: `Can Interact?` なら @1735: 手持ち（`heldItem`）の `Use`、なければカメラ前方 200 cm の `LineTraceSingle` で当たったアクタの `InteractWithObject`。テレポートの確定と**同じクリックで同時に走る**（両方とも入力を消費しない）。

---

## 4. `BP_Power_Teleport_Zone` と配置

### 4.1 クラス（確定）

| | 旧版 | 最新版 |
|---|---|---|
| バイトコード | `UserConstructionScript` だけ（空） | `UserConstructionScript`: 0〜50 の 51 スロットに `SetMaterial(i, M_TeleportZone)` |
| `Cube_GEN_VARIABLE`（両版同じ） | `StaticMesh` `/Engine/BasicShapes/Cube`、`OverrideMaterials` [M_TeleportZone]、`CastShadow` false、`BodyInstance` {`ObjectType` ECC_GameTraceChannel1, `CollisionEnabled` QueryOnly, `CollisionProfileName` Custom, 応答: WorldStatic/WorldDynamic/Pawn/Visibility/Camera/PhysicsBody/Vehicle/Destructible すべて ECR_Overlap（Teleport は既定値）}、`RelativeScale3D` [1.0, 1.0, 0.05]、**`bHiddenInGame` true** | 同じ |
| `M_TeleportZone` | `BLEND_Translucent`、Emissive = Multiply | `BLEND_Masked`、`MSM_Unlit`、`DitherTemporalAA` を使う |

「移動できる床」はこのアクタのメッシュ（Teleport オブジェクトの QueryOnly・非表示）で定義される。ゲームでは見えない。

### 4.2 病院 Zone 1（`pak_reference_2/_levels/06_Hospital_Zone_01.full.json`、確定）

| アクタ | 親 | メッシュ | ワールド | 備考 |
|---|---|---|---|---|
| `BP_Power_Teleport_Zone_2` | なし（ルート (0,0,0)） | `/Game/Meshes/06_Hospital/hospital_zone_01_teleport`（51 スロットすべて M_TeleportZone） | 位置 (0,0,0)、回転 0 | Cube に `bVisible` false。**`RelativeScale3D` の上書きが無いのでアーキタイプの (1,1,0.05) を継ぐ**（推測: 書き出しの world scale 1 はアーキタイプを合成していない。メッシュは Z −3.83〜+0.16 cm の平面なので、0.05 倍でも床面は ±0.2 cm しか動かない） |
| `BP_Power_Teleport_Zone_Ambulance` | `hospital_ambulance_new_teleport.StaticMeshComponent0` | Cube（`/Engine/BasicShapes/Cube`、既定） | ルート位置 (11245.0, −20080.0, 335.0)、回転 0、ルートのワールドスケール (3.52645, 4.87530, 1.0)（相対 [−1.5174, −66.666, 257.692]、[2.71266, 3.75023, 0.76923]） | 推測: Cube はアーキタイプの (1,1,0.05) を継ぐので箱は 352.6 × 487.5 × 5 cm、上面 z ≈ 337.5（救急車の屋根） |
| `hospital_ambulance_new_teleport` | — | `hospital_ambulance_new`、スケール 1.3 | (11246.97, −19993.33, 0.0) | `NoCollision`。レベル BP が目標「GET ON TOP OF THE AMBULANCE」を出し、屋根の `TriggerBox_06_AmbulanceTop` で `06_ReachAmbulance`。離陸時に `BlockingVolume_Ambulance_1〜4` を `SetCollisionEnabled(3)` |
| `BlockingVolume_Ambulance_1〜4` | 救急車 | ブラシ | 例 _1: (11400, −20075, 430) | 最初は `NoCollision`、応答は Pawn 以外 Ignore |
| `BlockingVolume_Ambulance_5・6` | 救急車 | ブラシ | _5: (11255, −20075, 145) | Query 有効、Pawn 以外 Ignore（テレポート中のカプセルは Pawn を無視するので止めない） |

### 4.3 病院 Zone 2（`06_Hospital_Zone_02.full.json`、確定）

| アクタ | 親 | メッシュ | ワールド | 備考 |
|---|---|---|---|---|
| `BP_Power_Teleport_Zone_2` | なし | `/Game/Meshes/06_Hospital/hospital_zone_02_teleport`（51 スロット M_TeleportZone） | ルート `RelativeLocation` (0,0,1.0) | Cube に **`RelativeScale3D` [1,1,1] の明示の上書き**（このメッシュは Z 0〜800 の 2 層なので必須）、`bVisible` false |
| `BP_Power_Teleport_Zone2_2` | `hospital_ambulance_new_teleport.StaticMeshComponent0` | Cube | ルート (−10351.0, −6544.0, 327.0)、相対回転 Yaw −179.99976、ルートのワールドスケール (3.13591, 4.04961, 1.0)（相対 [−3.305, −65.552, 251.108]、[2.41224, 3.11508, 0.76923]） | 推測: 箱 313.6 × 405.0 × 5 cm |
| `hospital_ambulance_new_teleport` | — | `hospital_ambulance_new`、スケール 1.3、Yaw 179.99976 | (−10355.30, −6629.22, 0.56) | `BlockAllDynamic`（WorldDynamic なのでテレポートのスイープは通り抜ける）、Movable |
| `ambulance_teleport_blocking`〜`4` | `TargetPoint_ambulance_teleport_blockingvolume_parent`（救急車の子） | ブラシ（200 cm 立方体の BodySetup、`CTF_UseSimpleAsComplex`） | 例: (−10510, −6539, 129) スケール (0.124, 2.183, 1.595) | `BlockAll`（WorldStatic なのでスイープを止める） |

### 4.4 ゾーンのメッシュ（`pak_reference_2/_meshes.json`・`_assets`、確定）

| メッシュ | バウンズ（原点 / 半径） | BodySetup |
|---|---|---|
| `hospital_zone_01_teleport` | (2137.5, −10587.5, −1.86) / (10012.5, 14012.5, 1.86)、7 スロット、LightMapResolution 64 | 単純コリジョンは全体を覆う凸包 1 個（`ConvexElems`）。`CollisionTraceFlag` は既定 |
| `hospital_zone_02_teleport` | (1395.19, 68.70, 400.0) / (16495.2, 6281.3, 400.0)、10 スロット | 同上（凸包は Z 0〜800 に広がる） |

- トレースは `bTraceComplex = true` なので、単純コリジョン（大きな凸包）ではなく**三角形（複雑コリジョン）**に当たる。UE5 で取り込むときも複雑コリジョンを残すこと（推測: 単純コリジョンでトレースすると凸包の上面に当たり、Zone 2 では 2 階の高さに輪が出る）。
- `_meshes.json` の `body_setup: false` は書き出しの判定で、`_assets` には `BodySetup_0` がある。

### 4.5 旧版のレベルの作り方（比較、確定）

- `03_Manor_Zone1`: `BP_Power_Teleport_Zone*` 5 個、すべて既定の Cube を**ルートのスケールで引き伸ばした薄い箱**（例 `BP_Power_Teleport_Zone_2`: ルート (59.99, 21654.91, 0.011)、スケール (114.54, 93.13, 1.0) → 11454 × 9313 × 5 cm）。
- `01_Hotel`: 専用メッシュ（`/Game/Meshes/TeleportMeshes/hotel_lobby_teleport_mesh`、`hotel_maze_teleport_mesh`）。`BP_Power_Teleport_Zone2.Cube` にはインスタンスで `CollisionResponses: [{Channel: "Teleport", Response: ECR_Block}]` がある（オブジェクトクエリには無関係）。
- `manor_teleport_mesh` の BodySetup は `DefaultInstance` {ECC_WorldStatic, BlockAll}（コンポーネント側で上書きされる）。

---

## 5. 演出のアセット

### 5.1 `CameraAnim_Teleport`（`pak_reference/_camera/CameraAnim_Teleport.json`・`_assets/DDeception/Content/Animation/Camera/CameraAnim_Teleport.json`、確定）

- `AnimLength` 0.5、`BaseFOV` 137.24078369140625、`BasePostProcessBlendWeight` 1.0、`GroupName` TeleportAnim、`bRelativeToInitialTransform`・`bRelativeToInitialFOV` は書き出しに無い（= クラス既定）。
- `BasePostProcessSettings`: `bOverride_WhiteTemp`・`bOverride_WhiteTint`・`bOverride_FilmWhitePoint`・`bOverride_SceneColorTint`・`bOverride_AutoExposureBias` が true、`AutoExposureBias` 1.1222918033599854（ほかは既定値 = WhiteTemp 6500、WhiteTint 0、FilmWhitePoint (1,1,1)、SceneColorTint (1,1,1)。いずれも中立。AutoExposureBias は t=0 からトラックが上書きする）。
- すべてのキーは `CIM_CurveAutoClamped` で、**保存された接線をそのまま使って評価する**（`FInterpCurve::Eval`: `CubicInterp(P0, Leave0×Δ, P1, Arrive1×Δ, α)`。UE5.8 `Core/Public/Math/InterpCurve.h` 361〜375 行で同じ式を確認）。

| トラック | 時刻 → 値（Arrive = Leave の接線、値/秒） |
|---|---|
| `InterpTrackMove_0` | 位置 (0,0,0)、回転 (0,0,0) の 1 キーだけ（カメラは動かない） |
| `CameraComponent.FieldOfView` | 0.0 → 90（0）/ 0.12999999523162842 → 150（0）/ 0.17999999225139618 → 80（0）/ 0.25 → 100（0）/ 0.4000000059604645 → 90（0） |
| `CameraComponent.PostProcessSettings.AutoExposureBias` | 0.0 → 0（0）/ 0.07000000029802322 → 1（15.384615898132324）/ 0.12999999523162842 → 2（62.0667724609375）/ 0.135416641831398 → 100（0）/ 0.13982369005680084 → 2（−107.3153076171875）/ 0.20999999344348907 → 0（0） |
| `CameraComponent.PostProcessSettings.SceneColorTint`（RGBA） | 0.0 → (1,1,1,1)（0,0,0,0）/ 0.06971153616905212 → (1,1,1,1)（7.799999713897705, −6.90725564956665, −7.799999713897705, 0）/ 0.12820513546466827 → (2.0, 0.11445438861846924, 0.0, 1.0)（7.075630187988281, −6.265793323516846, −7.075630187988281, 0）/ 0.21104170382022858 → (2.0, 0.11445438861846924, 0.0, 1.0)（−3.978832244873047, 3.5234375, 3.978832244873047, 0）/ 0.37953516840934753 → (1,1,1,1)（0） |

60fps の値（`CameraAnim_Teleport.csv` から抜粋、確定）:

| t | FOV | EV | Tint R, G, B |
|---:|---:|---:|---|
| 0.01667 | 92.71 | 0.097 | 0.976, 1.021, 1.024 |
| 0.05 | 109.80 | 0.645 | 0.921, 1.070, 1.079 |
| 0.1 | 141.89 | 1.150 | 1.528, 0.532, 0.472 |
| 0.11667 | 148.24 | 1.409 | 1.860, 0.238, 0.140 |
| 0.13333 | 149.11 | **67.69** | 2.033, 0.085, −0.033 |
| 0.15 | 125.36 | 1.088 | 2.101, 0.025, −0.101 |
| 0.18333 | 80.13 | −0.027 | 2.092, 0.033, −0.092 |
| 0.25 | 100.0 | 0 | 1.773, 0.316, 0.227 |
| 0.38333 | 90.34 | 0 | 1.0, 1.0, 1.0 |

注: Tint の B は負になる区間がある（保存された接線による行き過ぎ）。

**UE4 での適用のされ方（要注意）**
- 位置・回転・FOV はプレイヤーのカメラへの加算。PostProcess は `AnimCamera.PostProcessSettings` を重み `PostProcessBlendWeight × ブレンド重み`（= 1）で**上書き**合成（`bOverride_*` の項目だけ。レベルのボリュームと同じ補間の「上書き」）。4.21 では UE4 の `LocalPlayer::CalcSceneView` がカメラアニメの PP を**カメラコンポーネントの PP の後に**適用する（推測: 4.21 のソースは手元に無い。UE5.8 の `LocalPlayer.cpp` 948〜981 行は「Base（カメラの下）/ Override（カメラの上）」の 2 段で、`r.CameraAnimation.LegacyPostProcessBlending`（既定 true）はカメラアニメの PP を Base 側に置く）。
- 病院の PP ボリューム（Zone 2 の `PostProcessVolume_1`、`06_Hospital` の `PostProcessVolume_1`）は AutoExposureBias・SceneColorTint・WhiteTemp・FilmWhitePoint を上書きしていない（確定）。本作の `Config/DefaultEngine.ini` も自動露出オフ・Bias 0。したがって上書きでも加算でも結果は同じ（EV は `0 + トラック値`）。
- **FOV の基準（未確定）**: `bRelativeToInitialFOV` が既定（true）なら `POV.FOV += (AnimCam.FieldOfView − InitialFOV) × Scale` で 5〜170 に収める（Web 検索の要約で `CameraAnimInst.cpp` の式として出てくる。行は未確認）。`InitialFOV` の出どころは確認できなかったが、UE4.27 の公式ドキュメント「CameraAnims」は「キーは relative to initial で、アニメの time 0.0 からの差分だけが適用される」と説明している（検索結果の要約。ページ本体は 403 で取得できず）。→ **採用案: FOV = プレイヤーの FOV + (key(t) − 90)**。WebGL 版の収録の実測（広がる → 白 → 狭まる → 戻る）とも合う。もし `InitialFOV = BaseFOV (137.24)` なら t=0 で −47.2°（歩きの 90° が 42.8° に）と大きく縮むはずで、これは手元の旧版（`Launch-Classic-Ch3.cmd`）の 60fps 収録で一目で判別できる。
- **旧版の実機で観察（2026-09-16）**: Deadly Decadence の入口の噴水でテレポートを 2 回、60 fps で収録した（`observations/classic/tp-01.mkv`・`tp-02.mkv`）。クリックの直後に画面が広がり（輪と池の縁が小さくなる）、閃光（1 フレームだけ全面 (234, 245, 244)）の後の t≈0.24〜0.40 のフレームは、アニメの後のフレームに対して拡大率 1.26 → 1.30 → 1.14 → 1.06（画角 ≈ 103° → 93°）、アニメの後は 0.98 のまま跳ばない。`BaseFOV` 基準なら t≈0.28〜0.40 は 43〜51°（拡大率 0.39〜0.48）で、終わりで 90° へ跳ぶはず。→ **基準は t=0 のキー（90）で確定**（このアセットでは key(0) = 90 = カメラの既定 FOV なので、どちらと読んでも同じ値）。
- 参考: 最新版の `wtfUE4` は `BaseFOV`・`BasePostProcessBlendWeight` が書き出しに無い（クラス既定）。推測: 既定の BasePostProcessBlendWeight が 0 だと SceneColorTint トラックが効かないので、最新版は赤を別の PostProcess コンポーネントで出すようにした可能性がある（名前の "wtf" も含め推測）。

### 5.2 `BP_CameraShake_Streak`（`pak_reference/_assets/DDeception/Content/UI/Menu/Streaks/BP_CameraShake_Streak.json`、確定。両版で同じ）

- 親 `/Script/Engine.CameraShake`（UE5 では `ULegacyCameraShake`）。
- `OscillationDuration` 0.5、`OscillationBlendInTime` 0.0、`OscillationBlendOutTime` 0.25。
- `RotOscillation`: Pitch {Amplitude 0.25, Frequency 30.0}、Yaw {0.25, 40.0}、Roll {0.5, 35.0}。`FOVOscillation` {2.0, 10.0}。`LocOscillation` なし（0）。`InitialOffset` は既定 `EOO_OffsetRandom`（0〜2π の乱数）。AnimShake なし。
- 振動子（UE5.8 `EngineCameras/Private/Shakes/LegacyCameraShake.cpp` 29〜66 行、UE4 と同じ）: `offset += dt × Frequency; 値 = Amplitude × sin(offset)`（Frequency は rad/s。`UWaveOscillatorCameraShakePattern` は Hz で `× 2π` するので、使うなら 30/2π = 4.7746 Hz などに換算が要る）。
- 重み（同 198〜301 行）: 残り時間 `Remaining` を先に dt 減らし、`Remaining < 0.25` なら重み `= Remaining / 0.25`、0 で終了。ブレンドインなし。Pitch は ±89.9° を超えないよう切り詰める。
- 本作のパイプラインに `wasami_tools/toolsets/dd.py` の `import_dd_camera_shakes`（`LegacyCameraShake` の BP を原作の値で作る）がある。`'UI/Menu/Streaks/BP_CameraShake_Streak'` を渡せば同じ方法で作れる（推測: 実行は未確認）。

### 5.3 `M_Decal_Teleport`（確定。両版で同じ）

- `MaterialDomain` MD_DeferredDecal、`BlendMode` BLEND_Translucent、`DecalBlendMode` DBM_Emissive。`EmissiveColor` ← `MaterialExpressionMultiply_2`（cook で削除）。
- 使う関数: `/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential`、`ImageAdjustment/CheapContrast`、`Gradient/LinearGradient`、`Density/ExponentialDensity`（RadialGradientExponential の中）。テクスチャは無し。スカラー・ベクターのパラメータも無い（インラインのシェーダマップの uniform expression は既定の `SelectionColor`・`RefractionDepthBias` だけ）。
- **色・半径・密度・コントラストの定数はコンパイル済みシェーダにしか無く、回収できていない**（旧版 `M_Decal_Teleport.uexp` 158 KB には DXBC が無く、共有シェーダコードのアーカイブも展開物に無い）。形と色は観察で決める必要がある（WebGL 版も推定）。

### 5.4 `P_ky_cutter2`（`pak_reference/_particles.json` の `/Game/ThirdParty/AdvancedMagicFX13/Particles/P_ky_cutter2`、確定）

システム: エミッタ 2、`lod_distances` [0, 2500, 5000]、`bUseFixedRelativeBoundingBox` true、固定バウンズ (−150,−150,−50)〜(150,150,50)。どちらのエミッタも `ParticleSpriteEmitter`、`bUseLocalSpace` true、`ScreenAlignment` は既定 `PSA_Square`（書き出しに無い＝既定。UE5.8 `ParticleModules.cpp` 1057 行）。

**エミッタ 1「cutter」**（LOD0 の値。LOD1 は Rate 25・SpawnPerUnit の UnitScalar 100、LOD2 は Rate 0。照準は 15 m 以内なので常に LOD0）

| モジュール | 値 |
|---|---|
| Required | Material `/Game/ThirdParty/AdvancedMagicFX13/Materials/M_ky_slash01_4x4`、`InterpolationMethod` PSUVIM_Linear_Blend、SubImages 4 × 4、`RandomImageTime` 1.0 |
| Spawn | Rate 10、RateScale 1、BurstScale 1 |
| SpawnPerUnit | SpawnPerUnit 1、UnitScalar 既定 50（UE5.8 `ParticleModules_Spawn.cpp` 262 行）、MovementTolerance 既定 0.1 → ワールドで 50 cm 動くごとに 1 |
| Lifetime | 1.0 |
| Size | StartSize (300, 300, 300) |
| SizeMultiplyLife | 寿命 k/7 で 1.0, 1.0554098, 1.1982670, 1.3935925, 1.6064075, 1.8017330, 1.9445902, 2.0 |
| ColorOverLife | `DistributionVectorParticleParameter`（ParameterName `color`、ParamModes DPM_Direct ×3、Constant **(13.0, 0.0, 0.216663)**。コンポーネントがパラメータを渡さないので Constant）、AlphaOverLife 1 → 0（線形 2 点） |
| SubUV | SubImageIndex（寿命 k/15）: 0, 2.8058, 5.2389, 7.3239, 9.0892, 10.5603, 11.7648, 12.7288, 13.4794, 14.0432, 14.4468, 14.7173, 14.8810, 14.9650, 14.9957, 15.0 |
| Rotation | StartRotation 乱数 0〜1 回転（× 360°） |
| RotationRate | 乱数 0.5〜1.0 回転/s（× 360°/s） |
| OrientationAxisLock | `EPAL_Z`（ローカル Z を向いて床に寝る） |
| Velocity | 0 |
| Location | 乱数 (0,0,0)〜(0,0,5) |

**エミッタ 2（名前なし、赤い火花）**（LOD0・LOD1 同じ。LOD2 は Rate 2）

| モジュール | 値 |
|---|---|
| Required | Material `/Game/PyroParticlePack/Materials/PPP_Radial_Gradient_Doffed` |
| Spawn | Rate 30 |
| Lifetime | 2.0 |
| Size | 乱数 (5,5,5)〜(15,15,25)（PSA_Square なので X だけが効く） |
| Velocity | 乱数 (−5,−5,300)〜(5,5,500)、Radial 0 |
| ColorOverLife | (5,0,0) → (1,0,0)、Alpha 1 → 0 |
| Location | 乱数 (−170,−170,0)〜(170,170,0) |
| AttractorPoint | `bEnabled` false（Position 0、Range 1000、Strength 500） |
| Orbit | `bEnabled` false |
| SizeScaleBySpeed | 既定値（SpeedScale 0、MaxScale (1,1)）→ 倍率は常に 1 で効果なし（UE5.8 `ParticleModules_Size.cpp` 404〜424 行） |

**コンポーネントのスケール (0.66528, 0.2, 0.2) を受けた実際の見え方（確定値からの計算）**
- スプライトの大きさ: `GetParticleSize` が `Size.X × Scale.X`、PSA_Square なら `Size.Y = Size.X`（UE5.8 `ParticleSystemRender.cpp` 373〜384 行）。→ 斬撃は **一辺 199.6 cm の正方形**（寿命の終わりに 399.2 cm）、火花は 3.3〜10.0 cm の正方形。
- ローカル空間の位置と速度は描画時にコンポーネントの変換（スケール込み）を受ける: 火花の出現範囲 world X ±113.1 cm × world Y ±34 cm、上向き 60〜100 cm/s（寿命 2 s で 1.2〜2.0 m 上昇）。斬撃の出現高さ 0〜1 cm。いずれもパーティクル原点（床 + 13.75 cm）が基準。
- 推測: 斬撃のテクスチャの三日月はセルの縁（WebGL 版の測定で半径 0.42 セル）にあるので、見える輪の外径は 199.6 × 0.84 ≈ 1.68 m から始まる（WebGL 版の映像の実測「外径約 1.6 m」と合う）。

**`M_ky_slash01_4x4`**（`_assets/DDeception/Content/ThirdParty/AdvancedMagicFX13/Materials/M_ky_slash01_4x4.json`、確定）: BLEND_Translucent、MSM_Unlit、TwoSided、`bUsedWithParticleSprites`・`bUsedWithMeshParticles`。Emissive ← `MaterialExpressionLinearInterpolate_0`（グラフは cook で削除）。残っている式: `ParticleSubUV`（Texture `/Game/ThirdParty/AdvancedMagicFX13/Textures/T_ky_slash01_4x4`、SamplerType LinearColor）、ベクター `hilightColor` (3.9051919, 4.0955548, 5.0, 1.0)、スカラー `alphaDensity` 1.5、`colorCorrect` 2.0、`depthFade` 100.0。→ グラフのつなぎ方は推測で組むしかない。
- テクスチャ: `C:\Users\User\Desktop\wasami_deception\pak_reference\DDeception\Content\ThirdParty\AdvancedMagicFX13\Textures\T_ky_slash01_4x4.png`（2048 × 2048 RGB、元は PF_DXT1、sRGB false、12 ミップ、`TEXTUREGROUP_Effects`）。

**`PPP_Radial_Gradient_Doffed`**: BLEND_Translucent、MSM_Unlit、Emissive ← `ParticleColor` の RGB、`bEnableResponsiveAA` true、`bEnableSeparateTranslucency` false。関数 `Gradient/RadialGradient`・`Opacity/CameraDepthFade`（定数は削除）。テクスチャなし。

### 5.5 音（確定。すべて SoundWave で、SoundCue ではない。`pak_reference/_audio.json`・`_assets`）

| 使う所 | アセット | OGG（`pak_reference/` からの相対） | 長さ | ch / Hz | SoundClass | ループ | 鳴らし方 |
|---|---|---|---:|---|---|---|---|
| 照準開始（プレイヤー @10143） | `/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered` | `Engine/Content/VREditor/Sounds/UI/Teleport_Mode_Entered.ogg` | 1.956 s | 2 / 48000 | なし（既定） | なし | PlaySound2D 音量 1.75、ピッチ 1.0 |
| 照準ループ（アクタの Audio） | `/Game/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227` | `DDeception/Content/Audio/03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227.ogg` | 3.733 s | 2 / 48000 | `DD_SoundClass_SFX` | **`bLooping` true** | AudioComponent、音量 0.65、自動再生、減衰なし（2D）。スポーンで開始、アクタの破棄で停止 |
| 確定（アクタ @138） | `/Engine/VREditor/Sounds/UI/Teleport_Committed` | `Engine/Content/VREditor/Sounds/UI/Teleport_Committed.ogg` | 1.543 s | 2 / 48000 | なし | なし | PlaySound2D 音量 1.0、ピッチ 1.0（クリックの 0.12 s 後） |
| 再使用可能（プレイヤー @4116） | `/Game/Audio/UI/power_refilled` | `DDeception/Content/Audio/UI/power_refilled.ogg` | 1.515 s | 2 / 48000 | `DD_SoundClass_SFX` | なし | PlaySound2D 音量 0.5 |
| 使えない（プレイヤー @14826） | `/Game/Audio/UI/power_not_ready` | `DDeception/Content/Audio/UI/power_not_ready.ogg` | 2.005 s | 2 / 48000 | `DD_SoundClass_SFX` | なし | PlaySound2D 音量 0.35 |

- `DD_SoundClass_SFX`: `bApplyAmbientVolumes` true、子 `DD_SoundClass_SFX_Movies`（Volume 0.5）・`DD_SoundClass_SFX_UI`（`bIsUISound`・`bReverb` false）。`DD_SoundMix` の調整はすべて 1.0。
- 減衰の無い AudioComponent は空間化されない（UE5.8 `ActiveSound.cpp` 1182 行: 減衰設定が無ければ `UpdateAttenuation` を呼ばない）。スポーン位置が 50 m 下でも普通に聞こえる。
- UE5.8 のエンジンにも `Engine/Content/VREditor/Sounds/UI/Teleport_Committed.uasset` はあるが、4.21 のものと同一かは未確認。原作の OGG を取り込むのが確実。

---

## 6. 最新版（`pak_reference_2`）の `BP_Power_Teleport` との違い（記録用。採らない）

根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_Power_Teleport.txt`（2572 バイト）、`pak_reference_2/_assets/.../BP_Power_Teleport.json`。

| 項目 | 旧版 | 最新版 |
|---|---|---|
| カメラアニメ | `CameraAnim_Teleport`、`Duration` 0.0 | `/Game/Animation/Camera/wtfUE4`、**`Duration` 1.0**（@788）。wtfUE4 は長さ 0.5063 s、FOV 90 →（0.1501）150 →（0.2182）80 →（0.4919）90、AutoExposureBias トラックなし、SceneColorTint 1 →（0.1469）(2,0,0) →（0.16）(10,0,0) →（0.4663）(10,10,10)、Move トラックの 1 キーが原点でない（位置 (2368.6, −496.6, 325.8)、回転 (−6.7e-9, −0.708, 169.71)）、BaseFOV・BasePP は書き出しに無い |
| 赤い閃光 | なし | `PostProcess`（PostProcessComponent: `Settings.bOverride_SceneColorTint` true・`SceneColorTint` (10,0,0,1)、`Priority` 2.0、`BlendWeight` 0.0、`bUnbound` 既定）と Timeline `Red Effect`（長さ 0.5、トラック `Alpha`、`CurveFloat_0`: 0 s 0 [Linear] → 0.05 s 0 [Cubic, Auto] → 0.2 s 1.0 [Cubic, User, Arrive −3.1537771 / Leave −3.1537755] → 0.5 s 0 [Linear]）。更新（@2477）で **`BlendWeight = Alpha × 100`** |
| 赤い閃光の開始 | — | **クリックと同じフレーム**（@1061 Push @1121 → Delay(0.12) を登録 → Pop → @1121 `Red Effect.PlayFromStart`）。`pak_reference_2/README.md` §3.1 と WebGL 版の記録の「0.12 s 後」は誤り |
| 確定の条件 | なし | Decal の位置が (0,0,0) と許容 1e-4 で違うときだけ（@532）。推測: スポーン位置は 50 m 下なので実質いつも通る |
| 確定の入力 | LeftMouseButton | LeftMouseButton と **Gamepad_FaceButton_Bottom**（どちらも IE_Pressed、入力を消費しない） |
| Alpha の初期値 | 0.6 | **1.0**（初期距離 = Max Distance） |
| ボス戦 | なし | Tick の冒頭で `GetCurrentLevelName() == "06_Hospital_Bossfight"` なら毎回 `Max Distance = 1200.0`（@1605〜@1697） |
| ParticleSystem | 既定 | `PrimaryComponentTick.bStartWithTickEnabled` false（推測: 活性化で Tick が有効になるかは未確認） |
| Delay(0.12) の再開先 | @15 | @16（@15 は Red Effect の完了用の空の入口） |
| 変わらないもの | — | コンポーネントの値（Decal・Particle の変換、Audio 0.65、SpringArm）、トレース（500 cm、ObjectTypeQuery7、bTraceComplex）、距離の式、0.12 s、シェイク、確定音、SetActorLocation、カプセルの応答、`Used`、破棄、`Delay(0.5)` のラグ有効化 |
| プレイヤー側 | `Teleport Upgrades`（セーブ）で Max Distance | `Get Power Upgrade Level(1)` で同じ表（1000/1000/1125/1250/1375/1500）。照準開始の音 1.75・`Set Delay(0.05)`・50 m 下へのスポーン・クールダウン 5.0（00_Ballroom 1.0）は同じ |
| ゾーン | 空の UCS | UCS で 51 スロットに M_TeleportZone。`M_TeleportZone` は Masked・Unlit に変更 |
| チャンネル | Teleport 既定 Overlap、Malak（GameTraceChannel2、Block）あり | Teleport 既定 Ignore、Malak なし |
| CameraAnim_Teleport | あり | 消えた。同じ中身の `CameraAnim_Teleport1`・`2`（BaseFOV 137.24、同じ BasePP）がカットシーン用に残る |

---

## 7. UE5.8 で再現するときの注意（確かめた範囲）

1. **カメラアニメ**: `UCameraAnim`・`UCameraAnimInst`・`APlayerCameraManager::PlayCameraAnim` は 5.8 に無い（`Engine/Source`・`Plugins` を検索して該当なし）。代わりは `UCameraAnimationSequence`（TemplateSequence プラグイン）＋`UCameraAnimationCameraModifier`（EngineCameras プラグイン）だが、
   - FOV は「現在の POV の FOV を初期値にしたスタンドインにキーの絶対値を入れ、差分 × Scale を足す」実装（`CameraAnimationCameraModifier.cpp` 424〜458 行）なので、キーが 90 基準の絶対値だと結果が現在の FOV に依存して原作と変わる。
   - PP は `r.CameraAnimation.LegacyPostProcessBlending`（既定 true）で Base 側（カメラの PP の下）に入る。
   → **推奨（推測）**: C++ の `UCameraModifier` を 1 つ作り、`FInterpCurveFloat`／`FInterpCurveLinearColor` に原作のキー（時刻・値・Arrive/Leave 接線・`CIM_CurveAutoClamped`）をそのまま入れて `Eval` し、FOV に `(key − 90)` を足して 5〜170 に収め、`CameraOwner->AddCachedPPBlend(PP, 1.0, VTBlendOrder_Override)` で `bOverride_AutoExposureBias`・`bOverride_SceneColorTint`（と中立値の WhiteTemp/WhiteTint/FilmWhitePoint）を渡す。時間は `dt × Rate(1.0)` で進め、0.5 s で終わる（ブレンドなし）。
2. **カメラシェイク**: `ULegacyCameraShake`（`Plugins/Cameras/EngineCameras`、`EnabledByDefault` true、依存に TemplateSequence）。C++ から型を使うなら Build.cs に `EngineCameras` を足す。再生は `APlayerCameraManager::StartCameraShake(Class, 1.0, ECameraShakePlaySpace::CameraLocal)`。`MatineeCameraShake.h` は 5.5 で非推奨の別名。
3. **露出 +100 EV（2026-09-16 に PIE で確認、問題あり → 対処済み）**: UE5.8 のプレエクスポージャは前フレームの目の順応値の読み戻し（`GetLastEyeAdaptationExposure`）を使う（`PostProcessEyeAdaptation.cpp` 1504〜1580 行）。自動露出オフでも方式は Histogram のままで固定露出扱いにならないため、2^100 の露出が数フレーム遅れてプレエクスポージャに入り、シーンカラーの float が溢れる／TSR・TAA の履歴に inf が残る恐れがある。露出補正そのものは順応の平滑化の**後**に掛かる（`PostProcessEyeAdaptation.usf` 179〜193 行）ので、1 フレームの白は再現できる。PIE で確認し、問題があれば `r.EyeAdaptation.PreExposureOverride` などの対処をユーザーと相談。
   → **結果**: PIE の 60 fps の収録で、白の 1〜2 フレーム後に**真っ黒なフレームが 1 枚**出た（`FViewInfo::UpdatePreExposure` が Histogram の方式では読み戻しの露出をプリ露出に使うため、2^100 でシーンカラーが溢れる）。原作は `r.UsePreExposure=False` なのでこの黒は出ない。ユーザーの決定で `r.EyeAdaptation.PreExposureOverride=1` を `Config/DefaultEngine.ini` に入れ、黒が消えたこと・ふだんの絵が変わらないことを確かめた（実装記録 00・04）。
4. **Cascade**: `UParticleSystem`（`Engine/Classes/Particles`）、全モジュール、Cascade エディタ（`Source/Editor/Cascade`）、`UParticleSystemFactoryNew`、`Plugins/FX/CascadeToNiagaraConverter` がある。`P_ky_cutter2` は Cascade のまま原作の値で組める。本作の方針（UE に同じ仕組みがあれば値を写す）なら Cascade が素直。
5. **デカール**: UE5 で `DecalBlendMode` は廃止（`FUE5MainStreamObjectVersion::RemoveDecalBlendMode`、`Material.cpp` 3114 行）。Deferred Decal ドメイン・Translucent で Emissive だけをつなげば DBM_Emissive 相当。`DecalSize` は半径（`CalcBounds` が `FBoxSphereBounds(0, DecalSize, …)`）で UE4 と同じ。
6. **SpringArm**: 5.8 でも既定値・ラグの式・`GetSocketTransform` は同じ（§2.2）。`SetWorldLocation(..., ETeleportType::TeleportPhysics)` でもラグはリセットされない。
7. **コリジョン**: `Config/DefaultEngine.ini` の `[/Script/Engine.CollisionProfile]` に旧版と同じ `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,DefaultResponse=ECR_Overlap,bTraceType=False,bStaticObject=False,Name="Teleport")` を足す（本作の ini にはまだ無い。確定）。トレースは `LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_GameTraceChannel1), FCollisionQueryParams(…, bTraceComplex=true, 自分を無視))`。ゾーンのメッシュは複雑コリジョンを残して取り込み、コンポーネントは ObjectType Teleport・QueryOnly・非表示。
8. **入力**: 本作は Enhanced Input。原作のアクタ入力（`AutoReceiveInput`、入力を消費しない、軸は毎フレーム）をそのまま真似る必要はなく、プレイヤー側で「照準中の左クリック・ホイール」を扱えばよい。ただし原作ではクリックがプレイヤーの Interact（200 cm）にも届く点、ホイールは 1 目盛り ±1 の値である点を合わせる。Enhanced Input の Triggered は値 0 のフレームで呼ばれないので、距離は Alpha が変わったときに計算すれば同じ。
9. **音**: 3 つの 2D 音と 1 つの 2D ループ（§5.5）。AudioComponent を使うなら減衰なし・自動再生・アクタ破棄で停止。
10. **パーティクルの向き**: 照準アクタはワールド回転 0 のまま（プレイヤーに付けない）で、SpringArm だけを動かす構成にすれば、デカールとパーティクルの向き・スケールの合成が原作と一致する。

---

## 8. WebGL 版の記録（05-player-controller.md）と原作データの食い違い

| 項目 | WebGL 版の記録 | 原作データ（旧版） |
|---|---|---|
| 斬撃の形 | 「300 cm × (0.665, 0.2) の楕円」 | PSA_Square なので**正方形** 199.6 cm（× 1→2）。§5.4 |
| パーティクルの高さ | 輪の中心の床 + 0〜0.01 m | 原点は床（当たり点）の **13.75 cm 上**、斬撃はそこから 0〜1 cm |
| 火花の大きさ | 0.02〜0.05 m | 5〜15 cm × 0.665 = 3.3〜10.0 cm |
| 斬撃の色 | (4, 1.2, 2) に変更（原作は (13, 0, 0.22)） | 粒子色 (13, 0, 0.216663) に加え、マテリアルに `hilightColor` (3.905, 4.096, 5.0)・`colorCorrect` 2.0・`alphaDensity` 1.5 がある（グラフは不明） |
| 斬撃のテクスチャ | キャンバスで描いた近似 | 原作 `T_ky_slash01_4x4.png` が取り込める（本作は原作の素材を使う方針） |
| デカールの色・形 | 推定 | 同じく回収不可（§5.3） |
| 最初に床を捉えたとき | 即座にその位置 | 0.5 s までは即座、以後はスポーン位置（50 m 下）からラグで追う |
| 2 回目のクリック（0.12 s 内） | 無視（最初の位置で確定） | `Location` を更新する（DoOnce が代入の後） |
| 照準ループのフェード | 入り 0.02 s・出 0.03 s | フェードの指定なし（自動再生・破棄で停止） |
| 共有の 0.5 s ロック | なし | 使用可能なパワーの発動後 0.5 s はどちらの枠も発動しない |
| 使えない Q の判定 | クールダウン中かジャンプ中 | ジャンプの判定は無い。`CanMove?`（Use Power）と `Can Interact?`（入力）が前提。取り消しは「使用中の一覧に Teleport」かつ「同じ側」 |
| クリックの副作用 | なし | プレイヤーの左クリックの Interact（手持ち Use／前方 200 cm）も走る |
| FOV の合成 | 焦点距離の比で広げる（`widen`） | UE4 は度で加算（推定: `+ (key − 90)`、5〜170 に収める）。WebGL の `widen` はブラウザ向けの置き換え |
| 最新版の赤い閃光 | 「0.12 s 後」 | **クリックと同時**に開始（BlendWeight = Alpha × 100） |
| `pak_reference/README.md` のオフセット | カプセルを戻すのは @351・@461 | `.txt` では @307・@417 |

一致を確かめたもの: 距離 `Lerp(250, Max, Alpha)`・Q のたびに 0.6・ホイール ±0.1、トレース 500 cm（Teleport チャンネル）、ラグ開始 0.5 s・ラグ速度 10（UE 既定）、+125 cm、0.12 s、カプセルの Ignore/Block、クールダウン 5 s（00_Ballroom 1 s）、枠の 0.05 s、音量（1.75 / 0.65 / 1.0 / 0.5 / 0.35）、CameraAnim のキーと接線、シェイクの値と rad/s の解釈、火花の数・寿命・範囲・速さ・色、斬撃の数・寿命・回転・大きさの伸び・コマの進み、デカールの 2 m 四方。

---

## 9. 未解決・観察が要るもの

1. ~~CameraAnim の FOV の基準（90 か BaseFOV 137.24 か）~~ → 2026-09-16 に旧版の実機で 90（t=0 のキー）と確定。§5.1。
2. `M_Decal_Teleport` の色・グラデーションの半径と鋭さ、`M_ky_slash01_4x4`・`PPP_Radial_Gradient_Doffed` のグラフ（定数は cook で消えている）。
3. UE4.21 のカメラアニメの PP がカメラの PP の上か下か（本作のカメラは PP を上書きしていないので結果は同じはず）。
4. ~~UE5.8 で +100 EV のプレエクスポージャが問題を起こさないか~~ → 起こした（黒いフレーム）。`r.EyeAdaptation.PreExposureOverride=1` で対処（§7-3）。
5. Zone 1 の `BP_Power_Teleport_Zone_2.Cube` と救急車の Cube がアーキタイプの Z スケール 0.05 を継ぐこと（書き出しの world scale とは食い違う。本作のパイプラインがアーキタイプの値を補っているかを確認）。

## 参照したファイル（主なもの）

- `C:\Users\User\Desktop\wasami_deception\pak_reference\_bytecode\DDeception\Content\Blueprints\Main\Powers\BP_Power_Teleport.txt`
- `C:\Users\User\Desktop\wasami_deception\pak_reference\_assets\DDeception\Content\Blueprints\Main\Powers\BP_Power_Teleport.json`・`BP_Power_Teleport_Zone.json`・`M_Decal_Teleport.json`・`M_TeleportZone.json`
- `C:\Users\User\Desktop\wasami_deception\pak_reference\_bytecode\DDeception\Content\Blueprints\Main\BP_DD_PlayerCharacter.txt`（@1735, @2505, @4045〜@4481, @7716, @9531〜@11623, @14412〜@14885, @18106〜@18331, @18336, @21560〜@21841, @22224, @24141, @30854〜@31322, @31827, @31914）
- `C:\Users\User\Desktop\wasami_deception\pak_reference\_bytecode\DDeception\Content\UI\BP_Powers.txt`、`pak_reference\_assets\DDeception\Content\UI\BP_Powers.json`
- `C:\Users\User\Desktop\wasami_deception\pak_reference\_camera\CameraAnim_Teleport.json`・`.csv`、`_camera\_camera_shakes.json`、`_assets\DDeception\Content\UI\Menu\Streaks\BP_CameraShake_Streak.json`
- `C:\Users\User\Desktop\wasami_deception\pak_reference\_particles.json`、`_materials.json`、`_textures.json`、`_audio.json`、`_soundcues.json`
- `C:\Users\User\Desktop\decrypt_dd\dd_extracted\DDeception\Config\DefaultEngine.ini`・`DefaultInput.ini`（旧版の設定。`M_Decal_Teleport.uexp` もここ）
- `C:\Users\User\Desktop\wasami_deception\pak_reference_2\_raw\DDeception\Config\DefaultEngine.ini`
- `C:\Users\User\Desktop\wasami_deception\pak_reference_2\_bytecode\DDeception\Content\Blueprints\Main\Powers\BP_Power_Teleport.txt`・`BP_Power_Teleport_Zone.txt`、`_assets\...\BP_Power_Teleport.json`、`_camera\wtfUE4.json`
- `C:\Users\User\Desktop\wasami_deception\pak_reference_2\_levels\06_Hospital_Zone_01.full.json`・`06_Hospital_Zone_02.full.json`・`06_Hospital.full.json`、`_meshes.json`、`_assets\DDeception\Content\Meshes\06_Hospital\hospital_zone_0{1,2}_teleport.json`、`_bytecode\DDeception\Content\06_Hospital_Zone_01.txt`
- `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Cameras\EngineCameras\Source\EngineCameras\`（`LegacyCameraShake.h/.cpp`、`CameraAnimationCameraModifier.h/.cpp`、`WaveOscillatorCameraShakePattern.cpp`）、`Engine\Source\Runtime\Engine\Private\GameFramework\SpringArmComponent.cpp`、`Private\Collision\CollisionProfile.cpp`、`Private\PhysicsEngine\CollisionQueryFilterCallback.cpp`、`Private\Particles\ParticleSystemRender.cpp`・`ParticleModules*.cpp`、`Private\Components\DecalComponent.cpp`、`Private\LocalPlayer.cpp`、`Private\ActiveSound.cpp`、`Runtime\Renderer\Private\PostProcess\PostProcessEyeAdaptation.cpp`、`Engine\Shaders\Private\PostProcessEyeAdaptation.usf`、`Runtime\Core\Public\Math\InterpCurve.h`
- Web（FOV の相対化の裏づけ。いずれも検索結果の要約のみ、本文は取得できず）: [CameraAnims | UE 4.27 Documentation](https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/Framework/Camera/Animations)、[UCameraAnimInst | UE Documentation](https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Camera/UCameraAnimInst/)
