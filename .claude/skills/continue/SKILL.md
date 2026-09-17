---
name: continue
description: 中断した実装の続きを始める。.claude/progress/ の未完了の記録と git・エディタの実際の状態を照合してから、記録の「次にやること」に書かれたステップを 1 つ実行する。セッションの開始時、/clear の後、コンテキスト圧縮の後、「作業を続けて」「続きから」「再開して」と言われたときに使う。
---

# 中断した実装の続きを始める

`.claude/guides/progress-tracking.md` の「再開の手順」を実行する形にしたもの。**記録と git とエディタの状態だけから再開する**（前のセッションの会話は残っていない前提で進める）。

セッション開始時の hook（`.claude/scripts/session_start_hook.py`）が、未完了の進捗記録・main 以外のブランチ・未コミットの変更・直前のコミットを既に知らせている。その内容を出発点にする。

## 1. どこから再開するかを決める

```bash
ls .claude/progress/          # _template.md 以外があれば、それが未完了の作業
git branch                    # main 以外のローカルブランチも未完了の大規模改修かもしれない
git status --short
```

- **未完了の進捗記録がある** → その記録を読む。これが正本。
- **記録が無く、main 以外のブランチもない** → 前の実装は完了している。`.claude/references/handover.md` の「現状と次の一歩」を読み、次の一歩の 1 番目から始める。**新しい作業なので、まず進捗記録を作る**（`.claude/progress/_template.md` を写す。ファイル名は `<YYYYMMDD>-<slug>.md`）。
- **記録は無いのに main 以外のブランチや未コミットの変更がある** → 記録されていない作業。ユーザーの手による変更かもしれないので**触らずに確認する**。

読む順は「進捗記録 → 記録が名指しする実装記録（`.claude/implementation-records/`）→ handover」。ガイド（`.claude/guides/`）は必要になった節だけを読む。コンテキストを使い切らないよう、関係しないゾーン・記録は読まない。

## 2. 記録と実際の状態を照合する

記録の `base`（開始時の HEAD）と計画のチェックボックスが、実際の git とアセットに合っているかを確かめる。

```bash
git log --oneline <base>..HEAD      # 記録の base から何が入ったか
git status --short
git log -1 --format=%cr             # 直前のコミットからの経過（push の判断に使う）
git status -sb                      # ahead N = 未 push の件数
```

エディタ側は、記録の「再開時の注意」に書かれた出力（取り込んだアセット、組み立てたレベル、走らせたままの処理）を実際に確かめる。

```bash
python Tools/ue_remote.py -c "
import unreal
def main():
    print('level:', unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name())
    print('in PIE:', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor())
    print('dirty:', [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
main()
"
```

**食い違いを見つけたら、git・実ファイル・アセットの実際の状態を正として記録を直す。** どちらが正しいか決められないときはユーザーに確認する。**変更を捨てる操作（`git checkout --` / `restore` / `reset --hard` / `stash`、アセットの削除）は、必ず先にユーザーの確認を取る。**

## 3. ベースラインを確かめる

- **エディタが起動して応答するか**: 上の `ue_remote.py` が答えれば起きている。答えなければ `tasklist | grep -i UnrealEditor` で確認し、起動していなければ `python Tools/editor_cycle.py`（Claude は Windows のセッション 0 にいるので `UnrealEditor.exe` を直接起動しても落ちる）。エディタを閉じる・開き直すのに確認は要らない（`.claude/guides/verification.md`。閉じる前に保存する）。
- **PIE が残っていたら止める**（`editor_request_end_play`）。残っているとアセットの操作が `The Editor is currently in a play mode.` で失敗する。
- **MCP が応答しないとき**は `Tools/ue_remote.py` で作業できる（この PC は Docker Desktop が 127.0.0.1:8000 を掴むため、エディタを開き直すと MCP の接続が切れる。`/mcp` で再接続する）。
- **C++ を変える作業なら**、ビルドが通る状態から始める（`python Tools/editor_cycle.py`）。

## 4. ステップを 1 つ実行する

進捗記録の「計画」から、**未完了のステップを 1 つだけ**取る。1 ステップは 1 コミットで終わる大きさ。

1. そのステップを記録で「作業中」にし、**変更する予定のファイルとアセット**（`/Game/...`）を書く。
2. 実装する。時間のかかる処理（取り込み、レベルの組み立て、ライティングのビルド、シェーダーのコンパイル、C++ のビルド）は**先に「再開時の注意」へ、実行する手順・出力先・完了の確かめ方を書いてから**走らせる。バックグラウンドで走らせ、待つ間に別の作業を進める。
3. 判断したこと・ユーザーに確認して決めたことは、その場で「決定事項」に理由つきで書く。
4. 検証する（`.claude/guides/verification.md`。PIE・エディタの開き直し・本家の起動に確認は要らない。PIE は終わったら必ず止める）。
5. ソースを変えたなら対応する実装記録を直し、`python .claude/scripts/check_records.py --update` を通す。
6. 記録のステップを完了にし、「次にやること」を次のステップに更新してから、**記録も一緒にコミットする**。
7. push の条件（未 push 2 件以上 / 最終コミットから 10 分以上 / 大規模改修のマージ後）を満たしていれば main を push する。

## 5. ステップが終わったら

- **まだ残りのステップがある** → ここまでを記録に残し、**ユーザーに `/clear` をお願いする**（何が終わったか・次に何をするか・再開のために読むファイルを 1〜3 行添える）。会話が長いほど精度が落ちるので、続きは新しいセッションで始める。
- **作業がすべて終わった** → 実装記録と `handover.md` の「現状と次の一歩」を直し、**進捗記録を削除して**最後のコミットに含める（経緯は git 履歴に残る）。そのうえで `/clear` をお願いする。

## 迷ったら

| 状況 | どうするか |
| --- | --- |
| 記録の計画が今の状況に合わない | 計画を書き換え、理由を「決定事項」に書く。大きく変わるならユーザーに確認する |
| ステップが 1 コミットに収まらないと分かった | その場でステップを分け直して記録に書く |
| 記録に無いファイル・アセットの変更がある | ユーザーの作業かもしれないので触らず、報告して確認する |
| 中間状態が残る処理の途中で止まっていた | 「再開時の注意」の完了の確かめ方で今どこまで進んだかを判定してから、続きか、やり直しかを決める |
