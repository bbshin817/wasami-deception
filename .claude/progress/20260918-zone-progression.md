---
title: ゾーンの進行（作業一覧の項目 6）
status: 進行中
branch: feature/zone-progression   # ステップ 1 の始めに main から作る（計画のコミットは main）
base: 5e296a2
started: 2026-09-18 17:59
updated: 2026-09-19 01:00
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

- リフト（Zone 2 の床・角）とガレージリフト（両ゾーン）はステップ 8 で作った（実装記録 12）。Zone 1 のガレージリフトの `NurseNear` は項目 7（11 記録）。
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
- [x] 8b2. ガレージリフトのアクタ（2026-09-18 完了）: `AWasamiGarageLift`・`AWasamiGarageLiftZone1Special`（`bNurseNear`。項目 7 で結ぶ）・本家の ABP を写した `UWasamiGarageLiftAnimInstance`（07 の `FWasamiStateBlend` で 0.2 s の混ぜ、入り / 出の通知で Up / Down）、`dd_level._flow` の `GARAGE_LIFT_CLASSES` が両ゾーンに 3 台。テスト `Wasami.GarageLift.Actor`（55 本すべて通過）、PIE で Zone 1 の台に乗ると 316 cm 上がる（実装記録 12・01・11・07）。
- [x] 9. Zone 2 の地図（2026-09-19 完了）: `AWasamiMapArea`・`AWasamiMapTextureMultiFloor`（本家の `BP_MapArea`・`BP_MapTexture_MultiFloor`。0.9 s ごとにいる階の地図の絵とその階のシャードの印だけを出す）、`AWasamiShard::GetPlane`、前処理がアクタを鍵にした `Map` を名前で残す、`dd_tablet` の `T_06_Zone2_02`、`dd_level._map_plane`・`place_minimap`（`WasamiStageTools.place_dd_minimap`）で Zone 2 に置いた。テスト 56 本すべて通過、PIE で階ごとに絵と印が替わる（実装記録 03・01・06）。
- [x] 10a. 仕上げ 1 — Zone 1 の通しの収録（2026-09-19 完了。仕上げを 10a〜10c に分けた）: 到着 → 扉の破壊 → 迷路と回収 → 駐車場 → 扉が破られる → トンネル → ガレージリフト → テレポーテーションで救急車の屋根 → 救急車に乗ってトンネル → 読み込み画面 → Zone 2 の独房まで PIE で通った。グリッド `Intermediate/Overnight/through_z1_1.png`〜`_5.png`。屋根の後ろの端では低いフレームレートで落ちると分かった（実装記録 11 の既知の制約・確かめたこと、症状索引）。
- [x] 10b. 仕上げ 2 — Zone 2 の通しの収録（2026-09-19 完了）: 独房の扉 → ミニボスの廊下（GET PAST THE NURSES）→ 迷路の入口（COLLECT ALL SHARDS）→ `lift_4` で上の階と地図の切り替え → 最後の 1 個で COLLECT THE RING PIECE まで PIE で通った。グリッド `Intermediate/Overnight/through_z2_1.png`〜`_4.png`（実装記録 11 の確かめたこと。`lift_4` は `-2810` から走る〈12 記録〉）。
- [ ] 10c. 仕上げ 3 — 閉じる: 実装記録・handover・作業一覧（項目 6 を完了、完了の条件の読み替え）・note を直し、進捗記録を消して main へマージし push。

## 次にやること

ステップ 10c（閉じる）。実装記録 11（項目 6 の完了を「変更履歴」へ）・`.claude/references/handover.md` の「現状と次の一歩」・`.claude/roadmap.md`（項目 6 を完了にし、完了の条件の「ガレージリフトで Zone 2 へ」を下の「決定事項」の読み替えに直す。大目標 1 の節のほかの項目が残るので大目標はまだ達成でない）・note の原稿 `docs/note/progress.md`（`.claude/guides/note-progress.md`。GIF の元は収録 `Intermediate/DesktopAgent/shots/through_z1_*.mkv`・`through_z2_*.mkv`、git の外）を直し、進捗記録を消して `feature/zone-progression` を main へマージし、ブランチを消して push。要確認 2 件は項目を閉じても残るので、作業一覧か handover の要確認へ移す。

## 決定事項

- 2026-09-18: **Zone 1 から Zone 2 へは、救急車の上に乗る（`06_ReachAmbulance`）で移る**。作業一覧の項目 6 と大目標 1 の「ガレージリフトで Zone 2 へ」は、作業一覧を作ったときの読み違い — 本家のレベル BP では、ガレージリフト（`BP_06_GarageLift_Zone1_Special`）は駐車場の車のリフト（乗ると上がる。ナースが近づくと上がらない）で、Zone 2 を開くのは `06_ReachAmbulance`（保存 7 → 救急車が出る → 読み込み画面 → `OpenLevel('06_Hospital_Zone_02')`）。流れは本家のコードから写す決まりなので、救急車で作り、ガレージリフトはステップ 8 で仕掛けとして作る。項目を閉じるときに作業一覧の完了の条件を読み替える。ステップ 8b2 で、Zone 1 のガレージリフトは救急車の真後ろにあり、上がった台から屋根へは歩いて渡れず、救急車にテレポーテーションの的 `BP_Power_Teleport_Zone_Ambulance` があると分かった（「リフトで上がりテレポーテーションで屋根へ」と読める。推定。ステップ 10 で確かめる）。
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
- 06 から始めるには、PIE で `Wasami.Checkpoint 6` → 止めて始め直す（シャードは残るので、見せる収録では `Wasami.CollectShards` で 0 にする）。ガレージリフトからテレポーテーションで屋根へ: 台で上がった後 Space → 2 → E → ホイール 2 目盛り（`desktop.py scroll --dx 120` を 2 回）→ 左クリック。寄せないと屋根の後ろの端に着き、フレームレートが低いと走り出してすぐ落ちる（実装記録 11）。
- 06 の扉の所で PIE を確かめるときは `python Tools/pie.py place 7210 -21800 --yaw -90`（扉の 700 cm 手前。`-21300` は床が無く落ちる）。06 の箱は流れが結んでからでないと効かない（05 → `Wasami.CollectShards` → `Wasami.Trigger 06_CutsceneStart` → `Wasami.Trigger 06_DoorsLock`）。
- 取り込みの後、エディタにメッセージログの窓が浮いて出る（閉じるボタン (2198, 407)）。PIE の収録の範囲はビューポート `--region 1826 202 2982 860`（2026-09-18 の窓の配置）。
- テストの結果はエディタのメッセージログの窓に出て、ビューポートの左に浮いて残る（収録の前に閉じる。2026-09-18 は閉じるボタンが (2198, 407)）。
- 無人運転のときエディタは背面（駆動役のターミナルが前面）なので、テストの前にエディタのタイトルバーの空き（2026-09-18 は (2800, 78)。撮って確かめる）を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押して前面にする。

- Zone 1 のガレージリフトを PIE で確かめるとき: `python Tools/pie.py place 11249 -21250 125 --yaw 90 --pitch -8`（台の上。2.5 s で上がる）、台の状態は `Box` のワールドの Z（下 9.3・上 316.4）と `is_player_overlapping()`。台から救急車へは歩いて渡れない（12 記録）。
- Zone 2 のリフトを PIE で確かめるとき: 長い床 `lift_4` は `python Tools/pie.py place 6304 -2810 --yaw 90 --pitch -15`（`lift_11` と `lift_4` の間。`-2740` からでは縁で止まることがある。`-2850` は `lift_11` の上で、乗ると上がる）から `desktop.py hold shift w --ms 700`（歩きでは乗れない。12 記録）。角の `lift_7` は `place 4500 -3619 95`（プレイヤーが上の階〈Z > 610〉にいた直後は角のリフトが上にあり、置くと床に食い込んで押し出されるので、下りてから）。リフトの高さは `LiftMesh` の相対 Z（`GetHeight` は UFUNCTION でない）。

## 検証

- ステップ 10b: PIE で Zone 2 の独房から COLLECT THE RING PIECE まで通った（実装記録 11 の「確かめたこと」の通し Zone 2）。グリッド `Intermediate/Overnight/through_z2_1.png`〜`_4.png`。終わりに `Wasami.ResetSave`、エディタは Zone 1 に戻した。
- ステップ 10a: PIE で Zone 1 の 04 から Zone 2 の独房まで通った（実装記録 11 の「確かめたこと」の通し）。グリッド `Intermediate/Overnight/through_z1_1.png`〜`_5.png`。
- ステップ 9: テスト `Wasami.*` 56 本すべて通過、check_records OK。`place_minimap Zone2` = 板 1・箱 2・`failed_settings` 0。PIE で下の階 `T_06_Zone2`・印 204、上の階 `T_06_Zone2_02`・印 138。グリッド `Intermediate/Overnight/zone2_map_grid.png`。
- ステップ 8b2: テスト `Wasami.*` 55 本すべて通過（`Wasami.GarageLift.Actor` の警告なし）、check_records OK。`place_flow` Zone1 = `garageLifts` 1、Zone2 = 2、`failed_settings` 0。PIE のグリッド `Intermediate/Overnight/garage_lift_grid.png`。
- ステップ 8b1: `import_dd_gimmicks` の通しでメッシュ 1・アニメ 1、アニメの平行移動が psa と一致（12 記録）。
- ステップ 8a: テスト `Wasami.*` 54 本すべて通過、check_records OK。`place_flow Zone2` = リフト 15・`failed_settings` 0。PIE のグリッド `Intermediate/Overnight/lift_grid.png`。
- ステップ 1〜7b: 各ステップのテスト・PIE の結果は実装記録 11 の「確かめたこと」と 08・09・03・01（7a でテスト 52 本すべて通過）。グリッド `Intermediate/Overnight/cell_grid.png`・`cell_particles_grid.png`・`arrow_grid.png`・`ambulance_grid.png`。
