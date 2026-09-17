---
title: タブレットから使える特殊効果（パワー）をすべて実装する
status: 進行中
branch: feature/tablet-powers
base: ef11ea6
started: 2026-09-16 19:47
updated: 2026-09-17 19:35
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
    - [x] 11b2. Primal の球の推定の材質を直した … 2026-09-17。04 記録と `observations/README.md` の「Primal の球の見直し」。閃光の長さと球が残る時間の差は測り方（本家の収録の途切れ）と条件（段階・奥の扉）によるもので、材質の差ではなかった
    - [x] 11b3. Vanish の推定の材質を直した … 2026-09-17。04 記録と `observations/README.md` の「Vanish の見直し」。縁のもやはグラフと値を収録から当てはめて合った。煙は見えるようにしたが、位置と明るさは合わない（要確認）
    - [x] 11b4. 照準とテレキネシスの星屑の推定の材質を直した … 2026-09-17。04 記録と `observations/README.md` の「照準とテレキネシスの粒の見直し」。火花の数の差は本家の画質「高」によるもので、材質は変えていない。力場が明るいのは球の粒子の灯の強さの差で、未解決（要確認）
    - [ ] 11b5. **Telepathy の印**: 本家はぎざぎざの穴のあいた赤い煙の雲、本作は丸く柔らかいぼかし（`M_DD_Telepathy`）。大きさは同じ解像度（3440 × 1440 の別窓の PIE など）で比べ直す
    - 撮っていないもの: スピードブーストの揺れ（推定の `M_DD_ChameleonCameraShake`。作業一覧の項目 2 で本家のブーストを見るときに合わせる）、回収の音の重なり（要確認）
- [ ] 12. 仕上げ: 実装記録 04 の見直し、handover の「現状と次の一歩」、作業一覧の項目 1 を「完了」に、main へマージして push、この記録を消す

## 次にやること

ステップ 11b5（Telepathy の印）。

1. 本家の値と仮の値を分ける。本家の値は変えない。
   - 読むもの: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `TELEPATHY_*`・`_build_telepathy`・`make_telepathy_materials`、C++ の `UWasamiTelepathyTrackerWidget`（印の大きさ）。
   - 根拠: `.claude/references/powers/04-primal-telepathy.md`（Telepathy の節）、実装記録 04 の `M_DD_Telepathy`、原作の書き出し `MM_Telepathy`（式が 21 個、`Panner_0`・`Panner_1`、`RadialGradientExponential`）。
2. 本家の収録 `orig-telepathy-b.mkv`（Zone 1、速さ 1、Reaper Nurse に印）と比べる。本家の印は、穴のあいたぎざぎざの赤い煙の雲（画面幅の約 0.11）。本作は丸い柔らかいぼかし。
   - 大きさは同じ解像度で比べる（3440 × 1440 の `HighResShot` には UI が写らないので、印はビューポートの収録で比べる。DPI の拡大率に注意）。
   - 条件: 本家のセーブの Telepathy は Lv2。画質は「高」（`observations/tools/aim_setup.py` の cvar。`main(2)`）。敵の代わりに仮の的（`WasamiTestEnemy.spawn_test_enemy`）。
   - 11b3・11b4 の測り方（`observations/tools/vanish_fit/`、`aim_fit/tonemap.py`）が使える。UI の加算は Emissive × saturate(Opacity)。
3. 材質だけを作り直すときは `dd_powers.make_telepathy_materials()`。PIE 中に試すなら、材質のインスタンスの値は `observations/tools/aim_knobs.py` の形で変えられる（マスターの組み直しは PIE を止めてから）。

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
- 2026-09-17（11b2）: 11b3〜11b5 も、本家のセーブの強化段階（Speed 5・Teleport 3・Telepathy 2・Primal 3・Telekinesis 4・Vanish 3）と、本作に無い配置物の代わりの物で、条件を揃えてから比べる。11b1 の差のうち Primal の 2 つは、この条件の違いと測り方によるものだった。本作の既定は Lv5 のまま変えない。
- 2026-09-17（11b3）: 推定の材質の値は、本家の収録の半透明の重なりを「混ざる色 C と不透明度 a」に分けて当てはめて決める（`observations/tools/vanish_fit/`）。原作のグラフが無いので、候補の式を並べて誤差の小さい、値の少ないものを採る。
- 2026-09-17（11b4）: 本家の収録は画質「高」（本家の `GameUserSettings.ini` の `sg.EffectsQuality=2`）なので、11b5 以降も PIE の中だけ「高」の 3 つの cvar に合わせて比べる（`observations/tools/aim_setup.py`、終わったら `aim_restore.py`）。本作の既定の画質（「最高」）は変えない。
- 2026-09-17（記録を畳んだ）: 新しい決まり（`progress-tracking.md` の「記録を畳む」）に沿って、115 KB から畳んだ。消した決定・手順・検証が、実装記録・症状索引・ガイド・`observations/README.md` にあることを語句で確かめた。無かった 4 点は、症状索引・検証のガイド・観察の手順書に足した（`Desaturation` の入力名、取り込みの後に Zone 1 が未保存になる件、テストをリモート実行で回す方法、PIE の開始地点がランダムなこと）。

## 要確認（ユーザー）

- 2026-09-17: 本家の MOD の W-Editor のファイル — 観察中に押し間違いで W-Editor の画面が開き、`%LOCALAPPDATA%\SimpleModMenu\Saved\Transformation\World\OBJ-06_Hospital_Zone_01.sav` に扉 `BP_06_DoubleDoors13` の変換が書かれた（値は原作と同じ位置・回転・拡縮で、`Removed` は偽なので見え方は変わらない）。仮にそのまま残した。理由: 無人ではファイルを消さない・戻さない。場所: 上のファイル（W-Editor の Reset で戻すか、ファイルを消すか）
- 2026-09-17: シャードをまとめて回収したときの音の重なり — 本家の収録に音が無く比べられないので、本作のまま（8 つが 0.5 倍ずつで重なる）にした。理由: 音の確認は人が聞く。場所: 本家の Zone 1 でテレキネシスを使って聞き比べる（`observations/original/orig-telekinesis-pull.mkv` は音なし）
- 2026-09-17: テレキネシスの力場の材質の推定 — 仮に 4 つのグラフを推定し（球 = 暗い青の幕に明るい筋、オーラ = 4 層の曲げたパンと帯の中央の線の窓、地面の輪 = 輪 + いちばん明るい線 + 火花、星屑 = 小さな四角〈11b4 で本家の収録に合わせて直した〉）、決まらない値を仮にした（オーラの 4 層のタイリング・パンの速さ・曲げの速さと強さ、地面の輪の 2 つのパンの速さ）。理由: グラフは cook で消え、パラメータ・サンプル・関数だけが残る（04 記録）。見た目の値なので 11b で最新版の病院と見比べて直す。場所: `Content/Python/wasami_tools/pipeline/dd_powers.py` の `_build_wall02`・`_build_aura7`・`_build_shockwave02`・`_build_star_dust`、`AURA_LAYERS`・`SHOCKWAVE_PANS`（`TODO(仮)`）
- 2026-09-17: Vanish の煙の位置と明るさ — 本家の収録では煙がエレベーターの扉枠（234 cm 先）より奥に明るい藤色で出るが、原作の値どおりの本作は 92 cm 先・目の 97 cm 下に出て、画面全体を暗い紫に薄く覆う。原因はコードからは見つからなかった。仮に粒子の値は原作のまま、材質の `CameraDepthFade` を 64・0 にして見えるようにした。理由: 本家の値は変えない。薄めの値は収録から上限しか決まらない。場所: `dd_powers.SMOKE_FADE_*`（`TODO(仮)`）、症状索引の「本家と本作で、同じ値の粒子の出る位置が違って見える」。本家の実機で、煙を見下ろす・横を向いて使う収録を撮れば位置が分かる（煙を前へずらすなら、本家の値から離れるのでユーザーの判断が要る）
- 2026-09-17: テレキネシスの球の粒子の灯の強さ — 同じ条件（Lv4・画質「高」）で、力場の間の画面が本家より明るい（発動の約 0.7 秒後の平均が本家 (95〜99, 141〜147, 186〜192)、本作 (123〜127, 174〜176, 216〜218)）。灯を切ると本家よりずっと暗く、線形の明るさで本家の灯の寄与は本作の約 0.5〜0.7 倍。仮に原作の値のまま残した。理由: 灯の値は原作どおりで、推定の材質では説明できない（エンジンの違いを疑うが確かめていない）。場所: `P_ky_forceField_Telekinesis` の `sphere` の `ParticleModuleLight`（`BrightnessOverLife` 5）。本家に合わせて弱めるなら原作の値から離れるので、ユーザーの判断が要る

## 再開時の注意

- **状態**（2026-09-17 19:35。11b4 の後）
  - エディタは開いている（`L_Hospital_Zone1`、PIE なし、未保存なし）。`t.MaxFPS` は 0、画質の cvar は「最高」（1・3・2）に戻した。
  - C++ は 10b3 のままで、ビルドは最新。
  - 材質 `M_DD_DecalTeleport`・`M_DD_KySlash`・`M_DD_KyStarDust` とそのインスタンスは 11b4 の値で作り直して保存した。
- **11b の道具**
  - 撮影と測定の手順は `.claude/guides/observation.md`。PIE は `Tools/pie.py`、測るのは `Tools/video_probe.py`。
  - **撮る前に `python Tools/pie.py cmd "t.MaxFPS 60"`**（無いと 1 秒に 4〜10 枚。症状索引）。ビューポートは (1826, 205)〜(2978, 859)（11b2 も同じ）。
  - 静止画で比べる道具: `observations/tools/primal_*`・`vanish_*`・`aim_*`（git の外）。半透明の重なりを測る道具: `observations/tools/vanish_fit/`。表示値を線形に戻す道具: `observations/tools/aim_fit/tonemap.py`。
  - `desktop.py record` はすぐ戻る。`--seconds` より長く待ってから写す。
  - 本家の収録は `observations/original/`、本作の収録は `observations/ours/pie-*`。
- **取り込み直し**
  - `WasamiDDTools.import_dd_powers()` で行う（`from wasami_tools.toolsets.dd import WasamiDDTools`）。
  - 10b2 の戻り値は、音 7・シェイク 2・カメラアニメ 2・テクスチャ 14・メッシュ 2・マテリアル 28・パーティクル 3（約 37 秒）。
  - 取り込みの後に `L_Hospital_Zone1` が未保存になることがある（症状索引）。
- **テスト**: `Automation RunTests Wasami` で 21 件（検証のガイドの「テスト」）。
- **note**: 原稿は 11b2〜11b4 で直した（写真 03・06 も差し替えた）が、note へは未反映（セッションの値が無い）。

## 検証

- ステップ 11b4:
  - 材質 3 つはエラーなくコンパイルされた（`M_DD_KySlash` の式は原作と同じ 11 個）。インスタンスの `depthFade` は 100 のまま。
  - 本家と同じ条件（照準 Lv3・512.5 cm・画質「高」）の `HighResShot` で、輪が本家と同じ太いサーモン色の帯になった。デカールは暗い側 (143, 85, 86) ↔ 明るい側 (238, 102, 95)、周期 1.00 s（本家 (139, 81, 81) ↔ (238, 97, 90)）。火花の数と大きさは本家と重なった。
  - テレキネシス（Lv4・速さ 0.25・画質「高」）の星屑は本家と同じ小さな四角になった。力場の明るさの差は残る（要確認）。
  - PIE を止め、cvar と `t.MaxFPS` を戻し、未保存なし。ffmpeg と本家は動いていない。
  - check_records OK（6 件）。C++ は変えていないので、ビルドとテストは走らせていない。
  - note の原稿と写真 2 枚を直したが、note へは未反映（セッションの値が無い）。
