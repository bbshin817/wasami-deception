---
title: 祭壇の見た目（作業一覧の項目 31）
status: 進行中
branch: feature/altar-look
base: 63bab43
started: 2026-09-20 04:40
updated: 2026-09-20 05:15
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 祭壇の見た目（作業一覧の項目 31）

## 依頼

2026-09-20 の有人セッションの指摘「シャードリングの祭壇の見た目が本家と異なる気がします。私が記憶しているのは、MonkeyBusinessの祭壇」。本家の病院の祭壇はホテルと同じ `BP_01_Statue`・`ring_statue`・材質 `MM_00_Ballroom_Ring_Altar_Metal`（親 `MM_Main_Metal`）と水晶の球 `ring_statue_orb`（`m_crystal_Inst2`）。本作は祭壇・球・欠片（`M_ring_metal2`、親 `MM_Main_Substance_Fresnel`）の親が前処理のマスターに無く、`substance` の推定になっている（祭壇が青みの白、球が紫に光らない）。完了の条件は `.claude/roadmap.md` の項目 31（祭壇・球・欠片の材質をコンパイル済みのシェーダーの式で、置いたものは組み直さずに材質だけ作り直し、PIE で Zone 2 の祭壇を撮る。本家の実機との見比べはしない）。

## 計画

- [x] 1. 祭壇の金属 `MM_Main_Metal` を `M_DD_Metal` に組み、`MM_00_Ballroom_Ring_Altar_Metal` を同じパスで作り直した（2026-09-20。前処理 `MASTERS` の `metal`、`dd_stage` の `_build_metal`・`make_material` のその場の作り直し・`refresh_settings` の `materials_remade`。Zone 2 の祭壇に当たったまま真鍮色。01・08 記録）
- [ ] 2. 欠片の `MM_Main_Substance_Fresnel` をシェーダーから読んで推定のマスター `M_DD_SubstanceFresnel` を組み、同じ経路（`fresnel`）に載せて `M_ring_metal2` と書類の `MM_Shared_Secret_Folder` を作り直す
  - 変更予定: `Tools/dd/prepare_stage.py`、`dd_stage.py`、`paths.py`、`/Game/Pipeline/Materials/M_DD_SubstanceFresnel`、`/Game/DD/Meshes/Ring_Assets/ring_pieces/M_ring_metal2`、`/Game/DD/Materials/Shared/MM_Shared_Secret_Folder`
  - 読む: `python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Substance_Fresnel."`。インスタンスの値は `_materials.json`（`M_ring_metal2`: `Roughness Power` 1・`BaseReflectFractionIn` 0.246・`Fresnel ExponentIn` 5.79・`Fresnel Setting` (1.156, 0, 5)、書類: `Fresnel Setting` (1, 1, 1)）。
- [ ] 3. 球 `m_crystal_Inst2` を特殊シャードの推定 `m_crystal`（`dd_specials` の `M_DD_Crystal`。16 記録）の子にし、PIE で Zone 2 の祭壇（全回収の前の球つき）と欠片を撮って、記録を閉じる
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_specials.py`（`CRYSTAL_INSTANCES`）、ステージの経路（前処理で根 `m_crystal` を `crystal` にし、ステージの取り込みが `dd_specials` の結晶に任せる。作り手を 1 つにする）、`/Game/DD/Materials/Fords_Materials/m_crystal_Inst2`
  - 閉じる: 実装記録 01・08・16・18、作業一覧の項目 31 を完了・項目 28 の後回しの一覧の書類の縁の光の行を直す、handover、note の進捗記事（祭壇の GIF を差し替えるか）、feature/altar-look を main へマージ。

## 次にやること

ステップ 2: `python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Substance_Fresnel."` を読み（ベースパスの画素シェーダーの定数の束 cb3 の並びと、`Fresnel Setting` の xyz・`BaseReflectFractionIn`・`Fresnel ExponentIn` がどこに効くか）、`dd_stage.py` に `_build_substance_fresnel`（`paths.MASTER_FRESNEL` = `/Game/Pipeline/Materials/M_DD_SubstanceFresnel`）を足して、前処理の `MASTERS` に `fresnel` を足し、`prepare_stage.py` → `refresh_dd_stage_assets`（`materials_remade` 2 のはず）で作り直す。

## 決定事項

- 2026-09-20: 3 つの材質は、前処理（`prepare_stage.py` の `MASTERS`）が根で振り分け、ステージの取り込み（`dd_stage`）が作る経路に載せる（祭壇と欠片の材質は前処理の `CLASS_MATERIALS` でステージが作っているので、作り手を 1 つに保つ）。置いた祭壇・欠片・書類・球はアセットのパスで材質を指しているので、インスタンスを同じパスでその場で親を付け替えれば置き直さずに済む（`dd_assets.material_instance` と同じ作り） — 作業一覧の項目 31 の (4)。
- 2026-09-20: 3 ステップに分けてコミットするので作業ブランチ `feature/altar-look` で進める — `.claude/guides/git-workflow.md` の大規模改修（複数回のコミット）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは `L_Hospital_Zone2` を開いて応答している（2026-09-20 05:15、PIE なし・未保存なし）。祭壇は `ring_statue_2`（-8584, -983, 0）、ヨー 90（正面は +Y 側だが車が塞ぐ）。+X 側の (-8183, -982, 250)・ヨー 180 から撮ると球を掲げた像が見える（紫の灯で色は分かりにくいので、色は `CaptureAssetImage` のサムネイルで見た）。
- 作り直しの入口は `refresh_dd_stage_assets`（`dd_stage.refresh_settings`）: 親がステージのマスターで根の振り分けと違うインスタンスを同じパスで作り直す（`materials_remade`）。前処理を直したら先に `python Tools/dd/prepare_stage.py`。
- `MCP の CaptureViewport` の結果は大きすぎて会話に入らないので、保存されたファイルを `tmp/altar/decode.py <結果の txt> <png>` で PNG にして読む（`annotations` は全部 0 と `classFilter.refPath` 空で渡す）。

## 検証

- check_records: ステップ 1 で通した
- C++ ビルド: 不要（Python と材質だけ）
- ステップ 1: `M_DD_Metal` がコンパイルでき（画素 363 命令）、`refresh_settings` は `materials_remade` 1・ほかは変化なし、祭壇の部品の材質は同じパスのまま
