---
title: Zone 2 の Matron（ボスワサミ）
sources:
  - Content/Python/wasami_tools/pipeline/dd_boss.py
  - SourceArt/Wasami/boss_wasami.glb
  - Source/wasami_deception/WasamiMatron.h
  - Source/wasami_deception/WasamiMatron.cpp
  - Source/wasami_deception/WasamiBossAnimInstance.h
  - Source/wasami_deception/WasamiBossAnimInstance.cpp
  - Source/wasami_deception/Tests/WasamiMatronTests.cpp
updated: 2026-09-19
---

# Zone 2 の Matron（ボスワサミ）

## 役割
本家の Zone 2 の中ボス `BP_06_Matron_MiniBoss`（最新版 `pak_reference_2`）を、大きいボスワサミとして置く（作業一覧の項目 11）。**作っている途中**: いまあるのはボスワサミの素材の取り込みと、Matron のアクタ `AWasamiMatron`・ボスのアニメの再生 `UWasamiBossAnimInstance`・Matron の視界コーン 2 種（`WasamiViewcone.*`。07 記録）と、Zone 2 への配置（組み立ての `_flow`）・流れへの結び付け（`ActivateMinibossEnemies`）。PIE での確かめと通しはこれから（進捗記録 `20260919-matron`）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_wasami_boss()`（01 記録）→ `dd_boss.import_all()`。戻り値 `textures` 3 / `materials` 1 / `meshes` 1 / `animations` 3、`idle_head_cm`（Idle の最初のコマの頭の骨の高さ。拡縮 1 で 128.2 cm）。
- `dd_boss.prepare()`: 前処理した glb を書き、（役ごとの長さ、Idle の最初のコマの頭の骨の高さ〈m〉）を返す。`prepared_file()` がその場所。
- `AWasamiMatron`（`AActor`・`IWasamiViewconeInterface`）: `Activate()`（BlueprintCallable。Zone 2 の `Activate MiniBoss Enemies` が呼ぶ）、`IsActivated()`、`Switch()`、`LongCone`・`ShortCone`（`AWasamiViewcone*`、EditAnywhere。レベルの組み立てが入れる）、`bMode`・`bSpotted`（読み取り）、`GetMesh()`・`GetCloseArea()`。定数 `SwitchRate` 1 s、`ReinforcementTime` 1.0948 s、`MeshScale`（下の「大きさ」）。
- `UWasamiBossAnimInstance`: `bAlert`・`bSpotted`・`Location`（持ち主が Matron なら毎フレーム読む）、`PlayDetected()`（長さを返す）、`IsPlayingDetected()`、`GetMainClip(時刻, 重み)`・`GetLookAtAlpha()`（PIE での確かめ用）。状態 `FWasamiBossAnimState`（エンジンを使わない。テストの対象）、名前空間 `WasamiBossClip`（`Idle`・`Alert`・`Detected`）・`WasamiBossAnim`（`ClipNames`・`ClipPath`・本家の ABP の値）。

## 内部構造と処理の流れ

### 原本
`SourceArt/Wasami/boss_wasami.glb`（ユーザーのモデル。原本は git の外の `tmp/boss_wasami.glb`、2026-09-19 に写した。Git LFS）。Blender 4.5 の glTF で、作りは敵ワサミの `enemy_wasami_v3.glb` と同じ: シーンの根 `target_character` の子にメッシュ `output_unwrapped` と骨盤、スキン 1（骨 28、名前も同じ、`pelvis` が根、ルートの骨は無い）。キーの刻みも同じ（骨盤は 1/24 s と 2/30 s から、ほかの骨は 2/30 s から 30 fps）なので、敵の `dd_enemy._content_frames`・`_sample` がそのまま使える。メッシュ 95,427 頂点・100,128 三角形、高さ 1.70 m・幅 2.22 m（腕を広げた基準姿勢）、正面は glTF の +Z（UE では +Y）。材質 1（`BakedMaterial`）、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²。アニメ 7: `Long_Breathe_and_Look_Around`・`Alert`・`Lower_Weapon_Look_Raise`・`Walking_Scan_with_Sudden_Look_Back`・`Walking`・`Running`・`restpose`。

### 前処理（`prepare`）
- 役（`ROLES`。本家の `Matron_MiniBoss_AnimBP` が流すもの）: `Idle` = `Long_Breathe_and_Look_Around`（`loop`）、`Alert` = `Alert`（`closed`）、`Detected` = `Lower_Weapon_Look_Raise`（`once`）。各アニメを `dd_enemy._content_frames`（2/30 s から骨盤以外の骨の最後のキーまで）で 30 fps に標本化し直し、0 から並べる。
- ループの閉じ方は最初と最後のキーの差で決めた（2026-09-19）: `Long_Breathe_and_Look_Around` は最後のキーが最初の姿勢から 0.33° 外れ、最後の 1 コマの動きが 0.29° なので、敵の `loop` と同じく 1 コマ足りない形 → 最初のキーを最後の後に置く（`loop`）。`Alert` は最後のキーが最初の姿勢と 0.03° しか違わず、1 コマの動きが 4° 前後なので、既に閉じている → そのまま（`closed`。足すと 1 コマ止まる）。
- 使わないもの（`SKIPPED`）: `Walking`・`Running`・`Walking_Scan_with_Sudden_Look_Back`（本家の Matron は動かない）、`restpose`。役も `SKIPPED` にも無いアニメは、警告してそのままの名前で取り込む（敵と同じ）。
- メッシュの節とメッシュの名前を `SK_WasamiBoss` にし、`Intermediate/Pipeline/wasami/boss/WasamiBoss.glb` に書く（ファイル名をアセットの名前にしない理由は 07 記録の「取り込み」の 4）。
- Idle の最初のコマの `head` の骨の高さを測って返す（1.282 m。クリップの中で 1.243〜1.287 m。`head_end` 1.605 m、骨盤 0.619 m。敵ワサミの Idle より前かがみ）。Matron の大きさを決める材料（下の「原作データの根拠」）。

### 取り込み（`import_all`）
1. `dd_enemy._extract_textures(SOURCE, PREPARED_DIR, FOLDER, "T_WasamiBoss_")`: 敵と同じ設定で `T_WasamiBoss_<BaseColor|MetallicRoughness|Normal>`。
2. 材質: 敵のマスター `M_DD_WasamiGltf` を読み（無ければ `dd_enemy._build_master` で作る）、インスタンス `MI_WasamiBoss` にテクスチャ 3 枚を入れる。マスターは作り直さない（敵の取り込みのもの）。
3. `dd_enemy._import_model(instance, prepared_file(), FOLDER, MESH, "A_WasamiBoss_", 役)`: `PL_Wasami_Skeletal` で `/Game/Wasami/Boss` へ置き換えで取り込み、スロットにインスタンスを入れ、3 役のアニメがあるかを確かめる。インスタンスとメッシュとフォルダを保存する。

### Matron のアクタ（`AWasamiMatron`。本家の `BP_06_Matron_MiniBoss`）
- 部品: `DefaultSceneRoot` → `SkeletalMesh`（`SK_WasamiBoss` を `OnConstruction` でソフト参照から読む〈敵と同じ〉、`AnimClass` = `UWasamiBossAnimInstance`、拡縮 `MeshScale`、当たりなし・ナビに効かない〈本家の骨入りのメッシュの既定。コーンのトレースも通る〉。相対位置はレベルが置く）・`CloseArea`（`UBoxComponent`、Custom で Pawn だけ Overlap・ほか 7 つは Ignore、ナビに効かない〈本家の `AreaClass` は形の既定の NavArea_Obstacle で、重なるだけの箱はナビに入らない〉、箱は既定の 32。位置と拡縮はレベルが置く）。タグ `Enemy`（本家の CDO。敵を消す処理〈迷路の始まり・捕獲〉が Matron も消す）。`IWasamiEnemyInterface` は持たない（本家の Matron はナースでないので、オーブ・Primal Fear・Vanish・Telepathy は効かない）。本家の `ReceiveBeginPlay`・`Start Looking`・`Stop Looking` は空なので作らない。
- **`Activate`**: `Switch` を 1 s のループのタイマーに → コーン 2 つの `SetOwner(self)` → 2 つの `Initialize`（コーンの `bAutoOn` は偽なので、見始めても点かない）。
- **`Switch`**（1 s ごと）: `bMode` = プレイヤー（`GetPlayerCharacter(0)`）が `CloseArea` に重なっていない。`bMode` 真で DoOnce A（長いコーンの `TurnOn`・短いコーンの `TurnOff`・B を開く）、偽で DoOnce B（短いの `TurnOn`・長いの `TurnOff`・A を開く）。両方開いた状態から始まるので、最初の `Switch` で必ずどちらかが点き、その後は `bMode` が変わったときだけ切り替わる。コーンが消えた後も `bMode` は書き続ける（本家は消えたコーンへの呼び出しが空振りする）。
- **`Player Spotted`**（コーンが持ち主へ送る。DoOnce）: コーン 2 つを `Destroy` → アニメの `PlayDetected`（本家の `PlayMontage(DD_Matron_Zone_02_Detected_Montage)`）→ `bSpotted` 真。本家はモンタージュの `PlayMontageNotify`（1.0948 s）の `OnNotifyBegin` で `BP_06_ReaperNurse_Sentry` の全部に `Player Spotted` を送る。本作のクリップは長さが違うので、同じ秒数のタイマー（`ReinforcementTime`）で見張り（`AWasamiEnemySentry`）の全部に送る（見張りは跳び降りて追う。07 記録）。0.111 s の発見の声（`Matron_ReinforcementCall_01`）は作業一覧の項目 20 で鳴らす。

### アニメの再生（`UWasamiBossAnimInstance`。本家の `Matron_MiniBoss_AnimBP`）
敵の `UWasamiEnemyAnimInstance`（07 記録）と同じ 3 つの分け方: エンジンを使わない状態 `FWasamiBossAnimState` → ゲームスレッドの `NativeUpdateAnimation` が標本の一覧を作る → 代理 `FWasamiBossAnimInstanceProxy` の `Evaluate`（ワーカースレッド）が混ぜる。クリップの姿勢を混ぜる所は敵と共用（`WasamiEnemyAnim::BlendPoses`）。クリップはゲームのワールドのときだけ読む（エディタのレベルでは参照姿勢）。
- **毎フレーム読むもの**（本家のイベントグラフ）: 持ち主が `AWasamiMatron` なら `bAlert` = `bMode`・`bSpotted`、プレイヤーがいれば `Location` = その位置（既定は本家の (100, 0, 0)）。
- **状態機械 Idle ↔ Alert**: 本家は `Idle` →（`bAlert`、0.5 s）→ `Alert Transition`（切り替えのクリップ 0.8 s）→（残り 0.05 s 未満、0.2 s）→ `Alert`、`Alert` →（NOT `bAlert`、0.6 s）→ `Idle Transition`（0.8 s）→（残り 0.05 s 未満、0.2 s）→ `Idle`、どれも HermiteCubic。本作は切り替えのクリップが無いので、1 回のクロスフェード（`FWasamiStateBlend`）で直接もう一方へ移り、長さは本家の変化の全体（切り替えのクリップが残り 0.05 s になるまでの 0.75 s + 0.2 s = 0.95 s、`ChangeTime`。`TODO(仮)`）、曲線は HermiteCubic。本家の切り替えの状態は先へしか進めないので、変化は終わるまで折り返さない（終わってから次を始める）。最初の更新は移らない（本家の `bSkipFirstUpdateTransition`）ので Idle から始まる。入った状態のクリップは 0 から（変化が終わってから入るので、入る側の重みは必ず 0）。重み 0 の側は進めない。Idle（11.3 s）と Alert（4.0 s）はループ。**本家どおり、プレイヤーが遠い（`bMode` 真、長いコーン）ときが Alert、近い（`CloseArea` の中）ときが Idle。**
- **`DefaultSlot` の Detected**（本家のモンタージュ: ブレンドインはモンタージュの既定 0.25 s・Cubic、自動のブレンドアウトなし）: `PlayDetected` で 0 から流し、重みを 0.25 s で上げ（Cubic）、時刻を進めて最後の姿勢で止める。重みの分だけ状態機械の上に乗る（1 になれば状態機械は見えない。状態機械は下で進み続ける）。
- **LookAt**（本家: `spine_02`、`LookAt_Axis` (0, 1, 0) を骨の空間でなく部品の空間で、上の軸なし、補間なし、制限 90°、アルファは `bSpotted` の真偽で入り 0.5 s〈Cubic〉・出 0 s）: 代理の `Evaluate` で、混ぜた姿勢を部品の空間にして `spine_02` の変形を読み、`Location` を部品の空間へ移し（エンジンの `FBoneSocketTarget` の骨の無いときと同じく世界の位置と読む）、`AnimationCore::SolveAim(骨, 的, +Y, false, 上, 90)` の回転を骨の回転の前に掛け、`LocalBlendCSBoneTransforms` でアルファの分だけ混ぜて局所に戻す（エンジンの `FAnimNode_LookAt` と同じ手順。モジュールの非公開の依存に `AnimationCore`）。部品の +Y はボスの正面。
- 本家の ModifyBone（のこぎり 2 つの骨を拡縮 0）はボスに無い骨なので作らない。

### Zone 2 への配置（組み立ての `_flow`。01 記録）と流れ（11 記録）
- レベルの組み立て（`build_dd_stage_level` / `place_dd_flow Zone2`）が、本家の `MnM_Matron_Idle_2` を `AWasamiMatron` で根 (−6900.98, −1023.26, −50.22)・ヨー 90（前は −X）に置き、`SkeletalMesh` を相対 (3.0, −98.0, −57.0)、`CloseArea` を相対 (−27.38, 617.40, 234.0)・拡縮 (56.44, 10.05, 9.58)（机の前の帯）にする。コーンは `BP_06_Miniboss_viewcone_Matron_Long` を `AWasamiViewconeMatronLong` で (−7067.35, −1025.0, 676.26)・ヨー 180、扇 `Plane` を相対 (1673.83, 0, 0)・拡縮 (35.18, 32.04, 1)、`_Short_5` を `AWasamiViewconeMatronShort` で (−6630.76, −1025.0, 518.37)・ヨー 180・ピッチ −20、扇を相対 (659.98, 0, 0)・拡縮 (13.34, 17.79, 1)。Matron の `LongCone`・`ShortCone` にこの 2 つを入れる。3 つともフォルダ `Hospital/Gameplay/Enemies`、タグ `src:<本家の名前>`。
- Zone 2 の流れの `ActivateMinibossEnemies`（保存 8 の廊下の箱と、8 で開いたとき）が見張りの `Activate` の後に、名前 `AWasamiZone2Flow::Matron`（`MnM_Matron_Idle_2`）で引いた Matron の `Activate` を呼ぶ。迷路の始まり（`Trigger_MazeStart`）の `RemoveAllEnemies` がタグ `Enemy` で Matron を消す（コーンは本家どおり残って見続ける。持ち主の参照は壊れた Matron を指したまま残るが、本家の BP の実行系は壊れた〈Pending Kill の〉相手への呼び出しを飛ばし〈`ProcessContextOpcode` の `IsValid`〉、本作のコーンも `IsValid` で飛ばすので、見つけても何も起きない。07 記録）。

### 大きさ（`AWasamiMatron::MeshScale`。`TODO(仮)`）
本家の長いコーンの高さ（z 676.26。Matron の目の高さと読む）に、ボスの `Idle` の最初のコマの `head` の骨（拡縮 1 で 128.2 cm）が来る拡縮: (676.26 + 107.22) / 128.2 ≈ **6.111**（107.22 cm はメッシュの原点が床より下にある分: Matron の根の z −50.22 と部品の相対 z −57）。高さ約 10.4 m、頭の上は床から約 932 cm（本家の Matron は 1031 cm）。本家は `SK_Matron` を 5 倍で描く。PIE（2026-09-19）で、メッシュの範囲の上端は床から約 860 cm で部屋の天井の下にあり、机の後ろに立って天井・机・壁を突き抜けない。

### PIE で確かめたこと（2026-09-19、Zone 2 をチェックポイント 8 で）
- 見張りと一緒に起き、長いコーンが点く。`CloseArea` に入ると 1 s の内に短いコーンへ替わり、出ると長いコーンへ戻る。長いコーンに見つかると、見張り 6 体が跳び降りて追い、プレイヤーを捕まえる（収録。死亡 → 残りライフの画面まで）。
- **長いコーンを隠れて渡れる道が無い**（本家のレベルの置き方どおり）: 25 cm の格子でナビの上の点ごとに長いコーンの角度・長さと見通し（コーン → カプセルの中心の `Camera` の線）を調べた（`z2_corridor` の道を決めるのに使った）。入口（`PlayerStart_MiniBoss`）から机の前の帯へ行く道はどれも、Matron の目の高さから救急車 8 と 7・13 の間を見通す帯（y −1200〜−700）を渡る。見られない隙間は祭壇の像（`ring_statue_2`）の陰の 25〜50 cm の線と、その先の救急車 13 の端の約 60 cm だけで、コーンの 0.3〜0.5 s ごとの見張りに対して全力（600 cm/s）で 2〜4 回に 1 回見つかる。机の前の帯（`CloseArea`）の中は長いコーンの下（角度の外）で、短いコーンの線は机が遮る。台本 `z2_corridor`（01 記録）は Vanish で帯を渡る。

### テスト（`Tests/WasamiMatronTests.cpp`）
- `Wasami.Matron.Anim`: 変化の長さ 0.95 s、最初の更新は移らない、半分の時間で半分（HermiteCubic）、Alert は 0 から、変化の途中で `bAlert` が戻っても終わるまで進む、終わってから Idle へ戻り Idle は 0 から、Idle のループ、Detected の 0.125 s で半分・0.25 s で全部・下が見えない・最後の姿勢で止まる、LookAt の 0.25 s で半分・0.5 s で全部・出はすぐ、欠けた Detected は流れず欠けた Alert の分は Idle が埋める。
- `Wasami.Matron.Clips`: パス、取り込んだ 3 本の長さ（11.3・4.0・5.2 s）・骨格 `SK_WasamiBoss_Skeleton`・`spine_02` がある。
- `Wasami.Matron.Cones`: コーン 2 種の CDO の値（07 記録）。
- `Wasami.Matron.Removed`: 起きた Matron を消す（迷路の始まりの `RemoveAllEnemies` と同じ）→ コーンの持ち主は消えた Matron のまま → 長いコーンの前に立っても、コーン 2 つは消えず見続ける（壊れた持ち主に `Player Spotted` を送らない）。
- `Wasami.Matron.Actor`: CDO（タグ・拡縮・当たり・ナビ・`AnimClass`・箱の応答と大きさ）と、手で進めるゲームのワールド（0.0625 s 刻み。タイマーは最大 2 刻み遅れるので確かめる時刻に余裕を取る）: `Activate` の前は切り替えない → `Activate` でコーンが Matron のものになり見始める → 最初の `Switch` で `bMode`・長いコーンが 1 s 後に点く・短いのは消える・アニメが Alert → プレイヤーを `CloseArea`（Matron の後ろ）へ → 次の `Switch` で短いコーン・Idle、後ろは見えない → `bMode` が変わらない間は手で点けた長いコーンを消さない → 外へ出ると長いコーンの番 → 長いコーンの前 1500 cm に立つとサイトの間隔の内に見つかり、コーン 2 つが消え、Detected を流し、1.0948 s より前は見張りは追わず後は追う、Detected が全部・LookAt が 1 → 2 回目の `Player Spotted` は何もしない → `Switch` はコーンが無くても `bMode` を書く → Detected は最後の姿勢で止まる。

## 作るアセット

| パス | 中身 |
|---|---|
| `/Game/Wasami/Boss/SK_WasamiBoss` | スケルタルメッシュ（範囲の中心 (0, 0, 85)・広がり (111, 41, 85) cm。正面 +Y）。スロット 1 に `MI_WasamiBoss` |
| `/Game/Wasami/Boss/SK_WasamiBoss_Skeleton`・`_PhysicsAsset` | 骨格（28 本）と取り込みが作る物理アセット |
| `/Game/Wasami/Boss/A_WasamiBoss_Idle` | 11.300 s（339 コマ。ループを閉じた） |
| `/Game/Wasami/Boss/A_WasamiBoss_Alert` | 4.000 s（120 コマ） |
| `/Game/Wasami/Boss/A_WasamiBoss_Detected` | 5.200 s（156 コマ。1 回） |
| `/Game/Wasami/Boss/T_WasamiBoss_BaseColor`・`_MetallicRoughness`・`_Normal`、`MI_WasamiBoss` | テクスチャ 3 と材質のインスタンス（親 `/Game/Pipeline/Materials/M_DD_WasamiGltf`） |

## 原作データの根拠
- 役: 本家の `Matron_MiniBoss_AnimBP`（`pak_reference_2/_assets/DDeception/Content/Animation/Enemies/Nurse/Matron/MiniBoss/`）の状態機械の `Idle`（13.33 s ループ）・`Alert`（8.0 s ループ）と、`BP_06_Matron_MiniBoss` の `Player Spotted` が流す `DD_Matron_Zone_02_Detected_Montage`（1.3333 s、自動のブレンドアウトなし）。本作のアニメの割り当ては `.claude/references/enemy-wasami-motions.md` の「ボスワサミ」。
- 大きさの材料: 本家の `SK_Matron`（`pak_reference_2/_meshes_gltf/Animation/Enemies/Nurse/Matron/SK_Matron.gltf`）は高さ 2.276 m、基準姿勢の `head` の骨 1.819 m。レベルの `SkeletalMesh` の拡縮 5、メッシュの原点は床から −107 cm（Matron の根 z −50.22 + 部品の相対 z −57）。長い視界コーンは z 676（Matron の前 166 cm）。

## 依存関係
- `pipeline/dd_enemy.py`（`BONES`・`RATE`・`_content_frames`・`_sample`・`_key`・`_extract_textures`・`_build_master`・`_import_model`・`MASTER`）、`gltf.py`、`dd_assets.material_instance`。
- Matron: 視界コーン `AWasamiViewcone`・`AWasamiViewconeMatronLong/Short` と見張り `AWasamiEnemySentry`（07 記録）、`WasamiEnemyAnim::BlendPoses`・`FWasamiStateBlend`・`FWasamiBoolBlend`・`FWasamiEnemyAnimSample`（07 記録）、エンジンの `AnimationCore::SolveAim`・`FCSPose`。
- 使う側: レベルの組み立て `dd_level._flow`（01 記録）、`AWasamiZone2Flow::ActivateMinibossEnemies`（11 記録）。

## 既知の制約・注意点
- 敵ワサミを取り込み直してマスター `M_DD_WasamiGltf` を作り直しても、`MI_WasamiBoss` はテクスチャを上書きしているので変わらない。
- 本家の `Idle` ↔ `Alert` の間の切り替えのクリップ（各 0.8 s）に当たるものは原本に無い（1 回のクロスフェードで代える。上の「アニメの再生」）。
- LookAt の軸は部品の +Y（本家と同じ）なので、プレイヤーが真下に近いと上半身が大きく前へ倒れる（制限 90°）。見た目は PIE で見る。

## 変更履歴
- 2026-09-19: PIE で確かめた（大きさ・コーンの切り替え・見つかると 6 体が追う・隠れて渡れる道が無いこと）。コーンが壊れた持ち主・親へ送る呼び出しを `IsValid` で飛ばすようにし（07 記録）、テスト `Wasami.Matron.Removed` を足した（作業一覧の項目 11 のステップ 4）
- 2026-09-19: Zone 2 に置いた（組み立ての `_flow` が Matron とコーン 2 つを本家の位置・部品の変形で置き、Matron のコーンの参照を入れる）。Zone 2 の `ActivateMinibossEnemies` が Matron の `Activate` を呼ぶ（作業一覧の項目 11 のステップ 3）
- 2026-09-19: Matron のアクタ `AWasamiMatron`（部品・`Activate`・`Switch`・`Player Spotted`・1.0948 s の見張りへの知らせ）、ボスのアニメの再生 `UWasamiBossAnimInstance`（Idle ↔ Alert の 0.95 s のクロスフェード・Detected のスロット・`spine_02` の LookAt）、テスト `Wasami.Matron.*` 4 件を足した（作業一覧の項目 11 のステップ 2）。`Wasami.*` の 119 件が通った
- 2026-09-19: 初版。ボスワサミの素材の取り込み（`dd_boss.py`、原本 `boss_wasami.glb`）を記録（作業一覧の項目 11 のステップ 1）
