---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-17 16:30
---

# タブレットから使える特殊効果（パワー）をすべて実装する

## 依頼

2026-09-16 のユーザーの依頼（原文）:「次のタスクは、タブレットより利用可能な全ての特殊効果（テレポーテーション、スピードブースト、テレキネシス等々）を実装することです。実装に必要なステップを見積もり、着手してください。ただし、テレポーテーションの実装に限りpak_referenceを尊重するものとします。」

- 本家のパワーは 6 種（Speed Boost・Teleport・Telepathy・Primal Fear・Telekinesis・Vanish）。調査は `.claude/references/powers/`（README が索引と要点、01〜04 が根拠つきの本文）。
- テレポーテーションだけは `pak_reference/`（旧版）に従う。それ以外と、全パワーに共通の仕組みは `pak_reference_2/`（最新版）に従う。
- 作業一覧 `.claude/roadmap.md` の項目 1。完了の条件は、ステップ 10a〜12 が済み、この記録が消え、`feature/tablet-powers` が main にマージされていること。

## 計画

完了したステップの中身は、実装記録（00〜04・06）とコミットのメッセージにある。

- [x] 1. 原作データの調査（`.claude/references/powers/`）とユーザーへの確認 4 点
- [x] 2. パワーの土台（`UWasamiPowerComponent`・敵とシャードのインターフェース・タブレットの枠）… 2026-09-16。04 記録
- [x] 3. スピードブーストの演出（`UWasamiCameraAnim`・`UMG_SpeedBoost`・Chameleon の揺れ）… 2026-09-16。04 記録
- [x] 4a. テレポーテーションの仕組み（旧版）… 2026-09-16。04・01 記録
- [x] 4b. テレポーテーションのカメラアニメ（FOV の基準は旧版の実機で t=0 のキーと決めた）… 2026-09-16。04 記録
- [x] 5. テレポーテーションの見た目（Cascade を C++ の道具で組む、推定のデカール・斬撃・火花）… 2026-09-17。01・04 記録
- [x] 6. 一瞬の演出の共通部品 `AWasamiPowerBurst` と Primal Fear … 2026-09-17。04 記録
- [x] 7. Vanish … 2026-09-17。04 記録
- [x] 8. Telepathy … 2026-09-17。04 記録
- [x] 9a. シャードの最小限（見た目はワサミ餅）… 2026-09-17。06 記録
- [x] 9b. シャードの回収の閃光 `P_ky_flash3` … 2026-09-17。01・06 記録
- [x] 10a. テレキネシスの仕組み … 2026-09-17。04・06 記録
- [x] 10b. テレキネシスの粒子 `P_ky_forceField_Telekinesis`（10b1 道具と素材、10b2 推定の材質 4 つと粒子、10b3 C++ の参照と PIE）… 2026-09-17。01・04 記録
- [ ] 11. 実機との見比べ（推定したマテリアルとパーティクル）
  - [x] 11a. 最新版の実機の観察 … 2026-09-17。収録は `observations/original/orig-*`、測った値は `observations/README.md` の「パワーの演出」
  - [ ] 11b. PIE で同じものを撮って見比べ、仮の値（`TODO(仮)`）を直す。次の 2 つ以上に分ける（2026-09-17）:
    - [ ] 11b1. **作業中**。PIE で下の項目 1〜7 を本家と同じ条件で撮り、同じコマンドで測って `observations/README.md` の ours に並べる（値は変えない）。変更するファイル: `observations/README.md`、この記録。アセットは変えない（PIE と仮の的だけ）
    - [ ] 11b2 以降. 違いの大きいものから仮の値を直し、取り込み直す（11b1 の結果で分ける）
    見比べる項目（本家の結果 → 本作で確かめること）:
    1. **テレポートの照準**（最新版の病院。材質は両版で同じ）
       - 本家: 赤い渦の輪、赤いデカール、小さな赤い火花が数個。デカールの内側の中央値の R は 139 ↔ 238（約 1.0 s 周期）。
       - 本作で確かめること: `dd_powers.DECAL_COLOR`・`DECAL_PULSE_LOW`・`DECAL_PULSE_HIGH`・`DECAL_CONTRAST`、斬撃の色の出方、火花の大きさと数（ステップ 5 では PIE のほうが大きく多く見えた）。
    2. **倍率 25 のシェイクの間の黒いフレーム**
       - 本家: 見えない。下半分で輝度が 8 未満の画素の割合は、最大でも primal-a 0.099・primal-b 0.075・telekinesis-a 0.045。
       - 本作で確かめること: 同じ測り方で測り、下の決定事項「直さない」を見直す。
    3. **テレキネシス**
       - 本家: 閃光と青の色調の時系列（`observations/README.md`）。力場の間に廊下が照らされる様子は見えない。
       - 本作で確かめること: 同じ時系列になるか、力場の粒子の色・大きさ・速さ（材質 4 つは推定。要確認）、球の粒子の灯が照らす強さ（本作は 0.2〜0.7 s に画面が白っぽい水色になる）。
    4. **Primal**
       - 本家: 黄〜橙の白飛び → 赤の単色。
       - 本作で確かめること: `M_DD_Primal` のパンの速さ（仮）。
    5. **Vanish**
       - 本家: 桃〜白 → 紫のもや → 紫の揺らぐビネット（slomo の下で約 5.1 s 周期）。
       - 本作で確かめること: `M_DD_WobblyVignette` の速さ・周期・強さ（仮）、煙 `M_DD_LoopingSmoke` の見え方（本作は正面からほとんど見えない）。
    6. **Telepathy**
       - 本家: 敵の体に重なる、体と同じくらいの大きさの橙〜赤の炎のような印。
       - 本作で確かめること: `M_DD_Telepathy` の速さと Gain（仮）。本作は仮の的で撮る。
    7. **シャードの回転**
       - 本家: 21.0 s で同じ見た目に戻る。1 周とみなすと 17.1 °/s。
       - 本作で確かめること: 「1 ループ 2 周」の読み（10.8〜32.4 °/s）の範囲に入るので、同じ測り方（`video_probe.py period`）で比べる。
    8. **スピードブーストの揺れ**（推定の `M_DD_ChameleonCameraShake`）
       - 11a では撮っていない。撮るには本家をもう一度起動する。作業一覧の項目 2 でもブーストを実機で見るので、そこで合わせてもよい。
    9. **回収の音の重なり**
       - 要確認（人が聞く）。
- [ ] 12. 仕上げ: 実装記録 04 の見直し、handover の「現状と次の一歩」、作業一覧の項目 1 を「完了」に、main へマージして push、この記録を消す

## 次にやること

ステップ 11b1。`.claude/guides/observation.md` の「5.」「6.」に沿って撮って測る。撮るものの一覧（本家の条件は `observations/README.md` の「パワーの演出」）:

| 項目 | 場所 | 速さ | 長さ | 操作 | 測り方 | 本家 / 本作 |
| --- | --- | --- | --- | --- | --- | --- |
| Primal | 開始地点 | 0.25 | 10 s | 左 = Primal → Q | `series`・`--dark 8`・`sheet` | `orig-primal-a` / `pie-primal-a` |
| テレキネシス | 開始地点 | 0.25 | 10 s | 左 = Telekinesis → Q | `series`・`--dark 8`・`sheet` | `orig-telekinesis-a` / `pie-telekinesis-a` |
| テレキネシス（速さ 1） | 開始地点 | 1 | 6 s | 同上 | `series` | `orig-telekinesis-b` / `pie-telekinesis-b` |
| Vanish | 開始地点 | 0.25 | 16 s | 左 = Vanish → Q | `series`・`sheet` | `orig-vanish-a` / `pie-vanish-a` |
| Telepathy | 開始地点＋仮の的 | 1 | 10 s | 左 = Telepathy → Q | `sheet` | `orig-telepathy-b` / `pie-telepathy-c` |
| 照準 | Zone 1 の待合 | 1 | 5 s | 右 = Teleport → E、ホイールで手前へ、見下ろし | `series --box --stat median`・`HighResShot` | `orig-aim-a` / `pie-aim-b` |
| シャードの回転 | Zone 1 の待合（4 m 先） | 1 | 45 s | 見るだけ | `period` | `orig-shard-spin-long` / `pie-shard-spin-long` |

1. 開始地点（`python Tools/pie.py place -25 3735 --yaw -90`）で上の 5 本を撮る。枠の中身は `get_socket_power` で確かめる。
2. Zone 1 の待合（推定 `place 15 385 --yaw -90`）で照準とシャードを撮る。最初に、本家の `orig-aim-a-full.png` と同じ絵になるかを確かめる。
3. 測った値を `observations/README.md` の ours に本家と並べて書き、違いの大きい順に 11b2 以降のステップを決めてここに書く。

## 決定事項

実装済みのことの決定と理由は、実装記録（00〜04・06 の本文と「既知の制約・注意点」）、`.claude/guides/original-fidelity.md` の版の決定、症状索引に移してある。畳む前の全文は `git show f8a496f:.claude/progress/20260916-tablet-powers.md`。

- 2026-09-16: 大規模改修なので `feature/tablet-powers` で進める。main へのマージはステップ 12。
- 2026-09-16（ユーザーの回答）: 次の 5 点。版の決定は original-fidelity.md、実装は 04 記録にある。
  - 強化段階は Lv5 に固定する。
  - スピードブーストの再使用は、最新版のコードの値にする。
  - パワーが使えないときは、どれも無音にする。
  - 照準中に同じ側の Q で取り消す処理は、旧版から採る。
  - シャードの最小限を先に作る（済み）。
- 2026-09-17（ユーザーの回答）: シャードの見た目はワサミ餅（06 記録）。
- 2026-09-17: 推定したマテリアルは、`/Game/Pipeline/Materials/M_DD_*` のマスターと、原作のパスのインスタンスで作る。決まらない値は `TODO(仮)` にし、見た目は 11b で最新版の病院と見比べて決める。
  - デカールの色と明るさは、旧版の Manor ではポストプロセスが強すぎて戻せない。そのため最新版で決める。
- 2026-09-17（10a）: シェイクの間の黒いフレームは直さないとしていた。理由は、シェイク・タブレットの位置・隠さないことがどれも本家の値と仕組みの写しだから。11a で本家では見えなかったので、11b で本作を同じ測り方で測って見直す。
- 2026-09-17（11）: 観察（エディタを閉じて本家を動かす）と、見比べ・直し（エディタ）は同時にできず、1 コミットに収まらない。そのため 11a と 11b に分けた。
- 2026-09-17（11a）: 本家のセーブは読んだだけで、控えを `SaveBackups/pre-step11-<時刻>/` に写した。捕まって Restart したので、ゲーム自身がセーブを書いたかもしれない（Claude の編集ではない）。
- 2026-09-17（記録を畳んだ）: 新しい決まり（`progress-tracking.md` の「記録を畳む」）に沿って、115 KB から畳んだ。消した決定・手順・検証が、実装記録・症状索引・ガイド・`observations/README.md` にあることを語句で確かめた。無かった 4 点は、症状索引・検証のガイド・観察の手順書に足した（`Desaturation` の入力名、取り込みの後に Zone 1 が未保存になる件、テストをリモート実行で回す方法、PIE の開始地点がランダムなこと）。

## 要確認（ユーザー）

- 2026-09-17: 本家の MOD の W-Editor のファイル — 観察中に押し間違いで W-Editor の画面が開き、`%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に扉 `BP_06_DoubleDoors13` の変換が書かれた（値は原作と同じ位置・回転・拡縮で、`Removed` は偽なので見え方は変わらない）。仮にそのまま残した。理由: 無人ではファイルを消さない・戻さない。場所: 上のファイル（W-Editor の Reset で戻すか、ファイルを消すか）
- 2026-09-17: シャードをまとめて回収したときの音の重なり — 本家の収録に音が無く比べられないので、本作のまま（8 つが 0.5 倍ずつで重なる）にした。理由: 音の確認は人が聞く。場所: 本家の Zone 1 でテレキネシスを使って聞き比べる（`observations/original/orig-telekinesis-pull.mkv` は音なし）
- 2026-09-17: テレキネシスの力場の材質の推定 — 仮に 4 つのグラフを推定し（球 = 暗い青の幕に明るい筋、オーラ = 4 層の曲げたパンと帯の中央の線の窓、地面の輪 = 輪 + いちばん明るい線 + 火花、星屑 = 2 本の細い線を回して重ねた 4 本の光）、決まらない値を仮にした（オーラの 4 層のタイリング・パンの速さ・曲げの速さと強さ、地面の輪の 2 つのパンの速さ）。理由: グラフは cook で消え、パラメータ・サンプル・関数だけが残る（04 記録）。見た目の値なので 11b で最新版の病院と見比べて直す。場所: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_wall02`・`_build_aura7`・`_build_shockwave02`・`_build_star_dust`、`AURA_LAYERS`・`SHOCKWAVE_PANS`（`TODO(仮)`）

## 再開時の注意

- **状態**（2026-09-17 16:15。観察の手順書の作業で PIE を試して止めた後）
  - 本家は閉じてある。
  - エディタは開いている（`L_Hospital_Zone1`、PIE なし、未保存なし）。
  - C++ は 10b3 のままで、ビルドは最新。
- **11b の道具**
  - 撮影と測定の手順は `.claude/guides/observation.md`。PIE は `Tools/pie.py`、測るのは `Tools/video_probe.py`。
  - 本家の収録は `observations/original/`、本作の既存の収録は `observations/ours/pie-*`。
- **取り込み直し**
  - `WasamiDDTools.import_dd_powers()` で行う（`from wasami_tools.toolsets.dd import WasamiDDTools`）。
  - 10b2 の戻り値は、音 7・シェイク 2・カメラアニメ 2・テクスチャ 14・メッシュ 2・マテリアル 28・パーティクル 3（約 37 秒）。
  - 取り込みの後に `L_Hospital_Zone1` が未保存になることがある（症状索引）。
- **テスト**: `Automation RunTests Wasami` で 21 件（検証のガイドの「テスト」）。
- **note**: 原稿は 10b3 で直したが、note へは未反映（セッションの値が無い）。

## 検証

- ステップ 11a:
  - 最新版の pak の大きさが `pak_reference_2` と同じ（7,854,848,189 バイト）。
  - MOD の `clvl` が `Current Level: 06_Hospital_Zone_01` を返した。
  - 収録 12 本（`observations/original/orig-*`）を測った（`observations/README.md`）。
  - 本家を閉じ、エディタを開き直した（未保存なし）。
  - check_records OK（6 件）。
  - C++ とアセットは変えていないので、ビルドとテストは走らせていない。
