# 実装記録 索引

運用ルールは `.claude/guides/implementation-records.md`。同期チェックは `npm run records:check`。

## 記録一覧

| 記録 | 内容 |
| --- | --- |
| [00-overview.md](00-overview.md) | 全体像。起動シーケンス、フレーム更新順、モジュール依存、設計判断 |
| [01-boot-and-config.md](01-boot-and-config.md) | main.ts の起動、config.ts の全キーと既定値、URL クエリ上書き、index.html の DOM、Vite/TS/npm 設定 |
| [02-core-engine-loader-input.md](02-core-engine-loader-input.md) | エンジン生成（既定 WebGL2、WebGPU は選択制）、manifest に基づくアセットストリーミング、入力 |
| [03-debug-stats.md](03-debug-stats.md) | F3 の FPS / フレーム時間表示 |
| [04-game-state-save.md](04-game-state-save.md) | Game クラスの統括、GameState の規則・イベント、SaveStore、state.test.ts |
| [05-player-controller.md](05-player-controller.md) | Havok PhysicsCharacterController、移動・ダッシュ FOV・ブースト・振り向き・頭の揺れ・足音、テレポーテーション（照準の輪、発動の時間軸） |
| [06-audio.md](06-audio.md) | AudioManager、BGM/環境音/ボイス/字幕、長さを保つピッチシフト |
| [07-world-level-lights.md](07-world-level-lights.md) | 原作の Hotel の glTF + KTX2 + ライトマップ読み込み、colliders.json の衝突、ホテルの部品（祭壇・エレベーター・ロビー・ポータル）、灯と影付き PointLight 3 灯、デバッグフィールド（`?debug.field=true` の検証用の広場）、壊せる物（原作の Hotel の板張りのバリケード `BP_01_Woodboards` の値〈woodboards.ts〉で、駅の柵 `Fence_battle` の区画と板張りの通り道〈出口の板・低い戸口の格子の壁〉が 1 クリックか敵の接触で壊れて消える。ステージのロックピックとダッシュの障壁も同じ時間軸と煙で壊れる〈04〉） |
| [08-world-shards-gate.md](08-world-shards-gate.md) | シャード配置・回収判定・演出、扉（エレベーターの扉・門）の開閉、ロビーのポータル |
| [09-render-postfx-shaders.md](09-render-postfx-shaders.md) | TAA、DefaultRenderingPipeline、モーションブラー、UE のトーンマップ・ブルーム・SSAO・SSR・DOF、テレポートとブーストの画面、シェーダー一覧 |
| [10-hud-tablet.md](10-hud-tablet.md) | 3D タブレットと画面 DynamicTexture、HTML HUD、styles.css |
| [11-minimap.md](11-minimap.md) | 俯瞰 RenderTargetTexture → 読み戻し → 輪郭画像、ズーム、マーカー |
| [12-level-generation-blender.md](12-level-generation-blender.md) | 原作の Hotel の配置の前処理（scripts/hotel/）と Blender の組み立て・ライトマップベイク（build_hotel.py）、出力物 |
| [13-asset-pipeline.md](13-asset-pipeline.md) | ホテルのテクスチャとマテリアル表、ライトマップの KTX2、glTF 組み立て、vendor コピー、効果音コピー、シャード・敵のモデル前処理、UI 素材、各 manifest 形式 |
| [14-bench-and-smoke-tooling.md](14-bench-and-smoke-tooling.md) | Playwright ベンチ、スモークテスト、フロー検証、スクリーンショット |
| [15-enemies.md](15-enemies.md) | 敵 AI: 衝突箱から作るナビゲーション格子（A*・視線判定）、敵の頭脳（徘徊・追跡・見失い・グリッチ・捕獲）、シーン上の敵（モデル・アニメ・位置付きの音・詰め寄り） |

## ソース → 記録 対応表

| ソース | 記録 |
| --- | --- |
| `src/main.ts` | 00, 01 |
| `src/config.ts` | 01 |
| `src/core/overrides.ts` | 01 |
| `index.html`, `vite.config.ts`, `tsconfig.json`, `package.json`, `scripts/check-pages-limits.mjs`, `tests/pages-limits.test.ts` | 01 |
| `src/core/engine.ts`, `src/core/loader.ts`, `src/core/input.ts` | 02 |
| `src/debug/stats.ts` | 03 |
| `src/game/game.ts` | 00, 04 |
| `src/game/state.ts`, `src/game/save.ts`, `src/game/settings.ts`, `src/game/traps.ts`, `src/game/doors.ts`, `src/game/escape.ts`, `tests/state.test.ts`, `tests/settings.test.ts`, `tests/traps.test.ts`, `tests/doors.test.ts`, `tests/escape.test.ts` | 04 |
| `src/player/controller.ts`, `src/player/teleport.ts`, `src/player/teleport-fx.ts`, `src/player/teleport-ring.ts`, `src/player/boost-fx.ts`, `tests/teleport-fx.test.ts`, `tests/boost-fx.test.ts`, `src/core/ue-curve.ts`, `tests/ue-curve.test.ts`, `src/player/walk.ts`, `tests/walk.test.ts` | 05 |
| `src/audio/audio.ts`, `src/audio/pitch.ts`, `src/audio/stage-sounds.ts`, `src/game/stage-music.ts`, `tests/pitch.test.ts`, `tests/stage-music.test.ts` | 06 |
| `src/world/level.ts`, `src/world/lights.ts`, `src/world/stage-lamps.ts`, `tests/stage-lamps.test.ts`, `src/world/trap-fx.ts`, `src/world/door-parts.ts`, `src/world/debug-field.ts`, `src/world/carpet.ts`, `src/world/breakables.ts`, `src/world/woodboards.ts`, `tests/woodboards.test.ts` | 07 |
| `src/world/shards.ts`, `src/world/gate.ts`, `src/world/special-rules.ts`, `src/world/collect-fx.ts`, `src/world/flash-sprites.ts`, `src/world/shard-fx.ts`, `tests/shard-fx.test.ts`, `src/world/specials.ts`, `tests/specials.test.ts`, `src/world/interact.ts`, `tests/interact.test.ts` | 08 |
| `src/render/postfx.ts`, `src/render/shaders.ts`, `src/render/ue-grade.ts`, `src/render/scene-ao.ts`, `src/render/ssr.ts`, `src/render/depth-proxies.ts`, `src/render/stage-luts.ts`, `tests/ue-grade.test.ts` | 09 |
| `src/hud/tablet.ts`, `src/hud/hud.ts`, `src/styles.css`, `src/hud/title.ts`, `src/hud/options.ts`, `src/hud/pause.ts`, `src/hud/death.ts`, `src/hud/stage.ts`, `tests/title.test.ts`, `tests/options.test.ts`, `tests/pause.test.ts`, `tests/death.test.ts`, `src/hud/tablet-anim.ts`, `tests/tablet-anim.test.ts`, `src/hud/stage-intro.ts`, `tests/stage-intro.test.ts`, `src/hud/streak.ts`, `tests/streak.test.ts`, `src/hud/vignette-sides.ts`, `tests/vignette-sides.test.ts`, `src/hud/level-clear.ts`, `tests/level-clear.test.ts` | 10 |
| `src/hud/minimap.ts`, `src/hud/arrow-pointer.ts`, `tests/arrow-pointer.test.ts` | 11 |
| `scripts/cc2/ue.py`, `scripts/cc2/layout.py`, `scripts/blender/build_cc2.py`, `scripts/cc2/colliders.mjs`, `scripts/cc2/raster.mjs`, `scripts/cc2/raster.d.mts`, `scripts/cc2/heightfield.mjs`, `scripts/cc2/heightfield.d.mts`, `scripts/cc2/grading-luts.mjs`, `tests/cc2-heightfield.test.ts`, `tests/cc2-raster.test.ts` | 12 |
| `scripts/encode-ktx2.mjs`, `scripts/build-level.mjs`, `scripts/vendor-babylon.mjs`, `scripts/copy-sfx.mjs`, `scripts/prepare-shard-model.mjs`, `scripts/prepare-enemy-model.mjs`, `scripts/prepare-boost-fx.mjs`, `scripts/prepare-title-assets.mjs`, `scripts/prepare-pause-heads.mjs`, `scripts/prepare-cc2-textures.mjs`, `tests/shard-model.test.ts`, `tests/enemy-model.test.ts` | 13 |
| `scripts/bench.mjs`, `scripts/smoke.mjs`, `scripts/flow-check.mjs`, `scripts/enemy-check.mjs`, `scripts/shots.mjs`, `scripts/launch-browser.mjs` | 14 |
| `src/enemy/navgrid.ts`, `src/enemy/brain.ts`, `src/enemy/enemies.ts`, `tests/navgrid.test.ts`, `src/enemy/jumpscare.ts`, `tests/enemy-brain.test.ts`, `tests/jumpscare.test.ts` | 15 |

対象外: `legacy/`(旧 Three.js / vinext 版、参照のみ)、`public/`(生成物)、`assets-src/`(git 管理外の素材)。

## 新しい記録を作るとき
1. `_template.md` を複写し、番号を続けて命名する。
2. frontmatter の `sources:` に対象ファイルを全て列挙する。
3. この索引の両方の表に追記する。
4. `npm run records:update` でハッシュを登録する。
