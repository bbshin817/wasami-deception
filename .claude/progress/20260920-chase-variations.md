---
title: 追跡中のランダムの動き（作業一覧の項目 26）
status: 進行中
branch: main
base: bdc2ea6
started: 2026-09-20 09:32
updated: 2026-09-20 09:32
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 追跡中のランダムの動き（作業一覧の項目 26）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 26（大目標 2）。2026-09-18 のユーザーの指示: 「`enemy_wasami_v3` の追いかける動き（片手をついて飛び越える・突進・スライディングなど 6 本）を、Zone 1・2 の追跡中にたまにランダムで流す」「頻度はふつう（約 8 秒に 1 回）」「流す間も本家の追跡の速さを保つ」。

完了の条件（作業一覧）: 追跡中は約 8 秒に 1 回、v3 の追いかける動き 6 本からランダムに流す（前方が空いているときだけ、速さは追跡の速さのまま）。頻度と早回しの上限はテストにする。PIE で追跡中に流れることを収録する。

根拠: `.claude/references/enemy-wasami-motions.md` の「追跡中のランダムの動き（項目 7）」（6 本の長さ・前進・動きの速さの表）、実装記録 07。

## 計画

- [ ] 1. `AWasamiEnemy` に追跡中のランダムの動きを足し、単体テストとビルドまで ← 作業中
  - 変更予定: `Source/wasami_deception/WasamiEnemy.h`・`WasamiEnemy.cpp`、`Source/wasami_deception/Tests/WasamiEnemyTests.cpp`
  - 中身: 追跡（`bSeenPlayerRecently`）に入ったら 6〜10 s の乱数で次の機会を決め、0.5 s ごとの `MakeChoice` で機会が来たら 6 本（`WasamiEnemyClip::ChasePickUp`〜`ChaseSlide`）から直前と違う 1 本を抽選して `GetEnemyAnim()->PlayOnce` で流す。気絶・すでに 1 回再生中・扉を突いている（`AWasamiEnemy06Chase::bAttackDoor`）ときは流さず次の機会へ。前方が空いていなければ流さない。移動は止めない（`PlayOnce` はアニメだけ）。
  - テスト: 抽選（直前と同じを避ける・6 本が出る）、間隔（6〜10 s）、早回しの上限。
- [ ] 2. PIE で追跡中に流れることを確かめて収録し、記録を直して項目 26 を閉じる
  - 変更予定: `.claude/implementation-records/07-enemies.md`、`.claude/roadmap.md`、`.claude/references/enemy-wasami-motions.md`（仮を確定に）、`docs/note/progress.md` と note の記事
  - 中身: `Tools/pie.py` で Zone 1 に敵を出して追われ、6 本が流れるところを収録（Discord の連番のグリッド）。壁へのめり込みが無いことも見る。

## 次にやること

ステップ 1。`Source/wasami_deception/WasamiEnemy.h/.cpp` に下の「決定事項」の仕組みを書き、`Tests/WasamiEnemyTests.cpp` に抽選・間隔・早回しの上限のテストを足して `python Tools/editor_cycle.py` でビルドする。

## 決定事項

- 2026-09-20: 追跡の速さは今の設定のまま（`AWasamiEnemy::MaxSpeed` 430 cm/s） — 指示の「本家の追跡の速さ 800 cm/s」は本家のナースの値で、2026-09-19 のユーザーの指示（敵が速すぎる）で本作は 430 にしてある。動きを流す間も速さを変えないという趣旨を守る。
- 2026-09-20: 流す間の移動は `PlayOnce`（全身の 1 回再生）だけで、AI の移動は止めない — 6 本は取り込みで前進を消した「その場の形」（`dd_enemy.py` の `in_place`）なので、体は追跡の速さで進み続ける。
- 2026-09-20: 前方の空きは移動する距離（追跡の速さ × 流す秒数）を NavMesh のレイで見る — 動きの前進は取り込みで消してあるので、実際に進むのはこの距離。
- 2026-09-20: 早回しは「動きの速さ（`enemy-wasami-motions.md` の表）× `MeshScale`」を追跡の速さに合わせる比で、上限・下限で挟む（値はステップ 1 で決めて要確認に書く） — 足の滑りを減らすため。足りない分は体が滑る（指示の「早回しの上限は仮」）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ステップ 1 は C++ のビルドが要る: `python Tools/editor_cycle.py`（エディタを閉じて `Development Editor Win64` をビルドし開き直す）。走らせる前にエディタの未保存を確かめる。
- 敵の 1 回再生の口は `UWasamiEnemyAnimInstance::PlayOnce(FName Clip, PlayRate, BlendIn, BlendOut)`（長さを返す）。同じ口を `AWasamiEnemy06Chase::StartDoorAttack` が使っている（`Chase_Charge`）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
