---
title: 敵ワサミ（素体の素材とアニメの再生）
sources:
  - Content/Python/wasami_tools/pipeline/dd_enemy.py
  - Source/wasami_deception/WasamiEnemyAnimInstance.h
  - Source/wasami_deception/WasamiEnemyAnimInstance.cpp
  - Source/wasami_deception/Tests/WasamiEnemyTests.cpp
  - SourceArt/Wasami/enemy_wasami_v3.glb
  - SourceArt/Wasami/enemy_wasami_capture.glb
updated: 2026-09-18
---

# 敵ワサミ（素体の素材とアニメの再生）

## 役割
本家のナース（`BP_06_ReaperNurse`）の代わりに Zone 1・2 を巡回し追う敵ワサミ。いまは**素材の取り込み**（ユーザーのモデルを、スケルタルメッシュと役の名前で引けるアニメにする）と**アニメの再生**（`UWasamiEnemyAnimInstance`。本家のナースの ABP の形でクリップを混ぜる）まで（作業一覧の項目 4 のステップ 1・2）。敵のアクタ（`AWasamiEnemy`）、AI・捕獲はこの記録に書き足していく。役とアニメの対応の決まりは `.claude/references/enemy-wasami-motions.md`。

## 公開インターフェース
- `WasamiDDTools.import_wasami_enemy()`（01 記録）→ `dd_enemy.import_all()`。戻り値 `textures` 3 / `materials` 2 / `meshes` 1 / `animations` 19。
- `dd_enemy.make_capture_source(old_glb)`: ユーザーの旧モデル（`tmp/enemy_wasami.glb`、git の外）から `SourceArt/Wasami/enemy_wasami_capture.glb` を書く（2026-09-18 に 1 回走らせた。旧 glb が変わらない限り再び走らせる必要はない）。
- `dd_enemy.prepare()`: 前処理した glb を書き、役ごとの（長さ、骨盤を動かした量）を返す。`prepared_file()` がその場所。
- `UWasamiEnemyAnimInstance`（ネイティブの AnimInstance。メッシュの `AnimClass` にする。持ち主が毎フレームのフラグを書く）:
  - `bStunned`（持ち主の State が Stun。立ち上がりで気絶のクリップを `StunDuration` の長さで始める）、`StunDuration`（既定 17.0 = `BP_06_ReaperNurse` の `Delay 17.0`。敵のアクタが自分の値を入れる）、`bAggressiveIdle`（見張り）、`bNightmare`（全回収後の追跡の走り）。`Speed` は読み取り専用（持ち主の `GetVelocity().Size()`）。
  - `PlayOnce(Clip, PlayRate=1, BlendIn=0.25, BlendOut=0.25)`: 名前（`WasamiEnemyAnim::ClipNames`。`'Chase_Slide'` など）のクリップを全身に 1 回かぶせる。持ち主は動いたまま。戻り値は再生の秒数（長さ / 速さ）。無い名前・速さ ≤ 0 は 0 を返して警告する。`StopOnce(BlendOut=0.25)`、`IsPlayingOnce()`（ブレンドアウトが始まったら偽）、`GetMainClip(OutTime, OutWeight)`（いちばん重いクリップの名前。PIE の確かめ用）。いずれも Blueprint から呼べる（Python からも）。
- `WasamiEnemyClip`（クリップの番号。移動と気絶の 7 本、捕獲 3 本、追跡中の変化 6 本、役なし 3 本の順）、`WasamiEnemyAnim::ClipNames`・`FindClip(Name)`・`ClipPath(Clip)`（`/Game/Wasami/Enemy/A_WasamiEnemy_<名前>`。名前は取り込みの `ROLES` の 1 列目と同じ並び）。
- 定数: `MESH` = `/Game/Wasami/Enemy/SK_WasamiEnemy`、`SKELETON` = `…_Skeleton`、`PHYSICS_ASSET` = `…_PhysicsAsset`、`ANIM_PREFIX` = `A_WasamiEnemy_`、`MATERIAL` = `/Game/Wasami/Enemy/MI_WasamiEnemy`、`MASTER` = `/Game/Pipeline/Materials/M_DD_WasamiGltf`、`ROLES`（下の表）。

## 内部構造と処理の流れ

### 原本
- `enemy_wasami_v3.glb`（30 MB。ユーザーの `tmp/enemy_wasami_v3.glb` の写し）: Blender 4.5 の glTF。根 `target_character` の子にメッシュ `output_unwrapped`（89,572 頂点・104,806 三角形、材質 1、テクスチャは PNG の法線 2048²・色 2048²・金属と粗さ 4096²）と `pelvis`。スキン 1（骨 28、根 `pelvis`、ルートの骨なし）、アニメ 16（どれも LINEAR）。
- `enemy_wasami_capture.glb`（259 KB）: 旧モデル（骨 22。名前は全部 v3 にある）の節と、`Backflip`・`sliding_rool`・`Stylish_Walk`・`run_fast_2` のキーをそのまま（旧 glb と同一なのを確かめた）。メッシュ・スキン・テクスチャは入れない（20 MB を LFS に入れないため）。`run_fast_2` は下の載せ替えの物差し。

### 前処理（`prepare`）
glb のキーは 30 fps の動きを 24 fps の場面に焼いたもので、骨盤だけ 24 fps と 30 fps のキーが混ざり、どの骨も 2/30 s から始まる（骨盤は 1/24 s から）。Interchange はコマの境目で終わらないアニメを取り込まない（「アニメーションの長さ … はインポート フレームレート 30 fps と互換性がありません」。16 本中 11 本が落ちた）ので、**全部を 30 fps のコマ（`RATE`）で 0 から標本化し直す**。中身は 2/30 s（`CONTENT_START_FRAME`）から、骨盤以外の骨の最後のキーまで。回転は slerp、書き出す前に四元数の符号を前のキーにそろえる。

役ごとの作り方（`ROLES` の 4 列目）:
- `loop`: 最初のキーを最後の後にもう 1 つ置いて閉じる（元のループは 1 コマ手前で止まっている。`Running` の最初と最後の差は 1 コマの動きの 0.35 倍、`Walking` は 1.5 倍、`run_fast_2` は 1.1 倍で、周期 = 中身 + 1 コマと読んだ）。
- `in_place`: 骨盤の水平（glTF の x と z）を最初のキーの値に止める（高さは残す）。
- `stun_loop`: 無名のモーション（`STUN`、10.04 s）の 0.967〜2.467 s（30 fps のコマ 29〜74、`STUN_LOOP_FRAMES`）。前屈して揺れる部分で、1.5 s 以上離れた姿勢の組のうち最もそろう組（関節の平均のずれ 3.3 cm）。最後の 15 コマ（`STUN_BLEND_FRAMES`、0.5 s）を、ループの始まりの前の動き（コマ − 45）へ smoothstep で混ぜ、最後のキーを最初のキーにする。
- `stun_recover`: 元のモーションでループの後に続く 2.467 s〜終わり（前屈のまま 4.8 s まで、8.0 s で直立、10 s まで落ち着く）。最初の 15 コマはループの続き（コマ 29 + k）から混ぜて入るので、最初のキーがループの最初のキーと同じ。元は終わりで骨盤が横へ 0.2〜0.3 m ずれるので、全体に smoothstep で骨盤の水平をずらし、終わりを `Idle` の最初の骨盤（x 0.004、z −0.022 m）に合わせる（足は合計 0.29 m 滑る）。
- 捕獲（`CAPTURE`）: `once` のみ。`_Retarget` で v3 の骨へ載せ替える。

`_Retarget`（両方にある `run_fast_2` で測る。`REFERENCE_ANIMATION`）: 2 つのモデルの骨は**同じ向きを指し、骨の軸まわりのひねりだけが違う**。各骨の場面での回転は「旧 × 骨ごとに一定の回転」で、その回転は時間で変わらない（測った値: 腕と鎖骨で最大 21.7°、手は向きも少し違う。ずれ 0.075°）。骨 b（親 p）のローカルの回転 r は `inverse(p の回転) · r · (b の回転)` にする。骨盤の位置は「v3 の休み + 0.9929 ×（旧 − 旧の休み）」（x・y・z 同じ係数を最小二乗で。ずれ 0.0000 m）。ほかの骨の移動は捨てて v3 の骨の長さを使う。ずれが `RETARGET_TOLERANCE`（0.5°・2 mm）を超えたら例外。**同じ対応を旧と v3 の `Running`・`Walking`・`BeHit_FlyUp` に当てると v3 の値と 0.08° 以内で一致する**（気絶のモーションは頭だけ 4°）ので、ユーザーのツールが v3 用に書き出すのと同じ形になる。

v3 の `restpose`（腕を広げた基準姿勢、0.083 s）は取り込まない（`SKIPPED`）。一覧に無いアニメが v3 に足されたら、警告を出して元の名前で `once` として取り込む。

| 役（`A_WasamiEnemy_…`） | 元 | 作り方 | 長さ | 骨盤を止めた量（x / z） |
|---|---|---|---|---|
| `Idle` | `Idle_11` | loop | 1.933 s | |
| `Idle_Alert` | `Idle_5` | loop | 1.900 s | |
| `Walk` | `Walking` | loop | 1.033 s | |
| `Run` | `Running` | loop | 0.667 s | |
| `Run_Nightmare` | `run_fast_2` | loop_in_place | 0.600 s | −0.01 / 2.60 m（4.6 m/s） |
| `Stun_Loop` | `01a0a88f-…` | stun_loop | 1.500 s | |
| `Stun_Recover` | `01a0a88f-…` | stun_recover | 7.567 s | 終わりを 0.19 / −0.23 m 寄せた |
| `Capture_1`・`_2`・`_3` | 旧 `Backflip`・`sliding_rool`・`Stylish_Walk` | once（載せ替え） | 2.133・2.767・3.533 s | 動かしたまま（前へ 0.0・5.7・2.1 m） |
| `Chase_PickUp` | `Female_Run_Forward_Pick_Up_Right` | in_place | 1.233 s | 0.00 / 4.02 m |
| `Chase_Charge` | `Male_Head_Down_Charge` | in_place | 0.533 s | 0.01 / 2.17 m |
| `Chase_VaultRoll` | `Parkour_Vault_with_Roll` | in_place | 2.100 s | −0.01 / 4.61 m（途中で横に最大 0.22 m） |
| `Chase_VaultLand` | `Vault_and_Land` | in_place | 3.067 s | 0.64 / 1.05 m（始めの骨盤が 1.67 m と高い） |
| `Chase_RunFast` | `run_fast_5` | in_place | 1.833 s | 0.00 / 3.46 m |
| `Chase_Slide` | `slide_right` | in_place | 1.767 s | 0.01 / 3.35 m（途中で横に最大 0.17 m） |
| `BeHit_FlyUp`・`Knock_Down`・`Push_Up_To_Idle` | 同名（`push_up_to_idle`） | once | 1.533・2.500・3.100 s | 役なし（場面の代用の候補）。`Knock_Down` は骨盤が始めから 1.15 m 後ろにある |

足の運びの速さ（接地した足の後ろへの速さ。再生の速さを移動に合わせるときの物差し）: `Walk` 1.33 m/s、`Run` 4.5 m/s。本家の巡回 350 cm/s・追跡 800 cm/s より遅い。

### アニメの再生（`UWasamiEnemyAnimInstance`）

本家のナースの ABP（`nurse_idle1_Skeleton_AnimBlueprint`）の木を、アニメグラフのノードを使わずに C++ で持つ（本作は手作りの BP を持たず、MCP にアニメグラフを組む道具も無いため）。3 つに分かれる:

1. **`FWasamiEnemyAnimState`**（エンジンを使わない純粋な状態。テストの対象）: どのクリップを、どの時刻・重みで混ぜるかを決める。`Update(Inputs, DeltaSeconds)` → `GetSamples(OutSamples)`（重みの和は 1。欠けたクリップの分は正規化で埋める）。
2. **`UWasamiEnemyAnimInstance`**: `NativeInitializeAnimation` でクリップを読み（**ゲームのワールドのときだけ**。エディタのレベルでは参照姿勢のまま。`WasamiAssets.h` の起動時の読み込みを避けるため）、長さを状態に渡す。`NativeUpdateAnimation`（ゲームスレッド）で持ち主の速さとフラグから状態を進め、標本の一覧 `FrameSamples` を作る。
3. **`FWasamiEnemyAnimInstanceProxy`**: `PreEvaluateAnimation`（ゲームスレッド。更新の後・評価の前に必ず呼ばれる。`PreUpdate` は `NativeUpdateAnimation` より前なので使わない）で標本を写し、`Evaluate`（ワーカースレッド）で各クリップの姿勢を `UAnimSequence::GetAnimationPose` で取り出して `FAnimationRuntime::BlendPosesTogether` で混ぜる。スケルトンの無いクリップは飛ばし（エンジンのシーケンスプレーヤーと同じ判定）、標本が無ければ参照姿勢。ルートモーションは取り出さない。

木（重みは上から掛け合わせる）:

- **全身の 1 回再生**（本家の Slot `Fullbody` にモンタージュを入れる代わり）: 1 回再生の重みの和 W（最大 1）が上に乗り、下の木は 1 − W。複数あるときは重みの比で分ける。
- **根: 気絶**（`FWasamiBoolBlend`。本家の Blend Poses by bool、0.25 s、Linear）: 気絶の側 = `Stun_Loop` か `Stun_Recover`（`FWasamiStunPlayback`）、偽の側 = 移動。
- **移動: Idle ↔ Moving**（`FWasamiStateBlend`。本家のステートマシンの標準のブレンド）: 速さ > 5 で Moving へ 0.5 s（Sinusoidal）、速さ < 5 で Idle へ 0.25 s（ExpOut）。ちょうど 5 はどちらにも移らない。移る先の重みは、その時の重みから遷移の曲線に沿って 1 へ動く。本家は Skating → Stop Skating（`nurse_skate_stop` を 0.35 s から 1 回）→ Idle（残り 10 % 未満、0.5 s、Cubic）だが、止まるクリップが無いので Stop へ入る値のまま直接 Idle へ移る。
  - Idle の中: `Idle` | `Idle_Alert`（`bAggressiveIdle`、1.0 s、Linear。本家は Alert がさらに 2 本〈`bVarIdle` で 0.1 s〉だが、ここは 1 本）。
  - Moving の中: `Walk` | 走り（速さ > 400、0.25 s、Linear）。走りの中: `Run` | `Run_Nightmare`（`bNightmare`、0.25 s。本家に無い分岐で、走りと同じ切り替えにした。`TODO(仮)`）。
- **ブレンドの動き**（エンジンの `FAnimNode_BlendListBase` と同じ）: 重みは 1 / ブレンド時間 の速さで目標へ動く（途中で折り返すと、残りの重みの分の時間で戻る）。リセットの後の最初の更新は目標へ飛ぶ。
- **時刻の進め方**: 重み 0 の枝は進めない。重み 0 から入った状態（Idle / Moving）は中のクリップを 0 から始め、中の切り替えもリセットする（本家の `bAlwaysResetOnEntry` 偽・`bResetChildOnActivation` 偽と同じ）。**本家と違い、移動の木は気絶の下でも 1 回再生の下でも進める**（本家の Slot と BlendList は重み 0 の子を進めない）。気絶が明けたときに移動の木を速さに合った状態（止まっていれば Idle）にしておき、起き上がりの終わりの待機へ走りの姿勢が混ざらないようにするため。追跡中の変化が終わったときも走りの位相が続いている。
- **再生の速さ**（`TODO(仮)`。本家はスケートで速さ 1）: `Walk` = 速さ / 133 を 0.5〜2、`Run` = 速さ / 450 を 0.6〜1.8、`Run_Nightmare` = 速さ / 460 を 0.6〜1.8。分母は各クリップの接地した足の速さ（下の「足の運びの速さ」）、範囲は WebGL 版の 15 記録の `speedRatio` の範囲。巡回の 350 cm/s では `Walk` が上限 2 に当たる（足に合わせるなら 2.6 倍）。待機・気絶は速さ 1、1 回再生は指定の速さ。
- **気絶**（`FWasamiStunPlayback::Start(Duration, ループの長さ, 起き上がりの長さ)`）: 起き上がりを気絶の終わりにちょうど終わるよう `Duration − 起き上がりの長さ`（17 s なら 9.433 s）で始め、ループはその時刻に境目（時刻 0 = 起き上がりの最初のキーと同じ姿勢）が来る位相から始める（`LoopStart` = 1.5 − (9.433 mod 1.5) = 1.067 s）。気絶が起き上がりより短いときは、起き上がりを途中（長さ − Duration）から始める。起き上がりは終わりの姿勢で止まる。`bStunned` の立ち上がり（ブレンドアウト中の 2 回目も）で始め直し、同時に 1 回再生を 0.25 s でブレンドアウトさせる。
- **1 回再生**（`FWasamiOncePlayback`。UE のモンタージュの更新の順に倣う）: 重みを先に動かし（ブレンドインは 1 / BlendIn、ブレンドアウトは 1 / BlendOut の速さ）、次に時刻を進め（速さを掛け、長さで止める）、残りの実時間（(長さ − 時刻) / 速さ）が BlendOut 以下になったらブレンドアウトを始める。重み 0 で消える。新しい `PlayOnce` は前のものを新しい BlendIn でブレンドアウトさせ、既にブレンドアウト中のものは短い方の時間にする（BlendIn 0 なら前のものはすぐ消える）。`StopOnce` も同じ規則。

テスト（`Tests/WasamiEnemyTests.cpp`、`Wasami.Enemy.Anim.*`）: `Blends`（切り替えと遷移の曲線）、`Locomotion`（350・800・2000 cm/s、止まる、ちょうど 5、見張り、Nightmare）、`Stun`（17 s と 5 s の位相、走り → 気絶 → 起き上がり → 明け、2 回目）、`Once`（ブレンド、自動のブレンドアウト、2 倍速、途中の停止、重ね掛け、欠けたクリップ）、`Clips`（名前と場所。**取り込んだ 19 本が揃い、スケルトンが `SK_WasamiEnemy_Skeleton` で、長さがテストの値と合う**。取り込みが変わったらここが落ちる）。

### 取り込み
1. `_extract_textures`: glb に埋め込まれた PNG を `Intermediate/Pipeline/wasami/enemy/T_WasamiEnemy_<BaseColor|MetallicRoughness|Normal>.png` に書き出し、`dd_stage.import_texture` で取り込む（`TEXTURES`: 色は sRGB・`TEXTUREGROUP_Character`、金属と粗さは線形・`TEXTUREGROUP_CharacterSpecular`、法線は `TC_Normalmap`・`TEXTUREGROUP_CharacterNormalMap` で緑を反転〈glTF は Y 上向き〉）。4096² はそのまま（ストリーミングが描く分の mip だけ載せる）。
2. `M_DD_WasamiGltf`（`dd_assets.material` + `_build_master`）: glTF の metallic-roughness の係数 1 の形。色 → Base Color、金属と粗さの B → Metallic、G → Roughness、法線 → Normal。片面、`used_with_skeletal_mesh`。`MI_WasamiEnemy` はそのインスタンスでテクスチャ 3 枚を入れる。
3. `ensure_skeletal_pipeline`: `/Interchange/Pipelines/DefaultGLTFAssetsPipeline` を `PL_Wasami_Skeletal` に写し、種類ごとのフォルダなし、`use_source_name_for_asset` 偽・`asset_name` 空（こうするとメッシュは glTF のメッシュの名前、スケルトンと物理アセットはその `_Skeleton`・`_PhysicsAsset`、アニメは glTF のアニメの名前そのままになる。Interchange の `ImplementUseSourceNameForAssetOption`）、材質とテクスチャの取り込みなし、スタティックメッシュなし、Nanite なし、物理アセットあり、モーフなし、アニメあり・30 Hz で焼く。
4. `_import_model`: 前処理した glb（メッシュと節の名前を `SK_WasamiEnemy` にしてある）を `/Game/Wasami/Enemy` に置き換えで取り込み、スロット 1（`BakedMaterial`）に `MI_WasamiEnemy` を入れ、`ROLES` のアニメが全部あるかを確かめる。取り込みは呼び出しの中で終わる。

## 作るアセット

| パス | 中身 |
|---|---|
| `/Game/Wasami/Enemy/SK_WasamiEnemy` | スケルタルメッシュ。高さ 170 cm（`head_end` 168.5 cm）、幅 148 cm（基準姿勢の腕）。**正面は +Y、左手は +X**（UE のマネキンと同じ。アクタではメッシュを Yaw −90 にする。本家のナースのメッシュも Yaw −90） |
| `…/SK_WasamiEnemy_Skeleton` | 骨 28、根 `pelvis`（高さ 88.5 cm） |
| `…/SK_WasamiEnemy_PhysicsAsset` | Interchange の自動の物理アセット |
| `…/A_WasamiEnemy_<役>` | 上の表の 19 本 |
| `…/T_WasamiEnemy_*`・`MI_WasamiEnemy`、`/Game/Pipeline/Materials/M_DD_WasamiGltf` | 材質 |
| `/Game/Pipeline/Interchange/PL_Wasami_Skeletal` | 取り込みのパイプライン（`paths.SKELETAL_PIPELINE`） |
| `Intermediate/Pipeline/wasami/enemy/SK_WasamiEnemy.glb`・PNG 3 枚 | 前処理の出力（git の外。元の glb の BIN をそのまま持ち、使われなくなった元のアニメの accessor も残る） |

## 原作データの根拠
- モデルとモーションはユーザーの作ったもの（2026-09-18 の指示「`enemy_wasami_v3`・`wasami_mochi_v3`・`boss_wasami` をそれぞれ使用」、捕獲は「旧 glb の 3 本を流用」）。役の対応は一覧（`.claude/references/enemy-wasami-motions.md`）。
- アニメの再生の木と値: `pak_reference_2/_assets/DDeception/Content/Animation/Enemies/Nurse/Reaper/nurse_idle1_Skeleton_AnimBlueprint.json`（`BakedStateMachines` の遷移 3 本の `CrossfadeDuration`・`BlendMode`、`AnimGraphNode_BlendListByBool` 4 つの `BlendTime`・`BlendType`〈0.25 = 根の気絶と走り、1.0 = Alert、0.1 = Alert の 2 本〉、シーケンスプレーヤーの `PlayRate` 1）と、同じ場所の `_bytecode` の `.txt`（`bStunned = (BP_06_ReaperNurse.State == 2)`、`Speed = VSize(TryGetPawnOwner().GetVelocity())`、`bAgressiveIdle = bAggressiveIdle`、Skating → Stop は Speed < 5）。
- 気絶の長さ 17.0 s は `BP_06_ReaperNurse` の Make Choice の `Delay 17.0`。起き上がりをその中に収めるのはユーザーの「明けに起き上がる」の読み（本家に起き上がりのアニメは無く、0.25 s のブレンドで戻る）。
- 本家のナースのアクタの値（部品・気絶の流れ）は進捗記録 `20260917-enemy-wasami-body` の決定事項にあり、アクタを作るときにこの記録へ移す。

## 依存関係
- `pipeline/gltf.py`（glb の読み書き・標本化・四元数）、`dd_stage`（`import_texture`・`_Graph`・`VERSION_TAG`）、`dd_assets`（`material`・`material_instance`）、`paths`（01 記録）。
- エンジン: `InterchangeManager`・`InterchangeGenericAssetsPipeline`、`SkeletalMesh`・`AnimSequence`。
- アニメの再生: エンジンの `FAnimInstanceProxy`（`PreEvaluateAnimation`・`Evaluate`）、`FAnimationRuntime::BlendPosesTogether`、`FAlphaBlend::AlphaToBlendOption`、`WasamiAssets::Path`（00 記録）。追加のモジュールは要らない（`Engine` だけ）。
- 使う側: アニメの再生が取り込んだクリップを名前で読む。敵のアクタ（項目 4 のステップ 3）がアニメの再生を使う。

## 既知の制約・注意点
- glb に**ルートの骨が無い**ので、UE のルートモーションは使えない。前へ進むアニメは前処理でその場の形にした（捕獲の 3 本は捕獲の別室で使うので進んだまま）。
- Interchange は「Node [SK_WasamiEnemy] with a skinned mesh is not root」と警告する（根 `target_character` は単位の変換なので効かない）。
- 追跡中の変化 `Chase_VaultLand` は始めの骨盤が床から約 0.8 m 高く、平らな廊下では宙から始まって見える。扱いは PIE で決める。
- 気絶のモーションは、載せ替えの物差しとしては頭だけ 4° 合わない（旧と v3 で作り直されている）。捕獲の 3 本には関係しない。
- 前処理は純粋な Python（エディタの Python に numpy が無い）で、全体で数秒かかる。
- クリップの名前は `ClipNames` と取り込みの `ROLES` の両方にある。役を足す・名前を変えるときは両方を直す（`Wasami.Enemy.Anim.Clips` が食い違いを見つける）。
- クリップはゲームのワールドでしか読まないので、エディタのレベルに置いた敵は参照姿勢（腕を広げた形）で見える。
- 1 回再生にアニメ通知・終わりのイベントは無い。終わりは `IsPlayingOnce()` で見る。
- `FAnimInstanceProxy::IsSkeletonCompatible` は UE 5.8 で非推奨（警告 C4996）なので使わない。

## 変更履歴
- 2026-09-18: 初版。敵ワサミの素材の取り込み（`dd_enemy.py`、原本 2 つ）を記録
- 2026-09-18: アニメの再生 `UWasamiEnemyAnimInstance`（本家の ABP の木・気絶の位相合わせ・1 回再生の口）とテスト `Wasami.Enemy.Anim.*` を追加
