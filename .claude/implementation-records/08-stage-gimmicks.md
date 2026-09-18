---
title: ステージの仕掛け（両開き扉）
sources:
  - Source/wasami_deception/WasamiDoubleDoors.h
  - Source/wasami_deception/WasamiDoubleDoors.cpp
  - Source/wasami_deception/Tests/WasamiDoubleDoorsTests.cpp
  - Content/Python/wasami_tools/pipeline/dd_gimmicks.py
updated: 2026-09-18
---

# ステージの仕掛け（両開き扉）

## 役割
病院のステージで動く仕掛け。いまは本家の両開き扉 `BP_06_DoubleDoors`（`pak_reference_2` の `Blueprints/06_Hospital`）を写した `AWasamiDoubleDoors` だけ: 幅 400 cm の出入口の両端に蝶番のある扉 2 枚で、キャラクター（プレイヤーかナース）が前の箱に入ると奥へ、後ろの箱に入ると手前へ、音とともに 1 s で 90° 開き、両側を覆う `Leave` の箱からキャラクターが出て、プレイヤーが中に残っていなければ閉じる。閉ざされている（`bLocked`）と、入ってもガタつく音（2 s に 1 回まで）だけ。ゾーンの流れ（11 記録）が名指しする扉を `Lock`・`Unlock`・`Open Front`・`Force Close` で閉ざし・開ける。作業一覧の項目 6（ゾーンの進行）のステップ 3c で、流れが名指しする Zone 1 の 2 枚のために作った（残りの 60 枚を置くのは項目 8、Zone 2 の 1 枚は項目 13）。

## 公開インターフェース
- `AWasamiDoubleDoors`（`AActor`）
  - `OnOpen`（動的マルチキャスト。本家の `Open`: どちらかの側が開いた）、`OpenAmount`（本家の `Open Amount`、90°）、`bLocked`（流れが直に書く）。
  - `Lock()`・`Unlock()`・`OpenFront()`・`ForceClose()`・`UpdateAnimationSpeed(Speed)`（両方のタイムラインの速さ。病院では呼ぶ所が無い）。
  - `NotifyFrontEnter(Other)`・`NotifyBackEnter(Other)`・`NotifyLeave(Other)` … 前の箱に入った・後ろの箱に入った・`Leave` から出た（本家の 3 つの部品のイベント）。箱の重なりが呼ぶ。テストは直接呼ぶ。
  - 読むだけ: `AnySideOpen()`・`IsOpenFront()`・`IsOpenBack()`・`IsOpening()`・`IsClosing()`（タイムラインが走っているか）・`IsLockedSoundBlocked()`・部品 `GetStaticMesh()`・`GetStaticMesh1()`・`GetFrontEnter()`・`GetBackEnter()`・`GetLeave()`。
  - 静的: `EvaluateSwing(Seconds)`（タイムラインの Float の曲線）、定数 `SwingLength` 1・`CloseSoundTime` 0.5922・`LockedSoundDelay` 2。

## 内部構造と処理の流れ
- 部品（本家の SCS）: ルート `DefaultSceneRoot`、`StaticMesh`（x −200）・`StaticMesh1`（x +200）は `UStaticMeshComponent` の既定（BlockAllDynamic・Movable）でナビゲーションに入らない。`FrontEnter`（(0, −150, 50)・拡縮 (6.2587, 2.8200, 1)）・`BackEnter`（(0, 150, 50)・(6.2587, 2.9723, 1)）・`Leave`（(0, 0, 50)・(6.2587, 12.0652, 1)）は `UBoxComponent` の既定（32 cm・OverlapAllDynamic・ゲームで隠れる）を拡縮した箱（約 400 × 180、400 × 190、400 × 772 cm）。箱の `AreaClass`（NavArea_Obstacle）は `bCanEverAffectNavigation` 偽なので効かない。クラスのタグ `interact`（プレイヤーの「使えるもの」を見る処理が読む。項目 5 の視線の手）。
- **メッシュはクラスが入れない**（`/Game/DD` の素材はコンストラクタで読まない。`WasamiAssets.h`）。レベルの組み立て（`dd_level._flow`、01 記録）が、本家の SCS の `hospital_entrance_walkway_doubledoor2`（`StaticMesh`）・`doubledoor1`（`StaticMesh1`）と、そのメッシュの材質（`M_06_Hospital_Door_01`・`MM_Main_Substance_Glass_Doors`）を置いた扉に入れる。メッシュはステージの素材（complex-as-simple の当たり）。
- 前の箱に入る（本家 @475）: 相手を覚え（`Unlock` が使う）、`ACharacter` なら @550: 閉ざされていれば `LockedSound`、どちらかが開いていれば何もしない、そうでなければ `bOpenFront` 真・`Animation` 真 → `OpenDoor` → `OnOpen`。`OpenFront` は @550 から。
- 後ろの箱に入る（@1143）: `ACharacter` でなければ何もしない。閉ざされていれば `LockedSound`、開いていれば何もしない、そうでなければ `bOpenBack` 真・`Animation` 偽 → `OpenDoor` → `OnOpen`。
- `Leave` から出る（@889）: 相手を覚え（`Lock` が使う）、`ACharacter` で、`Player Overlapping?`（`Leave` に重なるプレイヤー。閉ざされていれば常に偽）が偽なら @1054: どちらかが開いていれば、開いていた側を偽にし（前なら `Animation` 真、後ろなら偽）`CloseDoor`。`ForceClose` は @1054 から。UE は重なりの一覧から外してから終わりのイベントを流すので、出ていくプレイヤーは数えない。
- `Lock`（@1333）: `bLocked` 真 → `Leave` から最後に出た相手で @889（閉ざしたので `Player Overlapping?` は偽: 相手がキャラクターなら開いた扉が閉じる。まだ誰も出ていなければ何もしない）。`Unlock`（@1359）: `bLocked` 偽 → 前の箱に最後に入った相手で @475（キャラクターなら前から開く）。**本家はこの 2 つで前のイベントの引数（ユーバーグラフの持続フレーム）をそのまま使う**ので、弱い参照で覚えて同じにした。
- `OpenDoor`（@665）: `PlaySoundAtLocation(SFX_06_DoubleDoor_Open, アクタの位置, 回転 0, 0.5, 1.2, 0, MonkeyAttenuation)` → `Timeline_0` を頭から。`CloseDoor`（@766）: 両方の扉の当たりを QueryAndPhysics（既定のまま。病院で当たりを切る所は無い）→ `Timeline_1` を頭から。
- `LockedSound`（@253）: DoOnce。`PlaySoundAtLocation(Locked_Door, アクタの位置, 回転 0, 0.5, 1.5, 0, 01_Lobby_Attenuation)` → 2 s の Delay で DoOnce を開け直す（タイマー）。
- タイムライン（`Timeline_0` 開く・`Timeline_1` 閉じる。どちらも 1 s、同じ Float の曲線 0 → 0.951（0.552 s）→ 1.040（0.685 s）→ 1（1 s）。書き出しのキーと UE 4.24 が出した接線のまま、`RCTM_Break` で入れて UE 5 に接線を計算し直させない）。UE の `FTimeline` を写した自前の小さな状態（位置と再生中か）をアクタのティックで進める（走っている間だけティックする）: `PlayFromStart` は位置 0 で更新を 1 回（イベントは出さない）してから再生。ティックで位置を `Δt × 速さ` 進め、1 を超えたら 1 で止め、閉じる側は `Sound` のキー（0.5922 s）が [前の位置, 新しい位置)（最後のティックは終わりを少し越す）に入ったら `PlaySoundAtLocation(SFX_06_DoubleDoor_Close, …, 0.5, 1.0, 0, MonkeyAttenuation)`、そして `Update Func`。両方が走るときは開く側 → 閉じる側の順（閉じる側が勝つ）。
- `Update Func`（開く / 閉じる、曲線の値 α）: 振れ幅 = `Animation` ? `OpenAmount` : −`OpenAmount`。開くときは `Lerp(0, 振れ幅, α)`、閉じるときは `Lerp(振れ幅, 0, α)` を `Apply Update Values`: `StaticMesh` の相対回転をヨー v、`StaticMesh1` を −v（前から開くと 2 枚とも +Y 側 = 後ろへ振れる）。

## 作るアセット
`pipeline/dd_gimmicks.py`（`WasamiDDTools.import_dd_gimmicks`。01 記録）: `/Game/DD/Audio/06_Hospital/SFX_06_DoubleDoor_Open`（0.622 s）・`SFX_06_DoubleDoor_Close`（1.007 s）、`/Game/DD/Audio/01_Hotel/Locked_Door`（SoundCue。Random に `Locked_Door_v1`・`_v2`、減衰 `MonkeyAttenuation`、1.343 s）とその波形 2、減衰 `/Game/DD/Audio/Misc/MonkeyAttenuation`・`/Game/DD/Audio/01_Hotel/01_Lobby_Attenuation`。扉のメッシュと材質はステージの素材。

## 原作データの根拠
- `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_DoubleDoors.txt`（上の番地）と `_assets/…/BP_06_DoubleDoors.json`（SCS の部品の位置・拡縮・メッシュ、`Open Amount` 90、タグ `interact`、`Timeline_0_Template`・`Timeline_1_Template`〈長さ 1、`CurveFloat_0_1`・`CurveFloat_0_1_3` のキー、`Sound` のイベントのキー `CurveFloat_0`〉、部品のイベントの結び付け `ComponentDelegateBinding_0`）。
- 音: `_assets/…/Audio/06_Hospital/SFX_06_DoubleDoor_Open.json`・`_Close.json`、`Audio/01_Hotel/Locked_Door.json`（Random・重み 1 × 2・`AttenuationSettings` `MonkeyAttenuation`）、`Audio/Misc/MonkeyAttenuation.json`。
- 置き場所と値: `pak_reference_2/_levels/06_Hospital_Zone_01.full.json`（前処理の `stage_ue.json` の `actors`。Zone 1 に 62 枚、Zone 2 に 1 枚〈`bLocked` 真〉）。子の `BP_06_DoubleDoors_Child`（1 枚扉 `hospital_entrance_walkway_singledoor`・箱の位置違い）は病院に置かれていない。

## 依存関係
- 自前: `AWasamiPlayerCharacter`（`Player Overlapping?` の相手のクラス。02 記録）、`WasamiAssets.h`。
- 使う側: ゾーンの流れ（`AWasamiZoneFlow::DoubleDoors`・`AWasamiZone1Flow` の 04 と 06。11 記録）、レベルの組み立て（`dd_level._flow`。01 記録）。
- エンジン: `UBoxComponent`・`UStaticMeshComponent`・`FRichCurve`・`UGameplayStatics::PlaySoundAtLocation`・`FTimerManager`。

## 既知の制約・注意点
- 置いてあるのは流れが名指しする Zone 1 の 2 枚だけ（`BP_06_DoubleDoors11`・`BP_06_DoubleDoors33_36`）。ほかの出入口は扉が無く、通り抜けられる（項目 8）。
- 扉は当たりを持ったまま掃引せずに回る（本家どおり）。開くときにプレイヤーが扉の振れる範囲（蝶番から 200 cm）にいると、扉がカプセルに食い込むことがある。
- `Unlock`・`Lock` は前のイベントの相手で本家の処理を繰り返す（上）。流れは `On04DoorBreak` で `Unlock` を使わず `bLocked` を直に書く（本家どおり）。
- 両方のタイムラインが同時に走ったときの勝ち方（閉じる側が後）は、本家では部品のティックの順で決まり、コードからは確定できない。

## テスト（`Tests/WasamiDoubleDoorsTests.cpp`）
`Wasami.DoubleDoors.Actor`: 曲線のキー（0・0.951・1.040・1）、部品の値（`interact`・90°・扉の位置と当たり・箱の大きさと位置と Pawn の重なり）、キャラクターでないと開かない、前から開いて 0.5 s で曲線どおり・1 s で 90°（2 枚は逆向き）、開いている間は後ろから開かない、`Leave` を出ると閉じ始めて 1 s で 0、後ろから開くと −90° / +90°、`Force Close`、閉ざすとガタつくだけ・2 s で次が鳴らせる・`Unlock` で前にいた者のために開く、`Lock` で開いた扉が閉じる、閉ざしたまま `Open Front` は開かない・`bLocked` 偽なら開く、`Update Animation Speed(2)` で 0.5 s で閉じる。テストのワールドの 0 秒のティックは `MinUndilatedFrameTime`（0.5 ms）進むので、途中の角度は 0.5005 s の値と比べる。流れから閉ざし・開けるのは `Wasami.ZoneFlow.Zone1`（11 記録）。

## 確かめたこと（2026-09-18、PIE）
Zone 1 の 04: エレベーターの前の `BP_06_DoubleDoors11` は赤い 2 枚扉で閉じている → 鍵が外れると手前へ 1 s で開き、少し行き過ぎて戻る → `Leave` の外へ出ると閉じる（11 記録の「確かめたこと」）。

## 変更履歴
- 2026-09-18: 初版。本家の `BP_06_DoubleDoors` を `AWasamiDoubleDoors` に写し、音の取り込み `dd_gimmicks.py` を足した（作業一覧の項目 6 のステップ 3c）
