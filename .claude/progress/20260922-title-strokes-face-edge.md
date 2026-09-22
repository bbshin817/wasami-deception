---
title: タイトル画面の筆の跡と、顔の左の境界を本家のものにする（作業一覧の項目 44）
status: 進行中
branch: main
base: 671e4e8
started: 2026-09-22 00:00
updated: 2026-09-22 00:00
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

- [ ] 1. 本家の式と絵を確定する（読むだけ。コードは変えない）
  - `python Tools/dd/cooked_shaders.py "TitleScreen/MM_TitleScreen_Mask_Grey." --show 4` と `"TitleScreen/MM_TitleScreen_Mask."`・`"TitleScreen/MM_TitleScreen_Mask_."` を読み、筆の跡の **UV のタイリング・Panner の速さ・色（Desaturation の割合）・Opacity の式**を確定する（14 記録の「筆の跡の材質」に書いた U 0.35 / V 1・速さ 0.02・`saturate(mask.A × strokes.A)` が本当にシェーダーのとおりかを確かめる）。
  - 本家の顔 `title_screen_profile_monkey`（1024²・RGBA・α 0〜255）の α の形を測る（どこで切れているか、縁がどれだけぼけているか）。本作の顔は 512² に WebGL 版の楕円のラジアルグラデーションを焼いてあるので、ここが食い違いの本体。
  - 煙 `title_screen_video_mask`（1920 × 1200・黒・α が煙）が顔の箱のどこに掛かるかを、ウィジェットの座標から計算する（顔は右端の中央から (−1089.6, −549.2) の 1100 四方、煙は左端から幅 2029.65 で全高、筆の跡は左端から幅 1654.65 で全高。描く順は 顔 → 煙 → 筆の跡 で本家と同じ。旧版・最新版でキャンバスの 13 枠は同一と確認済み）。
  - 変更予定: なし（分かったことをこの記録の「決定事項」に書く）
- [ ] 2. 筆の跡の材質を本家の式どおりに直す
  - ステップ 1 で差が出たところを `Content/Python/wasami_tools/pipeline/dd_ui.py` の `_build_title_strokes` に反映し、`dd_ui.import_title` で作り直して保存する。差が無ければ「差が無いことを確かめた」と書いて閉じる。
  - PIE の `L_Title` で筆の跡が流れることを確かめる（速さは `Tools/video_probe.py period` で測れる）。
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_ui.py`、`/Game/DD/UI/Main/TitleScreen/MM_TitleScreen_Mask_Grey`、実装記録 14
- [ ] 3. 顔の左の縁を本家の切り口にする
  - `Tools/dd/prepare_title.py` の焼き込み（`MASK_CENTRE`・`MASK_STOPS`）を、本家の顔の α の形（ステップ 1）に合わせて見直す。**楕円のグラデーションをやめて煙のマスクに切らせるのか、縁のぼけだけ本家に合わせるのかは、ステップ 1 の測りで決める**（WebGL 版の CSS は本家の切り口ではないので、最新版に寄せるというユーザーの回答に従う）。
  - `python Tools/dd/prepare_title.py` → `dd_ui.import_title` で `/Game/Wasami/UI/Title/T_TitleFace` を作り直し、PIE の `L_Title` で撮る。
  - 変更予定: `Tools/dd/prepare_title.py`、`/Game/Wasami/UI/Title/T_TitleFace`、実装記録 14
- [ ] 4. 本家と並べて確かめ、項目を閉じる
  - 本家の最新版（`Launch-Latest.cmd`）のタイトルを撮り（`.claude/guides/observation.md` の作法。既に `Intermediate/DesktopAgent/shots/obs-title.png`・`obs3-title.png`・`obs4-title.png` に 0.25 倍の絵がある。等倍が要るなら 1 回だけ起動する）、本作の PIE の絵と並べて、**筆の跡の形と流れ・顔の左の縁**が同じに見えることを確かめる。
  - 実装記録 14 と `.claude/roadmap.md` の項目 44 を直し、`python .claude/scripts/check_records.py --update` を通して、この記録を消してコミットする。
  - 変更予定: `.claude/implementation-records/14-title.md`、`.claude/roadmap.md`、`.claude/references/handover.md`

## 次にやること

ステップ 1。`python Tools/dd/cooked_shaders.py "TitleScreen/MM_TitleScreen_Mask_Grey." --show 4` で筆の跡の式を読み、本家の顔 `pak_reference/DDeception/Content/UI/Main/TitleScreen/title_screen_profile_monkey.png` の α の形を測って、この記録の「決定事項」に書く（コードは変えない）。

## 決定事項

- 2026-09-22: **項目 43 の完了を待たずに項目 44 を始める** — 項目 44 の依存「43（同じ画面を触るので後に）」は、43 の C++ とアセットの変更（完了の条件 (1)(2)）が済んでいるので満たされている。43 に残るのはパッケージ版での確かめ（`RunUAT.bat` の許可待ち）だけで、タイトルの画面を触る作業ではない。
- 2026-09-22: **本家のウィジェットの作りは旧版と最新版で同一** — `UMG_TitleScreen.json` のキャンバスの 13 枠（`Image_97` の顔・`VideoMask`・`Image_104` の筆の跡を含む）が両版でアンカーも座標も同じ。本作の `UWasamiTitleScreenWidget::BuildTree` も同じ順・同じ座標なので、**枠と重なりの順は直す対象ではない**。直すのは顔の絵の α と筆の跡の材質の式。
- 2026-09-22: **本家の顔は α を持つ切り抜き**（`title_screen_profile_monkey` 1024² RGBA・α 0〜255）。本作の顔は不透明な写真に WebGL 版の CSS の楕円マスクを焼いたもの（`Tools/dd/prepare_title.py` の `MASK_CENTRE`・`MASK_STOPS`）。**「左の境界が違う」の本体はここ**という見立てでステップ 1 の測りに入る。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 長時間処理はまだ無い。ステップ 2・3 の `dd_ui.import_title` は `python Tools/ue_remote.py` でエディタに投げる（数秒）。ステップ 3 の `prepare_title.py` は PIL と numpy を使うのでエディタの外の Python で走らせる。
- エディタの状態: このステップでは触っていない。ステップ 2 に入る前に PIE が残っていないかを確かめる。
- 本家の実機を起動するのはステップ 4 だけ（起動の作法は `.claude/guides/verification.md`・`.claude/guides/observation.md`。エディタと同時に動かさない）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目は C++ を変えない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
