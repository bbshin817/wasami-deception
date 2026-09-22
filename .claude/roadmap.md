# 作業一覧（最終目標までの段階）

2026-09-17 にユーザーが示した最終目標を、Claude が段階（項目）に分解したもの。**1 項目 = 進捗記録 1 件**（`.claude/progress/`）。2026-09-18 に、項目を**3 つの大目標**（最小の通しプレイ → ゲームとして一通り → 本家に忠実に）の節に分けた（下の「大目標」）。運用は `.claude/guides/autonomy.md` の「何を作業するか」: 進捗記録が無いときは、**進行中の大目標の節**の「未着手」で依存が満たされた最初の項目を取り、最初の反復は計画（進捗記録を作りステップに分けてコミット）だけで終える。項目が終わったら状態を「完了（日付）」にし、次の項目は次の反復で始める。大規模な項目は進捗記録の中でステップに分ける（`.claude/guides/git-workflow.md` の大規模改修 = 作業ブランチ）。

各項目は「目標 / 完了の条件（何を測って何に合えば終わりか）/ 根拠の置き場 / 依存 / 規模（下の「進捗率」）/ 状態」。根拠は原則コード（`pak_reference_2/`。テレポーテーションは `pak_reference/`。`.claude/guides/original-fidelity.md`）。数は本家のレベルの書き出し（`_levels/06_Hospital*.scene.json`）から数えた。

番号は項目を指すための名前で、**作業の順は大目標の節の中の並び順**（「最初の項目」は進行中の大目標の節の上から数える）。22・23 は 2026-09-17 の要確認の回答から足した項目。15・16（入口）は 2026-09-18 に取りやめた（経緯として残す）。24〜28 は 2026-09-18 に大目標に分けたとき、項目 6・7・9 から後の大目標へ回す部分を分けたものと、大目標ごとの確かめ・後回しの受け皿として足した。29 は 2026-09-20 の要確認の回答（EXTRAS を作る）から足した。30・31 は 2026-09-20 の有人セッションの指摘（ステージ OP が無い・祭壇の見た目が本家と違う）から足した。37〜46 は 2026-09-22 の有人セッションの指摘（ゲームレビュアーが通しで遊んだ 11 件）から足した。

## 最終目標（2026-09-17、ユーザーの原文）

```
- 敵は `tmp\enemy_wasami.glb` 。多数のモーションを含んでいるため、適切な用途に使う。
    - スタンオーブ使用時: “気絶”モーションを使用する。
    - プレイヤー捕獲時: 本家Monkey Business捕獲時の、黒背景+モンスター+カメラアニメーションの体で、敵ワサミのモーションを再生させる。
        - カメラは本家Monkey Business捕獲時のような、躍動的・非機械的な臨場感あるアニメーションを設ける。
        - 死亡時アニメーションは3種類を設け、ランダムにシャッフルする。
- ワサミシャードは、紫色のやや弱めな閃光を放つようにする。
- スタンオーブ、敵の位置がマップ上で表示される特別シャードは、本家と同一の見た目にする。
- ボス戦は設けない。Zone2の任意の箇所に、脱出ポータルを設置し、そこをくぐると脱出完了とする。

# WebGL版と同じように倣う点

- タイトル画面
- オプション画面
- Escaped後のスコア表示画面

# 既存の修正点

- マウスによる視点移動が遅すぎます
- スピードブースト使用時、集中線がおかしなノイズのように現れます

# 最終の理想点

- 本家と同じようにゲームが開始され、敵キャラは敵ワサミに、通常の紫シャードはワサミシャードに置き換えられ、ボス戦を設けずクリア出来るようにする。
- 除細動器、スピードブーストによって突破できるバリア、シークレット要素等は本家と同じように設置する。
```

## 最終目標について決めたこと（2026-09-17、ユーザーの回答）

- **敵のモーションの割り当て**（`enemy_wasami.glb` の 11 本）: 巡回 = `Walking`、追跡 = `Running`、全回収後（Nightmare）の追跡 = `run_fast_2`、気絶 = 無名のモーション `01a0a88f-…`（10.0 s。前屈の区間をループし、明けに起き上がりを再生する）→ **2026-09-18 のユーザーの指示で、気絶は v3 の `BeHit_FlyUp`・`Knock_Down` のどちらかをランダムに流して倒れ、明けに寝返り（Claude が作る。0.8 s）+ `push_up_to_idle` で起き上がる**（Primal Fear でもスタンオーブでも同じ。無名のモーションは使わなくなった）、捕獲 3 種 = `Backflip`・`sliding_rool`・`Stylish_Walk`、`restpose` は基準姿勢。旧 glb の `Shot_and_Fall_Forward`・`Stand_Up6` は使わない。気絶以外の割り当ては Claude の提案で、2026-09-17 にユーザーが承認した。**2026-09-18: モデルは `tmp/enemy_wasami_v3.glb`（通常の敵）・`tmp/boss_wasami.glb`（Matron）・`tmp/wasami_mochi_v3.glb`（シャード）を使う**（ユーザーの指示）。v3 にも上の巡回・追跡・Nightmare・気絶のアニメがあり、捕獲の 3 本は v3 に無いので旧 glb の 3 本を流用する（同日の回答）。役とアニメの対応・中身は `.claude/references/enemy-wasami-motions.md`。
- **追跡中のランダムの動き**（2026-09-18 のユーザーの指示）: `enemy_wasami_v3` の追いかける動き（片手をついて飛び越える・突進・スライディングなど 6 本）を、Zone 1・2 の追跡中にたまにランダムで流す。頻度は約 8 秒に 1 回、流す間も本家の追跡の速さ（800 cm/s）を保つ（同日の回答）。前方が空いているときだけ流す。詳細は同じ一覧。
- **場面の演技**（2026-09-18 の回答）: Zone 1 の途中の出来事と Zone 2 の始まり（捕まる → 独房）は残し、本家の専用の演技は v3 の動きで代用する（対応は同じ一覧）。
- **敵の赤い縁取り**: 作らない（2026-09-17 のユーザーの回答「不要」。最新版ではナースの `Custom Depth(Duration)` を呼ぶ者がいない）。
- **捕獲のカメラ**: 本家ホテル（旧版 `01_Hotel`）の捕獲のシーケンスのカメラを写し、ワサミの 3 モーションの長さに合わせる（「本家ホテルの 3 本を写す」）。2026-09-19 の有人セッションの指摘（「カメラがワサミから少し遠い」「Monkey Businessのような躍動感がカメラにない」「ワサミの動きがやや遅い（最初は高速で、イージングで徐々に等速に戻る、などが欲しい）」「Deadly Decadenceのような、顔を間近に映すジャンプスケアが1つは欲しい。ただし背景は黒で統一」）で、寄りを近くし、場面を速く始めて等速へ戻し、館のゴールドウォッチャーの捕獲の体で顔を間近に写す 4 本目を足した（項目 24）。
- **開始**（2026-09-18 に方針変更）: 入口レベル `06_Hospital` とその導入は**作らない**（ユーザー「Zone1は最初のシーンをまるまる飛ばし、本家のエレベータ上昇・到着から開始してください」）。ゲームは Zone 1 の本家の始まり（エレベーターの上昇と到着。`06_Hospital_Zone01_ElevatorArrive`、14.1 s）から始まる。ステージは `06_Hospital_Zone_01` → `06_Hospital_Zone_02`。（2026-09-17 の回答では入口も作る予定だった）
- **敵の AI**: 追跡型だけ。透明化（cloak）・薬投げ・ガスは作らない。速さ・視界・巡回・見張り・Nightmare の値は `BP_06_ReaperNurse*` と行動ツリーのコードから取る。
- **Zone 2 の Matron（中ボス）**: 大きい敵ワサミとして残す。3D モデルは `tmp/boss_wasami.glb`（2026-09-18 にユーザーが指定。アニメ 7 本）。
- **脱出**: ボス戦（`06_Hospital_Bossfight`）は作らない。Zone 2 のガレージ（本家がボス戦へ移る `Postmaze_Trigger_Garage` の場所）に**祭壇を本家と同じ見た目で置き、Deadly Decadence（旧版 `03_Manor_Zone2`）の流れ**（全回収 → 祭壇の球が消える → 祭壇を Use して欠片 → ポータル → くぐると脱出）に倣う。
- **音**: 本家の曲・環境音・効果音・Bierce の台詞をそのまま使い、**WebGL 版のワサミの声と字幕も付ける**。
- **シャードの光**: 両方。置かれている間は本家の紫の灯に加えて餅が紫に明滅し、回収時の閃光（`P_ky_flash3`）は紫でやや弱く。2026-09-17 の要確認の回答で、**餅の明滅はやめ、代わりに餅を今の 1.5 倍の大きさにして本家のシャードのように回す**（項目 22）。閃光は今の値（紫 × 各色の最大 ^ 0.5 × 0.8）で確定。餅のモデルは `tmp/wasami_mochi_v3.glb` に替える（2026-09-18。項目 22）。
- **2026-09-17 の要確認の回答（ほか）**: Vanish の煙は今の形（粒子は原作の値、材質のフェードだけ仮の `SMOKE_FADE_*`）でよい。シャードをまとめて回収したときの音の重なりはこのままでよい。テレキネシスの球の灯は本家の見え方に合わせて弱め、テレキネシスの力場の材質と Telepathy の印は本家の観察を続けて詰める（項目 23）。駆動役の使用量の読み方は当面決めない（今の `--usage-cmd` / `--no-usage-check` のまま）、上限の返事の文言も指定なし（今の仮のまま）。
- 既に決まっていたこと（そのまま）: パワー 6 種は Lv5 固定で最初から使える（祭壇での購入は無い。祭壇は脱出にだけ使う）、ライフ 3、制限時間なし、シャードはワサミ餅、原作のロゴとキャラクターのモデルは使わない。

## 大目標（2026-09-18、ユーザーの指示）

ユーザーの指示（原文）: 「最小の通しプレイ->ゲームとして1通り->本家に忠実に」「大きな3つの目標で、無人運転はこの目標を超えない(1つの目標を達成したら一旦止まる)方針にしてください」。きっかけは「見た目の忠実な詰めを一旦やめ、ゲームとして遊べることを目標とする場合、どれくらいでできそう？」への見積もり: 見た目を本家と見比べて詰める項目 23 は規模 2 で 11 時間を超えたが、見た目を詰めない項目（2・3・4・22）は規模 1 あたり約 1 時間だった。

| 大目標 | 達成の姿 | 見積もり（2026-09-18） |
| --- | --- | --- |
| 1. 最小の通しプレイ | Zone 1 のエレベーターの到着から、敵に追われながら全部回収 → 救急車で Zone 2（2026-09-19 に「ガレージリフトで」から読み替えた。項目 6）→ 全部回収 → 祭壇 → ポータルで脱出まで、PIE で通しで遊べる。捕まると死亡画面からチェックポイントで再開する | 規模 16、9/20〜21 ごろ |
| 2. ゲームとして一通り | タイトル → Zone 1 → Zone 2 → 脱出 → スコア画面まで、最終目標の要素（扉と罠・スピードバリア・スタンオーブと赤いシャード・Matron・秘密・捕獲のカメラ・場面・曲・台詞・オプションとポーズ）がそろって遊べる | 規模 26、9/22〜24 ごろ |
| 3. 本家に忠実に | 大目標 1・2 で後に回した見た目と演出を、原作のコード・アセットで詰める（コードで決まらなかったものだけ本家の実機を撮る。2026-09-21） | 規模 13（23=2 完了・28=4・32=1・33=2・34=2・35=1・36=1。2026-09-21 の有人セッションで配り直した） |
| 4. レビュー指摘の修正 | 2026-09-22 のゲームレビュアーの指摘 11 件（High 1・Medium 10）が直り、パッケージ版で確かめてある | 規模 16（37=2・38=1・39=1・40=1・41=2・42=3・43=1・44=2・45=1・46=2。2026-09-22 に Claude が見積もった） |

- **大目標の状態**は各節の頭の `- 状態:`（未着手 / 進行中 / 達成（日付））。**進行中は 1 つだけ**。
- **未着手 → 進行中に変えるのはユーザーだけ**（有人セッションで指示を受けて Claude が書き換える）。無人運転は大目標の状態を「進行中」にしない。**例外は節の頭の `- 始め方:` が「自動」の大目標**（無ければ「ユーザーの指示」）: 前の大目標を達成したら、無人運転が「進行中」にして続ける。いまは大目標 2（2026-09-19 のユーザーの指示「本目的2への以降は有人セッションなしで、無人運転が続行できるものとします。」）と大目標 3（2026-09-20 のユーザーの指示「大目標2が完了後、自動で3に移るように」。それまでは項目 28 の後回しの一覧をユーザーと見直してから始めることにしていたが、見直しは項目 28 の計画の反復で Claude が行う。大目標 3 の節の頭）。大目標 3 までは 2026-09-21 に達成した。**2026-09-22 に足した大目標 4（レビュー指摘の修正）の始め方は「ユーザーの指示」**なので、無人運転は自分ではそこへ移らない。
- **無人運転は、進行中の大目標の節の項目だけを取る**。ほかの節の項目の進捗記録は `status: 保留` にしてあり、飛ばす。
- **進行中の大目標の項目がすべて「完了」になったら**、その項目を閉じた反復で大目標を「達成（日付）」にし、状態ファイルに `stop`（理由「大目標 N（名前）を達成」）を書いて終える。次の大目標の計画は立てない。駆動役（`Tools/overnight.py`）も、反復の後に進行中の大目標の項目がすべて完了なら止まり、起動のときに進行中の大目標が無ければ起動を断る（Claude が書き忘れても超えないように二重にしてある）。**次の大目標の始め方が「自動」なら**、同じ反復で次の大目標を「進行中（日付から）」にして状態ファイルは `continue` にし、駆動役も止まらずに続ける（Discord に「🎯 大目標 N を達成」を送る）。
- 有人セッションでは達成を報告し、ユーザーが遊んで確かめてから、指示を受けて次の大目標を「進行中」にする。始め方が「自動」の大目標へは止まらずに移るので、前の大目標は後から遊んで確かめる（handover の「現状と次の一歩」に手順を書いておく）。

### 大目標 1・2 の決め方（見た目の詰めをしない）

- **ゲームの規則・流れ・配置は、今までどおり本家のコードから写す**（速さ・秒数・個数・条件・順序・出現点。読むだけで済むので省かない。`.claude/guides/original-fidelity.md`）。
- **見た目・演出は、原作のアセット（メッシュ・材質・テクスチャ・粒子・音・シーケンス）をそのまま使う**。書き出しに無いもの・材質の式が消えたものは手早い推定（仮の値、`TODO(仮)`）で済ませ、項目 28 の「後回しの一覧」に 1 行書く。
- **しないこと**: 本家の実機を起動しての観察と収録、本家と本作の収録を並べて測る見比べ、見た目を詰めるためだけのステップ。本家の実機を使うのは、ゲームの規則がコードで決まらないときだけ。
- **確かめは、PIE で動くこと・遊べること**（本作の側の収録。Discord の連番のグリッドは今までどおり）。
- 大目標 3 では、今までの決め方（本家の観察・収録の見比べ・原作の式の写し）に戻る。

## 進捗率（Discord の反復の報告）

2026-09-18 のユーザーの指示「進捗率は、全体の発足〜最終目標を母数として概算を出します」から。駆動役 `Tools/overnight.py`（`overall_progress`）が、反復ごとにここから計算して Discord の報告に出す。

- **進捗率 = 済んだ規模 ÷ 全体の規模**。
  - 全体 = 作業一覧の前に済んだ土台 + 取りやめ以外の各項目の「規模」。
  - 済んだ = 土台 + 状態が「完了」の項目 + 進行中の項目の規模 × その進捗記録の「計画」のうちチェック済みのステップ（字下げの無い `- [x]`）の割合。記録の `# ` の題に「項目 N」があれば、項目 N の記録とみなす。
- **大目標の進み**（2026-09-18 から、報告の進捗率の括弧の中）: 同じ計算を、進行中の大目標の節の項目だけで行う（土台は入れない）。
- **規模**は作業量の目安の相対値（1 = 小さな直し・見た目の詰め、3 = 仕組み 1 つ、5 = ゾーン全体に効く大きな仕組み）。2026-09-18 に Claude が各項目の中身から見積もった。**項目を足すときは規模も書く**（規模の無い項目は数えない）。
- 作業一覧の前に済んだ土台の規模: 8
  - 2026-09-16〜17 の、プロジェクトの雛形、ステージの前処理と Zone 1・2 の組み立てと焼き込み、プレイヤー、タブレット、パワー 5 種、シャード。git の 48 コミット分で、作業一覧の完了した 5 項目（規模 9）と見比べて決めた。

## 済んだ項目（大目標を決める前。2026-09-17〜18）

### 1. タブレットのパワーの残り（テレキネシス・実機との見比べ・仕上げ）

- 目標: 6 種のパワーを完成させる。残りはテレキネシス（仕組みと粒子）、推定した材質・粒子の実機との見比べ、実装記録と handover の仕上げ、main へのマージ。
- 完了の条件: 進捗記録 `20260916-tablet-powers.md` のステップ 10a〜12 が完了し、記録が消えて `feature/tablet-powers` が main にマージされている。
- 根拠: `.claude/references/powers/`、`pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/Powers/BP_TelekinesisPower.txt`。
- 依存: なし。
- 規模: 3
- 状態: **完了（2026-09-17）**。結果は実装記録 04・06、残った差と仮の値は 04 記録の「既知の制約・注意点」。2026-09-17 の回答で、テレキネシスの灯と材質、Telepathy の印は項目 23 で詰める。

### 2. 既存の修正 2 件（マウスの視点移動が遅い・スピードブーストの集中線がノイズに見える）

- 目標: 視点移動の速さを本家と同じにし、集中線を本家と同じ絵にする。
- 完了の条件: (1) 同じマウスの移動量に対する回転角が最新版の実機と一致する（`Tools/desktop.py` の `look` で同じ量を送って収録し、画面の回転を測る）。(2) ブースト中の画面（`observations/ours/`）で集中線が本家の実機の収録と同じ形（放射状の線のコマ送り）に見える。
- 根拠: 実装記録 02・04、`pak_reference_2/_raw/DDeception/Config/DefaultInput.ini`、`pak_reference_2/_bytecode/DDeception/Content/UI/Main/Powers/UMG_SpeedBoost.txt`、最新版の実機。
- 依存: 1（同じ C++ とマスターを触るので、マージの後）。
- 規模: 1
- 状態: **完了（2026-09-17）**。視点は Enhanced Input が `AxisConfig` の感度 0.07 を自動で重ねていたための 0.07² の二重掛けで、C++ の Scalar を外して実機と同じ 0.175°/カウントにした（02 記録・症状索引）。集中線は `M_Speedlines` の FlipBook を実機に合わせて 2 × 5・30 コマ/s にした（04・01 記録。測り方と値は `observations/README.md` の「視点の速さと集中線」）。2026-09-18 に、ユーザーの指摘（走査線が見える）で `M_Speedlines` の Opacity にテクスチャの A をつないだ（未接続で、A が 0 の画面いっぱいの横線が出ていた。原作のコンパイル済みシェーダーのとおり。04 記録）。

### 3. ワサミシャードの光（紫の明滅と、紫でやや弱い回収の閃光）

- 目標: 置かれている餅が紫に明滅し、回収の閃光が紫でやや弱くなる。
- 完了の条件: PIE の収録で、餅の発光が周期的に強弱し（周期・強さは仮でよい。要確認に書く）、回収の閃光の色相が紫で、画面の最大輝度が今の `P_ky_flash3` より低い。本家の紫の灯（175、半径 200、(194, 0, 255)）は変えない。
- 根拠: 実装記録 06（`AWasamiShard`、`M_DD_WasamiMochi`、`P_ky_flash3` の推定の材質）。本家に無い本作独自の見た目なので、色と強さは仮の値（`.claude/guides/autonomy.md` の線引き）。
- 依存: 1。
- 規模: 1
- 状態: **完了（2026-09-17）**。餅は `M_DD_WasamiMochi` の自己発光に紫の波（強さ 1.0・周期 2 s、位相は個体ごとの乱数）を足して明滅し、回収の閃光は本作の版 `/Game/Wasami/Shard/P_WasamiShardFlash`（`P_ky_flash3` の色を紫 × 各色の最大 ^ 0.5 × 0.8）にした。本家の紫の灯は変えていない。PIE の 4.4 m 先で、閃光の中心の最大輝度 247 → 170・色相 299°（実装記録 06、測り方と値は `observations/README.md` の「シャードの光の基準」「餅の紫の明滅」「紫の回収の閃光」）。2026-09-17 の回答で、閃光の値は確定、餅の明滅は項目 22 で外す。

### 4. 敵ワサミの素体（モデル・アニメ・敵の受け口）

- 目標: `enemy_wasami_v3.glb`（と、捕獲の 3 本だけ旧 `enemy_wasami.glb`）をスケルタルメッシュとアニメとして取り込み、敵のアクタ `AWasamiEnemy` の土台（モーションの再生、パワーからの受け口）を作る。AI はまだ入れない。赤い縁取りは作らない（上の「決めたこと」）。
- 完了の条件: 原本を `SourceArt/Wasami/`（Git LFS）に写し、取り込み（`wasami_tools` のツールセット）が `/Game/Wasami/Enemy` にメッシュ・スケルトン・v3 のアニメ 16 本と捕獲の 3 本を作り、一覧（`.claude/references/enemy-wasami-motions.md`）の役で引ける。`AWasamiEnemy` が `IWasamiEnemyInterface`（`SetState(Stun)`・`PlayerVanish`・`NoTelepathy`）を実装し、気絶で倒れて、明けに起き上がる（2026-09-18 に `Stun_Loop`〈ループ〉・`Stun_Recover` から、倒れる 2 本〈ランダム〉+ 寝返りからの起き上がり 2 本に替えた。上の「決めたこと」）。Primal Fear・Telepathy・Vanish の仮の的 `AWasamiTestEnemy` の代わりに PIE で使える。
- 根拠: `tmp/enemy_wasami_v3.glb`（Blender 4.5 の glTF、骨 28、UE のマネキン系の名前）、`pak_reference_2/_bytecode/DDeception/Content/Blueprints/Characters/Nurse/BP_06_ReaperNurse.txt`（部品と気絶）、実装記録 04（インターフェース）。
- 依存: 1。
- 規模: 3
- 状態: **完了（2026-09-18）**。`WasamiDDTools.import_wasami_enemy` が `/Game/Wasami/Enemy` に `SK_WasamiEnemy` と役の名前のアニメ 18 本（`A_WasamiEnemy_<役>`。2026-09-18 に気絶を替えて 19 → 18 本）を作り、`AWasamiEnemy`（本家のナースの値、0.5 s ごとの判断で気絶して 17 s 後に Patrol）と `UWasamiEnemyAnimInstance`（本家のナースの ABP の木、全身の 1 回再生 `PlayOnce`）が動く（実装記録 07）。PIE で立ち姿・巡回・追跡・Nightmare・気絶と明け・Telepathy・Vanish・1 回再生 9 本を確かめた（`observations/README.md` の「敵ワサミ」）。`Chase_VaultLand` は取り込みで床から跳ぶ形に直した。敵はまだレベルに置かれず（`SpawnEnemy` を呼ぶのは項目 6・7）、仮の的 `AWasamiTestEnemy` とそのテストは残してある。再生の速さ・Nightmare の切り替え・待機の選択・`Chase_VaultLand` の形は下の「未回答の要確認」。

### 22. ワサミシャードの見た目の変更（明滅をやめる・1.5 倍・本家のシャードのように回す）

- 目標: 2026-09-17 のユーザーの回答「餅自体の明滅は撤廃し、代わりにサイズを現在の1.5倍にし、本家シャードのように回転させて」と、2026-09-18 の指示（餅のモデルを `wasami_mochi_v3` にする）。
- 完了の条件: (1) `M_DD_WasamiMochi` の紫の明滅（`Pulse*` と、`AWasamiShard` が入れる位相の乱数）を外し、自己発光を WebGL 版の `Glow` 0.3 だけに戻す。本家の紫の灯と回収の閃光 `P_WasamiShardFlash`（色は `dd_shards` の紫の定数から作る）は変えない。(2) 餅のモデルを `tmp/wasami_mochi_v3.glb`（10 万三角形、テクスチャ 2048²・2048²・4096²。一覧の「ワサミ餅」）に替え、`SourceArt/Wasami/` の原本を置き換える。679 個を置くので Nanite とテクスチャの大きさの上限を決める。(3) 餅の大きさを今の 1.5 倍にする（`Mochi` の拡縮 0.055 → 0.0825。10 倍の下で 0.55 m → 0.825 m）。当たりのカプセル（原作の値）と灯は変えない。(4) 回り方を本家の結晶と同じに見えるようにする。今も本家の結晶のアニメ `soul_shard_skeletal_anim_loop` と同じ Z 軸まわりの速さ（個体ごとに 10.8〜32.4 °/s）でヨーが回っているが、顔が真上を向いた丸い餅なので回って見えにくい。計画で、最新版の実機の結晶と PIE の餅を同じ撮り方で並べ、回る軸・餅の向き・速さの見え方の違いを確かめてから、合わせ方を決める（餅の向きを変えるなら理由を決定事項に書く）。PIE の収録で、明滅が無いこと、大きさが 1.5 倍であること、本家のように回って見えることを確かめる。
- 根拠: 実装記録 06（`AWasamiShard`、`M_DD_WasamiMochi`、「餅の回転」）、`observations/README.md`（本家の結晶は 21.0 秒で 1 周）、`pak_reference_2/_assets/**/soul_shard_skeletal_anim_loop*`。
- 依存: 3。
- 規模: 1
- 状態: **完了（2026-09-18）**。(1) `M_DD_WasamiMochi` の `Pulse*` と `AWasamiShard` の位相の乱数を消し、自己発光を `Glow` 0.3 だけに戻した。(2) 餅を `wasami_mochi_v3`（101,368 三角形、PNG の 2048²・2048²・4096²、Nanite）に替え、Zone 1 の待合（餅 337 個）でフレーム時間が変わらず VRAM が +0.20 GB だったので**テクスチャの上限は設けない**と決めた。(3) `MochiSize` 0.55 → 0.825 m（画面の横幅 1.52 倍）。(4) 回り方は本家の既存の収録と同じ撮り方で 45 秒撮って見比べ（本作 13.3 s / 1 周 = 27.1 °/s、本家の実機の 1 個は 21.0 s = 17.1 °/s）、**速さは個体ごとの乱数（10.8〜32.4 °/s）のまま直さない**と決めた（本家の `BP_Shard` が同じ乱数を引くので乱数そのものが原作の値。軸と向きもコードのとおり）。実装記録 06 の「餅のモデル」「餅の回り方」、測り方と値は `observations/README.md`。要確認は残っていない。

## 大目標 1: 最小の通しプレイ

- 状態: **達成（2026-09-19）**（2026-09-18 から。遊んで確かめる手順は `.claude/references/handover.md` の「現状と次の一歩」）
- 達成の姿: Zone 1 のエレベーターの到着から、敵に追われながら全部回収 → 救急車で Zone 2（ガレージリフトで上がり、テレポーテーションで屋根へ。項目 6 の読み替え）→ 全部回収 → 祭壇 → ポータルで脱出まで、PIE で通しで遊べる。捕まると死亡画面からチェックポイントで再開する。タイトル・スコア画面・扉と罠・特殊シャード・Matron・場面・曲・台詞は大目標 2。
- 決め方: 上の「大目標 1・2 の決め方」（見た目の詰めをしない）。
- 達成の条件: この節の項目がすべて完了（最後の項目 27 が通しプレイを確かめる）。

### 5. ゲームの流れの土台（死亡・ライフ・チェックポイントとセーブ・死亡画面）

- 目標: 死ぬ → 死亡画面 → チェックポイントから再開、ライフとゲームオーバー、進行のセーブ（`SAVING PROGRESS`）を本家どおりに作る。敵の接触（項目 9）より前に作るので、死亡はデバッグの呼び出しで起こして確かめる。QUIT TO TITLE は、タイトル画面（項目 17、大目標 2）ができるまでは Zone 1 の最初からやり直す形でよい。
- 完了の条件: ゲームモードの `DeathEvent` で入力が止まりタブレットが下り、`UMG_DeathScreen`（WebGL 版 10 記録の死亡画面の時間・アニメ・音）が出て、ライフが残っていれば最後のチェックポイントで再開し（シャードの回収状態はセーブどおり）、0 なら RESTART / LAST CHECKPOINT / QUIT TO TITLE。チェックポイントの通過で右下に `Progress Saved`。セーブは `SaveGame`（本家の `BP_DD_levelStructSave` の項目）。パワーの死亡のリセット（実装記録 04）がここから呼ばれる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_DD_GameMode.txt`、`06_Hospital_Zone_01.txt`（`Respawn`、`Struct Save`、`Progress Saved`）、`pak_reference/_bytecode/DDeception/Content/Blueprints/UMG/UMG_DeathScreen.txt`、WebGL 版 04・10 記録。
- 依存: 1。
- 規模: 3
- 状態: **完了（2026-09-18）**。ゲームインスタンス（ライフ 3・回収済みのシャードの記憶）とセーブ（`structSlot` の病院の欄と `Last Checkpoint Warning`）、ゲームモードの `DeathEvent`・`SaveCheckpoint`（右下の SAVING PROGRESS）・チェックポイントの PlayerStart から出す開始、死亡画面 `UWasamiDeathScreenWidget`（ライフ −1・アニメ 4 本・パワーのリセット・今のレベルの開き直し）、ゲームオーバーの RESTART / LAST CHECKPOINT / QUIT TO TITLE と YES / NO の問い（実装記録 06・09）。完了の条件の読み替え（進めてみて原作に合わせた）: 死亡でタブレットを下ろす・入力を止める処理は原作に無く、ゲームを止めて画面が覆うことで満たす。右下は原作の画面の `SAVING PROGRESS`（`Progress Saved` は開発用の PrintString）。回収済みのシャードはディスクではなくゲームインスタンスが覚え、死亡と LAST CHECKPOINT の開き直しで戻らない。死亡画面の声は項目 20、チェックポイントを通る場面と区間の準備は項目 6・13（ゲームモードの `GetStartCheckpoint()` を見る）。確かめるときは PIE のコンソールで `Wasami.Kill`・`Wasami.Checkpoint N`・`Wasami.Lives N`・`Wasami.ResetSave`。

### 6. ゾーンの進行（開始の流れ・障壁とシャードチェッカー・全回収・救急車で Zone 2 へ）

- 目標: Zone 1 の開始から救急車で Zone 2 へ移るまでと、Zone 2 の開始を本家のレベル BP どおりに作る。ゲームの最初はここ（入口は作らない。2026-09-18）。
- 完了の条件: Zone 1 の開始（エレベーターの上昇と到着 `06_Hospital_Zone01_ElevatorArrive`〈扉 2 枚の動き、14.1 s〉、`Spawn`、目的の帯 `COLLECT ALL SHARDS`、タブレットの矢印 `BP_ArrowPointer`）→ シャード 337 の全回収で `All Shards Collected`（ゾーンの障壁 `BP_ZoneBarrier` とシャードチェッカー `BP_ZoneShardChecker`）→ 救急車の屋根 `TriggerBox_06_AmbulanceTop`（本家の `06_ReachAmbulance`。駐車場のガレージリフト `BP_06_GarageLift_Zone1_Special` で上がり、テレポーテーションで屋根へ）で Zone 2 へ移り（レベルの切り替えとセーブ）、Zone 2 の開始（`BP_06_GarageLift` ×2、`BP_06_Lift_03` ×8・`BP_06_Lift_04` ×2・`BP_06_LiftBase_Corner` ×5、地図 `BP_MapTexture_MultiFloor`・`BP_MapArea` ×2）まで PIE で通しで遊べる。Zone 2 は、本家の到着の後の捕まる場面と独房の場面（項目 25、大目標 2）を飛ばし、レベル BP で独房の場面の後にプレイヤーが動けるようになる位置と状態から始める。数と値はレベル BP とアクタのプロパティどおり。Bierce の台詞は項目 20、Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event` は項目 25（どちらも大目標 2）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`、`_sequences/06_Hospital_Zone01_ElevatorArrive.json`、`Blueprints/06_Hospital/Lifts/**`、`Blueprints/Main/BP_ZoneBarrier.txt`、`_levels/06_Hospital_Zone_0*.full.json`。
- 依存: 4、5。
- 規模: 4
- 状態: **完了（2026-09-19）**。ゲームモードが開始時に出す区間の流れ `AWasamiZone1Flow`・`AWasamiZone2Flow`（本家のレベル BP の代わり。トリガーの箱・チェックポイントの保存・目的の文・矢印の値）で、Zone 1 はエレベーターの到着のシーケンス（扉が約 11 s に開く）→ 扉の破壊（F の連打）と両開き扉 → 迷路で COLLECT ALL SHARDS → 全回収で障壁が壊れて駐車場へのフェード → トンネルの扉が閉ざされ 25 s 後に破られる → 救急車の屋根で保存 7 → 救急車が走り出し読み込み画面 → Zone 2、Zone 2 は独房（棘が下りる中で扉の鍵を外す）→ ミニボスの廊下（GET PAST THE NURSES）→ 迷路（COLLECT ALL SHARDS、乗ると上がる床 15 台、階ごとの地図）→ 全回収で COLLECT THE RING PIECE まで、PIE で通しで遊べる（実装記録 11・12・08・09・03・01）。**完了の条件の読み替え**（本家のコードに合わせた）: 「ガレージリフトで Zone 2 へ」は作業一覧を作ったときの読み違いで、本家で Zone 2 を開くのは救急車の屋根（`06_ReachAmbulance`）。ガレージリフトは駐車場の車のリフトで、上がった台から屋根へはテレポーテーションで移る（2026-09-19 に PIE で通した）。ガレージリフトの `NurseNear`・`Spawn Nurses*`・ミニボスは項目 7・11、場面は項目 25、Bierce の台詞とナースの放送は項目 20、曲は項目 19。読み替えは 2026-09-19 にユーザーが承認した。読み込み画面の紋章は、本家の魔法陣の中の印をワサミのシンボルに替えたもの（2026-09-19 のユーザーの回答。実装記録 09）。

### 7. NavMesh と敵の AI（追跡型）

- 目標: 敵ワサミを本家のナースの追跡型の行動（巡回・発見・追跡・見失い・見張り・Nightmare・出現）で動かす。透明化・薬投げ・ガスは作らない（ユーザーの回答）。
- 完了の条件: 両ゾーンに NavMesh（本家の `NavMeshBoundsVolume` はブラシの形が書き出しに無いので、ステージ全体を覆う箱と `NavModifierVolume` 相当を本作で置く）。行動ツリーの値（速さ・視界の距離と角度・Camera チャンネルの視線・見失うまでの時間・巡回の行き先）と出現（Zone 1 の `Spawn Nurses_06`、Zone 2 の `BP_06_ReaperNurse_Sentry` ×6 の位置）がコードどおり。Nightmare（全回収後）で `run_fast_2`。パワーの作用（Primal・オーブの気絶 17 s〈ナースの BP の `Delay 17.0`。15 s はホテルの猿の行動ツリーの値。項目 4 の調査〉、Vanish で見失う、Telepathy の印）が効く。PIE で巡回 → 発見 → 追跡 → 接触までを収録で確かめる（接触の先は項目 9）。追跡中のランダムの動きは項目 26（大目標 2）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Characters/Nurse/BP_06_ReaperNurse*.txt`、`Animation/Enemies/Nurse/Reaper/*AnimBlueprint*.txt`、行動ツリー（`_assets/DDeception/Content/AI/**`。無ければ `_bytecode` の BTT）、WebGL 版 15 記録（頭脳の作り）。
- 依存: 4、6。
- 規模: 4
- 状態: **完了（2026-09-19）**。両ゾーンに本家のナビのボリューム（Zone 1 に 2、Zone 2 に 59）と設定で NavMesh を焼き（実装記録 01・00）、敵ワサミ `AWasamiEnemy` が本家のナースの `Make Choice`（0.5 s ごとの判断と AI MoveTo。巡回はプレイヤーの周り 3000 の乱数の点へ 350、見つけると〈前 100° 未満・`Camera` の線〉800 で追い、見失うのは Vanish と 17 s の気絶だけ）で動く。Zone 1 は迷路の 3 体（`Spawn Nurses`）と駐車場の 06 型 2 体（`Spawn Nurses_06`。毎ティック追い、トンネルの扉を突く・`NurseNear`）、Zone 2 は迷路の 3 体（階が違うとリフトへ）とミニボスの廊下の見張り 6 体（視界コーン・地図の印・見つけると跳び降りて追う）。PIE で巡回 → 発見 → 追跡 → 接触の手前（中心の間 89 cm。先は項目 9）と、Primal Fear・オーブの代わりの気絶（17 s・明けに見失う）・Vanish（見失う）・Telepathy（6 体に印）を確かめて収録した（実装記録 07・11・12・03・04）。**完了の条件の読み替え**（本家のコードに合わせた）: (1) NavMesh は本家の `NavMeshBoundsVolume` をそのまま置いた（「ブラシの形が書き出しに無い」は読み違いで、ブラシの `BodySetup` の凸の形と変換が書き出しにある）。`NavModifierVolume` 相当とリフトの部品の `AreaClass`・`NavModifier` は道を変えないので写さない。(2) 「Nightmare（全回収後）で `run_fast_2`」は作らない（下の「未回答の要確認」。本家の病院の Zone は全回収で敵を全部消し、ナースは `Activate Frenzy` を実装しないので、全回収の後に追う敵がいない。`bNightmare` とアニメの分岐は残した）。透明化・薬投げ・スケートの音は作らず、台詞は項目 20、捕獲は項目 9、見つける前に気絶した見張りの姿勢と視界コーンの地図の印の見え方は項目 28 の後回しの一覧。

### 9. 捕獲の演出（黒背景 + 敵ワサミ、3 種のシャッフル）

- 目標: 捕まると本家ホテル（Monkey Business）と同じ体で、黒い別室で敵ワサミが 3 種のモーションのどれかを再生し、`JumpscareShake`（0.3）の後、3.5 s で死亡画面（項目 5）へ。カメラの動きは項目 24（大目標 2）。
- 完了の条件: 敵に触れた瞬間に敵を全部消し、入力を止め、タブレットを下ろし、`JumpscareCam` と等価の別室のカメラ（この項目では動かない。寄りと向きは `JumpscareCam` の置き方から）に切り替え、`Backflip`・`sliding_rool`・`Stylish_Walk`（旧 glb の 3 本を v3 の体で。2026-09-18 の回答）を重複なしのランダム（直前と同じものを避ける）で再生し、`JumpscareShake` を掛け、3.5 s 後に `UMG_DeathScreen`。PIE の収録で、接触から死亡画面・チェックポイントでの再開までつながる。
- 根拠: `pak_reference/_bytecode/DDeception/Content/01_Hotel.txt`（`JumpscareMonkey`、`Monkey_Killshot_03a*` の参照、`JumpscareShake` 0.3、Delay 3.5）、`pak_reference/_levels/01_Hotel.full.json`（`JumpscareCam` と `Monkey_Killshot_01`・`02`・`03a`・`03a2`〜`03a5` の LevelSequence。`_sequences/` には書き出されていない）、`pak_reference/_camera/_camera_shakes.json`、WebGL 版 15 記録（`jumpscare.ts`。3 本と読んだが、レベルの参照は 7 本あるので本数と選び方はこの項目で確かめる）。
- 依存: 4、5、7。
- 規模: 2
- 状態: **完了（2026-09-19）**。気絶していない敵ワサミ（迷路のナース・駐車場の 06 型・Zone 2 の見張り・迷路の型）に触れると、本家のナースの `Sphere`（半径 54.928、Pawn だけ Overlap）の重なりで敵を全部消し、`AWasamiCapture` がレベルの上の遠い所に黒い別室（本家の `jumpscareblock` の板）を出して、入力を止め、タブレットを下ろし、別室のカメラへ切り替え、敵ワサミが捕獲の 3 本（`A_WasamiEnemy_Capture_1`〜`3`）の 1 本を再生し、`JumpscareShake`（0.3。本家のデータから取り込み）を掛け、暗転して 3.5 s 後に死亡画面（ライフ −1）→ チェックポイントで開き直す（実装記録 07 の「捕獲の演出」「捕獲の判定」、06・02）。PIE で 3 本とも写り、3 種の敵とも接触から再開までつながることを収録した。**完了の条件の読み替え**（本家のコードに合わせた）: (1) 「重複なしのランダム（直前と同じものを避ける）」は、本家ホテルの `Death Event` が使うマクロ `Random Integer In Range (No Repeat)` どおり 3 回で 1 巡する袋にした（巡の中では重ならないが、巡の変わり目では同じものが続くことがある）。袋は死亡でレベルを開き直してもよいようゲームインスタンスに置いた。(2) 「寄りと向きは `JumpscareCam` の置き方から」は、本家の 1 本目の Matinee の t=0 のカメラがサルの頭を写す写し方（高さ 0.534・距離 0.96 倍）を、全身を使う 3 本のためにワサミの全身へ当てた（前 222・上 123 cm。サルの足からの差を体の比で縮めた寄りと顔の高さの寄りは、PIE で後転と滑りが画面から外れた）。(3) 本家は捕まえた敵が自分以外を消すが、本作は別室のワサミが引き継ぐので自分も消す。捕獲の音（本家ホテルの Matinee のサルの叫びとナイフ。WebGL 版は叫びだけ）はまだ鳴らさず、項目 19 に書いた。別室の見た目（寄り・灯・暗転・DOF）は項目 28 の後回しの一覧、カメラの動きは項目 24。

### 13. 脱出（ガレージの祭壇 → 欠片 → ポータル）と視線の手のマーク

- 目標: Zone 2 の全回収後、ガレージの祭壇（本家 `BP_01_Statue` の見た目）を Use して欠片を取り、現れたポータルをくぐると脱出完了。流れは Deadly Decadence（旧版 `03_Manor_Zone2`）に倣う。ボス戦は作らない。
- 完了の条件: 全回収で祭壇の球（`ring_statue_orb`）が消え、見て左クリック（視線の手のマーク `UMG_Interact`。実装記録の予定 05）で `Ring_Piece_Pickup` と `UMG_01_RingPieceCollect`、ポータル（本家 `BP_00_Teleport` の見た目と `21-Ballroom_portal_V2` の音）が現れ、くぐると敵を消して脱出。全回収前は祭壇を使えない。ガレージの位置は本家の `Postmaze_Trigger_Garage`。脱出の後のスコア画面（項目 14）ができるまでは、脱出で入力を止めて画面を暗くするだけでよい。欠片の画面（`UMG_01_RingPieceCollect`）が閉じると（本家の `Ring Piece Collect `）、灯 2 つを消して Zone 2 の障壁 `BP_ZoneBarrier_2` を `DestroyBarrier` で壊し、矢印をガレージ（`Postmaze_Trigger_Garage`）へ向ける（本家 @21373〜。障壁は項目 6 で置いた）。視線の手のマークと一緒に、障壁を見て左クリックしたときの本家の `InteractWithObject`（`DD_RingBarrierDenied_louder` と `UMG_TextPrompt`「Collect all soul shards in this zone to break the barrier.」、5 s に 1 回。実装記録 08）も作る。Bierce の台詞は項目 20（大目標 2）。
- 根拠: `pak_reference/_bytecode/DDeception/Content/03_Manor_Zone2.txt`（@1385〜@4380 の祭壇と欠片、`Portal Extra Brightness`）、`pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_02.txt`（@43〜 Postmaze、@1423〜@1773 の台詞・曲のフェード・ポータルの音・敵の除去）、`Blueprints/01_Hotel/BP_01_Statue.txt`、`Blueprints/00_Ballroom/BP_00_Teleport.txt`、`UI/Main/UMG_Interact`。
- 依存: 6、7。
- 規模: 2
- 状態: **完了（2026-09-19）**。見て使う仕組み（カメラの前 200 cm のトレースとタグ `interact`、画面の中央の手のマーク、左クリック。実装記録 05）、障壁を使ったときの拒否（`DD_RingBarrierDenied_louder` と文の枠 `UMG_TextPrompt`、5 s に 1 回。08・09）、Zone 2 の祭壇 `BP_01_Statue` と欠片 `BP_08_RingPiece_NoPickup`（全回収の前は拒否の音と文、後は `Interact All Shards`。08）、欠片の画面 `UMG_01_RingPieceCollect`（ゲームを止め、閉じると `Ring Piece Collect `: 祭壇の灯 2 つ・障壁・欠片を消し、ガレージへの扉 `BP_06_DoubleDoors2` の鍵を外し、矢印と HEAD TOWARDS THE GARAGE。09・11）、ガレージのポータル `BP_00_Teleport`（本家の最新版。ロゴはワサミの印。08）がガレージの箱 `Postmaze_Trigger_Garage` で開き、矢印と GET TO THE PORTAL、ポータルの前の箱（ホテルの出口の `EndTrigger` の位置と大きさ）で入力と走りを止め、黒に暗転し、`21-Ballroom_portal_V2` を鳴らして敵を消す（11）。PIE で `Wasami.Checkpoint 10` から祭壇 → 欠片の画面 → 扉を抜けてガレージの箱 → ポータルが開く → 暗転を 1 本で通した（11 記録の「通しの脱出」）。テストは `Wasami.` の 75 本。**完了の条件の読み替え**（本家のコードに合わせた）: (1) 祭壇はガレージに新しく置かず、本家の病院 Zone 2 の迷路の後の部屋にある祭壇 `ring_statue_2` をそのまま使い、ガレージにはポータルだけを置いた（本家の病院に「祭壇 → 欠片 → 障壁 → ガレージ」の流れがコードにあり、祭壇が障壁の奥のガレージにあると障壁を壊す前に祭壇へ行けず、条件の「欠片の画面が閉じると障壁を壊し、矢印をガレージへ向ける」が成り立たない）。(2) 「全回収で祭壇の球が消え」は項目 6 の全回収（`Postmaze Transition`）が済ませている。(3) ポータルは祭壇の後に「現れる」のでなく、本家のホテルの出口に倣い、初めから鍵をかけて置き、ガレージの箱で開く。祭壇の場所・目的の文・ロゴは下の「未回答の要確認」。粒子と推定の材質（祭壇・欠片・ポータル）は項目 28 の後回しの一覧。Bierce の台詞は項目 20、曲のフェードは項目 19、スコア画面は項目 14（それまでは暗転したまま止まる）。

### 27. 大目標 1 の通しプレイの確かめ

- 目標: 大目標 1 の達成の姿を、PIE で通しで遊んで確かめ、止まる箇所を直す。
- 完了の条件: PIE で Zone 1 のエレベーターの到着 → 敵に追われながら回収（シャードはデバッグの呼び出しで数個を残して回収してよい）→ 全回収 → ガレージリフトとテレポーテーションで救急車の屋根 → Zone 2 → 全回収 → 祭壇 → ポータル → 脱出までを 1 回の収録で通し、途中で 1 回捕まって死亡画面 → チェックポイントで再開する。進めない・落ちる・壁を抜ける・敵が動かないなどの止まる箇所が無い（見つけたら直す。大きいものは記録のステップに足す）。ユーザーが自分で遊んで確かめる手順（開くレベル、PIE の始め方、デバッグの呼び出し）を `.claude/references/handover.md` の「現状と次の一歩」に書く。Discord に通しの連番のグリッドを出す。
- 根拠: この節の項目 5・6・7・9・13 の完了の条件。
- 依存: 5、6、7、9、13。
- 規模: 1
- 状態: **完了（2026-09-19）**。台本 `Tools/playthrough.py`（区間 10 個を単独でも続けても流せる。01 記録）で、`Wasami.ResetSave` から 1 回の PIE・1 本の収録（230 s）で Zone 1 のエレベーターの到着 → 扉の鍵 → 迷路で 1 回捕まり死亡画面 → 05 で再開 → 全回収 → 駐車場 → トンネル → ガレージリフトとテレポーテーションで救急車の屋根 → Zone 2 の独房 → 見張りの廊下 → 迷路の全回収 → 祭壇と欠片の画面 → ガレージ → ポータルで暗転まで通した（シャードは各ゾーン 1 個を残してデバッグで回収し、最後を歩いて取った。11 記録の「確かめたこと」の「大目標 1 の通しプレイ」）。下見で見つけた止まる箇所 2 つを直した: Zone 2 のマップに道（NavMesh）が保存されておらず敵が動けなかった（`WasamiStageTools.build_navigation` で焼いて保存。01 記録・症状索引）、祭壇は担架に囲まれ正面から届かない（北西の隙間から入る。11 記録の「既知の制約」）。遊んで確かめる手順は handover の「現状と次の一歩」。

## 大目標 2: ゲームとして一通り

- 状態: **達成（2026-09-20）**（2026-09-19 から。遊んで確かめる手順は `.claude/references/handover.md` の「現状と次の一歩」）
- 始め方: 自動（大目標 1 を達成したら、無人運転がそのまま「進行中」にして続ける。2026-09-19 のユーザーの指示）
- 達成の姿: タイトル → Zone 1 → Zone 2 → 脱出 → スコア画面まで、最終目標の要素がそろって遊べる。
- 決め方: 上の「大目標 1・2 の決め方」（見た目の詰めをしない）。
- 達成の条件: この節の項目がすべて完了（最後の項目 21 が通しプレイと性能を確かめる）。

### 14. 脱出後のスコア表示画面（`UMG_LevelClear`。WebGL 版と同じ）

- 目標: You Escaped! からリザルト（TIME・SOUL SHARDS・BONUS SHARDS・SECRETS・DEATHS・STREAK のランクと FINAL RANK）まで、WebGL 版が原作から写した時間・アニメ・音・ランクの規則で出す。
- 完了の条件: WebGL 版 10 記録（`level-clear.ts`）と 04 記録（`results.ts`）の値どおりに動き、NEXT でタイトルへ戻る（セーブは消す）。ランクの規則は本家のレベル BP が `UMG_LevelClear` に入れる値。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Menu/UMG_LevelClear.txt`、`pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt`（病院の `Escape` と `Finished Level`。値の計算。2026-09-19 にホテルの `01_Hotel.txt` から直した）、`Blueprints/Main/BP_DD_GameMode.txt`（`Check Streak`）・`UI/Menu/Streaks/UMG_ShardStreak.txt`、WebGL 版 04・10 記録。
- 依存: 13。
- 規模: 2（2026-09-19 に 1 から。SHARD STREAK の行の元になるシャードの連続回収〈本家 `BP_DD_GameMode` の `Check Streak` と `UMG_ShardStreak`。200・500 でライフ +1〉がどの項目にも無かったので、この項目に入れた）
- 状態: **完了（2026-09-19）**。リザルトの規則 `FWasamiLevelResults`（病院の `Escape` が入れる 6 行のランクと加算、TOTAL SHARDS・FINAL RANK）、シャードの連続回収（ゲームモードの `CheckStreak` と節目の画面。200・500 で EXTRA LIFE !）、スコア画面 `UWasamiLevelClearWidget`（旧版 `UMG_LevelClear` の木・`ClearAnimation`・`ShowResults` の Delay の連鎖・行のアニメと判の音・数え上げ・NEXT）、ゲームモードの `Escape`（一時停止・チェックポイント 0 の保存・スコア画面）と `FinishedLevel`（1 s 後にセーブの病院の欄を空に・回収の記憶とライフを戻して Zone 1）ができた（実装記録 13）。PIE で Zone 2 のポータル → You Escaped! → リザルト（TIME 3 : 07 S・TOTAL SHARDS 1,483・FINAL RANK B）→ NEXT → Zone 1 の到着まで通した（台本 `z2_escape`。13 記録の「確かめたこと」）。テストは `Wasami.LevelClear.*` 4 件ほか。**完了の条件の読み替え**: (1) ランクの規則と値は、ホテルでなく本作のステージの病院のレベル BP `06_Hospital` の `Escape` から取った（時間の境目 2700 / 3600 / 4200 s、シャード 679 など）。木・アニメ・音・時間は WebGL 版と同じ旧版の `UMG_LevelClear`。(2) NEXT の行き先は、タイトル（項目 17）ができるまで Zone 1 の最初（死亡画面の QUIT TO TITLE と同じ代わりの道。項目 17 でタイトルに替えた）。セーブは本家どおり病院の欄だけを空にする。(3) XP の箱（レベルアップ）と DIARY UNLOCKED! は作らない（本作のパワーは Lv5 固定で、日記も無い。WebGL 版も同じ）。BONUS SHARDS・SECRETS は項目 10・12 まで 0、EASY MODE は項目 18 まで出さない。TOTAL SHARDS が 679 を 2 回数えることと病院の題字は下の「未回答の要確認」。

### 17. タイトル画面（NEW GAME / RESUME / OPTIONS / QUIT、ポップアップ。WebGL 版と同じ）

- 目標: WebGL 版が原作 `UMG_TitleScreen` から写したタイトル画面（配置・アニメ・音・NEW GAME の確認・開始の演出）を UMG で作る。ロゴとワサミの顔は本作の素材（`<WEBGL>/public/title/logo.webp`・`wasami-face.webp` ほか）。
- 完了の条件: WebGL 版 10 記録（`title.ts`）の曲線・時間・音量どおりに動き、NEW GAME で Zone 1（エレベーターの到着から。項目 6）へ、RESUME でセーブの地点へ。死亡画面の QUIT TO TITLE（09 記録）とスコア画面の NEXT の後（13 記録の `FinishedLevel`）の行き先を、今の代わりの道（Zone 1 の最初）からタイトルに替える（2026-09-19 に項目 14 から足した）。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt`・`UMG_PopUp.txt`、`_assets/**/UMG_TitleScreen.json`、WebGL 版 10 記録、`<WEBGL>/public/title/`。
- 依存: 5（セーブ）。
- 規模: 2
- 状態: **完了（2026-09-19）**。タイトルのレベル `L_Title`（パッケージの始まりのマップ `GameDefaultMap`）とゲームモード `AWasamiTitleGameMode`、画面 `UWasamiTitleScreenWidget`（旧版 `UMG_TitleScreen` の木・ホバー・Slideshow・曲・NEW GAME の FadeOut〈黒・脈動・赤と開始の音と声〉・RESUME の FadeOut_0）、ボタンの道（NEW GAME は進みがあれば RESTART? と問うてセーブを消し 10 s で Zone 1、RESUME は 5 s でチェックポイントのゾーン、OPTIONS は選択音だけ、QUIT は問うて閉じる）、素材（本作のロゴとワサミの顔に WebGL 版の色とマスクを焼き込む前処理 `Tools/dd/prepare_title.py`、本家の筆の跡・煙の黒・選択の印・曲・開始の音と声、焼き込みのシェーダーから組んだ筆の跡の材質）ができた（実装記録 14）。死亡画面の QUIT TO TITLE とスコア画面の NEXT の後の行き先をタイトルにした（09・13）。PIE で NEW GAME（問いあり・なし）→ Zone 1 の到着、ゲームオーバーの QUIT TO TITLE → RESUME → Zone 2、脱出のスコア画面の NEXT → タイトルを通した（台本 `Tools/playthrough.py` の区間 `title`。14 記録の「確かめたこと」）。テストは `Wasami.Title.*` 4 件ほか。**完了の条件の読み替え**: (1) 「WebGL 版 10 記録の曲線・時間・音量どおり」は、WebGL 版が写した元の旧版 `UMG_TitleScreen` の書き出しのキーから作った（元が同じなので値も同じ）。ただし暗転の後にレベルを開くまでは本家の Delay（NEW GAME 10 s・RESUME 5 s。WebGL 版の 3.75 s はブラウザの都合）。(2) RESUME は本家の問い `UMG_PopUp_Resume` を出さず（WebGL 版と同じ）、セーブのチェックポイントのゾーン（本家の入口 `06_Hospital` の振り分けで 7〜10 は Zone 2、ほかは Zone 1）を開く。(3) 死亡画面の QUIT TO TITLE はセーブを消さない（本家どおり。タイトルの RESUME でチェックポイントから続ける）。スコア画面の NEXT の後はセーブの病院の欄を空にしてタイトルへ（本家は次の章を開くが、本作に次の章は無い）。(4) OPTIONS の画面は項目 18。CHAPTERS・REPLAY・EXTRAS は作らない（本作は 1 ステージ）。右上の版の文字は下の「未回答の要確認」。

### 18. オプション画面とポーズ画面（WebGL 版と同じ）

- 目標: `UMG_Options`（画質・解像度・明るさ・音量 3 種・字幕・マウス感度・頭の揺れ・Y 反転・ダッシュの切り替え・マウスのスムージング・難易度。設定のセーブ `BP_DD_Settings_SaveGame`）と `UMG_Pause`（Esc）を WebGL 版の記録どおりに作る。
- 完了の条件: WebGL 版 04 記録（`settings.ts`）の既定値と適用先、10 記録（`options.ts`・`pause.ts`）の配置・アニメ・音どおり。マウス感度は `AWasamiPlayerCharacter::MouseSensitivity`（`Look` の値に掛ける。本家の `Character.MouseSensitivity` と同じ）に入れ、`DefaultInput.ini` の `AxisConfig` の 0.07 と視点の修飾子には触らない（項目 2 の結果。02 記録）。
- 根拠: `pak_reference/_bytecode/DDeception/Content/UI/Main/UMG_Options.txt`、`UI/Menu/Pause/UMG_Pause.txt`、`Blueprints/Save/BP_DD_Settings_SaveGame`、WebGL 版 04・10 記録。
- 依存: 17。
- 規模: 2
- 状態: **完了（2026-09-19）**。設定のセーブ `UWasamiSettingsSaveGame`（スロット `Settings`、本家の既定値）とその適用（両ゲームモードの BeginPlay。音量は本家の `DD_SoundMix` と SoundClass 5 つのクラスの上書き、スケーラビリティはパッケージだけ）、プレイヤーが読む値（感度〈設定 ÷ 0.5 を `Look` に掛ける〉・Y 反転・頭の揺れ・ダッシュの切り替え・マウスのスムージング）、難易度の効き先（スコア画面の EASY MODE、死亡画面の EASY の分岐）、オプション画面 `UWasamiOptionsWidget`（木・FadeIn・スライダーの 1/9 の吸着・矢印・SAVE & EXIT・CANCEL。タイトルの OPTIONS とポーズの OPTIONS から開き、タイトルの外では DIFFICULTY を外す）、ポーズ画面 `UWasamiPauseWidget`（Esc で開く。木・FadeIn・曲・時間の止まり、RESUME・RESTART? の YES / NO・OPTIONS・GIVING UP? の QUIT TO TITLE / QUIT TO DESKTOP / CANCEL。頭は本作のワサミを赤で塗ったもの）ができた（実装記録 15・09・02・14・01）。PIE でオプションの値の保存と効き（感度 1 で同じマウスの動きの回る角度が 2 倍）、ポーズのボタンの道、EASY でライフ 0 の死亡画面の上のポーズを確かめ、台本 `Tools/playthrough.py` の区間 `pause` で通した（15 記録の「確かめたこと」）。テストは `Wasami.Settings.*`・`Options.*`・`Pause.*`・`DeathScreen.Easy` ほか。**完了の条件の読み替え**: (1) オプション画面は旧版 `UMG_Options` の木を写した（最新版は AutoSettings プラグインの `SettingsUI` に替わり `UMG_Options` が無い。WebGL 版 04・10 記録も旧版を写したもの）。設定の項目は両版で同じ（最新版の `VSync`・`Motion Blur` は持たない）。(2) ポーズ画面の木・アニメ・音は旧版（両版で同じ）、開き方は旧版のキャラクターの Esc と Z 5（最新版の Z 1 では死亡画面の下になる）、ボタンの道の規則は両版を比べて違った RESTART の YES を最新版にした（セーブの病院の欄を空にして今のレベルを開き直す。ライフは戻さない）。QUIT TO DESKTOP は置いた（WebGL 版はブラウザなので置かなかった）。(3) 死亡画面の EASY はユーザーの回答で最新版に倣い、ライフ 0 でもゲームオーバーにせず止まる。本家どおりだと抜け道が無いので、その画面の上でだけ止まっていても Esc でポーズが開き、RESUME は止まりを解かない。(4) オプション画面の Esc は本家どおり何もしない（WebGL 版は CANCEL）。(5) マウス感度は設定 ÷ 0.5 を掛ける（ユーザーの回答。本家は値そのもの）。PIE の Esc はエディタが取るので、確かめは `Wasami.Pause`。残った要確認 4 件は下の「未回答の要確認」。

### 8. 動く部品と罠（両開き扉・除細動器・スピードバリア・のこぎりの罠・扉の破壊）

- 目標: 静的に置いてある部品を本家の BP どおりに動かす。除細動器は**罠**（`Charge` → `Fire` で 1.25 s ごとに `BP_HitFX` と実績 `06_Trap`、触れた player に `DeathEvent`）。
- 完了の条件: Zone 1 の両開き扉 62 + Zone 2 の 1、除細動器 23 + 13、スピードバリア 4（`BP_SpeedBarrier`。Speed Boost の突進でだけ壊れる。ダッシュでは拒否）、Zone 2 ののこぎりの罠 74（`BP_06_sawTrap_short01` 27・`short02` 16・`medium` 21・`long01` 10）、`BP_06_Hospital_DoorBreak` ×2 が、位置・タイミング・音・死亡の判定までコードどおりに動く。PIE で各種 1 つずつ収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_Defib.txt`・`BP_06_DoubleDoors.txt`・`BP_06_Hospital_DoorBreak.txt`・`Traps/BP_06_sawTrap_medium.txt`・`Traps/BP_06_TrapBase.txt`、`Blueprints/Main/Traps/BP_SpeedBarrier.txt`、`_levels/06_Hospital_Zone_0*.full.json`。
- 依存: 5（死亡）。
- 規模: 4
- 状態: **完了（2026-09-19）**。両開き扉を Zone 1 に 62 枚（Zone 2 の 1 枚は項目 6）、除細動器 `AWasamiDefib`（本家 `BP_06_Defib` の `Charge`・`Fire`・`Player Hit`）を両ゾーンに 23 + 13、スピードバリア `AWasamiSpeedBarrier`（`BP_SpeedBarrier`。Speed Boost の間だけ砕ける）を Zone 1 に 4、のこぎりの罠 `AWasamiSawTrap` と派生 3 つ（`BP_06_TrapBase` + `sawTrap_*`。骨入りのメッシュとアニメ、刃に付く当たり、閃き → 黒い画面 → 死）を Zone 2 に 74 置いた（実装記録 08・01・09・12）。PIE で扉の開閉（ナースとプレイヤー）、除細動器の放電・揺れ・台の間での死、スピードバリアがダッシュで止まり Speed Boost で砕けること、のこぎりの刃がせり上がって乗ると死ぬことを確かめ、台本 `Tools/playthrough.py` の頭からの通し（11 区間）が罠を置いた後も通る（台本は除細動器の放電を待って抜ける）。08 記録の「確かめたこと」。テストは `Wasami.Defib.*`・`SpeedBarrier.*`・`SawTrap.*` ほか。**完了の条件の読み替え**: (1) `BP_06_Hospital_DoorBreak` ×2 は項目 6 で両ゾーンに置いて流れに結んであったので、確かめるだけにした（通しで Zone 1 のエレベーターの前と Zone 2 の独房の 2 つとも外れた）。(2) 「各種 1 つずつ収録」は 08 記録の「罠を置いた後の通し」に並べた収録（扉 `door_walk`・除細動器 `pie-defib-fire2`・`pie-defib-hit`・スピードバリア `sb_break`・のこぎり `saw_catch`・扉の破壊 `traps_through6`。git の外）。(3) 罠の実績 `06_Trap` は写さない（本作に実績の仕組みが無い）。(4) 稲妻・網目・刃の見え方は本家と見比べていない（大目標 1・2 の決め方）。

### 10. 特殊シャード 2 種（スタンオーブ・敵の位置が地図に出るボーナスシャード）

- 目標: 本家と同一の見た目の `BP_PowerOrb`（オーブ）と `BP_BonusShard`（赤いシャード）を、本家の出現点と周期で出し、取ると敵が 17 s 気絶（倒れる 2 本からランダム → 明けに起き上がる。秒数とアニメは敵の側。項目 4）／敵が 60 s タブレットの地図に出る。
- 完了の条件: 出現点（Zone 1: オーブ 11・ボーナス 10、Zone 2: 10・10）と周期・最初の出現の時刻がレベル BP とアクタのコードどおり。メッシュと材質は原作のアセット（`power_orb`、赤いシャードの材質）。取得で `UMG_VignetteSides`（`ENEMIES STUNNED` / `ENEMIES REVEALED`。WebGL 版 10 記録）が出て、全敵に `SetState(Stun, byOrb)`、地図に敵の印。テレキネシスでは引き寄せられない。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_PowerOrb.txt`・`BP_BonusShard.txt`、`_assets/**/BP_PowerOrbSpawnPoint*`・`BP_BonusShardSpawnPoint*`、`.claude/references/dark-deception/04-mechanics-items.md`（原作データの値）、WebGL 版 08 記録。
- 依存: 7、9（気絶のモーションと敵の印）。
- 規模: 2
- 状態: **完了（2026-09-19）**。オーブ `AWasamiPowerOrb`（本家 `BP_PowerOrb`）と赤いシャード `AWasamiBonusShard`（`BP_BonusShard`）を共通の基底 `AWasamiSpecialShard`（本家の部品、150 s のタイマー → 0.1 s ごとの明滅 5 s → 閃光とともに出現点を乱数で選んで移る）で作り、取得の演出 `AWasamiStunCollectEffect`・`AWasamiBonusShardCollectEffect`、取得の画面 `UWasamiVignetteSidesWidget`（`UMG_VignetteSides`）、出現点 2 種、プレイヤーの地図の `AddToMap`・`RemoveFromMap`、敵の地図の印を足して、両ゾーンに本体 2 つずつと出現点（Zone 1: 11・10、Zone 2: 10・10）を置いた（実装記録 16・02・04・07・01）。PIE で出現点と周期（155 s で出現点へ移る）・明滅・取得（オーブで迷路のナース 3 体が気絶、赤いシャードで 50 m の内のナースが地図に出て 60 s で外れる）・セーブ（取った赤いシャードはチェックポイントからの再開で出ない、スコア画面の BONUS SHARDS 1/2）を確かめ、台本 `Tools/playthrough.py` の頭からの通し（11 区間）が通る（16 記録の「PIE での確かめ」）。テストは `Wasami.PowerOrb.*`・`BonusShard.*`・`VignetteSides.*`。**完了の条件の読み替え**: (1) 最初の出現の時刻は、病院の Zone のレベル BP がオーブと赤いシャードに触れないので、アクタのコード（`Shard Spawn Time` 150 s + 明滅 5 s = 155 s。病院の置いたものは既定のまま）から取った。(2) 赤いシャードのメッシュと材質は本家の `soul_shard` × 20 と `m_crystal_Inst`（WebGL 版の餅にしない。最終目標の「本家と同一の見た目」）。結晶と閃光の材質・地図の印の色はグラフが cook で消えていて推定（項目 28 の後回しの一覧）。(3) 「地図に敵の印」は本家どおりクラスごとに地図へ足し（`Add To Map(GetObjectClass)`）、本家のナースの地図の印（`Plane`・`M_Enemy`）を敵ワサミに付けた。(4) ゲームインスタンスの `Used Stun Orbs?`（実績だけに使う）は写さない。

### 11. Zone 2 の Matron（大きい敵ワサミ。視界コーンの中ボス）

- 目標: Zone 2 の `BP_06_Matron_MiniBoss` を、大きい敵ワサミとして本家の巡回路・視界コーン（`BP_06_Miniboss_viewcone_Matron_Long` / `_Short`）・発見で追跡型の行動に入る形で置く。
- 完了の条件: 巡回路と視界コーンの形・速さ・見つかった後の行動がコードどおり。モデルは `tmp/boss_wasami.glb`（2026-09-18。アニメの役は `.claude/references/enemy-wasami-motions.md` の「ボスワサミ」）で、大きさは本家の Matron から決める。PIE で見つからずに通れることと、見つかると追われることを収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Animation/Enemies/Nurse/Matron/MiniBoss/BP_06_Matron_MiniBoss.txt`、`Blueprints/06_Hospital/Miniboss/*.txt`、`_levels/06_Hospital_Zone_02.full.json`。
- 項目 7 から（2026-09-19）: 視界コーンの基底 `AWasamiViewcone`（本家の `BP_06_Miniboss_viewcone`）・インターフェース `IWasamiViewconeInterface`（`BPI_06_Viewcone`）・見張りの `_Nurse` と地図の印の材質、Zone 2 の流れの `Activate MiniBoss Enemies`（Matron の `MnM_Matron_Idle_2` を呼ぶ所だけ空け）は作ってある（実装記録 07・11）。この項目は Matron と `_Matron_Long`・`_Short` を足す。
- 依存: 7。
- 規模: 2
- 状態: **完了（2026-09-19）**。ボスワサミ（`boss_wasami.glb`）を取り込み（本家の `Matron_MiniBoss_AnimBP` が流す Idle・Alert・Detected の 3 本）、Matron `AWasamiMatron`（本家 `BP_06_Matron_MiniBoss`。1 s ごとの `Switch` がプレイヤーが机の前の箱 `CloseArea` にいるかで長短の視界コーンを切り替え、見つけるとコーンを消して Detected を流し、1.0948 s で見張り全部に `Player Spotted`）とボスのアニメ `UWasamiBossAnimInstance`（Idle ↔ Alert・最後の姿勢で止まる Detected・`spine_02` の LookAt）、視界コーン `AWasamiViewconeMatronLong`（3000・35°）/ `Short`（1350・35°）を作り、Zone 2 に本家の位置と部品の変形で置いて、流れの `ActivateMinibossEnemies` から起こした（実装記録 17・07・01・11）。PIE（チェックポイント 8）で大きさ（上端は床から約 860 cm、天井の下）・長短のコーンの切り替え・長いコーンに入ると見張り 6 体が跳び降りて追い捕まえることを収録し、台本 `Tools/playthrough.py` の `z2_corridor` を Vanish で Matron の横を抜ける道にして z2_corridor → z2_maze → z2_altar を通した（17 記録の「PIE で確かめたこと」）。テストは `Wasami.Matron.*` 5 件。**完了の条件の読み替え**（本家のコードに合わせた）: (1) 巡回路は無い（本家の Matron は `Actor` で、移動の部品も処理も無い）。「速さ」は Switch の 1 s とコーンの見張りの間隔。(2) 「見つかると追われる」は、Matron の発見（本家の Detected のモンタージュの 1.0948 s の通知）で見張り 6 体が追う形（Matron 自身は追わない）。(3) 「見つからずに通れる」は、本家のレベルの置き方では長いコーンを隠れて渡れる道がほぼ無い（25 cm の格子で調べた。隙間は 25〜60 cm の線だけで運任せ）ので、Vanish で消えて横を抜ける道で収録した。(4) 大きさは本家の長いコーンの高さ（z 676）にボスの Idle の頭の骨が来る拡縮 6.111（仮。項目 28 の後回しの一覧）。(5) 発見の声 `Matron_ReinforcementCall_01` は項目 20（敵の声）へ回した。

### 12. 秘密と収集物（シークレット）

- 目標: 本家の秘密の部屋と収集物を同じ場所に置き、スコアの `SECRETS` に数える。
- 完了の条件: `BP_SecretRoomZone` ×1・`BP_07_Zone1_SecretWall` ×1（Zone 2）、`BP_MysteryCollectable` ×3（Zone 2）、`BP_Collectable` ×2（Zone 1）+ ×1（Zone 2）が（入口の `BP_MysteryCollectable_LoreNote` ×1 は入口ごと作らない。2026-09-18）、見つけ方・取り方・表示（`UMG_Collectables_Secret`）までコードどおり。取ると進行にセーブされる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Shared/BP_SecretRoomZone.txt`、`Blueprints/Main/BP_MysteryCollectable*.txt`・`BP_Collectable.txt`、`UI/Main/UMG_Collectables_Secret.txt`、`_levels/06_Hospital*.full.json`。
- 依存: 5、6。
- 規模: 2
- 状態: **完了（2026-09-20）**。素材（音 5・絵 6、コンパイル済みシェーダーから写したグリッチ `M_DD_ChameleonGlitch`）、画面 3 つ（書類の `NEW EXTRAS UNLOCKED!`〈本家 `UMG_Collectables`〉・秘密の部屋の `YOU FOUND A MYSTERIOUS ROOM`〈`UMG_Collectables_Secret`〉・メモを読む画面〈`UMG_MysteryNote`。ゲームを止める〉）、書類 `AWasamiCollectable`（触れると声・画面・セーブの `Hospital.Secrets` に ID・消える。セーブにあれば 0.2 s 後に消える）・秘密の部屋の区域 `AWasamiSecretRoomZone`（出入りで囁きとグリッチ）・秘密の壁 `AWasamiSecretWall`（見て使うと 275 上がる）・メモ `AWasamiMysteryCollectable`・見て使う偽の部品 `AWasamiFakeUseActor` と派生 2 つ（秘密のエレベーターのシーケンス・おとりのエレベーターの扉）を作り、両ゾーンに本家の位置で置いた（実装記録 18・01・11）。PIE で両ゾーンの秘密・おとり・メモ・書類 4 つ・死んでも戻らないこと・スコアの `SECRETS 4/4` を確かめ（18 記録の「確かめたこと」）、台本 `Tools/playthrough.py` がのこぎりの罠の刃も待って渡るようにして頭から 11 区間が通った（01・08 記録）。テストは `Wasami.Secrets.*` 12 件。**完了の条件の読み替え**（本家のコードに合わせた）: (1) `BP_Collectable` は置いたもの 3（Zone 1 の 2 つは秘密のエレベーターの奥で、呼びボタン〈`BP_FakeUseActor_SequencePlayer`〉を使うと扉が開く。Zone 2 の 1 つは秘密の部屋）に、迷路の後にレベル BP が目印 `collec` に出す ID 3 を足した 4 つで、`SECRETS` の分母 4 と合う。(2) `BP_SecretRoomZone` とメモは `SECRETS` に数えない（表示と囁き・グリッチだけ。本家どおり）。(3) 書類の `Unlock` が本家の別のセーブに足す EXTRAS は、画面 `NEW EXTRAS UNLOCKED!` だけ出して保存は項目 29 へ回した（2026-09-20 の回答）。(4) Zone 2 の秘密の壁と部屋は 2 階にあり、本家どおりテレポートの床の上の層（迷路の入口の上の歩き道）から上がる。(5) 完了の条件の外で、Zone 1 のおとりのエレベーター 5 つ（`BP_FakeUseActor_06_HospitalZone1_Elevator`。扉が開いて空）も同じ基底で置いた。グリッチと書類の縁の光は推定（項目 28 の後回しの一覧）。

### 30. ステージ OP（本家のタイトルカード `UMG_ChapterPortal`。紋章と題字）

- 目標: 2026-09-20 の有人セッションの指摘「ステージOPがありません。紋章＋ステージロゴのやつ」。本家は入口 `06_Hospital` のレベル BP の `00_Initial Start` が `UMG_ChapterPortal`（`Level` 7）を出す。入口ごと取りやめた項目 16 に入っていたため抜けていた。本作は Zone 1 のエレベーターの上昇・到着（`06_Hospital_Zone01_ElevatorArrive`）から始まるので、その始まりに出す。
- 完了の条件: (1) 本家の `UMG_ChapterPortal` の木（ぼかし・赤い幕・黒い筆の帯 2 本・回る赤い輪とルーンの輪〈紋章〉・頭・題字）とアニメ `loop` を写したウィジェットを作る。`Construct` どおり `loop` を 0.8 s から 1 回・速さ 1 で流し、11 s で消える。(2) 頭は本家の `pause_reapernurse_head` の代わりにポーズ画面のワサミの頭（`/Game/Wasami/UI/Pause/T_PauseHead`。本家の頭と同じく赤く塗る）、題字は本家の `chapter_ui_title_tormenttherapy` の代わりにスコア画面と同じ「Stinky Gachimi」（`/Game/Wasami/UI/T_LevelTitle`。2026-09-20 の回答）。WebGL 版 10 記録の `stage-intro.ts` が同じ置き換えで写してあるので、値と置き方はそれに合わせる。(3) 出す時機と、その間にプレイヤーを止めるか（本家は出すと同時に `CanMove?` を偽にし、10 s 後に真）は、本家のレベル BP と Zone 1 の開始の流れ（エレベーターの 14.1 s）から決める。NEW GAME で Zone 1 を始めたときに出し、死んでチェックポイントから再開したとき（本家の `Respawn`）は出さない。(4) PIE で、タイトル → NEW GAME → エレベーターの上昇の間に OP が出て消えるまでを収録する。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt`（`00_Initial Start` → @19290。`python Tools/dd/bp_flow.py … "00_Initial Start"`）、`pak_reference_2/_bytecode/DDeception/Content/UI/Menu/UMG_ChapterPortal.txt`・`_assets/DDeception/Content/UI/Menu/UMG_ChapterPortal.json`（木とアニメ `loop`）、`UI/Main/chapter_ui_portal_outer` ほかのテクスチャ、WebGL 版 10 記録の「stage-intro.ts」と 04 記録の「ステージ OP」、実装記録 09（UI）・11（Zone 1 の開始）・15（ポーズの頭）・13（題字）。
- 依存: 6、17。
- 規模: 2
- 状態: **完了（2026-09-20）**。本家の `UMG_ChapterPortal` の木（ぼかし・薄い赤の幕・帯 2 本・輪とルーンと頭の `Icon`・題字 `TitleCard`）をスロットのまま C++ で組んだ `UWasamiChapterPortalWidget` と、その素材（`dd_ui.import_chapter_portal`: 輪・ルーン・帯 2 枚）、アニメ `loop`（10 本のトラックを書き出しのキーと接線で）と Construct（0.8 s から 1 回・11 s で外れる）を作った（09 記録）。出すのは本家の入口の `Spawn` どおり Zone 1 をセーブのチェックポイント 0 で開いたとき（ゲームモードの `PrepareStart` が 0 を 4 に書き直したときの `IsNewStart`。NEW GAME・RESTART・脱出の後）で、`00_Initial Start` どおり次のティックに出してプレイヤーの `bCanMove` を 10 s 偽にする（11・06 記録）。エレベーターの扉は 11.3 s に開くので流れは変えず、WebGL 版の推定の黒は写さない。テストは `Wasami.ChapterPortal.*` 2 件と `Wasami.ZoneFlow.NewStart`。PIE でタイトルの NEW GAME → 暗転 → Zone 1 のエレベーターの中で OP が出て約 9 s で消え、10 s で動け、約 11 s に扉が開くことと、`Wasami.Kill` → 開き直しでは出ないことを収録で確かめた（11 記録の「確かめたこと」の「ステージ OP の通し」）。

### 31. 祭壇の見た目（祭壇の金属・水晶の球・欠片の材質をコンパイル済みのシェーダーから）

- 目標: 2026-09-20 の有人セッションの指摘「シャードリングの祭壇の見た目が本家と異なる気がします。私が記憶しているのは、MonkeyBusinessの祭壇」。本家の病院の祭壇は Monkey Business（ホテル）と同じ `BP_01_Statue`・`ring_statue`・材質 `MM_00_Ballroom_Ring_Altar_Metal`（ホテルは拡縮 2.8、病院は 4）と水晶の球 `ring_statue_orb`（`m_crystal_Inst2`）。本作は 3 つの材質の親（祭壇の `MM_Main_Metal`、球の `m_crystal`、欠片の `M_ring_metal2` の `MM_Main_Substance_Fresnel`）が前処理のマスターに無く、`substance` の推定で作られている（祭壇が青みの白、球が紫に光らない）。項目 28 の後回しの一覧から移した。
- 完了の条件: (1) 祭壇: `MM_Main_Metal` をコンパイル済みのシェーダー（`python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Metal."`）の式で組む。2026-09-20 に読んだ式: 基底色は定数 (0.276042, 0.255386, 0.148085)（真鍮色）、金属 1、スペキュラ 0.5、粗さ `saturate(Roughness)`、法線は `Normal` のテクスチャを `Normal Flatness` で (0, 0, 1) へ寄せる、発光 = ((1 − max(N·V, 0))^6 × 0.999 + 0.001) × `Hover Intensity` × `Hover Color`（祭壇は `Hover Intensity` 0 なので光らない）。(2) 球: `m_crystal_Inst2`（紫、`emissive_entensity` 29）を、特殊シャードで組んだ `m_crystal` の推定（`dd_specials` の `M_DD_Crystal`。実装記録 16）の子にする。(3) 欠片: `MM_Main_Substance_Fresnel` を同じくシェーダーの式で組む。(4) 置いたものを組み直さずに済むなら材質だけ作り直し、PIE で Zone 2 の祭壇（全回収の前の球つき）を撮る。大目標 2 の決め方どおり、本家の実機との見比べはしない。
- 根拠: `Intermediate/Pipeline/dd/shaders/`（上のコマンドの出力）、`pak_reference_2/_materials.json`（3 つのインスタンスの値）、`_assets/DDeception/Content/Blueprints/01_Hotel/BP_01_Statue.json`、`_levels/01_Hotel.full.json`（ホテルの `BP_01_Statue`・`ring_statue_orb_4`）・`06_Hospital_Zone_02.full.json`（`ring_statue_2`・`ring_statue_orb_5`）、実装記録 01（前処理のマスター `MASTERS`・`dd_stage`）・08（祭壇と欠片）・16（`m_crystal`）。
- 依存: 13。
- 規模: 1
- 状態: **完了（2026-09-20）**。前処理 `MASTERS` が根で振り分け（祭壇 `metal`・欠片と書類 `fresnel`・球 `crystal`）、ステージの取り込み `dd_stage` がコンパイル済みのシェーダーの式で組んだ `M_DD_Metal`（真鍮の金属）・`M_DD_SubstanceFresnel`（紫の縁の光の欠片・白い縁の書類）に載せ、球は `dd_specials.make_crystal` に作らせて `M_DD_Crystal` の子（紫の渦の光）にした。4 つとも同じパスのまま作り直し（`refresh_settings` の `materials_remade`）、置いたものは置き直していない。PIE で Zone 2 の祭壇を、全回収の前の球つきを静止画で、全回収の後に欠片を取る区間 `z2_altar` を収録で撮った（実装記録 01・08・16・18）。本家の実機とは見比べていない（項目 28 の後回しの一覧）。

### 29. EXTRAS（秘密の書類で解放される収集物。タイトル画面から開く）

- 目標: 2026-09-20 のユーザーの回答「EXTRAを実装し、タイトル画面から参照できる形に」。秘密の書類（項目 12）を取ると、本家どおり EXTRAS が解放されて保存され、タイトル画面の EXTRAS から見られる。中身は同日の回答「枠組みだけ先に作る」: 本家どおりの画面と、解放・保存の仕組みを作り、並べる中身（絵・音・動画）は仮にしておく（本家の Art Gallery などの素材は使わない。中身は後でユーザーと決める）。
- 完了の条件: (1) タイトル画面に EXTRAS の入口がある（本家の旧版 v1.6.1 の `UMG_TitleScreen` にあればその位置と見た目、無ければ最新版に倣う。項目 17 の画面に足す）。(2) 本家の `UMG_Extras`（Art Gallery・Diary・Sound・Movie の区分〈`Enum_Collectables`〉と一覧、1 つを大きく見る `UMG_Extras_Extra`、音の `UMG_Extras_Sound_Button`・`_Bar`、動画の `UMG_Extras_Extra_Video`）を本家の木と動きどおりに作る。まだ解放していないものの見せ方も本家どおり。(3) 書類の `Unlock` が、置いた書類の `Collectables`（本家の病院の書類 4 つの値）を、本家の別のセーブ（`SaveSlot` の `Extras_Art`・`Extras_SFX` など）に当たる本作のセーブに足して保存し、EXTRAS に解放として出る。消えるのは本家と同じ時だけ（死亡・RESTART・新しいゲームで消えるかは本家のコードどおり）。`UWasamiCollectable` と `UWasamiCollectablesWidget` の `TODO(item 29)` を外す。(4) 中身は仮（本作にある絵や、区分と番号だけの札など。`TODO(仮)`）。(5) テストと、PIE で書類を取る → タイトルの EXTRAS に解放が出るのを確かめる。
- 根拠: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/UMG_Extras*.json`・`Extras_*.json`、`pak_reference*/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_Extras*.txt`、`Blueprints/Main/BP_Collectable.txt`（`Unlock`）、`Blueprints/Enums/Enum_Collectables`・`UI/Main/Collectables/Struct_Collectable`、本家のセーブ `SaveSlot` の型、レベルの書類の `Collectables` の値（`_levels/06_Hospital_Zone_0*.full.json`）。実装記録 18（書類）・14（タイトル画面）。
- 依存: 12、17。
- 規模: 3
- 状態: **完了（2026-09-20）**。セーブ `UWasamiSaveGame` に `ExtrasArt`・`ExtrasSFX` と `Unlock`、書類 `AWasamiCollectable` に `Collectables` と `Unlock`（読んだ写しに足してすぐ書き、ゲームモードの写しにも足す。両ゾーンの書類に本家の値を入れた）、素材の取り込み `dd_ui.import_extras`（絵 5・音 1・焼き込みのシェーダーから組んだ背景の材質）、最新版の `UMG_Extras` と部品（絵・動画・大きく見る画面・音のボタン・再生バー）を木と流れのまま C++ で組んだ画面、タイトル画面の EXTRAS（旧版の並びで NEW GAME と OPTIONS の間。押すと曲が消え、BACK で戻る）を作った（実装記録 06・18・09・19・14・01）。テストは `Wasami.Extras.*` 5 件と `Wasami.Secrets.Collectable.Unlock`。PIE で Zone 1 の秘密のエレベーターの書類（ID 1）を取り、タイトルの EXTRAS で Art Gallery の 19・20 だけが解放されて大きく見られるのを確かめた（19 記録の「確かめたこと」）。**完了の条件の読み替え**: (1) 画面は最新版の木（病院の書類が解放する Art 19〜22・Sound 5 は最新版にだけある）、入口の位置と押したときの流れは旧版のタイトル画面。(2) 本家は EXTRAS を別のセーブ `SaveSlot` に持つが、本作は 1 つのセーブに持ち、消えるのは NEW GAME の消去だけ（本家の `Erase Save Files` と同じ。死亡・RESTART では消えない）。(3) 日記の解放は本家の `Level Ranks` の判定のままで、本作ではいつも ID 0〜8 が解放。動画はいつも鍵で MOVIES の区分は本家どおり隠れる。中身は仮（絵 19〜22 は本作の絵、日記と曲は空、クレジットは本作の数行。`TODO(仮)`）で、何を並べるかは「未回答の要確認」。

### 24. 捕獲のカメラの動き（本家ホテルの捕獲の体）

- 目標: 捕獲の別室のカメラ（項目 9）に、本家ホテル（Monkey Business）の捕獲のような躍動的・非機械的な動きを付ける（最終目標の原文「カメラは本家Monkey Business捕獲時のような、躍動的・非機械的な臨場感あるアニメーションを設ける」）。2026-09-19 の有人セッションの指摘で、寄りを近くし、ワサミの動きを最初は速くイージングで等速に戻し、顔を間近に写すジャンプスケア（Deadly Decadence のような。背景は黒）を 1 本足すことも含めた（「決めたこと」の「捕獲のカメラ」）。
- 完了の条件: 本家のシーケンス（`Monkey_Killshot_*`）のカメラのトラック（位置・回転・FOV）が原作データから読めれば、それをワサミのモーションの長さに伸縮して当てる。読めなければ、3 本のモーションそれぞれに手で付けた動き（寄り・回り込み・揺れ。仮の値）にし、項目 28 の後回しの一覧に「本家旧版の実機の捕獲と見比べる」を書く。PIE の収録で 3 本とも止まらずに動く。
- 根拠: `pak_reference/_levels/01_Hotel.full.json`（`JumpscareCam` と `Monkey_Killshot_*` の LevelSequence。`_sequences/` には書き出されていない）、`pak_reference/_bytecode/DDeception/Content/01_Hotel.txt`、WebGL 版 15 記録（`jumpscare.ts`）。
- 依存: 9。
- 規模: 1
- 状態: **完了（2026-09-19）**（有人セッションで直した）。本家ホテルの Matinee 3 本のカメラ（`JumpscareCam` の `InterpTrackMove_2` の位置と回転のキー。「`Monkey_Killshot_*` の LevelSequence」は読み違いで、カメラは Matinee にある）を保存された接線のまま評価して別室のカメラを動かし、寄りをワサミの上半分の写し方（前 111・上 177 cm。前は全身で前 222）にした。場面の時間を 2.5 倍から 0.4 s で等速へ戻し（クリップ・カメラ・暗転がそれに乗る）、4 本目に館のゴールドウォッチャー `BP_03_Watcher` の捕獲の体で、走って寄るワサミの顔をカメラアニメ `03_Watcher_Kill3` で間近に写し、1.15 s で黒に切って死亡画面にした（背景は黒の別室）。顔が髪の陰で暗いので顔の灯を足した。PIE で 4 本を 60 fps で収録し、どれも寄った位置から動いて暗転し、死亡画面 → 開き直しにつながるのを見た（実装記録 07 の「捕獲の演出」）。**完了の条件の読み替え**（本家のコードに合わせた）: (1) 回転のキーをそのまま当てると、動くワサミ（後転・滑り・歩き）が寄りの画面から外れるので、ワサミの首を目で追う向きを重ねた。(2) `MonkeyJumpscare3` のカメラは止まって待つだけなので、その間に `MonkeyJumpscare` の揺れを伸ばして入れた。(3) 組を入れ替えた（床から見上げる `MonkeyJumpscare2` を正面を向いて歩く `Capture_3` に）。(4) 滑りと歩きは遠い出だしを飛ばした。いずれも仮の値で、項目 28 の後回しの一覧に書いた。FOV のトラックはキーが無い（90 のまま）。

### 25. ゲームの途中の場面（Zone 1 の出来事、Zone 2 の捕まる場面と独房）

- 目標: 本家のゲームの途中の場面を、ナースの演技を v3 の動きで代用して作る（2026-09-18 の回答「場面の演技」）。Zone 1 の途中の出来事 `06_Hospital_Zone1_06Event`、Zone 2 の到着の後の捕まる場面 `06_Hospital_Zone2_Capture` と独房の場面 `06_Hospital_Zone2_Cell`。
- 完了の条件: レベル BP どおりのきっかけ（`Arrive_CaptureCutscene`・`Cell Cutscene Start` ほか）で、`_sequences/` のカメラと役者のトラックを写したシーケンスが流れ、終わるとプレイヤーが本家と同じ位置と状態で動ける（項目 6 で飛ばした Zone 2 の始まりが本家どおりになる）。場面のナースの演技は v3 の動きで代用する（`.claude/references/enemy-wasami-motions.md` の「場面の代用」）。PIE で 3 つの場面を収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital_Zone_01.txt`・`06_Hospital_Zone_02.txt`、`_sequences/06_Hospital_Zone1_06Event.json`・`06_Hospital_Zone2_Capture.json`・`06_Hospital_Zone2_Cell.json`。
- 依存: 6、7。
- 規模: 2
- 状態: **完了（2026-09-20）**。場面は取り込み `dd_sequence` にトラック（骨のアニメ・可視・揺れ・スローモーション・部品の材質）を足して本家の LevelSequence として組み直し（CameraAnim は UE 5.8 に無いので落とす）、ナース 4 体は `AWasamiCutsceneNurse` で置いて本家のアニメを敵ワサミ v3 のクリップに読み替えた。流すのはゾーンの流れの `PlayCutscene`（視点をシネカメラへ 0.5 s で移し、スキップの画面 `UWasamiCutsceneWidget` を出して `OnFinished` を結ぶ）。Zone 1 は `05_ParkingLotCutscene` → `06_Hospital_Zone1_06Event`（10.53 s）→ `Transition06`、Zone 2 は本家の配布版の道（チェックポイント 7 で `PlayerStart_1` → 救急車の到着 6.77 s〈場面ではないので視点も入力もそのまま〉→ `Trigger_Arrive_CaptureScene` → 捕まる場面 26.23 s → 1 s → 独房の場面 74.07 s → `PlayerStart_Cell` で動けるようになり棘へ）。チェックポイント 7 で開き直すたびに 3 つを流し直す。P で飛ばせる（`SetPlaybackPosition(1e7, Jump)` → `OnFinished`）。PIE で 3 つを通して確かめた（実装記録 11 の「場面の通し」・01・09）。後回し 5 件（黒帯・ナースの演技・消えるときの材質・シネカメラの画角・`CameraAnim_Nurse_01`）は項目 28 の一覧にある。

### 26. 追跡中のランダムの動き

- 目標: 2026-09-18 のユーザーの指示（上の「決めたこと」の「追跡中のランダムの動き」）。
- 完了の条件: 追跡中は約 8 秒に 1 回、v3 の追いかける動き 6 本からランダムに流す（前方が空いているときだけ、速さは追跡の速さ〈本作は 430 cm/s。2026-09-19 のユーザーの指示。`.claude/references/enemy-wasami-motions.md` の「追跡中のランダムの動き」の 800 cm/s は本家のナースの値〉のまま。頻度と早回しの上限はテストにする）。PIE で追跡中に流れることを収録。
- 根拠: `.claude/references/enemy-wasami-motions.md`、実装記録 07。
- 依存: 7。
- 規模: 1
- 状態: **完了（2026-09-20）**。追跡している間、6〜10 s（平均 8 s）の乱数の間隔で機会が来て、6 本（`Chase_PickUp`・`Chase_Charge`・`Chase_VaultRoll`・`Chase_VaultLand`・`Chase_RunFast`・`Chase_Slide`）から直前と違う 1 本を等確率で引き、走り（430 cm/s のまま）の上に 1 回再生で重ねる（`FWasamiChaseVariations`・`AWasamiEnemy::UpdateChaseVariation`）。早回しは走りと同じ 0.6〜1.8 で挟み、前方の空きは体が進む距離（2.9〜6.3 m）を NavMesh のレイで見る。テスト 3 件を足して 148 件すべて成功。PIE で 4 回の追跡（計約 6 分）を収録し、間隔 6.49〜10.01 s（平均 8.02 s）で 6 本が 35 回流れ、壁へのめり込みが無いことを確かめた（実装記録 07）。

### 19. 曲と環境音・効果音の残り

- 目標: 病院の曲（通常・追跡・Nightmare）と環境音、まだ無い効果音を本家どおりに鳴らす。
- 完了の条件: `BP_06_MusicPlayer`（Zone 1）・`BP_06_MusicPlayer_Zone2` の切り替えとフェードがコードどおり。レベルの `AudioComponent`（Zone 1 43・Zone 2 146）と `AmbientSound`・`AudioVolume` を配置どおりに置く。減衰は SoundCue と減衰設定の値どおり。捕獲の別室（項目 9 の `AWasamiCapture`。いまは無音）の音: 本家ホテルの捕獲の Matinee の `InterpTrackSound`（`Evil_Monkey_Scream`・`EN01_Toy_Monkey_Attck_Knife_v1`/`v4`。`pak_reference/_levels/01_Hotel.full.json` の `MonkeyJumpscare*` の `InterpData`）を時刻どおりに鳴らす。サルの声なので、WebGL 版（04 記録の `caught`: `enemy_scream` = `Evil_Monkey_Scream` を 1.0 で 1 回、13 記録）のように叫びだけにするかは、この項目で WebGL 版に倣って決める。捕獲の間は音を聞く位置が別室のカメラへ移るので、2D か別室の中で鳴らす（実装記録 07 の「既知の制約」）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/08_BearHouse/BP_08_MusicPlayer.txt`（`BP_06_MusicPlayer`・`_Zone2` の親。派生は曲を差し替えるだけで処理を持たないので、書き出しに `.txt` は無く `_assets/…/BP_06_MusicPlayer*.json` に音がある）、`_soundcues.json`、`_levels/06_Hospital*.full.json`、実装記録 01（`UWasamiSoundCueLibrary`）。
- 依存: 6。
- 規模: 2
- 状態: **完了（2026-09-20）**。曲（本家の `BP_08_MusicPlayer` を写した `AWasamiMusicPlayer`・`AWasamiMusicPlayerZone2`。0.5 s ごとの `Update` が `bFadeOut`・`bOverrideMusic`・敵の追跡で 1 s のクロスフェード）を両ゾーンに置き、ゾーンの流れが 8 か所で切り替えるようにした。環境音 2 つ・館内放送 1 つ・残響のボリューム 2 つを本家の値のまま置き（`AAmbientSound`・`AAudioVolume`）、捕獲の別室に本家の音（ホテルは t = 0 の `Evil_Monkey_Scream`、顔は 0.2 s の `LIVING_STATUE_Laughter_05` と 1.05 s の `Axe_Hit_03` を 2D で）を足し、残りの効果音を洗い出して見つかった 1 つ（敵の移動音 `DD_Rollerskating_Fast_V1_LOOP`。速さが音量とピッチになる）を埋めた（実装記録 10・07・11・01）。テストは `Wasami.Music.*`・`Wasami.Capture.Sound`・`Wasami.Enemy.Actor.Sound` ほか 152 件すべて成功。PIE の Zone 1 で、曲（無音 → 通常 → 追跡の Panic）・環境音と残響・捕獲の 3 本・敵の移動音（巡回 200 cm/s で音量 0.5、追跡 430 cm/s で 1.0・ピッチ 1.2225）が鳴ることを確かめた。**完了の条件の読み替え**: (1) 「通常・追跡・Nightmare」は通常・追跡の 2 種（本家の病院に Nightmare の曲は無く、`Override Music` も音が空）。(2) `AudioComponent`（Zone 1 43・Zone 2 146）のほとんどは置いた BP の部品で実装済みだったので、新しく置いたのは Zone 1 の環境音 2・残響 2 と Zone 2 の館内放送 1。(3) 捕獲は WebGL 版に倣って叫びだけにし、ナイフの刺突と重ねる小さい叫びは落とした（本作のワサミのクリップは刺突をせず、跳び込むのも 1 体）。(4) 館内放送と放送の箱 `04_Intercom` は置くだけ（鳴らす中身は台詞なので項目 20）。残った要確認 3 件は下の「未回答の要確認」。

### 20. 台詞と字幕（Bierce の台詞、WebGL 版のワサミの声）

- 目標: Bierce の台詞（`BierceTalk_Blueprint`）を本家どおり鳴らして字幕を出し、WebGL 版のワサミの声（16 本 + 字幕の `manifest.json`。巡回・発見・タイトルなど）を敵とタイトルに付ける。
- 完了の条件: 各レベル BP の `Talk` の呼び出しがそろい、字幕は本家の `_localization.json` / `_strings.json` の文言。ワサミの声は WebGL 版 06・15 記録の鳴らし方（バス・音量・字幕の秒数・場面）どおり。原本は `<WEBGL>/voices/`（wav 55）と `<WEBGL>/public/voices/`（mp3 16 + manifest）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/**/BierceTalk_Blueprint.txt`、`_audio.json`（`Audio/Dialogue/Bierce/Ch06/TT/*`）、WebGL 版 06・15 記録。
- 依存: 7、17。
- 規模: 2
- 状態: **完了（2026-09-20）**。Bierce の波 15 本と一言の Cue を `/Game/DD/Audio/Dialogue/Bierce/Ch06/TT` に取り込み（字幕は本家の文字列表 `Strings` の文言を名前の対応で `USoundWave.Subtitles` に入れた）、話し役 `AWasamiBierceTalk`（本家の `BierceTalk_Blueprint`）を両ゾーンに置き、Zone 1 の 3 か所（扉の破壊・館内放送・発見の一言）と Zone 2 の 8 か所 + Matron の裏の放送を本家の秒数どおりに鳴らす。ワサミの声は WebGL 版が鳴らす 11 本を `/Game/Wasami/Voices` に取り込み、`WasamiVoice` の口から場面の 6 本（挨拶・初シャード・ブースト・秘密の壁・死亡画面の 2 本）と敵の 5 本（発見・巡回の呼びかけ 4 本）を鳴らす。実装記録 07・10・11。

### 21. 仕上げ（通しプレイ・性能・パッケージ）

- 目標: タイトルから脱出まで通しで遊べることを確かめ、性能（この PC で 1080p・60 fps 前後、VRAM 6 GB 以内）を整え、パッケージする。
- 完了の条件: 通しプレイの収録（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）で止まる箇所が無い。両ゾーンの性能の計測が `.claude/guides/performance.md` の目安に収まる。パッケージは**配布の話なので先にユーザーに確認する**（`.claude/guides/distribution.md`。無人モードでは飛ばして要確認に書く）。
- 根拠: `.claude/guides/performance.md`・`distribution.md`、実装記録 00。
- 依存: 大目標 1 と、大目標 2 のほかの項目すべて（2026-09-18 に大目標に分けるまでは 1〜20・22・23 だった。項目 23 は大目標 3 へ移った）。
- 規模: 3
- 状態: **完了（2026-09-20）**。台本 `Tools/playthrough.py` で頭から通して **13 区間すべてが終了コード 0** で通り（タイトルの NEW GAME → Zone 1 → 救急車 → Zone 2 の到着と独房 → 見張りの廊下 → 迷路 → 祭壇 → ポータル → スコア画面 → タイトルまで 408 s。ゲーム内の時間 5:25・SOUL SHARDS 679・LIVES LOST 1・FINAL RANK A）、**止まる箇所は無い**。下見で見つかった 1 か所（チェックポイント 7 が救急車の屋根で開くのに、台本が独房から始まる前提だった）は台本に区間 `z2_arrive`（中庭を歩いて捕まる場面へ）を足して直した（`ArriveEvent` は本家どおりなのでゲーム側は直していない。01 記録）。性能は道具 `Tools/perf_probe.py` を作って両ゾーンの代表 7 か所を 1080p 相当・Epic で測った（00 記録の「性能」）: **メモリは目安内**（GPU メモリ 2.9〜4.1 GB、カード全体 4.0〜5.3 GB / 6 GB、エディタの常駐 RAM 3.3 GB）、**fps は 46〜60 で目安の「60 前後」にわずかに届かない**（どこも GPU 律速で、GameThread は 9.5〜11.3 ms と余裕がある）。**完了の条件の読み替え**: (1) 性能は、エディタにだけ効く対処では製品の fps が動かず、製品側を落とすのは品質の決まりに反するので**対処を入れず**、選択肢（このまま / クックして本編を測る / 製品に画質の選択肢）を下の「未回答の要確認」に書いてユーザーの判断待ちにした。(2) パッケージは**クックとビルドを行わない**（配布の話なので無人モードでは飛ばす決まり）。下ごしらえの確認だけ行い、既定のマップとゲームモード・`/Game` の外を指す参照・原作のロゴとキャラクターのモデルの 3 つとも問題なしだった（ついでに道具のプラグイン 3 つを Editor ターゲット限定にし、パッケージ設定を空のままにする理由を `DefaultGame.ini` に書いた。00 記録）。使っていない原作の章の題字が `/Game` に残っていることは要確認。

## 大目標 3: 本家に忠実に

- 状態: **達成（2026-09-21）**（2026-09-20 から。この節の項目 23・28・32・33・34・35・36 がすべて完了）。**最初の 3 つの大目標はこれで全部達成**なので、無人運転はここで止まった（`.claude/references/handover.md` の「現状と次の一歩」）。その後、2026-09-22 の有人セッションで**大目標 4（レビュー指摘の修正）**を足した
- 始め方: 自動（大目標 2 を達成したら、無人運転がそのまま「進行中」にして続ける。2026-09-20 のユーザーの指示「大目標2が完了後、自動で3に移るように」）
- 達成の姿: 大目標 1・2 で後に回した見た目と演出が、本家の実機の観察と原作の式で本家どおりになる。
- 決め方: 今までの決め方（`.claude/guides/original-fidelity.md`。本家の観察・収録の見比べ・原作のコンパイル済みシェーダーの式の写し）。
- **後回しの一覧の見直しは Claude が行う**（2026-09-20 から。それまでは始める前に有人セッションでユーザーと見直すことにしていた）: 項目 28 を取った最初の反復（計画だけの反復）で一覧の各行を見て、大きいもの（本家の場面を撮って材質か動きを詰める、規模 2 以上の目安）は項目に立て直してこの節の項目 28 の後に足し（規模と依存を書く。行は一覧から新しい項目へ移す）、残りを項目 28 のステップにする。本家どおりにするか今のままにするかがユーザーの好みで分かれそうな行は、本家どおりを仮に採って要確認に書く。有人セッションで立て直しを直されたら従う。**2026-09-20 に実施済み**: 21 行のうち大きい 7 行を項目 32（捕獲の別室）・33（破壊と粒子）・34（ポータルと特殊シャード）に立て直し、残る 14 行を項目 28 のステップにした。**2026-09-21 の有人セッションで追認**（行の分け方はそのまま、28 に残る大きい行〈Matron・祭壇・場面の 4 行〉も立て直さない）。規模だけ配り直した: 28 = 3 → **4**、32 = 2 → **1**、33・34 = 各 2 のまま（合計 9 は不変）。
- **この節の項目は原作のコード・アセットで先に決める**（2026-09-21 のユーザーの回答）: 本家の実機は中の移動が遅すぎて（歩きでもテレポートでも 1 回 6〜10 s・向きが定まらず、69 m 先の祭壇に 7 回跳んで届かなかった）、1 回の収録で 1 か所しか回れない。だから各項目は原作のブループリント・アセットの値・cook の `ResourceData`・コンパイル済みシェーダーの式で決め、**それでも決まらなかったものだけ**を 1 回の収録にまとめる（項目 28 のステップ 16 のやり方）。項目 28 のステップ 6・7 はこのやり方で実際に閉じられた。
- 達成の条件: この節の項目がすべて完了。

### 23. パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）

- 目標: 2026-09-17 のユーザーの回答。テレキネシスの球の粒子の灯を本家の見え方に合わせて弱め、テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める。
- 完了の条件: (1) 本家と同じ条件（Lv4・画質「高」・速さ 0.25）で、力場の間の画面の平均が本家 (95〜99, 141〜147, 186〜192) に近づく（今の本作は (123〜127, 174〜176, 216〜218)。線形の明るさで、本家の灯の寄与は本作の約 0.5〜0.7 倍。灯を切ると本家よりずっと暗い）。灯は `P_ky_forceField_Telekinesis` の `sphere` の `ParticleModuleLight`（原作の `BrightnessOverLife` 5）。弱める値は、原作から離れる本作の調整としてコードと実装記録 04 に書く。(2) テレキネシスの力場の 4 つの材質（`M_DD_KyWall02`・`M_DD_KyAura7`・`M_DD_KyShockWave02`・`M_DD_KyStarDust`）と Telepathy の印（`M_DD_Telepathy`）が、推定と仮の値（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`TELEPATHY_PAN_*`・`TELEPATHY_GAIN` の `TODO(仮)`）ではなく、原作のコンパイル済みシェーダーから読んだ式そのものになる（グラフは cook で消えるが、シェーダーマップは残る。`Tools/dd/cooked_shaders.py`。UI の材質も自分のシェーダーマップを持つ）。式を写したうえで、本家の実機の無劣化の連写（力場のオーラと地面の輪・球の幕・終わりの破片、近くのナースの印の雲と縁のこぶ）と見比べて確かめる。撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。
- 根拠: 実装記録 04 の「既知の制約・注意点」、`observations/README.md`（パワーの作業のステップ 11b1・11b4・11b5）、`.claude/references/powers/03-telekinesis-vanish.md`・`04-primal-telepathy.md`。
- 依存: 1。
- 規模: 2
- 状態: **完了（2026-09-20）**（2026-09-18 に大目標 3 へ移して保留にし、大目標 3 が進行中になった 2026-09-20 に戻して終えた）。(1) テレキネシスの球の灯は `P_WasamiForceField` で `BrightnessOverLife` を **0.35 倍**にし、本家と同じ条件（Lv4・画質「高」・速さ 0.25）の画面の平均が (125.0, 175.2, 217.5) から **(95.1, 148.7, 193.0)** になって本家 (97.0, 144.4, 190.1) のこま間のばらつきに収まった。(2) 推定と仮の値をやめ、**5 つの材質をすべて原作のコンパイル済みシェーダーの式にした**: 力場の星屑・幕・オーラ・地面の輪（`AURA_LAYERS`・`SHOCKWAVE_PANS` は消えた。本家の終わりの白飛びした横長の破片は、幕でも星屑でもなくオーラの `T_ky_maskRGB5` の G の欠片だと分かり、幕の筋は Screen で白っぽく、星屑はテクスチャを読まない小さなひし形だった。球が縮む間の幕の流れる速さは、球の幾何・回転の乱数・カメラの揺れで決まるので対象から外した〈2026-09-18 のユーザーの回答〉）と Telepathy の印（`TELEPATHY_GAIN`・`TELEPATHY_UV_TILING` は消え、パンは V だけの 0.5・0.4、Opacity は 2 ×（勾配 − noise の G）×（noise16 の R × noise の R × 10 + 0.05）の saturate。勾配の密度は UE 4.24 の既定 1 を呼び出しに入れる）。確かめは、止めた動的インスタンスを描画先に描いて同じ式の numpy と画素ごとに平均 0.0009 で一致したことと、本家の無劣化の連写との大きさに依らない統計（細かさ・面積比・中央値・縁の凹凸）が重なったこと、PIE の実際の見え方（`observations/README.md` の 5a〜5g と「Telepathy の印を原作の式にした」、実装記録 04・01）。

### 28. 後回しにした見た目と演出を本家どおりに詰める

- 目標: 大目標 1・2 で「見た目の詰めをしない」決め方のために後に回したものを、本家の実機の観察と原作の式で詰める。
- 完了の条件: 下の「後回しの一覧」の各行が、本家どおりになったか、ユーザーがこのままでよいとした（本家どおりにできない行〈本家でその場面が出せない・本作の体では写せない〉は、理由を要確認に書いて閉じてよい）。一覧の見直しは 2026-09-20 の計画の反復で済ませ、大きい 7 行は項目 32・33・34 に立て直した（上の大目標 3 の節の頭。有人セッションで直されたら従う）。
- 後回しの一覧（大目標 1・2 の作業が 1 件 1 行で足す: `- <日付>（項目 N）: <何を> — 今は <何にした>。本家での確かめ方: <実機の場面・原作のアセット>`。2026-09-20 に 21 行のうち大きい 7 行を項目 32・33・34 へ移し、残る 14 行をこの項目のステップにした）:
  - 2026-09-20（項目 25）: 場面の上下の黒帯 — **閉じた（2026-09-21、ステップ 5）。本家の式で帯を出すようにした**: `MM_CutsceneBars` は cook でグラフが消えているので、コンパイル済みシェーダー（`cooked_shaders.py "MM_CutsceneBars."`）から式を読んだ — 帯の高さは `Mat_ParameterCol` の `Cutscene Bars` × **0.128205**（16:9 を 2.39:1 に切ったときの帯。画面の縦横比には依らない）で、上下とも `smoothstep(saturate(…))` を 50000 乗して境目を 1 画素に潰し、中は `PostProcessInput0`・帯は `BorderTexture`（`T_Black`）を選ぶ。本家のインスタンスの `Size`（0.2）・`VerticalBoxing`（0）はシェーダーのユニフォームに無い = グラフに繋がっていないので写していない。`dd_ui.make_cutscene_bars()` が材質 `/Game/DD/Materials/Special/PP/MM_CutsceneBars` とインスタンスと黒 `T_Black` を作り、本作はプレイヤーコントローラーの派生を持たないので `AWasamiPlayerCharacter` が `BeginPlay` で無限（`bUnbound`）のポストプロセスボリュームを作って付ける（本家は `DD_PlayerController` の `PostProcess` が重み 1 で持つ。範囲なしなのでどちらも同じ）。PIE の Zone 1 で `Cutscene Bars` を 1 にして `HighResShot 3440x1440` を撮ると上 184 行・下 184 行（高さの 12.78 %、境目は 1 行）で、本家の収録（上 184・下 183 = 12.78 / 12.71 %、にじみ無し）と一致する（実装記録 09・02・01）
  - 2026-09-18（項目 5）: 死亡画面のアニメの区間の `RestoreState`（Fade In・Death の終わりで元の値に戻すか）— **閉じた（2026-09-21、ステップ 3・6）。本家どおりで、直しは無し**: 本家（最新版）の `YOU ARE DEAD` は出てから 20 s 以上（22 枚）1 枚も変わらず、ボタン（再開する / 最後のチェックポイントへ / タイトルへ）を押すまで残る = アニメは終わりの値のまま（`RestoreState` を掛けない）で、本作の作りと同じ。同じ行の「シャードを拾った後の数の位置（12 px 下に残るか）」も**残らない**（白い字の外接矩形は、回収の前が y 34..151・x 64..328、揺れの最中が x 70..334〈右へ 6 px・上へ 1 px〉、1 s 後から 18 s 後まで y 34..152・x 64..329 で戻っていた）= 本作のタブレットの Count Shake（戻す）と同じ。ステップ 3 で出た要確認も原作のコードで閉じた: `YOU ARE DEAD` は残機 0 の死亡でだけ出て、残機が残れば同じウィジェットが「残機残り：」のまま自動でレベルを開き直す（「残機残り：」＋ドクロ＋英語のヒントの画面は死亡画面ではなくレベルの読み込みの画面。本作の項目 5 の作りも既にこのとおり。実装記録 09・06）
  - 2026-09-18（項目 5）: 死亡画面のボタンとヒントの書体 — **閉じた（2026-09-21、ステップ 6）。本家と同じだと確かめ、予備の書体だけ原作どおりに直した**: ボタンの `helvetica-normal` と見出しの `helvetica-neue-bold` は、取り込んだ FontFace の中身が pak の ttf とバイトまで同じ。ヒントのエンジンの `RobotoTiny` は面も合成フォントも UE 4.24 と UE 5.8 で同じ。大きさ（30・50・24）も本家のウィジェットどおりで、PIE と本家の収録の同じ文の字送りの幅が UI の拡縮の比まで一致した（2.2031 対 2.2040）。直したのは予備の書体だけ（`dd_assets.font` が原作と同じ `DroidSansFallback` を合成フォントに入れるようにし、2 つの Font を作り直した）。面を Default の書体に置く作りは変えない（本作の文は機械の言語に関わらず英語なので、英語の本家と同じ見え方を採る。実装記録 09・01）
  - 2026-09-18（項目 6）: 救急車が走り出して 2〜3 s の間、トンネルの床の前方が黒い矩形で欠けて見える — **読み違いを正した（2026-09-21、ステップ 15）。床は欠けておらず、黒い矩形は乗っている救急車そのもの**: 走り出して約 1.5 s でトンネル口の灯 4 つ（`PointLight353`・`354`・`355`・`357`。Movable・20/30 cd・減衰 2000・`VolumetricScatteringIntensity` 10）の届く範囲を出ると、救急車にはもう光が当たらない（トンネルの灯 `PointLight351_Tunnel_Blueprint` は 40 cd・減衰 1500 を天井の両脇 z 883 に置くので屋根では 0.4 lx ほど）。先の路面だけが見えるのは、その灯の `VolumetricScatteringIntensity` 20 がカメラと路面の間のフォグを光らせるからで、目の前の屋根にはその間のフォグが無い。**原作の灯の値はどれも本作と同じ**（BP `PointLight351_Tunnel_Blueprint` と 4 つの灯の書き出し）で、トンネルの床にも原作は何も置いていない。PIE で屋根の画面の明るさが走り出しの 1.0 → 2.4 s に 84 → 34 → 0.3 と落ちるのを測り、Static / Movable で絵が 1 画素も変わらないことも確かめた（進捗記録の 2026-09-21 版）。**本家でも同じだけ暗いことを確かめた（2026-09-21、ステップ 16）。この行は閉じる**: 走り出しそのものは撮れなかった（ナースのいる PARKING LOT へ降りられなかった）ので、**同じ灯・同じ高さの Zone 2 の到着のトンネル**で代えた。救急車の屋根の高さ（路面から 462 cm、灯は 884〜902 cm 上）から撮った 1 枚で、**カメラの近くの面は屋根の高さでも路面でも暗く**（天井 40.4・路面 36.9）、**奥の路面だけが散乱で明るい**（60.0）。灯そのものは 191.9、灯の脇の壁は 92.5（`observations/README.md` の「Zone 2 の到着のガレージとトンネル」）。灯のインスタンスは Zone 1 も Zone 2 も値を 1 つも上書きしていないので、BP の既定が両方に効いている
  - 2026-09-18（項目 6）: Zone 2 のリフトの乗り方（実装記録 12 の「既知の制約」）— **閉じた（2026-09-21、ステップ 10）。本家の値からそのまま出る動きなので、このままにする**: 角のリフトの上で床が沈んでは戻る量（75.6 cm）と周期（1 + 75.6/267.5 + 75.6/535 = 1.42 s）は、本家の `LiftCollisionOverlap`（箱の既定 32 × 親の拡縮 = 床と同じ広さ・床の面から 75.6 cm 上まで）と床の速さから計算どおりに出る（本作の実測 約 70 cm・約 1.4 s）。長い床に歩いて近づくと 50/300 = 0.167 s で 44.6 cm の段差ができるのも、本家のカプセル半径 50・歩き 300・床 267.5 cm/s の値そのまま（`MaxStepHeight` 45 の際で、走れば 22.3 cm）。実機の収録は要らないと判断した（実装記録 12）
  - 2026-09-19（項目 7）: Zone 2 の見張りの視界コーンの地図の印（扇 `map_enemy_search_Mat` と点 `0_DotCircle_Mat`）の見え方 — **閉じた（2026-09-21、ステップ 9）。本家どおり地図に出さない**: 本家のキャプチャは `Translucency` が偽で、両方の材質が Unlit・半透明だから 1 枚も描かれない（ステップ 7 で原作のコードから確かめた）。2026-09-21 のユーザーの回答「消す」に従い、コーンからタグ `dd_minimap` を外し（レベルに焼かれていた Zone 2 の Matron のコーン 2 つからも外した）、地図に写すための Default Lit・Masked の推定をやめて材質を本家の Unlit・半透明に戻した。PIE でキャプチャの `ShowOnlyActors` にコーンが 1 つも入らないことを確かめた（実装記録 07・03・01）
  - 2026-09-19（項目 7）: 見つける前に Primal Fear などで気絶した見張りの姿勢 — **閉じた（2026-09-21、ステップ 10）。本家どおり、明けずに倒れたままにした**: 本家のナースの ABP は `bStunned`（State == Stun）の間ずっと `nurse_stunned`（2.6333 s）をループし、起き上がりのクリップは無い。見張りは見つけるまで判断が動かないので State が Stun のまま終わらず、本家の収録 `orig-sentry-stun2.mkv` でも Primal Fear が効いた後 19.8 s まで明けなかった（閃光の後の周期が 2.62 s。以前「効かなかった」と書いたのは読み違い）。本作も見張りの `GetTimeToStunStart` を `IndefiniteStunSeconds` にして、見つけるまで倒れた姿勢を保ち、見つけたら 17 s を読み直して起き上がるようにした。倒れる・起き上がるアニメそのものは 2026-09-18 のユーザーの指示のまま（本家の立ったままの気絶には合わせない）
  - 2026-09-19（項目 11）: Matron（ボスワサミ）の大きさと動きの見え方 — **閉じた（2026-09-21、ステップ 11）。大きさを本家の待機の姿勢に合わせ、動きは本家の値どおりだと確かめた**: 本家の Matron は待機で机へ身を乗り出すので、見える高さは基準姿勢の 227.60 cm でなく 171.54 cm（`Tools/dd/psa_pose.py` で `SK_Matron.psk` を `DD_Matron_Zone_02_Idle.psa` の姿勢でスキニングして測った。のこぎりは本家が透明な材質で消している）。レベルの拡縮 5・原点 −107.22 cm と合わせると、本家の頭の上は床から **750.50 cm**（1031 cm は基準姿勢を測った誤り）。ボスワサミの `A_WasamiBoss_Idle` の 166.39 cm を同じ高さに合わせて拡縮を **6.111 → 5.1547** にした（2 割小さくなった）。待機 ↔ 警戒の 0.95 s と HermiteCubic、`spine_02` の LookAt（軸・制限 90°・入り 0.5 s Cubic・出 0 s）は ABP の値そのものなので今のままでよく、代えているのは原本に無い切り替えのクリップだけ。PIE（Zone 2）で机の後ろの見え方と頭の骨の高さ（警戒で 521 cm、計算どおり）を確かめた（実装記録 17・01）
  - 2026-09-20（項目 12）: 秘密の部屋のグリッチ（本家 Chameleon の `Glitch - Advanced`、材質 `M_GlitchHLSL`）と書類の材質の縁の光 — **閉じた（2026-09-21、ステップ 12）。どちらも本家どおりだと原作のシェーダーと値で確かめ、直さなかった**: グリッチは `M_GlitchHLSL` のコンパイル済みの画素シェーダー 293 行と命令ごとに突き合わせ、写した 4 つの枝（行の乱れ・RGB のずれ・行ずらし・格子のずれ）が同じだと確かめた。写していない枝は Chameleon の `Glitch - Advanced`（`BlendMode` 0・マスク `T_base_white_d` を拡縮 1 × 1・`BlendDistance` 0・`CustomDepth` と `StencilBuffer` 偽）ではすべて素通りになる。**距離の混ぜが素通りなのは `BlendDistance` 0 のためだけではなく**、`Set Advanced Effect Features` が `BlendDistanceLog` に `(BlendDistanceInvert → 0 − 0.5) × 2 = −1` を入れるため（材質の既定 +1 のままだと `1 − saturate(pow(深さ ÷ 0, 10))` = 0 で、グリッチが 1 画素も出ない）。値も `Glitch Func` が入れる 6 つ（`Amount` 0.5・`Speed` 10・`Density` 30・`GridDistortion` の 3 つ）だけで、残りは材質の既定のまま。書類は本家の `MM_Shared_Secret_Folder` が持つ 4 つの値（`Fresnel Setting` (1, 1, 1, 1)・`Albedo` と `Packed` が `secret_file_01_D`・`Normal` が `flat_N`）がそのまま入っていて、親の発光も本家のベースパスの画素シェーダーと同じ Fresnel（指数 5・基底の反射率 0.04・写した法線）× `Fresnel Setting` の RGB。PIE の秘密の部屋でグリッチの入り切りと書類の白い縁を撮って確かめた（実装記録 18・01）。残る違いは、ずらしと行の番号を場面の絵の UV でなくビューポートの UV で読むことだけで、ビューが場面の絵を満たすとき（パッケージの全画面）は同じになる
  - 2026-09-20（項目 31）: 祭壇 `ring_statue` の真鍮の金属・欠片の紫の縁の光・祭壇の球の紫の渦の光の見え方 — **閉じた（2026-09-21、ステップ 13）。球の推定に残っていた枝を本家のシェーダーどおりに入れ、金属と欠片は既に本家どおりだと確かめた**: **球** `m_crystal` は、反射と屈折の両方を「`distortion_normal` を UV × 0.1 で読んで接空間からワールドへ移した法線 + 頂点法線」を軸に取っていた（**正規化しない**）。この `distortion_normal` は親も 3 つのインスタンスも `/Engine/EditorShapes/Textures/T_ShapeNormal` で、255 分の 1 の揺らぎしかない平らな法線だが、**平らでも軸は頂点法線の 2 倍**になり、`reflect`・`refract` の結果が単位法線のときと別物になる（正面では反射が 7 倍の長さ）ので省けない枝だった。`M_DD_Crystal` に入れて `CRYSTAL_LEFT_OUT` を空にした（推定で外したものは無くなった）。`T_ShapeNormal` は本家も pak に cook して入れているので `/Engine/` のパスのまま使う。**金属**（`MM_00_Ballroom_Ring_Altar_Metal`: `Roughness` 0.35・`Hover Intensity` 0・`Hover Color` (0.3448, 0, 0.75)・`Normal` = `RingStatue_N`、`Albedo` は入れず親の真鍮の定数）と**欠片**（`M_ring_metal2`: `Roughness Power` 1・`BaseReflectFractionIn` 0.2463・`Fresnel ExponentIn` 5.7948・`Fresnel Setting` (1.1556, 0, 5)・テクスチャ 3）は本家のインスタンスと 1 つずつ一致し、`ring_statue_2`・`ring_statue_orb_5` の拡縮も本家と同じ 4.0 だった。PIE（Zone 2 のガレージ）で球の渦と、球を消した後の欠片を撮って確かめた（実装記録 16・08・01）
  - 2026-09-20（項目 25）: 場面のナースの演技（本家の専用のアニメ 10 種）— **閉じた（2026-09-21、ステップ 14a）。尺は本家どおりで、1 回きりの演技だけ繰り返さないようにした**: 本家の 26 区間を本家のクリップの長さと突き合わせると、区間は「周期を繰り返すもの」（`ReaperNurse_Boss_Idle_01` 1.417 s を 5.933 s で 4.2 回、`nurse_idle_01` 3.933 s を 5.600 s で 1.4 回、`Fast_Jump_Up_Air` 0.433 s を 0.933 s で 2.2 回）と「1 回の演技を入れるもの」（殴る `Event_39` 1.633 s が区間とちょうど同じ、台詞の `Event_40`〜`46` は区間より長いので切る）の 2 つで、**区間が余る所の繰り返しは本家自身の作り**だった。本作の区間・`StartFrameOffset`・逆再生は本家の書き出しをそのまま写していて、折り返しの規則も UE 4.24（`Fmod`）と 5.8（`bLooping`）で同じ。ただし代用のクリップは長さが違うので、1 回の演技の区間が繰り返しになっていた（捕まる場面の殴りが `Chase_Charge` 0.533 s で 3 回突進）。**1 回きりの代用だけ区間を 1 回で満たす `play_rate` にした**（`ONE_SHOT_CLIPS`。殴りは 0.3265 倍の 1 回になり、PIE で頭の骨を 0.1 s ごとに追って山と谷が 1 つずつだと確かめた）。周期の代用（`Idle`・`Idle_Alert`・`Walk`・`Run`）は本家と同じ 1 のまま（実装記録 01）。**歩幅は合わせられない**: 本家のナースはローラースケートで滑り、後ずさり `ReaperNurse_Walk_Back` は足の振りが片足 29 cm／1.667 s しかないのに移動のトラックは 3.2 s で 290 cm（90.6 cm/s）動かす（`Tools/dd/psa_pose.py` で psa の骨を測った。psa の単位は cm の 16.667 倍）。滑る動きが敵ワサミに無いので、歩く代用では足の運びが合わない（2026-09-18 のユーザーの決定による代用の限界）。
  - 2026-09-20（項目 25）: 独房でナースが消えるときの材質の動き（`MovieSceneComponentMaterialTrack` の `Efficiency` 0 → 1）— **閉じた（2026-09-21、ステップ 14b）。本家の消える材質をワサミのマスターに入れ、本家どおり溶けて消えるようにした**: 本家の `M_06_Nurse_Body` は Basic Stealth System の `M_BSS_Character1` のインスタンスで、親も材質関数も cook で式が消えているので、コンパイル済みシェーダーから読み直した — ノイズ 2 枚を毎秒 (0.08, 0.07) と (0.02, −0.05) で流して `saturate(Fast.r × 3 − Slow.r)` を作り、それが `lerp(WorstEfficiency, BestEfficiency, Efficiency)` を上回る所だけ残す Masked のマスク（クリップ 0.3333）と、そこから `FringeSize` 0.1 の帯だけ `GlowColor` 赤 × `GlowIntensity` 25 で光らせる発光。**どちらのノイズを 3 倍するかはシェーダーに無い**が、2 枚とも sRGB なので逆に読むと `Efficiency` 0 で体が半分欠け 1 でも 6 割残る — `BSS_Noise2` を 3 倍する読みだけが成り立つ。`M_DD_WasamiGltf` に入れ（値は本家のインスタンスのものをマスターの既定にした）、代用のアニメ `nurse_cloak` → `Walk`（歩き去る）を `Idle`（その場で消える。本家の移動トラックはこの場面でナースを 1 cm も動かさない）に直した。PIE で材質トラックが本家の刻みどおり 23.867→24.333 s で 0→1、28.833→29.3 s で 1→0、73.433→73.8 s で 0→1 を動かすのを読み、`SceneCapture2D` で 5 段階を撮って 0 で穴なし・間は赤い縁・1 で完全に消えるのを確かめた（実装記録 07・01・17）。**遊びの中の敵ワサミは消えない**: 本家の `BP_06_ReaperNurse` は遊びの中でも `Cloak` で `Efficiency` を動かすが、本作はまだ場面のトラックしか動かさない（後回しの一覧には足さない。本作の敵の振る舞いは項目 7 で決めたもので、透明化は入っていない）。
  - 2026-09-20（項目 25）: 場面のシネカメラ（`06_CineCamera`・`CineCameraActor_2`）の画角 — **閉じた（2026-09-21、ステップ 14c）。「本家の置いたものは値を何も上書きしない」が読み違いで、上書きしていた**: 本家のレベルの `CineCameraComponent` はフィルムバック 36 × 20.25 mm・焦点距離 19.304428 / 17.929493 mm・`bConstrainAspectRatio` 偽（Zone 2 は絞り 10・焦点の平滑 4.0 も）を持ち、アクタは `LookatTrackingSettings` で `06_CameraTarget` / `cameralook` を追う。取り込みがそれを落としていたので、前処理に `camera`、`dd_sequence` に `_cine_camera`・`_look_at` を足して当てた。**水平の画角が 63.2° / 65.6° → 85.9947° / 90.2249°** になり、本家が焼いた `FieldOfView` と一致した（PIE で視点をカメラに移して測り、追従も効いていることを確かめた）。**UE 4.24 と 5.8 のクラスの既定は、この場面が使う範囲では同じ**（両方の `BaseEngine.ini` の Universal Zoom・焦点距離 35・f/2.8・焦点 Manual 100000。UE 5 の `ExposureMethod` も焦点が `DoNotOverride` でなければ効かない）ので、既定の差を埋める直しは要らなかった。実装記録 01。
  - 2026-09-20（項目 25）: 捕まる場面のカメラアニメ `CameraAnim_Nurse_01`（本家は 20.53〜25.27 s の `MovieSceneCameraAnimTrack` でシネカメラに重ねる）— **閉じた（2026-09-21、ステップ 14d・16）。移動だけを変換トラックに焼き、回転は入れていない（未回答の要確認 2）**: UE 5.8 に camera anim のトラックは無いので、本家の CameraAnim（Matinee の Move トラック）のキーを保存された接線のまま評価して、場面のシネカメラの変換トラックに 20.53〜25.27 s の区間で焼き足した（同じ区間の本家のカメラの揺れ `MovieSceneCameraShakeTrack` は前から入っている）。本家のアニメは**回転も持つ**（yaw +180 で後ろを向き、最後にロール −81 で横倒し）が、写しても `ACineCameraActor` の LookAt が毎フレーム回転を書き直すので見えない（本家も同じ作り。PIE で測った。実装記録 01 の「既知の制約」）。本家のティックの順が逆だった可能性は消せず、回転が効いた場合の絵はまるで違うので、どちらにするかは下の「未回答の要確認」の 2。**2026-09-21（ステップ 16）に本家で撮ろうとして届かなかった**（MOD の TUNNEL = チェックポイント 7 が開く到着のガレージから、捕まる場面のトリガーまで 148 m の道が見つからない。症状索引）
- 根拠: 各行に書く。
- 依存: 大目標 2。
- 規模: 4
- 状態: **完了（2026-09-21）**。一覧に残した 14 行を 1 行ずつ閉じた。**10 行を直した**（場面の上下の黒帯を本家の式で出す・死亡画面の予備の書体・見張りの視界コーンの地図の印を消す・気絶した見張りを倒れたままにする・Matron の拡縮 5.1547・祭壇の球の結晶の式・場面の 1 回きりの演技・独房でナースが消える材質・場面のシネカメラの画角・捕まる場面のカメラアニメの移動）。**4 行は本家どおりだと確かめて直さなかった**（死亡画面のアニメの `RestoreState`・Zone 2 のリフトの乗り方・秘密の部屋のグリッチと書類の縁の光・救急車の走り出しの黒い矩形〈「トンネルの床が欠ける」は読み違いで、黒い矩形は光の届く範囲を出た救急車そのもの〉）。**完了の条件の読み替え**: (1) 2026-09-21 に「原作のコード・アセットで先に決め、それでも決まらないものだけ収録する」（この節の頭）へ切り替えたので、本家の実機の収録で確かめたのは 5 行（黒帯・書体・気絶した見張りの姿勢・地図の印・トンネルの灯の暗さ。収録は 4 回、合わせて約 2 時間）で、残る 9 行は cook のコンパイル済みシェーダーと原作の値・アセットで閉じた。(2) **3 行はこのままでよいかをユーザーに聞く**（下の「未回答の要確認」の 3 件: Matron の大きさの合わせ方・捕まる場面のカメラアニメの回転・救急車の暗さを Zone 2 の到着のトンネルで代わりに確かめたこと）。(3) 観察の台本の A5（特殊シャードの出現と地図の印）・B1（扉の破片）・B2（独房の粒子）は本家で撮れなかった（A5 は死亡でレベルが読み直されて時計が戻る、B1・B2 は到着のガレージから捕まる場面のトリガーまで 148 m の道が見つからない。症状索引）が、3 つとも項目 34・33 の行で、どちらの項目も「コードとアセットで閉じる」決め方になっている。

### 32. 捕獲の別室の見た目と動きを本家の 6 本と見比べて詰める

- 目標: 大目標 2 の項目 9・24 で本家ホテル・館の Matinee のキーから組んだ捕獲の別室（カメラの寄り・灯・場面の時間・暗転）を、原作の Matinee とアセットの値どおりにする。
- 完了の条件（2026-09-21 にコード優先へ書き換え。節の頭を見る）: 原作の Matinee 6 本（`MonkeyJumpscare`・`2`・`3`・`03_Watcher_Kill*`）のキー（カメラの位置・FOV・DOF・揺れ・フェード）と灯のアクターの値を読み、本作の実装と 1 つずつ突き合わせて差を埋める。写していない `JumpscareCam` の旧 DOF のトラック（焦点 142.9 → 10・領域 571.4 → 100）と、直線にした暗転（本家は曲線の `StartCameraFade`）を入れるかどうかもキーから決める。**コードで決まらなかったものだけ**、旧版の実機を 1 回だけ撮って見比べる。本作の体（ワサミのモデルと骨）では写せない差は、理由を書いて閉じてよい。
- 今は（項目 28 の「後回しの一覧」から 2026-09-20 に移した行。本家での確かめ方も各行にある）:
  - 2026-09-19（項目 9・24）: 捕獲の別室の見た目と動き — 今は本家ホテルの 1 本目の Matinee の t=0 のカメラがサルの頭を写す写し方をワサミの上半分に当てた寄り（前 111・上 177 cm、FOV 90）から、Matinee のカメラのキーで動き、ワサミの首を目で追う向きを重ね（`AimSpeed` 8）、首の前 50 cm で突っ込みを止める。サルの真上の天井灯 `ceilinglights_80` を縮めた位置の灯（1500・半径 500・水色）と顔の灯（頭の前 50・上 30、300・半径 200）。組は `Capture_1`↔`MonkeyJumpscare`・`Capture_2`↔`MonkeyJumpscare3`（止まる区間に `MonkeyJumpscare` の揺れを 1.797 倍で入れた）・`Capture_3`↔`MonkeyJumpscare2`。場面の時間は 2.5 倍から 0.4 s で等速へ。滑りは 0.45 s、歩きは 1.2 s から始める。暗転は直線の `StartCameraFade`（本家は曲線）。`JumpscareCam` の旧 DOF（焦点 142.9 → 10・領域 571.4 → 100 のトラック）は写していない。顔の 4 本目（館の `BP_03_Watcher` の体）は、走りで 250 cm 後ろから 0.1 s の指数で寄り、カメラは前 121.5・上 186・FOV 75 から `03_Watcher_Kill3`、頭の前 40 cm で止め、ウォッチャーのカメラへの 0.15 s の寄せは切り替えにした。本家での確かめ方: 旧版の実機のホテルでサルに捕まる 3 本（`MonkeyJumpscare`・`2`・`3`）と館でゴールドウォッチャーに捕まる 3 本を撮り、寄り・明るさ・ぼけ・揺れの大きさ・暗転の速さを見比べる
- 根拠: 実装記録 07 の `AWasamiCapture`、`pak_reference/` の `MonkeyJumpscare*`・`03_Watcher_Kill*` の Matinee、下の「今は」の行。
- 依存: 大目標 2（項目 9・24）。
- 規模: 1
- 要確認（ユーザー。2026-09-21 に残した。答えが出たら直してこの行を消す）:
  - **顔の 4 本目の明るさ**: 原作の 2 灯（`jumpscarelight`・`_5`。強さ 15・届く距離 15 を頭の比 0.3347 で写して 1.680・5.02 cm）のままだと、前髪の陰でワサミの顔がほとんど読めない（PIE の顔の中央の輝度が 255 中 9〜19）。原作のサルは髪が無いので天井灯で顔が写る。**原作に無い明るさを足すか**（PIE で試した目安: 強さ 4 倍・距離 1.5 倍で 60〜69、10 倍・2 倍で 66〜121）、原作の値のままにするか。灯の位置は目のくぼみに正しく乗っている（絵は `tmp/obs-cap/grid-face.png`。git の外）。
  - **被写界深度の写し方**: 原作の Gaussian（焦点 10 cm・はっきり写る帯 100 cm・遠景のぼけ 16 px、`PostProcessVolume_1` の f/4）は UE 5.8 の desktop に無い。帯の中央 60 cm × `FrameScale` を cinematic の焦点（502.6 → 70.3 cm）にし、絞りは原作の 4.0 にした（仮）。原作のぼけは背景（ホテルの実部屋）を潰すためのもので、本作の部屋は黒いので効き目はワサミの体にだけ出る。**絞りを強める（f/1.4 など）か、入れないか**の選択もある。
- 状態: **完了（2026-09-21）**。原作の Matinee 6 本（`MonkeyJumpscare`・`2`・`3`・`03_Watcher_Kill*`）と灯・揺れ・ボリュームの値を実装と 1 つずつ突き合わせ（進捗記録の表）、差のあった 3 つを埋めた: **暗転**を Matinee の曲線 `3a² − 2a³` に（本家の 2 キーは接線 0 の自動クランプ。それまでは直線）、**被写界深度**を `JumpscareCam` の 2 トラックから（Gaussian が UE 5.8 に無いので、はっきり写る帯の中央を cinematic の焦点へ写した仮。要確認）、**灯**を原作の値に（天井灯は部屋の倍率 `SceneScale` 0.878 で長さ・その 2 乗で強さ、顔は `jumpscarelight`・`_5` の 2 灯を頭の比 0.3347 で写して目のくぼみへ）。一致していて直すことが無かったもの（カメラのキー・FOV 90・切り替え・暗転の時刻・音・天井灯の値・黒い板・揺れ `JumpscareShake` の全値・アニメの再生）と、理由を書いて閉じたもの（`MonkeyJumpscare3` は原作でもカメラに何も写らない・舞台はホテルの実部屋・部屋の大きさ）は実装記録 07 に書いた。PIE で 4 本を通して確かめ、**顔が暗い**（原作の 2 灯のままだと前髪の陰で顔の中央が 255 中 9〜19。灯の位置は目に正しく乗る）ことと、被写界深度の写し方を要確認に残した。本家の実機の収録は要らなかった（すべて原作のコードとアセットで決まった）。

### 33. 破壊と粒子の見え方（扉の破片・独房の粒子・黒い塵・扉突きの塵・除細動器の放電）

- 目標: 大目標 1・2 で焼き込みのシェーダーと cook の表から推定で組んだ破壊と粒子の材質・分布を、cook の `ResourceData` とコンパイル済みシェーダーの式から本家どおりにする。
- 完了の条件（2026-09-21 にコード優先へ書き換え。節の頭を見る）: 下の 4 行が原作のデータどおりになる。共通の作業は、cook の焼き込みの表と合わない GPU のエミッタ（`Fracture_concrete_3` の `Fragments`・`DustTrail` ほか）の分布を型データの `ResourceData` から組み直すこと。暗い場所でほとんど見えない粒子は、原作でもエミッタの位置と分布が同じ（＝本家でも見えない）ことを原作のデータで確かめたら閉じてよい。**コードで決まらなかったものだけ**、1 回の収録にまとめる。（2026-09-20・21 の収録では本家の該当の場面に届かなかった〈B1 の扉の破片・B2 の独房の粒子は、到着のガレージから 148 m の道が見つからない。項目 28 のステップ 16〉ので、収録に頼らずコードとアセットで閉じる）
- 今は（項目 28 の「後回しの一覧」から 2026-09-20 に移した行。本家での確かめ方も各行にある）:
  - 2026-09-18（項目 6）: トンネルの扉が破られるときの破片 `Fracture_concrete_3` の煙と破片の見え方 — **閉じた（2026-09-21、ステップ 1〜3・4〜6・8）。GPU のエミッタを cook の型データの `ResourceData` から組み直し、材質 3 つを書き出しの式の型と焼き込みのシェーダーどおりにした**: 焼き込みの「表」はこの 2 エミッタでは本家が使った値と合わないので、型データを正にする（道具 `Tools/dd/gpu_emitters.py`）。半透明のライティングの値を原作から写したので**煙は黒っぽくなった**（写す前は白く明るかった）。PIE では破片が飛び散り、黒い煙が 0.2 s で扉の口を覆って 1.2 s ほどで晴れる。以前は: 材質 3 つ（`whispOne_Master_directional`・`_amb`・`DebrisMaster`）を推定し（実装記録 08）、GPU のエミッタ 2 つ（`Fragments`・`DustTrail`）は cook の焼き込みの表から分布を作り直して組んだ。cook の表はその 2 つで本家が使った値と合わず（`DustTrail` の色は表 1 → 0.36 に対し cook の GPU のデータ `ResourceData` は一定の 0.078、大きさは表の上限 1 に対し約 6 倍）、PIE では煙がほとんど見えない。本家での確かめ方: 最新版の Zone 1 で 06_DoorsLock から 25 s 待ち、扉が破られる所を収録する。cook の `ResourceData`（`Fracture_concrete_3.json` の型データ）から分布を組み直す手もある。
  - 2026-09-18（項目 6）: Zone 2 の独房の粒子の材質 4 つ（`M_06_NurseSparks`・`M_Spark`・`M_Radial_Gradient`・`Squib_one`）と、棘の 58.8 s の黒い塵 `Fracture_dark_slow` の見え方 — **閉じた（2026-09-21、ステップ 4・8）。4 材質は焼き込みのシェーダーと命令まで一致していて直しは無し、黒い塵は原作の置き場所どおり（独房の床の 3 m 下）なので本家でも見えない**: PIE では廊下の床に赤い小さな十字の火花（`P_06_NurseSparks`）、独房の扉に橙の火花の筋（`Concrete_impact_large`）が見え、黒い塵は床に隠れる。以前は焼き込みのベースパスのシェーダーの式で推定し（実装記録 08）、黒い塵は PIE で 10 粒が描かれているが暗い廊下ではほとんど見えない（エミッタは本家でも床の 3 m 下）。本家での確かめ方: 最新版の Zone 2 で独房を出て廊下に立ち（棘の箱の外）、開いてから 58.8 s を待って独房を収録する。
  - 2026-09-19（項目 7）: Zone 1 の駐車場のナースがトンネルの扉を突く動きと、その `Hit FX` の塵 `P_06_NurseDoorHit` の見え方 — **閉じた（2026-09-21、ステップ 5・8）。材質 `Whisps_additive` の親を書き出しどおりに直し、塵が暗いトンネルで見えないのは本家も同じだと原作のデータで確かめた**: 加算 × ライティングありの半透明なので、灯の当たらない所では足す色が 0 になる。湧く位置（扉のトンネル側 89 cm・高さ 120 cm）も 10 粒 1 回の撒き方も原作のまま。以前は本家の針のモンタージュ（`ReaperNurse_Needle_Attack_NoSound_Montage`、ナースの骨）の代わりにワサミの `Chase_Charge`（頭を下げて突っ込む 0.53 s）を本家の時刻で流し、塵は推定の `Whisps_trans` に加算を上書きした材質（暗いトンネルではほとんど見えない。実装記録 07・08）。本家での確かめ方: 最新版の Torment Therapy の Zone 1 で、駐車場からトンネルの扉を閉ざされる所（`06_DoorsLock`）の後ろから 25 s を撮る
  - 2026-09-19（項目 8）: 除細動器の放電 `P_06_Defib` の稲妻の見え方 — **閉じた（2026-09-21、ステップ 7・8）。`M_ky_spark02_4x4` を書き出しの式の型と両方のスイッチの焼き込みどおりにした**: 直したのは glow のスイッチの形だけで、式・命令は推定と一致していた。PIE では 2.25 s ごとに赤橙の稲妻が 3 本ほど廊下を横切り、約 0.7 s で消える。以前は描かれる唯一のエミッタ `thander` の材質 `M_ky_spark02_4x4` を焼き込みのシェーダーの式で組んだ（`MI_ky_spark02_4x5`: 稲妻の形 × 粒子の色 + 赤い火花。実装記録 08）。本家での確かめ方: 本家の最新版の病院 Zone 1・2 の除細動器に近づいて放電を撮り、色・明るさ・稲妻の長さを見比べる。
- 根拠: 実装記録 08（仕掛けと粒子）・07（ナースの扉突き）、`pak_reference_2` の `Fracture_concrete_3.json`・`Fracture_dark_slow`・`P_06_Defib`・`P_06_NurseDoorHit`、下の「今は」の行。
- 依存: 大目標 2（項目 6・7・8）。
- 規模: 2
- 状態: **完了（2026-09-21）**。4 行とも原作のデータで閉じた。**直したのは 6 つ**: GPU のエミッタ 5 つ（3 システム）を cook の型データの `ResourceData` から組み直し（突き合わせの道具 `Tools/dd/gpu_emitters.py` を作り、`fx.Cascade.UseVelocityForMotionBlur=0` で最後の差を消した）、半透明のライティングの値を cook の書き出しから写すようにし（`_lit_particle`）、whisp 2 材質のテクスチャの取り方と落としていた `Radius` の薄めを直し、`DebrisMaster` の SubUV の混ぜ・色の順・落としていた `DepthFade` を直し、`M_ky_spark02_4x4` の glow のスイッチの形を書き出しの型に合わせた。**直さなかったもの**は理由を実装記録 08 に書いた（独房の 4 材質は命令まで一致、`M_Radial_Gradient` の関数呼び出しは展開したまま、`Squib_one` の `Metallic` はつながない、`P_06_NursesLand` の死んだ量子化データ）。PIE で 4 か所を見て確かめた（実装記録 08 の「確かめたこと（2026-09-21）」）。**完了の条件の読み替え**: (1) 本家の実機の収録は 1 回も要らなかった（4 行とも原作のコード・アセット・コンパイル済みシェーダーで決まった）。(2) 「暗くて見えない粒子」2 つ（ナースの扉突きの塵・棘の黒い塵）は、原作と同じ材質・同じモジュール・同じ置き場所だと確かめて閉じた（本家でも見えない）。(3) 扉の破片の煙は、原作の半透明のライティングの値を写したぶん**以前より暗く**なった（原作どおり）。

### 34. ポータルと特殊シャードの見え方（渦・ロゴ・結晶・閃光・地図の印）

- 目標: 大目標 2 の項目 13・10 で cook で式が消えた材質を推定で組んだポータルと特殊シャードを、原作の粒子の型データと残っているシェーダーから本家どおりにし、作っていない粒子を足す。
- 完了の条件（2026-09-21 にコード優先へ書き換え。節の頭を見る）: 下の 2 行が原作のデータどおりになる。ポータルは、まだ無い `PPP_PortalAppear`・`_Lock` を粒子の型データ（`ResourceData`）と残っているシェーダーから作るか、作らない理由を書いて閉じる（親 `PPP_Particles_lit`・`_fogged` の式は cook で消えている）。特殊シャードは、結晶の屈折（外した `distortion_normal`）・閃光の不透明度・地図の印の色を材質とコンパイル済みシェーダーの式で決める。**コードで決まらなかったものだけ**、1 回の収録にまとめる。（2026-09-20 の収録では A5〈特殊シャードの出現と地図の印〉が撮れなかった〈死亡でレベルが読み直されて 155 s の時計が戻る。項目 28 のステップ 3〉ので、収録に頼らずコードとアセットで閉じる）
- 今は（項目 28 の「後回しの一覧」から 2026-09-20 に移した行。本家での確かめ方も各行にある）:
  - 2026-09-19（項目 13）: ガレージのポータル `AWasamiPortal` の見え方 — 今は `M_00_Portal_Vortex` を焼き込みのシェーダーの式で組み（本家は Lit・Base Color なしを Unlit にした）、ロゴの親 `M_00_Portal_Monkey` も同じく推定、開くときと鍵をかけるときの粒子 `PPP_PortalAppear`・`_Lock` は作っていない（PyroParticlePack の親 `PPP_Particles_lit`・`_fogged` の式が cook で消えている。部品はテンプレートなし）。本家での確かめ方: 旧版のホテルの出口のポータルが欠片の後に開く場面（`01_Hotel` の `Collect Ring Piece` の後の `Lock/Unlock(False, False)`）と舞踏会場のポータル、`cooked_shaders.py "PyroParticlePack/Materials/PPP_Particles_lit."` のベースパス
  - 2026-09-19（項目 10）: 特殊シャードの結晶の材質 `m_crystal`（オーブ `m_crystal_Inst3`・赤いシャード `m_crystal_Inst`。2026-09-20 の項目 31 から Zone 2 の祭壇の球 `m_crystal_Inst2` も）、出現点を移るときの閃光の材質 `M_ky_primitiveColor`・`M_ky_lensFlare02`、地図の印 `M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy` の見え方 — 今は結晶をコンパイル済みのシェーダーの式で組み（反射と屈折の向きを曲げる `distortion_normal` は外し、親のキューブの既定は `DefaultTextureCube`）、閃光の 2 つもシェーダーから推定（`useHilight` の不透明度の相手は不明で粒子の α）、印の色はシェーダーの定数 (1, 0.2903, 0)・(1, 0, 0) のまま（`M_Shard` は画面の実測で (0.70, 0.0071, 1.0) に合わせてあり、定数は (0.482, 0, 1)。実装記録 16）。本家での確かめ方: 最新版の病院で 155 s 待ってオーブと赤いシャードの出現・明滅・移動を寄って撮り、取ってタブレットの地図の印（オーブ・赤いシャード・敵の三角）を撮る
- 根拠: 実装記録 08（ポータル）・16（特殊シャード）、`cooked_shaders.py "PyroParticlePack/Materials/PPP_Particles_lit."`、下の「今は」の行。
- 依存: 大目標 2（項目 13・10）。
- 規模: 2
- 状態: **完了（2026-09-21）**。2 行とも原作のデータで閉じ、**本家の実機の収録は 1 回も要らなかった**（節の頭の「コード優先」どおり）。**ポータル**: 渦 `M_00_Portal_Vortex` と 8 インスタンスを書き出しの `Expressions` と突き合わせて直し（差は 3 つ ── `Rotator` の既定・どのシェーダーも引かない `Albedo_1`・グループ名。式そのものは推定と一致）、ロゴの親 `M_DD_PortalLogo` を原作どおりにして式 12 → 9（焼き込みは不変）、鍵 `M_00_Portal_Lock` は直しなし。まだ無かった `PPP_PortalAppear`・`_Lock` は**作ることにして作った**（親 `PPP_Particles_lit`・`_fogged` の式が焼き込みから全部読めたので、材質 6 とテクスチャ 6 とともに `import_portal` へ。GPU の `Sparks` の焼き込みは原作と色の ±1/255 だけ違う）。出す側もつないだ（`AWasamiPortal` の 2 部品と `AWasamiSpeedBarrier::BreakIfBoosting` の 0.3 倍）。**特殊シャード**: 閃光 2 材質の不透明度を焼き込みで確定し（`M_ky_primitiveColor` は `DepthFade` の前に `useHilight` のスイッチ、`M_ky_lensFlare02` は粒子の α を掛ける。マスクの読みは G → R）、結晶 `m_crystal` から `Max`・`Saturate` の 2 つを外し（エンジンの clamp と分かった。式 49 → 46）、地図の印 3 つは色・マスク・しきいとも原作どおりで直しなし。PIE で結晶 3 つ・閃光・取得・地図の印・ポータルの鍵と開くを見て確かめた（実装記録 08・16 の「確かめたこと（2026-09-21）」）。**分かったこと**: 鍵をかけるときの `PPP_PortalAppear_Lock` は**本家でも見えない**（粒子の部品が `Logo` の子で、`Lock/Unlock` が先に `Logo` の拡縮を 0 にする。原作の @1043 → @1558）ので、本家の順のままにした。**1 行はユーザーに聞く**（下の「未回答の要確認」: `M_Shard` の色を原作の定数に戻すか）。

### 35. 2026-09-21 のユーザーの回答の反映（移動音のピッチ・脱出の曲・EXTRAS の曲・不要なアセット）

- 目標: 2026-09-21 の有人セッションで閉じた要確認のうち、コードとアセットの変更が要る 5 つを反映する。
- 完了の条件:
  - 敵の移動音のピッチの割り当てを本作の速さに合わせる（本家は 400〜800 cm/s → 1.2〜1.5 で、本作の巡回 200・追跡 430 では 1.2225 までしか上がらない）。波は本家の `DD_Rollerskating_Fast_V1_LOOP` のまま。場所: `WasamiEnemy.h` の `MoveVolumeSpeed` ほか（実装記録 07 の「移動音」）。
  - Zone 2 の脱出で曲が**聞こえる形で**引く（今は `bFadeOut` が真だが、`Escape` が同じフレームでゲームを止めるので 0.5 s のタイマーが回らず、スコア画面の下で鳴り続ける）。止める前に部品を直にフェードアウトする。場所: `WasamiZone2Flow.cpp` の `OnEndTrigger`（実装記録 10 の「既知の制約」・11）。
  - EXTRAS の**曲の欄 10 個を埋める**（本作で実際に鳴っている曲を名前つきで並べ、余る欄は減らす）。日記 10 は空のまま、絵とクレジットは今のまま。場所: `UWasamiExtrasWidget`（実装記録 19）。
  - 使っていない原作の題字 `/Game/DD/UI/Menu/TitleCards/chapter_ui_title_tormenttherapy` と `/Game/Pipeline/Debug` の検証用の材質 17 個を**消す**（クックは `/Game` を全部入れるのでパッケージに混ざる。題字は「原作のロゴは使わない」方針にも合わない）。前処理が作り直さないように、取り込みのスクリプト側も直す。
  - 敵の足の運びの再生の速さの `TODO(仮)` を外す（巡回 200・追跡 430 では `Walk` = 1.5・`Run` = 0.96 でどちらも上限・下限に当たらず、保留のときの懸念は起きない）。場所: `WasamiEnemyAnimInstance.h`。
- 根拠: 2026-09-21 の有人セッションのユーザーの回答（この節の下の「未回答の要確認」から移した）。
- 依存: なし。
- 規模: 1
- 状態: **完了（2026-09-21）**。5 つとも入れた（6 ステップ + 途中で増えた 1 つ）: **移動音**は `NurseSkateSpeed` 800 を根拠に `MoveVolumeSpeed` = 400 × `MaxSpeed` / `NurseSkateSpeed` = 215・`MovePitchSpeed` = `MaxSpeed` = 430 へ移し（足の運びの `TODO(仮)` も外した。07 記録）、**脱出の曲**は `AWasamiMusicPlayer::FadeAllMusicOut(Duration)` を足して `OnEndTrigger` で止める前にフェードを始め、`PauseTimeCounter` で時計を止めて 1 s 後に `Escape` を呼ぶようにし（10・11 記録）、**EXTRAS の曲の欄**は `DD_SoundClass_Music` の音 4 本を名前つきで並べて `SoundCount` を 10 → 4 にし（19 記録）、**Zone 1 の書類**が本家の Sound 5 ではなくその 4 本とも解放するようにし（組み立ての `COLLECTABLE_SOUNDS`。01・18・19 記録）、**使っていないアセット 18 個**（`/Game/Pipeline/Debug` の `M_Probe_*` 17 個と原作の章の題字）を消した（どれも参照 0 件。作り直す経路は無く、取り込みのスクリプトの修正は要らなかった）。PIE で 3 つとも確かめた: 巡回 200 でピッチ 1.2000・音量 0.9302 → 追跡 430 でピッチ 1.4987・音量 1.0000（07 記録）、脱出は t = 0.032 s で**ゲームが止まっていないまま曲が `FADING_OUT`**・t = 1.026 s で `Escape` が止めてスコア画面（10・11 記録）、Zone 1 の書類でセーブの `ExtrasSFX` が [] → [0, 1, 2, 3]（19 記録）。**完了の条件の読み替え**: (1) 音そのものは PIE では聞けない（エディタが前面でないと出力が無音。症状索引）ので、部品の再生状態と値で確かめた。(2) 曲の欄は「10 個を埋める」ではなく本作に実在する 4 本に減らした（ユーザーの「余る欄は減らす」）。**3 行はユーザーに聞く**（下の「未回答の要確認」: 曲の名前 3 つが仮・書類が 4 本とも解放すること・スコア画面の 1 s の遅れ）。計画の段階で 5 ステップに分けた: 移動音のピッチと足の運びの `TODO(仮)`（どちらも敵のヘッダーの定数なので C++ のビルド 1 回にまとめる） → Zone 2 の脱出の曲のフェードアウト → EXTRAS の曲の欄 → 使っていないアセット 18 個の削除と取り込みのスクリプトの修正 → PIE での確かめと項目を閉じる。**ステップ 3 で 1 つ増えた**（EXTRAS の曲の欄は 4 本になったが、Zone 1 の書類が解放するのは本家のままの Sound 5 で本作の欄に無いため、書類が 4 本とも解放するようにするステップを挟む）。消すアセット（`M_Probe_*` 17 個と題字）は**どちらも git の管理外**で、`M_Probe_*` を作る Python は無く（検証のときに MCP で直に作ったもの）、題字も `dd_ui.py` の取り込みの一覧には既に入っていない。

### 36. Windows のパッケージと本編の性能の計測

- 目標: `Development` の Win64 でパッケージし、エディタの描画が載らない本編の実際の fps を測って、項目 21 で保留にした性能の判断（このまま / 画質の選択肢を用意する）を閉じる。
- 完了の条件: `.claude/guides/distribution.md` の手順でクック → ビルド → ステージ → pak → アーカイブが通り、出来た exe がタイトルから脱出まで通しで遊べる。両ゾーンの代表の場所（項目 21 で PIE を測った 7 か所）の fps を `stat unit` / `stat fps` で測り、00 記録の性能の表に本編の列を足す。**1080p で 60 前後に届かなければ**、製品に画質の選択肢（解像度スケールか品質プリセット）を用意する項目を立てる。パッケージに原作のロゴとキャラクターのモデルが入っていないことを確かめる。**手順は `distribution.md` の「まだ整備していない」の節に書き足す**。
- 根拠: 2026-09-21 のユーザーの回答（項目 21 の要確認 (b)「クックして本編の fps を測る」）。`.claude/guides/distribution.md`、`.claude/guides/performance.md`、実装記録 00 の「性能」。
- 依存: 35（不要なアセットを消してからクックする）。
- 規模: 1
- 状態: **完了（2026-09-21）**。`Development` の Win64 で `BuildCookRun` が通り、`Saved/Archive/Windows/`（約 1.7 GB）に遊べる exe が出来た（手順と確かめ方は `.claude/guides/distribution.md`）。途中で 2 つ直した: クックが `GameFeatureData` の規則が無くてエラーで落ちるのと、**`bCookAll=True` が無いと `/Game` のアセットが `L_Title` の 1 つしか入らない**のに `BUILD SUCCESSFUL` になること（どちらも `Config/DefaultGame.ini`。症状索引）。**本編の fps は製品の初期値の HIGH で 56.4〜72.4（7 か所の平均 63.2）**で目安に届いたので、**画質の選択肢を用意する項目は立てない**（VERY HIGH だけ平均 50.9。OPTIONS には QUALITY 4 段と RESOLUTION SCALE が既にある。測り方は `Tools/game_perf.py`、表は 00 記録）。**パッケージ版はタイトルから脱出まで 1 回の起動で通った**（`Tools/game_flow.py`。228 s、18 の節目がすべて期待どおり、`Crashes` 0 件。00 記録）。パッケージに原作のロゴとキャラクターのモデルは入っていないが、**原作のナースの姿を描いた絵 3 枚**が入るので下の「未回答の要確認」に足した。ついでにデバッグの `Wasami.Delay`・`Wasami.Status`（06 記録）を足し、パッケージ版で `Wasami.Settings` が落ちるのと `Wasami.ResetSave` がタイトルで効かないのを直した。**配布は行っていない**（ユーザーに確認してから）。画面への入力が塞がれていたため、マウスとキーそのものはパッケージ版では未確認。
- 注記: **クックとビルドはユーザーが 2026-09-21 に承認した**ので無人運転で行ってよい。ただし**配布（誰かに渡す・公開する）は別途ユーザーに確認する**（`.claude/guides/distribution.md`）。

## 大目標 4: レビュー指摘の修正

- 状態: **進行中（2026-09-22 から）**（同日の有人セッションでユーザーが「無人運転に渡す」と決めた）
- 始め方: ユーザーの指示（大目標 2・3 と違い「自動」ではない。この節を「進行中（日付から）」にするのはユーザーだけ。2026-09-22 に指示を受けて Claude が書き換えた）
- きっかけ: 2026-09-22 の有人セッション。ユーザーがゲームレビュアーに通しで遊んでもらい、**High 1 件・Medium 10 件**の指摘を受けた。指摘の原文は各項目の「目標」に 1 件ずつ写してある（11 件が項目 37〜46 の 10 項目。項目 42 だけ 2 件をまとめた）。
- 達成の姿: 11 件の指摘がすべて直り、**パッケージ版**で確かめてある。
- 決め方: 大目標 3 と同じ（`.claude/guides/original-fidelity.md`。原作のコード・アセットで先に決め、それでも決まらないものだけ本家の実機を撮る）。ただし**本家に無い本作のもの**（ワサミのモデル・本作の声・本作のシンボル）は本家に根拠が無いので、ユーザーの指示と本作の今までの決めごとに従う。
- 確かめ: **PIE だけで済ませない**。指摘のうち項目 39 は PIE では出ずパッケージ版だけで出た（エディタ専用のエンジンのアセットはクックされない）。見た目に関わる項目は `Saved/Archive/Windows/wasami_deception.exe` でも見る（`.claude/guides/distribution.md`）。**レビュアーが遊んだのは Mac 版**（2026-09-22 のユーザーの回答。`~/Applications/WasamiDeception` の `.app`）だが、クックの中身は同じなので直し方は変わらない。Claude が確かめられるのは Windows 版なので、そちらで見て、Mac 版はユーザーに見てもらう。
- 達成の条件: この節の項目がすべて完了。

### 37. 敵ワサミの手のひらと手の甲が逆（High）

- 目標: 指摘（High）「敵ワサミの手のひら・手の甲が逆になっている」。手のひらの側と手の甲の側が入れ替わって見える。
- 完了の条件: (1) **どこで入れ替わるかを切り分ける**: 原本 `SourceArt/Wasami/enemy_wasami_v3.glb` そのもの / 前処理 `dd_enemy` が書く `Intermediate/Pipeline/wasami/enemy/WasamiEnemy.glb` / Interchange の取り込み / アニメ（捕獲の 3 本だけ通る `_Retarget`。07 記録に「腕と鎖骨で最大 21.7°、**手は向きも少し違う**」と測ってある）のどれか。原本と取り込み後のメッシュを同じ姿勢で並べて見るのが早い。(2) 本作の側が原因なら直す。**原本の側なら直さずユーザーに伝えて指示を仰ぐ**（モデルはユーザーの素材）。(3) 立ち姿・巡回・追跡・捕獲の 4 つで絵を撮り、手のひらが体の内側を向くことを確かめる。
- 根拠: `Content/Python/wasami_tools/pipeline/dd_enemy.py`（`ROLES`・`_Retarget`）、実装記録 07、`.claude/references/enemy-wasami-motions.md`。
- 依存: なし。
- 規模: 2
- 状態: **ユーザー待ち**（2026-09-22 から。進捗記録 `20260922-enemy-hand-flip`）。切り分けは済み: **原因は原本 `SourceArt/Wasami/enemy_wasami_v3.glb` の側**（基準姿勢が回外＝手のひらが上なのに、アニメ 16 本がそれを打ち消さないので、どの姿勢でも手のひらが体の外を向く）。前処理の出力はメッシュもスキンも原本と**バイト単位で同一**なので無罪、取り込みとアニメの作り直しも無関係。同じ骨組みのボス `boss_wasami.glb` は基準姿勢が回内で、全アニメで手のひらが内を向く（正しい）。完了の条件 (2) のとおり**直さずユーザーに指示を仰ぐ**段階（下の「未回答の要確認」の 3）。回答が来たら、(A) モデルを直してもらう → 取り込み直すだけ、(B) 本作の側で補正 → `dd_enemy.prepare` で全アニメの手を前腕の軸まわりに 180° ねじる、のどちらかを行い、立ち姿・巡回・追跡・捕獲の 4 つで確かめる。

### 38. ライフ減少からの復帰で、タブレットとダッシュの状態を引き継ぐ

- 目標: 指摘「ライフ減少からの復帰時、Shiftによるダッシュが無効化されている。本家にかかわらず、次の値はライフ減少前の状態を引き継ぐ … タブレットの表示状態 / Shiftのダッシュ状態（OptionでShift未押下でもダッシュ状態を設定している場合）」。死亡からの再開は本家どおりレベルを開き直す（06 記録）ので、プレイヤーが作り直されてタブレットは下がり（`bTabletUp` が偽）、ダッシュも切れる（`bSprintHeld`・`bSprintLatch` が偽）。**Shift を押したまま捕まると、再開後に押下の合図が来ない**ので、離して押し直すまで走れない（`AWasamiPlayerCharacter::SprintPressed` は押下でしか立たない）。
- 完了の条件: (1) 死ぬ直前の「タブレットが上がっているか」と「ダッシュが入っているか」（`bToggleSprint` のときは掛け金 `bSprintLatch`、そうでないときは Shift を押しているか）を、レベルを開き直しても残る `UWasamiGameInstance`（ライフと回収の記憶と同じ場所）に持ち越し、再開したプレイヤーで戻す。(2) PIE で、タブレットを上げたまま・Shift を押したまま捕まり、再開の直後にタブレットが上がったままで走れることを確かめる（トグルの設定が入のときも）。(3) **本家の作りに関わらずこうする**というユーザーの指示を 02・06 記録に残す。
- 根拠: `Source/wasami_deception/WasamiPlayerCharacter.cpp`（`SprintPressed`・`StopSprinting`・`ToggleTablet`・`ApplySpeed`）、`WasamiGameInstance.h`、実装記録 02・06。
- 依存: なし。
- 規模: 1
- 状態: **未着手**

### 39. 捕獲の別室の地面のグリッド（パッケージに入らないエンジンのアセット）

- 目標: 指摘「敵ワサミ襲撃時のアニメについて、本来暗闇のはずが、デバッグと思しきグリッドが地面に表示されている」。別室の黒い壁 6 枚は `/Engine/EngineDebugMaterials/BlackUnlitMaterial`（**エディタ専用のデバッグ材質**）を使っており、これはクックされないので、パッケージ版では材質の無い板になり既定の市松（グリッド）で描かれる。2026-09-22 に Windows のパッケージの `.utoc` の名前で確かめた: `BlackUnlitMaterial` は**入っておらず**、`BasicShapes/Plane`・`Cube`・`Sphere`・`BasicShapeMaterial`・`WhiteSquareTexture`・`Roboto*` は入っている。同じ理由で入っていないものがほかに 2 つある: `/Engine/EngineFonts/RobotoTiny`（死亡画面のヒントと SAVING PROGRESS の字）と `/Engine/Functions/Engine_MaterialFunctions02/ExampleContent/Textures/SphereRenderHelper`（SAVING の絵）。
- 完了の条件: (1) 別室の壁の材質を、クックされるものに替える（本作の `/Game/Wasami/…` に黒の Unlit を 1 つ作る）。(2) `RobotoTiny`・`SphereRenderHelper` も替え、パッケージ版で死亡画面と SAVING の見た目を確かめる。(3) ゲームのコードが参照する `/Engine/…` を洗い出し、パッケージに入るものだけにする（`WasamiSpecialSpawnPoint` のビルボードはエディタでしか出ないので除いてよい）。(4) パッケージ版で捕獲を 4 種とも見て、背景が真っ黒であることを確かめる。
- 根拠: `Source/wasami_deception/WasamiCapture.cpp`（`WallMesh`・`WallMaterial`）、`WasamiDeathScreenWidget.cpp`・`WasamiSavingWidget.cpp`、`Saved/StagedBuilds/Windows/wasami_deception/Content/Paks/wasami_deception-Windows.utoc`、症状索引「パッケージ版でだけ、黒いはずの板が灰色のグリッドになる」、`.claude/guides/distribution.md`。
- 依存: なし。
- 規模: 1
- 状態: **未着手**

### 40. 捕獲の音を本家の音源からワサミの音源に替える

- 目標: 指摘「敵ワサミ襲撃時について、本家の音源が再生されている。ここはワサミの任意の音源へ差し替える」。今は本家ホテルの叫び `Evil_Monkey_Scream`、館の笑い `LIVING_STATUE_Laughter_05`、斧 `Axe_Hit_03` の 3 本（`dd_enemy.CAPTURE_SOUNDS`。捕獲の 3 本は叫び 1 つ、顔を寄せる 4 本目は笑いと斧）。
- 完了の条件: (1) **2026-09-22 のユーザーの回答どおりに当てる**: 捕獲 3 本の頭の叫び（本家の `Evil_Monkey_Scream` の代わり）= `you`〈オマエ・ジャ、5.0 s。WebGL 版が持っていて未取り込みなので `dd_voices` に足して取り込む〉、顔を間近に写す 4 本目（本家の笑いと斧の代わり）= `Over`〈あっ、終わりです。取り込み済み〉。`SourceArt/Wasami/Voices/manifest.json`。(2) 捕獲の 4 本それぞれの音の入る時刻（`SoundTime`）を新しい音の長さに合わせる。(3) 本家の 3 本を捕獲から外す（ほかで使っていないことを確かめてから）。
- 根拠: `Source/wasami_deception/WasamiCapture.cpp`（`ScreamSound`・`LaughSound`・`HitSound`・`NumSounds`・`SoundTime`）、`Content/Python/wasami_tools/pipeline/dd_enemy.py`（`CAPTURE_SOUNDS`）、`Source/wasami_deception/WasamiVoice.h`、実装記録 07・10。
- 依存: なし。
- 規模: 1
- 状態: **未着手**（使う音源は 2026-09-22 に決まった。上の「完了の条件」）

### 41. Zone 1 → Zone 2 の救急車（2 台に見える・プレイヤーが置いていかれる）

- 目標: 指摘「Zone1 → Zone2への転換時、トンネルの中に救急車が2台存在し、プレイヤーが置いていかれる」。本家は屋根に乗ったまま運ばれて読み込み画面に入る。**置いていかれるのは既知**（症状索引「収録中に救急車の屋根からプレイヤーが落ちる」。屋根の後ろの壁 `BlockingVolume_Ambulance_3` が、フレームレートが落ちるとカプセルに食い込んで後ろへ押し出す）で、本家も同じ作りなので直していなかったが、パッケージ版で起きるなら本作の側で直す。2 台に見えるのは、走り出す `hospital_ambulance_new_teleport` のほかに Zone 1 の駐車場の救急車（本家は 5 台）が見えているためか、組み立てが同じ車を 2 回置いているかのどちらか。
- 完了の条件: (1) 組み立てた `L_Hospital_Zone1` の救急車のアクタを数え、本家の 5 台と重なりが無いことを確かめる。違えば組み立てを直す。(2) 置いていかれないようにする（屋根に乗っている間はプレイヤーを救急車に付ける / 後ろの壁を掃引で動かす / 走り出しでプレイヤーを屋根の中央へ寄せる のどれか）。**本家と違う作りにするので、選んだ理由を 11 記録に書く**。(3) パッケージ版で、屋根に乗ってから読み込み画面まで運ばれることを 2 回続けて確かめる。
- 根拠: `Source/wasami_deception/WasamiZone1Flow.cpp`（`On06ReachAmbulance`）、`pak_reference_2/_sequences/06_Hospital_Zone1_AmbulanceTakeOff.json`（動くのは `hospital_ambulance_new_teleport` とスポットライト 2 つだけ。本家もプレイヤーを付けない）、`_levels/06_Hospital_Zone_01.full.json`、症状索引、実装記録 11。
- 依存: なし。
- 規模: 2
- 状態: **未着手**

### 42. Zone 2 の場面の代用の見直し（テラスの棒立ち・注射器で殴られる場面）

- 目標: 指摘 2 件。(a)「Zone2のボスワサミと敵ワサミの対峙場面で、テラスに本家にはいないワサミが両手を掲げている」(b)「本家はナースに注射器で殴打されプレイヤーが床に倒れる場面だが、おそらくナースのボーンアニメを引き継げていないのか、全く理由のわからない状態になっている」。(a) は本家で**最初から隠してある**場面用のナース（`nurse_idle1_2`・`nurse_idle2_2` は `bHidden: true`）が見えていて、アニメが当たらず基準姿勢（腕を広げた `restpose`）で立っている、という見え方に合う。(b) は場面の代用（`.claude/references/enemy-wasami-motions.md` の「場面の代用」: 待ち構え = `Idle_5`、殴る = `Male_Head_Down_Charge`）が、本家の殴打の演技として読めていない。
- 完了の条件: (1) 場面に出るワサミが、本家で隠れている間は隠れている（組み立ての `bHidden` と可視のトラックの既定値を PIE で追い、直す）。(2) Zone 2 の捕まる場面 `06_Hospital_Zone2_Capture` の代用を選び直し、「殴られて床に倒れる」と読める形にする（v3 の 18 本から選ぶ。届かなければカメラと暗転で見せ方を変える）。(3) 独房 `06_Hospital_Zone2_Cell` と Zone 1 の出来事 `06_Hospital_Zone1_06Event` も同じ目で見直す。(4) 3 つの場面を撮って並べ、棒立ちのワサミが居ないことを確かめる。
- 根拠: `pak_reference_2/_levels/06_Hospital_Zone_02.full.json`（`nurse_idle1_2`・`nurse_idle2_2` の `bHidden`）、`Content/Python/wasami_tools/pipeline/dd_sequence.py`（`boolean` の既定値、ナースの配置）、`Source/wasami_deception/WasamiCutsceneNurse.*`、実装記録 07・11、`.claude/references/enemy-wasami-motions.md`。
- 依存: なし。
- 規模: 3
- 状態: **未着手**

### 43. タイトル画面の曲を本家（最新版）のものにする

- 目標: 指摘「タイトル画面のBGMが本家と異なる」。本作は本家の**旧版**の `UMG_TitleScreen` を写した（WebGL 版がそうしていたため）ので、曲がポーズ画面と同じ `Audio/UI/Pause_Sound_v1` になっている。**最新版**の `UMG_TitleScreen` は `CreateSound2D(/Game/Audio/DD_-_Dark_Deception_-_Theme_v1_3, 0.6, 1.0, 0.0)` → `FadeIn(2.0, 0.5)` でテーマ曲を流す（旧版と最新版で違う数少ない場所）。
- 完了の条件: (1) `DD_-_Dark_Deception_-_Theme_v1_3.ogg` を `/Game/DD/Audio` に取り込む（`dd_ui` の取り込みの一覧に足す）。(2) タイトルの曲をそれに替え、音量 0.6 → `FadeIn(2.0, 0.5)` を最新版のとおりにする。EXTRAS の曲の一覧（`Pause_Sound_v1` を「Pause Theme」として出している。項目 35 の要確認 1）とポーズ画面はそのまま。(3) パッケージ版でタイトルを開き、本家と同じ曲が同じ入り方で鳴ることを確かめる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/UI/Main/TitleScreen/UMG_TitleScreen.txt`（@6598・@10519）、`pak_reference_2/DDeception/Content/Audio/DD_-_Dark_Deception_-_Theme_v1_3.ogg`、`Source/wasami_deception/WasamiTitleScreenWidget.cpp`、実装記録 14。
- 依存: なし。
- 規模: 1
- 状態: **未着手**（**2026-09-22 のユーザーの回答「曲も絵も最新版に寄せる」**で確定。最終目標の「タイトル画面は WebGL 版に倣う」〈= 本家の旧版〉より指摘を優先する）

### 44. タイトル画面の筆の跡と、顔の左の境界を本家のものにする

- 目標: （**2026-09-22 のユーザーの回答「曲も絵も最新版に寄せる」**で、この項目は行うと決まった）指摘「タイトル画面における背景の筆跡、モチーフキャラクター画像の左の境界線のイメージが本家と異なる」。本家は顔（動画）の左の縁を煙のマスク `title_screen_video_mask` で切り、背景の筆の跡を `MM_TitleScreen_Mask_Grey` で流す。本作は WebGL 版の CSS（楕円のラジアルグラデーションのマスク）を焼き込んだ顔を出しているので、左の縁が本家の筆の切り口にならない。筆の跡も、cook で式が消えた材質を「`title_screen_chapters_background` を Panner で流す」と推定して組んである（14 記録）ので、本家と違って見える余地がある。
- 完了の条件: (1) 本家の顔の左の縁（`title_screen_video_mask`・`MM_TitleScreen_Mask`・`MM_TitleScreen_Mask_Grey`）をコンパイル済みのシェーダー（`python Tools/dd/cooked_shaders.py "TitleScreen/MM_TitleScreen_Mask"`）で確定し、本作の顔にも同じ切り口を掛ける。(2) 筆の跡のテクスチャと流れ（速さ・色・不透明度）を同じ式で確定する。(3) 本家のタイトル画面と本作の絵を並べ、背景の筆の跡と顔の左の縁が同じ形に見えることを確かめる。
- 根拠: `pak_reference_2/_assets/DDeception/Content/UI/Main/TitleScreen/MM_TitleScreen_Mask*.json`、`DDeception/Content/UI/Main/TitleScreen/title_screen_video_mask.png`、`Tools/dd/prepare_title.py`、`Content/Python/wasami_tools/pipeline/dd_ui.py`（`_build_title_strokes`）、実装記録 14。
- 依存: 43（同じ画面を触るので後に）。
- 規模: 2
- 状態: **未着手**

### 45. ステージ OP の紋章の中で、ワサミのシンボルが下にずれている

- 目標: 指摘「Zone1開幕アニメにおいて、紋章のワサミシンボルが若干下にズレている」。ステージ OP（`UWasamiChapterPortalWidget`）は輪とルーンの真ん中に本作の頭 `T_PauseHead` を 470 px 四方で重ねている。絵の中でのシンボルの位置が本家のキャラクターの印と違うと、同じ箱に入れても下に寄って見える。読み込み画面の紋章（`Tools/dd/prepare_loader.py` の `MARK_CENTRE = (256, 262)` は魔法陣の中心 (254.5, 255.5) より 6.5 px 下）も同じ作りなので、一緒に見る。
- 完了の条件: (1) 本家の紋章の絵の中で、キャラクターの印の見た目の中心が輪の中心からどれだけずれているかを測る。(2) 本作のシンボルの見た目の中心が輪の中心に来るようにする（前処理で絵を置き直すか、ウィジェットでずらすか。選んだ方を 09・14 記録に書く）。(3) ステージ OP と読み込み画面の両方を撮り、輪の中心に来ていることを確かめる。
- 根拠: `Source/wasami_deception/WasamiChapterPortalWidget.cpp`（`Icon` の `Logo`）、`Tools/dd/prepare_loader.py`、`pak_reference_2/DDeception/Content/UI/Main/Loaders/loader_*.png`、実装記録 09。
- 依存: なし。
- 規模: 1
- 状態: **未着手**

### 46. ガラスが透けない（ガラスのマスターを原作のシェーダーから組む）

- 目標: 指摘「本家であれば透過しているはずの、Zone1の扉のガラスなどが燻んでいる」。本家の扉のガラス `MM_Main_Substance_Glass_Doors` は `MM_Main_Substance_Glass_ColorMask`（`BLEND_Translucent`）の子で、`MaskedColor` と `RefractionDepthBias` を持ち、インスタンスで `BLEND_AlphaComposite` に上書きしている。本作はガラスの系列も一律に `M_DD_Substance` の子にしていて、この材質の不透明度は「アルベドの α × `Opacity Override`（既定 1）」なので、α が 1 のテクスチャでは透けず、くすんだ板に見える。
- 完了の条件: (1) `MM_Main_Substance_Glass` と `MM_Main_Substance_Glass_ColorMask` のコンパイル済みのシェーダー（`python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Substance_Glass"`）を読み、式どおりのマスターを作る（項目 31 の `M_DD_Metal`・`M_DD_SubstanceFresnel` と同じやり方）。(2) 前処理 `MASTERS` の振り分けにガラスを足し、ガラスのインスタンスを新しいマスターの子にする。(3) Zone 1 の扉のガラスと外のガラス（`M_06_Hospital_ExteriorGlass_01`・`_02`）を本家の絵と見比べ、透け方と映り込みが同じに見えることを確かめる。半透明は Nanite を切る対象（`translucent_meshes`）なので、焼き込みと fps も見る。
- 根拠: `pak_reference_2/_materials.json`（`MM_Main_Substance_Glass`・`_ColorMask`・`MM_Main_Substance_Glass_Doors`・`_DoorsNontransparent`）、`Content/Python/wasami_tools/pipeline/dd_stage.py`（`MASTERS`・`make_material`・`BLEND`）、`Tools/dd/prepare_stage.py`、実装記録 01。
- 依存: なし。
- 規模: 2
- 状態: **未着手**


## 取りやめた項目

### 15. 入口レベル `06_Hospital`（レベルの取り込みと組み立て）

- 目標: 本家の入口レベルを Zone 1・2 と同じ仕組み（前処理 → `/Game/DD` → レベルの組み立て → 焼き込み）で作る。
- 完了の条件: `L_Hospital_Entrance`（仮名）に配置 466・灯 307・霧・空・反射キャプチャがそろい、High で焼け、本家の実機（REPLAY の Torment Therapy は入口から始まる）の開始地点と色味を比べて Zone 1 と同じ精度（1〜2 割）に収まる。動く部品（両開き扉 10 + 4、除細動器 1、ガレージリフト 1、扉の破壊 3、針 `BP_06_Needles` ×7）は静的な配置でよい（動きは項目 16）。
- 根拠: `pak_reference_2/_levels/06_Hospital.scene.json`・`.full.json`、実装記録 00・01（取り込みと焼き込み）。
- 依存: 1。
- 状態: **取りやめ（2026-09-18）**。ユーザーの方針変更で入口は作らない（ゲームは Zone 1 のエレベーターの到着から始まる。項目 6）。

### 16. 入口の流れとカットシーン（ステージ OP・導入・注射室・レントゲン室・追走・エレベーター）

- 目標: 入口レベルの本家の流れを、ナースを敵ワサミに替えて再現する。
- 完了の条件: レベル BP どおりに、タイトルカード `UMG_ChapterPortal`（ステージ OP。WebGL 版 10 記録の `stage-intro.ts`）、Bierce の台詞、救急車の到着・扉・エレベーターのシーケンス（`06_Hospital_Entrance_*`）、ナースの導入（`BP_06_NurseInteract_Intro` ×10・`BP_ReaperNurse_IntroAI`。ワサミのモーションで代用）、注射室（`01_NeedleRoomOverlap`、針 ×7）、レントゲン室のカウントダウン（`BP_06_Countdown`・`_XRay`・`_Spikes`、`UMG_06_Countdown`）、追走（`06_Hospital_Entrance_Escape_DuoNurses`、`BP_06_ReaperNurse_EscapeSpecial`）、扉の破壊 → `OpenLevel(06_Hospital_Zone_01)` まで通しで遊べる。本家のカットシーンのカメラのキーは `_sequences/06_Hospital_Entrance_*` から写す。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/06_Hospital.txt`、`_sequences/06_Hospital_Entrance_*`、`Blueprints/06_Hospital/BP_06_Countdown*.txt`・`BP_06_Needles.txt`・`BP_06_NurseInteract_Intro.txt`、`Animation/06_Hospital/NurseIntro/*`。
- 依存: 15、7、5。
- 状態: **取りやめ（2026-09-18）**。項目 15 と同じ。このうちタイトルカード `UMG_ChapterPortal`（ステージ OP）は、2026-09-20 の有人セッションの指摘で項目 30 として Zone 1 の始まりに移した。

## 未回答の要確認（ユーザー）

閉じた進捗記録に残っていた要確認（記録ごと）。答えが出たら該当の場所を直してここから消す。SessionStart hook は未完了の進捗記録の要確認しか出さないので、ここは朝の一覧に出ない。

2026-09-21 の有人セッションで、それまで残っていた 9 件すべてに回答をもらった（答えは項目 35・36 と各実装記録へ移した。2026-09-20 の回答の反映は進捗記録 `20260920-user-answers`、2026-09-21 の分は項目 35 の記録）。その後に出たのが下の 4 件と、2026-09-22 の有人セッション（ゲームレビュアーの指摘）の 4 件。

### 20260922-review-findings（大目標 4。2026-09-22 の有人セッションで出た。4 件中 3 件は同じ日に回答をもらった）

1. ~~捕獲でどのワサミの音源を鳴らすか~~（項目 40）… **回答済み（2026-09-22）**: 叫び = `you`（オマエ・ジャ、5.0 s。未取り込みなので `dd_voices` に足す）、顔を間近に写す 4 本目 = `Over`（あっ、終わりです）。
2. ~~タイトル画面を本家の最新版に寄せてよいか~~（項目 43・44）… **回答済み（2026-09-22）**: 「曲も絵も最新版に寄せる」。最終目標（2026-09-17）の「タイトル画面は WebGL 版と同じように倣う」（= 本家の旧版）より、レビュアーの指摘を優先する。
3. **敵ワサミの手の直し方**（項目 37）: 切り分けが済み、**原因は原本 `enemy_wasami_v3.glb` の基準姿勢（回外＝手のひらが上）とアニメの食い違い**と分かった（同じ作者・同じ骨組みのボス `boss_wasami.glb` は回内で正しい）。(A) **モデルを直してもらう**（ボスと同じ回内の基準姿勢で書き出し直す。本作のコードは変えない）か、(B) **本作の側で補正する**（前処理で全アニメの手を前腕の軸まわりに 180° ねじる。ユーザーの素材には触らないが、手首の継ぎ目が出ないか要確認）か（未回答）。
4. ~~レビュアーが遊んだのは Windows 版か Mac 版か~~（大目標 4 全体）… **回答済み（2026-09-22）**: **Mac 版**（`~/Applications/WasamiDeception` の `.app`）。クックの中身は同じなので直し方は変わらない。Claude の確かめは Windows 版で行い、Mac 版はユーザーに見てもらう。

### 20260921-package-win64-perf（項目 36。2026-09-21 に閉じた）

1. **原作のナースの姿を描いたテクスチャ 3 枚をワサミの絵に替えるか**（ステップ 4）: パッケージの中身を見て見つかった。`.claude/guides/original-fidelity.md` の「ステージの中にキャラクターの姿が描かれたテクスチャ（ポスター、看板など）があったときは、ワサミの絵に差し替えるかをユーザーに確認する」に当たる（中身は絵で、キャラクターのモデルではない。原作のロゴとキャラクターのモデルは入っていないことを確かめた）。
   - `hospital_poster_nurse_01_D`（`M_06_Hospital_Poster_01`。紙袋をかぶったナースが「TAKE YOUR MEDICINE!」と言う漫画風の絵）… **Zone 1 で使っている**。
   - `hospital_decal_nurseambulance`（`M_06_Hospital_Decal_NurseAmbulance`。救急車の上で注射器を構えるナースの絵）… **Zone 1・Zone 2 の両方で使っている**。
   - `hospital_poster_nurse_02`（`M_06_Hospital_Poster_14`。注射器を持つナースの黒い影絵と「GET VACCINATED!」）… **どのレベルからも使っていない**が、`bCookAll=True` でパッケージには入る。
   - 替えるなら、WebGL 版で CC2 のポスターにしたのと同じやり方（前処理でワサミの絵を描いて `/Game/Wasami` に取り込み、材質のテクスチャを差し替える）。替えないなら「本家の絵のまま置く」と決めて `original-fidelity.md` の表に 1 行足す。
2. **パッケージ版のマウスとキーそのものが未確認**（ステップ 6・6b）: ファイアウォールの確認の窓が前面を離さず画面への入力が届かないので（下の項目 33 の 1 と同じ原因）、通しプレイはコマンドラインの `-ExecCmds` で回り道した。タイトルの NEW GAME・欠片の画面の CLOSE・スコア画面の NEXT を**パッケージ版で実際に押せるか**だけが残っている（PIE では `Tools/playthrough.py` が押して通している）。窓が消えたら `python Tools/game_flow.py run` の後に手で確かめたい。

### 20260920-deferred-look-polish（項目 28。2026-09-21 に閉じた）

1. **Matron の大きさの合わせ方**（ステップ 11）: 拡縮を、本家とこちらの**待機の姿勢**の見える高さを合わせて **5.1547** にした（本家の頭の上は床から 750.50 cm）。敵ワサミは 2026-09-18 のユーザーの指示で**基準姿勢どうし**で合わせているが、Matron で同じことをすると 6.694 になり、本家の見た目より 3 割高くなる（本家は待機で机へ身を屈めるのに、ボスワサミの待機はほぼ立ったままなので、姿勢を無視すると合わない）。この合わせ方でよいか（項目 28 の一覧の Matron の行、実装記録 17）。
2. **捕まる場面のカメラアニメの回転を入れるか**（ステップ 14d・16）: 本家の `CameraAnim_Nurse_01` は移動と回転の両方を持つ（yaw +180 で後ろを向き、最後にロール −81 で横倒しになる＝ナースに捕まったプレイヤーのカメラ）が、本作は**移動だけを変換トラックに焼き、回転は入れていない**。場面のシネカメラに当てても、本家も本作も `ACineCameraActor` の LookAt が毎フレーム回転を書き直すので回転は見えない（PIE で測った。実装記録 01 の「既知の制約」）。ただし**本家のティックの順が逆だった可能性は消せない**（同じティックグループでは登録順）。回転が効いた場合の絵はまるで違う（Sequencer のプレビューでは駐車場を振り返る構図になる）。**どちらにするか**（いまは「移動だけ・LookAt のまま」）。本家の実機で確かめるには、Zone 2 の到着のガレージから 148 m 歩いて `Trigger_Arrive_CaptureScene` を踏む必要があり、2026-09-21 に 25 分かけて道が見つからなかった（症状索引）。
3. **救急車の走り出しの暗さを、代わりの場所で確かめたこと**（ステップ 16）: 本家の Zone 1 の走り出しは、ナースのいる PARKING LOT に降りてから 65 m 先の救急車まで行く必要があるので撮っていない。代わりに、**同じ灯（`PointLight351_Tunnel_Blueprint`、インスタンスの上書き無し）が同じ高さ（路面の 884〜902 cm 上）に並ぶ Zone 2 の到着のトンネル**を、救急車の屋根の高さから 1 枚撮って測った（近くの面は屋根の高さでも路面でも暗く〈40.4・36.9〉、奥の路面だけが散乱で明るい〈60.0〉）。本作の PIE の測定と同じ関係なので「本家どおり」で閉じたが、**この代え方でよいか**。

### 20260921-user-answers-followup（項目 35。2026-09-21 に閉じた）

1. **EXTRAS の曲の名前のうち ID 1〜3 が仮**（ステップ 3）: `Hospital Panic Track`・`Hospital Zone 2 Normal Track`・`Pause Theme`。本家の EXTRAS はこの 3 本を並べていないので名前が無く、本家のファイル名から起こした。ID 0 の `Cold Hearted` だけは本家の EXTRAS の名前そのまま（`UMG_Extras` の `Extras_Sound_Button_C_14` = 本家の Sound 5 が同じ Zone 1 の曲）。**正式な曲名があれば差し替える**（`WasamiExtrasWidget.cpp` の `SoundTracks`。19 記録）。
2. **Zone 1 の書類が曲 4 本とも解放すること**（ステップ 4）: 本家はこの書類が Sound 5 の 1 本だけを解放する。本作は章が 1 つで書類もこれ 1 つなので、本家のままだと残り 3 本が永久に鍵になるため 4 本とも解放するようにした（PIE で `ExtrasSFX` が [0, 1, 2, 3] になるのを確かめた）。**1 本だけにしたい場合は言ってほしい**（組み立ての `COLLECTABLE_SOUNDS`。18・19 記録）。
3. **脱出でスコア画面が 1 s 遅れて出ること**（ステップ 2）: 曲のフェードを聞かせるため、引き金からスコア画面までの 1 s は画面が黒のまま（PIE の実測で 1.026 s）。本家はポータルに触れた所ですぐスコア画面が出る。**1 s が長ければ短くできる**（`AWasamiZone2Flow::EscapeMusicFade`。曲のフェードも同じ長さになる。10・11 記録）。
4. **音は耳で確かめていない**（ステップ 6）: エディタが前面でないと UE は出力の音量を 0 にする（`UnfocusedVolumeMultiplier`。コンソール変数が無いので戻せない）ので、PIE の音は録っても無音になる（症状索引）。脱出のフェードと EXTRAS の 4 本は、部品の再生状態（`FADING_OUT` / `PLAYING`）と値で確かめてある。**気になるようなら前面で 1 度聞いてみてほしい。**

### 20260921-portal-shard-look（項目 34。2026-09-21 に閉じた）

1. **`M_Shard`（通常のシャードの地図の印）の色を原作の定数に戻すか**（ステップ 7）: 原作の材質の定数は (0.482481, 0, 1) だが、`dd_shards` は大目標 2 のときに PIE のタブレットを実測して (0.70, 0.0071, 1.0) に寄せている（本家の画面の #d21ee6 に合わせるため。実装記録 06）。大目標 3 は「根拠は原則コード」なので原作の定数に戻す手もあるが、タブレットの見え方が本家とずれるため**今は実測の値のまま**にした。特殊シャードの印 3 つ（`M_PowerOrb`・`M_Bonus_Shard`・`M_Enemy`）は原作の定数どおりで、この件の影響は受けない。

### 20260921-destruction-particles（項目 33。2026-09-21 に閉じた）

1. **Windows のファイアウォールの許可ダイアログが画面に出たまま**（ステップ 3〜8）: エディタを開き直したときに「パブリック ネットワークとプライベート ネットワークにこのアプリへのアクセスを許可しますか？」（UnrealEditor / `PickerHost.exe`、画面の (1492, 487)〜(1947, 904)）が出て、そのままになっている。OS 全体の設定なので触っていない。**許可するか閉じるかを決めてほしい。** 出ている間はビューポートの左端 (1820〜1947) が隠れるので、PIE の絵は重ならない矩形 (2230, 215)〜(2865, 1020) でしか撮れない（重なっている窓はもう 1 つ、エディタの「出力ログ」の浮いた窓 (1220, 394)〜(2220, 994) もある）。
2. **note の GIF `12-doors-busted.gif` を撮り直したい**（ステップ 8）: 扉が破られるときの煙が、原作の半透明のライティングの値を写したぶん**白く明るい煙から黒っぽい煙に変わった**（原作どおり）。`.claude/guides/note-progress.md` の「見た目や操作が変わったものは GIF を撮り直す」に当たるが、ビューポート全体（1826, 205）〜（2864, 859）を撮るには 1 の 2 つの窓が邪魔なので撮っていない。1 が片づいたら撮り直して記事を更新する。`28-defib.gif`（除細動器）は、材質の直しが式の形だけで見え方が変わらないので撮り直さない。
