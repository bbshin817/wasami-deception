---
title: 仕上げ（通しプレイ・性能・パッケージ）
status: 進行中
branch: main
base: 72d9cb4
started: 2026-09-20 14:43
updated: 2026-09-20 15:30
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
- [ ] 5. **性能の計測（Zone 1）** ← 次: 1080p 相当で `stat unit`・`stat fps`・`stat RHI`・`stat memory`、代表の 3 か所（リフトの到着・迷路・駐車場）。測り方（PIE の解像度の合わせ方・値の取り方）はこのステップで決めて記録に書く
- [ ] 6. 性能の計測（Zone 2）と判断: 目安（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）に対してどうか。外れていれば対処を次のステップに分ける（対処は**エディタにだけ効く場所**。製品の品質は落とさない）
- [ ] 7. パッケージの下ごしらえの確認だけ行う: 既定のマップとゲームモード（`Config/DefaultEngine.ini`）、`/Game` の外（`Intermediate/Pipeline/`）を指す参照が無いか、原作のロゴとキャラクターのモデルが入っていないか。**パッケージ（クック・ビルド）の実行そのものは配布の話なので無人モードでは行わず、要確認に書く**
- [ ] 8. 締め: 実装記録（00 に性能の計測、01 に台本の直し）・`handover.md` の「現状と次の一歩」・`roadmap.md`（項目 21 を完了、**大目標 2 を「達成（2026-09-20）」、大目標 3 を「進行中」**）・note の記事 `docs/note/progress.md` と note 本体、`check_records.py --update`、進捗記録を消して最後のコミットに含める。状態ファイルは `continue`（`done` に「大目標 2 を達成し、大目標 3 へ移った」）

## ステップ 4 の通し（2026-09-20）

- `run --from title --to z2_escape --setup --shots --record through_21.mkv --record-seconds 500` が**終了コード 0**。ログは `Intermediate/Overnight/through_21f.log`、収録は `Intermediate/DesktopAgent/shots/through_21.mkv`、区間ごとの絵は同じ場所の `pt_*.png`（27 枚）。
- 区間の時間（括弧はステップ 2・3 の前回）: title 13.8 s(17.0) / z1_arrive 25.5(27.9) / pause 18.9(18.5) / z1_maze 27.7(30.1) / z1_shards 6.4(6.2) / z1_parking 17.2(18.4) / z1_ambulance 45.5(33.0) / **z2_arrive 128.5**(128.4) / z2_cell 14.5(14.5) / z2_corridor 41.0(41.0) / z2_maze 1.2(1.0) / z2_altar 26.4(24.3) / z2_escape 38.7(38.1)。**合計 408.4 s**。
- 止まった箇所は無い。台本が自分で直したのは 2 か所（前回と同じ性質）: z1_ambulance の車庫で 1 回「stuck: path again」（経路を引き直して進んだ。z1_ambulance が前回より 12 s 長いのはこれ）、z2_corridor で 1 回「キーが届かない → ビューポートを押し直す」。
- スコア画面の結果（`pt_z2_escape_results.png`）: TIME 5:25（ゲーム内の時間。台本の 408 s はレベルの読み込みとメニューを含む）、SOUL SHARDS 679、BONUS 0/2、SECRETS 0/4、LIVES LOST 1、SHARD STREAK 500、TOTAL 819、**FINAL RANK A**。
- 通しで確かめた流れ: タイトルの NEW GAME → Zone 1 の 04 到着・鍵開け → 一時停止のメニュー（感度・QUIT TO TITLE・RESUME）→ 迷路で捕まって死亡・05 から再開（ライフ 3→2）→ シャード全回収（ライフ 3 に戻る）→ 駐車場 → 救急車の屋根 → Zone 2 の中庭・捕まる場面・独房 → 通路（Vanish で見張りの前を抜ける）→ 迷路 → 祭壇のリングの欠片 → ガレージのポータル → スコア画面（You Escaped!・FINAL RANK）→ 5 s でタイトル（セーブは空、RESUME 無し）。

## 次にやること

ステップ 5（性能の計測・Zone 1）。測り方をこのステップで決めて記録に書く: PIE の解像度を 1080p 相当に合わせ（`r.SetRes 1920x1080f` は PIE では効かないので、新しいエディタの窓〈`Wasami.Pie` 相当が無ければ PIE の窓の大きさ〉か `r.ScreenPercentage` で合わせる案を先に試す）、`stat unit`・`stat fps`・`stat RHI`・`stat memory` を代表の 3 か所（リフトの到着 cp 4・迷路 cp 5・駐車場 cp 6）で読む。読み取りは画面の数字ではなく、コンソールの `stat` の出力か `Tools/pie.py cmd` で取れる値を優先する（数字を目で読むなら `Tools/desktop.py shot` の切り出し）。目安は `.claude/guides/performance.md`（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）。台本の区間（`z1_arrive`・`z1_maze`・`z1_parking`）で場所へ運ぶと速い。

## 決定事項

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
