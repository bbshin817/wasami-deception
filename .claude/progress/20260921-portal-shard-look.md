---
title: 作業一覧の項目 34（ポータルと特殊シャードの見え方）
status: 進行中
branch: main
base: 3226bb8
started: 2026-09-21 09:21
updated: 2026-09-21 11:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（上限 30 KB） -->

# 作業一覧の項目 34（ポータルと特殊シャードの見え方）

## 依頼

`.claude/roadmap.md` の大目標 3 の項目 34。大目標 2 の項目 13（脱出のポータル）・10（特殊シャード）で **cook で式が消えた材質を推定で組んだ**ところを、原作の粒子の型データと残っているコンパイル済みシェーダーから本家どおりにし、作っていない粒子を足す。

完了の条件（作業一覧より）:
- ポータル: まだ無い `PPP_PortalAppear`・`PPP_PortalAppear_Lock` を粒子の型データ（`ResourceData`）と残っているシェーダーから**作るか、作らない理由を書いて閉じる**。渦 `M_00_Portal_Vortex`・ロゴの親 `M_00_Portal_Monkey` は推定なので原作のデータで詰める。
- 特殊シャード: 結晶の屈折・閃光の不透明度・地図の印の色を材質とコンパイル済みシェーダーの式で決める。
- **コードで決まらなかったものだけ**を 1 回の収録にまとめる（本家の実機は移動が遅すぎて 1 回の収録で 1 か所しか回れない）。

## 計画

- [x] 1. 渦の材質 `M_00_Portal_Vortex` と 8 つのインスタンスを原作のデータどおりにした（差は 3 つで、式は一致していた）
- [x] 2. ロゴの親 `M_DD_PortalLogo`（本家 `M_00_Portal_Monkey`）を原作のデータどおりにした（式 12 → 9、焼き込みは不変）。鍵 `M_00_Portal_Lock` は照合だけで直しなし
- [x] 3. `PPP_PortalAppear`・`_Lock` を**作ると決めた**（下の「決定事項」に理由。親 2 つの式も焼き込みから全部読めた）
- [ ] 4. 親 `PPP_Particles_lit`・`PPP_Particles_lit_fogged` と 5 つのインスタンス・テクスチャ 5 枚を作る ← 次
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_gimmicks.py`（材質の節を足す）、`.claude/implementation-records/08-stage-gimmicks.md`
  - 作るもの: テクスチャ `ThirdParty/PyroParticlePack/Textures/Particles/` の `Smoke_8x4_01`・`FireBlast_8x4_03_`・`FireBlast_8x4_02`・`Flames_8x4_03`・`Spark_01`（どれも `pak_reference_2` に PNG がある。`SphericalNormal_001` は下の「親の式」のとおりつながっていないので要らない）→ 親 2 つ（`dd_assets.estimated_materials` の master + base の形。`/Game/Pipeline/Materials/M_DD_PPPParticlesLit`・`M_DD_PPPParticlesLitFogged`）→ インスタンス 5 つ（`PPP_Particle_01_Smoke_2_A_DOF`・`PPP_Particles_01_Blast_02_A_DOF` は fogged の子、`PPP_Particles_01_Blast_01_Add`・`PPP_Particles_01_Burnout_01_A1`・`PPP_Particles_01_Sparks_01_A` は lit の子）
  - 注意: インスタンスは `BasePropertyOverrides`（`PPP_Particles_01_Burnout_01_A1` だけ `BLEND_Additive`、ほかは `BLEND_Translucent`、どれも `OpacityMaskClipValue` 0.3333）を持つが `dd_assets.estimated_materials` はそれを書かない。`dd_assets.base_property_overrides(mic, rel, version)` を後から当てるか、ヘルパーに足す
- [ ] 5. `PPP_PortalAppear`・`_Lock` を `dd_particles.particle_system(rel, 2)` で組み、`AWasamiPortal` の鍵のとき・開くときにつなぐ
- [ ] 6. 閃光の材質 `M_ky_primitiveColor`・`M_ky_lensFlare02` の不透明度をコンパイル済みシェーダーの式で確定して直す
- [ ] 7. 結晶 `m_crystal` と地図の印 3 つ（`M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`）をシェーダーと突き合わせ、差があれば直す
- [ ] 8. PIE でポータル（鍵 → 開く）と特殊シャード（出現・閃光・取得・地図の印）を見て確かめ、実装記録 08・16 と作業一覧・handover を直して項目 34 を閉じる

## 次にやること

ステップ 4。上の「作るもの」の順に `dd_gimmicks.py` へ材質の節を足して走らせる。親の式は下の「原作の親 2 つの式」のとおり組み、出来た材質の命令数を `MaterialEditingLibrary.get_statistics` で見て、**原作のインスタンス自身の焼き込み**（`python Tools/dd/cooked_shaders.py "DOFFED/PPP_Particle_01_Smoke_2_A_DOF."` など。静的スイッチを持つインスタンスは自分のシェーダーマップを持つ）と枝の数が合うことを確かめる。

## 決定事項

- 2026-09-21（ステップ 3）: **`PPP_PortalAppear`・`_Lock` は作る**。理由: (1) cook は粒子の分布を**焼き込みの表ごと**残していて（`Table` の `Values`・`Op`・`TimeScale`）、`dd_particles.particle_system` はまさにそれを読んで Cascade を組み直す仕組みが既にある（GPU エミッタも `ResourceData` から）。(2) 効くエミッタは少ない ── `PPP_PortalAppear` は 6 つのうち 3 つ（`Smoke`・`blast2`・GPU の `Sparks`）、`_Lock` は 2 つ（`Smoke`・`blast2`）だけが有効で、残りは `RequiredModule.bEnabled` が false。(3) 親 `PPP_Particles_lit`・`PPP_Particles_lit_fogged` の式は cook で消えているが、**焼き込みから全部読めた**（下）。(4) 要るテクスチャ 5 枚は `pak_reference_2` に PNG がある。(5) `PPP_Radial_Gradient_Doffed` は項目 2（パワー）で作り済み（`M_DD_PPPRadialGradient`）。
- 2026-09-21（ステップ 3）: **`_Lock` は `PPP_PortalAppear` からライトの module（`ParticleModuleLight_0`）を抜いたもの**で、ほかの違いは `bEnabled` と `EmitterLoops` の有無だけ（書き出しの export は 102 個 対 101 個）。2 つとも同じ 6 つの材質を引く。
- 2026-09-21（ステップ 2）: **式の数が合わないときは、焼き込みが同じになる形のうち安い方を採る**。数は式自身の定数（`Multiply`・`Add` の B）に持たせる。
- 2026-09-21: 項目 33 と同じ進め方 — **本家の実機の収録に頼らず、原作のブループリント・アセットの値・cook の `ResourceData`・コンパイル済みシェーダーの式で決める**（作業一覧の大目標 3 の節の頭、2026-09-21 のユーザーの回答）。

## 原作の親 2 つの式（ステップ 3 で焼き込みから読んだ。ステップ 4 で組む）

読んだもの: `python Tools/dd/cooked_shaders.py "PyroParticlePack/Materials/PPP_Particles_lit." --show 5`（翻訳の基本パスの画素シェーダー。スプライトの頂点ファクトリなので `DynamicParameter` が補間子 TEXCOORD1 に乗る）と `..._lit_fogged.`、`"DOFFED/PPP_Particle_01_Smoke_2_A_DOF."`（静的スイッチ `UseAlphaClip` = false の枝）。

**`PPP_Particles_lit`**（`BLEND_Translucent`・既定の Lit・`bUsedWithParticleSprites`・`bEnableSeparateTranslucency` = false → UE 5 の `MTP_BEFORE_DOF`）:

- `Tex` = `TextureSampleParameterSubUV`「ParticleTexture」（既定 `Flames_16x4_01`）、`PC` = `ParticleColor`、`DP` = `DynamicParameter`（`alphaClipMultiplier`・`GlowMultiplier`・`DephFadeDistance`・`NearCameraFade`、既定 (0, 0, 0, 1)。使うのは x・y・w で z は死んでいる）
- `camFade` = `CameraDepthFade`（`FadeOffset` = スカラー「Camera Fade」既定 50、`FadeLength` = `DP.NearCameraFade`）= `saturate((PixelDepth − 50) / NearCameraFade)`
- `par` = `Lerp(1, Power(OneMinus(Fresnel(Exponent 0.5, BaseReflectFraction 0.05)), 2), スカラー「UseParallelFade」既定 0)`（焼き込みは `0.95·sqrt(1 − max(N·V, 0)) + 0.05` → `1 − x` → `max(·,0)` → 二乗。`max` が付くのは UE の `Power` が `pow(max(base,0), e)` を出すから）
- `a0` = `par × camFade`
- `colour` = `Tex.rgb × PC.rgb`
- **BaseColor** = `colour × a0`（シェーダーの `mul_sat` の saturate は基本パス側）
- **EmissiveColor** = `Clamp(colour × DP.GlowMultiplier × a0, Min 0, Max 自分自身) × スカラー「Emissive Multiplier」既定 1`（書き出しの `Clamp_16` は焼き込みでは `min(x, max(x, 0))` = 恒等。名前が残っているのでそのまま作る）
- **Opacity** = `a0 × DepthFade(Opacity = alphaTerm, FadeDistance = スカラー「FadeDistance」既定 0)`（`dd_assets.depth_faded_opacity` の形。焼き込みのユニフォームに `Max(FadeDistance, 1e-05)` が出るのが `DepthFade` の印）
  - `alphaTerm` = 静的スイッチ「UseAlphaClip」（既定 true）
    - **true**（書き出しの A = `Multiply_37`）: `max(1 − pow(max((1 − Lerp(Tex.a, Tex.g, スカラー「UseGreenForAlphaClip」既定 0)) × max((1 − PC.a) × DP.alphaClipMultiplier, 1), 0), Tex.a), 0) × PC.a`
    - **false**（B = `Multiply_35`）: `Tex.a × PC.a`
- **つながっていないもの**: Normal。`SphericalNormal_001` の `TextureSample`（`ParticleMacroUV` が UV）とスカラー「NormalStrength」は式としては残っているが、焼き込みは法線が既定 (0,0,1)（`normalize(Normal × View.NormalOverrideParameter.w + .xyz)` だけ）で、法線のサンプルもユニフォームの `NormalStrength` も無い。**式は作らずに置く**（渦の `Albedo_1` と同じ扱いでもよい）

**`PPP_Particles_lit_fogged`**: **同じ式で、3 か所だけ定数に畳まれている** ── 「Camera Fade」→ 定数 50、「FadeDistance」→ 定数 25（焼き込みは `saturate((SceneDepth − PixelDepth) × 0.04)`）、「Emissive Multiplier」**無し**（Emissive は `Clamp_16` そのもの）。`bEnableSeparateTranslucency` は既定の true のまま → UE 5 の `MTP_AFTER_DOF`。パラメータは「UseParallelFade」「UseGreenForAlphaClip」「ParticleTexture」と静的スイッチ「UseAlphaClip」だけ。

**インスタンス 5 つ**（`ParticleTexture` と `UseAlphaClip` が主な違い）:

| インスタンス | 親 | ParticleTexture | UseAlphaClip | ほか |
| --- | --- | --- | --- | --- |
| `PPP_Particle_01_Smoke_2_A_DOF` | fogged | `Smoke_8x4_01` | false | Translucent |
| `PPP_Particles_01_Blast_02_A_DOF` | fogged | `FireBlast_8x4_03_` | true | `UseGreenForAlphaClip` 0、Translucent |
| `PPP_Particles_01_Blast_01_Add` | lit | `FireBlast_8x4_02` | （上書きなし = 親の true） | `FadeDistance` 100、Translucent、静的置換なし |
| `PPP_Particles_01_Burnout_01_A1` | lit | `Flames_8x4_03` | false | `FadeDistance` 0、**Additive** |
| `PPP_Particles_01_Sparks_01_A` | lit | `Spark_01` | false | Translucent |

どれも `RefractionDepthBias` 0（屈折はつながっていないので効かない）と `OpacityMaskClipValue` 0.3333 を持つ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 原作の在り処: 粒子 = `pak_reference_2/_assets/DDeception/Content/ThirdParty/PyroParticlePack/Particles/PPP_PortalAppear.json`・`PPP_PortalAppear_Lock.json`、親 = 同じ `Materials/PPP_Particles_lit.json`・`PPP_Particles_lit_fogged.json`、インスタンス = `Materials/Particles/` と `Materials/Particles/DOFFED/`、テクスチャ = `Textures/Particles/`（PNG は `pak_reference_2/DDeception/Content/...` 側）。
- **書き出しの export は `outer` で辿る**（`ParticleLODLevel_0`・`_2` のような名前は emitter ごとに重なるので、`outer + '.' + name` を鍵にしないと別の emitter の LOD を掴む）。参照は `PPP_PortalAppear.ParticleSpriteEmitter_0.ParticleLODLevel_0` の形。
- 使う道具: `python Tools/dd/cooked_shaders.py "<パスの一部>." [--show N]`（`--show` なしで並ぶ表の ps_5_0 のうち、texture3d と `sample_l` の scene depth を引く 200 行台のものが翻訳の基本パス）、`python Tools/dd/gpu_emitters.py`（GPU のエミッタの型データと焼き込みの突き合わせ）。
- 渦・ロゴの材質の読み方（ステップ 1・2）: 書き出しの `exports[0].props.Expressions` に原作の式の数と生き残りの名前、`exports[1:]` に生きている式の既定とグループ。**`props` は材質の入力の一部しか残さない**ので「書き出しに無い＝つながっていない」とは読まない。
- **材質の中身を UE 5.8 の Python から読む道**: 式の一覧は出せない。使えるのは `MaterialEditingLibrary.get_num_material_expressions` と `get_statistics`、インスタンス側の `get_*_parameter_names` / `get_material_instance_*_parameter_value`。パラメータのグループは保存した `.uasset` の名前表を見る。
- **結晶の `distortion_normal` は項目 28 のステップ 13（2026-09-21）で入れ済み**。作業一覧の項目 34 の「今は」の行はそれより前の記述なので、ステップ 7 では現物を見て判断する。

## 検証

- check_records: ステップ 2 で通した（01・08 のハッシュを更新）
- C++ ビルド: ステップ 5 で `AWasamiPortal` を変えるときだけ要る
- エディタでの確認: ステップ 2 までは材質の焼き込みが変わらないことを `get_statistics` で確かめた。**PIE で見るのはステップ 8**
