---
title: 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）
status: 進行中
branch: main
base: ca4891f
started: 2026-09-21 06:53
updated: 2026-09-21 07:20
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
- [ ] 2. `dd_particles` が GPU のエミッタの分布を焼き込みの表でなく `ResourceData` から組むようにする
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_particles.py`、`.claude/references/troubleshooting.md` の GPU のエミッタの項
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

## GPU のエミッタの突き合わせ（ステップ 1 の結果）

本作の `/Game/DD` の 1005 のパーティクルシステムのうち **GPU のエミッタを持つのは 3 つ・5 エミッタだけ**（エディタで全走査した）。項目 33 の対象 4 行のうち GPU のエミッタがあるのは 1（トンネルの扉の破片）と 2 の `Concrete_impact_large` だけで、`Fracture_dark_slow`・`P_06_NurseSparks`・`P_06_NurseDoorHit`・`P_06_Defib` には無い（＝それらはステップ 4〜6 の材質の話）。差は `python Tools/dd/gpu_emitters.py <名前> --ours` で出る（`*` の行）。

| システム / エミッタ | 行 | 原作（cook の `ResourceData`） | 本作（UE 5.8 が分布から組んだもの） |
| --- | --- | --- | --- |
| Fracture_concrete_3 / Fragments | 色 | 一定 (0.205, 0.205, 0.205) | 一定の白 (1, 1, 1) |
| 〃 | アルファ | 16 点。0.53 まで 1 を保って 0 へ | 2 点。直線で 1 → 0 |
| Fracture_concrete_3 / DustTrail | 色 | 一定 0.0784 | 1.0 → 0.36 |
| 〃 | アルファ | 2.2408 → 0.3869（8 点） | 1.0 → 0.3869（64 点） |
| 〃 | 大きさ | 600 → 3853.6（6.4 倍に育つ） | 439.6 → 603.7 |
| 〃 | SubUV | 1 → 63（8×8 の 64 枚に合う） | 1 → 127（2 周する） |
| Concrete_impact_large / ConcreteBits | 色 | 一定 (0.0902, 0.0863, 0.0863) | 一定の白 |
| 〃 | アルファ | 16 点。保ってから 0 へ | 2 点。直線 |
| Concrete_impact_large / Sparks | 色 | 一定の HDR 橙 (10, 3.20271, 1.76228) | 青白 165 → 橙 0.865 の減衰 |
| 〃 | アルファ | 16 点。0.8 まで 1 を保って 0 へ | 128 点。1 → 0 |
| P_06_NursesLand / ConcreteBits | 色・アルファ | 一定 (0.3178, 0.3039, 0.3039)・アルファ 1 | **一致**（cook の 16 点は死んだ残骸） |
| 上の 5 つすべて | bUseVelocityForMotionBlur | False | True（UE 5 の CVar の既定） |

**どれも原作のデータで決まる。本家の収録は要らない。** ほかの行（`MiscBias`・`SimulationAttrCurve*`・衝突・軌道・`InvMaxSize`・`MaxLifetime`・`MaxParticleCount` など）はすべて一致している。

## 次にやること

ステップ 2。`dd_particles` の `_gpu_distributions` を、GPU のエミッタのときだけ焼き込みの表でなく cook の `ResourceData` から分布を作るように変える（色とアルファ → `ColorOverLife` モジュールの 2 つ、大きさ → `SizeMultiplyLife` の `LifeMultiplier`（misc の R・G ÷ `MaxSize`）、SubUV → `SubUV` モジュールの `SubImageIndex`（misc の B））。`Scale` が 0 の面は一定、量子化の並びが空なら一定。当てにするモジュールが 1 つでないときは例外にする。

## 決定事項

- 2026-09-21: 原作の cook の JSON には GPU のエミッタの `ResourceData` がそのまま入っている（`Tools/dd/gpu_emitters.py` が読む）。`DustTrail` の色が表 1 → 0.36 に対し cook は一定の 0.0784 という既知のずれも、`ColorBias` として実際に入っていた。
- 2026-09-21: 本家の収録には頼らない（作業一覧の大目標 3 の節の頭。移動が遅すぎて該当の場面に届かない）。コードで決まらなかったものだけをステップ 7 で 1 回の収録にまとめるか、決まらない理由を書いて閉じる。
- 2026-09-21: **GPU のエミッタは `ResourceData` が正本**。`UParticleModuleTypeDataGpu::Build` は丸ごと `#if WITH_EDITOR` なので、cook されたゲームは組み直さず保存された `ResourceData` を読んで描く。モジュールの焼き込みの表と食い違っていても、原作の見え方は `ResourceData` の側（原作の BallisticsVFX の 2 つは実際に食い違っていて、表の側が別の版に見える。`P_06_NursesLand` のような本家製のものは一致する）。
- 2026-09-21: `ResourceData` をアセットに書いても無駄。**UE 5.8 は読み込みのたびに組み直す**（`UParticleEmitter::PostLoad` → `UpdateModuleLists` → `Build`）ので、分布の側を `ResourceData` から作って組み直させる。量子化（8 ビット）を経た値がゲームの使う値そのものなので、量子化の並びを合わせれば足りる。
- 2026-09-21: `bUseVelocityForMotionBlur` は原作（UE 4.21・4.24）に無い項目で、UE 5 の CVar `fx.Cascade.UseVelocityForMotionBlur`（既定 true）から来る。本作の Cascade はすべて原作のものなので、`Config/DefaultEngine.ini` の `[SystemSettings]` で 0 にする（ステップ 3）。モジュールごとの上書きにはしない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 2 の確かめには動いているエディタが要る（`python Tools/dd/gpu_emitters.py <名前> --ours`）。
- 組み直しは `WasamiDDTools.import_dd_gimmicks`・`import_dd_cutscenes`（`dd_gimmicks.py` の `BURST`・`CELL_PARTICLES`・`CUTSCENE_PARTICLES`）から。GPU のエミッタの表を書き換えたら、エディタで `Values` の数と `EntryCount × EntryStride` を比べて確かめる（症状索引「粒子が出て 1 秒ほどで `Array index out of bounds`」）。

## 検証

- check_records: OK（2026-09-21。`Tools/dd/gpu_emitters.py` を 01 記録に足した）
- C++ ビルド: 未実行（この項目は C++ を変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: `gpu_emitters.py --ours` で 5 つの GPU のエミッタを読み出した（ステップ 1）。組み直しと PIE はこれから
