---
title: タイトル画面
sources:
  - Tools/dd/prepare_title.py
updated: 2026-09-19
---

# タイトル画面

## 役割
ゲームの始まりのタイトル画面（本家の旧版 v1.6.1 の `UI/Main/TitleScreen/UMG_TitleScreen`。WebGL 版が写したもの）。いまは素材だけ（作業一覧の項目 17 のステップ 1）: 本家の筆の跡・煙の黒・選択の印・曲と開始の音と声、本作のロゴとそのグロー・ワサミの顔。画面の木とアニメ・ボタンの道・タイトルのレベルはこれから（進捗記録 `20260919-title-screen`）。

## 公開インターフェース
- `python Tools/dd/prepare_title.py [--out <dir>]` … 本作の顔とロゴのグローを作る（下の「作るアセット」）。PIL と numpy を使う（エディタの Python には無い）。
- `dd_ui.import_title()`（`WasamiDDTools.import_dd_ui` の `import_all` も呼ぶ）… タイトルの素材を取り込んで保存する。戻り値は `textures` 3 / `sounds` 3 / `materials` 1 / `wasami_textures` 3（`import_all` では `title_` を前に付ける）。前処理の画像が無ければ例外。

## 内部構造と処理の流れ
### 前処理（`Tools/dd/prepare_title.py`）
WebGL 版は顔とロゴを CSS で飾っていた（`.claude/references/webgl/implementation-records/10-hud-tablet.md` の styles.css「タイトル画面」）。UMG は絵をそのまま描くので、CSS をここで絵に焼き込む。
- **顔**（`face`）: 原本 `SourceArt/Wasami/UI/title_face.png`（512 × 512、WebGL 版の `public/title/wasami-face.webp` を PNG にしたもの）に、WebGL 版の `.title__monster` の `filter`（`grayscale(0.75) sepia(0.5) hue-rotate(38deg) saturate(0.85) brightness(0.46) contrast(1.45)`）を Filter Effects の仕様の行列と伝達関数で sRGB のまま順にかけ（1 つごとに 0..1 に収める）、`mask-image`（`radial-gradient(ellipse 50% 50% at 50% 52%, 不透明 30 %, α 0.5 58 %, 透明 86 %)`: 箱の (50 %, 52 %) を中心に、半径を箱の半分とした楕円の距離で線形に）を α にして `Intermediate/Pipeline/wasami/ui/title_face.png`（RGBA 512 × 512）に書く。
- **ロゴのグロー**（`glow`）: WebGL 版の `.title__logo img` の `drop-shadow(0 0 116u rgba(255, 40, 0, 0.28))`（ロゴを 848 単位の幅で描く）を、ロゴの α を 1/4（`GLOW_SCALE`）に縮め、四方に 100 px（`GLOW_PAD`）の余白を足して σ = 116 / 2 単位（縮めた絵で 33.2 px）のガウスでぼかし、× 0.28 を α・色 (255, 40, 0) にして `Intermediate/Pipeline/wasami/ui/title_logo_glow.png`（686 × 402）に書く。画面はロゴの箱から四方に `GLOW_PAD` × `GLOW_SCALE` ロゴ px 広げた箱にロゴの下で描く。余白が 3σ 未満なら例外。
- ロゴ自体（`SourceArt/Wasami/UI/title_logo.png`、1942 × 809、WebGL 版の `public/title/logo.webp` を PNG にしたもの）はそのまま取り込む。

### 筆の跡の材質（`dd_ui._build_title_strokes`）
本家の `MM_TitleScreen_Mask_Grey`（UI・半透明）は cook で式が消え、書き出しに残るのはテクスチャのサンプル 2 つ（筆の跡は `Panner_0` の座標、マスクは UV のまま）・`Time Multiplier`（既定 1）・Emissive が `Desaturation_0` であることだけ。焼き込みのシェーダー（`python Tools/dd/cooked_shaders.py "TitleScreen/MM_TitleScreen_Mask_Grey." --show 4`）から残りを読んで組んだ:
- 筆の跡 `title_screen_chapters_background` を、`TextureCoordinate`（U 0.35・V 1）を `Panner`（速さ U 0.02・V 0、時間は `Time` × `Time Multiplier`）に通した座標で引く。
- Emissive = その RGB の `Desaturation`（Fraction なし = UE の輝度の係数 (0.3, 0.59, 0.11) の内積）。
- Opacity = マスク `title_screen_video_mask` の α × 筆の跡の α（シェーダーは `saturate` する）。
- 筆の跡の絵は一様な色 sRGB (151, 8, 0) で α が最大 45 なので、灰は線形で約 0.094（sRGB 約 86）、不透明度は最大 0.18 × マスク。WebGL 版の灰 179・α × 2.5・1 タイル 100 s は推定で、採らない。

## 作るアセット
`dd_ui.import_title()`:

| 種類 | アセット | 数 |
| --- | --- | --- |
| テクスチャ（本家） | `/Game/DD/UI/Main/TitleScreen/title_screen_video_mask`（1920 × 1200、黒で α が煙）・`title_screen_chapters_background`（5760 × 1200、筆の跡）・`title_screen_selection_marker`（394 × 74、ホバーの赤い印） | 3 |
| 音（本家） | `/Game/DD/Audio/UI/Pause_Sound_v1`（曲、音量 0.4・ループ、94.8 s）・`Start_New_Game`（9.4 s）、`/Game/DD/Audio/Titlescreen/Bierce_Title_Modified_03`（声、2.5 s） | 3 |
| 材質（本家） | `/Game/DD/UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey`（UI・半透明。上の式） | 1 |
| テクスチャ（本作） | `/Game/Wasami/UI/Title/T_TitleLogo`（1942 × 809）・`T_TitleLogoGlow`（686 × 402）・`T_TitleFace`（512 × 512）。本家の UI の絵と同じ sRGB・既定の圧縮・UI の LOD グループ | 3 |

選択音 `UI_Select_V3` はタブレット（`dd_tablet`、03 記録）、ポップアップの音と `quit_window_frame`・`helvetica-normal_Font` は死亡画面（09 記録）の取り込みが作る。前処理の出力は git の外（`Intermediate/Pipeline`）、原本 `SourceArt/Wasami/UI/title_*.png` は Git LFS。

## 原作データの根拠
- 素材と材質: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey.json`（残った式）と焼き込みのシェーダー（上）、`pak_reference_2/_textures.json`（テクスチャの設定）、`pak_reference/_assets/.../UMG_TitleScreen.json`（`Image_104` のブラシがこの材質そのもの・色味なし、`VideoMask` が `title_screen_video_mask`）。
- 曲と音: `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt` の Construct（`CreateSound2D(Pause_Sound_v1, 1, 0.5)` → `FadeIn(2, 0.5)`）と `UMG_TitleScreen.json` の `FadeOut` の音のトラック（`Start_New_Game`・`Bierce_Title_Modified_03`）。
- 本作の顔とロゴの飾り: WebGL 版 `src/styles.css` の `.title__monster`・`.title__logo img`（`.claude/references/webgl/implementation-records/10-hud-tablet.md`）。

## 依存関係
- `dd_ui.py`・`dd_assets.py`・`dd_stage.py`（01 記録の取り込みの仕組み）
- 使う側: タイトルの画面（これから）
- エンジン: `MaterialExpressionPanner`・`MaterialExpressionDesaturation`・`TextureFactory`・`SoundFactory`

## 既知の制約・注意点
- 顔とロゴのグローは WebGL 版の CSS の見た目を焼いたもの（本家に無い本作の素材）。直すときは `prepare_title.py` の定数を変えて前処理と `import_dd_ui` をやり直す。

## 変更履歴
- 2026-09-19: 初版。前処理 `Tools/dd/prepare_title.py`（顔に WebGL 版のフィルタとマスク、ロゴのグロー）と `dd_ui.import_title`（本家のテクスチャ 3・音 3、焼き込みのシェーダーから組んだ `MM_TitleScreen_Mask_Grey`、本作のテクスチャ 3）を足した（作業一覧の項目 17 のステップ 1）
