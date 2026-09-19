---
title: タイトル画面
sources:
  - Tools/dd/prepare_title.py
  - Source/wasami_deception/WasamiTitleScreenWidget.h
  - Source/wasami_deception/WasamiTitleScreenWidget.cpp
  - Source/wasami_deception/WasamiTitleGameMode.h
  - Source/wasami_deception/WasamiTitleGameMode.cpp
  - Source/wasami_deception/Tests/WasamiTitleScreenTests.cpp
updated: 2026-09-20
---

# タイトル画面

## 役割
ゲームの始まりのタイトル画面（本家の旧版 v1.6.1 の `UI/Main/TitleScreen/UMG_TitleScreen`。WebGL 版が写したもの。最新版 v1.9.6〈`pak_reference_2`〉はアニメが `FadeOut_NewGame`・`FadeOut_Resume`〈RESUME にも開始の音と声〉・`FadeOut_Norm` に分かれ、RESUME の YES は病院なら `06_Hospital` を開くが、最終目標の「WebGL版と同じように倣う点: タイトル画面」に従って旧版を写す。2026-09-19）。メニューは RESUME / NEW GAME / OPTIONS / QUIT（本家の CHAPTERS・REPLAY・EXTRAS と隠れた動画・スライドは WebGL 版と同じく作らない。本作は 1 ステージで章の選択・リプレイ・おまけが無い。QUIT は WebGL 版に無い〈ページは閉じられない〉が、デスクトップのゲームを閉じる道として本家どおり置く）。素材（作業一覧の項目 17 のステップ 1: 本家の筆の跡・煙の黒・選択の印・曲と開始の音と声、本作のロゴとそのグロー・ワサミの顔）、画面（ステップ 2: `UWasamiTitleScreenWidget`。木・ホバー・アニメ 3 本とその音・曲）、ボタンの道とタイトルのレベル（ステップ 3: NEW GAME・RESUME・OPTIONS・QUIT、`L_Title` と `AWasamiTitleGameMode`、パッケージの始まりのマップ）、死亡画面の QUIT TO TITLE とスコア画面の NEXT の後の行き先（ステップ 4）まで。

## 公開インターフェース
- `python Tools/dd/prepare_title.py [--out <dir>]` … 本作の顔とロゴのグローを作る（下の「作るアセット」）。PIL と numpy を使う（エディタの Python には無い）。
- `dd_ui.import_title()`（`WasamiDDTools.import_dd_ui` の `import_all` も呼ぶ）… タイトルの素材を取り込んで保存する。戻り値は `textures` 3 / `sounds` 3 / `materials` 1 / `wasami_textures` 3（`import_all` では `title_` を前に付ける）。前処理の画像が無ければ例外。

- `UWasamiTitleScreenWidget`（`UUserWidget`。C++ で木を組む）
  - `Show(WorldContextObject)`（BlueprintCallable）: 最初のプレイヤーに作って Z 1（`ViewportZOrder`）で足す（本家のレベル `TitleScreen` の BeginPlay）。PIE で画面だけを出すときはリモート実行で `unreal.WasamiTitleScreenWidget.show(<ゲームのワールド>)`。
  - `SaveSlotName`（既定は `UWasamiSaveGame::SlotName`。テストは別のスロット）、`HasProgress(Save)`（`Hospital.LevelCheckpoint` > 0）、`VersionText()`（`v` + プロジェクト設定の `ProjectVersion`）。
  - `Begin(bHasProgress)`（Construct の中身。`NativeConstruct` がセーブを読んで呼ぶ）、`Advance(DeltaSeconds)`（`NativeTick` から）、`PlayFadeOut()`（NEW GAME の暗転）、`PlayFadeOut0()`（RESUME の暗転）、`FadeOutMusic(Seconds)`。
  - ボタンの道（下の「ボタンの道」）: `PressNewGame()`・`PressResume()`・`PressOptions()`・`PressQuit()`（ボタンのクリック〈RESUME は押下〉がこれを呼ぶ）、問いの YES の `NewGameEvent()`・`QuitEvent()`（UFUNCTION）。定数 `NewGameQuestion`・`PopUpZOrder`（2）・`NewGameDelay`（10）・`ResumeDelay`（5）・`NewGameMusicFadeOut`（1）・`ResumeMusicFadeOut`（4）。
  - テスト用の読み出し: `GetElapsed`・`HasResume`・`GetCoverOpacity`（`Image_128`）・`GetBlackOpacity`（`Image_0`）・`GetRedOpacity`（`Image_2`）・`GetPulseScale`（`CanvasPanel_0`）・`GetStartVolume`・`HasPlayedVoice`・`GetLevelToOpen`（道が開くレベル。道の前は空）・`HasLeft`（遅延が切れてレベルを開いた）・`HasQuit`。曲線の静的関数 `EvaluateSlideshow`・`EvaluateBlack`・`EvaluatePulse`・`EvaluateRed`・`EvaluateStartVolume` と定数（長さ・声の時刻・曲の値）。
- `AWasamiTitleGameMode`（`AGameModeBase`）… タイトルのレベルのゲームモード（下の「タイトルのレベル」）。ポーンなし（`PlayerCanRestart` が偽）。
- タイトルのレベル `/Game/Stage/Maps/L_Title`（`Content/Stage` は git の外）: `WasamiStageTools.build_title_level(map_path="")`（`dd_level.build_title`）が作る。`Config/DefaultEngine.ini` の `GameDefaultMap`（パッケージの始まり）はこのレベル。エディタの開始のレベル `EditorStartupMap` は Zone 1 のまま。
- テスト `Wasami.Title.Curves`・`Wasami.Title.Screen`・`Wasami.Title.Animations`・`Wasami.Title.WaysOut`（`Tests/WasamiTitleScreenTests.cpp`）。

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

### 画面（`UWasamiTitleScreenWidget`）
本家の旧版 `UMG_TitleScreen` の木を、スロットの値のまま（書き出しに無い値はスロットの既定。キャンバスの余白 (0, 0, 100, 30)）`RebuildWidget` で組む。描く順は本家の `CanvasPanel_0` のスロットの順（WebGL 版 10 記録の styles.css「タイトル画面」と同じ）:
1. `Button_0`: 全面の黒（入力を取らないボタンなので、黒のブラシの `UBorder`）。
2. （`CanvasPanel_1`〈隠れたスライドと、黒の上のぼかし〉と `Image_1`〈隠れた動画〉は作らない。ぼかしは黒をぼかすだけで見えない）
3. `Image_97`: 顔。右端の中央から (−1089.6, −549.2)、1100 四方の自動の大きさ（本家は進んだ章の敵の横顔。本作は `T_TitleFace`）。
4. `VideoMask`: 煙の黒 `title_screen_video_mask` を左端から幅 2029.65・上から下まで。
5. `Image_104`: 筆の跡の材質を左端から幅 1654.65・上から下まで。本家どおり無効（`SetIsEnabled(false)`）。
6. `LogoGlow`（本作だけ）と `Image_103`: 本作のロゴを、WebGL 版が本家のロゴ（(4, −44)、1043.7 × 564.9）の文字の箱に合わせた (58.8, 61.6)・幅 848・高さ 848 × 809 / 1942 に。グロー `T_TitleLogoGlow` はその下に、前処理の縮めたロゴ（486 × 202）がロゴの箱に重なるように縦横それぞれの倍率で、余白 100 px を含めて置く。
7. `TextBlock_79`: 左下の注記（左端から 48・下端から 81.08。本家の著作権の代わりに WebGL 版の `UNOFFICIAL FAN GAME — NOT AFFILIATED WITH GLOWSTICK ENTERTAINMENT`）。`helvetica-normal_Font` 18、灰 0.107（linear）。
8. `VerticalBox_160`: メニュー。左端の中央から (7.06, −93.09)、421.75 × 550.72。ボタンは上から `Resume`・`NewGame`・`Options`・`Quit`（本家の `Chapters`・`Replay`・`Extras` は作らない）。各ボタンの縦並びのスロットは下の余白 −10。
   - ボタンの様式は本家の `Setup Buttons` の後のもの: 押したときとホバーのブラシが `title_screen_selection_marker`（394 × 74、Image で描く＝ボタンいっぱいに引き伸ばす）、ふだんは同じブラシの Tint の α 0。余白は UE 4 の既定（ふだん 2、押したとき (2, 3, 2, 1)。UE 5 の既定の様式は違うので書く）。
   - 文字（`Resume_Text` など）: `helvetica-normal_Font` 30、灰 0.107、最小の幅 250、Margin 10。ボタンのスロットは書き出しの余白 (40, 3) と既定の右 4・下 2、既定の中央揃え（文字は 250 の箱の中で左寄せ、箱はボタンの中央。WebGL 版は左寄せで置いていた）。
   - ホバー: 入ると文字が白、出ると `Unhovered Color`（0.107）。クリックは本家の結線どおり RESUME だけ `OnPressed`、ほかは `OnClicked`（下の「ボタンの道」）。
9. `Image_0`（黒）・`Image_128`（黒。各辺から (−31.5, −80, −22, −57.5) はみ出す）・`Image_2`（赤 (0.266, 0, 0.002)）: どれも全面、不透明度 0、入力を取らない。
10. `TextBlock_0`: 版の文字。右端の上から (−81.92, 12) に左寄せ（黒の上でも見える）。18、灰 0.107。

Construct（`NativeConstruct`。本家どおり DoOnce）: セーブ（`SaveSlotName`。無ければ読まない）に進みが無ければ `Resume` を外す（本家は `New Game?` が立ち `Progress` < 2 なら外す。本作は Zone 1 を開くとチェックポイント 4 が書かれ、NEW GAME・RESTART・スコア画面の NEXT が病院の欄を空にするので、進み = `LevelCheckpoint` > 0）→ `Slideshow` → `SetInputMode_UIOnlyEx`（この画面、マウスを閉じ込めない）とカーソル → 曲 `Pause_Sound_v1` を `CreateSound2D`（音量 1・ピッチ 0.5）→ `FadeIn(2, 0.5)`（SoundWave の音量 0.4 と掛けて 0.2）。

アニメ（書き出しのキーと UE の自動の接線。区間は終わった後も最後の値のまま〈どの区間も `KeepState`〉）:
- `Slideshow`（2.5 s）: `Image_128` の不透明度 1 → 0。
- `FadeOut`（NEW GAME。長さは音の区間の 565580 tick = 9.43 s、絵は 3.75 s で終わる）: `Image_0` 0 → 1（3.5 s、接線 1/3.75）→ 1（3.75 s）。`CanvasPanel_0` の拡大 1（0.15 s）→ 1.05（0.25 s）→ 1（0.3 s）。`Image_2` 0（0.15 s）→ 0.5（0.25 s）→ 0.2（0.3 s）→ 0（3.75 s）。0.15 s からの区間は、その前も最初のキーの値が木の値と同じなので曲線のまま評価する。音: `Start_New_Game` を初めから `CreateSound2D` で鳴らし、音量を区間の曲線 0.6（0.45 s まで）→ 0.3（1.65 s から）で毎ティック動かす。`Bierce_Title_Modified_03` を 98999 tick（1.65 s）で `PlaySound2D`（音量 1）。WebGL 版 10 記録の `ANIM`・`SOUND` と値が一致。
- `FadeOut_0`（RESUME。3.75 s）: `Image_0` だけ、`FadeOut` と同じキー。

### ボタンの道
本家の旧版の Ubergraph（`python Tools/dd/bp_flow.py pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt <番地>`）を写す。遅延（本家の `Delay`）はウィジェットのティックで、アニメの後に数える。クリックはティックの間に来るので、遅延はクリックの時の `Elapsed` から数える。
- **NEW GAME**（@8532）: 本家は `SaveSlot` の `New Game?`（新しいゲームを始めるまで立つ）なら問わずに進む。本作のセーブにその印は無いので、**進み（`HasProgress`。クリックの時にセーブを読む）があれば問う**。問いは `UWasamiPopUpWidget`（`STARTING A NEW GAME WILL RESET ALL PROGRESS.`、Frame は既定の 0 = RESTART の枠、Z 2）と選択音 `UI_Select_V3`（1）。YES（`NewGameEvent`。本家の `New Game` @8743）は問いの `PressNo`（閉じて選択音 0.7）の後に、問わないときと同じ道へ。NO は問いが自分で閉じるだけ。
  - その道（@1748）: 曲を 1 s で消す → 本家のゲームモードの `Erase Save Files`（本作は `UWasamiSaveGame::Erase`: 新しいセーブを書く。06 記録）→ `SetInputMode_GameOnly` → `FadeOut` → **10 s** 後に Zone 1 を開く（本家は最初の章の `00_TypeWriter`。空のセーブの Zone 1 はエレベーターの到着から）。
- **RESUME**（@9272。押下）: 本家は章のチェックポイントが 0 より大きければ問い `UMG_PopUp_Resume`（チェックポイントから続けるか。NO でその章を最初から）を出すが、**作らない**（WebGL 版と同じく問わずに続きへ。NO は NEW GAME と重なり、YES の行き先は問わないときと同じ）。@115: 曲を 4 s で消す → `SetInputMode_GameOnly` → `FadeOut_0` → **5 s** 後にセーブのチェックポイントのゾーン（`AWasamiGameMode::LevelForCheckpoint`: 本家の病院の入口 `06_Hospital` の `Spawn` @81063 の振り分け。7〜10 は Zone 2、ほかは Zone 1。02 記録）を開く。本家は章の最初のレベル（`00_Ballroom`）を開き、その `Spawn` がチェックポイントから続ける。
- **OPTIONS**（@8405）: オプション画面 `UWasamiOptionsWidget::Show`（本家の `CreateAndAddWidget(UMG_Options, 10)`。タイトルでは DIFFICULTY が出る。15 記録）と選択音（1）。プレイヤーがいなければ画面は出ない。
- **QUIT**（@8527 → @1004）: `UWasamiPopUpWidget`（Frame 1 = `quit_window_frame`。文は空で、枠の絵が問う。Z 2）と選択音（1）。YES（`QuitEvent`。@8705）は `QuitGame(Self, None, Quit, False)`（問いは出たまま）。PIE では PIE が止まる。
- 道が 1 つ始まると（`GetLevelToOpen` が空でない）ほかのボタンは何もしない。本家は入力をゲームへ渡す（`SetInputMode_GameOnly`: ビューポートがマウスを捕まえる）のでメニューがクリックを受けないのを、プレイヤーの居ないテストでも同じになるように書いたもの。
- レベルを開くのは `UGameplayStatics::OpenLevel(<名前>, true)`。ワールドが無いとき（テスト）は `HasLeft` を立てるだけ。
- 暗転の後の時間（10 s・5 s）は本家のコードの `Delay`。WebGL 版は 3.75 s で始めていたが、ブラウザのユーザー操作の有効期間に収めるための都合（WebGL 版 01 記録）で、UE には無いので採らない（2026-09-19）。

### タイトルのレベル（`L_Title` と `AWasamiTitleGameMode`）
本家のタイトルは別のレベル `TitleScreen`（中身は 2D のウィジェットだけ。3D の背景は無い）で、そのレベル BP の `ReceiveBeginPlay`（`pak_reference/_bytecode/DDeception/Content/TitleScreen.txt` @4390）が画面を出す。本作は空のレベル `L_Title` の World Settings の GameMode Override を `AWasamiTitleGameMode` にし、ゲームモードの `BeginPlay` がレベル BP の受け持ちをする:
- 本家のタイトルのゲームモードも `BP_DD_GameMode`（レベル BP がそれに Cast する）なので、その BeginPlay の `Check Settings Save` → `Set Settings` を `BeginPlay` の頭で通す（ゲームインスタンスの `CheckSettingsSave`。15 記録）。
- ゲームインスタンスの `Used Hard Respawn?`・`Hard Check Point`（入口のもの。本作に無い）は写さない。ゲームモードの `Reset Game Instance(True)`（本家の `BP_DD_GameMode` @38662）→ 本作は回収の記憶を空に・ライフ 3（06 記録）。
- `Create(UMG_TitleScreen)` → `AddToViewport(1)`（`UWasamiTitleScreenWidget::Show`）。`SaveSlot` を読んで章と実績を決める所は写さない（画面が自分でセーブを読む）。
- ポーンは出さない（`PlayerCanRestart` が偽。プレイヤーの開始の場所が無くても警告が出ない）。ゾーンのゲームモード `AWasamiGameMode`（02 記録）は BeginPlay でゾーンの準備（チェックポイント・開始の場所・ゾーンの流れ・黒からの明け）をするので、タイトルでは使わない。
- `dd_level.build_title`: `LevelEditorSubsystem.new_level` で空のレベルを作り（あれば開くだけ）、World Settings（`GameplayStatics.get_all_actors_of_class(world, WorldSettings)` の最初）の `default_game_mode` を `/Script/wasami_deception.WasamiTitleGameMode` にして保存し、前に開いていたレベルを開き直す。C++ のモジュールがビルドされていないとクラスが無いので例外。

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
- 画面: `pak_reference/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.json`（木は `UMG_TitleScreen_C.WidgetTree` の側の `CanvasPanelSlot`・`VerticalBoxSlot`・`ButtonSlot`。アニメは `FadeOut_INST` などの `MovieScene` のトラックと区間、音の区間の開始 `SectionStartTimeSeconds` 1.65 と `KeepState` は `PrecompiledEvaluationTemplate`。`Unhovered Color` は `Default__UMG_TitleScreen_C`）。Construct・`Setup Buttons`・ホバーは `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt`（`python Tools/dd/bp_flow.py <file> Construct` / `"Setup Buttons"` / `BndEvt__NewGame_K2Node_ComponentBoundEvent_0_OnButtonHoverEvent__DelegateSignature`）。ボタンのスロットの既定（余白 (4, 2)・中央揃え）と UE 4 の既定のボタンの余白は UE のソースの `UButtonSlot` と UMG の既定の様式。
- 素材と材質: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey.json`（残った式）と焼き込みのシェーダー（上）、`pak_reference_2/_textures.json`（テクスチャの設定）、`pak_reference/_assets/.../UMG_TitleScreen.json`（`Image_104` のブラシがこの材質そのもの・色味なし、`VideoMask` が `title_screen_video_mask`）。
- 曲と音: `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt` の Construct（`CreateSound2D(Pause_Sound_v1, 1, 0.5)` → `FadeIn(2, 0.5)`）と `UMG_TitleScreen.json` の `FadeOut` の音のトラック（`Start_New_Game`・`Bierce_Title_Modified_03`）。
- 本作の顔とロゴの飾り: WebGL 版 `src/styles.css` の `.title__monster`・`.title__logo img`（`.claude/references/webgl/implementation-records/10-hud-tablet.md`）。

## 依存関係
- `dd_ui.py`・`dd_assets.py`・`dd_stage.py`（01 記録の取り込みの仕組み）
- 画面: `UWasamiSaveGame`（`Erase` を含む。06 記録）、`UWasamiPopUpWidget`・`WasamiWidgetAnimation.h`（09 記録）、`AWasamiGameMode::Zone1LevelName`・`LevelForCheckpoint`（02 記録）、`WasamiAssets.h`
- ゲームモード: `UWasamiGameInstance`（06 記録）、`UWasamiTitleScreenWidget`
- レベルの道具: `dd_level.py` の `_open_level`（01 記録）
- 使う側: `L_Title`（`GameDefaultMap`。00 記録）。死亡画面の QUIT TO TITLE（09 記録）とスコア画面の NEXT の後（ゲームモードの `FinishedLevel`。13 記録）とデバッグの `Wasami.Title`（06 記録）が `TitleLevelName` を開く。台本 `Tools/playthrough.py` の区間 `title`（01 記録）
- エンジン: `MaterialExpressionPanner`・`MaterialExpressionDesaturation`・`TextureFactory`・`SoundFactory`、`UGeneralProjectSettings`（`EngineSettings`）

## 既知の制約・注意点
- 版の文字の `ProjectVersion`（`Config/DefaultGame.ini`）は 1.0.0（`v1.0.0`。2026-09-20 のユーザーの回答。それまでは仮の 0.1.0）。
- NEW GAME が問うかどうかは本家の `New Game?` の代わりに進みで決める（上の「ボタンの道」）。本家は一度でも新しいゲームを始めた後は、進みが無くても問う。
- `L_Title` は git の外（`Content/Stage`）。作り直すときは C++ をビルドしてから `WasamiStageTools.build_title_level`。エディタで遊んで確かめるときは `L_Title` を開いて PIE（エディタの開始のレベルは Zone 1）。
- `SetInputMode_UIOnlyEx` にこの画面を渡すと、画面が焦点を持てないので `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget` が出る。本家も焦点を持てない画面を渡しているので、そのままにしている（死亡画面も同じ）。
- 顔とロゴのグローは WebGL 版の CSS の見た目を焼いたもの（本家に無い本作の素材）。直すときは `prepare_title.py` の定数を変えて前処理と `import_dd_ui` をやり直す。

## 確かめたこと（2026-09-19、PIE、`L_Title` から、エディタを前面）
- `python Tools/playthrough.py run title --record step4_title.mkv --shots`（セーブはチェックポイント 4）: RESUME のある画面で NEW GAME → RESTART? の枠と STARTING A NEW GAME WILL RESET ALL PROGRESS. → YES で問いが閉じ、約 0.3 s で赤の閃光（収録の全体の R が 34 → 67）、赤が引きながら暗くなり YES から約 3.6 s で真っ黒（右上の版の文字だけ残る）、YES から約 11 s で Zone 1 がエレベーターの到着（チェックポイント 4・ライフ 3・シャード 337）で開いた。収録は `video_probe.py series` で測った。グリッド `Intermediate/Overnight/title_newgame_grid.png`（git の外）。
- Zone 1 で `Wasami.Checkpoint 8`・`Wasami.Lives 1`・`Wasami.Kill` → YOU ARE DEAD と 3 つのボタン → QUIT TO TITLE → 約 1 s でタイトル（チェックポイント 8 のまま・RESUME あり・ライフ 3）→ RESUME → 約 6 s で Zone 2 の `PlayerStart_MiniBoss`（チェックポイント 8・目的 Get past the nurses）。
- `python Tools/playthrough.py run z2_escape --setup`: スコア画面の NEXT から約 4 s でゲームが動き、タイトルがチェックポイント 0・RESUME なし・ライフ 3 で開いた。続けて `run title`: 問わずに暗転し、Zone 1 がチェックポイント 4・ライフ 3 で開いた。

## 変更履歴
- 2026-09-20: 版を 1.0.0 にした（2026-09-20 のユーザーの回答）
- 2026-09-19: タイトルのゲームモードの `BeginPlay` の頭で設定を読んで当てるようにした（15 記録。作業一覧の項目 18 のステップ 1）
- 2026-09-19: OPTIONS がオプション画面を Z 10 で開くようにした（15 記録。作業一覧の項目 18 のステップ 4）
- 2026-09-19: 死亡画面の QUIT TO TITLE とスコア画面の NEXT の後の行き先をタイトルにし、デバッグ `Wasami.Title` と台本の区間 `title` を足して、通しで確かめた（作業一覧の項目 17 のステップ 4）
- 2026-09-19: 初版。前処理 `Tools/dd/prepare_title.py`（顔に WebGL 版のフィルタとマスク、ロゴのグロー）と `dd_ui.import_title`（本家のテクスチャ 3・音 3、焼き込みのシェーダーから組んだ `MM_TitleScreen_Mask_Grey`、本作のテクスチャ 3）を足した（作業一覧の項目 17 のステップ 1）
- 2026-09-19: 画面 `UWasamiTitleScreenWidget` を足した: 本家の旧版の木をスロットのまま（作らない部品を除く）、Setup Buttons の後の様式とホバー、Construct（進みが無ければ RESUME を外す・Slideshow・入力・曲）、FadeOut（黒・脈動・赤と開始の音と声）と FadeOut_0、テスト `Wasami.Title.*`（作業一覧の項目 17 のステップ 2）
- 2026-09-19: ボタンの道（NEW GAME〈進みがあれば問う・セーブを消す・10 s で Zone 1〉・RESUME〈5 s でチェックポイントのゾーン〉・OPTIONS〈選択音だけ〉・QUIT〈問うて閉じる〉）とタイトルのレベル `L_Title`・`AWasamiTitleGameMode`・`build_title_level`、`GameDefaultMap` をタイトルに、テスト `Wasami.Title.WaysOut`（作業一覧の項目 17 のステップ 3）
