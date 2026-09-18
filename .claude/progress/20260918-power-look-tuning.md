---
title: パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）
status: 進行中
branch: main
base: 559d87e
started: 2026-09-18 03:55
updated: 2026-09-18 09:45
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
- [ ] 5. テレキネシスの力場の材質を収録に合わせて詰める … 5a〜5d に分けた（2026-09-18）
  - [x] 5a. 連写を測る道具（`burst_flow.py`・`forcefield_stats.sh`）を作り、本家を測った。**地面の輪（`SHOCKWAVE_PANS`）は写っていないので対象外**。
  - [x] 5b. オーラの大きさを本家と比べ、**今の値のままにした**（px の倍率 2.97、8 枚の自己相関で比べると試した 3 つは今の値と区別できない）。
  - [x] 5c. 収録の仕掛けを 3 つ直して（窓の最大化・ソケットの持ち越し・PIE のクリックで回る視点）幕を測り直した。
  - [x] 5c2a. **V の窓の流れは対象外**（球の幾何と発動ごとの乱数で決まる）。
  - [x] 5c2b. **幕の広い模様の差は背景だった** … 2026-09-18。窓に写っているのは白飛びした廊下で、`room`（部屋をどれだけ測っているか）は奥の扉の窓で本家 0.81・本作 0.16〜0.38。**材質は変えていない**。値と表は `observations/README.md` の「幕の広い模様は背景だった」、道具は `tools/burst_bands.py`。
  - [x] 5d1. **破片は星屑（`dustSq`）で、本家のそれは四芒星**だと分かった … 2026-09-18。道具 `burst_blobs.py`（明るい塊を 1 つずつ拾う）と `forcefield_solo.py`（エミッタを 1 つだけにする）を作り、本作のエミッタ別の収録 4 件（`pie-solo-{aura,ground,sphere,dust}025`）で確かめた。**値もアセットも変えていない**。`observations/README.md` の「終わりの破片は星屑で、本家のは四芒星だった」。
  - [x] 5d2. **星屑を四芒星にした** … 2026-09-18。`swSQdust` の真偽を入れ替え、偽の側を「2 本の線（サンプルの R の `starDensity` 乗）を `Blend_Screen` → `DiamondGradient`（`Falloff` = `starDensity`）で腕を切る → `RadialGradientExponential` → `flashPower`」にした。`RadialGradientExponential` は中心でも 0.632 なので冪の内側に置けない。道具 `material_probe.py`（収録せずに材質の形を見る）。収録 `pie-dust-star2-025`。`observations/README.md` の「星屑を四芒星にした」。
- [ ] 6. Telepathy の印を収録に合わせて詰める（`TELEPATHY_PAN_*`、縁のこぶ）
- [ ] 7. 仕上げ（実装記録 04 の「既知の制約・注意点」と変更履歴、`observations/README.md`、`.claude/roadmap.md` の項目 23、`handover.md`、note の原稿）

## 次にやること

ステップ **5d3**（**芯の白飛び**）。5d2 で形は合ったが、芯の色が本家 (250, 255, 255) に対し本作 (1, 173, 223) のまま。

- **材質では直せないと分かっている**。Emissive は粒子の色 (0, 0.3225, 1)、UE 5 は Opacity を saturate する（`MaterialTemplate.ush` の `GetMaterialOpacity`、エンジンのソースで確認）ので、translucent の合成ではこの色より明るくならない。
- **筋は `dustSq` の `ParticleModuleColor` の `StartAlpha` 2**（cook にそうある。本作の `P_WasamiForceField` も同じか確かめる）。UE 4.21 が Opacity を saturate していなければ 2 × (0, 0.3225, 1) = (0, 0.645, 2) が書かれ、青が 1 を超えてトーンマッパー（AP1 へ移してチャンネルごとに曲げる）で白くなる。**まず UE 4.21 の `MaterialTemplate.ush` に saturate があるかを調べる**（手元に UE4 は無いので、pak やエンジンのソースが無ければ「合わせるための逸脱」としてユーザーに確認する）。
- 直すなら、材質ではなく**粒子の色か、Emissive に粒子の α を掛ける**形になる。原作のグラフから離れるので、決めたら実装記録 04 の `M_DD_KyStarDust` に理由を書く。
- 測り方: `tools/material_probe.py`（形は収録なしで見られる）と、収録は `forcefield_solo.py` で `dustSq` だけにしてから `sh observations/tools/telekinesis_burst.sh observations/ours/pie-dust-<版> 200`。終わったら `main('all')` で戻す。塊は `burst_blobs.py`、芯の色は 1 粒を切り出して読む。

そのあとはステップ 6（Telepathy）→ 7（仕上げ）。

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

- 2026-09-18（5d1）: **終わりの破片は `dustSq`（星屑）**。寿命から τ1.0 より後に残れるのはこれだけで（`aura` は τ1.0 まで、`ground`・`sphere` は寿命 1.0）、その頃は廊下が元の暗さに戻るので 1 粒ずつ拡大して見られる。**幕（`sphere`）は本作の τ0.7〜0.8 の塊のほとんどだが、5c2b で対象から外した**ので触らない。
- 2026-09-18（5d1）: **エミッタを 1 つだけにするには `DetailModeBitmask` を 0 にする**。LOD の `bEnabled` は系の読み込みで真に戻され、黙って全部入りのまま撮れる。切ったものが本当に消えたか 1 コマ見る。
- 2026-09-18（5d1）: **まばらな粒は帯の統計では測れない**（`room` が τ0.85 で 0.78〜0.97）。`burst_blobs.py` で 1 つずつ拾い、**短軸/長軸 > 0.2** だけを粒として数える（細長い塊は部屋の稜線）。
- 2026-09-18（5d1）: **芯の白飛びは「とても明るい青」の意味**。4 つの材質はどれも `BLEND_Translucent`・`MSM_Unlit` で Emissive は粒子の色（`dustSq` は R = 0）だが、UE のトーンマッパーは AP1 に移してからチャンネルごとに曲げるので、十分明るい青は R も 1 を超える。**色の推定を疑う前に明るさを疑う。**
- 2026-09-18（5d2）: **材質の形は収録せずに見る**（`tools/material_probe.py`）。ドメインを UI にしないと `DrawMaterialToRenderTarget` は真っ白、RTF_RGBA8 は線形の値。症状索引に 2 件。
- 2026-09-18（5d2）: **`RadialGradientExponential`（0.5・1）は中心でも 0.632**なので `starDensity` 乗の内側に置けない。**`starDensity` 乗の内側に入れた因子が形を独り占めする**（冪が残すのはスプライトの数百分の一）。症状索引に 1 件。
- 2026-09-18（5d2）: **材質を作り直した直後に PIE を撮らない**（シェーダーの準備中の文字がビューポートに出る。残りを数える API は無いので、撮る前に左上を 1 枚見る）。**`desktop.py` は代理人が落ちていることがある**（`start` → `ping` の `rect` を確かめる。最大化されていたら `click 2680 12 --count 2`）。症状索引に 2 件。

## 要確認（ユーザー）

- **幕の模様を合わせる対象から外した**（2026-09-18、5c2b）。2026-09-17 の依頼の「球の幕の筋（本作は少ない）」は、本家の連写では**幕ではなく背景（白飛びした廊下と、本作の代用の扉）を測っていた**と分かった。`_build_wall02` の推定は否定されていないが、この収録では良し悪しを決められない。詰めるなら、**幕が暗く特徴の無い背景を覆う所**で本家を撮り直し（球の中から壁や床だけが写る向き）、本作も同じ向きで撮る必要がある。
- **幕が画面を横切る速さ（V の窓の流れ）は対象外**（2026-09-18、5c2a）。球の幾何と発動ごとの乱数（球の回転とカメラの揺れ）で決まり、本家の 1 回と比べても材質の良し悪しが分からない。詰めるなら本家の複数回の収録が要る。
- **地面の輪の流れ（`SHOCKWAVE_PANS`）は仮の値のまま**（2026-09-18、5a）。本家の連写 3 件のどこにも輪が写っていない（プレイヤーの視点からは幕と光に隠れて見えない）。合わせたいなら、輪だけを狙った本家の追加の観察が要る。

## 再開時の注意

- 連写は `observations/original/`・`observations/ours/`、道具は `observations/tools/`（どれも git の外）。使い方は `observations/README.md` の tools の表（5c2b で `burst_bands.py` を足した）。`times.json` は `perf_counter` 秒で**等間隔ではない**。
- **PIE を動かしたままアセットを作り直すと `unreal.load_asset` が None を返す**（`'NoneType' object has no attribute 'set_editor_property'` で止まる）。`telekinesis_burst.sh` は PIE を動かしたまま終わるので、作り直す前に `python Tools/pie.py stop`。
- 本作の収録は `pie-dust-star2-025`（5d2 の直した後。閃光 f026 t=1.470）・`pie-dust-star025`（5d2 の失敗。粒が写らない）・`pie-solo-{aura,ground,sphere,dust}025`（5d1。閃光は f025 t=1.397 / f026 t=1.476 / f025 t=1.415 / f025 t=1.399）・`pie-wall-{a..e}025`（5c・5c2a）・`pie-wall-noshake{,-b,-c}025`（5c2a）・`pie-tk-{a..e}025`（5b）。5c2b の絵は `ours/veil-5c2b-scene.png`・`veil-5c2b-broad.png`。走り書きは `tmp/tk5d/`（5d1・5d2 の絵と `M_Probe_*.png`）・`tmp/tk5b/`・`tmp/tk5c/`・`tmp/tk5c2/`・`tmp/tk5c2b/`（git の外）。
- カメラの揺れを切って撮るには、PIE を始める前に `Tools/ue_remote.py` で `unreal.get_default_object(unreal.WasamiTelekinesisPower).set_editor_property('shake_class', None)`。戻すのは `unreal.load_class(None, '/Game/DD/Animation/01_Hotel/01_Hotel_Lobby_ElevatorShakeStop.01_Hotel_Lobby_ElevatorShakeStop_C')`（CDO なので保存は要らない）。
- 背景で走らせた測りの出力を読むときは、パスを**スラッシュ**で書く（`"...\tasks\$f.output"` は `\$` が展開を止めて空になる）。
- 本家のセーブは、観察の途中で捕まって書き換わっている（控えは `%LOCALAPPDATA%\DDeception\SaveBackups\pre-obs-20260918-044132`。**戻さない・編集しない**）。本家をもう一度起動するときはエディタを先に閉じる（VRAM 6 GB）。手順は `.claude/guides/observation.md`。
- PIE のビューポートの上に Automation のログやメッセージログの小窓が出ていることがある。`shot --region 1826 205 2978 859` で見て、出ていたら ×（約 (2199, 407)）を押す。**ビューポートの中を中心 (2400, 530) 以外で押すと視点が回る**ので、押した後は `pie.py place` で置き直す。

## 検証

- ステップ 5d2 は `dd_powers._build_star_dust` を組み直し、`make_telekinesis_materials()` で 4 つの材質を作り直した（C++ は変えていない）。`dustSq` だけの収録 2 件で確かめ、`forcefield_solo.py` のエミッタは `main('all')` で 15 に戻して保存済み。プローブの材質は `/Game/Pipeline/Debug`（git の外）に保存した。`check_records.py --update` は OK・7 件。本家は起動していない。エディタは `L_Hospital_Zone1`・PIE なし・未保存なし。
