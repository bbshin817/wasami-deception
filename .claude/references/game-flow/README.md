# 死亡・再開・セーブの流れ（原作調査）

2026-09-18 に、本家の病院（Torment Therapy）の死亡から再開まで・ライフ・チェックポイントとセーブ・死亡画面・`SAVING PROGRESS` を原作データから調べた資料（作業一覧の項目 5 のステップ 2）。実装の説明ではない（それは `.claude/implementation-records/`）。

- 根拠はすべて **`pak_reference_2`（最新版）** のバイトコード `pak_reference_2/_bytecode/DDeception/Content/` と、アセット `pak_reference_2/_assets/DDeception/Content/`。`@番地` はそのファイルの番地。病院は最新版にしか無い。
- 読み方: `python Tools/dd/bp_flow.py <file.txt> <イベント名 | 番地>`（入口から制御の流れで読む。01 記録）。
- 死亡画面は旧版（v1.6.1。WebGL 版 10 記録の `death.ts` の元）から Construct が大きく変わった（28,379 → 48,376 バイト。`Get Current Progress`・`Local Lives`・ゲームステートの `Lives Lost`・セーブの `Deaths`・声の選び方・`Image_0` → `YouAreDead`）。**アニメ 4 本（Fade In・Fade Out・Shake・Death）のキーと時間は WebGL 版 10 記録の値と同じ**（下の「アニメ」）。

## 全体の流れ（捕まってから再開まで）

| 順 | どこで | すること | 根拠 |
| --- | --- | --- | --- |
| 1 | 敵（項目 9） | 捕獲の演出（視点を敵へ 0.15 s で移す・カメラアニメ・揺れ）の後、`UMG_BlackFade_3`（`Disappear After` 5）を画面に足し、`GameMode.DeathEvent(Self)` | `Blueprints/Characters/Nurse/BP_06_ReaperNurse.txt` の `Jumpscare Handle`（@2248〜@2471） |
| 2 | ゲームモード `DeathEvent(Cause)` | `DoOnce`（`Reset Death` で開く）。Bierce の独り言のタイマーを止め、**`Death Dispatcher` を流す**。`Shard Streak` = max(`Shard Streak`, セーブの `CurrentStreak`)、セーブの `CurrentStreak` = 0 | `Blueprints/Main/BP_DD_GameMode.txt` @34486 → @33223 → @15098 |
| 3 | レベル BP `DeathEvent(Cause)`（Setup で `Death Dispatcher` に結ぶ） | `DoOnce`。`UMG_DeathScreen` を作り、`Level` を入れ（声の選び分け。下）、`Respawn Event` → レベルの `Respawn` を結び、**`AddToViewport(5)`・`SetGamePaused(true)`** | `06_Hospital_Zone_01.txt` @14791 → @5731、Zone 2 は @23519 → @5758 |
| 4 | 死亡画面 Construct | ライフ −1、死亡数 +1 を保存し、画面を進める（下の「死亡画面」） | `Blueprints/UMG/UMG_DeathScreen.txt` @43730 |
| 5 | 死亡画面（ライフ > 0） | 声が終わるか 6 s → 0.5 s → Fade Out → 2 s → プレイヤーの **`Reset Powers`**、`Respawn Event` を流し、**今のレベルを `OpenLevel` で開き直す** | 同 @2743 → @15 → @117 |
| 6 | レベル BP `Respawn` | 画面を外し、ポーズを解き、`EnableInput`、`GameMode.Reset Death`、`Spawn`。ただし直後に 5 の `OpenLevel` でレベルごと読み直すので、実際の再開は読み直したレベルの `Spawn` | Zone 1 @14796 → @8167、Zone 2 @23764 → @6222 |
| 7 | 読み直したレベル | ゲームモードの BeginPlay が 0.2 s 後に回収済みのシャードを消し（下の「シャード」）、レベルの `Spawn` がセーブのチェックポイントの場所へ移して、その区間の準備をやり直す | 下の「チェックポイント」 |

- **再開は「レベルの開き直し」**。敵・扉・曲・シャードはレベルの初期状態に戻り、残るのはゲームインスタンス（ライフ・回収済みのシャード）とディスクのセーブ（チェックポイント・死亡数）だけ。敵をリセットする特別な処理は無い。
- 死亡の途中でプレイヤーの入力を明示的に止める・タブレットを下ろす処理は無い（敵が視点を奪い、レベル BP がゲームを止め、死亡画面の黒が覆う）。
- `Death Dispatcher` を結ぶのはレベル BP だけ（病院では Zone 1・2 の Setup。@2111）。敵は結ばない。

## 状態の持ち主

| 持ち主 | 項目 | 使われ方 |
| --- | --- | --- |
| ゲームインスタンス `BP_DD_GameInstance`（レベルを開き直しても残る。ディスクには書かない） | `Lives` | ライフ。`BP_DD_Functions` の `Get Lives`・`Decrement Lives`（−1、0..6 に Clamp）・`Increment Lives`（+1、同）・`Reset Lives` |
| | `Shards To Be Removed`（`FVector` の配列） | 回収したシャードの開始位置（整数に切り捨て）。レベルの読み直しで消す |
| | `Hard Check Point`・`Used Hard Respawn?`・`Game Instance Time` | 入口レベルの仕組み・S ランクの判定・通しの時間（病院の Zone では書かない） |
| レベルのセーブ `BP_DD_levelStructSave`（スロット `structSlot`） | `levelStruct`（`DD_LevelStructureyyy` の配列 11 個。既定はすべて 0） | レベルごとの進み。**病院は添字 5**（`Enum_Levels` の Asylum = 7 → 添字の表 `Get Level Struct Index`: 0→0, 1→1, 2→2, 3→0, 4→0, 5→3, 6→4, 7→5, 8→6, 9→7, 10→8, 11→9）。添字 10 はどのレベルも使わない空の項目で、RESTART がそれで上書きして消す |
| `DD_LevelStructureyyy` | `LevelCheckpoint`（int）・`Deaths`（int）・`Time`（float）・`CurrentStreak`（int）・`Streak`（byte の enum）・`BonusShards`・`Secrets`・`AchievementData`（配列） | チェックポイントの通過で `LevelCheckpoint` と `Time`、死亡画面で `Deaths` と `CurrentStreak`、シャードの回収で `CurrentStreak`（`Check Streak`）。書くたびに `SaveGameToSlot(Struct Save, 'structSlot')` |
| 全体のセーブ `BP_DD_SaveGame`（スロット `SaveSlot`） | `Player Level`・`Total Lives`・`Progress`・`Last Checkpoint Warning` ほか | ライフの上限、タイトルの続き、LAST CHECKPOINT の S ランクの警告を 1 度出したか |
| ゲームモード `BP_DD_GameMode` | `Struct Save`（読んだセーブ）・`Level`（`Enum_Levels`）・`Time`・`Total Shards`・`Shard Streak`・`Current Objective` | BeginPlay の `Check For Level Struct Save`（@40634）がスロットを読むか、無ければ作って書く |
| ゲームステート `DD_GameState` | `Lives Lost` | 死亡画面が +1 |

- **時間**: ゲームモードの Tick が `Time += dt`（ゲートが開いている間。ポーズ画面が `Pause Time Counter` / `Unpause Time Counter`）。チェックポイントの通過で `levelStruct.Time += Time` にして `Reset Time Counter`（`Time` = 0）。
- **ライフの数**（`Reset Lives`、`Blueprints/Macros/BP_DD_Functions.txt`）: `SaveSlot` の `Player Level` 0〜4 → 3、5〜9 → 4、10〜14 → 5、15〜16 → 6。読めた値が 3 未満なら 3、セーブが無ければ 3。本作にプレイヤーのレベルは無いので **3**。連続回収の 200・500 で +1（上限 6。`UMG_ShardStreak`、WebGL 版 10 記録）。
- ゲームモードの `Reset Game Instance(Replay)`（@38662）: `Shards To Be Removed` を空に、`Lives` = `Total Lives` の後 `Reset Lives`、ほかのフラグ（オーブ・秘密・連続回収）を戻す。

## チェックポイント（`LevelCheckpoint`）と再開の場所

レベルの `Spawn` は `Load Progress By Level(7, Fake)` でセーブの `levelStruct[5].LevelCheckpoint` を読み、値ごとに場所と準備を決める。`Fake` は**パッケージしていないときだけ**その値を返す開発用の近道（Zone 1 は 5、Zone 2 は 8、入口は −1 = 近道なし）。

| 値 | レベル | 再開の場所（PlayerStart の名前。取り込み済みでタグに入っている） | 準備 | そこへ書く場面 |
| --- | --- | --- | --- | --- |
| 0〜3 | 入口 `06_Hospital` | — | — | 入口の場面（本作は作らない） |
| 4 | Zone 1 | `04_Start` | エレベーターの到着（扉 `BP_06_DoubleDoors11` を Lock、シーケンス `ElevatorArrive`、揺れ、7 s 後に揺れを止めてインターコムと扉の破壊を結ぶ） | 入口の `03_ElevatorEnter`（`06_Hospital.txt` @80127。SAVING PROGRESS の後に Zone 1 を開く） |
| 5 | Zone 1 | `05_Start` | `05_Persistent`（ナース 3 体を出す・目的 COLLECT ALL SHARDS・全回収を結ぶ・1 s 後に `Check Shards`） | `05_Transition`（扉の破壊の後の `BP_04_Trigger_Maze`。@16695 → @6119） |
| 6 | Zone 1 | `06_Start` | `06` のナース 2 体・扉のロックと救急車の上のトリガーを結ぶ・矢印と目的 REACH THE TUNNEL | 書く場面が無い（`06 Transition` は駐車場の場面の後にそこへ移すだけで保存しない） |
| 7 | Zone 2 | `PlayerStart_1` | `Arrive Event`（到着の場面） | Zone 1 の `06_ReachAmbulance`（救急車の上。@17329 → @10732。保存の後に敵を消し、目的 GOOD LUCK、救急車が出て 8 s 後に `UMG_Loading`、さらに 2.5 s で Zone 2 を開く） |
| 8 | Zone 2 | `PlayerStart_MiniBoss` | `Miniboss Start `（名前の後ろに空白。`Activate MiniBoss Enemies`・目的 Get past the nurses） | `Miniboss_Trigger_Transition`（@25096） |
| 9 | Zone 2 | `PlayerStart_Maze` | `Maze Start`（ナースを出す・目的 COLLECT ALL SHARDS） | `Maze Trigger Start`（@22245） |
| 10 | Zone 2 | `PlayerStart_PostMaze` | `Postmaze Start`（目的 COLLECT THE RING PIECE） | `Maze All Shards`（@21854） |
| 11〜12 | ボス戦 `06_Hospital_Bossfight` | — | — | Zone 2 の `Postmaze_Trigger_Ambulance`（@21531。本作はボス戦を作らない） |

- どの Zone の `Spawn` も、0 なら入口 `06_Hospital` を開き、表に無い値なら何もしない（その Zone の既定の場所のまま）。入口の `Spawn`（@81063）は 4〜6 で Zone 1、7〜10 で Zone 2、11〜12 でボス戦を開く（タイトルの続きもここを通る）。
- `Spawn` の頭で必ず `SetViewTargetWithBlend(プレイヤー, 0, …)` と `EnableInput` をする（Zone 1 @13483、Zone 2 @22328）。
- **チェックポイントの保存**（Zone 1 の 2 か所・Zone 2 の 4 か所。どれも同じ形）: `CreateAndAddWidget(UMG_Saving, Z 0)` → `levelStruct[5].LevelCheckpoint` = 値 → `Time += ゲームモードの Time` → `Reset Time Counter` → `SaveGameToSlot('structSlot')` →（開発用の `PrintString 'Progress Saved'`。画面に出るのは SAVING PROGRESS）→ その場面の続き。

## `SAVING PROGRESS`（`UI/Main/UMG_Saving`）

- 木: `CanvasPanel_0` の右下に 2 つ。`TextBlock_232`「SAVING PROGRESS」（`RobotoTiny` の Light、大きさは既定。アンカー (1, 1)、オフセット Left −300・Top −60、自動の大きさ）と `Throbber_207`（1 片、横・縦の動きは偽〈不透明度の明滅は既定のまま〉、`SphereRenderHeightMap` 25 × 25。アンカー (1, 1)、Left −332・Top −56）。どちらも `RenderOpacity` 0。
- Construct: `init`（3 s）を再生して、Delay 3 で `RemoveFromParent`。`init` は 2 つの `RenderOpacity` を 0 → 0.5（0.5 s）→ 0.5（2.0 s まで）→ 0（2.5 s）→ 0（3 s）。

## 死亡画面（`Blueprints/UMG/UMG_DeathScreen`）

### Construct（@43730）の時間の流れ

時刻は Construct からの秒。ゲームは止まっている（`SetGamePaused`）。

1. すぐ: `Decrement Lives`、`Lives Lost` +1、`Set Tip`（ヒント）、`Local Lives` = 残りのライフ、`levelStruct[5].Deaths` +1・`CurrentStreak` = 0 を `structSlot` に保存。`LivesArray` = [`Life1`..`Life6`]。
2. 声を選ぶ: `Level` の配列（下）から、まだ選んでいない番号を無作為に 1 つ（`CreateSound2D`）。
3. **0.5 s**: ライフ > 0 なら `Life_Lost`（音量 0.6）。
4. **0.5 s**（声の Delay）:
   - **ライフ > 0**: 声を再生し、終わりで `Proceed`。`Fade In` を速さ 1.5 で再生、`Life Animation`（下）。**6 s** の Delay（声と早い方。`DoOnce`）→ ライフ > 0 を確かめて **0.5 s** → `Fade Out`（速さ 1）→ **2 s** → プレイヤーの `Reset Powers` → `Respawn Event` → 今のレベルを `OpenLevel`。
   - **ライフ 0**: 声は鳴らさない。`Fade In`（1.5）。難易度が Easy（`Global Settings Save Instance.Difficulty` = 0）なら `Life Animation` だけで先へ進まない（本作は作らない）。それ以外は**ゲームオーバー**: `Life1`〜`Life6`・見出し・ヒントを隠し、**0.25 s** → `66_-_Game_Over`（0.7）・Bierce の `Bierce_Game_Over_01_Cue`（熊のレベルは Malak）・`Death` を再生・`SetInputMode_UIOnlyEx` → **2 s** → RESTART と QUIT TO TITLE を Visible、LAST CHECKPOINT は `Hard Check Point` > 0 か `LevelCheckpoint` > 0 のときだけ Visible（どちらも 0 なら `RemoveFromParent`）、マウスカーソルを出す。熊のレベルでなければ `Death` の 1.25 s 後に `Bierce_Additional_Laughter_01`。
- `Life Animation`（@46420）: 添字 `ライフ + 1`〜5 のアイコンを外し（残りのライフ + 1 個が並ぶ）、最初に外した 1 s 後に `Shake`（ループの中の Delay なので 2 回目以降は無視される。ライフが 5 以上だと外すものが無く、`Shake` も出ない）。`Shake` の 0.1 s のイベントが `Update Life`（添字 `ライフ` のアイコン = 失った 1 個を外す）。
- `Level`（声の選び分け）: レベル BP が入れる。Zone 1 は原因がプレイヤー自身なら 4（Traps）、ほかは 7（Asylum）。Zone 2 は原因がプレイヤーか `BP_GremClown` なら 7、ほかは 4（Zone 1 と逆。原作のまま）。声の配列は `BierceDeathAsylum`（`Bierce_Asylum_Death_01`〜`04`）・`BierceDeathTraps` ほか。
- ヒント `Tips`（`Set Tip` @47062。レベル名で分け、まだ出していないものから無作為）:
  - Zone 1（と入口）: 「Listen for skating sounds to determine if any Reaper Nurses are close to you.」「Use double doors to reveal the location of cloaked Reaper Nurses. Be careful though - the nurses can hear you opening them as well.」「Don't always do what you're told. The Reaper Nurses should not be trusted. (hospital tests)」
  - Zone 2: 上の 2 本と「Use your tablet to check if you are within each Reaper Nurse's view. Avoid being detected.」「Watch your step. Saw blades in the floor can be difficult to see.」
  - 該当しないレベル: 「Try not to die next time.」

### ゲームオーバーの 3 つのボタン

| ボタン | 押すと | 根拠 |
| --- | --- | --- |
| RESTART | `UMG_PopUp`（「ARE YOU SURE YOU WANT TO RESTART?」、Z 5）。YES で: ポップアップの Close Animation、`Used Hard Respawn?` = 偽・`Hard Check Point` = 0、`Reset Game Instance(False)`、`Reset Lives`、カーソルを消し、`levelStruct[今のレベル]` = `levelStruct[10]`（空）で保存 → `Fade Out` → 1 s → `SetInputMode_GameOnly`、`Shards To Be Removed` を空に、今のレベルを `OpenLevel`（チェックポイント 0 なので、Zone は入口を開く） | @46192 → @36419、YES は @47057 → @36706 → @25218 → @4742 → @2133 |
| LAST CHECKPOINT | `SaveSlot` の `Last Checkpoint Warning` が偽なら、`UI_Select_V3` と `UMG_PopUp`（Frame 2、「Obtaining S Rank is not possible with Last Checkpoint.\r\n \r\nContinue anyway?」、Z 10）。YES で警告を真にして保存。真なら確かめずに: `Used Hard Respawn?` = 真、カーソルを消し、`Game Instance Time += 今の時刻` → `Fade Out` → 1 s → `SetInputMode_GameOnly`、**`Reset Lives`**、今のレベルを `OpenLevel`（回収済みのシャードは残る） | @46682、YES は @46975 → @46828 → @13865 → @4547 |
| QUIT TO TITLE | `Used Hard Respawn?` = 偽・`Hard Check Point` = 0、カーソルを消し、`Shards To Be Removed` を空に → `Fade Out` → 1 s → `OpenLevel('TitleScreen')` | @46109 → @36950 → @2104 |

- ボタンのホバーの 6 つのイベント（@46202 ほか）は文字の色だけを変える。

### ウィジェットの木と素材

- `CanvasPanel_0` の子: `Button_22`（黒い地。中に `HorizontalBox_114` と `Life1`〜`Life6`〈`/Game/UI/Main/life_icon_02`、100 × 100〉）、`Image_161`（赤いビネット。`/Game/UI/Menu/Streaks/T_Vignette`、1920 × 1080、Tint linear (0.38, 0, 0)、α 0）、`TextBlock_149`「REMAINING LIVES:」（`helvetica-neue-bold_Font` 50）、`Tips`（`RobotoTiny` の Light、linear 0.62）、`YouAreDead`（`/Game/UI/Main/you_are_dead`、1285 × 301、`Scale` 1.05、α 0）、`Button_0`（黒い覆い。`Fade In` / `Fade Out` の相手）、`VerticalBox_161` に `Restart`・`QUITTOTITLE`・`LastCheckpoint`（ボタンの色 linear 0.115、文字「RESTART」「QUIT TO TITLE 」「LAST CHECKPOINT」は `helvetica-normal_Font` 30、linear 0.521、α 0）。スロットの位置は `pak_reference_2/_assets/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.json` の `CanvasPanelSlot_*`（WebGL 版 10 記録の styles.css の死亡画面にも写してある）。
- 音: `Life_Lost`（0.6）、`66_-_Game_Over`（0.7）、`UI_Select_V3`（1.0）、ポップアップの音（`UMG_PopUp`）。声（Bierce）は項目 20。

### アニメ（ティックは 60,000/s。WebGL 版 10 記録の `death.ts` と同じ値）

| アニメ | 長さ | 中身 |
| --- | --- | --- |
| `Fade In` | 1 s | `Button_0` の α 1 → 0（終わりの接線は User） |
| `Fade Out` | 1 s | `Button_0` の α 0 → 1 |
| `Shake` | 1 s | `Image_161` の α 0 → 1（0.1 s）→ 0.3（0.25 s）→ 0（1 s）、`Image_161` の Scale 1.25 → 1.1（0.1 s）→ 1（1 s）、`CanvasPanel_0` の平行移動 0（0.1 s）→ 10 → −5 → 0（0.3 s）、`Life1`〜`Life6` の色（緑と青が 0.1 s で 0 → 0.3 s で 1）、イベント `Update Life`（0.1 s） |
| `Death` | 3.5 s | REMAINING LIVES とヒントの α 1 → 0（0.5 s）、YOU ARE DEAD の α 0（0.5 s）→ 1（2.5 s）、ボタンの文字・`VerticalBox_161` の不透明度 0 → 1（0.5〜2.5 s）、QUIT TO TITLE は 1 s から |

## シャード（回収済みの記憶）

- **回収**（`Blueprints/Main/BP_Shard.txt` の `Collect(NoSound?)` @80）: `NoSound?` が偽なら、BeginPlay の位置 `Previous Location` を整数に切り捨てた値を `Shards To Be Removed` に `AddUnique`。どちらも消えて、音量 0.65（`NoSound?` なら 0）で鳴る。
- **レベルの読み直し**（ゲームモードの BeginPlay @33233 → 0.2 s → @4551）: `Total Shards` = レベルのシャードの数（消す前）。`Shards To Be Removed` が空でなければ、各シャードの**今の位置**を切り捨てて一致するものを `DestroyActor`（音も数えもしない）、タブレットの数を残りに、残りが 0 なら `All Shards Already Collected` を流す。
- **空にする所**: RESTART・QUIT TO TITLE・`UMG_Loading`（次のレベルへ移るときの読み込み画面の Construct）・ポーズ画面・`Reset Game Instance`。**ディスクには保存しない**（タイトルから続けると、チェックポイントの区間のシャードはすべて戻る。病院の Zone のシャードの区間はチェックポイント 5・9 から始まる）。
- 開発用: プレイヤーコントローラーの J キー（パッケージしていないとき）が一覧を `PrintString` する。

## 本作での扱い（ステップ 3〜7 の前提）

- 写すもの: 死亡 → 死亡画面 → 再開の流れと時間（上）、ライフ 3（上限 6）、チェックポイントの値と再開の場所（上の表の 4〜10）、`SAVING PROGRESS`、シャードの回収の記憶、ゲームオーバーの 3 つのボタン、ヒント、死亡数と時間のセーブ。
- 作らないもの・置き換え: 入口 `06_Hospital` とボス戦（チェックポイント 0〜3・11〜12）。**Zone の開始は 4（Zone 1 の到着）**で、RESTART と「0 のときの入口」は Zone 1 の 4 から。QUIT TO TITLE はタイトル（項目 17）ができるまで Zone 1 の 4 から。Easy の分岐・Malak・熊のレベル・`Hard Check Point`（入口だけの仕組み）は作らない。Bierce の声（死亡・ゲームオーバー・笑い声）は項目 20。
- チェックポイント 6 は原作に書く場面が無いので、本作も書かない（再開の場所の対応は持つ）。
- 連続回収（`Check Streak`・`UMG_ShardStreak`・ライフ +1）とリザルトの `Used Hard Respawn?` は、この項目ではセーブの欄だけ持ち、数え方と画面は後の項目（14 のリザルト）で結ぶ。
