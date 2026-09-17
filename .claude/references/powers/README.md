# タブレットのパワー（特殊効果）の原作調査

2026-09-16 に、タブレットから使える本家のパワー 6 種を原作データから調べた資料。実装の説明ではない（それは `.claude/implementation-records/`）。本文は 4 つの報告に分けてあり、値にはすべてバイトコードのオフセットやアセット名の根拠が付いている。

| ファイル | 内容 | 主な根拠 |
| --- | --- | --- |
| [01-player-system.md](01-player-system.md) | プレイヤー側の仕組み（枠・入力・連打防止・クールダウン・強化段階・枠の表示）、スピードブースト（最新版）と旧版との違い、`BP_PowerOrb`、敵のインターフェース | `pak_reference_2` の `BP_DD_PlayerCharacter`・`BP_Powers`・`UMG_TabletPowers` |
| [02-teleport.md](02-teleport.md) | テレポーテーション（**旧版**）の全処理、照準のアクタの構成、Teleport チャンネル、病院のゾーン、カメラアニメ・シェイク・デカール・パーティクル・音、最新版との違い、WebGL 版の記録の訂正 | `pak_reference` の `BP_Power_Teleport` ほか |
| [03-telekinesis-vanish.md](03-telekinesis-vanish.md) | テレキネシスと Vanish、シャード側の引き寄せ（`BP_Shard` の `Activate`）、敵の反応 | `pak_reference_2` の `BP_TelekinesisPower`・`BP_VanishPower`・`BP_Shard` |
| [04-primal-telepathy.md](04-primal-telepathy.md) | Primal Fear と Telepathy（最新版のトラッカー方式）、病院のナースの気絶 | `pak_reference_2` の `BP_PrimalPower`・`Powers/Telepathy/*` |

## 版の方針（2026-09-16 のユーザーの指示）

- **テレポーテーションだけ `pak_reference`（旧版）に従う**。最新版の `wtfUE4` と赤い閃光は採らない（2026-09-15 の決定と同じ）。
- それ以外のパワーと、全パワー共通の仕組み（枠・入力・クールダウン）は `pak_reference_2`（最新版）に従う。テレキネシスと Vanish は最新版にしか無い。
- WebGL 版で決めた「Telekinesis は実装しない」は、今回の依頼がテレキネシスを名指ししているので取り下げた。

## パワーの一覧（`Enum_RingAltar_Skills` の並び）

| 番号 | パワー | 何をするか | 枠のゲージ | 実体 |
| --- | --- | --- | --- | --- |
| 0 | Speed Boost | 歩きもダッシュも決まった速さにする | 効果中 1→0、再使用 0→1 | プレイヤーの中 |
| 1 | Teleport | 照準の輪の位置（Teleport チャンネルの床）へ跳ぶ | Q で 0.05 s で 0、移動後 5 s で 0→1 | `BP_Power_Teleport` |
| 2 | Telepathy | 全敵の位置に赤い煙の印を画面に重ねる（壁越し） | 効果中 1→0、再使用 0→1 | `BP_Telepathy` + `BP_TelepathyTracker` |
| 3 | Primal Fear | 半径内の敵を 1 回だけ気絶させる（壁越し） | 0.05 s で 0、再使用 0→1 | `BP_PrimalPower` |
| 4 | Telekinesis | 半径内のシャードを引き寄せて回収する | 0.05 s で 0、再使用 0→1 | `BP_TelekinesisPower` + `BP_Shard.Activate` |
| 5 | Vanish | 15 s 敵に見えなくなる（カプセルの Camera 応答を Ignore） | 効果中 1→0、再使用 0→1 | `BP_VanishPower` + `UMG_Vanish` |

## 強化段階ごとの値（コードの値。段階はセーブの `<パワー> Upgrades`、0〜5）

| パワー | 値 | Lv0 | Lv1 | Lv2 | Lv3 | Lv4 | Lv5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Speed Boost | 速さ (cm/s) | 870 | 870 | 890 | 910 | 930 | 950 |
| | 効果時間 (s) | 6.75 | 6.75 | 7.5 | 8.25 | 9.0 | 9.75 |
| | 再使用 (s、効果の後) | 9.5 | 9.5 | 9.0 | 8.5 | 8.0 | 7.5 |
| Teleport | 最大距離 (cm) | 1000 | 1000 | 1125 | 1250 | 1375 | 1500 |
| | 再使用 (s、移動の後) | 5 | 5 | 5 | 5 | 5 | 5 |
| Telepathy | 効果時間 (s) | 5 | 5 | 6 | 7 | 8 | 9 |
| | 再使用 (s、効果の後) | 8.5 | 8.5 | 8.0 | 7.5 | 7.0 | 6.5 |
| Primal Fear | 半径 (cm) | 1500 | 1500 | 2000 | 2500 | 3000 | 3500 |
| | 再使用 (s、使って 0.06 s 後から) | 35 | 35 | 32 | 29 | 26 | 23 |
| Telekinesis | 半径 (cm) | 1750 | 2000 | 2250 | 2500 | 2750 | 3000 |
| | 再使用 (s、使って 0.06 s 後から) | 10.5 | 10.0 | 9.5 | 9.0 | 8.5 | 8.0 |
| Vanish | 効果時間 (s) | 15 | 15 | 15 | 15 | 15 | 15 |
| | 再使用 (s、効果の後) | 30 | 28 | 24 | 20 | 18 | 15 |

- リングの祭壇でパワーを解放すると段階は 1 になり、1 回の購入で +1（上限 5）。表示用の DataTable（`PowersTable_*` の `Upgrade2`〜`6`）は Lv1〜Lv5 に当たり、コードと一致する。
- **例外: スピードブーストの再使用だけ、最新版のコードは表示と旧版より 1.0 s 長い**（表示と旧版は Lv1 で 8.5 s）。
- 旧版のテレポートの最大距離も同じ表（1000〜1500）。

## 全パワー共通の仕組み（最新版）

- 枠は左右 2 つ。どちらも「解放済みのパワーの配列」の添字を持つ（既定は左右とも 0）。Q = 左、E = 右、1 / 2 = その枠の添字を +1（末尾の次は 0。**タブレットを構えているときだけ**、音 `UI_Select_V3` 1.5 / 2.0）。R は割り当てだけで何もしない。
- 使うと枠を弾ませる（`Use Left` / `Use Right`: 0.05 s で 1.25 倍 → 0.15 s で 1.1 倍 → 0.5 s で 1.0 倍）。これは使えるかの判定より前なので、使えないときも弾む。
- 使えないとき、最新版は何もしない（旧版は `power_not_ready` 0.35、照準中の同じ側の Q はテレポートの取り消し）。
- 連打防止が 2 段: Q / E ごとに 0.5 s、パワーの発動全体で 0.5 s。
- 回復のたびに `power_refilled`（0.5）。死亡して生き返るとき、Speed Boost・Teleport・Primal・Vanish だけ即座に回復する。
- 敵への作用は `DD_EnemyInterface`（`Set State(2 = Stun, byOrb)`・`Player Vanish`・`No Telepathy`）、シャードへの作用は `DD_TelekinesisInterface.Activate`。病院のレベル BP にパワーの特別扱いは無い。

## UE5 で作るときの要点

- `UCameraAnim` は UE5 に無い。テレポートの `CameraAnim_Teleport`（FOV・露出・色調）とブーストの `CameraAnim_SpeedBoost`（色調だけ）は、原作のキーを C++ で評価してカメラに足す（02 の §7）。
- カメラシェイクは `ULegacyCameraShake`（既存の `WasamiDDTools.import_dd_camera_shakes` で作れる）。使うのは `BP_CameraShake_Streak` と `01_Hotel_Lobby_ElevatorShakeStop`（Scale 25）。
- Cascade（`UParticleSystem`）は UE 5.8 に残っている。使うのは `P_ky_cutter2`（テレポート）・`P_ky_forceField_Telekinesis`・`PPP_VanishPuff`。Python から組めるかは未確認。
- ポストプロセスの一瞬の演出（Primal・Telekinesis・Vanish）は、全画面の `UPostProcessComponent` 2 つの `BlendWeight` を同じ形のカーブで動かす。
- マテリアルのグラフが cook で消えていて推定が要るもの: `M_Decal_Teleport`・`M_ky_slash01_4x4`・`PPP_Radial_Gradient_Doffed`・`M_05_Primal`・`MM_Telepathy`・`MM_WobblyVignette`・`M_ky_*`（テレキネシスの粒子）・`MM_Powers`（本作は `M_DD_Powers` で作ってある）。見比べは最新版・旧版の実機で行う（`.claude/guides/original-fidelity.md`）。
- 本作に敵（M4）とシャード（M3）はまだ無い。パワーの側は受け口（インターフェース）まで作り、確認は仮の的で行う。
