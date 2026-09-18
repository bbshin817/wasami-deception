---
title: ゾーンの進行（トリガー・区間の流れ）
sources:
  - Source/wasami_deception/WasamiTriggerBox.h
  - Source/wasami_deception/WasamiTriggerBox.cpp
  - Source/wasami_deception/WasamiZoneFlow.h
  - Source/wasami_deception/WasamiZoneFlow.cpp
  - Source/wasami_deception/WasamiZone1Flow.h
  - Source/wasami_deception/WasamiZone1Flow.cpp
  - Source/wasami_deception/WasamiZone2Flow.h
  - Source/wasami_deception/WasamiZone2Flow.cpp
  - Source/wasami_deception/Tests/WasamiZoneFlowTests.cpp
updated: 2026-09-18
---

# ゾーンの進行（トリガー・区間の流れ）

## 役割
本家の病院の 2 つのレベル BP（`pak_reference_2` の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`）が受け持つ「区間の流れ」。本作のレベルにはレベル BP が無いので、ゲームモードが開始時にゾーンの流れのアクタ（`AWasamiZone1Flow`・`AWasamiZone2Flow`）を出し、それが開いたチェックポイントの区間を始め（本家の `Spawn`）、トリガーの箱・全回収・チェックポイントの保存・目的の文・タブレットの矢印の値で区間から区間へ進める。トリガーの箱は本家の `BP_TriggerBox_Base`（`AWasamiTriggerBox`）。作業一覧の項目 6（ゾーンの進行）のステップ 1 で骨組みを作った。

**流れだけを進める**。場面・シーケンス・扉・障壁・読み込み画面・声（Bierce・ナースの放送）・曲・ナースは、それを作る項目・ステップが埋める。各イベントの中の、本家でそれをする位置にコメントで印を付けてある（`Not yet:` は項目 6 の残りのステップ、`(item N)` は作業一覧の別の項目）。

## 公開インターフェース
- `AWasamiTriggerBox`（`AActor`）… 本家の `BP_TriggerBox_Base`。
  - `OnTrigger`（動的マルチキャスト。本家の `Trigger`）、`bEndOverlap`（本家の `End Overlap`。病院で真にしているものは無い）、`GetBox()`、`HasFired()`。
  - `NotifyPlayerOverlap(bool bBegin)` … プレイヤーが入った / 出た。重なりの開始・終了はプレイヤー（`GetPlayerCharacter(0)`）のときだけこれを呼ぶ。デバッグのコマンドとテストは直接呼ぶ。
- `AWasamiZoneFlow`（`AActor`、Abstract）… 2 つのゾーンの共通の土台。
  - 静的関数: `SpawnFor(Mode, Zone)`（1 → `AWasamiZone1Flow`、2 → `AWasamiZone2Flow`、ほかは null。`SpawnActorDeferred` で `Mode` を入れてから `FinishSpawning`）、`SourceTag(Name)`（`src:<Name>`）、`FindSource(World, Name)`（そのタグを持つ最初のアクタ）、`RemoveAllEnemies(World)`（本家の `BP_DD_Functions` の同名の関数: タグ `Enemy` のアクタをすべて破棄）、`DestroyAllShards(World)`（残りの `AWasamiShard` を回収せずに破棄）。
  - `CallEvent(FunctionName)` … 引数の無い UFUNCTION のイベントを名前で呼ぶ（デバッグの `Wasami.Flow`・テスト）。無ければ偽。
  - `GetSection()` … 最後に来たイベントの本家の名前（`04_Start`・`05_Persistent`・`05 All Shards Collected`・`Miniboss Transition `〈後ろに空白〉など）。
  - 矢印の値: `IsArrowOnShards()`（本家の `Shards?`。既定は真 = `BP_ArrowPointer` の CDO）、`GetArrowColor()`（`Change Color` の色。`TOptional`、与えられるまで未設定）、`GetArrowTarget()`（`Target`）。矢印のアクタ（ステップ 6）がこれを読む。
  - `GetMode()`。
  - 派生が使う保護の関数: `StartAt(Checkpoint)`（仮想。本家の `Spawn`）、`Enter(Name)`、`BindTrigger(Source, Function)`（本家の `BindDelegate` + `AddMulticastDelegate`: `FScriptDelegate::BindUFunction` を箱の `OnTrigger` に `AddUnique`。箱が無ければ警告）、`BindAllShardsCollected(Function)`（ゲームモードの `OnAllShardsCollected` に同じく）、`SetObjective`、`SaveCheckpoint`（ゲームモードの `SaveCheckpoint`）、`SetArrowShards`・`SetArrowColor`・`SetArrowTarget`、`SetVolumeCollision(Source, Enabled)`（名前のボリュームのブラシの `SetCollisionEnabled`）、`TeleportPlayerTo(PlayerStartTag)`（本家の `K2_TeleportTo(PlayerStart の位置, 回転 0)` と `SetControlRotation(PlayerStart の回転)`）、`After(Seconds, Then)`（本家の `Delay`。弱い参照のラムダをタイマーで。0 秒は次のティック）、`Source(Name)`。
- `AWasamiZone1Flow` … 定数 `ArrivalShakeSeconds` 7、`ShardCheckDelay` 1、`DoorsBreakSeconds` 25、`TakeOffDelay` 1、`LoadingDelay` 7、`OpenZone2Delay` 2.5。イベント（UFUNCTION）: `On04Intercom`・`On04DoorBreak`・`On05Transition`・`On05AllShardsCollected`・`On05ParkingLotCutscene`・`On06TunnelEnter`・`On06DoorsLock`・`On06ReachAmbulance`。
- `AWasamiZone2Flow` … 定数 `SpikesDeathDelay` 0.5。イベント: `OnCellCutsceneFinished`・`OnSpikesDeath`・`OnMinibossBierceTalk`・`OnMinibossTriggerTransition`・`OnMinibossBehindMatron`・`OnMazeTriggerStart`・`OnMazeAllShards`。
- デバッグのコンソールコマンド（PIE では `python Tools/pie.py cmd "…"`）: `Wasami.Flow <関数名>`（`WasamiZoneFlow.cpp`。ゾーンの流れのイベントを呼ぶ。例 `Wasami.Flow On04DoorBreak`）。`Wasami.Trigger <本家の名前>`・`Wasami.CollectShards [N]` はゲームモード（06 記録）。

## 内部構造と処理の流れ

### トリガーの箱（`AWasamiTriggerBox`）
- 部品: ルートの `Box`（`UBoxComponent`、既定の半径 32 cm・`bHiddenInGame`・QueryOnly）。当たりは本家の `Box_GEN_VARIABLE` の Custom: 種類 WorldStatic、WorldStatic・WorldDynamic・Visibility・Camera・PhysicsBody・Vehicle・Destructible を無視、Pawn を Overlap。プロジェクトの Teleport チャンネルは本家の一覧に無いので既定の Overlap のまま（テレポートの照準のトレースを遮らない）。本家の空の `Cube`（メッシュの無い StaticMeshComponent）と開発用の `Debug` の PrintString は写さない。レベルの組み立てがアクタの拡縮で大きさを決める（01 記録）。
- `NotifyPlayerOverlap(bBegin)`: 入る・出るの 2 つの DoOnce（`bBeginClosed`・`bEndClosed`）。通ったほうの DoOnce は**発火するかどうかに関係なく閉じる**（本家 @244・@369）。`bBegin != bEndOverlap` のときだけ `bFired` を真にして `OnTrigger` を流す。**結ばれる前に通った箱はそれで使い切られる**（本家も同じ。本家は扉の奥などに置いて先に通れないようにしている）。

### 流れの生成と開始（`AWasamiZoneFlow`）
- ゲームモードの `BeginPlay`（06 記録）が、Zone 2 をチェックポイント 0 で開いたとき（Zone 1 を開き直す）以外、`SpawnFor(this, ZoneOf(レベル名))` で出す。流れの `BeginPlay` が `StartAt(Mode->GetStartCheckpoint())`（本家の `Setup` → `Important Casts` → `Spawn`。`Spawn` の頭の `SetViewTargetWithBlend`・`EnableInput` は、開いたばかりのレベルでは既定のまま）。
- 本家の名前で置かれたアクタを `src:<名前>` のタグで探す。タグはレベルの組み立てが付ける（メッシュは `src:<アクタ名>`、トリガーの箱とボリュームも同じ。01 記録）。

### Zone 1（`AWasamiZone1Flow`、本家 `06_Hospital_Zone_01`）
- `StartAt`（本家 `Spawn` @13483、`Load Progress By Level(7, 5)`）: 4 → `Start04`、5 → `Persistent05`、6 → `Start06`、ほかは何もしない（7 以上は Zone 2 のもの。0 はゲームモードが 4 にする）。チェックポイントの PlayerStart へ移すのはゲームモード（06 記録）。
- `Start04`（`04_Start`）: 7 s 後に `04_Intercom` → `On04Intercom`。まだ: `BP_06_DoubleDoors11` の Lock、`06_Hospital_Zone01_ElevatorArrive`、揺れ `01_Hotel_Lobby_ElevatorShake`、7 s 後の揺れの止め・`BP_06_Hospital_DoorBreak_2` の `Enable Switch` と `Finished Event` → `On04DoorBreak`（ステップ 3）。
- `On04Intercom`: 声だけ（`Nurse_Hospital_Zone01_Event_37_Intercom` 0.6 → 13 s → Bierce。項目 20）。
- `On04DoorBreak`（`04_DoorBreak`）: `BP_04_Trigger_Maze` → `On05Transition`。まだ: `DoubleDoors11` の `bLocked` 偽・`Open Front`（ステップ 3）、1 s 後の Bierce（項目 20）。
- `On05Transition`（`05_Transition`）: `SaveCheckpoint(5)` → `Persistent05`。
- `Persistent05`（`05_Persistent`）: 目的 `COLLECT ALL SHARDS`、全回収 → `On05AllShardsCollected`、1 s 後にゲームモードの `CheckShards`（開き直したときにシャードが残っていなければ、これで全回収に進む）。`Spawn Nurses`（項目 7）・曲の `bFadeOut` 偽（項目 19）。
- `On05AllShardsCollected`（`05 All Shards Collected`）: `RemoveAllEnemies`、矢印 `Shards?` 偽・色 (1, 0.8002, 0, 1)・的 `06_CutsceneStart`、目的 `REACH THE PARKING LOT`、`06_CutsceneStart` → `On05ParkingLotCutscene`。まだ: `BP_ZoneBarrier_2` の `Destroy`（ステップ 4）。曲（項目 19）。
- `On05ParkingLotCutscene`（`05_ParkingLotCutscene`）: 場面 `06_Hospital_Zone1_06Event`（項目 25）を飛ばし、その終わりの `06_Transition` = `Transition06` をすぐ呼ぶ。
- `Transition06`（`06 Transition`）: `DestroyAllShards`、`TeleportPlayerTo("06_Start")`、`Start06`。まだ: `Basic DD Fade Out(2)`（シーケンス `Ballroom_Event_Fade` を 2 倍速。ステップ 4）。
- `Start06`（`06_Start`。本家は `Spawn` の 6 と `06 Transition` の後半が同じ所）: `06_DoorsLock` → `On06DoorsLock`、`TriggerBox_06_AmbulanceTop` → `On06ReachAmbulance`、矢印 偽・(1, 0.8317, 0, 1)・的 `06_TunnelEnter`、目的 `REACH THE TUNNEL`、`06_TunnelEnter` → `On06TunnelEnter`。`Spawn Nurses_06`（項目 7）。
- `On06TunnelEnter`: 矢印 偽・(1, 0.8317, 0, 1)・的 `TriggerBox_06_AmbulanceTop`、目的 `GET ON TOP OF THE AMBULANCE`。
- `On06DoorsLock`: `BlockingVolume_1` の当たりを QueryAndPhysics → 25 s → NoCollision。まだ: `BP_06_DoubleDoors33_36` の Lock・`Force Close`、25 s 後の `DD_TT_Door_BustedOpen_02`・`BP_07_CameraShake_Jump`（0〜3000）・`Fracture_concrete_5`・0.1 s 後に扉を消す（ステップ 4）。ナースの `bAttackDoor`（項目 7）。
- `On06ReachAmbulance`: `SaveCheckpoint(7)`、`RemoveAllEnemies`、矢印 偽・(1, 0.8317, 0, 1)・的 `Plane48_2`、目的 `GOOD LUCK`、`BlockingVolume_Ambulance_4`・`_2`・`_1`・`_3` の当たりを QueryAndPhysics → 1 s → 7 s → ゲームインスタンスの `ForgetCollectedShards`（本家は `UMG_Loading` の Construct がする）・`RemoveAllEnemies` → 2.5 s → `OpenLevel(L_Hospital_Zone2)`。まだ: `06_Hospital_Zone1_AmbulanceTakeOff` と揺れ `06_CameraShake_Zone1_AmbulanceTakeOff`（拡縮 4）、`UMG_Loading`（Level 7、Z 5）と `21-Ballroom_portal_V2`（ステップ 5）。曲（項目 19）。

### Zone 2（`AWasamiZone2Flow`、本家 `06_Hospital_Zone_02`）
- `StartAt`（本家 `Spawn` @22328、`Load Progress By Level(7, 8)`）: 7 → `OnCellCutsceneFinished`（本家は `Arrive Event` → 捕まる場面 → 独房の場面。項目 25 まで飛ばす）、8 → `Miniboss Start ` → `MinibossTransition`、9 → `Maze Start` → `MazeTransition`、10 → `Postmaze Start` → `PostmazeTransition`（本家の `… Start` の PlayerStart への移動はゲームモード）。
- `OnCellCutsceneFinished`: `Trigger_Cell_Spikes` → `OnSpikesDeath`、`BP_MiniBoss_Trigger` → `OnMinibossTriggerTransition`、1 s 後に `Miniboss_BierceTalk` → `OnMinibossBierceTalk`。まだ: `06_Hospital_Zone2_Spikes`、`BP_06_Hospital_DoorBreak_2` → `Cell_DoorBreak`（ステップ 7）。曲（項目 19）・Bierce（項目 20）。
- `OnSpikesDeath`（`Spikes_Death`）: 0.5 s 後にゲームモードの `DeathEvent(プレイヤー)`。まだ: `BP_HitFX`・`DD_Needle_Trap_R1_V3`（ステップ 7）。
- `OnMinibossTriggerTransition`: `MinibossTransition` → `SaveCheckpoint(8)`。
- `MinibossTransition`（`Miniboss Transition `）: 矢印 偽・(0, 0, 0, 0)・的なし、目的 `Get past the nurses `（本家どおり後ろに空白）、`Trigger_MazeStart` → `OnMazeTriggerStart`、`Trigger_Miniboss_BehindMatron` → `OnMinibossBehindMatron`（放送の声だけ。項目 20）。実績（`06_NurseAlert`）は本作に無い。`Activate MiniBoss Enemies`（項目 11）。
- `OnMazeTriggerStart`: `MazeTransition` → `SaveCheckpoint(9)`（1 s 後の Bierce とリフトの一言は項目 20）。
- `MazeTransition`（`Maze Transition `）: `RemoveAllEnemies`、矢印 `Shards?` 真、目的 `COLLECT ALL SHARDS`、全回収 → `OnMazeAllShards`。`Spawn Nurses`（項目 7）。本家の Zone 2 には 1 s 後の `Check Shards` が無いので、9 で開き直してシャードが残っていなければ全回収には進まない（本家も同じ）。
- `OnMazeAllShards`: `SaveCheckpoint(10)` → `PostmazeTransition`（2 s 後の Bierce は項目 20）。
- `PostmazeTransition`: `RemoveAllEnemies`、`DestroyAllShards`、`src:ring_statue_orb_5` のアクタをすべて破棄、次のティック（本家の `Delay 0`）に矢印 偽・(1, 0.8941, 0, 1)・的 `BP_08_RingPiece_NoPickup_5`（まだ置いていないので null）、目的 `COLLECT THE RING PIECE`。`BP_Collectable` ID 3 を `collec` に出す（項目 12）、`ring_statue_2` の `Interact All Shards` → `Collected Ring Piece`（項目 13）。

## 作るアセット
なし（アクタはゲームモードが実行時に出す。トリガーの箱とボリュームはレベルの組み立てが置く。01 記録）。

## 原作データの根拠
- `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`（`python Tools/dd/bp_flow.py <file> <イベント名>` で読む。番地は上に書いた入口）、`Blueprints/Main/BP_TriggerBox_Base.txt`（@244・@369）と `_assets/…/BP_TriggerBox_Base.json`（`Box_GEN_VARIABLE` の当たり、SCS の `Cube` はテンプレート無し）、`Blueprints/Macros/BP_DD_Functions.txt` の `Remove All Enemies`・`Basic DD Fade Out`、`UI/Main/UMG_Loading.txt` の `Construct`（`Shards To Be Removed` を空に）、`_assets/…/BP_ArrowPointer.json`（`Shards?` 既定 真）。
- レベルのトリガーとボリュームの位置・拡縮・当たり: `pak_reference_2/_levels/06_Hospital_Zone_0{1,2}.full.json`（前処理を通して `stage_ue.json` の `actors`。01 記録）。

## 依存関係
- 自前: `AWasamiGameMode`（`GetStartCheckpoint`・`SaveCheckpoint`・`CheckShards`・`OnAllShardsCollected`・`CurrentObjective`・`DeathEvent`・`Zone2LevelName`。06・02 記録）、`UWasamiGameInstance::ForgetCollectedShards`、`AWasamiShard`、レベルの組み立て（`dd_level._flow`。01 記録）。
- 使う側: ゲームモード（生成）、プレイヤーのタブレットの帯（`CurrentObjective`。02・03 記録）、これからの矢印（ステップ 6）。
- エンジン: `UBoxComponent`、`UBrushComponent`（`AVolume`）、`FScriptDelegate::BindUFunction`、`FTimerManager`、`UGameplayStatics::OpenLevel`。

## 既知の制約・注意点
- **結ぶ前に通ったトリガーの箱は使い切られる**（上）。いまは扉（ステップ 3・4）が無いので、PIE で先に歩くと 04 の `BP_04_Trigger_Maze` などを使い切ってしまう。確かめるときは結ばれてから通すか、`Wasami.Flow`・`Wasami.Trigger` で進める。
- Zone 1 で 7 を保存してから Zone 2 が開くまでの 10.5 s に死ぬと、Zone 1 が 7 で開き直され、`StartAt` は何もせず UE の既定の PlayerStart から始まる（本家も `Spawn` に 7 の枝が無い）。
- Zone 2 の 7 はいま `PlayerStart_1`（救急車の到着の場所）から始まる。独房の `PlayerStart_Cell` にするのはステップ 7（独房の扉の破壊と一緒に）。
- テストのワールドでタイマーを進めるには注意が 2 つある（`Tests/WasamiZoneFlowTests.cpp` の `Advance`）: ティックとティックの間に置いたタイマーは保留になり、次のティックの終わりで始まる（`FTimerManager` の `PendingTimerSet`。本家の `Delay` と同じ）。1 回のティックの経過はワールドの `MaxUndilatedFrameTime`（0.4 s）で切られる。0 秒のティックを 1 回挟んでから 0.1 s 刻みで進める。
- 本家のレベル BP の開発用の PrintString（`Progress Saved`・`ALL SHARDS COLLECTED!`）と実績は写さない。

## テスト（`Tests/WasamiZoneFlowTests.cpp`）
`Wasami.ZoneFlow.TriggerBox`（箱の大きさ・当たり・入る / 出るの発火）、`Wasami.ZoneFlow.Zone1`（4 → 7 s 後の放送 → 扉の破壊 → 保存 5 → シャードの確かめ〈0.03 s 間隔の 2 回は最初の 0.05 s 後に 1 回〉→ 駐車場 → 06 → トンネル → 扉が 25 s → 救急車で保存 7 と塞ぎ）、`Wasami.ZoneFlow.Zone2`（7 → 1 s 後の Bierce の箱 → 保存 8 → 9 → 全回収で 10・球が消える・次のティックで COLLECT THE RING PIECE → 棘で 0.5 s 後に死亡）、`Wasami.ZoneFlow.Start`（5・6・8・9・10 で開いたときの区間と目的、ゾーンの外では流れを出さない）。テストのセーブはスロット `WasamiTest_ZoneFlow`。

## 確かめたこと（2026-09-18、PIE）
`Wasami.ResetSave` → Zone 1 を開く（04_Start、目的は空）→ `Wasami.Flow On04DoorBreak` → `BP_04_Trigger_Maze` に立つ（保存 5・COLLECT ALL SHARDS）→ `Wasami.CollectShards 3`（残り 3、目的はそのまま）→ `Wasami.CollectShards`（REACH THE PARKING LOT）→ `06_CutsceneStart` に立つ（06_Start へ移り REACH THE TUNNEL）→ `06_DoorsLock`（`BlockingVolume_1` が QueryAndPhysics）→ `06_TunnelEnter`（GET ON TOP OF THE AMBULANCE）→ 救急車の屋根（保存 7・GOOD LUCK・塞ぎ）→ 約 10.5 s 後に Zone 2 が開く（7、`PlayerStart_1`）→ `Wasami.Trigger BP_MiniBoss_Trigger`（保存 8・`Get past the nurses `）→ `Wasami.Trigger Trigger_MazeStart`（保存 9・COLLECT ALL SHARDS）→ `Wasami.CollectShards`（保存 10・COLLECT THE RING PIECE・球が消える）→ Zone 2 を開き直す（10、`PlayerStart_PostMaze`、シャード 0・COLLECT THE RING PIECE）。

## 変更履歴
- 2026-09-18: 初版。トリガーの箱と、2 つのゾーンの区間の流れの骨組み（作業一覧の項目 6 のステップ 1）
