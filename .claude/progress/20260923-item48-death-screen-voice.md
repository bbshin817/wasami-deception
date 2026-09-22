---
title: 項目 48 ライフ 0 の死亡画面でワサミの声を鳴らさない
status: 進行中
branch: main
base: 3369a84
started: 2026-09-23 04:15
updated: 2026-09-23 04:15
---

# 項目 48 ライフ 0 の死亡画面でワサミの声を鳴らさない

## 依頼

作業一覧 `.claude/roadmap.md` の項目 48（大目標 4「レビュー指摘の修正」）。

2026-09-23 のユーザーの回答「死亡画面を黙らせる」（項目 40 の要確認）。顔の捕獲でライフ 0 になると、捕獲が 0.2 s に `Over`（あっ、終わりです）を鳴らし、死亡画面のゲームオーバーが 1.90 s に**同じ声をもう一度**鳴らす（間の無音 0.98 s）。**死亡画面の側を鳴らさない**ことにする。

完了の条件（作業一覧より）:

1. `UWasamiDeathScreenWidget` のゲームオーバーの段が声を鳴らさない（`GameOverVoiceSound` と `EStep::GameOver` の `PlaySound`）。**ライフが残るときの `Fine` は変えない**。
2. ホテル型の捕獲（`You` が t = 0）でライフ 0 になったときはゲームオーバーの声がまったく無くなるので、`66_-_Game_Over` の曲だけで間が持つかを PIE で見る（本家はここで Bierce の台詞が鳴る）。
3. PIE で顔 × ライフ 0 とホテル型 × ライフ 0 を録り、`Over` が 1 回だけ鳴ることを波で確かめる。テスト（`Wasami.GameFlow.*`・死亡画面の木を見るもの）を直す。

## 計画

- [ ] 1. 死亡画面のゲームオーバーの段から声を外す（C++ とテストと実装記録） ← 次
  - 変更予定: `Source/wasami_deception/WasamiDeathScreenWidget.h`（`GameOverVoiceSound` の宣言 241 行）、`Source/wasami_deception/WasamiDeathScreenWidget.cpp`（151 行の代入・514 行の `PlaySound`・509 行からのコメント）、`.claude/implementation-records/09-ui.md`、`.claude/implementation-records/10-audio.md`
  - `LifeVoiceSound`（`Fine`）と `EStep::LifeLost` は触らない。
  - `Tools/editor_cycle.py` でビルドし、`Wasami.DeathScreen.*`・`Wasami.GameFlow.*`・`Wasami.Capture.*`・`Wasami.Voice.*` を走らせる。
- [ ] 2. PIE で顔 × ライフ 0 とホテル型 × ライフ 0 を録って確かめ、項目を完了にする
  - 録り方は実装記録 07 の「確かめたこと（2026-09-22）」と同じ（`au.DisableAppVolume 1` → `AudioMixerLibrary.start_recording_output` で主サブミックスを録り、原本 `SourceArt/Wasami/Voices/*.wav` との相互相関で時刻を測る。手順は症状索引）。時刻を測る回は `bThrottleCPUWhenNotForeground` を偽にする。
  - 顔（`Wasami.Capture 3`）× ライフ 0: `Over` が 0.2 s の 1 回だけで、1.90 s にもう一度出ないこと。
  - ホテル型（`Wasami.Capture 0`）× ライフ 0: 声は `You`（t = 0）だけで、死亡画面に声が無いこと。`66_-_Game_Over` の曲が鳴っていて間が持つかを見る（持たないと判断したら、本家と違う作りは足さずに要確認へ書く）。
  - ライフ 0 にするには、捕獲の前に `Wasami.Lives 1` を送る（死亡画面の Construct がライフを −1 するので、1 から 0 になってゲームオーバーの段に入る。`WasamiGameMode.cpp` の `LivesCommand`・09 記録）。
  - 確かめた結果を 09 記録（死亡画面）と 10 記録（声）に書き、作業一覧の項目 48 を「完了（日付）」にし、進捗記録を消して 1 コミットにする。

## 次にやること

ステップ 1。`WasamiDeathScreenWidget.cpp` の `EStep::GameOver`（509〜520 行）から `PlaySound(GameOverVoiceSound, VoiceVolume);` を外し、`GameOverVoiceSound`（.h 241 行・.cpp 151 行）を消して、コメント（「ワサミの `Over` が Bierce のゲームオーバーの台詞の代わり」）を今の作り（声は鳴らさない・理由はユーザーの 2026-09-23 の回答）に書き直す。`python Tools/editor_cycle.py` でビルドしてテストを走らせ、09・10 記録を直して `check_records.py --update` を通す。

## 決定事項

- 2026-09-23: 死亡画面のゲームオーバーの声は**消す**（`GameOverVoiceSound` ごと外す）— ユーザーの回答「死亡画面を黙らせる」。捕獲の側（`Over` 0.2 s・`You` t = 0）は項目 40 で決めたまま触らない。
- 2026-09-23: 本家はここで Bierce の台詞を鳴らすが、**本作では声を足さない**（ユーザーの回答が「黙らせる」なので、代わりの声も入れない）。曲 `66_-_Game_Over` だけになる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- まだ何も変更していない（base 3369a84 = クリーンな main）。
- C++ を変えるので、ステップ 1 は `python Tools/editor_cycle.py`（エディタを閉じる → ビルド → 開き直す）が要る。エディタを閉じる前に保存し、PIE が残っていたら止める。
- 音を録る手順（ステップ 2）は実装記録 07 の「確かめたこと（2026-09-22）」と症状索引の「PIE で音が録れない」の節にある。PIE は終わったら必ず止める。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
