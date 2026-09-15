# 敵と AI の挙動

信頼度の表記は [README.md](README.md) を参照。
ここの記述はすべて、プレイヤーが観察した挙動(wiki、TV Tropes、攻略記事)に基づく。開発元の実装(UE4 のビヘイビアツリーなど)を示す一次資料は見つかっていない。

## 敵に共通する性質

- 倒せない。気絶・回避・減速・凍結だけができる。[A]
- ほとんどの敵は Doug の正確な位置を知らず、見つけたら追う。[C]
- Vanish を使うと、多くの通常の敵は "wandering"(徘徊)状態に戻る。例は Murder Monkeys、Agatha、Dread Duckies、Clown Gremlins、Reaper Nurses、Lucky the Rabbit、Penny the Chicken、Hangry the Pig、Trigger Teddies。Gold Watchers と Mama Bear は効果が切れるまで動かなくなる。[C]
- 全シャードを集めるとフレンジー(Nightmare Mode)になる。目が赤くなり、配色が変わり、ずっと攻撃的になる。例外や名称の扱いは [05-presentation.md](05-presentation.md)。[B]
- 捕まると即座にキル演出。通常の敵はキルアニメが 3〜4 種類あり、Reaper Nurses、Penny the Chicken、Trigger Teddies は 1 種類だけ。[C]
- 気絶画面は通常は黄色で、Reaper Nurses だけは危険度を示すため赤。[C]

## 敵ごとの挙動

### Murder Monkeys(Monkey Business)
- **見た目**: 両手がナイフの巨大なぜんまい仕掛けの猿のおもちゃ。シンバルを叩く猿のおもちゃを、赤い制服のベルボーイにした姿。[C]
- **出現**: Doug が板を壊してホテルの廊下に入ると、南西・南東・北東の隅に 3 体。1.1.2 で AI を改善したのに合わせて 4 体から減らした。[B]
- **探知**: ホテル内では常に Doug の位置がわかる(ある程度テレパシーのように)。ただし遠く離れると見失うことがあり、Vanish で追跡が切れる。見失ってから歩き出す。秘密の部屋には入れず、外で待つ。[C]
- **追跡**: 常に最短経路で追う。近づくと少し加速する。全力で走る Doug には追いつけないが、散らばった状態だと角で待ち伏せたり挟み撃ちにしたりする(1.1.2 で散開する "smart AI" になった)。[B]
- **弱点**: 振り向くのが苦手。止まって「グリッチ」してから方向を変えるので、挟まれたら Teleportation ですり抜けられる。[B]
- **音**: うるさい足音と甲高い息づかいで接近がわかる。1.1.2 で、近づくと方向のわかる攻撃音を出すようになった。動いている間はぜんまいの機械音がループする。声優はおらず、叫び声は映画 Gremlins の音声で、ジャンプスケアでも同じ叫びが鳴る。[B] / [C]
- **全回収後**: Enhanced 版以降は消え、Chef Monkeys が代わりに追う。欠片を取った後は、エレベーターからあふれた猿の群れがロビーを追ってくる。[B]
- [×] 「常にマップ上の位置を知っていて、パワーなしで避けられる唯一の敵」は否定された(Ch1 にはそもそもパワーがない)。

### Agatha(Elementary Evil)
- **背景**: メイドの娘で、Bierce が Malak を召喚する生贄にした。Nightmare Realms で Malak の養女になり、指輪の欠片の番をしている。学校("Agatha Elementary")に取りつき、Malak を "Daddy" と呼ぶ。声は Kat Cressida。[C]
- **探知**: Doug の正確な位置は常には知らないが、校内のどのあたりにいるかは常にわかっている。Doug が開け閉めしたドアの音は距離に関係なくすべて聞きつけ、調べに行く。呼吸音も聞き取る。[B]
- **巡回**: ゆっくり廊下を巡回し、Doug をあざけりながら自分の居場所を口にする。[C]
- **追跡**: Doug を見つけると、全力疾走より速く走るので、逃げるには Speed Boost が要る。見失うと追うのをやめて巡回に戻る。Easy Mode ではかなり遅い。[C]
- **テレポート**: 閉じたドアはテレポートで通り抜ける。Zone 1 からテレポートするが、Zone 2 ではドアへの反応が増え、テレポートの頻度と距離も増える。長く追うと、特に角や部屋で Doug の先へテレポートして行く手をふさぐ。[B]
  - テレポートには音の合図があり、紫の火花と黒い霧として見える。[C]
- **弱点**: ドアを閉めると、テレポートで通り抜けるぶん遅れる。廊下の中央を通るので、角では壁ぎわを回ると距離を稼げる。[C]
- **Nightmare Mode**: 目が赤く光り、肌が灰色になる。無敵になって Primal Fear が効かない。それ以外の姿では、ほかの多くの敵と同じく気絶させられる。[C]
- [×] 「ステージ唯一の敵でボスでもあり、隠れても長くはもたない」は否定された。

### Reaper Nurses と Matron(Torment Therapy)
- 病院を上司の Matron と守る。[C]
- **隠密**: Zone 1 で探している間は透明になっている。Doug に近づくか、何かに邪魔されたときだけ姿を見せる。遠くから見つけるには Reveal Shard か Telepathy を使うか、隠せない音とスケートの跡を頼りにする。[C]
- **追跡**: 正確な位置は知らず、見つけたら追う。Agatha と同じく、Doug が開けたドアへ向かう。一度見つけると追い続け、速い(疾走する救急車に追いつくほど)。逃げるには気絶させるか Vanish を使うか、Teleport と Speed Boost をつなげる。近距離では、麻痺ガスを出す気絶薬を投げる。[C]
- **キル演出**: 巨大な注射器で刺す 1 種類だけ。[C]
- 全シャード回収の前(Zone 2)に Nightmare Mode になり、回収前でも殺しに来る最初の敵(2 番目は Trigger Teddies)。[C]

### Lucky the Rabbit と Joy Joy Gang(Mascot Mayhem)
- Joy Joy Gang は Lucky the Rabbit、Penny the Chicken、Hangry the Pig の 3 体のマスコット(アニマトロニクス)。[C]
- **Lucky の追跡**: 25 秒ごとに、Doug のものと同じような Speed Boost を短時間使って追いつく。プレイヤーの Speed Boost の強化が低いうちは、それより速い。「25 秒」は wiki の値で、Zone 1 での挙動。Lucky 自身の速度の数値はどこにもない。[B]
- **外部センサー**: 敵は自分の目に加えて、壁の監視カメラでも Doug を見つける。カメラは見つけると大きな警報を鳴らし、Joy Joy Gang に位置を知らせる。Vanish でカメラにもアニマトロニクスにも見つからずに進める。[B]
- **風船のクローン**: 一部の Lucky のクローンは風船に乗って上から探す。視野が広く、見つけるとゆっくり滑空して近づき、周囲に爆弾を落とす。地上のクローンと違って気絶しない。[C]
- Zone 3 で感電し、全回収の前に Nightmare Mode になる。[C]
- 圧砕機で潰された後、Joy Kill として再構成される。[B]
- 別作品 Super Dark Deception の Lucky(Bunny Barrage、5 秒)とは別物。[B]

### そのほかの敵(情報が少ないもの)
| 敵 | わかっていること | 信頼度 |
| --- | --- | --- |
| Chef Monkeys | Monkey Business で全回収後に Murder Monkeys と入れ替わる(Enhanced 版以降) | [B] |
| Gold Watchers | 更新で、プレイヤーの視界に入っている間は動かないように変わった。Vanish の間は動かない。全回収時に変身せず消える側の例として挙がっている | [C] |
| Dread Duckies / Doom Ducky | Stranger Sewers の敵。Dread Duckies は大きく、Telepathy で見つけるのが有効 | [C] |
| Clown Gremlins | Primal Fear が有効。Crazy Carnevil のボス戦で群れになり、ハンマーで対処する | [C] |
| Goliath Clowns | Crazy Carnevil のボス戦に 4 体 | [C] |
| Clown Cars | Nightmare Mode にならない(Malak とボスを除けば最初の例)。Vanish が効かない | [B] / [C] |
| Trigger Teddies | Ch4 の敵。Teddies が指輪の欠片を運ぶゾーンがある。気絶アニメがないが、Stun Orb では気絶する。キルアニメは 1 種類 | [B] / [C] |
| Mama Bear | Vanish の間は動かない | [C] |
| Malak | 多くのステージで全回収後に現れる。Stun Orb、Reveal Shard、Primal Fear、Vanish が効かず、Telepathy では見える | [C] / [B] |

## AI 設計パターン(観察記述からの抽出)

上の観察を、実装に使える形に並べ直したもの。事実の出典は上の各節で、ここでの分類は本資料の整理 [推測]。

### 知覚(どうやってプレイヤーを見つけるか)
| パターン | 例 | 手触り |
| --- | --- | --- |
| 全知 + 最短経路 | Murder Monkeys | 常に迫られる圧。位置取りと逃げ道の読み合いになる |
| 音(ドア・呼吸)+ 大まかなエリア | Agatha、Reaper Nurses | プレイヤーの行動(ドアの開閉)が見つかる危険になる |
| 視覚 + 外部センサー(監視カメラと警報) | Lucky / Joy Joy Gang | 監視網を避けるステルスになる |
| 透明化して、近づくと姿を見せる | Reaper Nurses(Zone 1) | 音と痕跡を頼りにした緊張 |
| 見られている間は止まる | Gold Watchers | 視線の管理(しだれ天使型) |
| 上空からの広い視野 | Lucky の風船クローン | 開けた場所の危険 |

### 追跡の強さの調整
- 接近時に少し加速するが、全力疾走には追いつかない(Monkeys)。逃げ切れる余地を残す。
- 周期的なブースト(Lucky、25 秒ごと)。プレイヤーの成長(Speed Boost の強化)で相対的に弱くなる。
- 先回りのテレポート(Agatha)。追跡が長引くほど、角や部屋で先回りが増える。
- 疾走より速い(Agatha、Reaper Nurses)。パワーを使わないと逃げられない局面を作る。
- ゾーンや段階で強くなる(Agatha の Zone 2、早めの Nightmare Mode)。

### 逃げ道になる弱点
- 振り向きが遅い(Monkeys の停止と「グリッチ」)。
- ドアの通過に時間がかかる(Agatha のテレポート)。
- 廊下の中央を通る(角の内側で差がつく)。
- 見失うと巡回に戻る(Agatha)。秘密の部屋に入れない(Monkeys)。

### 状態の流れ
徘徊・巡回(wandering)→ 発見・追跡 → 見失い → 巡回。これに割り込むのが、気絶(Stun Orb、Primal Fear)、Vanish(徘徊へ戻す、または凍結)、フレンジー(全回収で段階的に強くなる)。

### 手がかりの設計
接近は音で知らせる。足音、息づかい、ぜんまい音、方向のわかる攻撃音、テレポート音、あざけりの声、警報、スケートの跡。詳しくは [05-presentation.md](05-presentation.md)。

### 実装上の注意(開発者の証言)
- テレポート系の移動は抜け道を作りやすく、実装が最も難しかった(Teleport パワー)。[C]
- 壁越しに敵を表示する処理(旧 Telepathy)は、フレーム落ちの原因になった。[C]
