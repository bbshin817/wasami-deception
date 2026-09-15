---
title: HUD・手持ちタブレット（3D タブレット / DOM HUD / スタイル）
sources:
  - src/hud/tablet.ts
  - src/hud/hud.ts
  - src/styles.css
  - src/hud/title.ts
  - src/hud/options.ts
  - src/hud/pause.ts
  - src/hud/death.ts
  - src/hud/stage.ts
  - tests/title.test.ts
  - tests/options.test.ts
  - tests/pause.test.ts
  - tests/death.test.ts
  - src/hud/tablet-anim.ts
  - tests/tablet-anim.test.ts
  - src/hud/stage-intro.ts
  - tests/stage-intro.test.ts
  - src/hud/streak.ts
  - tests/streak.test.ts
  - src/hud/vignette-sides.ts
  - tests/vignette-sides.test.ts
  - src/hud/level-clear.ts
  - tests/level-clear.test.ts
updated: 2026-09-15
---

# HUD・手持ちタブレット

## 役割
- `Tablet`（tablet.ts）: Dark Deception 風の手持ち 3D タブレット。カメラに親子付けした iPhone 12 以降風（平らな側面 + 小さな縁 R）の黒い板（前面は黒いサテンの非金属、縁と背面は暗いガンメタル）+ 黒ガラス画面で、画面内容（上部の帯と 2 つのパワー枠〈原作どおり左は Q のテレポーテーション、右は E のスピードブースト〉、シャード残数、枠付きのミニマップ、目的バンド）を `DynamicTexture` に 30Hz で描画する。残り時間はタブレットには表示しない（ゲーム側の制限時間・30 秒警告ボイスは 04 参照）。原作と同じくカメラに固定されていて、視点移動では画面上の位置が変わらず、速さで広がる FOV（ダッシュ・ブースト）で画面の中心へ向かって小さくなる。位置と大きさは原作のタブレットの配置から決めている（5 節）。壁際でも寄らない（6 節）。
- `Hud`（hud.ts）: DOM 側の最小 HUD。字幕、画面の中央の手のマーク（原作の `UMG_Interact`）と、右下の「SAVING PROGRESS」表示（原作の `UMG_Saving`）。
- `ShardStreakFx`（streak.ts）: シャードの連続回収の節目の表示（原作の `UMG_ShardStreak`）。節目の画像（NOT BAD など）・紫の周辺減光・EXTRA LIFE ! を Web Animations で出し、2 s で消す（「シャード連続回収の表示」の節）。
- `VignetteSidesFx`（vignette-sides.ts）: 特殊シャードを取ったときの ENEMIES STUNNED / ENEMIES REVEALED（原作の `UMG_VignetteSides`）。橙 / 赤の画面の縁と下の文字を原作の Anim で出し、2 s で消す。
- `LevelClearFx`（level-clear.ts）: ポータルで出る脱出の画面（原作の `UMG_LevelClear`）。赤い You Escaped! と白い閃光から黒いリザルト（6 行のランクと加算シャードの数え上げ、TOTAL SHARDS、FINAL RANK）へ原作のアニメと音で進め、NEXT で Fade Out してタイトルへ戻す。値は 04 記録の `game/results.ts`。
- styles.css: 上記 DOM HUD と、タイトル（原作の `UMG_TitleScreen` の配置）/ ローディング / ポーズ（原作の `UMG_Pause` の配置）/ 死亡画面 / 脱出の画面（原作の `UMG_LevelClear` の配置）のスタイル。
- `unit()`（stage.ts）: メニューの 1 単位の px（16:9 の舞台 `#stage` の `getBoundingClientRect().height / 1080`。CSS の `--u` と同じ値。`#stage` が無ければ `min(innerWidth, innerHeight) / 1080`）。options.ts の `arrange` とスライダー、death.ts の Shake が使う。
- `TitleFx`（title.ts）: タイトル画面の動きと音（原作のウィジェットアニメと、それと Blueprint が鳴らす音）。
- `PauseFx`（pause.ts）: ポーズ画面の動きと音（原作の `UMG_Pause` のウィジェットアニメと、Blueprint が鳴らす音）。
- `DeathFx`（death.ts）: 死亡画面・ゲームオーバー画面の動きと音（原作の `UMG_DeathScreen` のウィジェットアニメと Delay、Blueprint が鳴らす音）。
- `StageIntro` / `IntroClock` / `introAt`（stage-intro.ts）: ステージ OP（原作のレベルのタイトルカード `UMG_ChapterPortal` と、それを出すレベル BP の Delay）。ゲームの時計で 1 フレームずつ描く。
- `OptionsMenu`（options.ts）: タイトルの OPTIONS（原作の `UMG_Options`）の操作と、グリッドの 4 つのボックスの配置（Slate の `SGridPanel` の規則）。

## 公開インターフェース

### tablet.ts
- `interface TabletFrame { player:{x,z}, yaw, markers: MapMarker[], arrow: MapArrow | null, time }` — 毎フレーム `update` に渡す入力（ミニマップの描画に使う。`arrow` は地図の矢印で、板が非表示なら null。11 記録）。速さ・FOV・視点の移動量・画面の赤は受け取らない（FOV で縮むのはカメラの子だから。テレポートとスピードブーストの赤は、postfx のシーンのティントがタブレットにも掛かる。09 記録）。
- `class Tablet`
  - `constructor(scene, camera, minimap: Minimap, reflected: AbstractMesh[])` — `reflected` は ReflectionProbe の `renderList`（game.ts では `level.staticMeshes` + `level.gateMeshes` + 各ランプの `glow`）。`scene` と `camera` は構築（メッシュ、ルートの親、反射プローブの位置）にだけ使い、保持しない。
  - `setVisible(visible)` / `toggle(): boolean`（Space/Tab。戻り値は「しまった状態か」）/ `lowered`（public フィールド）
  - `setShards(remaining)` / `setObjective(text)` — `setShards` は残りが減ったとき（回収。リスタートで戻るときは除く）に `flash` = 0 にし、原作の `Count Shake`（マップの閃きと数字の揺れ、8 節）を始める。
  - `setRefill(slot: 'teleport'|'boost', percent)` — 枠の原作の `Percent`（`MM_Powers`。1 = 使える、0 = 使えない、初期値 1）: 色つきのアイコンを、枠の中心から 12 時を起点に時計回りの扇形 `percent` 周ぶんだけに、残りを灰色で描く。`percent` が前回から 0.02 を超えて変わったら次のフレームで描き直す（照準の Q の灰色、取り消しで色が戻る）。game.ts は毎フレーム、テレポートの枠に `teleport.percent`、ブーストの枠に効果中は `boostTime / duration`、効果の後は `1 − boostCooldown / cooldown` を渡す。
  - `pop(slot)` — そのパワーが使われた: 枠が弾む（原作の「Use Left」/「Use Right」。`popScale`）。game.ts は Q / E を押すたびに呼ぶ（原作は使えるかを判定する前に弾ませる）。
  - `update(dt, f: TabletFrame)` — 上げ下げの補間、配置、30Hz の画面再描画（パワー枠の弾みとマップの閃きの間は毎フレーム）。
  - `readonly minimap` — game.ts から `tablet.minimap.toggleZoom()` で参照。

### tablet-anim.ts（タブレットのウィジェットアニメ。Babylon 非依存）
- 原作（pak_reference の `UI/Tablet/UMG_Tablet`）のウィジェットアニメを `core/ue-curve.ts` の曲線で持つ。`tests/tablet-anim.test.ts` が確かめる。
- `COUNT_SHAKE = { speed 2, flash, flashColor '#b900ff', x, y, scale, rest }` — `Count Shake`（BP_Shard の `Collect` が `PlayAnimation(Count Shake, 0, 1, Forward, 2.0)` で再生。長さ 0.2 s、再生速度 2 なので 0.1 s）。
  - マップの部分: `Image_41` の ColorAndOpacity の α が `flash` = 0.25（0 s）→ 0（0.2 s）、両端の接線 0（smoothstep）。`flashColor` は `Image_41` のブラシの色 linear (0.485, 0, 1) の sRGB。
  - 数字の部分: `ShardCount` の RenderTransform（原作のウィジェットの px、y は下向き。キーの接線は 1 tick = 1/60000 s あたりなので 60000 倍して毎秒に）: `x` 0（0 s）→ −14（0.05 s）→ 0（0.1 s、接線 14/0.15）→ 0（0.2 s）、`y` 0 → 9（接線 −120）→ −12（接線 −60）→ 0、`scale` 1 → 1.1 → 1（0 / 0.05 / 0.1 s、接線 0）。`rest` (0, −12) は `ShardCount` 自身の RenderTransform の移動。色のトラックは無い。
- `collectFlash(t)`: 回収から `t` 秒後の α（`evaluateCurve(flash, 2t)`。0 未満と 0.1 s 以降は 0）。
- `countShake(t)`: 回収から `t` 秒後の数字の、静止位置からのずれ `{x, y}`（原作のウィジェットの px）と `scale`。再生中はキーの移動が元の `rest` を上書きするので `キー − rest`（最初は 12 px 下）、0 未満と 0.1 s 以降は `{0, 0, 1}`（`EvalOptions.CompletionMode` が RestoreState で、終わると元の RenderTransform に戻る）。

### hud.ts
- `class Hud` — `root`（#hud）、`setVisible(visible)`、`setInteract(on)`、`showSaving()`、`subtitle(text, speaker?, seconds = CONFIG.hud.subtitleSeconds)`、`fade(opacity, seconds)`。
### title.ts（タイトル画面の動きと音）
- 原作（pak_reference の `UI/Main/TitleScreen/UMG_TitleScreen` と `UMG_PopUp`、v1.6.1）のウィジェットアニメを UE の曲線（`core/ue-curve.ts` の `evaluateCurve`。接線は UE の auto と同じく隣のキーとの傾き）で持ち、60Hz で標本化した Web Animations（`fill: 'both'`）で再生する。音は原作のアニメと Blueprint が鳴らすもので、音量は SoundWave の Volume × 呼び出しの値、再生速度は呼び出しの Pitch。
- `ANIM`（秒。キーは [時刻, 値, 接線]）:
  - `reveal`（Slideshow。原作は Construct で再生）: 2.5 s。`cover` 1 → 0（接線 0）。
  - `leave`（FadeOut = NEW GAME / FadeOut_0 = 続行）: 3.75 s。`black` 0 → 1（3.5 s、接線 1/3.75）→ 1（3.75 s）。FadeOut だけ `pulse`（画面全体の拡大）1（0.15 s）→ 1.05（0.25 s）→ 1（0.3 s）と `red` 0（0.15 s）→ 0.5（0.25 s、接線 0.2/0.15）→ 0.2（0.3 s、接線 −0.5/3.5）→ 0（3.75 s）。
  - `popup`（Popup）: 0.5 s。`fade` 0 → 1（0.25 s）、`scale` 0 → 1（0.25 s、接線 2。0.33 s で約 1.074 まで行き過ぎる）→ 1（0.5 s）。`closeFrom` 0.25（Press No は 0.25 s から逆再生）。
  - `options`（UMG_Options の FadeIn。キーは Popup と同じなので `popup` の曲線で再生する）: `closeFrom` 0.25、`hideAfter` 0.3（Cancel は 0.25 s から逆再生し、Delay 0.3 s の後にウィジェットを外す）。
- `SOUND`: `music`（`title_music` = Pause_Sound_v1。SoundWave の Volume 0.4 × FadeIn の 0.5 = 0.2、再生速度 0.5、2 s でフェードイン、先頭から、sfx バス。タイトルの音はすべて UI 音 `ui: true` で鳴らす。06 記録）、`musicOut`（`newGame` 1 s / `resume` 4 s。原作の Music の FadeOut）、`select`（`ui_select` = UI_Select_V3、0.7。閉じるときは再生速度 `closeRate` 0.7）、`save`（`select` = UI_Select_V2、1。SoundWave に Volume が無いので既定の 1。SAVE & EXIT）、`popup`（`ui_popup` = UI_Window_PopUp_V3、1）、`start`（`start` = Start_New_Game。音量の曲線 0.6（0〜0.45 s、接線 −0.3/1.65）→ 0.3（1.65 s）で、その後は 0.3 のまま）、`voice`（`title_voice` = Bierce_Title_Modified_03、1.65 s 後、voice バス）。`Leave` 型は `'newGame' | 'resume'`。
- `frames(length, at)`: `at(t)` を `round(length × 60)` 区間で標本化したキーフレーム（`offset` 0〜1）。`envelope(curve, step = 0.05)`: 音量の曲線を `AudioManager` の `envelope` の点（最後のキーまで 50 ms ごと）にする。
- `class TitleFx(root, audio)`:
  - `reveal()`: `.title__cover` を `reveal` で消し、BGM をループで始める（`audio.loop('title_music', { volume 0.2, rate 0.5, fadeIn 2, offset 0 })`）。ブラウザは最初のクリックかキー入力まで音を止めているので、`document` の `pointerdown` / `keydown` で `audio.resume()` する（止まっている間に予約したフェードインは再開してから進む）。
  - `click()`: `ui_select`（0.7）。
  - `openPopup(layer, sound = true)`: 閉じる途中のタイマーを取り消し、`layer.hidden = false`、`sound` なら `ui_popup`（RESTART?。OPTIONS は `false`: 原作の OPTIONS はタイトルのクリック音だけ）、層の中の `.popup-backdrop` の不透明度と `.popup-card` の不透明度・`scale` を `popup` で 0.5 s。
  - `closePopup(layer, hideAfter = 0.25): Promise<void>`: `ui_select` を再生速度 0.7 で鳴らし、`popup` を 0.25 s から逆に 0.25 s 再生し、`hideAfter` 秒後に hidden にして resolve（RESTART? は 0.25、OPTIONS は `ANIM.options.hideAfter` 0.3）。閉じる途中なら同じ Promise を返す。
  - `save()`: `select`（UI_Select_V2、1）。SAVE & EXIT のクリック。その後 main.ts が `closePopup` で閉じるので閉じる音も重なる（原作の SAVE & EXIT は最後に Cancel を呼ぶ）。
  - `stopMusic(fade = 1)`: BGM を止め、`pointerdown` / `keydown` の listener を外す。main.ts の `onStarted` でも呼ぶ（デバッグ API から始めたときも BGM を残さない）。
  - `leave(kind)`: BGM を `musicOut[kind]` 秒で止め、`.title__fade` を `black` で黒くする。`newGame` は `#title` の `scale` を `pulse`、`.title__flash` の不透明度を `red` で動かし、`start` を `envelope(SOUND.start.volume)` で、`title_voice` を 1.65 s 後に鳴らす。3.75 s 後に resolve。
- `tests/title.test.ts`: 各アニメが原作のキーを通ること、`frames` の標本数と offset、BGM の音量と速度、`start` の音量の点の始点・終点と範囲、OPTIONS の閉じ方（0.25 s から逆再生、0.3 s で外す）と SAVE & EXIT の音量。

### pause.ts（ポーズ画面の動きと音）
- 原作（pak_reference の `UI/Menu/Pause/UMG_Pause`、v1.6.1）のウィジェットアニメを title.ts と同じく UE の曲線と 60Hz の Web Animations（`frames`）で再生し、Blueprint が鳴らす音を鳴らす。音量は SoundWave の Volume × 呼び出しの値、再生速度は呼び出しの Pitch。
- `ANIM`: `fade`（FadeIn。Construct で再生）: 0.5 s、`opacity` 0 → 1（接線 0）。RESUME は逆再生し、同じ長さの Delay の後に SetGamePaused(false) とウィジェットの削除。`popup`（Popup = GIVING UP?、Popup_0 = RESTART?）: title.ts の `ANIM.popup` と同じキー（カードの不透明度と `scale`、`closeFrom` 0.25）。同じ 0.25 s で Blur+Red が 1 → 0、CanvasPanel_3 が 0 → 1。
- `SOUND`: `open`（`pause` = UI_Pause、1。SoundWave に Volume が無い）、`music`（`title_music` = Pause_Sound_v1。SoundWave の Volume 0.4、CreateSound2D の Pitch 1、先頭から、FadeIn 1 s。Destruct の FadeOut 0.5 s が `fadeOut`）、`select`（title.ts の `SOUND.select` = UI_Select_V3 0.7。NO / CANCEL は再生速度 0.7）、`popup`（title.ts の `SOUND.popup` = UI_Window_PopUp_V3、1）。どれも UI 音（`ui: true`）で鳴らすので、ポーズ中に止めたゲームの音（06 記録の `setPaused`）とは別に鳴る。
- `class PauseFx(root, audio)`（`#pause`。`.pause__wash` = Blur+Red、`.pause__main` = 帯・頭・EASY MODE・メニュー、`.pause__veil` = CanvasPanel_3 を引く）:
  - `show()`（原作の Construct）: 残ったタイマーとアニメを取り消し、カードを hidden、`.pause__veil` の `is-blocking` と `.pause__main` / `root` の `inert` を外して表示する。`pause`（1）を鳴らし、`.pause__wash` と `.pause__main` の不透明度を `fade` で 0 → 1。音楽を 0.05 s で止めてから `title_music` を `loop`（0.4、fadeIn 1、offset 0）。
  - `resume(): Promise<void>`（RESUME）: `root.inert = true`、`click()`、`fade` を逆再生し、0.5 s 後に `hide()` して resolve。
  - `hide()`（ウィジェットの削除。原作の Destruct）: タイマーを取り消して `root.hidden = true`、音楽を 0.5 s で止める。
  - `click(rate = 1)`: `ui_select`（0.7）。
  - `openPopup(card)`（RESTART / QUIT）: `card.hidden = false`、`.pause__main` を `inert`、`.pause__veil` に `is-blocking`（原作の redblock を Visible にしてメニューのクリックを遮る）、`click()` と `ui_popup`。カードを `popup` の不透明度と `scale`、`.pause__veil` を `fade`、`.pause__wash` を `1 − fade` で 0.5 s 再生する。
  - `closePopup(card): Promise<void>`（NO / CANCEL）: 閉じる途中（`.pause__main` が `inert` でない）なら何もせず resolve。`inert` と `is-blocking` を外し、`click(0.7)`、`popup` を 0.25 s から逆に 0.25 s 再生し、0.25 s 後にカードを hidden にして resolve（その間に別のポップアップが開いていれば隠さない）。
- `tests/pause.test.ts`: FadeIn のキー（0.25 s で 0.5、0.5 s で 1、立ち上がりが緩い）、ポップアップのキーが title.ts と同じで 0.25 s から閉じること、音の音量（UI_Pause 1、Pause_Sound_v1 は 0.4・1 s で入り 0.5 s で消える、選択音とポップアップの音は title.ts と同じ）。

### death.ts（死亡画面の動きと音）
- 原作（pak_reference の `Blueprints/UMG/UMG_DeathScreen`、v1.6.1）のウィジェットアニメを title.ts と同じく UE の曲線と 60Hz の Web Animations（`frames`）で再生し、Construct とイベントの Delay と Blueprint が鳴らす音をなぞる。音はすべて UI 音（`ui: true`）で、ゲームを止めた下でも鳴る。音量は SoundWave の Volume × 呼び出しの値。
- `TIMING`（Construct からの秒）: `reveal` 0.5（Construct の 2 つの Delay 0.5。声・Fade In・Life Animation と、ライフが残っていれば Life_Lost）、`fadeInRate` 1.5（Fade In の再生速度）、`shake` 1.5（Life Animation の Delay 1.0 の後の Shake。@8222）、`gameOver` 0.75（ライフ 0: reveal の後の Delay 0.25 で曲・声・Death）、`buttons` 2.75（その Delay 2.0 の後、ボタンが押せてマウスが出る）、`voiceMax` 6（声が終わるか reveal から 6 s の早い方で進む。DoOnce）、`proceed` 0.5（その後の Delay 0.5 で Fade Out）、`respawn` 2（Fade Out から Delay 2.0 で Respawn）、`leave` 1（ボタンの Fade Out と Delay 1.0 の後にレベル〈タイトル〉を開く）。
- `proceedAt(voice)` = `reveal + min(max(0, voice), voiceMax) + proceed`（ライフが残っているときの Fade Out の開始）。
- `ANIM`（キーは [時刻, 値, 接線]。接線は UE の auto = 隣のキーを結ぶ傾き。Fade の片側は原作のユーザー接線 −3.375811e-5/tick = −2.0255/s）:
  - `fadeIn`（Button_0 の背景の不透明度。長さ 1 s を 1.5 倍速）: 1 → 0（出口の接線 −2.0255。ゆっくり始まり速く抜ける）。`fadeOut`: 0（入口の接線 −2.0255。0 未満に沈むので約 0.35 s までは何も出ない）→ 1（59999/60000 s）。
  - `shake`（1 s。`update` 0.1 = イベントトラックの Update Life）: `vignette`（Image_161 の不透明度）0 → 1（0.1 s）→ 0.3（0.25 s）→ 0（1 s）、`vignetteScale` 1.25 → 1.1（0.1 s）→ 1、`x` / `y`（CanvasPanel_0 の平行移動、単位）0.1〜0.3 s に (10, −10) → (−5, 5) → 0、`skull`（ライフの緑と青）1 → 0（0.1 s）→ 1（0.3 s）。
  - `death`（3.5 s）: `heading`（REMAINING LIVES）1 → 0（0.5 s）、`tip` 1 → 0（0.5 s）、`dead`（YOU ARE DEAD）0（0.5 s）→ 1（2.5 s）、`text`（3 つのボタンの文字）・`menu`（VerticalBox_161 のレンダー不透明度）0 → 1（0.5〜2.5 s）、`checkpoint`（LAST CHECKPOINT のボタンの色の α。接線 0.4）0 → 1（0.5〜2.5 s）、`quit`（QUIT TO TITLE の）0（1 s まで）→ 1（2.5 s）。ボタンの見かけは text × ボタンの α × menu。
  - `popup`: title.ts の `ANIM.popup`（RESTART の `UMG_PopUp`）。
- `SOUND`: `lifeLost`（`life_lost` = Life_Lost、0.6。1.0 s 目に強い打撃があり、reveal に鳴らすと Shake の瞬間に重なる）、`music`（`game_over` = 66_-_Game_Over。SoundWave の 0.7 × 呼び出しの 0.7 = 0.49、1 回だけ、sfx バス）、`voice`（`lifeLost: 'fine'`、`gameOver: 'over'`。原作はビアスの声〈館の死亡台詞のランダムと Bierce_Game_Over_01〉だが、ユーザーの選択でワサミの声。字幕は出さない〈原作の CreateSound2D の声も字幕なし〉。ゲームオーバーの 1.25 s 後の笑い声は鳴らさない）、`select` / `popup`（title.ts の `SOUND.select` / `SOUND.popup`）。
- `TIPS`（本作の内容に合わせた 7 本。原作のヒントは館の猿やスタンオーブなど本作に無いものなので書き直した）、`pickTip(shown, random)`: まだ出していないものから無作為に 1 本（全部出したら一巡してやり直す。原作の Set Tip も既出を配列に覚えて避ける）。`skullColor(gb)`: ライフのアイコン（sRGB 124 の灰色）に Shake の色を掛けた CSS の色（線形空間で緑と青に `gb` を掛ける）。
- `class DeathFx(root, audio)`（`#death`。`.death__canvas` = CanvasPanel_0、`.death__life` ×6 = Life1..6、`.death__menu` と `[data-death="restart|checkpoint|quit"]`、`.death__heading`、`.death__dead`、`.death__tip`、`.death__cover` = Button_0、`.death__vignette` = Image_161）:
  - `open(lives): Promise<void>`（Construct。`lives` は捕獲の後の残り）: 前回のタイマー・アニメ・曲を片付け（`reset`）、表示し、ヒントを選ぶ。ライフの行は `lives + 1` 個（Life Animation）、ライフ 0 なら行・見出し・ヒントを隠す（どれも黒い覆いの下）。reveal で Fade In、ライフが残っていれば `life_lost` と声 `fine`。ライフが残っていれば shake で Shake（0.1 s でアイコン `[lives]` を hidden にし、行が中央に詰まる）、`proceedAt(声の長さ)` で Fade Out、その 2 s 後に resolve（リスポーンの時）。ライフ 0 なら gameOver で `game_over` を 1 回（`audio.loop(..., { repeat: false, offset: 0 })`）と声 `over` と Death、buttons で `.death__menu` の `inert` を外して resolve（原作どおりどのボタンも選ばない。3 つとも灰色）。
  - `leave(): Promise<void>`（RESTART の YES・LAST CHECKPOINT・QUIT TO TITLE）: `leaving` を立ててメニューを `inert` にし、Fade Out、1 s 後に resolve。音は鳴らさない（原作の LAST CHECKPOINT〈S ランクの警告を出さないとき〉と QUIT TO TITLE は無音）。
  - `hide()`: 片付けて hidden（リスポーン、レベルを開き直すとき。曲は 0.05 s で切る）。
  - `click(rate = 1)`: `ui_select`（0.7）。`openPopup(layer)`（RESTART と、LAST CHECKPOINT の S ランクの警告 `#death-warning`。main.ts がどちらの層かを渡す）: メニューを `inert` にし、`ui_popup` と Popup。`closePopup(layer, sound = true)`: `sound` なら再生速度 0.7 の選択音（NO = 原作の Press No）、YES は自分のクリックの後に `sound = false`（原作の Close Animation は無音）。0.25 s から逆に 0.25 s 再生し、終わりで hidden にして、`leave` の後でなければメニューの `inert` を外して resolve（YES の Fade Out の間にボタンが押せないように）。
- `tests/death.test.ts`: Fade In / Fade Out の形（半ばでまだ 0.7 超の黒、Fade Out は 0.2 s で 0 未満）、Shake のキー（0.1 s で 1、0.25 s で 0.3、0.15 s で ±10 など）と `skullColor`、Death のキー、`TIMING` と `proceedAt`（1.78 s の声で 2.78、10 s の声でも 7）、音量（0.6、0.49、声の id）、ヒントが一巡するまで重ならないこと。

### level-clear.ts（脱出の画面の動きと音 = 原作の `UI/Menu/UMG_LevelClear`）
原作の 01_Hotel のレベル BP がポータル（`EndTrigger`）で SetGamePaused の後すぐに Create → AddToViewport(6) するウィジェット。値は 04 記録の `game/results.ts`。
- `ANIM`（原作のウィジェットアニメのキー。接線は原作の 1 tick あたりの値 × 60000 = 1 s あたり。Sequencer のキーは到着と出発の接線を別々に持ち、このアセットでは一部が食い違ったまま残っているので、食い違うキーは `keys()` が同じ時刻に 2 つ置き、前の区間は到着・後ろの区間は出発の接線で補間させる）:
  - `clear`（ClearAnimation、4.39 s = 音のトラックの終わり。Construct が再生）: ウィジェット全体の不透明度 0 → 1（0.25 s）、ResultsBox 0 → 1（3.0〜3.25 s）、You Escaped!（Image_216）の不透明度 0 → 1（0.5〜0.75 s）・回転 45° → 0（0.75）→ −10°（0.8）→ 0（0.95）・拡大 2 → 1 → 1.1 → 1、赤（ClearLevel）の不透明度 0 → 1（0.25）… 1（2.75）→ 0（3.0）と 0.75〜0.95 s の揺れ（x 0, 6, −2, 0 / y 0, 3, −7, 0 単位）、白いビネット（Image_6）0.7〜1.0 s と白（Image_7）0.7〜0.85 s の閃光（どちらも 0.75 s で 1）、EASY MODE 0 → 1（3.0〜3.25 s）。音 `escaped` は 0.75 s、イベント ShowResults は 3.25 s。
  - `row`（Time Animation 〜 Shard Streak Animation。どれも 1 s で同じキー）: 値の不透明度 0 → 1（0.25 s）と拡大 1.5 → 1（0.25）→ 1.05（0.3）→ 1（0.5）、ランクは 0.25 s 遅れ、加算シャードはさらに 0.25 s 遅れて同じように出る。0.5 s のイベントでその行の数え上げ（`counter`）。
  - `total`（Total Shards Animation 0.5 s。出方は行の値と同じ）、`final`（Final Rank Animation 0.75 s: 文字の拡大 1.5 → 1（0.1）→ 1.15（0.15）→ 1（0.5）と不透明度 0 → 1（0.25）、ルートのキャンバスの揺れ 0.15〜0.35 s（x 0, 4, −2, 0 / y 0, −7, 3, 0 単位）、0.12 s に `grade_stamp_1`）、`fadeOut`（黒 0 → 1、1 s）。
- `TIMING`（ShowResults からの Delay の連鎖）: 行 `rows` は TIME 0・SOUL SHARDS 0.25・BONUS SHARDS 0.5・SECRETS 0.75・LIVES LOST 1.0・SHARD STREAK 1.25 s、TOTAL SHARDS 1.75 s、FINAL RANK 2.75 s、UI 入力 `input` 3.75 s（開いてから 7.0 s。NEXT とカーソル）、NEXT から Finished までの `finish` 4 s。
- 行ごとの音と数え上げ: ランクの判 `grade_stamp_2` は Time・Soul Shards・Bonus Shards で 0.4 s、ほかで 0.45 s（各アニメの音のトラック）。数え上げの幅（`Counter(n, span)`）は SECRETS 0.05 s、SHARD STREAK 0.5 s、ほか 0.25 s。SOUL SHARDS は数え上げが無い（文字は空のまま）。TOTAL SHARDS も 0.5 s で数えるが、原作の文字は合計のバインド関数に縛られていて SetText が効かないので、最初から合計を出し、ループ音だけが鳴る。
- `SOUND`: `escaped` / `grade_stamp_2`（`stamp`）/ `grade_stamp_1`（`finalStamp`）/ `xp_fill`（`fill`、ループ）。どれも音量 1（アセットにもトラックにも音量が無い）の UI 音。`RANK_COLOR`（`Get_*Rank_ColorAndOpacity_0` の linear の色を sRGB に）: S は金 `rgb(248, 206, 114)`、A〜C は暗い赤 `rgb(193, 0, 0)`、無しは `transparent`。
- `class LevelClearFx(root, audio)`（`#end`）:
  - `open(results, easy): Promise<void>`: `reset()`、表示、`fill`（各行の値・ランクの文字と色、合計、FINAL RANK の文字と色。原作はどれも変数にバインドされていて、レベル BP が追加の前に入れる）、EASY でなければ EASY MODE を隠す（原作の Construct は RemoveFromParent）、ClearAnimation を Web Animations（60 Hz の標本、`fill: 'both'`）で再生、0.75 s に `escaped`、3.25 s に ShowResults。7.0 s に NEXT の `inert` を外して `clear--input`（カーソルを出す）にして resolve（main.ts がポインタロックを外す）。
  - ShowResults: 各行を `TIMING.rows` で `playRow`（値・ランク・加算のアニメ、判の音、0.5 s に `count`）、`TIMING.total` で合計のアニメと見えない数え上げ、`TIMING.final` で FINAL RANK のアニメ・キャンバスの揺れ（`unit()` の px）・0.12 s の `grade_stamp_1`。
  - `count(el, counter)`: `xp_fill` のループを UI 音で鳴らし（原作の CreateSound2D）、`requestAnimationFrame` の `tick` で `+counter.at(t)` を書き、`counter.length` に達したらループを 0.02 s で止める（原作の Stop）。
  - `leave()`（NEXT。原作の DoOnce）: NEXT を `inert` にし `clear--input` を外し（原作の SetInputMode_GameOnly とカーソルを隠す）、Fade Out、4 s 後に resolve（main.ts が再読込してタイトルへ）。`hide()`。`reset()` はタイマー・rAF・ループ・アニメを止め、NEXT を `inert` に戻す。

### stage-intro.ts（ステージ OP = 原作のレベルのタイトルカード）
- 原作（pak_reference、v1.6.1）のレベル BP `03_Manor_Zone1` は、開始の流れ（Spawn → Traps No Death）の後 Delay 3.0 で `UMG_ChapterPortal` を作り（`Level` = 2、AddToViewport の Z 5）、プレイヤーの `CanMove?` を False にし、Delay 10.0（再開位置 @15）で True に戻し、目的を設定した後の Delay 0.5（@530）で最初の台詞（BierceTalk の Talk、`Bierce_Manor_Zone1_01`）を言わせる。`CanMove?` は BP_DD_PlayerCharacter の移動・視点・ダッシュ・タブレットを止める。ウィジェットの Construct は `Level` で頭（Level 2 は `pause_watchman_head`）と題字（`chapter_ui_title_deadlydecadence`）を選び、`loop` を 0.8 s から 1 回・速度 1 で再生し、Delay 11 で RemoveFromParent する。
- メニューの Web Animations と違い、ゲームの時計で描く（game.ts が毎フレーム `show(introAt(t))`）。ポーズでゲームと一緒に止まる。Babylon 非依存で `node --test` で確かめる。
- `TIMING`（レベルの開始からの秒）: `card` 3（カードを作りプレイヤーを止める）、`from` 0.8 / `length` 10（`loop` の再生範囲）、`release` 13（`CanMove?` を戻す）、`voice` 13.5（最初の台詞）、`done` 14（RemoveFromParent）。
- `ANIM`（`loop` のキー。時刻はアニメの秒、接線は書き出された値 /tick × 60000。`Interp` は 05 記録の ue-curve.ts）:
  - `canvas`（CanvasPanel_0 の不透明度）0（0.8 s）→ 1（2 s、接線 0.117647）→ 1（8.5 s、−0.125）→ 0（10 s）。UE の auto 接線で 2〜8.5 s は 1 を超えるが、画面では 1 に切る。
  - `blur`（BackgroundBlur_0 の強さ）0 → 10（2 s。入口の接線 0.470588、そこから Linear で平ら）→ 10（8.5 s、−1.290323）→ 0（9.75 s）。
  - `shakeX` / `shakeY`（CanvasPanel_0 の平行移動、単位）2.2 s に 0 → 2.25 s に (−10, 3)（接線 70 / −40）→ 2.3 s の 1 tick 前に (7, −4)（100 / −30）→ 2.35 s に 0。
  - `band1` / `band2`（Image_48 / Image_49 のスロットの Left）501.62 → −1124.61、1103.77 → −1115.90（10 s、Linear）。
  - `ring` / `runes`（Image_40 / Image_41 の角度、時計回りの度）0 → 800、0 → −359（10 s、Linear）。
  - `titleOpacity`（TitleCard の不透明度）0（2 s、接線 0.454545）→ 1（2.2 s）。`titleScale` 1.5（2 s）→ 1（2.2 s、−1.4）→ 1.15（2.25 s）→ 1（2.35 s）。
  - `titleRed` / `titleGreenBlue`（TitleCard の ColorAndOpacity。区間は 2〜3 s、α は 1 のまま）: 2.2 s のキーが Constant で 2.25 s まで赤を保ち、2.25 s に白 (1, 1, 1)、2.35 s に赤（R の接線 −4.895833、G/B は −6.666667）、2.4 s に R 0.265625、2.55 s の 1 tick 前に赤（1.835938）。
  - `black`（本作の推定。レベルが開くときの黒）: アニメの 1.15 s まで 1 → 0.48（1.5 s、接線 −1.65）→ 0（2.1 s）。原作データにこの暗転の処理は無いが、収録（1080p60）ではカードが始まるまで画面が黒く、景色はアニメの 1.2〜2.05 s にかけてカードより少し遅れて明ける（空の輝度の比 1.3 s 0.17、1.5 s 0.52、1.8 s 0.86、2.0 s 0.97）ので、それに合わせた。
- `TITLE_GREY` 196、`titleColor(rgb)`: 原作の題字（一様な灰 sRGB 196）に線形空間で色を掛けた CSS の色（赤 `rgb(196, 0, 0)`、白 `rgb(196, 196, 196)`、R 0.265625 は `rgb(107, 0, 0)`）。
- `introAt(t): IntroFrame`（`t` はレベルの開始からの秒）: `t < card` なら `{ black: 1, card: null }`。以降はアニメの時刻 `a = from + (t − card)` で、`black` と、`a < length` の間だけ `card`（`opacity`、`blur` = 強さ × 不透明度〈ウィジェットの bApplyAlphaToBlur、既定で有効〉、`shake`、`bands`、`rings`、`title`〈`opacity`、`scale`、`color`〉）。
- `class IntroClock`: `t`（秒）、`advance(dt)` は通り過ぎたイベント（`'release'` 13 s → `'voice'` 13.5 s → `'done'` 14 s）を順に 1 回ずつ返す。`holding` は `t < release`。
- `class StageIntro(root)`（`#intro`）: `show(frame | null)`。null で `#intro` を hidden。黒の不透明度、`.intro__canvas`（`card` が null なら hidden）の `translate`（`calc(N * var(--u))`）、`.intro__blur` の `backdrop-filter: blur(強さ × --u)`（0.01 以下なら none）、`.intro__wash` / 帯 / `.intro__icon` の不透明度（キャンバスの不透明度）、帯の `left`（`960 − 4165 / 2 + Left` 単位）、輪とルーンの `rotate`、`.intro__title` の不透明度（キャンバス × 題字）・`scale`・`background-color`。
- `tests/stage-intro.test.ts`: `TIMING` の Delay の差（3、10、0.5、11）、キャンバスとぼかしのキー（0.8 s で 0、2 s で 1、ぼかしは 5 s で 10）、帯と輪が一定の速さで動くこと、題字の登場と拡大と色（2.24 s でまだ赤、2.25 s で白、2.4 s で暗い赤）、揺れのキー、`introAt`（3 s までは黒だけ、3 s でカードの不透明度 0、4.2 s で 1 と黒ほぼ 0、12.2 s でカードなし）、`IntroClock` が刻み方によらずイベントを 1 回ずつ順に返し、13 s の間 `holding` であること。

### streak.ts（シャード連続回収の表示 = 原作の `UI/Menu/Streaks/UMG_ShardStreak`）
- 原作の `BP_DD_GameMode` の Check Streak（@29549）は回収ごとに CurrentStreak を +1 し、20・50・100・150・200・250・350・500・700・1000 ちょうどで `UMG_ShardStreak`（Streak 1..10）を AddToPlayerScreen（ZOrder 2）する。ウィジェットの Construct は Streak で画像 `shard_streak_<数>` を選び、`Anim` を再生し、Streak 5・8（200・500）なら extralife を Visible にして Lives + 1（0..6 に Clamp）、Delay 2.0 で RemoveFromParent。本作は数と残機を `GameState`（04 記録）、音とシェイクを game.ts が受け持ち、ここは表示だけ。
- `STREAK_ANIM`: 原作の `Anim`（1.5 s。接線はアセットに保存された 1 tick あたりの値 × 60000）、`remove` 2（Delay 2.0）。
  - `card`（StreakImage）: scale 2 → 1（0.15 s）→ 1.1（0.25）→ 1（0.5）→ 1（0.9）→ 1.2（1.5）、ColorAndOpacity の α 0 → 1（0.15）→ 1（0.9）→ 0（1.5）。
  - `vignette`（Image_161）: scale 1（0〜0.35 s は Linear のキー、0.9 まで 1）→ 2（1.5）、α 0 → 1（0.15）→ 0.5（0.25）→ 0.25（0.9）→ 0（1.5）。ウィジェットの既定（ColorAndOpacity の α 0.25、RenderTransform の scale 2）はアニメが上書きする。
  - `life`（extralife）: 区間は 0.2 s から。RenderOpacity 0 → 1（0.35）→ 1（0.9）→ 0（1.5）、scale 1.25 → 0.95 → 1 → 1.1。0.35 s のキーの接線（2.5、−0.625）は隣のキーどうしの傾きと合わないが、アセットの値をそのまま使う。
- `streakImage(count)` → `./title/streak-<count>.webp`（13 記録）。`streakSound(count)` → `{ id, volume: 0.7 }`: 20・50 `streak_v1a`、100〜200 `streak_v2`、250・350 `streak_v3a`、500〜1000 `streak_v4`（原作の PlaySound2D 1.0 × SoundWave の Volume 0.7。V3A だけ音のクラスが SFX_UI だが、本作は同じ sfx バスで鳴らす）。
- `class ShardStreakFx(root)`: `#streak` の `.streak__card` / `.streak__vignette` / `.streak__life` を持ち、構築時に 10 枚の画像を先読みする（`cards`）。`show(count, life)`: 前の表示を消し、画像を差し替え、`life` で `.streak__life` の `hidden` を決め（原作の SetVisibility Visible / Hidden）、3 つの要素に `frames()`（60 Hz）の Web Animations を `fill: 'both'` で掛けて、2 s 後に `hide()`。`hide()`: タイマーとアニメを止めて `#streak` を隠す。原作は節目のたびに新しいウィジェットを重ねるが、本作は 1 枚を差し替える（節目どうしは 30 個以上離れていて、2 s の間に重なることはまず無い）。

### vignette-sides.ts（特殊シャードの表示 = 原作の `UI/Menu/Streaks/UMG_VignetteSides`）
原作の `BP_PowerOrb` / `BP_BonusShard` が取られたときに `AddToPlayerScreen(5)` するウィジェット（pak_reference v1.6.1）。
- `VSIDES_ANIM`（Anim、1.5 s。キーは原作の値、接線は 1 ティック = 1/60000 s あたりの値を秒あたりに）: `remove` 2（Construct の Delay 2.0 の後に RemoveFromParent）、`vignette.alpha`（Image_161 の ColorAndOpacity の α: 0 → 0.15 s で 1 → 0.25 s で 0.5 → 0.9 s で 0.25 → 1.5 s で 0）、`vignette.scale`（描画の拡大: 1.4 から 0.35 s の 1.25 まで直線、0.9 s まで 1.25、1.5 s で 2）、`text.opacity`（TextBlock_47 の RenderOpacity: 0 → 0.1 s で 1、0.7 s まで 1、1.5 s で 0）、`text.scale`（4 → 0.1 s で 1 → 0.15 s で 1.1 → 0.25 s で 1）、`canvas`（CanvasPanel_0 の角度 0 → 0.1 s で 0.5° → 0.133 s で 0.5° → 0.15 s で −2° → 0.25 s で 0 と拡大 1 → 1.05 → 1。区間は 28000 ティック = 0.467 s まで、`end`）。
- `VSIDES`: `stun`（Color (1, 0.4654, 0)、`ENEMIES STUNNED`）、`reveal`（Color (1, 0, 0.0167)、`ENEMIES REVEALED`）、`image`（Image_161 のブラシの色 (1, 0.2308, 0)）、`textAlpha` 0.8（文字の白の α）、`angle` −0.158°（CanvasPanel_0 の平常の角度）。原作の BP は TextToDisplay を上書きする（ウィジェットの既定の「ENEMIES STUNNED FOR 30 SECONDS」は出ない）。
- `vsidesColors(kind)`: 縁 = `image × Color`、文字 = `Color`（ウィジェットの SetColorAndOpacity が全体に掛かる）を線形 → sRGB にした CSS の色（オーブ: 縁 `rgb(255, 92, 0)`・文字 `rgba(255, 182, 0, 0.8)`、赤いシャード: 縁 `rgb(255, 0, 0)`・文字 `rgba(255, 0, 35, 0.8)`）。
- `class VignetteSidesFx(root)`: `show(kind)` は `#vsides` に `.vsides__canvas`（`.vsides__vignette` と `.vsides__text`）を足し、3 つに `frames()`（60 Hz）の Web Animations を `fill: 'both'` で掛け（キャンバスは区間の後は平常の角度）、2 s 後に消す。取るたびに新しく足す（原作もウィジェットを重ねる）。`clear()` は全部すぐ消す（レベルを開き直すとき）。最後の 1 枚が消えたら `#vsides` を隠す。
- `tests/vignette-sides.test.ts`（2 件）: 文字と CSS の色、縁のアニメ（0.15 s で 1、0.9 s で 0.25、最初の区間が直線、1.5 s で拡大 2）、文字の 4 倍からの出方と 0.7 s までの保持、キャンバスの −2°、区間の長さ、保存された接線が隣の点の傾きと一致すること。

### options.ts（タイトルとポーズの OPTIONS = 原作の `UI/Menu/UMG_Options`）
Babylon 非依存（`../game/settings.ts` を拡張子付きで import）。
- `interface GridSlot { column; row; columnSpan; rowSpan; padding: [左, 上, 右, 下]; nudge: [x, y] }`、`interface Rect { x; y; width; height }`。
- `GRID`（UE の単位）: 原作の `GridPanel_0`。`size` [1234.535, 673.848]、`columnFill` [0.5]（列 0）、`rowFill` [0.5]（行 0）、`slots`（すべて上下左右の中央寄せ）: `graphics`（列 0・行 1〜3）、`audio`（列 0・行 3、上の余白 150、Nudge (0, 34.411)）、`controls`（列 1〜2・行 2〜3、上の余白 150、Nudge (34.645, −182.728)）、`difficulty`（列 0・行 1〜3）。
- `slateGrid(size, columnFill, rowFill, slots)`: UE 4.21 の `SGridPanel` の配置。各スロットの大きさ＋余白を、スパンする列・行に均等に割り、各セルは最大の取り分を持つ。Fill 係数のあるセルが残り（負にもなる）を係数で分ける。各スロットは余白の内側でセルの中央に置き（余白の内側より大きくはしない）、Nudge を足す。中央寄せは Slate の `AlignChild`（`(room − child)/2 + pre − (pre + post)/2`）。
- `class OptionsMenu(root)`（`#options`）:
  - コンストラクタ: `button[data-arrow]`（`quality` / `difficulty`、`data-delta` ±1）のクリックで `stepValue`（QUALITY 0..3、DIFFICULTY 0..1）、`[data-check]` のクリックで反転、`[data-slider]` にスライダーの操作を付ける。
  - `show(settings)`: 原作の Setup Values。写しを下書きにして表示し、ボックスを配置する（層を表示してから呼ぶ）。`document.fonts.status` が `loaded` でなければ、読み込み後にもう一度配置する。
  - `value`: 編集した設定の写し（SAVE & EXIT が保存する）。
  - スライダー: 押す・ドラッグで、ポインタの位置の値を `snap`（1/9）してから設定する（原作の OnValueChanged）。位置の値は原作の `SSlider::PositionToValue`（つまみの幅 18 を両端から引いた `(x − 18) / (幅 − 36)`）。カードの拡大を打ち消すため、`getBoundingClientRect` の幅と `offsetWidth` の比で局所座標にする。ポインタキャプチャで枠の外へのドラッグも追う。左右（上下）キーで 1/9 ずつ動かす（原作のキーは 0.01 刻みで、吸着で元に戻るので効かない。本作の変更）。
  - 表示（`render`）: スライダーの `--v`、`aria-valuenow` / `aria-valuetext`、値の欄（`[data-text]`）は `sliderText` か `QUALITY_TEXT` / `DIFFICULTY_TEXT`、チェックの `aria-checked`。
  - 配置（`arrange`）: 各ボックスの幅と高さ（`getComputedStyle` の px ÷ 1 単位の px = stage.ts の `unit()`）を `slateGrid` に渡し、`left` / `top` を `calc(N * var(--u))` で置く。レンダー変換の平行移動は CSS 側。
- `tests/options.test.ts`: 実寸に近い 4 つのボックスで、列 0・行 0 が残りを取ること（行 0 は負になる）、スパンの均等割り、余白と Nudge、部屋より大きくならないこと。

## 内部構造と処理の流れ

### 1. 3D メッシュ構築（constructor）
- 定数: `TABLET_GROUP = 1`（`renderingGroupId`）、`HELVETICA_BOLD = 'DD Helvetica Neue', 'Helvetica Neue', Helvetica, Arial, sans-serif`（シャード数と「Z」ラベル）と `HUD_FONT = 'DD Helvetica Neue', 'Helvetica Neue', Arial, 'Hiragino Sans', 'Noto Sans JP', sans-serif`（目的バンド。和文はシステムの和文へ落ちる）: 原作の `UMG_Tablet` の `ShardCount`・`TextBlock_0`（目的）・`TextBlock_107`（Z）はどれも helvetica-neue-bold_Font で、同梱した原作のフォント（styles.css の `@font-face 'DD Helvetica Neue'`、13 記録）を先頭に置く。キャンバスは Web フォントが読み込まれてからでないと使わないので、コンストラクタで `document.fonts.load("700 16px 'DD Helvetica Neue'")` の後に描き直させる（`accum = Infinity`）。`CAP = 0.714`（字高 / フォントサイズ。原作のフォントの 8 と H の高さ 714 / 1000）、`RING = '#4a464c'`（パワー枠の輪）、`FRAME = '#7d7476'`（マップ枠。地図の線と同じ色味の暖かい灰）。画面の灰色は発光で描くのでトーンマップで暗部が沈む。原作の画面上の実測（輝度 20〜60）に近づくよう、元の色は明るめにしてある、`LAYOUT`（画面レイアウト。8 節）。
- `CONFIG.hud.tablet.size = [0.16, 0.21, 0.014]`（幅・高さ・厚み m。縦横比 0.76 は参考画像の原作タブレットの実測）、`cornerRadius 0.01`（幅の 6.3%、同じく実測）、`edgeRadius 0.0015`（厚み 14mm に対し 1.5mm の小さな縁 R。側面はほぼ平らで、iPhone 12 以降の角張った縁のイメージ）、`bezel 0.005`（原作は画面の UI が縁の近くまで届くので細い）、`screenRadius 0.005`（= cornerRadius - bezel で、ガラスの角が外形の角に沿う）。
- ルート `TransformNode('tablet')` を `camera` の子にする。
- 本体 `Mesh('tabletBody')`: `buildBody(w,h,t,r,e)` が生成する `VertexData`。角丸矩形アウトライン（`outline()`、各コーナー 8 分割、CCW）を、半径 `e` の丸縁プロファイル（前後それぞれ 5 ステップ、計 12 リング）で掃引し、背面(+Z)だけを `GeometryBuilder.cap()` の扇で塞ぐ（前面は次の `tabletFace`）。縁の UV は周長方向に沿わせる（ヘアライン筋が縁に沿って流れるように）。`build()` で面法線と頂点法線の向きが合うよう三角形の巻き順を補正。
- 前面 `Mesh('tabletFace')`: `buildPanel(w-2e, h-2e, r-e, -t/2)`（丸縁の内側の平らな前面）。縁・背面と材質を分けるための別メッシュ。
- 画面 `Mesh('tabletScreen')`: `buildPanel(w-2*bezel, h-2*bezel, screenRadius, -t/2 - 0.0005)` の平板（前面より 0.5mm 手前）。
- 3 メッシュ共通: `renderingGroupId = 1`、`isPickable = false`、`receiveShadows = false`（理由は「既知の制約・注意点」）、`alwaysSelectAsActiveMesh = true`。
- 深度クリア: rendering group 1 はレベル（group 0）の後に描かれる。ソース内に `setRenderingAutoClearDepthStencil` の明示呼び出しは無く、Babylon の既定（グループ切替時に深度/ステンシルを自動クリア）に依存している。これにより壁にめり込んでも欠けない。

### 2. マテリアル
- `createMetal()` → `PBRMaterial('tabletMetal')`（縁と背面）: `albedoColor = CONFIG.hud.tablet.metal.albedo [0.15,0.15,0.16]`、`metallic = 1`、`roughness = 1`、`metallicTexture = brushedTexture()`（G=roughness、B=metalness を使用、`useRoughnessFromMetallicTextureGreen/useMetallnessFromMetallicTextureBlue = true`）、`environmentIntensity = metal.environment (1.0)`、`maxSimultaneousLights = CONFIG.lights.shadowCasters + 1 (= 4)`。参考画像の原作タブレットは前面が黒く、縁だけが淡く光るので、暗いガンメタルにしてランプの光が縁に細く乗るだけにしている。
- `brushedTexture(scene, roughness=metal.roughness 0.4)`: 256×256 `DynamicTexture('tabletBrushed')`。ベース `rgb(255, round(0.4*255)=102, 255)` に、LCG 乱数（seed 1234567、16807 mod 2^31-1）で横方向 1px の筋を 1400 本（G を ±35 揺らす）。`gammaSpace = false`、WRAP、`uScale = vScale = 2`。
- `createFace()` → `PBRMaterial('tabletFace')`（前面）: `albedoColor = face.albedo [0.012,0.012,0.013]`、`metallic 0`、`roughness = face.roughness (0.55)`、`environmentIntensity = face.environment (0.6)`、`maxSimultaneousLights 4`。非金属なので映り込みはフレネルの分だけで、ほぼ黒に見える。
- `createScreen()` → `PBRMaterial('tabletScreen')`: 黒アルベド、`metallic 0`、`roughness = screen.roughness (0.12)`、`emissiveTexture = 画面 DynamicTexture`、`emissiveColor 白`、`emissiveIntensity = screen.intensity (2.0)`、`environmentIntensity = screen.reflection (0.3)`（参考画像では画面の黒い部分に映り込みが見えないので弱め）、`maxSimultaneousLights 4`。
- 画面 `DynamicTexture('tabletScreen', {600, 800})`（`screen.resolution`。ガラスの縦横比 0.15:0.20 に合わせる）、TRILINEAR、CLAMP、`anisotropicFilteringLevel = 8`。

### 3. ReflectionProbe
- `CONFIG.hud.tablet.reflectionProbe = { enabled: true, size: 128, refreshRate: 4, range: 14 }`。
- 描画対象は、コンストラクタに渡された `reflected`（静的メッシュ・門・灯の炎）のうち、ワールド AABB が目から `range`（14 m）以内のものだけ。`onBeforeRenderObservable` で目が前回選んだ位置から 2 m 動くたびに選び直す。Babylon の RenderTarget はキューブの各面で描画リストをメッシュ単位のカリングなしに全部描くため、館の拡張（静的メッシュ 28 タイル・灯 120）後に全部を渡していたときは、ヘッドレスの WebGL2 で 81.6 fps（p95 22.6 ms）まで落ちていた（絞った後 282.7 fps）。
- `new ReflectionProbe('tabletProbe', 128, scene, true, true, true)`、`renderList = reflected`、`refreshRate = 4`（4 フレームに 1 回）。メッシュには `attachToMesh` せず、`scene.onBeforeRenderObservable` で毎フレーム `probe.position = camera.globalPosition`（目の位置）にする。`probe.cubeTexture` を metal / face / glass の `reflectionTexture` に設定（シーンの HDR 環境より優先）。
- `syncEnabled()` で非表示になると `refreshRate = RenderTargetTexture.REFRESHRATE_RENDER_ONCE`、再表示で 4 に戻す。

### 4. 影響するライト
- `maxSimultaneousLights = 4` なので、`lights.ts` の影付き `PointLight('lampLight0..2')`（`shadowCasters = 3`）の影響を受ける。
- `level.ts` の `HemisphericLight('lightmapCarrier')` は `includedOnlyMeshes = staticMeshes` のため タブレットには当たらない。
- `receiveShadows = false` なので、ランプの光は影なしで当たる。タブレット自身もシャドウキャスターに登録されていない。

### 5. 配置と FOV（`place()`）
- `CONFIG.hud.tablet.position = [-0.194, -0.0368, 0.3105]`（カメラ空間 m、+x 右 / +y 上 / +z 前。本体の中心で、前面はそこから `size[2] / 2` 手前）、`rotationDeg = [0, 0, 0]`（スクリーンに平行。右辺の見え方は視差による）。
- 位置の根拠: 原作のタブレット（pak_reference の `BP_DD_PlayerCharacter` の `Tablet`、カメラの子。RelativeLocation の X 35.40・Y −21.99 cm）。出し入れのタイムライン `TabletInterp`（@24551〜24708）が X・Y を保ったまま Z を VLerp(−39.70, −4.32, Interp)、回転を RLerp([0, 90, 180], [0, 90, 0]) にするので、出した状態は Z −4.32・回転 (0, 90, 0)。メッシュ `tablet_new_pCube2` の箱は 18.5 × 1 × 23.8 cm（中心の Z +0.1475）で、回転 (0, 90, 0) では厚みがカメラ側へ伸び、前面が 34.40 cm 先に来る。本作の本体（0.16 × 0.21 m、縦横比 0.762。原作は 0.777）を原作を高さで合わせた k = 0.21 / 0.238 倍と見なし、位置も k 倍した: 中心 (−21.99k, (−4.32 + 0.1475)k) = (−0.194, −0.0368)、前面 34.40k = 0.3035 m で中心の z は 0.3105。FOV 90° の 16:9 では本体が x 4.9〜32.0 %・y 30〜91.5 %（幅 27.2 %・高さ 61.5 %）を占め、2026-09-13 の原作の収録（1080p60、静止時）の x 4.8〜31.8 %・y 30.3〜91.0 % と四辺とも 0.5 % 以内で合う（幅で合わせると高さが 2 % 大きく出る）。
- FOV: 原作と同じくカメラの子のまま動かないので、カメラの FOV（速さで 90〜115°。05 記録）が広がると画面の中心へ向かって縮む（ダッシュ 102.5° で 0.80 倍、ブースト 113.75° で 0.65 倍、115° で 0.64 倍。テレポートのカメラアニメやシェイクの FOV でも縮む）。同じ収録のダッシュ中の本体は 21.4 % × 48.9 %（本作 21.8 % × 49.4 %）、ブースト中はパワー枠の中心間の距離が静止時の 0.644〜0.657 倍（本作の本体は 17.7 % × 40.1 %）。
  - 根拠: 原作のタブレットの位置を動かすのは出し入れのタイムライン（`TabletInterp` 0.5 s・`Timeline_1` 0.3 s）だけで、速さでは動かさない。画面収録の縮み（ダッシュ 0.8 倍、ブースト中 0.637 倍）は 1/tan(FOV/2) と合い、FOV そのもの。縮み・戻りの速さは FOV の追従（05 記録の `fovFollow`、5.0/s）で決まる。2026-09-11 の収録のブーストの終わり（2 回）では、画面上の大きさから逆算した FOV の残りが 4.9〜5.0/s で縮み、出だしから 50 % が 0.13〜0.15 s、90 % が 0.45〜0.47 s。
- 後傾の角 `a = tilt × stow.tilt (55°)`、`half = size[1] / 2`。位置 = `(px, py - drop*lowerDrop - half + half·cos a, pz + half·sin a)`、`lowerDrop = 0.26`。下辺を軸に上端が奥へ倒れるので、中心は上と奥へ回る。x は固定（視点移動の横揺れは無い）。
- 回転 = `(rx° + a, ry°, rz°)`。しまうと下辺を軸に 55° 後ろへ倒れながら 26 cm 下がり、倒れた上端が 16:9 の画面の下端をちょうど越える。視点移動のロールは無い。

### 6. 揺れと出し入れ（`update()`）
- 速さでは動かない（原作どおり。ダッシュで少し下がる独自の上下の揺れ `swayY` は削除した）。
- `root` はカメラの子なので、歩き・走りのカメラシェイク（05 記録）もブースト・テレポートのシェイクもカメラと一緒に受け、画面上では揺れない（ユーザーの指示。原作のタブレットはカメラのコンポーネントの子で、カメラシェイクはカメラマネージャの視点にだけ掛かるので、原作では画面上でシェイクの逆に揺れる）。
- しまう・出す（`stow(dt)`）: 原作の検証映像（60fps）の実測に合わせた。`toggle()` で `stowTime = 0` とし、その時点の `drop` / `tilt`（0..1）を `stowFrom` に残す（途中で押し直してもそこから続く）。しまうときは `p = min(1, stowTime / stow.hideTime (0.12 s))` で `drop = from + (1 - from)·p²`（2 次の ease-in で加速して落ちる。映像は 7〜8 フレームで消える）、`tilt = from + (1 - from)·(1 - (1 - p)²)`（ease-out で落下より先に倒れる。映像は 2 フレーム目で約 23°、5 フレーム目で約 55°）。出すときは `left = exp(-stow.showRate (17) · stowTime)` で `drop = from·left`（1 フレームごとに残りが約 0.75 倍、約 0.22 s で収まる）、`tilt = from·√left`（倒れが位置より遅れて戻り、残り 8% でもまだ約 13°）。game.ts 側の `bob = sin(time*headBobFrequency(1.9)*2π) * 0.012 * min(1, speed/3)`。
- 原作ではマウスで視点を動かしてもタブレットは画面上で動かない。ラン中の縮み（0.8 倍）は FOV（5 節）。壁際でも寄らない: 原作データで位置を動かすのは出し入れのタイムラインだけで、2026-09-13 の収録でも扉の前で静止時より大きくならない（以前は物理レイで前方の壁を測って `reach.minDepth` まで手前へ寄せていたが、その根拠の「壁際で 1.25 倍」は、走りから止まったときの FOV の戻り〈102.5° → 90° で 1.25 倍〉を見ていたと考えられる）。
- 画面の再描画は 30Hz だが、どちらかの枠が弾んでいる間（`pops` が `POP_TIME` 0.5 s 未満）は `accum` を 1/30 以上にして毎フレーム描き直す。`pops`（枠ごとの `pop()` からの秒数、初期値 Infinity）は `update` で毎フレーム `dt` ずつ進める。

### 7. 表示状態（`syncEnabled()`）
- `shown = visible && !(lowered && stowTime >= stow.hideTime)`（しまい終えたら消す）。変化時に `root.setEnabled(shown)`、probe の refreshRate 切替、再表示時は `accum = Infinity`（即再描画）。
- `toggle()` は game.ts で Space/Tab に割当、SE `tablet_down` / `tablet_up`（volume 0.6）。

### 8. 画面描画（`drawScreen(f)` — 30Hz）
- `accum >= 1 / CONFIG.hud.minimap.redrawHz (30)` で再描画。`u = W/300`（元の 300px DOM 設計の 1px 相当。マーカーの大きさとグローの単位）。
- レイアウトは参考画像（原作のタブレットのスクリーンショット）で本体前面に対する割合を実測した `LAYOUT` 定数（x は本体の幅、y は本体の高さに対する割合）。ガラスは `bezel` だけ内側にあるので、`X(fx) = (fx*bw - bezel)/(bw - 2*bezel)*W`、`Y(fy)` も同様に画面テクスチャの px へ変換する（長さは `LX` / `LY`）。
  - `bar { top 0.103, bottom 0.224 }`、`socket { x 0.138, d 0.193, ring 0.0125 }`（左の円の中心 x、直径、輪の太さ。右は左右対称）、`count 0.095`（数字の高さ）、`frame { x 0.063, bottom 0.922, line 0.0075 }`、`band { top 0.868, text 0.026 }`（目的バンドの上端と字高）、`zoomKey { x 0.0866, baseline 0.8503, size 0.033, opacity 0.38 }`（「Z」ラベル）、`panel { x0 0.0717, x1 0.9253, top 0.243, bottom 0.9046 }`（原作のマップのパネル。地図の縮尺と中心、閃き）。この 2 つは参考画像ではなく原作データのウィジェットの配置から換算した（下記。原作のウィジェットの 1 px は本体前面の `WIDGET = { x 0.0013477, y 0.0010333 }`）。
- 背景 `#000` で全塗り。
- 上部の帯: 左右の円の中心どうしをつなぐ矩形（`sx = X(0.138)` 〜 `W - sx`、`bar.top` 〜 `bar.bottom`）。塗りは `shade()` の横グラデ `#433c3d → #615859`（原作では左が暗く右が明るい。画面の黒い部分に映り込みはないので、反射ではなく絵として描く）。
- マップ: `rect = { x: X(frame.x), y: 帯の下端, w: 左右対称, h: band.top まで }` を切り抜きの矩形として、`panel`（`LAYOUT.panel` を画面テクスチャの px にしたもの。原作のキャプチャが引き伸ばされるパネルで、地図の縮尺と中心はこれで決まる）と一緒に `minimap.draw(ctx, rect, panel, u, player, yaw, markers, arrow, time)` に渡し（11-minimap.md 参照）、`FRAME` 色・線幅 `LX(frame.line)` で `strokeRect`。
- シャード獲得の閃き（原作の `Count Shake` の `Image_41`。tablet-anim.ts）: `collectFlash(flash)` が 0 より大きい間、マップの直後（枠と目的バンドより前）に `flashImage` を `panel` へ引き伸ばし、不透明度 `min(1, collectFlash × FLASH_GAIN)` で描く。`flashImage` は `T_Vignette` の α（左右の縁で最大 153 / 255、上下の縁はほぼ 0）を `COUNT_SHAKE.flashColor` で塗ったもの（`tint()`）なので、マップの左右の縁が紫に光って 0.1 s で消える。
  - `LAYOUT.panel = { x0 0.0717, x1 0.9253, top 0.243, bottom 0.9046 }`: 原作の `Image_41`（と地図の `Image_80`）はマップのパネル（`CanvasPanel_762`。Overlay_102 の余白 34 の内側で、背景 `tablet_screen_bg` の 47.35〜673.33 × 176.36〜816.65。枠線より内側で、下端は目的バンドの下まで続く）いっぱい。`zoomKey` と同じ換算で本体前面の割合にした。目的バンド（原作の Button_0 も `Image_41` より後に並ぶ）が下端を覆う。
  - `FLASH_GAIN` 2.5: 本家の収録（2026-09-13、1080p60）の 204.27 s の回収では、マップの左の縁の帯（パネルの幅の約 2〜8 %）がマップの黒 (0, 14, 28) から最初のフレームで B 59（+31）、右の縁で +39 になり、1 / 0.77 / 0.39 / 0.06 と 4 フレームで消える。本作は発光をトーンマップするので原作どおりの α（縁で最大 0.15）では黒に沈む。ヘッドレスで 1/60 s ずつ撮ると、2.5 で左の縁の帯が最初のフレームで B +32〜35、1 / 0.69 / 0.38 / 0.06 と消え、光がパネルの幅の約 16 % まで届く（収録は約 13 %）。原作の画面の光が青紫に見えるのは館の青い色調のためで、本作は原作データの紫のまま（R も少し上がる）。
- 目的バンド: 枠の内側の最下部（`band.top` 〜 `frame.bottom`、幅は枠の外側まで）を `shade()` で塗る。文字は大文字化して末尾のピリオドを除き（原作の表示に合わせる。`GameState.objective` の文字列は変えない）、`700 (LY(band.text)/CAP)px HUD_FONT`、`#f2f2f2`、グロー `rgba(255,255,255,0.5)` blur `5u`、字高の中央を帯の中央に置く。幅超過時はフォントサイズを縮小。
- パワー枠（帯の両端に重ね、マップの上の角にもかかる）: `drawSocket()` は黒い円 + `RING` 色の輪（直径 `LX(socket.d)`、輪 `LX(socket.ring)`）。
  - 両方とも `drawSlot(ctx, icon, spec, x, y, d, ring, u, percent, scale)`（`percent` は `setRefill` で受けた値、`scale` は `popScale(pops[枠])`）: `scale` が 1 でなければ枠の中心を軸に拡大してから、枠（`RING` の輪。光らせない）を描き、`POWER_ICONS` のアイコン画像を辺 `d*spec.size` で、画像内の不透明部分の中心 `(spec.cx, spec.cy)` が枠の中心に来るように置く（下記の原作の表示）。原作の参考画像に合わせ、キー表示は描かない。
  - `POWER_ICONS`（原作のタブレットと同じく左 Teleportation、右 Speed Boost）: `teleport = { url './tablet/teleportation.webp', size 0.73, cx 0.502, cy 0.475 }`、`boost = { url './tablet/speed_boost.webp', size 0.86, cx 0.526, cy 0.498 }`。size は参考画像で測った見える部分の大きさ（枠の直径に対しコイル 0.61×0.65、矢 0.72×0.71）を、画像の不透明部分の外接矩形（1000px 中 826×912 / 825×842）で割ったもの。
  - 原作の表示（両方の枠。ユーザーの指示）: 原作（pak_reference の `BP_Powers`・`UMG_TabletPowers`・`MM_Powers`）の枠のマテリアルは、通常のアイコン（`DisabledPower` = `ring_altar_power_teleport_icon` / `..._speed_boost_icon`）と灰色のアイコン（`EnabledPower` = `..._icon_inactive`）を、UV を角度にした値（`VectorToRadialValue`）と `Percent` で切り替える円弧のマスク。`drawSlot` は `percent` < 1 のとき、アイコンを `grayscale(1) brightness(2.6)`・不透明度 0.8 で描き（灰色のアイコンの代わり）、`percent` > 0 ならその上に色つきのアイコン（赤い影 `rgba(224,38,42,0.55)` blur `4u`）を、枠の中心を頂点とする半径 `d*spec.size` の扇形（-90° から時計回りに `TAU × percent`）で切り抜いて重ねる。角度の起点と向きは、`VectorToRadialValue` の中身もマテリアルのつなぎも cook で消えていて決まらないので、12 時から時計回りにした（UV を分解して組み直しているのは起点を回すためと読め、以前の画面収録でテレポートの枠に右から色が戻って見えたこととも合う）。輪の光と円弧は描かない。
  - 左の枠（テレポーテーション、05 記録）: Q で `Set Delay(0.05)` → `Percent` が 1 なので長さ 1 のタイムラインを再生速度 20 で逆再生（0.05 s で灰色）、移動の後の `Set Delay(5)` で 0 から再生速度 1/5（5 s で元の色）、取り消し（`Reset Teleport`）で即座に 1。
  - 弾み（`popScale(t)`）: 原作の `UMG_TabletPowers` のウィジェットアニメ「Use Left」/「Use Right」（パワーの枠の `RenderTransform` の拡大 1 → 1.25（0.05 s）→ 1.1（0.15 s）→ 1（0.5 s）。キーは 3 次で UE の自動の接線 = 前後のキーを結ぶ傾き、両端は 0）。原作の `Use Power` はこれを先頭で再生してから使えるかを判定するので、使えないとき（クールダウン中）や取り消しの Q でも弾む。game.ts は Q / E を押すたびに呼ぶ。
  - 右の枠（スピードブースト、05 記録）: 原作の `Set Delay Speed Boost` は FlipFlop で、1 回目（E の発動の `Set Delay(6.75)`）はタイムラインを逆再生、2 回目（効果が切れた後の `Set Delay(8.5)`）は頭から再生する。つまり効果中に色が 1→0 へ減り、効果の後のクールダウンで 0→1 へ戻る（時間は既定の強化レベルの値で、本作の `player.boost` の `duration` / `cooldown` と同じ）。残り時間の表示はこの枠だけ。
- 中央シャード数: 帯の中央、`700 (LY(count)/CAP)px HELVETICA_BOLD`（数字の高さを本体高さの 9.5% にする）、白系 `#f5f5f5`、回収のたびに原作の `Count Shake` の数字の部分（`countShake(flash)`。tablet-anim.ts）で揺らす: 帯の中心（原作の文字の枠は帯と同じで、回転・拡大の軸はその中心）を軸に `scale` 倍し、静止位置から (`LX(x × WIDGET.x)`, `LY(y × WIDGET.y)`) ずらす。0.1 s で、最初に 12 px 下へ飛び、0.025 s に左下 (−14, +21) で 1.1 倍、0.05 s に静止位置で 1 倍、また下がって 0.1 s に元へ戻る。色は変えない（原作の Count Shake に色のトラックは無い）。影 `rgba(255,255,255,0.45)` blur `10u`。
  - フォントの根拠: 原作の `ShardCount`（helvetica-neue-bold_Font、Size 100 = 96 DPI で 133.3 px、`OutlineSettings.OutlineSize` 4、中央揃え）。Helvetica Neue Bold の数字の高さ 0.714 em で 95.2 px = 本体高さの 0.098 で、`LAYOUT.count` 0.095 とほぼ合う。輪郭（UE の既定で黒 4 px）は描かない: 原作の UI テスト映像（Teleportation、720p）の「0」は縁に黒が見えず白いにじみに見えるので、これまでの白い影のままにした。
- 描く順: 帯 → マップ → 閃き → 枠 → 目的バンド → パワー枠 → 数字 → 「Z」（原作でも Z はウィジェットのルートで画面のキャンバスの後に並ぶ）。
- 赤: タブレットは画の一部なので、テレポートとスピードブーストのカメラアニメのシーンのティント（09 記録）で、ほかと一緒に赤くなる（原作と同じ）。画面の描き方は赤のときも変えない。
- 「Z」ラベル（Z キーのズーム。11 記録）: マップの左下、目的バンドのすぐ上に、白の `Z` を `700 LY(zoomKey.size)px HELVETICA_BOLD`、`textAlign left`・`alphabetic` で `(X(zoomKey.x), Y(zoomKey.baseline))` に不透明度 `zoomKey.opacity` 0.38 で描く。
  - 根拠: 原作（pak_reference の `UMG_Tablet`）の `TextBlock_107` "Z"（helvetica-neue-bold_Font、既定の 24pt = 96 DPI で 32 px、RenderOpacity 0.1、白）。スロットはルートの中心から (−304, 312) の自動サイズで、背景 `tablet_screen_bg`（714 × 864）では行の左上 (51, 732)。フォントの hhea（upm 1000、ascender 975、descender −217、lineGap 29）から Slate のベースラインは行の上端 + 32.13 px = 764.13。背景の px を `LAYOUT` の割合へは、背景の枠線の中心 x 33.5 / 682 を `frame.x` 0.063 / 0.937、帯の下端 158 を `bar.bottom` 0.224、目的バンド（Button_0）の上端 781.25 を `band.top` 0.868 に合わせて換算した（x 0.0013477、y 0.0010333 / px。どちらも本体で約 0.216 mm / px）: x 0.0866、ベースライン 0.8503、字の大きさ 0.033。2026-09-13 の原作の収録（1080p60、静止時）の Z は 13 × 16 px（幅 × 字高）でベースラインが目的バンドの 11.5 px 上、この換算の予測（13 × 15.4 px、11.6 px 上）と合う。
  - 明るさ: 原作の 0.1 は収録の画面でマップの黒 (0, 14, 28) の上に輝度 37〜42（マップの線 35、目的バンド 24 より明るい灰）。本作の画面は発光をトーンマップするので 0.1 では黒 (0) の上に 2 しか出ず見えない。ヘッドレスで撮った画面の輝度が 0.3 で 26、0.45 で 54 だったので、原作の約 40 になる 0.38 にした（ほかの灰色と同じく、画面上の見え方で合わせる）。
  - フォントは原作の `helvetica-neue-bold.ttf` を同梱して使う（`HELVETICA_BOLD`。13 記録）。読み込めないときだけシステムの Helvetica Neue / Helvetica / Arial になる。
- 最後に `screen.update()`。

### 9. アイコン読込
- `loadImage(url)` が `public/tablet/` の webp を `Image` で読み（URL はタイトル画像と同じくページからの相対 `./tablet/...`。Vite の `base: './'`）、`onload` で `accum = Infinity`（即再描画）。読み込み前はアイコンなしの枠だけを描く。
- 閃きの画像は `FLASH_URL = './title/death-vignette.webp'`（死亡画面の赤いビネット。原作の同じ `T_Vignette` の α に赤を付けたもの。13 記録）を読み、`onload` で `tint(image, COUNT_SHAKE.flashColor)`（同じ大きさのキャンバスに描いて `source-in` で塗る = α だけ残して紫にする）を `flashImage` にする。読み込み前の回収では閃きを描かない。

### hud.ts（DOM HUD）
- 要素 id: `#hud`（root）、`#interact`、`#subtitles`、`#saving`（index.html に定義）。以前の拾得プロンプト `#prompt`（呼び出し元が無かった）と中央のバナー `#banner`（全回収の `COLLECTED ALL SHARDS`、欠片の `RING PIECE`）は原作に無いので消した。ファンゲームのロックピックのウィジェット `#lockpick`（`Lockpicking`）も、障壁を手のマークと 1 クリックで壊すようにしたので消した（04 記録）。
- `setInteract(on)`: `#interact`（手のマーク）の `hidden` を、変わるときだけ切り替える。game.ts が毎フレーム「視線の先に使えるものがあるか」で呼ぶ（04 記録）。
- `showSaving()`: `#saving` を表示し `CONFIG.game.savingToastSeconds (2.6)` 秒後に隠す（再呼出でタイマーリセット）。
- `voiceSubtitles`（既定 true）: 設定の SUBTITLES。false なら `speaker` 付きの字幕（ワサミの声に付くもの）を出さない（原作の SetSubtitlesEnabled は声の字幕だけに効く。案内の字幕は出す）。
- `subtitle(text, speaker?, seconds=3.2)`: `speaker` があって `voiceSubtitles` が false なら何もしない。それ以外は `<p>` を追加（speaker があれば `<span class="speaker">名前:</span>` を先頭に）、3 件以上なら古いものを削除。`seconds` 後に `is-out` を付け 450ms 後に remove。ボイス字幕は `max(2.2, 音声長 + 1.2)` 秒。
- `fade(opacity, seconds)`: 全画面の黒 `#fade`（`#hud` の外、舞台の直下。index.html）の `transition` を `opacity <seconds>s linear` にして `opacity` を入れる。ホテルのエレベーターの乗車の暗転と明け（04 記録）。`syncWorld` / `toCheckpoint` が `fade(0, 0)` で消す。

### styles.css（現存クラス）
- `@font-face 'DD Roboto'`（`/fonts/Roboto-Bold.ttf`、weight 700、`font-display: block`。原作のエンジンの Roboto Bold。13 記録）、`@font-face 'DD Helvetica Neue'`（`/fonts/helvetica-neue-bold.ttf`、weight 700）と `'DD Helvetica'`（`/fonts/helvetica-normal.ttf`、weight 400）（原作の UI の書体 helvetica-neue-bold_Font / helvetica-normal_Font、どちらも `font-display: block`。13 記録）。`:root { --red: #e0262a; --hud-font: ... }` と、16:9 の舞台の大きさ `--stage-w: min(100vw, 100vh * 16 / 9)` / `--stage-h: min(100vh, 100vw * 9 / 16)`、メニューの共通の変数 `--u: calc(var(--stage-h) / 1080)`（下記）、`--menu-font: 'DD Helvetica', Helvetica, 'Helvetica Neue', Arial, var(--hud-font)`（原作の `helvetica-normal_Font`: タイトルのメニュー・版と著作権、ポップアップの本文、死亡画面のボタン）、`--options-font: 'DD Helvetica Neue', 'Helvetica Neue', Helvetica, Arial, var(--hud-font)`（原作の `helvetica-neue-bold_Font`: OPTIONS の見出し・値・ボタン、ShardStreak、VignetteSides、Collectables、死亡画面の REMAINING LIVES、脱出の画面）、`--roboto: 'DD Roboto', Roboto, var(--options-font)`。`[hidden] { display: none !important }`。
- `.stage`（`#stage`）: 窓に収まる最大の 16:9 の箱（`position fixed; inset 0; margin auto`、幅 `--stage-w`・高さ `--stage-h`、`overflow hidden`、`contain: layout paint`）。窓が 16:9 より横長なら左右、縦長なら上下に body の黒（`#000`）が黒帯として見える。containment により、中の `position: fixed` の層（`.hud`・`.screen`・`.options-layer`・`.stats`・`.fatal`）は窓ではなく舞台を containing block にして舞台いっぱいに広がり、はみ出しは舞台で切れる。`#scene` は `position absolute; inset 0; 100% × 100%` で舞台いっぱい（Babylon の `engine.resize()` がキャンバスの表示寸法から描画解像度を取るので、描画も 16:9 になる）。
- `.hud`: `position: fixed; inset: 0; pointer-events: none`。
- `.subtitles`: `bottom: 6.5vh`、幅 `min(70vw, 1100px)`、`500 clamp(15px, 1.05vw, 22px)/1.45 'DD Helvetica', var(--hud-font)`（原作の台詞 `UMG_DialogueBox` は helvetica-normal_Font。和文はシステムの和文へ落ちる）。`p` は `fadein 0.25s`、`p.is-out` は `opacity 0; transition 0.4s`。`.speaker` 色 `#c9a2ff`。
- `.interact`（`img#interact`、`src` は `./title/interact.webp`。13 記録）: 画面の中央（`left/top 50%` と `translate(-50%, -50%)`）に `90u` 四方、`opacity 0.5`、`pointer-events: none`（原作の `UMG_Interact` の `Image_18`: `interact_icon_03` を 90 × 90、白の α 0.5、アンカーとアラインメント 0.5 の自動の大きさ）。
- `.fade`: `position: absolute; inset: 0`、黒、`opacity 0`、`pointer-events: none`（ゲームと HUD の上、メニューの下）。
- `.saving`: 右下（`right 2.4vw; bottom 2.6vh`）、`300 clamp(13px, 0.95vw, 19px) var(--roboto)`（原作の `UMG_Saving` の RobotoTiny の Light = 同梱の Roboto Light）、`letter-spacing 0.07em`。`.saving__icon`: 0.85em の円、`border 2px rgba(225,225,225,0.35)`、上辺のみ白、`@keyframes spin` 0.8s linear infinite。
- `@keyframes fadein { from { opacity: 0 } }`。
- `.stats`: 右上 FPS オーバーレイ（`#9f9`、monospace 12px、z-index 20）。
- `.screen`（fixed grid center, z-index 10）、`.screen--loading`（無地の `#050202`、中央に `.progress` だけ）。
- `.progress` / `.progress__bar`（高さ 3px）/ `.progress__bar i`（`linear-gradient(90deg, #7a0c0c, #ff3b2f)`、`transition: width 0.15s`）、`#progress-label`。
- `.fatal`（下部の赤いエラー枠、z-index 30）。

### styles.css（タイトル画面 = 原作の `UMG_TitleScreen`）
原作（pak_reference の `UMG_TitleScreen`、v1.6.1）のウィジェットの配置を UE の単位のまま置いている。UE はウィジェットを 1920×1080 で組み、画面の短辺 / 1080 倍に拡大する（UE 既定の DPI。原作データに独自の設定は見当たらない）ので、`:root` の変数 `--u`（舞台 `#stage` の高さ / 1080。舞台は 16:9 なので短辺は常に高さ）を 1 単位とし（ポーズ画面と OPTIONS も同じ）、各要素を原作と同じ辺・中央に固定してオフセットを `calc(N * var(--u))` で書く（以下 `Nu`）。フォントサイズは UE のポイント（96 DPI）なので px は ×4/3。
- `.screen--title`: `display: block`（`.screen` の grid 中央寄せを打ち消す）、`overflow: hidden`、背景 `#000`。フォントの変数は `:root`（上記）。表示時のフェードインは CSS ではなく `.title__cover`（title.ts）。
- 重なり順（DOM 順 = 原作のウィジェット木の順）: `img.title__monster`（Image_97）→ `.title__mask`（VideoMask）→ `.title__wisps`（Image_104）→ `h1.title__logo`（Image_103）→ `.title__copyright`（TextBlock_79）→ `nav.menu`（VerticalBox_160）→ `.title__fade`（Image_0）→ `.title__cover`（Image_128）→ `.title__flash`（Image_2）→ `.title__version`（TextBlock_0）→ `#restart`（`.confirm-layer`、z-index 3）。OPTIONS（`#options`）は `#title` の外（下記）。
- `.title__monster`（原作は敵の顔の 1100u の正方形。本作はワサミの顔写真）: 右端の中央に固定し `left: calc(100% − 1089.6u)`、`top: calc(50% − 549.2u)`、1100u 四方。512px の写真なので `filter: grayscale(0.75) sepia(0.5) hue-rotate(38deg) saturate(0.85) brightness(0.46) contrast(1.45)` で暗く緑がかった色にし、`mask-image: radial-gradient(ellipse 50% 50% at 50% 52%, #000 30%, rgba(0,0,0,0.5) 58%, transparent 86%)` で写真の背景（本棚・白い壁）を黒へ溶かす（原作の顔の画像は背景が黒い）。
- `.title__mask`（VideoMask）: 左上から `2029.65u` × 全高を黒で塗り、`public/title/mask.webp`（原作の `title_screen_video_mask` の形。白。左が不透明、右が煙状に透明）を `100% 100%` に引き伸ばしたマスクにする。顔の左側が煙の黒に呑まれる。
- `.title__wisps`（Image_104 = `MM_TitleScreen_Mask_Grey`）: 左上から `1654.65u` × 全高を灰 `rgb(179, 179, 179)`（13 記録の推定の灰）で塗り、マスクを 2 枚重ねて両方がある所だけ残す（`mask-composite: intersect`、`-webkit-mask-composite: source-in`）: `public/title/wisps.webp`（原作の `title_screen_chapters_background` の筆の跡の形。白。13 記録）を `auto 100% repeat-x`（テクスチャの 4.8:1 のまま画面の高さに合わせる）で敷いたものと、`mask.webp` を `0 0 / 100% 100%`（原作のマテリアルと同じく video_mask の α で左側だけに出す）。`@keyframes wisps` 100s linear infinite で筆の跡のマスクの位置（`mask-position` の 1 枚目）を `−480vh`（1 タイル）まで動かして左へ流す。UV の倍率・パンの速さ・不透明度の掛け方は cook で消えているので推定。
- `.title__logo`: 原作ロゴ（Image_103: (4, −44)、1043.7×564.9u）の文字が占める (54, 78)〜(919, 398) に本作ロゴ（`public/title/logo.webp`、1942x809、透明の余白込み）の文字を合わせ、`left 58.8u; top 61.6u; width 848u`（文字の幅 830u、中心 (486.5, 238)）。`img` に `drop-shadow(0 0 116u rgba(255,40,0,0.28))`: 原作ロゴのテクスチャに描かれたグロー（(255,40,0)、文字の縁で α 0.14、約 100u で 0）に、ぼかし 2σ = 116u・縁で半分になる α 0.28 で近づけたもの。
- `.menu`: 左端の中央に固定し `left 7.06u; top calc(50% − 93.09u); width 421.75u` の縦並び。
- `.menu__item`（原作のボタン。本作の項目は RESUME / NEW GAME / OPTIONS だけ）: `height 73u`、`margin-bottom −10u`（VerticalBoxSlot の下 −10 で重なり、間隔 63u）、`padding 15u 0 0 52u`（UE のボタンの既定の余白 2 + ButtonSlot の左 40・上 3 + 文字の Margin 10）、`400 40u/46u var(--menu-font)`（30pt）、色 `#5c5c5c`（原作の `Unhovered Color` linear 0.107 の sRGB）、左寄せ。`position relative`・`isolation isolate`。`::before`（ボタンいっぱい、`z-index −1` で文字の後ろ）を原作の筆の色 `rgb(99, 8, 0)` で塗り、`public/title/marker.webp`（原作の `title_screen_selection_marker` の形。白）をマスクにする。マスクは大きさ 0 で置いておく（画像の読み込みを先に済ませる）。
  - `:hover` / `:focus-visible` / `:active`: `::before` のマスクを `100% 100%`（原作の Hovered / Pressed のブラシ。赤い筆がボタン全体に引き伸ばされる。ブラシに Tint は無い）、色 `#fff`、outline なし。Slate はブラシを瞬時に切り替えるので transition はない。
- `.title__copyright`（左下。上端が下から 81.08u、`left 48u`）と `.title__version`（右上 `top 12u; right 16.6u`。原作は右端から 81.92u の位置から左寄せで、`v1.6.1` の右端がここ）: `400 24u/28u var(--menu-font)`（18pt）、`#5c5c5c`、`white-space: nowrap`。
- `.title__fade`（Image_0、黒）/ `.title__cover`（Image_128、黒）/ `.title__flash`（Image_2、`rgb(141,0,6)` = linear (0.266, 0, 0.002)）: `inset 0`、`pointer-events none`、`opacity 0`。不透明度は title.ts のアニメが動かす。
- OPTIONS（原作の `UMG_Options`。`#options` = `.options-layer`）: 原作のウィジェット木を同じ単位で写したもの。
  - `.options-layer`: `position fixed; inset 0`、`z-index 12`（タイトルとポーズの両方から開くので、どちらの `.screen`（10）より上。原作もポーズからは ZOrder 10 でポーズの 5 の上に作る）、`scale 1.015`（原作のルート CanvasPanel_0 のレンダー変換）。子の `.options__backdrop`（Blur+Red: ぼかし 8 と赤 linear (0.182, 0, 0) の 0.371）は RESTART? の `.confirm__backdrop` と同じ規則。
  - `.options`（CanvasPanel_2。開閉のアニメの対象）: `inset 0`。`img.options__frame`（`options-frame.webp`）は左右 179.396u、上下 −0.959u のストレッチ（`width calc(100% − 358.792u)`、`height calc(100% + 1.918u)`）。見出し `OPTIONS` は枠に描かれているので、`.options__title` は見えなくする（`.confirm__title` と同じ規則）。
  - `.options__grid`（GridPanel_0）: 中央から `left −638.052u; top −343.1u`、`1234.535u × 673.848u`。原作の CanvasPanel_1（中央から (−673.562, −383.544)、1338.088 × 812.282）の中央から (−16.266, −28.773) に整列 0.5 で置かれたものを、画面の中央からの位置にした。
  - `.options__box`（VerticalBox）: `position absolute; width max-content; flex column`。`left` / `top` は options.ts が決める。レンダー変換の平行移動は `graphics` / `audio` が `translate −50u 0`、`controls` が `0 254u`、`difficulty` が `650u −92u`。
  - `.options__head`: `700 48u/1.192 var(--options-font)`（Helvetica Neue Bold 36pt。行の高さは原作のフォントの hhea の (975 + 217)/1000）、白、`min-width 550u`、中央揃え。
  - `.options__row`（HorizontalBox）: `flex; align-items center`、`margin-top 15u`（見出しの直後の行は `--first` で 25u。AUDIO の最初の行は 15u のまま）。`--unused` は `visibility hidden`（原作の RESOLUTION の 2 行。一方は不透明度 0、もう一方は Hidden で、どちらも場所は取る）。
  - `.options__label`: `700 32u/1.1719 var(--roboto)`（UE の TextBlock の既定 Roboto Bold 24pt。行の高さは hhea の (1900 + 500)/2048）、`#bcbcbc`（linear 0.5）、`min-width 310u`（DIFFICULTY の行は `--short` で 240.994u）。
  - `.options__value`（Border + TextBlock）: `content-box`、`min-width 61.277u`（`--wide` は 215u）、`padding 10u`、`border-image: url(options-box.webp) 10% fill / 24.2u stretch`（原作の Box 描画: 余白 0.1 × ImageSize 242 = 24.2u。テクスチャが 242×55 なので、上下の縁は 4.4 倍に引き伸ばされて太く見える）、`700 24u/1.192 var(--options-font)`（18pt）、`#bcbcbc`、中央揃え。スライダーの行は `--gap` で左に 26u。
  - `.options__arrow`（中身の無いボタン = ブラシの大きさ 21×32u）: `options-arrow.webp`（白い山形）をマスクにし、通常は原作の矢印の赤 `rgb(192, 0, 0)`、hover / focus-visible で白（原作のホバーのブラシは同じ山形の白）、押下で赤に戻る。左は `--left`（`rotate 180deg`、右に 5u）、右は左に 5u。
  - `.options__slider`: `flex 1 0 16u`（Fill。望む幅は 16）、高さ 30u、右に 30u（MOUSE SENSITIVITY は `--tight` で −10u）。`.options__bar` は左右 9u の内側に高さ 4u の赤 `#f00`（SliderBarColor (1,0,0)）。`.options__thumb`（`options-thumb.webp` を 18×30u）は `left calc(9u + var(--v) × (100% − 36u))`（SSlider はつまみの幅だけ両端を空ける）。
  - `.options__check`: `57u × 64u`（画像 55×64 と右の余白 2）、左に 18u。`aria-checked` で `options-check.webp` / `options-checked.webp` を切り替える（2 枚を重ねて置く）。hover / focus-visible で `filter: brightness(0.749)`（原作の Construct がホバーの画像に付ける色 linear 0.515625）。
  - `.options__actions`（HorizontalBox_11）: `left 50%; top calc(100% − 6.752u)`、`translate −50% −125%`（整列 (0.5, 1.25)）。`.options__btn`: `700 40u/1.192 var(--options-font)`（30pt）、`#5f5f5f`（ボタンの ColorAndOpacity linear 0.1146）、`padding 2u`（押下は 3u 2u 1u。UE の既定のボタン）、hover / focus-visible で白。`#options-save` は右に 40u、`#options-cancel` は左に 40u。
- RESTART?（セーブありの NEW GAME。原作の `UMG_PopUp` に Frame 0 の枠）:
  - `.confirm-layer`: `inset 0`、`z-index 3`。子の `.confirm__backdrop`（`popup-backdrop`。カード `.confirm` は `popup-card`。title.ts の開閉のアニメがこの 2 つのクラスで引く。原作の CanvasPanel_3: `BackgroundBlur` の強さ 8 と赤い `redblock` linear (0.182, 0, 0, 0.371)）は `rgba(118,0,0,0.371)` + `backdrop-filter: blur(8u)`。背景とカードの不透明度を別々にアニメさせるため兄弟にしている。
  - `.confirm`（カード）: 画面中央に `1222u × 928u`、`translate: -50% -50%`（拡大のアニメは `scale` プロパティで掛けるので transform は使わない）。`img.confirm__frame` が `public/title/restart-frame.webp`（原作の `restart_window_frame_2`: 縁のかすれた黒い地、赤い細線、赤い見出し `Restart?`）。
  - `.confirm__title`（`RESTART?`）: 見出しは枠に描かれているので、支援技術向けに残して見えなくする（1px、`clip-path: inset(50%)`）。
  - `.confirm__text`: `top 516u`、`400 37.33u/1 var(--menu-font)`（原作の `PopupText`: Helvetica 28pt、linear 0.965 → `#fbfbfb`）。`.confirm__actions`: `top 624u`、中央寄せ、`gap 90.7u`。`.confirm__btn`（YES / NO）: `700 37.33u/1 var(--roboto)`（UE の TextBlock の既定 Roboto Bold 28pt。同梱の `DD Roboto`）、`#5f5f5f`（ボタンの ColorAndOpacity linear 0.1146）、`:hover` / `:focus-visible` で白。文字の行の位置と間隔は、pak の値（VerticalBox の +115.2、YES の右 50 など）と UE の既定のボタンの余白からは確定しきれないので、v1.5.3 の画面収録で測った値（枠の赤線の上端から 29.8vh・39.8vh、間隔 8.4vh）を 1080 基準の単位にしたもの。

### styles.css（死亡画面 = 原作の `UMG_DeathScreen`）
原作（v1.6.1）のウィジェット木を同じ単位（`--u`）で写したもの。1080p の収録（03_Manor）で、見出しの字の上端 325 px・ヒント 707 px・ボタンの間隔 75 px・色を確かめた。
- 重なり順（DOM 順 = CanvasPanel_0 の Slots の順）: `.death__back`（Button_22。黒、各辺から 25u はみ出す）の中に `.death__lives`（HorizontalBox_114。中央）→ `.death__menu`（VerticalBox_161）→ `.death__heading`（TextBlock_149）→ `img.death__dead`（Image_0）→ `.death__tip`（Tips）→ `.death__cover`（Button_0）→ `.death__vignette`（Image_161）。その上に RESTART の `#death-restart`（`.confirm-layer`。タイトルの RESTART? と同じ部品と `restart-frame.webp`、文言 `ARE YOU SURE YOU WANT TO RESTART?`）と、LAST CHECKPOINT の警告 `#death-warning`（同じ部品に `blank-frame.webp`〈原作の `UMG_PopUp` の Frame 2 = blank_window_frame。題字の無い枠〉、`.confirm--long`、文言 `Obtaining S Rank is not possible with Last Checkpoint.` / 空行 / `Continue anyway?`。原作の `Text` の `\r\n \r\n` を `<br />&nbsp;<br />` で）。`.confirm--long`: 3 行を原作の縦並びの箱の中央のまま置くため、行の高さを 1.3（推定）にし、文を 54.13u 上へ、ボタンを 54.13u 下へ（3 × 48.53 − 37.33 の半分）。
- `.screen--death`: `display: block`、`overflow: hidden`。`.death__canvas`: `inset 0`（Shake が `translate` を動かす）。
- `.death__life`: 100u 四方、周りに 10u（HorizontalBoxSlot の余白）。`public/title/life.webp` をマスクにし、`background-color` を塗る（既定 `rgb(124,124,124)`。Shake が `skullColor` で赤くする）。
- `.death__menu`: 横は中央、上端が下端から 358.19u、縦並び、既定 `opacity 0`。`.death__btn`: 周りに 10u（スロット）、内側 4u・6u（UE のボタン 2 + ButtonSlot の既定 (4, 2)）、`400 40u/46u var(--menu-font)`（Helvetica 30pt）、`#454545`（文字の linear 0.521 × ボタンの Unhover Color 0.115。収録の実測 70）、既定 `opacity 0`。hover / focus-visible で `#bfbfbf`（ボタンの色が白 = 文字の 0.521）。原作のホバーは色だけで音は無い。
- `.death__heading`: 上端 308.4u の中央、`700 66.67u/1.192 var(--options-font)`（Helvetica Neue Bold 50pt）、白。
- `.death__dead`: 中央に 1285 × 301u、`scale 1.05`、既定 `opacity 0`。
- `.death__tip`: 中央から右 2.54u・下 161.33u を上端の中央に、`300 32u/1.1719 var(--roboto)`（RobotoTiny の Light 24pt。同梱の `DD Roboto` の weight 300 = `Roboto-Light.ttf`。日本語は `--hud-font` の日本語書体へ落ちる）、`#cecece`（linear 0.62）。
- `.death__cover`: 黒、左 −49.55u・上 −76.58u・右 −43.54u・下 −82.58u（Button_0 のオフセット）、クリックを通す。既定は不透明（Fade In が消す）。
- `.death__vignette`: 画面いっぱいを Image_161 の色 linear (0.38, 0, 0) = sRGB 166 の赤 `rgb(166, 0, 0)` で塗り、`public/title/death-vignette.webp`（T_Vignette の α。白）をマスクにする。既定 `opacity 0`。
- `@font-face 'DD Roboto'` の weight 300（`/fonts/Roboto-Light.ttf`、`font-display: block`）。

### styles.css（脱出の画面 = 原作の `UMG_LevelClear`）
- `.screen--clear`（`display: block`、`overflow: hidden`、UI 入力までは `cursor: none`、`.clear--input` で既定のカーソル）。`.clear__canvas`（ルートのキャンバス。全体の不透明度と FINAL RANK の揺れ）の中に原作のスロット順で、`.clear__black`（Image_4。端から少しはみ出す黒）、`.clear__results`（ResultsBox: 中央の 1820 × 980 単位の縦並び。Helvetica Neue Bold〈`--options-font`〉、行の高さ 1.192）、`.clear__easy`（Roboto Bold 24pt = 32px、`rgb(173, 173, 173)`、左上が 48.36 % − 213.86 単位・90 % − 74.58 単位）、`button.clear__next`（左上が右端から 163.38・下端から 89.08 単位、46.67px、ボタンの色 linear 0.115 = `rgb(95, 95, 95)`、ホバーで白）、`.clear__red`（ClearLevel。`::before` が Image_5 の赤でさらに外まで塗り、揺れても端が出ない）とその中央の `img.clear__escaped`（1141 × 276）、`.clear__fade`（Fade Out の黒）、`.clear__vignette`（`death-vignette.webp` の α を白に）と `.clear__flash`（白）。
- ResultsBox の中: `.clear__level`（648.72 × 129.6 の箱に `stage-title.webp` のマスクを `contain` で収めて `rgb(196, 0, 0)`。原作はレベルのタイトルカードの絵〈館は灰 196 の `chapter_ui_title_deadlydecadence`〉にブラシの Tint (1, 0, 0)。以前は `#f00` で鮮やかすぎた）、`h2.clear__heading`（RESULTS、36pt = 48px、`rgb(105, 105, 105)`、余白 20）、`img.clear__line`（`results-line.webp` を幅いっぱい・高さ 18、余白 10）、`.clear__rows` の 6 行 `.clear__row[data-row]`（縦中央: `.clear__label` 左 250・最小幅 400・46.67px、`.clear__value` 左 500・最小幅 196.15・右寄せ・40px、`.clear__rank` 左 30・53.33px、`.clear__shards` 左 65・最小幅 243・33.33px の紫 `rgb(121, 65, 150)`）、2 本目の線、`.clear__total`（`TOTAL SHARDS:` 40px と値 66.67px・最小幅 140.66・中央寄せ、紫 `rgb(121, 66, 150)`）、`.clear__final`（`FINAL RANK` 66.67px `rgb(173, 173, 173)` と 100px の文字。既定の拡大 1.15）。色は原作の linear を sRGB にしたもの。

### styles.css（ステージ OP = 原作の `UMG_ChapterPortal`）
- 原作のウィジェット木を同じ単位（`--u`）で写す。`.intro` は `position: fixed`・`z-index 5`（HUD の上、画面〈10〉の下。原作の AddToViewport の Z 5）、`pointer-events: none`。DOM の順は原作のスロットの順: `.intro__black`（本作の推定の黒、`#000`）→ `.intro__canvas`（CanvasPanel_0）の中に `.intro__blur`（BackgroundBlur_0。オフセットどおり左 40.96・上 34.08・右 21.28・下 33.67 単位だけ画面の外まで）→ `.intro__wash`（Image_1。linear (0.109, 0, 0) の 0.2 = `rgba(93, 0, 0, 0.2)`、左 13.51・上 15.02・右 8.41・下 13.48 単位外まで）→ `.intro__band--1` / `--2`（Image_48 / Image_49。4165×872 単位、上端 119.86 / 123.46 単位〈画面の中心 540 + Top 15.86 / 19.46 − 高さの半分〉。黒を塗り、`stage-band-1.webp` / `stage-band-2.webp`〈白〉をマスクにする）→ `.intro__icon`（Icon。470 単位四方、左上 (243.52, 279.48)〈画面の (0.2518, 0.4778) からオフセット (−239.91, −236.52)〉）の中に同じ大きさで重ねた `.intro__ring`（`stage-ring.webp` の α を黒で塗り、`::after` で同じ画像の輝度を `mask`〈`luminance`〉にして原作の赤 `rgb(192, 0, 0)` を重ねる。輪の赤 189〜192 は輝度の灰 251〜255 として残る）・`.intro__runes`（`stage-runes.webp`〈白〉をマスクにして `rgb(192, 0, 0)`）と、`.intro__logo`（ポーズ画面の頭 `pause-head.webp`。原作もこの頭は `pause_*_head` と同じテクスチャ）→ `.intro__title`（TitleCard）。`.intro__logo` も原作どおりアイコンと同じ 470 単位で描く。`pause-head.webp` は白の頭（13 記録。原作の頭と同じくテクスチャの中央に置き、大きさは館の Watchman と同じインク量）なので、`mask` にして原作の頭の赤 `rgb(192, 0, 0)` を塗る。あごひげの右下の角が中心から 118.4 単位で、ルーン（`stage-runes.webp` の半径 138〜164 px、126.7〜150.6 単位）の内縁の 8.3 単位内側に収まる。
- `.intro__title`: 原作の 901×180 の枠（画面の (0.36875, 0.4889) からオフセット (71.38, −116.53)、レンダー変換で (−72, 23) 動かした所）の中心 (1157.88, 524.47) に、ユーザーの題字を原作の文字の幅 898 単位（高さ 216.5 単位）で中心をそろえて置く。`stage-title.webp` の α を `mask` にし、色は `background-color`（既定 `rgb(196, 0, 0)`）。
- アニメで動く値（不透明度、ぼかし、`translate`、帯の `left`、`rotate`、`scale`、色）は stage-intro.ts が毎フレーム入れる。キャンバスの不透明度は `.intro__canvas` ではなく各層に掛ける（不透明度が 1 未満の祖先があると、`.intro__blur` の `backdrop-filter` が後ろのゲームを読めなくなる。ポーズ画面と同じ制約）。

### styles.css（シャード連続回収 = 原作の `UMG_ShardStreak`）
- `.streak`（`#hud` の先頭。`inset: 0`、`pointer-events: none`）。DOM の順は原作のスロットの順（画像 → 周辺減光 → extralife）。単位は `--u`（舞台の高さの 1/1080）。
- `.streak__card`: 612 × 227 単位を舞台の中心に（原作の StreakImage: アンカー中央、Alignment 0.5、ブラシの大きさで自動サイズ）。
- `.streak__vignette`: 舞台の上下 0.54・左右 0.96 単位内側までを紫 `rgb(130, 0, 166)`（Image_161 のブラシの色 linear (0.2248, 0, 0.3802)）で塗り、`/title/death-vignette.webp`（T_Vignette の α）でマスクする。
- `.streak__life`: 280.18 × 129.10 単位、左右中央で上端が中心から 115.46 下（原作の extralife: アンカー中央、Alignment (0.5, 0)）。`.streak__skull` は `/title/life.webp`（白）をマスクにして灰 `rgb(124, 124, 124)`（原作の Image_88 は Tint 無しで、テクスチャの灰がそのまま出る）を 90 単位で左 −8・上 20（アンカー (0, 0.5) から −44.55）、`.streak__text`「EXTRA LIFE !」は Helvetica Neue Bold 32 単位（24 pt）の白で左 80・上 44（原作の OutlineSize は既定の 0 で縁取りなし）。

### styles.css（特殊シャードの表示 = 原作の `UMG_VignetteSides`）
- `.vsides`（`#hud` の `#streak` の後。`inset: 0`、`pointer-events: none`）。単位は `--u`（舞台の高さの 1/1080）。
- `.vsides__canvas`（原作のルートの CanvasPanel_0: 画面いっぱい、`transform-origin` 中央。角度と拡大は JS）。DOM の順は原作のスロットの順（縁 → 文字）。
- `.vsides__vignette`: 舞台の上下 0.54・左右 0.96 単位内側まで（原作の Image_161: アンカー全面、オフセット 0.96 / 0.54）を JS の色で塗り、`/title/vignette-sides.webp`（T_VignetteNew の α）でマスクする。
- `.vsides__text`: 原作の TextBlock_47（アンカー下中央、左 −72.96・上 −233.08、151 × 40 単位、中央揃え）の箱に、Helvetica Neue Bold 46.67 単位（35 pt）、はみ出しを左右均等に（flex の中央寄せ）。縁取りは 1 単位の黒 α 0.638（原作の OutlineSettings）を 4 方向の `text-shadow` で。拡大の中心は箱の中央（原作の描画の軸 0.5, 0.5）。

### styles.css（ポーズ画面 = 原作の `UMG_Pause`）
原作（pak_reference の `UI/Menu/Pause/UMG_Pause`、v1.6.1）のウィジェット木を同じ単位（`--u`）で写したもの。ルートの CanvasPanel_0 が画面いっぱいで、各要素は原作と同じく画面の中央か上端から置く。
- 重なり順（DOM 順 = CanvasPanel_0 の Slots の順）: `.pause__wash`（Blur+Red）→ `.pause__main`（`.pause__stroke` = Image_152、`img.pause__head` = Icon、`#pause-easy` = TextBlock_1、`nav.pause__menu` = VerticalBox_113）→ `.pause__veil`（CanvasPanel_3）→ `#pause-quit-card`（Givingupbox）→ `#pause-restart-card`（RestartBox）。原作の Slots に ZOrder は無い。
- `.screen--pause`: `display: block`、`overflow: hidden`（背景は持たない。赤とぼかしは `.pause__wash`）。
- `.pause__wash` / `.pause__veil`（BackgroundBlur の強さ 8 と、linear (0.182, 0, 0) の 0.371 に染めた Image）: `inset 0`、`rgba(118,0,0,0.371)` + `backdrop-filter: blur(8u)`。`.pause__veil` は `opacity 0`、`pointer-events none` で、ポップアップの間だけ `.is-blocking` で `pointer-events auto`（原作の redblock が Visible / HitTestInvisible を切り替える）。原作のスロットは画面の縁から 10〜36u はみ出しているが、見た目は変わらないので `inset 0`。
- FadeIn の不透明度は `#pause` ではなく `.pause__wash` と `.pause__main` に掛ける（PauseFx）。`backdrop-filter` の要素の祖先の不透明度が 1 未満だと、その祖先が Backdrop Root になってゲームの画面をぼかせないため。原作は CanvasPanel_0 にまとめて掛ける（重なりのある部分で合成の仕方が少し違う）。
- `.pause__stroke`（Image_152。黒を塗り、`pause-stroke.webp`〈白〉をマスクにする）: 幅 912u、中央から左 20.56u（`left calc(50% − 476.56u)`）、縦は上端の 20.63u 上から下端の 38.92u 下まで引き伸ばす（`top −20.63u`、`height calc(100% + 59.55u)`。原作のスロットは縦にストレッチ、横は AutoSize）。
- `.pause__head`（Icon。`div`）: 原作どおり上端の中央に 900u 四方で、整列 (0.5, 0.2)（上端が画面の 180u 上。`left calc(50% − 450u); top −180u`）。13 記録の `pause-head.webp`（ユーザー提供のワサミの絵を白にして、原作の頭のテクスチャと同じくテクスチャの中央に置いた 1024²）を `mask` にし、原作の頭の赤 `rgb(192, 0, 0)`（`pause_*_head` の 1 色。2026-09-13 の Ballroom の収録でも頭は R 191 前後）を塗る。原作は GameMode.Level で画像を切り替える（館は Watchman）。頭は 144〜482u に収まり、EASY MODE の下端 81.7u とメニューの上端 540u のどちらからも離れる。
- `.pause__easy`（TextBlock_1）: 中央から上 515.46u が上端（`top calc(50% − 515.46u)`、`translate −50% 0`）、`min-width 358.48u`、中央揃え、`700 48u/1.1719 var(--roboto)`（UE の TextBlock の既定 Roboto Bold 36pt）、`rgb(192,0,0)`（linear 0.5255）。
- `.pause__menu`（VerticalBox_113）: 横は中央、上端が画面の中央（整列 (0.5, 0)）の縦並び。`.pause__item`（ボタン。VerticalBoxSlot の既定で箱の幅いっぱい）と `.pause__btn`（ポップアップのボタン）: `padding 2u`（押下は 3u 2u 1u。UE の既定のボタン）、`700 48u/1.1719 var(--roboto)`（36pt。`.pause__btn` は 37.33u = 28pt）、`#5f5f5f`（`Unhovered Color` linear 0.1146）、中央揃え、hover / focus-visible で白（原作のホバーは色だけで音は無い）。
- `.pause__card`（Givingupbox / RestartBox。子の最大 1222×928 の CanvasPanel）: 画面の中央に `1222u × 928u`、`translate −50% −50%`、既定 `opacity 0` と `scale 0`（ポップアップのアニメで中心から拡大）。`img.pause__frame` と `.pause__peek`（`div`）はカードいっぱい。`.pause__peek` は `pause-peek.webp`（白い絵を黒い地に載せたもの）の α を `mask` にして黒を塗り、`::after` で同じ画像の輝度を `mask`（`luminance`）にして `rgb(192, 0, 0)` を重ねる（黒い地の上に赤い絵）。RESTART? は枠（`pause-restart-frame.webp`、1222×532）を中央に（`top 198u; height 532u`）、頭を下へ 77.31u（`top 77.31u`。切り口が枠の赤線に来る）。
- `.pause__quit`（VerticalBox_0）: カードの中央から下 167.79u に中心（`translate −50% −50%`）の縦並び。`.pause__text`（quittext）: `700 40u/1.1719 var(--roboto)`（30pt）、白、中央揃え、下に 30u、`white-space: pre`（原作の文 `YOU WILL BE ABLE TO RESTART \r\nFROM LAST CHECKPOINT` の改行の前の空白を残す。UE はその空白ごと行を中央に置く）。ボタンは上に 15u。原作の QUIT TO DESKTOP を置かないので 3 行で、箱は中央寄せのまま詰まる（原作でそのボタンを Collapsed にしたときと同じ）。
- `.pause__choices`（HorizontalBox_0）: 上端がカードの中央から下 60.39u（`translate −50% 0`）。YES / NO はそれぞれ左右 35u・上 15u の余白。
- 見出し（`GIVING UP?` / `RESTART?`）は枠に描かれているので、`.pause__title` は `.confirm__title` と同じ規則で見えなくする。

## 依存関係
- tablet.ts → `../config`（CONFIG）、`./minimap`（`Minimap`, `MapMarker` 型）。画像 `public/tablet/teleportation.webp` / `speed_boost.webp`（ユーザー提供の 1000×1000 透過 webp、そのまま同梱）。
- hud.ts → `../config`。
- 使う側: `src/game/game.ts`（`Hud`/`Minimap`/`Tablet` を生成、毎フレーム `tablet.update`）。
- styles.css のタイトル画面は `index.html` の `#title` 以下の構造と `public/title/`（`logo.webp` / `wasami-face.webp` と、13 記録の `assets:title` が原作から作る `mask.webp` / `wisps.webp` / `marker.webp` / `restart-frame.webp` / `options-*.webp`）とフォント `public/fonts/Roboto-Bold.ttf` に対応する。CSS の `url()` は `/title/...`（フォントの `/fonts/...` と同じ書き方）。
- styles.css のポーズ画面は `index.html` の `#pause` 以下と `public/title/`（13 記録の `assets:title` の `pause-stroke.webp` / `pause-restart-frame.webp` / `pause-quit-frame.webp` と、`assets:pause` の `pause-head.webp` / `pause-peek.webp`）に対応する。
- title.ts → `../core/ue-curve.ts`（拡張子付き。Node のテストが解決できるように）、`../audio/audio`（型のみ）。使う側は `src/main.ts`（`wireMenu`・`wireOptions` と `boot`。01 記録）と pause.ts。音の id は 13 記録の `SFX`。
- pause.ts → `../core/ue-curve.ts`、`./title.ts`（`frames`、`ANIM.popup`、`SOUND.select` / `popup`。どちらも拡張子付き）、`../audio/audio`（型のみ）。使う側は `src/main.ts`（`wirePause` と `boot`）。
- options.ts → `../game/settings.ts`（拡張子付き。04 記録）。使う側は `src/main.ts`（`wireOptions`）。
- death.ts → `../core/ue-curve.ts`、`./title.ts`（`frames`、`ANIM.popup`、`SOUND.select` / `popup`。どちらも拡張子付き）、`../audio/audio`（型のみ）。音の id は 13 記録の `SFX`（`life_lost`、`game_over`、`ui_select`、`ui_popup`）と `public/voices`（`fine`、`over`）。styles.css の死亡画面は `index.html` の `#death` 以下と `public/title/`（13 記録の `assets:title` の `life.webp` / `you-are-dead.webp` / `death-vignette.webp` / `restart-frame.webp`）・`public/fonts/Roboto-Light.ttf` に対応する。使う側は `src/main.ts`（`wireDeath` と `onDeath`。01 記録）。
- level-clear.ts → `../core/ue-curve.ts`、`../game/results.ts`（`Counter`・`RANK_TEXT` と型）、`./stage.ts`（`unit`）、`./title.ts`（`frames`）（どれも拡張子付き）、`../audio/audio`（型のみ）。音の id は 13 記録の `SFX`（`escaped`、`grade_stamp_1`、`grade_stamp_2`、`xp_fill`）。styles.css の脱出の画面は `index.html` の `#end` 以下と `public/title/`（13 記録の `assets:title` の `you-escaped.webp` / `results-line.webp` / `stage-title.webp` / `death-vignette.webp`）・`public/fonts/Roboto-Bold.ttf` に対応する。使う側は `src/main.ts`（`onEnd` と NEXT。01 記録）。
- stage-intro.ts → `../core/ue-curve.ts`（拡張子付き）だけ。styles.css のステージ OP は `index.html` の `#intro` 以下と `public/title/`（13 記録の `assets:title` の `stage-band-1.webp` / `stage-band-2.webp` / `stage-ring.webp` / `stage-runes.webp` / `stage-title.webp` と、`assets:pause` の `pause-head.webp`）に対応する。使う側は `src/game/game.ts`（`startIntro` / `updateIntro`。04 記録）。
- Babylon.js: `TransformNode`, `Mesh`, `VertexData`, `PBRMaterial`, `DynamicTexture`, `ReflectionProbe`, `RenderTargetTexture`, `Texture`, `Vector3/Vector4`, `Color3`。

## 設定・調整値
- `hud.tablet.*`（position, rotationDeg, size, bezel, cornerRadius, screenRadius, edgeRadius, lowerDrop, stow.{hideTime,showRate,tilt}, metal.{albedo,roughness,environment}, face.{albedo,roughness,environment}, screen.{resolution,intensity,roughness,reflection}, reflectionProbe.{enabled,size,refreshRate}）。
- `hud.minimap.redrawHz`（30）、`hud.subtitleSeconds`（3.2）、`game.savingToastSeconds`（2.6）。
- 参照する他キー: `lights.shadowCasters`（3）、`player.boost.duration` / `cooldown`（6.75 / 8.5）と `player.teleport.cooldown`（5、`teleport.charge` の中で使う）。どれも game.ts が `setRefill` の `percent` を求めるのに使う。
- すべて URL クエリ `?hud.tablet.position=...` 形式で上書き可能（config.ts の仕組み）。タイトル画面の見た目は CSS 直書きで、CONFIG には無い。

## 既知の制約・注意点
- OPTIONS の配置は原作データ（ウィジェット木と Slate の規則）から組んだもので、原作の画面収録では確かめていない（手元の収録に OPTIONS が写っていない）。ボックスの大きさは字の行の高さと幅で決まるので、見出しと値の Helvetica Neue Bold がシステムに無い環境（Windows では Arial になる）では少しずれる。
- OPTIONS は SAVE & EXIT のときだけ保存・適用する（原作どおり）。Esc は CANCEL と同じ（原作には無い。RESTART? の Esc と揃えた）。スライダーの左右キーは 1/9 ずつ動かす（原作では効かない）。
- 旧 SETTINGS ダイアログの INPUT タブ（操作説明）と VIDEO タブ（API・解像度・F3 の案内）は、原作の OPTIONS に無いので無くなった。
- タブレット画面のフォントは `Roboto` / `Helvetica Neue` 系（Roboto はバンドルしていないので、多くの Mac では Helvetica Neue になる）で、Metal Mania は使っていない（唯一使っていた中央のバナーを消したので、フォントも外した）。タイトル画面とポーズ画面も Metal Mania を使わない（ロゴと RESTART? / GIVING UP? の見出しは画像、項目は `--menu-font` / `--roboto`）。
- rendering group 1 の深度クリアは Babylon 既定の自動クリアに依存しており、明示設定は無い。既定を変える変更が入ると壁への埋まりが再発しうる。
- `probe.renderList` は構築時のコピーで固定。後からレベルにメッシュを足しても映り込まない。
- `drawScreen` は表示中のみ実行される。回収の `Count Shake`（マップの閃きと数字の揺れ）は 0.1 s で終わるので、しまっている間の回収では描かれない（原作もタブレットが下りていれば見えない）。
- `tablet.setVisible(false)` はロード完了時（`minimap.capture` 直後）に呼ばれ、`start()` で `true` に戻る。それまではプリウォームのために表示状態で構築される。
- Space/Tab の `toggle()` はポーズ中は呼ばれない（game.ts の `active` 判定内）。
- 画面は 16:9 の舞台に固定し、窓の縦横比が違えば黒帯を出す。メニューは原作と同じく短辺基準の単位と辺・中央への固定で置くが、舞台がいつも 16:9 なので配置は窓の縦横比によらず 1920×1080 のときと同じ比になる（原作が 16:9 以外で見せる崩れ方、例えば 5:4 で煙のマスクが画面の大半を覆うことは起きない）。単位を JS で使うときは `innerWidth` / `innerHeight` ではなく stage.ts の `unit()` を使う。
- 顔のアップは 512px の写真を 1080p で 1100px に拡大しているのでぼやける。暗さと 2 つのマスクで目立たなくしている（原作にないフィルムグレインは外した）。
- `.title__wisps` のアニメは `background-position` なので、表示中はこの層（1080p で約 1655×1080）を毎フレーム描き直す（transform で動かすと 1 タイル分〈高さの 4.8 倍〉の幅の層が要るため）。タイトルが hidden の間は描かれない。
- **壁際のタブレット**：カプセル（半径 0.32 m + `keepDistance` 0.04 m）は目を壁から約 0.36 m までしか離さない。タブレットは目から 0.31 m 先・0.19 m 左にあり、壁際でも寄らない（原作どおり）ので、壁に斜めに寄ったり壁沿いに立ったりすると壁の裏側に入る（描画は group 1 の深度クリアで欠けない）。
- 以前は反射プローブをタブレットに付け、影も受けていたため、そこでプローブが館の外（黒）を映し、ランプの影判定でも影になり、金属枠と画面の映り込みが黒くなっていた（円形の間の壁際で確認）。プローブは目の位置から撮り、影は受けないようにしている。

## 変更履歴
- 2026-09-11: ローディング画面用の `.screen--loading` を追加し、`.screen--title` に表示時のフェードインを追加
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: 縁 R を 8mm→1.5mm、厚みを 30mm→14mm に変更（iPhone 12 以降風の角張った縁）。画面右下の残り時間（カウントダウン）表示と `setTimer` を削除。
- 2026-09-11: タイトル画面を原作のメインメニュー風に作り直した（左上のロゴ画像と赤いもや、灰色の筋、右側の顔のアップ、グレイン、左の縦メニューとホバーの赤い筆、左下の表記、右上の版番号、OPTIONS の SETTINGS ダイアログ）。`.title__inner` / `.title__kicker` / `.title__sub` / `.title__api` / `.title__actions` / `.controls` を削除
- 2026-09-11: タブレット上部の E スロット（`ICON_INTERACT`、`setInteractHot`、`drawSlot` の `hot`）を削除。シャードが触れて自動回収になり、吸い寄せのパワーも実装しないため。`Hud.setPrompt` は呼び出し元が無くなった
- 2026-09-11: 反射プローブをタブレットではなく目の位置から撮り、タブレットは影を受けないようにした（壁際でタブレットが壁の裏に入り、金属枠と映り込みが黒くなっていた）
- 2026-09-11: 参考画像（原作 Dark Deception のタブレット）に合わせ、本体を 0.16×0.21 m・角の半径 10mm・ベゼル 5mm にし、前面を別メッシュ `tabletFace`（黒いサテンの非金属）に分けた。縁と背面のガンメタルを暗くし（albedo 0.42→0.15）、画面ガラスの映り込みを 0.6→0.3 に弱め、画面テクスチャを 600×800 にした
- 2026-09-11: 画面の UI を参考画像に合わせて作り直した（実測の `LAYOUT`。上部はピル形のバーをやめ、左暗→右明の灰色の帯 + 両端の黒い円 2 つ〈左は空、右は Q ブースト〉、数字は Arial Black 900 → Roboto/Helvetica Neue 700、マップを灰色の枠で囲み、目的バンドを枠内の下端に横グラデで置き、末尾のピリオドを表示しない）。「Z」ラベルを削除
- 2026-09-11: 原作 v1.5.3 の RESTART? 確認ダイアログ（`.confirm-layer` / `.confirm*`、開閉アニメ）と、YES の後の赤い閃光と暗転（`.title__fade`、`title-leave`）を追加。`.title__version` を `z-index 1` にして暗転の上に残す
- 2026-09-11: 原作のプレイ動画に合わせてタブレットの動きを変更。視点移動による横揺れとロール（`lookDx`、`swayX`/`swayR`）を削除し、奥行き `depth` を追加（ダッシュで `sprintPush` だけ奥へ、物理レイで測った前方の壁に近いと `minDepth` まで手前へ）。静止位置を動画の実測で [-0.225,-0.035,0.474] → [-0.192,-0.035,0.49] にした（旧位置のまま壁際で拡大すると左端が画面外に出た）
- 2026-09-11: `POWER_ICONS` のコメントのキー表記を原作どおり（左 Q、右 E）に直した（描画は変わらない）
- 2026-09-11: 左の枠をテレポーテーションの状態表示にした（`setTeleport`: 使える = 赤、照準中 = 灰色、充填中 = 灰色の上に右端から赤が戻る。`POWER_ICONS` に `w`、`drawSlot` に `refill`）。発動の瞬間に画面が横にずれるグリッチ（`TabletFrame.glitch`、`drawGlitch`）を追加。原作の画面収録で確認した挙動
- 2026-09-11: 左右のパワー枠を参考画像（原作のタブレット）とユーザー提供のアイコンに合わせた。左の枠に Teleportation のコイル（表示だけ）、右の枠の SVG の稲妻 `ICON_BOOST` を Speed Boost の画像に置き換え（`POWER_ICONS`、`public/tablet/*.webp`、`svgImage` → `loadImage`）、枠のキー表示「Q」を削除
- 2026-09-11: スピードブーストの赤い円弧が効果切れの後も残る不具合を直した。以前は効果中は効果の残り、効果後はクールダウンの残りを同じ円弧で描いていた。円弧は効果中だけにし、クールダウン中はアイコンを暗くするだけにした（`setBoost(active, cooldown01)` → `setBoost(left01, cooling)`、`drawSlot` の `cooldown` → `timer`）
- 2026-09-11: クールダウン中は、効果中に 0% まで減った円弧が `player.boost.cooldown` の時間をかけて 100% まで戻るようにした（`setBoost(left01, cooling)` → `setBoost(state, arc01)`。`setTeleport` と同じ形）
- 2026-09-11: 円弧の表示をすべてのパワーに共通化した（ユーザーの指示）。`setBoost` / `setTeleport` を `setPower(slot, state, arc01)` と `PowerState` にまとめ、`drawSlot` は `{ state, arc }` だけで描く。テレポーテーションの充填表示（灰色のアイコンに右端から赤が戻る、`drawSlot` の `refill`、`POWER_ICONS` の `w`）を廃止し、位置調整中は輪が光って円弧 100%、発動後は暗いアイコンに円弧が 0%→100%（4.5 s）にした
- 2026-09-11: 敵に捕まったときの赤い閃光（`Hud.flash`、`#flash`、`.flash` / `catch-flash`）を追加
- 2026-09-11: 反射プローブの描画対象を目から `reflectionProbe.range`（14 m）以内に絞った（館の拡張で全体を 6 面描くのが重くなっていた）
- 2026-09-12: パワーの使用中にタブレットの表示を赤くする `TabletFrame.tint` を追加した（地図の線・枠・数字・目的の字・帯。赤が変わる間は毎フレーム描き直す）。原作の検証映像でテレポートの暗転の間に表示が赤くなるのに合わせた
- 2026-09-12: ブースト中のダッシュでさらに奥へ押す `TabletFrame.boostDash` と `reach.boostPush`（0.17 m、0.63 倍）を追加した。ブーストの効果中は画面が赤くなる（`tint`、`hud.tablet.tintTime` 0.5 s で出入り。04 記録）
- 2026-09-12: しまう・出すの動きを原作の検証映像に合わせた（`lower` の指数追従と中心を軸の 30° 前傾をやめ、`stow()`: しまうは 0.12 s の 2 次の加速で落ち下辺を軸に 55° 後ろへ倒れるのが先に進む、出すは毎秒 17 の指数で上がり倒れが遅れて戻る。`lowerDrop` 0.24→0.26、`hud.tablet.stow`）
- 2026-09-12: pak_reference の原作データに合わせ、テレポートの発動時のグリッチ（`TabletFrame.glitch`、`drawGlitch`）とテレポート中の表示の赤を削除した（原作のタブレットはシーンのティントで一緒に赤くなるだけ）。原作の「Use Left」/「Use Right」の枠の弾み（`pop(slot)`、`popScale`、`drawSlot` の `scale`）を追加した
- 2026-09-12: ユーザーの指示でテレポートの枠を原作どおりの表示にした（`setRefill(slot, percent)`、`PowerSlot.refill`、`POWER_ICONS` の `w`: 照準で灰色、移動の後 5 s で右端から色が戻る、輪の光と円弧なし）。枠の弾みを Q / E を押すたびにした（原作は使えるかの判定の前に弾む）
- 2026-09-12: ユーザーの指示で、両方のパワー枠を原作の円弧のマスク（`MM_Powers` の `VectorToRadialValue` と `Percent`）にした: 色つきのアイコンを枠の中心から 12 時を起点に時計回りの扇形で `percent` 周ぶんだけ見せる（以前のテレポートの枠は右端からの横の切り抜き）。スピードブーストの枠も原作どおり効果中に色が減りクールダウンで戻る表示にし、`setPower`・`PowerState`・円弧・輪の光・クールダウン中の暗さ・`POWER_ICONS` の `w` を削除した
- 2026-09-12: スピードブーストの間のタブレットの独自の赤（`TabletFrame.tint`、`drawnTint`、`FRAME_RED` / `TEXT_RED`、`shade` と `glow` の赤、`hud.tablet.tintTime`、minimap の `tint`）を削除した。原作ではブーストのカメラアニメのシーンのティントがタブレットにも掛かるだけなので、テレポートと同じく postfx のティントで赤くなる（09 記録）
- 2026-09-12: タイトル画面を原作のデータ（pak_reference の `UMG_TitleScreen` / `UMG_PopUp`、v1.6.1）に合わせて作り直した。配置を UE の単位（`--u` = 短辺 / 1080）と原作のアンカー・オフセットにし、原作の煙のマスク（`.title__mask`）・パンする灰色の筋（`.title__wisps`）・ホバーの赤い筆（`marker.webp`）・RESTART? の枠（`img.confirm__frame`、`.confirm__backdrop`）を使い、文字の色と大きさを原作の値にした（項目 30pt `#5c5c5c` など）。表示時の黒からのフェードイン、NEW GAME の赤い閃光と脈動と暗転、RESUME の暗転、ポップアップの開閉を原作のアニメの曲線で `title.ts` の `TitleFx` が再生し、BGM・選択音・ポップアップの音・`Start_New_Game`・ビアスの声を鳴らす。フィルムグレイン、`.title__glow`、`title-leave`、`confirm-*` の CSS アニメを削除
- 2026-09-12: タイトルの OPTIONS を原作の `UMG_Options`（v1.6.1）に倣って作り直した（旧 SETTINGS ダイアログの `.settings*` を削除）: ウィジェット木を同じ単位で写した `.options*`、Slate のグリッドでボックスを置く `options.ts`（`OptionsMenu`、`slateGrid`、`GRID`）と `tests/options.test.ts`、FadeIn / Cancel の開閉と SAVE & EXIT の音（title.ts の `openPopup(layer, sound)`、`closePopup(layer, hideAfter)`、`save()`、`ANIM.options`、`SOUND.save`。ポップアップの部品は `.popup-backdrop` / `.popup-card` で引く）。同梱の Roboto Bold（`DD Roboto`）を加え、RESTART? の YES / NO もそれで描くようにした
- 2026-09-12: 設定の SUBTITLES で声の字幕を出さないようにする `Hud.voiceSubtitles` を追加した
- 2026-09-12: FOV 補正の基準をカメラの歩きの FOV から、位置を測った FOV の定数 `PLACED_FOV` 75 に替えた（カメラの FOV が原作どおり 90〜115° になっても、タブレットの見かけは変わらない）
- 2026-09-12: 歩きの上下揺れ（`TabletFrame.bob`）をやめ、歩き・走りのカメラシェイクを物理の後に呼ぶ `unshake` が親の `hold` で打ち消して、タブレットがシェイクに付いていかない（画面上で逆に揺れる）ようにした
- 2026-09-12: ユーザーの指示でタブレットを揺らさないようにした: シェイクの打ち消し（`hold` / `unshake`）をやめ、`root` をカメラに直接付けた（カメラシェイクごと一緒に動き、画面上で動かない）
- 2026-09-12: タブレットの縮みを原作どおり FOV だけにした: ダッシュ・ブーストで奥へ押す `reach.sprintPush` / `boostPush`（`TabletFrame.dash` / `boostDash`）と FOV の打ち消し（`fovScale`、`TabletFrame.fovDegrees`）をやめ、歩きの FOV で測った位置に見せる定数 `PLACE_SCALE` にした（カメラの子のまま FOV で縮む）。原作に無いダッシュの沈み込み（`swayY`、`hud.tabletSway`、`swayMetersPerPx`）も削除した。ブーストの後の戻りが二段の遅れ（`boostDash` 6/s → `depth` 8/s）の S 字で出だしが遅かったのが、FOV の追従（5.0/s。05 記録）どおりになる（ユーザーの指摘: 解除後の戻りが本家より遅い）
- 2026-09-12: ポーズ画面を原作の `UMG_Pause`（v1.6.1）に倣って作り直した（旧 `.screen--pause` の黒い半透明と `PAUSED` の見出し・`.btn` のボタンを削除）: ウィジェット木を同じ単位で写した `.pause*`（赤いぼかし、黒い筆の帯、ワサミの頭、EASY MODE、RESUME / RESTART / OPTIONS / QUIT、RESTART? と GIVING UP? のカードとのぞく頭）、FadeIn・ポップアップのアニメと UI_Pause・Pause_Sound_v1・選択音・ポップアップの音を鳴らす `pause.ts` の `PauseFx` と `tests/pause.test.ts`。`--u` とフォントの変数を `:root` に移し、`.options-layer` を `position fixed`・`z-index 12` にしてポーズの上にも開けるようにした
- 2026-09-12: タイトルとポーズ画面の音をすべて UI 音（`ui: true`。06 記録）で鳴らすようにした（ポーズ中に止めるゲームの音とは別の AudioContext）
- 2026-09-13: 死亡画面・ゲームオーバー画面を原作の `UMG_DeathScreen`（v1.6.1）に倣って追加した: ウィジェット木を同じ単位で写した `.death*`（黒地、ライフのドクロの行、REMAINING LIVES、ヒント、YOU ARE DEAD、RESTART / LAST CHECKPOINT / QUIT TO TITLE、Fade In / Fade Out の黒、Shake の赤いビネット）と RESTART の確認、Construct の Delay とアニメと音をなぞる `death.ts` の `DeathFx` と `tests/death.test.ts`。ヒントの書体に Roboto Light（`DD Roboto` の 300）を加えた
- 2026-09-13: 死亡画面を結線した（main.ts）。`DeathFx` はボタンが出ても RESTART にフォーカスしない（原作どおり）、Fade Out の間は確認を閉じてもボタンを戻さない（`leaving`）。捕まったときの赤い閃光（`Hud.flash`、`#flash`、`.flash` / `catch-flash`）を削除した
- 2026-09-13: 画面を 16:9 に固定し、窓の縦横比が違えば黒帯を出すようにした: `.stage`（`--stage-w` / `--stage-h`、`contain: layout paint` で中の fixed の層の containing block にする）、`--u` を `100vmin / 1080` から舞台の高さ / 1080 に、`#scene` を舞台いっぱいに。単位の px を新しい `stage.ts` の `unit()` にまとめ、options.ts と death.ts の `min(innerWidth, innerHeight) / 1080` を置き換えた
- 2026-09-13: タブレットの配置を原作データ（`BP_DD_PlayerCharacter` の `Tablet` と `TabletInterp`、メッシュ `tablet_new_pCube2`）に合わせ、どの状態でも約 1.2 倍大きくした（ユーザーの指摘: 平常時・ダッシュ・ブーストで参照動画より小さい）。`position` を FOV 75° 換算の [-0.192, -0.035, 0.49] からカメラ空間の実寸 [-0.194, -0.0368, 0.3105] にし、`PLACED_FOV` / `PLACE_SCALE` をやめた。以前の実測はダッシュ中のフレームだったと考えられる。壁際で手前に寄る処理（`reach`、`depth`、`wallDepth()`）を削除した（原作にない。収録でも寄らない）
- 2026-09-13: ミニマップのズームの補間をやめたので、`drawScreen` と `minimap.draw` に `dt` を渡さなくなった（11 記録）
- 2026-09-13: 「Z」ラベルを原作どおりマップの左下に描くようにした（ユーザーの指示。原作の `TextBlock_107` の配置から `LAYOUT.zoomKey`、`KEY_FONT`。明るさは原作の収録の画面の灰に合わせて不透明度 0.38）。2026-09-11 に「原作の画面に無い」として消していたが、原作の画面にも収録にもある
- 2026-09-13: シャードを回収するとマップの左右の縁が紫に光るようにした（ユーザーの指示。原作の `Count Shake` の `Image_41`: `T_Vignette` を紫で 0.1 s。新しい `tablet-anim.ts` の `COUNT_SHAKE` / `collectFlash`、`LAYOUT.flash`、`FLASH_URL` と `tint()`、`setShards` の `flash`。明るさは本家の収録に合わせて `gain` 2.5）
- 2026-09-13: 本家に揃えた（ユーザーの指示）: シャード数の弾み（本作独自の `bump`: 0.4 s で 1.18 倍、色を `#e7a2ff` へ、`mix()`）をやめ、原作の `Count Shake` の数字の部分（`countShake`、`WIDGET`。0.1 s の揺れと 1.1 倍、色なし）にした。回収のときだけ揺れる（以前はリスタートで残りが戻るときも弾んだ）。地図の縮尺を原作のパネルで決めるため、`LAYOUT.flash` を `LAYOUT.panel` にして地図（`minimap.draw` の `panel`）と閃きで共用し、閃きの `gain` を `FLASH_GAIN` に分けた（11 記録）
- 2026-09-13: ステージ OP（原作のレベルのタイトルカード `UMG_ChapterPortal` と、それを出すレベル BP の Delay）を追加した: ウィジェット木を同じ単位で写した `.intro*`（ぼかし、赤い幕、流れる黒い筆の帯 2 本、回る赤い輪とルーンの輪とワサミの頭、ユーザーの題字「Stinky Gachimi」）と、`loop` のキーをゲームの時計で 1 フレームずつ描く `stage-intro.ts`（`TIMING`、`ANIM`、`introAt`、`IntroClock`、`StageIntro`）と `tests/stage-intro.test.ts`。この時点ではまだ game.ts から呼んでいない
- 2026-09-13: ワサミの頭がほかの要素に掛からないように、中点と大きさを直した（ユーザーの指示）: ステージ OP の輪の `.intro__logo` は、頭の最小包含円をアイコンの中心に合わせて半径 110.2 単位にした（アイコンいっぱいの 470 単位のままだと、あごひげ・髪・頬がルーンに掛かっていた）。ポーズ画面の `.pause__head` は 900 → 830u 四方、上端 −180 → −160u（あごひげが RESUME に掛かっていた）。RESTART? / GIVING UP? ののぞく頭は掛かっていないので変えていない
- 2026-09-13: 原作の Hotel のエレベーターの乗車の暗転のため、`Hud.fade(opacity, seconds)` と `.fade`（`#fade`）を足した
- 2026-09-13: 原作のシャード連続回収の表示 `ShardStreakFx`（streak.ts、原作の `UMG_ShardStreak`）と `.streak` の CSS、`tests/streak.test.ts` を足した（ユーザーの指示: 「あと n 個」をやめ、本家の「Not Bad」のような表示に）
- 2026-09-13: 原作の特殊シャードの表示 `VignetteSidesFx`（vignette-sides.ts、原作の `UMG_VignetteSides`）と `.vsides` の CSS、`tests/vignette-sides.test.ts` を足した
- 2026-09-13: `TabletFrame` に地図の矢印 `arrow`（`MapArrow | null`）を足し、`minimap.draw` に渡すようにした（11 記録）
- 2026-09-14: 脱出の画面を原作の `UMG_LevelClear` に倣って作り直した（ユーザーの指示）: `LevelClearFx`（level-clear.ts）と `.screen--clear` / `.clear__*` の CSS、`tests/level-clear.test.ts` を足し、以前の終了画面（`.screen--end`、`.pause__inner`、`.btn`、見出し `YOU ESCAPED` と `もう一度`）を削除した
- 2026-09-14: ユーザーの指示で本家に無い表示を外した: `Hud` の `setPrompt` / `showBanner` と `#prompt` / `#banner`、`.prompt` / `.banner` の CSS、バナーだけが使っていた `@font-face 'Metal Mania'` とフォントのファイル
- 2026-09-14: `Tablet.screenRect`（ダッシュのラジアルブラーのマスクだけに使っていた）を削除した（09 記録）
- 2026-09-14: 画面の中央の手のマーク（原作の `UMG_Interact`）: `Hud.setInteract`、`#interact`、`.interact` の CSS を足した
- 2026-09-14: ロックピックのウィジェット（ファンゲームの `Lockpicking`）: `Hud.setLockpick` / `lockpickPress`、`#lockpick`、`.lockpick` の CSS（4 つのリング・進み具合・F・押した時の揺れ）を足した
- 2026-09-14: タブレットのシャード数のフォントを原作の `ShardCount` の helvetica-neue-bold に寄せた（ユーザーの指示）: `COUNT_FONT`（Roboto が先頭で、Roboto の入った環境では数字が Roboto になっていた）と `KEY_FONT` を `HELVETICA_BOLD` にまとめた
- 2026-09-14: ユーザーの指示で原作の UI の書体を同梱して使うようにした: `@font-face 'DD Helvetica Neue'` / `'DD Helvetica'`（13 記録が書き出す原作の helvetica-neue-bold / helvetica-normal）、`--options-font` / `--menu-font` の先頭をそれに。原作と食い違っていた `.collectable__text`（原作は helvetica-neue-bold なのに `--menu-font`）を `--options-font`、`.saving`（原作は RobotoTiny の Light）を `--roboto` の 300、`.subtitles`（原作の DialogueBox は helvetica-normal）を 'DD Helvetica' 先頭にした。タブレットは `HELVETICA_BOLD` と目的バンドの `HUD_FONT`（以前は Roboto 先頭）を 'DD Helvetica Neue' 先頭にし、`CAP` を 0.714 に、読み込み後に描き直す
- 2026-09-14: ワサミの頭を白いアセットにして CSS で着色するようにし、大きさと中心を直した（ユーザーの指示: 「ややサイズが大きく、中心点がズレており、着色もゲーム内よりも鮮やかな赤」「アセットは白塗りにしてゲームエンジン内で赤へ着色する」）: `.pause__head` / `.pause__peek` を `img` から `div` にし、`pause-head.webp` / `pause-peek.webp` のマスクに原作の頭の赤 `rgb(192, 0, 0)` を塗る（これまでは絵の (255, 0, 0) のまま）。頭の大きさと置き方は 13 記録の `HEAD` に移し、`.pause__head` は原作どおり 900u・上端 −180u、`.intro__logo` はアイコンと同じ 470 単位に戻した（2026-09-13 の 830u・421.9 単位の調整をやめた）
- 2026-09-14: 同じ指示（「同アプローチが効くアセット全て」）で、13 記録が白で書き出すようにした 1 色の素材を、CSS のマスクに原作のテクスチャの色を塗る形にした: ステージ OP の帯（黒）・輪（α を黒、`::after` で輝度を `rgb(192, 0, 0)`）・ルーン（192 の赤）、タイトルの煙（黒）・筋（灰 179。筋と煙の 2 枚のマスクを `intersect` し、アニメは `mask-position`）・メニューのホバーの筆（`::before` に (99, 8, 0)）、OPTIONS の矢印（1 枚のマスクを赤と白に塗り分け）、ポーズの縞（黒）、Collectables のカード（黒）とアイコン（`--icon` と `rgb(193, 0, 1)` の乗算。`img` → `div`）、連続回収のドクロ（灰 124）、死亡画面のビネット（166 の赤）。脱出の画面の題字 `.clear__level` の `#f00` は、原作の LevelName（Tint (1, 0, 0) × 館のタイトルカードの灰 196）の `rgb(196, 0, 0)` に直した
- 2026-09-14: Hotel の削除で、秘密を取ったときの表示 `collectables.ts`（原作の `UMG_Collectables`）と index.html の `#collectable`、styles.css の `.collectable*` を消した（ステージに秘密は無い）
- 2026-09-15: パンチの溜めのウィジェット（ファンゲームの `Aim`）の `Hud.setAim`・`#aim`・`.aim` の CSS と、駅の床の案内の文字の書体 `@font-face 'CC2 Beer Money'`（`beer_money.ttf`）を足した
- 2026-09-15: 本作の規範とユーザーの選択で、パンチと駅の床の案内の文字をやめたので、`Hud.setAim`・`#aim`・`.aim` の CSS と `@font-face 'CC2 Beer Money'`（`public/fonts/beer_money.ttf` も）を削除した。壊せる物は原作どおり視線の手のマーク `#interact` で示す（04・07 記録）
- 2026-09-15: 本作の規範で、ロックピックの障壁を原作の板張りのバリケードのように手のマーク `#interact` と 1 クリックで壊すようにしたので、ファンゲームの `Lockpicking` のウィジェットの `Hud.setLockpick`・`lockpickPress`、index.html の `#lockpick`、styles.css の `.lockpick*`（4 つのリング・F・`lockpick-wobble`）を削除した（04 記録）
