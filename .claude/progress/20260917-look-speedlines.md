---
title: マウスの視点移動の速さとスピードブーストの集中線の修正（作業一覧の項目 2）
status: 進行中
branch: feature/look-and-speedlines
base: 7208432
started: 2026-09-17 20:54
updated: 2026-09-17 22:05
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
- [ ] 3. 視点移動を直す: `WasamiPlayerCharacter.cpp` の `Look` の対応づけから `UInputModifierScalar`（`MouseAxisSensitivity`）を外す（下の決定事項）→ `python Tools/editor_cycle.py` でビルド → ステップ 1 と同じ測り方（`observations/README.md`。`pie.py state` のヨーの差）で 0.175°/カウント（本家の値）になることを確かめる。`look` は本家と同じく `--burst` を付けて送る（例: `--dx 2057 --steps 17 --burst 11` で 360.0°、`--dx 100 --steps 10 --burst 10` で 17.5°）
  - 変更予定: `Source/wasami_deception/WasamiPlayerCharacter.cpp`（`MouseAxisSensitivity` の定数と説明、`Map(LookAction, EKeys::Mouse2D, …)`）、実装記録 02（視点の係数の説明）。`Config/DefaultInput.ini` の Mouse2D の行は残す（これが効いている 0.07）。
- [ ] 4. 集中線を直す（`M_Speedlines` の FlipBook を 2 列 × 5 段・位相 `Time × 3`〈下の決定事項〉にする。UE 5.8 の FlipBook が位相の小数部を取るかを先に読む。テクスチャの `NeverStream` も原作に揃える）→ `WasamiDDTools.import_dd_powers()` で作り直す → PIE でブースト中を撮り本家と見比べる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`（`_build_speedlines`・`FLIPBOOK_*`）、`/Game/DD/UI/Main/Powers/M_Speedlines`（必要なら `T_Speedlines`。原作は `NeverStream` 真で本作は偽なので揃える）、実装記録 04（「入力はすべて既定 = 2 × 2」「原作のまま」の記述）・01。
- [ ] 5. 仕上げ: 作業一覧の項目 2 を「完了」、handover の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（note へは会話でセッションの値が渡されていなければ未反映）、症状索引（要れば）、`check_records.py --update`、記録を消して main へマージ → ブランチを削除 → push。

## 次にやること

ステップ 3。`WasamiPlayerCharacter.cpp` の `Map(LookAction, EKeys::Mouse2D, …)` から `Sensitivity`（`UInputModifierScalar`、`MouseAxisSensitivity` 0.07）を外し、定数と説明を直す → `python Tools/editor_cycle.py` → PIE（`pie.py start` → `place 15 385 --yaw -90` → ビューポートを押す → `look --burst` → `pie.py state`）で 0.175°/カウントを確かめる → 実装記録 02 → `check_records.py --update` → コミット。

## 決定事項

- 2026-09-17（ステップ 1 で確定）: **視点が遅い原因は感度 0.07 の二重掛け**。Enhanced Input の `IEnhancedInputSubsystemInterface::ApplyAxisPropertyModifiers`（UE 5.8 `EnhancedInputSubsystemInterface.cpp` 614〜690 行）が、マウスのキー（`Mouse2D` も `IsMouseButton()` が真。CVar `input.GlobalAxisConfigMode` の既定 0 = マウスだけ）の対応づけに、旧入力の `AxisConfig` の感度（`Config/DefaultInput.ini` の Mouse2D 0.07）を `UInputModifierScalar` として先頭に自動で足す（「Sensitivity stacks with user defined」）。本作は C++ でも Scalar 0.07 を足していたので 0.07² になった。実測 0.01225°/カウント = 0.07 × 0.07 × FOV 90 × 0.01111 × 2.5。`InputYawScale_DEPRECATED` の 2.5 / −2.5 は効いている（`bEnableLegacyInputScales` 真）。先の「`AxisConfig` は `KeyState->Value` にだけ掛かるので効かない」は誤りだった（`RawValue` を読むのは正しいが、別の経路で修飾子になる）。
- 2026-09-17: 直し方は **C++ の Scalar を外し、`DefaultInput.ini` の `AxisConfig`（原作の DefaultInput.ini の値 0.07）を効かせる**。UE4 と同じ仕組み（軸の設定の感度）に値を写す形で、original-fidelity の「UE に同じ仕組みがあれば値を写すだけ」に合う。実行中に C++ 側の複製を 1.0 にすると 0.175°/カウント（本家の式の期待値）になった。自動の修飾子は Smooth・FOV の前に入るが、どれも値に比例するので順は結果を変えない。
- 2026-09-17: 本家の視点の処理（`BP_DD_PlayerCharacter` @36429〜@36968）は、軸の値 × CVar `Character.MouseSensitivity` を `AddControllerYawInput` へ、縦は × （`Character.InvertY` が 1 なら 1、0 なら −1）× 感度を `AddControllerPitchInput` へ。感度の既定は 1.0（`BP_DD_GameInstance` の `RegisterFloatCVarSetting`。OPTIONS のスライダーは右端 2.0）。最新版には `Character.MouseSmoothing`（既定 1、プレイヤーの `Mouse Smoothing Change` で切り替え）もある。本作の `Look` はこの形（`MouseSensitivity` 1.0）で、ここは変えない。
- 2026-09-17（ステップ 2）: **`M_Speedlines` の FlipBook は 2 列 × 5 段、位相は 1 秒に 3 周（30 コマ/s）、UV は `TexCoord 0` のまま（ずらさない）**。入力は原作データに無い（関数の呼び出しの `FunctionInputs` は cook で消え、`Expressions` に消えた式が 4 つある = 定数 2・定数 5・`Time`・×3 の `Multiply` などと読める）ので、最新版の実機の無劣化の連写とテクスチャの照合で決めた（`observations/README.md`）。1 コマ 1081 行の読みが 2 × 2（2702 行）と半コマずらしより明らかに合い、コマは 0〜9 の順、30.005 コマ/s で確かなフレームの 37/46 が合う。WebGL 版の 60 コマ/s とは違う（実機を採る）。見た目の値なので要確認にはしない。実装記録 04 の「2 × 2（原作のまま）」の記述はステップ 4 で直す。
- 2026-09-17（ステップ 1）: ノイズに見える主因の見立て — 2 × 2 読みで 2.5 段ぶんを縦に縮め、ミップの無いテクスチャの 1 px の水平線の束がちらつく。2 × 5 なら全画面では拡大になる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: ステップ 2 の終わりに `editor_cycle.py --no-build --no-quit` で開き直した（L_Hospital_Zone1、PIE なし）。本家は閉じた。PIE と本家は反復の終わりに必ず止める・閉じる。
- 本家のセーブ: 観察で本家が `SaveSlot.sav`・`structSlot.sav` を書き換えた（21:25。いつもの動き）。控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260917-211519`。戻さない。
- `desktop.py look` は `dx // steps // burst` を送る（割り切れる量にする）。
- MOD の W-Editor のファイル `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` は前の作業の要確認のまま（作業一覧の「未回答の要確認」。今回は書き込みなし）。触らない。

## 検証

- check_records: ステップ 2 で OK（`Tools/desktop.py` の `--burst` を 01 記録に書いた）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 1 で PIE の測定（視点・集中線）。コードの変更はまだ無い
