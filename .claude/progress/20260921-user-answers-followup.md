---
title: 2026-09-21 のユーザーの回答の反映（作業一覧の項目 35）
status: 進行中
branch: main
base: e91ec43
started: 2026-09-21 11:13
updated: 2026-09-21 11:13
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 2026-09-21 のユーザーの回答の反映（作業一覧の項目 35）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 35。2026-09-21 の有人セッションで閉じた要確認のうち、コードとアセットの変更が要る 5 つを反映する。

- 敵の移動音のピッチの割り当てを**本作の速さ**に合わせる（本家は 400〜800 cm/s → 1.2〜1.5 で、本作の巡回 200・追跡 430 では 1.2225 までしか上がらない）。波は本家の `DD_Rollerskating_Fast_V1_LOOP` のまま。場所: `WasamiEnemy.h` の `MoveVolumeSpeed` ほか。
- Zone 2 の脱出で曲が**聞こえる形で**引く（今は `bFadeOut` が真だが、`Escape` が同じフレームでゲームを止めるので 0.5 s のタイマーが回らず、スコア画面の下で鳴り続ける）。止める前に部品を直にフェードアウトする。場所: `WasamiZone2Flow.cpp` の `OnEndTrigger`。
- EXTRAS の**曲の欄 10 個を埋める**（本作で実際に鳴っている曲を名前つきで並べ、余る欄は減らす）。日記 10 は空のまま、絵とクレジットは今のまま。場所: `UWasamiExtrasWidget`。
- 使っていない原作の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy` と `/Game/Pipeline/Debug` の検証用の材質 17 個を**消す**（クックは `/Game` を全部入れるのでパッケージに混ざる。題字は「原作のロゴは使わない」方針にも合わない）。前処理が作り直さないように、取り込みのスクリプト側も直す。
- 敵の足の運びの再生の速さの `TODO(仮)` を外す（巡回 200・追跡 430 では `Walk` = 1.5・`Run` = 0.96 でどちらも上限・下限に当たらず、保留のときの懸念は起きない）。場所: `WasamiEnemyAnimInstance.h`。

## 計画

- [ ] 1. 敵の移動音のピッチと足の運びの `TODO(仮)`（C++ のヘッダー 2 つ。ビルド 1 回で済むのでまとめる）
  - 変更予定: `Source/wasami_deception/WasamiEnemy.h`（`MoveVolumeSpeed` 400 / `MovePitchSpeed` 800 を本作の巡回 200・追跡 430 に合わせて割り直す。`MoveMinPitch` 1.2・`MoveMaxPitch` 1.5 は本家のまま）、`Source/wasami_deception/WasamiEnemyAnimInstance.h`（104 行の `TODO(仮)` を外して、なぜ仮でなくなったかを書く）、実装記録 07
- [ ] 2. Zone 2 の脱出で曲を聞こえる形で引く
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.cpp` の `OnEndTrigger`（`bFadeOut` を立てるだけでなく、止める前に `UAudioComponent` を直にフェードアウトする）、必要なら `WasamiMusicPlayer` に入口を足す、実装記録 10・11
- [ ] 3. EXTRAS の曲の欄 10 個を埋める
  - 変更予定: `Source/wasami_deception/WasamiExtrasWidget.h`/`.cpp`（`SoundCount` 10 を本作で鳴っている曲の数に減らし、名前を並べる）、実装記録 19
- [ ] 4. 使っていないアセットを消し、前処理が作り直さないようにする
  - 変更予定: `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`・`/Game/Pipeline/Debug` の `M_Probe_*` 17 個（エディタで削除）、`Tools/dd/prepare_level_title.py`・`Content/Python/wasami_tools/pipeline/dd_ui.py`（題字を取り込まないようにする）、実装記録 01・09
- [ ] 5. PIE で 4 つを通して確かめ、項目 35 を閉じる（実装記録・`handover.md`・`roadmap.md`、進捗記録の削除）

## 次にやること

ステップ 1。`WasamiEnemy.h` の移動音の速さの割り当て（`MoveVolumeSpeed` 400・`MovePitchSpeed` 800）を本作の巡回 200・追跡 430 に合わせて割り直し、`WasamiEnemyAnimInstance.h` の `TODO(仮)`（104 行）を外す。どちらも `static constexpr` の定数とコメントなので、`python Tools/editor_cycle.py` のビルド 1 回で両方入る。

## 決定事項

- 2026-09-21: ステップ 1 で移動音と足の運びをまとめる — どちらも `AWasamiEnemy` まわりのヘッダーの定数で、C++ のビルド 1 回（数分）を 2 回に分けない方が速いため。
- 2026-09-21: アセットの削除は**ユーザーの指示済み**（項目 35 の完了の条件。2026-09-21 の有人セッションの回答）なので、無人運転で行ってよい。ただし消すのはこの 18 個だけ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 下調べで分かっていること（ステップに入る前に読み直さなくてよい）:
  - 移動音: `WasamiEnemy.h:161-167` に `MoveVolume` 0・`MoveVolumeSpeed` 400・`MoveVolumeInterp` 5・`MovePitchSpeed` 800・`MoveMinPitch` 1.2・`MoveMaxPitch` 1.5・`MovePitchInterp` 2。使うのは `WasamiEnemy.cpp:218-222` の `GetMappedRangeValueClamped` 2 つ。
  - 足の運び: `WasamiEnemyAnimInstance.h:104-108` の `TODO(仮)` と `WalkStrideSpeed` 133。
  - 脱出の曲: `WasamiZone2Flow.cpp:446` の `OnEndTrigger`。471-474 行で `Music->bFadeOut = true;` を立てている（コメントに「聞こえない」理由が書いてある）。
  - EXTRAS: `WasamiExtrasWidget.h:78` の `SoundArchiveSection` 3、記録 19 に `SoundCount` 10。
  - 消すアセット: `Content/Pipeline/Debug/M_Probe_*.uasset` 17 個と `Content/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy.uasset`。**どちらも git の管理外**（`git ls-files` が空）なので、消してもコミットには出ない。`M_Probe_*` を作る Python は無く（`grep -rn "M_Probe" --include=*.py` が空）、検証のときに MCP で直に作ったものらしい。題字は `dd_ui.py:165-167` の取り込みの一覧には**もう入っていない**（同じ `TitleCards` の 3 枚だけ）ので、`Tools/dd/prepare_level_title.py:26` が前処理で書き戻さないかを確かめる。
- エディタの状態: 未確認（このセッションでは触っていない）。ステップ 1 は C++ なので `python Tools/editor_cycle.py` から始める。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
