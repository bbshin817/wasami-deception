---
title: 捕獲の別室を原作の Matinee とアセットの値どおりにする（作業一覧の項目 32）
status: 進行中
branch: main
base: 2399e97
started: 2026-09-21 05:29
updated: 2026-09-21 05:29
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 捕獲の別室を原作の Matinee とアセットの値どおりにする（作業一覧の項目 32）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 3 の項目 32。大目標 2 の項目 9・24 で本家ホテル・館の Matinee のキーから組んだ捕獲の別室（カメラの寄り・灯・場面の時間・暗転）を、**原作の Matinee とアセットの値どおりにする**。

完了の条件（2026-09-21 にコード優先へ書き換え済み）:
- 原作の Matinee 6 本（`MonkeyJumpscare`・`2`・`3` と `03_Watcher_Kill*`）のキー（カメラの位置・FOV・DOF・揺れ・フェード）と灯のアクターの値を読み、本作の実装と 1 つずつ突き合わせて差を埋める。
- 写していない `JumpscareCam` の旧 DOF のトラック（焦点 142.9 → 10・領域 571.4 → 100）と、直線にした暗転（本家は曲線の `StartCameraFade`）を入れるかどうかも**キーから決める**。
- **コードで決まらなかったものだけ**、旧版の実機を 1 回だけ撮って見比べる。
- 本作の体（ワサミのモデルと骨）では写せない差は、理由を書いて閉じてよい。

大目標 3 の節の頭の決まり: **原作のコード・アセットで先に決める**（本家の実機は中の移動が遅すぎて 1 回の収録で 1 か所しか回れない）。

## 計画

- [ ] 1. 原作の 6 本と関係アクタの値を全部書き出し、本作の実装との**突き合わせ表**を作る ← 作業中
  - 読む: `pak_reference/_levels/01_Hotel.full.json`（`MonkeyJumpscare`・`2`・`3` の `InterpData_0` 配下の全トラック =
    `InterpTrackMove_*`・`InterpTrackFloatProp_*`〈FOV / DOF 2 本〉・`InterpTrackFade_0`・`InterpTrackDirector_0`・`InterpTrackAnimControl_*`・`InterpTrackSound_*`、
    `MatineeActor` 自身の `InterpPosition`・`PlayRate`、`JumpscareCam`（`CameraActor` + `CameraComponent`: FOV・DOF の既定）、`JumpscareMonkey`、`JumpscareOffset`、天井灯 `ceilinglights_80`）
  - 読む: `pak_reference/_camera/03_Watcher_Kill3.json`・`.csv` と `_camera_shakes.json` の `JumpscareShake`、`pak_reference/_assets/…/BP_03_Watcher`（`_bytecode` @5222〜）
  - 読む: 本作 `Source/wasami_deception/WasamiCapture.h`・`.cpp` の表と定数
  - 出す: 差の一覧を**この記録の「突き合わせ表」**に 1 行 1 項目で書く（項目 / 原作の値 / 本作の値 / 差 / 埋める・閉じるの別）。ここで残りのステップを立て直す。
  - 変更予定: `.claude/progress/20260921-capture-room-fidelity.md` だけ（読むだけのステップ）
- [ ] 2〜n. ステップ 1 の突き合わせ表から立てる（DOF のトラック、暗転の曲線、揺れ、FOV、灯、カメラのキーの評価 … 差のあるものを 1 ステップ 1〜2 件で埋める）
- [ ] 最後. PIE で 4 本を通して確かめ、実装記録 07 と作業一覧の項目 32 を締める（`check_records.py --update`）

## 次にやること

ステップ 1。上の「読む」の 3 群を読んで、突き合わせ表をこの記録に書き、残りのステップを立て直してコミットする。
`pak_reference/_levels/01_Hotel.full.json` は 4.3 MB・7186 件の配列で、`o['path']` に `01_Hotel.PersistentLevel.MonkeyJumpscare…` を含むものを拾えば全トラックが取れる（`o['class']` に型、`o['props']` に値）。

## 決定事項

- 2026-09-21: 本家の実機の収録は**最後の手段**にする — 大目標 3 の節の頭（2026-09-21 のユーザーの回答）。原作のキーとアセットの値で決められるものは全部コードで決め、決まらなかったものだけを 1 回の収録にまとめる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理は無い（ステップ 1 は読むだけ）。エディタは触らない。
- 本作の捕獲の実装は `Source/wasami_deception/WasamiCapture.h`・`.cpp`、説明は実装記録 07 の「捕獲の演出」。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
