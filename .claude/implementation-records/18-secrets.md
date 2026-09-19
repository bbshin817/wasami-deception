---
title: 秘密と収集物
sources:
  - Content/Python/wasami_tools/pipeline/dd_secrets.py
updated: 2026-09-20
---

# 秘密と収集物

## 役割
本家の病院の秘密と収集物（作業一覧の項目 12）。Zone 1 の秘密のエレベーター 2 つの奥と Zone 2 の秘密の部屋・迷路の後の秘密の書類（`BP_Collectable`。スコアの `SECRETS` の 4）、Zone 2 の秘密の部屋（`BP_SecretRoomZone`）と秘密の壁（`BP_07_Zone1_SecretWall`）、部屋のメモ 3 枚（`BP_MysteryCollectable`）、Zone 1 の見て使うエレベーター（`BP_FakeUseActor` の派生）。**作っている途中**: いまあるのは素材の取り込み（`dd_secrets.py`）と、Zone 1 の秘密のエレベーターのシーケンス 2 本（01 記録の「シーケンス」）。アクタと画面はこれから（進捗記録 `20260919-secrets`）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_dd_secrets()`（素材。前処理 `Tools/dd/prepare_stage.py` と `WasamiStageTools.import_dd_stage_assets` を残りが 0 になるまで、`import_dd_tablet`・`import_dd_ui` の後に）。戻り値 `sounds` 5 / `textures` 6 / `meshes` 1 / `materials` 1。
- `dd_secrets.import_all()`・`make_glitch()`・`dress_secret_file()`。

## 内部構造と処理の流れ
- `import_all`: 先にステージとタブレット・UI が作るもの（`STAGE_MADE`・`NEEDS`）があるかを確かめ、無ければ何を先に走らせるかを書いて止まる。音 5・絵 6 を取り込み（`dd_assets.sound`・`texture`。書き出しの音量・ループ・音のクラス、絵の圧縮・sRGB・LOD の群）、書類のメッシュに材質を入れ、グリッチの材質を組み、`/Game/DD` と `/Game/Pipeline` を保存する。
- `dress_secret_file`: `secret_file` の 2 つの枠（`lambert1`・`phong1`）に `MM_Shared_Secret_Folder`（本家のメッシュの枠の材質。`BP_Collectable` の部品は上書きしない）。ステージの取り込みはメッシュに材質を入れない（置くときに組み立てが入れる）が、書類は迷路の後に流れが実行時に出すので、メッシュ自身に持たせる。
- グリッチ `make_glitch`（本家の `ThirdParty/Chameleon/Materials/M_GlitchHLSL`。cook で式が消えた後処理の材質）: 既定値は書き出しのパラメータ（`dd_assets.parameter_defaults`）。`BlendingOpacity` は書き出しの式に無い（関数 `MF_SetBlending` の中）ので、シェーダーの表の既定 1。`_build_glitch` がコンパイル済みのシェーダー（`python Tools/dd/cooked_shaders.py "Chameleon/Materials/M_GlitchHLSL." --show 1`。SM5 の後処理のピクセルシェーダー 293 行、一様の表は cb2）を Custom ノード 2 つに写す:
  - `GLITCH_OFFSETS`（入力 `UV`〈`ScreenPosition` の `ViewportUV`〉・`T`〈`Time`〉・パラメータ 11・`Dot1`/`Dot2`〈`DotValue` (0, 3)・`DotValue2` (9, 7)〉）:
    - 行の乱れ: `ft = floor(T × Speed)`、`a = frac(sin(dot(UV × Density × 0.0001 × ft, DotValue)))`、`b = frac(sin(dot((ft × RandomSeed, ft), DotValue)))`、`dx = (a^Pow1 × a^Pow2 − b^Pow3 × Amount) × b × 0.05`。緑を `UV + (dx, 0)`、青を `UV − (dx, 0)` で読む（戻り値の float4）。
    - 行ずらし: `tx = T × Speed / 20`、`fr` = `tx / 32` の符号付きの小数部、行 `floor(UV.y × 32) / 32 + 10`、`floor(tx)` と `DotValue2` の乱数 2 つの平均 `n`（−1〜1）。`|n|` が `1 − Blockeffect × 0.1` を超えた行だけ `sign(n) × Amount` まで横にずらす（`ShiftUV`、0〜1 に切る）。
    - 格子のずれ: `ty = T × GridDistortionSpeed`。格子の数 `GridDistortionSize` と `round(frac(sin(ty × 2π)) × GridDistortionSize / 2)` の 2 つで `(UV, ty)` を切り、セルの番号を 32 bit の整数の乱数（`× 1664525 + 1013904223` の後に 3 成分を掛け合わせて足すのを 2 巡、上 16 bit ÷ 65536）にする。2 つの `min` の明るさ（0.3, 0.59, 0.11）を丸めてどちらの xy を使うかを選び、`× GridDistortionPower` だけ UV をずらす（`BlockUV`）。混ぜる量は 2 つの `max` の明るさ（`BlockWeight`）。
  - 場面 `PostProcessInput0` を 5 か所（そのまま・緑・青・行ずらし・格子）で読み、`GLITCH_MIX`: 赤はそのまま・緑と青は横から、を行ずらしと半々、それを格子の読みと `BlockWeight` で混ぜ、変わった分の `BlendingOpacity` 倍を場面に足す（0 未満は 0）。
  - 写さない枝: 混ぜ方 0 以外（`BlendMode` の switch の 1〜20）、マスクの絵（Chameleon は白 `T_base_white_d`）、距離の混ぜ（Chameleon の `BlendDistance` 0 では全体）、ステンシルとカスタム深度（`isStencil`・`isCD` 0）、選択の色（`SelectionColor` の a 0）。シェーダーはずらしを場面の絵の UV、乱数をビューポートの UV で読むが、推定は両方ビューポートの UV（ビューが絵を満たすときは同じ）。

## 作るアセット
- 音（`/Game/DD/Audio/…`）: `SharedGameplay/Bierce_Secret_Files_Pickup`（書類を取る）・`SharedGameplay/67-Dark_Whispers_SFX_0704`（秘密の部屋の囁き）・`SharedGameplay/DD_LVL2_15_V1_Secret_Mystery_Room_120818`（`UMG_Collectables_Secret` の曲）・`02_School/Sliding_Wall`（秘密の壁）・`Misc/DD_LoreNote_01`（`E Note` のメモ）。
- 絵（`/Game/DD/UI/Main/…`）: `Collectables/art_icon`・`diary_icon`・`sound_icon`・`movie_icon`（`UMG_Collectables` が乱数で 1 つ）・`Collectables/extras_unlock_bg`（両方の画面の枠）・`T_MysteryRoom`（`UMG_Collectables_Secret`）。
- `/Game/DD/Meshes/Shared/secret_file` の枠 2 つに `/Game/DD/Materials/Shared/MM_Shared_Secret_Folder`。
- `/Game/Pipeline/Materials/M_DD_ChameleonGlitch`（後処理。スカラー 12〈`Density` 1・`Speed` 10・`RandomSeed` 1・`Amount` 0.5・`Pow1` 7・`Pow2` 3・`Pow3` 18・`Blockeffect` 1・`GridDistortionSpeed` 1・`GridDistortionSize` 5.344284・`GridDistortionPower` 0.01・`BlendingOpacity` 1〉、ベクトル 2〈`DotValue`・`DotValue2`〉）。
- ステージの素材として（前処理の `CLASS_MESHES`・`CLASS_MATERIALS`。01 記録）: `/Game/DD/Meshes/Shared/secret_file`・`/Game/DD/Meshes/03_Manor/manor_fake_wall`、材質 `MM_Shared_Secret_Folder`・`M_06_Hospital_Brick_01`・`M_06_Hospital_MysteryRoom_Note_01`〜`03` とテクスチャ 4（`secret_file_01_D`・`mysteryroom_hospital_note_01`・`_02`・`mysteryroom_note_prescription_01`。メモの画面の紙にもなる）。
- Zone 1 のシーケンス（`dd_sequence`。01 記録）: `/Game/DD/Animation/06_Hospital/06_Hospital_Zone1_SecretElevator`（扉 `hospital_elevator_doors_L_elevator_door_2`・`R_elevator_door2` と音の目印 `secret_elevator_sound`）・`…SecretElevator1`（扉 `R_elevator_door3`・`R_elevator_door4` と同じ目印。本家どおり 1 つ目のエレベーターの目印で鳴る）。どちらも 5 s。アクタは `src:06_Hospital_Zone1_SecretElevator`・`src:06_Hospital_Zone1_SecretElevator1_2`。

## 原作データの根拠
- 部品と音・絵: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_Collectable.json`（`StaticMesh` の `secret_file`・`Audio` の `Bierce_Secret_Files_Pickup`）、`Blueprints/Shared/BP_SecretRoomZone.json`（子のアクタ `Chameleon` の値 `Glitch` 真・`Glitch Speed` 10・`Glitch Lines` 30・`Glitch Blocking` 0.5・`BlendingOpacity` 0）、`Blueprints/02_School/BP_03_SecretWall1.json`（`manor_fake_wall`）、`Blueprints/Main/BP_MysteryCollectable.json`（`Plane` はエンジンの `Plane`）、`UI/Main/UMG_Collectables.json`・`UMG_Collectables_Secret.json`・`Blueprints/UMG/UMG_MysteryNote.json` の参照。
- グリッチ: `ThirdParty/Chameleon/Materials/M_GlitchHLSL.json`（パラメータの既定）と最新版の pak のコンパイル済みシェーダー（上）。Chameleon の既定（`ThirdParty/Chameleon/Chameleon.json`）は `Glitch Grid Distortion Power` 0.001・`Size` 10・`Speed` 1。Chameleon の `Glitch Func`（`_bytecode/…/Chameleon.txt`）が `Amount` ← `Glitch Blocking`・`Speed` ← `Glitch Speed`・`Density` ← `Glitch Lines`・`GridDistortion*` ← 同名の値を入れる。

## 依存関係
- `dd_assets`（音・絵・材質・パラメータの既定）、`dd_stage._Graph`、`paths`。
- ステージの素材（`Tools/dd/prepare_stage.py` → `import_dd_stage_assets`）、`import_dd_tablet`（書体 `helvetica-neue-bold_Font`）、`import_dd_ui`（矢印 `selection_bar_arrow_hover`）。
- 使う側: これから作る書類・秘密の部屋・壁・メモのアクタと画面。

## 既知の制約・注意点
- グリッチは推定（大目標 1・2 の決め方。本家の画面とは見比べていない）。本家の絵と並べて詰めるのは作業一覧の項目 28。
- `MM_Shared_Secret_Folder` の親 `MM_Main_Substance_Fresnel` は前処理で `other` になり、M_DD_Substance に載る（縁の Fresnel の光は無い）。

## 変更履歴
- 2026-09-20: 初版。素材の取り込み `dd_secrets.py` と Zone 1 の秘密のエレベーターのシーケンス 2 本（作業一覧の項目 12 のステップ 1）
