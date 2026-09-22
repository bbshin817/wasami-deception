---
title: Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）
status: 進行中
branch: main
base: 055a05f
started: 2026-09-22 13:36
updated: 2026-09-22 15:05
---

# Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 41（大目標 4「レビュー指摘の修正」）。レビュアーの指摘（Medium）:

> Zone1 → Zone2への転換時、トンネルの中に救急車が2台存在し、プレイヤーが置いていかれる

本家は屋根に乗ったまま運ばれて読み込み画面に入る。完了の条件:

1. 組み立てた `L_Hospital_Zone1` の救急車のアクタを数え、本家の 5 台と重なりが無いことを確かめる。違えば組み立てを直す。
2. 置いていかれないようにする（屋根に乗っている間はプレイヤーを救急車に付ける / 後ろの壁を掃引で動かす / 走り出しでプレイヤーを屋根の中央へ寄せる のどれか）。**本家と違う作りにするので、選んだ理由を 11 記録に書く**。
3. パッケージ版で、屋根に乗ってから読み込み画面まで運ばれることを 2 回続けて確かめる。

## 計画

- [x] 1. 計画を立てる（完了の条件 (1) はここで済ませた） … 2026-09-22 完了。**本作の Zone 1 の救急車は本家と同じ 5 台で重なりも余分も無く、トンネルの中にあるのは走り出す `hospital_ambulance_new_teleport` の 1 台だけ**（駐車場の 4 台はトンネル口から 61 m 以上離れる）。
- [x] 2. 既存のパッケージ版でレビュアーと同じ絵を撮り、「2 台」と「置いていかれる」を再現する … 2026-09-22 完了。**置いていかれるのは 3 回とも再現、「2 台」は再現せず**（下の「決定事項」）。
- [ ] 3. 置いていかれるのを直す（完了の条件 (2)） ← 次
  - 変更予定: `Source/wasami_deception/WasamiZone1Flow.cpp`・`.h`、`Source/wasami_deception/Tests/WasamiZoneFlowTests.cpp`、`.claude/implementation-records/11-zone-flow.md`
- [ ] 4. 救急車を Movable で置く（「2 台」の唯一の見当。下の「決定事項」）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_level.py` か `dd_sequence.py`、`.claude/implementation-records/01-stage-pipeline.md`、Zone 1・Zone 2 の組み立て直し
- [ ] 5. パッケージ版で 2 回続けて確かめる（完了の条件 (3)）

## 次にやること

ステップ 3。**屋根の上のプレイヤーが走り出しの約 2 s 後に振り落とされる**のを直す（下の「決定事項」の測り）。直し方の候補は 3 つで、まず本家の作り（`AWasamiZone1Flow::On06ReachAmbulance`）と 11 記録の既知の制約を読んでから選ぶ:

- (A) 屋根に乗っている間はプレイヤーを救急車（`hospital_ambulance_new_teleport`）に付け、読み込み画面で外す。確実だが本家に無い作り。
- (B) 後ろの壁 `BlockingVolume_Ambulance_3` を掃引で動かす（シーケンスが動かすので、走り出しの間だけ別に動かすことになる）。
- (C) 走り出しでプレイヤーを屋根の中央へ寄せる … **今回の再現では中央（y −20060、壁から 2.5 m）に立たせても落ちた**ので、これだけでは足りない。

直した後、`python Intermediate/Overnight/probe_ambulance.py track`（下の「再開時の注意」）で `player=` の y が読み込み画面まで伸びる（z が 402 のまま）ことを見る。

## 決定事項

- 2026-09-22: **完了の条件 (1) は済み**（計画のステップ 1）。レベルの救急車は 5 台で本家（`pak_reference_2/_levels/06_Hospital_Zone_01.scene.json`）と一致、トンネルの中は 1 台だけ。前処理のシーケンスは `possessable` をレベルの実アクタへ解決し `spawn_template` は `null` なので、シーケンスが複製を生むこともない（`Content/Python/wasami_tools/pipeline/dd_sequence.py`）。走り出しのシーケンス `06_Hospital_Zone1_AmbulanceTakeOff` が動かすのも `hospital_ambulance_new_teleport` と スポットライト 2 つだけ、駐車場の場面 `06_Hospital_Zone1_06Event` は救急車を 1 台も動かさない（`pak_reference_2/_sequences/*.json` の `bindings`）。
- 2026-09-22: **「置いていかれる」はパッケージ版で確実に起きる（3 回とも）。** 屋根の中央に立たせて走り出させると、プレイヤーは最初の 1.77 s の区間（−19993 → −19083、約 514 cm/s）は屋根（`BlockingVolume_Ambulance_5` の上、z 402）に乗って運ばれるが、**速い区間（約 1940 cm/s）に入って約 0.5〜2 s で屋根から外れ、トンネルの路面（z 90）に落ちて y −17850〜−18840 で止まる**。救急車だけが去り、8 s 後に読み込み画面 → Zone 2 へ進む。`t.MaxFPS 60` で毎回起きるので、症状索引「収録中に救急車の屋根からプレイヤーが落ちる」が言う**収録の負荷や屋根の後ろの端に立ったときだけの話ではない**（あの記述は PIE のもの。今回は中央でも落ちた）。
- 2026-09-22: **「2 台」はパッケージ版（Windows）では再現しない。** 3 つの見方で撮って、どの瞬間も救急車は 1 台だけだった: (a) 走り出しの手前の路面から（トンネル口に 1 台、置き去りの影も残らない）、(b) 屋根の上からトンネルの奥を見て、(c) 落ちた後の路面から。駐車場の 4 台はトンネルからは見えない。**唯一の見当は救急車の可動性**（下）。レビュアーが遊んだのは Mac 版なので、**Mac でだけ出る描画の食い違い**の可能性が残る（ユーザーに見てもらう）。
- 2026-09-22: **救急車のメッシュの可動性が `Static` のままシーケンスに動かされている**（`Intermediate/Pipeline/dd/stage_ue.json` の `hospital_ambulance_new_teleport` の `props.Mobility = EComponentMobility::Static`。原作がそう置いているのを前処理がそのまま写している）。UE はシーケンスが動かす部品を Movable にしておく決まりで、Static のまま動かすと描画側（静的な描画リスト・Lumen の面キャッシュ・焼いた影）が元の場所に残ることがある。Windows では残らなかったが、**Mac（Metal）の「2 台」の唯一の見当はこれ**なので、ステップ 4 で Movable にして直す（屋根の当たり `BlockingVolume_Ambulance_1〜6` は救急車の子で、こちらは既に Movable）。
- 2026-09-22: **パッケージ版の切り分けは `Intermediate/Overnight/probe_ambulance.py`（git の外）で行う**。`Tools/game_flow.py` と同じく `-ExecCmds` だけで動かす（画面への入力は要らない）。分かったこと 2 つ:
  - `TriggerBox_06_AmbulanceTop` は `Start06()`（駐車場の場面の後）で結ばれるので、**そこまでの流れを順に起こさないと `Wasami.Trigger` は何もしない**（黙って無視される）。
  - `BugItGo` は最後に `Ghost()` を呼ぶので、**置いた後に `walk` を送らないと**プレイヤーは宙に浮いたまま落ちず、動く床にも乗らない（最初の 2 回はこれで「運ばれない」と読み違えた）。

## 要確認（ユーザー）

- 2026-09-22: **完了の条件 (3)（パッケージ版での確かめ）には `RunUAT.bat BuildCookRun` の許可が要る**。直しを入れた後のパッケージを作れないと、ステップ 5 が項目 39 と同じところで止まる。許可の件は進捗記録 `20260922-cooked-engine-assets` の「要確認」と同じなので、そちらに答えがもらえればこの項目も進む。
- 2026-09-22: **「2 台」は Mac 版でしか出ていない見込み**。Windows のパッケージ版では 3 つの見方で撮っても 1 台だけだった。ステップ 4 で救急車を Movable にして直すが、**直った絵は Mac でユーザーに見てもらうしかない**。
- 2026-09-22: **祭壇の球の材質がパッケージ版で既定の材質に落ちている**（この項目の外で見つけた。パッケージ版のログ: `Material /Game/DD/Materials/Fords_Materials/m_crystal_Inst2 missing usage flag Nanite!` と `... StaticLighting!` → `Default Material will be used in game.`）。Zone 2 のガレージの祭壇の球（08 記録）が本家と違う見た目になっているはず。**作業一覧に項目を立てて直すか**。

## 再開時の注意

- エディタは開き直してある（`python Tools/editor_cycle.py --no-quit --no-build`）。パッケージ版を走らせる前には**必ず閉じる**（VRAM 6 GB。`python Tools/editor_cycle.py --quit-only` → 走らせる → `--no-quit --no-build`）。
- **パッケージ版の切り分け役**: `python Intermediate/Overnight/probe_ambulance.py track|ride|look`（git の外。消えたら書き直す。1 回 2 分ほど）。`Saved/Archive/Windows/wasami_deception.exe`（2026-09-21 の組み立て）をタイトルから走らせ、Zone 1 の流れを `Wasami.Delay` の台本で救急車まで進めてから、`track`=屋根に落として運ばれるかを 0.5 s ごとの `Wasami.Status` で測る / `ride`=同じで前を向いて 0.5 s ごとに撮る / `look`=救急車の 8 m 後ろの路面から撮る。撮ったものは `Intermediate/Overnight/ambulance/<mode>/`。
- 走り出しの流れ（`AWasamiZone1Flow::On06ReachAmbulance`）: 屋根の `TriggerBox_06_AmbulanceTop` を踏む → チェックポイント 7 を保存 → 1 s 後にシーケンス `06_Hospital_Zone1_AmbulanceTakeOff` と画面揺れ → 8 s で読み込み画面 → 10.5 s で Zone 2 を開く。シーケンスは救急車を y −19993 → −19083（1.77 s）→ −3093（10 s）へ動かす（線形部は約 1940 cm/s）。
- **屋根の当たりは `BlockingVolume_Ambulance_5`**（上面 z 312、プレイヤーは z 402 で立つ）。1〜4 は走り出しで当たりが入る囲い（前 y −19825・後ろ y −20310・左右 x 11085/11400）、6 は下の板。6 つとも救急車の子で Movable。救急車のメッシュ自身は `NoCollision`。
- **同じ流れをもう一度 `Wasami.Trigger TriggerBox_06_AmbulanceTop` で起こすとシーケンスが頭から流れ直し**、救急車が駐車位置へ飛んで戻る（プレイヤーは宙に取り残される）。切り分けのときに二重に起こさない。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- パッケージ版（`Saved/Archive/Windows`、2026-09-21 の組み立て）: 走り出しを 3 回起こし、`Wasami.Status` の `player=` と 25 枚の絵で上の「決定事項」を確かめた（2026-09-22）
