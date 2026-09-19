**最終更新: 2026-09-19**

『Dark Deception』のワサミ版ファンゲーム「WASAMI DECEPTION」（Unreal Engine 5 で制作中）の、敵ワサミのモーションの一覧です。モーションごとに、名前・長さ・ゲームの中でいつ使う予定かを GIF と一緒に載せます。

※ 非公式のファン作品です。原作のロゴとキャラクターのモデルは使いません。配布の予定はありません。

※ GIF はゲームの画面ではなく、敵ワサミのモデルのファイルを Blender で描いたものです。速さは実際のままで、床の升目は 1 m 角です。使いどころは予定で、ゲームの中で見て変えることがあります。

## 早見

- **Idle_11** … ふだんの待機
- **Idle_5** … 見張りの待機（Zone 2）
- **Walking** … 巡回
- **Running** … プレイヤーを追いかけるとき。捕まったときに顔を間近に見せる演出にも使う
- **run_fast_2** … シャードを全部集めた後に追いかけるとき
- **BeHit_FlyUp・Knock_Down** … 気絶して倒れる（2 本から 1 本をランダム）
- **push_up_to_idle** … 気絶から起き上がる（前に寝返りを足す）
- **Female_Run_Forward_Pick_Up_Right・Male_Head_Down_Charge・Parkour_Vault_with_Roll・Vault_and_Land・run_fast_5・slide_right** … 追いかけている間に、ときどき挟む動き
- **Backflip・sliding_rool・Stylish_Walk** … 捕まったときの演出（この 3 本と、Running で顔を間近に見せる 1 本の、4 本から 1 本）
- **01a0a88f-0db6-7251-85e6-1a97b799ee52** … 使わない（以前の気絶）
- **restpose** … 使わない（腕を広げた基準の姿勢）

## 待機と移動

### Idle_11（待機）

ふだん立っているとき。まっすぐ立ち、頭を少しうつむけます。2.0 秒のループ。

![Idle_11](../../observations/ours/note/enemy/01-idle.gif)

### Idle_5（見張りの待機）

Zone 2 で見張りをしている 6 体。足を前後に開いた低い構えです。2.0 秒のループ。

![Idle_5](../../observations/ours/note/enemy/02-idle-alert.gif)

### Walking（巡回）

決まった道を歩いて見回るとき（秒速 3.5 m）。1.1 秒のループ。

![Walking](../../observations/ours/note/enemy/03-walk.gif)

### Running（追跡）

プレイヤーを見つけて追いかけるとき（秒速 4.3 m）。0.7 秒のループ。捕まったときの演出の 1 本では、黒い背景の奥からこの走りで一気に寄って来て、顔が画面いっぱいになります。

![Running](../../observations/ours/note/enemy/04-run.gif)

### run_fast_2（シャードを全部集めた後の追跡）

シャードを全部集めると、追いかけるときの走りがこれに変わります。0.7 秒のループ。GIF では前へ進みますが、ゲームではその場で走る形に直し、進むのは体の移動で出します。

![run_fast_2](../../observations/ours/note/enemy/05-run-nightmare.gif)

## 気絶

Primal Fear やスタンオーブで気絶すると、倒れる 2 本のどちらかをランダムに流し、あおむけのまま寝ています。気絶の 17 秒が終わるころに寝返りを打ってうつぶせになり、push_up_to_idle で起き上がって、17 秒ちょうどで待機の姿勢に戻ります。

### BeHit_FlyUp（倒れる）

打たれて高く宙へ飛ばされ、あおむけに倒れる。1.6 秒（GIF はほかより少し引いて描きました）。

![BeHit_FlyUp](../../observations/ours/note/enemy/16-behit-flyup.gif)

### Knock_Down（倒れる）

後ろへ吹き飛ばされて倒れる。2.6 秒。

![Knock_Down](../../observations/ours/note/enemy/17-knock-down.gif)

### push_up_to_idle（起き上がる）

床から起き上がり、待機の姿勢に戻る。3.2 秒。うつぶせから始まる動きなので、ゲームでは前に、あおむけから体を半回転させてうつぶせになる寝返り（0.8 秒。元のモーションには無く、こちらで作りました）を足して、あわせて 3.9 秒で流します。GIF は元の 3.2 秒だけです。

![push_up_to_idle](../../observations/ours/note/enemy/18-push-up-to-idle.gif)

### ゲームでの気絶の流れ

倒れる → 寝る → 寝返り → 起き上がる を、ゲームと同じつなぎ方で描いたものです。ゲームでは倒れてから 10〜11 秒ほど寝ていますが、GIF では寝ている時間を 1 秒に縮めました。

BeHit_FlyUp で倒れたとき:

![BeHit_FlyUp で倒れてから起き上がるまで](../../observations/ours/note/enemy/19-stun-flyup.gif)

Knock_Down で倒れたとき:

![Knock_Down で倒れてから起き上がるまで](../../observations/ours/note/enemy/20-stun-knock-down.gif)

## 追いかけている間にときどき挟む動き

Zone 1・2 でプレイヤーを追いかけている間、6〜10 秒ごと（平均 8 秒）に下の 6 本から 1 本をランダムに流します。直前と同じものは避け、前がその動きの分だけ空いているときだけ流します。流している間も秒速 8 m で追いかけ続け、捕まえることも気絶することもあります。

GIF では前へ進みますが、ゲームではその場で動く形に直し、秒速 8 m に合わせて早回しで流します。

### Female_Run_Forward_Pick_Up_Right

走りながら右手で何かを拾う。1.2 秒。

![Female_Run_Forward_Pick_Up_Right](../../observations/ours/note/enemy/07-chase-pickup.gif)

### Male_Head_Down_Charge

頭を下げて突進する。0.5 秒。

![Male_Head_Down_Charge](../../observations/ours/note/enemy/08-chase-charge.gif)

### Parkour_Vault_with_Roll

飛び越えて前へ転がる。2.1 秒。

![Parkour_Vault_with_Roll](../../observations/ours/note/enemy/09-chase-vault-roll.gif)

### Vault_and_Land

元は、高さ約 77 cm の台の上から、縁の手すりに片手をついて跳び越え、床へ降りる動きです（GIF は台と手すりを足して描きました）。3.1 秒。ゲームの廊下は平らなので、床から跳んで床へ降りる形に直し、斜めに進むのを真っすぐにして、着地して起き上がったところ（2.4 秒）で切って使います。

![Vault_and_Land](../../observations/ours/note/enemy/10-chase-vault-land.gif)

### run_fast_5

別の走り。1.8 秒。

![run_fast_5](../../observations/ours/note/enemy/11-chase-run-fast.gif)

### slide_right

スライディング。1.8 秒。

![slide_right](../../observations/ours/note/enemy/12-chase-slide.gif)

## 捕まったときの演出

プレイヤーが捕まると、黒い背景の中で敵ワサミがこの 3 本か、Running で顔を間近に見せる 1 本の、4 本のどれかを見せます（順番はシャッフル）。3 本は以前のモデルにあった動きで、今のモデルの骨に載せ替えて使います。ゲームの中では最初は速く、だんだん等速に戻す速さで流し、sliding_rool と Stylish_Walk は遠くから寄って来る出だしを飛ばします。

### Backflip

バク転。2.1 秒。

![Backflip](../../observations/ours/note/enemy/13-capture-backflip.gif)

### sliding_rool

床へ滑り込んで転がり、起き上がる。2.8 秒。

![sliding_rool](../../observations/ours/note/enemy/14-capture-sliding-roll.gif)

### Stylish_Walk

気取った歩き。3.5 秒。

![Stylish_Walk](../../observations/ours/note/enemy/15-capture-stylish-walk.gif)

## 使わない動き

### 01a0a88f-0db6-7251-85e6-1a97b799ee52

名前が文字の並びだけのモーションです。前かがみでふらつき、やがて体を起こして上を仰ぎます。10.0 秒。以前は気絶に使っていましたが、上の「倒れて、寝返りを打って起き上がる」流れに替えました。

![01a0a88f-0db6-7251-85e6-1a97b799ee52](../../observations/ours/note/enemy/06-stun.gif)

## 場面での使い方（仮）

原作では場面ごとに専用の演技がありますが、敵ワサミには無いので、上のモーションで代わりをします。

- **Zone 1 の途中で 2 体が現れて去る場面** … 構える → Idle_5、跳んで去る → Parkour_Vault_with_Roll か Vault_and_Land の後に Running
- **Zone 2 の始まりで捕まる場面** … 待ち構える → Idle_5、殴る → Male_Head_Down_Charge
- **Zone 2 の独房の場面** … 待つ・話す → Idle_11、後ずさる → Walking の逆再生、消える → Walking で去る
