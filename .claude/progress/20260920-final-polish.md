---
title: 仕上げ（通しプレイ・性能・パッケージ）
status: 進行中
branch: main
base: 72d9cb4
started: 2026-09-20 14:43
updated: 2026-09-20 16:55
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
- [ ] 6. **性能の計測（Zone 2）と判断** ← 次
  - 目安（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）に対してどうか。外れていれば対処を次のステップに分ける（対処は**エディタにだけ効く場所**。製品の品質は落とさない）
- [ ] 7. パッケージの下ごしらえの確認だけ行う: 既定のマップとゲームモード（`Config/DefaultEngine.ini`）、`/Game` の外（`Intermediate/Pipeline/`）を指す参照が無いか、原作のロゴとキャラクターのモデルが入っていないか。**パッケージ（クック・ビルド）の実行そのものは配布の話なので無人モードでは行わず、要確認に書く**
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

## 次にやること

ステップ 6（性能の計測・Zone 2 と、目安に対する判断）。ステップ 5 と同じ測り方で Zone 2 の代表の 3 か所を測る: 独房（cp 7 で開いて `PlayerStart_Cell` へ。`Wasami.Flow OnCellCutsceneFinished`）・見張りの廊下（cp 8）・迷路か祭壇（cp 9 か 10）。手順は `python Tools/pie.py start`（エディタで `L_Hospital_Zone2` を開いてから）→ `python Tools/perf_probe.py measure --label z2_… --checkpoint N --level L_Hospital_Zone2 --slomo 0.05` → `python Tools/pie.py stop`。そのうえで目安（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）に対する判断を書く。Zone 1 は 46〜58 fps で 60 に届いていないが、**PIE にはエディタの描画も載る**ので、パッケージでどうなるかはクックしないと分からない（クックは配布の話なので無人モードでは行わない）。対処を入れるならエディタにだけ効く場所に限り、製品側を落とさないと収まらないと分かったら要確認に書く。

## 決定事項

- 2026-09-20（ステップ 5）: 性能の数字は**画面を読まず、エンジンの CSV プロファイラの列から取る**。`stat unit` などは画面に描くだけで、値をファイルに出す口が無いため。CSV には stat unit・stat RHI・stat memory・stat streaming に当たる列がそろっている。
- 2026-09-20（ステップ 5）: 1080p は `r.ScreenPercentage` で**画素数を合わせて**近似する。`r.SetRes` は PIE に効かず、PIE の窓（エディタのビューポート）の大きさは変えられないため。出力は 1039x654 のままなので、画素あたりの負荷は 1080p と同じだがポストプロセスと UI のぶんだけ軽い。
- 2026-09-20（ステップ 5）: 敵のいる場所は `slomo 0.05` で測る。等倍だと立っている間に捕まってレベルが開き直り、測定に混ざるため（fps の差は 1 % 未満）。

- 2026-09-20: パッケージ（クックとビルド）は無人モードでは**実行しない**。作業一覧の項目 21 の完了の条件が「配布の話なので先にユーザーに確認する（無人モードでは飛ばして要確認に書く）」と決めているため。ステップ 7 はクック前の確かめだけにする。
- 2026-09-20: 性能の対処が要るときも、変えてよいのは**エディタにだけ効く場所**だけ（`.claude/guides/performance.md` の大原則 2）。製品側を落とさないと収まらないと分かったら、勝手に決めずに要確認へ書く。
- 2026-09-20（ステップ 2・3）: Zone 2 の到着は**ゲーム側を直さない**。`ArriveEvent` は本家の @24359（Is Packaged For Distribution の真の側）を写したもので、中庭を歩いてトリガーに入るのが本家の遊び方。直したのは台本だけ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動していて `L_Hospital_Zone2` を開いている。PIE は止めた。保存していないマップは無い（2026-09-20 16:05）。
- **通しの台本** `Tools/playthrough.py`（項目 27 で作り、項目 8・10・11・12・21 で育てた。説明は実装記録 01）: PIE（`Tools/pie.py start`）とデスクトップの代理（`Tools/desktop.py start`）の両方が要る。頭から流すときは `run --from title --to z2_escape --setup`（**`--setup` が要る**。無いと `L_Title` が開いておらず頭の区間が 5 s で落ちる）。終わったら**必ず** `python Tools/pie.py stop`。
  - **台本が走っている間に、ほかの `Tools/playthrough.py status` や `Tools/ue_remote.py` を呼ばない**。エディタのリモート実行は接続を 1 つしか持てず、後から繋ぐと走っている方が `ConnectionAbortedError` で落ちる。進み具合はログの `tail` で見る。
  - ビューポートの座標 `VIEWPORT = (1822, 206, 2862, 858)` は 2026-09-20 も合っていた。
- 台本は開発用のセーブを `Wasami.ResetSave` で書き換える（本作のセーブ。本家のセーブではない）。
- 収録は `Intermediate/DesktopAgent/`、絵は `Intermediate/DesktopAgent/shots`。ステップ 3 のログは `Intermediate/Overnight/through_21d.log`（救急車 → 通路）・`through_21e.log`（独房を単独で）。中庭を測った使い捨ての道具は `Intermediate/Overnight/probe_*.py`（git の外）。

## 検証

- check_records: ステップ 3 で実行
- C++ ビルド: 未実行（ステップ 2・3 は C++ を変えていない）
- エディタでの確認（取り込み・組み立て・PIE）: 2026-09-20 に PIE で `z1_ambulance` → `z2_arrive` → `z2_cell` → `z2_corridor` を通し（終了コード 0）、`z2_cell` を単独（`--setup`）でも通した
