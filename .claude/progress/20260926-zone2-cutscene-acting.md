---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 09:05
---

# 項目 54 Zone 2 の捕まる場面・独房の場面の演技（項目 42 の再発）

## 依頼

2026-09-26 のユーザーの指摘（パッケージ版を遊んで）:

> Zone2のボスワサミ->牢獄へのシーンで、謎に手を掲げているワサミ、振り返る際のモーションが壊滅的。ワサミが殴りかかるシーン、床に倒れ込むシーンについて、本家を踏襲できていない

同じ日のユーザーの決定（作り方）:

> 走る動きは現状の .glb に同梱のものを優先し、Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用させる / Matron は対象外 / 武器を振りかざすシーンは対象外 / （移動する演技は）本家のアニメのまま滑らせる

当たりは 3 つ（作業一覧の項目 54 に詳しい）:
- **(a)** 手を掲げたワサミ → **ステップ 1 で正体を確かめた（下の「決定事項」）。Matron ではなく Zone 2 の見張り 6 体の `Idle_Alert`（`Idle_5`）**。
- **(b)** 振り向きが壊滅的 = シネカメラの LookAt が本家の回転のキー（yaw 4.5 → 173.4、pitch −6.1 → −4.6）を毎フレーム上書きしている。
- **(c)** 殴る → 床に倒れる が本家を踏襲できていない（(b) を直せば半分は直る見込み）。

さらに **(3b)** 捕まる場面の待ち構えと独房の場面の演技を、**本家のナースの psa を敵ワサミへリターゲット**したものに替える。

## 計画

- [x] 1. (a) の正体を確かめる（Matron の手の上がった停止か、別のものか） → **Matron ではなかった**（下の「決定事項」）
- [ ] 2. (b) シネカメラの回転のキーを効かせる（本家が回転にキーを打つ区間で LookAt を切る） ← 次
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`（`_cine_camera`・`_look_at`）
- [ ] 3. 本家のナースの psa → 敵ワサミのリターゲットの仕組みを作り、`ReaperNurse_Idle_Alert` 1 本で確かめる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_enemy.py`（ナースの psa の口とリターゲット）、`/Game/Wasami/Enemy/A_WasamiEnemy_Cut_*`
- [ ] 4. 残り 10 本（`nurse_idle_01`・`Event_40`〜`42`・`44`〜`47`・`ReaperNurse_Walk_Back`・`nurse_cloak`）を焼き、`Event_43` の扱いを決める。**併せて (a) の直し**: 見張り 6 体（`AWasamiEnemySentry`）の `Idle_Alert` を、リターゲットした `ReaperNurse_Idle_Alert` に向ける（要確認 1）
- [ ] 5. 場面の読み替えを差し替え（捕まる場面・独房の場面）、(c) 殴打と倒れ込みを見直す
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`、`Source/wasami_deception/WasamiCutsceneNurse.*`
- [ ] 6. 対応表・実装記録・`distribution.md` を直し、パッケージを作り直して 2 場面を通しで録って確かめる

## 次にやること

ステップ 2。`dd_sequence` のシネカメラで、本家が回転にキーを打つ区間（捕まる場面 19.67〜22.13 s の yaw 4.5 → 173.4・pitch −6.1 → −4.6）に LookAt 追従が勝たないようにする。今の作りは `_cine_camera`/`_look_at` が本家のレベルの `bEnableLookAtTracking`（真）と `ActorToTrack`（`cameralook`）をそのまま写しており、`ACineCameraActor::Tick` が毎フレーム回転を書き直すのでシーケンスの回転のキーが残らない。直したら PIE でカメラの実際の yaw・pitch を測ってキーに沿うことを確かめる（01 記録の「既知の制約・注意点」を書き直す）。

## 決定事項

- 2026-09-26（ステップ 1）: **(a)「手を掲げたワサミ」は Matron ではない。** PIE で捕まる場面（`Wasami.Flow OnArriveCaptureCutscene`）を流して測った: 場面の間ずっと Matron は `Idle`（`Long_Breathe_and_Look_Around`、重み 1.0）で、手は頭より 2.6〜3.0 m 下（`hand_l − head` −270 cm・`hand_r − head` −299 cm。メッシュの拡縮 5.15）。`Detected` はミニボスの廊下でコーンに見つかったときしか流れないので、この場面では一度も流れない。→ **`UWasamiBossAnimInstance` は直さない。**
- 2026-09-26（ステップ 1）: **ユーザーの言う場面は `06_Hospital_Zone2_Capture` → `06_Hospital_Zone2_Cell` で間違いない。** 捕まる場面のカメラは受付台の後ろに立つ巨大な Matron（ボスワサミ）を正面から大きく写す（(b) の LookAt でロビー側へ振り回されるため）。
- 2026-09-26（ステップ 1）: **「両手を横に掲げて」見えるのは Zone 2 の見張り 6 体（`AWasamiEnemySentry`）の `Idle_Alert`（`Idle_5`）。** 腕を左右へ伸ばした構えで、手は頭より 40〜48 cm 下。6 体とも同じ位相で流れるので、同じ形のまま止まって見える。捕まる場面でカメラがロビーを向くと画面に入る。本家の見張りは同じ場面で `ReaperNurse_Idle_Alert`（`bAggressiveIdle`）なので、**ステップ 3 で焼くリターゲットをそのまま見張りにも使えば本家と同じ構えになる**（要確認 1）。
- 2026-09-26（ステップ 1）: 場面の 2 体（`AWasamiCutsceneNurse`）は、**場面が始まってからは頭より上に手が上がらない**（唯一の例外は殴打 `Chase_PickUp` の振りかぶりで、場面の 19.3〜20.9 s に最大 +45.5 cm。これは本家の `Event_39` も同じ動き）。項目 42 の `Idle` の埋めは効いていて、基準姿勢は隠れている間しか出ない。独房の 1 体は 74 s ずっと `Idle_11` のループ。
- 2026-09-26: **リターゲットは IK Rig / IK Retargeter ではなく、前処理（Python）で焼く。** 理由: 本作には既に (1) ActorX の psa を読む口（`dd_skeletal.read_psa`。骨 92 本の名前・親・基準姿勢と毎コマのキーが取れる）と (2) 骨の対応で回転を載せ替えて prepared glb に書き、UE へ取り込む仕組み（`dd_enemy._Retarget`。旧 glb の捕獲 3 本を v3 の骨へ載せた）がある。Blender の psk/psa アドオン・FBX・IK Rig のアセットを足さずに済み、やり直しがきく。
- 2026-09-26: **骨の対応**（ナース 92 → ワサミ v3 28）: `Nurse_ROOTSHJnt` → `pelvis`、`Spine_01/02/(03+04+Top)` → `spine_01/02/03`、`Neck_01`・`Neck_Top` → `neck_01`・`head`、`l/r_Arm_Clavicle/Shoulder/Elbow/Wrist` → `clavicle/upperarm/lowerarm/hand`、`l/r_Leg_Hip/Knee/Ankle/Ball` → `thigh/calf/foot/ball`。指 20・スカート 6・車輪 4・腰の補助 4・武器 1・`Trajectory`・メッシュのノードは捨てる。載せ方は基準姿勢を基準にした世界空間（`new_world(t) = old_world(t) · old_rest_world⁻¹ · new_rest_world`）で、骨盤の位置は背の比で縮める。ステップ 3 で実際に測って詰める。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`ReaperNurse_Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。

## 要確認（ユーザー）

1. **見張り 6 体の待機も、リターゲットした `ReaperNurse_Idle_Alert` に替えてよいか**（2026-09-26、ステップ 1）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近い。**無人運転では替える方向で進める**（ステップ 4）。違うならステップ 4 を戻す。

## 再開時の注意

- ナースの psa は `pak_reference_2/_anims_psa/Animation/Enemies/Nurse/Reaper/`（`ReaperNurse_Idle_Alert`・`nurse_idle_01`・`ReaperNurse_Walk_Back`・`nurse_cloak`）と `.../06_Hospital/NurseIntro/Anims/StandingAnims/`（`Nurse_Hospital_Zone01_Event_40`〜`47`）。要る 12 本の在処は確かめ済み。骨は 12 本とも同じ 92 本（`Nurse` → `Nurse_SHJntGrp` → `Nurse_TrajectorySHJnt` → `Nurse_ROOTSHJnt` → 体幹）。
- **場面を PIE で流す手順**（ステップ 1 で使った。git の外の道具）: `python Tools/pie.py start` → `python Tools/pie.py cmd "t.MaxFPS 60"` → `python Tools/desktop.py record --grab gdi --region 1819 268 2865 1108 --seconds 110 --name <名前>.mkv` → `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"`（捕まる 26.23 s → 約 1 s → 独房 74.07 s）。姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`（1 回目で開始・2 回目で停止。出力 `Intermediate/Overnight/cut_pose.jsonl` に手と頭の高さの差）。Matron 単体は `observations/tools/matron_pose.py`。ビューポートの範囲は `python observations/tools/check_viewport.py` が出す（今は 1819 68 3279 1269、ゲームの絵だけなら 1819 268 2865 1108）。
- 本家のアニメがパッケージに入るので、ステップ 6 で `.claude/guides/distribution.md` に書く。
- パッケージの作り直しは `.claude/guides/distribution.md` の手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: 未実行（ソースはまだ変えていない）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 1 で PIE を 4 回（Zone 2）。捕まる場面と独房の場面を通しで録り（`Intermediate/DesktopAgent/shots/pie-54-full.mkv`、git の外）、Matron・場面の 2 体・見張り 6 体の手と頭の高さを毎フレーム測った。PIE は停止済み。
