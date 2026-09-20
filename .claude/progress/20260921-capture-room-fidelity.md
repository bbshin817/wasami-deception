---
title: 捕獲の別室を原作の Matinee とアセットの値どおりにする（作業一覧の項目 32）
status: 進行中
branch: main
base: 2399e97
started: 2026-09-21 05:29
updated: 2026-09-21 06:10
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 捕獲の別室を原作の Matinee とアセットの値どおりにする（作業一覧の項目 32）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 3 の項目 32。大目標 2 の項目 9・24 で組んだ捕獲の別室（カメラの寄り・灯・場面の時間・暗転）を、**原作の Matinee とアセットの値どおりにする**。

完了の条件:
- 原作の Matinee 6 本（`MonkeyJumpscare`・`2`・`3` と `03_Watcher_Kill*`）のキーと灯のアクターの値を、本作の実装と 1 つずつ突き合わせて差を埋める。← 表はステップ 1 で作成済み
- **コードで決まらなかったものだけ**、旧版の実機を 1 回だけ撮って見比べる。
- 本作の体（ワサミのモデルと骨）では写せない差は、理由を書いて閉じてよい。

大目標 3 の節の頭の決まり: **原作のコード・アセットで先に決める**（本家の実機は中の移動が遅すぎて 1 回の収録で 1 か所しか回れない）。

## 突き合わせ表（ステップ 1 の結果。読んだもの: `pak_reference/_levels/01_Hotel.full.json`、`_camera/03_Watcher_*.json`、`_camera/_camera_shakes.json`、`Source/wasami_deception/WasamiCapture.h`・`.cpp`）

### A. 一致していて直すことがない

| 項目 | 原作の値 | 本作 |
| --- | --- | --- |
| カメラのキー | MJ1 17 点・MJ2 16 点（`InterpGroup_2.InterpTrackMove_2`、世界座標・接線つき） | `Hotel1/2Position`・`Euler` に保存値どおり |
| FOV | `InterpTrackFloatProp_0`（FOVAngle）に**キーが無い** → `CameraComponent` の既定 90 | `FieldOfView = 90` |
| 画面の切り替え | `InterpTrackDirector_0`: t=0 に `NewCameraGroup`、`TransitionTime` 0 | 切り替えのみ（ブレンド無し） |
| 長さと暗転のキー | 2.1212 / 2.4643 / 3.0684、フェード 1.7005→1.7700 / 1.9207→2.0513 / 2.6005→2.7700、`bPersistFade` true | `MatineeFades` に同値、保持つき |
| 音 | 3 本とも `InterpTrackSound_0` の t=0 に `Evil_Monkey_Scream`（音量 1・ピッチ 1） | t=0 に 1 本。省いた分（MJ2 のナイフ 0.4837 音量 **1.7**、MJ3 のナイフ v1 2.4375 音量 **1.5**、小さな悲鳴 4 本 0.4796/0.8424/0.947/1.1007 音量 0.5・ピッチ 1.1/0.95/1/1.05）は本作に当たる動きが無い |
| 天井灯 | `ceilinglights_80`: 1500 Unitless・届く距離 500・源 24.715・色 (255,236,142)・影あり | 同値（位置は場面の倍率 0.878 で） |
| 黒い板 | `jumpscareblock`〜`_4`: `/Engine/BasicShapes/Plane` + `BlackUnlitMaterial`・`CastShadow` false | 同じ組み合わせで 6 枚 |
| 揺れ | `ClientPlayCameraShake(JumpscareShake, 0.3)` を 1 回 | `ShakeScale = 0.3` で 1 回 |
| アニメの再生 | `InterpTrackAnimControl`: `StartTime` 0・`AnimStartOffset` 0・`AnimPlayRate` 1・ループ無し（MJ1 `Monkey_Killshot_01`、MJ2 `_02`、MJ3 `_03a`） | 本作は `ClipStarts` と `StartRate` で時間を歪める（ユーザーの指示「最初は高速で、イージングで等速に」）→ **そのまま残す** |
| 03_Watcher_Kill3 | トラックは Move 1 本だけ（長さ 1.0187・FOV も PostProcess も無し）。`AxeKill` 0.5107・`Kill2` 0.8249 も同じ形 | `WatcherKill3Length` と位置・回転のキー。使うのは Kill3 だけ（BP の掴み笑いの kill に対応） |

### B. 差があり埋める

| 項目 | 原作の値 | 本作 | 対応 |
| --- | --- | --- | --- |
| 暗転の曲線 | `InterpTrackFade_0` の 2 キーは `CIM_CurveAutoClamped` で**接線が両端 0** → 三次エルミート = smoothstep | `StartCameraFade` の**直線**（`StartFade` に TODO(仮) つき） | ステップ 2 |
| 被写界深度 | `JumpscareCam` が `FocalDistance` 142.943→10（0.128 で保持を抜け 0.2248 で到達）・`FocalRegion` 571.429→100（0→0.2315）・`FarBlurSize` 16.152・Near/FarTransitionRegion は既定（300/500）を上書き。方式は 01_Hotel の **bUnbound な `PostProcessVolume_1`** が `DOFM_Gaussian`・`Fstop` 4.0 を敷き、カメラは方式を上書きしないので **Gaussian** | **何も入れていない** | ステップ 3 |
| 顔の灯 | `jumpscarelight`・`jumpscarelight_5`: 猿の頭のソケット `Monkey_Head_TopSHJnt` に付く点光源 2 灯、強さ **15**・届く距離 **15**・影なし・ソケット系で ±4.416/8.515/11.875 と ±4.399/8.324/11.747（猿のスケール 4 なので世界では ±17.7 / 34.1 / 47.2 cm） | 1 灯の仮（強さ 300・届く距離 200・頭 +(50,0,30)） | ステップ 4 |

### C. 理由を書いて閉じる

| 項目 | 分かったこと | どうするか |
| --- | --- | --- |
| MJ3 の静止 | MJ3 の 5 体の `Monkey_Killshot_03a*` は**遠くの別の黒箱**（X 9306〜9444・Y 1514〜1781・Z 13058〜13187、`jumpscareblock*` 5 枚に囲まれる。いずれも `bVisible` false で待機）で動き、`JumpscareMonkey` は 1 キーで **(0,0,0) に送られる**。カメラは 0.204〜2.572 秒を部屋 1 で静止 → **原作の MJ3 はカメラに何も写らない**（音だけ） | 本作の仮（MJ1 の揺れを 1.7966 倍に伸ばす）を残す。原作に倣うと真っ黒になる |
| 舞台 | 原作の舞台（`JumpscareMonkey` の足元 4737.98/1072.72/6917.72）は黒箱ではなく**ホテルの実部屋**（同じ場所に `hotel_door_hotel_door36`・`hotel_frame_264`・`CeilingLight122` のメッシュとシャード 2 個）。背景を潰していたのは Gaussian DOF | 本作の黒い部屋は項目 9 の設計として残す（病院に同じ部屋は無い） |
| 部屋の大きさ | 原作の黒箱は床 11.25×11.5 m・壁 7.5×6.5 m（Plane のスケール） | 本作の 20 m 立方を残す（Capture_2 が 7.75 m 下がる） |
| `JumpscareOffset` | 保存された位置は古く、トラックの値と Y で 500・X で 60.8 ずれる。トラックは世界座標（天井灯の実座標と一致）で、実装は t=0 のキーから組む | 影響なし |

## 計画

- [x] 1. 原作の 6 本と関係アクタの値を全部書き出し、突き合わせ表を作る（上の表。残りのステップもここで立て直した）
- [ ] 2. 暗転を原作の曲線にする ← 次
  - `StartFade` の `StartCameraFade`（直線）をやめ、Matinee と同じ三次エルミート（両端の接線 0 = smoothstep）で毎フレーム `SetManualCameraFade` する。終わったら黒を保持（`bPersistFade` = 保持つき）。顔（`FadeDuration` 0）は即黒のまま
  - 変更予定: `Source/wasami_deception/WasamiCapture.h`・`.cpp`、`Tests/WasamiCaptureTests.cpp`（曲線の値の検査）
- [ ] 3. 被写界深度を入れる
  - UE 5.8 の Gaussian 系（`FocalRegion`・`Near/FarTransitionRegion`・`FarBlurSize`）は **Mobile 専用**（`Engine/Source/Runtime/Engine/Classes/Engine/Scene.h` の 2363〜 が `Lens|Mobile Depth of Field`）で desktop では効かない。cinematic の `DepthOfFieldFocalDistance` + `Fstop` に写す
  - 仮の対応: `FocalDistance` = 原作の（焦点 + 領域 ÷ 2）（= 428.66 → 60 cm。Gaussian のはっきり写る帯の中央）× カメラの倍率 `FrameScale`、`Fstop` = 4.0（`PostProcessVolume_1` の値）。時刻は原作のキーのまま（Matinee の時間で評価）
  - 変更予定: `WasamiCapture.h`・`.cpp`、`Tests/WasamiCaptureTests.cpp`
- [ ] 4. 灯を原作の値にする
  - 顔の灯を `jumpscarelight`・`_5` の 2 灯に置き換える（強さ 15・届く距離 15・影なし）。位置は猿の頭のソケット系の値を本作の `head` の骨へ写す（軸の対応が決まらなければ左右 ±17.7・前 と 上 の仮を理由つきで書く）。1 灯の仮（`FaceLightOffset`・`FaceLightIntensity`・`FaceLightRadius`）は捨てる
  - 天井灯の届く距離と強さを場面の倍率（0.878）に合わせるかを決める（今は位置だけ倍率、距離・強さは原作のまま）
  - 変更予定: `WasamiCapture.h`・`.cpp`、`Tests/WasamiCaptureTests.cpp`
- [ ] 5. PIE で 4 本を通して確かめ、取り込んだ `JumpscareShake` が原作の値（持続 1.0・ブレンドイン 0.2・アウト 0、回転 pitch 2/40Hz・yaw 2/50・roll 4/35、FOV 0.1/3）と合うかを見て、実装記録 07 と作業一覧の項目 32 を締める（`check_records.py --update`）

## 次にやること

ステップ 2（暗転の曲線）。`Source/wasami_deception/WasamiCapture.cpp` の `StartFade`（709 行の手前）が `StartCameraFade` で直線に暗くしている。原作の 2 キーは接線 0 の `CIM_CurveAutoClamped` なので `FMath::CubicInterp(0, 0, 1, 0, alpha)`（= smoothstep）で毎フレーム `SetManualCameraFade(Amount, FLinearColor::Black, false)` に置き換える。暗転の始まりと長さ（`FadeStart`・`FadeDuration`）は今の計算のまま。

## 決定事項

- 2026-09-21: 本家の実機の収録は**最後の手段**にする — 大目標 3 の節の頭（2026-09-21 のユーザーの回答）。ステップ 1 で全部コードから決まったので、今のところ収録は要らない。
- 2026-09-21: 原作の被写界深度は Gaussian（レベル全体に敷かれた `PostProcessVolume_1` が方式を決め、カメラは焦点だけ上書き）。UE 5.8 の desktop に Gaussian は無いので cinematic に写す（上のステップ 3 の仮）。値をそのまま入れると焦点 10 cm で顔までぼけるため、はっきり写る帯（焦点〜焦点＋領域）の中央を焦点にする。

## 要確認（ユーザー）

- 被写界深度の写し方（ステップ 3 の仮）: 原作の Gaussian（焦点 10 cm・はっきり写る帯 100 cm・遠景のぼけ 16 px）は UE 5.8 の desktop に無い。帯の中央（60 cm）を cinematic の焦点にし、絞りは原作のレベルの 4.0 にする。原作は背景（ホテルの実部屋）を潰すためのぼけだが、本作の部屋は黒いので効き目はワサミの体にだけ出る。入れない選択もある。
- 天井灯の届く距離（500）と強さ（1500）を場面の倍率 0.878 に合わせるか（合わせないと被写体が 30 % ほど明るい）。ステップ 4 で仮に決める。

## 再開時の注意

- 長時間処理は無い。ステップ 2〜4 は C++ の変更なので、書き終えたら `python Tools/editor_cycle.py` でビルドして開き直す（尋ねずに走らせてよい）。
- 本作の捕獲の実装は `Source/wasami_deception/WasamiCapture.h`・`.cpp`、説明は実装記録 07 の「捕獲の演出」。テストは `Source/wasami_deception/Tests/WasamiCaptureTests.cpp`。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
