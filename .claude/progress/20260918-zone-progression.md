---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-18 17:59
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ゾーンの進行（作業一覧の項目 6）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 6（大目標 1「最小の通しプレイ」）。

- 目標: Zone 1 の開始から Zone 2 に移るまでと、Zone 2 の開始を本家のレベル BP どおりに作る。ゲームの最初はここ（入口は作らない。2026-09-18）。
- 完了の条件（作業一覧の原文の要旨）: Zone 1 の開始（エレベーターの到着 `06_Hospital_Zone01_ElevatorArrive`〈扉 2 枚の動き、14.1 s〉、`Spawn`、目的の帯 `COLLECT ALL SHARDS`、タブレットの矢印 `BP_ArrowPointer`）→ シャード 337 の全回収で `All Shards Collected`（ゾーンの障壁 `BP_ZoneBarrier` とシャードチェッカー `BP_ZoneShardChecker`）→ Zone 2 へ移り（レベルの切り替えとセーブ）、Zone 2 の開始（`BP_06_GarageLift` ×2、`BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5、地図 `BP_MapTexture_MultiFloor`・`BP_MapArea` ×2）まで PIE で通しで遊べる。Zone 2 は、到着の後の捕まる場面と独房の場面（項目 25、大目標 2）を飛ばし、レベル BP で独房の場面の後にプレイヤーが動けるようになる位置と状態から始める。数と値はレベル BP とアクタのプロパティどおり。Bierce の台詞は項目 20、Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event` は項目 25。
- 大目標 1・2 の決め方: 流れ・値・配置は本家のコードから写し、見た目は原作のアセットをそのまま使う。無いものは推定で済ませ、項目 28 の後回しの一覧に 1 行書く。本家の実機は起動しない。

## 本家の流れ（2026-09-18 に読んだもの。根拠は `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`_02.txt`、読み方は `python Tools/dd/bp_flow.py <file> <イベント名>`）

Zone 1（`Level` = 7）:

- **04（`04_Start`）**: `DoubleDoors11` を Lock、シーケンス `ElevatorArrive` を再生、`01_Hotel_Lobby_ElevatorShake`（1）→ 7 s → `ElevatorShakeStop`、`04_Intercom` のトリガーを結ぶ（ナースの放送の声 → 13 s → Bierce。声は項目 20）、`BP_06_Hospital_DoorBreak_2.Enable Switch` と `Finished Event` → `04_DoorBreak`: `DoubleDoors11` の `bLocked` = 偽・`Open Front`、`BP_04_Trigger_Maze` → `05_Transition`（1 s 後の Bierce は項目 20）。
- **`05_Transition`**: チェックポイント 5 を保存（SAVING PROGRESS）→ `05_Persistent`: `Spawn Nurses`（項目 7）、目的 `COLLECT ALL SHARDS`、ゲームモードの `All Shards Collected` → `05 All Shards Collected` を結ぶ、曲（項目 19）、1 s 後に `Check Shards`。
- **`05 All Shards Collected`**: `Remove All Enemies`、矢印の `Shards?` = 偽・色 (1, 0.8002, 0)・的 `06_CutsceneStart`、目的 `REACH THE PARKING LOT`、`06_CutsceneStart` → `05_ParkingLotCutscene`、**`BP_ZoneBarrier_2.Destroy`**（`P_ky_impact3` ×2 の大きさ・`Barrier_Shatter`・消える）。
- **`05_ParkingLotCutscene`**: `06_CineCamera` と場面 `06_Hospital_Zone1_06Event`（項目 25）→ 終わりで `06 Transition`: 視点をプレイヤーへ、`Basic DD Fade Out(2)`、**残りの `BP_Shard` をすべて消し**、タブレットの数を空に、`06_Start` へ移し、`Spawn Nurses_06`（項目 7）、`06_DoorsLock` と `TriggerBox_06_AmbulanceTop` を結び、矢印の色 (1, 0.8317, 0)・的 `06_TunnelEnter`、目的 `REACH THE TUNNEL`、`06_TunnelEnter` を結ぶ。
- **`06_TunnelEnter`**: 矢印の的 `TriggerBox_06_AmbulanceTop`、目的 `GET ON TOP OF THE AMBULANCE`。
- **`06_DoorsLock`**: `DoubleDoors33_36` を Lock・`Force Close`、`BlockingVolume_1` の当たりを入れ（3）、`06 Nurses` の `bAttackDoor` = 真（項目 7）→ **25 s** → `DD_TT_Door_BustedOpen_02`（扉の位置、`01_Lobby_Attenuation`）、`BP_07_CameraShake_Jump`（0〜3000）、`Fracture_concrete_5` の粒子、`BlockingVolume_1` の当たりを切り、`bAttackDoor` = 偽 → 0.1 s → 扉を消す。
- **`06_ReachAmbulance`**: チェックポイント **7** を保存、`Remove All Enemies`、矢印の的 `Plane48_2`、目的 `GOOD LUCK`、曲、`BlockingVolume_Ambulance_1〜4` の当たりを入れ → 1 s → シーケンス `06_Hospital_Zone1_AmbulanceTakeOff`・`06_CameraShake_Zone1_AmbulanceTakeOff`（4）→ 7 s → `UMG_Loading`（`Level` 7、Z 5）・`21-Ballroom_portal_V2`・`Remove All Enemies` → 2.5 s → `OpenLevel('06_Hospital_Zone_02')`。
- `TriggerVolume_1` にナースが入ると `hospital_garage_lift_anim_Anim_2`（`BP_06_GarageLift_Zone1_Special`）の `NurseNear` = 真（そのリフトは以後プレイヤーを乗せても上がらない）。

Zone 2（`Level` = 7）:

- **7**: 本家は `Arrive Event`（`PlayerStart_1`・救急車の到着・`Trigger_Arrive_CaptureScene` → 捕まる場面 → 独房の場面〈`PlayerStart_Cell` へ移す〉）。**本作は `Cell Cutscene Finished` の状態から始める**: 入力を戻し、曲、シーケンス `06_Hospital_Zone2_Spikes` を再生、`BP_06_Hospital_DoorBreak_2.Enable Switch` と `Finished Event` → `Cell_DoorBreak`（シーケンス `Cell_DoorPicked`・`BP_04_BossFight_CameraShake_Initial`）、`Trigger_Cell_Spikes` → `Spikes_Death`（`BP_HitFX`・`DD_Needle_Trap_R1_V3` → 0.5 s → `DeathEvent(プレイヤー)`）、`BP_MiniBoss_Trigger` → `Miniboss_Trigger_Transition`、1 s 後の Bierce（項目 20）。
- **`Miniboss_Trigger_Transition`**: `Miniboss Transition`（`Activate MiniBoss Enemies` は項目 11、目的 `Get past the nurses `〈後ろに空白〉、矢印の `Shards?` = 偽・色 (0, 0, 0, 0)・的なし、`Trigger_MazeStart` → `Maze Trigger Start`）→ チェックポイント **8** を保存。8 で開いたときは `PlayerStart_MiniBoss` から同じ `Miniboss Transition`。
- **`Maze Trigger Start`**: `Maze Transition`（`Remove All Enemies`、矢印の `Shards?` = 真、目的 `COLLECT ALL SHARDS`、`All Shards Collected` → `Maze All Shards`、`Spawn Nurses`〈項目 7〉）→ チェックポイント **9** を保存。9 で開いたときは `PlayerStart_Maze` から同じ。
- **`Maze All Shards`**: チェックポイント **10** を保存 → `Postmaze Transition`（`Remove All Enemies`、残りの `BP_Shard` を消す、`BP_Collectable` ID 3 を `collec` に出す〈項目 12〉、`ring_statue_2.Interact All Shards` → `Collected Ring Piece`〈項目 13〉、`ring_statue_orb_5` を消す、矢印の色 (1, 0.8941, 0)・的 `BP_08_RingPiece_NoPickup_5`、目的 `COLLECT THE RING PIECE`）。10 で開いたときは `PlayerStart_PostMaze` から同じ。`BP_ZoneBarrier_2` を消すのは `Collected Ring Piece`（項目 13）。
- ゲームモード `Check Shards`（`BP_DD_GameMode` @34491）: `Collect Shard` を放ち → 0.05 s → 残りの `BP_Shard` が `Total Shards / 2` ちょうどなら 1 度だけ Bierce の半分の声（項目 20）、1 個未満なら `All Shards Collected` を放つ（`Bierce All Shards` の声は項目 20）・`Event All Shards`（敵の `Activate Frenzy` = Nightmare。項目 7 の敵に効かせる）。
- `BP_ZoneShardChecker`: `Collect Shard` のたびに 1 s 後、箱の中の `BP_Shard` が 0 なら開発用の PrintText と、誰も結ばない `Collected` を放って消える。**意味を持つのは矢印**: `BP_ArrowPointer` は `Shards?` が真のとき、プレイヤーが重なるシャードチェッカーの箱の中のシャードが 100 未満なら最も近いシャードを指す。
- `BP_TriggerBox_Base`: プレイヤーが重なると 1 度だけ `Trigger` を放つ（`EndOverlap` が偽のとき。DoOnce）。
- `BP_06_Hospital_DoorBreak`: 箱に入ると `UMG_07_Boss_Switchbox` の画面（ウィジェットの部品）を見せ、`Interact`（F）を押すたびに `Interact Event` と `SFX_06_Lockpicking`、画面の `Finished` で `Press_Slam_02`・`SFX_06_Lockpicked`、箱を消し → `Finished Event` → 1 s → 画面を消す。`Progress Speed` はアクタのプロパティ。
- Zone 2 のリフト（`BP_06_LiftBase`・`_Corner`）: 上にキャラクターが乗ると `Top Location` へ `FInterpTo_Constant` で上がり、降りると戻る（音の FadeIn/FadeOut 0.5）。ガレージリフト（`BP_06_GarageLift`）は `Overlap Box` にプレイヤーがいる間 ABP の `PlayerOn?` が真（上がる）で、`AnimNotify_Player On/Off Event` で音。
- シーケンスは `pak_reference_2/_sequences/<名前>.json`・`.csv`（ElevatorArrive = エレベーターの扉 2 枚の Transform、AmbulanceTakeOff = `hospital_ambulance_new_teleport` の Transform と音・スポットライト 2 灯の強さ、Zone2_Spikes = `spikes` の Z・音・粒子 2 つ、Cell_DoorPicked = 独房の扉の Transform と粒子）。動かす相手はレベルに `StaticMeshActor` として置いてある（Static なので Movable にする）。

## 計画

- [ ] 1. 区間の流れの土台（C++）: `AWasamiTriggerBox`（`BP_TriggerBox_Base`）、ゲームモードの `Check Shards`・`All Shards Collected`・`Remove All Enemies`・敵の Frenzy、ゾーンの進行を受け持つ仕組み（本家の 2 つのレベル BP のイベントを、チェックポイントの区間ごとに持つ。本作はレベル BP が無いので C++ に置く）の骨組みと目的の帯の文字、デバッグ `Wasami.CollectShards [残す数]`。レベルの組み立て（`dd_level`）に `BP_TriggerBox_Base_C`（Zone 1 に 6、Zone 2 に 8）と、トリガーのほか流れが名指しする `BlockingVolume`（当たりは切って置く）を足し、両ゾーンに置く。テスト。
  - 変更予定: `Source/wasami_deception/WasamiTriggerBox.*`（新規）、`WasamiGameMode.*`、`WasamiZoneFlow.*`（新規。名前は作るときに決める）、`Tests/WasamiZoneFlowTests.cpp`（新規）、`Content/Python/wasami_tools/pipeline/dd_level.py`、`Tools/dd/prepare_stage.py`（要れば）、`/Game/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2`
- [ ] 2. シーケンスの取り込み: `pak_reference_2/_sequences/` の JSON から UE の LevelSequence を作る道具（UE の仕組みをそのまま使う）と、ElevatorArrive・AmbulanceTakeOff・Zone2_Spikes・Cell_DoorPicked の 4 本。動かすアクタ（エレベーターの扉 2 枚・救急車・spikes・独房の扉・音の点・粒子）をレベルで Movable にして結ぶ。PIE で再生して動きを確かめる。
- [ ] 3. Zone 1 の 04 と 05: 到着（ElevatorArrive・揺れ 7 s・`DoubleDoors11` の Lock）、扉の破壊 `BP_06_Hospital_DoorBreak`（`UMG_07_Boss_Switchbox` の画面・`Interact` = F の入力・音）、`Open Front`、`BP_04_Trigger_Maze` → 保存 5 → `05_Persistent`（目的・全回収を結ぶ・1 s 後の Check Shards）。両開き扉は流れが名指しする 2 枚（`DoubleDoors11`・`DoubleDoors33_36`）だけを、本家のメッシュと `Lock`・`Open Front`・`Force Close` の最小限で置く（押して開く・残りの 60 枚は項目 8）。
- [ ] 4. Zone 1 の全回収から 06 まで: `BP_ZoneBarrier`（本家のメッシュ 2 枚と材質・灯・`Destroy` の粒子と音）と `BP_ZoneShardChecker` の箱を置き、`05 All Shards Collected`（障壁が崩れる・目的 REACH THE PARKING LOT）、`06_CutsceneStart` で場面（項目 25）を飛ばして `06 Transition`（フェード・残りのシャードを消す・06_Start へ）、`06_TunnelEnter`、`06_DoorsLock`（25 s で扉が破られる）。06 で開いたときの準備も。
- [ ] 5. Zone 1 から Zone 2 へ: `06_ReachAmbulance`（保存 7・救急車の塞ぎ・AmbulanceTakeOff と揺れ・7 s → `UMG_Loading` の最小限〈回収の記憶を空にする本家の Construct を含む〉・音 → 2.5 s → Zone 2 を開く）。
- [ ] 6. タブレットの矢印 `BP_ArrowPointer`（プレイヤーの子のアクタ。的を指す・`Shards?` のときシャードチェッカーの箱の中の最も近いシャードを指す・`Change Color`）。両ゾーンの区間の値を結ぶ。
- [ ] 7. Zone 2 の区間: 7 で `PlayerStart_Cell` から独房の場面の後の状態で始める（Spikes・独房の扉の破壊・`Spikes_Death`）、`Miniboss_Trigger_Transition`（保存 8。Matron は項目 11）、`Maze Trigger Start`（保存 9・全回収を結ぶ）、`Maze All Shards`（保存 10・`Postmaze Transition` のうち項目 13 に属さない分）、8〜10 で開いたときの準備。`BP_ZoneBarrier_2`（Zone 2）を置く（消すのは項目 13）。ゲームモードの 7 の PlayerStart を `PlayerStart_Cell` にする。
- [ ] 8. リフト: Zone 2 の `BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5（乗ると上がる床）と、ガレージリフト `BP_06_GarageLift` ×2（Zone 2）・`BP_06_GarageLift_Zone1_Special`（Zone 1。`TriggerVolume_1` にナースが入ると上がらない）。
- [ ] 9. Zone 2 の地図 `BP_MapTexture_MultiFloor` と `BP_MapArea` ×2（いる階の箱で地図の絵を `T_06_Zone2` ↔ `T_06_Zone2_02` に替える）。
- [ ] 10. 仕上げ: PIE で Zone 1 の到着 → 扉の破壊 → 全回収（デバッグで数個を残す）→ 障壁 → 駐車場 → トンネル → 扉が破られる → 救急車 → Zone 2 の独房 → 扉の破壊 → 迷路 → 全回収 → COLLECT THE RING PIECE までを通しで収録し、Discord のグリッドにする。実装記録・handover・作業一覧（項目 6 を完了、完了の条件の読み替え）・note を直し、進捗記録を消して main へマージし push。

## 次にやること

ステップ 1 を始める。main から `feature/zone-progression` を作って切り替え、記録のステップ 1 を「作業中」にする。本家の `BP_TriggerBox_Base`（`pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_TriggerBox_Base.txt` と `_assets/…/BP_TriggerBox_Base.json` の `Box` の大きさ）と、レベルの書き出しのトリガーの `Box` の変換（`pak_reference_2/_levels/06_Hospital_Zone_0*.full.json`）を読み、実装記録 06（ゲームモード）と 01（`dd_level` の組み立て）を読んでから C++ を書く。

## 決定事項

- 2026-09-18: **Zone 1 から Zone 2 へは、救急車の上に乗る（`06_ReachAmbulance`）で移る**。作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」は、作業一覧を作ったときの読み違い — 本家のレベル BP では、ガレージリフト（`BP_06_GarageLift_Zone1_Special`）は駐車場の車のリフト（乗ると上がる。ナースが近づくと上がらない）で、Zone 2 を開くのは `06_ReachAmbulance`（保存 7 → 救急車が出る → 読み込み画面 → `OpenLevel('06_Hospital_Zone_02')`）。流れは本家のコードから写す決まりなので、救急車で作り、ガレージリフトはステップ 8 で仕掛けとして作る。項目を閉じるときに作業一覧の完了の条件を読み替える。
- 2026-09-18: 本家の場面は飛ばす（項目 25）: Zone 1 の `06_CutsceneStart` では `06_Hospital_Zone1_06Event` を流さず、すぐ `06 Transition` へ。Zone 2 の 7 は `Arrive Event`・捕まる場面・独房の場面を流さず、`PlayerStart_Cell` で `Cell Cutscene Finished` の状態から始める。
- 2026-09-18: 声・曲・ナースは別の項目: Bierce の台詞と `04_Intercom` のナースの放送（項目 20）、`BP_06_MusicPlayer` の `bFadeOut`（項目 19）、`Spawn Nurses`・`Spawn Nurses_06`・`bAttackDoor`・`Activate MiniBoss Enemies`（項目 7・11）。この項目では呼び出しの口（区間の準備の場所）だけ作り、中身はそれぞれの項目が埋める。
- 2026-09-18: 両開き扉は流れが名指しする 2 枚だけを置く（ステップ 3）。`BP_ZoneShardChecker` は矢印の区域として置き、開発用の PrintText と誰も結ばない `Collected` は写さない。
- 2026-09-18: 作業ブランチ `feature/zone-progression` で進める（`.claude/guides/git-workflow.md` の大規模改修: 複数の仕組みにまたがり、複数回のコミットに分ける）。計画のコミットだけ main に置く。

## 要確認（ユーザー）

- 2026-09-18: 作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」— 本家のコードどおり**救急車の上に乗って Zone 2 へ移る**形に読み替える。理由: 本家の `06_Hospital_Zone_01` のレベル BP で Zone 2 を開くのは `06_ReachAmbulance`（`TriggerBox_06_AmbulanceTop`）で、ガレージリフトは駐車場の車のリフトの仕掛け。場所: この記録の「決定事項」、項目を閉じるときに `.claude/roadmap.md` の項目 6 の完了の条件。

## 再開時の注意

- エディタの状態はこの反復では触っていない（計画だけ）。

## 検証

- check_records: 未実行（ソースの変更なし）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
