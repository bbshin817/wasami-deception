---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 08:25
---

# 項目 54 Zone 2 の捕まる場面・独房の場面の演技（項目 42 の再発）

## 依頼

2026-09-26 のユーザーの指摘（パッケージ版を遊んで）:

> Zone2のボスワサミ->牢獄へのシーンで、謎に手を掲げているワサミ、振り返る際のモーションが壊滅的。ワサミが殴りかかるシーン、床に倒れ込むシーンについて、本家を踏襲できていない

同じ日のユーザーの決定（作り方）:

> 走る動きは現状の .glb に同梱のものを優先し、Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用させる / Matron は対象外 / 武器を振りかざすシーンは対象外 / （移動する演技は）本家のアニメのまま滑らせる

当たりは 3 つ（作業一覧の項目 54 に詳しい）:
- **(a)** 手を掲げたワサミ = Matron の `Detected`（`Lower_Weapon_Look_Raise`）が、武器の無い手を上げたまま止まっている見込み。
- **(b)** 振り向きが壊滅的 = シネカメラの LookAt が本家の回転のキー（yaw 4.5 → 173.4、pitch −6.1 → −4.6）を毎フレーム上書きしている。
- **(c)** 殴る → 床に倒れる が本家を踏襲できていない（(b) を直せば半分は直る見込み）。

さらに **(3b)** 捕まる場面の待ち構えと独房の場面の演技を、**本家のナースの psa を敵ワサミへリターゲット**したものに替える。

## 計画

- [ ] 1. (a) Matron の手の上がった停止を確かめて直す ← 次
  - 変更予定: `Source/wasami_deception/WasamiBossAnimInstance.*` か `Content/Python/wasami_tools/pipeline/dd_boss.py`（どちらを直すかは見てから）
- [ ] 2. (b) シネカメラの回転のキーを効かせる（本家が回転にキーを打つ区間で LookAt を切る）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`（`_cine_camera`・`_look_at`）
- [ ] 3. 本家のナースの psa → 敵ワサミのリターゲットの仕組みを作り、`ReaperNurse_Idle_Alert` 1 本で確かめる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_enemy.py`（ナースの psa の口とリターゲット）、`/Game/Wasami/Enemy/A_WasamiEnemy_Cut_*`
- [ ] 4. 残り 10 本（`nurse_idle_01`・`Event_40`〜`42`・`44`〜`47`・`ReaperNurse_Walk_Back`・`nurse_cloak`）を焼き、`Event_43` の扱いを決める
- [ ] 5. 場面の読み替えを差し替え（捕まる場面・独房の場面）、(c) 殴打と倒れ込みを見直す
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`、`Source/wasami_deception/WasamiCutsceneNurse.*`
- [ ] 6. 対応表・実装記録・`distribution.md` を直し、パッケージを作り直して 2 場面を通しで録って確かめる

## 次にやること

ステップ 1。PIE で Zone 2 の捕まる場面（`06_Hospital_Zone2_Capture`）を流し、その間の Matron を撮って、`Detected`（`Lower_Weapon_Look_Raise`、5.292 s）の最後の姿勢で手が上がったまま止まっているかを見る。上がっていれば、最後の姿勢から `Alert`/`Idle` へ戻すか、手の上がらない代用に替える（17 記録の `UWasamiBossAnimInstance`）。

## 決定事項

- 2026-09-26: **リターゲットは IK Rig / IK Retargeter ではなく、前処理（Python）で焼く。** 理由: 本作には既に (1) ActorX の psa を読む口（`dd_skeletal.read_psa`。骨 92 本の名前・親・基準姿勢と毎コマのキーが取れる）と (2) 骨の対応で回転を載せ替えて prepared glb に書き、UE へ取り込む仕組み（`dd_enemy._Retarget`。旧 glb の捕獲 3 本を v3 の骨へ載せた）がある。Blender の psk/psa アドオン・FBX・IK Rig のアセットを足さずに済み、やり直しがきく（作業一覧の項目 54 の「作り方の見込み」を差し替える）。
- 2026-09-26: **骨の対応**（ナース 92 → ワサミ v3 28）: `Nurse_ROOTSHJnt` → `pelvis`、`Spine_01/02/(03+04+Top)` → `spine_01/02/03`、`Neck_01`・`Neck_Top` → `neck_01`・`head`、`l/r_Arm_Clavicle/Shoulder/Elbow/Wrist` → `clavicle/upperarm/lowerarm/hand`、`l/r_Leg_Hip/Knee/Ankle/Ball` → `thigh/calf/foot/ball`。指 20・スカート 6・車輪 4・腰の補助 4・武器 1・`Trajectory`・メッシュのノードは捨てる。載せ方は基準姿勢を基準にした世界空間（`new_world(t) = old_world(t) · old_rest_world⁻¹ · new_rest_world`）で、骨盤の位置は背の比で縮める。ステップ 3 で実際に測って詰める。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`ReaperNurse_Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ナースの psa は `pak_reference_2/_anims_psa/Animation/Enemies/Nurse/Reaper/`（`ReaperNurse_Idle_Alert`・`nurse_idle_01`・`ReaperNurse_Walk_Back`・`nurse_cloak`）と `.../06_Hospital/NurseIntro/Anims/StandingAnims/`（`Nurse_Hospital_Zone01_Event_40`〜`47`）。要る 12 本の在処は確かめ済み。骨は 12 本とも同じ 92 本（`Nurse` → `Nurse_SHJntGrp` → `Nurse_TrajectorySHJnt` → `Nurse_ROOTSHJnt` → 体幹）。
- 本家のアニメがパッケージに入るので、ステップ 6 で `.claude/guides/distribution.md` に書く。
- パッケージの作り直しは `.claude/guides/distribution.md` の手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
