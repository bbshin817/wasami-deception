---
title: 項目 51 原作のナースの絵のテクスチャ 3 枚をワサミの絵に替える
status: 進行中
branch: main
base: 1b21586
started: 2026-09-23 05:52
updated: 2026-09-23 05:52
---

# 項目 51 原作のナースの絵のテクスチャ 3 枚をワサミの絵に替える

## 依頼

作業一覧 `.claude/roadmap.md` の項目 51（大目標 4）。2026-09-23 のユーザーの回答「今後ワサミの絵に変える」。ステージの中にナースの姿が描かれたテクスチャが 3 枚あるので、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）で替える。

- `hospital_poster_nurse_01_D`（材質 `M_06_Hospital_Poster_01`、1024×1024 RGB・`PF_DXT1`・sRGB）… 紙袋をかぶったナースが拳を上げ、吹き出しに「TAKE YOUR MEDICINE!」。水色の集中線とハーフトーンの地。**Zone 1 に 16 枚**。
- `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`、1024×1024 RGBA・`PF_DXT5`・decal のマスター）… 救急車の屋根に座ったナースが大きな注射器を槍のように構える漫画。**Zone 1 に 1 枚・Zone 2 に 2 枚**（`/Engine/BasicShapes/Plane` に貼る）。
- `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`、1024×1024 RGB・`PF_DXT1`・sRGB）… 赤地に黒い影絵（注射器を構える背の高い人物と、ハートを浮かべる小さい人物）と「GET VACCINATED!」。**Zone 1 に 7 枚**。

完了の条件（作業一覧より）: (1) 3 枚それぞれ、元の構図と文言はそのままにナースをワサミに置き換えた絵を前処理で描く（大きさ・圧縮の設定は元に合わせる）。(2) 材質 3 つのテクスチャを差し替える（原作の材質の式は変えない）。(3) PIE で Zone 1・Zone 2 の貼ってある場所を撮り、ナースの姿が 1 枚も残っていないことを確かめる。(4) `.claude/guides/original-fidelity.md` の表に 3 行足す。

## 計画

- [ ] 1. 前処理 `Tools/dd/prepare_nurse_posters.py` を作り、`hospital_poster_nurse_01_D` のワサミ版を描く（`--preview` で元と並べたシートを書いて目で確かめる）
  - 変更予定: `Tools/dd/prepare_nurse_posters.py`（新規）、出力 `Intermediate/Pipeline/wasami/stage/*.png`
- [ ] 2. 同じ前処理で残り 2 枚（`hospital_decal_nurseambulance`・`hospital_poster_nurse_02`）を描く
  - 変更予定: `Tools/dd/prepare_nurse_posters.py`
- [ ] 3. 取り込みと材質の差し替え: `Tools/dd/prepare_stage.py` の `note_texture` が 3 枚を `/Game/Wasami/Stage/...` とワサミ版の png へ向けるようにし、エディタで取り込み → 材質 3 つを作り直して保存
  - 変更予定: `Tools/dd/prepare_stage.py`、`/Game/Wasami/Stage/T_Hospital_Poster_Nurse_01`・`T_Hospital_Decal_NurseAmbulance`・`T_Hospital_Poster_Nurse_02`、`/Game/DD/Materials/06_Hospital/M_06_Hospital_Poster_01`・`M_06_Hospital_Poster_14`・`M_06_Hospital_Decal_NurseAmbulance`
- [ ] 4. PIE で Zone 1・Zone 2 の貼ってある場所を撮ってナースが 1 枚も残っていないことを確かめ、`.claude/guides/original-fidelity.md` の表に 3 行足し、実装記録 01 を直して項目を閉じる
  - 変更予定: `.claude/guides/original-fidelity.md`、`.claude/implementation-records/01-*`、`.claude/roadmap.md`

## 次にやること

ステップ 1。`Tools/dd/prepare_portal_logo.py` の作り（PIL + numpy、`pak_reference_2` の png を読み `Intermediate/Pipeline/wasami/...` に書く）に倣って `Tools/dd/prepare_nurse_posters.py` を作り、`hospital_poster_nurse_01_D` のナースを消してワサミに置き換える。WebGL 版のやり方（13 記録）= 人物の箱を周りの色でぼかして塗りつぶし、そこへワサミの絵（`SourceArt/Wasami/UI/title_face.png` の写真、または `pause_head.png`・`wasami_symbol.png` の白抜き）を置く。題字・吹き出し・地は残す。

## 決定事項

- 2026-09-23: 差し替えは**テクスチャの取り込み元を替える**やり方にする — `prepare_stage.py` の `note_texture` で 3 枚の `file`（ワサミ版の png）と `asset`（`/Game/Wasami/Stage/...`）を差し替えれば、`dd_stage.make_material` が材質の `albedo`/`emissive` パラメータにそのまま新しいテクスチャを入れる。材質の式（マスター）には触らない。作業一覧の「`/Game/Wasami` に取り込み、材質のテクスチャを差し替える」どおり。
- 2026-09-23: 4 枚目の `hospital_poster_nurse_03`（「BURN FAT」と炎）は**人物が描かれていないので対象外**（絵を見て確かめた）。`hospital_nurse_statue_*` は本作の Zone 1・Zone 2 が使っていない（`stage_ue.json` の textures に無い）。`nurse-low5_items_*`（材質 `M_06_Nurse_Items`）は Zone 2 の独房の天井の注射針のメッシュが使うだけで、キャラクターの姿ではない。
- 2026-09-23: 作業一覧の「`hospital_poster_nurse_02` はどのレベルからも使っていない」は**誤り**（`stage_ue.json` で Zone 1 のポスター枠 7 枚が材質 `M_06_Hospital_Poster_14` を持つ）。作業一覧の記述をこの計画のコミットで直した。

## 要確認（ユーザー）

- 2026-09-23: 3 枚のワサミの絵の見た目 — 仮に WebGL 版の CC2 のポスターと同じ考え方（人物の箱を周りの色で埋め、ワサミの写真か白抜きの顔を元の人物の大きさ・位置に置く）で描く。理由: 本家に無い本作のものなので根拠は本作の今までの決めごと（`.claude/references/webgl/implementation-records/13-asset-pipeline.md`）。場所: `Tools/dd/prepare_nurse_posters.py`。出来た絵は `Intermediate/Pipeline/wasami/stage/_sheet.png`（元と並べたシート）で見てもらう。
- 2026-09-23: 差し替えで使わなくなる原作のテクスチャ `/Game/DD/Textures/06_Hospital/hospital_poster_nurse_01_D`・`hospital_poster_nurse_02`・`hospital_decal_nurseambulance` の扱い — 仮に**消さずに残す**（アセットの削除はユーザーの確認が要る。`.claude/guides/verification.md`）。どのレベルからも参照されなくなるが `bCookAll=True` でパッケージには入る。消してよいか確認したい。

## 再開時の注意

- 長時間処理はまだ無い。ステップ 3 でエディタの取り込み（`Tools/ue_remote.py` 経由）と材質の作り直しが入る。
- エディタの状態は未確認（このステップでは触っていない）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目では C++ を触らない見込み）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
