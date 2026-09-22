---
title: Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）
status: 進行中
branch: main
base: 055a05f
started: 2026-09-22 13:36
updated: 2026-09-22 14:20
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

- [x] 1. 計画を立てる（完了の条件 (1) はここで済ませた） … 2026-09-22 完了。救急車は本家と同じ 5 台で重なりも余分も無く、トンネルの中は走り出す 1 台だけ。
- [x] 2. 既存のパッケージ版で再現する … 2026-09-22 完了。置いていかれるのは 3 回とも再現、「2 台」は再現せず。
- [x] 3a. PIE で再現し、落ちる仕組みを測る … 2026-09-22 完了。**PIE でもパッケージ版と同じに落ちる**ので、以後の切り分けと直しの確かめはパッケージ版が要らない。**可動性は原因ではない**（下の「決定事項」）。
- [ ] 3b. 置いていかれるのを直す（完了の条件 (2)） ← 次
  - 変更予定: `Source/wasami_deception/WasamiZone1Flow.cpp`・`.h`、`Source/wasami_deception/Tests/WasamiZoneFlowTests.cpp`、`.claude/implementation-records/11-zone-flow.md`、`.claude/references/troubleshooting.md`（835 行の既存の項目は「収録の負荷や屋根の後ろの端でだけ」と書いていて誤り。原因と対処を書き直す）
- [ ] 4. 救急車を Movable で置く（「2 台」の唯一の見当。下の「決定事項」）
  - 変更予定: `Tools/dd/prepare_stage.py` か `Content/Python/wasami_tools/pipeline/dd_level.py`、`.claude/implementation-records/01-stage-pipeline.md`、Zone 1・Zone 2 の組み立て直し
- [ ] 5. パッケージ版で 2 回続けて確かめる（完了の条件 (3)）

## 次にやること

ステップ 3b。**プレイヤーは救急車の進みの 6 割しか運ばれず、差が屋根の長さに届いたところで後ろへ抜けて落ちる**（下の「決定事項」の測り）。**見込みの原因は囲いの壁そのもの**: `On06ReachAmbulance` が当たりを入れる `BlockingVolume_Ambulance_1〜4` は救急車の子で、シーケンスが親を動かす。UE のベース移動（`UCharacterMovementComponent::UpdateBasedMovement`）はプレイヤーを掃引で動かすので、**壁の当たりの位置が更新される前にプレイヤーが動くフレームでは、古い位置の壁に阻まれて進みが削られる**（t=2.7 の 46 cm の後退は食い込みの押し戻しと読める）。

まず**走り出しの間だけ囲い 1〜4 の当たりを切って PIE で測り**、これで運ばれるなら原因が確定する。切り分けは `python Intermediate/Overnight/pie_ambulance.py`（下の「再開時の注意」）に `--no-walls` を足して行う。直し方の候補:

- (D) 走り出しの間は囲い 1〜4 の当たりを切る … 本家の柵の意図（横と後ろから落ちないようにする）を失うが、屋根の中央に立つ限り落ちない。いちばん小さい変更。
- (A) 屋根に乗っている間はプレイヤーを救急車（`hospital_ambulance_new_teleport`）に付け、読み込み画面で外す。確実だが本家に無い作り。柵は残せる。
- (B) 後ろの壁 `BlockingVolume_Ambulance_3` を掃引で動かす（シーケンスが動かすので、走り出しの間だけ別に動かすことになる）。
- (C) 走り出しでプレイヤーを屋根の中央へ寄せる … **中央（y −20060、壁から 2.5 m）に立たせても落ちた**ので、これだけでは足りない。

直した後、`python Intermediate/Overnight/pie_ambulance.py` で `player=` の y が読み込み画面まで伸びる（z が 402 のまま）ことを見る。

## 決定事項

- 2026-09-22: **完了の条件 (1) は済み**（ステップ 1）。レベルの救急車は 5 台で本家（`pak_reference_2/_levels/06_Hospital_Zone_01.scene.json`）と一致、トンネルの中は 1 台だけ。前処理のシーケンスは `possessable` をレベルの実アクタへ解決し `spawn_template` は `null` なので複製も生まない。
- 2026-09-22: **「置いていかれる」は PIE でもパッケージ版と同じに起きる**（ステップ 3a）。屋根の中央 (11245, −20060) に落として乗せ、`t.MaxFPS 60` で走り出させたときの `Wasami.Status` の `player=`（走り出しを t=1.0 とする）:

  | t | 1.0 | 1.3 | 1.7 | 2.0 | 2.3 | 2.7 | 3.0 | 3.3 | 4.0 |
  | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
  | y | −20060 | −20019 | −19903 | −19725 | −19498 | **−19544** | −19262 | −18877 | −18877 |
  | z | 402 | 402 | 402 | 402 | 402 | 402 | 402 | 348 | 90 |

  **プレイヤーは t=1.0→3.0 の 2 s で 798 cm しか進まない**が、シーケンスの救急車は同じ 2 s で約 1278 cm 進む（1.77 s で 910 cm ＋ 残りが約 1600 cm/s）。**差の 480 cm は屋根の前後の長さ 485 cm（囲いの前 y −19825・後ろ y −20310）とほぼ同じ**で、遅れが屋根の長さに届いた瞬間に後ろへ抜けて落ちる。t=2.7 の 46 cm の後退（−19498 → −19544）は、壁に食い込んで押し戻された跡と読める。つまり**ベース移動は効いているが、進みの 6 割しか伝わっていない**。
- 2026-09-22: **救急車の可動性は「置いていかれる」の原因ではない**（ステップ 3a）。`hospital_ambulance_new_teleport` のメッシュを Movable にして同じ測りをすると、**y の値が 1 cm も違わず同じ**だった（−20019 / −19903 / −19725 / −19498 / −19544 / −19262 / −18877）。可動性を直すのは「2 台」（ステップ 4）のためで、落ちるのとは別。測った後はレベルを Static に戻してある（dirty 無し）。
- 2026-09-22: **「2 台」はパッケージ版（Windows）では再現しない。** 3 つの見方で撮って、どの瞬間も救急車は 1 台だけだった。**唯一の見当は救急車の可動性**: `hospital_ambulance_new_teleport` のメッシュは `Static` のままシーケンスに動かされている（`Intermediate/Pipeline/dd/stage_ue.json`。原作の `AStaticMeshActor` の既定をそのまま写したもので、原作も Static）。UE はシーケンスが動かす部品を Movable にしておく決まりで、Static のまま動かすと描画側（静的な描画リスト・Lumen の面キャッシュ・焼いた影）が元の場所に残ることがある。Windows では残らなかったが、レビュアーが遊んだのは Mac 版なので、**Mac（Metal）でだけ出る描画の食い違い**の見当としてステップ 4 で直す。屋根の当たり `BlockingVolume_Ambulance_1〜6` は救急車の子で、こちらは既に Movable。

## 要確認（ユーザー）

- 2026-09-22: **完了の条件 (3)（パッケージ版での確かめ）には `RunUAT.bat BuildCookRun` の許可が要る**（許可の件は進捗記録 `20260922-cooked-engine-assets` の「要確認」と同じ）。ただし**ステップ 3a で PIE でも同じに再現すると分かった**ので、直しの切り分けと確かめは PIE で進められる。止まるのはステップ 5 だけ。
- 2026-09-22: **「2 台」は Mac 版でしか出ていない見込み**。Windows のパッケージ版では 3 つの見方で撮っても 1 台だけだった。ステップ 4 で救急車を Movable にして直すが、**直った絵は Mac でユーザーに見てもらうしかない**。
- 2026-09-22: **祭壇の球の材質がパッケージ版で既定の材質に落ちている**（この項目の外で見つけた。パッケージ版のログ: `Material /Game/DD/Materials/Fords_Materials/m_crystal_Inst2 missing usage flag Nanite!` と `... StaticLighting!` → `Default Material will be used in game.`）。Zone 2 のガレージの祭壇の球（08 記録）が本家と違う見た目になっているはず。**作業一覧に項目を立てて直すか**。

## 再開時の注意

- エディタは開いたまま（`L_Hospital_Zone1`、PIE は止めてある、dirty 無し）。
- **PIE の切り分け役**: `python Intermediate/Overnight/pie_ambulance.py [--fps N]`（git の外。消えたら書き直す。1 回 80 秒ほど）。PIE を起こし、`Tools/pie.py cmd` で `Wasami.Delay` の台本（`Wasami.ResetSave` → `open L_Hospital_Zone1` → 各トリガー → 屋根へ `BugItGo` → `walk`）を一度に積み、0.25 s ごとの `Wasami.Status` をログ `Saved/Logs/wasami_deception.log` から読み返して表にする。終わると PIE を止める。**`Wasami.Delay` は PIE でも PIE のワールドに効く**（確かめた）。
  - `BugItGo` は最後に `Ghost()` を呼ぶので、**置いた後に `walk` を送らないと**プレイヤーは宙に浮いたまま落ちず、動く床にも乗らない。
  - `TriggerBox_06_AmbulanceTop` は `Start06()`（駐車場の場面の後）で結ばれるので、**そこまでの流れを順に起こさないと `Wasami.Trigger` は何もしない**。屋根に着地するとトリガーは自分で発火するので、`Wasami.Trigger` で二重に起こさない（シーケンスが頭から流れ直し、救急車が駐車位置へ飛んで戻る）。
- パッケージ版の切り分け役は `python Intermediate/Overnight/probe_ambulance.py track|ride|look`（`Saved/Archive/Windows/wasami_deception.exe`、2026-09-21 の組み立て）。エディタを閉じてから走らせる（VRAM 6 GB。`python Tools/editor_cycle.py --quit-only` → 走らせる → `--no-quit --no-build`）。
- 走り出しの流れ（`AWasamiZone1Flow::On06ReachAmbulance`）: 屋根の `TriggerBox_06_AmbulanceTop` を踏む → チェックポイント 7 を保存 → **囲い `BlockingVolume_Ambulance_1〜4` の当たりを入れる** → 1 s 後にシーケンス `06_Hospital_Zone1_AmbulanceTakeOff` と画面揺れ → 8 s で読み込み画面 → 10.5 s で Zone 2 を開く。シーケンスは救急車を y −19993 → −19083（1.77 s）→ −3093（10 s）へ動かす。
- **屋根の当たりは `BlockingVolume_Ambulance_5`**（上面 z 312、プレイヤーは z 402 で立つ）。1〜4 は走り出しで当たりが入る囲い（前 y −19825・後ろ y −20310・左右 x 11085/11400）、6 は下の板。6 つとも救急車の子で Movable。救急車のメッシュ自身は `NoCollision`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- PIE（`L_Hospital_Zone1`、`t.MaxFPS 60`）: 走り出しを 2 回（Static / Movable）起こし、上の表の測りを得た（2026-09-22）
- パッケージ版（`Saved/Archive/Windows`、2026-09-21 の組み立て）: 走り出しを 3 回起こし、置いていかれるのを 3 回とも再現、「2 台」は出ないと確かめた（2026-09-22）
