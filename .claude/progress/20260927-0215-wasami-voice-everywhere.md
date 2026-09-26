---
title: スコア画面の XP 加算音の除去と、本家のナース・ビアスの声をすべてワサミの声へ
status: ユーザー待ち
branch: main
base: c944245
started: 2026-09-27 02:15
updated: 2026-09-27 02:45
---

# スコア画面の XP 加算音の除去と、本家の声のワサミ化

## 依頼

2026-09-27 の有人セッション:
- 「you escaped 後に、本家である XP ゲージの加算音だけが本作で流れています。除去してください」
- 「本家ナース・ビアスの声を除き、すべてワサミボイスに置き換えてください。」

## 計画

- [x] 1. XP の加算音を鳴らさない … 2026-09-27 完了（13 記録）
- [x] 2〜5. ワサミの台詞 37 本の書き出し・取り込み、置き換え表 `dd_voices.REPLACES`、C++（Zone 1・2 の流れ・タイトル）、シーケンス 4 本・23 区間 … 2026-09-27 完了（表は 10 記録）
- [x] 6. テスト 158 件成功、Development の `game_flow.py run` 通過、Release 用 Shipping と zip（02:42）を作り直した
- [ ] 7. ユーザーが遊んで声の当て方を確かめる ← 次

## 次にやること

ユーザーが zip の Shipping で声を聞き、置き換え表（10 記録の表）の差し替えを指示したら `dd_voices.REPLACES`（シーケンス）と C++ の該当の波の名前を直し、`dd_sequence.replace_voices()` で当て直す。問題が無ければこの記録を閉じる。

## 決定事項

- 2026-09-27: 置き換え表は `dd_voices.REPLACES` の 1 か所。ビアス（案内役）はワサミの字幕付き、ナース（敵）は字幕なし（館内放送 Event_37 は本家どおり字幕付き）。マップは作り直さない（道が消える。前の記録）: 館内放送 Event_48 は流れが鳴らす前に音を差し替える。

## 要確認（ユーザー）

- 2026-09-27: 本家の各台詞に当てるワサミの台詞 — 仮に Claude が意味の近いものを選んだ（表は `dd_voices.REPLACES`）。

## 再開時の注意

- 台詞の素材を作り直すときは `python Tools/dd/prepare_voices.py --lines-only` → `WasamiDDTools.import_wasami_voices()`。

## 検証

- check_records: OK
- テスト: Wasami.* 158 件成功
- パッケージ: Development の `game_flow.py run` 通過、Shipping 0 エラー
- 実際の音を耳で聞く確かめはしていない
