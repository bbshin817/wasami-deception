---
title: ゲームの流れの土台（作業一覧の項目 5）
status: 進行中
branch: main
base: 4f12a50
started: 2026-09-18 15:29
updated: 2026-09-18 15:29
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ゲームの流れの土台（作業一覧の項目 5）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 5（大目標 1「最小の通しプレイ」の最初の項目）:

- 目標: 死ぬ → 死亡画面 → チェックポイントから再開、ライフとゲームオーバー、進行のセーブ（`SAVING PROGRESS`）を本家どおりに作る。敵の接触（項目 9）より前に作るので、死亡はデバッグの呼び出しで起こして確かめる。QUIT TO TITLE は、タイトル画面（項目 17、大目標 2）ができるまでは Zone 1 の最初からやり直す形でよい。
- 完了の条件: ゲームモードの `DeathEvent` で入力が止まりタブレットが下り、`UMG_DeathScreen`（WebGL 版 10 記録の死亡画面の時間・アニメ・音）が出て、ライフが残っていれば最後のチェックポイントで再開し（シャードの回収状態はセーブどおり）、0 なら RESTART / LAST CHECKPOINT / QUIT TO TITLE。チェックポイントの通過で右下に `Progress Saved`。セーブは `SaveGame`（本家の `BP_DD_levelStructSave` の項目）。パワーの死亡のリセット（実装記録 04 の `ResetPowers`）がここから呼ばれる。
- 決め方: 大目標 1・2 の決め方（`.claude/roadmap.md`）。規則・流れ・値は本家のコードから写し、見た目は原作のアセットをそのまま使って、無いものは推定で済ませて項目 28 の「後回しの一覧」に書く。本家の実機は見た目のためには起動しない。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 完了。ステップ 2〜7 に分けた。順序の理由は「決定事項」
- [ ] 2. 原作の死亡・再開・セーブの流れを読んでまとめる（コードを読むだけ。C++ は変えない） ← 次
  - 読むもの（すべて `pak_reference_2/_bytecode/DDeception/Content/`）:
    - `Blueprints/Main/BP_DD_GameMode.txt`: `DeathEvent(Cause Actor)`（入口 @34486 → @33223 → …）、`Reset Death`（@35117）、`Lives`・`Total Lives`、`Struct Save`（`DD_LevelStructureyyy` の配列をレベルの enum 0〜11 で引く）と `Check For Level Struct Save`・書き出し、`Death Dispatcher` を誰が結ぶか。
    - `Blueprints/Main/DD_PlayerController.txt` の `Shards To Be Removed`、`Blueprints/Main/BP_Shard.txt` の `NoSound` の分岐、`UI/Main/UMG_Loading.txt`（シャードの消し方）。
    - `Blueprints/Main/BP_DD_PlayerCharacter.txt` の死亡（入力・タブレット・`Reset All Powers` の呼び所）。
    - `06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`: `DeathEvent`（@14791。`UMG_DeathScreen` を作り `AddToViewport(5)`・`SetGamePaused`）、`Respawn`（@14796 → @8167: 画面を外す・再開・`EnableInput`・`Reset Death`・`Spawn`）、`Spawn`（@13483。チェックポイント番号ごとの PlayerStart〈Zone 2 は `PlayerStart_Cell`・`_Maze`・`_MiniBoss`・`_PostMaze`・`_1`〉と、そこで戻すもの〈敵・扉・曲〉）、`UMG_Saving` と `Progress Saved`（Zone 1 は @6119 と @10742 の 2 か所、Zone 2 は 4 か所。どのイベントから来るか）。
    - `Blueprints/UMG/UMG_DeathScreen.txt`（最新版。WebGL 版 10 記録の `death.ts` は旧版 v1.6.1 から写したので、時間・アニメ・音・ヒントの違いを見る）、`UI/Main/UMG_Saving.txt`、ゲームオーバーの 3 つのボタンの行き先（`RestartEvent`・`Last Checkpoint`・QUIT TO TITLE、`Get Current Progress`）。
    - アセットの一覧（`pak_reference_2/_assets/`）: `BP_DD_levelStructSave.json`・`DD_LevelStructureyyy.json`（項目は `LevelCheckpoint`・`Deaths`・`Time`・`Streak`・`CurrentStreak`・`BonusShards`・`Secrets`・`AchievementData`）、`UMG_DeathScreen.json`・`UMG_Saving.json` のウィジェットの木と参照する素材（テクスチャ・フォント・音）。
  - 出力: `.claude/references/game-flow/README.md`（パワーの調査 `.claude/references/powers/` と同じ形。根拠の版と @ 番地つき）。CLAUDE.md の「参考資料」に 1 行。読んだ結果に合わせて、この記録のステップ 3〜7 の中身（変更予定・値）を書き直す。
- [ ] 3. セーブとゲームモードの状態（C++）
  - `USaveGame` の派生（本家の `BP_DD_levelStructSave` / `DD_LevelStructureyyy` の項目。本作は 06_Hospital の Zone 1・2 だけ）、ゲームモードのライフ（`Total Lives` 3）・`DeathEvent(Cause)`・`ResetDeath`・死亡の通知（本家の `Death Dispatcher`）、チェックポイントの保存（番号・シャードの回収状態・死亡数・時間）と読み込み、シャードの `Shards To Be Removed`（再開・読み込みで回収済みのシャードを音なしで消す）。
  - 変更予定: `Source/wasami_deception/WasamiGameMode.*`、新規 `WasamiSaveGame.*`、`WasamiShard.*`、新規 `Tests/WasamiGameFlowTests.cpp`、実装記録 06・02
  - 確かめ方: 自動テスト（ライフの増減、セーブの往復、回収済みのシャードが消える）→ PIE でシャードを数個取って保存 → PIE を止めて始め直し、取ったシャードが無いこと
- [ ] 4. 死亡画面（`UWasamiDeathScreenWidget`）と素材
  - 本家の `UMG_DeathScreen` のウィジェットの木・時間・アニメ（Fade In・Life Animation・Shake・Death・Fade Out）・音（`Life_Lost`・`66_-_Game_Over`・選択音・ポップアップ）を、タブレットと同じく C++ で木を組む形で写す。素材（ライフのアイコン・YOU ARE DEAD の絵・赤いビネット・フォント・音）を原作データから取り込む。声（Bierce / WebGL 版のワサミ）は項目 20 に回し、声の長さ 0 として進める。ゲームを止めた下で動くこと（本家は `SetGamePaused`）。
  - 変更予定: 新規 `Source/wasami_deception/WasamiDeathScreenWidget.*`、`Content/Python/wasami_tools/pipeline/`（UI の素材の取り込み。新規か `dd_tablet.py` の流用はステップ 2 の後に決める）、アセット `/Game/DD/UI/...`、実装記録 09-ui（新規）・01
  - 確かめ方: ウィジェットのアニメの値のテスト、PIE で画面を出して収録（ライフ残り・ライフ 0 の 2 通り）
- [ ] 5. 死亡から再開までをつなぐ
  - ゲームモードの `DeathEvent` → プレイヤーの入力を止めタブレットを下ろす → 死亡画面 → 暗転の 2 s 後に `ResetPowers`・ゲームの再開・チェックポイントの PlayerStart へ移す（本家のレベル BP の `Respawn` → `Spawn`）。ゾーンごとのチェックポイント番号 → PlayerStart の対応（レベルの `PlayerStart` のタグ）。`UMG_Saving`（右下の `Progress Saved`）。チェックポイントを通る場面（Zone 1 の全回収・リフトなど）は項目 6・13 で結ぶので、ここは保存の口とデバッグの呼び出しまで。
  - デバッグの呼び出し: コンソールコマンド（例 `Wasami.Kill`・`Wasami.Checkpoint N`）。
  - 変更予定: `WasamiGameMode.*`、`WasamiPlayerCharacter.*`、新規 `WasamiSavingWidget.*`（または死亡画面と同じファイル）、`Tools/pie.py`（必要なら）、実装記録 02・04・06・09
  - 確かめ方: PIE でチェックポイントを保存 → 別の場所へ移る → `Wasami.Kill` → 死亡画面 → チェックポイントで再開（パワーのリセット、シャードの数、ライフの減り）を収録する
- [ ] 6. ゲームオーバー（ライフ 0）の 3 つのボタン
  - RESTART（確かめのポップアップ → セーブを消して Zone 1 の最初から）、LAST CHECKPOINT（ライフを満たして最後のチェックポイントから）、QUIT TO TITLE（タイトル〈項目 17〉ができるまでは Zone 1 の最初から）。マウスカーソルと UI の入力モード、ホバーの色。行き先の細部はステップ 2 の結果で決める。
  - 変更予定: `WasamiDeathScreenWidget.*`、`WasamiGameMode.*`、テスト、実装記録 06・09
  - 確かめ方: PIE で 3 回死んでゲームオーバー → 各ボタンを押して行き先を確かめる（収録）
- [ ] 7. 仕上げ
  - PIE で「チェックポイント → 死亡 → 再開 → 3 回目でゲームオーバー → LAST CHECKPOINT」を 1 本収録し、`python Tools/video_probe.py sheet` で Discord の連番のグリッドにする。
  - 実装記録（06・02・04・09）と `_index.md`、`python .claude/scripts/check_records.py --update`。`.claude/roadmap.md` の項目 5 を「完了（日付）」、後回しにした見た目を項目 28 の一覧へ。`.claude/references/handover.md` の「現状と次の一歩」。note の原稿 `docs/note/progress.md`（`.claude/guides/note-progress.md`。セッションの値が無ければ「note へは未反映」）。
  - この記録を削除して最後のコミットに含める。

## 次にやること

ステップ 2。`BP_DD_GameMode.txt` の `DeathEvent`（@34486 → @33223）から読み始め、上の一覧の順に読んで `.claude/references/game-flow/README.md` にまとめる。バイトコードは長い（ゲームモード 10,882 行・死亡画面 7,887 行）ので、入口の番地から `Jump` / `PushExecutionFlow` をたどり、呼び出し・定数・`Delay` だけを拾う（`grep "^@\|Function\|CallMath\|Const"`）。

## 決定事項

- 2026-09-18: 最初のステップを「読むだけ」にした — 死亡・再開・セーブはゲームモード・プレイヤーコントローラー・プレイヤー・2 つのレベル BP・死亡画面にまたがり、どの C++ のクラスに何を持たせるか（ステップ 3〜6 の分け方）が読んだ結果で変わる。パワーの作業（`.claude/references/powers/`）と同じく、根拠を先に 1 か所にまとめる。
- 2026-09-18: 死亡画面の声（本家の Bierce の死亡台詞・ゲームオーバーの台詞、WebGL 版のワサミの声）は項目 20（台詞と字幕、大目標 2）に回し、この項目では鳴らさない — 項目 20 が台詞と字幕を受け持つ。死亡画面は声の長さで進む（WebGL 版 `proceedAt` = reveal + min(声, 6) + 0.5）ので、声の長さ 0 として進める。
- 2026-09-18: チェックポイントを通る場面（Zone 1 の全回収・ガレージリフト・Zone 2 の各所）はこの項目では結ばない — 場面そのものが項目 6・13 の作業。この項目は保存の口・再開の位置の対応・デバッグの呼び出しまでを作り、項目 6・13 がその口を呼ぶ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは動いている想定。ステップ 2 はコードを読むだけなのでエディタに触らない。ステップ 3 から C++ を変えるので `python Tools/editor_cycle.py` で閉じて建て直す（閉じる前に保存し、PIE は止める）。
- 保留の記録 `20260918-power-look-tuning.md`（項目 23、大目標 3）は触らない。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
