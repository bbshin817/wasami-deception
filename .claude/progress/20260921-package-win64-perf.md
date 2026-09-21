---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 13:50
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
- [x] 5. 2026-09-21 完了。**`Tools/game_perf.py`**（画面への入力を使わず、コマンドラインの `-ExecCmds` だけで測る）を作り、**VERY HIGH で 7 か所**を測った（下の「検証」）。`Wasami.Settings` がパッケージ版で落ちること・ファイアウォールの確認が前面を離さないことを症状索引に書いた。
- [ ] 5b. **既定の HIGH で 7 か所**を測り、実装記録 00 の性能の表に本編の 2 列を足す
- [ ] 5c. `Wasami.Settings` がパッケージ版で落ちるのを直す（`FAudioThread::RunCommandOnAudioThread` で包む → ビルド → パッケージし直し）
- [ ] 6. パッケージ版で通しプレイ（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）※**画面への入力が塞がれている間はできない**（下の「要確認」のファイアウォールの確認）
- [ ] 7. 結果をまとめて項目 36 を閉じる（60 前後に届かなければ画質の選択肢の項目を立てる。大目標 3 の達成）

## 次にやること

ステップ 5b: **既定の HIGH（製品の初期値 Quality 2）で同じ 7 か所を測り**、実装記録 00 の「性能」の表に**本編の 2 列**（VERY HIGH と HIGH）を足す。ステップ 5 と同じコマンドの `--game-quality 3` を `2` にするだけ（7 回で 6 分ほど）。VERY HIGH の値は下の「検証」に控えてある。

```bash
for spec in "pkgh_z1_cp4_arrive:L_Hospital_Zone1:4" "pkgh_z1_cp5_maze:L_Hospital_Zone1:5" "pkgh_z1_cp6_parking:L_Hospital_Zone1:6" "pkgh_z2_cp8_watch:L_Hospital_Zone2:8" "pkgh_z2_cp9_maze:L_Hospital_Zone2:9" "pkgh_z2_cp10_garage:L_Hospital_Zone2:10"; do
  IFS=: read -r label level cp <<< "$spec"
  python -u Tools/game_perf.py measure --label "$label" --level "$level" --checkpoint "$cp" --game-quality 2 --frames 1800 --seconds 10
done
python -u Tools/game_perf.py measure --label pkgh_z2_cp7_cell --level L_Hospital_Zone2 --checkpoint 7 --game-quality 2 --frames 1800 --seconds 10 --pre "Wasami.Flow OnCellCutsceneFinished" --pre "BugItGo -14573.65 1694.18 92 0 -67.86 0"
python Tools/game_perf.py table --prefix pkgh_
```

## 決定事項

- 2026-09-21: 出力先は `Saved/Archive/Windows`（`Saved/` は git の対象外）。**クックとパッケージ版の実行の間はエディタを閉じる**（VRAM 6 GB。エディタだけで 2.9〜4.1 GB）。
- 2026-09-21: **本編の画質は製品自身の OPTIONS の QUALITY で測る**（`UWasamiSettingsSaveGame::Quality`。0 LOW〜3 VERY HIGH、初期値 2 HIGH）。`GameUserSettings.ini` の `sg.*` は起動時に製品の設定で上書きされるので当てにしない。PIE の表（11 群すべて 3）と並べるのは **VERY HIGH** の列で、**HIGH** の列が遊ぶ人の実際の絵。
- 2026-09-21: **解像度は `GameUserSettings.ini` の `FullscreenMode=2`（ウィンドウ）+ `ResolutionSizeX/Y=1920/1080`** で 1080p にそろえた（既定の `1`＝ボーダーレスだとデスクトップの 3440x1440 になる）。控えは同じ場所の `.bak`。
- 2026-09-21: **測定はパッケージ版が前面でなくても成立する**（前面を握られていても窓は普通に描き続ける）。ただし通しプレイ（ステップ 6）は入力が要るのでできない。

## 要確認（ユーザー）

- 2026-09-21（ステップ 4）: **原作のナースの姿を描いたテクスチャ 3 枚がパッケージに入る**。`.claude/guides/original-fidelity.md` の「ステージの中にキャラクターの姿が描かれたテクスチャ（ポスター、看板など）があったときは、ワサミの絵に差し替えるかをユーザーに確認する」に当たるので、**替えるかを確かめたい**（中身は絵で、キャラクターのモデルではない）。
  - `hospital_poster_nurse_01_D`（`M_06_Hospital_Poster_01`。紙袋をかぶったナースが「TAKE YOUR MEDICINE!」と言う漫画風の絵）… **Zone 1 で使っている**。
  - `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`。救急車の上で注射器を構えるナースの絵）… **Zone 1・Zone 2 の両方で使っている**。
  - `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`。注射器を持つナースの黒い影絵と「GET VACCINATED!」）… **どのレベルからも使っていない**が、`bCookAll=True` でパッケージには入る。
  - 替えるなら、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）。替えないなら「本家の絵のまま置く」と決めて `original-fidelity.md` の表に 1 行足す。
  - 原作のロゴとキャラクターの**モデル**は入っていない（下の「検証」）。

- 2026-09-21（ステップ 5）: **Windows のファイアウォールの確認（UnrealEditor 宛て）が画面の前面を離さず、`Tools/desktop.py` の入力が届かない**。`SetForegroundWindow`・`AttachThreadInput`・`SwitchToThisWindow`・ゲームの窓のクリックのどれでも戻らなかった。OS 全体の入力は操作しない決まりなので Claude は押さない。**ユーザーに押してもらいたい**（「許可」でも「キャンセル」でもよい）。消えるまで**ステップ 6 の通しプレイと、画面の入力が要る観察はできない**（症状索引）。

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」。出来上がりは `Saved/Archive/Windows/`（起動は直下の `wasami_deception.exe`）、作り直しは 5 分 22 秒。**`BUILD SUCCESSFUL` は中身を保証しない**ので、作り直したら `grep -c "^/game/" Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` が 1139 前後かを見る（症状索引）。
- **本編の fps の測り方**は `Tools/game_perf.py`（記録 01 の道具の表）。画面への入力は使わない。CSV は `Saved/Archive/Windows/wasami_deception/Saved/Profiling/CSV/`、ログは同じ `Saved/Logs/wasami_deception.log`。二重に走らせると互いの CSV を読んでしまうので錠（`Intermediate/Perf/.game_perf.lock`）を置いてある。
- **エディタは閉じてある**（ステップ 4 の後に開き直したが、ステップ 5 の頭で閉じた）。ステップ 5c で C++ を直すときに `python Tools/editor_cycle.py` で開き直す。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- **ステップ 5（2026-09-21）— パッケージした本編の fps、1080p ウィンドウ・VERY HIGH（製品の最高画質）、各 10 s・`t.MaxFPS 500`**。PIE の列は実装記録 00 の表（1080p 相当・Epic）。

| 場所（チェックポイント） | 本編 fps avg | p95 の fps | Frame ms | Game ms | GPU ms | Draws | Prims | GPU メモリ | （PIE fps） | （PIE GPU ms） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Z1 リフトの到着（cp 4） | 43.2 | 32.9 | 23.15 | 2.51 | 22.54 | 901 | 331k | 2679 MB | 46.0 | 21.10 |
| Z1 迷路の始まり（cp 5） | 54.9 | 37.5 | 18.22 | 3.40 | 17.43 | 1003 | 347k | 2708 MB | 57.7 | 16.64 |
| Z1 駐車場（cp 6） | 51.7 | 40.0 | 19.35 | 3.01 | 18.84 | 402 | 811k | 2769 MB | 52.7 | 18.40 |
| Z2 独房（cp 7 + 場面の後） | 49.1 | 33.4 | 20.37 | 5.03 | 19.76 | 340 | 34k | 2920 MB | 49.7 | 19.51 |
| Z2 見張りの廊下（cp 8） | 46.7 | 31.9 | 21.41 | 5.35 | 20.65 | 463 | 878k | 2950 MB | 47.3 | 20.50 |
| Z2 迷路（cp 9） | 58.2 | 46.0 | 17.17 | 5.35 | 16.67 | 330 | 17k | 2087 MB | 60.1 | 16.08 |
| Z2 祭壇の車庫（cp 10） | 52.6 | 30.4 | 19.02 | 4.85 | 17.79 | 358 | 37k | 2887 MB | 56.0 | 17.20 |

- **同じ場所を見ている**ことの確かめ: プリミティブ数が PIE とほぼ同じ（331k 対 336k、347k 対 350k、811k 対 816k、34k 対 36k、878k 対 884k、17k 対 20k、37k 対 40k）。ドローコールは本編のほうが毎回 100〜150 少ない（エディタだけの描画のぶん）。
- **本編は PIE より 1〜6 % 遅い**（43.2〜58.2 fps 対 46.0〜60.1）。**GameThread は 9.5〜11.3 ms → 2.5〜5.4 ms に減った**（エディタのぶんが消えた）が、**GPU が 0.15〜1.44 ms 増えた**ので差し引きで遅い。どこも GPU 律速のまま（GPU ms ≒ Frame ms）。**1080p・最高画質で 60 fps には届かない**（目安に 3〜28 % 足りない）。
- GPU メモリは 2087〜2950 MB（予算 5198 MB）で PIE より 0.7〜1.4 GB 少なく、`nvidia-smi` のカード全体の山も 2850〜3712 MB / 6144 MB。**メモリは余裕がある。**
- 測定中のパッケージ版は前面ではない（ファイアウォールの確認が前面）が、窓は普通に描き続けており、絵も正しい（`Intermediate/DesktopAgent/shots/shot-133110.png` は Zone 1 の廊下）。
