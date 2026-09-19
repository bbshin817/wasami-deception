---
title: Zone 2 の Matron（ボスワサミ）
sources:
  - Content/Python/wasami_tools/pipeline/dd_boss.py
  - SourceArt/Wasami/boss_wasami.glb
updated: 2026-09-19
---

# Zone 2 の Matron（ボスワサミ）

## 役割
本家の Zone 2 の中ボス `BP_06_Matron_MiniBoss`（最新版 `pak_reference_2`）を、大きいボスワサミとして置く（作業一覧の項目 11）。**作っている途中**: いまあるのはボスワサミの素材の取り込みだけ。Matron のアクタ・アニメの再生・視界コーン 2 種・Zone 2 への配置はこれから（進捗記録 `20260919-matron`）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_wasami_boss()`（01 記録）→ `dd_boss.import_all()`。戻り値 `textures` 3 / `materials` 1 / `meshes` 1 / `animations` 3、`idle_head_cm`（Idle の最初のコマの頭の骨の高さ。拡縮 1 で 128.2 cm）。
- `dd_boss.prepare()`: 前処理した glb を書き、（役ごとの長さ、Idle の最初のコマの頭の骨の高さ〈m〉）を返す。`prepared_file()` がその場所。

## 内部構造と処理の流れ

### 原本
`SourceArt/Wasami/boss_wasami.glb`（ユーザーのモデル。原本は git の外の `tmp/boss_wasami.glb`、2026-09-19 に写した。Git LFS）。Blender 4.5 の glTF で、作りは敵ワサミの `enemy_wasami_v3.glb` と同じ: シーンの根 `target_character` の子にメッシュ `output_unwrapped` と骨盤、スキン 1（骨 28、名前も同じ、`pelvis` が根、ルートの骨は無い）。キーの刻みも同じ（骨盤は 1/24 s と 2/30 s から、ほかの骨は 2/30 s から 30 fps）なので、敵の `dd_enemy._content_frames`・`_sample` がそのまま使える。メッシュ 95,427 頂点・100,128 三角形、高さ 1.70 m・幅 2.22 m（腕を広げた基準姿勢）、正面は glTF の +Z（UE では +Y）。材質 1（`BakedMaterial`）、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²。アニメ 7: `Long_Breathe_and_Look_Around`・`Alert`・`Lower_Weapon_Look_Raise`・`Walking_Scan_with_Sudden_Look_Back`・`Walking`・`Running`・`restpose`。

### 前処理（`prepare`）
- 役（`ROLES`。本家の `Matron_MiniBoss_AnimBP` が流すもの）: `Idle` = `Long_Breathe_and_Look_Around`（`loop`）、`Alert` = `Alert`（`closed`）、`Detected` = `Lower_Weapon_Look_Raise`（`once`）。各アニメを `dd_enemy._content_frames`（2/30 s から骨盤以外の骨の最後のキーまで）で 30 fps に標本化し直し、0 から並べる。
- ループの閉じ方は最初と最後のキーの差で決めた（2026-09-19）: `Long_Breathe_and_Look_Around` は最後のキーが最初の姿勢から 0.33° 外れ、最後の 1 コマの動きが 0.29° なので、敵の `loop` と同じく 1 コマ足りない形 → 最初のキーを最後の後に置く（`loop`）。`Alert` は最後のキーが最初の姿勢と 0.03° しか違わず、1 コマの動きが 4° 前後なので、既に閉じている → そのまま（`closed`。足すと 1 コマ止まる）。
- 使わないもの（`SKIPPED`）: `Walking`・`Running`・`Walking_Scan_with_Sudden_Look_Back`（本家の Matron は動かない）、`restpose`。役も `SKIPPED` にも無いアニメは、警告してそのままの名前で取り込む（敵と同じ）。
- メッシュの節とメッシュの名前を `SK_WasamiBoss` にし、`Intermediate/Pipeline/wasami/boss/WasamiBoss.glb` に書く（ファイル名をアセットの名前にしない理由は 07 記録の「取り込み」の 4）。
- Idle の最初のコマの `head` の骨の高さを測って返す（1.282 m。クリップの中で 1.243〜1.287 m。`head_end` 1.605 m、骨盤 0.619 m。敵ワサミの Idle より前かがみ）。Matron の大きさを決める材料（下の「原作データの根拠」）。

### 取り込み（`import_all`）
1. `dd_enemy._extract_textures(SOURCE, PREPARED_DIR, FOLDER, "T_WasamiBoss_")`: 敵と同じ設定で `T_WasamiBoss_<BaseColor|MetallicRoughness|Normal>`。
2. 材質: 敵のマスター `M_DD_WasamiGltf` を読み（無ければ `dd_enemy._build_master` で作る）、インスタンス `MI_WasamiBoss` にテクスチャ 3 枚を入れる。マスターは作り直さない（敵の取り込みのもの）。
3. `dd_enemy._import_model(instance, prepared_file(), FOLDER, MESH, "A_WasamiBoss_", 役)`: `PL_Wasami_Skeletal` で `/Game/Wasami/Boss` へ置き換えで取り込み、スロットにインスタンスを入れ、3 役のアニメがあるかを確かめる。インスタンスとメッシュとフォルダを保存する。

## 作るアセット

| パス | 中身 |
|---|---|
| `/Game/Wasami/Boss/SK_WasamiBoss` | スケルタルメッシュ（範囲の中心 (0, 0, 85)・広がり (111, 41, 85) cm。正面 +Y）。スロット 1 に `MI_WasamiBoss` |
| `/Game/Wasami/Boss/SK_WasamiBoss_Skeleton`・`_PhysicsAsset` | 骨格（28 本）と取り込みが作る物理アセット |
| `/Game/Wasami/Boss/A_WasamiBoss_Idle` | 11.300 s（339 コマ。ループを閉じた） |
| `/Game/Wasami/Boss/A_WasamiBoss_Alert` | 4.000 s（120 コマ） |
| `/Game/Wasami/Boss/A_WasamiBoss_Detected` | 5.200 s（156 コマ。1 回） |
| `/Game/Wasami/Boss/T_WasamiBoss_BaseColor`・`_MetallicRoughness`・`_Normal`、`MI_WasamiBoss` | テクスチャ 3 と材質のインスタンス（親 `/Game/Pipeline/Materials/M_DD_WasamiGltf`） |

## 原作データの根拠
- 役: 本家の `Matron_MiniBoss_AnimBP`（`pak_reference_2/_assets/DDeception/Content/Animation/Enemies/Nurse/Matron/MiniBoss/`）の状態機械の `Idle`（13.33 s ループ）・`Alert`（8.0 s ループ）と、`BP_06_Matron_MiniBoss` の `Player Spotted` が流す `DD_Matron_Zone_02_Detected_Montage`（1.3333 s、自動のブレンドアウトなし）。本作のアニメの割り当ては `.claude/references/enemy-wasami-motions.md` の「ボスワサミ」。
- 大きさの材料: 本家の `SK_Matron`（`pak_reference_2/_meshes_gltf/Animation/Enemies/Nurse/Matron/SK_Matron.gltf`）は高さ 2.276 m、基準姿勢の `head` の骨 1.819 m。レベルの `SkeletalMesh` の拡縮 5、メッシュの原点は床から −107 cm（Matron の根 z −50.22 + 部品の相対 z −57）。長い視界コーンは z 676（Matron の前 166 cm）。

## 依存関係
- `pipeline/dd_enemy.py`（`BONES`・`RATE`・`_content_frames`・`_sample`・`_key`・`_extract_textures`・`_build_master`・`_import_model`・`MASTER`）、`gltf.py`、`dd_assets.material_instance`。
- 使う側: これから（Matron のアクタ）。

## 既知の制約・注意点
- 敵ワサミを取り込み直してマスター `M_DD_WasamiGltf` を作り直しても、`MI_WasamiBoss` はテクスチャを上書きしているので変わらない。
- 本家の `Idle` ↔ `Alert` の間の切り替えのクリップ（各 0.8 s）に当たるものは原本に無い。

## 変更履歴
- 2026-09-19: 初版。ボスワサミの素材の取り込み（`dd_boss.py`、原本 `boss_wasami.glb`）を記録（作業一覧の項目 11 のステップ 1）
