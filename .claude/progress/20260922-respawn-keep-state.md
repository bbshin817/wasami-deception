---
title: ライフ減少からの復帰で、タブレットとダッシュの状態を引き継ぐ（作業一覧の項目 38）
status: 進行中
branch: main
base: beccd4e
started: 2026-09-22 11:19
updated: 2026-09-22 11:19
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む -->

# ライフ減少からの復帰で、タブレットとダッシュの状態を引き継ぐ（項目 38）

## 依頼

2026-09-22 のレビュアーの指摘（Medium）: 「ライフ減少からの復帰時、Shift によるダッシュが無効化されている。本家にかかわらず、次の値はライフ減少前の状態を引き継ぐ … タブレットの表示状態 / Shift のダッシュ状態（Option で Shift 未押下でもダッシュ状態を設定している場合）」。

- **本家の作りに関わらずこうする**（ユーザーの指示。本家は死ぬとレベルを開き直してプレイヤーを作り直すだけ）。
- 完了の条件（`.claude/roadmap.md` の項目 38）:
  1. 死ぬ直前の「タブレットが上がっているか」と「ダッシュが入っているか」（`bToggleSprint` のときは掛け金 `bSprintLatch`、そうでないときは Shift を押しているか）を `UWasamiGameInstance` に持ち越し、再開したプレイヤーで戻す。
  2. PIE で、タブレットを上げたまま・Shift を押したまま捕まり、再開の直後にタブレットが上がったままで走れることを確かめる（トグルの設定が入のときも）。
  3. この決めごとを実装記録 02・06 に残す。

## 計画

- [ ] 1. 計画（この記録を作り、作業一覧の項目 38 を「進行中」にする） ← 作業中
  - 変更予定: `.claude/progress/20260922-respawn-keep-state.md`、`.claude/roadmap.md`
- [ ] 2. 現象の再現と実装（PIE で再現して原因を確かめ、C++ を直してビルドする）
  - 変更予定: `Source/wasami_deception/WasamiGameInstance.h`・`.cpp`、`WasamiPlayerCharacter.h`・`.cpp`、`WasamiCapture.cpp`
- [ ] 3. PIE で確かめ（トグル入・切の両方）、実装記録 02・06 を直し、記録を畳んで閉じる
  - 変更予定: `.claude/implementation-records/02-player.md`・`06-game-flow-save.md`、`.claude/roadmap.md`

## 次にやること

ステップ 2。まず PIE で再現する（`Tools/pie.py start` → `Tools/desktop.py` でタブレットを上げ Shift を押したまま敵に捕まる → 再開後に走れるか）。`AWasamiPlayerCharacter::SprintPressed`／`ToggleTablet` に一時のログを足して、**再開後に Enhanced Input の `Started` が届いているか**を確かめてから直し方を決める（下の「調べてあること」の見立て）。

## 調べてあること（ステップ 2 の出発点）

- 死ぬ流れ: 捕獲 `AWasamiCapture` が始まると `DisableInput` と `PutDownTablet()` を呼ぶ（`WasamiCapture.cpp:600` 付近）。つまり**死亡画面が出るころには `bTabletUp` はもう偽**なので、持ち越しの記憶は**捕獲が始まる時点（`PutDownTablet()` の前）**に取る。
- 死亡画面 `UWasamiDeathScreenWidget::NativeConstruct` が `DecrementLives()` を呼び（`:293`）、`OpenLevel(今のレベル)` で開き直す（`:501`・`:689`）。プレイヤーは作り直されるので `bTabletUp`・`bSprintHeld`・`bSprintLatch` はすべて既定の偽に戻る。
- 持ち越しの置き場は `UWasamiGameInstance`（ライフと「回収した欠片」と同じ。ディスクには書かない）。**忘れる場所は `ForgetCollectedShards()` と同じ**（RESTART・QUIT TO TITLE・次のレベルへの読み込み）。
- Shift の見立て: `SprintPressed` は `ETriggerEvent::Started` にしか繋がっておらず（`WasamiPlayerCharacter.cpp:287`）、押したままレベルが開き直ると押下の合図が来ない（または `bCanMove` が偽の間に来て捨てられる。`SprintPressed` の頭で `if (!bCanMove) return;`）。**`ETriggerEvent::Triggered`（押している間は毎フレーム来る）も繋いで押しっぱなしを拾い直す**のが素直。ただしトグル設定（`bToggleSprint`）のときは毎フレーム反転してしまうので、`Triggered` の処理は押しっぱなしの側だけに効かせる。
- 新しいゲーム（チェックポイント 0）だけが `AWasamiZone1Flow::InitialStart()` を通って `bCanMove` を一時的に偽にする。死亡からの再開（チェックポイント 4 以降）はそこを通らないので、`bCanMove` は初めから真。

## 決定事項

- 2026-09-22: **持ち越しは `UWasamiGameInstance`** に置く — ライフと同じく「レベルを開き直しても残り、ディスクには書かない」ものだから（`WasamiGameInstance.h` の説明のとおり）。RESTART・QUIT TO TITLE・次のレベルでは忘れる（`ForgetCollectedShards` と同じ場所）ので、死亡からの再開のときだけ戻る。
- 2026-09-22: **記憶を取るのは捕獲が始まる時点**（`AWasamiCapture` が `PutDownTablet()` を呼ぶ直前） — 死亡画面まで待つと、捕獲が下ろしたあとの「下がっている」を覚えてしまうから。
- 2026-09-22: **ダッシュは、押しっぱなしの側も `Triggered` で拾い直す** — 持ち越しだけだと、死亡画面の間に Shift を離した人が走りっぱなしになる。押している間だけ真になる合図を足すほうが、どちらの場合も正しくなる。

## 要確認（ユーザー）

- 2026-09-22: **復帰したときのタブレットの出し方** — 仮に「演出（上がるアニメ）と woosh 音なしで、最初から上がった状態」にする。理由: 指摘は「表示状態を引き継ぐ」なので、再開のたびに上げ直す演出が入るのは引き継ぎに見えないため。場所: ステップ 2 で `AWasamiPlayerCharacter` に足す復帰の処理（`TabletInterp = 1` で置く）。

## 再開時の注意

- 長時間処理はまだ無い。ステップ 2 で C++ を変えたら `python Tools/editor_cycle.py`（エディタを閉じて Live Coding 無しでビルドし、開き直す）。
- PIE は `Tools/pie.py start` → 終わったら必ず `stop`。押しっぱなしの入力は `Tools/desktop.py hold`。
- 項目 37 の記録 `20260922-enemy-hand-flip.md` は `status: ユーザー待ち` で残してある（この作業とは別。触らない）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
