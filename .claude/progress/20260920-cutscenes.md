---
title: ゲームの途中の場面（Zone 1 の出来事、Zone 2 の捕まる場面と独房）
status: 進行中
branch: feature/cutscenes
base: 4ef89d6
started: 2026-09-20 06:50
updated: 2026-09-20 08:55
---

# ゲームの途中の場面（作業一覧の項目 25）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 25（大目標 2）。本家のゲームの途中の場面を、ナースの演技を敵ワサミ v3 の動きで代用して作る（2026-09-18 のユーザーの回答「場面の演技」）。

- 対象は 3 つ: Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event`（10.53 s）、Zone 2 の捕まる場面 `06_Hospital_Zone2_Capture`（26.23 s）、独房の場面 `06_Hospital_Zone2_Cell`（74.07 s）。
- 完了の条件: レベル BP どおりのきっかけ（`Arrive_CaptureCutscene`・`Cell Cutscene Start` ほか）で、`_sequences/` のカメラと役者のトラックを写したシーケンスが流れ、終わるとプレイヤーが本家と同じ位置と状態で動ける（項目 6 で飛ばした Zone 2 の始まりが本家どおりになる）。場面のナースの演技は v3 の動きで代用（`.claude/references/enemy-wasami-motions.md` の「場面の代用」）。PIE で 3 つの場面を収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`、`pak_reference_2/_sequences/06_Hospital_Zone1_06Event.json`・`06_Hospital_Zone2_Capture.json`・`06_Hospital_Zone2_Cell.json`（`.csv` は 30 fps に標本化した同じ中身で、確かめに使える）。実装記録 11（ゾーンの進行）・01（取り込みの `dd_sequence`）・07（敵ワサミのアニメ）。
- 依存: 6、7（どちらも完了）。規模 2。

## 計画

- [x] 1. **シーケンスの組み立ての足りないトラック**（`dd_sequence`）— 骨のアニメ・可視・揺れ・スローモーション・部品の材質を足し（CameraAnim は UE 5.8 に無いので落とす）、3 つの場面と救急車の到着を `SEQUENCE_ACTORS` に足して両ゾーンを組み直した。ナース 4 体は `AWasamiCutsceneNurse`（新しい C++）で置き、本家のアニメを `NURSE_ANIMS` で敵ワサミのクリップに読み替える。実装記録 01・08・11（コミット `3221644`）
- [x] 2. **場面を流す土台** — スキップの画面 `UWasamiCutsceneWidget`（本家の `UMG_CutsceneWidget`。帯・PRESS P TO SKIP・暗転で終わりへ飛ばす。入力はプレイヤーコントローラーの `InputComponent`）と、`AWasamiZoneFlow` の `PlayCutscene`・`SequencePlayer`・`SetPlayerViewTarget`・`DisablePlayerInput`・`EnablePlayerInput`。実装記録 09・11・`_index`（コミット `da86730`）
- [x] 3. **Zone 1 の途中の出来事** — `On05ParkingLotCutscene` が `PlayCutscene("06_Hospital_Zone1_06Event", On06Transition, "06_CineCamera")` で場面を流し、`On06Transition` → `Transition06`（視点をプレイヤーへ戻す所も足した）。実装記録 11・09（コミット `fff77a4`）
- [x] 4. **Zone 2 の到着と捕まる場面** — `StartAt` の 7 が `ArriveEvent`（`PlayerStart_1` → 0.3 s → 到着 `06_Hospital_Zone2_AmbulanceArrive1_2`。場面ではないので視点も入力もそのまま）→ `OnEscapeAmbulanceArrive`（塞ぎを消す）、`Trigger_Arrive_CaptureScene` → `OnArriveCaptureCutscene`（入力を切って `PlayCutscene(Capture, OnCellCutsceneStart, CineCameraActor_2)`）→ `OnCellCutsceneStart`（独房の場面はステップ 5 まで飛ばす）。基底の `PlaySequence` に `OnFinished` の結び、`OnCellCutsceneFinished` に入力と視点の戻し、ゲームモードの 7 の PlayerStart を `PlayerStart_1` に。実装記録 11・06（コミット `ef952b5`）
- [x] 5. **Zone 2 の独房の場面** — `OnCellCutsceneStart` が 1 s（`CellSequenceDelay`）待って `PlayCutscene("06_Hospital_Zone2_Cell", OnCellCutsceneFinished, カメラなし, 帯を滑らせない)`（74.07 s）→ `TeleportPlayerTo("PlayerStart_Cell")`。視点は捕まる場面の `CineCameraActor_2` のまま（場面がそれを動かす）、黒も場面のフェードが引き継ぐので、仮の `SkippedCellSceneEnd`・`StopCameraFade` と定数 `FalseCeilingOpen`・`WallSwitchThrown` を消した。実装記録 11（コミット `b084df2`）
- [ ] 6. **通しの確かめと閉じる**
  - PIE で 3 つの場面を収録（Zone 1 の全回収後 → 出来事 → 06 へ、Zone 2 の始まり → 到着 → 捕まる → 独房 → 棘）。**本家はチェックポイント 7 で開き直すたびに 3 つの場面を流し直す**（棘で死んだときも）ので、そのとおりになっていることと、スキップ（P）で飛ばせることを見る。実装記録 11（と 01・07）を直し、`check_records.py --update`、roadmap の項目 25 を完了、handover と note の原稿を直して進捗記録を消す。

## 次にやること

ステップ 6（通しの確かめと閉じる）。PIE（`python Tools/pie.py start` → `cmd` → `stop`）で 3 つの場面を見る。

- Zone 1: 迷路のシャードを全回収して駐車場へ（または `cmd "Wasami.Flow On05ParkingLotCutscene"`）→ 出来事 `06_Hospital_Zone1_06Event`（10.53 s）→ `Transition06`。
- Zone 2: チェックポイント 7 から → 救急車の到着（6.77 s。視点も入力もそのまま庭を歩ける）→ `Trigger_Arrive_CaptureScene` → 捕まる場面（26.23 s）→ 1 s → 独房の場面（74.07 s）→ `Cell Cutscene Finished` → 棘。**7 で開き直すたびに 3 つ流し直す**ことも見る（棘で死んだときも）。
- 見るもの: 視点が 0.5 s でシネカメラへ移る・スキップの画面（`PRESS P TO SKIP`。上下の黒帯は項目 28 の後回しでまだ出ない）・**P で飛ばすと `OnFinished` が流れて次へ進む**（`SetPlaybackPosition(1e7, Jump)` の置き換えの確かめ。下の「再開時の注意」）・独房の場面の頭の黒が 7.43 s で晴れる・終わりに視点が 2 s でプレイヤーへ戻って動ける。ナースの演技の代用も見て、直すなら `dd_sequence.NURSE_ANIMS`（下の「要確認」）。
- 閉じる: 実装記録 11（と 01・07）を直して `python .claude/scripts/check_records.py --update`、`.claude/roadmap.md` の項目 25 を完了、`.claude/references/handover.md` の「現状と次の一歩」と note の原稿 `docs/note/progress.md`（+ `tmp/note-cli`）を直し、進捗記録を消して最後のコミットに含める。`feature/cutscenes` を main へマージ → ローカルブランチを消して push。

## 決定事項
- 2026-09-20: 場面の**台詞と曲は項目 19・20 の口にとどめる**（master の音のトラックに組み込まれていてそのまま鳴る。足りないものが出たらコメントで回す）。
- 2026-09-20（ステップ 4）: **捕まる場面のカメラアニメ `CameraAnim_Nurse_01` は流さない**（Matinee の Move トラックだけで `UWasamiCameraAnim` が持てない）。同じ区間 20.53〜25.27 s の本家のカメラの揺れは流れるので、それで済ませて項目 28 の後回しの一覧に書いた。
- 2026-09-20（ステップ 4）: **チェックポイント 7 は開き直すたびに 3 つの場面を流し直す**（本家の `Respawn` → `Spawn` @22328 → `Arrive Event`。棘で死んで開き直したときも同じで、スキップの画面で飛ばす）。

## 要確認（ユーザー）

- 2026-09-20（ステップ 1、仮で進めた）: 場面のナースの演技の読み替え（`dd_sequence.NURSE_ANIMS`）。構え → `Idle_Alert`、跳躍 → `Chase_VaultRoll` → 空中は `Run` → もう一度 `Chase_VaultRoll`、殴る → `Chase_Charge`、待機と台詞の演技（`Event_40`〜`47`）→ `Idle`、後ずさり → `Walk` の逆再生、透明化 → `Walk`。ステップ 6 の PIE で見て直す（見た目の仮の値なので、作業一覧の項目 28 の後回しの一覧にも書いた）。

## 再開時の注意

- 作業ブランチ `feature/cutscenes`（`main` から）。
- 3 つの場面は組み上がってレベルに置いてある（`/Game/DD/Animation/06_Hospital/06_Hospital_Zone1_06Event`・`_Zone2_AmbulanceArrive1`・`_Zone2_Capture`・`_Zone2_Cell` と、同じ名前の `LevelSequenceActor`〈到着だけ `…_AmbulanceArrive1_2`〉）。ナース 4 体・シネカメラ 2 台も置いてある（タグ `src:<本家の名前>`）。**`dd_sequence` を直したとき**は `python Tools/ue_remote.py <script>` から `dd_sequence.place("Zone1")` / `("Zone2")` で組み直し、そのつど `dd_level.build_navigation()` を呼ぶ（保存でレベルの道が空になる）。
- **スキップの飛ばしは PIE で確かめること**: UE4 の `JumpToSeconds(1e7)` を UE 5.8 の `SetPlaybackPosition(1e7, Jump)` に置き換えた。場面の範囲に丸められて `OnFinished` が流れるはずだが、実際に流れるかは PIE で見る（流れなければ `PlayTo` か、終わりの直前へ飛ばして再生を続ける形にする）。
- 場面の尺と作り: `06Event` 10.53 s、救急車の到着 6.77 s（場面ではない）、`Capture` 26.23 s（シネカメラ、`camera look`、両開き扉 2、`nurse_idle1`）+ master 11 音・Slomo・Fade（22.27 s の 0 → 25.2 s の 1 で黒のまま終わる）、`Cell` 74.07 s（シネカメラ、`nurse_idle2`、牢の扉・壁のスイッチ・偽の天井・棘ほか）+ master 14 音・Slomo・Fade（0〜3.93 s は黒、7.43 s で晴れる）。どれも tick 24000・表示 30 fps。`pak_reference_2/_sequences/*.csv` は同じ中身を 30 fps で標本化した表で、PIE の収録と数字で見比べるのに使える。
- Automation テストは、エディタが背面だと 3 fps で進まない。`python Tools/desktop.py start` → `ping` で前面の窓を見て、背面なら `click 2816 90 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（2026-09-20 のエディタの窓のタイトルバーの空きの座標。端末が前面のときは端末も許可に入れる）で前面にしてから走らせ、終わったら `stop`。走らせるのは `python Tools/ue_remote.py -c "…execute_console_command(None, 'Automation RunTests Wasami')"`、終わりは `Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty`（ログの時刻は UTC = 日本時間 −9 時間）、結果は `Test Completed. Result={Success}` を数える。

## 検証

- check_records: OK（19 件。記録 11 のハッシュを更新した）
- C++ ビルド: OK（`Tools/editor_cycle.py`。警告なし）
- テスト: `Automation RunTests Wasami` 145 件すべて Success（`Wasami.ZoneFlow.Zone2` に独房の場面を足した: 捕まる場面の後に `Cell Cutscene Start` → 1 s は流れず → 流れて 74.07 s → `Cell Cutscene Finished` → 棘）
- エディタでの確認: Zone 2 のレベルに `src:06_Hospital_Zone2_Cell` と `PlayerStart_Cell` があることを確かめた（視点・スキップ・黒帯・独房への移りはステップ 6 の PIE）
