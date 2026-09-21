---
title: 2026-09-21 のユーザーの回答の反映（作業一覧の項目 35）
status: 進行中
branch: main
base: e91ec43
started: 2026-09-21 11:13
updated: 2026-09-21 12:05
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 2026-09-21 のユーザーの回答の反映（作業一覧の項目 35）

## 依頼

作業一覧 `.claude/roadmap.md` の項目 35。2026-09-21 の有人セッションで閉じた要確認のうち、コードとアセットの変更が要る 5 つを反映する。

- Zone 2 の脱出で曲が**聞こえる形で**引く（今は `bFadeOut` が真だが、`Escape` が同じフレームでゲームを止めるので 0.5 s のタイマーが回らず、スコア画面の下で鳴り続ける）。止める前に部品を直にフェードアウトする。場所: `WasamiZone2Flow.cpp` の `OnEndTrigger`。
- EXTRAS の**曲の欄 10 個を埋める**（本作で実際に鳴っている曲を名前つきで並べ、余る欄は減らす）。日記 10 は空のまま、絵とクレジットは今のまま。場所: `UWasamiExtrasWidget`。
- 使っていない原作の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy` と `/Game/Pipeline/Debug` の検証用の材質 17 個を**消す**（クックは `/Game` を全部入れるのでパッケージに混ざる。題字は「原作のロゴは使わない」方針にも合わない）。前処理が作り直さないように、取り込みのスクリプト側も直す。

## 計画

- [x] 1. 敵の移動音のピッチと足の運びの `TODO(仮)` — `NurseSkateSpeed` 800 を足し、`MoveVolumeSpeed` = 400 × `MaxSpeed` / `NurseSkateSpeed` = 215・`MovePitchSpeed` = `MaxSpeed` = 430 に移した（巡回 200 でピッチ 1.2・音量 0.93、追跡 430 でピッチ 1.5）。足の運びの `TODO(仮)` も外した。記録 07
- [ ] 2. Zone 2 の脱出で曲を聞こえる形で引く
  - 変更予定: `Source/wasami_deception/WasamiZone2Flow.cpp` の `OnEndTrigger`（`bFadeOut` を立てるだけでなく、止める前に `UAudioComponent` を直にフェードアウトする）、必要なら `WasamiMusicPlayer` に入口を足す、実装記録 10・11
- [ ] 3. EXTRAS の曲の欄 10 個を埋める
  - 変更予定: `Source/wasami_deception/WasamiExtrasWidget.h`/`.cpp`（`SoundCount` 10 を本作で鳴っている曲の数に減らし、名前を並べる）、実装記録 19
- [ ] 4. 使っていないアセットを消し、前処理が作り直さないようにする
  - 変更予定: `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`・`/Game/Pipeline/Debug` の `M_Probe_*` 17 個（エディタで削除）、`Tools/dd/prepare_level_title.py`・`Content/Python/wasami_tools/pipeline/dd_ui.py`（題字を取り込まないようにする）、実装記録 01・09
- [ ] 5. PIE で 4 つを通して確かめ、項目 35 を閉じる（実装記録・`handover.md`・`roadmap.md`、進捗記録の削除）

## 次にやること

ステップ 2。`WasamiZone2Flow.cpp` の `OnEndTrigger`（446 行、`Music->bFadeOut = true;` は 471-474 行）で、`Escape` がゲームを止める前に曲の `UAudioComponent` を直にフェードアウトする（`bFadeOut` の 0.5 s のタイマーは止まったフレームでは回らない）。必要なら `AWasamiMusicPlayer` に入口を足す。C++ なので`python Tools/editor_cycle.py` のビルドが要る。

## 決定事項

- 2026-09-21: アセットの削除は**ユーザーの指示済み**（項目 35 の完了の条件。2026-09-21 の有人セッションの回答）なので、無人運転で行ってよい。ただし消すのはこの 18 個だけ。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 下調べで分かっていること（ステップに入る前に読み直さなくてよい）:
  - 脱出の曲: `WasamiZone2Flow.cpp:446` の `OnEndTrigger`。471-474 行で `Music->bFadeOut = true;` を立てている（コメントに「聞こえない」理由が書いてある）。
  - EXTRAS: `WasamiExtrasWidget.h:78` の `SoundArchiveSection` 3、記録 19 に `SoundCount` 10。
  - 消すアセット: `Content/Pipeline/Debug/M_Probe_*.uasset` 17 個と `Content/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy.uasset`。**どちらも git の管理外**（`git ls-files` が空）なので、消してもコミットには出ない。`M_Probe_*` を作る Python は無く（`grep -rn "M_Probe" --include=*.py` が空）、検証のときに MCP で直に作ったものらしい。題字は `dd_ui.py:165-167` の取り込みの一覧には**もう入っていない**（同じ `TitleCards` の 3 枚だけ）ので、`Tools/dd/prepare_level_title.py:26` が前処理で書き戻さないかを確かめる。
- エディタの状態: ステップ 1 のビルドの後に開き直し、応答している（PIE は動いていない）。
- **Automation テストの回し方**: 画面の前面には項目 33 の要確認のファイアウォールのダイアログ（`PickerHost.exe`）が居座っていてエディタを前面にできない（プロセスの中から `SetForegroundWindow` を呼んでも Windows が断る）。開いたエディタのまま、リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` → `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests <filter>')` → `Saved/Logs/wasami_deception.log` の `Test Completed` を読む → 偽にしたものを真に戻す、で回せる（ステップ 1 で確かめた。フレームレートの門が開くまで約 2 分待つ）。パーティクルを見ないテストだけなら `UnrealEditor-Cmd.exe … -Unattended -NullRHI` のほうが速い（症状索引）。

## 検証

- check_records: OK（20 件。記録 07 のハッシュを更新）
- C++ ビルド: OK（`Tools/editor_cycle.py`。新しい警告なし）
- Automation（ステップ 1）: `Wasami.Enemy` 17 件すべて成功。移動音の `Wasami.Enemy.Actor.Sound` は新しい値（0 で無音・107.5 で音量 0.5・200 で 0.93・215 で満・322.5 でピッチ 1.35・430 で 1.5・2000 で頭打ち）で成功。`-NullRHI` の `UnrealEditor-Cmd.exe` では `Chase06` が落ちるが、これはパーティクルが作られない `-NullRHI` のせいで（`Defib.Charge`・`ZoneBarrier.Actor` も同じ理由で落ちる）、エディタの中では成功する（症状索引に書いた）。
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 5 でまとめて行う
