---
title: ゲームの流れ（シャード・ライフ・セーブ）
sources:
  - Source/wasami_deception/WasamiGameInstance.h
  - Source/wasami_deception/WasamiGameInstance.cpp
  - Source/wasami_deception/WasamiSaveGame.h
  - Source/wasami_deception/WasamiSaveGame.cpp
  - Source/wasami_deception/Tests/WasamiGameFlowTests.cpp
  - Source/wasami_deception/WasamiShard.h
  - Source/wasami_deception/WasamiShard.cpp
  - Source/wasami_deception/Tests/WasamiShardTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_shards.py
  - SourceArt/Wasami/wasami_mochi.glb
updated: 2026-09-19
---

# ゲームの流れ（シャード・ライフ・セーブ）

## 役割
本家のソウルシャード（最新版 `pak_reference_2` の `Blueprints/Main/BP_Shard`）。ステージに置かれ、触れると回収され（タブレットの数が 1 減る）、テレキネシスで引き寄せられる。見た目は本作のワサミ餅（ユーザーの決定）。チェックポイント・セーブ・ライフ・死亡・脱出（M3 の残り）はこの記録に書き足していく。原作の流れの調べは `.claude/references/game-flow/README.md`。

ゲームの流れの土台（作業一覧の項目 5）のステップ 3 で、**ゲームインスタンス**（ライフと回収済みのシャードの記憶）・**セーブ**（`structSlot`）と、ゲームモード（02 記録）の死亡の受け口・時間・チェックポイントの保存・開き直したときの回収済みのシャードの除去を足した（下の「ライフ・セーブ・死亡の受け口」）。ステップ 4 で死亡画面（ライフ −1・死亡数の保存・ゲームオーバーの表示・パワーのリセットとレベルの開き直し）を足した（09 記録）。ステップ 5 で、病院の Zone のレベル BP が受け持つ所をゲームモードに足した: `DeathEvent` が死亡画面を出してゲームを止める、セーブのチェックポイントの PlayerStart から始める、チェックポイントの保存で SAVING PROGRESS、レベルを開くたびの黒からの明け、デバッグのコンソールコマンド（下の「開始の場所・死亡画面・SAVING PROGRESS」）。ステップ 6 でゲームオーバーの 3 つのボタンの行き先（RESTART・LAST CHECKPOINT・QUIT TO TITLE。ライフ・回収の記憶・セーブの扱いを含む）を足した（09 記録）。

シャードは**最小限**（タブレットのパワーの作業〈作業一覧の項目 1〉のステップ 9 で作った）: 連続回収の判定 `Check Streak`、`bDisabled` と `Enable` はまだ無い。ゲームモードの `Check Shards`（`Collect Shard` の通知と全回収の判定）は作業一覧の項目 6 のステップ 1 で足した（下の「シャードの確かめ」）。回収の閃光 `P_ky_flash3` はステップ 9b で足し、作業一覧の項目 3 で本作の紫でやや弱い版 `P_WasamiShardFlash` に替えた。

## 公開インターフェース
- `AWasamiShard`（`AActor`、`IWasamiTelekinesisInterface` を実装）
  - `Collect(bool bNoSound)` … 本家の `Collect(NoSound?)`。1 回だけ（DoOnce）。下の「回収」。
  - `Activate`（インターフェース）… 本家の `Activate`。下の「引き寄せ」。
  - `IsPulling()`・`GetPullRate()`・`GetSpinRate()` … 確認用。
  - 静的関数: `EvaluatePullAlpha(Seconds)`（`Shard Pull` の `Alpha`）、`PullLocation(From, Player, Alpha)`（ExpoIn で水平だけ寄せた位置）、`SpinSpeed(PlayRate)`（餅の回る速さ °/s）。定数 `PullLength` = 1。
  - 部品: `DefaultSceneRoot`、`Body`（本家の `SkeletalMesh` の位置と拡縮だけを持つ `USceneComponent`）、その子の `Mochi`（`UStaticMeshComponent`）・`PointLight`・`Capsule`、ルートの子の `Plane`（ミニマップの印）。`GetPlane()` が印を返す（Zone 2 の階ごとの地図が、プレイヤーの階のシャードの印だけを見せる。03 記録）。
  - 値: `LightIntensity` 175（本家の `Light Intensity`）、`MinimapPlaneHeight` 2000（`Minimap Plane Height`）。
  - 素材（ソフト参照、`WasamiAssets.h`）: `MochiMesh` `/Game/Wasami/Shard/SM_WasamiMochi`、`PlaneMesh` `/Engine/BasicShapes/Plane`、`MapMarkMaterial` `/Game/DD/Materials/Shared/M_Shard`、`PickupSound` `/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2_Cue`、`PickupConcurrency` `/Game/DD/Audio/OnlyFew`、`CollectShake` `/Game/DD/Blueprints/Shared/BP_CameraShake_ShardCollect`、`CollectFlash` `/Game/Wasami/Shard/P_WasamiShardFlash`（本家は `P_ky_flash3`。下の「本作の回収の閃光」）。
- `UWasamiGameInstance`（`UGameInstance`。`Config/DefaultEngine.ini` の `GameInstanceClass`、00 記録）… 本家の `BP_DD_GameInstance`。レベルを開き直しても残り、ディスクには書かない。
  - `GetLives()`・`DecrementLives()`・`IncrementLives()`（どちらも 0..`MaxLives` 6 に Clamp）・`ResetLives()`（= `StartingLives` 3）… 本家の `BP_DD_Functions` の同名の関数。本家の既定の `Lives` は 3、`Reset Lives` はプレイヤーのレベル 0〜4 で 3（本作にプレイヤーのレベルは無い）。
  - `RememberCollectedShard(StartLocation)`（`ShardKey` にして `AddUnique`）・`ForgetCollectedShards()`・`GetShardsToBeRemoved()` … 本家の `Shards To Be Removed`。静的関数 `ShardKey(Location)` は本家の `FTruncVector`（各成分を 0 の方へ切り捨てた整数）を `FVector` に戻したもの。
- `UWasamiSaveGame`（`USaveGame`）… 本作のセーブ 1 つ。スロット `SlotName` = `structSlot`（本家の `BP_DD_levelStructSave` のスロット）、`UserIndex` 0。`Hospital`（`FWasamiLevelProgress`）と `bLastCheckpointWarning`（本家の `SaveSlot` の `Last Checkpoint Warning`）。
  - `FWasamiLevelProgress` … 本家の `DD_LevelStructureyyy` のうち病院が書く欄: `LevelCheckpoint`（int。Zone 1 は 4〜6、Zone 2 は 7〜10、0 は無し）・`Deaths`・`Time`（float、秒）・`CurrentStreak`・`Streak`（本家の byte の enum を `uint8` で）。本家は 11 個の配列 `levelStruct` の添字 5 が病院。
- ゲームモードの口（`AWasamiGameMode`、02 記録）: `DeathEvent(Cause)`・`ResetDeath()`・`IsDeathOpen()`、`OnDeath`（本家の `Death Dispatcher`）・`OnAllShardsAlreadyCollected`、`CheckShards()`・`OnCollectShard`・`OnAllShardsCollected`、`GetZoneFlow()`、`PauseTimeCounter()`・`UnpauseTimeCounter()`・`ResetTimeCounter()`・`GetTime()`、`SaveCheckpoint(Checkpoint)`、`GetSave()`・`WriteSave()`、`GetWasamiGameInstance()`、`GetTotalShards()`・`GetShardStreak()`、`GetStartCheckpoint()`（レベルを開いたときのチェックポイント。項目 6・13 が区間の準備に使う）、静的関数 `RemoveCollectedShards(World, Collected)`・`ZoneOf(LevelName)`・`PlayerStartTagFor(Zone, Checkpoint)`・`DeathScreenLevelFor(Zone, bCausedByPlayer)`、定数 `Zone1LevelName`（`L_Hospital_Zone1`）・`Zone2LevelName`（`L_Hospital_Zone2`）・`OpeningFadeSpeed` 10・`OpeningFadeZOrder` 10、`SaveSlotName`（既定 `structSlot`。テストが別のスロットにする）。
- デバッグのコンソールコマンド（`WasamiGameMode.cpp`。PIE では `python Tools/pie.py cmd "…"`）: `Wasami.Kill`（プレイヤーを原因に `DeathEvent`）、`Wasami.Checkpoint N`（`SaveCheckpoint(N)`。SAVING PROGRESS も出る）、`Wasami.ResetSave`（セーブの `Hospital` と警告を空にして書き、ライフ 3・回収の記憶を空に。開き直すと最初から）、`Wasami.Lives N`（ライフを N〈0〜6〉に）、`Wasami.CollectShards [N]`・`Wasami.Trigger <名前>`（下の「シャードの確かめ」）。本家の開発用の近道（パッケージしないときの `Fake` のチェックポイント、J キー）は写さない。
- ツール: `WasamiDDTools.import_dd_shards()`（素材）、`WasamiStageTools.place_dd_shards(zone)`（配置。01 記録）。

## 内部構造と処理の流れ

### 部品（本家の `BP_Shard.json` の SCS）
- `Body` = 本家の `SkeletalMesh`: 相対位置 (0, 2.2888e−5, 97.0854)、拡縮 10。
  - `Mochi`: 拡縮 0.0825（10 倍の下で 0.825 m。WebGL 版の `game.shard.size` 0.55 の 1.5 倍、ユーザーの依頼）、当たりなし・ナビに関わらない・影なし、描画距離 3000（本家の `LDMaxDrawDistance`）。メッシュは構築時に入れる（`OnConstruction`）。材質はメッシュが持つ。
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
4. ゲームモード（`GetAuthGameMode<AWasamiGameMode>`）の `CheckShards()`（本家 @773 の `GameMode.Check Shards`。下の「シャードの確かめ」）。
5. `ClientStartCameraShake(BP_CameraShake_ShardCollect, 0.4, CameraLocal)`。
6. `SpawnEmitterAtLocation(P_WasamiShardFlash, Body の位置, 回転 0, 拡縮 0.2, 自動破棄, プールなし, 自動起動)`（本家は `P_ky_flash3` を `SkeletalMesh` の `K2_GetComponentLocation` に。`BP_Shard` @900〜@950）。部品はワールドの `WorldSettings` に付き、シャードの破棄の影響を受けない。エミッタの長さ（`RequiredModule` の既定の 1 秒）で終わって消える。
7. `NoSound` が偽なら、ゲームインスタンス（`UWasamiGameInstance`。無ければ飛ばす）の `RememberCollectedShard(PreviousLocation)`（BeginPlay の位置を切り捨てて `AddUnique`。本家 @1228〜@1388）。
8. `PlaySound2D(Soul_Shard_Pickup_v2_Cue, NoSound ? 0 : 0.65, 1.0, 0, OnlyFew)` → `Destroy()`。本家は破棄してから鳴らすが、同じフレームなので聞こえ方は同じ。破棄の後のワールドの取り方を当てにしないよう、音を先にした。
- 重なりの開始（`OnCapsuleBeginOverlap`）: 相手がプレイヤー（`GetPlayerCharacter(0)`）なら `Collect(false)`。本家の重なりの経路は `NoSound` を書かずに回収へ飛ぶが、そこへ来るのは DoOnce が開いているとき（＝`Collect` がまだ呼ばれていない、`NoSound` が既定の偽）だけなので同じ。
- タブレットの数は、プレイヤーが 0.1 秒ごとにシャードのアクタを数え直す（02 記録。破棄されたアクタは数えない）。回収の直後の −1 は本家どおり画面に直接書く。

### ライフ・セーブ・死亡の受け口（ゲームモードとゲームインスタンス）
- **ゲームモードの BeginPlay**（本家 @33228〜）: `CheckForLevelStructSave`（本家の `Check For Level Struct Save` @40634。`LoadGameFromSlot(SaveSlotName, 0)` を `UWasamiSaveGame` にキャストして持ち、無ければ `CreateSaveGameObject` してすぐ `SaveGameToSlot`）。本家はここでタブレットの数をレベルのシャードの数にするが、本作はプレイヤーの 0.1 秒ごとの数え直し（02 記録）が同じことをする。**0.2 秒後**（本家の `Delay 0.2` → @4551）に `RemoveShardsToBeRemoved`: `TotalShards` = レベルのシャードの数（消す前）。ゲームインスタンスの `Shards To Be Removed` が空でなければ `RemoveCollectedShards` で消し、残りが 1 未満なら `OnAllShardsAlreadyCollected` を流す（本家も空のときは数えて終わる）。
- `RemoveCollectedShards(World, Collected)`: 各シャードの**今の位置**を `ShardKey` にして `Collected` にあるものを集めてから `Destroy`（音も数えもしない）、残りの数（`TActorIterator` は破棄したアクタを数えない）を返す。
- **時間**: ゲームモードの `Tick` が `Time += dt`（本家の `ReceiveTick` のゲート。始めから開いている）。`PauseTimeCounter` / `UnpauseTimeCounter` でゲートを閉じる・開く、`ResetTimeCounter` で 0。ゲームを止めている間（`SetGamePaused`）はティックしないので数えない（本家も同じ）。
- **`DeathEvent(Cause)`**（本家 @34486 → @15098）: DoOnce（`bDeathClosed`。`ResetDeath` で開く）。`OnDeath.Broadcast(Cause)`、**死亡画面を出す**（下）、`ShardStreak` = max(`ShardStreak`, セーブの `CurrentStreak`)、セーブの `CurrentStreak` = 0（書くのは死亡画面）。本家が先に止める Bierce の独り言のタイマーは声の項目（20）までは無い。死亡画面の Construct は足したその場で走り、セーブの `CurrentStreak` を 0 にするので、画面が出たときの連続回収の最高は 0 と比べることになる（本家も同じ順。本家のレベル BP は `Death Dispatcher` を受けて同じフレームで画面を足す）。
- **`SaveCheckpoint(Checkpoint)`**（本家のレベル BP のチェックポイントの保存と同じ形）: SAVING PROGRESS（`UWasamiSavingWidget::Show`、Z 0。09 記録）、`LevelCheckpoint` = 値、`Time += ゲームモードの Time`、`ResetTimeCounter`、`WriteSave`。呼ぶ場面はまだ無い（項目 6・13。いまはデバッグの `Wasami.Checkpoint N`）。
- レベルを開き直すと（本家の再開も開き直し）、ゲームモードは作り直されてセーブを読み直し、ゲームインスタンスのライフと `Shards To Be Removed` は残る。

### 開始の場所・死亡画面・SAVING PROGRESS（本家の Zone のレベル BP の受け持ち）
本作の Zone にはレベル BP が無いので、本家の `06_Hospital_Zone_01`・`_02` のレベル BP が死亡と再開のためにすることをゲームモードが持つ。
- **開始の準備 `PrepareStart`**（1 回だけ）: セーブを読み（`CheckForLevelStructSave`）、Zone 1（`ZoneOf` がレベル名の `Zone1` / `Zone2` で 1 / 2、ほかは 0）でチェックポイントが 0 なら **4 を書く**（本家は入口の `03_ElevatorEnter` が 4 を保存してから Zone 1 を開くので、Zone 1 は必ず 4 以上。本作に入口は無い）。その値を `StartCheckpoint` に持つ。プレイヤーはどのアクタの `BeginPlay` より先に置かれる（`UEngine::LoadMap` が `SpawnPlayActor` の後に `BeginPlay`）ので、最初に呼ぶのは下の `ChoosePlayerStart`。プレイヤーのいないワールド（テスト）では `BeginPlay`。
- **開始の場所 `ChoosePlayerStart`**: 本家の Zone の `Spawn`（Zone 1 @13483、Zone 2 @22328）は、チェックポイントの PlayerStart へ `K2_TeleportTo`（回転 0）して `SetControlRotation`（その PlayerStart の回転）する。本作はその PlayerStart から出す（出る位置とヨーは同じ）。対応は `PlayerStartTagFor`: Zone 1 の 4 → `04_Start`、5 → `05_Start`、6 → `06_Start`、Zone 2 の 7 → `PlayerStart_Cell`（本家の 7 は到着の `PlayerStart_1` から始まり、独房の場面の頭の `Cell Cutscene Start` が `PlayerStart_Cell` へ移す。場面は項目 25 まで飛ばすので、その後の位置から。2026-09-18、項目 6 のステップ 7a）、8 → `PlayerStart_MiniBoss`、9 → `PlayerStart_Maze`、10 → `PlayerStart_PostMaze`。PlayerStart は `PlayerStartTag` が本家の名前（取り込みが入れる。01 記録）。表に無い値は UE の既定の選び方（空いている PlayerStart から無作為。本家もその Zone の既定の場所のまま）。PIE の Play From Here（`APlayerStartPIE`）があればそれを優先する。
- **Zone 2 でチェックポイントが 0** なら `BeginPlay` で Zone 1 を開く（本家は入口を開く）。PIE で Zone 2 を試すときは、先に `Wasami.Checkpoint 7`（〜10）を書いてから開く。
- **区間の準備**（ナース・目的・扉・シーケンス。本家の `Spawn` の続き）はゾーンの流れ（`AWasamiZoneFlow`、11 記録）が `GetStartCheckpoint()` を見て始める。ゲームモードの `BeginPlay` が、Zone 2 を 0 で開いて Zone 1 を開き直すとき以外、`AWasamiZoneFlow::SpawnFor(this, ZoneOf(レベル名))` で出して `GetZoneFlow()` に持つ。本家の `Spawn` の頭の `SetViewTargetWithBlend`・`EnableInput` は、開き直したワールドでは既定のままなので要らない。
- **死亡画面 `ShowDeathScreen`**（本家の Zone の `DeathEvent`、Zone 1 @14791 → @5731、Zone 2 @23519 → @5758）: `UWasamiDeathScreenWidget::Show(this, Level)`（Z 5 で足してゲームを止める。09 記録）。`Level` は `DeathScreenLevelFor`: Zone 1 は原因がプレイヤー（`AWasamiPlayerCharacter`。本家はクラスの一致）なら Traps（4）、ほかは Asylum（7）。Zone 2 は逆（本家はプレイヤーか `BP_GremClown` で Asylum）。本家は画面の `Respawn Event` をレベルの `Respawn`（画面を外す・ポーズを解く・`EnableInput`・`Reset Death`・`Spawn`）に結ぶが、画面がその直後に今のレベルを開き直すので結ばない。
- **レベルを開いたときの黒からの明け**: `BeginPlay` が `UWasamiBlackFadeWidget::Show(this, false, 10, 10)`（本家の @6710。`FadeOut` を速さ 10 = 0.5 s。09 記録）。プレイヤーコントローラーが無ければ（テスト）出さない。

### シャードの確かめ（`Check Shards`。本家 `BP_DD_GameMode` @34491）
- `CheckShards()`: `OnCollectShard`（本家の `Collect Shard`）を流し、**0.05 s 後**（本家の `Delay 0.05`）にレベルの `AWasamiShard` を数え、1 未満なら `OnAllShardsCollected`（本家の `All Shards Collected`）を流す。待っている間にもう一度呼ばれても待ち直さない（`IsTimerActive` なら置かない。本家の `Delay` と同じ）。呼ぶのはシャードの `Collect` と、ゾーンの流れ（Zone 1 の `05_Persistent` の 1 s 後。11 記録）。
- 本家はその間に `Check Streak`（連続回収の数と `UMG_ShardStreak`。まだ無い）、待った後に Bierce の独り言の開始（項目 20）、半分のときの声・全回収の声を鳴らすが、病院（`Level` 7）はどちらの声も `None`。全回収の `Event All Shards` は `BP_Monkey` だけを `Activate Frenzy` にし（病院にはいない）、独り言のタイマーを止めるだけなので、病院では `All Shards Collected` を流すことに尽きる。開発用の PrintString（`ALL SHARDS COLLECTED!`）は写さない。
- デバッグのコンソールコマンド: `Wasami.CollectShards [N]`（レベルのシャードを N 個残して、触れたときと同じく `Collect(false)` で回収する。既定 0）、`Wasami.Trigger <本家の名前>`（その名前のトリガーの箱〈11 記録〉を、プレイヤーが通ったように発火させる）。

## 作るアセット
`WasamiDDTools.import_dd_shards`（`pipeline/dd_shards.py`）が作る。

| パス | 中身 |
| --- | --- |
| `/Game/Wasami/Shard/SM_WasamiMochi` | 餅のメッシュ（`SourceArt/Wasami/wasami_mochi.glb` = ユーザーの `wasami_mochi_v3`、101,368 三角形、LOD 1 枚、Nanite、スロット 1 に `MI_WasamiMochi`）。1.00 × 1.00 × 0.88 m で原点が中心（UE の X・Y・Z。顔は横を向く） |
| `/Game/Wasami/Shard/T_WasamiMochi_BaseColor`・`_MetallicRoughness`・`_Normal` | glb に埋め込まれた PNG（色 2048²・法線 2048²・金属と粗さ 4096²）を `Intermediate/Pipeline/wasami/shard/` に書き出して取り込む。ベースカラーは sRGB、金属と粗さはリニア、法線は `TC_Normalmap`・`TEXTUREGROUP_WorldNormalMap`・緑を反転（glTF の法線は Y が上向き、UE は下向き）。**大きさは焼いたままにし、上限は設けない**（下の「餅のモデル」） |
| `/Game/Pipeline/Materials/M_DD_WasamiMochi`、`/Game/Wasami/Shard/MI_WasamiMochi` | glTF の金属・粗さの材質（係数はすべて 1）: ベースカラー、金属 = B、粗さ = G、法線、両面。自己発光 = ベースカラー × `Glow`〈0.3。WebGL 版の `game.shard.glow`〉 |
| `/Game/Pipeline/Materials/M_DD_MapMark`、`/Game/DD/Materials/Shared/M_Shard` | 地図の印の推定のマスターと、その原作のパスのインスタンス。`Color` をベースカラー（シーンキャプチャが読む）と自己発光に出す。`M_Shard` の色は (0.70, 0.0071, 1.0)（下の「印の色」） |
| `/Game/DD/Audio/SharedGameplay/Soul_Shard_Pickup_v2`・`Soul_Shard_Pickup_v2_Cue` | 回収の音（0.43775 s）と、その Cue（`SoundNodeModulator` のピッチ 0.9〜1.1、音量は既定の 0.95〜1.05 → `SoundNodeWavePlayer`）。`dd_assets.sound_cue`（01 記録） |
| `/Game/DD/Audio/OnlyFew` | 同時発音（`MaxCount` 1・`StopOldest`・`VolumeScale` 0.5） |
| `/Game/DD/Blueprints/Shared/BP_CameraShake_ShardCollect` | 回収の揺れ（0.1 s、ブレンドアウト 0.05 s、ロール 1.5°・FOV 3° を周波数 15 で、ほかは振幅 0） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash3` | 原作の回収の閃光（下の「回収の閃光」）。`dd_particles` が書き出しから組む（01 記録）。シャードは使わず、比べるために残す |
| `/Game/Wasami/Shard/P_WasamiShardFlash` | 本作の回収の閃光（下の「本作の回収の閃光」）。`dd_shards.make_flash` が `P_ky_flash3` の書き出しの色を直して組む |
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

### 本作の回収の閃光（`P_WasamiShardFlash`）
ユーザーの依頼（「紫色のやや弱めな閃光」）による本作独自の見た目で、本家にも WebGL 版にも無い。`P_ky_flash3` の書き出しを `dd_particles.particle_system(…, target, adjust)` に渡し、組む前に色の表だけを直す（`dd_shards._purple_flash`。構造・寿命・大きさ・α・材質は原作のまま。材質は原作のパスのものを共有する）。
- 7 つの `ParticleModuleColorOverLife` の参照表の色ごとに、色 → `FLASH_COLOR` × (その色の最大のチャンネル ^ `FLASH_GAMMA`) × `FLASH_STRENGTH`。`FLASH_COLOR` はシャードの灯と同じ紫 (194, 0, 255) のリニア (0.539, 0, 1.0)、`FLASH_GAMMA` は仮の 0.5、`FLASH_STRENGTH` は仮の 0.8。表の範囲（`MinValueVec`・`MaxValueVec`〈チャンネルごと〉、`MinValue`・`MaxValue`〈その最小・最大〉）も直した表から出す（cook が残した値の関係と同じ）。書き出しの色のモジュールが 7 つでない、または別の色のモジュールがあれば例外。
- その結果: core (0.96, 0, 1.79)、glow (1.14, 0, 2.12) → (0.43, 0, 0.8)、shockwave (0.43, 0, 0.8) → (0.37, 0, 0.70)、glowSub・decoCore (0.37, 0, 0.70)、dust_line (0.75, 0, 1.39)、light (0.43, 0, 0.8)。
- 指数を掛ける理由: 紫は輝度が白の約 0.19 倍なので、ただの係数（線形の 0.6 を PIE で試した）では明るい中心の星は残っても、半透明の白い衝撃波の輪と虹の円が 1/9 の輝度になってほぼ見えなくなった。歩いてシャードに触れたときに画面に出るのは主にこの輪。指数 0.5 で明るい所を抑え、薄い所を残す。係数は、1.34（中心を線形 0.6 と同じ 3.0 に保つ値）では紫のもやが原作の閃光より強くなったので 0.8 にした（PIE の測り方と値は `observations/README.md` の「紫の回収の閃光」）。
- 灯の明るさ（2.5）は変えない: 粒子の灯の色は粒子の色 × α × `ParticleModuleLight` の色 × 明るさ（UE 5.8 の `ParticleSystemRender.cpp`・`ParticleModules.cpp`）なので、色の係数だけで灯も同じだけ弱まる。
- `M_ky_polarGlow02` の自己発光は虹のテクスチャ × 粒子の色なので、緑が 0 の紫を掛けると虹の緑が消える。

### 閃光の材質（推定。グラフは cook で消えている）
共通: 半透明・Unlit・スプライトとメッシュの粒子用。原作のインスタンスと粒子が使う静的スイッチの側だけを作る（`dd_shards.py` の各ビルダーの説明に、残っていた式と推定を書いた）。マスター・原作のパスのインスタンス・原作のインスタンスは `dd_assets.estimated_materials` が作り、グラフの小道具も `dd_assets` のもの（01 記録。テレキネシスの力場も同じ作り方）。
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
- 餅: WebGL 版の `public/assets/models/wasami_mochi.glb`（`scripts/prepare-shard-model.mjs` が原本の 300 万三角形・2048² から作ったもの）と `src/world/shards.ts`・`src/config.ts` の `game.shard`（大きさ 0.55・`glow` 0.3。大きさはユーザーの依頼で 1.5 倍にした）。浮き沈み・脈動は WebGL 版の表現で本家に無いので採らない。
- UE 5.8: `ShapeComponent.cpp`（`OverlapAllDynamic`、`AreaClass` の既定）、`SkinnedMeshComponent.cpp`（`bRecentlyRendered` の 1 秒）、`Timeline.cpp`（`PlayFromStart` の更新）、`KismetMathLibrary`（`VEase`）、`GameplayStatics.h`（`PlaySound2D` の `bIsUISound` 既定 真。UE 4.24 も真）。

## 依存関係
- ライフ・セーブ: ゲームモード（`AWasamiGameMode`、02 記録）がセーブとゲームインスタンスを使い、シャードが `RememberCollectedShard` を呼ぶ。エンジンの `USaveGame`・`UGameplayStatics`（`LoadGameFromSlot`・`SaveGameToSlot`・`CreateSaveGameObject`）・`UKismetMathLibrary::FTruncVector`・`TActorIterator`。
- 自前: `AWasamiPlayerCharacter`（`GetTabletScreen`、02 記録）、`UWasamiTabletWidget`（`GetShardCount`・`SetShardCount`・`PlayCountShake`、03 記録）、`IWasamiTelekinesisInterface`（04 記録）、`WasamiAssets.h`（00 記録）。取り込みは `dd_assets`・`dd_stage`・`paths`（01 記録）。
- 使う側: プレイヤーの `ShardActorClass`（既定がこのクラス。数と地図、02 記録）、レベルの組み立て（01 記録）、テレキネシス（`AWasamiTelekinesisPower::PullShards` が半径の中のシャードに `Activate` を呼ぶ。04 記録）。
- エンジン: `UCapsuleComponent`・`UPointLightComponent`・`UStaticMeshComponent`、`UGameplayStatics`（`PlaySound2D`・`GetPlayerCharacter`・`GetPlayerController`）、`UKismetMathLibrary::VEase`、`FRichCurve`、`APlayerController::ClientStartCameraShake`。

## 既知の制約・注意点
- **自動テストも本物のセーブに触れる**: `FTestWorldWrapper::BeginPlayInTestWorld` は既定のゲームモード（`AWasamiGameMode`）を作るので、ワールドを遊ばせるテストはどれも `structSlot` を読み、無ければ空のセーブを書く（`Saved/SaveGames/structSlot.sav`）。空のセーブはセーブが無いのと同じ中身（チェックポイント 0）。ゲームモードのテストは `SaveSlotName` を `WasamiTest_structSlot` にして、終わりに消す。
- セーブを消してやり直すときは、PIE で `Wasami.ResetSave` の後にレベルを開き直す（`open L_Hospital_Zone1`）か、PIE の外で `Saved/SaveGames/structSlot.sav` を消す。消した直後の読み込みは `LogStreaming: Warning: Failed to read file '…/structSlot.sav'` を 1 行出す（無いときの `LoadGameFromSlot`。害は無い）。
- **見た目は原作と違う**（ユーザーの決定。`.claude/guides/original-fidelity.md`）。大きさ・位置・回転の速さ・灯は原作の値に合わせ、材質は餅のテクスチャ（推定なし）に WebGL 版の自己発光を足したもの。大きさだけはユーザーの依頼で原作の 1.5 倍。回る速さは個体ごとの乱数で、最新版の実機で撮った 1 つ（21.0 秒で 1 周 = 17.1 °/s）は本作の範囲（10.8〜32.4 °/s）に入った（パワーの作業のステップ 11a・11b1。`observations/README.md`）。2026-09-17 のユーザーの回答に従い、**紫の明滅を外して餅を 1.5 倍（0.825 m）にした**（作業一覧の項目 22）。2026-09-18 に**モデルをユーザーの `wasami_mochi_v3` に替えた**（同じ項目。下の「餅のモデル」）。**回り方は本家の結晶と同じ撮り方で見比べたうえで、直さないと決めた**（2026-09-18。下の「餅の回り方」）。回収の閃光の値（`FLASH_GAMMA` 0.5・`FLASH_STRENGTH` 0.8）は同じ回答で確定した。
- 原作の結晶の `Material`（`m_crystal_Inst1`）は、餅がメッシュの材質を持つので使わない。

### 餅のモデル（`wasami_mochi_v3`、2026-09-18）

ユーザーの指示（`.claude/references/enemy-wasami-motions.md`）で、WebGL 版が 6,000 三角形・1024² の JPEG に落としたものから、焼いたままの `wasami_mochi_v3` に替えた。**そのまま取り込む**（減らさない・上限を設けない）: シャードは両ゾーンで 679 個あるが全部が同じメッシュとテクスチャ 3 枚を共有するので、個数では増えない。

`MochiSize` 0.825 m はメッシュのいちばん長い軸（X）に掛かる。v3 も旧モデルも X が 1.00 m なので拡縮 0.0825 は変えていない。ただし v3 は上から見て丸い（1.00 × 1.00、旧は 1.00 × 0.86）ので、回っても幅が縮まなくなり、画面では約 1.1 倍に見える。

Zone 1 の待合（`place 0 -157 --yaw -90`、餅 337 個が見える）で測った、入れ替えの前後（`observations/README.md`）:

| | 旧（6,000 三角形・1024²） | v3（101,368 三角形・2048²/4096²） |
| --- | --- | --- |
| フレーム時間（`stat unit` 表示中、`t.MaxFPS 0`） | 12.11 ms | 11.98 ms |
| GPU 時間 | 9.1 ms | 9.1〜9.2 ms |
| VRAM（`stat RHI`） | 2.98 GB / 5.08 GB | 3.18 GB / 5.08 GB |
| プリミティブ | 604.4K | 601.4K |

Nanite が画面の大きさに合わせて三角形を出すので、17 倍の密度でも描画は増えない（プリミティブはむしろ減った）。VRAM は +0.20 GB で、`.claude/guides/performance.md` の目安（6 GB に対して 5 GB 程度まで）に収まる。**このため開発中のテクスチャの上限も設けない。**

### 餅の回り方（本家の結晶と比べて、直さないと決めた。2026-09-18）

**個体ごとの乱数（再生速度 0.05〜0.15 = 10.8〜32.4 °/s、Z 軸のヨー）のままにする。** 本家の `BP_Shard` の `BeginPlay` が `RandomFloatInRange(0.05, 0.15)` を引くので、**乱数そのものが原作の値**（`.claude/guides/original-fidelity.md` の「UE に同じ仕組みがあれば値を写すだけにする」）。最新版の実機で撮れた 1 個の 17.1 °/s（21.0 s で 1 周。パワーの作業のステップ 11a）はその 1 回の引きに過ぎないので、そこには寄せない。軸（Z）と向き（ヨーが増える = 手前の面が画面の左へ流れる）も上の「餅の回転」のとおりで、**C++ は変えていない**。

見比べ（`observations/README.md` の「餅の回り方を本家の結晶と比べた」）: 本家の既存の収録 `orig-shard-spin-long.mkv`（4 m 先・45 秒・10 fps）と同じ測り方で、PIE の v3 の餅を同じ距離（`place 15 385 --yaw -90 --pitch -15`、3.8 m 先のシャード、画面の (2296, 425)〜(2476, 595)、`t.MaxFPS 60`）で 45 秒撮り（449 枚・抜けなし）、`Tools/video_probe.py period` で 1 周を測った。

| | 1 周 | 速さ | 再生速度 |
| --- | --- | --- | --- |
| 本家の実機（結晶 1 個） | 21.0 s | 17.1 °/s | 0.079 相当 |
| 本作（v3 の餅、2026-09-18） | 13.3 s | 27.1 °/s | 0.125 相当 |
| 本作（同じシャード、2026-09-17） | 25.0 s | 14.4 °/s | 0.057 相当 |

- 本作の 2 回の値が違うのは、PIE を始め直すと `BeginPlay` が引き直すため。どちらも範囲（0.05〜0.15）の中にある。
- 本家の 21.0 s は再生速度 0.079 相当で、**「アニメ 1 ループで 2 周」の読み（上の「餅の回転」）の裏づけになる**。360° の読みだと 0.159 になり、原作の乱数の範囲の外に出てしまう。
- 一巡の判定: 一巡で元の絵に戻る差は 1.86、半周ずれた絵との差は 17.5（9.4 倍）。本家の結晶は 13.6 対 17〜20（1.4 倍）で、v3 の餅は顔が横を向くぶん一巡がはっきり分かる（顔が正面に来るのが 2.3〜3.4 s、白い裏へ回るのが 6.7〜10.0 s、13.3 s で戻る）。
- 本家の結晶が回ると**先端が左右に倒れて見える**のは、メッシュの長い軸が Z から傾いているため。上から見て丸い v3 の餅（1.00 × 1.00）には出ない。餅の見た目はもともと原作と違う（ユーザーの決定）ので、ここは合わせない。
- `M_Shard` の色は推定（原作の値は cook で消えた。上の「印の色」）。病院の実機の地図にシャードが写る場面をまだ撮っていない。
- 回収の音の同時発音は、2 つ目で 1 つ目が止まらない（上の「確かめたこと」）。
- 回転のティックは 340 個ほどのシャードすべてで走るが、描かれていないシャードは回転を書かない。
- ゲームモードの `Check Shards`、`bDisabled`/`Enable` は未実装（上の「役割」）。
- **閃光の材質は推定**（上の「閃光の材質」）。見え方は本家と見比べていない（パワーの作業のステップ 11 では撮らなかった。作業一覧の項目 3 で色と強さを変えるときに合わせる）。
- **エディタで最初の閃光だけ、描画が約 0.6 秒止まる**。初めて使う粒子の材質のシェーダー（3 件）をエディタがその場でコンパイルするため（ログの `LogShaderCompilers` のジョブ 0.5〜0.6 秒）。ゲームの時間は止まらず、2 回目からは止まらない。2026-09-17（作業一覧の項目 3 のステップ 1）に、シェーダーが作られた後のエディタのセッションで撮ったときは、描画は止まらず、**最初の閃光の粒子が描かれなかった**（灯だけが床を少し照らした）。2 回目からは描かれた。見え方を撮るときは、先に 1 回捨ての回収をする。パッケージしたゲームではシェーダーが先に作られているので起きないはず。
- 本家の数の読み取りは文字列を整数にする（`Conv_StringToInt`）ので、1,000 以上で桁区切りが入ると 1 と読む癖がある。本作は整数を持つので起きない（病院は 342 以下）。
- テストのワールドにはプレイヤーがいないので、回収はタブレットが無いところで止まる（本家と同じ）。回収の結果は PIE で確かめる。

## 確かめたこと（2026-09-17、PIE、Zone 1 の −Y へ延びる廊下。シャード 331・330・4・_2・5 が X≈0 に並ぶ）
- 配置: Zone 1 に 337、Zone 2 に 342。単独で置いていたシャードの灯は外れ、Zone 1 の灯は 1,120 → 783。シャードの部品は Movable なので焼き込みはそのまま。PIE の開始でシャード 337・画面の数「337」。
- 見た目: 廊下の中央に餅が並んで見え、近づくとワサミの顔の餅。**v3 は顔が横を向き、暗い髪が上に乗る**ので、ヨーで回すと顔が正面に来ては裏へ回る（2026-09-18。旧モデルは顔が真上を向いていて回って見えにくかった）。描かれている 3 つのヨーを 3.5 秒おいて読むと、毎秒 27.3・23.1・16.1°（範囲 10.8〜32.4 の中）。地図の印は本編では描かれない（`WasRecentlyRendered` が偽）。
- 地図: タブレットを上げると、廊下に並ぶシャードが紫の四角で出る（上の「印の色」）。
- 触れて回収（前進の入力で歩いてシャード 331 へ）: プレイヤーの中心がシャードから 97 cm（カプセルの半径の和 99.6 cm）に来たフレームで、アクタが消え、画面の数が 337 → 336。同じフレームの `Count Shake` は約 0.03 秒ぶん進んだ値（数の移動 (−8.4, 6.2)・拡縮 1.06・閃きの α 0.236。プレイヤーの画面の更新が回収より後のフレーム順のため）で、0.09 秒後に数の変換が元の (0, −12)・1.0 に戻り、α は 0 のまま。揺れは FOV が最大 +0.78°・ロールが最大 0.39° で、0.1 秒で 0 に戻った。
- 音（`ListWaves`）: `Soul_Shard_Pickup_v2` が 1 つ、音量 0.47〜0.51 で鳴る。0.65 × Cue の既定の `VolumeMultiplier` 0.75（UE 5.8 も 0.75。原作の書き出しは既定と同じ値を省くので原作も 0.75。書き出した Cue 126 個のうち 20 個だけが別の値を持つ）× Modulator の音量 0.95〜1.05 = 0.46〜0.51 と合う。
- 引き寄せ（プレイヤーは (0, −157) に立ったまま、4.4 m 先の 330 と 10.4 m 先の 4 に Python から `activate()`）: どちらも初めはほとんど動かず（0.35 秒で 8 cm と 52 cm）、最後に一気に寄り、プレイヤーに触れた所で回収された（4 が 0.64 秒、330 が 0.84 秒。再生速度の乱数で遠い方が先に着いた）。高さは 0 のまま。数は 336 → 335 → 334。
- **同時発音の差（原因不明。2026-09-17 のユーザーの回答でこのままにする）**: 0.2 秒差で 2 つ回収すると、1 つ目の音は止まらず音量が 0.5 倍（0.24）になって鳴り続け、2 つが重なった（2 回試して同じ）。`OnlyFew` は `MaxCount` 1・`StopOldest`・`VolumeScale` 0.5 を原作どおりに写してあり、UE 5.8 の `SoundConcurrency.cpp` を読む限りは古い方が止まるはず。原因は特定していない。テレキネシスでまとめて回収したときに聞こえ方が変わりうるが、本家の収録（パワーの作業のステップ 11a）には音が無く、聞き比べていない（ユーザーは「このままでよい」とした）。2026-09-17（ステップ 10a）: テレキネシスで 8 個が約 0.24 秒の間に回収されたとき、8 つの音がどれも止まらず、音量が新しい順に 0.47・0.24・0.13・0.06・0.03・0.01・0.01・0.00（新しい音が来るたびに古い音が 0.5 倍）で重なった。本家の `OnlyFew` の書き出しは `MaxCount` 1・`StopOldest`・`VolumeScale` 0.5 だけで、ほかの値（`VoiceStealReleaseTime` など）は既定のまま。

- 回収の閃光（2026-09-17、同じ廊下。Python から 4 m 先のシャードの `collect`、続けて歩いて 2 つに触れる。60 fps の gdigrab の収録と毎フレームの記録）: 回収のフレームに `P_ky_flash3` の部品が `WorldSettings` に 1 つ出て、位置は結晶の位置（高さ 97.1 cm）、拡縮 0.2、約 1.0 秒で消えた（3 回とも）。4 m 先からは、虹色の星と半透明の円（最初の絵）、紫の衝撃波の円・明るい中心の星・横の帯・床を照らす灯が 3〜4 フレームで消え、床の明るさが少し残った。歩いて触れたときは、閃光がカメラの約 1 m 先・60 cm 下に出るので、画面の下に紫の衝撃波の弧と細い光の筋が 5 フレームほど見えた。最初の 1 回だけ描画が約 0.6 秒止まった（上の「既知の制約」）。

## 確かめたこと（2026-09-18、PIE、Zone 1。死亡から LAST CHECKPOINT までの通し）
- `Wasami.ResetSave` → `open L_Hospital_Zone1` で `04_Start`（エレベーターの前）から始め、`Wasami.Checkpoint 5` で右下に SAVING PROGRESS。`Wasami.Kill` を 3 回: 1 回目・2 回目は死亡画面（ドクロ 3 → 2、2 → 1、赤い揺れ）の後に `05_Start`（待合）で開き直し、3 回目で YOU ARE DEAD と 3 つのボタン。LAST CHECKPOINT → S ランクの警告 → YES で `05_Start` から再開し、ライフ 3・セーブのチェックポイント 5・死亡数 3・`Last Checkpoint Warning` 真。死亡から開き直しまで約 7 s、ゲームオーバーからボタンまで約 3 s。収録は `Intermediate/DesktopAgent/shots/step7-flow.mkv`（git の外）。

## テスト（`Tests/WasamiShardTests.cpp`）
- `Wasami.Shard.PullCurve` … `Alpha` の値（0 / 0.375 / 0.75 / 1 秒）、ExpoIn の位置（Alpha 0 で元の位置、0.5 で 1/32、0.9 で 1/2、1 でプレイヤーの X・Y、高さは元のまま）、回る速さ（0.05 で 10.8、0.15 で 32.4 °/s）。
- `Wasami.Shard.Actor` … 一時的なゲームのワールドに置いて、閃光の既定のパス（`CollectFlash`）、カプセル（半径と半高さ 49.57、高さ約 100 cm、`WorldStatic`・`Custom`・QueryOnly・Pawn とワールドへ Overlap・重なりのイベントあり）、灯（位置・強さ 175・単位なし・半径 200・色・影なし・Movable）、餅（0.825 m・97.085 cm・描画距離 3000・カスタム プリミティブ データなし）、印（20 m 上・拡縮・当たりなし）、再生速度の範囲。`Activate` でその場の位置の更新、再生速度の範囲、0.45 の時点でわずかにしか寄らないこと、終わりに原点へ着いて止まること、プレイヤーがいないので破棄されないこと。
- `Wasami.Tablet.CountShake`（03 記録）。
- `Tests/WasamiGameFlowTests.cpp`: `Wasami.GameFlow.Lives`（3 で始まり、0..6 に Clamp、`ResetLives` で 3。`ShardKey` の 0 の方への切り捨て、同じ整数の位置は 1 つ、`ForgetCollectedShards`）、`Wasami.GameFlow.Save`（スロット名 `structSlot`、全欄のメモリ上の往復）、`Wasami.GameFlow.RemoveShards`（3 つ置いて、切り捨てて一致する 2 つが消え、残り 1 を返す）、`Wasami.GameFlow.GameMode`（テスト用のスロットで BeginPlay がセーブを作って書く、1 秒の時間・止めている間は数えない、`SaveCheckpoint(5)` がスロットに書いて時間を足し 0 に戻す、`DeathEvent` の DoOnce と `ResetDeath`・連続回収の最高、作り直したゲームモードがスロットを読む、Zone でないワールドの `GetStartCheckpoint` はセーブの値のまま）、`Wasami.GameFlow.Checkpoints`（`ZoneOf`、`PlayerStartTagFor` の 7 つと表に無い値、`DeathScreenLevelFor` の 4 通り）、`Wasami.GameFlow.Saving`（09 記録の SAVING PROGRESS の `init` の値と 3 s で外れること、黒のフェードの両端と速さ 10 で 0.5 s）、`Wasami.GameFlow.Loading`（09 記録の読み込み画面の `FadeIn` の値・2.5 s からの逆再生・3.5 s で外れること・紋章は 7 番の `loader_wasami` だけ）。

## 変更履歴
- 2026-09-19: テスト `Wasami.GameFlow.Loading` に紋章の既定（7 番だけ `/Game/Wasami/UI/loader_wasami`）を足した（09 記録）
- 2026-09-19: `GetPlane()`（地図の印）を足した（Zone 2 の階ごとの地図が使う。03 記録）
- 2026-09-18: テスト `Wasami.GameFlow.Loading`（読み込み画面。09 記録）を足した（作業一覧の項目 6 のステップ 5）
- 2026-09-18: ゲームモードに `Check Shards`（`Collect Shard`・0.05 s 後の全回収の判定）とゾーンの流れの生成、`Zone2LevelName`、デバッグの `Wasami.CollectShards`・`Wasami.Trigger` を足し、シャードの回収が `Check Shards` を呼ぶようにした。目的の既定を空にした（作業一覧の項目 6 のステップ 1。流れは 11 記録）
- 2026-09-18: 作業一覧の項目 5（ゲームの流れの土台）を終えた。死亡から LAST CHECKPOINT までを PIE で通して確かめた（上の「確かめたこと」の 2026-09-18）
- 2026-09-18: 病院の Zone のレベル BP の受け持ちをゲームモードに足した: `DeathEvent` が死亡画面を出してゲームを止める、セーブのチェックポイントの PlayerStart から出す（Zone 1 の 0 は 4 を書く、Zone 2 の 0 は Zone 1 を開く）、`SaveCheckpoint` の SAVING PROGRESS、開いたときの黒からの明け、デバッグのコンソールコマンド 4 つ（テスト 2 本。PIE で `Wasami.Checkpoint 5` → 別の場所で 2 つ回収 → `Wasami.Kill` → 死亡画面 → `05_Start` で再開〈ライフ 2・死亡数 1・シャード 335〉、`Wasami.Checkpoint 7` → Zone 2 の `PlayerStart_1`、`Wasami.ResetSave` → Zone 2 を開くと Zone 1 の `04_Start`。作業一覧の項目 5 のステップ 5）
- 2026-09-18: ゲームインスタンス（`UWasamiGameInstance`: ライフ 3・0..6、回収済みのシャードの記憶）とセーブ（`UWasamiSaveGame`、`structSlot`）を足し、ゲームモードに死亡の受け口・時間・チェックポイントの保存・開き直したときの回収済みのシャードの除去を足した。シャードの回収がゲームインスタンスに位置を覚えさせる（テスト 4 本。PIE で 2 つ回収 → `open L_Hospital_Zone1` → 337 が 335 になり、回収した 2 つが無いのを確かめた。作業一覧の項目 5 のステップ 3）
- 2026-09-18: 餅の回り方を本家の結晶と同じ撮り方で見比べ、**直さないと決めた**（速さは個体ごとの乱数のまま。上の「餅の回り方」。作業一覧の項目 22 のステップ 4。コードは変えていない）
- 2026-09-18: 餅のモデルを `wasami_mochi_v3` に替えた（6,000 → 101,368 三角形、テクスチャは PNG の 2048²・2048²・4096²。下の「餅のモデル」。作業一覧の項目 22 のステップ 3）
- 2026-09-18: 餅の紫の明滅を外し、大きさを 0.55 → 0.825 m（1.5 倍）にした（`M_DD_WasamiMochi` の自己発光は `Glow` だけ、`PulsePhaseData` と `BeginPlay` の乱数を削除、テスト。作業一覧の項目 22 のステップ 2）
- 2026-09-17: 要確認の回答を反映した（閃光の係数は確定〈`dd_shards` の `TODO(仮)` を外した〉、同時発音の差はこのまま、餅の明滅は作業一覧の項目 22 で外す。コメントと記録だけで、値は変えていない）
- 2026-09-17: 回収の閃光の色の係数を「最大 ^ 0.5 × 0.8」にした（線形 0.6 では衝撃波の輪が見えなくなったため。PIE で 3 通りを測った）
- 2026-09-17: 回収の閃光を本作の紫でやや弱い `P_WasamiShardFlash` に替えた（`dd_shards.make_flash`、`CollectFlash` の既定のパスとテスト。作業一覧の項目 3）
- 2026-09-17: 餅の紫の明滅を足した（`M_DD_WasamiMochi` の自己発光、`BeginPlay` で位相の乱数、テスト。作業一覧の項目 3）
- 2026-09-17: 閃光の材質を作る繰り返しと小道具を `dd_assets` へ移した（`make_flash_materials` は `estimated_materials` を呼ぶ。作るものは同じで、取り込み直して `flash_materials` 13・静的マスク G / R を確かめた）
- 2026-09-17: 回収の閃光 `P_ky_flash3` を足した（`Collect` の揺れの後に出す。素材の取り込みは `dd_shards.py` のテクスチャ 4・推定のマスター 5・インスタンス 8・粒子 1）
- 2026-09-17: 初版（シャードの最小限: `AWasamiShard`〈ワサミ餅・灯・カプセル・地図の印・回転〉、回収、引き寄せ、素材の取り込み `dd_shards.py`、配置、テスト）
- 2026-09-18: Zone 2 の 7 の PlayerStart を独房の `PlayerStart_Cell` にした（飛ばした独房の場面がプレイヤーを移す所。作業一覧の項目 6 のステップ 7a、11 記録）
