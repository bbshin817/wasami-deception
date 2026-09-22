---
title: ステージ OP の紋章でワサミのシンボルが下にずれている（作業一覧の項目 45）
status: 進行中
branch: main
base: 79e8e63
started: 2026-09-22 00:00
updated: 2026-09-22 18:20
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

- [x] 1. **測る** — 済。直す量（470 px で上へ 11.3 px、X は直さない）と直す場所（ウィジェットの `Logo`）を決めた。根拠と測り方は 09 記録の「頭の絵のずれ」へ移した。
- [x] 2. **直す** — 済。`UWasamiChapterPortalWidget::HeadOffset`（0, −11.3）を足して `Logo` に `SetRenderTranslation`、テスト `Wasami.ChapterPortal.Tree` にその確認を足した。ビルドは通り、`Wasami.ChapterPortal.Loop`・`Tree` とも成功。09 記録も直して `check_records --update` 済み。
- [ ] 3. **確かめて閉じる**（完了の条件 (3)）: PIE でステージ OP と読み込み画面を撮り、`observations/tools/portal_symbol_compose.py` の輪の内側の円を重ねたグリッドと同じ見え方（印が輪の中心）になっていることを確かめる。作業一覧の項目 45 を「完了」にし、この記録を消してコミット。
  - 変更予定: `.claude/roadmap.md`

## 次にやること

ステップ 3（確かめて閉じる）。PIE で Zone 1 のステージ OP と読み込み画面を撮って印の位置を見る（撮り方は下の「再開時の注意」）。良ければ作業一覧の項目 45 を「完了」にし、この記録を消してコミット。

## 決定事項

- 2026-09-22（ステップ 1）: **読み込み画面の `MARK_CENTRE = (256, 262)` は直さない。** 本家の 9 枚の印の箱の中心の平均が (255.8, 260.5) でこの値そのもの。本作の `loader_wasami.png` の印は「中点」の測り方で魔法陣の中心から −7.3 px（512 では −8.0 px、上）、本家の 9 枚は −7.8〜+12.4 px に散らばっていて本作はその幅の中。本家自身が 1 枚ずつ目分量で置いていて揃える値が無い。**ステップ 3 で読み込み画面を撮るのは「本家の散らばりの中にある」ことの確認までで、直さない。**
- 2026-09-22（ステップ 2）: 未コミットの変更（ソース・テスト・09 記録）が前の反復の打ち切りで残っていたので、捨てずにそのまま活かしてビルドと検証から続けた。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動している。長時間処理は無し。C++ は最新（`Binaries/Win64/UnrealEditor-wasami_deception.dll` 18:09、ソース 18:08）。
- 見比べる絵は `observations/tools/portal_symbol_compose.py`（輪・ルーン・印を 470 px で重ね、輪の内側の円と十字を描く。`--shift-y` で動かせる）。`observations/portal_symbol/candidates2.png` に 3 案の見比べがある。`observations/` は git の外。
- **背面のエディタは 3 fps で、Automation テストは 10 fps を待って進まない**（600 s で打ち切り）。走らせる前に `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)`、終わったら `True` に戻す（症状索引）。PIE の撮影も同じ。
- 未完了の記録がほかに 4 件あるが、すべて `status: ユーザー待ち`（`20260922-cooked-engine-assets`・`20260922-enemy-hand-flip`・`20260922-title-theme-music`・`20260922-zone1-ambulance`）。無人運転は飛ばす。

## 検証

- check_records: OK（20 件、09-ui.md のハッシュを更新）
- C++ ビルド: OK（`Tools/editor_cycle.py`、2026-09-22 18:09）
- 自動テスト: `Wasami.ChapterPortal.Loop`・`Wasami.ChapterPortal.Tree` とも Success（エディタの中、2026-09-22 18:17）
- エディタでの確認（PIE でステージ OP を撮る）: 未実行（ステップ 3）
