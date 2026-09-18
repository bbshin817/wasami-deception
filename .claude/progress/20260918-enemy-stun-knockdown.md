---
title: 敵ワサミの気絶を「倒れる → 寝返り → 起き上がる」にする
status: 進行中
branch: feature/enemy-stun-knockdown
base: 6915229
started: 2026-09-18 12:30
updated: 2026-09-18 14:25
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
- [x] 3a. 記録 … 2026-09-18 完了。`dd_enemy.py` の寝返りの `TODO(仮)` を外し（0.8 s と手足の寄せ方は PIE で潜りも跳びも無く確定）、実装記録 07・索引、`.claude/references/enemy-wasami-motions.md`、作業一覧（「決めたこと」・項目 4・10）、`observations/README.md`（ステップ 2 の PIE の値）を新しい気絶に合わせた。
- [x] 3b1. note のモーション一覧の記事 … 2026-09-18 完了。気絶の節を倒れる 2 本（ランダム）→ 寝返り + `push_up_to_idle` にし、ゲームと同じつなぎの GIF `19-stun-flyup`・`20-stun-knock-down`（`stun_sequence_blender.py` → `motion_gifs_encode.py`。描き方はガイド）を足し、`01a0a88f-…` は「使わない動き」へ。note へ反映（published）。
- [ ] 3b2. note の進捗記事（Primal Fear の文と GIF 06 の撮り直し）。note へ反映。記録を消して main へマージし、作業ブランチを消して push。

## 次にやること

ステップ 3b2: `.claude/guides/note-progress.md`（進捗記事の節と「GIF」）に従い、(1) `docs/note/progress.md` の Primal Fear の文（「ふらついて動けなくなります」）を「倒れて、17 秒の終わりに起き上がる」に直し、GIF `observations/ours/note/gif/06-primal-fear.gif` を撮り直す（ガイドの 06 の撮り方。倒れるところと、できれば起き上がりまで。収録 `observations/ours/enemy-stun/pie-enemy-stun-seq.mkv` が使えるかも先に見る）。(2) `tmp/note-cli` で進捗記事を書き換える。(3) 記録を消して main へマージし、`handover.md` の「現状と次の一歩」を確かめ、作業ブランチを消して push。

## 決定事項

（残りのステップに効くものなし。気絶の作りと理由は実装記録 07、PIE の値は `observations/README.md` の「敵ワサミ」）

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 作業ブランチ `feature/enemy-stun-knockdown`（main の 6915229 から）。
- エディタは開いたまま（セッション 1）。C++ はステップ 2 でビルド済み。PIE は止めた。
- PIE の収録（git の外）: `observations/ours/enemy-stun/pie-enemy-stun-seq.mkv`（21 s。敵 (60, −1150)・ヨー 0、プレイヤー (−60, −550)・ヨー −90・ピッチ −6。撮り始めの約 1 s 後に `WasamiPrimalPower.stun_enemies`）、コマの一覧 `pie-stun-seq-1fps.png`、人に見せる時刻入りのグリッド `sheet-1-fall.png`・`sheet-2-getup.png`（`video_probe.py sheet`）。毎フレームの記録 `Saved/enemy_probe/stun-seq.json` を読む道具 `observations/tools/enemy_stun_check.py`。GIF 06 の撮り直しはステップ 3b。
- 未完了の記録がもう 1 件ある（`20260918-power-look-tuning.md`、main 上の作業。こちらを終えてから戻る）。

## 検証

- ステップ 1・2: 取り込み 18 本、テスト `Wasami.` 29 件すべて通過、PIE の値は `observations/README.md` の「敵ワサミ」の気絶の行（替えた後）。
- ステップ 3a: `check_records.py --update`（01・07 のハッシュを更新、7 件すべて同期）。ソースの変更はコメントだけで、取り込み直しは要らない。
