---
title: ゲームの途中の場面（Zone 1 の出来事、Zone 2 の捕まる場面と独房）
status: 進行中
branch: feature/cutscenes
base: 4ef89d6
started: 2026-09-20 06:50
updated: 2026-09-20 08:25
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
- [ ] 4. **Zone 2 の到着と捕まる場面**（`AmbulanceArrive1` → `Capture`）
  - 本家の `Arrive Event` @24359: 配布版なら `PlayerStart_1` へ移して 0.3 s 後に `AmbulanceArrive1` を流し（`OnFinished` → `Escape_AmbulanceArrive`）、`Trigger_Arrive_CaptureScene` → `Arrive_CaptureCutscene`（`Disable Player Input` → 視点を `CineCameraActor_2` へ 0.5 s → `Capture` を流す → `OnFinished` → `Cell Cutscene Start`）。本作はパッケージするので配布版の道を写す（エディタの道は到着と捕まる場面を飛ばす）。
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.{h,cpp}`、Zone 2 のレベル
- [ ] 5. **Zone 2 の独房の場面**（`Cell`）
  - 本家の `Cell Cutscene Start` @3149: 1 s 待って `Cell` を流し（スキップ不可）、`OnFinished` → `Cell Cutscene Finished`、プレイヤーを `PlayerStart_Cell` へ移して向きを合わせ、曲を `FadeIn(0.5, 0.3)`。いまの `SkippedScenesEnd`（チェックポイント 7 で場面を飛ばして状態だけ入れる道）を、開き直し（`StartAt` の 7）専用に整理する。
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.{h,cpp}`
- [ ] 6. **通しの確かめと閉じる**
  - PIE で 3 つの場面を収録（Zone 1 の全回収後 → 出来事 → 06 へ、Zone 2 の始まり → 到着 → 捕まる → 独房 → 棘）。チェックポイント 7 で開き直したときに場面が流れないことも見る。実装記録 11（と 01・07）を直し、`check_records.py --update`、roadmap の項目 25 を完了、handover と note の原稿を直して進捗記録を消す。

## 次にやること

ステップ 4（Zone 2 の到着と捕まる場面）。本家 `06_Hospital_Zone_02.txt` の `Arrive Event` @24359 は `IsPackagedForDistribution()` で分岐し、配布版の道（本作が写す方）は `PlayerStart_1` へ移して 0.3 s 後に `06_Hospital_Zone2_AmbulanceArrive1_2` を流し、`OnFinished` → `Escape_AmbulanceArrive`。その後 `Trigger_Arrive_CaptureScene` → `Arrive_CaptureCutscene`（`Disable Player Input` → 視点を `CineCameraActor_2` へ 0.5 s → `06_Hospital_Zone2_Capture` を流す → `OnFinished` → `Cell Cutscene Start`）。Zone 1 と同じく `PlayCutscene(場面, 終わりのイベント, カメラ)` で書ける（捕まる場面だけ先に `DisablePlayerInput`）。いまの `AWasamiZone2Flow` は項目 6 でこの始まりを飛ばしているので、そこを置き換える。変更予定: `Source/wasami_deception/WasamiZone2Flow.{h,cpp}`、`Tests/WasamiZoneFlowTests.cpp`、実装記録 11。

## 決定事項
- 2026-09-20: **スキップの入力はプレイヤーコントローラーの `InputComponent` に足す** — 本家も `DD_PlayerController` が `AnyKey` と `Skip Cutscene`（P・Gamepad_Special_Right）を取る。本作のキー割り当てはプレイヤーキャラクターの Enhanced Input にあるが、場面の間は `DisableInput` でそれが止まるので、そこには置けない。画面が外れるときに自分の結び付けを外す。
- 2026-09-20: **`PlayCutscene` は入力を切らない** — 本家は場面ごとに違う（Zone 1 の出来事は入力を切らずに見せ、Zone 2 の捕まる場面は先に `Disable Player Input`、独房はすでに切れている）。切るのは呼ぶ側（ステップ 3〜5）。
- 2026-09-20: **既存の `dd_sequence` を広げる**（場面ごとの手書きの再生機を作らない） — シーケンスは `AWasamiZoneFlow::PlaySequence`（`GetSequencePlayer()->Play()`）で既に流れている。ステップ 1 で 3 つの場面が組めるようになったので、残りは流す所を作るだけ。
- 2026-09-20: **Zone 2 の始まりは配布版の道を写す** — 本家の `Arrive Event` は `IsPackagedForDistribution()` で分岐し、配布版だけ救急車の到着 `AmbulanceArrive1` を流す。本作はパッケージして遊ぶので配布版の道が本家の姿（ステップ 4）。
- 2026-09-20: **捕まる場面の CameraAnim は流れが流す** — UE 5.8 に `MovieSceneCameraAnimTrack` が無い（`skipped_tracks` に出る）。`CameraAnim_Nurse_01` は `/Game/DD/…` に `UWasamiCameraAnim` として取り込めるので、ステップ 4 で `UWasamiCameraAnimModifier::Play` を場面の 20.53 s（区間 492800〜606400 刻み、4.73 s）に合わせて呼ぶ。
- 2026-09-20: 場面の**台詞と曲は項目 19・20 の口にとどめる** — Capture・Cell の master の音のトラックには Bierce の台詞（`Bierce_TormentTherapy_Event_11`〜`16`）と足音の SoundCue が組み込まれ、ステップ 1 で取り込み済み（そのまま鳴る）。足りないものが出たらコメントで項目 19・20 に回す。

## 要確認（ユーザー）

- 2026-09-20（ステップ 1、仮で進めた）: 場面のナースの演技の読み替え（`dd_sequence.NURSE_ANIMS`）。構え → `Idle_Alert`、跳躍 → `Chase_VaultRoll` → 空中は `Run` → もう一度 `Chase_VaultRoll`、殴る → `Chase_Charge`、待機と台詞の演技（`Event_40`〜`47`）→ `Idle`、後ずさり → `Walk` の逆再生、透明化 → `Walk`。ステップ 6 の PIE で見て直す（見た目の仮の値なので、作業一覧の項目 28 の後回しの一覧にも書いた）。

## 再開時の注意

- **テストの世界に素の `APlayerController` を置かない**（ステップ 4・5 も同じ）: `PlayCutscene` の `SetViewTargetWithBlend` の後にもう一度視点を変えると、local でないコントローラーではエンジンが `ClientSetViewTarget` に無限に潜ってエディタが落ちる（`.claude/references/troubleshooting.md` の「テストの世界で `SetViewTargetWithBlend` を 2 回呼ぶと…」）。視点・スキップの画面・入力の停止はステップ 6 の PIE で見る。落ちる所は `UE_LOG(LogTemp, Display, …)` の印を置いて `Saved/Logs/wasami_deception.log` で読む。
- 作業ブランチ `feature/cutscenes`（`main` から。ステップ 1 をコミット済み）。
- 3 つの場面は組み上がってレベルに置いてある（`/Game/DD/Animation/06_Hospital/06_Hospital_Zone1_06Event`・`_Zone2_AmbulanceArrive1`・`_Zone2_Capture`・`_Zone2_Cell` と、同じ名前の `LevelSequenceActor`。流すのは `AWasamiZoneFlow::PlaySequence(名前)`）。ナース 4 体・シネカメラ 2 台も置いてある（タグ `src:<本家の名前>`）。
- **`dd_sequence` を直したら** `python Tools/ue_remote.py <script>` から `dd_sequence.place("Zone1")` / `("Zone2")` で組み直し、**そのつど `dd_level.build_navigation()` を呼ぶ**（保存でレベルの道が空になる。Zone を切り替えるときは「開くだけの呼び出し」→「焼く呼び出し」の 2 回）。確かめ方は `skipped_tracks` が CameraAnim と `Ballroom_Event_Fade` のイベントだけ・`missing` と `missing_particles` が空。
- Automation テストは、エディタが背面だと 3 fps で進まない。`python Tools/desktop.py start` → `ping` で前面の窓を見て（2026-09-20 はエディタが前面のままだった。座標 2752, 81 のクリックは別の窓に当たる）、背面なら `click <エディタのタイトルバー> --allow UnrealEditor.exe` で前面にしてから走らせ、終わったら `stop`。走らせるのは `python Tools/ue_remote.py -c "…execute_console_command(None, 'Automation RunTests Wasami')"`、結果は `Saved/Logs/wasami_deception.log` の `Test Completed` を数える。
- **スキップの飛ばしは PIE で確かめること**: UE4 の `JumpToSeconds(1e7)` を UE 5.8 の `SetPlaybackPosition(1e7, Jump)` に置き換えた。場面の範囲に丸められて `OnFinished` が流れるはずだが、実際に流れるかはステップ 6 の PIE で見る（流れなければ `PlayTo` か、終わりの直前へ飛ばして再生を続ける形にする）。
- 3 つの場面の尺と作り: `06Event` 10.53 s（ナース 2 体、シネカメラと `06_CameraTarget`、スポットライト 4、粒子と音）、`Capture` 26.23 s（シネカメラ、`camera look`、両開き扉 2、`nurse_idle1`）+ master 11 音・Slomo・Fade、`Cell` 74.07 s（シネカメラ、`nurse_idle2`、牢の扉・壁のスイッチ・偽の天井・棘ほか）+ master 14 音・Slomo。どれも tick 24000・表示 30 fps。
- `pak_reference_2/_sequences/*.csv` は同じ中身を 30 fps で標本化した表で、PIE の収録と数字で見比べるのに使える。

## 検証

- check_records: OK（19 件。記録 09・11 と `_index` を直した）
- C++ ビルド: OK（`Tools/editor_cycle.py`。`WasamiCutsceneWidget` を足した）
- テスト: `Automation RunTests Wasami` 145 件すべて Success（新しい `Wasami.Cutscene.Widget.Tree`・`.Animation` を含む）
- エディタでの確認: ステップ 1 で両ゾーンを組み直し、3 つの場面とナース 4 体・シネカメラ 2 台が置いてあることを確かめてある（ステップ 3 以降はこれを流す）
