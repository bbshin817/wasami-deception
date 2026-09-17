---
title: マウスの視点移動の速さとスピードブーストの集中線の修正（作業一覧の項目 2）
status: 進行中
branch: feature/look-and-speedlines
base: 7208432
started: 2026-09-17 20:54
updated: 2026-09-17 22:25
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

- [x] 1. PIE で今の値を測り、原因を切り分けた（測った値と撮った物は `observations/README.md` の「視点の速さと集中線」）
- [x] 2. 本家の実機（最新版）で撮った: 視点は **0.175°/カウント**（感度 1・平滑化オン）、集中線は **2 × 5 を行 0 から 1 画面 1 コマ・1 秒に 30 コマ**（測り方と値は `observations/README.md` の original の「視点の速さと集中線」）
- [x] 3. 視点を直した: `Look` の対応づけから Scalar を外し、感度 0.07 は `DefaultInput.ini` の `AxisConfig` だけで効かせる（実装記録 02・症状索引）。PIE で 0.175°/カウント（dx 100 → 17.5°、dx 2057 → 359.94°、dy 100 → −17.5°）
- [ ] 4. 集中線を直す（`M_Speedlines` の FlipBook を 2 列 × 5 段・位相 `Time × 3`〈下の決定事項〉にする。UE 5.8 の FlipBook が位相の小数部を取るかを先に読む。テクスチャの `NeverStream` も原作に揃える）→ `WasamiDDTools.import_dd_powers()` で作り直す → PIE でブースト中を撮り本家と見比べる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`（`_build_speedlines`・`FLIPBOOK_*`）、`/Game/DD/UI/Main/Powers/M_Speedlines`（必要なら `T_Speedlines`。原作は `NeverStream` 真で本作は偽なので揃える）、実装記録 04（「入力はすべて既定 = 2 × 2」「原作のまま」の記述）・01。
- [ ] 5. 仕上げ: 作業一覧の項目 2 を「完了」、handover の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（note へは会話でセッションの値が渡されていなければ未反映）、症状索引（要れば）、`check_records.py --update`、記録を消して main へマージ → ブランチを削除 → push。

## 次にやること

ステップ 4。UE 5.8 の `FlipBook` の材質関数（`/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook`）が位相の小数部を取るか・コマの順（左→右、上→下）を読む → `dd_powers.py` の `_build_speedlines` で `FLIPBOOK_*` を 2 列 × 5 段・位相 `Time × 3` にし、`T_Speedlines` の `NeverStream` を真にする → `WasamiDDTools.import_dd_powers()` → PIE でブースト中を撮り（`observations/README.md` のステップ 1 と同じ走り）、本家の連写と見比べる → 実装記録 04・01 → `check_records.py --update` → コミット。

## 決定事項

- 2026-09-17: 視点の原因と直し方（`AxisConfig` の自動の Scalar との二重掛け）はステップ 3 で実装記録 02 と症状索引へ移した。本家の `Look` の形（× `Character.MouseSensitivity` 1.0）は変えていない。
- 2026-09-17（ステップ 2）: **`M_Speedlines` の FlipBook は 2 列 × 5 段、位相は 1 秒に 3 周（30 コマ/s）、UV は `TexCoord 0` のまま（ずらさない）**。入力は原作データに無い（関数の呼び出しの `FunctionInputs` は cook で消え、`Expressions` に消えた式が 4 つある = 定数 2・定数 5・`Time`・×3 の `Multiply` などと読める）ので、最新版の実機の無劣化の連写とテクスチャの照合で決めた（`observations/README.md`）。1 コマ 1081 行の読みが 2 × 2（2702 行）と半コマずらしより明らかに合い、コマは 0〜9 の順、30.005 コマ/s で確かなフレームの 37/46 が合う。WebGL 版の 60 コマ/s とは違う（実機を採る）。見た目の値なので要確認にはしない。実装記録 04 の「2 × 2（原作のまま）」の記述はステップ 4 で直す。
- 2026-09-17（ステップ 1）: ノイズに見える主因の見立て — 2 × 2 読みで 2.5 段ぶんを縦に縮め、ミップの無いテクスチャの 1 px の水平線の束がちらつく。2 × 5 なら全画面では拡大になる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: ステップ 3 で `editor_cycle.py` でビルドして開き直した（L_Hospital_Zone1、PIE なし、`desktop.py` の係も止めた）。本家は起動していない。PIE と本家は反復の終わりに必ず止める・閉じる。
- 本家のセーブ: 観察で本家が `SaveSlot.sav`・`structSlot.sav` を書き換えた（21:25。いつもの動き）。控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260917-211519`。戻さない。
- `desktop.py look` は `dx // steps // burst` を送る（割り切れる量にする）。PIE のビューポート（3440 × 1440 の画面で (2750, 500) あたり。撮って確かめる）を押すとき、前面が端末なら最初の 1 回だけ `--allow WindowsTerminal.exe --allow UnrealEditor.exe`。
- MOD の W-Editor のファイル `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` は前の作業の要確認のまま（作業一覧の「未回答の要確認」。今回は書き込みなし）。触らない。

## 検証

- check_records: ステップ 3 で OK（02 記録）
- C++ ビルド: ステップ 3 で成功（`editor_cycle.py`）
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 3 で PIE の視点が 0.175°/カウント（`observations/README.md` の ours の表）。集中線は未変更
