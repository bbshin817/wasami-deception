---
title: 仕上げ（通しプレイ・性能・パッケージ）
status: 進行中
branch: main
base: 72d9cb4
started: 2026-09-20 14:43
updated: 2026-09-20 14:43
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
- [ ] 2. **通しを頭から流して、止まる箇所を洗う** ← 次
  - `python Tools/pie.py start` → `python Tools/desktop.py start` → `python Tools/playthrough.py run --from title --to z2_escape --record through_21a.mkv`
  - 項目 12 のステップ 6b（`faf686f`）以降に入った要素（項目 30 ステージ OP・25 場面・29 EXTRAS・19 曲・20 台詞・26 追跡のランダムの動き）で台本の時機がずれている見込み。止まった区間・原因・直す先（ゲーム側か台本側か）をこの記録に一覧にするところまで。直しは次のステップ。
  - 変更予定: この記録だけ（直しは次）
- [ ] 3. 止まる箇所を直す（ゲーム側 / 台本側）。1 コミットに収まらなければその場でステップを分ける
  - 変更予定: `Tools/playthrough.py`、止まる原因しだいで `Source/wasami_deception/...`
- [ ] 4. 通しをもう 1 回、頭から最後まで流して通ることを確かめる（収録と、人に見せる連番のグリッド）
- [ ] 5. 性能の計測（Zone 1）: 1080p 相当で `stat unit`・`stat fps`・`stat RHI`・`stat memory`、代表の 3 か所（リフトの到着・迷路・駐車場）。測り方（PIE の解像度の合わせ方・値の取り方）はこのステップで決めて記録に書く
- [ ] 6. 性能の計測（Zone 2）と判断: 目安（1080p で 60 fps 前後・VRAM 5 GB まで・エディタの常駐 RAM 20 GB まで）に対してどうか。外れていれば対処を次のステップに分ける（対処は**エディタにだけ効く場所**。製品の品質は落とさない）
- [ ] 7. パッケージの下ごしらえの確認だけ行う: 既定のマップとゲームモード（`Config/DefaultEngine.ini`）、`/Game` の外（`Intermediate/Pipeline/`）を指す参照が無いか、原作のロゴとキャラクターのモデルが入っていないか。**パッケージ（クック・ビルド）の実行そのものは配布の話なので無人モードでは行わず、要確認に書く**
- [ ] 8. 締め: 実装記録（00 に性能の計測、01 に台本の直し）・`handover.md` の「現状と次の一歩」・`roadmap.md`（項目 21 を完了、**大目標 2 を「達成（2026-09-20）」、大目標 3 を「進行中」**）・note の記事 `docs/note/progress.md` と note 本体、`check_records.py --update`、進捗記録を消して最後のコミットに含める。状態ファイルは `continue`（`done` に「大目標 2 を達成し、大目標 3 へ移った」）

## 次にやること

ステップ 2。`python Tools/pie.py start` → `python Tools/desktop.py start` → `python Tools/playthrough.py run --from title --to z2_escape --record through_21a.mkv` を流し、止まった区間と原因を上の計画のステップ 2 の下に一覧で書く（直さない）。終わったら `python Tools/pie.py stop` と `python Tools/desktop.py stop`。

## 決定事項

- 2026-09-20: パッケージ（クックとビルド）は無人モードでは**実行しない**。作業一覧の項目 21 の完了の条件が「配布の話なので先にユーザーに確認する（無人モードでは飛ばして要確認に書く）」と決めているため。ステップ 7 はクック前の確かめだけにする。
- 2026-09-20: 性能の対処が要るときも、変えてよいのは**エディタにだけ効く場所**だけ（`.claude/guides/performance.md` の大原則 2）。製品側を落とさないと収まらないと分かったら、勝手に決めずに要確認へ書く。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動していて、`L_Hospital_Zone2` を開いている。PIE は動いていない。保存していないマップは無い（2026-09-20 14:43）。
- **通しの台本** `Tools/playthrough.py`（項目 27 で作り、項目 8・10・11・12 で育てた。説明は実装記録 01）: PIE（`Tools/pie.py start`）とデスクトップの代理（`Tools/desktop.py start`）の両方が要る。区間は `title, z1_arrive, pause, z1_maze, z1_shards, z1_parking, z1_ambulance, z2_cell, z2_corridor, z2_maze, z2_altar, z2_escape` の 12。頭からの通しは前回（`faf686f`）で 264 s。終わったら**必ず** `python Tools/pie.py stop`。
  - ビューポートの座標 `VIEWPORT = (1822, 206, 2862, 858)` はエディタの窓の配置に依存する。台本が画面を押せないときは、エディタの窓の絵を撮って測り直す。
- 台本は開発用のセーブを `Wasami.ResetSave` で書き換える（本作のセーブ。本家のセーブではない）。
- 収録は `Intermediate/DesktopAgent/` の下。撮った絵は `Intermediate/DesktopAgent/shots`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
