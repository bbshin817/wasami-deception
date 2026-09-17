---
title: 敵ワサミ（素体の素材）
sources:
  - Content/Python/wasami_tools/pipeline/dd_enemy.py
  - SourceArt/Wasami/enemy_wasami_v3.glb
  - SourceArt/Wasami/enemy_wasami_capture.glb
updated: 2026-09-18
---

# 敵ワサミ（素体の素材）

## 役割
本家のナース（`BP_06_ReaperNurse`）の代わりに Zone 1・2 を巡回し追う敵ワサミ。いまは**素材の取り込みだけ**（作業一覧の項目 4 のステップ 1）: ユーザーのモデルを、スケルタルメッシュと役の名前で引けるアニメにする。アニメの再生（`UWasamiEnemyAnimInstance`）と敵のアクタ（`AWasamiEnemy`）、AI・捕獲はこの記録に書き足していく。役とアニメの対応の決まりは `.claude/references/enemy-wasami-motions.md`。

## 公開インターフェース
- `WasamiDDTools.import_wasami_enemy()`（01 記録）→ `dd_enemy.import_all()`。戻り値 `textures` 3 / `materials` 2 / `meshes` 1 / `animations` 19。
- `dd_enemy.make_capture_source(old_glb)`: ユーザーの旧モデル（`tmp/enemy_wasami.glb`、git の外）から `SourceArt/Wasami/enemy_wasami_capture.glb` を書く（2026-09-18 に 1 回走らせた。旧 glb が変わらない限り再び走らせる必要はない）。
- `dd_enemy.prepare()`: 前処理した glb を書き、役ごとの（長さ、骨盤を動かした量）を返す。`prepared_file()` がその場所。
- 定数: `MESH` = `/Game/Wasami/Enemy/SK_WasamiEnemy`、`SKELETON` = `…_Skeleton`、`PHYSICS_ASSET` = `…_PhysicsAsset`、`ANIM_PREFIX` = `A_WasamiEnemy_`、`MATERIAL` = `/Game/Wasami/Enemy/MI_WasamiEnemy`、`MASTER` = `/Game/Pipeline/Materials/M_DD_WasamiGltf`、`ROLES`（下の表）。

## 内部構造と処理の流れ

### 原本
- `enemy_wasami_v3.glb`（30 MB。ユーザーの `tmp/enemy_wasami_v3.glb` の写し）: Blender 4.5 の glTF。根 `target_character` の子にメッシュ `output_unwrapped`（89,572 頂点・104,806 三角形、材質 1、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²）と `pelvis`。スキン 1（骨 28、根 `pelvis`、ルートの骨なし）、アニメ 16（どれも LINEAR）。
- `enemy_wasami_capture.glb`（259 KB）: 旧モデル（骨 22。名前は全部 v3 にある）の節と、`Backflip`・`sliding_rool`・`Stylish_Walk`・`run_fast_2` のキーをそのまま（旧 glb と同一なのを確かめた）。メッシュ・スキン・テクスチャは入れない（20 MB を LFS に入れないため）。`run_fast_2` は下の載せ替えの物差し。

### 前処理（`prepare`）
glb のキーは 30 fps の動きを 24 fps の場面に焼いたもので、骨盤だけ 24 fps と 30 fps のキーが混ざり、どの骨も 2/30 s から始まる（骨盤は 1/24 s から）。Interchange はコマの境目で終わらないアニメを取り込まない（「アニメーションの長さ … はインポート フレームレート 30 fps と互換性がありません」。16 本中 11 本が落ちた）ので、**全部を 30 fps のコマ（`RATE`）で 0 から標本化し直す**。中身は 2/30 s（`CONTENT_START_FRAME`）から、骨盤以外の骨の最後のキーまで。回転は slerp、書き出す前に四元数の符号を前のキーにそろえる。

役ごとの作り方（`ROLES` の 4 列目）:
- `loop`: 最初のキーを最後の後にもう 1 つ置いて閉じる（元のループは 1 コマ手前で止まっている。`Running` の最初と最後の差は 1 コマの動きの 0.35 倍、`Walking` は 1.5 倍、`run_fast_2` は 1.1 倍で、周期 = 中身 + 1 コマと読んだ）。
- `in_place`: 骨盤の水平（glTF の x と z）を最初のキーの値に止める（高さは残す）。
- `stun_loop`: 無名のモーション（`STUN`、10.04 s）の 0.967〜2.467 s（30 fps のコマ 29〜74、`STUN_LOOP_FRAMES`）。前屈して揺れる部分で、1.5 s 以上離れた姿勢の組のうち最もそろう組（関節の平均のずれ 3.3 cm）。最後の 15 コマ（`STUN_BLEND_FRAMES`、0.5 s）を、ループの始まりの前の動き（コマ − 45）へ smoothstep で混ぜ、最後のキーを最初のキーにする。
- `stun_recover`: 元のモーションでループの後に続く 2.467 s〜終わり（前屈のまま 4.8 s まで、8.0 s で直立、10 s まで落ち着く）。最初の 15 コマはループの続き（コマ 29 + k）から混ぜて入るので、最初のキーがループの最初のキーと同じ。元は終わりで骨盤が横へ 0.2〜0.3 m ずれるので、全体に smoothstep で骨盤の水平をずらし、終わりを `Idle` の最初の骨盤（x 0.004、z −0.022 m）に合わせる（足は合計 0.29 m 滑る）。
- 捕獲（`CAPTURE`）: `once` のみ。`_Retarget` で v3 の骨へ載せ替える。

`_Retarget`（両方にある `run_fast_2` で測る。`REFERENCE_ANIMATION`）: 2 つのモデルの骨は**同じ向きを指し、骨の軸まわりのひねりだけが違う**。各骨の場面での回転は「旧 × 骨ごとに一定の回転」で、その回転は時間で変わらない（測った値: 腕と鎖骨で最大 21.7°、手は向きも少し違う。ずれ 0.075°）。骨 b（親 p）のローカルの回転 r は `inverse(p の回転) · r · (b の回転)` にする。骨盤の位置は「v3 の休み + 0.9929 ×（旧 − 旧の休み）」（x・y・z 同じ係数を最小二乗で。ずれ 0.0000 m）。ほかの骨の移動は捨てて v3 の骨の長さを使う。ずれが `RETARGET_TOLERANCE`（0.5°・2 mm）を超えたら例外。**同じ対応を旧と v3 の `Running`・`Walking`・`BeHit_FlyUp` に当てると v3 の値と 0.08° 以内で一致する**（気絶のモーションは頭だけ 4°）ので、ユーザーのツールが v3 用に書き出すのと同じ形になる。

v3 の `restpose`（腕を広げた基準姿勢、0.083 s）は取り込まない（`SKIPPED`）。一覧に無いアニメが v3 に足されたら、警告を出して元の名前で `once` として取り込む。

| 役（`A_WasamiEnemy_…`） | 元 | 作り方 | 長さ | 骨盤を止めた量（x / z） |
|---|---|---|---|---|
| `Idle` | `Idle_11` | loop | 1.933 s | |
| `Idle_Alert` | `Idle_5` | loop | 1.900 s | |
| `Walk` | `Walking` | loop | 1.033 s | |
| `Run` | `Running` | loop | 0.667 s | |
| `Run_Nightmare` | `run_fast_2` | loop_in_place | 0.600 s | −0.01 / 2.60 m（4.6 m/s） |
| `Stun_Loop` | `01a0a88f-…` | stun_loop | 1.500 s | |
| `Stun_Recover` | `01a0a88f-…` | stun_recover | 7.567 s | 終わりを 0.19 / −0.23 m 寄せた |
| `Capture_1`・`_2`・`_3` | 旧 `Backflip`・`sliding_rool`・`Stylish_Walk` | once（載せ替え） | 2.133・2.767・3.533 s | 動かしたまま（前へ 0.0・5.7・2.1 m） |
| `Chase_PickUp` | `Female_Run_Forward_Pick_Up_Right` | in_place | 1.233 s | 0.00 / 4.02 m |
| `Chase_Charge` | `Male_Head_Down_Charge` | in_place | 0.533 s | 0.01 / 2.17 m |
| `Chase_VaultRoll` | `Parkour_Vault_with_Roll` | in_place | 2.100 s | −0.01 / 4.61 m（途中で横に最大 0.22 m） |
| `Chase_VaultLand` | `Vault_and_Land` | in_place | 3.067 s | 0.64 / 1.05 m（始めの骨盤が 1.67 m と高い） |
| `Chase_RunFast` | `run_fast_5` | in_place | 1.833 s | 0.00 / 3.46 m |
| `Chase_Slide` | `slide_right` | in_place | 1.767 s | 0.01 / 3.35 m（途中で横に最大 0.17 m） |
| `BeHit_FlyUp`・`Knock_Down`・`Push_Up_To_Idle` | 同名（`push_up_to_idle`） | once | 1.533・2.500・3.100 s | 役なし（場面の代用の候補）。`Knock_Down` は骨盤が始めから 1.15 m 後ろにある |

足の運びの速さ（接地した足の後ろへの速さ。再生の速さを移動に合わせるときの物差し）: `Walk` 1.33 m/s、`Run` 4.5 m/s。本家の巡回 350 cm/s・追跡 800 cm/s より遅い。

### 取り込み
1. `_extract_textures`: glb に埋め込まれた PNG を `Intermediate/Pipeline/wasami/enemy/T_WasamiEnemy_<BaseColor|MetallicRoughness|Normal>.png` に書き出し、`dd_stage.import_texture` で取り込む（`TEXTURES`: 色は sRGB・`TEXTUREGROUP_Character`、金属と粗さは線形・`TEXTUREGROUP_CharacterSpecular`、法線は `TC_Normalmap`・`TEXTUREGROUP_CharacterNormalMap` で緑を反転〈glTF は Y 上向き〉）。4096² はそのまま（ストリーミングが描く分の mip だけ載せる）。
2. `M_DD_WasamiGltf`（`dd_assets.material` + `_build_master`）: glTF の metallic-roughness の係数 1 の形。色 → Base Color、金属と粗さの B → Metallic、G → Roughness、法線 → Normal。片面、`used_with_skeletal_mesh`。`MI_WasamiEnemy` はそのインスタンスでテクスチャ 3 枚を入れる。
3. `ensure_skeletal_pipeline`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を `PL_Wasami_Skeletal` に写し、種類ごとのフォルダなし、`use_source_name_for_asset` 偽・`asset_name` 空（こうするとメッシュは glTF のメッシュの名前、スケルトンと物理アセットはその `_Skeleton`・`_PhysicsAsset`、アニメは glTF のアニメの名前そのままになる。Interchange の `ImplementUseSourceNameForAssetOption`）、材質とテクスチャの取り込みなし、スタティックメッシュなし、Nanite なし、物理アセットあり、モーフなし、アニメあり・30 Hz で焼く。
4. `_import_model`: 前処理した glb（メッシュと節の名前を `SK_WasamiEnemy` にしてある）を `/Game/Wasami/Enemy` に置き換えで取り込み、スロット 1（`BakedMaterial`）に `MI_WasamiEnemy` を入れ、`ROLES` のアニメが全部あるかを確かめる。取り込みは呼び出しの中で終わる。

## 作るアセット

| パス | 中身 |
|---|---|
| `/Game/Wasami/Enemy/SK_WasamiEnemy` | スケルタルメッシュ。高さ 170 cm（`head_end` 168.5 cm）、幅 148 cm（基準姿勢の腕）。**正面は +Y、左手は +X**（UE のマネキンと同じ。アクタではメッシュを Yaw −90 にする。本家のナースのメッシュも Yaw −90） |
| `…/SK_WasamiEnemy_Skeleton` | 骨 28、根 `pelvis`（高さ 88.5 cm） |
| `…/SK_WasamiEnemy_PhysicsAsset` | Interchange の自動の物理アセット |
| `…/A_WasamiEnemy_<役>` | 上の表の 19 本 |
| `…/T_WasamiEnemy_*`・`MI_WasamiEnemy`、`/Game/Pipeline/Materials/M_DD_WasamiGltf` | 材質 |
| `/Game/Pipeline/Interchange/PL_Wasami_Skeletal` | 取り込みのパイプライン（`paths.SKELETAL_PIPELINE`） |
| `Intermediate/Pipeline/wasami/enemy/SK_WasamiEnemy.glb`・PNG 3 枚 | 前処理の出力（git の外。元の glb の BIN をそのまま持ち、使われなくなった元のアニメの accessor も残る） |

## 原作データの根拠
- モデルとモーションはユーザーの作ったもの（2026-09-18 の指示「`enemy_wasami_v3`・`wasami_mochi_v3`・`boss_wasami` をそれぞれ使用」、捕獲は「旧 glb の 3 本を流用」）。役の対応は一覧（`.claude/references/enemy-wasami-motions.md`）。
- 本家のナースの値（部品・気絶・ABP）は進捗記録 `20260917-enemy-wasami-body` の決定事項にあり、アクタとアニメの再生を作るときにこの記録へ移す。

## 依存関係
- `pipeline/gltf.py`（glb の読み書き・標本化・四元数）、`dd_stage`（`import_texture`・`_Graph`・`VERSION_TAG`）、`dd_assets`（`material`・`material_instance`）、`paths`（01 記録）。
- エンジン: `InterchangeManager`・`InterchangeGenericAssetsPipeline`、`SkeletalMesh`・`AnimSequence`。
- 使う側: まだ無い（アニメの再生と敵のアクタが作業一覧の項目 4 のステップ 2・3 で使う）。

## 既知の制約・注意点
- glb に**ルートの骨が無い**ので、UE のルートモーションは使えない。前へ進むアニメは前処理でその場の形にした（捕獲の 3 本は捕獲の別室で使うので進んだまま）。
- Interchange は「Node [SK_WasamiEnemy] with a skinned mesh is not root」と警告する（根 `target_character` は単位の変換なので効かない）。
- 追跡中の変化 `Chase_VaultLand` は始めの骨盤が床から約 0.8 m 高く、平らな廊下では宙から始まって見える。扱いは PIE で決める。
- 気絶のモーションは、載せ替えの物差しとしては頭だけ 4° 合わない（旧と v3 で作り直されている）。捕獲の 3 本には関係しない。
- 前処理は純粋な Python（エディタの Python に numpy が無い）で、全体で数秒かかる。

## 変更履歴
- 2026-09-18: 初版。敵ワサミの素材の取り込み（`dd_enemy.py`、原本 2 つ）を記録
