---
title: 作業一覧の項目 34（ポータルと特殊シャードの見え方）
status: 進行中
branch: main
base: 3226bb8
started: 2026-09-21 09:21
updated: 2026-09-21 09:21
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（上限 30 KB） -->

# 作業一覧の項目 34（ポータルと特殊シャードの見え方）

## 依頼

`.claude/roadmap.md` の大目標 3 の項目 34。大目標 2 の項目 13（脱出のポータル）・10（特殊シャード）で **cook で式が消えた材質を推定で組んだ**ところを、原作の粒子の型データと残っているコンパイル済みシェーダーから本家どおりにし、作っていない粒子を足す。

完了の条件（作業一覧より）:
- ポータル: まだ無い `PPP_PortalAppear`・`PPP_PortalAppear_Lock` を粒子の型データ（`ResourceData`）と残っているシェーダーから**作るか、作らない理由を書いて閉じる**（親 `PPP_Particles_lit`・`_fogged` の式は cook で消えている）。渦 `M_00_Portal_Vortex`・ロゴの親 `M_00_Portal_Monkey` は推定なので原作のデータで詰める。
- 特殊シャード: 結晶の屈折・閃光の不透明度・地図の印の色を材質とコンパイル済みシェーダーの式で決める。
- **コードで決まらなかったものだけ**を 1 回の収録にまとめる（節の頭の「この節の項目は原作のコード・アセットで先に決める」。本家の実機は移動が遅すぎて 1 回の収録で 1 か所しか回れない）。

## 計画

- [ ] 1. 渦の材質 `M_00_Portal_Vortex`（と 8 つのインスタンス）を書き出しの式の型とコンパイル済みシェーダーどおりにする ← 次
  - 変更予定: `Content/Python/wasami_tools/pipeline/`（ポータルの材質を作っている所）、`/Game/Pipeline/Materials/...`、`/Game/DD/Materials/00_Ballroom/M_00_Portal_Vortex*`
- [ ] 2. ロゴの親 `M_00_Portal_Monkey` と鍵 `M_00_Portal_Lock` を同じく原作のデータどおりにする
- [ ] 3. `PPP_PortalAppear`・`_Lock` の型データと親の材質（`PPP_Particles_lit`・`_fogged`）を読み、**作るか作らないかを決める**（作るなら材質をこのステップで組む）
- [ ] 4. ステップ 3 で作ると決めたら、エミッタを `ResourceData` から組んで取り込み、`AWasamiPortal` の部品につなぐ（作らないなら理由を実装記録 08 に書いて飛ばす）
- [ ] 5. 閃光の材質 `M_ky_primitiveColor`・`M_ky_lensFlare02` の不透明度をコンパイル済みシェーダーの式で確定して直す
- [ ] 6. 結晶 `m_crystal` と地図の印 3 つ（`M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`）をシェーダーと突き合わせ、差があれば直す
- [ ] 7. PIE でポータル（鍵 → 開く）と特殊シャード（出現・閃光・取得・地図の印）を見て確かめ、実装記録 08・16 と作業一覧・handover を直して項目 34 を閉じる

## 次にやること

ステップ 1。`python Tools/dd/cooked_shaders.py "MasterMaterials/M_00_Portal_Vortex."` でベースパスの式を読み、書き出し（`pak_reference_2/_assets/DDeception/Content/Materials/MasterMaterials/M_00_Portal_Vortex.json`）の式の型と突き合わせて、今の推定（実装記録 08 の「ガレージのポータル」、作っているのは `wasami_tools` の取り込み）との差を洗い出してから直す。

## 決定事項

- 2026-09-21: 項目 33 と同じ進め方にする — **本家の実機の収録に頼らず、原作のブループリント・アセットの値・cook の `ResourceData`・コンパイル済みシェーダーの式で決める**。理由: 作業一覧の大目標 3 の節の頭（2026-09-21 のユーザーの回答）。項目 33 は 4 行とも収録なしで閉じられた。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 原作の材質と粒子の在り処: 渦・ロゴ・鍵 = `pak_reference_2/_assets/DDeception/Content/Materials/MasterMaterials/M_00_Portal_Vortex.json`・`Materials/00_Ballroom/M_00_Portal_{Monkey,Lock}.json` とインスタンス 8 つ（同じ `00_Ballroom`）、粒子 = `ThirdParty/PyroParticlePack/Particles/PPP_PortalAppear.json`・`PPP_PortalAppear_Lock.json`。
- 使う道具: `python Tools/dd/cooked_shaders.py "<パスの一部>."`（cook で消えた式を焼き込みから読む）、`python Tools/dd/gpu_emitters.py`（項目 33 で作った、GPU のエミッタの型データと焼き込みの表の突き合わせ）。
- **結晶の `distortion_normal` は項目 28 のステップ 13（2026-09-21）で入れ済み**（実装記録 16 の「結晶の材質」の「推定で外したもの: 無し」）。作業一覧の項目 34 の「今は」の行はそれより前の記述なので、ステップ 6 では現物を見て判断する。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
