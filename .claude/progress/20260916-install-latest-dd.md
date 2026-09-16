---
title: 本家の最新版を手元に入れて旧版と同居させる
status: 進行中
branch: main
base: 30e291d
started: 2026-09-16 12:55
updated: 2026-09-16 12:55
---

# 本家の最新版を手元に入れて旧版と同居させる

## 依頼

ユーザーの指示（2026-09-16）: 「最新版と旧版が同居できるなら、最新版入れておきましょうか！」

- 旧版（Dark Deception Classic Ch3 = `pak_reference/` と同一のビルド）は壊さずに残す。
- 目的は、最新版でしか観測できないもの（病院「Torment Therapy」のステージ、新しい版のタブレットの特殊効果）を実機で見られるようにすること。

## 作業前の状態（2026-09-16 に確認）

- Steam の app 332950 は 1 つしか登録できない。現在の `steamapps/appmanifest_332950.acf` は旧版を指している（`installdir = Dark Deception Classic Ch3`、`buildid 3951230`）。
- **危険**: その live のマニフェストは Steam に書き換えられて `StateFlags 6`（更新が必要）・`TargetBuildID 7641935`・`ScheduledAutoUpdate` 付きになっていた。この状態で Steam が更新すると、`installdir` が旧版のフォルダなので**旧版が最新版で上書きされる**。
- 最新版の途中までのダウンロードが取り置かれている: `steamapps/332950-latest-partial-20260911-233356.disabled`（8.0 GB）と `steamapps/appmanifest_332950.latest-partial-20260911-233356.acf.disabled`（`installdir = Dark Deception`、`BytesDownloaded 7,185,870,208 / BytesToDownload 8,386,179,824` ≒ 86 %）。
- `steamapps/common/Dark Deception` は空のフォルダとして存在（最新版の入り先）。`steamapps/common/Dark Deception Classic Ch3` が旧版（1.9 GB）。
- C: の空きは 201 GB。Steam も DDeception も起動していない。
- 旧版の綺麗なマニフェストの控え: `C:\Users\User\AppData\Local\DDeception\steam-appmanifest-332950-classic.acf`（`StateFlags 4`・`TargetBuildID 3951230`）。これを live に戻せば旧版の登録に戻せる。
- 旧版のランチャ `Launch-Classic-Ch3.cmd` は exe を直接起動するので、Steam の登録がどちらを指していても動く（必要なのは Steam が起動していることと DLC の owned + installed）。

## 計画

- [x] 1. 状態の確認（上に記録）
- [ ] 2. セーブのバックアップ（`%LOCALAPPDATA%\DDeception\Saved` を丸ごと。旧版のセーブは能力の解放を当てた状態） ← 作業中
- [ ] 3. Steam の登録を最新版に入れ替える（Steam を止めた状態で、live のマニフェストを退避 → `*.latest-partial-*.acf.disabled` を live に戻す、`332950-latest-partial-*.disabled` を `steamapps/downloading/332950` に戻す）
- [ ] 4. Steam をコンソールのセッションで起動し、ダウンロードの再開を `.acf` の `BytesDownloaded` / `BytesStaged` で追う（残り約 1.2 GB + 検証）
- [ ] 5. 完了の確認（`common/Dark Deception/DDeception/Content/Paks/*.pak` の pak 版とファイル数を `pak_reference_2/_manifest.json` と比べる、`06_Hospital*` があることを確かめる）
- [ ] 6. 最新版のランチャを旧版と同じ形で用意する（`Launch-Latest.cmd` / `.ps1`）
- [ ] 7. ルールを 2 つのビルド前提に直す（`.claude/guides/original-fidelity.md`・`verification.md`・`CLAUDE.md`・`.claude/references/handover.md`）

## 次にやること

ステップ 2 から。セーブを退避したら、Steam が止まっていることを確かめてマニフェストと部分ダウンロードを入れ替える。

## 決定事項

- 2026-09-16: 同居は「Steam の登録は最新版（`installdir = Dark Deception`）、旧版は `Launch-Classic-Ch3.cmd` で exe を直接起動」という形にする。これなら Steam が更新しても旧版のフォルダに触らないので、今より安全（今の live は旧版のフォルダを指したまま更新待ちで危ない）。
- 2026-09-16: 2 つのビルドは**セーブの置き場所を共有する**（どちらも `%LOCALAPPDATA%\DDeception\Saved`）。最新版が旧版のセーブを書き換える可能性があるので、先に丸ごとバックアップし、ルールにも「ビルドを行き来するときはセーブを入れ替える」と書く。

## 再開時の注意

- ダウンロードは Steam が勝手に進める。進み具合は `C:\Program Files (x86)\Steam\steamapps\appmanifest_332950.acf` の `BytesDownloaded` / `BytesStaged` / `StateFlags` を読む（`StateFlags 4` で完了、`6` は更新待ち、`1026` は更新中）。
- 途中で止まっているときは Steam を一度終了して起動し直す（`steam.exe -shutdown` → スケジュールタスクで起動）。Steam を止めずに `.acf` を書き換えない（Steam が上書きする）。
- 旧版の登録に戻すには、Steam を止めて `C:\Users\User\AppData\Local\DDeception\steam-appmanifest-332950-classic.acf` を `steamapps/appmanifest_332950.acf` に上書きコピーする。
- Claude はセッション 0 にいるので Steam もゲームも直接起動できない。`Tools/editor_cycle.py` の `start_editor` と同じスケジュールタスクの手を使う。

## 検証

- check_records: 未実行（リポジトリのソースは変えていない）
- ダウンロードの完了確認: 未実行
- 最新版の pak と `pak_reference_2` の一致確認: 未実行
