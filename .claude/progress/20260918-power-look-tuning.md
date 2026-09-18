---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 10:04
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（.claude/guides/progress-tracking.md の「記録を畳む」） -->

# パワーの見た目の詰め（作業一覧の項目 23）

## 依頼

2026-09-17 のユーザーの回答（`.claude/roadmap.md` の項目 23）:

- **テレキネシスの球の粒子の灯を、本家の見え方に合わせて弱める**（ステップ 2 で済んだ）。
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**。力場 = オーラの 4 層と地面の輪の流れ、球の幕の筋（本作は少ない）、終わりの破片。Telepathy = 雲の流れる速さと向き、縁のこぶ（本作は丸に近い）。
- 撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。

詰める先: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_aura7`（`AURA_LAYERS`）・`_build_shockwave02`（`SHOCKWAVE_PANS`）・`_build_wall02`・`_build_star_dust`、Telepathy の `TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`（どれも `TODO(仮)`）。根拠は実装記録 04 の「既知の制約・注意点」、`observations/README.md`（ステップ 11b1・11b4・11b5・11b6 と、ステップ 4・5a〜5c2b の節）、`.claude/references/powers/03-telekinesis-vanish.md` の 2.6・`04-primal-telepathy.md`。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 作成。
- [x] 2. テレキネシスの球の灯を弱めた … 2026-09-18。本作だけの写し `/Game/Wasami/Powers/P_WasamiForceField`（灯 5.0 × 0.35 = 1.75）。記録は実装記録 01・04 と `observations/README.md` の 11b6。
- [x] 3. 本家で撮るものの一覧（観察の台本）を作った … 2026-09-18。
- [x] 4. 本家の実機（最新版）で撮った … 2026-09-18、04:41〜05:15。無劣化の連写 5 件（`orig-tk-a025`・`orig-tk-b010`・`orig-tk-c010-down`・`orig-telepathy-c1`・`orig-telepathy-d025`）。条件と気づきは `observations/README.md` の「力場と Telepathy の印の無劣化の連写」。
- [ ] 5. テレキネシスの力場の材質を詰める … 5a〜5g に分けた（2026-09-18。5d3 から、**原作のコンパイル済みシェーダーの式を写し、収録は確かめに使う**）
  - [x] 5a〜5c2b. 連写を測る道具を作り、オーラの大きさは今の値のまま、地面の輪は写っていない、幕の流れと広い模様は収録では決められない（背景と幾何）と分かった。材質は変えていない。詳細は `observations/README.md` の 5a〜5c2b の節。
  - [x] 5d1・5d2. 終わりの破片を星屑と読んで四芒星にした（**5d3 で両方とも誤りと分かり取り消した**）。道具 `burst_blobs.py`・`forcefield_solo.py`・`material_probe.py` は使える。
  - [x] 5d3. **星屑を原作のシェーダーの式どおりにした** … 2026-09-18。道具 `Tools/dd/cooked_shaders.py`（cook のシェーダーの逆アセンブル。01 記録の「cook のシェーダーを読む」）。偽の側 = `DiamondGradient` × (twinkle + `starPower`) の小さなひし形で**白飛びしない**。本家の白飛びした四芒星は星屑ではない（幕らしい）。収録 `pie-dust-cook025`。`observations/README.md`「星屑は原作のシェーダーどおりのひし形だった」。
  - [ ] 5e. **幕（`sphere`、`_build_wall02`）を式どおりにし、白飛びした破片が幕かを確かめる**。
  - [ ] 5f. **オーラ（`_build_aura7`、`AURA_LAYERS`）を式どおりにする**（パンの向きと曲げは仮の値だった）。
  - [ ] 5g. **地面の輪（`_build_shockwave02`、`SHOCKWAVE_PANS`）を式どおりにする**（パンの速さは仮の値だった）。
- [ ] 6. Telepathy の印を詰める（`TELEPATHY_PAN_*`、縁のこぶ）。**先に `MM_Telepathy` の親の式を `cooked_shaders.py` で読む**（UI の材質もシェーダーマップを持つはず）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ **5e**（**幕を式どおりにする**）。

- 読む: `python Tools/dd/cooked_shaders.py "AdvancedMagicFX09/Materials/M_ky_wall02_4x4_two." --show 37`（メッシュの粒子の半透明のベースパス。スプライトは 05。どれがメッシュかは補間子の並びで確かめる）。5d3 で 05 と 37 を読んだかぎり:
  - Emissive = `Screen(baseColor, tex.rgb × (粒子の色 + 1))` = 1 − (1 − `baseColor`)(1 − tex × (1 + 粒子の色))（`mad r2.yzw, v2.xyz, tex, tex` の後に `c + x(1 − c)`。cb3[2] = `baseColor`、最後の lerp は `SelectionColor`）。tex は `baseTex` の SubUV の 2 コマの線形の混ぜ。
  - Opacity = saturate(深度のぼかし 100 cm〈定数 0.01〉× (tex.R + `opacity`〈cb3[4].y〉) × 粒子の α)。
  - 本作の推定（Emissive = `Lerp(baseColor, 粒子の色, R)`、Opacity = `saturate(R + opacity) × α`、深度のぼかし無し）と違う。**粒子の色は (0, 2.94, 20) → (0, 6.46, 44) → (0, 0.66, 5) なので、原作は筋の明るいところで白飛びする**。
- 組む: `dd_powers._build_wall02` を式どおりに（`dd_assets.depth_faded_opacity` の距離は定数 100）。`make_telekinesis_materials()` → PIE を止めてから。
- 確かめる: `forcefield_solo.py` で `sphere` だけ → `sh observations/tools/telekinesis_burst.sh observations/ours/pie-wall-cook025 200` → `main('all')`。本家 `orig-tk-a025` の τ0.7〜1.1 の白飛びした塊（f114 の (1233, 769)〜(1318, 814) は 104 画素が 245 以上）が出るかを `burst_blobs.py`（`bb.OURS` を差し替えて）と拡大で見る。**粒子系は閃光の 0.2 秒後に出る**ので、幕は τ1.2 まで残る。
- 5c2b の「幕の模様は収録では決められない」は、式を写せば要らなくなる（要確認の「幕の模様」を閉じる。5c2a の流れの速さは幾何で決まるので残る）。

そのあとは 5f（オーラ）→ 5g（地面の輪）→ 6（Telepathy）→ 7（仕上げ）。

## 決定事項

- 2026-09-18: 収録の比べ方は**青の閃光のフレームを 0 秒として +1.6〜2.2 秒の画面全体の平均**（ステップ 2 で灯を決めたときと同じ窓）。用意は `observations/tools/telekinesis_begin.sh`。
- 2026-09-18: 本家の収録は**無劣化の連写**（19〜20 枚/s）。h264 の 4:2:0 は細い筋と、赤だけを足す印の細かさを崩すので、**11b5 で測った印の「細かさ 0.02〜0.03」は低めに出ている恐れがある**（ステップ 6 で `orig-telepathy-d025` から測り直す）。
- 2026-09-18: **`orig-telepathy-c1`（速さ 1）には発動前のフレームが無い**ので、背景には `orig-telepathy-d025/f000` を使う（同じ 1 回の生で、視点を動かしていない）。
- 2026-09-18（5a）: **比べるのは同じ slomo で撮った収録の秒数**（ゲーム秒に直さない）。崩れ方は slomo にそのまま従わない。
- 2026-09-18（5b）: **本作と本家の px の倍率は 2.97**（床の市松の十字。画面の幅の比と同じで、上下の中心が揃う）。**値を変える前に、同じ値で 2 回撮って揺れの幅を出す**。
- 2026-09-18（5c）: **収録の枠がビューポートかを毎回確かめる**（番人 `tools/check_viewport.py`）。**`pie.py place` はクリックとキーの後にもう一度呼ぶ**。
- 2026-09-18（5c2a）: **球の湧く場所・大きさ・寿命・回転はもう疑わない**（本家のコードと cook と一致を確認済み）。`burst_flow.py` の印字の px は `--ds` の後の px（本家 ×2、本作 ×2.97）。
- 2026-09-18（5c2b）: **白飛びした窓で材質の模様を比べない**。V・F の窓に写るのは青く白飛びした廊下で、模様の大半は部屋の形。窓ごとに `room`（`burst_bands.py`。100〜700 px の帯と閃光の前の同じ窓との相関）を先に見て、0.3 を超えるならその窓で材質は決められない。**閃光の前のフレームで本家と本作の背景が揃っているかも先に見る**（本作の奥の扉は観察の代用のエレベーターの扉で、本家の赤い両開きの扉より 2 倍明るく、閃光の最中は真っ白に飛ぶ）。
- 2026-09-18（5c2b）: **40 本家 px より細かい帯は比べない**（本作のビューポートは 1152 px 幅で、TAA と解像度で細部が落ちる）。

- 2026-09-18（5d3）: **材質は先に cook のシェーダーを読んで式を写す**（`Tools/dd/cooked_shaders.py`。決め方の階段の「本家のコード」）。収録との見比べは写した後の確かめにだけ使う。5d1・5d2 は収録から式を読もうとして 2 回外した。
- 2026-09-18（5d3）: **粒子系は閃光の 0.2 秒後に出る**（BP の `Delay 0.2`。閃光は t=0 の `PostProcess1`）。寿命で粒子を見分けるときは足す（`aura`・`ground`・`sphere` は τ1.2 まで、`dustSq` は τ1.6 まで）。
- 2026-09-18（5d1）: **エミッタを 1 つだけにするには `DetailModeBitmask` を 0 にする**。LOD の `bEnabled` は系の読み込みで真に戻され、黙って全部入りのまま撮れる。切ったものが本当に消えたか 1 コマ見る。
- 2026-09-18（5d1）: **まばらな粒は帯の統計では測れない**（`room` が τ0.85 で 0.78〜0.97）。`burst_blobs.py` で 1 つずつ拾い、**短軸/長軸 > 0.2** だけを粒として数える（細長い塊は部屋の稜線）。
- 2026-09-18（5d1）: **白飛びは「とても明るい青」の意味**。UE のトーンマッパーは AP1 に移してからチャンネルごとに曲げるので、十分明るい青は R も 1 を超えて白くなる（幕の粒子の色は B が 20〜44）。
- 2026-09-18（5d2）: **材質の形は収録せずにも見られる**（`tools/material_probe.py`）。ドメインを UI にしないと `DrawMaterialToRenderTarget` は真っ白、RTF_RGBA8 は線形の値。症状索引に 2 件。
- 2026-09-18（5d2）: **材質を作り直した直後に PIE を撮らない**（シェーダーの準備中の文字がビューポートに出る。残りを数える API は無いので、撮る前に左上を 1 枚見る）。**`desktop.py` は代理人が落ちていることがある**（`start` → `ping` の `rect` を確かめる。最大化されていたら `click 2680 12 --count 2`）。症状索引に 2 件。

## 要確認（ユーザー）

- 2026-09-18: **5d2 の四芒星の星屑を取り消し、原作のコンパイル済みシェーダーの式（小さな水色のひし形・明滅・白飛びしない）にした**。理由: cook のシェーダーは `.uexp` に残っていて、`Tools/dd/cooked_shaders.py` で逆アセンブルすると式がそのまま読める（決め方の階段の「本家のコード」）。これに合わせて**計画を「収録から推定する」から「式を写して収録で確かめる」に変え、5e〜5g（幕・オーラ・地面の輪）を足した**。ステップ 6（Telepathy）も先に式を読む。場所: `dd_powers._build_star_dust`、進捗記録の計画。
- **幕の模様を合わせる対象から外した**（2026-09-18、5c2b）→ **5e で式を写すので、撮り直しは要らない見込み**。
- **幕が画面を横切る速さ（V の窓の流れ）は対象外**（2026-09-18、5c2a）。球の幾何と発動ごとの乱数（球の回転とカメラの揺れ）で決まり、本家の 1 回と比べても材質の良し悪しが分からない。詰めるなら本家の複数回の収録が要る。
- **地面の輪の流れ（`SHOCKWAVE_PANS`）は仮の値のまま**（2026-09-18、5a）→ **5g で式から読む見込み**（輪は本家の連写に写っていない）。

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`、道具は `observations/tools/`（どれも git の外）。使い方は `observations/README.md` の tools の表（5c2b で `burst_bands.py` を足した）。`times.json` は `perf_counter` 秒で**等間隔ではない**。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**（`'NoneType' object has no attribute 'set_editor_property'` で止まる）。`telekinesis_burst.sh` は PIE を動かしたまま終わるので、作り直す前に `python Tools/pie.py stop`。
- 本作の収録は `pie-dust-cook025`（5d3。原作の式の星屑だけ。閃光 f027 t=1.511）・`pie-dust-star2-025`（5d2 の四芒星。閃光 f026 t=1.470）・`pie-dust-star025`（5d2 の失敗。粒が写らない）・`pie-solo-{aura,ground,sphere,dust}025`（5d1。閃光は f025 t=1.397 / f026 t=1.476 / f025 t=1.415 / f025 t=1.399）・`pie-wall-{a..e}025`（5c・5c2a）・`pie-wall-noshake{,-b,-c}025`（5c2a）・`pie-tk-{a..e}025`（5b）。5c2b の絵は `ours/veil-5c2b-scene.png`・`veil-5c2b-broad.png`。走り書きは `tmp/tk5d3/`（5d3 の絵と、最初に書いた取り出しの小道具）・`tmp/tk5d/`（5d1・5d2 の絵と `M_Probe_*.png`）・`tmp/tk5b/`・`tmp/tk5c/`・`tmp/tk5c2/`・`tmp/tk5c2b/`（git の外）。
- カメラの揺れを切って撮るには、PIE を始める前に `Tools/ue_remote.py` で `unreal.get_default_object(unreal.WasamiTelekinesisPower).set_editor_property('shake_class', None)`。戻すのは `unreal.load_class(None, '/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.01_Hotel_Lobby_ElevatorShakeStop_C')`（CDO なので保存は要らない）。
- cook のシェーダーの出力は `Intermediate/Pipeline/dd/shaders/<名前>/`（git の外、作り直せる）。**`uniforms:` の名前は 1 度ずつ最初に出た順**で、cb3 の位置ではない（使い方から読む）。
- 背景で走らせた測りの出力を読むときは、パスを**スラッシュ**で書く（`"...\tasks\$f.output"` は `\$` が展開を止めて空になる）。
- 本家のセーブは、観察の途中で捕まって書き換わっている（控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。**戻さない・編集しない**）。本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順は `.claude/guides/observation.md`。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 5d3 は `dd_powers._build_star_dust` を組み直し、`make_telekinesis_materials()` で 4 つの材質を作り直した（式 31 個。ログにコンパイルのエラーなし。C++ は変えていない）。`dustSq` だけの収録 `pie-dust-cook025` で小さな水色のひし形を確かめ、`forcefield_solo.py` は `main('all')` で 15 に戻して保存済み。`check_records.py --update` は OK。本家は起動していない。エディタは `L_Hospital_Zone1`・PIE なし・未保存なし。
