---
title: ゲームの流れの土台（作業一覧の項目 5）
status: 進行中
branch: main
base: 4f12a50
started: 2026-09-18 15:29
updated: 2026-09-18 16:40
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ゲームの流れの土台（作業一覧の項目 5）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 5（大目標 1「最小の通しプレイ」の最初の項目）:

- 目標: 死ぬ → 死亡画面 → チェックポイントから再開、ライフとゲームオーバー、進行のセーブ（`SAVING PROGRESS`）を本家どおりに作る。敵の接触（項目 9）より前に作るので、死亡はデバッグの呼び出しで起こして確かめる。QUIT TO TITLE は、タイトル画面（項目 17、大目標 2）ができるまでは Zone 1 の最初からやり直す形でよい。
- 完了の条件: ゲームモードの `DeathEvent` で入力が止まりタブレットが下り、`UMG_DeathScreen`（WebGL 版 10 記録の死亡画面の時間・アニメ・音）が出て、ライフが残っていれば最後のチェックポイントで再開し（シャードの回収状態はセーブどおり）、0 なら RESTART / LAST CHECKPOINT / QUIT TO TITLE。チェックポイントの通過で右下に `Progress Saved`。セーブは `SaveGame`（本家の `BP_DD_levelStructSave` の項目）。パワーの死亡のリセット（実装記録 04 の `ResetPowers`）がここから呼ばれる。（この条件の読み替えは「決定事項」の 2026-09-18 ステップ 2 の分）
- 決め方: 大目標 1・2 の決め方（`.claude/roadmap.md`）。規則・流れ・値は本家のコードから写し、見た目は原作のアセットをそのまま使って、無いものは推定で済ませて項目 28 の「後回しの一覧」に書く。本家の実機は見た目のためには起動しない。

**原作の調べ: `.claude/references/game-flow/README.md`**（ステップ 2 でまとめた。値・時間・番地はすべてそこ。各ステップはまずそこを読む）。

## 計画

- [x] 1. 計画 … 2026-09-18 完了
- [x] 2. 原作の死亡・再開・セーブの流れを読んでまとめる … 2026-09-18 完了。`.claude/references/game-flow/README.md` と、読む道具 `Tools/dd/bp_flow.py`（01 記録）。下のステップ 3〜7 を読んだ結果で書き直した
- [ ] 3. ゲームインスタンス・セーブ・ゲームモードの状態（C++。画面はまだ無し）
  - 新規 `UWasamiGameInstance`（本家の `BP_DD_GameInstance`）: `Lives`（始め 3、`Decrement`/`Increment` は 0..6 に Clamp、`ResetLives` = 3）と `ShardsToBeRemoved`（`TArray<FVector>`。回収したシャードの開始位置を整数に切り捨てた値）。`Config/DefaultEngine.ini` の `GameInstanceClass`（00 記録）。
  - 新規 `UWasamiSaveGame`（`USaveGame`、スロット `structSlot`）: 病院の 1 項目だけ（`LevelCheckpoint`・`Deaths`・`Time`・`CurrentStreak`・`Streak`）と、本家の `SaveSlot` の `bLastCheckpointWarning`。ゲームモードの BeginPlay で読むか、無ければ作って書く（`Check For Level Struct Save`）。
  - `AWasamiGameMode`: `DeathEvent(AActor* Cause)`（`DoOnce`、`ResetDeath` で開く。`OnDeath` の多重デリゲート = 本家の `Death Dispatcher`、連続回収の最高を締めて `CurrentStreak` = 0）、時間（Tick で `Time += dt`、`PauseTimeCounter`/`UnpauseTimeCounter`）、`SaveCheckpoint(int32)`（`LevelCheckpoint`・`Time += Time`・`Time` = 0・保存。SAVING PROGRESS の画面はステップ 5 で足す）、BeginPlay の 0.2 s 後に `TotalShards` を数え、`ShardsToBeRemoved` に一致するシャードを消し、タブレットの数を残りに、0 なら `OnAllShardsAlreadyCollected`。
  - `AWasamiShard`: BeginPlay の位置を覚え、`Collect(bNoSound)` が偽なら切り捨てた位置をゲームインスタンスに `AddUnique`。
  - 変更予定: `Source/wasami_deception/WasamiGameMode.*`、新規 `WasamiGameInstance.*`・`WasamiSaveGame.*`、`WasamiShard.*`、`Config/DefaultEngine.ini`、新規 `Tests/WasamiGameFlowTests.cpp`、実装記録 06（ゲームの流れの節）・02・00、`_index.md`
  - 確かめ方: 自動テスト（ライフの Clamp、セーブの往復、切り捨てた位置の一致でシャードが消える、`DeathEvent` の `DoOnce`）→ PIE でシャードを 2 つ取り、コンソールの `open L_Hospital_Zone1` で開き直して、取ったシャードが無く数が合うこと
- [ ] 4. 死亡画面（`UWasamiDeathScreenWidget`）と素材
  - 本家の `UMG_DeathScreen` の木（資料の「ウィジェットの木と素材」）を、タブレットと同じく C++ で木を組む形で写す。アニメ 4 本（資料の表。WebGL 版 10 記録と同じ値）、Construct の時間の流れ（資料の 1〜4。**声は項目 20 に回し、声の長さ 0**＝ライフ > 0 は 0.5 s の後すぐ `Proceed` の DoOnce から 0.5 s → Fade Out）、`Life Animation` と `Update Life`、ヒント（Zone 1・Zone 2 の文言、既出を避ける）、ゲームオーバーの表示（ボタンは出すだけ。押した後はステップ 6）。ゲームを止めた下で動くこと（アニメとタイマーはウィジェットの Tick、音は UI 音）。
  - 素材: `/Game/UI/Main/life_icon_02`・`you_are_dead`・`/Game/UI/Menu/Streaks/T_Vignette`・`helvetica-neue-bold_Font`・`helvetica-normal_Font`（`RobotoTiny` はエンジン）・音 `Life_Lost`・`66_-_Game_Over`・`UI_Select_V3`。取り込みは `dd_tablet.py` の流れを借りるか新規 `dd_ui.py`（着手時に決める）。
  - 変更予定: 新規 `Source/wasami_deception/WasamiDeathScreenWidget.*`、`Content/Python/wasami_tools/pipeline/`（UI の素材）、アセット `/Game/DD/UI/...`、新規 実装記録 09-ui、01、`_index.md`、テスト（アニメの値・時間の流れ）
  - 確かめ方: 自動テスト、PIE で画面だけを出して収録（ライフ残り 2 と 0 の 2 通り）
- [ ] 5. 死亡から再開までをつなぐ・SAVING PROGRESS・デバッグの呼び出し
  - `DeathEvent` → `OnDeath` → 死亡画面を `AddToViewport(5)`・`SetGamePaused(true)`（本家はレベル BP。本作はゾーンの BP が無いのでゲームモードが受け持つ）→ ライフ > 0 なら画面の終わりにプレイヤーの `ResetPowers` → **今のレベルを `OpenLevel` で開き直す**。
  - 開始の場所: ゲームモードがセーブの `LevelCheckpoint` から PlayerStart をタグで選ぶ（`ChoosePlayerStart`。Zone 1: 0・4 → `04_Start`、5 → `05_Start`、6 → `06_Start`。Zone 2: 7 → `PlayerStart_1`、8 → `PlayerStart_MiniBoss`、9 → `PlayerStart_Maze`、10 → `PlayerStart_PostMaze`、0 → Zone 1 を開く）。区間の準備（ナース・目的・扉）は項目 6・13 が結ぶ口 `OnSpawnAtCheckpoint(int32)` を出す。
  - `UWasamiSavingWidget`（資料の「SAVING PROGRESS」。右下の文字と Throbber、`init` 3 s）を `SaveCheckpoint` が出す。
  - デバッグ: コンソールコマンド `Wasami.Kill`（プレイヤーを原因に `DeathEvent`）、`Wasami.Checkpoint N`（`SaveCheckpoint(N)`）、`Wasami.ResetSave`。
  - 変更予定: `WasamiGameMode.*`、`WasamiDeathScreenWidget.*`、新規 `WasamiSavingWidget.*`、`WasamiPlayerCharacter.*`（必要なら）、`Tools/pie.py`（必要なら）、実装記録 06・09・02・04
  - 確かめ方: PIE で `Wasami.Checkpoint 5` → 別の場所へ歩く → `Wasami.Kill` → 死亡画面 → `05_Start` で再開（パワーのリセット、シャードの数、ライフの減り）を収録する
- [ ] 6. ゲームオーバー（ライフ 0）の 3 つのボタン
  - 資料の「ゲームオーバーの 3 つのボタン」: RESTART（`UMG_PopUp`「ARE YOU SURE YOU WANT TO RESTART?」→ ライフ 3・回収の記憶を空に・セーブの項目を空に → Zone 1 を開く = `04_Start`）、LAST CHECKPOINT（`bLastCheckpointWarning` が偽なら S ランクの警告の `UMG_PopUp`〈Frame 2〉を 1 度 → ライフ 3 → 今のレベルを開き直す。回収の記憶は残す）、QUIT TO TITLE（タイトル〈項目 17〉ができるまでは RESTART と同じ行き先を確かめ無しで）。マウスカーソルと UI の入力モード、ホバーの色、`Fade Out` → 1 s。`UMG_PopUp` の木と素材はこのステップで読む（WebGL 版 10・13 記録にも写しがある）。
  - 変更予定: `WasamiDeathScreenWidget.*`、新規 `WasamiPopUpWidget.*`（か同じファイル）、`WasamiGameMode.*`、テスト、実装記録 06・09
  - 確かめ方: PIE で 3 回死んでゲームオーバー → 各ボタンを押して行き先を確かめる（収録）
- [ ] 7. 仕上げ
  - PIE で「チェックポイント → 死亡 → 再開 → 3 回目でゲームオーバー → LAST CHECKPOINT」を 1 本収録し、`python Tools/video_probe.py sheet` で Discord の連番のグリッドにする。
  - 実装記録（06・02・04・09）と `_index.md`、`python .claude/scripts/check_records.py --update`。`.claude/roadmap.md` の項目 5 を「完了（日付）」、後回しにした見た目を項目 28 の一覧へ。`.claude/references/handover.md` の「現状と次の一歩」。note の原稿 `docs/note/progress.md`（`.claude/guides/note-progress.md`。セッションの値が無ければ「note へは未反映」）。
  - この記録を削除して最後のコミットに含める。

## 次にやること

ステップ 3。まず `.claude/references/game-flow/README.md` の「状態の持ち主」「シャード（回収済みの記憶）」と、今の `WasamiGameMode.*`・`WasamiShard.*`・`Tests/WasamiShardTests.cpp`・実装記録 06 を読んでから、`UWasamiGameInstance`・`UWasamiSaveGame` を足す。C++ を書き終えたら `python Tools/editor_cycle.py` で建て直す（閉じる前に保存、PIE は止める）。

## 決定事項

- 2026-09-18: 死亡画面の声（本家の Bierce の死亡台詞・ゲームオーバーの台詞・笑い声、WebGL 版のワサミの声）は項目 20（台詞と字幕、大目標 2）に回し、この項目では鳴らさない — 項目 20 が台詞と字幕を受け持つ。死亡画面は声が終わるか 6 s で進むので、声の長さ 0 として進める。
- 2026-09-18: チェックポイントを通る場面（Zone 1 の `05_Transition`・救急車、Zone 2 の各所）と区間の準備（ナース・目的・扉）はこの項目では結ばない — 場面そのものが項目 6・13 の作業。この項目は `SaveCheckpoint`・開始の場所の対応・`OnSpawnAtCheckpoint`・デバッグの呼び出しまでを作り、項目 6・13 がその口を呼ぶ。
- 2026-09-18（ステップ 2）: **再開は原作どおり今のレベルを `OpenLevel` で開き直す** — 原作の死亡画面は `Reset Powers` と `Respawn Event` の後に今のレベルを開き直し、敵・扉・シャードを個別に戻す処理を持たない（資料の「全体の流れ」）。本作もそうすれば、項目 6・13 は開始の口だけ持てばよい。残るのはゲームインスタンスとディスクのセーブだけ。
- 2026-09-18（ステップ 2）: **回収済みのシャードはゲームインスタンスに持ち、ディスクには書かない**（原作の `Shards To Be Removed`。RESTART・QUIT・次のレベルへの移動で空にする）。完了の条件の「シャードの回収状態はセーブどおり」は、死亡と LAST CHECKPOINT の開き直しで回収済みが戻らないこと、と読む。
- 2026-09-18（ステップ 2）: 死亡のときにタブレットを下ろす・入力を止める処理は足さない — 原作に無い（ゲームを止めて死亡画面が覆い、開き直しで戻る）。完了の条件の「入力が止まりタブレットが下り」はゲームの一時停止で満たす。
- 2026-09-18（ステップ 2）: 右下の表示は `SAVING PROGRESS`（`UMG_Saving`）。`Progress Saved` は原作の開発用の `PrintString` なので出さない。
- 2026-09-18（ステップ 2）: セーブは `USaveGame` 1 つ（スロット `structSlot`）に、原作の 2 つのスロットのうち要る欄（病院の `levelStruct[5]` の欄と `Last Checkpoint Warning`）だけを持つ — 本作は病院の 1 レベルだけで、UE の仕組みは同じ。ライフは原作どおりプレイヤーのレベル 0 の 3（本作にプレイヤーのレベルは無い）。
- 2026-09-18（ステップ 2）: チェックポイント 0（入口）は Zone 1 の `04_Start` として扱う。原作の開発用の近道（パッケージしないときの `Fake` = Zone 1 は 5、Zone 2 は 8）は写さず、デバッグのコンソールコマンドで置き換える。Easy の分岐・`Hard Check Point`・ボス戦（11〜12）は作らない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは動いている想定。ステップ 3 から C++ を変えるので `python Tools/editor_cycle.py` で閉じて建て直す（閉じる前に保存し、PIE は止める）。
- 保留の記録 `20260918-power-look-tuning.md`（項目 23、大目標 3）は触らない。
- 調べ物のサブエージェントは前面で呼ぶ（2026-09-18 の反復 2 がバックグラウンドのまま打ち切られた。`.claude/guides/autonomy.md` の「無人モード」）。

## 検証

- check_records: ステップ 2 で通した（`bp_flow.py` を 01 記録に載せた）
- C++ ビルド: 未実行（ステップ 3 から）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
