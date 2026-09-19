---
title: ゲームの途中の場面（Zone 1 の出来事、Zone 2 の捕まる場面と独房）
status: 進行中
branch: feature/cutscenes
base: 4ef89d6
started: 2026-09-20 06:50
updated: 2026-09-20 06:50
---

# ゲームの途中の場面（作業一覧の項目 25）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 25（大目標 2）。本家のゲームの途中の場面を、ナースの演技を敵ワサミ v3 の動きで代用して作る（2026-09-18 のユーザーの回答「場面の演技」）。

- 対象は 3 つ: Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event`（10.53 s）、Zone 2 の捕まる場面 `06_Hospital_Zone2_Capture`（26.23 s）、独房の場面 `06_Hospital_Zone2_Cell`（74.07 s）。
- 完了の条件: レベル BP どおりのきっかけ（`Arrive_CaptureCutscene`・`Cell Cutscene Start` ほか）で、`_sequences/` のカメラと役者のトラックを写したシーケンスが流れ、終わるとプレイヤーが本家と同じ位置と状態で動ける（項目 6 で飛ばした Zone 2 の始まりが本家どおりになる）。場面のナースの演技は v3 の動きで代用（`.claude/references/enemy-wasami-motions.md` の「場面の代用」）。PIE で 3 つの場面を収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`、`pak_reference_2/_sequences/06_Hospital_Zone1_06Event.json`・`06_Hospital_Zone2_Capture.json`・`06_Hospital_Zone2_Cell.json`（`.csv` は 30 fps に標本化した同じ中身で、確かめに使える）。実装記録 11（ゾーンの進行）・01（取り込みの `dd_sequence`）・07（敵ワサミのアニメ）。
- 依存: 6、7（どちらも完了）。規模 2。

## 計画

- [ ] 1. **シーケンスの組み立ての足りないトラック**（`dd_sequence`）← 次
  - いまの `dd_sequence.track()` は Transform・Float・Fade・Particle・Audio だけを組み、ほかは `skipped_tracks` に落ちる。3 つの場面に要るのは SkeletalAnimation（10 + 2 + 14）・Visibility（2 + 1 + 1）・CameraShake（3 + 2 + 9）・CameraAnim（1）・Slomo（Capture・Cell の master）・ComponentMaterial（Cell 1）。これらを足し、3 つの場面を組んで `skipped_tracks` が空になることを確かめる。
  - 場面の役者（`BP_06_ReaperNurse_06Special`・`_Special2`・`nurse_idle1`・`nurse_idle2`）は本家のナースなので、敵ワサミ v3 のメッシュで置き、SkeletalAnimation の本家のアニメを「場面の代用」の対応表で v3 のアニメに読み替える（読み替えの表は取り込みの側に持つ）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_sequence.py`、`dd_level.py`（役者の置き方）、`/Game/DD/Animation/06_Hospital/...`
- [ ] 2. **場面を流す土台**（`AWasamiZoneFlow`）
  - 本家の `Initialize Cutscene Widget(player, bCanSkip, ...)`（`BP_DD_Functions`。スキップの画面 `UMG_Cutscene`）と、場面の間のプレイヤーの入力（`Disable Player Input`）・視点（`SetViewTargetWithBlend(CineCameraActor, 0.5)`）・`OnFinished` の結びを 1 つにまとめた `PlayCutscene` を作る。
  - 変更予定: `Source/wasami_deception/WasamiZoneFlow.{h,cpp}`、新しい `WasamiCutsceneWidget.{h,cpp}`
- [ ] 3. **Zone 1 の途中の出来事**（`06_Hospital_Zone1_06Event`）
  - いまの `On05ParkingLotCutscene` は場面を飛ばして `Transition06` を直に呼んでいる（実装記録 11）。本家 @17019 どおり場面を流し、`OnFinished` → `06_Transition` → `Transition06` にする。
  - 変更予定: `Source/wasami_deception/WasamiZone1Flow.{h,cpp}`、Zone 1 のレベル
- [ ] 4. **Zone 2 の到着と捕まる場面**（`AmbulanceArrive1` → `Capture`）
  - 本家の `Arrive Event` @24359: 配布版なら `PlayerStart_1` へ移して 0.3 s 後に `AmbulanceArrive1` を流し（`OnFinished` → `Escape_AmbulanceArrive`）、`Trigger_Arrive_CaptureScene` → `Arrive_CaptureCutscene`（`Disable Player Input` → 視点を `CineCameraActor_2` へ 0.5 s → `Capture` を流す → `OnFinished` → `Cell Cutscene Start`）。本作はパッケージするので配布版の道を写す（エディタの道は到着と捕まる場面を飛ばす）。
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.{h,cpp}`、Zone 2 のレベル
- [ ] 5. **Zone 2 の独房の場面**（`Cell`）
  - 本家の `Cell Cutscene Start` @3149: 1 s 待って `Cell` を流し（スキップ不可）、`OnFinished` → `Cell Cutscene Finished`、プレイヤーを `PlayerStart_Cell` へ移して向きを合わせ、曲を `FadeIn(0.5, 0.3)`。いまの `SkippedScenesEnd`（チェックポイント 7 で場面を飛ばして状態だけ入れる道）を、開き直し（`StartAt` の 7）専用に整理する。
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.{h,cpp}`
- [ ] 6. **通しの確かめと閉じる**
  - PIE で 3 つの場面を収録（Zone 1 の全回収後 → 出来事 → 06 へ、Zone 2 の始まり → 到着 → 捕まる → 独房 → 棘）。チェックポイント 7 で開き直したときに場面が流れないことも見る。実装記録 11（と 01・07）を直し、`check_records.py --update`、roadmap の項目 25 を完了、handover と note の原稿を直して進捗記録を消す。

## 次にやること

ステップ 1。作業ブランチ `feature/cutscenes` を切ってから、`Content/Python/wasami_tools/pipeline/dd_sequence.py` の `track()` に SkeletalAnimation・Visibility・CameraShake・CameraAnim・Slomo・ComponentMaterial を足す。3 つの場面の JSON で必要な `Params`・`channels` の形を先に読む（`python -c` で `tracks[].sections[]` を見る）。

## 決定事項

- 2026-09-20: **既存の `dd_sequence` を広げる**（場面ごとの手書きの再生機を作らない） — 本家のシーケンスは既に LevelSequence アセットとして組み立てて置く仕組みがあり（実装記録 01 の `dd_sequence`、`AWasamiZoneFlow::PlaySequence` が `GetSequencePlayer()->Play()`）、Zone 2 の棘 `06_Hospital_Zone2_Spikes` などは既にこの道で流れている。足りないのはトラックの種類だけ。
- 2026-09-20: **Zone 2 の始まりは配布版の道を写す** — 本家の `Arrive Event` は `IsPackagedForDistribution()` で分岐し、配布版だけ救急車の到着 `AmbulanceArrive1` を流す（エディタでは捕まる場面から）。本作はパッケージして遊ぶので配布版の道が本家の姿。
- 2026-09-20: **場面のナースは敵ワサミ v3 で代用** — `.claude/references/enemy-wasami-motions.md` の「場面の代用」の表（構え → `Idle_5`、跳んで去る → `Parkour_Vault_with_Roll`、殴る → `Male_Head_Down_Charge`、待機 → `Idle_11`、後ずさり → `Walking` の逆再生、透明化 → `Walking` で去る）。PIE で見て合わなければ表ごと直す。
- 2026-09-20: 場面の**台詞と曲は項目 19・20 の口にとどめる** — Capture・Cell の master には Bierce の台詞（`Bierce_TormentTherapy_Event_11`〜`16`）と足音があり、シーケンスの Audio トラックとして組めば鳴るので、音の素材が取り込めていればそのまま流し、足りないものはコメントで項目 19・20 に回す。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- まだ何も変更していない。作業ブランチ `feature/cutscenes` は未作成（ステップ 1 の始めに切る）。
- 3 つの場面の尺と作り: `06Event` 10.53 s・19 バインディング（ナース 2 体、シネカメラと `06_CameraTarget`、スポットライト 4、粒子と音）、`Capture` 26.23 s・6 バインディング（シネカメラ、`camera look`、両開き扉 2、`nurse_idle1`）+ master 11 音・Slomo・Fade、`Cell` 74.07 s・12 バインディング（シネカメラ、`nurse_idle2`、牢の扉・壁のスイッチ・偽の天井・棘ほか）+ master 14 音・Slomo。どれも tick 24000・表示 30 fps。
- `pak_reference_2/_sequences/*.csv` は同じ中身を 30 fps で標本化した表で、PIE の収録と数字で見比べるのに使える。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
