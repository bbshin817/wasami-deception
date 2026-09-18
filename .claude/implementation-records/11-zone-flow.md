---
title: ゾーンの進行（トリガー・区間の流れ・扉の破壊）
sources:
  - Source/wasami_deception/WasamiDoorBreak.h
  - Source/wasami_deception/WasamiDoorBreak.cpp
  - Source/wasami_deception/Tests/WasamiDoorBreakTests.cpp
  - Source/wasami_deception/WasamiTriggerBox.h
  - Source/wasami_deception/WasamiTriggerBox.cpp
  - Source/wasami_deception/WasamiZoneFlow.h
  - Source/wasami_deception/WasamiZoneFlow.cpp
  - Source/wasami_deception/WasamiZone1Flow.h
  - Source/wasami_deception/WasamiZone1Flow.cpp
  - Source/wasami_deception/WasamiZone2Flow.h
  - Source/wasami_deception/WasamiZone2Flow.cpp
  - Source/wasami_deception/Tests/WasamiZoneFlowTests.cpp
  - Source/wasami_deception/WasamiZoneShardChecker.h
  - Source/wasami_deception/WasamiZoneShardChecker.cpp
updated: 2026-09-18
---

# ゾーンの進行（トリガー・区間の流れ・扉の破壊）

## 役割
本家の病院の 2 つのレベル BP（`pak_reference_2` の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`）が受け持つ「区間の流れ」。本作のレベルにはレベル BP が無いので、ゲームモードが開始時にゾーンの流れのアクタ（`AWasamiZone1Flow`・`AWasamiZone2Flow`）を出し、それが開いたチェックポイントの区間を始め（本家の `Spawn`）、トリガーの箱・全回収・チェックポイントの保存・目的の文・タブレットの矢印の値で区間から区間へ進める。トリガーの箱は本家の `BP_TriggerBox_Base`（`AWasamiTriggerBox`）。扉を F の連打で破る鍵は本家の `BP_06_Hospital_DoorBreak`（`AWasamiDoorBreak`。画面は 09 記録の `UWasamiSwitchboxWidget`）。作業一覧の項目 6（ゾーンの進行）のステップ 1 で骨組みを作った。

**流れを進め、レベル BP が自分で流すもの（レベルのシーケンスと揺れ）を流し、扉の破壊を起こして聞き、名指しする両開き扉（08 記録の `AWasamiDoubleDoors`）を閉ざし・開け、ゾーンの障壁（08 記録の `AWasamiZoneBarrier`）を壊す**。場面・読み込み画面・声（Bierce・ナースの放送）・曲・ナースは、それを作る項目・ステップが埋める。各イベントの中の、本家でそれをする位置にコメントで印を付けてある（`Not yet:` は項目 6 の残りのステップ、`(item N)` は作業一覧の別の項目）。

## 公開インターフェース
- `AWasamiTriggerBox`（`AActor`）… 本家の `BP_TriggerBox_Base`。
  - `OnTrigger`（動的マルチキャスト。本家の `Trigger`）、`bEndOverlap`（本家の `End Overlap`。病院で真にしているものは無い）、`GetBox()`、`HasFired()`。
  - `NotifyPlayerOverlap(bool bBegin)` … プレイヤーが入った / 出た。重なりの開始・終了はプレイヤー（`GetPlayerCharacter(0)`）のときだけこれを呼ぶ。デバッグのコマンドとテストは直接呼ぶ。
- `AWasamiDoorBreak`（`AActor`）… 本家の `BP_06_Hospital_DoorBreak`。
  - `FinishedEvent`（動的マルチキャスト。本家の `Finished Event`）、`ProgressSpeed`（本家の `Progress Speed`。クラスの既定 5、レベルのアクタが Zone 1 は 1.5・Zone 2 は 3.0 を持つ）。
  - `EnableSwitch()`（箱が重なりを出すようになる）、`Disable()`（鍵の画面と箱を消す。病院では呼ぶ所が無い）、`Interact()`（F。範囲の中なら鍵の `InteractEvent` とカチッという音）。
  - `NotifyPlayerOverlap(bool bBegin)` … プレイヤーが箱に入った / 出た。箱の重なりはプレイヤーのときだけこれを呼ぶ。テストは直接呼ぶ。
  - `RemoveWidgetDelay` 1（鍵が外れてから画面を消すまで）、`IsInRange()`・`GetBox()`・`GetWidget()`・`GetUIWidget()`。
- `AWasamiZoneFlow`（`AActor`、Abstract）… 2 つのゾーンの共通の土台。
  - 静的関数: `SpawnFor(Mode, Zone)`（1 → `AWasamiZone1Flow`、2 → `AWasamiZone2Flow`、ほかは null。`SpawnActorDeferred` で `Mode` を入れてから `FinishSpawning`）、`SourceTag(Name)`（`src:<Name>`）、`FindSource(World, Name)`（そのタグを持つ最初のアクタ）、`RemoveAllEnemies(World)`（本家の `BP_DD_Functions` の同名の関数: タグ `Enemy` のアクタをすべて破棄）、`DestroyAllShards(World)`（残りの `AWasamiShard` を回収せずに破棄）。
  - `CallEvent(FunctionName)` … 引数の無い UFUNCTION のイベントを名前で呼ぶ（デバッグの `Wasami.Flow`・テスト）。無ければ偽。
  - `GetSection()` … 最後に来たイベントの本家の名前（`04_Start`・`05_Persistent`・`05 All Shards Collected`・`Miniboss Transition `〈後ろに空白〉など）。
  - 矢印の値: `IsArrowOnShards()`（本家の `Shards?`。既定は真 = `BP_ArrowPointer` の CDO）、`GetArrowColor()`（`Change Color` の色。`TOptional`、与えられるまで未設定）、`GetArrowTarget()`（`Target`）。地図の矢印 `AWasamiArrowPointer`（03 記録）が `Find Object` のたびにこれを取る（本家はレベル BP が矢印に入れる）。
  - `GetMode()`。
  - `FadeSequence`（`EditDefaultsOnly`、`TSoftObjectPtr<ULevelSequence>`、既定 `/Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade`: 2 s まで黒、5 s で晴れる。01 記録の「シーケンス」）。
  - 派生が使う保護の関数: `StartAt(Checkpoint)`（仮想。本家の `Spawn`）、`Enter(Name)`、`BindTrigger(Source, Function)`（本家の `BindDelegate` + `AddMulticastDelegate`: `FScriptDelegate::BindUFunction` を箱の `OnTrigger` に `AddUnique`。箱が無ければ警告）、`EnableDoorBreak(Source, Function)`（名前の扉の破壊の `EnableSwitch` と、その `FinishedEvent` に流れのイベントを `AddUnique`。本家は両ゾーンともこの 2 つを続けて呼ぶ。無ければ警告）、`DoubleDoors(Source)`（名前の両開き扉。無ければ null と警告）、`ZoneBarrier(Source)`（名前のゾーンの障壁。無ければ null と警告）、`BindAllShardsCollected(Function)`（ゲームモードの `OnAllShardsCollected` に同じく）、`SetObjective`、`SaveCheckpoint`（ゲームモードの `SaveCheckpoint`）、`SetArrowShards`・`SetArrowColor`・`SetArrowTarget`、`PlaySequence(Source)`（本家の `GetSequencePlayer` → `Play`: `src:<名前>` の `ALevelSequenceActor` のプレイヤーを `Play`。無ければ警告）、`PlayCameraShake(Shake, Scale = 1)`（本家の `ClientPlayCameraShake(揺れ, Scale, 0 = CameraLocal, 回転 0)`: プレイヤーのコントローラーの `ClientStartCameraShake`。揺れは `TSoftClassPtr` で、ここで読み込む）、`PlayWorldCameraShake(Shake, Epicenter, Inner, Outer, Falloff, bOrient)`（本家の同名: `UGameplayStatics::PlayWorldCameraShake`）、`PlayFadeOut(PlayRate)`（本家の `Basic DD Fade Out`: `FadeSequence` の `ULevelSequencePlayer::CreateLevelSequencePlayer`〈再生の設定は既定 = 本家の値〉→ `SetPlayRate` → `Play`）、`PlaySoundAt(Sound, Location, Attenuation)`（本家の `PlaySoundAtLocation(音, 位置, 回転 0, 1, 1, 0, 減衰)`）、`ActivateEmitter(Source)`（名前の `AEmitter` の `ParticleSystemComponent` の `Activate(true)`。無ければ警告）、`SetVolumeCollision(Source, Enabled)`（名前のボリュームのブラシの `SetCollisionEnabled`）、`TeleportPlayerTo(PlayerStartTag)`（本家の `K2_TeleportTo(PlayerStart の位置, 回転 0)` と `SetControlRotation(PlayerStart の回転)`）、`After(Seconds, Then)`（本家の `Delay`。弱い参照のラムダをタイマーで。0 秒は次のティック）、`Source(Name)`。
- `AWasamiZone1Flow` … 揺れ（`EditDefaultsOnly`、`/Game/DD` の本家の揺れの BP。01 記録の「シーケンス」）: `ElevatorShakeClass`（`01_Hotel_Lobby_ElevatorShake`）・`ElevatorShakeStopClass`（`01_Hotel_Lobby_ElevatorShakeStop`）・`DoorsBustedShakeClass`（`BP_07_CameraShake_Jump`）・`TakeOffShakeClass`（`06_CameraShake_Zone1_AmbulanceTakeOff`）。音: `DoorsBustedSound`（`/Game/DD/Audio/06_Hospital/DD_TT_Door_BustedOpen_02`）・`DoorsBustedAttenuation`（`01_Lobby_Attenuation`）・`PortalSound`（`/Game/DD/Audio/00_Ballroom/21-Ballroom_portal_V2`）。定数 `ArrivalShakeSeconds` 7、`ShardCheckDelay` 1、`TransitionFadeRate` 2、`DoorsBreakSeconds` 25、`DoorsBustedShakeRadius` 3000、`DoorsGoneDelay` 0.1、`TakeOffDelay` 1、`LoadingDelay` 7、`OpenZone2Delay` 2.5、`TakeOffShakeScale` 4。イベント（UFUNCTION）: `On04Intercom`・`On04DoorBreak`・`On05Transition`・`On05AllShardsCollected`・`On05ParkingLotCutscene`・`On06TunnelEnter`・`On06DoorsLock`・`On06ReachAmbulance`。
- `AWasamiZone2Flow` … 揺れ `DoorPickedShakeClass`（`BP_04_BossFight_CameraShake_Initial`）、音 `SpikesSound`（`/Game/DD/Audio/06_Hospital/DD_Needle_Trap_R1_V3`）。定数 `SpikesDeathDelay` 0.5、飛ばした場面が残す位置 `AmbulanceArrived`・`FalseCeilingOpen`・`WallSwitchThrown`。イベント: `OnCellCutsceneFinished`・`OnCellDoorBreak`・`OnSpikesDeath`・`OnMinibossBierceTalk`・`OnMinibossTriggerTransition`・`OnMinibossBehindMatron`・`OnMazeTriggerStart`・`OnMazeAllShards`。
- `AWasamiZoneShardChecker`（`AActor`）… 本家の `BP_ZoneShardChecker`（`Blueprints/Shared`）。ゾーンを覆う箱で、地図の矢印の区域（矢印はプレイヤーが重なる最初のチェッカーの箱の中のシャードを指す。03 記録）。
  - 部品: `DefaultSceneRoot`（置いたアクタの拡縮が箱の大きさ）と `Box`（`UBoxComponent` の既定のまま: 32 cm、`OverlapAllDynamic`。本家の値も既定）。`GetBox()`。
  - `CheckShards()`（UFUNCTION。本家の `Check Shards`）… `BeginPlay` でゲームモードの `OnCollectShard` に結ぶ。1 s 後（`CheckShardsDelay`。本家の `Delay`: 待っている間の呼び出しは待ち直さない）、箱に重なる `AWasamiShard` が 0 なら `K2_DestroyActor`。消えた後は矢印が `Shards?` の間隠れる。
  - `GetShards(TArray<AActor*>&)` … 箱に重なるシャード。
  - 写さないもの: 開発用の `PrintText`（`COLLECTED ALL SHARDS IN THIS ZONE`）、誰も結ばない `Collected`、ほかの章のレベル BP だけが呼ぶ `Remove All In Zone`・`Collect All In Zone` とその `ZoneShards`（`BeginPlay` の箱の中のシャード）。
  - 組み立ての `_flow` が両ゾーンの `BP_ZoneShardChecker_2` を置く（01 記録）: Zone 1 は (130, −8440, 0)・ヨー 180（本家の拡縮 (−275.94, −275.94, 22) を前処理が正の拡縮と 180° に直したもの）・拡縮 (275.94, 275.94, 22) で箱は ±8830 × ±8830 × ±704 cm。**Zone 1 のシャード 337 のうち、障壁の手前の廊下（y −17398〜−18602）の 3 つは箱の外**（本家も同じ。その 3 つには矢印が向かず、ほかの 334 を集めると箱が消えて矢印が隠れる）。Zone 2 は (7443, 87, 689)・拡縮 (401.33, 210.72, 52.30) で、シャード 342 がすべて中。
- デバッグのコンソールコマンド（PIE では `python Tools/pie.py cmd "…"`）: `Wasami.Flow <関数名>`（`WasamiZoneFlow.cpp`。ゾーンの流れのイベントを呼ぶ。例 `Wasami.Flow On04DoorBreak`）。`Wasami.Trigger <本家の名前>`・`Wasami.CollectShards [N]` はゲームモード（06 記録）、`Wasami.Interact [N]`（F を N 回）もゲームモード（02 記録）。

## 内部構造と処理の流れ

### トリガーの箱（`AWasamiTriggerBox`）
- 部品: ルートの `Box`（`UBoxComponent`、既定の半径 32 cm・`bHiddenInGame`・QueryOnly）。当たりは本家の `Box_GEN_VARIABLE` の Custom: 種類 WorldStatic、WorldStatic・WorldDynamic・Visibility・Camera・PhysicsBody・Vehicle・Destructible を無視、Pawn を Overlap。プロジェクトの Teleport チャンネルは本家の一覧に無いので既定の Overlap のまま（テレポートの照準のトレースを遮らない）。本家の空の `Cube`（メッシュの無い StaticMeshComponent）と開発用の `Debug` の PrintString は写さない。レベルの組み立てがアクタの拡縮で大きさを決める（01 記録）。
- `NotifyPlayerOverlap(bBegin)`: 入る・出るの 2 つの DoOnce（`bBeginClosed`・`bEndClosed`）。通ったほうの DoOnce は**発火するかどうかに関係なく閉じる**（本家 @244・@369）。`bBegin != bEndOverlap` のときだけ `bFired` を真にして `OnTrigger` を流す。**結ばれる前に通った箱はそれで使い切られる**（本家も同じ。本家は扉の奥などに置いて先に通れないようにしている）。

### 扉の破壊（`AWasamiDoorBreak`）
- 部品（本家の SCS）: ルートの `DefaultSceneRoot`、`Widget`（`UWidgetComponent`: 画面空間・`DrawSize` 64 × 64・隠れた状態・クラス `UWasamiSwitchboxWidget`。拡縮 0.5 は画面空間では効かない。`WindowVisibility` Visible は UE 4.24 の既定で、ワールド空間の仮想窓だけが使う）、`Box1`（`UBoxComponent`: 既定の OverlapAllDynamic・`bHiddenInGame`、半径 (100, 150, 100)・拡縮 0.5 = 50 × 75 × 50 cm、`bGenerateOverlapEvents` 偽）。`Box1` の `AreaClass`（NavArea_Obstacle）は写さない（何も遮らない箱はナビゲーションに入らない）。
- `BeginPlay`（本家 @632 → @375）: 部品の `BeginPlay` が作った画面を `UIWidget` に取り、その `Finished` を `Finished` に結び、`ProgressSpeed` を渡す。音 4 つをここで読む（ソフト参照）。
- 入力: 本家のクラスは `AutoReceiveInput` Player0・優先度 5 で `Interact`（F、`IE_Pressed`、入力を消費しない）を自分で受ける。本作ではプレイヤーが F を受けて `OnInteract` を流し（02 記録）、`BeginPlay` でそれを聞く（プレイヤーは `BeginPlay` の前に置かれる）。`EndPlay` で外す。
- 箱の重なりの開始・終了（本家 @49・@169）: 相手が `GetPlayerCharacter(0)` のときだけ `NotifyPlayerOverlap`。入ると `bInRange` 真・画面を見せる、出ると `bInRange` 偽・画面を隠す・画面の `Progress` を 0・`SetProgress(0)`（鍵が空に戻る）。
- `Interact`（本家 @489）: `bInRange` のときだけ、画面の `InteractEvent` → `PlaySoundAtLocation(SFX_06_Lockpicking, アクタの位置, 回転 0, 1, 1, 0, 01_Lobby_Attenuation)`。
- `Finished`（本家 @763。画面の `Finished` から）: `PlaySound2D(Press_Slam_02, 0.5, 2)`・`PlaySound2D(SFX_06_Lockpicked, 2, 2)` → `Box1` を消す → `bInRange` 偽 → 画面を見せる → 1 s の Delay を置いてから `FinishedEvent`（本家は Delay の後の流れを先に積むので、`Finished Event` は同じフレームに流れる）→ 1 s 後に `Widget` を消す。**箱を消すと重なりの終わりが流れ、鍵は空に戻って隠れ、すぐ見せ直される**（UE の `OnComponentDestroyed` → `ClearComponentOverlaps`。本家も同じ呼び順）。画面はその後に `Completed`（火花と消え方）を流す。
- `EnableSwitch`（本家 @1039）: `Box1->SetGenerateOverlapEvents(true)`。既に箱の中に立っているプレイヤーは、動くまで入ったことにならない（UE の重なりの更新は動いたときだけ）。
- `Disable`（本家 @1078）: `Widget` と `Box1` を消す。
- レベルの組み立て（`dd_level._flow`、01 記録）が本家の位置に `src:<本家の名前>` で置き、`Progress Speed` を書く（両ゾーンの `BP_06_Hospital_DoorBreak_2`。Zone 1 は (0, 915, 125)・1.5 = 67 回、Zone 2 は (−14145, 1210, 155)・3.0 = 34 回）。

### 流れの生成と開始（`AWasamiZoneFlow`）
- ゲームモードの `BeginPlay`（06 記録）が、Zone 2 をチェックポイント 0 で開いたとき（Zone 1 を開き直す）以外、`SpawnFor(this, ZoneOf(レベル名))` で出す。流れの `BeginPlay` が `StartAt(Mode->GetStartCheckpoint())`（本家の `Setup` → `Important Casts` → `Spawn`。`Spawn` の頭の `SetViewTargetWithBlend`・`EnableInput` は、開いたばかりのレベルでは既定のまま）。
- 本家の名前で置かれたアクタを `src:<名前>` のタグで探す。タグはレベルの組み立てが付ける（メッシュは `src:<アクタ名>`、トリガーの箱とボリュームも同じ。01 記録）。

### Zone 1（`AWasamiZone1Flow`、本家 `06_Hospital_Zone_01`）
- `StartAt`（本家 `Spawn` @13483、`Load Progress By Level(7, 5)`）: 4 → `Start04`、5 → `Persistent05`、6 → `Start06`、ほかは何もしない（7 以上は Zone 2 のもの。0 はゲームモードが 4 にする）。チェックポイントの PlayerStart へ移すのはゲームモード（06 記録）。
- `Start04`（`04_Start`）: `BP_06_DoubleDoors11`（エレベーターの前の扉）の `Lock`（本家 @14507。シーケンスより先）、シーケンス `06_Hospital_Zone01_ElevatorArrive`（扉 2 枚が 11.3〜13.5 s に開く・音。14.1 s）と揺れ `01_Hotel_Lobby_ElevatorShake`（7.3 s、入り 2 s）→ 7 s 後に揺れ `01_Hotel_Lobby_ElevatorShakeStop`（0.5 s の強い揺れで終わる。本家は前の揺れを止めず重ねるだけ）と `04_Intercom` → `On04Intercom`、`EnableDoorBreak("BP_06_Hospital_DoorBreak_2", On04DoorBreak)`（本家 @294〜@353）。
- `On04Intercom`: 声だけ（`Nurse_Hospital_Zone01_Event_37_Intercom` 0.6 → 13 s → Bierce。項目 20）。
- `On04DoorBreak`（`04_DoorBreak`。扉の破壊の `FinishedEvent` から）: `DoubleDoors11` の `bLocked` を偽に直に書いてから `Open Front`（本家 @16506。`Unlock` を使わないのは、`Unlock` は前の箱に最後に入った者がいるときだけ開けるため）→ `BP_04_Trigger_Maze` → `On05Transition`。1 s 後の Bierce（項目 20）。
- `On05Transition`（`05_Transition`）: `SaveCheckpoint(5)` → `Persistent05`。
- `Persistent05`（`05_Persistent`）: 目的 `COLLECT ALL SHARDS`、全回収 → `On05AllShardsCollected`、1 s 後にゲームモードの `CheckShards`（開き直したときにシャードが残っていなければ、これで全回収に進む）。`Spawn Nurses`（項目 7）・曲の `bFadeOut` 偽（項目 19）。
- `On05AllShardsCollected`（`05 All Shards Collected`）: `RemoveAllEnemies`、矢印 `Shards?` 偽・色 (1, 0.8002, 0, 1)・的 `06_CutsceneStart`、目的 `REACH THE PARKING LOT`、`06_CutsceneStart` → `On05ParkingLotCutscene`、`BP_ZoneBarrier_2`（駐車場への出入口の障壁）の `DestroyBarrier`（本家 @16409）。曲（項目 19）。本家のレベル BP が障壁を呼ぶのはここだけなので、6 で開き直しても障壁は残る（プレイヤーはその先から始まる）。
- `On05ParkingLotCutscene`（`05_ParkingLotCutscene`）: 場面 `06_Hospital_Zone1_06Event`（項目 25）を飛ばし、その終わりの `06_Transition` = `Transition06` をすぐ呼ぶ。
- `Transition06`（`06 Transition`）: `DestroyAllShards`、`PlayFadeOut(2)`（本家の `Basic DD Fade Out(2)`: 1 s 黒、1.5 s で晴れる。その前の `SetViewTargetWithBlend(プレイヤー)` は、場面を飛ばすので視点がプレイヤーから離れず要らない）、`TeleportPlayerTo("06_Start")`、`Start06`。本家の `Spawn` の 6 は `Start06` と同じ所から（フェードは無い）。
- `Start06`（`06_Start`。本家は `Spawn` の 6 と `06 Transition` の後半が同じ所）: `06_DoorsLock` → `On06DoorsLock`、`TriggerBox_06_AmbulanceTop` → `On06ReachAmbulance`、矢印 偽・(1, 0.8317, 0, 1)・的 `06_TunnelEnter`、目的 `REACH THE TUNNEL`、`06_TunnelEnter` → `On06TunnelEnter`。`Spawn Nurses_06`（項目 7）。
- `On06TunnelEnter`: 矢印 偽・(1, 0.8317, 0, 1)・的 `TriggerBox_06_AmbulanceTop`、目的 `GET ON TOP OF THE AMBULANCE`。
- `On06DoorsLock`: `BP_06_DoubleDoors33_36`（トンネルの手前の扉）の `Lock` → `Force Close`（本家 @17544・@17443）、`BlockingVolume_1` の当たりを QueryAndPhysics → 25 s → `BreakDoorsIn`（本家 @1053〜@1016）: 扉の位置で `DD_TT_Door_BustedOpen_02`（`01_Lobby_Attenuation`）と `PlayWorldCameraShake(BP_07_CameraShake_Jump, 扉の位置, 0, 3000, 1, 真)`、`ActivateEmitter("Fracture_concrete_5")`（レベルのエミッタ。扉の 232 cm 手前で拡縮 4、ふだんは眠っている。01 記録）、`BlockingVolume_1` を NoCollision → 0.1 s 後に扉を `Destroy`。ナースの `bAttackDoor`（項目 7）。
- `On06ReachAmbulance`: `SaveCheckpoint(7)`、`RemoveAllEnemies`、矢印 偽・(1, 0.8317, 0, 1)・的 `Plane48_2`、目的 `GOOD LUCK`、`BlockingVolume_Ambulance_4`・`_2`・`_1`・`_3` の当たりを QueryAndPhysics → 1 s → `PlaySequence("06_Hospital_Zone1_AmbulanceTakeOff")`（救急車が付いたボリュームとプレイヤーごと走る）と揺れ `06_CameraShake_Zone1_AmbulanceTakeOff`（拡縮 4）→ 7 s → 読み込み画面 `UWasamiLoadingWidget::Show(AsylumLevel)`（Level 7、Z 5。その Construct がゲームインスタンスの回収の記憶を空にする。09 記録）・`PlaySound2D(21-Ballroom_portal_V2)`・`RemoveAllEnemies` → 2.5 s → `OpenLevel(L_Hospital_Zone2)`（本家 @796〜@5）。曲（項目 19）。

### Zone 2（`AWasamiZone2Flow`、本家 `06_Hospital_Zone_02`）
- `StartAt`（本家 `Spawn` @22328、`Load Progress By Level(7, 8)`）: 7 → `SkippedScenesEnd` → `OnCellCutsceneFinished`（本家は `Arrive Event` → 捕まる場面 → 独房の場面。項目 25 まで飛ばす。プレイヤーを独房の `PlayerStart_Cell` に出すのはゲームモード: 本家の `Cell Cutscene Start` がそこへ移す）、8 → `Miniboss Start ` → `MinibossTransition`、9 → `Maze Start` → `MazeTransition`、10 → `Postmaze Start` → `PostmazeTransition`（本家の `… Start` の PlayerStart への移動はゲームモード）。
- `SkippedScenesEnd`（7 だけ。本家も 8〜10 ではこれらの場面を流さないので、救急車はトンネルに残る）: 飛ばした場面のうち、状態を残す区間（`KeepState`。ほかの区間はシーケンスの既定 RestoreState で終わると戻る）の最後のキーを入れる。到着 `06_Hospital_Zone2_AmbulanceArrive1`（本家は配布版だけ流す）の救急車 `hospital_ambulance_new_arrive` を (−14157.71484375, −5025.021484375, 800) へ（回転はそのまま。付いた `Ambulance_Arrive_Blockers*` も動く）、その終わりの `Escape_AmbulanceArrive` が消す `Ambulance_Arrive_Blockers4`、独房の場面 `06_Hospital_Zone2_Cell` の偽の天井 `hospital_zone_02_holdingCell_01_false_ceiling_11` を (−0.037109375, 864.614990234375, 0)、壁のスイッチ `…_wall_switch_14` をロール 40.809776306152344。Static のものは Sequencer と同じく Movable にしてから動かす。カメラ・フェード（最後は 0）・ナースの絵・救急車の灯（置いた値も終わりも 0）は写さない。
- `OnCellCutsceneFinished`: `PlaySequence("06_Hospital_Zone2_Spikes")`（棘が下りてくる。70 s）、`EnableDoorBreak("BP_06_Hospital_DoorBreak_2", OnCellDoorBreak)`、`Trigger_Cell_Spikes` → `OnSpikesDeath`、`BP_MiniBoss_Trigger` → `OnMinibossTriggerTransition`、1 s 後に `Miniboss_BierceTalk` → `OnMinibossBierceTalk`（本家 @23769〜@3057）。視点を 2 s でプレイヤーに戻す `SetViewTargetWithBlend` は、場面を飛ばしたので要らない。曲（項目 19）・Bierce（項目 20）。**棘の箱（`Trigger_Cell_Spikes`。棘に付いて下りる）は開いてから約 19.2 s で独房の床に立つプレイヤーの頭に届く**（箱の下端 = 518 + 棘の Z、棘は 0 s に −150 から毎秒約 9.91 cm）ので、その間に鍵（Zone 2 の 3.0 で 34 回）を外して出る。
- `OnCellDoorBreak`（`Cell_DoorBreak`、本家 @25292）: `PlaySequence("06_Hospital_Zone2_Cell_DoorPicked")`（独房の扉が開く。4.27 s）と揺れ `BP_04_BossFight_CameraShake_Initial`（拡縮 1。本家はプレイヤーのカメラマネージャーの `PlayCameraShake`、同じこと）。
- `OnSpikesDeath`（`Spikes_Death`、本家 @6810）: `AWasamiHitFX`（08 記録の打たれた閃き。本家の `BP_HitFX` を原点に）と `PlaySound2D(DD_Needle_Trap_R1_V3)` → 0.5 s 後にゲームモードの `DeathEvent(プレイヤー)`。
- `OnMinibossTriggerTransition`: `MinibossTransition` → `SaveCheckpoint(8)`。
- `MinibossTransition`（`Miniboss Transition `）: 矢印 偽・(0, 0, 0, 0)・的なし、目的 `Get past the nurses `（本家どおり後ろに空白）、`Trigger_MazeStart` → `OnMazeTriggerStart`、`Trigger_Miniboss_BehindMatron` → `OnMinibossBehindMatron`（放送の声だけ。項目 20）。実績（`06_NurseAlert`）は本作に無い。`Activate MiniBoss Enemies`（項目 11）。
- `OnMazeTriggerStart`: `MazeTransition` → `SaveCheckpoint(9)`（1 s 後の Bierce とリフトの一言は項目 20）。
- `MazeTransition`（`Maze Transition `）: `RemoveAllEnemies`、矢印 `Shards?` 真、目的 `COLLECT ALL SHARDS`、全回収 → `OnMazeAllShards`。`Spawn Nurses`（項目 7）。本家の Zone 2 には 1 s 後の `Check Shards` が無いので、9 で開き直してシャードが残っていなければ全回収には進まない（本家も同じ）。
- `OnMazeAllShards`: `SaveCheckpoint(10)` → `PostmazeTransition`（2 s 後の Bierce は項目 20）。
- `PostmazeTransition`: `RemoveAllEnemies`、`DestroyAllShards`、`src:ring_statue_orb_5` のアクタをすべて破棄、次のティック（本家の `Delay 0`）に矢印 偽・(1, 0.8941, 0, 1)・的 `BP_08_RingPiece_NoPickup_5`（まだ置いていないので null）、目的 `COLLECT THE RING PIECE`。`BP_Collectable` ID 3 を `collec` に出す（項目 12）、`ring_statue_2` の `Interact All Shards` → `Collected Ring Piece`（項目 13）。

## 作るアセット
独房の棘の音 `/Game/DD/Audio/06_Hospital/DD_Needle_Trap_R1_V3` と、棘と独房の扉のシーケンスが起こすエミッタの粒子（`P_06_NurseSparks`・`Fracture_dark_slow`・`Concrete_impact_large`）は `dd_gimmicks.import_cell`（08 記録）。ほかはなし（流れのアクタはゲームモードが実行時に出す。トリガーの箱・ボリューム・扉の破壊はレベルの組み立てが置く。01 記録。扉の破壊の画面と音の素材は `dd_ui.import_door_break`。09 記録）。

## 原作データの根拠
- `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`（`python Tools/dd/bp_flow.py <file> <イベント名>` で読む。番地は上に書いた入口）、`Blueprints/Main/BP_TriggerBox_Base.txt`（@244・@369）と `_assets/…/BP_TriggerBox_Base.json`（`Box_GEN_VARIABLE` の当たり、SCS の `Cube` はテンプレート無し）、`Blueprints/Macros/BP_DD_Functions.txt` の `Remove All Enemies`・`Basic DD Fade Out`、`UI/Main/UMG_Loading.txt` の `Construct`（`Shards To Be Removed` を空に）、`_assets/…/BP_ArrowPointer.json`（`Shards?` 既定 真）、`Blueprints/06_Hospital/BP_06_Hospital_DoorBreak.txt`（上の番地）と `_assets/…/BP_06_Hospital_DoorBreak.json`（`Box1_GEN_VARIABLE`・`Widget_GEN_VARIABLE`・`Progress Speed` 5・`AutoReceiveInput`・`InputActionDelegateBindings` の `Interact`）、`_raw/DDeception/Config/DefaultInput.ini`（`Interact` = F）。
- Zone 2 の 7 の始まり: `06_Hospital_Zone_02.txt` の `Arrive Event`・`Arrive_CaptureCutscene`・`Cell Cutscene Start`・`Escape_AmbulanceArrive`、`pak_reference_2/_sequences/06_Hospital_Zone2_AmbulanceArrive1.json`・`_Capture.json`・`_Cell.json`（最後のキー）と `_assets/…/Animation/06_Hospital/` の同名（区間の `EvalOptions.CompletionMode` が `KeepState` のもの。無いものはシーケンスの既定 = `BaseEngine.ini` の RestoreState）、`_levels/06_Hospital_Zone_02.full.json`（`Trigger_Cell_Spikes` の箱は `spikes` に付き (−14170, 1610, 550)・拡縮 (19.11, 15.40, 1)、`Ambulance_Arrive_Blockers4` は救急車に付く）。
- シャードチェッカー: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Shared/BP_ZoneShardChecker.txt`（`ReceiveBeginPlay` @1794、`Check Shards` @1955〜@337、`Remove All In Zone`・`Collect All In Zone` は病院のレベル BP から呼ばれない〈`grep` で確かめた〉）と `_assets/…/BP_ZoneShardChecker.json`（`Box_GEN_VARIABLE` は `AreaClass` だけ）。
- レベルのトリガーとボリュームの位置・拡縮・当たり: `pak_reference_2/_levels/06_Hospital_Zone_0{1,2}.full.json`（前処理を通して `stage_ue.json` の `actors`。01 記録）。

## 依存関係
- 扉の破壊: 画面 `UWasamiSwitchboxWidget`（09 記録）、プレイヤーの `OnInteract`（02 記録）、素材 `/Game/DD/Audio/06_Hospital/Lockpicking/SFX_06_Lockpicking`（SoundCue）・`SFX_06_Lockpicked`・`/Game/DD/Audio/07_FunPlace/Press_Slam_02`・`/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation`（`dd_ui`。01・09 記録）。
- 自前: `AWasamiGameMode`（`GetStartCheckpoint`・`SaveCheckpoint`・`CheckShards`・`OnAllShardsCollected`・`CurrentObjective`・`DeathEvent`・`Zone2LevelName`。06・02 記録）、`UWasamiGameInstance::ForgetCollectedShards`、`AWasamiShard`、レベルの組み立て（`dd_level._flow`。01 記録）。
- レベルシーケンス（2026-09-18、項目 6 のステップ 2）: 流れが再生するシーケンスは、本家どおりレベルに置いた `LevelSequenceActor`（タグ `src:06_Hospital_Zone01_ElevatorArrive`・`src:06_Hospital_Zone1_AmbulanceTakeOff`・`src:06_Hospital_Zone2_Spikes`・`src:06_Hospital_Zone2_Cell_DoorPicked`）にある（`dd_sequence`。01 記録）。フェードの `/Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade` はアクタを置かない（`Basic DD Fade Out` が実行時にプレイヤーを作る）。流れは `PlaySequence` でこれを再生する（Zone 1 の 04 と救急車。Zone 2 の 2 つはステップ 7）。流れの揺れも同じ組み立てが作る（`dd_sequence.CAMERA_SHAKES`）。
- 両開き扉: `AWasamiDoubleDoors`（08 記録）。組み立ての `_flow` が Zone 1 の 2 枚を `src:` タグで置く（01 記録）。
- ゾーンの障壁: `AWasamiZoneBarrier`（08 記録）。組み立ての `_flow` が両ゾーンの `BP_ZoneBarrier_2` を置く（01 記録）。Zone 2 のものを壊すのは項目 13（欠片の画面が閉じたときの `Ring Piece Collect `、本家 @21485）。
- 読み込み画面: `UWasamiLoadingWidget`（09 記録）と音 `21-Ballroom_portal_V2`（`dd_ui.import_loading`）。
- 使う側: ゲームモード（生成）、プレイヤーのタブレットの帯（`CurrentObjective`。02・03 記録）、地図の矢印（`IsArrowOnShards`・`GetArrowTarget`・`GetArrowColor`、シャードチェッカー。03 記録）。
- 扉が破られるとき: 音 `DD_TT_Door_BustedOpen_02`・揺れ `BP_07_CameraShake_Jump`・レベルのエミッタ `Fracture_concrete_5`（粒子 `/Game/DD/ThirdParty/BallisticsVFX/Particles/Destruction/Fractures/V2/Fracture_concrete_3`、材質は推定。`dd_gimmicks`・`dd_level._flow`。01・08 記録）。
- エンジン: `UBoxComponent`、`UBrushComponent`（`AVolume`）、`FScriptDelegate::BindUFunction`、`FTimerManager`、`UGameplayStatics::OpenLevel`・`PlayWorldCameraShake`・`PlaySoundAtLocation`、`ALevelSequenceActor`・`ULevelSequencePlayer`（モジュール `LevelSequence`・`MovieScene`）、`APlayerController::ClientStartCameraShake`、`AEmitter`・`UParticleSystemComponent`。

## 既知の制約・注意点
- **結ぶ前に通ったトリガーの箱は使い切られる**（上）。04 はエレベーターの前の扉（`DoubleDoors11`）が鍵を破るまで、05 から先は駐車場への出入口の障壁（`BP_ZoneBarrier_2`）が全回収まで道をふさぐので、ふつうに遊べば先の箱には届かない。デバッグで区間を飛ばしてから歩くと箱を使い切ることがあるので、確かめるときは結ばれてから通すか、`Wasami.Flow`・`Wasami.Trigger` で進める。
- 扉の破壊はプレイヤーの `OnInteract` を `BeginPlay` で聞くので、後から出し直したポーンの F は届かない（いまは死ぬとレベルを開き直すので起きない）。テストのワールドにはプレイヤーがいないので、テストは `Interact` を直接呼ぶ。
- Zone 1 で 7 を保存してから Zone 2 が開くまでの 10.5 s に死ぬと、Zone 1 が 7 で開き直され、`StartAt` は何もせず UE の既定の PlayerStart から始まる（本家も `Spawn` に 7 の枝が無い）。
- Zone 2 の 7 は、飛ばした場面の代わりに開いてすぐ棘が下り始める。本家は開き直すたびに捕まる場面と独房の場面（約 100 s）を流してから棘を始めるので、鍵を外せる時間（約 19 s）は同じだが、開いたときの黒のフェード（1〜2.5 s）の間も棘は進む。
- テストのワールドでタイマーを進めるには注意が 2 つある（`Tests/WasamiZoneFlowTests.cpp` の `Advance`）: ティックとティックの間に置いたタイマーは保留になり、次のティックの終わりで始まる（`FTimerManager` の `PendingTimerSet`。本家の `Delay` と同じ）。1 回のティックの経過はワールドの `MaxUndilatedFrameTime`（0.4 s）で切られる。0 秒のティックを 1 回挟んでから 0.1 s 刻みで進める。
- 本家のレベル BP の開発用の PrintString（`Progress Saved`・`ALL SHARDS COLLECTED!`）と実績は写さない。

## テスト（`Tests/WasamiZoneFlowTests.cpp`・`Tests/WasamiDoorBreakTests.cpp`）
`Wasami.DoorBreak.Actor`（部品の値、`EnableSwitch` で箱が起きる、範囲の外の F は効かない、入ると見え・出ると隠れて空になる、Zone 2 の 3.0 で 34 回目に外れ・箱が消え・画面は見えたまま・1 s 後に画面が消える）、`Wasami.DoorBreak.Lock`（09 記録）、`Wasami.ZoneFlow.TriggerBox`（箱の大きさ・当たり・入る / 出るの発火）、`Wasami.ZoneFlow.Zone1`（障壁を置き、4 で到着のシーケンスが流れ、エレベーターの前の扉が閉ざされて `Open Front` でも開かない〈空の 14.1 s のシーケンスを持つ `ALevelSequenceActor` と両開き扉 2 枚を置く〉→ 7 s 後の放送と扉の破壊が起きる → 扉の破壊を 1.5 で 67 回押して `04_DoorBreak`・扉が開いて前から開き始める → 保存 5 → シャードの確かめ〈0.03 s 間隔の 2 回は最初の 0.05 s 後に 1 回〉→ 全回収で障壁が壊れる → 駐車場 → 06〈`Ballroom_Event_Fade` のプレイヤーが作られ 2 倍速で流れる〉→ トンネル → 開けておいたトンネルの手前の扉が閉ざされて閉じ始める・`BlockingVolume_1` が 25 s・眠らせた `AEmitter`〈空の粒子系。レベルのと同じく眠らせるため、生成の後に `DeactivateImmediate`〉が 25 s で起き、0.2 s 後に扉が消えている → 救急車で保存 7 と塞ぎ・1 s 後に救急車のシーケンス〈空の 13.9 s〉が流れる）、`Wasami.ZoneFlow.Zone2`（7 → 飛ばした場面の状態〈救急車がヨーを保って着いた所・前の塞ぎが消える・偽の天井が開く・スイッチが位置を保って倒れる〉と棘のシーケンス〈空の 70 s〉が流れる → 独房の扉の鍵を 3.0 で 34 回押して `Cell_DoorBreak`・扉のシーケンスが流れる → 1 s 後の Bierce の箱 → 保存 8 → 9 → 全回収で 10・球が消える・次のティックで COLLECT THE RING PIECE → 棘ですぐ打たれた閃き・0.5 s 後に死亡）、`Wasami.ZoneFlow.Start`（5・6・8・9・10 で開いたときの区間と目的、8〜10 では救急車は動かない、ゾーンの外では流れを出さない）。テストのセーブはスロット `WasamiTest_ZoneFlow`。シャードチェッカーは `Wasami.ArrowPointer.ZoneShardChecker`（`Tests/WasamiArrowPointerTests.cpp`。03 記録: 箱の既定の値、シャードが残るうちは消えない、`Check Shards` を待っている間に呼んでも待ち直さず 1 s で消える）。

## 確かめたこと（2026-09-18、PIE）
`Wasami.ResetSave` → Zone 1 を開く（04_Start、目的は空）→ `Wasami.Flow On04DoorBreak` → `BP_04_Trigger_Maze` に立つ（保存 5・COLLECT ALL SHARDS）→ `Wasami.CollectShards 3`（残り 3、目的はそのまま）→ `Wasami.CollectShards`（REACH THE PARKING LOT）→ `06_CutsceneStart` に立つ（06_Start へ移り REACH THE TUNNEL）→ `06_DoorsLock`（`BlockingVolume_1` が QueryAndPhysics）→ `06_TunnelEnter`（GET ON TOP OF THE AMBULANCE）→ 救急車の屋根（保存 7・GOOD LUCK・塞ぎ）→ 約 10.5 s 後に Zone 2 が開く（7、`PlayerStart_1`。ステップ 7a から独房の `PlayerStart_Cell`）→ `Wasami.Trigger BP_MiniBoss_Trigger`（保存 8・`Get past the nurses `）→ `Wasami.Trigger Trigger_MazeStart`（保存 9・COLLECT ALL SHARDS）→ `Wasami.CollectShards`（保存 10・COLLECT THE RING PIECE・球が消える）→ Zone 2 を開き直す（10、`PlayerStart_PostMaze`、シャード 0・COLLECT THE RING PIECE）。


扉の破壊（項目 6 のステップ 3b）: セーブを空にして Zone 1 の 04 から、7 s 後に `(0, 1010)` に立つと扉の前に鍵（F と灰色の輪）が出る → `Wasami.Interact 30` で輪が 12 時から時計回りに 45 % 赤くなる → 残り 37 回で外れ、輪が空に戻り赤い火花が出て約 0.25 s で消える（収録 `Intermediate/DesktopAgent/shots/doorbreak2.mkv`、git の外）→ 1 s 後に扉の破壊のアクタには `DefaultSceneRoot` だけが残る → `Wasami.Trigger BP_04_Trigger_Maze` で COLLECT ALL SHARDS（`04_DoorBreak` で迷路の箱が結ばれていた）。

両開き扉（項目 6 のステップ 3c）: セーブを空にして Zone 1 の 04 から、`BP_06_DoubleDoors11` は `bLocked` 真で閉じている（(0, 1700) から赤い扉が見える）→ `(0, 1010)` に立ち、7 s 後に `Wasami.Interact 67` で鍵が外れると扉が手前（+Y）へ 1 s で開き、少し行き過ぎて戻る → `(0, 1700)` へ移ると `Leave` の箱を出たので閉じる（本家どおり）。`(0, 1480)`（`Leave` の外）から `Wasami.Flow On04DoorBreak` では開いたまま（収録 `Intermediate/DesktopAgent/shots/doubledoors_open2.mkv`、git の外）。

障壁（項目 6 のステップ 4a）: 05 で全回収（`Wasami.CollectShards`）すると REACH THE PARKING LOT になり、駐車場への出入口の障壁が閃光とともに消える（08 記録の「確かめたこと」）。

フェードと扉が破られる（項目 6 のステップ 4b）: 05 で全回収 → `Wasami.Trigger 06_CutsceneStart` で画面が黒くなり 1 s そのまま、1.5 s で 06_Start（駐車場）が晴れて見える（収録の明るさ: 0 → 1.85 s に黒 → 2.8 s から晴れ始め 4.2 s で一定）。`(7210, -21800)`（扉の 700 cm 手前。-21300 は床が無く落ちる）で -Y を向き `Wasami.Trigger 06_DoorsLock` → 25 s で赤い扉が消え、コンクリートの破片が手前へ飛び散って向こうのロビーが見える（グリッド `Intermediate/Overnight/doors_busted_grid.png`、git の外）。煙はほとんど見えない（作業一覧の後回しの一覧）。

救急車で Zone 2 へ（項目 6 のステップ 5）: 05 → `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` → 救急車の屋根 `(11245, -20055, 470)` に +Y を向けて置く → SAVING PROGRESS、約 1 s 後に救急車がトンネルを走り出し、乗ってから 8 s で画面全体が暗い赤になり、約 4.7 s 後（PIE の読み込みを含む）に Zone 2 の独房の前の廊下が映る。走り出しの 2〜3 s にトンネルの床の前方が黒い矩形で欠けて見えた（作業一覧の後回しの一覧）。

独房から始める（項目 6 のステップ 7a）: `Wasami.Checkpoint 7` の後に Zone 2 を開くと、独房の `PlayerStart_Cell` (−14573.65, 1694.18) に立ち、救急車は (−14157.71, −5025.02, 800)・偽の天井は y 864.61・スイッチはロール 40.81 で、`Ambulance_Arrive_Blockers4` は無い。天井の針が下りてくる（開いて約 4.35 s で棘の Z −200）。扉の前 `(-14145, 1330)` で −Y を向くと F と灰色の輪が出る → `Wasami.Interact 34` で 0.5 s 以内に独房の扉が開いて向こうの壁のスイッチが見える（火花の粒子は 7b）→ 独房に残ると開いてから約 19.4 s で打たれた閃き（08 記録）と死亡画面。ライフが残る間はレベルが開き直り、また独房から始まる（収録 `Intermediate/DesktopAgent/shots/cell_door.mkv`・`spikes_death.mkv`、git の外。グリッド `Intermediate/Overnight/cell_grid.png`）。

## 変更履歴
- 2026-09-18: 初版。トリガーの箱と、2 つのゾーンの区間の流れの骨組み（作業一覧の項目 6 のステップ 1）
- 2026-09-18: `PlaySequence`・`PlayCameraShake` を足し、Zone 1 の 04 でエレベーターの到着のシーケンスと揺れ・7 s 後の揺れの終わりを流すようにした（項目 6 のステップ 3a）
- 2026-09-18: シャードチェッカー `AWasamiZoneShardChecker`（本家の `BP_ZoneShardChecker`）を足し、組み立てで両ゾーンに置いた。流れの矢印の値を地図の矢印（03 記録）が取るようにした（項目 6 のステップ 6）
- 2026-09-18: 扉の破壊 `AWasamiDoorBreak` と流れの `EnableDoorBreak` を足し、Zone 1 の 04 の 7 s 後に `BP_06_Hospital_DoorBreak_2` を起こして、外れたら `04_DoorBreak` へ進むようにした。テスト 2 本を足し、Zone 1 のテストを扉の破壊を解いて進むようにした（項目 6 のステップ 3b）
- 2026-09-18: 流れに `DoubleDoors` を足し、Zone 1 の 04 でエレベーターの前の扉（`BP_06_DoubleDoors11`）を閉ざし、鍵が外れたら開け、06 の扉の塞ぎでトンネルの手前の扉（`BP_06_DoubleDoors33_36`）を閉ざして閉じるようにした。Zone 1 のテストに両開き扉を足した（項目 6 のステップ 3c）
- 2026-09-18: 流れに `ZoneBarrier` を足し、Zone 1 の全回収で `BP_ZoneBarrier_2` を壊すようにした。Zone 1 のテストに障壁を足した（項目 6 のステップ 4a）
- 2026-09-18: Zone 1 の救急車の上で、1 s 後に救急車のシーケンスと揺れ、7 s 後に読み込み画面（`UWasamiLoadingWidget`）とポータルの音を出すようにした。回収の記憶を空にするのは読み込み画面の Construct に移した。Zone 1 のテストに救急車のシーケンスを足した（項目 6 のステップ 5）
- 2026-09-18: 流れに `PlayFadeOut`・`PlayWorldCameraShake`・`PlaySoundAt`・`ActivateEmitter` を足し、Zone 1 の 06 Transition のフェードと、06 の扉の塞ぎの 25 s 後に扉が破られる所（音・揺れ・破片・0.1 s 後に扉を消す）を埋めた。Zone 1 のテストに足した（項目 6 のステップ 4b）
- 2026-09-18: Zone 2 の 7 を独房から始めるようにした: 飛ばした場面が残す状態（救急車・前の塞ぎ・偽の天井・壁のスイッチ）を入れ、棘のシーケンス・独房の扉の鍵（`OnCellDoorBreak`: 扉のシーケンスと揺れ）・棘の死の打たれた閃き（`AWasamiHitFX`、08 記録）と音を足した。ゲームモードの 7 の PlayerStart を `PlayerStart_Cell` にした（06 記録）。Zone 2 と開始のテストを足した（項目 6 のステップ 7a）
