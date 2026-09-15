# 出典・否定された主張・未解決の問い

2026-09-11 の deep-research(Run `wf_3afddf04-c43`)の結果。

## 出典

| URL | 種類 | 主に使った内容 |
| --- | --- | --- |
| https://store.steampowered.com/app/332950/Dark_Deception/ | 一次(開発元のストアページ) | 作品の説明、基本ルール、作風、発売日、DLC 構成 |
| Steam のニュースとパッチノート(app 332950、GetNewsForApp。検証者が参照) | 一次 | タブレットの探知機、マップズーム(1.4)、Reveal の 1 分、Ring Altar のリセット、レベル 15 の上限、Ch4 と Enhanced の日付 |
| Steam の DLC ページ(Ch2 app 1017030、Ch3 app 1019930、Ch4 app 1019931、Ch5 app 1019932。検証者が参照) | 一次 | 各チャプターの発売日、Ch5 の "Coming soon" |
| https://dark-deception-game.fandom.com/wiki/Dark_Deception | 二次(ファン wiki) | 物語、チャプター構成、更新履歴、エンジン(infobox) |
| https://dark-deception-game.fandom.com/wiki/Level | 二次(Stub) | レベル、上限 15、S ランクの獲得シャード |
| https://dark-deception-game.fandom.com/wiki/Soul_Shards | 二次 | シャードの設定、特殊シャード、フレンジー、S ランク |
| https://dark-deception-game.fandom.com/wiki/Powers | 二次 | パワーの一覧、解放順、効果、Speed Boost の値、開発者の発言 |
| https://dark-deception-game.fandom.com/wiki/Murder_Monkeys | 二次 | Murder Monkeys の AI、音、見た目 |
| https://dark-deception-game.fandom.com/wiki/Agatha | 二次 | Agatha の AI、背景 |
| https://dark-deception-game.fandom.com/wiki/Reaper_Nurses | 二次 | Reaper Nurses の AI、キル演出 |
| https://dark-deception-game.fandom.com/wiki/Lucky_the_Rabbit | 二次 | Lucky の AI、監視カメラ、クローン |
| https://dark-deception-game.fandom.com/wiki/Doug_Houser | 二次 | 主人公、パワー、ボス戦 |
| https://dark-deception-game.fandom.com/wiki/Nightmare_Realms | 二次 | 世界観、場所、住人 |
| https://dark-deception-game.fandom.com/wiki/Vince_Livings | 二次 | 開発の経緯、Unreal Engine への移行 |
| https://tvtropes.org/pmwiki/pmwiki.php/Characters/DarkDeception | 二次 | 敵の性質(一部は否定された) |
| https://tvtropes.org/pmwiki/pmwiki.php/VideoGame/DarkDeception | 二次 | 作品の特徴、脱出の大追跡、最初のジャンプスケア、頭韻 |
| https://cliqist.com/2015/01/14/dark-deception-interview/ | 二次(2015 年のインタビュー) | Unity での開発、Pac-Man の発想、当初の構想 |
| https://jayisgames.com/review/dark-deception.php | ブログ(2014 年のデモのレビュー) | Unity のデモ、操作、声 |
| https://goldplatedgames.com/2018/10/15/review-dark-deception/ | ブログ(2018 年のレビュー) | 初期の Ch1、ジャンプスケアへの評価、残機、Bierce の人物像 |
| https://gameplay.tips/guides/9780-dark-deception.html | ブログ(攻略) | Monkey Business の流れ、残機、操作、特殊シャード |
| https://steamcommunity.com/sharedfiles/filedetails/?id=2578820031 | フォーラム(Steam ガイド) | Elementary Evil のゾーン、S ランクの条件、脱出 |
| https://en.namu.wiki/w/Dark%20Deception/%EC%B1%95%ED%84%B0 | 信頼性低 | 使っていない |
| 本リポジトリ `public/sfx/manifest.json` | 公式サウンドパックのファイル名 | 音の分類、ステージ番号と舞台の推測 |
| https://www.youtube.com/watch?v=bsP5T1zbAHw | 動画(SmackNPie、DDE + Chapter 4 のメニュー、v1.7.0) | メインメニューの配置、ホバー表現、SETTINGS ダイアログ(2026-09-11 に追加。観察なので [推測] 扱い) |
| https://www.youtube.com/watch?v=fiWmeIArPNU | 動画のサムネイル(Chapter 4 のメインメニュー) | メニュー項目の並び(RESUME〜QUIT)の確認 |
| ユーザー提供の画面収録(2026-09-11、9.5 秒、音声なし。YouTube の動画「Dark Deception『Chapter 1 - 3：S Rank TOUR』」の全画面再生を収録したもの。版表記 v1.5.3) | 動画 | メニューのホバー、NEW GAME の RESTART? ダイアログの見た目と開閉の時間、YES の後の暗転(観察なので [推測] 扱い) |

本作 README の「参考」には、操作の出典(Tablet、Death Tips、Powers、Soul Shards の各 wiki ページ、Steam の操作ガイド 2765368098)がある。

## 否定された主張(使わない)

| 主張 | 票 | 誤りの箇所 |
| --- | --- | --- |
| ステージごとに固有の敵がいて、個別の AI を持つ(Steam の宣伝文を仕様として扱ったもの) | 1-2 | 宣伝文としては本物だが、実際は複数のステージに出る敵(Malak)や、1 ステージに複数種いる例がある |
| UE4 製で、Ch1 2018-09-27、Ch2 2019-01-22、Ch3 2019-06-24、Ch4 2021-09-28 に発売し、Ch5 は未発表 | 0-3 | 開発元とエンジンは正しいとされた。誤りは Steam の日付との食い違い(Ch1 は 9/26、Ch2 は 1/21)と、Ch5 を「未発表」とした点(Steam ページがあり、発売日が未定なだけ) |
| 赤いシャードは 60 秒、Stun Orb は 15 秒で、Malak と Trigger Teddies は気絶しない | 0-3 | Trigger Teddies の部分が誤り(気絶アニメがないだけで、Stun Orb で気絶する)。60 秒、15 秒、Malak の免疫は wiki 本文にある |
| パワーは Ch2〜5 で 1 つずつ解放され、2 つ装備してタブレットの 2 つの円で切り替える | 0-3 | 「Ch2〜5」が誤り(公開済みの 6 種は Ch2〜4 で解放、Ch5 は未公開)。2 枠の仕様は裏付けがある |
| 全回収で全敵が "Nightmare Mode" になり、赤黒くなって速くなる | 0-3 | 一般化しすぎ。変身しない敵(Malak、ボス、Clown Cars)、消える敵(Gold Watchers)、回収前に変身するステージがある。色も「暗くなる、または赤黒」 |
| Murder Monkeys は常に位置を知っていて、パワーなしで避けられる唯一の敵 | 0-3 | TV Tropes の意見を事実にしたもの。Ch1 にはパワーがないので「唯一」は成り立たない |
| Agatha はステージ唯一の敵でボスでもあり、隠れても長くはもたない | 0-3 | 断定しすぎ(Malak も出る、見失うと巡回に戻るなど) |

## 未解決の問い

- UE4 製であることの一次資料(Glowstick の発表、Unreal の事例紹介など)。
- 捕まったときのジャンプスケアや捕獲演出の作り(カメラの動き、時間、画面効果)と、音で危険を知らせる設計の詳細。
- Red Shard と Stun Orb の効果時間の現行値、ボスや各敵への効き方。
- S ランクの判定条件の全ステージ共通の定義と、獲得 XP の計算方法。
- 各敵の移動速度の実数値、Doug の通常速度と疾走速度(Speed Boost の 870 と比べるため)。
- 敵 AI の実装方式(経路探索、知覚システム)についての、データマイニングや開発者インタビューなどの一次情報。
- Deadly Decadence の主な敵(Gold Watchers と推測)と、Trigger Teddies の登場ステージの確認。

## 調査の制約

- Fandom は直接取得すると Cloudflare に阻まれる(HTTP 402)ので、MediaWiki API の wikitext で読んだ。
- 一部の検証者は Web 検索の予算を使い切り、外部の反証を探しきれていない。
- 抽出した 95 件のうち、検証したのは 25 件。残りは各ファイルで [C] として区別している。
