---
title: 特殊シャード 2 種（作業一覧の項目 10）
status: 進行中
branch: feature/special-shards
base: ae0762c
started: 2026-09-19 20:19
updated: 2026-09-19 21:36
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
- [ ] 6. PIE で確かめ、通しを流す
  - PIE（両ゾーン）: 155 s（かデバッグの `Wasami.PowerOrb [N]`・`Wasami.BonusShard [N]`）で出現点に明滅して現れ、地図に印、取るとオーブは ENEMIES STUNNED・橙の閃き・揺れ・敵が倒れて 17 s で起きる、赤いシャードは ENEMIES REVEALED・赤の閃き・地図に敵の印が 60 s、チェックポイントの後に死んで開き直すと赤いシャードが出ない、スコア画面の BONUS SHARDS が数える。収録して連番のグリッドを Discord に。
  - 台本 `Tools/playthrough.py` を頭から流し、置いた後も通ることを確かめる（台本が特殊シャードを取りに行く必要は無い）。
  - 変更予定: 要るなら `Tools/playthrough.py`、実装記録 16・01
- [ ] 7. 閉じる
  - 作業一覧の項目 10 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順に特殊シャードを足す）を直す。note の原稿に節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 6 を始める（作業ブランチ `feature/special-shards` の上）。`Tools/pie.py` で Zone 1 の PIE を始め、デバッグの `Wasami.PowerOrb` / `Wasami.PowerOrb N`・`Wasami.BonusShard N`（N は出現点の番号。明滅なしで点 N へ）で本体を出し、プレイヤーを `place` でその近くに置いて見え方・地図の印・取得（画面・閃き・揺れ・敵の気絶 / 地図の敵の印）を確かめる。手順は `.claude/guides/observation.md` と `Tools/pie.py`。

## 決定事項

（実装済みの決定の理由は実装記録 16 に移した）

- 2026-09-19: 最初の出現の時刻はアクタのコード（`Shard Spawn Time` 150 s + 明滅 5 s = 155 s。両方とも病院の置いたものは既定のまま）から取る。完了の条件の「レベル BP」は、病院の Zone のレベル BP がオーブと赤いシャードに触れないので読み替える — `pak_reference_2` の `_bytecode` を `BP_PowerOrb`・`BP_BonusShard` で grep して、参照は `BP_DD_GameMode`（`Used Stun Orbs?` を戻すだけ）・プレイヤー・エレメンタリーのレベルだけ。

## 本家の流れ（読んだもの）

作ったものの流れと置き場所は実装記録 16 の「内部構造と処理の流れ」と「配置」。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いたまま（PIE なし、いまのレベルは L_Hospital_Zone2）。両ゾーンのレベルは git の外（`Content/Stage/`）なので、配置はコミットに入らない（作り直しは `place_dd_flow` → 別の呼び出しで `build_navigation`）。

## 検証

- check_records: OK（2026-09-19 21:36、16 件）
- C++ ビルド: 成功（2026-09-19、ステップ 4。最初のビルドはステップ 2・3 のファイルの無名名前空間の名前がぶつかって落ち、改名して通った）
- 自動テスト: `Wasami.*` 115 件すべて成功（2026-09-19、ステップ 4。`Wasami.BonusShard.*` 3 件を含む）
- `python Tools/check_unity_names.py`: ぶつかりなし（ステップ 4）
- エディタでの確認: ステップ 1 の素材が取り込まれ、推定のマスターがコンパイルされた。ステップ 2 の画面を PIE で `Show` して収録した（`Intermediate/Overnight/shots-vsides-*.png`）。本体の見え方は PIE（ステップ 6）で
