---
title: Zone 1 の露出を原作に合わせる
status: ユーザー待ち
branch: main
base: f0496b1
started: 2026-09-16 16:10
updated: 2026-09-16 16:55
---

# Zone 1 の露出を原作に合わせる

## 依頼

`.claude/references/handover.md` の「次の一歩」1 と、前のコミット（f0496b1）の締め:
**Zone 1 の露出を決め、本家の実機で撮った `observations/README.md` の値と同じ場所・同じ向きで撮って突き合わせる。**
本作は自動露出が約 8.7 倍まで持ち上がっていて、タブレットの上の帯が 162（原作は 32）、壁も床も白っぽかった。

## 調べて分かったこと（根拠は 00 記録の「露出」に本文として書いた）

- 原作の Zone 1 に `PostProcessVolume` は 0 個。Zone 2 と入口の 1 個ずつも露出を上書きしていない。
  → **露出はプロジェクト設定だけで決まっている**（`r.DefaultFeature.AutoExposure=False` = 露出 1.0 に固定）。ポストプロセスボリュームは置かない。
- UE 4.24 の `AutoExposureBias` の既定は 0.0（原作の第三者プラグイン 2 つの、上書きしていない `FPostProcessSettings` が根拠）。UE5 の既定 1.0 は 2 倍明るいので 0.0 を明示した。
- 灯の単位（Unitless）は取り込みのままで正しい。

## 計画

- [x] 1. 原作の露出の根拠を取る
- [x] 2. `Config/DefaultEngine.ini` を原作に合わせる
- [x] 3. 同じ場所・同じ向きで撮って突き合わせる（`WasamiDevTools.capture_pose` を足した）
- [x] 4. 実装記録（00・01・03）と handover と `observations/README.md` を直し、`check_records.py --update` を通してコミット
- [ ] 5. **ユーザーの確認待ち**（下の「次にやること」）

## 結果

同じ視点（Zone 1 の廊下 (1801, −9601, 187)・ヨー 90）の 1280x720:

| | 画面全体の中央値 RGB | 平均輝度 | 上位 1 % |
| --- | --- | --- | --- |
| UE5 の既定 | (175, 170, 140) | 165.7 | 245.5 |
| 原作の設定 | (13, 13, 0) | 21.1 | 194.1 |
| 実機の廊下（別の廊下） | (44, 38, 39) | — | 天井の灯 227〜236 |

露出の持ち上げは消えた。**残る差は灯の側**（明るい面は近いのに中央値が暗い = 間接光が足りない。本作は Lumen の動的な灯、原作は 1,121 個を焼き込み）。M1 の残りとして handover に書いた。

## 次にやること

ユーザーに 3 つ確認してから進める:

1. **エディタを開き直してよいか**（`python Tools/editor_cycle.py`）。理由は 2 つ。
   - `Config/DefaultEngine.ini` に入れた cvar が起動時に読まれることの確認（いまのエディタには同じ値をコンソールで入れてあるだけ）。確認は `EditorAppToolset.SearchCVars` で `r.DefaultFeature.AutoExposure` が 0、`.Bias` が 0、`.ExtendDefaultLuminanceRange` が 0 になっていること。
   - 足した `WasamiDevTools.capture_pose` は起動時に登録されるので、開き直すまで MCP から呼べない（いまは同じ処理をスクラッチのスクリプトで動かして確かめた）。
2. **PIE を開いてよいか**。Zone 1 の開始地点でタブレットを上げて撮り、上の帯が 32（原作）に近づいたかを見る。これが露出が正しいことの決め手（UMG の決まった色なので灯の差が混ざらない）。03 記録の「確かめたこと」を更新する。
3. **モーションブラー**をどうするか。原作のプロジェクト設定は `r.DefaultFeature.MotionBlur=False`（ゲームに設定項目は無く、BP のバイトコードも触っていない）。`handover.md` の「ステージのボリュームのモーションブラー 0 は採らず本家の既定 0.5」と食い違うので、写していない。

## 決定事項

- 2026-09-16: Zone 1 にポストプロセスボリュームは置かない。原作の Zone 1 にも無く、露出はプロジェクト設定で固定されているため。
- 2026-09-16: ローカル露出（UE5 のみ）は 1.0 にして無効化する。原作の UE 4.24 に無い仕組みなので、入れると原作と違う絵になる。
- 2026-09-16: `r.UsePreExposure=False` は写さない。露出 1.0 では絵が変わらず、切り替えるとシェーダーが全部コンパイルし直しになるため。
- 2026-09-16: `r.DefaultFeature.MotionBlur=False` は写さない（上の 3 のとおりユーザーに確認する）。

## 再開時の注意

- **いまのエディタにはコンソールで同じ cvar を入れてある**（開いているレベルは `L_Hospital_Zone1`、PIE は動いていない、保存していないアセットは無い）。開き直せば ini から読まれる。開き直さずに閉じると、次の起動でも ini から読まれるので問題はない。
- `Tools/desktop.py` の操作エージェントを起動したままにしてある（30 分で自分で終わる）。止めるなら `python Tools/desktop.py stop`。
- エディタが前面でないと `HighResShot` も `take_high_res_screenshot` も**何も言わずに失敗する**（ファイルが出ない）。`WasamiDevTools.capture_pose` を使う。
- `unreal.Rotator` の位置引数は `(roll, pitch, yaw)`。ヨーのつもりで 2 番目に渡すとピッチになる。

## 検証

- check_records: OK（4 件）
- C++ ビルド: 不要（ini と Python のみ）
- エディタでの確認: 撮影で before / after を比較済み（`observations/ours/`）。ini の読み込みと `capture_pose` の登録は**開き直し待ち**、タブレットの帯は **PIE 待ち**
