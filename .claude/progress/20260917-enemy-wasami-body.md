---
title: 敵ワサミの素体（作業一覧の項目 4）
status: 進行中
branch: feature/enemy-wasami-body
base: b25dd05
started: 2026-09-17 23:14
updated: 2026-09-18 01:20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 敵ワサミの素体（作業一覧の項目 4）

## 依頼

`.claude/roadmap.md` の項目 4（ユーザーの最終目標 2026-09-17「敵は `tmp\enemy_wasami.glb`。多数のモーションを含んでいるため、適切な用途に使う。スタンオーブ使用時: “気絶”モーションを使用する」）。2026-09-18 のユーザーの指示「`enemy_wasami_v3`・`wasami_mochi_v3`・`boss_wasami` をそれぞれ使用してください」「`enemy_wasami_v3` は追いかける際に使える様々なモーションを含んでいます（例えば、片手をついて障害物を飛び越える動作など）。Zone 1・2 でプレイヤーを追いかける際、たまにランダムで流すように」。**この項目で使うのは `tmp/enemy_wasami_v3.glb`**（と、捕獲の 3 本だけ旧 `tmp/enemy_wasami.glb`）。

- 目標: 敵ワサミをスケルタルメッシュとアニメとして取り込み、敵のアクタ `AWasamiEnemy` の土台（モーションの再生、パワーからの受け口）を作る。**AI はまだ入れない**（項目 7。追跡中のランダムの動きも項目 7）。赤い縁取りは作らない。
- 完了の条件: 原本を `SourceArt/Wasami/`（Git LFS）に置き、取り込み（`wasami_tools` のツールセット）が `/Game/Wasami/Enemy` にメッシュ・スケルトン・物理アセットと、v3 のアニメ 16 本 + 捕獲の 3 本を作り、一覧 `.claude/references/enemy-wasami-motions.md` の役（`Idle`・`Idle_Alert`・`Walk`・`Run`・`Run_Nightmare`・`Stun_Loop`・`Stun_Recover`・`Capture_1`〜`3`・追跡中の変化 6 本）で引ける。`AWasamiEnemy` が `IWasamiEnemyInterface`（`SetState(Stun)`・`PlayerVanish`・`NoTelepathy`）を実装し、気絶で `Stun_Loop` をループして明けに `Stun_Recover` で起き上がる。Primal Fear・Telepathy・Vanish の仮の的 `AWasamiTestEnemy` の代わりに PIE で使える。
- ユーザーの決定: 役とアニメの対応は一覧のとおり（巡回 `Walking`、追跡 `Running`、全回収後の追跡 `run_fast_2`、気絶は無名のモーション、捕獲は旧 glb の `Backflip`・`sliding_rool`・`Stylish_Walk`〈2026-09-18 の回答「旧 glb の 3 本を流用」〉。待機 `Idle_11`・見張り `Idle_5` は Claude の仮）。敵の AI は追跡型だけ（透明化・薬投げ・ガスは作らない）。原作のキャラクターのモデルは使わない。

## 計画

大規模改修（C++・取り込みにまたがり、複数コミット）なので作業ブランチ `feature/enemy-wasami-body` で進める（`.claude/guides/git-workflow.md`）。

- [x] 0. 計画 … 2026-09-17 完了（2026-09-18 に v3 に合わせて直した）。本家のナース（`BP_06_ReaperNurse`・ABP `nurse_idle1_Skeleton_AnimBlueprint`）の値は下の決定事項、glb の中身は一覧。
- [x] 1. 原本と取り込み … 2026-09-18 完了。`SourceArt/Wasami/enemy_wasami_v3.glb`・`enemy_wasami_capture.glb`（LFS）、`pipeline/gltf.py`・`dd_enemy.py`、`WasamiDDTools.import_wasami_enemy` で `/Game/Wasami/Enemy` に `SK_WasamiEnemy` とアニメ 19 本（役の名前・長さ・作り方は実装記録 07 の表）。
- [ ] 2. アニメの再生（C++）— ネイティブの `UWasamiEnemyAnimInstance`（独自の `FAnimInstanceProxy` の `Evaluate` でシーケンスを標本化して混ぜる。`UAnimSingleNodeInstance` と同じ形）。役の名前でシーケンスを持つ（取り込みが入れる）。本家の ABP の形を写す: 根は `bStunned`（持ち主の State == Stun）で 0.25 s のブレンド、移動は待機（速さ ≤ 5）↔ 移動（速さ > 5 へ 0.5 s、止まるとき 0.25 s）、移動の中は速さ > 400 で走り（ブレンド 0.25 s）。待機は `Idle`（見張りの `bAggressiveIdle` なら `Idle_Alert`）、歩き `Walk`、走り `Run`、Nightmare の走り `Run_Nightmare`（持ち主のフラグで選ぶ）。再生の速さは移動の速さに合わせる（足の滑りを抑える。本家はスケートなので速さ 1。見た目の仮の値）。気絶は `Stun_Loop` のループ → 明けに `Stun_Recover`。**1 回再生の口**を作る: 捕獲（項目 9）と追跡中の変化（項目 7。移動は止めず、再生の速さを指定でき、途中で止められ、終わりに走りへ混ぜて戻る）。区間の時刻・重み・遷移はテスト（`Wasami.Enemy.*`）にする。ビルドは `Tools/editor_cycle.py`。
  - アセットは `/Game/Wasami/Enemy/A_WasamiEnemy_<役>`（`dd_enemy.ANIM_PREFIX`）。C++ はソフト参照で持つ（`ConstructorHelpers` で読まない。`.claude/guides/unreal-workflow.md`）。役の名前から引くか、取り込みが CDO に入れるかはこのステップで決める。
  - 変更予定: `Source/wasami_deception/WasamiEnemyAnimInstance.*`、`Tests/WasamiEnemyTests.cpp`、実装記録 07
- [ ] 3. 敵のアクタ（C++）— `AWasamiEnemy : ACharacter, IWasamiEnemyInterface`。本家の値（下の決定事項）で部品を作り、気絶の流れ（`SetState` は State を入れるだけ → 0.5 s ごとの判断で State == Stun なら 1 回だけ: 移動を即止め → 17.0 s → Patrol）、`GetState`・`PlayerVanish`・`NoTelepathy` を本家どおりに（`CustomDepth(Duration)` は作らない）。気絶の音（`Nurse_Hospital_Zone01_Stunned`）と `Cloak(False)` は作らない（音は項目 20 でワサミの声、透明化は作らない）。気絶が来たら 1 回再生（追跡中の変化）を止める。Python から置ける `SpawnEnemy`。テスト（気絶の 17 s と Patrol への戻り、判断の 0.5 s の遅れ、2 回目の気絶、タグとインターフェース、Primal/Vanish/Telepathy から届くこと）。仮の的のテストはそのまま残す。
  - 変更予定: `Source/wasami_deception/WasamiEnemy.*`、`Tests/WasamiEnemyTests.cpp`、実装記録 07・04
- [ ] 4. PIE での確かめ — `L_Hospital_Zone1` に `AWasamiEnemy` を置き、待機・見張りの待機・歩き・走り・Nightmare の走り・気絶（Primal Fear で）→ 起き上がり → 17 s で Patrol、Telepathy の印、Vanish の通知、捕獲の 3 本と追跡中の変化 6 本（1 回再生の口をデバッグで呼ぶ）を収録と画面で確かめる（`Tools/pie.py`・`Tools/video_probe.py`。`observations/ours/`）。足の滑り（再生の速さと移動の速さ）と、体の大きさ（本家のカプセルの高さ約 236 cm に対するワサミの 170 cm）を測る。`Idle_11`・`Idle_5` の割り当てと `Vault_and_Land`（始めが高い）の扱いを画面を見て決め、一覧を直す。
  - 変更予定: `observations/`、実装記録 07（結果）、`.claude/references/enemy-wasami-motions.md`
- [ ] 5. 仕上げ — 作業一覧の項目 4 を完了にし、handover の「現状と次の一歩」、note の原稿（`docs/note/progress.md`）、要確認を作業一覧の「未回答の要確認」へ移し、記録を消して main へマージ・push（`git checkout main` → `git merge --no-ff`）。

## 次にやること

ステップ 2: アニメの再生 `UWasamiEnemyAnimInstance`（計画の 2。決定事項の「本家のナース」の ABP の値と、「気絶の起き上がり」「再生の速さの物差し」を使う）。先にエンジンの `UAnimSingleNodeInstance`・`FAnimInstanceProxy` の形を読んで、区間の時刻と混ぜ方を純粋な関数に分けてテストにする。

## 決定事項

- 2026-09-18（ユーザーの指示と回答）: モデルは `enemy_wasami_v3`（2026-09-17 の一覧の「作り直し」は v3 で置き換わった。v3 の中身と役の対応は一覧）。捕獲の 3 本は v3 に無いので旧 glb の 3 本を流用。追跡中のランダムの動き（約 8 s に 1 回、速さは 800 cm/s のまま、前方が空いているときだけ）は項目 7 で作り、この項目では 1 回再生の口とその場の形の前処理まで作る。入口レベルは作らない（作業一覧の「決めたこと」）。
- 2026-09-17（ユーザーの回答「不要」）: 赤い縁取りは作らない。Chameleon の `Custom Depth Highlighter (Clip)` も敵の `CustomDepth(Duration)` も作らない（最新版ではナースの `Custom Depth(Duration)` を呼ぶ者がいない。`.claude/references/powers/04-primal-telepathy.md` 3.10 節）。
- 2026-09-17: アニメはネイティブの `UAnimInstance`（独自の Proxy）で再生する — MCP の `BlueprintTools` にアニメグラフのノードを置く道具が見当たらず（`create_node`・`write_graph_dsl` はあるがアニメグラフに効くか未確認）、本作は手作りの BP を持たない（C++ と取り込みのスクリプトで作り直せる形）。Proxy で行き詰まったら `BlueprintTools.create_node` でアニメ BP を試す。
- 2026-09-18（ステップ 1 の結果。理由は実装記録 07）: 前へ進むアニメはその場の形にし、捕獲の 3 本は進んだまま（`Capture_2` 5.7 m・`_3` 2.1 m。項目 9 で扱う）。気絶の起き上がりの終わりの骨盤のずれは取り込みで待機の位置へ寄せたので、ステップ 2 では扱わない。メッシュの正面は UE の +Y（アクタでメッシュを Yaw −90。ステップ 3）。
- 2026-09-18: 再生の速さの物差し（接地した足の後ろへの速さ）は `Walk` 1.33 m/s・`Run` 4.5 m/s・`Run_Nightmare` 4.6 m/s（骨盤の前進から）。本家の 350・800 cm/s に合わせると `Walk` 2.6 倍・`Run` 1.8 倍になる。上限を設けるか（見た目の仮の値）はステップ 2 で決め、ステップ 4 で見る。
- 2026-09-17: 本家のナース（`pak_reference_2/_bytecode/…/Characters/Nurse/BP_06_ReaperNurse.txt` と `.json`、ABP `Animation/Enemies/Nurse/Reaper/nurse_idle1_Skeleton_AnimBlueprint`）の値 — ステップ 2〜3 で写す:
  - 親 `BP_DD_Character_Base` → `Character`。CDO: `Tags=["Enemy"]`、`AutoPossessAI=PlacedInWorldOrSpawned`、`bUseControllerRotationYaw=false`、`Normal Speed=350`・`Skate Speed=800`（`Set Walk State` が `bNormalWalk` で MaxWalkSpeed を切り替える。真なら 350）。移動: `MaxWalkSpeed=800`、`RotationRate=(0,300,0)`、`bUseControllerDesiredRotation=true`、`bOrientRotationToMovement=true`。見張り `BP_06_ReaperNurse_Sentry` は CDO で `bAggressiveIdle=true`。
  - カプセル: 半分の高さ 118.05822、半径は上書きなし（エンジンの既定 34 と推測。ステップ 3 で UE 4.24 の `ACharacter` の既定を確かめる）、`AreaClass=NavArea_Obstacle`（基底）。メッシュ: 位置 (−0.00006, −0.0002, −117.84394)、Yaw −90、拡縮 1、`bRenderCustomDepth` は既定（偽）。
  - ほかの部品: 捕獲の判定 `Sphere`（半径 54.928、Pawn だけ Overlap、`NavArea_Obstacle`。項目 9）、`Camera`（カプセルの子 (40,0,100) Yaw 180。本家の病院の捕獲用。本作はホテルの体なので使わない）、`PillSpawn`・`Skate Audio`・`Cloak Timeline`（作らない）、`Talk Audio`（項目 20）、上空の板 `StaticMesh`（`M_Enemy`、(0, 21.9, 1117.8)、拡縮 (2.52, 2.52, 10)。地図の印と推測。項目 10 の敵の表示で確かめる）。捕獲の判定は State ≠ Stun のときだけ（@9892）。
  - 気絶: `Set State` は `State = 引数` だけ（`byOrb` は使わない。Primal もオーブも同じ）。0.5 s ごとのループのタイマー `Make Choice` が State == 2 を見て DoOnce → `StopMovementImmediately` → `Cloak(False)` → 台詞 → `Delay 17.0` → `State = 0`（Patrol）で DoOnce を戻す。粒子・モンタージュは無く、見た目は ABP の `bStunned`（`nurse_stunned` のループ、2.633 s）だけ。起き上がりのアニメは無い（0.25 s のブレンドで戻る）。
  - `Get State` はナースの実装が常に 0（Patrol）を返す（変数 `State` を返さない）。`PlayerVanish` は `Seen Player Recently = False` だけ。`NoTelepathy` は偽。`Custom Depth(Duration)`（縁取り）は作らない。
  - ABP: 根は BlendListByBool(`bStunned`, 0.25 s)。偽の側は Slot `Fullbody` ← 上半身の Slot `DefaultSlot` ← ステートマシン: Idle（`nurse_idle_01`。`bAgressiveIdle` なら Alert の 2 本）→ Skating（速さ > 5、0.5 s、Sinusoidal。中は速さ > 400 で `nurse_skate_run`、それ以外 `nurse_skate_normal`、0.25 s）→ Stop（速さ < 5、0.25 s、ExpOut。`nurse_skate_stop` を 0.35 から 1 回）→ Idle（残り 10 % 未満、0.5 s、Cubic）。
  - パワーの探し方: Primal = 球の重なり（インターフェースまたはタグ）、オーブ・Vanish = タグ `Enemy` の全員、Telepathy = インターフェースの全員。**敵はタグ `Enemy` とインターフェースの両方が要る**。基底の BeginPlay は `CanSpawn` が偽なら自分を破棄し、真なら `Ignore All Speed Barriers`（項目 8）。
- 2026-09-17: 気絶の起き上がりは 17 s の中に収める — 本家は 17 s で Patrol に戻り、すぐ判断を再開する（規則の値）。ワサミの起き上がりを明けの後に足すと敵の動きが遅れるので、前屈のループを 17 s − 起き上がりの長さまで続け、17 s ちょうどに起き上がり終わって Patrol に戻す（ユーザーの「明けに起き上がる」の読み。見た目の仮の作り）。
  - 2026-09-18 に取り込んだ長さ: `Stun_Loop` 1.500 s（最後のキー = 最初のキー）、`Stun_Recover` 7.567 s（**最初のキーが `Stun_Loop` の最初のキーと同じ**）。なので起き上がりはループの境目（ループの時刻 0）で切り替えれば継ぎ目が無い。気絶の始まりでループの位相をずらし（17 − 7.567 = 9.433 s が 1.5 s の倍数の境目に来るよう、始めの時刻を 9.433 mod 1.5 = 0.433 s にする）、入りは走りからの 0.25 s のブレンドで隠す。

## 要確認（ユーザー）

（なし。2026-09-18 に v3 の使い方〈捕獲の 3 本・追跡中の動きの速さと頻度・場面の代用〉の回答を得た）

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いていて、PIE なし・未保存なし（2026-09-18 01:20 時点）。MCP はつながる。
- 原本は `tmp/` にもある（git の外）: `enemy_wasami_v3.glb`（`SourceArt/` に写した）、旧 `enemy_wasami.glb`（`enemy_wasami_capture.glb` の元）、`boss_wasami.glb`（項目 11）、`wasami_mochi_v3.glb`（項目 22）。消さない。
- 取り込みをやり直すときは MCP の `WasamiDDTools.import_wasami_enemy`（置き換えなので何度でもよい。数秒）。ツールセットのクラスを変えたら先に `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`。
- アニメ BP の道具の有無は MCP の `describe_toolset`（`editor_toolset.toolsets.blueprint.BlueprintTools`）で見た。出力が 68 KB あるので、読むなら python で名前だけ抜く。

## 検証

- ステップ 1（2026-09-18）: 取り込みはエラーなし。エディタで、アセット 26、骨 28・根 `pelvis`・高さ 170 cm・正面 +Y、ループ 5 本と `Stun_Loop` の最初と最後の差 0.0 cm、その場の形の 7 本は骨盤の水平の動き 0、`Stun_Recover` の最初 = `Stun_Loop` の最初・終わりの骨盤 = `Idle` の骨盤、スロットに `MI_WasamiEnemy`、未保存なしを確かめた。捕獲の載せ替えは、同じ対応を旧と v3 の `Running`・`Walking`・`BeHit_FlyUp` に当てて 0.08° 以内で一致（見た目はステップ 4 の PIE で見る）。`check_records.py` OK。
