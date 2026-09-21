---
title: 2026-09-21 のユーザーの回答の反映（作業一覧の項目 35）
status: 進行中
branch: main
base: e91ec43
started: 2026-09-21 11:13
updated: 2026-09-21 13:05
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
- [x] 2. Zone 2 の脱出で曲を聞こえる形で引く — `AWasamiMusicPlayer::FadeAllMusicOut(Duration)` を足し、`OnEndTrigger` で `bFadeOut` 真 + `FadeAllMusicOut(1 s)` → `PauseTimeCounter` → 1 s 後に `Escape`。記録 10・11
- [x] 3. EXTRAS の曲の欄 — `SoundTracks`（`FWasamiExtrasTrack`）に本作の曲 4 本（`DD_SoundClass_Music` の音すべて）を名前つきで並べ、`SoundCount` 10 → 4・日記は `DiaryCount` 10 に分けた。記録 19
- [x] 4. Zone 1 の書類が曲 4 本とも解放するようにする — 組み立てに `_collectables` と `COLLECTABLE_SOUNDS` = (0, 1, 2, 3) を足し（本家の Sound の ID を本作の 4 本に置き換える）、`L_Hospital_Zone1` の書類（`ID` 0）にも入れて保存した。記録 01・18・19
- [x] 5. 使っていないアセット 18 個を消した — `/Game/Pipeline/Debug` の `M_Probe_*` 17 個（空になったフォルダーごと）と `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy`。どれも参照 0 件。前処理・取り込みに作り直す経路は無くスクリプトの修正は要らなかった。記録 00・症状索引
- [ ] 6. PIE で通して確かめ、項目 35 を閉じる（実装記録・`handover.md`・`roadmap.md`、進捗記録の削除）

## 次にやること

ステップ 6。PIE で Zone 1 → Zone 2 を通して確かめ、項目 35 を閉じる。見るのは (1) 脱出で曲が**実際に聞こえる形で**引き、スコア画面が 1 s 遅れて出るのが不自然でないか、(2) Zone 1 の書類を取ると EXTRAS の曲 4 本が解放されて鳴るか、(3) 敵の移動音のピッチが巡回と追跡で変わるか。終わったら実装記録・`handover.md`・`roadmap.md`（項目 35 を完了に）を直し、この進捗記録を消して最後のコミットに含める。

## 決定事項

- 2026-09-21（ステップ 2）: 脱出の曲は**止める前にフェードを始め、`Escape` をその 1 s だけ待たせる**。止めたゲームは `Update` が来ないだけでなく**ゲームの音が全部止まる**（`FAudioDevice::HandlePause` が UI の音でない発音体を止める。WebGL 版 06 記録の調べとも一致）ので、止めたのと同じフレームでフェードを始めても聞こえないため。長さは本家のフェード `AWasamiMusicPlayer::FadeDuration` = 1 s。待つ間は画面が黒のまま（`bHold`）で、レベルの時間が伸びないよう引き金で `PauseTimeCounter` を呼ぶ。
- 2026-09-21（ステップ 3）: EXTRAS に並べる曲は「`SoundClassObject` が `DD_SoundClass_Music` の音」で選んだ（本作に 4 本。`66_-_Game_Over` と秘密の部屋の曲・環境音は `DD_SoundClass_SFX` なので曲ではない）。名前は ID 0 だけ**本家の EXTRAS がこの曲に付けている `Cold Hearted`**（`UMG_Extras` の `Extras_Sound_Button_C_14` = 本家の Sound 5 が同じ Zone 1 の曲）、残り 3 本は本家に名前が無いので本家のファイル名から起こした仮の名前。

## 要確認（ユーザー）

- EXTRAS の曲の名前のうち **ID 1〜3 が仮**（`Hospital Panic Track`・`Hospital Zone 2 Normal Track`・`Pause Theme`）。本家の EXTRAS はこの 3 本を並べていないので名前が無く、本家のファイル名から起こした。ID 0 の `Cold Hearted` だけは本家の EXTRAS の名前そのまま。正式な曲名があれば差し替える（`WasamiExtrasWidget.cpp` の `SoundTracks`）。
- Zone 1 の書類（`ID` 0）が**曲 4 本とも解放する**ようにする（本家はこの書類が Sound 5 の 1 本だけ）。本作は章が 1 つで書類もこれ 1 つなので、本家のままだと 3 本が永久に鍵になる。1 本だけにしたい場合は言ってほしい。
- 脱出でスコア画面が**1 s 遅れて**出る（曲のフェードを聞かせるため、その間は画面が黒のまま）。本家はポータルに触れた所ですぐスコア画面が出る。1 s が長ければ短くできる（`AWasamiZone2Flow::EscapeMusicFade`。曲のフェードも同じ長さになる）。

## 再開時の注意

- エディタの状態: ステップ 1 のビルドの後に開き直し、応答している（PIE は動いていない。開いているレベルは `L_Hospital_Zone1`、汚れなし）。
- **Automation テストの回し方**: 画面の前面には項目 33 の要確認のファイアウォールのダイアログ（`PickerHost.exe`）が居座っていてエディタを前面にできない（プロセスの中から `SetForegroundWindow` を呼んでも Windows が断る）。開いたエディタのまま、リモート実行で `unreal.find_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground', False)` → `unreal.SystemLibrary.execute_console_command(None, 'Automation RunTests <filter>')` → `Saved/Logs/wasami_deception.log` の `Test Completed` を読む → 偽にしたものを真に戻す、で回せる（ステップ 1 で確かめた。フレームレートの門が開くまで約 2 分待つ）。パーティクルを見ないテストだけなら `UnrealEditor-Cmd.exe … -Unattended -NullRHI` のほうが速い（症状索引）。

## 検証

- check_records: OK（20 件）
- C++ ビルド: OK（`Tools/editor_cycle.py`。新しい警告なし。`WasamiCapture.cpp` の C4305 5 件は前からのもの）。`editor_cycle.py` は無名名前空間の名前の衝突を先に見るので、`.cpp` に足す補助関数は既にある名前（`MusicTrack` など）を避ける。
- Automation（ステップ 3）: エディタの中で `Wasami.Extras`（5 件）すべて成功（`Screen` は曲 4 本と名前・音、`SoundButton` は解放と再生バーの名前を見ている）。ステップ 2 の `Wasami.Music`・`Wasami.ZoneFlow` も成功済み。
- ステップ 5: 18 個とも参照 0 件を確かめてから消し、レジストリを再走査して「題字なし・`/Game/Pipeline/Debug` なし・リダイレクタ 0」を確かめた。`delete_asset` は 18 個とも真を返したのに題字の `.uasset` だけディスクに残ったので、ファイルを消した（症状索引に書いた）。
- ステップ 4: 組み立ての `_collectables` をエディタで実際に走らせ、置いてある書類が `ID` 0 = SOUND 0・1・2・3、`ID` 1 = ART_GALLERY 19・20 になったのを確かめてレベルを保存した（Zone 2 の書類 `ID` 2 は ART_GALLERY 21・22 だけで Sound を持たないので変わらない。`stage_ue.json` で確認）。
- エディタでの確認（取り込み・組み立て・PIE）: ステップ 6 でまとめて行う（脱出は**実際に聞こえるか**と、スコア画面が 1 s 遅れて出るのが不自然でないかを見る）
