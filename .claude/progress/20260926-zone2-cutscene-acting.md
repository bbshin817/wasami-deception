---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 21:10
---

# 項目 54 Zone 2 の捕まる場面・独房の場面の演技（項目 42 の再発）

## 依頼

2026-09-26 のユーザーの指摘（パッケージ版を遊んで）:

> Zone2のボスワサミ->牢獄へのシーンで、謎に手を掲げているワサミ、振り返る際のモーションが壊滅的。ワサミが殴りかかるシーン、床に倒れ込むシーンについて、本家を踏襲できていない

同じ日のユーザーの決定（作り方）:

> 走る動きは現状の .glb に同梱のものを優先し、Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用させる / Matron は対象外 / 武器を振りかざすシーンは対象外 / （移動する演技は）本家のアニメのまま滑らせる

当たりは 3 つ（作業一覧の項目 54 に詳しい）。さらに **(3b)** 捕まる場面の待ち構えと独房の場面の演技を、**本家のナースの psa を敵ワサミへリターゲット**したものに替える。

## 計画

- [x] 1. (a) の正体は **Zone 2 の見張り 6 体（`AWasamiEnemySentry`）の `Idle_Alert`**（Matron ではない）
- [x] 2. (b) 振り向きを直した → カメラアニメのずれをカメラ**アクタ**ではなく**カメラの部品**へ（01 記録）
- [x] 3. ナースの psa → v3 のリターゲットを `dd_enemy` に作った（`_NurseRetarget`。07 記録）
- [x] 4. 本家のナースの 12 本を焼いて取り込み、見張りの `Idle_Alert` を本家のものに替えた（07・01 記録）
- [x] 5. 読み替えを本家のクリップへ差し替え（`NURSE_ANIMS` の `Cut_*`・`FILL_CLIP`）、Zone 2 を置き直して 2 場面を PIE で通して見た。対応表と 01 記録も直した
- [x] 6. 回転を部品に当てて試し、**シーケンスからは入れられない**と分かった（LookAt が書き戻す）。本家は部品の**加算のずれ**に渡していると突き止めた（01 記録・症状索引）
- [x] 7. (c) 倒れ込み: カメラアニメの移動と回転を `AWasamiCameraAnimOffset` + `UWasamiCameraAnimOffsetModifier` で視点に当てた。PIE で横倒しが見えた（01・04 記録・症状索引）
- [ ] 8. `.claude/guides/distribution.md`（本家のアニメが入る）を直し、パッケージを作り直して 2 場面を通しで録って確かめる（対応表と 01 記録はステップ 5 で済み）

## 次にやること

ステップ 8（最後）。**`.claude/guides/distribution.md` に本家のアニメが入ることを書き、パッケージを作り直して 2 場面を通しで録って確かめる**。

- 配布ガイド: 本家のナースの psa から焼いたクリップ 12 本（`/Game/Wasami/Enemy/A_WasamiEnemy_*`。見張りの `Idle_Alert` と独房の場面の 11 本）と、本家のカメラアニメ `/Game/DD/Animation/06_Hospital/CameraAnim_Nurse_01` がパッケージに入る。
- パッケージ: 同じガイドの手順（前のパッケージは 2026-09-23 04:00）。
- 確かめ: パッケージ版で Zone 2 を進めて捕まる場面と独房の場面を通しで録り、(a) 手を掲げたワサミ・(b) 振り向き・(c) 倒れ込みの 3 つが直っていることを見る。PIE での絵はステップ 7 の「検証」にある。

## 決定事項

- 2026-09-26（ステップ 2・6・7、実装記録へ移したので要点だけ）: **カメラアニメのずれは、シーケンスのトラックでは当てられない**（アクタに足すと LookAt が壊れ、部品に打つと回転が `GetCameraView` に書き戻される）。**部品の加算のずれも使えない**（UE 5.8 のシーケンサーのカメラシェイクが毎フレーム `ClearAdditiveOffset` する。捕まる場面は 22.133 s から揺れの区間がある）。**カメラモディファイアで視点に当てる**のが答え（01 記録の「既知の制約・注意点」、04 記録の `UWasamiCameraAnimOffsetModifier`、症状索引の 2 件）。
- 2026-09-26（ステップ 7）: **アニメのキーは 2.0 s までで区間は 4.733 s あるが、曲線は最後の値を保たせる**（`FInterpCurve::Eval` のまま。本家の `AnimLength` は書き出しに無く UE4 の既定 3.0 なので、本家は 3.0 s で切れた可能性がある）。理由: この場面のフェードは 22.27 s から 25.2 s で真っ暗になるので、2.0 s の後（22.53 s 以降）はほとんど見えず、保つ方が急に起き上がらない。
- 2026-09-26（ステップ 7）: **見張りの `Idle_Alert` の長さのテストがステップ 4 から落ちていた**ので直した（`WasamiEnemyTests.cpp` の `ImportedLengths`、57 コマ → 180 コマ = 6.000 s）。07 記録は初めから 6.000 s と書いてあった。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。

## 要確認（ユーザー）

1. **見張り 6 体の待機を、リターゲットした `ReaperNurse_Idle_Alert` に替えた**（2026-09-26、ステップ 1・4）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近いと判断した。**違うなら `dd_enemy.ROLES` の `Idle_Alert` の行を `(V3, "Idle_5", "loop")` に戻し、`SKIPPED` から `Idle_5` を外して取り込み直す**（捕まる場面の待ち構えだけを本家のものにするなら、`Cut_ReaperNurse_Idle_Alert` の役を足して `NURSE_ANIMS` の `ReaperNurse_Idle_Alert` をそこへ向ける）。
2. **独房の場面のカメラの手前に、灰色の四角が浮いている**（2026-09-26、ステップ 5 の連番で見つけた。項目 54 の外）。正体は本家の落書きのデカール `Plane30_2`（`/Engine/BasicShapes/Plane` に材質 `M_06_Hospital_Decal_Graffiti_07`、位置 −14800, 1631, 205、拡縮 5.09）。**この材質の親 `/Game/Pipeline/Materials/M_DD_Decal` はドメインが `MD_DEFERRED_DECAL`** で、デカール専用の材質はスタティックメッシュに貼れないため、UE が既定の灰色で描いている。本家は同じ板に同じ材質を貼っているので、**前処理の側で「デカールの材質がメッシュに貼られているときは、半透明の面の材質（`Surface`・`Translucent`）を親にする」か、板を `DecalActor` に置き換えるか**の判断が要る。独房の場面のカメラの手前 2.6 m にあり、被写界深度でぼけた灰色の板として毎回映る。直すなら作業一覧に項目を足す。

## 再開時の注意

- **場面を PIE で流す手順**（git の外の道具）: `python Tools/pie.py start` → `cmd "t.MaxFPS 60"` → `python Tools/desktop.py click 2719 82 --allow WindowsTerminal.exe`（**エディタを前面にする**。背面だと収録に端末やメッセージログの窓が写る）→ `python observations/tools/check_viewport.py` → `python Tools/desktop.py record --grab gdi --region 1819 68 3279 1268 --seconds 32 --name <名前>.mkv` → `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"` → `desktop.py wait --ms 36000` → `record_status` → `python Tools/pie.py stop`。**収録を始める前に `ue_remote.py` を呼ばない**（エディタのメッセージログの窓が前に出る。出たら右上の × を `desktop.py click 2199 407 --allow UnrealEditor.exe` で閉じる）。**場面は 1 回の PIE で 1 回だけ流す**。
- **収録の時刻と場面の時刻**: 引き金の少し前から収録が始まるので毎回ずれる。まず `python Tools/video_probe.py sheet <mkv> <png> --start 0 --end 30 --every 100 --cols 5 --width 300 --crop 0,90,1060,960` で粗く見て、殴打の腕が写るコマを探してから細かく刻む（`--every 14` で約 0.3 s）。**`video_probe.py frames` は出力が長いので使わない**。
- 姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`。**視点（加算のずれが乗った絵）は `observations/tools/cut_pov_log.py`**（ステップ 7 で作った。`pov` と カメラの部品の世界の姿勢 `eye` の差が当たっているずれ。出力 `Intermediate/Overnight/cut_pov.jsonl`）。どちらも 1 回目で開始・2 回目で停止。
- 前処理やシーケンスを変えたら `python Tools/ue_remote.py observations/tools/nurse_step7_place.py`（`dd_assets`・`dd_sequence` を `importlib.reload` して `place("Zone2")` し、置いた `AWasamiCameraAnimOffset` の中身を出す）→ **別の呼び出しで** `dd_level.build_navigation('')`。エディタを開き直すと Zone 1 が開くので、先に `load_level('/Game/Stage/Maps/L_Hospital_Zone2')`。

## 検証

- check_records: OK（20 件）
- C++ ビルド: OK。`Automation RunTests Wasami` は **158 件すべて成功**（ステップ 4 から落ちていた `Wasami.Enemy.Anim.Clips` も直した）。カメラアニメのテストは `Wasami.CameraAnim.Move`（本家のキー 4 点で `EvalMove` の位置・ロール・ピッチ・ヨー、最後のキーの後の保持、`ApplyOffset` の合成、`IsInSection`）
- 置き直し: 6 シーケンス・29 結び付け・トラック 55・区間 122・キー 526・`camera_anims` 1・`camera_anim_offsets` 1、ナビ 29/29
- **ステップ 7 の PIE**: 視点の記録（`Intermediate/Overnight/cut_pov.jsonl`）で、場面の 20.53〜25.27 s の全区間でずれが視点に乗った（22.4 s 以降はロール −79.5 のまま。本家の曲線の −81.4 にシェイクが足された値）。収録 `Intermediate/DesktopAgent/shots/step7d_capture.mkv`、連番 `Intermediate/Overnight/step7_fall_grid.png`（どれも git の外）: 殴打の腕が横切った後、**カメラが横倒しになって廊下を横から見たまま暗転する**（(c) の倒れ込みが出た）。ステップ 5・6 の「レンガの壁を正面から見たまま暗転」は解消。
