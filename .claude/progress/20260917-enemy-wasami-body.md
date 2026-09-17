---
title: 敵ワサミの素体（作業一覧の項目 4）
status: 進行中
branch: feature/enemy-wasami-body
base: b25dd05
started: 2026-09-17 23:14
updated: 2026-09-18 00:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 敵ワサミの素体（作業一覧の項目 4）

## 依頼

`.claude/roadmap.md` の項目 4（ユーザーの最終目標 2026-09-17「敵は `tmp\enemy_wasami.glb`。多数のモーションを含んでいるため、適切な用途に使う。スタンオーブ使用時: “気絶”モーションを使用する」）。2026-09-18 にユーザーが「通常の敵ワサミは新しく作り直します。必要なモーションを一式」と言い、Claude が名前つきの一覧 `.claude/references/enemy-wasami-motions.md` を示した（ユーザーがその一覧で作る）。**今の `tmp/enemy_wasami.glb` は届くまでの仮**。

- 目標: `enemy_wasami.glb` をスケルタルメッシュとアニメとして取り込み、敵のアクタ `AWasamiEnemy` の土台（モーションの再生、パワーからの受け口）を作る。**AI はまだ入れない**（項目 7）。赤い縁取りは作らない（2026-09-17 のユーザーの回答。決定事項）。
- 完了の条件: 取り込み（`wasami_tools` のツールセット）が `/Game/Wasami/Enemy` にメッシュ・スケルトン・アニメを作り、アニメは一覧の名前（`Idle`・`Walk`・`Run`・`Stun_Loop` など）で並ぶ（仮の glb は「仮の対応」で読み替える）。新しいモデルが届いていれば、それを `SourceArt/Wasami/enemy_wasami.glb`（Git LFS）に置いて使う。`AWasamiEnemy` が `IWasamiEnemyInterface`（`SetState(Stun)`・`PlayerVanish`・`NoTelepathy`）を実装し、気絶で `Stun_Loop` をループして明けに `Stun_Recover` で起き上がる。Primal Fear・Telepathy・Vanish の仮の的 `AWasamiTestEnemy` の代わりに PIE で使える。
- ユーザーの決定（作業一覧の「決めたこと」）: 用途と名前は一覧 `.claude/references/enemy-wasami-motions.md` の A・B（巡回 `Walk`、追跡 `Run`、全回収後の追跡 `Run_Nightmare`、気絶 `Stun_Loop` → 明けに `Stun_Recover`、捕獲 `Capture_1`〜`3`、待機 `Idle`、見張り `Idle_Alert`）。仮の glb では 2026-09-17 に承認した割り当て（`Walking`・`Running`・`run_fast_2`・無名のモーション〈10.0 s。前半が前屈、6〜8 s で起き上がる〉・`Backflip`・`sliding_rool`・`Stylish_Walk`）を一覧の「仮の対応」で読み替える。敵の AI は追跡型だけ（透明化・薬投げ・ガスは作らない）。原作のキャラクターのモデルは使わない。

## 計画

大規模改修（C++・取り込みにまたがり、複数コミット）なので作業ブランチ `feature/enemy-wasami-body` で進める（`.claude/guides/git-workflow.md`）。

- [x] 0. 計画 … 2026-09-17 完了。glb の中身と本家のナース（`BP_06_ReaperNurse`・ABP `nurse_idle1_Skeleton_AnimBlueprint`）を調べ、この記録を作った。作業一覧の気絶の秒数を 17 s に直した。
- [ ] 1. 原本と取り込み — 取り込みのモジュール `Content/Python/wasami_tools/pipeline/dd_enemy.py` を作る。原本は `SourceArt/Wasami/enemy_wasami.glb`（新しいモデル。届いたら置く）を先に探し、無ければ仮の `tmp/enemy_wasami.glb`（git の外。LFS には入れない）を読む。glb を読み、`Intermediate/Pipeline/wasami/enemy/` に前処理した写し（アニメの名前を一覧の名前にそろえる〈仮の glb は「仮の対応」の表で読み替え、無名のモーションは前半 `Stun_Loop`・後半 `Stun_Recover` に切り分ける〉、ループの前進〈仮の `run_fast_2`〉を消してその場の形にする、ルートの骨があればそれを根にする）とテクスチャ 3 枚を書き出し、glTF の PBR の推定のマスター（餅の `M_DD_WasamiMochi` から明滅を除いた形）とインスタンス、Interchange のスケルタルの取り込み（本作のパイプライン `/Game/Pipeline/Interchange/PL_Wasami_Skeletal`）でメッシュ・スケルトン・物理アセット・アニメを `/Game/Wasami/Enemy` に作る。ツールセットに `WasamiDDTools.import_wasami_enemy()` を足す。確かめ: 骨の数とルートの有無、身長、正面の向き、アニメの名前と長さが原本と一致、ループの前進が消えている。実装記録 07（新規）と 01・索引を書く。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_enemy.py`・`paths.py`・`toolsets/dd.py`、`/Game/Wasami/Enemy/*`、`/Game/Pipeline/Interchange/PL_Wasami_Skeletal`、`/Game/Pipeline/Materials/M_DD_WasamiGltf`、`.claude/implementation-records/07-enemies.md`・`01-stage-pipeline.md`・`_index.md`
- [ ] 2. アニメの再生（C++）— ネイティブの `UWasamiEnemyAnimInstance`（独自の `FAnimInstanceProxy` の `Evaluate` でシーケンスを標本化して混ぜる。`UAnimSingleNodeInstance` と同じ形）。本家の ABP の形を写す: 根は `bStunned`（持ち主の State == Stun）で 0.25 s のブレンド、移動は待機（速さ ≤ 5）↔ 移動（速さ > 5 へ 0.5 s、止まるとき 0.25 s）、移動の中は速さ > 400 で走り（ブレンド 0.25 s）。本作の割り当て（一覧の A）: 待機 `Idle`（見張りの `bAggressiveIdle` なら `Idle_Alert`）・歩き `Walk`・走り `Run`・Nightmare の走り `Run_Nightmare`（持ち主のフラグで選ぶ）、任意の `Stop`・`Stun_Start` は有れば使う。無いアニメ（仮の glb の `Idle`・`Idle_Alert`）は `Stun_Recover` の終わりの姿勢で止めて代わりにする。再生の速さは移動の速さに合わせる（足の滑りを抑える。本家はスケートなので速さ 1。見た目の仮の値）。気絶は `Stun_Loop` のループ → 明けに `Stun_Recover`。捕獲の 1 回再生の口も作る（項目 9 で使う）。区間の時刻・重み・遷移はテスト（`Wasami.Enemy.*`）にする。ビルドは `Tools/editor_cycle.py`。
  - 変更予定: `Source/wasami_deception/WasamiEnemyAnimInstance.*`、`Tests/WasamiEnemyTests.cpp`、実装記録 07
- [ ] 3. 敵のアクタ（C++）— `AWasamiEnemy : ACharacter, IWasamiEnemyInterface`。本家の値（下の決定事項）で部品を作り、気絶の流れ（`SetState` は State を入れるだけ → 0.5 s ごとの判断で State == Stun なら 1 回だけ: 移動を即止め → 17.0 s → Patrol）、`GetState`・`PlayerVanish`・`NoTelepathy` を本家どおりに（`CustomDepth(Duration)` は作らない）。気絶の音（`Nurse_Hospital_Zone01_Stunned`）と `Cloak(False)` は作らない（音は項目 20 でワサミの声、透明化は作らない）。Python から置ける `SpawnEnemy`。テスト（気絶の 17 s と Patrol への戻り、判断の 0.5 s の遅れ、2 回目の気絶、タグとインターフェース、Primal/Vanish/Telepathy から届くこと）。仮の的のテストはそのまま残す。
  - 変更予定: `Source/wasami_deception/WasamiEnemy.*`、`Tests/WasamiEnemyTests.cpp`、実装記録 07・04
- [ ] 4. PIE での確かめ — `L_Hospital_Zone1` に `AWasamiEnemy` を置き、待機・歩き・走り・Nightmare の走り・気絶（Primal Fear で）→ 起き上がり → 17 s で Patrol、Telepathy の印、Vanish の通知を収録と画面で確かめる（`Tools/pie.py`・`Tools/video_probe.py`。`observations/ours/`）。足の滑り（再生の速さと移動の速さ）と、体の大きさ（カプセルに対するワサミの背丈）を測る。
  - 変更予定: `observations/`、実装記録 07（結果）
- [ ] 5. 仕上げ — 作業一覧の項目 4 を完了にし、handover の「現状と次の一歩」、note の原稿（`docs/note/progress.md`）、要確認を作業一覧の「未回答の要確認」へ移し、記録を消して main へマージ・push（main の worktree は 2026-09-17 に消したので、普通の `git checkout main` → `git merge --no-ff` でよい）。

## 次にやること

ステップ 1: `feature/enemy-wasami-body` で、`SourceArt/Wasami/enemy_wasami.glb`（新しいモデル）が届いているかを見て、無ければ仮の `tmp/enemy_wasami.glb` で `dd_enemy.py` を書く（名前は `.claude/references/enemy-wasami-motions.md`）（glb の読み方は `dd_shards._glb`、取り込みは `dd_stage.import_mesh` の Interchange の呼び方を下敷きに、スケルタルとアニメを有効にしたパイプラインを作る）。

## 決定事項

- 2026-09-17（ユーザーの回答「よい」）: モーションの割り当て（巡回 `Walking`・追跡 `Running`・全回収後の追跡 `run_fast_2`・捕獲 `Backflip`・`sliding_rool`・`Stylish_Walk`）で確定。
- 2026-09-17（ユーザーの回答「不要」）: 赤い縁取りは作らない。Chameleon の `Custom Depth Highlighter (Clip)` も敵の `CustomDepth(Duration)` も作らず、計画から縁取りのステップを外した（旧ステップ 5・6 を 4・5 に繰り上げ）。最新版ではナースの `Custom Depth(Duration)` を呼ぶ者がいない（`.claude/references/powers/04-primal-telepathy.md` 3.10 節）。
- 2026-09-18（ユーザー「通常の敵ワサミは新しく作り直します。必要なモーションを一式」）: モデルとモーションはユーザーが作り直す。Claude が名前つきの一覧 `.claude/references/enemy-wasami-motions.md`（A 基本 7 本 + 任意 2、B 捕獲 3、C 場面 17。ルートの骨・その場のループ・名前の決まりつき）を示した。**名前を契約にする**: 取り込みと `UWasamiEnemyAnimInstance` は一覧の名前でアニメを選ぶ。新しいモデルが届くまでは仮の glb で仕組みを先に作る（仮の glb は 20 MB で、差し替わるので LFS に入れない）。届いたら `SourceArt/Wasami/enemy_wasami.glb` に置いて取り込み直す（身長やカプセルに対する大きさも測り直す）。`restpose` は腕を広げた基準姿勢なので待機に使わない。
- 2026-09-17: アニメはネイティブの `UAnimInstance`（独自の Proxy）で再生する — MCP の `BlueprintTools` にアニメグラフのノードを置く道具が見当たらず（`create_node`・`write_graph_dsl` はあるがアニメグラフに効くか未確認）、本作は手作りの BP を持たない（C++ と取り込みのスクリプトで作り直せる形）。Proxy で行き詰まったら `BlueprintTools.create_node` でアニメ BP を試す。
- 2026-09-17: `run_fast_2` の骨盤の前進は前処理で消す — glb の骨盤（根の骨）が 0.625 s で glTF の +Z（正面）へ 2.69 m 進む（`Walking`・`Running` はその場）。ルートの骨が無いので UE のルートモーションの切り方は使えない。`Stylish_Walk`（2.2 m）・`sliding_rool`（6.0 m）は捕獲の別室で使うので、そのまま（項目 9 で扱う）。
- 2026-09-17: glb の中身（`tmp/enemy_wasami.glb`、Blender 4.5）: 骨 22（`pelvis` が根。UE のマネキン系の名前。シーンの根 `target_character` の子に骨盤とメッシュ）、スキンのメッシュ 1（9,273 頂点・7,157 三角形、材質 1 `BakedMaterial`、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²）、高さ 1.70 m、正面は glTF の +Z、アニメ 11 本の長さ（s）: Running 0.667・Walking 1.042・無名 10.0・Backflip 2.167・BeHit_FlyUp 1.583・Shot_and_Fall_Forward 2.25・Stand_Up6 6.708・Stylish_Walk 3.583・run_fast_2 0.625・sliding_rool 2.792・restpose 0.083。無名のモーションの骨盤は終わりで (−0.185, +0.256) m ずれる（起き上がりの後に体が約 30 cm 跳ぶので、ステップ 2 で扱う）。待機のモーションは無い。
- 2026-09-17: 本家のナース（`pak_reference_2/_bytecode/…/Characters/Nurse/BP_06_ReaperNurse.txt` と `.json`、ABP `Animation/Enemies/Nurse/Reaper/nurse_idle1_Skeleton_AnimBlueprint`）の値 — ステップ 2〜3 で写す:
  - 親 `BP_DD_Character_Base` → `Character`。CDO: `Tags=["Enemy"]`、`AutoPossessAI=PlacedInWorldOrSpawned`、`bUseControllerRotationYaw=false`、`Normal Speed=350`・`Skate Speed=800`（`Set Walk State` が MaxWalkSpeed を切り替える）。移動: `MaxWalkSpeed=800`、`RotationRate=(0,300,0)`、`bUseControllerDesiredRotation=true`、`bOrientRotationToMovement=true`。
  - カプセル: 半分の高さ 118.05822、半径は上書きなし（エンジンの既定 34 と推測。ステップ 3 で UE 4.24 の `ACharacter` の既定を確かめる）、`AreaClass=NavArea_Obstacle`（基底）。メッシュ: 位置 (−0.00006, −0.0002, −117.84394)、Yaw −90、拡縮 1、`bRenderCustomDepth` は既定（偽）。
  - ほかの部品: 捕獲の判定 `Sphere`（半径 54.928、Pawn だけ Overlap、`NavArea_Obstacle`。項目 9）、`Camera`（カプセルの子 (40,0,100) Yaw 180。本家の病院の捕獲用。本作はホテルの体なので使わない）、`PillSpawn`・`Skate Audio`・`Cloak Timeline`（作らない）、`Talk Audio`（項目 20）、上空の板 `StaticMesh`（`M_Enemy`、(0, 21.9, 1117.8)、拡縮 (2.52, 2.52, 10)。地図の印と推測。項目 10 の敵の表示で確かめる）。捕獲の判定は State ≠ Stun のときだけ（@9892）。
  - 気絶: `Set State` は `State = 引数` だけ（`byOrb` は使わない。Primal もオーブも同じ）。0.5 s ごとのループのタイマー `Make Choice` が State == 2 を見て DoOnce → `StopMovementImmediately` → `Cloak(False)` → 台詞 → `Delay 17.0` → `State = 0`（Patrol）で DoOnce を戻す。粒子・モンタージュは無く、見た目は ABP の `bStunned`（`nurse_stunned` のループ、2.633 s）だけ。起き上がりのアニメは無い（0.25 s のブレンドで戻る）。
  - `Get State` はナースの実装が常に 0（Patrol）を返す（変数 `State` を返さない）。`PlayerVanish` は `Seen Player Recently = False` だけ。`NoTelepathy` は偽。`Custom Depth(Duration)`（縁取り）は作らない。
  - ABP: 根は BlendListByBool(`bStunned`, 0.25 s)。偽の側は Slot `Fullbody` ← 上半身の Slot `DefaultSlot` ← ステートマシン: Idle（`nurse_idle_01`。`bAgressiveIdle` なら Alert の 2 本）→ Skating（速さ > 5、0.5 s、Sinusoidal。中は速さ > 400 で `nurse_skate_run`、それ以外 `nurse_skate_normal`、0.25 s）→ Stop（速さ < 5、0.25 s、ExpOut。`nurse_skate_stop` を 0.35 から 1 回）→ Idle（残り 10 % 未満、0.5 s、Cubic）。
  - パワーの探し方: Primal = 球の重なり（インターフェースまたはタグ）、オーブ・Vanish = タグ `Enemy` の全員、Telepathy = インターフェースの全員。**敵はタグ `Enemy` とインターフェースの両方が要る**。基底の BeginPlay は `CanSpawn` が偽なら自分を破棄し、真なら `Ignore All Speed Barriers`（項目 8）。
- 2026-09-17: 気絶の起き上がりは 17 s の中に収める — 本家は 17 s で Patrol に戻り、すぐ判断を再開する（規則の値）。ワサミの起き上がり（約 5 s）を明けの後に足すと敵の動きが遅れるので、前屈のループを 17 s − 起き上がりの長さまで続け、17 s ちょうどに起き上がり終わって Patrol に戻す（ユーザーの「明けに起き上がる」の読み。見た目の仮の作り）。

## 要確認（ユーザー）

（なし。2026-09-17 の 3 件は回答を決定事項へ移した）

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いていて、PIE なし・未保存なし（2026-09-17 23:10 時点）。MCP はつながる。
- 仮の原本は `tmp/enemy_wasami.glb`（git の外、2026-09-17 の時点で 11 本）で、ほかの場所には無い。新しいモデル（ユーザーが作り直す）の置き場所はまだ聞いていない。`SourceArt/Wasami/` か `tmp/` に新しい glb が現れたら、中身（骨・名前・長さ）を一覧と照らしてから使う。
- アニメ BP の道具の有無は MCP の `describe_toolset`（`editor_toolset.toolsets.blueprint.BlueprintTools`）で見た。出力が 68 KB あるので、読むなら python で名前だけ抜く。

## 検証

- 計画のみ（ソースの変更なし。作業一覧の気絶の秒数を 17 s に直した）。2026-09-17 23:39 に要確認の回答を反映した（ソースはコメントだけ）。
