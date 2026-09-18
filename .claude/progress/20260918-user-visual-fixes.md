---
title: ユーザーの指摘 3 件（敵ワサミの大きさ・ブーストの横線・タブレットのアイコンの位置）
status: 進行中
branch: main
base: 073c833
started: 2026-09-18 12:10
updated: 2026-09-18 12:40
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ユーザーの指摘 3 件（敵ワサミの大きさ・ブーストの横線・タブレットのアイコンの位置）

## 依頼

2026-09-18 のユーザーの指摘（観測元は UE5 エディタのテストプレイ・GIF・スクショのみ）。「高画質・本番リリースにおいては問題がない場合、対応不要。今後実装予定、あるいはすでに検討事項に入っている場合も対応不要」。

- 敵ワサミが極端に小さい。「Z軸スケールは本家の敵と同じ身長になるよう、敵ワサミモデルはX・Y・Zスケールを拡大する必要があります」
- Speed Boost で集中線以外に走査線が見える
- タブレットの特殊効果ロゴが、タブレットの円からやや下よりに配置されている

調べた結果、3 件とも本番でも出る本作の誤りだった（下の決定事項）。作業一覧の項目 23（`20260918-power-look-tuning.md`）とは別の作業で、そちらの記録は触らない。

## 計画

- [x] 1. ブーストの横線 … 2026-09-18 完了。`M_Speedlines` の Opacity ← `T_Speedlines` の A（04 記録、測った値は `observations/README.md` の「A を不透明度にした後」）
- [ ] 2. タブレットのアイコン ← 作業中: `WasamiTabletWidget.cpp` の `PowersX/Y` を背景のキャンバス基準 (0.0390625, 3.45947265625) に、「Z」を (51, 732) に
  - 変更予定: `Source/wasami_deception/WasamiTabletWidget.cpp`、実装記録 03
- [ ] 3. 敵ワサミの大きさ: `AWasamiEnemy` のメッシュに一様の拡縮 `MeshScale` = 229.0514 / 168.53（頭頂の骨どうし）、再生の速さの分母（歩幅の速さ）も × `MeshScale`、テストの期待値
  - 変更予定: `Source/wasami_deception/WasamiEnemy.h/.cpp`、`WasamiEnemyAnimInstance.cpp`（`.h` のコメント）、`Tests/WasamiEnemyTests.cpp`、実装記録 07
- 2 と 3 は C++ なので書き終えてから `python Tools/editor_cycle.py` を 1 回走らせ、テスト `Wasami.Enemy`・`Wasami.Tablet`（あれば）を通し、PIE で確かめて、それぞれコミットする。
- 最後に note の原稿と記事（`.claude/guides/note-progress.md`）を見直す。

## 次にやること

ステップ 2 と 3 の C++ を書く（`WasamiTabletWidget.cpp` の `PowersX/Y` と「Z」、`AWasamiEnemy::MeshScale` と歩幅の速さ、テスト）→ `python Tools/editor_cycle.py` → テスト → PIE で確かめる。

## 決定事項

- 2026-09-18: ブーストの横線は本番でも出る — `T_Speedlines` の画面いっぱいの横線の画素は RGB が白で A が 0（見せる楔は A > 0）。原作のコンパイル済みシェーダー（`python Tools/dd/cooked_shaders.py "UI/Main/Powers/M_Speedlines." --show 4`）は `mov_sat r0.w, r0.w` で A を不透明度にしている（コマ送り 2 × 5・3 周/秒は本作と一致）。本家の実機の連写 `orig-speedlines-burst2/f064.png`（コマ 0）には楔だけが出て横線は 1 本も無い。本作は Opacity が未接続（1）なので横線が出ていた。
- 2026-09-18: タブレットのアイコンは原作より約 12 texel 低い — 原作 `UMG_Tablet`（pak_reference_2）では背景のキャンバス `CanvasPanel_1` が中心から (−355, −420)、`UMG_TabletPowers` が (−356.9609, −428.5405)（その中の `CanvasPanel_2` は自分の中心から (−355, −420)）なので、背景から見た枠の原点は (0.0390625, 3.45947265625)。本作は画面の角を中心 − (357, 432) と取って (2.039, 15.459) にしていた。実測も合う（アイコンの中心が円の中心より、原作の実機は約 8 texel 下、本作は約 20 texel 下）。「Z」（`TextBlock_107`、中心から (−304, 312)）も同じ取り違えで (2, 12) ずれている。
- 2026-09-18: 敵ワサミの身長は頭頂の骨どうしで合わせる — 本家のナース `nurse_idle1` の `Nurse_TopOfHead_AuxSHJnt` は 229.0514 cm（その上の帽子を含むメッシュの頂は 246.35 cm）、ワサミの `head_end` は 168.53 cm（髪を含むメッシュの頂 170.0 cm）。身長は帽子を含まないので 229.0514 / 168.53 = 1.3591。部品の拡縮で掛ける（取り込みは変えない）。足はメッシュの原点（床）にあるので位置は変わらない。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いたまま（`L_Hospital_Zone1`、PIE なし、未保存なし。2026-09-18 12:10）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
