---
title: ワサミシャードの見た目の変更（作業一覧の項目 22）
status: 進行中
branch: main
base: 2c3ba1c
started: 2026-09-18 03:09
updated: 2026-09-18 03:45
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
- [x] 2. 明滅をやめて餅を 1.5 倍にした… 2026-09-18 完了。自己発光は `Glow` だけに戻し、`PulsePhaseData` と `BeginPlay` の乱数を削除、`MochiSize` 0.55 → 0.825。テスト 29 本成功、PIE で明滅の消失と横幅 1.52 倍を測った（`observations/README.md`）
- [x] 3. 餅のモデルを `wasami_mochi_v3` に替えた… 2026-09-18 完了。glb を差し替えて `import_mochi()`（101,368 三角形・Nanite・PNG 2048²/2048²/4096²）。Zone 1 で前後を測り、フレーム時間は変わらず VRAM +0.20 GB だったのでテクスチャの上限は設けない（`observations/README.md`、実装記録 06 の「餅のモデル」）
- [ ] 4. 回り方を本家の結晶に合わせる ← 次
  - 本家の結晶の既存の収録（`observations/original/orig-shard-spin-long.mkv`。4 m 先・45 秒・10 fps、周期 21.0 s = 17.1 °/s）と、v3 の餅を同じ撮り方（4 m 先・同じ画角・同じ切り出し）で並べ、**回る軸・餅の向き・速さの見え方**の違いを測る。
  - ステップ 3 で分かったこと: v3 は**顔が横を向き暗い髪が上に乗る**ので、今のヨーの回転でも顔が正面に来ては裏へ回るのが分かる。軸を変える必要はおそらく無く、**速さ（今は個体ごとの乱数 10.8〜32.4 °/s、本家は 17.1 °/s）と、1 周がどう見えるか**が論点。
  - 合わせ方を測った結果から決め、理由を「決定事項」に書く。実装は `AWasamiShard` の回転（`SpinSpeed` と `Mochi` の相対回転）。
  - 変更予定: `Source/wasami_deception/WasamiShard.cpp`・`.h`、`Tests/WasamiShardTests.cpp`
  - 確かめ方: PIE の収録で、本家と同じ見え方（1 周の時間と、回っていることが分かる形）になっていることを測る
- [ ] 5. 仕上げ
  - 実装記録 06 に回り方の決定を書く（明滅・大きさはステップ 2、モデルとテクスチャの決定はステップ 3 で済み。`enemy-wasami-motions.md` の未決もステップ 3 で埋めた）、`python .claude/scripts/check_records.py --update`
  - `.claude/roadmap.md` の項目 22 を「完了（日付）」にし、残った要確認を「未回答の要確認」へ移す。`.claude/references/handover.md` の「現状と次の一歩」を直す
  - note の原稿 `docs/note/progress.md` を直す（`.claude/guides/note-progress.md`。セッションの値が無ければ「note へは未反映」と報告する）
  - この記録を削除して最後のコミットに含める

## 次にやること

ステップ 4。本家の既存の収録 `observations/original/orig-shard-spin-long.mkv`（4 m 先・45 秒・10 fps、周期 21.0 s = 17.1 °/s）と、v3 の餅を PIE で同じように撮ったものを `Tools/video_probe.py period` で並べ、1 周の時間と「回っていると分かる形」を比べる。v3 は顔が横向きなのでヨーのままでよさそうで、残る論点は速さ（乱数 10.8〜32.4 °/s のままにするか、本家の 17.1 °/s に寄せるか）。

## 決定事項

- 2026-09-18: 本家の結晶の観察は既にある収録（`observations/original/orig-shard-spin-long.mkv`）を使い、本家の実機は起動し直さない — 4 m 先・45 秒の収録があり、周期 21.0 s も測ってある（`observations/README.md` の「シャードの回転」）。ステップ 4 で軸や向きが読み取れなかったときだけ撮り直す。
- 2026-09-18: v3 の 4096² のテクスチャは**そのまま取り込み、開発中の上限も設けない**（ステップ 3 で確定） — Zone 1 で前後を測って、フレーム時間 12.11 → 11.98 ms・GPU 9.1 ms で変わらず、VRAM だけ +0.20 GB（2.98 → 3.18 GB / 5.08 GB）だった。餅 679 個は同じメッシュとテクスチャ 3 枚を共有するので個数では増えない。
- 2026-09-18: 餅の大きさの確かめ方は「暖色の連結成分の横幅」だが、**種は 1 点でなく窓 (520, 395)〜(665, 500) の中でいちばん大きい成分を採る** — v3 は暗い髪が上に乗るので、ステップ 2 の種 (583, 447) が髪に当たる角度で何も拾えなかった。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは動いている想定。PIE を始めると `L_Hospital_Zone1` が dirty になるが、保存しない。ステップ 4 は C++ を変えるので、実装の後に `python Tools/editor_cycle.py`（尋ねずに走らせてよい）。
- **測る前にエディタを前面にする**（背面は 3 fps）。`python Tools/desktop.py click 2400 700 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（端末は画面の左 [226, 234, 1355, 869] にあるので、この座標はビューポートの上）。
- `stat unit`・`stat RHI` はエディタに残り、PIE を始め直しても点いたまま。`pie.py cmd "stat unit"` はトグルなので消えてしまう（撮った画面で確かめる）。
- PIE の収録は `Tools/desktop.py record --grab gdi --region 1826 204 2978 858`。ビューポートの左 400 px ほどは浮いた Automation のログの窓に隠れるが、餅の箱（555, 422）〜（612, 472）はその外。
- テストは MCP の `AutomationTestToolset.AutomationTestToolset` の `DiscoverTests`（`bForceRediscover`）→ `RunTestsByFilter`（`StartsWith:Wasami`）。`call_tool` は `toolset_name` と `tool_name` を別に渡す（つなげた名前は見つからない）。

## 検証

- check_records: OK（2026-09-18。`--update` で 01・06 のハッシュを更新）
- C++ ビルド: 成功（2026-09-18。`Tools/editor_cycle.py`、警告なし）。ステップ 3 は C++ を変えていない
- テスト: `Wasami` 29 本成功（2026-09-18、ステップ 3 でも。メッシュの入れ替えで落ちる期待値は無かった）
- PIE: v3 の餅が正しく描かれ（金属 0・つや有り、顔が横向き）、フレーム時間は変わらず VRAM +0.20 GB（2026-09-18、ステップ 3。`observations/README.md`）
