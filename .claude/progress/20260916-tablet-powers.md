---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-16 22:52
---

# タブレットから使える特殊効果（パワー）をすべて実装する

## 依頼

2026-09-16 のユーザーの依頼（原文）:「次のタスクは、タブレットより利用可能な全ての特殊効果（テレポーテーション、スピードブースト、テレキネシス等々）を実装することです。実装に必要なステップを見積もり、着手してください。ただし、テレポーテーションの実装に限りpak_referenceを尊重するものとします。」

- 本家のパワーは 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）。調査の結果は `.claude/references/powers/`（README が索引と要点、01〜04 が根拠つきの本文）。
- テレポーテーションだけは `pak_reference/`（旧版）に従う。それ以外と全パワー共通の仕組みは `pak_reference_2/`（最新版）に従う。

## 計画

各ステップの終わりにコミットし、記録を直して、ユーザーに `/clear` をお願いする。

- [x] 1. 原作データの調査をまとめる（`.claude/references/powers/`）、ユーザーに 4 点を確認
- [x] 2. パワーの土台（C++）… 2026-09-16 完了。`UWasamiPowerComponent`（枠・Q/E/1/2・2 段の連打防止・ゲージ・強化段階の表・死亡のリセット・スピードブースト Lv5）、敵とシャードのインターフェース、タブレットの 6 アイコンと弾み、パワーの音とシェイクの取り込み、パイプラインの素材のソフト参照化（エディタが落ちた件の対策）。実装記録は 04 を新設し、00〜03 を直した
- [x] 3. スピードブーストの演出 … 2026-09-16 完了。`CameraAnim_SpeedBoost` の赤い色調（UE4 の CameraAnim を C++ で再生する `UWasamiCameraAnim` / `UWasamiCameraAnimModifier` を新設）、`UMG_SpeedBoost`（`UWasamiSpeedBoostWidget`、原作のグラフどおりの `M_Speedlines` と `T_VignetteNew`）、FX（`UWasamiChameleonComponent`、推定の `M_DD_ChameleonCameraShake`）。放射ブラーは本家で無効なので作らない。取り込みのテクスチャとマスターの作り方を `dd_assets` に共通化。実装記録は 01〜04 と索引を直した
- [x] 4a. テレポーテーションの仕組み（旧版）… 2026-09-16 完了。Teleport チャンネル（旧版の既定 Overlap）、病院のゾーン（床のメッシュと救急車の屋根の箱をクラスの `Cube` の値で。前処理の `teleport_zones`・組み立ての `_set_collision`）、照準のアクタ `AWasamiTeleportAim`（SpringArm のラグ・500 cm の下向きトレース・ホイール・左クリック。デカールは UE の既定の材質のまま）、0.12 s 後のスイープ移動、シェイク、音 3 つ（エンジンの音は `/Game/DD/_Engine/…`）、同じ側の Q / E での取り消し、再使用 5 s、死亡のリセット、テスト `Wasami.Powers.TeleportDistance`。両ゾーンを組み立て直して High で焼き直した。実装記録は 00・01・02・04 と索引、検証のガイドを直した
- [ ] 4b. テレポーテーションのカメラアニメ: `CameraAnim_Teleport`（旧版）の取り込みと再生（ステップ 3 の `UWasamiCameraAnimModifier`。露出・色調はそのまま動く）、FOV のトラックの再生（基準が `BaseFOV` 137.24 か t=0 のキーかを決める）、+100 EV の白が UE 5.8 で壊れないかの確認、PIE での通しの確認
- [ ] 5. テレポーテーションの見た目: デカール `M_Decal_Teleport`（推定）、パーティクル `P_ky_cutter2`（Cascade を Python で組めるか確かめる。だめなら Niagara）
- [ ] 6. 一瞬の演出の共通部品（全画面のポストプロセス 2 つとカーブ）と Primal Fear（半径 3500 の重なり判定で敵に `Set State(Stun)`、球 `M_05_Primal`、シェイク `ElevatorShakeStop` ×25、`Stun_Wave_Attack_New_04`、再使用 23 s）
- [ ] 7. Vanish（カプセルの Camera 応答、`Player Vanish` の通知、`UMG_Vanish` と `MM_WobblyVignette`、ポストプロセス、煙 `PPP_VanishPuff`、15 s・再使用 15 s）
- [ ] 8. Telepathy（`BP_Telepathy` とトラッカー、`UMG_TelepathyTracker` と `MM_Telepathy`、開始と終わりの音、シェイク、9 s・再使用 6.5 s）
- [ ] 9. シャードの最小限（M3 の前倒し。最新版の `BP_Shard`）: 病院の配置（Zone 1 は 337、Zone 2 は 342）、回収（数・音 `Soul_Shard_Pickup_v2_Cue` と同時発音 `OnlyFew`・シェイク `BP_CameraShake_ShardCollect`・閃光 `P_ky_flash3`）、`Activate` の引き寄せ（`Shard Pull`、ExpoIn）、タブレットの数と地図
- [ ] 10. テレキネシス（半径 3000 の重なり判定、`P_ky_forceField_Telekinesis`、ポストプロセス、シェイク、音、再使用 8 s）
- [ ] 11. 実機との見比べ（推定したマテリアルとパーティクル）と PIE での確認（エディタの PIE と本家の起動はユーザーの確認を取る）
- [ ] 12. 仕上げ: 実装記録（04 はステップ 2 で新設し、各ステップで書き足す）、handover の「現状と次の一歩」、main へマージして push、この記録を消す

## 次にやること

ステップ 4b（テレポーテーションのカメラアニメ）。まず `.claude/references/powers/02-teleport.md` の §5.1（`CameraAnim_Teleport` のキーと「FOV の基準（未確定）」）と §7-1・§7-3、実装記録 `04-powers.md` の「カメラアニメの再生」「テレポーテーション」を読む。
1. **FOV の基準を手元の旧版で決める**（ユーザーの回答）: エディタを閉じて（`python Tools/editor_cycle.py --quit-only`。閉じる前にユーザーの確認）、`python Tools/console_session.py C:\Users\User\AppData\Local\DDeception\Launch-Classic-Ch3.cmd` で旧版を起動し（起動もユーザーの確認を取る。`.claude/guides/verification.md`・`original-fidelity.md` の「本家のゲームを手元で動かす」）、テレポーテーションを使った最初の数フレームを撮って、t=0 で画面が急に狭まる（FOV 90 → 約 43。`BaseFOV` 137.24 が基準）か、広がり始める（90 → 150。t=0 のキー 90 が基準）かを見る。撮影は 1 回に数秒かかるので、`desktop.py shot` を連続で撮るか、画面収録の手段を先に考える。
2. `dd_powers` の `CAMERA_ANIMS` に `(1, "Animation/Camera/CameraAnim_Teleport")` を足して取り込む。`UWasamiCameraAnim` に FOV のトラックの再生を足す（基準の決め方は 1 の結果。`UWasamiCameraAnimModifier::ModifyCamera` で `POV.FOV += (キー − 基準) × 重み`、5〜170 に収める。いまの `Play` は FOV のトラックがあると警告を出す）。
3. `AWasamiTeleportAim::Confirm` の DoOnce の中で `Play(CameraAnim_Teleport, 1, 1, 0, 0, loop なし, Duration 0)`（CameraLocal）。取り消しでもカメラアニメは止めない（カメラマネージャ側。本家どおり）。
4. PIE で通し（+100 EV の白が UE 5.8 のプレエクスポージャや TSR の履歴で壊れないか。02-teleport.md §7-3）。テストに FOV の値を足す。

## 決定事項

- 2026-09-16: テレキネシスを実装する — これまでの「Telekinesis は実装しない」（WebGL 版のユーザーの指定）を、今回の依頼がテレキネシスを名指ししているので取り下げる。
- 2026-09-16: テレポーテーション以外のパワーと共通の仕組みは `pak_reference_2` に従う — 依頼の「テレポーテーションの実装に限り pak_reference を尊重する」から。
- 2026-09-16: 大規模改修なので `feature/tablet-powers` ブランチで進める（`.claude/guides/git-workflow.md`）。
- 2026-09-16（ユーザーの回答）: 強化段階は **Lv5（最大）** に固定する。本作にリングの祭壇が無いため。
- 2026-09-16（ユーザーの回答）: スピードブーストの再使用は**最新版のコード**の値（表示より 1 s 長い。Lv5 で 7.5 s）。
- 2026-09-16（ユーザーの回答）: テレキネシスを確かめるため、**シャードの最小限を先に作る**（M3 の前倒し。ステップ 9）。
- 2026-09-16（ユーザーの回答）: パワーが使えないときは**どのパワーも無音**（最新版どおり）。テレポートの照準中に同じ側の Q で取り消す処理は旧版から採る。
- 2026-09-16: 枠の初期値は原作のコードどおり左右とも添字 0（解放済みの先頭 = Speed Boost）。原作の `Selected Power Left/Right` は GameInstance の値でディスクに保存されないので、本家を起動して病院に入ったときも左右とも Speed Boost になるはず（実機では未確認）。
- 2026-09-16: 敵（M4）はまだ無いので、Primal Fear・Vanish・Telepathy は受け口のインターフェースまで作り、確認は仮の的（インターフェースを実装したテスト用のアクタ）で行う。
- 2026-09-16（ステップ 2 の構成）: 原作の「プレイヤーのパワーの処理」「`BP_Powers`（ゲージのタイムライン 6 本）」「`UMG_TabletPowers` の `Power` 配列と左右の添字」を 1 つの `UWasamiPowerComponent`（プレイヤーに付ける）にまとめる。原作は 3 か所に分けているが、どれもプレイヤー 1 人に 1 つで、互いに直接呼び合うだけなので分ける意味が無い。タブレットのウィジェットは表示だけ（6 アイコンの MID・左右の枠の出し分け・弾む演出）を受け持つ。
- 2026-09-16: 原作の `Delay` ノード（動いている間に呼び直しても無視される）は、`FTimerManager` の「動いていなければ仕掛ける」で写す。死亡のリセットの後に古い Delay が残って早く切れる、といった原作の癖もそのまま出る。
- 2026-09-16: Telepathy・Primal・Telekinesis・Vanish のクールダウン明けの Gate は「Open の直後に Enter」（@6301〜@6444 ほか）なので、閉じても次の Enter の前に必ず開く＝素通しと同じ。Gate の状態は持たず、「明けたら充填」「リセットは充填」とする。リセットは使っていないときも充填を通るので `power_refilled` が鳴る（Reset Primal = @37261: Push @6444 → @27788 の Close）。スピードブースト（DoOnce_3 / DoOnce_4）とテレポート（DoOnce_5）は Start Closed で、使った後だけ効く（@1211・@3345）。
- 2026-09-16: 枠を弾ませる `Use Left` / `Use Right` は、原作のキー（ティック 0 / 3000 / 9000 / 30000 → 1.0 / 1.25 / 1.10 / 1.0）と書き出された接線（1 ティックあたり 1.1111e-5・−9.2593e-6。秒に直すと 0.6667・−0.5556）を `FRichCurve` にして評価し、枠の画像の `RenderTransform` の拡縮に入れる（UMG のアニメを C++ で作らない）。時間はプレイヤーのティックで進める。
- 2026-09-16: パワーの新しい音と素材は両版で同一（アイコン 12 枚の PNG・テクスチャ設定、`power_refilled`・`Shard_Streak_Milestone_V5`・`UI_Select_V3` の ogg、`BP_CameraShake_Streak` の値を突き合わせた）。`MM_Powers_*` のインスタンス名は 6 つそろっている最新版の名前を使う。`Shard_Streak_Milestone_V5` の SoundWave は音量 0.7・ピッチ 2.0、最新版だけ同時発音 `NewSoundConcurrency`（MaxCount 2・VolumeScale 0.5）を持つので、それも写す（SoundWave の値は書き出しから読む汎用の取り込みにする）。
- 2026-09-16: 本家の変数 `Has Input`・`Can Interact?`（既定 true。台本が切り替える）をプレイヤーに足す。`Use Power` は `Can Use Tablet?` と `Has Input`、Q / E は `Can Interact?` を見る。
- 2026-09-16: ステップ 2 では、スピードブースト以外のパワーは枠を弾ませて連打防止を通り `OnPowerUsed` を出すところまで（使える状態は変えない）。各パワーの中身は後のステップで足す。テレポートのリセットの残り（照準のアクタを消す・`UsedTeleport`）もステップ 4 で足す。
- 2026-09-16（ユーザーの回答）: 落ちたエディタの起動し直しと、素材の取り込み・テスト・PIE の確認（エディタへの Q/E/1/2/Space の入力を含む）まで進めてよい。
- 2026-09-16: パイプラインが作る素材は C++ のコンストラクタで読まず、ソフト参照（`TSoftObjectPtr` / `TSoftClassPtr` の UPROPERTY に既定のパスを入れる）にして BeginPlay / RebuildWidget で読む。起動時に読むとルートに入り、以後の作り直しでエディタが落ちるため（再開時の注意）。この先のステップで推定のマテリアルを何度も作り直すので、ここで直しておく。
- 2026-09-16: テストは `Source/wasami_deception/Tests/` に置き、モジュールのヘッダーは `../` で読む（Build.cs に include パスを足すより変更が小さい）。
- 2026-09-16（ユーザーの回答、ステップ 3）: エディタを閉じてビルドし開き直すこと、取り込み・テスト・PIE の確認（エディタへの Q/E の入力を含む）まで進めてよい。
- 2026-09-16: 実装記録は、パワーの仕組みを `04-powers.md` として**このステップで新設**する（新しいソースはどれかの記録に載せる決まりのため）。索引の予定表の番号は 1 つずつ後ろにずらす。
- 2026-09-16（ステップ 3 の調査）: **放射ブラーは作らない**。最新版のプレイヤーの子アクタ `FX`（`Chameleon`）は `Radial Blur`（有効フラグ）が CDO で false、プレイヤーのテンプレートは `Custom Depth Highlighter` 系しか上書きせず、`Radial Blur` を true にするコードもどこにも無い（`Chameleon_C.Radial Blur` を書くのはプレイヤーの `Radial Blur Width` だけ）。`InitChameleon` は無効な効果の関数を素通りするので、ブースト中の `Radial Blur Width` の書き込みは絵に出ない。効くのは `Camera Shake`（ブースト中だけ true、`ShakePower` = 速さ 0〜870 → 0〜0.003、`ShakeFQ` → 0〜15）だけ。
- 2026-09-16（ステップ 3）: Chameleon は「プレイヤーに付けた範囲なしの PostProcess に、有効な効果の MID をブレンダブル（重み 1）で足す」作り（`InitChameleon` → `Set Advanced Effect Features` → `AddOrUpdateBlendable`、`ApplyChameleonSettings` が毎ティック設定を上書きして外す）。本作は `UWasamiChameleonComponent`（アクタコンポーネント）をプレイヤーに付け、`BeginPlay` で範囲なしの `UPostProcessComponent` を作って持ち、`bCameraShake` が真の間だけ揺れのマテリアルの MID を足す（`UPostProcessComponent` を継ぐ案は MinimalAPI でリンクできず取りやめた）。揺れの詳細設定は CDO の既定（ブレンド Normal・不透明度 1・カスタム深度なし）なので効果をそのまま出す。`Native Post Process` は上書きなし。
- 2026-09-16（ステップ 3）: `M_CameraShake`（Chameleon）はグラフの大半（HLSL・数式・シーンテクスチャ）が書き出されていない（残っているのは `ShakePower` 既定 0.01・`ShakeFQ` 既定 50・`MakeFloat2` 1 つ・`MF_SetBlending`・`MF_DepthOnlyMasking`）。**推定**で `/Game/Pipeline/Materials/M_DD_ChameleonCameraShake` を作る: 画面の UV を `MakeFloat2(sin(Time×FQ), cos(Time×FQ)) × Power` だけずらして PostProcessInput0 を読む（UE の Sine/Cosine は周期 1 = FQ Hz）。ステップ 11 で実機と見比べる。
- 2026-09-16（ステップ 3）: `M_Speedlines` は書き出しにグラフが残っている（`FlipBook` 関数の呼び出しを入力なしで置き、その出力 2 番を `T_Speedlines` の UV に、RGB を Emissive へ。UI・Translucent、Opacity は未接続 = 1）。UE 5.8 の `FlipBook` は 4.24 の書き出しと同じ 38 ノードで、入力の既定は列 2・行 2・位相 Time（関数の中で frac）・UV TexCoord0。出力の並びは `SortPriority` だけで決まる（4.24 と同じ比較）ので、同じ関数を既定のまま呼び出力 2 番をつなげば原作と同じ絵になる → 推定ではなく原作どおりとして `/Game/DD/UI/Main/Powers/M_Speedlines` に作る。UI マテリアルの色はウィジェットの色と不透明度（頂点カラー）が掛かる（UE 5.8 `SlateElementPixelShader.usf` の `GetColor`）ので、赤・不透明度 0〜0.15 がそのまま効く。
- 2026-09-16（ステップ 3）: `T_Speedlines`（3841×5404）は原作でも非圧縮 BGRA8・ミップ 1（`_textures.json`）。性能のガイドの「素材の設定は原作のまま」に従い TC_Default・sRGB・UI のまま取り込み、実際の大きさを測って記録する。
- 2026-09-16（ステップ 3）: **カメラアニメは C++ の汎用の再生で写す**（ステップ 4 の `CameraAnim_Teleport` も同じ部品で再生する）。UE 5.8 に `UCameraAnim` は無く、後継の `UCameraAnimationSequence` は Python から PP のトラックを組むのが難しく確かめにくい。データは取り込みで `UWasamiCameraAnim`（データアセット: 長さ・BaseFOV・基準の PP・重み・PP のトラックのキーと接線）に写し、`UWasamiCameraAnimModifier`（`UCameraModifier`）が再生する。評価は UE4 の Matinee と同じ `FInterpCurve`（保存された接線のまま）。ブレンドは UE 5.8 の後継（`CameraAnimationCameraModifier.cpp`）と同じ形（直線、ブレンドインとアウトの重みの小さい方、PP は `VTBlendOrder_Base` = 通常のカメラの下）。`Duration` は「ブレンドを含む長さ」（UE 5.8 の `DurationOverride` の説明。UE4 の実装は手元に無い）とし、`Duration − BlendOut` で止め始める。FOV のトラックはステップ 4 で足す（基準の FOV が未確定のため）。Move トラック（どちらのアニメも原点の 1 キー）は扱わない。基準の PP の `bOverride_FilmWhitePoint` は UE5 に無い（既定値の中立なので落としても同じ）。
- 2026-09-16（ステップ 3）: ブーストの演出の出し入れは原作どおり。使ったとき `PlayCameraAnim(CameraAnim_SpeedBoost, 1, 1, 0.5, 0.5, loop なし, Duration = 効果時間)`・`Sprinting Effects` のタイマー（0.001 s）と `Camera Shake = true`・`UMG_SpeedBoost` を `AddToViewport(1)`。終わり（@30）でタイマーを止め `Camera Shake = false`・ウィジェットを外す（カメラアニメは止めない。効果時間で自然に終わる）。死亡のリセット（@37067）だけカメラアニメを `Stop(true)` で即座に止めてから終わりの処理へ。
- 2026-09-16（ステップ 3 で見つけたこと。M4 向け）: プレイヤーの Chameleon はテンプレートで `Custom Depth Highlighter (Clip)` が常に有効（縁取りの色 (1,0,0)、中の色 (0.0802,0,0)）。病院の `BP_06_ReaperNurse` を含む敵が `SetRenderCustomDepth` を呼ぶので、敵の赤い縁取りはこの仕組み。敵を作るときに足す。
- 2026-09-16（ステップ 4 の分け方）: 規模が大きいので 4a（仕組み）と 4b（カメラアニメと FOV）に分ける。FOV の基準が未確定なので、それに依らない部分を先に 1 コミットにする。
- 2026-09-16（ステップ 4）: FOV の基準（`CameraAnim_Teleport` の `BaseFOV` 137.24 か t=0 のキー 90 か）は、UE 4.21 の `CameraAnimInst.cpp` が手元に無く（`pak_reference`・`pak_reference_2` の C++ は書き出しの道具 cue4parse の同梱物だけ。`gh` も無い）、Epic の API ドキュメントは 403、Web の要約は「time 0.0 からの差分」とだけ言うので決まらない。**ユーザーの回答: ステップ 4b で手元の旧版を観察して決める**（エディタを閉じてから `Launch-Classic-Ch3.cmd`。起動はその時にもう一度確認する）。
- 2026-09-16（ユーザーの回答、ステップ 4a）: エディタを閉じてビルドし開き直すこと、取り込みと病院の 2 つのレベルの組み立て直し、テストと PIE の確認（エディタへの Q・左クリック・ホイールの入力を含む）まで進めてよい。

## 再開時の注意

- **2026-09-16 22:52（ステップ 4a の終わり）**: エディタは起動している（PID 28356、セッション 1、`L_Hospital_Zone1`、PIE は止めた、未保存なし）。操作エージェントは止めた。MCP は開き直しで切れたまま（`Tools/ue_remote.py` で同じことができる）。両ゾーンは組み立て直して High で焼いてある（Zone 1 は 80.3 秒、Zone 2 は 37.1 秒）。PIE のプレイヤースタートは 4 つからランダムに選ばれる（今回は (−25, 3735) や (15, 385)・南向き。前回の記録の (5620, −23410) とは限らない）。
- **組み立て直すと焼き込みが外れる**（メッシュを作り直すため）。`build_dd_stage_level` の後は `observations/tools/bake_level.py` で High 品質で焼き直す。
- **独自の当たりチャンネルは Python では設定の名前で出る**（`unreal.CollisionChannel.ECC_TELEPORT`。`ECC_GAME_TRACE_CHANNEL1` は無い）。
- 以下は 4a の途中の記録（手順は済んだ）。
- **2026-09-16 22:20 ごろ（ステップ 4a の途中）**: コード（C++・ini・前処理・組み立て・音の取り込み）は書き終えた。前処理は走らせ済み（`stage_ue.json` の差分はゾーンの 4 か所だけ。直前の出力の控えは scratchpad にあったが、セッションごとに消える）。続きの手順: (1) `python Tools/editor_cycle.py`（エディタを閉じてビルドし開き直す。当たりのチャンネルの ini も開き直しで読まれる）→ 完了は `ue_remote.py` が答え、`unreal.WasamiTeleportAim` があること。(2) `WasamiDDTools.import_dd_powers()`（音が 5 つになる）。(3) `WasamiStageTools.build_dd_stage_level` を Zone1・Zone2（組み立て直し。ゾーンの部品の当たりを読み戻す）。(4) `Automation RunTests Wasami`（6 件）。(5) PIE（Q で照準・ホイール・クリックで移動・同じ側の Q で取り消し・再使用 5 s）。Python の編集は `open(p, 'w')` だと改行が CRLF になるので、`newline=''` か Edit を使う（一度 CRLF にして LF に戻した）。
  - 22:30 ごろ: (1) ビルド済み（警告なし、エディタは開き直した）、(2) 取り込み済み（音 5、読み戻しも原作どおり）、実装記録 00・02・04 は書いた（01 と索引はまだ）。**(3) の組み立て直しはメッシュを作り直すので焼き込みが外れる**。組み立ての後に `observations/tools/bake_level.py` で High 品質で焼き直す（`python Tools/ue_remote.py -c "$(cat observations/tools/bake_level.py; echo; echo "main('L_Hospital_Zone1', 'QUALITY_HIGH')")"`。Zone 1 は約 106 秒、Zone 2 は約 48 秒）。完了の確かめ方: 組み立ての戻り値の `meshes` が Zone 1 で 924・Zone 2 で 820、焼いた後に `L_Hospital_Zone*_BuiltData` が更新され、未保存が 0。最後に Zone 1 を開いておく。
- 2026-09-16 22:10 の時点（ステップ 3 の終わり）で、エディタは起動している（PID 24460、セッション 1、`L_Hospital_Zone1`、PIE は止めた、未保存の変更なし）。ビルドは済み（ステップ 3 の最後にテストだけ Live Coding で直した。次のフルビルドで取り込まれる）。操作エージェント（`Tools/desktop.py`）は止めた。MCP は開き直しで切れたまま（`Tools/ue_remote.py` で同じことができる）。
- **C++ で `UPostProcessComponent`・`ULegacyCameraShake` を継がない**（MinimalAPI で他のモジュールからはリンクできない。ステップ 3 で一度ビルドが落ちた）。
- PIE で「走りながら」の絵を撮るときは、エディタの Python で `unreal.register_slate_post_tick_callback` に `player.add_movement_input(forward, 1.0, False)` を入れて前進させ、その間に `Tools/desktop.py shot` で撮る（`hold w` の間はエージェントが撮影できない）。終わったら `unregister_slate_post_tick_callback`。PIE の `shot showui` はエディタ全体を撮りビューポートが黒くなるので使えない。画面に載ったウィジェットは `unreal.WidgetLibrary.get_all_widgets_of_class(world, cls, True)`（False だと外したものも GC まで数える）。`unreal.Rotator` はキーワード（`roll=, pitch=, yaw=`）で作る。開始地点は (5620, −23410, 90.15)・ヨー 180 で、前方（駐車場）に 30 m ほど走れる。
- エディタが背面だと Automation テストは「10 FPS 待ち」で止まる（3 FPS）。エディタが前面になると進む。
- **エディタが落ちた件（20:36）は解決済み**: C++ のコンストラクタで読んだ素材は起動時にルートに入り、作り直すと `!IsRooted()` で落ちる。C++ はパイプラインの素材をソフト参照で持つように直し（00 記録）、再起動後に `obj refs` で `M_DD_MapPlane` がルートに入っていないこと（起動時に読まれる `WorldGridMaterial` には `(root)` が付く）を確かめてから取り込みをやり直した。以後、C++ に素材を足すときは `ConstructorHelpers` を使わない。
- `Tools/desktop.py` で PIE に入力を送るときは、ビューポートを 1 回クリックして焦点を渡してからキーを送る（クリックしないとキーが届かなかった）。PIE を始めた直後に前面にある窓がテストのログなどのときは、先に閉じる。
- 画面の撮影 1 回に数秒かかるので、時間に依存する確認は `unreal.GameplayStatics.get_time_seconds` と一緒に読む（scratchpad の `probe_powers.py` がパワーの状態を読む。scratchpad はセッションごとなので、要るときは作り直す）。

## 検証

- ステップ 4a: C++ ビルド成功（警告なし）、`Automation RunTests Wasami` 6 件成功（`Wasami.Powers.TeleportDistance` を新設）、check_records OK（00〜04）、前処理の差分はゾーンの 4 か所だけ、取り込み `import_dd_powers`（音 5。テレポートの 3 つの長さ・ループを読み戻した）、両ゾーンの組み立て直し（Zone 1 は配置 924、Zone 2 は 820）とゾーンの当たりの読み戻し（開き直しても保たれる）、High での焼き直し、PIE で照準・切り替えの停止・ホイール・クリックでの移動（0.12 s + 1 フレーム、カプセルの Ignore → Block）・再使用 5 s・同じ側での取り消し・逆側では取り消さない・リセット（照準中、クリックの直後）を確かめた（詳細は 04 記録の「確かめたこと」）

- ステップ 3: C++ ビルド成功（警告なし）、`Automation RunTests Wasami` 5 件成功（`Wasami.CameraAnim.Playback`・`Tracks` を新設）、check_records OK（00〜04）、取り込み `import_dd_powers`（音 2・シェイク 1・カメラアニメ 1・テクスチャ 2・マテリアル 2）と `import_dd_tablet`（前回と同じ数）、PIE でブーストの色調・集中線とビネット・揺れの値（950 cm/s で 0.003 / 15）・終わりの片付け・リセットの即時停止を確かめた（詳細は 04 記録の「確かめたこと」）

- check_records: OK（ステップ 2。00〜04 の 5 件）
- C++ ビルド: 成功（ステップ 2、警告なし）
- Automation テスト: `Wasami.Powers`（Gauge・Tuning・SocketBounce）3 件すべて成功
- 取り込み: `import_dd_tablet`（テクスチャ 25・マテリアル 2・メッシュ 1・フォント 1・音 3・ミニマップとアイコン 8）、`import_dd_powers`（音 2・シェイク 1）。音量・ピッチ・同時発音・シェイクの値・インスタンスのテクスチャを読み戻して原作の値どおり。`Failed to compile` なし
- PIE（ユーザーの了承のうえで入力を送った）: E / Q でブースト（950・約 2 秒でゲージ 0.796・約 18 秒後には戻る）、タブレットの枠のアイコンが扇形に減る、1 / 2 の巡回と末尾からの折り返し、6 種のアイコンの表示、タブレットを下ろすと巡回しない、未実装の Teleport は使える状態のまま、`ResetPowers` で即座に戻る。PIE は止めた
