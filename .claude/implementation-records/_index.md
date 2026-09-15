# 実装記録 索引

運用ルールは `.claude/guides/implementation-records.md`。同期チェックは `python .claude/scripts/check_records.py`。

## 記録一覧

| 記録 | 内容 |
| --- | --- |
| [00-overview.md](00-overview.md) | 全体像。モジュールとビルド、プラグイン、`Config/` の設定（Lumen・レイトレース・既定のマップとゲームモード・Python）、作業の流れ |
| [01-stage-pipeline.md](01-stage-pipeline.md) | 取り込みの仕組み。前処理 `Tools/cc2/prepare_stage.py`（UE への座標の戻し、区画ごとの glb）、ツールセット `WasamiStageTools` / `WasamiDDTools` / `WasamiDevTools`、マスターマテリアルとテクスチャの設定、ステージのレベルの組み立て、リモート実行とエディタの開き直し |
| [02-player.md](02-player.md) | プレイヤーとゲームモード。カプセルとカメラ、Enhanced Input、歩き・ダッシュ・ブースト、速さに連動する FOV、頭の揺れ、180° ターン |

## ソース → 記録 対応表

| ソース | 記録 |
| --- | --- |
| `wasami_deception.uproject`、`.mcp.json`、`Config/*.ini` | 00 |
| `Source/wasami_deception.Target.cs`、`Source/wasami_deceptionEditor.Target.cs` | 00 |
| `Source/wasami_deception/wasami_deception.Build.cs`、`wasami_deception.cpp`、`wasami_deception.h` | 00 |
| `Tools/cc2/prepare_stage.py`、`Tools/ue_remote.py`、`Tools/editor_cycle.py` | 01 |
| `Content/Python/init_unreal.py`、`Content/Python/wasami_tools/**` | 01 |
| `Source/wasami_deception/WasamiGameMode.*`、`WasamiPlayerCharacter.*` | 02 |

## これから増える記録（予定）

| 記録 | 内容 |
| --- | --- |
| 03-interact-teleport-tablet | 視線の手のマーク、テレポーテーション、タブレット（本家の値） |
| 04-game-flow-save | シャード・チェックポイント・セーブ・ライフ・死亡・脱出 |
| 05-enemies | ワサミの敵、巡回・発見・追跡・グリッチ・捕獲 |
| 06-stage-gimmicks | 扉 6 種・罠の扉・噴出・車・街灯・脱出のトラックと群れ |
| 07-ui | タイトル・OPTIONS・ポーズ・死亡・脱出の画面・字幕・SAVING |
| 08-audio | 曲・効果音・声・減衰 |
