---
title: 特殊シャード 2 種（作業一覧の項目 10）
status: 進行中
branch: feature/special-shards
base: ae0762c
started: 2026-09-19 20:19
updated: 2026-09-19 20:35
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 特殊シャード 2 種（作業一覧の項目 10）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 10（大目標 2）。本家と同一の見た目の `BP_PowerOrb`（スタンオーブ）と `BP_BonusShard`（赤いシャード）を、本家の出現点と周期で出し、取ると敵が 17 s 気絶（秒数とアニメは敵の側。項目 4・7 で作り済み）／敵が 60 s タブレットの地図に出る。完了の条件: 出現点（Zone 1: オーブ 11・ボーナス 10、Zone 2: 10・10）と周期・最初の出現の時刻がコードどおり。メッシュと材質は原作のアセット（`power_orb`、赤いシャードの材質）。取得で `UMG_VignetteSides`（`ENEMIES STUNNED` / `ENEMIES REVEALED`）が出て、全敵に `SetState(Stun, byOrb)`、地図に敵の印。テレキネシスでは引き寄せられない。大目標 1・2 の決め方（見た目を本家と見比べて詰めない。推定の材質・粒子は項目 28 の後回しの一覧へ）で進める。

## 計画

- [x] 0. 本家のコードと今の実装を読み、計画を立てる … 2026-09-19 完了。読んだものは下の「本家の流れ（読んだもの）」と「今の実装」。
- [x] 1. 素材の取り込み … 2026-09-19 完了。`dd_specials.py`（新）と `WasamiDDTools.import_dd_specials`（`import_dd_shards`・`import_dd_gimmicks` の後）。作ったものと推定は実装記録 16。本体の各部品が読むパス: メッシュ `/Game/DD/Meshes/Shared/power_orb`・`/Game/DD/Meshes/Ring_Assets/soul_shard`、材質 `/Game/DD/Materials/Fords_Materials/m_crystal_Inst3`・`m_crystal_Inst`、印 `/Game/DD/Materials/Shared/M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`、粒子 `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_{PowerOrb,BonusOrb}_{Appear,Disappear}`・`P_ky_impact`・`P_ky_impact1`、音 `/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_`・`Bonus_Shard_Pickup_v1`・`Stun_Wave_Attack_New_04`。
- [ ] 2. 画面 `UWasamiVignetteSidesWidget`（本家 `UMG_VignetteSides`）とテスト
  - 木をスロットのまま C++ で組み（`UWasamiShardStreakWidget` と同じ作り。13 記録）、`Color`・`Text?`・`TextToDisplay` を受け、Construct で文字と色、`Anim`（1.5 s。キーは書き出しの値。WebGL 版 10 記録の `VSIDES_ANIM` が同じ値を写している）を 1 回、2 s で `RemoveFromParent`。`Text?` 偽なら文字を隠す。Z 5 で出す。
  - テスト `Wasami.VignetteSides.*`（文字・色、2 s で消える、アニメの値）。
  - 変更予定: `Source/wasami_deception/WasamiVignetteSidesWidget.*`（新）、`Tests/WasamiVignetteSidesTests.cpp`（新）、実装記録 16（画面は 09 にも一言）・`_index.md`
- [ ] 3. オーブ `AWasamiPowerOrb` と取得の演出 `AWasamiStunCollectEffect`、テスト
  - 部品と流れは下の「本家の流れ」。出現点は置いた点を配列で持つ（点を専用の小さなクラスで置くか、組み立てが位置の配列を書くかは、ステップ 5 の組み立ての手間で決める）。
  - `AWasamiStunCollectEffect` は `AWasamiPowerBurst`（04 記録。PostProcess・PostProcess1 と 2 s のタイムライン）の派生にし、球（`M_05_Primal`、`Color` (0.258, 0.0737, 0)、半径 = Lerp(0, Range, float) / 50 の拡縮、`Desaturation`・`Opacity`）をプレイヤーの位置に。`Range` と PP の値・曲線は `_assets/…/Powers/BP_StunCollectEffect.json` から。Primal Fear（`AWasamiPrimalPower`）の球と同じ作りなら共有する。
  - デバッグのコンソールコマンド（例 `Wasami.Special orb|bonus [点]`: 今すぐ明滅を始める・点へ移す）を置く（155 s を待たずに確かめるため）。
  - テスト `Wasami.PowerOrb.*`（部品の値、150 s で明滅 → 5 s で出現点へ・次の 150 s、明滅の 0.1 s の交互と隠れている間も取れること、取得で全 `Enemy` に `SetState(Stun, true)`・壊れる・1 回だけ、プレイヤー以外では取れない、テレキネシスのインターフェースを持たない）。
  - 変更予定: `Source/wasami_deception/WasamiPowerOrb.*`（新）、`WasamiStunCollectEffect.*`（新）、`Tests/WasamiSpecialShardTests.cpp`（新）、実装記録 16・04（`AWasamiPowerBurst` の派生）・`_index.md`
- [ ] 4. 赤いシャード `AWasamiBonusShard` と `AWasamiBonusShardCollectEffect`、地図の敵の印、テスト
  - 流れは下の「本家の流れ」。`ID`（Zone 2 は 1）を持ち、`BeginPlay` でセーブ（`AWasamiGameMode::GetSave()` の `BonusShards`）にあれば消える。取得でセーブの `BonusShards` に `AddUnique`（書き込みは次のチェックポイントの保存。本家どおり）。
  - 地図: プレイヤーに `AddToMap(Class)`・`RemoveFromMap(Class)`（本家どおりそのクラスの全アクタをミニマップのキャプチャに足す・外す）を足し、`RefreshMinimapContents` が作り直すときに残す集合にする（02・03 記録）。敵 `AWasamiEnemy` に本家のナースの地図の印（`StaticMesh` = Plane・`M_Enemy`・相対位置 (0, 21.88, 1117.84)・拡縮 (2.524, 2.524, 10)・影なし）を足す（07 記録）。オーブと赤いシャードの自分の印（Plane・`M_PowerOrb` / `M_Bonus_Shard`・(0, 0, 2000)・(1.5, 1.5, 10)）は本家どおりキャプチャが常に写す（本家のプレイヤーの ShowOnly の一覧に `BP_PowerOrb`・`BP_BonusShard` がある）。Zone 2 の階ごとの地図はシャードだけを見るので、印は階に関わらず出る（本家どおり）。
  - テスト（同じファイル）: セーブにある ID なら消える、取得でセーブに ID・部品が消える・60 s は残る、1〜2 s ごとに敵を地図に足し直し（後から出た敵も載る）、60 s で外して消える、地図の集合が `RefreshMinimapContents` の後も残る。
  - 変更予定: `Source/wasami_deception/WasamiBonusShard.*`（新）、`WasamiBonusShardCollectEffect.*`（新。3 と同じファイルでもよい）、`WasamiPlayerCharacter.*`、`WasamiEnemy.*`、テスト、実装記録 16・02・03・07
- [ ] 5. 両ゾーンに置く
  - `dd_level._flow` に `BP_PowerOrb_C` → `AWasamiPowerOrb`、`BP_BonusShard_C` → `AWasamiBonusShard`（Zone 2 は `ID` 1。前処理の `props` にある）と出現点（`BP_PowerOrbSpawnPoint_C` Zone 1 に 11・Zone 2 に 10、`BP_BonusShardSpawnPoint_C` 10・10。前処理の `actors` に位置がある。置いたものの `Spawn Points` の配列は前処理が落としているが、各ゾーンの点がすべて配列に入っているので、クラスで集めればよい）を足す。前処理が本体の灯を `lights` に並べ（`BP_PowerOrb_2.PointLight`・`BP_BonusShard_2.PointLight`）、前の組み立てが `Hospital/Lights/…` に置いているので、のこぎりの罠の灯と同じく `_lights` で外し、`place_flow` が既に置いたものを取り除く。本体の最初の位置は地図の外（Zone 1 のオーブ z 5725 など。最初の明滅は見えない）。
  - カプセルの `AreaClass`（NavArea_Obstacle）が NavMesh に効くなら、本家と同じか確かめたうえで焼き直す（`WasamiStageTools.build_navigation`）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_level.py`、両ゾーンのレベル、実装記録 01・16
- [ ] 6. PIE で確かめ、通しを流す
  - PIE（両ゾーン）: 155 s（かデバッグのコマンド）で出現点に明滅して現れ、地図に印、取るとオーブは ENEMIES STUNNED・橙の閃き・揺れ・敵が倒れて 17 s で起きる、赤いシャードは ENEMIES REVEALED・赤の閃き・地図に敵の印が 60 s、チェックポイントの後に死んで開き直すと赤いシャードが出ない、スコア画面の BONUS SHARDS が数える。収録して連番のグリッドを Discord に。
  - 台本 `Tools/playthrough.py` を頭から流し、置いた後も通ることを確かめる（台本が特殊シャードを取りに行く必要は無い）。
  - 変更予定: 要るなら `Tools/playthrough.py`、実装記録 16・01
- [ ] 7. 閉じる
  - 作業一覧の項目 10 を完了にし（完了の条件の読み替えを書く）、実装記録と handover の「現状と次の一歩」（遊んで確かめる手順に特殊シャードを足す）を直す。note の原稿に節と GIF を足して記事を書き換える（`.claude/guides/note-progress.md`）。作業ブランチの上でこの記録を消し、main へマージして push、ブランチを消す。

## 次にやること

ステップ 2 を始める（作業ブランチ `feature/special-shards` の上）。`UWasamiShardStreakWidget`（13 記録）の木の組み方を読み、`_assets/DDeception/Content/UI/Menu/Streaks/UMG_VignetteSides.json` の木と `Anim` のキーから `UWasamiVignetteSidesWidget` を C++ で組む。WebGL 版 10 記録の `VSIDES_ANIM`・`VSIDES` が同じ値を写している（照らし合わせに使う）。

## 決定事項

- 2026-09-19: 最初の出現の時刻はアクタのコード（`Shard Spawn Time` 150 s + 明滅 5 s = 155 s。両方とも病院の置いたものは既定のまま）から取る。完了の条件の「レベル BP」は、病院の Zone のレベル BP がオーブと赤いシャードに触れないので読み替える — `pak_reference_2` の `_bytecode` を `BP_PowerOrb`・`BP_BonusShard` で grep して、参照は `BP_DD_GameMode`（`Used Stun Orbs?` を戻すだけ）・プレイヤー・エレメンタリーのレベルだけ。
- 2026-09-19: ゲームインスタンスの `Used Stun Orbs?` は写さない — 使うのは実績（エレメンタリー）だけで、本作に実績の仕組みが無い（罠の実績と同じ扱い）。
- 2026-09-19: 赤いシャードの見た目は本家の `soul_shard` × 20 と `m_crystal_Inst`（ユーザーの最終目標「本家と同一の見た目にする」）。通常のシャードのワサミ餅には替えない（WebGL 版は餅にしたが、最終目標の指定が優先）。
- 2026-09-19: 特殊シャードは実装記録を新しい 16 に分ける（06 が 68 KB あり、シャード・セーブと別の仕組みのため）。

## 本家の流れ（読んだもの）

`python Tools/dd/bp_flow.py pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/<BP>.txt <イベント>`。部品の値は `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/<BP>.json`。

- **両方に共通**: 1 体を置いておき、出現点を移る（置いた位置は地図の外）。`BeginPlay` で `Shard Spawn Time`（CDO 150）のタイマー → `Spawn …`: `Flicker()`（`DefaultSceneRoot` の見え隠れを反転。最初の呼び出しで隠れる）、0.1 s ごとの `Flicker` のタイマー、`Delay 5` → `Flicker()`・タイマーを消す・見える、今の位置に消える粒子、出現点を `RandomIntegerInRange(0, n−1)`（今の点も選ばれる。「選び済み」の分岐は定数 False で切れている）→ `SetActorLocation`（テレポート）→ タイマーを 150 s で掛け直す → 新しい位置に現れる粒子。`ReceiveTick` は `soul_shard` を `Float 3`（既定 0）で回すので回らない。重なりはプレイヤーだけ、DoOnce。見え隠れはカプセルを残すので隠れている間も取れる。
  - SCS: `DefaultSceneRoot` → `soul_shard`（当たり無し）→ `Capsule`・`PPP_Collect_Shard`（起こさない）・`PointLight`（減衰 200・強さ 1000・影なし）、`DefaultSceneRoot` → `StaticMesh`（地図の印。Plane、(0, 0, 2000)、(1.5, 1.5, 10)、影なし）。`Capsule` は相対 (0, 0, 0.29)・拡縮 0.1・`AreaClass` NavArea_Obstacle。
- **オーブ** `BP_PowerOrb`: `soul_shard` = `power_orb`・`m_crystal_Inst3`・(0, 0, 125.25)・拡縮 0.5408。カプセル 840.6（× 0.1 = 84 cm）。灯の色 FColor (0, 146, 255) = sRGB (255, 146, 0)。地図の印 `M_PowerOrb`。粒子 `P_ky_flash_PowerOrb_Disappear`・`_Appear` を拡縮 0.5。
  - 取得: 2 つのタイマーを消す → `P_ky_impact`（拡縮 1）→ `DestroyActor` → ゲームインスタンスの `Used Stun Orbs?`（写さない）→ `PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75)` → `UMG_VignetteSides`（Color (1, 0.4654, 0)、`Text?` 真、`ENEMIES STUNNED`、Z 5）→ `PlayCameraShake(BP_CameraShake_Streak, 1)` → `PlaySoundAtLocation(8-Dark_power_ball_countdown_, (0,0,0), 1, 1)` → `BP_StunCollectEffect` を出す → タグ `Enemy` の全アクタの `DD_EnemyInterface.Set State(2, True)`（2 = Stun）。
  - `BP_StunCollectEffect`: `BeginPlay` で球に `M_05_Primal` の動的インスタンス・`Color` (0.258, 0.0737, 0)、プレイヤーの位置へ、`PlaySoundAtLocation(Stun_Wave_Attack_New_04, (0,0,0), 1, 2)`、`ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, 25)`、タイムライン（2 s）: 球の拡縮 = Lerp(0, Range, float) / 50、PostProcess の重み Lerp(1, 0, float2)、PostProcess1 は MapRangeClamped(float2, 0, 0.3, 1, 0)、`Desaturation`・`Opacity` のトラック。終わりで消える。
- **赤いシャード** `BP_BonusShard`: `soul_shard` = `soul_shard`・`m_crystal_Inst`・(0, 0, 97.09)・拡縮 20。カプセル 49.57（× 0.1 = 5 cm）。灯 FColor (0, 31, 255) = sRGB (255, 31, 0)。地図の印 `M_Bonus_Shard`。粒子 `P_ky_flash_BonusOrb_Disappear`・`_Appear` を拡縮 1。`ID`（Zone 1 は既定 0、Zone 2 は 1）。
  - `BeginPlay`: `Delay 0` の後、ゲームモードの `Struct Save` の今のレベルの `BonusShards` に `ID` があれば `DestroyActor`、無ければタイマー（`Float 1` と `Game State` は入れるだけで使わない）。
  - 取得: タイマーを消す → セーブの `BonusShards` に `AddUnique(ID)` → `P_ky_impact1` → `PlaySound2D(Bonus_Shard_Pickup_v1, 0.8, 1)` → `UMG_VignetteSides`（Color (1, 0, 0.0167)、`ENEMIES REVEALED`、Z 5）→ `BP_CameraShake_Streak` → `StaticMesh`・`soul_shard`・`PointLight` を消す（アクタは残る）→ `BP_BonusShardCollectEffect` → 地図: タグ `Enemy` の全アクタについてプレイヤーの `Add To Map(そのクラス)`（`GetAllActorsOfClass` をミニマップの `SceneCaptureComponent2D.ShowOnlyActors` に足す）。ループの中の `Delay(RandomFloatInRange(1, 2))` が明けると Gate（開いて始まる）を通って全敵を足し直す（1〜2 s ごと。後から出た敵も載る）。ループの後の `Delay 60` → Gate を閉じ、全敵の `Remove From Map(クラス)`（`ShowOnlyActors` から外す）→ `DestroyActor`。
  - `BP_BonusShardCollectEffect`: 球なしの PostProcess 2 つ（PostProcess: 彩度 0・ColorGain (1.61, 0, 0.447)、PostProcess1: ColorGain (1.61, 0.1296, 0)・Midtones 100・Fringe 50）と 2 s のタイムライン、`PlaySoundAtLocation(Stun_Wave_Attack_New_04, (0,0,0), 0.5, 1.5)`、`ElevatorShakeStop` 25。
- **地図**: 本家のプレイヤーの ShowOnly の一覧は `BP_Player_TelepathyVisualization`・`BP_ArrowPointer`・`BP_Shard`・`BP_MapTexture`・`BP_PowerOrb`・`BP_BonusShard`・`BP_MiniMapMarker` のクラスの全アクタ。敵はふだん写らず、`Add To Map` の間だけ（本家のナース `BP_06_ReaperNurse` の `StaticMesh` = Plane・`M_Enemy`・(0, 21.88, 1117.84)・(2.524, 2.524, 10)）。`BP_MapTexture_MultiFloor` はシャードの印だけを階で出し分ける。
- **レベル**: `_levels/06_Hospital_Zone_0{1,2}.full.json` に `BP_PowerOrb_2`・`BP_BonusShard_2` が 1 体ずつ、出現点が Zone 1 に 11・10、Zone 2 に 10・10。置いたものの上書きは Zone 2 の赤いシャードの `ID` 1 だけ（`Shard Spawn Time` は既定の 150）。前処理 `stage_ue.json` の `zones.<Zone>.actors` に本体と出現点の位置、`lights` に本体の灯がある。

## 今の実装

- 敵: `IWasamiEnemyInterface::SetState(State, bByOrb)` と 17 s の気絶・倒れる 2 本のアニメは作り済み（07 記録）。敵はタグ `Enemy`（`WasamiEnemy.cpp`）。
- セーブ: `UWasamiSaveGame` の病院の欄に `TArray<int32> BonusShards`（06 記録）。スコア画面の BONUS SHARDS は `Progress.BonusShards.Num()` を数える（`WasamiLevelResults.cpp`。13 記録）。ゲームモードの `GetSave()`。
- ミニマップ: `AWasamiPlayerCharacter::RefreshMinimapContents` がタグ `MinimapTag`・矢印・シャードで `ShowOnlyActors` を毎回作り直す（02・03 記録）。シャードの地図の印は `AWasamiShard` の Plane・`M_Shard`（`M_DD_MapMark` のインスタンス。`dd_shards.py`）。
- 一瞬の演出の基底 `AWasamiPowerBurst` と、球を持つ `AWasamiPrimalPower`（04 記録）。揺れ `BP_CameraShake_Streak` は `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いたまま（PIE なし）。新しいツール `WasamiDDTools.import_dd_specials` はエディタを開き直すまで MCP に出ない（ステップ 2 の C++ のビルドの開き直しで出る）。

## 検証

- check_records: OK（2026-09-19 20:33、16 件）
- C++ ビルド: 未実行
- エディタでの確認: ステップ 1 の取り込みが通り（音 3・テクスチャ 4・結晶の材質 4・メッシュ 2・印 4・粒子の材質 7・粒子 6）、推定のマスター 4 つがコンパイルされ、インスタンスの値と `useHilight` 真・`power_orb` のスロットが本家どおりと確かめた。見え方は PIE（ステップ 6）で
