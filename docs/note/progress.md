**最終更新: 2026-09-19**

『Dark Deception』のワサミ版ファンゲーム「WASAMI DECEPTION」を Unreal Engine 5 で作っています。いま遊べることを、実際のプレイ画面の GIF で紹介します。開発が進むたびに、この記事を書き換えます。

※ 非公式のファン作品です。原作のロゴとキャラクターのモデルは使いません。配布の予定はありません。

## いま出来ること

### 最初から最後まで通して遊べる

Zone 1 のエレベーターの到着から、敵ワサミに捕まってのやり直し、救急車で Zone 2 へ、祭壇でリングの欠片を取ってポータルから脱出するまで、ひと続きで遊べるようになりました（GIF は約 4 分の通しを 30 倍の速さにしたもの。シャードは確かめのための命令で大半を回収しています）。

![Zone 1 の到着からポータルでの脱出まで（30 倍速）](../../observations/ours/note/gif/23-playthrough.gif)

### タイトル画面から始まる

起動するとタイトル画面。NEW GAME で赤く明滅して暗転し、Zone 1 のエレベーターの到着から始まります。進み具合が残っているときは「RESTART?」と確かめてから最初に戻し、RESUME なら最後に通ったチェックポイントから続けられます。QUIT で終了（OPTIONS はまだ中身がありません。GIF は暗転している間を縮めています）。

![タイトル画面の NEW GAME から Zone 1 の到着へ](../../observations/ours/note/gif/25-title.gif)

### 病院を走ってシャードを集める

舞台は原作の病院「Torment Therapy」の Zone 1・Zone 2。集めるシャードは紫の結晶の代わりに「ワサミ餅」で、触れると紫の閃光とともに回収されます。Shift でダッシュ。

![廊下を走ってシャードを集める](../../observations/ours/note/gif/01-run-collect.gif)

### Zone 1: エレベーターの到着から駐車場へ

ゲームは Zone 1 のエレベーターが着くところから始まります。扉の前の鍵を F の連打で外して先へ進み、シャードを全部集めると駐車場への障壁が消えます。

![エレベーターの扉が開く](../../observations/ours/note/gif/11-elevator-arrive.gif)

駐車場の奥のトンネルへ向かうと、扉が閉ざされ、しばらくすると破られます。

![トンネルの扉が破られる](../../observations/ours/note/gif/12-doors-busted.gif)

### 救急車に乗って Zone 2 へ

ガレージのリフトで上がり、Teleport で救急車の屋根へ跳び移ると、救急車がトンネルを走り出し、Zone 2 へ運ばれます。

![リフトで上がり、Teleport で救急車の屋根へ](../../observations/ours/note/gif/13-garage-lift-teleport.gif)

![救急車でトンネルを抜けて Zone 2 の独房へ](../../observations/ours/note/gif/14-ambulance-zone2.gif)

### Zone 2: 独房から迷路へ

Zone 2 は独房から。天井の棘が下りてくる前に、扉の鍵を外して逃げ出します（GIF は確かめのための命令で鍵を一度に外しています）。

![独房の扉を破る](../../observations/ours/note/gif/15-cell-door.gif)

迷路の床は、乗ると上の階へ上がります。タブレットの地図も、いる階の絵に替わります。

![床に乗って上の階へ。地図が切り替わる](../../observations/ours/note/gif/16-lift-map.gif)

最後のシャードを取ると、目的が「COLLECT THE RING PIECE」に変わります。

![最後のシャードを取る](../../observations/ours/note/gif/17-last-shard.gif)

### 祭壇で欠片を取り、ポータルから脱出

シャードを全部集めたら、迷路の先の部屋の祭壇へ。使える物を見ると画面の真ん中に手のマークが出て、左クリックで使えます。祭壇を使うとリングの欠片が手に入り、ガレージへの障壁が消えて扉が開きます（シャードが残っているうちは、祭壇も障壁も使えません）。

![祭壇を使ってリングの欠片を取る](../../observations/ours/note/gif/21-ring-piece.gif)

ガレージに入ると、奥のトンネルの口のポータルが開きます。くぐると画面が暗くなって脱出です（GIF は確かめのための命令でポータルの前へ移っています）。

![開いたポータルをくぐって脱出](../../observations/ours/note/gif/22-escape-portal.gif)

### 脱出するとスコア画面

脱出すると赤い画面に「You Escaped!」が飛び込み、続いてリザルトが 1 行ずつ出ます。かかった時間・集めたシャード・失ったライフ・シャードの連続回収がそれぞれ S〜C で採点され、ボーナスのシャードを足した合計と、最後に総合のランクが出ます。NEXT を押すとタイトル画面に戻ります。死なずにシャードを取り続けると連続回収の札が出て、200 個と 500 個ではライフが 1 つ増えます。

![ポータルをくぐって You Escaped! からリザルトまで](../../observations/ours/note/gif/24-level-clear.gif)

### 敵ワサミに追われる

病院の中を敵ワサミが歩き回っています。前にいるところを見られると、走って追いかけてきます。一度見つかると、Vanish で姿を消すか、Primal Fear で気絶させるまで追ってきます。

![巡回していた敵ワサミが振り向き、走ってくる](../../observations/ours/note/gif/18-enemy-chase.gif)

Zone 2 の廊下では、高い所から敵ワサミが見張っています。見張りの視界に入ると、跳び降りて追ってきます。

![見張りの敵ワサミが跳び降りて追ってくる](../../observations/ours/note/gif/19-sentry-jump.gif)

### 敵ワサミに捕まると

敵ワサミに触れると捕まります。画面が真っ暗な部屋に切り替わり、揺れるカメラの間近で敵ワサミが 4 種の動きのどれかを見せつけ（1 種は奥から走り込んで顔が画面いっぱいに迫る）、暗転して死亡の画面へ。ライフが 1 つ減り、最後に通ったチェックポイントからやり直します（GIF は 2 種をつないだもの。確かめのためにライフを増やしています）。

![敵ワサミに捕まり、暗い部屋から死亡の画面へ](../../observations/ours/note/gif/20-capture.gif)

### タブレット

Space で出すと、残りのシャードの数と地図が見えます。Z で地図の縮尺を切り替え、1 / 2 で左右の枠のパワーを選びます。

![タブレット](../../observations/ours/note/gif/02-tablet.gif)

### パワー 6 種

原作のパワーを 6 つとも、最大の強化段階で使えます。Q / E で左右の枠のパワーを使います。

**Speed Boost** … しばらくのあいだ速く走れます。画面が赤く染まり、縁に集中線が流れます。

![Speed Boost](../../observations/ours/note/gif/03-speed-boost.gif)

**Teleport** … 床に照準の輪を出し、マウスのホイールで距離を変えて、クリックで瞬間移動します。

![Teleport](../../observations/ours/note/gif/04-teleport.gif)

**Telepathy** … 壁の向こうの敵に赤い煙のような印が付き、居場所が分かります。

![Telepathy](../../observations/ours/note/gif/05-telepathy.gif)

**Primal Fear** … 周りの敵を気絶させます。敵ワサミは倒れて 17 秒のあいだ動けなくなり、最後に寝返りを打って起き上がります（GIF は寝ている間を縮めています）。

![Primal Fear で敵ワサミを気絶させる](../../observations/ours/note/gif/06-primal-fear.gif)

**Telekinesis** … 周りのシャードをまとめて引き寄せます（GIF は 4 分の 1 の速さ）。

![Telekinesis](../../observations/ours/note/gif/07-telekinesis.gif)

**Vanish** … しばらくのあいだ敵から見えなくなります。画面が紫に沈み、縁に紫のもやが流れます。

![Vanish](../../observations/ours/note/gif/08-vanish.gif)

※ パワーの GIF の敵は、敵ワサミが自分で動けるようになる前に、確かめのために置いたり走らせたりしたものです。

### 死亡とチェックポイント

ライフは 3 つ。死ぬと残りのライフが 1 つ減る画面が出て、最後に通ったチェックポイントからやり直します。拾ったシャードは拾ったまま。チェックポイントを通ると、右下に「SAVING PROGRESS」と出て進み具合が保存されます。

![死亡の画面からチェックポイントで再開](../../observations/ours/note/gif/09-death-respawn.gif)

ライフが尽きると「YOU ARE DEAD」。最初からやり直す RESTART、最後のチェックポイントに戻る LAST CHECKPOINT、タイトル画面に戻る QUIT TO TITLE（タイトルの RESUME で最後のチェックポイントから続けられます）を選べます。LAST CHECKPOINT を選ぶと、S ランクが取れなくなる警告が一度だけ出ます。

![ゲームオーバーから LAST CHECKPOINT](../../observations/ours/note/gif/10-game-over.gif)

※ ゲームオーバーの GIF は、確かめのための命令でライフを減らしています。

## これから

オプションとポーズ、扉や罠・特殊なシャード・大きい敵ワサミ・場面・曲をそろえて、ゲームとして一通り遊べるようにします。
