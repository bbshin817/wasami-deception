---
title: UE5 版の立ち上げ（引き継ぎ調査・運用ルール・ステージの取り込み）
branch: master
base: 0ebb8f3
started: 2026-09-16
---

## 依頼

> 本プロジェクトは、次のプロジェクトを引き継ぎ、Dark Deception のワサミ版ファンゲームを構築することです。"C:\Users\User\Downloads\wasami-deseption" このプロジェクトでは WebGL 版として進行していましたが、さらなるクオリティの向上を目指して、本プロジェクトへバトンを渡しました。このプロジェクトでは、UE5 を MCP 経由で操作して実装を進め、最終的に完全に遊べるものを作成することが目標です。何を継承すべきか、引き継ぐリソースや方向性などを調査し、着手してください

## 計画

| # | ステップ | 状態 | 変更するもの |
| --- | --- | --- | --- |
| 1 | 引き継ぎ調査と運用ルール | 完了 | `CLAUDE.md`、`.claude/guides/*`、`.claude/references/handover.md` |
| 2 | ステージの前処理 | 完了 | `Tools/cc2/prepare_stage.py`、`Tools/ue_remote.py` |
| 3 | プロジェクトの Python ツールセット（取り込みと組み立て） | 完了 | `Content/Python/init_unreal.py`、`Content/Python/wasami_tools/` |
| 4 | CC2 の取り込み（メッシュ 68・テクスチャ 286・マテリアル 157） | 完了 | `/Game/CC2/…`、`/Game/Pipeline/…` |
| 5 | ステージのレベルの組み立て（配置・灯・ボリューム・霧・空・キャプチャ）と見た目の確認 | 作業中 | `/Game/Stage/Maps/L_Zone1` |
| 6 | 本家のカメラシェイク（歩き・走りの頭の揺れ）を `/Game/DD/…` に作る | 完了 | `Content/Python/wasami_tools/pipeline/dd_assets.py`、`/Game/DD/Blueprints/Main/` |
| 7 | プレイヤーの C++ の骨組み（本家の値の一人称の移動・FOV・頭の揺れ・180°・ブースト） | コードを書いた（未ビルド） | `Source/wasami_deception/WasamiGameMode.*`、`WasamiPlayerCharacter.*` |
| 8 | プロジェクト設定（ハードウェアのレイトレースを切る、LiveCodingToolset、既定のマップとゲームモード）、エディタを閉じて C++ をビルドし開き直す（ユーザーに確認） | 未着手 | `Config/DefaultEngine.ini`、`wasami_deception.uproject` |

## 決定事項

- 2026-09-16: ユーザーの回答: (1) エディタは Claude が閉じて開き直してよい、(2) 取り込んだ素材など作り直せるものは git の外、手作りのアセットは Git LFS、(3) コミットと push は WebGL 版と同じ運用（実装ごとにコミット、条件を満たすと自動で push）、(4) 参照データ（pak_reference・pak_reference_2・cc2_reference）はこのリポジトリの直下へ移す。→ 3 つとも WebGL 版から移し、`.gitignore` に足した。WebGL 版の派生データ（assets-src/cc2/layout.json・stage.json・tex）は WebGL 版の場所から読む。
- 2026-09-16: リモート（bbshin817/wasami-deception-native）は空だった。ローカルのブランチ名を master から main にし、WebGL 版と同じく main を push する。
- 2026-09-16: エディタで Python を動かすため、Project Settings の Python の Remote Execution と Developer Mode を MCP（ConfigSettingsToolset）で有効にした（`Config/DefaultEngine.ini`）。リモート実行はローカルのマルチキャストだけ。
- 2026-09-16: CC2 の書き出しの glb は Interchange で取り込むと元の UE のメッシュ空間に戻る（ATM・Blocker_Cube_001 の境界が cook されたメッシュと一致）。配置は WebGL 版の layout.json の行列を UE に戻して使う（最大の誤差 0.0001 cm/m）。
- 2026-09-16: 取り込みは glTF の既定のパイプラインを複製した自前のパイプライン（種類ごとのサブフォルダなし、マテリアルとテクスチャを取り込まない）で、区画ごとに S0, S1, … のマテリアルを付け直した glb から行う（書き出しの glb は区画が同じマテリアルを共有していてスロットが潰れるため）。
- 2026-09-16: アセットの置き場所は CC2 の /Game の木をそのまま `/Game/CC2/` に写す（名前が元のプロジェクトどおり一意になる）。
- 2026-09-16: 最初の組み立てで多くの面が既定のマテリアル（灰の格子）になった。原因はマスターマテリアルの ORM の既定テクスチャ（エンジンの sRGB の `WhiteSquareTexture`）が Masks のサンプラーに合わず、ORM を持たないインスタンスがすべてコンパイルに失敗したこと（ログの `Sampler type is Masks, should be Color` 1,822 件）。ORM の既定を線形の白 `T_Default_Masks` にし、マスターを版付きで作り直す（`MASTER_VERSION`）。あわせて取り込みが推測したテクスチャの圧縮（UI 用 15 枚、法線と誤認 1 枚）を用途ごとに直す（`refresh_cc2_asset_settings`）。

## 次にやること

- ステップ 3: ツールセットを書き、エディタに登録して MCP から呼べることを確かめる。

## 再開時の注意

- エディタは `/Temp/Untitled_1`（保存していない空のレベル）を開いている。
- `/Game/Scratch/PL_Test` は取り込みの試験で作った保存していないパイプライン（読み込み中で消せなかった）。エディタを開き直せば消える。残っていたら消す。
- ツールセットはエディタの起動後に作ったので、リモート実行で `sys.path` に `Content/Python` を足して登録した。次の起動からは `init_unreal.py` が登録する。
- `Intermediate/Pipeline/cc2/` は `python Tools/cc2/prepare_stage.py` で作り直せる（数十秒）。
