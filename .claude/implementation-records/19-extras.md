---
title: EXTRAS
sources:
  - Source/wasami_deception/WasamiExtrasItemWidget.h
  - Source/wasami_deception/WasamiExtrasItemWidget.cpp
  - Source/wasami_deception/WasamiMaximizePictureWidget.h
  - Source/wasami_deception/WasamiMaximizePictureWidget.cpp
  - Source/wasami_deception/WasamiExtrasSoundWidget.h
  - Source/wasami_deception/WasamiExtrasSoundWidget.cpp
  - Source/wasami_deception/Tests/WasamiExtrasTests.cpp
updated: 2026-09-20
---

# EXTRAS

## 役割
タイトル画面の EXTRAS（作業一覧の項目 29）。秘密の書類（18 記録）が解放してセーブに足した EXTRAS（`UWasamiSaveGame` の `ExtrasArt`・`ExtrasSFX`。06 記録）を見る画面。本家の最新版（`pak_reference_2`）の `UMG_Extras` とその部品を写す（病院の書類が解放する Art 19〜22・Sound 5 は最新版にだけある）。並べる中身（絵・音・動画）は仮で、本家の Art Gallery の絵・曲・声・動画は使わない（2026-09-20 のユーザーの回答「枠組みだけ先に作る」。中身は後でユーザーと決める）。いまは並べる部品と大きく見る画面まで（画面本体とタイトル画面の入口は項目 29 の残りのステップ）。素材の取り込みは 09 記録（`dd_ui.import_extras`）。

## 公開インターフェース
- `UWasamiExtrasItemWidget`（本家 `UMG_Extras_Extra`。Art Gallery の絵 1 枚）: `ID`・`Art`・`Text`・`Save`（無ければ Construct が `SaveSlotName` から読み、スロットが空なら新しいセーブ）・`Begin()`（Construct）・`Press()`（クリック。解放済みなら選択音と大きく見る画面を出して true）・`IsUnlocked()`・`GetButton`/`GetImage`/`GetScaleBox`・定数 `MaximizeZOrder` 3・`SelectPitch` 1.25・`NormalGrey` 0.4531。
- `UWasamiExtrasVideoWidget`（`UMG_Extras_Extra_Video`。上の派生）: ボタンの枠が 250（Normal の絵 300）、いつも鍵。
- `UWasamiMaximizePictureWidget`（`UMG_MaximizePicture`）: `Show(WorldContext, Texture, Text)`（`AddToViewport(3)`。プレイヤーのコントローラーが無ければ null）・`Begin`/`Advance`（ティック）・`PressBack`・`HoverBack(bHovered)`・`GetFadeInTime`・`IsClosing`/`IsFinished`・`EvaluateOpacity`/`EvaluateScale`・部品の取得・定数 `FadeInLength`（30001/60000 s）・`FadeInSpeed` 2・`BackPitch` 0.7・`BackDelay` 0.25・`UnhoveredGrey` 0.11。
- `UWasamiExtrasSoundBarWidget`（`UMG_Extras_Sound_Bar`。音の一覧の下の再生バー）: `SetSound(Sound, Text)`・`Pause`/`Resume`・`StopSound`（画面本体の `Check If Playing` が使う）・`OnPlaybackPercent(Wave, Percent)`（音の知らせ）・`GetSound`/`GetPlaybackPercent`/`GetSoundWave`/`GetText`・`GetDurationText`/`GetElapsedText`/`GetTitleText`（本家の結び付け）・`FormatTime(Seconds)`・`ApplyBindings`・部品の取得。
- `UWasamiExtrasSoundButtonWidget`（`UMG_Extras_Sound_Button`。日記・曲 1 つ）: `ID`・`Text`・`Sound`・`SoundBar`・`bDiary`（既定 true）・`Save`/`SaveSlotName`・`Begin`・`Press`・`Deselect`・`Hover(bHovered)`・`IsUnlocked`・`GetLabelText`・`IsPlaying`/`IsPaused`/`IsSelected`・部品の取得・定数 `LevelRankCount` 9・`SelectPitch` 1.5・色 `RestColour`/`HoverColour`/`SelectedColour`。
- テスト `Wasami.Extras.Item`・`.MaximizePicture`・`.SoundButton`・`.SoundBar`。

## 内部構造と処理の流れ
木は本家の順に C++ で組む（09 記録の画面と同じ作り）。UE 5 の既定のボタンの様式は UE 4 と違う（角丸の箱・前景色・余白）ので、ボタンは刷毛 4 つと余白を UE 4 の既定（箱・端を 8/32 残す・32 四方、余白 (2, 2, 2, 2) と押したとき (2, 3, 2, 1)）で明示する。進行バーは UE 5.8 の既定の様式が UE 4 と同じ（箱・端 5/12・塗りの色 (1, 0.22, 0)）なので、既定に本家の絵と色だけを入れる。
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
なし（素材は 09 記録の `dd_ui.import_extras`: `/Game/DD/UI/Main/TitleScreen/extras_play_icon`・`extras_pause_icon`・`locked`・`locked_-_Copy`、`/Game/DD/UI/RingAltar_UI/Textures/ring_altar_power_equipped_frame`。ほかに `/Game/DD/Audio/UI/UI_Select_V3`、`/Game/DD/UI/Fonts/helvetica-normal_Font`・`helvetica-neue-bold_Font`、エンジンの `WhiteSquareTexture`）。

## 原作データの根拠
- 木とスロット: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_Extras_Extra.json`・`UMG_Extras_Extra_Video.json`・`UMG_MaximizePicture.json`・`UMG_Extras_Sound_Button.json`・`UMG_Extras_Sound_Bar.json`（クラス側 `…_C.WidgetTree` の書き出し。スロットの書き出しに無い値は `CanvasPanelSlot` の既定〈Offsets (0, 0, 100, 30)〉、文字の大きさは UE 4 の既定 24）。結び付けはクラスの `Bindings`、ボタンのイベントは `ComponentDelegateBinding`。
- 流れ: `python Tools/dd/bp_flow.py pak_reference_2/_bytecode/DDeception/Content/UI/Main/TitleScreen/<名前>.txt <イベント>`。絵: `Construct`（@252）・クリック（@2511 → @10: `Create(UMG_MaximizePicture)` に `Texture `・`Text` を名前で入れ `AddToViewport(3)`）。動画: `PreConstruct`/`Construct`（@187: `Extras_Movies[ID]` の `Image` と `Unlocked?`）・クリック（@2514 → `UMG_MaximizeVideo`）。大きく見る画面: `Construct`（@305）・Back（@144・@15）・ホバー（@238・@196）。音のボタン: `PreConstruct`/`Construct`（@950）・クリック（@2043、DoOnce @29〜@938）・`Deselect`（@2409）・ホバー（@1485・@1764）・`GetText_0`。再生バー: `Construct`（@10）・`Set Sound`（@193）・`Pause`（@455）・`Resume`（@493）・`Destruct`（@531）・`CustomEvent_0`（@142）・`GetPercent_0`・`GetText_0`〜`_2`。
- 日記の解放の順位: `pak_reference_2/_assets/DDeception/Content/Blueprints/Save/BP_DD_SaveGame.json` の既定の `Level Ranks`（9 つとも `NewEnumerator4`）と `Blueprints/Enums/Enum_Ranks.json`（4 = None）。
- 本家の画面本体が部品をどう置くか（ID・絵・文字・音・再生バー・`Diary?` を部品ごとに持つ）: `UMG_Extras.json` の `UMG_Extras_Extra_C`・`_Video_C`・`_Sound_Button_C` の書き出し。画面本体の `Check If Playing` は再生中の音のボタンを `Deselect` し、そのバーの音を止める。

## 依存関係
- `UWasamiSaveGame`（`IsUnlocked`・`SlotName`・`UserIndex`。06 記録）、`WasamiWidgetAnimation.h`（09 記録）、`WasamiAssets.h`
- 使う側: EXTRAS の画面本体（項目 29 の次のステップ）
- エンジン: `UUserWidget`・`UWidgetTree`・`UButton`・`UCanvasPanel`・`UScaleBox`・`UImage`・`UTextBlock`・`UProgressBar`・`UBackgroundBlur`・`UAudioComponent`（`OnAudioPlaybackPercent`）・`UGameplayStatics`（`CreateSound2D`・`PlaySound2D`・`LoadGameFromSlot`）

## 既知の制約・注意点
- 本家の部品は Construct ごとに `SaveSlot` を読むが、本作は画面本体が読んだセーブを `Save` に渡せる（渡さなければ部品が読む）。本家はタイトルのゲームモードの `Check For Save` が `SaveSlot` を作ってから EXTRAS が押せるので、スロットが空のときは新しいセーブ（何も解放していない）として扱う。
- 本家の絵の `ClickedEvent`（ディスパッチャ）はどこからも呼ばれないので写さない。
- 音のボタンが押されたとき、本家はワールドの音のボタンをすべて `Deselect` する（別の一覧のものも）。テストのワールドの無いボタンどうしも互いに当たる（害は無い）。
- 日記の解放（`Level Ranks[ID]` が None のとき解放）は本家のままの判定で、本作ではいつも `ID` 0〜8 が解放になる。
- 動画は鍵のまま（`UMG_MaximizeVideo` は作らない）。

## 変更履歴
- 2026-09-20: 初版。並べる部品（絵・動画・音のボタン・再生バー）と大きく見る画面を足した（作業一覧の項目 29 のステップ 3）
