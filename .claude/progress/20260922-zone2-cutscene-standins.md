---
title: Zone 2 の場面の代用の見直し（テラスの棒立ち・注射器で殴られる場面）（作業一覧の項目 42）
status: 進行中
branch: main
base: 8d85cc7
started: 2026-09-22 15:15
updated: 2026-09-22 15:40
---

# Zone 2 の場面の代用の見直し（作業一覧の項目 42）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 42（大目標 4）。ゲームレビュアーの指摘 2 件:

- (a)「Zone2のボスワサミと敵ワサミの対峙場面で、テラスに本家にはいないワサミが両手を掲げている」
- (b)「本家はナースに注射器で殴打されプレイヤーが床に倒れる場面だが、おそらくナースのボーンアニメを引き継げていないのか、全く理由のわからない状態になっている」

完了の条件は項目 42 の 4 つ（(1) 見えている間に棒立ち（基準姿勢）にならない〈2026-09-22 に「隠れている間は隠れている」から書き換え。下の決定事項〉、(2) 捕まる場面の代用を選び直す、(3) 独房と Zone 1 の出来事も同じ目で見直す、(4) 3 つの場面を撮って棒立ちが居ないことを確かめる）。

## 計画

- [x] 1. 棒立ちのワサミの正体を突き止める（PIE と原作のデータで確定。下の「決定事項」）
- [ ] 2. **アニメの切れ目を待機の動きで埋める**（完了の条件 1・3） ← 次
  - `dd_sequence.py` が場面を組み立てるときに、ナースの骨アニメのトラックの**空き区間**（下の表）へ `A_WasamiEnemy_Idle` の区間を足す（前後の区間と少し重ねて繋ぐ。原作自身の区間も 0.4 s ほど重ねてある）。C++ は変えない見込み。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`、（組み立て直しで）`/Game/Stage/Maps/L_Hospital_Zone2`・`L_Hospital_Zone1` とその下の場面のシーケンス資産。
- [ ] 3. **捕まる場面の殴打の代用を選び直す**（完了の条件 2）
  - 対象は `Nurse_Hospital_Zone01_Event_39`（17.2〜20.87 s の 2 本目、1.633 s）。今は `Chase_Charge`（頭を下げた突進）。v3 の 18 本から選び直す。**カメラはこのとき引いた絵**（テラスのワサミは画面の 1/10 ほど。`z2cap-t19.5.png`）なので、読めるかは絵で見る。
  - 変更予定: `dd_sequence.py` の `NURSE_ANIMS`、`.claude/references/enemy-wasami-motions.md` の「場面の代用」、組み立て直し。
- [ ] 4. **Zone 1 の `06_Hospital_Zone1_06Event` を同じ目で見直す**（完了の条件 3）
  - `BP_06_ReaperNurse_06Special`・`2` は**骨アニメのトラックを持たない**（下の表）。本作でこの 2 体が何として置かれ、何を再生しているかを PIE で見る（代役 `AWasamiCutsceneNurse` ではなく組み立てが置く敵のはず）。
- [ ] 5. **3 つの場面を撮って並べ、棒立ちが居ないことを確かめる**（完了の条件 4）。実装記録 07・11 と作業一覧を直し、記録を消す

## 次にやること

ステップ 2。`dd_sequence.py` の骨アニメのトラックを組む所で、**原作の区間で覆われていない所に `Idle` の区間を足す**。埋める区間（原作のデータから。下の表）:

| 場面 | 結び | 空き（アニメが無い） |
| --- | --- | --- |
| `06_Hospital_Zone2_Capture`（0〜26.23 s） | `nurse_idle1` | **0〜17.20 s**、19.17〜19.23 s、20.87〜26.23 s |
| `06_Hospital_Zone2_Cell`（0〜74.07 s） | `nurse_idle2` | 24.90〜27.77 s、64.83〜68.83 s、73.83〜74.07 s |
| `06_Hospital_Zone1_06Event`（0〜10.53 s） | `SkeletalMesh`（2 体） | 2.67〜5.93 s / 2.67〜3.77 s、10.27〜10.53 s |
| 同上 | `BP_06_ReaperNurse_06Special`・`2` | **全区間**（骨アニメのトラックが無い。ステップ 4 で本作の置き方を見る） |

## 決定事項

- 2026-09-22: **(a) の正体が確定した** — 捕まる場面 `06_Hospital_Zone2_Capture` のテラスの代役 `nurse_idle1_2` が、**頭の 17.2 秒のあいだアニメの区間に覆われず、敵ワサミ v3 の基準姿勢（腕を左右に広げた `restpose`）で、シネカメラの真正面に立っている**。PIE（`Wasami.Flow OnArriveCaptureCutscene`）で測った値:
  - t = 0.17 s: `hidden=False`・`anim_instance=None`・`hand_l` が根から **+175 cm**（腕を広げた姿勢）。絵 `Intermediate/DesktopAgent/shots/z2cap-3.png`（画面の真ん中を占める）。
  - t = 16.9 s まで +175 のまま → t = 17.5 s で `AnimSequencerInstance` が付いて +132 → 最後の区間の終わり（20.87 s）の後も **+123 のまま止まる**（基準姿勢へは戻らない）。つまり**棒立ちが出るのは最初の区間より前だけ**。
  - 見張り 6 体（`WasamiEnemySentry`）は `hand_l` +14〜15 cm で自前の待機を回しており、問題は無い。Matron も自前の `WasamiBossAnimInstance`。
- 2026-09-22: **本家でもこのナースは t = 0 から見えている**（＝可視の移植は正しく、直すのは姿勢）。根拠を 2 つ重ねた:
  - 可視のトラックのキー 2 つはどちらも `bHidden = false`、`FMovieSceneBoolChannel` の `PreInfinityExtrap` の既定 `RCCE_Constant` で最初のキーより前もキー 0 の値。
  - その区間（`MovieSceneBoolSection_0`）の書き出しに **`SectionRange` が無い**＝クラスの既定と同じで、`UMovieSceneBoolSection` の構築子は `SetRange(TRange<FFrameNumber>::All())`（UE 5.8 のソース）＝**無限の区間**。病院の場面 26 本の bool の区間はすべて `SectionRange` を持たず、transform・audio・骨アニメの区間はすべて持つので、書き出しの取りこぼしではない。
  - よって**完了の条件 (1) の前提（本家では隠してある）は外れ**。作業一覧の条件 (1) を「見えている間に棒立ちにならない」へ書き換えた。
- 2026-09-22: **直し方は「アニメの切れ目を待機で埋める」**（ステップ 2）。本家のナースは `AnimClass` を持たない代わりに**基準姿勢が自然な直立**なので、切れ目でも「立っているナース」に見える。本作の代役の基準姿勢は腕を広げた `restpose` なのでそのままでは使えない。隠してしまうと本家に無い「居ないナース」になるので、**待機の動き `Idle` で埋める**のが本家の見え方に一番近い。独房の切れ目（本家は `…AnimBlueprint_lookat_cutscene` が待機を回す）も同じ埋め方で合う。
- 2026-09-22: **(b) の今の代用**（ステップ 3 で選び直す対象）— 待ち構え `ReaperNurse_Idle_Alert` → `Idle_Alert`（`Idle_5`）、殴る `Nurse_Hospital_Zone01_Event_39`（19.23〜20.87 s）→ `Chase_Charge`（`Male_Head_Down_Charge`、0.533 s・2.17 m 前進）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- レベルの資産のパスは **`/Game/Stage/Maps/L_Hospital_Zone1`・`L_Hospital_Zone2`**（`/Game/DD/Levels/…` ではない）。
- 調べ直すときの道具（git の外。`Intermediate/Scratch/`）:
  - `seq_gaps.py`（`python Intermediate/Scratch/seq_gaps.py 06_Hospital_Zone2_Capture …`）… 原作の JSON から結び・区間・可視のキー・**アニメの空き区間**を出す。
  - `z2_poses.py`（`python Tools/ue_remote.py …`）… PIE のワサミの骨を持つアクタを `hand_l` の高さ順に並べる（+175 なら基準姿勢、+14 なら腕を下ろした待機）。
  - `z2_scrub.py` … 捕まる場面を止めて任意の時刻へ送る（`T = …` を書き換えて実行）。
- PIE の撮り方: `python Tools/pie.py start` → デスクトップのエージェント（`python Tools/desktop.py start`）→ **駆動役のターミナルが前面のときは `python Tools/desktop.py click 2957 95 --allow WindowsTerminal.exe`** でエディタを前面にし、以後は `--allow UnrealEditor.exe`。ビューポートだけの枠は `shot --region 1826 176 2864 1234`（`--name` には拡張子が要る）。メッセージログの小窓がビューポートに重なるので先に閉じる。**PIE は終わったら必ず止める**。
- 前処理の組み立て直しは `python Tools/dd/prepare_stage.py` →（エディタで）`dd_sequence`。手順は 01 記録。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目は C++ を変えない見込み）
- エディタでの確認: 捕まる場面を PIE で通し、棒立ちを絵と骨の高さで確かめた（ステップ 1）
