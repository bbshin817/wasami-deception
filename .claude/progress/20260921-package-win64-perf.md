---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 16:05
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む -->

# Windows のパッケージと本編の性能の計測（作業一覧の項目 36）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 36（大目標 3 の最後の項目）。2026-09-21 の有人セッションで、項目 21 の性能の要確認に「クックして本編の fps を測る」と回答をもらった分。

- `Development` の Win64 でパッケージ（クック → ビルド → ステージ → pak → アーカイブ）し、出来た exe がタイトルから脱出まで通しで遊べる。
- 両ゾーンの代表の場所（項目 21 で PIE を測った 7 か所）の fps を測り、実装記録 00 の性能の表に**本編の列**を足す。
- **1080p で 60 前後に届かなければ**、製品に画質の選択肢（解像度スケールか品質プリセット）を用意する項目を立てる。
- パッケージに原作のロゴとキャラクターのモデルが入っていないことを確かめる。
- **手順は `.claude/guides/distribution.md` の「まだ整備していない」の節に書き足す**。
- クックとビルドはユーザーが 2026-09-21 に承認済み（無人運転で行ってよい）。**配布（誰かに渡す・公開する）は別途ユーザーに確認する**ので行わない。

## 計画

- [x] 1. 計画（この記録を作ってステップに分ける） … 2026-09-21 完了。
- [x] 2. `BuildCookRun` を通した … 2026-09-21 完了。`GameFeatureData` の規則が無くてクックがエラー 2 件で落ちたので `Config/DefaultGame.ini` に足し、`BUILD SUCCESSFUL`。手順を `distribution.md` に、失敗を症状索引に書いた。
- [x] 3. パッケージの中身の確認 … 2026-09-21 完了。**`/Game` のアセットが `L_Title` の 1 つしか入っていなかった**（全 501 パッケージ・494 クック。残りはエンジンとプラグインの既定）。`Config/DefaultGame.ini` に `[/Script/UnrealEd.ProjectPackagingSettings]` の `bCookAll=True` を足して直し、症状索引・`distribution.md`・実装記録 00 を直した。**作り直しはステップ 4。**
- [x] 4. パッケージの作り直しと中身の確認 … 2026-09-21 完了。`bCookAll=True` で **1650 パッケージがクックされ、`/game/` は 1139 件**（`Content/` の全部）。原作のロゴのテクスチャもキャラクターのモデルも入っていない。exe が起動してタイトルが出た。**原作のナースの姿の絵 3 枚**が入っているのを見つけた（要確認）。
- [x] 5. 2026-09-21 完了。**`Tools/game_perf.py`**（画面への入力を使わず、コマンドラインの `-ExecCmds` だけで測る）を作り、**VERY HIGH で 7 か所**を測った。`Wasami.Settings` がパッケージ版で落ちること・ファイアウォールの確認が前面を離さないことを症状索引に書いた。
- [x] 5b. 2026-09-21 完了。**既定の HIGH で同じ 7 か所**を測り、実装記録 00 に「パッケージした本編の fps」の節（2 画質 × 7 か所）を足した。**HIGH は 56.4〜72.4 fps（平均 63.2）で目安に届き**、VERY HIGH は 43.2〜58.2（平均 50.9）。
- [x] 5c. 2026-09-21 完了。音量の読みを `FAudioThread::RunCommandOnAudioThread` + `FAudioCommandFence` に移して**パッケージ版の `Wasami.Settings` が落ちなくなった**（`L_Title` と `L_Hospital_Zone1` の両方で 3 クラスの音量を印字して正常終了、`Saved/Crashes/` は 0 件）。症状索引と 15 記録を直し、パッケージを作り直した（1 分 52 秒、`/game/` 1139 件）。
- [x] 6a. 2026-09-21 完了。デバッグの **`Wasami.Delay S Command …`**（実時間 S 秒後にコンソールコマンドを走らせる。レベルをまたぐ）と **`Wasami.Status`**（レベル・チェックポイント・ライフ・シャード・目的・プレイヤー・画面のウィジェットを 1 行でログに）を足し、PIE とパッケージ版の両方で確かめた（06 記録）。パッケージも作り直した（1 分 44 秒、`/game/` 1139 件）。
- [x] 6b. 2026-09-21 完了。**`Tools/game_flow.py`** を作り、パッケージ版をタイトル → Zone 1 の到着 → 脱出 → スコア画面まで**1 回の起動（228 s）で通した**（18 の節目がすべて期待どおり、絵 19 枚、`Saved/…/Crashes` 0 件）。途中で `Wasami.ResetSave` がタイトルで効かない（病院のゲームモードが無い）のを見つけて直し、パッケージを作り直した（1 分 35 秒）。01・06 記録。
- [ ] 7. 結果をまとめて項目 36 を閉じる（画質の選択肢の項目は立てない＝上の「決定事項」。大目標 3 の達成）

## 次にやること

ステップ 7: 項目 36 を閉じる。

- 実装記録 00 の「パッケージした本編」の節に**通しプレイが通ったこと**（`Tools/game_flow.py`、228 s、18 の節目、落ちない）を 2〜3 行足す。
- `.claude/guides/distribution.md` の「パッケージ」に**出来たパッケージの確かめ方の最後の 1 つ**として `python Tools/game_flow.py run` を足す（中身の確認 → 原作の素材の確認 → 通しプレイ、の順）。
- `.claude/references/handover.md` の「現状と次の一歩」を直し、**ユーザーが遊んで確かめる手順**（`Saved/Archive/Windows/wasami_deception.exe` を起動して NEW GAME）を書く。
- `.claude/roadmap.md` の項目 36 を完了にする。**これで大目標 3 の項目がすべて完了**なので、同じコミットで大目標 3 を「達成（2026-09-21）」にし、進捗記録を消して `stop` を書く（`.claude/skills/continue/SKILL.md` の「5. ステップが終わったら」）。
- 画質の選択肢の項目は立てない（下の「決定事項」）。ナースの絵 3 枚は**ユーザー待ちのまま残す**（下の「要確認」）。

## 決定事項

- 2026-09-21（ステップ 5b）: **画質の選択肢を足す項目は立てない**。項目 36 の依頼は「1080p で 60 前後に届かなければ画質の選択肢を用意する項目を立てる」だったが、**製品の初期値の HIGH で平均 63.2 fps**（7 か所中 5 か所が 60 以上）で届いており、OPTIONS には既に QUALITY 4 段と RESOLUTION SCALE がある（15 記録）。60 を割るのは最高画質の VERY HIGH だけ。
- 2026-09-21（ステップ 6）: **通しプレイはコマンドラインから行う**（画面への入力が塞がれているため。下の「要確認」）。**マウスとキーそのもの**（タイトルの NEW GAME、欠片の画面の CLOSE、スコア画面の NEXT）は確かめられないので回り道した。この 3 つは PIE では `Tools/playthrough.py` が実際に押して通している（11 記録）ので、残るのは「パッケージ版でも押せるか」だけ。
- 2026-09-21（ステップ 6b）: **チェックポイント 6 はどこからも保存されない**が、これは本家どおりで不具合ではない。本家の `06_Hospital_Zone_01` のレベル BP が `LevelCheckpoint` を書くのは 2 か所だけで、本作の `SaveCheckpoint(5)`（迷路の箱）と `SaveCheckpoint(7)`（救急車の屋根）に当たる。6 は開始位置の値としてだけ使う（`ChoosePlayerStart` → `06_Start`、`Tools/game_perf.py` の駐車場の計測）。

## 要確認（ユーザー）

- 2026-09-21（ステップ 4）: **原作のナースの姿を描いたテクスチャ 3 枚がパッケージに入る**。`.claude/guides/original-fidelity.md` の「ステージの中にキャラクターの姿が描かれたテクスチャ（ポスター、看板など）があったときは、ワサミの絵に差し替えるかをユーザーに確認する」に当たるので、**替えるかを確かめたい**（中身は絵で、キャラクターのモデルではない）。
  - `hospital_poster_nurse_01_D`（`M_06_Hospital_Poster_01`。紙袋をかぶったナースが「TAKE YOUR MEDICINE!」と言う漫画風の絵）… **Zone 1 で使っている**。
  - `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`。救急車の上で注射器を構えるナースの絵）… **Zone 1・Zone 2 の両方で使っている**。
  - `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`。注射器を持つナースの黒い影絵と「GET VACCINATED!」）… **どのレベルからも使っていない**が、`bCookAll=True` でパッケージには入る。
  - 替えるなら、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）。替えないなら「本家の絵のまま置く」と決めて `original-fidelity.md` の表に 1 行足す。
  - 原作のロゴとキャラクターの**モデル**は入っていない（ステップ 4 で確かめた）。

- 2026-09-21（ステップ 5）: **Windows のファイアウォールの確認（UnrealEditor 宛て）が画面の前面を離さず、`Tools/desktop.py` の入力が届かない**。窓は `PickerHost.exe` の「Windows セキュリティ」（2026-09-21 14 時の時点でも前面のまま。`python Tools/desktop.py start` → `ping` の `foreground` で見える）。`SetForegroundWindow`・`AttachThreadInput`・`SwitchToThisWindow`・ゲームの窓のクリックのどれでも戻らなかった。OS 全体の入力は操作しない決まりなので Claude は押さない。**ユーザーに押してもらいたい**（「許可」でも「キャンセル」でもよい）。消えるまで**ステップ 6 の通しプレイと、画面の入力が要る観察はできない**（症状索引）。

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」。出来上がりは `Saved/Archive/Windows/`（起動は直下の `wasami_deception.exe`）。C++ だけ変えたときの作り直しは **1 分 35 秒〜1 分 44 秒**。**`BUILD SUCCESSFUL` は中身を保証しない**ので、作り直したら `grep -c "^/game/" Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 1139 前後かを見る（症状索引）。
- **パッケージ版の通しプレイ**は `python Tools/game_flow.py run`（228 s。01 記録の道具の表）。二重起動を防ぐ錠は `Tools/game_perf.py` と共有（`Intermediate/Perf/.game_perf.lock`）。
- **エディタは閉じてある**（ステップ 6a の前に閉じた）。パッケージを作り直したのでエディタ側のビルドは古い。PIE が要るときは `python Tools/editor_cycle.py` で建て直して開く。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- **ステップ 6b（2026-09-21）— パッケージ版の通しプレイ**。`python Tools/game_flow.py run`: 1 回の起動で **228 s**、`-ExecCmds` の 59 コマンドがすべて時刻どおりに走り、**18 の節目が全部期待どおり**（終了コード 0）。タイトル（`WasamiTitleScreenWidget`）→ `Wasami.ResetSave` + `open` → Zone 1 がチェックポイント 4・ライフ 3・シャード 337 で開いてステージ OP（`WasamiChapterPortalWidget`）→ エレベーターの扉が開く → 鍵 → 迷路の箱で保存 5 → 全回収で REACH THE PARKING LOT（ライフ 4。連続回収の褒賞）→ 駐車場の場面 → GET ON TOP OF THE AMBULANCE → 屋根で保存 7・GOOD LUCK → **Zone 2 が開く**（7、シャード 342）→ 捕まる場面（`input=0`）→ 独房の場面（101 s）→ 入力が戻る → 鍵 → 独房を出て廊下の箱で保存 8 → 迷路の箱で保存 9 → 全回収で保存 10・COLLECT THE RING PIECE（ライフ 5）→ 欠片で HEAD TOWARDS THE GARAGE → ガレージで GET TO THE PORTAL → ポータルで**スコア画面**（`WasamiLevelClearWidget`、停止、チェックポイント 0、死亡 0）→ `quit` で正常終了。`Saved/Archive/Windows/wasami_deception/Saved/Crashes` は 0 件。レベルの読み込みは Zone 1 が 0.60 s・Zone 2 が 1.12 s。
  - 絵 19 枚 `Intermediate/GameFlow/01-title.png`〜`19-z2_results.png`（git の外）。見て確かめたのは、エレベーターの扉が開いて赤い両開き扉が見えること、独房の天井から棘が下りていること、ガレージ手前で Bierce の字幕 `You got it! Now get out of this twisted hospital!` が出ること、スコア画面が `Stinky Gachimi` / TIME 3:39 S / SOUL SHARDS 679 S / BONUS SHARDS 0/2 C / SECRETS 0/4 C / LIVES LOST 0 S / SHARD STREAK 500 S / TOTAL SHARDS 819 / FINAL RANK A になること。
  - **1 回目は失敗した**（`Wasami.ResetSave` がタイトルで効かず、前のチェックポイント 7 のまま Zone 1 が開いた）。直してパッケージを作り直した（`BuildCookRun` 1 分 35 秒、`/game/` 1139 件）。
- **ステップ 5・5b（2026-09-21）— パッケージした本編の fps**。表と考察は実装記録 00 の「パッケージした本編の fps」へ移した。要点: **既定の HIGH は 56.4〜72.4 fps（平均 63.2）で目安に届き**、VERY HIGH は 43.2〜58.2（平均 50.9）。どの品質も GPU 律速で、GPU メモリは 2087〜2950 MB（予算 5198 MB）。
