# 実装記録 索引

運用ルールは `.claude/guides/implementation-records.md`。同期チェックは `python .claude/scripts/check_records.py`。

## 記録一覧

| 記録 | 内容 |
| --- | --- |
| [00-overview.md](00-overview.md) | 全体像。モジュールとビルド、プラグイン、`Config/` の設定（露出・静的ライティング・レイトレース・既定のマップとゲームモード・Python）、灯の焼き込み（原作と同じ High 品質）と実機との比較、作業の流れ |
| [01-stage-pipeline.md](01-stage-pipeline.md) | 取り込みの仕組み。病院ステージの前処理 `Tools/dd/prepare_stage.py`（テレポートのゾーンを含む）、レベルの組み立て（シャードの配置を含む）、ツールセット `WasamiDDTools` / `WasamiStageTools` / `WasamiDevTools`、本家のアセットを原作データから作り直す仕組み（Cascade のパーティクルを組む `UWasamiCascadeLibrary`、SoundCue を組む `UWasamiSoundCueLibrary`、材質のインスタンスの静的マスクを書く `UWasamiMaterialLibrary` の C++ の道具を含む）、本作の素材（`SourceArt/`）の取り込み、リモート実行とエディタの開き直し |
| [02-player.md](02-player.md) | プレイヤーとゲームモード。カプセルとカメラ、Enhanced Input（テレポートへ渡す左クリックとホイールを含む）、歩き・ダッシュ・ブースト、速さに連動する FOV、頭の揺れ、180° ターン、タブレットの出し入れとミニマップのキャプチャ |
| [03-tablet.md](03-tablet.md) | タブレットの画面（`UWasamiTabletWidget`。パワーの枠の出し分けと弾み、シャードの回収の Count Shake）、素材の取り込み（`dd_tablet.py`）、ミニマップの仕掛け（Zone 2 の階ごとの地図 `AWasamiMapTextureMultiFloor`・`AWasamiMapArea`: 本家の `BP_MapTexture_MultiFloor`・`BP_MapArea`。いる階の地図の絵とシャードの印を出す）と地図の矢印（`AWasamiArrowPointer`: 本家の `BP_ArrowPointer`。最も近いシャードかゾーンの的を指す）、見張りの視界コーンの地図の印の材質 |
| [04-powers.md](04-powers.md) | タブレットのパワー（`UWasamiPowerComponent`）。左右の枠と Q / E / 1 / 2、2 段の連打防止、ゲージ、強化段階の値、死亡のリセット、スピードブーストとその演出（赤い色調・集中線とビネット・画面の揺れ）、テレポーテーション（旧版。照準のアクタ `AWasamiTeleportAim`・移動・取り消し・再使用・カメラアニメ）、一瞬の演出の共通の基底 `AWasamiPowerBurst` と Primal Fear（`AWasamiPrimalPower`）、テレキネシス（`AWasamiTelekinesisPower`。半径の中のシャードを引き寄せる。力場の粒子と推定の材質）、Vanish（`AWasamiVanishPower`・画面の `UWasamiVanishWidget`・カプセルの Camera 応答）、Telepathy（`AWasamiTelepathyPower`・敵ごとの画面空間の印 `AWasamiTelepathyTracker` と `UWasamiTelepathyTrackerWidget`）、仮の的 `AWasamiTestEnemy`、UE4 の CameraAnim の再生（`UWasamiCameraAnim`。PP と FOV のトラック）、プレイヤーの FX（本家の Chameleon）、敵とシャードのインターフェース |
| [06-game-flow-save.md](06-game-flow-save.md) | ゲームの流れ。ゲームインスタンス（`UWasamiGameInstance`: ライフ 3・0..6、回収済みのシャードの記憶）とセーブ（`UWasamiSaveGame`、スロット `structSlot`）、ゲームモードの死亡の受け口・時間・チェックポイントの保存・開き直したときの回収済みのシャードの除去と、本家の Zone のレベル BP の受け持ち（チェックポイントの PlayerStart から出す・死亡画面を出す・SAVING PROGRESS・デバッグのコンソールコマンド `Wasami.Kill` ほか）。シャード（`AWasamiShard`: 本作のワサミ餅・紫の灯・拾うカプセル・地図の印・回転、触れて回収〈数の −1・Count Shake・揺れ・閃光・音〉、テレキネシスの引き寄せ）と回収の閃光 `P_ky_flash3`（推定の材質 5 つ）、その素材の取り込み（`dd_shards.py`）。画面は 09 記録。脱出はこれから |
| [07-enemies.md](07-enemies.md) | 敵ワサミ。素材の取り込み（`dd_enemy.py`: ユーザーのモデルを 30 fps に標本化し直し、役の名前のアニメ 18 本〈ループを閉じる・追跡中の変化をその場の形に・気絶を倒れる 2 本と寝返りからの起き上がり 2 本にする・旧モデルの捕獲の 3 本を載せ替える〉とスケルタルメッシュ・材質にする）と、アニメの再生（`UWasamiEnemyAnimInstance`: 本家のナースの ABP の木〈気絶・Idle ↔ Moving・歩き / 走り〉を C++ の状態と Proxy で持ち、気絶で倒れる 2 本からランダムに流して起き上がりを 17 s の終わりに合わせ、起き上がりに入るコマで敵を移し、全身の 1 回再生の口を持つ）、敵のアクタ（`AWasamiEnemy`: 本家のナースの部品・`CanSpawn`・0.5 s ごとの判断〈巡回・発見・追跡・見失い。AI MoveTo〉と 17 s の気絶・パワーの受け口・`SpawnEnemy`）、Zone 1 の駐車場の 06 型（`AWasamiEnemy06Chase`: 毎ティック追い、扉を突く）、Zone 2 の迷路の型（`AWasamiEnemyZone2`: 階が違うとリフトへ）、ミニボスの廊下の見張りと視界コーン（`AWasamiEnemySentry`・`AWasamiViewcone`: 見つけると跳び降りて追う。地図の印）、捕獲の演出（`AWasamiCapture`: 本家ホテルの `Death Event` の体の黒い別室で捕獲の 3 本のどれかを再生し、暗転して 3.5 s 後に死亡画面。敵に触れたときの判定はこれから） |
| [08-stage-gimmicks.md](08-stage-gimmicks.md) | ステージの仕掛け。本家の両開き扉 `BP_06_DoubleDoors`（`AWasamiDoubleDoors`: 前後の箱でキャラクターが入ると 1 s で開き、`Leave` から出ると閉じる。`Lock`・`Unlock`・`Open Front`・`Force Close`、開閉のタイムラインと音、閉ざされたときのガタつく音）と、ゾーンの障壁 `BP_ZoneBarrier`（`AWasamiZoneBarrier`: 光る板 2 枚・灯・唸り、`Destroy` の閃光と砕ける音）、打たれたときの画面の閃き `BP_HitFX`（`AWasamiHitFX`: 境界なしの赤い PP が 0.35 s で晴れ、カメラが揺れる）、その素材の取り込み（`dd_gimmicks.py`。障壁の材質 `MM_SpeedBarrier` は焼き込みのシェーダーの式で組む）。メッシュと板の材質はレベルの組み立てが入れる。置いてあるのは流れが名指しする Zone 1 の 2 枚と両ゾーンの障壁。ほかの扉・罠・噴出・車などはこれから |
| [09-ui.md](09-ui.md) | 画面。死亡画面（`UWasamiDeathScreenWidget`: 本家の `UMG_DeathScreen` の木をスロットのまま C++ で組み、Construct〈ライフ −1・死亡数の保存〉と遅延・アニメ 4 本〈Fade In・Fade Out・Shake・Death〉をウィジェットのティックで進める。声は長さだけ待つ。ライフが残れば終わりにパワーをリセットしてレベルを開き直し、0 ならゲームオーバーの 3 つのボタン〈RESTART・LAST CHECKPOINT・QUIT TO TITLE。ホバーの色と行き先〉）、YES / NO の問い（`UWasamiPopUpWidget`: 本家の `UMG_PopUp`）、SAVING PROGRESS（`UWasamiSavingWidget`）、扉の破壊の鍵（`UWasamiSwitchboxWidget`: 本家の `UMG_07_Boss_Switchbox`。F で埋まる輪と火花、焼き込みのシェーダーから組んだ材質）、レベルを開いたときの黒のフェード（`UWasamiBlackFadeWidget`）、Zone 2 へ移るときの読み込み画面（`UWasamiLoadingWidget`: 本家の `UMG_Loading`。紋章はユーザーの確認待ち）、書き出しのキーから曲線を作る小道具（`WasamiWidgetAnimation.h`）と素材の取り込み（`dd_ui.py`）。タイトル・ポーズ・脱出・字幕はこれから |
| [11-zone-flow.md](11-zone-flow.md) | ゾーンの進行。本家の `BP_TriggerBox_Base`（`AWasamiTriggerBox`）と、扉を F の連打で破る本家の `BP_06_Hospital_DoorBreak`（`AWasamiDoorBreak`）、地図の矢印の区域になる本家の `BP_ZoneShardChecker`（`AWasamiZoneShardChecker`）、本家の 2 つの Zone のレベル BP の区間の流れ（`AWasamiZoneFlow`・`AWasamiZone1Flow`・`AWasamiZone2Flow`: ゲームモードが開始時に出し、開いたチェックポイントの区間を始め、トリガー・全回収・保存・目的・矢印の値で進める。ナースを `TargetPoint` に出す〈`SpawnEnemy`〉。場面・声などは各項目が埋める）、デバッグの `Wasami.Flow` |
| [12-lifts.md](12-lifts.md) | リフト。Zone 2 の迷路の 2 つの階をつなぐ本家の `BP_06_Lift_03`・`_04`（`AWasamiLift`: キャラクターが乗る間 535 cm 上がり、降りると下りる。乗ったプレイヤーで `Player Overlap`）と `BP_06_LiftBase_Corner`（`AWasamiCornerLift`: プレイヤーの階で待ち、1 s 立つともう一方の階へ）、共通の `AWasamiLiftBase`（一定の速さの上下と動き出し・止まりの音）。メッシュと箱の大きさはレベルの組み立てが入れる。ガレージリフト `BP_06_GarageLift`（`AWasamiGarageLift`: 骨入りのメッシュ〈`dd_skeletal.py` で取り込む〉の台がプレイヤーが乗る間アニメで約 3 m 上がる。本家の ABP の状態機械を `UWasamiGarageLiftAnimInstance` に写した。Zone 1 の `_Zone1_Special` はナースが近いと上がらない） |

## ソース → 記録 対応表

| ソース | 記録 |
| --- | --- |
| `wasami_deception.uproject`、`.mcp.json`、`Config/*.ini` | 00 |
| `Source/wasami_deception.Target.cs`、`Source/wasami_deceptionEditor.Target.cs` | 00 |
| `Source/wasami_deception/wasami_deception.Build.cs`、`wasami_deception.cpp`、`wasami_deception.h`、`WasamiAssets.h` | 00 |
| `Tools/dd/prepare_stage.py`、`Tools/dd/cooked_shaders.py`、`Tools/dd/bp_flow.py`、`Tools/ue_remote.py`、`Tools/editor_cycle.py`、`Tools/console_session.py`、`Tools/desktop.py`、`Tools/desktop_agent.py`、`Tools/pie.py`、`Tools/video_probe.py`、`Tools/overnight.py`、`Tools/discord_notify.py` | 01 |
| `Content/Python/init_unreal.py`、`Content/Python/wasami_tools/**` | 01 |
| `Source/wasami_deception/WasamiCascadeLibrary.*`、`Tests/WasamiCascadeTests.cpp`、`WasamiSoundCueLibrary.*`、`WasamiMaterialLibrary.*` | 01 |
| `Source/wasami_deception/WasamiGameMode.*`、`WasamiPlayerCharacter.*` | 02 |
| `Source/wasami_deception/WasamiTabletWidget.*`、`WasamiArrowPointer.*`、`Tests/WasamiArrowPointerTests.cpp`、`WasamiMapArea.*`、`WasamiMapTextureMultiFloor.*`、`Tests/WasamiMapMultiFloorTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_tablet.py` | 03 |
| `Source/wasami_deception/WasamiGameInstance.*`、`WasamiSaveGame.*`、`Tests/WasamiGameFlowTests.cpp` | 06 |
| `Source/wasami_deception/WasamiDeathScreenWidget.*`、`WasamiPopUpWidget.*`、`WasamiSwitchboxWidget.*`、`WasamiSavingWidget.*`、`WasamiBlackFadeWidget.*`、`WasamiLoadingWidget.*`、`WasamiWidgetAnimation.h`、`Tests/WasamiDeathScreenTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_ui.py` | 09（`dd_ui.py` は 01 にも載せる） |
| `Source/wasami_deception/WasamiShard.*`、`Tests/WasamiShardTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_shards.py`、`SourceArt/Wasami/wasami_mochi.glb` | 06（`dd_shards.py` は 01 にも載せる） |
| `Source/wasami_deception/WasamiTriggerBox.*`、`WasamiDoorBreak.*`、`Tests/WasamiDoorBreakTests.cpp`、`WasamiZoneFlow.*`、`WasamiZone1Flow.*`、`WasamiZone2Flow.*`、`Tests/WasamiZoneFlowTests.cpp`、`WasamiZoneShardChecker.*` | 11 |
| `Source/wasami_deception/WasamiLift.*`、`WasamiGarageLift.*`、`Tests/WasamiLiftTests.cpp`、`Tests/WasamiGarageLiftTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_skeletal.py` | 12（`dd_skeletal.py` は 01 にも載せる） |
| `Source/wasami_deception/WasamiDoubleDoors.*`、`Tests/WasamiDoubleDoorsTests.cpp`、`WasamiZoneBarrier.*`、`Tests/WasamiZoneBarrierTests.cpp`、`WasamiHitFX.*`、`Tests/WasamiHitFXTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_gimmicks.py` | 08（`dd_gimmicks.py` は 01 にも載せる） |
| `Content/Python/wasami_tools/pipeline/dd_enemy.py`、`SourceArt/Wasami/enemy_wasami_v3.glb`、`SourceArt/Wasami/enemy_wasami_capture.glb` | 07（`dd_enemy.py` は 01 にも載せる） |
| `Source/wasami_deception/WasamiEnemy.*`、`WasamiEnemy06Chase.*`、`WasamiEnemyZone2.*`、`WasamiEnemySentry.*`、`WasamiViewcone.*`、`WasamiEnemyAnimInstance.*`、`Tests/WasamiEnemyTests.cpp`、`Tests/WasamiTestListener.h`、`WasamiCapture.*`、`Tests/WasamiCaptureTests.cpp` | 07 |
| `Source/wasami_deception/WasamiPowerTypes.*`、`WasamiPowerComponent.*`、`WasamiEnemyInterface.h`、`WasamiTelekinesisInterface.h`、`WasamiCameraAnim.*`、`WasamiChameleonComponent.*`、`WasamiSpeedBoostWidget.*`、`WasamiTeleportAim.*`、`WasamiPowerBurst.*`、`WasamiPrimalPower.*`、`WasamiTelekinesisPower.*`、`WasamiVanishPower.*`、`WasamiVanishWidget.*`、`WasamiTelepathyPower.*`、`WasamiTelepathyTracker.*`、`WasamiTelepathyTrackerWidget.*`、`Tests/WasamiPowerTests.cpp`、`Tests/WasamiCameraAnimTests.cpp`、`Tests/WasamiTestEnemy.*` | 04 |

## これから増える記録（予定）

| 記録 | 内容 |
| --- | --- |
| 05-interact | 視線の手のマーク（左クリックの調べる処理もここ。テレポーテーションは 04） |
| 10-audio | 曲・効果音・声・減衰 |
