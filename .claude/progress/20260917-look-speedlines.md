---
title: マウスの視点移動の速さとスピードブーストの集中線の修正（作業一覧の項目 2）
status: 進行中
branch: feature/look-and-speedlines
base: 7208432
started: 2026-09-17 20:54
updated: 2026-09-17 21:15
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
- [ ] 2. 本家の実機（最新版）で同じものを撮る（1 回・30 分目安。`.claude/guides/observation.md`）
  - エディタを保存して閉じ → `Launch-Latest.cmd` → MOD の Maps で Zone 1 の開始地点 → 敵を消す。
  - 視点: 開始地点で前の画面を撮り、`look --dx 2057 --steps 17`（期待値 0.175°/カウントなら 360.0°）→ 撮る、`look --dx 1029 --steps 7`（180.1°）→ 撮る、`look --dy 300 --steps 10` → 撮る（52.5° 見下ろす）。前後の画面のずれ（px）と水平 FOV から回転角を出す（直す前の本作なら 360° のはずが 25.2° しか回らない）。本家の設定の値（`Character.MouseSensitivity`・`Character.MouseSmoothing`）を `%LOCALAPPDATA%\DDeception\Saved\` の中から探して**読むだけ**（編集しない）。見つからなければ OPTIONS の画面で表示を撮る。
  - 集中線: 長くまっすぐ走れる所（本作では Zone 1 の待合から北。本家でも見通しのよい廊下を選ぶ）で Speed Boost を使って走り、ブースト中の静止画を十数枚（`shot`）と、時刻つきの連続撮影（`record`）を撮る。静止画を `T_Speedlines`（2 列 × 5 段、1 コマ 1920 × 1080 相当）の 10 コマと突き合わせ、(a) 1 画面に映るのが 1 コマか（2 × 5 読み）2.5 段ぶんか（2 × 2 読み）、(b) 線が集まる六角形の「目」が画面の中央にあるか上下の端にあるか（テクスチャの目は行 1080 × k にある）、(c) コマ送りの速さ（撮影の時刻とコマ番号の並びから）を決める。赤い線は不透明度 0.15 で薄いので、明るさを上げた切り抜きで見る。
  - 本家を閉じてエディタを開き直す。本家の画面は Discord へ送らない。
  - 変更予定: `observations/original/orig-look-*`・`orig-speedlines-*`（git の外）、`observations/README.md`。
- [ ] 3. 視点移動を直す: `WasamiPlayerCharacter.cpp` の `Look` の対応づけから `UInputModifierScalar`（`MouseAxisSensitivity`）を外す（下の決定事項）→ `python Tools/editor_cycle.py` でビルド → ステップ 1 と同じ測り方（`observations/README.md`）で 0.175°/カウント（ステップ 2 の本家の値）になることを確かめる
  - 変更予定: `Source/wasami_deception/WasamiPlayerCharacter.cpp`（`MouseAxisSensitivity` の定数と説明、`Map(LookAction, EKeys::Mouse2D, …)`）、実装記録 02（視点の係数の説明）。`Config/DefaultInput.ini` の Mouse2D の行は残す（これが効いている 0.07）。
- [ ] 4. 集中線を直す（`M_Speedlines` の FlipBook の列・段・位相をステップ 2 の観察に合わせる。テクスチャの設定が原因ならそれも）→ `WasamiDDTools.import_dd_powers()` で作り直す → PIE でブースト中を撮り本家と見比べる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`（`_build_speedlines`・`FLIPBOOK_*`）、`/Game/DD/UI/Main/Powers/M_Speedlines`（必要なら `T_Speedlines`。原作は `NeverStream` 真で本作は偽なので揃える）、実装記録 04（「入力はすべて既定 = 2 × 2」「原作のまま」の記述）・01。
- [ ] 5. 仕上げ: 作業一覧の項目 2 を「完了」、handover の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（note へは会話でセッションの値が渡されていなければ未反映）、症状索引（要れば）、`check_records.py --update`、記録を消して main へマージ → ブランチを削除 → push。

## 次にやること

ステップ 2。エディタを保存して閉じ（`python Tools/editor_cycle.py` の閉じる側、または MCP で保存してから終了）、`Launch-Latest.cmd` で本家を起動し、`.claude/guides/observation.md` の手順で Zone 1 の開始地点へ出て敵を消し、上の計画の視点と集中線を撮る。

## 決定事項

- 2026-09-17（ステップ 1 で確定）: **視点が遅い原因は感度 0.07 の二重掛け**。Enhanced Input の `IEnhancedInputSubsystemInterface::ApplyAxisPropertyModifiers`（UE 5.8 `EnhancedInputSubsystemInterface.cpp` 614〜690 行）が、マウスのキー（`Mouse2D` も `IsMouseButton()` が真。CVar `input.GlobalAxisConfigMode` の既定 0 = マウスだけ）の対応づけに、旧入力の `AxisConfig` の感度（`Config/DefaultInput.ini` の Mouse2D 0.07）を `UInputModifierScalar` として先頭に自動で足す（「Sensitivity stacks with user defined」）。本作は C++ でも Scalar 0.07 を足していたので 0.07² になった。実測 0.01225°/カウント = 0.07 × 0.07 × FOV 90 × 0.01111 × 2.5。`InputYawScale_DEPRECATED` の 2.5 / −2.5 は効いている（`bEnableLegacyInputScales` 真）。先の「`AxisConfig` は `KeyState->Value` にだけ掛かるので効かない」は誤りだった（`RawValue` を読むのは正しいが、別の経路で修飾子になる）。
- 2026-09-17: 直し方は **C++ の Scalar を外し、`DefaultInput.ini` の `AxisConfig`（原作の DefaultInput.ini の値 0.07）を効かせる**。UE4 と同じ仕組み（軸の設定の感度）に値を写す形で、original-fidelity の「UE に同じ仕組みがあれば値を写すだけ」に合う。実行中に C++ 側の複製を 1.0 にすると 0.175°/カウント（本家の式の期待値）になった。自動の修飾子は Smooth・FOV の前に入るが、どれも値に比例するので順は結果を変えない。
- 2026-09-17: 本家の視点の処理（`BP_DD_PlayerCharacter` @36429〜@36968）は、軸の値 × CVar `Character.MouseSensitivity` を `AddControllerYawInput` へ、縦は × （`Character.InvertY` が 1 なら 1、0 なら −1）× 感度を `AddControllerPitchInput` へ。感度の既定は 1.0（`BP_DD_GameInstance` の `RegisterFloatCVarSetting`。OPTIONS のスライダーは右端 2.0）。最新版には `Character.MouseSmoothing`（既定 1、プレイヤーの `Mouse Smoothing Change` で切り替え）もある。本作の `Look` はこの形（`MouseSensitivity` 1.0）で、ここは変えない。
- 2026-09-17: `M_Speedlines` の FlipBook の入力（列・段・位相・UV）は原作データに無い — 関数の呼び出しの `FunctionInputs` は cook で消え、`M_Speedlines` の `Expressions` には消えた式が 4 つ（`null`）ある（定数や Time などが入力につながっていたはず）。実装記録 04 の「入力はすべて既定 = 2 × 2（原作のまま）」はコードの根拠ではなく推定。WebGL 版は検証映像から 2 列 × 5 段・`floor(t × 60) mod 10` にしていた（WebGL 05 記録の `boost-fx.ts`）。列・段・位相・速さはステップ 2 の観察で決める（見た目なので、決めきれなければ仮で進めてよい）。
- 2026-09-17（ステップ 1）: ノイズに見える主因の見立て — 2 × 2 読みで 2.5 段ぶんを縦に縮め（ビューポートで約 3.3 倍、全画面でも約 1.5 倍の縮小）、ミップの無いテクスチャの 1 px の水平線の束がちらつく。2 × 5 読みなら全画面では拡大になる。ただしテクスチャ自体にも水平線が多いので、2 × 5 でも水平線は出る。本家の絵と比べて決める。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: ステップ 1 の終わりに起動中（L_Hospital_Zone1、PIE なし、未保存なし）。`t.MaxFPS` は 0 に戻した。PIE と本家は反復の終わりに必ず止める・閉じる。
- `desktop.py look` は `dx // steps` を steps 回送る（割り切れない量は丸められる）。
- 本家の設定のファイルは読むだけにする（本家のセーブの編集は夜間に行わない）。
- MOD の W-Editor のファイル `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` は前の作業の要確認のまま（作業一覧の「未回答の要確認」）。触らない。

## 検証

- check_records: 未実行（ソースの変更なし）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 1 で PIE の測定（視点・集中線）。コードの変更はまだ無い
