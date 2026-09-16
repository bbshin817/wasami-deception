# 実装記録 索引

運用ルールは `.claude/guides/implementation-records.md`。同期チェックは `python .claude/scripts/check_records.py`。

## 記録一覧

| 記録 | 内容 |
| --- | --- |
| [00-overview.md](00-overview.md) | 全体像。モジュールとビルド、プラグイン、`Config/` の設定（露出・静的ライティング・レイトレース・既定のマップとゲームモード・Python）、灯の焼き込み（原作と同じ High 品質）と実機との比較、作業の流れ |
| [01-stage-pipeline.md](01-stage-pipeline.md) | 取り込みの仕組み。病院ステージの前処理 `Tools/dd/prepare_stage.py`、ツールセット `WasamiDDTools` / `WasamiDevTools`、本家のアセットを原作データから作り直す仕組み、リモート実行とエディタの開き直し |
| [02-player.md](02-player.md) | プレイヤーとゲームモード。カプセルとカメラ、Enhanced Input、歩き・ダッシュ・ブースト、速さに連動する FOV、頭の揺れ、180° ターン、タブレットの出し入れとミニマップのキャプチャ |
| [03-tablet.md](03-tablet.md) | タブレットの画面（`UWasamiTabletWidget`。パワーの枠の出し分けと弾み）、素材の取り込み（`dd_tablet.py`）、ミニマップの仕掛け |
| [04-powers.md](04-powers.md) | タブレットのパワー（`UWasamiPowerComponent`）。左右の枠と Q / E / 1 / 2、2 段の連打防止、ゲージ、強化段階の値、死亡のリセット、スピードブースト、敵とシャードのインターフェース |

## ソース → 記録 対応表

| ソース | 記録 |
| --- | --- |
| `wasami_deception.uproject`、`.mcp.json`、`Config/*.ini` | 00 |
| `Source/wasami_deception.Target.cs`、`Source/wasami_deceptionEditor.Target.cs` | 00 |
| `Source/wasami_deception/wasami_deception.Build.cs`、`wasami_deception.cpp`、`wasami_deception.h`、`WasamiAssets.h` | 00 |
| `Tools/dd/prepare_stage.py`、`Tools/ue_remote.py`、`Tools/editor_cycle.py`、`Tools/console_session.py`、`Tools/desktop.py`、`Tools/desktop_agent.py` | 01 |
| `Content/Python/init_unreal.py`、`Content/Python/wasami_tools/**` | 01 |
| `Source/wasami_deception/WasamiGameMode.*`、`WasamiPlayerCharacter.*` | 02 |
| `Source/wasami_deception/WasamiTabletWidget.*`、`Content/Python/wasami_tools/pipeline/dd_tablet.py` | 03 |
| `Source/wasami_deception/WasamiPowerTypes.*`、`WasamiPowerComponent.*`、`WasamiEnemyInterface.h`、`WasamiTelekinesisInterface.h`、`Tests/WasamiPowerTests.cpp` | 04 |

## これから増える記録（予定）

| 記録 | 内容 |
| --- | --- |
| 05-interact | 視線の手のマーク（テレポーテーションは 04 に書き足す） |
| 06-game-flow-save | シャード・チェックポイント・セーブ・ライフ・死亡・脱出 |
| 07-enemies | ワサミの敵、巡回・発見・追跡・グリッチ・捕獲 |
| 08-stage-gimmicks | 扉 6 種・罠の扉・噴出・車・街灯・脱出のトラックと群れ |
| 09-ui | タイトル・OPTIONS・ポーズ・死亡・脱出の画面・字幕・SAVING |
| 10-audio | 曲・効果音・声・減衰 |
