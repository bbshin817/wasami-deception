---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 15:40
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
- [ ] 6. カメラアニメの回転を部品に当てて、(c) 殴打と倒れ込みを絵で判断する（要確認 2。`dd_sequence` の `camera_offset`・`CAMERA_OFFSET_CHANNELS`・`_camera_anim_move`）
- [ ] 7. `.claude/guides/distribution.md`（本家のアニメが入る）を直し、パッケージを作り直して 2 場面を通しで録って確かめる（対応表と 01 記録はステップ 5 で済み）

## 次にやること

ステップ 6。**捕まる場面のカメラアニメ `CameraAnim_Nurse_01` の回転を、移動と同じくカメラの部品（`CameraComponent` の相対回転）に打ってみて、絵で判断する**（要確認 2。`camera_offset` は今 `CAMERA_OFFSET_CHANNELS` の位置 3 軸だけを打ち、`_camera_anim_move` は回転 3 軸を「あることの確認」だけして捨てている。Matinee の回転の下トラックは `AXIS_RotationX/Y/Z` で、`MOVE_AXES[3:]` にある）。UE 4.24 は カメラアニメをカメラの空間で足す（`FCameraAnimationHelper::ApplyOffset`: 位置はカメラの向きで回して足し、**回転は掛け合わせる**）ので、部品の相対回転に打つのは同じ合成になる（部品の世界の向き = 部品の相対回転 ∘ アクタの向き、アクタの向きは LookAt が毎フレーム書く）。**判断の材料**: 今の絵では殴打の腕が横切った直後、**24.2〜25.2 s（場面の 20.5〜21.5 s）にレンガの壁を正面から見たまま暗転**する（下の「検証」の連番）。回転を入れて「掴まれて向きを変えられ、横倒しに倒れる」絵になれば (c) の答え。ならなければ入れずに戻し、理由を決定事項に書く。手順は下の「再開時の注意」。

## 決定事項

- 2026-09-26（ステップ 2）: **カメラアニメのずれは、カメラのアクタではなく `CameraComponent` の相対位置に打つ**（`dd_sequence.camera_offset`）。理由: `ACineCameraActor::Tick` の LookAt は**アクタの位置**から追う先を見た向きでアクタを回すので、ずれをアクタに足すと向きが壊れる（直す前は pitch −74° で床を向いていた＝指摘の「壊滅的」）。部品へ移すと向きは本家の回転キーと一致した（01 記録）。**この決定が要確認 2 とステップ 6 の前提**。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。ステップ 6・7 の絵でも直さない。

## 要確認（ユーザー）

1. **見張り 6 体の待機を、リターゲットした `ReaperNurse_Idle_Alert` に替えた**（2026-09-26、ステップ 1・4）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近いと判断した。**違うなら `dd_enemy.ROLES` の `Idle_Alert` の行を `(V3, "Idle_5", "loop")` に戻し、`SKIPPED` から `Idle_5` を外して取り込み直す**（捕まる場面の待ち構えだけを本家のものにするなら、`Cut_ReaperNurse_Idle_Alert` の役を足して `NURSE_ANIMS` の `ReaperNurse_Idle_Alert` をそこへ向ける）。
2. **捕まる場面のカメラアニメの「回転」を入れてよいか**（2026-09-26、ステップ 2）。2026-09-23 に「回転は入れない」を追認してもらったが、その理由は「LookAt が毎フレーム回転を書き直すので見えない」で、これは**ずれをアクタに足していたとき**の話。部品に当てる今は LookAt が触るのはアクタだけなので、**部品に当てれば回転も見える**。`CameraAnim_Nurse_01` の回転は yaw +180 → +203・pitch −45 → −71・roll −83 で、「掴まれて向きを変えられ、横倒しに倒れる」絵になり、(c) の「床に倒れ込む」に近づく見込み。**無人運転ではステップ 6 で入れる方向で試し、絵で判断する**。
3. **独房の場面のカメラの手前に、灰色の四角が浮いている**（2026-09-26、ステップ 5 の連番で見つけた。項目 54 の外）。正体は本家の落書きのデカール `Plane30_2`（`/Engine/BasicShapes/Plane` に材質 `M_06_Hospital_Decal_Graffiti_07`、位置 −14800, 1631, 205、拡縮 5.09）。**この材質の親 `/Game/Pipeline/Materials/M_DD_Decal` はドメインが `MD_DEFERRED_DECAL`** で、デカール専用の材質はスタティックメッシュに貼れないため、UE が既定の灰色で描いている。本家は同じ板に同じ材質を貼っているので、**前処理の側で「デカールの材質がメッシュに貼られているときは、半透明の面の材質（`Surface`・`Translucent`）を親にする」か、板を `DecalActor` に置き換えるか**の判断が要る。独房の場面のカメラの手前 2.6 m にあり、被写界深度でぼけた灰色の板として毎回映る。直すなら作業一覧に項目を足す。

## 再開時の注意

- **場面を PIE で流す手順**（git の外の道具）: `python Tools/pie.py start` → `cmd "t.MaxFPS 60"` → `python Tools/desktop.py start`（入っていなければ）→ `python Tools/desktop.py record --grab gdi --region 1819 68 3279 1268 --seconds 106 --name <名前>.mkv`（範囲は `python observations/tools/check_viewport.py` の値。高さは偶数に）→ `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"` → `python Tools/desktop.py wait --ms 112000 --timeout 150` → `record_status` → `python Tools/pie.py stop`。**場面は 1 回の PIE で 1 回だけ流す**（2 回流すと重なって絵が読めない）。
- **収録の時刻と場面の時刻**: 引き金から収録が始まるまで約 3.75 s。捕まる場面は収録の 3.75〜30.0 s（26.23 s）、独房は 31〜105 s（74.07 s）。連番は `python Tools/video_probe.py sheet <mkv> <png> --start T --end T --every <フレーム数> --cols 5 --width 400 --crop 0,90,1060,960`（51 fps なので `--every 26` で約 0.5 s、`--crop` はビューポートからエディタの枠を落とす）。
- 姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`（1 回目で開始・2 回目で停止。出力 `Intermediate/Overnight/cut_pose.jsonl`）。カメラは `observations/tools/cut_camera_log.py`（同じ使い方。アクタと部品の位置・向き・追う先）。
- 前処理やシーケンスを変えたら `python Tools/ue_remote.py observations/tools/nurse_step5_place.py`（`dd_sequence` を `importlib.reload` して `place("Zone2")`。エディタの Python はモジュールを抱え込む）→ **別の呼び出しで** `dd_level.build_navigation('')`（置き直しでナビが空になる）。
- 本家のアニメがパッケージに入るので、ステップ 7 で `.claude/guides/distribution.md` に書く。パッケージの作り直しは同じガイドの手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: OK（20 件。ステップ 5 で `dd_sequence.py` の変更に合わせて 01 記録を直した）
- C++ ビルド: この項目ではまだ C++ を変えていない
- **ステップ 5 の PIE**（収録 `Intermediate/DesktopAgent/shots/step5_scenes.mkv`、106 s・51 fps。連番は `Intermediate/Overnight/step5_*.png`。どれも git の外）:
  - シーケンスの区間を読み出して差し替えを確かめた（`06_Hospital_Zone2_Cell` の 17 区間が `Cut_nurse_idle_01`・`Cut_…_Event_40`〜`47`・`Cut_ReaperNurse_Walk_Back`・`Cut_nurse_cloak` で `reverse` は全て False、`06_Hospital_Zone2_Capture` は埋めの `Cut_nurse_idle_01` 0〜17.2 s → `Idle_Alert` 17.2〜19.17 s → 殴打 `Chase_PickUp` 19.23〜20.87 s）。置き直しは 6 シーケンス・29 結び付け・123 区間・埋め 5、ナビは 29/29。
  - 独房の場面: 台詞の間、腕を上げる・腰に当てる・身振りをする本家の演技が出る（基準姿勢も T ポーズも出ない）。透明化は場面の 23.9 s と終わり（収録 101.8〜102.4 s）に赤い粒とともに消える。
  - 捕まる場面: 殴打の腕が画面を横切った後（収録 23.8 s）、**24.2〜25.2 s はレンガの壁を正面から見たまま暗転する**（(c) の「床に倒れ込む」がまだ無い。ステップ 6 の判断の材料）。
