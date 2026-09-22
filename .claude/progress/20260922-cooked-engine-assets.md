---
title: 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）
status: ユーザー待ち
branch: main
base: 9c77bb2
started: 2026-09-22 12:01
updated: 2026-09-22 13:20
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
- [x] 3a. `/Engine/…` 参照の洗い出し（完了の条件 3）。コード・前処理・`Config` の残りはすべてパッケージに入るものだけだと確かめ、症状索引と `distribution.md` に再発の見張りの grep を書いた
- [ ] 3b. パッケージ版での確かめ（完了の条件 2 の見た目・4）— **許可待ちで止まっている**（下の「要確認」）

## 次にやること

ステップ 3b。**ユーザーが `RunUAT.bat BuildCookRun` の許可をくれてから**（下の「要確認」）:

1. `python Tools/editor_cycle.py --quit-only` → `.claude/guides/distribution.md` の「パッケージ」の `RunUAT.bat BuildCookRun` → `python Tools/editor_cycle.py --no-quit --no-build`。
2. `.utoc` に `RobotoTiny.uasset`・`SphereRenderHeightMap.uasset`・`M_WasamiCaptureBlack.uasset` が入ったことを確かめる（確かめ方は下の「再開時の注意」）。
3. パッケージ版で捕獲を 4 種とも見て、別室の背景が真っ黒であること（グリッドが出ないこと）と、SAVING PROGRESS・死亡画面のヒントの字が出ることを確かめる。
4. 確かめた結果を症状索引（「出典」の行の「パッケージ版での見た目の確かめは未了」）と作業一覧の項目 39 に書き、この記録を消す。

## 決定事項

- 2026-09-22: **パッケージに入っていないのは 3 つだけ**と確かめた（`.utoc` の名前）。入っていない: `EngineDebugMaterials/BlackUnlitMaterial`・`EngineFonts/RobotoTiny`（面も同名なので両方）・`Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHeightMap`。入っている: `BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`EngineResources/Black`・`EngineResources/DefaultTextureCube`・`EngineFonts/Roboto`・`RobotoDistanceField`・`Faces/RobotoLight`・`RobotoRegular`・`RobotoBold`・`DroidSansFallback`・`EngineMaterials/DefaultNormal`・`WorldGridMaterial`・`EditorShapes/Textures/T_ShapeNormal`・`EngineDebugMaterials/VertexColorViewMode_RedOnly`。確かめ方は `grep -a -o -E "[ -~]{4,}" <…>.utoc | grep -x "<名前>.uasset"`（症状索引）。
- 2026-09-22: **前処理（Python）が指す `/Engine/…` はすべてパッケージに入っている**ので、直すのは C++ の参照だけ。`dd_assets`・`dd_powers`・`dd_audio` が使う `VREditor/Sounds/UI/Teleport_*` と `EngineSounds/ReverbSettings/BunkerHall`・`ParkingLot` は、すでに `/Game/DD/_Engine/…` に複製してある。
- 2026-09-22: **`WasamiSpecialSpawnPoint` のビルボード（`VertexColorViewMode_RedOnly`）は触らない**。エディタでしか出ないうえ、パッケージにも入っている。テスト（`Tests/*.cpp`）の `/Engine/…` も、テストがパッケージに入らないので触らない。

## 要確認（ユーザー）

- 2026-09-22: **`RunUAT.bat BuildCookRun`（パッケージの作り直し）の許可**。ステップ 3b にはパッケージ版が要るが、この反復では 3 通り（bash の直呼び・`sh` の台本・PowerShell）とも自動モードの判定に止められた（理由「Real-World Transactions」）。`.claude/settings.json` にも規則が無い。**配布ではなく手元での確かめのための組み立て**（出力は git の外の `Saved/Archive`・`Saved/StagedBuilds`）なので、許可をもらえれば進む。許可の仕方は 2 つ: (a) `.claude/settings.json` の `permissions.allow` に `Bash("C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun *)` を足す、(b) 有人セッションでユーザーが 1 度走らせる。

## 再開時の注意

- ステップ 3b で **パッケージを作り直す**（`.claude/guides/distribution.md` の「パッケージ」の `RunUAT.bat BuildCookRun`）。差分なら 1 分ほど、全クックで 5 分 22 秒。`run_in_background` で走らせ、**応答を終える前に必ず結果を読む**。出来た印は `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` の `grep -c "^/game/"` が 1139 前後であること。
- パッケージに何が入ったかの確かめ: `grep -a -o -E "[ -~]{4,}" Saved/StagedBuilds/Windows/wasami_deception/Content/Paks/wasami_deception-Windows.utoc | grep -x "<名前>.uasset"`。
- エディタは起動していて、C++ はステップ 2 のビルドが通っている。PIE は止めてある。

## 検証

- check_records: ステップ 2 で通した
- C++ ビルド: ステップ 2 で `Tools/editor_cycle.py`（成功）
- ステップ 2 の見た目: PIE で `unreal.WasamiSavingWidget.show()` → `screenshot showui`。SAVING PROGRESS の字が Roboto Light で出て、丸（`SphereRenderHeightMap`）も明滅の山で出た。死亡画面・SAVING の CDO のソフト参照が 3 つとも `/Game/DD/_Engine/…` を指すことも確かめた。作り直した Font・面・テクスチャは UE 5.8 のエンジンのアセットと設定が一致（面の ttf は md5 まで同じ）。
- ステップ 3a の洗い出し: `Source/`（`Tests/` 以外）に残る `/Engine/…` は `BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`EngineResources/WhiteSquareTexture`・`EngineFonts/Roboto`・`EngineDebugMaterials/VertexColorViewMode_RedOnly` だけで、全部パッケージに入っている。前処理と `Config/DefaultInput.ini` の `DefaultVirtualJoysticks` も同じ。前回のクックの `ReferencedSet.txt` の `/engine/` にも `blackunlitmaterial`・`robototiny`・`sphererenderheightmap` は無く、決定事項の一覧と一致した。
- パッケージ版での確かめはステップ 3b（許可待ち）。
