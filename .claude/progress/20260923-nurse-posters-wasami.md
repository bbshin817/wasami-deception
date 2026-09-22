---
title: 項目 51 原作のナースの絵のテクスチャ 3 枚をワサミの絵に替える
status: 進行中
branch: main
base: 1b21586
started: 2026-09-23 05:52
updated: 2026-09-23 07:30
---

# 項目 51 原作のナースの絵のテクスチャ 3 枚をワサミの絵に替える

## 依頼

作業一覧 `.claude/roadmap.md` の項目 51（大目標 4）。2026-09-23 のユーザーの回答「今後ワサミの絵に変える」。ステージの中にナースの姿が描かれたテクスチャが 3 枚あるので、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）で替える。

- `hospital_poster_nurse_01_D`（材質 `M_06_Hospital_Poster_01`、1024×1024 RGB・`PF_DXT1`・sRGB）… 紙袋をかぶったナースが拳を上げ、吹き出しに「TAKE YOUR MEDICINE!」。**Zone 1 に 16 枚**。
- `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`、1024×1024 RGBA・`PF_DXT5`・decal のマスター）… 救急車の屋根に座ったナースが大きな注射器を構える漫画。**Zone 1 に 1 枚・Zone 2 に 2 枚**（`/Engine/BasicShapes/Plane` に貼る）。
- `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`、1024×1024 RGB・`PF_DXT1`・sRGB）… 赤地に黒い影絵と「GET VACCINATED!」。**Zone 1 に 7 枚**。

完了の条件（作業一覧より）: (1) 3 枚それぞれ、元の構図と文言はそのままにナースをワサミに置き換えた絵を前処理で描く（大きさ・圧縮の設定は元に合わせる）。(2) 材質 3 つのテクスチャを差し替える（原作の材質の式は変えない）。(3) PIE で Zone 1・Zone 2 の貼ってある場所を撮り、ナースの姿が 1 枚も残っていないことを確かめる。(4) `.claude/guides/original-fidelity.md` の表に 3 行足す。

## 計画

- [x] 1. 前処理 `Tools/dd/prepare_nurse_posters.py` を作り、`hospital_poster_nurse_01_D` のワサミ版を描いた
- [x] 2. 同じ前処理で残り 2 枚（`hospital_poster_nurse_02`・`hospital_decal_nurseambulance`）を描いた（実装記録 01 の「ナースの絵をワサミに替える」に 2 行足した）
- [x] 3. ワサミ版 3 枚を `/Game/Wasami/Stage/T_*` に取り込み、材質 3 つがそれを指すようにした（実装記録 01 に前処理の差し替えと取り込みを書いた）
- [ ] 4. PIE で Zone 1・Zone 2 の貼ってある場所を撮ってナースが 1 枚も残っていないことを確かめ、`.claude/guides/original-fidelity.md` の表に 3 行足し、実装記録 01 を直して項目を閉じる
  - 変更予定: `.claude/guides/original-fidelity.md`、`.claude/implementation-records/01-*`、`.claude/roadmap.md`

## 次にやること

ステップ 4。PIE で貼ってある場所を撮り、ナースが 1 枚も残っていないことを確かめて項目を閉じる。
- 撮る所（`Intermediate/Pipeline/dd/stage_ue.json` の placements から座標を引ける）: Zone 1 の `M_06_Hospital_Poster_01` 16 枚・`M_06_Hospital_Poster_14` 7 枚・救急車の落書き 1 枚、Zone 2 の落書き 2 枚。`Tools/pie.py start` → `place X Y --yaw N` → `Tools/desktop.py shot`、終わったら `Tools/pie.py stop`。
- 確かめたら `.claude/guides/original-fidelity.md` の表に 3 行足し、実装記録 01 を仕上げ、`.claude/roadmap.md` の項目 51 を完了にして進捗記録を消す。

## 決定事項

- 2026-09-23: 4 枚目の `hospital_poster_nurse_03`（「BURN FAT」と炎）は**人物が描かれていないので対象外**（絵を見て確かめた）。`hospital_nurse_statue_*` は本作の Zone 1・Zone 2 が使っていない（`stage_ue.json` の textures に無い）。`nurse-low5_items_*`（材質 `M_06_Nurse_Items`）は Zone 2 の独房の天井の注射針のメッシュが使うだけで、キャラクターの姿ではない。ステップ 4 の表もこの 3 枚だけ書く。

## 要確認（ユーザー）

- 2026-09-23: 3 枚のワサミの絵の見た目 — 仮に WebGL 版の CC2 のポスターと同じ考え方（人物の頭を周りの色で埋め、ワサミの頭を元の大きさ・位置に置く）で描いた。理由: 本家に無い本作のものなので根拠は本作の今までの決めごと（`.claude/references/webgl/implementation-records/13-asset-pipeline.md`）。場所: `Tools/dd/prepare_nurse_posters.py`。出来た絵は `python Tools/dd/prepare_nurse_posters.py --preview` が書く `Intermediate/Pipeline/wasami/stage/_nurse_sheet.png`（元と新しいものが 3 行で並んだシート）で見てもらう。
- 2026-09-23: `hospital_poster_nurse_02` の影絵は**2 人とも頭を替えた** — 絵は注射器を構える人物と注射される人物の 2 人で、片方だけワサミにすると 1 枚のポスターに人間とワサミが並ぶ。本作は原作のキャラクターを出さないので両方をワサミにした。片方（注射器を持つ方だけ）にしたいなら `poster_02` の for 文から一方を外す。
- 2026-09-23: 差し替えで使わなくなる原作のテクスチャ `/Game/DD/Textures/06_Hospital/hospital_poster_nurse_01_D`・`hospital_poster_nurse_02`・`hospital_decal_nurseambulance` の扱い — 仮に**消さずに残す**（アセットの削除はユーザーの確認が要る。`.claude/guides/verification.md`）。どのレベルからも参照されなくなるが `bCookAll=True` でパッケージには入る。消してよいか確認したい。

## 再開時の注意

- エディタは起動していて `L_Hospital_Zone1` を開いている（PIE でない・未保存なし）。ステップ 3 の取り込みと保存は済んでいる。
- ステップ 3 でやり直したいときは `python Tools/ue_remote.py tmp/import_nurse.py`（取り込み → 材質 3 つの作り直し → 保存。git の対象外）。
- `tmp/nurse/` はステップ 1・2 で座標を読むために撮った拡大図（git の対象外、消してよい）。

## 検証

- ステップ 1・2: `python Tools/dd/prepare_nurse_posters.py --preview` が 3 枚とも通り、`_nurse_sheet.png` と拡大図を目で確かめた。01 は紙袋の頭がワサミに、02 は影絵 2 つの頭がワサミの線画に（注射器・ハート・文字・体はそのまま）、落書きはナース帽と紙袋の頭が消えてワサミの頭に（注射器・腕・白衣・救急車はそのまま）。
- 差分の数え上げ: 02 で変わった画素は x 37–433・y 31–640 の中だけ（文字の所は 0）。落書きは α の最大が 194 のまま、消えたのは x 495–576・y 145–292（帽子と頭）だけ、新しく描いた画素の α は 166 以下。
- check_records: OK（20 件）
- C++ ビルド: 未実行（この項目では C++ を触らない見込み）
- ステップ 3: `python Tools/dd/prepare_stage.py` が problems 0 で通り、3 件のテクスチャが `/Game/Wasami/Stage/T_*` と `Intermediate/.../wasami_*.png` を指すことを JSON で確認。エディタで `import_batch` が 3 件取り込み（1024×1024・`TC_Default`・sRGB）、材質 3 つの `Albedo`/`Emissive`/`Texture` がワサミ版を指し、未保存のパッケージが 0 件であることを確認。
- PIE での見え方の確認: 未実行（ステップ 4）
