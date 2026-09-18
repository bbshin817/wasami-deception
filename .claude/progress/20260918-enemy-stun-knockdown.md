---
title: 敵ワサミの気絶を「倒れる → 寝返り → 起き上がる」にする
status: 進行中
branch: feature/enemy-stun-knockdown
base: 6915229
started: 2026-09-18 12:30
updated: 2026-09-18 12:30
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
- [ ] 2. C++（`WasamiEnemyAnimInstance`・`WasamiEnemy`・テスト）← 次
  - クリップ: `WasamiEnemyClip` の `StunLoop`・`StunRecover`・`BeHitFlyUp`・`KnockDown`・`PushUpToIdle` をやめ、`StunFlyUp`・`StunKnockDown`・`StunGetUpFlyUp`・`StunGetUpKnockDown` を `RunNightmare` の後に置く（名前は `Stun_FlyUp` ほか。`ClipNames`・コメントの `dd_enemy.ROLES` と同じ順）。取り込んだ長さ（30 fps のコマ数）: Idle 58・IdleAlert 57・Walk 31・Run 20・RunNightmare 18・StunFlyUp 46・StunKnockDown 75・StunGetUpFlyUp 117・StunGetUpKnockDown 117・Capture 64・83・106・Chase 37・16・63・72・55・53（テストの `ImportedLengths`）。
  - 気絶の再生 `FWasamiStunPlayback`: 始まりで倒れる 2 本からランダム（`FWasamiEnemyAnimState` に `FRandomStream`。インスタンスが `FMath::Rand()` で種をまき、テストは種を決める）→ 倒れる（1 回）→ 最後の姿勢で止まる → 起き上がりの始め `GetUpStart = max(倒れる長さ, 気絶の長さ − 起き上がりの長さ)` から起き上がりを流し、気絶の終わりで終える。気絶が解けて混ぜて戻る間は時間を進めない（その間に移し替えが起きないように）。
  - 移し替え: 起き上がりに入った更新で状態が知らせ、`UWasamiEnemyAnimInstance::NativeUpdateAnimation`（ゲームスレッド。同じコマの姿勢の評価の前）がアクタを動かす。差は `NativeInitializeAnimation` で倒れる 2 本ごとに求める: 骨盤（骨組みの根、インデックス 0）の部品空間の変形を `UAnimSequence::GetBoneTransform`（`OutAtom` は骨組みの基準姿勢で初期化する。トラックが無いと書かれない）で、倒れる終わりと起き上がりの 0 s から取り、`Delta = GetUp0.Inverse() * FallEnd` を縦軸の回転と XY だけに直す。アクタの新しい変形 = `Rel.Inverse() * Delta * Rel * ActorOld`（`Rel` はメッシュの相対変形。拡縮 1.3591 込み）。`SetActorLocationAndRotation`（`ETeleportType::TeleportPhysics`）し、コントローラーの `SetControlRotation` も新しい向きへ（`bUseControllerDesiredRotation` で戻されないように）。
  - テスト `Wasami.Enemy.Anim.Stun`（倒れる → 止まる → 移し替え 1 回 → 起き上がりが気絶の終わりで終わる、短い気絶、混ぜて戻る間は進まない、再気絶）・`Wasami.Enemy.Actor.Stun`（読み込んだ長さ、移し替えでアクタが回って動く）・`Anim.Clips`（長さ）を直す。
  - `Tools/editor_cycle.py` でビルド、PIE で Primal Fear を当てて収録（`observations/tools/note_gif.py` の敵の置き方）。
- [ ] 3. 記録と note: 実装記録 07、`.claude/references/enemy-wasami-motions.md`、作業一覧の「決めたこと」と項目 10 の気絶の書き方、note のモーション一覧の記事（気絶の節）と進捗記事の Primal Fear の文と GIF 06 の撮り直し。main へマージ。

## 次にやること

ステップ 2: `WasamiEnemyAnimInstance.h/.cpp` のクリップの並びと気絶の再生・移し替えを上の計画どおりに書き、テストを直して `python Tools/editor_cycle.py` でビルドする（今のエディタの C++ は古いクリップ名を探すので、PIE では気絶のクリップが無い警告が出る）。

## 決定事項

- 2026-09-18: 測った向き（Blender、v3 の glb）— `BeHit_FlyUp`・`Knock_Down` の終わりはどちらもあおむけ（胸の正面の上向き成分 +1.00・+0.99）で頭は元の後ろ側、`push_up_to_idle` の始めはうつぶせ（−0.99）で頭は前側、終わりは前を向いて立つ（17° ずれ）。起き上がると敵は倒れる前と逆を向き、`Knock_Down` では約 0.67 m（メッシュの大きさで約 0.9 m）後ろに立つので、アクタを移す必要がある。
- 2026-09-18: 移すのは寝て止まっている間（倒れた最後の姿勢と起き上がりの最初の姿勢が同じ世界の姿勢になる瞬間）。気絶の終わり（タイマーの `EndStun`）で移すと、アニメの更新の後なので 1 コマずれる。アニメの更新（ゲームスレッド）の中で移せば、同じコマの姿勢の評価に間に合う。
- 2026-09-18: UE で測った移し替えの差（部品空間、`check_transfer.py` と同じ `pelvis` の `get_bone_pose_for_time`）: FlyUp はヨー −174.241°・移動 (1.44, 12.80, 0) cm、KnockDown はヨー 168.796°・(10.08, −51.85, 0) cm。傾きは 0（純粋なヨーと水平の移動）。`pelvis` が骨組みの根。起き上がりの終わりの骨盤は `Idle` の 0 s と同じ XY（高さは 0.8 cm 違う）。
- 2026-09-18: 寝返りは、手足が 35 % で腕立ての構えに寄る形が最も床に潜らず（関節の最低 −0.08 m）、両端を除いて約 5 cm 浮かせた（`ROLL_FLOOR_SLACK` 3 cm を超えた分）。寝返りの長さ 0.8 s と手足の寄せ方は仮（PIE で見て決める。`dd_enemy.py` の `TODO(仮)`）。
- 2026-09-18: 壁際で倒れると、移し替え（最大で約 0.7 m）でカプセルが壁に掛かることがある。スイープせずに移し、キャラクターの移動が押し出すのに任せる（仮。PIE で見る）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 作業ブランチ `feature/enemy-stun-knockdown`（main の 6915229 から）。
- エディタは開いたまま（セッション 1）。`/Game/Wasami/Enemy` は取り込み直して保存済み。エディタの C++ はまだ古い（ステップ 2 のビルドまで、気絶のクリップが見つからない警告が出る）。
- 未完了の記録がもう 1 件ある（`20260918-power-look-tuning.md`、main 上の作業。こちらを終えてから戻る）。

## 検証

- ステップ 1: `import_wasami_enemy` → アニメ 18 本（ログの値はエディタの外の前処理と一致）。Blender の通しの描画で、切り替えの骨盤のずれ 0.0000 m、寝返りの最も低い骨 −0.03 m（`lowerarm_l`）。コマの一覧で「倒れる → 寝る → 横向き → うつぶせ → 腕立て → 立つ」と読めた。
- check_records: 実装記録 07・01 はステップ 3 で直す（それまで `dd_enemy.py`・`dd.py` のハッシュが合わない）
- C++ ビルド: 未実行
