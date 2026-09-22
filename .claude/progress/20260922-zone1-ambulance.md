---
title: Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）
status: 進行中
branch: main
base: 055a05f
started: 2026-09-22 13:36
updated: 2026-09-22 15:00
---

# Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 41（大目標 4「レビュー指摘の修正」）。レビュアーの指摘（Medium）:

> Zone1 → Zone2への転換時、トンネルの中に救急車が2台存在し、プレイヤーが置いていかれる

本家は屋根に乗ったまま運ばれて読み込み画面に入る。完了の条件:

1. 組み立てた `L_Hospital_Zone1` の救急車のアクタを数え、本家の 5 台と重なりが無いことを確かめる。違えば組み立てを直す。
2. 置いていかれないようにする。**本家と違う作りにするので、選んだ理由を 11 記録に書く**。
3. パッケージ版で、屋根に乗ってから読み込み画面まで運ばれることを 2 回続けて確かめる。

## 計画

- [x] 1. 計画（完了の条件 (1) も）… 救急車は本家と同じ 5 台で重なりも余分も無く、トンネルの中は走り出す 1 台だけ。
- [x] 2. 既存のパッケージ版で再現 … 置いていかれるのは 3 回とも再現、「2 台」は再現せず。
- [x] 3a. PIE で再現し落ちる仕組みを測る … PIE でも同じに落ちる。可動性は原因でない。
- [x] 3b. 置いていかれるのを直す（完了の条件 (2)）… 2026-09-22 完了。原因は**後ろの壁 `BlockingVolume_Ambulance_3`**（下の「決定事項」）。`On06ReachAmbulance` はこの 1 枚だけ当たりを入れないようにし、屋根の真ん中と後ろの端の両方から読み込み画面まで 100 % 運ばれることを PIE で確かめた。
- [ ] 4. 救急車を Movable で置く（「2 台」の唯一の見当。下の「決定事項」） ← 次
  - 変更予定: `Tools/dd/prepare_stage.py` か `Content/Python/wasami_tools/pipeline/dd_level.py`、`.claude/implementation-records/01-stage-pipeline.md`、Zone 1・Zone 2 の組み立て直し
- [ ] 5. パッケージ版で 2 回続けて確かめる（完了の条件 (3)。`RunUAT.bat` の許可待ち。下の「要確認」）

## 次にやること

ステップ 4。`hospital_ambulance_new_teleport` のメッシュは `Static` のままシーケンスに動かされている（`Intermediate/Pipeline/dd/stage_ue.json`。原作の `AStaticMeshActor` の既定をそのまま写したもので、原作も Static）。UE はシーケンスが動かす部品を Movable にしておく決まりなので、**前処理でシーケンスが動かすアクタを Movable にする**（`dd_level` の組み立て側か、シーケンスの `possessable` の解決側で、動かされる側に印を付ける）。Zone 1 の走り出しの救急車と Zone 2 の到着の救急車（`hospital_ambulance_new_arrive`）が対象。直したら Zone 1・Zone 2 を組み立て直し、PIE で走り出しが前と同じに動くことだけ確かめる（「2 台」は Windows では出ないので、直った絵は Mac のユーザーに見てもらうしかない）。

## 決定事項

- 2026-09-22: **完了の条件 (1) は済み**（ステップ 1）。レベルの救急車は 5 台で本家（`pak_reference_2/_levels/06_Hospital_Zone_01.scene.json`）と一致、トンネルの中は 1 台だけ。前処理のシーケンスは `possessable` をレベルの実アクタへ解決し `spawn_template` は `null` なので複製も生まない。
- 2026-09-22: **「置いていかれる」の原因は走り出しで当たりを入れる囲いのうち後ろの壁 `BlockingVolume_Ambulance_3`**（ステップ 3b）。囲い 1〜4 は救急車の子で、シーケンスが親を掃引なしで動かす。プレイヤーは屋根 `BlockingVolume_Ambulance_5` を土台にして運ばれる（`UCharacterMovementComponent::UpdateBasedMovement` の掃引の移動）が、**1 フレームの進みが後ろの壁との隙間を超えると壁がカプセルの中に現れ、土台の移動が始めからの食い込みで中止になり、押し出しがプレイヤーを 46 cm 後ろへ出して屋根から外す**。測り（屋根の真ん中に落として走り出させ、プレイヤーと救急車の y の差を追った）: 差は −66.7 のままで 100 % 運ばれ、救急車が 2440 cm/s へ上がる瞬間だけ 310 cm（救急車の進み 264 ＋ 押し戻し 46）を一度に失って外れる。**囲いを 4 枚とも切ると最後まで 100 % 運ばれる**ので、原因は囲いで確定。
- 2026-09-22: **直しは「後ろの 1 枚だけ当たりを入れない」**（ステップ 3b）。前（`_2`）と左右（`_1`・`_4`）はプレイヤーへ近づかないので本家どおり入れる。付けたままにする案（プレイヤーを救急車に付ける・後ろの壁を掃引で動かす・屋根の中央へ寄せる）はどれも副作用が大きい: 付けると土台の移動と二重になりうる、掃引で動かすと壁が止まるだけでプレイヤーを押さない、中央へ寄せても隙間（約 198 cm）を超える進みでは同じに外れる。失うのは「走り出しの間に後ろへ歩いて落ちるのを防ぐ」だけ。
- 2026-09-22: **PIE はエディタが前面でないとき約 3 fps（1 フレーム 0.33 s）で回る**。`t.MaxFPS 60` を送っても上がらない。上の測りはこの 3 fps（1 フレーム 800 cm）で行ったので、実機よりずっと厳しい条件。症状索引に書いた。
- 2026-09-22: **救急車の可動性は「置いていかれる」の原因ではない**（ステップ 3a）。Movable にして同じ測りをすると y が 1 cm も違わなかった。可動性を直すのは「2 台」（ステップ 4）のため。
- 2026-09-22: **「2 台」はパッケージ版（Windows）では再現しない。** 3 つの見方で撮って、どの瞬間も救急車は 1 台だけだった。**唯一の見当は救急車の可動性**（`Static` のままシーケンスに動かされている）。Windows では描画側が残らなかったが、レビュアーが遊んだのは Mac 版なので、**Mac（Metal）でだけ出る描画の食い違い**の見当としてステップ 4 で直す。屋根の当たり `BlockingVolume_Ambulance_1〜6` は救急車の子で、こちらは既に Movable。

## 要確認（ユーザー）

- 2026-09-22: **完了の条件 (3)（パッケージ版での確かめ）には `RunUAT.bat BuildCookRun` の許可が要る**（許可の件は進捗記録 `20260922-cooked-engine-assets` の「要確認」と同じ）。直し自体は PIE で確かめられるので、止まるのはステップ 5 だけ。
- 2026-09-22: **「2 台」は Mac 版でしか出ていない見込み**。Windows のパッケージ版では 3 つの見方で撮っても 1 台だけだった。ステップ 4 で救急車を Movable にして直すが、**直った絵は Mac でユーザーに見てもらうしかない**。
- 2026-09-22: **祭壇の球の材質がパッケージ版で既定の材質に落ちている**（この項目の外で見つけた。パッケージ版のログ: `Material /Game/DD/Materials/Fords_Materials/m_crystal_Inst2 missing usage flag Nanite!` と `... StaticLighting!` → `Default Material will be used in game.`）。Zone 2 のガレージの祭壇の球（08 記録）が本家と違う見た目になっているはず。**作業一覧に項目を立てて直すか**。

## 再開時の注意

- エディタは開いたまま（`L_Hospital_Zone1`、PIE は止めてある、dirty 無し）。C++ はビルド済み。
- **PIE の切り分け役**: `python Intermediate/Overnight/pie_ambulance.py [--fps N] [--y -20250] [--no-walls] [--trace-only]`（git の外。消えたら書き直す。1 回 90 秒ほど）。PIE を起こし、`Wasami.Delay` の台本（`Wasami.ResetSave` → `open L_Hospital_Zone1` → 各トリガー → 屋根へ `BugItGo` → `walk`）を一度に積み、`Wasami.Status` のログと、毎ティックの測り（`Intermediate/Overnight/ambulance_trace.py` がスレートの毎ティックの呼び出しで書く: プレイヤー・救急車・囲いの位置、移動の様式、土台）を表にする。**`--y` は屋根のどこに落とすか**（−20060 が真ん中、−20250 がテレポーテーションの着く後ろの端）。
  - `BugItGo` は最後に `Ghost()` を呼ぶので、**置いた後に `walk` を送らないと**動く床に乗らない。
  - 屋根に着地するとトリガーは自分で発火するので、`Wasami.Trigger` で二重に起こさない（シーケンスが頭から流れ直す）。
  - レベルのアクタの名前は `StaticMeshActor_5` の類で、**原作の名前はラベル**にある（`get_actor_label()`）。
- パッケージ版の切り分け役は `python Intermediate/Overnight/probe_ambulance.py track|ride|look`（`Saved/Archive/Windows/wasami_deception.exe`、2026-09-21 の組み立て）。エディタを閉じてから走らせる（VRAM 6 GB。`python Tools/editor_cycle.py --quit-only` → 走らせる → `--no-quit --no-build`）。
- 走り出しの流れ（`AWasamiZone1Flow::On06ReachAmbulance`）: 屋根の `TriggerBox_06_AmbulanceTop` を踏む → チェックポイント 7 → **囲い `_4`・`_2`・`_1` の当たりを入れる（`_3` は入れない）** → 1 s 後にシーケンス `06_Hospital_Zone1_AmbulanceTakeOff` と画面揺れ → 8 s で読み込み画面 → 10.5 s で Zone 2 を開く。シーケンスは救急車を y −19993 → −19083（1.77 s）→ −3093（10 s）へ動かす。

## 検証

- check_records: OK（20 件、2026-09-22）
- C++ ビルド: ok（`Tools/editor_cycle.py`、2026-09-22）
- Automation テスト: `Wasami.ZoneFlow` の 7 件中 6 件が Success。残る `Zone1` の「the burst woken」は `-NullRHI` では粒子が起動しないためで、この変更とは無関係（症状索引「Automation テストを `-NullRHI` で走らせると…」）。
- PIE（`L_Hospital_Zone1`、約 3 fps）: 直した後、屋根の真ん中（y −20060）と後ろの端（y −20250）の両方から走り出させ、**どちらも z 402 のまま読み込み画面（乗ってから 8 s）を越えて運ばれた**（`Intermediate/Overnight/run_fixed.txt`・`run_rear.txt`、git の外）。直す前は乗って約 1.6 s で屋根の後ろへ抜けて落ちていた。
- パッケージ版（`Saved/Archive/Windows`、2026-09-21 の組み立て）: 直す前に走り出しを 3 回起こし、置いていかれるのを 3 回とも再現、「2 台」は出ないと確かめた（2026-09-22）。直した後はステップ 5（許可待ち）。
