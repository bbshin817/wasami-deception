---
title: 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）
status: 進行中
branch: main
base: ca4891f
started: 2026-09-21 06:53
updated: 2026-09-21 08:10
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）

## 依頼

`.claude/roadmap.md` の大目標 3「本家に忠実に」の項目 33。大目標 1・2 で焼き込みのシェーダーと cook の表から推定で組んだ破壊と粒子の材質・分布を、cook の `ResourceData` とコンパイル済みシェーダーの式から本家どおりにする。対象は 4 行:

1. トンネルの扉の破片 `Fracture_concrete_3`（GPU のエミッタ `Fragments`・`DustTrail` の分布が cook の焼き込みの表と合わない。材質 3 つは推定）
2. Zone 2 の独房の粒子 4 材質（`M_06_NurseSparks`・`M_Spark`・`M_Radial_Gradient`・`Squib_one`）と棘の黒い塵 `Fracture_dark_slow`
3. ナースの扉突きの塵 `P_06_NurseDoorHit`（推定の `Whisps_trans` に加算を上書き）
4. 除細動器の放電 `P_06_Defib` の `thander`（材質 `M_ky_spark02_4x4`）

条件（2026-09-21 にコード優先へ書き換え。`.claude/roadmap.md` の大目標 3 の節の頭）: **本家の実機は中の移動が遅すぎて 1 回の収録で 1 か所しか回れず、2026-09-20・21 の収録では該当の場面（B1 の扉の破片・B2 の独房の粒子）に届かなかった**ので、原作のブループリント・アセットの値・cook の `ResourceData`・コンパイル済みシェーダーの式で決め、**それでも決まらなかったものだけ**を 1 回の収録にまとめる。暗い場所でほとんど見えない粒子は、原作でもエミッタの位置と分布が同じ（＝本家でも見えない）ことを原作のデータで確かめたら閉じてよい。大目標 3 なので、見た目の詰めは後回しにしない。

## 計画

- [x] 1. 原作の GPU のエミッタの `ResourceData` を読んで本作の粒子と突き合わせる表を作った（`Tools/dd/gpu_emitters.py`。下の表）
- [x] 2. `dd_particles._gpu_resource` が、GPU のエミッタの色・アルファ・大きさ・SubUV を cook の `ResourceData` から組むようにした（01 記録）。**cook が分布オブジェクトを残している所はそのまま**（原作の組み立てが読んだ値そのもの）。書き出し 105 の GPU のシステムで例外の出方が変更前と同じことを確かめた（31 は `_table_distribution` の既知の未対応で、本作が取り込まないもの）
- [ ] 3. GPU のエミッタを持つ 3 つのシステムを組み直し、`gpu_emitters.py --ours` の差が消えたことを確かめて PIE で見る。`fx.Cascade.UseVelocityForMotionBlur=0` も入れる
  - 変更予定: `/Game/DD/.../Fracture_concrete_3`・`Concrete_impact_large`・`P_06_NursesLand`、`Config/DefaultEngine.ini`
- [ ] 4. Zone 2 の独房の粒子 4 材質と棘の黒い塵 `Fracture_dark_slow` を原作のデータと突き合わせる
  - 変更予定: `/Game/DD/...` の材質 4 つ・`Fracture_dark_slow`
- [ ] 5. ナースの扉突きの塵 `P_06_NurseDoorHit`（材質 `Whisps_trans` の推定）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_NurseDoorHit` と材質
- [ ] 6. 除細動器の放電 `P_06_Defib` の `thander`（`M_ky_spark02_4x4`）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_Defib`、`M_ky_spark02_4x4`
- [ ] 7. PIE で 4 か所を通して確かめ、実装記録 07・08 と作業一覧を直して項目 33 を閉じる
  - 変更予定: `.claude/implementation-records/07-*`・`08-*`、`.claude/roadmap.md`、`.claude/references/handover.md`

## GPU のエミッタの突き合わせ（ステップ 1・2 の結果）

本作の `/Game/DD` の 1005 のパーティクルシステムのうち **GPU のエミッタを持つのは 3 つ・5 エミッタだけ**（エディタで全走査した）。項目 33 の対象 4 行のうち GPU のエミッタがあるのは 1（トンネルの扉の破片）と 2 の `Concrete_impact_large` だけで、`Fracture_dark_slow`・`P_06_NurseSparks`・`P_06_NurseDoorHit`・`P_06_Defib` には無い（＝それらはステップ 4〜6 の材質の話）。差は `python Tools/dd/gpu_emitters.py <名前> --ours` で出る（`*` の行）。

ステップ 3 で直るはずの差（組み直す前の値。原作 → 本作）:

| システム / エミッタ | 原作（cook の `ResourceData`） | 変更前の本作 |
| --- | --- | --- |
| Fracture_concrete_3 / Fragments | 色 一定 0.205、アルファ 16 点（0.8 まで 1 を保って 0 へ） | 白、アルファ 2 点の直線 |
| Fracture_concrete_3 / DustTrail | 色 一定 0.0784、アルファ 8 点 2.2408 → 0.3869、大きさ 1 → 6.42（128 点）、SubUV 1 → 63（128 点） | 1.0 → 0.36、アルファ 64 点 1.0 → 0.3869、大きさ 0.73 → 1.0、SubUV 1 → 127 |
| Concrete_impact_large / ConcreteBits | 色 一定 (0.0902, 0.0863, 0.0863)、アルファ 16 点 | 白、アルファ 2 点 |
| Concrete_impact_large / Sparks | 色 一定の HDR 橙 (10, 3.20271, 1.76228)、アルファ 16 点 | 青白 165 → 橙 0.865 の減衰、アルファ 128 点 |
| P_06_NursesLand / ConcreteBits | 一定 (0.3178, 0.3039, 0.3039)・アルファ 1 | **一致**（変わらないはず） |
| 上の 5 つすべて | bUseVelocityForMotionBlur False | True（UE 5 の CVar の既定。ステップ 3 の `DefaultEngine.ini`） |

**どれも原作のデータで決まる。本家の収録は要らない。** ほかの行（`MiscBias`・`SimulationAttrCurve*`・衝突・軌道・`InvMaxSize`・`MaxLifetime`・`MaxParticleCount` など）はすべて一致している。

## 次にやること

ステップ 3。GPU のエミッタを持つ 3 つのシステムを組み直し（`WasamiDDTools.import_dd_gimmicks`・`import_dd_cutscenes`）、`python Tools/dd/gpu_emitters.py <名前> --ours` で上の表の差が消えたことを確かめ、`Config/DefaultEngine.ini` の `[SystemSettings]` に `fx.Cascade.UseVelocityForMotionBlur=0` を入れて PIE で見る。

## 決定事項

- 2026-09-21: 本家の収録には頼らない（作業一覧の大目標 3 の節の頭。移動が遅すぎて該当の場面に届かない）。コードで決まらなかったものだけをステップ 7 で 1 回の収録にまとめるか、決まらない理由を書いて閉じる。
- 2026-09-21: `bUseVelocityForMotionBlur` は原作（UE 4.21・4.24）に無い項目で、UE 5 の CVar `fx.Cascade.UseVelocityForMotionBlur`（既定 true）から来る。本作の Cascade はすべて原作のものなので、`Config/DefaultEngine.ini` の `[SystemSettings]` で 0 にする（ステップ 3）。モジュールごとの上書きにはしない。
- 2026-09-21: 読み戻した曲線は、エディタの `OptimizeLookupTable` が標本点の外の角を 1〜2 段（255 分の数）丸める。ステップ 3 で量子化の並びがそこだけ数ずれても直さない（見え方は同じで、原作自身がその丸めを通った値）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 3 の確かめには動いているエディタが要る（`python Tools/dd/gpu_emitters.py <名前> --ours`）。
- 組み直しは `WasamiDDTools.import_dd_gimmicks`・`import_dd_cutscenes`（`dd_gimmicks.py` の `BURST`・`CELL_PARTICLES`・`CUTSCENE_PARTICLES`）から。GPU のエミッタの表を書き換えたら、エディタで `Values` の数と `EntryCount × EntryStride` を比べて確かめる（症状索引「粒子が出て 1 秒ほどで `Array index out of bounds`」）。

## 検証

- check_records: OK（2026-09-21。`Tools/dd/gpu_emitters.py` と `_gpu_resource` を 01 記録に足した）
- C++ ビルド: 未実行（この項目は C++ を変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: `gpu_emitters.py --ours` で 5 つの GPU のエミッタを読み出した（ステップ 1）。組み直しと PIE はこれから
