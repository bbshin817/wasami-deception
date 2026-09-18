---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-18 23:25
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

- Zone 2 のリフト（乗ると上がる床・角）はステップ 8a で作った（実装記録 12）。ガレージリフトの ABP の音の鳴らし方と、レベル BP の結び方は下。
- ガレージリフト（`Lifts/Garage/`・`Meshes/06_Hospital/hospital_garage_lift_anim_Skeleton_AnimBlueprint`）: 骨入りのメッシュ（拡縮 30）に ABP、`Box`（当たり）・`Overlap Box`（Pawn だけ重なり）が骨 `joint4` に付く。ABP は毎更新 `PlayerOn?` = 持ち主の `Player Overlapping?`（`Overlap Box` にプレイヤー。`_Zone1_Special` は `NurseNear` なら偽）。音 `Audio` = `DD_TT_GarageLift_Up`、`Audio1` = `_Down`（`01_Lobby_Attenuation`）。Zone 1 のレベル BP が `TriggerVolume_1` にナースが入ると `NurseNear` = 真（2 か所。項目 7 のナースと一緒に結ぶ）。
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
- [x] 7a. 独房から始める（2026-09-18 完了）: 8〜10 はステップ 1 で出来ていたので、7 の始まりだけ。ゲームモードの 7 → `PlayerStart_Cell`、7 だけ飛ばした場面が残す状態（救急車・`Ambulance_Arrive_Blockers4`・偽の天井・壁のスイッチ）、棘のシーケンス・独房の扉の鍵 → `OnCellDoorBreak`（扉のシーケンスと揺れ）、棘の死に `AWasamiHitFX`（本家の `BP_HitFX`）と `DD_Needle_Trap_R1_V3`（`dd_gimmicks.import_cell`）。テスト 52 本すべて通過、PIE で確かめた（実装記録 11・08・06・01）。
- [x] 7b. 独房の粒子（2026-09-18 完了）: `dd_gimmicks.import_cell` に `P_06_NurseSparks`・`Fracture_dark_slow`・`Concrete_impact_large` とテクスチャ 4・推定の材質 4（焼き込みのベースパスの式）を足し、`place_dd_sequences Zone2` でエミッタにテンプレートを入れた（`missing_particles` なし）。`dd_assets.main_export` の名前違いの書き出し。PIE で火花と扉の破片を確かめ、黒い塵は薄くて見えないので後回しの一覧へ（実装記録 08・01・11）。
- [x] 8a. 乗ると上がる床（2026-09-18 完了）: `AWasamiLiftBase`・`AWasamiLift`（`BP_06_Lift_03`・`_04`）・`AWasamiCornerLift`（`BP_06_LiftBase_Corner`）、前処理の `CLASS_MESHES`、`_flow` が Zone 2 に 15 台、`dd_gimmicks.import_lifts`（音 3）。テスト 2 本（54 本すべて通過）、PIE で走って乗ると上の階へ運ばれる（実装記録 12・01・08・11）。
- [x] 8b1. ガレージリフトの取り込み（2026-09-18 完了）: `dd_skeletal.py`（psa → glTF のアニメ、24 Hz で焼く `PL_DD_Skeletal`）で `/Game/DD/Meshes/06_Hospital/hospital_garage_lift_anim`（骨 4・`_Skeleton`・`_PhysicsAsset`）と `_Anim`（2.4583 s・59 コマ）、材質 4 は前処理の `CLASS_MATERIALS`、`M_DD_Substance` に `used_with_skeletal_mesh`、`dd_gimmicks.import_garage_lift`（実装記録 12・01・07・08）。
- [ ] 8b2. ガレージリフトのアクタ（アセットは 8b1 で作った。台 `joint4` は部品の拡縮 30 で約 9.5 cm → 316.7 cm に上がる。アニメの姿勢は `AnimationLibrary.get_bone_pose_for_time` で見られる）: `Source/wasami_deception/WasamiGarageLift.h/.cpp`（新規。12 記録に足す）と ABP の状態機械の写し（Default = 0 コマで止める〈再生速度 0・ループ〉、PlayerOn = 1 倍で 1 回、行き来は 0.2 s の HermiteCubic の混ぜ〈降りると 0.2 s で下りる〉、PlayerOn の入りの通知で `Audio`〈Up〉を鳴らす・出で `Audio` を 0.25 s で消して `Audio1`〈Down〉を鳴らす。`PlayerOn?` は毎更新 `Player Overlapping?`）、`dd_level._flow`（`BP_06_GarageLift` ×2〈Zone 2〉・`BP_06_GarageLift_Zone1_Special`〈Zone 1。`NurseNear` なら偽〉）。部品: `SkeletalMesh`（拡縮 30。コリジョンはエンジンの既定 NoCollision）→ 骨 `joint4` に `Box`（Custom・QueryAndPhysics・Pawn だけ Block、(0, 0, 0)・ロール −90・拡縮 (0.2031, 0.2734, 0.00953)）と `Overlap Box`（Pawn だけ Overlap、(0, −2.333, 0)・ロール −90・拡縮 (0.2031, 0.2734, 0.07127)）、`Audio`（Up）・`Audio1`（Down）は `01_Lobby_Attenuation`・自動で鳴らない。
- [ ] 9. Zone 2 の地図 `BP_MapTexture_MultiFloor` と `BP_MapArea` ×2（いる階の箱で地図の絵を `T_06_Zone2` ↔ `T_06_Zone2_02` に替える）。
- [ ] 10. 仕上げ: PIE で Zone 1 の到着 → 扉の破壊 → 全回収（デバッグで数個を残す）→ 障壁 → 駐車場 → トンネル → 扉が破られる → 救急車 → Zone 2 の独房 → 扉の破壊 → 迷路 → 全回収 → COLLECT THE RING PIECE までを通しで収録し、Discord のグリッドにする。実装記録・handover・作業一覧（項目 6 を完了、完了の条件の読み替え）・note を直し、進捗記録を消して main へマージし push。

## 次にやること

ステップ 8b2（ガレージリフトのアクタ）を始める。記録のステップ 8b2 を「作業中」にする。まず本家の置き場所（`pak_reference_2/_levels/06_Hospital_Zone_01.full.json`・`_02.full.json` の `BP_06_GarageLift*`）と、ABP の状態機械の写し方を決める（07 記録の `UWasamiEnemyAnimInstance` が C++ の状態と Proxy で本家の木を持つ手本。状態の入りと出の通知で音）。

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
- 独房の棘の箱 `Trigger_Cell_Spikes`（本家どおり棘に付いて下りる。x −14781〜−13559・y 1117〜2103）は廊下の端に少しかかる。廊下で長く待つときは箱の外（`place -13300 1000` など）に立つ。`slomo 5` 以上で待つときは `pie.py state` の間隔が長く、狙った時刻を越えやすい。Spikes の火花は Zone 2 を開いて 0〜4.3 s だけ（`--warmup 0` で始めてすぐ `place -12500 1100 --yaw 180` と `slomo 0.3`）。
- 2026-09-18 のステップ 7a の後、ゲームのセーブは 7（Zone 2 を開くと独房から）。独房では開いて約 19 s で棘に殺される（本家どおり）ので、PIE の確かめは素早く: `python Tools/pie.py place -14145 1330 --yaw -90 --pitch -15`（独房の扉の前）→ `pie.py cmd "Wasami.Interact 34"` で扉が開く。ライフ 3 つを使い切ると YOU ARE DEAD で止まる（PIE を始め直せば戻る）。
- 救急車に乗せるには `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` の後に `python Tools/pie.py place 11245 -20055 470 --yaw 90 --pitch -10`。10.5 s 後に Zone 2 へ移り、セーブが 7 になる（確かめたら `Wasami.ResetSave`）。
- 06 の扉の所で PIE を確かめるときは `python Tools/pie.py place 7210 -21800 --yaw -90`（扉の 700 cm 手前。`-21300` は床が無く落ちる）。06 の箱は流れが結んでからでないと効かない（05 → `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` → `Wasami.Trigger 06_DoorsLock`）。
- 取り込みの後、エディタにメッセージログの窓が浮いて出る（閉じるボタン (2198, 407)）。PIE の収録の範囲はビューポート `--region 1826 202 2982 860`（2026-09-18 の窓の配置）。
- テストの結果はエディタのメッセージログの窓に出て、ビューポートの左に浮いて残る（収録の前に閉じる。2026-09-18 は閉じるボタンが (2198, 407)）。
- 無人運転のときエディタは背面（駆動役のターミナルが前面）なので、テストの前にエディタのタイトルバーの空き（2026-09-18 は (2800, 78)。撮って確かめる）を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押して前面にする。

- Zone 2 のリフトを PIE で確かめるとき: 長い床 `lift_4` は `python Tools/pie.py place 6304 -2740 --yaw 90 --pitch -15`（`lift_11` と `lift_4` の間。`-2850` は `lift_11` の上で、乗ると上がる）から `desktop.py hold shift w --ms 700`（歩きでは乗れない。12 記録）。角の `lift_7` は `place 4500 -3619 95`（プレイヤーが上の階〈Z > 610〉にいた直後は角のリフトが上にあり、置くと床に食い込んで押し出されるので、下りてから）。リフトの高さは `LiftMesh` の相対 Z（`GetHeight` は UFUNCTION でない）。

## 検証

- ステップ 8b1: `import_dd_gimmicks` の通しで `garage_lift_skeletal_meshes` 1・`_animations` 1、アニメの `joint2`・`joint3` の平行移動が psa と一致（0 s: 11.617・0.041、2.458 s: 15.498・6.401）、スロット 4 つに名前どおりの材質、未保存のパッケージなし、check_records OK。見た目は 8b2 の PIE で見る。
- ステップ 8a: テスト `Wasami.*` 54 本すべて通過、check_records OK。`place_flow Zone2` = リフト 15・`failed_settings` 0。PIE のグリッド `Intermediate/Overnight/lift_grid.png`。
- ステップ 1〜7b: 各ステップのテスト・PIE の結果は実装記録 11 の「確かめたこと」と 08・09・03・01（7a でテスト 52 本すべて通過）。グリッド `Intermediate/Overnight/cell_grid.png`・`cell_particles_grid.png`・`arrow_grid.png`・`ambulance_grid.png`。
