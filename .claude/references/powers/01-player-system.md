# 原作調査: プレイヤー側の「パワーの仕組み」（pak_reference_2 = 最新版 UE 4.24）

調査日 2026-09-16。読み取りだけで、リポジトリは変更していない。

## 表記の約束

- **PC** = `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`。`@数字` は、特に断らない限り `ExecuteUbergraph_BP_DD_PlayerCharacter` の中のオフセット。
- **PW** = `pak_reference_2/_bytecode/DDeception/Content/UI/BP_Powers.txt`（`ExecuteUbergraph_BP_Powers` の中のオフセット）
- **TP** = `pak_reference_2/_bytecode/DDeception/Content/UI/Tablet/UMG_TabletPowers.txt`（`ExecuteUbergraph_UMG_TabletPowers` の中のオフセット）
- **旧PC** = `pak_reference/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_PlayerCharacter.txt`（旧版 UE 4.21）
- パワーの番号は `Enum_RingAltar_Skills` に従う: 0 Speed Boost、1 Teleport、2 Telepathy、3 Primal Fear、4 Telekinesis、5 Vanish、6 None。
- 段階（Lv）は、セーブの `<パワー> Upgrades`（int）の値 0〜5。
- 「推測」と書いたものは、コードから確定できなかった解釈。書いていないものは、バイトコードか JSON で確認した値。
- ウィジェットアニメのキーの時刻はティック値。`TickResolution` がアセットに無いので、`pak_reference/README.md` §427 に従って既定の 60000/秒で秒に直した。

---

## 0. 要点

1. **枠は左右 2 つ**。どちらの枠も「解放済みのパワーの配列 `Power`」の添字（`Left Power Index`、`Right Power Index`）を持つ。**Q = 左の枠を使う**、**E = 右の枠を使う**、**1 = 左の枠の添字を +1**、**2 = 右の枠の添字を +1**（末尾の次は 0 に戻る）。左右が同じパワーを指すことも禁止されていない。**R（`Use Power`）は割り当てだけあって、どの BP も受けていない**。
2. 使える状態かどうかは、`Power` の要素 `Struct_Power` の `Available?` で判定する。使えないときは**最新版では何も起きない**。旧版では `power_not_ready`（音量 0.35）を鳴らしていた。
3. スピードブースト（0）だけはプレイヤーの中で完結する。ほかの 5 つは、テレポート・プライマル・テレキネシス・バニッシュが「プレイヤーの位置から Z −5000」に、テレパシーが原点に、それぞれ専用のアクター（`BP_Power_Teleport`、`BP_PrimalPower`、`BP_TelekinesisPower`、`BP_VanishPower`、`BP_Telepathy`）をスポーンする。
4. クールダウンは、プレイヤーの `Delay`（ラッチの Delay）と、`BP_Powers`（Power Controller）のタイムライン（長さ 1 秒、0→1 の直線、再生速度 1/秒数）で作る。タイムラインの値は、タブレットのアイコンのマテリアルの `Percent` に書き込む。
5. 段階は `BP_DD_SaveGame` の `<パワー> Upgrades` に保存される。範囲は 0〜5 で、新規ゲームでは 0。リングアルターでパワーを解放すると 1 になり、それ以降は 1 回につき +1（コスト 2）。**スピードブーストの再使用時間だけは、最新版のコードが表示用 DataTable と旧版より 1.0 秒長い**（Lv1 で 9.5 秒、表と旧版は 8.5 秒）。
6. `BP_PowerOrb` はパワーを充填する拾い物ではなく、**敵を全員スタンさせるオーブ**。病院の Zone 1 には本体 1 個と出現点 11 個、Zone 2 には本体 1 個と出現点 10 個がある。150 秒ごとに出現点をランダムに移る。
7. 病院の Zone 1・Zone 2 はゲームモードを上書きしていない。そのため、プロジェクトの既定の `BP_DD_GameMode` と、そのポーン `BP_DD_PlayerCharacter` が使われる。`_NoPowers` を使うのは `08_BearHouse_*` の 4 レベルだけ。

---

## 1. 枠（スロット）の仕組みと入力

### 1.1 キー割り当て（`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini` 50〜67 行）

| ActionName | キーボード | ゲームパッド |
|---|---|---|
| `Use Power Left` | Q | Gamepad_LeftTrigger |
| `Use Power Right` | E | Gamepad_RightTrigger |
| `Use Power` | R | （なし） |
| `Cycle Power Left` | One（1） | Gamepad_LeftShoulder |
| `Cycle Power Right` | Two（2） | Gamepad_RightShoulder |
| `Toggle Tablet` | SpaceBar | Gamepad_FaceButton_Bottom |
| （参考）`Buy Upgrade` | E | Gamepad_FaceButton_Left |

- `Use Power`（R）を受ける `InpActEvt_Use Power_*` は、全バイトコードを探しても無かった。PC の入力イベントは `Use Power Left/Right`・`Cycle Power Left/Right` の 4 つだけ。入力設定の画面（`_assets/.../UI/Pages/InputSettingsPage.json`）にも R の項目は無い。つまり **R は死んだ割り当て**。

### 1.2 枠に何が入るか（TP）

- 配列 `Power `（名前の末尾に空白がある）は `Struct_Power {Power(Enum_RingAltar_Skills), Available?(bool)}` の配列。
  - CDO の `Power_Default` は、0〜5 の 6 要素が並び、すべて `Available?=true`（`_assets/.../UI/Tablet/UMG_TabletPowers.json` の `Default__UMG_TabletPowers_C`）。
- **Check**（`Construct` の @2496 の後に続く。`Check` の入口 @4168 は @2868 へ飛ぶ）の流れ:
  - プレイヤーが `BP_DD_PlayerCharacter_NoPowers` のとき（@2894〜@2959）: `Power = Power_Default` にしてから、`Power Progress` のしきい値で要素を外す（§7.2）。
  - 通常のとき: `LoadGameFromSlot('SaveSlot', 0)` で `BP_DD_SaveGame` を読み（@3101〜@3201）、`Power = Power_Default` にする（@3211）。続く Sequence で、各パワーの解放フラグが false なら、その要素を `Array_RemoveItem` で外す。
    - `Speed Boost Unlock`（@3268、偽なら @2318 で添字 0 を外す）
    - `Teleport Unlock`（@3305 → @3342 で 1）
    - `Telepathy Unlock`（@3819 → @3856 で 2）
    - `Primal Unlock`（@3923 → @385 で 3）
    - `Telekinesis Unlock`（@3960 → @3997 で 4）
    - `Vanish Unlock`（@4064 → @4101 で 5）
  - 最後に `Delay 0.2`（@3764）してから @15 へ進む。
  - @15: `Power` が空なら `SetVisibility(Hidden)` にして終わる。空でなければ `SetVisibility(HitTestInvisible)` にしてから、`Remember Previous Powers` → `Update Powers` を呼ぶ。
- **Remember Previous Powers**（TP の同名関数 @0〜@637）: `BP_DD_GameInstance` の `Selected Power Left/Right` が `Power` の有効な添字なら、`Left/Right Power Index` にその値を入れる。
  - GameInstance の CDO は `{"Lives": 3}` だけなので、既定は **左右とも 0**。つまり、最初は左右とも `Power[0]`（解放済みの先頭のパワー）が入る。これはコードからの結論で、実機では未確認。
  - `Selected Power*` を書くのは `Update Powers` だけ（@739・@874）。GameInstance の値なのでレベルをまたいで残るが、ディスクのセーブには入らない。
- **Update Powers**（@653）: `Selected Power Right/Left` に現在の添字を書いてから、次の 2 つを行う。
  - 左の枠: `Skill1.SetBrushFromMaterial(switch Power[Left Power Index].Power)`（@980）
  - 右の枠: `Skill2.SetBrushFromMaterial(switch Power[Right Power Index].Power)`（@1203）
  - switch の対応は、0 → MID `Speed Boost`、1 → `Teleport`、2 → `Telepathy`、3 → `Primal Fear`、4 → `Telekinesis`、5 → `Vanish`、6 → None。
- セーブの `Saved Equipped Skill 1/2`（Byte）は、`UMG_RingAltar` が読むだけ（`UMG_RingAltar.txt` @2130・@2180 で `Equipped Skill 1/2` に代入）。書く場所はどこにも無く、タブレットも使わない。推測: 古い装備の仕組みの名残。

### 1.3 各入力の処理

| 入力 | 処理（根拠） |
|---|---|
| Q `Use Power Left` | `Can Interact?` が偽なら何もしない。DoOnce（`IsClosed_8`）を通ったら `Use Power(True)` を呼び、`Delay 0.5` の後に DoOnce を戻す（PC @23370〜@23520、戻すのは @6833）。つまり 0.5 秒に 1 回まで。 |
| E `Use Power Right` | 同じ流れで `Use Power(False)` を呼ぶ（@23521〜@23671、戻すのは @6810）。 |
| 1 `Cycle Power Left` | `isTabletUp? AND Can Cycle Left?` のときだけ `Powers.Cycle Power Left()` を呼ぶ（@11533〜@11617）。**タブレットを構えているときだけ切り替えられる**。 |
| 2 `Cycle Power Right` | `isTabletUp? AND Can Cycle Right?` のときだけ `Powers.Cycle Power Right()` を呼ぶ（@11618〜@11702）。 |
| Space `Toggle Tablet` | `Has Input` と `Can Use Tablet?` を確かめてから `isTabletUp?` を反転する（@26278 → @25501、@25577 で False、@25680 で True）。 |

- **Cycle Power Left / Right**（TP @1370 / @1844）:
  1. `Power.Length > 0` のときだけ、`PlaySound2D(UI_Select_V3, 1.5, 2.0)` を鳴らす（@1473 / @1947）。
  2. 添字が `Length-1` なら 0 に戻す（@1685 / @2159）。そうでなければ `Clamp(添字+1, 0, 5)` にする（@1812 / @2286）。
  3. `Update Powers` を呼ぶ。
  - まとめると、**解放済みのパワーの中を巡回する**。反対の枠と同じパワーになっても避けない。`Available?`（クールダウン中かどうか）にもよらない。切り替えの演出は音だけで、アニメは無い。
- `Can Cycle Left?/Right?`（既定は true）は、テレポートを使っている間だけ、使った側の枠で false になる（§2.3）。

---

## 2. `Use Power` の流れ（PC）

### 2.1 入口（`Use Power` の引数 `1?` = 左なら true）

1. `Can Use Tablet?` が偽なら終わる（@29314）。`Has Input` が偽なら終わる（@29324）。**タブレットを構えている必要は無い**。
2. `Powers.Animation(1?)` で枠を弾ませる（@29334）。これは使えるかどうかの判定より前なので、**クールダウン中でも枠は弾む**。
3. `Power To Use = Powers.Power[1? ? Left Power Index : Right Power Index]` にする（@29398）。その後 @11703 へ飛ぶ（@29545）。
4. `Power To Use.Available?` が偽なら @16335 へ飛ぶ。@16335 は `Power.Length < 1` を判定するが、真でも偽でも実行はそこで終わる（@16450・@16460）。つまり**使えないときは音も何も無い**。
5. 使えるとき:
   - Sequence の後段に `Delay 0.5`（@16280 → @1668 で DoOnce_11 を戻す）を積む。
   - DoOnce_11（@11731〜@11791）を通ったら、`UsedPower` を放送する（@11802）。
   - `Power To Use.Power` で分岐する（@11821〜@12131）。0 → @13628、1 → @12146、2 → @16461、3 → @18337、4 → @19661、5 → @20985。
   - Q/E の DoOnce とは別に、**Use Power の中にも 0.5 秒の連打防止がある**。

### 2.2 どのパワーにも共通する手順

- `Active Powers`（byte 配列）にパワーの番号を `AddUnique` する。
- `Array_Find(Power, {X, true})` で添字を探し、`Array_Set(添字, {X, false})` で使えない状態にする。その後 `Powers.Update Powers()` を呼ぶ。
- `PowerController.Set Delay(秒, X)` を呼び、アイコンのゲージを動かす（§3.3）。

### 2.3 パワーごとの分岐

| パワー | 使った瞬間（根拠） | スポーン |
|---|---|---|
| 0 Speed Boost | §5 を参照（@13628〜） | なし（プレイヤーの中で処理） |
| 1 Teleport | `CurrentSide = 1?`（@12146）。`UsedTeleportPower` を放送（@12253）。`PlaySound2D(/Engine/VREditor/Sounds/UI/Teleport_Mode_Entered, 1.75, 1.0)`（@12272）。使えない状態にする。`Set Delay(0.05, 1)`（@12637）。 | `BeginDeferredActorSpawnFromClass(BP_Power_Teleport, Transform(プレイヤー位置 − (0,0,5000), 回転 0, 拡縮 1), CollisionHandling=1(AlwaysSpawn), Owner=None)`（@12869）。`SetFloatPropertyByName('Max Distance', 表)`（@13132）。`FinishSpawningActor`（@13455）。`Used` にプレイヤーの `UsedTeleport` を結ぶ（@13493・@13516）。使った側の `Can Cycle Left?/Right?` を false にする（@13557〜@13623）。テレポート用の Gate を開き、DoOnce_5 を戻す（@13582 → @23175・@23152）。 |
| 2 Telepathy | `UsedTelepathy` を放送（@16461）。`PlaySound2D(/Game/Audio/SharedGameplay/Telepathy, 0.6, 1.0)`（@16568）。使えない状態にする。`PlayCameraShake(BP_CameraShake_Streak, 1.0)`（@16959）。 | `BP_Telepathy` を**原点 (0,0,0)** に、CollisionHandling=0 でスポーンする（@17101）。`'Time' = 表`（@17364）。`Set Delay(Time, 2)`（@17793）。`Delay(Time)` の後に終了処理（→ @4271）。 |
| 3 Primal Fear | `UsedPrimal` を放送（@18337）。使えない状態にする。`Set Delay(0.05, 3)`（@18750）。 | `BP_PrimalPower` をプレイヤー位置 − (0,0,5000) に AlwaysSpawn（@18982）。`'Range' = 表`（@19245）。`Delay 0.06` の後にクールダウン開始（→ @5187）。 |
| 4 Telekinesis | `UsedTelekinesis` を放送（@19661）。使えない状態にする。`Set Delay(0.05, 4)`（@20074）。 | `BP_TelekinesisPower` をプレイヤー位置 − (0,0,5000) に AlwaysSpawn（@20306）。`'Range' = 表`（@20569）。`Delay 0.06` の後にクールダウン開始（→ @8329）。 |
| 5 Vanish | `Active Powers` に 5 を足す（@20985）。`UsedVanish` を放送（@21073）。使えない状態にする。`CapsuleComponent.SetCollisionResponseToChannel(4, 0)`（@21398。4 = ECC_Camera、0 = Ignore）。`Set Delay(15, 5)`（@21628）。`UMG_Vanish` を作り、`'Speed' = 15` にして、`Vanish Widget` として `AddToPlayerScreen(0)` する（@21799〜@22226）。 | `BP_VanishPower` をプレイヤー位置 − (0,0,5000)、**回転はプレイヤーの回転**で AlwaysSpawn（@22486）。`Delay(15)` の後に終了処理（→ @6856）。 |

- **テレポートの終わり（`UsedTeleport`、@35746）**: `BP_Power_Teleport` の `Used` から呼ばれる。
  1. 使った側の `Can Cycle` を true に戻す（@35760 / @36084）。
  2. `Active Powers` から 1 を外す。
  3. `Set Delay(レベル名=='00_Ballroom' ? 1.0 : 5.0, 1)` を呼ぶ（@35988）。
  4. Gate が開いていれば `Delay(5.0 または 1.0)`（@26327〜@26546）の後、DoOnce_5 を経て使える状態に戻す（@3325〜@3761）。
  - **テレポートの再使用は段階によらず 5.0 秒**（`00_Ballroom` だけ 1.0 秒）。
- 各アクターの中身（Teleport のクリックでの確定など）はこの報告の範囲外。確認できた入口は次のとおり。
  - `BP_Telepathy`: BeginPlay で `Update Targets` を呼び、0.8 秒ごとに繰り返す。`DD_EnemyInterface` の `No Telepathy` を見る。`Time` 秒後に `Finish`。
  - `BP_VanishPower`: 'Enemy' タグの敵に `Player Vanish` を呼ぶ。
  - `BP_PrimalPower`: 範囲内の敵に `Set State(2, false)` を呼ぶ。
  - `BP_Power_Teleport`: 左クリックかパッド A の入力と、`Max Distance` を持つ。

---

## 3. クールダウンと強化段階

### 3.1 `Get Power Upgrade Level(Power) → Result`（PC の関数 @0〜@462）

- `GameMode.Global Save Instance`（`BP_DD_SaveGame`）の値を返す。対応は 0 → `Speed Boost Upgrades`、1 → `Teleport Upgrades`、2 → `Telepathy Upgrades`、3 → `Primal Upgrades`、4 → `Telekinesis Upgrades`、5 → `Vanish Upgrades`。6（None）と既定は 0。
- 保存先は `BP_DD_SaveGame` の int 6 個。CDO に値が無いので既定は 0。新規ゲームでも 0 に戻す（`BP_DD_GameMode.txt` @40176〜@40401。同じ処理で `*Unlock` も false に戻す）。
- 何段あるか:
  - プレイヤー側の switch は 0〜5 の 6 通り。
  - リングアルターでパワーを解放すると、`Auto Buy Upgrade` が、値が 1 未満なら 1 にする（`UMG_RingAltar.txt` @22013〜@23047）。
  - `Buy Upgrade` は、値が 5 より大きければ買えない（@12701）。買うときは `PowersTable_*` の行名配列 `[値]` の行から `Cost` を読み（@13414〜@14095）、スキルポイントを引いて値を +1 する（例: Telekinesis @14506〜@14570。音は `Power_Upgrade_Unlock`）。
  - 表の行は `Upgrade2`〜`Upgrade6` の 5 行で、Cost はすべて 2。値が 5 のときは添字 5 の行が無く、読みが失敗する。したがって**実際の上限は 5**（行が無くて止まる点は推測）。
- `BP_DD_PlayerCharacter_NoPowers` は、この関数を「常に 3 を返す」ものに上書きしている（`BP_DD_PlayerCharacter_NoPowers.txt`）。
- スキルポイントは 0〜30、プレイヤーのレベルは 1〜15 にクランプされる（`BP_DD_GameMode.txt` の `Clamp Level + Skillpoints`）。

### 3.2 段階ごとの値（コード）と表示用 DataTable（`_datatables.json` の `/Game/UI/RingAltar_UI/Enums/PowersTable_*`）

表の行 `Upgrade2`〜`Upgrade6` は、コードの Lv1〜Lv5 に一致する。Lv0 は「未解放」の値。

| パワー | 値 | Lv0 | Lv1 | Lv2 | Lv3 | Lv4 | Lv5 | 表（Upgrade2→6） | 根拠 |
|---|---|---|---|---|---|---|---|---|---|
| Speed Boost | 速さ（`Sprinting Speed` と `Walking Speed` の両方） | 870 | 870 | 890 | 910 | 930 | 950 | 870/890/910/930/950 ✓ | @14211〜@14760 |
| | 効果時間 | 6.75 | 6.75 | 7.5 | 8.25 | 9.0 | 9.75 | 6.75/7.5/8.25/9/9.75 ✓ | @15045〜@15394 |
| | 再使用（効果が切れてから） | **9.5** | **9.5** | **9.0** | **8.5** | **8.0** | **7.5** | **8.5/8/7.5/7/6.5 ✗（表はコードより 1.0 短い）** | @472〜@830 |
| Teleport | `Max Distance` | 1000 | 1000 | 1125 | 1250 | 1375 | 1500 | 1000/1125/1250/1375/1500 ✓ | @12911〜@13265 |
| | 再使用（テレポート完了から） | 5.0 | 5.0 | 5.0 | 5.0 | 5.0 | 5.0 | （表に無い） | @35881〜@36076（00_Ballroom は 1.0） |
| Telepathy | `Time`（効果時間） | 5 | 5 | 6 | 7 | 8 | 9 | 5/6/7/8/9 ✓ | @17143〜@17497 |
| | 再使用（効果が切れてから） | 8.5 | 8.5 | 8.0 | 7.5 | 7.0 | 6.5 | 8.5/8.0/7.5/7.0/6.5 ✓ | @4410〜@4768 |
| Primal Fear | `Range` | 1500 | 1500 | 2000 | 2500 | 3000 | 3500 | 1500/2000/2500/3000/3500 ✓ | @19024〜@19378 |
| | 再使用（使ってから 0.06 秒後に開始） | 35 | 35 | 32 | 29 | 26 | 23 | 35/32/29/26/23 ✓ | @5217〜@5710 |
| Telekinesis | `Range` | 1750 | 2000 | 2250 | 2500 | 2750 | 3000 | 2000/2250/2500/2750/3000 ✓ | @20348〜@20702 |
| | 再使用（使ってから 0.06 秒後に開始） | 10.5 | 10.0 | 9.5 | 9.0 | 8.5 | 8.0 | 10.00/9.50/9.00/8.50/8.00 ✓ | @8359〜@8845 |
| Vanish | 効果時間 | 15 | 15 | 15 | 15 | 15 | 15 | 15 ×5 ✓ | @21438〜@21796 |
| | 再使用（効果が切れてから） | 30 | 28 | 24 | 20 | 18 | 15 | 28/24/20/18/15 ✓ | @6856〜@7206 |

- レベル名で値を変える場所が 3 つある。いずれも病院とは関係しない。
  - Primal の再使用: `GetCurrentLevelName == '00_Circus_Entrance'` なら 5.0（@5378）。推測: 実際のマップ名は `05_Circus_Entrance` なので、この比較は常に偽になっている。
  - Telekinesis の再使用: `'00_Ballroom'` なら 1.0（@8520）。
  - Teleport の再使用: `'00_Ballroom'` なら 1.0（@35927）。
- Speed Boost の表示用の表の文字列は `SPEED: <YEL>870</> |TIME: <YEL>6.75</> |COOL: <YEL>8.5</>` など。
- **旧版（pak_reference）の Speed Boost の再使用は 8.5/8.5/8.0/7.5/7.0/6.5 で、表と一致する**（旧PC @479〜@594、switch @697）。最新版だけが +1.0 秒になっている。**両方の版で値が違うので、どちらを採るかはユーザーの確認が必要**（`.claude/guides/original-fidelity.md`）。

### 3.3 Power Controller（`BP_Powers`、プレイヤーの子アクター `Power Controller` のクラス）

- プレイヤーの `Power Controller Cast`（@36313）で `PowerController` に入る。BP_Powers の BeginPlay（PW @1367）は、`Delay 0.2` の後に `Player` と `Powers`（= `Player.Powers`）を取る（PW @15〜@176）。
- タイムラインは 6 本。長さはすべて 1.0 秒で、曲線は `(0,0)→(1,1)` の Linear（`_assets/.../UI/BP_Powers.json` の CurveFloat_0 系）。Update では、対応するアイコンの MID に `SetScalarParameterValue('Percent', 値)` を書く。

| タイムライン | パワー | Update の書き込み先 |
|---|---|---|
| `Timeline` | Speed Boost | `Powers.Speed Boost`（PW @2474） |
| `Timeline_0` | Teleport | `Powers.Teleport`（@277） |
| `Timeline_1` | Telepathy | `Powers.Telepathy`（@454） |
| `Timeline_2` | Primal Fear | `Powers.Primal Fear`（@631） |
| `Timeline_3` | Vanish | `Powers.Vanish`（@808） |
| `Timeline_4` | Telekinesis | `Powers.Telekinesis`（@985） |

- `Set Delay(Duration, Power)`（PW @1609）は、パワーの番号で `Set Delay <パワー>(Duration)` に振り分ける。
- `Set Delay <パワー>(D)` は、`SetPlayRate(1/D)` にしてから FlipFlop で再生の向きを交互に変える。
  - 1 回目（A）: `ReverseFromEnd`。Percent が 1→0 と D 秒かけて減る。効果中、または使った瞬間の 0.05 秒。
  - 2 回目（B）: `PlayFromStart`。Percent が 0→1 と D 秒かけて戻る。クールダウン。
  - 各パワーの入口と FlipFlop: Speed Boost PW @1422（A @178 / B @244）、Telepathy @2349（@2024 → A @532 / B @598）、Primal @3060（@1161 → A @709 / B @775）、Vanish @3268（@1228 → A @886 / B @952）、Telekinesis @3476（@1314 → A @1062 / B @1128）。
  - **Teleport だけは FlipFlop を使わない**。`Percent < 1.0` なら PlayFromStart、そうでなければ ReverseFromEnd（PW @2091〜@2344）。
- この結果、ゲージは次のように動く。
  - Speed Boost・Telepathy・Vanish: 効果時間のあいだ 1→0 に減り、クールダウンのあいだ 0→1 に戻る。
  - Teleport・Primal・Telekinesis: 使った瞬間に 0.05 秒で 0 になり、クールダウンのあいだ 0→1 に戻る。Teleport は照準中ずっと 0 のまま。
- `Stop <パワー> Timeline` はタイムラインを止め、`Percent = 1.0` にする（PW @2806・@2889・@2972・@3185・@3393・@3602）。
- `Reset All Powers`（PW @2551）は Sequence で次を順に行う。
  1. `Player.Reset Speed Boost 1` → `Player.Reset Speed Boost 2` → `Stop Speed Boost Timeline`
  2. `Player.Reset Teleport` → `Stop Teleport Timeline`（@2755）
  3. `Stop Primal Timeline` → `Player.Reset Primal`（@2704）
  4. `Stop Vanish Timeline` → `Player.Reset Vanish`（@2653）
  - **Telepathy と Telekinesis はここでは戻さない**。
- `Reset Teleport`（PW @3055）は @2755 へ飛ぶ。

### 3.4 プレイヤー側の Reset 系と `Recheck Powers`・`UI Cooldown`

| 関数（入口の IntConst） | 役割 |
|---|---|
| `Reset Powers`（37130） | `PowerController.Reset All Powers()` を呼ぶ。呼び出し元は `UMG_DeathScreen` だけ（`Fade Out` の 2 秒後、`Respawn Event` の直前。`UMG_DeathScreen.txt` @15〜@244）。 |
| `Reset Speed Boost 1`（37067） | 再生中の `CameraAnim_SpeedBoost` を `Stop(True)` してから @30 へ飛ぶ。ブーストを即座に終わらせる（速さを 600/300 に戻し、クールダウンを始める）。DoOnce_3 はブーストを使ったときだけ開くので、効果中でなければ何もしない。 |
| `Reset Speed Boost 2`（37105） | @1206 へ飛ぶ。DoOnce_4 を経て、`power_refilled` を鳴らし、使える状態に戻す。これもブーストを使った後だけ効く。 |
| `Reset Teleport`（37110） | Sequence で次を行う。(1) @26557 テレポート用の Gate を閉じる。(2) @3325 DoOnce_5 で即座に充填（音つき）。(3) @36100 スポーン済みの `BP_Power_Teleport` を `K2_DestroyActor`。(4) @35746 `UsedTeleport` の処理（切り替えを許し、`Set Delay(5,1)` を呼ぶ。Gate が閉じているので Delay は始まらない）。 |
| `Reset Primal`（37261） | Gate_2 を閉じる（@27788 → @27394）。その後、@6444 で即座に充填（音つき）。 |
| `Reset Vanish`（37737） | Gate_3 を閉じる（@27838 → @27826）。その後、@7927 で即座に充填し、`Vanish Widget.RemoveFromParent` する。 |
| `Reset Telekinesis`（37753） | Gate_4 を閉じる（@28238 → @28226）。その後、@9572 で即座に充填する。呼び出し元は見つからなかった。 |
| `Recheck Powers`（36137） | `UMG_Tablet.UMG_TabletPowers.Check()` を呼び、解放フラグから `Power` を作り直す。呼び出し元は `UMG_RingAltar`（@5839 付近）と `00_Ballroom`。 |
| `UI Cooldown`（デリゲート `(Slot 1?, Seconds)`） | Speed Boost が `UI Cooldown(True, 10.0)` を放送する（@15020）。購読しているのは `UMG_Tablet`（§4.4）。 |

- クールダウンの Gate について:
  - Primal・Vanish・Telekinesis・Telepathy のクールダウン明けは「Gate を Open してから Enter する」（例: Primal @6301 → @6412 Open → @6424 Enter → @6434）。Reset で閉じても、残っている Delay が終わると開き直して、もう一度充填する。推測: Reset の後に元のクールダウンが終わると、`power_refilled` がもう一度鳴る。
  - Telepathy の Gate_1 には Close が無い。
- 推測: `Check()` は `Power = Power_Default`（すべて `Available?=true`）で作り直す。そのため、クールダウン中に `Recheck Powers` が呼ばれると、その場で使える状態に戻る。

### 3.5 `power_refilled` を鳴らす条件

- 鳴らし方はどこでも `PlaySound2D(/Game/Audio/UI/power_refilled, 音量 0.5, ピッチ 1.0)`。音の長さは 1.515 秒。
- 鳴るのは、クールダウンの Delay が終わって `Available?` を true に戻すとき。Reset 系で即座に充填するときも同じ処理を通るので鳴る。
  - Speed Boost @1252（DoOnce_4 の中）、Teleport @3396（DoOnce_5）、Telepathy @3880（Gate_1）、Primal @6444（Gate_2）、Vanish @7927（Gate_3）、Telekinesis @9572（Gate_4）。
- `power_not_ready`（2.005 秒）は、最新版のどのバイトコードからも参照されていない。旧版は §5.4 を参照。

---

## 4. タブレットの枠の表示

### 4.1 配置（`_assets/.../UI/Tablet/UMG_Tablet.json`・`UMG_TabletPowers.json`）

- `UMG_Tablet` の中の `UMG_TabletPowers` の Slot は、中央アンカーで Offsets が (−356.96, −428.54)、大きさが 714×864。タブレットの背景 `tablet_screen_bg` は 714×864。
- `UMG_TabletPowers` の中の `CanvasPanel_2` は、中央アンカーで (−355, −420)、714×864。その中の 2 つの枠は次のとおり。
  - **左の枠** `CanvasPanel_3`（`Skill1` を含む）: ストレッチの余白が L 23.43 / T 32 / R 562.25 / B 705.05。矩形にすると x 23.4〜151.7、y 32〜159（約 128×127）。
  - **右の枠** `CanvasPanel_4`（`Skill2` を含む）: L 560 / T 32 / R 25 / B 705。矩形は x 560〜689、y 32〜159（約 129×127）。
  - `Skill1`・`Skill2` はどちらも `Image`。Slot はアンカー 0〜1、余白 0 で枠いっぱいに広がる。Brush の ImageSize は 105×104。

### 4.2 アイコン（Construct、TP @2496〜@2849）

- `CreateDynamicMaterialInstance` で、次の MID を作る。
  - `Teleport` ← `MM_Powers_Inst_Teleport`
  - `Speed Boost` ← `MM_Powers_SpeedBoost`
  - `Telepathy` ← `MM_Powers_Inst_Telepathy`
  - `Primal Fear` ← `MM_Powers_PrimalFear`
  - `Vanish` ← `MM_Powers_Vanish`
  - `Telekinesis` ← `MM_Powers_Inst_Telekinesis`
- どの枠にどのアイコンを出すかは、§1.2 の `Update Powers` で決まる。左 = `Power[Left Power Index]`、右 = `Power[Right Power Index]`。**本作のように「左はテレポート、右はスピードブースト」と固定されてはいない**。
- マテリアル（`_materials.json`）:
  - 親は `MM_Powers`（Translucent、Unlit）。パラメータはテクスチャの `DisabledPower` と `EnabledPower`、スカラーの `Percent`（既定 1.0）。
  - 例えば SpeedBoost は `DisabledPower` = `ring_altar_power_speed_boost_icon`、`EnabledPower` = `ring_altar_power_speed_boost_icon_inactive`。名前と中身が逆に見えるが、書き出しのとおり。
  - **`Percent` をマテリアルの中でどう使うか（下から満ちる、など）は、ノードのグラフが書き出しに無いので未確定**。

### 4.3 枠を弾ませる演出（`Use Left` / `Use Right`）

- `Animation(Left?)`（TP @2385）が `PlayAnimation(Left? ? Use Left : Use Right, StartAt 0, Loops 1, Forward, Speed 1.0, RestoreState false)` を呼ぶ。
- `Use Left_INST` は `CanvasPanel_3`（左の枠）に、`Use Right_INST` は `CanvasPanel_4`（右の枠）に結ばれている（AnimationBindings）。
- トラックは `RenderTransform` の `MovieScene2DTransformSection` で、Scale の X と Y が同じキーを持つ。補間は Cubic（Auto）。

  | 時刻（ティック） | 秒 | Scale |
  |---|---|---|
  | 0 | 0 | 1.0 |
  | 3000 | 0.05 | 1.25 |
  | 9000 | 0.15 | 1.10 |
  | 30000 | 0.5 | 1.0 |

  - Translation・Rotation・Shear は 0 の定数（左のみ明記。右は Scale だけ）。PlaybackRange は 0〜30001（0.5 秒）、DisplayRate は 20fps。
- 切り替え（1/2）のときの演出は `UI_Select_V3`（音量 1.5、ピッチ 2.0）の音だけ。

### 4.4 `UMG_Tablet` の `UI Cooldown`（`UMG_Tablet.txt` の Construct @880〜@1023 で購読、本体 @1234〜）

- 処理は次のとおり。
  1. `PlayAnimation(Slot1? ? Skill 1 Fade : Skill 2 Fade, 0, 1, Forward, Seconds/100)`
  2. `PlayAnimation(Skill 1/2 Flicker, 0, Loops 0（無限）, Forward, 1.0)`
  3. `Delay(Seconds)` の後に Flicker を止める（@15）。
- ただし、結び付け先の `Skill1Red`・`Skill2Red`・`CanvasPanel_2`・`CanvasPanel_3` は、`UMG_Tablet` のウィジェット木に無い。木にあるのは `CanvasPanel_0/1/4/5/762`、`black_overlay`、`ShardCount`、`UMG_TabletPowers` など。推測: 見た目の効果は無い（旧 UI の名残）。
- Speed Boost は常に `Slot1?=True` で呼ぶ。

### 4.5 未使用のウィジェット

- `UI/Tablet/UMG_TabletPowers1`・`UMG_TabletPowers2` は、どのバイトコードからもレベルからも参照されていない。1 は MID を Teleport と SpeedBoost の 2 つだけ作る、2 は MID を作らない、という違いがある。推測: 古い試作。

---

## 5. スピードブースト（最新版）のすべて

### 5.1 使った瞬間（PC @13628〜）

1. `Active Powers` に 0 を足す（@13648）。
2. `PlaySoundAtLocation(/Game/Audio/UI/Shard_Streak_Milestone_V5, 自分の位置, 回転 0, 音量 1.0, ピッチ 1.0, Start 0, 減衰 None)` を鳴らす（@13716）。音の長さは 1.058 秒。
3. `Available?` を false にして `Update Powers` を呼ぶ（@13804〜@14074）。
4. `PlayCameraShake(/Game/UI/Menu/Streaks/BP_CameraShake_Streak, Scale 1.0, CameraLocal)` を呼ぶ（@14136）。
   - シェイクの中身（`_camera/_camera_shakes.json`）: 振動時間 0.5 秒、BlendIn 0、BlendOut 0.25。回転は Pitch（振幅 0.25、周波数 30）、Yaw（0.25、40）、Roll（0.5、35）。FOV は振幅 2.0、周波数 10。
5. `Sprinting Speed = 表`（@14401）、`Walking Speed = 表`（@14742）にする。**歩いていても走っていても、ブースト中は同じ速さ**。`MaxWalkSpeed = Sprinting? ? Sprinting Speed : Walking Speed`（@14912）。
6. Sequence で次を行う。
   - `UI Cooldown(True, 10.0)` を放送する（@15020）。
   - `PowerController.Set Delay(効果時間, 0)`（@15235）。
   - `Update Bob()`。
   - `PlayCameraAnim(/Game/Animation/Camera/CameraAnim_SpeedBoost, Rate 1.0, Scale 1.0, BlendIn 0.5, BlendOut 0.5, bLoop false, bRandomStart false, Duration=効果時間, CameraLocal)`（@15636）。
     - アニメの中身（`_camera/CameraAnim_SpeedBoost.json`）: 長さ 24.50 秒、base_fov 137.24。トラックは `PostProcessSettings.SceneColorTint` の 1 本だけで、値は **(2.0, 0.584, 0.498, 1.0) の定数**（赤みがかった色調）。PostProcess の BlendWeight は 1.0。
   - `Delay(効果時間)` の後、終了処理（@30）へ。
   - DoOnce_3（@16257）と DoOnce_4（@16234）を戻す。
   - `K2_SetTimer('Sprinting Effects', 0.001, 繰り返し)`（@23268）。`Chameleon FX.Camera Shake = True`（@23332）。
   - @11203 で `UMG_SpeedBoost` を作り、`AddToViewport(ZOrder 1)`（@11256）。

### 5.2 効果中に毎フレーム動くもの

- `Sprinting Effects`（@35054）: `Speed` は速度の大きさ（@31419）。
  - `Chameleon FX.Radial Blur Width = MapRangeClamped(Speed, 0, 870, 0, 1)`
  - `Camera Shake Frequency = MapRangeClamped(Speed, 0, 870, 0, 15)`
  - `Camera Shake Power = MapRangeClamped(Speed, 0, 870, 0, 0.003)`
  - `Chameleon` は `/Game/ThirdParty/Chameleon/Chameleon`。プレイヤーの子アクター `FX` で、相対位置 Z +2000、拡縮 (5,5,1)。
- `FOV Multiplier`（@34747）: BeginPlay から 0.001 秒ごとに常に動く。ブースト専用ではない。
  - `Camera.SetFieldOfView(FInterpTo(現在の FOV, MapRangeClamped(Speed, 300, 900, 90, 115), Delta Seconds, 0.5))`
- `UMG_SpeedBoost`（`UI/Main/Powers/UMG_SpeedBoost.txt`）: Tick のたびに `Delay 0.001` を挟んでから次を行う。
  - `Lines.SetOpacity(MapRangeClamped(Player.Speed, 0, 900, 0, 0.15))`
  - `Image_72.SetOpacity(MapRangeClamped(Player.Speed, 0, 900, 0, 0.5))`
  - ウィジェット: `Lines` は `M_Speedlines`、色 (1,0,0,1)、RenderScale 1.25。`Image_72` は `T_VignetteNew` 1024²、色 (1,0,0,1)、RenderScale 1.2。どちらも全画面ストレッチ。
  - アニメ `Fade`（Image_72 のアルファ 0→1 を 0.5 秒、Scale 1.5→1.3）はあるが、コードからは再生されていない。
- `Update Bob`（@34932 → @10513）: 移動中なら、`HeadBobbing` の CVar が 1 のとき Walk/Run のシェイクを再生する。ブースト専用ではない。

### 5.3 終わったとき（@30、DoOnce_3）

1. `Sprinting Speed = 600`、`Walking Speed = 300`、`MaxWalkSpeed` を戻す（@76〜@141）。`Update Bob`。
2. `Active Powers` から 0 を外す（@268）。`ClearTimer('Sprinting Effects')`。
3. `Radial Blur Width = 0`（@358）、`Chameleon FX.Camera Shake = False`（@403）。`UMG_SpeedBoost.RemoveFromParent`（@436）。
4. `Set Delay(再使用, 0)`（@662）。`Delay(再使用)` の後、@1206 の DoOnce_4 で `power_refilled` を鳴らし、`Available? = true` にして `Update Powers` を呼ぶ。

- 速さ・時間・再使用の段階ごとの値は §3.2 を参照。

### 5.4 旧版（pak_reference）との違い

| 項目 | 旧版 | 最新版 |
|---|---|---|
| 再使用 Lv0〜5 | 8.5/8.5/8.0/7.5/7.0/6.5（旧PC @479〜@594） | **9.5/9.5/9.0/8.5/8.0/7.5**（@472〜@587） |
| 段階の取り方 | `GameMode.Global Save Instance.Speed Boost Upgrades` を直接読む（旧PC @12342 など） | `Get Power Upgrade Level(0)`（上書きでき、NoPowers では 3 に固定） |
| `Use Power` の前提 | `CanMove?` だけ（旧PC @24141） | `Can Use Tablet?` と `Has Input`（@29314・@29324） |
| Q/E の連打防止 | なし（旧PC @18336・@18362 は `Can Interact?` の後、すぐ `Use Power`） | DoOnce と 0.5 秒で戻す仕組みが、Q/E 側と Use Power 側の二重にある |
| 使えないとき | テレポートを使っていて同じ側なら `PowerController.Reset Teleport`（照準の取り消し）。それ以外は `PlaySound2D(power_not_ready, 0.35, 1.0)`（旧PC @14604〜@14885） | **何もしない**（@16335〜@16460） |
| `UsedPower` の放送 | なし（デリゲート自体が無い） | あり（@11802） |
| 選んだ枠を次のレベルに引き継ぐか | なし（旧 `UMG_TabletPowers` に `Remember Previous Powers` が無い） | GameInstance の `Selected Power Left/Right` で引き継ぐ |
| パワーの数 | 4（0〜3。Telekinesis と Vanish は無い） | 6 |
| Chameleon のパス | `/Game/Chameleon/Chameleon` | `/Game/ThirdParty/Chameleon/Chameleon` |
| 同じもの | 速さ 870〜950、効果時間 6.75〜9.75、`Shard_Streak_Milestone_V5`、`BP_CameraShake_Streak`、`CameraAnim_SpeedBoost`（1, 1, 0.5, 0.5）、`UI Cooldown(True,10)`、`UMG_SpeedBoost`（中身の定数も同じ）、Sprinting Effects（0〜870 の対応）、FOV（300〜900 → 90〜115、補間 0.5）、終わった後の 600/300、`power_refilled` 0.5 | |

- テレパシーは、旧版ではプレイヤーの `Telepathy Func`（`GetAllActorsWithInterface(DD_EnemyInterface)`）で処理していた。最新版では `BP_Telepathy` をスポーンする。

---

## 6. `BP_PowerOrb`（`Blueprints/Main/BP_PowerOrb.txt`）と `BP_PowerOrbSpawnPoint`

- **役割**: 拾うと敵が全員スタンするオーブ。パワーの充填はしない。
- **拾ったとき**（Capsule の BeginOverlap で、相手がプレイヤーのとき。@3015 → @2279）:
  1. DoOnce を通る。タイマーを消す（@2330〜）。
  2. `P_ky_impact` を出す。`K2_DestroyActor`。
  3. `GameInstance.Used Stun Orbs? = True`（@2600）。
  4. `PlaySound2D(Soul_Shard_Pickup_v2_Cue, 0.8, 0.75)`（@2633）。
  5. @1811 で `UMG_VignetteSides` を作る。Color は (1, 0.4654, 0, 1)、`Text?` は true、`TextToDisplay` は「ENEMIES STUNNED」。`AddToPlayerScreen(5)`。`PlayCameraShake(BP_CameraShake_Streak)`。
  6. @15 で `PlaySoundAtLocation(8-Dark_power_ball_countdown_, 原点, 1, 1)` を鳴らす（音の長さ 17.34 秒）。
  7. `BP_StunCollectEffect` をスポーンする。`PrintString('Collected Power Orb')`。
  8. `GetAllActorsWithTag('Enemy')` で集めた敵のうち `DD_EnemyInterface` を持つものに、`Set State(2, byOrb=True)` を呼ぶ（@729）。
- **出現の仕組み**:
  - `Shard Spawn Time` の既定は 150.0（CDO）。病院のレベルのインスタンスは上書きしていない。
  - BeginPlay（@2834）で `SetTimerDelegate(Spawn Power Orb, 150, 繰り返さない)` を仕掛ける。
  - `Spawn Power Orb`（@3094）:
    1. `Flicker` と、0.1 秒ごとの `Flicker` タイマーで点滅させる。
    2. `Delay 5` の後（@844）、点滅を止めて表示する。
    3. 今いる場所に `P_ky_flash_PowerOrb_Disappear`（拡縮 0.5）を出す。
    4. `Spawn Points` から `RandomIntegerInRange(0, N−1)` で 1 つ選ぶ。重複を避ける分岐は `JumpIfNot False` で無効になっている。
    5. `SetActorLocation(teleport=True)` で移る。
    6. `Timer()` でまた 150 秒のタイマーを仕掛ける（@3249 → @2834）。
    7. `P_ky_flash_PowerOrb_Appear` を出す。
  - `Spawn Points` は、構築スクリプトで `Auto Assign Spawn Points` が真のとき、`GetAllActorsOfClass(BP_PowerOrbSpawnPoint)` で集める。レベルのインスタンスには、配列が保存済みで入っている。
  - `BP_PowerOrbSpawnPoint` にはロジックが無い。ビルボード（`m_crystal_Inst3`）だけのマーカー。
- **部品**:
  - `soul_shard`: `/Game/Meshes/Shared/power_orb`、マテリアル `m_crystal_Inst3`。
  - `Capsule`: 半径と半高がどちらも 840.62、相対拡縮 0.1。親の拡縮はここでは未確認。
  - `PointLight`: 強さ 1000、半径 200、色 (0,146,255)。
  - `StaticMesh`: Plane に `M_PowerOrb` を貼ったもの。Z +2000、拡縮 (1.5, 1.5, 10)。推測: マップ用の目印。
  - `PPP_Collect_Shard`（自動では起動しない）。
- **病院への配置**（`_levels/*.full.json`）:

  | レベル | 本体 | 本体の位置 | 出現点 |
  |---|---|---|---|
  | `06_Hospital_Zone_01` | `BP_PowerOrb_2` ×1 | (−2965, 2535, **5725**) | 11 個（`BP_PowerOrbSpawnPoint2〜11`、`_2`。Z ≈ 0。例: (−1705, −2200)、(3910, −3609)、(−5108, −5413)、(−7509, −8590)、(3.1, −8710)、(5405, −10526)、(3223, −10929)、(9.0, −14012)、(−5711, −14111)、(5390, −14100)、(−1499, −16221)） |
  | `06_Hospital_Zone_02` | `BP_PowerOrb_2` ×1 | (30698.8, 36877, 548.0) | 10 個（Z は 0 か約 530。x 5384〜14125、y −5479〜5455） |

  - 推測: どちらの本体も、初めの位置はプレイできる範囲の外（高所か遠方）にある。最初の 150 秒後に、はじめて出現点へ移って見えるようになる。
  - 他のレベルの数（本体/出現点）: 01_Hotel 1/9、02_elementaryschool 2/11、03_Manor_Zone1 1/7、Zone2 1/8、04_Sewer 2/11、05_Circus_Zone1 1/10、Zone2 1/10、07_FunPlace_Zone_01 1/8、Zone_02 1/10、Zone_03 0/8、08_BearHouse_Zone_02 1/10、Zone_03 1/12。
- 未確認: Tick（@2915）の `soul_shard.K2_AddLocalRotation(Yaw = Float 3)`。`Float 3` は CDO にもレベルのインスタンスにも値が無いので、回転速度が分からない。
- 別物: `Blueprints/06_Hospital/Bossfight/BP_06_Bossfight_Pickup_PowerOrb` はボス戦専用で、今回は中身を見ていない。

---

## 7. 病院で使われるゲームモードとプレイヤー、パワーを制限するステージ

### 7.1 病院の Zone 1・Zone 2

- WorldSettings が持つプロパティは `NavigationSystemConfig`、`DefaultReverbSettings`、`BookmarkArray` だけで、`DefaultGameMode` の上書きは無い。
- そのため `DefaultEngine.ini` の `GlobalDefaultGameMode=/Game/Blueprints/Main/BP_DD_GameMode.BP_DD_GameMode_C`（13 行目）が使われる。
  - `BP_DD_GameMode` の CDO: `DefaultPawnClass = BP_DD_PlayerCharacter`、`PlayerControllerClass = DD_PlayerController`、`GameStateClass = DD_GameState`、`PlayerStateClass = BP_DD_PlayerState`。
  - `GameInstanceClass = BP_DD_GameInstance`（`DefaultEngine.ini` 10 行目）。
- 病院のレベルのバイトコード（`06_Hospital_Zone_01.txt`・`_02.txt`）には、パワーに関わる呼び出しが無い。タブレットについては `ShardCount` を触るだけ。

### 7.2 パワーを制限するステージ

- `BP_DD_GameMode_NoPowers`（`DefaultPawnClass = BP_DD_PlayerCharacter_NoPowers`）を使うのは、`08_BearHouse_Exterior`、`08_BearHouse_Zone_01`、`_02`、`_03` の 4 つだけ。
- `BP_DD_PlayerCharacter_NoPowers` の中身:
  - `Get Power Upgrade Level` を「常に 3 を返す」ものに上書きしている。
  - int 変数 `Power Progress` を持つ（既定 0）。
- タブレットの `Check` は、`Power Progress` のしきい値で次のようにパワーを外す（TP @3030〜@3749）。

  | パワー | 残る条件 | 根拠 |
  |---|---|---|
  | Speed Boost | `Power Progress > 3` | @3030 |
  | Teleport | `> 5` | @3409 |
  | Telepathy | `> 4` | @3480 |
  | Primal | `> 1` | @3551 |
  | Telekinesis | `> 2` | @3622 |
  | Vanish | `> 0` | @3693 |

  - つまり、解放される順は Vanish → Primal → Telekinesis → Speed Boost → Telepathy → Teleport。
- 値は、レベルから `BP_DD_Functions.Set Player Power Progress` で入れ、`Update Player Powers`（`Powers.Check()`）で反映する。各レベルでの定数: Exterior 6、Zone_01 は 6 と 1、Zone_02 は 6・3・2、Zone_03 は 5 など。
- ほかのゲームモードの上書き:
  - `BP_DD_GameMode_NoPawn`: 00_Ballroom_Cinematic、TitleScreen
  - `BP_DD_GameMode_Dream`（ポーン `BP_07_Cutscene_PlayerCharacter`）: Cutscene_01_*
  - `GameModeBase`: 00_Splash、09_MallPreview

---

## 8. パワーの使用に関するデリゲートの購読者（プレイヤー以外の全バイトコード）

| デリゲート | 購読者（AddMulticastDelegate の場所 → 受けるイベント） |
|---|---|
| `UsedTelepathy` | `00_Ballroom`（@38814 → `OpenDoorThing`）、`00_Ballroom_Damaged`（@28582 → `OpenDoorThing`） |
| `UsedTeleportPower` | `00_Ballroom`（@12673 → `UsedTeleportPower`）、`00_Ballroom_Damaged`（@12918 → 同じ名前） |
| `UsedPrimal` | `05_Circus_Entrance`（@7438 → `00_Primal`）、`08_BearHouse_Zone_01`（@31204 → `04 Malak Line`）、`Blueprints/00_Ballroom/BP_Ballroom_Event5_Monkey`（@995 → `Primal`） |
| `UsedVanish` | `07_FunPlace_Zone_01`（@55079 で結ぶ → `00_VanishedSecurity`、@55193 で追加） |
| `UsedTelekinesis` | **購読者なし** |
| `UsedPower` | `08_BearHouse_Zone_02`（@33809 → `Malak Power Line `）、`08_BearHouse_Zone_03`（@14587 → 同じ名前） |
| `UI Cooldown` | `UI/Tablet/UMG_Tablet`（Construct @1000・@1023 → `UI Cooldown`） |

- 病院のレベルと病院の敵は、どれも購読していない。いずれもチュートリアルや台本のためのもの。
- 関連: `BP_DD_Functions.Is Player Using Power ?(Power)` は `Active Powers` に含まれるかを返す。呼び出し元は次の 2 つ。
  - `BTS_ActorInRange`: `Is Player Using Power ?(5)`（Vanish）。敵の State が 2（Stun）か Vanish 中なら、`IsInRange? = false` にする（@164〜@316）。
  - `BP_03_Watcher`: 同じく 5（Vanish）で呼ぶ。

---

## 9. 敵側のインターフェース `/Game/Blueprints/Characters/Shared/DD_EnemyInterface`

関数の一覧は `_assets/.../DD_EnemyInterface.json` による。State の型は `Enum_EnemyStates`。

| 関数 | 引数 | パワーとの関係と呼び出し元（最新版） |
|---|---|---|
| `Player Vanish` | なし | **Vanish**。呼ぶのは `BP_VanishPower.txt`（@169 付近。'Enemy' タグの敵を順に呼ぶ）。実装しているのは `BP_DD_Character_Base`、`BP_06_ReaperNurse`（入口 10871）など 25 個。 |
| `No Telepathy` | 出力 `b` | **Telepathy**。敵がテレパシーに映らないかどうか。呼ぶのは `BP_Telepathy.txt`（`Update Targets` の @372）。`BP_DD_Character_Base` と `BP_06_ReaperNurse` の実装は false を返す（映る）。 |
| `Set State` | `State`（Enum_EnemyStates）、`byOrb` | **Primal Fear と PowerOrb**。呼ぶのは `BP_PrimalPower`（`Set State(2, false)`）、`BP_PowerOrb`（`Set State(2, true)`）、`08_BearHouse_Zone_03`（`2, false`）、`BTT_SetenemyState`（`State` の値で）。 |
| `Get State` | 出力 `State` | 間接的に関係する（Stun の判定）。呼ぶのは `BTS_ActorInRange` と、ホテルの音楽プレイヤー 3 つ。 |
| `Custom Depth` | `Duration` | 推測: 旧テレパシーの強調表示。**最新版には呼び出し元が無い**。旧版ではプレイヤー（旧PC、`Telepathy Func` 付近）が呼んでいた。 |
| `Activate Frenzy` / `Get Frenzy` | なし / 出力 `Frenzy` | パワーとは関係しない。`BP_DD_GameMode`・`BP_Monkey` が呼ぶ / `BTT_SetSpeed` が読む。 |
| `Get Run Speed` / `Get Walk Speed` | 出力 | パワーとは関係しない（`BTT_SetSpeed`）。 |
| `HammerHit` | なし | パワーとは関係しない（`BP_05_HoldableHammer`）。 |
| `Chasing `（名前の末尾に空白） | `bChasing` | パワーとは関係しない（音楽プレイヤーなど）。 |

- `Enum_EnemyStates` の表示名の対応（DisplayNameMap）は、NewEnumerator0 = Patrol、1 = Pursue、3 = Stun、4 = Teleport。NewEnumerator2 が欠けている。
  - 推測: 2 を削除したため値が詰め直され、**値 2 = Stun** になった。`BTS_ActorInRange` が「State==2 なら範囲外」として扱っていることとも合う。
- Vanish の見えなさは、2 つの仕組みで作られている。
  1. プレイヤーの Capsule が Camera チャンネルを Ignore にし、終わったら Block に戻す（@21398 / @7334）。
  2. AI が `Is Player Using Power ?(5)` を見る。
  - 推測: 敵の視線判定が Camera チャンネルのトレースを使っている。

---

## 10. 本作への含意（実装の参考。どちらを採るかの判断はしていない）

- 本作の「再使用 8.5 秒」は、旧版と DataTable の Lv1 の値にあたる。**最新版のコードは Lv1 で 9.5 秒**。どちらに揃えるかはユーザーの確認が要る。
  - 速さ 870、時間 6.75、歩き 300、ダッシュ 600 は、どちらの版でも同じ。
  - 段階を 1 つに固定して作るなら、パワーを解放したときの値である Lv1 がリングアルターの流れに合う。
- ブースト中は `Walking Speed` も 870 になる。そのためダッシュキーを押していなくても 870 で動く。
- 左右の枠の中身は固定ではない。`Power`（解放済みの配列）と添字 2 つで決まり、1/2 キーで巡回し、切り替えはタブレットを構えている間だけ。左右とも既定は添字 0。
- 使えないときの `power_not_ready` は、最新版には無い。R キーは何もしない。
- ゲージ `Percent` の動き:
  - 効果時間を持つパワー（Speed Boost・Telepathy・Vanish）は、効果中に 1→0、クールダウン中に 0→1。
  - それ以外（Teleport・Primal・Telekinesis）は、使った瞬間に 0 になり、クールダウン中に 0→1。
- 死んで生き返るときの `Reset Powers` は、Speed Boost・Teleport・Primal・Vanish だけを即座に充填する。Telepathy と Telekinesis は戻さない。

---

## 11. 未確定・確認できなかったこと

- `MM_Powers` の中で `Percent` がどう描かれるか。マテリアルのグラフが `_materials.json` に無い。手元の本家で観察すれば確かめられる。
- 左右の枠が既定で同じパワー（添字 0）を出すという結論は、コードから導いたもの。実機での見え方は未確認。
- 強化段階の上限が 5 であることのうち、「行が無いので買えない」という止まり方は推測。
- `UMG_Tablet` の `UI Cooldown` のアニメに効果が無いこと。結び付け先のウィジェットが木に無いことからの推測。
- `BP_PowerOrb` の回転速度（`Float 3` の値）と、Capsule の実際の大きさ（親の拡縮）。
- 病院の PowerOrb 本体の初めの位置がプレイできる範囲の外かどうか。座標からの推測で、地図との照合はしていない。
- `Reset` 系の後に元のクールダウンが終わったとき、充填の音がもう一度鳴るかどうか。Gate の構造からの推測。
- `Reset Telekinesis` と `BP_Powers.Reset Teleport` の呼び出し元。最新版では見つからなかった。
- 各パワーのアクター（`BP_Power_Teleport` など）の詳しい挙動。この報告の範囲外。
