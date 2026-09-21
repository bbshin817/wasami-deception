---
title: EXTRAS
sources:
  - Source/wasami_deception/WasamiExtrasWidget.h
  - Source/wasami_deception/WasamiExtrasWidget.cpp
  - Source/wasami_deception/WasamiExtrasItemWidget.h
  - Source/wasami_deception/WasamiExtrasItemWidget.cpp
  - Source/wasami_deception/WasamiMaximizePictureWidget.h
  - Source/wasami_deception/WasamiMaximizePictureWidget.cpp
  - Source/wasami_deception/WasamiExtrasSoundWidget.h
  - Source/wasami_deception/WasamiExtrasSoundWidget.cpp
  - Source/wasami_deception/Tests/WasamiExtrasTests.cpp
updated: 2026-09-21
---

# EXTRAS

## 役割
タイトル画面の EXTRAS（作業一覧の項目 29）。秘密の書類（18 記録）が解放してセーブに足した EXTRAS（`UWasamiSaveGame` の `ExtrasArt`・`ExtrasSFX`。06 記録）を見る画面。本家の最新版（`pak_reference_2`）の `UMG_Extras` とその部品を写す（病院の書類が解放する Art 19〜22・Sound 5 は最新版にだけある）。並べる中身（絵・日記の声・動画）は仮で、本家の Art Gallery の絵・日記の声・動画は使わない（2026-09-20 のユーザーの回答「枠組みだけ先に作る」。中身は後でユーザーと決める）。**SOUND ARCHIVE だけは本作で実際に鳴っている曲 4 本を名前つきで並べる**（欄も 10 → 4 に減らす。2026-09-21 のユーザーの回答。下の「曲の一覧」）。画面本体 `UWasamiExtrasWidget` と並べる部品・大きく見る画面。入口はタイトル画面の EXTRAS（`UWasamiTitleScreenWidget::PressExtras`: `Show` → `OnFadeMusic` に曲の `FadeInMusic` → 選択音 → 曲を 1 s で消す。14 記録）。素材の取り込みは 09 記録（`dd_ui.import_extras`）。

## 公開インターフェース
- `UWasamiExtrasWidget`（本家 `UMG_Extras`。EXTRAS の画面本体）: `Show(WorldContext)`（BlueprintCallable。本家のタイトルの EXTRAS の `Create` → `AddToViewport(2)`。プレイヤーのコントローラーが無ければ null）・`OnFadeMusic`（本家の `FadeMusic`。BACK が画面を外すときに知らせる。タイトルが曲を戻すのに結ぶ）・`Save`/`SaveSlotName`（部品に渡すセーブ。無ければ木を組むときにスロットから読み、空なら新しいセーブ）・`Begin()`（Construct）・`Advance(Dt)`（ティック）・`Select(Section)`（区分のボタンのクリック）・`HoverSection(Section, bHovered)`・`PressBack`・`HoverBack`・`ResetAllColors`・`CheckIfPlaying`・`GetActiveButton`/`GetFadeInTime`/`GetCreditsTime`/`IsClosing`/`IsFinished`・`EvaluateOpacity`/`EvaluateCreditsY`・部品の取得（区分のボタン・スイッチャー・クレジット・絵・日記・曲・動画・再生バー 2 つ・WrapBox 4 つ）・定数 `CreditsSection` 0・`ArtGallerySection` 1・`BierceDiariesSection` 2・`SoundArchiveSection` 3・`MoviesSection` 4（本家の `Active Button` とスイッチャーの番号）・`ArtCount` 35・`DiaryCount` 10・`SoundCount` 4・`VideoCount` 10・`FadeInLength`（30001/60000 s）・`FadeInSpeed` 2・`CreditsScrollLength`（2388001/60000 s）・`BackPitch` 0.7・`BackDelay` 0.25・`UnhoveredGrey` 0.11・`ViewportZOrder` 2・`CreditsText`（仮のクレジット）。素材の欄 `ArtTextures`（ID ごとの絵。仮）・`SoundTracks`（`FWasamiExtrasTrack` の配列。曲の名前 `Name` と音 `Sound` を ID の順に。下の「曲の一覧」）。
- `UWasamiExtrasItemWidget`（本家 `UMG_Extras_Extra`。Art Gallery の絵 1 枚）: `ID`・`Art`・`Text`・`Save`（無ければ Construct が `SaveSlotName` から読み、スロットが空なら新しいセーブ）・`Begin()`（Construct）・`Press()`（クリック。解放済みなら選択音と大きく見る画面を出して true）・`IsUnlocked()`・`GetButton`/`GetImage`/`GetScaleBox`・定数 `MaximizeZOrder` 3・`SelectPitch` 1.25・`NormalGrey` 0.4531。
- `UWasamiExtrasVideoWidget`（`UMG_Extras_Extra_Video`。上の派生）: ボタンの枠が 250（Normal の絵 300）、いつも鍵。
- `UWasamiMaximizePictureWidget`（`UMG_MaximizePicture`）: `Show(WorldContext, Texture, Text)`（`AddToViewport(3)`。プレイヤーのコントローラーが無ければ null）・`Begin`/`Advance`（ティック）・`PressBack`・`HoverBack(bHovered)`・`GetFadeInTime`・`IsClosing`/`IsFinished`・`EvaluateOpacity`/`EvaluateScale`・部品の取得・定数 `FadeInLength`（30001/60000 s）・`FadeInSpeed` 2・`BackPitch` 0.7・`BackDelay` 0.25・`UnhoveredGrey` 0.11。
- `UWasamiExtrasSoundBarWidget`（`UMG_Extras_Sound_Bar`。音の一覧の下の再生バー）: `SetSound(Sound, Text)`・`Pause`/`Resume`・`StopSound`（画面本体の `Check If Playing` が使う）・`OnPlaybackPercent(Wave, Percent)`（音の知らせ）・`GetSound`/`GetPlaybackPercent`/`GetSoundWave`/`GetText`・`GetDurationText`/`GetElapsedText`/`GetTitleText`（本家の結び付け）・`FormatTime(Seconds)`・`ApplyBindings`・部品の取得。
- `UWasamiExtrasSoundButtonWidget`（`UMG_Extras_Sound_Button`。日記・曲 1 つ）: `ID`・`Text`・`Sound`・`SoundBar`・`bDiary`（既定 true）・`Save`/`SaveSlotName`・`Begin`・`Press`・`Deselect`・`Hover(bHovered)`・`IsUnlocked`・`GetLabelText`・`IsPlaying`/`IsPaused`/`IsSelected`・部品の取得・定数 `LevelRankCount` 9・`SelectPitch` 1.5・色 `RestColour`/`HoverColour`/`SelectedColour`。
- テスト `Wasami.Extras.Screen`・`.Item`・`.MaximizePicture`・`.SoundButton`・`.SoundBar`。

## 内部構造と処理の流れ
木は本家の順に C++ で組む（09 記録の画面と同じ作り）。UE 5 の既定のボタンの様式は UE 4 と違う（角丸の箱・前景色・余白）ので、ボタンは刷毛 4 つと余白を UE 4 の既定（箱・端を 8/32 残す・32 四方、余白 (2, 2, 2, 2) と押したとき (2, 3, 2, 1)）で明示する。進行バーとスライダーは UE 5.8 の既定の様式が UE 4 と同じ（進行バー: 箱・端 5/12・塗りの色 (1, 0.22, 0)。スライダー: 白の色の刷毛の棒・太さ 2・つまみ `Common/Button` 8 × 14）なので、既定に本家の値だけを入れる。
- 画面本体 `UWasamiExtrasWidget`: `CanvasPanel_0`（`FadeIn` が不透明度を動かす）→ `Button_116`（全面・黒の無地の箱。上の `Image_0` がポインタを取るので押されない）・`Image_0`（全面・`MM_TitleScreen_Mask_`〈筆の跡が流れる〉。既定の `Visible` でタイトル画面へ触らせない）・`BackgroundBlur_0`（全面・3。既定の `SelfHitTestInvisible`）・`VerticalBox_0`（左の中央から 70 右・自動の大きさ・揃え (0, 0.5)）→ 区分のボタン `ArtGallery`・`BierceDiaries`・`SoundArchive`・`Movies`（隠れて無効）・`Credits`（背景は透明・0.11 の灰。文字 helvetica-normal 36 を左寄せ）・`Back`（左下から (24, −69.1)・自動の大きさ。helvetica-neue-bold 24）・`Slider_0`（左の中央から 452 右・幅 100 × 高さ 746.2・縦・値 1・棒の太さ 6・棒 (1, 0, 0, 0.5)・つまみ透明。飾りの赤い線で、動かしても何も起きない）・`WidgetSwitcher_276`（中央から (−401, −514.6)・1328.3 × 1031。木の番号は 2、Construct が 1 にする）。
  - スイッチャーの頁: 0 `HorizontalBox_0`（下 10 空け・中央）→ `RichTextBlock_258`（中央揃え・折り返し・初めは 1110 下。様式の表は本家の `RichText_Credits` の 5 行〈REG: Roboto Light 16・0.9647 の白、YEL: helvetica 16・(1, 0.9019, 0)、Locked: helvetica 16・0.2448 の灰、RED: Roboto Bold 16・(0.9216, 0.0037, 0.0015)、RED_BOLD: Roboto Bold 18・白。影なし〉をこの画面が作る `UDataTable`）、1 `WrapBox_0`（上下の中央・間 30）→ 絵 35（ID 0〜34）、2 `CanvasPanel_19`・3 `CanvasPanel_20` → 再生バー（中央から (−661.2, −207.7)・1324 × 30）と `WrapBox_1`・`_2`（中央から 107.3 上・揃え (0.5, 0)・自動の大きさ・間 30・折り返し 1635.4・0.95 倍 → 音のボタン〈日記は `bDiary` で 10〈ID 0〜9〉、曲は `SoundTracks` の 4〈ID 0〜3〉。5 つで 1 段〉）、4 `WrapBox_3`（上下の中央・間 30・折り返し 1553.5・27 左へ → 動画 10〈ID 0, 1, 2 × 8〉）。部品には ID・絵・再生バー・画面が読んだセーブを入れてから置く。
  - `Begin`（Construct @649）: `UI_Window_PopUp_V2`（1, 1）、`FadeIn` を頭から速さ 2、@755（スイッチャーを 1・`Active Button` 1・`Reset All Colors`・ART GALLERY を白・`Check If Playing`。音は鳴らさない）。
  - `Select`: ART GALLERY（@2219）・BIERCE DIARIES（@2332）・SOUND ARCHIVE（@3070）は自分が選ばれていれば何もしない。CREDITS（@2805）と MOVIES（@3405）はいつも進む。進むと、スイッチャー・`Active Button`・`Reset All Colors`・そのボタンを白・（CREDITS だけ `Credits_Scroll` を頭から速さ 1）・`Check If Playing`。選択音 `UI_Select_V3`（1, 1）は ART GALLERY だけ先、ほかは後。
  - ホバー: 区分のボタンは自分が選ばれていなければ白、外れて 0.11 の灰（選ばれていれば何もしない）。BACK は白と灰（@914・@981）。
  - `Check If Playing`（@3337）: ワールドの音のボタンすべて（本家の `GetAllWidgetsOfClass`〈上だけに限らない〉。テストのワールドの無いものどうしも含む）のうち `Playing?` のものを `Deselect` し、その再生バーの音を止める。一時停止中のもの（`Playing?` が倒れている）はそのまま。
  - `PressBack`（@1023）: `FadeIn` を終わりから逆に速さ 2、`UI_Select_V3`（1, 0.7）、0.25 s 後に `FadeMusic` を知らせて外す（待っている間の 2 回目の `Delay` は効かない）。外れると部品の再生バーの Destruct が音を止める。
  - アニメ（ティックで進める。値は本家の書き出しの鍵そのまま）: `FadeIn` は `CanvasPanel_0` の不透明度 0 → 1（0.5 s・三次で接線 0）。`Credits_Scroll` は `RichTextBlock_258` の平行移動の Y を 1110 → −4121.86 へ 39.8 s で直線に（X は 0）。本家の長いクレジットの長さに合わせた値なので、仮の短いクレジットは約 4 s で下から現れ、約 13 s で上へ抜ける。
  - 仮の中身（TODO(仮)。2026-09-20 のユーザーの回答「枠組みだけ先に作る」。本家の Art Gallery の絵・日記の声・動画・クレジットは使わない）: 病院の書類が解放する絵 19〜22 に本作の絵（19 タイトルの顔 `T_TitleFace`・20 ポーズの `T_PausePeek`・21 `T_PauseHead`・22 ポータルの `T_Portal_Wasami`）、ほかの絵は無し（解放されない）。絵の `Text`・日記の音と名前は空（押すと選ばれて再生バーは 0:00 のまま）。クレジットは本作の数行（`CreditsText`: WASAMI DECEPTION / A DARK DECEPTION FAN GAME / ORIGINAL GAME / DARK DECEPTION / GLOWSTICK ENTERTAINMENT）。
  - **曲の一覧**（`SoundTracks`。本作の音のうち `SoundClassObject` が `DD_SoundClass_Music` のもの 4 つ＝鳴る曲すべてを、初めて聞く順に。2026-09-21 のユーザーの回答「本作で実際に鳴っている曲を名前つきで並べ、余る欄は減らす」）:
    - ID 0 `Cold Hearted` … Zone 1 の通常の曲（`06_Hospital/Music/DD_-_…_Hospital_Zone_1_-_Normal_Track_v1_2_-_LOOPING`。10 記録）。**名前は本家の EXTRAS がこの曲に付けているもの**（`UMG_Extras` の `Extras_Sound_Button_C_14`＝本家の Sound 5）。
    - ID 1 `Hospital Panic Track` … 追跡の曲（`…_Hospital_-_Panic_Track_v1_2_-_LOOPING`）
    - ID 2 `Hospital Zone 2 Normal Track` … Zone 2 の通常の曲（`…_Hospital_Zone_2_-_Normal_Track_v1_1_-_LOOPING`）
    - ID 3 `Pause Theme` … タイトル画面とポーズの曲（`UI/Pause_Sound_v1`。14・15 記録）
    - 1〜3 は本家の EXTRAS に無い曲なので、名前は本家のファイル名から起こした仮のもの（要確認）。曲以外（`66_-_Game_Over`・`DD_LVL2_15_V1_Secret_Mystery_Room_120818`・環境音）は `DD_SoundClass_SFX` なので並べない。
    - 解放は本家どおり `ExtrasSFX` に ID があるかで、Zone 1 の書類（`ID` 0）が入れる（18 記録）。本家はその書類が Sound 5（＝同じ Zone 1 の曲）だけを解放するが、本作は曲がこの 4 本しか無く、ほかの章の書類も無いので、**この書類で 4 本とも解放する**（そうしないと 3 本が永久に鍵のまま。要確認）。組み立ては本家の Sound を `dd_level.COLLECTABLE_SOUNDS` = (0, 1, 2, 3) に置き換える（18・01 記録）。
- 絵 `UWasamiExtrasItemWidget`: `CanvasPanel_0` → `Button_104`（中央に 150 四方〈動画は 250〉。Normal: 枠 `ring_altar_power_equipped_frame` 150 四方〈動画 300〉・端 0.1・0.516 の灰、Hovered: 枠 105×104 白、Pressed: `WhiteSquareTexture` 白、Disabled: `WhiteSquareTexture` 黒）・`ScaleBox_0`（全面から 5 内側・`ScaleToFill`〈満たして切る〉・当たりなし）→ `Image_1`（鍵 `locked`、刷毛 2500 四方・当たりなし）。
  - `Begin`（本家 Construct @252）: `Button_104` の様式を、Normal = 木の Normal を 0.4531 の灰に、Hovered = 木の Hovered を白に、Pressed = 絵の無い白（`Image` で描く 32 四方）に差し替える。セーブを読み（`Save` が無ければスロットから。スロットが空なら新しいセーブ）、`IsUnlocked`（`UWasamiSaveGame::IsUnlocked` の Art Gallery = `ExtrasArt` に `ID`）なら `Image_1` に `Art`（`SetBrushFromTexture(…, False)` で刷毛の 2500 四方のまま）・ボタンは `Visible`、でなければ鍵・`HitTestInvisible`（押せず、ホバーもしない）。
  - `Press`（@2511）: 解放済みだけ、`UI_Select_V3`（1, 1.25）→ `UWasamiMaximizePictureWidget::Show(Art, Text)`。
  - 動画 `UWasamiExtrasVideoWidget`: 本家は `Extras_Movies[ID].Unlocked?` で解放し、その欄の `Image` を出し、押すと `UMG_MaximizeVideo`。病院の書類の `Unlock` に動画の分岐が無く、本作のセーブは `Extras_Movies` を持たないので、いつも鍵で押しても何もしない。`UMG_MaximizeVideo` は作らない（TODO(仮): 動画はユーザーと決める）。
- 大きく見る画面 `UWasamiMaximizePictureWidget`: `CanvasPanel_0` → `Button_116`（全面・黒 0.5〈UE 4 の既定のボタンを黒に染めたもの〉・当たりなし・フォーカスなし）・`BackgroundBlur_0`（全面・6・`Visible` で下の EXTRAS の画面へ触らせない）・`ScaleBox_0`（全面から 100 内側・`ScaleToFit`、`UserSpecifiedScale` 0.933 は使われない）→ `Button_0`（黒・当たりなし・フォーカスなし）→ `Image_1`（ボタンの枠の余白 50。刷毛 1024 四方、Construct が絵の大きさに）・`TextBlock_1`（上の中央から 27.4 下・helvetica-normal 30・影 (1, 1) 黒 0.734。本家の結び付けで `Text`）・`Back`（下の中央から 69.1 上・背景透明・0.11 の灰 → `TextBlock_0` `BACK` helvetica-neue-bold 24）。
  - `FadeIn`（0.5 s）: `CanvasPanel_0` の不透明度 0 → 1（0.5 s）、`Button_0` の拡縮 0.5 → 1（0.25 s）。どちらの再生も速さ 2 なので、実際は 0.25 s・0.125 s。
  - `Begin`（Construct @305）: `FadeIn` を頭から、`Image_1` に `Texture`（絵の大きさで）。`PressBack`（@144）: `FadeIn` を終わりから逆に（速さ 2）、`UI_Select_V3`（1, 0.7）、0.25 s 後に外す（待っている間の 2 回目の `Delay` は効かない）。`HoverBack`: ホバーで白、外れて 0.11 の灰。
- 再生バー `UWasamiExtrasSoundBarWidget`: `CanvasPanel_1` → `ProgressBar_316`（横に全幅から左右 148.17 内側・中央に高さ 30。地 `WhiteSquareTexture` (0.057, 0, 0, 0.54)・塗り赤、割合 0 のまま）・`ProgressBar_315`（同じ位置。地は透明・塗り (0.503, 0, 0)）・`TextBlock_202`（中央の 92 上・helvetica-normal 30）・`TextBlock_357`（左端から 30・24）・`TextBlock_358`（右端から 30・24）。
  - 本家の Construct の `CreateSound2D(None)` は何も作らないので、`Sound` は最初の `Set Sound` まで空。`SetSound`（@193）: 鳴っている音を止め、`Text` を持ち、`CreateSound2D(音, 1, 1, 0, None, False, True)`（UI の音・レベルを越えない・終われば消える）の `OnAudioPlaybackPercent` を `OnPlaybackPercent` に結んで鳴らす。`Playback Percent` と `Sound Wave` は新しい音が知らせるまで前の音のまま。`Pause`/`Resume`: `SetPaused`。Destruct: 止める。
  - 結び付け（ティックごとに `ApplyBindings`）: `ProgressBar_315` の割合 = `Playback Percent`、`TextBlock_358` = `Sound Wave` の `Duration`、`TextBlock_357` = `Duration × Playback Percent`、`TextBlock_202` = `Text` の大文字。時間は `FTimespan::FromSeconds` の分（1 桁以上）と秒（2 桁）を `:` で（秒の端数は切る。音が無い間は 0:00）。
- 音のボタン `UWasamiExtrasSoundButtonWidget`: `CanvasPanel_0` → `Button_0`（中央に 256 四方。刷毛は 3 つとも `WhiteSquareTexture`、背景色と内容の色 0.502 の暗い赤）・`Button_1`（中央に 245 四方。Normal は `WhiteSquareTexture` 黒・当たりなし・フォーカスなし）→ `Buton`（`extras_play_icon` 100×107・0.502 の暗い赤・当たりなし）・`TextBlock_131`（中央・揃え (0.5, −2.5)・helvetica-normal 18・(0.503, 0, 0)・当たりなし。本家の結び付けで `DIARY n`／`SOUND n`〈`ID` + 1〉）。
  - `Begin`（PreConstruct と Construct が同じ @950）: セーブを読み、解放済み（日記: 本家は `Level Ranks[ID]` が None〈4〉。本作のセーブは順位を持たない〈13 記録〉ので、本家の新しいセーブと同じく 9 つとも None とみなし `0 ≤ ID < 9`。曲: `ExtrasSFX` に `ID`）ならそのまま、でなければ `Button_0` を `HitTestInvisible`・`TextBlock_131` を外す・`Buton` を `locked_-_Copy`（絵の大きさで）。
  - `Press`（@2043）: `UI_Select_V3`（1, 1.5）。再生中なら `Paused?` を立て・バーを止め・再生の絵・`Playing?` を倒す。そうでなく `Paused?` ならバーを再開（`Paused?` は立ったまま）、でなければバーの `SetSound(Sound, Text)`。どちらも `Playing?` を立て・一時停止の絵。続けて DoOnce（@29）: 閉じていなければ閉じ、ほかの音のボタンをすべて（本家の `GetAllWidgetsOfClass` と同じくワールドが同じもの。テストのワールドの無いものどうしも含む）`Deselect` し、`Selected?` を立て・色を白・一時停止の絵。
  - `Deselect`（@2409）: 解放済みだけ、DoOnce を開け、`Paused?`・`Playing?`・`Selected?` を倒し、再生の絵・色を暗い赤。`Hover`: 選ばれていなければホバーで赤 (1, 0, 0)、外れて暗い赤（文字・絵・`Button_0` の背景色）。

## 作るアセット
なし（素材は 09 記録の `dd_ui.import_extras`: 背景の材質 `/Game/DD/UI/Main/TitleScreen/MM_TitleScreen_Mask_`・開く音 `/Game/DD/Audio/UI/UI_Window_PopUp_V2`・`/Game/DD/UI/Main/TitleScreen/extras_play_icon`・`extras_pause_icon`・`locked`・`locked_-_Copy`、`/Game/DD/UI/RingAltar_UI/Textures/ring_altar_power_equipped_frame`。ほかに `/Game/DD/Audio/UI/UI_Select_V3`、エンジンの `/Engine/EngineFonts/Roboto`（クレジット）、仮の絵 `/Game/Wasami/UI/Title/T_TitleFace`・`/Game/Wasami/UI/Pause/T_PausePeek`・`T_PauseHead`・`/Game/Wasami/Portal/T_Portal_Wasami`、`/Game/DD/UI/Fonts/helvetica-normal_Font`・`helvetica-neue-bold_Font`、エンジンの `WhiteSquareTexture`）。

## 原作データの根拠
- 画面本体の木とアニメ: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_Extras.json` のクラス側 `UMG_Extras_C.WidgetTree`（部品の ID・絵・文字・音・再生バー・`Diary?` もここ）と `FadeIn`・`Credits_Scroll`（`MovieScene` の鍵。`NewAnimation` は空）、既定値 `Unhovered Color` 0.11。イベントの結び付けは `ComponentDelegateBinding_0`。流れ: `python Tools/dd/bp_flow.py pak_reference_2/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_Extras.txt <イベント>`（Construct @649・区分のクリック @2219・@2332・@3070・@3405・@2805・ホバー・Back @1023・`Reset All Colors` @2599・`Check If Playing` @3337）。クレジットの様式: `pak_reference_2/_datatables.json` の `/Game/Blueprints/UMG/RichText_Credits`。
- 木とスロット: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_Extras_Extra.json`・`UMG_Extras_Extra_Video.json`・`UMG_MaximizePicture.json`・`UMG_Extras_Sound_Button.json`・`UMG_Extras_Sound_Bar.json`（クラス側 `…_C.WidgetTree` の書き出し。スロットの書き出しに無い値は `CanvasPanelSlot` の既定〈Offsets (0, 0, 100, 30)〉、文字の大きさは UE 4 の既定 24）。結び付けはクラスの `Bindings`、ボタンのイベントは `ComponentDelegateBinding`。
- 流れ: `python Tools/dd/bp_flow.py pak_reference_2/_bytecode/DDeception/Content/UI/Main/TitleScreen/<名前>.txt <イベント>`。絵: `Construct`（@252）・クリック（@2511 → @10: `Create(UMG_MaximizePicture)` に `Texture `・`Text` を名前で入れ `AddToViewport(3)`）。動画: `PreConstruct`/`Construct`（@187: `Extras_Movies[ID]` の `Image` と `Unlocked?`）・クリック（@2514 → `UMG_MaximizeVideo`）。大きく見る画面: `Construct`（@305）・Back（@144・@15）・ホバー（@238・@196）。音のボタン: `PreConstruct`/`Construct`（@950）・クリック（@2043、DoOnce @29〜@938）・`Deselect`（@2409）・ホバー（@1485・@1764）・`GetText_0`。再生バー: `Construct`（@10）・`Set Sound`（@193）・`Pause`（@455）・`Resume`（@493）・`Destruct`（@531）・`CustomEvent_0`（@142）・`GetPercent_0`・`GetText_0`〜`_2`。
- 日記の解放の順位: `pak_reference_2/_assets/DDeception/Content/Blueprints/Save/BP_DD_SaveGame.json` の既定の `Level Ranks`（9 つとも `NewEnumerator4`）と `Blueprints/Enums/Enum_Ranks.json`（4 = None）。
- 本家の画面本体が部品をどう置くか（ID・絵・文字・音・再生バー・`Diary?` を部品ごとに持つ）: `UMG_Extras.json` の `UMG_Extras_Extra_C`・`_Video_C`・`_Sound_Button_C` の書き出し。画面本体の `Check If Playing` は再生中の音のボタンを `Deselect` し、そのバーの音を止める。

## 依存関係
- `UWasamiSaveGame`（`IsUnlocked`・`SlotName`・`UserIndex`。06 記録）、`WasamiWidgetAnimation.h`（09 記録）、`WasamiAssets.h`
- 使う側: 画面本体 `UWasamiExtrasWidget` が部品を置く。画面本体を使うのはタイトル画面の EXTRAS（`UWasamiTitleScreenWidget::PressExtras`。14 記録）
- エンジン: `UUserWidget`・`UWidgetTree`・`UVerticalBox`・`UWidgetSwitcher`・`UWrapBox`・`UHorizontalBox`・`URichTextBlock`（`FRichTextStyleRow` の `UDataTable`）・`USlider`・`UButton`・`UCanvasPanel`・`UScaleBox`・`UImage`・`UTextBlock`・`UProgressBar`・`UBackgroundBlur`・`UAudioComponent`（`OnAudioPlaybackPercent`）・`UGameplayStatics`（`CreateSound2D`・`PlaySound2D`・`LoadGameFromSlot`）

## 既知の制約・注意点
- 本家の部品は Construct ごとに `SaveSlot` を読むが、本作は画面本体が読んだセーブを `Save` に渡せる（渡さなければ部品が読む）。本家はタイトルのゲームモードの `Check For Save` が `SaveSlot` を作ってから EXTRAS が押せるので、スロットが空のときは新しいセーブ（何も解放していない）として扱う。
- 本家の絵の `ClickedEvent`（ディスパッチャ）はどこからも呼ばれないので写さない。
- 音のボタンが押されたとき、本家はワールドの音のボタンをすべて `Deselect` する（別の一覧のものも）。テストのワールドの無いボタンどうしも互いに当たる（害は無い）。
- 日記の解放（`Level Ranks[ID]` が None のとき解放）は本家のままの判定で、本作ではいつも `ID` 0〜8 が解放になる。
- 動画は鍵のまま（`UMG_MaximizeVideo` は作らない）。MOVIES の区分は本家どおり隠れて無効。
- 中身は仮（上の「仮の中身」）。絵・日記の声・動画・クレジットはユーザーと決めて `ArtTextures` と部品の値・`CreditsText` を差し替える。曲は決まっている（上の「曲の一覧」）が、ID 1〜3 の名前は仮。
- 曲はどれもループの波なので、再生バーは最後まで行くと頭へ戻って鳴り続ける（本家の EXTRAS の曲も `LOOPING` で同じ）。
- 配置は本家の 1920 × 1080 の値のまま。16:9 より横が狭い画面（エディタの PIE の窓など）では、中央からの位置で置く頁が左の区分に重なる（本家も同じ置き方）。
- `Slider_0` は飾りだが本家どおり操作できる（値が変わるだけ）。
- C++ で木を組むので、Python の `unreal.new_object` で作って `add_to_viewport` しても木が組まれず何も映らない。`Show` で出す（症状索引）。

## 確かめたこと（2026-09-20、PIE）

- Zone 1 で `Wasami.ResetSave`（EXTRAS は消さない）の後、秘密のエレベーターの奥の書類（ID 1。`Collectables` は Art Gallery 19・20）を取ると `NEW EXTRAS UNLOCKED!` が出て、ゲームモードのセーブの `ExtrasArt` が [19, 20]・`Hospital.Secrets` が [1]、ディスクのスロット `structSlot` は `ExtrasArt` [19, 20]・`Secrets` []（書類の `Unlock` は読んだ写しに足して書くので、病院の進みは前のチェックポイントのまま。18 記録）。
- 続けて `Wasami.Title` でタイトルへ移ると、EXTRAS の Art Gallery で 35 枚のうち ID 19・20 の 2 枚だけが絵になり（残りは鍵）、ID 19 を押すと大きく見る画面が出た。日記は仮のまま（鍵ではなく、押しても鳴らない。当時は曲も同じ）。
- 収録のグリッド: `Intermediate/DesktopAgent/shots/extras_step6_{doors,collect,title,maximize}.png`（git の外）。

## 変更履歴
- 2026-09-21: Zone 1 の書類が SOUND ARCHIVE の 4 本とも解放するようにした（組み立ての `COLLECTABLE_SOUNDS`。18・01 記録。作業一覧の項目 35 のステップ 4）
- 2026-09-21: SOUND ARCHIVE に本作の曲 4 本を名前つきで並べた（`SoundTracks`・`FWasamiExtrasTrack`、`SoundCount` 10 → 4、日記の 10 は `DiaryCount` へ。テスト `Wasami.Extras.Screen`・`.SoundButton`。作業一覧の項目 35 のステップ 3）
- 2026-09-21: 有人セッションのユーザーの回答で、EXTRAS は**曲の欄だけ埋める**ことにした（本作で実際に鳴っている曲を名前つきで並べ、余る欄は減らす）。日記 10 は空のまま、絵 19〜22 とクレジットは今のまま（作業一覧の項目 35）
- 2026-09-20: 作業一覧の項目 29 を閉じた（PIE で書類 → タイトルの EXTRAS まで通して確かめた。上の「確かめたこと」。ステップ 6）
- 2026-09-20: 画面本体 `UWasamiExtrasWidget`（本家 `UMG_Extras`: 区分 5 つ・スイッチャーの頁 5 つ・`FadeIn`・`Credits_Scroll`・`Check If Playing`・BACK と `FadeMusic`、クレジットの様式の表、仮の中身）と `Show`、テスト `Wasami.Extras.Screen` を足した（作業一覧の項目 29 のステップ 4）
- 2026-09-20: 初版。並べる部品（絵・動画・音のボタン・再生バー）と大きく見る画面を足した（作業一覧の項目 29 のステップ 3）
