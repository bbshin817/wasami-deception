---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 05:10
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# パワーの見た目の詰め（作業一覧の項目 23）

## 依頼

2026-09-17 のユーザーの回答（`.claude/roadmap.md` の項目 23）:

- **テレキネシスの球の粒子の灯を、本家の見え方に合わせて弱める**（ステップ 2 で済んだ）。
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**。力場 = オーラの 4 層と地面の輪の流れ、球の幕の筋（本作は少ない）、終わりの破片。Telepathy = 雲の流れる速さと向き、縁のこぶ（本作は丸に近い。前回の収録では雲の変わる速さは今の仮の値の 1〜1.5 倍かもしれない）。
- 撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。

詰める先: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_aura7`（`AURA_LAYERS`）・`_build_shockwave02`（`SHOCKWAVE_PANS`）・`_build_wall02`、Telepathy の `TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`（どれも `TODO(仮)`）。根拠は実装記録 04 の「既知の制約・注意点」、`observations/README.md`（ステップ 11b1・11b4・11b5・11b6）、`.claude/references/powers/03-telekinesis-vanish.md` の 2.6・`04-primal-telepathy.md`。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 作成。
- [x] 2. テレキネシスの球の灯を弱めた … 2026-09-18。本作だけの写し `/Game/Wasami/Powers/P_WasamiForceField`（灯 5.0 × 0.35 = 1.75）。記録は実装記録 01・04 と `observations/README.md` の 11b6。
- [x] 3. 本家で撮るものの一覧（観察の台本）を作った … 2026-09-18。下の「ステップ 4 の台本」。
- [ ] 4. 本家の実機（最新版）で撮る … 台本のとおり。1 回・30 分を目安（撮影は 20 分の見込み）。エディタを閉じてから起動し、終わったら閉じてエディタを開き直す。
- [ ] 5. テレキネシスの力場の材質を収録に合わせて詰める（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`_build_wall02`、灯の色味の残り）
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## ステップ 4 の台本（本家の最新版で撮るもの）

読む順は `.claude/guides/observation.md`（起動・MOD の座標・片付け）→ この表。**撮り終えたらすぐ閉じる**。

**既存の収録で足りるので撮り直さないもの**: 画面全体の明るさと色（11b6、`orig-telekinesis-a`）、星屑の形（11b4）、印の濃さ・大きさ・面積の統計（11b5、`orig-telepathy-b`）。

**撮り方の決め事**

- すべて**無劣化の連写**（`python observations/tools/burst.py <出力> --count N [--region L T R B]`、全画面で約 17 枚/s）。`record` の h264 は 4:2:0 なので、オーラの細い筋と、赤だけを足す印の細かさが崩れる。Claude はセッション 1 にいる（`python Tools/desktop.py ping` の `session` = `console_session` = 1）ので、バックグラウンドで直接走らせる。
- 連写はフレームを全部メモリに貯めてから PNG を書く（全画面 1 枚 約 15 MB）。**全画面は 130 枚まで、範囲を絞るときは 320 枚まで**。書き出しの 1 分前後の間にクールダウンが明ける（テレキネシスは Lv4 で 8.5 秒 = slomo 0.25 で 34 秒・0.1 で 85 秒）。
- 時刻は `times.json`（`perf_counter`。等間隔ではない）。`video_probe.py` は mkv 専用なので、連写はフォルダーを取る道具（`tools/tele_fit/dmap.py` など）で測り、`--fps` の一定間隔の前提を `times.json` に差し替える（ステップ 5 でやる小さな直し）。
- セーブの段階は**テレキネシス Lv4・Telepathy Lv2**（PIE も 11b4 と同じくこの段階に合わせる。力場の見た目は段階に依らない〈拡大は常に 2〉、Telepathy の段階は時間だけ）。画質は本家のまま「高」で、PIE 側は `tools/aim_setup.py` の 3 つの cvar で合わせる。

| # | 項目 | 場所 | 速さ | 長さ | 操作 | 測り方 | 出力 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A1 | 力場の全体（終わりの破片、幕の色） | 開始地点 | 0.25 | 全画面 130 枚（約 8 s = 演出の全部） | Space・1 × 4（左枠をテレキネシスに）・Space → 連写を始める → 0.5 s 後に Q | 終わりのぎざぎざの破片の形と数、暗い所の幕の色（灯の色味の残りの手がかり）、シート | `orig-tk-a025/` |
| A2 | オーラの筋と幕の流れ | 開始地点 | **0.1** | 中央 1200 × 1200 を 320 枚（約 19 s） | 連写 → Q | 1 枚ごとの位相相関で流れ、自己相関の減り。本作の同じ撮り方と比べて `AURA_LAYERS` のパンと繰り返しを決める | `orig-tk-b010/` |
| A3 | 地面の輪 | 開始地点 | 0.1 | 全画面 130 枚（約 8 s） | `look --dy 171 --steps 3 --burst 6` で約 30° 見下ろす（1 カウント 0.175°、1 個あたり 9.5 カウント） → `shot --name orig-tk-down-pose.png`（姿勢の控え） → 連写 → Q | 輪の筋の流れと向き（`SHOCKWAVE_PANS`）。ピッチは姿勢の控えを PIE の絵と重ねて決める（11b4 の `aim_fit/cmp.py` と同じ要領） | `orig-tk-c010-down/`・`orig-tk-down-pose.png` |
| A4 | （時間が余れば）力場の全画面の動画 | 開始地点 | 0.1 | 25 s | `record --grab gdi` → Q | `series`（画面全体の推移）・`sheet` | `orig-telekinesis-slow.mkv` |
| B1 | 印（速さ 1 の素の姿） | Zone 1 の待合 | 1 | 全画面 120 枚（約 7 s） | 到着したらすぐ連写 → Q（枠は開始地点で Telepathy に送っておく）。視点は動かさない | `tele_fit/dmap.py`（dR の分布・面積・細かさ）、`flow.py`（雲の向きと速さ） | `orig-telepathy-c1/` |
| B2 | 印（遅くして長く） | Zone 1 の待合 | 0.25 | 全画面 220 枚（約 13 s） | 到着 → MOD で `slomo 0.25` → 連写 → Q | 同上。印が画面でほぼ止まるので、箱の拡縮に邪魔されずに流れを測れる | `orig-telepathy-d025/` |

**順番**: A1 →（MOD で `slomo 0.1`）→ A2 → A3 →（A4）→ 枠を Telepathy へ送る → Maps で ZONE 1 → B1（約 7 秒で捕まる）→ 死亡 →「続ける」→ Maps で ZONE 1 → B2（捕まるまで）→ 片付け。

**なぜこの撮り方か**

- **slomo 0.1**: オーラは寿命 0.8 秒で回りながら縮む（`MeshRotationRate (0,0,-1)`、大きさは 1.556 → 0.542 倍）ので、0.25 では 1 枚ごとに絵が動きすぎて流れを追えない。材質の `Time` はワールドの時間なのでパンも 1/10 になり、実時間へは掛け算だけで戻せる。A1 だけ 0.25 なのは、全画面 130 枚で演出の始めから終わり（破片）までが 1 回に収まるため。
- **印の slomo**: 印は UI の材質で、UE 5.8 では `Time` が実時間（slomo に従わない。11b3）。本家（UE 4.24）も同じなら、slomo をかけると**印の位置だけが止まり、雲の流れは普通の速さのまま**になる。B1（速さ 1）と B2（0.25）の自己相関の減り方を比べれば、本家でも実時間かどうかが分かる。
- **枠を先に送る**: Zone 1 は到着から約 7 秒で捕まるので、着いてからタブレットを操作する余裕がない。枠はレベルをまたいで残る（本家は GameInstance で持つ。参考 03 の 1.1）。到着後に印が出なければ枠が戻っているということなので、次の回で送り直す。
- **敵**: B1・B2 は捕まって終わる前提（無敵は当てにしない。`.claude/guides/observation.md`）。敵を消すと印が出ないので、Remove All は使わない。

**片付け**: `slomo 1` → 本家を閉じる（observation.md の 7）→ `python Tools/editor_cycle.py --no-build --no-quit` でエディタを開き直す → 連写は `observations/original/` へ移す（git の外）。

## 次にやること

ステップ 4（本家の実機で撮る）。上の台本のとおり。エディタを閉じてから `Launch-Latest.cmd` を起動し、A1 → A2 → A3 →（A4）→ B1 → B2 の順に撮って、撮ったものと気づいたことを `observations/README.md` に足してコミットする。撮れなかった項目は飛ばして記録に書き、ステップ 5（力場の材質）へ進む。

## 決定事項

- 2026-09-18: ステップの順は「灯→台本→本家の収録→材質→印→仕上げ」— 灯を先に直しておかないと、力場の材質を見比べるときに画面の明るさが灯の差で濁るため。
- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6〜+2.2 秒の画面全体の平均**。ステップ 5（材質）でもこの窓で揃える。用意は `observations/tools/telekinesis_begin.sh`。
- 2026-09-18: ステップ 4 の収録は**無劣化の連写**にする。h264 の 4:2:0 は細い筋と、赤だけを足す印の細かさを崩すので、11b5 で測った「細かさ 0.02〜0.03」も低めに出ている恐れがある（B1 で確かめる）。
- 2026-09-18: 力場は **slomo 0.1** で撮る（オーラの流れを 1 枚ごとに追うため）。印は UI の材質なので slomo で位置だけが止まる利点があり、B1（速さ 1）と B2（0.25）の両方を撮って本家でも UI が実時間かを確かめる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- **ステップ 4 の途中で止まったとき**: `tasklist | grep -i -E "DDeception|ffmpeg|python"` で本家・収録・連写が残っていないかを見る。本家が残っていたら observation.md の 7 の手順で閉じる（動かなければ `taskkill`）。連写のフォルダーに `times.json` があれば書き出しまで終わっている（無ければその回は捨てる）。エディタは `python Tools/editor_cycle.py --no-build --no-quit` で開き直す。セーブは読むだけで、戻さない・編集しない。
- 本家の実機を起動するときはエディタを先に閉じる（VRAM 6 GB。`.claude/guides/verification.md`）。収録は `Tools/desktop.py`、測るのは `Tools/video_probe.py` と `observations/tools/*_fit/`。
- 撮った動画・連写は git の対象外（`observations/` の下。README には測った値と条件だけを書く）。
- **Automation テストはエディタが前面でないと進まない**（背面は 3 fps で `FWaitForInteractiveFrameRate` が落ちる）。`Tools/ue_remote.py` で `execute_console_command(None, 'Automation RunTests Wasami')` を送ってから、`python Tools/desktop.py click 2680 83 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（タイトルバーの空き）で前面にし、`Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty` と `Result={Success}` を数える。エディタを開き直した後は MCP の `call_tool` が `Tool ... not found` になる（無人モードでは `/mcp` を頼めない）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 3 はドキュメントだけ（ソース・アセットの変更なし）。`check_records.py` は OK（7 件、ハッシュは動いていない）。
