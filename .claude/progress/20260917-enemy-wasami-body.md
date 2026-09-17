---
title: 敵ワサミの素体（作業一覧の項目 4）
status: 進行中
branch: feature/enemy-wasami-body
base: b25dd05
started: 2026-09-17 23:14
updated: 2026-09-18 03:05
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
- [x] 4b. `Chase_VaultLand` の形 … 2026-09-18 完了。前処理の `vault`（台の高さ 76.6 cm を離れるまで下げて着地までに戻す・2.4 s で切る・31° 斜めの進みを真っすぐに回す）。前処理の glb を `WasamiEnemy.glb` に改名（前の名前ではアニメが置き換わっていなかった。症状索引）。PIE で床から跳ぶのを確かめた（`observations/README.md`）。
- [ ] 5. 仕上げ — 作業一覧の項目 4 を完了にし、handover の「現状と次の一歩」、note の原稿（`docs/note/progress.md`）、要確認を作業一覧の「未回答の要確認」へ移し、記録を消して main へマージ・push（`git checkout main` → `git merge --no-ff`）。

## 次にやること

ステップ 5: 仕上げ（計画の 5）。作業一覧 `.claude/roadmap.md` の項目 4 を完了にし、`.claude/references/handover.md` の「現状と次の一歩」と note の原稿 `docs/note/progress.md`（`.claude/guides/note-progress.md`）を直し、下の要確認 4 件を作業一覧の「未回答の要確認」へ移し、この記録を消して main へマージ（`git checkout main` → `git merge --no-ff feature/enemy-wasami-body`）・push・ブランチ削除。

## 決定事項

- 2026-09-18（ユーザーの指示と回答）: モデルは `enemy_wasami_v3`。捕獲の 3 本は旧 glb の 3 本を流用。追跡中のランダムの動き（約 8 s に 1 回、速さは 800 cm/s のまま、前方が空いているときだけ）は項目 7 で作り、この項目では 1 回再生の口とその場の形の前処理まで作る。入口レベルは作らない（作業一覧の「決めたこと」）。ユーザーは追跡中に流す例に「片手をついて障害物を飛び越える動作」を挙げた。
- 2026-09-17（ユーザーの回答「不要」）: 赤い縁取りは作らない（Chameleon の `Custom Depth Highlighter (Clip)` も敵の `CustomDepth(Duration)` も）。
- 2026-09-18（ステップ 4・4b の PIE。値は `observations/README.md`）: 待機 `Idle_11`・見張り `Idle_5`、気絶の明けのブレンド 0.25 s、巡回の歩きの再生の速さの上限 2 はそのまま（要確認）。`Chase_VaultLand` は追跡の候補に残し、取り込みで床から跳ぶ形にした（要確認）。

## 要確認（ユーザー）

- 2026-09-18: 敵の足の運びに合わせた再生の速さ — 仮に `Walk` = 速さ / 133 を 0.5〜2 倍、`Run` = 速さ / 450・`Run_Nightmare` = 速さ / 500 を 0.6〜1.8 倍にした。PIE では追跡の走りはほぼ滑らない（1 %）が、巡回 350 cm/s の歩きは上限 2 倍で足が速さの 24 % 滑る（上限を 2.6 にすれば滑らないが、1 秒に約 5 歩のせかせかした歩きになる）。理由: 本家はスケートで速さ 1、範囲は WebGL 版の値、分母は PIE で測った足の速さ。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the play rate follows the speed`。
- 2026-09-18: 全回収後の追跡の走り `Run_Nightmare` への切り替え — 仮に走りと同じ 0.25 s のブレンドにした。理由: 本家の ABP に無い分岐。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the original has one run`。
- 2026-09-18: 待機と見張りの待機のアニメ — 仮に `Idle_11`（直立）と `Idle_5`（足を開いた低い構え）にした。理由: Claude が v3 の中から選び、PIE で見て不自然さが無かった（`observations/ours/pie-enemy-idle-*.png`・`pie-enemy-alert-*.png`）。場所: `.claude/references/enemy-wasami-motions.md` の表、`dd_enemy.py` の `ROLES`。
- 2026-09-18: `Chase_VaultLand`（`Vault_and_Land`）の扱い — 元は「高さ約 76 cm の台の上から片手をついて跳び降りる」動きで、平らな廊下では宙から始まった。仮に、取り込みで台の高さの分を離れるまで下げて床から跳び越える形（見えない低い障害物を越える形。足は最高約 78 cm、骨盤は 130 cm）にし、着地後に立っているだけの区間を切り（2.4 s）、この 1 本だけ 31° 斜めに進むのを真っすぐに回した（体の向きは −7° で始まり 21° で終わる）。ほかの案: 追跡の候補から外して場面（項目 6 の Zone 1 の出来事）でだけ使う / そのまま流す。理由: ユーザーが追跡中の例に挙げた動きなので、候補に残す。場所: `dd_enemy.py` の `VAULT_FRAMES`・`_vault`、一覧の追跡中の変化の表、映像 `observations/ours/pie-enemy-once-Chase_VaultLand.mkv`・`-sheet.png`。

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いていて、PIE なし・未保存のマップなし（2026-09-18 03:05 時点。ステップ 4b のビルドで開き直した。テストの一時的なワールド `/Temp/Untitled_*` が未保存に出るのは無視してよい）。`/Game/Wasami/Enemy` は 02:54 に取り込み直した状態。
- PIE で敵を動かして撮るのは `observations/tools/enemy_*`（使い方は `observations/README.md` の tools/ の表。`ue_remote.py -c` で `sys.path` に `observations/tools` を足して `import enemy_probe`）。PIE を始めたら `DisableAllScreenMessages`・`t.MaxFPS 60`・シャードを隠す（`WasamiShard` の `set_actor_hidden_in_game`）、終わったら `t.MaxFPS 0` と `pie.py stop`。`bSeenPlayerRecently` など `VisibleInstanceOnly` の値は Python から書けない。
- PIE・テストの後に Automation の「メッセージ ログ」の小窓がビューポートに重なって前面に出る。閉じるボタンは (2198, 408)（`desktop.py click 2198 408 --allow UnrealEditor.exe`）。
- Automation テストはエディタが前面でないと 3 fps で待たされる。前面が無人運転の端末のときは、エディタのタイトルバー (2700, 76) を `desktop.py click … --allow WindowsTerminal.exe --allow UnrealEditor.exe` で 1 回押す。テストは `ue_remote.py` で `Automation RunTests Wasami` を送り、ログの `Automation Test Queue Empty N tests performed` を待つ（2026-09-18 は 29 本）。
- 原本は `tmp/` にもある（git の外）: `enemy_wasami_v3.glb`、旧 `enemy_wasami.glb`、`boss_wasami.glb`（項目 11）、`wasami_mochi_v3.glb`（項目 22）。消さない。
- 取り込みをやり直すときは MCP の `call_tool`（`toolset_name` = `wasami_tools.toolsets.dd.WasamiDDTools`、`tool_name` = `import_wasami_enemy`。短い名前 `WasamiDDTools` では見つからない）。置き換えなので何度でもよい（数秒。2026-09-18 に前処理の glb を改名してアニメも置き換わるようにした）。ツールセットのクラスを変えたら先に `python Tools/ue_remote.py -c "import wasami_tools; from toolset_registry import _reload; _reload.reload_module(wasami_tools)"`。
- `Tools/editor_cycle.py` はビルドに失敗するとエディタを閉じたままにする。直したら `--no-quit`。

## 検証

- ステップ 1〜4（2026-09-18）: 取り込み、ビルド（警告なし）、`Wasami` 29 本が成功、`check_records.py` OK。PIE の確かめ（`observations/README.md`）。
- ステップ 4b（2026-09-18）: 取り込み直し（19 本の長さがテストの期待と一致、`Chase_VaultLand` 72 フレーム）、ビルド（警告なし）、`Wasami` 29 本が成功、PIE の骨の標本で足は床（10〜11 cm）から離れて始まる、`check_records.py` OK。
