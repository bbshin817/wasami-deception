---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-16 20:35
---

# タブレットから使える特殊効果（パワー）をすべて実装する

## 依頼

2026-09-16 のユーザーの依頼（原文）:「次のタスクは、タブレットより利用可能な全ての特殊効果（テレポーテーション、スピードブースト、テレキネシス等々）を実装することです。実装に必要なステップを見積もり、着手してください。ただし、テレポーテーションの実装に限りpak_referenceを尊重するものとします。」

- 本家のパワーは 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）。調査の結果は `.claude/references/powers/`（README が索引と要点、01〜04 が根拠つきの本文）。
- テレポーテーションだけは `pak_reference/`（旧版）に従う。それ以外と全パワー共通の仕組みは `pak_reference_2/`（最新版）に従う。

## 計画

各ステップの終わりにコミットし、記録を直して、ユーザーに `/clear` をお願いする。

- [x] 1. 原作データの調査をまとめる（`.claude/references/powers/`）、ユーザーに 4 点を確認
- [ ] 2. パワーの土台（C++）
  - パワーの管理（新しいコンポーネント）: 解放済み 6 種の配列、左右の枠の添字（既定は左右とも 0 = Speed Boost。原作のコードどおり）、Q / E で使う、1 / 2 で巡回（タブレットを構えているときだけ、`UI_Select_V3` 1.5 / 2.0）、2 段の 0.5 s の連打防止、使えないときは無音、ゲージ（効果中 1→0・再使用 0→1、0.05 s で 0）、回復で `power_refilled` 0.5、`Active Powers`、強化段階 Lv5 の値の表、死亡時のリセット（Speed Boost・Teleport・Primal・Vanish）
  - 受け口: 敵（`Set State(Stun)`・`Player Vanish`・`No Telepathy`・`Get State`、状態の列挙 Patrol / Pursue / Stun / Teleport）、シャード（`Activate`）のインターフェース
  - タブレット: 6 つのアイコン（`MM_Powers_*` の 6 インスタンス）を枠の添字で出し分ける、`Use Left` / `Use Right` の弾む演出
  - スピードブーストを新しい管理に移す: Lv5 = 950 cm/s・9.75 s・再使用 7.5 s（効果の後）、歩きも 950、`Shard_Streak_Milestone_V5`、`BP_CameraShake_Streak`
  - 素材: アイコン 8 枚（telepathy・primal・telekinesis・vanish の色つきと灰色）、音（`power_refilled`・`Shard_Streak_Milestone_V5`）、シェイク `BP_CameraShake_Streak`
  - 変更予定: `Source/wasami_deception/`（新規 `WasamiPowers*`、`WasamiPlayerCharacter.*`、`WasamiTabletWidget.*`、`WasamiEnemyInterface.*` など）、`Content/Python/wasami_tools/pipeline/dd_tablet.py`（または新規 `dd_powers.py`）、`/Game/DD/UI/RingAltar_UI/Textures`・`/Game/DD/Materials/MasterMaterials`・`/Game/DD/Audio`・`/Game/DD/UI/Menu/Streaks`
- [ ] 3. スピードブーストの演出: `CameraAnim_SpeedBoost` の色調（ブレンド 0.5 s）、`UMG_SpeedBoost`（集中線 `M_Speedlines`・ビネット `T_VignetteNew`）、Chameleon（放射ブラー・画面の揺れ。0〜870 cm/s に連動）
- [ ] 4. テレポーテーションの仕組み（旧版）: Teleport チャンネル（旧版の既定 Overlap）、病院のゾーン（`hospital_zone_01/02_teleport` と救急車の屋根の箱。複雑コリジョン、非表示、QueryOnly）をレベルの組み立てに足す、照準のアクタ（SpringArm のラグ・500 cm の下向きトレース・ホイール・左クリック）、0.12 s 後のスイープ移動（カプセルの WorldDynamic / Pawn を Ignore → Block）、`CameraAnim_Teleport`（FOV・露出・色調を C++ で評価）、シェイク、音 4 つ（照準ループを含む）、照準中の同じ側の Q で取り消し、再使用 5 s、最大距離 1500（Lv5）
- [ ] 5. テレポーテーションの見た目: デカール `M_Decal_Teleport`（推定）、パーティクル `P_ky_cutter2`（Cascade を Python で組めるか確かめる。だめなら Niagara）
- [ ] 6. 一瞬の演出の共通部品（全画面のポストプロセス 2 つとカーブ）と Primal Fear（半径 3500 の重なり判定で敵に `Set State(Stun)`、球 `M_05_Primal`、シェイク `ElevatorShakeStop` ×25、`Stun_Wave_Attack_New_04`、再使用 23 s）
- [ ] 7. Vanish（カプセルの Camera 応答、`Player Vanish` の通知、`UMG_Vanish` と `MM_WobblyVignette`、ポストプロセス、煙 `PPP_VanishPuff`、15 s・再使用 15 s）
- [ ] 8. Telepathy（`BP_Telepathy` とトラッカー、`UMG_TelepathyTracker` と `MM_Telepathy`、開始と終わりの音、シェイク、9 s・再使用 6.5 s）
- [ ] 9. シャードの最小限（M3 の前倒し。最新版の `BP_Shard`）: 病院の配置（Zone 1 は 337、Zone 2 は 342）、回収（数・音 `Soul_Shard_Pickup_v2_Cue` と同時発音 `OnlyFew`・シェイク `BP_CameraShake_ShardCollect`・閃光 `P_ky_flash3`）、`Activate` の引き寄せ（`Shard Pull`、ExpoIn）、タブレットの数と地図
- [ ] 10. テレキネシス（半径 3000 の重なり判定、`P_ky_forceField_Telekinesis`、ポストプロセス、シェイク、音、再使用 8 s）
- [ ] 11. 実機との見比べ（推定したマテリアルとパーティクル）と PIE での確認（エディタの PIE と本家の起動はユーザーの確認を取る）
- [ ] 12. 仕上げ: 実装記録（04 を新設）、handover の「現状と次の一歩」、main へマージして push、この記録を消す

## 次にやること

ステップ 2（パワーの土台）を始める。まず `.claude/references/powers/README.md` と `01-player-system.md` を読み、C++ の構成（パワーの管理コンポーネントと受け口のインターフェース）を決めて、この記録の「決定事項」に書いてから作る。

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

## 再開時の注意

- エディタは起動している（2026-09-16 19:50 の時点で PID 18140、セッション 1）が、このセッションでは触っていない。MCP の接続も未確認。
- 調査の報告の元は scratchpad にあったが、`.claude/references/powers/` に写したので scratchpad は不要。

## 検証

- check_records: 未実行（ステップ 1 はソースを変えていない）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
