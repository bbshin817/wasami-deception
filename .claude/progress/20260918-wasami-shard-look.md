---
title: ワサミシャードの見た目の変更（作業一覧の項目 22）
status: 進行中
branch: main
base: 2c3ba1c
started: 2026-09-18 03:09
updated: 2026-09-18 03:09
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# ワサミシャードの見た目の変更（作業一覧の項目 22）

## 依頼

2026-09-17 のユーザーの回答（作業一覧 `.claude/roadmap.md` の項目 22）:

> 餅自体の明滅は撤廃し、代わりにサイズを現在の1.5倍にし、本家シャードのように回転させて

2026-09-18 のユーザーの指示: 餅のモデルは `tmp/wasami_mochi_v3.glb` を使う（`.claude/references/enemy-wasami-motions.md` の「ワサミ餅」）。

変えないもの: 本家の紫の灯（強さ 175・半径 200・色 (194, 0, 255)）、回収の閃光 `P_WasamiShardFlash`（紫 × 各色の最大 ^ 0.5 × 0.8。2026-09-17 に確定）、拾う当たりのカプセル（原作の値）、地図の印。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 完了。ステップ 2〜5 に分け、順序と本家の観察の扱いを「決定事項」に書いた
- [ ] 2. 明滅をやめて餅を 1.5 倍にする ← 次
  - `M_DD_WasamiMochi` の自己発光を WebGL 版の `Glow` 0.3 だけに戻す（`PulseColor`・`PulseStrength`・`PulsePeriod`・`PulsePhase` のノードと `MI_WasamiMochi` の値を消す）。`AWasamiShard` の `PulsePhaseData` と `BeginPlay` の位相の乱数も消す。
  - `Mochi` の拡縮 0.055 → 0.0825（`ShardBodyScale` 10 の下で 0.55 m → 0.825 m）。カプセル・灯・印は変えない。
  - 変更予定: `Source/wasami_deception/WasamiShard.cpp`・`WasamiShard.h`、`Tests/WasamiShardTests.cpp`（0.55 m と位相の期待）、`Content/Python/wasami_tools/pipeline/dd_shards.py`（`_build_mochi`・`import_mochi`・冒頭の説明）、アセット `/Game/Pipeline/Materials/M_DD_WasamiMochi`・`/Game/Wasami/Shard/MI_WasamiMochi`
  - 確かめ方: `python Tools/editor_cycle.py` でビルド → `WasamiDDTools.import_dd_shards`（材質だけ作り直す）→ Wasami のテスト全体 → PIE の Zone 1 の廊下（実装記録 06 の「確かめたこと」と同じ −Y の廊下）で 4.4 m 先の餅を 6 秒撮り、明るさが一定であること（明滅が無いこと）と、画面の中の餅が約 1.5 倍に見えることを測る
- [ ] 3. 餅のモデルを `wasami_mochi_v3` に替える
  - `tmp/wasami_mochi_v3.glb` を `SourceArt/Wasami/wasami_mochi.glb` に置き換える（Git LFS。今の 6,000 三角形の glb は履歴に残る）。`dd_shards.py` のテクスチャの取り出しを v3 の中身（法線 2048²・色 2048²・金属と粗さ 4096² の PNG。今は 1024² の JPEG）に合わせ、`SM_WasamiMochi` を取り込み直す（Nanite）。
  - 変更予定: `SourceArt/Wasami/wasami_mochi.glb`、`Content/Python/wasami_tools/pipeline/dd_shards.py`、アセット `/Game/Wasami/Shard/SM_WasamiMochi`・`T_WasamiMochi_BaseColor`・`_MetallicRoughness`・`_Normal`、実装記録 06 の「作るアセット」
  - 確かめ方: 取り込みの後に Zone 1（餅 337 個）で PIE を撮り、見た目（顔の向き・大きさ）と `stat unit`・`stat RHI`（テクスチャプール）・`stat fps` を今の餅と比べる。三角形が 6,000 → 101,368 になるので、フレーム時間かプールが悪くなるなら Nanite の設定かテクスチャの上限（`.claude/guides/performance.md`。エディタにだけ効く場所）で抑え、決めた値を実装記録に書く
- [ ] 4. 回り方を本家の結晶に合わせる
  - 本家の結晶の既存の収録（`observations/original/orig-shard-spin-long.mkv`。4 m 先・45 秒・10 fps、周期 21.0 s = 17.1 °/s）と、ステップ 3 の後の PIE の餅を同じ撮り方（4 m 先・同じ画角・同じ切り出し）で並べ、**回る軸・餅の向き・速さの見え方**の違いを測る。今は顔が真上を向いた餅のヨーなので回って見えにくい。
  - 合わせ方（軸を変える / 餅を横倒しにする / 速さを変える）を測った結果から決め、理由を「決定事項」に書く。実装は `AWasamiShard` の回転（`SpinSpeed` と `Mochi` の相対回転）か、取り込みでのメッシュの向き。
  - 変更予定: `Source/wasami_deception/WasamiShard.cpp`・`.h`、`Tests/WasamiShardTests.cpp`、（向きを変えるなら）`dd_shards.py`
  - 確かめ方: PIE の収録で、本家と同じ見え方（1 周の時間と、回っていることが分かる形）になっていることを測る
- [ ] 5. 仕上げ
  - 実装記録 06 を現行の実装に合わせる（明滅の節を消し、大きさ・モデル・回り方の決定を書く）、`.claude/references/enemy-wasami-motions.md` の「ワサミ餅」の未決（Nanite・テクスチャ・顔の向き）を埋める、`python .claude/scripts/check_records.py --update`
  - `.claude/roadmap.md` の項目 22 を「完了（日付）」にし、残った要確認を「未回答の要確認」へ移す。`.claude/references/handover.md` の「現状と次の一歩」を直す
  - note の原稿 `docs/note/progress.md` を直す（`.claude/guides/note-progress.md`。セッションの値が無ければ「note へは未反映」と報告する）
  - この記録を削除して最後のコミットに含める

## 次にやること

ステップ 2。`Content/Python/wasami_tools/pipeline/dd_shards.py` の `_build_mochi`（`MOCHI_PULSE_*` のノード）と `import_mochi` の材質インスタンスの値から明滅を外し、`AWasamiShard`（`WasamiShard.h` の `PulsePhaseData`、`WasamiShard.cpp` の `BeginPlay` の `SetCustomPrimitiveDataFloat`）と `MochiSize` 0.55 → 0.825 を直し、`Tests/WasamiShardTests.cpp` の期待（「the mochi is 0.55 m」と位相の 2 行）を直す。その後 `python Tools/editor_cycle.py` → 取り込み → テスト → PIE。

## 決定事項

- 2026-09-18: 順序は「明滅と大きさ（2）→ モデル（3）→ 回り方（4）」にした — 回って見えるかは v3 の餅の顔の向きと大きさで変わるので、回り方はモデルを替えた後に決める。明滅の撤去と 1.5 倍はモデルに関係なく決まっているので先にやる。
- 2026-09-18: 本家の結晶の観察は既にある収録（`observations/original/orig-shard-spin-long.mkv`）を使い、本家の実機は起動し直さない — 4 m 先・45 秒の収録があり、周期 21.0 s も測ってある（`observations/README.md` の「シャードの回転」）。`.claude/guides/observation.md` の「本家は 1 回・30 分を目安」に沿う。ステップ 4 で軸や向きが読み取れなかったときだけ撮り直す。
- 2026-09-18: v3 の 4096² のテクスチャは、敵ワサミ（実装記録 07、`dd_enemy.py` の「the metallic-roughness map is 4096² and stays so」）と同じくそのまま取り込む方針で始める — 餅 679 個は同じテクスチャを共有するので枚数は増えず、ストリーミングは描く mip だけ読む。ステップ 3 の PIE の測りで悪くなっていたら、そこで上限を決める。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは動いている想定（ステップ 2 で C++ を変えるので `python Tools/editor_cycle.py` で閉じて建て直す。閉じる前に保存し、PIE は止める）。
- 長い処理: C++ のビルドとエディタの開き直し（`python Tools/editor_cycle.py`。ログは `Intermediate/EditorCycle/`）、餅の取り込み（MCP かリモート実行で `WasamiDDTools.import_dd_shards`。出力は `/Game/Wasami/Shard` と `/Game/Pipeline/Materials`、途中の絵は `Intermediate/Pipeline/wasami/shard/`）。
- ステップ 3 で `SourceArt/Wasami/wasami_mochi.glb` を置き換えるときは、`tmp/wasami_mochi_v3.glb`（18 MB、git の外）からコピーする。Git LFS で追われている（`.gitattributes` の `*.glb`）。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
