---
title: アセットパイプライン（ステージのテクスチャ・KTX2・glTF 組立・vendor・SFX）
sources:
  - scripts/encode-ktx2.mjs
  - scripts/build-level.mjs
  - scripts/vendor-babylon.mjs
  - scripts/copy-sfx.mjs
  - scripts/prepare-shard-model.mjs
  - scripts/prepare-enemy-model.mjs
  - scripts/prepare-boost-fx.mjs
  - scripts/prepare-title-assets.mjs
  - scripts/prepare-pause-heads.mjs
  - scripts/prepare-cc2-textures.mjs
  - scripts/prepare-special-shards.mjs
  - tests/shard-model.test.ts
  - tests/enemy-model.test.ts
  - tests/ktx2-textures.test.ts
updated: 2026-09-15
---

# アセットパイプライン（テクスチャ取得・KTX2・glTF 組立・vendor・SFX・シャードモデル・敵モデル）

## 役割
ステージ（Chaotic Customer 2 の Zone_1。2026-09-14 から。以前は原作の Hotel、12.）のテクスチャを KTX2（UASTC）にしてマテリアル表を作り（14.）、ベイクしたライトマップを KTX2 にし（1.）、Blender が出した `level.glb` にマテリアル表どおり KTX2 テクスチャを割り当てて `public/assets/level/level.gltf` を組み立て、実行時ローダーが使う各 manifest を生成する（2.）。加えて KTX2 デコーダの同梱、効果音のコピー、ソウルシャードの見た目に使うワサミ餅モデルの簡略化、敵（ワサミ）のモデルの軽量化を行う。生成物は git に同梱済みで、作り直すときだけ実行する。

## 公開インターフェース
すべて Node ESM スクリプト（`node scripts/<name>.mjs`）。`package.json` の scripts:

| npm script | 実体 | 主なオプション |
| --- | --- | --- |
| `assets:cc2-tex` | `prepare-cc2-textures.mjs` | `--force`, `--jobs N`（既定 4）, `--preview`、環境変数 `CC2_REF` |
| `assets:ktx2` | `encode-ktx2.mjs`（ライトマップ） | `--effort N`（既定 3）, `--force` |
| `assets:level` | `build-level.mjs` | `--src DIR`（既定 `assets-src/level`） |
| `assets:vendor` | `vendor-babylon.mjs` | `--force` |
| `assets:sfx` | `copy-sfx.mjs` | 環境変数 `SFX_SRC`（既定 `<repo>/game_sound_effects`）と `CC2_REF`（既定 `<repo>/cc2_reference`）、`ffmpeg`（libopus） |
| `assets:shard` | `prepare-shard-model.mjs` | 第 1 引数 `*.glb`（既定 `models/wasami_mochi.glb`）, `--tris N`（既定 6000）, `--tex N`（既定 1024） |
| `assets:enemy` | `prepare-enemy-model.mjs` | 第 1 引数 `*.glb`（既定 `legacy/models/wasami-enemy.glb`）, `--tex N`（既定 1024） |
| `assets:boostfx` | `prepare-boost-fx.mjs` | 環境変数 `PAK_REF`（既定 `<repo>/pak_reference`） |
| `assets:title` | `prepare-title-assets.mjs` | 環境変数 `PAK_REF`（既定 `<repo>/pak_reference`） |
| `assets:pause` | `prepare-pause-heads.mjs` | なし（入力は `assets-src/pause/wawa.png`） |
| `assets:specials` | `prepare-special-shards.mjs` | 環境変数 `PAK_REF`（既定 `<repo>/pak_reference`） |
| `assets:all` | cc2-layout → cc2-tex → cc2-luts → cc2-bake → cc2-colliders → ktx2 → level → vendor（sfx は含まない。cc2_reference が要る） | — |

`copy-sfx.mjs` は `SFX`（id → 元ファイル相対パス）と `CC2_SFX`（id → Chaotic Customer 2 の書き出しの `Content/` からの wav の相対パス）、`prepare-cc2-textures.mjs` は `REPLACE`（差し替えるテクスチャ → 置き換える箱）を export する。

## 内部構造と処理の流れ

### 1. ライトマップの KTX2（`scripts/encode-ktx2.mjs`）
- 前提: `basisu` CLI（`brew install basis_universal`）と `sharp`。`node scripts/encode-ktx2.mjs [--force] [--effort N]`（既定 3）。
- ライトマップのページ `assets-src/level/lightmap_<n>.png` と `lightmap_indirect_<n>.png`（12 記録の `build_cc2.py` のベイク。ページが無ければ警告）を `public/assets/level/<name>.ktx2` へ。出力先の `lightmap*.ktx2` のうち、いまのページに無いもの（Hotel の 1 枚のライトマップ、減ったページ）は消す。符号化は `basisu -uastc -ktx2 -mipmap -quality 70 -effort <effort> -linear`（ベイク側でガンマ 2.2 に符号化済みなので linear 扱い。実行時は `Texture.gammaSpace = true` と `level = lightmapScale × intensity` で復号する。07 記録）。出力が入力より新しければ飛ばす。寸法が 4 の倍数でなければエラー（WebGPU は 4×4 ブロック圧縮〈ASTC / BC7〉のテクスチャを寸法が 4 の倍数でないと作れず、そのテクスチャを使うフレームの描画がまるごと捨てられる。WebGL2 は受け付けるので WebGPU だけで起きる）。Hotel のときは 1 枚ずつ 4096² で 7.2 MB / 6.5 MB だった。
- レベルのテクスチャは `prepare-cc2-textures.mjs`（14.。Hotel のときは `prepare-hotel-textures.mjs`。2026-09-14 に削除）。以前の Poly Haven / ambientCG の 4K の取得（`fetch-textures.mjs` / `texture-sets.mjs`）と `public/assets/textures/`（172 MB）は、原作の Hotel への置き換えで 2026-09-13 に削除した（ユーザーの指示。反射の HDRI `public/assets/env/ballroom_1k.hdr` は残す）。
- `tests/ktx2-textures.test.ts` が生成物を検査する: `public/assets/` 以下のすべての KTX2（識別子を確認）の幅・高さが 4 の倍数、`assets-src/level/materials.json` の各テクスチャ（`baseColor` / `normal` / `orm` / `emissive`）が `assets/cc2/tex/*.ktx2` にあって一辺 2048 以下（参照が 100 件より多い）。


### 2. レベル組立（`scripts/build-level.mjs`）
1. `readGlb` で `assets-src/level/level.glb`（12 記録の `build_cc2.py`）を JSON チャンク（`0x4e4f534a`）と BIN チャンク（`0x004e4942`）に分解し、`stripColours(json, bin)` で頂点の色（`COLOR_n`。ファンゲームのメッシュの頂点の塗りと Blender の属性。実行時の材質は使わず、Babylon は COLOR_0 をベースの色に掛けてしまう）を外して、まだ参照されている accessor と bufferView だけを 4 バイト境界で詰め直す（ステージの試しの組み立てで 147 MB → 120 MB）。`assets-src/level/materials.json`（`materials`。14.）を読む。
2. `splitBin(json, bin)`: BIN チャンクを bufferView の境目で `BIN_PART_BYTES`(16 MiB)以下の連続した区間に分け、`json.buffers = [{ uri: 'level-<n>.bin', byteLength }, …]` にして、各 bufferView の `buffer` と `byteOffset` をその区間に付け替える(頂点・インデックスのバイトはそのまま)。Cloudflare Pages が 25 MiB を超えるファイルを受け付けないため(2026-09-12 に 1 本の `level.bin` 35.3 MiB でデプロイが失敗した)。bufferView が buffer 0 以外にある、1 つで `BIN_PART_BYTES` を超える、区間の先頭が 4 バイト境界でない、のいずれかならエラー。`images`/`textures` を空にし、`samplers = [{ magFilter 9729 (LINEAR), minFilter 9987 (LINEAR_MIPMAP_LINEAR), wrapS/T 10497 (REPEAT) }]`、`extensionsUsed` と `extensionsRequired` に `KHR_texture_basisu` を追加。
3. `texture(url)`: `public/<url>` の存在を確認（無ければ「run node scripts/prepare-cc2-textures.mjs」エラー）、`images` に `{ uri: assets/level からの相対（'../cc2/tex/<フォルダー>__<名前>.<kind>.ktx2'）, mimeType: 'image/ktx2', name }`、`textures` に `{ sampler: 0, name, extensions: { KHR_texture_basisu: { source } } }`（同じ URL は 1 つにまとめる）。
4. glTF の各マテリアルを名前（UE のマテリアル名。Blender が付ける）で `materials.json` から引き、`pbrMetallicRoughness`（`baseColorFactor` / `metallicFactor` / `roughnessFactor`、`baseColorTexture`、ORM があれば `metallicRoughnessTexture` と `occlusionTexture`（strength 1））、`normalTexture`（scale 1）、`emissiveFactor` と `emissiveTexture`、強さが 1 でなく発光があれば `KHR_materials_emissive_strength`、`unlit` なら `KHR_materials_unlit`（どちらも `extensionsUsed` に足す）、`alphaMode`（MASK なら `alphaCutoff`）、`doubleSided`、`extras.ue`（名前）と、灯の発光面なら `extras.glow` を書く。表に無いマテリアル（`M_Default` 以外）は警告。マテリアルの名前はステージでは `<UE のマテリアル名>@<フォルダー>`（12 記録の layout.py の鍵）。Hotel のときは 83 マテリアル、151 テクスチャだった。
5. `public/assets/level/` の古い `level.bin` / `level-<n>.bin` を消してから `level-<n>.bin` と `level.gltf`（minified JSON）を書き、`level-meta.json`・`colliders.json`・`stage.json`（12 記録の layout.py の目印とギミック）をコピーし（無ければ警告）、Hotel の `hotel.json` が残っていれば消す（`src/world/level.ts` が読む。07 記録）。`public/assets/level/captures/` を消してから、`assets-src/level/captures/*.hdr`（原作の反射キャプチャの全天球。12 記録）があればそこへコピーする。現行は 3 本（`level-0.bin` 16.7 MB / `level-1.bin` 16.7 MB / `level-2.bin` 2.0 MB）。
6. **`public/assets/manifest.json`** を生成: `{ generated: ISO 日時, totalBytes, files: [{ url, bytes, kind }] }`。`kind` は `level`（`level-<n>.bin` を buffers の順に）、`texture`（glTF の images）、`lightmap`（ページ `lightmap_<n>.ktx2` / `lightmap_indirect_<n>.ktx2` をページの順に）、`env`（反射キャプチャ `assets/level/captures/*.hdr` 12 枚を名前順に、と `public/assets/env/*` 全部）、`model`（`public/assets/models/*.glb`。ディレクトリが無ければ飛ばす）、`audio`（`public/sfx/*.ogg` と `public/voices/*.mp3`）。現行 442 ファイル、402,808,475 バイト（約 384.1 MiB。ステージ: level 122.0 MB〈`level-0..7.bin`、最大 16.8 MB〉・texture 241.0 MB・lightmap 20.5 MB・env 2.5 MB・model 2.2 MB・audio 14.6 MB。2026-09-14 のマテリアルの直しと焼き直しの後。原作の Hotel のときは約 137.6 MiB）。`level.gltf`・`colliders.json`・`stage.json` は manifest に含まれない（fetch で読む）。
7. 集計ログ（node 数、マテリアル数、三角形数 = indices count/3 の合計、テクスチャ数、manifest のファイル数と MB）。

### 5. KTX2 デコーダ同梱（`scripts/vendor-babylon.mjs`）
`public/vendor/ktx2/` に、`node_modules/babylonjs-ktx2decoder/babylon.ktx2Decoder.js` と `license.md`（→ `LICENSE.md`）をコピーし、`https://cdn.babylonjs.com` から以下を取得（存在すればスキップ、`--force` で再取得）: `ktx2Transcoders/1/uastc_astc.wasm`, `uastc_bc7.wasm`, `uastc_rgba8_unorm_v2.wasm`, `uastc_rgba8_srgb_v2.wasm`, `uastc_r8_unorm.wasm`, `uastc_rg8_unorm.wasm`, `msc_basis_transcoder.js`, `msc_basis_transcoder.wasm`, `zstddec.wasm`。すべてベース名で保存。実行時に CDN へ出ないための措置（`smoke.mjs` が外部リクエスト数を数える）。

### 6. 効果音コピー（`scripts/copy-sfx.mjs`）
- 原作の Hotel のための追加（2026-09-13、88 → 100 件）: BGM の対 `bgm_normal` / `bgm_chase` を原作のホテルの `01_Hotel/Dark_Deception_Darkness_Orch_theme_r1_071518_LOOP.ogg` / `…Darkness_theme_Panic_Mode_071518_LOOP.ogg`（原作の `BP_01_Hotel_MusicPlayer_DistanceBased` が交互に鳴らす対。以前は Manor の Normal / Panic Track）に替え、エレベーター `elevator_ding` / `elevator_doors` / `elevator_move` / `elevator_stop`（`01_Hotel/DD_ElevatorDing` / `DoorSliding` / `MovingLoop` / `Stopping`）、`elevator_slam_1..5`（`20-Elevator_Slams_V1..5`）、扉を破る `door_breach_1` / `door_breach_2`（`18-Wood_Door_Breach_Destroy_FromOutside_V1` / `V5`。原作のレベル BP が参照する）、欠片 `ring_piece`（`RingStatue/Ring_Piece_Pickup_v1`）を足した。
- `SFX` マップ（100 件。プレイヤーの足音は原作の SoundCue の全部: `fs_hard_1..10` = `Footsteps/Marble/03_FS_Marble_Sneaker_v*_1.ogg`、`fs_carpet_1..25` = `Footsteps/Carpet/FOL_FS_01_Carpet_v*.ogg`）: 論理 id → 音源パック内の相対パス。BGM（`title_theme`, `bgm_normal`, `bgm_chase`）、ゲーム音（`shard_pickup`, 特殊シャード（`bonus_pickup` = `SharedGameplay/Bonus_Shard_Pickup_v1.ogg`（赤いシャードの取得音、0.43 s）、`stun_countdown` = `SharedGameplay/8-Dark_power_ball_countdown_.ogg`（オーブの取得で鳴る 17.3 s の音）、`stun_wave` = `SharedGameplay/Stun_Wave_Attack_New_04.ogg`（どちらの取得の演出も鳴らす波の音、1.71 s）。オーブの取得音は `shard_pickup`）, `streak_v1a/v2/v3a/v4`（= `UI/Shard_Streak_Milestone_V1A/V2/V3A/V4.ogg`。原作の `BP_DD_GameMode` の Check Streak がシャード連続回収の節目で鳴らす音。20・50 が V1A、100〜200 が V2、250・350 が V3A、500〜1000 が V4）, `barrier_denied`, `gate_lever/creak/rumble`, 原作の脱出の画面 `UMG_LevelClear` が鳴らすもの（`escaped` = `UI/UI_YouEscaped.ogg`（ClearAnimation の 0.75 s、3.64 s）、`grade_stamp_1` / `grade_stamp_2` = `UI/Level_Clear_Grade_Stamp_v1.ogg` / `_v2.ogg`（Final Rank Animation / 各行のアニメのランクの判の音、0.67 s / 0.55 s）、`xp_fill` = `UI/UI_XP_Bar_Fill_V2A_0617.ogg`（行の加算シャードを数え上げる間の 4.18 s のループ））, `life_lost`, `game_over`（= `SharedGameplay/66_-_Game_Over.ogg`。原作の死亡画面がライフ 0 で 1 回鳴らす曲、41 s）, `tablet_up/down`, `power_boost/ready/not_ready`。`power_boost` = `UI/Shard_Streak_Milestone_V5.ogg` は原作のスピードブーストの発動音で、pak_reference の同名のファイルと同一（中身は V4 と同じで、原作はアセットに Pitch 2.0・Volume 0.7 を設定して鳴らす。再生の速さと音量は game.ts が掛ける））、テレポート（`teleport_aim` = `03_Manor/DD_LVL2_07_Teleport_Aiming_Loop_1227.ogg` の照準ループ 3.73 s、`teleport` = `Engine/VREditor/Sounds/UI/Teleport_Committed.ogg`（原作の確定音、1.54 s）、`teleport_enter` = `Engine/VREditor/Sounds/UI/Teleport_Mode_Entered.ogg`（原作の照準開始の音、1.96 s）。この 2 つは原作が使う UE エンジン同梱の音で、音源パックには無いので、pak_reference の `Engine/Content/VREditor/Sounds/UI/` から音源パックの `Engine/VREditor/Sounds/UI/` へコピーしてある。ファイル名の LVL2 は館のステージ）、UI（`pause`, `select`, `start`, `save`）、タイトル画面（原作の `UMG_TitleScreen` / `UMG_PopUp` が鳴らすもの。`title_music` = `UI/Pause_Sound_v1.ogg`（ポーズのループ。原作のタイトルはこれを再生速度 0.5 で鳴らす）、`ui_select` = `UI/UI_Select_V3.ogg`（メニューのクリック）、`ui_popup` = `UI/UI_Window_PopUp_V3.ogg`（RESTART? が開く音）、`title_voice` = `Titlescreen/Bierce_Title_Modified_03.ogg`（NEW GAME の暗転中のビアスの声、2.53 s））、足音 `fs_hard_1..10`（Marble/Sneaker）と `fs_carpet_1..8`。壊せる物（07 記録の breakables.ts。駅の柵の区画）が壊れる音 `boards_break` = `01_Hotel/Wooden_Boards_Breaking_v5.ogg`（原作の Hotel の板張りのバリケード `BP_01_Woodboards` が鳴らす音）。
- 敵の音（原作の Murder Monkeys のもの、`01_Hotel/` と `Footsteps/Enemies/Monkey/`）: `enemy_alert` = `Toy_Monkey_CloseProximityAlert.ogg`（追跡中に近づいたときの警告）、`enemy_scream` = `Evil_Monkey_Scream.ogg`（捕まったとき）、`frenzy_loop` = `23-Frenzy_Monkeys_Ambient_LOOP.ogg`（全回収後の環境音）、足音 `enemy_fs_1..12` = `Monkey_FS_Wood_01..12_1.ogg`。
- 各ファイルを `public/sfx/<id>.ogg` にコピーし、`public/sfx/manifest.json` = `{ <id>: { url: 'sfx/<id>.ogg', source: '<元の相対パス>' } }` を書く。音源パックが無ければ exit 1。
- ステージの音 `CC2_SFX`（37 件、id は `cc2_`。どこで何を鳴らすかは 06 記録）: 書き出し（`CC2_REF`、既定 `cc2_reference/Chaotic_Customer_2/Content/`）の 16-bit PCM の wav を `ffmpeg -c:a libopus -b:a 128k -ar 48000` で `public/sfx/<id>.ogg`（Ogg Opus）にする。この Mac の ffmpeg には libvorbis が無い（Vorbis はネイティブの実験的なエンコーダだけ）ので Opus。出力が wav より新しければ変換を飛ばす。manifest の `source` は `cc2:<相対パス>`。書き出しが無ければ exit 1。内訳: 曲 4（`cc2_carol` = `Content_game/Audio/Soundtrack/Act_1_Soundtrack/CC_Carol_Of_the_bells.wav`、`cc2_dethsmass` = `…/CC_Merry_Dethsmass.wav`、`cc2_horrors` = `…/CC_Merry_horrors.wav`、`cc2_parkovka` = `Content_game/Audio/PARKOVKA_LOMAETCA.wav`）、障壁 2（`Content_game/Barrier/Barrier_Loop.wav`、`Content_game/Audio/Barrier_Shatter.wav`）、扉 15（`Content_game/models/Zone_1/Door/Locked_Door_v1.wav`・`Gym_Door_Open_v1..3.wav`・`Gym_Door_Close_v1..3.wav`、`Content_game/Audio/vent1.wav`・`vent2.wav`・`Garage_Button_SFX.wav`・`GarageDoor_open_SFX.wav`・`Gazel_2_Crash_1.wav`・`Gazel_1_Crash.wav`、`Content_game/models/Blocker/Blocker_Sound.wav`、`Content_game/Characters_Chaotic_Customer/manequin/18-Wood_Door_Breach_Destroy_FromOutside_V1.wav`）、罠とトラック 9（`Content_game/Audio/` の `PAR`・`Car_SFX_N_mute`・`Car_SFX_N_mute_2`・`Car_horn`・`RL_bodyfall_Dirt_M4_Close_Stereo_Hard_Impact_10`・`Lucky_Punch_2`・`Beep_beep1`・`Beep_beep_2`・`Gazel_boom`）、配電盤 4（`Content_game/Audio/Unused_Soundtracks/broken_lamp_spark_1..3.wav`、`Content_game/Audio/DD_LVL2_06_Stage_Lights_1_102918.wav`）、シャード 3（`Content_game/Audio/Soul_Shard_Pickup_v2..4.wav`）。計 6.4 MB。

### 7. シャードモデル前処理（`scripts/prepare-shard-model.mjs`）
- 入力: `models/wasami_mochi.glb`（git 管理外の元素材。約 300 万三角形、頂点 約 153 万、2048² JPEG 3 枚（baseColor / metallicRoughness / normal）、約 89.9 MB、sha256 `6b071c1d…`）。単一メッシュ・単一三角形プリミティブ・スキン/アニメ無しでなければエラー。
- 読み出し: GLB を JSON / BIN チャンクに分け、`POSITION` / `NORMAL` / `TEXCOORD_0` / indices をアクセサから取り出す（Float32 / Uint32 / Uint16、stride 無しのみ）。
- 簡略化: `MeshoptSimplifier.simplifyWithAttributes(indices, positions, 3, [normal, uv], 5, weights [0.1, 0.1, 0.1, 0.5, 0.5], null, TARGET_TRIS*3, 0.008, ['Prune', 'Permissive'])`。UV に重みを置いてテクスチャの継ぎ目を保つ。300 index 未満になったらエラー。
- 使われなくなった頂点を詰め直し（自前の remap）、位置を AABB 中心で原点にそろえ、最大辺が **1 m** になるよう等倍スケールする（法線はそのまま）。実行時は `CONFIG.game.shard.size` で拡大縮小する。
- テクスチャ: 3 枚とも `sharp().resize(TEX, TEX, { kernel: 'lanczos3', fit: 'fill' }).jpeg({ quality: 85, mozjpeg: true })` で縮小して埋め込む。マテリアル・サンプラー・ノードは元のまま。
- 出力 `public/assets/models/wasami_mochi.glb`: 頂点 65536 未満なら Uint16 インデックス。`asset.extras` に `{ source, sourceSHA256, sourceTriangles, simplificationError, textureSize }`。現行は 6000 三角形 / 4315 頂点 / 誤差 0.0019 / 1024² / 309,540 bytes。
- 実行後は `npm run assets:level` で `public/assets/manifest.json` の `model` エントリを更新する（`assets:all` には含まれない。元素材がない環境でも `assets:all` が通るように）。
- `tests/shard-model.test.ts` が生成物を検査する: GLB ヘッダ長の一致、2,000,000 bytes 未満、`extensionsRequired` 無し、メッシュ 1 個、6000 三角形以下、`POSITION` の min/max から最大辺 1 ± 1e-4 と中心が原点、画像がすべて埋め込み（`uri` 無し）、baseColor と normal テクスチャがある。

### 8. 敵モデル前処理（`scripts/prepare-enemy-model.mjs`）
- 入力: `legacy/models/wasami-enemy.glb`（旧 Three.js 版の敵。Meshy のワサミのメッシュ 27,416 三角形 + 24 関節のスキンに、`idle` / `walk` / `run` / `skip` / `spin` の 5 クリップを旧版の `legacy/scripts/prepare-enemy.mjs` がまとめたもの。2048² PNG 1 枚（baseColor と emissive が共有）、7.6 MB。バインドポーズの高さ 1.7 m、`Armature` の拡縮 0.01 で関節は cm 単位）。スキン 1 個・バッファ 1 個・5 クリップがそろっていなければエラー。
- Hips の水平移動の除去: 全クリップの `Hips` の translation キーの x / z を、ノードの静止時の translation の x / z に書き換える（上下の動きは残す）。LINEAR / STEP の VEC3 float、stride 無しでなければエラー。アクセサの min / max も合わせる。実行時は位置をナビゲーションが決めるので、クリップ自体の前進（`spin` は約 2.6 m）で体がずれないようにする。現行 290 キー。
- テクスチャ: 埋め込み画像を `sharp().resize(TEX, TEX, { kernel: 'lanczos3', fit: 'fill' }).jpeg({ quality: 85, mozjpeg: true })` にし、`mimeType` を `image/jpeg` に。
- BIN をバッファビューごとに（4 バイト境界で）組み直して書き出す。メッシュ・スキン・マテリアル（emissive 付き、`KHR_materials_specular` / `KHR_materials_ior`）・ノードは元のまま。`asset.extras` に `{ source, sourceSHA256, clips, textureSize, hipsTranslationKeysFlattened }`。
- 出力 `public/assets/models/wasami_enemy.glb`（現行 1.84 MB）。実行後は `npm run assets:level` で manifest の `model` エントリを更新する（`assets:all` には含まれない）。
- `tests/enemy-model.test.ts` が生成物を検査する: GLB ヘッダ長の一致、4,000,000 bytes 未満、`extensionsRequired` 無し、スキン 1 個・関節 24、クリップ名と順序、画像が埋め込みの JPEG、5 クリップすべての Hips の translation キーの x / z が静止値と一致。

### 9. スピードブーストの画面と壊せる物の煙の素材（`scripts/prepare-boost-fx.mjs`）
- 原作（pak_reference。git の対象外）の 4 枚を `public/fx/` の WebP（quality 90・alphaQuality 100、lanczos3 で `size` に）にする。`BOOST_FX`（`{ from, to, size, channel }[]`）を export する。`channel` が `'alpha'` / `'red'` の 3 枚はシェーダが読む 1 チャンネルだけのグレースケール、`'rgba'` の煙は色と α のまま。
  - `speedlines.webp` 1920×2700: `UI/Main/Powers/T_Speedlines.png`（3841×5404、1920×1080 のコマが 2 列 × 5 段のフリップブック）の α。コマは 960×540 になる（UE の FlipBook と同じく等分するので、縦横を別々に縮めてよい）。
  - `vignette.webp` 256×256: `UI/Menu/Streaks/T_VignetteNew.png`（1024²、中央が透明で縁が白の角丸）の α。
  - `radial-mask.webp` 256×256: `Chameleon/Materials/T_RadialBlurMask.png`（2048²、中央が黒の角丸）の R。
  - `smoke-subuv.webp` 1024×1024（`'rgba'`、416 KB）: `StarterContent/Textures/T_Smoke_SubUV.png`（1024²。8 × 8 の煙の SubUV で、原作の取り込み 2048 と LODBias 1 のゲームの大きさ = 1 コマ 128 px）。原作の板張りのバリケードが壊れるときに出す `P_Explosion1` の材質 `M_smoke_subUV` のテクスチャ。
- 使う側は 09 記録（`postfx.ts` のブーストのパス）と 07 記録（煙は breakables.ts の `SpriteManager`）。manifest には入れず、`Texture` / `SpriteManager` が直接読む。
- pak_reference が無ければ「pak_reference not found」で終了（生成物は git に同梱済み）。

### 10. タイトル画面の素材（`scripts/prepare-title-assets.mjs`）
- 原作（pak_reference）の UI テクスチャのうち、ロゴとキャラクターを含まないものだけを `public/title/` の WebP（quality 90、alphaQuality 100、lanczos3）にする。原作ロゴ `title_screen_logo` と敵の顔 `title_screen_profile_*`、ポーズ画面の怪物の頭 `pause_*_head` / `quit_window_head_*`、ステージのタイトルカードの題字 `chapter_ui_title_*` は使わない。`TITLE_ASSETS`、`WISPS`、`DEATH_VIGNETTE`、`VIGNETTE_SIDES`、`STAGE_TITLE`、`FONTS` を export する。ステージのタイトルカードの題字だけはユーザー提供の画像（`assets-src/stage/title.png`）から作る。
- 1 色だけのもの（`TITLE_ASSETS` の `tone: 'white'`、`WISPS`、`DEATH_VIGNETTE`、`VIGNETTE_SIDES`、`STAGE_TITLE`）は白に α を付けて形だけを持たせ、黒地に赤のもの（`tone` に色の値 c）は赤の割合を灰（R × 255 / c）にして書き出す。色はページ（10 記録）が原作のテクスチャの色で塗る（ユーザーの指示: アセットは白塗りにしてエンジン内で着色する）。`tone` の無いもの（枠・カード・飾り文字・streak のカードなど、何色かで描かれたもの）はそのままの色。
  - `marker.webp` 394×74: `UI/Main/TitleScreen/title_screen_selection_marker.png`（メニュー項目のホバーで背後に出る暗い赤 (99,8,0) の筆）。そのままの大きさ。白。
  - `mask.webp` 960×600: `UI/Main/TitleScreen/title_screen_video_mask.png`（1920×1200。RGB はすべて黒で、左が不透明、右が煙状に透明）。画面の高さに引き伸ばして使うので半分に縮める。白。
  - `restart-frame.webp` 1222×928: `UI/Menu/Pause/restart_window_frame_2.png`（RESTART? のカード。縁のかすれた黒い地、赤い細線、赤い見出し `Restart?` が描き込まれている）。そのままの大きさ。
  - 原作の `UMG_Options`（OPTIONS 画面）が使う `UI/Menu/Settings/` のテクスチャ。すべてそのままの大きさ。
    - `options-frame.webp` 1732×1200: `options_window_frame.png`（OPTIONS のカード。縁のかすれた黒い地、赤い細線の枠、上の中央に赤い見出し `OPTIONS`）。
    - `options-box.webp` 242×55: `selection_bar.png`（値の欄。黒地に赤い細線の枠）。
    - `options-arrow.webp` 21×32: `selection_bar_arrow_normal.png`（赤 (192,0,0) の右向きの山形）。白。ホバーの `selection_bar_arrow_hover.png` は同じ山形の白なので、この 1 枚をページが赤と白に塗り分ける（以前の `options-arrow-hover.webp` は削除した）。
    - `options-check.webp` / `options-checked.webp` 55×64: `checkbox_icon.png`（赤枠の空の箱）/ `checkbox_icon_checked.png`（赤いチェックの入った箱）。
    - `options-thumb.webp` 22×38: `slider_bar_tab.png`（スライダーのつまみ。黒地に赤枠）。
  - 原作の `UMG_Pause`（ポーズ画面）が使う `UI/Menu/Pause/` のテクスチャ。すべてそのままの大きさ。
    - `pause-stroke.webp` 912×1200: `pause_screen_bg.png`（縦長の黒い筆の跡。x 126〜864 が不透明な黒で、左右の縁がかすれる）。白。
    - `pause-restart-frame.webp` 1222×532: `restart_window_frame.png`（RESTART? のカード。縁のかすれた黒い地（x 24〜1192・y 3〜528）、赤い細線（x 73〜1129・y 72〜465）、赤い見出し `Restart?`）。
    - `pause-quit-frame.webp` 1222×928: `quit_window_frame.png`（GIVING UP? のカード。黒い地 x 24〜1192・y 143〜903、赤い細線 x 73〜1129・y 193〜838、赤い見出し `Giving Up?`）。
  - 原作の `UMG_DeathScreen`（死亡画面）が使うもの:
    - `life.webp` 90×90: `UI/Main/life_icon_02.png`（ライフのドクロ。一様な灰 sRGB 124 で、形は α）。そのままの大きさ（画面では 100u に描く）。白（死亡画面とシャードの連続回収の EXTRA LIFE ! が CSS のマスクにして灰 124 などを塗る）。
    - `you-are-dead.webp` 1285×301: `UI/Main/you_are_dead.png`（赤い `You Are Dead` の飾り文字）。そのままの大きさ。ロゴではなく、タイトルの RESTART? の枠の見出しと同じ UI の文字として使う。
    - `death-vignette.webp` 960×540（`DEATH_VIGNETTE`）: `UI/Menu/Streaks/T_Vignette.png`（1920×1080。RGB はほぼ白、形は α〈最大 153〉）の α を白の RGB に付ける。画面いっぱいに引き伸ばすので半分に縮める。死亡画面（Image_161 のブラシの色 linear (0.380, 0, 0) = sRGB (166, 0, 0)）、シャードの連続回収（紫）、脱出の画面の白い閃き（`UMG_LevelClear` の Image_6、色なし）がマスクにして色を塗り、タブレットのシャード獲得の閃き（10 記録）はこの画像の α だけを使って紫に塗る。
  - 原作の `UMG_LevelClear`（脱出の You Escaped! とリザルト）が使うもの。どちらもそのままの大きさ:
    - `you-escaped.webp` 1141×276: `UI/Menu/you_escaped.png`（白地に黒い縁取りの `You Escaped!` の飾り文字。Image_216 のブラシの大きさ）。`you-are-dead.webp` と同じく UI の文字として使う（ロゴではない）。
    - `results-line.webp` 914×18: `UI/Menu/results_window.png`（左右の端がかすれる灰の横線。Image_1 / Image_2 のブラシ。リザルトの幅に引き伸ばして描く）。
  - 原作の `UMG_PopUp` の Frame 2（死亡画面の LAST CHECKPOINT の S ランクの警告）: `blank-frame.webp` 1222×928: `UI/Menu/Pause/blank_window_frame.png`（RESTART? と同じかすれた黒い地と赤い細線で、見出しの無い枠）。そのままの大きさ。
  - 原作の `UMG_Collectables`（秘密を取ったときの表示。ステージに秘密は無く、いまはどれも使っていない）が使うもの: `extras-card.webp` 696×204: `UI/Main/Collectables/extras_unlock_bg.png`（786×248、黒。Image_89 のブラシの大きさに縮める。白）、`extras-art.webp` / `extras-diary.webp` / `extras-sound.webp` / `extras-movie.webp` 159×145: `art_icon` / `diary_icon` / `sound_icon` / `movie_icon.png`（Image_249 のブラシ。そのままの大きさ。赤 (193,0,1) の 1 色で、diary だけ中に黒い線がある。`tone: 193` で赤を白にし、黒は黒のまま）。どれもロゴやキャラクターではない。
  - 原作の `UMG_Interact`（視線の先に使えるものがあるときの表示）が使うもの: `interact.webp` 90×90: `UI/Main/interact_icon_03.png`（90×90。`Image_18` のブラシの大きさそのまま。白い手の形）。ロゴやキャラクターではない。
  - 原作の `UMG_VignetteSides`（特殊シャードの取得の ENEMIES STUNNED / ENEMIES REVEALED）が使うもの:
    - `vignette-sides.webp` 960×540（`VIGNETTE_SIDES`）: `UI/Menu/Streaks/T_VignetteNew.png`（1024²。RGB は白、α は中央 0・縁 196〜229）の α を白の RGB に付ける。画面に引き伸ばして CSS のマスクとして色を付ける（10 記録）ので半分に縮める。スピードブーストの `vignette.webp`（9 節）と同じ元画像。
  - 原作の `UMG_ShardStreak`（シャード連続回収の節目の表示）が使うもの:
    - `streak-<数>.webp` 612×227 × 10 枚（数 = 20, 50, 100, 150, 200, 250, 350, 500, 700, 1000）: `UI/Menu/Streaks/shard_streak_<数>.png`（灰の筆の跡に紫の飾り文字の称号 `NOT BAD` など と白い `<数> SHARD STREAK!`）。そのままの大きさ（ウィジェットの `StreakImage` のブラシの大きさ）。EXTRA LIFE ! のドクロは死亡画面と同じ `life.webp`、紫の周辺減光は `death-vignette.webp` の α を使う（10 記録）。
  - 原作の `UMG_ChapterPortal`（ステージのタイトルカード）が使うもの。すべてそのままの大きさ（ウィジェットはどれもほぼその大きさで描く）:
    - `stage-band-1.webp` / `stage-band-2.webp` 4165×872: `UI/Menu/TitleCards/chapter_ui_banner_bg_01.png` / `_02.png`（横長の黒い筆の帯。RGB は黒で、形は α。01 は最大 255、02 は最大 153。不透明な行は y 20〜835）。白。
    - `stage-ring.webp` 512×512: `UI/Main/chapter_ui_portal_outer.png`（赤い輪。内側は黒く塗られ、外に 3 つの小さな円。不透明部分は x 34〜477・y 38〜511。赤は 189〜192、縁は黒）。`tone: 192`: 黒は黒のまま、赤は R × 255 / 192 の灰（ほぼ白）にする。ページが α を黒、輝度を赤 (192,0,0) で塗る（Chrome の輝度マスクは灰の sRGB 値をそのまま使うので、元の赤の値に戻る）。
    - `stage-runes.webp` 512×512: `UI/Menu/TitleCards/chapter_title_portal_inner.png`（赤 (192,0,0) のルーン文字の輪だけ。`chapter_ui_portal_inner` と違い細い円は無い）。白。
  - `stage-title.webp` 1796×433（`STAGE_TITLE`）: ユーザー提供の題字 `assets-src/stage/title.png`（2172×724 の透過 PNG、赤〈約 (210, 1, 1)〉の「Stinky Gachimi」）の不透明部分（α > 8。x 68〜2109・y 133〜624 の 2042×492）を切り出し、幅 `width` 1796（原作の題字の文字幅 898u の 2 倍）に縦横比を保って縮め（lanczos3）、α だけを白の RGB に付ける。原作の `chapter_ui_title_*` は一様な灰 sRGB 196 の文字で、ウィジェットのアニメの色が塗るので、同じ形にする（CSS のマスクに使い、灰 196 は stage-intro.ts の `TITLE_GREY` が色に掛ける）。
  - `public/fonts/Roboto-Bold.ttf`（`FONTS`）: pak の `Engine/Content/EngineFonts/Faces/RobotoBold.ttf`（UE の TextBlock の既定フォント Roboto Bold、1.00000 / 2011、Apache License 2.0）をそのままコピーする。OPTIONS のラベルと RESTART? の YES / NO が使う。ライセンスの表記は手で置いた `public/fonts/LICENSE-Roboto.txt`。同じく `RobotoLight.ttf` を `public/fonts/Roboto-Light.ttf` にコピーする（死亡画面のヒントの RobotoTiny の Light）。原作の UI の書体（`DDeception/Content/UI/Fonts`）も書き出す: `helvetica-neue-bold.ttf`（原作の `helvetica-neue-bold_Font`。Linotype の Helvetica Neue Bold、135,312 バイト）はそのままコピーし、`helvetica-normal.ttf`（`helvetica-normal_Font`、Helvetica-Normal）は `repair: true` で `repairSfnt` を通す（32,016 バイト）。原作の helvetica-normal は glyf の長さが最後のグリフより 2,412 バイト長く cmap に重なり（"overlapping tables"）、cmap のサブテーブルの language が 1 で、どちらも Chrome のフォント検査（OTS）に拒否されるため、表を 1 つずつタグ順・4 バイト境界で書き直し（glyf は loca の終わりまで）、cmap の language を 0 にし、チェックサムと head の checkSumAdjustment を計算し直す（グリフは変えない）。出どころと著作権の表記は手で置いた `public/fonts/NOTICE-original-fonts.txt`。Helvetica Neue は商用の書体だが、ユーザーの指示（2026-09-14「フォントが本家データに含まれているなら、それも移植対応」）で同梱する。
  - `wisps.webp` 3840×800: `UI/Main/TitleScreen/title_screen_chapters_background.png`（5760×1200。色は一様な (151,8,0) で、筆の形は α（最大 45）だけにある）の α を `alphaGain` 2.5 倍し、白の RGB に付ける（ページが灰 179 で塗る）。原作の `MM_TitleScreen_Mask_Grey` はこれを脱色してパンするが、不透明度の掛け方が cook で消えているので、明るさは以前の動画の観察（最も濃いところで灰 0.7・不透明度 0.45 相当）に合わせた推定。
- 使う側は 10 記録（styles.css のタイトル画面・ポーズ画面・死亡画面・ステージ OP）。`<img>` と CSS の `url()` がページから相対で読む（manifest には入れない）。
- pak_reference が無ければ「pak_reference not found」で終了（生成物は git に同梱済み）。

### 11. ポーズ画面の頭（`scripts/prepare-pause-heads.mjs`）
- 原作の `UMG_Pause` はレベルの怪物の頭を出す（`GameMode.Level` で切り替え。館は Watchman）: メニューの上の大きな頭（`pause_*_head`: 1024² のテクスチャを 900u 四方で描き、頭は x 270〜752・y 321〜833 に赤 (192,0,0) の 1 色）と、RESTART? / GIVING UP? のカードの上からのぞく頭（`quit_window_head_*`: 1222×928、黒い地に赤い線で、カードの赤線の y 193 で切れる）。本作は原作の頭を使わず、ユーザー提供のワサミの赤い絵 `assets-src/pause/wawa.png`（`SOURCE`。512×512 の透過 PNG、赤 1 色で顔の暗い所がくり抜かれている。不透明部分は x 70〜463・y 4〜507）から 2 枚を作る。どちらも `public/title/` の可逆 WebP。色は持たせず白で書き出し、ページ（10 記録）が原作の頭の赤 (192, 0, 0) を塗る。
  - `pause-head.webp` 1024×1024（`HEAD`）: 絵の α を白に付け、インク（α の総和）がテクスチャの `ink` 0.03257（原作の館の頭 `pause_watchman_head` の α の総和 34149 / 1024²）になる大きさにして（lanczos3）、顔の軸（絵が鏡像と最もよく重なる縦の線 `mirrorAxis`。元画像の x 260.5）を x `centre[0]` 512 に、重心を y `centre[1]` 512 に置く（原作の頭の重心は y 496〜581、Ballroom のサル 521）。原作の頭の枠 [270, 321, 753, 834) に収めていた前の版は、塗りの多い顔が大きく見えた。できた頭は x 366〜666・y 369〜753、インク 0.0326。ステージ OP（470 単位）ではあごひげの角が中心から 118.4 単位（ルーンの内縁 126.7 単位の内側）、ポーズ画面（900u、上端 −180u）では 144〜482u。
  - `pause-peek.webp` 1222×928（`PEEK`）: 絵の輪郭を `seal` 14px ぶん太らせて隙間をふさぎ、縁から届かない内側を塗りつぶして（閉じた輪郭の中）`seal − halo` だけ細らせ（輪郭より `halo` 6px 外まで）、σ 0.8 でぼかした黒い地の上に白い絵を重ねる（RGB は画素のうち絵の割合の灰、α は地と絵を合わせたもの。ページが α を黒、輝度を赤にする）。目の下 `below` 276（元画像の px）で水平に切り、上端が `top` 4、切り口が `bottom` 193 に来る大きさにして、顔の軸を `centre` x 598 に置く（原作の `quit_window_head_watchman` は x 405〜791 で中心 598。原作の Watchman / Monkey の切り口は 193〜195）。
- 使う側は 10 記録（`index.html` の `.pause__head` / `.pause__peek`、styles.css の `.intro__logo`）。

### 13. 特殊シャードの素材（`scripts/prepare-special-shards.mjs`）
- 原作のオブジェクト `power_orb`（`BP_PowerOrb` の `soul_shard` 部品のメッシュ `/Game/Meshes/Shared/power_orb`。pak の `_meshes_gltf/Meshes/Shared/power_orb.gltf` は umodel の出力で m・(UE.X, UE.Z, UE.Y)、半径 0.44 m の球、285 頂点・480 三角形）を、位置・法線・インデックス（Uint16）だけの GLB `public/assets/models/power_orb.glb`（10.4 KB、マテリアル無し。見た目は `src/world/specials.ts` が付ける。08 記録）にする。`ORB` を export。
- `BP_StunCollectEffect` の球の `M_05_Primal` が読む `Textures/05_Circus/T_05_PortalMaps.png`（2048² RGB。青い粒の雲の中央に緑）を 512² に縮め（lanczos3）、脱色して明るさを全域に広げた（`grayscale` + `normalise`）WebP `public/fx/stun-sphere.webp`（quality 85、49.4 KB）にする。`SPHERE_TEXTURE` を export。脱色は、`M_05_Primal` の Desaturation と Color の掛け方が cook で消えていて、元の青と緑に橙の Color を掛けるとほぼ黒になるため（灰の模様に Color で色を付ける推定。08 記録）。
- 実行後は `npm run assets:level` で manifest の `model` を更新する（`assets:all` には含まれない）。

### 14. ステージ（Chaotic Customer 2 の Zone_1）のテクスチャとマテリアル表（`scripts/prepare-cc2-textures.mjs`）
- 入力: `assets-src/cc2/layout.json`（`npm run assets:cc2-layout`、12 記録。マテリアル 157 種を書き出しの `fix_materials` と同じく解決したもの: 種類ごとのテクスチャ〈`diffuse` / `normal` / `orm` / `occlusion` / `roughness` / `metallic` / `emissive` / `opacity`〉、ブレンド、両面、定数）と cc2_reference の PNG（環境変数 `CC2_REF`、既定 `<repo>/cc2_reference`）。どちらかが無ければ終了。`node scripts/prepare-cc2-textures.mjs [--force] [--jobs N]`（既定 4 並列、出力が入力より新しければ飛ばす）。
- 出力:
  - `public/assets/cc2/tex/<フォルダー>__<名前>.<kind>.ktx2`: 名前は `Content/Content_game/models/`（または `Content/`）からの相対パスの英数字以外を `__` にしたもの（書き出しの PNG は `plane_for_tex_DefaultMaterial_BaseColor` など同じ名前が多くのフォルダーにあるため）。作った絵は `gen__…`。kind と basisu の設定は Hotel（12.）と同じ（`KINDS`）。一辺は `MAX_SIZE` 1024（ユーザーの指定）、迷路と駅の大きな部品（`WIDE_MESHES`: `Zone_1_ENHANCED` / `Metro` / `tunnel` / `Branch_loc` の glb）に使われるマテリアル 48 種だけ `MAX_WIDE` 2048 に縮め（lanczos3）、4 の倍数に切り上げる。法線は G を反転（UE は DirectX 規約）。アルベドの α は MASK / BLEND / ADD で α のあるものだけ残す。現行 280 枚・233.3 MB（albedo 129・normal 73・orm 71・emissive 7。2048 の 102 枚で 172 MB。最大 `Zone_3__Broken_floor__plane_for_tex_DefaultMaterial_BaseColor.albedo.ktx2` 3.51 MB）。
  - `assets-src/level/materials.json`（git 管理。build-level.mjs が読む）: layout.json の鍵（`<名前>@<フォルダー>`）→ `baseColor` / `normal` / `orm` / `emissive`（`public/` からの URL）、`baseColorFactor`（テクスチャがあれば白、無ければ定数色。α は `opacity`）、`metallicFactor` / `roughnessFactor`（ORM があれば 1）、`emissiveFactor`、`emissiveStrength` 1（cook 後の式には倍率が残らない。fix_materials と同じ）、`alphaMode`（MASK は α があるか opacity 0 のとき、BLEND / ADD は BLEND）、`alphaCutoff`（`clip`、既定 0.3333）、`unlit` false、`doubleSided`、`glow`（街灯 `lamp` とレトロな灯 `retroLamp` の配置のマテリアルで発光のあるもの。現行 2 種）、`bakeAlbedo`（Blender のベイクが貼るアルベド）、`ue`（アセットのパス）。
  - `assets-src/cc2/tex/`（git の対象外）: 組み立てた ORM（`<鍵>_orm.png`。遮蔽・粗さ・金属が別のテクスチャなら R / G / B に詰め、無いものは定数）、デカールのアルベド（`<鍵>_decal.png`。色のテクスチャか定数色に、不透明度〈マスクの R、`opacityChannel` が `a` なら色の α〉を α に。表では BLEND・両面・粗さ 0.8）、差し替えの絵（`replace/`）。
- ファンゲームの人物の絵は持ち込まない（ユーザーの指定 2026-09-14: ワサミの絵に差し替え）。`REPLACE` はテクスチャ（`Content_game/models/Zone_1/` からのパス）ごとに、人物の箱（絵の割合 x0, y0, x1, y1）を周りの色でぼかして塗りつぶし（箱と余白 8 % を σ = 箱の短辺 × 0.18 でぼかす）、写真（`public/wasami.webp` を楕円でぼかしたもの）か赤い顔（`assets-src/pause/wawa.png` の形。地が赤っぽければ暗い赤 (70, 8, 6)、明るければ (150, 14, 10)、暗ければ (214, 26, 18)）を置く。赤い輪のデカール `Portal_Decal_D/H/P` は中心の円（`disc`）の α を抜き、輪の赤の平均色の顔を置く。対象はポスターと落書き 10 枚（`MonkeAd2-4`、`Portal_Decal_D/H/P`、`Poster/` の `1794_…`・`1917_…`・`ConeAd`・`JJG_build_board`・`OriginalPopuskIManda`・`circus_build_board`）で、題字や地は残す（10 % の格子で測った箱）。`1829_…`（コーンとアヒルの端）と `1881_…`（クレヨン）は人物が無いので元のまま。現行の配置が使うのは 7 種。ロゴは見当たらない。`--preview` は差し替えだけを作って元の絵と並べた `assets-src/cc2/tex/replace/_sheet.png` を書いて終わる。

### 実行時向け manifest の形式まとめ
| ファイル | 生成元 | 形式 |
| --- | --- | --- |
| `public/assets/manifest.json` | build-level.mjs | `{ generated, totalBytes, files: [{ url, bytes, kind: 'level'\|'texture'\|'lightmap'\|'env'\|'model'\|'audio' }] }` |
| `public/assets/level/colliders.json` | cc2/colliders.mjs → build-level.mjs | `{ floor, wall, ramps: { positions, indices }, waypoints, enemySpawns }`（Blender の m の箱、階段の坂のメッシュ、巡回点。12 記録） |
| `public/assets/level/stage.json` | cc2/layout.py → build-level.mjs | `{ floors, start, checkpoints, shards, enemySpawns, mannequinRespawns, specials, triggers, panel, barriers, doors, hiddenWalls }`（12 記録） |
| `public/assets/credits.json` | 手で保守（以前は fetch-textures.mjs） | `{ hdri: { source, id, resolution, page, file, license } }`（反射の HDRI だけ） |
| `public/sfx/manifest.json` | copy-sfx.mjs | `{ id: { url, source } }`（137 件: `SFX` 100 と `CC2_SFX` 37） |
| `public/voices/manifest.json` | （生成スクリプトなし・手作業/旧版由来） | `{ normalization: { integratedLUFS: -23, truePeakDB: -6 }, clips: [{ id, category, url: '/voices/<id>.mp3', subtitle, duration, truePeakDB, source }] }`（15 クリップ、category は intro / patrol / chase / boost / caught / respawn / pickup / ring / win） |

## 依存関係
- 外部: `sharp`（リサイズ・グレースケール・PNG / JPEG / WebP 出力、SVG の描画）、`meshoptimizer`（シャードモデルの簡略化）、`basisu` CLI、`babylonjs-ktx2decoder`（npm）、cdn.babylonjs.com（`vendor-babylon.mjs` の取得時のみ）、pak_reference（`PAK_REF`。ホテル・タイトル・ブーストと壊せる物の煙の素材）
- 自前: `build-level.mjs` は Blender 出力と `materials.json` / `colliders.json` / `stage.json`（記録 #12）に依存。
- 実行時の利用者: `src/core/loader.ts`（`assets/manifest.json` で進捗バー・プリフェッチ）、`src/world/level.ts`（`lightmap*.ktx2`, `level-meta.json`, `colliders.json`, `stage.json`）、`src/game/game.ts`（`assets/manifest.json` を `loadManifest` で読み、`voices/manifest.json` を fetch）。`sfx/manifest.json` は実行時には読まれず、音源パックとの対応表（クレジット用）。KTX2 の復号は `public/vendor/ktx2/` から。

## 設定・調整値
- `src/config.ts` は参照しない。調整はスクリプト内定数: `prepare-cc2-textures.mjs` の `MAX_SIZE` 1024・`MAX_WIDE` 2048・`WIDE_MESHES`・`KINDS`・`REPLACE`、ライトマップの quality 70、`--effort`（3）、`SFX`・`CC2_SFX`（と Opus の 128 kbps）。
- `CONFIG.lightmap.full` / `indirect` が `lightmap.ktx2` / `lightmap_indirect.ktx2` のパスを持ち、`lights.mode` で切り替える。

## 既知の制約・注意点
- `prepare-cc2-textures.mjs` は出力が入力より新しいと飛ばすが、差し替えの絵は毎回作り直すので、毎回符号化し直すものがある（差し替え 7 枚と組み立てた ORM・デカールは毎回符号化し直す）。
- ステージの素材の作り直しには cc2_reference（書き出しへのリンク、git の対象外）が要る（`assets:cc2-layout` / `assets:cc2-tex` / `assets:cc2-bake`）。生成物（`public/assets/cc2/tex/`、`assets-src/level/materials.json`）は git に同梱する。
- 原作のマテリアルの式は cook で消えているので、`Roughness Power` / `Metallic Power` / `Normal Flatness` などのスカラーは使っていない（glTF の係数に写せないため）。
- 以前の館の `assets-src/textures/`（Poly Haven の取得のキャッシュ、git の対象外）は手元に残っていても使わない。
- manifest の `audio` は `public/sfx/*.ogg` と `public/voices/*.mp3` をディレクトリ走査で拾うので、`assets:sfx` の後に `assets:level` を再実行しないと `manifest.json` が古くなる（`assets:all` に sfx は含まれない）。
- `vendor-babylon.mjs` は `ktx2Transcoders/1/` の固定パスを取得する。Babylon のバージョン更新時にデコーダとの整合を確認する必要がある。
- `voices/manifest.json` の生成手順はリポジトリ内に無い（`README` では「以前の版から引き継いだ」）。正規化値の意味（LUFS / true peak）は manifest 内の記述のみ。

## 変更履歴
- 2026-09-11: 初版（現行実装を記録）
- 2026-09-11: シャードモデル前処理 `prepare-shard-model.mjs`（`assets:shard`）と `tests/shard-model.test.ts` を追加。`build-level.mjs` の manifest に kind `model` を追加
- 2026-09-11: `encode-ktx2.mjs` が幅だけ見てリサイズを判断していたため `velvet`（4096×3941）がそのまま UASTC になり、WebGPU で ASTC の CreateTexture が失敗して黒フレームの原因になっていた。幅か高さが違えばリサイズし、`--size` とライトマップの寸法が 4 の倍数でなければエラーにした。`velvet` の 3 枚を 4096² で作り直し、`tests/ktx2-textures.test.ts` を追加
- 2026-09-11: テレポーテーション用に `SFX` へ `teleport_aim`（照準ループ）と `teleport`（発動音）を追加（49 → 51 件）。`assets:sfx` → `assets:level` で manifest を更新
- 2026-09-11: 敵のモデル前処理 `prepare-enemy-model.mjs`（`assets:enemy`）と `tests/enemy-model.test.ts` を追加。`SFX` に敵の音 `enemy_alert` / `enemy_scream` / `frenzy_loop` / `enemy_fs_1..12` を追加（51 → 66 件）
- 2026-09-12: テレポートの音を原作のものにした: `teleport` を原作の確定音 `Teleport_Committed`（以前は Agatha のテレポート音 `DD_LVL2_17_V3_Teleport_New`）にし、照準開始の `teleport_enter`（`Teleport_Mode_Entered`）を追加した（66 → 67 件）。どちらも pak_reference から音源パックの `Engine/VREditor/Sounds/UI/` へコピーし、`assets:sfx` → `assets:level` で manifest を更新（館の glTF は変わらない）
- 2026-09-12: `power_boost` を原作のスピードブーストの発動音 `UI/Shard_Streak_Milestone_V5.ogg` にした（以前 `SharedGameplay/Stun_Wave_Attack_New_04.ogg`）。`assets:sfx` → `assets:level` で manifest を更新（件数は 67 のまま）
- 2026-09-12: スピードブーストの画面の素材を作る `prepare-boost-fx.mjs`（`assets:boostfx`）を追加した: 原作の集中線のフリップブック・ビネット・ラジアルブラーのマスクを `public/fx/` のグレースケール WebP にする
- 2026-09-12: `power_boost`（V5）の原作のアセットの設定（Pitch 2.0・Volume 0.7、波形は V4 と同一）を追記した
- 2026-09-12: タイトル画面を原作に倣うため、`SFX` に `title_music` / `ui_select` / `ui_popup` / `title_voice` を追加し（67 → 71 件、`assets:sfx` → `assets:level` で manifest を更新。館の glTF は変わらない）、原作の UI テクスチャから `public/title/` の素材を作る `prepare-title-assets.mjs`（`assets:title`）を追加した
- 2026-09-12: タイトルの OPTIONS を原作に倣うため、`prepare-title-assets.mjs` に原作の `UMG_Options` のテクスチャ 7 枚（`options-frame` / `options-box` / `options-arrow(-hover)` / `options-check(ed)` / `options-thumb`）を追加した
- 2026-09-12: `prepare-title-assets.mjs` が原作のエンジンの Roboto Bold を `public/fonts/Roboto-Bold.ttf` にコピーするようにした（`FONTS`。OPTIONS のラベルと RESTART? の YES / NO 用）
- 2026-09-12: `SFX` の絨毯の足音を原作の `Footsteps_Carpet` の 25 種すべてにした（71 → 88 件。`assets:sfx` → `assets:level` で manifest を更新。館の glTF は変わらない）
- 2026-09-12: ポーズ画面を原作に倣うため、`prepare-title-assets.mjs` に原作の `UMG_Pause` のテクスチャ 3 枚（`pause-stroke` / `pause-restart-frame` / `pause-quit-frame`）を追加した
- 2026-09-12: ポーズ画面の頭を作る `prepare-pause-heads.mjs`（`assets:pause`）を追加した: ユーザー提供のワサミの赤い絵（`assets-src/pause/wawa.png`）を原作の頭の枠に収めた `pause-head.webp` と、黒い地に載せてカードからのぞかせる `pause-peek.webp`
- 2026-09-13: 死亡画面を原作に倣うため、`SFX` に `game_over`（`66_-_Game_Over`）を足し、制限時間の撤廃で使わなくなった `danger_loop` を外した（88 件のまま。`public/sfx/danger_loop.ogg` は削除し、`assets:sfx` → `assets:level` で manifest を更新。館の glTF は変わらない）。`prepare-title-assets.mjs` に原作の `UMG_DeathScreen` の素材（`life.webp` / `you-are-dead.webp` / `death-vignette.webp`〈`DEATH_VIGNETTE`〉）と Roboto Light を足した
- 2026-09-13: 原作の Hotel に置き換えた（ユーザーの指示）: `build-level.mjs` を `materials.json` で KTX2 を割り当てる形にし（`TINTS` とテクスチャセットをやめ、`KHR_materials_unlit` / `emissive_strength`、`colliders.json` のコピーと `hotel.json` にポータルのテクスチャ）、`encode-ktx2.mjs` をライトマップだけに、`fetch-textures.mjs` / `texture-sets.mjs` と `public/assets/textures/` を削除、`credits.json` を HDRI だけにした。`copy-sfx.mjs` にホテルの BGM の対・エレベーター・扉を破る音・欠片の音（88 → 100 件）、`prepare-hotel-textures.mjs` にポータルのテクスチャとワサミの赤い顔のロゴ。`tests/ktx2-textures.test.ts` はマテリアル表の検査に
- 2026-09-13: 原作の Hotel のテクスチャとマテリアル表を作る `prepare-hotel-textures.mjs` を追加した（`public/assets/hotel/tex/` の KTX2 151 枚と `assets-src/level/materials.json`。キャラクターの絵 10 枚はワサミの絵に差し替え。まだ実行時には使わない）
- 2026-09-13: ステージ OP（原作の `UMG_ChapterPortal`）のため、`prepare-title-assets.mjs` に原作の帯 2 枚・赤い輪・ルーンの輪（`stage-band-1` / `stage-band-2` / `stage-ring` / `stage-runes`）と、ユーザー提供の題字 `assets-src/stage/title.png` から作るマスク `stage-title.webp`（`STAGE_TITLE`）を足した
- 2026-09-13: シャード連続回収（原作の `UMG_ShardStreak`）のため、`SFX` の `shard_streak`（V2）を原作の節目の 4 種 `streak_v1a` / `streak_v2` / `streak_v3a` / `streak_v4` に置き換え（100 → 103 件。`public/sfx/shard_streak.ogg` は削除し、`assets:sfx` → `assets:level` で manifest を更新。館の glTF は変わらない）、`prepare-title-assets.mjs` に原作の節目の画像 10 枚（`streak-20` … `streak-1000`）を足した
- 2026-09-13: 原作の特殊シャード（`BP_PowerOrb` / `BP_BonusShard`）のため、`SFX` に `bonus_pickup` / `stun_countdown` / `stun_wave` を足し（103 → 106 件）、`prepare-title-assets.mjs` に `UMG_VignetteSides` の `vignette-sides.webp`（`VIGNETTE_SIDES`）を、新しい `prepare-special-shards.mjs`（`assets:specials`）でオーブのメッシュ `power_orb.glb` と球のテクスチャ `stun-sphere.webp` を作るようにした。`build-level.mjs` は `hotel.json` の `specials` をそのまま書く（変更なし）
- 2026-09-13: UE の減衰の窓で焼き直したライトマップ（12 記録）を `assets:ktx2 --force` → `assets:level` で KTX2 と manifest に入れた（スクリプトは変わらない）
- 2026-09-14: ステージを Chaotic Customer 2 の Zone_1 へ差し替えるため（ユーザーの指示）、そのテクスチャとマテリアル表を作る `prepare-cc2-textures.mjs`（`assets:cc2-tex`。280 枚・233.3 MB、人物の絵 7 種はワサミの絵に差し替え）を追加し、`materials.json` をステージのものにした。`encode-ktx2.mjs` はライトマップのページ `lightmap(_indirect)_<n>.png` を符号化して古いライトマップを消し、`build-level.mjs` は `stage.json` をコピーして `hotel.json` とポータルのテクスチャをやめ、manifest にページを載せる。`tests/ktx2-textures.test.ts` はステージのマテリアル表を検査する。まだ実行時には使わない（ベイクは次）
- 2026-09-14: `build-level.mjs` が glb の頂点の色を外して buffer を詰め直すようにした（`stripColours`）
- 2026-09-13: 結晶のマテリアルの発光の頭打ちを 8 から `CRYSTAL_GLOW_MAX` 3 にした（UE 4.21 のトーンマップにしたところ障壁の球がピンクに白く飛んでいたので、本家の収録に合わせた。`assets:hotel-tex` → `assets:level` で `materials.json` と `level.gltf` を更新）
- 2026-09-13: build-level.mjs が反射キャプチャの全天球（`assets-src/level/captures/*.hdr`。12 記録）を `public/assets/level/captures/` にコピーし、manifest に `env` として入れるようにした（297 ファイル・138.7 MB）
- 2026-09-14: 脱出の画面を原作の `UMG_LevelClear` に倣うため、`SFX` に `grade_stamp_1` / `grade_stamp_2` / `xp_fill` を足し、原作のどの Blueprint も使わない `level_complete` を外した（106 → 108 件。`public/sfx/level_complete.ogg` は削除）。`prepare-title-assets.mjs` に `you-escaped.webp` と `results-line.webp` を足した。manifest の `audio` は build-level.mjs と同じ規則（`public/sfx/*.ogg` の名前順）で音の区間だけを書き直した（館の glTF は変わらないので build-level は動かしていない。299 ファイル）
- 2026-09-14: 原作の Hotel の秘密と隠し扉（08・12 記録）のため、`SFX` に `secret_pickup` / `secret_revealed` を足し（108 → 110 件）、`prepare-special-shards.mjs` の GLB の書き出しを関数にして（区画をまとめ、UV も書ける。`power_orb.glb` は同じバイト列）秘密のフォルダー `secret_file.glb` とそのテクスチャ `secret-file.webp` / `secret-file-packed.webp` を作るようにし、`prepare-title-assets.mjs` に `UMG_PopUp` の枠 `blank-frame.webp` と `UMG_Collectables` のカードとアイコン（`extras-card` / `extras-art` / `extras-diary` / `extras-sound` / `extras-movie`）を足した。layout.mjs → Blender の `--no-bake` → build-level.mjs で `level.gltf` / bin・`colliders.json`・`hotel.json`・manifest を更新した（302 ファイル・138.8 MB）
- 2026-09-14: `SFX` マップから原作の Hotel が使わない音 9 件（`amb_bass`・`torch_loop`・`portal_loop`・`sweetener_1..3`・`barrier_success`・`portal_unlocked`・`breath`）を外し、`public/sfx` のファイルと `sfx/manifest.json`・`assets/manifest.json`（`totalBytes` も）から消した（110 → 101 件、manifest は 302 → 293 ファイル）
- 2026-09-14: `prepare-title-assets.mjs` の `TITLE_ASSETS` に原作の `UMG_Interact` の手 `interact_icon_03` → `interact.webp`（90×90）を足した
- 2026-09-14: ユーザーの指示で原作の UI の書体を同梱した: `FONTS` に `helvetica-neue-bold.ttf`（そのまま）と `helvetica-normal.ttf`（`repair`）、ブラウザが読めるように表を書き直す `repairSfnt`、`public/fonts/NOTICE-original-fonts.txt`（10 記録の styles.css と tablet.ts が使う）
- 2026-09-14: ユーザーの指示でポーズ画面の頭を白で書き出すようにした（色はページが原作の頭の赤で塗る）。あわせて頭が大きく中心がずれて見えるとの指摘から、`HEAD` を原作の頭の枠に収める形から、館の Watchman と同じインク量・顔の軸を中央・重心を中心に置く形（`ink`、`centre`、`mirrorAxis`）にし、`PEEK` も顔の軸を `centre` 598 に置くようにした
- 2026-09-14: 同じ指示（「同アプローチが効くアセット全て」）で、`prepare-title-assets.mjs` の 1 色だけの素材を白で書き出すようにした: `TITLE_ASSETS` に `tone`（`'white'`、または黒地に赤の割合を灰にする色の値）を足し、`marker` / `mask` / `options-arrow` / `pause-stroke` / `life` / `extras-card` / `stage-band-1` / `stage-band-2` / `stage-runes` を白、`extras-*` を `tone: 193`、`stage-ring` を `tone: 192` に。`WISPS`（`grey` 179）・`DEATH_VIGNETTE`（166 の赤）・`STAGE_TITLE`（`grey` 196）も白にした。白と同じ形になった `options-arrow-hover.webp` は削除した
- 2026-09-14: Hotel を削除した（ユーザーの指示）: `prepare-hotel-textures.mjs` と `public/assets/hotel/tex/`（KTX2 155 枚）、npm scripts の `assets:hotel-layout` / `assets:hotel-tex` / `assets:bake` を消し、`assets:all` をステージの順（cc2-layout → cc2-tex → cc2-luts → cc2-bake → cc2-colliders → ktx2 → level → vendor）に。`prepare-special-shards.mjs` から秘密のフォルダー（`secret_file.glb`・`secret-file*.webp`）を、`copy-sfx.mjs` から `secret_pickup` / `secret_revealed` を外し、それらのファイルを消した（`public/sfx/manifest.json` は 99 件）
- 2026-09-14: ステージの音のため `copy-sfx.mjs` に `CC2_SFX`（37 件）を足した: Chaotic Customer 2 の書き出しの wav を ffmpeg で Ogg Opus（128 kbps）にして `public/sfx/cc2_*.ogg`（計 6.4 MB）へ。環境変数 `CC2_REF`。`public/sfx/manifest.json` は 136 件（06 記録）
- 2026-09-14: マテリアルを直して焼き直したステージを組み立て直した（12 記録）: manifest は 439 本・384.1 MiB（KTX2 269、ページ 4 枚 20.5 MB、`level-0..7.bin`）
- 2026-09-15: `CC2_SFX` に駅の柵の `cc2_wood_break_1` / `_2`（`Wood_Break1` / `Wood_Break2`）としゃがみの `cc2_crouch_1` / `_2`（`Crouch_1` / `Crouch_2`）を足した（37 → 41 件、`public/sfx/manifest.json` は 140 件。06 記録）
- 2026-09-15: `CC2_SFX` に敵へのパンチの `cc2_mannequin_hit_1..3`（`M_Hit_battle_01..03`）と `cc2_mannequin_fall`（`Mannequin_Fall_Impact`）を足した（41 → 45 件、`public/sfx/manifest.json` は 144 件。06・15 記録）。`assets/manifest.json` の `audio` には次の組み立てで入った
- 2026-09-15: 駅の柵の板を動く部品にして焼き直したステージを組み立て直した（12 記録）: manifest は 447 本・384.2 MiB（KTX2 269、ページ 4 枚 19.6 MB、`level-0..7.bin`。新しい音 8 本が `audio` に入った）
- 2026-09-15: パンチと駅の柵のファンゲームの音をやめ、壊せる物の音と煙を原作のものにした: `CC2_SFX` から `cc2_wood_break_1` / `_2`・`cc2_mannequin_hit_1..3`・`cc2_mannequin_fall` を消し（45 → 39 件）、`SFX` に `boards_break`（`01_Hotel/Wooden_Boards_Breaking_v5.ogg`）を足した（99 → 100 件、`public/sfx/manifest.json` は 139 件）。`prepare-boost-fx.mjs` が `T_Smoke_SubUV` を色と α のまま `smoke-subuv.webp` に書く（`channel: 'rgba'`、全部 `alphaQuality` 100）。`assets/manifest.json` は 442 本・402,808,475 バイト（07 記録）
- 2026-09-15: ファンゲームのしゃがみをやめたので（本作の規範。05・06 記録）、`CC2_SFX` から `cc2_crouch_1` / `_2`（`Crouch_1` / `Crouch_2`）を消し（39 → 37 件、`public/sfx/manifest.json` は 137 件）、`public/sfx/cc2_crouch_1.ogg`・`_2.ogg` も消した。同じ変更で、駅の板張りの通り道（出口の板と低い戸口の格子の壁）を動く部品にしたステージを焼き直して組み直す（`assets:cc2-layout` → `assets:cc2-bake --size 4096 --density 10 --samples 64` → `assets:cc2-colliders` → `assets:ktx2` → `assets:level`。12 記録）。`assets/manifest.json` はその組み立てで変わる
