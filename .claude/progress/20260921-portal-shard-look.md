---
title: 作業一覧の項目 34（ポータルと特殊シャードの見え方）
status: 進行中
branch: main
base: 3226bb8
started: 2026-09-21 09:21
updated: 2026-09-21 15:05
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
- [x] 4. 親 2 つ（`M_DD_PPPParticlesLit`・`_LitFogged`）・インスタンス 5・テクスチャ 6・`PPP_Radial_Gradient_Doffed` の ThirdParty 版を作った。パラメータ・既定・テクスチャ・静的スイッチ・`BasePropertyOverrides` は書き出しと一致（実装記録 08）
- [x] 5. `PPP_PortalAppear`・`_Lock` を組んで `import_portal` に足した（`dd_particles` に `RDO_Extreme` の表を足した。GPU の `Sparks` の焼き込みは原作と色の ±1/255 だけ違う）
- [x] 5b. `AWasamiPortal` が `BeginPlay` で 2 つの部品に粒子を入れ、`AWasamiSpeedBarrier::BreakIfBoosting` が本家どおり `P_ky_impact2` より先に `PPP_PortalAppear` を 0.3 倍で出すようにした（テスト 4 件が通る。08 記録）
- [ ] 6. 閃光の材質 `M_ky_primitiveColor`・`M_ky_lensFlare02` の不透明度をコンパイル済みシェーダーの式で確定して直す
- [ ] 7. 結晶 `m_crystal` と地図の印 3 つ（`M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`）をシェーダーと突き合わせ、差があれば直す
- [ ] 8. PIE でポータル（鍵 → 開く）と特殊シャード（出現・閃光・取得・地図の印）を見て確かめ、実装記録 08・16 と作業一覧・handover を直して項目 34 を閉じる

## 次にやること

ステップ 6（閃光の材質の不透明度）。特殊シャードの閃光が引く `M_ky_primitiveColor`・`M_ky_lensFlare02` の Opacity を `python Tools/dd/cooked_shaders.py "<パスの一部>."` の焼き込みで確定し、推定と違えば pipeline（`Content/Python/wasami_tools/pipeline/` の該当ファイル。まず現物の式がどこで組まれているかを grep する）を直して取り込み直す。式の数が合わないときは「決定事項」の 1 つめの決め方に従う。

## 決定事項

- 2026-09-21（ステップ 2）: **式の数が合わないときは、焼き込みが同じになる形のうち安い方を採る**。数は式自身の定数（`Multiply`・`Add` の B）に持たせる（ステップ 6・7 でも同じ）。
- 2026-09-21: 項目 33 と同じ進め方 — **本家の実機の収録に頼らず、原作のブループリント・アセットの値・cook の `ResourceData`・コンパイル済みシェーダーの式で決める**（作業一覧の大目標 3 の節の頭、2026-09-21 のユーザーの回答）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **`Tools/ue_remote.py` のスクリプトは、pipeline を直したら先に読み込み直す**（`import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)`）。しないと古いモジュールのまま走り、直したはずの例外がそのまま出る。
- 使う道具: `python Tools/dd/cooked_shaders.py "<パスの一部>." [--show N]`（`--show` なしで並ぶ表の ps_5_0 のうち、texture3d と `sample_l` の scene depth を引く 200 行台のものが翻訳の基本パス）。
- 材質の読み方（ステップ 6・7 でも同じ）: 書き出しの `exports[0].props.Expressions` に原作の式の数と生き残りの名前、`exports[1:]` に生きている式の既定とグループ。**`props` は材質の入力の一部しか残さない**ので「書き出しに無い＝つながっていない」とは読まない。
- **材質の中身を UE 5.8 の Python から読む道**: 式の一覧は出せない。使えるのは `MaterialEditingLibrary.get_num_material_expressions` と `get_statistics`、インスタンス側の `get_*_parameter_names` / `get_material_instance_*_parameter_value`。パラメータのグループは保存した `.uasset` の名前表を見る。
- **`get_statistics` の命令数は原作の焼き込みと突き合わせられない**（ステップ 4 で確かめた）: 返るのはローカル頂点ファクトリのベースパスで、`ParticleColor` は 1・`DynamicParameter` は既定 (0, 0, 0, 1) に畳まれる（粒子の式だけの材質と定数だけの材質が同じ 337 命令・補間子 0 になる）。粒子の式を含む枝の差は原作より小さく出るので、照合は**パラメータ・既定・静的スイッチ・上書きの一致**と、**枝の上下関係**（スイッチを倒さない子は親と同じ命令数、真の側は偽の側より重い）で見る。
- **結晶の `distortion_normal` は項目 28 のステップ 13（2026-09-21）で入れ済み**。作業一覧の項目 34 の「今は」の行はそれより前の記述なので、ステップ 7 では現物を見て判断する。

## 検証

- check_records: ステップ 5b で通した（08 のハッシュを更新）
- C++ ビルド: ステップ 5b で `python Tools/editor_cycle.py` が成功し、エディタは開き直して応答している（MCP も再接続済み）
- 自動テスト: `AutomationTestToolset` の `DiscoverTests` → `RunTestsByFilter("StartsWith:Wasami.Portal+StartsWith:Wasami.SpeedBarrier")` で 4 件とも合格（`Wasami.Portal.Actor` に粒子のテンプレートの確認を足した）
- エディタでの確認: ステップ 5 で `import_portal` を回し、テクスチャ 11・材質 23・粒子 2 が出来て `/Game/DD`・`/Game/Pipeline` を保存した。**PIE で見るのはステップ 8**
