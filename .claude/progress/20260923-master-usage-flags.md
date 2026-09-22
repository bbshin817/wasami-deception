---
title: 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す
status: 進行中
branch: main
base: 8b1cc1d
started: 2026-09-23 03:33
updated: 2026-09-23 03:33
---

# 項目 47 ステージのマスターの用途フラグ（Nanite・StaticLighting）を立て直す

## 依頼

`.claude/roadmap.md` の項目 47（大目標 4）。2026-09-23 のユーザーの回答「はい（項目を立てて直す）」（項目 46 の要確認）。

前処理が作るマスターが `bUsedWithNanite`・`bUsedWithStaticLighting` を偽のまま持っている。**エディタは足りない用途フラグをその場で立て直すので PIE では出ないが、パッケージ版では既定の材質（灰色の市松）に落ちる**。

完了の条件:

1. どのマスターにどのフラグが要るかを洗い出す（ステージの 7 つ `M_DD_Substance`・`M_DD_Decal`・`M_DD_Unlit`・`M_DD_Metal`・`M_DD_SubstanceFresnel`・`M_DD_Glass`・`M_DD_GlassSewerage` と、**ステージの外で実際に落ちている `M_DD_Crystal` を含む前処理のマスター全部**。使うメッシュが Nanite か、焼き込みの対象かで決める。本家の材質の設定にフラグがあればそれを写す）。
2. 前処理がマスターを作る・作り直すところ（`dd_stage.ensure_masters` と各モジュールの同じ口）でフラグを立てて保存し、`Content/Wasami`・`Content/DD` のマスターを作り直す。
3. PIE で Zone 1・Zone 2 の見た目が変わらないことを確かめる。
4. パッケージ版を作り直し、ログに `missing usage flag` が 1 件も出ないことと、祭壇の球が本家の見た目で出ることを確かめる。

## 計画

- [ ] 1. 洗い出し（完了の条件 (1)）← 次にやること
  - 前処理が作るマスターを全部列挙し、各マスターのインスタンスがレベルでどう使われているか（Nanite のメッシュか・焼き込みの対象か・スケルタルか・粒子か・デカールか）をエディタで調べて、**マスターごとの要るフラグの表**をこの記録に書く。
  - 変更予定: この記録だけ（調べるための使い捨ての Python は `Intermediate/` へ）
- [ ] 2. 前処理にフラグを入れる（完了の条件 (2)）
  - 変更予定: `Content/Python/wasami_tools/pipeline/dd_stage.py`（`ensure_masters`・`MASTER_VERSION`）、`dd_assets.py`（`material()`）、フラグが要るほかのモジュール、`/Game/Pipeline/Materials/*`・`/Game/Wasami/**`・`/Game/DD/**` のマスター
- [ ] 3. マスターを作り直して PIE で確かめる（完了の条件 (3)）
  - 変更予定: マスターの uasset（作り直し）
- [ ] 4. パッケージ版を作り直してログと祭壇の球を確かめる（完了の条件 (4)）
  - 変更予定: `Saved/Archive/Windows/`（git の外）

## 次にやること

ステップ 1。まずエディタが起きているか確かめ（`python Tools/ue_remote.py -c ...`）、使い捨ての Python で
(a) 前処理が作るマスター（`unreal.Material` で、パイプラインが作った印のあるもの）を全部列挙し、
(b) `L_Hospital_Zone1`・`L_Hospital_Zone2` とほかのレベルのコンポーネントを歩いて「そのマスターの子のインスタンスが付いているメッシュが Nanite か・`Mobility Static` で焼き込みの対象か・スケルタルか・粒子か」を数え、
(c) マスターごとの要るフラグの表をこの記録に書く。

## 決定事項

- 2026-09-23: **本家の `pak_reference_2/_materials.json` に用途フラグは入っていない**（`bUsedWith*` が 1 件も無い。cook で落ちるか、書き出しが拾っていない）。なので「本家の設定を写す」はできず、**本作の実際の使われ方から決める**（roadmap の条件 (1) の但し書きどおり）。
- 2026-09-23: 用途フラグは 1 つにつきシェーダーの組み合わせが増えるので、**全部のマスターに全部立てるのではなく、実際に使うものだけ立てる**（この PC は VRAM 6 GB。`.claude/guides/performance.md`）。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- パッケージ版のログで実際に落ちているのは **`/Game/DD/Materials/Fords_Materials/m_crystal_Inst2`（`M_DD_Crystal` の子）の `Nanite` と `StaticLighting` の 2 件だけ**（`Saved/Archive/Windows/wasami_deception/Saved/Logs/wasami_deception.log`）。ただしこのログはそのとき遊んだ場所しか通っていないので、**ログだけを当てにせず (1) の洗い出しで全部見る**。
- 前処理がマスターを作る口は 2 つ: `dd_stage._material()` / `ensure_masters()`（ステージの 7 つ。`MASTER_VERSION` を上げると作り直す）と `dd_assets.material()`（ほかのモジュールが全部通る）。フラグはこの 2 か所に入れれば行き渡る。
- 既にフラグを立てているところがある（`dd_stage` の `used_with_skeletal_mesh`、`dd_gimmicks`・`dd_powers` の `used_with_static_lighting`・`used_with_particle_sprites` など）。重複して上書きしないようにする。
- 長時間処理: パッケージの作り直し（ステップ 4）。手順と確かめ方は `.claude/guides/distribution.md`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行（この項目は Python の前処理だけ）
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
