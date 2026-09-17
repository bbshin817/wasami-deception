---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 08:30
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
  - [x] 5c. 収録の仕掛けを直して幕を測り直した … 2026-09-18。窓の最大化・ソケットの持ち越し・PIE のクリックで回る視点の 3 つを直して番人を付け、同じ値で 2 回撮って本家と比べた。**5b の「本作の幕は崩れるのが速い」は中央の窓では再現しない**。残る違いは「広い模様が本家の約半分」と「V の窓の流れが 3〜4 倍速い」。値と表は `observations/README.md` の「幕を本家と比べた」。
  - [x] 5c2a. **V の窓の流れを合わせる対象から外した** … 2026-09-18。球の湧く場所・大きさは本家のコードと cook のままで（カメラは球の中に居続ける）、流れは幾何と発動ごとの乱数（球の回転・カメラの揺れ）で決まる。本作 8 回で 58〜259、本家 1 回は41〜126（本家の値は `--ds 2` の 2 倍し忘れ。表を直した）。`observations/README.md`「幕の流れは合わせる対象から外した」
  - [ ] 5c2b. 幕の「広い模様」（本作 40〜48 対 本家 82 本家 px）を材質で足せるか試す
  - [ ] 5d. 終わりの破片を「角ばった板状のかけら」に見直す（形・数・尾）
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ 5c2b（幕の**広い模様**だけを本家に近づける）。差は `observations/README.md`「幕を本家と比べた」の表:
本作 40〜48 / 65 に対し本家 82 / 92〜102 本家 px（V・F の両方。細かい太さは合っている）。**流れはもう見ない**（5c2a）。

1. 狙いは「細かい筋の太さを変えずに、幅の広い柔らかい光の帯を足す」こと。今の推定は
   Opacity = `saturate(R + opacity) × 粒子の α` で、筋の上はほぼ不透明なので**手前の 1 枚しか見えていない**恐れがある。
   候補: 1 枚ごとの不透明度を下げて重なりで見せる（`opacity` を下げつつ Emissive を上げる）、
   `baseTex` の低い MIP を混ぜる、`baseColor` 側に広い勾配を足す。式は 10 個に収める（cook の数）。
2. 材質を作り直す前に **`python Tools/pie.py stop`**（PIE 中は `unreal.load_asset` が None を返す）。
   作り直しは `Tools/ue_remote.py` から `dd_powers.make_telekinesis_materials()`。
3. 測り方は 5c と同じ。`sh observations/tools/telekinesis_burst.sh observations/ours/pie-wall-<版> 200` を
   **同じ値で 2 回**、`python Tools/pie.py stop`、
   `SCALE=2.97 sh observations/tools/forcefield_stats.sh <連写> 0.25 <閃光> 375 126 777 528`。
   見るのは V・F の**広い模様**（`--ds` の後の px なので本作は ×2.97、本家は ×2）。
   **2 回の差より小さい変化は採らない**（5b の決まり）。変えたら**画面全体の明るさを測り直す**
   （閃光から +1.6〜2.2 収録 s の平均が本家 (96.9, 148.1, 191.3)）。

## 決定事項

- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6〜+2.2 秒の画面全体の平均**（ステップ 2 で灯を決めたときと同じ窓）。ステップ 5 でもこの窓で揃える。用意は `observations/tools/telekinesis_begin.sh`。
- 2026-09-18: 本家の収録は**無劣化の連写**（19〜20 枚/s）。h264 の 4:2:0 は細い筋と、赤だけを足す印の細かさを崩すので、**11b5 で測った印の「細かさ 0.02〜0.03」は低めに出ている恐れがある**（ステップ 6 で `orig-telepathy-d025` から測り直す）。
- 2026-09-18: **`orig-telepathy-c1`（速さ 1）には発動前のフレームが無い**ので、背景には `orig-telepathy-d025/f000` を使う（同じ 1 回の生で、視点を動かしていない。動いたナースの画素は G・B の変化で外れる）。
- 2026-09-18: 本家の**幕は「幅の広い柔らかい光の筋」**で細い線の束ではなく、**終わりの破片は「角ばった板状のかけら」**（丸い粒ではない）。5c・5d はこの 2 点を目標にする。
- 2026-09-18（5a）: **地面の輪（`SHOCKWAVE_PANS`）は合わせる対象から外し、仮の値のまま置く**。3 つの連写のどこにも輪が写っておらず（見下ろした収録は幕の横帯で埋まり、正面の収録で床の奥に見えるのは光と筋）、本家の実機でもプレイヤーの視点から輪は見えないので、合わせる先が無い。根拠は `observations/README.md` の「力場の模様を測った」の 3 と `orig-tk-a025-ring-sheet.png`。
- 2026-09-18（5a）: **比べるのは同じ slomo で撮った収録の秒数**（ゲーム秒に直さない）。同じ τ・同じ収録のずれで 0.25 倍と 0.1 倍の収録を比べると崩れ方が 0.87 対 0.66 で、ゲーム時間に直すと 2.5 倍ずれるため（同 4）。
- 2026-09-18（5a）: 模様の崩れ方は**最良のずれで補正して**測る（ずれ 0 だと流れを変化に数えてしまう）。太さは高域を取ってから測る（取らないと画面全体の明るさに相関が支配される）。
- 2026-09-18（5b）: 決めるのは **1 枚ごとの太さではなく 8 枚の自己相関の曲線**。1 枚ごとの `width` は探す半径で頭打ちになるうえ揺れが大きく、5a の 3 枚の表はそのせいで本家を細く見せていた（本家の細かい dx は 22〜52 ではなく 22〜76）。**値を変える前に、同じ値で 2 回撮って揺れの幅を出す**。
- 2026-09-18（5b）: **本作と本家の px の倍率は 2.97**（床の市松の十字の広い段。画面の幅の比と同じで、上下の中心が揃う）。`forcefield_stats.sh` の `SCALE=2.97` がこれを使う。

- 2026-09-18（5c）: **収録の枠はビューポートかどうかを毎回確かめる**。エディタの窓が最大化されていると
  (1826, 205)〜(2978, 859) はビューポートではなく、黙って別の視界とアウトライナーを撮る。番人は
  `observations/tools/check_viewport.py`（`telekinesis_burst.sh` が毎回呼ぶ）。症状索引にも書いた。
- 2026-09-18（5c）: **`pie.py place` は、クリックとキーを送り終えた後にもう一度呼ぶ**（PIE 中のクリックは
  マウスの差分だけ視点を回す）。ただし最初の 1 回は消せない（`pie_pose.py` はポーンから 800 cm 以内の
  エレベーターの扉を隠すので、隠す前にポーンが廊下に立っている必要がある）。
- 2026-09-18（5c）: **幕の「広い模様」の不足は幕だけの話**。同じ収録の A の窓（オーラ）では広い模様も
  本家と合うので、ブルームなど画面全体の後処理の差ではない。

- 2026-09-18（5c2a）: **幕が画面を横切る速さ（V の窓の流れ）は合わせる対象から外した**。球の湧く場所と大きさは
  本家のコード（`BP_TelekinesisPower` の bytecode・`BP_DD_PlayerCharacter` の SpringArm +95）と cook のままで、
  **カメラは球の中心の 85 cm 下に居続ける**（半径 2600 → 132 cm）。見かけの流れは この幾何（31 → 62）と、
  **発動ごとの乱数**（球の回転 Z 軸 ±0.05 回転/s = 最大 135、長さ 0.5 s のカメラの揺れ 50/35/10 Hz）で決まる。
  本作 8 回で 58〜259、本家 1 回は 41〜126 本家 px/収録 s。材質では決まらない。
- 2026-09-18（5c2a）: **`burst_flow.py` の `flow`・`width` が印字する px は `--ds` の後の px**。本家（`--ds 2`）は
  2 倍、本作（`SCALE=2.97` で `--ds 1`）は 2.97 倍して本家の px にする。5c の表の本家の流れ「20〜63」はこの 2 倍を
  忘れた値で、正しくは 41〜126（表は直した）。
- 2026-09-18（5c2a）: **球の湧く場所・大きさ・寿命・回転はもう疑わない**（本家のコードと cook と一致を確認済み）。
  力場の見え方で残る差は材質の推定の側だけ。

## 要確認（ユーザー）

- **幕が画面を横切る速さ（V の窓の流れ）を合わせる対象から外した**（2026-09-18、5c2a）。2026-09-17 の依頼の
  「球の幕の筋」のうち、太さ・崩れる速さ・薄れる間の流れは本家と合っているが、横切る速さは球の幾何と発動ごとの
  乱数（球の回転とカメラの揺れ）で決まり、本家の 1 回と比べても材質の良し悪しが分からないため。詰めるなら
  本家の複数回の収録が要る。
- **地面の輪の流れ（`SHOCKWAVE_PANS`）は仮の値のまま置いた**（2026-09-18、5a）。2026-09-17 の依頼は「オーラの 4 層と地面の輪の流れ」を詰めることだったが、本家の連写 3 件のどこにも輪が写っていない（プレイヤーの視点からは幕と光に隠れて見えない）。合わせたいなら、輪だけを狙った本家の追加の観察（球の外から見る方法があるか）が要る。

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`（git の外）。`observations/tools/` も git の外なので、5a・5b で作った `burst_flow.py`・`forcefield_stats.sh`・`telekinesis_burst.sh` はコミットに入らない（使い方は `observations/README.md` の tools の表に書いた）。`times.json` は `perf_counter` 秒で**等間隔ではない**ので、一定間隔を前提にする古い道具（`tele_fit/*`）に渡す前に時刻を差し替える。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**（`'NoneType' object has no attribute 'set_editor_property'` で止まる。play mode とは言われない）。`telekinesis_burst.sh` は PIE を動かしたまま終わるので、作り直す前に `python Tools/pie.py stop`。
- 収録は `observations/ours/pie-wall-{a..e}025`（5c・5c2a、同じ値で 5 回）・`pie-wall-noshake{,-b,-c}025`（5c2a、カメラの揺れを切って 3 回）と `pie-tk-{a..e}025`（5b）。曲線を比べる走り書きは `tmp/tk5b/acf.py`、測りの出力は `tmp/tk5c/`・`tmp/tk5c2/`（どれも git の外）。
- カメラの揺れを切って撮るには、PIE を始める前に `Tools/ue_remote.py` で
  `unreal.get_default_object(unreal.WasamiTelekinesisPower).set_editor_property('shake_class', None)`。戻すのは `unreal.load_class(None, '/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.01_Hotel_Lobby_ElevatorShakeStop_C')` を入れ直す（CDO なので保存の要る変更にはならない）。
- 背景で走らせた測りの出力を読むときは、パスを**スラッシュ**で書く（`"...	asks\$f.output"` は `\$` が展開を止めて空になる）。
- 本家のセーブは、観察の途中で捕まって書き換わっている（控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。**戻さない・編集しない**）。W-Editor は開いていない。
- 本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順・座標・つまずきは `.claude/guides/observation.md`（2026-09-18 に連写の速さ・死亡画面のボタン・印が画面空間であることを足した）。
- **Automation テストはエディタが前面でないと進まない**（背面は 3 fps で `FWaitForInteractiveFrameRate` が落ちる）。`Tools/ue_remote.py` で `execute_console_command(None, 'Automation RunTests Wasami')` を送ってから、`python Tools/desktop.py click 2680 83 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（タイトルバーの空き）で前面にし、`Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty` と `Result={Success}` を数える。エディタを開き直した後は MCP の `call_tool` が `Tool ... not found` になる（無人モードでは `/mcp` を頼めない）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 5b は `dd_powers.py` の注記だけを変え、**値とアセットは 5a の終わりと同じ**（試した値は最後に戻し、`/Game/Pipeline` と `/Game/DD` の 4 つの材質を作り直して今の値に戻してある）。本家は閉じたまま。
