---
title: 追跡中のランダムの動き（作業一覧の項目 26）
status: 進行中
branch: main
base: bdc2ea6
started: 2026-09-20 09:32
updated: 2026-09-20 09:55
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 追跡中のランダムの動き（作業一覧の項目 26）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 26（大目標 2）。2026-09-18 のユーザーの指示: 「`enemy_wasami_v3` の追いかける動き（片手をついて飛び越える・突進・スライディングなど 6 本）を、Zone 1・2 の追跡中にたまにランダムで流す」「頻度はふつう（約 8 秒に 1 回）」「流す間も本家の追跡の速さを保つ」。

完了の条件（作業一覧）: 追跡中は約 8 秒に 1 回、v3 の追いかける動き 6 本からランダムに流す（前方が空いているときだけ、速さは追跡の速さのまま）。頻度と早回しの上限はテストにする。PIE で追跡中に流れることを収録する。

根拠: `.claude/references/enemy-wasami-motions.md` の「追跡中のランダムの動き（項目 7）」（6 本の長さ・前進・動きの速さの表）、実装記録 07。

## 計画

- [x] 1. `AWasamiEnemy` に追跡中のランダムの動きを足し、単体テストとビルドまで（`FWasamiChaseVariations`・`UpdateChaseVariation`・`IsWayAheadClear`・`ChaseVariationRate`。テスト 3 件を足して `Automation RunTests Wasami` 148 件すべて成功。実装記録 07 に書いた）
- [ ] 2. PIE で追跡中に流れることを確かめて収録し、記録を直して項目 26 を閉じる ← 次
  - 変更予定: `.claude/roadmap.md`（項目 26 の完了の条件の「800 cm/s」を 430 に直して完了にする）、`.claude/references/enemy-wasami-motions.md`（仮を確定に）、`.claude/implementation-records/07-enemies.md`（PIE で分かったこと）、`.claude/references/handover.md`、`docs/note/progress.md` と note の記事
  - 中身: `Tools/pie.py` で Zone 1 に敵を出して追われ、6 本が流れるところを収録（Discord の連番のグリッド）。**見るもの**: 約 8 s に 1 回流れるか、走りへの戻りが自然か、壁へのめり込みが無いか（前方の空きの判定が効いているか）、足の滑り（とくに上限に当たる `Chase_VaultLand`）。

## 次にやること

ステップ 2。`python Tools/pie.py start` → Zone 1 のチェックポイント 5（迷路）でプレイヤーの前に `AWasamiEnemy::SpawnEnemy` で敵を出して追わせ、`GetChaseVariations().GetLast()` と `UWasamiEnemyAnimInstance::GetMainClip` を見ながら収録する。

## 決定事項

- 2026-09-20: 追跡の速さは今の設定のまま（`AWasamiEnemy::MaxSpeed` 430 cm/s） — 指示の「本家の追跡の速さ 800 cm/s」は本家のナースの値で、2026-09-19 のユーザーの指示（敵が速すぎる）で本作は 430 にしてある。動きを流す間も速さを変えないという趣旨を守る。作業一覧の項目 26 の完了の条件の「800 cm/s」もステップ 2 で直す。
- 2026-09-20: 早回しの上限・下限は走りと同じ 0.6〜1.8（`WasamiEnemyAnim::RunRateMin/Max`。WebGL 版 15 記録の範囲）にした — 新しい値を作らずに済み、指示の「早回しの上限は仮」に収まる。`Chase_VaultLand` だけは 6.1 倍要るので上限に当たり、足りない分は体が滑る（ステップ 2 の PIE で見る）。
- 2026-09-20: 前方が塞がっている機会は**開いたまま**にし、次の判断（0.5 s 後）で引き直す — 捨てて次の間隔（6〜10 s）を引くと、廊下や角で頻度が指示の「約 8 秒に 1 回」より落ちるため。1 回再生が走っている・扉を突いている・クリップが無い機会は捨てる（しばらく続く状態なので、開いたままだと解けた瞬間に必ず流れてしまう）。

## 要確認（ユーザー）

- 早回しの上限を走りと同じ 1.8 にしたので、`Chase_VaultLand`（動きの速さ 52 cm/s）は追跡の速さ 430 に足りず、流している 1.33 s の間に体が滑って進みます（ほかの 5 本は上限の中に収まります）。滑りが気になるなら、この 1 本だけ外すか、上限を上げる（例: 3.0）かを選べます。

## 再開時の注意

- ステップ 2 は PIE。テストを走らせた直後は**エディタが前面**でないと 3 fps のままなので（`Automation` は 10 fps を待つ）、`Tools/desktop.py click <エディタの題名の帯> --allow …` で前面にしてから始める（2026-09-20: 実座標 (2900, 80) がエディタの題名の帯。(2800, 28) はデスクトップに当たって explorer が前面になった）。
- 追跡中の様子を読む口: `AWasamiEnemy::GetChaseVariations()`（`GetWait()`・`GetLast()`）、`UWasamiEnemyAnimInstance::GetMainClip(時刻, 重み)`、`AWasamiEnemy::IsWayAheadClear(距離)`。

## 検証

- check_records: ステップ 1 で実行（`--update` 済み）
- C++ ビルド: ok（`python Tools/editor_cycle.py`、2026-09-20）
- テスト: `Automation RunTests Wasami` 148 件すべて成功（新しい 3 件 `Wasami.Enemy.Anim.ChaseVariations`・`Wasami.Enemy.Chase.Chances`・`Wasami.Enemy.Actor.ChaseVariation` を含む）
- エディタでの確認（PIE の収録）: 未実行（ステップ 2）
