---
title: 敵ワサミの素体（作業一覧の項目 4）
status: 進行中
branch: feature/enemy-wasami-body
base: b25dd05
started: 2026-09-17 23:14
updated: 2026-09-18 02:20
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

- [x] 0. 計画 … 2026-09-17 完了（2026-09-18 に v3 に合わせて直した）。本家のナース（`BP_06_ReaperNurse`・ABP `nurse_idle1_Skeleton_AnimBlueprint`）の値は実装記録 07、glb の中身は一覧。
- [x] 1. 原本と取り込み … 2026-09-18 完了。`SourceArt/Wasami/enemy_wasami_v3.glb`・`enemy_wasami_capture.glb`（LFS）、`pipeline/gltf.py`・`dd_enemy.py`、`WasamiDDTools.import_wasami_enemy` で `/Game/Wasami/Enemy` に `SK_WasamiEnemy` とアニメ 19 本（役の名前・長さ・作り方は実装記録 07 の表）。
- [x] 2. アニメの再生 … 2026-09-18 完了。`UWasamiEnemyAnimInstance`（純粋な状態 `FWasamiEnemyAnimState` + Proxy。本家の ABP の木、気絶の起き上がりを 17 s の終わりに合わせる位相、全身の 1 回再生 `PlayOnce`/`StopOnce`/`IsPlayingOnce`）とテスト `Wasami.Enemy.Anim.*` 5 本。中身は実装記録 07 の「アニメの再生」。
- [x] 3. 敵のアクタ … 2026-09-18 完了。`AWasamiEnemy`（本家のナースの CDO・部品・`CanSpawn`・0.5 s ごとの判断と 17 s の気絶・インターフェース・`SpawnEnemy`）、アニメの再生が持ち主の敵から気絶と残りを毎フレーム読む形に、テスト `Wasami.Enemy.Actor.*` 3 本。中身は実装記録 07 の「敵のアクタ」。
- [ ] 4. PIE での確かめ — `L_Hospital_Zone1` の PIE に `AWasamiEnemy` を出し（`unreal.WasamiEnemy.spawn_enemy(PIE のワールド, カプセルの中心, yaw, sentry)`。中心は床 + 118.06）、待機・見張りの待機・歩き・走り・Nightmare の走り・気絶（Primal Fear で）→ 起き上がり → 17 s で Patrol、Telepathy の印、Vanish の通知、捕獲の 3 本と追跡中の変化 6 本（1 回再生の口をデバッグで呼ぶ）を収録と画面で確かめる（`Tools/pie.py`・`Tools/video_probe.py`。`observations/ours/`）。足の滑り（再生の速さと移動の速さ）と、体の大きさ（本家のカプセルの高さ約 236 cm に対するワサミの 170 cm。Telepathy の印はカプセルの中心＝胸の高さに付く）を測る。`Idle_11`・`Idle_5` の割り当てと `Vault_and_Land`（始めが高い）の扱いを画面を見て決め、一覧を直す。**起き上がりの終わり**（`Stun_Recover` の 7.567 s）は待機より頭が約 10 cm 低く 15 cm 後ろ（うなだれた姿勢。クリップ自体がそう終わる）で、明けの 0.25 s で待機へ戻るのが目立つかを見る（目立つなら明けのブレンドを延ばすか、取り込みで終わりを待機へ寄せる）。**再生の速さの上限**（巡回 350 cm/s の歩きが上限 2 で足が滑る）も見る。
  - 変更予定: `observations/`、実装記録 07（結果）、`.claude/references/enemy-wasami-motions.md`
- [ ] 5. 仕上げ — 作業一覧の項目 4 を完了にし、handover の「現状と次の一歩」、note の原稿（`docs/note/progress.md`）、要確認を作業一覧の「未回答の要確認」へ移し、記録を消して main へマージ・push（`git checkout main` → `git merge --no-ff`）。

## 次にやること

ステップ 4: PIE での確かめ（計画の 4）。先に `Tools/pie.py` と `.claude/guides/observation.md` の PIE の節を読み、撮るものの一覧を作ってから始める。敵を動かすのは AI が無いので、Python から `add_movement_input` か速さの直書きで（どちらが効くかは未確認）。

## 決定事項

- 2026-09-18（ユーザーの指示と回答）: モデルは `enemy_wasami_v3`（2026-09-17 の一覧の「作り直し」は v3 で置き換わった。v3 の中身と役の対応は一覧）。捕獲の 3 本は v3 に無いので旧 glb の 3 本を流用。追跡中のランダムの動き（約 8 s に 1 回、速さは 800 cm/s のまま、前方が空いているときだけ）は項目 7 で作り、この項目では 1 回再生の口とその場の形の前処理まで作る。入口レベルは作らない（作業一覧の「決めたこと」）。
- 2026-09-17（ユーザーの回答「不要」）: 赤い縁取りは作らない。Chameleon の `Custom Depth Highlighter (Clip)` も敵の `CustomDepth(Duration)` も作らない（最新版ではナースの `Custom Depth(Duration)` を呼ぶ者がいない。`.claude/references/powers/04-primal-telepathy.md` 3.10 節）。
- 2026-09-18（ステップ 1・2 の結果。理由は実装記録 07）: 前へ進むアニメはその場の形にし、捕獲の 3 本は進んだまま（`Capture_2` 5.7 m・`_3` 2.1 m。項目 9 で扱う）。メッシュの正面は UE の +Y（アクタでメッシュを Yaw −90。PIE でアクタの前へ向くことを確かめた）。
- 2026-09-18（ステップ 3。値と理由は実装記録 07 の「敵のアクタ」）: 本家のナースの値を写し、アニメは本家の ABP どおり毎フレーム State == Stun を読む（`SetState` の直後から気絶の姿勢、止まるのは次の判断）。`CanSpawn` は本家の既定の偽のままなので、エディタのレベルに置く敵は詳細で真にする（項目 7・10 で敵を出す仕組みを決める）。本家のナースのほかの部品（捕獲の判定 `Sphere` は項目 9、上空の板は項目 10、`Talk Audio` は項目 20）の値も 07 記録にある。

## 要確認（ユーザー）

- 2026-09-18: 敵の足の運びに合わせた再生の速さ — 仮に `Walk` = 速さ / 133 を 0.5〜2 倍、`Run` = 速さ / 450・`Run_Nightmare` = 速さ / 460 を 0.6〜1.8 倍にした（巡回の 350 cm/s では歩きが 2 倍で頭打ちになり、足が少し滑る）。理由: 本家はスケートで速さ 1、範囲は WebGL 版の値。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the play rate follows the speed`（ステップ 4 の PIE で見る）。
- 2026-09-18: 全回収後の追跡の走り `Run_Nightmare` への切り替え — 仮に走りと同じ 0.25 s のブレンドにした。理由: 本家の ABP に無い分岐。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the original has one run`。

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いていて、PIE なし・未保存なし（2026-09-18 02:20 時点。ステップ 3 のビルドで開き直したので、この反復の MCP は切れている。次の反復では `Tools/ue_remote.py` で状態を確かめる）。
- PIE で敵を出すのは `unreal.WasamiEnemy.spawn_enemy`（`CanSpawn` を真にする。`World.spawn_actor` などで出すと BeginPlay で消える）。アニメの口は `enemy.get_enemy_anim()`（`play_once('Chase_Slide')`・`get_main_clip()`）、気絶は Primal Fear（Python からインターフェースの `set_state` を呼べるかは未確認）。ステップ 2 ではプレイヤーの `mesh` に `SK_WasamiEnemy` を載せ、`register_slate_post_tick_callback` で前進させて歩き・走りを見た（PIE を止めれば消える）。
- Automation テストはエディタが前面でないと 3 fps で待たされる。前面が無人運転の端末のときは、エディタのタイトルバー（端末と重ならない位置。2026-09-18 は (2700, 76)）を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押す。終わりはログの `Automation Test Queue Empty N tests performed`。
- 原本は `tmp/` にもある（git の外）: `enemy_wasami_v3.glb`（`SourceArt/` に写した）、旧 `enemy_wasami.glb`（`enemy_wasami_capture.glb` の元）、`boss_wasami.glb`（項目 11）、`wasami_mochi_v3.glb`（項目 22）。消さない。
- 取り込みをやり直すときは MCP の `WasamiDDTools.import_wasami_enemy`（置き換えなので何度でもよい。数秒）。ツールセットのクラスを変えたら先に `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`。
- アニメ BP の道具の有無は MCP の `describe_toolset`（`editor_toolset.toolsets.blueprint.BlueprintTools`）で見た。出力が 68 KB あるので、読むなら python で名前だけ抜く。

## 検証

- ステップ 1・2（2026-09-18）: 取り込みと `Wasami.Enemy.Anim.*` 5 本は成功。PIE でプレイヤーのメッシュに載せて待機・1 回再生・気絶・歩き・走りを見た。捕獲の載せ替えの見た目と、足の滑り・起き上がりの終わりはステップ 4 で見る。
- ステップ 3（2026-09-18）: ビルドは警告なし。`Wasami.Enemy.*` 8 本と `Wasami` 全体 29 本が成功（1 回目は気絶のテストの期待の時刻がエンジンのタイマーの刻みとずれて落ち、`GetStunTimeLeft` が Patrol の間も残りを返す誤りも見つかった。両方直して再ビルド。症状索引に「タイマーが 1〜2 刻み遅れて発火する」）。PIE はステップ 4。`check_records.py` OK。
