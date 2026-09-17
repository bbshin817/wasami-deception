---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 05:30
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# パワーの見た目の詰め（作業一覧の項目 23）

## 依頼

2026-09-17 のユーザーの回答（`.claude/roadmap.md` の項目 23）:

- **テレキネシスの球の粒子の灯を、本家の見え方に合わせて弱める**（ステップ 2 で済んだ）。
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**。力場 = オーラの 4 層と地面の輪の流れ、球の幕の筋（本作は少ない）、終わりの破片。Telepathy = 雲の流れる速さと向き、縁のこぶ（本作は丸に近い）。
- 撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。

詰める先: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_aura7`（`AURA_LAYERS`）・`_build_shockwave02`（`SHOCKWAVE_PANS`）・`_build_wall02`、Telepathy の `TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`（どれも `TODO(仮)`）。根拠は実装記録 04 の「既知の制約・注意点」、`observations/README.md`（ステップ 11b1・11b4・11b5・11b6 と、下のステップ 4 の節）、`.claude/references/powers/03-telekinesis-vanish.md` の 2.6・`04-primal-telepathy.md`。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 作成。
- [x] 2. テレキネシスの球の灯を弱めた … 2026-09-18。本作だけの写し `/Game/Wasami/Powers/P_WasamiForceField`（灯 5.0 × 0.35 = 1.75）。記録は実装記録 01・04 と `observations/README.md` の 11b6。
- [x] 3. 本家で撮るものの一覧（観察の台本）を作った … 2026-09-18。
- [x] 4. 本家の実機（最新版）で撮った … 2026-09-18、04:41〜05:15（約 34 分）。無劣化の連写 5 件（A1・A2・A3・B1・B2。A4 は不要で飛ばした）。条件・測った値・気づいたことは `observations/README.md` の「力場と Telepathy の印の無劣化の連写」。
- [ ] 5. テレキネシスの力場の材質を収録に合わせて詰める（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`_build_wall02`、灯の色味の残り）
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ 5（テレキネシスの力場の材質）。本家の連写 `observations/original/orig-tk-a025`（0.25 倍・全画面 150 枚）・`orig-tk-b010`（0.1 倍・中央 1200²・320 枚）・`orig-tk-c010-down`（0.1 倍・見下ろし 150 枚）と、PIE の同じ条件の収録を見比べて、`dd_powers.py` の `AURA_LAYERS`（オーラの層のパンと繰り返し）・`SHOCKWAVE_PANS`（地面の輪）・`_build_wall02`（幕）を決める。

手順の見込み:

1. 連写を測る道具を用意する（`tools/tele_fit/dmap.py` などはフォルダーを取れるが `--fps` の一定間隔を前提にする。**`times.json` を読む形に直す**。並べて見るのは `tools/burst_sheet.py`）。
2. PIE を本家と同じ条件で撮る: `observations/tools/telekinesis_setup.py`・`telekinesis_begin.sh`（画質「高」の 3 つの cvar・Lv4・開始地点・奥をふさぐ）→ `slomo 0.25` と `0.1` で `burst.py`（PIE はビューポートなので範囲を絞る）。
3. 1 枚ごとの位相相関で流れ（向きと速さ）を出し、`AURA_LAYERS` のパンを合わせる。幕は「幅の広い柔らかい筋」に寄せる（下の決定事項）。
4. 終わりの破片は、板状のかけらとして粒子の見た目を見直す（形・数・尾）。

## 決定事項

- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6〜+2.2 秒の画面全体の平均**（ステップ 2 で灯を決めたときと同じ窓）。ステップ 5 でもこの窓で揃える。用意は `observations/tools/telekinesis_begin.sh`。
- 2026-09-18: 本家の収録は**無劣化の連写**（19〜20 枚/s）。h264 の 4:2:0 は細い筋と、赤だけを足す印の細かさを崩すので、**11b5 で測った印の「細かさ 0.02〜0.03」は低めに出ている恐れがある**（ステップ 6 で `orig-telepathy-d025` から測り直す）。
- 2026-09-18: **`orig-telepathy-c1`（速さ 1）には発動前のフレームが無い**ので、背景には `orig-telepathy-d025/f000` を使う（同じ 1 回の生で、視点を動かしていない。動いたナースの画素は G・B の変化で外れる）。
- 2026-09-18: **見下ろした収録（`orig-tk-c010-down`）から地面の輪の筋は読めない**（画面が水色で飽和する）。ステップ 5 で輪を測るときは、飽和が引く収録の 4 秒以降か `orig-tk-b010` を使う。
- 2026-09-18: 本家の**幕は「幅の広い柔らかい光の筋」**で細い線の束ではなく、**終わりの破片は「角ばった板状のかけら」**（丸い粒ではない）。ステップ 5 はこの 2 点を目標にする。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 連写は `observations/original/`（git の外）。`times.json` は `perf_counter` 秒で**等間隔ではない**ので、一定間隔を前提にする道具（`tele_fit/*`）に渡す前に時刻を差し替える。
- 本家のセーブは、観察の途中で捕まって書き換わっている（控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。**戻さない・編集しない**）。W-Editor は開いていない。
- 本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順・座標・つまずきは `.claude/guides/observation.md`（2026-09-18 に連写の速さ・死亡画面のボタン・印が画面空間であることを足した）。
- **Automation テストはエディタが前面でないと進まない**（背面は 3 fps で `FWaitForInteractiveFrameRate` が落ちる）。`Tools/ue_remote.py` で `execute_console_command(None, 'Automation RunTests Wasami')` を送ってから、`python Tools/desktop.py click 2680 83 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（タイトルバーの空き）で前面にし、`Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty` と `Result={Success}` を数える。エディタを開き直した後は MCP の `call_tool` が `Tool ... not found` になる（無人モードでは `/mcp` を頼めない）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 4 はソース・アセットを変えていない（`check_records.py` は OK、7 件）。本家は閉じ（`tasklist` で 0 件）、エディタは `editor_cycle.py --no-build --no-quit` で開き直した。連写 5 件はすべて `times.json` まで書き終えている。
