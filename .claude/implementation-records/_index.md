# 実装記録 索引

運用ルールは `.claude/guides/implementation-records.md`。同期チェックは `python .claude/scripts/check_records.py`。

## 記録一覧

| 記録 | 内容 |
| --- | --- |
| [00-overview.md](00-overview.md) | 全体像。モジュールとビルド、プラグイン、`Config/` の設定（露出・静的ライティング・レイトレース・既定のマップとゲームモード・Python）、灯の焼き込み（原作と同じ High 品質）と実機との比較、作業の流れ |
| [01-stage-pipeline.md](01-stage-pipeline.md) | 取り込みの仕組み。病院ステージの前処理 `Tools/dd/prepare_stage.py`（テレポートのゾーンを含む）、レベルの組み立て（シャードの配置を含む）、ツールセット `WasamiDDTools` / `WasamiStageTools` / `WasamiDevTools`、本家のアセットを原作データから作り直す仕組み（Cascade のパーティクルを組む `UWasamiCascadeLibrary`、SoundCue を組む `UWasamiSoundCueLibrary`、材質のインスタンスの静的マスクを書く `UWasamiMaterialLibrary` の C++ の道具を含む）、本作の素材（`SourceArt/`）の取り込み、リモート実行とエディタの開き直し |
| [02-player.md](02-player.md) | プレイヤーとゲームモード。カプセルとカメラ、Enhanced Input（テレポートへ渡す左クリックとホイールを含む）、歩き・ダッシュ・ブースト、速さに連動する FOV、頭の揺れ、180° ターン、タブレットの出し入れとミニマップのキャプチャ |
| [03-tablet.md](03-tablet.md) | タブレットの画面（`UWasamiTabletWidget`。パワーの枠の出し分けと弾み、シャードの回収の Count Shake）、素材の取り込み（`dd_tablet.py`）、ミニマップの仕掛け |
| [04-powers.md](04-powers.md) | タブレットのパワー（`UWasamiPowerComponent`）。左右の枠と Q / E / 1 / 2、2 段の連打防止、ゲージ、強化段階の値、死亡のリセット、スピードブーストとその演出（赤い色調・集中線とビネット・画面の揺れ）、テレポーテーション（旧版。照準のアクタ `AWasamiTeleportAim`・移動・取り消し・再使用・カメラアニメ）、一瞬の演出の共通の基底 `AWasamiPowerBurst` と Primal Fear（`AWasamiPrimalPower`）、テレキネシス（`AWasamiTelekinesisPower`。半径の中のシャードを引き寄せる。力場の粒子と推定の材質）、Vanish（`AWasamiVanishPower`・画面の `UWasamiVanishWidget`・カプセルの Camera 応答）、Telepathy（`AWasamiTelepathyPower`・敵ごとの画面空間の印 `AWasamiTelepathyTracker` と `UWasamiTelepathyTrackerWidget`）、仮の的 `AWasamiTestEnemy`、UE4 の CameraAnim の再生（`UWasamiCameraAnim`。PP と FOV のトラック）、プレイヤーの FX（本家の Chameleon）、敵とシャードのインターフェース |
| [06-game-flow-save.md](06-game-flow-save.md) | ゲームの流れ。いまはシャード（`AWasamiShard`: 本作のワサミ餅・紫の灯・拾うカプセル・地図の印・回転、触れて回収〈数の −1・Count Shake・揺れ・閃光・音〉、テレキネシスの引き寄せ）と回収の閃光 `P_ky_flash3`（推定の材質 5 つ）、その素材の取り込み（`dd_shards.py`）。チェックポイント・セーブ・ライフ・死亡・脱出はこれから |

## ソース → 記録 対応表

| ソース | 記録 |
| --- | --- |
| `wasami_deception.uproject`、`.mcp.json`、`Config/*.ini` | 00 |
| `Source/wasami_deception.Target.cs`、`Source/wasami_deceptionEditor.Target.cs` | 00 |
| `Source/wasami_deception/wasami_deception.Build.cs`、`wasami_deception.cpp`、`wasami_deception.h`、`WasamiAssets.h` | 00 |
| `Tools/dd/prepare_stage.py`、`Tools/ue_remote.py`、`Tools/editor_cycle.py`、`Tools/console_session.py`、`Tools/desktop.py`、`Tools/desktop_agent.py` | 01 |
| `Content/Python/init_unreal.py`、`Content/Python/wasami_tools/**` | 01 |
| `Source/wasami_deception/WasamiCascadeLibrary.*`、`Tests/WasamiCascadeTests.cpp`、`WasamiSoundCueLibrary.*`、`WasamiMaterialLibrary.*` | 01 |
| `Source/wasami_deception/WasamiGameMode.*`、`WasamiPlayerCharacter.*` | 02 |
| `Source/wasami_deception/WasamiTabletWidget.*`、`Content/Python/wasami_tools/pipeline/dd_tablet.py` | 03 |
| `Source/wasami_deception/WasamiShard.*`、`Tests/WasamiShardTests.cpp`、`Content/Python/wasami_tools/pipeline/dd_shards.py`、`SourceArt/Wasami/wasami_mochi.glb` | 06（`dd_shards.py` は 01 にも載せる） |
| `Source/wasami_deception/WasamiPowerTypes.*`、`WasamiPowerComponent.*`、`WasamiEnemyInterface.h`、`WasamiTelekinesisInterface.h`、`WasamiCameraAnim.*`、`WasamiChameleonComponent.*`、`WasamiSpeedBoostWidget.*`、`WasamiTeleportAim.*`、`WasamiPowerBurst.*`、`WasamiPrimalPower.*`、`WasamiTelekinesisPower.*`、`WasamiVanishPower.*`、`WasamiVanishWidget.*`、`WasamiTelepathyPower.*`、`WasamiTelepathyTracker.*`、`WasamiTelepathyTrackerWidget.*`、`Tests/WasamiPowerTests.cpp`、`Tests/WasamiCameraAnimTests.cpp`、`Tests/WasamiTestEnemy.*` | 04 |

## これから増える記録（予定）

| 記録 | 内容 |
| --- | --- |
| 05-interact | 視線の手のマーク（左クリックの調べる処理もここ。テレポーテーションは 04） |
| 07-enemies | ワサミの敵、巡回・発見・追跡・グリッチ・捕獲 |
| 08-stage-gimmicks | 扉 6 種・罠の扉・噴出・車・街灯・脱出のトラックと群れ |
| 09-ui | タイトル・OPTIONS・ポーズ・死亡・脱出の画面・字幕・SAVING |
| 10-audio | 曲・効果音・声・減衰 |
