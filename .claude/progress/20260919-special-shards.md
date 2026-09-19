---
title: 特殊シャード 2 種（作業一覧の項目 10）
status: 進行中
branch: feature/special-shards
base: ae0762c
started: 2026-09-19 20:19
updated: 2026-09-19 21:30
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
- [ ] 5. 両ゾーンに置く
  - `dd_level._flow` に `BP_PowerOrb_C` → `AWasamiPowerOrb`、`BP_BonusShard_C` → `AWasamiBonusShard`（Zone 2 は `ID` 1。前処理の `props` にある）と出現点（`BP_PowerOrbSpawnPoint_C` Zone 1 に 11・Zone 2 に 10、`BP_BonusShardSpawnPoint_C` 10・10 → `AWasamiPowerOrbSpawnPoint`・`AWasamiBonusShardSpawnPoint`。前処理の `actors` に位置がある。置いたものの `Spawn Points` の配列は前処理が落としているが、各ゾーンの点がすべて配列に入っているので、本体の `SpawnPoints` を空にしてクラスで集めさせればよい）を足す。前処理が本体の灯を `lights` に並べ（`BP_PowerOrb_2.PointLight`・`BP_BonusShard_2.PointLight`）、前の組み立てが `Hospital/Lights/…` に置いているので、のこぎりの罠の灯と同じく `_lights` で外し、`place_flow` が既に置いたものを取り除く。本体の最初の位置は地図の外（Zone 1 のオーブ z 5725 など。最初の明滅は見えない）。本体に `dd_minimap` のタグを付けない（地図の板の組み立てがそのタグのアクタを消す）。
  - 本体のカプセルは `OverlapAllDynamic` で Pawn を止めず、印は当たりもナビゲーションも無いので、NavMesh は焼き直さなくてよいはず（置いた後に確かめ、変わっていれば `WasamiStageTools.build_navigation` で焼き直す）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_level.py`、両ゾーンのレベル（L_Hospital_Zone1・Zone2）、実装記録 01・16
- [ ] 6. PIE で確かめ、通しを流す
  - PIE（両ゾーン）: 155 s（かデバッグの `Wasami.PowerOrb [N]`・`Wasami.BonusShard [N]`）で出現点に明滅して現れ、地図に印、取るとオーブは ENEMIES STUNNED・橙の閃き・揺れ・敵が倒れて 17 s で起きる、赤いシャードは ENEMIES REVEALED・赤の閃き・地図に敵の印が 60 s、チェックポイントの後に死んで開き直すと赤いシャードが出ない、スコア画面の BONUS SHARDS が数える。収録して連番のグリッドを Discord に。
  - 台本 `Tools/playthrough.py` を頭から流し、置いた後も通ることを確かめる（台本が特殊シャードを取りに行く必要は無い）。
  - 変更予定: 要るなら `Tools/playthrough.py`、実装記録 16・01
- [ ] 7. 閉じる
  - 作業一覧の項目 10 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順に特殊シャードを足す）を直す。note の原稿に節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 5 を始める（作業ブランチ `feature/special-shards` の上）。`Content/Python/wasami_tools/pipeline/dd_level.py` の `_flow` と `_lights` を読み、のこぎりの罠（08 記録）の置き方と灯の外し方に倣って、本体 2 つと出現点を両ゾーンに置く。前処理の出力 `stage_ue.json` の `zones.<Zone>.actors`・`props`・`lights` で位置と `ID` を確かめる。

## 決定事項

- 2026-09-19: 最初の出現の時刻はアクタのコード（`Shard Spawn Time` 150 s + 明滅 5 s = 155 s。両方とも病院の置いたものは既定のまま）から取る。完了の条件の「レベル BP」は、病院の Zone のレベル BP がオーブと赤いシャードに触れないので読み替える — `pak_reference_2` の `_bytecode` を `BP_PowerOrb`・`BP_BonusShard` で grep して、参照は `BP_DD_GameMode`（`Used Stun Orbs?` を戻すだけ）・プレイヤー・エレメンタリーのレベルだけ。
- 2026-09-19: ゲームインスタンスの `Used Stun Orbs?` は写さない — 使うのは実績（エレメンタリー）だけで、本作に実績の仕組みが無い（罠の実績と同じ扱い）。
- 2026-09-19: 赤いシャードの見た目は本家の `soul_shard` × 20 と `m_crystal_Inst`（ユーザーの最終目標「本家と同一の見た目にする」）。通常のシャードのワサミ餅には替えない（WebGL 版は餅にしたが、最終目標の指定が優先）。
- 2026-09-19: 出現点は小さなアクタのクラス 2 つで置く（本家どおりのクラスで、組み立ては `spawn_actor_from_class` だけで済む。本体は `SpawnPoints` が空ならクラスで集める）。
- 2026-09-19: 特殊シャードは実装記録を新しい 16 に分ける（06 が 68 KB あり、シャード・セーブと別の仕組みのため）。
- 2026-09-19（ステップ 4）: 地図の印（特殊シャード・敵）は当たりを持たせない（本家はスタティックメッシュの既定の BlockAllDynamic。地図の矢印〈03 記録〉と同じく Zone 2 の上の階を遮らないように）。理由は実装記録 16・07 に書いた。

## 本家の流れ（読んだもの）

作ったものの流れは実装記録 16 の「内部構造と処理の流れ」。配置（ステップ 5）に要ることだけ残す。

- **レベル**: `_levels/06_Hospital_Zone_0{1,2}.full.json` に `BP_PowerOrb_2`・`BP_BonusShard_2` が 1 体ずつ、出現点が Zone 1 に 11・10、Zone 2 に 10・10。置いたものの上書きは Zone 2 の赤いシャードの `ID` 1 だけ（`Shard Spawn Time` は既定の 150）。前処理 `stage_ue.json` の `zones.<Zone>.actors` に本体と出現点の位置、`lights` に本体の灯がある。
- **地図**: `BP_MapTexture_MultiFloor` はシャードの印だけを階で出し分けるので、特殊シャードと敵の印は階に関わらず出る（本家どおり）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いたまま（PIE なし）。ステップ 4 のビルドで開き直した（MCP は再接続済み）。

## 検証

- check_records: OK（2026-09-19 21:29、16 件）
- C++ ビルド: 成功（2026-09-19、ステップ 4。最初のビルドはステップ 2・3 のファイルの無名名前空間の名前がぶつかって落ち、改名して通った）
- 自動テスト: `Wasami.*` 115 件すべて成功（2026-09-19、ステップ 4。`Wasami.BonusShard.*` 3 件を含む）
- `python Tools/check_unity_names.py`: ぶつかりなし（ステップ 4）
- エディタでの確認: ステップ 1 の素材が取り込まれ、推定のマスターがコンパイルされた。ステップ 2 の画面を PIE で `Show` して収録した（`Intermediate/Overnight/shots-vsides-*.png`）。本体の見え方は PIE（ステップ 6）で
