---
title: Zone 2 の場面の代用の見直し（テラスの棒立ち・注射器で殴られる場面）（作業一覧の項目 42）
status: 進行中
branch: main
base: 8d85cc7
started: 2026-09-22 15:15
updated: 2026-09-22 15:15
---

# Zone 2 の場面の代用の見直し（作業一覧の項目 42）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 42（大目標 4）。ゲームレビュアーの指摘 2 件:

- (a)「Zone2のボスワサミと敵ワサミの対峙場面で、テラスに本家にはいないワサミが両手を掲げている」
- (b)「本家はナースに注射器で殴打されプレイヤーが床に倒れる場面だが、おそらくナースのボーンアニメを引き継げていないのか、全く理由のわからない状態になっている」

完了の条件は項目 42 の 4 つ（(1) 隠れている間は隠れている、(2) 捕まる場面の代用を選び直す、(3) 独房と Zone 1 の出来事も同じ目で見直す、(4) 3 つの場面を撮って棒立ちが居ないことを確かめる）。

## 計画

- [ ] 1. **棒立ちのワサミの正体を突き止める** ← 次
  - PIE で Zone 2 を `Wasami.Checkpoint 7` から通し、救急車の到着 → 捕まる場面（26.23 s）→ 独房（74.07 s）を撮って、棒立ちのワサミがどのアクタ・いつ・どこに出るかを確かめる（`Tools/pie.py`・`Tools/desktop.py`。手順は 11 記録の「場面の通し（2026-09-20）」）。
  - 変更予定: この記録だけ（調べるだけ）。絵は `Intermediate/DesktopAgent/shots/`（git の外）。
- [ ] 2. **アニメの区間が無い間の姿勢を直す**（完了の条件 1）
  - 変更予定: `Source/wasami_deception/WasamiCutsceneNurse.h`・`.cpp`、`Content/Python/wasami_tools/pipeline/dd_sequence.py`（どちらで直すかはステップ 1 の絵で決める）、`/Game/DD/Levels/L_Hospital_Zone2`。
- [ ] 3. **捕まる場面の殴打の代用を選び直す**（完了の条件 2）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`（`NURSE_ANIMS`）、`.claude/references/enemy-wasami-motions.md` の「場面の代用」、`/Game/DD/Levels/L_Hospital_Zone2`。
- [ ] 4. **独房 `06_Hospital_Zone2_Cell` と Zone 1 の `06_Hospital_Zone1_06Event` を同じ目で見直す**（完了の条件 3）
  - 変更予定: 同上と `/Game/DD/Levels/L_Hospital_Zone1`。
- [ ] 5. **3 つの場面を撮って並べ、棒立ちが居ないことを確かめる**（完了の条件 4）。実装記録 07・11 と作業一覧を直し、記録を消す

## 次にやること

ステップ 1。PIE で Zone 2 を `Wasami.Checkpoint 7` から開き、捕まる場面（`06_Hospital_Zone2_Capture`）の **t = 0〜17.2 s** にテラスの `nurse_idle1_2`（代役）がどう見えるかを撮る。下の「決定事項」の見立て（基準姿勢で立っている）が当たっているかを絵で確かめ、外れていたらそこで見立てを立て直す。

## 決定事項

- 2026-09-22: **(a) の見立て**（計画を立てる前の調査。原作のデータで裏付け済み、絵はまだ）— **捕まる場面の `nurse_idle1_2` が、シーケンスの頭 17.2 秒のあいだアニメの区間に覆われず、骨組みの基準姿勢（腕を広げた `restpose`）で立っている**。根拠:
  - 場所は (−10910, −1010, 800)・ヨー −90（テラス。捕まる場面の起こし箱 `Trigger_Arrive_CaptureScene` (−11600, −1000, 965) のすぐ隣）。
  - 可視トラック（`06_Hospital_Zone2_Capture`）のキーは 16.8 s と 17.33 s の **2 つとも `bHidden = false`**、`DefaultValue` は `true`。`FMovieSceneBoolChannel` の `PreInfinityExtrap` の既定は `RCCE_Constant`（UE 5.8 の `MovieSceneBoolChannel.h`）なので**最初のキーより前も キー 0 の値**、つまり **t = 0 から見えている**。UE 4.24 も同じ（`FMath::Max(0, UpperBound-1)`）ので、**本家も t = 0 から見えている**。前処理の反転（`dd_sequence.boolean`）は正しい。
  - アニメのトラックは **17.2〜19.167 s の `ReaperNurse_Idle_Alert` と 19.233〜20.867 s の `Nurse_Hospital_Zone01_Event_39` の 2 本だけ**。その前 17.2 秒は何も流れない。
  - 本家の `nurse_idle1_2` の `SkeletalMeshComponent0` は **`AnimClass` を持たない**ので、本家でもその 17.2 秒は基準姿勢で立っている。違うのは**姿勢そのもの**で、ナースの基準姿勢は自然な直立、本作の敵ワサミ v3 の基準姿勢は**腕を広げた `restpose`**（`.claude/references/enemy-wasami-motions.md` の「（使わない）」）。指摘の「両手を掲げている」はこれに合う。
  - 独房の `nurse_idle2_2` は本家では **`AnimClass`（`nurse_idle1_Skeleton_AnimBlueprint_lookat_cutscene`）を持つ**ので、アニメの区間の切れ目でも待機の姿勢を保つ。本作の代役 `AWasamiCutsceneNurse` はアニメインスタンスを持たないので、切れ目（24.9〜27.767 s、64.833〜68.833 s など）で基準姿勢に戻るはず。
  - つまり**隠す・隠さないの移植の誤りではなく、代役のモデルの基準姿勢が本家のナースと違うことが原因**という見立て。直す向きは「区間の無い間に `Idle` を保たせる」か「アニメの始まりまで隠す」のどちらか（ステップ 2 で絵を見て決める）。
- 2026-09-22: **(b) の今の代用**（ステップ 3 で選び直す対象）— 待ち構え `ReaperNurse_Idle_Alert` → `Idle_Alert`（`Idle_5`、1.958 s）、殴る `Nurse_Hospital_Zone01_Event_39`（1.633 s の区間）→ `Chase_Charge`（`Male_Head_Down_Charge`、0.533 s・2.17 m 前進）。**頭を下げて突進する動き**なので「注射器で殴打する」演技には読めない。v3 の 18 本から選び直す（`.claude/references/enemy-wasami-motions.md`）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 2〜4 で前処理を回すときは `python Tools/dd/prepare_stage.py` →（エディタで）`dd_sequence` の組み立て直し。手順は 01 記録。
- PIE は終わったら必ず止める（`python Tools/pie.py stop`）。
- 「2 台」「置いていかれる」の項目 41 と「グリッド」の項目 39 は `RunUAT.bat` の許可待ちで `status: ユーザー待ち`。この項目は PIE で閉じられる見込みだが、完了の条件 4 の確かめはパッケージ版でも見られると良い（同じ許可の件）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
