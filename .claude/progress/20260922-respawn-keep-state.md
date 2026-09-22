---
title: ライフ減少からの復帰で、タブレットとダッシュの状態を引き継ぐ（作業一覧の項目 38）
status: 進行中
branch: main
base: beccd4e
started: 2026-09-22 11:19
updated: 2026-09-22 11:45
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

- [x] 1. 計画（記録を作り、作業一覧の項目 38 を「進行中」にした。b077e56）
- [x] 2. 実装（ゲームインスタンスの持ち越し `RememberPlayerState` / `TakeCarriedPlayerState`、捕獲が下ろす前に書く、プレイヤーの `RestoreState` と押しっぱなしの Shift の見張り。ビルド OK、テスト `Wasami.GameFlow.Lives`・`Wasami.Capture.Room`・`Wasami.Capture.Catch` 成功。実装記録 02・06・07 も更新）
- [ ] 3. PIE で確かめ（トグル入・切の両方）、記録を畳んで閉じる ← 次
  - 変更予定: `.claude/roadmap.md`（項目 38 を完了に）、必要なら `.claude/implementation-records/02-player.md`
  - 手順: エディタは起動済み。**先に `bThrottleCPUWhenNotForeground` を偽に**（下の「再開時の注意」）→ `python Tools/pie.py start` → `Tools/desktop.py down shift w`（`--allow UnrealEditor.exe`。エディタが前面でないときの断りは症状索引の「前面の窓が許可の一覧に無い」）でタブレットを上げ（Space）Shift を押したまま `Wasami.Capture` か敵に捕まる → 再開後に `Wasami.Status` とプレイヤーの `MaxWalkSpeed`（リモート実行で `get_character_movement().max_walk_speed`）とタブレットの上下を見る → トグル設定（`Wasami.Settings ToggleSprint 1`）でも同じことを見る → `pie.py stop` → 絞りを真に戻す

## 次にやること

ステップ 3。上の手順で PIE で確かめ、通ったら作業一覧の項目 38 を完了にし、この記録を消してコミットする。

## 調べてあること

- 死ぬ流れ: 捕獲 `AWasamiCapture::Start` が `DisableInput` → 持ち越しを書く → `PutDownTablet()`。死亡画面の `EStep::Respawn` が `OpenLevel(今のレベル)`。プレイヤーは作り直され、`BeginPlay` が持ち越しを取って `RestoreState` を呼ぶ。
- 確かめたいところ: (1) 再開直後にタブレットが上がったままか（演出なしで）、(2) Shift を押したままなら走れるか、(3) 死亡画面の間に Shift を離していたら歩きのままか、(4) TOGGLE SPRINT 入のときに掛け金が戻るか。

## 決定事項

（実装済みの決めごとは実装記録 02「ダッシュ」「持ち越したダッシュの見張り」・06「ゲームインスタンス」・07「流れ」へ移した）

## 要確認（ユーザー）

- 2026-09-22: **復帰したときのタブレットの出し方** — 仮に「演出（上がるアニメ）と woosh 音なしで、最初から上がった状態」にする。理由: 指摘は「表示状態を引き継ぐ」なので、再開のたびに上げ直す演出が入るのは引き継ぎに見えないため。いまはそう実装してある（`AWasamiPlayerCharacter::RestoreState`: `TabletInterp` = 1 で置き、woosh は鳴らさない）。違うならその 1 か所を直す。

## 再開時の注意

- エディタは起動済み（ビルド済み。2026-09-22 11:41）。C++ を変えたら `python Tools/editor_cycle.py`。
- **エディタが背面だと 3 fps に落ち、Automation テストも PIE の確かめも進まない**。リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`、終わったら `True` に戻す（症状索引の「エディタが背面にあると…」）。
- PIE は `Tools/pie.py start` → 終わったら必ず `stop`。押しっぱなしの入力は `Tools/desktop.py down shift w` →`up shift w`。
- 項目 37 の記録 `20260922-enemy-hand-flip.md` は `status: ユーザー待ち` で残してある（この作業とは別。触らない）。

## 検証

- check_records: OK（20 件。02・06・07 のハッシュを更新）
- C++ ビルド: OK（`editor_cycle.py --no-quit`、161 s。エディタも起動して応答）
- 自動テスト: `Wasami.GameFlow.Lives`・`Wasami.Capture.Room`・`Wasami.Capture.Catch` 成功（2026-09-22）
- PIE での確かめ: 未実行（ステップ 3）
