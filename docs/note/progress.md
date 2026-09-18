**最終更新: 2026-09-19**

『Dark Deception』のワサミ版ファンゲーム「WASAMI DECEPTION」を Unreal Engine 5 で作っています。いま遊べることを、実際のプレイ画面の GIF で紹介します。開発が進むたびに、この記事を書き換えます。

※ 非公式のファン作品です。原作のロゴとキャラクターのモデルは使いません。配布の予定はありません。

## いま出来ること

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

※ 敵ワサミはまだ自分では巡回も追跡もしません。GIF の敵は、確かめのために置いたり走らせたりしたものです。

### 死亡とチェックポイント

ライフは 3 つ。死ぬと残りのライフが 1 つ減る画面が出て、最後に通ったチェックポイントからやり直します。拾ったシャードは拾ったまま。チェックポイントを通ると、右下に「SAVING PROGRESS」と出て進み具合が保存されます。

![死亡の画面からチェックポイントで再開](../../observations/ours/note/gif/09-death-respawn.gif)

ライフが尽きると「YOU ARE DEAD」。最初からやり直す RESTART、最後のチェックポイントに戻る LAST CHECKPOINT、QUIT TO TITLE（タイトル画面はまだ無いので、今は Zone 1 の最初に戻ります）を選べます。LAST CHECKPOINT を選ぶと、S ランクが取れなくなる警告が一度だけ出ます。

![ゲームオーバーから LAST CHECKPOINT](../../observations/ours/note/gif/10-game-over.gif)

※ まだ敵に捕まる演出が無いので、GIF は確かめのための命令で死なせています。

## これから

敵ワサミの巡回と追跡、捕まったときの演出、Zone 2 の祭壇と脱出のポータルを作っていきます。
