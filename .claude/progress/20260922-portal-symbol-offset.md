---
title: ステージ OP の紋章でワサミのシンボルが下にずれている（作業一覧の項目 45）
status: 進行中
branch: main
base: 79e8e63
started: 2026-09-22 00:00
updated: 2026-09-22 00:00
---

# ステージ OP の紋章でワサミのシンボルが下にずれている（作業一覧の項目 45）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 45（大目標 4「レビュー指摘の修正」）。レビュアーの指摘（Medium）:

> Zone1開幕アニメにおいて、紋章のワサミシンボルが若干下にズレている

完了の条件（作業一覧より）:

1. 本家の紋章の絵の中で、キャラクターの印の見た目の中心が輪の中心からどれだけずれているかを測る。
2. 本作のシンボルの見た目の中心が輪の中心に来るようにする（前処理で絵を置き直すか、ウィジェットでずらすか。選んだ方を 09・14 記録に書く）。
3. ステージ OP と読み込み画面の両方を撮り、輪の中心に来ていることを確かめる。

## 計画

- [x] 1. **測る**（完了の条件 (1)）— 済。本家の印 8 枚・本家の頭 7 枚・読み込み画面の印 9 枚と本作の絵を同じ測り方で測り、直す量（470 px で **上へ 11.3 px**、X は直さない）と直す場所（ウィジェットの `Logo` の `SetRenderTranslation`）を決めた。下の「決定事項」。
- [ ] 2. **直す**（完了の条件 (2)）: `Source/wasami_deception/WasamiChapterPortalWidget.cpp` の `Logo` に `SetRenderTranslation(FVector2D(0.f, -11.3f))` を入れ（`PlaceInPortal` の前後どちらでもよいが、`Ring`・`Runes` と同じく作る所で）、理由（本作の頭の絵は canvas の中で下に寄っている）をコメントに書く。テスト `Source/wasami_deception/Tests/WasamiChapterPortalTests.cpp` の `Logo` の項にこの移動の確認を足す（大きさ 470 と slot の offsets 0 は今のまま通る）。**読み込み画面（`Tools/dd/prepare_loader.py` の `MARK_CENTRE`）は直さない**（決定事項）。`python Tools/editor_cycle.py` でビルドし直す。
  - 変更予定: `Source/wasami_deception/WasamiChapterPortalWidget.cpp`・`Source/wasami_deception/Tests/WasamiChapterPortalTests.cpp`
- [ ] 3. **確かめて閉じる**（完了の条件 (3)）: PIE でステージ OP と読み込み画面を撮り、`observations/tools/portal_symbol_compose.py` の輪の内側の円を重ねたグリッドと同じ見え方になっていることを確かめる。実装記録 09 に「頭の絵のずれをウィジェットで戻している」ことと測った値を書き、`python .claude/scripts/check_records.py --update`。作業一覧の項目 45 を「完了」にし、この記録を消してコミット。
  - 変更予定: `.claude/implementation-records/09-ui.md`、`.claude/roadmap.md`

## 次にやること

ステップ 2（直す）。`WasamiChapterPortalWidget.cpp` の `Logo` に `SetRenderTranslation(FVector2D(0.f, -11.3f))` を入れ、テストに確認を足して `python Tools/editor_cycle.py`。

## 決定事項

- 2026-09-22: **この反復は計画だけ**（無人運転の新しい項目の初回）。`.claude/skills/continue/SKILL.md` の 1 節。
- 2026-09-22（ステップ 1）: **本家のステージ OP の印は `Textures/00_Ballroom/portal_<キャラ>.png` で、病院の `portal_nurse.png` は `pause_reapernurse_head.png` を半分に縮めた同じ絵**（α の範囲が 1024 の x314-709 y274-827 と 512 の x157-354 y137-413 でぴったり一致）。つまり**本作が `T_PauseHead`（ポーズ画面の頭）を使っているのは本家と同じ作り**で、直すのは絵の中の位置だけでよい。本家の `UMG_ChapterPortal` の `Logo` の slot は offsets も render transform も持たず、輪・ルーン・印の 3 枚とも 470 px 四方を中心で重ねているだけ（`pak_reference_2/_assets/.../UMG_ChapterPortal.json` の `CanvasPanelSlot_9`）。**ずれは絵の側にしか無い。**
- 2026-09-22（ステップ 1）: **「見た目の中心」は α の外接箱の中心と α の重心の中点と決めた。** 外接箱だけだと本作の頭は舌（細く下に伸びる）に引かれて下に出すぎ、重心だけだと髪の量に引かれて上に出すぎる（本作の頭は 470 px 換算で 箱 +22.5 / 重心 +0.1 と 22 px も食い違う）。中点にすると**本家の病院の印は +1.1 px（＝輪の中心）**になり、絵を見た印象とも合う。
- 2026-09-22（ステップ 1）: **上へ 11.3 px（470 px 換算）動かす。X は動かさない。** 本作の頭は同じ測り方で **+11.3 px 下**（箱 +22.5・重心 +0.1 の中点）、X は −0.25 px で無視できる。3 案（今のまま / −11.3 / 箱を中心に合わせる −22.5）を輪の内側の円（半径 124.8 px @470）と重ねて描いて見比べ、−22.5 は髪が円の上端に付いて逆に高く見えたので **−11.3** を採った（`observations/portal_symbol/candidates2.png`）。
- 2026-09-22（ステップ 1）: **直す場所はウィジェット（`Logo` の `SetRenderTranslation`）**。`T_PauseHead` はポーズ画面（900 px の頭）とも共用で、そちらはレビューで指摘されていない。絵を置き直すとポーズ画面まで動く。
- 2026-09-22（ステップ 1）: **読み込み画面の `MARK_CENTRE = (256, 262)` は直さない。** 本家の 9 枚の印の箱の中心の平均が (255.8, 260.5) でこの値そのもの。本作の `loader_wasami.png` の印は同じ「中点」の測り方で魔法陣の中心から **−7.3 px（512 では −8.0 px、上）**、本家の 9 枚は −7.8〜+12.4 px に散らばっていて、本作はその幅の中に入っている。本家自身が 1 枚ずつ目分量で置いているので、揃える値が無い。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動している。長時間処理は無し。
- 測る台本は `observations/tools/portal_symbol_measure.py`（本家・本作の印の箱・重心・中点を 470 px 換算で並べる）と `observations/tools/portal_symbol_compose.py`（輪・ルーン・印を 470 px で重ねて輪の内側の円と十字を描く。`--shift-y` で動かせる）。`observations/` は git の外。
- 未完了の記録がほかに 4 件あるが、すべて `status: ユーザー待ち`（`20260922-cooked-engine-assets`・`20260922-enemy-hand-flip`・`20260922-title-theme-music`・`20260922-zone1-ambulance`）。無人運転は飛ばす。
- ステップ 2 は C++ を変えるので `python Tools/editor_cycle.py`（ビルド込みの開き直し）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
