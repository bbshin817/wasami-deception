# WASAMI DECEPTION（UE5 版）— Claude Code 索引

Dark Deception のワサミ版ファンゲームの UE 5.8 版。WebGL 版（`C:\Users\User\Downloads\wasami-deseption`、以下 `<WEBGL>`）の後継で、エディタを MCP 経由で操作して作る。目標は最初から最後まで遊べるゲーム。
このファイルは `.claude/` 配下の索引だけを持つ。本文はここに書かず、各ファイルを参照する。

## 必読（作業前）
- 対話と報告の言語（**セッション内の対話・応答・実装報告はすべて日本語**）: `.claude/guides/communication.md`
- 実装進捗記録（**ファイルやアセットの変更を伴う作業は `.claude/progress/` に進捗を先に書いてから進める。セッション開始・/clear・圧縮の後は、まず未完了の記録と main 以外のローカルブランチを確認して再開する**）: `.claude/guides/progress-tracking.md`
- エディタの操作（**MCP のツールは 1 つずつ順に呼び、結果を必ず確かめる。一括の変更の前後で保存する。取り込みとステージの組み立ては `Content/Python/wasami_tools` のツールセットを MCP から呼ぶ。C++ のビルドの手順**）: `.claude/guides/unreal-workflow.md`
- 原作への忠実さ（**本家 Dark Deception の原作データ `pak_reference/` に忠実に倣う。ステージだけはファンゲーム『Chaotic Customer 2』の Zone_1（`cc2_reference/`）。本家に無いギミックは複雑にせず、壊せる物は本家のホテルの板張りのバリケードのように 1 クリックで崩れて消える。原作のロゴとキャラクターのモデルは使わない。UE に同じ仕組みがあれば値を写すだけにし、作り直さない**）: `.claude/guides/original-fidelity.md`
- コミットとブランチ（**実装ごとにコミット、大規模改修は作業ブランチ→main へマージ→ローカルブランチ削除、最終コミットから 10 分経過・未 push 2 件以上・大規模改修のマージ後のいずれかで main を push。参照データから作り直せる素材は git の外、手作りのアセットは Git LFS**）: `.claude/guides/git-workflow.md`

## 参考資料
- WebGL 版からの引き継ぎ調査（何を持ち越し、何を UE に置き換えるか、マイルストーン、ユーザーの決定）: `.claude/references/handover.md`
- WebGL 版の仕様と実装記録（ゲームの流れ・値・ユーザーの決定の経緯。**ゲームの規則の正本**）: `<WEBGL>/README.md`、`<WEBGL>/.claude/implementation-records/`（00 が全体像）
- 本家の原作データ（UE 4.21。**実装値の根拠として最優先**）: `pak_reference/README.md`
- 本家の新しい版（UE 4.24。pak_reference に無いものの根拠。両方にあって違うものはユーザーに確認）: `pak_reference_2/README.md`
- ステージの原作データ（CC2 の書き出し。**ステージの実装値の根拠として最優先**）: `<WEBGL>/.claude/references/chaotic-customer-2/README.md`
- 本家の用語・敵・仕組み・演出と本作との対応（Web 調査。[A]/[B] だけを値の根拠にする）: `<WEBGL>/.claude/references/dark-deception/README.md`

## 構成
- `Source/wasami_deception/` … ゲームの C++ モジュール
- `Content/Python/` … エディタの Python（`init_unreal.py` がプロジェクトのツールセットを登録する）
- `Tools/` … エディタの外で動くスクリプト（参照データの前処理 `Tools/cc2/`、リモート実行 `Tools/ue_remote.py`）
- `Intermediate/Pipeline/` … 前処理の出力（git の対象外、作り直せる）

## よく使うコマンド
- ステージの前処理: `python Tools/cc2/prepare_stage.py`
- エディタで Python を実行: `python Tools/ue_remote.py <file.py>`
- C++ のビルド（エディタを閉じて）: `& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" wasami_deceptionEditor Win64 Development "-Project=C:\Users\User\Desktop\wasami_deception\wasami_deception.uproject" -WaitMutex`
