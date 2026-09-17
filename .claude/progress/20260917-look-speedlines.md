---
title: マウスの視点移動の速さとスピードブーストの集中線の修正（作業一覧の項目 2）
status: 進行中
branch: feature/look-and-speedlines
base: 7208432
started: 2026-09-17 20:54
updated: 2026-09-17 20:54
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# マウスの視点移動の速さとスピードブーストの集中線の修正

## 依頼

作業一覧 `.claude/roadmap.md` の項目 2。ユーザーの原文（2026-09-17 の最終目標の「既存の修正点」）:

```
- マウスによる視点移動が遅すぎます
- スピードブースト使用時、集中線がおかしなノイズのように現れます
```

- 目標: 視点移動の速さを本家と同じにし、集中線を本家と同じ絵にする。
- 完了の条件: (1) 同じマウスの移動量に対する回転角が最新版の実機と一致する（`Tools/desktop.py look` で同じ量を送って測る）。(2) ブースト中の本作の画面（`observations/ours/`）で、集中線が本家の実機の収録と同じ形（放射状の線のコマ送り）に見える。
- 大規模改修として作業ブランチ `feature/look-and-speedlines` で進める（プレイヤーの C++ と取り込みの Python にまたがり、複数のコミットに分ける）。

## 計画

- [ ] 1. PIE で本作の今の値を測り、原因を切り分ける（コードは変えない）
  - 視点: PIE の実行中の値を読む（`python Tools/pie.py cmd "getall PlayerController InputYawScale_DEPRECATED"`・`InputPitchScale_DEPRECATED`、カメラの FOV）。Zone 1 の開始地点で `look --dx N`（N = 100・300・1000 など、ほかの入力なし）を送り、`pie.py state` のヨーの差から「1 カウントあたりの度」を出す。縦（`--dy`）も同じ。式の期待値（本家: カウント × 0.07 × FOV × 0.01111 × 2.5 × 感度 1.0。FOV 90 なら 0.175°/カウント）と比べ、どの係数が違うかを決める。PIE の窓が前面で視点が捕まっていること（`look` が効くこと）を先に確かめる。
  - 集中線: PIE で Speed Boost を使って走り、ブースト中の画面を数枚撮る（`observations/ours/pie-speedlines-*.png`）。今の `M_Speedlines`（2 × 2 で読む）が 1 コマに何段ぶんを映しているか、線がノイズに見える原因（コマの読み方か、ミップの無い 3841 × 5404 の縮小のちらつきか）を画から判断する。
  - 変更予定: `.claude/progress/20260917-look-speedlines.md`、`observations/README.md`（測った値）。
- [ ] 2. 本家の実機（最新版）で同じものを撮る（1 回・30 分目安。`.claude/guides/observation.md`）
  - エディタを保存して閉じ → `Launch-Latest.cmd` → MOD の Maps で Zone 1 の開始地点 → 敵を消す。
  - 視点: ステップ 1 と同じ `look` の量で前後を撮り、画面の中央付近の目印の横ずれ（px）から回転角を出す（水平 FOV と解像度 3440 × 1440 から `atan`）。同じ撮り方で PIE にも当てられるよう、測り方を README に書く。本家の設定の値（`Character.MouseSensitivity`・`Character.MouseSmoothing`）を `%LOCALAPPDATA%\DDeception\Saved\` の中から探して**読むだけ**（編集しない）。見つからなければ OPTIONS の画面で表示を撮る。
  - 集中線: Speed Boost を使って走り、ブースト中の静止画を十数枚（`shot`）と、時刻つきの連続撮影（`record`）を撮る。静止画を `T_Speedlines`（2 列 × 5 段、1 コマ 1920 × 1080 相当）の 10 コマと突き合わせ、1 画面に映るのが 1 コマか（2 × 5 読み）2.5 段ぶんか（2 × 2 読み）と、コマ送りの速さ（撮影の時刻とコマ番号の並びから）を決める。
  - 本家を閉じてエディタを開き直す。本家の画面は Discord へ送らない。
  - 変更予定: `observations/original/orig-look-*`・`orig-speedlines-*`（git の外）、`observations/README.md`。
- [ ] 3. 視点移動を直す（C++ か `Config/`）→ `python Tools/editor_cycle.py` でビルド → ステップ 1 と同じ測り方で本家と一致を確かめる
  - 変更予定: `Source/wasami_deception/WasamiPlayerCharacter.cpp`（`MouseAxisSensitivity` まわり・修飾子の並び）か `Config/DefaultInput.ini` / `Config/DefaultGame.ini`（`InputYawScale` 系）、実装記録 02。
- [ ] 4. 集中線を直す（`M_Speedlines` の FlipBook の列・段・位相をステップ 2 の観察に合わせる。テクスチャの設定が原因ならそれも）→ `WasamiDDTools.import_dd_powers()` で作り直す → PIE でブースト中を撮り本家と見比べる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`（`_build_speedlines`・`FLIPBOOK_*`）、`/Game/DD/UI/Main/Powers/M_Speedlines`（必要なら `T_Speedlines`）、実装記録 04（「入力はすべて既定 = 2 × 2」「原作のまま」の記述）・01。
- [ ] 5. 仕上げ: 作業一覧の項目 2 を「完了」、handover の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（note へは会話でセッションの値が渡されていなければ未反映）、症状索引（要れば）、`check_records.py --update`、記録を消して main へマージ → ブランチを削除 → push。

## 次にやること

ステップ 1。エディタが起きているかを `python Tools/ue_remote.py` で確かめ、`python Tools/pie.py start` → Zone 1 の開始地点で `getall PlayerController InputYawScale_DEPRECATED` を読み、`look --dx` の量ごとのヨーの差を測る。

## 決定事項

- 2026-09-17: 作業一覧が最初に疑った「`AxisConfig` の Mouse2D の感度 0.07 と `Scalar` 0.07 の二重掛け」は、エンジンのコードでは起きない — Enhanced Input はキーの `RawValue` を読み（UE 5.8 `EnhancedPlayerInput.cpp` 549 行）、`AxisConfig` の感度は `KeyState->Value` にだけ掛かる（`PlayerInput.cpp` 1260 行の `MassageVectorAxisInput`）。`Config/DefaultInput.ini` の Mouse2D の行は効いていない（直すかどうかはステップ 3 で決める）。
- 2026-09-17: 次に疑うもの（ステップ 1 で確かめる）: (a) `APlayerController::InputYawScale_DEPRECATED` の C++ の既定は 1.0（`PlayerController.h` 485 行）で、2.5 / −2.5 はエンジンの `BaseGame.ini` の `InputYawScale=2.5`・`InputPitchScale=-2.5` が CoreRedirect（`CoreRedirects.cpp` 3669 行の `InputYawScale` → `InputYawScale_DEPRECATED`）で入る前提。実行中に本当に 2.5 か。(b) Mouse2D の生の値の大きさ（UE4 の MouseX と同じか）。(c) FOV スケーリングに渡るカメラの FOV。(d) `UInputModifierSmooth`（サンプル数が常に 1）と UE4 の `SmoothMouse` の差。
- 2026-09-17: 本家の視点の処理（`BP_DD_PlayerCharacter` @36429〜@36968）は、軸の値 × CVar `Character.MouseSensitivity` を `AddControllerYawInput` へ、縦は × （`Character.InvertY` が 1 なら 1、0 なら −1）× 感度を `AddControllerPitchInput` へ。感度の既定は 1.0（`BP_DD_GameInstance` の `RegisterFloatCVarSetting`。OPTIONS のスライダーは右端 2.0）。最新版には `Character.MouseSmoothing`（既定 1、プレイヤーの `Mouse Smoothing Change` で切り替え）もある。本作の `Look` はこの形（`MouseSensitivity` 1.0）で、ここは変えない。
- 2026-09-17: `M_Speedlines` の FlipBook の入力（列・段・位相）は原作データに無い — cook は資産やパラメータを参照する式しか残さず（`_assets` 全体に `MaterialExpressionConstant`・`Time`・`Multiply` は 1 つも無い）、`M_Speedlines` の `Expressions` には消えた式が 4 つ（`null`）ある。実装記録 04 の「入力はすべて既定 = 2 × 2（原作のまま）」はコードの根拠ではなく推定。WebGL 版は検証映像から 2 列 × 5 段・`floor(t × 60) mod 10` にしていた（WebGL 05 記録の `boost-fx.ts`）。列・段・速さはステップ 2 の観察で決める（見た目なので、決めきれなければ仮で進めてよい）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: 開始時点では未確認（ステップ 1 の最初に確かめる）。PIE と本家は反復の終わりに必ず止める・閉じる。
- 本家の設定のファイルは読むだけにする（本家のセーブの編集は夜間に行わない）。
- MOD の W-Editor のファイル `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` は前の作業の要確認のまま（作業一覧の「未回答の要確認」）。触らない。

## 検証

- check_records: 未実行（ソースの変更なし）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
