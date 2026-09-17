---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 04:30
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# パワーの見た目の詰め（作業一覧の項目 23）

## 依頼

2026-09-17 のユーザーの回答（`.claude/roadmap.md` の項目 23）:

- **テレキネシスの球の粒子の灯を、本家の見え方に合わせて弱める**。今は原作の値（`P_ky_forceField_Telekinesis` の `sphere` の `ParticleModuleLight`、`BrightnessOverLife` 5）のままで、力場の間の画面が本家より明るい。同じ条件（Lv4・画質「高」・速さ 0.25）の画面全体の平均は本家 (95〜99, 141〜147, 186〜192)・本作 (123〜127, 174〜176, 216〜218)。線形の明るさで本家の灯の寄与は本作の約 0.5〜0.7 倍。弱める値は**原作から離れる本作の調整**としてコードと実装記録 04 に書く。
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**。力場 = オーラの 4 層と地面の輪の流れ、球の幕の筋（本作は少ない）、終わりの破片。Telepathy = 雲の流れる速さと向き、縁のこぶ（本作は丸に近い。前回の収録では雲の変わる速さは今の仮の値の 1〜1.5 倍かもしれない）。
- 撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。

詰める先: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_aura7`（`AURA_LAYERS`）・`_build_shockwave02`（`SHOCKWAVE_PANS`）・`_build_wall02`、Telepathy の `TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`（どれも `TODO(仮)`）。根拠は実装記録 04 の「既知の制約・注意点」、`observations/README.md`（ステップ 11b1・11b4・11b5）、`.claude/references/powers/03-telekinesis-vanish.md`・`04-primal-telepathy.md`。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 作成。
- [x] 2. テレキネシスの球の灯を弱めた … 2026-09-18。本作だけの写し `/Game/Wasami/Powers/P_WasamiForceField`（`BrightnessOverLife` 5.0 × `FORCE_FIELD_LIGHT_SCALE` 0.35 = 1.75）を作って C++ をそちらに向けた。画面全体の平均は本家 (97.0, 144.4, 190.1) に対し (125.0, 175.2, 217.5) → (95.1, 148.7, 193.0)。記録は実装記録 01・04 と `observations/README.md` の 11b6。
- [ ] 3. 本家で撮るものの一覧（観察の台本）を作る
  - 力場（オーラの層・地面の輪・球の幕・終わりの破片）と、**近くの**ナースの Telepathy の印について、場所・操作・速さ・範囲・測り方（`video_probe.py` の何をどう使うか）を先に書く。既存の収録で足りるものは除く。
- [ ] 4. 本家の実機（最新版）で撮る … 1 回・30 分を目安。エディタを閉じてから起動し、終わったら閉じてエディタを開き直す。
- [ ] 5. テレキネシスの力場の材質を収録に合わせて詰める（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`_build_wall02`）
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ 3（本家で撮るものの一覧 = 観察の台本を作る）。力場のオーラの層・地面の輪・球の幕の筋・終わりの破片と、近くのナースの Telepathy の印について、場所・操作・速さ・範囲・測り方（`video_probe.py` の何をどう使うか）を先に書く。既存の収録（`observations/original/orig-telekinesis-a/-b/-c`・`orig-telepathy-a/-b`）で足りるものは除く。台本は進捗記録か `observations/README.md` に書き、コミットしてからステップ 4（本家の起動）へ進む。

## 決定事項

- 2026-09-18: ステップの順は「灯→台本→本家の収録→材質→印→仕上げ」— 灯を先に直しておかないと、力場の材質を見比べるときに画面の明るさが灯の差で濁るため。
- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6～+2.2 秒の画面全体の平均**。ステップ 5（材質）でもこの窓で揃える。用意は `observations/tools/telekinesis_begin.sh`。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- 本家の実機を起動するときはエディタを先に閉じる（`.claude/guides/verification.md`）。収録は `Tools/desktop.py`、測るのは `Tools/video_probe.py`（`.claude/guides/observation.md`）。本家の全画面は 1 秒に約 10 枚しか撮れないので、一瞬の演出は MOD の `Console Command` で `slomo 0.25`。
- 収録した動画は git の対象外（`observations/` の下。README には測った値と条件だけを書く）。
- **Automation テストはエディタが前面でないと進まない**（背面は 3 fps で、`FWaitForInteractiveFrameRate` が 10 FPS を 600 秒待って落ちる）。`Tools/ue_remote.py` で `execute_console_command(None, 'Automation RunTests Wasami')` を送ってから、`python Tools/desktop.py click 2680 83 --allow WindowsTerminal.exe --allow UnrealEditor.exe`（エディタのタイトルバーの空き）で前面にし、`Saved/Logs/wasami_deception.log` の `Automation Test Queue Empty` と `Result={Success}` を数える。エディタを開き直した後は MCP の `call_tool` が `Tool ... not found` になる（無人モードでは `/mcp` を頒めない）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- check_records: OK（7 件。ハッシュは 01・04 を更新）
- C++ ビルド: 成功（警告なし）、テスト Wasami 29 本が成功
- エディタでの確認: `make_force_field()` で `/Game/Wasami/Powers/P_WasamiForceField` を作り、PIE で 2 回収録して測った（PIE は止めた。cvar は `aim_restore.py` で戻した）
