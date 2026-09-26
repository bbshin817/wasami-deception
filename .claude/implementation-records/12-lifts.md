---
title: リフト（Zone 2 の乗ると上がる床・角のリフト・ガレージリフト）
sources:
  - Source/wasami_deception/WasamiLift.h
  - Source/wasami_deception/WasamiLift.cpp
  - Source/wasami_deception/Tests/WasamiLiftTests.cpp
  - Source/wasami_deception/WasamiGarageLift.h
  - Source/wasami_deception/WasamiGarageLift.cpp
  - Source/wasami_deception/Tests/WasamiGarageLiftTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_skeletal.py
updated: 2026-09-26
---

# リフト（Zone 2 の乗ると上がる床・角のリフト・ガレージリフト）

## 役割
Zone 2 の迷路の 2 つの階をつなぐ床。本家の `pak_reference_2` の `Blueprints/06_Hospital/Lifts/Zone2` を写した。`AWasamiLift`（本家の `BP_06_Lift` = `BP_06_LiftBase` の子。置かれているのはその子の `BP_06_Lift_03`〈長い床〉8 台と `BP_06_Lift_04`〈広い床〉2 台）は、キャラクターが乗っている間 535 cm 上がり、いなくなると下りる。`AWasamiCornerLift`（本家の `BP_06_LiftBase_Corner`、角に 5 台）は、プレイヤーのいる階で待ち、1 s 立つともう一方の階へ動く。作業一覧の項目 6（ゾーンの進行）のステップ 8a で作った。ガレージリフト（`AWasamiGarageLift`、本家の `BP_06_GarageLift`。骨入りのメッシュの車のリフト）は、プレイヤーが乗っている間アニメで台が約 3 m 上がり、降りると 0.2 s で下りる。Zone 2 の車庫に 2 台、Zone 1 の駐車場の救急車の後ろに `AWasamiGarageLiftZone1Special`（ナースが近いと上がらない）が 1 台。ステップ 8b1 でメッシュとアニメを取り込み（下の「作るアセット」）、8b2 でアクタを作った。

## 公開インターフェース
- `AWasamiLiftBase`（`AActor`、抽象）: 本家では `BP_06_LiftBase_Corner` は `BP_06_LiftBase` の子でなく写し（`Actor` の子）なので、2 つが共通に持つものをここに置いた。
  - `TopLocation` 535（本家の `Top Location`。両クラスの既定で、置かれたものは上書きしない）。
  - `IsMoving()`（`IsMoving?`）・`IsCharacterOnTop()`（`Character on Top?`）・`GetHeight()`（`LiftMesh` の相対 Z）・`UpdatePosition(DeltaSeconds)`（`Update Position`。Tick が呼ぶ）・`IsMovementSoundOn()`。
  - 部品の取り出し `GetLiftMesh()`・`GetLiftCollision()`・`GetLiftCollisionOverlap()`・`GetBottomCollision()`・`GetLiftCollision1()`・`GetMoveLocation()`・`GetAudio()`・`GetMovementAudio()`。定数 `SpeedWithCharacter` 0.5・`SpeedWithoutCharacter` 1.0（Top Location の何倍 / s）。
- `AWasamiLift`: `OnPlayerOverlap`（動的マルチキャスト。本家の `Player Overlap`: プレイヤーが床に乗った）、`NotifyOverlap(Other)`（`LiftCollisionOverlap` の重なりの始まり。テストが直接呼ぶ）。
- `AWasamiCornerLift`: `bPlayerForceMovement`（`Player Force Movement`）・`bGoUp`（`GoUp?`）、`IsPlayerOnTopFloor()`（`What Floor is Player On?`）・`IsDoubleCheckPending()`・`DoubleCheck()`、`NotifyActorBeginOverlap` / `EndOverlap`（`AActor` のもの。本家の `ReceiveActorBeginOverlap` / `EndOverlap`）。定数 `TopFloorHeight` 610（ワールドの Z）・`DoubleCheckDelay` 1。

- `AWasamiGarageLift`（`AActor`）: `IsPlayerOverlapping()`（本家の `Player Overlapping?`、BlueprintPure・仮想）、部品の取り出し `GetSkeletalMesh()`・`GetBox()`・`GetOverlapBox()`・`GetAudio()`・`GetAudio1()`、定数 `PlatformBone`（`joint4`）。ソフト参照 `MeshAsset`・`UpSound`・`DownSound`・`SoundAttenuation`。
- `AWasamiGarageLiftZone1Special`: `bNurseNear`（本家の `NurseNear`。真なら `IsPlayerOverlapping` が偽。Zone 1 の流れがナースが `TriggerVolume_1` に入ると真にし、戻さない。11 記録。2026-09-19 の PIE で、ナースが箱を通った後はプレイヤーが乗っても上がらなかった）。
- `UWasamiGarageLiftAnimInstance`（ネイティブの AnimInstance。メッシュの `AnimClass`）: `bPlayerOn`（`PlayerOn?`）、`IsInPlayerOn()`（状態機械の今の状態）・`GetPlayerOnWeight()`（PlayerOn の重み）・`GetPlayerOnTime()`（PlayerOn のシーケンスプレーヤーの時刻）・`GetLength()`（アニメの長さ）、定数 `CrossfadeDuration` 0.2・`UpFadeOutSeconds` 0.25。

## 内部構造と処理の流れ
- 部品（本家の SCS。箱は `UBoxComponent` の既定の 32 cm を拡縮）: `DefaultSceneRoot` → `LiftMesh`（動く床のメッシュ、`NoCollision`）→ `LiftCollision`（BlockAll・WorldStatic、拡縮 (4.5524, 4.5861, 1.1901) = 291 × 294 × 76 cm）→ その子の `LiftCollisionOverlap`（既定の OverlapAllDynamic、(0, 0, 63.51)。親の拡縮で床と同じ広さ、床の上面から 76 cm の厚さ）と `BottomCollision`（範囲 (32, 32, 256.75)・(0, 0, −288.75)、BlockAll。親の拡縮で床の下 611 cm。本家の説明「プレイヤーがリフトの中に落ちないように」）。`LiftMesh` の子に `MoveLocation`（(0, 0, 149.54)。床と一緒に動く。Zone 2 の迷路のナースが階の違うプレイヤーを追うときに向かう所: 07 記録の「Zone 2 の迷路の型」。読むのは `AWasamiLift` のものだけ）。根の子に `LiftCollision1`（BlockAll、拡縮 (4.5524, 4.5861, 1.1901)。構築スクリプトで (0, 0, Top Location)）、`Audio`（`DD_TT_GarageLift_Down`・`MonkeyAttenuation`、自動で鳴らない）、`MovementAudio`（`DD_TT_Lift_Loop`・0.7・ピッチ 1.5・`01_Lobby_Attenuation`、自動で鳴らない）。音と減衰は `BeginPlay` でソフト参照から入れる（`WasamiAssets.h`）。
- **メッシュとクラスごとの箱の大きさはクラスが入れない**。レベルの組み立て（`dd_level._flow` の `LIFT_CLASSES`、01 記録）が、`BP_06_Lift_03` に `hospital_zone_02_lifts_lift_03`・`LiftCollision` (4.5524, 9.1536, 1.1901)・`LiftCollision1` (4.5524, 10.5057, 0.6494)、`_04` に `lift_04`・(8.0683, 4.5422, 1.1901)・(8.6070, 4.5861, 1.1901)、角に `lift_01`（箱は既定）を入れる（子の ICH の値）。メッシュの材質はメッシュのもの（`M_06_Hospital_Lift`・`M_06_Hospital_Lift_02`）。
- Tick → `UpdatePosition`: 目標 = `GetTargetHeight()`、速さ = `Character on Top?`（`LiftCollisionOverlap` に `ACharacter` が重なる）なら Top × 0.5、でなければ Top × 1.0 cm/s（535 cm を上りは 2 s、下りは 1 s）。`LiftMesh` の Z を `FMath::FInterpConstantTo` で進める。`IsMoving?` = 動かした後の高さからもう 1 歩進めた値が目標と違う（本家の Blueprint が純粋ノードをもう一度評価する。着く 1 歩前に止まったと見なす）。
- 音: 本家の DoOnce 2 つ（止まる側は閉じて始まる）が互いを開け直すので、`IsMoving?` の変わり目で鳴る: 動き出すと `MovementAudio->FadeIn(0.5, 1, 0)` と `Audio->Play(0)`、止まると `FadeOut(0.5, 0)` と `Audio->Play(0)`。始めから止まっている床は鳴らない。
- `AWasamiLift`: 目標 = キャラクターが乗っていれば Top、でなければ 0。`BeginPlay`（本家の `BP_06_Lift` の `ReceiveBeginPlay`）で `LiftCollision1` を消す。`LiftCollisionOverlap` の重なりの始まりで相手が `AWasamiPlayerCharacter`（本家の `BP_DD_PlayerCharacter` へのキャスト）なら `OnPlayerOverlap`。Zone 2 のレベル BP は `Maze Trigger Start` の 1 s 後の `Setup Bierce Lift Quip` で全部の `BP_06_LiftBase` にこれを結び、`Bierce Lift Quip`（DoOnce・1 s 後に `Bierce_TormentTherapy_Gameplay_07`）を流す（項目 20。11 記録の `OnMazeTriggerStart` のコメント）。
- `AWasamiCornerLift`: 目標 = `Player Force Movement` なら `GoUp?` の階、でなければプレイヤーのいる階（プレイヤーの Z > 610 なら Top、プレイヤーがいなければ 0）。アクタの重なりの始まりで相手がプレイヤーなら 1 s の `Double Check` のタイマー（置き直し）。`Double Check`: まだ `LiftCollisionOverlap` にプレイヤーが重なっていれば `Player Force Movement` 真・`GoUp?` = 上の階にいない・`Audio` を鳴らす。重なりの終わりでプレイヤーならタイマーを止め `Player Force Movement` 偽。`LiftCollision1` は消さない。
- 写さないもの: `Preview Top`（エディタで床を上に見せる構築スクリプトの分岐）、`NavModifier`（`NavArea_Default`。通れる所を通れるままにするだけで道を変えない）と箱の `AreaClass`（動く障害物〈`bDynamicObstacle`〉のときだけ効く。2026-09-19、項目 7 のステップ 1 で確かめた。01 記録の「ナビゲーション」）。床の箱は BlockAll の当たりとして道の形に入る（エンジンの既定）。`LiftCollisionOverlap` は何も遮らないのでナビゲーションに入れない。

### ガレージリフト（`AWasamiGarageLift`）
- 部品（本家の SCS）: `DefaultSceneRoot` → `SkeletalMesh`（`hospital_garage_lift_anim`、拡縮 30、当たりはエンジンの既定の NoCollision、`AnimClass = UWasamiGarageLiftAnimInstance`）→ 骨 `joint4` に `Box`（ロール −90.0002・拡縮 (0.2031, 0.2734, 0.00953)。メッシュの 30 倍で 390 × 525 × 18.3 cm の台。QueryAndPhysics、WorldDynamic で Pawn だけ Block）と `Overlap Box`（(0, −2.333, 0)・ロール −90.0002・拡縮 (0.2031, 0.2734, 0.07127)。台の上 2〜139 cm。Pawn だけ Overlap、ナビゲーションに入れない）。根に `Audio`（`DD_TT_GarageLift_Up`）・`Audio1`（`DD_TT_GarageLift_Down`）、どちらも `01_Lobby_Attenuation`・自動で鳴らない。箱の当たりは本家の Custom の一覧どおりエンジンのチャンネルを Ignore にし、並ばない Pawn は既定の Block（`Overlap Box` は Overlap）、プロジェクトの Teleport は既定の Overlap のまま。
- メッシュは `PreRegisterAllComponents` と `OnConstruction` でソフト参照から入れる（出したアクタは `OnConstruction` の前に部品を登録するので、登録の前に入れないと箱の `joint4` が見つからず警告が出る）。音と減衰は `BeginPlay`。アクタは Tick しない（台を動かすのはメッシュのアニメ。骨に付いた箱は骨の更新で動き、エンジンの既定 `AlwaysTickPoseAndRefreshBones` なので見えないときも動く）。
- `Player Overlapping?`: `Overlap Box` が最初のプレイヤーのキャラクターと重なっているか。`_Zone1_Special` は `NurseNear` なら偽、でなければ親のもの。
- **アニメ（`UWasamiGarageLiftAnimInstance`、本家の `hospital_garage_lift_anim_Skeleton_AnimBlueprint`）**: 本家の ABP の状態機械 1 つを C++ で持つ（07 記録の敵ワサミと同じ形: `NativeUpdateAnimation` で状態を進め、Proxy の `Evaluate` がアニメの姿勢を取り出して混ぜる）。
  - 毎更新、`PlayerOn?` = 持ち主（`AWasamiGarageLift` へのキャスト）の `Player Overlapping?`（本家の `BlueprintUpdateAnimation`）。
  - 状態 `Default`（初めの状態。シーケンスプレーヤーの速さ 0・ループ = アニメの 0 コマ、台が下）と `PlayerOn`（速さ 1・ループしない = 2.4583 s で上がり、最後のコマで止まる）。遷移は `PlayerOn?`（入る）と `!PlayerOn?`（出る。ABP の CopyRecords の `LogicalNegateBool`）、どちらも 0.2 s の HermiteCubic の混ぜ（07 記録の `FWasamiStateBlend`。本家の遷移の積み重ねを、混ぜ始めの重みから入る状態へ 1 まで、に縮めたもの）。最初の更新は遷移しない（`bSkipFirstUpdateTransition`）。
  - `PlayerOn` の時刻は、その状態の重みが 0 のときに入ったときだけ 0 に戻る（エンジンの `SetState` は、重みの残っている状態を初期化しない）。降りて 0.2 s 以内に乗り直すと、上がり切った台は上がったまま。重みがある間だけ進む。
  - 通知（本家の状態の `StartNotify` / `EndNotify` = `Player On Event` / `Player Off Event`）: `PlayerOn` に入ると持ち主の `Audio->Play(0)`、出ると `Audio->FadeOut(0.25, 0)` → `Audio1->Play(0)`。
  - Proxy: 重み 0 なら 0 コマ、1 なら `PlayerOn` の時刻、間なら 2 つを `FAnimationRuntime::BlendTwoPosesTogether` で混ぜる。スケルトンの無いアニメは参照姿勢。
  - アニメを読むのはゲームのワールドだけ（07 記録と同じ）。エディタのレベルでは参照姿勢で、**参照姿勢の台は上にある**（`Box` が Z 333 cm。0 コマは 9.3 cm、上がり切ると 316.4 cm）。UE の `bUpdateAnimationInEditor` の既定は偽なので、本家のエディタでも参照姿勢と見る。
- 写さないもの: `Box` の `AreaClass`（`NavArea_Obstacle`。動く障害物のときだけ効く）と `PhysMaterialOverride`（`PhysMat_Metal`。面を読むものが無い）、`SkeletalMesh` の `AnimationData.AnimToPlay`（ABP のモードでは使われない）と置かれた部品の `EndTickGroup`（`TG_PostPhysics`）。
- 置き場所（`dd_level._flow` の `GARAGE_LIFT_CLASSES`。01 記録）: Zone 1 `hospital_garage_lift_anim_Anim_2`（`_Zone1_Special`、(11248.9, −21154.9, 0)・ヨー 0。救急車〈(11245, −20080)〉の後ろ）、Zone 2 `BP_06_GarageLift2`（(−12851.3, −5038.3, 800.1)・ヨー −90）・`BP_06_GarageLift_2`（(−10348.9, −5446.0, 0)・ヨー 0）。どれも置かれた値の上書きは無い。

## 作るアセット
- メッシュ `/Game/DD/Meshes/06_Hospital/hospital_zone_02_lifts_lift_01`・`_03`・`_04` とその材質 `/Game/DD/Materials/06_Hospital/M_06_Hospital_Lift`・`M_06_Hospital_Lift_02`（テクスチャ `hospital_lift_01_D/N/S`・`hospital_lift_02_D/N/S`）: ステージの素材（前処理の `CLASS_MESHES` → `WasamiStageTools.import_dd_stage_assets`。01 記録）。
- ガレージリフトの骨入りのメッシュ `/Game/DD/Meshes/06_Hospital/hospital_garage_lift_anim`（骨 4 本 `joint1`〜`joint4`、`_Skeleton`・`_PhysicsAsset` は取り込みのもの）とアニメ `hospital_garage_lift_anim_Anim`（24 fps・59 コマ・2.4583 s）: `dd_skeletal.import_garage_lift`（`dd_gimmicks.import_all` → `WasamiDDTools.import_dd_gimmicks`。01 記録）。本家の psa を読んで glTF のアニメにして取り込む。動くのは `joint2`（X 11.617 → 15.498）と `joint3`（X 0.041 → 6.401）の平行移動だけで、`joint1` は Z −12.094 で X を上に向け、台の `joint4` は `joint3` の 0.745 先（部品の拡縮 30 で、台は約 9.5 cm → 316.7 cm に上がる）。材質はスロットの名前でステージの `M_06_Hospital_MetalPanel_04`・`M_06_Hospital_Concrete_06_Painted1`・`M_06_Hospital_MetalBrushed_02`・`/Game/DD/Materials/07_FunPlace/M_07_TP_DiamondPlate`（前処理の `CLASS_MATERIALS`）、その親の `M_DD_Substance` に `used_with_skeletal_mesh`。
- 音 `/Game/DD/Audio/06_Hospital/DD_TT_GarageLift_Down`（0.883 s・0.55）・`DD_TT_GarageLift_Up`（5.665 s。ガレージリフト）・`DD_TT_Lift_Loop`（26.87 s・ループ）と減衰 `MonkeyAttenuation`・`01_Lobby_Attenuation`: `dd_gimmicks.import_lifts`（`WasamiDDTools.import_dd_gimmicks`。08 記録）。

## 原作データの根拠
- 処理: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/Lifts/Zone2/BP_06_LiftBase.txt`（`Update Position`・`Character on Top?`・Tick の DoOnce 2 つ・重なりの `Player Overlap`・構築スクリプト）、`BP_06_Lift.txt`（`ReceiveBeginPlay` で `LiftCollision1` を消す、構築スクリプト）、`BP_06_LiftBase_Corner.txt`（`What Floor is Player On?` の 610、`Double Check` の 1 s、`ReceiveActorBeginOverlap` / `EndOverlap`）。
- 部品と値: `_assets/…/Lifts/Zone2/BP_06_LiftBase.json`・`BP_06_LiftBase_Corner.json`（SCS の部品・音・減衰・箱の拡縮と位置、`Top Location` 535）、`BP_06_Lift.json`（`LiftCollision1`）、`BP_06_Lift_03.json`・`_04.json`（ICH のメッシュと箱の拡縮）。
- ガレージリフトのメッシュとアニメ: `pak_reference_2/_meshes_gltf/Meshes/06_Hospital/hospital_garage_lift_anim.gltf`（参照の姿勢）・`_anims_psa/Meshes/06_Hospital/hospital_garage_lift_anim_Anim.psa`（キー）・`_assets/…/Meshes/06_Hospital/hospital_garage_lift_anim_Anim.json`（`NumFrames` 60・`SequenceLength` 2.4583）・`hospital_garage_lift_anim.json`（材質 4 の import）・`_PhysicsAsset.json`（`joint1` のカプセル 1 つ）。
- 置き場所: `pak_reference_2/_levels/06_Hospital_Zone_02.full.json`（前処理の `stage_ue.json` の `actors`。15 台とも Top Location の上書きなし。`LiftCollision1` は構築スクリプトの (0, 0, 535)）。ガレージリフトは `_levels/06_Hospital_Zone_01.full.json`・`_02.full.json` の `BP_06_GarageLift*`（`DefaultSceneRoot` の位置と回転だけ）。
- ガレージリフトのアクタ: `_bytecode/DDeception/Content/Blueprints/06_Hospital/Lifts/Garage/BP_06_GarageLift.txt`（`Player Overlapping?`）・`BP_06_GarageLift_Zone1_Special.txt`（`NurseNear` なら偽、でなければ親）、`_assets/…/Lifts/Garage/BP_06_GarageLift.json`（SCS: 部品・`AttachToName` `joint4`・箱の当たりと変換・音と減衰・メッシュの拡縮 30 と `AnimClass`）。
- ガレージリフトの ABP: `_bytecode/DDeception/Content/Meshes/06_Hospital/hospital_garage_lift_anim_Skeleton_AnimBlueprint.txt`（`BlueprintUpdateAnimation` → `PlayerOn?`、`AnimNotify_Player On Event` → `Audio.Play(0)`、`AnimNotify_Player Off Event` → `Audio.FadeOut(0.25, 0, Linear)` → `Audio1.Play(0)`）、`_assets/…/Meshes/06_Hospital/hospital_garage_lift_anim_Skeleton_AnimBlueprint.json`（`BakedStateMachines`: 状態 2・遷移 0.2 s HermiteCubic・`PlayerOn` の `StartNotify` 0 / `EndNotify` 1、`EvaluateGraphExposedInputs` の遷移の条件、シーケンスプレーヤー 2 つの速さとループ、`bSkipFirstUpdateTransition`）。
- ナースが近いとき: `_bytecode/DDeception/Content/06_Hospital_Zone_01.txt` の `TriggerVolume_1` の重なり（`BP_06_ReaperNurse` なら `NurseNear` 真）と `Check Lift Nurses`（11 記録）。
- Bierce の一言: `_bytecode/DDeception/Content/06_Hospital_Zone_02.txt` の `Setup Bierce Lift Quip`・`Bierce Lift Quip`（`python Tools/dd/bp_flow.py … 'Setup Bierce Lift Quip'`）。

## 依存関係
- 自前: `AWasamiPlayerCharacter`（`Player Overlap` の相手のクラス。02 記録）、`WasamiAssets.h`。
- 使う側: レベルの組み立て（`dd_level._flow`。01 記録）、Zone 2 の流れ（項目 20 で `OnPlayerOverlap` を結ぶ。11 記録）。
- エンジン: `UBoxComponent`・`UStaticMeshComponent`・`UAudioComponent`（`FadeIn`・`FadeOut`）、`FMath::FInterpConstantTo`、`UGameplayStatics::GetPlayerCharacter`・`GetWorldDeltaSeconds`、`FTimerManager`、`ACharacter`。ガレージリフトは `USkeletalMeshComponent`（骨に付けた部品）、`UAnimInstance`・`FAnimInstanceProxy`、`UAnimSequence::GetAnimationPose`、`FAnimationRuntime::BlendTwoPosesTogether`、07 記録の `FWasamiStateBlend`（`WasamiEnemyAnimInstance.h`）。
- ガレージリフトの使う側: レベルの組み立て（`GARAGE_LIFT_CLASSES`。01 記録）、Zone 1 の流れ（`OnNurseLiftTrigger` が `bNurseNear` を書く。11 記録）。

## 既知の制約・注意点
- **角のリフトの上で立ち止まると、床が約 70 cm 沈んでは戻るのを約 1.4 s ごとに繰り返す**（本家のコードどおり）: 上の階では `LiftCollision1` が床と同じ所に残るので、1 s 後の `Double Check` で床が下りても、プレイヤーは `LiftCollision1` の上に残る → 重なりが切れて `Player Force Movement` 偽 → プレイヤーの階（上）へ戻る → また 1 s 後…。上の階から角のリフトで下りることはできない。下から上るときは、床に乗ったプレイヤーが `LiftCollision1` を抜けて上の階に着く（PIE で Z 90 → 626）。本家の実機で同じかは確かめていない（作業一覧の項目 28 の後回しの一覧）。
- **歩き（300 cm/s）で下の階から長い床に近づくと、床が先に上がって段差になり、乗れないことがある**: `LiftCollisionOverlap` は床と同じ広さなので、カプセル（半径 50）が縁の 50 cm 手前で重なった時点で上がり始める（267.5 cm/s）。段差が `MaxStepHeight`（45）を超えると壁になり、縁に触れたまま床が 160〜180 cm で上下を繰り返す。走り（600 cm/s）なら乗れる（PIE で確かめた）。**本家も同じ値で同じきわどさ**（2026-09-21 に原作のアセットで確かめた。作業一覧の項目 28 のステップ 10）: 重なりの箱・床の速さ・プレイヤーのカプセル半径 50 と歩き 300・ダッシュ 600 はすべて原作から写した値で、歩いて縁に着くまでの 50/300 = 0.167 s に床は 44.6 cm 上がる（`MaxStepHeight` 45 の際）。走りなら 0.083 s で 22.3 cm。UE 4.24 と 5.8 の段差の上がり方の差までは追えないが、決め手になる値は本家のままなので直さない。
- メッシュは Movable（本家の部品の既定）なので、焼き込みの灯には入らない。置く・置き直すのに焼き直しは要らない。
- **Zone 1 のガレージリフトから救急車の屋根へは歩いて渡れない**: 上がり切った台（316 cm）の +Y の縁から救急車の後ろまで数 m の隙間があり、走っても床へ落ちる（PIE で確かめた）。救急車にはテレポーテーションの的 `BP_Power_Teleport_Zone_Ambulance`（(11245, −20080, 335)）があるので、本家は「台で上がってテレポーテーションで屋根へ」と読める（推定。通しで確かめるのは項目 6 のステップ 10）。
- エディタのレベルでは台が上（参照姿勢）に見える（上の「アニメ」）。敵の経路（項目 7）をエディタで作るときは、`Box` が上にある形で入ることに気を付ける。

## テスト（`Tests/WasamiLiftTests.cpp`・`Tests/WasamiGarageLiftTests.cpp`）
- `Wasami.Lift.Actor`: 部品の値（Top 535・`LiftCollision` 291 × 294 × 76 cm の Block と `LiftMesh` の子・メッシュの当たりなし・重なりの箱の位置と Pawn の重なり・`BottomCollision` 611 cm と位置・遊ぶと `LiftCollision1` が無い・音 2 つが自動で鳴らず 0.7 / 1.5）、誰もいなければ下のまま、キャラクター（コントローラーなしで物理を動かす `ACharacter`）を乗せると 1 s で 267.5 cm 上がり音が鳴り、2 s で 535 に止まって音が止み、キャラクターを運ぶ、降りると 0.5 s で半分・1 s で 0、`OnPlayerOverlap` はプレイヤーだけ（`K2_DestroyActor` を結んだ聞き手が消えるかで見る）。
- `Wasami.Lift.Corner`: `LiftCollision1` が (0, 0, 535) に残って Block、プレイヤー（`APlayerController::SetPawn` で最初のコントローラーのキャラクターにし、動かさない）が下の階なら下、Z 700 なら 0.5 s で半分・1 s で上、戻ると下、乗って 0.5 s で降りると `Double Check` が取り消される、1 s 立つと `Player Force Movement`・`GoUp?` で 267.5 cm/s で上がる、降りると強制が切れて下りる。

- `Wasami.GarageLift.Actor`: 部品の値（メッシュが入り拡縮 30・当たりなし、アニメ 2.4583 s、箱 2 つが `joint4` に付き 390 × 525 × 18.3 cm の Block と 137 cm の厚さの Overlap、Pawn だけに応える、音 2 つが自動で鳴らず Up / Down）、誰もいなければ `Default` で台が下、プレイヤー（`APlayerController::SetPawn` で最初のコントローラーのキャラクターにし、`bRunPhysicsWithNoController` で落ちて乗る）を乗せると `PlayerOn`（重み 1）、1.2 s で途中、2.8 s で 307 cm 上がって最後のコマで止まりプレイヤーを運ぶ、降りて 0.1 s は混ぜの途中で乗り直すと時刻が最後のまま上に戻る、降りて 0.3 s で `Default`・台が下（0.2 s）、乗り直すと時刻が 0 から、`_Zone1_Special` は `bNurseNear` で重なりが偽になり `Default` へ戻る。

## 確かめたこと（2026-09-18、PIE）
- Zone 2 の `lift_4`（`BP_06_Lift_03`）の手前 `(6304, −2740)` から +Y へ走って乗ると、床が上がり、下の階の LIAR LIAR の壁から上の階の HOME WRECKER の鉄柵の前まで運ばれる（プレイヤーの Z 90 → 634）。降りると 1 s で下りる（グリッド `Intermediate/Overnight/lift_grid.png`、収録 `Intermediate/DesktopAgent/shots/lift_ride2.mkv`、git の外）。歩いて近づいたときは上の「既知の制約」のとおり乗れなかった（`lift_ride.mkv`）。2026-09-19 の通しでは `(6304, −2740)` から走っても、加速しきる前に縁の 50 cm 手前で床が上がり始めて縁で止まることが 2 回あった。助走を 70 cm 足した `(6304, −2810)` からなら乗れる（`lift_11` の上に置かない）。
- 角の `lift_7` に乗ると 1 s 後に上がり、プレイヤーごと上の階へ着いて、その後は上の「既知の制約」のとおり沈んでは戻るのを繰り返した。プレイヤーが上の階（Z 634）にいる間は、離れた角のリフトもすべて上がる。

- ガレージリフト（ステップ 8b2）: Zone 1 の `hospital_garage_lift_anim_Anim_2` は、遊び始めると台が下（`Box` Z 9.3）。台に乗ると 2.5 s ほどで台が 316.4 cm、プレイヤーが Z 415.7 まで上がり、救急車の屋根の高さから見下ろす（グリッド `Intermediate/Overnight/garage_lift_grid.png`、収録 `Intermediate/DesktopAgent/shots/garage_lift_ride.mkv`、git の外）。降りると `DD_TT_GarageLift_Down` が鳴り（`Up` は止まる）台が下り、乗り直すと `DD_TT_GarageLift_Up` が鳴った。台から救急車へ走ると床に落ちた（上の「既知の制約」）。Zone 2 の 2 台は置いただけ（PIE で乗っていない）。

## 変更履歴
- 2026-09-21: Zone 2 のリフトの乗り方の 2 つのくせ（角のリフトの上での上下、長い床に歩いて乗れない）が、本家の部品の寸法と値からそのまま出ることを原作のアセットで確かめた（`BP_06_LiftBase` の `LiftCollisionOverlap` は箱の既定 32 に親 `LiftCollision` の拡縮が掛かって床と同じ広さ・床の面から 75.6 cm 上まで、`BP_06_Lift_03` は `LiftCollision1` をレベルの外〈z −25665〉へ、`BP_06_Lift` は `BeginPlay` で消す）。コードの変更は無し（作業一覧の項目 28 のステップ 10）
- 2026-09-19: `dd_skeletal` の取り込みが本家の骨格のソケットを足すようにした（`add_sockets`。のこぎりの罠のため。01・08 記録）。ガレージリフトの骨格にソケットは無く、何も変わらない
- 2026-09-18: 初版。本家の `BP_06_LiftBase`・`BP_06_Lift`（`_03`・`_04`）・`BP_06_LiftBase_Corner` を `AWasamiLiftBase`・`AWasamiLift`・`AWasamiCornerLift` に写し、組み立てが Zone 2 に 15 台置くようにした（作業一覧の項目 6 のステップ 8a）
- 2026-09-18: ガレージリフトの骨入りのメッシュとアニメを取り込んだ（`dd_skeletal`。作業一覧の項目 6 のステップ 8b1）
- 2026-09-18: ガレージリフトのアクタ `AWasamiGarageLift`・`AWasamiGarageLiftZone1Special` と本家の ABP の写し `UWasamiGarageLiftAnimInstance`、テスト `Wasami.GarageLift.Actor` を足し、組み立てが両ゾーンに 3 台置くようにした（作業一覧の項目 6 のステップ 8b2）
- 2026-09-19: `AWasamiGarageLiftZone1Special` の `bNurseNear` を Zone 1 の流れが書くようになった（コメントだけ直した。作業一覧の項目 7 のステップ 3）
- 2026-09-19: `MoveLocation` を Zone 2 の迷路のナースが読むようになった（コメントだけ直した。作業一覧の項目 7 のステップ 4）
- 2026-09-19: `dd_skeletal` が根以外の骨の回転のキーも運ぶようにした（のこぎりの罠のため。01・08 記録）。ガレージリフトの骨は回らないので、`prepare` の出力は変わらない（取り込み直していない）
