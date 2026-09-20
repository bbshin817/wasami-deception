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
- **テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める**。力場 = オーラの 4 層と地面の輪の流れ、球の幕の筋（本作は少ない）、終わりの破片。Telepathy = 雲の流れる速さと向き、縁のこぶ（本作は丸に近い）。
- 撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。

詰める先: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_aura7`（`AURA_LAYERS`）・`_build_shockwave02`（`SHOCKWAVE_PANS`）・`_build_wall02`・`_build_star_dust`、Telepathy の `TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`（どれも `TODO(仮)`）。根拠は実装記録 04 の「既知の制約・注意点」、`observations/README.md`（ステップ 11b1・11b4・11b5・11b6 と、ステップ 4・5a〜5c2b の節）、`.claude/references/powers/03-telekinesis-vanish.md` の 2.6・`04-primal-telepathy.md`。

## 計画

- [x] 1. 計画（この記録）… 2026-09-18 作成。
- [x] 2. テレキネシスの球の灯を弱めた … 2026-09-18。本作だけの写し `/Game/Wasami/Powers/P_WasamiForceField`（灯 5.0 × 0.35 = 1.75）。記録は実装記録 01・04 と `observations/README.md` の 11b6。
- [x] 3. 本家で撮るものの一覧（観察の台本）を作った … 2026-09-18。
- [x] 4. 本家の実機（最新版）で撮った … 2026-09-18、04:41〜05:15。無劣化の連写 5 件（`orig-tk-a025`・`orig-tk-b010`・`orig-tk-c010-down`・`orig-telepathy-c1`・`orig-telepathy-d025`）。条件と気づきは `observations/README.md` の「力場と Telepathy の印の無劣化の連写」。
- [x] 5. テレキネシスの力場の材質 4 つを原作のコンパイル済みシェーダーの式どおりにした … 2026-09-18（5a〜5c2b は収録を測る道具と、収録では決められないことの確認。5d1・5d2 は収録から式を読もうとして外し取り消した。5d3 星屑・5e 幕・5f オーラ・5g 地面の輪を `Tools/dd/cooked_shaders.py` で式どおりに）。本家の終わりの白飛びした横長の破片はオーラの欠片。経緯は `observations/README.md` の 5a〜5g の節、式は実装記録 04 の表。
- [ ] 6. Telepathy の印を詰める（`TELEPATHY_PAN_*`、縁のこぶ）。**先に `MM_Telepathy` の親の式を `cooked_shaders.py` で読む**（UI の材質もシェーダーマップを持つはず）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

2026-09-20 に**保留から進行中に戻した**（大目標 2 を達成し、始め方が「自動」の大目標 3「本家に忠実に」が進行中になったため。項目 23 はその節の最初の項目）。大目標 3 の決め方は今までどおり（本家の実機の観察・収録の見比べ・原作のコンパイル済みシェーダーの式の写し。`.claude/guides/original-fidelity.md`）。下のステップ 6 から続ける。

ステップ **6**（**Telepathy の印を式どおりにする**。5d3〜5g と同じ手順）。

- 読む: `python Tools/dd/cooked_shaders.py "Powers/Telepathy/MM_Telepathy."` と `"Powers/Telepathy/MM_Telepathy_Inst."`（インスタンスが自分のシェーダーマップを持つか `uniforms:` を見る）。UI の材質なので読むのは Slate のピクセルシェーダー（`M_Speedlines` は `--show 4` だった。入力に粒子の頂点の形が無い組）。表の後の cb3 の並びでパラメータを読む。
- 確かめたいこと: `Panner_0`・`Panner_1` の速さと向き（`TELEPATHY_PAN_A`・`TELEPATHY_PAN_B`）、TexCoord の倍率（`TELEPATHY_UV_TILING`）、2 つのノイズ（`T_ky_noise16`・`T_ky_noise`）のどのチャンネルか、放射グラデーションとのつなぎ方と倍率（`TELEPATHY_GAIN`）、`Speed`（インスタンスの値）の使い方。縁のこぶ（本家はいびつ、本作は丸に近い）が式から出るか。
- 組む: `dd_powers._build_telepathy` を式どおりに（仮の値は消す）→ `make_telepathy_materials()`（**PIE を止めてから**、`importlib.reload(dd_powers)`）。書き戻すファイルは改行を LF のまま保つ（`io.open` の newline に LF を渡す）。
- 確かめる: 11b5 の手順（`observations/README.md`「Telepathy の印の見直し」、道具 `tools/tele_fit`・`tele_scout.py`）で PIE の印を撮り、本家 `orig-telepathy-d025`（背景は `orig-telepathy-d025/f000`）と並べる。

そのあとは 7（仕上げ。`.claude/roadmap.md` の項目 23 の (2) の `AURA_LAYERS`・`SHOCKWAVE_PANS`・推定のグラフの記述も直す）。

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

- 2026-09-18（ユーザーの回答）: **5d2 の四芒星の取り消しと、計画を「式を写して収録で確かめる」に変えたことは了承**（「問題ありません。より確証を得られる要素が増えたのは良いことです」）。
- 2026-09-18（ユーザーの回答）: **幕が画面を横切る速さ（V の窓の流れ）は検討事項から外す**。決めているのは球の幾何・回転の乱数・カメラの揺れで、どれも本家どおり（コードで確認済み）。材質の分は 5e で原作の式になる。遊んで明らかに違って見えたら、本家を何回か撮って比べ直す。

## 要確認（ユーザー）

- （なし）

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`、道具は `observations/tools/`（どれも git の外）。使い方は `observations/README.md` の tools の表（5c2b で `burst_bands.py` を足した）。`times.json` は `perf_counter` 秒で**等間隔ではない**。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**（`'NoneType' object has no attribute 'set_editor_property'` で止まる）。`telekinesis_burst.sh` は PIE を動かしたまま終わるので、作り直す前に `python Tools/pie.py stop`。
- 本作の収録は `pie-ground-cook025`（5g。地面の輪だけ、式どおり。閃光 f024 t=1.403）・`pie-all-cook025`（5f。全部入り、式どおりの星屑・幕・オーラ。閃光 f025 t=1.397）・`pie-aura-cook025`（5f。オーラだけ。閃光 f025 t=1.402）・`pie-wall-cook025`（5e。式どおりの幕だけ。閃光 f025 t=1.419）・`pie-dust-cook025`（5d3。原作の式の星屑だけ。閃光 f027 t=1.511）・`pie-dust-star2-025`（5d2 の四芒星。閃光 f026 t=1.470）・`pie-dust-star025`（5d2 の失敗。粒が写らない）・`pie-solo-{aura,ground,sphere,dust}025`（5d1。閃光は f025 t=1.397 / f026 t=1.476 / f025 t=1.415 / f025 t=1.399）・`pie-wall-{a..e}025`（5c・5c2a）・`pie-wall-noshake{,-b,-c}025`（5c2a）・`pie-tk-{a..e}025`（5b）。5c2b の絵は `ours/veil-5c2b-scene.png`・`veil-5c2b-broad.png`。走り書きは `tmp/tk5g/`（5g の見比べ `cmp.py` と絵）・`tmp/tk5f/`（5f の見比べ `cmp.py`・`cmp_all.py`・`blobs.py`・`blobs_all.py` と絵。前の全部入り `pie-tk-a025` の閃光は f023 t=1.490）・`tmp/tk5e/`（5e の見比べ `cmp.py`・`blobs.py` と絵）・`tmp/tk5d3/`（5d3 の絵と、最初に書いた取り出しの小道具）・`tmp/tk5d/`（5d1・5d2 の絵と `M_Probe_*.png`）・`tmp/tk5b/`・`tmp/tk5c/`・`tmp/tk5c2/`・`tmp/tk5c2b/`（git の外）。
- カメラの揺れを切って撮るには、PIE を始める前に `Tools/ue_remote.py` で `unreal.get_default_object(unreal.WasamiTelekinesisPower).set_editor_property('shake_class', None)`。戻すのは `unreal.load_class(None, '/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.01_Hotel_Lobby_ElevatorShakeStop_C')`（CDO なので保存は要らない）。
- cook のシェーダーの出力は `Intermediate/Pipeline/dd/shaders/<名前>/`（git の外、作り直せる）。**`uniforms:` の名前は 1 度ずつ最初に出た順**で、cb3 の位置ではない（使い方から読む）。
- 背景で走らせた測りの出力を読むときは、パスを**スラッシュ**で書く（`"...\tasks\$f.output"` は `\$` が展開を止めて空になる）。
- 本家のセーブは観察の途中で捕まって書き換わったが、**このまま使う**（2026-09-18 のユーザーの回答。遊んで書き換わるのは気にしなくてよい。`.claude/guides/verification.md`）。控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順は `.claude/guides/observation.md`。
- **本家と同じ解像度で 1 枚撮るには**、PIE で `pause` してから `HighResShot 3440x1440`（`Saved/Screenshots/WindowsEditor/`）。`pie.py cmd` の遅れで狙った τ より遅れる（5f では Q から 3.2 秒待って τ≈1.0）。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 5g は `dd_powers._build_shockwave02` を組み直し（`SHOCKWAVE_PANS` を消した）、`make_telekinesis_materials()` で 4 つの材質を作り直した（`M_DD_KyShockWave02` の式 31 個 = cook と同じ。ログにコンパイルのエラーなし。C++ は変えていない）。`ground` だけの `pie-ground-cook025` を撮り、`forcefield_solo.py` は `main('all')` で 15 に戻して保存済み。PIE は止めた。`check_records.py --update` は OK。本家は起動していない。エディタは `L_Hospital_Zone1`・PIE なし・未保存なし。
