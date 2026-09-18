---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-18 22:05
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

- 扉の破壊（ステップ 3b で作った。実装記録 11）: Zone 2 は `OnCellCutsceneFinished` で `EnableDoorBreak("BP_06_Hospital_DoorBreak_2", Cell_DoorBreak)`（`Cell_DoorBreak` はシーケンス `Cell_DoorPicked`・`PlayCameraShake(BP_04_BossFight_CameraShake_Initial, 1)`。ステップ 7）。
- Zone 2 のリフト（`BP_06_LiftBase`・`_Corner`）: 上にキャラクターが乗ると `Top Location` へ `FInterpTo_Constant` で上がり、降りると戻る（音の FadeIn/FadeOut 0.5）。ガレージリフト（`BP_06_GarageLift`）は `Overlap Box` にプレイヤーがいる間 ABP の `PlayerOn?` が真（上がる）で、`AnimNotify_Player On/Off Event` で音。Zone 1 の `TriggerVolume_1`（置いた）にナースが入ると `hospital_garage_lift_anim_Anim_2`（`BP_06_GarageLift_Zone1_Special`）の `NurseNear` = 真（以後プレイヤーを乗せても上がらない）。
- シーケンス（ステップ 2 で置いた）: 両ゾーンのレベルの `LevelSequenceActor`（`src:06_Hospital_Zone01_ElevatorArrive` など 4 つ）を `GetSequencePlayer` → `Play` で流す。ElevatorArrive = 扉 2 枚と音（14.1 s）、AmbulanceTakeOff = 救急車（付いたボリュームごと）と音・スポットライト 2 灯（13.9 s）、Zone2_Spikes = 棘・音の点・火花（70 s）、Cell_DoorPicked = 独房の扉・粒子・音と Bierce の一言（4.3 s）。
- ゲームモードの `Event All Shards` は `BP_Monkey` だけを Frenzy にする（2026-09-18 に読み直した。ナースには効かない）。

## 計画

- [x] 1. 区間の流れの土台（2026-09-18 完了）: `AWasamiTriggerBox`、ゲームモードの `CheckShards`、`AWasamiZoneFlow`・`AWasamiZone1Flow`・`AWasamiZone2Flow`（両ゾーンの区間・トリガー・保存・目的・矢印の値・救急車で Zone 2 を開く所まで）、デバッグ `Wasami.Flow`・`Wasami.Trigger`・`Wasami.CollectShards`、組み立ての `place_dd_flow`（両ゾーンに置いた）。テスト 4 本と PIE の通し（実装記録 11）。
- [x] 2. シーケンスの取り込み（2026-09-18 完了）: `dd_sequence.py`（本家の生の書き出しから LevelSequence を作り、レベルのアクタに結び、`LevelSequenceActor` を `src:<本家の名前>` で置く）、`WasamiStageTools.place_dd_sequences`、組み立ての最後に呼ぶ。両ゾーンに置いた（Zone 1: ElevatorArrive・AmbulanceTakeOff、Zone 2: Spikes・Cell_DoorPicked、ほかに `Ballroom_Event_Fade`）。曲線は本家の CSV と一致、PIE で 4 本とも動くことを確かめた（実装記録 01 の「シーケンス」）。
- [x] 3a. 到着（2026-09-18 完了）: `AWasamiZoneFlow::PlaySequence`・`PlayCameraShake`、流れの揺れ 5 本（`dd_sequence.CAMERA_SHAKES`、両ゾーンで作った）、`Start04` で ElevatorArrive と揺れ → 7 s 後に `ElevatorShakeStop`（実装記録 11・01）。
- [x] 3b. 扉の破壊（2026-09-18 完了）: `AWasamiDoorBreak`・画面 `UWasamiSwitchboxWidget`（`M_UI_Radial` は焼き込みのシェーダーから組んだ）・プレイヤーの F（`OnInteract`・`Wasami.Interact`）・流れの `EnableDoorBreak`、両ゾーンに置いた（`place_dd_flow`）。Zone 1 の 04 の 7 s 後に起き、外れると `04_DoorBreak`（実装記録 11・09・02・01）。
- [x] 3c. 両開き扉（2026-09-18 完了）: `AWasamiDoubleDoors`（本家の `BP_06_DoubleDoors` 全部）・音の取り込み `dd_gimmicks.py`・`_flow` が Zone 1 の 2 枚をメッシュと材質ごと置く。流れは 04 で `DoubleDoors11` を `Lock`、鍵が外れたら `bLocked` 偽 → `Open Front`、`06_DoorsLock` で `DoubleDoors33_36` を `Lock` → `Force Close`（実装記録 08・11・01）。
- [x] 4a. 障壁（2026-09-18 完了）: `AWasamiZoneBarrier`（本家の `BP_ZoneBarrier`）・素材 `dd_gimmicks.import_zone_barrier`（`MM_SpeedBarrier` は焼き込みのシェーダーの式、`P_ky_impact3`）・`dd_assets.base_property_overrides`・テクスチャの貼り方、`_flow` が両ゾーンの `BP_ZoneBarrier_2` を置く（障壁の灯は単独で置かない）、流れの `ZoneBarrier`・全回収で `DestroyBarrier`（実装記録 08・11・01）。
- [x] 4b. フェードと扉が破られる（2026-09-18 完了）: 流れの `PlayFadeOut`・`PlayWorldCameraShake`・`PlaySoundAt`・`ActivateEmitter`、`Transition06` のフェード（2 倍速）、`On06DoorsLock` の 25 s 後の `BreakDoorsIn`。素材 `dd_gimmicks.import_doors_busted`（音・`Fracture_concrete_3`・推定の材質）、`_flow` が `Fracture_concrete_5` を置く。`dd_particles` が GPU のエミッタを組めるようにし、数の指数の形で配列の要素を失う不具合を直した（実装記録 11・08・01、症状索引 3 件）。
- [x] 5. Zone 1 から Zone 2 へ（2026-09-18 完了）: `06_ReachAmbulance` の 1 s 後に救急車のシーケンスと揺れ（拡縮 4）、7 s 後に読み込み画面 `UWasamiLoadingWidget`（本家の `UMG_Loading`。Construct が回収の記憶を空にする。紋章は要確認で出さない）とポータルの音（`dd_ui.import_loading`）→ 2.5 s 後に Zone 2（実装記録 09・11・01・06）。
- [x] 6. 地図の矢印（2026-09-18 完了）: `AWasamiArrowPointer`（本家の `BP_ArrowPointer`。プレイヤーの子のアクタ、流れの矢印の値を `Find Object` で取る）・`AWasamiZoneShardChecker`（`BP_ZoneShardChecker`。`_flow` が両ゾーンに置いた）・推定の `M_DD_Arrow`（`dd_tablet`）。テスト 2 本、PIE で最も近いシャードと駐車場を指すことを確かめた（実装記録 03・11・02・01）。
- [ ] 7. Zone 2 の区間: 7 で `PlayerStart_Cell` から独房の場面の後の状態で始める（Spikes・独房の扉の破壊・`Spikes_Death`）、`Miniboss_Trigger_Transition`（保存 8。Matron は項目 11）、`Maze Trigger Start`（保存 9・全回収を結ぶ）、`Maze All Shards`（保存 10・`Postmaze Transition` のうち項目 13 に属さない分）、8〜10 で開いたときの準備（Zone 2 の障壁はステップ 4a で置いた。壊すのは項目 13）。ゲームモードの 7 の PlayerStart を `PlayerStart_Cell` にする。Spikes・Cell_DoorPicked が発火する粒子のシステム 3 つ（`Blueprints/Characters/Nurse/P_06_NurseSparks`、BallisticsVFX の `Fracture_dark_slow`・`Concrete_impact_large`。`dd_particles`、材質は推定）を取り込み、`place_dd_sequences Zone2` を走らせ直してエミッタにテンプレートを入れる（いまはテンプレートなし）。
- [ ] 8. リフト: Zone 2 の `BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5（乗ると上がる床）と、ガレージリフト `BP_06_GarageLift` ×2（Zone 2）・`BP_06_GarageLift_Zone1_Special`（Zone 1。`TriggerVolume_1` にナースが入ると上がらない）。
- [ ] 9. Zone 2 の地図 `BP_MapTexture_MultiFloor` と `BP_MapArea` ×2（いる階の箱で地図の絵を `T_06_Zone2` ↔ `T_06_Zone2_02` に替える）。
- [ ] 10. 仕上げ: PIE で Zone 1 の到着 → 扉の破壊 → 全回収（デバッグで数個を残す）→ 障壁 → 駐車場 → トンネル → 扉が破られる → 救急車 → Zone 2 の独房 → 扉の破壊 → 迷路 → 全回収 → COLLECT THE RING PIECE までを通しで収録し、Discord のグリッドにする。実装記録・handover・作業一覧（項目 6 を完了、完了の条件の読み替え）・note を直し、進捗記録を消して main へマージし push。

## 次にやること

ステップ 7（Zone 2 の区間）を始める。記録のステップ 7 を「作業中」にし、変えるファイルを書く。大きいので、まず本家の `06_Hospital_Zone_02` のレベル BP（`python Tools/dd/bp_flow.py pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_02.txt --list`）で `Cell Cutscene Finished`・`Spikes`・`Cell_DoorBreak`・`Miniboss_Trigger_Transition`・`Maze Trigger Start`・`Maze All Shards`・`Postmaze Transition` を読み、実装記録 11 の Zone 2 の `Not yet:` と照らして 7a・7b… に分け直す（目安: 7a 独房から始める〈`PlayerStart_Cell`・棘・扉の破壊・`Cell_DoorPicked`〉、7b 粒子 3 つの取り込みと `place_dd_sequences Zone2`、7c 8〜10 の区間）。

## 決定事項

- 2026-09-18: **Zone 1 から Zone 2 へは、救急車の上に乗る（`06_ReachAmbulance`）で移る**。作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」は、作業一覧を作ったときの読み違い — 本家のレベル BP では、ガレージリフト（`BP_06_GarageLift_Zone1_Special`）は駐車場の車のリフト（乗ると上がる。ナースが近づくと上がらない）で、Zone 2 を開くのは `06_ReachAmbulance`（保存 7 → 救急車が出る → 読み込み画面 → `OpenLevel('06_Hospital_Zone_02')`）。流れは本家のコードから写す決まりなので、救急車で作り、ガレージリフトはステップ 8 で仕掛けとして作る。項目を閉じるときに作業一覧の完了の条件を読み替える。
- 2026-09-18: 本家の場面は飛ばす（項目 25）: Zone 1 の `06_CutsceneStart` では `06_Hospital_Zone1_06Event` を流さず、すぐ `06 Transition` へ。Zone 2 の 7 は `Arrive Event`・捕まる場面・独房の場面を流さず、`PlayerStart_Cell` で `Cell Cutscene Finished` の状態から始める。
- 2026-09-18: 声・曲・ナースは別の項目: Bierce の台詞と `04_Intercom` のナースの放送（項目 20）、`BP_06_MusicPlayer` の `bFadeOut`（項目 19）、`Spawn Nurses`・`Spawn Nurses_06`・`bAttackDoor`・`Activate MiniBoss Enemies`（項目 7・11）。この項目では呼び出しの口（区間の準備の場所）だけ作り、中身はそれぞれの項目が埋める。
- 2026-09-18: 両開き扉は流れが名指しする 2 枚だけを置く（ステップ 3c）。`DoubleDoors33_36` の `Lock` → `Force Close` も 3c で結んだ（同じ扉の呼び出しなので）。
- 2026-09-18: 区間の流れは `AWasamiZoneFlow` の派生 2 つ（ゲームモードが開始時に出す。レベル BP の代わり）。本家の名前で置かれたアクタは `src:<名前>` タグで探す。残りのステップは各イベントの `Not yet:` の所を埋める（実装記録 11）。
- 2026-09-18: 作業ブランチ `feature/zone-progression` で進める（`.claude/guides/git-workflow.md` の大規模改修: 複数の仕組みにまたがり、複数回のコミットに分ける）。計画のコミットだけ main に置く。
- 2026-09-18: シーケンスは本家どおりレベルの `LevelSequenceActor`（タグ `src:<本家の名前>`）から再生する（本家のレベル BP は置かれたアクタの `GetSequencePlayer` → `Play`）。本家で Static の救急車と独房の扉は Static のまま（UE の Sequencer が動かす間だけ Movable にする。PIE で動くことを確かめた）。

## 要確認（ユーザー）

- 2026-09-18: Zone 2 へ移るときの読み込み画面の紋章 `loader_reapernurse`（本家の `UI/Main/Loaders`。リーパーナースの印の絵で、キャラクターそのものは描かれていない）を使ってよいか — 仮に使わず、暗い赤の全面だけを出している。理由: 原作の素材の使用範囲で「判断に迷うもの（キャラクターが写った画像や UI 素材など）」に当たる。使ってよければ `dd_ui` で取り込み `UWasamiLoadingWidget::LevelEmblems` の 7 番に入れるだけ。ほかの案: 本作のワサミの絵に替える。場所: 実装記録 09 の「既知の制約」、`WasamiLoadingWidget.h` の `LevelEmblems`。

- 2026-09-18: 作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」— 本家のコードどおり**救急車の上に乗って Zone 2 へ移る**形に読み替える。理由: 本家の `06_Hospital_Zone_01` のレベル BP で Zone 2 を開くのは `06_ReachAmbulance`（`TriggerBox_06_AmbulanceTop`）で、ガレージリフトは駐車場の車のリフトの仕掛け。場所: この記録の「決定事項」、項目を閉じるときに `.claude/roadmap.md` の項目 6 の完了の条件。

## 再開時の注意

- 結ぶ前に通ったトリガーの箱は使い切られる（本家も同じ）。04 はエレベーターの前の扉が鍵を破るまで、05 は駐車場への障壁が全回収まで道をふさぐ。PIE で 05 より先を確かめるときは、`Wasami.Flow On04DoorBreak` などで進めてから箱に立つ。扉の破壊は `python Tools/pie.py place 0 1010 --yaw -90 --pitch -20`（04 から 7 s 後）→ `pie.py cmd "Wasami.Interact 67"` で外れる。
- 収録は `desktop.py record` の既定（ddagrab）が止まるので `--grab gdi` と `t.MaxFPS 60`（症状索引）。
- ゲームのセーブ（本作の `structSlot`）はステップ 5 の後に `Wasami.ResetSave` した（次に Zone 1 を開くと 04 から）。05 から確かめるときは `Wasami.Flow On05Transition`。すでに 05 で始まっているときに `On05Transition` を重ねると全回収が 2 回結ばれ、2 回目が「no zone barrier」を出す（害は無い）。
- 救急車に乗せるには `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` の後に `python Tools/pie.py place 11245 -20055 470 --yaw 90 --pitch -10`。10.5 s 後に Zone 2 へ移り、セーブが 7 になる（確かめたら `Wasami.ResetSave`）。
- 06 の扉の所で PIE を確かめるときは `python Tools/pie.py place 7210 -21800 --yaw -90`（扉の 700 cm 手前。`-21300` は床が無く落ちる）。06 の箱は流れが結んでからでないと効かない（05 → `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` → `Wasami.Trigger 06_DoorsLock`）。
- 取り込みの後、エディタにメッセージログの窓が浮いて出る（閉じるボタン (2198, 407)）。PIE の収録の範囲はビューポート `--region 1826 202 2982 860`（2026-09-18 の窓の配置）。
- テストの結果はエディタのメッセージログの窓に出て、ビューポートの左に浮いて残る（収録の前に閉じる。2026-09-18 は閉じるボタンが (2198, 407)）。
- 無人運転のときエディタは背面（駆動役のターミナルが前面）なので、テストの前にエディタのタイトルバーの空き（2026-09-18 は (2800, 78)。撮って確かめる）を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押して前面にする。

## 検証

- ステップ 6: C++ ビルド OK、テスト `Wasami.*` 51 本すべて通過（`Wasami.ArrowPointer.Actor`・`.ZoneShardChecker` を足した）、check_records OK。`place_dd_flow` で両ゾーンに `shardCheckers` 1。PIE の地図の矢印のグリッド `Intermediate/Overnight/arrow_grid.png`。
- ステップ 5: C++ ビルド OK、テスト `Wasami.*` 49 本すべて通過（`Wasami.GameFlow.Loading` を足し、`Wasami.ZoneFlow.Zone1` に救急車のシーケンスが 1 s 後に流れるのを足した）、check_records OK。音 `21-Ballroom_portal_V2`（2.48 s）。PIE: 救急車の屋根で SAVING PROGRESS → 約 1 s 後に走り出す → 乗って 8 s で暗い赤の全面 → PIE の読み込みを含め約 4.7 s 後に Zone 2（グリッド `Intermediate/Overnight/ambulance_grid.png`）。
- ステップ 1〜4b: 各ステップのテスト・PIE の結果は実装記録 11 の「確かめたこと」と 08・09・01（最後はテスト 48 本すべて通過）。
