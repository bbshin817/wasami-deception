---
title: 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）
status: 進行中
branch: main
base: ca4891f
started: 2026-09-21 06:53
updated: 2026-09-21 06:53
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

- [ ] 1. 原作の GPU のエミッタの `ResourceData` を読んで本作の粒子と突き合わせる表を作る（調査）
  - 変更予定: `.claude/progress/20260921-destruction-particles.md`（表は記録の中）、要れば `Tools/dd/` に読み出しの小道具
- [ ] 2. `dd_particles` が GPU のエミッタの分布を焼き込みの表でなく `ResourceData` から組むようにする
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_particles.py`、`.claude/references/troubleshooting.md` の GPU のエミッタの項
- [ ] 3. `Fracture_concrete_3`（トンネルの扉の破片）を組み直して PIE で見る
  - 変更予定: `/Game/DD/.../Fracture_concrete_3`、材質 3 つ（`whispOne_Master_directional`・`_amb`・`DebrisMaster`）
- [ ] 4. Zone 2 の独房の粒子 4 材質と棘の黒い塵 `Fracture_dark_slow` を原作のデータと突き合わせる
  - 変更予定: `/Game/DD/...` の材質 4 つ・`Fracture_dark_slow`
- [ ] 5. ナースの扉突きの塵 `P_06_NurseDoorHit`（材質 `Whisps_trans` の推定）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_NurseDoorHit` と材質
- [ ] 6. 除細動器の放電 `P_06_Defib` の `thander`（`M_ky_spark02_4x4`）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_Defib`、`M_ky_spark02_4x4`
- [ ] 7. PIE で 4 か所を通して確かめ、実装記録 07・08 と作業一覧を直して項目 33 を閉じる
  - 変更予定: `.claude/implementation-records/07-*`・`08-*`、`.claude/roadmap.md`、`.claude/references/handover.md`

## 次にやること

ステップ 1。`pak_reference_2` の対象 4 つの cook の JSON から、GPU のエミッタ（`ParticleModuleTypeDataGpu`）の `props.ResourceData` と `props.EmitterInfo` を読み出し、本作の今の粒子（`/Game/DD/...`）の値と並べた表をこの記録に作る。どの行が原作のデータで決まり、どれが決まらないか（＝収録にまとめる候補）をここで決める。

## 決定事項

- 2026-09-21: 原作の cook の JSON には GPU のエミッタの `ResourceData` が**そのまま入っている**（`Fracture_concrete_3` の `ParticleModuleTypeDataGpu_0` で確認: `QuantizedColorSamples` 8 点・`QuantizedMiscSamples` 256 点・`ColorScale`/`ColorBias`/`MiscScale`/`MiscBias`/`SubImageSize`/`SizeBySpeed`/`RotationRateScale`/`DragCoefficientBias`/`ScreenAlignment` ほか）。これが GPU のエミッタの見え方の正本なので、焼き込みの表（`_table_distribution`）でなくこちらから組む。`DustTrail` の色が表 1 → 0.36 に対し cook は一定の 0.078 という既知のずれも、`ColorBias` = 0.0784 として実際に入っていた。
- 2026-09-21: 本家の収録には頼らない（作業一覧の大目標 3 の節の頭。移動が遅すぎて該当の場面に届かない）。コードで決まらなかったものだけをステップ 7 で 1 回の収録にまとめるか、決まらない理由を書いて閉じる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。エディタは起動していなくてよい（ステップ 1 は `pak_reference_2` の JSON を読むだけ）。
- ステップ 3 以降は `python Tools/dd/prepare_stage.py` と `dd_particles` の作り直し（エディタの Python）が要る。GPU のエミッタの表を書き換えたら、エディタで `Values` の数と `EntryCount × EntryStride` を比べて確かめる（症状索引「粒子が出て 1 秒ほどで `Array index out of bounds`」）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目は C++ を変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
