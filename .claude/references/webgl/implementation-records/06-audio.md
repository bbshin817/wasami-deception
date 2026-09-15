---
title: オーディオ
sources:
  - src/audio/audio.ts
  - src/audio/pitch.ts
  - src/audio/stage-sounds.ts
  - src/game/stage-music.ts
  - tests/pitch.test.ts
  - tests/stage-music.test.ts
updated: 2026-09-15
---

# オーディオ

## 役割
Web Audio API を直接使った小さなミキサー。BGM のクロスフェード、ワンショット SE、ループ層、HRTF 定位エミッタ、リスナー追従を提供する。ゲームの音と UI の音（タイトルとポーズ画面）を別の AudioContext で鳴らし、ポーズ中はゲームの音だけをその位置で止める（原作の SetGamePaused は UI 音以外を一時停止する）。
Babylon.js のオーディオエンジン（v1/v2 とも）は使用しない。どの音をいつ鳴らすかの判断は `src/game/game.ts` 側にある。

## 公開インターフェース
- `interface Vec3Like { x; y; z }`
- `interface LoopHandle { gain: GainNode; panner?: PannerNode; moveTo(p); stop(fade = 0.6) }` — `moveTo` は位置付きのループのパンナーを `p`（audio 空間）へ動かす（定位なしなら何もしない。車とトラックの音が付いて行く）。
- `interface Attenuation { inner; falloff; natural? }` — UE のインラインの減衰（球。m）。`PlayOptions.attenuation` と `loop` の `attenuation` で渡すと、その音のパンナーを既定の代わりにこれにする（下の「パンナー」）。
- `class AudioManager`
  - private `world` / `ui`（`Mix { ctx: AudioContext; buses }`。ゲームの音と UI の音）。`AudioBuffer` は両方で共用する（デコードと事前生成はゲームの文脈）。
  - `decode(id: string, data: ArrayBuffer): Promise<void>` — `decodeAudioData` して `buffers` に登録。失敗時は `console.warn('[audio] cannot decode …')` して無視。
  - `has(id)`, `duration(id)`（未登録は 0）
  - `resume()` — ブラウザがクリックかキー入力まで止めている音を出す: UI の文脈は常に、ゲームの文脈はポーズ中でなければ `ctx.resume()`。
  - `setPaused(paused)` — `true` でゲームの文脈を `suspend()`（BGM・ループ・予約済みの音がその位置で止まり、`currentTime` も止まる）、`false` で `resume()`。UI の文脈は変わらない。game.ts の `pause()` / `resume()` が呼ぶ。
  - `states()` — `{ world, ui, paused }`（2 つの文脈の `state` と、ポーズ中か。デバッグ API の `state().audio`）。
  - `play(id, { volume = 1, rate = 1, bus = 'sfx', position?, delay = 0, envelope?, ui = false })`（`ui` なら UI の文脈で鳴らす） → `AudioBufferSourceNode | null`（実体は private `playBuffer(buffer, options)`）。`delay` 秒後に鳴らし始める（`src.start(currentTime + delay)`）。`envelope` は `[開始からの秒, ゲイン]` の点の列で、`volume` の代わりにゲインを点から点へ直線で動かす（最初の点の値から始め、最後の点の値を保つ。`setValueAtTime` + `linearRampToValueAtTime`）。
  - `preparePitchVariants(id, semitones, count)` — デコード済みの `id` から、ピッチを ±`semitones` に等間隔で散らした `count` 個の `AudioBuffer` を `pitchShift` で事前生成する（長さ・テンポ・サンプルレートは元と同じ）。`id` が無ければ何もしない。
  - `playPitchVariant(id, options)` — 事前生成した中からランダムに 1 つ再生する。直前と同じものは選ばない（残り `count − 1` 個から一様に選ぶ）。未生成なら `play` と同じ。
  - `playVariant(prefix, count, volume = 1, position?)` — `${prefix}1..count` からランダムに 1 つ、`rate = 0.94 + random × 0.12` で再生（`position` があればその位置で）
  - `play(id, options)` の `options.position`（`PlayOptions.position`、audio 空間の座標。`AudioManager.toAudioSpace` で変換する）— 1 回きりの音をその位置で鳴らす
  - `loop(id, { volume = 1, rate = 1, bus = 'sfx', fadeIn = 0.5, offset?, position?, ui = false, repeat = true })` → `LoopHandle | null`。`repeat: false` は 1 回だけ鳴らす（`stop(fade)` で止められる）。`rate` は再生速度（UE の Pitch と同じく音程も変わる）。`offset` は鳴らし始める位置（秒）で、省くと従来どおりランダム（同じループが重なっても位相がそろわないように）。
  - `setVolumes({ music, sfx, voice })` — 設定の音量（0..1。タイトルの OPTIONS の MUSIC / SFX / DIALOGUE。原作のサウンドクラスの音量）を両方の文脈の各バスの `CONFIG.audio.*` に掛けて `setValueAtTime` で即座に入れる。game.ts の `applySettings` が起動時と SAVE & EXIT で呼ぶ。
  - `busGains()` — ゲームの文脈の各バスの今のゲイン（デバッグ API 用）。
  - `setMusic(id | null, fade = 1.5, volume = 1, { fadeOut?, offset? })` — 現在の BGM と同じ id なら何もしない。旧 BGM を `stop(fadeOut ?? fade)`、新 BGM を `music` バスで `fadeIn = fade` で開始（`offset` 秒から。省くとループのランダムな位置から）
  - `musicId` — 今の BGM の id（null: 無し。デバッグ API の `state().music`）
  - `setListener(pos, forward, up)`
  - `static toAudioSpace(p)` — Babylon（左手系）→ Web Audio（右手系）へ Z を反転

## 内部構造と処理の流れ
### グラフ構成（コンストラクタ）
- `mix()` で `new AudioContext({ latencyHint: 'interactive' })` とその配線を 2 組作る: ゲームの音の `world` と UI の音の `ui`（同じ構成。コンプレッサも文脈ごと）。以下は 1 組の構成。
- `master` GainNode（`CONFIG.audio.master = 0.9`）→ `DynamicsCompressorNode`（`threshold −10 dB`, `ratio 3`、他は既定）→ `ctx.destination`。
- バス 3 本（それぞれ GainNode → master）: `sfx` = `CONFIG.audio.sfx (0.9)`, `voice` = `CONFIG.audio.voice (1.0)`, `music` = `CONFIG.audio.music (0.55)`。それぞれに設定の音量が掛かる（`setVolumes`。原作のサウンドクラスとの対応は Music → `music`、SFX → `sfx`、Dialogue → `voice`。タイトルとポーズ画面の BGM `Pause_Sound_v1` は原作でも SFX クラスなので、UI の文脈の `sfx` バス）。
- 実効音量 = 個別 `volume` × バス × master（コンプレッサ前）。

### サウンドレジストリ（`buffers: Map<string, AudioBuffer>`）
- 音源の一覧は **`public/assets/manifest.json`**（`scripts/build-level.mjs` が生成）の `kind === 'audio'` エントリ。`Game.create` が `AssetStore.preload` で ArrayBuffer を取得し、URL のファイル名から拡張子（`.ogg` / `.mp3`）を除いた文字列を id として `audio.decode(id, buffer)` を全件 `Promise.all` で実行する。
- したがって id は `sfx/shard_pickup.ogg → 'shard_pickup'`、`voices/greeting.mp3 → 'greeting'` のように SE とボイスが同じ名前空間に入る。
- `public/sfx/manifest.json` は `scripts/copy-sfx.mjs` が生成する `{ id: { url, source } }` 形式の出自記録で、**ランタイムでは読まれない**（`src/` からの参照なし）。
- `public/voices/manifest.json` は `Game.create` が fetch し、`clips[].id` と `clips[].subtitle` だけを `Map<string, Voice>` に取り込む（`category`, `duration`, `truePeakDB`, `normalization` は未使用）。取得失敗時は `{ clips: [] }`。

### 再生系
- `play`: `AudioBufferSourceNode` → 個別 GainNode → バス。`src.start()` 即時。未登録 id は null を返して無音（例外にしない）。
- `playVariant`: 敵の足音など番号付きバリエーション向け。ピッチを ±6% 揺らす。
- 位置付きの 1 回きりの音（`play` / `playVariant` の `position`）: `src → gain → spatial(position) → bus`。`spatial` はループと共通の `PannerNode` の設定（下）を作る。鳴らすたびに新しいパンナーで、音源が動いても位置は鳴らした時点のまま（敵の足音やボイスは短いので追わない）。
- `loop`: `src.loop = repeat`（既定 true。`repeat: false` なら 1 回だけ鳴らし、フェードで止められる一度きりの音にする。死亡画面のゲームオーバーの曲）。ゲインを `0 → volume` に `fadeIn` 秒で `linearRampToValueAtTime`。`position` があれば `spatial(position)` の `PannerNode`（`panningModel 'HRTF'`, `distanceModel 'inverse'`, `refDistance 1.2`, `rolloffFactor 1.6`, `maxDistance 40`）を挟む。`offset` が無ければ、`repeat` のときは**開始オフセットを `random × buffer.duration` にして**（`repeat: false` は先頭から）同一ループ（松明）が位相同期しないようにする。`stop(fade)` は現在値からゲインを 0 へランプし `fade + 0.05` 秒後に `src.stop`。
- パンナー（`spatial(ctx, position, attenuation?)`）: `panningModel 'HRTF'`。`attenuation` が無ければ本作の既定（`inverse`、`refDistance 1.2`、`rolloffFactor 1.6`、`maxDistance 40`）。あれば UE の球に合わせる: `refDistance` = 内側の半径、`maxDistance` = 内側の半径 + 減衰の距離。Linear は `distanceModel 'linear'`・`rolloffFactor 1`（内側で等倍、端で 0）。NaturalSound は `distanceModel 'exponential'`・`rolloffFactor = 3 / log10(max / ref)`（端でちょうど −60 dB。UE の NaturalSound の曲線そのものではない近似）。
- `setMusic`: BGM は `loop()` の薄いラッパ。`music` フィールドに `{ id, handle }` を 1 本だけ保持。既定はフェードアウトとフェードインを同じ長さで重ねるクロスフェード（ステージの曲は `fadeOut` で別の長さにする）。
- `setListener`: `AudioListener` の `positionX/Y/Z`, `forwardX/Y/Z`, `upX/Y/Z` を `setTargetAtTime(値, now, 0.02)` で滑らかに更新（Z は符号反転）。`positionX` が無い古い実装ではスキップ。

### `game.ts` からの使い方（誰が何を鳴らすか）
- **アンロック**: `AudioContext` はページロード時（`Game.create`）に生成されるため、ブラウザは `suspended` で始まる。`Game.start()`（タイトルの START ボタン＝ユーザー操作）と `Game.resume()`（RESUME ボタン）で `audio.resume()` を呼び再生を有効化する。デコードは suspended 状態でも進む。
- **BGM（デバッグフィールド）**: `start()` と `again()` で `setMusic(chase ? 'bgm_chase' : 'bgm_normal', 2.5 / 2)`。`chaseChanged` で `setMusic(chase ? 'bgm_chase' : 'bgm_normal', chase ? 0.4 : 2)`（追跡開始は 0.4 秒で急転、解除は 2 秒）。ステージではどれも鳴らさない。`title_theme` はデコードされるが現状どこからも再生されない。
- **ステージの曲**（Chaotic Customer 2 の Zone_1。`src/game/stage-music.ts` の `StageMusic`、`STAGE_MUSIC`）: game.ts の `toCheckpoint`（レベルを開き直す。原作の RestartLevel）が `setMusic(null, 0.1)` で切り、`stageMusic.reset(checkpoint, allShards)`。広間のトリガー（`checkpoint` イベントの 2）で `reachHall()`。`updateStage` が毎フレーム `update(dt, 敵が追っているか || 強制の追跡, allShards)` を呼び、返った `MusicEvent` を鳴らす（`music` → `setMusic(id, fadeIn, volume, { fadeOut, offset })`、`once` → `play(id, { bus: 'music' })`）。
  - 探索中（チェックポイント 1、トリガーまで）は無音。トリガーから 0.6 s（`music_manequins_start` の Delay）で、誰にも見られていなければ `cc2_carol`（`CC_Carol_Of_the_bells`。fadeIn 2 s、もう一方を 1 s で消す）、見られていれば `cc2_dethsmass`（`CC_Merry_Dethsmass`。fadeIn 0.5 s、もう一方を 0.5 s で消す）。どちらも 2D・ループ・0.7。切り替えるたびに曲の頭から（原作は毎回新しい AudioComponent）。チェックポイント 2 から開き直したときも 0.6 s 後に始まる（原作の `trigger` の BeginPlay）。
  - 全回収で両方を 2 s で消し（`stop_music_maneqiuns`）、以後は鳴らさない（開き直しても）。
  - 脱出（チェックポイント 4）: 開いて 1.5 s（街灯が色の巡回に変わる時）に `cc2_horrors`（`CC_Merry_horrors`。`FadeIn(0.1, 1, 6)`: 曲の 6 s から 0.1 s で、1.0、ループ）。その `parkovkaDelay()` 秒後（`RandomFloatInRange(RandomFloatInRange(10, 14), RandomFloatInRange(16, 18))`）に `cc2_parkovka`（`PARKOVKA_LOMAETCA`）を 2D で 1 回。原作が 3 s で消す `Timeline_8` の後は分からないので、脱出の終わりまで鳴らす。
  - `track` は今の曲（`'calm'`・`'chase'`・`'escape'`・null。デバッグ API の `state().stageMusic`）。
- **ステージの効果音**（`src/audio/stage-sounds.ts` の `STAGE_SOUND`。キューごとに音の候補・音量（波形 × キュー × コンポーネント）・ピッチ・減衰（cm → m）か、減衰無し＝2D。game.ts の `cue(c, at?)` が候補から 1 つをその位置で、`cueLoop(c, at, { fadeIn, repeat, offset })` がループで鳴らし、`follow(handle, at)` で動かす）:
  - 障壁: ロックピックとダッシュの障壁は立っている間 `cc2_barrier_loop`（600/1000 NS。`setBarrier` で掛け直し）、壊れると止めて `cc2_barrier_shatter` 0.75（1000/3000 NS。シャードの障壁 `barrier_C` は 550/3600 NS）。シャードの障壁がチェックポイント 1 で Box に初めて触れて立つとき `shardBarrierAppear` = `stun_wave` 1.0（550/3600 NS。ファンゲームの `Stun_Wave_Attack_New_01` は書き出しにも音源パックにも無いので、同じ組の `_04` で代わりにする。04 記録の `updateShardBarriers`）。
  - 扉: 施錠 `cc2_door_locked`（600/2400 Linear。4 s に 1 回）、開き戸 `cc2_door_open_1..3` / `cc2_door_close_1..3`（500/3600 NS）、通気口 `cc2_vent_1..2`（2D）、シャッターのボタン `cc2_garage_button`（1000/5000 NS）と `cc2_garage_open`（1000/6000 NS。マネキンが開けるときは後者だけ）、壊れる `cc2_crash_1..2` 1.7（1000/9000 NS）、遮断機 `cc2_blocker` 2（600/2500 NS）、罠の扉が破れる `cc2_trap_breach` 0.6（600/3600 NS）。
  - 罠: 噴出 `cc2_jet`（700/7000 NS）、車のエンジン `cc2_car_engine`（1000/5000 NS。レベルを開くたびに掛け直し、車に付いて行く）、クラクション `cc2_car_horn`（1000/5000 Linear。車の前の Box1 に入ると、3 s に 1 回）。
  - トラック: 出るときに `cc2_truck_beep_1..2` 2（1000/10000 NS、1 回）とエンジン `cc2_truck_engine`（Pitch 0.6、1000/7000 NS、fadeIn 2 s、頭から）。どちらもトラックに付いて行き、`Stop_bus` でエンジンを止めて `cc2_truck_boom` 2（1000/10000 NS）。
  - 轢かれた（車かトラック。`killPlayer(true)`）: `cc2_run_over_1` と `cc2_run_over_2` を 2D で同時に。
  - 配電盤: 使った時に `cc2_spark_1..3` 4（×2 × 波形 2、500/3600 NS）を 2 つ、`Activate_escape_seq`（`panel.escape` s）で `cc2_stage_lights` 2・Pitch 1.3（2D）。
  - 壊せる物（駅の柵の区画と板張りの通り道）: 壊れるたびに `boards_break`（原作の Hotel の板張りのバリケード `BP_01_Woodboards` の `Wooden_Boards_Breaking_v5`: 呼び出しの 1.0 × 波形の Volume 0.6、減衰 `MonkeyAttenuation` は NaturalSound で形を上書きしていないので UE の既定の 400 cm の球と 3600 cm の減衰）を区画の中心で。キューは `STAGE_SOUND` ではなく 07 記録の `world/woodboards.ts` の `BOARDS_BREAK`（同じ `StageCue` の形で `cue` が鳴らす。`Breakables.onBreak`）。
  - シャード: `STAGE_SHARD`（`cc2_shard_2/3/4` を MultiGate＝3 つを一巡するまで重複なしで、ピッチ 1 / 1.05 / 0.95、0.4 × FadeIn の 0.3 = 0.12、2D）。デバッグフィールドは従来の `shard_pickup`。
  - 敵はワサミなので、原作のマネキンの音（叫び・足音・刺す音）は使わず、敵の音は下の「敵」のまま。
- **アンビエントループ**: 全回収から欠片までのフレンジーの `frenzy_loop`（下の「敵」）だけ。以前の `amb_bass`（03_Manor の SFX_BassAmbience）、`torch_loop`（03_Manor の松明）、祭壇の `portal_loop`（00_Ballroom の ring altar の唸り）は原作の Hotel が鳴らさないのでやめ、音源も外した（2026-09-14）。原作の Hotel に置かれた環境音（厨房の空調・冷蔵庫・蒸気、エレベーターとロビーの曲）はまだ入れていない。`danger_loop` は制限時間の撤廃（2026-09-13）でどこからも鳴らさない。
- **BGM の音源**: `bgm_normal` / `bgm_chase` はホテルの対（原作の `BP_01_Hotel_MusicPlayer_DistanceBased` が交互に鳴らす `Dark_Deception_Darkness_Orch_theme_r1_071518_LOOP` / `Dark_Deception_Darkness_theme_Panic_Mode_071518_LOOP`。13 記録）。
- **ホテルの終盤の音**（game.ts が鳴らす。04 記録）: `ring_piece`、`elevator_ding` / `elevator_doors` / `elevator_move` / `elevator_stop`、`elevator_slam_1..5`、`door_breach_1`。
- **秘密と隠し扉**（game.ts が鳴らす。04・08 記録）: `secret_pickup`（0.85。秘密を取ったとき。原作の `BP_Collectable` の CreateSound2D）、`secret_revealed`（1.0。隠し扉が開くとき。原作の `BP_Openabledoor` の PlaySound2D）。どちらも定位なしのゲームの音（ポーズで止まる）。
- **足音**（原作の `Footsteps_Carpet` / `Footsteps_Marble`。間隔は 05 記録の `FOOTSTEP`）: `PlayerController.onFootstep(pitch)` → `Game.footstep(pitch)` が床材を判定し（`src/world/carpet.ts` の `CarpetMap.under`、記録 07）、絨毯（マテリアル名 `/Rug|Velvet/i`）なら `fs_carpet_1..25`、それ以外は `fs_hard_1..10` から重複なしで 1 つ選び（UE の SoundNodeRandom の既定。全部鳴らしたら補充）、`play(id, { volume: 0.875, rate: pitch × (0.9〜1.1) })`（原作の音量 0.7 × SoundCue の 1.25、ピッチはブースト中 1.5 × Modulator の 0.9〜1.1）。定位なし（原作は足元で鳴るが、減衰 `01_Lobby_Attenuation` の内側 6 m で等倍）。
- **敵**（`src/enemy/enemies.ts`、記録 15）: 足音 `enemy_fs_1..12`（`playVariant`、その敵の足元で定位、走り 1.1 / 歩き 0.75）、発見のボイス `found`（1.0）と巡回中の呼びかけ `calling` / `others` / `think` / `remember`（0.9）をその敵の頭の位置で、追跡中に近づいたときの `enemy_alert`（0.75、定位なし）。game.ts が捕まったときの `enemy_scream`（1.0）とタブレットを下ろす `tablet_down`（0.6）、全回収でフレンジーの環境音 `frenzy_loop`（music バス 0.45、fadeIn 2 s）を鳴らす。
- **ワンショット SE と音量**: `pause` 1.0（原作の UI_Pause。ポーズ画面の `PauseFx` が UI 音で鳴らす。10 記録）, `tablet_up/down` 0.6, `select` 0.4, `shard_pickup` 0.65（原作の `BP_Shard` の `PlaySound2D` の音量。キュー自身は 1.0。`playPitchVariant` で事前生成したピッチ違いの 1 つ。取得音のピッチだけを毎回わずかに変え、長さ・テンポは変えない）, `streak_v1a` / `streak_v2` / `streak_v3a` / `streak_v4` 0.7（シャードの連続回収の節目。原作の Check Streak の PlaySound2D 1.0 × SoundWave の Volume 0.7。どれを鳴らすかは hud/streak.ts の `streakSound`。10 記録）, `barrier_denied` 0.8, `gate_lever` 1.0, `gate_creak`（500 ms 遅延）1.0, `gate_rumble` 0.8, `power_boost` 0.7・`rate` 2（原作の `Shard_Streak_Milestone_V5` のアセット自体の Volume 0.7 と Pitch 2.0。UE の Pitch は再生速度でもある）, `power_not_ready` 0.35（ブースト・テレポートとも原作の値）, `power_ready` 0.5, `save` 0.35, 脱出の画面（原作の `UMG_LevelClear`。10 記録の `LevelClearFx` が UI 音で鳴らす）の `escaped` 1.0（開いてから 0.75 s）・`grade_stamp_2` 1.0（行ごと）・`grade_stamp_1` 1.0（FINAL RANK）・`xp_fill` 1.0（数え上げの間のループ）。特殊シャード（08 記録の collect-fx.ts `COLLECT_SOUND`。原作の BP_PowerOrb / BP_BonusShard とその演出の BP）: オーブの取得 `shard_pickup` 0.8・`rate` 0.75 × 0.9〜1.1（キューのモジュレーター。長さを保つピッチ違いは使わない）、`stun_countdown` 0.35（オーブの 17.3 s のカウントダウン。レベルを開き直すと止める）、`stun_wave` オーブ 1.0・`rate` 2 / 赤いシャード 0.5・`rate` 1.5、`bonus_pickup` 0.8。どれも定位なし（原作は PlaySound2D か、減衰の設定の無い PlaySoundAtLocation）。
- **ボイス＋字幕**: `Game.say(id)` が `voices` Map に id があるときだけ `play(id, { bus: 'voice' })` し、`hud.subtitle(subtitle, 'ワサミ', max(2.2, duration(id) + 1.2))` を表示。使用 id: `greeting`（開始）, `well`（1 個目回収）, `fast`（ブースト成功）, `best`（隠し扉が開いた 0.5 s 後。原作の `Bierce_Secret` の代わり。脱出では鳴らさない）。死亡画面（10 記録の `DeathFx`）が UI 音で `life_lost`（0.6）、`game_over`（0.49、1 回）、ボイス `fine`（ライフが残っているとき）と `over`（ゲームオーバー）を字幕なしで鳴らす。manifest にある `calling / others / follow / remember / think / safe / wait / you` は `say` からは使わない（`calling` / `others` / `think` / `remember` / `found` は敵が位置付きで鳴らす。記録 15）。
- **リスナー**: 毎フレーム `setListener(camera.position, player.forward, camera.upVector)`。

## 依存関係
- import: `../config`（`CONFIG.audio`）のみ。Babylon には依存しない。`stage-sounds.ts` は `Attenuation` の型だけ、`stage-music.ts` は何も import しない（テストが Node で読む）。
- 使う側: `src/game/game.ts`（唯一の利用者）。
- 外部: ブラウザ標準 Web Audio API（`AudioContext`, `GainNode`, `DynamicsCompressorNode`, `AudioBufferSourceNode`, `PannerNode`, `AudioListener`）。
- 音源ファイル: `public/sfx/*.ogg`（`scripts/copy-sfx.mjs` で Dark Deception サウンドパックからコピーし、ステージの `cc2_*` は Chaotic Customer 2 の書き出しの wav を ffmpeg で Ogg Opus に。13 記録）、`public/voices/*.mp3`。

## 設定・調整値
- `CONFIG.audio.master` 0.9, `music` 0.55, `sfx` 0.9, `voice` 1.0 — 起動時に GainNode へ反映。URL クエリで上書き可（例: `?audio.music=0.3`）。ランタイムで変更する API はない。
- `CONFIG.audio.shardPickupPitch { semitones: 1, variants: 9 }` — `shard_pickup` のピッチ違いを ±1 半音の範囲に等間隔で 9 個（−1, −0.75, …, +1 半音）作る（game.ts が参照）。値は本作のもの（原作の `Soul_Shard_Pickup_v2_Cue` は SoundNodeModulator の PitchMin 0.9〜PitchMax 1.1 で、再生速度ごと変わる。2026-09-11 の判断で長さを保つピッチ違いのままにしている）。

### ピッチシフト（pitch.ts）
- `pitchShift(channels: readonly Float32Array[], ratio): Float32Array[]` — 全周波数を `ratio`（= 2^(半音/12)）倍にし、**長さは入力と同じ**にする。`|ratio − 1| < 1e-6` ならコピーを返す。`playbackRate` / `detune` は速度も変わるので使わない。
- 手順: `stretch(channels, ratio)`（WSOLA で長さを ratio 倍に、音程はそのまま）→ Catmull-Rom 補間で ratio サンプル刻みに読み直して元の長さに戻す（音程が ratio 倍になる）。
- `stretch`（WSOLA）: グレイン `FRAME` 1024 サンプル（48 kHz で約 21 ms）、周期 Hann 窓、合成ホップ `HOP` 512（50% 重なりで窓の和がちょうど 1）、分析側の名目位置 `round(k*HOP/factor)`。k ≥ 1 では名目位置の ±`SEEK` 256 サンプルを探し、前のグレインの自然な続き（前の中心 + HOP）の前半 512 サンプルとの正規化相互相関（2 サンプルおきに計算）が最大の位置を選ぶ。探索はモノラル合成で行い、同じ位置を全チャンネルに使う。グレイン 0 は入力の 0 サンプル目を中心に置くのでアタックが崩れない。入力範囲外は 0 として読む。
- コスト: `shard_pickup`（0.44 s、21,012 フレーム × 2ch）の 9 個で約 117 ms（Node 実測）。読み込み中（「サウンドをデコードしています…」の段階）に一度だけ行う。
- `tests/stage-music.test.ts`: プラットフォームでは無音で、トリガーの 0.6 s 後に Carol（fadeIn 2・fadeOut 1・0.7・頭から）、追跡で Dethsmass と戻り、全回収で 2 s で消えて戻らない（開き直しても）、脱出の 1.5 s で Merry_horrors（6 s から 0.1 s）と PARKOVKA が 1 回、`parkovkaDelay` が 10〜18 s。
- `tests/pitch.test.ts`: 440 Hz 正弦波（0.5 s）を ±0.5 / ±1 半音ずらし、長さが同じ・中央半分の周波数（ゼロ交差の補間）が目標の ±0.5% 以内・RMS が ±10% 以内。減衰する 2ch の音（1200 / 900 Hz）を +1 半音ずらし、先頭 96 サンプルのピークが元の 0.8 倍以上・2ch 目の周波数が ±1% 以内。ratio 1 は同一内容の別配列。
- コンプレッサ閾値 −10 dB / 比 3、パンナーの距離パラメータ、ピッチ揺らぎ幅、フェード既定値（loop 0.5 / stop 0.6 / setMusic 1.5）はコード内定数。

## 既知の制約・注意点
- Babylon.js の `AudioEngine`/`Sound`（v1）や Audio Engine v2 は使っていない。`scene` にオーディオ関連の設定は不要。
- ポーズ中は `setPaused(true)` でゲームの文脈を suspend し、BGM・環境音・効果音・予約済みの音をその位置で止める（再開で続きから）。UI の文脈（タイトルとポーズ画面の音。`ui: true`）は鳴り続ける。原作の SetGamePaused は UI 音（Widget の PlaySound2D / CreateSound2D）以外を一時停止し、館の BGM は `BP_03_01_MusicPlayer` の AudioComponent（UI 音ではない）なので止まる。終了画面（`endRun`）はゲームを止めても音は止めない（`life_lost` などを鳴らすため）。
- ゲーム中の UI 寄りの音（タブレットの出し入れ、ミニマップの `select` など）はゲームの文脈で鳴らす（ポーズ中は鳴らないので区別しない）。
- `play()` で生成したソースは追跡していないため、個別停止・全停止の手段がない（ループは `LoopHandle` 経由で停止可）。
- 同一 id を短時間に多重再生しても制限しない。
- NaturalSound の減衰は指数の近似（端で −60 dB）。UE の曲線と途中の形は一致しない。
- ステージの音の音量は原作の値そのまま（配電盤の火花は 4、遮断機・トラックは 2）。1 を超える分はコンプレッサが抑える。
- `decode` は `data.slice(0)` でコピーしてから渡すため、`AssetStore` 側のバッファは消費されない。
- `voices/manifest.json` の `url` は先頭 `/` 付き絶対パスだが、実際の読み込みは `assets/manifest.json` の相対 URL 経由で行われるため、この `url` 値は使われない。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: `shard_pickup` を毎回 `rate` 0.94〜1.06 のランダムなピッチで鳴らすようにした（`CONFIG.audio.shardPickupPitch`）
- 2026-09-11: 再生速度ではなくピッチだけを変えるように変更。`pitch.ts`（WSOLA + リサンプルで長さを保つピッチシフト）と `AudioManager.preparePitchVariants` / `playPitchVariant` を追加し、`shardPickupPitch` を `{ semitones: 1, variants: 9 }` に変更
- 2026-09-11: 1 回きりの音にも位置を付けられるようにした（`PlayOptions.position`、`playVariant` の `position`、パンナーの設定を `spatial()` にまとめた）。敵の音の使い方を追記
- 2026-09-12: `power_boost` を原作の `Shard_Streak_Milestone_V5` の 1.0（以前 `Stun_Wave_Attack_New_04` の 0.8）、ブーストの `power_not_ready` を 0.35（以前 0.6）にした
- 2026-09-12: ポーズ中の音を原作に倣った: AudioContext をゲーム用と UI 用に分け（`PlayOptions.ui`、`mix()`）、`setPaused` でゲームの音だけをその位置で止めるようにした（以前はポーズ中も BGM・ループが鳴り続けた）。`suspend()` と公開の `ctx` を削除し、`states()` を追加した
- 2026-09-12: `power_boost` を音量 0.7・`rate` 2 にした（原作の SoundWave に設定された Volume と Pitch）
- 2026-09-12: タイトル画面の音を原作どおりに鳴らすため、`PlayOptions` に `delay` と `envelope`（音量の折れ線）、`loop` に `rate` と `offset` を追加した。`Game.start` の SE `start`（0.7）をやめた（NEW GAME の暗転でタイトルが鳴らす。10 記録）
- 2026-09-12: 設定の音量（タイトルの OPTIONS の MUSIC / SFX / DIALOGUE）を各バスに掛ける `setVolumes` と、デバッグ用の `busGains` を追加した
- 2026-09-12: プレイヤーの足音を原作どおりにした: `fs_carpet_` を 25 種に、音量 0.875・再生速度 pitch × 0.9〜1.1（ブースト中 1.5）、重複なしの抽選（`playVariant` は使わない）。未参照だった `audio.footstepInterval` を削除
- 2026-09-13: 制限時間の撤廃に合わせ、残り 30 秒の `danger_loop` とボイス `wait`、時間切れのボイス `over` と `life_lost` を鳴らさなくなった（04 記録）
- 2026-09-13: `loop` に `repeat`（false で 1 回だけ鳴らす。開始は先頭から）を足した（死亡画面のゲームオーバーの曲。10 記録）
- 2026-09-13: 捕獲の音を原作の死亡の流れに合わせた: 捕まったときのボイス `you` と `life_lost` をやめ（`life_lost` は死亡画面が 0.6 で鳴らす）、タブレットを下ろす `tablet_down` を足した（04・10 記録）
- 2026-09-13: 原作の Hotel に置き換えた: BGM の対をホテルのものに、`torch_loop` を鳴らさない（電灯）、`portal_loop` を祭壇に、ホテルの終盤の音を足した（04・13 記録。このファイルのコードは変更なし）
- 2026-09-13: 10 個ごとの `shard_streak` 0.5 をやめ、シャードの連続回収の節目の `streak_v1a` / `streak_v2` / `streak_v3a` / `streak_v4` 0.7 にした（04・10・13 記録。このファイルのコードは変更なし）
- 2026-09-13: 特殊シャードの音（`bonus_pickup`・`stun_countdown`・`stun_wave`、オーブの `shard_pickup`）を足した（04・08・13 記録。このファイルのコードは変更なし）
- 2026-09-14: 脱出の音を原作の `UMG_LevelClear` のもの（`escaped`・`grade_stamp_1` / `grade_stamp_2`・`xp_fill`）にし、原作が使わない `level_complete` をやめた。脱出では BGM をフェードせず `setPaused(true)` で止める（04・10・13 記録。このファイルのコードは変更なし）
- 2026-09-14: 原作の Hotel の秘密と隠し扉の音（`secret_pickup` 0.85・`secret_revealed` 1.0）と、隠し扉の台詞に使うボイス `best` を記した（04・08・13 記録。このファイルのコードは変更なし）
- 2026-09-14: 原作の Hotel が鳴らさない音をやめ、音源も外した: `amb_bass`・`torch_loop`・`portal_loop`・`sweetener_1..3`・`barrier_success`・`portal_unlocked`・`breath`。全回収の `found` もやめた（`say` の使用 id から外した）。`frenzy_loop` の fadeIn の記述を実装どおり 2 s に直した
- 2026-09-14: `shard_pickup` の音量を原作の `BP_Shard` の `PlaySound2D` の 0.65 にした（04・08 記録。このファイルのコードは変更なし）
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 にしたのに合わせ、その音を入れた: 音ごとの減衰（`Attenuation`、UE の Linear と NaturalSound）、`setMusic` の `fadeOut` と `offset`、`musicId`、`LoopHandle.moveTo` を足し、ステージの曲の状態機械（`stage-music.ts`）と効果音の表（`stage-sounds.ts`）を置いた。ステージではデバッグフィールドの BGM の対を鳴らさない（04・13 記録）。「HRTF パンナーはループにしか付かない」という古い記述を消した（1 回きりの音にも付く）
- 2026-09-15: `STAGE_SOUND` に駅の柵の `woodBreak`（`cc2_wood_break_1..2`、1000/8000 NS）としゃがみの `crouch`（`cc2_crouch_1..2`、2D）を足した（04・05・07・13 記録）
- 2026-09-15: `STAGE_SOUND` に敵へのパンチの `mannequinHit`（`cc2_mannequin_hit_1..3`、2、1000/6000 NS）と倒れる `mannequinFall`（`cc2_mannequin_fall`、2.4、1000/4500 NS）を足した（04・13・15 記録）
- 2026-09-15: `STAGE_SOUND` にシャードの障壁が現れる `shardBarrierAppear`（`stun_wave`、1、550/3600 NS）を足した（04 記録）
- 2026-09-15: パンチと駅の柵のファンゲームの音をやめた: `STAGE_SOUND` の `woodBreak`・`mannequinHit`・`mannequinFall` を削除し、壊せる物が壊れる音を原作の `Wooden_Boards_Breaking_v5`（`boards_break`、0.6、UE の既定の 400/3600 NS。07 記録の woodboards.ts の `BOARDS_BREAK`）にした（04・07・13 記録）
- 2026-09-15: ファンゲームのしゃがみをやめたので（本作の規範。05 記録）、`STAGE_SOUND` の `crouch`（`cc2_crouch_1..2`、2D）を削除した（04・13 記録）
