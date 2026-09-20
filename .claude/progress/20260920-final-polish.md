---
title: 仕上げ（通しプレイ・性能・パッケージ）
status: 進行中
branch: main
base: 72d9cb4
started: 2026-09-20 14:43
updated: 2026-09-20 17:55
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
- [x] 2. 通しを頭から流して、止まる箇所を洗う … 2026-09-20 完了。12 区間のうち**止まったのは `z2_cell` の頭（Zone 2 の到着）だけ**。下の「ステップ 2・3 で分かったこと」。
- [x] 3. 止まる箇所を直した（台本に区間 `z2_arrive` を足した） … 2026-09-20 完了。下の「ステップ 2・3 で分かったこと」。
- [x] 4. 通しをもう 1 回、頭から最後まで流した … 2026-09-20 完了。**13 区間すべてが通り（終了コード 0）、タイトルから脱出の後のタイトルまで 408 s（6 分 48 秒）**。下の「ステップ 4 の通し」。
- [x] 5. 性能の計測（Zone 1） … 2026-09-20 完了。測り方を決めて道具 `Tools/perf_probe.py` を作り、代表の 3 か所で測った。下の「ステップ 5 の計測（Zone 1）」。
- [x] 6. 性能の計測（Zone 2）と、目安に対する判断 … 2026-09-20 完了。代表の 4 か所（独房・見張りの廊下・迷路・祭壇）を測り、**メモリは目安内、fps は 1080p 相当で 47〜60 と目安の 60 にわずかに届かない**。どこも GPU 律速で、エディタにだけ効く対処では製品の fps は動かないので**対処のステップは足さず、要確認に書いた**。下の「ステップ 6 の計測（Zone 2）と判断」。
- [x] 7. パッケージの下ごしらえの確認 … 2026-09-20 完了。3 つとも問題なし（既定のマップ・ゲームモードは正しい、`/Game` の外を指す参照は `/ACLPlugin` だけ、原作のキャラクターのモデルとロゴは入っていない）。直したのは 2 つ: エディタの道具のプラグインを Editor ターゲット限定にし、パッケージ設定を空のままにする理由を ini に書いた。下の「ステップ 7 の確認」。クックとビルドは行っていない（配布の話）。
- [ ] 8. 締め: 実装記録（00 に性能の計測、01 に台本の直し）・`handover.md` の「現状と次の一歩」・`roadmap.md`（項目 21 を完了、**大目標 2 を「達成（2026-09-20）」、大目標 3 を「進行中」**）・note の記事 `docs/note/progress.md` と note 本体、`check_records.py --update`、進捗記録を消して最後のコミットに含める。状態ファイルは `continue`（`done` に「大目標 2 を達成し、大目標 3 へ移った」）

## ステップ 4 の通し（2026-09-20）

- `run --from title --to z2_escape --setup --shots --record through_21.mkv` が終了コード 0。13 区間すべてが通り、タイトルから脱出の後のタイトルまで **408.4 s**（止まった箇所は無い。台本が自分で直したのは 2 か所: 救急車の車庫で経路の引き直し 1 回、見張りの廊下でビューポートの押し直し 1 回）。収録は `Intermediate/DesktopAgent/shots/through_21.mkv`、区間の絵は同じ場所の `pt_*.png`（27 枚）、ログは `Intermediate/Overnight/through_21f.log`。
- スコア画面（`pt_z2_escape_results.png`）: TIME 5:25、SOUL SHARDS 679、BONUS 0/2、SECRETS 0/4、LIVES LOST 1、SHARD STREAK 500、TOTAL 819、**FINAL RANK A**。

## ステップ 5 の計測（Zone 1、2026-09-20）

**測り方**（道具 `Tools/perf_probe.py`。説明は実装記録 01）: 画面の数字は読まず、エンジンの CSV プロファイラ（`csvprofile start`/`stop`）が 1 フレーム 1 行で書く列を使う。1080p 相当は `r.ScreenPercentage 175`（PIE のビューポート 1039x654 → 1818x1144 = 208 万画素。1080p は 207 万）、`t.MaxFPS 500` でエンジンの 62 fps の平滑を外し、`slomo 0.05` で敵を実質止めて 10 s 測る。スケーラビリティは 11 群すべて 3（Epic）で、製品と同じ品質。

| 場所（チェックポイント） | fps avg | p95 フレームの fps | Frame ms | Game ms | Render ms | GPU ms | DrawCalls | Prims |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| リフトの到着（cp 4） | 46.0 | 33.9 | 21.72 | 9.56 | 21.70 | 21.10 | 1019 | 336k |
| 迷路の始まり（cp 5） | 57.7 | 54.8 | 17.33 | 9.50 | 17.31 | 16.64 | 1107 | 350k |
| 駐車場（cp 6） | 52.7 | 36.9 | 18.96 | 9.63 | 18.93 | 18.40 | 503 | 816k |
| 参考: cp 4 を 720p 相当（116 %） | 79.4 | 76.2 | 12.60 | 8.96 | 12.57 | 12.05 | 1018 | 336k |

- **どこも GPU 律速**（GPU ≒ FrameTime、GameThread は 9.5 ms で余裕がある）。720p にすると 79 fps まで上がるので、フレーム時間のほとんどは画素にかかっている（1080p にすると +9 ms）。
- メモリはどの場所も同じ: `GPUMem/LocalUsedMB` 4111 MB（予算 4893 MB）、`RenderTargetPoolUsed` 1545 MB、カード全体で 5348 MB / 6144 MB。エディタの常駐 RAM 約 3.0 GB（目安 20 GB に対して余裕）、システムの空き 14 GB。
- 結果の JSON は `Intermediate/Perf/z1_*.json`（git の外）、元の CSV は `Saved/Profiling/CSV/`。
- 捨てた測り方: 等倍（slomo なし）で cp 6 を測ると、10 s 立っている間にナースに捕まってレベルが開き直り、700 ms のハッチと描画数の乱れ（DrawCalls 204〜3657）が入った。cp 4・cp 5 は等倍でも `slomo 0.05` でも fps の差が 1 % 未満だったので、3 か所とも `slomo 0.05` に揃えた。

## ステップ 6 の計測（Zone 2）と判断（2026-09-20）

測り方はステップ 5 と同じ（`r.ScreenPercentage 175` で 1080p 相当、`t.MaxFPS 500`、`slomo 0.05` で 10 s、スケーラビリティは 11 群すべて Epic）。独房だけはチェックポイント 7 で開いたあと `PlayerStart_Cell` へ置いて `Wasami.Flow OnCellCutsceneFinished` を送り、台本の区間 `z2_cell` と同じ状態にしてから測った（使い捨ての道具 `Intermediate/Overnight/setup_cell.py`）。

| 場所（チェックポイント） | fps avg | p95 フレームの fps | Frame ms | Game ms | GPU ms | DrawCalls | Prims | GPU メモリ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 独房（cp 7 + 場面の後） | 49.7 | 47.8 | 20.11 | 11.00 | 19.51 | 476 | 36k | 3631 MB |
| 見張りの廊下（cp 8） | 47.3 | 35.0 | 21.14 | 11.33 | 20.50 | 584 | 884k | 3790 MB |
| 迷路（cp 9） | 60.1 | 55.7 | 16.64 | 11.02 | 16.08 | 447 | 20k | 2871 MB |
| 祭壇の車庫（cp 10） | 56.0 | 37.9 | 17.84 | 10.28 | 17.20 | 474 | 40k | 3592 MB |
| 参考: cp 8 を 720p 相当（116 %） | 71.9 | 69.6 | 13.91 | 11.74 | 13.27 | 584 | 884k | 3626 MB |

- Zone 1 と同じく**どこも GPU 律速**（GPU ≒ FrameTime、GameThread は 10〜11 ms で余裕がある）。いちばん重い見張りの廊下（884k プリミティブ。中庭と救急車の車庫が見通せる）でも、720p 相当に落とすと 72 fps まで上がる。
- メモリ: GPU メモリ 2871〜3790 MB（予算 5198 MB）、カード全体で 3980〜4806 MB / 6144 MB、エディタの常駐 RAM 3.3 GB、システムの空き 13.8 GB。**Zone 1（4111 MB / カード 5348 MB）より軽い**。
- 結果の JSON は `Intermediate/Perf/z2_*.json`（git の外）。

**目安（`.claude/guides/performance.md`「1080p で 60 fps 前後・VRAM 5 GB 程度まで・エディタの常駐 RAM 20 GB 程度まで」）に対する判断**

- **メモリは両ゾーンとも目安内**（VRAM は最大 4.8 GB、RAM は 3.3 GB）。ここは対処不要。
- **fps は両ゾーンで 46〜60、平均 52 ほど**で、目安の 60 に 10〜20 % 届かない。ただし **PIE の数字にはエディタ自身の描画が載る**（エディタの UI と、最適化されていないエディタ用の描画経路）ので、パッケージした本編が同じ数字になるとは限らない。確かめるにはクックが要り、クックは配布の話なので無人モードでは行わない。
- 対処できる場所が無い: 逼迫を避ける設定は**エディタにだけ効く場所**に置く決まりで（同ガイドの大原則 2）、エディタ側を軽くしても製品の fps は動かない。製品の fps を上げるには `Config/DefaultEngine.ini` の描画設定か製品の既定のスケーラビリティを下げることになり、これは「最終的に遊べるゲームの品質を損なってはならない」に反する。**よって対処のステップは足さず、要確認（下）に選択肢を書いた。**

## ステップ 7 の確認（2026-09-20）

**(1) 既定のマップとゲームモード・パッケージに入るマップ** … 問題なし。`Config/DefaultEngine.ini` は `GameDefaultMap=/Game/Stage/Maps/L_Title`、`EditorStartupMap=L_Hospital_Zone1`（エディタだけ）、`GlobalDefaultGameMode=/Script/wasami_deception.WasamiGameMode`、`GameInstanceClass=WasamiGameInstance`。`/Game` のマップは 3 つだけ（`L_Title`・`L_Hospital_Zone1`・`L_Hospital_Zone2`。Zone 1・2 には焼いた照明の `_BuiltData.uasset` が並ぶ）。`Config/DefaultGame.ini` に `ProjectPackagingSettings` の節が無い＝マップの一覧が空なので、クックは **`/Game` を全部入れる**経路に落ちる（下の決定事項）。

**(2) `/Game` の外を指す参照** … 問題なし。3 つのマップから辿れる 821 パッケージのうち `/Game`・`/Engine`・`/Script` の外は `/ACLPlugin` の 2 つだけ（ワサミのアニメの圧縮設定。エンジンに既定で入る Runtime のプラグインなのでパッケージに入る）。`Intermediate/Pipeline/` など `/Game` の外のファイルを指す参照は無い（取り込み元のパスは `AssetImportData` に残るがエディタ専用で、クックには関わらない）。
  - ついでに見つけて直したもの: `.uproject` で有効にしているエディタの道具のプラグイン 3 つ（`ModelContextProtocol`・`AllToolsets`・`LiveCodingToolset`）に `TargetAllowList` が無く、**製品のビルドにも入る設定**だった（`ModelContextProtocol` は Runtime のモジュールを 2 つ持つので、そのままだと本編に MCP が同梱される）。先にあった `ModelingToolsEditorMode` と同じく `"TargetAllowList": ["Editor"]` を足した。ゲームのモジュール（`wasami_deception.Build.cs`）はどれにも依存していない。

**(3) 原作のロゴとキャラクターのモデル** … 入っていない。`/Game` のスケルタルメッシュは 7 つで、本作の `SK_WasamiEnemy`・`SK_WasamiBoss` と、原作の**小道具**（ガレージのリフト、のこぎりの罠 4 つ）だけ。タイトルの絵は本作の `T_TitleLogo`・`T_TitleLogoGlow`・`T_TitleFace`。`M_DD_PortalLogo` は名前だけで、使っているのは本作の `MI_Portal_Wasami`（`T_Portal_Wasami`）。
  - 使っていない原作の章の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`（「TORMENT THERAPY」の文字の絵）が残っている。画面には出ない（本作は `T_LevelTitle`）が、クックは全部入れるのでパッケージには入る。削除は確認が要るので消さず、要確認へ。

**クックに入る量**: `/Game` の 1134 パッケージのうち 1106 が、3 つのマップか C++ の名指しから辿れる。辿れないのは 28 個だけ（`/Game/Pipeline/Debug` の検証用の材質 17、原作の UI 5〈上の題字とタブレットの地図の `tablet_map_shard` など 4。この 4 つは**本家でもどこからも参照されていない**ので、本作に欠けている機能ではない。地図の印は材質 `M_Shard` などで描く〉、原作の ThirdParty のテクスチャ 3、取り込みの設定 `PL_*` 3）。

## 次にやること

ステップ 8（締め）。**この記録を消して最後のコミットに含める**。やること: (a) 実装記録 00 に性能の計測（ステップ 5・6 の表と判断）とパッケージの下ごしらえ（ステップ 7）を、01 に通しの台本の直し（区間 `z2_arrive` と `SETUPS` のタグ）を書く、(b) `handover.md` の「現状と次の一歩」を直し、ユーザーが遊んで確かめる手順を書く、(c) `.claude/roadmap.md` の項目 21 を完了にし、**大目標 2 を「達成（2026-09-20）」・大目標 3 を「進行中（2026-09-20 から）」**にする（大目標 3 の始め方は「自動」）、(d) note の記事 `docs/note/progress.md` と note 本体を「いま何が出来るか」に直す（`.claude/guides/note-progress.md`）、(e) `python .claude/scripts/check_records.py --update`、(f) 状態ファイルは `continue`（`done` に「大目標 2 を達成し、大目標 3 へ移った」）。

## 決定事項

- 2026-09-20（ステップ 7）: パッケージ設定（`ProjectPackagingSettings`）は**空のままにする**。UE 5.8 のクックは、マップの一覧（`MapsToCook`・`[AllMaps]`）も `DirectoriesToAlwaysCook` も無いときだけ「`/Game` を全部入れる」経路に落ちる（`CookOnTheFlyServer.cpp` の `bCookAllByDefault = true` と、`CollectFilesToCook` の終わりの `if (bCookAll || (bCookAllByDefault && NumFilesAddedByCommandLineOrGameCallback == 0))`）。本作は **C++ が `/Game` のパスを直に名指しして読むアセットが 237 個**あり、そのうち 195 個はどのマップからも参照されていない（アセットレジストリから辿れない）ので、Project Settings の「Maps to Cook」「Directories to Always Cook」を埋めるとその数え上げが 0 でなくなり、195 個が黙って落ちる。理由は `Config/DefaultGame.ini` にコメントで書いた（絞るなら `bCookAll=True` にする）。
- 2026-09-20（ステップ 7）: エディタの道具のプラグインは `TargetAllowList: ["Editor"]` を付ける。製品のビルドに MCP の Runtime のモジュールが入らないようにするため。付けた後に開き直して、3 つとも有効のままエディタが開くことを確かめた。

- 2026-09-20（ステップ 5）: 性能の数字は**画面を読まず、エンジンの CSV プロファイラの列から取る**。`stat unit` などは画面に描くだけで、値をファイルに出す口が無いため。CSV には stat unit・stat RHI・stat memory・stat streaming に当たる列がそろっている。
- 2026-09-20（ステップ 5）: 1080p は `r.ScreenPercentage` で**画素数を合わせて**近似する。`r.SetRes` は PIE に効かず、PIE の窓（エディタのビューポート）の大きさは変えられないため。出力は 1039x654 のままなので、画素あたりの負荷は 1080p と同じだがポストプロセスと UI のぶんだけ軽い。
- 2026-09-20（ステップ 5）: 敵のいる場所は `slomo 0.05` で測る。等倍だと立っている間に捕まってレベルが開き直り、測定に混ざるため（fps の差は 1 % 未満）。

- 2026-09-20: パッケージ（クックとビルド）は無人モードでは**実行しない**。作業一覧の項目 21 の完了の条件が「配布の話なので先にユーザーに確認する（無人モードでは飛ばして要確認に書く）」と決めているため。ステップ 7 はクック前の確かめだけにする。
- 2026-09-20: 性能の対処が要るときも、変えてよいのは**エディタにだけ効く場所**だけ（`.claude/guides/performance.md` の大原則 2）。製品側を落とさないと収まらないと分かったら、勝手に決めずに要確認へ書く。
- 2026-09-20（ステップ 2・3）: Zone 2 の到着は**ゲーム側を直さない**。`ArriveEvent` は本家の @24359（Is Packaged For Distribution の真の側）を写したもので、中庭を歩いてトリガーに入るのが本家の遊び方。直したのは台本だけ。

## 要確認（ユーザー）

- 2026-09-20（ステップ 7）: **使っていない原作の章の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`（「TORMENT THERAPY」の文字の絵）が `/Game` に残っている**。画面には出ないが、クックは `/Game` を全部入れるのでパッケージには入る。消してよいか（アセットの削除は確認が要る）。同じく使っていない `/Game/Pipeline/Debug` の検証用の材質 17 個も、消すなら一緒に。

- 2026-09-20（ステップ 6）: **1080p・Epic の fps が両ゾーンで 46〜60 と、目安の「60 前後」にわずかに届かない**（どこも GPU 律速。メモリは目安内）。エディタにだけ効く対処では製品の fps は動かないので、次のどれにするかを決めてほしい。
  - (a) このままにする（PIE にはエディタの描画も載るので、パッケージした本編は速い見込み。実測にはクックが要る）。
  - (b) クックして本編の fps を測る（**配布の話なので無人モードでは行わない**。クックしてよいか）。
  - (c) 製品に画質の選択肢（解像度スケールか品質プリセット）を用意する。本家にも Options の画質設定があるので原作にも倣うが、作業一覧に無い新しい項目になる。

## 再開時の注意

- エディタはステップ 7 で開き直して `L_Hospital_Zone1` を開いている（ビルド ok、道具のプラグイン 3 つは有効のまま）。PIE は止まっている。保存していないマップは無い（2026-09-20 17:5x）。MCP は開き直すと切れるので、要るときは `/mcp` で繋ぎ直す。
- 性能を測る道具は `Tools/perf_probe.py`（`measure --label … --checkpoint N --level … --slomo 0.05`。PIE が要る）。チェックポイントで開くだけでは届かない場所（独房）は、`Intermediate/Overnight/setup_cell.py`（git の外）のように先に置いてから `--checkpoint` なしで測る。
- **通しの台本** `Tools/playthrough.py`（項目 27 で作り、項目 8・10・11・12・21 で育てた。説明は実装記録 01）: PIE（`Tools/pie.py start`）とデスクトップの代理（`Tools/desktop.py start`）の両方が要る。頭から流すときは `run --from title --to z2_escape --setup`（**`--setup` が要る**。無いと `L_Title` が開いておらず頭の区間が 5 s で落ちる）。終わったら**必ず** `python Tools/pie.py stop`。
  - **台本が走っている間に、ほかの `Tools/playthrough.py status` や `Tools/ue_remote.py` を呼ばない**。エディタのリモート実行は接続を 1 つしか持てず、後から繋ぐと走っている方が `ConnectionAbortedError` で落ちる。進み具合はログの `tail` で見る。
  - ビューポートの座標 `VIEWPORT = (1822, 206, 2862, 858)` は 2026-09-20 も合っていた。
- 台本は開発用のセーブを `Wasami.ResetSave` で書き換える（本作のセーブ。本家のセーブではない）。
- 収録は `Intermediate/DesktopAgent/`、絵は `Intermediate/DesktopAgent/shots`。ステップ 3 のログは `Intermediate/Overnight/through_21d.log`（救急車 → 通路）・`through_21e.log`（独房を単独で）。中庭を測った使い捨ての道具は `Intermediate/Overnight/probe_*.py`（git の外）。

## 検証

- check_records: ステップ 7 で実行（OK）
- C++ ビルド: 2026-09-20 のステップ 7 で `Tools/editor_cycle.py` が通った（`Result: Succeeded`。C++ は変えていないが uproject を変えたので開き直した）
- エディタでの確認（取り込み・組み立て・PIE）: 2026-09-20 に PIE で `z1_ambulance` → `z2_arrive` → `z2_cell` → `z2_corridor` を通し（終了コード 0）、`z2_cell` を単独（`--setup`）でも通した
