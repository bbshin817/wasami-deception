---
title: 観察の手順書と、進捗記録を畳む方針
status: 進行中
branch: feature/tablet-powers
base: 5338f5e
started: 2026-09-17 16:00
updated: 2026-09-17 16:10
---

# 観察の手順書と、進捗記録を畳む方針

## 依頼

2026-09-17、1 時間枠の無人運転（ステップ 10b1〜11a）の所感への回答として:

- 「2. 観察手順をまとめてください」— 反復 4（ステップ 11a、48 分）で、本家の MOD の操作（敵を出す・God Mode の確かめ直し・捕まってやり直し・W-Editor の押し間違い）に約 12 分かかり、解析のスクリプトも毎回その場で書いていた。
- 「3. 進捗について、記録のたびに内容を端的に要約し、残すべき要点のみにまとめる方針はどう？」— `20260916-tablet-powers.md` が 118 KB（うち決定事項 52 KB）あり、反復のたびに読んで約 4.6 万トークンを使っていた。
- 「4. 無視する」— 使用量の見張り（`--usage-cmd`・上限の文言）は扱わない。
- 1（終了時刻の超過）と 5（ログ・作業ツリーなど）は今回の対象外。

## 計画

- [x] 1. 観察の手順書と道具 … 2026-09-17 完了。`.claude/guides/observation.md`（原則・撮るものの一覧・本家の起動・MOD のメニューの上端の座標〈W-Editor は (990, 487)〉と Maps の座標・場所〈`04_Start`、ZONE 1 は `05_Start` と推定〉・本家で撮る・PIE で同じものを撮る・測る・片付け）、`Tools/video_probe.py`（`frames`・`sheet`・`series`〈`--box`・`--stat`・`--dark`〉・`period`）、`Tools/pie.py`（`state`・`start`・`place`・`cmd`・`stop`）。本家の 11a の収録で 11a の測定値に合うこと（テレキネシスの白飛び・照準の R 140〜237・シャードの周期 20.9〜21.0 s を約 3 秒で）と、PIE の開始 → 配置 → `slomo 0.25` → 停止（未保存なし）を確かめた。CLAUDE.md・verification.md・症状索引・observations/README.md・実装記録 01 と索引を直した
- [ ] 2. 進捗記録を畳む決まり: `progress-tracking.md` の「書き方」、`_template.md`、`/continue`（`SKILL.md` の手順 6）、`session_start_hook.py`（大きすぎる記録を知らせる）、CLAUDE.md の索引の要約。 ← 作業中
  - 変更予定: `.claude/guides/progress-tracking.md`、`.claude/progress/_template.md`、`.claude/skills/continue/SKILL.md`、`.claude/scripts/session_start_hook.py`、`CLAUDE.md`
- [ ] 3. `20260916-tablet-powers.md` を新しい決まりで畳む。消す前に、決定事項が実装記録（01・04・06）にあるかを確かめ、無くて今も効くものは実装記録へ移す。
- [ ] 4. 仕上げ: この記録を消してコミットし、`/clear` をお願いする。

## 次にやること

ステップ 2。`progress-tracking.md` の「書き方」に「畳む」決まり（ステップを閉じるときにそのステップの分を 1 行に、決定事項は今も効くものだけ・実装済みのことは実装記録へ、次にやることは置き換え、再開時の注意は今も有効なものだけ、目安 20 KB・上限 30 KB）を足し、`_template.md`・`/continue` の手順 6・hook（30 KB を超えた記録を知らせる）・CLAUDE.md の索引に反映する。

## 決定事項

- 2026-09-17: この作業は `feature/tablet-powers` でコミットする — 無人運転が走るのはこのブランチで、決まりをすぐ効かせたい。実装記録 01 は main とこのブランチで大きく違い、main で直すとマージで衝突する。main へはステップ 12 のマージで入る。
- 2026-09-17: 進捗記録の方針はユーザーの提案（要約して要点だけ残す）に沿い、「ステップを閉じるときに、そのステップの分を畳む」「上限（30 KB）を超えたら全体を畳む」「消す前に、今も効く決定を実装記録へ移す」を足す — 毎回全体を書き直すと、トークンを使い、要約のたびに中身が少しずつ変わる恐れがあるため。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは開いている（`L_Hospital_Zone1`、PIE なし、未保存なし）。ステップ 1 の PIE の確認は止めてある。

## 検証

- check_records: OK（ステップ 1。01 に `Tools/pie.py`・`Tools/video_probe.py` を足した）
- C++ ビルド: 対象外（C++ は変えない）
- エディタでの確認: `Tools/pie.py` の state（PIE 外・PIE 中）・start・place・cmd・stop と、PIE 外の cmd が「not in PIE」で失敗すること（ステップ 1）
- 解析: `Tools/video_probe.py` を `observations/original/` の 11a の収録で確かめた（ステップ 1）
