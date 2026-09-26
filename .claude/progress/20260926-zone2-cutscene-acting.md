---
title: 項目 54 Zone 2 の捕まる場面・独房の場面の演技
status: 進行中
branch: main
base: 02a1ea4
started: 2026-09-26 08:25
updated: 2026-09-26 14:05
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
- [x] 3. ナースの psa → v3 のリターゲットを `dd_enemy` に作った（`_NurseRetarget`・`NURSE_BONES`・`_nurse_world`。作り方と限界は 07 記録）
- [x] 4. 本家のナースの 12 本を焼いて取り込んだ（`Idle_Alert` + `Cut_*` 11 本）。`Event_43` は載せる側にし、(a) の直しとして見張りの `Idle_Alert` を本家のものに替えた（下の「決定事項」、07・01 記録）
- [ ] 5. 場面の読み替えを差し替え（捕まる場面・独房の場面）、(c) 殴打と倒れ込みを見直す。**カメラアニメの回転を部品に当てるかをここで決める**（要確認 2）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`（`NURSE_ANIMS`・`ONE_SHOT_CLIPS`）、`Source/wasami_deception/WasamiCutsceneNurse.*`
- [ ] 6. 対応表（`.claude/references/enemy-wasami-motions.md` の「場面の代用」）・実装記録・`distribution.md` を直し、パッケージを作り直して 2 場面を通しで録って確かめる

## 次にやること

ステップ 5。`dd_sequence.NURSE_ANIMS` の読み替えを、取り込んだ本家のクリップに差し替える（`nurse_idle_01` → `Cut_nurse_idle_01`、`Nurse_Hospital_Zone01_Event_40`〜`47` → `Cut_…_Event_40`〜`47`〈`43` も〉、`ReaperNurse_Walk_Back` → `Cut_ReaperNurse_Walk_Back`〈逆再生の `True` は要らなくなる。本家のクリップ自身が後ずさり）、`nurse_cloak` → `Cut_nurse_cloak`。`ReaperNurse_Idle_Alert`・`ReaperNurse_Boss_Idle_01` は `Idle_Alert` のままでよい（役の中身が本家のものになった）。**`ONE_SHOT_CLIPS`（1 回で区間を埋めるよう遅くするもの）に `Cut_*` を入れるかを決める** — 本家は区間の長さでそのまま流すので、入れずに等速で流し、余りは最後の姿勢で持たせるのが本家に近い（`dd_sequence` の `play_rate` と `fill_rest_pose` を読んでから決める）。`Chase_PickUp`（殴打の代用）はそのまま。差し替えたら `WasamiStageTools.place_dd_sequences(zone="Zone2")` → 別の呼び出しで `build_navigation()` → PIE で 2 場面を流して連番で見る（下の「再開時の注意」）。**カメラアニメの回転（要確認 2）もこのステップで絵を見て決める**。

## 決定事項

- 2026-09-26（ステップ 2）: **カメラアニメ `CameraAnim_Nurse_01` のずれは、カメラのアクタではなく `CameraComponent` の相対位置に打つ**（`dd_sequence.camera_offset`）。理由: `ACineCameraActor::Tick` の LookAt は**アクタの位置**から追う先を見た向きでアクタを回すので、ずれをアクタに足すと向きが壊れる（直す前は pitch −74° で床を向いていた＝指摘の「壊滅的」）。部品へ移すと向きは本家の回転キーと一致した（01 記録）。**この決定が要確認 2 の前提**。
- 2026-09-26（ステップ 4）: **`Idle_Alert` の役そのものを本家の `ReaperNurse_Idle_Alert` に替えた**（要確認 1 をこの向きで進めた）。`bAggressiveIdle` を立てるのは Zone 2 の見張り 6 体だけなので、この役を替えても迷路のナースたちは変わらない。捕まる場面の待ち構えも `NURSE_ANIMS` で同じ役を引いているので、C++ も読み替えも触らずに両方が本家の構えになる（列挙に `Cut_*` を足す口は要らなかった）。v3 の `Idle_5` は誰も使わなくなったので `SKIPPED` に入れ、取り込み済みの `A_WasamiEnemy_Idle_5` とステップ 3 の `A_WasamiEnemy_Cut_ReaperNurse_Idle_Alert` は消した（どちらも前処理が作った物で、この記録が名指ししている物）。
- 2026-09-26（ステップ 4）: **`Event_43` は載せる側**（作業一覧の「見て決める」への答え）。上げるのは片手で、右手首が頭の 7 cm 上まで 4.9 m/s、左手は 1.1 m/s で止まったまま。殴打 `Event_39` は 4 cm を 11.6 m/s で上げて 4.9 m/s で振り下ろすので、別物の身振り（`observations/tools/nurse_ev43_probe.py`）。Blender で本家と v3 を並べた絵（`Intermediate/Overnight/ev43_pair.png`）でも、腕を肩の高さへ振る台詞の身振りに見える。
- 2026-09-26（ステップ 4）: **病院の台詞の 8 本はコマの速さが整数でない**（`Event_40` は 186 コマ・6.127907 s = 30.19 fps）。本家の AnimSequence はキーを `SequenceLength` に等間隔で並べるので、本家でも 30 fps より少し速く流れているということ。`dd_skeletal.frame_rate(rel, whole=False)` を足してそのまま読み、30 fps の格子へ標本化し直した（取り込んだ長さは本家と 0.017 s 以内）。整数を求める既定は残した（整数でない速さは普通は読み違いの印）。
- 2026-09-26: `Event_46`（14.5 s）・`47`（5.0 s）・`Walk_Back` の**脚が滑る動きはそのまま使う**（2026-09-26 のユーザーの決定「本家のアニメのまま滑らせる」）。
- 本家のクリップの役の名前は `Cut_<本家のアニメ名>`、`ROLES` の `name` の列は**本家のパス**（psa と AnimSequence の両方をそこから引く）。

## 要確認（ユーザー）

1. **見張り 6 体の待機を、リターゲットした `ReaperNurse_Idle_Alert` に替えた**（2026-09-26、ステップ 1・4）。2026-09-26 の決定は「Zone2 のカットシーンでプレイヤーを攻撃 → 投獄までの流れ等に適用」で、見張り（ゲーム中の敵）は名指しされていない。ただし (a) の「手を掲げたワサミ」の正体は見張りの `Idle_5` で、本家の見張りは同じ場面で `ReaperNurse_Idle_Alert` を流すので、替えるのが本家に近いと判断した。**違うなら `dd_enemy.ROLES` の `Idle_Alert` の行を `(V3, "Idle_5", "loop")` に戻し、`SKIPPED` から `Idle_5` を外して取り込み直す**（捕まる場面の待ち構えだけを本家のものにするなら、`Cut_ReaperNurse_Idle_Alert` の役を足して `NURSE_ANIMS` の `ReaperNurse_Idle_Alert` をそこへ向ける）。
2. **捕まる場面のカメラアニメの「回転」を入れてよいか**（2026-09-26、ステップ 2）。2026-09-23 に「回転は入れない」を追認してもらったが、その理由は「LookAt が毎フレーム回転を書き直すので見えない」で、これは**ずれをアクタに足していたとき**の話。部品に当てる今は LookAt が触るのはアクタだけなので、**部品に当てれば回転も見える**。`CameraAnim_Nurse_01` の回転は yaw +180 → +203・pitch −45 → −71・roll −83 で、「掴まれて向きを変えられ、横倒しに倒れる」絵になり、(c) の「床に倒れ込む」に近づく見込み。いま見えている 20.5〜22.3 s は、ずれでナースを通り抜けた先の壁を向いたまま暗転する。**無人運転ではステップ 5 で入れる方向で試し、絵で判断する**。

## 再開時の注意

- **取り込みと確かめ**（git の外の道具。エディタもレベルも触らない）: `python Tools/ue_remote.py observations/tools/nurse_step4_import.py`（`dd_enemy.import_all()` → 12 本の長さを本家と並べる。エディタの Python はモジュールを抱え込むので、前処理を変えたら `importlib.reload` を通すこの台本から呼ぶ）、`observations/tools/nurse_step4_check.py`（取り込んだ 12 本を前処理の glb と比べ、足の高さ・頭の高さ・骨盤の移動を出す）。
- **1 本ずつ描いて見る**: `"C:\Program Files\Blender Foundation\Blender 4.0lender.exe" -b --factory-startup --python observations/tools/motion_gifs_blender.py -- Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb <出力先> <アニメ名>` で 20 fps の PNG。**Blender はアクション名を 63 文字で切る**ので、長い名前（`A_WasamiEnemy_Cut_Nurse_Hospital_Zone01_Event_4*`）は `…_Event_43_target_charact` のように切られた名前で指定する。本家のナース自身は `observations/tools/nurse_step4_probe.py`（`Intermediate/Overnight/nurse_<名前>.glb` を書く。`LOOK` に並べた分）→ 同じ台本に `--pelvis Nurse_ROOTSHJnt`。並べた絵は PIL で組む。
- **場面を PIE で流す手順**（git の外の道具）: `python Tools/pie.py start` → `cmd "t.MaxFPS 60"` →（撮るなら `python Tools/desktop.py record --grab gdi --region 1819 268 2865 1108 --seconds 30 --name <名前>.mkv`）→ `python Tools/pie.py cmd "Wasami.Flow OnArriveCaptureCutscene"`（捕まる 26.23 s → 約 1 s → 独房 74.07 s）→ 終わったら `python Tools/pie.py stop`。**場面は 1 回の PIE で 1 回だけ流す**（2 回流すと前の独房の場面と重なって絵が読めない）。撮った物は `python Tools/video_probe.py sheet <mkv> <png> --start T --end T --every <フレーム数> --cols 5 --width 400` で並べる（47.7 fps なので `--every 24` で約 0.5 s）。
- 姿勢を毎フレーム測るのは `python Tools/ue_remote.py observations/tools/cut_pose_log.py`（1 回目で開始・2 回目で停止。出力 `Intermediate/Overnight/cut_pose.jsonl`）。カメラは `observations/tools/cut_camera_log.py`（同じ使い方。アクタと部品の位置・向き・追う先）。ビューポートの範囲は `python observations/tools/check_viewport.py`。
- 前処理やシーケンスを変えたら `WasamiStageTools.place_dd_sequences(zone="Zone2")` → **別の呼び出しで** `build_navigation()`（置き直しでナビが空になる）。
- 本家のアニメがパッケージに入るので、ステップ 6 で `.claude/guides/distribution.md` に書く。
- パッケージの作り直しは `.claude/guides/distribution.md` の手順（前のパッケージは 2026-09-23 04:00）。

## 検証

- check_records: OK（20 件。ステップ 4 で `dd_enemy.py`・`dd_skeletal.py` の変更に合わせて 07・01 記録を直した）
- C++ ビルド: この項目ではまだ C++ を変えていない
- エディタでの確認（ステップ 4）: `import_all()` を走らせ（textures 5 / materials 3 / meshes 1 / **animations 29** / sounds 1）、本家の 12 本が入った。取り込んだ姿勢は前処理の glb と **0.0006 m 以内**で一致、足は床（−0.007〜+0.025 m。`Event_47` の踏み出しだけ +0.308 m）、頭は骨盤の 0.38〜0.55 m 上、長さは本家と 0.017 s 以内。`Event_43` は本家と v3 を並べて描いて演技の一致を見た（`Intermediate/Overnight/ev43_pair.png`、git の外）。`Tools/wasami_hands.py palms` の「最も内を向かない手のひら」は **−0.96**（本家の台詞の身振り。項目 37 の退行ではない。07 記録）。PIE は使っていない（レベルを触らずに済んだ）。
