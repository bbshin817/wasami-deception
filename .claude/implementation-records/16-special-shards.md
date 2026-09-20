---
title: 特殊シャード（スタンオーブと赤いシャード）
sources:
  - Content/Python/wasami_tools/pipeline/dd_specials.py
  - Source/wasami_deception/WasamiVignetteSidesWidget.h
  - Source/wasami_deception/WasamiVignetteSidesWidget.cpp
  - Source/wasami_deception/Tests/WasamiVignetteSidesTests.cpp
  - Source/wasami_deception/WasamiSpecialShard.h
  - Source/wasami_deception/WasamiSpecialShard.cpp
  - Source/wasami_deception/WasamiPowerOrb.h
  - Source/wasami_deception/WasamiPowerOrb.cpp
  - Source/wasami_deception/WasamiBonusShard.h
  - Source/wasami_deception/WasamiBonusShard.cpp
  - Source/wasami_deception/WasamiBonusShardCollectEffect.h
  - Source/wasami_deception/WasamiBonusShardCollectEffect.cpp
  - Source/wasami_deception/WasamiStunCollectEffect.h
  - Source/wasami_deception/WasamiStunCollectEffect.cpp
  - Source/wasami_deception/WasamiSpecialSpawnPoint.h
  - Source/wasami_deception/WasamiSpecialSpawnPoint.cpp
  - Source/wasami_deception/Tests/WasamiSpecialShardTests.cpp
updated: 2026-09-21
---

# 特殊シャード（スタンオーブと赤いシャード）

## 役割
本家の特殊シャード 2 種（最新版 `pak_reference_2` の `Blueprints/Main/BP_PowerOrb` = スタンオーブ、`BP_BonusShard` = 赤いシャード）。1 体ずつ置かれ、150 s ごとに 5 s 明滅して出現点を移る。オーブを取ると全敵が気絶し、赤いシャードを取ると 60 s 敵がタブレットの地図に出る（作業一覧の項目 10）。**作っている途中**: いまあるのは素材の取り込み、取得の画面 `UWasamiVignetteSidesWidget`、共通の基底 `AWasamiSpecialShard`、オーブ `AWasamiPowerOrb` と赤いシャード `AWasamiBonusShard`、それぞれの取得の演出 `AWasamiStunCollectEffect`・`AWasamiBonusShardCollectEffect`、出現点 2 種、プレイヤーの `AddToMap`・`RemoveFromMap`（02 記録）と敵の地図の印（07 記録）。両ゾーンに置いてある（組み立ての `dd_level._flow`。01 記録）。PIE で両ゾーンを確かめた（下の「PIE での確かめ」。作業一覧の項目を閉じるのは進捗記録 `20260919-special-shards` のステップ 7）。

## 公開インターフェース
- ツール: `WasamiDDTools.import_dd_specials()`（素材。`import_dd_shards` と `import_dd_gimmicks` の後。地図の印のマスターと粒子の材質を共有する）。
- `UWasamiVignetteSidesWidget`（`UUserWidget`）… 取得の画面（本家 `UI/Menu/Streaks/UMG_VignetteSides`）。`Show(WorldContext, Color, bText, TextToDisplay)`（本家の拾い物の `Create(Self, UMG_VignetteSides_C, None)` → 名前で `Color`・`Text?`・`TextToDisplay` → `AddToPlayerScreen(5)`。プレイヤーのコントローラが無いと出さない）・`Color`（既定 (1, 0.3952, 0)）・`bText`（`Text?`、既定 偽）・`TextToDisplay`・定数 `StunnedColor` (1, 0.4654, 0)・`RevealedColor` (1, 0, 0.0167)・`StunnedText()`「ENEMIES STUNNED」・`RevealedText()`「ENEMIES REVEALED」（オーブと赤いシャードが入れる値）・`AnimLength`（90001 / 60000 s）・`RemoveDelay` 2 s・`ZOrder` 5・static `LoadAssets(Out)`（絵と書体を読んで足す。オーブが `BeginPlay` で呼ぶ）・`Begin()`（Construct の頭から。テストが単独で呼ぶ）・`Advance(DeltaSeconds)`・`IsFinished()`・`GetTextBlock()`・`GetVignette()`・`Evaluate*`（`Anim` の 6 本の曲線）・素材の `VignetteTexture`（`T_VignetteNew`。`import_dd_powers` が取り込む）・`TextFont`。

- `AWasamiSpecialShard`（`AActor`、抽象）… 2 つが共有する部品と周期。`ShardSpawnTime`（既定 150 s）・`SpawnPoints`（空なら `BeginPlay` でレベルの `SpawnPointClass` のアクタを全部。本家はエディタのボタン `Auto Assign Spawn Points` がクラスで集めて入れる）・`SpawnSpecialShard()`（明滅を始める。本家のオーブは `Spawn Power Orb`、赤いシャードは `Spawn Special Shard`。明滅中と取った後の呼び出しは無視）・`MoveToSpawnPoint(Index)`（明滅なしで点 `Index` へ。範囲外なら動かない。閃光と次の 150 s は同じ）・`Collect()`（1 回だけ。スポーンのタイマーを消して派生の `CollectShard`）・`IsFlickering()`・`IsCollected()`・`GetTimeToSpawn()`（次の `SpawnSpecialShard` までの秒。無ければ −1）・`GetSpawnPoints()`・`GetCrystal()`・`GetMapMark()`・`GetCapsule()`・`GetLight()`・`GetCrystalLocation()`（結晶を消した後は消したときの位置）・定数 `FlickerLength` 5・`FlickerPeriod` 0.1・素材（`CrystalMesh`・`CrystalMaterial`・`MapMarkMesh`・`MapMarkMaterial`・`DisappearFlash`・`AppearFlash`・`CollectImpact`・`PickupSound`・`CollectShake`）。派生が上書きするもの: `LoadPickupAssets`・`BeginCycle`（既定はスポーンのタイマー）・`CollectShard`、コンストラクタで結晶の位置と拡縮・カプセルの大きさ・灯の色・`SpawnPointClass`・`MoveFlashScale`。
- `AWasamiPowerOrb`（`AWasamiSpecialShard`）… スタンオーブ（本家 `Blueprints/Main/BP_PowerOrb`）。static `StunAllEnemies(WorldContext)`・素材の `CountdownSound`・`CollectEffectClass`。デバッグのコンソールコマンド `Wasami.PowerOrb [N]`（レベルのオーブが今すぐ明滅を始める。N を付けると明滅なしで出現点 N へ）。
- `AWasamiBonusShard`（`AWasamiSpecialShard`）… 赤いシャード（本家 `Blueprints/Main/BP_BonusShard`）。`ID`（セーブの `BonusShards` に入る番号。既定 0、Zone 2 の置いたものは 1）・`RevealEnemies()`（地図に敵を足す 1 巡）・`IsRevealing()`・`GetRevealTimeLeft()`（地図から外すまでの秒。無ければ −1）・定数 `RevealLength` 60・`RevealRoundMin` 1・`RevealRoundMax` 2・`CollectEffectClass`。デバッグのコマンド `Wasami.BonusShard [N]`（オーブと同じ形）。
- `AWasamiStunCollectEffect`（`AWasamiSphereBurst`。04 記録）… オーブの取得の演出（本家 `Blueprints/Main/Powers/BP_StunCollectEffect`）。static `GrowthCurve()`・`DesaturationCurve()`・`OpacityCurve()`、定数 `StunRange` 4000。
- `AWasamiBonusShardCollectEffect`（`AWasamiPowerBurst`。04 記録）… 赤いシャードの取得の演出（本家 `Blueprints/Main/Powers/BP_BonusShardCollectEffect`）。`LoadDefaultAssets(Out)`・素材の `WaveSound`・`ShakeClass`・定数 `WaveVolume` 0.5・`WavePitch` 1.5。
- `AWasamiSpecialSpawnPoint`（抽象）と `AWasamiPowerOrbSpawnPoint`・`AWasamiBonusShardSpawnPoint` … 出現点（本家 `BP_PowerOrbSpawnPoint`・`BP_BonusShardSpawnPoint`）。`GetBillboard()`・`BillboardMaterial`・定数 `BillboardSize` 32。


## 内部構造と処理の流れ

### 共通の作り `AWasamiSpecialShard`
本家の 2 つの BP は部品も周期の流れも同じで、値だけが違う（`BP_PowerOrb`・`BP_BonusShard` の SCS とバイトコードを並べて確かめた）。
- 部品（本家の SCS のまま）: `DefaultSceneRoot`（Movable）→ `soul_shard`（結晶。`NoCollision`）→ その子に `Capsule`（相対 (0, 0, 0.29)・拡縮 0.1。`UShapeComponent` の既定の `OverlapAllDynamic`）と `PointLight`（相対 (−0.216, 0, −0.452)・拡縮 0.1、Movable、単位なし 1000・減衰 200・影なし）。ルートの子に `StaticMesh`（地図の印: `Plane`、相対 (0, 0, 2000)・拡縮 (1.5, 1.5, 10)・影なし）。メッシュと材質は `OnConstruction` で入れる。本家の `PPP_Collect_Shard` はどの流れも起こさないので作らない。本家の `NavArea_Obstacle` は、Pawn を止めない体はナビゲーションに入らないので効かない（写していない）。本家の Tick は `Float 3`（0）で結晶を回すだけなので Tick しない。
- **地図の印の当たりは無し**（2026-09-19、ステップ 4 から）: 本家は印をスタティックメッシュの既定（BlockAllDynamic）のままにしているので、20 m 上に見えない板が浮く。地図の矢印（03 記録）と同じく、Zone 2 の上の階で何も遮らないよう `NoCollision`・ナビゲーションに効かない。
- `BeginPlay`: 取得と移動で使う素材（粒子 3・音・シェイク、`UWasamiVignetteSidesWidget::LoadAssets`、派生の `LoadPickupAssets`）を読んで持つ → 出現点が空ならレベルの `SpawnPointClass` のアクタを集める → `BeginCycle`（オーブは `ShardSpawnTime` の繰り返さないタイマーで `SpawnSpecialShard`）。
- `SpawnSpecialShard`: `Flicker`（ルートの見え隠れを子ごと反転。反転の状態は明滅をまたいで残る。最初は隠れる）→ 0.1 s ごとの `Flicker` のタイマー → 5 s（本家の `Delay 5`）で: もう一度 `Flicker` → タイマーを消す → 見える → `MoveToSpawnPoint(RandomIntegerInRange(0, n − 1))`（今いる点も選ばれる。本家の「選び済み」の分岐は定数 False で切れている）。
- `MoveToSpawnPoint`: 結晶の位置に消える閃光（`MoveFlashScale`）→ 点の位置へ `SetActorLocation`（スイープなし・テレポート）→ `ShardSpawnTime` のタイマーを掛け直す → 新しい位置に現れる閃光。出現は 150 s + 5 s ごと（最初は 155 s）。取った後も止めない（下の赤いシャード）。
- 隠れている間もカプセルは残るので取れる。重なりは相手が `GetPlayerCharacter(0)` のときだけ `Collect`（本家の DoOnce）→ スポーンのタイマーを消す → `CollectShard`。
- 地図: プレイヤーの `MinimapActorClasses` の既定に 2 つのクラスがあり、キャプチャがいつも写す（02 記録。本家の `Show Only` の一覧も両方をクラスで持つ）。見え隠れで印も明滅する（本家どおり）。タグ `dd_minimap` を付けないのは、地図の板の組み立て（`place_dd_minimap`）がそのタグのアクタを消して置き直すため。
- テレキネシスのインターフェースを持たないので引き寄せられない。

### オーブ `AWasamiPowerOrb`
- 値: 結晶 `power_orb`・`m_crystal_Inst3`、相対 (0, 0, 125.25)・拡縮 0.5408。カプセルの半径と半高 840.6（世界で 45.46 cm の球、中心 125.4 cm）。灯 sRGB (255, 146, 0)（書き出しの FColor は B, G, R, A）。印 `M_PowerOrb`。出現点 `AWasamiPowerOrbSpawnPoint`。閃光 `P_ky_flash_PowerOrb_Disappear`・`_Appear` を拡縮 0.5。
- `CollectShard`: 明滅のタイマーも消す → 結晶の位置に `P_ky_impact`（拡縮 1）→ `PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75)` → `UWasamiVignetteSidesWidget::Show(StunnedColor, 真, ENEMIES STUNNED)` → カメラマネージャーの `StartCameraShake(BP_CameraShake_Streak, 1, CameraLocal)` → `PlaySoundAtLocation(8-Dark_power_ball_countdown_, 原点, 1, 1)`（減衰の設定が無いのでどこでも同じに聞こえる。17.3 s）→ `AWasamiStunCollectEffect` を原点に出す → `StunAllEnemies` → `Destroy`（明滅の `Delay` もここで終わる）。本家は `DestroyActor` を閃光の直後に呼んで続きを流す（本作は最後に呼ぶ。見た目は同じ）。ゲームインスタンスの `Used Stun Orbs?`（実績だけ）と `PrintString` は写さない。
- `StunAllEnemies`: タグ `Enemy` の全アクタのうち `IWasamiEnemyInterface` を実装するものに `SetState(Stun, true)`（本家は `GetAllActorsWithTag(Enemy)` → インターフェースへのキャスト）。距離も遮蔽も見ない。気絶の秒数とアニメは敵の側（07 記録）。

### 赤いシャード `AWasamiBonusShard`
- 値: 結晶 `soul_shard`・`m_crystal_Inst`、相対 (0, 0, 97.09)・拡縮 20。カプセル 49.57（世界で 99.14 cm の球、中心 102.9 cm）。灯 sRGB (255, 31, 0)。印 `M_Bonus_Shard`。出現点 `AWasamiBonusShardSpawnPoint`。閃光 `P_ky_flash_BonusOrb_Disappear`・`_Appear` を拡縮 1。
- `BeginCycle`（本家の `BeginPlay` の `Delay 0` の後）: 次のティックで、ゲームモードのセーブ（`AWasamiGameMode::GetSave()` の `Hospital.BonusShards`。本家はゲームモードの `Level` で `levelStruct` を引く）に `ID` があれば `Destroy`、無ければスポーンのタイマー。本家がここで入れる乱数の `Float 1` とゲームステートは何も使わない。
- `CollectShard`: スポーンのタイマーだけを消す（明滅は止めない）→ セーブの `BonusShards` に `AddUnique(ID)`（書き込みは次のチェックポイントの保存。本家どおり。スコア画面の BONUS SHARDS が数える〈13 記録〉）→ 結晶の位置に `P_ky_impact1` → `PlaySound2D(Bonus_Shard_Pickup_v1, 0.8, 1)` → `Show(RevealedColor, 真, ENEMIES REVEALED)` → `BP_CameraShake_Streak` → 印・結晶・灯を `DestroyComponent`（アクタは残る。結晶の位置は覚えておく）→ `AWasamiBonusShardCollectEffect` を原点に → `GetPlayerCharacter(0)` を `AWasamiPlayerCharacter` にキャストして覚え、できたら `RevealEnemies`（できなければ何も起きず、シャードは残る。本家どおり）。`PrintString`（Collected Special Shard）は写さない。
- `RevealEnemies`（本家の ForEach とその中の `Delay`）: タグ `Enemy` の全アクタについて、プレイヤーの `AddToMap(そのクラス)`。ループの本体で `Delay(RandomFloatInRange(1, 2))` を掛ける（本家の `Delay` は動いている間の呼び出しを無視するので、1 巡に 1 つ。敵が 0 のときは掛からず、そこで巡りが止まる）。ループの後の `Delay 60` は最初の巡りの 1 回だけが効く。1〜2 s ごとに足し直すので、後から出た敵も載る。本家の `Gate`（初めは開き、60 s で閉じる）は、60 s でシャードごと消えるので写していない。
- 60 s（`EndReveal`）: タグ `Enemy` の全アクタについて `RemoveFromMap(そのクラス)` → `Destroy`。
- **明滅の最中に取ったとき**（本家どおり）: 明滅のタイマーと `Delay 5` は止まらないので、5 s の終わりに結晶のあった所で消える・現れる閃光が 2 つ出て、アクタは出現点へ移り、スポーンのタイマーが 150 s で掛かる（60 s で消えるので効かない）。本家は消した結晶のコンポーネントの位置を読むので、閃光は取った所に出る。

### 取得の演出 `AWasamiBonusShardCollectEffect`
本家の `BP_BonusShardCollectEffect` は球の無い `AWasamiPowerBurst` の枠（04 記録）: `PostProcess` の `ColorGain` (1.61, 0, 0.447, 1)（赤紫。彩度 0）、`PostProcess1` は中間調 × 100・色収差 50（ゲインと彩度の値は上書きのフラグが無いので写さない）、`float2` は Primal Fear と同じキー。`BeginPlay` で `PlaySoundAtLocation(Stun_Wave_Attack_New_04, 原点, 0.5, 1.5)` と `ClientPlayCameraShake(01_Hotel_Lobby_ElevatorShakeStop, 25)`、2 s のタイムラインで重みを `float2` から、終わりで消える。**動かない**（ボリュームが範囲なしなので位置は絵に関係しない。本家もプレイヤーへ動かさない）。タイムラインの `float`・`desaturation`・`opacity` は何にもつながっていないので写さない。

### 取得の演出 `AWasamiStunCollectEffect`
本家の `BP_StunCollectEffect` は `BP_PrimalPower` と部品も流れも同じで（04 記録の `AWasamiSphereBurst`）、違いは値だけ: `Range` 4000、`PostProcess` の `ColorGain` (1.61, 0.9416, 0, 1)（橙。`PostProcess1` は Primal と同じ中間調 × 100・色収差 50）、球の MID の `Color` (0.258, 0.0737, 0)、波のピッチ 2、トラック `float`（0 → 1.977 s で 1.014 → 2.497 s で 1.079。2 s で半径 約 40.9 m）・`desaturation`（0.015 → 0.42 s で 0.080 → 1.92 s で 0.254）・`opacity`（1 → 0.75 s で 0.634 → 1.92 s で −0.002）。`float2` は Primal と同じキー。**敵を気絶させない**（オーブが全敵に送る）。オーブが原点に出し、`StartPower` がプレイヤーの位置へ動かす。2 s で消える。

### 出現点
本家の 2 つとも、ルートの `MaterialBillboard`（32 × 32 cm・ワールド空間・ゲーム中は隠れる）だけ。材質はオーブの点が `m_crystal_Inst3`、赤いシャードの点がエンジンの `VertexColorViewMode_RedOnly`（`OnConstruction` で入れる）。本体は点のアクタの位置を読む。

### 配置
ステージの組み立て（`dd_level._flow`。01 記録の「特殊シャード」）が、本家のレベルの置き場所に本体を 1 つずつ（Zone 1 のオーブは地図の外の z 5725、赤いシャードは y 189375・z −11995。Zone 2 は (30699, 36877, 548) と (36563, 59764, −1905)。最初の明滅までは見えない所にいる）と出現点（Zone 1 に 11・10、Zone 2 に 10・10）を置く。本家の置いたものの `Spawn Points` はそのゾーンの同じ種類の点の全部なので、本作は `SpawnPoints` を空にしてクラスで集めさせる。置いたものの値は Zone 2 の赤いシャードの `ID` 1 だけ（Zone 1 は既定の 0。セーブの `BonusShards` はゾーンをまたいで 1 つなので、2 つの ID が違う）。

### 取得の画面 `UWasamiVignetteSidesWidget`
- 木（`RebuildWidget` が本家のスロットどおりに組む。連続回収の画面 `UWasamiShardStreakWidget` と同じ作り。13 記録）: ルート `CanvasPanel_0`（`HitTestInvisible`、平常の角度 −0.158°）→ `Image_161`（`T_VignetteNew` を 1024² のブラシで、アンカー全面・オフセット 0.96 / 0.54・整列 (0.5, 0.5)・自動の大きさ、`ColorAndOpacity` (1, 0.2308, 0, 0.4177)、平常の拡大 2）→ `TextBlock_47`（アンカー下中央、左 −72.96・上 −233.08、151 × 40、中央揃え。白 α 0.8、helvetica-neue-bold 35、縁取り 1 の黒 α 0.638〈影にも〉、平常の `RenderOpacity` 0。既定の文は本家の「ENEMIES STUNNED FOR 30 SECONDS」で、本家の拾い物はどれも上書きする）。
- `NativeConstruct` → `Begin`: `Text?` なら `TextBlock_47` に `TextToDisplay`、偽なら `Hidden`。続けて**ウィジェット全体**の `SetColorAndOpacity(Color)`（本家のバイトコードは対象なしの `SetColorAndOpacity` = `UUserWidget` のもの。文字と縁の色の両方に掛かる: オーブは橙の縁と橙の文字、赤いシャードは赤）。`Anim` を 0 から、2 s で `RemoveFromParent`。`NativeTick` → `Advance` が経過を進めて曲線を当てる（UMG のアニメは使わない。スレートの実時間で進むので `slomo` の影響を受けない）。
- `Anim`（1.5 s。キーは書き出しの値、接線は書き出しの自動の値を `WasamiWidgetAnimation` で）: 文字の拡大 4 → 0.1 s で 1 → 0.15 s で 1.1 → 0.25 s で 1、文字の不透明 0 → 0.1 s で 1 → 0.7 s で 1 → 1.5 s で 0（キーの間で自動の接線が 1 を少し越える。本家も同じ）、縁の拡大 1.4 から 0.35 s の 1.25 まで直線 → 0.9 s まで 1.25 → 1.5 s で 2、縁の α 0 → 0.15 s で 1 → 0.25 s で 0.5 → 0.9 s で 0.25 → 1.5 s で 0（色のほかのチャンネルにキーは無く、画像の (1, 0.2308, 0) のまま）、ルートの角度 0 → 0.1 s で 0.5° → 0.133 s で 0.5° → 0.15 s で −2° → 0.25 s で 0 と拡大 1 → 0.133 s で 1.05 → 0.467 s で 1。ルートの区間は 28000 ティック（0.467 s）で終わり、その後はルートの平常の値（WebGL 版と同じ読み）。アニメの後（1.5〜2 s）は最後の値のまま（UMG_ShardStreak と同じ。平常の値に戻さない）。

## 作るアセット
`WasamiDDTools.import_dd_specials`（`pipeline/dd_specials.py` の `import_all`）が作る。すべて `pak_reference_2` から。

| パス | 中身 |
| --- | --- |
| `/Game/DD/Meshes/Shared/power_orb` | オーブの本体（半径 約 44 cm の球。スロット 1 に `m_crystal_Inst3`〈本家の既定〉）。`dd_assets.static_mesh`（Nanite なし） |
| `/Game/DD/Meshes/Ring_Assets/soul_shard` | 赤いシャードの本体（2.2 × 1.8 × 8.6 cm。本家は 20 倍で置く）。スロットの本家の既定 `m_crystal_Inst1` は作らない（赤いシャードが `m_crystal_Inst` を当てる） |
| `/Game/Pipeline/Materials/M_DD_Crystal` | 本家の `m_crystal` の推定（下の「結晶の材質」） |
| `/Game/DD/Materials/Fords_Materials/m_crystal`・`m_crystal_Inst3`・`m_crystal_Inst`・`m_crystal_Inst2` | 原作のパスの推定のインスタンスと、その子のオーブ用（橙: `color1` (0.526, 0.094, 0.047)・`emissive_col` (0.896, 0.226, 0)・`Fresnel Setting` (5, 0.592, 0)・`emissive_entensity` 29.9・`env_cubemap` `DefaultTextureCube`）と赤いシャード用（赤: `color1` (0.531, 0.009, 0)・`emissive_col` (0.156, 0, 0.013)・`emissive_entensity` 29.9）と Zone 2 の祭壇の球用（紫: `color1` (0.010, 0, 0.078)・`emissive_col` (0.133, 0, 0.391)・`emissive_entensity` 29・`emissive_speed` 0.15・`roughness` 0.01・`env_cubemap` `DefaultTextureCube`。ステージの取り込み `dd_stage` が `make_material` からここの `make_crystal` を呼んで作らせる。2026-09-20）。`color2` はどれも黒。`distortion_normal` は 3 つとも `T_ShapeNormal`〈オーブと祭壇の球は明示、赤いシャードは親から〉）。既にあるインスタンスは同じパスのまま親を付け替え、ステージが入れていた `base_property_overrides` を外す |
| `/Game/DD/Materials/Shared/M_PowerOrb` | オーブの地図の印。`dd_shards` の `M_DD_MapMark` のインスタンス、`Color` (1, 0.2903, 0) |
| `/Game/Pipeline/Materials/M_DD_MapMarkMasked` | 形で切り抜く地図の印の推定のマスター: `Color` をベースカラーと自己発光に、`Mask`（`T_EnemyTriangle`）の R をマスクに（Masked、しきい 0.3333） |
| `/Game/DD/Materials/Shared/M_Bonus_Shard`・`M_Enemy` | 赤いシャードと、赤いシャードが地図に出す敵の印。`M_DD_MapMarkMasked` のインスタンス、`Color` (1, 0, 0) |
| `/Game/DD/Textures/Shared/T_EnemyTriangle` | 三角の形（1024²、G8、リニア） |
| `/Game/DD/ThirdParty/AdvancedMagicFX13/Particles/P_ky_flash_PowerOrb_Appear`・`_Disappear`・`P_ky_flash_BonusOrb_Appear`・`_Disappear` | 出現点を移るときの消える・現れる閃光。`dd_particles` が書き出しから組む |
| `.../Particles/P_ky_impact`・`P_ky_impact1` | オーブ・赤いシャードを取ったときの閃光（`P_ky_impact2`・`3` と同じ材質 2 つ） |
| `.../Textures/T_ky_flash01_4x4`・`T_ky_lensFlare01`・`T_ky_maskRGB3` | 閃光のテクスチャ（原作の設定のまま。`T_ky_maskRGB3` は AdvancedMagicFX13 のもの。除細動器の AdvancedMagicFX09 のものとは別） |
| `.../Materials/MI_ky_flare01b_primitiveG`・`R` | 消える閃光の星。`dd_shards` の推定の `M_ky_flare01_primitive` のインスタンス（`alphaDensity` 1.3 / 1.8・`baseTex` `T_ky_flash01_4x4`・`selectCh` G / R） |
| `/Game/Pipeline/Materials/M_DD_KyPrimitiveColor`・`M_DD_KyLensFlare02`、`.../Materials/M_ky_primitiveColor`・`MI_ky_primitiveColor`・`M_ky_lensFlare02` | 消える閃光の残りの材質の推定（下の「閃光の材質」）。`MI_ky_primitiveColor` は `useHilight` 真と両面の上書き |
| `/Game/DD/Audio/SharedGameplay/8-Dark_power_ball_countdown_`・`Bonus_Shard_Pickup_v1`・`Stun_Wave_Attack_New_04` | オーブの取得・赤いシャードの取得・取得の演出の波の音 |

使うが、ここでは作らないもの: `Soul_Shard_Pickup_v2_Cue`（`import_dd_shards`）、`BP_CameraShake_Streak`・`01_Hotel_Lobby_ElevatorShakeStop`・`M_05_Primal`・`T_VignetteNew`（`import_dd_powers`）、`helvetica-neue-bold_Font`（`import_dd_tablet`）。`PPP_Collect_Shard` はどちらの BP の流れも起こさないので作らない。

### 結晶の材質（`m_crystal`。グラフは cook で消えている）
書き出しに残るのは出力の一部（金属・スペキュラ・自己発光。法線は未接続）、パラメータ、Noise 2 つの FeatureLevelSwitch、`BoundingBoxBased_0-1_UVW`、Custom を通して読むキューブ。コンパイル済みのシェーダー（`Tools/dd/cooked_shaders.py "Fords_Materials/m_crystal."` の SM5 のベースパス）を読んで、式をそのまま組んだ（不透明・ライトあり）:
- **法線 N**（反射と屈折の両方がこれを軸に取る）= TransformVector(接空間 → ワールド)(`distortion_normal` を UV × 0.1 で読んだ接空間の法線) + 頂点法線。**正規化しない**（シェーダーにも `rsq` が無い）。`distortion_normal` は親の既定も 3 つのインスタンスも `/Engine/EditorShapes/Textures/T_ShapeNormal` で、中身は 255 分の 1 の揺らぎしかない平らな法線なので、N はほぼ**頂点法線の 2 倍**になる。長さが 2 だと `reflect`・`refract` の結果は単位法線のときと別物（正面では反射が 7 倍の長さ）なので、平らでも省けない
- 自己発光 = max(0, Noise(反射ベクトル（上の N を軸に、正規化せず）× 0.75 + 時間 × `emissive_speed`。3D テクスチャのグラディエント・乱流・4 段・−0.5〜0.5) × `emissive_col` × `emissive_entensity` + Fresnel(指数 5、基底 0.04) × `Fresnel Setting` + `Additive Emissive`)
- t = Noise(ワールド位置 × `tile_ratio` − 時間 × `emissive_speed`。テクスチャのシンプレックス・乱流・4 段・0〜1) + バウンディングボックスの Z（0〜1）− 0.5
- ベースカラー = saturate(lerp(`color2`, `color1`, t) + `env_cubemap` を refract(−カメラ, 上の N, 0.66) の向きで × 0.5)（屈折は Custom `return refract(-V, N, 0.66);`。HLSL の `refract` は全反射（cos² < 0）で 0 を返し、シェーダーも同じ判定を持つ）
- 金属 = t × 0.5、スペキュラ = t、粗さ = `roughness`（0.01）
- 推定で外したもの: **無し**（2026-09-21、作業一覧の項目 28 のステップ 13 で `distortion_normal` を入れ、`CRYSTAL_LEFT_OUT` は空になった）。本家の親の `env_cubemap` だけは既定が空なので、こちらの親は `DefaultTextureCube` を既定にした（赤いシャードはこれを継ぐ）。`T_ShapeNormal` はエディタの内容だが、本家も pak に cook して入れている（`pak_reference_2/Engine/Content/EditorShapes/Textures/`）ので `/Engine/` のパスのまま使う。

### 地図の印の色
3 つとも書き出しは `Constant3Vector` を値なしで持つだけだが、コンパイル済みのシェーダーに定数が残っていた: `M_PowerOrb` (1, 0.2903, 0)、`M_Bonus_Shard`・`M_Enemy` (1, 0, 0)（`T_EnemyTriangle` の 1 チャンネルを 0.3333 で切る）。ついでに `M_Shard` の定数は (0.482481, 0, 1) と分かった（`dd_shards` は画面の実測で (0.70, 0.0071, 1.0) に合わせている。06 記録）。新しい 3 つはシェーダーの定数のままにし、タブレットでの見え方は見比べていない（作業一覧の項目 28 の後回し）。

### 閃光の材質（推定。グラフは cook で消えている）
- `M_ky_primitiveColor`: 残る式は `hilightPower`・`hilightColor`・`MF_ky_addHilight` と、自己発光（A = Add、B = 粒子の色）と不透明度（A = Multiply、B = 粒子の α）の静的スイッチ `useHilight`。シェーダーでは、スイッチ偽は不透明度 = 粒子の α を深さで 100 かけて消す。真（`MI_ky_primitiveColor`）は `T_ky_maskRGB3` の G と B を UV × 0.2 に時間 × (0.1, −1)・(−0.2, −2) で流して読み、n1 × n2 × (n1 + n2) × 2500 × `hilightPower` × `hilightColor` の光り。推定は真の側で粒子の色にこれを足し（Add）、不透明度は両側とも深さで消す粒子の α（Multiply の相手は分からない）。
- `M_ky_lensFlare02`: 自己発光 = 粒子の色。不透明度 = saturate(lerp(`remap1`, `remap2`, s(時間, 2)) × G^`alphaDensity`)。G は `T_ky_lensFlare01` を lerp(`rotRemap1`, `rotRemap2`, s(時間, 0.5)) × 0.25 だけ中心で回した UV で読む。s(時間, p) = (sin(2π sin(2π 時間 / p)) + 1) / 2（周期 p の Sine を `Sine_Remapped` に通したもの）。粒子の α は入らない。

## 原作データの根拠
- 本体・部品・流れ: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_PowerOrb.json`・`BP_BonusShard.json`、`_bytecode/.../BP_PowerOrb.txt`・`BP_BonusShard.txt`（`python Tools/dd/bp_flow.py`）。取得の演出は `Blueprints/Main/Powers/BP_StunCollectEffect`・`BP_BonusShardCollectEffect`。
- メッシュ: `_meshes.json` の `/Game/Meshes/Shared/power_orb`・`/Game/Meshes/Ring_Assets/soul_shard`。
- 材質: `_assets/.../Materials/Fords_Materials/m_crystal*.json`、`Materials/Shared/M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`・`M_Shard.json`、`ThirdParty/AdvancedMagicFX13/Materials/*.json` と、それぞれのコンパイル済みシェーダー（`Tools/dd/cooked_shaders.py`）。
- 取得の画面: `_assets/DDeception/Content/UI/Menu/Streaks/UMG_VignetteSides.json`（木・`Anim` のキー・区間）と `_bytecode/.../UMG_VignetteSides.txt`（Construct）。オーブと赤いシャードが入れる値は `BP_PowerOrb.txt` @1811〜・`BP_BonusShard.txt` @3341〜。
- WebGL 版: `.claude/references/webgl/implementation-records/08`（特殊シャード）・`10`（`UMG_VignetteSides`）・`11`（地図の印）。

## 依存関係
- `dd_assets`（音・テクスチャ・メッシュ・材質・インスタンス・推定の材質）、`dd_particles`（Cascade の粒子）、`dd_stage._Graph`（01 記録）
- `dd_shards` の `M_DD_MapMark` と閃光の材質（06 記録）、`dd_gimmicks` の `MI_ky_flare14R`（08 記録）
- 取得の画面: `WasamiWidgetAnimation.h`（曲線）、`WasamiAssets.h`、`import_dd_powers` の `T_VignetteNew`（04 記録）と `import_dd_tablet` の書体 `helvetica-neue-bold_Font`（03 記録）
- エンジン: `MaterialExpressionNoise`・`MaterialExpressionCustom`・`MaterialExpressionFresnel`・`MaterialExpressionRotator`
- 本体: `AWasamiSphereBurst`・`AWasamiPowerBurst`・`AWasamiPrimalPower::PrimalFadeCurve`（04 記録。取得の演出の基底と曲線）、`IWasamiEnemyInterface` とタグ `Enemy`、敵の地図の印（07 記録）、プレイヤーの `MinimapActorClasses`・`AddToMap`・`RemoveFromMap`（02 記録）、ゲームモードのセーブの `BonusShards`（06 記録）、`UWasamiVignetteSidesWidget`

## PIE での確かめ（2026-09-19、進捗記録のステップ 6）
収録と画像は `Intermediate/DesktopAgent/shots/`・`Intermediate/Overnight/`（git の外）。調べる道具は `observations/tools/specials/`（`probe.py` が本体・出現点・敵の状態を読む、`move.sh` が本体を好きな所へ動かす、`freeze.sh` が敵の時間を止める。git の外）。
- **Zone 1**（`Wasami.ResetSave` から、04）: オーブの出現点 11・赤いシャードの 10・`ID` 0。`slomo 5` で早回しすると、143〜159 s の間に両方とも出現点へ移った（オーブ [10]、赤いシャード [5]）。`Wasami.PowerOrb` で 0.1 s ごとの明滅 → 5 s で橙の閃光が出て別の点へ移った（`pie-orb-flicker.mkv`、グリッド `specials-grid-flicker.png`）。
- **オーブ**（Zone 2 のチェックポイント 9。迷路の入口に動かして歩いて取る）: タブレットの地図にオーブの橙の四角。取ると黄の閃き → ENEMIES STUNNED → 橙の球が広がって約 2 s で晴れる（`pie-orb-collect2.mkv`、`specials-grid-orb.png`）。迷路のナース 3 体が `Stun`・気絶の残り 16.4 s（取った直後に読んだ）。
- **赤いシャード**（同じ所）: 地図に赤い三角。取ると赤紫の閃き → 赤の ENEMIES REVEALED と赤い縁（`pie-bonus-collect.mkv`、`specials-grid-bonus.png`）。地図を広げる（Z）と 50 m の内のナース 2 体に赤い三角、60 s 後にシャードが消えて印も外れた（`specials-grid-map.png`）。`Wasami.Checkpoint 9` → `Wasami.Kill` で開き直すと、赤いシャードは出ずオーブは出た。`Wasami.LevelClear` の BONUS SHARDS は 1/2・A・+15。
- **出現点のそばの罠**: Zone 2 のオーブの出現点 5 (7124, 1766) はのこぎりの罠（`WasamiSawTrap_3` (7169, 1750)）の通り道にあり、歩いて取りに行くと取った直後にのこぎりで死んだ（打たれた閃き → 死亡画面）。本家の配置どおりで、Zone 2 の出現点の多くが迷路の罠の通り道にある。
- **通し**（台本 `Tools/playthrough.py` を `Wasami.ResetSave` から 1 回の PIE で、`shards_through2.mkv`）: 11 区間がすべて終わりに着いた（終了コード 0、266 s）。1 回目は `z2_altar` の最初の歩きで迷路の入口のアーチの脇に当たって止まり、台本を直した（01 記録）。どの区間もゾーンに 155 s いないので、特殊シャードは台本の道に出ない。

## テスト（`Tests/WasamiSpecialShardTests.cpp`）
- `Wasami.PowerOrb.Parts`: 部品の値（結晶のメッシュ・材質・位置・拡縮・当たりなし、カプセルの 45.46 cm・中心・`OverlapAllDynamic`・Pawn の重なり・重なりの通知、灯の色・単位なし 1000・減衰 200・影なし、印の材質・20 m 上・拡縮・影なし・当たりなし）、`ShardSpawnTime` 150、テレキネシスのインターフェースを持たない、プレイヤーの地図がオーブのクラスを写す、出現点のビルボード（ルート・ゲーム中は隠れる・32 cm・`m_crystal_Inst3`）。
- `Wasami.PowerOrb.CollectEffect`: 演出の 4 本のトラック（書き出しのキーと接線から Python で計算した値）、橙のゲイン・閃光・`Range` 4000・球の色・ピッチ 2、Primal Fear は色を入れずピッチ 1。
- `Wasami.PowerOrb.Cycle`: 出現点 3 つ（と赤いシャードの点 1 つ。数えない）のテストのワールドで、150 s までは明滅しない → 明滅の間は置いた位置・次のタイマーなし・3 s で約 30 回反転・灯と印も一緒・カプセルは残る → 155 s の後は見えて出現点の 1 つにいて、移ってから 150 s → 2 巡しても出現点 → `MoveToSpawnPoint(1)`。0.05 s のティックは 150 s で少し遅れるので、見る時刻はタイマーから 0.5 s ずらす。
- `Wasami.PowerOrb.Collect`: ほかのキャラクターが触れても取れない、明滅で隠れている間にプレイヤーが触れると取れて消える、遠い敵も含めてインターフェースを持つ敵 2 体に `SetState(Stun, true)` が 1 回ずつ（タグだけのアクタには送らない）、演出が 1 つプレイヤーの位置に出て 2 s で消える。テストのワールドにはローカルプレイヤーが無いので、ENEMIES STUNNED は画面に出ない。
- `Wasami.BonusShard.Parts`: 部品の値（結晶 `soul_shard`・`m_crystal_Inst`・97.09 cm 上・20 倍、カプセルの 99.14 cm・中心、灯の色、印 `M_Bonus_Shard`・当たりなし）、`ID` 0、テレキネシスなし、地図が写す、演出の赤いゲイン・閃光・波の 0.5 とピッチ 1.5、敵（`AWasamiEnemy::SpawnEnemy`）の地図の印（カプセルの子・`Plane`・`M_Enemy`・カプセルの中心から (21.9, 0, 1000)・拡縮・影なし・当たりなし）。
- `Wasami.BonusShard.Save`: テストのワールドのゲームモードのセーブ（メモリの上だけ）の `BonusShards` が {1} のとき、`ID` 1 は 1 ティックで消え、`ID` 0 は残って 150 s のタイマー。`Delay 0` の前はタイマーが無い。
- `Wasami.BonusShard.Collect`: 出現点 1 つ（オーブの点は数えない）、`AWasamiPlayerCharacter`（コントローラの `SetPawn`）、敵 2 クラス（代役とナース）。取る前は地図にシャードだけ → 明滅の最中に触れて取る: アクタは残る・セーブに `ID`・結晶と印と灯が消える・演出が 1 つ・60 s の暴き・敵がすぐ地図に → 地図の作り直しの後も載る → 明滅の 5 s が終わる・演出が消える → 後から出た敵が 2.1 s 以内に載る → 60 s の直前はまだ、直後にシャードが消えて敵が地図から外れ、外れたまま。

## 既知の制約・注意点
- 結晶と閃光の材質・地図の印の色は推定で、本家と見比べていない（大目標 1・2 の決め方。作業一覧の項目 28 の後回しの一覧）。
- 赤いシャードは本家の結晶（`soul_shard` × 20・`m_crystal_Inst`）で、通常のシャードのワサミ餅には替えない（WebGL 版は餅にしたが、最終目標の「本家と同一の見た目にする」が優先。2026-09-19）。
- 地図の印（特殊シャードの 20 m 上、敵の 10 m 上の板）は本家と違って当たりを持たない（上の「共通の作り」）。
- 地図に敵を足すのはクラスごと（本家の `Add To Map(GetObjectClass)`）なので、サブクラスも含めてそのクラスの敵がみな載る。赤いシャードの 60 s の間は、敵の骨格メッシュも地図のキャプチャに写る（印が 10 m 上から覆う。本家も同じ）。
- 取得の画面のルートを区間（0.467 s）の後に平常の値（角度 −0.158°）へ戻す読みは、UE の既定と食い違う: ウィジェットのアニメは区間の後も最後の値（角度 0°・拡縮 1）を保つ（`KeepState`。18 記録の「内部構造」。2026-09-20 に分かった）。差は 0.16° で見た目は変わらないので直していない。直すなら `EvaluateCanvasAngle`・`EvaluateCanvasScale` の区間の後を区間の終わりの値にし、テスト `Wasami.VignetteSides.Anim` の「its own tilt after the section」を 0 にする。
- Zone 2 の祭壇の球 `m_crystal_Inst2` もここで作る（`CRYSTAL_INSTANCES`）。前処理が根 `m_crystal` を `crystal` に振り分け、ステージの取り込みは自分で作らずに `make_crystal` を呼ぶ（01 記録。2026-09-20、作業一覧の項目 31。それまではステージが汎用の `M_DD_Substance` で作り、白っぽかった）。`make_crystal` はマスター `M_DD_Crystal` から作り直すので、ステージの取り込みが結晶のインスタンスに当たるたびに走る（今は 1 つ）。

## 変更履歴
- 2026-09-21: `M_DD_Crystal` が `distortion_normal` を読むようにした（反射と屈折の軸になる正規化しない法線。作業一覧の項目 28 のステップ 13）
- 2026-09-20: `CRYSTAL_INSTANCES` に Zone 2 の祭壇の球 `m_crystal_Inst2` を足し、`make_crystal` が既にあるインスタンスの `base_property_overrides` を外すようにした（ステージの取り込みが呼ぶ。作業一覧の項目 31 のステップ 3）
- 2026-09-19: PIE で両ゾーンを確かめ、台本の通しを流した（上の「PIE での確かめ」。ステップ 6）
- 2026-09-19: 両ゾーンに本体と出現点を置いた（`dd_level._flow`。上の「配置」。ステップ 5）
- 2026-09-19: 赤いシャード `AWasamiBonusShard`（本家 `BP_BonusShard`）と取得の演出 `AWasamiBonusShardCollectEffect`、デバッグのコマンド `Wasami.BonusShard`、テスト `Wasami.BonusShard.*` を足した。オーブと同じ部品と周期を基底 `AWasamiSpecialShard` に移し（`SpawnPowerOrb` は `SpawnSpecialShard` に）、印の当たりを外した。ユニティビルドの塊が変わり、`WasamiStunCollectEffect.cpp`（Primal とぶつかった曲線のキーを `Stun*` に）と `WasamiVignetteSidesWidget.cpp`（連続回収の画面とぶつかった名前を `Sides*` に）の無名名前空間の名前を改めた（ステップ 4）
- 2026-09-19: オーブ `AWasamiPowerOrb`（本家 `BP_PowerOrb`）、取得の演出 `AWasamiStunCollectEffect`（`BP_StunCollectEffect`。Primal Fear と共有する球の基底 `AWasamiSphereBurst` を 04 記録に）、出現点 2 種、デバッグのコマンド `Wasami.PowerOrb`、テスト `Wasami.PowerOrb.*` を足した（ステップ 3）
- 2026-09-19: 初版（素材の取り込み `dd_specials.py` と `WasamiDDTools.import_dd_specials`。作業一覧の項目 10 のステップ 1）
- 2026-09-19: 取得の画面 `UWasamiVignetteSidesWidget`（本家 `UMG_VignetteSides`）とテスト `Wasami.VignetteSides.*` を足した（ステップ 2）
