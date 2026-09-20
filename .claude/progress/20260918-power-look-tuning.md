---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-20
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# パワーの見た目の詰め（作業一覧の項目 23）

## 依頼

2026-09-17 のユーザーの回答（`.claude/roadmap.md` の項目 23）:

- **テレキネシスの球の粒子の灯を、本家の見え方に合わせて弱める**（ステップ 2 で済んだ）。
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**（ステップ 5・6 で、どちらも原作のコンパイル済みシェーダーの式どおりになった）。

## 計画

- [x] 1〜4. 計画、球の灯を弱める、観察の台本、本家の無劣化の連写 5 件 … 2026-09-18（`observations/README.md` の「力場と Telepathy の印の無劣化の連写」）。
- [x] 5. テレキネシスの力場の材質 4 つを原作の式どおりにした … 2026-09-18（5d3 星屑・5e 幕・5f オーラ・5g 地面の輪。経緯は `observations/README.md` の 5a〜5g、式は実装記録 04 の表）。
- [x] 6. Telepathy の印を原作の式どおりにした … 2026-09-20。`M_DD_Telepathy` を cook の Slate のピクセルシェーダーから読んだ式に置き換え（当てはめの `TELEPATHY_GAIN`・`TELEPATHY_UV_TILING` は消えた）、描画先と本家の連写と PIE で確かめた（`observations/README.md` の「Telepathy の印を原作の式にした」、実装記録 04・01）。
- [ ] 7. 仕上げ（下の「次にやること」）

## 次にやること

ステップ **7**（仕上げ。これで項目 23 は終わり）。

- `.claude/roadmap.md` の項目 23 を完了にする。**(2) の本文に残る推定の記述**（`AURA_LAYERS`・`SHOCKWAVE_PANS`・「推定のグラフ」・Telepathy の当てはめ）を、5d3〜5g と 6 で式どおりになった今の姿に直す。
- `.claude/references/handover.md` の「現状と次の一歩」を直す。
- note の原稿 `docs/note/progress.md` を「いま何が出来るか」に合わせて直し、`tmp/note-cli` で同じ記事を書き換える（`.claude/guides/note-progress.md`。`Tools/note.local.json` が無ければ原稿だけ直して「note へは未反映」と報告する）。
- 実装記録 04 の「既知の制約・注意点」と表、01 の変更履歴は 5d3〜5g・6 で直してある。読み返して食い違いがないかだけ見る。
- 終わったら**この進捗記録を消して**最後のコミットに含め、`.claude/roadmap.md` の項目 23 を完了にする（大目標 3 の残りの項目はまだあるので、大目標の状態は触らない）。

## 決定事項

- 2026-09-18（5d3）: **材質は先に cook のシェーダーを読んで式を写す**（`Tools/dd/cooked_shaders.py`。決め方の階段の「本家のコード」）。収録との見比べは写した後の確かめにだけ使う。UI の材質（Telepathy）も自分のシェーダーマップを持つ（ステップ 6）。
- 2026-09-18（ユーザーの回答）: **幕が画面を横切る速さは検討事項から外す**（球の幾何・回転の乱数・カメラの揺れで決まり、どれも本家どおり）。
- 2026-09-20（6）: **どちらのノイズのサンプルかは、シェーダーマップのテクスチャの式の参照番号と cook の `Expressions` の並びで決める**（`Tools/dd/cooked_shaders.py` は 2D テクスチャの式を印字しないので、`tmp/tk6/tex_uniforms.py` で読んだ）。
- 2026-09-20（6）: **エンジンの材質関数の既定値は版で変わるので、cook の式に出る入力は呼び出しに入れる**（`RadialGradientExponential` の密度は UE 4.24 が 1、UE 5.8 が 2.33）。

## 要確認（ユーザー）

- （なし）

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`、道具は `observations/tools/`（どれも git の外）。使い方は `observations/README.md` の tools の表。走り書きは `tmp/tk5*/`・`tmp/tk6/`（git の外）。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**ので、作り直す前に `python Tools/pie.py stop`。
- **`desktop.py` の入力は、前面の窓が許した処理のときだけ届く**。端末が前面のときは、最初の 1 回だけ `click <エディタの中> --allow UnrealEditor.exe --allow WindowsTerminal.exe` でエディタを前に出してから、以降は `--allow UnrealEditor.exe` で送る。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。
- 実装記録 01 は **CRLF**（ほかの記録・ソースは LF）。書き戻すときは改行を保つ。

## 検証

- ステップ 6: `make_telepathy_materials()` で 3 つを作り直し（接続の失敗なし、コンパイルのエラーなし）、`Speed` 0 の描画先と numpy の予測が平均 0.0009（255 段階で 0.24）で一致。本家の連写 `orig-telepathy-d025` の 4 枚との大きさに依らない統計も重なる。PIE（`L_Hospital_Zone1`、仮の的 3 体）で印を 1 枚撮って確かめ、PIE は止めた。`check_records.py --update` は OK。C++ は変えていない。本家は起動していない。エディタは `L_Hospital_Zone1`・PIE なし・未保存なし。
