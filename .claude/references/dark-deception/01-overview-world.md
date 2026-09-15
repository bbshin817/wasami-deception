# 作品概要と世界観

信頼度の表記は [README.md](README.md) を参照。

## 作品の位置づけ

- 公式の説明: "Dark Deception is a story driven first-person horror action maze game that mixes the fast-paced style of classic arcade games with fun horror game design." — ストーリー主導の一人称ホラーアクション迷路ゲーム。クラシックなアーケードの速いテンポとホラーを組み合わせる。[A]
- 基本ルール: "Run for your life and run fast. Enemies can be stunned and avoided, but not killed." — 敵は気絶させたり避けたりはできるが、倒せない。プレイヤーは基本的に逃げる。[A]
  - パワーや特殊シャードの気絶・減速・凍結は、どれも敵を倒さない。ボス戦も周回や回避で生き延びる形式。[B]
  - 物語上の例外として、Mascot Mayhem では Joy Joy Gang が圧砕機で潰された後、Joy Kill として再構成される。[B]
- 迷路には罠や危険物がある。流血やゴアの表現はなく、独自のブラックユーモアが作風の特徴。[A′]
- ストアの文言 "Every nightmare presents a unique creature that has its own distinct AI"(悪夢ごとに固有の敵と AI)は宣伝文で、実際は複数のステージに出る敵(Malak など)や、1 ステージに複数種いる例がある。文字どおりの仕様としては使わない。[×]
- 発想の元は Pac-Man(ドットがソウルシャードに、ゴーストが怪物に置き換わった形)。2015 年のインタビューで作者は "VR first person horror Pac-Man" と表現している。[C]
- 販売形態: Chapter 1 は無料で、Chapter 2 以降は個別の DLC として販売。[A′]

## 開発の経緯とエンジン

| 時期 | 出来事 | 信頼度 |
| --- | --- | --- |
| 2013 | Vince Livings が Glowstick Games, LLC を共同設立し、開発を始める | [C] |
| 2014 | 無料デモ(Windows/Mac)。**Unity 製**、クレジットは Glowstick Games。ホテルの迷路で紫のシャードを集め、3 回失敗で終わり。WASD 移動、Shift 長押しでダッシュ | [C] |
| 2015-01 | 第 2 回 Kickstarter。インタビューで "We are building Dark Deception in Unity." と発言。当初は 2 人チームのモバイル向けとして考え、VR(Oculus Rift)前提で設計していた。戦闘、RPG 要素、手続き生成レベル、10〜20 時間のボリュームを構想。霧の濃さで難易度を変える仕組みがあり、最大では約 4 フィート(約 1.2 m)先しか見えなかった | [C] |
| 2015 | 技術担当の共同設立者が抜け、Glowstick Games を閉鎖 | [C] |
| 2016 | Glowstick Entertainment, Inc. を設立。HTC の ViveX Batch I に参加して出資を受ける | [C] |
| 2017 | Nikson と Daniel Dombrowsky の参加に合わせて、**Unreal Engine で作り直す** | [C] |
| 2018-09-26 | Steam で発売(Chapter 1)。wiki は 9/27 と記載 | [A] |
| 2019-01-21 | Chapter 2(Steam の DLC ページの日付。wiki は 1/22) | [A] |
| 2019-06-24 | Chapter 3 | [A] |
| 2021-09-28 | Chapter 4 と、見た目とレベルデザインを更新した Enhanced 版 | [A] |
| 未定 | Chapter 5 は発表済みで、Steam ページ(app 1019932)は "Coming soon"。発売日は未定。TGS 2023 のデモが告知されていた | [A] / [C] |

- **エンジン**: Fandom の作品ページの infobox は Unreal Engine 4 と記載している。検証では「開発元とエンジンは正しい」とされたが、主張全体は日付の誤りで否定された。一次資料(開発元の発表など)は未確認なので、厳密には「UE4 製とされる」と書く。[B 相当]
- 実装面の証言(wiki が開発者の発言として載せているもの):
  - Nikson は、Teleport がマップの抜け道(exploit)を多く生み、最も実装が難しいパワーだったとして、追加を後悔していると述べた。[C]
  - Vince は、旧版の Telepathy で大きなフレーム落ちが起きていたと述べた。[C]
- Vince Livings は Glowstick Entertainment の代表で、Dark Deception の主な制作者。毎月の Q&A 配信で情報を発表している。[C]

## 世界観

- **Nightmare Realms**(悪夢の領域): 物語の主な舞台。負の魂が集まり、究極の悪 "Demon God" の座を争う場所で、その一部を悪魔 Malak が支配する。旧称は "Dark Dimension"。[C]
- Bierce's Ballroom から見ると、浮かぶ岩が点在する果てしない闇。各悪夢(= ステージ)はポータルでつながり、関わった人物の罪や欠点に応じた形になる。[C]
- wiki が挙げる Nightmare Realms 内の場所: Bierce's Ballroom、The Hotel、The School、The Manor、The Sewer、The Circus、The Hospital、The Theme Park、The Cave、The Mall、The Prison。[C]
- **Soul Shards**(ソウルシャード): Malak の犠牲者の魂の残滓。怪物たちとその主 Malak が守っている。[B]
  - 人の魂でできていて、苦しんで叫んでいるらしいことが示唆されている。Bearly Buried では、Malak に送られて処刑された者が苦悶の顔として洞窟の壁に取り込まれている。[C]
- **Riddle of Heaven**: Malak が持っていた指輪で、欠片が各悪夢に散らばっている。欠片は Malak がシャードの力で作った障壁に覆われ、悪夢ごとの必要数のシャードを集めると障壁が消えて取れる。[B]
- 物語の流れ: 悪魔の儀式を行った Doug Houser が、煉獄のような舞踏会場で目覚める。Bierce は「生者の世界へ戻る方法」として、ポータルの先でシャードを集めるよう求める。[B]
- Bierce の本当の目的は指輪の欠片で、純粋な善意の案内人ではない。[B]
- Mascot Mayhem の最後で Doug は指輪の欠片をすべて奪われ、欠片に結びついたパワーも失う。[C](ネタバレ)

## 主要人物

### Doug Houser(主人公)
- 元刑事弁護士。経緯は不明のまま煉獄のような Nightmare Realms に入り、Bierce から「生き直す機会」と引き換えに指輪の欠片集めを持ちかけられる。[C]
- ゲーム中はほとんど顔を見せない。回想を除くと、プレイ中にモデルが映るのは舞踏会場のカットシーン 1 回だけ。[C]
- 声は Christopher Corey Smith。プレアルファのデモでは警官で、残機が警察バッジで表示されていた。[C]

### Bierce
- 舞踏会場の主で、優雅な装いの女性。Doug をポータルへ送り出す案内役。[B]
- 辛辣で皮肉屋。レビューでは Elvira にたとえられている。2014 年のデモでは、英国なまりの女性の声(Carolyn Seymour)がプレイヤーをあざけっていた。[C]
- メイドの娘 Agatha を生贄にして Malak を召喚した。[C]
- ゲーム内の台詞の例(Monkey Business): "You've gathered all of the soul shards. The ring piece is now exposed, and the monsters will go into a frenzy." [B](検証者が wiki の転載で確認)

### Malak
- 悪魔の王(demon lord)。Nightmare Realms の一部を支配し、指輪の持ち主として Bierce と敵対する。[C]
- 多くのステージで、シャードを全部集めた後に現れる。ゲームが進むほど危険になる。[C]
- Stun Orb、Reveal Shard、Primal Fear は効かない。Telepathy では位置がわかる。Vanish は効かない。[C] / [B]
- 全シャード回収後の変身(フレンジー)をしない側の敵。[C]
