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

ステージ OP（`UWasamiChapterPortalWidget`）は輪（`chapter_ui_portal_outer`）とルーンの輪（`chapter_title_portal_inner`）の真ん中に本作の頭 `T_PauseHead` を 470 px 四方で重ねている。読み込み画面の紋章（`Tools/dd/prepare_loader.py` の `MARK_CENTRE`）も同じ作りなので一緒に見る。

完了の条件（作業一覧より）:

1. 本家の紋章の絵の中で、キャラクターの印の見た目の中心が輪の中心からどれだけずれているかを測る。
2. 本作のシンボルの見た目の中心が輪の中心に来るようにする（前処理で絵を置き直すか、ウィジェットでずらすか。選んだ方を 09・14 記録に書く）。
3. ステージ OP と読み込み画面の両方を撮り、輪の中心に来ていることを確かめる。

## 計画

- [ ] 1. **測る**（完了の条件 (1)）: 本家の 9 枚の `pause_*_head.png`（ステージ OP と ポーズ画面が使う頭）と 9 枚の `loader_*.png` の印について、輪の中心からのずれを 3 通り（α の外接箱の中心・α の重心・見た目の中心）で測る。本作の `pause_head.png`・`wasami_symbol.png` も同じ測り方で測る。**ずらす量と、どこで直すか（前処理で絵を置き直す / ウィジェットの `Logo` を `SetRenderTranslation` でずらす）を決めて「決定事項」に書く。**
  - 変更予定: この記録だけ（測る台本は `observations/tools/` に置く。git の外）
- [ ] 2. **直す**（完了の条件 (2)）: 決めた場所に直しを入れる。ステージ OP の頭（`Source/wasami_deception/WasamiChapterPortalWidget.cpp` の `Logo`、または `Tools/dd/prepare_*` で `pause_head.png` を置き直した絵を作って `dd_ui.import_pause` から取り込む）と、読み込み画面の `Tools/dd/prepare_loader.py` の `MARK_CENTRE`。テスト `Source/wasami_deception/Tests/WasamiChapterPortalTests.cpp`（`Logo` の大きさ 470 を見ている）を直す。C++ を変えたら `python Tools/editor_cycle.py`。
  - 変更予定: `Source/wasami_deception/WasamiChapterPortalWidget.cpp`・`Tests/WasamiChapterPortalTests.cpp`、`Tools/dd/prepare_loader.py`、`/Game/Wasami/UI/Pause/T_PauseHead`・`/Game/Wasami/UI/loader_wasami`
- [ ] 3. **確かめて閉じる**（完了の条件 (3)）: PIE でステージ OP（`Wasami.ChapterPortal` 相当か Zone 1 の開始）と読み込み画面を撮り、輪の中心と印の中心を重ねたグリッドで確かめる。実装記録 09（必要なら 15）を直し、`python .claude/scripts/check_records.py --update`。作業一覧の項目 45 を「完了」にし、この記録を消してコミット。
  - 変更予定: `.claude/implementation-records/09-ui.md`、`.claude/roadmap.md`

## 次にやること

ステップ 1（測る）。`observations/tools/` に測る台本を書き、本家の 9 枚の頭・9 枚の紋章と本作の 2 枚を同じ測り方で測って、下の「下調べ（2026-09-22）」の数値を確かめ直し、ずらす量と直す場所を決める。

## 下調べ（2026-09-22。計画を立てた反復で測った暫定値）

α > 8 の外接箱の中心と、キャンバスの中心（1024 なら (511.5, 511.5)、512 なら (255.5, 255.5)）とのずれ。**+ は下 / 右**。

| 絵 | 大きさ | 外接箱の中心 | 中心からのずれ |
| --- | --- | --- | --- |
| 本作 `SourceArt/Wasami/UI/pause_head.png` | 1024 | (516.0, 560.5) | **(+4.5, +49.0)** |
| 本家 `pause_reapernurse_head.png`（病院） | 1024 | (511.5, 550.5) | (+0.0, +39.0) |
| 本家 `pause_ducky_head.png` | 1024 | (511.0, 571.0) | (−0.5, +59.5) |
| 本家 `pause_lucky_head.png` | 1024 | (526.0, 507.0) | (+14.5, −4.5) |
| 本作 `SourceArt/Wasami/UI/wasami_symbol.png` | 512 | (267.0, 254.5) | (+11.5, −1.0) |
| 本家 `chapter_title_portal_inner`（ルーンの輪） | 512 | (255.0, 255.5) | (−0.5, **0.0**) |
| 本家 `chapter_ui_portal_outer`（外の輪） | 512 | (255.5, 274.5) | (+0.0, +19.0) |

- **ルーンの輪は絵の中心にぴったり**なので、470 px の箱に入れたとき輪の中心 = 箱の中心。ずれはすべて頭の絵の側から来る。
- 本作の頭は 1024 で +49 px 下 = **470 px では約 22 px 下**。本家の病院の頭も +39 px（470 で約 18 px）下なので、**本家も同じ向きにずれている**。差は 470 px で約 4.6 px。
- ただし `pause_head.png` の α の重心は (505.9, 511.8) でほぼキャンバスの中心なので、**外接箱の中心と重心が 49 px も食い違う**（上の方に濃い部分がある）。「見た目の中心」がどちらに近いかはステップ 1 で決める。
- 読み込み画面の `MARK_CENTRE = (256, 262)` は魔法陣の中心 (254.5, 255.5) より 6.5 px 下（512 で 1.3 %）。本家の印の箱の中心から取った値なので、**本家の印も少し下に置かれている**。

## 決定事項

- 2026-09-22: **この反復は計画だけ**（無人運転の新しい項目の初回）。`.claude/skills/continue/SKILL.md` の 1 節。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは起動している（PID 30880）。長時間処理は無し。
- 未完了の記録がほかに 4 件あるが、すべて `status: ユーザー待ち`（`20260922-cooked-engine-assets`・`20260922-enemy-hand-flip`・`20260922-title-theme-music`・`20260922-zone1-ambulance`）。無人運転は飛ばす。
- ステップ 2 で C++ を変えるなら `python Tools/editor_cycle.py`（ビルド込みの開き直し）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
