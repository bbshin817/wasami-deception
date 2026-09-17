---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 06:17
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
- [ ] 5. テレキネシスの力場の材質を収録に合わせて詰める（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`_build_wall02`、灯の色味の残り）… 1 コミットに収まらないので 5a〜5d に分けた（2026-09-18）
  - [x] 5a. 連写を測る道具を作り、本家を測った … 2026-09-18。`observations/tools/burst_flow.py`（`width`・`decay`・`flow`）と台本 `forcefield_stats.sh`。値と分かったことは `observations/README.md` の「力場の模様を測った」。
  - [x] 5b. オーラの大きさを本家と比べ、**今の値のままにした** … 2026-09-18。px の倍率 2.97、自己相関の曲線で比べると試した 3 つは今の値と区別できなかった。値と理由は `observations/README.md` の「オーラの大きさを本家と比べた」。
  - [ ] 5c. 幕 `_build_wall02` を「幅の広い柔らかい光の筋」に寄せる
  - [ ] 5d. 終わりの破片を「角ばった板状のかけら」に見直す（形・数・尾）
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ 5c（幕 `_build_wall02` を「幅の広い柔らかい光の筋」に寄せる）。手順は 5b と同じ道具で:

1. `sh observations/tools/telekinesis_burst.sh observations/ours/pie-wall-<版> 200` で本作を撮る（閃光の時刻が印字される）。**終わったら `python Tools/pie.py stop`**（PIE のままアセットを作り直すと `load_asset` が None になる）。
2. V（τ 0.30〜0.45）と F（0.50〜0.70）の窓を、**中央 1200²**（本家 (1120, 120)〜(2320, 1320) / 本作 (375, 126)〜(777, 528)）で比べる。`SCALE=2.97 sh observations/tools/forcefield_stats.sh <連写> 0.25 <閃光> 375 126 777 528`。
3. 1 枚ごとの太さは揺れるので、決めるのは **8 枚の自己相関の曲線**（5b と同じ `tmp/tk5b/acf.py` の作り。同じ値で 2 回撮って揺れの幅を先に出す）。
4. 本家との既知の差（5b の測りで出た。どれも V・F の窓、本作の値は今の `_build_wall02`）: **本作の幕は崩れるのが速い**（中央の窓で相関が 0.5 を切るのが本作 収録 0.38 s、本家 約 1.0 s）。太さは近い（細かい dx 21〜24・dy 12 対 本家 25〜28・13）。
5. 直したら材質を作り直して撮り直し、同じ表で確かめる。

## 決定事項

- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6〜+2.2 秒の画面全体の平均**（ステップ 2 で灯を決めたときと同じ窓）。ステップ 5 でもこの窓で揃える。用意は `observations/tools/telekinesis_begin.sh`。
- 2026-09-18: 本家の収録は**無劣化の連写**（19〜20 枚/s）。h264 の 4:2:0 は細い筋と、赤だけを足す印の細かさを崩すので、**11b5 で測った印の「細かさ 0.02〜0.03」は低めに出ている恐れがある**（ステップ 6 で `orig-telepathy-d025` から測り直す）。
- 2026-09-18: **`orig-telepathy-c1`（速さ 1）には発動前のフレームが無い**ので、背景には `orig-telepathy-d025/f000` を使う（同じ 1 回の生で、視点を動かしていない。動いたナースの画素は G・B の変化で外れる）。
- 2026-09-18: 本家の**幕は「幅の広い柔らかい光の筋」**で細い線の束ではなく、**終わりの破片は「角ばった板状のかけら」**（丸い粒ではない）。5c・5d はこの 2 点を目標にする。
- 2026-09-18（5a）: **地面の輪（`SHOCKWAVE_PANS`）は合わせる対象から外し、仮の値のまま置く**。3 つの連写のどこにも輪が写っておらず（見下ろした収録は幕の横帯で埋まり、正面の収録で床の奥に見えるのは光と筋）、本家の実機でもプレイヤーの視点から輪は見えないので、合わせる先が無い。根拠は `observations/README.md` の「力場の模様を測った」の 3 と `orig-tk-a025-ring-sheet.png`。
- 2026-09-18（5a）: **比べるのは同じ slomo で撮った収録の秒数**（ゲーム秒に直さない）。同じ τ・同じ収録のずれで 0.25 倍と 0.1 倍の収録を比べると崩れ方が 0.87 対 0.66 で、ゲーム時間に直すと 2.5 倍ずれるため（同 4）。
- 2026-09-18（5a）: 模様の崩れ方は**最良のずれで補正して**測る（ずれ 0 だと流れを変化に数えてしまう）。太さは高域を取ってから測る（取らないと画面全体の明るさに相関が支配される）。
- 2026-09-18（5b）: **オーラの `AURA_LAYERS` は今の値のまま**にした。自己相関の曲線で本家と比べると、試した 3 つの tiling は**同じ値で 2 回撮ったときの揺れ（合計 0.075 対 0.090）の中**で、今の値と区別できなかった。根拠と表は `observations/README.md` の「オーラの大きさを本家と比べた」。
- 2026-09-18（5b）: 決めるのは **1 枚ごとの太さではなく 8 枚の自己相関の曲線**。1 枚ごとの `width` は探す半径で頭打ちになるうえ揺れが大きく、5a の 3 枚の表はそのせいで本家を細く見せていた（本家の細かい dx は 22〜52 ではなく 22〜76）。**値を変える前に、同じ値で 2 回撮って揺れの幅を出す**。
- 2026-09-18（5b）: **本作と本家の px の倍率は 2.97**（床の市松の十字の広い段。画面の幅の比と同じで、上下の中心が揃う）。`forcefield_stats.sh` の `SCALE=2.97` がこれを使う。

## 要確認（ユーザー）

- **地面の輪の流れ（`SHOCKWAVE_PANS`）は仮の値のまま置いた**（2026-09-18、5a）。2026-09-17 の依頼は「オーラの 4 層と地面の輪の流れ」を詰めることだったが、本家の連写 3 件のどこにも輪が写っていない（プレイヤーの視点からは幕と光に隠れて見えない）。合わせたいなら、輪だけを狙った本家の追加の観察（球の外から見る方法があるか）が要る。

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`（git の外）。`observations/tools/` も git の外なので、5a・5b で作った `burst_flow.py`・`forcefield_stats.sh`・`telekinesis_burst.sh` はコミットに入らない（使い方は `observations/README.md` の tools の表に書いた）。`times.json` は `perf_counter` 秒で**等間隔ではない**ので、一定間隔を前提にする古い道具（`tele_fit/*`）に渡す前に時刻を差し替える。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**（`'NoneType' object has no attribute 'set_editor_property'` で止まる。play mode とは言われない）。`telekinesis_burst.sh` は PIE を動かしたまま終わるので、作り直す前に `python Tools/pie.py stop`。
- 5b の収録は `observations/ours/pie-tk-{a,b,c,d,e}025`（a・e が今の値、b・c・d が試した tiling）。曲線を比べる走り書きは `tmp/tk5b/acf.py`（git の外。`RUNS` に「名前・フォルダー・閃光の時刻・範囲・倍率」を足すだけで増やせる）。
- 背景で走らせた測りの出力を読むときは、パスを**スラッシュ**で書く（`"...	asks\$f.output"` は `\$` が展開を止めて空になる）。
- 本家のセーブは、観察の途中で捕まって書き換わっている（控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。**戻さない・編集しない**）。W-Editor は開いていない。
- 本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順・座標・つまずきは `.claude/guides/observation.md`（2026-09-18 に連写の速さ・死亡画面のボタン・印が画面空間であることを足した）。
- **Automation テストはエディタが前面でないと進まない**（背面は 3 fps で `FWaitForInteractiveFrameRate` が落ちる）。`Tools/ue_remote.py` で `execute_console_command(None, 'Automation RunTests Wasami')` を送ってから、`python Tools/desktop.py click 2680 83 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（タイトルバーの空き）で前面にし、`Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty` と `Result={Success}` を数える。エディタを開き直した後は MCP の `call_tool` が `Tool ... not found` になる（無人モードでは `/mcp` を頼めない）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 5b は `dd_powers.py` の注記だけを変え、**値とアセットは 5a の終わりと同じ**（試した値は最後に戻し、`/Game/Pipeline` と `/Game/DD` の 4 つの材質を作り直して今の値に戻してある）。本家は閉じたまま。
