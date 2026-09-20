---
title: 破壊と粒子の見え方を原作のデータどおりにする（作業一覧の項目 33）
status: 進行中
branch: main
base: ca4891f
started: 2026-09-21 06:53
updated: 2026-09-21 23:15
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
- [x] 4. 独房の粒子 4 材質は焼き込みのシェーダーと命令まで一致（直すところ無し）。`Fracture_dark_slow` は `Whisps_trans2`（`whispOne_Master_directional`）を使うので、`_lit_particle` が半透明のライティングの値を cook の書き出しから写すようにした（08 記録）
- [x] 5. ナースの扉突きの塵 `P_06_NurseDoorHit` の材質を原作のデータで直した。whisp の `Base`（と `_directional` の `Normal`）は段階を混ぜる SubUV ではなく素の `TextureSampleParameter2D`、落としていた `Radius` はカメラの近くの薄め（`SphereMask` hardness 10 % の `1 −`）だった（01・08 記録）
- [ ] 6. 破片の材質 `DebrisMaster` を原作のデータで確かめて直す（ステップ 5 で見つかった: 原作の `Base Map`・`Normal Map` は `TextureSampleParameterSubUV` で、本作は段階を混ぜず〈原作の書き出しは `bBlend` を書いていない = 既定の真〉、原作がつなぐ `ParticleMacroUV` の UV も作っていない。焼き込み `Tools/dd/cooked_shaders.py "Fragments/DebrisMaster."` の標本の数で混ぜているかを決める）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_gimmicks.py` の `_build_debris`・`_sub_uv`、`/Game/Pipeline/Materials/M_DD_Debris`
- [ ] 7. 除細動器の放電 `P_06_Defib` の `thander`（`M_ky_spark02_4x4`）を原作のデータで確かめて直す
  - 変更予定: `/Game/DD/.../P_06_Defib`、`M_ky_spark02_4x4`
- [ ] 8. PIE で 4 か所を通して確かめ、実装記録 07・08 と作業一覧を直して項目 33 を閉じる
  - 変更予定: `.claude/implementation-records/07-*`・`08-*`、`.claude/roadmap.md`、`.claude/references/handover.md`

## 残りのステップに効くこと（ステップ 1〜5 で分かった）

- 項目 33 の残りの対象（`P_06_Defib`）に **GPU のエミッタは無い**ので、ステップ 7 は材質の話だけ。
- **cook の書き出しは 367 の材質のどれにも `BaseColor`・`Opacity`・`Roughness`・`OpacityMask` を持たない**（一律に落ちる）。その 4 つが無いことは「つないでいない」の証拠にならない。**ただし残った式（`Expressions`）の型と設定は当てになる**（`TextureSampleParameter2D` / `ParticleSubUV` / `TextureSampleParameterSubUV` が書き分けられている）ので、**式の正本は「書き出しに残る式の型 → 焼き込みのシェーダー」の順**（`python Tools/dd/cooked_shaders.py "<パスの一部>." --show N`）。
- 材質インスタンスの焼き込みは親と同じ式なので（`Whisps_additive` は静的な置き換えを持つが uniform の並びは親と同じ）、**マスターの式はどちらのシェーダーからでも読める**。ベースパスは影のパスより多くを見せる（色・法線も出る）。
- 半透明のライティングの値は `_lit_particle` が書き出しから写す。ライティングありの半透明の材質を足すときは `_lit_particle(mat, <原作のパス>)` を呼ぶだけでよい。

## 次にやること

ステップ 6。破片の材質 `DebrisMaster` の 2 枚のテクスチャ（`Base Map`・`Normal Map`）を、焼き込みの標本の数と書き出しの `ParticleMacroUV` の接続で確かめて直す。組み直しは `dd_gimmicks.import_doors_busted`（`Tools/ue_remote.py` から。MCP の `describe_toolset` は `WasamiDDTools` の道具を返さない）。

## 決定事項

- 2026-09-21: 本家の収録には頼らない（作業一覧の大目標 3 の節の頭。移動が遅すぎて該当の場面に届かない）。コードで決まらなかったものだけをステップ 8 で 1 回の収録にまとめるか、決まらない理由を書いて閉じる。
- 2026-09-21: 原作が engine の材質関数を呼んでいても、**その中身を展開した本作の式と焼き込みの命令が同じなら展開したままにする**（`M_Radial_Gradient` の `Gradient/RadialGradient`）。UE 5.8 の関数の中身は Python から読めず、写すと 4.21 との違いを持ち込みかねない。焼き込みと一致している方を正とする。
- 2026-09-21: 原作が定数をつないでいる入力でも、**焼き込みで既定値に畳まれているならつながない**（`Squib_one` の `Metallic` = 0）。
- 2026-09-21: **書き出しに残る式の型を、推定より先に読む**。`whispOne_Master_directional`・`_amb` の `Base` を段階を混ぜる SubUV だと推定していたが、書き出しは `TextureSampleParameter2D` と書いていて、焼き込みも 1 画素につき 1 回しか読んでいなかった（`Squib_one` の法線は `ParticleSubUV`、`DebrisMaster` の 2 枚は `TextureSampleParameterSubUV` と書き分けられている）。

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
- エディタでの確認: ステップ 1〜3 で GPU の 5 エミッタの差が消え、PIE（Zone 1）で扉の破片が出るのを見た。ステップ 4 で `import_dd_gimmicks` を通し、7 つのマスター材質の設定が cook の書き出しと一致するのを読み出して確かめた（`M_DD_WhispDirectional`・`M_DD_WhispAmb`・`M_DD_Debris`・`M_DD_Squib`・`M_DD_NurseSparks`・`M_DD_BvfxSpark`・`M_DD_BvfxRadialGradient`）。ログに材質のコンパイルのエラーは無い。**見え方はまだ PIE で見ていない**（Zone 2 の独房・棘の塵はステップ 7 の通しで見る）。
