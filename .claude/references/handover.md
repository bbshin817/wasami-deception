# 引き継ぎ調査（WebGL 版 → UE5 版）

2026-09-16 の調査。WebGL 版（`C:\Users\User\Downloads\wasami-deseption`、以下「WebGL 版」）から、この UE5 版へ何を持ち越し、何を UE の仕組みに置き換えるかをまとめる。WebGL 版のファイルは以下 `<WEBGL>/…` と書く。

## 1. WebGL 版の到達点

Babylon.js 9.26 + Havok + Vite/TypeScript の一人称ホラー探索（Dark Deception のワサミ版ファンゲーム）。270 コミット、最終 2026-09-15。

- **遊べる流れ**: タイトル → ステージ OP → 地下鉄のホーム（チェックポイント 1）→ 柵・板張り・ダッシュの障壁を 1 クリックで壊して階段へ → 地上の広間のトリガー（チェックポイント 2、1 s 後に敵 10 体）→ シャード 301 個（ワサミ餅）→ シャードの障壁が割れる → 配電盤 → 暗転して脱出（チェックポイント 4、トラックと群れ）→ ゴールでクリア画面。ライフ 3、制限時間なし、捕まると死亡画面からチェックポイントへ。
- **ステージ**: ファンゲーム『Chaotic Customer 2』（UE5.4）の `Chaotic_Customer_Zone_1`。CUE4Parse の書き出しから配置 1,242・灯 301・マテリアル 157 を組み立て、Blender でライトマップを焼き直していた。
- **本家（Dark Deception）から写したもの**: プレイヤー（歩き 300・ダッシュ 600・ブースト 870 cm/s、FOV 90→115 の速さ連動、頭の揺れ、足音、中クリックの 180°）、テレポーテーション（Q・ホイール・左クリック、カメラアニメ）、スピードブースト（6.75 s / 8.5 s）、3D タブレット（地図・Z ズーム・矢印）、シャードの取得演出と連続回収、特殊シャード 2 種、死亡画面・ポーズ・タイトル・OPTIONS・ステージ OP・脱出の画面（どれも本家の UMG と BP のバイトコードの値）。
- **ステージのギミック（ファンゲームの BP の値）**: 扉 210（施錠・開き戸・通気口・シャッター・ブロッカー）、罠の扉 25、床の噴出 6、車 2、街灯の点滅、脱出のトラック 5 本と群れ。
- **作っていないもの**: 脱出のボス戦のマネキン、トラックの標識・火・灯、脱出ゾーンの灯と飾り、壊れた扉の破片、車輪の回転。

仕様の正本は `<WEBGL>/README.md`、`<WEBGL>/.claude/implementation-records/`（00 が全体像、04 がゲームの規則）、`<WEBGL>/.claude/references/chaotic-customer-2/README.md`（ステージのデータから確かめた流れと値）、`<WEBGL>/.claude/references/dark-deception/07-wasami-mapping.md`（本家との対応）。

## 2. 引き継ぐもの

### 2.1 方向性とユーザーの決定（そのまま守る）

`.claude/guides/original-fidelity.md` に移した。要点:

- あらゆる要素は本家（`pak_reference`）に基づく。ステージだけが例外で、CC2 の Zone_1（`cc2_reference`）が根拠。
- 本家に無いギミックは複雑にしない。壊せる物は本家のホテルの板張りのバリケード（`BP_01_Woodboards`）のように 1 クリックで崩れて自然に消える。初見で案内なしに遊べる。
- ファンゲームのしゃがみ・スライディング・パンチ・ロックピックの連打・床の案内の文字は採らない（2026-09-15 の決定）。
- テレポーテーションとモーションブラーは旧版の `pak_reference`（UE 4.21）に従う。ステージのボリュームのモーションブラー 0 は採らず本家の既定 0.5。
- 原作とファンゲームのロゴ・キャラクターのモデルは使わない。人物の描かれたポスターと落書き 10 枚はワサミの絵に差し替えたものを使う。敵はワサミ。
- Telekinesis は実装しない。ライフは 3（ファンゲームは 5）。制限時間は無い。
- 本作独自として保留中の要素（敵がワサミの声で話す、知覚の弱さなど）は `07-wasami-mapping.md` の「保留」の表。

### 2.2 根拠のデータ（このリポジトリの直下に移した。git の対象外）

| データ | 場所 | 大きさ | 中身 |
| --- | --- | --- | --- |
| 本家の原作データ（UE 4.21） | `pak_reference/` | 2.9 GB | BP バイトコード、全アセットのプロパティ、UMG、カメラ、SoundCue、パーティクル、テクスチャ、音、レベル配置。読み方は同フォルダの README |
| 本家の新しい版（UE 4.24） | `pak_reference_2/` | 18.3 GB | 後のチャプターと追加の効果。旧版と違うものはユーザーに確認 |
| CC2 の書き出し（UE 5.4） | `cc2_reference/` | 7.5 GB | glb（メッシュ空間は UE そのもの）、usda、png、wav 206、BP の逆コンパイル、umap の全プロパティ |
| 調査の資料 | `<WEBGL>/.claude/references/` | — | CC2 のゲームの流れと値、本家の用語・敵・仕組み・演出、本作との対応 |

2026-09-16 にユーザーの指示で WebGL 版からこのリポジトリの直下へ移した（.gitignore）。UE5 版はこれらを読み取り専用で参照し、`Tools/` のスクリプトが UE で使える形にする。

### 2.3 素材

| 素材 | WebGL 版の場所 | UE5 版での扱い |
| --- | --- | --- |
| ステージのメッシュ・テクスチャ | `cc2_reference`（glb・png） | `/Game/CC2/…` に CC2 の /Game の木をそのまま写して取り込む（`Tools/cc2`、`Content/Python/wasami_tools`） |
| 差し替えたテクスチャ（ワサミのポスター 10 枚、合成した ORM、デカールの色 + α） | `assets-src/cc2/tex/`（`replace/` ほか） | そのまま取り込む（`/Game/CC2/Generated`） |
| ステージの配置と判断 | `assets-src/cc2/layout.json`・`assets-src/level/stage.json` | UE の単位に戻して使う（`Tools/cc2/prepare_stage.py`） |
| 効果音・曲 | `public/sfx/`（138、`manifest.json`）、`game_sound_effects/`（本家のファン向け音源パック 3,681） | SoundWave / SoundCue に |
| ワサミの声 | `voices/`（wav 原本 112）、`public/voices/`（16 本 + 字幕の `manifest.json`） | SoundWave + 字幕 |
| 敵のモデル | `public/assets/models/wasami_enemy.glb`（アニメ付き）、`models/enemy/`（Meshy の原本 5 本: 待機・歩き・走り・スキップ・回転ジャンプ） | スケルタルメッシュとアニメに |
| シャード | `models/wasami_mochi.glb`（原本 90 MB）、`public/assets/models/wasami_mochi.glb`（6000 三角形） | スタティックメッシュ |
| オーブ | `public/assets/models/power_orb.glb`（本家のメッシュ） | スタティックメッシュ |
| UI の素材 | `public/title/`（45、本家の UI テクスチャと本作のロゴ・ワサミの頭）、`public/tablet/`、`public/fx/`、`assets-src/pause/wawa.png`、`assets-src/stage/title.png`（題字 Stinky Gachimi）、`public/wasami.webp` | UMG のテクスチャ。本家のものは `pak_reference` の原本（png）から取り直せる |
| フォント | `public/fonts/`（helvetica-neue-bold・helvetica-normal・Roboto） | UE のフォントアセット |

### 2.4 運用のルール

日本語での対話、進捗記録（`.claude/progress/`）、原作への忠実さ、実装ごとのコミットは `.claude/guides/` に UE 向けに直して移した。WebGL 版の実装記録（`implementation-records/`、records:check のハッシュ同期）は TypeScript のソースに結び付いた仕組みなので移さず、UE5 版では C++ / Python / アセットに合わせて作り直す。

## 3. 引き継がないもの（UE の仕組みに置き換わる）

WebGL 版の多くの工夫は「ブラウザで UE の見た目と挙動を再現する」ためのもの。UE5 版では本家・CC2 と同じエンジンの機能をそのまま使えるので、値だけを持ち越し、仕組みは作り直す。

| WebGL 版 | UE5 版 |
| --- | --- |
| UE 4.21 のトーンマップ・ブルーム・SSAO・SSR・霧・DOF の再実装（`render/*`、LUT の焼き込み） | UE のポストプロセス。CC2 のボリューム（`PostProcessVolume3`・`4`）の設定値と LUT をそのまま入れる |
| Blender のライトマップ焼き直し、近い 3 灯だけの影、灯の状態の差分シェーダー | Lumen（GI・反射）と 301 灯のリアルタイム。灯の値は CC2 の単位（cd・減衰半径・矩形の大きさ）そのまま |
| KTX2、manifest、ストリーミング、Cloudflare Pages の 25 MiB 上限 | UE のテクスチャストリーミングとパッケージ |
| Havok のキャラクターコントローラ、CharacterMovement の計算の移植（`walk.ts`） | `UCharacterMovementComponent` そのもの（本家の値を設定するだけ） |
| 衝突の 10 cm 格子（`colliders.json`）、敵のナビ格子（`navgrid.ts`） | メッシュの当たり（CC2 の BodySetup: 複雑な当たりか単純な形）と NavMesh |
| HTML / CSS の UI（`hud/*`、`styles.css`） | UMG（本家も UMG なので、`pak_reference` のウィジェット木のアンカー・オフセット・アニメをそのまま移せる） |
| WebAudio の減衰の再現 | SoundCue と減衰設定（CC2・本家の値） |
| カメラアニメ・シェイクの 60 fps サンプル（CSV） | UE のカメラシェイク（本家の振幅・周波数）とカーブ |
| Playwright のベンチ・flow-check | UE の Automation / Functional Test と MCP の PIE 操作 |

## 4. UE5 版の方針

- **エンジン**: UE 5.8.2（`C:/Program Files/Epic Games/UE_5.8`）、C++ プロジェクト `wasami_deception`。
- **作り方**: ゲームの中核（プレイヤー、ゲームの規則とセーブ、敵の AI、ギミック）は C++。UI は UMG（C++ の基底 + ウィジェット BP）。調整値はデータアセット。ステージの素材は参照データからスクリプトで取り込み、何度でも作り直せるようにする。
- **エディタの操作**: MCP（`unreal-mcp` のツールセット）が基本。取り込みやステージの組み立てはプロジェクトの Python ツールセット（`Content/Python/wasami_tools`）として MCP から呼ぶ。C++ は Live Coding か、エディタを閉じて UBT でビルド。
- **描画**: Lumen（ソフトウェアのレイトレース）、仮想シャドウマップ。この PC の GPU（GeForce GTX 1660 SUPER、4 GB）は RT コアが無いので、ハードウェアのレイトレースは切る。
- **目標**: この PC（i7-9700K、32 GB）で 1080p・60 fps 前後。スケーラビリティで調整する。

## 5. 進め方（マイルストーン）

| # | 内容 | 主な根拠 |
| --- | --- | --- |
| M0 | 基盤: 運用ルール、取り込みの仕組み、プロジェクト設定 | — |
| M1 | ステージ: Zone_1 のメッシュ・マテリアル・灯・ボリューム・霧・空・デカール、当たり、NavMesh | cc2_reference、layout.json |
| M2 | プレイヤー: 移動・FOV・頭の揺れ・足音・180°・視線の手のマーク、ブースト、テレポーテーション | pak_reference（`BP_DD_PlayerCharacter`、`BP_Power_Teleport`） |
| M3 | 流れ: シャード、チェックポイントとセーブ、ライフと死亡、障壁、配電盤、脱出、クリア | CC2 のレベル BP、本家の GameMode |
| M4 | 敵: ワサミ（スケルタル・アニメ）、巡回・発見・追跡・グリッチ・捕獲、特別な敵 | 本家の `BP_Monkey`、WebGL 版 15 記録 |
| M5 | ギミック: 扉 6 種、罠の扉、噴出、車、街灯、脱出のトラックと群れ | CC2 の BP |
| M6 | UI: タイトル、OPTIONS、ポーズ、死亡、脱出の画面、ステージ OP、タブレットと地図、字幕、SAVING | 本家の UMG |
| M7 | 音: 曲、効果音、声、減衰 | CC2・本家の SoundCue |
| M8 | 仕上げ: 性能、パッケージ、テスト | — |

## 6. 決まったこと（2026-09-16 のユーザーの回答）

- エディタは Claude が閉じて開き直してよい（C++ のビルドと設定の反映のため）。
- 参照データから作り直せる素材（`/Game/CC2` など、数 GB）は git の外に置く。手で作るアセットは Git LFS で扱う（`.gitattributes`）。
- コミットと push は WebGL 版と同じ運用（実装ごとにコミット、条件を満たすと main を自動で push。`.claude/guides/git-workflow.md`）。
- 参照データ（pak_reference・pak_reference_2・cc2_reference）はこのリポジトリの直下へ移した（`.gitignore`）。WebGL 版の派生データ（`assets-src/cc2/layout.json`・`assets-src/level/stage.json`・`assets-src/cc2/tex/`）と調査の資料（`.claude/references/`）は WebGL 版の場所から読む。
