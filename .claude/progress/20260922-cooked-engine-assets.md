---
title: 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）
status: 進行中
branch: main
base: 9c77bb2
started: 2026-09-22 12:01
updated: 2026-09-22 12:45
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 項目 39: 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 4 の項目 39。レビューの指摘「敵ワサミ襲撃時のアニメについて、本来暗闇のはずが、デバッグと思しきグリッドが地面に表示されている」。

エディタ専用・クックされないエンジンのアセットを本作が C++ から指しているため、パッケージ版で材質や書体が落ちる。完了の条件（作業一覧より）:

1. 別室の壁の材質を、クックされるものに替える（本作の `/Game/…` に黒の Unlit を 1 つ作る）。
2. `RobotoTiny`・`SphereRenderHeightMap` も替え、パッケージ版で死亡画面と SAVING の見た目を確かめる。
3. ゲームのコードが参照する `/Engine/…` を洗い出し、パッケージに入るものだけにする（`WasamiSpecialSpawnPoint` のビルボードはエディタでしか出ないので除いてよい）。
4. パッケージ版で捕獲を 4 種とも見て、背景が真っ黒であることを確かめる。

## 計画

- [x] 1. 捕獲の別室の黒い壁を、本作の黒の Unlit 材質 `/Game/Wasami/Enemy/M_WasamiCaptureBlack` に替えた
- [x] 2. `RobotoTiny`（と面）・`SphereRenderHeightMap` を `dd_assets.engine_font` / `texture` で `/Game/DD/_Engine/…` に作り直し、死亡画面のヒントと SAVING PROGRESS の参照を替えた（01・09 記録）
- [ ] 3. `/Engine/…` 参照の洗い出しと、パッケージ版での確かめ（完了の条件 3・4）
  - 変更予定: `.claude/references/troubleshooting.md`（一覧の更新）、`.claude/guides/distribution.md`、`.claude/roadmap.md`

## 次にやること

ステップ 3。

1. ゲームのコード（`Source/` の `Tests/` 以外）と前処理（`Content/Python/wasami_tools`）の `/Engine/…` をもう一度 grep し、**パッケージに入らないものが残っていないか**を確かめる（下の決定事項の一覧が基準）。残っていれば `/Game/DD/_Engine` に作り直す。
2. パッケージを作り直し（下の「再開時の注意」）、`.utoc` に `RobotoTiny.uasset`・`SphereRenderHeightMap.uasset`・`M_WasamiCaptureBlack.uasset` が入ったことを確かめる。
3. パッケージ版で捕獲を 4 種とも見て、別室の背景が真っ黒であること（グリッドが出ないこと）と、SAVING PROGRESS・死亡画面のヒントの字が出ることを確かめる。
4. `.claude/references/troubleshooting.md` と `.claude/guides/distribution.md` の一覧を、確かめた結果に合わせて直す。

## 決定事項

- 2026-09-22: **パッケージに入っていないのは 3 つだけ**と確かめた（`.utoc` の名前）。入っていない: `EngineDebugMaterials/BlackUnlitMaterial`・`EngineFonts/RobotoTiny`（面も同名なので両方）・`Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`。入っている: `BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`EngineResources/Black`・`EngineResources/DefaultTextureCube`・`EngineFonts/Roboto`・`RobotoDistanceField`・`Faces/RobotoLight`・`RobotoRegular`・`RobotoBold`・`DroidSansFallback`・`EngineMaterials/DefaultNormal`・`WorldGridMaterial`・`EditorShapes/Textures/T_ShapeNormal`・`EngineDebugMaterials/VertexColorViewMode_RedOnly`。確かめ方は `grep -a -o -E "[ -~]{4,}" <…>.utoc | grep -x "<名前>.uasset"`（症状索引）。
- 2026-09-22: **前処理（Python）が指す `/Engine/…` はすべてパッケージに入っている**ので、直すのは C++ の参照だけ。`dd_assets`・`dd_powers`・`dd_audio` が使う `VREditor/Sounds/UI/Teleport_*` と `EngineSounds/ReverbSettings/BunkerHall`・`ParkingLot` は、すでに `/Game/DD/_Engine/…` に複製してある。
- 2026-09-22: **`WasamiSpecialSpawnPoint` のビルボード（`VertexColorViewMode_RedOnly`）は触らない**。エディタでしか出ないうえ、パッケージにも入っている。テスト（`Tests/*.cpp`）の `/Engine/…` も、テストがパッケージに入らないので触らない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- ステップ 3 で **パッケージを作り直す**（`.claude/guides/distribution.md` の「パッケージ」の `RunUAT.bat BuildCookRun`）。差分なら 1 分ほど、全クックで 5 分 22 秒。`run_in_background` で走らせ、**応答を終える前に必ず結果を読む**。出来た印は `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` の `grep -c "^/game/"` が 1139 前後であること。
- パッケージに何が入ったかの確かめ: `grep -a -o -E "[ -~]{4,}" Saved/StagedBuilds/Windows/wasami_deception/Content/Paks/wasami_deception-Windows.utoc | grep -x "<名前>.uasset"`。
- エディタは起動していて、C++ はステップ 2 のビルドが通っている。PIE は止めてある。

## 検証

- check_records: ステップ 2 で通した
- C++ ビルド: ステップ 2 で `Tools/editor_cycle.py`（成功）
- ステップ 2 の見た目: PIE で `unreal.WasamiSavingWidget.show()` → `screenshot showui`。SAVING PROGRESS の字が Roboto Light で出て、丸（`SphereRenderHeightMap`）も明滅の山で出た。死亡画面・SAVING の CDO のソフト参照が 3 つとも `/Game/DD/_Engine/…` を指すことも確かめた。作り直した Font・面・テクスチャは UE 5.8 のエンジンのアセットと設定が一致（面の ttf は md5 まで同じ）。
- パッケージ版での確かめはステップ 3。
