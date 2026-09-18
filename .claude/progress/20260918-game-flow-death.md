---
title: ゲームの流れの土台（作業一覧の項目 5）
status: 進行中
branch: main
base: 4f12a50
started: 2026-09-18 15:29
updated: 2026-09-18 18:40
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
- [x] 3. ゲームインスタンス・セーブ・ゲームモードの状態 … 2026-09-18 完了。`UWasamiGameInstance`（ライフ・回収済みのシャード）・`UWasamiSaveGame`（`structSlot`、`Hospital` の欄と `bLastCheckpointWarning`）・ゲームモードの `DeathEvent`/`ResetDeath`/`OnDeath`・時間・`SaveCheckpoint`・`GetSave`/`WriteSave`・0.2 s 後の回収済みシャードの除去（`OnAllShardsAlreadyCollected`）。中身は実装記録 06 の「ライフ・セーブ・死亡の受け口」。テスト 33 本通過、PIE で 2 つ回収 → `open L_Hospital_Zone1` → 337 が 335
- [x] 4. 死亡画面と素材 … 2026-09-18 完了。`UWasamiDeathScreenWidget`（木・Construct・遅延・アニメ 4 本・ゲームオーバーの表示・ライフが残れば `ResetPowers` → `OnRespawn` → `OpenLevel`、画面を出す静的な `Show(WorldContext, Level)`）と `dd_ui.py`（`import_dd_ui`）。中身は実装記録 09。テスト 36 本通過、PIE でライフ残り 2 と 0 を収録
- [x] 5. 死亡から再開までをつなぐ・SAVING PROGRESS・デバッグの呼び出し … 2026-09-18 完了。ゲームモードが本家の Zone のレベル BP の受け持ち（`ChoosePlayerStart` でチェックポイントの PlayerStart、Zone 1 の 0 は 4 を書く、Zone 2 の 0 は Zone 1 を開く、`DeathEvent` で死亡画面、`SaveCheckpoint` で `UWasamiSavingWidget`、`BeginPlay` で `UWasamiBlackFadeWidget` の 0.5 s の明け）と `Wasami.Kill`・`Wasami.Checkpoint N`・`Wasami.ResetSave`・`Wasami.Lives N`。PlayerStart の `PlayerStartTag` に本家の名前（取り込みと両マップ）。中身は実装記録 06 の「開始の場所・死亡画面・SAVING PROGRESS」と 09。テスト 38 本通過、PIE で `Checkpoint 5` → 2 つ回収 → `Kill` → `05_Start` で再開（ライフ 2・シャード 335）、Zone 2 の 7 と 0 も確かめた
- [ ] 6. ゲームオーバー（ライフ 0）の 3 つのボタン … **作業中**（2026-09-18 18:40〜）
  - 資料の「ゲームオーバーの 3 つのボタン」: RESTART（`UMG_PopUp`「ARE YOU SURE YOU WANT TO RESTART?」→ ライフ 3・回収の記憶を空に・セーブの項目を空に → Zone 1 を開く = `04_Start`）、LAST CHECKPOINT（`bLastCheckpointWarning` が偽なら S ランクの警告の `UMG_PopUp`〈Frame 2〉を 1 度 → ライフ 3 → 今のレベルを開き直す。回収の記憶は残す）、QUIT TO TITLE（タイトル〈項目 17〉ができるまでは RESTART と同じ行き先を確かめ無しで）。マウスカーソルと UI の入力モード、ホバーの色、`Fade Out` → 1 s。`UMG_PopUp` の木と素材はこのステップで読む（WebGL 版 10・13 記録にも写しがある）。
  - 変更予定: `WasamiDeathScreenWidget.*`、新規 `WasamiPopUpWidget.*`、`WasamiGameMode.*`（`ResetGameInstance`）、`Tests/WasamiDeathScreenTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_ui.py`（枠 3 枚 `/Game/DD/UI/Menu/Pause/*_window_frame*` と `/Game/DD/Audio/UI/UI_Window_PopUp_V3`）、実装記録 06・09
  - 読んだ結果（番地は資料の表）: ポップアップは `UI/Main/TitleScreen/UMG_PopUp`（木・Popup のアニメ 0.5 s・Construct の音 `UI_Window_PopUp_V3`・YES は OnClicked で `UI_Select_V3` と YesClick、NO は OnPressed で 0.25 s から逆再生・選択音の Pitch 0.7・0.25 s で外す、Close Animation は無音で逆再生と 0.25 s、ホバーは白と `Unhovered Color` 0.114583、文字は `PopupText` の Default = helvetica-normal 28・0.9647）。死亡画面のホバーは白と `Unhover Color` 0.114583。3 つの道は別々の DoOnce
  - 確かめ方: PIE で `Wasami.Lives 1` → `Wasami.Kill` でゲームオーバー（約 3 s でボタン）→ 各ボタンを押して行き先を確かめる（収録）。ボタンはマウスで押す（`python Tools/desktop.py click X Y --allow UnrealEditor.exe`。ゲームオーバーは `SetInputMode_UIOnlyEx` とカーソル）
- [ ] 7. 仕上げ
  - PIE で「チェックポイント → 死亡 → 再開 → 3 回目でゲームオーバー → LAST CHECKPOINT」を 1 本収録し（ステップ 5 の死亡 → `05_Start` の収録は `Intermediate/DesktopAgent/shots/step5-death-respawn.mkv`、グリッドは `Intermediate/Overnight/shots/death_respawn_checkpoint5.png`）、`python Tools/video_probe.py sheet` で Discord の連番のグリッドにする。
  - 実装記録（06・02・04・09）と `_index.md`、`python .claude/scripts/check_records.py --update`。`.claude/roadmap.md` の項目 5 を「完了（日付）」、後回しにした見た目を項目 28 の一覧へ。`.claude/references/handover.md` の「現状と次の一歩」。note の原稿 `docs/note/progress.md`（`.claude/guides/note-progress.md`。セッションの値が無ければ「note へは未反映」）。
  - この記録を削除して最後のコミットに含める。

## 次にやること

ステップ 6。まず資料の「ゲームオーバーの 3 つのボタン」（番地つき）と実装記録 09（死亡画面の `Buttons`・`RestartButton` ほか・`SetInputMode_UIOnlyEx`）・06（`Wasami.ResetSave` の中身・`GetStartCheckpoint`）を読み、`UMG_PopUp` の書き出し（`pak_reference_2/_assets/DDeception/Content/` の下を `find -iname "UMG_PopUp*"`）と WebGL 版 10・13 記録の写しを見てから、ボタンの押下・ホバーの色・ポップアップを作る。

## 決定事項

- 2026-09-18: 死亡画面の声（本家の Bierce の死亡台詞・ゲームオーバーの台詞・笑い声、WebGL 版のワサミの声）は項目 20（台詞と字幕、大目標 2）に回し、この項目では鳴らさない — 項目 20 が台詞と字幕を受け持つ。ステップ 4 で、声の長さ 0 はやめて原作の台詞の長さだけ無音で待つことにした（0 だと Shake が Fade Out の黒に隠れる。09 記録）。
- 2026-09-18: チェックポイントを通る場面（Zone 1 の `05_Transition`・救急車、Zone 2 の各所）と区間の準備（ナース・目的・扉）はこの項目では結ばない — 場面そのものが項目 6・13 の作業。この項目は `SaveCheckpoint`・開始の場所の対応・デバッグの呼び出しまでを作り、項目 6・13 はゲームモードの `GetStartCheckpoint()` を見て区間を準備する（ステップ 5 で、通知 `OnSpawnAtCheckpoint` はやめた。開始の値はプレイヤーが置かれる時、どの `BeginPlay` より先に決まるので、読む側の `BeginPlay` で足りる）。
- 2026-09-18（ステップ 2）: **再開は原作どおり今のレベルを `OpenLevel` で開き直す** — 原作の死亡画面は `Reset Powers` と `Respawn Event` の後に今のレベルを開き直し、敵・扉・シャードを個別に戻す処理を持たない（資料の「全体の流れ」）。本作もそうすれば、項目 6・13 は開始の口だけ持てばよい。残るのはゲームインスタンスとディスクのセーブだけ。
- 2026-09-18（ステップ 2）: **回収済みのシャードはゲームインスタンスに持ち、ディスクには書かない**（原作の `Shards To Be Removed`。RESTART・QUIT・次のレベルへの移動で空にする）。完了の条件の「シャードの回収状態はセーブどおり」は、死亡と LAST CHECKPOINT の開き直しで回収済みが戻らないこと、と読む。
- 2026-09-18（ステップ 2）: 死亡のときにタブレットを下ろす・入力を止める処理は足さない — 原作に無い（ゲームを止めて死亡画面が覆い、開き直しで戻る）。完了の条件の「入力が止まりタブレットが下り」はゲームの一時停止で満たす。
- 2026-09-18（ステップ 2）: 右下の表示は `SAVING PROGRESS`（`UMG_Saving`）。`Progress Saved` は原作の開発用の `PrintString` なので出さない。
- 2026-09-18（ステップ 2）: セーブは `USaveGame` 1 つ（スロット `structSlot`）に、原作の 2 つのスロットのうち要る欄（病院の `levelStruct[5]` の欄と `Last Checkpoint Warning`）だけを持つ — 本作は病院の 1 レベルだけで、UE の仕組みは同じ。ライフは原作どおりプレイヤーのレベル 0 の 3（本作にプレイヤーのレベルは無い）。
- 2026-09-18（ステップ 2）: チェックポイント 0（入口）は Zone 1 の `04_Start` として扱う。原作の開発用の近道（パッケージしないときの `Fake` = Zone 1 は 5、Zone 2 は 8）は写さず、デバッグのコンソールコマンドで置き換える。Easy の分岐・`Hard Check Point`・ボス戦（11〜12）は作らない。

- 2026-09-18（ステップ 4）: 死亡画面の後半（プレイヤーの `ResetPowers`・`Respawn Event`・今のレベルの `OpenLevel`、ゲームオーバーの `SetInputMode_UIOnlyEx` とカーソル）は原作どおりウィジェットが持つ。ボタンの行き先（ステップ 6）も死亡画面に書く。ゲームステートの `Lives Lost` と `Virtual Cursor`（ゲームパッド）は写さない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは動いている想定（ステップ 5 の後に建て直して開き直した）。C++ を変えたら `python Tools/editor_cycle.py`（閉じる前に保存し、PIE は止める）。テストはリモート実行で `Automation RunTests Wasami` を送り、ログの `Test Completed` を読む（ステップ 4 では MCP の `RunTestsByFilter` がソケットごと切れて走らなかった）。**エディタが背面だと 3 fps のまま待つ**ので、`python Tools/desktop.py start` の後、エディタの詳細パネルの空き (3133, 733) を `click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で押して前面にする。開き直すとメッセージログの小窓がビューポートに重なる（閉じるボタンは (2197, 407)）。PIE のビューポートは (1825, 202)〜(2979, 858)。
- PIE で死なせる: `python Tools/pie.py cmd "Wasami.Kill"`（ゲームオーバーは先に `"Wasami.Lives 1"`）。チェックポイントは `"Wasami.Checkpoint N"`、最初からは `"Wasami.ResetSave" "open L_Hospital_Zone1"`。PIE の開始は `04_Start`（セーブの値の PlayerStart）から。死亡の後の開き直しは約 7 s。
- セーブは `Saved/SaveGames/structSlot.sav`（git の外）。ワールドを遊ばせる自動テストも既定のゲームモードを作るので、無ければ空のセーブを書く（06 記録の「既知の制約」）。やり直しの確かめでは消してよい（本作のセーブ。本家のセーブではない）。
- ステップ 6 の覚え: RESTART の行き先は本家では「チェックポイント 0 のまま今のレベル」→ Zone は入口を開く。本作は Zone 1 を開けば `PrepareStart` が 0 を 4 にする（Zone 2 で押しても 0 なら Zone 1 へ移る）。ボタンを押した後の `Fade Out` → 1 s は、死亡画面の予定（`Schedule`）に足せばゲームを止めたまま進む。
- 保留の記録 `20260918-power-look-tuning.md`（項目 23、大目標 3）は触らない。
- 調べ物のサブエージェントは前面で呼ぶ（2026-09-18 の反復 2 がバックグラウンドのまま打ち切られた。`.claude/guides/autonomy.md` の「無人モード」）。

## 検証

- check_records: ステップ 5 で通した（01・02・06・09・`_index.md`）
- C++ ビルド: ステップ 5 で通した（`editor_cycle.py`）。自動テスト 38 本通過（`Wasami.GameFlow.Checkpoints`・`Saving` を含む）
- エディタでの確認: ステップ 5 で PIE（上の計画のステップ 5 の行）。SAVING PROGRESS は右下に文字と明滅する丸
