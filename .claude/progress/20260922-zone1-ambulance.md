---
title: Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）（作業一覧の項目 41）
status: ユーザー待ち
branch: main
base: 055a05f
started: 2026-09-22 13:36
updated: 2026-09-22 15:40
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
- [x] 2. 既存のパッケージ版で再現 … 置いていかれるのは 3 回とも再現、「2 台」は再現せず（Windows）。
- [x] 3a. PIE で再現し落ちる仕組みを測る … PIE でも同じに落ちる。可動性は原因でない。
- [x] 3b. 置いていかれるのを直す（完了の条件 (2)）… `On06ReachAmbulance` が屋根の後ろの壁 `BlockingVolume_Ambulance_3` だけ当たりを入れないようにした（理由は 11 記録の「既知の制約」）。
- [x] 4. 救急車を Movable で置く（「2 台」の唯一の見当）… 2026-09-22 完了。`dd_sequence` がシーケンスの動かす結び付けを Movable にするようにし（`moves`・`movable`）、両ゾーンを置き直した。直したのは 3 つ（下の「決定事項」）。
- [ ] 5. パッケージ版で 2 回続けて確かめる（完了の条件 (3)。**`RunUAT.bat` の許可待ち**。下の「要確認」） ← 次

## 次にやること

ステップ 5（**ユーザーが `RunUAT.bat BuildCookRun` の許可をくれてから**。下の「要確認」と、作業一覧の「未回答の要確認」の `20260922-cooked-engine-assets`）:

1. `python Tools/editor_cycle.py --quit-only` → `.claude/guides/distribution.md` の「パッケージ」の `RunUAT.bat BuildCookRun` → 走らせる → `python Tools/editor_cycle.py --no-quit --no-build`。
2. `python Intermediate/Overnight/probe_ambulance.py ride` を 2 回走らせ、**どちらも屋根に乗ってから読み込み画面まで運ばれる**ことを確かめる（直す前は 3 回とも落ちていた）。
3. `probe_ambulance.py look` で救急車が 1 台だけであることも撮っておく（「2 台」は Mac でしか出ていないので、直った絵はユーザーに見てもらう）。
4. 確かめたら記録を消し、作業一覧の項目 41 を「完了」にして handover を直す。

## 決定事項

- 2026-09-22: **「2 台」はパッケージ版（Windows）では再現しない。** 3 つの見方で撮って、どの瞬間も救急車は 1 台だけだった。唯一の見当は可動性で、レビュアーが遊んだ Mac（Metal）でだけ出る描画の食い違いとしてステップ 4 で直した。
- 2026-09-22: **ステップ 4 は「シーケンスが動かす結び付けを Movable にする」という前処理の規則にした**（`dd_sequence.moves`・`movable`。理由と規則は 01 記録の「シーケンス」）。救急車 1 台だけを名指しで直すより、同じ穴（本家のレベルが動かすものを `Static` で置いている）を全部ふさげる。読み取りだけの調べで**動かされているのに `Static` なのは 3 つだけ**と分かったので、影響は限られる: Zone 1 の走り出す救急車 `hospital_ambulance_new_teleport`、Zone 2 の独房の扉 `…_jail_door_17` と壁のスイッチ `…_wall_switch_14`（どれも動くべきもの）。**キーが 1 つだけで動かない結び付け（Zone 2 の独房の部屋）は `Static` のまま残した**（焼いた灯を失わないため）。
- 2026-09-22: 直しの副作用として **Zone 2 の独房の扉と壁のスイッチも焼いた灯を失う**（Movable になったため）。どちらも場面で動く物なので本来 Movable が正しく、見た目の詰めは大目標 3 の範囲（項目 28 の後回しの一覧）。

## 要確認（ユーザー）

- 2026-09-22: **完了の条件 (3)（パッケージ版での確かめ）には `RunUAT.bat BuildCookRun` の許可が要る**（作業一覧の「未回答の要確認」の `20260922-cooked-engine-assets` と同じ件）。直し 2 つはどちらも PIE で確かめてあるので、止まっているのはステップ 5 だけ。
- 2026-09-22: **「2 台」は Mac 版でしか出ていない見込み**。Windows のパッケージ版では 3 つの見方で撮っても 1 台だけだった。救急車を Movable にして直したが、**直った絵は Mac でユーザーに見てもらうしかない**。
- 2026-09-22: **祭壇の球の材質がパッケージ版で既定の材質に落ちている**（この項目の外で見つけた。パッケージ版のログ: `Material /Game/DD/Materials/Fords_Materials/m_crystal_Inst2 missing usage flag Nanite!` と `... StaticLighting!` → `Default Material will be used in game.`）。Zone 2 のガレージの祭壇の球（08 記録）が本家と違う見た目になっているはず。**作業一覧に項目を立てて直すか**（項目 39 と同じ「クックで初めて出る類い」）。

## 再開時の注意

- エディタは開いたまま（`L_Hospital_Zone1`、PIE は止めてある、dirty 無し）。C++ はビルド済み。
- パッケージ版の切り分け役は `python Intermediate/Overnight/probe_ambulance.py track|ride|look`（`Saved/Archive/Windows/wasami_deception.exe`）。エディタを閉じてから走らせる（VRAM 6 GB。`python Tools/editor_cycle.py --quit-only` → 走らせる → `--no-quit --no-build`）。**いまの `Saved/Archive` は 2026-09-21 の組み立てで、ステップ 3b・4 の直しが入っていない**ので、まず組み立て直す。
- PIE の切り分け役は `python Intermediate/Overnight/pie_ambulance.py [--fps N] [--y -20250] [--no-walls] [--trace-only]`（git の外。1 回 90 秒ほど。使い方は台本の頭に書いてある。`Tools/pie.py start` を先に、`stop` を後に）。
- シーケンスの結び付けの可動性を読み取りだけで調べる役は `python Tools/ue_remote.py Intermediate/Overnight/probe_seq_mobility.py`、置き直しは `Intermediate/Overnight/rebuild_sequences.py`（**エディタが読み込んだ pipeline のモジュールは `importlib.reload` しないと古いまま**。台本の頭でやっている）。

## 検証

- check_records: OK（20 件、2026-09-22）
- C++ ビルド: ok（`Tools/editor_cycle.py`、2026-09-22）
- Automation テスト: `Wasami.ZoneFlow` の 7 件中 6 件が Success。残る `Zone1` の「the burst woken」は `-NullRHI` では粒子が起動しないためで、この変更とは無関係（症状索引「Automation テストを `-NullRHI` で走らせると…」）。
- PIE（`L_Hospital_Zone1`、約 3 fps ＝ 1 フレーム 800 cm の最悪の場合）: ステップ 3b の後、屋根の真ん中（y −20060）と後ろの端（y −20250）の両方から走り出させ、どちらも z 402 のまま読み込み画面（乗ってから 8 s）を越えて運ばれた。**ステップ 4（Movable）の後も同じで、真ん中から 100 % 運ばれて Zone 2 が開き、到着の救急車の屋根にも乗れた**（15025 cm を 100 % 追随、屋根を離れた差 −10 cm）。直す前は乗って約 1.6 s で屋根の後ろへ抜けて落ちていた。
- 置き直し（`dd_sequence.place`、2026-09-22）: Zone 1 は 6 本・結び付け 32・トラック 39・区間 57・キー 213、Zone 2 は 6 本・結び付け 29・トラック 55・区間 117・キー 584（どちらも前と同じ）。`missing` は空、`made_movable` は Zone 1 が救急車 1 つ・Zone 2 が独房の扉と壁のスイッチ 2 つ。置き直した後にもう一度読み取りだけで調べ、3 つとも `MOVABLE` で保存されていることを確かめた。
- パッケージ版（`Saved/Archive/Windows`、2026-09-21 の組み立て）: 直す前に走り出しを 3 回起こし、置いていかれるのを 3 回とも再現、「2 台」は出ないと確かめた。直した後はステップ 5（許可待ち）。
