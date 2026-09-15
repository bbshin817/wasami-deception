# WASAMI DECEPTION — Claude Code 索引

Babylon.js 9.26(既定 WebGL2、WebGPU は `?render.preferWebGPU=true`)+ Havok + Vite 8 / TypeScript の一人称ホラー探索スライス。
このファイルは `.claude/` 配下リソースの索引だけを持つ。本文はここに書かず、各ファイルを参照する。

## 必読(作業前)
- 対話と報告の言語(**セッション内の対話・応答・実装報告はすべて日本語**): `.claude/guides/communication.md`
- 実装進捗記録(**ファイル変更を伴う作業は `.claude/progress/` に進捗を先に書いてから進める。セッション開始・/clear・圧縮の後は、まず未完了の記録と main 以外のローカルブランチを確認して再開する**): `.claude/guides/progress-tracking.md`
- 全体像・起動順・フレーム更新順: `.claude/implementation-records/00-overview.md`
- 記録一覧とソース→記録の対応表: `.claude/implementation-records/_index.md`
- 実装記録の運用ルール(**ソース変更時は必ず記録を更新**): `.claude/guides/implementation-records.md`
- 検証用ブラウザの運用ルール(**リポジトリ直下の `.is_headless` が `true` なら、検証用ブラウザはミュートかつ画面に出さない(ヘッドレス)。ポインターロックを無効化して Mac のカーソルを動かさず、この Mac の他の作業に影響を与えない**。Playwright スクリプトは `scripts/launch-browser.mjs` で起動する): `.claude/guides/verification-browser.md`
- 原作への忠実さの運用ルール(**本家の原作データは `pak_reference/` にある。実装は基本的に本家に忠実に倣う。スピードブースト・テレポーテーション・タブレット・モーションブラーなど、あらゆる要素は本家に基づく(外部参照によるステージの実装は例外)。本家にないギミックは複雑にせず、壊せる物は本家のホテルの板張りのバリケードのように 1 クリックで崩れて自然に消え、初見でも案内なしに遊べるようにする。原作データで確定できない実装は、本家を入念に観察してから検討し、必要ならユーザーに本家の画面収録を求める。原作のロゴとキャラクターのモデルは使わない(オブジェクトとステージのモデルは使ってよい)。ステージはファンゲーム『Chaotic Customer 2』の Zone_1 で、ステージの根拠はその書き出し `cc2_reference/`**): `.claude/guides/original-fidelity.md`
- コミットとブランチの運用ルール(**実装ごとにコミット、大規模改修は作業ブランチ→main へマージ→ローカルブランチ削除、最終コミットから 10 分経過・未 push 2 件以上・大規模改修のマージ後のいずれかで push**): `.claude/guides/git-workflow.md`
- デプロイの運用ルール(**main への push で Cloudflare Pages にデプロイされる。配信先は mTLS(クライアント証明書)で保護されていて一般には公開されない。検証は手元の dev サーバーで行う。Pages は 25 MiB を超えるファイルが 1 つでもあるとデプロイ全体を失敗させるので、どのアセットも 1 ファイル 25 MiB 以下(新しく作るものは 16 MiB 以下が目安)にし、大きくなるアセットは分割する。`npm test` と `npm run build` が上限を検査する**): `.claude/guides/deployment.md`

## 実装記録(`.claude/implementation-records/`)
| 記録 | 対象 |
| --- | --- |
| 01-boot-and-config | main.ts、config.ts、URL 上書き、index.html、Vite/TS 設定 |
| 02-core-engine-loader-input | エンジン選択、アセットストリーミング、入力 |
| 03-debug-stats | F3 の FPS 表示 |
| 04-game-state-save | Game 統括、GameState の規則・イベント、セーブ、テスト |
| 05-player-controller | Havok キャラクターコントローラ、ダッシュ FOV、ブースト、足音、テレポーテーション |
| 06-audio | WebAudio 管理、BGM/環境音/ボイス |
| 07-world-level-lights | glTF/KTX2/ライトマップ読み込み、壁灯と影付き 3 灯、デバッグフィールド(`?debug.field=true`) |
| 08-world-shards-gate | シャード回収、門の開放 |
| 09-render-postfx-shaders | TAA、DRP、モーションブラー、UE のトーンマップ・ブルーム・SSAO・SSR・DOF、テレポートとブーストの画面、GLSL/WGSL |
| 10-hud-tablet | 3D タブレット、HTML HUD、styles.css |
| 11-minimap | 俯瞰 RTT → 輪郭画像 |
| 12-level-generation-blender | Blender 手続き生成とライトマップベイク |
| 13-asset-pipeline | テクスチャ取得、KTX2 変換、glTF 組み立て、manifest |
| 14-bench-and-smoke-tooling | Playwright ベンチ、スモーク、スクリーンショット |
| 15-enemies | 敵 AI(ナビゲーション格子、敵の頭脳、シーン上の敵、捕獲の演出) |

## 参考資料(`.claude/references/`)
- 原作の実データ(pak を展開・変換したもの。Blueprint のバイトコード、アセットのプロパティ、カメラ、音、テクスチャ、レベル配置など。git の対象外。**実装値の根拠として最優先**): `pak_reference/README.md`
- 原作の実データの新しい版(Steam 版の pak、UE 4.24。後のチャプターと pak_reference に無い効果を含む。同じ形式で変換。git の対象外。**pak_reference に無いものの根拠。両方にあって違うものはユーザーに確認**。旧版との差分は `_diff.json`): `pak_reference_2/README.md`
- 原作 Dark Deception の用語・人物・ステージ・仕組み・演出と、本作との対応(Web 調査。**原作由来の表現を扱う前に参照。記述ごとの信頼度 [A]〜[C] を確認し、実装値の根拠には [A]/[B] だけを使う。pak_reference と食い違えば pak_reference を採る**。records:check の対象外): `.claude/references/dark-deception/README.md`
- ステージの原作データ(ファンゲーム『Chaotic Customer 2』の CUE4Parse の書き出し。`cc2_reference/` = 書き出しの本体、git の対象外。形式・マップ Zone_1 の中身・灯・ボリューム・データから確かめたゲームの流れ。**ステージの実装値の根拠として最優先**): `.claude/references/chaotic-customer-2/README.md`

## 同期の仕組み
- `npm run records:check` … 記録とソースの差分を検出(hooks でも自動実行)
- `npm run records:update` … 記録本文を直した後にハッシュを更新
- hooks 設定例: `.claude/hooks.example.json`(`.claude/settings.json` に貼る)
- スクリプト本体: `.claude/scripts/`

## 開発コマンド
`npm run dev`(5190)/ `npm run build` / `npm test` / `npm run typecheck` / `npm run bench`。
素材の再生成は README「アセットの作り方」と 13-asset-pipeline を参照。
