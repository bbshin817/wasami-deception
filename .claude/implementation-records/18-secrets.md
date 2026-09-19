---
title: 秘密と収集物
sources:
  - Content/Python/wasami_tools/pipeline/dd_secrets.py
  - Source/wasami_deception/WasamiCollectablesWidget.h
  - Source/wasami_deception/WasamiCollectablesWidget.cpp
  - Source/wasami_deception/WasamiMysteryNoteWidget.h
  - Source/wasami_deception/WasamiMysteryNoteWidget.cpp
  - Source/wasami_deception/WasamiCollectable.h
  - Source/wasami_deception/WasamiCollectable.cpp
  - Source/wasami_deception/WasamiSecretRoomZone.h
  - Source/wasami_deception/WasamiSecretRoomZone.cpp
  - Source/wasami_deception/WasamiSecretWall.h
  - Source/wasami_deception/WasamiSecretWall.cpp
  - Source/wasami_deception/WasamiMysteryCollectable.h
  - Source/wasami_deception/WasamiMysteryCollectable.cpp
  - Source/wasami_deception/WasamiFakeUseActor.h
  - Source/wasami_deception/WasamiFakeUseActor.cpp
  - Source/wasami_deception/Tests/WasamiSecretsTests.cpp
updated: 2026-09-20
---

# 秘密と収集物

## 役割
本家の病院の秘密と収集物（作業一覧の項目 12）。Zone 1 の秘密のエレベーター 2 つの奥と Zone 2 の秘密の部屋・迷路の後の秘密の書類（`BP_Collectable`。スコアの `SECRETS` の 4）、Zone 2 の秘密の部屋（`BP_SecretRoomZone`）と秘密の壁（`BP_07_Zone1_SecretWall`）、部屋のメモ 3 枚（`BP_MysteryCollectable`）、Zone 1 の見て使うエレベーター（`BP_FakeUseActor` の派生）。素材の取り込み（`dd_secrets.py`）、Zone 1 の秘密のエレベーターのシーケンス 2 本（01 記録の「シーケンス」）、画面 3 つ（書類の `NEW EXTRAS UNLOCKED!`・秘密の部屋の `YOU FOUND A MYSTERIOUS ROOM`・メモを読む画面）、書類・秘密の部屋の区域・秘密の壁・メモ・見て使う偽の部品（シーケンスを流すもの・おとりのエレベーター）のアクタ、両ゾーンへの置き方（`dd_level`）と迷路の後の書類（`AWasamiZone2Flow`）。PIE で確かめた（下の「確かめたこと」）。書類が本家の別のセーブに足す EXTRAS は作業一覧の項目 29。

## 公開インターフェース
- ツール: `WasamiDDTools.import_dd_secrets()`（素材。前処理 `Tools/dd/prepare_stage.py` と `WasamiStageTools.import_dd_stage_assets` を残りが 0 になるまで、`import_dd_tablet`・`import_dd_ui` の後に）。戻り値 `sounds` 5 / `textures` 6 / `meshes` 1 / `materials` 1。
- `dd_secrets.import_all()`・`make_glitch()`・`dress_secret_file()`。
- `UWasamiCollectablesWidget`（本家 `UMG_Collectables`）: `Show(WorldContext)`（`AddToPlayerScreen(0)`。最初のプレイヤーのコントローラーで作り、無ければ null）・`LoadAssets`・`Begin`（Construct）/`Advance`（ティック）・`GetIconIndex`・`EvaluateScale`/`EvaluateOpacity`/`EvaluateFlashOpacity`。
- `UWasamiCollectablesSecretWidget`（`UMG_Collectables_Secret`。上の派生）: `Show`（`AddToViewport(1)`）・`LoadAssets`。
- `UWasamiMysteryNoteWidget`（`UMG_MysteryNote`）: `Show(WorldContext, Texture, Texts, bLoreNote)`（`AddToViewport(2)`）・`Begin`/`Advance`・`PressNextPage`・`PressClose`・`GetCurrentText`・`GetPageText`・`Evaluate*`。
- `AWasamiCollectable`（本家 `BP_Collectable`）: `ID`（セーブの `Secrets` の番号）・`FileMesh`（`secret_file` のソフト参照）・`Collect()`（取る。DoOnce）・`IsTaken`・`GetBouncePosition`・`EvaluateBounce`/`BounceHeight`/`BounceYaw`・部品の取得。
- `AWasamiSecretRoomZone`（`BP_SecretRoomZone`）: `NotifyPlayerOverlap(bBegin)`（箱の重なりがプレイヤーのときだけ呼ぶ）・`GetGlitchOpacity`・`HasShownBanner`・`GetWhispers`・`GetGlitch`/`GetGlitchMaterial`。
- `AWasamiSecretWall`（`BP_03_SecretWall1` と子 `BP_07_Zone1_SecretWall` の既定値）: `IWasamiInteractable`（05 記録）・`Height`（275）・`IsUsed`/`IsMoving`/`GetMoveUpPosition`/`GetOGHeight`/`GetInterpHeight`・`EvaluateMoveUp`。
- `AWasamiMysteryCollectable`（`BP_MysteryCollectable`）: `IWasamiInteractable`・`Texture`（ソフト参照）・`Texts`・`bLoreNote`（`Lore Note`）・`GetLastNote`・`GetPlane`。
- `AWasamiFakeUseActor`（`BP_FakeUseActor`）: `IWasamiInteractable`・`bInactive`・`OnUsed`（`Used`）・`Activate()`・`IsUsed`・`GetBox`。派生 `AWasamiFakeUseSequencePlayer`（`_SequencePlayer`）: `Sequence`（レベルの `ALevelSequenceActor`）。派生 `AWasamiFakeUseElevator`（`BP_FakeUseActor_06_HospitalZone1_Elevator`）: `EvaluateDoors`・`DoorsStart`/`DoorsEnd`/`SequenceLength`/`DoorTravel`・`IsPlaying`/`GetSequencePosition`・部品の取得（`GetRightDoor` = `StaticMesh`・`GetLeftDoor` = `StaticMesh1`）。
- テスト `Wasami.Secrets.Widgets.Collectables`・`.Secret`・`.MysteryNote`、`Wasami.Secrets.Collectable.Parts`・`.Collect`・`.Save`、`Wasami.Secrets.SecretRoomZone`、`Wasami.Secrets.SecretWall`、`Wasami.Secrets.MysteryCollectable`、`Wasami.Secrets.FakeUse.Actor`・`.SequencePlayer`・`.Elevator`。

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
- 画面（木は本家の順に C++ で組み、アニメと `Delay` は画面のティックで進める。16 記録の `UMG_VignetteSides` と同じ作り）:
  - 書類の画面: `CanvasPanel_0`（1.2 倍・不透明度 0・当たりなし）→ `Image_297`（`WhiteSquareTexture` を赤に。画面の縁から少しはみ出す全面、不透明度 0）・`Image_89`（`extras_unlock_bg` 696×204 を中央に）・`Image_249`（絵 159×145 を中央の左 185 に）・`TextBlock_150`（helvetica-neue-bold 20、中央の左 90 から右へ）。`NewAnimation_1`（2.2 s）: 全体の拡縮 0 → 1.1（0.25 s）→ 1（0.4 s）、不透明度 0 → 1（0.25 s）…1（1.75 s）→ 0（2 s）、赤の閃き 0（0.15 s）→ 0.2（0.25 s）→ 0（0.6 s）。Construct: アニメ・`NEW EXTRAS UNLOCKED!`・乱数 0〜3 で `art_icon`・`diary_icon`・`sound_icon`・`movie_icon`（`SetBrushFromTexture(…, False)` で 159×145 のまま）・2 s で外す。
  - 秘密の部屋の画面: 木とアニメは同じで、絵が `T_MysteryRoom`、文字が 18 で中央の左 129・上 4。Construct: `DD_LVL2_15_V1_Secret_Mystery_Room_120818` を 0.5 で・`YOU FOUND A MYSTERIOUS ROOM`・2 s で外す。
  - メモの画面: `SizeBox_1`（1727×955）→ `ScaleBox_91` → `paper`（絵の大きさ。枠に収まるよう縮む）、`BackgroundBlur_0`（2）、`Image_0`（黒 0.798、当たりなし）、`SizeBox_0`（1547×724）→ `ScaleBox_0`（縮めるだけ）→ `RichTextBlock_0`（1400 で折り返し。本家の `MysteryText` の様式を画面が持つ表で: Default は helvetica-normal 18・黒の縁 2・(0.965, 0.965, 0.965)、Player は同じで (1, 0.900, 0.432)）、`Close`（背景が透明のボタンに `CLOSE`〈helvetica-neue-bold 36、白 0.5〉。右下の左 236・上 104）、`TextBlock_256`（ページ「n/N」= 本家 `GetText_0` の結び付け。helvetica-neue-bold 30・縁 4・幅 200 以上、下の中央の上 150）、`NextPage`（`selection_bar_arrow_hover` 21×32、普段は 0.703 の灰、中央の右 76・下 420）。`Open`（0.75 s）: 紙が下 1052 から −50（0.15 s）を経て 0（0.35 s）、角度 1.75° → −0.75° → 0、紙・黒・CLOSE・文・矢印の不透明度 0 → 1（0.35 s）、ページは 0.15 s から、ぼかし 0 → 3。`SwitchPage`（0.25 s）: 文の不透明度 0 → 1。
    - Construct（@183）: 紙に `Texture`（絵の大きさで）、UI だけの入力（この画面・`DoNotLock`）とカーソル、`Open`、ゲームを止める、`Update Text`（@1256: `Texts[currentText]`）、1 ページなら `NextPage` を外す、`E Note` なら `DD_LoreNote_01` を `CreateSound2D` で作って 0.5 s で上げる。
    - 矢印（@917）: `SwitchPage` を頭から、最後のページなら 0 へ・それ以外は次へ、`Update Text`。CLOSE（@823）: ゲームを動かし、ゲームだけの入力、カーソルを隠し、`Open` を逆に（UE の逆再生は `StartAtTime` を終わりから数えるので 0.4 s から 0 へ。0.05 s はそのまま）、0.5 s 後に外す（待っている間の 2 回目の `Delay` は効かない）。Destruct（@1361）: 音を 0.5 s で下げる。
- 書類 `AWasamiCollectable`: 部品は `DefaultSceneRoot` → `Box`（z 36.44・拡縮 (1.23, 1.30, 1)。UE の重なりの箱、道の外）・`StaticMesh`（z 36.44・60 倍・当たりなし。メッシュは `OnConstruction` が `FileMesh` を読み込んで入れる〈特殊シャードと同じ作り。レベルの組み立てが置くものも、迷路の後に流れが出すものも同じ〉）・`PointLight`（根の上 z 35.77、1000〈単位なし〉・250 cm・`SoftSourceRadius` 2000・影なし・Movable）。
  - BeginPlay: 取るときの声と画面の素材を読み込み、`Bounce` を 0 から、0.2 s のタイマーでセーブの確かめ（ゲームモードのセーブの `Hospital.Secrets` が `ID` を含めば `Destroy`）。
  - ティック（`Bounce`。5 s のループを 3 倍の速さ、位置は 5 で回る）: `NewTrack_0`（0・1・0 を 0・2.5・5 s、3 次で接線は 0）の値 v で、メッシュの相対位置 (0, 0, Lerp(35, 40, v))・ヨー Lerp(0, 7, v)。
  - 箱の重なり → 相手が `GetPlayerCharacter(0)` なら `Collect`: DoOnce → `CreateSound2D(Bierce_Secret_Files_Pickup, 0.85, 1)`・`Play`（ワールドの音なので書類が消えても鳴り続ける）→ `UWasamiCollectablesWidget::Show` → セーブの `Hospital.Secrets` に `AddUnique(ID)` → `Destroy`（灯も一緒に消える）。
  - 写さないもの: `Unlock` の EXTRAS（`Collectables` の型で本家の別のセーブ `SaveSlot` の `Extras_Art`・`Extras_SFX` に足して保存。作業一覧の項目 29 で作る。`TODO(item 29)`）と `Collectables` の変数、`Secret?`（真なら、ゲームステートの `Secrets Amount`〈どこも読まない〉を足し、レベルがホテルなら実績。セーブの `Secrets` への `AddUnique` は `Unlock` の終わりで `Secret?` に関わらず行う）、使われない `Audio` の部品、`Bounce` が待つ `DD_GameState` への cast。
- 秘密の部屋の区域 `AWasamiSecretRoomZone`: 部品は `DefaultSceneRoot` → `Box`（z 20・拡縮 (5, 2, 1.5)。Zone 2 の置き方は自分の大きさを持つ）・`Glitch`（本家の子のアクタ `Chameleon` の `InternalPP` の代わりの後処理。`bUnbound`・`BlendRadius` 0）。
  - BeginPlay: 画面の素材を読み込み、`CreateSound2D(67-Dark_Whispers_SFX_0704, 1, 1, 0, None, False, False)` を持つ（自動で消えない UI の音。テストのワールドでは作られない）。グリッチ `M_DD_ChameleonGlitch` の MID を作り、Chameleon の `Glitch Func` が入れる値（`Amount` 0.5〈`Glitch Blocking`〉・`Speed` 10・`Density` 30〈`Glitch Lines`〉・`GridDistortionPower` 0.001・`Size` 10・`Speed` 1〈Chameleon の既定〉）を入れて `AddOrUpdateBlendable(MID, 1)`、`BlendingOpacity` 0。
  - 箱の重なり（プレイヤーだけ）→ 入る: 囁き `FadeIn(1, 1, 0)`・`BlendingOpacity` 1・DoOnce で `UWasamiCollectablesSecretWidget::Show`。出る: `FadeOut(1, 0)`・`BlendingOpacity` 0。
  - 本家の Chameleon は毎ティック `InitChameleon` → `Glitch Func` → `Set Advanced Effect Features` で `Glitch - Advanced` を MID に入れ直すので、区域が構造体を書き換えると次のフレームで効く。ここは値を入れた時に MID へ直接書く（同じ見え方で 1 フレーム早い）。`Glitch - Advanced` のほかの項目（`BlendMode` 0・白いマスク 1 × 1・`BlendDistance` 0・鋭さ 10・カスタム深度とステンシルなし）は推定の材質が持つ枝そのものなので入れない。Chameleon の後処理は `Unbound` 真・重み 1・優先度 0。
- 秘密の壁 `AWasamiSecretWall`: 部品は `DefaultSceneRoot`（Movable）→ `StaticMesh`（100 倍・タグ `interact`・重なりを作らない・道の外〈子の `bCanEverAffectNavigation` 偽〉・UE の `BlockAllDynamic`・Movable。メッシュと材質はレベルの組み立てが入れる: 親の材質 `M_03_Manor_PaintedWall_01`、子の `M_07_TP_Stonewall_01`、Zone 2 の置いた壁の `M_06_Hospital_Brick_01`）。`Height` は子の 275（親は 0）。
  - BeginPlay: `OG Height` = 根の z、`InterpHeight` = `Height` + 根の z。
  - `InteractWithObject` → DoOnce → メッシュのタグを消す（`Array_Clear` を 2 回）→ `PlaySoundAtLocation(Sliding_Wall, アクタ, 0.65, 1, 0, 01_Lobby_Attenuation)` → `Move Up` を速さ 0.7 で頭から。ティックで位置を進め（タイムラインの長さは既定の 5 s。書き出しに `TimelineLength` が無い）、`Alpha`（0 → 1 を 3 s・線形、後は 1）で z を `Lerp(OG Height, InterpHeight, Alpha)`（掃引なし）。3 / 0.7 = 4.29 s で上がりきり、7.14 s でティックを止める（`Finished` は何もしない）。
- メモ `AWasamiMysteryCollectable`: 部品は `DefaultSceneRoot` → `Plane`（エンジンの `/Engine/BasicShapes/Plane` と `BasicShapeMaterial`〈エンジンのものなのでコンストラクタで引く〉、相対 (0, −0.0001, 215.42)・ロール 90 で立て・拡縮 (0.64, 1, 1)、タグ `interact`、UE の `BlockAllDynamic`〈見るトレースが Plane の箱の当たりに止まる〉）。Zone 2 に置いたメモは `Plane` を根に戻し（相対 0）、根の変形と `Plane` の材質を持つ（ステップ 5）。
  - BeginPlay: `Texture` とメモの画面の素材（`UWasamiMysteryNoteWidget::LoadAssets`）を読み込む。
  - `InteractWithObject`（DoOnce なし。画面がゲームを止め、閉じるまで次は押せない）→ `UWasamiMysteryNoteWidget::Show(this, Texture, Texts, bLoreNote)`（本家 `Create(Self, UMG_MysteryNote_C, None)` に名前で `Texture`・`Texts`・`E Note` を入れて `AddToViewport(2)`）。何も保存しない。
  - 写さないもの: CDO のアクタのタグ `interact`（置いたものは空にしてあり、アクタのタグを読むものも無い）、既定の紙 `sewer_note_01`。
- 見て使う偽の部品 `AWasamiFakeUseActor`: 部品は `DefaultSceneRoot` → `Box`（UE の 32 cm・根の上。`Custom` で `WorldDynamic`・`QueryAndPhysics`、`WorldStatic`・`WorldDynamic`・`Pawn`・`PhysicsBody`・`Vehicle`・`Destructible` を無視し、トレースの `Visibility`・`Camera` は既定の block〈本家の一覧は既定と違うものだけ〉。タグ `interact`・道の外）。プレイヤーは通り抜け、見るトレースだけが止まる。
  - BeginPlay: `bInactive` なら `Box` の当たりを `NoCollision`。`Activate` → `QueryOnly`。病院に `bInactive` のものは無く、`Used` を結ぶものも無い（両ゾーンのレベル BP に参照が無い）。
  - `InteractWithObject` → DoOnce → `OnUsed` を放送 → `UsedEvent`。親の `Used Event` はアクタを消す（`K2_DestroyActor`）。派生はどちらも親を呼ばずに置き換えるので、使った後も残り、`Box` のタグ（手のマーク）も残る（2 回目からは何もしない）。
  - `AWasamiFakeUseSequencePlayer`: `UsedEvent` → `Sequence->GetSequencePlayer()->Play()`（無ければ警告だけ）。Zone 1 の秘密のエレベーター 2 つ（`BP_FakeUseActor_2` → `06_Hospital_Zone1_SecretElevator`、`BP_FakeUseActor5` → `…SecretElevator1_2`）。
  - `AWasamiFakeUseElevator`: 部品は `Scene`（根の下、相対 (305, 70, −145)）→ `StaticMesh`（右の扉）・`StaticMesh1`（左の扉）・`Audio1`（相対 (0, 0, 160)・自動で鳴らない。BeginPlay で `DD_TT_Elevator_Doors_Open` と `01_Lobby_Attenuation` を入れる）。扉は UE の `BlockAllDynamic`・道の外、メッシュはレベルの組み立てが入れる（`hospital_elevator_doors_R_elevator_door`・`…_L_elevator_door_`）。
    - `UsedEvent` → `ActorSequence` を頭から（ティックで進める）→ `Audio1.Play(0)`（音は使った時に、扉は 2.23 s から）。本家の `ActorSequence`（5 s = 120000 ÷ 24000）は `StaticMesh1` の x を 0 → 145、`StaticMesh` の x を 0 → −145 を 53600〜119200（2.23〜4.97 s）で動かす 2 本の変形のトラック（キー 2 つの 3 次・自動の接線は両端で 0 = `3t² − 2t³`。回転 0・拡縮 1 もキー）で、区間は `KeepState`・範囲は無限。ティックは相対の変形ごと (±x, 0, 0) に入れ、5 s で止めて開いたまま。
- 置き方（レベルの組み立て `dd_level._flow`。01 記録。`place_dd_flow` で置き直す）: 本家のクラス → C++ の表 `SECRET_CLASSES`、`set_secret` が置いたものの値を入れる。書類は `ID`（Zone 1 の `BP_Collectable_2` は既定の 0・`BP_Collectable2_5` は 1、Zone 2 の `BP_Collectable_2` は 2）。区域は `Box` の相対の変形（Zone 2: (−95, 0, 20)・拡縮 (13.83, 16.22, 1.5)）。壁は `StaticMesh` の相対 (18.53, 0, 0) と `manor_fake_wall` に置いたものの材質 `M_06_Hospital_Brick_01`（根の拡縮 2.146 はアクタの拡縮）。メモは根の変形（回転・拡縮も）、`Texture`（`/Game/DD/Textures/06_Hospital/…`）、`Texts`（書き出しの文字列表の参照 `06_Hospital_Note_00`〜`02` を本家の `Blueprints/Main/Strings/Strings` の本文にした文字列。文字列表は取り込まない）、`Plane` の相対の変形（0 に戻す）と材質（`M_06_Hospital_MysteryRoom_Note_01`〜`03`）。おとりのエレベーターは扉 2 枚のメッシュ（右 `StaticMesh` = `hospital_elevator_doors_R_elevator_door`、左 `StaticMesh1` = `…_L_elevator_door_`、材質はメッシュのもの）。秘密のエレベーターの `Sequence` は `link_sequence_players` が書き出しの `Sequence` の名前（`06_Hospital_Zone1_SecretElevator`・`…SecretElevator1_2`）のタグ `src:` の `LevelSequenceActor` に結ぶ。シーケンスのアクタは `dd_sequence` が流れのアクタの後に置き直すので、`place_flow` の後と `dd_sequence.place_all` の終わりの両方で結ぶ。フォルダは `Hospital/Gameplay/Secrets`。書類の灯は書類の部品なので、前処理が書類の下に挙げる灯は置かない（`_lights` が飛ばし、`place_flow` が前の組み立てのもの〈`Hospital/Lights/BP_Collectable_C`〉を消す。どれも Movable で焼いた光に入っていない）。
- 迷路の後の書類（11 記録の `Postmaze Transition`）: `AWasamiZone2Flow::PostmazeTransition` が目印 `collec` の変形に `AWasamiCollectable` を `ID` 3 で出す（本家 `BeginDeferredActorSpawnFromClass(BP_Collectable, …, 2)` = `AdjustIfPossibleButAlwaysSpawn`・`SetIntPropertyByName(ID, 3)`）。チェックポイント 10 から始め直すたびに出て、セーブに 3 があれば 0.2 s 後に消える。
- ウィジェットのアニメの区間が途中で終わった後は、区間の最後の値が残る: 本家の BaseEngine.ini は `WidgetAnimation` の `DefaultCompletionMode` を設定せず、既定は 0 = `KeepState`（区間は `ProjectDefault` でそれに従う）。メモのぼかしは `Open` の後も 3（木の 2 に戻らない）。

## 作るアセット
- 音（`/Game/DD/Audio/…`）: `SharedGameplay/Bierce_Secret_Files_Pickup`（書類を取る）・`SharedGameplay/67-Dark_Whispers_SFX_0704`（秘密の部屋の囁き）・`SharedGameplay/DD_LVL2_15_V1_Secret_Mystery_Room_120818`（`UMG_Collectables_Secret` の曲）・`02_School/Sliding_Wall`（秘密の壁）・`Misc/DD_LoreNote_01`（`E Note` のメモ）。
- 絵（`/Game/DD/UI/Main/…`）: `Collectables/art_icon`・`diary_icon`・`sound_icon`・`movie_icon`（`UMG_Collectables` が乱数で 1 つ）・`Collectables/extras_unlock_bg`（両方の画面の枠）・`T_MysteryRoom`（`UMG_Collectables_Secret`）。
- `/Game/DD/Meshes/Shared/secret_file` の枠 2 つに `/Game/DD/Materials/Shared/MM_Shared_Secret_Folder`。
- `/Game/Pipeline/Materials/M_DD_ChameleonGlitch`（後処理。スカラー 12〈`Density` 1・`Speed` 10・`RandomSeed` 1・`Amount` 0.5・`Pow1` 7・`Pow2` 3・`Pow3` 18・`Blockeffect` 1・`GridDistortionSpeed` 1・`GridDistortionSize` 5.344284・`GridDistortionPower` 0.01・`BlendingOpacity` 1〉、ベクトル 2〈`DotValue`・`DotValue2`〉）。
- ステージの素材として（前処理の `CLASS_MESHES`・`CLASS_MATERIALS`。01 記録）: `/Game/DD/Meshes/Shared/secret_file`・`/Game/DD/Meshes/03_Manor/manor_fake_wall`、材質 `MM_Shared_Secret_Folder`・`M_06_Hospital_Brick_01`・`M_06_Hospital_MysteryRoom_Note_01`〜`03` とテクスチャ 4（`secret_file_01_D`・`mysteryroom_hospital_note_01`・`_02`・`mysteryroom_note_prescription_01`。メモの画面の紙にもなる）。
- Zone 1 のシーケンス（`dd_sequence`。01 記録）: `/Game/DD/Animation/06_Hospital/06_Hospital_Zone1_SecretElevator`（扉 `hospital_elevator_doors_L_elevator_door_2`・`R_elevator_door2` と音の目印 `secret_elevator_sound`）・`…SecretElevator1`（扉 `R_elevator_door3`・`R_elevator_door4` と同じ目印。本家どおり 1 つ目のエレベーターの目印で鳴る）。どちらも 5 s。アクタは `src:06_Hospital_Zone1_SecretElevator`・`src:06_Hospital_Zone1_SecretElevator1_2`。

## 原作データの根拠
- 部品と音・絵: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_Collectable.json`（`StaticMesh` の `secret_file`・`Audio` の `Bierce_Secret_Files_Pickup`）、`Blueprints/Shared/BP_SecretRoomZone.json`（子のアクタ `Chameleon` の値 `Glitch` 真・`Glitch Speed` 10・`Glitch Lines` 30・`Glitch Blocking` 0.5・`BlendingOpacity` 0）、`Blueprints/02_School/BP_03_SecretWall1.json`（`manor_fake_wall`）、`Blueprints/Main/BP_MysteryCollectable.json`（`Plane` はエンジンの `Plane`）、`UI/Main/UMG_Collectables.json`・`UMG_Collectables_Secret.json`・`Blueprints/UMG/UMG_MysteryNote.json` の参照。
- 画面: `_assets/DDeception/Content/UI/Main/UMG_Collectables.json`・`UMG_Collectables_Secret.json`・`Blueprints/UMG/UMG_MysteryNote.json`（木は `…_C.WidgetTree` の側、アニメは `MovieScene` の区間と `AnimationBindings`〈`Image_92` の結び付けは `Image_0`、`TextBlock_1` は `RichTextBlock_0`〉、`GetText_0` の結び付けはクラスの `Bindings`）と `Blueprints/UMG/MysteryText.json`（`rows`）。流れは `_bytecode/…/UMG_Collectables.txt`・`UMG_Collectables_Secret.txt`・`UMG_MysteryNote.txt`（`python Tools/dd/bp_flow.py <file> Construct` ほか。`Update Text` と `GetText_0` は関数の本体を読む）。Z 順は `Blueprints/Main/BP_Collectable.txt`（`AddToPlayerScreen(0)`）・`Blueprints/Shared/BP_SecretRoomZone.txt`（`CreateAndAddWidget(…, None, 1)`）・`Blueprints/Main/BP_MysteryCollectable.txt`（`AddToViewport(2)`）。逆再生の始まりは UE の `UMG/Private/Animation/WidgetAnimationState.cpp`、区間の後の値は `MovieScene/Public/Evaluation/MovieSceneCompletionMode.h` と各区間の `EnableAndSetCompletionMode`。
- メモと偽の部品: `_assets/DDeception/Content/Blueprints/Main/BP_MysteryCollectable.json`（`Plane_GEN_VARIABLE`・CDO の `Texture`・`Tags`）・`BP_FakeUseActor.json`（`Box_GEN_VARIABLE` の当たりとタグ）・`BP_FakeUseActor_SequencePlayer.json`・`Blueprints/06_Hospital/BP_FakeUseActor_06_HospitalZone1_Elevator.json`（部品と `ActorSequence` の `MovieScene`・2 本の `MovieScene3DTransformSection`）、流れは `_bytecode/…` の同じ名前の `.txt`（`python Tools/dd/bp_flow.py <file> InteractWithObject`・`ReceiveBeginPlay`・`Activate`・`"Used Event"`）。置いたものは `_levels/06_Hospital_Zone_01.full.json`（`BP_FakeUseActor*`）・`06_Hospital_Zone_02.full.json`（`mysteryroom_note_01`〜`03`）。
- グリッチ: `ThirdParty/Chameleon/Materials/M_GlitchHLSL.json`（パラメータの既定）と最新版の pak のコンパイル済みシェーダー（上）。Chameleon の既定（`ThirdParty/Chameleon/Chameleon.json`）は `Glitch Grid Distortion Power` 0.001・`Size` 10・`Speed` 1。Chameleon の `Glitch Func`（`_bytecode/…/Chameleon.txt`）が `Amount` ← `Glitch Blocking`・`Speed` ← `Glitch Speed`・`Density` ← `Glitch Lines`・`GridDistortion*` ← 同名の値を入れる。

## 依存関係
- `dd_assets`（音・絵・材質・パラメータの既定）、`dd_stage._Graph`、`paths`。
- 画面: `WasamiWidgetAnimation.h`（キーから曲線）、`WasamiAssets.h`。
- アクタ: ゲームモードのセーブ `UWasamiSaveGame::Hospital.Secrets`（06 記録。スコアの `SECRETS` が数える、13 記録）、見て使う仕組み `IWasamiInteractable` とプレイヤーのトレース（05 記録）、画面 3 つ（上）、グリッチの材質（上）、レベルのシーケンス（`ALevelSequenceActor`。モジュール `LevelSequence`）、エンジンの `Plane`・`BasicShapeMaterial`。書体 `helvetica-neue-bold_Font`（`import_dd_tablet`）・`helvetica-normal_Font`（`dd_ui`）、矢印 `selection_bar_arrow_hover`（`import_dd_ui`）、メモの絵（ステージの素材）。
- ステージの素材（`Tools/dd/prepare_stage.py` → `import_dd_stage_assets`）、`import_dd_tablet`（書体 `helvetica-neue-bold_Font`）、`import_dd_ui`（矢印 `selection_bar_arrow_hover`）。
- 使う側: 書類・秘密の部屋の区域・メモのアクタ（画面とグリッチ）。レベルの組み立て（`dd_level._flow`・`link_sequence_players`、`dd_sequence.place_all`）と迷路の後の書類（`AWasamiZone2Flow::PostmazeTransition`）。

## 既知の制約・注意点
- グリッチは推定（大目標 1・2 の決め方。本家の画面とは見比べていない）。本家の絵と並べて詰めるのは作業一覧の項目 28。
- 書類の画面は本家どおり出すが、EXTRAS（本家の別のセーブの `Extras_Art`・`Extras_SFX`）はまだ無いので、引いた絵は何にも残らない（`TODO(item 29)`）。2026-09-20 のユーザーの回答で、EXTRAS は作ってタイトル画面から見られる形にする（中身は枠組みだけ先に作る）。作業一覧の項目 29。
- メモの画面の木の既定の紙 `sewer_note_01`（下水道のメモ）は取り込まない（Construct が `Texture` を入れる）。本家の `Virtual Cursor`（ゲームパッド）はほかの画面と同じく写さない（09 記録）。`SetInputMode_UIOnlyEx` に画面を渡すと焦点を持てない警告が出るのは本家どおり（15 記録）。
- テストで画面の木を作るときは `TakeWidget()` の戻り値を持つ。リッチテキストはスレートの木と一緒に様式を放すので、持たないと `GetDefaultTextStyle` が ensure に当たる。
- 書類はセーブへの書き込みを赤いシャードと同じくメモリの上だけで行い、ディスクへはチェックポイントの保存で書く（本家どおり）。チェックポイントの前に死んで開き直すと書類はまた出る。
- テストのワールドのプレイヤーのコントローラーはローカルのプレイヤーを持たないので、書類と区域の画面は作られず `PlayerController_0` のエラーが出る（テストは `AddExpectedError` で受ける）。
- `MM_Shared_Secret_Folder` の親 `MM_Main_Substance_Fresnel` は前処理で `fresnel` になり、コンパイル済みのシェーダーの式で組んだ `M_DD_SubstanceFresnel` に載る（`Fresnel Setting` (1, 1, 1) の白い縁の光。2026-09-20、作業一覧の項目 31。01 記録）。
- おとりのエレベーターの `ActorSequence` は `ActorSequenceComponent` を使わずティックで写した（プラグイン `ActorSequence` をモジュールに足さず、キー 2 つの曲線は式で足りる）。変形のトラックが扉の相対の変形を丸ごと入れるので、扉の相対の変形を置き場で変えても使うと (±x, 0, 0) に戻る（本家も同じ。置いたものは変えていない）。
- メモの `Plane` の当たりはエンジンの `Plane` の厚み 0 の箱（100 × 100 × 0）。エディタのワールドと PIE では見るトレースが止まるが、テストのワールドのトレースは静的メッシュの体を拾わないので、テストは `LineTraceComponent` で確かめる（症状索引）。
- テストのワールドにはプレイヤーの画面が無いので、メモの画面が作られるかはテストで確かめられない（`GetLastNote` が null になることだけ）。画面の中身は `Wasami.Secrets.Widgets.MysteryNote`、PIE での読みは下の「確かめたこと」。
- Zone 2 の秘密の壁と部屋は 2 階（床 z 約 500）にあり、道の網は 1 階とつながらない。本家は Zone 2 のテレポートの床 `hospital_zone_02_teleport`（1 階と 2 階の 2 層）で上がる作り。本作でも迷路の入口の上の 2 階の歩き道（(−3300, 0)・(−2500, 550)、z 531）に Teleport のチャンネルの上の層があり、そこから 2 階を西へ歩くと壁の前 (−3560, 0, 503) に着く（2 階の道の網は一続き）。

## 確かめたこと（2026-09-20、PIE）

- 置いたもの（`Wasami.ResetSave` の後）: Zone 1 に書類 ID 0・1、見て使う偽の部品 7（秘密のエレベーター 2・おとり 5）。Zone 2 に書類 ID 2、壁・区域、メモ 3、チェックポイント 10 から始めると目印 `collec` (−6755, −1014, 87) に書類 ID 3。
- Zone 1 の秘密のエレベーター 2 つ: 呼びボタン（`BP_FakeUseActor_2`・`5`）に手のマーク → 左クリックで扉が開き（シーケンス）、奥の書類を踏むと `NEW EXTRAS UNLOCKED!` とセーブの `Secrets` に ID。おとり（`BP_FakeUseActor2`）は扉が開いて空の箱のまま止まり、手のマークは残る（本家の派生は親の `Used Event` を呼ばない）。
- Zone 2: 壁は 1 クリックで上がり（約 4.3 s）、部屋に入ると `YOU FOUND A MYSTERIOUS ROOM`・囁き・グリッチ。出ると囁きが止み、グリッチの `BlendingOpacity` が 0。メモ 3 枚はどれも 1 ページ（1/1）で、開くとゲームが止まってカーソルが出て、CLOSE で戻る。書類は宙に浮く紙挟み（灯つき）。
- 書類はメモリの上のセーブに足すだけで、チェックポイントの保存（`Wasami.Checkpoint`）の後に死ぬと開き直しても戻らない（迷路の後の ID 3 も出た直後に消える）。
- 4 つそろえて脱出（`Wasami.Escape`）すると、スコア画面が `SECRETS 4/4`（S・+35）。
- 画面の操作の注意: PIE でクリックの位置を変えるとカーソルの移動が視点を回すので、見て使う物を狙うときは、狙いを入れてから前のクリックと同じ位置を押す。

## 変更履歴
- 2026-09-20: 作業一覧の項目 12 を閉じた（役割の「作っている途中」を外した）
- 2026-09-20: 書類の EXTRAS の要確認に回答が出た（EXTRAS を作り、タイトル画面から見られる形に。中身は枠組みだけ先に）。`TODO(仮)` を作業一覧の項目 29 を指す `TODO(item 29)` にした
- 2026-09-20: PIE で両ゾーンの秘密を確かめた（「確かめたこと」。作業一覧の項目 12 のステップ 6）
- 2026-09-20: 両ゾーンに置いた（`dd_level` の `SECRET_CLASSES`・`set_secret`・`link_sequence_players`）。書類のメッシュを `OnConstruction` で入れるようにし、迷路の後の書類 ID 3 を Zone 2 の流れが出す（作業一覧の項目 12 のステップ 5）
- 2026-09-20: メモ `AWasamiMysteryCollectable` と見て使う偽の部品 `AWasamiFakeUseActor`・`AWasamiFakeUseSequencePlayer`・`AWasamiFakeUseElevator`、テスト 4 件を足した（作業一覧の項目 12 のステップ 4）
- 2026-09-20: 書類 `AWasamiCollectable`・秘密の部屋の区域 `AWasamiSecretRoomZone`・秘密の壁 `AWasamiSecretWall` とテスト 5 件を足した（作業一覧の項目 12 のステップ 3）
- 2026-09-20: 画面 3 つ（`UWasamiCollectablesWidget`・`UWasamiCollectablesSecretWidget`・`UWasamiMysteryNoteWidget`）とテスト `Wasami.Secrets.Widgets.*` を足した（作業一覧の項目 12 のステップ 2）
- 2026-09-20: 初版。素材の取り込み `dd_secrets.py` と Zone 1 の秘密のエレベーターのシーケンス 2 本（作業一覧の項目 12 のステップ 1）
