---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 14:20
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
- [ ] 6b. `Tools/game_flow.py` でパッケージ版を通す（Zone 1 の到着 → 迷路 → 駐車場 → 救急車 → Zone 2 → 独房 → 廊下 → 迷路 → 祭壇 → 脱出 → スコア画面。節目で `Wasami.Status` と `HighResShot`）。落ちないこと・各レベルが開くこと・スコア画面が出ることを見る。
- [ ] 7. 結果をまとめて項目 36 を閉じる（画質の選択肢の項目は立てない＝上の「決定事項」。大目標 3 の達成）

## 次にやること

ステップ 6b: **`Tools/game_flow.py`** を書いて、パッケージ版を Zone 1 の到着 → 脱出まで通す。

- 作り: `Tools/game_perf.py` の `launch(map, commands, timeout)`（起動して自分で終わるまで待つ）を使い、`-ExecCmds` に `Wasami.Delay <秒> <コマンド>` を並べて節目を作る。節目ごとに `Wasami.Status` と `Shot showui`、最後は `quit`。結果は `Saved/Archive/Windows/wasami_deception/Saved/Logs/wasami_deception.log` の `LogWasamiDebug` の行（`Wasami.Status level=… checkpoint=… widgets=…`）と `…/Saved/Screenshots/Windows/ScreenShotNNNNN.png` で見る。
- どのチェックポイント・どの `Wasami.*` で場面を作るかは **`Tools/playthrough.py` の `SETUPS`（1298 行〜）と各区間の関数が正本**（`z1_arrive` … `z2_escape`）。歩きは出来ないので `BugItGo X Y Z Pitch Yaw Roll`（`Tools/game_perf.py` の 7 か所の座標も使える）で運び、扉と箱は `Wasami.Trigger <名前>`・`Wasami.Interact`・`Wasami.Flow <イベント>` で進める。
- 見るもの: 落ちないこと（`Saved/Archive/Windows/wasami_deception/Saved/Crashes` が空）、Zone 1 → Zone 2 のレベルの開き直しが通ること、シャードの回収と祭壇と脱出が進むこと、スコア画面（`WasamiLevelClearWidget`）が出ること。
- **キーとマウスそのもの（タイトルの NEW GAME のクリック、歩き、ポーズ）は確かめられない**（下の「要確認」のセキュリティの確認が消えるまで）。

## 決定事項

- 2026-09-21（ステップ 6）: **通しプレイはコマンドラインから行う**。Windows のセキュリティの確認が前面を離さないので画面への入力（キーとマウス）はゲームに届かない（下の「要確認」。2026-09-21 15:00 の時点でも前面のまま）。パッケージ版は `-ExecCmds` しか受け口が無く、それは起動の 1 ティックで走り切るので、**実時間で遅れて走るコマンド `Wasami.Delay` を足して**節目を並べ、`Wasami.Status` と `HighResShot`（ゲームの中から撮るので前面が要らない）で確かめる。キーとマウスそのものの確かめは残るので要確認に書く。

- 2026-09-21: 出力先は `Saved/Archive/Windows`（`Saved/` は git の対象外）。**クックとパッケージ版の実行の間はエディタを閉じる**（VRAM 6 GB。エディタだけで 2.9〜4.1 GB）。
- 2026-09-21: **測定はパッケージ版が前面でなくても成立する**（前面を握られていても窓は普通に描き続ける）。ただし通しプレイ（ステップ 6）は入力が要るのでできない。
- 2026-09-21（ステップ 5b）: **画質の選択肢を足す項目は立てない**。項目 36 の依頼は「1080p で 60 前後に届かなければ画質の選択肢を用意する項目を立てる」だったが、**製品の初期値の HIGH で平均 63.2 fps**（7 か所中 5 か所が 60 以上）で届いており、OPTIONS には既に QUALITY 4 段と RESOLUTION SCALE がある（15 記録）。60 を割るのは最高画質の VERY HIGH だけ。

## 要確認（ユーザー）

- 2026-09-21（ステップ 4）: **原作のナースの姿を描いたテクスチャ 3 枚がパッケージに入る**。`.claude/guides/original-fidelity.md` の「ステージの中にキャラクターの姿が描かれたテクスチャ（ポスター、看板など）があったときは、ワサミの絵に差し替えるかをユーザーに確認する」に当たるので、**替えるかを確かめたい**（中身は絵で、キャラクターのモデルではない）。
  - `hospital_poster_nurse_01_D`（`M_06_Hospital_Poster_01`。紙袋をかぶったナースが「TAKE YOUR MEDICINE!」と言う漫画風の絵）… **Zone 1 で使っている**。
  - `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`。救急車の上で注射器を構えるナースの絵）… **Zone 1・Zone 2 の両方で使っている**。
  - `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`。注射器を持つナースの黒い影絵と「GET VACCINATED!」）… **どのレベルからも使っていない**が、`bCookAll=True` でパッケージには入る。
  - 替えるなら、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）。替えないなら「本家の絵のまま置く」と決めて `original-fidelity.md` の表に 1 行足す。
  - 原作のロゴとキャラクターの**モデル**は入っていない（ステップ 4 で確かめた）。

- 2026-09-21（ステップ 5）: **Windows のファイアウォールの確認（UnrealEditor 宛て）が画面の前面を離さず、`Tools/desktop.py` の入力が届かない**。窓は `PickerHost.exe` の「Windows セキュリティ」（2026-09-21 14 時の時点でも前面のまま。`python Tools/desktop.py start` → `ping` の `foreground` で見える）。`SetForegroundWindow`・`AttachThreadInput`・`SwitchToThisWindow`・ゲームの窓のクリックのどれでも戻らなかった。OS 全体の入力は操作しない決まりなので Claude は押さない。**ユーザーに押してもらいたい**（「許可」でも「キャンセル」でもよい）。消えるまで**ステップ 6 の通しプレイと、画面の入力が要る観察はできない**（症状索引）。

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」。出来上がりは `Saved/Archive/Windows/`（起動は直下の `wasami_deception.exe`）。C++ だけ変えたときの作り直しは **1 分 44 秒**。**`BUILD SUCCESSFUL` は中身を保証しない**ので、作り直したら `grep -c "^/game/" Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 1139 前後かを見る（症状索引）。
- **パッケージ版の動かし方**（画面への入力は届かない）: `Tools/game_perf.py` の `launch()` で `-ExecCmds` を渡す。`Wasami.Delay` で節目を並べ、`Wasami.Status` でログに読み、絵は **`Shot showui`**（`HighResShot` は 3D だけで UI の画面は真っ黒）。コマンドの文字列に**コンマは使えない**（`-ExecCmds` がコンマで切る）。
- **エディタは閉じてある**（ステップ 6a のパッケージの前に閉じた）。PIE が要るときは `python Tools/editor_cycle.py` で開き直す。
- **本編の fps の測り方**は `Tools/game_perf.py`（記録 01 の道具の表）。二重に走らせると互いの CSV を読むので錠（`Intermediate/Perf/.game_perf.lock`）がある。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- **ステップ 6a（2026-09-21）— 足したデバッグコマンド**。PIE（Zone 1）: `Wasami.Status` が 1 行（`level=L_Hospital_Zone1 … lives=3 shards=337/337 … widgets=WasamiInteractWidget`）、`Wasami.Delay 3 Wasami.Status` がちょうど 3.000 s 後に走った。`Wasami.Delay 8 …` は途中の `Wasami.Title` の開き直しをまたいで `level=L_Title widgets=WasamiTitleScreenWidget` を出し、`Wasami.Delay 2 HighResShot 1` も 2 s 後に PNG を書いた。**パッケージ版**（`L_Title`）: 同じ 3 本が同じように走り（`lives=3`、4.0 s 後に 2 本目、9 s の `quit` で正常終了、`Saved/Crashes` 0 件）、`Shot showui` がタイトルの絵（ロゴ・RESUME/NEW GAME/EXTRAS/OPTIONS/QUIT・`UNOFFICIAL FAN GAME` の注意書き・v1.0.0）を撮れた。`HighResShot 1` は 3D だけなので同じ画面が真っ黒だった（症状索引）。
- **ステップ 5・5b（2026-09-21）— パッケージした本編の fps**（1080p ウィンドウ、VERY HIGH と既定の HIGH の 2 画質 × 7 か所、各 10 s・`t.MaxFPS 500`）。**表と考察は実装記録 00 の「パッケージした本編の fps」へ移した**。要点だけ:
  - **HIGH（遊ぶ人が見る絵）は 56.4〜72.4 fps（平均 63.2）で「1080p で 60 前後」に届く**。VERY HIGH は 43.2〜58.2（平均 50.9）で届かない。
  - VERY HIGH（PIE と同じ品質）の本編は PIE より 1〜6 % 遅いだけなので、**PIE の数字は本編の目安に使える**。
  - どの品質も GPU 律速、GPU メモリは 2087〜2950 MB（予算 5198 MB）で余裕がある。
  - 同じ場所を見ていることはプリミティブ数が PIE と一致することで確かめた。
  - 測定中のパッケージ版は前面ではない（ファイアウォールの確認が前面）が、窓は普通に描き続けており絵も正しい。
