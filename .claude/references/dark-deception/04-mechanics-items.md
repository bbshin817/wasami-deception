# ゲームの仕組みとアイテム

信頼度の表記は [README.md](README.md) を参照。

## 基本の流れ

1. 舞踏会場(Bierce's Ballroom)からポータルでステージ(悪夢)に入る。
2. 敵から逃げながら、迷路のソウルシャードを集める。ステージはゾーンに分かれ、ゾーンの必要数を集めると先へ進める。[C]
3. 全部集めると指輪の欠片を覆う障壁が消え、敵がフレンジーになる。[B]
4. 祭壇をクリックして指輪の欠片を取る。[C]
5. 出口のポータルまで逃げる。多くのステージは、出口ポータルへの大追跡で終わる。[C]

## ソウルシャード(Soul Shards)

- 見た目は大きく鋭い紫の結晶。[C]
- 罠の裏、塊、孤立した場所、ゾーンごとの分散など、意図的に配置されている。[C]
- 数は数百個単位。Elementary Evil は 283 個(Zone 1 は 127 個)、Stranger Sewers は 310 個、初期版の Monkey Business は 289 個。[C] / [B-]
- 触れると回収される。本作は仕様の指定で E キー回収に変えている(README の操作表)。
- **Shard Streak**: 死なずに集め続けると連続数が伸び、節目で残機がもらえる。[C] サウンドパックに `Shard_Streak_Milestone` がある。
- 累計の獲得数が XP になり、レベルが上がる(下の「レベル」)。[B]

## 特殊シャード

| 名前 | 見た目 | 効果 | 信頼度 |
| --- | --- | --- | --- |
| Enemy Reveal shard(Reveal Shard、Bonus Shard、Red Shard とも) | 赤いシャード | 敵の位置を 1 分間表示する。以前は死ぬまで続いたのを弱体化した。Malak は表示されない | [A](Steam のパッチノート "Reveal Enemies will now last 1 minute after collecting the red bonus shard") |
| Stun Orb | オレンジ(攻略記事では黄色)の球 | 敵を動けなくし、攻撃もできなくする。wiki では 15 秒。Malak には効かない | [C](15 秒と Malak の免疫は wiki 本文と一致) |

- 迷路のランダムな場所にあり、一時的に有利になる。[C]
- 初期版のレビューでは、敵を見るための特別な宝石は迷路に 1 つだけで、60 秒しかもたない。[C]
- [×] 「Trigger Teddies も Stun Orb が効かない」は否定された。wiki の Trigger Teddies のページには、気絶アニメはないが Stun Orb で気絶する、とある。
- Telekinesis では引き寄せられない(下の「パワー」)。[B]
- 原作データ(pak_reference、v1.6.1。本作の実装の根拠。[07-wasami-mapping.md](07-wasami-mapping.md)): Stun Orb は `BP_PowerOrb`、Reveal Shard は `BP_BonusShard`(Hotel のレベル上の名前は `BP_SpecialShard`)。硬直は行動ツリー `DD_BehaviorTree_Monkey` の Wait 15 s(wiki の 15 秒と一致。`UMG_VignetteSides` の既定の文言「ENEMIES STUNNED FOR 30 SECONDS」は、取得の処理が「ENEMIES STUNNED」に上書きするので出ない)、表示は Delay 60 s(パッチノートの 1 分と一致)。Hotel では赤いシャードが 100 s、オーブが 150 s ごとに、それぞれ 10 / 9 か所の出現点を移る(最初に現れるのはレベルの開始から 105 s / 155 s 後)。どちらも 1 個ずつで、ランダムな場所というより決まった出現点の中からランダムに選ばれる。

## 障壁(Barriers)

| 種類 | 解除条件 | 信頼度 |
| --- | --- | --- |
| 指輪の障壁(紫、祭壇を覆う) | そのステージの必要数のシャードを全部集める | [B] |
| 紫の障壁(エリアを区切る) | そのエリアのシャードを全部集めるか、ボスを倒す(広く言えば現在の目標の達成) | [B] |
| 赤い障壁(クモの巣状、"speed barrier") | Speed Boost の突進でしか壊せない | [B] |

- サウンドパックには `RingBarrierSuccess`(解除)と `RingBarrierDenied`(拒否)がある。本作は全回収と閉じた門への接近に使っている。

## 残機・チェックポイント・ランク

- 残機は 3。すべて失うと最後のチェックポイントからやり直す。初期版は迷路の最初からだった。[C]
- チェックポイントに入ると、画面右下で通知が点滅する。[C]
- 死んでもステージの最初に戻らずに済むチェックポイントは、更新で追加された。[B]
- サウンドパックには `Life_Lost`、`Level_Complete`、`UI_YouEscaped` がある。
- **S ランク**: 全シャードを集め、最後のチェックポイントを使わずにクリアすると S ランク。レベルアップでスキルポイントと、ずっと残る追加の残機がもらえる。[C]
  - ステージによっては細かい条件がある(Elementary Evil の例: 20 分以内、全 283 個、ボーナスシャード 2 個、秘密 4 つ、死亡 1 回以内、Streak 250)。[C]
  - 全ステージで S ランクを取ると、Ch5 の隠しステージ(Wicked Ways)が解放される。[C]
- 秘密(Secret)の部屋は、ミニマップの出っ張りの先にあるドアで見つかる。[C]

## レベルとスキルポイント

- ステージをクリアすると、累計のソウルシャード(XP)に応じてレベルが上がり、スキルポイントが増える。上限はレベル 15。[B](骨格は Steam のパッチノートでも確認: XP 付与の不具合修正、レベル 15 の上限)
- スキルポイントは舞踏会場の **Ring Altar**(Bierce の像)で使い、主にパワーの持続時間とクールダウンを強化する。[B]
- 後の更新で、振り分けたポイントを全額戻すリセットボタンが追加された。[A] / [B]

## パワー(Powers)

- 敵を倒すものはない。[A] / [B]
- 同時に 2 つ装備でき、タブレット画面の上半分にある 2 つの円で示される。PC では 1 / 2 で切り替え、Q / E で発動する。[B](検証で否定されたのは「Ch2〜5 で解放」の部分だけで、2 枠の仕様自体は Steam の議論でも一致)/ [C]
- 解放はステージ開始前に Ring Altar で行う。公開済みの 6 種はすべて Ch2〜Ch4 で解放される。[B]

| パワー | 解放(開始前) | 効果 | 信頼度 |
| --- | --- | --- | --- |
| Speed Boost | Elementary Evil | 大幅に加速し、赤い障壁を突き破れる。既定のレベル 2 で速度 870、持続 6.75 秒、クールダウン 8.5 秒。最大のレベル 6 で 950、9.75 秒、6.5 秒 | [B](wiki 値) |
| Teleportation | Deadly Decadence | 前方へ瞬間移動して敵や罠をすり抜ける。壁やドアは通れない。経路上のシャードを拾う。距離は強化で伸びる | [B] / [C] |
| Telepathy | Stranger Sewers | ゾーン内の全敵(Malak を含む)の位置を、壁越しに見える赤い煙で示す | [B] |
| Primal Fear | Crazy Carnevil | 近くの敵をすべて気絶させる衝撃波(持ち運べる Stun Orb)。半径約 35 m、クールダウン 23 秒(wiki 値)。Malak と Nightmare Mode の Agatha には効かない | [C] |
| Telekinesis | Torment Therapy | 青い範囲内の通常のシャードを引き寄せる。Red Shard や Stun Orb などの特殊シャードは対象外 | [B] |
| Vanish | Mascot Mayhem | 敵の視界と監視カメラから姿を消す。多くの敵は徘徊に戻り、Gold Watchers と Mama Bear は動かなくなる。ボス、Clown Cars、Malak には効かない | [B] / [C] |
| Summon(非公式名) | Prison Panic(Ch5、未公開) | 不明。Ch5 ではパワーが 2 つ追加され、Holiday Horror の後に解放されると開発者が述べている | [C] |

- Speed Boost のパワーは Ch2 の前に舞踏会場で受け取り、スピード障壁を越えるのに必須。[C]
- 本作で使っている効果音 `power_ready` / `power_not_ready` の元は、原作 UI の `power_refilled` / `power_not_ready`。

## タブレット(Tablet)

- 手に持つ端末で、周辺の迷路のマップを表示する。[C]
- 残りのシャードが一定数を切ると、残りのシャードと目標の方向を指す探知機が働く。[A](Steam の告知 "Map now has a detector that points to remaining shards and level objectives once the shard count is reduced a certain amount.")
- マップのズームは 1.4 で M キーとして追加され、後に Z キーへ移った。[A] / [B]
- パワーのクールダウンとタイマーを表示する。[B]
- 敵は表示しない。敵を見るには Reveal Shard か Telepathy が要る。[C]
- パワーの 2 枠(上半分の 2 つの円)。[B]

## 操作(PC の既定)

| 入力 | 動作 | 信頼度 |
| --- | --- | --- |
| WASD / マウス | 移動 / 視点 | [C] |
| Shift 長押し | ダッシュ(2014 年のデモから) | [C] |
| 中クリック | 180 度のクイックターン | [B] |
| Q / E | パワーの発動 | [C] |
| 1 / 2 | パワーの切り替え | [C] |
| Z | マップのズーム(1.4 では M) | [B] |
| Space | タブレットの出し入れ | 本作 README の出典(Tablet の wiki ページ)による。今回は未検証 |

- **Adaptive FOV**: 移動速度に応じて視野角が変わる。[B]

## 難易度

- Easy Mode がある。Easy では Agatha がかなり遅い。[C]
- 2015 年の開発版では、ステージの霧の濃さで難易度を変えていた(製品版での有無は不明)。[C]
