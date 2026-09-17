---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-17 17:45
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
  - [ ] 11b. PIE で同じものを撮って見比べ、仮の値（`TODO(仮)`）を直す
    - [x] 11b1. PIE で本家と同じ条件で撮って測った … 2026-09-17。結果は `observations/README.md` の「パワーの演出の見比べ」。シェイク中の黒（見えない）とシャードの回転（範囲内）は直さないと決めた（04 記録）
    - [ ] 11b2. **（作業中）Primal**: 閃光の模様が大きく黄白に寄る（本家は赤橙の地に細かい白い粒）、閃光が 1.5 倍長い、球の赤いまだらが +3.5 s まで残る（本家は +1.0 s で消える）。`M_DD_Primal`（`PRIMAL_PAN_SPEED` などの仮の値と `T_05_PortalMaps` の使い方）と、球の大きさ・消え方が本家の値かを確かめて直す
    - [ ] 11b3. **Vanish**: 画面全体の紫が強い（B +40、本家は +6〜18）、縁のビネットが濃く明るい（左端 R 68〜93、本家 29〜46）、明滅の周期 2.0 s（本家約 5.1 s。UI の材質の時間は slomo に従わない前提）、中央の煙のもやが見えない（本家は約 3.8 s × 0.25 残る）。`WOBBLE_*`・ウィジェットの色・`M_DD_LoopingSmoke` を直す
    - [ ] 11b4. **照準とテレキネシスの粒**: デカールの暗い側 R 74（本家 139）と明るい側の白さ（`DECAL_PULSE_LOW`・`DECAL_PULSE_HIGH`・`DECAL_CONTRAST`）、火花が多く大きい、輪が細く暗い。テレキネシスの星屑が画面を横切るほど大きい（本家は小さな粒）、力場が少し明るい（G +20）
    - [ ] 11b5. **Telepathy の印**: 本家はぎざぎざの穴のあいた赤い煙の雲、本作は丸く柔らかいぼかし（`M_DD_Telepathy`）。大きさは同じ解像度（3440 × 1440 の別窓の PIE など）で比べ直す
    - 撮っていないもの: スピードブーストの揺れ（推定の `M_DD_ChameleonCameraShake`。作業一覧の項目 2 で本家のブーストを見るときに合わせる）、回収の音の重なり（要確認）
- [ ] 12. 仕上げ: 実装記録 04 の見直し、handover の「現状と次の一歩」、作業一覧の項目 1 を「完了」に、main へマージして push、この記録を消す

## 次にやること

ステップ 11b2（Primal）。**作業中**（2026-09-17 17:45〜）。変える予定: `Content/Python/wasami_tools/pipeline/dd_powers.py`（`_build_primal`・`PRIMAL_*`）、`/Game/Pipeline/Materials/M_DD_Primal`・`/Game/DD/Materials/05_Circus/M_05_Primal`、`observations/README.md`、実装記録 04（Primal の見た目の推定）。C++ は変えない見込み。

- 分かったこと（録画の比べ直し）:
  - 閃光と赤の長さは本家のコードどおり（本家の録画は発動の直前に 0.38 s の途切れがあり、「1.5 倍長い」は測り方のずれ。赤が消える時刻から逆算すると本家の発動は 1.79 s 付近）。
  - 本家の録画は Primal が Lv3（半径 2500）、本作は Lv5（3500）。本作には廊下の奥の両開き扉（開始地点の正面 1,036 cm）が無い。この 2 つで「球が +3.5 s まで残る」が説明できる（材質のせいではない）。
  - 本家の球（t≈0.27 s、`orig-2.883`）: 暗い地 (61, 0, 3)・雲 (131, 9, 21)・横長のブロック状の白い欠片 (254, 197, 180)（面積 15 %）・床や壁と交わる所の明るい線 (252, 183, 170)。欠片は R の粒（2〜6 テクセル）を球の UV のまま（タイリング 1）貼った大きさと数に合う。本作の「大きなまだら」は B の雲を明るく不透明に出していたため。
- 直し方: 欠片 = saturate(R × 利得) を明るく、雲 = B で暗い地と中くらいの赤の間、交わる所 = 1 − DepthFade を明るく（録画で見える）。決まらない値はマスターのパラメータにして、PIE の MID で合わせる。
- 比べ方: PIE で強化段階を 3 にし、遠くのエレベーターの扉を (−25, 2699) に動かして奥をふさぎ、Primal をリモート実行で出して、アクタの `CustomTimeDilation` でタイムラインを t≈0.27 s に止め、`HighResShot 3440x1440` を本家の 2.883 s のフレームと比べる。

1. `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_primal`・`PRIMAL_*` と `Source/wasami_deception/WasamiPrimalPower.cpp` を読み、閃光の模様・色・長さと球の消え方のどれが本家の値の写しで、どれが仮の値かを分ける（本家の値は変えない。根拠は `.claude/references/powers/04-primal-telepathy.md`）。
2. 仮の値を `orig-primal-a.mkv` に近づける（模様の細かさ＝タイリング、黄白 → 赤橙の色、パンの速さ）。取り込み直しは `WasamiDDTools.import_dd_powers()`。
3. 11b1 と同じ条件（`observation.md` の「5.」。`t.MaxFPS 60`、開始地点、扉を隠す、slomo 0.25、10 s、左 = Primal）で `pie-primal-b.mkv` を撮り、`series --dark 8`・`sheet` で本家と比べて `observations/README.md` に書く。

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
- 2026-09-17（11）: 観察（エディタを閉じて本家を動かす）と、見比べ・直し（エディタ）は同時にできず、1 コミットに収まらない。そのため 11a と 11b に分けた。
- 2026-09-17（11a）: 本家のセーブは読んだだけで、控えを `SaveBackups/pre-step11-<時刻>/` に写した。捕まって Restart したので、ゲーム自身がセーブを書いたかもしれない（Claude の編集ではない）。
- 2026-09-17（11b1）: 11b を、撮って測る 11b1 と、差の大きい順に直す 11b2〜11b5 に分けた（Primal → Vanish → 照準とテレキネシスの粒 → Telepathy）。どれも取り込み直しと PIE の撮り直しで 1 コミットに収まる大きさ。
- 2026-09-17（11b1）: 本家の Telepathy の収録で敵の体に出る溶岩のような模様は、Reaper Nurse が姿を現す演出（`BP_06_ReaperNurse` の `Cloak`・マテリアルの `Efficiency`）で、Telepathy の印ではない。透明化は作らない（作業一覧の項目 7、ユーザーの回答）ので、Telepathy は印の雲だけを比べる。
- 2026-09-17（記録を畳んだ）: 新しい決まり（`progress-tracking.md` の「記録を畳む」）に沿って、115 KB から畳んだ。消した決定・手順・検証が、実装記録・症状索引・ガイド・`observations/README.md` にあることを語句で確かめた。無かった 4 点は、症状索引・検証のガイド・観察の手順書に足した（`Desaturation` の入力名、取り込みの後に Zone 1 が未保存になる件、テストをリモート実行で回す方法、PIE の開始地点がランダムなこと）。

## 要確認（ユーザー）

- 2026-09-17: 本家の MOD の W-Editor のファイル — 観察中に押し間違いで W-Editor の画面が開き、`%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に扉 `BP_06_DoubleDoors13` の変換が書かれた（値は原作と同じ位置・回転・拡縮で、`Removed` は偽なので見え方は変わらない）。仮にそのまま残した。理由: 無人ではファイルを消さない・戻さない。場所: 上のファイル（W-Editor の Reset で戻すか、ファイルを消すか）
- 2026-09-17: シャードをまとめて回収したときの音の重なり — 本家の収録に音が無く比べられないので、本作のまま（8 つが 0.5 倍ずつで重なる）にした。理由: 音の確認は人が聞く。場所: 本家の Zone 1 でテレキネシスを使って聞き比べる（`observations/original/orig-telekinesis-pull.mkv` は音なし）
- 2026-09-17: テレキネシスの力場の材質の推定 — 仮に 4 つのグラフを推定し（球 = 暗い青の幕に明るい筋、オーラ = 4 層の曲げたパンと帯の中央の線の窓、地面の輪 = 輪 + いちばん明るい線 + 火花、星屑 = 2 本の細い線を回して重ねた 4 本の光）、決まらない値を仮にした（オーラの 4 層のタイリング・パンの速さ・曲げの速さと強さ、地面の輪の 2 つのパンの速さ）。理由: グラフは cook で消え、パラメータ・サンプル・関数だけが残る（04 記録）。見た目の値なので 11b で最新版の病院と見比べて直す。場所: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_wall02`・`_build_aura7`・`_build_shockwave02`・`_build_star_dust`、`AURA_LAYERS`・`SHOCKWAVE_PANS`（`TODO(仮)`）

## 再開時の注意

- **状態**（2026-09-17 18:00。11b1 の撮影の後）
  - 本家は閉じてある。
  - エディタは開いている（`L_Hospital_Zone1`、PIE なし、未保存なし）。`t.MaxFPS` は 0 に戻した。
  - C++ は 10b3 のままで、ビルドは最新。
- **11b の道具**
  - 撮影と測定の手順は `.claude/guides/observation.md`。PIE は `Tools/pie.py`、測るのは `Tools/video_probe.py`。
  - **撮る前に `python Tools/pie.py cmd "t.MaxFPS 60"`**（無いと 1 秒に 4〜10 枚。症状索引）。ビューポートの位置はエディタの窓で変わる（11b1 は (1826, 205)〜(2978, 859)）。
  - 枠の中身とゲージは `tmp/pie_sockets.py`（git の外。`get_socket_power`・`is_power_available`・`get_gauge_percent` を表示する）。無ければ `observation.md` の「5.」の説明で書き直す。
  - 本家の収録は `observations/original/`、本作の既存の収録は `observations/ours/pie-*`。
- **取り込み直し**
  - `WasamiDDTools.import_dd_powers()` で行う（`from wasami_tools.toolsets.dd import WasamiDDTools`）。
  - 10b2 の戻り値は、音 7・シェイク 2・カメラアニメ 2・テクスチャ 14・メッシュ 2・マテリアル 28・パーティクル 3（約 37 秒）。
  - 取り込みの後に `L_Hospital_Zone1` が未保存になることがある（症状索引）。
- **テスト**: `Automation RunTests Wasami` で 21 件（検証のガイドの「テスト」）。
- **note**: 原稿は 10b3 で直したが、note へは未反映（セッションの値が無い）。

## 検証

- ステップ 11b1:
  - PIE の収録 8 本（`observations/ours/pie-*`。`t.MaxFPS 60` で 1 秒に 47〜50 枚、シャードは 10 fps）を、本家と同じコマンドで測った（`observations/README.md`）。
  - 照準の構図は `HighResShot 3440x1440` で本家と同じ（市松の十字の大きさ）と確かめた。
  - PIE を止め、未保存なし。ffmpeg は残っていない。
  - check_records OK（6 件）。C++ とアセットは変えていないので、ビルドとテストは走らせていない。
