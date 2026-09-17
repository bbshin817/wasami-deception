---
title: 敵ワサミの素体（作業一覧の項目 4）
status: 進行中
branch: feature/enemy-wasami-body
base: b25dd05
started: 2026-09-17 23:14
updated: 2026-09-18 02:50
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

- [x] 0〜3. 計画・原本と取り込み・アニメの再生・敵のアクタ … 2026-09-18 完了。`SourceArt/Wasami/` の glb 2 つ、`dd_enemy.py`（`/Game/Wasami/Enemy` に `SK_WasamiEnemy` とアニメ 19 本）、`UWasamiEnemyAnimInstance`、`AWasamiEnemy`、テスト `Wasami.Enemy.*` 8 本。中身は実装記録 07。
- [x] 4. PIE での確かめ … 2026-09-18 完了。立ち姿・歩き・走り・Nightmare・気絶と明け・Telepathy・Vanish・1 回再生 9 本を撮って測った（値は `observations/README.md` の「敵ワサミ」、道具は `observations/tools/enemy_*`）。`Run_Nightmare` の分母を 460 → 500（足の滑り −8 % → −0.5 %）。ビルドで無名名前空間の `EnemyTag` がぶつかり、仮の的の側を `TestEnemyTag` にした（症状索引）。
- [ ] 4b. `Chase_VaultLand` の形を直す（仮の扱い。要確認）— `dd_enemy.py` にこの 1 本の作り方を足す: その場の形に加え、足の高さから台の高さ（離れる前の接地の足 − 着地後の足。PIE の骨では約 76 cm）と、離れる時刻（約 0.83 s）・着地の時刻（約 1.56 s）を glb から測り、離れるまでは骨盤を台の高さだけ下げ、着地までに `_smooth` で 0 へ戻す。着地の後、骨盤が待機の高さに戻って止まる所（約 2.4 s）で切る。`WasamiDDTools.import_wasami_enemy` で取り込み直し、`Wasami.Enemy.Anim.Clips` の長さの期待（`Tests/WasamiEnemyTests.cpp`）を直してビルド、`Wasami` 全体を回す。PIE で `sh observations/tools/enemy_once_rec.sh Chase_VaultLand 8` を撮り直し、骨の標本で足が床（12 cm 前後）から離れて始まり、宙に浮かないことを確かめる。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_enemy.py`、`/Game/Wasami/Enemy/A_WasamiEnemy_Chase_VaultLand`、`Source/wasami_deception/Tests/WasamiEnemyTests.cpp`、実装記録 07、一覧、`observations/README.md`
- [ ] 5. 仕上げ — 作業一覧の項目 4 を完了にし、handover の「現状と次の一歩」、note の原稿（`docs/note/progress.md`）、要確認を作業一覧の「未回答の要確認」へ移し、記録を消して main へマージ・push（`git checkout main` → `git merge --no-ff`）。

## 次にやること

ステップ 4b: `Chase_VaultLand` の形を直す（計画の 4b）。先に `dd_enemy.py` の `_in_place`・`_pelvis_offset`・`_stun_recover`（トリムの例）と `pipeline/gltf.py` の骨の世界位置の求め方を読み、glb の足（`ball_l`・`ball_r`）の高さから台の高さと 2 つの時刻を測ってから形を決める。

## 決定事項

- 2026-09-18（ユーザーの指示と回答）: モデルは `enemy_wasami_v3`。捕獲の 3 本は旧 glb の 3 本を流用。追跡中のランダムの動き（約 8 s に 1 回、速さは 800 cm/s のまま、前方が空いているときだけ）は項目 7 で作り、この項目では 1 回再生の口とその場の形の前処理まで作る。入口レベルは作らない（作業一覧の「決めたこと」）。ユーザーは追跡中に流す例に「片手をついて障害物を飛び越える動作」を挙げた。
- 2026-09-17（ユーザーの回答「不要」）: 赤い縁取りは作らない（Chameleon の `Custom Depth Highlighter (Clip)` も敵の `CustomDepth(Duration)` も）。
- 2026-09-18（ステップ 4 の PIE。値は `observations/README.md`）: 待機 `Idle_11`・見張り `Idle_5` はそのまま（Claude の仮）。気絶の明けのブレンド 0.25 s は変えない（うなだれた頭が動き出しと同時に上がり、跳ねては見えない）。巡回の歩きの再生の速さの上限 2 は変えない（要確認）。`Chase_VaultLand` は、ユーザーの挙げた例がこの動きなので追跡の候補から外さず、台の無い床でも跳び越える形に取り込みで直す（ステップ 4b。要確認）。

## 要確認（ユーザー）

- 2026-09-18: 敵の足の運びに合わせた再生の速さ — 仮に `Walk` = 速さ / 133 を 0.5〜2 倍、`Run` = 速さ / 450・`Run_Nightmare` = 速さ / 500 を 0.6〜1.8 倍にした。PIE では追跡の走りはほぼ滑らない（1 %）が、巡回 350 cm/s の歩きは上限 2 倍で足が速さの 24 % 滑る（上限を 2.6 にすれば滑らないが、1 秒に約 5 歩のせかせかした歩きになる）。理由: 本家はスケートで速さ 1、範囲は WebGL 版の値、分母は PIE で測った足の速さ。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the play rate follows the speed`。
- 2026-09-18: 全回収後の追跡の走り `Run_Nightmare` への切り替え — 仮に走りと同じ 0.25 s のブレンドにした。理由: 本家の ABP に無い分岐。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the original has one run`。
- 2026-09-18: 待機と見張りの待機のアニメ — 仮に `Idle_11`（直立）と `Idle_5`（足を開いた低い構え）にした。理由: Claude が v3 の中から選び、PIE で見て不自然さが無かった（`observations/ours/pie-enemy-idle-*.png`・`pie-enemy-alert-*.png`）。場所: `.claude/references/enemy-wasami-motions.md` の表、`dd_enemy.py` の `ROLES`。
- 2026-09-18: `Chase_VaultLand`（`Vault_and_Land`）の扱い — これは「高さ約 76 cm の台の上から片手をついて跳び越えて床へ降りる」動きで、平らな廊下では宙から始まる。仮に、取り込みで台の高さの分を下げて床から跳び越える形にし、着地後の立ったままの区間を切る（ステップ 4b）。ほかの案: 追跡の候補から外して場面（項目 6 の Zone 1 の出来事）でだけ使う / そのまま流す。理由: ユーザーが追跡中の例に挙げた動きなので、候補に残す。場所: `dd_enemy.py`（4b で作る）、一覧の追跡中の変化の表。

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いていて、PIE なし・未保存なし（2026-09-18 02:50 時点。ステップ 4 のビルドで開き直した）。
- PIE で敵を動かして撮るのは `observations/tools/enemy_*`（使い方は `observations/README.md` の tools/ の表。`ue_remote.py -c` で `sys.path` に `observations/tools` を足して `import enemy_probe`）。PIE を始めたら `DisableAllScreenMessages`・`t.MaxFPS 60`・シャードを隠す（`WasamiShard` の `set_actor_hidden_in_game`）、終わったら `t.MaxFPS 0` と `pie.py stop`。`bSeenPlayerRecently` など `VisibleInstanceOnly` の値は Python から書けない。
- PIE・テストの後に Automation の「メッセージ ログ」の小窓がビューポートに重なって前面に出る。閉じるボタンは (2198, 408)（`desktop.py click 2198 408 --allow UnrealEditor.exe`）。
- Automation テストはエディタが前面でないと 3 fps で待たされる。前面が無人運転の端末のときは、エディタのタイトルバー (2700, 76) を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押す。テストは `ue_remote.py` で `Automation RunTests Wasami` を送り、ログの `Automation Test Queue Empty N tests performed` を待つ（2026-09-18 は 29 本）。
- 原本は `tmp/` にもある（git の外）: `enemy_wasami_v3.glb`、旧 `enemy_wasami.glb`、`boss_wasami.glb`（項目 11）、`wasami_mochi_v3.glb`（項目 22）。消さない。
- 取り込みをやり直すときは MCP の `WasamiDDTools.import_wasami_enemy`（置き換えなので何度でもよい。数秒）。ツールセットのクラスを変えたら先に `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`。
- `Tools/editor_cycle.py` はビルドに失敗するとエディタを閉じたままにする。直したら `--no-quit`。

## 検証

- ステップ 1〜3（2026-09-18）: 取り込み、ビルド（警告なし）、`Wasami` 29 本が成功。`check_records.py` OK。
- ステップ 4（2026-09-18）: PIE の確かめ（上）。分母を直した後のビルドは警告なし、`Wasami` 29 本が成功、Nightmare の足の滑り −0.5 %。
