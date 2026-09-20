---
title: 項目 28 後回しにした見た目と演出を本家どおりに詰める
status: 進行中
branch: main
base: aeb1667
started: 2026-09-20 17:03
updated: 2026-09-20 17:40
---

<!-- 続きをするのに要ることだけを書く。ステップを閉じるときにその分を畳む（目安 20 KB・上限 30 KB） -->

# 項目 28 後回しにした見た目と演出を本家どおりに詰める（大目標 3）

## 依頼

作業一覧 `.claude/roadmap.md` の大目標 3 の項目 28。大目標 1・2 の「見た目の詰めをしない」決め方のために後に回したものを、本家の実機の観察と原作の式で詰める。完了の条件は、項目 28 の「後回しの一覧」の各行が本家どおりになったか、ユーザーがこのままでよいとしたか、本家どおりにできない（本家でその場面が出せない・本作の体では写せない）理由を要確認に書いて閉じたか。

## 計画

- [x] 1. 後回しの一覧を見直し、大きい 7 行を項目 32・33・34 に立て直す … 2026-09-20 完了。21 行のうち 7 行を新項目へ移し、14 行が項目 28 に残った（規模は 28 = 3、新項目 = 各 2）
- [x] 2. 本家（最新版）の観察の台本を作る … 2026-09-20 完了。下の「撮るものの一覧」。1 回で撮り切れない量なので、収録を 2 回（ステップ 3・4）に分けた
- [ ] 3. 収録セッション 1: Zone 1（A1〜A9）→ 救急車でそのまま Zone 2 へ入って B1・B2 ← 次
- [ ] 4. 収録セッション 2: Zone 2 の残り（B3〜B8）
- [ ] 5. 場面の上下の黒帯 `MM_CutsceneBars`（本家の JSON に式があるので、収録は太さと出入りの確かめに使う）
- [ ] 6. 死亡画面の 2 行（アニメの `RestoreState`・ボタンとヒントの書体）
- [ ] 7. Zone 2 のリフトの乗り方と、気絶した見張りの姿勢
- [ ] 8. 見張りの視界コーンの地図の印（扇と点の材質）
- [ ] 9. Matron の大きさと動き
- [ ] 10. 秘密の部屋のグリッチと書類の縁の光
- [ ] 11. 祭壇の見え方（金属・欠片・球）
- [ ] 12. 場面の 4 行（ナースの演技の尺、消える材質の動き、シネカメラの画角、カメラアニメ）
- [ ] 13. 救急車が走り出すときのトンネルの床の黒い矩形の欠け
- [ ] 14. 締め（一覧の各行の結果を作業一覧に書き、項目 28 を完了にする）

## 次にやること

ステップ 3（収録セッション 1）。下の「撮るものの一覧」の A1〜A9 → B1・B2 を、`.claude/guides/observation.md` の 1〜4・7 の手順で撮る。撮ったものは `observations/README.md` の original の節に表で書く。

## 撮るものの一覧（本家 = 最新版 `Launch-Latest.cmd`、MOD 入り、3440×1440）

**共通の決まり**（`.claude/guides/observation.md`）: 収録は `python Tools/desktop.py record --grab gdi --seconds N --name <名前>.mkv`（全画面は約 10 枚/s）。細かい模様と字形は無劣化の連写 `python observations/tools/burst.py <出力> --count N --delay 0.3 [--region L T R B]`（約 19 枚/s。Claude はセッション 1 にいるので直に走らせられる）。一瞬の演出は MOD のコンソールで `slomo 0.25`（レベルを読み直すと 1 に戻るので飛んだ後は打ち直す）。**同じ画面を 3 回試して撮れなければ飛ばし、理由を記録に書く**。

### 収録セッション 1（ステップ 3。目安 30 分）

起動して Maps（1 枚目）→ TORMENT THERAPY (2033, 523) → 病院の 12 のチェックポイント。

| # | 撮るもの（項目 28 の行 / 項目 33・34） | 場所・出し方 | 速さ | 操作と長さ | ファイル名 |
| --- | --- | --- | --- | --- | --- |
| A0 | 前提の記録 | タイトル → Settings の一覧を 1 枚、ゲーム中に Space でタブレット → 枠のパワーと強化段階 | 1 | `shot` 3 枚 | `obs-a0-*.png` |
| A2 | 死亡画面の行 2「シャードの数の位置」 | **ZONE 1** (1697, 523)（`05_Start` (15, 385)。待合の廊下はシャードだらけ） | 1 | Space でタブレット → シャードを 1 つ踏む。数の所を `--region` で 6 s | `orig-shard-count.mkv` |
| A1 | 死亡画面の行 1「`RestoreState`」・行 2「書体」 | 同じ。着いて 7 s ほどで Reaper Nurse に捕まる（捕まらなければその場で待つ） | 1 | 捕まる前に `record --seconds 20` を始める。死亡画面が出たら Fade In の後も見えているかを見る。続けて `burst.py orig-death-fonts --count 40`（RESTART の字形とヒントの太さ） | `orig-death.mkv` / `orig-death-fonts/` |
| — | 敵を消す | `M` → Active Enemy (990, 303) → Find All (1327, 860) → Remove All (1947, 860) → Find All で 0 体 | — | — | — |
| A4 | 項目 33「除細動器の放電」 | 同じ区間。`BP_06_Defib4` (−5, −796) は cp5 の 12 m 南（廊下をまっすぐ）。**放電に触れると死ぬので 5 m 手前で止まる** | 0.25 | `record --seconds 10` + `burst.py orig-defib-b --count 60`（色・明るさ・稲妻の長さ） | `orig-defib.mkv` / `orig-defib-b/` |
| A5 | 項目 34「特殊シャードの出現・明滅・移動と地図の印」 | 同じ区間。**レベルに入って 155 s**でオーブ（橙）と赤いシャードが出現点へ移る。その後は 150 s ごと（明滅 5 s） | 1（移動は 0.25） | 155 s の前にタブレットを上げて地図を撮る → 出たら地図の印（オーブ・赤いシャード・敵の三角）を `burst` → 近い方へ行って寄り、次の 150 s の明滅と移動を `record --seconds 12` | `orig-special-map/` / `orig-special-move.mkv` |
| A7 | 項目 28「場面のナースの演技」の Zone 1 分（撮れない見込み） | **PARKING LOT** (2047, 523)（`06_Start` (5620, −23410)）。着いたら目的の文を撮る。`REACH THE PARKING LOT` なら `06_CutsceneStart` (4975, −23395) はまだ生きているので歩いて入る。`REACH THE TUNNEL` なら場面は過ぎているので**飛ばして理由を記録に書く**（本家は全回収の後にしか結ばない） | 1 | 生きていれば `record --seconds 25` | `orig-z1-06event.mkv` |
| A8 | 項目 33「扉の破片」「ナースの扉突き」 | 同じ区間。`06_DoorsLock` (7210, −22255) に入ると扉 `BP_06_DoubleDoors33_36` (7201, −22512) が閉まり、ナース 2 体が突き始めて **25 s で破れる**（`Fracture_concrete_5` の破片と煙）。突く 2 体は扉に付いているので追ってこない | 1 | 扉の後ろ（駐車場側）5〜8 m から `record --seconds 32`。破れたらすぐ `M` → Remove All | `orig-doorbreak.mkv` |
| A9 | 項目 28「救急車の走り出しのトンネルの床の黒い矩形」 | 同じ区間。屋根へは `BP_Power_Teleport_Zone_Ambulance` (11245, −20080, 335) にテレポート（右の枠を Teleport にして E → `scroll --dx -120` ×7 → `look --dy 300` → (1720, 720) をクリック）。届かなければガレージリフト `hospital_garage_lift_anim_Anim_2` (11249, −21155) に乗る。`TriggerBox_06_AmbulanceTop` (11245, −20055, 440) に入ると 1 s 後に走り出す | 1 | 前（トンネルの床）を向いて `record --seconds 14`。**走り出して 2〜3 s の床**が要る。7 s で読み込み画面 → 2.5 s で Zone 2 が開く | `orig-ambulance.mkv` |
| B1 | 項目 28「ナースの演技」「消える材質」「シネカメラの画角」「カメラアニメ」、ステップ 5「黒帯」 | A9 からそのまま Zone 2（cp 7）に入る。救急車の到着 6.77 s →（庭を歩いて `Trigger_Arrive_CaptureScene` に入る）→ 捕まる場面 26.23 s → 1 s → 独房の場面 74.07 s | 1 | 到着が始まったら `record --seconds 120`。**スキップの画面が出ても押さない**。見どころ: 捕まる場面の 20.53〜25.27 s（`CameraAnim_Nurse_01` の揺れ）、独房の場面の 23.9〜24.9 s（ナースが消える `Efficiency` 0 → 1）、両方の頭と尻の黒帯 | `orig-z2-cutscenes.mkv` |
| B2 | 項目 33「独房の粒子 4 つ」「黒い塵」 | 場面の後。棘のシーケンスが始まり、**開いてから約 19.2 s で棘が独房の床のプレイヤーの頭に届く**ので、先に扉の鍵（`key f` を 40 回・間隔 60 ms）を外して廊下へ出る（`BP_06_Hospital_DoorBreak_2` (−14145, 1210)）。黒い塵 `Fracture_dark_slow` は棘の **58.8 s** | 1 | 廊下（棘の箱の外）から独房を向いて 55〜65 s を `record --seconds 20` | `orig-cell-particles.mkv` |

### 収録セッション 2（ステップ 4。目安 30 分）

| # | 撮るもの | 場所・出し方 | 速さ | 操作と長さ | ファイル名 |
| --- | --- | --- | --- | --- | --- |
| B3 | 項目 28「Matron の大きさと動き」「見張りの視界コーンの地図の印」「気絶した見張りの姿勢」 | Maps の下の段の**ミニボスの区間**（ボタン名は押して目的の文 `Get past the nurses ` で確かめる。`PlayerStart_MiniBoss` (−10501, −2516)）。机 `hospital_matron_desk_8` (−6901, −1021) の後ろに Matron (−7518, −1051)、長いコーン (−7067, −1025, 676)・短いコーン (−6631, −1025, 518) | 1 | (1) 机の正面から `burst.py orig-matron-height --count 40`（頭の高さと長いコーンの位置）。(2) 机の前へ入って構えの移り方を `record --seconds 12`。(3) 見つかったときの Detected と上半身の向きを `record --seconds 10`。(4) タブレットを上げて地図の扇と点を `burst.py orig-sentry-map --count 30`。(5) 見つかる前に Primal Fear を見張りに当て、棚の上と跳び降りた後の姿勢を `record --seconds 25` | `orig-matron-height/` `orig-matron-pose.mkv` `orig-matron-detect.mkv` `orig-sentry-map/` `orig-sentry-stun.mkv` |
| B4 | 項目 28「祭壇の見え方（球つき）」 | 同じ区間。`ring_statue_2` (−8584, −983) と球 `ring_statue_orb_5`（全回収の前なので球がある） | 1 | 寄って `burst.py orig-altar-orb --count 50`（真鍮の金属・欠片の紫の縁の光・球の紫の渦） | `orig-altar-orb/` |
| B6 | 項目 28「Zone 2 のリフトの乗り方」 | **迷路の区間**（目的 `COLLECT ALL SHARDS`。`PlayerStart_Maze` (−3373, 0)）。入ったらすぐ敵を消す。長い床 `lift_4` (6304, −2270) には (6304, −2810) から +Y へ、角の `lift_7` (4500, −3619) | 1 | (1) 長い床に**歩いて**近づく（縁で段差になるか）→ (2) 走って乗る → (3) 角のリフトに下から乗り、上の階で 10 s 立ち止まる（沈んでは戻るか）。各 `record --seconds 15` | `orig-lift-walk.mkv` `orig-lift-run.mkv` `orig-lift-corner.mkv` |
| B7 | 項目 28「秘密の部屋のグリッチと書類の縁の光」 | 同じ区間の入口。迷路の入口の上の 2 階（z 約 531）へテレポートで上がり、西へ歩いて秘密の壁 `BP_07_Zone1_SecretWall_2` (−3737, 12) の前 (−3560, 0, 503) へ。1 クリックで約 4.3 s かけて上がる | 1 | 部屋に入る・出るを `record --seconds 20`（グリッチの出入り）。中の書類 `BP_Collectable_2` (−4416, 18, 588) に寄って `burst.py orig-secret-folder --count 40`（縁の光） | `orig-secretroom.mkv` `orig-secret-folder/` |
| B8 | 項目 28「祭壇の見え方（欠片）」 | **COLLECTING RING PIECE** (2047, 733)（`PlayerStart_PostMaze` (−1680, 0)）。西へ歩いて祭壇 (−8584, −983) へ。球は消え、欠片が出ている | 1 | 取る前を `burst.py orig-altar-piece --count 40`、取る所を `record --seconds 12` | `orig-altar-piece/` `orig-altar-take.mkv` |

**まだ撮り方が決まっていないもの**: A7（Zone 1 の途中の出来事）は本家が全回収の後にしか場面を結ばないので、`PARKING LOT` で目的の文を見て決める。

## 決定事項

- 2026-09-20: 本家の観察は項目ごとに起こさず、同じ版で撮れるものをまとめる — `.claude/guides/observation.md` の「本家は 1 回・30 分を目安に撮り終えて閉じる」に合わせるため。最新版（病院 Zone 1・2）は項目 28 のステップ 3・4 でまとめて撮り、収録を項目 33・34 でも使う。旧版（ホテル・館）は項目 32・34 で撮る。
- 2026-09-20: 収録を 2 回に分けた（ステップ 3 = Zone 1 →（救急車で続けて）Zone 2 の場面と独房、ステップ 4 = Zone 2 の Matron・祭壇・迷路・秘密の部屋）— 撮るものが 20 を超えて 1 回 30 分に収まらないため。切れ目を救急車の走り出しに置いたのは、そこが本家の Zone 1 → Zone 2 の乗り換えで、A9 の収録がそのまま B1 の場面につながるから。
- 2026-09-20: 書類の縁の光は Zone 2 の秘密の部屋の書類 (−4416, 18, 588) で撮る — Zone 1 の秘密のエレベーター (7494, −3600) は待合の廊下から 85 m あり、秘密の部屋の書類と同じ材質 `MM_Shared_Secret_Folder` なので。Zone 1 の分は時間が余ったときだけ。
- 2026-09-20: 項目 28 の規模は 3 のまま、新しい 3 項目は各 2 にした — 進捗率の母数は 3 → 9 に増える。

## 要確認（ユーザー）

- 2026-09-20: 後回しの一覧の見直し（7 行を項目 32・33・34 へ、残り 14 行を項目 28 のステップへ）と規模の配り直し（28 = 3、32〜34 = 各 2）— 仮に上のとおりにした。理由: 作業一覧の大目標 3 の節の頭「大きいもの（本家の場面を撮って材質か動きを詰める、規模 2 以上の目安）は項目に立て直す」。有人セッションで立て直しを直されたら従う。場所: `.claude/roadmap.md` の大目標 3 の節（項目 28 の一覧と項目 32〜34）。

## 再開時の注意

**ステップ 3・4 は本家を起こす。中断したときの戻し方**（`.claude/guides/observation.md` の 1・7）:

1. `tasklist | grep -i -E "UnrealEditor|DDeception|ffmpeg"` で何が動いているかを見る。
2. 本家が残っていたら Esc → 「やめる」(1720, 1020) → 「デスクトップへ戻る」(1720, 1044)。死亡画面からは「タイトルへ」(1700, 1220) → タイトルの「やめる」(280, 1176) → 「はい」(1562, 944)。どのボタンも押す前に `python Tools/desktop.py look --dx 25 --dy -10` でカーソルを動かす。動かなければ `taskkill`。
3. `python Tools/desktop.py stop`、ffmpeg が残っていないことを確かめる。
4. `python Tools/editor_cycle.py --no-build --no-quit` でエディタを開き直す。

**起動の前に毎回**: 本家の pak が `pak_reference_2` と同一（7,854,848,189 バイト）であることを確かめ、セーブの控えを `/c/Users/User/AppData/Local/DDeception/SaveBackups/pre-obs-<日付>/` に取る（控えは消さない）。`python Tools/editor_cycle.py --quit-only` でエディタを閉じてから（VRAM 6 GB）`python Tools/console_session.py --wait DDeception-Win64-Shipping.exe "C:\Users\User\AppData\Local\DDeception\Launch-Latest.cmd"`。

**MOD のメニュー**: `M` で開閉。左の列は送った位置を覚えているので、押す前に `python Tools/desktop.py scroll` を 8 回で上端へ戻す。**W-Editor (990, 487) は押さない**。押したら `shot --scale 0.3` で確かめてから次へ。

## 検証

- check_records: 未実行（ソースは変えていない）
- C++ ビルド: 未実行
- エディタでの確認（取り込み・組み立て・PIE）: 未実行
