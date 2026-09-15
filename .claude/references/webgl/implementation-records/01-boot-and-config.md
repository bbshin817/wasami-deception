---
title: 起動シーケンスと設定（main / config / overrides / ビルド設定）
sources:
  - src/main.ts
  - src/config.ts
  - src/core/overrides.ts
  - index.html
  - vite.config.ts
  - tsconfig.json
  - package.json
  - scripts/check-pages-limits.mjs
  - tests/pages-limits.test.ts
updated: 2026-09-15
---

# 起動シーケンスと設定（main / config / overrides / ビルド設定）

## 役割
初期バンドルを最小（ローディング画面 + タイトル画面）に保ち、Babylon.js とレベル本体を動的 import で遅延ロードする起動処理。
初回ロードは「進捗バー + ラベルだけのローディング画面（`#loading`）」→ 読み込み完了後に「タイトル画面（`#title`）」の 2 段構成。
タイトル画面は Dark Deception のタイトルのウィジェット（pak_reference の `UMG_TitleScreen` / `UMG_PopUp`）の配置・アニメ・音に倣っている（見た目は 10-hud-tablet の styles.css 節、動きと音は同記録の title.ts 節）。
全チューニング値を `CONFIG` 一箇所に集約し、URL クエリで葉の値を上書きできるようにする。
Vite / TypeScript / npm scripts のビルド設定もここに記録する。

## 公開インターフェース
- `src/config.ts`
  - `export const CONFIG` : 全設定のネストしたプレーンオブジェクト（凍結はしていない、実行時に書き換え可）。
  - `export type Config = typeof CONFIG`。
- `src/core/overrides.ts`
  - `applyOverrides(target: Record<string, unknown>, params: URLSearchParams): string[]` : `a.b.c=value` 形式のクエリを `target` に破壊的に適用し、適用した `key=raw` 文字列の配列を返す。
- `src/main.ts` : export なし。モジュール評価時に `boot()` を実行する副作用モジュール。
- `index.html` : `<script type="module" src="/src/main.ts">` が唯一のエントリ。`lang="ja"`、favicon `./wasami.webp`、title `WASAMI DECEPTION — Vertical Slice`。タイトル画面の画像 `./title/logo.webp`（ロゴ、1942x809）と `./title/wasami-face.webp`（右側の顔のアップ、512x512。`legacy/enemies/WA_0010.webp` と同じ画像）、RESTART? の枠 `./title/restart-frame.webp`（原作の `restart_window_frame_2`。13 記録）、OPTIONS の枠 `./title/options-frame.webp`（原作の `options_window_frame`）、GIVING UP? / RESTART? の枠 `./title/pause-quit-frame.webp` / `./title/pause-restart-frame.webp`（原作の `quit_window_frame` / `restart_window_frame`）を `<img>` で持つ。ポーズ画面の頭とカードからのぞく頭は、白い画像（`pause-head.webp` / `pause-peek.webp`）を CSS のマスクにして色を塗る `div`（10 記録）。`<body>` の中身（キャンバスと `#hud` 〜 `#fatal` のすべての層）は 16:9 の舞台 `#stage` に入れる（黒帯の出し方は 10 記録の styles.css 節）。

## 内部構造と処理の流れ

### 起動シーケンス（main.ts の `boot`）
1. `main.ts` 評価直後（他モジュールより先）に `applyOverrides(CONFIG, new URLSearchParams(location.search))` を実行。以降に動的 import されるモジュールはすべて上書き済みの `CONFIG` を見る。
2. `$()`（`document.getElementById` の薄いラッパ）で `scene` / `loading` / `title` / `pause` / `end` / `death` / `progress-fill` / `progress-label` / `btn-start` / `btn-reset` / `options` / `restart` を取得。モジュール評価時に `#title-version` を `v${version}`（`package.json` の `version` を named import。現在 `v0.2.0`）にし、`document.fonts.load("700 32px 'DD Roboto'")` で OPTIONS のラベルのフォントを先に読み込む（ロード画面の裏で。OPTIONS を開いたときの計測に間に合わせる）。
3. `progress(fraction, text)` : `#progress-fill` の `width` を `(fraction*100).toFixed(1)%` に、`#progress-label` の `textContent` を `text` にする。
4. `boot()`:
   - タイトルメニューの結線は Game の生成後に行う（音を鳴らす `TitleFx` が `game.audio` を使うため。タイトルはそれまで出さない）。
   - `progress(0, 'ENGINE を読み込んでいます…')`。
   - `await import('./game/game')` で `Game` を遅延ロード（Babylon 本体・Havok・レベルコードはこのチャンクに入る）。
   - `Game.create(canvas, hooks)` を呼ぶ。hooks は `onProgress` = `progress`、`onStarted`（`#title` を隠し、`fx?.stopMusic()` でタイトルの BGM を止める。デバッグ API から始めたときも）、`onPauseChange(paused)`（ポーズなら `syncEasy()` → `pause.show()`、再開で `#pause` が出ていれば `pause.hide()`。下記「ポーズ画面の結線」）、`onEnd(results)`（下記）、`onDeath(lives)`（下記「死亡画面の結線」）。
   - `Game.create` 内の進捗マイルストーン（呼び出し側から観測できる値）: 0.01 GPU 初期化 → 0.04 `${api} で起動しました` → `0.05 + 0.83 * (loaded/total)` でアセットストリーミング（`LOADING x.x / y.y MB — ファイル名`）→ 0.89 Havok → 0.90 館の組み立て（`debug.field` のときはデバッグフィールドの組み立て）→ 0.93 サウンドデコード → 0.96 シェーダーコンパイル → 1.0 `READY`。
   - 生成後: `wireOptions` に渡す `prefs` を `{ get: () => game.settings, save: (s) => { game.saveSettings(s); syncEasy(); } }` にする（04 記録）。
   - `hasSave = game.hasSave && !CONFIG.debug.freshSave`。原作と同じく、セーブありなら `#btn-start` = `RESUME`（セーブから続行）と `#btn-reset` = `NEW GAME`（RESTART? の確認の後、セーブを消してその場で最初から）を並べ、セーブなしなら `#btn-start` = `NEW GAME` だけ（`#btn-reset` は hidden）。
   - `fx = new TitleFx(title, game.audio)` と `pause = new PauseFx(pauseScreen, game.audio)`（10 記録）を作り、`openOptions = wireOptions(fx, prefs)`、`wireMenu(fx, hasSave, start, openOptions)`、`wirePause(pause, game, openOptions)` で結線し、`death = new DeathFx(#death, game.audio)`（10 記録）を作って `wireDeath(death, game)` で結線し、`clear = new LevelClearFx(#end, game.audio)`（10 記録）を作って NEXT（`#clear-next`）を結線した後、`#loading` を隠す。`CONFIG.debug.autostart` が真ならタイトルを出さずに即 `start()`、偽なら `#title` を表示して `fx.reveal()`（黒の覆いが 2.5 s で消え、BGM が 2 s でフェードイン。10 記録）。
5. `onEnd(results)`（ポータルの脱出。原作の `UMG_LevelClear`）: `#pause` が出ていれば `pause.hide()`、`clear.open(results, game.settings.difficulty === 0)`（EASY なら EASY MODE を出す）。画面が UI 入力に切り替わったら（開いてから 7 s）`game.input.exitLock()`（原作はここでマウスを出す）。NEXT（`#clear-next`）は `clear.leave()`（Fade Out と Delay 4.0）の後に `location.reload()`（原作の Replay Mode の道はタイトルを開く。セーブはポータルで消してある）。
   `onDeath(lives)`（捕獲の 3.5 s 後）: `#pause` が出ていれば `pause.hide()`、`death.open(lives)`。ライフが残っていれば、それが済んだら（リスポーンの時）`death.hide()` → `game.respawn()`。0 なら、ボタンが押せるようになったら `game.input.exitLock()`（原作はここでマウスを出す）。
6. 失敗時は `boot().catch(fatal)` : `console.error` の後、`#fatal` を表示して `起動に失敗しました:\n<message>\n<stack>` を書き込む。

### OPTIONS の結線（main.ts の `wireOptions(fx, prefs)` → `openOptions(from)`）
- タイトルのメニューとポーズ画面の両方が、自分のクリック音の後に `openOptions(from)` で開く（原作もタイトルとポーズの両方から `UMG_Options` を作る。ポーズからは ZOrder 10 で、ポーズの 5 の上）。`#options` は `#title` の外にあり、どちらの上にも出る（10 記録）。開閉のアニメと音は `TitleFx` の `openPopup` / `closePopup`。
- `interface Prefs { get(): Readonly<Settings>; save(settings): void }`: OPTIONS に出す設定と、SAVE & EXIT がそれをどうするか。`type OpenOptions = (from: HTMLElement) => void`。
- `menu = new OptionsMenu(options)`（原作の `UMG_Options`。10 記録の options.ts）。`openOptions(from)`: 閉じた後のフォーカス先に `from` を覚え、`fx.openPopup(options, false)`（FadeIn。開く音は無い）→ `menu.show(prefs.get())`（原作の Setup Values）→ `#options-cancel` にフォーカス。
- `close()`（wireOptions 内）: 閉じる途中（`options.inert`）なら何もしない。`options.inert = true` にして `fx.closePopup(options, ANIM.options.hideAfter)`（再生速度 0.7 の選択音と FadeIn の逆再生、0.3 s 後に hidden）の後、`inert` を戻して `from` にフォーカス。`#options-cancel` の click と、OPTIONS が開いている間の Esc（`document` の `keydown`）に結線（下書きは捨てる）。
- `#options-save` の click: 閉じる途中でなければ `fx.save()`（UI_Select_V2）→ `prefs.save(menu.value)`（保存と適用。原作の SAVE & EXIT は適用 → Save Values → Cancel の順）→ `close()`。

### タイトルメニューの結線（main.ts の `wireMenu(fx, hasSave, start, openOptions)`）
- `leave(kind, fresh)`（wireMenu 内）: `#title` を `inert` にし、`fx.leave(kind)`（原作の暗転のアニメと音、3.75 s。10 記録）が終わってから `start(fresh)`。原作はこの後 Delay（NEW GAME 10 s / 続行 5 s）してレベルを開くが、本作はアニメの終わりにその場で始める（下記「既知の制約」）。
- `cancelRestart()`（wireMenu 内）: `fx.closePopup(restart)`（再生速度 0.7 の選択音と Popup の逆再生、0.25 s）の後 `#btn-reset` にフォーカス。`#restart-no` の click と Esc に結線。
- `#btn-options` の click: `fx.click()`（原作のタイトルが Options を作るときの UI_Select_V3）→ `openOptions(#btn-options)`。
- `document` の `keydown`（`#title` が hidden か `inert`（YES の後の暗転中）、または OPTIONS が開いている間は何もしない）:
  - RESTART? が開いている: `Escape` で `cancelRestart()`、`ArrowLeft` / `ArrowRight` で YES と NO の間でフォーカスを移す（`preventDefault`）。ほかのキーは無視。
  - それ以外: `ArrowUp` / `ArrowDown` で表示中の `.menu__item` の間をフォーカス移動（端で折り返し、フォーカスが無ければ先頭）。`preventDefault` する。決定は `<button>` の標準動作（Enter / Space）。

### ポーズ画面の結線（main.ts の `wirePause(pause, game, openOptions)`）
原作の `UMG_Pause` の処理に倣う（アニメと音は 10 記録の `PauseFx`）。
- `onPauseChange(true)`（`game.pause()`。ポインタロックの喪失と `window` の `blur`）: `syncEasy()`（`#pause-easy` を難易度が EASY（0）のときだけ出す。原作は TextBlock_1 の色を難易度に結び付けていて、EASY で赤、NORMAL で透明。HARD はメニューから選べない）→ `pause.show()`。SAVE & EXIT の後も `syncEasy()`。
- `#pause-resume`: `pause.resume()`（選択音と FadeIn の逆再生。原作どおり 0.5 s の Delay の後にウィジェットを消す）が終わってから `game.resume()`（ポインタロックはクリックのユーザー操作の有効期間内に取れる）。
- `#pause-restart`: `pause.openPopup(#pause-restart-card)`（Popup_0。選択音とウィンドウの音）→ `#pause-no` にフォーカス。`#pause-no`: `pause.closePopup(card)`。
- `#pause-yes`: `pause.click()` → `pause.hide()` → `game.restart()`（最後のセーブへ戻る。04 記録）。
- `#pause-options`: `pause.click()` → `openOptions(#pause-options)`。
- `#pause-quit`: `pause.openPopup(#pause-quit-card)`（Popup）→ `#pause-cancel` にフォーカス。`#pause-cancel`: `pause.closePopup(card)`。
- `#pause-to-title`: `pause.click()` → `location.reload()`（原作は TitleScreen を開く。セーブは残り、GIVING UP? の文言どおり最後のセーブより後の進み具合は失われる）。原作の QUIT TO DESKTOP はブラウザでは終了できないので置かない（ユーザーの指定）。
- `document` の `keydown`: `#pause` が出ていて `inert` でなく、OPTIONS が閉じているとき、`Escape` で開いているポップアップを閉じる（NO / CANCEL と同じ）。

### 死亡画面の結線（main.ts の `wireDeath(death, game)`）
原作の `UMG_DeathScreen` のライフ 0 の処理に倣う（アニメと音は 10 記録の `DeathFx`）。
- `[data-death="restart"]`: `death.openPopup(#death-restart)`（`ui_popup` と Popup）→ `#death-no` にフォーカス。`#death-no`: `death.closePopup(card)`（再生速度 0.7 の選択音）の後 RESTART にフォーカス。
- `#death-yes`: `death.click()` → `death.closePopup(card, false)`（原作の Close Animation は無音）と同時に `death.leave()`（Fade Out と Delay 1.0）→ `death.hide()` → `game.newRun()`（館を最初から。04 記録）。
- `[data-death="checkpoint"]`: `death.leave()` → `death.hide()` → `game.restart(true)`（セーブへ戻り、ライフは満タン。`true` は原作の GameInstance の `Used Hard Respawn?` で、脱出の画面の LIVES LOST がセーブのライフ数になる）。ただし `game.checkpointWarning.get()` が偽の間（原作のセーブの `Last Checkpoint Warning`。04 記録）は先に `death.click()`（原作の PlaySound2D(UI_Select_V3, 1.0)）→ `death.openPopup(#death-warning)`（原作の `UMG_PopUp` を Frame 2 = 題字の無い枠で、文言 `Obtaining S Rank is not possible with Last Checkpoint.` / 空行 / `Continue anyway?`）→ `#death-warning-no` にフォーカス。
- `#death-warning-no`: `death.closePopup(warning)` の後 LAST CHECKPOINT にフォーカス（何度でも聞く）。`#death-warning-yes`: `death.click()` → `death.closePopup(warning, false)` → `game.checkpointWarning.set(true)`（原作は YES で真にしてセーブする。以後の LAST CHECKPOINT は聞かない）→ 上の LAST CHECKPOINT の処理（Fade Out → `restart(true)`）。
- `[data-death="quit"]`: `death.leave()` → `location.reload()`（原作は TitleScreen を開く）。
- `document` の `keydown`: 死亡画面が出ていて RESTART の確認か LAST CHECKPOINT の警告が開いているとき、`Escape` でその NO と同じ。

### ボタン / イベント配線（main.ts）
| 要素 | イベント | 動作 |
|---|---|---|
| `#btn-start` | click | セーブあり（RESUME）: `fx.click()` → `leave('resume', false)`（黒へ 3.5 s の暗転。BGM は 4 s で消える）→ `game.start()`（セーブを反映して続きから）。セーブなし（NEW GAME）: 選択音なしで `leave('newGame', true)`（原作もセーブがなければ確認せずにすぐ暗転する。新しいゲームなので `start(true)` で LAST CHECKPOINT の警告のフラグも戻す） |
| `#btn-reset` | click | `fx.click()` → `fx.openPopup(restart)`（開く音と Popup のアニメ）→ `#restart-no` にフォーカス |
| `#restart-no` | click | `cancelRestart()` |
| `#restart-yes` | click | `fx.click()`、`fx.closePopup(restart)`（原作の New Game はポップアップの Press No を呼ぶので、閉じる音も重なる）、`leave('newGame', true)`（赤い閃光と画面の脈動、`Start_New_Game` とビアスの声、黒へ暗転）→ `game.start(true)`（セーブを消し、リロードせずに最初から） |
| `#btn-options` | click | `fx.click()` → `openOptions(#btn-options)` |
| `#options-save` | click | `fx.save()` → `prefs.save(menu.value)` → `close()` |
| `#options-cancel` | click | `close()` |
| `#pause-resume` | click | `pause.resume()` → `game.resume()` |
| `#pause-restart` / `#pause-no` / `#pause-yes` | click | RESTART? を開く / 閉じる / `pause.hide()` → `game.restart()` |
| `#pause-options` | click | `pause.click()` → `openOptions(#pause-options)` |
| `#pause-quit` / `#pause-cancel` / `#pause-to-title` | click | GIVING UP? を開く / 閉じる / `location.reload()` |
| `#clear-next` | click | `clear.leave()`（Fade Out と Delay 4.0）の後 `location.reload()`（脱出の画面が UI 入力に切り替わるまで `inert`） |
| `[data-death="restart"]` / `#death-no` / `#death-yes` | click | RESTART の確認を開く / 閉じる / Fade Out の後 `game.newRun()` |
| `[data-death="checkpoint"]` / `[data-death="quit"]` | click | Fade Out の後 `game.restart(true)` / `location.reload()` |
| `#scene`（canvas）と舞台の外の黒帯（`document` の click のうち、対象が canvas・`body`・`html` のもの） | click | `game.playing && !game.paused && !game.input.locked` のとき `game.input.requestLock()`（Chrome は Esc 後 ~1 秒再ロックを拒否するため） |

### index.html の DOM id と利用者
| id | 要素 | 利用者 |
|---|---|---|
| `stage` | `<div class="stage">`（キャンバスと全画面の層を包む 16:9 の舞台） | styles.css の `.stage`、`src/hud/stage.ts` の `unit()` |
| `scene` | `<canvas tabindex="0">` | main.ts、`Game.create`、`Input`（マウス / Pointer Lock 対象） |
| `hud` | `<div class="hud" hidden>` | `src/hud/hud.ts` の `root` |
| `streak` | `#collectable` の後の `<div class="streak" hidden>`（シャード連続回収の節目。原作の `UMG_ShardStreak` のスロット順に `.streak__card`（`<img>`）、`.streak__vignette`、`.streak__life`（`.streak__skull` と `.streak__text`「EXTRA LIFE !」）） | `src/hud/streak.ts` の `ShardStreakFx`（game.ts の `streakView`。10 記録） |
| `vsides` | `#streak` の後の `<div class="vsides" hidden>`（特殊シャードの ENEMIES STUNNED / ENEMIES REVEALED。原作の `UMG_VignetteSides`。取るたびに `.vsides__canvas` を中に足す） | `src/hud/vignette-sides.ts` の `VignetteSidesFx`（game.ts の `vsidesView`。10 記録） |
| `subtitles` | `aria-live="polite"` の字幕 | hud.ts |
| `saving` | `SAVING PROGRESS` トースト | hud.ts |
| `interact` | 画面の中央の手のマーク（原作の `UMG_Interact`） | hud.ts |
| `intro` | ステージ OP の層 `<div class="intro" hidden aria-hidden="true">`（原作の `UMG_ChapterPortal` のウィジェット木の順。10 記録）。子に `.intro__black`（レベルが開くときの黒）と `.intro__canvas`（`.intro__blur`、`.intro__wash`、`.intro__band--1` / `--2`、`.intro__icon` の中の `.intro__ring` / `.intro__runes` / `.intro__logo`、`.intro__title`） | `src/hud/stage-intro.ts` の `StageIntro` |
| `stats` | `<pre class="stats" hidden>` | `src/game/game.ts` の `statsEl`（F3 オーバーレイ、03 記録参照） |
| `fade` | `<div class="fade">`（`#hud` の後、舞台の直下） | `src/hud/hud.ts` の `fade`（配電盤の後の脱出の開始の暗転。04・10 記録） |
| `loading` | ローディング画面 `<section class="screen screen--loading">`（初期表示、進捗バーとラベルだけ） | main.ts |
| `progress`, `progress-fill`, `progress-label` | `#loading` 内のプログレスバー（ラベル初期値 `ENGINE を準備しています…`） | main.ts `progress()` |
| `title` | タイトル画面 `<section hidden>`（読み込み完了後に表示）。子に `img.title__monster` / `.title__mask` / `.title__wisps` / `h1.title__logo > img` / `.title__copyright` / `nav.menu` / `.title__fade` / `.title__cover` / `.title__flash` / `#title-version` / `#restart`（原作のウィジェット木の順） | main.ts |
| `btn-start`, `btn-reset`, `btn-options` | `nav.menu` 内の `.menu__item`（HTML の初期ラベルは `NEW GAME` / `NEW GAME`（hidden）/ `OPTIONS`） | main.ts |
| `title-version` | 右上のバージョン表記 `v0.2.0` | main.ts |
| `options`, `options-title`, `options-save`, `options-cancel` | OPTIONS の層 `div.options-layer`（`#pause` の後、`#end` の前。タイトルとポーズの両方から開くので `#title` の外。初期 `hidden`。子に赤い幕とぼかしの `.options__backdrop.popup-backdrop` と、`role="dialog"` のカード `.options.popup-card`: 枠の画像 `img.options__frame`、見えない見出し `OPTIONS`、`.options__grid` の 4 つの `.options__box[data-box]`（`graphics` / `audio` / `controls` / `difficulty`）、ボタン `SAVE & EXIT` / `CANCEL`）。行の操作は `button[data-arrow][data-delta]`、`[data-slider]`（`role="slider"`）、`button[data-check]`（`role="checkbox"`）、値の欄は `[data-text]` | main.ts `wireOptions`、`src/hud/options.ts` |
| `restart`, `restart-title`, `restart-text`, `restart-yes`, `restart-no` | RESTART? の層 `.confirm-layer`（初期 `hidden`。子に赤い幕とぼかしの `.confirm__backdrop`）と `role="alertdialog"` の `.confirm`（枠の画像 `img.confirm__frame`、見えない見出し `RESTART?`、本文 `STARTING A NEW GAME WILL RESET ALL PROGRESS.`、ボタン `YES` / `NO`） | main.ts |
| `pause` | ポーズ画面 `<section class="screen screen--pause" hidden>`（原作の `UMG_Pause` のウィジェット木の順）。子に `.pause__wash`（Blur+Red）、`.pause__main`（`.pause__stroke`、`.pause__head`、`#pause-easy`、`nav.pause__menu`）、`.pause__veil`（CanvasPanel_3）、`#pause-quit-card`、`#pause-restart-card` | main.ts、`src/hud/pause.ts` |
| `pause-easy` | `EASY MODE`（初期 `hidden`） | main.ts `syncEasy()` |
| `pause-resume`, `pause-restart`, `pause-options`, `pause-quit` | `nav.pause__menu` の `.pause__item`（RESUME / RESTART / OPTIONS / QUIT） | main.ts `wirePause` |
| `pause-quit-card`, `pause-quit-title`, `pause-quit-text`, `pause-to-title`, `pause-cancel` | GIVING UP? の `role="alertdialog"` のカード `.pause__card--quit`（初期 `hidden`。枠の画像 `img.pause__frame`、のぞく頭 `.pause__peek`、見えない見出し、本文 `YOU WILL BE ABLE TO RESTART ␣/FROM LAST CHECKPOINT`（改行の前の空白も原作どおり）、ボタン `QUIT TO TITLE` / `CANCEL`） | main.ts `wirePause` |
| `pause-restart-card`, `pause-restart-title`, `pause-yes`, `pause-no` | RESTART? のカード `.pause__card--restart`（初期 `hidden`。枠、のぞく頭、見えない見出し、ボタン `YES` / `NO`） | main.ts `wirePause` |
| `death` | 死亡画面 `<section class="screen screen--death" hidden>`（原作の `UMG_DeathScreen` のウィジェット木の順。10 記録）。子に `.death__canvas`（`.death__back` の中の `.death__lives` と 6 つの `i.death__life`、`nav.death__menu`（初期 `inert`）の `button.death__btn[data-death="restart|checkpoint|quit"]`（RESTART / LAST CHECKPOINT / QUIT TO TITLE）、`.death__heading`（`REMAINING LIVES:`）、`img.death__dead`（`./title/you-are-dead.webp`）、`.death__tip`、`.death__cover`、`.death__vignette`）と `#death-restart` | `src/hud/death.ts` の `DeathFx`、main.ts の `wireDeath` と `onDeath` |
| `death-restart`, `death-restart-title`, `death-restart-text`, `death-yes`, `death-no` | 死亡画面の RESTART の確認 `.confirm-layer`（初期 `hidden`。タイトルの `#restart` と同じ部品と枠 `./title/restart-frame.webp`、本文 `ARE YOU SURE YOU WANT TO RESTART?`、ボタン `YES` / `NO`） | `DeathFx` の `openPopup` / `closePopup` |
| `death-warning`, `death-warning-text`, `death-warning-yes`, `death-warning-no` | 死亡画面の LAST CHECKPOINT の S ランクの警告 `.confirm-layer`（初期 `hidden`。同じ部品に `.confirm--long` と枠 `./title/blank-frame.webp`、本文 3 行、ボタン `YES` / `NO`） | `DeathFx` の `openPopup` / `closePopup`（main.ts の `wireDeath`） |
| `end`, `clear-next` | 脱出の画面 `<section class="screen screen--clear" hidden>`（原作の `UMG_LevelClear` のウィジェット木の順。10 記録）。子に `.clear__canvas`（`.clear__black`、`.clear__results`（`.clear__level`、`h2.clear__heading` `RESULTS`、2 本の `img.clear__line`（`./title/results-line.webp`）の間の `.clear__rows` の 6 行 `.clear__row[data-row="time\|soulShards\|bonusShards\|secrets\|livesLost\|shardStreak"]`（`.clear__label` / `.clear__value` / `.clear__rank` / `.clear__shards`）、`.clear__total`（`TOTAL SHARDS:` と `.clear__total-value`）、`.clear__final`（`FINAL RANK` と `.clear__final-rank`））、`p.clear__easy`（`EASY MODE`）、`button#clear-next.clear__next`（`NEXT`、初期 `inert`）、`.clear__red` の中の `img.clear__escaped`（`./title/you-escaped.webp`）、`.clear__fade`、`.clear__vignette`、`.clear__flash`） | `src/hud/level-clear.ts` の `LevelClearFx`、main.ts の `onEnd` と NEXT |
| `fatal` | 致命的エラー表示 | main.ts `fatal()`（`scripts/smoke.mjs` も監視） |

- 左下の `.title__copyright` は `UNOFFICIAL FAN GAME — NOT AFFILIATED WITH GLOWSTICK ENTERTAINMENT`（原作の位置にある著作権表記の代わり）。
- OPTIONS の行は原作どおり: GRAPHICS（QUALITY の矢印、RESOLUTION SCALE と BRIGHTNESS のスライダー、見えない RESOLUTION の行）、AUDIO（MUSIC / SFX / DIALOGUE のスライダー、SUBTITLES のチェック、見えない RESOLUTION の行）、CONTROLS（MOUSE SENSITIVITY のスライダー、HEAD BOBBING / INVERTED Y AXIS / TOGGLE SPRINT / MOUSE SMOOTHING のチェック）、DIFFICULTY（DIFFICULTY の矢印）。矢印の `aria-label` は `QUALITY を下げる` / `上げる` など。

### CONFIG の構造と既定値（config.ts）
| グループ | キー = 既定値 |
|---|---|
| `render` | `preferWebGPU=false`（既定 WebGL2。02 記録）, `hardwareScaling=1`, `maxDevicePixelRatio=1`, `msaaSamples=1`, `clearColor=[0.004,0.003,0.003]`, `depthProxies={enabled:true, cell:20}`（深度のパスが静的な形を 20 m の格子ごとに束ねて描く。09 記録） |
| `camera` | `fov=90`, `fovFast=115`, `fovSpeeds=[3,9]`, `fovInterpSpeed=0.5`（原作の `FOV Multiplier`。05 記録）, `near=0.05`, `far=90`, `eyeHeight=1.62`, `mouseSensitivity=0.0021` |
| `player` | `walkSpeed=3`, `sprintSpeed=6`（原作の Walking / Sprinting Speed）, `capsuleRadius=0.32`, `capsuleHeight=1.8`, `pickupRadius=0.5`（原作のプレイヤーの CollisionCylinder の半径。シャードの回収の判定だけに使う。08 記録）, `gravity=-19.6`, `boost={speed:8.7, duration:6.75, cooldown:8.5}`, `teleport={minDistance:2.5, maxDistance:10, startAlpha:0.6, wheelStep:0.1, traceDepth:5, lagDelay:0.5, lagSpeed:10, landHeight:1.25, cooldown:5, ring:{glow:{size:2, density:2.33, color:[1.0,0.15,0.5], intensity:0.6}, slash:{size:1.3, rate:10, spacing:0.5, life:1, spin:[0.5,1], color:[4,1.2,2]}, sparks:{rate:30, life:2, size:[0.02,0.05], speed:[0.6,1.0], radius:0.8, color:[5,0,0], end:[1,0,0]}}}`（原作 `BP_Power_Teleport` と `P_ky_cutter2` の値。05 記録） |
| `lights` | `count=301`（ステージの灯の数。07 記録）, `shadowCasters=3`, `shadowMapSize=512`, `mode='hybrid'`（`'baked'|'realtime'|'hybrid'`。'hybrid' のライトマップのある面では近い灯の拡散は灯の状態が焼き込みから変わった分だけ。07 記録）, `color=[1.0,0.5,0.2]`（レベルに色が無いとき）, `intensity=6.25`（出力 1 = 2.4 cd の灯。ライトマップ × 1 に合わせた）, `range=11`（レベルに範囲が無いとき）, `reassignInterval=0.2`, `hysteresis=0.3`, `fadeTime=0.35`, `flicker={amount:0, speed:9}`（ホテルの電灯はちらつかない）, `emissiveIntensity=9` |
| `lightmap` | `full='assets/level/lightmap_{page}.ktx2'`, `indirect='assets/level/lightmap_indirect_{page}.ktx2'`（`{page}` はライトマップのページ。07 記録）, `scale=4`（使っていない。倍率は level-meta.json のページごと）, `intensity=1`（ベイクが UE の単位なので 1。Hotel は 1.6） |
| `environment` | `hdr='assets/env/ballroom_1k.hdr'`, `intensity=0.2`, `diffuse=0.05`, `cubeSize=256`（デバッグフィールドとキャプチャが無いときの HDRI）, `captures={enabled:true, size:128, intensity:1}`（原作の反射キャプチャ。07 記録） |
| `materials` | `normalStrength=1.0`, `anisotropy=8`（以前の館のテクスチャの繰り返し `tiling` は削除） |
| `post` | `taa={enabled:true, samples:8, factor:0.12, reprojectHistory:true, clampHistory:true}`, `fxaa=false`, `bloom={enabled:true, scale:1}`（UE 4.21 のガウスブルーム。`scale` は UE の上に掛ける倍率。09 記録）, `ssao={enabled:true}`, `dof={enabled:true}`（原作のボリュームの SSAO と Gaussian DOF。09 記録）, `ssr={enabled:true, debug:0}`（原作のボリュームの画面空間の反射。`debug` 1・2 は反射の重みや当たりを色で出す。09 記録）, `ue={exposureScale:1}`（ステージのボリュームの手動露出に掛ける倍率。09 記録）, `motionBlur={enabled:true, objectBased:true, strength:1.2, samples:32}` |
| `game` | `shardCount=301`（ステージの shard_C）, `lives=3`（原作の Total Lives。04 記録）, `shard={model:'assets/models/wasami_mochi.glb', size:0.55, glow:0.3, reach:0.4957}`（`reach` は原作の BP_Shard の当たりの球の半径。08 記録）, `special={red:{size:2, height:1.1, reach:0.99, tint:[1,0.09,0.06], glow:[1.3,0.05,0.03]}, orb:{scale:0.5408, height:1.2525, reach:0.45, albedo:[0.526,0.094,0.047], emissive:[1.4,0.36,0]}, halo:{size:1.8, intensity:1.2, red:[1,0.0137,0], orb:[1,0.287,0]}, sphere:{intensity:6}, map:{red:'#ff2a1a', orb:'#ff9a1a', enemy:'#ff2020', redSize:1.5, orbSize:1.5, enemySize:2.5239}}`（特殊シャード。大きさ・高さ・取れる広さは原作のデータ、見た目と地図の色は推定。08・11 記録）, `gateOpenTime=4.2`, `gateLift=3.8`（デバッグフィールドの門）, `chaseOnAllShards=false`（ステージの全回収は障壁を壊すだけ）, `saveKey='wasami-deception.save.v4'`, `settingsKey='wasami-deception.settings'`（プレイヤーの設定。04 記録）, `checkpointWarningKey='wasami-deception.lastCheckpointWarning'`（LAST CHECKPOINT の S ランクの警告に YES と答えたか。NEW GAME で戻る。04 記録）, `savingToastSeconds=2.6`, `stage={enemyDelay:1, barrierGone:3, panel:{stop:1, escape:2, fade:1}, doorTime:1.6, specialSpeed:4.25, specials:20}`（ステージの流れ。ファンゲームのレベル BP と各 BP の Delay。`doorTime` はデバッグフィールドの門。`specials` は特別な敵のプールの数: 脱出の群れ〈15、3、2〉と罠の扉から出る敵。04・15 記録） |
| `enemy` | `enabled=true`, `count=10`（ステージの Spawn_Enemies の 10 体。15 記録）, `model='assets/models/wasami_enemy.glb'`, `height=2.1`, `modelYaw=0`, `navCell=0.35`, `radius=0.4`, `navHeight=2.0`, `walkSpeed=2`, `chaseSpeed=4.3`（原作の BP_Monkey の Walk Speed 200・Run Speed 430 cm/s）, `accel=8`, `turnRate={wander:2.5, chase:4.5}`, `senseRadius=14`, `noiseScale=1.4`, `sightRange=32`, `sightFov=140`, `loseTime=5`, `catchRadius=0.95`, `stagger={angle:100, time:0.75, cooldown:2}`, `repath=0.4`, `lookahead=2`, `idle=[1,3]`, `clipSpeed={walk:1.8, run:4.6}`, `emissive=0.12`, `chaseHold=3`, `vocal=[14,26]`, `alertRange=7`, `catch={reach:0.75, time:3.5}`（15 記録） |
| `hud.minimap` | `captureSize=2048`, `viewWidth=[40,100]`（原作のキャプチャの `OrthoWidth` 4000 / 10000 cm。11 記録）, `rotateWithPlayer=true`, `redrawHz=30` |
| `hud.tablet` | `position=[-0.194,-0.0368,0.3105]`（カメラ空間の実寸。原作の `Tablet` の配置を本体の高さで 0.21/0.238 倍したもの。10 記録）, `rotationDeg=[0,0,0]`, `size=[0.16,0.21,0.014]`, `bezel=0.005`, `cornerRadius=0.01`, `screenRadius=0.005`, `edgeRadius=0.0015`, `lowerDrop=0.26`, `stow={hideTime:0.12, showRate:17, tilt:55}`, `metal={albedo:[0.15,0.15,0.16], roughness:0.4, environment:1.0}`, `face={albedo:[0.012,0.012,0.013], roughness:0.55, environment:0.6}`, `screen={resolution:[600,800], intensity:2.0, roughness:0.12, reflection:0.3}`, `reflectionProbe={enabled:true, size:128, refreshRate:4, range:14}` |
| `hud`（その他） | `subtitleSeconds=3.2` |
| `audio` | `master=0.9`, `music=0.55`, `sfx=0.9`, `voice=1.0`, `shardPickupPitch={semitones:1, variants:9}` |
| `debug` | `stats=false`, `chase=false`, `autostart=false`, `freshSave=false`, `skipIntro=false`（true ならステージ OP を飛ばし、開始してすぐ動ける。検証スクリプト用、04・14 記録）, `fixedDt=0`（> 0 なら毎フレームその秒数だけ進める。1 フレームずつ撮影するスクリプト用、04 記録）, `traps=true`（false ならステージの床の噴出と車が止まり誰も殺さない。全シャードへ瞬間移動する検証スクリプト用、04・14 記録）, `field=false`（true なら館の代わりにデバッグフィールドを読み込む。読み込むアセットはモデル・音・HDR だけで、セーブは `game.saveKey + '.field'`。04・07 記録） |

### URL クエリ上書き（overrides.ts の `applyOverrides` / `coerce`）
- クエリの各 `[key, raw]` について `key.split('.')` でパスを辿り、親ノードと葉 `leaf` を決める。
- 次のいずれかならスキップ: 親が存在しない / 親がオブジェクトでない / `leaf` が親に無い / 葉の現在値が配列以外のオブジェクト（グループ丸ごとの上書きは不可）。パス長が 2 以上のときだけ `console.warn('[config] unknown override "<key>"')` を出す（`?foo=1` のような 1 段キーは黙って無視）。
- 型強制 `coerce(current, raw)` は **既存値の型** に合わせる:
  - number: `Number(raw)`。NaN なら throw → `console.warn('[config] bad value for "<key>":', err)` で該当キーのみ捨てる。
  - boolean: `raw === '' || raw === '1' || raw === 'true'` のときだけ true（`?debug.stats` のように値なしでも true。それ以外の文字列は false）。
  - 配列: `raw.split(',')`。`current[0]` が number なら各要素を `Number()`、それ以外は文字列のまま。
  - それ以外（string）: `raw` をそのまま代入。ユニオン型（`lights.mode` など）の値検証は行わない。
- 成功した項目は `key=raw` として集め、1 件以上あれば `console.info('[config] overrides:', applied.join(' '))`。戻り値はその配列。
- `tests/state.test.ts` の "URL overrides coerce to the existing value type" で 5 件適用・`nope.x` 無視を検証している。

### ビルド / ツール設定
- `package.json`: `wasami-deception` 0.2.0、`"type": "module"`、Node `>=22.13.0`。依存は `@babylonjs/core` 9.26.0 / `@babylonjs/havok` 1.3.14 / `@babylonjs/loaders` 9.26.0。devDeps は `typescript` 5.9.3、`vite` 8.0.13、`playwright-core` 1.63.0、`sharp`、`babylonjs-ktx2decoder`、`meshoptimizer`（^1.2.0、シャードモデルの簡略化）、`@types/node`。`version` は main.ts がタイトル右上の表記に使う。
- npm scripts: `dev`（`vite --host 127.0.0.1`）、`build`（`tsc --noEmit && vite build && node scripts/check-pages-limits.mjs dist`）、`preview`、`typecheck`、`test`（`node --experimental-transform-types --no-warnings --test tests/*.test.ts` — Node ネイティブの TS 実行）、`assets:cc2-layout`（Blender を絶対パスで `-b --factory-startup` で起動して `scripts/cc2/layout.py`。Chaotic Customer 2 の Zone_1 の前処理。12 記録）/ `assets:cc2-bake`（Blender を絶対パスで起動して `scripts/blender/build_cc2.py`。その組み立てとベイク。12 記録）/ `assets:cc2-colliders`（`scripts/cc2/colliders.mjs`。その衝突の前処理。12 記録）/ `assets:cc2-tex`（`scripts/prepare-cc2-textures.mjs`。そのテクスチャとマテリアル表。13 記録）/ `assets:cc2-luts`（`scripts/cc2/grading-luts.mjs`。ボリュームの色補正の LUT を `src/render/stage-luts.ts` に。12 記録）、`assets:ktx2`（ライトマップ）/ `assets:level` / `assets:sfx` / `assets:shard`（`scripts/prepare-shard-model.mjs`）/ `assets:enemy`（`scripts/prepare-enemy-model.mjs`）/ `assets:boostfx`（`scripts/prepare-boost-fx.mjs`。原作のスピードブーストの画面の素材。13 記録）/ `assets:title`（`scripts/prepare-title-assets.mjs`。原作のタイトル画面の UI 素材。13 記録）/ `assets:specials`（`scripts/prepare-special-shards.mjs`。原作の特殊シャードの素材。13 記録）/ `assets:vendor` / `assets:all`、`bench`（`scripts/bench.mjs`）。
- `vite.config.ts`: `base: './'`（相対パスで配信、サブディレクトリ配置に対応）、`optimizeDeps.exclude: ['@babylonjs/havok']`（Havok の wasm ローダーが `import.meta.url` を使うため事前バンドル禁止）、`server` 127.0.0.1:**5190**、`preview` 127.0.0.1:**4173**、`build.target 'es2022'`、`assetsInlineLimit 0`（アセットを data URI 化しない）、`chunkSizeWarningLimit 6000`。
- `scripts/check-pages-limits.mjs [dir ...]`（既定 `dist`）: ディレクトリ以下を再帰的に数え、`PAGES_MAX_FILE_BYTES`（25 MiB = 26,214,400 bytes。ちょうどは可）を超えるファイルごとに名前と大きさを、ファイル数が `PAGES_MAX_FILES`（20,000、Pages の Free プラン）を超えたらその数を stderr に出して終了コード 1。通れば `pages limits OK: <dir> (<n> files, largest <x> MiB)`。Cloudflare Pages はビルドの後にこの 2 つを検査し、超えるとデプロイ全体を失敗させる（2026-09-12 に 35.3 MiB の `level.bin` で失敗）ので、`build` の最後で `dist` を検査して手元でも同じ失敗になるようにしている。運用は `.claude/guides/deployment.md`。
- `tests/pages-limits.test.ts`: `spawnSync(process.execPath, ['scripts/check-pages-limits.mjs', 'public'])` の終了コードが 0 であること（失敗時は stderr をメッセージに出す）。`public/` は Vite がそのまま `dist/` にコピーするので、コミット前の `npm test` で上限超えに気づける。
- `tsconfig.json`: `target ES2022`、`module ESNext`、`moduleResolution bundler`、`lib [ES2022, DOM, DOM.Iterable]`、`types [vite/client, node]`、`strict`、`noUnusedLocals` / `noUnusedParameters` / `noFallthroughCasesInSwitch`、`isolatedModules`、`allowImportingTsExtensions`、`resolveJsonModule`（main.ts の `package.json` import に必要）、`skipLibCheck`、`noEmit`、`useDefineForClassFields`。`include: [src, tests, vite.config.ts]`。

## 依存関係
- main.ts → `./styles.css`、`../package.json`（`version` のみ。Vite の JSON named export なので他のフィールドはバンドルに入らない）、`./config`（`CONFIG`）、`./core/overrides`（`applyOverrides`）、動的 import `./game/game`（`Game`）。
- main.ts → `./hud/options`（`OptionsMenu`）、`./hud/title`（`TitleFx`、`ANIM`）、`./hud/pause`（`PauseFx`）、型だけ `./game/game`（`Game`）と `./game/settings`（`Settings`）。
- `Game` から使うメンバ: `Game.create`、`api`、`engine.getRenderWidth/Height`、`hasSave`、`start` / `resume` / `retry` / `restart`、`settings` / `saveSettings`、`audio`、`playing` / `paused`、`input.locked` / `input.requestLock`。
- `CONFIG` は src 配下ほぼ全モジュールが import する。`applyOverrides` は main.ts と `tests/state.test.ts` のみ。
- 外部ライブラリは main.ts 自体では使わない（Babylon は game チャンク側）。

## 設定・調整値
- main.ts が直接参照するのは `CONFIG.debug.freshSave`（セーブ無視）と `CONFIG.debug.autostart`（タイトル省略）のみ。
- 葉の値はすべて `?a.b.c=value` で上書き可。例: `?post.motionBlur.strength=0.6&camera.fovFast=120&lights.shadowCasters=2`、`?render.preferWebGPU=true`（WebGPU を試す。既定は WebGL2）、`?debug.autostart=true&debug.freshSave=true&debug.stats=true`（bench.mjs が使用）。

## 既知の制約・注意点
- `CONFIG` は `as unknown as Record<string, unknown>` でキャストして破壊的に書き換える。凍結していないので実行中に他モジュールが書き換えても検出できない。
- 上書きは型強制のみで範囲・列挙の検証はない（`lights.mode=foo` も通る）。配列は長さチェックなし（`lights.color=1,1` で 2 要素になる）。
- boolean は `''`/`'1'`/`'true'` 以外すべて false（`yes`, `on` は false）。
- グループ（オブジェクト）そのものは上書きできない。配列は例外的に葉扱い。
- 1 段キー（`?foo=1`）は警告なしで無視される仕様（第三者のクエリパラメータでノイズを出さないため）。
- `hasSave` 判定は `Game.create` 完了後なので、タイトル画面はそれまで出さない（`#btn-start` の `disabled` は使わない。HTML の初期ラベルは `NEW GAME`）。
- セーブありの `NEW GAME`（`#btn-reset`）は、原作と同じく RESTART? で確認し、YES で暗転してからリロードせずに最初から始まる。`game.start(true)` は YES のクリックから 3.75 秒後（title.ts の `ANIM.leave.length`）に呼ぶので、Pointer Lock と AudioContext の再開はクリックによるユーザー操作の有効期間（Chrome は約 5 秒）に収まる。原作の Delay（10 秒 / 5 秒）まで待つとこれを超えるので、暗転のアニメの終わりで始める（RESUME も同じ）。ロックが取れなかったときはキャンバスのクリックで取り直す。
- RESUME とセーブなしの NEW GAME は収録に映っていないので、確認も暗転もなく即開始する。
- タイトルのアニメの長さは title.ts の `ANIM` だけが持つ（CSS にはアニメの長さを書かない）。
- 原作メニューの CHAPTERS / REPLAY / EXTRAS / QUIT は、スライスに該当機能がないので置いていない。
- タイトル画面の `<img>` は `#title` が hidden（display:none）の間もページ読み込み時に取得されるので、タイトル表示時には読み込み済み。
- 起動に失敗した場合はローディング画面のまま `#fatal` が重なる（タイトル画面は出ない）。
- Pointer Lock 再取得は canvas click に依存する。Esc 直後 ~1 秒は Chrome がロックを拒否するため、その間のクリックは無視される。
- `assets:cc2-layout` / `assets:cc2-bake` は macOS の `/Applications/Blender.app` 固定パス。
- `assetsInlineLimit 0` と `chunkSizeWarningLimit 6000` により、Babylon を含む game チャンクが数 MB でも警告を出さない。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: 初回ロードを「ローディング画面（`#loading`、進捗バー + ラベル）→ タイトル画面」の 2 段構成に変更。タイトル画面から進捗バーと `LOADING…` の disabled ボタンを外した
- 2026-09-11: タイトル画面を原作のメインメニュー風に作り直した。ロゴを画像（`title/logo.webp`）に、ボタンを `RESUME` / `NEW GAME` / `OPTIONS` に変更（`CONTINUE` / `ENTER THE MANOR` / `最初から` を廃止）。操作説明と API 表示（`#api-label`）は OPTIONS の SETTINGS ダイアログへ移し、右上に `v0.2.0` を表示
- 2026-09-11: devDependency `meshoptimizer` と npm script `assets:shard` を追加
- 2026-09-11: `game.shard`（シャードのモデル・大きさ・自己発光）を追加
- 2026-09-11: セーブありの NEW GAME に原作の RESTART? 確認ダイアログ（`#restart`、YES / NO）を付けた。YES は赤い閃光と暗転の後、リロードせずに `game.start(true)` で最初から始める（以前はセーブを消してリロードし、タイトルでもう一度 NEW GAME を押す必要があった）
- 2026-09-11: `player.interactRadius` / `interactViewDot` を削除し、`game.shard.touchRadius`（触れて回収する判定の広さ）を追加
- 2026-09-11: `audio.shardPickupPitch`（シャード取得音のピッチの揺らぎ幅）を追加
- 2026-09-11: `audio.shardPickupPitch` を再生速度の幅 0.06 から `{ semitones: 1, variants: 9 }`（長さを保つピッチ違いの幅と個数）に変更
- 2026-09-11: `lights.hysteresis`（影付きライトの割り当てで、割り当て中のランプを優遇する割合）を追加
- 2026-09-11: `render.preferWebGPU` の既定を false（WebGL2）に変更
- 2026-09-11: `player.teleport`（テレポーテーションの距離・クールダウン・照準の輪・演出の時間軸）を追加。INPUT タブの `COLLECT SHARD = E / LEFT CLICK`（シャードは触れて回収になっていた）を `TELEPORT = E (HOLD TO AIM)` に置き換えた
- 2026-09-11: `post.teleport`（テレポートのパスの放射ズーム量とビネットの色）を追加
- 2026-09-11: `player.teleport.wheelStep`（ホイールでの距離調整の刻み）を追加。INPUT タブを原作どおりの操作（TELEPORT = Q、TELEPORT DISTANCE = MOUSE WHEEL、TELEPORT CONFIRM = LEFT CLICK、SPEED BOOST = E）に変更
- 2026-09-11: `hud.tablet.position` を原作の動画の実測で [-0.225,-0.035,0.474] → [-0.192,-0.035,0.49] にし、`hud.tablet.reach`（ダッシュで奥へ・壁際で手前へ動く奥行き）を追加。`hud.tabletSway` は歩行の上下揺れだけの倍率になった
- 2026-09-11: `player.teleport.fx` を参考動画の演出に合わせて `{charge, rush, blackout, recover, fovKick, dolly}` → `{rush, hold, recover, swing, focal, dolly}` に、`post.teleport.vignetteColor` を `white`（ホワイトアウトの色）に置き換えた
- 2026-09-11: 館の迷路に合わせ、`lights.count` 8→49、`game.timeLimit` 300→360、`game.shardCount` 3→100、`game.shardMilestone` 10 と `audio.torchLoops` 6 を追加、`game.saveKey` を v2、`hud.minimap.captureSize` 1024→2048、`viewRadius` [11,20]→[11,24] にした
- 2026-09-11: `player.teleport.fx` の rush/hold/recover を 1.25 倍（0.1/0.04/0.13 → 0.125/0.05/0.1625）、`focal` 0.3→0.2、`swing` 0.25→0.4 にし、焦点距離のバウンスの減り方 `bounce` 0.4 を追加した
- 2026-09-11: npm script `assets:enemy`（敵モデルの前処理）を追加
- 2026-09-11: 館の拡張に合わせ、`lights.count` 49→120、`game.timeLimit` 360→720 にした（全シャードを近い順に回る経路が約 1,020 m で完璧なダッシュ 142 s。以前の比率 4.8 倍）
- 2026-09-11: 敵の設定 `enemy` を追加。終了画面に `caught`（`YOU WERE CAUGHT`、`セーブから再開`）を加え、`#hud` に捕まったときの閃光 `#flash` を置いた
- 2026-09-11: `hud.tablet.reflectionProbe.range`（反射プローブに描くメッシュの範囲 14 m）を追加
- 2026-09-12: `player.teleport.fx` を原作の検証映像に合わせて `{rush 0.14, hold 1/60, dark 0.075, recover 0.235, swing 0.2, dip 0.225}` にし（`bounce` → `dip`、`dark` を追加）、`post.teleport` に `flare` を足して白を [0.93, 0.96, 0.95] に、`debug.fixedDt` を追加した
- 2026-09-12: 照準の輪 `player.teleport.marker` を検証映像に合わせて `{radius 0.85, spin 4, color [1.0, 0.62, 0.82], intensity 1.8}` にし、`appear`（弧が一周して描かれる秒数）と `sparks`（火花の毎秒の数）を追加した
- 2026-09-12: スピードブーストの `camera.fovBoost` 101、`hud.tablet.reach.boostPush` 0.17、`hud.tablet.tintTime` 0.5、`post.boost`（赤い縁と集中線、発動の閃光）を追加した
- 2026-09-12: タブレットのしまう・出す `hud.tablet.stow`（`hideTime` 0.12、`showRate` 17、`tilt` 55）を追加し、`lowerDrop` を 0.24→0.26 にした
- 2026-09-12: `debug.field`（館の代わりにデバッグフィールドを読み込む）を追加した
- 2026-09-12: `player.teleport` を pak_reference の原作の値に置き換えた（`{distance, minDistance, wheelStep, cooldown 4.5, wallGap, fx}` → `{minDistance 2.5, maxDistance 10, startAlpha 0.6, wheelStep 0.1, traceDepth 5, lagDelay 0.5, lagSpeed 10, landHeight 1.25, cooldown 5}`。カメラアニメは 05 記録の teleport-fx.ts の定数）。`post.teleport` を削除した
- 2026-09-12: 照準の輪の `player.teleport.marker` を、原作の構成の `player.teleport.ring { glow, slash, sparks }` に置き換えた
- 2026-09-12: npm script `assets:boostfx`（スピードブーストの画面の素材を pak_reference から作る）を追加
- 2026-09-12: npm script `assets:title`（タイトル画面の素材を pak_reference から作る）を追加
- 2026-09-12: `hud.tablet.tintTime` を削除した（スピードブーストの赤は原作のカメラアニメのシーンのティントになり、値は 05 記録の `boost-fx.ts` にある）
- 2026-09-12: `post.boost`（検証映像から合わせたブーストの赤い縁と集中線、閃光）を削除した（原作のデータは 05 記録の `boost-fx.ts`）
- 2026-09-12: タイトル画面を原作のデータ（`UMG_TitleScreen` / `UMG_PopUp`）に倣って作り直した: メニューの結線を Game の生成後の `wireMenu(fx, hasSave, start)` に移し、アニメと音を `hud/title.ts` の `TitleFx` に任せた（RESUME も原作どおり暗転してから始まる。`setRestart`・`RESTART_CLOSE_MS`・`TITLE_LEAVE_MS` を削除）。`index.html` の `#title` を原作のウィジェットの順にし、`.title__mask` / `.title__cover` / `.title__flash` / `.confirm__backdrop` / `img.confirm__frame` を追加、`.title__glow` を削除
- 2026-09-12: タイトルの OPTIONS を原作の設定画面（`UMG_Options`）にした: `index.html` の SETTINGS ダイアログ（`btn-options-close`、`opt-renderer` / `opt-resolution`、VIDEO / INPUT タブ）を原作のウィジェット木の `#options` に置き換え、`wireMenu` に `prefs`（`game.settings` / `game.saveSettings`）と OPTIONS の開閉（`OptionsMenu`、`closeOptions`、SAVE & EXIT / CANCEL / Esc）を加えた。起動時に `DD Roboto` を読み込む。`CONFIG.game.settingsKey` を追加
- 2026-09-12: `camera.fovWalk` / `fovSprint` / `fovBoost` / `fovLerp` を原作の FOV の曲線 `fov` / `fovFast` / `fovSpeeds` / `fovInterpSpeed` に、`player.walkSpeed` / `sprintSpeed` を原作の 3 / 6 に、`player.boost.multiplier` を `speed` 8.7 に置き換え、`player.acceleration` / `deceleration` を削除した（05 記録）
- 2026-09-12: `camera.headBobAmplitude` / `sprintRoll` を削除し（頭の揺れは原作のカメラシェイク。05 記録）、`camera.headBobFrequency` は足音の周期、`hud.tabletSway` はダッシュで下がる量の倍率だけになった
- 2026-09-12: `camera.headBobFrequency` と `audio.footstepInterval` を削除した（足音は原作の Tick の Delay。05 記録）
- 2026-09-12: `hud.tablet.reach.sprintPush` / `boostPush`、`hud.tablet.swayMetersPerPx`、`hud.tabletSway` を削除した（タブレットは原作どおりカメラの FOV だけで縮む。10 記録）。`camera` の説明に FOV の追従の速さ（5.0/s、05 記録）を書いた
- 2026-09-12: ポーズ画面を原作の `UMG_Pause` に倣って作り直した: `index.html` の `#pause`（`PAUSED`・`RESUME`・`セーブを消して最初から`）を原作のウィジェット木（赤いぼかし、黒い筆の帯、ワサミの頭、EASY MODE、RESUME / RESTART / OPTIONS / QUIT、RESTART? と GIVING UP? のカード）に置き換え、`#options` を `#title` の外へ移してポーズからも開けるようにした。main.ts は OPTIONS の結線を `wireOptions` に分け、`wirePause` と `PauseFx`（10 記録）でポーズ画面を動かす。`#btn-resume` / `#btn-restart` を削除
- 2026-09-12: npm script `build` の最後に `node scripts/check-pages-limits.mjs dist` を足し、`tests/pages-limits.test.ts` で `public/` を検査するようにした（Cloudflare Pages の 1 ファイル 25 MiB・20,000 ファイルの上限。35.3 MiB の `level.bin` でデプロイが失敗したため）
- 2026-09-13: ユーザーの指示で制限時間を撤廃した: `game.timeLimit` を削除し、終了画面の `timeUp`（`TIME'S UP`）をなくした
- 2026-09-13: `game.lives`（ライフの数 3。原作の `BP_DD_SaveGame` の Total Lives）を追加した
- 2026-09-13: 死亡画面 `#death`（原作の `UMG_DeathScreen`）を `DeathFx` と `wireDeath` で結線し、`onDeath(lives)` を加えた。終了画面は脱出だけになり（`onEnd(detail)`、`endKind` を削除）、`#flash` を外した
- 2026-09-13: 画面を 16:9 に固定した: `index.html` の `<body>` の中身を `#stage` で包み、ポインターの取り直しを舞台の外の黒帯のクリックでも行うようにした（canvas の click から `document` の click へ）
- 2026-09-13: `hud.tablet.position` を FOV 75° 換算の [-0.192,-0.035,0.49] から原作データ由来のカメラ空間の実寸 [-0.194,-0.0368,0.3105] にし（どの状態でも約 1.2 倍大きく見える）、`hud.tablet.reach`（壁際で手前へ寄る奥行き）を削除した（10 記録）
- 2026-09-13: `camera.turnAroundTime`（中クリックの 180° ターンの時間）を削除した。原作どおり向きを一度に変え、カメラの回転ラグで回す（05 記録）
- 2026-09-13: `hud.minimap.viewRadius` [11, 24]（矩形の短辺の半分の m）を `viewWidth` [40, 100]（タブレットのマップのパネルの幅の m。原作のキャプチャの `OrthoWidth`）に置き換えた（11 記録）。URL の上書きも `?hud.minimap.viewWidth=` になる
- 2026-09-13: `index.html` にステージ OP の層 `#intro`（原作の `UMG_ChapterPortal`。10 記録）を足した
- 2026-09-13: `debug.skipIntro`（ステージ OP を飛ばす。検証スクリプトが使う）を足した（04・14 記録）
- 2026-09-13: 原作の Hotel に置き換えた（ユーザーの指示）: `lights.count` 120 → 167、`lights.flicker.amount` 0.08 → 0、`game.shardCount` 100 → 289、`game.saveKey` v2 → v3、`game.hotel` を追加、`audio.torchLoops` 6 → 0、`materials.tiling` を削除。npm scripts の `assets:textures` を削除し、`assets:hotel-layout` / `assets:hotel-tex` を足し、`assets:bake` を `build_hotel.py` に、`assets:all` をホテルの順に。`index.html` に乗車の暗転 `#fade` を足した
- 2026-09-13: `game.shardMilestone` を削除した（10 個ごとの節目をやめ、原作のシャード連続回収にした。04・10 記録）。`index.html` の `#hud` の先頭に連続回収の節目の表示 `#streak`（`.streak__card` / `.streak__vignette` / `.streak__life`）を足した
- 2026-09-13: npm script `assets:specials`（原作の特殊シャードの素材を pak_reference から作る）を追加
- 2026-09-13: 原作の特殊シャードのため `game.special`（見た目・高さ・取れる広さ・光の輪・硬直の球・地図の印）を追加し、`index.html` の `#hud` に `#vsides`（`UMG_VignetteSides`）を足した
- 2026-09-13: UE 4.21 のトーンマップにしたため、`post.toneMapping` / `exposure` / `contrast` / `vignette` / `grain` / `chromaticAberration` を削除し、`post.ue.exposure` を追加した（09 記録）
- 2026-09-13: `post.bloom` を UE 4.21 のブルームの `{ enabled, scale }` に、`post.ue.exposure` をゾーンごと（`maze` 0.5・`lobby` 0.3・`hotel` 0.5）にした（09 記録）
- 2026-09-13: 灯のグレアのスプライトを外したので `lights.halo` を削除した（07 記録）
- 2026-09-13: ライトマップの焼き直しに合わせて `post.ue.exposure` を `maze` 0.75・`lobby` 0.35 にした（09・12 記録）
- 2026-09-13: `post.ssao` と `post.dof`（09 記録）、`environment.captures`（07 記録）を足し、反射を原作のキャプチャにしたので `post.ue.exposure` を `maze` 0.76・`lobby` 0.7 に合わせ直した
- 2026-09-13: `post.ssr`（原作の画面空間の反射と、そのデバッグ表示）と `render.depthProxies`（深度のパスの代理）を足した（09 記録）
- 2026-09-14: 脱出の画面 `#end` を原作の `UMG_LevelClear` のウィジェット木に作り直し（`#end-title` / `#end-sub` / `#btn-again` を削除）、`LevelClearFx` と `onEnd(results)`・NEXT（`#clear-next`）を結線した。LAST CHECKPOINT は `game.restart(true)`（原作の Used Hard Respawn?）
- 2026-09-14: 原作の LAST CHECKPOINT の S ランクの警告（`UMG_PopUp` の枠 2、`#death-warning`）を `wireDeath` に結線し（YES まで毎回聞き、YES を `game.checkpointWarning` に保存）、Esc でも閉じるようにした。セーブの無い NEW GAME も `start(true)`（警告のフラグを戻す）。`index.html` に秘密を取ったときの表示 `#collectable` と `#death-warning` を足し、`CONFIG.game.checkpointWarningKey` を足した
- 2026-09-14: 原作に無い HUD の要素 `#prompt`（拾得プロンプト）と `#banner`（中央のバナー）を index.html から消した（10 記録）
- 2026-09-14: `audio.torchLoops` を削除した（炎の音 `torch_loop` を音源ごと外した。06 記録）
- 2026-09-14: `post.radialBlur` と `post.chaseTint` を削除した（09 記録）
- 2026-09-14: `enemy.frenzySpeed` と `enemy.frenzyEmissive` を削除した（フレンジーの敵の 1.15 倍速と赤い発光をユーザーの指示で撤廃。15 記録）
- 2026-09-14: `game.shard.touchRadius`（0.3）を原作の当たりの球の `reach`（0.4957）に替え、`player.pickupRadius`（0.5、原作のプレイヤーの半径）を足した（シャードの判定がシビアというユーザーの指摘。08 記録）
- 2026-09-14: npm scripts に `assets:cc2-layout`（Chaotic Customer 2 の Zone_1 の前処理。12 記録）を足した
- 2026-09-14: npm scripts に `assets:cc2-bake`（その組み立てとベイク。12 記録）を足した
- 2026-09-14: npm scripts に `assets:cc2-colliders`（その衝突の前処理。12 記録）を足した
- 2026-09-14: npm scripts に `assets:cc2-tex`（そのテクスチャとマテリアル表。13 記録）を足した
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 にした（ユーザーの指示）: `lights.count` 167 → 301、`lights.mode` 'realtime' → 'hybrid'（値に 'hybrid' を足した）、`lightmap.full` / `indirect` をページの `{page}` 付きの URL に、`game.shardCount` 289 → 301、`game.chaseOnAllShards` true → false、`game.saveKey` v3 → v4、`game.hotel` を `game.stage` に、`enemy.count` 3 → 10
- 2026-09-14: ステージのポストプロセスボリュームにした（09 記録）: `post.ue.exposure`（ゾーンごとの推定の露出）を `post.ue.exposureScale` 1（ボリュームの手動露出に掛ける倍率）に、`lightmap.intensity` と `environment.captures.intensity` を 1.6 → 1、`lights.intensity` を 10 → 6.25 に。npm scripts に `assets:cc2-luts` を足した
- 2026-09-14: Hotel を削除した（ユーザーの指示）: npm scripts の `assets:hotel-layout` / `assets:hotel-tex` / `assets:bake` を消し、`assets:all` をステージの順に。index.html から秘密を取ったときの表示 `#collectable` を消した
- 2026-09-15: `index.html` の `#hud` に `#aim`（パンチの溜め。ファンゲームの `Aim`。10 記録）を足した
- 2026-09-15: `enemy.spares` 4（パンチで倒れた後のリスポーンの予備の個体。15 記録）を足した。`enemy.catch.time` の記載を実装の 3.5 に直した
- 2026-09-15: `enemy.walkSpeed` 2・`chaseSpeed` 4.3 にし、`closeBoost`・`closeRange` を削除した（原作の BP_Monkey。15 記録）
- 2026-09-15: パンチと敵のリスポーンをやめたので、`index.html` の `#aim`（10 記録）と `enemy.spares`（15 記録）を削除した
- 2026-09-15: ロックピックの障壁を手のマークと 1 クリックで壊すようにしたので、`index.html` の `#lockpick`（ファンゲームの `Lockpicking` のウィジェット。10 記録）を削除した
