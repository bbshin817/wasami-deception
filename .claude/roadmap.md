# 作業一覧（最終目標までの段階）

2026-09-17 にユーザーが示した最終目標を、Claude が段階（項目）に分解したもの。**1 項目 = 進捗記録 1 件**（`.claude/progress/`）。順序は依存と「早く通しで遊べる形にする」ことで決めた。運用は `.claude/guides/autonomy.md` の「何を作業するか」: 進捗記録が無いときは「未着手」で依存が満たされた最初の項目を取り、最初の反復は計画（進捗記録を作りステップに分けてコミット）だけで終える。項目が終わったら状態を「完了（日付）」にし、次の項目は次の反復で始める。大規模な項目は進捗記録の中でステップに分ける（`.claude/guides/git-workflow.md` の大規模改修 = 作業ブランチ）。

各項目は「目標 / 完了の条件（何を測って何に合えば終わりか）/ 根拠の置き場 / 依存 / 状態」。根拠は原則コード（`pak_reference_2/`。テレポーテーションは `pak_reference/`。`.claude/guides/original-fidelity.md`）。数は本家のレベルの書き出し（`_levels/06_Hospital*.scene.json`）から数えた。

## 最終目標（2026-09-17、ユーザーの原文）

```
- 敵は `tmp\enemy_wasami.glb` 。多数のモーションを含んでいるため、適切な用途に使う。
    - スタンオーブ使用時: “気絶”モーションを使用する。
    - プレイヤー捕獲時: 本家Monkey Business捕獲時の、黒背景+モンスター+カメラアニメーションの体で、敵ワサミのモーションを再生させる。
        - カメラは本家Monkey Business捕獲時のような、躍動的・非機械的な臨場感あるアニメーションを設ける。
        - 死亡時アニメーションは3種類を設け、ランダムにシャッフルする。
- ワサミシャードは、紫色のやや弱めな閃光を放つようにする。
- スタンオーブ、敵の位置がマップ上で表示される特別シャードは、本家と同一の見た目にする。
- ボス戦は設けない。Zone2の任意の箇所に、脱出ポータルを設置し、そこをくぐると脱出完了とする。

# WebGL版と同じように倣う点

- タイトル画面
- オプション画面
- Escaped後のスコア表示画面

# 既存の修正点

- マウスによる視点移動が遅すぎます
- スピードブースト使用時、集中線がおかしなノイズのように現れます

# 最終の理想点

- 本家と同じようにゲームが開始され、敵キャラは敵ワサミに、通常の紫シャードはワサミシャードに置き換えられ、ボス戦を設けずクリア出来るようにする。
- 除細動器、スピードブーストによって突破できるバリア、シークレット要素等は本家と同じように設置する。
```

## 最終目標について決めたこと（2026-09-17、ユーザーの回答）

- **敵のモーションの割り当て**（`enemy_wasami.glb` の 11 本）: 巡回 = `Walking`、追跡 = `Running`、全回収後（Nightmare）の追跡 = `run_fast_2`、**気絶 = 無名のモーション `01a0a88f-…`（10.0 s）**（ユーザーの指摘を数値で確かめた: 最初の約 5 s は両足がほぼ固定〈ずれ 20 cm 以内〉で頭が腰より約 30 cm 前に出た前屈、6〜8 s で起き上がり直立で終わる。気絶中は前屈の区間をループし、明けに起き上がりを再生する）、捕獲 3 種 = `Backflip`・`sliding_rool`・`Stylish_Walk`、`restpose` は基準姿勢。`BeHit_FlyUp`・`Shot_and_Fall_Forward`・`Stand_Up6` は使わない（気絶の復帰は無名のモーションの後半で足りる）。気絶以外の割り当てはユーザーが明示していない Claude の提案なので、項目 4 の計画で「要確認（ユーザー）」に 1 行残す。
- **捕獲のカメラ**: 本家ホテル（旧版 `01_Hotel`）の捕獲のシーケンスのカメラを写し、ワサミの 3 モーションの長さに合わせる（「本家ホテルの 3 本を写す」）。
- **開始**: 入口レベル `06_Hospital`（ナースの導入・注射室・レントゲン室のカウントダウン・追走・エレベーター）も作る。ステージは `06_Hospital` → `06_Hospital_Zone_01` → `06_Hospital_Zone_02`。
- **敵の AI**: 追跡型だけ。透明化（cloak）・薬投げ・ガスは作らない。速さ・視界・巡回・見張り・Nightmare の値は `BP_06_ReaperNurse*` と行動ツリーのコードから取る。
- **Zone 2 の Matron（中ボス）**: 大きい敵ワサミとして残す。**3D モデルはユーザーが後で指定する**（それまでは敵ワサミの拡大で仮）。
- **脱出**: ボス戦（`06_Hospital_Bossfight`）は作らない。Zone 2 のガレージ（本家がボス戦へ移る `Postmaze_Trigger_Garage` の場所）に**祭壇を本家と同じ見た目で置き、Deadly Decadence（旧版 `03_Manor_Zone2`）の流れ**（全回収 → 祭壇の球が消える → 祭壇を Use して欠片 → ポータル → くぐると脱出）に倣う。
- **音**: 本家の曲・環境音・効果音・Bierce の台詞をそのまま使い、**WebGL 版のワサミの声と字幕も付ける**。
- **シャードの光**: 両方。置かれている間は本家の紫の灯に加えて餅が紫に明滅し、回収時の閃光（`P_ky_flash3`）は紫でやや弱く。
- 既に決まっていたこと（そのまま）: パワー 6 種は Lv5 固定で最初から使える（祭壇での購入は無い。祭壇は脱出にだけ使う）、ライフ 3、制限時間なし、シャードはワサミ餅、原作のロゴとキャラクターのモデルは使わない。

## 項目

### 1. タブレットのパワーの残り（テレキネシス・実機との見比べ・仕上げ）

- 目標: 6 種のパワーを完成させる。残りはテレキネシス（仕組みと粒子）、推定した材質・粒子の実機との見比べ、実装記録と handover の仕上げ、main へのマージ。
- 完了の条件: 進捗記録 `20260916-tablet-powers.md` のステップ 10a〜12 が完了し、記録が消えて `feature/tablet-powers` が main にマージされている。
- 根拠: `.claude/references/powers/`、`pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_TelekinesisPower.txt`。
- 依存: なし。
- 状態: **完了（2026-09-17）**。結果は実装記録 04・06、残った差と仮の値は 04 記録の「既知の制約・注意点」と下の「未回答の要確認」。

### 2. 既存の修正 2 件（マウスの視点移動が遅い・スピードブーストの集中線がノイズに見える）

- 目標: 視点移動の速さを本家と同じにし、集中線を本家と同じ絵にする。
- 完了の条件: (1) 同じマウスの移動量に対する回転角が最新版の実機と一致する（`Tools/desktop.py` の `look` で同じ量を送って収録し、画面の回転を測る）。(2) ブースト中の画面（`observations/ours/`）で集中線が本家の実機の収録と同じ形（放射状の線のコマ送り）に見える。
- 根拠: 実装記録 02・04、`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini`、`pak_reference_2/_bytecode/DDeception/Content/UI/Main/Powers/UMG_SpeedBoost.txt`、最新版の実機。
- 依存: 1（同じ C++ とマスターを触るので、マージの後）。
- 状態: **完了（2026-09-17）**。視点は Enhanced Input が `AxisConfig` の感度 0.07 を自動で重ねていたための 0.07² の二重掛けで、C++ の Scalar を外して実機と同じ 0.175°/カウントにした（02 記録・症状索引）。集中線は `M_Speedlines` の FlipBook を実機に合わせて 2 × 5・30 コマ/s にした（04・01 記録。測り方と値は `observations/README.md` の「視点の速さと集中線」）。

### 3. ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光）

- 目標: 置かれている餅が紫に明滅し、回収の閃光が紫でやや弱くなる。
- 完了の条件: PIE の収録で、餅の発光が周期的に強弱し（周期・強さは仮でよい。要確認に書く）、回収の閃光の色相が紫で、画面の最大輝度が今の `P_ky_flash3` より低い。本家の紫の灯（175、半径 200、(194, 0, 255)）は変えない。
- 根拠: 実装記録 06（`AWasamiShard`、`M_DD_WasamiMochi`、`P_ky_flash3` の推定の材質）。本家に無い本作独自の見た目なので、色と強さは仮の値（`.claude/guides/autonomy.md` の線引き）。
- 依存: 1。
- 状態: **完了（2026-09-17）**。餅は `M_DD_WasamiMochi` の自己発光に紫の波（強さ 1.0・周期 2 s、位相は個体ごとの乱数）を足して明滅し、回収の閃光は本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash`（`P_ky_flash3` の色を紫 × 各色の最大 ^ 0.5 × 0.8）にした。本家の紫の灯は変えていない。PIE の 4.4 m 先で、閃光の中心の最大輝度 247 → 170・色相 299°（実装記録 06、測り方と値は `observations/README.md` の「シャードの光の基準」「餅の紫の明滅」「紫の回収の閃光」）。仮の値は下の「未回答の要確認」。

### 4. 敵ワサミの素体（モデル・アニメ・敵の受け口）

- 目標: `enemy_wasami.glb` をスケルタルメッシュとアニメとして取り込み、敵のアクタ `AWasamiEnemy` の土台（モーションの再生、パワーからの受け口、赤い縁取り）を作る。AI はまだ入れない。
- 完了の条件: 原本を `SourceArt/Wasami/enemy_wasami.glb`（Git LFS。`tmp/` は git の対象外）に写し、取り込み（`wasami_tools` のツールセット）が `/Game/Wasami` にメッシュ・スケルトン・アニメ 11 本を作る。`AWasamiEnemy` が `IWasamiEnemyInterface`（`SetState(Stun)`・`PlayerVanish`・`NoTelepathy`）を実装し、気絶で無名のモーションの前屈をループして明けに起き上がる。本家の Chameleon の赤い縁取り（`SetRenderCustomDepth`。実装記録 04 の決定事項）が付く。Primal Fear・Telepathy・Vanish の仮の的 `AWasamiTestEnemy` の代わりに PIE で使える。モーションの割り当て（上の「決めたこと」）を要確認に 1 行残す。
- 根拠: `tmp/enemy_wasami.glb`（Blender 4.5 の glTF、骨 22、UE のマネキン系の名前）、`pak_reference_2/_bytecode/DDeception/Content/Blueprints/Characters/Nurse/BP_06_ReaperNurse.txt`（部品と縁取り）、実装記録 04（インターフェース）。
- 依存: 1。
- 状態: **進行中**（進捗記録 `20260917-enemy-wasami-body.md`、ブランチ `feature/enemy-wasami-body`。2026-09-17 に計画。縁取りは最新版では呼ぶ者がいないと分かったので、仕組みだけ作って要確認に書いた）。

### 5. ゲームの流れの土台（死亡・ライフ・チェックポイントとセーブ・死亡画面）

- 目標: 死ぬ → 死亡画面 → チェックポイントから再開、ライフとゲームオーバー、進行のセーブ（`SAVING PROGRESS`）を本家どおりに作る。敵はまだ無いので、死亡は項目 8 の罠（除細動器）かデバッグの呼び出しで起こす。
- 完了の条件: ゲームモードの `DeathEvent` で入力が止まりタブレットが下り、`UMG_DeathScreen`（WebGL 版 10 記録の死亡画面の時間・アニメ・音）が出て、ライフが残っていれば最後のチェックポイントで再開し（シャードの回収状態はセーブどおり）、0 なら RESTART / LAST CHECKPOINT / QUIT TO TITLE。チェックポイントの通過で右下に `Progress Saved`。セーブは `SaveGame`（本家の `BP_DD_levelStructSave` の項目）。パワーの死亡のリセット（実装記録 04）がここから呼ばれる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt`、`06_Hospital_Zone_01.txt`（`Respawn`、`Struct Save`、`Progress Saved`）、`pak_reference/_bytecode/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.txt`、WebGL 版 04・10 記録。
- 依存: 1。
- 状態: 未着手。

### 6. ゾーンの進行（開始の流れ・障壁とシャードチェッカー・全回収・ガレージリフトで Zone 2 へ）

- 目標: Zone 1 の開始からガレージリフトで Zone 2 に降りるまでと、Zone 2 の開始を本家のレベル BP どおりに作る。
- 完了の条件: Zone 1 の開始（`Spawn`、Bierce の台詞、目的の帯 `COLLECT ALL SHARDS`、タブレットの矢印 `BP_ArrowPointer`）→ シャード 337 の全回収で `All Shards Collected`（ゾーンの障壁 `BP_ZoneBarrier` とシャードチェッカー `BP_ZoneShardChecker`）→ ガレージリフト `BP_06_GarageLift_Zone1_Special` で Zone 2 へ移り（レベルの切り替えとセーブ）、Zone 2 の開始（`BP_06_GarageLift` ×2、`BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5、地図 `BP_MapTexture_MultiFloor`・`BP_MapArea` ×2）まで PIE で通しで遊べる。数と値はレベル BP とアクタのプロパティどおり。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`、`Blueprints/06_Hospital/Lifts/**`、`Blueprints/Main/BP_ZoneBarrier.txt`、`_levels/06_Hospital_Zone_0*.full.json`。
- 依存: 5。
- 状態: 未着手。

### 7. NavMesh と敵の AI（追跡型）

- 目標: 敵ワサミを本家のナースの追跡型の行動（巡回・発見・追跡・見失い・見張り・Nightmare・出現）で動かす。透明化・薬投げ・ガスは作らない（ユーザーの回答）。
- 完了の条件: 両ゾーンに NavMesh（本家の `NavMeshBoundsVolume` はブラシの形が書き出しに無いので、ステージ全体を覆う箱と `NavModifierVolume` 相当を本作で置く）。行動ツリーの値（速さ・視界の距離と角度・Camera チャンネルの視線・見失うまでの時間・巡回の行き先）と出現（Zone 1 の `Spawn Nurses_06`、Zone 2 の `BP_06_ReaperNurse_Sentry` ×6 の位置）がコードどおり。Nightmare（全回収後）で `run_fast_2`。パワーの作用（Primal・オーブの気絶 17 s〈ナースの BP の `Delay 17.0`。15 s はホテルの猿の行動ツリーの値。項目 4 の調査〉、Vanish で見失う、Telepathy の印）が効く。PIE で巡回 → 発見 → 追跡 → 接触までを収録で確かめる（接触の先は項目 9）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Characters/Nurse/BP_06_ReaperNurse*.txt`、`Animation/Enemies/Nurse/Reaper/*AnimBlueprint*.txt`、行動ツリー（`_assets/DDeception/Content/AI/**`。無ければ `_bytecode` の BTT）、WebGL 版 15 記録（頭脳の作り）。
- 依存: 4、6。
- 状態: 未着手。

### 8. 動く部品と罠（両開き扉・除細動器・スピードバリア・のこぎりの罠・扉の破壊）

- 目標: 静的に置いてある部品を本家の BP どおりに動かす。除細動器は**罠**（`Charge` → `Fire` で 1.25 s ごとに `BP_HitFX` と実績 `06_Trap`、触れた player に `DeathEvent`）。
- 完了の条件: Zone 1 の両開き扉 62 + Zone 2 の 1、除細動器 23 + 13、スピードバリア 4（`BP_SpeedBarrier`。Speed Boost の突進でだけ壊れる。ダッシュでは拒否）、Zone 2 ののこぎりの罠 74（`BP_06_sawTrap_short01` 27・`short02` 16・`medium` 21・`long01` 10）、`BP_06_Hospital_DoorBreak` ×2 が、位置・タイミング・音・死亡の判定までコードどおりに動く。PIE で各種 1 つずつ収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_Defib.txt`・`BP_06_DoubleDoors.txt`・`BP_06_Hospital_DoorBreak.txt`・`Traps/BP_06_sawTrap_medium.txt`・`Traps/BP_06_TrapBase.txt`、`Blueprints/Main/Traps/BP_SpeedBarrier.txt`、`_levels/06_Hospital_Zone_0*.full.json`。
- 依存: 5（死亡）。
- 状態: 未着手。

### 9. 捕獲の演出（黒背景 + 敵ワサミ + 本家ホテルのカメラ、3 種のシャッフル）

- 目標: 捕まると本家ホテル（Monkey Business）と同じ体で、黒い別室で敵ワサミが 3 種のモーションのどれかを再生し、本家のカメラの動きと `JumpscareShake`（0.3）の後、3.5 s で死亡画面（項目 5）へ。
- 完了の条件: 敵に触れた瞬間に敵を全部消し、入力を止め、タブレットを下ろし、`JumpscareCam` と等価の別室のカメラに切り替え、`Backflip`・`sliding_rool`・`Stylish_Walk` を重複なしのランダム（直前と同じものを避ける）で再生し、本家のシーケンスのカメラのトラック（位置・回転・FOV）をワサミのモーションの長さに伸縮して当て、3.5 s 後に `UMG_DeathScreen`。収録で本家旧版の実機（ホテル）の捕獲と並べ、黒背景・寄り・揺れの体が同じに見える。
- 根拠: `pak_reference/_bytecode/DDeception/Content/01_Hotel.txt`（`JumpscareMonkey`、`Monkey_Killshot_03a*` の参照、`JumpscareShake` 0.3、Delay 3.5）、`pak_reference/_levels/01_Hotel.full.json`（`JumpscareCam` と `Monkey_Killshot_01`・`02`・`03a`・`03a2`〜`03a5` の LevelSequence。`_sequences/` には書き出されていない）、`pak_reference/_camera/_camera_shakes.json`、WebGL 版 15 記録（`jumpscare.ts`。3 本と読んだが、レベルの参照は 7 本あるので本数と選び方はこの項目で確かめる）。
- 依存: 4、5、7。
- 状態: 未着手。

### 10. 特殊シャード 2 種（スタンオーブ・敵の位置が地図に出るボーナスシャード）

- 目標: 本家と同一の見た目の `BP_PowerOrb`（オーブ）と `BP_BonusShard`（赤いシャード）を、本家の出現点と周期で出し、取ると敵が 17 s 気絶（気絶モーション。秒数は敵の側。項目 4）／敵が 60 s タブレットの地図に出る。
- 完了の条件: 出現点（Zone 1: オーブ 11・ボーナス 10、Zone 2: 10・10）と周期・最初の出現の時刻がレベル BP とアクタのコードどおり。メッシュと材質は原作のアセット（`power_orb`、赤いシャードの材質）。取得で `UMG_VignetteSides`（`ENEMIES STUNNED` / `ENEMIES REVEALED`。WebGL 版 10 記録）が出て、全敵に `SetState(Stun, byOrb)`、地図に敵の印。テレキネシスでは引き寄せられない。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_PowerOrb.txt`・`BP_BonusShard.txt`、`_assets/**/BP_PowerOrbSpawnPoint*`・`BP_BonusShardSpawnPoint*`、`.claude/references/dark-deception/04-mechanics-items.md`（原作データの値）、WebGL 版 08 記録。
- 依存: 7、9（気絶のモーションと敵の印）。
- 状態: 未着手。

### 11. Zone 2 の Matron（大きい敵ワサミ。視界コーンの中ボス）

- 目標: Zone 2 の `BP_06_Matron_MiniBoss` を、大きい敵ワサミとして本家の巡回路・視界コーン（`BP_06_Miniboss_viewcone_Matron_Long` / `_Short`）・発見で追跡型の行動に入る形で置く。
- 完了の条件: 巡回路と視界コーンの形・速さ・見つかった後の行動がコードどおり。モデルは**ユーザーが後で指定する**（それまで敵ワサミの拡大で仮。要確認に書く）。PIE で見つからずに通れることと、見つかると追われることを収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Animation/Enemies/Nurse/Matron/MiniBoss/BP_06_Matron_MiniBoss.txt`、`Blueprints/06_Hospital/Miniboss/*.txt`、`_levels/06_Hospital_Zone_02.full.json`。
- 依存: 7。
- 状態: 未着手（モデルはユーザー待ち。仕組みは仮のモデルで進めてよい）。

### 12. 秘密と収集物（シークレット）

- 目標: 本家の秘密の部屋と収集物を同じ場所に置き、スコアの `SECRETS` に数える。
- 完了の条件: `BP_SecretRoomZone` ×1・`BP_07_Zone1_SecretWall` ×1（Zone 2）、`BP_MysteryCollectable` ×3（Zone 2）、`BP_Collectable` ×2（Zone 1）+ ×1（Zone 2）、入口の `BP_MysteryCollectable_LoreNote` ×1 が、見つけ方・取り方・表示（`UMG_Collectables_Secret`）までコードどおり。取ると進行にセーブされる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Shared/BP_SecretRoomZone.txt`、`Blueprints/Main/BP_MysteryCollectable*.txt`・`BP_Collectable.txt`、`UI/Main/UMG_Collectables_Secret.txt`、`_levels/06_Hospital*.full.json`。
- 依存: 5、6。
- 状態: 未着手。

### 13. 脱出（ガレージの祭壇 → 欠片 → ポータル）と視線の手のマーク

- 目標: Zone 2 の全回収後、ガレージの祭壇（本家 `BP_01_Statue` の見た目）を Use して欠片を取り、現れたポータルをくぐると脱出完了。流れは Deadly Decadence（旧版 `03_Manor_Zone2`）に倣う。ボス戦は作らない。
- 完了の条件: 全回収で祭壇の球（`ring_statue_orb`）が消え、見て左クリック（視線の手のマーク `UMG_Interact`。実装記録の予定 05）で `Ring_Piece_Pickup` と `UMG_01_RingPieceCollect`、Bierce の台詞、ポータル（本家 `BP_00_Teleport` の見た目と `21-Ballroom_portal_V2` の音）が現れ、くぐると敵を消して脱出（項目 14 の画面へ）。全回収前は祭壇を使えない。ガレージの位置は本家の `Postmaze_Trigger_Garage`。
- 根拠: `pak_reference/_bytecode/DDeception/Content/03_Manor_Zone2.txt`（@1385〜@4380 の祭壇と欠片、`Portal Extra Brightness`）、`pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_02.txt`（@43〜 Postmaze、@1423〜@1773 の台詞・曲のフェード・ポータルの音・敵の除去）、`Blueprints/01_Hotel/BP_01_Statue.txt`、`Blueprints/00_Ballroom/BP_00_Teleport.txt`、`UI/Main/UMG_Interact`。
- 依存: 6、7。
- 状態: 未着手。

### 14. 脱出後のスコア表示画面（`UMG_LevelClear`。WebGL 版と同じ）

- 目標: You Escaped! からリザルト（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・DEATHS・STREAK のランクと FINAL RANK）まで、WebGL 版が原作から写した時間・アニメ・音・ランクの規則で出す。
- 完了の条件: WebGL 版 10 記録（`level-clear.ts`）と 04 記録（`results.ts`）の値どおりに動き、NEXT でタイトルへ戻る（セーブは消す）。ランクの規則は本家のレベル BP が `UMG_LevelClear` に入れる値。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt`、`01_Hotel.txt`（値の計算）、WebGL 版 04・10 記録。
- 依存: 13。
- 状態: 未着手。

### 15. 入口レベル `06_Hospital`（レベルの取り込みと組み立て）

- 目標: 本家の入口レベルを Zone 1・2 と同じ仕組み（前処理 → `/Game/DD` → レベルの組み立て → 焼き込み）で作る。
- 完了の条件: `L_Hospital_Entrance`（仮名）に配置 466・灯 307・霧・空・反射キャプチャがそろい、High で焼け、本家の実機（REPLAY の Torment Therapy は入口から始まる）の開始地点と色味を比べて Zone 1 と同じ精度（1〜2 割）に収まる。動く部品（両開き扉 10 + 4、除細動器 1、ガレージリフト 1、扉の破壊 3、針 `BP_06_Needles` ×7）は静的な配置でよい（動きは項目 16）。
- 根拠: `pak_reference_2/_levels/06_Hospital.scene.json`・`.full.json`、実装記録 00・01（取り込みと焼き込み）。
- 依存: 1。
- 状態: 未着手。

### 16. 入口の流れとカットシーン（ステージ OP・導入・注射室・レントゲン室・追走・エレベーター）

- 目標: 入口レベルの本家の流れを、ナースを敵ワサミに替えて再現する。
- 完了の条件: レベル BP どおりに、タイトルカード `UMG_ChapterPortal`（ステージ OP。WebGL 版 10 記録の `stage-intro.ts`）、Bierce の台詞、救急車の到着・扉・エレベーターのシーケンス（`06_Hospital_Entrance_*`）、ナースの導入（`BP_06_NurseInteract_Intro` ×10・`BP_ReaperNurse_IntroAI`。ワサミのモーションで代用）、注射室（`01_NeedleRoomOverlap`、針 ×7）、レントゲン室のカウントダウン（`BP_06_Countdown`・`_XRay`・`_Spikes`、`UMG_06_Countdown`）、追走（`06_Hospital_Entrance_Escape_DuoNurses`、`BP_06_ReaperNurse_EscapeSpecial`）、扉の破壊 → `OpenLevel(06_Hospital_Zone_01)` まで通しで遊べる。本家のカットシーンのカメラのキーは `_sequences/06_Hospital_Entrance_*` から写す。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt`、`_sequences/06_Hospital_Entrance_*`、`Blueprints/06_Hospital/BP_06_Countdown*.txt`・`BP_06_Needles.txt`・`BP_06_NurseInteract_Intro.txt`、`Animation/06_Hospital/NurseIntro/*`。
- 依存: 15、7、5。
- 状態: 未着手。

### 17. タイトル画面（NEW GAME / RESUME / OPTIONS / QUIT、ポップアップ。WebGL 版と同じ）

- 目標: WebGL 版が原作 `UMG_TitleScreen` から写したタイトル画面（配置・アニメ・音・NEW GAME の確認・開始の演出）を UMG で作る。ロゴとワサミの顔は本作の素材（`<WEBGL>/public/title/logo.webp`・`wasami-face.webp` ほか）。
- 完了の条件: WebGL 版 10 記録（`title.ts`）の曲線・時間・音量どおりに動き、NEW GAME で入口レベル（項目 16。無い間は Zone 1）へ、RESUME でセーブの地点へ。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt`・`UMG_PopUp.txt`、`_assets/**/UMG_TitleScreen.json`、WebGL 版 10 記録、`<WEBGL>/public/title/`。
- 依存: 5（セーブ）。
- 状態: 未着手。

### 18. オプション画面とポーズ画面（WebGL 版と同じ）

- 目標: `UMG_Options`（画質・解像度・明るさ・音量 3 種・字幕・マウス感度・頭の揺れ・Y 反転・ダッシュの切り替え・マウスのスムージング・難易度。設定のセーブ `BP_DD_Settings_SaveGame`）と `UMG_Pause`（Esc）を WebGL 版の記録どおりに作る。
- 完了の条件: WebGL 版 04 記録（`settings.ts`）の既定値と適用先、10 記録（`options.ts`・`pause.ts`）の配置・アニメ・音どおり。マウス感度は `AWasamiPlayerCharacter::MouseSensitivity`（`Look` の値に掛ける。本家の `Character.MouseSensitivity` と同じ）に入れ、`DefaultInput.ini` の `AxisConfig` の 0.07 と視点の修飾子には触らない（項目 2 の結果。02 記録）。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Main/UMG_Options.txt`、`UI/Menu/Pause/UMG_Pause.txt`、`Blueprints/Save/BP_DD_Settings_SaveGame`、WebGL 版 04・10 記録。
- 依存: 17。
- 状態: 未着手。

### 19. 曲と環境音・効果音の残り

- 目標: 病院の曲（通常・追跡・Nightmare）と環境音、まだ無い効果音を本家どおりに鳴らす。
- 完了の条件: `BP_06_MusicPlayer`（Zone 1）・`BP_06_MusicPlayer_Zone2`・入口の曲の切り替えとフェードがコードどおり。レベルの `AudioComponent`（入口 29・Zone 1 43・Zone 2 146）と `AmbientSound`・`AudioVolume` を配置どおりに置く。減衰は SoundCue と減衰設定の値どおり。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_MusicPlayer*.txt`（`BP_08_MusicPlayer` の派生）、`_soundcues.json`、`_levels/06_Hospital*.full.json`、実装記録 01（`UWasamiSoundCueLibrary`）。
- 依存: 6、16。
- 状態: 未着手。

### 20. 台詞と字幕（Bierce の台詞、WebGL 版のワサミの声）

- 目標: Bierce の台詞（`BierceTalk_Blueprint`）を本家どおり鳴らして字幕を出し、WebGL 版のワサミの声（16 本 + 字幕の `manifest.json`。巡回・発見・タイトルなど）を敵とタイトルに付ける。
- 完了の条件: 各レベル BP の `Talk` の呼び出しがそろい、字幕は本家の `_localization.json` / `_strings.json` の文言。ワサミの声は WebGL 版 06・15 記録の鳴らし方（バス・音量・字幕の秒数・場面）どおり。原本は `<WEBGL>/voices/`（wav 55）と `<WEBGL>/public/voices/`（mp3 16 + manifest）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/**/BierceTalk_Blueprint.txt`、`_audio.json`（`Audio/Dialogue/Bierce/Ch06/TT/*`）、WebGL 版 06・15 記録。
- 依存: 7、16、17。
- 状態: 未着手。

### 21. 仕上げ（通しプレイ・性能・パッケージ）

- 目標: タイトルから脱出まで通しで遊べることを確かめ、性能（この PC で 1080p・60 fps 前後、VRAM 6 GB 以内）を整え、パッケージする。
- 完了の条件: 通しプレイの収録（入口 → Zone 1 → Zone 2 → 脱出 → スコア）で止まる箇所が無い。両ゾーンと入口の性能の計測が `.claude/guides/performance.md` の目安に収まる。パッケージは**配布の話なので先にユーザーに確認する**（`.claude/guides/distribution.md`。無人モードでは飛ばして要確認に書く）。
- 根拠: `.claude/guides/performance.md`・`distribution.md`、実装記録 00。
- 依存: 1〜20。
- 状態: 未着手。

## 未回答の要確認（ユーザー）

閉じた進捗記録に残っていた要確認（記録ごと）。答えが出たら該当の場所を直してここから消す。SessionStart hook は未完了の進捗記録の要確認しか出さないので、ここは朝の一覧に出ない。

**`20260917-autonomy.md`（無人運転の仕組み）**

- 2026-09-17: 駆動役の使用量の読み方 — 仮に `--usage-cmd` で外から差し込む形にし、差し込みが無ければ止まる。理由: auto モードの分類器が認証情報のファイルと CLI 本体の読み取りを拒否した。選べるのは (a) ユーザーが使用量を JSON で出すコマンドを用意して `--usage-cmd` に渡す、(b) `--no-usage-check` で回す（5 時間の枠だけ待つ。週間の枠の判定は無い。`--until` で区切る）、(c) Claude に `.credentials.json` の読み取りを許すか、応答の形を教える。場所: `Tools/overnight.py` の `read_usage`・`budget_verdict`、`.claude/guides/autonomy.md` の「予算」。
- 2026-09-17: 使用量の上限に達したときの `claude -p` の返事の文言 — 仮に `usage limit reached` / `hit your limit` / `rate limit`（`|エポック秒` 付きならその時刻まで待つ）にした。理由: 実物を上限まで使って確かめられない。場所: `Tools/overnight.py` の `LIMIT_PATTERN`・`LIMIT_EPOCH`。

**`20260917-shard-glow.md`（作業一覧の項目 3。ワサミシャードの光）**

- 2026-09-17: 餅の紫の明滅の強さと周期 — 仮に `PulseStrength` 1.0・`PulsePeriod` 2.0 s、色は本家の灯と同じ紫にした（4.4 m 先で餅の平均の赤と青が 117 → 202、白飛びなし。明滅はテクスチャに掛けるので顔の模様は残る）。理由: 本家にも WebGL 版にも無い本作独自の見た目で、値の根拠が無い。場所: `Content/Python/wasami_tools/pipeline/dd_shards.py` の `MOCHI_PULSE_*`（変えたら `import_mochi()` を回す）、実装記録 06
- 2026-09-17: 回収の閃光の色と強さ — 仮に原作の `P_ky_flash3` の色の表を、灯と同じ紫 × (各色の最大 ^ 0.5) × 0.8 にした（0.25 倍速・4.4 m 先で、中心の星は輝度 247〈白〉→ 170〈紫、色相 299°〉、紫の円と衝撃波の輪は原作と同じ広がりでやや淡い。虹の円は紫の濃淡になる）。理由: 「紫でやや弱く」の度合いが決まっていない。場所: `dd_shards.py` の `FLASH_GAMMA`・`FLASH_STRENGTH`・`FLASH_COLOR`（`make_flash()` を回すだけでよい）、実装記録 06

**`20260917-look-speedlines.md`（作業一覧の項目 2。視点の速さと集中線）**

- 2026-09-17: 取り残された main の worktree — Claude の一時フォルダの `scratchpad/main-wt`（`git worktree list` に出る）が古い main（`a8ef1ad`）のまま main を開いていて、`git checkout main` を妨げる。独自の作業は入っていないことを確かめたが、消すには `git worktree remove --force` が要るので、仮に残した。理由: 無人では変更を捨てる形の操作をしない。場所: 症状索引の「`git checkout main` が … already checked out」。消してよければ `git worktree remove --force <そのパス>`（その後は普通の `git checkout main` と `git merge --no-ff` に戻せる）

**`20260916-tablet-powers.md`（作業一覧の項目 1。タブレットのパワー）**

- 2026-09-17: 本家の MOD の W-Editor のファイル — 観察中に押し間違いで W-Editor の画面が開き、`%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に扉 `BP_06_DoubleDoors13` の変換が書かれた（値は原作と同じ位置・回転・拡縮で、`Removed` は偽なので見え方は変わらない）。仮にそのまま残した。理由: 無人ではファイルを消さない・戻さない。場所: 上のファイル（W-Editor の Reset で戻すか、ファイルを消すか）
- 2026-09-17: シャードをまとめて回収したときの音の重なり — 本作のまま（テレキネシスで 8 つ回収すると、古い音が止まらず 0.5 倍ずつで重なる。`OnlyFew` の値は原作どおり）にした。理由: 本家の収録に音が無く比べられない。音の確認は人が聞く。場所: 実装記録 06 の「同時発音の差（未解決）」。本家の Zone 1 でテレキネシスを使って聞き比べる（`observations/original/orig-telekinesis-pull.mkv` は音なし）
- 2026-09-17: テレキネシスの力場の材質の推定 — 4 つのグラフを推定し（球 = 暗い青の幕に明るい筋、オーラ = 4 層の曲げたパンと帯の中央の線の窓、地面の輪 = 輪 + いちばん明るい線 + 火花、星屑 = 小さな四角）、決まらない値を仮にした（オーラの 4 層のタイリング・パンの速さ・曲げの速さと強さ、地面の輪の 2 つのパンの速さ）。星屑は本家の収録に合わせて直し、幕は本家と「近い（筋は少ない）」と見て仮の値のまま残した。理由: グラフは cook で消え、パラメータ・サンプル・関数だけが残る（実装記録 04）。場所: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_wall02`・`_build_aura7`・`_build_shockwave02`、`AURA_LAYERS`・`SHOCKWAVE_PANS`（`TODO(仮)`）
- 2026-09-17: Vanish の煙の位置と明るさ — 本家の収録では煙がエレベーターの扉枠（234 cm 先）より奥に明るい藤色で出るが、原作の値どおりの本作は 92 cm 先・目の 97 cm 下に出て、画面全体を暗い紫に薄く覆う。原因はコードからは見つからなかった。仮に粒子の値は原作のまま、材質の `CameraDepthFade` を 64・0 にして見えるようにした。理由: 本家の値は変えない。薄めの値は収録から上限しか決まらない。場所: `dd_powers.SMOKE_FADE_*`（`TODO(仮)`）、実装記録 04 の「既知の制約・注意点」、症状索引の「本家と本作で、同じ値の粒子の出る位置が違って見える」。本家の実機で、煙を見下ろす・横を向いて使う収録を撮れば位置が分かる（煙を前へずらすなら、本家の値から離れるのでユーザーの判断が要る）
- 2026-09-17: テレキネシスの球の粒子の灯の強さ — 同じ条件（Lv4・画質「高」）で、力場の間の画面が本家より明るい（発動の約 0.7 秒後の平均が本家 (95〜99, 141〜147, 186〜192)、本作 (123〜127, 174〜176, 216〜218)）。灯を切ると本家よりずっと暗く、線形の明るさで本家の灯の寄与は本作の約 0.5〜0.7 倍。仮に原作の値のまま残した。理由: 灯の値は原作どおりで、推定の材質では説明できない（エンジンの違いを疑うが確かめていない）。場所: `P_ky_forceField_Telekinesis` の `sphere` の `ParticleModuleLight`（`BrightnessOverLife` 5）、実装記録 04 の「既知の制約・注意点」。本家に合わせて弱めるなら原作の値から離れるので、ユーザーの判断が要る
- 2026-09-17: Telepathy の印のパンの速さと向き — 収録から、雲の変わる速さは今の仮の値の 1〜1.5 倍ほどと分かったが、向きと正確な速さは決まらないので、仮の値（`dd_powers.TELEPATHY_PAN_A`・`_B`）のまま残した。雲の形も、本家は縁にこぶのある塊、本作は丸に近い。理由: 原作のグラフが cook で消えていて、1 本の収録からは決めきれない。場所: `dd_powers.TELEPATHY_*`（`TODO(仮)`）、実装記録 04 の「既知の制約・注意点」。本家の Zone 1 で近くのナースの印を長めに撮れば詰められる
