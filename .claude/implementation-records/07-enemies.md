---
title: 敵ワサミ（素体の素材・アニメの再生・敵のアクタ）
sources:
  - Content/Python/wasami_tools/pipeline/dd_enemy.py
  - Source/wasami_deception/WasamiEnemy.h
  - Source/wasami_deception/WasamiEnemy.cpp
  - Source/wasami_deception/WasamiEnemy06Chase.h
  - Source/wasami_deception/WasamiEnemy06Chase.cpp
  - Source/wasami_deception/WasamiEnemyZone2.h
  - Source/wasami_deception/WasamiEnemyZone2.cpp
  - Source/wasami_deception/WasamiEnemySentry.h
  - Source/wasami_deception/WasamiEnemySentry.cpp
  - Source/wasami_deception/WasamiViewcone.h
  - Source/wasami_deception/WasamiViewcone.cpp
  - Source/wasami_deception/WasamiEnemyAnimInstance.h
  - Source/wasami_deception/WasamiEnemyAnimInstance.cpp
  - Source/wasami_deception/Tests/WasamiEnemyTests.cpp
  - Source/wasami_deception/Tests/WasamiTestListener.h
  - SourceArt/Wasami/enemy_wasami_v3.glb
  - SourceArt/Wasami/enemy_wasami_capture.glb
updated: 2026-09-19
---

# 敵ワサミ（素体の素材・アニメの再生・敵のアクタ）

## 役割
本家のナース（`BP_06_ReaperNurse`）の代わりに Zone 1・2 を巡回し追う敵ワサミ。いまは**素材の取り込み**（ユーザーのモデルを、スケルタルメッシュと役の名前で引けるアニメにする）、**アニメの再生**（`UWasamiEnemyAnimInstance`。本家のナースの ABP の形でクリップを混ぜる）、**敵のアクタ**（`AWasamiEnemy`。本家のナースの部品と気絶。パワーの受け口）（作業一覧の項目 4 のステップ 1〜3）と、**判断**（本家のナースの `Make Choice`: 巡回・発見・追跡・見失い。作業一覧の項目 7 のステップ 2）、**06 の追跡型**（Zone 1 の駐車場の `BP_06_ReaperNurse_06_Chase`: 毎ティック追い、トンネルの扉を突く。ステップ 3）、**Zone 2 の迷路の型**（`BP_06_ReaperNurse_Zone2`: プレイヤーと階が違うとリフトへ向かう。ステップ 4）、**見張りと視界コーン**（Zone 2 のミニボスの廊下の `BP_06_ReaperNurse_Sentry` と `BP_06_Miniboss_viewcone(_Nurse)`: 高い所で見張り、コーンに入ると跳び降りて追う。ステップ 5a・5b）まで。出現はゾーンの流れ（11 記録）。追跡中の変化・捕獲はこの記録に書き足していく。役とアニメの対応の決まりは `.claude/references/enemy-wasami-motions.md`。

## 公開インターフェース
- `WasamiDDTools.import_wasami_enemy()`（01 記録）→ `dd_enemy.import_all()`。戻り値 `textures` 3 / `materials` 2 / `meshes` 1 / `animations` 18。
- `dd_enemy.make_capture_source(old_glb)`: ユーザーの旧モデル（`tmp/enemy_wasami.glb`、git の外）から `SourceArt/Wasami/enemy_wasami_capture.glb` を書く（2026-09-18 に 1 回走らせた。旧 glb が変わらない限り再び走らせる必要はない）。
- `dd_enemy.prepare()`: 前処理した glb を書き、役ごとの（長さ、骨盤を動かした量）を返す。`prepared_file()` がその場所。
- `UWasamiEnemyAnimInstance`（ネイティブの AnimInstance。メッシュの `AnimClass` にする。持ち主が毎フレームのフラグを書く）:
  - `bStunned`（持ち主の State が Stun。立ち上がりで気絶のクリップを `StunDuration` の長さで始め、起き上がりに入る更新で持ち主を移す。下の「気絶」）、`StunDuration`（既定 17.0 = `BP_06_ReaperNurse` の `Delay 17.0`）、`bAggressiveIdle`（見張り）、`bNightmare`（全回収後の追跡の走り）。**持ち主が `AWasamiEnemy` なら毎フレーム、この 4 つを持ち主から読む**（`IsStunned()`・`GetStunTimeLeft()`・`bAggressiveIdle`・`bNightmare`。本家の ABP のイベントグラフと同じ引き方）。ほかの持ち主では外から入れる。`Speed` は読み取り専用（持ち主の `GetVelocity().Size()`）。
  - `PlayOnce(Clip, PlayRate=1, BlendIn=0.25, BlendOut=0.25)`: 名前（`WasamiEnemyAnim::ClipNames`。`'Chase_Slide'` など）のクリップを全身に 1 回かぶせる。持ち主は動いたまま。戻り値は再生の秒数（長さ / 速さ）。無い名前・速さ ≤ 0 は 0 を返して警告する。`StopOnce(BlendOut=0.25)`、`IsPlayingOnce()`（ブレンドアウトが始まったら偽）、`GetMainClip(OutTime, OutWeight)`（いちばん重いクリップの名前。PIE の確かめ用）。いずれも Blueprint から呼べる（Python からも）。
  - C++ だけ: `GetAnimState()`（再生の状態。テスト用）、`GetGetUpMove(Fall)`（倒れる 1 本とその起き上がりの `MeasureGetUpMove`。両方読めたときだけ値がある）。
- `AWasamiEnemy : ACharacter, IWasamiEnemyInterface`（`WasamiEnemy.h`）:
  - static `SpawnEnemy(WorldContext, Location, Yaw=0, bSentry=false)`（BlueprintCallable。カプセルの中心を Location に置き、`bCanSpawn` を真にして出す。重なりは可能ならずらして必ず出す。PIE の Python から `unreal.WasamiEnemy.spawn_enemy(world, loc, yaw)`）。
  - インターフェース: `SetState`（State を入れるだけ。`bByOrb` は使わない）、`GetState`（**常に Patrol**。本家のナースと同じ）、`PlayerVanish`（`bSeenPlayerRecently` を偽に）、`NoTelepathy`（偽）。
  - `SetWalkState(bNormal)`（`MaxWalkSpeed` を `NormalSpeed` 350 / `SkateSpeed` 800 に）、`GetCurrentState()`（本当の State）、`IsStunned()`（State == Stun）、`IsStunRunning()`（判断が気絶を始め 17 s を待っている）、`GetStunTimeLeft()`（巡回に戻るまでの秒。下の「敵のアクタ」）、`GetEnemyAnim()`（メッシュの `UWasamiEnemyAnimInstance`）。
  - 設定: `bCanSpawn`（既定 偽。ExposeOnSpawn）、`bAggressiveIdle`（見張り。ExposeOnSpawn）、`bNightmare`（全回収後の追跡の走り）、`NormalSpeed`・`SkateSpeed`、`bNormalWalk`、`bSeenPlayerRecently`（本家の `Seen Player Recently`）、`PointOfInterest`（最後に追ったときのプレイヤーの位置。0 で無し）、`RandomPoint`（読み取りだけ）。
  - 判断（下の「判断」）: `IsChasing()`（= `bSeenPlayerRecently`。本家の `Chasing`）、`CanSeePlayer()`、`ChasePlayer()`・`NotSeeingPlayer()`・`GenerateRandomPoint()`（BlueprintCallable。Python から `can_see_player()` などで呼べる）、C++ の virtual `GetPlayerTarget()`（既定はプレイヤー）・`GetRandomPointDestination()`（既定は `RandomPoint`。Zone 2 の型が階の違うときにリフトへ替える）、イベント `OnCloseBy`（BlueprintAssignable。見失ってからの最初の追跡で 1 回。受け手は Zone 1 の `Setup Nurse Bierce Quips`〈項目 20〉）。
  - 派生の口: virtual `IsChasing()`、virtual `GetTimeToStunStart()`（気絶を入れてから 17 s が始まるまで。既定は次の判断までの残り）、protected `StartStun()`（気絶の DoOnce。判断と 06 型の tick が呼ぶ）、protected virtual `BeginNurse()`（本家のナースの `ReceiveBeginPlay` の中身。`BeginPlay` が呼ぶ。見張りは空にし、`Player Spotted` で親のものを呼ぶ）。
  - 定数: `CapsuleRadius` 34、`CapsuleHalfHeight` 118.058、`MaxSpeed` 800、`TurnRate` 300、`MeshX/Y/Z`・`MeshYaw`（メッシュの相対位置と向き）、`DecisionInterval` 0.5、`StunSeconds` 17、`ViewAngle` 100、`ForgetSeconds` 3、`RandomPointRadius` 3000、`ChaseAcceptance` 5・`PointOfInterestAcceptance` 5・`RandomPointAcceptance` 50。
- `AWasamiEnemy06Chase : AWasamiEnemy`（`WasamiEnemy06Chase.h`。下の「06 の追跡型」）: `bAttackDoor`（BlueprintReadWrite。Zone 1 の流れが `06_DoorsLock` で真、扉が破れて偽）、`DoorLocation`（ExposeOnSpawn。流れがレベルの `DoorLocation` を入れる。本家も読まない）、`HitFX()`（BlueprintCallable）、`GetDoorAttacks()`（突いた回数。確かめ用）、`IsChasing()` は常に真、`GetTimeToStunStart()` は 0。定数 `DoorAttackLength` 0.9667・`DoorHitTime` 0.4132・`DoorAttackBlendOutTrigger` 0.5・`DoorAttackBlendOut` 0.25・`DoorAttackSeconds`（= 0.7167）・`DoorAttackMaxWait` 0.5・`HitFXChance` 0.5・`HitFXForward` 230・`HitFXScale` 0.5・`HitShakeRadius` 3000・`DoorAttackClip`（`Chase_Charge`）。
- `AWasamiEnemyZone2 : AWasamiEnemy`（`WasamiEnemyZone2.h`。下の「Zone 2 の迷路の型」）: `IsUp()`・`IsPlayerUp()`・`IsSameLevelAsPlayer()`（BlueprintPure）、`GetClosestLift(OutMoveLocation)`（C++。`AWasamiLift` を返す）、`GetPlayerTarget()`・`GetRandomPointDestination()` の上書き。定数 `UpperFloorZ` 640・`PlayerUpperFloorZ` 610・`ClosestLiftStart` 1e9（2 乗の距離）。
- `AWasamiEnemySentry : AWasamiEnemy, IWasamiViewconeInterface`（`WasamiEnemySentry.h`。下の「見張りと視界コーン」）: `Activate()`（BlueprintCallable。コーンに `Offset` を渡して `Initialize`）、`GetViewcone()`（コーンのアクタ。見つけた後は null）、`GetJumpDownSpot()`、`IsChasing()` は自分の `bChasing`。設定 `Offset`（EditAnywhere。コーンが最初に見るまでの秒）、読み取り `bChasing`・`bVarIdle`。部品 `Viewcone`（`UChildActorComponent`、`AWasamiViewconeNurse`）・`JumpDownSpot`（`USceneComponent`。置いたものごとに相対位置を入れる）。定数 `ViewconeLocation` (0, 0, 72)・`ViewconeRotation`（ピッチ −20）・`JumpSpeed` 400・`JumpUpSpeed` 500。CDO は `bAggressiveIdle` 真。
- `IWasamiViewconeInterface`（`WasamiViewcone.h`。本家の `BPI_06_Viewcone`）: `StartLooking`・`StopLooking`・`PlayerSpotted`（BlueprintNativeEvent。既定は何もしない）。見張りと項目 11 の Matron が受ける。
- `AWasamiViewcone : AActor`（`WasamiViewcone.h`。本家の `BP_06_Miniboss_viewcone`）: `Initialize()`・`IsInitialized()`・`UpdateSight()`・`PlayerInsideCone()`・`PlayerInFullView()`・`TurnOn()`・`TurnOff()`・`IsOn()`・`GetFade()`（BlueprintCallable / Pure。`UpdateSight` は C++ だけ）、`GetPlane()`・`GetDot()`。設定 `Length`（既定 1000）・`Angle`（45）・`bAutoOn`（真。本家の `bAutoOn?`）・`Offset`（0）。派生の口 protected virtual `InitializeFinished()`（既定は空）。定数 `SightRateMin` 0.3・`SightRateMax` 0.5・`TurnOnDelay` 1・`FadeInLength` 0.5・`PlaneLift` (0, 0, 1000)・`PlaneLocation` (4.8, 0, 2000)・`DotLocation` (0, 0, 1000)。
- `AWasamiViewconeNurse : AWasamiViewcone`（本家の `BP_06_Miniboss_viewcone_Nurse`）: `Length` 1500・`Angle` 20、`Turn()`（点く ↔ 消える）。定数 `TurnRate` 10・`NursePlaneLocation` (837.8, 0, 0)・`NursePlaneScale` (17.006, 9.620, 28)。
- `WasamiEnemyClip`（クリップの番号。移動 5 本、気絶 4 本〈倒れる `StunFlyUp`・`StunKnockDown`、起き上がり `StunGetUpFlyUp`・`StunGetUpKnockDown`〉、捕獲 3 本、追跡中の変化 6 本の順で 18 本）、`WasamiEnemyAnim::ClipNames`・`FindClip(Name)`・`ClipPath(Clip)`（`/Game/Wasami/Enemy/A_WasamiEnemy_<名前>`。名前は取り込みの `ROLES` の 1 列目と同じ並び）、`NumStunFalls` 2・`GetUpAfter(Fall)`（倒れる 1 本の後の起き上がり）、`GetRootTransform(Sequence, Time)`（骨組みの根 = `pelvis` のメッシュの空間の変形）、`MeasureGetUpMove(Fall, GetUp)`（下の「移し替え」）。
- 定数: `MESH` = `/Game/Wasami/Enemy/SK_WasamiEnemy`、`SKELETON` = `…_Skeleton`、`PHYSICS_ASSET` = `…_PhysicsAsset`、`ANIM_PREFIX` = `A_WasamiEnemy_`、`MATERIAL` = `/Game/Wasami/Enemy/MI_WasamiEnemy`、`MASTER` = `/Game/Pipeline/Materials/M_DD_WasamiGltf`、`ROLES`（下の表）。

## 内部構造と処理の流れ

### 原本
- `enemy_wasami_v3.glb`（30 MB。ユーザーの `tmp/enemy_wasami_v3.glb` の写し）: Blender 4.5 の glTF。根 `target_character` の子にメッシュ `output_unwrapped`（89,572 頂点・104,806 三角形、材質 1、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²）と `pelvis`。スキン 1（骨 28、根 `pelvis`、ルートの骨なし）、アニメ 16（どれも LINEAR）。
- `enemy_wasami_capture.glb`（259 KB）: 旧モデル（骨 22。名前は全部 v3 にある）の節と、`Backflip`・`sliding_rool`・`Stylish_Walk`・`run_fast_2` のキーをそのまま（旧 glb と同一なのを確かめた）。メッシュ・スキン・テクスチャは入れない（20 MB を LFS に入れないため）。`run_fast_2` は下の載せ替えの物差し。

### 前処理（`prepare`）
glb のキーは 30 fps の動きを 24 fps の場面に焼いたもので、骨盤だけ 24 fps と 30 fps のキーが混ざり、どの骨も 2/30 s から始まる（骨盤は 1/24 s から）。Interchange はコマの境目で終わらないアニメを取り込まない（「アニメーションの長さ … はインポート フレームレート 30 fps と互換性がありません」。16 本中 11 本が落ちた）ので、**全部を 30 fps のコマ（`RATE`）で 0 から標本化し直す**。中身は 2/30 s（`CONTENT_START_FRAME`）から、骨盤以外の骨の最後のキーまで。回転は slerp、書き出す前に四元数の符号を前のキーにそろえる。

役ごとの作り方（`ROLES` の 4 列目）:
- `loop`: 最初のキーを最後の後にもう 1 つ置いて閉じる（元のループは 1 コマ手前で止まっている。`Running` の最初と最後の差は 1 コマの動きの 0.35 倍、`Walking` は 1.5 倍、`run_fast_2` は 1.1 倍で、周期 = 中身 + 1 コマと読んだ）。
- `in_place`: 骨盤の水平（glTF の x と z）を最初のキーの値に止める（高さは残す）。
- `vault`（`Vault_and_Land` だけ。`_vault` の後に `in_place`）: 元は「高さ 76.6 cm の台の上に立ち、少しかがんで右脚を振り上げ（0.567 s〜）、左足が台を離れ（コマ 28 = 0.933 s の後）、床に着地し（コマ 50 = 1.667 s）、かがんでから起き上がる（骨盤はコマ 74 = 2.467 s で止まり、その後は立っているだけ）」動き（`VAULT_FRAMES`。コマは元の 30 fps の番号）。追跡の廊下には台が無いので床から跳び越える形にする: (1) コマ 74 で切る（2.4 s）。(2) 台の高さ（最初のキーと最後のキーの、`FEET`〈`ball_l`・`ball_r`〉の低いほうの高さの差。骨の世界位置は `gltf.world_transforms`）だけ骨盤を下げ、離れる時刻から着地までに smoothstep で 0 に戻す。こうすると床に立ったまま手を約 70 cm の高さにつき、足は 1.1 s に約 78 cm、骨盤は 1.3 s に 128 cm まで上がり、1.6 s に着地する。(3) 元はこの 1 本だけ 31° 斜めへ進み（ほかの 5 本は真っすぐ +z。体の向きも 25° → 53°）、その場の形にすると足の運びが進む向きと合わないので、骨盤の回転と位置を上下の軸まわりに −31.7°（最初と最後の骨盤を結ぶ向きを +z へ）回す。向きは −7° で始まり、跳ぶ間は約 48° の横向き、21° で終わる。前後は元の 1.24 m を止める。**仮の扱い**（2026-09-18、要確認）。
- `stun_fall`（`_stun_fall`。`BeHit_FlyUp`・`Knock_Down`）: 気絶で倒れる 2 本（2026-09-18 のユーザーの指示。どちらかをランダム）。骨盤の水平を、最初のキーが `Idle` の最初の骨盤（x 0.004、z −0.022 m）に来るよう全体でずらす（`Knock_Down` は元の骨盤が始めから 1.15 m 後ろにある）。どちらもあおむけで終わり（胸の正面の上向き成分 +1.00・+0.99）、頭は元の後ろ側。
- `stun_get_up`（`_stun_get_up`）: その倒れる 1 本の後の起き上がり = 寝返り（`ROLL_FRAMES` 24 = 0.8 s）+ `push_up_to_idle`（`GET_UP`。うつぶせ〈−0.99〉で頭が前側から始まり、腕立てから立つ）。(1) `push_up_to_idle` を、骨盤 → 首（`NECK`）の水平の向きが倒れた終わりと同じになるよう上下の軸まわりに回し（頭どうしが同じ側）、骨盤を倒れた終わりの骨盤の上へ動かす。(2) 寝返り（`_roll`）: 骨盤を体の長い軸（倒れた終わりの骨盤 → 首の水平の向き）まわりに smoothstep で半回転させ、腕立ての最初の骨盤の回転との残りの差（4.6°・7.1°）を同じ割合で足す。ほかの骨はローカルの回転を倒れた終わり → 腕立ての最初へ、寝返りの `ROLL_LIMB_SHARES` の割合までに寄せる（あおむけで横に広げた腕を先に体へ寄せるため。一緒に回すと下になる腕が床を 38 cm 突き抜けた）。回る向き 2 つ × 割合 4 つのうち、関節の最も低い点が高いものを選ぶ（両方とも 35 % が選ばれ、最低 −0.08 m）。それでも `ROLL_FLOOR_SLACK`（3 cm）より下がる分だけ骨盤を持ち上げる（前後 3 コマにわたって保ち、両端 3 コマで 0 に戻す。約 5 cm）。(3) 全体を、終わりの骨盤が `Idle` の最初の骨盤の水平位置に来て、骨盤の前（`Idle` の最初のキーで前を指す骨盤のローカルの軸）が +z を向くように回して動かす。そのため起き上がりの最初のキーは、倒れた終わりの姿勢を上下の軸まわりに回して水平に動かしたもの（前処理のログ: FlyUp は −174.2° 回して (0.03, 0.13) m、KnockDown は 168.8° 回して (0.20, −0.49) m）。敵はこの差だけ、起き上がりを始めるときにアクタごと移す（アニメの再生）。寝返りの長さ 0.8 s と手足の寄せ方（候補 4 つからの選び方）は Claude の仮の値だったが、2026-09-18 の PIE（Primal Fear で `Stun_KnockDown`、4 fps のコマの一覧）で床への潜りも骨盤の跳びも見えなかったので確定した（ユーザーの回答「Claude が寝返りを作る。PIE で見て決める」）。
- 捕獲（`CAPTURE`）: `once` のみ。`_Retarget` で v3 の骨へ載せ替える。

`_Retarget`（両方にある `run_fast_2` で測る。`REFERENCE_ANIMATION`）: 2 つのモデルの骨は**同じ向きを指し、骨の軸まわりのひねりだけが違う**。各骨の場面での回転は「旧 × 骨ごとに一定の回転」で、その回転は時間で変わらない（測った値: 腕と鎖骨で最大 21.7°、手は向きも少し違う。ずれ 0.075°）。骨 b（親 p）のローカルの回転 r は `inverse(p の回転) · r · (b の回転)` にする。骨盤の位置は「v3 の休み + 0.9929 ×（旧 − 旧の休み）」（x・y・z 同じ係数を最小二乗で。ずれ 0.0000 m）。ほかの骨の移動は捨てて v3 の骨の長さを使う。ずれが `RETARGET_TOLERANCE`（0.5°・2 mm）を超えたら例外。**同じ対応を旧と v3 の `Running`・`Walking`・`BeHit_FlyUp` に当てると v3 の値と 0.08° 以内で一致する**（気絶のモーションは頭だけ 4°）ので、ユーザーのツールが v3 用に書き出すのと同じ形になる。

v3 の `restpose`（腕を広げた基準姿勢、0.083 s）と、使わなくなった気絶のモーション `01a0a88f-…`（`OLD_STUN`、10.04 s。2026-09-18 まで前屈の揺れのループと起き上がりに分けて使っていた）は取り込まない（`SKIPPED`）。`push_up_to_idle` は起き上がりの中でだけ使う。一覧に無いアニメが v3 に足されたら、警告を出して元の名前で `once` として取り込む。

| 役（`A_WasamiEnemy_…`） | 元 | 作り方 | 長さ | 骨盤を止めた量（x / z） |
|---|---|---|---|---|
| `Idle` | `Idle_11` | loop | 1.933 s | |
| `Idle_Alert` | `Idle_5` | loop | 1.900 s | |
| `Walk` | `Walking` | loop | 1.033 s | |
| `Run` | `Running` | loop | 0.667 s | |
| `Run_Nightmare` | `run_fast_2` | loop_in_place | 0.600 s | −0.01 / 2.60 m（4.6 m/s） |
| `Stun_FlyUp` | `BeHit_FlyUp` | stun_fall | 1.533 s | 動かしたまま（骨盤は −0.01 / −0.04 m 進む） |
| `Stun_KnockDown` | `Knock_Down` | stun_fall | 2.500 s | 動かしたまま（0.02 / −0.67 m、後ろへ） |
| `Stun_GetUp_FlyUp`・`_KnockDown` | 寝返り + `push_up_to_idle` | stun_get_up | 3.900 s（0.8 + 3.1） | 終わりを `Idle` の位置と向きへ |
| `Capture_1`・`_2`・`_3` | 旧 `Backflip`・`sliding_rool`・`Stylish_Walk` | once（載せ替え） | 2.133・2.767・3.533 s | 動かしたまま（前へ 0.0・5.7・2.1 m） |
| `Chase_PickUp` | `Female_Run_Forward_Pick_Up_Right` | in_place | 1.233 s | 0.00 / 4.02 m |
| `Chase_Charge` | `Male_Head_Down_Charge` | in_place | 0.533 s | 0.01 / 2.17 m |
| `Chase_VaultRoll` | `Parkour_Vault_with_Roll` | in_place | 2.100 s | −0.01 / 4.61 m（途中で横に最大 0.22 m） |
| `Chase_VaultLand` | `Vault_and_Land` | vault | 2.400 s | 0.00 / 1.24 m（−31.7° 回した後。骨盤を離れるまで 76.6 cm 下げた） |
| `Chase_RunFast` | `run_fast_5` | in_place | 1.833 s | 0.00 / 3.46 m |
| `Chase_Slide` | `slide_right` | in_place | 1.767 s | 0.01 / 3.35 m（途中で横に最大 0.17 m） |

足の運びの速さ（接地した足の後ろへの速さ。再生の速さを移動に合わせるときの物差し。項目 4 のステップ 4 の PIE で、骨 `ball_l`・`ball_r` の世界での速さから測った。**メッシュの元の大きさ（拡縮 1）での値**）: `Walk` 1.34 m/s、`Run` 4.46 m/s、`Run_Nightmare` 4.96 m/s。敵のアクタは `MeshScale`（1.3591）倍で描くので、ゲームの中では 1.82・6.06・6.74 m/s。

### アニメの再生（`UWasamiEnemyAnimInstance`）

本家のナースの ABP（`nurse_idle1_Skeleton_AnimBlueprint`）の木を、アニメグラフのノードを使わずに C++ で持つ（本作は手作りの BP を持たず、MCP にアニメグラフを組む道具も無いため）。3 つに分かれる:

1. **`FWasamiEnemyAnimState`**（エンジンを使わない純粋な状態。テストの対象）: どのクリップを、どの時刻・重みで混ぜるかを決める。`Update(Inputs, DeltaSeconds)` → `GetSamples(OutSamples)`（重みの和は 1。欠けたクリップの分は正規化で埋める）。
2. **`UWasamiEnemyAnimInstance`**: `NativeInitializeAnimation` でクリップを読み（**ゲームのワールドのときだけ**。エディタのレベルでは参照姿勢のまま。`WasamiAssets.h` の起動時の読み込みを避けるため）、長さを状態に渡す。`NativeUpdateAnimation`（ゲームスレッド）で持ち主の速さとフラグから状態を進め、標本の一覧 `FrameSamples` を作る。
3. **`FWasamiEnemyAnimInstanceProxy`**: `PreEvaluateAnimation`（ゲームスレッド。更新の後・評価の前に必ず呼ばれる。`PreUpdate` は `NativeUpdateAnimation` より前なので使わない）で標本を写し、`Evaluate`（ワーカースレッド）で各クリップの姿勢を `UAnimSequence::GetAnimationPose` で取り出して `FAnimationRuntime::BlendPosesTogether` で混ぜる。スケルトンの無いクリップは飛ばし（エンジンのシーケンスプレーヤーと同じ判定）、標本が無ければ参照姿勢。ルートモーションは取り出さない。

木（重みは上から掛け合わせる）:

- **全身の 1 回再生**（本家の Slot `Fullbody` にモンタージュを入れる代わり）: 1 回再生の重みの和 W（最大 1）が上に乗り、下の木は 1 − W。複数あるときは重みの比で分ける。
- **根: 気絶**（`FWasamiBoolBlend`。本家の Blend Poses by bool、0.25 s、Linear）: 気絶の側 = 倒れる 1 本かその起き上がり（`FWasamiStunPlayback`。1 回だけ流すので繰り返さない）、偽の側 = 移動。
- **移動: Idle ↔ Moving**（`FWasamiStateBlend`。本家のステートマシンの標準のブレンド）: 速さ > 5 で Moving へ 0.5 s（Sinusoidal）、速さ < 5 で Idle へ 0.25 s（ExpOut）。ちょうど 5 はどちらにも移らない。移る先の重みは、その時の重みから遷移の曲線に沿って 1 へ動く。本家は Skating → Stop Skating（`nurse_skate_stop` を 0.35 s から 1 回）→ Idle（残り 10 % 未満、0.5 s、Cubic）だが、止まるクリップが無いので Stop へ入る値のまま直接 Idle へ移る。
  - Idle の中: `Idle` | `Idle_Alert`（`bAggressiveIdle`、1.0 s、Linear。本家は Alert がさらに 2 本〈`bVarIdle` で 0.1 s〉だが、ここは 1 本）。
  - Moving の中: `Walk` | 走り（速さ > 400、0.25 s、Linear）。走りの中: `Run` | `Run_Nightmare`（`bNightmare`、0.25 s。本家に無い分岐で、走りと同じ切り替えにした。`TODO(仮)`）。
- **ブレンドの動き**（エンジンの `FAnimNode_BlendListBase` と同じ）: 重みは 1 / ブレンド時間 の速さで目標へ動く（途中で折り返すと、残りの重みの分の時間で戻る）。リセットの後の最初の更新は目標へ飛ぶ。
- **時刻の進め方**: 重み 0 の枝は進めない。重み 0 から入った状態（Idle / Moving）は中のクリップを 0 から始め、中の切り替えもリセットする（本家の `bAlwaysResetOnEntry` 偽・`bResetChildOnActivation` 偽と同じ）。**本家と違い、移動の木は気絶の下でも 1 回再生の下でも進める**（本家の Slot と BlendList は重み 0 の子を進めない）。気絶が明けたときに移動の木を速さに合った状態（止まっていれば Idle）にしておき、起き上がりの終わりの待機へ走りの姿勢が混ざらないようにするため。追跡中の変化が終わったときも走りの位相が続いている。
- **再生の速さ**（`TODO(仮)`。本家はスケートで速さ 1）: `Walk` = 速さ / (133 × `MeshScale`) を 0.5〜2、`Run` = 速さ / (450 × `MeshScale`) を 0.6〜1.8、`Run_Nightmare` = 速さ / (500 × `MeshScale`) を 0.6〜1.8。分母は各クリップの接地した足の速さ（上の「足の運びの速さ」）を、敵のアクタがメッシュを描く大きさ `AWasamiEnemy::MeshScale` 倍にしたもの（歩幅が大きさに比例するため。`WasamiEnemyAnimInstance.cpp` の `StrideScale`）、範囲は WebGL 版の 15 記録の `speedRatio` の範囲。巡回 350 cm/s の `Walk` は 1.94、追跡 800 cm/s の `Run` は 1.31。拡縮 1 のときの PIE での接地中の足の滑り（前向きの速さ）は、追跡 800 cm/s の `Run` で 1 %（ほぼ止まる）、巡回 350 cm/s の `Walk` は上限 2 に当たって速さの 24 % 滑った（2026-09-18 に大きくしてからは、上限に当たらない。滑りは測り直していない）。`Run_Nightmare` は初めの分母 460（取り込み前の骨盤の進みから出した値）では足が後ろへ 8 % 流れたので、測った 500 にした。待機・気絶は速さ 1、1 回再生は指定の速さ。
- **気絶**（`FWasamiStunPlayback::Start(Duration, 倒れる長さ, 起き上がりの長さ)`）: `bStunned` の立ち上がり（ブレンドアウト中の 2 回目も）で、倒れる 2 本（`Stun_FlyUp` 1.533 s・`Stun_KnockDown` 2.5 s）から 1 本を `FWasamiEnemyAnimState::StunRandom` で選び（2026-09-18 のユーザーの回答「ランダム」。Primal Fear でもスタンオーブでも同じ。欠けた 1 本は残りに譲る。種は `Init` の引数で、インスタンスは `FMath::Rand()`、テストは決めた種）、1 回再生を 0.25 s でブレンドアウトさせる。倒れる 1 本を 0 から流して終わりの姿勢（あおむけ）で止め、起き上がり（3.9 s）を `GetUpStart = max(倒れる長さ, Duration − 起き上がりの長さ)`（17 s なら 13.1 s）から流して終わりの姿勢で止める。起き上がりが気絶の終わりにちょうど終わり、短い気絶では倒れ終わってすぐ起き上がり、終わる前に明ける。**時刻は `bStunned` の間だけ進める**: 明けてブレンドアウトする間は姿勢を保ち、寝ている間に明けても起き上がり（と移し替え）は起きずに待機へ混ざる。
- **移し替え**（`UWasamiEnemyAnimInstance::MoveToGetUp`）: 起き上がりの最初のキーは、倒れる終わりの姿勢を上下の軸まわりに回して床の上で動かしたもの（前処理の `stun_get_up`。起き上がりの終わりを `Idle` の位置と向きにそろえたため。敵は倒れる前と逆を向いて立つ）。そのまま流すと体が跳ぶので、再生の状態が起き上がりに入った更新で `StunGetUpStarted` を立て、`NativeUpdateAnimation`（ゲームスレッド。同じコマの姿勢の評価の前）がアクタを `Rel⁻¹ · Move · Rel · Actor` へ移す（`Rel` はメッシュの相対変形〈拡縮 1.3591 込み〉、`Move` = `MeasureGetUpMove` = 起き上がりの 0 s の根⁻¹ · 倒れる終わりの根 を上下の軸の回転と水平の移動だけにしたもの。`NativeInitializeAnimation` で倒れる 2 本ぶん求める）。`SetActorLocationAndRotation`（`TeleportPhysics`、スイープしない）の後、コントローラーの `SetControlRotation` も新しい向きにする（`bUseControllerDesiredRotation` で戻されないように）。気絶の終わり（`EndStun`）で移さないのは、タイマーがアニメの更新の後に進み 1 コマずれるため。UE で測った `Move`: FlyUp はヨー −174.241°・(1.44, 12.80, 0) cm、KnockDown は 168.796°・(10.08, −51.85, 0) cm（部品の空間。傾きは 0）。PIE（2026-09-18、KnockDown）ではアクタがヨー +168.80°・71.79 cm 動き、そのコマの骨盤の世界の動きは 0.00 cm。`GetRootTransform` は、`UAnimSequence::GetBoneTransform` がトラックの無い骨を書かないので、骨組みの基準姿勢で初期化してから読む。
- **1 回再生**（`FWasamiOncePlayback`。UE のモンタージュの更新の順に倣う）: 重みを先に動かし（ブレンドインは 1 / BlendIn、ブレンドアウトは 1 / BlendOut の速さ）、次に時刻を進め（速さを掛け、長さで止める）、残りの実時間（(長さ − 時刻) / 速さ）が BlendOut 以下になったらブレンドアウトを始める。重み 0 で消える。新しい `PlayOnce` は前のものを新しい BlendIn でブレンドアウトさせ、既にブレンドアウト中のものは短い方の時間にする（BlendIn 0 なら前のものはすぐ消える）。`StopOnce` も同じ規則。

テスト（`Tests/WasamiEnemyTests.cpp`）— 敵のアクタ `Wasami.Enemy.Actor.*`: `Defaults`（CDO と部品の値。メッシュの拡縮 `MeshScale` を含む）、`Stun`（手で進めるゲームのワールドで 0.0625 s 刻み。上のタイマーの刻みで、判断は 0.625・1.125 … s の更新に来る: `CanSpawn` なしは消える、AI が付く、メッシュとアニメ（起き上がりの長さと移し替えの差が読めている）、`Set Walk State`、`SetState` の直後は気絶だが動いたまま → 判断で止まる、アニメの残り、2 回目と Patrol への往復で延びない（Patrol の間の残りは 0）、6.0625 s に始め直した気絶が 13.75 s の更新で起き上がりに入り、敵が `Move` のヨーだけ回って高さを保ち、骨盤の世界の位置と向きが変わらず〈0.5 cm・0.5°〉、コントローラーも回り、次の更新では動かない、17.625 s の更新で起き上がりが終わり次の更新で Patrol、明けの次の判断から 2 回目）、`Powers`（Primal Fear・Vanish・Telepathy が届く）、`Choice`（ナビのデータの無いワールドで、プレイヤーは所有しないキャラクター: `CanSeePlayer` の角度〈99° は見え、101°・真後ろは見えず、500 m 先も見える〉・間のキャラクター・Vanish のカプセル、最初の判断は巡回の歩き〈350〉で見えたら次から追跡〈800〉・`PointOfInterest`・`OnCloseBy` は 1 回、隠れても追い続けて位置を知る、Vanish の次の判断で `PointOfInterest` へ歩き、その移動の失敗〈0.1 s 後〉で 0 に、最後の追跡から 3 s で `CloseBy` が開き直す、気絶で判断が止まると最後の追跡から 3 s で見失う。道具 `UWasamiTestListener`〈引数なしの動的デリゲートを数える〉）、`Zone2`（プレイヤーがいなければ下の階と読む、640 / 610 ちょうどは上でない、同じ階ならプレイヤーと `RandomPoint`、本家の位置に置いた 2 基のリフトでは原点に近い方〈ナースのそばの方でなく〉、角のリフトは入らない、`Move Location` は床の 149.5 cm 上、ナースが上でプレイヤーが下でも同じリフト、同じ距離なら後の方、31623 cm より遠いリフトは選ばれず的も位置も無い）。アニメの再生 `Wasami.Enemy.Anim.*`: `Blends`（切り替えと遷移の曲線）、`Locomotion`（350・800・2000 cm/s〈再生の速さは `MeshScale` 倍の歩幅で割る〉、止まる、ちょうど 5、見張り、Nightmare）、`Stun`（17 s の倒れる → 止まる → 起き上がりと、入ったことの知らせが 1 回、3 s の短い気絶、倒れる 2 本の抽選〈40 回で両方〉と欠けた 1 本、走り → 気絶 → 起き上がり → 明け、2 回目、寝ている間に明けると姿勢を保って起き上がらない）、`Once`（ブレンド、自動のブレンドアウト、2 倍速、途中の停止、重ね掛け、欠けたクリップ）、`Clips`（名前と場所。**取り込んだ 18 本が揃い、スケルトンが `SK_WasamiEnemy_Skeleton` で、長さがテストの値と合う**。倒れる 2 本の `MeasureGetUpMove` が上の測った値と 0.1 以内。取り込みが変わったらここが落ちる）。

### 敵のアクタ（`AWasamiEnemy`）

本家のナース `BP_06_ReaperNurse`（親 `BP_DD_Character_Base` → `Character`）の CDO と部品の値を写す。

- **CDO**: タグ `Enemy`、`AutoPossessAI = PlacedInWorldOrSpawned`（エンジンの既定の `AIController` が付く。付くと移動の計算が走る）、`bUseControllerRotationYaw = false`。
- **カプセル**: 半分の高さ 118.05822（本家の上書き）、半径 34（本家は上書きしない。UE 5.8 の `ACharacter` の既定。UE4 から同じ値で、4.24 のソースは手元に無い）。当たりはエンジンの既定の `Pawn`。本家の基底の `AreaClass = NavArea_Obstacle` は 4.24 の `ShapeComponent` の既定と同じなので書かない（UE5 は `bUseSystemDefaultObstacleAreaClass`）。
- **移動**: `MaxWalkSpeed` 800、`RotationRate` (0, 300, 0)、`bUseControllerDesiredRotation`・`bOrientRotationToMovement` 真。`Set Walk State` は `bNormalWalk` で 350 / 800 を選ぶ。
- **メッシュ**（`CharacterMesh0`）: 相対位置 (−0.00006, −0.0002, −117.84394)・Yaw −90.00012（本家のまま。`SK_WasamiEnemy` も正面が +Y なので、アクタの前を向く。足はカプセルの底から 0.2 cm 上）、拡縮は X・Y・Z とも `MeshScale` = 229.05135 / 168.52719 = 1.3591（本作の値。2026-09-18 のユーザーの指示「Z軸スケールは本家の敵と同じ身長になるよう、敵ワサミモデルはX・Y・Zスケールを拡大する」。頭頂の骨どうしで合わせる: 本家のナース `nurse_idle1` の基準姿勢の `Nurse_TopOfHead_AuxSHJnt` が 229.05 cm〈その上の帽子を含むメッシュの頂は 246.35 cm〉、`SK_WasamiEnemy` の `head_end` が 168.53 cm〈髪を含むメッシュの頂は 170.0 cm〉。身長なので帽子は含めない。足はメッシュの原点にあるので拡縮しても床に立つ）、`AnimClass = UWasamiEnemyAnimInstance`。メッシュ `/Game/Wasami/Enemy/SK_WasamiEnemy` はソフト参照で、`OnConstruction` で読む（`WasamiAssets.h` の起動時の読み込みを避ける。シャードと同じ）。
- **BeginPlay**（中身は `BeginNurse`）: 基底どおり `bCanSpawn` が偽なら自分を消す（本家は既定が偽で、Zone 1 のレベルのスクリプト `Spawn Nurses` が `CanSpawn` を真にして出す。`SpawnEnemy` が同じことをする）。真なら、基底の `Ignore All Speed Barriers`（項目 8）の後、ナースの `Generate Random Point` と、`Make Choice` の 0.5 s ごとのループのタイマー（最初は 0.5 s 後）。
- **気絶**（ナースの Make Choice の DoOnce）: `SetState` は State を入れるだけ。判断（`MakeChoice`）が State == Stun を見たら、1 回だけ（`bStunRunning`）`StopMovementImmediately` → 17 s のタイマー → `EndStun` で State = Patrol・`bStunRunning` 偽。待っている間は判断は何もしない。待っている間の 2 回目の気絶は時間を延ばさず、State を Patrol にしてまた Stun にしても始め直さない（最初の 17 s で終わる）。`StopMovementImmediately` はナビの移動の `StopActiveMovement` も呼ぶので、**AI の経路の追従も止まる**（UE 4.24 も同じ。止められた移動の依頼は失敗として知らせる）。ナースの `Cloak(False)`（透明化）と気絶の台詞（`Nurse_Hospital_Zone01_Stunned`。項目 20 でワサミの声）は作らない。

### 判断（`MakeChoice`。本家のナースの `Make Choice` の残り）

行動ツリーは無く、0.5 s ごとの判断とエンジンの AI MoveTo（`UAIBlueprintHelperLibrary::CreateMoveToProxyObject`。BP の「AI MoveTo」の中身）だけで動く。

- **判断の順**（本家の Sequence）: 気絶でなく、薬投げ（`bThrowing`。作らない）でもなければ、(1) `bSeenPlayerRecently` が真なら `ChasePlayer` と 3 s の `RetriggerableDelay`（タイマーを毎回掛け直す。明けたら `bSeenPlayerRecently` 偽・`Reset Detection`）、偽なら `NotSeeingPlayer`。(2) その後で `CanSeePlayer` が真なら `bSeenPlayerRecently` = 真（追うのは次の判断から）。(3) プレイヤーから 1500 cm より遠いと透明化（`Cloak`。作らない）。
- **見失い方**: 追っている間は判断のたびに 3 s の遅延が掛け直されるので、**見えなくなっただけでは見失わない**（追跡はプレイヤーの今の位置へ向かう）。見失うのは Vanish（`PlayerVanish` で `bSeenPlayerRecently` 偽）と、判断が止まる気絶（最後の追跡から 3 s で遅延が明ける）だけ。
- **`CanSeePlayer`**: 前（`GetActorForwardVector`）とプレイヤーへの向き（`Normal`、許容 0.0001）の角度（`DegAcos`）が 100° 未満（距離の上限なし）かつ、自分の位置 → プレイヤーの位置の `LineTraceSingle`（`Camera` チャンネル、複雑、自分を除く）の当たりのアクタがプレイヤーのキャラクター。ほかのキャラクター（別の敵）に当たると見えない。Vanish 中はプレイヤーのカプセルが `Camera` を無視する（04 記録）ので、線が素通りして見えない。
- **`ChasePlayer`**: `bSeenPlayerRecently` 真、`PointOfInterest` = プレイヤーの位置、`SetWalkState(false)`（800）、`GetPlayerTarget()` へ受け入れ半径 5・`bStopOnOverlap` 偽で移動（成功も失敗も何もしない）。DoOnce（`Reset Detection` で開く）で発見の台詞（項目 20）と `OnCloseBy`。最初の追跡の 5 s 後の薬投げは作らない。
- **`NotSeeingPlayer`**: `PointOfInterest` が 0（許容 0.0001）なら `GetRandomPointDestination()` へ半径 50、終われば（成功でも失敗でも）`GenerateRandomPoint`。0 でなければ `PointOfInterest` へ半径 5、終われば `PointOfInterest` = 0。どちらも `SetWalkState(true)`（350）。
- **`GenerateRandomPoint`**: プレイヤーがいればその位置、いなければ自分の位置の周り 3000 の `K2_GetRandomReachablePointInRadius`（起点から辿り着ける点）。ナビのデータが無ければ `RandomPoint` は前のまま。
- **移動の依頼は判断ごとに前の依頼を止める**（`UPathFollowingComponent::RequestMove` が前の依頼を `Aborted` で終わらせ、その依頼の代理が失敗を知らせる。UE 4.24 も同じ）。そのため (a) 巡回の行き先は判断のたびに引き直される（行き先に向かう依頼は 1 つ前の判断で引いた点へ。`PointOfInterest` への移動も次の判断の依頼で消える）、(b) 巡回はプレイヤーの周り 3000 の点へ 0.5 s ごとに向きを変えながら、全体としてプレイヤーの方へ寄っていく。本家の BP をそのまま写した結果で、手を加えていない（2026-09-19 の PIE で、Zone 1 の迷路の 115 m 先から 30 s ほどでプレイヤーの前へ来た）。
- 受け入れ半径の 5 はプレイヤーのカプセルに当たって届かないので、追跡は依頼が続いたまま触れて止まる（PIE で中心の間 89 cm）。接触から先は捕獲（項目 9）。
- **派生が替える口**: `GetPlayerTarget()`・`GetRandomPointDestination()`（Zone 2 の型）。06 の追跡型と Zone 2 の型は下。
- **パワーの作用を PIE で確かめたこと**（2026-09-19。パワーはソケットに合わせて `UsePower` で実際に使った）: Primal Fear（Lv5）で、追っていた Zone 2 の見張りのナースが倒れて 17 s 動かず（`GetStunTimeLeft` が 17 → 0）、明けの最初の判断は歩き（350）で `PointOfInterest` へ向かい（気絶の間に見失っていた）、見えたので次の判断から走って接触した。オーブの代わりの `SetState(Stun, true)` も同じ 17 s と明けの見失い。Vanish の間は `bSeenPlayerRecently`・`CanSeePlayer` とも偽のまま、歩きで乱数の点を回り、3 m まで近づいても追わなかった。Telepathy は 6 体すべての位置に印を出し、9 s で消えた。Zone 1 の迷路（チェックポイント 5）で、歩いて巡回 → 振り向いて見つけ → 走って約 0.5 s で目の前、を収録した（収録は git の外の `Intermediate/DesktopAgent/shots/step6-*.mkv`）。
- **アニメとの受け渡し**: アニメは毎フレーム `IsStunned()` を読む（本家の ABP の `bStunned = State == 2`。`SetState` の直後から気絶の姿勢になり、止まるのは次の判断）。立ち上がりで `GetStunTimeLeft()` を読み、起き上がりを巡回に戻る瞬間に終える。残りは、気絶でなければ 0、判断の前なら「次の判断までの残り（判断のタイマーの残り）+ 17 s」、判断の後なら 17 s のタイマーの残り。エンジンではタイマーが移動とメッシュの更新（TG_PrePhysics）の後に進むので、アニメが読む残りはその前のフレームの終わりの値で、そのフレームの経過と合わせて合う。
- **エンジンのタイマーの刻み**（UE 5.8 の `FTimerManager`。UE4 も同じ作り）: タイマーは「期限を**過ぎた**最初の更新」で発火する（`InternalTime > ExpireTime`）。更新の外（BeginPlay・テストの本文）や発火の処理の中で入れたタイマーは保留になり、その更新の終わりの時刻から数え始める。そのため判断はフレームの粒で最大 1 フレーム遅れ、気絶の 17 s は判断のフレームの終わりから数える。アニメが判断の前に読んだ残りは実際の終わりより最大 2 フレーム短く、起き上がりの終わりの姿勢を最大 2 フレーム保ってから明ける（見た目には分からない）。

### 06 の追跡型（`AWasamiEnemy06Chase`。本家の `BP_06_ReaperNurse_06_Chase`）

Zone 1 の流れの `Spawn Nurses_06` が駐車場に 2 体出す（11 記録）。本家のナースの子で、`BeginPlay` は親のまま（0.5 s の判断も走る）、tick を自分のものに替える。

- **tick**: State == Stun なら気絶の DoOnce（`StartStun`。止まり、17 s 後に Patrol）だけ。そうでなければ毎ティック `ChasePlayer`（見えていなくても追う。`bSeenPlayerRecently` が毎ティック真になり、判断の 3 s の遅延も明けないので、**見失わない**。Vanish で偽になっても次のティックで戻る）。`bAttackDoor` の間は、そのうえで扉を突く DoOnce。
- **気絶**: 本家は判断と tick にそれぞれ DoOnce があり、どちらも 17 s 後に State = 0 にする。本作は 1 つ（`StartStun`）にまとめた。tick が先に始めるので終わりは tick の 17 s で、本家と同じ時刻になる（後から明ける判断側の DoOnce は State を 0 にし直すだけ）。`GetTimeToStunStart` は 0（次のティックで始まる）。
- **扉を突く**（本家の `PlayMontage(ReaperNurse_Needle_Attack_NoSound_Montage)` の DoOnce）: 始めに `PlayOnce(Chase_Charge)`（本家のモンタージュはナースの骨なので、頭を下げて突っ込むワサミのクリップで代用。Zone 2 の捕まる場面の「殴る」と同じ代用。仮）。時刻は本家のモンタージュの値で数える: `DoorHitTime` 0.413 s に通知（`OnNotifyBegin`）→ `RandomBoolWithWeight(0.5)` で `HitFX`。`DoorAttackSeconds` 0.717 s（長さ 0.967 − `BlendOutTriggerTime` 0.5 で blend out が始まり、既定の 0.25 s で終わる所。`OnCompleted`）に終わり → `RandomFloatInRange(0, 0.5)` 待って DoOnce を開く（0 は次のティック）。中断（`OnInterrupted`）で開かないのは、代用が中断されないので写さない。1 回あたり平均約 1.2 s なので、扉が破れるまでの 25 s に 1 体あたり約 20 回突く（2026-09-19 の PIE で、2 体の `HitFX` の塵が 1 秒に約 1 つずつ出た）。
- **`HitFX`**: カプセルの前 230 cm（本家の `ParticleSystem` 部品〈`Glass_fracture`、起こさない〉の相対位置）に `P_06_NurseDoorHit` を拡縮 0.5 で `SpawnEmitterAtLocation`（向き 0・自動で消える・使い回さない）、`Audio`（カプセルに付けた `UAudioComponent`、`20-Elevator_Slams`〈3 本のランダムの SoundCue、`01_Lobby_Attenuation`、ピッチ 1.8〉、自動で鳴らない）を `Play(0)`、自分の位置を中心に `01_Hotel_Lobby_ElevatorShakeStop` を `PlayWorldCameraShake`（内 0・外 3000・減衰 1・中心へ向ける）。素材は `WasamiDDTools.import_dd_gimmicks`（08 記録）。
- **`Chasing`**: 常に真（本家の上書き）。薬投げ・透明化（`Pill Throw`・`Cloak`）は本家でも空。

### Zone 2 の迷路の型（`AWasamiEnemyZone2`。本家の `BP_06_ReaperNurse_Zone2`）
Zone 2 の `Maze Transition` の `Spawn Nurses` が `NurseSpawn_4`・`_1`・`_2` に 1 体ずつ出す（11 記録）。迷路は 2 つの階をリフト（12 記録）でしかつながないので、関数 2 つだけを上書きし、ほかはナースのまま（CDO の上書きは体の材質だけで、ワサミには効かない）。
- **階**: `IsUp` = 自分の Z > 640、`IsPlayerUp` = プレイヤーの Z > 610（プレイヤーがいなければ Z 0 と読む。本家の None の読みと同じ）、`IsSameLevelAsPlayer` = 両者が等しい。Z はカプセルの中心（床の約 118 cm 上）なので、下の階の床の Z 約 0 と上の階の約 535 の間を分ける値。
- **`GetClosestLift`**: 本家の `GetAllActorsOfClass(BP_06_LiftBase)` と `MoreporkFunctions.GetFurthestOrClosestActor(FromLocation (0, 0, 0), UseClosest 真)`。対象は `AWasamiLift`（本家の `BP_06_Lift_03`・`_04` の 10 基。角のリフト `BP_06_LiftBase_Corner` は `BP_06_LiftBase` の子でないので入らない）。**起点は世界の原点でナースの位置ではない**ので、どのナースもいつも同じリフト（原点に最も近い `hospital_zone_02_lifts_lift_04_61`、(6303, −439, −30)）を選ぶ。本家のまま写した。2 乗の距離が今の最良以下なら置き換える（同じ距離なら後の方）。最良の初めは 1e9 なので、原点から 31623 cm より遠いリフトは選ばれない（Zone 2 には無い）。戻り値の位置はそのリフトの `Move Location`（床〈`LiftMesh`〉の 149.5 cm 上。床と一緒に動く）の今の位置、リフトが無ければ 0。
- **`GetPlayerTarget`**: 同じ階ならプレイヤー、違えばそのリフト（`ChasePlayer` がそのアクタへ半径 5 で移動する）。**`GetRandomPointDestination`**: 同じ階なら `RandomPoint`、違えばそのリフトの `Move Location`。
- 下の階のナースはリフトに乗ると上がり（キャラクターが乗る間 535 cm 上がる。12 記録）、上の階で `IsUp` が真になってプレイヤーを追う。上の階のナースが下の階のプレイヤーを追うときは、リフトのアクタ（下の階の高さ）へ向かうので、上の階の NavMesh の、リフトの穴の上に焼かれた面（01 記録の「ナビゲーション」）を通る。
- PIE（2026-09-19、Zone 2 をチェックポイント 9 で開く）: `Maze Transition ` で 3 体が下の階に出て巡回する。プレイヤーを上の階 (7295, −1176) に置くと、3 体とも `IsSameLevelAsPlayer` が偽になって `lift_04_61` へ歩き、台に乗って上がり（Z 312 → 650）、上の階で見つけて 800 で追い、プレイヤーの手前で止まった（約 30 s）。プレイヤーを下の階 (2978, 70) に移すと、上の 3 体が同じ `lift_04_61` の所から下りて約 10 s でプレイヤーの手前へ来た。

### 見張りと視界コーン（`AWasamiEnemySentry`・`AWasamiViewcone`。本家の `BP_06_ReaperNurse_Sentry`・`BP_06_Miniboss_viewcone(_Nurse)`）
Zone 2 のミニボスの廊下（「GET PAST THE NURSES」）の高い所（Z 約 430〜450）に本家のレベルが置く 6 体。置くのはレベルの組み立て（01 記録の `_flow` の見張り: 位置と向き・`CanSpawn`・`Offset`・`Jump Down Spot` の相対位置）、見始めるのはゾーンの流れの `Activate MiniBoss Enemies`（11 記録）。
- **見張りの BeginPlay は空**（`BeginNurse` を空で上書き）: 判断しない・動かない・`CanSpawn` も見ない（本家の `ReceiveBeginPlay` が親を呼ばない）。待機は `bAggressiveIdle` の `Idle_Alert`。
- **`Activate`**: 子のコーンの `Offset` に自分の `Offset`（本家のレベルで `ReaperNurse_Idle_Alert3`・`4`・`7` が 10、ほかは 0）を入れ、`Initialize`。
- **コーンの `Initialize` → `UpdateSight`**: 0.3〜0.5 s の乱数（1 回だけ引く）のループのタイマー。`UpdateSight` はまず `Delay(Offset)`（待つ間に来た呼び出しは捨てる。2 回目からは `Offset` 0 の `Delay` で次のティックだが、ここではすぐ調べる）→ `Offset` = 0 → (1) `PlayerInsideCone` かつ `PlayerInFullView` なら DoOnce で `Player Spotted` を親のアクタ（`GetParentActor`）と持ち主（`GetOwner`。子のアクタは持ち主を持たないので null）に送る → (2) DoOnce で `InitializeFinished` と、`bAutoOn` なら `TurnOn`。見つけて見張りがコーンを消したら (2) は飛ばす。
- **`PlayerInsideCone`**: `bOn`、コーンのアクタの前（見張りの頭 72 cm 上で 20° 下向き）とプレイヤーへの向き（`Normal`、許容 0.0001）の角度（`DegAcos`）< `Angle`、`GetDistanceTo(プレイヤー)` < `Length`。**`PlayerInFullView`**: `bOn`、コーン → プレイヤーの `LineTraceSingle`（`Camera`、複雑、親のアクタと自分を除く）が当たり、当たりの距離 ≤ `Length`、当たったのがプレイヤー（本家はプレイヤーのクラスへのキャスト）、かつ `PlayerInsideCone`。Vanish 中はプレイヤーのカプセルが `Camera` を無視するので見つからない。
- **`TurnOn`**: 親の `Start Looking` → `Delay(1)`（待つ間の 2 回目は捨てる）→ `bOn` 真・`Fade In` を今の位置から前へ。**`TurnOff`**: 親の `Stop Looking`、`bOn` 偽、`Fade In` を今の位置から後ろへ。`Fade In` は 0.5 s の曲線（0 → 1 の 3 次、自動の接線で平ら）で、`Plane` の `Opacity` を書く（アクタのティックで進め、端で止める）。
- **`_Nurse`**: `InitializeFinished` で 10 s のループの `Turn`（真偽を反転し、真なら `TurnOff`、偽なら `TurnOn`）。見始めてから 10 s 見て、10 s 消え、点いた 1 s 後にまた見る、を繰り返す。
- **見張りの `Player Spotted`**（DoOnce）: 実績 `06_NurseAlert` は作らない。`Viewcone` 部品を `DestroyComponent`（コーンのアクタも消える）、`bChasing` 真（以後ずっと。`IsChasing` はこれを返し、ナースの `Seen Player Recently` が偽になっても真）、`GetDirectionUnitVector(自分, JumpDownSpot)` の X・Y × 400 と Z 500 で `LaunchCharacter`（XY・Z とも上書き）、親の `BeginNurse`（ナースの判断が 0.5 s ごとに始まる）。本家はこの後 tick の Gate を開き、ナースの tick（スケートの音。作らない）が動き出す。
- **`Start Looking`・`Stop Looking`**: `bVarIdle` を偽・真に（本家のナースの変数。ABP の Alert の 2 本目の組が読むが、本作のアニメは Alert 1 本なので読まない）。
- **地図の印**: コーンの `Scene` の下の `Plane`（エンジンの `Plane`、`map_enemy_search_Mat`。`_Nurse` は (837.8, 0, 0)・拡縮 (17.006, 9.620, 28) でコーンに沿う扇）と `Plane1`（`0_DotCircle_Mat`、(0, 0, 1000)）。`BeginPlay` で隠しを解き（本家の CDO は `bHidden`）、次のティックで `Plane` の `Opacity` を 0 にして `AddLocalOffset(0, 0, 1000)`。両方ともコーンの 1000 上（ピッチ −20 の座標なので水平にも前へずれる）にあり、本家はプレイヤーの地図のキャプチャの `Show Only` が `BP_06_Miniboss_viewcone` を全部足す。本作はコーンにタグ `dd_minimap` を付け、プレイヤーの `RefreshMinimapContents`（02 記録）が拾う。本家は天井の上に置いて本編から見えなくしているが、本作は地図の板と同じく `bVisibleInSceneCaptureOnly`。影なし・当たりなし・ナビに効かない。材質はソフト参照（`/Game/DD/Blueprints/06_Hospital/Miniboss/Tex/map_enemy_search_Mat`・`/Game/DD/ThirdParty/M5VFXVOL2/Materials/Master/0_DotCircle_Mat`）で `BeginPlay` に読む。材質は `import_dd_tablet` が作る（03 記録の `make_viewcone_materials`。本家の Unlit・半透明を、キャプチャの `SCS_BaseColor` に写る Default Lit・Masked にした推定）。
- **見つける前の気絶**: 判断が動いていないので、Primal Fear で State が Stun になると気絶の姿勢のまま 17 s の DoOnce が始まらない（本家も同じ。見つけて判断が始まると 17 s で明ける）。コーンは見続ける。
- **PIE で確かめたこと**（2026-09-19、Zone 2 をチェックポイント 8〈`Miniboss Transition `〉で）: 6 体が本家の位置の棚の上（床の Z 311〜334）に立ち、コーンは `Offset` 0 の 3 体と 10 の 3 体が 10 s ずつ交互に点く・消える。点いているコーンでも角度の外（`Alert3` から 1029 cm・35° 横）のプレイヤーは見つけない。消えているコーンの中に立ったプレイヤーを、`Alert_5` がコーンが点いた直後に見つけ、コーンを消して跳び（Z 564 まで上がって）`Jump Down Spot` の側へ降り、追ってプレイヤーの 90 cm まで来た（接触の先は項目 9）。6 体の `Jump Down Spot` の下の床はどれも NavMesh の上で、プレイヤーの出発点への道がつながる。地図の印は 03 記録の「確かめたこと」。

### 取り込み
1. `_extract_textures`: glb に埋め込まれた PNG を `Intermediate/Pipeline/wasami/enemy/T_WasamiEnemy_<BaseColor|MetallicRoughness|Normal>.png` に書き出し、`dd_stage.import_texture` で取り込む（`TEXTURES`: 色は sRGB・`TEXTUREGROUP_Character`、金属と粗さは線形・`TEXTUREGROUP_CharacterSpecular`、法線は `TC_Normalmap`・`TEXTUREGROUP_CharacterNormalMap` で緑を反転〈glTF は Y 上向き〉）。4096² はそのまま（ストリーミングが描く分の mip だけ載せる）。
2. `M_DD_WasamiGltf`（`dd_assets.material` + `_build_master`）: glTF の metallic-roughness の係数 1 の形。色 → Base Color、金属と粗さの B → Metallic、G → Roughness、法線 → Normal。片面、`used_with_skeletal_mesh`。`MI_WasamiEnemy` はそのインスタンスでテクスチャ 3 枚を入れる。
3. `ensure_skeletal_pipeline`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を `PL_Wasami_Skeletal` に写し、種類ごとのフォルダなし、`use_source_name_for_asset` 偽・`asset_name` 空（こうするとメッシュは glTF のメッシュの節の名前〈節が無名なら `<ファイル>_node_<番号>`。2026-09-18 に `dd_skeletal` で分かった〉、スケルトンと物理アセットはその `_Skeleton`・`_PhysicsAsset`、アニメは glTF のアニメの名前そのままになる。Interchange の `ImplementUseSourceNameForAssetOption`）、材質とテクスチャの取り込みなし、スタティックメッシュなし、Nanite なし、物理アセットあり、モーフなし、アニメあり・30 Hz で焼く（引数 `pipeline`・`sample_rate` で別の管を別の速さで作れる。本家の骨入りのメッシュの `dd_skeletal` が `PL_DD_Skeletal` を 24 Hz で作る。01・12 記録）。
4. `_import_model`: 前処理した glb（メッシュと節の名前を `SK_WasamiEnemy` にしてある）を `/Game/Wasami/Enemy` に置き換えで取り込み（既にあるアセットは同じオブジェクトに書き戻す。**ファイル名はどのアセットとも違う `WasamiEnemy.glb`**: UE 5.8 の `InterchangeManager.cpp` は、置き換えの取り込みでファイル名と同じ名前のアセットが行き先にあると、そのアセットだけの再取り込みに変える。2026-09-18 まで `SK_WasamiEnemy.glb` だったので、2 回目からはメッシュだけが置き換わりアニメは最初の取り込みのままだった）、スロット 1（`BakedMaterial`）に `MI_WasamiEnemy` を入れ、`ROLES` のアニメが全部あるかを確かめる。取り込みは呼び出しの中で終わる。

## 作るアセット

| パス | 中身 |
|---|---|
| `/Game/Wasami/Enemy/SK_WasamiEnemy` | スケルタルメッシュ。高さ 170 cm（`head_end` 168.5 cm）、幅 148 cm（基準姿勢の腕）。敵のアクタは 1.3591 倍で描く（上の「メッシュ」）。**正面は +Y、左手は +X**（UE のマネキンと同じ。アクタではメッシュを Yaw −90 にする。本家のナースのメッシュも Yaw −90） |
| `…/SK_WasamiEnemy_Skeleton` | 骨 28、根 `pelvis`（高さ 88.5 cm） |
| `…/SK_WasamiEnemy_PhysicsAsset` | Interchange の自動の物理アセット |
| `…/A_WasamiEnemy_<役>` | 上の表の 18 本（2026-09-18 に `Stun_Loop`・`Stun_Recover`・`BeHit_FlyUp`・`Knock_Down`・`Push_Up_To_Idle` を消した） |
| `…/T_WasamiEnemy_*`・`MI_WasamiEnemy`、`/Game/Pipeline/Materials/M_DD_WasamiGltf` | 材質 |
| `/Game/Pipeline/Interchange/PL_Wasami_Skeletal` | 取り込みのパイプライン（`paths.SKELETAL_PIPELINE`） |
| `Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb`・PNG 3 枚 | 前処理の出力（git の外。元の glb の BIN をそのまま持ち、使われなくなった元のアニメの accessor も残る） |

## 原作データの根拠
- モデルとモーションはユーザーの作ったもの（2026-09-18 の指示「`enemy_wasami_v3`・`wasami_mochi_v3`・`boss_wasami` をそれぞれ使用」、捕獲は「旧 glb の 3 本を流用」）。役の対応は一覧（`.claude/references/enemy-wasami-motions.md`）。
- アニメの再生の木と値: `pak_reference_2/_assets/DDeception/Content/Animation/Enemies/Nurse/Reaper/nurse_idle1_Skeleton_AnimBlueprint.json`（`BakedStateMachines` の遷移 3 本の `CrossfadeDuration`・`BlendMode`、`AnimGraphNode_BlendListByBool` 4 つの `BlendTime`・`BlendType`〈0.25 = 根の気絶と走り、1.0 = Alert、0.1 = Alert の 2 本〉、シーケンスプレーヤーの `PlayRate` 1）と、同じ場所の `_bytecode` の `.txt`（`bStunned = (BP_06_ReaperNurse.State == 2)`、`Speed = VSize(TryGetPawnOwner().GetVelocity())`、`bAgressiveIdle = bAggressiveIdle`、Skating → Stop は Speed < 5）。
- 気絶の長さ 17.0 s は `BP_06_ReaperNurse` の Make Choice の `Delay 17.0`。起き上がりをその中に収めるのはユーザーの「明けに起き上がる」の読み（本家に起き上がりのアニメは無く、0.25 s のブレンドで戻る）。
- 判断: `_bytecode/…/Nurse/BP_06_ReaperNurse.txt` の `Make Choice`（@7107〜@7523。`python Tools/dd/bp_flow.py <file> "Make Choice"` で順が読める）、`Chase Player`（@7524〜@7935、DoOnce @5624・@5639 の `broadcast CloseBy`、`Reset Detection` @10646）、`Not Seeing Player`（@7940〜@8473）、`Generate Random Point`（@8474〜@8794）、移動の代理の `OnSuccess_C62BD9…`・`OnFail_…`（@3910・@3952 → `Generate Random Point`）、`…_A0F68D…`（@5551・@10818 → `Point Of Interest` = 0）、`…_D93573…`（@5319・@9789。何もしない）、関数 `Can See Player`・`Player Target`・`Random Point Destination`・`Distance To Player`。エンジンの挙動は UE 5.8 のソース（`AIBlueprintHelperLibrary.cpp` の `CreateMoveToProxyObject` と代理の `OnMoveCompleted`・`OnNoPath`・`OnAtGoal`、`AIController.cpp` の `MoveTo`、`PathFollowingComponent.cpp` の `RequestMove`、`NavMovementComponent.h` の `StopMovementImmediately`、`NavigationSystem.cpp` の `K2_GetRandomReachablePointInRadius`）。本家のナースは AI のコントローラを替えない（`AIControllerClass` を上書きしない）ので、エンジンの既定の `AIController`。
- 敵のアクタの値: `pak_reference_2/_assets/DDeception/Content/Blueprints/Characters/Nurse/BP_06_ReaperNurse.json`（CDO と `CollisionCylinder`・`CharMoveComp`・`CharacterMesh0`）、`BP_06_ReaperNurse_Sentry.json`（`bAggressiveIdle`）、`_bytecode/…/BP_06_ReaperNurse.txt`（BeginPlay の `K2_SetTimer('Make Choice', 0.5, 真)`、Make Choice の入口 @7107 の `State == 2` → DoOnce → `StopMovementImmediately` → `Cloak(False)` → `Talk` → `Delay 17.0` → @355 `State = 0` と DoOnce を開く、`Set State`・`Get State`〈ByteConst 0〉・`Player Vanish`〈@10871 `Seen Player Recently = False`〉・`No Telepathy`〈偽〉、`Set Walk State`）、`_bytecode/…/Shared/BP_DD_Character_Base.txt`（`CanSpawn` と `Ignore All Speed Barriers`）、`_bytecode/DDeception/Content/06_Hospital_Zone_01.json`（`Spawn Nurses` が `SetBoolPropertyByName(CanSpawn, True)`）。
- 06 の追跡型: `_bytecode/…/Nurse/BP_06_ReaperNurse_06_Chase.txt`（`ReceiveTick` @1029〜@1150: `State == 2` → DoOnce @882〈`StopMovementImmediately`・`Delay 17`・@66 `State = 0`〉、`bAttackDoor` → `Chase Player` と DoOnce @435 の `CreateProxyObjectForPlayMontage`、そうでなければ `Chase Player`。`OnNotifyBegin_9B2D…` @109 の `RandomBoolWithWeight(0.5)` → `Hit FX` @1153、`OnCompleted_9B2D…` @246 の `Delay(RandomFloatInRange(0, 0.5))` → @43 DoOnce を開く、`OnBlendOut`・`OnInterrupted` は何もしない、関数 `Chasing` は真）、`_assets/…/Nurse/BP_06_ReaperNurse_06_Chase.json`（`Audio` の `20-Elevator_Slams`・自動で鳴らない、`ParticleSystem` の相対位置 (230, 0, 0)・`Glass_fracture`・自動で起きない）、`_assets/…/Animation/Enemies/Nurse/Reaper/ReaperNurse_Needle_Attack_NoSound_Montage.json`（長さ 0.9667、`PlayMontageNotify` 0.4132、`BlendOutTriggerTime` 0.5）。
- Zone 2 の迷路の型: `_bytecode/…/Nurse/BP_06_ReaperNurse_Zone2.txt`（関数 `is Up?`〈Z > 640〉・`is Player Up?`〈プレイヤーの Z > 610〉・`Same Level As Player?`・`Get Closest Lift`〈`GetAllActorsOfClass(BP_06_LiftBase_C)` → `GetFurthestOrClosestActor(VectorConst (0, 0, 0), …, True, Self, …)` → `Move Location.K2_GetComponentLocation`〉・`Player Target`・`Random Point Destination`）、`_bytecode/…/Macros/MoreporkFunctions.txt` の `GetFurthestOrClosestActor`（UseClosest なら最良 1e9 から、`CurrentDist > BestDistance` でなければ置き換え）、`_assets/…/Nurse/BP_06_ReaperNurse_Zone2.json`（CDO の上書きは体の材質だけ）、`_assets/…/Lifts/Zone2/*.json`（`BP_06_Lift` の親は `BP_06_LiftBase`、`BP_06_LiftBase_Corner` の親は `Actor`、`Move Location` は `LiftMesh` の子で (0, 0, 149.535)）、`_levels/06_Hospital_Zone_02.full.json`（リフトの位置）。
- 見張り: `_bytecode/…/Nurse/BP_06_ReaperNurse_Sentry.txt`（`ReceiveBeginPlay` は空、`Activate` @758〈`Viewcone.ChildActor` を `BP_06_Miniboss_viewcone` にキャストして `Offset` を入れ `Initialize`〉、`Player Spotted` @1035 → DoOnce @720 → @15〈`Set Struct Achievement('06_NurseAlert', 1)`・`Viewcone.K2_DestroyComponent`・`Chasing = True`・`GetDirectionUnitVector` の X・Y × 400 と 500 で `LaunchCharacter(…, True, True)`・親の `ReceiveBeginPlay`・Gate を開く @544〉、`ReceiveTick` @1018〈Gate が開いていれば親の `ReceiveTick`〉、`Start Looking` @1040・`Stop Looking` @1052〈`bVarIdle`〉、関数 `Chasing`〈自分の `Chasing`〉、`UserConstructionScript`〈`Initial Yaw` = ヨー。読む所は無い〉、`Update Rotation`〈呼ぶ所は無い〉）、`_assets/…/Nurse/BP_06_ReaperNurse_Sentry.json`（`bAggressiveIdle` 真、`Viewcone_GEN_VARIABLE` の `ChildActorClass` `BP_06_Miniboss_viewcone_Nurse`・(0, 0, 72)・ピッチ −19.99996、`Jump Down Spot` は `CollisionCylinder` の子）。
- 視界コーン: `_bytecode/…/06_Hospital/Miniboss/BP_06_Miniboss_viewcone.txt`（`ReceiveBeginPlay` @780〈`SetActorHiddenInGame(False)`・`Delay(0)`・`Plane` の `Opacity` 0・`K2_AddLocalOffset(0, 0, 1000)`〉、`Initialize` @1443〈`K2_SetTimer('Update Sight', RandomFloatInRange(0.3, 0.5), True)`〉、`Update Sight` @1384〈`Delay(Offset)` → @212 `Offset = 0` → Sequence: 2 つの関数 → DoOnce @391 → @406 親と持ち主の `Player Spotted`／DoOnce @346 → `Initialize Finished` → `bAutoOn?` なら `Turn On`〉、`Turn On` @1013〈親の `Start Looking`・`Delay(1)` → @736 `bOn = True`・`Fade In.Play`〉、`Turn Off` @1204〈親の `Stop Looking`・`bOn = False`・`Fade In.Reverse`〉、`Fade In__UpdateFunc` @1546、関数 `Player Inside Cone?`・`Player In Full View?`〈`LineTraceSingle(自分, プレイヤー, TraceTypeQuery2 = Camera, 複雑, [親], None, …, 自分を除く)`、`BP_DD_PlayerCharacter` へのキャスト、`Distance ≤ Length`〉）、`BP_06_Miniboss_viewcone_Nurse.txt`（`Initialize Finished` @68〈`K2_SetTimer('Turn', 10, True)`〉、`Turn` @124〈真偽を反転し、真なら `Turn Off`〉）、`_assets/…/Miniboss/BP_06_Miniboss_viewcone.json`（CDO の `Length` 1000・`Angle` 45・`bAutoOn?` 真・`bHidden` 真、`Plane` と `Plane1` の位置と材質・当たりなし、`Fade In` の曲線〈0 s 0・0.5 s 1、3 次・自動〉）、`BP_06_Miniboss_viewcone_Nurse.json`（`Length` 1500・`Angle` 20、`Plane` の位置と拡縮）、`BPI_06_Viewcone.json`、`Tex/map_enemy_search_Mat.json`（Unlit・半透明、`Opacity` パラメータ）、`_bytecode/…/Main/BP_DD_PlayerCharacter.txt` の `Show Only`（@7966 `GetAllActorsOfClass(BP_06_Miniboss_viewcone)` を `ShowOnlyActors` に足す）、`_levels/06_Hospital_Zone_02.full.json` の `ReaperNurse_Idle_Alert3`〜`7`・`_Alert_5`（`Offset`・`Jump Down Spot` の相対位置・`CollisionCylinder` の変換）。
- 本家のナースのほかの部品（捕獲の判定 `Sphere`〈半径 54.928、Pawn だけ Overlap。捕獲は State ≠ Stun のときだけ〉は項目 9、上空の板 `StaticMesh`〈`M_Enemy`、(0, 21.9, 1117.8)、拡縮 (2.52, 2.52, 10)。地図の印と推測〉は項目 10、`Talk Audio` は項目 20。`Camera`〈本家の病院の捕獲用〉・`PillSpawn`・`Skate Audio`・`Cloak Timeline` は作らない）。

## 依存関係
- `pipeline/gltf.py`（glb の読み書き・標本化・四元数）、`dd_stage`（`import_texture`・`_Graph`・`VERSION_TAG`）、`dd_assets`（`material`・`material_instance`）、`paths`（01 記録）。
- エンジン: `InterchangeManager`・`InterchangeGenericAssetsPipeline`、`SkeletalMesh`・`AnimSequence`。
- アニメの再生: エンジンの `FAnimInstanceProxy`（`PreEvaluateAnimation`・`Evaluate`）、`FAnimationRuntime::BlendPosesTogether`、`FAlphaBlend::AlphaToBlendOption`、`WasamiAssets::Path`（00 記録）。追加のモジュールは要らない（`Engine` だけ）。
- 06 の追跡型: `UAudioComponent`・`UGameplayStatics::SpawnEmitterAtLocation`（Cascade）・`PlayWorldCameraShake`、`/Game/DD` の素材 3 つ（08 記録の `import_nurse_door_hit`。ソフト参照で、音は BeginPlay、粒子と揺れは最初の `HitFX` で読む）。出すのは Zone 1 の流れ（11 記録の `SpawnEnemy`）。
- 敵のアクタ: `ACharacter`・`UCharacterMovementComponent`・`FTimerManager`、`IWasamiEnemyInterface`（04 記録）、`WasamiAssets::Path`。判断はモジュール `AIModule`（`UAIBlueprintHelperLibrary`・`UAIAsyncTaskBlueprintProxy`・既定の `AIController`）・`NavigationSystem`（`UNavigationSystemV1`）と、両ゾーンの NavMesh（01 記録の「ナビゲーション」、設定は 00 記録）。
- Zone 2 の迷路の型: `AWasamiLift`（12 記録）の位置と `GetMoveLocation()`。出すのは Zone 2 の流れ（11 記録）。
- 見張りと視界コーン: `UChildActorComponent`、`UKismetSystemLibrary::LineTraceSingle`、`FRichCurve`（`Fade In`）、プレイヤーの地図のキャプチャのタグ `dd_minimap`（02 記録）、`/Game/DD` の材質 2 つ（03 記録の `make_viewcone_materials`）。見始めるのは Zone 2 の流れ（11 記録の `Activate MiniBoss Enemies`）。
- 使う側: アニメの再生が取り込んだクリップを名前で読み、持ち主の敵のアクタから値を読む。パワー（04 記録）の Primal Fear（球の重なりの Pawn とインターフェース）・Vanish（タグ `Enemy` とインターフェース）・Telepathy（インターフェース）が敵のアクタに届く。2 つの状態の混ぜ `FWasamiStateBlend` はガレージリフトのアニメ（12 記録）も使う。

## 既知の制約・注意点
- glb に**ルートの骨が無い**ので、UE のルートモーションは使えない。前へ進むアニメは前処理でその場の形にした（捕獲の 3 本は捕獲の別室で使うので進んだまま）。
- Interchange は「Node [SK_WasamiEnemy] with a skinned mesh is not root」と警告する（根 `target_character` は単位の変換なので効かない）。
- 追跡中の変化 `Chase_VaultLand` の元は台の上から跳び降りる動きで、そのままでは平らな廊下で宙に浮いて見え、着地後の約 0.7 s は立ったまま体が滑った（2026-09-18 の PIE）。前処理の `vault` で床から跳び越える形にした（上）。見えない障害物を跳び越える形なので、前に物が無い所では不自然かもしれない（要確認）。着地からの 0.8 s（かがんで起き上がる間）は、ほかの変化と同じく体が前へ滑る。
- 気絶のモーションは、載せ替えの物差しとしては頭だけ 4° 合わない（旧と v3 で作り直されている）。捕獲の 3 本には関係しない。
- 前処理は純粋な Python（エディタの Python に numpy が無い）で、全体で数秒かかる。
- クリップの名前は `ClipNames` と取り込みの `ROLES` の両方にある。役を足す・名前を変えるときは両方を直す（`Wasami.Enemy.Anim.Clips` が食い違いを見つける）。
- クリップはゲームのワールドでしか読まないので、エディタのレベルに置いた敵は参照姿勢（腕を広げた形）で見える。
- 1 回再生にアニメ通知・終わりのイベントは無い。終わりは `IsPlayingOnce()` で見る。
- `FAnimInstanceProxy::IsSkeletonCompatible` は UE 5.8 で非推奨（警告 C4996）なので使わない。
- 敵のアクタのカプセルは本家のナースの高さ（約 236 cm）。ワサミは 2026-09-18 に `MeshScale` 倍にして、メッシュの頂 231 cm（PIE の待機で `head_end` 228.9 cm）になった（それまでは 170 cm で、カプセルより 66 cm 低かった）。Telepathy の印はアクタの位置（カプセルの中心、床から 120 cm）に付くので、今は腰の高さに重なる（本家のナースも同じ位置。大きくする前は胸に重なって見えた）。
- 捕獲の 3 本は前へ進んだ形のままなので、1 回再生が終わると体が元の位置へ跳んで戻る（`Capture_2` は 5.7 m）。捕獲の別室での使い方は項目 9。
- `bCanSpawn` が既定で偽なので、エディタのレベルに置いた敵は PIE で消える（置くなら詳細で真にする）。エディタのレベルでは参照姿勢で見える。
- 気絶の間は判断が止まり、`StopMovementImmediately` が経路の追従も止める（2026-09-19 の PIE で、プレイヤーが 12 m 離れても 17 s 動かなかった。起き上がりの移し替えだけ動く）。
- 約 20 m より遠い 2 点の道は途中までになることがある（01 記録）。巡回の点はプレイヤーの周り 3000 なので、遠くからでも途中までの道を繰り返してプレイヤーへ寄る。
- 06 の追跡型は毎ティック移動の依頼を出し直す（本家どおり。1 秒に数十回の経路探索）。2 体なので重さは見えない。
- 06 の追跡型の扉を突く代用 `Chase_Charge`（0.53 s）は本家の針の突き（0.97 s）より短く、次の突きまで約 0.2〜0.7 s 立つ。見た目は大目標 3 で詰める（作業一覧の項目 28）。
- Zone 2 の迷路のナースは、目当てのリフト（原点に最も近い `lift_04_61`）でなくても、通り道のリフトの台に乗るとそのリフトで上がる（リフトはキャラクターが乗ると上がる。12 記録）。本家も同じ作り。2026-09-19 の PIE では、上の階に置いたプレイヤーへ 3 体のうち 2 体が `lift_04_61` で上がり、1 体はそれより先に上の階にいた。
- 視界コーンの `Update Sight` の 2 回目からの `Delay(0)` は本家では次のティックだが、本作はすぐ調べる（1 コマの差）。`BeginPlay` の `Delay(0)` の後の板の持ち上げは次のティックで本家どおり。
- **見つける前に気絶した見張りは、アニメだけが 17 s で起き上がる**: 判断が動いていないので State は Stun のまま（上の「見つける前の気絶」。本家どおり）だが、アニメは立ち上がりで `GetStunTimeLeft()`（判断のタイマーが無いので 0 + 17 s）を読み、17 s で起き上がって立つ。2026-09-19 の PIE で、Primal Fear を浴びた棚の上の見張りが約 14 s で起き上がりの移し替えにより最大約 0.7 m 動いた（棚からは落ちなかった）。見つけて跳び降りた後の 17 s は立ったまま止まり、明けて追う。本家の ABP は State が 0 に戻るまで気絶の姿勢のまま。見た目の差だけなので直していない（作業一覧の項目 28。詰めるなら、見張りの `GetTimeToStunStart` を見つけるまで大きな値にし、アニメが残りを読み直す口を足す）。
- 起き上がりの移し替え（最大で約 0.7 m）はスイープしないので、壁際で倒れるとカプセルが壁に掛かることがある。キャラクターの移動が押し出すのに任せている（`TODO(仮)`。PIE では廊下の真ん中でしか見ていない）。

## 変更履歴
- 2026-09-18: 初版。敵ワサミの素材の取り込み（`dd_enemy.py`、原本 2 つ）を記録
- 2026-09-18: アニメの再生 `UWasamiEnemyAnimInstance`（本家の ABP の木・気絶の位相合わせ・1 回再生の口）とテスト `Wasami.Enemy.Anim.*` を追加
- 2026-09-18: 敵のアクタ `AWasamiEnemy`（本家のナースの部品・`CanSpawn`・0.5 s の判断と 17 s の気絶・インターフェース・`SpawnEnemy`）とテスト `Wasami.Enemy.Actor.*` を追加。アニメの再生が持ち主の敵から値を読むようにした
- 2026-09-18: PIE で確かめた（立ち姿・歩き・走り・気絶と明け・Telepathy・Vanish・1 回再生 9 本。値は `observations/README.md`）。足の運びの速さを測り直し、`Run_Nightmare` の分母を 460 → 500 にした
- 2026-09-18: 追跡中の変化 `Chase_VaultLand` を前処理の `vault` で床から跳び越える形にし（台の高さ 76.6 cm を離れるまで下げる・2.4 s で切る・−31.7° 回す）、前処理の glb を `WasamiEnemy.glb` に改名した（`SK_WasamiEnemy.glb` ではメッシュだけの再取り込みになり、アニメが置き換わっていなかった）。テストの長さの期待を直した（作業一覧の項目 4 のステップ 4b）
- 2026-09-18: 敵のアクタのメッシュを X・Y・Z とも `MeshScale`（1.3591。本家のナースと頭頂の骨の高さをそろえる）倍にし、再生の速さの分母（歩幅の速さ）も同じ倍率にした。テストの期待値を直し、`Defaults` に拡縮を足した（ユーザーの指摘「敵ワサミが極端に小さい」）
- 2026-09-18: 気絶のアニメを「倒れる（`BeHit_FlyUp`・`Knock_Down` のどちらか）→ 寝返り → `push_up_to_idle` で起き上がる」に替えるため、取り込みの役を `Stun_FlyUp`・`Stun_KnockDown`・`Stun_GetUp_FlyUp`・`Stun_GetUp_KnockDown` にした（`stun_fall`・`stun_get_up`。ユーザーの指示と回答「ランダム」「Claude が寝返りを作る」）。`Stun_Loop`・`Stun_Recover` と役なしの 3 本はやめた。アニメの再生はまだ古いクリップを探す（作業ブランチ `feature/enemy-stun-knockdown` のステップ 2 で直す）
- 2026-09-18: アニメの再生の気絶を「倒れる 2 本からランダム → 終わりで止まる → 起き上がり（気絶の終わりに終わる）」にし、起き上がりに入る更新で敵を移す（`MoveToGetUp`・`MeasureGetUpMove`）ようにした。明けてブレンドアウトする間は気絶の時刻を進めない。テスト `Anim.Stun`・`Anim.Clips`・`Actor.Stun` を直した（作業ブランチのステップ 2。PIE で確かめた）
- 2026-09-18: `ensure_skeletal_pipeline` に管と焼く速さの引数を足した（本家のガレージリフトのアニメを 24 Hz で焼くため。既定は前のまま。作業一覧の項目 6 のステップ 8b1）
- 2026-09-19: 判断（本家のナースの `Make Choice` の残り: `CanSeePlayer`・`ChasePlayer`・`NotSeeingPlayer`・`GenerateRandomPoint`・3 s の見失い・`PointOfInterest`・`OnCloseBy`）を AI MoveTo の代理で足し、テスト `Actor.Choice` と道具 `UWasamiTestListener` を足した。PIE（Zone 1 の迷路）で巡回・発見・追跡・Vanish・気絶を確かめた（作業一覧の項目 7 のステップ 2）
- 2026-09-19: 06 の追跡型 `AWasamiEnemy06Chase`（毎ティックの追跡・扉を突く代用と `HitFX`・常に `Chasing`）を足し、気絶の DoOnce を `StartStun` に出し、`IsChasing` と `GetTimeToStunStart` を virtual にした。テスト `Actor.Chase06` を足した（作業一覧の項目 7 のステップ 3）
- 2026-09-19: Zone 2 の迷路の型 `AWasamiEnemyZone2`（階の判定と、階が違うときの原点に最も近いリフトとその `Move Location`）とテスト `Actor.Zone2` を足した（作業一覧の項目 7 のステップ 4）
- 2026-09-19: 見張り `AWasamiEnemySentry` と視界コーン `IWasamiViewconeInterface`・`AWasamiViewcone`・`AWasamiViewconeNurse` を足し、`AWasamiEnemy` の `BeginPlay` の中身を `BeginNurse` に出した。テスト `Actor.Sentry` を足し、`Actor.Choice` のリスナーを `TStrongObjectPtr` で持つようにした（全体の実行で 1 回、最初の `CloseBy` を取りこぼした。参照の無い `NewObject` が回収された疑い）（作業一覧の項目 7 のステップ 5a）
- 2026-09-19: `WasamiViewcone.cpp` の無名名前空間の `MinimapTag`・`OpacityName` を `ViewconeMinimapTag`・`ViewconeOpacityName` に改めた（ユニティビルドで `WasamiPlayerCharacter.cpp`・`WasamiPrimalPower.cpp` の同じ名前とぶつかった。ステップ 5a では未コミットのファイルが塊の外でコンパイルされて表に出ず、コミットの後の最初のビルドで落ちた。症状索引）
- 2026-09-19: 見張り 6 体をレベルに置き（01 記録の `_flow`）、視界コーンの板の材質を取り込んだ（03 記録）。PIE で見張り・コーンの交互の点滅・見つけて跳び降りて追うことを確かめた（作業一覧の項目 7 のステップ 5b）
