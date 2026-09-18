---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-18 18:32
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ゾーンの進行（作業一覧の項目 6）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 6（大目標 1「最小の通しプレイ」）。

- 目標: Zone 1 の開始から Zone 2 に移るまでと、Zone 2 の開始を本家のレベル BP どおりに作る。ゲームの最初はここ（入口は作らない。2026-09-18）。
- 完了の条件（作業一覧の原文の要旨）: Zone 1 の開始（エレベーターの到着 `06_Hospital_Zone01_ElevatorArrive`〈扉 2 枚の動き、14.1 s〉、`Spawn`、目的の帯 `COLLECT ALL SHARDS`、タブレットの矢印 `BP_ArrowPointer`）→ シャード 337 の全回収で `All Shards Collected`（ゾーンの障壁 `BP_ZoneBarrier` とシャードチェッカー `BP_ZoneShardChecker`）→ Zone 2 へ移り（レベルの切り替えとセーブ）、Zone 2 の開始（`BP_06_GarageLift` ×2、`BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5、地図 `BP_MapTexture_MultiFloor`・`BP_MapArea` ×2）まで PIE で通しで遊べる。Zone 2 は、到着の後の捕まる場面と独房の場面（項目 25、大目標 2）を飛ばし、レベル BP で独房の場面の後にプレイヤーが動けるようになる位置と状態から始める。数と値はレベル BP とアクタのプロパティどおり。Bierce の台詞は項目 20、Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event` は項目 25。
- 大目標 1・2 の決め方: 流れ・値・配置は本家のコードから写し、見た目は原作のアセットをそのまま使う。無いものは推定で済ませ、項目 28 の後回しの一覧に 1 行書く。本家の実機は起動しない。

## 本家の流れ（残りのステップで要るもの）

流れの全体と、各イベントのうちまだ埋めていない所（コメントの `Not yet:`）は実装記録 11（`.claude/implementation-records/11-zone-flow.md`）。根拠は `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`_02.txt`、読み方は `python Tools/dd/bp_flow.py <file> <イベント名>`。

- `BP_06_Hospital_DoorBreak`: 箱に入ると `UMG_07_Boss_Switchbox` の画面（ウィジェットの部品）を見せ、`Interact`（F）を押すたびに `Interact Event` と `SFX_06_Lockpicking`、画面の `Finished` で `Press_Slam_02`・`SFX_06_Lockpicked`、箱を消し → `Finished Event` → 1 s → 画面を消す。`Progress Speed` はアクタのプロパティ。流れの側の口は Zone 1 の `On04DoorBreak`、Zone 2 は `Cell_DoorBreak`（シーケンス `Cell_DoorPicked`・`PlayCameraShake(BP_04_BossFight_CameraShake_Initial, 1)`。ステップ 7 で足す）。
- `BP_ZoneShardChecker`: `Collect Shard` のたびに 1 s 後、箱の中の `BP_Shard` が 0 なら開発用の PrintText と誰も結ばない `Collected` を放って消える。**意味を持つのは矢印**: `BP_ArrowPointer` は `Shards?` が真のとき、プレイヤーが重なるシャードチェッカーの箱の中のシャードが 100 未満なら最も近いシャードを指す。矢印の値は流れの `IsArrowOnShards`・`GetArrowColor`・`GetArrowTarget`（`Shards?` の既定は真）。
- Zone 2 のリフト（`BP_06_LiftBase`・`_Corner`）: 上にキャラクターが乗ると `Top Location` へ `FInterpTo_Constant` で上がり、降りると戻る（音の FadeIn/FadeOut 0.5）。ガレージリフト（`BP_06_GarageLift`）は `Overlap Box` にプレイヤーがいる間 ABP の `PlayerOn?` が真（上がる）で、`AnimNotify_Player On/Off Event` で音。Zone 1 の `TriggerVolume_1`（置いた）にナースが入ると `hospital_garage_lift_anim_Anim_2`（`BP_06_GarageLift_Zone1_Special`）の `NurseNear` = 真（以後プレイヤーを乗せても上がらない）。
- シーケンスは `pak_reference_2/_sequences/<名前>.json`・`.csv`（ElevatorArrive = エレベーターの扉 2 枚の Transform、AmbulanceTakeOff = `hospital_ambulance_new_teleport` の Transform と音・スポットライト 2 灯の強さ、Zone2_Spikes = `spikes` の Z・音・粒子 2 つ、Cell_DoorPicked = 独房の扉の Transform と粒子）。動かす相手はレベルに `StaticMeshActor` として置いてある（Static なので Movable にする。救急車と棘には流れのボリューム・トリガーが付いている）。`Basic DD Fade Out(2)` はシーケンス `Ballroom_Event_Fade` を 2 倍速で流す（ステップ 4 の 06 Transition）。
- ゲームモードの `Event All Shards` は `BP_Monkey` だけを Frenzy にする（2026-09-18 に読み直した。ナースには効かない）。

## 計画

- [x] 1. 区間の流れの土台（2026-09-18 完了）: `AWasamiTriggerBox`、ゲームモードの `CheckShards`、`AWasamiZoneFlow`・`AWasamiZone1Flow`・`AWasamiZone2Flow`（両ゾーンの区間・トリガー・保存・目的・矢印の値・救急車で Zone 2 を開く所まで）、デバッグ `Wasami.Flow`・`Wasami.Trigger`・`Wasami.CollectShards`、組み立ての `place_dd_flow`（両ゾーンに置いた）。テスト 4 本と PIE の通し（実装記録 11）。
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

ステップ 2（シーケンスの取り込み）を始める。ブランチ `feature/zone-progression` のまま、記録のステップ 2 を「作業中」にする。`pak_reference_2/_sequences/` の `06_Hospital_Zone01_ElevatorArrive`・`06_Hospital_Zone1_AmbulanceTakeOff`・`06_Hospital_Zone2_Spikes`・`06_Hospital_Zone2_Cell_DoorPicked`（と 06 Transition の `Ballroom_Event_Fade`）の JSON・CSV の形を読み、UE の LevelSequence をスクリプトで作れるか（Python の `unreal.LevelSequence` とトラック・キーの API。`animation_toolset` の Sequencer ツールも見る）を確かめてから、道具を `Content/Python/wasami_tools/pipeline/` に書く。動かす相手のメッシュ（`src:` タグ）を Movable にして結ぶ。流れの側は `WasamiZone1Flow.cpp`・`WasamiZone2Flow.cpp` の `Not yet:` のシーケンスの所から呼ぶ（実装記録 11）。

## 決定事項

- 2026-09-18: **Zone 1 から Zone 2 へは、救急車の上に乗る（`06_ReachAmbulance`）で移る**。作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」は、作業一覧を作ったときの読み違い — 本家のレベル BP では、ガレージリフト（`BP_06_GarageLift_Zone1_Special`）は駐車場の車のリフト（乗ると上がる。ナースが近づくと上がらない）で、Zone 2 を開くのは `06_ReachAmbulance`（保存 7 → 救急車が出る → 読み込み画面 → `OpenLevel('06_Hospital_Zone_02')`）。流れは本家のコードから写す決まりなので、救急車で作り、ガレージリフトはステップ 8 で仕掛けとして作る。項目を閉じるときに作業一覧の完了の条件を読み替える。
- 2026-09-18: 本家の場面は飛ばす（項目 25）: Zone 1 の `06_CutsceneStart` では `06_Hospital_Zone1_06Event` を流さず、すぐ `06 Transition` へ。Zone 2 の 7 は `Arrive Event`・捕まる場面・独房の場面を流さず、`PlayerStart_Cell` で `Cell Cutscene Finished` の状態から始める。
- 2026-09-18: 声・曲・ナースは別の項目: Bierce の台詞と `04_Intercom` のナースの放送（項目 20）、`BP_06_MusicPlayer` の `bFadeOut`（項目 19）、`Spawn Nurses`・`Spawn Nurses_06`・`bAttackDoor`・`Activate MiniBoss Enemies`（項目 7・11）。この項目では呼び出しの口（区間の準備の場所）だけ作り、中身はそれぞれの項目が埋める。
- 2026-09-18: 両開き扉は流れが名指しする 2 枚だけを置く（ステップ 3）。`BP_ZoneShardChecker` は矢印の区域として置き、開発用の PrintText と誰も結ばない `Collected` は写さない。
- 2026-09-18: 区間の流れは `AWasamiZoneFlow` の派生 2 つ（ゲームモードが開始時に出す。レベル BP の代わり）。本家の名前で置かれたアクタは `src:<名前>` タグで探す。残りのステップは各イベントの `Not yet:` の所を埋める（実装記録 11）。
- 2026-09-18: 作業ブランチ `feature/zone-progression` で進める（`.claude/guides/git-workflow.md` の大規模改修: 複数の仕組みにまたがり、複数回のコミットに分ける）。計画のコミットだけ main に置く。

## 要確認（ユーザー）

- 2026-09-18: 作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」— 本家のコードどおり**救急車の上に乗って Zone 2 へ移る**形に読み替える。理由: 本家の `06_Hospital_Zone_01` のレベル BP で Zone 2 を開くのは `06_ReachAmbulance`（`TriggerBox_06_AmbulanceTop`）で、ガレージリフトは駐車場の車のリフトの仕掛け。場所: この記録の「決定事項」、項目を閉じるときに `.claude/roadmap.md` の項目 6 の完了の条件。

## 再開時の注意

- 結ぶ前に通ったトリガーの箱は使い切られる（本家も同じ）。扉が無いうちに PIE で確かめるときは、`Wasami.Flow On04DoorBreak` などで進めてから箱に立つ。
- ゲームのセーブ（本作の `structSlot`）は PIE の後に `Wasami.ResetSave` で空にしてある（次に開くと Zone 1 の 04_Start）。

## 検証

- ステップ 1: check_records OK、C++ ビルド OK、テスト `Wasami.*` 44 本すべて通過、PIE で Zone 1 の 04 → 保存 5 → 全回収 → 06 → 扉の塞ぎ → 救急車で保存 7 → Zone 2 → 保存 8・9・10 → COLLECT THE RING PIECE、10 で開き直し（実装記録 11 の「確かめたこと」）。
