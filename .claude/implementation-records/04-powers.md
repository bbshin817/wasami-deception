---
title: タブレットのパワー（枠・入力・ゲージ・強化段階・スピードブースト）
sources:
  - Source/wasami_deception/WasamiPowerTypes.h
  - Source/wasami_deception/WasamiPowerTypes.cpp
  - Source/wasami_deception/WasamiPowerComponent.h
  - Source/wasami_deception/WasamiPowerComponent.cpp
  - Source/wasami_deception/WasamiEnemyInterface.h
  - Source/wasami_deception/WasamiTelekinesisInterface.h
  - Source/wasami_deception/Tests/WasamiPowerTests.cpp
updated: 2026-09-16
---

# タブレットのパワー

## 役割
本家 Dark Deception のタブレットのパワー 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）の土台。本家がプレイヤー（`BP_DD_PlayerCharacter`）・ゲージのアクタ（`BP_Powers`）・タブレットの枠（`UMG_TabletPowers`）に分けて持つものを、プレイヤーに付ける `UWasamiPowerComponent` 1 つにまとめる。いま中身まであるのはスピードブーストだけで、ほかの 5 種は枠に出て入力を受けるところまで（進捗記録 `.claude/progress/20260916-tablet-powers.md` のステップ 3〜10 で足す）。原作の調査は `.claude/references/powers/`。**テレポーテーションだけ `pak_reference`（旧版）、それ以外は `pak_reference_2`（最新版）に従う**（ユーザーの指示）。

## 公開インターフェース

### `WasamiPowerTypes.h`
- `EWasamiPower : uint8` … `SpeedBoost`・`Teleport`・`Telepathy`・`PrimalFear`・`Telekinesis`・`Vanish`・`None`（本家の `Enum_RingAltar_Skills` の並び）。`WasamiPowerCount` = 6。
- `FWasamiPowerSlot`（USTRUCT）… `Power`（既定 SpeedBoost）と `bAvailable`（既定 false）。本家の `Struct_Power`。
- `FWasamiPowerTuning` … 強化段階ごとの値（下の表）。`ForLevel(Level)`（0〜5 に丸める）、定数 `TeleportCooldown` 5・`VanishDuration` 15・`MaxLevel` 5。
- `FWasamiPowerGauge` … `BP_Powers` のタイムライン 1 本と、それがアイコンに書く `Percent`。`SetDelay(Seconds, bTeleport)`・`Stop()`・`Tick(DeltaSeconds)`・`IsPlaying()`・`Percent`。

### `UWasamiPowerComponent : UActorComponent`（`AWasamiPlayerCharacter` の `Powers`）
- 入力: `UsePowerLeftPressed()` / `UsePowerRightPressed()`（Q / E）、`CyclePower(bool bLeft)`（BlueprintCallable）と `CyclePowerLeft()` / `CyclePowerRight()`（1 / 2）。プレイヤーの入力が直に結ぶ（02 記録）。
- `UsePower(bool bLeft)`・`ResetPowers()`（BlueprintCallable）。
- 読み出し（BlueprintPure）: `GetSocketPower(bLeft)`（枠が解放済みの範囲の外なら `None`）、`GetGaugePercent(Power)`、`IsPowerAvailable(Power)`、`IsUsingPower(Power)`（本家の `Is Player Using Power ?`。`Active Powers` に入っているか）、`HasPowers()`、`GetUpgradeLevel(Power)`。C++ だけの `GetTuning(Power)`。
- `OnPowerUsed(EWasamiPower)`（BlueprintAssignable）… 本家の `UsedPower`（と、パワーごとの `UsedTelepathy` などをまとめたもの）。
- 設定（EditAnywhere）: `UnlockedPowers`（既定は 6 種すべてを並び順に）、`UpgradeLevel`（既定 5。0〜5）。
- 素材（ソフト参照。`BeginPlay` で読む。00 記録の決まり）: `RefillSound` `/Game/DD/Audio/UI/power_refilled`、`CycleSound` `/Game/DD/Audio/UI/UI_Select_V3`、`BoostSound` `/Game/DD/Audio/UI/Shard_Streak_Milestone_V5`、`BoostShakeClass` `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak`（`_C`）。

### `IWasamiEnemyInterface`（`UWasamiEnemyInterface`）
本家の `DD_EnemyInterface` のうちパワーに関わる 4 つ。どれも `BlueprintNativeEvent`（C++ の敵は `_Implementation` を上書きし、呼ぶ側は `IWasamiEnemyInterface::Execute_*`）。既定の中身は本家の `BP_DD_Character_Base` と同じ。
- `SetState(EWasamiEnemyState State, bool bByOrb)` … Primal Fear は `(Stun, false)`、パワーオーブは `(Stun, true)`。既定は何もしない。
- `GetState()` … 既定は `Patrol`。
- `PlayerVanish()` … Vanish を使った瞬間に全敵へ 1 回。既定は何もしない。
- `NoTelepathy()` … true ならテレパシーに映らない。既定は false。
- `EWasamiEnemyState : uint8` … `Patrol`・`Pursue`・`Stun`・`Teleport`（本家の `Enum_EnemyStates`。表示名の表で値 2 が欠けているが、値 2 が Stun として使われている）。

### `IWasamiTelekinesisInterface`（`UWasamiTelekinesisInterface`）
本家の `DD_TelekinesisInterface`。`Activate()`（`BlueprintNativeEvent`、既定は何もしない）。本家では `BP_Shard` が実装し、テレキネシスが届くとプレイヤーへ飛んで回収される。

## 内部構造と処理の流れ

### 枠と解放済みのパワー（`BeginPlay`）
- `Powers`（`FWasamiPowerSlot` の配列）を `UnlockedPowers` から、すべて使える状態で作る（本家の `Check` が `Power_Default` から未解放を外すのと同じ結果）。
- 左右の枠は `Powers` の添字 `LeftIndex` / `RightIndex` を持ち、**既定は左右とも 0**（本家の GameInstance の `Selected Power Left/Right` の既定。左右とも Speed Boost）。左右が同じパワーを指してもよい。
- タブレットの画面へは、プレイヤーが毎フレーム `GetSocketPower` と `GetGaugePercent` を渡す（02・03 記録）。

### Q / E（`UsePowerKey`）
`bCanInteract`（本家の `Can Interact?`）が偽なら何もしない。キーごとの DoOnce（`bLeftKeyClosed` / `bRightKeyClosed`）を通ったら `UsePower` を呼び、0.5 秒後に戻す。

### `UsePower(bLeft)`
1. プレイヤーの `bCanUseTablet` と `bHasInput` が両方真でなければ終わる（タブレットを構えている必要は無い）。
2. **枠を弾ませる**（`UWasamiTabletWidget::BounceSocket`）。使えるかの判定より前なので、使えないときも弾む。
3. 枠のパワーを取る（添字が範囲外なら構造体の既定 = 使えない）。`bAvailable` が偽なら終わる。**音も何も出さない**（最新版どおり。旧版は `power_not_ready` を鳴らしていた。ユーザーの回答）。
4. `Use Power` 側の DoOnce（`bUseClosed`）を通ったら、`OnPowerUsed` を出し、パワーごとの処理へ（いまは `SpeedBoost` だけ。ほかは何もしない）。
5. 最後に 0.5 秒の Delay で DoOnce を戻す。**連打防止は Q / E ごとと、発動全体の 2 段**。

### 1 / 2（`CyclePower`）
**タブレットを構えているとき**（`IsTabletUp()`）で、その側の `bCanCycleLeft` / `bCanCycleRight`（テレポートの照準中だけ偽になる。ステップ 4）が真のときだけ効く。解放済みが 1 つ以上なら `UI_Select_V3` を音量 1.5・ピッチ 2.0 で鳴らし、添字が末尾なら 0、そうでなければ `Clamp(添字 + 1, 0, 5)`。使えるかどうかにはよらない。

### Delay（`Delay`）
本家の Blueprint の `Delay` ノードは、数えている間にもう一度呼ばれても無視される。`FTimerManager` で「そのハンドルのタイマーが動いていなければ仕掛ける」として写す。原作の癖（死亡のリセットの後に古い Delay が残って早く切れるなど）もそのまま出る。

### ゲージ（`FWasamiPowerGauge`、`TickComponent`）
- 本家の `BP_Powers` のタイムライン（長さ 1 秒、0→1 の直線）。`SetDelay(D)` は再生速度を 1/D にし、FlipFlop で「終わりから逆再生（アイコンが 1→0）」と「頭から再生（0→1）」を交互に行う（FlipFlop の最初は逆再生）。**テレポートだけは FlipFlop を使わず、アイコンの値が 1 未満なら頭から、そうでなければ終わりから**。位置を動かすだけでは `Percent` は変わらず、次のティックから書く。端に着いたら止まる。
- `Stop()`（本家の `Stop <power> Timeline`）は止めて `Percent` を 1 にする。
- コンポーネントのティックで 6 本とも進める。
- 結果として、効果時間のあるパワー（Speed Boost・Telepathy・Vanish）は効果中に 1→0・再使用中に 0→1、ほか（Teleport・Primal・Telekinesis）は使った瞬間に 0.05 秒で 0・再使用中に 0→1 になる。

### 充填（`Refill`・`SetPowerAvailable`）
`power_refilled` を `PlaySound2D` の音量 0.5 で鳴らし、そのパワーの `bAvailable` を真にする。`SetPowerAvailable` は本家の `Array_Find({P, 逆の値})` → `Array_Set` と同じく、値が逆になっている最初の要素だけを書き換える。

### スピードブースト（`UseSpeedBoost` → `EndSpeedBoost` → `RefillSpeedBoost`）
- 使った瞬間: `Active Powers` に足す → `Shard_Streak_Milestone_V5` をプレイヤーの位置で鳴らす（`PlaySoundAtLocation`、音量・ピッチ 1.0、減衰なし。SoundWave 自体が音量 0.7・ピッチ 2.0 と同時発音 `NewSoundConcurrency` を持つ）→ 使えない状態にする → `BP_CameraShake_Streak` を倍率 1.0・`CameraLocal` で再生 → **歩きもダッシュも**ブーストの速さにする（`SetMoveSpeeds(速さ, 速さ)`）→ ゲージの `SetDelay(効果時間)` → `Delay(効果時間)` で終わりへ → 終わり用と充填用の DoOnce を開く。
- 終わり（DoOnce_3。開いているときだけ）: 閉じる → 速さを **300 / 600 に戻す**（本家は元の値ではなく定数を書く）→ `Active Powers` から外す → ゲージの `SetDelay(再使用)` → `Delay(再使用)` で充填へ。
- 充填（DoOnce_4。開いているときだけ）: 閉じて `Refill`。
- どちらの DoOnce も最初は閉じている（本家の Start Closed。@1211・@1241）ので、使う前にリセットしても何も起きない。
- Lv5 の値: 速さ 950 cm/s・効果 9.75 秒・再使用 7.5 秒（効果の後。合わせて 17.25 秒）。
- 本家の `UI Cooldown(True, 10)` の放送は作っていない（受ける `UMG_Tablet` の結び付け先のウィジェットが木に無く、見た目の効果が無いと見られる。調査 01 §4.4）。演出（`CameraAnim_SpeedBoost` の色調、`UMG_SpeedBoost`、Chameleon の放射ブラーと揺れ）はステップ 3。

### 死亡のリセット（`ResetPowers`。本家の `BP_Powers.Reset All Powers`）
1. スピードブースト: 終わりの処理（効果中なら即座に終わり、再使用のゲージが始まる）→ 充填の処理（使った後なら即座に充填）→ ゲージを止めて 1。
2. テレポート: ゲージを止めて 1（照準のアクタを消す・充填・`UsedTeleport` はテレポートの実装で足す）。
3. Primal Fear: ゲージを止めて 1 → 充填。
4. Vanish: ゲージを止めて 1 → 充填（ウィジェットを消す処理は Vanish の実装で足す）。
- **Primal と Vanish は、使っていなくても充填を通るので `power_refilled` が鳴る**（本家の `Reset Primal` = @37261: Push @6444 → @27788 の Gate の Close。`Reset Vanish` = @37737 も同じ形）。Telepathy と Telekinesis はリセットしない。
- 本家のクールダウン明けの Gate（Telepathy・Primal・Telekinesis・Vanish）は「Open の直後に Enter」（@6301〜@6444 ほか）なので、閉じても次の Enter の前に必ず開く＝素通しと同じ。Gate の状態は持たない。
- 呼ぶのは死亡画面（本家は `UMG_DeathScreen` の暗転の 2 秒後、生き返りの直前）。死亡はまだ無い（M5）。

## 強化段階の値（`FWasamiPowerTuning`。本家の `BP_DD_PlayerCharacter` の分岐）

| 段階 | ブースト速さ / 時間 / 再使用 | テレポート距離 | テレパシー時間 / 再使用 | Primal 半径 / 再使用 | テレキネシス半径 / 再使用 | Vanish 再使用 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 870 / 6.75 / 9.5 | 1000 | 5 / 8.5 | 1500 / 35 | 1750 / 10.5 | 30 |
| 1 | 870 / 6.75 / 9.5 | 1000 | 5 / 8.5 | 1500 / 35 | 2000 / 10.0 | 28 |
| 2 | 890 / 7.5 / 9.0 | 1125 | 6 / 8.0 | 2000 / 32 | 2250 / 9.5 | 24 |
| 3 | 910 / 8.25 / 8.5 | 1250 | 7 / 7.5 | 2500 / 29 | 2500 / 9.0 | 20 |
| 4 | 930 / 9.0 / 8.0 | 1375 | 8 / 7.0 | 3000 / 26 | 2750 / 8.5 | 18 |
| **5（本作）** | **950 / 9.75 / 7.5** | **1500** | **9 / 6.5** | **3500 / 23** | **3000 / 8.0** | **15** |

- テレポートの再使用は段階によらず 5 秒、Vanish の効果は 15 秒。
- 本作は祭壇が無いので全パワーを Lv5 に固定（ユーザーの回答）。本家は段階をパワーごとにセーブに持つが、本作は `UpgradeLevel` 1 つ。
- **スピードブーストの再使用は最新版のコードの値**（祭壇の表と旧版より 1 秒長い。ユーザーの回答）。

## 作るアセット
取り込みは `WasamiDDTools.import_dd_powers()`（`pipeline/dd_powers.py`、01 記録）。アイコンは `import_dd_tablet()`（03 記録）。

| パス | 中身 |
| --- | --- |
| `/Game/DD/Audio/UI/power_refilled` | 充填の音（1.515 秒、48 kHz） |
| `/Game/DD/Audio/UI/Shard_Streak_Milestone_V5` | ブーストの音（1.058 秒。SoundWave の Volume 0.7・Pitch 2.0・ConcurrencySet `NewSoundConcurrency`） |
| `/Game/DD/Audio/NewSoundConcurrency` | 同時発音（MaxCount 2・VolumeScale 0.5、ほかは既定） |
| `/Game/DD/UI/Menu/Streaks/BP_CameraShake_Streak` | ブーストのシェイク（`LegacyCameraShake`。振動 0.5 秒・ブレンドイン 0・アウト 0.25、回転 Pitch 0.25/30・Yaw 0.25/40・Roll 0.5/35、FOV 2.0/10） |

## 原作データの根拠
- 仕組みの全体と各値: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`（`Use Power` @29314〜、使えないときの @16335、DoOnce_11 @11731、スピードブースト @13628〜@16280、終わり @30、充填 @1206、Reset 系 @37067〜@37753）、`UI/BP_Powers.txt`（`Set Delay`・FlipFlop・`Reset All Powers`）、`UI/Tablet/UMG_TabletPowers.txt`（`Check`・`Cycle Power Left/Right`・`Update Powers`）。まとめは `.claude/references/powers/01-player-system.md`。
- `Has Input`・`Can Interact?`・`Can Use Tablet?`・`Can Cycle Left?/Right?` の既定（すべて true）: `pak_reference_2/_assets/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.json` の CDO。
- キー: `pak_reference_2/_raw/DDeception/Config/DefaultInput.ini`（`Use Power Left` Q、`Use Power Right` E、`Cycle Power Left` 1、`Cycle Power Right` 2、`Use Power` R は受ける BP が無い）。
- 敵とシャードのインターフェース: `pak_reference_2/_assets/DDeception/Content/Blueprints/Characters/Shared/DD_EnemyInterface.json`・`DD_TelekinesisInterface.json`、既定の中身は `BP_DD_Character_Base.txt`（調査 04 §4）。
- 素材: `pak_reference_2/_assets/DDeception/Content/Audio/UI/*.json`・`Audio/NewSoundConcurrency.json`・`UI/Menu/Streaks/BP_CameraShake_Streak.json`（旧版と同じ値。アイコン・音の ogg も両版で同一であることを突き合わせた）。

## 依存関係
- `AWasamiPlayerCharacter`（02 記録）: `bCanInteract`・`bCanUseTablet`・`bHasInput`・`IsTabletUp()`・`SetMoveSpeeds()`・`GetTabletScreen()`・コントローラのカメラマネージャ。プレイヤーがこのコンポーネントを作り、入力を結び、毎フレーム画面へ値を渡す。
- `UWasamiTabletWidget`（03 記録）: `BounceSocket`。
- `WasamiAssets.h`（00 記録）。
- エンジン: `FTimerManager`、`UGameplayStatics::PlaySound2D` / `PlaySoundAtLocation`、`APlayerCameraManager::StartCameraShake`。

## テスト（`Tests/WasamiPowerTests.cpp`）
`Automation RunTests Wasami.Powers`（3 件、2026-09-16 にすべて成功）。
- `Wasami.Powers.Gauge` … FlipFlop の交互の向き、途中の値（2 秒で 1 秒後 0.5 など）、端で止まる、`Stop` で 1、テレポートの向きの決まり方。
- `Wasami.Powers.Tuning` … Lv5 の値、段階の丸め、Lv0 のテレキネシス半径、Lv1 のブーストの再使用 9.5。
- `Wasami.Powers.SocketBounce` … 弾みのキーの値と、キーの間の値（0.1 秒で 1.19028）。

## 確かめたこと（2026-09-16、PIE、`L_Hospital_Zone1` の開始地点、ユーザーの了承のうえで `Tools/desktop.py` から入力）
- 始めは左右とも Speed Boost、6 種とも使える、ゲージはすべて 1。
- E: 歩き・ダッシュ・`MaxWalkSpeed` が 950 になり、`IsUsingPower(SpeedBoost)` が真、使えない状態、約 2 秒後のゲージが 0.796（1 − 2 / 9.75 = 0.795）。使ってから約 18 秒後（効果 9.75 秒と再使用 7.5 秒を過ぎた時点）に読むと、300 / 600・使える・ゲージ 1 に戻っていた。
- Q（左 = Speed Boost）で同じように効き、タブレットの左の枠のアイコンが扇形に灰色へ変わっていくのを撮った。
- タブレットを上げて 1 で左が Teleport に、2 で右が Teleport → Telepathy → … → Vanish → Speed Boost と巡回した。右の枠を 6 種すべてに切り替えて撮り、どのアイコンも出た。
- タブレットを下ろすと 1 / 2 は効かない。Q で Teleport（中身が未実装）を使っても使える状態のまま。
- ブースト中に `ResetPowers` を呼ぶと、即座に 300 / 600・使える・ゲージ 1 に戻った。
- PIE のログにこの仕組みの警告やエラーは無かった。

## 既知の制約・注意点
- **スピードブースト以外のパワーは中身が無い**（枠に出る・弾む・`OnPowerUsed` が出るだけで、使える状態は変わらない）。テレポートの照準中の `bCanCycleLeft/Right` も、まだ偽にする側がいない。
- 本家は `Check` を `Delay 0.2` の後に行うが、本作は `BeginPlay` ですぐ作る。
- 本家の `Power` 配列は解放フラグ（セーブ）から作るが、本作はすべて解放済み（`UnlockedPowers`）。選んだ枠をレベルをまたいで残す本家の仕組み（GameInstance）は、ステージが病院だけなので作っていない。
- 本家のパワーの放送（`UsedTeleportPower`・`UsedPrimal` など）を購読するのは本家のチュートリアルや台本のレベルだけで、病院には無い。本作は `OnPowerUsed` 1 つにまとめた。
- ゲームパッドの割り当て（LT / RT / LB / RB）はまだ入れていない（プレイヤーの入力がキーボードとマウスだけのため）。
- 音と揺れはユーザーのスピーカーと画面で確かめていない（PIE の確認は値と絵）。

## 変更履歴
- 2026-09-16: 初版（パワーの土台: 枠・Q/E/1/2・2 段の連打防止・ゲージ・強化段階の表・死亡のリセット・スピードブースト、敵とシャードのインターフェース、テスト）
