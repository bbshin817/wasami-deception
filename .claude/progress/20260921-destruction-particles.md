---
title: 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）
status: 進行中
branch: main
base: ca4891f
started: 2026-09-21 06:53
updated: 2026-09-21 08:45
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

- [x] 1〜3. GPU のエミッタ（3 システム・5 エミッタ）を cook の `ResourceData` に合わせた。突き合わせの道具 `Tools/dd/gpu_emitters.py` を作り（ステップ 1）、`dd_particles._gpu_resource` が色・アルファ・大きさ・SubUV を cook の `ResourceData` から組むようにし（ステップ 2。01 記録）、3 つを組み直して差が消えたことを確かめ、`Config/DefaultEngine.ini` の `[SystemSettings]` に `fx.Cascade.UseVelocityForMotionBlur=0` を入れて PIE で扉の破片を見た（ステップ 3。00 記録）
- [ ] 4. Zone 2 の独房の粒子 4 材質と棘の黒い塵 `Fracture_dark_slow` を原作のデータと突き合わせる
  - 変更予定: `/Game/DD/...` の材質 4 つ・`Fracture_dark_slow`
- [ ] 5. ナースの扉突きの塵 `P_06_NurseDoorHit`（材質 `Whisps_trans` の推定）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_NurseDoorHit` と材質
- [ ] 6. 除細動器の放電 `P_06_Defib` の `thander`（`M_ky_spark02_4x4`）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_Defib`、`M_ky_spark02_4x4`
- [ ] 7. PIE で 4 か所を通して確かめ、実装記録 07・08 と作業一覧を直して項目 33 を閉じる
  - 変更予定: `.claude/implementation-records/07-*`・`08-*`、`.claude/roadmap.md`、`.claude/references/handover.md`

## GPU のエミッタの結果（ステップ 1〜3。ここは済み）

本作の `/Game/DD` の 1005 のパーティクルシステムのうち **GPU のエミッタを持つのは 3 つ・5 エミッタだけ**（`Fracture_concrete_3` の `Fragments`・`DustTrail`、`Concrete_impact_large` の `ConcreteBits`・`Sparks`、`P_06_NursesLand` の `ConcreteBits`）。項目 33 の残りの対象（`Fracture_dark_slow`・`P_06_NurseSparks`・`P_06_NurseDoorHit`・`P_06_Defib`）には GPU のエミッタが無いので、ステップ 4〜6 は材質の話だけ。

`python Tools/dd/gpu_emitters.py <名前> --ours` の差（`*` の行）は、組み直した後は次の 2 種類だけ残る。**どちらも直さない**（下の決定事項）:

- 量子化の角の 1〜3 段（255 分の）のずれ。`OptimizeLookupTable` の丸めで、見え方は同じ。
- `P_06_NursesLand` の `ConcreteBits` の `QuantizedColorSamples` 16 点（本作は空）。

## 次にやること

ステップ 4。Zone 2 の独房の粒子 4 材質（`M_06_NurseSparks`・`M_Spark`・`M_Radial_Gradient`・`Squib_one`）と棘の黒い塵 `Fracture_dark_slow` を、原作の cook の材質（`Tools/dd/cooked_shaders.py`）とアセットの値で突き合わせ、推定で組んだところを直す。組み直しは `WasamiDDTools.import_dd_gimmicks`。

## 決定事項

- 2026-09-21: 本家の収録には頼らない（作業一覧の大目標 3 の節の頭。移動が遅すぎて該当の場面に届かない）。コードで決まらなかったものだけをステップ 7 で 1 回の収録にまとめるか、決まらない理由を書いて閉じる。
- 2026-09-21: 読み戻した曲線は、エディタの `OptimizeLookupTable` が標本点の外の角を 1〜3 段（255 分の数）丸める。量子化の並びがそこだけ数ずれても直さない（見え方は同じで、原作自身がその丸めを通った値）。
- 2026-09-21: `P_06_NursesLand` の `ConcreteBits` が持つ `QuantizedColorSamples` 16 点は**原作の死んだデータ**なので、本作が空でも直さない。cook には `ColorScale` が無く（＝(0,0,0,0)）、色もアルファも一定（`ColorBias` (0.3178, 0.3039, 0.3039, 1)）。`FComposableDistribution::QuantizeVector4` は引き当ての表が 1 点のとき早く返り、**前の標本を消さない**ので、原作では色が一定になる前の曲線が残ったまま cook に入った。本作は初めから一定なので標本が空になるが、`FParticleCurveTexture::AddCurve` は空の配列をそのまま受け（割り当て 0）、`ColorScale` が 0 なので描かれる色は同じ。

## 要確認（ユーザー）

- 2026-09-21: エディタを開き直したときに Windows の**ファイアウォールの許可ダイアログ**（「パブリック ネットワークとプライベート ネットワークにこのアプリへのアクセスを許可しますか？」/ UnrealEditor）が画面に出たまま。OS 全体の設定なので触っていない。ユーザーに閉じるか許可するかを決めてもらう（出ている間は画面の左寄りが隠れ、PIE の撮影の範囲が狭まる）。

## 再開時の注意

- 長時間処理は無い。突き合わせ（`python Tools/dd/gpu_emitters.py <名前> --ours`、`python Tools/dd/cooked_shaders.py <パスの一部>.`）には動いているエディタが要る。
- 組み直しは `WasamiDDTools.import_dd_gimmicks`（`dd_gimmicks.py` の `BURST`・`CELL_PARTICLES`・`CUTSCENE_PARTICLES`）から。`import_dd_gimmicks` 1 回で切り出しの粒子も入る。
- `Config/DefaultEngine.ini` の CVar はエンジンの起動時にしか読まれない。粒子の `ResourceData` に効く設定を足したら `python Tools/editor_cycle.py --no-build` で開き直してから確かめる。
- **`Tools/playthrough.py` の `VIEWPORT` (1822, 206, 2862, 858) は今の画面に合っていない**（出力ログの窓がビューポートの左半分に重なっている）。PIE を撮るときは `python Tools/desktop.py shot --scale 1.0` で測り直す。2026-09-21 に扉の破片を撮ったときは `--crop 2230,430,2860,985` で重なりを避けた。

## 検証

- check_records: OK（2026-09-21。`fx.Cascade.UseVelocityForMotionBlur` を 00 記録に足した）
- C++ ビルド: この項目は C++ を変えない
- エディタでの確認: 3 つの GPU のシステムを組み直し、`gpu_emitters.py --ours` で 5 エミッタとも `bUseVelocityForMotionBlur` の差が消えたのを確かめた。PIE（Zone 1）で扉の破片 `Fracture_concrete_3` を出し、塵が 0.2 秒ほどで視界を覆って 1.4 秒ほどで晴れるのを見た（`Intermediate/Overnight/shots/z1_doors_burst.png`）。ログに粒子のエラーは無い。Zone 2 の独房の `Concrete_impact_large` はステップ 7 の通しで見る。
