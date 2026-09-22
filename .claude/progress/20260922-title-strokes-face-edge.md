---
title: タイトル画面の筆の跡と、顔の左の境界を本家のものにする（作業一覧の項目 44）
status: 進行中
branch: main
base: 671e4e8
started: 2026-09-22 00:00
updated: 2026-09-22 17:45
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# タイトル画面の筆の跡と、顔の左の境界を本家のものにする（作業一覧の項目 44）

## 依頼

2026-09-22 の有人セッションのレビュー指摘「タイトル画面における背景の筆跡、モチーフキャラクター画像の左の境界線のイメージが本家と異なる」（`.claude/roadmap.md` の項目 44）。同じ日のユーザーの回答「曲も絵も最新版に寄せる」で、この項目を行うと決まった。

完了の条件（作業一覧）:
1. 本家の顔の左の縁（`title_screen_video_mask`・`MM_TitleScreen_Mask`・`MM_TitleScreen_Mask_Grey`）をコンパイル済みのシェーダーで確定し、本作の顔にも同じ切り口を掛ける。
2. 筆の跡のテクスチャと流れ（速さ・色・不透明度）を同じ式で確定する。
3. 本家のタイトル画面と本作の絵を並べ、背景の筆の跡と顔の左の縁が同じ形に見えることを確かめる。

## 計画

- [x] 1. 本家の式と絵を確定した（式は焼き込みのシェーダーと本作の材質で一致。顔の α は 14 記録の「前処理」へ移した）。
- [x] 2・2b. 筆の跡を本家と見比べ、明るさが半分だった原因（UE 5 の Slate の「無効」= α 0.45 倍）を突き止めて `Image_104` を有効にした。**完了の条件 2 を満たした**。
- [x] 3. 顔の α を本家の切り口にした（**この項目の本体**。`Tools/dd/prepare_title.py` の `feather` と `vignette`、`T_TitleFace` を作り直し、14 記録の「前処理」と「変更履歴」）。右端・上端・下端の黒い隙間が消え、左の境界は行ごとに 540〜594 px とばらつく煙の縁になった。**完了の条件 1 を満たした**。
- [ ] 4. 本家と並べて確かめ、項目を閉じる
  - **本家の起動は要らない見込み**（下の「決定事項」の 2）。手元の `Intermediate/DesktopAgent/shots/obs4-title.png`（最新版 v1.9.6、3440 × 1440 の 1/4）と本作の `t44-ours-0.png` を並べ、筆の跡（形・流れ・明るさ。ステップ 2b で確かめ済み）と、顔が箱の四辺まで続くこと・左の境界が煙の縁であることを 1 枚のグリッドにして残す。顔の絵そのものは本家と中身が違うので比べない。
  - `.claude/roadmap.md` の項目 44 を「完了」にし、`.claude/references/handover.md` の「現状と次の一歩」を直して、`python .claude/scripts/check_records.py --update` を通し、この記録を消してコミットする。
  - 変更予定: `.claude/roadmap.md`、`.claude/references/handover.md`

## 次にやること

ステップ 4。`obs4-title.png` と `t44-ours-0.png`（無ければ「再開時の注意」の別窓の PIE で撮り直して `observations/tools/title_fit/cmp.py` で切り出す）を並べたグリッドを作り、作業一覧の項目 44 を完了にして、handover を直し、この記録を消してコミットする。

## 決定事項

- 2026-09-22: **本家の横顔は 9 枚とも「不透明な絵」で、端の暗さは絵の中身**（`UI/Main/TitleScreen/ProfileIcons/` の 9 枚は α > 0.95 が 91.1 %〈monkey〉〜100 %〈5 枚〉）。本作の顔もこの作りに揃えた（α は左の細い羽根だけ、楕円は RGB の暈しへ）。**明るさも本家の幅の中**（輝度 × α の平均は本家 0.022〜0.175、本作 0.051）。
- 2026-09-22: **本家の最新版のタイトルの右側は横顔ではなく細い光の筋**（`obs4-title.png`）。旧版 `UMG_TitleScreen`（本作が写した木）の `Image_97` とは中身が違うので、**顔の絵そのものは並べて比べられない**。比べるのは作り（絵が箱の四辺まで続くか・左の境界が煙の縁か）と筆の跡。ステップ 4 で本家を起動し直す必要は無い。
- 2026-09-22: **撮り方と測り方**（ステップ 4 で使う）。別窓の PIE を 2580 × 1080 で頼むと中身は 2580 × 1082（DPI の倍率 1.0019）、画面の (433, 191)-(3013, 1273)。これを 860 × 360 に縮めると、本家の 3440 × 1440 を 1/4 にした `obs4-title.png` と同じ枠になる。道具は `observations/tools/title_fit/`（`shots.py` = 連写、`cmp.py` = 切り出しと形の比べ、`box.py` = 筆の跡の平地の箱の輝度、`raise_pie.py` = 撮る前に PIE の窓を前へ出す）。
  - **端末の窓が PIE の窓に被る**（画面の (174, 182)-(1303, 817)）。被ったまま測ると値が 2 倍以上に化けるので、`shots.py` は 1 枚ごとに `raise_pie.raise_pie()`（`SetForegroundWindow`）を呼んでから撮る。`Tools/desktop.py` の入力も前面の窓でないと断られる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理は無い（ステップ 4 は絵を並べて記録を直すだけ）。
- **別窓の PIE の出し方**（撮り直すときだけ）: MCP の `ConfigSettingsToolset.ConfigSettingsToolset.SetSectionProperties`（`containerName` `Editor`・`categoryName` `LevelEditor`・`sectionName` `PlayIn`・`propertiesJson` に `NewWindowWidth` 2580・`NewWindowHeight` 1080・`CenterNewWindow` 真）→ エディタで `/Game/Stage/Maps/L_Title` を開く → `EditorToolset.EditorAppToolset.StartPIE`（`options` に `bSimulate` 偽・`playMode` `PlayMode_InEditorFloating`・`warmupSeconds` 3）→ `python observations/tools/title_fit/shots.py <枚数> <間隔 s>` → `python Tools/pie.py stop` → **設定を 1280・720・偽に戻す**（ユーザーの設定）。
- PIE は終わったら必ず止める。`editor_cycle` の後のエディタは Zone 1 を開くので、`L_Title` を開き直してから PIE にする。

## 検証

- check_records: OK（20 件、14 記録のハッシュを更新）
- エディタでの確認（取り込み・PIE）: ステップ 3 で `dd_ui.import_title` から `T_TitleFace`（512²・sRGB）を作り直し、別窓の PIE で 3 枚撮って測った（上の「計画」の 3）。PIE は止め、PIE の窓の設定も戻した。
