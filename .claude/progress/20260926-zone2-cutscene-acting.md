---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 10:40
---

# 項目 54 Zone 2 の捕まる場面・独房の場面の演技（項目 42 の再発）

## 依頼

2026-09-26 のユーザーの指摘（パッケージ版を遊んで）:

> Zone2のボスワサミ->牢獄へのシーンで、謎に手を掲げているワサミ、振り返る際のモーションが壊滅的。ワサミが殴りかかるシーン、床に倒れ込むシーンについて、本家を踏襲できていない

同じ日のユーザーの決定（作り方）:

> 走る動きは現状の .glb に同梱のものを優先し、Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用させる / Matron は対象外 / 武器を振りかざすシーンは対象外 / （移動する演技は）本家のアニメのまま滑らせる

当たりは 3 つ（作業一覧の項目 54 に詳しい）。さらに **(3b)** 捕まる場面の待ち構えと独房の場面の演技を、**本家のナースの psa を敵ワサミへリターゲット**したものに替える。

## 計画

- [x] 1. (a) の正体を確かめた → **Matron ではなく Zone 2 の見張り 6 体（`AWasamiEnemySentry`）の `Idle_Alert`（`Idle_5`）**（作業一覧の項目 54 の (a)）
- [x] 2. (b) 振り向きを直した → カメラアニメのずれをカメラ**アクタ**ではなく**カメラの部品**に当てるようにして、LookAt がアクタの keyed な道から追うようにした。PIE で測った向きは本家の回転キーと一致（下の「決定事項」、01 記録）
- [ ] 3. 本家のナースの psa → 敵ワサミのリターゲットの仕組みを作り、`ReaperNurse_Idle_Alert` 1 本で確かめる ← 次
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_enemy.py`（ナースの psa の口とリターゲット）、`/Game/Wasami/Enemy/A_WasamiEnemy_Cut_*`
- [ ] 4. 残り 10 本（`nurse_idle_01`・`Event_40`〜`42`・`44`〜`47`・`ReaperNurse_Walk_Back`・`nurse_cloak`）を焼き、`Event_43` の扱いを決める。**併せて (a) の直し**: 見張り 6 体（`AWasamiEnemySentry`）の `Idle_Alert` を、リターゲットした `ReaperNurse_Idle_Alert` に向ける（要確認 1）
- [ ] 5. 場面の読み替えを差し替え（捕まる場面・独房の場面）、(c) 殴打と倒れ込みを見直す。**カメラアニメの回転を部品に当てるかをここで決める**（要確認 2）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`、`Source/wasami_deception/WasamiCutsceneNurse.*`
- [ ] 6. 対応表・実装記録・`distribution.md` を直し、パッケージを作り直して 2 場面を通しで録って確かめる

## 次にやること

ステップ 3。本家のナースの psa（92 骨）を敵ワサミ v3（28 骨）へ載せ替える前処理を `dd_enemy` に作り、`ReaperNurse_Idle_Alert` 1 本だけ焼いて `/Game/Wasami/Enemy/A_WasamiEnemy_Cut_ReaperNurse_Idle_Alert` に取り込み、PIE で腕のひねり・足の接地・体の向きを見る。骨の対応と載せ方は下の「決定事項」。

## 決定事項

- 2026-09-26（ステップ 2）: **カメラアニメ `CameraAnim_Nurse_01` のずれは、カメラのアクタではなく `CameraComponent` の相対位置に打つ**（`dd_sequence.camera_offset`。`bake_location` は消した）。理由: `ACineCameraActor::Tick` の LookAt は**アクタの位置**から追う先を見た向きでアクタを回すので、ずれをアクタに足すと追う先との位置関係が変わって向きが壊れる（PIE で測った直す前の値は yaw 167 → 42・pitch −1 → −74 で床を向いていた＝指摘の「壊滅的」）。部品へ移すと向きは **yaw 173.39・pitch −4.93** に収まり、本家の回転キー（22.13 s の yaw 173.39・pitch −4.57）と一致した。本家のキー自身が「ずれを足さない位置から `cameralook` を見た向き」そのもの（t = 0 の yaw −1.72・pitch 1.40 も一致）なので、**本家も部品側に当てていた**と読む。見える絵（持ち上げ → 落下）はアニメを載せた部品の側なので変わらない。
- 2026-09-26（ステップ 2）: 作業一覧の項目 54 の (2) は「本家がキーを打つ区間で `bEnableLookAtTracking` を切る」と書いていたが、**切らずに済ませた**。切らなくても向きがキーと一致する（上）うえ、0〜19.67 s の細かいカメラワークは本家では `cameralook` の密なキーを LookAt が追って作っており、疎な回転キー（0・4・19.67 s の 3 つ）では再現できないため。
- 2026-09-26: **リターゲットは IK Rig / IK Retargeter ではなく、前処理（Python）で焼く。** 理由: 本作には既に (1) ActorX の psa を読む口（`dd_skeletal.read_psa`。骨 92 本の名前・親・基準姿勢と毎コマのキーが取れる）と (2) 骨の対応で回転を載せ替えて prepared glb に書き、UE へ取り込む仕組み（`dd_enemy._Retarget`。旧 glb の捕獲 3 本を v3 の骨へ載せた）がある。Blender の psk/psa アドオン・FBX・IK Rig のアセットを足さずに済み、やり直しがきく。
- 2026-09-26: **骨の対応**（ナース 92 → ワサミ v3 28）: `Nurse_ROOTSHJnt` → `pelvis`、`Spine_01/02/(03+04+Top)` → `spine_01/02/03`、`Neck_01`・`Neck_Top` → `neck_01`・`head`、`l/r_Arm_Clavicle/Shoulder/Elbow/Wrist` → `clavicle/upperarm/lowerarm/hand`、`l/r_Leg_Hip/Knee/Ankle/Ball` → `thigh/calf/foot/ball`。指 20・スカート 6・車輪 4・腰の補助 4・武器 1・`Trajectory`・メッシュのノードは捨てる。載せ方は基準姿勢を基準にした世界空間（`new_world(t) = old_world(t) · old_rest_world⁻¹ · new_rest_world`）で、骨盤の位置は背の比で縮める。ステップ 3 で実際に測って詰める。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`ReaperNurse_Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。

## 要確認（ユーザー）

1. **見張り 6 体の待機も、リターゲットした `ReaperNurse_Idle_Alert` に替えてよいか**（2026-09-26、ステップ 1）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近い。**無人運転では替える方向で進める**（ステップ 4）。違うならステップ 4 を戻す。
2. **捕まる場面のカメラアニメの「回転」を入れてよいか**（2026-09-26、ステップ 2）。2026-09-23 に「回転は入れない」を追認してもらったが、その理由は「LookAt が毎フレーム回転を書き直すので見えない」で、これは**ずれをアクタに足していたとき**の話。部品に当てる今は LookAt が触るのはアクタだけなので、**部品に当てれば回転も見える**。`CameraAnim_Nurse_01` の回転は yaw +180 → +203・pitch −45 → −71・roll −83 で、「掴まれて向きを変えられ、横倒しに倒れる」絵になり、(c) の「床に倒れ込む」に近づく見込み。いま見えている 20.5〜22.3 s は、ずれ（前へ 230 cm・上へ 205 cm）でナースを通り抜けた先の壁を向いたまま暗転する。**無人運転ではステップ 5 で入れる方向で試し、絵で判断する**。

## 再開時の注意

- ナースの psa は `pak_reference_2/_anims_psa/Animation/Enemies/Nurse/Reaper/`（`ReaperNurse_Idle_Alert`・`nurse_idle_01`・`ReaperNurse_Walk_Back`・`nurse_cloak`）と `.../06_Hospital/NurseIntro/Anims/StandingAnims/`（`Nurse_Hospital_Zone01_Event_40`〜`47`）。要る 12 本の在処は確かめ済み。骨は 12 本とも同じ 92 本（`Nurse` → `Nurse_SHJntGrp` → `Nurse_TrajectorySHJnt` → `Nurse_ROOTSHJnt` → 体幹）。
- **場面を PIE で流す手順**（git の外の道具）: `python Tools/pie.py start` → `cmd "t.MaxFPS 60"` →（撮るなら `python Tools/desktop.py record --grab gdi --region 1819 268 2865 1108 --seconds 30 --name <名前>.mkv`）→ `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"`（捕まる 26.23 s → 約 1 s → 独房 74.07 s）→ 終わったら `python Tools/pie.py stop`。**場面は 1 回の PIE で 1 回だけ流す**（2 回流すと前の独房の場面と重なって絵が読めない）。撮った物は `python Tools/video_probe.py sheet <mkv> <png> --start T --end T --every <フレーム数> --cols 5 --width 400` で並べる（`--every` は整数のフレーム間隔。47.7 fps なので 24 で約 0.5 s）。
- 姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`（1 回目で開始・2 回目で停止。出力 `Intermediate/Overnight/cut_pose.jsonl`）。カメラは `observations/tools/cut_camera_log.py`（同じ使い方。`Intermediate/Overnight/cut_camera.jsonl` にアクタと部品の位置・向き・追う先）。Matron 単体は `observations/tools/matron_pose.py`。ビューポートの範囲は `python observations/tools/check_viewport.py`。
- 前処理やシーケンスを変えたら `WasamiStageTools.place_dd_sequences(zone="Zone2")` → **別の呼び出しで** `build_navigation()`（置き直しでナビが空になる）。
- 本家のアニメがパッケージに入るので、ステップ 6 で `.claude/guides/distribution.md` に書く。
- パッケージの作り直しは `.claude/guides/distribution.md` の手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: OK（20 件。`dd_sequence.py` の変更に合わせて 01 記録を直した）
- C++ ビルド: この項目ではまだ C++ を変えていない
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 2 で Zone 2 のシーケンスを置き直し（6 本・トラック 56・区間 123・キー 574・`camera_anims` 1・`missing` 空）、ナビを焼き直した（29/29）。PIE で捕まる場面を 3 回流し、カメラのアクタと部品の位置・向きを毎フレーム測って本家の回転キーと突き合わせた（上の「決定事項」）。絵は `Intermediate/DesktopAgent/shots/z2cap-step2b.mkv`・`z2cap-step2-grid.png`・`z2cap-step2-early.png`（git の外）。PIE は停止済み。
