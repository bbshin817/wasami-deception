---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-18 19:00
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
- シーケンス（ステップ 2 で置いた）: 両ゾーンのレベルの `LevelSequenceActor`（`src:06_Hospital_Zone01_ElevatorArrive` など 4 つ）を `GetSequencePlayer` → `Play` で流す。ElevatorArrive = 扉 2 枚と音（14.1 s）、AmbulanceTakeOff = 救急車（付いたボリュームごと）と音・スポットライト 2 灯（13.9 s）、Zone2_Spikes = 棘・音の点・火花（70 s）、Cell_DoorPicked = 独房の扉・粒子・音と Bierce の一言（4.3 s）。`Basic DD Fade Out(2)` は `/Game/DD/Animation/00_Ballroom/Ballroom_Event_Fade` を実行時のプレイヤーで 2 倍速に流す（ステップ 4 の 06 Transition）。
- ゲームモードの `Event All Shards` は `BP_Monkey` だけを Frenzy にする（2026-09-18 に読み直した。ナースには効かない）。

## 計画

- [x] 1. 区間の流れの土台（2026-09-18 完了）: `AWasamiTriggerBox`、ゲームモードの `CheckShards`、`AWasamiZoneFlow`・`AWasamiZone1Flow`・`AWasamiZone2Flow`（両ゾーンの区間・トリガー・保存・目的・矢印の値・救急車で Zone 2 を開く所まで）、デバッグ `Wasami.Flow`・`Wasami.Trigger`・`Wasami.CollectShards`、組み立ての `place_dd_flow`（両ゾーンに置いた）。テスト 4 本と PIE の通し（実装記録 11）。
- [x] 2. シーケンスの取り込み（2026-09-18 完了）: `dd_sequence.py`（本家の生の書き出しから LevelSequence を作り、レベルのアクタに結び、`LevelSequenceActor` を `src:<本家の名前>` で置く）、`WasamiStageTools.place_dd_sequences`、組み立ての最後に呼ぶ。両ゾーンに置いた（Zone 1: ElevatorArrive・AmbulanceTakeOff、Zone 2: Spikes・Cell_DoorPicked、ほかに `Ballroom_Event_Fade`）。曲線は本家の CSV と一致、PIE で 4 本とも動くことを確かめた（実装記録 01 の「シーケンス」）。
- [ ] 3. Zone 1 の 04 と 05: 到着（ElevatorArrive を再生・揺れ 7 s・`DoubleDoors11` の Lock）、扉の破壊 `BP_06_Hospital_DoorBreak`（`UMG_07_Boss_Switchbox` の画面・`Interact` = F の入力・音）、`Open Front`、`BP_04_Trigger_Maze` → 保存 5 → `05_Persistent`（目的・全回収を結ぶ・1 s 後の Check Shards）。両開き扉は流れが名指しする 2 枚（`DoubleDoors11`・`DoubleDoors33_36`）だけを、本家のメッシュと `Lock`・`Open Front`・`Force Close` の最小限で置く（押して開く・残りの 60 枚は項目 8）。
- [ ] 4. Zone 1 の全回収から 06 まで: `BP_ZoneBarrier`（本家のメッシュ 2 枚と材質・灯・`Destroy` の粒子と音）と `BP_ZoneShardChecker` の箱を置き、`05 All Shards Collected`（障壁が崩れる・目的 REACH THE PARKING LOT）、`06_CutsceneStart` で場面（項目 25）を飛ばして `06 Transition`（フェード・残りのシャードを消す・06_Start へ）、`06_TunnelEnter`、`06_DoorsLock`（25 s で扉が破られる）。06 で開いたときの準備も。
- [ ] 5. Zone 1 から Zone 2 へ: `06_ReachAmbulance`（保存 7・救急車の塞ぎ・AmbulanceTakeOff と揺れ・7 s → `UMG_Loading` の最小限〈回収の記憶を空にする本家の Construct を含む〉・音 → 2.5 s → Zone 2 を開く）。
- [ ] 6. タブレットの矢印 `BP_ArrowPointer`（プレイヤーの子のアクタ。的を指す・`Shards?` のときシャードチェッカーの箱の中の最も近いシャードを指す・`Change Color`）。両ゾーンの区間の値を結ぶ。
- [ ] 7. Zone 2 の区間: 7 で `PlayerStart_Cell` から独房の場面の後の状態で始める（Spikes・独房の扉の破壊・`Spikes_Death`）、`Miniboss_Trigger_Transition`（保存 8。Matron は項目 11）、`Maze Trigger Start`（保存 9・全回収を結ぶ）、`Maze All Shards`（保存 10・`Postmaze Transition` のうち項目 13 に属さない分）、8〜10 で開いたときの準備。`BP_ZoneBarrier_2`（Zone 2）を置く（消すのは項目 13）。ゲームモードの 7 の PlayerStart を `PlayerStart_Cell` にする。Spikes・Cell_DoorPicked が発火する粒子のシステム 3 つ（`Blueprints/Characters/Nurse/P_06_NurseSparks`、BallisticsVFX の `Fracture_dark_slow`・`Concrete_impact_large`。`dd_particles`、材質は推定）を取り込み、`place_dd_sequences Zone2` を走らせ直してエミッタにテンプレートを入れる（いまはテンプレートなし）。
- [ ] 8. リフト: Zone 2 の `BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5（乗ると上がる床）と、ガレージリフト `BP_06_GarageLift` ×2（Zone 2）・`BP_06_GarageLift_Zone1_Special`（Zone 1。`TriggerVolume_1` にナースが入ると上がらない）。
- [ ] 9. Zone 2 の地図 `BP_MapTexture_MultiFloor` と `BP_MapArea` ×2（いる階の箱で地図の絵を `T_06_Zone2` ↔ `T_06_Zone2_02` に替える）。
- [ ] 10. 仕上げ: PIE で Zone 1 の到着 → 扉の破壊 → 全回収（デバッグで数個を残す）→ 障壁 → 駐車場 → トンネル → 扉が破られる → 救急車 → Zone 2 の独房 → 扉の破壊 → 迷路 → 全回収 → COLLECT THE RING PIECE までを通しで収録し、Discord のグリッドにする。実装記録・handover・作業一覧（項目 6 を完了、完了の条件の読み替え）・note を直し、進捗記録を消して main へマージし push。

## 次にやること

ステップ 3（Zone 1 の 04 と 05）を始める。記録のステップ 3 を「作業中」にし、変えるファイルを書く。まず流れがシーケンスを再生する口を C++ に作る: `AWasamiZoneFlow` に `PlaySequence(Name)`（`src:<Name>` の `ALevelSequenceActor` の `GetSequencePlayer()->Play()`。本家のレベル BP と同じ）を足す（`wasami_deception.Build.cs` に `LevelSequence`・`MovieScene` のモジュール）。`Start04` の `Not yet:`（`WasamiZone1Flow.cpp`。実装記録 11）から `06_Hospital_Zone01_ElevatorArrive` と揺れ `01_Hotel_Lobby_ElevatorShake` を始め、7 s 後に揺れを止める。続けて `BP_06_Hospital_DoorBreak`（本家の BP を `bp_flow.py` で読む）と両開き扉 2 枚の最小限。

## 決定事項

- 2026-09-18: **Zone 1 から Zone 2 へは、救急車の上に乗る（`06_ReachAmbulance`）で移る**。作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」は、作業一覧を作ったときの読み違い — 本家のレベル BP では、ガレージリフト（`BP_06_GarageLift_Zone1_Special`）は駐車場の車のリフト（乗ると上がる。ナースが近づくと上がらない）で、Zone 2 を開くのは `06_ReachAmbulance`（保存 7 → 救急車が出る → 読み込み画面 → `OpenLevel('06_Hospital_Zone_02')`）。流れは本家のコードから写す決まりなので、救急車で作り、ガレージリフトはステップ 8 で仕掛けとして作る。項目を閉じるときに作業一覧の完了の条件を読み替える。
- 2026-09-18: 本家の場面は飛ばす（項目 25）: Zone 1 の `06_CutsceneStart` では `06_Hospital_Zone1_06Event` を流さず、すぐ `06 Transition` へ。Zone 2 の 7 は `Arrive Event`・捕まる場面・独房の場面を流さず、`PlayerStart_Cell` で `Cell Cutscene Finished` の状態から始める。
- 2026-09-18: 声・曲・ナースは別の項目: Bierce の台詞と `04_Intercom` のナースの放送（項目 20）、`BP_06_MusicPlayer` の `bFadeOut`（項目 19）、`Spawn Nurses`・`Spawn Nurses_06`・`bAttackDoor`・`Activate MiniBoss Enemies`（項目 7・11）。この項目では呼び出しの口（区間の準備の場所）だけ作り、中身はそれぞれの項目が埋める。
- 2026-09-18: 両開き扉は流れが名指しする 2 枚だけを置く（ステップ 3）。`BP_ZoneShardChecker` は矢印の区域として置き、開発用の PrintText と誰も結ばない `Collected` は写さない。
- 2026-09-18: 区間の流れは `AWasamiZoneFlow` の派生 2 つ（ゲームモードが開始時に出す。レベル BP の代わり）。本家の名前で置かれたアクタは `src:<名前>` タグで探す。残りのステップは各イベントの `Not yet:` の所を埋める（実装記録 11）。
- 2026-09-18: 作業ブランチ `feature/zone-progression` で進める（`.claude/guides/git-workflow.md` の大規模改修: 複数の仕組みにまたがり、複数回のコミットに分ける）。計画のコミットだけ main に置く。
- 2026-09-18: シーケンスは本家どおりレベルの `LevelSequenceActor`（タグ `src:<本家の名前>`）から再生する（本家のレベル BP は置かれたアクタの `GetSequencePlayer` → `Play`）。本家で Static の救急車と独房の扉は Static のまま（UE の Sequencer が動かす間だけ Movable にする。PIE で動くことを確かめた）。フェードの `Ballroom_Event_Fade` はアクタを置かず、`Basic DD Fade Out` と同じく実行時にプレイヤーを作って 2 倍速で流す（ステップ 4）。

## 要確認（ユーザー）

- 2026-09-18: 作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」— 本家のコードどおり**救急車の上に乗って Zone 2 へ移る**形に読み替える。理由: 本家の `06_Hospital_Zone_01` のレベル BP で Zone 2 を開くのは `06_ReachAmbulance`（`TriggerBox_06_AmbulanceTop`）で、ガレージリフトは駐車場の車のリフトの仕掛け。場所: この記録の「決定事項」、項目を閉じるときに `.claude/roadmap.md` の項目 6 の完了の条件。

## 再開時の注意

- 結ぶ前に通ったトリガーの箱は使い切られる（本家も同じ）。扉が無いうちに PIE で確かめるときは、`Wasami.Flow On04DoorBreak` などで進めてから箱に立つ。
- ゲームのセーブ（本作の `structSlot`）は PIE の後に `Wasami.ResetSave` で空にしてある（次に開くと Zone 1 の 04_Start）。
- `WasamiStageTools.place_dd_sequences` は MCP にはエディタを開き直すまで出ない。それまでは `python Tools/ue_remote.py -c "...dd_sequence.place('Zone2')..."`（`importlib.reload` してから）で呼ぶ。

## 検証

- ステップ 1: check_records OK、C++ ビルド OK、テスト `Wasami.*` 44 本すべて通過、PIE で Zone 1 の 04 → 保存 5 → 全回収 → 06 → 扉の塞ぎ → 救急車で保存 7 → Zone 2 → 保存 8・9・10 → COLLECT THE RING PIECE、10 で開き直し（実装記録 11 の「確かめたこと」）。
- ステップ 2: 作った曲線を本家の `_sequences/*.csv` と比べて一致（扉の X は小数 3 桁、独房の扉の最後のヨー −151.426）。PIE で ElevatorArrive（扉が 11.3〜13.5 s に開いて残る）・AmbulanceTakeOff（10 s で Y −19993 → −3093、付いたボリュームも動く、スポットライト 0 → 1、13.9 s で元に戻る）・Cell_DoorPicked・Spikes（10 倍速で棘が Z −150 → −733.7、`Trigger_Cell_Spikes` も動く）。check_records OK。
