# 原作調査: Primal Fear と Telepathy（pak_reference_2 が主、pak_reference は比較）

調査日 2026-09-16。リポジトリのファイルは変更していない。
パスの略記: `P2` = `pak_reference_2/`、`P1` = `pak_reference/`。`BC` = `_bytecode/DDeception/Content/`、`AS` = `_assets/DDeception/Content/`。
`@数字` は `.txt` の逆アセンブルのオフセット。行頭に `@` が無い命令は、直前の `@` の命令の続き。
**確定** = バイトコードかアセットの値で確認した。**推測** = UE の仕様や状況から判断した（要確認）。**未確定** = 確かめられなかった。

---

## 0. 要点

1. **Primal Fear**（`P2/BC/Blueprints/Main/Powers/BP_PrimalPower.txt`）: 使った瞬間に、プレイヤーの位置を中心とする半径 `Range` の球の中にいる Pawn を `SphereOverlapActors` で集める。そのうち `DD_EnemyInterface` を実装する敵に `Set State(2 = Stun, byOrb=False)` を 1 回だけ送る。壁越しにも効き、視線の判定は無い。見た目は 2 秒のタイムラインで、赤い半透明の球（`M_05_Primal`）が膨らみながら消え、全画面のポストプロセス 2 つ（赤い単色化と白い閃光＋色収差）が 0.5 秒／0.29 秒で消える。終わるとアクタを破棄する。気絶の秒数は**敵の側**が決める（病院のナースは 17 秒）。
2. 強化段階（保存値 `Primal Upgrades` が 0〜5）: 範囲は 1500/1500/2000/2500/3000/3500、クールダウンは 35/35/32/29/26/23 秒。DataTable の表示（Upgrade2〜6）と一致する。レベル名が `00_Circus_Entrance` のときだけクールダウンは 5 秒。
3. **Telepathy**（最新版で作り直されている）: `BP_Telepathy` を原点に出す。`BP_Telepathy` は 0.8 秒ごとに `GetAllActorsWithInterface(DD_EnemyInterface)` を実行し、`No Telepathy` が False の敵ごとに `BP_TelepathyTracker` を 1 つずつ出す。トラッカーは**画面空間（Screen）のウィジェットコンポーネント**で、`UMG_TelepathyTracker` を表示する。中身は赤い加算の UI マテリアル `MM_Telepathy` の煙状の円。毎 Tick 敵の位置に移り、距離に応じて大きさが 0.5（0 cm）から 0.1（10000 cm）へ変わる。壁越しに見えるのは画面空間のウィジェットだからで、カスタム深度やポストプロセスのマテリアルは**使っていない**。
4. Telepathy の強化段階（`Telepathy Upgrades` が 0〜5）: 効果時間は 5/5/6/7/8/9 秒、クールダウンは 8.5/8.5/8.0/7.5/7.0/6.5 秒。DataTable と一致する。
5. 旧版（UE 4.21）の Telepathy は別の実装だった。プレイヤーの `Telepathy Func` が全敵の `Custom Depth(Duration)` を呼び、敵のメッシュの `RenderCustomDepth` を点ける方式。最新版ではこの呼び出しが消えている（`Custom Depth` はインターフェースと敵に残るが、誰も呼ばない）。
6. 病院 Zone 1・Zone 2 のレベル BP に、この 2 つのパワーの特別扱いは無い。Zone 2 のマトロン（ミニボス `BP_06_Matron_MiniBoss`）は `Enemy` タグを持つが、インターフェースを実装していない。そのため Primal は効かず、Telepathy にも出ない。
7. 地図: Telepathy のトラッカーはタブレットの地図（SceneCapture の `ShowOnlyActors`）に**載らない**。
8. パーティクル（Cascade）はどちらのパワーでも使っていない。

---

## 1. 共通: プレイヤー側の「パワーを使う」流れ（`P2/BC/Blueprints/Main/BP_DD_PlayerCharacter.txt`）

### 1.1 入力から Use Power まで（確定）
- 入力の割り当て（`P2/_raw/DDeception/Config/DefaultInput.ini` 54〜55, 66〜67 行）: `Use Power Left` = Q / Gamepad_LeftTrigger、`Use Power Right` = E / Gamepad_RightTrigger。
- `InpActEvt_Use Power Left`（@23370）: `Can Interact?` が偽なら終わる。DoOnce を通って `Use Power(True)` を呼ぶ。`Delay 0.5` の後（再開先 @6833）に DoOnce を戻す。`Use Power Right`（@23521）は `Use Power(False)` を呼び、0.5 秒後に @6810 で戻す。
- `Use Power`（関数の入口は IntConst 29314）:
  - @29314 `Can Use Tablet?`、@29324 `Has Input` のどちらかが偽なら終わる。
  - @29334 `Powers.Animation(引数)` を呼ぶ。`Powers` は `UMG_TabletPowers`。使えるかどうかの判定より**前**に呼ぶ。
  - @29398 `Power To Use = Powers.Power[ 引数 ? Left Power Index : Right Power Index ]`（@29448 の Select で True→Left、False→Right）。
  - @29545 → @11703 `Power To Use.Available?` が偽なら @16335 へ行く。@16335〜@16460 は `Powers.Power` の長さを見るだけで、何もしない。**使えないときの音は無い**。旧版は `power_not_ready` を音量 0.35 で鳴らしていた（`P1` の同じファイルの @14826）。最新版はこの音を参照していない（`P2/_diff.json` の `project_references.BP_DD_PlayerCharacter`）。
  - @11726 で @16280 を積む。そこでは `Delay 0.5` の後に @1668 で下の DoOnce を戻す。つまり**どのパワーでも、使ってから 0.5 秒は次のパワーを使えない**。
  - @11776〜@11791 DoOnce、@11802 `UsedPower` を通知する。
  - @11821〜@12131 `Power To Use.Power` で分岐する。2 → @16461（Telepathy）、3 → @18337（Primal）。
- パワーの列挙（`P2/AS/Blueprints/Enums/Enum_RingAltar_Skills.json`）: 0 Speed Boost、1 Teleport、2 Telepathy、3 Primal Fear、4 Telekinesis、5 Vanish、6 None。

### 1.2 強化段階の取得（確定）
`Get Power Upgrade Level(Power)`（同ファイル 10543 行以降）: `GameMode.Global Save Instance` の `Telepathy Upgrades`（Power=2）または `Primal Upgrades`（Power=3）を返す（int）。`BP_DD_SaveGame` の既定値は 0。
値の表は 0〜5 の 6 通りあり、**0 と 1 は同じ値**。

### 1.3 クールダウンの表示（`P2/BC/UI/BP_Powers.txt`、確定）
- プレイヤーの `PowerController`（`BP_Powers`）の `Set Delay(Duration, Power)`（@1609）が、Power に応じて `Set Delay Telepathy`（@2349）または `Set Delay Primal`（@3060）を呼ぶ。
- 各パワーは長さ 1.0 秒・線形 0→1 のタイムラインを 1 本ずつ持つ（Telepathy は `Timeline_1`、Primal は `Timeline_2`。`P2/AS/UI/BP_Powers.json` の `CurveFloat_0_1_2`・`_3`）。`SetPlayRate(1/Duration)` の後、FlipFlop で分岐する（Telepathy は @2024、Primal は @1161）。**1 回目は `ReverseFromEnd`、2 回目は `PlayFromStart`**（@532/@598、@709/@775）。
- 更新（Telepathy は @454、Primal は @631）で、`Powers.Telepathy` または `Powers.Primal Fear`（`UMG_TabletPowers` が `MM_Powers_Inst_Telepathy` / `MM_Powers_PrimalFear` から作る MID）の `Percent` にトラックの値を入れる。
- したがってアイコンの `Percent` は、1 回目の `Set Delay` の秒数で 1→0 に減り、2 回目の秒数で 0→1 に戻る。

### 1.4 回復とリセット（確定）
- クールダウンの `Delay` が終わると Gate を通り（Telepathy は @3762〜@3870、Primal は @6301〜@6434）、`PlaySound2D(/Game/Audio/UI/power_refilled, 音量 0.5, ピッチ 1.0)` を鳴らす（Telepathy は @3880、Primal は @6444）。続いて `Available?` を True に戻し、`Powers.Update Powers()` を呼ぶ。
- `Reset Primal`（入口は IntConst 37261）: @37261 で @6444 を積む → @27788 → @6311（Gate の初期化）→ @27394 で Gate を閉じる → @6444 で回復音を鳴らし、すぐ使える状態に戻す。
- `BP_Powers.Reset All Powers`（@2551）が戻すのは Speed Boost 1/2・Teleport・Primal（`Stop Primal Timeline` は @3185 で Stop した後 `Percent=1.0`）・Vanish だけ。**Telepathy は戻さない**。`Stop Telepathy Timeline`（@2972）はどこからも呼ばれていない（全バイトコードを grep した）。
- `Reset Powers` の呼び出し元は `P2/BC/Blueprints/UMG/UMG_DeathScreen.txt` @208（死亡画面からやり直すとき。直後に `OpenLevel` で現在のレベルを開き直す）。

### 1.5 `Active Powers`（確定）
使っている間、プレイヤーの `Active Powers`（byte 配列）にパワーの番号が入る。Telepathy は効果時間の間、Primal は 0.06 秒だけ入る。`BP_DD_Functions` の `Is Player Using Power ?` などが読む（敵やトラップ側の判定用）。

---

## 2. Primal Fear

### 2.1 プレイヤー側（`BP_DD_PlayerCharacter.txt` @18337〜、確定）
| @ | 処理 |
|---|---|
| 18337 | `UsedPrimal` を通知 |
| 18376 | `Active Powers` に 3 を AddUnique |
| 18444〜18714 | `Powers.Power` の Primal を `Available?=False` にして `Update Powers()` |
| 18750 | `PowerController.Set Delay(0.05, 3)`（アイコンを 0.05 秒で空にする） |
| 18869 | 出現位置 = `GetPlayerCharacter(0)` の位置 − (0, 0, 5000) |
| 18982 | `BeginDeferredActorSpawnFromClass(BP_PrimalPower_C, 回転 0・拡大 1, 衝突処理 1 = AlwaysSpawn, Owner なし)` |
| 19024〜19139, 19245 | `SetFloatPropertyByName('Range', 段階の表)`。0→1500、1→1500、2→2000、3→2500、4→3000、5→3500 |
| 19568 | `FinishSpawningActor`（同じ変換） |
| 19621 | `Delay 0.06` → 再開は @5187 |
| 5187〜5355 | クールダウンの表: 0→35、1→35、2→32、3→29、4→26、5→23。`GetCurrentLevelName == '00_Circus_Entrance'`（@5378）なら 5.0 |
| 5498 | `PowerController.Set Delay(クールダウン, 3)` |
| 5733 | `Active Powers` から 3 を除く |
| 6084 | `Delay(クールダウン)` → @6301（回復、1.4 節） |

- 旧版（`P1` の同じファイル @16631〜）も、値（1500〜3500、23〜35、0.05、0.06、−5000、`00_Circus_Entrance` で 5）は同じ。

### 2.2 BP_PrimalPower の構成（`P2/AS/Blueprints/Main/Powers/BP_PrimalPower.json`、確定）
- 親は `Actor`。CDO は `Range = 1500.0`（プレイヤーが出すときに上書きする）。タグなし。
- SCS:
  - `DefaultSceneRoot`（SceneComponent、既定値のまま）
    - `Sphere`（StaticMeshComponent）: `StaticMesh=/Engine/BasicShapes/Sphere`（半径 50 cm。`P2/_meshes.json` の BoxExtent 50）、`OverrideMaterials=[/Game/Materials/05_Circus/M_05_Primal]`、`BodyInstance` は `CollisionProfileName=NoCollision`・`CollisionEnabled=NoCollision`・`ObjectType=ECC_WorldStatic`・Visibility と Camera を Ignore。相対変換は既定（原点・拡大 1）。
    - `PostProcess`（PostProcessComponent）: `BlendWeight=0.0`、`Settings`: `bOverride_ColorSaturation=True`・`ColorSaturation=(0,0,0,1)`、`bOverride_ColorGain=True`・`ColorGain=(1.61, 0.129563, 0.0, 1)`。
    - `PostProcess1`（PostProcessComponent）: `BlendWeight=0.0`、`Settings`: `bOverride_ColorGamma=True`（`ColorGamma` の値は書かれていないので既定の (1,1,1,1)。**実質効かない**）、`bOverride_ColorGainMidtones=True`・`ColorGainMidtones=(100,100,100,1)`、`bOverride_SceneFringeIntensity=True`・`SceneFringeIntensity=50.0`。`ColorSaturation=(0,0,0,1)` と `ColorGain=(1.61,0.1296,0,1)` も値はあるが、override フラグが無いので効かない。
    - 両方とも `bUnbound`・`Priority`・`BlendRadius` は書かれていないので UE 4.24 の既定値（**推測**: `bUnbound=true`、Priority 0、BlendRadius 100）。つまり全画面に効く。
  - `Timeline_0`（TimelineTemplate）: `TimelineLength=2.0`、ループなし、トラックは `float`・`float2`・`desaturation`・`opacity`（2.4 節）。

### 2.3 処理の全体（`BP_PrimalPower.txt`、確定）
1. `ReceiveBeginPlay` → @915
2. @915 `Sphere.CreateDynamicMaterialInstance(0, M_05_Primal)` → @988 `Material Instance` に入れる。
3. @1007〜@1083 `K2_SetActorLocation(プレイヤーの位置, bSweep=False, bTeleport=False)`。出現時の −5000 cm から、プレイヤーの位置（カプセル中心）へ移る。以後はプレイヤーに付いていかない。
4. `PlaySoundAtLocation(Stun_Wave_Attack_New_04, 位置 (0,0,0), 回転 0, 音量 1.0, ピッチ 1.0, 開始 0.0, 減衰なし, 同時発音なし, Owner なし)`（@1083 の直後）。
5. @1187 Sequence（先に 6〜8、後で 9）
6. @1218 `GetPlayerController(0).ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop_C, Scale=25.0, PlaySpace=0（CameraLocal）, 回転 0)`
7. @1283 ObjectTypes = [2]（`ObjectTypeQuery3` = **Pawn**。`DefaultEngine.ini` の独自チャンネルは `ECC_GameTraceChannel1 "Teleport"` だけなので、並びは既定のまま）
8. @1383 `SphereOverlapActors(中心=プレイヤーの位置, 半径=Range, [Pawn], クラスの絞り込みなし, 除外=空配列, OutActors)` → ForEach（@1450→@887→@696→@30）
   - @94 `DoesImplementInterface(Item, DD_EnemyInterface)`、@132 `Item.ActorHasTag('Enemy')`、@187 OR。偽なら次へ（@225）。
   - @294〜@389 `PrintText(表示名, 画面 True, ログ False, 色 (1.0, 0.031128, 0, 1), 2.0 秒)`。デバッグ表示で、Shipping では何も出ない（**推測**: UE の仕様）。
   - @488 `Item` を `DD_EnemyInterface` へキャストし、成功なら @567 `Set State(State=2, byOrb=False)`。
   - @607〜@875 は何にも繋がっていない DoOnce（効果なし）。
9. @1478 `Timeline_0.PlayFromStart()`
10. 更新（`Timeline_0__UpdateFunc` → @1993 → @1511）:
    - @1511〜@1641 `Sphere.SetWorldScale3D( Lerp(0, Range, float) / 50 )`。球の半径 = `Range × float`（cm）。
    - @1682〜@1729 `PostProcess.BlendWeight = Lerp(1, 0, float2)`
    - @1778〜@1835 `PostProcess1.BlendWeight = MapRangeClamped(float2, 0, 0.3, 1, 0)`
    - @1884 `Material Instance.SetScalarParameterValue('Desaturation', desaturation)`
    - @1938 `Material Instance.SetScalarParameterValue('Opacity', opacity)`
11. 終了（`Timeline_0__FinishedFunc` → @15）: `K2_DestroyActor()`。アクタの寿命は約 2.0 秒。

- **気絶の判定は 0 秒の時点で 1 回だけ、半径 `Range` で行う**。球が広がる見た目とは連動しない。
- 遮蔽物は無視する（トレースが無い）。3D の球なので、上下の階も半径の中なら効く。
- 敵がいなくても、音・揺れ・見た目・クールダウンは同じように起きる（数を見る分岐が無い）。

### 2.4 タイムラインのキー（`BP_PrimalPower.json` の `CurveFloat_0〜3`、確定）
すべて `TangentWeightMode=RCTWM_WeightedNone`。値は (Time, Value, Interp, ArriveTangent, LeaveTangent)。

- `float`（CurveFloat_0）: (0.0, 0.0, Cubic/User, 1.5797575, 1.5797579)、(0.992901, 0.9189547, Cubic/User, 0.3629918, 0.3629917)、(1.51348, 0.9834880, Linear/Auto, 0, 0)
- `float2`（CurveFloat_1）: (−0.0116005, 0.0, Cubic/User, −0.0950284, −0.0950288)、(0.5, 1.0, Cubic/User, 4.5270514, 4.5270581)
- `desaturation`（CurveFloat_2）: (0.0069571, 0.0154881, Cubic/Auto, 0, 0)、(1.0165639, 0.1703681, Cubic/Auto, 0.6986592, 0.6986592)、(1.4109414, 0.9963947, Linear/Auto, 0, 0)
- `opacity`（CurveFloat_3）: (0.0, 1.0, Cubic/User, −0.2174575, −0.2174581)、(0.6773992, 0.7253548, Cubic/User, −0.5983154, −0.5983162)、(1.5923555, 0.0025815, Linear/Auto, 0, 0)

UE4 の FRichCurve の評価（区間の左のキーの補間モードで決まる。3 次は Bezier で、P1 = P0 + Leave×Δt/3、P2 = P3 − Arrive×Δt/3。範囲外は端の値）で標本化した結果（計算値）:

| t [s] | float | float2 | desaturation | opacity | 球の半径（Range 1500） | PostProcess の重み | PostProcess1 の重み |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0.00 | 0.000 | −0.001 | 0.015 | 1.000 | 0 | 1.001 | 1.000 |
| 0.10 | 0.151 | 0.029 | 0.014 | 0.976 | 226 | 0.971 | 0.902 |
| 0.20 | 0.287 | 0.132 | 0.009 | 0.946 | 430 | 0.868 | 0.558 |
| 0.30 | 0.409 | 0.321 | 0.005 | 0.910 | 613 | 0.679 | 0.000 |
| 0.40 | 0.518 | 0.606 | 0.002 | 0.869 | 776 | 0.394 | 0 |
| 0.50 | 0.614 | 1.000 | 0.004 | 0.822 | 921 | 0.000 | 0 |
| 0.80 | 0.831 | 1 | 0.059 | 0.635 | 1246 | 0 | 0 |
| 1.00 | 0.921 | 1 | 0.159 | 0.438 | 1382 | 0 | 0 |
| 1.20 | 0.969 | 1 | 0.577 | 0.230 | 1453 | 0 | 0 |
| 1.40 | 0.983 | 1 | 0.995 | 0.066 | 1474 | 0 | 0 |
| 1.60〜2.00 | 0.983 | 1 | 0.996 | 0.003 | 1475 | 0 | 0 |

`PostProcess1` の重みは t ≈ 0.291 秒で 0 になる。球は最終的に `Range` の 98.3% まで広がる。

### 2.5 見た目
- **球**（確定）: 半径 50 cm の球を、プレイヤーの位置を中心に 0 から `0.983×Range` まで広げる。プレイヤーは球の内側にいるので、`M_05_Primal` の `TwoSided=true` が効く。
- **M_05_Primal**（`P2/AS/Materials/05_Circus/M_05_Primal.json`、`P2/_materials.json`）:
  - 確定: `BlendMode=BLEND_Translucent`、`TwoSided=true`、`bUsedWithStaticLighting=true`（旧版との違いはこのフラグだけ）。シェーディングモデルは書かれていないので既定の DefaultLit（**推測**）。
  - パラメータ: `Color`（Vector、既定 (1,0,0,1)。Primal は変更しない＝赤）、`Opacity`（Scalar、既定 1.0）、`Desaturation`（Scalar、既定 0）。
  - テクスチャ: `TextureSample_0` = `/Game/Textures/05_Circus/T_05_PortalMaps`（座標は `MaterialExpressionPanner_1` から）。
  - `EmissiveColor` は `MaterialExpressionAdd_2` に繋がっているが、このノードは cook で消えている。書き出しに残る入力は EmissiveColor だけで、Opacity 入力の接続は無い。
  - **グラフは再現に推測が要る**。推測: Emissive = Color × (パンさせた T_05_PortalMaps のチャンネル) + … に Desaturation をかけ、Opacity パラメータ × テクスチャのチャンネルを不透明度に入れる。
  - T_05_PortalMaps は 2048² の DXT1、sRGB=false。チャンネル詰めのマスクで、R はほぼ空、G は中心の光、B は粒状のノイズ（縮小画像で目視）。
- **ポストプロセス**（確定、値は 2.2 節）:
  - `PostProcess`: 彩度 0（白黒）× ゲイン (1.61, 0.13, 0) で**赤い単色**の画面になる。0.5 秒で消える。
  - `PostProcess1`: 中間調のゲイン ×100（**白く飛ぶ閃光**）と色収差 50。約 0.29 秒で消える。
- **カメラシェイク**（`P2/_camera/_camera_shakes.json`・`P2/AS/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.json`、確定）: 親は `CameraShake`。`OscillationDuration=0.5`、`BlendInTime=0.0`、`BlendOutTime=0.5`。`LocOscillation`: X 振幅 2.0・周波数 50、Y 2.0・35、Z 3.0・10。回転と FOV の揺れは無い。`InitialOffset` などは書かれていないので既定（**推測**: EOO_OffsetRandom）。Scale 25 なので、実際の振幅は X/Y 50 cm、Z 75 cm。
- パーティクル・デカール・ウィジェットは**使っていない**（BP の imports に無い。確定）。

### 2.6 音（確定）
- `/Game/Audio/SharedGameplay/Stun_Wave_Attack_New_04`: SoundWave、2ch、44100 Hz、1.710 秒、SoundClass `DD_SoundClass_SFX`（音量の指定なし＝1.0）。減衰設定なし。
  - OGG: `P2/DDeception/Content/Audio/SharedGameplay/Stun_Wave_Attack_New_04.ogg`
  - 呼び出し: `PlaySoundAtLocation`、位置は定数 (0,0,0)、音量 1.0、ピッチ 1.0。
  - **推測**: 減衰設定が無いので空間化されず、2D と同じに聞こえる（UE の仕様）。
- 回復音 `/Game/Audio/UI/power_refilled`: 2ch、48000 Hz、1.515 秒、SFX。`PlaySound2D`、音量 0.5。OGG は `P2/DDeception/Content/Audio/UI/power_refilled.ogg`。

### 2.7 敵への通知と、効かない条件
- 通知（確定）: `DD_EnemyInterface.Set State(State: Enum_EnemyStates = 2, byOrb: bool = False)`。**気絶の秒数は渡さない**。
- `Enum_EnemyStates`（`P2/AS/Blueprints/Enums/Enum_EnemyStates.json`）の並びは Patrol, Pursue, Stun, Teleport。値 2 = **Stun**（**確定**: ナースが State==2 のときに気絶処理をし、接触しても殺さないことで裏付けた。4.2 節）。比較: Power Orb（`P2/BC/Blueprints/Main/BP_PowerOrb.txt` 167 行）は `Set State(2, True)`。
- 効かない条件（確定）:
  1. 敵のどのプリミティブも、オブジェクトタイプが Pawn でない（キャラクターのカプセルは Pawn）。
  2. `Enemy` タグも無く、インターフェースも実装していない。
  3. タグはあるがインターフェースを実装していない（キャストに失敗し、`Set State` を呼ばない）。例: `BP_06_Matron_MiniBoss`。
  4. `Set State` の実装が State 2 を処理しない敵（例: `BP_DD_Character_Base` の既定の実装は空）。
  5. 敵の側の都合で処理が遅れる、または止まる（4.3 節の見張りナース）。
  - 旧版の判定は `ActorHasTag('Enemy')` だけで、最新版で `DoesImplementInterface` との OR になった。

### 2.8 強化段階（DataTable との照合、確定）
`P2/_datatables.json` の `/Game/UI/RingAltar_UI/Enums/PowersTable_Primal`:

| 行 | 表示 | コードの段階 | コードの値 |
|---|---|---|---|
| Upgrade2 | RANGE 1500 / COOLDOWN 35 | 0・1 | 1500 / 35 |
| Upgrade3 | RANGE 2000 / COOLDOWN 32 | 2 | 2000 / 32 |
| Upgrade4 | RANGE 2500 / COOLDOWN 29 | 3 | 2500 / 29 |
| Upgrade5 | RANGE 3000 / COOLDOWN 26 | 4 | 3000 / 26 |
| Upgrade6 | RANGE 3500 / COOLDOWN 23 | 5 | 3500 / 23 |

どの行も Cost は 2。`UMG_RingAltar` は行名の配列の i 番目を `Rich Upgrade Text Descriptions[i]` に表示する（`P2/BC/UI/RingAltar_UI/UMG_RingAltar.txt` @8313〜@8975）。行と保存値の対応は値が一致することから判断した。

### 2.9 旧版（P1）との違い（確定）
- `BP_PrimalPower`: 敵の判定（2.7 節）と、`Set State` の引数（旧版は `Set State(2)` だけで byOrb が無い）。旧版には空の `UserConstructionScript` がある。コンポーネント・タイムライン・`Range` の値はすべて同じ。
- `M_05_Primal`: `bUsedWithStaticLighting` のフラグだけが違う（見た目は同じ）。

### 2.10 BP_StunCollectEffect との関係（確定）
`P2/BC/Blueprints/Main/Powers/BP_StunCollectEffect.txt` は Power Orb を拾ったときの演出（`BP_PowerOrb` と `BP_06_Bossfight_Pickup_PowerOrb` から出す）。Primal と同じ作りだが、次の点が違う。
- **敵を気絶させない**（SphereOverlap が無い。気絶は `BP_PowerOrb` 側の `Set State(2, True)`）。
- 音のピッチが 2.0。
- `Color` を (0.258, 0.0737, 0, 1)（橙）にする（@209）。Primal は既定の赤のまま。

---

## 3. Telepathy

### 3.1 プレイヤー側（`BP_DD_PlayerCharacter.txt` @16461〜、確定）
| @ | 処理 |
|---|---|
| 16461 | `UsedTelepathy` を通知 |
| 16500 | `Active Powers` に 2 を AddUnique |
| 16568 | `PlaySound2D(/Game/Audio/SharedGameplay/Telepathy, 音量 0.6, ピッチ 1.0, 開始 0)` |
| 16627〜16897 | Telepathy を `Available?=False` にして `Update Powers()` |
| 16959 | `GetPlayerCameraManager(0).PlayCameraShake(BP_CameraShake_Streak_C, Scale 1.0, PlaySpace 0, 回転 0)` |
| 17101 | `BeginDeferredActorSpawnFromClass(BP_Telepathy_C, 原点・回転 0・拡大 1, 衝突処理 0, Owner なし)` |
| 17143〜17258, 17364 | `SetFloatPropertyByName('Time', 表)`。0→5、1→5、2→6、3→7、4→8、5→9 |
| 17565 | `FinishSpawningActor` |
| 17793 | `PowerController.Set Delay(Time, 2)`（アイコンが効果時間をかけて 1→0 に減る） |
| 18164 | `Delay(Time)` → 再開は @4271 |
| 4271〜4291 | `Active Powers` から 2 を除く |
| 4351 | `PlaySound2D(/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered, 音量 1.0, ピッチ 1.5)`（終わりの音） |
| 4410〜4525 | クールダウンの表: 0→8.5、1→8.5、2→8.0、3→7.5、4→7.0、5→6.5 |
| 4600 | `PowerController.Set Delay(クールダウン, 2)`（アイコンが 0→1 に戻る） |
| 4971 | `Delay(クールダウン)` → @3762（回復、1.4 節） |

- 効果時間は、プレイヤーの Delay と `BP_Telepathy` の `Finish` タイマーの 2 つがそれぞれ `Time` 秒を数える。

### 3.2 BP_Telepathy（`P2/BC/Blueprints/Main/Powers/Telepathy/BP_Telepathy.txt`、確定）
- 構成（`P2/AS/.../Telepathy/BP_Telepathy.json`）: 親は Actor、`DefaultSceneRoot` だけ。変数は `Time`（float、既定 0。出すときに入れる）と `Actors With Tracker`（配列）。
- `ReceiveBeginPlay` → @72: すぐ `Update Targets()` を呼ぶ → @86 `K2_SetTimer('Update Targets', 0.8, ループ True)` → @10 `K2_SetTimer('Finish', Time, ループ False)`。
- `Update Targets`:
  - `GetAllActorsWithInterface(DD_EnemyInterface)` で**レベル内の全敵**を集める（距離の制限は無い）。
  - 各敵を @293 でインターフェースへキャストする。成功なら @372 `No Telepathy(out b)`、失敗なら @512 で b=False。
  - @418 b が True なら飛ばす。
  - @606 `Actors With Tracker` に入っていれば飛ばす。
  - @681〜@794 `BeginDeferredActorSpawnFromClass(BP_TelepathyTracker_C, 敵の位置・回転 0・拡大 1, 0)` → `SetObjectPropertyByName('Actor', 敵)` → @990 `FinishSpawningActor` → @1028 `Actors With Tracker` に追加。
  - 効果中に現れた敵も、次の 0.8 秒の更新で拾う。
- `Finish`: `K2_ClearTimer('Update Targets')` → `GetAllActorsOfClass(BP_TelepathyTracker)` の全部に @320 `Remove()` → 自分を `K2_DestroyActor()`。
- 敵がいないとき: 何も出ないだけで、使用・音・揺れ・クールダウンは同じ（分岐が無い）。

### 3.3 BP_TelepathyTracker（`BP_TelepathyTracker.txt`・`.json`、確定）
- 構成: 親は Actor、CDO は `PrimaryActorTick.bCanEverTick=true`。`DefaultSceneRoot` の下に `Widget`（**WidgetComponent**）: `Space=EWidgetSpace::Screen`、`WidgetClass=UMG_TelepathyTracker_C`、`WindowVisibility=Visible`。ほかの値（DrawSize・Pivot・bDrawAtDesiredSize など）は書かれていないので既定（**推測**: UE 4.24 の既定は DrawSize 500×500、Pivot (0.5,0.5)、bDrawAtDesiredSize=false）。
- `ReceiveBeginPlay` → @270: `Widget.GetUserWidgetObject()` を `UMG_TelepathyTracker` へキャストして `Widget Reference` に入れ、`Open Gate()` を呼ぶ。Gate は閉じた状態で始まり、@411→@245→@233 で開く。
- `ReceiveTick` → @265 → Gate が開いていれば `Update()`。
- `Update`:
  - `IsValid(Actor)` が偽なら `Remove()`。
  - 真なら @93 `K2_SetActorLocation(Actor の位置, sweep なし)`（毎 Tick 敵の原点＝カプセル中心へ移る）。
  - @159 `GetDistanceTo(プレイヤー)` → @196 `MapRangeUnclamped(距離, 0, 10000, 0.5, 0.1)` → @253 `Widget Reference.Set Size(値)`。
  - クランプしないので、12500 cm で 0、それより遠いと負の値になる（**推測**: 反転して表示される）。
- `Remove` → @416: `Close Gate()`（追従と大きさの更新を止める）→ @430 `Widget Reference.Remove()`（消えるアニメ）→ @15 `Delay 0.5` → @70 `K2_DestroyActor()`。

### 3.4 UMG_TelepathyTracker（`UMG_TelepathyTracker.txt`・`.json`、確定）
- ウィジェットの木: ルートは `SizeBox_54`（`WidthOverride=256`、`HeightOverride=256`、変数）。その中に `Image_90`（`Brush.ResourceObject=/Game/Blueprints/Main/Powers/Telepathy/MM_Telepathy_Inst`、ほかの Brush の値は既定）。
- `Construct` → @511 → @154:
  - `PlayAnimation(Appear, 開始 0.0, ループ 1, Forward, 速度 1.0, RestoreState False)`
  - @201 `Image_90.GetDynamicMaterial()`
  - @243 `'Tiling' = RandomFloatInRange(0.5, 1.5)`
  - @335 `'Speed' = RandomFloatInRange(0.5, 1.5)`
  - @427〜@465 `Image_90.SetRenderTransformAngle(RandomFloatInRange(0.5, 360.0))`
- `Remove` → @102: `PlayAnimation(Disappear, 0.0, 1, Forward, 1.0, False)`
- `Set Size(Float)` → @10: `SizeBox_54.SetRenderScale((Float, Float))`
- アニメ（チックは既定の 60000/秒。表示レート 20 fps）。どちらも `Image_90` の `RenderTransform` の Scale（X/Y 同じ）と `RenderOpacity` を動かす:
  - **Appear**（0〜30001 チック ≈ 0.5 秒）: Scale のキー 0→0.0、15000→1.0（接線 3.0e-5/チック）、30000→0.95。すべて Cubic/Auto。Opacity は 0→0.0、30001→1.0（Cubic/Auto、接線 0）。
  - **Disappear**（0〜18000 ≈ 0.3 秒）: Scale のキー 0→1.0、9000→1.1（接線 −3.33e-5）、18000→0.0。Opacity は 0→1.0、18000→0.0。

### 3.5 MM_Telepathy（`P2/AS/.../Telepathy/MM_Telepathy.json`・`MM_Telepathy_Inst.json`）
- 確定:
  - `MaterialDomain=MD_UI`、`BlendMode=BLEND_Additive`。
  - `EmissiveColor` = `VectorParameter "Color"` の RGB。既定は (1, 0, 0, 1) で、**赤**。
  - パラメータ: `Tiling`（既定 1.0）、`Speed`（既定 1.0）。
  - テクスチャ: `TextureSample_0` = `/Game/ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16`（座標は `Panner_0`）、`TextureSample_1` = `/Game/ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise`（`Panner_1`）。
  - 使っているマテリアル関数: `/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential`、`/Engine/Functions/Engine_MaterialFunctions01/Density/ExponentialDensity`。
- `MM_Telepathy_Inst`（MIC）: `Size=1.0`、`Speed=1.0`、`RefractionDepthBias=0`、上書きは `BlendMode=Additive`・`ShadingModel=Unlit`。`Size` の GUID は親に書き出されている式に無い。
- **グラフの大半（パンナー・乗算など、Opacity 側の接続）は cook で消えていて、再現には推測が要る**。
  - 推測: Opacity = (Tiling でタイル・Speed でパンした 2 枚のノイズ) × `RadialGradientExponential`（中心が濃い円）→ `ExponentialDensity`。見た目は「赤い煙の丸い塊」。
- テクスチャ（`P2/_textures.json`）:
  - `T_ky_noise16` は 1024² の DXT1、sRGB=false、Effects グループ。縦の筋状ノイズ。
  - `T_ky_noise` は 512² の DXT1、sRGB=true、Effects グループ。雲状ノイズ。
  - どちらも RGB のチャンネルに別々の模様が入っている（縮小画像で目視）。

### 3.6 音と画面の演出（確定）
- 開始の音: `/Game/Audio/SharedGameplay/Telepathy`。SoundWave、2ch、44100 Hz、1.760 秒、SoundClass なし。`PlaySound2D`、音量 0.6。OGG は `P2/DDeception/Content/Audio/SharedGameplay/Telepathy.ogg`。
- 終わりの音: `/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered`（**エンジンの VREditor の音**）。2ch、48000 Hz、1.956 秒。音量 1.0、ピッチ 1.5。OGG は `P2/Engine/Content/VREditor/Sounds/UI/Teleport_Mode_Entered.ogg`。
- 回復音は `power_refilled`（音量 0.5）。
- カメラシェイク `BP_CameraShake_Streak`（`P2/AS/UI/Menu/Streaks/BP_CameraShake_Streak.json`）: `OscillationDuration=0.5`、`BlendIn=0.0`、`BlendOut=0.25`。`RotOscillation`: Pitch 振幅 0.25・周波数 30、Yaw 0.25・40、Roll 0.5・35。`FOVOscillation`: 振幅 2.0・周波数 10。位置の揺れは無い。Scale 1.0。
- ポストプロセス・画面全体のウィジェット・カスタム深度は**使っていない**。画面に出るのはトラッカーだけ。

### 3.7 強化段階（DataTable との照合、確定）
`PowersTable_Telepathy`:

| 行 | 表示 | コードの段階 | コードの値 |
|---|---|---|---|
| Upgrade2 | DURATION 5.0 / COOLDOWN 8.5 | 0・1 | 5 / 8.5 |
| Upgrade3 | DURATION 6.0 / COOLDOWN 8.0 | 2 | 6 / 8.0 |
| Upgrade4 | DURATION 7.0 / COOLDOWN 7.5 | 3 | 7 / 7.5 |
| Upgrade5 | DURATION 8.0 / COOLDOWN 7.0 | 4 | 8 / 7.0 |
| Upgrade6 | DURATION 9.0 / COOLDOWN 6.5 | 5 | 9 / 6.5 |

### 3.8 タブレットの地図（確定）
- プレイヤーの `Show Only`（入口は IntConst 32171）が地図の `SceneCaptureComponent2D.ShowOnlyActors` に入れるクラスは、`BP_Player_TelepathyVisualization`・`BP_ArrowPointer`・`BP_Shard`・`BP_MapTexture`・`BP_PowerOrb`・`BP_BonusShard`・`BP_MiniMapMarker`・`BP_06_Miniboss_viewcone`。**`BP_TelepathyTracker` は入っていない**。
- 画面空間のウィジェットはシーンキャプチャに映らない（**推測**: UE の仕様）ので、Telepathy は地図に出ない。
- `BP_Player_TelepathyVisualization`（`Plane` メッシュ＋`/Game/Materials/Player/M_TelepathyRange`。Translucent・Unlit）は地図に載せる対象に入っているが、両方の版で、どこからも生成されず、どのレベルにも置かれていない（全バイトコードとレベルを grep した）。**使われていない名残**。

### 3.9 対象から外す敵（確定）
`No Telepathy` を True で返すのは、全実装のうち `P2/BC/Blueprints/08_BearHouse/BP_08_BearTrap.txt` だけ。ほかの実装（`BP_DD_Character_Base`、病院の `BP_06_ReaperNurse` を含む）はすべて False を返す。インターフェースを実装しない敵（Zone 2 のマトロンのミニボスなど）は、そもそも対象にならない。
- ナースの「消える」状態（`Cloak`）は見ないので、消えているナースも表示される（トラッカーは可視性を判定しない）。

### 3.10 旧版（P1）との違い（確定）
- 旧版には `Blueprints/Main/Powers/Telepathy/` が無い。プレイヤーの `Telepathy Func(Duration)`（@31620）が `GetAllActorsWithInterface(DD_EnemyInterface)` の全部に `Custom Depth(Duration)` を呼んでいた（@20958）。`Remove Telepathy Visual` もあった。
- 敵の `Custom Depth` の実装（例: ナースは @10651 で `Mesh.SetRenderCustomDepth(True)` → `Delay(Duration)` → @3661 で False）は最新版にも残るが、**最新版では誰も呼ばない**（`VirtualFunction Custom Depth(` を全体で grep すると `P1` の 1 件だけ）。
- 旧版でカスタム深度を画面に描いていたポストプロセスは**未確定**。Chameleon に `M_CustomDepthHighlighter(Clip)` があるが、調べたレベル（04_Sewer、02_elementaryschool、05_Circus_Zone1）の Chameleon ではこの効果を有効にしていなかった。
- 表の値（5〜9、8.5〜6.5）、開始の音、揺れ、終わりの音は旧版も同じ。
- ユーザーの決定（Telepathy は `pak_reference_2` に従う）に従えば、最新版のトラッカー方式を採る。

---

## 4. 敵側: DD_EnemyInterface と実装例

### 4.1 インターフェース（`P2/AS/Blueprints/Characters/Shared/DD_EnemyInterface.json`、確定）
関数:
- `Activate Frenzy()`
- `Get Frenzy(out Frenzy: bool)`
- `Get Run Speed(out float)`、`Get Walk Speed(out float)`
- `Get State(out State: Enum_EnemyStates)`
- **`Set State(State: Enum_EnemyStates, byOrb: bool)`**
- **`Custom Depth(Duration: float)`**
- `HammerHit()`
- `Player Vanish()`
- `Chasing (out bChasing: bool)`（関数名の末尾に空白がある）
- **`No Telepathy(out b: bool)`**

旧版には `Player Vanish`・`Chasing `・`No Telepathy` と、`Set State` の `byOrb` が無い。
実装している基底は `P2/BC/Blueprints/Shared/BP_DD_Character_Base.txt`（親は Character）。ここでは `Set State` などのイベントは空、`No Telepathy`・`Chasing ` は False、`Get State` は 0 を返す。

### 4.2 病院のナース `BP_06_ReaperNurse`（`P2/BC/Blueprints/Characters/Nurse/BP_06_ReaperNurse.txt`、確定）
- 親は `BP_DD_Character_Base`。CDO は `Tags=["Enemy"]`、`AutoPossessAI=PlacedInWorldOrSpawned`、カプセルの半高 118.058。
- `Set State` → @9652: `State = 引数`（byOrb は使わない）。
- 気絶の処理:
  - `Make Choice`（`K2_SetTimer('Make Choice', 0.5, ループ)`。22 行目付近）→ @7107 `State == 2` なら @5940（DoOnce）→ @5772。
  - @5798 `CharacterMovement.StopMovementImmediately()` → `Cloak(False)`（姿を現す）→ `Talk(/Game/Audio/06_Hospital/Nurse/Nurse_Hospital_Zone01_Stunned, True)` → **`Delay 17.0`**（@5888）→ @355 `State = 0`（Patrol）、DoOnce を戻す。
  - したがって、反応までの遅れは最大 0.5 秒、**気絶は 17 秒**。
  - 気絶ボイスは SoundCue で、6 本の波形（`Nurse_Hospital_Zone01_Stunned_01〜06.ogg`、`P2/DDeception/Content/Audio/06_Hospital/Nurse/`）から選ぶ。
- 接触の判定（@9844〜@9933）: 相手がプレイヤーで、かつ `State != 2` のときだけ @5361（捕まえる処理。@5253 で `Jumpscare Handle()`）へ進む。**気絶中は捕まらない**。
- `No Telepathy` は False（@3803）。

### 4.3 派生クラス（確定、一部推測）
- `BP_06_ReaperNurse_06_Chase`（Zone 1 のレベル BP の `Spawn Nurses_06` が出す）: `ReceiveTick` で親の Tick を呼んだ後、@1053 で `State == 2` を見て、@893 `StopMovementImmediately` → `Delay 17.0` → @66 `State = 0`。
- `BP_06_ReaperNurse_Zone2`（Zone 2 の `Spawn Nurses` が出す）: 親の `BP_06_ReaperNurse` のまま（気絶は上書きしていない）。
- `BP_06_ReaperNurse_Sentry`（Zone 2 に 6 体置かれている。親は `_06_Chase`）:
  - `ReceiveTick`（@1018）では、閉じた Gate の後ろで親の `ReceiveTick` を呼ぶ（@647）。
  - Gate は `Player Spotted`（@1035 → @720 → … → @703 から @15）の経路の最後（@519〜@544）で開く。その経路では `Set Struct Achievement('06_NurseAlert')`、視野コーンの破棄、`LaunchCharacter(..., Z=500)`、親の `ReceiveBeginPlay` も行う。
  - **推測**: 見つかる前の見張りナースは、Primal で `State=2` が入っても気絶の処理（17 秒）が動かない。見つかって Gate が開いた時点で、State が 2 のままなら気絶が始まる。
- `BP_06_Matron_MiniBoss`（Zone 2 に 1 体）:
  - 親は **Actor**。`Tags=["Enemy"]`。コンポーネントは SkeletalMesh と `CloseArea`（Box、Pawn を Overlap）。
  - `Set State` も `No Telepathy` も持たない（インターフェースを実装していない）。**Primal も Telepathy も効かない**。
- `BP_06_Nurse_Cutscene`・`BP_ReaperNurse_IntroAI`・`BP_06_Miniboss_viewcone_*`: インターフェースを実装しないので対象外。

---

## 5. 病院 Zone 1・Zone 2 での扱い（確定）
- `P2/BC/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt` の関数一覧と、`primal|telepath|power` の grep（大文字小文字を区別しない）で、パワーに触れる処理は**無い**。
- `UsedPrimal` を購読するのは `05_Circus_Entrance`、`08_BearHouse_Zone_01`、`BP_Ballroom_Event5_Monkey`。`UsedTelepathy` を購読するのは `00_Ballroom`、`00_Ballroom_Damaged`（チュートリアル）。どれも病院ではない。
- ゲームモード: 両ゾーンの WorldSettings にゲームモードの上書きが無いので、`GlobalDefaultGameMode=BP_DD_GameMode`（パワーあり）。`BP_DD_GameMode_NoPowers` を使うのは 08_BearHouse 系。
- クールダウンの特例は `00_Circus_Entrance`（5 秒）だけ。
- ステージの敵:
  - Zone 1: 配置された敵は無い。レベル BP の `Spawn Nurses`（4755 行）が `BP_06_ReaperNurse`、`Spawn Nurses_06`（4923 行）が `BP_06_ReaperNurse_06_Chase` を出す。
  - Zone 2: `BP_06_ReaperNurse_Sentry` ×6 と `BP_06_Matron_MiniBoss` ×1 が置かれている。`Spawn Nurses`（6919 行）が `BP_06_ReaperNurse_Zone2` を出す。
- Zone 2 には `BP_SecretRoomZone_2` の子アクタとして Chameleon（ポストプロセスの詰め合わせ）がある。有効なのは Glitch だけで、Telepathy とは関係ない。

---

## 6. 使っているアセットと、実在するファイル

「実在」は `P2/` の下で ls して確かめたもの。

| 種類 | アセット | ファイル（P2 の下） | 備考 |
|---|---|---|---|
| BP | `/Game/Blueprints/Main/Powers/BP_PrimalPower` | `_bytecode/.../Powers/BP_PrimalPower.txt`、`_assets/.../BP_PrimalPower.json` | |
| BP | `/Game/Blueprints/Main/Powers/Telepathy/BP_Telepathy`・`BP_TelepathyTracker` | 同じフォルダ | |
| ウィジェット | `/Game/Blueprints/Main/Powers/Telepathy/UMG_TelepathyTracker` | 同じフォルダ | |
| ウィジェット（タブレット） | `/Game/UI/Tablet/UMG_TabletPowers`、制御は `/Game/UI/BP_Powers` | `_bytecode/DDeception/Content/UI/...` | アイコンの `Percent` |
| メッシュ | `/Engine/BasicShapes/Sphere` | `_meshes_gltf/BasicShapes/Sphere.gltf`（+ `.bin`） | 半径 50 cm。UE5 にも同じエンジンのアセットがある |
| マテリアル | `/Game/Materials/05_Circus/M_05_Primal` | —（グラフは一部消失） | Translucent・両面 |
| マテリアル | `/Game/Blueprints/Main/Powers/Telepathy/MM_Telepathy`・`MM_Telepathy_Inst` | — （グラフは一部消失） | MD_UI・Additive |
| マテリアル（アイコン） | `/Game/Materials/MasterMaterials/MM_Powers`（Translucent。`DisabledPower`・`EnabledPower`・`Percent`）、`MM_Powers_PrimalFear`、`MM_Powers_Inst_Telepathy` | — （親のグラフは消失） | |
| テクスチャ | `/Game/Textures/05_Circus/T_05_PortalMaps` | `DDeception/Content/Textures/05_Circus/T_05_PortalMaps.png` | 2048²、線形 |
| テクスチャ | `/Game/ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16` | `DDeception/Content/ThirdParty/AdvancedMagicFX13/Textures/T_ky_noise16.png` | 1024²、線形 |
| テクスチャ | `/Game/ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise` | `DDeception/Content/ThirdParty/AdvancedMagicFX09/Textures/T_ky_noise.png` | 512²、sRGB |
| テクスチャ（アイコン） | `ring_altar_power_primal_icon`（`DisabledPower` 側）・`_inactive`（`EnabledPower` 側）、`ring_altar_power_telepathy_icon`・`_inactive`（ほかに `_hover`・`_selected`） | `DDeception/Content/UI/RingAltar_UI/Textures/ring_altar_power_{primal,telepathy}_icon*.png` | 105×104、BGRA |
| マテリアル関数（エンジン） | `RadialGradientExponential`、`ExponentialDensity` | —（UE5 に同名がある） | |
| 音 | `/Game/Audio/SharedGameplay/Stun_Wave_Attack_New_04` | `DDeception/Content/Audio/SharedGameplay/Stun_Wave_Attack_New_04.ogg` | |
| 音 | `/Game/Audio/SharedGameplay/Telepathy` | `DDeception/Content/Audio/SharedGameplay/Telepathy.ogg` | |
| 音 | `/Game/Audio/UI/power_refilled` | `DDeception/Content/Audio/UI/power_refilled.ogg` | |
| 音（エンジン） | `/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered` | `Engine/Content/VREditor/Sounds/UI/Teleport_Mode_Entered.ogg` | |
| 音（敵側） | `/Game/Audio/06_Hospital/Nurse/Nurse_Hospital_Zone01_Stunned`（Cue） | `DDeception/Content/Audio/06_Hospital/Nurse/Nurse_Hospital_Zone01_Stunned_01〜06.ogg` | |
| カメラシェイク | `/Game/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop`、`/Game/UI/Menu/Streaks/BP_CameraShake_Streak` | `_camera/_camera_shakes.json` | |
| DataTable | `PowersTable_Primal`・`PowersTable_Telepathy` | `_datatables.json` | |
| 未使用 | `/Game/Blueprints/Main/BP_Player_TelepathyVisualization`、`/Game/Materials/Player/M_TelepathyRange` | — | 生成されない |

パーティクル・glTF の独自メッシュ・CameraAnim は、どちらのパワーも使わない。

---

## 7. UE 5.8 で再現する際の注意

1. **Cascade は不要**。どちらもパーティクルを使わない。
2. **Primal の判定**: `UKismetSystemLibrary::SphereOverlapActors`（ObjectTypes は Pawn だけ）で、UE5 でも同じ関数が使える。遮蔽の判定を**足さない**こと（原作は壁越しに効く）。判定は発動時に 1 回、半径は `Range` 全体。
3. **ポストプロセス**: 無制限（Unbound）の `UPostProcessComponent` を 2 つ置き、`BlendWeight` をタイムラインで動かす。`ColorSaturation`・`ColorGain`・`ColorGainMidtones`・`SceneFringeIntensity` は UE5 の `FPostProcessSettings` にもある。ただし UE5 はトーンマッパーや作業色空間などの既定が違うので、同じ値でも見え方がずれうる。実機と比べて確かめること（**推測**）。`PostProcess1` の `ColorGamma` は値が既定なので省いてよい。`BlendWeight` は t=0 で 1.00075 になる（カーブが負に少し振れるため）。
4. **タイムライン**: `UTimelineComponent` に `UCurveFloat` を足すか、C++ で `FRichCurve` にキーを入れて評価する。キーは 2.4 節の値をそのまま写す。`RCTM_User`・`RCTM_Auto` の接線は保存された値を使い、UE5 に自動計算させないこと。
5. **カメラシェイク**: UE4 の `UCameraShake`（振動）は、UE5 では `ULegacyCameraShake`（GameplayCameras プラグイン）か、`UCameraShakeBase` と Wave Oscillator パターンの組み合わせになる。
   - 呼び出しは `ClientStartCameraShake` と `PlayerCameraManager->StartCameraShake`。
   - UE4 の FOscillator は、sin の引数に `DeltaTime × Frequency` を足す（Frequency は実質 rad/秒）。UE5 の Wave Oscillator の Frequency の単位（2π 倍の違いがありうる）は UE5.8 のソースで確かめて換算すること（**要確認**）。
   - Primal は Scale 25 なので、位置の振幅は 50/50/75 cm になる。
6. **Telepathy の壁越し表示**: カスタム深度やポストプロセスのマテリアルではなく、`UWidgetComponent(Space=Screen)` で作る。UE5 でも同じ仕組みがある（画面空間のウィジェットは常に最前面に描かれる）。DrawSize は既定の 500×500 のままにし、ルートの SizeBox（256）とのかね合いは実機で見た目を確かめる（**未確定**）。DPI の拡大率も効く。
7. **MM_Telepathy**: UE5 のマテリアルドメイン `User Interface` と `Additive` で作る。グラフは推測で組む（3.5 節）。`Tiling`・`Speed` はトラッカーごとにランダムに入れる（0.5〜1.5）。回転角（0.5〜360）もランダム。
8. **M_05_Primal**: Translucent・両面・パラメータ `Color`/`Opacity`/`Desaturation`。グラフは推測で組む。UE5 では半透明の影と照明（Lumen の半透明の扱いなど）を確かめ、原作と同じく影を落とさない設定にする（**推測**: 原作の半透明は影を落とさない）。
9. **音**:
   - Primal の `PlaySoundAtLocation` は位置が (0,0,0) で減衰設定も無いので、実質 2D で鳴る（**推測**）。UE5 では `PlaySound2D` に置き換えるのが安全。
   - `Teleport_Mode_Entered` はエンジンの VREditor の中身なので、UE5.8 のエンジンに同じパスがあるとは限らない。`P2` の OGG をプロジェクトに取り込むこと。
10. **敵の土台**: C++ の UINTERFACE で `SetState(EEnemyState, bool bByOrb)`・`NoTelepathy()`・`CustomDepth(float)` などを用意すると、後で作る敵がそのまま使える。
    - 列挙の並びは Patrol=0、Pursue=1、Stun=2、Teleport=3 で揃える。
    - 気絶の秒数（ナースは 17 秒）は敵の側に置く。
    - Primal は `Enemy` タグでも拾うので、タグも付けておく。
11. **Deferred Spawn**: Primal は −5000 cm に出してから BeginPlay で移る。C++ では最初からプレイヤーの位置に出してよい（見た目は同じ。**推測**）。`Range`・`Time` は「出す前に入れる」値として扱う。
12. **PrintText**（デバッグ表示）は原作の出荷版では出ないので、再現しない。
13. **リセットの非対称**: 死亡してやり直すとき、Primal は即座に回復するが、Telepathy は回復しない（原作どおり。ただし原作は直後にレベルを開き直す）。本作のやり直し方式によって扱いを決める必要がある（**要判断**）。
14. **使えないときの音**: 最新版は鳴らさない（旧版は `power_not_ready` を音量 0.35）。版の指定（Telepathy・Primal は `pak_reference_2`）に従えば鳴らさない。ただし、この処理は全パワー共通の土台の問題なので、テレポートの版（`pak_reference`）との兼ね合いは土台を作る人が判断する。

---

## 8. 推測・未確定の一覧

- **推測**: PostProcessComponent の `bUnbound`・Priority・BlendRadius は UE 4.24 の既定値（true・0・100）。
- **推測**: `M_05_Primal` のシェーディングモデルは DefaultLit（書き出しに無い）。グラフの中身（Emissive = Add_2 の中身、Opacity 入力の接続）。
- **推測**: `MM_Telepathy` の不透明度のグラフ（ノイズ 2 枚 × 放射グラデーション × 指数密度）。`MM_Telepathy_Inst` の `Size` は親の現在の式に無い（使われていない可能性）。
- **推測**: `Stun_Wave_Attack_New_04` は減衰設定が無いので空間化されない。
- **推測**: 画面空間のウィジェットはシーンキャプチャ（地図）に映らない。
- **推測**: 見張りナースは見つかる前は気絶処理が動かない（Gate の開く経路はバイトコードで確認済み。遊んだときの挙動は未観察）。
- **推測**: 距離が 12500 cm を超えると `Set Size` が負になり、トラッカーが反転する。
- **未確定**: トラッカーの実際の画面上の大きさ（DrawSize 500 と SizeBox 256 の関係、DPI）。実機での観察が要る。
- **未確定**: 旧版の Telepathy でカスタム深度を描いたポストプロセスの出所（見たのは旧版の 04_Sewer・02_elementaryschool・05_Circus_Zone1 の Chameleon の設定と、プレイヤーのアセットで、有効な Highlighter は見つからなかった）。最新版の実装には関係しない。
- **要確認**: UE5.8 のカメラシェイクの周波数の単位（7 節 5 項）。
