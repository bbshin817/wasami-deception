# Chaotic Customer 2 参考資料（ステージ Zone_1 の原作データ）

2026-09-14 にユーザーの指示で、ステージを Dark Deception の Hotel から、ファンゲーム『Chaotic Customer 2』（UE5.4、Dark Deception の二次創作）のマップ `Chaotic_Customer_Zone_1` に差し替えた。この資料は、その原作データ（CUE4Parse の書き出し）の置き場所・形式・中身と、データから確かめたゲームの流れをまとめる。

- 本作の実装の説明ではない（それは `.claude/implementation-records/`）。`records:check` の同期対象外。
- ステージ（配置・メッシュ・マテリアル・灯・ボリューム・ギミックの配置と値・音）の根拠はこのデータ。UI・プレイヤーの操作・シャードの演出・死亡画面など、ゲーム全体の仕組みは引き続き Dark Deception（`pak_reference/`）が根拠（`.claude/guides/original-fidelity.md`）。
- 原作の素材の使用範囲は Dark Deception と同じ: ロゴとキャラクターのモデル（マネキン `manequin`、`Original`、`Dispatcher`、`Puppet_master`、`Doug_s_hands` などの `Characters_Chaotic_Customer/`）は使わない。オブジェクトとステージのモデル・マテリアル・テクスチャ・配置・音は使ってよい。キャラクターが描かれた絵（ポスターなど）は使う前にユーザーに確認する。

## 置き場所

| パス | 内容 |
| --- | --- |
| `<repo>/cc2_reference` | 書き出しの本体（3.3 GB、`.gitignore`）。2026-09-15 に `/Users/sbaba/Downloads/Windows/export`（以前はそこへのシンボリックリンク）からここへ移した。スクリプトは環境変数 `CC2_REF`（既定 `<repo>/cc2_reference`）で読む。`_fix_tools/ue_common.py` の `EXPORT` は自分の位置から求める |
| `cc2_reference/Chaotic_Customer_2/Content/` | ゲームの Content（`Content_game/`、`ThirdPersonBP/`、`StarterContent/` など） |
| `cc2_reference/Engine/Content/` | エンジンの素材（`BasicShapes/Cube` など） |
| `cc2_reference/_fix_tools/` | 書き出しを補修したスクリプト（`ue_common.py`: 読み方の共通処理、`fix_levels.py`: BP のメッシュを USD に差し込む、`fix_materials.py`: マテリアル → テクスチャの解決、`fix_decals.py`: デカールをメッシュに投影、`export_lightmaps.py`: 焼き済みライトマップの復号） |
| `cc2_reference/_lightmaps/Chaotic_Customer_Zone_1/` | 原作の焼き済みライトマップ（HDR と `lightmaps.tsv`）。617 区画のうち中身のあるのは 57 だけで値もごく暗いので、本作は使わない（Blender で焼き直す） |
| `cc2_reference/_backup_before_fix/` | 補修前の原本 |
| `cc2_reference/_asset_report/csv/` | 書き出し時の資産表（`images.csv` など。`fix_materials.py` が読む）。2026-09-15 に `/Users/sbaba/Downloads/Windows/CC2_Asset_Report` から移した |
| `cc2_reference/_raw/` | ゲームの pak を展開した生アセット（3.6 GB。`export_lightmaps.py` が焼き済みライトマップを復号するときに読む）。2026-09-15 に `/Users/sbaba/Downloads/Windows/Chaotic_Customer_2-Windows_extracted` から移した。ゲーム本体（exe・pak）は消した |

## 形式

- `<名前>.uasset.json` / `.umap.json`: パッケージのオブジェクトの配列（`Type`、`Name`、`Outer`、`Class`、`Template`、`Properties`）。既定値と同じプロパティは入っていないので、BP の部品の値は `Template`（BP のテンプレート）と親クラスをたどって解決する（`ue_common.resolve_prop`）。
- `<名前>.cpp`: Blueprint のバイトコードの逆コンパイル（疑似 C++）。Delay・DoOnce・Sequence の後の `goto` は誤りが多く、分岐は生バイトコードの `EX_PushExecutionFlow` で確かめる。
- `<名前>.glb`: スタティックメッシュ（m、glTF の Y 上）。`TEXCOORD_1` がライトマップ UV（メッシュの `LightMapCoordinateIndex`）。
- `<名前>.usda`: メッシュ（cm、Z 上、`st` / `st1`）とマテリアル（UsdPreviewSurface）。マップの `.usda` は配置を解決した USD で、スコープの名前がアクター名、BP の部品も差し込み済み。デカールは `<マップ>_Decals.usda` にメッシュとして投影済み。
- テクスチャは `<名前>.png`（ほぼ 2048²）。法線マップは UE の DirectX 規約（G 下）。
- 座標: UE は cm・Z 上・左手系。USD は (X, −Y, Z) cm（`ue_common.ue_loc`）。Blender 5.1 の USD 読み込みは m に直す（Blender = (X, −Y, Z) × 0.01）。
- Blender 5.1.2 に USD（`pxr` 0.25.8）が同梱されている。システムの `/usr/bin/python3` には numpy と PIL があり、pxr は無い。

## マップ `Chaotic_Customer_Zone_1`

- アクター 2,052（StaticMeshActor 791、`shard_C` 302、`Door_zone_1_Locked_C` 121、PointLight 117、DecalActor 113、`Lamp_zone_1_C` 48、RectLight 36 など）。配置のメッシュ 1,988 個・72 種、約 276 万三角形（`girlyand` の電飾 211 個で 77 万、ゾーン本体 `Zone_1_ENHANCED` 53 万、車 30 万）。マテリアル 148（不透明 134・マスク 7・半透明 6、発光 17）、テクスチャ 273 枚（ほぼ 2048²）。
- **2 層**:
  - 地上（UE z ≈ −3.3 m の床）: 駐車場のような通路の迷路（車・木箱・柵・クリスマスの飾り）。範囲 x −8,100〜6,700、y −33,000〜−5,000 cm（約 130 × 277 m）。
  - 地下（z ≈ −20.6〜−23 m）: 地下鉄の駅（ホーム x −8,930〜−2,670、z −2,060）と、列車・トンネル（x −21,500〜−10,700）。ホームの東端のつづら折りの階段で地上の南端へ上がる（通過点の推定 (−965, −5435, −2025) → (1410, −4705, −1245) → (10, −3705, −525)）。平面図で地上と重なる。
- **灯**（継承をたどった値。Stationary は既定の Mobility）: 可動の点光源 8 cd・半径 550 cm（52）、街灯 `Lamp_zone_1` の矩形ライト 3.2 cd・半径 900 cm・15 × 150 cm・扉の角度 70°（48）、罠の扉の点光源 8 cd（25）、地下鉄のホームの点光源 6〜7 cd・500〜600 cm（約 40）、ごく弱い補助光 0.03 cd（23）、看板などの矩形 0.4 cd（27）、ロックピックの障壁の矩形 1600（無単位）、スポット 15 cd（5）など約 330 基。単位は大半が Candelas。
- **ポストプロセスボリューム**:
  - `PostProcessVolume3`（地上全体。中心 (−682, −22728, 537)、半分の大きさ = 拡縮 × 100 cm: (16763, 25526, 1862)）: WhiteTemp 7485.7、ColorSaturation (1.1, 1.05, 1, 0.9)、ColorGamma (0.95, 1.05, 1.04, 1)、ColorGain (1.1, 0.9, 1.1, 1.15)、ColorOffsetShadows w −0.007、ColorSaturationMidtones w 1.1、ColorGammaMidtones (1.09, 1, 1, 1)、BloomIntensity 1.1、手動露出（ISO 4500、シャッター 1 s、f 4.5）、VignetteIntensity 0.2、AO 強さ 1.0・半径 90 cm、SSR MaxRoughness 1.0、ColorGradingLUT `ThirdPersonBP/Blueprints/TX_LUT_02`（256 × 16）強さ 0.7、MotionBlurAmount 0、LensFlare 強さ 2.07・しきい値 9.82。
  - `PostProcessVolume4`（地下鉄。中心 (−10067, −5523, −2287)、半分 (12103, 1928, 1000)、Priority −1）: WhiteTemp 7900・Tint 0.1、Saturation / Contrast / Gamma / Gain / GainShadows / OffsetShadows、SceneColorTint (1, 0.930, 0.961)、BloomIntensity 1.35、手動露出（ISO 4800、1 s、f 4.4）、ColorGradingLUT `Content_game/LUTs/LUT_U1_Filmic_Horror_Night` 強さ 0.12、ほかは上と同じ。
  - `PostProcessVolume_1`（無限範囲）: ブレンダブル `Stylized_light` の重み 0（効かない）。
- **霧** `ExponentialHeightFog`（z −273）: FogDensity 0.3、FogInscatteringLuminance 0.01、DirectionalInscatteringLuminance (0.25, 0.25, 0.125)、体積フォグ有効（VolumetricFogEmissive (0.012, 0.0127, 0.02)）。灯の VolumetricScatteringIntensity は街灯 0.5。
- **SkyLight**（Movable）: `Content_game/HDRI/HDRI_Epic_Courtyard_Daylight`（`.hdr` あり）、Intensity 0.75、LightColor sRGB (86, 105, 114)、LowerHemisphereColor 0.05。
- **反射キャプチャ**: 箱 2 つだけ（地下: (−5787, −5475, −2510) 半分 (4300, 1000, 1000)、階段: (238, −4320, −1650) 半分 (2000, 2000, 1000)）。地上はスカイライトだけ。
- `DefaultEngine.ini` の描画設定は `r.CustomDepth=3`、`r.AntiAliasingMethod=3` だけ（Lumen などの明示は無い）。

## ゲームの流れ（2026-09-14 の調査。レベル BP・各 BP の生バイトコードから）

座標は UE の cm。「推定」と書いたもの以外はデータで確かめた。

### 進行とチェックポイント
キャラクターの整数 `checkpoint`（セーブスロット "checkpoint"）。0 = 列車の中で開始、1 = 鉄パイプを取って列車が駅へ走る、2 = 地上のホールの `trigger2` に入ってマネキンが出る、4 = 配電盤の後の脱出フェーズ。リスポーン位置: 1 (−6729, −4882, −2349) yaw −90、2・3 (2362.6, −5643.6, −234.2) yaw 180、4 (−804, −30720, −251) yaw −90（シャードを全部消して shard = 0、2 秒後に Mannequin_BP を全部消す）、0 は PlayerStart2（(−18491, −4286, −2320)、列車の中）。残機 `lifes` = 5（死亡で 1 減らし、残れば 3.5 秒後に RestartLevel、0 でゲームオーバー）。タイマーは無い。

### 導入（checkpoint 0 → 1）
checkpoint 0 では Act2 のカットシーン（15.8 s）の間 16 s 入力を切り、`Platform_tunel` ×8 がトンネルのループで走行中に見せる。列車内の Box2 で次のカットシーン、4 s 後に (−16889, −4438, −2320) へ移し、13 s 後に鉄パイプ `Metal_pipe_bp`（(−17139, −4431, −2197)）を出す。取ると checkpoint = 1 で保存し、2 s 後から 9 s かけて列車が x −18,110 → −7,201 へ（約 12 m/s）、10.5 s 後に扉 6 枚が開く。ホームを東へ歩き、階段を上ると `TriggerBox_3`（(−100, −3480, −530)、階段の上）の先が地上（エレベーターや梯子の BP は無い）。

### 地上（checkpoint 2）
- `trigger2`（(1378, −6162, −292)、範囲 x −1259..4015・y −6915..−5409）: checkpoint < 2 なら 2 にして保存し、プレイヤーの BP の `music_manequins_start()`（マネキンの BGM。Zone_3 では `Dispatcher_music_start()`）、1 s 後に `Spawn_Enemies` 10 体（`Mannequin_BP_C` を自分の位置の 100 cm 上に 1 体ずつ）。trigger の BP 自身は音を持たない。
- **シャード** `shard_C`: 半径約 70 cm の球に触れると消え、Niagara 0.5 s、Pickup_v2/v3/v4 を重複なしのランダム（FadeIn 0.3）、カメラシェイク t_2 強さ 3・0.2 s、`player.shard −= 1`、名前をセーブ。本来の範囲に 301 個（`shard_3` だけ (333744, −1023585) の届かない所。キャラの BeginPlay は `shard = 最後の ArrayIndex` = 301 で、301 個取ると 0 になる）。`actor_bag_fix_shards`（302）は各シャードの子の空のアクターで、タブレットの矢印が最寄りのシャードを指すのに使う（100 以下で働く、推定）。
- shard = 0 で: `barrier_C` 2 枚（`barrier2` (−810, −30189) 南の配電盤室、`barrier_2` (963, −3498) 階段の上・エレベーターの脇）が 1 s ごとの監視で気づいて爆発し 3 s 後に消える、`Door_TRAP` が反応しなくなる、`Lamp_zone_1` の点滅が止まる。
- **特殊シャード**: どちらも最初は地図の外（x ≈ 20,200）、60 s ごとに 4 か所から重複なしのランダムで移る。`red_shard`: (−6120, −19898)、(477, −19004)、(−4727, −30846)、(−820, −6159)。取ると "enemies_revialed"、全 Mannequin_BP を可視化（シャードの数は減らない）。`stun_orb`: (4422, −21559)、(−3483, −24210)、(1779, −30852)、(465, −14992)。取ると全マネキンの移動と衝突を止め、17 s 後に戻す。
- **障壁**: `Interactivate_barrier` ×7 は 400 cm 以内で interact するとロックピックの連打のウィジェット、`Percent_to_Finish ≥ 0.25` で割れて 1 s 後に消える。`barrierspeedbust` ×4 は `MaxWalkSpeed ≥ 900`（ダッシュ・ブースト中）で当たると割れる（checkpoint ≥ 2 で読み込むと 5000 cm 以内のものは消える）。
- **配電盤**（`Lamp_zone_1_on_off_2`、(−635, −31016)）: "Ele" を interact → 1 s 後に移動を止める → 2 s 後に `Activate_escape_seq`（checkpoint = 4 で保存、SpotLight を 8000、1 s 後にフェード、2 s 後に `Set_camera_anim = 1`）→ レベル BP が `CUTSCENE_END_ZONE_2`（7.4 s）を流して RestartLevel。

### 脱出フェーズ（checkpoint 4）
- BeginPlay: 入力を 0.8 s 切り、`Truck_boss` を Spline_3 に出し（NewVar_0 = 8: スプラインを 8 s で走り切る。Spline_3 は 8,362 cm）、`Escape_zone_1_Blueprint` を (−809, −29645, −334) に出し、`barrierspeedbust6` を消し、0.2 s 後に `Mannequin_Escape` 14 体（x −441..−1146、y −31586..−32696、z −247、yaw 90）。
- `Escape_zone` の箱: Box10 (−784, −22335) → priority 1、Box11 (4569, −21161) → 2、Box12 (3817, −18421) → 3、Box13 (−2149, −14228) → 4（それぞれトラックを Spline_1 2.5 s / Spline_4 2 s / Spline_5 7 s / Spline_6 2 s で DoOnce）。Box15 (296, −19080) → Mannequin_Escape 3 体（x 1553）、Box16 (−802, −7004) → 2 体、Box17 (743, −5451) → `Manequin_Boss_fight_bp` 2 体が (966, −5636 / −5836, 410) から落ちてくる。非表示の Cube の壁 10 枚（100 × 600 × 500 cm）。
- トラックは Big door を壊し、`Gazelle_boom_point` で止まり、触れると死亡。`Lamp_zone_1` は checkpoint 4 で全部消灯。
- 終点: `TriggerBox_1`（(−269, −7063, −269)）で追手の Mannequin_Escape を全部消す。`Lamp_zone_1_on_off` の Box（約 (1870, −4218)）に checkpoint 4 で入ると Act3 のカットシーン（13.4 s）、7 s 後に `Stone_broken`（(2606, −3494)）、7.5 s 後に "End_of_the_act"。

### 敵と罠
- `Mannequin_BP`: PawnSensing で見つけ、追跡の速さはプレイヤーとの距離を 810〜1100 cm/s に収めた値、4500 cm 以上離れると諦める、触れると即死、300 cm 以内の Big door を開ける。`Respawn_manequin` ×8: 死んだマネキンを `RandomInteger(11)` の一致した地点に倒れた状態で出し、2 s 後に動き出す。
- `Door_TRAP` ×25: shard ≠ 0 の間、入るたびに 4 択を重複なしのランダムで引き、1 回だけ扉が破れて灯が消え、木の割れる音、Mannequin_BP を 1 体出す。
- `Pair_trap` ×6: 床の範囲に乗ると 3 s 噴き出し、4 s で再装填。触れると死亡。
- `car_trap` ×2: 7 s 周期で車が通る（色は 4 種からランダム）。触れると死亡、マネキンは轢かれて止まる。checkpoint 4 では止まる。

### 扉
`Door_zone_1_Locked` ×121 は開かない（interact で施錠音、4 s のクールダウン）。`Big_door_zone_1_BP` ×20 はシャッターで、`Open()` で開閉（ダッシュ・トラック・Mannequin_Escape で壊れる）。`Door_zone_2` ×6 は interact でプレイヤーと反対側へ開く（0.7 s）。`Vent_door_BP` ×13 は interact でダクトの扉を開閉。`Doors_train` ×12 は列車のスクリプトが開閉。`Blocker_Bp` ×25 は interact でスライドして消える。

### 灯
- `Lamp_zone_1` ×48: 矩形 3.2 cd。敵に見られている間は 0.4 s のループで 3.2 → 1.0 を 0.1 s 刻みに点滅、解けると 2 s かけて 3.2 → 0（0.4 s）→ 3.2。checkpoint 4 では全部消灯（光を隠し暗いマテリアル）。
- `Light_retro_breaking` ×7: 点光源 0.4 cd（230 cm）。0 ↔ 0.4 を 0.2 / 0.45 / 0.1 / 0.1 / 0.1 / 0.2 / 0.2 / 0.5 s で繰り返す（1.85 s 周期）。
- `Lamp_broke`: 静的。

### レベル BP のその他
`TriggerBox_2`（(1670, −3490, −150)）と `TriggerBox_3` は地図の平面（`Plane_2` = 地上の地図 `CC_ZONE_1_ENHANCED_MAP_Mat`、`Plane2` = 地下の地図）とシャードのアイコンの表示を切り替える（ミニマップ用、推定）。`StaticMeshActor_1`（(2970, −3500, −383)、非表示の 500 × 475 cm の箱）は衝突の壁で、ずっと残る（推定）。

## ギミックの BP の詳細（2026-09-14 の調査。生バイトコードの逆アセンブルで分岐を確かめた）

座標は UE の cm（ワールド）、時間は s。`.cpp` の `goto` は誤りがあるので、分岐は生バイトコードで照合した（例: Lamp の Timeline_1 の Update は点滅のゲートを閉じない）。上の節と食い違う点はこちらが正しい。

### 上の節から直した点
- マネキンは見えただけでは追わない。Tick で「追跡中でない・気絶していない・プレイヤーが Vanish でない・距離 < 1000・プレイヤーの速度 > 350」のとき正体が明かされ（`eye_revealed`、顔の板、叫び `Scream_mannequin_CUE`（1000/6000）、2 s 止まる）、PawnSensing（SightRadius 5500、SensingInterval 0.1、聴覚は未使用、周辺視野角は既定の 90° と推定）はその後の追跡にだけ使う。
- 街灯の点滅は灯ごとの視線ではない。マネキンが追跡を始めるたびに全 48 灯が一斉に点滅する。
- 速度障壁（`barrierspeedbust`）を割るのはスピードブーストだけ（ブースト中の `MaxWalkSpeed` 1200。ダッシュは `LaunchCharacter` で飛ぶだけ）。
- checkpoint 4 の街灯は `Escape_from_zone1` の直後に `Escape_from_zone2` も呼ばれ、1.5 s 後に色が巡回する光で点き直る（実機での確認が要る）。
- 死んだマネキンは約 46.7 % の確率で `Respawn_manequin` から戻らない。

### プレイヤー（ThirdPersonCharacter）
- 速さ（`MaxWalkSpeed`）: 歩き 300、走り 650、しゃがみ 200、ブースト中 1200（変数 `speed bust`。ブーストはカメラシェイク `Speed_Boost_Shake_2`、2D の音、Delay 22 s）。ブーストの開始で 300 cm 以内の速度障壁に `Speed_Barrier_Fix()`（Box の中なら割る）。
- interact: FollowCamera から前方 500 cm、半径 5 の球トレース（TraceTypeQuery2）。キーの経路は未確定（E が 2 つある）。
- checkpoint 4 の BeginPlay: 街灯を全部 `Escape_from_zone1` → `Escape_from_zone2`、1.5 s 後に `CC_Merry_horrors` を `FadeIn(0.1, 1, 6)`（0.1 s のフェードで、曲の 6 s から。以前「FadeIn 6 s」と書いたのは誤り）、10〜18 s 後に `PARKOVKA_LOMAETCA` と Timeline_29。
- BP が鳴らす音: `05_Tablet_Woosh_v1/v2_1`、`AA_-_Dash_c(_2)`、`Crouch`、`Icon_Move_v2`、`Pause_Sound_v1`、`UI_Pause`、`Speed_Boost_Sfx…_Cue`、`Stun_Wave_Attack_New_04`、`Telepathy`、`telekinez`、`Teleport_Committed`、`Teleport_Mode_Entered`、`power_refilled_Cue`。音楽は `Content_game/Audio/Soundtrack/Act_1_Soundtrack/`（`CC_Merry_horrors`・`CC_Carol_Of_the_bells`・`CC_Merry_Dethsmass`・`CC_Retro_Horror`）。`Eq_Conctroller`（`CC_Retro_Horror`）はマップに置かれていない。マップに AmbientSound は無い。

### ロックピックの障壁 `Interactivate_barrier`（7）と `Lockpicking`
- 配置（z −122）: 10 (−6089, −30182)、5 (−794, −11701)、6 (−803, −19618)、7 (−3451, −23578)、8 (1837, −23582)、9 (4467, −30182)、_2 (1825, −5296, −220)。_2 以外は root ×1.7・Box (4,4,5) で半分 (218, 218, 272)、_2 は ×1・Box (4,6,5) で半分 (128, 192, 160)。
- 部品: 板 Plane ×3（Roll 90、`Interactive_Barrier_mat`）、不可視の Cube（衝突）、RectLight ×2（Intensity 1600（単位の指定なし）、FFBD59、310×310、減衰 550、Volumetric 3。_2 以外は SourceWidth 270）、ループ音 `Barrier_Loop`（600 + falloff 1000）、粒子 `explousion_interaction_barrier`、画面空間のウィジェット `Lockpicking_C`（最初は非表示）、ActorSequence（拡縮 3 → 2.8 → 3、2.5 s のループ）、色の Timeline_0（4 s のループ、R 1 → 0.25 → 1・G 0.389 → 0.097 → 0.389・B 0.06 → 0.015 → 0.06、キー 0.006 / 1.994 / 4.006 s、材質の "Param"）。
- Box に入るとウィジェットを出し、出ると隠す。入力は **F**（と FaceButton_Left）: 全障壁のうち `GetDistanceTo ≤ 400` の最初の 1 基の `F_barrier()` → Box の中ならウィジェットの `f tikanie()`。押すたびに（Gate が開いている間）`NewAnimation`（キャンバスを 0.15 s で −8°・1.07 倍、0.25 s で戻す。接線 0 の三次）を再生し、`progress += 0.05`、表示の `Percent = FInterpEaseInOut(Percent, progress, 0.1, 0.03)`。Delay 0.01 のループで `progress ≥ 0.251` なら成功（入力と減衰を閉じ `Percent = 0.25`）、`progress ≤ 0` なら 0 にしてそのフレームは何もしない（表示は止まる）、それ以外は Tick で fps = trunc(1/dt) により ≤30: −0.002、≤60: −0.0013、≤120: −0.0012、≤240: −0.0009、それ以上 −0.0007 を引き、`Percent = FInterpEaseInOut(Percent, progress, 0.1, 0.1)`（押した時と指数が違う。実質 0.06〜0.08 /s）。最低 6 回。失敗は無い。円環は `Image_101`（`progressbar_Inst1`、親 `krugovoy_proggress`、DAAB00、`Perentage`、max 4。0.25 で満環と推定）、文字 "F"（85 pt）。
- 成功: ループ音の位置で `Barrier_Shatter_Cue`（1, 1）と粒子、板を隠して衝突を切り、Cube の衝突を切り、ループ音を止め、灯 2 つとウィジェットを破棄、1 s 後に DestroyActor。checkpoint ≥ 2 で読み込むと、プレイヤーの BeginPlay の 0.1 s 後にプレイヤーから 1000 cm 以内のものを破棄。

### 速度障壁 `barrierspeedbust`（4）
- 5 (496, −15660, −180) yaw 180 ×1.3、6 (−2131, −15660, −180) yaw 180 ×1.3、7 (−802, −24901, −180) yaw 180 ×1.3、_2 (−1414, −5448, −2319) yaw 90 ×1（Box を (0, −66, 0) へ）。
- Box（Roll 90、拡縮 (7.075, 5.216, 3.664)。局所の半分は幅 226・厚み 117・高さ 167 × root の拡縮）、板 Plane ×5.4（`speed_bus_barriert`）、RectLight 3 cd・FF595C・565²、粒子 `explousion_speed_boost_barrier`、色の Timeline 4 s（R 1 → 0.25 → 1、G 0.083 → 0.017 → 0.083、B 0.06 → 0.015 → 0.06）。
- Box に入った瞬間に `MaxWalkSpeed ≥ 900` なら割れる（`Barrier_Shatter_Cue`、粒子、隠して衝突を切り、灯を破棄、1 s 後に消える）。900 未満で触れると（一度だけ）板の衝突が QueryAndPhysics になり壁になる。
- BeginPlay の 0.1 s 後、レベルが Zone_1 で checkpoint ≥ 2 で、その時点のプレイヤー（リスポーン地点）から 3D で 5000 cm 以内なら破棄（cp2/3 のリスポーンから _2 まで約 4318 cm）。cp4 ではレベル BP が `barrierspeedbust6` を破棄。

### シャードの障壁 `barrier_C`（2）
- barrier2 (−810, −30189, −118) 回転 (0, 90, −90) ×1.3（Box の中心 ≈ (1939, −3495, −118)、半分 ≈ (949, 442, 156)）、barrier_2 (963, −3498, −194) ×1.3（Box ≈ (2203, −3498, −348)、半分 ≈ (899, 388, 111)）。どちらの Box も階段の上。
- 板 Plane ×3.5（`school_decal_speedBarrier_01_A_Mat_Inst`、最初は非表示）、RectLight ×2（既定 1500 をレベルで 450、FA82FF、351.9 × 319.4）、音 `Stun_Wave_Attack_New_01`（出現）・`_02` = `Barrier_Shatter_Cue`、粒子 `Soul_shard_barrier_explo`、Timeline 4 s（R 0.556 → 0.165 → 0.556、G 0、B 1 → 0.3 → 1）。
- BeginPlay の 0.1 s 後: checkpoint 7 なら破棄、2 なら表示・衝突・灯・ループ音、それ以外（4 を含む）は非表示・衝突なし（このとき割れる音と粒子が 1 回出る）。1 s ごとに `shard == 0` を見て、一度だけ Timeline を止め、板を隠し（衝突は残る）、灯を消し、割れる音と粒子、ループ音を止め、3 s 後に DestroyActor。checkpoint 1 で Box に触れると 1 回目は出現、2 回目以降は `not_yet_C` ウィジェット（4 s のクールダウン）。

### 罠の扉 `Door_TRAP`（25）
- 部品: `Modul_wall_Door` ×0.5、Box（(−281, 10, 50)、半分 (272, 414.5, 32)。扉の手前の床の板）、Sphere（半径 32。全配置で (82, 0, 45)、Door_TRAP5 だけ (98, 0, 45)。扉の奥）、PointLight 8 cd・FF7070・半径 550（(11, −39, 163)）、不可視の Cube（衝突）、音 `Locked_Door_v1`（600/2400）と `18-Wood_Door_Breach_Destroy_FromOutside_V1`。
- Box にプレイヤーが入り `shard ≠ 0` なら、一度だけ MultiGate（4 出力・ランダム・ループなし）: 0/2/3 は何もせず 1 s 後にまた反応、1 は扉を破って以後反応しない（1〜4 回目の進入のどれかで必ず 1 回、各 1/4）。破る: ActorSequence 0.521 s（局所 X 0 → −60（cubic、0.317 s）・Z 0 → 14（線形、0.212 s）・Pitch 0 → 90（線形、0.212 s）・Yaw 0 → 10（cubic、0.317 s）。扉が手前に倒れる）、PointLight を破棄、Cube の衝突を切り、木の割れる音、Sphere の位置に `Mannequin_BP`（回転 0）。

### 床の噴出 `Pair_trap`（6）
- z −279、Pitch 90。2 (−1449, −20310)、3 (−133, −20252)、4 (4846, −22921)、5 (4032, −23189)、6 (−7035, −20277)、_2 (35, −6876)。Box（噴射・致死）と Box1（踏むと作動）は同じ中心で、ワールドの半分は Box (66, 142, 65)、Box1 (434, 142, 65)（Pair_trap_2 だけ X と Y が入れ替わる）。Niagara `Pair_trap`、音 `PAR`（700 + 7000）。
- BeginPlay の 0.1 s 後に Box の衝突を切り、0.01 s ごとに監視。Box1 に乗っていれば一度だけ: Box の衝突を入れ、Niagara と PAR、3 s 後に衝突を切り Niagara を止め、さらに 4 s 後に再び作動できる（乗り続けると 7 s 周期）。Box に触れると一度だけ `death()` と `Death_C`。

### 車 `car_trap`（2）
- car_trap2 (8978, −24239, −335) yaw 90、car_trap_2 (−10190, −18959, −335) yaw −90。Box（車体・致死。半分 (111, 326, 124)）、車輪（ActorSequence で Roll 0 → 360 を 0.375 s のループ）、音 `Car_SFX_N_mute`（常時、1000/5000）・`Car_horn`（1000/5000）、Box1（クラクション。半分 ≈ (322, 1539, 205)、車の約 1219 cm 前と推定）。
- Timeline 8 s・線形 0 → 22000 で Box の相対位置 (0, 値, 90)（2750 cm/s）。最初の Update で Delay 7 が始まり、7 s ごとに PlayFromStart（約 19250 cm 進んで先頭へ）。car_trap2 は x 8978 → −10272（y −24239、z −245）、car_trap_2 は x −10190 → 9060（y −18959）。色は MultiGate（ランダム・ループなし）で Car1 / Car3 / Car4 / Car2（4 回で出尽くして以後変わらない）。
- プレイヤーが Box に触れると一度だけ `death()`・`Death_C`・2D の `RL_bodyfall_Dirt_M4_Close_Stereo_Hard_Impact_10` と `Lucky_Punch_2`。マネキンは `Death_by_car()`・`manequin_stun`。Box1 にプレイヤーが入るとクラクション（3 s のクールダウン）。checkpoint 4 では BeginPlay の 0.1 s 後に Timeline を止め、車は地図の外 (−11522, −24239) / (10310, −18959) に止まる。

### 街灯 `Lamp_zone_1`（48）とレトロな灯 `Light_retro_breaking`（7）
- 街灯: `Parking_Lamp` ×2（z 84）、RectLight 3.2 cd・15 × 150・BarnDoor 70°・減衰 900・MaxDraw 5500・Volumetric 0.5・回転 (P −90, Y −56.31, R −33.69)。プレイヤーの整数 `Player See By Monster` をマネキンが追跡の開始で +1（`levelRun_manequin_zone_1()`: shard ≠ 0 なら全 48 灯に `Activate_lamP-break()`）、諦めて −1（`levelWalk_manequin_zone_1()`、全灯に `DeActivate_lamP-break()`）。
- 点滅: Timeline_0 を PlayFromStart（0.4 s のループ、cubic、キー 0: 3.2（接線 0）・0.1: 1.0（59.33）・0.2: 3.2（−27.20）・0.3: 1.0（20.69）・0.4: 3.2（−54.72））。回復: 値が 0 なら Timeline_0 を止め Timeline_1 を Play（2 s、キー 0: 3.2（−8.44）・0.4: 0（0.30）・2.0: 3.2（3.96）。2 回目以降は終端の 3.2 のまま、と推定）。shard == 0 では点滅しない（`Deacticate_after_complete`）。
- `Escape_from_zone1`: RectLight を隠し、材質スロット 1 を `Parking_Lamp_Albedo_Mat_Inst` に。`Escape_from_zone2`: 同じく消して 1.5 s 後に表示、スロット 1 を `Parking_Lamp_Albedo_Mat`、影なし、Timeline_2 で色を巡回（3 s のループ、線形、キー 0 / 1 / 1.993 / 2.993 s、R 0.87 → 0.381 → 0.33 → 0.871・G 0.149 → 1 → 0.49 → 0.15・B 0.149 → 0.55 → 1 → 0.15）。
- レトロな灯: 点光源 0.4 cd・A2EFFF・減衰 230・SourceLength 315、メッシュ Cube（拡縮 (0.1, 0.1, 2.9)、Roll −90。点灯 `M_LightStage_Arrows`、消灯 White）。全基同じ位相で 1.85 s 周期: 消 0.2・点 0.45・消 0.1・点 0.1・消 0.1・点 0.2・消 0.2・点 0.5 s。

### 扉
- `Door_zone_1_Locked`（121）: `Door_closed()` → 一度だけ `Locked_Door_v1`、4 s 後にまた反応。開かない。
- `Door_zone_2`（6。(5340, −22921)、(512, −6211)、(4472, −23793)、(−361, −7078)、(−6534, −20282)、(6428, −22921)、z −331）: Box（(−335, 0, 65)、半分 320 × 357 × 32）で `right` = false、Box1（+315 側）で true。`Door_Open()` は一度だけ・0.7 s 後にまた反応: 閉まっていれば `right` なら Open_Left、それ以外 Open_Right と `Open_door`、開いていれば逆再生と `Closed_door`。ヒンジ（Sphere）の Yaw を ∓91（0.35 s で ±91、0.425 s で ±93、0.512 s で ±91、cubic）。
- `Vent_door_BP`（13、z −218）: `Open_door_vent()` は一度だけ・交互に開閉（開: Pitch 0 → 120、線形 0.667 s、ポーンの衝突を無視。閉: 逆再生、0.6 s 後に遮る）、どちらも `vent1_Cue`、0.75 s 後にまた反応。
- `Blocker_Bp`（25、z −340、拡縮 (−1, 1, 1)）: `Activate_Blocker` は一度だけ: シーケンス 2.754 s（ヒンジの Pitch 0 → −15（0.096 s、線形）→ +4（0.4 s）→ −75（2.75 s、cubic））、不可視の壁 Cube（255.6 × 11 × 100）を破棄、`Blocker_Sound`（600/2500）。`Disaster_destroy` はアクターを破棄（Mannequin_Escape のトレースから）。
- `Big_door_zone_1_BP`（20）: `Open()` は `World_change`（Big_door_zone_1_BP23 だけ true）なら何もせず、それ以外は一度だけ `Garage_Button_SFX` と `GarageDoor_open_SFX`、2 s 後にまた反応。開: シャッターの Z 0 → 227.5（0.317 s）→ 239.1（0.808 s）→ 350（1.275 s）→ 360（1.671 s）、衝突を切る。閉: 215 → 0（0.313 s）→ 3（0.946 s）、衝突を入れる。`Opendoor()`（マネキンが使う）は必ず開ける。マネキンは 1 s ごとに 300 cm 以内の閉じた扉に `Opendoor()`（2 s のクールダウン）。`Disaster_destroy`（一度だけ）: シャッターを破棄して GeometryCollection（×1.1）、Box の位置に RadialImpulse（半径 200、強さ 2500）と AddImpulse (10000, 10000, 10000)、`Smoke_Exp` と `Gazel_2_Crash_1_Cue`（1.7）、2 s 後に Dither 1 → 0（1 s）、さらに 1 s 後に破棄。壊すもの: トラックのトレース、Mannequin_Escape のトレース、プレイヤーのダッシュ中（`Dash_stun`）の Box1 接触（(0, 2, 262)、半分 234 × 10 × 219。閉まっていて `World_change` が偽のとき）。

#### 扉の部品・支点・当たり（2026-09-14 の 2 回目の調査。上と食い違う点はこちらが正しい）
局所はアクターの root（`DefaultSceneRoot`、原点）の座標（cm）。シーケンスのチャンネルは Rotation[0] = Roll（X）・[1] = Pitch（Y）・[2] = Yaw（Z）で値は部品の相対変換そのもの、補間 0 = 線形・2 = 三次。配置のシーケンスの写しのキーは BP と同じ。
- **interact**（ThirdPersonCharacter `interact()`、`.cpp:12810–12823`）: 左クリックを押した瞬間（`InpActEvt_LeftMouseButton_…_38`）とパッドの FaceButton_Left。E ではない（E は `ability2`、F はロックピック）。FollowCamera から前方 500 cm・半径 5 の `SphereTraceSingle`（Camera チャンネル）で、距離が 25 cm 以下か 200 cm 以上なら何もしない。当たったアクターを直接キャスト: `Door_zone_1_Locked` → `Door_closed()`、`Vent_door_BP` → `Open_door_vent()`、`Big_door_zone_1_BP` は当たった部品が `Door_Metal_zone_1`（脇のボタン）のときだけ `Open()`、`Blocker_Bp` → `Activate_Blocker()`、`Door_zone_2` → `Door_Open()`。これらを呼ぶのはプレイヤーだけ（`Opendoor` はマネキンだけ）。`Timeline_29` から周期的に呼ばれる 2 つ目の interact は照準の下の物の確認（案内の表示と推定）。`Door_TRAP` の `Door_closed()` はどこからも呼ばれない。
- **マネキンは閉じた扉に遮られない**: `Mannequin_BP` の CollisionCylinder（半径 30）は PhysicsBody で WorldStatic を無視し、扉はどれも WorldStatic。移動はナビメッシュの `MoveTo`（ナビに影響しないのは Door_zone_2・罠の扉・Blocker の板・シャッター。Locked の扉・ベントの蓋と Cube・Blocker の柱と壁はナビを削る）。プレイヤー（半径 45・半高 103）は閉じた扉 6 種すべてに遮られる。
- `Door_zone_1_Locked`（配置名は Door_zone_2、_3 …）: `Modul_wall_Door` ×0.5（BlockAll）と音。当たり 中心 (−7.5, 0, 122.5)・半辺 (17.1, 70.1, 122.6)。
- `Door_zone_2`（配置名は Door_zone_1、_47、_113、_114、_116、_118）: `Sphere`（(−7, 70, 0)、×0.1）→ `Modul_wall_Door`（実質 root の原点・×0.5。WorldStatic と Destructible だけ無視）。動くのは `Sphere` の Yaw: 支点 局所 (−7, 70, 0)・軸 局所 Z。`Open_Left` 0 → −91（0.35 s）→ −93（0.425）→ −91（0.5125）、`Open_Right` 0 → 91（**0.375 s**）→ 93（0.425）→ 91（0.5125）、三次、終わり 0.5167 s。閉じた当たりは Locked と同じ箱、開いても板の当たりは残る（−91 で x −147.3..−6.6・y 53.4..90、+91 で x −7.3..133.3）。
- `Vent_door_BP`（z −218）: `Vent_door` ×2.2（BlockAll）、`Cube`（(0, 0, −51)、拡縮 (0.03125, 1.09375, 1.09375)、非表示）。動くのは `Vent_door` の Pitch: 支点 root の原点（蓋の上端）・軸 局所 Y、0 → 120 線形 0.6667 s（終わり 0.675）。当たり 閉 中心 (0.1, 0, −53.8)・半辺 (2.3, 56.6, 56.6)、開 x −3.6..96.7・z −3.3..57.2（+X へ跳ね上がる）。開けると蓋が Pawn を無視、閉じると 0.6 s 後に遮る。`Cube`（半辺 (1.6, 54.7, 54.7)）は BeginPlay の 0.1 s 後に衝突を切り、プレイヤーのしゃがみの後（`Crawling_is?()`、`.cpp:7022`）に 1 s だけ戻る。
- `Blocker_Bp`（z −340、root ×(−1, 1, 1)）: 柱 `Blocker_Cube`（×0.35、Pawn を遮る）、`Sphere`（(0, −35, 106)、×0.34）→ 板 `Blocker_Cube_001`（(0, 100, −302.857)）、壁 `Cube`、音。動くのは `Sphere` の Pitch: 支点 局所 (0, −35, 106)・軸 局所 Y（最初の区間は線形、残りは三次、2.754 s）。当たり（鏡映前の局所）: 板 閉 中心 (−119, −37.5, 106.4)・半辺 (136, 2.6, 8.5)、−75° で x −73.9..13・z 87.4..354.7。柱 中心 (0, −3.5, 70)・半辺 (35, 38.5, 70)。壁 中心 (−129, −37, 55)・半辺 (127.8, 5.5, 50)（`Activate_Blocker` で破棄）。root の拡縮 (−1, 1, 1) は局所の x と Pitch の符号を反転したのと同じ。
- `Big_door_zone_1_BP`: シャッター `Big_door_zone_1`（×2.2、Pawn を遮る）、ボタン `Door_Metal_zone_1`（(307, 0, 107)、×2.2）、`Box`（(0, 0, 73)、半辺 (256, 128, 96)）。動くのは `Big_door_zone_1` の相対 Z（開 1.675 s、閉は 215 から 3 まで 0.975 s）、開けると同時に衝突を切り閉じると入れる。当たり シャッター 閉 中心 (0, 0.9, 245.1)・半辺 (222.7, 7.3, 215.9)、開 z 389..821。ボタン 中心 (307, 10.5, 135.6)・半辺 (19.7, 11.5, 33.9)。
- `Door_TRAP`: 動くのは `Modul_wall_Door`（支点 root の原点 = 扉の下端の中央）: T(X, 0, Z)·R(Pitch, Yaw, 0)·S(0.5)。当たり 閉 扉は Locked と同じ箱、`Cube` 中心 (−7, 0, 69)・半辺 (0.54, 66.4, 96.9)。倒れた後の扉は x −313.5..−47.8・y −111.5..69・z −10.5..23.6（床の上の段差として残る）。

### 脱出フェーズ（checkpoint 4）
- レベル BP の BeginPlay の 0.1 s 後: 入力を切り 0.8 s → `Truck_boss`（`NewVar_0` = 8、Spline_3）→ 入力を戻す → 0.2 s 後に `Mannequin_Escape` 14 体（z −247、yaw 90、x −441〜−1146、y −31586〜−32696）→ `Escape_zone_1_Blueprint` を (−809, −29645, −334) に → 0.1 s ごとに `Gazelle_priority` を見るループ → `barrierspeedbust6` を破棄。`Gazelle_priority` の値ごとに一度だけトラック: 1 = Spline_1（2.5 s）、2 = Spline_4（2 s、標識のデカールを Z 反転）、3 = Spline_5（7 s）、4 = Spline_6（2 s、標識のデカールを Z 反転）。
- スプライン（z −334、始点 → 終点、長さ、平均速さ、終点の `Gazelle_boom_point`）: Spline_3 (−774, −32486) → (−845, −24130)（5 点、8366、1046 cm/s、(−837, −23745)）、Spline_1 (−2919, −20283) → (−484, −21397)（3243、1297、(−224, −21282)）、Spline_4 (5767, −21627) → (3026, −21601)（2741、1370、(2883, −21593)）、Spline_5 (7233, −18967) → (−3862, −18727)（11115、1588、(−4022, −18665)）、Spline_6 (−1933, −13806) → (−2312, −15459)（1726、863、(−2428, −15633)）。
- `Truck_boss`: 車輪のループ（Roll −360 を 0.396 s）、Timeline（0 → 1 を 1 s、PlayRate 1/`NewVar_0`）で距離 = f(t) × 全長（f(s) = −0.78s³ + 1.56s² + 0.22s）の位置と回転。0.01 s ごとに Box1（≈ (330, 40, 93)）から前方 150 cm・半径 50 の球トレース: Big door なら `Disaster_destroy`、`Gazelle_boom_point`（不可視の Cube 153 × 153 × 506）なら `Stop_bus`（揺れのシーケンス 0.78 s、`Exp_fire`、`Gazel_boom`、車輪とエンジン音を止め、Box3 の衝突を切る。トラックは残る）。`Gazel_2_Beep` を 1 回、`Exp_fire` に `Car_SFX_N_mute_2_Cue`（2 s でフェードイン）。Box3（(−6, 40, 160)、半分 336 × 118 × 171）にプレイヤーが触れると一度だけ `death()`。
- `Escape_zone_1_Blueprint` の箱（z −334。Box10〜13 は半分 858 × 1225 × 267、Box14〜17 は 858 × 654 × 267。プレイヤーに一度だけ）: Box10 (−784, −22335) 優先度 1、Box11 (4569, −21161) 2、Box12 (3817, −18421) 3、Box13 (−2149, −14228) 4、Box14 (−2873, −22742) 何もしない、Box15 (296, −19080) Mannequin_Escape 3 体（x 1553、y −17493 / −17646 / −17811、z −280、yaw 90）、Box16 (−802, −7004) 2 体（(−883, −6027)、(−718, −5882)）、Box17 (743, −5451) `Manequin_Boss_fight_bp` を (966, −5636, 410) と (966, −5836, 410) に yaw 180 で `Activate_fall()`。不可視の Cube の壁 10 枚（90.6 × 559.4 × 540.6、z 187.7）、点光源 9（1.2 cd、D8EAFF、250）。
- `Mannequin_Escape`: 0.25 s 後に `MaxWalkSpeed` 700 か 650（650 だけ走りのアニメ）、2 s 後から 0.01 s ごとにプレイヤーへ `MoveTo`（許容 5）と、前方 150 cm・半径 50 の球トレースで Big door と Blocker の `Disaster_destroy`。GravityScale 4、カプセルの半径 22.5。Sphere（半径 116）に触れると一度だけ `death()`。
- `TriggerBox_1`（(−269, −7063, −269)、半分 (67.8, 187.5, 66.1)）: 入ると一度だけ全 `Mannequin_Escape` を破棄（ボス戦のマネキンは残る）。
- `Manequin_Boss_fight_bp`: 300 cm 以内で攻撃（`MaxWalkSpeed` 0、ATTACKING 0.8 s、0.2 s 後のトレースで `Blood_2` の `Hit_to_player` と Hurt の音、2 s のクールダウン）、平時 80 cm/s で `MoveTo`、3〜4 s ごとに 2 s ガード、`Hit_point` ≥ 4 で倒れて消える。`death()` は呼ばない。

#### 脱出フェーズ・トラック・車・噴出の詳細（2026-09-14 の 2 回目の調査。上と食い違う点はこちらが正しい）
生バイトコードで照合（レベル BP の .cpp 380 行の `goto Label_10220` は実際は 7523 の POP（終了））。
- **レベル BP の checkpoint 4**（`ThirdPersonBP/Maps/Chaotic_Customer_Zone_1.cpp` 204–378・648–877）: BeginPlay → Delay 0.1 → Delay 0（1 フレーム）→ `checkpoint == 4` → DisableInput → Delay 0.8 → Sequence: 0 `Truck_boss` を (0, 0, 0) に遅延スポーン（`NewVar_0` 8、Spline_3）、1 EnableInput、2 Delay 0.2 の後 `Mannequin_Escape` **15 体**（z −247・Yaw 90、順に (−997, −31850)、(−1054, −31598)、(−1146, −31919)、(−1086, −32084)、(−1043, −32247)、(−1118, −32395)、(−1005, −32635)、(−806, −32696)、(−636, −32635)、(−484, −32386)、(−565, −32212)、(−441, −32084)、(−457, −31909)、(−568, −31850)、(−564, −31586)）、3 `Escape_zone_1_Blueprint` を (−809, −29645, −334)（トラックと同じフレーム）、4 0.1 s ごとに `Gazelle_priority` を見て値ごとに DoOnce でトラック（1: Spline_1・2.5、2: Spline_4・2 とデカールの相対拡縮を (0.009, 0.07, −0.07)、3: Spline_5・7、4: Spline_6・2。消されないので最大 5 台）、5 `barrierspeedbust6` を破棄。
- **Escape_zone の箱**（プレイヤーだけに DoOnce）: Box10〜13 = priority 1〜4、Box14 何もしない、Box15 `Mannequin_Escape` 3 体 (1553, −17493 / −17646 / −17811, −280) Yaw 90、Box16 2 体 (−883, −6027, −280)・(−718, −5882, −280) Yaw 90、Box17 `Manequin_Boss_fight_bp` 2 体 (966, −5636, 410)・(966, −5836, 410) Yaw 180 と直後の `Activate_fall()`。ほかに飾りのマネキン 30 体（Box〜Box9 に `Manequin__1_` を 3 体ずつ、`Manikenstay_Anim`。組の中心 (−1528, −29532)、(−82, −29532, 180°)、(−1486, −26880)、(−160, −26880, 180°)、(−196, −24243, 180°)、(−4118, −24240)、(−152, −22914, 180°)、(−796, −20951, −90°)、(1842, −18340, −90°)、(−4105, −18963)）、不可視の壁 Cube〜Cube9（90.6 × 559.4 × 540.6、中心 z −146.3、WorldDynamic だけ無視、ナビに影響しない。中心 (−1416, −29540)、(−173, −29540)、(−173, −26859)、(−1459, −26859)、(−216, −24282)、(−192, −22906)、(−4092, −24249)、(−767, −20974, −90°)、(1849, −18364, −90°)、(−4104, −18958)）、点光源 9（1.2 cd、D8EAFF、減衰 250、影なし、z −174: (−792, −29532)、(−1618, −24245)、(−288, −21658)、(4435, −21378)、(4435, −20389)、(4435, −19022)、(152, −19022)、(−3261, −18807)、(−1630, −14891)）。
- **スプライン**（`Spline_2_C`、閉じない、z −334、Rotation は Z 回転だけ。点のワールド座標・接線 T（到着 = 出発）・U = User / A = Auto、弧長）: Spline_1 (−2919, −20283) T(1027.2, 77.5) / (−2056.4, −21254.1) T(562.1, −2386.0) / (−484.4, −21396.6) T(493.6, 223.7) すべて U、3242.9。Spline_3 (−774, −32486) T(−3.1, 413.2) U / (−777.1, −30042.8) T(−52.5, 2326.8) A / (−879.0, −27832.4) T(204.6, 2305.8) U / (−780.7, −25431.1) T(17.1, 1851.3) A / (−844.7, −24129.8) T(−64.0, 1301.3) A、**8366.0**。Spline_4 (5766.7, −21627.4) T(−1354.8, 19.0) / (4411.8, −21608.5) T(−1370.4, 13.0) / (3025.9, −21601.5) T(−1386.0, 7.0) すべて A、2740.9。Spline_5 (7233.2, −18967.4) T(−2823.2, 143.1) U / (3228.6, −18941.4) T(−4461.6, 0.5) A / (−1690.0, −18966.5) T(−3545.3, 107.0) A / (−3862.0, −18727.5) T(−578.5, 279.0) U、11115.4。Spline_6 (−1932.9, −13805.5) T(−119.4, −224.1) / (−2124.3, −14515.2) T(−120.0, −1507.9) / (−2311.6, −15459.3) T(−709.5, −1007.1) すべて U、1725.9。`Gazelle_boom_point`（z −335。Cube 1.53 × 1.53 × 5.06 倍、非表示、トレースだけ遮る）: Spline_3 → 5_14 (−837, −23745)、Spline_1 → 4_11 (−224, −21282)、Spline_4 → 3_8 (2883, −21593)、Spline_5 → 2 (−4022, −18665)、Spline_6 → _2 (−2428, −15633)。
- **Truck_boss**（+X が前）: 車体 `Gazelle_Gazelle`（ワールドで ×1.125）、前輪 `_001`（アクターの (216.9, 0, 41.4)）、後輪 `_002`（(−135, 0, 41.4)）。材質は 3 つとも `Gazelle_DefaultMaterial_BaseColor_Mat`。常時点く `Fire_gazelle`（Niagara）と PointLight（2500 unitless、FFB69B、半径 500）が (271.9, 41, 164.4)。デカール `Way_sign_Mat`（実寸の半分 ≈ (1.65, 92, 149.9)、中心 (−135, −74.3 / 145.1, 209.3)、±Y へ投影。priority 2・4 の Z 反転は標識の前後の鏡像）。車輪: Roll 0 → −360 を 0.3958 s、再生範囲 **0.4125 s** のループ。止まる揺れ（ActorSequence1、0.779 s）: Box2 の Z 34 → 50.12（0.254 s、線形）→ 34（0.483、三次）、Pitch 0 → −10（0.15、線形）→ −2（0.417、三次）→ 0（0.767、線形）。Timeline_0: 長さ 1、キー (0, 接線 0.22016656)・(1, 接線 1) の三次 → f(s) = −0.77983s³ + 1.55967s² + 0.22017s、PlayRate 1/`NewVar_0`。Update: 距離 = f × スプライン長で `GetLocation/RotationAtDistanceAlongSpline`（World）→ `K2_SetActorTransform`。BeginPlay → Delay 0.1 → 車輪のループ・Play・トレースのループ・`Gazel_2_Beep`（`Beep_beep1_Cue`）・`Car_SFX_N_mute_2_Cue` を `Exp_fire` に FadeIn 2 s。トレースは Box1（アクターの (329.6, 39.7, 92.6)）から前方 150 cm・半径 50。**Stop（DoOnce）は揺れ・`Exp_fire`・`Gazel_boom`・車輪を止め・Box3 の衝突を切り・エンジン音を止めるが、Timeline_0 は止めない**（トラックは終点まで進んで止まる）。
- **car_trap**: Car_Car のスロット 0 は `Car_02`（固定）、スロット 1 が既定 `Car1`、車輪は `Car_03`。MultiGate の出力 0〜3 がスロット 1 に `Content_game/models/Car_1/Car1`（Car_Gray_BaseColor）・`Car3`（Blue）・`Car4`（Black）・`Car2`（Red）。Timeline は自動再生しない: BeginPlay → Delay 0.1 → checkpoint 4 なら `Stop()` だけ（Box は配置値の y 20500 のまま = 地図の外、車輪も回らない）、それ以外は PlayFromStart（その場で Update が 1 回、と推定）→ 色 → 車輪（Sphere1・2 の Roll 0 → 360、0.375 s のループ）。つまり最初の 0.1 s はどの checkpoint でも y 20500。
- **Pair_trap**（マップの 6 基は `ThirdPersonBP/Blueprints/Pair_trap_C`）: 根（Niagara と音、z −279）は Box の中心から局所 Z に 143 cm。局所 X がワールドの上、局所 Z が水平（2・4・6 は +Y、3・5 は −Y、_2 は −X）。致死の Box は根から局所 Z に 1〜285 cm・上下 ±65・横 ±66、Box1 は横 ±434。Niagara `Pair_trap`（エミッタ "Fountain"、CPU、ワールド空間、スプライト `Smoke_2`、FaceCamera、`Scale Alpha` の LUT 0.1 → 0）: 水平の噴流で、長さはおよそ Box の 285 cm（速度・寿命・大きさは VM の定数に焼き込まれていて読めない）。根の位置: 2 (−1449, −20453)、3 (−133, −20109)、4 (4846, −23064)、5 (4032, −23046)、6 (−7035, −20420)、_2 (178, −6876)。
- **Mannequin_Escape**: カプセル半径 22.474（半高は既定 88 と推定）、見えるメッシュ `Manequin__1_`、Sphere 半径 116、GravityScale 4。カプセルは PhysicsBody で WorldStatic だけ無視: 脱出の不可視の壁（WorldDynamic と推定）には遮られる。graph に 750・950・850・800 の速さもあり、「700 か 650」は再確認が要る。

### マネキン `Mannequin_BP`
- CharMoveComp: 既定 MaxWalkSpeed 100、MaxAcceleration 1000、RotationRate yaw 180。当たりの Capsule ×3（半径 66、半高さ 132）。BeginPlay の 0.1 s 後にスキンを 4 種からランダム、1 s ごとの Big door（300 cm）、徘徊。
- 徘徊: `GetRandomReachablePointInRadius(8000)` へ 108 cm/s で `MoveTo`、RandomFloatInRange(RandomFloatInRange(10, 15), RandomFloatInRange(20, 25)) s ＋ 0.1 s ごとに次の目的地。
- 追跡: `eye_revealed` 中に OnSeePawn のたびに徘徊を閉じ、プレイヤーの位置へ `MoveTo`（許容 0）、`MaxWalkSpeed = clamp(距離, 810, 1100)`。距離 < 4500 なら一度だけ `Player See By Monster` +1・走りの BGM・全灯の点滅。
- 攻撃: 追跡中に RandomFloatInRange(RandomFloatInRange(4, 6), RandomFloatInRange(8, 10)) s ごと、カプセルからプレイヤーへの線トレースが何にも当たらなければ、速さ 0 → 1.4 s 後に移動を止め → 1.7〜2.0 s の間、毎フレーム前方 66 cm へ sweep 付きで瞬間移動（飛びかかり）→ 3 s 後に次の攻撃。
- 諦める: `MoveTo` の終わりに距離 ≥ 4500 なら徘徊へ（−1、歩きの BGM、全灯に DeActivate）。
- 即死: Capsule がプレイヤーの CapsuleComponent に重なると、プレイヤーが `Dash_stun` 中ならマネキンが 10 s 気絶（衝突と移動を切り、`eye_revealed` を解除）、それ以外は一度だけ `Screamer_slide`（カメラを 0.25 s でマネキンへ）と `death()`（叫びのアニメ 2 種からランダム）。
- 死亡（`Death_by_car` など）: 1 s 後に配置の番号 i ごとに `RandomInteger(11)` を引き直し、一致した `Respawn_manequin` の `Spawn_new_manequin()`（該当なしの確率 ≈ 46.7 %）。3 s 後に溶けて消え（1 s）、1 s 後に破棄。`Respawn_manequin`（8、z −335: (−1766, −30846)、(1604, −32497)、(−1168, −8146)、(281, −13320)、(−1767, −17427)、(−1568, −21189)、(−1904, −27240)、(4834, −29304)）は 2 s 後に自分の位置 +45 z に倒れた状態で出し、さらに 2 s 後に動き出す。

### 駅の柵 `Fence_battle` と、殴る・しゃがみ・スライディング（2026-09-14 の 3 回目の調査。`.cpp` と uasset から）
開始地点のホームにある木の柵。静的な物ではなく、プレイヤーが殴って壊す。ホームの床に置かれた案内の文字（ファンゲームのチュートリアル）が、柵を殴る → しゃがむ → ブーストしながらしゃがんで（スライディング）戸口の板の下をくぐり速度障壁を割る、の順を教える。
- 配置（z はどれも −2371）: `Fence_battle_C` の `Fence_battle_2`（(−3395, −5588) yaw −180。案内の文字と重なり箱を持つ）、`Fence_battle_2_C` の `_4`（(−5657, −5891) yaw 90）・`_5`（(−6912, −5739) yaw 90）・`_6`（(−2603, −5200) yaw −180）・`_8`（(−2603, −5989) yaw −180）。
- 部品: 区画が 2 つ。見えない `Cube`（局所 (0, −3, 59)、拡縮 (1, 3.25, 4.03125)）と `Cube1`（(0, −343, 59)、(1, 3.28125, 4.03125)）が当たり（Custom、Destructible だけ無視）。板はメッシュ `Fence2` で、Cube の組が Fence2・7・8・9、Cube1 の組が Fence4・5・6・10（局所の位置・Roll・拡縮は各 uasset。`Fence_battle_2_C` は拡縮 1 で間隔が広い）。`Fence3` は NoCollision の飾り。板は PhysicsBody（Camera と Visibility を無視）。
- `Hit_1`（Cube）/ `Hit_2`（Cube1）: Aim ウィジェットの `Percent ≥ 0.5` なら FlipFlop（Fence9、無ければ Fence8 ⇄ Fence7、無ければ Fence2）、そうでなければ MultiGate（順に Fence9|8、Fence7|2、Fence2、Fence8。Cube1 の FlipFlop は Fence5|6 ⇄ Fence4|10、MultiGate は Fence5|6、Fence6、Fence4|10、Fence10）の板を `SetSimulatePhysics(true)`・Pawn を無視（落ちる）。そのたびに区画ごとの数を + 1、4 以上で一度だけ区画の当たりを切る。`Wood_Break2_Cue` を区画の位置で鳴らす。
- 案内の文字（`Fence_battle_C` だけ。`Tutorial_text_C`、フォント `beer_money_Font` 30、ワールドの WidgetComponent を床に寝かせる（Pitch 90）、DrawSize 1920 × 1080）: BeginPlay の 0.2 s 後に言語で文字を入れる。Widget（局所 x 167）「USE "Tab" TO CHANGE CURRENT ITEM.」、Widget1（x −921）「USE "CTRL"」、Widget2（x −1691）「USE "SPEED BOOST" + "CTRL"」（z −59）。重なり箱 Box（局所 x −1566、拡縮 (13.9375, 8.25, 3.15625)、y −126）でプレイヤーが入ると一度だけ Widget1 の、Box1（x −2400）で Widget2 の `NewAnimation`（RenderOpacity の Float トラック）を再生。
- Tab（`checkpoint ≥ 1` のとき）: FlipFlop で `Item_nomber_slot` を 1（タブレットを隠し、手を上げるシーケンス `Hands_up` を等速で、`Doug_hands_run` を表示）⇄ 0（手を 2 倍速で逆再生し、0.2 s 後にタブレット）。
- 殴る（左クリック、slot 1 のとき）: 押すと `Hands_up_anim` = 1、`RetriggerableDelay(0.5)` の後 0 でなければ 2（構え）。離したとき 2 なら 3 にし、Aim を `Reverse`、`Delay(0.16)` → `interact_2_push`（FollowCamera の位置から前へ 270 cm、半径 `Push_Radius` 40 の SphereTraceSingle）で当たった物: `Mannequin_BP` なら `Death_by_punch`、`Statue_BP` なら `Damage`、`Fence_battle_2_C` / `Fence_battle_C` の Cube なら `Hit_1`・Cube1 なら `Hit_2`、ボス戦のマネキンなら `Manequin_bruh`。カメラシェイク `Player_shake/Hit`。`Delay(0.4)` の後 `Hands_up_anim` = 0。離したとき 2 でなければ 0（殴らない）。
- Aim ウィジェット（`Content_game/Widgets/Aim`、画面中央の照準 4 枚 `Aim_m` と ProgressBar `Progress_bar_aim_Imgg`）: 有効な間、Tick ごとに `Percent` に FPS の区分の量を足す（≤30 fps: 0.011、≤60: 0.0082、≤120: 0.0032、≤240: 0.0022、≤360: 0.0014。1 を超えると止まる）。60 fps で 0.5 s の構えは 0.246。
- マネキンの `Death_by_punch`: Aim の `Percent ≥ 0.25` なら死亡（移動を止めカプセルの当たりを切る。`Death_by_car` と同じく死亡の流れ）、未満なら `Hit_points_mini` + 1 で、2 になると死亡。
- しゃがみ（CTRL）: `Cant_up` なら何もしない。ブースト中（`speed_bust_usanulsa_i_pomenalsya_field` かつ `MaxWalkSpeed > 500`）なら一度だけ `Sit_in_boost`（スライディング）: Timeline_21（カプセルの半高 103 → 45、`CurveFloat_12_13`、0.15 s）と Timeline_22（CameraBoom の z 61 → 30、0.15 s）。そうでなければ FlipFlop: しゃがむ（`Sit_or_not`、Timeline_5 の半高 103 → 45（0.35 s、三次）、Timeline_6 の CameraBoom z 61 → 30（0.35 s、線形）、Timeline_23 −70 → 0（0.35 s）、シェイク `Sit_down_anim`、`MaxWalkSpeed` 200）⇄ 立つ（逆再生、シェイク、300）。2D の `Crouch`。`Crouch_branch`（カメラから真上へ 100 cm、半径 5 の SphereTrace）が StaticMeshActor に当たれば `Cant_up`（立てない）。スライディングの終わりは Timeline_21・22・24 の逆再生と Delay(3)（経路の細部は未確認）。
- プレイヤーのカプセルは半径 45・半高 103（しゃがむと半高 45 で高さ 90 cm）。

### 確定できなかったこと
扉を操作する interact のキーの経路、2000 cm 以内の閉じた Big door をすべて開ける処理（`things_trayska_telepathy` の後）のきっかけ、ブーストの発動条件と 22 s 後の処理、`Escape_from_zone1` のもう 1 つの呼び出し元、checkpoint 4 の街灯の最後の状態、街灯の Timeline_1 の 2 回目以降、`barrier_C` の爆発と音と `not_yet` の音の資産（書き出しで null）、`Pair_trap` の噴射の向き（Niagara）、円環の材質の計算、ボス戦マネキンの `Activate_fall` とダメージ、周辺視野角、クラクションの Box1 のワールドの大きさ、`Interactivate_barrier` と `barrier_C` の灯の単位。

## 音（2026-09-14 の調査。SoundCue・SoundWave・コンポーネントのプロパティとバイトコードから）
- 音量 = 波形の `Volume` × キューの `VolumeMultiplier` × コンポーネントの `VolumeMultiplier` × フェード。波形の `Pitch` は wav に焼かれていない（再生側で掛ける）。34 のキューはどれも `SoundNodeRandom`（重み 1）だけで、Modulator・Looping・Concurrency・減衰アセットのノードは無い。SoundAttenuation / SoundConcurrency のアセットも無く、減衰は常にインライン（球。省いた項目は UE の既定: 内側 400・減衰 3600・Linear）。減衰の無い音は `SpawnSoundAtLocation` でも 2D。
- 以下「内側/減衰（cm）、曲線」。NS = NaturalSound。
- **曲**: 探索中（`trigger2` まで）は無音（マップに AmbientSound は無い）。`music_manequins_start`（`trigger` の重なり `trigger.cpp:131`、チェックポイント 2 の BeginPlay `:173`）: `Player See By Monster` = 0、0.6 s 待って Gate 15 を開け、以後 Tick: 0 なら `CC_Carol_Of_the_bells`（FadeIn 2 s、Dethsmass を 1 s で消す）、0 より大きければ `CC_Merry_Dethsmass`（FadeIn 0.5 s、Carol を 0.5 s で消す）。切り替えるたびに新しいコンポーネント（曲の頭から）。どちらも 2D・ループ・波形 0.7・SoundClass `Soundtrack`。全回収（Zone_1 で `shard == 0`、`ThirdPersonCharacter.cpp:2763`）で `stop_music_maneqiuns` と同じく両方を 2 s で消し、以後 `levelRun` / `levelWalk` はすぐ戻る（無音）。脱出は上のとおり。Merry_horrors は `Timeline_8` が終わって 3 s 後に 3 s で消える（`Timeline_8` が何かは不明）。
- **置かれた音**: 障壁（`Interactivate_barrier` 7・`barrierspeedbust` 4）の `Barrier_Loop`（600/1000 NS、ループ、BeginPlay から、割れると止める）。`car_trap` 2 の `Car_SFX_N_mute`（1000/5000 NS、ループ、自動で鳴る）。`Train_entrance_BP`（チェックポイント 0）の `TrainLoop`（FadeIn(3, 0.6)、×0.7、1000/7000 NS）と `CC_Intro`（1000/10000 NS）、`Doors_train` 12 の開閉（1000/8000 NS）。街灯とレトロな灯には音が無い。
- **出来事ごと**: 障壁の破砕 `Barrier_Shatter_Cue`（×1.5 × 波形 0.5、1000/3000 NS。`barrier_C` はコンポーネントの 550/3600 NS）、`barrier_C` の出現 `Stun_Wave_Attack_New_01`（550/3600 NS）。施錠の扉 `Locked_Door_v1`（600/2400 Linear、4 s の DoOnce）。開き戸 `Gym_Door_Open_v1..3` / `Gym_Door_Close_v1..3`（500/3600 NS）。通気口 `vent1` / `vent2`（減衰なし＝2D）。シャッターの `Open()` は `Garage_Button_SFX`（1000/5000 NS）と `GarageDoor_open_SFX`（1000/6000 NS）、壊れると `Gazel_2_Crash_1` / `Gazel_1_Crash`（×1.7、1000/9000 NS）。遮断機 `Blocker_Sound`（波形 2、600/2500 NS）。罠の扉は `18-Wood_Door_Breach_Destroy_FromOutside_V1`（CC2 のファイル、波形 0.6、600/3600 NS）。噴出 `PAR`（700/7000 NS）。車の `Car_horn`（1000/5000 **Linear**、3 s のクールダウン）、轢かれると 2D で `RL_bodyfall_Dirt_M4_Close_Stereo_Hard_Impact_10` + `Lucky_Punch_2`（`Truck_boss`・`Mannequin_Escape` も同じ対）。トラックはビープ `Beep_beep1` / `Beep_beep_2`（×2、1000/10000 NS、1 回）、エンジン `Car_SFX_N_mute_2`（波形 Pitch 0.6、1000/7000 NS、FadeIn 2 s、`Exp_fire` に付く）、`Stop_bus` で `Gazel_boom`（×2、1000/10000 NS）とエンジンを止める。配電盤の "Ele" で `Lamp_sounds` 2 つ（`broken_lamp_spark_1..3`、×2 × 波形 2、500/3600 NS）、1 s 後に動けなくなり、その 2 s 後に `Activate_escape_seq` と 2D の `DD_LVL2_06_Stage_Lights_1_102918`（2、Pitch 1.3）。シャードは 2D の `Soul_Shard_Pickup_v2/v3/v4`（MultiGate、0.4、ピッチ 1 / 1.05 / 0.95、FadeIn(0, 0.3) でさらに 0.3）。
- マネキンの音（本作は敵がワサミなので使わない）: 見つけた叫び `Scream_X_01..05`（×2、1000/6000 NS）、歩き `M_Walk_1..3`・走り `M_Run_S-1..3`（500/1500 NS、アニメの通知）、捕まえると `knife_stab01..04`（×1.1、2D）、倒れる `Mannequin_Fall_Impact`（波形 2.4、1000/4500 NS）。死亡時は `lifes_widget` の `Life_Lost`、ゲームオーバーは `66_-_Game_Over`（どちらも 2D）。
- 分からないもの: `barrier_C` の追加の破砕音と `not_yet` の音（アセットが null）、Sound の無い曲のコンポーネント（Zone_2・Dispatcher・PARKOVKA の後）、`Timeline_8`。
- wav は 206 本すべてある（16-bit PCM）。

## 使っていないもの・注意
- カットシーン（LevelSequence）の動画は無く、再現しない（ユーザーの回答: 駅のホームから始める）。
- UE5 の Lumen が使われたかは設定から確定できない（`DefaultEngine.ini` に明示が無い）。
