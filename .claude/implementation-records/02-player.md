---
title: プレイヤーとゲームモード
sources:
  - Source/wasami_deception/WasamiGameMode.h
  - Source/wasami_deception/WasamiGameMode.cpp
  - Source/wasami_deception/WasamiPlayerCharacter.h
  - Source/wasami_deception/WasamiPlayerCharacter.cpp
updated: 2026-09-20
---

# プレイヤーとゲームモード

## 役割
本家 Dark Deception の `BP_DD_PlayerCharacter` に倣った一人称のプレイヤー。移動・視点・速さに連動する FOV・頭の揺れ・180° ターンと、手に持つタブレット（板の出し入れ、画面、ミニマップのシーンキャプチャ）、タブレットのパワー（`UWasamiPowerComponent`。中身は 04 記録）を持つ。`AWasamiGameMode` がそれを既定のポーンとして出し、タブレットの帯に出す目的の文を持つ。

## 公開インターフェース

- `AWasamiGameMode : AGameModeBase` — コンストラクタで `DefaultPawnClass = AWasamiPlayerCharacter::StaticClass()`。`Config/DefaultEngine.ini` の `GlobalDefaultGameMode` がこれを指す。`CurrentObjective`（FText、既定は空 = 本家の `BP_DD_GameMode` の `Current Objective` の既定）はタブレットの帯に出す目的。ゾーンの流れ（11 記録）が区間ごとに入れる。ほかにゲームの流れの受け持ち（BeginPlay の頭で設定を読んで当てる〈ゲームインスタンスの `CheckSettingsSave`。15 記録〉、BeginPlay でセーブを読むか作る、0.2 秒後に回収済みのシャードを消す、時間を数えるティック、`DeathEvent`・`OnDeath`、`SaveCheckpoint`）と、本家の Zone のレベル BP の受け持ち（`ChoosePlayerStart` でセーブのチェックポイントの PlayerStart から出す、`DeathEvent` で死亡画面を出してゲームを止める、`SaveCheckpoint` の SAVING PROGRESS、開いたときの黒からの明け、デバッグのコンソールコマンド `Wasami.Kill` ほか）と、シャードの `Check Shards`（全回収の通知）と `Check Streak`（連続回収。13 記録）、病院の `Escape`・`Finished Level`（脱出でスコア画面〈設定の難易度が EASY なら EASY MODE〉を出し、NEXT の後にタイトルへ。13 記録）、開始時にゾーンの流れ（`AWasamiZoneFlow`、11 記録）を出すことを持つ。レベルの名前の定数（`Zone1LevelName`・`Zone2LevelName`・`TitleLevelName` = `L_Title`）と、セーブのチェックポイントから続けるゾーンを選ぶ `LevelForCheckpoint(Checkpoint)`（本家の病院の入口 `06_Hospital` の `Spawn` @81063: 7〜10 は Zone 2、ほかは Zone 1。タイトルの RESUME が使う。14 記録）も持つ。タイトルのレベルは別のゲームモード `AWasamiTitleGameMode`（14 記録）。その中身は 06 記録の「ライフ・セーブ・死亡の受け口」「開始の場所・死亡画面・SAVING PROGRESS」「シャードの確かめ（`Check Shards`）」。
- `AWasamiPlayerCharacter : ACharacter`
  - `IsSprintOn()` / `IsTabletUp()`（BlueprintPure）、`ToggleTablet()` / `PutDownTablet()` / `ResizeMap()` / `SetMoveSpeeds(Walking, Sprinting)`（BlueprintCallable。`PutDownTablet` は本家の `Put Down Tablet`〈@31904〉: 上げていれば判定なしで下ろす〈woosh とカーブ〉。捕獲〈07 記録〉が呼ぶ。2 つの速さを書いて使う方を当てる。スピードブーストが使う。`StopSprinting()` は本家の `Sprinting?` を偽にするところ〈`BP_00_Teleport` の入り方〉: 押しと切り替えの両方の走りを消して歩きの速さを当てる。Zone 2 の脱出〈11 記録〉が呼ぶ）、`GetTabletScreen()`（画面のウィジェット。ウィジェットコンポーネントが作るまでは null）、`GetPowers()`。
  - `IsMapZoomedOut()`（Z で地図を引いているか。本家の `mapZoomedOut?`）、`GetCamera()`、`GetArrowPointer()`（地図の矢印。子のアクタができてから。03 記録）。
  - `AddToMap(Class)`・`RemoveFromMap(Class)`（本家の同名のイベント。そのクラスの今いる全アクタを地図に足す・外す。赤いシャードが敵を足す。16 記録）、`IsOnMap(Actor)`（キャプチャの `ShowOnlyActors` に入っているか。最後の作り直しの時点）。
  - `OnInteract`（`FSimpleMulticastDelegate`。C++ だけ）と `InteractPressed()`（それを流す。F が呼び、デバッグの `Wasami.Interact` も呼ぶ）。
  - `EscapePressed()`（Esc と、デバッグの `Wasami.Pause`〈`WasamiPauseWidget.cpp`〉が呼ぶ）: ゲームが止まっていなければポーズ画面 `UWasamiPauseWidget::Show`（Z 5。15 記録）。止まっていれば開かない（EASY でライフ 0 の死亡画面の上も。本家どおり。09 記録）。
  - 見て使う（本家の `Interact (Secondary)` と手のマーク。流れは 05 記録）: 定数 `InteractDistance` 200、`InteractSecondaryPressed()` / `InteractSecondaryReleased()`（左クリックの押し・離し）、`TraceInteract(FHitResult&)`（カメラから前 200 cm の Visibility の線のトレース）、`UpdateInteractWidget()`（ティックが呼ぶ）、`GetInteractWidget()`（`UWasamiInteractWidget`。`BeginPlay` で作る）。
  - 移動の値: `WalkingSpeed` 300、`SprintingSpeed` 600（cm/s）。
  - オプション: `bToggleSprint`、`MouseSensitivity` 1.0、`bInvertY`、`bHeadBob`（本家の OPTIONS の TOGGLE SPRINT / MOUSE SENSITIVITY / INVERTED Y AXIS / HEAD BOBBING）。`BeginPlay` でゲームインスタンスの設定から `ApplySettings(Settings)` で入れる（感度は設定 / 0.5。既定の設定で上の値になる）。`SetUpMouseSmoothing(Settings)` は本家の `Set Up Mouse Smoothing`（スプリングアームの回転ラグ 12.5 / 50。オプションの SAVE & EXIT だけが呼ぶ）。中身は 15 記録。
  - カメラ: `BaseFOV` 90、`FastFOV` 115、`FOVSpeedRange` (300, 900)、`FOVInterpSpeed` 0.5。
  - 頭の揺れ: `WalkShakeClass` / `RunShakeClass`（`TSoftClassPtr`。既定は `/Game/DD/Blueprints/Main/BP_DD_PlayerCharacter_WalkShake` と `_RunShake` の `_C`）。
  - タブレット: `bCanMove`（本家の `CanMove?`。false の間は移動・視点・ダッシュ・タブレットが止まる）、`bCanUseTablet`（`Can Use Tablet?`。タブレットとパワー）、`bHasInput`（`Has Input`。本家は台本の場面で切る。パワーが見る）、`bCanInteract`（`Can Interact?`。Q / E、左クリックの見て使う、手のマークが見る）、`ShardActorClass`（画面が数え、地図に写すシャードのクラス。既定は `AWasamiShard`〈06 記録〉。空なら 0 を出す）、`MinimapActorClasses`（地図がいつも写すほかのクラス。既定は特殊シャードのオーブ `AWasamiPowerOrb` と赤いシャード `AWasamiBonusShard`〈16 記録〉。本家の `Show Only` の一覧も `BP_PowerOrb`・`BP_BonusShard` をクラスで持つ）。
  - 素材（ソフト参照。`BeginPlay` で読む。00 記録の決まり）: `TabletMesh`（`/Game/DD/Meshes/Player/Tablet/tablet_new_pCube2`）、`MinimapTarget`（`/Game/DD/UI/Minimap/T_NewMap`）、`TabletUpSound`（`/Game/DD/Audio/SharedGameplay/05_Tablet_Woosh_v2_1`）、`TabletDownSound`（`_v1_1`）、`ResizeMapSound`（`/Game/DD/Audio/UI/UI_Select_V3`）。
  - コンポーネント: `Tablet`（板のスタティックメッシュ）、`TabletScreen`（`UWidgetComponent`、`UWasamiTabletWidget`）、`MinimapCapture`（`USceneCaptureComponent2D`）、`ArrowPointer`（`UChildActorComponent`、名前は本家の `BP_ArrowPointer`。地図の矢印 `AWasamiArrowPointer` を持つ。03 記録）、`Powers`（`UWasamiPowerComponent`）、`Chameleon`（名前は `FX`。`UWasamiChameleonComponent`、`GetChameleon()`。本家の子アクタ `FX` の Chameleon。本家は Z +2000・拡縮 (5,5,1) に置くが、範囲なしのボリュームなので位置は絵に関係せず、アクタコンポーネントにした。中身は 04 記録）。

## 内部構造と処理の流れ

- **体とカメラ**（コンストラクタ）: カプセルは半径 50・半高 88（本家の `CollisionCylinder` は半径 50、半高は `ACharacter` の既定 88）、歩ける斜面は 44°（`WalkableSlope_Increase`）。`USpringArmComponent` をカプセルに付け、相対位置 (0, 0, 95)、`TargetArmLength` 0、当たり判定なし、`bUsePawnControlRotation` true、回転ラグ 20（`CameraRotationLagSpeed`）、位置ラグの速さ 8。`UCameraComponent` をアームのソケットに付け、FOV 90。目の高さは床から約 183 cm。
- **入力**（`CreateInput`、`SetupPlayerInputComponent`）: Enhanced Input の `UInputAction` と `UInputMappingContext` を実行時に作る（アセットにしない）。
  - 移動: W / ↑（Swizzle で Y に）、S / ↓（Swizzle + Negate）、D / →、A / ←（Negate）。`Move` は操作の向き（ヨーだけ）の前と右へ `AddMovementInput`。
  - 視点: `EKeys::Mouse2D` に Smooth → FOVScaling（`FOVScale` 0.01111、`UE4_BackCompat`）の順で修飾子を付ける（UE4 のマウススムージングと FOV スケーリングと同じ）。感度 0.07（本家の `DefaultInput.ini` の `MouseX/Y`）は C++ では掛けず、`Config/DefaultInput.ini` の `AxisConfig`（Mouse2D 0.07）で効かせる: Enhanced Input の `ApplyAxisPropertyModifiers`（UE 5.8 `EnhancedInputSubsystemInterface.cpp`）が、マウスのキーの対応づけに旧入力の `AxisConfig` の感度を `UInputModifierScalar` として先頭に自動で足す（CVar `input.GlobalAxisConfigMode` の既定 0 = マウスだけ）。C++ でも Scalar を足すと 0.07² になり、視点が 1/14 の速さになる（2026-09-17 に直した。1 カウント 0.01225° → 0.175°）。自動の修飾子は Smooth より前に入るが、どれも値に比例するので順は結果を変えない。1 カウント 0.175°（= 0.07 × 感度 1 × FOV 90 × 0.01111 × 2.5）は最新版の実機で測った値と一致する（`observations/README.md` の「視点の速さと集中線」）。`Look` は `AddControllerYawInput` / `AddControllerPitchInput`（`bInvertY` でなければ Y を反転）。コントローラ側の 2.5 / −2.5 は `bEnableLegacyInputScales=True` により掛かる。
  - Shift（ダッシュ）、中クリック（180° ターン）、Space（タブレット）、Z（地図の拡縮）。
  - Q / E / 1 / 2（本家の `Use Power Left` / `Use Power Right` / `Cycle Power Left` / `Cycle Power Right`）は `Powers` の `UsePowerLeftPressed` / `UsePowerRightPressed` / `CyclePowerLeft` / `CyclePowerRight` に直に結ぶ（中身は 04 記録）。本家の `Use Power`（R）はどの BP も受けていないので割り当てない。
  - 左クリック（`IA_LeftMouseButton`、押した瞬間 = `Started`、離した瞬間 = `Completed`）とホイール（`IA_MouseWheelAxis`、`EKeys::MouseWheelAxis` の Axis1D、1 目盛り ±1）は、`LeftMousePressed` / `MouseWheel` から `Powers` の `ConfirmTeleport` / `AdjustTeleportDistance` へ渡す（テレポートの照準が出ているときだけ効く。04 記録）。本家では照準のアクタ（`BP_Power_Teleport`）がキーを直に受け、入力を消費しないので、同じクリックでプレイヤー自身の `Interact (Secondary)` も走る。本作も `LeftMousePressed` が `ConfirmTeleport` の後に `InteractSecondaryPressed` を呼び、`LeftMouseReleased` が `InteractSecondaryReleased` を呼ぶ（前方 200 cm の `InteractWithObject` / `StopInteractWithObject`。05 記録）。ホイールの Axis1D は値が 0 のフレームでは呼ばれないが、本家の毎フレームの軸の束縛も値が変わるフレームでしか結果が変わらないので同じ。どちらも `bCanMove` などの条件を見ない（本家の照準のアクタも見ない）。
  - Esc（`IA_Escape`、押した瞬間 = `Started`）は `EscapePressed`。本家の旧版はキャラクターが Esc を直に受け（`InpActEvt_Escape` @7758 → `CreateAndAddWidget(UMG_Pause, 5)`）、最新版はプレイヤーコントローラーが Esc とゲームパッドの Special Left で Z 1 に作る（`DD_PlayerController` @746）。どちらも条件は見ず、キーの結び付けが止まっている間は動かない（`bExecuteWhenPaused` 偽）ので、`EscapePressed` がゲームの止まりを見る（デバッグのコマンドも同じ道を通るよう、入力アクションは止まっている間も起こす `bTriggerWhenPaused`）。EASY でライフ 0 の死亡画面の上も開かない（本家は止まったまま抜け道が無く、本作も同じ。2026-09-20 のユーザーの回答「本家通り」。それまではそこでだけ開いていた。15 記録）。ポーズ画面が UI だけの入力の様式にするので、開いている間は Esc が届かない。PIE ではエディタが Esc で遊びを止めるので、確かめは `Wasami.Pause`。
  - F（`IA_Interact`、押した瞬間 = `Started`）は `InteractPressed` → `OnInteract`。本家の `Interact` はプレイヤー自身は受けず、扉の破壊（`BP_06_Hospital_DoorBreak`）など 6 つの BP が `AutoReceiveInput` でキーを直に受ける（入力は消費しない）。本作ではそれらのアクタが `OnInteract` を聞く（扉の破壊は 11 記録）。条件（`bCanMove` など）は見ない（本家のアクタも見ない）。ゲームパッドのキーはほかの操作と同じく割り当てない。
  - 本家の割り当ての全体（`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` の `ActionMappings` / `AxisMappings`。2026-09-16 に実機 v1.9.6 でも同じことを確認）:

    | 本家の操作 | キー | 本作 |
    | --- | --- | --- |
    | Forward / Left | W・S（−1）/ A（−1）・D | 同じ |
    | LookHorizontal / LookVertical | MouseX / MouseY | 同じ |
    | Sprint | LeftShift | 同じ |
    | Interact | F | `OnInteract`（受けるアクタが聞く。扉の破壊） |
    | Interact (Secondary) | 左クリック | 前方 200 cm の `InteractWithObject`（05 記録。テレポートの確定と同じクリック） |
    | （テレポートの照準のアクタがキーを直に受ける） | 左クリック / マウスホイール | 同じ（04 記録） |
    | Toggle Tablet | SpaceBar | 同じ |
    | Resize Map | Z | 同じ |
    | Use Power | R | 割り当てない（本家でも何もしない） |
    | Use Power Left / Right | Q / E | 同じ（04 記録） |
    | Cycle Power Left / Right | 1 / 2 | 同じ（04 記録） |
    | 180 Turn | 中クリック | 同じ |
    | Skip Cutscene | P | 未実装 |
    | Buy Upgrade | E | 未実装 |
- **速さ**（`ApplySpeed`）: ダッシュの有無で `SprintingSpeed` / `WalkingSpeed` を `MaxWalkSpeed` に入れる。加減速は UE の既定のまま（本家も `MaxWalkSpeed` しか上書きしていない）。スピードブーストは `SetMoveSpeeds` で 2 つの速さをどちらもブーストの速さにし、終わると 300 / 600 に戻す（本家と同じく、元の値ではなく定数に戻す。04 記録）。
- **ダッシュ**: Shift の押下で入り、離すと戻る。`bToggleSprint` のときは押すたびに反転（本家の TOGGLE SPRINT）。向きは問わない。
- **FOV**（`UpdateFOV`）: `BeginPlay` で 0.001 秒のループタイマーを張り、毎回 `FInterpTo(現在, MapRangeClamped(速さ, 300→900, 90→115), フレームの delta, 0.5)`。本家の `FOV Multiplier` と同じ仕組み（タイマーが 1 フレームに何度も呼ばれるので、追従の速さはフレームレートで変わる）。
- **頭の揺れ**（`UpdateHeadBob` / `StopHeadBob`、本家の `Update Bob`）: ダッシュの状態が変わったら止める。速さが 1 cm/s 以下なら止める。動いていて未再生なら、`bHeadBob` のときにダッシュかどうかでシェイクを 1 つ再生する。止めるときはブレンドアウトさせる（`StopAllInstancesOfCameraShake(..., false)`）。
- **180° ターン**（`TurnAround`）: 押した瞬間に `SetControlRotation(0, ヨー + 180, 0)`（ピッチは水平に戻る）。回って見えるのはスプリングアームの回転ラグによる。
- **素材の読み込み**（`BeginPlay` の最初）: `Tablet->SetStaticMesh(TabletMesh)`、`MinimapCapture->TextureTarget = MinimapTarget`、3 つの音と 2 つのシェイクを読み、非公開の `Loaded*` に持つ（`Tablet` は既定の Movable なので実行中に差し替えられる）。
- **タブレットを画面に留める**（`PlaceTablet`、`BeginPlay` の `SetTickGroup(TG_PostUpdateWork)`）: 毎フレーム、板の**ワールド変換を「これから描く視点」から置き直す**（`PlayerCameraManager` の `GetCameraLocation` / `GetCameraRotation` に、タイムラインが決めた視点空間の位置と回転を掛ける）。UE のカメラシェイクはカメラマネージャの視点にだけ掛かり、`UCameraComponent` は動かないので、板をカメラの子のままにすると歩くたびに**画面上でシェイクと逆に揺れる**。視点に直に置けば画面上で 1 px も動かない（ユーザーの指示。WebGL 版 10 記録の 2026-09-12 と同じ判断）。カメラマネージャが視点を決めた後に動かすため、`BeginPlay` でアクタのティックを `TG_PostUpdateWork` に移している。カメラマネージャが取れないとき、**ビューターゲットがプレイヤーでないとき**（捕獲の別室のカメラ。07 記録）はカメラコンポーネントの変換を使う（別室のカメラの前に下ろすタブレットが写らないように。本家はカメラの子なので同じになる）。
- **タブレットの構成**（コンストラクタ）: `Tablet` はカメラの子で（メッシュは `BeginPlay` で入れる）、当たり無し、`bSelfShadowOnly`、初期位置は伏せた状態 (35.39891, −21.994417, −39.701378)・回転 (P0, Y90, R180)。その子の `TabletScreen` は相対位置 (0, 0.8922737, 0.2078171)・回転 (P0, Y90, R0)・スケール (0.28, 0.024465779, 0.024465779)、World 空間、DrawSize 714 × 864、Masked かつ片面（本家が上書きしている `Widget3DPassThrough_Masked_OneSided` が選ばれる）。`MinimapCapture` はカプセルの子で (0, 0, 3000)・ピッチ −90、正射影 `OrthoWidth` 4000、`SCS_BaseColor`、`PRM_UseShowOnlyList`、ターゲットは `MinimapTarget`（`BeginPlay` で入れる）。`ArrowPointer` はメッシュ（`CharacterMesh0`）の子で (0, 0, 2000)・拡縮 (5, 5, 1)、子のアクタのクラスは `AWasamiArrowPointer`（本家の `BP_ArrowPointer_GEN_VARIABLE`）。`bCaptureEveryFrame` は false から始め、タブレットを上げるときに true、下ろし終わったときに false にする（本家は常に撮っているが、下ろしている間は画面が見えないので絵は変わらない。`.claude/guides/performance.md`）。
- **出し入れ**（`ToggleTablet` / `UpdateTablet` / `ApplyTabletInterp` / `PlaceTablet`）: Space。`bCanMove` と `bCanUseTablet` が両方 true のときだけ効く。上げるときは `05_Tablet_Woosh_v2_1`、下げるときは `05_Tablet_Woosh_v1_1` を音量 0.5・ピッチ 1.5 で鳴らし、それぞれのカーブを頭から再生する（途中で押し直しても頭から。本家の `PlayFromStart` と同じ）。毎フレーム `TabletRaiseCurve`（0.5 秒。0 → 0.3 s で 0.9〈接線 2.0〉→ 0.5 s で 1.0）か `TabletLowerCurve`（0.3 秒。1 → 0.2 s で 0.1〈接線 −0.4674788 / −1.3626982 / −3.1061733〉→ 0.3 s で 0）を評価して `TabletInterp` に入れ、`PlaceTablet` が視点空間の位置の Z を −39.701378 ↔ −4.321648 に線形補間、回転を (P0,Y90,R180) と (P0,Y90,R0) の最短の Slerp にする。板は隠さない（伏せると視界の下に出るだけ）。
- **地図の拡縮**（`ResizeMap`）: Z。タブレットが上がっているときだけ効き、`UI_Select_V3` を音量 0.5・ピッチ 4.0 で鳴らして `OrthoWidth` を 4000 ↔ 10000 で切り替える。
- **画面の更新**: `UpdateTablet` が毎フレーム、画面に「パワーが 1 つでもあるか」（`SetPowersVisible`）、左右の枠が指すパワー（`ShowSocketPowers`）、6 つのパワーのゲージ（`SetPowerPercent`）を渡し、枠の弾みとシャードの `Count Shake` を進め（`TickAnimations`）、帯にゲームモードの `CurrentObjective` を渡す（どれも値が変わったときだけ画面に書く。03 記録）。`UpdateTabletScreen` は 0.1 秒ごとのタイマーで、`RefreshMinimapContents`（タグ `dd_minimap` のアクタ・地図の矢印・`MinimapActorClasses` の各クラスのアクタ・`AddToMap` が足したアクタ・`ShardActorClass` のアクタを `ShowOnlyActors` に入れ直し、シャードの数を数える。本家の `Show Only` も `BP_ArrowPointer` を足す）を呼んでからシャード数を渡す。
- **地図に足す・外す**（`AddToMap` / `RemoveFromMap`）: 本家は呼ばれた時点のそのクラスの全アクタをキャプチャの `ShowOnlyActors` に `Add`（重複も足す）・`RemoveItem` する。本作は `ShowOnlyActors` を 0.1 秒ごとに作り直すので、足したアクタを弱い参照の一覧 `MapAddedActors` に持ち（重複は持たない。外すときは全部外れるので見た目は同じ）、作り直しのたびに足す（消えたアクタは一覧から落とす）。呼ばれたらすぐ作り直す。後から出たアクタは、もう一度呼ばれるまで載らない（本家どおり。赤いシャードが 1〜2 s ごとに呼び直す）。

## 原作データの根拠
- `pak_reference/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.json`: `Walking Speed` 300、`Sprinting Speed` 600、`CollisionCylinder` の半径 50 と歩ける斜面 44°、`SpringArm_GEN_VARIABLE`（`TargetArmLength` 0、`bDoCollisionTest` false、`bUsePawnControlRotation` true、`bEnableCameraRotationLag` true、`CameraRotationLagSpeed` 20、`CameraLagSpeed` 8、相対位置 (0, 0, 95)）、`Tablet_GEN_VARIABLE`・`Widget_GEN_VARIABLE`・`SceneCaptureComponent2D_GEN_VARIABLE` の各設定。
- タブレットの出し入れ: 同 BP のバイトコード `InpActEvt_Toggle Tablet`（@19575。`CanMove?` と `Can Use Tablet?` の判定、woosh の音量 0.5・ピッチ 1.5、`TabletInterp` / `Timeline_1` の `PlayFromStart`）、その更新（@24367〜@25154 の `RLerp` と `VLerp`）、タイムラインのカーブ `CurveFloat_1`（長さ 0.5）と `CurveFloat_1_2`（長さ 0.3）。地図の拡縮は @18388（`UI_Select_V3` 0.5 / 4.0、`OrthoWidth` 10000 ↔ 4000）。キー割り当ては `pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` の `Toggle Tablet` = SpaceBar、`Resize Map` = Z。
- 頭の揺れ: `BP_DD_PlayerCharacter_WalkShake`（ピッチ 振幅 0.2・周波数 12、上下 振幅 2・周波数 13・オフセット 0、ブレンドイン 1・アウト 0.5）と `_RunShake`（ピッチ 0.5 / 18、ヨー 0.2 / 10、上下 4 / 20）。`/Game/DD` に同じ値の `LegacyCameraShake` として作ってある（01 記録）。
- FOV の 90→115（`MapRangeClamped(Speed, 300, 900, 90, 115)` を 0.001 秒のタイマーで `FInterpTo` 0.5）、180° ターン、マウスの感度 0.07 と FOV スケーリング: WebGL 版の実装記録 05（`.claude/references/webgl/implementation-records/05-player-controller.md`）にバイトコードの根拠がある。

## 依存関係
- `EnhancedInput`（`UInputAction`、`UInputMappingContext`、修飾子 `UInputModifierSwizzleAxis` / `Negate` / `Smooth` / `FOVScaling`）。
- タブレットの画面は `UWasamiTabletWidget`（03 記録）、板・画面・地図の素材は `WasamiDDTools.import_dd_tablet` が作る `/Game/DD` のアセット（03 記録）、地図の板はレベルの組み立てが置く（01 記録）。`UMG`（`UWidgetComponent`）と `Engine`（`USceneCaptureComponent2D`、`FRichCurve`）。
- `UCameraShakeBase`（`/Game/DD` の `LegacyCameraShake` の Blueprint を `TSoftClassPtr` で読む）。
- パワーは `UWasamiPowerComponent`（04 記録）。コンポーネントはこのクラスの `bCanInteract`・`bCanUseTablet`・`bHasInput`・`IsTabletUp()`・`SetMoveSpeeds()`・`GetTabletScreen()`・`GetChameleon()` を使う。FX は `UWasamiChameleonComponent`（04 記録）。
- `WasamiAssets.h`（00 記録。ソフト参照の既定のパス）。
- `Config/DefaultEngine.ini` の `GlobalDefaultGameMode` と `Config/DefaultInput.ini`（00 記録）。

## 既知の制約・注意点
- `ULegacyCameraShake` は `MinimalAPI` なので C++ で派生できない。本家のシェイクは Blueprint として作っている（01 記録の `WasamiDDTools`）。
- 入力の実際の手触り（ダッシュ時の FOV の広がり、頭の揺れ、180°）はまだ確かめていない。エディタが背面にあるとティックが 3 fps ほどに落ち、リモートからの疑似操作では確かめられない（`.claude/guides/verification.md`）。2026-09-16 から `Tools/desktop.py` で対話デスクトップに入力を送れるので、PIE でも本家の実機でも同じ操作を送って比べられる（2026-09-17 から、エディタの開き直しと PIE に確認は要らない。`.claude/guides/verification.md`）。
- 本家はタブレットをカメラ → `Scene`(0, 0, −94.9577) → `Tablet` と繋いでいるが、タイムラインが入れる相対 Z（−39.70 → −4.32）はカメラ基準の値で、`Scene` を挟むと板はカメラの約 1 m 下に行き画面に映らない。原作の収録から測った画面上の位置（x 4.8〜31.8 %・y 30.3〜91.0 %。WebGL 版 10 記録）は、カメラ相対 (35.399, −21.994, −4.322) に置いた計算（x 4.6〜31.5 %・y 30.0〜91.5 %）と合うので、`Scene` は置かずカメラの直下に付けている。
- 本家の画面はシャードの数を 0.01 秒ごとに数え直す（`UMG_Tablet` の Construct のループ）。本作は 0.1 秒ごと（数は回収でしか変わらないので見た目は変わらない）。
- シャードを回収した瞬間の −1 と `Count Shake` は、シャードが画面に直接書く（06 記録）。0.1 秒ごとの数え直しは、破棄されたシャードを数えない。
- まだ無いもの: 足音。しゃがみは作らない（本家に無い）。
- 素材はソフト参照なので、`/Game/DD` が無い（パイプラインを回す前の）状態でもエディタは起動する。その場合、PIE で板・音・揺れが無いだけになる。

## 変更履歴
- 2026-09-20: ゲームモードに `IsNewStart()`（Zone 1 をセーブの 0 で開いた）と、ゾーンを見分けるレベル名 `LevelName`（空なら今のレベル。テスト用）を足した。中身は 06・11 記録（作業一覧の項目 30 のステップ 3）
- 2026-09-20: ゲームモードのデバッグのコンソールコマンドに `Wasami.ChapterPortal`（ステージ OP を出すだけ。09 記録）を足した（作業一覧の項目 30 のステップ 1）
- 2026-09-20: ゲームモードに `TakeFoundVoice()`（敵の発見の声を全体で 12 s に 1 回に絞る関門。`FoundVoiceGap`。中身は 07 記録の「声」）を足した（作業一覧の項目 20 のステップ 7）
- 2026-09-20: `EscapePressed` の EASY でライフ 0 の死亡画面の例外を外した（本家どおりの行き止まり。2026-09-20 のユーザーの回答。09・15 記録）
- 2026-09-19: 地図に足す・外す `AddToMap`・`RemoveFromMap`・`IsOnMap` を足し、`MinimapActorClasses` の既定に赤いシャード `AWasamiBonusShard` を足した（作業一覧の項目 10 のステップ 4。16 記録）
- 2026-09-19: 地図がいつも写すクラスの一覧 `MinimapActorClasses`（既定はオーブ `AWasamiPowerOrb`）を足した（作業一覧の項目 10 のステップ 3。16 記録）
- 2026-09-19: `Escape` がスコア画面に設定の難易度を渡すようにした（作業一覧の項目 18 のステップ 2。13・15 記録）
- 2026-09-19: ゲームモードの `BeginPlay` の頭で設定を読んで当てる（本家の `Check Settings Save` → `Set Settings`）ようにし、プレイヤーに `ApplySettings`（`BeginPlay` で設定の感度・Y 反転・頭の揺れ・ダッシュの切り替えを入れる）と `SetUpMouseSmoothing` を足した。中身は 15 記録（作業一覧の項目 18 のステップ 1）
- 2026-09-19: ゲームモードの `FinishedLevel` の後（脱出のスコア画面の NEXT）の行き先をタイトルにし、`GetLevelToOpen` とデバッグの `Wasami.Title` を足した。中身は 13・06 記録（作業一覧の項目 17 のステップ 4）
- 2026-09-19: ゲームモードに `TitleLevelName`（`L_Title`）と `LevelForCheckpoint` を足した（タイトルの RESUME。14 記録。作業一覧の項目 17 のステップ 3）
- 2026-09-19: `StopSprinting()` を足した（Zone 2 の脱出が入力と一緒に走りを止める。作業一覧の項目 13 のステップ 5b）
- 2026-09-19: 見て使う仕組みを足した: ティックの最初に手のマークの出し入れ（`UpdateInteractWidget`）、`BeginPlay` で手のマークのウィジェットを作る、左クリックの押しで `ConfirmTeleport` に続けて `InteractSecondaryPressed`、離しで `InteractSecondaryReleased`（05 記録。作業一覧の項目 13 のステップ 1）
- 2026-09-19: `PutDownTablet`（本家の `Put Down Tablet`）を足し、ビューターゲットがプレイヤーでない間はタブレットを自分のカメラに置くようにした（捕獲の別室。07 記録）
- 2026-09-18: 地図の矢印の子のアクタ `ArrowPointer`（`GetArrowPointer`）と、矢印が読む `IsMapZoomedOut`・`GetCamera` を足し、地図のキャプチャに矢印を写すようにした（03 記録。作業一覧の項目 6 のステップ 6）
- 2026-09-19: ゲームモードに脱出の `Escape`・`FinishedLevel`・`Wasami.Escape` を足した。中身は 13 記録（作業一覧の項目 14 のステップ 5）
- 2026-09-19: ゲームモードに連続回収 `CheckStreak`（`Check Shards` から毎回）・`Wasami.Streak N` を足した。中身は 13 記録（作業一覧の項目 14 のステップ 2）
- 2026-09-18: F（`IA_Interact`）と `OnInteract`・`InteractPressed`、ゲームモードのデバッグのコマンド `Wasami.Interact [N]`（F を N 回）を足した（扉の破壊が聞く。作業一覧の項目 6 のステップ 3b）
- 2026-09-18: 目的の既定を空にし（本家の既定。ゾーンの流れが入れる）、ゲームモードに `Check Shards` とゾーンの流れの生成を足した。中身は 06・11 記録（作業一覧の項目 6 のステップ 1）
- 2026-09-18: ゲームモードに本家の Zone のレベル BP の受け持ち（開始の場所・死亡画面・SAVING PROGRESS・黒からの明け・デバッグのコンソールコマンド）を足した。中身は 06 記録（作業一覧の項目 5 のステップ 5）
- 2026-09-18: ゲームモードにゲームの流れの受け持ち（セーブ・時間・死亡の受け口・チェックポイントの保存・回収済みのシャードの除去）を足した。中身は 06 記録（作業一覧の項目 5 のステップ 3）
- 2026-09-17: 視点の対応づけから Scalar 0.07 を外した（`AxisConfig` の 0.07 と重なって視点が遅すぎた。ユーザーの指摘、作業一覧の項目 2）
- 2026-09-17: `ShardActorClass` の既定を `AWasamiShard` にした。画面のアニメを進める呼び出しを `TickAnimations` に改めた（シャードの `Count Shake` も進める）
- 2026-09-16: 左クリックとホイールの入力を足し、テレポートの照準（04 記録）へ渡すようにした
- 2026-09-16: 本家の子アクタ `FX`（Chameleon）にあたる `UWasamiChameleonComponent` を足した（`GetChameleon()`。スピードブーストの画面の揺れが使う）
- 2026-09-16: スピードブーストを `UWasamiPowerComponent`（04 記録）へ移し、Q / E / 1 / 2 の入力、`bHasInput`・`bCanInteract`、`SetMoveSpeeds`・`GetTabletScreen` を足した。画面にはパワーの枠・ゲージ・弾みを渡すようにした。素材（板・地図のターゲット・音・揺れ）をソフト参照にして `BeginPlay` で読むようにした（コンストラクタで読むとエディタの起動時にルートに入り、パイプラインが作り直すとエディタが落ちる）
- 2026-09-16: 初版（移動・視点・FOV・頭の揺れ・180°・ブーストを記録）
- 2026-09-16: タブレット（板・画面・ミニマップのシーンキャプチャ、Space の出し入れと Z の拡縮）と、ゲームモードの `CurrentObjective`、`bCanMove` による移動・視点・ダッシュの停止を足した
- 2026-09-16: ミニマップのシーンキャプチャを、タブレットを上げている間だけ撮るようにした（性能のルール）
- 2026-09-16: 歩くとタブレットが揺れる件を直した（ユーザーの指摘）。板をカメラの子として置くのをやめ、`TG_PostUpdateWork` で毎フレーム、カメラマネージャの視点に直接置くようにした（`PlaceTablet`）
- 2026-09-19: デバッグの `Wasami.Capture [N]` の説明に、顔の 4 本目（N 3、1.15 s で死亡画面）を足した（中身は 06・07 記録。作業一覧の項目 24）
- 2026-09-19: `EscapePressed` が、EASY でライフ 0 の死亡画面の上では止まっていてもポーズ画面を開くようにした（09・15 記録。作業一覧の項目 18 のステップ 6b）
- 2026-09-19: Esc（`IA_Escape`）と `EscapePressed`（ポーズ画面を開く。止まっている間は開かない）、デバッグ `Wasami.Pause` を足した（15 記録。作業一覧の項目 18 のステップ 5）
