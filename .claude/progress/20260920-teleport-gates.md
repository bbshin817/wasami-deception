---
title: 有人セッションの指摘 3 件（テレポートで扉を抜ける・祭壇の見た目・ステージ OP）
status: 進行中
branch: feature/secrets
base: 5736007
started: 2026-09-20 10:30
updated: 2026-09-20 10:30
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB。.claude/guides/progress-tracking.md の「記録を畳む」） -->

# 有人セッションの指摘 3 件（テレポートで扉を抜ける・祭壇の見た目・ステージ OP）

## 依頼

2026-09-20 の有人セッションのユーザーの指摘（原文）:

- 「シャードリングの祭壇の見た目が本家と異なる気がします。私が記憶しているのは、MonkeyBusinessの祭壇なのですが、ここはどうなっている？」
- 「エレベータ、Fロックドアなどがテレポーテーションによってすり抜けられます。」（F ロックドア = F の連打で破る扉 `BP_06_Hospital_DoorBreak` が開ける `BP_06_DoubleDoors11`）
- 「ステージOPがありません。紋章＋ステージロゴのやつ」

## 計画

- [x] 1. 作業一覧に項目を足す … 2026-09-20 完了。大目標 2 の項目 12 の後に項目 30（ステージ OP、規模 2）・31（祭壇の材質、規模 1）。項目 28 の祭壇の行を 31 へ移し、取りやめた 16 に OP を 30 へ移したと書いた
- [ ] 2. テレポートの移動が、道の途中の WorldDynamic の壁（Pawn を止める物。扉・エレベーターの扉）の手前で止まるようにする（`AWasamiTeleportAim::Commit`）。行き先の真下の WorldDynamic の物（救急車の屋根）は除く。テスト、ビルド、PIE で Zone 1 の開始のエレベーターと `BP_06_DoubleDoors11` を確かめる。実装記録 04 と症状索引
  ← 作業中
  - 変更予定: `Source/wasami_deception/WasamiTeleportAim.*`、`Source/wasami_deception/Tests/WasamiPowerTests.cpp`、`.claude/implementation-records/04-powers.md`
- [ ] 3. コミットを main へ cherry-pick し、feature/secrets へ main をマージする（不具合は main で直す決まり。エディタが feature/secrets の C++ で開いているので、直しは feature/secrets で作って確かめてから main へ移す）

## 次にやること

ステップ 2: `AWasamiTeleportAim::Commit` の移動の前に、プレイヤーのカプセルで行き先までを WorldDynamic の物にスイープし、Pawn を止める部品（行き先の真下の物を除く）に当たればその手前を行き先にする。テスト `Wasami.Powers.TeleportGates` を足して `Tools/editor_cycle.py` でビルドする。

## 決定事項

- 2026-09-20: テレポートで扉を抜けるのは本家のデータでも起きる（移動の間はカプセルが WorldDynamic を無視し、扉・エレベーターの扉は WorldDynamic、テレポートの床〈`hospital_zone_01_teleport`〉は扉の両側の廊下を覆う。エレベーターの中は覆わないが、すぐ外は覆うので中から外へ抜けられる）。ユーザーの指摘で本作は塞ぐ — 本家から外れる直しとして実装記録 04 に書く。
- 2026-09-20（項目 31 へ移した）: 祭壇は本家でも Monkey Business と同じ `BP_01_Statue`・`ring_statue`・`MM_00_Ballroom_Ring_Altar_Metal`（ホテルは拡縮 2.8、病院は 4）。本作は材質の親 `MM_Main_Metal`・球の `m_crystal`・欠片の `MM_Main_Substance_Fresnel` が前処理のマスターに無く、`substance` の推定になっている（祭壇が青みの白、球が光らない）。焼き込みのシェーダーは残っている（`MM_Main_Metal` は基底色 (0.276042, 0.255386, 0.148085)・金属 1・スペキュラ 0.5・粗さ `Roughness`・法線 `Normal` を `Normal Flatness` で (0,0,1) へ、発光 = (1 − N·V)^6 × 0.999 + 0.001 × `Hover Intensity` × `Hover Color`）。数ステップなので項目 31 にして無人運転に任せる。
- 2026-09-20（項目 30 へ移した）: ステージ OP は本家の入口 `06_Hospital` のレベル BP が出す `UMG_ChapterPortal`（`Level` 7、11 s で消える）で、入口ごと取りやめた項目 16 に入っていたため抜けていた。WebGL 版 10 記録の `stage-intro.ts` が写してある（頭はワサミ、題字は「Stinky Gachimi」）。数ステップなので項目 30 にして無人運転に任せる。

## 要確認（ユーザー）

（なし）

## 再開時の注意

- エディタは `L_Hospital_Zone1` を開いている（保存済み、PIE なし）。駆動役は止まっている。

## 検証

- check_records: 未実行
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
