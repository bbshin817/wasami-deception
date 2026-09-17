---
title: マウスの視点移動の速さとスピードブーストの集中線の修正（作業一覧の項目 2）
status: 進行中
branch: feature/look-and-speedlines
base: 7208432
started: 2026-09-17 20:54
updated: 2026-09-17 21:55
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# マウスの視点移動の速さとスピードブーストの集中線の修正

## 依頼

作業一覧 `.claude/roadmap.md` の項目 2。ユーザーの原文（2026-09-17 の最終目標の「既存の修正点」）:

```
- マウスによる視点移動が遅すぎます
- スピードブースト使用時、集中線がおかしなノイズのように現れます
```

- 目標: 視点移動の速さを本家と同じにし、集中線を本家と同じ絵にする。
- 完了の条件: (1) 同じマウスの移動量に対する回転角が最新版の実機と一致する（`Tools/desktop.py look` で同じ量を送って測る）。(2) ブースト中の本作の画面（`observations/ours/`）で、集中線が本家の実機の収録と同じ形（放射状の線のコマ送り）に見える。
- 大規模改修として作業ブランチ `feature/look-and-speedlines` で進める（プレイヤーの C++ と取り込みの Python にまたがり、複数のコミットに分ける）。

## 計画

- [x] 1. PIE で今の値を測り、原因を切り分けた（測った値と撮った物は `observations/README.md` の「視点の速さと集中線」）
- [x] 2. 本家の実機（最新版）で撮った: 視点は **0.175°/カウント**（感度 1・平滑化オン）、集中線は **2 × 5 を行 0 から 1 画面 1 コマ・1 秒に 30 コマ**（測り方と値は `observations/README.md` の original の「視点の速さと集中線」）
- [x] 3. 視点を直した: `Look` の対応づけから Scalar を外し、感度 0.07 は `DefaultInput.ini` の `AxisConfig` だけで効かせる（実装記録 02・症状索引）。PIE で 0.175°/カウント（dx 100 → 17.5°、dx 2057 → 359.94°、dy 100 → −17.5°）
- [x] 4. 集中線を直した: `M_Speedlines` の FlipBook を 2 × 5・位相 `Time` × 3 に、`dd_assets.texture` が書き出しの `NeverStream` を写す（`T_Speedlines` とタブレットの UI 19 枚が真）。取り込み直して PIE で連写し、走っている 39 枚がすべて 2 × 5、30.05 コマ/s で 32 枚が合った（`observations/README.md` の ours の「集中線を直した後」、実装記録 04・01）
- [ ] 5. 仕上げ: 作業一覧の項目 2 を「完了」、handover の「現状と次の一歩」、note の原稿 `docs/note/progress.md`（note へは会話でセッションの値が渡されていなければ未反映）、症状索引（要れば）、`check_records.py --update`、記録を消して main へマージ → ブランチを削除 → push。

## 次にやること

ステップ 5（仕上げ）。作業一覧 `.claude/roadmap.md` の項目 2 を「完了」にする → `handover.md` の「現状と次の一歩」を直す → note の原稿 `docs/note/progress.md` を「いま何が出来るか」に合わせて直す（会話でセッションの値が渡されていなければ note へは未反映と書く）→ 症状索引は要るものだけ（集中線は実装記録 04 に書いたので不要のはず）→ `check_records.py --update` → この記録を消して、main へマージ → ブランチを削除 → push。

## 決定事項

- 2026-09-17: 視点の原因と直し方（`AxisConfig` の自動の Scalar との二重掛け）はステップ 3 で実装記録 02 と症状索引へ移した。本家の `Look` の形（× `Character.MouseSensitivity` 1.0）は変えていない。
- 2026-09-17: 集中線の値（2 × 5・30 コマ/s）の根拠と直す前のノイズの原因は、ステップ 4 で実装記録 04 と `observations/README.md` へ移した。見た目の値なので要確認にはしない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタ: ステップ 4 の後も起動したまま（L_Hospital_Zone1、PIE なし、未保存なし、`desktop.py` の係は止めた）。本家は起動していない。PIE と本家は反復の終わりに必ず止める・閉じる。
- 本家のセーブ: 観察で本家が `SaveSlot.sav`・`structSlot.sav` を書き換えた（21:25。いつもの動き）。控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260917-211519`。戻さない。
- MOD の W-Editor のファイル `%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` は前の作業の要確認のまま（作業一覧の「未回答の要確認」。今回は書き込みなし）。触らない。

## 検証

- check_records: ステップ 4 で OK（01・04 記録）
- C++ ビルド: ステップ 3 で成功（`editor_cycle.py`）。ステップ 4 は Python と記録だけ
- エディタでの確認: ステップ 3 で視点 0.175°/カウント。ステップ 4 で `import_dd_powers()`（sounds 7・camera_shakes 2・camera_anims 2・textures 14・meshes 2・materials 28・particle_systems 3）と `import_dd_tablet()`（前と同じ数）が通り、`Failed to compile` なし。`M_Speedlines` は 6 ノード。PIE の連写で 2 × 5・30 コマ/s
