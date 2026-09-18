**最終更新: 2026-09-18**

『Dark Deception』のワサミ版ファンゲーム「WASAMI DECEPTION」を Unreal Engine 5 で作っています。いま遊べることを、実際のプレイ画面の GIF で紹介します。開発が進むたびに、この記事を書き換えます。

※ 非公式のファン作品です。原作のロゴとキャラクターのモデルは使いません。配布の予定はありません。

## いま出来ること

### 病院を走ってシャードを集める

舞台は原作の病院「Torment Therapy」の Zone 1・Zone 2。集めるシャードは紫の結晶の代わりに「ワサミ餅」で、触れると紫の閃光とともに回収されます。Shift でダッシュ。

![廊下を走ってシャードを集める](../../observations/ours/note/gif/01-run-collect.gif)

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

**Primal Fear** … 周りの敵を気絶させます。敵ワサミは 17 秒のあいだ、ふらついて動けなくなります。

![Primal Fear で敵ワサミを気絶させる](../../observations/ours/note/gif/06-primal-fear.gif)

**Telekinesis** … 周りのシャードをまとめて引き寄せます（GIF は 4 分の 1 の速さ）。

![Telekinesis](../../observations/ours/note/gif/07-telekinesis.gif)

**Vanish** … しばらくのあいだ敵から見えなくなります。画面が紫に沈み、縁に紫のもやが流れます。

![Vanish](../../observations/ours/note/gif/08-vanish.gif)

※ 敵ワサミはまだ自分では巡回も追跡もしません。GIF の敵は、確かめのために置いたり走らせたりしたものです。

## これから

敵ワサミの巡回と追跡、捕まったときの演出、死亡とセーブ、罠、脱出のポータルを作っていきます。
