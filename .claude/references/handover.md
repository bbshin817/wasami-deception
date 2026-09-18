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

- あらゆる要素は本家に基づく。**ステージも 2026-09-16 から本家の病院（`pak_reference_2` の `06_Hospital_Zone_01`・`06_Hospital_Zone_02`）**（ユーザーの指示で CC2 から変更。例外は無くなった）。
- 本家に無いギミックは複雑にしない。壊せる物は本家のホテルの板張りのバリケード（`BP_01_Woodboards`）のように 1 クリックで崩れて自然に消える。初見で案内なしに遊べる。
- ファンゲームのしゃがみ・スライディング・パンチ・ロックピックの連打・床の案内の文字は採らない（2026-09-15 の決定）。
- テレポーテーションとモーションブラーは旧版の `pak_reference`（UE 4.21）に従う。ステージのボリュームのモーションブラー 0 は採らず本家の既定 0.5。
- 原作とファンゲームのロゴ・キャラクターのモデルは使わない。人物の描かれたポスターと落書き 10 枚はワサミの絵に差し替えたものを使う。敵はワサミ。
- ~~Telekinesis は実装しない~~（2026-09-16 に取り下げ。パワーは 6 種すべて作る。`.claude/guides/original-fidelity.md`）。ライフは 3（ファンゲームは 5）。制限時間は無い。
- 本作独自として保留中の要素（敵がワサミの声で話す、知覚の弱さなど）は `07-wasami-mapping.md` の「保留」の表。
- 根拠は**原則としてコード（原作データ）**。視覚的な比較が要るときは、この PC に入っている遊べる本家（`pak_reference` と同一のビルド）で観察してよい（2026-09-16 の指示）。

### 2.2 根拠のデータ（このリポジトリの直下に移した。git の対象外）

| データ | 場所 | 大きさ | 中身 |
| --- | --- | --- | --- |
| 本家の原作データ（UE 4.21） | `pak_reference/` | 2.9 GB | BP バイトコード、全アセットのプロパティ、UMG、カメラ、SoundCue、パーティクル、テクスチャ、音、レベル配置。読み方は同フォルダの README |
| 本家の新しい版（UE 4.24） | `pak_reference_2/` | 18.3 GB | 後のチャプターと追加の効果。旧版と違うものはユーザーに確認 |
| 手元で遊べる本家・旧版（`pak_reference` と同一のビルド） | `C:\Users\User\AppData\Local\DDeception\Launch-Classic-Ch3.cmd`（本体は Steam の `Dark Deception Classic Ch3`） | 1.9 GB | 実機。REPLAY で 5 ステージ（`01_Hotel`・`02_ElementarySchool`・`03_Manor_Zone1`・`04_Sewer`・`05_Circus_Entrance`）。モーションブラーと旧版のテレポーテーションはこちらで観察 |
| 手元で遊べる本家・最新版（`pak_reference_2` と同一のビルド。2026-09-16 に Steam から入れた） | `C:\Users\User\AppData\Local\DDeception\Launch-Latest.cmd`（本体は Steam の `Dark Deception`） | 7.9 GB | 実機。**病院（`06_Hospital`・Zone 1・Zone 2）と新しい版のタブレットの効果はこちらで観察**。REPLAY に Torment Therapy・Mascot Mayhem が増える（Ch5 は未所有） |
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
| M1 | ステージ: 病院 Zone 1・Zone 2 のメッシュ・マテリアル・灯・霧・空・ポストプロセス、当たり、NavMesh | pak_reference_2 の `_levels/06_Hospital_Zone_0*` |
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

## 7. 現状（2026-09-19 の作業の終わり）と次の一歩

できたこと:

- **タブレットのパワー 6 種（2026-09-17、作業一覧の項目 1。`feature/tablet-powers` を main へマージ）**: Speed Boost（演出つき）・Teleport（旧版 `pak_reference` に従う。照準・移動・取り消し・カメラアニメ）・Telepathy（壁越しの印）・Primal Fear・Telekinesis（シャードの引き寄せと力場の粒子）・Vanish が、Lv5 固定で最初から使える（04 記録）。Q / E で使い、1 / 2 で枠を切り替える。シャードの最小限（ワサミ餅 337 / 342、回収・閃光・引き寄せ。06 記録）もこの作業で作った。原作のグラフが cook で消えた材質は推定で組み、最新版の実機の収録と同じ条件で見比べて値を決めた（`observations/README.md`）。敵に効くパワー（Primal Fear・Vanish・Telepathy）は、2026-09-19 に敵ワサミ（項目 7）でも確かめた。残った差と仮の値は、04 記録の「既知の制約・注意点」にある（テレキネシスの灯と材質、Telepathy の印は作業一覧の項目 23 で詰める）。
- **マウスの視点の速さと集中線の修正（2026-09-17、作業一覧の項目 2。`feature/look-and-speedlines` を main へマージ）**: 視点が遅すぎたのは、Enhanced Input がマウスの対応づけに `AxisConfig` の感度 0.07 を自動で重ね、C++ の Scalar 0.07 と二重に掛かっていたため。Scalar を外し、最新版の実機で測った 0.175°/カウントと一致させた（02 記録・症状索引）。スピードブーストの集中線がノイズに見えたのは `M_Speedlines` の FlipBook が既定の 2 × 2 だったためで、実機の連写に合わせて 2 × 5・30 コマ/s にした（04・01 記録。測り方は `observations/README.md` の「視点の速さと集中線」）。
- **ワサミシャードの光（2026-09-17、作業一覧の項目 3。`feature/shard-glow` を main へマージ）**: 置かれている餅が紫に明滅し（`M_DD_WasamiMochi` の自己発光に紫の波。位相は個体ごとの乱数）、回収の閃光が紫でやや弱い本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash` になった（原作のパスの `P_ky_flash3` は変えない）。本家の紫の灯はそのまま。閃光の係数は 2026-09-17 にユーザーが確定し、餅の明滅は同じ回答で外すことになった（作業一覧の項目 22: 明滅をやめ、1.5 倍にして本家のように回す）（06 記録。測り方は `observations/README.md` の「紫の回収の閃光」ほか）。
- **敵ワサミの素体（2026-09-18、作業一覧の項目 4。`feature/enemy-wasami-body` を main へマージ）**: ユーザーのモデル `enemy_wasami_v3.glb`（と捕獲の 3 本だけ旧 glb）を `SourceArt/Wasami/` に置き、`WasamiDDTools.import_wasami_enemy` が `/Game/Wasami/Enemy` に `SK_WasamiEnemy` と役の名前のアニメ 18 本を作る（glb の 24/30 fps の混在を 30 fps に標本化し直す、気絶を倒れる 2 本〈`BeHit_FlyUp`・`Knock_Down`。ランダム〉と、寝返り〈0.8 s。取り込みで作る〉+ `push_up_to_idle` の起き上がり 2 本にする〈2026-09-18 のユーザーの指示。`feature/enemy-stun-knockdown` を main へマージ〉、追跡中の変化をその場の形にする、旧モデルの 3 本を骨のひねりの対応で載せ替える、`Chase_VaultLand` を床から跳ぶ形にする）。`AWasamiEnemy` は本家のナース `BP_06_ReaperNurse` の値（カプセル・速さ 350 / 800・`CanSpawn`・0.5 s ごとの判断・17 s の気絶・`IWasamiEnemyInterface`）を持ち、`UWasamiEnemyAnimInstance` が本家のナースの ABP の木と全身の 1 回再生（捕獲と追跡中の変化の口）を持つ（07 記録）。PIE で立ち姿・巡回・追跡・Nightmare・気絶と明け・Telepathy・Vanish・1 回再生 9 本を確かめた（`observations/README.md` の「敵ワサミ」）。敵の出現と AI は項目 7（下）。再生の速さ・Nightmare の切り替え・待機の選択・`Chase_VaultLand` の形は作業一覧の「未回答の要確認」。
- **ワサミシャードの見た目（2026-09-18、作業一覧の項目 22。main で作業）**: 餅の紫の明滅を外して自己発光を `Glow` 0.3 だけに戻し、大きさを 0.55 → 0.825 m（1.5 倍）にし、モデルをユーザーの `wasami_mochi_v3`（101,368 三角形・テクスチャ 2048²/2048²/4096²・Nanite）に替えた。Zone 1 の餅 337 個でフレーム時間は変わらず VRAM が +0.20 GB だったので、開発中のテクスチャの上限は設けない。回り方は本家の結晶の収録と同じ撮り方で見比べ、**速さは個体ごとの乱数（10.8〜32.4 °/s）のままにして直さない**と決めた（本家の `BP_Shard` が同じ乱数を引く。本作 27.1 °/s、本家の実機の 1 個は 17.1 °/s、どちらも同じ範囲）。本家の紫の灯と回収の閃光は変えていない（06 記録の「餅のモデル」「餅の回り方」、`observations/README.md`）。
- **ゲームの流れの土台（2026-09-18、作業一覧の項目 5。大目標 1 の最初の項目。main で作業）**: ライフ 3 とゲームオーバー、死亡画面（本家の `UMG_DeathScreen` を写した C++ のウィジェット。ドクロが 1 つ減る・赤い揺れ・ヒント）、最後のチェックポイントの PlayerStart で今のレベルを開き直す再開（回収したシャードは戻らない）、チェックポイントの保存と右下の SAVING PROGRESS、ゲームオーバーの RESTART / LAST CHECKPOINT（はじめてだけ S ランクの警告）/ QUIT TO TITLE（タイトルができるまでは Zone 1 の最初から）ができた（06・09 記録、原作の調べは `.claude/references/game-flow/README.md`）。**死ぬ場面とチェックポイントを通る場面はまだ無い**（項目 9・6・13）ので、PIE のコンソールで `Wasami.Kill`・`Wasami.Checkpoint N`・`Wasami.Lives N`・`Wasami.ResetSave`（後に `open L_Hospital_Zone1`）で確かめる。死亡画面の声は項目 20。
- **ゾーンの進行（2026-09-19、作業一覧の項目 6。`feature/zone-progression` を main へマージ）**: 本家の 2 つのレベル BP の代わりに、ゲームモードが開始時に区間の流れ（`AWasamiZone1Flow`・`AWasamiZone2Flow`）を出し、トリガーの箱・チェックポイントの保存・目的の文・タブレットの矢印で区間を進める（11 記録）。Zone 1 はエレベーターの到着（本家のシーケンスを取り込んで再生）→ 扉の破壊（F の連打）と両開き扉 → 迷路で COLLECT ALL SHARDS → 全回収で障壁が壊れ、駐車場へのフェード → トンネルの扉が閉ざされ 25 s 後に破られる → ガレージリフトで上がり、テレポーテーションで救急車の屋根へ → 救急車が走り出して読み込み画面 → Zone 2。Zone 2 は独房（棘が下りる中で扉の鍵を外す）→ ミニボスの廊下 → 迷路（乗ると上がる床 15 台、階ごとの地図）→ 全回収で COLLECT THE RING PIECE まで（08・09・12・03・01 記録）。作業一覧の「ガレージリフトで Zone 2 へ」は本家のコードどおり救急車で移る形に読み替えた（作業一覧の「未回答の要確認」）。敵は項目 7（下）で入った。遊んで確かめるときは `Wasami.ResetSave` の後に `L_Hospital_Zone1` で PIE、先へ飛ばすのは `Wasami.CollectShards`・`Wasami.Trigger <箱の名前>`・`Wasami.Flow <イベント>`、Zone 2 の独房からは `Wasami.Checkpoint 7` の後に `L_Hospital_Zone2`（手順の例は 11 記録の「確かめたこと」の通し）。
- **NavMesh と敵の AI（2026-09-19、作業一覧の項目 7。`feature/enemy-ai` を main へマージ）**: 両ゾーンに本家のナビのボリュームで NavMesh を焼き（01・00 記録）、敵ワサミ `AWasamiEnemy` が本家のナースの `Make Choice` を写した 0.5 s ごとの判断と AI MoveTo で、巡回（プレイヤーの周り 3000 の乱数の点へ 350 cm/s）→ 発見（前 100° 未満・`Camera` の線）→ 追跡（800 cm/s。見失うのは Vanish と 17 s の気絶だけ）をする（07 記録）。区間の流れが出す（11 記録）: Zone 1 の迷路の 3 体と駐車場の 06 型 2 体（毎ティック追い、トンネルの扉を突く）、Zone 2 の迷路の 3 体（階が違うとリフトで上下する。12 記録）とミニボスの廊下の見張り 6 体（視界コーンが 10 s ずつ点いて消え、中に入ると跳び降りて追う。タブレットの地図に扇が出る。03 記録）。Primal Fear・気絶・Vanish・Telepathy が敵に効く。**捕まる所はまだ無く**（項目 9）、敵はプレイヤーの手前（中心の間 約 90 cm）で止まる。全回収の後の Nightmare は本家の病院に無いので作っていない（作業一覧の「未回答の要確認」）。遊んで確かめるときは、Zone 1 の迷路が `Wasami.Checkpoint 5`、Zone 2 のミニボスの廊下が `Wasami.Checkpoint 8`・迷路が `9`（それぞれ後に `open L_Hospital_Zone1` / `L_Hospital_Zone2`）。


- M0: 運用ルール（`CLAUDE.md`、`.claude/guides/`）、参照データの移動、git（main、Git LFS、origin へ push 済み）。
- ステージ: CC2 の Zone_1 を取り込んで組み立てるところまで作ったが、**2026-09-16 の方針変更（ステージを本家の病院へ）で削除した**（`/Game/CC2`・`L_Zone1`・`Tools/cc2/`・`cc2_assets.py`・`cc2_level.py`）。実装は git 履歴（`f2ad354` 以前）に残っているので、必要なら取り出せる。
- M1（病院のステージ）: `python Tools/dd/prepare_stage.py` → `WasamiStageTools` で `/Game/DD` にメッシュ 64・テクスチャ 282・マテリアル 143（マスターは `M_DD_Substance`・`M_DD_Decal`・`M_DD_Unlit`）、レベル `L_Hospital_Zone1`（アクタ 2,059 = 配置 923・灯 1,120・反射キャプチャ 10・霧・スカイライト・プレイヤースタート 4）と `L_Hospital_Zone2`（配置 819・灯 751）。PIE での確認はユーザーが実施し「概ね問題なし」（2026-09-16）。
- M2: タブレット（2026-09-16）。板（本家のメッシュ、カメラの子、出し入れは本家の 2 本のタイムラインのカーブそのまま）、画面（`UWasamiTabletWidget`。本家 `UMG_Tablet` の px で背景・シャード数・パワー枠・地図・プレイヤーの印・目的の帯・Z）、ミニマップ（レベルに置いた地図の板をプレイヤーのシーンキャプチャが撮り、`T_NewMap` を画面に映す。Z で `OrthoWidth` 4000 ↔ 10000）。素材は `WasamiDDTools.import_dd_tablet` が `/Game/DD` に 30 個作る。PIE で画面の位置・色・パワー枠の扇形・地図の拡縮を確かめ、ユーザーが手元で出し入れ・音・地図の追従を確認済み。歩くと画面上で揺れていた問題は、板を毎フレーム視点（カメラマネージャの POV）に置き直す形にして直した（02・03 記録）。
- M2 の始まり: C++ の `WasamiGameMode` と `WasamiPlayerCharacter`（本家の値: カプセル 50 / 88、SpringArm (0, 0, 95)・長さ 0・回転ラグ 20、歩き 300・ダッシュ 600・ブースト 870 cm/s を 6.75 s・再使用 8.5 s、速さに連動する FOV 90→115〈本家の 0.001 s のタイマー〉、頭の揺れ〈本家の歩き・走りのシェイクを `/Game/DD` に作った LegacyCameraShake〉、中クリックの 180°、マウスの軸〈感度 0.07・UE4 の FOV スケーリング・スムージング〉）。PIE でホームに出て、各値が本家どおりなことを確かめた。

- 本家の実機（2026-09-16）: 旧版（`pak_reference` と同一）と最新版（`pak_reference_2` と同一。Steam から入れた）が同居。画面操作は Claude が `Tools/desktop.py`（セッション 1 の常駐エージェント）で行える。ユーザーが用意した MOD（Simple Mod Menu v3.1.3）の **Maps** から`06_Hospital_Zone_01` の「ZONE 1 STARTING POINT」へ直接飛べる（`clvl` で確認済み）。**Zone 1 の実機の画面と色を測って `observations/README.md` に残した**（タブレットの上の帯 (32,32,31)・地図の黒 (0,0,0)・通路の線 (93,93,92)、ステージは中央値 (44,38,39)・天井の灯 (227,236,235)・赤い壁 (68,33,33)・床 (59,83,84)）。

まだ確かめていないこと・課題:

- 実際の入力での動き（歩く・ダッシュ・FOV の広がり・頭の揺れ・180°・ブースト・タブレットの出し入れと音）。エディタが背後にあると 3 fps ほどに落ち、ビューポートが描かれないので `UWidgetComponent` も描き直されない。リモートの疑似操作（`LaunchCharacter`）では速さが出ず確かめられなかった。PIE で触るか、入力を流す Automation テストを作る。
- ~~**Zone 1 の露出**~~ → **2026-09-16 に直して確かめた**（00 記録の「露出」）。原作の Zone 1 にポストプロセスボリュームは無く、露出はプロジェクト設定で決まっていた（`r.DefaultFeature.AutoExposure=False` で露出 1.0 に固定）。同じ設定を `Config/DefaultEngine.ini` に写した。決め手はタブレット（UMG の決まった色なので灯の差が混ざらない）: **上の帯 162 → 34**（実機 32）、**地図の黒 (4,3,2) → (0,0,0)**（実機も (0,0,0)）。ステージも PIE で中央値 (41,38,25)・最大 231 となり、実機の (44,38,39)・天井の灯 227〜236 とほぼ合う。
- ~~**色温度の差**~~ → **2026-09-16 に主因を突き止めて直した**（01 記録の「灯」）。絵を並べると差は青の不足ではなく「本作だけ暖色のもやが画面を覆っている」ことだった。原作の Zone 1 の灯 1,120 個のうち **1,015 個は Static（焼き込み）** で、エンジンは焼かれた Static の灯を動的に描かず（`FLightSceneInfo::ShouldRenderLightViewIndependent`）、ボリューメトリック フォグの注入からも外すため、原作では天井灯 294 個の `VolumetricScatteringIntensity` 20.0 もシャード 337 個の 2.5 も効いていなかった。本作は何も焼かないので全部 Movable になり、その値がそのまま効いてもやになっていた。取り込み（`dd_level.py` の `BAKED_LIGHT_MOBILITY`）で 0 にし、両ゾーンのレベルに適用済み。PIE で撮り直すともやは消え、天井のパネル灯・扉枠の青い灯・床のタイルが見えるようになった（`observations/ours/zone1-corridor-pie-nofoghaze.png`）。**後の訂正（同日）**: 「1,015 個は Static」は誤りで、原作の灯の大半は Stationary だった（書き出しが土台と同じ Mobility を省いていた。01 記録）。いまは原作どおり Stationary の灯の散乱を残し（`BAKED_LIGHT_MOBILITY` は無くなった）、焼き込みと合わせた絵で実機と比べている（下の「色味」）。「青い灯」も R と B の取り違えで、本当は赤。
- **スカイライトは寄与がほとんど無い**ことも確かめた（強度 0 / 0.5 / 5 / 50 の振りで、原作の 0.5 では平均が 0.05 も動かない）。原作の `HDRI_Epic_Courtyard_Daylight` は書き出しが 512×512・8bit の 1 面だけで復元できず、シーンのキャプチャに落ちている。同じアセットは Epic の StarterContent のものだが、この PC の UE 5.8 には入っていない（要るなら Epic Games Launcher から足してもらう）。
- ~~**明るさ**~~ → **2026-09-16 に原作と同じ焼き込みへ切り替えて解決した**（00 記録の「灯の焼き込み」）。本作の間接光は**実質ゼロだった**（PIE で Lumen の間接光を切っても平均輝度が 18.8 → 16.2 と動くだけ）。原因はステージ本体が結合された巨大メッシュ（最大 270 m）で、RT コアの無いこの PC では Lumen がメッシュの距離フィールドをたどるしかなく、1 メッシュ 256 ボクセルの上限で 1 ボクセルが 50 cm を超えて壁も廊下も潰れていたこと。原作は `r.AllowStaticLighting=True` で 1,015 個の Static の灯を焼いており（`VisibilityId` 963 個・`LightGuid` 665 個が証拠）、本作も同じにした: `r.AllowStaticLighting=True`・`r.GenerateMeshDistanceFields=False`・`r.DynamicGlobalIlluminationMethod=0`・`r.ReflectionMethod=2`、灯とメッシュの Mobility は書き出しのまま、ライトマップ UV は原作のものを使い（結合メッシュ 7 個だけ UE に作らせる）、解像度は表面積から 1 テクセル 20 cm で決める。**Zone 1 のビルドは 122 秒**（ライトマップ 45.2 MB）。実機の開始地点と同じ視点で、画面全体の中央値 (10,9,6) → **(23,22,14)**、平均輝度 18.8 → **32.2**、床 (39,34,24) → **(68,63,45)**。**ここに書いた焼き方の細部（Mobility、ライトマップ UV と解像度、所要時間）は同日のうちに原作に揃え直した**。正本は 00 記録の「灯の焼き込み」。
- ~~**色味**~~ → **2026-09-16 にひと区切りつけた**（M1 の残り。00 記録の「灯の焼き込み」、数値は `observations/README.md`）。実機の開始地点と同じ視点の PIE で、**画面全体の平均輝度 68.7（実機 58.1）、色相は一致、どの領域も実機の 1〜2 割増しまで**合った。それまでの差の原因は 4 つで、どれも原作のデータの読み違いだった（01 記録）:
  (1) `M_DD_Substance` のコンパイル失敗で、病院の壁・床が既定のマテリアル（灰色の市松）で描かれていた、(2) 灯の色（FColor）の R と B の取り違え（天井灯は本当は青白、扉枠の灯は本当は赤）、(3) Mobility の省略を Static と読んでいた（本当は土台の値 = 灯の大半は Stationary）ことと、ステージ本体の `bCastShadowAsTwoSided` の取りこぼし、(4) ライトマップの解像度と UV を原作のメッシュの値にしていなかった（原作の結合メッシュは UV1 が全頂点 0 で、ステージ本体の間接光は 1 点の値）。
  残りは床の手前（77 対 57）・エレベーターの壁（左 56 対 44、右 68 対 59）・天井の中央（195 対 183）。**焼き込みの品質では変わらない**（Preview → High で ±2 以内）。床の中ほどの差（97 対 49）は、実機で正面をふさぐ赤い両開き扉が本作に無いための映り込みで、PIE の中で扉の位置をふさぐと 52 になった。
- **画角**（2026-09-16 に直した）: 原作（UE 4.24）のエンジン既定は `AspectRatioAxisConstraint=MaintainXFOV`、UE 5.8 の既定は `MaintainYFOV`。どちらのプロジェクトの ini にも上書きが無いので、そのままだとこの PC の 21:9（3440x1440）で水平 107°（原作は 90°）になり、何もかも小さく写っていた。`Config/DefaultEngine.ini` の `[/Script/Engine.LocalPlayer]` に `MaintainXFOV` を入れた。16:9 ではどちらでも同じ。
- ~~**焼き込みの残り**~~ → **2026-09-16 に両ゾーンを原作と同じ High 品質で焼いた**（原作の BuiltData の `LevelLightingQuality` が `Quality_High`。Zone 1 は 106 秒・35.9 MB、Zone 2 は 48 秒・20.6 MB）。警告はインポータンスボリュームが無いこととライトマップ UV の重なり（7 メッシュ）だけで、**どちらも原作どおり**なので直さない（01 記録の「既知の制約・注意点」）。焼く手順は git の外の `observations/tools/bake_level.py`（使い方は `observations/README.md`）。**レベルを組み立て直すとライトマップは無効になるので焼き直す**。
- **モーションブラー**: 原作のプロジェクト設定は `r.DefaultFeature.MotionBlur=False`（ゲームに設定項目は無く、BP でも触っていない）ことが分かったが、**2026-09-16 にユーザーが「0.5 のまま（今は変えない）」と決めた**ので写していない。上の 2.1 の「本家の既定 0.5」はそのまま有効。
- MCP の再接続: Docker Desktop が `0.0.0.0:8000` を掴んでいるため、エディタを閉じている間に Claude Code の接続が切れる。開き直した後は `/mcp` で再接続する（`.claude/guides/unreal-workflow.md`）。
- ステージ（病院）の残り: 性能の計測（開始地点だけ測った: PIE の平均 12.7 ms・95 パーセンタイル 16.8 ms、**VRAM 5.1 GB は `.claude/guides/performance.md` の目安の上限**。ステージ全体と Zone 2 はまだ）、動く部品（両開き扉 62・除細動器 23・ガレージリフト・ゾーンの障壁・スピードバリア 4）はまだ静的な配置か未実装。スカイライトと反射キャプチャのキューブマップ（書き出しが平面 PNG で回収不能）と Zone 2 の `ColorGradingLUT`（寄与 0）・`WeightedBlendables` は未対応。マテリアルの `Normal Flatness` は式が cook で消えていて適用していない。

次の一歩: **2026-09-17 から `.claude/roadmap.md`（作業一覧）が正本**。ユーザーが示した最終目標（敵は `enemy_wasami.glb`、本家ホテルの体の捕獲、紫のワサミシャード、本家どおりの特殊シャード、ボス戦なしで Zone 2 のガレージの祭壇とポータルから脱出、〈2026-09-18 から〉入口レベルは作らず Zone 1 のエレベーターの到着から始める・敵は `enemy_wasami_v3`・Matron は `boss_wasami`・餅は `wasami_mochi_v3`・追跡中にランダムの動き、WebGL 版どおりのタイトル・オプション・スコア画面、マウス感度と集中線の修正、除細動器・スピードバリア・秘密）を項目に分解し、2026-09-18 に 3 つの大目標（1 最小の通しプレイ → 2 ゲームとして一通り → 3 本家に忠実に）の節に分けてあり、`/continue` は未完了の進捗記録が無ければ進行中の大目標の節の「未着手」で依存が満たされた最初の項目から始め、大目標を達成したら止まる（2026-09-17 に項目 1〜3、2026-09-18 に項目 4 と項目 22〈ワサミシャードの見た目の変更〉が終わった。項目 23〈パワーの見た目の詰め〉は計画の 1〜5 まで進めて大目標 3 へ移し保留。2026-09-18 に大目標 1 の項目 5〈ゲームの流れの土台〉、2026-09-19 に項目 6〈ゾーンの進行〉と項目 7〈NavMesh と敵の AI〉が終わった。次は大目標 1 の項目 9〈捕獲の演出〉）。下の一覧は 2026-09-16 時点のもの（経緯として残す。0 は作業一覧の項目 1、1〜3 は項目 5〜8・13、4 は項目 21 に含めた）。

2026-09-16 時点の次の一歩（おすすめの順）:

0. ~~タブレットのパワー 6 種をすべて作る~~ → **2026-09-17 に完了**（上の「できたこと」。調査は `.claude/references/powers/`）。テレポーテーション（下の 2）とシャードの最小限（下の 1 の一部）もこの作業に含む。
1. M3: シャード（ワサミ餅）337 個、チェックポイントとセーブ、死亡とライフ、ゾーンの障壁とシャードチェッカー、ガレージリフトで Zone 2 へ。配置はすべて `stage_ue.json` の `actors` にある。
2. M2 の残り: 視線の手のマーク（interact）、テレポーテーション（範囲は `hospital_zone_01_teleport` メッシュ）。
3. M1 の残り: NavMesh、動く部品（扉・リフト・障壁）を静的な配置から作り直す（M5 の準備）。赤い両開き扉（`BP_06_DoubleDoors`）を置いたら、開始地点をもう一度実機と比べる（床の中ほどの映り込みが消えるはず）。
4. 色味の残り 1〜2 割（床の手前・エレベーターの壁・天井）。候補は、マテリアルの `Normal Flatness`（式が cook で消えて適用していない）、Substrate（原作は使っていない）、反射キャプチャ（原作のキューブマップは回収できない）。比べる足場は `observations/README.md`（`tools/pie_pose.py`・`cmp.py`・`pie_cmd.py`）。
