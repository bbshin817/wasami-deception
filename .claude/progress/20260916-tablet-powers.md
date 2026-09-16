---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-16 21:15
---

# タブレットから使える特殊効果（パワー）をすべて実装する

## 依頼

2026-09-16 のユーザーの依頼（原文）:「次のタスクは、タブレットより利用可能な全ての特殊効果（テレポーテーション、スピードブースト、テレキネシス等々）を実装することです。実装に必要なステップを見積もり、着手してください。ただし、テレポーテーションの実装に限りpak_referenceを尊重するものとします。」

- 本家のパワーは 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）。調査の結果は `.claude/references/powers/`（README が索引と要点、01〜04 が根拠つきの本文）。
- テレポーテーションだけは `pak_reference/`（旧版）に従う。それ以外と全パワー共通の仕組みは `pak_reference_2/`（最新版）に従う。

## 計画

各ステップの終わりにコミットし、記録を直して、ユーザーに `/clear` をお願いする。

- [x] 1. 原作データの調査をまとめる（`.claude/references/powers/`）、ユーザーに 4 点を確認
- [x] 2. パワーの土台（C++）… 2026-09-16 完了。`UWasamiPowerComponent`（枠・Q/E/1/2・2 段の連打防止・ゲージ・強化段階の表・死亡のリセット・スピードブースト Lv5）、敵とシャードのインターフェース、タブレットの 6 アイコンと弾み、パワーの音とシェイクの取り込み、パイプラインの素材のソフト参照化（エディタが落ちた件の対策）。実装記録は 04 を新設し、00〜03 を直した
- [ ] 3. スピードブーストの演出: `CameraAnim_SpeedBoost` の色調（ブレンド 0.5 s）、`UMG_SpeedBoost`（集中線 `M_Speedlines`・ビネット `T_VignetteNew`）、Chameleon（放射ブラー・画面の揺れ。0〜870 cm/s に連動）
- [ ] 4. テレポーテーションの仕組み（旧版）: Teleport チャンネル（旧版の既定 Overlap）、病院のゾーン（`hospital_zone_01/02_teleport` と救急車の屋根の箱。複雑コリジョン、非表示、QueryOnly）をレベルの組み立てに足す、照準のアクタ（SpringArm のラグ・500 cm の下向きトレース・ホイール・左クリック）、0.12 s 後のスイープ移動（カプセルの WorldDynamic / Pawn を Ignore → Block）、`CameraAnim_Teleport`（FOV・露出・色調を C++ で評価）、シェイク、音 4 つ（照準ループを含む）、照準中の同じ側の Q で取り消し、再使用 5 s、最大距離 1500（Lv5）
- [ ] 5. テレポーテーションの見た目: デカール `M_Decal_Teleport`（推定）、パーティクル `P_ky_cutter2`（Cascade を Python で組めるか確かめる。だめなら Niagara）
- [ ] 6. 一瞬の演出の共通部品（全画面のポストプロセス 2 つとカーブ）と Primal Fear（半径 3500 の重なり判定で敵に `Set State(Stun)`、球 `M_05_Primal`、シェイク `ElevatorShakeStop` ×25、`Stun_Wave_Attack_New_04`、再使用 23 s）
- [ ] 7. Vanish（カプセルの Camera 応答、`Player Vanish` の通知、`UMG_Vanish` と `MM_WobblyVignette`、ポストプロセス、煙 `PPP_VanishPuff`、15 s・再使用 15 s）
- [ ] 8. Telepathy（`BP_Telepathy` とトラッカー、`UMG_TelepathyTracker` と `MM_Telepathy`、開始と終わりの音、シェイク、9 s・再使用 6.5 s）
- [ ] 9. シャードの最小限（M3 の前倒し。最新版の `BP_Shard`）: 病院の配置（Zone 1 は 337、Zone 2 は 342）、回収（数・音 `Soul_Shard_Pickup_v2_Cue` と同時発音 `OnlyFew`・シェイク `BP_CameraShake_ShardCollect`・閃光 `P_ky_flash3`）、`Activate` の引き寄せ（`Shard Pull`、ExpoIn）、タブレットの数と地図
- [ ] 10. テレキネシス（半径 3000 の重なり判定、`P_ky_forceField_Telekinesis`、ポストプロセス、シェイク、音、再使用 8 s）
- [ ] 11. 実機との見比べ（推定したマテリアルとパーティクル）と PIE での確認（エディタの PIE と本家の起動はユーザーの確認を取る）
- [ ] 12. 仕上げ: 実装記録（04 はステップ 2 で新設し、各ステップで書き足す）、handover の「現状と次の一歩」、main へマージして push、この記録を消す

## 次にやること

ステップ 3（スピードブーストの演出）を始める。まず `.claude/references/powers/01-player-system.md` の §5（とくに §5.1 の `CameraAnim_SpeedBoost`、§5.2 の `Sprinting Effects`・`UMG_SpeedBoost`）と、実装記録 `04-powers.md` の「スピードブースト」を読む。`CameraAnim_SpeedBoost` の書き出し（`pak_reference_2/_camera/CameraAnim_SpeedBoost.json`）、`UMG_SpeedBoost`（`UI/Main/Powers/`）、`M_Speedlines`・`T_VignetteNew`、Chameleon（`/Game/ThirdParty/Chameleon/Chameleon`）の中身を確かめ、どう作るかを「決定事項」に書いてから作る。素材は `dd_powers.py` に足し、C++ ではソフト参照で持つ（`WasamiAssets.h`）。

## 決定事項

- 2026-09-16: テレキネシスを実装する — これまでの「Telekinesis は実装しない」（WebGL 版のユーザーの指定）を、今回の依頼がテレキネシスを名指ししているので取り下げる。
- 2026-09-16: テレポーテーション以外のパワーと共通の仕組みは `pak_reference_2` に従う — 依頼の「テレポーテーションの実装に限り pak_reference を尊重する」から。
- 2026-09-16: 大規模改修なので `feature/tablet-powers` ブランチで進める（`.claude/guides/git-workflow.md`）。
- 2026-09-16（ユーザーの回答）: 強化段階は **Lv5（最大）** に固定する。本作にリングの祭壇が無いため。
- 2026-09-16（ユーザーの回答）: スピードブーストの再使用は**最新版のコード**の値（表示より 1 s 長い。Lv5 で 7.5 s）。
- 2026-09-16（ユーザーの回答）: テレキネシスを確かめるため、**シャードの最小限を先に作る**（M3 の前倒し。ステップ 9）。
- 2026-09-16（ユーザーの回答）: パワーが使えないときは**どのパワーも無音**（最新版どおり）。テレポートの照準中に同じ側の Q で取り消す処理は旧版から採る。
- 2026-09-16: 枠の初期値は原作のコードどおり左右とも添字 0（解放済みの先頭 = Speed Boost）。原作の `Selected Power Left/Right` は GameInstance の値でディスクに保存されないので、本家を起動して病院に入ったときも左右とも Speed Boost になるはず（実機では未確認）。
- 2026-09-16: 敵（M4）はまだ無いので、Primal Fear・Vanish・Telepathy は受け口のインターフェースまで作り、確認は仮の的（インターフェースを実装したテスト用のアクタ）で行う。
- 2026-09-16（ステップ 2 の構成）: 原作の「プレイヤーのパワーの処理」「`BP_Powers`（ゲージのタイムライン 6 本）」「`UMG_TabletPowers` の `Power` 配列と左右の添字」を 1 つの `UWasamiPowerComponent`（プレイヤーに付ける）にまとめる。原作は 3 か所に分けているが、どれもプレイヤー 1 人に 1 つで、互いに直接呼び合うだけなので分ける意味が無い。タブレットのウィジェットは表示だけ（6 アイコンの MID・左右の枠の出し分け・弾む演出）を受け持つ。
- 2026-09-16: 原作の `Delay` ノード（動いている間に呼び直しても無視される）は、`FTimerManager` の「動いていなければ仕掛ける」で写す。死亡のリセットの後に古い Delay が残って早く切れる、といった原作の癖もそのまま出る。
- 2026-09-16: Telepathy・Primal・Telekinesis・Vanish のクールダウン明けの Gate は「Open の直後に Enter」（@6301〜@6444 ほか）なので、閉じても次の Enter の前に必ず開く＝素通しと同じ。Gate の状態は持たず、「明けたら充填」「リセットは充填」とする。リセットは使っていないときも充填を通るので `power_refilled` が鳴る（Reset Primal = @37261: Push @6444 → @27788 の Close）。スピードブースト（DoOnce_3 / DoOnce_4）とテレポート（DoOnce_5）は Start Closed で、使った後だけ効く（@1211・@3345）。
- 2026-09-16: 枠を弾ませる `Use Left` / `Use Right` は、原作のキー（ティック 0 / 3000 / 9000 / 30000 → 1.0 / 1.25 / 1.10 / 1.0）と書き出された接線（1 ティックあたり 1.1111e-5・−9.2593e-6。秒に直すと 0.6667・−0.5556）を `FRichCurve` にして評価し、枠の画像の `RenderTransform` の拡縮に入れる（UMG のアニメを C++ で作らない）。時間はプレイヤーのティックで進める。
- 2026-09-16: パワーの新しい音と素材は両版で同一（アイコン 12 枚の PNG・テクスチャ設定、`power_refilled`・`Shard_Streak_Milestone_V5`・`UI_Select_V3` の ogg、`BP_CameraShake_Streak` の値を突き合わせた）。`MM_Powers_*` のインスタンス名は 6 つそろっている最新版の名前を使う。`Shard_Streak_Milestone_V5` の SoundWave は音量 0.7・ピッチ 2.0、最新版だけ同時発音 `NewSoundConcurrency`（MaxCount 2・VolumeScale 0.5）を持つので、それも写す（SoundWave の値は書き出しから読む汎用の取り込みにする）。
- 2026-09-16: 本家の変数 `Has Input`・`Can Interact?`（既定 true。台本が切り替える）をプレイヤーに足す。`Use Power` は `Can Use Tablet?` と `Has Input`、Q / E は `Can Interact?` を見る。
- 2026-09-16: ステップ 2 では、スピードブースト以外のパワーは枠を弾ませて連打防止を通り `OnPowerUsed` を出すところまで（使える状態は変えない）。各パワーの中身は後のステップで足す。テレポートのリセットの残り（照準のアクタを消す・`UsedTeleport`）もステップ 4 で足す。
- 2026-09-16（ユーザーの回答）: 落ちたエディタの起動し直しと、素材の取り込み・テスト・PIE の確認（エディタへの Q/E/1/2/Space の入力を含む）まで進めてよい。
- 2026-09-16: パイプラインが作る素材は C++ のコンストラクタで読まず、ソフト参照（`TSoftObjectPtr` / `TSoftClassPtr` の UPROPERTY に既定のパスを入れる）にして BeginPlay / RebuildWidget で読む。起動時に読むとルートに入り、以後の作り直しでエディタが落ちるため（再開時の注意）。この先のステップで推定のマテリアルを何度も作り直すので、ここで直しておく。
- 2026-09-16: テストは `Source/wasami_deception/Tests/` に置き、モジュールのヘッダーは `../` で読む（Build.cs に include パスを足すより変更が小さい）。
- 2026-09-16: 実装記録は、パワーの仕組みを `04-powers.md` として**このステップで新設**する（新しいソースはどれかの記録に載せる決まりのため）。索引の予定表の番号は 1 つずつ後ろにずらす。

## 再開時の注意

- 2026-09-16 21:15 の時点で、エディタは起動している（PID 28568、セッション 1、`L_Hospital_Zone1` を開いている、PIE は止めた、未保存の変更なし）。操作のエージェント（`Tools/desktop.py`）は止めた。MCP はエディタを開き直した後に切れている（`/mcp` の再接続はユーザーに頼む。それまでは `Tools/ue_remote.py` で `from wasami_tools.toolsets.dd import WasamiDDTools` を呼べば同じ）。
- **エディタが落ちた件（20:36）は解決済み**: C++ のコンストラクタで読んだ素材は起動時にルートに入り、作り直すと `!IsRooted()` で落ちる。C++ はパイプラインの素材をソフト参照で持つように直し（00 記録）、再起動後に `obj refs` で `M_DD_MapPlane` がルートに入っていないこと（起動時に読まれる `WorldGridMaterial` には `(root)` が付く）を確かめてから取り込みをやり直した。以後、C++ に素材を足すときは `ConstructorHelpers` を使わない。
- `Tools/desktop.py` で PIE に入力を送るときは、ビューポートを 1 回クリックして焦点を渡してからキーを送る（クリックしないとキーが届かなかった）。PIE を始めた直後に前面にある窓がテストのログなどのときは、先に閉じる。
- 画面の撮影 1 回に数秒かかるので、時間に依存する確認は `unreal.GameplayStatics.get_time_seconds` と一緒に読む（scratchpad の `probe_powers.py` がパワーの状態を読む。scratchpad はセッションごとなので、要るときは作り直す）。

## 検証

- check_records: OK（ステップ 2。00〜04 の 5 件）
- C++ ビルド: 成功（ステップ 2、警告なし）
- Automation テスト: `Wasami.Powers`（Gauge・Tuning・SocketBounce）3 件すべて成功
- 取り込み: `import_dd_tablet`（テクスチャ 25・マテリアル 2・メッシュ 1・フォント 1・音 3・ミニマップとアイコン 8）、`import_dd_powers`（音 2・シェイク 1）。音量・ピッチ・同時発音・シェイクの値・インスタンスのテクスチャを読み戻して原作の値どおり。`Failed to compile` なし
- PIE（ユーザーの了承のうえで入力を送った）: E / Q でブースト（950・約 2 秒でゲージ 0.796・約 18 秒後には戻る）、タブレットの枠のアイコンが扇形に減る、1 / 2 の巡回と末尾からの折り返し、6 種のアイコンの表示、タブレットを下ろすと巡回しない、未実装の Teleport は使える状態のまま、`ResetPowers` で即座に戻る。PIE は止めた
