---
title: タブレット（画面のウィジェットと素材、ミニマップ）
sources:
  - Source/wasami_deception/WasamiTabletWidget.h
  - Source/wasami_deception/WasamiTabletWidget.cpp
  - Content/Python/wasami_tools/pipeline/dd_tablet.py
updated: 2026-09-16
---

# タブレット（画面のウィジェットと素材、ミニマップ）

## 役割
本家の手持ちタブレット（`pak_reference` の `UI/Tablet/UMG_Tablet`）の画面と、それが使う素材。板そのものの出し入れとコンポーネントの構成はプレイヤー側（02 記録）にあり、ここは **画面の中身**（`UWasamiTabletWidget`）と **素材を原作データから作る仕組み**（`dd_tablet.py`）、そして **ミニマップの仕掛け**を受け持つ。

## 公開インターフェース

### `UWasamiTabletWidget : UUserWidget`
- `ScreenWidth` 714 / `ScreenHeight` 864（px。原作のウィジェットの大きさで、`UWidgetComponent` の DrawSize もこれ）。
- `SetShardCount(int32)` … 中央の数字。値が変わったときだけ `SetText` する。
- `SetObjective(const FText&)` … 下の帯。大文字にして出す（原作の束縛 `GetText_1` が `TextToUpper`）。
- `SetPowerCharge(bool bLeftSocket, float Percent)` … パワー枠のマテリアルの `Percent`（1 = 使える、0 = 使えない）。0.001 より小さい変化は無視する。
- `RebuildWidget()` … 初回だけ `BuildScreen` でウィジェットの木を作る。ウィジェット BP を使わないのは、原作の px をそのまま定数で持つため。

### `dd_tablet.py`（`WasamiDDTools.import_dd_tablet` から呼ぶ）
- `import_all()` … 下の「作るアセット」を全部作り、`/Game/DD` と `/Game/Pipeline` を保存する。戻り値は種類ごとの数。
- 部分ごとに `import_textures()` / `import_mesh()` / `make_body_materials()` / `import_font()` / `import_sounds()` / `ensure_render_target()` / `make_minimap_materials()`。
- `asset(rel)` … 原作の `/Game/<rel>` を `/Game/DD/<rel>` に読み替える。`_texture_settings(version, rel)` … 原作の sRGB・圧縮・LOD グループを `_textures.json` から引く。

## 内部構造と処理の流れ

### 画面の配置（`BuildScreen`）
原作のウィジェットは 714 × 864 のキャンバスの中で、中心アンカーのキャンバスを 3 段入れ子にしている。本作は同じ矩形を画面の px に直した定数で置く（入れ子の計算は定数のコメントに残す）。ルートは `UCanvasPanel`「Screen」。描く順（＝原作のスロットの並び）は 背景 → シャード数 → 地図 → パワー枠 → 「Z」。

| 部品 | 位置（714 × 864 の px） | 中身 |
| --- | --- | --- |
| 背景 `Background` | 0, 0 – 714, 864 | `tablet_screen_bg`（帯と枠の絵込み） |
| シャード数 `ShardCount` | 164.16989, 32 – 549.02615, 158.9541 | 白字・中央揃え、フォント 100・輪郭 4、`Margin` 上 −22、`RenderTransform` 移動 (0, −12)、影なし |
| 地図の板 `MapPanel` | 47.3467, 176.3646 – 673.3267, 816.6533 | 下の 4 つを載せるキャンバス |
| ├ `Map` | 全面 | `M_DD_MapScreen`（シーンキャプチャのレンダーターゲット） |
| ├ `PlayerMark` | 中心から (−29.133867, −27.701376)、60 × 60 | `tablet_map_player` |
| ├ `Flash` | 全面 | `T_Vignette` に色 (0.48515, 0, 1) を掛け、`ColorAndOpacity` の α 0（シャード回収の閃き用。鳴らす側は未実装） |
| └ 目的の帯 `ObjectiveBand` | 板の下端から (中心 −324.2652588, −35.398716)、644.7755 × 49.11 | `UBorder`（色 0.043735 の灰、中央寄せ）+ `Objective`（フォント 24） |
| 左のパワー枠 `LeftPower` | 25.4711, 47.4595 – 153.7851, 174.4136 | `MM_Powers_Inst_Teleport` の動的インスタンス |
| 右のパワー枠 `RightPower` | 562.0391, 47.4595 – 691.0391, 174.4595 | `MM_Powers_SpeedBoost` の動的インスタンス |
| 「Z」`ZoomKey` | 中心から (−304, 312)、自動サイズ | フォント 24、`RenderOpacity` 0.1 |

フォントはすべて `/Game/DD/UI/Fonts/helvetica-neue-bold_Font` の `Default` タイプフェイス。原作がサイズを指定していない `TextBlock_0`（目的）と `TextBlock_107`（Z）は UMG の既定の 24 のまま。

### ミニマップの仕掛け
原作と同じく **レベルに置いた地図の板をシーンキャプチャで上から撮る**。

1. レベルの組み立て（`dd_level.py` の `_map_plane`、01 記録）が `/Engine/BasicShapes/Plane` を原作の `BP_MapTexture` の位置・スケールで置き、ゾーンの地図マテリアルを入れ、タグ `dd_minimap` を付ける。
2. プレイヤーの `USceneCaptureComponent2D`（02 記録）が、`dd_minimap` のアクタとシャードだけを `ShowOnlyActors` に入れて `T_NewMap` に描く（正射影・`SCS_BaseColor`）。カプセルの子なので視点のヨーで一緒に回り、画面中央に固定したプレイヤーの印の下で地図が回る。
3. 画面の `Map` が `M_DD_MapScreen` 越しにそのレンダーターゲットを映す。

### マテリアル（原作のグラフは cook で消えているので作り直したもの）
- `M_DD_MapPlane`: `Texture` パラメータをそのままベースカラーに出す。**Unlit にしない**（キャプチャの `SCS_BaseColor` は GBuffer のベースカラーを読むので、Unlit だと何も写らない）。
- `M_DD_MapScreen`: マテリアルドメイン User Interface、`Texture`（Linear Color サンプラ）を Final Color に、Opacity は定数 1（キャプチャの α は当てにならない）。
- `M_DD_Powers`: マテリアルドメイン User Interface・Translucent。UV を中心基準にし、`atan2(u, −v)` を 2π で割って 1 を足し `Frac` で 0〜1 の「12 時からの時計回りの角度」にし、`ceil(saturate(Percent − 角度))` のマスクで `EnabledPower`（灰色のアイコン）と `DisabledPower`（色つきのアイコン）を混ぜる。`Percent` 1 で全面が色つき。

## 作るアセット
`WasamiDDTools.import_dd_tablet` が `/Game/DD`（原作の `/Game` の木をそのまま）に 30 個。

| 種類 | パス | 数・設定 |
| --- | --- | --- |
| テクスチャ | `Textures/Characters/Player/Tablet/Tablet_{Front_D,Back_D,N,S}`、`UI/Tablet/tablet_screen_bg`・`tablet_map_{player,shard,ring,bonus_shard,arrow}`、`UI/Menu/Streaks/T_Vignette`、`UI/RingAltar_UI/Textures/ring_altar_power_{teleport,speed_boost}_icon{,_inactive}`、`UI/Minimap/T_06_Zone01`・`T_06_Zone2` | 17。sRGB・圧縮・LOD グループは原作の `_textures.json` のまま |
| メッシュ | `Meshes/Player/Tablet/tablet_new_pCube2` | 1（Nanite なし、スロット `phong2`・`phong3`） |
| マテリアル | `Materials/Player/Tablet/M_P_TabletBack`・`M_P_TabletFront` | 2（`M_DD_Substance` のインスタンス、Albedo / Normal / Packed） |
| フォント | `UI/Fonts/helvetica-neue-bold`（FontFace）・`helvetica-neue-bold_Font`（Font、Runtime） | 2 |
| 音 | `Audio/SharedGameplay/05_Tablet_Woosh_v1_1`・`_v2_1`、`Audio/UI/UI_Select_V3`（Volume 0.7） | 3 |
| ミニマップ | `UI/Minimap/T_NewMap`（レンダーターゲット 512 × 512 RGBA8）、`UI/Minimap/MM_Map_06_Zone01`・`MM_Map_06_Zone2`、`Materials/MasterMaterials/MM_Powers_Inst_Teleport`・`MM_Powers_SpeedBoost` | 5 |

自前のマスターは `/Game/Pipeline/Materials/M_DD_MapPlane`・`M_DD_MapScreen`・`M_DD_Powers`（3）。どれも呼び直すと作り直す（既にあるものは読み込んで親とパラメータを入れ直す）。

## 原作データの根拠
- ウィジェットの配置・フォント・色: `pak_reference/_assets/DDeception/Content/UI/Tablet/UMG_Tablet.json`（`CanvasPanel_0`/`_1`/`_762`、`Overlay_102` のパディング 34、`Image_25`・`Image_41`・`Image_212`・`Button_0`・`TextBlock_0`・`TextBlock_107`・`ShardCount`）と `UMG_TabletPowers.json`（`CanvasPanel_2`/`_3`/`_4`、`Skill1`・`Skill2`）。
- パワー枠のマテリアル: `pak_reference/_materials.json` の `/Game/Materials/MasterMaterials/MM_Powers`（パラメータ `DisabledPower`・`EnabledPower`・`Percent`、Translucent・Unlit）とそのインスタンス。枠を 12 時から時計回りに塗る向きは、グラフが cook で消えているため WebGL 版 10 記録（原作の画面収録から決めた）に従う。
- シャード数の書式: `UMG_Tablet` のバイトコード（`GetAllActorsOfClass(BP_Shard)` の数を `Conv_IntToText(…, 1, 324)` で入れる）。
- ミニマップ: `BP_DD_PlayerCharacter` の `SceneCaptureComponent2D`（正射影・`OrthoWidth` 4000・`T_NewMap`・`SCS_BaseColor`・`PRM_UseShowOnlyList`）と `Show Only`（@27345）、`pak_reference_2/_levels/06_Hospital_Zone_01.full.json` の `BP_MapTexture_2`（`/Engine/BasicShapes/Plane`、位置 (30, −10775, −500)、スケール 365.014984、マテリアル `MM_Map_06_Zone01`）。
- 目的の文字列: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt` の `Current Objective`（@2293 の `COLLECT ALL SHARDS` ほか）。

## 依存関係
- `UMG`（`UUserWidget`・`UWidgetTree`・`UCanvasPanel`・`UImage`・`UBorder`・`UTextBlock`）、`Slate` / `SlateCore`（`FSlateFontInfo`）。`wasami_deception.Build.cs` に足してある。
- `dd_tablet.py` は `dd_stage` の `import_texture` / `import_mesh` / `ensure_masters` / `_Graph` と `paths` を使う。
- 使う側: `AWasamiPlayerCharacter`（02 記録）が `UWidgetComponent` にこのクラスを載せ、毎フレーム `SetPowerCharge` と `SetObjective`、0.1 秒ごとに `SetShardCount` を呼ぶ。`dd_level.py`（01 記録）が地図の板を置く。

## 既知の制約・注意点
- **ウィジェット BP を使っていない**。原作の配置は px の実数（`47.3467` など）で、手で置くと誤差が出るうえ git の外の LFS 資産になるため、C++ の `WidgetTree->ConstructWidget` で組み立てている。
- `MaterialExpressionIf` の `ConstAGreaterThanB` などは Python から触れないので、扇形のマスクは `ceil(saturate(…))` で作っている。入力が 1 本のノード（`Frac`・`Saturate`・`Ceil`・`ComponentMask`）は `connect_material_expressions` のピン名を `""` にしないと繋がらない（`"Input"` は失敗し、その場でエラーにならずコンパイル時に「Missing … input」になる）。
- 原作の `Image_83`（`00_Ballroom` でだけ出す黒い覆い）と `UMG_MiniMap` のウィジェット階層は作っていない（本作のステージは病院だけで、地図は `Map` が直接映す）。
- シャード回収の閃き（原作の `Count Shake`）と数字の揺れは、まだ再生する側がいない。`Flash` は α 0 で置いてあるだけ。
- Zone 2 の地図は原作では `BP_MapTexture_MultiFloor` で階ごとに `T_06_Zone2` / `T_06_Zone2_02` を切り替える。本作は 1 階ぶん（`MM_Map_06_Zone2`）だけを置いている。
- 地図の板は UE5 の `bVisibleInSceneCaptureOnly` を立てて本編の描画と Lumen から外している（原作は床下に置いてベイク済みライティングで済ませていた）。

## 変更履歴
- 2026-09-16: 初版（画面のウィジェット、素材の取り込み、ミニマップの仕掛けを記録）
