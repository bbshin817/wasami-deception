---
title: 仕上げ（通しプレイ・性能・パッケージ）
status: 進行中
branch: main
base: 72d9cb4
started: 2026-09-20 14:43
updated: 2026-09-20 15:08
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 仕上げ（通しプレイ・性能・パッケージ）

作業一覧 `.claude/roadmap.md` の項目 21。**大目標 2「ゲームとして一通り」の最後の項目**（ほかの項目はすべて完了）。

## 依頼

- 目標: タイトルから脱出まで通しで遊べることを確かめ、性能（この PC で 1080p・60 fps 前後、VRAM 6 GB 以内）を整え、パッケージする。
- 完了の条件: 通しプレイの収録（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）で止まる箇所が無い。両ゾーンの性能の計測が `.claude/guides/performance.md` の目安に収まる。パッケージは**配布の話なので先にユーザーに確認する**（`.claude/guides/distribution.md`。無人モードでは飛ばして要確認に書く）。
- 根拠: `.claude/guides/performance.md`・`.claude/guides/distribution.md`、実装記録 00。
- 規模: 3。

## 計画

- [x] 1. 計画（この記録を作る） … 2026-09-20 完了。
- [x] 2. 通しを頭から流して、止まる箇所を洗う … 2026-09-20 完了。12 区間のうち**止まったのは `z2_cell` の頭（Zone 2 の到着）だけ**。下の「ステップ 2 で分かったこと」。
- [ ] 3. **止まる箇所を直す（台本側）** ← 次
  - `Tools/playthrough.py` に区間 `z2_arrive` を足し、`z2_cell` を独房から始まるものに戻す。`SECTIONS`・`SETUPS`・docstring の区間の一覧も直す。
  - `z2_arrive` の中身: 中庭（`PlayerStart_1`）で救急車の到着を待ち、`Trigger_Arrive_CaptureScene` (-11600, -1000, 965) まで**直線の歩き**（中庭に navmesh が無い。`straight=True` で刻む）→ 捕獲と独房の 2 本の場面が終わって入力が戻り、視点が戻るのを待つ（終わりの合図は `PlayerStart_Cell` 付近に居ること・入力が戻っていること）→ `z2_cell` へ渡す。
  - 変更予定: `Tools/playthrough.py` だけ（ゲーム側は本家どおりなので直さない）
- [ ] 4. 通しをもう 1 回、頭から最後まで流して通ることを確かめる（収録と、人に見せる連番のグリッド）
- [ ] 5. 性能の計測（Zone 1）: 1080p 相当で `stat unit`・`stat fps`・`stat RHI`・`stat memory`、代表の 3 か所（リフトの到着・迷路・駐車場）。測り方（PIE の解像度の合わせ方・値の取り方）はこのステップで決めて記録に書く
- [ ] 6. 性能の計測（Zone 2）と判断: 目安（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）に対してどうか。外れていれば対処を次のステップに分ける（対処は**エディタにだけ効く場所**。製品の品質は落とさない）
- [ ] 7. パッケージの下ごしらえの確認だけ行う: 既定のマップとゲームモード（`Config/DefaultEngine.ini`）、`/Game` の外（`Intermediate/Pipeline/`）を指す参照が無いか、原作のロゴとキャラクターのモデルが入っていないか。**パッケージ（クック・ビルド）の実行そのものは配布の話なので無人モードでは行わず、要確認に書く**
- [ ] 8. 締め: 実装記録（00 に性能の計測、01 に台本の直し）・`handover.md` の「現状と次の一歩」・`roadmap.md`（項目 21 を完了、**大目標 2 を「達成（2026-09-20）」、大目標 3 を「進行中」**）・note の記事 `docs/note/progress.md` と note 本体、`check_records.py --update`、進捗記録を消して最後のコミットに含める。状態ファイルは `continue`（`done` に「大目標 2 を達成し、大目標 3 へ移った」）

## ステップ 2 で分かったこと（2026-09-20）

**止まるのは 1 か所だけ**。ゲーム側の不具合ではなく、**台本が古い**（項目 25 で入れた Zone 2 の到着の流れを知らない）。

- `z2_cell` の頭: 救急車の後、Zone 2 は**独房ではなく中庭**（`PlayerStart_1` = -22545, -5035, 1262）で開く。台本は独房（`PlayerStart_Cell` = -14574, 1694, 92）から始まる前提で歩き出し、中庭に navmesh が無いので直線で歩いて (-15139, -4169) で引っかかる（`FAILED: stuck at (-15139, -4169) walking to (-14145, 1330)`）。
  - 本家どおりの流れ（`Source/wasami_deception/WasamiZone2Flow.cpp` の `ArriveEvent`。cp 7）: `PlayerStart_1` へ置く → 0.3 s 後に `06_Hospital_Zone2_AmbulanceArrive1_2` を流し `Trigger_Arrive_CaptureScene`（-11600, -1000, 965、箱 ±32 / ±360 / ±208）を結ぶ。救急車が着くと `Ambulance_Arrive_Blockers4` が消える（それまで道を塞ぐ）。トリガーに入ると入力を切って `06_Hospital_Zone2_Capture`（`CineCameraActor_2`）→ 1 s 後に `06_Hospital_Zone2_Cell` と `PlayerStart_Cell` への移動 → 終わりに入力が戻り、視点が 2 s で戻り、スパイクが降り、独房の錠（`BP_06_Hospital_DoorBreak_2`）が押せるようになる。
  - `SETUPS` の `z2_cell` は cp 7 = 到着から始まるので、区間を分けたら `z2_cell` 単体の setup をどうするか（cp 7 + 到着を通すか、コンソールで独房へ置くか）を決める。
- **`z2_cell` の中身（スパイク・F の解錠・通路の箱まで）は今回走っていない**ので、到着を直して初めて分かる。ステップ 3 で通らなければその場で直す。
- 通った 11 区間と時間: title 17.0 s / z1_arrive 27.9 / pause 18.5 / z1_maze 30.1 / z1_shards 6.2（ここまで頭から 99.7 s）、z1_parking 18.4 / z1_ambulance 43.9、z2_corridor 36.2 / z2_maze 1.0 / z2_altar 24.3 / z2_escape 38.1（Zone 2 の 4 区間で 99.6 s、最後はタイトルに戻りセーブが空）。合計の見込みは到着の場面を入れて 5 分ほど。
- z1_maze の死亡 → 05 の開き直しは 13 s（捕獲 → 死亡画面 → 開き直し）。ライフは 3 → 2 に減り、z1_shards のシャードの回収の間に 3 に戻った（回収でライフが増える仕組み。止まりはしないので今回は追っていない）。

## 次にやること

ステップ 3。`Tools/playthrough.py` に区間 `z2_arrive` を足して Zone 2 の到着（中庭 → `Trigger_Arrive_CaptureScene` → 捕獲と独房の 2 本の場面）を通し、`z2_cell` を独房から始まるものに戻す。直したら `python Tools/pie.py start` → `python Tools/desktop.py start` → `python Tools/playthrough.py run --from z1_ambulance --to z2_corridor --setup` で到着から通路までを確かめる（`z2_cell` の中身は今回まだ走っていない）。終わったら `python Tools/pie.py stop`。

## 決定事項

- 2026-09-20: パッケージ（クックとビルド）は無人モードでは**実行しない**。作業一覧の項目 21 の完了の条件が「配布の話なので先にユーザーに確認する（無人モードでは飛ばして要確認に書く）」と決めているため。ステップ 7 はクック前の確かめだけにする。
- 2026-09-20: 性能の対処が要るときも、変えてよいのは**エディタにだけ効く場所**だけ（`.claude/guides/performance.md` の大原則 2）。製品側を落とさないと収まらないと分かったら、勝手に決めずに要確認へ書く。
- 2026-09-20（ステップ 2）: Zone 2 の到着は**ゲーム側を直さない**。`ArriveEvent` は本家の @24359（Is Packaged For Distribution の真の側）を写したもので、中庭を歩いてトリガーに入るのが本家の遊び方。直すのは台本だけ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動していて `L_Hospital_Zone2` を開いている。PIE は止めた。保存していないマップは無い（2026-09-20 15:08）。
- **通しの台本** `Tools/playthrough.py`（項目 27 で作り、項目 8・10・11・12 で育てた。説明は実装記録 01）: PIE（`Tools/pie.py start`）とデスクトップの代理（`Tools/desktop.py start`）の両方が要る。頭から流すときは `run --from title --to z2_escape --setup`（**`--setup` が要る**。無いと `L_Title` が開いておらず頭の区間が 5 s で落ちる）。終わったら**必ず** `python Tools/pie.py stop`。
  - **台本が走っている間に、ほかの `Tools/playthrough.py status` や `Tools/ue_remote.py` を呼ばない**。エディタのリモート実行は接続を 1 つしか持てず、後から繋ぐと走っている方が `ConnectionAbortedError` で落ちる（2026-09-20 に 1 回やった）。進み具合はログの `tail` で見る。
  - ビューポートの座標 `VIEWPORT = (1822, 206, 2862, 858)` は 2026-09-20 も合っていた。
- 台本は開発用のセーブを `Wasami.ResetSave` で書き換える（本作のセーブ。本家のセーブではない）。
- 収録は `Intermediate/DesktopAgent/`、絵は `Intermediate/DesktopAgent/shots`。ステップ 2 のログは `Intermediate/Overnight/through_21a.log`（頭から）・`through_21b.log`（z1_parking から）・`through_21c.log`（z2_corridor から）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（ステップ 2 は C++ を変えていない）
- エディタでの確認（取り込み・組み立て・PIE）: 2026-09-20 に PIE で 11 区間を通し、`z2_cell` の頭で止まることを確かめた
