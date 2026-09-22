---
title: 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）
status: 進行中
branch: main
base: 9c77bb2
started: 2026-09-22 12:01
updated: 2026-09-22 12:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 項目 39: 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 4 の項目 39。レビューの指摘「敵ワサミ襲撃時のアニメについて、本来暗闇のはずが、デバッグと思しきグリッドが地面に表示されている」。

別室の黒い壁 6 枚が `/Engine/EngineDebugMaterials/BlackUnlitMaterial`（エディタ専用のデバッグ材質）を使っており、これはクックされないので、パッケージ版では材質の無い板になり既定の市松（グリッド）で描かれる。

完了の条件（作業一覧より）:

1. 別室の壁の材質を、クックされるものに替える（本作の `/Game/…` に黒の Unlit を 1 つ作る）。
2. `RobotoTiny`・`SphereRenderHeightMap` も替え、パッケージ版で死亡画面と SAVING の見た目を確かめる。
3. ゲームのコードが参照する `/Engine/…` を洗い出し、パッケージに入るものだけにする（`WasamiSpecialSpawnPoint` のビルボードはエディタでしか出ないので除いてよい）。
4. パッケージ版で捕獲を 4 種とも見て、背景が真っ黒であることを確かめる。

## 計画

- [x] 1. 捕獲の別室の黒い壁を、本作の黒の Unlit 材質 `/Game/Wasami/Enemy/M_WasamiCaptureBlack` に替えた（`dd_enemy._build_capture_black` が作り、`import_wasami_enemy` の一部。`WasamiCapture` の `WallMaterial` とテストもそのパス・名前に）
- [ ] 2. `RobotoTiny` と `SphereRenderHeightMap` を本作に複製し、死亡画面と SAVING の参照を替える
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_assets.py`（複製）、`Source/wasami_deception/WasamiDeathScreenWidget.cpp`・`.h`、`WasamiSavingWidget.cpp`・`.h`、`/Game/DD/_Engine/EngineFonts/RobotoTiny`（+ 面）・`/Game/DD/_Engine/Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`（新）、実装記録 09
- [ ] 3. `/Engine/…` 参照の洗い出しと、パッケージ版での確かめ
  - 変更予定: `.claude/references/troubleshooting.md`（一覧の更新）、`.claude/guides/distribution.md`、実装記録 01、`.claude/roadmap.md`

## 次にやること

ステップ 2。`RobotoTiny`（`/Engine/EngineFonts/RobotoTiny` とその面）と `SphereRenderHeightMap` を `/Game/DD/_Engine/…` に複製する処理を `dd_assets.py` に足し（複製の決まりは `Content/Python/wasami_tools/toolsets/dd.py` のリバーブ・VREditor の音と同じ）、`WasamiDeathScreenWidget.cpp` と `WasamiSavingWidget.cpp:46` の参照を替える。

## 決定事項

- 2026-09-22: **パッケージに入っていないのは 3 つだけ**と確かめた（`.utoc` の名前）。入っていない: `EngineDebugMaterials/BlackUnlitMaterial`・`EngineFonts/RobotoTiny`・`Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`。入っている: `BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`EngineResources/Black`・`EngineResources/DefaultTextureCube`・`EngineFonts/Roboto`・`RobotoDistanceField`・`Faces/RobotoLight`・`RobotoRegular`・`RobotoBold`・`DroidSansFallback`・`EngineMaterials/DefaultNormal`・`WorldGridMaterial`・`EditorShapes/Textures/T_ShapeNormal`・`EngineDebugMaterials/VertexColorViewMode_RedOnly`。確かめ方は `grep -a -o -E "[ -~]{4,}" <…>.utoc | grep -x "<名前>.uasset"`（症状索引）。
- 2026-09-22: **作業一覧と症状索引・配布ガイドの `SphereRenderHelper` は誤り**で、コードが指しているのは `SphereRenderHeightMap`（`WasamiSavingWidget.cpp:46`。本家の `UMG_Saving.json` も同じ）。この計画のコミットで 3 か所を直した。
- 2026-09-22: **前処理（Python）が指す `/Engine/…` はすべてパッケージに入っている**ので、直すのは C++ の 3 か所だけ。`dd_assets`・`dd_powers`・`dd_audio` が使う `VREditor/Sounds/UI/Teleport_*` と `EngineSounds/ReverbSettings/BunkerHall`・`ParkingLot` は、すでに `/Game/DD/_Engine/…` に複製してある（`.utoc` で名前が見えるのはその複製の方）。
- 2026-09-22: **`RobotoTiny` は `/Game/DD/_Engine/EngineFonts/` に複製する**（`/Engine/EngineFonts/Roboto` の `Light` 書体で代用しない）。理由: 本家の `UMG_Saving` と死亡画面が `RobotoTiny` を名指ししており（`pak_reference_2/_assets/DDeception/Content/UI/Main/UMG_Saving.json`）、`Roboto` の `Light` の面（`Faces/RobotoLight`）とは**同じ Roboto-Light.ttf だがヒンティングが違う**（`RobotoTiny` は `EFontHinting::Auto`、`RobotoLight` は `::AutoLight`）。エンジンの資産を `/Game/DD/_Engine/…` に複製するのは、リバーブと VREditor の音ですでに使っている決まり（`Content/Python/wasami_tools/toolsets/dd.py`）。
- 2026-09-22: **`WasamiSpecialSpawnPoint` のビルボード（`VertexColorViewMode_RedOnly`）は触らない**。エディタでしか出ないうえ、パッケージにも入っている。テスト（`Tests/*.cpp`）の `/Engine/…` も、テストがパッケージに入らないので触らない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ステップ 3 で **パッケージを作り直す**（`.claude/guides/distribution.md` の「パッケージ」の `RunUAT.bat BuildCookRun`）。差分なら 1 分ほど、全クックで 5 分 22 秒。`run_in_background` で走らせ、**応答を終える前に必ず結果を読む**。出来た印は `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` の `grep -c "^/game/"` が 1139 前後であること。
- パッケージに何が入ったかの確かめ: `grep -a -o -E "[ -~]{4,}" Saved/StagedBuilds/Windows/wasami_deception/Content/Paks/wasami_deception-Windows.utoc | grep -x "<名前>.uasset"`。
- エディタは未確認（この反復では触っていない）。ステップ 1 でアセットを作る前に `python Tools/ue_remote.py` で応答を確かめ、PIE が残っていたら止める。

## 検証

- check_records: ステップ 1 で通した
- C++ ビルド: ステップ 1 で `Tools/editor_cycle.py`
- エディタでの確認: `M_WasamiCaptureBlack` を作って保存した（Unlit・不透明・片面・式 1）。捕獲の見た目はステップ 3 のパッケージ版で確かめる
