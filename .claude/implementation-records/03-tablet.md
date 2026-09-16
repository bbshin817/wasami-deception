---
title: タブレット（画面のウィジェットと素材、ミニマップ）
sources:
  - Source/wasami_deception/WasamiTabletWidget.h
  - Source/wasami_deception/WasamiTabletWidget.cpp
  - Content/Python/wasami_tools/pipeline/dd_tablet.py
updated: 2026-09-17
---

# タブレット（画面のウィジェットと素材、ミニマップ）

## 役割
本家の手持ちタブレット（`pak_reference` の `UI/Tablet/UMG_Tablet`）の画面と、それが使う素材。パワーの枠だけは、6 種のパワーを持つ最新版の `UMG_TabletPowers`（`pak_reference_2`）に倣う（何を出すかを決めるのは 04 記録のコンポーネント）。板そのものの出し入れとコンポーネントの構成はプレイヤー側（02 記録）にあり、ここは **画面の中身**（`UWasamiTabletWidget`）と **素材を原作データから作る仕組み**（`dd_tablet.py`）、そして **ミニマップの仕掛け**を受け持つ。

## 公開インターフェース

### `UWasamiTabletWidget : UUserWidget`
- `ScreenWidth` 714 / `ScreenHeight` 864（px。原作のウィジェットの大きさで、`UWidgetComponent` の DrawSize もこれ）。
- `SetShardCount(int32)` … 中央の数字。値が変わったときだけ `SetText` する。`GetShardCount()` は出している数（まだ何も入れていなければ 0）。
- `PlayCountShake()` … 本家の `PlayAnimation(Count Shake, 0, 1, Forward, 2.0)`。シャードが回収されたときに呼ぶ（06 記録）。下の「Count Shake」。
- `static EvaluateCountShakeTranslation(Seconds)`・`EvaluateCountShakeScale(Seconds)`・`EvaluateCountShakeFlash(Seconds)` … アニメの秒での値（テストが使う）。定数 `CountShakeLength` 0.2・`CountShakeSpeed` 2。
- `SetObjective(const FText&)` … 下の帯。大文字にして出す（原作の束縛 `GetText_1` が `TextToUpper`）。
- `SetPowersVisible(bool)` … 2 つの枠を出す / 隠す（本家の `Check` が、解放済みのパワーが無ければ `UMG_TabletPowers` を隠す）。表示の状態は自前の `bPowersVisible` で持つ（`UWidget::IsVisible` は Slate の実体ができるまで false を返すため）。
- `ShowSocketPowers(EWasamiPower Left, EWasamiPower Right)` … 本家の `Update Powers`。各枠に、そのパワーのアイコンの MID を `SetBrushFromMaterial` で出す。変わったときだけ書く。`None` は何も出さない（`SetBrushFromMaterial(None)` は白い四角を描くので、ブラシを空にして `SetOpacity(0)`）。
- `SetPowerPercent(EWasamiPower, float)` … そのパワーのアイコンの `Percent`（1 = 満タン）。本家と同じくアイコン（MID）ごとの値なので、左右が同じパワーなら同じゲージになる。0.001 より小さい変化は無視する。
- `BounceSocket(bool bLeft)` … 本家の `Use Left` / `Use Right`。弾みを頭から再生する。
- `TickAnimations(float DeltaSeconds)` … 弾みと `Count Shake` をプレイヤーのティックで進める（UMG のアニメを C++ で作らない）。弾みは枠の画像の `SetRenderScale` に入れる。
- `static EvaluateSocketBounce(float Seconds)` … 弾みの拡縮（テストが使う）。
- 素材（ソフト参照の UPROPERTY。`RebuildWidget` で読む。00 記録の決まり）: `BackgroundTexture`・`PlayerMarkTexture`・`VignetteTexture`・`MapMaterial`・`PowerMaterials`（6 つ、`EWasamiPower` の順）・`ScreenFont`。
- `RebuildWidget()` … 初回だけ `BuildScreen` でウィジェットの木を作る。ウィジェット BP を使わないのは、原作の px をそのまま定数で持つため。

### `dd_tablet.py`（`WasamiDDTools.import_dd_tablet` から呼ぶ）
- `import_all()` … 下の「作るアセット」を全部作り、`/Game/DD` と `/Game/Pipeline` を保存する。戻り値は種類ごとの数。
- 部分ごとに `import_textures()` / `import_mesh()` / `make_body_materials()` / `import_font()` / `import_sounds()` / `ensure_render_target()` / `make_minimap_materials()`。
- `asset(rel)` … 原作の `/Game/<rel>` を `/Game/DD/<rel>` に読み替える。テクスチャは `dd_assets.texture`（原作の sRGB・圧縮・LOD グループを `_textures.json` から入れる）、自前のマスターは `dd_assets.material` で作る（01 記録）。

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
| 左のパワー枠 `LeftPower` | 25.4711, 47.4595 – 153.7851, 174.4136 | 左の枠が指すパワーのアイコン（下の「パワーの枠」） |
| 右のパワー枠 `RightPower` | 562.0391, 47.4595 – 691.0391, 174.4595 | 右の枠が指すパワーのアイコン |
| 「Z」`ZoomKey` | 中心から (−304, 312)、自動サイズ | フォント 24、`RenderOpacity` 0.1 |

枠の位置は最新版の `UMG_TabletPowers`（`CanvasPanel_3` の余白 L 23.43 / T 32 / R 562.25 / B 705.05、`CanvasPanel_4` の 560 / 32 / 25 / 705、ウィジェットの Slot (−356.96, −428.54)）でも同じ値。

フォントはすべて `/Game/DD/UI/Fonts/helvetica-neue-bold_Font` の `Default` タイプフェイス。原作がサイズを指定していない `TextBlock_0`（目的）と `TextBlock_107`（Z）は UMG の既定の 24 のまま。

### パワーの枠
- `BuildScreen` が、`PowerMaterials` の 6 つ（`/Game/DD/Materials/MasterMaterials/MM_Powers_SpeedBoost`・`MM_Powers_Inst_Teleport`・`MM_Powers_Inst_Telepathy`・`MM_Powers_PrimalFear`・`MM_Powers_Inst_Telekinesis`・`MM_Powers_Vanish`）から動的インスタンス `PowerIcons` を作る（本家の `Construct` と同じ 6 つ）。読めなかったものは null のまま（その枠は何も出さない）。
- 枠の画像は `CanvasPanel_3` / `CanvasPanel_4` いっぱいの矩形に置くので、`RenderTransform` の既定の中心 (0.5, 0.5) で拡縮すると、本家がキャンバスごと拡縮するのと同じになる。
- **弾み**: 本家のアニメのトラックは `RenderTransform` の Scale（X と Y が同じキー）。キーはティック 0 / 3000 / 9000 / 30000（毎秒 60000。`pak_reference/README.md` の約束）→ 0 / 0.05 / 0.15 / 0.5 秒で 1.0 / 1.25 / 1.10 / 1.0、補間は Cubic（Auto）。書き出しの接線（1 ティックあたり 0 / 1.1111e-5 / −9.2593e-6 / 0）を秒あたり（0 / 0.66667 / −0.55556 / 0）にして `FRichCurve` の User 接線に入れる。接線は前後のキーから求まる自動接線と一致する（(1.1−1.0)/0.15、(1.0−1.25)/0.45）。0.5 秒で 1.0 に戻り、再生を止める（`RestoreState` は偽）。
- 呼ぶ側（02・04 記録）: プレイヤーが毎フレーム `SetPowersVisible` / `ShowSocketPowers` / `SetPowerPercent` ×6 / `TickAnimations` を、パワーのコンポーネントが Q / E で `BounceSocket` を呼ぶ。

### Count Shake（シャードの回収）
- 本家の `UMG_Tablet` のアニメ `Count Shake`（両版で同じ）。再生範囲 [0, 12001)（毎秒 60000 ティック）で、最後に評価されるのはティック 12000 = 0.2 秒。`PlayAnimation` の速さ 2 なので実時間 0.1 秒。
  - `ShardCount` の 2D 変換の区間（[0, 12000]、**`CompletionMode` RestoreState**）: 移動 X はキー 0 / 3000 / 6000 / 12000 で 0 / −14 / 0 / 0（接線は 1 ティックあたり 0 / 0 / 0.0015556 / 0）、移動 Y は 0 / 9 / −12 / 0（0 / −0.002 / −0.001 / 0）、拡縮 X・Y は 0 / 3000 / 6000 で 1 / 1.1 / 1（接線 0）。どれも Cubic（Auto）で、書き出しの接線は前後のキーの自動接線と一致する。データのある移動と拡縮だけを書き、角度と傾きは触らない。
  - `Image_41`（`Flash`）の色の区間（[0, 12001)、既定の KeepState）: α だけが 0 / 12000 で 0.25 / 0（接線 0）。RGB は書かない（紫の色合いはブラシの色のまま）。
- `PlayCountShake` はその場で最初のフレーム（移動 0・拡縮 1・α 0.25）を入れる（UE の `PlayAnimation`。ステップ 8 で確かめた）。再生中にもう一度呼ぶと頭からやり直す。**元に戻す変換は、止まっている状態から再生を始めたときの値**（UE の pre-animated state は最初に取った値を持つ）で、本作の数の変換は `ShardTranslationY` の (0, −12)。
- `TickAnimations` がアニメの秒を 経過 × 2 だけ進めて値を書き、0.2 秒に達したらその値を書いてから、数の変換を元に戻して止める。α は 0 のまま残る。
- 版の違い: 旧版の `ShardCount` は `RenderTransform` の移動 (0, −12) を持つが、最新版は持たない（本作の画面は旧版の配置）。どちらもこのアニメの後は元の変換に戻るので、差は変わらない。

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
`WasamiDDTools.import_dd_tablet` が `/Game/DD`（原作の `/Game` の木をそのまま）に 42 個。

| 種類 | パス | 数・設定 |
| --- | --- | --- |
| テクスチャ | `Textures/Characters/Player/Tablet/Tablet_{Front_D,Back_D,N,S}`、`UI/Tablet/tablet_screen_bg`・`tablet_map_{player,shard,ring,bonus_shard,arrow}`、`UI/Menu/Streaks/T_Vignette`、`UI/RingAltar_UI/Textures/ring_altar_power_{teleport,speed_boost}_icon{,_inactive}`（`pak_reference`）と `ring_altar_power_{telepathy,primal,telekinesis,vanish}_icon{,_inactive}`（`pak_reference_2`。両版で PNG も設定も同じ）、`UI/Minimap/T_06_Zone01`・`T_06_Zone2` | 25。sRGB・圧縮・LOD グループは原作の `_textures.json` のまま（アイコンは sRGB・既定の圧縮・`TEXTUREGROUP_UI`、105 × 104） |
| メッシュ | `Meshes/Player/Tablet/tablet_new_pCube2` | 1（Nanite なし、スロット `phong2`・`phong3`、ライトマップは原作の 64・UV 2） |
| マテリアル | `Materials/Player/Tablet/M_P_TabletBack`・`M_P_TabletFront` | 2（`M_DD_Substance` のインスタンス、Albedo / Normal / Packed） |
| フォント | `UI/Fonts/helvetica-neue-bold`（FontFace）・`helvetica-neue-bold_Font`（Font、Runtime） | 2 |
| 音 | `Audio/SharedGameplay/05_Tablet_Woosh_v1_1`・`_v2_1`、`Audio/UI/UI_Select_V3`（Volume 0.7。値は SoundWave の書き出しから `dd_assets.sound` が入れる） | 3 |
| ミニマップ | `UI/Minimap/T_NewMap`（レンダーターゲット 512 × 512 RGBA8）、`UI/Minimap/MM_Map_06_Zone01`・`MM_Map_06_Zone2` | 3 |
| パワーのアイコン | `Materials/MasterMaterials/MM_Powers_SpeedBoost`・`MM_Powers_Inst_Teleport`・`MM_Powers_Inst_Telepathy`・`MM_Powers_PrimalFear`・`MM_Powers_Inst_Telekinesis`・`MM_Powers_Vanish`（`M_DD_Powers` のインスタンス。`DisabledPower` = 色つき、`EnabledPower` = `_inactive`、`Percent` 1.0。名前は最新版のインスタンスのまま） | 6 |

自前のマスターは `/Game/Pipeline/Materials/M_DD_MapPlane`・`M_DD_MapScreen`・`M_DD_Powers`（3）。どれも呼び直すと作り直す（既にあるものは読み込んで親とパラメータを入れ直す）。

## 原作データの根拠
- ウィジェットの配置・フォント・色: `pak_reference/_assets/DDeception/Content/UI/Tablet/UMG_Tablet.json`（`CanvasPanel_0`/`_1`/`_762`、`Overlay_102` のパディング 34、`Image_25`・`Image_41`・`Image_212`・`Button_0`・`TextBlock_0`・`TextBlock_107`・`ShardCount`）と `UMG_TabletPowers.json`（`CanvasPanel_2`/`_3`/`_4`、`Skill1`・`Skill2`）。
- パワーの枠: `pak_reference_2/_assets/DDeception/Content/UI/Tablet/UMG_TabletPowers.json`（`Use Left` / `Use Right` の `MovieScene2DTransformSection_0` の Scale のキーと接線、結び付け先 `CanvasPanel_3` / `CanvasPanel_4`）と、そのバイトコード（`Construct` の 6 つの MID、`Update Powers`、`Check`。`.claude/references/powers/01-player-system.md` §1.2・§4）。
- パワー枠のマテリアル: `pak_reference/_materials.json` と `pak_reference_2/_materials.json` の `/Game/Materials/MasterMaterials/MM_Powers`（パラメータ `DisabledPower`・`EnabledPower`・`Percent`、Translucent・Unlit）とそのインスタンス。枠を 12 時から時計回りに塗る向きは、グラフが cook で消えているため WebGL 版 10 記録（原作の画面収録から決めた）に従う。
- シャード数の書式: `UMG_Tablet` のバイトコード（`GetAllActorsOfClass(BP_Shard)` の数を `Conv_IntToText(…, 1, 324)` で入れる）。
- ミニマップ: `BP_DD_PlayerCharacter` の `SceneCaptureComponent2D`（正射影・`OrthoWidth` 4000・`T_NewMap`・`SCS_BaseColor`・`PRM_UseShowOnlyList`）と `Show Only`（@27345）、`pak_reference_2/_levels/06_Hospital_Zone_01.full.json` の `BP_MapTexture_2`（`/Engine/BasicShapes/Plane`、位置 (30, −10775, −500)、スケール 365.014984、マテリアル `MM_Map_06_Zone01`）。
- 目的の文字列: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt` の `Current Objective`（@2293 の `COLLECT ALL SHARDS` ほか）。

## 依存関係
- `UMG`（`UUserWidget`・`UWidgetTree`・`UCanvasPanel`・`UImage`・`UBorder`・`UTextBlock`）、`Slate` / `SlateCore`（`FSlateFontInfo`）。`wasami_deception.Build.cs` に足してある。
- `dd_tablet.py` は `dd_stage` の `import_mesh` / `ensure_masters` / `_Graph` と `paths` を使う。
- `WasamiPowerTypes.h`（`EWasamiPower`・`WasamiPowerCount`。04 記録）、`WasamiAssets.h`（00 記録）。`dd_tablet.py` は `dd_assets`（01 記録。`sound`・`texture`・`material`・`export_json`）も使う。
- 使う側: `AWasamiPlayerCharacter`（02 記録）が `UWidgetComponent` にこのクラスを載せ、毎フレーム枠・ゲージ・弾みと `SetObjective`、0.1 秒ごとに `SetShardCount` を呼ぶ。`UWasamiPowerComponent`（04 記録）が `BounceSocket` を呼ぶ。`dd_level.py`（01 記録）が地図の板を置く。

## 確かめたこと（2026-09-16、PIE の 1280 × 720 の撮影）
- 画面上の板の占める範囲は x 約 4〜32 %・y 約 28〜93 %。原作の収録（1080p60、静止時）の x 4.8〜31.8 %・y 30.3〜91.0 % とおおむね合う。
- 画面の色は原作どおり。**ただしステージの露出が効いている**: 撮影では地図の黒が (4, 3, 2) なのに目的の帯が 165、上の帯が 162 と明るく出る。帯の色（線形 0.043735 = sRGB 59）から逆算した露出は約 8.7 倍で、165 / 8.7 は sRGB 59、162 / 8.7 は sRGB 76（原作の帯の灰 67〜97）と一致する。板そのものも 126 / 8.7 = sRGB 43 で、`Tablet_Front_D`（ほぼ真っ黒）どおり暗い。**明るく見えるのはタブレットではなくステージの側の問題**（自動露出が持ち上げている）。→ **2026-09-16 に直した**: 原作は自動露出を切って露出を 1.0 に固定しており（ポストプロセスボリュームではなくプロジェクト設定。00 記録の「露出」）、同じ設定を `Config/DefaultEngine.ini` に入れた。**タブレットの画面の値はまだ撮り直していない**（PIE が要る）。
- **本家の実機で同じ場所（Zone 1 の開始地点）を撮って測った**（2026-09-16、最新版 v1.9.6・3440x1440・MOD 入り。画像と測り方は `observations/README.md`）: 上の帯の地 **(32, 32, 31)**、シャードの数字 (214, 214, 215)、パワー枠の赤 (215, 0, 8)、地図の地 **(0, 0, 0)**、地図の通路の線 (93, 93, 92)、自分の位置の赤 (161, 7, 20)、下の帯 (0, 0, 0)。本作の上の帯 162 に対して原作は 32 なので、逆算していた露出約 8.7 倍（sRGB では約 5 倍）の差が実機でも裏づけられた。**露出を直せば帯は原作どおりになる**（本作 162 ÷ 8.7 → sRGB 59〜76 は原作の 32 より明るいので、帯の素の色ではなく露出と、タブレットに当たる光の両方を見る）。→ **2026-09-16 に直して確かめた**（00 記録の「露出」）。原作は自動露出を切って露出を 1.0 に固定しており、同じ設定を `Config/DefaultEngine.ini` に入れた。PIE（Zone 1 の廊下 (1801, −9601)・ヨー 90、1280 × 720、`observations/ours/zone1-corridor-pie-tablet.png`）で撮り直すと、**上の帯の地は 162 → (34, 34, 33)**（本家の実機は (32, 32, 31)）、**地図の地は (4, 3, 2) → (0, 0, 0)**（実機も (0, 0, 0)）。タブレットの画面は UMG の決まった色で灯の差が混ざらないので、これが露出が原作どおりになったことの決め手。ステージ側の基準（同じ撮影）: 画面全体の中央値 (44, 38, 39)、天井の灯 (227, 236, 235)、赤い壁 (68, 33, 33)、床 (59, 83, 84)。
- パワー枠の扇形: `Percent` を 1.0 / 0.75 / 0.25 / 0.0 にしてマテリアルを 128 × 128 に描き、12 時から時計回りの象限ごとに色つきの画素を数えた。1.0 = 4 象限すべて、0.75 = 12・3・6 時が色つきで 9 時だけ灰色、0.25 = 12 時だけ、0.0 = なし。原作の向きどおり。
- パワーの枠（2026-09-16、PIE の実画面）: 右の枠を 6 種すべてに切り替えて撮り、どのアイコンも出た。スピードブーストを使うと、その枠のアイコンが扇形に灰色へ変わった（撮った時点では左上が灰色で、12 時から時計回りの色つきの扇形が縮んでいく向きと合う。04 記録の「確かめたこと」）。弾みは短くて撮れていない（値はテストで確かめた）。
- ミニマップ: `T_NewMap` を書き出して確認。`OrthoWidth` 4000 と 10000 で写る範囲が変わる（地図の線は輝度 38 の灰、背景は黒、α は 0）。
- 歩いても画面上で揺れないこと: 歩きのカメラシェイクを掛けた PIE で 4 回標本を取り、視点（`PlayerCameraManager` の POV）は Z 186.65 → 187.15 → 185.74 → 183.56・ピッチ 0.19 → 0.03 → −0.19 → 0.16 と揺れているのに、**板の視点空間での位置は 4 回とも (35.3989, −21.9944, −4.3216) で不動**だった。同じときカメラコンポーネントは視点空間で Z −1.50 → −2.00 → −0.59 → +1.59 と動いており、板をカメラの子にしたままだとこのぶん逆に揺れていた（02 記録の `PlaceTablet`）。
- 画面が毎フレーム描き直されることは確かめていない（エディタが背面だとビューポートが描かれず、`UWidgetComponent` が描き直さない。`.claude/guides/verification.md`）。出し入れの手触りと音も同じ理由で未確認。

## 既知の制約・注意点
- **ウィジェット BP を使っていない**。原作の配置は px の実数（`47.3467` など）で、手で置くと誤差が出るうえ git の外の LFS 資産になるため、C++ の `WidgetTree->ConstructWidget` で組み立てている。
- `MaterialExpressionIf` の `ConstAGreaterThanB` などは Python から触れないので、扇形のマスクは `ceil(saturate(…))` で作っている。入力が 1 本のノード（`Frac`・`Saturate`・`Ceil`・`ComponentMask`）は `connect_material_expressions` のピン名を `""` にしないと繋がらない（`"Input"` は失敗し、その場でエラーにならずコンパイル時に「Missing … input」になる）。
- 原作の `Image_83`（`00_Ballroom` でだけ出す黒い覆い）と `UMG_MiniMap` のウィジェット階層は作っていない（本作のステージは病院だけで、地図は `Map` が直接映す）。
- シャード回収の閃き（`Count Shake`）の α は、本作の画面ではプレイヤーのティックで進む（本家は UMG の再生）。回収がプレイヤーの画面の更新より先に起きたフレームでは、最初に表示される値が 1 フレームぶん進んでいる（06 記録の「確かめたこと」）。
- **素材はコンストラクタで読まない**（ソフト参照）。2026-09-16 まではコンストラクタが `ConstructorHelpers` で `M_DD_MapScreen` と `MM_Powers_*` を読んでおり、エディタの起動時にルートに入ったそれらを `import_dd_tablet` が作り直そうとしてエディタが落ちた（01 記録の注意点）。
- `MM_Powers` の中で `Percent` をどう描くかは、原作のグラフが無いので WebGL 版の見立て（12 時から時計回り）のまま。最新版の実機で 6 種の枠を見比べるのはステップ 11（進捗記録）。
- 本家の `UMG_Tablet` の `UI Cooldown`（スピードブーストが放送する）は、結び付け先のウィジェットが木に無く見た目の効果が無いと見られるので作っていない（04 記録）。
- Zone 2 の地図は原作では `BP_MapTexture_MultiFloor` で階ごとに `T_06_Zone2` / `T_06_Zone2_02` を切り替える。本作は 1 階ぶん（`MM_Map_06_Zone2`）だけを置いている。
- 地図の板は UE5 の `bVisibleInSceneCaptureOnly` を立てて本編の描画と Lumen から外している（原作は床下に置いてベイク済みライティングで済ませていた）。
- シーンキャプチャは**タブレットを上げている間だけ**描く（`bCaptureEveryFrame`）。原作は常に描いているが、下ろしている間は画面が見えないので絵は変わらない（`.claude/guides/performance.md`）。
- `UWidgetComponent` は `bTickWhenOffscreen` が false のままなので、画面がビューポートに映っていない間は描き直さない（下ろしている間は描画も止まる）。エディタを背面にして PIE を撮ると、この理由で地図が止まったままになる。

## 変更履歴
- 2026-09-17: シャードの回収の `Count Shake`（`PlayCountShake`・`GetShardCount`・評価の静的関数）を足し、`TickSockets` を `TickAnimations` に改めた。テスト `Wasami.Tablet.CountShake`（`Tests/WasamiShardTests.cpp`。06 記録）
- 2026-09-16: テクスチャの取り込み（`_texture_settings`）とマスターの作り直し（`_master`）を `dd_assets` の `texture` / `material` へ移した（パワーの取り込みと共通にするため。作るものは同じで、取り直しても種類ごとの数は変わらなかった）
- 2026-09-16: パワーの枠を最新版の `UMG_TabletPowers` に合わせ、6 つのアイコン（MID）を持って、左右の枠が指すパワーを出し分けるようにした（`ShowSocketPowers`・`SetPowerPercent`・`SetPowersVisible`）。Q / E の弾み（`BounceSocket`・`TickSockets`）を足した。`SetPowerCharge` を外した。アイコン 8 枚とインスタンス 4 つを取り込みに足した。素材をソフト参照にし、`RebuildWidget` で読むようにした
- 2026-09-16: 露出をプロジェクト設定で原作に合わせた（00 記録の「露出」）。タブレットの画面の値の撮り直しは PIE 待ち
- 2026-09-16: 初版（画面のウィジェット、素材の取り込み、ミニマップの仕掛けを記録）
