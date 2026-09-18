---
title: 敵ワサミの気絶を「倒れる → 寝返り → 起き上がる」にする
status: 進行中
branch: feature/enemy-stun-knockdown
base: 6915229
started: 2026-09-18 12:30
updated: 2026-09-18 14:10
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 敵ワサミの気絶を「倒れる → 寝返り → 起き上がる」にする

## 依頼

2026-09-18 のユーザーの指示（note の記事「敵ワサミのモーション一覧」を見て）:
「・BeHit_FlyUp/Knock_Down: スタン特殊効果/スタンオーブを使用した際、このアニメーションを流すように。
・Pushupto_idle: スタン効果が切れた後の復帰」

同日の回答:
- 倒れる 2 本の使い分けは「ランダム」（Primal Fear でもスタンオーブでも、2 本のどちらかをランダムに）。今の気絶の `01a0a88f-…`（`Stun_Loop`・`Stun_Recover`）は使わなくなる。
- 倒れた後のあおむけ → `push_up_to_idle` のうつぶせのつなぎは「Claude が寝返りを作る」（取り込みで、床の上で体を長い軸まわりに半回転させてうつぶせへ移る約 0.8 s のつなぎを作る。仮で PIE で見て決める）。

## 計画

- [x] 1. 取り込み … 2026-09-18 完了。`dd_enemy.py` の `ROLES` が `Stun_FlyUp`・`Stun_KnockDown`（倒れる、始めの骨盤を `Idle_11` の位置へ）と `Stun_GetUp_FlyUp`・`Stun_GetUp_KnockDown`（`_stun_get_up`: 寝返り `_roll` + `push_up_to_idle`、終わりが待機の位置と向き）を作り、`/Game/Wasami/Enemy` はアニメ 18 本（古い 5 本は消した）。通しの見た目は `observations/tools/stun_sequence_blender.py`（連番 `observations/ours/enemy-stun/frames/`）。
- [x] 2. C++ … 2026-09-18 完了。`WasamiEnemyClip` を取り込みの 18 本の並びにし（`StunFlyUp`・`StunKnockDown`・`StunGetUpFlyUp`・`StunGetUpKnockDown`。`WasamiEnemyAnim::GetUpAfter`）、`FWasamiStunPlayback` を「倒れる（1 回・終わりで止まる）→ 起き上がり（`GetUpStart = max(倒れる長さ, 気絶の長さ − 起き上がりの長さ)`）」に、倒れる 2 本は `FWasamiEnemyAnimState::StunRandom`（`Init` の種。インスタンスは `FMath::Rand()`）で抽選。起き上がりに入った更新が `StunGetUpStarted` を立て、`UWasamiEnemyAnimInstance::MoveToGetUp` がアクタとコントローラーの向きを移す（差は `WasamiEnemyAnim::MeasureGetUpMove`、`NativeInitializeAnimation` で倒れる 2 本ぶん）。テスト 29 件すべて通過、PIE で確かめた（下の「検証」）。
- [ ] 3. 記録と note: 実装記録 07、`.claude/references/enemy-wasami-motions.md`、作業一覧の「決めたこと」と項目 10 の気絶の書き方、note のモーション一覧の記事（気絶の節）と進捗記事の Primal Fear の文と GIF 06 の撮り直し。main へマージ。

## 次にやること

ステップ 3: 実装記録 07（気絶の節・クリップの表・テスト。下の決定事項のうち移し替えと寝返りの理由を移す）と 01（`dd_enemy.py`・`dd.py`）を直して `python .claude/scripts/check_records.py --update` を通し、`.claude/references/enemy-wasami-motions.md`・作業一覧（「決めたこと」と項目 10 の気絶）・note のモーション一覧の記事（気絶の節）と進捗記事（Primal Fear の文と GIF 06 の撮り直し）を直して、main へマージする。

## 決定事項

- 2026-09-18: 測った向き（Blender、v3 の glb）— `BeHit_FlyUp`・`Knock_Down` の終わりはどちらもあおむけ（胸の正面の上向き成分 +1.00・+0.99）で頭は元の後ろ側、`push_up_to_idle` の始めはうつぶせ（−0.99）で頭は前側、終わりは前を向いて立つ（17° ずれ）。起き上がると敵は倒れる前と逆を向き、`Knock_Down` では約 0.67 m（メッシュの大きさで約 0.9 m）後ろに立つので、アクタを移す必要がある。
- 2026-09-18: 移すのは寝て止まっている間（倒れた最後の姿勢と起き上がりの最初の姿勢が同じ世界の姿勢になる瞬間）。気絶の終わり（タイマーの `EndStun`）で移すと、アニメの更新の後なので 1 コマずれる。アニメの更新（ゲームスレッド）の中で移せば、同じコマの姿勢の評価に間に合う。
- 2026-09-18: UE で測った移し替えの差（部品空間、`check_transfer.py` と同じ `pelvis` の `get_bone_pose_for_time`）: FlyUp はヨー −174.241°・移動 (1.44, 12.80, 0) cm、KnockDown はヨー 168.796°・(10.08, −51.85, 0) cm。傾きは 0（純粋なヨーと水平の移動）。`pelvis` が骨組みの根。起き上がりの終わりの骨盤は `Idle` の 0 s と同じ XY（高さは 0.8 cm 違う）。
- 2026-09-18: 寝返りは、手足が 35 % で腕立ての構えに寄る形が最も床に潜らず（関節の最低 −0.08 m）、両端を除いて約 5 cm 浮かせた（`ROLL_FLOOR_SLACK` 3 cm を超えた分）。寝返りの長さ 0.8 s と手足の寄せ方は仮だったが、ステップ 2 の PIE（4 fps のコマの一覧）で床への潜りも跳びも見えなかったので、このままにする（ステップ 3 で `dd_enemy.py` の `TODO(仮)` を外し、理由を実装記録 01 へ）。
- 2026-09-18: 壁際で倒れると、移し替え（最大で約 0.7 m）でカプセルが壁に掛かることがある。スイープせずに移し、キャラクターの移動が押し出すのに任せる（仮。`MoveToGetUp` の `TODO(仮)`。廊下の真ん中でしか見ていない）。
- 2026-09-18（ステップ 2）: 気絶が解けて混ぜて戻る間は気絶の時間を進めない（`Inputs.bStunned` のときだけ `Advance`）。寝ている間に気絶が解けても、起き上がりと移し替えは起きずに待機へ混ぜて戻る。気絶のクリップは 1 回だけ流すので `bLoop` は偽。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 作業ブランチ `feature/enemy-stun-knockdown`（main の 6915229 から）。
- エディタは開いたまま（セッション 1）。C++ はステップ 2 でビルド済み。PIE は止めた。
- PIE の収録（git の外）: `observations/ours/enemy-stun/pie-enemy-stun-seq.mkv`（21 s。敵 (60, −1150)・ヨー 0、プレイヤー (−60, −550)・ヨー −90・ピッチ −6。撮り始めの約 1 s 後に `WasamiPrimalPower.stun_enemies`）、コマの一覧 `pie-stun-seq-1fps.png`、人に見せるグリッド `pie-stun-grid.png`。毎フレームの記録 `Saved/enemy_probe/stun-seq.json` を読む道具 `observations/tools/enemy_stun_check.py`。GIF 06 の撮り直しはステップ 3。
- 未完了の記録がもう 1 件ある（`20260918-power-look-tuning.md`、main 上の作業。こちらを終えてから戻る）。

## 検証

- ステップ 1: `import_wasami_enemy` → アニメ 18 本（ログの値はエディタの外の前処理と一致）。Blender の通しの描画で、切り替えの骨盤のずれ 0.0000 m、寝返りの最も低い骨 −0.03 m（`lowerarm_l`）。コマの一覧で「倒れる → 寝る → 横向き → うつぶせ → 腕立て → 立つ」と読めた。
- ステップ 2: `editor_cycle.py` でビルド成功。Automation `Wasami.` 29 件すべて通過（`Anim.Stun`・`Anim.Clips`〈測った差: FlyUp −174.241°・(1.44, 12.80)、KnockDown 168.796°・(10.08, −51.85)、許容 0.1〉・`Actor.Stun`〈起き上がりに入るコマで骨盤の世界の位置と向きが変わらない〉）。PIE（Primal Fear の `stun_enemies`、`Stun_KnockDown` が出た）: 倒れる 54.06 s → 起き上がり 67.52 s（17.36 − 3.9 s 後）→ 待機 71.59 s。起き上がりに入るコマでアクタはヨー +168.80°・71.79 cm 動き、骨盤の 1 コマの動きは 0.00 cm（倒れる間の最大 10.12 cm、起き上がりの始めの 1 秒の最大 1.58 cm）。コマの一覧で「吹き飛ぶ → あおむけ → 寝返り → うつぶせ → 腕立て → 立つ」と読めた。
- check_records: 実装記録 07・01 はステップ 3 で直す（それまで `WasamiEnemyAnimInstance.*`・`WasamiEnemyTests.cpp`・`dd_enemy.py`・`dd.py` のハッシュが合わない）
