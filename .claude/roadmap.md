# 作業一覧（最終目標までの段階）

2026-09-17 にユーザーが示した最終目標を、Claude が段階（項目）に分解したもの。**1 項目 = 進捗記録 1 件**（`.claude/progress/`）。2026-09-18 に、項目を**3 つの大目標**（最小の通しプレイ → ゲームとして一通り → 本家に忠実に）の節に分けた（下の「大目標」）。運用は `.claude/guides/autonomy.md` の「何を作業するか」: 進捗記録が無いときは、**進行中の大目標の節**の「未着手」で依存が満たされた最初の項目を取り、最初の反復は計画（進捗記録を作りステップに分けてコミット）だけで終える。項目が終わったら状態を「完了（日付）」にし、次の項目は次の反復で始める。大規模な項目は進捗記録の中でステップに分ける（`.claude/guides/git-workflow.md` の大規模改修 = 作業ブランチ）。

各項目は「目標 / 完了の条件（何を測って何に合えば終わりか）/ 根拠の置き場 / 依存 / 規模（下の「進捗率」）/ 状態」。根拠は原則コード（`pak_reference_2/`。テレポーテーションは `pak_reference/`。`.claude/guides/original-fidelity.md`）。数は本家のレベルの書き出し（`_levels/06_Hospital*.scene.json`）から数えた。

番号は項目を指すための名前で、**作業の順は大目標の節の中の並び順**（「最初の項目」は進行中の大目標の節の上から数える）。22・23 は 2026-09-17 の要確認の回答から足した項目。15・16（入口）は 2026-09-18 に取りやめた（経緯として残す）。24〜28 は 2026-09-18 に大目標に分けたとき、項目 6・7・9 から後の大目標へ回す部分を分けたものと、大目標ごとの確かめ・後回しの受け皿として足した。

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
| 3. 本家に忠実に | 大目標 1・2 で後に回した見た目と演出を、本家の実機の観察と原作の式で詰める | 始める前にユーザーと中身を決める |

- **大目標の状態**は各節の頭の `- 状態:`（未着手 / 進行中 / 達成（日付））。**進行中は 1 つだけ**。
- **未着手 → 進行中に変えるのはユーザーだけ**（有人セッションで指示を受けて Claude が書き換える）。無人運転は大目標の状態を「進行中」にしない。**例外は節の頭の `- 始め方:` が「自動」の大目標**（無ければ「ユーザーの指示」）: 前の大目標を達成したら、無人運転が「進行中」にして続ける。いまは大目標 2 だけ（2026-09-19 のユーザーの指示「本目的2への以降は有人セッションなしで、無人運転が続行できるものとします。」）。大目標 3 は項目 28 の後回しの一覧をユーザーと見直してから始めるので、ユーザーの指示のまま。
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

- 状態: 進行中（2026-09-19 から）
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
- 状態: 未着手。

### 10. 特殊シャード 2 種（スタンオーブ・敵の位置が地図に出るボーナスシャード）

- 目標: 本家と同一の見た目の `BP_PowerOrb`（オーブ）と `BP_BonusShard`（赤いシャード）を、本家の出現点と周期で出し、取ると敵が 17 s 気絶（倒れる 2 本からランダム → 明けに起き上がる。秒数とアニメは敵の側。項目 4）／敵が 60 s タブレットの地図に出る。
- 完了の条件: 出現点（Zone 1: オーブ 11・ボーナス 10、Zone 2: 10・10）と周期・最初の出現の時刻がレベル BP とアクタのコードどおり。メッシュと材質は原作のアセット（`power_orb`、赤いシャードの材質）。取得で `UMG_VignetteSides`（`ENEMIES STUNNED` / `ENEMIES REVEALED`。WebGL 版 10 記録）が出て、全敵に `SetState(Stun, byOrb)`、地図に敵の印。テレキネシスでは引き寄せられない。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Main/BP_PowerOrb.txt`・`BP_BonusShard.txt`、`_assets/**/BP_PowerOrbSpawnPoint*`・`BP_BonusShardSpawnPoint*`、`.claude/references/dark-deception/04-mechanics-items.md`（原作データの値）、WebGL 版 08 記録。
- 依存: 7、9（気絶のモーションと敵の印）。
- 規模: 2
- 状態: 未着手。

### 11. Zone 2 の Matron（大きい敵ワサミ。視界コーンの中ボス）

- 目標: Zone 2 の `BP_06_Matron_MiniBoss` を、大きい敵ワサミとして本家の巡回路・視界コーン（`BP_06_Miniboss_viewcone_Matron_Long` / `_Short`）・発見で追跡型の行動に入る形で置く。
- 完了の条件: 巡回路と視界コーンの形・速さ・見つかった後の行動がコードどおり。モデルは `tmp/boss_wasami.glb`（2026-09-18。アニメの役は `.claude/references/enemy-wasami-motions.md` の「ボスワサミ」）で、大きさは本家の Matron から決める。PIE で見つからずに通れることと、見つかると追われることを収録。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Animation/Enemies/Nurse/Matron/MiniBoss/BP_06_Matron_MiniBoss.txt`、`Blueprints/06_Hospital/Miniboss/*.txt`、`_levels/06_Hospital_Zone_02.full.json`。
- 項目 7 から（2026-09-19）: 視界コーンの基底 `AWasamiViewcone`（本家の `BP_06_Miniboss_viewcone`）・インターフェース `IWasamiViewconeInterface`（`BPI_06_Viewcone`）・見張りの `_Nurse` と地図の印の材質、Zone 2 の流れの `Activate MiniBoss Enemies`（Matron の `MnM_Matron_Idle_2` を呼ぶ所だけ空け）は作ってある（実装記録 07・11）。この項目は Matron と `_Matron_Long`・`_Short` を足す。
- 依存: 7。
- 規模: 2
- 状態: 未着手。

### 12. 秘密と収集物（シークレット）

- 目標: 本家の秘密の部屋と収集物を同じ場所に置き、スコアの `SECRETS` に数える。
- 完了の条件: `BP_SecretRoomZone` ×1・`BP_07_Zone1_SecretWall` ×1（Zone 2）、`BP_MysteryCollectable` ×3（Zone 2）、`BP_Collectable` ×2（Zone 1）+ ×1（Zone 2）が（入口の `BP_MysteryCollectable_LoreNote` ×1 は入口ごと作らない。2026-09-18）、見つけ方・取り方・表示（`UMG_Collectables_Secret`）までコードどおり。取ると進行にセーブされる。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/Shared/BP_SecretRoomZone.txt`、`Blueprints/Main/BP_MysteryCollectable*.txt`・`BP_Collectable.txt`、`UI/Main/UMG_Collectables_Secret.txt`、`_levels/06_Hospital*.full.json`。
- 依存: 5、6。
- 規模: 2
- 状態: 未着手。

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
- 状態: 未着手。

### 26. 追跡中のランダムの動き

- 目標: 2026-09-18 のユーザーの指示（上の「決めたこと」の「追跡中のランダムの動き」）。
- 完了の条件: 追跡中は約 8 秒に 1 回、v3 の追いかける動き 6 本からランダムに流す（前方が空いているときだけ、速さは 800 cm/s のまま。`.claude/references/enemy-wasami-motions.md` の「追跡中のランダムの動き」。頻度と早回しの上限はテストにする）。PIE で追跡中に流れることを収録。
- 根拠: `.claude/references/enemy-wasami-motions.md`、実装記録 07。
- 依存: 7。
- 規模: 1
- 状態: 未着手。

### 19. 曲と環境音・効果音の残り

- 目標: 病院の曲（通常・追跡・Nightmare）と環境音、まだ無い効果音を本家どおりに鳴らす。
- 完了の条件: `BP_06_MusicPlayer`（Zone 1）・`BP_06_MusicPlayer_Zone2` の切り替えとフェードがコードどおり。レベルの `AudioComponent`（Zone 1 43・Zone 2 146）と `AmbientSound`・`AudioVolume` を配置どおりに置く。減衰は SoundCue と減衰設定の値どおり。捕獲の別室（項目 9 の `AWasamiCapture`。いまは無音）の音: 本家ホテルの捕獲の Matinee の `InterpTrackSound`（`Evil_Monkey_Scream`・`EN01_Toy_Monkey_Attck_Knife_v1`/`v4`。`pak_reference/_levels/01_Hotel.full.json` の `MonkeyJumpscare*` の `InterpData`）を時刻どおりに鳴らす。サルの声なので、WebGL 版（04 記録の `caught`: `enemy_scream` = `Evil_Monkey_Scream` を 1.0 で 1 回、13 記録）のように叫びだけにするかは、この項目で WebGL 版に倣って決める。捕獲の間は音を聞く位置が別室のカメラへ移るので、2D か別室の中で鳴らす（実装記録 07 の「既知の制約」）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/06_Hospital/BP_06_MusicPlayer*.txt`（`BP_08_MusicPlayer` の派生）、`_soundcues.json`、`_levels/06_Hospital*.full.json`、実装記録 01（`UWasamiSoundCueLibrary`）。
- 依存: 6。
- 規模: 2
- 状態: 未着手。

### 20. 台詞と字幕（Bierce の台詞、WebGL 版のワサミの声）

- 目標: Bierce の台詞（`BierceTalk_Blueprint`）を本家どおり鳴らして字幕を出し、WebGL 版のワサミの声（16 本 + 字幕の `manifest.json`。巡回・発見・タイトルなど）を敵とタイトルに付ける。
- 完了の条件: 各レベル BP の `Talk` の呼び出しがそろい、字幕は本家の `_localization.json` / `_strings.json` の文言。ワサミの声は WebGL 版 06・15 記録の鳴らし方（バス・音量・字幕の秒数・場面）どおり。原本は `<WEBGL>/voices/`（wav 55）と `<WEBGL>/public/voices/`（mp3 16 + manifest）。
- 根拠: `pak_reference_2/_bytecode/DDeception/Content/Blueprints/**/BierceTalk_Blueprint.txt`、`_audio.json`（`Audio/Dialogue/Bierce/Ch06/TT/*`）、WebGL 版 06・15 記録。
- 依存: 7、17。
- 規模: 2
- 状態: 未着手。

### 21. 仕上げ（通しプレイ・性能・パッケージ）

- 目標: タイトルから脱出まで通しで遊べることを確かめ、性能（この PC で 1080p・60 fps 前後、VRAM 6 GB 以内）を整え、パッケージする。
- 完了の条件: 通しプレイの収録（タイトル → Zone 1 → Zone 2 → 脱出 → スコア）で止まる箇所が無い。両ゾーンの性能の計測が `.claude/guides/performance.md` の目安に収まる。パッケージは**配布の話なので先にユーザーに確認する**（`.claude/guides/distribution.md`。無人モードでは飛ばして要確認に書く）。
- 根拠: `.claude/guides/performance.md`・`distribution.md`、実装記録 00。
- 依存: 大目標 1 と、大目標 2 のほかの項目すべて（2026-09-18 に大目標に分けるまでは 1〜20・22・23 だった。項目 23 は大目標 3 へ移った）。
- 規模: 3
- 状態: 未着手。

## 大目標 3: 本家に忠実に

- 状態: 未着手
- 始め方: ユーザーの指示（先に項目 28 の後回しの一覧をユーザーと見直して項目を立て直す）
- 達成の姿: 大目標 1・2 で後に回した見た目と演出が、本家の実機の観察と原作の式で本家どおりになる。
- 決め方: 今までの決め方（`.claude/guides/original-fidelity.md`。本家の観察・収録の見比べ・原作のコンパイル済みシェーダーの式の写し）。
- **始める前に、有人セッションでユーザーと中身を見直す**（項目 28 の後回しの一覧から項目を立て直す）。
- 達成の条件: この節の項目がすべて完了。

### 23. パワーの見た目の詰め（テレキネシスの力場と球の灯、Telepathy の印）

- 目標: 2026-09-17 のユーザーの回答。テレキネシスの球の粒子の灯を本家の見え方に合わせて弱め、テレキネシスの力場の推定の材質と Telepathy の印は、本家の実機の観察を続けて詰める。
- 完了の条件: (1) 本家と同じ条件（Lv4・画質「高」・速さ 0.25）で、力場の間の画面の平均が本家 (95〜99, 141〜147, 186〜192) に近づく（今の本作は (123〜127, 174〜176, 216〜218)。線形の明るさで、本家の灯の寄与は本作の約 0.5〜0.7 倍。灯を切ると本家よりずっと暗い）。灯は `P_ky_forceField_Telekinesis` の `sphere` の `ParticleModuleLight`（原作の `BrightnessOverLife` 5）。弱める値は、原作から離れる本作の調整としてコードと実装記録 04 に書く。(2) 最新版の実機で、テレキネシスの力場（オーラの 4 層と地面の輪の流れ、球の幕の筋〈本作は少ない〉、終わりの破片）と、近くのナースの Telepathy の印（雲の流れる速さと向き、縁のこぶ〈本作は丸に近い〉。前回の収録では、雲の変わる速さは今の仮の値の 1〜1.5 倍）を長めに撮り、`dd_powers.py` の推定のグラフ（`_build_wall02`・`_build_aura7`・`_build_shockwave02`、Telepathy の `M_DD_Telepathy`）と仮の値（`AURA_LAYERS`・`SHOCKWAVE_PANS`・`TELEPATHY_PAN_*`、どれも `TODO(仮)`）を見比べて詰める。撮るものの一覧を先に作り、本家は 1 回・30 分を目安にする（`.claude/guides/observation.md`）。
- 根拠: 実装記録 04 の「既知の制約・注意点」、`observations/README.md`（パワーの作業のステップ 11b1・11b4・11b5）、`.claude/references/powers/03-telekinesis-vanish.md`・`04-primal-telepathy.md`。
- 依存: 1。
- 規模: 2
- 状態: **保留**（2026-09-18 に大目標 3 へ移した。進捗記録 `20260918-power-look-tuning.md` は `status: 保留`）。計画の 1〜5 は済み: 球の灯を 0.35 倍に弱め、力場の材質 4 つ（星屑・幕・オーラ・地面の輪）を原作のコンパイル済みシェーダーの式どおりにした（実装記録 04、`observations/README.md`）。残りは 6（Telepathy の印を式どおりにする）と 7（仕上げ）。

### 28. 後回しにした見た目と演出を本家どおりに詰める

- 目標: 大目標 1・2 で「見た目の詰めをしない」決め方のために後に回したものを、本家の実機の観察と原作の式で詰める。
- 完了の条件: 下の「後回しの一覧」の各行が、本家どおりになったか、ユーザーがこのままでよいとした。大目標 3 を始める前に、有人セッションでユーザーと一覧を見直し、大きいものは項目に立て直す。
- 後回しの一覧（大目標 1・2 の作業が 1 件 1 行で足す: `- <日付>（項目 N）: <何を> — 今は <何にした>。本家での確かめ方: <実機の場面・原作のアセット>`）:
  - 2026-09-18（項目 5）: 死亡画面のアニメの区間の `RestoreState`（Fade In・Death の終わりで元の値に戻すか）— 今は戻さない（最後の値のまま。WebGL 版の収録ではゲームオーバーのボタンが見えている）。タブレットの Count Shake（03 記録）は終わりで戻しているので、どちらかが UE 4.24 の振る舞いと違う。本家での確かめ方: 最新版で 1 回死に、死亡画面が Fade In の後も見えているかと、シャードを拾った後の数の位置（12 px 下に残るか）を収録する。
  - 2026-09-18（項目 5）: 死亡画面のボタンとヒントの書体 — 今はボタンの `helvetica-normal_Font` の既定の書体を helvetica の面に、ヒントは UE5 の `RobotoTiny` の `Light`。本家の Font は既定がエンジンの Roboto で helvetica は en-US の副書体、ヒントは UE 4.24 の RobotoTiny。本家での確かめ方: 最新版のゲームオーバーの画面を撮り、RESTART の字形とヒントの太さを比べる。
  - 2026-09-18（項目 6）: トンネルの扉が破られるときの破片 `Fracture_concrete_3` の煙と破片の見え方 — 今は材質 3 つ（`whispOne_Master_directional`・`_amb`・`DebrisMaster`）を推定し（実装記録 08）、GPU のエミッタ 2 つ（`Fragments`・`DustTrail`）は cook の焼き込みの表から分布を作り直して組んだ。cook の表はその 2 つで本家が使った値と合わず（`DustTrail` の色は表 1 → 0.36 に対し cook の GPU のデータ `ResourceData` は一定の 0.078、大きさは表の上限 1 に対し約 6 倍）、PIE では煙がほとんど見えない。本家での確かめ方: 最新版の Zone 1 で 06_DoorsLock から 25 s 待ち、扉が破られる所を収録する。cook の `ResourceData`（`Fracture_concrete_3.json` の型データ）から分布を組み直す手もある。
  - 2026-09-18（項目 6）: Zone 2 の独房の粒子の材質 4 つ（`M_06_NurseSparks`・`M_Spark`・`M_Radial_Gradient`・`Squib_one`）と、棘の 58.8 s の黒い塵 `Fracture_dark_slow` の見え方 — 今は焼き込みのベースパスのシェーダーの式で推定し（実装記録 08）、黒い塵は PIE で 10 粒が描かれているが暗い廊下ではほとんど見えない（エミッタは本家でも床の 3 m 下）。本家での確かめ方: 最新版の Zone 2 で独房を出て廊下に立ち（棘の箱の外）、開いてから 58.8 s を待って独房を収録する。
  - 2026-09-18（項目 6）: 救急車が走り出して 2〜3 s の間、トンネルの床の前方が黒い矩形で欠けて見える（救急車は本家どおり Static で、Sequencer が動かす間だけ Movable になる。その影か VSM の欠けと思われる）— 今はそのまま。本家での確かめ方: 本家の Zone 1 で救急車の屋根に乗り、走り出しのトンネルの床を見る。収録 `Intermediate/DesktopAgent/shots/ambulance_zone2.mkv`（git の外）の 4.5〜6 s
  - 2026-09-18（項目 6）: Zone 2 のリフトの乗り方（実装記録 12 の「既知の制約」）— 今は本家のコードどおり: 角のリフト `BP_06_LiftBase_Corner` の上で立ち止まると、上の階に残る `LiftCollision1` のせいで床が約 70 cm 沈んでは戻るのを約 1.4 s ごとに繰り返し、上から下りられない。長い床 `BP_06_Lift_03` は歩き（300 cm/s）で近づくと縁に触れた時点で上がり始めて段差になり、走らないと乗れない。UE 4.24 の動く床の扱いで本家も同じになるかは未確認。本家での確かめ方: 最新版の Zone 2 の迷路で、長い床に歩いて近づく所と、角のリフトに下から乗って上で立ち止まる所を収録する。
  - 2026-09-19（項目 7）: Zone 1 の駐車場のナースがトンネルの扉を突く動きと、その `Hit FX` の塵 `P_06_NurseDoorHit` の見え方 — 今は本家の針のモンタージュ（`ReaperNurse_Needle_Attack_NoSound_Montage`、ナースの骨）の代わりにワサミの `Chase_Charge`（頭を下げて突っ込む 0.53 s）を本家の時刻で流し、塵は推定の `Whisps_trans` に加算を上書きした材質（暗いトンネルではほとんど見えない。実装記録 07・08）。本家での確かめ方: 最新版の Torment Therapy の Zone 1 で、駐車場からトンネルの扉を閉ざされる所（`06_DoorsLock`）の後ろから 25 s を撮る
  - 2026-09-19（項目 7）: Zone 2 の見張りの視界コーンの地図の印（扇 `map_enemy_search_Mat` と点 `0_DotCircle_Mat`）の見え方 — 今は本家の Unlit・半透明の式（焼き込みのシェーダー）を、地図のキャプチャ（`SCS_BaseColor`）に写る Default Lit・Masked にした推定（切り抜き 0.1。扇の縁が硬く、薄れはベースカラーの暗さだけ。実装記録 03 の「マテリアル」）。本家の地図でコーンがどう見えるかは実機と見比べていない
  - 2026-09-19（項目 7）: 見つける前に Primal Fear などで気絶した見張りの姿勢 — 今はアニメが 17 s で起き上がって立つ（State は本家どおり Stun のまま。見つけて跳び降りた後の 17 s も立ったまま止まる。実装記録 07 の「既知の制約」）。本家での確かめ方: 最新版の Zone 2 のミニボスの廊下で、見張りに見つかる前に Primal Fear を使い、棚の上と跳び降りた後の姿勢を見る（本家の ABP は State が 0 に戻るまで気絶の姿勢）
  - 2026-09-19（項目 9・24）: 捕獲の別室の見た目と動き — 今は本家ホテルの 1 本目の Matinee の t=0 のカメラがサルの頭を写す写し方をワサミの上半分に当てた寄り（前 111・上 177 cm、FOV 90）から、Matinee のカメラのキーで動き、ワサミの首を目で追う向きを重ね（`AimSpeed` 8）、首の前 50 cm で突っ込みを止める。サルの真上の天井灯 `ceilinglights_80` を縮めた位置の灯（1500・半径 500・水色）と顔の灯（頭の前 50・上 30、300・半径 200）。組は `Capture_1`↔`MonkeyJumpscare`・`Capture_2`↔`MonkeyJumpscare3`（止まる区間に `MonkeyJumpscare` の揺れを 1.797 倍で入れた）・`Capture_3`↔`MonkeyJumpscare2`。場面の時間は 2.5 倍から 0.4 s で等速へ。滑りは 0.45 s、歩きは 1.2 s から始める。暗転は直線の `StartCameraFade`（本家は曲線）。`JumpscareCam` の旧 DOF（焦点 142.9 → 10・領域 571.4 → 100 のトラック）は写していない。顔の 4 本目（館の `BP_03_Watcher` の体）は、走りで 250 cm 後ろから 0.1 s の指数で寄り、カメラは前 121.5・上 186・FOV 75 から `03_Watcher_Kill3`、頭の前 40 cm で止め、ウォッチャーのカメラへの 0.15 s の寄せは切り替えにした。本家での確かめ方: 旧版の実機のホテルでサルに捕まる 3 本（`MonkeyJumpscare`・`2`・`3`）と館でゴールドウォッチャーに捕まる 3 本を撮り、寄り・明るさ・ぼけ・揺れの大きさ・暗転の速さを見比べる
  - 2026-09-19（項目 13）: Zone 2 の祭壇と欠片の材質 — 今は祭壇の `MM_00_Ballroom_Ring_Altar_Metal`（親 `MM_Main_Metal`: 法線 `RingStatue_N`・`Roughness` 0.35・`Hover Color`）と欠片の `M_ring_metal2`（親 `MM_Main_Substance_Fresnel`）を前処理の `substance` のマスターの推定で作った（祭壇は色のテクスチャが無く、桃色の灯で青みの白の像に見える。実装記録 08）。欠片の光の粒子 `P_08_RingPiece` の材質も推定の `M_ky_primitive` の子。本家での確かめ方: 最新版の Zone 2 の迷路の後の部屋の祭壇と欠片を撮り、`python Tools/dd/cooked_shaders.py "MasterMaterials/MM_Main_Metal."` で式を読む。
  - 2026-09-19（項目 13）: ガレージのポータル `AWasamiPortal` の見え方 — 今は `M_00_Portal_Vortex` を焼き込みのシェーダーの式で組み（本家は Lit・Base Color なしを Unlit にした）、ロゴの親 `M_00_Portal_Monkey` も同じく推定、開くときと鍵をかけるときの粒子 `PPP_PortalAppear`・`_Lock` は作っていない（PyroParticlePack の親 `PPP_Particles_lit`・`_fogged` の式が cook で消えている。部品はテンプレートなし）。本家での確かめ方: 旧版のホテルの出口のポータルが欠片の後に開く場面（`01_Hotel` の `Collect Ring Piece` の後の `Lock/Unlock(False, False)`）と舞踏会場のポータル、`cooked_shaders.py "PyroParticlePack/Materials/PPP_Particles_lit."` のベースパス
- 根拠: 各行に書く。
- 依存: 大目標 2。
- 規模: 3
- 状態: 未着手。

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
- 状態: **取りやめ（2026-09-18）**。項目 15 と同じ。

## 未回答の要確認（ユーザー）

閉じた進捗記録に残っていた要確認（記録ごと）。答えが出たら該当の場所を直してここから消す。SessionStart hook は未完了の進捗記録の要確認しか出さないので、ここは朝の一覧に出ない。

### 項目 18（オプション画面とポーズ画面、2026-09-19 に閉じた記録 `20260919-options-pause`）

- 2026-09-19: オプション画面で Esc を押しても何も起きない（本家どおり。閉じるのは CANCEL / SAVE & EXIT だけ）。WebGL 版は Esc を CANCEL と同じにしていた。Esc でも閉じたいか。場所: 実装記録 15（`UWasamiOptionsWidget`）。
- 2026-09-19: 本家の最新版は、EASY でライフ 0 の死亡画面のあいだゲームが止まっていて Esc も効かない（コードどおりだと抜け道の無い行き止まり）。回答（抜けるのはポーズから）どおりにするため、その死亡画面の上でだけ、止まっていても Esc でポーズが開くようにした（ポーズは旧版の Z 5 で死亡画面の上に出す）。本家どおり行き止まりのままにするか、この形でよいか。場所: 実装記録 02（`EscapePressed`）・09（死亡画面の EASY）。
- 2026-09-19: その死亡画面の上で開いたポーズの RESUME は、ポーズを閉じても**ゲームを止めたまま**死亡画面に戻す（本家には無い道なので本作で決めた。解くと黒い死亡画面の下でゲームが動き、敵にまた捕まる）。この形でよいか。場所: 実装記録 15（`UWasamiPauseWidget` の RESUME）。
- 2026-09-19: ポーズの RESTART の YES はライフを戻さない（本家の両版どおり。戻すのは死亡画面の RESTART・レベルの終わり・タイトルの `Reset Game Instance` だけで、病院のレベルはライフを読まない）。そのため EASY でライフ 0 の死亡画面からポーズの RESTART で抜けても 0 のままで、次に死ぬとまた同じ死亡画面になる（NORMAL のゲームオーバーの RESTART は 3 に戻す）。本家どおりでよいか、ポーズの RESTART（か EASY の死亡画面から抜けたとき）でライフを 3 に戻すか。場所: 実装記録 15 の「ボタンの道」。

### 項目 17（タイトル画面、2026-09-19 に閉じた記録 `20260919-title-screen`）

- 2026-09-19: タイトルの右上の版の文字 — 仮にプロジェクト設定の `ProjectVersion` を `0.1.0` にして `v0.1.0` と出した。理由: 本家は `v1.6.1`、WebGL 版は `package.json` の版（0.2.0）を出していた。本作に版の決まりが無い。場所: `UWasamiTitleScreenWidget` の `TextBlock_0`（実装記録 14）と `Config/DefaultGame.ini`（`TODO(仮)`）。

### 項目 14（スコア画面、2026-09-19 に閉じた記録 `20260919-level-clear`）

- 2026-09-19: TOTAL SHARDS は本家のコードどおり **679 を 2 回数える**（病院のレベル BP が `Shards_Shards` にも 679 を入れ、画面の合計は `Shards_Shards` と `Shards_Var` の文字の両方を足す。新しいセーブで 1483、全部 S で 1558）。本家の書き誤りらしいが、ゲームの規則の値なのでコードに従った。1 回にしたいときは `FWasamiLevelResults::GetTotalShards` を直す。場所: 実装記録 13。
- 2026-09-19: スコア画面の上のレベルの題字 — 仮に本家の病院の題字 `chapter_ui_title_tormenttherapy`（「Torment Therapy」の飾り文字。本家どおり赤く染める）を使った。理由: 本作のステージは本家の病院そのもので、題字はロゴでもキャラクターでもない UI の文字（WebGL 版の `you-escaped` と同じ扱い）。WebGL 版はステージが違ったので、ユーザーの題字「Stinky Gachimi」を使っていた。場所: 実装記録 13（`LevelNameTexture`）。

### 項目 13（脱出、2026-09-19 に閉じた記録 `20260919-escape`）

- 2026-09-19: ガレージでポータルが開いた後の目的の文 — 仮に `GET TO THE PORTAL` にした。理由: 本家のホテルが出口のポータルを開けたときの `Get back to the portal.` を、病院の目的の書き方（`HEAD TOWARDS THE GARAGE` などの大文字）に合わせ、まだポータルへ行っていないので back を落とした。本家の病院にはポータルへ向かう文が無い（救急車の `GET ON TOP OF THE AMBULANCE`）。場所: `WasamiZone2Flow.cpp` の `OnPostmazeTriggerGarage`、実装記録 11。
- 2026-09-19: 祭壇の場所 — 仮に本家の病院 Zone 2 の祭壇 `ring_statue_2`（迷路の後の部屋）をそのまま使い、ガレージにはポータルだけを置いた。理由: 本家の病院に祭壇と「欠片 → 障壁 → ガレージ」の流れが既にあり、作業一覧の完了の条件（欠片の画面で障壁が壊れてガレージへ）とも合う。2026-09-17 の最終目標の「ガレージの祭壇とポータルで脱出」は、病院に祭壇があると知る前の言い方と読んだ。場所: 上の項目 13 の状態（完了の条件の読み替え）。
- 2026-09-19: ポータルの `Logo`（本家は `M_00_Portal_Monkey` のサルの顔）— 仮にワサミのシンボル（読み込み画面の紋章と同じユーザーの印）に替えた（本家の印と同じ大きさ・位置・赤で描いた `/Game/Wasami/Portal/T_Portal_Wasami`）。理由: 原作のキャラクターは使わない（`.claude/guides/original-fidelity.md`）、読み込み画面の紋章の前例（2026-09-19 のユーザーの回答）。場所: 実装記録 08 の「ガレージのポータル」、前処理 `Tools/dd/prepare_portal_logo.py`。

### 項目 7（NavMesh と敵の AI、2026-09-19 に閉じた記録 `20260919-enemy-ai`）

- 2026-09-19: 完了の条件の「Nightmare（全回収後）で `run_fast_2`」— 仮に作らない（`AWasamiEnemy::bNightmare` とアニメの `Run_Nightmare` の分岐は残す）。理由: 本家の病院の Zone では全回収で敵を全部消し（Zone 1 の `05 All Shards Collected`、Zone 2 の `Postmaze Transition`。本作も項目 6 で同じ）、ナースはゲームモードの `Activate Frenzy` を実装していないので、全回収の後に追ってくる敵がいない（Nightmare のナース `BP_06_ReaperNurse_06_Chase_Nightmare` は入口 `06_Hospital` だけ）。ほかの案: 全回収の後も敵を残し `run_fast_2` で追わせる（本家に無い規則なので値が要る）。場所: 上の項目 7 の状態、実装記録 07 の「既知の制約」。

### 項目 4（敵ワサミの素体、2026-09-18 に閉じた記録 `20260917-enemy-wasami-body`）

- 2026-09-18: 敵の足の運びに合わせた再生の速さ（2026-09-19 に巡回が 200・追跡が 430 になり、`Walk` は 1.11・`Run` は 0.70 で上限・下限に当たらない。下の歩きの滑りは 350 のときの話） — 仮に `Walk` = 速さ / 133 を 0.5〜2 倍、`Run` = 速さ / 450・`Run_Nightmare` = 速さ / 500 を 0.6〜1.8 倍にした。PIE では追跡の走りはほぼ滑らない（1 %）が、巡回 350 cm/s の歩きは上限 2 倍で足が速さの 24 % 滑る（上限を 2.6 にすれば滑らないが、1 秒に約 5 歩のせかせかした歩きになる）。理由: 本家はスケートで速さ 1、範囲は WebGL 版の値、分母は PIE で測った足の速さ。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the play rate follows the speed`。
- 2026-09-18: 全回収後の追跡の走り `Run_Nightmare` への切り替え — 仮に走りと同じ 0.25 s のブレンドにした。理由: 本家の ABP に無い分岐。場所: `WasamiEnemyAnimInstance.h` の `TODO(仮): the original has one run`。
- 2026-09-18: 待機と見張りの待機のアニメ — 仮に `Idle_11`（直立）と `Idle_5`（足を開いた低い構え）にした。理由: Claude が v3 の中から選び、PIE で見て不自然さが無かった（`observations/ours/pie-enemy-idle-*.png`・`pie-enemy-alert-*.png`）。場所: `.claude/references/enemy-wasami-motions.md` の表、`dd_enemy.py` の `ROLES`。
- 2026-09-18: `Chase_VaultLand`（`Vault_and_Land`）の扱い — 元は「高さ約 76 cm の台の上から片手をついて跳び降りる」動きで、平らな廊下では宙から始まった。仮に、取り込みで台の高さの分を離れるまで下げて床から跳び越える形（見えない低い障害物を越える形。足は最高約 78 cm、骨盤は 130 cm）にし、着地後に立っているだけの区間を切り（2.4 s）、この 1 本だけ 31° 斜めに進むのを真っすぐに回した（体の向きは −7° で始まり 21° で終わる）。ほかの案: 追跡の候補から外して場面（項目 6 の Zone 1 の出来事）でだけ使う / そのまま流す。理由: ユーザーが追跡中の例に挙げた動きなので、候補に残す。場所: `dd_enemy.py` の `VAULT_FRAMES`・`_vault`、一覧の追跡中の変化の表、映像 `observations/ours/pie-enemy-once-Chase_VaultLand.mkv`・`-sheet.png`。
