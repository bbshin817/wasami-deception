---
title: 特殊シャード 2 種（作業一覧の項目 10）
status: 進行中
branch: feature/special-shards
base: ae0762c
started: 2026-09-19 20:19
updated: 2026-09-19 22:06
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 特殊シャード 2 種（作業一覧の項目 10）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 10（大目標 2）。本家と同一の見た目の `BP_PowerOrb`（スタンオーブ）と `BP_BonusShard`（赤いシャード）を、本家の出現点と周期で出し、取ると敵が 17 s 気絶（秒数とアニメは敵の側。項目 4・7 で作り済み）／敵が 60 s タブレットの地図に出る。完了の条件: 出現点（Zone 1: オーブ 11・ボーナス 10、Zone 2: 10・10）と周期・最初の出現の時刻がコードどおり。メッシュと材質は原作のアセット（`power_orb`、赤いシャードの材質）。取得で `UMG_VignetteSides`（`ENEMIES STUNNED` / `ENEMIES REVEALED`）が出て、全敵に `SetState(Stun, byOrb)`、地図に敵の印。テレキネシスでは引き寄せられない。大目標 1・2 の決め方（見た目を本家と見比べて詰めない。推定の材質・粒子は項目 28 の後回しの一覧へ）で進める。

## 計画

- [x] 0. 本家のコードと今の実装を読み、計画を立てる … 2026-09-19 完了。
- [x] 1. 素材の取り込み … 2026-09-19 完了。`dd_specials.py` と `WasamiDDTools.import_dd_specials`。作ったもののパスと推定は実装記録 16 の「作るアセット」。
- [x] 2. 画面 `UWasamiVignetteSidesWidget`（本家 `UMG_VignetteSides`）とテスト … 2026-09-19 完了。実装記録 16。
- [x] 3. オーブ `AWasamiPowerOrb` と取得の演出 `AWasamiStunCollectEffect`、出現点 2 種、テスト … 2026-09-19 完了。実装記録 16・04。
- [x] 4. 赤いシャード `AWasamiBonusShard` と `AWasamiBonusShardCollectEffect`、地図の敵の印、テスト … 2026-09-19 完了。オーブと同じ部品と周期を基底 `AWasamiSpecialShard` に移した（`SpawnPowerOrb` → `SpawnSpecialShard`）。プレイヤーの `AddToMap`・`RemoveFromMap`・`IsOnMap`（02 記録）、敵の `MapMark`（07 記録）、デバッグの `Wasami.BonusShard [N]`、テスト `Wasami.BonusShard.Parts`・`.Save`・`.Collect`。ユニティビルドの名前のぶつかりを洗い出す `Tools/check_unity_names.py` を足した。作りは実装記録 16。
- [x] 5. 両ゾーンに置く … 2026-09-19 完了。`dd_level._flow` が本体 2 つと出現点を置き（`SpawnPoints` は空でクラスで集める。Zone 2 の赤いシャードは `ID` 1）、本体の灯を単独で置かない。両ゾーンに `place_dd_flow`（Zone 1: 本体 2・点 21、Zone 2: 2・20、前の灯 2 つずつを外した、`failed_settings` 0）→ `build_navigation`（2/2・29/29）。作りは実装記録 01・16 の「配置」。
- [x] 6. PIE で確かめ、通しを流す … 2026-09-19 完了。両ゾーンで周期・明滅・取得・気絶・地図の暴き・セーブ・スコア画面を確かめ、台本の通しが 11 区間とも通った（1 回目に止まった `z2_altar` の歩きを `snap` にした）。結果は実装記録 16 の「PIE での確かめ」、台本は 01。
- [ ] 7. 閉じる
  - 作業一覧の項目 10 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順に特殊シャードを足す）を直す。note の原稿に節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 7（閉じる）を始める（作業ブランチ `feature/special-shards` の上）。note の GIF は、ステップ 6 の収録（git の外の `Intermediate/DesktopAgent/shots/`）から切り出せる: オーブの取得 `pie-orb-collect2.mkv`（2.4〜4.5 s）、赤いシャードの取得 `pie-bonus-collect.mkv`（2.0〜4.2 s）、明滅と移る閃光 `pie-orb-flicker.mkv`（5.9〜7.4 s。手前に餅が重なる）。地図の暴きは静止画 3 枚を並べた `Intermediate/Overnight/specials-grid-map.png`。GIF の作り方は `.claude/guides/note-progress.md` と `observations/tools/note_gif.py`。

## 決定事項

（実装済みの決定の理由は実装記録 16 に移した）

- 2026-09-19: 最初の出現の時刻はアクタのコード（`Shard Spawn Time` 150 s + 明滅 5 s = 155 s。両方とも病院の置いたものは既定のまま）から取る。完了の条件の「レベル BP」は、病院の Zone のレベル BP がオーブと赤いシャードに触れないので読み替える — `pak_reference_2` の `_bytecode` を `BP_PowerOrb`・`BP_BonusShard` で grep して、参照は `BP_DD_GameMode`（`Used Stun Orbs?` を戻すだけ）・プレイヤー・エレメンタリーのレベルだけ。

## 本家の流れ（読んだもの）

作ったものの流れと置き場所は実装記録 16 の「内部構造と処理の流れ」と「配置」。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いたまま（PIE なし、いまのレベルは L_Hospital_Zone1）。両ゾーンのレベルは git の外（`Content/Stage/`）なので、配置はコミットに入らない（作り直しは `place_dd_flow` → 別の呼び出しで `build_navigation`）。
- 開発用のセーブ（`Saved/SaveGames/structSlot.sav`）は、ステップ 6 の通しの終わりの脱出で空（チェックポイント 0）になっている。ステップ 6 の前のセーブ（Zone 2 のチェックポイント 7）の控えは取らなかった。Zone 2 の確かめの後の控え（チェックポイント 9・`BonusShards` {1}）は `Intermediate/SaveBackups/20260919-special-shards/`。

## 検証

- check_records: OK（2026-09-19 22:05、16 件）
- C++ ビルド: 成功（2026-09-19、ステップ 4。最初のビルドはステップ 2・3 のファイルの無名名前空間の名前がぶつかって落ち、改名して通った）
- 自動テスト: `Wasami.*` 115 件すべて成功（2026-09-19、ステップ 4。`Wasami.BonusShard.*` 3 件を含む）
- `python Tools/check_unity_names.py`: ぶつかりなし（ステップ 4）
- エディタでの確認: ステップ 1 の素材が取り込まれ、推定のマスターがコンパイルされた。ステップ 2 の画面を PIE で `Show` して収録した（`Intermediate/Overnight/shots-vsides-*.png`）。ステップ 6 で両ゾーンの本体・取得・地図・セーブを PIE で確かめた（実装記録 16 の「PIE での確かめ」）
- 台本の通し: `python Tools/playthrough.py run --from z1_arrive --to z2_escape --setup --record shards_through2.mkv --record-seconds 420 --shots` で 11 区間すべて（終了コード 0、266 s。2026-09-19 22:02）
