---
title: 見て使う（視線の手のマークと左クリック）
sources:
  - Source/wasami_deception/WasamiInteractable.h
  - Source/wasami_deception/WasamiInteractWidget.h
  - Source/wasami_deception/WasamiInteractWidget.cpp
  - Source/wasami_deception/Tests/WasamiInteractTests.cpp
  - Source/wasami_deception/Tests/WasamiTestInteractable.h
  - Source/wasami_deception/Tests/WasamiTestInteractable.cpp
updated: 2026-09-19
---

# 見て使う（視線の手のマークと左クリック）

## 役割
本家 Dark Deception の「見て使う」仕組みを写したもの。プレイヤーがカメラの前 200 cm に使える物を捉えている間、画面の中央に手のマーク（本家の `UMG_Interact`）を出し、左クリック（本家の `Interact (Secondary)`）でその物の `InteractWithObject` を呼ぶ。使える物はインターフェース `IWasamiInteractable`（本家の `BP_InteractInterface`）を持つアクタ。作業一覧の項目 13 のステップ 1 で作った。プレイヤー側の処理（トレース・ティック・左クリック）は `AWasamiPlayerCharacter`（02 記録）にあり、この記録で流れをまとめる。使う側（障壁の拒否・祭壇・欠片）は項目 13 の後のステップで足す（08・11 記録）。

## 公開インターフェース
- `IWasamiInteractable`（`UINTERFACE(BlueprintType)`）: `InteractWithObject(AActor* Interactee)`（BlueprintNativeEvent。`Interactee` はプレイヤー。本家の引数も Actor）、`StopInteractWithObject()`（同。クリックを離した）。既定はどちらも何もしない。本家の 3 つ目の `Use` は手に持つ物のもので、本作に手に持つ物は無いので持たない。
- `UWasamiInteractWidget : UUserWidget`: `GetImage()`（`Image_18`）。ソフト参照 `IconTexture`（`/Game/DD/UI/Main/interact_icon_03`）。
- プレイヤー（`AWasamiPlayerCharacter`、02 記録）: 定数 `InteractDistance` 200、`InteractSecondaryPressed()`・`InteractSecondaryReleased()`（左クリックの押し・離し。テストが直接呼ぶ）、`TraceInteract(FHitResult&)`、`UpdateInteractWidget()`（ティックが呼ぶ）、`GetInteractWidget()`。条件は `bCanInteract`（本家の `Can Interact?`）。

## 内部構造と処理の流れ
- **トレース**（`TraceInteract`）: カメラコンポーネントの位置から前方ベクトル × 200 cm まで、`ECC_Visibility` の線のトレース（単純な当たり、プレイヤー自身を除く）。本家の `LineTraceSingle(Self, カメラの位置, + 前 × 200, Visibility, bTraceComplex 偽, 無視なし, 描かない, bIgnoreSelf 真)`。
- **手のマーク**（`UpdateInteractWidget`、`Tick` の最初）: `bCanInteract` が偽なら何もしない（手のマークはそのときの見え方のまま。本家のティックもこの分岐ごと飛ばす）。真ならトレースし、当たって当たったアクタが有効で、当たった部品にタグ `interact` があれば `SelfHitTestInvisible`（本家の 3）、それ以外は `Collapsed`（本家の 2）。見るのは**部品のタグ**で、アクタがインターフェースを持つかは見ない（本家どおり。使える物は使える部品にだけタグを付ける）。
- **ウィジェット**（`BeginPlay`）: `CreateWidget(World, UWasamiInteractWidget)`（本家の `Create(Self, UMG_Interact, None)`: 持ち主のプレイヤーなし）、ゲームのビューポートがあれば `AddToViewport(0)`（テストの世界には無いので足さない）、`Collapsed`。
- **左クリック**（`LeftMousePressed`）: 先にテレポートの照準へ `ConfirmTeleport`（04 記録）、続けて `InteractSecondaryPressed`。本家では照準のアクタ（`BP_Power_Teleport`）がキーを直に受けて入力を消費せず、プレイヤーの `Interact (Secondary)` も同じクリックで走るので、照準の有無で分けない（照準のアクタの入力はポーンより上に積まれるので先）。
- **押したとき**（`InteractSecondaryPressed`、本家の `InpActEvt_Interact (Secondary)` の押し @24464）: `bCanInteract` が偽なら何もしない。手に持つ物の `Use` は無いので飛ばす。トレースし、当たったアクタを `InteractHitActor` に残し（当たらなければ null）、それが `IWasamiInteractable` なら `Execute_InteractWithObject(Actor, this)`。
- **離したとき**（`InteractSecondaryReleased`、本家の離し @24161）: 新しくトレースせず、最後の押しの当たり（`InteractHitActor`）が `IWasamiInteractable` なら `Execute_StopInteractWithObject`。`bCanInteract` は見ない（本家どおり。押しが `Can Interact?` で止められても、前の押しの当たりへ送る）。
- **手のマークの木**（`RebuildWidget`、本家の `UMG_Interact` の木とスロット）: `CanvasPanel_0`（`HitTestInvisible`）→ `Image_18`（ブラシ `interact_icon_03`・90 × 90、色 (1, 1, 1, 0.5)、RenderTransform の Scale (0.5, 0.5)、`HitTestInvisible`）。スロットはアンカー (0.5, 0.5)・Alignment (0.5, 0.5)・Offsets (0, 0, 100, 40)・`bAutoSize`（大きさは絵の 90 × 90 で、Offsets の右と下は効かない。書き出しで既定から変えてあるのは下の 40 だけ）。画面の中央に 45 × 45 の見た目で出る。

## 作るアセット
- `/Game/DD/UI/Main/interact_icon_03`（テクスチャ、90 × 90、`TEXTUREGROUP_UI`。`pak_reference_2` の `_textures.json` の sRGB・圧縮・LOD グループ）: `dd_ui.import_interact()`（`import_all` も呼ぶ。ツールセット `WasamiDDTools.import_dd_ui`、01・09 記録）が取り込む。原作から作り直せるので git の外。

## 原作データの根拠
- プレイヤー: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`（`python Tools/dd/bp_flow.py … <イベント名>`）。`ReceiveBeginPlay` の @28293（`Create(Self, UMG_Interact_C, None)`・`AddToViewport(0)`・`SetVisibility(2)`）、`ReceiveTick` の @30390〜31349（`Can Interact?` → トレース → `ComponentHasTag('interact')` → 3 / 2）、`InpActEvt_Interact (Secondary)_K2Node_InputActionEvent_0`（押し、@24464）と `_1`（離し、@24161）。`Can Interact?` の既定（真）は同じ BP の CDO。
- キー: `pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` の `Interact (Secondary)` = `LeftMouseButton`（ゲームパッドの `Gamepad_FaceButton_Right` はほかの操作と同じく割り当てない）。
- インターフェース: `pak_reference_2/_assets/DDeception/Content/Blueprints/BP_InteractInterface.json`（関数 `InteractWithObject(Interactee: Actor)`・`StopInteractWithObject`・`Use`）。病院の実装は `BP_ZoneBarrier`・`BP_01_Statue`・`BP_08_RingPiece_NoPickup`・`BP_06_NurseInteract_Intro`・`BP_FakeUseActor_06_HospitalZone1_Elevator` など。`StopInteractWithObject` はどれも空。
- 手のマーク: `pak_reference_2/_assets/DDeception/Content/Blueprints/UMG/UMG_Interact.json`。絵は `pak_reference_2/DDeception/Content/UI/Main/interact_icon_03.png`。

## 依存関係
- プレイヤー `AWasamiPlayerCharacter`（02 記録）がトレース・ティック・左クリックを持つ。左クリックはテレポートの照準（`UWasamiPowerComponent::ConfirmTeleport`、04 記録）と分け合う。
- 素材の取り込み `dd_ui.import_interact`（01・09 記録）。
- エンジン: `UWorld::LineTraceSingleByChannel`、`UPrimitiveComponent::ComponentHasTag`、`UUserWidget`・`UCanvasPanel`・`UImage`、`UInterface`。

## 既知の制約・注意点
- タグ `interact` を持つ部品はまだ無い（障壁・祭壇が項目 13 のステップ 2・3 で付ける）。レベルの組み立ては本家の部品のタグを写さないので、本家のレベルの静的メッシュに `interact` があっても手のマークは出ない。
- テスト用のアクタ `AWasamiTestInteractable`（`Tests/`。100 cm の BlockAll の箱で、部品にタグ `interact`、呼ばれた数を数える）は、ゲームのモジュールに入るがレベルには置かない（04 記録の `AWasamiTestEnemy` と同じ扱い）。
- テスト: `Wasami.Interact.Widget`（`UMG_Interact` の木とスロット）、`Wasami.Interact.Trace`（手のマークの出し入れ〈タグ・`Can Interact?`・200 cm〉、押しの `InteractWithObject` と離しの `StopInteractWithObject`、`Can Interact?` が偽の押しでも離しは前の当たりへ）。

## 変更履歴
- 2026-09-19: 初版。インターフェース `IWasamiInteractable`、手のマーク `UWasamiInteractWidget`、プレイヤーのトレース・ティック・左クリック（02 記録）、素材の取り込み `dd_ui.import_interact`、テスト 2 本（作業一覧の項目 13 のステップ 1）
