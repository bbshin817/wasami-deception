---
title: Zone 2 の場面の代用の見直し（テラスの棒立ち・注射器で殴られる場面）（作業一覧の項目 42）
status: 進行中
branch: main
base: 8d85cc7
started: 2026-09-22 15:15
updated: 2026-09-22 16:40
---

# Zone 2 の場面の代用の見直し（作業一覧の項目 42）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 42（大目標 4）。ゲームレビュアーの指摘 2 件:

- (a)「Zone2のボスワサミと敵ワサミの対峙場面で、テラスに本家にはいないワサミが両手を掲げている」
- (b)「本家はナースに注射器で殴打されプレイヤーが床に倒れる場面だが、おそらくナースのボーンアニメを引き継げていないのか、全く理由のわからない状態になっている」

完了の条件は項目 42 の 4 つ（(1) 見えている間に棒立ち（基準姿勢）にならない、(2) 捕まる場面の代用を選び直す、(3) 独房と Zone 1 の出来事も同じ目で見直す、(4) 3 つの場面を撮って棒立ちが居ないことを確かめる）。

## 計画

- [x] 1. 棒立ちのワサミの正体を突き止めた（捕まる場面のテラスの代役 `nurse_idle1_2` が、頭の 17.2 秒を敵ワサミの基準姿勢で立っていた）
- [x] 2. アニメの切れ目を `Idle` で埋めた（完了の条件 1・3。`dd_sequence.fill_rest_pose`。01 記録）
- [ ] 3. **捕まる場面の殴打の代用を選び直す**（完了の条件 2） ← 次
  - 対象は `Nurse_Hospital_Zone01_Event_39`（19.233〜20.833 s、本家のアニメは 1.633 s）。今は `Chase_Charge`（頭を下げた突進、`play_rate` 0.3265 で 1 回）。v3 の 18 本から選び直す。**カメラはこのとき引いた絵**（テラスのワサミは画面の 1/10 ほど。`z2cap-t19.5.png`）なので、読めるかは絵で見る。
  - 変更予定: `dd_sequence.py` の `NURSE_ANIMS`、`.claude/references/enemy-wasami-motions.md` の「場面の代用」、組み立て直し。
- [ ] 4. **Zone 1 の `06_Hospital_Zone1_06Event` を同じ目で見直す**（完了の条件 3）
  - `BP_06_ReaperNurse_06Special`・`2` は**骨アニメのトラックを持たない**（可視・変換・音だけ）。本作でこの 2 体が何として置かれ、何を再生しているかを PIE で見る（代役 `AWasamiCutsceneNurse` ではなく組み立てが置く敵のはず）。
- [ ] 5. **3 つの場面を撮って並べ、棒立ちが居ないことを確かめる**（完了の条件 4）。実装記録 07・11 と作業一覧を直し、記録を消す

## 次にやること

ステップ 3。`NURSE_ANIMS` の `Nurse_Hospital_Zone01_Event_39` の代用（今は `Chase_Charge`）を v3 の 18 本から選び直す。候補は `.claude/references/enemy-wasami-motions.md` の一覧から。選んだら `place_dd_sequences Zone2` → `build_navigation` → PIE で 19.2〜20.9 s を撮って読めるか見る。

## 決定事項

- 2026-09-22: **(a) の正体**（ステップ 1）— 捕まる場面 `06_Hospital_Zone2_Capture` のテラスの代役 `nurse_idle1_2` が、頭の 17.2 秒のあいだアニメの区間に覆われず、敵ワサミ v3 の基準姿勢（腕を左右に広げた `restpose`）で立っていた。本家でもこのナースは t = 0 から見えている（可視の区間は無限・キーは両方 `bHidden = false`）ので、直すのは可視ではなく姿勢。
- 2026-09-22: **直し方は「基準姿勢が出る切れ目だけを `Idle` で埋める」**（ステップ 2、実装済み。理由と規則は 01 記録の「シーケンス」の骨のアニメへ移した）。`KeepState` の区間の後は最後の姿勢が残り、それが本家の見え方なので触らない。
- 2026-09-22: **(b) の今の代用**（ステップ 3 で選び直す対象）— 待ち構え `ReaperNurse_Idle_Alert` → `Idle_Alert`（`Idle_5`）、殴る `Nurse_Hospital_Zone01_Event_39`（19.233〜20.833 s）→ `Chase_Charge`（`Male_Head_Down_Charge`、0.533 s・2.17 m 前進）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- レベルの資産のパスは **`/Game/Stage/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2`**、シーケンスは `/Game/DD/Animation/06_Hospital/…`。
- **`place_dd_sequences`（`dd_sequence.place`）の後はレベルを開き直して `build_navigation`**（保存で道が空になる。01 記録）。Zone 1 と Zone 2 の両方。今は両方とも焼いて保存済み。
- 調べ直すときの道具（git の外。`Intermediate/Scratch/`）:
  - `check_fills.py`（`python Tools/ue_remote.py …`）… 組み上がったシーケンスの骨アニメの区間を秒で並べる（埋めた区間も出る）。
  - `z2_sweep.py` / `z2_inview.py` … PIE で捕まる場面を止めて複数の時刻へ送り、テラスのナースの `hand_l` の高さ（**基準姿勢 = +175、`Idle` = +132**）・アニメのインスタンス・画面の位置を出す。
  - `z2_at.py` + `__t = <秒>` を前に置いた台本 … 任意の時刻へ送る（撮る前に）。
  - `seq_gaps.py` … 原作の JSON から区間と空きを出す。**ただし書き出しの名前で引くので、同名の区間があるトラック（Zone 1 の `SkeletalMesh` 2 体）は混ざる**。確かなのは組み上がった方（`check_fills.py`）。
- PIE の撮り方: `python Tools/pie.py start` → `cmd "Wasami.Flow OnArriveCaptureCutscene"` → `python Tools/desktop.py shot --region 1826 176 2864 1234 --name <名前>.png`（エディタが前面のときは `click` を挟まない）。**PIE は終わったら必ず止める**（`pie.py stop`、`slomo 1` に戻す）。
- 撮るときの注意: 捕まる場面のカメラは動く。t = 0 付近はプレイヤー自身のカメラでテラスのナースが大写しになり（`z2cap-3.png` がそれ）、シネカメラに切り替わると 24 m ほど先の画面の 1/10 になる。

## 検証

- check_records: OK（20 件）
- C++ ビルド: 未実行（この項目は C++ を変えない）
- ステップ 2 の確認: `place` の戻り値の `idle_fills` が Zone 1 で 3・Zone 2 で 5（本家の空きの数と一致）。組み上がった区間を `check_fills.py` で見て、3 つの場面とも再生範囲が隙間なく埋まった。PIE で捕まる場面を 0.5〜18 s まで送り、テラスのナースが `hand_l` +132（`Idle`、`AnimSequencerInstance` 付き）になった（前は +175 の基準姿勢・インスタンス無し）。絵は `z2cap-fix-t8.png`・`z2cap-fix-3.png`
