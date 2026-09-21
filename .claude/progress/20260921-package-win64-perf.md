---
title: Windows のパッケージと本編の性能の計測（作業一覧の項目 36）
status: 進行中
branch: main
base: 5a12f23
started: 2026-09-21 12:31
updated: 2026-09-21 13:15
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
- [ ] 5. 本編の fps を 7 か所で測る（実装記録 00 の表に本編の列を足す）
- [ ] 6. パッケージ版で通しプレイ（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）
- [ ] 7. 結果をまとめて項目 36 を閉じる（60 前後に届かなければ画質の選択肢の項目を立てる。大目標 3 の達成）

## 次にやること

ステップ 5: **本編の fps を 7 か所で測る**（実装記録 00 の性能の表に本編の列を足す）。測る前に下の 2 つを決める（ステップ 4 で分かった）:
- **解像度**: 本編は `Saved/Archive/Windows/wasami_deception/Saved/Config/Windows/GameUserSettings.ini` の `FullscreenMode=1`（ボーダーレス）が効いて、`-windowed -ResX=1920 -ResY=1080` を付けても**デスクトップの 3440x1440（496 万画素 = 1080p の 2.4 倍）**で描いた。PIE の表と比べるには 1080p にそろえる必要がある。`FullscreenMode=2`（ウィンドウ）に書き換えて起動するか、コンソールで `r.setres 1920x1080w`。
- **画質**: 本編の初回起動が自動判定して `GameUserSettings.ini` に書いた既定は **`sg.*=2`（High）**（`ViewDistanceQuality` だけ 3、`ResolutionQuality=100`）。PIE の表は 11 群すべて 3（Epic）で測ったので、**Epic にそろえた列**（PIE と比べる用）と、**既定の High のまま**（遊ぶ人が実際に得る絵）の 2 つを測るのがよい。
- **測り方**: `Tools/perf_probe.py` は PIE 専用（`pie.remote()` を使う）ので本編には使えない。本編では `~` でコンソールを開いて `csvprofile start` / `stop` をキーで打ち、出来た `Saved/Archive/Windows/wasami_deception/Saved/Profiling/CSV/Profile(*).csv` を `perf_probe.py` と同じ列でまとめる（`stat unit` の画面を読むより正確）。**入力は `python Tools/desktop.py ... --allow wasami_deception.exe`**。
- **7 か所への行き方**: 本編にはチェックポイントで開く手立てが無いので、コンソールの `open L_Hospital_Zone1` + セーブのチェックポイントか、通しで遊んで着いた所で測るかを決める（ステップ 6 の通しプレイと合わせると 1 度で済むかもしれない）。

## 決定事項

- 2026-09-21: 出力先は `Saved/Archive/Windows`（`Saved/` は git の対象外）。
- 2026-09-21: **クックとパッケージ版の実行の間はエディタを閉じる**。この PC の VRAM は 6 GB で、エディタだけで 2.9〜4.1 GB の GPU メモリを使う（実装記録 00 の性能の表）。本家を同時に動かさないのと同じ理由（`.claude/guides/verification.md`）。
- 2026-09-21: パッケージ版は**対話デスクトップで起動する**（`python Tools/console_session.py <exe>`）。Claude は Windows のセッション 0 にいるので直に起動すると DXGI で落ちる。操作と撮影は `python Tools/desktop.py`。
- 2026-09-21: 本編の fps は、PIE と同じ測り方（エンジンの CSV プロファイラ）を使えるか試してから決める。`Development` のパッケージはコンソール（`~`）と `stat unit` / `csvprofile start|stop` が使える。CSV は `<パッケージ>/wasami_deception/Saved/Profiling/CSV` に出る。読めなければ `stat unit` の画面を撮って読む（項目 36 の完了の条件は `stat unit` / `stat fps`）。
- 2026-09-21: **`BUILD SUCCESSFUL` は中身を保証しない**ので、パッケージのたびに `ReferencedSet.txt` の `/game/` の数を見る（症状索引・`distribution.md`）。`UnrealPak.exe <…>.utoc -List` は中身の確認に使えない（ファイル名を持つ入り口だけを出し、クックしたパッケージはパッケージ ID で引くので名前が出ない）。

## 要確認（ユーザー）

- 2026-09-21（ステップ 4）: **原作のナースの姿を描いたテクスチャ 3 枚がパッケージに入る**。`.claude/guides/original-fidelity.md` の「ステージの中にキャラクターの姿が描かれたテクスチャ（ポスター、看板など）があったときは、ワサミの絵に差し替えるかをユーザーに確認する」に当たるので、**替えるかを確かめたい**（中身は絵で、キャラクターのモデルではない）。
  - `hospital_poster_nurse_01_D`（`M_06_Hospital_Poster_01`。紙袋をかぶったナースが「TAKE YOUR MEDICINE!」と言う漫画風の絵）… **Zone 1 で使っている**。
  - `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`。救急車の上で注射器を構えるナースの絵）… **Zone 1・Zone 2 の両方で使っている**。
  - `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`。注射器を持つナースの黒い影絵と「GET VACCINATED!」）… **どのレベルからも使っていない**が、`bCookAll=True` でパッケージには入る。
  - 替えるなら、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）。替えないなら「本家の絵のまま置く」と決めて `original-fidelity.md` の表に 1 行足す。
  - 原作のロゴとキャラクターの**モデル**は入っていない（下の「検証」）。

## 再開時の注意

- **パッケージのコマンド**は `.claude/guides/distribution.md`「パッケージ」に実際に通った形で書いた。出来上がりは `Saved/Archive/Windows/`（起動は直下の `wasami_deception.exe`）。作り直しは 2026-09-21 13:01〜13:07 の **5 分 22 秒**（クック 1650 パッケージ、`BUILD SUCCESSFUL`・`ExitCode=0`）。
- **パッケージ版の起動と入力**:
  ```bash
  python Tools/desktop.py start
  python Tools/console_session.py "C:\Users\User\Desktop\wasami_deception\Saved\Archive\Windows\wasami_deception.exe" -windowed -ResX=1920 -ResY=1080
  python Tools/desktop.py shot --scale 0.45                        # 撮る
  python Tools/desktop.py click X Y --allow wasami_deception.exe   # 入力は --allow が要る
  taskkill //F //IM wasami_deception.exe                           # 終わり（入力が通らないときも確実）
  ```
  本編のログは `Saved/Archive/Windows/wasami_deception/Saved/Logs/wasami_deception.log`。
- **⚠ デスクトップへの入力が今は通らない**: Windows のファイアウォールの確認（「UnrealEditor にパブリック／プライベート ネットワークへのアクセスを許可しますか?」）が前面に出たままで、`desktop.py` が `PermissionError: the foreground window is PickerHost.exe` で断る。OS の設定なので触っていない（`.claude/guides/verification.md`「OS 全体の入力は操作しない」）。**ステップ 5・6 の前に消えているかを `python Tools/desktop.py ping` で見る**（要確認）。
- **比べる相手**（実装記録 00 の「性能」の PIE の 7 か所。1080p 相当・Epic、fps avg / p95 / GPU ms）:
  | 場所（チェックポイント） | PIE fps avg | p95 | GPU ms |
  | --- | --- | --- | --- |
  | Z1 リフトの到着（cp 4） | 46.0 | 33.9 | 21.10 |
  | Z1 迷路の始まり（cp 5） | 57.7 | 54.8 | 16.64 |
  | Z1 駐車場（cp 6） | 52.7 | 36.9 | 18.40 |
  | Z2 独房（cp 7 + 場面の後） | 49.7 | 47.8 | 19.51 |
  | Z2 見張りの廊下（cp 8） | 47.3 | 35.0 | 20.50 |
  | Z2 迷路（cp 9） | 60.1 | 55.7 | 16.08 |
  | Z2 祭壇の車庫（cp 10） | 56.0 | 37.9 | 17.20 |
- **エディタ**: ステップ 4 の終わりに開き直した（ステップ 5・6 の前にまた閉じる）。C++ は変えないのでビルドは要らない。
- 走らせたままのバックグラウンドの処理・未保存のアセットは無い。

## 検証

- **ステップ 4（2026-09-21）**:
  - (a) 本編が入った: `Saved/Cooked/Windows/wasami_deception/Metadata/ReferencedSet.txt` は 1650 行で `^/game/` が **1139 件**（`Content/` の 1139 パッケージ全部）。ログは `Packages Cooked: 1650, Packages Incrementally Skipped: 0, Packages Skipped by Platform: 7, Total Packages: 1657`、`Success - 0 error(s), 1 warning(s)`、`BUILD SUCCESSFUL`・`ExitCode=0`。直す前は `/game/` が 1 件だったので、`bCookAll=True` が効いた。
  - 出来上がりも増えた: `Saved/Archive/Windows` は **1.7 GB**（前は 1.0 GB）で、`wasami_deception-Windows.ucas` が **970 MB**（前は 198 MB）。
  - (b) 原作の**ロゴ**は入っていない: `ReferencedSet.txt` の `logo` はエンジンの `zenlogo_64` と本作の `t_titlelogo`・`t_titlelogoglow`・`m_dd_portallogo` だけ（`darkdeception` は曲のファイル名 3 つ = 音なので使ってよい）。原作の**キャラクターのモデル**も入っていない: スケルタルメッシュは本作の `sk_wasamienemy`・`sk_wasamiboss` の 2 体だけで、`hospital_*_anim_skeleton` は小物（ガレージのリフト・のこぎり罠）の動き。`m_06_nurse_items` と `nurse-low5_items_*` はナースの持ち物のテクスチャだが、**絵は金属と針だけで人物の姿は無く**、本作ではレベルの小物「独房の天井の針」（`hospital_zone_02_holdingCell_01_needles_ceiling`）に使っている。`bp_dd_playercharacter_*shake` は揺れ、`m_06_nursesparks` は火花、`tablet_*` はプレイヤーの持つタブレット。
  - **ただし原作のナースの姿を描いた絵が 3 枚入る**（上の「要確認」）。
  - (c) 起動: `console_session.py` で exe を起動してタイトル画面が出た（`Intermediate/DesktopAgent/shots/shot-130826.png`。本作のロゴ・ワサミの顔・NEW GAME / EXTRAS / OPTIONS / QUIT・`UNOFFICIAL FAN GAME - NOT AFFILIATED WITH GLOWSTICK ENTERTAINMENT`・`v1.0.0`）。ログは `Game Engine Initialized.` → `LoadMap(/Game/Stage/Maps/L_Title)` 0.58 s。エラーは 1 件だけで軽いもの（`LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget`。タイトル画面。遊ぶのに支障は無い）。
