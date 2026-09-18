---
title: 読み込み画面の紋章（要確認の回答の反映）
status: 進行中
branch: fix/loader-emblem   # main から。作業ツリーは scratchpad/wt。終わったら main を早送りし、feature/enemy-ai へマージする
base: 784c37c
started: 2026-09-19 03:32
updated: 2026-09-19 03:55
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 読み込み画面の紋章（要確認の回答の反映）

## 依頼

2026-09-19 の有人セッションで、作業一覧の「未回答の要確認」に答えがあった。

- 項目 6 の読み込み画面の紋章: 「`tmp\wasami_symbol.png` をシンボルとし、魔法陣的なアセットは本家から流用して」。
- 項目 6 の「ガレージリフトで Zone 2 へ」を救急車で移る形に読み替えたこと: 「承認」。

## 計画

- [ ] 1. 紋章を作って出す ← 作業中
  - 変更予定: `SourceArt/Wasami/UI/wasami_symbol.png`（ユーザーのシンボル。Git LFS）、`.gitattributes`、`Tools/dd/prepare_loader.py`（新規。本家の `loader_reapernurse` の魔法陣の内側の印を消し、シンボルを合成して `Intermediate/Pipeline/wasami/ui/loader_wasami.png` に書く）、`Content/Python/wasami_tools/pipeline/dd_ui.py`（`import_loading` が `/Game/Wasami/UI/loader_wasami` を取り込む）、`Source/wasami_deception/WasamiLoadingWidget.*`（`LevelEmblems` の 7 番の既定）、テスト `Wasami.GameFlow.Loading`、実装記録 09・01、`.claude/roadmap.md`（項目 6 の状態・未回答の要確認）、`.claude/guides/original-fidelity.md`（原作の素材の使用範囲）
  - 確かめ: `feature/enemy-ai` へマージした作業コピーで前処理 → ビルド → 取り込み → テスト → 画面で見る。通ったら main をこのブランチまで早送りする。

## 次にやること

変更は `fix/loader-emblem` にコミット済み。本来の作業コピー（`feature/enemy-ai`）で `git merge fix/loader-emblem` → `python Tools/dd/prepare_loader.py` → `python Tools/editor_cycle.py` → リモート実行で `dd_ui.import_loading()` → Automation `Wasami.GameFlow.Loading` → PIE で `unreal.WasamiLoadingWidget.show(<ゲームのワールド>, 7)` を撮って見る → `check_records.py` が OK。通ったら main を `fix/loader-emblem` へ早送りし、記録を消し、ブランチと作業ツリーを片付けて push。

## 決定事項

- 2026-09-19: 本家の紋章 9 枚は外の魔法陣（外の輪・文字の帯・小円 3 つ）が同じで、内側の黒い円の中の印だけが違う。`loader_reapernurse` の内側の印を消し（黒で埋める）、そこにシンボルを置く。シンボルの赤は本家の印と同じ赤にそろえる。
- 2026-09-19: 合成はエディタの外の前処理（システムの Python と PIL）で行う。エディタの Python には PIL が無い。合成した画像は本家の素材を含むので git に入れず、`Intermediate/Pipeline/` に書く（`prepare_stage.py` と同じ扱い）。
- 2026-09-19: 敵の AI の作業と関係がないので、`.claude/guides/autonomy.md` の「不具合の改善」に沿って main 側で直す。エディタと作業コピーを `feature/enemy-ai` のままにするため、作業ツリーで書いて `feature/enemy-ai` へマージし、そこで確かめる。

## 要確認（ユーザー）

- （なし）

## 再開時の注意

- 作業ツリー: `C:/Users/User/AppData/Local/Temp/claude/c--Users-User-Desktop-wasami-deception/359f914f-6f38-4af8-9ef8-80dcc7444c4f/scratchpad/wt`（`git worktree list`）。LFS の中身は取り出していない（`GIT_LFS_SKIP_SMUDGE=1`）ので、そこで .uasset を触らない。消えていたら `git worktree prune` してブランチ `fix/loader-emblem` から作り直す。
- 作業ツリーでは glb が LFS のポインタのままなので、`check_records.py` は `06` の `wasami_mochi.glb` と `07` の glb 2 つをずれと言う（`--update` を作業ツリーで走らせない。走らせたら glb の 3 件を戻す）。本来の作業コピーでは OK になるはず。
- 本来の作業コピーの `.claude/progress/20260919-enemy-ai.md` に、中断した無人運転の反復 18 の未コミットの 2 行（ステップ 5b を作業中に）がある。消さない。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
