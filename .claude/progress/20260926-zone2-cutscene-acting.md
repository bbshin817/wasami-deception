---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 17:10
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
- [ ] 7. (c) 倒れ込み: カメラアニメのずれ（移動 + 回転）を C++ で `UCameraComponent::AddAdditiveOffset` に渡す
- [ ] 8. `.claude/guides/distribution.md`（本家のアニメが入る）を直し、パッケージを作り直して 2 場面を通しで録って確かめる（対応表と 01 記録はステップ 5 で済み）

## 次にやること

ステップ 7。**捕まる場面のカメラアニメ `CameraAnim_Nurse_01` のずれ（移動 3 軸 + 回転 3 軸）を、シーケンスのトラックではなく実行時に `UCameraComponent::AddAdditiveOffset(FTransform, FOV)` へ渡す**（下の決定事項の 2026-09-26 ステップ 6）。作りの案:

- 曲線をどこへ持つか: 前処理 `dd_sequence._camera_anim_move` は既に 6 つの曲線（Matinee の接線つき）を読んで返す。`dd_assets.camera_anim` が本家の `CameraAnim` を写す先の **`UWasamiCameraAnim`（`Source/wasami_deception/WasamiCameraAnim.h`。04 記録）に移動トラックの `InterpCurveFloat` 6 本を足す**のが素直（今は `InterpTrackMove` を読まない）。`camera_offset` が打っている位置のキーは、加算のずれに移したら**消す**（二重に当たる）。
- いつ当てるか: 場面の 20.53〜25.27 s。`AWasamiZoneFlow::PlayCutscene` が持つ `ULevelSequencePlayer` の位置を毎フレーム見て、区間の中なら曲線を評価して `AddAdditiveOffset`、外なら `ClearAdditiveOffset`（`bUseAdditiveOffset` は当てた次の `GetCameraView` で消えるので毎フレーム入れ直す）。当て先は場面のシネカメラの `UCineCameraComponent`。
- 確かめ方: PIE で `observations/tools/cut_camera_log.py`（部品の向きではなく**見えている絵**が変わる。ログでは分からないので連番で見る）。今の絵は殴打の腕が横切った直後、レンガの壁を正面から見たまま暗転する（下の「検証」）。回転が乗れば最後に横倒しになるはず。
- C++ を触るので `python Tools/editor_cycle.py`（尋ねずに走らせる）。テストは `Wasami.` に足す。

## 決定事項

- 2026-09-26（ステップ 2）: **カメラアニメのずれは、カメラのアクタではなく `CameraComponent` の相対位置に打つ**（`dd_sequence.camera_offset`）。理由: `ACineCameraActor::Tick` の LookAt は**アクタの位置**から追う先を見た向きでアクタを回すので、ずれをアクタに足すと向きが壊れる（直す前は pitch −74° で床を向いていた＝指摘の「壊滅的」）。部品へ移すと向きは本家の回転キーと一致した（01 記録）。**この決定が要確認 2 とステップ 6 の前提**。
- 2026-09-26（ステップ 6）: **カメラアニメの回転は、シーケンスのトラックからは当てられない。本家と同じく部品の「加算のずれ」に渡す**。理由: `UCameraComponent::GetCameraView` は、持ち主が `ACineCameraActor` でその LookAt がこのフレームに走っていたら**部品の世界の向きを LookAt の向きへ書き戻す**（`LastLookatTrackingRotationFrame == GFrameNumber` なら `SetWorldRotation(LastLookatTrackingRotation)`）ので、アクタに打っても部品に打っても回転は消える（PIE で測った: キーは入っているのに部品の相対回転は毎フレーム 0。`cut_nolookat.py` で LookAt を切ると同じキーが曲線どおりに出る）。本家（UE 4.24）はカメラアニメを `AddAdditiveOffset` に渡し、`GetCameraView` はこの書き戻しの**直後**に `bUseAdditiveOffset` を当てるので、**本家では回転（最後に roll −83 で横倒し）が見えている**。つまり (c) の「床に倒れ込む」は本家のカメラアニメの回転そのもの。**2026-09-23 の「回転は入れない」は前提ごと落ちた**（要確認 2 は答えが出たのでこの決定に畳んだ）。01 記録の「既知の制約・注意点」と症状索引に書いた。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。ステップ 6・7 の絵でも直さない。

## 要確認（ユーザー）

1. **見張り 6 体の待機を、リターゲットした `ReaperNurse_Idle_Alert` に替えた**（2026-09-26、ステップ 1・4）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近いと判断した。**違うなら `dd_enemy.ROLES` の `Idle_Alert` の行を `(V3, "Idle_5", "loop")` に戻し、`SKIPPED` から `Idle_5` を外して取り込み直す**（捕まる場面の待ち構えだけを本家のものにするなら、`Cut_ReaperNurse_Idle_Alert` の役を足して `NURSE_ANIMS` の `ReaperNurse_Idle_Alert` をそこへ向ける）。
2. **（2026-09-26 のステップ 6 で答えが出たので取り下げ）** 捕まる場面のカメラアニメの回転は、本家では見えている（上の決定事項）。ステップ 7 で入れる。
3. **独房の場面のカメラの手前に、灰色の四角が浮いている**（2026-09-26、ステップ 5 の連番で見つけた。項目 54 の外）。正体は本家の落書きのデカール `Plane30_2`（`/Engine/BasicShapes/Plane` に材質 `M_06_Hospital_Decal_Graffiti_07`、位置 −14800, 1631, 205、拡縮 5.09）。**この材質の親 `/Game/Pipeline/Materials/M_DD_Decal` はドメインが `MD_DEFERRED_DECAL`** で、デカール専用の材質はスタティックメッシュに貼れないため、UE が既定の灰色で描いている。本家は同じ板に同じ材質を貼っているので、**前処理の側で「デカールの材質がメッシュに貼られているときは、半透明の面の材質（`Surface`・`Translucent`）を親にする」か、板を `DecalActor` に置き換えるか**の判断が要る。独房の場面のカメラの手前 2.6 m にあり、被写界深度でぼけた灰色の板として毎回映る。直すなら作業一覧に項目を足す。

## 再開時の注意

- **場面を PIE で流す手順**（git の外の道具）: `python Tools/pie.py start` → `cmd "t.MaxFPS 60"` → `python Tools/desktop.py start`（入っていなければ）→ `python Tools/desktop.py record --grab gdi --region 1819 68 3279 1268 --seconds 106 --name <名前>.mkv`（範囲は `python observations/tools/check_viewport.py` の値。高さは偶数に）→ `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"` → `python Tools/desktop.py wait --ms 112000 --timeout 150` → `record_status` → `python Tools/pie.py stop`。**場面は 1 回の PIE で 1 回だけ流す**（2 回流すと重なって絵が読めない）。
- **収録の時刻と場面の時刻**: 引き金から収録が始まるまで約 3.75 s。捕まる場面は収録の 3.75〜30.0 s（26.23 s）、独房は 31〜105 s（74.07 s）。連番は `python Tools/video_probe.py sheet <mkv> <png> --start T --end T --every <フレーム数> --cols 5 --width 400 --crop 0,90,1060,960`（51 fps なので `--every 26` で約 0.5 s、`--crop` はビューポートからエディタの枠を落とす）。
- 姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`（1 回目で開始・2 回目で停止。出力 `Intermediate/Overnight/cut_pose.jsonl`）。カメラは `observations/tools/cut_camera_log.py`（同じ使い方。アクタと部品の**世界と相対**の位置・向き・追う先）。LookAt を切って流すのは `observations/tools/cut_nolookat.py`（ステップ 6 で作った。どれも git の外）。
- 前処理やシーケンスを変えたら `python Tools/ue_remote.py observations/tools/nurse_step5_place.py`（`dd_sequence` を `importlib.reload` して `place("Zone2")`。エディタの Python はモジュールを抱え込む）→ **別の呼び出しで** `dd_level.build_navigation('')`（置き直しでナビが空になる）。
- 本家のアニメがパッケージに入るので、ステップ 7 で `.claude/guides/distribution.md` に書く。パッケージの作り直しは同じガイドの手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: OK（ステップ 6 で `dd_sequence.py` の変更に合わせて 01 記録を直した）
- C++ ビルド: この項目ではまだ C++ を変えていない（ステップ 7 で触る）
- **ステップ 5 の PIE**: 独房の場面は本家の演技（腕を上げる・腰に当てる・身振り）が出て、基準姿勢も T ポーズも出ない。透明化も本家の刻みで消える。捕まる場面は殴打の腕が画面を横切った後、**レンガの壁を正面から見たまま暗転する**（(c) の倒れ込みが無い）。
- **ステップ 6 の PIE**（収録 `Intermediate/DesktopAgent/shots/step6_capture.mkv`、33 s・42 fps。連番は `Intermediate/Overnight/step6_a.png`・`step6_b.png`。どれも git の外）: 回転 3 軸を部品に打って流したが、**絵はステップ 5 と同じ**（壁を見たまま暗転）。測ると部品の相対回転は全区間 0 で、部品の世界の向きはアクタと 1 度も違わなかった（`cut_camera.jsonl`）。区間には回転のキーが 16 本ずつ入っている（`Rotation.X/Y/Z`）。Python から相対回転を入れても次のフレームで 0 に戻る（`cam_rel_fight.txt`）。**LookAt を切ると同じキーが曲線どおりに出た**（`cut_nolookat.txt`: 場面の 22.4 s で roll −82.93・yaw 203.24・pitch −11.62、22.73 s 以降 roll −81.39・yaw 202.98）。これで原因が `UCameraComponent::GetCameraView` の書き戻しだと確定し、回転のキーは消してシーケンスを組み直した（部品の区間は `Location.X/Y/Z` 16 本ずつだけ）。置き直しは 6 シーケンス・29 結び付け・123 区間、ナビは 29/29。
